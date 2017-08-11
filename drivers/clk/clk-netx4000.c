/*
* Clock driver for Hilscher netX4000 based platforms
*
* drivers/clk/clk-netx4000.c
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

#include <linux/clk-provider.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <mach/hardware.h>


#ifdef CONFIG_OF
/* *************************/
/* Gated peripheral clocks */
/* *************************/

#define NOCPWRCTRL_OFS          (0x40)
#define NOCPWRMASK_OFS          (0x44)
#define NOCPWRSTAT_OFS          (0x48)
#define CLKCFG_OFS              (0x4C)

struct netx4000_hw_clk {
	struct clk_hw hw;
	u32 rate;
	u32 portctrl_mask;
};
#define to_netx4000_hw_clk(p) container_of(p, struct netx4000_hw_clk, hw)

static DEFINE_RAW_SPINLOCK(powerctrl_lock);

static unsigned long netx4000_clk_recalc_rate(struct clk_hw *hw,
		unsigned long parent_rate)
{
	struct netx4000_hw_clk *netx4000_hw_clk = to_netx4000_hw_clk(hw);
	return netx4000_hw_clk->rate;
}

static unsigned long netx4000_clk_recalc_accuracy(struct clk_hw *hw,
		unsigned long parent_accuracy)
{
	return 0;
}

static int netx4000_clk_enable(struct clk_hw *hw)
{
	struct netx4000_hw_clk *netx4000_hw_clk = to_netx4000_hw_clk(hw);
	u32 mask = netx4000_hw_clk->portctrl_mask;
	void __iomem *base = (void __iomem*)(NETX4000_SYSTEMCTRL_VIRT_BASE);
	uint32_t val;
	unsigned long flags;

	/* Check if this clock is allowed to be disabled/enabled by hardware */
	if((ioread32(base + NOCPWRMASK_OFS) & mask) != mask)
		return -EPERM;

	/* Enable clock and power */
	raw_spin_lock_irqsave(&powerctrl_lock, flags);
	while( (ioread32(base + NOCPWRSTAT_OFS) & mask) != mask) {
		val = ioread32(base + CLKCFG_OFS);
		iowrite32(val | mask, base + CLKCFG_OFS);
		val = ioread32(base + NOCPWRCTRL_OFS);
		iowrite32(val | mask, base + NOCPWRCTRL_OFS);
	}
	raw_spin_unlock_irqrestore(&powerctrl_lock, flags);

	return 0;
}

static void netx4000_clk_disable(struct clk_hw *hw)
{
	struct netx4000_hw_clk *netx4000_hw_clk = to_netx4000_hw_clk(hw);
	u32 mask = netx4000_hw_clk->portctrl_mask;
	void __iomem *base = (void __iomem*)(NETX4000_SYSTEMCTRL_VIRT_BASE);
	uint32_t val;
	unsigned long flags;

	/* Check if this clock is allowed to be disabled/enabled by hardware */
	if((ioread32(base + NOCPWRMASK_OFS) & mask) != mask)
		return;

	/* Disable clock and power */
	raw_spin_lock_irqsave(&powerctrl_lock, flags);
	val = ioread32(base + NOCPWRCTRL_OFS);
	iowrite32(val & ~mask, base + NOCPWRCTRL_OFS);
        while( (ioread32(base + NOCPWRSTAT_OFS) & mask) == mask) ;
	val = ioread32(base + CLKCFG_OFS);
	iowrite32(val & ~mask, base + CLKCFG_OFS);
        raw_spin_unlock_irqrestore(&powerctrl_lock, flags);
}

static int netx4000_clk_is_enabled(struct clk_hw *hw)
{
	struct netx4000_hw_clk *netx4000_hw_clk = to_netx4000_hw_clk(hw);
	u32 mask = netx4000_hw_clk->portctrl_mask;
	void __iomem *base = (void __iomem*)(NETX4000_SYSTEMCTRL_VIRT_BASE);

	return (ioread32(base + NOCPWRSTAT_OFS) & mask) ? 1 : 0;
}

static const struct clk_ops netx4000_hw_clk_ops = {
	.recalc_rate = netx4000_clk_recalc_rate,
	.recalc_accuracy = netx4000_clk_recalc_accuracy,
        .enable = netx4000_clk_enable,
        .disable = netx4000_clk_disable,
        .is_enabled = netx4000_clk_is_enabled,
};

void of_netx4000_periph_clk_setup(struct device_node *node)
{
	const char *clk_name = node->name;
	struct clk_init_data init;
	struct netx4000_hw_clk *netx4000_hw_clk;
	int rc;

	netx4000_hw_clk = kzalloc(sizeof(*netx4000_hw_clk), GFP_KERNEL);
	BUG_ON(!netx4000_hw_clk);

	of_property_read_string(node, "clock-output-names", &clk_name);

	init.name = clk_name;
	init.flags = CLK_IS_BASIC;
	init.num_parents = 0;
	init.ops = &netx4000_hw_clk_ops;

	rc = of_property_read_u32(node, "clock-frequency", &netx4000_hw_clk->rate);
	BUG_ON(rc);

	rc = of_property_read_u32(node, "clock-mask", &netx4000_hw_clk->portctrl_mask);
	WARN_ON(rc);

	netx4000_hw_clk->hw.init = &init;

	rc = clk_hw_register(NULL, &netx4000_hw_clk->hw);
	BUG_ON(rc);

	of_clk_add_provider(node, of_clk_src_simple_get, netx4000_hw_clk->hw.clk);
}
EXPORT_SYMBOL_GPL(of_netx4000_periph_clk_setup);
CLK_OF_DECLARE(netx4000_periph_clk, "hilscher,netx4000-periph-clock", of_netx4000_periph_clk_setup);


/* *************************/
/* CPU main clock */
/* *************************/
static u32 get_netx4000_cpu_rate(void)
{
	/* get current PLL speed */
	if ((ioread32((void __iomem*)NETX4000_SYSTEMCTRL_VIRT_BASE) & (1<<8)) == (1<<8))
		return (600*1000*1000);/* 600MHZ */
	else
		return (400*1000*1000);/* 400MHZ */
}

void of_netx4000_cpu_clk_setup(struct device_node *node)
{
	struct clk *clk;
	const char *clk_name = node->name;
	u32 rate = get_netx4000_cpu_rate();

	of_property_read_string(node, "clock-output-names", &clk_name);

	clk = clk_register_fixed_rate(NULL, clk_name, NULL,
				    0, rate);
	if (!IS_ERR(clk))
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
}
EXPORT_SYMBOL_GPL(of_netx4000_cpu_clk_setup);
CLK_OF_DECLARE(netx4000_cpu_clk, "hilscher,netx4000-cpu-clock", of_netx4000_cpu_clk_setup);
#endif

