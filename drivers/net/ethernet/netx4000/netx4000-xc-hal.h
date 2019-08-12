#ifndef __NETX4000_XC_HAL_H
#define __NETX4000_XC_HAL_H

#ifdef __BAREBOX_CODE
	/* Barebox specific code */
#else
	/* Linux specific code */
	#include <linux/ioport.h>
	#include "netx4000-xc-res.h"
#endif

#define _HW_CONCAT(a,b) a ## b

#define HW_MSK(bf)           _HW_CONCAT(MSK_NX4000_, bf)
#define HW_DFLT_VAL(reg)     _HW_CONCAT(DFLT_VAL_NX4000_, reg)

typedef enum {
	XC_TYPE_RPEC = 0,
	XC_TYPE_TPEC,
	XC_TYPE_RPU,
	XC_TYPE_TPU
} XC_TYPE_E;

typedef struct NX4000_XPEC_IRQ_REGS_Ttag
{
	  volatile uint32_t aulIrq_xpec[4];
} NX4000_XPEC_IRQ_REGS_T;

struct xc_res {
	uint8_t xcinst;
	uint8_t portno;

	struct resource *rpec_res;
	struct resource *tpec_res;
	struct resource *rpu_res;
	struct resource *tpu_res;

	struct resource *rpec_regs_res;
	struct resource *tpec_regs_res;
	struct resource *xmac_regs_res;

	struct resource *startstop_res;
	struct resource *irqregs_res;
	struct resource *dpm_res;

	ETHMAC_XPEC_DPM* __iomem xpec_dpm;

	void* __iomem        rpec_pram_area;
	void* __iomem        tpec_pram_area;
	void* __iomem rpu_pram_area;
	void* __iomem tpu_pram_area;

	NX4000_XPEC_AREA_T* __iomem rpec_reg_area;
	NX4000_XPEC_AREA_T* __iomem tpec_reg_area;
	NX4000_XMAC_AREA_T* __iomem xmac_area;

	NX4000_XC_START_STOP_AREA_T* __iomem xc_startstop;
	uint32_t* __iomem      xpec_irqregs;
	//FIXME 	ETHMAC_XPEC_DPM* __iomem xpec_dpm;
};

int xc_release_xc_res(struct xc_res *xcRes);
int xc_reset(struct xc_res* xc);
int xc_load( struct xc_res* x, XC_TYPE_E eXcType, const uint32_t* pulXcPrg);
int xc_start(struct xc_res* xc);
int xc_stop(struct xc_res* xc);

#ifdef __BAREBOX_CODE
	/* Barebox specific code */
	struct xc_res *xc_alloc_xc_res(struct device_d *dev, uint32_t uiPort);
#else
	struct xc_res *xc_alloc_xc_res(struct device *dev, uint32_t uiPort);
#endif

#endif



