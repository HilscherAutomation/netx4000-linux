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

#endif
