/*
* XC Ethernet HAL functions for Hilscher netX4000 based platforms
*
* drivers/net/netx4000-xceth-hal.c
*
* (C) Copyright 2014 Hilscher Gesellschaft fuer Systemautomation mbH
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

#ifdef __BAREBOX_CODE
// 	#warning "BAREBOX_CODE"
	#include <common.h>
	#include <init.h>

	#include <mach/regdef_netx4000.h>
	#include "netx4000-xceth-hal.h"

	/* Remove comment tags below for using linux code */
// 	#define request_mem_region(start,n,name)   request_iomem_region(name, start, start+n-1)
// 	#define release_region(start,n)            release_region(start)
// 	#define ioremap(res_cookie,size)           IOMEM(res_cookie)

#else
// 	#warning "LINUX_CODE"
	#include <linux/device.h>
	#include <linux/slab.h>
	#include <linux/io.h>
	#include <linux/ioport.h>

	#include "netx4000-xc-res.h"
	#include "netx4000-xceth-hal.h"
#endif

#define __ETHMAC_DISABLE_CHECKS__  1

/******************************************************************************/
/* Definitions                                                                */
/******************************************************************************/

#define ETHMAC_PORTS  4

#define ETHERNET_MINIMUM_FRAMELENGTH                    60
#define ETHERNET_MAXIMUM_FRAMELENGTH                    1518

#define NUM_FIFO_CHANNELS_PER_UNIT                      16      /**< Number of FIFO units per XC channel */
#define FIFO_ENTRIES                                    100     /**< FIFO depth for each of the 8 FIFOs  */
#define ETH_FRAME_BUF_SIZE                              1560    /**< size of a frame buffer              */
#define INTRAM_SEGMENT_SIZE                             0x10000 /**< size of the internal ram segments   */

#define ETHERNET_FIFO_EMPTY                             0      /**< Empty pointer FIFO               */
#define ETHERNET_FIFO_IND_HI                            1      /**< High priority indication FIFO    */
#define ETHERNET_FIFO_IND_LO                            2      /**< Low priority indication FIFO     */
#define ETHERNET_FIFO_REQ_HI                            3      /**< High priority request FIFO       */
#define ETHERNET_FIFO_REQ_LO                            4      /**< Low priority request FIFO        */
#define ETHERNET_FIFO_CON_HI                            5      /**< High priority confirmation FIFO  */
#define ETHERNET_FIFO_CON_LO                            6      /**< Low priority confirmation FIFO   */

/* confirmation error codes */
#define CONF_ERRCODE_TX_SUCCESSFUL_WITHOUT_RETRIES      0x0    /**< Confirmation: Success on first try   */
#define CONF_ERRCODE_TX_SUCCESSFUL_WITH_RETRIES         0x1    /**< Confirmation: Success after some retries */
#define CONF_ERRCODE_TX_FAILED_LATE_COLLISION           0x8    /**< Confirmation: Error late collision */
#define CONF_ERRCODE_TX_FAILED_LINK_DOWN_DURING_TX      0x9    /**< Confirmation: Error link down      */
#define CONF_ERRCODE_TX_FAILED_EXCESSIVE_COLLISION      0xa    /**< Confirmation: Error collision      */
#define CONF_ERRCODE_TX_FAILED_UTX_UFL_DURING_TX        0xb    /**< Confirmation: Error FIFO overflow  */
#define CONF_ERRCODE_TX_FAILED_FATAL_ERROR              0xc    /**< Confirmation: Error Fatal          */

#define PFIFO_INST_CNT  2

/******************************************************************************/
/* Address arrays                                                             */
/******************************************************************************/

static uint32_t* const _s_apulXpecDramArea[] = {
	(uint32_t*) Adr_NX4000_xc0_tpec0_dram_ram_start,
	(uint32_t*) Adr_NX4000_xc0_tpec1_dram_ram_start,
	(uint32_t*) Adr_NX4000_xc1_tpec0_dram_ram_start,
	(uint32_t*) Adr_NX4000_xc1_tpec1_dram_ram_start
};
#define XPEC_RAM_SIZE (REL_Adr_NX4000_ram_end+sizeof(uint32_t))

static uint32_t* const _s_ptXpecIrqRegs[] = {
	(uint32_t*)  Adr_NX4000_xc0_irq_xpec0,
	(uint32_t*)  Adr_NX4000_xc0_irq_xpec1,
	(uint32_t*)  Adr_NX4000_xc1_irq_xpec0,
	(uint32_t*)  Adr_NX4000_xc1_irq_xpec1
};

static NX4000_POINTER_FIFO_AREA_T* const _s_aptPFifo[] = {
	(NX4000_POINTER_FIFO_AREA_T*) Addr_NX4000_xc0_pointer_fifo,
	(NX4000_POINTER_FIFO_AREA_T*) Addr_NX4000_xc1_pointer_fifo
};

static uint32_t* const _s_apulIntRamStart[] = {
	(uint32_t*) Addr_NX4000_intram0,
	(uint32_t*) Addr_NX4000_intram1,
	(uint32_t*) Addr_NX4000_intram2,
	(uint32_t*) Addr_NX4000_intram3
};
#define INTRAM_SIZE (REL_Adr_NX4000_intram_end+sizeof(uint32_t))

/******************************************************************************/
/* Global variables                                                           */
/******************************************************************************/

struct resource *XpecDRam_res[ETHMAC_PORTS] = {NULL};
struct resource *XpecIrqRegs_res[4] = {NULL};
struct resource *PFifo_res[PFIFO_INST_CNT] = {NULL};
struct resource *IntRamStart_res[4] = {NULL};

static volatile ETHMAC_XPEC_DPM* s_aptXpecDRam[ETHMAC_PORTS];
static volatile uint32_t* s_ptXpecIrqRegs[ETHMAC_PORTS];
static volatile NX4000_POINTER_FIFO_AREA_T* s_aptPFifo[PFIFO_INST_CNT];
static volatile uint32_t* s_apulIntRamStart[4];

struct fifo_res *g_fifoRes[ETHMAC_PORTS] = {NULL};

static uint32_t refcount_bitfield = 0;

/******************************************************************************/
/* HAL function code                                                          */
/******************************************************************************/

void netx4000_pfifo_reset(uint8_t xcNo)
{
	unsigned int uCnt;

	if (xcNo < PFIFO_INST_CNT) {
		/* set reset flag of all fifos */
		iowrite32(0xffffffff, &s_aptPFifo[xcNo]->ulPfifo_reset);

		/* reset pointer fifo borders */
		for(uCnt = 0; uCnt < 32; uCnt++) {
			iowrite32(((uCnt+1)* 100)-1, &s_aptPFifo[xcNo]->aulPfifo_border[uCnt]) ;
		}

		/* clear reset flag of all fifos */
		iowrite32(0, &s_aptPFifo[xcNo]->ulPfifo_reset);
	}
}

#ifdef __BAREBOX_CODE

static int netx4000_xceth_release_fifo_res(struct fifo_res *fifoRes)
{
	uint8_t xcinst = fifoRes->xcinst;
	uint8_t xcNo   = fifoRes->xcNo;
// 	uint8_t xcPortNo = fifoRes->xcPortNo;

	if (XpecDRam_res[xcinst]) {
		release_region(XpecDRam_res[xcinst]);
		XpecDRam_res[xcinst] = NULL;
	}
	if (XpecIrqRegs_res[xcinst]) {
		release_region(XpecIrqRegs_res[xcinst]);
		XpecIrqRegs_res[xcinst] = NULL;
	}

	refcount_bitfield &= ~(1<<xcinst);

	if ((refcount_bitfield & (0x3<<xcNo)) == 0) {
		if (PFifo_res[xcNo]) {
			release_region(PFifo_res[xcNo]);
			PFifo_res[xcNo] = NULL;
		}
	}
	if (refcount_bitfield == 0) {
		if (IntRamStart_res[0]) {
			release_region(IntRamStart_res[0]);
			IntRamStart_res[0] = NULL;
		}
		if (IntRamStart_res[1]) {
			release_region(IntRamStart_res[1]);
			IntRamStart_res[1] = NULL;
		}
		if (IntRamStart_res[2]) {
			release_region(IntRamStart_res[2]);
			IntRamStart_res[2] = NULL;
		}
		if (IntRamStart_res[3]) {
			release_region(IntRamStart_res[3]);
			IntRamStart_res[3] = NULL;
		}
	}

	free(fifoRes);

	return 0;
}

static struct fifo_res *netx4000_xceth_alloc_fifo_res(struct device_d *dev, uint8_t xcinst)
{
	struct fifo_res *fifoRes;
	struct resource *res;
	uint8_t xcNo   = xcinst >> 1;
	uint8_t xcPortNo = xcinst & 1;

	fifoRes = xzalloc(sizeof(*fifoRes));
	if (!fifoRes)
		return NULL;

	fifoRes->xcinst = xcinst;
	fifoRes->xcNo = xcNo;
	fifoRes->xcPortNo = xcPortNo;

	do {
		res = request_iomem_region(dev_name(dev), (resource_size_t)_s_apulXpecDramArea[xcinst], (resource_size_t)_s_apulXpecDramArea[xcinst]+XPEC_RAM_SIZE-1);
		if (IS_ERR(res))
			break;
		XpecDRam_res[xcinst] = res;

		res = request_iomem_region(dev_name(dev), (resource_size_t)_s_ptXpecIrqRegs[xcinst], (resource_size_t)(_s_ptXpecIrqRegs[xcinst]+1)-1);
		if (IS_ERR(res))
			break;
		XpecIrqRegs_res[xcinst] = res;

		if ((refcount_bitfield & (0x3<<xcNo)) == 0) {
			res = request_iomem_region(dev_name(dev), (resource_size_t)_s_aptPFifo[xcNo], (resource_size_t)(_s_aptPFifo[xcNo]+1)-1);
			if (IS_ERR(res))
				break;
			PFifo_res[xcNo] = res;
		}

		if (refcount_bitfield == 0) {
			res = request_iomem_region(dev_name(dev), (resource_size_t)_s_apulIntRamStart[0], (resource_size_t)_s_apulIntRamStart[0]+INTRAM_SIZE-1);
			if (IS_ERR(res))
				break;
			IntRamStart_res[0] = res;

			res = request_iomem_region(dev_name(dev), (resource_size_t)_s_apulIntRamStart[1], (resource_size_t)_s_apulIntRamStart[1]+INTRAM_SIZE-1);
			if (IS_ERR(res))
				break;
			IntRamStart_res[1] = res;

			res = request_iomem_region(dev_name(dev), (resource_size_t)_s_apulIntRamStart[2], (resource_size_t)_s_apulIntRamStart[2]+INTRAM_SIZE-1);
			if (IS_ERR(res))
				break;
			IntRamStart_res[2] = res;

			res = request_iomem_region(dev_name(dev), (resource_size_t)_s_apulIntRamStart[3], (resource_size_t)_s_apulIntRamStart[3]+INTRAM_SIZE-1);
			if (IS_ERR(res))
				break;
			IntRamStart_res[3] = res;
		}

		refcount_bitfield |= (1<<xcinst);
	} while (0);

	if (IS_ERR(res))
		goto err_out;

	s_aptXpecDRam[xcinst] = IOMEM(XpecDRam_res[xcinst]->start);
	s_ptXpecIrqRegs[xcinst] = IOMEM(XpecIrqRegs_res[xcinst]->start);
	s_aptPFifo[xcNo] = IOMEM(PFifo_res[xcNo]->start);
	s_apulIntRamStart[0] = IOMEM(IntRamStart_res[0]->start);
	s_apulIntRamStart[1] = IOMEM(IntRamStart_res[1]->start);
	s_apulIntRamStart[2] = IOMEM(IntRamStart_res[2]->start);
	s_apulIntRamStart[3] = IOMEM(IntRamStart_res[3]->start);

	return fifoRes;

err_out:
	netx4000_xceth_release_fifo_res(fifoRes);

	return NULL;
}

#else /* __BAREBOX_CODE */

static int netx4000_xceth_release_fifo_res(struct fifo_res *fifoRes)
{
	uint8_t xcinst = fifoRes->xcinst;
	uint8_t xcNo   = fifoRes->xcNo;
// 	uint8_t xcPortNo = fifoRes->xcPortNo;

	if (XpecDRam_res[xcinst]) {
		release_mem_region(XpecDRam_res[xcinst]->start, resource_size(XpecDRam_res[xcinst]));
		XpecDRam_res[xcinst] = NULL;
	}
	if (XpecIrqRegs_res[xcinst]) {
		release_mem_region(XpecIrqRegs_res[xcinst]->start, resource_size(XpecIrqRegs_res[xcinst]));
		XpecIrqRegs_res[xcinst] = NULL;
	}

	refcount_bitfield &= ~(1<<xcinst);

	if ((refcount_bitfield & (0x3<<xcNo)) == 0) {
		if (PFifo_res[xcNo]) {
			release_mem_region(PFifo_res[xcNo]->start, resource_size(PFifo_res[xcNo]));
			PFifo_res[xcNo] = NULL;
		}
	}
	if (refcount_bitfield == 0) {
		if (IntRamStart_res[0]) {
			release_mem_region(IntRamStart_res[0]->start, resource_size(IntRamStart_res[0]));
			IntRamStart_res[0] = NULL;
		}
		if (IntRamStart_res[1]) {
			release_mem_region(IntRamStart_res[1]->start, resource_size(IntRamStart_res[1]));
			IntRamStart_res[1] = NULL;
		}
		if (IntRamStart_res[2]) {
			release_mem_region(IntRamStart_res[2]->start, resource_size(IntRamStart_res[2]));
			IntRamStart_res[2] = NULL;
		}
		if (IntRamStart_res[3]) {
			release_mem_region(IntRamStart_res[3]->start, resource_size(IntRamStart_res[3]));
			IntRamStart_res[3] = NULL;
		}
	}

	kfree(fifoRes);

	return 0;
}

static struct fifo_res *netx4000_xceth_alloc_fifo_res(struct device *dev, uint8_t xcinst)
{
	struct fifo_res *fifoRes;
	struct resource *res;
	uint8_t xcNo   = xcinst >> 1;
	uint8_t xcPortNo = xcinst & 1;

	fifoRes = kzalloc(sizeof(*fifoRes), GFP_KERNEL);
	if (!fifoRes)
		return NULL;

	fifoRes->xcinst = xcinst;
	fifoRes->xcNo = xcNo;
	fifoRes->xcPortNo = xcPortNo;

	do {
		res = request_mem_region((resource_size_t)_s_apulXpecDramArea[xcinst], (resource_size_t)XPEC_RAM_SIZE, dev_name(dev));
		if (res == NULL)
			break;
		XpecDRam_res[xcinst] = res;

		res = request_mem_region((resource_size_t)_s_ptXpecIrqRegs[xcinst], (resource_size_t)sizeof(*_s_ptXpecIrqRegs[xcinst]), dev_name(dev));
		if (res == NULL)
			break;
		XpecIrqRegs_res[xcinst] = res;

		if ((refcount_bitfield & (0x3<<xcNo)) == 0) {
			res = request_mem_region((resource_size_t)_s_aptPFifo[xcNo], (resource_size_t)sizeof(*_s_aptPFifo[xcNo]), dev_name(dev));
			if (res == NULL)
				break;
			PFifo_res[xcNo] = res;
		}

		if (refcount_bitfield == 0) {
			res = request_mem_region((resource_size_t)_s_apulIntRamStart[0], (resource_size_t)INTRAM_SIZE, dev_name(dev));
			if (res == NULL)
				break;
			IntRamStart_res[0] = res;

			res = request_mem_region((resource_size_t)_s_apulIntRamStart[1], (resource_size_t)INTRAM_SIZE, dev_name(dev));
			if (res == NULL)
				break;
			IntRamStart_res[1] = res;

			res = request_mem_region((resource_size_t)_s_apulIntRamStart[2], (resource_size_t)INTRAM_SIZE, dev_name(dev));
			if (res == NULL)
				break;
			IntRamStart_res[2] = res;

			res = request_mem_region((resource_size_t)_s_apulIntRamStart[3], (resource_size_t)INTRAM_SIZE, dev_name(dev));
			if (res == NULL)
				break;
			IntRamStart_res[3] = res;
		}

		refcount_bitfield |= (1<<xcinst);
	} while (0);

	if (res == NULL)
		goto err_out;

	s_aptXpecDRam[xcinst] = ioremap(XpecDRam_res[xcinst]->start, resource_size(XpecDRam_res[xcinst]));
	s_ptXpecIrqRegs[xcinst] = ioremap(XpecIrqRegs_res[xcinst]->start, resource_size(XpecIrqRegs_res[xcinst]));
	s_aptPFifo[xcNo] = ioremap(PFifo_res[xcNo]->start, resource_size(PFifo_res[xcNo]));
	s_apulIntRamStart[0] = ioremap(IntRamStart_res[0]->start, resource_size(IntRamStart_res[0]));
	s_apulIntRamStart[1] = ioremap(IntRamStart_res[1]->start, resource_size(IntRamStart_res[1]));
	s_apulIntRamStart[2] = ioremap(IntRamStart_res[2]->start, resource_size(IntRamStart_res[2]));
	s_apulIntRamStart[3] = ioremap(IntRamStart_res[3]->start, resource_size(IntRamStart_res[3]));

	return fifoRes;

err_out:
	netx4000_xceth_release_fifo_res(fifoRes);

	return NULL;
}

void netx4000_xceth_confirm_irq( uint8_t port, uint32_t mask)
{
	iowrite32(mask & 0xFFFF, s_ptXpecIrqRegs[port]);
}

uint32_t netx4000_xceth_get_irq( uint8_t port)
{
	return (ioread32(s_ptXpecIrqRegs[port]) & 0xFFFF);
}

int netx4000_xceth_set_irq( uint8_t uiPort, uint32_t mask)
{
	iowrite32(mask & MSK_ETHMAC_INTERRUPTS_ENABLE_IND_HI_VAL, &s_aptXpecDRam[uiPort]->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_INTERRUPTS_ENABLE_IND_HI);
	iowrite32(mask & MSK_ETHMAC_INTERRUPTS_ENABLE_IND_LO_VAL, &s_aptXpecDRam[uiPort]->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_INTERRUPTS_ENABLE_IND_LO);
	iowrite32(mask & MSK_ETHMAC_INTERRUPTS_ENABLE_CON_HI_VAL, &s_aptXpecDRam[uiPort]->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_INTERRUPTS_ENABLE_CON_HI);
	iowrite32(mask & MSK_ETHMAC_INTERRUPTS_ENABLE_CON_LO_VAL, &s_aptXpecDRam[uiPort]->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_INTERRUPTS_ENABLE_CON_LO);
	iowrite32(mask & MSK_ETHMAC_INTERRUPTS_ENABLE_LINK_CHANGED_VAL, &s_aptXpecDRam[uiPort]->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_INTERRUPTS_ENABLE_LINK_CHANGED);
	iowrite32(mask & MSK_ETHMAC_INTERRUPTS_ENABLE_COL_VAL, &s_aptXpecDRam[uiPort]->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_INTERRUPTS_ENABLE_COL);
	iowrite32(mask & MSK_ETHMAC_INTERRUPTS_ENABLE_EARLY_RCV_VAL, &s_aptXpecDRam[uiPort]->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_INTERRUPTS_ENABLE_EARLY_RCV);
	iowrite32(mask & MSK_ETHMAC_INTERRUPTS_ENABLE_RX_ERR_VAL, &s_aptXpecDRam[uiPort]->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_INTERRUPTS_ENABLE_RX_ERR);
	iowrite32(mask & MSK_ETHMAC_INTERRUPTS_ENABLE_TX_ERR_VAL, &s_aptXpecDRam[uiPort]->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_INTERRUPTS_ENABLE_TX_ERR);

	return 0;
}

#endif /* __BAREBOX_CODE */

int netx4000_xceth_fifo_release(uint8_t xcinst)
{

	if (!g_fifoRes[xcinst])
		return -EINVAL;

	netx4000_xceth_release_fifo_res(g_fifoRes[xcinst]);
	g_fifoRes[xcinst] = NULL;

	return 0;
}

#ifdef __BAREBOX_CODE
int netx4000_xceth_fifo_request(struct device_d *dev, uint8_t xcinst)
#else
int netx4000_xceth_fifo_request(struct device *dev, uint8_t xcinst)
#endif
{
	struct fifo_res *res;

	res = netx4000_xceth_alloc_fifo_res(dev, xcinst);
	if(!res)
		return -EINVAL;

	g_fifoRes[xcinst] = res;

	return 0;
}

/* Configuration */

/* Initialize FIFO Unit */
void netx4000_xceth_initFifoUnit(unsigned int uiPort)
{
	uint32_t     ulFifoPtr = 0;
	unsigned int uiFifoStart;
	unsigned int uiFifoEnd;
	unsigned int uiIdx;
	unsigned int uiEmptyPtrCnt;
	uint32_t     ulFifoMsk;
	unsigned int uiXc   = uiPort >> 1;
	unsigned int uiXpec = uiPort & 1;

	ulFifoMsk = (1UL << NUM_FIFO_CHANNELS_PER_UNIT) - 1;
	ulFifoMsk = ulFifoMsk << (NUM_FIFO_CHANNELS_PER_UNIT * uiXpec);

	/* set reset bit for all pointer FIFOs */
	iowrite32(ioread32(&s_aptPFifo[uiXc]->ulPfifo_reset) | ulFifoMsk, &s_aptPFifo[uiXc]->ulPfifo_reset);

	/* get FIFO start and end number of this port number */
	uiFifoStart = uiXpec * NUM_FIFO_CHANNELS_PER_UNIT;
	uiFifoEnd = uiFifoStart + NUM_FIFO_CHANNELS_PER_UNIT;

	for(uiIdx = uiFifoStart; uiIdx < uiFifoEnd; uiIdx++) {
		iowrite32((uiIdx * FIFO_ENTRIES) + FIFO_ENTRIES - 1, &s_aptPFifo[uiXc]->aulPfifo_border[uiIdx]);
	}

	/* clear reset bit for all pointer FIFO */
	iowrite32(ioread32(&s_aptPFifo[uiXc]->ulPfifo_reset) & ~ulFifoMsk, &s_aptPFifo[uiXc]->ulPfifo_reset);

	/*** fill empty pointer FIFO ***/

	/* first DWORD in segment 0 is hard wired + IRQ vectors, so it cannot be used */
	uiEmptyPtrCnt = (INTRAM_SEGMENT_SIZE / ETH_FRAME_BUF_SIZE) - 1;

	/* each port has it's own internal ram bank */
	ulFifoPtr |= (uiPort << SRT_ETHMAC_FIFO_ELEMENT_INT_RAM_SEGMENT_NUM);

	/* fill the empty pointer FIFO */
	for(uiIdx = 1; uiIdx <= uiEmptyPtrCnt; uiIdx++) {
		ulFifoPtr &= ~MSK_ETHMAC_FIFO_ELEMENT_FRAME_BUF_NUM;
		ulFifoPtr |= (uiIdx << SRT_ETHMAC_FIFO_ELEMENT_FRAME_BUF_NUM);
		iowrite32(ulFifoPtr, &s_aptPFifo[uiXc]->aulPfifo[uiFifoStart + ETHERNET_FIFO_EMPTY]);
	}
}

int netx4000_xceth_set_link_mode(unsigned int uiPort, bool fValid, unsigned int uiSpeed, bool fFdx)
{
	uint32_t ulVal = 0; /* default: invalid link and SPEED10 and HDX */

#ifndef __ETHMAC_DISABLE_CHECKS__
	/* check the port number */
	if (uiPort >= ETHMAC_PORTS)
		return -EINVAL;

	/* check speed */
	if ((uiSpeed != 10) && (uiSpeed != 100))
		return -EINVAL;
#endif

	if (fValid) {
		/* link valid: set speed and duplex */
		if (fValid) ulVal |= MSK_ETHMAC_ARM2XPEC_INTERRUPTS_VALID;
		if (uiSpeed == 100) ulVal |= MSK_ETHMAC_ARM2XPEC_INTERRUPTS_SPEED100;
		if (fFdx) ulVal |= MSK_ETHMAC_ARM2XPEC_INTERRUPTS_FDX;
	}

// 	/* set request */
// 	s_ptXpecIrqRegs->aulIrq_xpec[uiPort] = MSK_ETHMAC_ARM2XPEC_INTERRUPTS_LINK_MODE_UPDATE_REQ | ulVal;
//
// 	/* wait for xPEC confirmed request */
// 	while(s_ptXpecIrqRegs->aulIrq_xpec[uiPort] & MSK_ETHMAC_ARM2XPEC_INTERRUPTS_LINK_MODE_UPDATE_REQ) {
// 	  /* FIXME */
// 	}
	/* set request */
	iowrite32(MSK_ETHMAC_ARM2XPEC_INTERRUPTS_LINK_MODE_UPDATE_REQ | ulVal, s_ptXpecIrqRegs[uiPort]);

	/* wait for xPEC confirmed request */
	while(ioread32(s_ptXpecIrqRegs[uiPort]) & MSK_ETHMAC_ARM2XPEC_INTERRUPTS_LINK_MODE_UPDATE_REQ) {
	  /* FIXME */
	}
  return 0;
}

int netx4000_xceth_set_mac_address(uint32_t uiPort, ETH_MAC_ADDRESS_TYPE_E eType, ETHERNET_MAC_ADDR_T tMacAddr)
{
	uint32_t ulMacHi, ulMacLo;

#ifndef __ETHMAC_DISABLE_CHECKS__
	/* check the port number */
	if (uiPort >= ETHMAC_PORTS)
		return -EINVAL;
#endif

	ulMacHi = (uint32_t)(tMacAddr[5] << 8)  | (uint32_t)tMacAddr[4];
	ulMacLo = (uint32_t)(tMacAddr[3] << 24) | (uint32_t)(tMacAddr[2]  << 16) | (uint32_t)(tMacAddr[1]  << 8) | (uint32_t)tMacAddr[0];

	switch(eType) {
	case ETH_MAC_ADDRESS_CHASSIS:
		iowrite32(ulMacHi, &s_aptXpecDRam[uiPort]->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_INTERFACE_MAC_ADDRESS_HI);
		iowrite32(ulMacLo, &s_aptXpecDRam[uiPort]->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_INTERFACE_MAC_ADDRESS_LO);
		break;
	case ETH_MAC_ADDRESS_2ND_CHASSIS:
		iowrite32(ulMacHi, &s_aptXpecDRam[uiPort]->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_2ND_INTERFACE_MAC_ADDRESS_HI);
		iowrite32(ulMacLo, &s_aptXpecDRam[uiPort]->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_2ND_INTERFACE_MAC_ADDRESS_LO);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int netx4000_xceth_get_mac_address(uint32_t uiPort, ETH_MAC_ADDRESS_TYPE_E eType, ETHERNET_MAC_ADDR_T* ptMacAddr)
{
	uint32_t ulTempLo, ulTempHi;

#ifndef __ETHMAC_DISABLE_CHECKS__
	if (uiPort >= ETHMAC_PORTS)
		return -EINVAL;
#endif

	switch(eType) {
	case ETH_MAC_ADDRESS_CHASSIS:
		ulTempLo = ioread32(&s_aptXpecDRam[uiPort]->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_INTERFACE_MAC_ADDRESS_LO);
		ulTempHi = ioread32(&s_aptXpecDRam[uiPort]->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_INTERFACE_MAC_ADDRESS_HI);
		break;
	case ETH_MAC_ADDRESS_2ND_CHASSIS:
		ulTempLo = ioread32(&s_aptXpecDRam[uiPort]->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_2ND_INTERFACE_MAC_ADDRESS_LO);
		ulTempHi = ioread32(&s_aptXpecDRam[uiPort]->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_2ND_INTERFACE_MAC_ADDRESS_HI);
		break;
	default:
		return -EINVAL;
	}

	(*ptMacAddr)[0] = (uint8_t)ulTempLo;
	(*ptMacAddr)[1] = (uint8_t)(ulTempLo >> 8);
	(*ptMacAddr)[2] = (uint8_t)(ulTempLo >> 16);
	(*ptMacAddr)[3] = (uint8_t)(ulTempLo >> 24);
	(*ptMacAddr)[4] = (uint8_t)ulTempHi;
	(*ptMacAddr)[5] = (uint8_t)(ulTempHi >> 8);

	return 0;
}

/* TX */

int netx4000_xceth_get_frame(unsigned int uiPort, ETHERNET_FRAME_T** pptFrame, void** phFrame)
{
	uint32_t     ulFifoPtr = 0;
	unsigned int uiRamSegment;
	unsigned int uiFrameIdx;
	unsigned int uiFifo;
	unsigned int uiXc   = uiPort >> 1;
	unsigned int uiXpec = uiPort & 1;

#ifndef __ETHMAC_DISABLE_CHECKS__
	/* check the port number */
	if (uiPort >= ETHMAC_PORTS)
		return -EINVAL;
#endif

	/* get FIFO fill level and check if there is at least one element */
	uiFifo = NUM_FIFO_CHANNELS_PER_UNIT * uiXpec + ETHERNET_FIFO_EMPTY;

	/* keep at least one pointer for the XC level (two parties share this empty pointer FIFO) */
	if (ioread32(&s_aptPFifo[uiXc]->aulPfifo_fill_level[uiFifo]) < 2)
		return -EBUSY;

	/* retrieve the FIFO element */
	ulFifoPtr = ioread32(&s_aptPFifo[uiXc]->aulPfifo[uiFifo]);

	/* extract RAM bank and frame number */
	uiFrameIdx   = (ulFifoPtr & MSK_ETHMAC_FIFO_ELEMENT_FRAME_BUF_NUM) >> SRT_ETHMAC_FIFO_ELEMENT_FRAME_BUF_NUM;
	uiRamSegment = (ulFifoPtr & MSK_ETHMAC_FIFO_ELEMENT_INT_RAM_SEGMENT_NUM) >> SRT_ETHMAC_FIFO_ELEMENT_INT_RAM_SEGMENT_NUM;

	/* set result */
	*pptFrame = (ETHERNET_FRAME_T*)(s_apulIntRamStart[uiRamSegment] + (ETH_FRAME_BUF_SIZE * uiFrameIdx / sizeof(uint32_t)));
	*phFrame  = (void*)ulFifoPtr;

	return 0;
}

int netx4000_xceth_send_frame(unsigned int uiPort, void* hFrame, uint32_t ulLength, unsigned int uHighPriority)
{
	uint32_t ulFifoPtr = 0;
	unsigned int uiFifo;
	unsigned int uiXc  = uiPort >> 1;
	unsigned int uiXpec = uiPort & 1;

#ifndef __ETHMAC_DISABLE_CHECKS__
	/* check the port number */
	if(uiPort >= ETHMAC_PORTS)
		return -EINVAL;

	if ((ulLength < ETHERNET_MINIMUM_FRAMELENGTH) || (ulLength > ETHERNET_MAXIMUM_FRAMELENGTH))
		return -EINVAL;
#endif

	uiFifo  = uHighPriority ? ETHERNET_FIFO_REQ_HI : ETHERNET_FIFO_REQ_LO;
	uiFifo += NUM_FIFO_CHANNELS_PER_UNIT * uiXpec;

	/* create FIFO element */
	ulFifoPtr = ((uint32_t)hFrame) & (MSK_ETHMAC_FIFO_ELEMENT_FRAME_BUF_NUM | MSK_ETHMAC_FIFO_ELEMENT_INT_RAM_SEGMENT_NUM);

	ulFifoPtr &= ~(MSK_ETHMAC_FIFO_ELEMENT_SUPPRESS_CON | MSK_ETHMAC_FIFO_ELEMENT_FRAME_LEN);
	ulFifoPtr |= (ulLength << SRT_ETHMAC_FIFO_ELEMENT_FRAME_LEN);

	/* request transmission by writing into according request FIFO */
	iowrite32(ulFifoPtr, &s_aptPFifo[uiXc]->aulPfifo[uiFifo]);

	return 0;
}

int netx4000_xceth_get_send_cnf_fill_level(unsigned int uiPort, unsigned int uHighPriority, uint32_t *pulCnfFillLevel)
{
	unsigned int uiFifo;
	unsigned int uiXc   = uiPort >> 1;
	unsigned int uiXpec = uiPort & 1;

#ifndef __ETHMAC_DISABLE_CHECKS__
	/* check the port number */
	if (uiPort >= ETHMAC_PORTS)
		return -EINVAL;
#endif

	/* select FIFO based on priority selector */
	uiFifo  = uHighPriority ? ETHERNET_FIFO_CON_HI : ETHERNET_FIFO_CON_LO;
	uiFifo += NUM_FIFO_CHANNELS_PER_UNIT * uiXpec;

	/* retrieve the current fill level from the selected FIFO */
	*pulCnfFillLevel = ioread32(&s_aptPFifo[uiXc]->aulPfifo_fill_level[uiFifo]);

	return 0;
}

int netx4000_xceth_get_send_cnf(unsigned int uiPort, ETHERNET_FRAME_T** pptFrame, void** phFrame, uint32_t* pulLength, unsigned int uHighPriority)
{
	unsigned int    uiFrameIdx;
	unsigned int    uiRamSegment;
	uint32_t        ulFillLevel;
	unsigned int    uiFifo;
	uint32_t        ulFifoPtr = 0;
	uint32_t        ulVal;
	unsigned int uiXc   = uiPort >> 1;
	unsigned int uiXpec = uiPort & 1;
	int rc;

#ifndef __ETHMAC_DISABLE_CHECKS__
	/* check the port number */
	if (uiPort >= ETHMAC_PORTS)
		return -EINVAL;
#endif

	uiFifo  = uHighPriority ? ETHERNET_FIFO_CON_HI : ETHERNET_FIFO_CON_LO;
	uiFifo += NUM_FIFO_CHANNELS_PER_UNIT * uiXpec;

	rc = netx4000_xceth_get_send_cnf_fill_level(uiPort, uHighPriority, &ulFillLevel);
	if (rc != 0)
		return rc;

	if (ulFillLevel == 0)
		return -EBUSY;

	/* retrieve the stored FIFO element */
	ulFifoPtr = ioread32(&s_aptPFifo[uiXc]->aulPfifo[uiFifo]);

	/* extract RAM bank and frame number */
	uiFrameIdx   = (ulFifoPtr & MSK_ETHMAC_FIFO_ELEMENT_FRAME_BUF_NUM) >> SRT_ETHMAC_FIFO_ELEMENT_FRAME_BUF_NUM;
	uiRamSegment = (ulFifoPtr & MSK_ETHMAC_FIFO_ELEMENT_INT_RAM_SEGMENT_NUM) >> SRT_ETHMAC_FIFO_ELEMENT_INT_RAM_SEGMENT_NUM;

	/* set result */
	*pptFrame = (ETHERNET_FRAME_T*)(s_apulIntRamStart[uiRamSegment] + ((uiFrameIdx * ETH_FRAME_BUF_SIZE) / sizeof(uint32_t)));
	*phFrame  = (void*)ulFifoPtr;

	/* retrieve the length from the FIFO element */
	*pulLength = (ulFifoPtr & MSK_ETHMAC_FIFO_ELEMENT_FRAME_LEN) >> SRT_ETHMAC_FIFO_ELEMENT_FRAME_LEN;

	/* decode the error code into our own version of it */
	ulVal = (ulFifoPtr & MSK_ETHMAC_FIFO_ELEMENT_ERROR_CODE) >> SRT_ETHMAC_FIFO_ELEMENT_ERROR_CODE;

	switch(ulVal) {
	case CONF_ERRCODE_TX_SUCCESSFUL_WITHOUT_RETRIES:
		rc = 0;
		break;
	case CONF_ERRCODE_TX_SUCCESSFUL_WITH_RETRIES:
		rc = 0;
		break;
	case CONF_ERRCODE_TX_FAILED_LATE_COLLISION:
		rc = -EIO;
		break;
	case CONF_ERRCODE_TX_FAILED_LINK_DOWN_DURING_TX:
		rc = -EIO;
		break;
	case CONF_ERRCODE_TX_FAILED_EXCESSIVE_COLLISION:
		rc = -EIO;
		break;
	case CONF_ERRCODE_TX_FAILED_UTX_UFL_DURING_TX:
		rc = -EIO;
		break;
	case CONF_ERRCODE_TX_FAILED_FATAL_ERROR:
		rc = -EIO;
		break;
	}

	return rc;
}

int netx4000_xceth_send_frame_without_cnf(unsigned int uiPort, void* hFrame, uint32_t ulLength, unsigned int uHighPriority)
{
	uint32_t ulFifoPtr = 0;
	unsigned int uiFifo;
	unsigned int uiXc  = uiPort >> 1;
	unsigned int uiXpec = uiPort & 1;

#ifndef __ETHMAC_DISABLE_CHECKS__
	/* check the port number */
	if (uiPort >= ETHMAC_PORTS)
		return -EINVAL;

	if ((ulLength < ETHERNET_MINIMUM_FRAMELENGTH) || (ulLength > ETHERNET_MAXIMUM_FRAMELENGTH))
		return -EINVAL;
#endif

	uiFifo  = uHighPriority ? ETHERNET_FIFO_REQ_HI : ETHERNET_FIFO_REQ_LO;
	uiFifo += NUM_FIFO_CHANNELS_PER_UNIT * uiXpec;

	/* create FIFO element */
	ulFifoPtr = ((uint32_t)hFrame) & (MSK_ETHMAC_FIFO_ELEMENT_FRAME_BUF_NUM | MSK_ETHMAC_FIFO_ELEMENT_INT_RAM_SEGMENT_NUM);

	ulFifoPtr |= MSK_ETHMAC_FIFO_ELEMENT_SUPPRESS_CON;
	ulFifoPtr &= ~MSK_ETHMAC_FIFO_ELEMENT_FRAME_LEN;
	ulFifoPtr |= (ulLength << SRT_ETHMAC_FIFO_ELEMENT_FRAME_LEN);

	/* request transmission by writing into according request FIFO */
	iowrite32(ulFifoPtr, &s_aptPFifo[uiXc]->aulPfifo[uiFifo]);

	return 0;
}

/* RX */

int netx4000_xceth_get_recv_fill_level(unsigned int uiPort, unsigned int uHighPriority, uint32_t *pulFillLevel)
{
	unsigned int uiFifo;
	unsigned int uiXc  = uiPort >> 1;
	unsigned int uiXpec = uiPort & 1;

#ifndef __ETHMAC_DISABLE_CHECKS__
	/* check the port number */
	if (uiPort >= ETHMAC_PORTS)
		return -EINVAL;
#endif

	uiFifo  = uHighPriority ? ETHERNET_FIFO_IND_HI : ETHERNET_FIFO_IND_LO;
	uiFifo += NUM_FIFO_CHANNELS_PER_UNIT * uiXpec;

	/* get fill level of FIFO */
	*pulFillLevel = ioread32(&s_aptPFifo[uiXc]->aulPfifo_fill_level[uiFifo]);

	return 0;
}

int netx4000_xceth_recv_frame(unsigned int uiPort, ETHERNET_FRAME_T** pptFrame, void** phFrame, uint32_t* pulLength, unsigned int uHighPriority)
{
#ifndef __ETHMAC_DISABLE_CHECKS__
	uint32_t ulFillLevel;
	int rc
#endif
	uint32_t ulOffset = 0;
	uint32_t ulFifoPtr = 0;
	uint32_t ulVal;
	unsigned int uiFifo;
	unsigned int uiXc  = uiPort >> 1;
	unsigned int uiXpec = uiPort & 1;

#ifndef __ETHMAC_DISABLE_CHECKS__
	/* check the port number */
	if (uiPort >= ETHMAC_PORTS)
		return -EINVAL;
#endif

	uiFifo  = uHighPriority ? ETHERNET_FIFO_IND_HI : ETHERNET_FIFO_IND_LO;
	uiFifo += NUM_FIFO_CHANNELS_PER_UNIT * uiXpec;

#ifndef __ETHMAC_DISABLE_CHECKS__
	/* get current fill level from FIFO and check if there is at least one element */
	rc = netx4000_xceth_get_recv_fill_level(uiPort, uHighPriority, &ulFillLevel);
	if (rc)
		return rc;

	if (0 == ulFillLevel)
		return -EBUSY;
#endif

	/* retrieve the FIFO element from the FIFO */
	ulFifoPtr = ioread32(&s_aptPFifo[uiXc]->aulPfifo[uiFifo]);

	ulVal = (ulFifoPtr & MSK_ETHMAC_FIFO_ELEMENT_FRAME_BUF_NUM) >> SRT_ETHMAC_FIFO_ELEMENT_FRAME_BUF_NUM;
	ulOffset = ulVal * ETH_FRAME_BUF_SIZE;

	/* set result */
	ulVal = (ulFifoPtr & MSK_ETHMAC_FIFO_ELEMENT_INT_RAM_SEGMENT_NUM) >> SRT_ETHMAC_FIFO_ELEMENT_INT_RAM_SEGMENT_NUM;

	*pptFrame = (ETHERNET_FRAME_T*)(s_apulIntRamStart[ulVal] + (ulOffset / sizeof(uint32_t)));
	*pulLength = (ulFifoPtr & MSK_ETHMAC_FIFO_ELEMENT_FRAME_LEN) >> SRT_ETHMAC_FIFO_ELEMENT_FRAME_LEN;
	*phFrame = (void*)ulFifoPtr;

	return 0;
}

/* TX / RX */

int netx4000_xceth_release_frame(unsigned int uiPort, void* hFrame)
{
	uint32_t ulFifoPtr = 0;
	unsigned int uiFifo;
	unsigned int uiXc  = uiPort >> 1;
	unsigned int uiXpec = uiPort & 1;

#ifndef __ETHMAC_DISABLE_CHECKS__
	/* check the port number */
	if (uiPort >= ETHMAC_PORTS)
		return -EINVAL;
#endif

	uiFifo = ETHERNET_FIFO_EMPTY;
	uiFifo += NUM_FIFO_CHANNELS_PER_UNIT * uiXpec;

	/* clear all fields except buffer number and segment number in FIFO element */
	ulFifoPtr = ((uint32_t)hFrame) & (MSK_ETHMAC_FIFO_ELEMENT_FRAME_BUF_NUM | MSK_ETHMAC_FIFO_ELEMENT_INT_RAM_SEGMENT_NUM);

	/* return pointer in empty FIFO */
	iowrite32(ulFifoPtr, &s_aptPFifo[uiXc]->aulPfifo[uiFifo]);

	return 0;
}

int netx4000_xceth_mode_promisc(unsigned int uiPort, unsigned int uEnable)
{
#ifndef __ETHMAC_DISABLE_CHECKS__
	/* check the port number */
	if( uiPort >= ETHMAC_PORTS )
		return -EINVAL;
#endif
	if (uEnable) {
		iowrite32(MSK_ETHMAC_HELP_RX_FRWD2LOCAL, &s_aptXpecDRam[uiPort]->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_MONITORING_MODE);
	}
	else {
		iowrite32(0, &s_aptXpecDRam[uiPort]->tETHMAC_CONFIG_AREA_BASE.ulETHMAC_MONITORING_MODE);
	}

	return 0;
}
