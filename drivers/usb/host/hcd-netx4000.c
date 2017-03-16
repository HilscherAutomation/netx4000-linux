/*
* USB HCD driver for Hilscher netx4000 based platforms
*
* drivers/usb/host/hcd-netx4000.c
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
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#define DRIVER_DESC  "USB HCD driver for Hilscher netx4000 based platforms"
#define DRIVER_NAME "hcd-netx4000"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/usb/ehci_pdriver.h>
#include <linux/usb/ohci_pdriver.h>

#include <mach/hardware.h>
#include <mach/platform.h>

struct netx4000_priv {
	struct platform_device *pdev;
	struct platform_device *ehci_dev;
	struct platform_device *ohci_dev;
	
	struct resource *res_mem; /* physical baseaddr */
	void *ba; /* virtual baseaddr */
	unsigned int irq;

	unsigned int num_ports;

	unsigned int ahbpci_win1_ctr;
	unsigned int ahbpci_win2_ctr;
	unsigned int pciahb_win1_ctr;
	unsigned int pciahb_win2_ctr;

	unsigned int pciahb_win1_base_address;
	unsigned int pciahb_win2_base_address;
	unsigned int ahbpci_bridge_base_address;

	unsigned int ohci_base_address;
	unsigned int ehci_base_address;
};

static inline int32_t ioset32(uint32_t setmask, void *addr)
{
	uint32_t val;

	val = readl(addr);
	writel(val | setmask, addr);

	return 0;
}

static inline int32_t ioclear32(uint32_t clearmask, void *addr)
{
	uint32_t val;

	val = readl(addr);
	writel(val & ~clearmask, addr);

	return 0;
}

static int netx4000_hcd_chip_reset(struct netx4000_priv *priv)
{
	NX4000_USB_HOST_AREA_T *chip = priv->ba;

	ioset32((0x3 << 10) | /* win1 size 2gb */
		MSK_NX4000_USB_HOST_USBCTR_PLL_RST |
		MSK_NX4000_USB_HOST_USBCTR_USBH_RST,
		(void*)&chip->ulUSB_HOST_USBCTR);
	udelay(5);
	ioclear32(MSK_NX4000_USB_HOST_USBCTR_PLL_RST |
		MSK_NX4000_USB_HOST_USBCTR_PCICLK_MASK |
		MSK_NX4000_USB_HOST_USBCTR_USBH_RST,
		(void*)&chip->ulUSB_HOST_USBCTR);

	return 0;
}

#define USB2CFG  (NETX4000_SYSTEMCTRL_VIRT_BASE + 0x10)
#define netx4000_hcd_power_up()  ioclear32(0x1, (void*)USB2CFG)
#define netx4000_hcd_power_down()  ioset32(0x1, (void*)USB2CFG)
#define netx4000_hcd_h2mode_disable()  ioclear32(0x2, (void*)USB2CFG)
#define netx4000_hcd_h2mode_enable()  ioset32(0x2, (void*)USB2CFG)
static int netx4000_hcd_chip_init(struct netx4000_priv *priv)
{
	NX4000_USB_HOST_AREA_T *chip = priv->ba;

	if (priv->num_ports > 1)
		netx4000_hcd_h2mode_enable();

	if (netx4000_periph_clock_enable(NETX4000_USB_CLOCK_EN)) {
		dev_err(&priv->pdev->dev, "netx4000_periph_clock_enable() failed\n");
		return -EIO;
	}
	netx4000_hcd_power_up();

	netx4000_hcd_chip_reset(priv);

	/* AHB-PCI Bridge PCI Communiction Registers */
	writel(MSK_NX4000_USB_HOST_AHB_BUS_CTR_SMODE_READY_CTR |
		MSK_NX4000_USB_HOST_AHB_BUS_CTR_MMODE_HBUSREQ |
		MSK_NX4000_USB_HOST_AHB_BUS_CTR_MMODE_WR_INCR |
		MSK_NX4000_USB_HOST_AHB_BUS_CTR_MMODE_BYTE_BURST |
		MSK_NX4000_USB_HOST_AHB_BUS_CTR_MMODE_HTRANS,
		&chip->ulUSB_HOST_AHB_BUS_CTR);

	writel((priv->pciahb_win1_ctr & MSK_NX4000_USB_HOST_PCIAHB_WIN1_CTR_AHB_BASEADR) |
		(0 << SRT_NX4000_USB_HOST_PCIAHB_WIN1_CTR_ENDIAN_CTR) |
		(3 << SRT_NX4000_USB_HOST_PCIAHB_WIN1_CTR_PREFETCH),
		&chip->ulUSB_HOST_PCIAHB_WIN1_CTR); /* (1)-a */

	writel((priv->pciahb_win2_ctr & MSK_NX4000_USB_HOST_PCIAHB_WIN2_CTR_AHB_BASEADR) |
		(0 << SRT_NX4000_USB_HOST_PCIAHB_WIN2_CTR_ENDIAN_CTR) |
		(3 << SRT_NX4000_USB_HOST_PCIAHB_WIN2_CTR_PREFETCH),
		&chip->ulUSB_HOST_PCIAHB_WIN2_CTR); /* (1)-b */

	writel((priv->ahbpci_win2_ctr & MSK_NX4000_USB_HOST_AHBPCI_WIN2_CTR_PCIWIN2_BASEADR) |
		(0 << SRT_NX4000_USB_HOST_AHBPCI_WIN2_CTR_BURST_EN) | 
		(3 << SRT_NX4000_USB_HOST_AHBPCI_WIN2_CTR_PCICMD),
		&chip->ulUSB_HOST_AHBPCI_WIN2_CTR); /* (2) */

	writel(0x000b103f,&chip->ulUSB_HOST_PCI_INT_ENABLE);

	writel(MSK_NX4000_USB_HOST_PCI_ARBITER_CTR_PCIBP_MODE |
		MSK_NX4000_USB_HOST_PCI_ARBITER_CTR_PCIREQ0 |
		MSK_NX4000_USB_HOST_PCI_ARBITER_CTR_PCIREQ1,
		&chip->ulUSB_HOST_PCI_ARBITER_CTR);

	/* PCI Configuration Registers for AHB-PCI Bridge */

	writel((0x40000000 & MSK_NX4000_USB_HOST_AHBPCI_WIN1_CTR_PCIWIN1_BASEADR) |
		(5 << SRT_NX4000_USB_HOST_AHBPCI_WIN1_CTR_PCICMD),
		&chip->ulUSB_HOST_AHBPCI_WIN1_CTR); /* (3) */

	writel(MSK_NX4000_USB_HOST_CMND_STS_OHCI_SERR_ENABLE |
		MSK_NX4000_USB_HOST_CMND_STS_OHCI_PARITY_ERROR_RESPONSE |
		MSK_NX4000_USB_HOST_CMND_STS_OHCI_BUS_MASTER |
		MSK_NX4000_USB_HOST_CMND_STS_OHCI_MEMORY_SPACE,
		&chip->ulUSB_HOST_CMND_STS_OHCI);

	writel(priv->ahbpci_bridge_base_address, &chip->ulUSB_HOST_BASEAD_OHCI); /* (4) */
	writel(priv->pciahb_win1_base_address, &chip->ulUSB_HOST_WIN1_BASEAD); /* (5)-a */
	writel(priv->pciahb_win2_base_address, &chip->ulUSB_HOST_WIN2_BASEAD); /* (5)-b */

	/* PCI Configuration Registers for OHCI/EHCI */

	writel((0x80000000 & MSK_NX4000_USB_HOST_AHBPCI_WIN1_CTR_PCIWIN1_BASEADR) |
		(5 << SRT_NX4000_USB_HOST_AHBPCI_WIN1_CTR_PCICMD),
		&chip->ulUSB_HOST_AHBPCI_WIN1_CTR); /* (6) */

	writel(MSK_NX4000_USB_HOST_CMND_STS_OHCI_SERR_ENABLE |
		MSK_NX4000_USB_HOST_CMND_STS_OHCI_PARITY_ERROR_RESPONSE |
		MSK_NX4000_USB_HOST_CMND_STS_OHCI_BUS_MASTER |
		MSK_NX4000_USB_HOST_CMND_STS_OHCI_MEMORY_SPACE,
		&chip->ulUSB_HOST_CMND_STS_OHCI);

	writel(MSK_NX4000_USB_HOST_CMND_STS_OHCI_SERR_ENABLE |
		MSK_NX4000_USB_HOST_CMND_STS_OHCI_PARITY_ERROR_RESPONSE |
		MSK_NX4000_USB_HOST_CMND_STS_OHCI_BUS_MASTER |
		MSK_NX4000_USB_HOST_CMND_STS_OHCI_MEMORY_SPACE,
		&chip->ulUSB_HOST_CMND_STS_EHCI);

	writel(priv->ohci_base_address, &chip->ulUSB_HOST_BASEAD_OHCI); /* (7) */
	writel(priv->ehci_base_address, &chip->ulUSB_HOST_BASEAD_EHCI); /* (7) */

	/* UTMI */

//	writel(0x0, &chip->ulUSB_HOST_UTMICTRL_OHCI); // it's the same register as ...
	writel(0x0, &chip->ulUSB_HOST_UTMICTRL_EHCI);

	return 0;
}

static const struct usb_ehci_pdata ehci_pdata = {
};

static const struct usb_ohci_pdata ohci_pdata = {
};

static struct platform_device *netx4000_hcd_create_pdev(struct netx4000_priv *priv, bool ehci)
{
	struct platform_device *hci_dev;
	struct resource hci_res[2];
	int ret;

	memset(hci_res, 0, sizeof(hci_res));

	hci_res[0].start = priv->res_mem->start + ((ehci) ? 0x1000 : 0);
	hci_res[0].end = hci_res[0].start + 0x1000 - 1;
	hci_res[0].flags = IORESOURCE_MEM;

	hci_res[1].start = priv->irq;
	hci_res[1].flags = IORESOURCE_IRQ;

	hci_dev = platform_device_alloc(ehci ? "ehci-platform" : "ohci-platform" , 0);
	if (!hci_dev)
		return ERR_PTR(-ENOMEM);

	hci_dev->dev.parent = &priv->pdev->dev;
	hci_dev->dev.dma_mask = &hci_dev->dev.coherent_dma_mask;

	ret = platform_device_add_resources(hci_dev, hci_res, ARRAY_SIZE(hci_res));
	if (ret)
		goto err_alloc;
	if (ehci)
		ret = platform_device_add_data(hci_dev, &ehci_pdata, sizeof(ehci_pdata));
	else
		ret = platform_device_add_data(hci_dev, &ohci_pdata, sizeof(ohci_pdata));
	if (ret)
		goto err_alloc;
	ret = platform_device_add(hci_dev);
	if (ret)
		goto err_alloc;
		

	return hci_dev;

err_alloc:
	platform_device_put(hci_dev);
	return ERR_PTR(ret);
}

static int netx4000_hcd_probe(struct platform_device *pdev)
{
	struct netx4000_priv *priv;
	int rc;

	priv = devm_kzalloc(&pdev->dev, sizeof(struct netx4000_priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->pdev = pdev;
	platform_set_drvdata(pdev, priv);
	
	priv->res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!priv->res_mem) {
		dev_err(&pdev->dev, "reg not provided in DT");
		rc = -EINVAL;
		goto err_out;
	}
	priv->ba = devm_ioremap(&pdev->dev, priv->res_mem->start, resource_size(priv->res_mem));
	if (IS_ERR(priv->ba)) {
		rc = PTR_ERR(priv->ba);
		goto err_out;
	}

	priv->irq = platform_get_irq(pdev, 0);
	if (priv->irq < 0) {
		dev_err(&pdev->dev, "interrupts not provided in DT");
		rc = -EINVAL;
		goto err_out;
	}

	rc = of_property_read_u32(pdev->dev.of_node, "num-ports", &priv->num_ports);
	if (rc) {
		dev_warn(&pdev->dev, "num_ports not provided in DT => defaulting to one port!");
		priv->num_ports = 1;
	}
	else if ((priv->num_ports < 1) || (priv->num_ports > 2)) {
		dev_err(&pdev->dev, "num_ports out of range [1,2]");
		rc = -EINVAL;
		goto err_out;
	}

	/* Initialize the USB chip */

	priv->pciahb_win1_ctr = PHYS_OFFSET; /* DDR-RAM 0x40000000 */
	priv->pciahb_win2_ctr = 0x0; /* unused */
	priv->pciahb_win1_base_address = PHYS_OFFSET; /* DDR-RAM 0x40000000 */
	priv->pciahb_win2_base_address = 0x0; /* unused */

	priv->ahbpci_win1_ctr = 0x0; /* xxx */
	priv->ahbpci_win2_ctr = priv->res_mem->start; /* 0xf9000000 */
	priv->ahbpci_bridge_base_address = priv->ahbpci_win2_ctr+0x10800; /* 0xf9010800 */

	priv->ohci_base_address = priv->ahbpci_win2_ctr;
	priv->ehci_base_address = priv->ohci_base_address+0x1000;

	rc = netx4000_hcd_chip_init(priv);
	if (rc)
		goto err_out;

	/* Add USB devices */

	priv->ehci_dev = netx4000_hcd_create_pdev(priv, true);
	if (IS_ERR(priv->ehci_dev)) {
		rc = PTR_ERR(priv->ehci_dev);
		goto err_out;
	}

	priv->ohci_dev = netx4000_hcd_create_pdev(priv, false);
	if (IS_ERR(priv->ohci_dev)) {
		rc = PTR_ERR(priv->ohci_dev);
		goto err_out;
	}

	dev_info(&pdev->dev, "successfully initialized!\n");
	
	return 0;

err_out:
	if (!IS_ERR_OR_NULL(priv->ohci_dev))
		platform_device_unregister(priv->ohci_dev);
	if (!IS_ERR_OR_NULL(priv->ehci_dev))
		platform_device_unregister(priv->ehci_dev);

	return rc;
}

static int netx4000_hcd_remove(struct platform_device *pdev)
{
	struct netx4000_priv *priv = platform_get_drvdata(pdev);
	NX4000_USB_HOST_AREA_T *chip = priv->ba;
	
	if (priv->ohci_dev)
		platform_device_unregister(priv->ohci_dev);
	if (priv->ehci_dev)
		platform_device_unregister(priv->ehci_dev);

	ioset32(MSK_NX4000_USB_HOST_USBCTR_PLL_RST | MSK_NX4000_USB_HOST_USBCTR_USBH_RST, (void*)&chip->ulUSB_HOST_USBCTR);

	netx4000_hcd_power_down();

	dev_info(&pdev->dev, "successfully removed!\n");
	
	return 0;
}

static const struct of_device_id netx4000_hcd_of_match[] = {
	{.compatible = "hilscher,netx4000-hcd",},
	{},
};
MODULE_DEVICE_TABLE(of, netx4000_hcd_of_match);

static struct platform_driver netx4000_hcd_driver = {
	.probe		= netx4000_hcd_probe,
	.remove		= netx4000_hcd_remove,
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table = netx4000_hcd_of_match,
	},
};

static int __init netx4000_hcd_init(void)
{
	pr_info("%s: %s\n", DRIVER_NAME, DRIVER_DESC);
	platform_driver_register(&netx4000_hcd_driver);

	return 0;
}
module_init(netx4000_hcd_init);

static void __exit netx4000_hcd_exit(void)
{
	platform_driver_unregister(&netx4000_hcd_driver);
}
module_exit(netx4000_hcd_exit);

/* --- Module information --- */

MODULE_AUTHOR("Hilscher Gesellschaft fuer Systemautomation mbH");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_ALIAS("platform:"DRIVER_NAME);
MODULE_LICENSE("GPL v2");

