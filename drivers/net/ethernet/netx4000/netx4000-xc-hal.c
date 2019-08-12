
/* TODO: Perhaps, we could define it by yocto recipe. */
// #define __BAREBOX_CODE 1

#ifdef __BAREBOX_CODE
// 	#warning "BAREBOX_CODE"
	#include <common.h>
	#include <init.h>

	#include <mach/hardware.h>
	#include "netx4000-xc-hal.h"
	#include "netx4000-xc-main.h"

	/* Remove comment tags below for using linux code */
// 	#define request_mem_region(start,n,name)   request_iomem_region(name, start, start+n-1)
// 	#define release_region(start,n)            release_region(start)
// 	#define ioremap(res_cookie,size)           IOMEM(res_cookie)

#else
// 	#warning "LINUX_CODE"
	/* Linux specific code */
	#include <linux/slab.h>
	#include <linux/io.h>
	#include <linux/module.h>
	#include <linux/init.h>
	#include <linux/device.h>

	#include "netx4000-xc-hal.h"

	#define Addr_NX4000_xc_xpec_irq_registers 0xF4023A60U
	#define HW_PTR_XPEC_IRQ_REGS(var)   static volatile uint32_t*  const var   = (volatile uint32_t*) Addr_NX4000_xc_xpec_irq_registers;
	HW_PTR_XPEC_IRQ_REGS(s_ptXpecIrqRegs)

#endif

/*****************************************************************************/
/* Definitions                                                               */
/*****************************************************************************/

#define NUM_XPECS_PER_XC 2
#define NUM_XMACS_PER_XC 2

#define NUM_XPECS 4
#define NUM_XMACS 4

#define XMAC_RPU_DWORD_RAMSIZE 0x0100
#define XMAC_TPU_DWORD_RAMSIZE 0x0100
#define XPEC_DWORD_RAMSIZE     0x0800

/*****************************************************************************/
/*  Variables                                                                */
/*****************************************************************************/

static const uint32_t XcCode_rpu_reset0[27] = {
  0x00000064, /* program size */
  0x00000000, /* trailing loads size */
  0xf4020000, /* start address */
  0x01300001, 0xe15bde81, 0x01380001, 0xe15bde82, 0x0143fffd, 0xe15bde83, 0x0147fffd, 0xe15bde84,
  0x01480001, 0xe15bde85, 0x0143fff9, 0xe15bde86, 0x014bfffd, 0xe15bde87, 0x01440001, 0xe15bde88,
  0x0143fffd, 0xe15bde89, 0x01480001, 0xe15bde8a, 0x01380005, 0xe15bde8b, 0x01080000, 0x001fde8b,
  /* trailing loads */
};

static const uint32_t XcCode_rpu_reset1[27] = {
  0x00000064, /* program size */
  0x00000000, /* trailing loads size */
  0xf4020800, /* start address */
  0x01300001, 0xe15bde81, 0x01380001, 0xe15bde82, 0x0143fffd, 0xe15bde83, 0x0147fffd, 0xe15bde84,
  0x01480001, 0xe15bde85, 0x0143fff9, 0xe15bde86, 0x014bfffd, 0xe15bde87, 0x01440001, 0xe15bde88,
  0x0143fffd, 0xe15bde89, 0x01480001, 0xe15bde8a, 0x01380005, 0xe15bde8b, 0x01080000, 0x001fde8b,
  /* trailing loads */
};

static const uint32_t XcCode_rpu_reset2[27] = {
  0x00000064, /* program size */
  0x00000000, /* trailing loads size */
  0xf4021000, /* start address */
  0x01300001, 0xe15bde81, 0x01380001, 0xe15bde82, 0x0143fffd, 0xe15bde83, 0x0147fffd, 0xe15bde84,
  0x01480001, 0xe15bde85, 0x0143fff9, 0xe15bde86, 0x014bfffd, 0xe15bde87, 0x01440001, 0xe15bde88,
  0x0143fffd, 0xe15bde89, 0x01480001, 0xe15bde8a, 0x01380005, 0xe15bde8b, 0x01080000, 0x001fde8b,
  /* trailing loads */
};

static const uint32_t XcCode_rpu_reset3[27] = {
  0x00000064, /* program size */
  0x00000000, /* trailing loads size */
  0xf4021800, /* start address */
  0x01300001, 0xe15bde81, 0x01380001, 0xe15bde82, 0x0143fffd, 0xe15bde83, 0x0147fffd, 0xe15bde84,
  0x01480001, 0xe15bde85, 0x0143fff9, 0xe15bde86, 0x014bfffd, 0xe15bde87, 0x01440001, 0xe15bde88,
  0x0143fffd, 0xe15bde89, 0x01480001, 0xe15bde8a, 0x01380005, 0xe15bde8b, 0x01080000, 0x001fde8b,
  /* trailing loads */
};

static const uint32_t XcCode_tpu_reset0[27] = {
  0x00000064, /* program size */
  0x00000000, /* trailing loads size */
  0xf4020400, /* start address */
  0x014c0601, 0xe15bde81, 0x01540001, 0xe15bde82, 0x015ffffd, 0xe15bde83, 0x0163fffd, 0xe15bde84,
  0x01640001, 0xe15bde85, 0x015ffff9, 0xe15bde86, 0x0167fffd, 0xe15bde87, 0x01600001, 0xe15bde88,
  0x015ffffd, 0xe15bde89, 0x01640001, 0xe15bde8a, 0x01540005, 0xe15bde8b, 0x01080000, 0x001fde8b,
  /* trailing loads */
};

static const uint32_t XcCode_tpu_reset1[27] = {
  0x00000064, /* program size */
  0x00000000, /* trailing loads size */
  0xf4020c00, /* start address */
  0x014c0601, 0xe15bde81, 0x01540001, 0xe15bde82, 0x015ffffd, 0xe15bde83, 0x0163fffd, 0xe15bde84,
  0x01640001, 0xe15bde85, 0x015ffff9, 0xe15bde86, 0x0167fffd, 0xe15bde87, 0x01600001, 0xe15bde88,
  0x015ffffd, 0xe15bde89, 0x01640001, 0xe15bde8a, 0x01540005, 0xe15bde8b, 0x01080000, 0x001fde8b,
  /* trailing loads */
};

static const uint32_t XcCode_tpu_reset2[27] = {
  0x00000064, /* program size */
  0x00000000, /* trailing loads size */
  0xf4021400, /* start address */
  0x014c0601, 0xe15bde81, 0x01540001, 0xe15bde82, 0x015ffffd, 0xe15bde83, 0x0163fffd, 0xe15bde84,
  0x01640001, 0xe15bde85, 0x015ffff9, 0xe15bde86, 0x0167fffd, 0xe15bde87, 0x01600001, 0xe15bde88,
  0x015ffffd, 0xe15bde89, 0x01640001, 0xe15bde8a, 0x01540005, 0xe15bde8b, 0x01080000, 0x001fde8b,
  /* trailing loads */
};

static const uint32_t XcCode_tpu_reset3[27] = {
  0x00000064, /* program size */
  0x00000000, /* trailing loads size */
  0xf4021c00, /* start address */
  0x014c0601, 0xe15bde81, 0x01540001, 0xe15bde82, 0x015ffffd, 0xe15bde83, 0x0163fffd, 0xe15bde84,
  0x01640001, 0xe15bde85, 0x015ffff9, 0xe15bde86, 0x0167fffd, 0xe15bde87, 0x01600001, 0xe15bde88,
  0x015ffffd, 0xe15bde89, 0x01640001, 0xe15bde8a, 0x01540005, 0xe15bde8b, 0x01080000, 0x001fde8b,
  /* trailing loads */
};

/* Use the same reset code binaries (placeholder for different reset codes) */
#define XcCode_xc0_rpu_reset0 XcCode_rpu_reset0
#define XcCode_xc0_rpu_reset1 XcCode_rpu_reset1

#define XcCode_xc1_rpu_reset0 XcCode_rpu_reset2
#define XcCode_xc1_rpu_reset1 XcCode_rpu_reset3

#define XcCode_xc0_tpu_reset0 XcCode_tpu_reset0
#define XcCode_xc0_tpu_reset1 XcCode_tpu_reset1

#define XcCode_xc1_tpu_reset0 XcCode_tpu_reset2
#define XcCode_xc1_tpu_reset1 XcCode_tpu_reset3

static const uint32_t* paulxMacRpuCodes[4]=
{
  XcCode_xc0_rpu_reset0,
  XcCode_xc0_rpu_reset1,
  XcCode_xc1_rpu_reset0,
  XcCode_xc1_rpu_reset1,
};

static const uint32_t* paulxMacTpuCodes[4]=
{
  XcCode_xc0_tpu_reset0,
  XcCode_xc0_tpu_reset1,
  XcCode_xc1_tpu_reset0,
  XcCode_xc1_tpu_reset1,
};

static uint32_t refcount = 0;

struct resource *startstop_res = NULL;
struct resource *irqregs_res = NULL;
struct resource *dpm_res = NULL;

#ifdef __BAREBOX_CODE

int xc_release_xc_res(struct xc_res *xcRes)
{
	if (xcRes->rpec_res)
		release_region(xcRes->rpec_res);
	if (xcRes->tpec_res)
		release_region(xcRes->tpec_res);
	if (xcRes->rpu_res)
		release_region(xcRes->rpu_res);
	if (xcRes->tpu_res)
		release_region(xcRes->tpu_res);

	if (xcRes->rpec_regs_res)
		release_region(xcRes->rpec_regs_res);
	if (xcRes->tpec_regs_res)
		release_region(xcRes->tpec_regs_res);
	if (xcRes->xmac_regs_res)
		release_region(xcRes->xmac_regs_res);

	if (--refcount == 0) {
		if (startstop_res)
			release_region(startstop_res);
		if (irqregs_res)
			release_region(irqregs_res);
		if (dpm_res)
			release_region(dpm_res);
	}

	kfree(xcRes);

	return 0;
}

struct xc_res *xc_alloc_xc_res(struct device_d *dev, uint32_t uiPort)
{
	struct xc_res *xcRes;
	struct resource *res;

	xcRes = kzalloc(sizeof(*xcRes),GFP_KERNEL);
	if (!xcRes)
		return NULL;

	do {
		res = request_iomem_region(dev_name(dev), (resource_size_t)s_apulRpecPramArea[uiPort], (resource_size_t)s_apulRpecPramAreaEnd[uiPort]+sizeof(uint32_t)-1);
		if (IS_ERR(res))
			break;
		xcRes->rpec_res = res;

		res = request_iomem_region(dev_name(dev), (resource_size_t)s_apulTpecPramArea[uiPort], (resource_size_t)s_apulTpecPramAreaEnd[uiPort]+sizeof(uint32_t)-1);
		if (IS_ERR(res))
			break;
		xcRes->tpec_res = res;

		res = request_iomem_region(dev_name(dev), (resource_size_t)s_aptRpuPramArea[uiPort], (resource_size_t)s_aptRpuPramAreaEnd[uiPort]+sizeof(uint32_t)-1);
		if (IS_ERR(res))
			break;
		xcRes->rpu_res = res;

		res = request_iomem_region(dev_name(dev), (resource_size_t)s_aptTpuPramArea[uiPort], (resource_size_t)s_aptTpuPramAreaEnd[uiPort]+sizeof(uint32_t)-1);
		if (IS_ERR(res))
			break;
		xcRes->tpu_res = res;

		res = request_iomem_region(dev_name(dev), (resource_size_t)s_aptRpecRegArea[uiPort], (resource_size_t)(s_aptRpecRegArea[uiPort]+1)-1);
		if (IS_ERR(res))
			break;
		xcRes->rpec_regs_res = res;

		res = request_iomem_region(dev_name(dev), (resource_size_t)s_aptTpecRegArea[uiPort], (resource_size_t)(s_aptTpecRegArea[uiPort]+1)-1);
		if (IS_ERR(res))
			break;
		xcRes->tpec_regs_res = res;

		res = request_iomem_region(dev_name(dev), (resource_size_t)s_aptXmacArea[uiPort], (resource_size_t)(s_aptXmacArea[uiPort]+1)-1);
		if (IS_ERR(res))
			break;
		xcRes->xmac_regs_res = res;

		if (refcount++ == 0) {
			res = request_iomem_region(dev_name(dev), (resource_size_t)s_ptXcStartStop, (resource_size_t)s_ptXcStartStop+sizeof(uint32_t)-1);
			if (IS_ERR(res))
				break;
			startstop_res = res;

			//TODO:
			res = request_iomem_region(dev_name(dev), (resource_size_t)s_ptXpecIrqRegs, (resource_size_t)s_ptXpecIrqRegs+sizeof(uint32_t)-1);
			//if (IS_ERR(res))
			//	break;
			//irqregs_res = res;

//FIXME 			res = request_iomem_region(dev_name(dev), start, end);
// 			if (IS_ERR(res))
// 				break;
// 			dpm_res = res;
		}
		xcRes->startstop_res = startstop_res;
		xcRes->irqregs_res = irqregs_res;
		xcRes->dpm_res = dpm_res;
	} while (0);

	if (IS_ERR(res))
		goto err_out;

	printk("%s: refcount = %i\n", __func__, refcount);

	xcRes->xcinst = uiPort;

	xcRes->rpec_pram_area = IOMEM(xcRes->rpec_res->start);
	xcRes->tpec_pram_area = IOMEM(xcRes->tpec_res->start);
	xcRes->rpu_pram_area = IOMEM(xcRes->rpu_res->start);
	xcRes->tpu_pram_area = IOMEM(xcRes->tpu_res->start);

	xcRes->rpec_reg_area = IOMEM(xcRes->rpec_regs_res->start);
	xcRes->tpec_reg_area = IOMEM(xcRes->tpec_regs_res->start);
	xcRes->xmac_area = IOMEM(xcRes->xmac_regs_res->start);

	xcRes->xc_startstop = IOMEM(xcRes->startstop_res->start);
	xcRes->xpec_irqregs = IOMEM(xcRes->irqregs_res->start);
//FIXME 	xcRes->xpec_dpm = IOMEM(xcRes->dpm_res->start);

	return xcRes;

err_out:
	xc_release_xc_res(xcRes);

	return NULL;
}

#else /* __BAREBOX_CODE */

int xc_release_xc_res(struct xc_res *xcRes)
{
	if (xcRes->rpec_res)
		release_mem_region(xcRes->rpec_res->start, resource_size(xcRes->rpec_res));
	if (xcRes->tpec_res)
		release_mem_region(xcRes->tpec_res->start, resource_size(xcRes->tpec_res));
	if (xcRes->rpu_res)
		release_mem_region(xcRes->rpu_res->start, resource_size(xcRes->rpu_res));
	if (xcRes->tpu_res)
		release_mem_region(xcRes->tpu_res->start, resource_size(xcRes->tpu_res));

	if (xcRes->rpec_regs_res)
		release_mem_region(xcRes->rpec_regs_res->start, resource_size(xcRes->rpec_regs_res));
	if (xcRes->tpec_regs_res)
		release_mem_region(xcRes->tpec_regs_res->start, resource_size(xcRes->tpec_regs_res));
	if (xcRes->xmac_regs_res)
		release_mem_region(xcRes->xmac_regs_res->start, resource_size(xcRes->xmac_regs_res));

	if (--refcount == 0) {
		if (startstop_res)
			release_mem_region(startstop_res->start, resource_size(startstop_res));
		if (irqregs_res)
			release_mem_region(irqregs_res->start, resource_size(irqregs_res));
		if (dpm_res)
			release_mem_region(dpm_res->start, resource_size(dpm_res));
	}

	kfree(xcRes);

	return 0;
}
EXPORT_SYMBOL_GPL(xc_release_xc_res);

struct xc_res *xc_alloc_xc_res(struct device *dev, uint32_t uiPort)
{
	struct xc_res *xcRes;
	struct resource *res;

	xcRes = kzalloc(sizeof(*xcRes), GFP_KERNEL);
	if (!xcRes)
		return NULL;

	do {
		res = request_mem_region((resource_size_t)s_apulRpecPramArea[uiPort], (resource_size_t)s_apulRpecPramAreaEnd[uiPort]-(resource_size_t)s_apulRpecPramArea[uiPort]+sizeof(uint32_t), dev_name(dev));
		if (IS_ERR(res))
			break;
		xcRes->rpec_res = res;

		res = request_mem_region((resource_size_t)s_apulTpecPramArea[uiPort], (resource_size_t)s_apulTpecPramAreaEnd[uiPort]-(resource_size_t)s_apulTpecPramArea[uiPort]+sizeof(uint32_t), dev_name(dev));
		if (IS_ERR(res))
			break;
		xcRes->tpec_res = res;

		res = request_mem_region((resource_size_t)s_aptRpuPramArea[uiPort], (resource_size_t)s_aptRpuPramAreaEnd[uiPort]-(resource_size_t)s_aptRpuPramArea[uiPort]+sizeof(uint32_t), dev_name(dev));
		if (IS_ERR(res))
			break;
		xcRes->rpu_res = res;

		res = request_mem_region((resource_size_t)s_aptTpuPramArea[uiPort], (resource_size_t)s_aptTpuPramAreaEnd[uiPort]-(resource_size_t)s_aptTpuPramArea[uiPort]+sizeof(uint32_t), dev_name(dev));
		if (IS_ERR(res))
			break;
		xcRes->tpu_res = res;

		res = request_mem_region((resource_size_t)s_aptRpecRegArea[uiPort], (resource_size_t)sizeof(*s_aptRpecRegArea[uiPort]), dev_name(dev));
		if (IS_ERR(res))
			break;
		xcRes->rpec_regs_res = res;

		res = request_mem_region((resource_size_t)s_aptTpecRegArea[uiPort], (resource_size_t)sizeof(*s_aptTpecRegArea[uiPort]), dev_name(dev));
		if (IS_ERR(res))
			break;
		xcRes->tpec_regs_res = res;

		res = request_mem_region((resource_size_t)s_aptXmacArea[uiPort], (resource_size_t)sizeof(*s_aptXmacArea[uiPort]), dev_name(dev));
		if (IS_ERR(res))
			break;
		xcRes->xmac_regs_res = res;

		if (refcount++ == 0) {
			res = request_mem_region((resource_size_t)s_ptXcStartStop, (resource_size_t)sizeof(*s_ptXcStartStop), dev_name(dev));
			if (IS_ERR(res))
				break;
			startstop_res = res;

			//TODO:
			//res = request_mem_region((resource_size_t)s_ptXpecIrqRegs, (resource_size_t)sizeof(*s_ptXpecIrqRegs), dev_name(dev));
			//if (IS_ERR(res))
			//	break;
			//irqregs_res = res;

//FIXME 			res = request_mem_region(start, n, dev_name(dev));
// 			if (IS_ERR(res))
// 				break;
// 			dpm_res = res;
		}
		xcRes->startstop_res = startstop_res;
		//xcRes->irqregs_res = irqregs_res;
		xcRes->dpm_res = dpm_res;
	} while (0);

	if (IS_ERR(res))
		goto err_out;

	xcRes->xcinst = uiPort;

	xcRes->rpec_pram_area = ioremap(xcRes->rpec_res->start, resource_size(xcRes->rpec_res));
	xcRes->tpec_pram_area = ioremap(xcRes->tpec_res->start, resource_size(xcRes->tpec_res));
	xcRes->rpu_pram_area = ioremap(xcRes->rpu_res->start, resource_size(xcRes->rpu_res));
	xcRes->tpu_pram_area = ioremap(xcRes->tpu_res->start, resource_size(xcRes->tpu_res));

	xcRes->rpec_reg_area = ioremap(xcRes->rpec_regs_res->start, resource_size(xcRes->rpec_regs_res));
	xcRes->tpec_reg_area = ioremap(xcRes->tpec_regs_res->start, resource_size(xcRes->tpec_regs_res));
	xcRes->xmac_area = ioremap(xcRes->xmac_regs_res->start, resource_size(xcRes->xmac_regs_res));

	xcRes->xc_startstop = ioremap(xcRes->startstop_res->start, resource_size(xcRes->startstop_res));
	//xcRes->xpec_irqregs = ioremap(xcRes->irqregs_res->start, resource_size(xcRes->irqregs_res));
//FIXME 	xcRes->xpec_dpm = ioremap(xcRes->dpm_res->start, resource_size(xcRes->dpm_res));

	return xcRes;

err_out:
	xc_release_xc_res(xcRes);

	return NULL;
}
EXPORT_SYMBOL_GPL(xc_alloc_xc_res);

#endif /* __BAREBOX_CODE */


/*****************************************************************************/
/*! Reset XC Code
* \description
*   Reset XC port.
* \class
*   XC
* \params
*   port          [in]  XC Port Number
*   pvUser           [in]  User Specific Paramters
* \return
*   0 on success
*   -1 on erroneous                                                          */
/*****************************************************************************/
int __xc_reset(struct xc_res* xc)
{
  NX4000_XMAC_AREA_T* ptXmac;
  NX4000_XPEC_AREA_T* ptRpec;
  NX4000_XPEC_AREA_T* ptTpec;
  volatile uint32_t* pulRpecPram;
  volatile uint32_t* pulTpecPram;
  unsigned int uIdx;

  if(xc->xcinst >= NUM_XPECS)
	return -1;

  ptXmac      = xc->xmac_area;
  ptRpec      = xc->rpec_reg_area;
  ptTpec      = xc->tpec_reg_area;
  pulRpecPram = xc->rpec_pram_area;
  pulTpecPram = xc->tpec_pram_area;

  /* Stop xPEC and xMAC */
  xc->xc_startstop->ulXc_start_stop_ctrl = ( HW_MSK(xc_start_stop_ctrl_xc0_stop_rpec0) |
                                            HW_MSK(xc_start_stop_ctrl_xc0_stop_tpec0) |
                                            HW_MSK(xc_start_stop_ctrl_xc0_stop_rpu0)  |
                                            HW_MSK(xc_start_stop_ctrl_xc0_stop_tpu0)
                                           ) << (4 * xc->xcinst);

  /* Clear output enable of tx asap */
  ptXmac->ulXmac_config_obu  = HW_DFLT_VAL(xmac_config_obu);

  /* reset mii_txd_oe_next to avoid MII.TXEN spike (config_mii[2] = 1 resets internal OBU signal mii_txd_oe_nxt) */
  ptXmac->ulXmac_config_mii  = HW_DFLT_VAL(xmac_config_mii) | (1U << 2);

  /* Clear output enable of io0..5 asap */
  ptRpec->aulStatcfg[xc->portno] = 0xffff0000;

  switch( xc->portno )
  {
    case 0: ptRpec->ulXpec_config = HW_MSK(xpec_config_reset_urx_fifo0) | HW_MSK(xpec_config_reset_utx_fifo0);
            break;
    case 1: ptRpec->ulXpec_config = HW_MSK(xpec_config_reset_urx_fifo1) | HW_MSK(xpec_config_reset_utx_fifo1);
            break;
  }

  /* load rate multiplier reset code */
  if( xc_load( xc, XC_TYPE_RPU, paulxMacRpuCodes[xc->xcinst]) != 0) {
    return -2;
  }
  if( xc_load( xc, XC_TYPE_TPU, paulxMacTpuCodes[xc->xcinst]) != 0) {
    return -3;
  }

  ptXmac->ulXmac_rpu_pc      = 0; /* Reset PC to 0 */
  ptXmac->ulXmac_tpu_pc      = 0; /* Reset PC to 0 */

  xc->xc_startstop->ulXc_start_stop_ctrl = (HW_MSK(xc_start_stop_ctrl_xc0_start_rpu0) |
                                           HW_MSK(xc_start_stop_ctrl_xc0_start_tpu0)) << (4 * xc->xcinst);

  /* !!!! ATTENTION: There must be enough time between starting xMAC and stopping xMAC to execute reset program */

  pulRpecPram[0] = 0xC0000FFF;                                   /* Use the command wait b000000000000,b111111111111 at Address 0*/
  pulTpecPram[0] = 0xC0000FFF;                                   /* Use the command wait b000000000000,b111111111111 at Address 0*/

  ptRpec->ulXpec_pc = 0x7ff;                                     /* Reset the Program Counter to 0x7ff */
  ptTpec->ulXpec_pc = 0x7ff;                                     /* Reset the Program Counter to 0x7ff */


  /* let the XC run for at least 10 cycles */
  for (uIdx = 0; uIdx < 10; uIdx++)
  {
    xc->xc_startstop->ulXc_start_stop_ctrl = (HW_MSK(xc_start_stop_ctrl_xc0_start_rpec0) |
                                             HW_MSK(xc_start_stop_ctrl_xc0_start_tpec0)) << (4 * xc->xcinst);
  }

  xc->xc_startstop->ulXc_start_stop_ctrl = (HW_MSK(xc_start_stop_ctrl_xc0_stop_rpec0) |
                                           HW_MSK(xc_start_stop_ctrl_xc0_stop_tpec0)) << (4 * xc->xcinst);
  ptRpec->ulXpec_pc     = 0x7ff;                                 /* Reset the Program Counter to 0x7ff */
  ptTpec->ulXpec_pc     = 0x7ff;                                 /* Reset the Program Counter to 0x7ff */

  /* reset all registers */
  ptRpec->aulXpec_r[0]   = 0;
  ptTpec->aulXpec_r[0]   = 0;
  ptRpec->aulXpec_r[1]   = 0;
  ptTpec->aulXpec_r[1]   = 0;
  ptRpec->aulXpec_r[2]   = 0;
  ptTpec->aulXpec_r[2]   = 0;
  ptRpec->aulXpec_r[3]   = 0;
  ptTpec->aulXpec_r[3]   = 0;
  ptRpec->aulXpec_r[4]   = 0;
  ptTpec->aulXpec_r[4]   = 0;
  ptRpec->aulXpec_r[5]   = 0;
  ptTpec->aulXpec_r[5]   = 0;
  ptRpec->aulXpec_r[6]   = 0;
  ptTpec->aulXpec_r[6]   = 0;
  ptRpec->aulXpec_r[7]   = 0;
  ptTpec->aulXpec_r[7]   = 0;

  /* Note regarding stat_bits_shared: this register is used for XC instance crossover protocols, protocol has to clear this register when initializing */

  ptRpec->ulRange_urtx_count = 0;
  ptTpec->ulRange_urtx_count = 0;
  ptRpec->ulRange45          = 0;
  ptTpec->ulRange45          = 0;
  ptRpec->ulRange67          = 0;
  ptTpec->ulRange67          = 0;
  ptRpec->ulUrx_count        = 0;
  ptTpec->ulUrx_count        = 0;
  ptRpec->ulUtx_count        = 0;
  ptTpec->ulUtx_count        = 0;

  /* Stop all Timers */
  ptRpec->ulTimer4       = 0;
  ptTpec->ulTimer4       = 0;
  ptRpec->ulTimer5       = 0;
  ptTpec->ulTimer5       = 0;
  ptRpec->aulTimer[0]    = 0;
  ptTpec->aulTimer[0]    = 0;
  ptRpec->aulTimer[1]    = 0;
  ptTpec->aulTimer[1]    = 0;
  ptRpec->aulTimer[2]    = 0;
  ptTpec->aulTimer[2]    = 0;
  ptRpec->aulTimer[3]    = 0;
  ptTpec->aulTimer[3]    = 0;

  ptRpec->ulIrq          = 0xFFFF0000; /* Clear XPEC side IRQ request lines */
  ptTpec->ulIrq          = 0xFFFF0000; /* Clear XPEC side IRQ request lines */

  /* Reset events */
  ptRpec->ulEc_maska     = 0x0000FFFF;
  ptTpec->ulEc_maska     = 0x0000FFFF;
  ptRpec->ulEc_maskb     = 0x0000FFFF;
  ptTpec->ulEc_maskb     = 0x0000FFFF;
  ptRpec->aulEc_mask[0]  = 0x0000FFFF;
  ptTpec->aulEc_mask[0]  = 0x0000FFFF;
  ptRpec->aulEc_mask[1]  = 0x0000FFFF;
  ptTpec->aulEc_mask[1]  = 0x0000FFFF;
  ptRpec->aulEc_mask[2]  = 0x0000FFFF;
  ptTpec->aulEc_mask[2]  = 0x0000FFFF;
  ptRpec->aulEc_mask[3]  = 0x0000FFFF;
  ptTpec->aulEc_mask[3]  = 0x0000FFFF;
  ptRpec->aulEc_mask[4]  = 0x0000FFFF;
  ptTpec->aulEc_mask[4]  = 0x0000FFFF;
  ptRpec->aulEc_mask[5]  = 0x0000FFFF;
  ptTpec->aulEc_mask[5]  = 0x0000FFFF;
  ptRpec->aulEc_mask[6]  = 0x0000FFFF;
  ptTpec->aulEc_mask[6]  = 0x0000FFFF;
  ptRpec->aulEc_mask[7]  = 0x0000FFFF;
  ptTpec->aulEc_mask[7]  = 0x0000FFFF;
  ptRpec->aulEc_mask[8]  = 0x0000FFFF;
  ptTpec->aulEc_mask[8]  = 0x0000FFFF;
  ptRpec->aulEc_mask[9]  = 0x0000FFFF;
  ptTpec->aulEc_mask[9]  = 0x0000FFFF;

  /* Reset shared registers */
  /* Reset SR0-3 for XPEC0, SR4-7 for XPEC1 */
  for( uIdx = (4 * xc->portno); uIdx < (4 * xc->portno + 4); ++uIdx )
    ptRpec->aulXpec_sr[uIdx] = 0;
  /* Reset SR8-11 for XPEC0, SR12-15 for XPEC1 */
  for( uIdx = (8 + 4 * xc->portno); (uIdx < 12 + 4 * xc->portno); ++uIdx )
    ptRpec->aulXpec_sr[uIdx] = 0;

  ptRpec->ulDatach_wr_cfg = 0;
  ptTpec->ulDatach_wr_cfg = 0;
  ptRpec->ulDatach_rd_cfg = 0;
  ptTpec->ulDatach_rd_cfg = 0;
  ptRpec->ulSysch_addr    = 0;
  ptTpec->ulSysch_addr    = 0;

  /* confirm all interrupts from xPEC -> Select the specific XC instance and get IRQ */
   *xc->xpec_irqregs = 0x0000FFFF;
  //*xc->xpec_irqregs = 0x0000FFFF;

  /* hold xMAC */
  xc->xc_startstop->ulXc_start_stop_ctrl = (HW_MSK(xc_start_stop_ctrl_xc0_stop_rpu0) |
                                           HW_MSK(xc_start_stop_ctrl_xc0_stop_tpu0)) << (4 * xc->xcinst);

  /* reset all xMAC registers to default values */
  ptXmac->ulXmac_rx_hw               = 0;
  ptXmac->ulXmac_rx_hw_count         = 0;
  ptXmac->ulXmac_tx                  = 0;
  ptXmac->ulXmac_tx_hw               = 0;
  ptXmac->ulXmac_tx_hw_count         = 0;
  ptXmac->ulXmac_tx_sent             = 0;
  ptXmac->aulXmac_wr[0]              = 0;
  ptXmac->aulXmac_wr[1]              = 0;
  ptXmac->aulXmac_wr[2]              = 0;
  ptXmac->aulXmac_wr[3]              = 0;
  ptXmac->aulXmac_wr[4]              = 0;
  ptXmac->aulXmac_wr[5]              = 0;
  ptXmac->aulXmac_wr[6]              = 0;
  ptXmac->aulXmac_wr[7]              = 0;
  ptXmac->aulXmac_wr[8]              = 0;
  ptXmac->aulXmac_wr[9]              = 0;
  ptXmac->ulXmac_config_mii          = 0;
  ptXmac->ulXmac_config_rx_nibble_fifo  = HW_DFLT_VAL(xmac_config_rx_nibble_fifo);
  ptXmac->ulXmac_config_tx_nibble_fifo  = 0;
  ptXmac->ulXmac_rpu_count1          = 0;
  ptXmac->ulXmac_rpu_count2          = 0;
  ptXmac->ulXmac_tpu_count1          = 0;
  ptXmac->ulXmac_tpu_count2          = 0;
  ptXmac->ulXmac_rx_count            = 0;
  ptXmac->ulXmac_tx_count            = 0;
  ptXmac->ulXmac_rpm_mask0           = 0;
  ptXmac->ulXmac_rpm_val0            = 0;
  ptXmac->ulXmac_rpm_mask1           = 0;
  ptXmac->ulXmac_rpm_val1            = 0;
  ptXmac->ulXmac_tpm_mask0           = 0;
  ptXmac->ulXmac_tpm_val0            = 0;
  ptXmac->ulXmac_tpm_mask1           = 0;
  ptXmac->ulXmac_tpm_val1            = 0;

  ptXmac->ulXmac_rx_crc_polynomial_l = 0;
  ptXmac->ulXmac_rx_crc_polynomial_h = 0;
  ptXmac->ulXmac_rx_crc_l            = 0;
  ptXmac->ulXmac_rx_crc_h            = 0;
  ptXmac->ulXmac_rx_crc_cfg          = 0;
  ptXmac->ulXmac_tx_crc_polynomial_l = 0;
  ptXmac->ulXmac_tx_crc_polynomial_h = 0;
  ptXmac->ulXmac_tx_crc_l            = 0;
  ptXmac->ulXmac_tx_crc_h            = 0;
  ptXmac->ulXmac_tx_crc_cfg          = 0;

  ptXmac->ulXmac_rx_crc32_l          = 0;
  ptXmac->ulXmac_rx_crc32_h          = 0;
  ptXmac->ulXmac_rx_crc32_cfg        = 0;
  ptXmac->ulXmac_tx_crc32_l          = 0;
  ptXmac->ulXmac_tx_crc32_h          = 0;
  ptXmac->ulXmac_tx_crc32_cfg        = 0;

  ptXmac->ulXmac_config_sbu2         = HW_DFLT_VAL(xmac_config_sbu2);
  ptXmac->ulXmac_config_obu2         = HW_DFLT_VAL(xmac_config_obu2);

  ptXmac->ulXmac_rpu_pc              = 0;
  ptXmac->ulXmac_tpu_pc              = 0;

  return 0;
}

#ifdef __BAREBOX_CODE

int xc_reset(struct xc_res* xc)
{
    struct resource *res;
    int rc;

    res = request_iomem_region("netx4000-xc-hal-reset", (resource_size_t)s_ptXpecIrqRegs, (resource_size_t)sizeof(s_ptXpecIrqRegs));
    if (IS_ERR(res))
	return -EBUSY;
    xc->irq_reg_res = res;
    xc->irq_reg_area = IOMEM(xc->irq_reg_res->start);

    rc = __xc_reset(xc);

    release_region(xc->irq_reg_res);
    xc->irq_reg_res = NULL;
    xc->irq_reg_area = 0;

    return rc;
}

#else /* __BAREBOX_CODE */

int xc_reset(struct xc_res* xc)
{
    struct resource *res;
    int rc;

    res = request_mem_region((resource_size_t)(uint32_t)s_ptXpecIrqRegs + xc->xcinst*(sizeof(uint32_t)), sizeof(uint32_t), "netx4000-xc-hal-reset");
    if (res == NULL)
	return -EBUSY;
    xc->irqregs_res = res;
    xc->xpec_irqregs = ioremap(xc->irqregs_res->start, sizeof(uint32_t));

    rc = __xc_reset(xc);

    iounmap(xc->xpec_irqregs);

    release_mem_region(xc->irqregs_res->start, resource_size(xc->irqregs_res));
    xc->irqregs_res = NULL;
    xc->xpec_irqregs = 0;

    return rc;
}
EXPORT_SYMBOL_GPL(xc_reset);

#endif /* __BAREBOX_CODE */



/*****************************************************************************/
/*! Load XC Code
* \description
*   Load XC port.
* \class
*   XC
* \params
*   port              [in]  XC Port Number
*   eXcType              [in]  XC Entity To Download To (RPU, TPU, xPEC)
*   pulXcPrg             [in]  Pointer To Microcode
*   pvUser               [in]  User specific parameter
* \return
*   0 on success
*   -1 on erroneous                                                          */
/*****************************************************************************/
int xc_load(struct xc_res* xc, XC_TYPE_E eXcType, const uint32_t* pulXcPrg)
{
  volatile uint32_t *pulDst, *pulDstCnt;
  volatile uint32_t *pulRamStart, *pulRamEnd;
  uint32_t ram_area = 0;
  const uint32_t *pulSrcStart, *pulSrcCnt, *pulSrcEnd;
  unsigned int uiElements;

  /* check the instance number */
  if(xc->xcinst >= NUM_XPECS)
	return 1;

  /* get the start and end address of the ram area, get the physical address */
  switch( eXcType )
  {
    case XC_TYPE_RPEC: pulRamStart     = (volatile uint32_t*)xc->rpec_pram_area;
                       pulRamEnd       = (volatile uint32_t*)((uint32_t)xc->rpec_pram_area + xc->rpec_res->end - xc->rpec_res->start);
			ram_area = xc->rpec_res->start;
                       break;
    case XC_TYPE_TPEC: pulRamStart     = (volatile uint32_t*)xc->tpec_pram_area;
                       pulRamEnd       = (volatile uint32_t*)((uint32_t)xc->tpec_pram_area + xc->tpec_res->end - xc->tpec_res->start);//xc->tpec_res->end;
			ram_area = xc->tpec_res->start;//xc->tpec_pram_area;
                       break;
    case XC_TYPE_RPU:  pulRamStart     = (volatile uint32_t*)xc->rpu_pram_area;
                       pulRamEnd       = (volatile uint32_t*)((uint32_t)xc->rpu_pram_area + xc->rpu_res->end - xc->rpu_res->start);//xc->rpu_res->end;
			ram_area = xc->rpu_res->start;//xc->rpu_pram_area;
                       break;
    case XC_TYPE_TPU:  pulRamStart     = (volatile uint32_t*)xc->tpu_pram_area;
                       pulRamEnd       = (volatile uint32_t*)((uint32_t)xc->tpu_pram_area + xc->tpu_res->end - xc->tpu_res->start);//xc->tpu_res->end;
			ram_area = xc->tpu_res->start;//xc->tpu_pram_area;
                       break;

    default:           return 2; /* unknown unit type */
  }

  /* get the number of code elements */
  uiElements = pulXcPrg[0] / sizeof(uint32_t) - 1;

  /* get the pointer in the xc area */
  /* ram_virtual_start + code_physical_start - ram_physical_start */
  pulDst = (volatile uint32_t*) ((uint32_t)pulRamStart + pulXcPrg[2] - ram_area);

  /* the code must fit into the ram area */
  if( (pulDst<pulRamStart) || ((pulDst+uiElements)>pulRamEnd) ) {
    /* the code exceeds the program ram! */
    return 3;
  }

  /* Map to virtual memory */
  //pulDst = phys_to_virt((phys_addr_t)pulDst);

  /* get source start and end pointer */
  pulSrcStart = pulXcPrg + 3;
  pulSrcEnd = pulSrcStart + uiElements;

  /* copy the code to xc ram */
  pulSrcCnt = pulSrcStart;
  pulDstCnt = pulDst;
  while( pulSrcCnt<pulSrcEnd ) {
    //TODO: get offset for virt addr
    //printk("write to %p\n", pulDstCnt);

    *pulDstCnt = *pulSrcCnt;
    pulDstCnt++;
    pulSrcCnt++;
  }

#ifndef XC_LOAD_NO_COMPARE
  /* compare the code */
  pulSrcCnt = pulSrcStart;
  pulDstCnt = pulDst;
  while( pulSrcCnt<pulSrcEnd ) {
    if( *pulDstCnt != *pulSrcCnt )
    {
      return 4;
    }
    pulDstCnt++;
    pulSrcCnt++;
  }
#endif

  /* get the number of trailing loads */
  uiElements = pulXcPrg[1] / sizeof(uint32_t);

  /* get source start and end pointer */
  pulSrcCnt = pulXcPrg + 2 + (pulXcPrg[0] / sizeof(uint32_t));
  pulSrcEnd = pulSrcCnt + uiElements;

  //TODO: we need to work with offsets here (so we can write to ramarea+offset) - the code contains absolute addresses
  /* write all trailing loads */
  while( pulSrcCnt<pulSrcEnd ) {
    /* get the destination address ( ram_virtual_start + data_physical_start - ram_physical_start) */
    //pulDst = (volatile uint32_t*) ((uint32_t)pulRamStart + *pulSrcCnt - ram_area);
    pulDst = ioremap(*pulSrcCnt,sizeof(uint32_t));

    /* Map to virtual memory */
    //pulDst = phys_to_virt((phys_addr_t)pulDst);
    pulSrcCnt++;

    /* write the data */
    *pulDst = *pulSrcCnt;
	iounmap(pulDst);
    pulSrcCnt++;
  }

  return 0;
}
EXPORT_SYMBOL_GPL(xc_load);

/*****************************************************************************/
/*! Start XC Port
* \description
*   Start XC port.
* \class
*   XC
* \params
*   port              [in]  XC Port Number
*   pvUser               [in]  User specific parameter
* \return
*   0 on success
*   -1 on erroneous                                                          */
/*****************************************************************************/
int xc_start(struct xc_res* xc)
{
	/* reset pc of units */
	xc->xmac_area->ulXmac_rpu_pc = 0;
	xc->xmac_area->ulXmac_tpu_pc = 0;
	xc->rpec_reg_area->ulXpec_pc = 0x7ff;
	xc->tpec_reg_area->ulXpec_pc = 0x7ff;

	/* start units */
	xc->xc_startstop->ulXc_start_stop_ctrl = ( (HW_MSK(xc_start_stop_ctrl_xc0_start_rpec0) |
                                             HW_MSK(xc_start_stop_ctrl_xc0_start_tpec0)  |
                                             HW_MSK(xc_start_stop_ctrl_xc0_start_rpu0)   |
                                             HW_MSK(xc_start_stop_ctrl_xc0_start_tpu0)
                                           ) << (4 * xc->xcinst));

	return 0;
}
EXPORT_SYMBOL_GPL(xc_start);

/*****************************************************************************/
/*! Stop XC Port
* \description
*   Stop XC port.
* \class
*   XC
* \params
*   port              [in]  XC Port Number
*   pvUser               [in]  User specific parameter
* \return
*   0 on success
*   -1 on erroneous                                                          */
/*****************************************************************************/
int xc_stop(struct xc_res* xc)
{
	/* stop units */
	xc->xc_startstop->ulXc_start_stop_ctrl = ( (HW_MSK(xc_start_stop_ctrl_xc0_stop_rpec0) |
                                             HW_MSK(xc_start_stop_ctrl_xc0_stop_tpec0) |
                                             HW_MSK(xc_start_stop_ctrl_xc0_stop_rpu0)  |
                                             HW_MSK(xc_start_stop_ctrl_xc0_stop_tpu0)
                                            ) << (4 * xc->xcinst));
	return 0;
}
EXPORT_SYMBOL_GPL(xc_stop);
