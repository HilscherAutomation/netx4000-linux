/*
* arch/arm/mach-netx4000/include/mach/hardware.h
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

#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#define NETX4000_SYSTEMCTRL_PHYS_BASE		(0xF8000000)
#define NETX4000_SYSTEMCTRL_VIRT_BASE		NETX4000_SYSTEMCTRL_PHYS_BASE
#define NETX4000_SYSTEMCTRL_SIZE		SZ_4K

#define NETX4000_UART_BASE_PHYS_BASE		(0xF8036000)
#define NETX4000_UART_BASE_VIRT_BASE		NETX4000_UART_BASE_PHYS_BASE
#define NETX4000_UART_BASE_SIZE			SZ_4K

#define NETX4000_PRIV_PER_PHYS_BASE		(0xFAF00000)
#define NETX4000_PRIV_PER_VIRT_BASE		NETX4000_PRIV_PER_PHYS_BASE
#define NETX4000_PRIV_PER_SIZE			SZ_8K


#define NETX4000_DDR_CLOCK_EN			(1 << 6)
#define NETX4000_GFX_CLOCK_EN			(1 << 5)
#define NETX4000_3PSW_CLOCK_EN			(1 << 4)
#define NETX4000_GMAC_CLOCK_EN			(1 << 3)
#define NETX4000_USB_SDIO_EN			(1 << 2)
#define NETX4000_USB_PCIE_EN			(1 << 1)
#define NETX4000_USB_CLOCK_EN			(1 << 0)
int netx4000_periph_clock_enable(uint32_t mask);

#endif
