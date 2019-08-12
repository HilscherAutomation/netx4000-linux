
#ifndef NETX4000_XC_H
#define NETX4000_XC_H

#define FIRMWARE_NAME "hethmac-xc"

#include "netx4000-xc-hal.h"
#include "netx4000-xceth-hal.h"

struct port {
	struct device* dev;

	uint8_t portno;
	uint8_t no_of_xcinst;
	struct xc_res* xc_res[2];

	//NX4000_POINTER_FIFO_AREA_T* ptPFifo;
	//uint32_t pfifo_offset; /* FIFO_CHANNELS_PER_UNIT... */

	struct device_node *devnode;
};

void xc_units_enable(uint8_t port);
void xc_units_disable(uint8_t port);
struct port *port_request( uint8_t port, struct device *dev);
void port_free(struct port* portno);
int port_start(struct port* portno);
int port_stop(struct port* portno);
int port_reset(struct port* portno);
int port_request_firmware(struct port* portno);
int port_set_irq(struct port* portno, uint32_t mask);
uint32_t port_get_irq(struct port* port);
void port_confirm_irq(struct port* port, uint32_t mask);
int port_set_link_mode( struct port* p, int fValid, unsigned int uiSpeed, int fFdx);
int port_set_mac_addr( struct port* p, ETH_MAC_ADDRESS_TYPE_E eType, ETHERNET_MAC_ADDR_T tMacAddr);
int port_get_mac_addr( struct port* p, ETH_MAC_ADDRESS_TYPE_E eType, ETHERNET_MAC_ADDR_T* ptMacAddr);

#endif
