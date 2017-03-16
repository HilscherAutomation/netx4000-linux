/*
* This file contains generic code for Hilscher netX4000 based platforms.
*
* arch/arm/mach-netx4000/board-generic.c
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

#include <linux/of_platform.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/hardware/cache-l2x0.h>
#include <linux/reboot.h>
#include <mach/hardware.h>

extern struct smp_operations netx4000_smp_ops;

static struct map_desc netx4000_io_desc[] __initdata __maybe_unused = {
	{
		.virtual	= NETX4000_PRIV_PER_VIRT_BASE,
		.pfn		= __phys_to_pfn(NETX4000_PRIV_PER_PHYS_BASE), /* Convert a physical address to a Page Frame Number */
		.length		= SZ_8K,
		.type		= MT_DEVICE
	}, {
		.virtual	= NETX4000_UART_BASE_VIRT_BASE,
		.pfn		= __phys_to_pfn(NETX4000_UART_BASE_PHYS_BASE), /* Convert a physical address to a Page Frame Number */
		.length		= SZ_4K,
		.type		= MT_DEVICE
	}, {
		.virtual	= NETX4000_SYSTEMCTRL_VIRT_BASE,
		.pfn		= __phys_to_pfn(NETX4000_SYSTEMCTRL_PHYS_BASE), /* Convert a physical address to a Page Frame Number */
		.length		= SZ_4K,
		.type		= MT_DEVICE
	}
};

void __init netx4000_map_io(void)
{
	iotable_init(netx4000_io_desc, ARRAY_SIZE(netx4000_io_desc));
}

static void __init netx4000_generic_init(void)
{
	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);

	asm volatile (
		"MRC p15,0,r4,c1,c0,1\n"  // read Aux Control reg
		"ORR r4,r4,#1<<9\n"       // enable parity
		"MCR p15,0,r4,c1,c0,1\n"  // write Aux Control reg
		:::"r4"
	);
}

#define RAP_SYSCTRL_RSTCTRL 0xf8000050
#define RAP_SYSCTRL_RSTMASK 0xf8000054

void netx4000_restart(enum reboot_mode mode, const char *cmd)
{
	void __iomem *reset_ctl = (void __iomem*)ioremap(RAP_SYSCTRL_RSTCTRL,4);
	void __iomem *reset_msk = (void __iomem*)ioremap(RAP_SYSCTRL_RSTMASK,4);
	uint32_t reset_mask = 0;

	reset_mask = readl(reset_msk);
	writel((reset_mask | 1), reset_msk);
	writel(1, reset_ctl);
	while (1)
		cpu_relax();
}

static const char *netx4000_boards_compat[] __initdata = {
	"hilscher,netx4000",
	NULL,
};

DT_MACHINE_START(NETX4000_DT, "netX4000 using device tree")
	.reserve      = NULL,
	.smp          = smp_ops(netx4000_smp_ops),
	.map_io       = netx4000_map_io,
	.l2c_aux_val  = L310_AUX_CTRL_DATA_PREFETCH | L310_AUX_CTRL_INSTR_PREFETCH,
	.l2c_aux_mask = ~0,
	.init_early   = NULL,
	.init_irq     = NULL, /* not needed since it is part of DT -> gic */
	.handle_irq   = NULL, /* not needed since it is part of DT -> gic */
	.init_machine = netx4000_generic_init,
	.init_time    = NULL, /* not needed since it is part of DT -> clk, timer */
	.restart      = netx4000_restart,
	.dt_compat    = netx4000_boards_compat, /* list of compatible platforms */
MACHINE_END

