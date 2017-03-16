/*
* This file contains common code that is intended to be used across
* boards so that it's not replicated.
*
* arch/arm/mach-netx4000/common.c
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

#include <linux/io.h>
#include <linux/spinlock.h>
#include <mach/hardware.h>

static DEFINE_RAW_SPINLOCK(powerctrl_lock);

#define NOCPWRCTRL_OFS		(0x40)
#define NOCPWRMASK_OFS		(0x44)
#define NOCPWRSTAT_OFS		(0x48)
#define CLKCFG_OFS		(0x4C)
int netx4000_periph_clock_enable(uint32_t mask)
{
	void __iomem *base = (void __iomem*)(NETX4000_SYSTEMCTRL_VIRT_BASE);
	uint32_t val;
	unsigned long flags;

	/* Check if this clock is allowed to be disabled/enabled by hardware */
	if((readl(base + NOCPWRMASK_OFS) & mask) != mask)
		return -EPERM;

	/* Enable clock and power */
	raw_spin_lock_irqsave(&powerctrl_lock, flags);
	while( (readl(base + NOCPWRSTAT_OFS) & mask) != mask) {
		val = readl(base + CLKCFG_OFS);
		writel(val | mask, base + CLKCFG_OFS);
		val = readl(base + NOCPWRCTRL_OFS);
		writel(val | mask, base + NOCPWRCTRL_OFS);
	}
	raw_spin_unlock_irqrestore(&powerctrl_lock, flags);

	return 0;
}
EXPORT_SYMBOL_GPL(netx4000_periph_clock_enable);

