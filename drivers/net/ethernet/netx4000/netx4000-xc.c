/*
* MDIO XC driver for Hilscher netx4000 based platforms
*
* drivers/net/ethernet/netx4000-xc.c
*
* (C) Copyright 2015 Hilscher Gesellschaft fuer Systemautomation mbH
* http://www.hilscher.com
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; version 2 of
* the License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
*/

#include <linux/init.h>
#include <linux/device.h>
#include <linux/firmware.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/export.h>
#include <linux/of.h>
#include <linux/of_fdt.h>

#include <mach/hardware.h>
#include <mach/irqs.h>

#include "netx4000-xc.h"
#include "netx4000-xceth-hal.h"

static DEFINE_MUTEX(xc_lock);

static uint32_t xc_in_use = 0;

//TODO:
#define Addr_NX4000_xc_start_stop 0xF4023A80U

/* Code to support firmware files provided in DT format */
/*
-------------------------------------------------------------------------------------------------------------------------
|                            xc0                            |                            xc1                            | eXcNo
-------------------------------------------------------------------------------------------------------------------------
|            xcPort0          |            xcPort1          |            xcPort0          |            xcPort1          | eXcPortNo
-------------------------------------------------------------------------------------------------------------------------
| rpec0 | tpec0 | rpu0 | tpu0 | rpec1 | tpec1 | rpu1 | tpu1 | rpec2 | tpec2 | rpu2 | tpu2 | rpec3 | tpec3 | rpu3 | tpu3 | eXcPortType
-------------------------------------------------------------------------------------------------------------------------
*/

typedef enum {
    XC0 = 0,
    XC1,
    XC_MAX
} eXcNo_t;

typedef enum {
    XC_PORT0 = 0,
    XC_PORT1,
    XC_PORT_MAX
} eXcPortNo_t;

typedef enum {
    XC_PORT_TYPE_RPEC = 0,
    XC_PORT_TYPE_TPEC,
    XC_PORT_TYPE_RPU,
    XC_PORT_TYPE_TPU,
    XC_PORT_TYPE_MAX
} eXcPortType_t;

struct xcunit {
    char name[8];
    eXcNo_t uXcNo;
    eXcPortNo_t uXcPortNo;
    eXcPortType_t eXcPortType;
};

#define for_all_xcs(xc) \
for (xc=0; xc<XC_MAX; xc++)

#define for_all_xc_ports(xc,port) \
for_all_xcs(xc) \
    for (port=0; port<XC_PORT_MAX; port++)

#define for_all_types(type) \
    for (type=0; type<XC_PORT_TYPE_MAX; type++)

#define for_all_xc_port_types(xc,port,type) \
for_all_xc_ports(xc,port) \
    for_all_types(type)

#define xcunit_index(xc,port,type) ((xc*XC_PORT_MAX*XC_PORT_TYPE_MAX) + (port*XC_PORT_TYPE_MAX) + type)

struct xcunit xcunit[] = {
    { "rpec0", XC0, XC_PORT0, XC_PORT_TYPE_RPEC},
    { "tpec0", XC0, XC_PORT0, XC_PORT_TYPE_TPEC},
    { "rpu0",  XC0, XC_PORT0, XC_PORT_TYPE_RPU},
    { "tpu0",  XC0, XC_PORT0, XC_PORT_TYPE_TPU},
    { "rpec1", XC0, XC_PORT1, XC_PORT_TYPE_RPEC},
    { "tpec1", XC0, XC_PORT1, XC_PORT_TYPE_TPEC},
    { "rpu1",  XC0, XC_PORT1, XC_PORT_TYPE_RPU},
    { "tpu1",  XC0, XC_PORT1, XC_PORT_TYPE_TPU},
    { "rpec2", XC1, XC_PORT0, XC_PORT_TYPE_RPEC},
    { "tpec2", XC1, XC_PORT0, XC_PORT_TYPE_TPEC},
    { "rpu2",  XC1, XC_PORT0, XC_PORT_TYPE_RPU},
    { "tpu2",  XC1, XC_PORT0, XC_PORT_TYPE_TPU},
    { "rpec3", XC1, XC_PORT1, XC_PORT_TYPE_RPEC},
    { "tpec3", XC1, XC_PORT1, XC_PORT_TYPE_TPEC},
    { "rpu3",  XC1, XC_PORT1, XC_PORT_TYPE_RPU},
    { "tpu3",  XC1, XC_PORT1, XC_PORT_TYPE_TPU}
};
struct firmware_data {
	char firmware[16];
	char version[16];
	uint32_t *xc_microcode[XC_PORT_TYPE_MAX];
};

struct fw_info {
	struct device_node* root;
	struct firmware* fw;
};

struct fw_info* get_root_node(struct port* p, char* name, struct fw_info* fw_info)
{
	int ret = 0;
	const struct firmware* fw = NULL;
	struct device_node* root = NULL;

	if (0 == (ret = request_firmware(&fw, name, p->dev))) {
		root = of_fdt_unflatten_tree( (unsigned long*)fw->data, p->dev->of_node, &root);
		if (root == NULL) {
			dev_err( p->dev, "%s: Failed to interprete firmware!\n", __func__);
			release_firmware( fw);
			fw_info = NULL;
		} else {
			fw_info->fw = (struct firmware*)fw;
			fw_info->root = root;
		}
	} else {
		dev_err( p->dev, "%s: Error requesting firmware %d!\n", __func__, ret);
		fw_info = NULL;
	}
	return fw_info;
}

void free_root_node(struct fw_info* fw_info)
{
	release_firmware( fw_info->fw);
}

static int parse_fw_dtb( struct port* p, struct xc_res* xc, struct firmware_data *fwdata)
{
	struct device_node *node;
	char name[256];
	int xcinst = xc->xcinst;
	int xcno = 0;
	int rc = -EAGAIN;
	struct fw_info fw;
	struct device_node* root = NULL;
	char* xc_name[2];

	snprintf(name, sizeof(name), "%s%d.bin", FIRMWARE_NAME, xcinst);

	if (NULL == get_root_node(p, name, &fw)) {
		return -ENOENT;
	}
	if ((rc = of_property_read_string_array(p->dev->of_node, "xc_res", (const char**)xc_name, p->no_of_xcinst)) > 0) {
		int index = xcinst;
		if (index != 0) {
			index = index % p->no_of_xcinst;
		}
		snprintf(name, sizeof(name), "%s", xc_name[index]);
	} else {
		dev_err( p->dev, "%s: Failed to read xc res array!\n", __func__);
		free_root_node( &fw);
		return -ENOENT;
	}
	root = fw.root;
	node = of_find_node_by_name(root, name);
	if (node) {
		struct property *prop;
		int type = 0;
		void *pv;

		rc = of_property_read_string(root, "firmware", (const char**)&pv);
		if (rc) {
			dev_err( p->dev, "%s: Reading firmware name failed\n", __func__);
			rc = -ENODATA;
			goto err_out;
		}
		strncpy(fwdata->firmware, pv, sizeof(fwdata->firmware));

		rc = of_property_read_string(root, "version", (const char**)&pv);
		if (rc) {
			dev_err( p->dev, "%s: Reading firmware version failed\n", __func__);
			rc = -ENODATA;
			goto err_out;
		}
		strncpy(fwdata->version, pv, sizeof(fwdata->version));

		for_all_xc_port_types(xcno, xcinst, type) {
			int i = xcunit_index(xcno, xcinst, type);

			prop = of_find_property(node, xcunit[i].name, NULL);
			if (!prop)
				continue;

			fwdata->xc_microcode[type] = kzalloc(max(8 /* program size, trailing loads size */, prop->length), GFP_KERNEL);
			if (!fwdata->xc_microcode[type]) {
				dev_err( p->dev, "%s: xzalloc() failed!\n", __func__);
				rc = -ENOMEM;
				goto err_out;
			}
			rc = of_property_read_u32_array( node, xcunit[i].name, fwdata->xc_microcode[type], prop->length/sizeof(uint32_t));
			if (IS_ERR((const void*)rc)) {
				dev_err( p->dev, "%s: Reading microcode failed!\n", __func__);
				rc = PTR_ERR((const void*)rc);
				goto err_out;
			}
		}
		rc = 0;
	} else {
		dev_err( p->dev, "%s: Parsing firmware failed!\n", __func__);
	}
	free_root_node( &fw);
err_out:
	return rc;
}

/*****************************************************************************/
/* Definitions                                                               */
/*****************************************************************************/

int port_set_mac_addr( struct port* p, ETH_MAC_ADDRESS_TYPE_E eType, ETHERNET_MAC_ADDR_T tMacAddr)
{
	return netx4000_xceth_set_mac_address( p->portno, eType, tMacAddr);
}

int port_get_mac_addr( struct port* p, ETH_MAC_ADDRESS_TYPE_E eType, ETHERNET_MAC_ADDR_T* ptMacAddr)
{
	return netx4000_xceth_get_mac_address( p->portno, eType, ptMacAddr);
}

int port_set_link_mode( struct port* p, int fValid, unsigned int uiSpeed, int fFdx)
{
	return netx4000_xceth_set_link_mode( p->portno, fValid, uiSpeed, fFdx);
}


int port_reset( struct port* p)
{
	int ret = 0;
	int i;

	for (i=0;i<p->no_of_xcinst;i++) {
		ret = xc_reset( p->xc_res[i]);
	}
	return ret;
}

int port_start( struct port* p)
{
	int ret = 0;
	int i;

	for (i=0;i<p->no_of_xcinst;i++) {
		ret = xc_start( p->xc_res[i]);
	}
	return ret;
}

int port_stop( struct port* p)
{
	int ret = 0;
	int i;

	for (i=0;i<p->no_of_xcinst;i++) {
		ret = xc_stop( p->xc_res[i]);
	}
	return ret;
}

int port_request_firmware(struct port *p)
{
	int ret = -EINVAL;
	int xc_res = 0;
	struct firmware_data fw;

	for (xc_res=0;xc_res<p->no_of_xcinst;xc_res++) {
		int type = 0;

		dev_dbg( p->dev, "%s: requesting firmware for %d\n", __func__, p->portno);
		ret = parse_fw_dtb( p, p->xc_res[xc_res], &fw);
		if (ret < 0) {
			return ret;
		}
		ret = -EINVAL;
		for_all_types(type) {
			ret = xc_load(  p->xc_res[xc_res], type, fw.xc_microcode[type]);
			if (ret < 0) {
				dev_err(p->dev, "%s: downloading firmware failed for type=%d (ret=%d)\n", __func__, type, ret);
				return ret;
			}
		}
	}
	return ret;
}

void port_confirm_irq( struct port* p, uint32_t mask)
{
	int i = 0;

	for (i=0;i<p->no_of_xcinst;i++) {
		//*p->xc_res[i]->xpec_irqregs->aulIrq_xpec[i] = 0xFFFF & mask;
		*p->xc_res[i]->xpec_irqregs = 0xFFFF & mask;
	}
}

uint32_t port_get_irq( struct port* p)
{
	int i = 0;
	uint32_t flags = 0;

	for (i=0;i<p->no_of_xcinst;i++) {
		flags |= 0xFFFF & *p->xc_res[i]->xpec_irqregs;
	}
	return flags;
}

int port_set_irq( struct port* p, uint32_t mask)
{
	int i = 0;

	for (i=0;i<p->no_of_xcinst;i++) {
		p->xc_res[i]->xpec_dpm->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_INTERRUPTS_ENABLE_IND_HI = mask & MSK_ETHMAC_INTERRUPTS_ENABLE_IND_HI_VAL;
		p->xc_res[i]->xpec_dpm->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_INTERRUPTS_ENABLE_IND_LO = mask & MSK_ETHMAC_INTERRUPTS_ENABLE_IND_LO_VAL;
		p->xc_res[i]->xpec_dpm->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_INTERRUPTS_ENABLE_CON_HI = mask & MSK_ETHMAC_INTERRUPTS_ENABLE_CON_HI_VAL;
		p->xc_res[i]->xpec_dpm->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_INTERRUPTS_ENABLE_CON_LO = mask & MSK_ETHMAC_INTERRUPTS_ENABLE_CON_LO_VAL;
		p->xc_res[i]->xpec_dpm->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_INTERRUPTS_ENABLE_LINK_CHANGED = mask & MSK_ETHMAC_INTERRUPTS_ENABLE_LINK_CHANGED_VAL;
		p->xc_res[i]->xpec_dpm->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_INTERRUPTS_ENABLE_COL = mask & MSK_ETHMAC_INTERRUPTS_ENABLE_COL_VAL;
		p->xc_res[i]->xpec_dpm->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_INTERRUPTS_ENABLE_EARLY_RCV = mask & MSK_ETHMAC_INTERRUPTS_ENABLE_EARLY_RCV_VAL;
		p->xc_res[i]->xpec_dpm->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_INTERRUPTS_ENABLE_RX_ERR = mask & MSK_ETHMAC_INTERRUPTS_ENABLE_RX_ERR_VAL;
		p->xc_res[i]->xpec_dpm->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_INTERRUPTS_ENABLE_TX_ERR = mask & MSK_ETHMAC_INTERRUPTS_ENABLE_TX_ERR_VAL;
	}
	return 0;
}

struct port *port_request( uint8_t port, struct device *dev)
{
	struct port *p = NULL;
	int xcinst = 0;
	int offset;
	int ret = -EAGAIN;

	dev_dbg( dev, "%s: requesting port %d\n", __func__, port);

	mutex_lock(&xc_lock);

	if (xc_in_use & (1 << port))
		goto exit;

	p = kzalloc(sizeof (struct port), GFP_KERNEL);
	if (!p)
		goto exit;

	if ( (p->no_of_xcinst = of_property_read_string_array(dev->of_node, "xc_res", NULL, 2)) > 0) {
		ret = 0;
		p->portno = port;
		offset = (p->portno * p->no_of_xcinst);
		for (xcinst = 0; xcinst < p->no_of_xcinst; xcinst++) {
			xc_units_enable( offset + xcinst);
			if ((p->xc_res[xcinst] = xc_alloc_xc_res(dev, offset + xcinst)) == NULL) {
				dev_err( p->dev, "%s: Failed to allocate xc resources!\n", __func__);
				ret = -EINVAL;
			}
		}
		if (ret == 0) {
			p->dev = dev;
			port_reset(p);
			if ((ret = port_request_firmware( p)) == 0)
				xc_in_use |= (1 << p->portno);
		}
		if (ret != 0) {
			for (xcinst = 0; xcinst < p->no_of_xcinst; xcinst++) {
				if (p->xc_res[xcinst] != NULL)
					xc_release_xc_res(p->xc_res[xcinst]);
				p->xc_res[xcinst] = NULL;
			}
		}
	}
	if (ret != 0) {
		dev_err( dev, "%s: Failed to initialize xc resources!\n", __func__);
		kfree(p);
		p = NULL;
	}
exit:
	mutex_unlock(&xc_lock);
	return p;
}

void port_free(struct port* p)
{
	int i = 0;

	mutex_lock(&xc_lock);
	if (xc_in_use & (1 << p->portno)) {
		xc_in_use &= ~(1 << p->portno);
		for (i=0;i<p->no_of_xcinst;i++) {
			int xcinst = p->xc_res[i]->xcinst;
			xc_release_xc_res( p->xc_res[i]);
			xc_units_disable( xcinst);
		}
		kfree(p);
	}
	mutex_unlock(&xc_lock);
}

#define ASIC_CTRL_ACCESS_KEY 0xf408017c

/* misc clock is set by phy driver */
#define XPIC_CLOCK(port) (0x20000<<port)
#define XMAC_CLOCK(port) (0x00100<<port)
#define TPEC_CLOCK(port) (0x00010<<port)
#define RPEC_CLOCK(port) (0x00001<<port)

#define XC_PORT_CLOCK(port) (XPIC_CLOCK(port)|XMAC_CLOCK(port)|TPEC_CLOCK(port)|RPEC_CLOCK(port))

void xc_units_enable( uint8_t xcinst) {

	if ((xc_in_use & (1 << xcinst)) == 0) {
		/* enable clocks */
		volatile uint32_t* clock = ioremap(0xf4080138, sizeof(uint32_t));
		volatile uint32_t* key_addr = ioremap(ASIC_CTRL_ACCESS_KEY, sizeof(key_addr));
		if (clock != NULL) {
			uint32_t val = ioread32(clock);
			uint32_t keyval = ioread32(key_addr);
			iowrite32(keyval, key_addr);
			writel((val|XC_PORT_CLOCK(xcinst)), clock);
			iounmap(key_addr);
			iounmap(clock);
		}
	}
}

void xc_units_disable(uint8_t xcinst) {
	if ((xc_in_use & (1 << xcinst)) == 0) {
		/* disable clocks */
		volatile uint32_t* clock = ioremap(0xf4080138, sizeof(uint32_t));
		volatile uint32_t* key_addr = ioremap(ASIC_CTRL_ACCESS_KEY, sizeof(key_addr));
		if (clock != NULL) {
			uint32_t val = ioread32(clock);
			uint32_t keyval = ioread32(key_addr);
			iowrite32(keyval, key_addr);
			writel((val&~XC_PORT_CLOCK(xcinst)), clock);
			iounmap(key_addr);
			iounmap(clock);
		}
	}
}

EXPORT_SYMBOL(port_free);
EXPORT_SYMBOL(port_request);
EXPORT_SYMBOL(port_request_firmware);
EXPORT_SYMBOL(port_reset);
EXPORT_SYMBOL(port_set_irq);
EXPORT_SYMBOL(port_get_irq);
EXPORT_SYMBOL(port_confirm_irq);
EXPORT_SYMBOL(port_start);
EXPORT_SYMBOL(port_stop);
EXPORT_SYMBOL(port_set_link_mode);
EXPORT_SYMBOL(port_get_mac_addr);
EXPORT_SYMBOL(port_set_mac_addr);
