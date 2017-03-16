/*
* cpufreq driver for Hilscher netX4000 based platforms
*
* drivers/cpufreq/netx4000-cpufreq.c
*
* (C) Copyright 2016 Hilscher Gesellschaft fuer Systemautomation mbH
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

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/module.h>

static struct cpufreq_frequency_table netx4000_freq_table[] = {
	{ 0, 0, 400000 }, /* NOTE: This entry will be replaced by "real" 
				   clock rate */
	{ 0, 0, CPUFREQ_TABLE_END },
};

static int netx4000_cpufreq_set_target(struct cpufreq_policy *policy,
				      unsigned int index)
{
	if (index > 0)
		return -EINVAL;

	return 0;
}

static int netx4000_cpufreq_driver_init(struct cpufreq_policy *policy)
{
	int ret;
	struct device *cpu_dev;

	cpu_dev = get_cpu_device(policy->cpu);
	if (!cpu_dev) {
		pr_err("failed to get cpu%d device\n", policy->cpu);
		return -ENODEV;
	}

	policy->clk = clk_get(cpu_dev, NULL);
	if (IS_ERR(policy->clk)) {
		pr_err("Unable to obtain cpu clock: %ld\n",
		       PTR_ERR(policy->clk));
		return PTR_ERR(policy->clk);
	}

	netx4000_freq_table[0].frequency = clk_get_rate(policy->clk) / 1000;

	ret = cpufreq_generic_init(policy, netx4000_freq_table, 0);

	if (ret != 0) {
		pr_err("Failed to configure frequency table: %d\n",
		       ret);
		clk_put(policy->clk);
	}

	return ret;
}

static struct cpufreq_driver netx4000_cpufreq_driver = {
	.verify		= cpufreq_generic_frequency_table_verify,
	.target_index	= netx4000_cpufreq_set_target,
	.get		= cpufreq_generic_get,
	.init		= netx4000_cpufreq_driver_init,
	.name		= "cpufreq-netx4000",
};

static int __init netx4000_cpufreq_init(void)
{
	return cpufreq_register_driver(&netx4000_cpufreq_driver);
}
module_init(netx4000_cpufreq_init);

MODULE_DESCRIPTION("cpufreq driver for Hilscher netX4000 controller");
MODULE_AUTHOR("Hilscher Gesellschaft fuer Systemautomation mbH");
MODULE_LICENSE("GPL");

