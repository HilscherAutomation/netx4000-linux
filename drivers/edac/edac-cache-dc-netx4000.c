/*
* EDAC Cache DC driver for Hilscher netX4000 based platforms
*
* drivers/edac/edac-cache-dc-netx4000.c
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

#define DRIVER_DESC "Hilscher EDAC Cache DC driver for Hilscher netx4000 based platforms"
#define DRIVER_NAME "edac-cache-dc-netx4000"

#define FEATURE_ERROR_INJECTION 1

#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#include "edac_core.h"

#define CPU0_SBE_INT(ba)	(ba+0x80)
#define CPU1_SBE_INT(ba)	(ba+0x84)
#define CPU0_DBE_INT(ba)	(ba+0x88)
#define CPU1_DBE_INT(ba)	(ba+0x8C)

static const struct of_device_id netx4000_edac_cache_dc_match[] = {
	{ .compatible = "hilscher,edac-cache-dc-netx4000", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, netx4000_edac_cache_dc_match);

struct priv_data {
	void		*ba; /* base address */
	uint32_t	irq_sbe_core0;
	uint32_t	irq_dbe_core0;
	uint32_t	irq_sbe_core1;
	uint32_t	irq_dbe_core1;
	uint32_t	irq_sbe_pl310;
	uint32_t	irq_dbe_pl310;

#ifdef FEATURE_ERROR_INJECTION
	void		*regaddr;
#endif
};

#ifdef FEATURE_ERROR_INJECTION
/**
 * netx4000_edac_cache_dc_error_injection_store - Inject a SBE at L1 cache for testing.
 * @cdi:	Pointer to the platform_device struct
 * @buf:	Pointer to user data
 * @count:	Number of given data bytes
 *
 * Return:	
 */
static ssize_t netx4000_edac_cache_dc_error_injection_store(struct edac_device_ctl_info *cdi, const char *buf, size_t count)
{
	struct priv_data *priv = cdi->pvt_info;
	uint32_t regoff, regval, timeout;
	
	
	if (sscanf(buf, "%x %x", &regoff, &regval) < 2) {
		pr_err("Invalid or missing arguments [regoff, regval]\n");
		return -EINVAL;
	}

	if (!((regoff>=0) && (regoff<=0x18)) &&
		!((regoff>=0x20) && (regoff<=0x38)) &&
		!((regoff>=0x40) && (regoff<=0x44))) {
		pr_err("Invalid or missing arguments [regoff, regval]\n");
		return -EINVAL;
	}

	if (priv->regaddr) {
		pr_err("An error injection is already active.\n");
		return -EBUSY;
	}
	
	priv->regaddr = priv->ba + (regoff&~0x3);
	iowrite32(regval, priv->regaddr);

	timeout = 1000; /* 1000ms */
	while (--timeout) {
		if (priv->regaddr == 0)
			break;
		mdelay(1);
	}
	if (timeout == 0) {
		iowrite32(0, priv->regaddr);
		priv->regaddr = 0;
		pr_err("The error injection is timed out (1000ms).\n");
	}

	return count;
}

static struct edac_dev_sysfs_attribute netx4000_edac_cache_dc_sysfs_attributes[] = {
	__ATTR(error_injection, S_IWUSR, NULL, netx4000_edac_cache_dc_error_injection_store),
	NULL
};

#endif /* FEATURE_ERROR_INJECTION */

static irqreturn_t netx4000_edac_cache_dc_isr(int irq, void *dev_id)
{
	struct edac_device_ctl_info *dci = dev_id;
	struct priv_data *priv = dci->pvt_info;
	uint32_t val32;
	char msg[64];
	
#ifdef FEATURE_ERROR_INJECTION	
	if (priv->regaddr) {
		iowrite32(0, priv->regaddr);
		priv->regaddr = 0;
	}
#endif

	/* Handle single bit errors */
	if (irq == priv->irq_sbe_core0) {
		val32 = ioread32(CPU0_SBE_INT(priv->ba));
		snprintf(msg, sizeof(msg), "single bit error (0x%x:%s%s%s%s%s)",
			val32, (val32 & (1<<0)) ? " ddata" : "", (val32 & (1<<1)) ? " dtag" : "", (val32 & (1<<2)) ? " idata" : "", (val32 & (1<<3)) ? " itag" : "", (val32 & (1<<4)) ? " tlb" : "");
		edac_device_handle_ce(dci, 0, 0, msg);
		iowrite32(-1, CPU0_SBE_INT(priv->ba));
	}
	if (irq == priv->irq_sbe_core1) {
		val32 = ioread32(CPU1_SBE_INT(priv->ba));
		snprintf(msg, sizeof(msg), "single bit error (0x%x:%s%s%s%s%s)",
			val32, (val32 & (1<<0)) ? " ddata" : "", (val32 & (1<<1)) ? " dtag" : "", (val32 & (1<<2)) ? " idata" : "", (val32 & (1<<3)) ? " itag" : "", (val32 & (1<<4)) ? " tlb" : "");
		edac_device_handle_ce(dci, 1, 0, msg);
		iowrite32(-1, CPU1_SBE_INT(priv->ba));
	}
	if (irq == priv->irq_sbe_pl310) {
		edac_device_handle_ce(dci, 0, 1, "single bit error");
		edac_device_handle_ce(dci, 1, 1, "single bit error");
	}

	/* Handle double bit errors */
	if (irq == priv->irq_dbe_core0) {
		val32 = ioread32(CPU0_DBE_INT(priv->ba));
		snprintf(msg, sizeof(msg), "double bit error (0x%x:%s%s%s%s%s)",
			val32, (val32 & (1<<0)) ? " ddata" : "", (val32 & (1<<1)) ? " dtag" : "", (val32 & (1<<2)) ? " idata" : "", (val32 & (1<<3)) ? " itag" : "", (val32 & (1<<4)) ? " tlb" : "");
		edac_device_handle_ue(dci, 0, 0, msg);
		iowrite32(-1, CPU0_DBE_INT(priv->ba));
	}
	if (irq == priv->irq_dbe_core1) {
		val32 = ioread32(CPU1_DBE_INT(priv->ba));
		snprintf(msg, sizeof(msg), "double bit error (0x%x:%s%s%s%s%s)",
			val32, (val32 & (1<<0)) ? " ddata" : "", (val32 & (1<<1)) ? " dtag" : "", (val32 & (1<<2)) ? " idata" : "", (val32 & (1<<3)) ? " itag" : "", (val32 & (1<<4)) ? " tlb" : "");
		edac_device_handle_ue(dci, 1, 0, msg);
		iowrite32(-1, CPU1_DBE_INT(priv->ba));
	}
	if (irq == priv->irq_dbe_pl310) {
		edac_device_handle_ue(dci, 0, 1, "double bit error");
		edac_device_handle_ue(dci, 1, 1, "double bit error");
	}

	return IRQ_HANDLED;
}

/**
 * netx4000_edac_cache_dc_probe - Check controller and bind driver
 * @pdev:	Pointer to the platform_device struct
 *
 * Probes a specific controller instance for binding with the driver.
 *
 * Return:	0 if the controller instance was successfully bound to the driver; otherwise, < 0 on error.
 */
static int netx4000_edac_cache_dc_probe(struct platform_device *pdev)
{
	struct edac_device_ctl_info *dci;
	struct priv_data *priv;
	struct of_device_id *id;
	int rc;

	switch (edac_op_state) {
		case EDAC_OPSTATE_INVAL:
			edac_op_state = EDAC_OPSTATE_INT;
		case EDAC_OPSTATE_INT:
			break;
		case EDAC_OPSTATE_POLL:
		default:
			dev_err(&pdev->dev, "Error: Unsupported edac_op_state (%d)\n", edac_op_state);
			return -EINVAL;
	}

	dci = edac_device_alloc_ctl_info(sizeof(*priv), "cpu", 2, "L", 2, 1, NULL, 0, 0);
	if (!dci) {
		pr_err("%s: edac_device_alloc_ctl_info() failed\n", __func__);
		return -ENOMEM;
	}

	priv = dci->pvt_info;
	
	platform_set_drvdata(pdev, dci);

	/* Read the register base address from DT and map it */
	priv->ba = of_iomap(pdev->dev.of_node, 0);
	if (priv->ba == NULL) {
		pr_err("%s: of_iomap() failed\n", __func__);
		rc = -EINVAL;
		goto err_out;
	}

	id = of_match_device(netx4000_edac_cache_dc_match, &pdev->dev);
	dci->dev = &pdev->dev;
	dci->mod_name = pdev->dev.driver->name;
	dci->ctl_name = id ? id->compatible : "unknown";
	dci->dev_name = dev_name(&pdev->dev);

#ifdef FEATURE_ERROR_INJECTION
	dci->sysfs_attributes = netx4000_edac_cache_dc_sysfs_attributes;
#endif

	/* Register all required IRQs from DT */
	{
		uint32_t *pirq = &priv->irq_sbe_core0, nirq=6;
		
		priv->irq_sbe_core0 = platform_get_irq_byname(pdev, "sbe-core0");
		priv->irq_dbe_core0 = platform_get_irq_byname(pdev, "dbe-core0");
		priv->irq_sbe_core1 = platform_get_irq_byname(pdev, "sbe-core1");
		priv->irq_dbe_core1 = platform_get_irq_byname(pdev, "dbe-core1");
		priv->irq_sbe_pl310 = platform_get_irq_byname(pdev, "sbe-pl310");
		priv->irq_dbe_pl310 = platform_get_irq_byname(pdev, "dbe-pl310");

		while (nirq--) {
			rc = devm_request_irq(&pdev->dev, *pirq, netx4000_edac_cache_dc_isr, 0, dev_name(&pdev->dev), dci);
			if (rc < 0) {
				pr_err("%s: Error: Unable to request irq %d\n", __func__, *pirq);
				goto err_out;
			}
			pirq++;
		}
	}

	rc = edac_device_add_device(dci);
	if (rc < 0) {
		pr_err("%s: Failed to register with EDAC core\n", __func__);
		goto err_out;
	}
	dev_info(&pdev->dev, "successfully initialized!\n");

	return 0;

err_out:
	edac_device_free_ctl_info(dci);
	return rc;
}

/**
 * netx4000_edac_cache_dc_remove - Unbind driver from controller
 * @pdev:	Pointer to the platform_device struct
 *
 * Return:	Unconditionally 0
 */
static int netx4000_edac_cache_dc_remove(struct platform_device *pdev)
{
	struct edac_device_ctl_info *dci = platform_get_drvdata(pdev);

	edac_device_del_device(&pdev->dev);
	edac_device_free_ctl_info(dci);

	dev_info(&pdev->dev, "successfully removed!\n");

	return 0;
}


static struct platform_driver netx4000_edac_cache_dc_driver = {
	.probe = netx4000_edac_cache_dc_probe,
	.remove = netx4000_edac_cache_dc_remove,
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table = netx4000_edac_cache_dc_match,
	},
};

static int __init netx4000_edac_cache_dc_init(void)
{
	pr_info("%s: %s\n", DRIVER_NAME, DRIVER_DESC);
	return platform_driver_register(&netx4000_edac_cache_dc_driver);
}
module_init(netx4000_edac_cache_dc_init);

static void __exit netx4000_edac_cache_dc_exit(void)
{
	platform_driver_unregister(&netx4000_edac_cache_dc_driver);
}
module_exit(netx4000_edac_cache_dc_exit);

MODULE_AUTHOR("Hilscher Gesellschaft fuer Systemautomation mbH");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL v2");

