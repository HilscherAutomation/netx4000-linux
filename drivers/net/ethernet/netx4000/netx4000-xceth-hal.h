#ifndef __NETX4000_XCETH_H
#define __NETX4000_XCETH_H

#include "ethmac_xpec_regdef.h"

#define __PACKED_PRE
#define __PACKED_POST

/*
 * hal_ethmac.h
 */

/* MAC address structure */
typedef uint8_t ETHERNET_MAC_ADDR_T[6];

/*****************************************************************************/
/*! MAC addresses
* \description
*    Describes the different types of MAC addresses.                         */
/*****************************************************************************/
typedef enum
{
  ETH_MAC_ADDRESS_CHASSIS,         /**< Primary Chassis MAC address */
  ETH_MAC_ADDRESS_2ND_CHASSIS      /**< Secondary Chassis MAC address */
} ETH_MAC_ADDRESS_TYPE_E;

/*****************************************************************************/
/*! Ethernet Frame Structure                                                 */
/*****************************************************************************/
typedef __PACKED_PRE struct ETHERNET_FRAME_Ttag
{
  ETHERNET_MAC_ADDR_T     tDstAddr;        /**< Destination MAC address (DA)      */
  ETHERNET_MAC_ADDR_T     tSrcAddr;        /**< Source MAC address (SA)           */
  uint16_t                usType;          /**< Frame length/type (LT)            */
  uint8_t                 abData[1504];    /**< Frame data excluding DA,SA,LT,FCS */
  uint8_t                 abRes[18];       /**< reserved, shall be zero           */
  uint32_t                ulTimestampNs;   /**< receive time stamp [nanoseconds]  */
  uint32_t                ulTimestampS;    /**< receive time stamp [s]            */
} __PACKED_POST ETHERNET_FRAME_T;

struct fifo_res {
	uint8_t xcinst;
	uint8_t xcNo;
	uint8_t xcPortNo;
};

/* Prototypes */
void netx4000_pfifo_reset(uint8_t xcNo);

int netx4000_xceth_fifo_release(uint8_t xcinst);
#ifdef __BAREBOX_CODE
int netx4000_xceth_fifo_request(struct device_d *dev, uint8_t xcinst);
#else
int netx4000_xceth_fifo_request(struct device *dev, uint8_t xcinst);
#endif

void netx4000_xceth_initFifoUnit(unsigned int uiPort);
int netx4000_xceth_set_link_mode(unsigned int uiPort, bool fValid, unsigned int uiSpeed, bool fFdx);
int netx4000_xceth_set_mac_address(uint32_t uiPort, ETH_MAC_ADDRESS_TYPE_E eType, ETHERNET_MAC_ADDR_T tMacAddr);
int netx4000_xceth_get_mac_address(uint32_t uiPort, ETH_MAC_ADDRESS_TYPE_E eType, ETHERNET_MAC_ADDR_T* ptMacAddr);
int netx4000_xceth_get_frame(unsigned int uiPort, ETHERNET_FRAME_T** pptFrame, void** phFrame);
int netx4000_xceth_send_frame(unsigned int uiPort, void* hFrame, uint32_t ulLength, unsigned int uHighPriority);
int netx4000_xceth_get_send_cnf_fill_level(unsigned int uiPort, unsigned int uHighPriority, uint32_t *pulCnfFillLevel);
int netx4000_xceth_get_send_cnf(unsigned int uiPort, ETHERNET_FRAME_T** pptFrame, void** phFrame, uint32_t* pulLength, unsigned int uHighPriority);
int netx4000_xceth_send_frame_without_cnf(unsigned int uiPort, void* hFrame, uint32_t ulLength, unsigned int uHighPriority);
int netx4000_xceth_get_recv_fill_level(unsigned int uiPort, unsigned int uHighPriority, uint32_t *pulFillLevel);
int netx4000_xceth_recv_frame(unsigned int uiPort, ETHERNET_FRAME_T** pptFrame, void** phFrame, uint32_t* pulLength, unsigned int uHighPriority);
int netx4000_xceth_release_frame(unsigned int uiPort, void* hFrame);
int netx4000_xceth_set_irq(uint8_t xcinst, uint32_t mask);
uint32_t netx4000_xceth_get_irq(uint8_t port);
void netx4000_xceth_confirm_irq(uint8_t port, uint32_t mask);
int netx4000_xceth_mode_promisc(unsigned int uiPort, unsigned int uEnable );

#endif