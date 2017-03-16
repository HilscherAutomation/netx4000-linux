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

static u32 get_netx4000_cpu_rate(void)
{
	void __iomem *ulRAPSysCtrlBootMode = ioremap( 0xf8000000, 4);

	/* get current PLL speed */
	if ((readl(ulRAPSysCtrlBootMode) & (1<<8)) == (1<<8))
		return (600*1000*1000);/* 600MHZ */
	else
		return (400*1000*1000);/* 400MHZ */
}

#ifdef CONFIG_OF
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

