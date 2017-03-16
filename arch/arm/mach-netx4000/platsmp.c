/*
* This file contains SMP specific code, used to start up the second processor.
*
* arch/arm/mach-netx4000/platsmp.c
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

#include <linux/device.h>
#include <asm/smp.h>
#include <asm/smp_scu.h>
#include <asm/io.h>
#include <mach/hardware.h>

#define SMP_WAKEUP_PHYS_BASE  (0x0507FF00)
#define SMP_WAKEUP_SIZE       (0x0100)
#define SMP_PFN_OFFS          (0xE8)
#define SMP_NEG_PFN_OFFS      (0xEC)

extern void secondary_startup(void);

static void __init netx4000_smp_init_cpus(void)
{
	int i = 0;
	int ncores = 2;

	for (i = 0; i < ncores; i++)
		set_cpu_possible(i, true);
}

static void __init netx4000_smp_prepare_cpus(unsigned int max_cpus)
{
	if (scu_a9_has_base()) {
		u32 base = scu_a9_get_base();
		void __iomem* scu_base = ioremap(base, SZ_256);
		if (!scu_base) {
			pr_err("ioremap(scu_base) failed\n");
			return;
		}
		scu_enable(scu_base);
		iounmap(scu_base);
	}
}

static void netx4000_secondary_init(unsigned int cpu)
{
	asm volatile (
		"MRC p15,0,r4,c1,c0,1\n"  // read Aux Control reg
		"ORR r4,r4,#1<<9\n"       // enable parity
		"MCR p15,0,r4,c1,c0,1\n"  // write Aux Control reg
		:::"r4"
	);
}

static int netx4000_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
#if defined(CONFIG_HVC_DCC)
	pr_err("SMP: DCC console is enabled in configuration, "
	       "skipping secondary CPU initialization as all "
	       "JTAG debuggers only allow consoles on DCC of "
	       "core 0\n");
#else
	void __iomem *mem_pen = ioremap(SMP_WAKEUP_PHYS_BASE, SMP_WAKEUP_SIZE);
	u32 entry_func = virt_to_phys(secondary_startup);

	/* write start address of secondary cpu */
	writel(entry_func, mem_pen + SMP_PFN_OFFS);
	writel(~entry_func, mem_pen + SMP_NEG_PFN_OFFS);

	dsb_sev();

	iounmap(mem_pen);
#endif

	return 0;
}

struct smp_operations netx4000_smp_ops __initdata = {
	.smp_init_cpus		= netx4000_smp_init_cpus,
	.smp_prepare_cpus	= netx4000_smp_prepare_cpus,
	.smp_secondary_init	= netx4000_secondary_init,
	.smp_boot_secondary	= netx4000_boot_secondary,
	.cpu_die			= NULL,
};

