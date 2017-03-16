/*
* arch/arm/mach-netx4000/include/mach/uncompress.h
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

#include <linux/amba/serial.h>
#include <linux/io.h>

#ifdef CONFIG_DEBUG_UNCOMPRESS
extern void inline putc(char c);
#endif
static inline void flush(void) {}
static inline void arch_decomp_setup(void) {}

#ifndef CONFIG_DEBUG_UART_VIRT
  #define CONFIG_DEBUG_UART_VIRT 0xF8036000
#endif

static inline void putc(char c)
{
	while (__raw_readw(CONFIG_DEBUG_UART_VIRT + UART01x_FR) & UART01x_FR_TXFF);
	__raw_writew(c,(void* __iomem)(CONFIG_DEBUG_UART_VIRT + UART01x_DR));
}
