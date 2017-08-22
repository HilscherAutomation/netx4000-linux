/*
* Platform device driver for AMBA devices.
*
* drivers/amba/amba-platform.c
*
* (C) Copyright 2017 Hilscher Gesellschaft fuer Systemautomation mbH
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

#define DRIVER_DESC "Platform device driver for AMBA devices"
#define DRIVER_NAME "amba-platform"

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/amba/bus.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

static int amba_platform_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	struct amba_device *adev;
	const void *prop;
	int i, ret;
	
	adev = amba_device_alloc(NULL, 0, 0);
	if (adev == NULL) {
		dev_err(&pdev->dev, "%s: amba_device_alloc() failed\n", __func__);
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, adev);

	adev->dev.of_node = node;
	of_dma_configure(&adev->dev, node);

	/* Allow the HW Peripheral ID to be overridden */
	prop = of_get_property(node, "arm,primecell-periphid", NULL);
	if (prop)
		adev->periphid = of_read_ulong(prop, 1);

	for (i = 0; i < AMBA_NR_IRQS; i++)
		adev->irq[i] = irq_of_parse_and_map(node, i);

	ret = of_address_to_resource(node, 0, &adev->res);
	if (ret) {
		dev_err(&pdev->dev, "of_address_to_resource() failed!\n");
		goto err_out;
	}

	/* Set AMBA device name equal to platform device name.
	 * This is required for pinctrl. */
	adev->dev.init_name = dev_name(&pdev->dev);

	/* Release pinctrl pins as platform does not need them. */
	if (pdev->dev.pins) {
		devm_pinctrl_put(pdev->dev.pins->p);
		devm_kfree(&pdev->dev, pdev->dev.pins);
		pdev->dev.pins = NULL;
	}

	ret = amba_device_add(adev, &iomem_resource);
	if (ret) {
		dev_err(&pdev->dev, "amba_device_add() failed (%d)\n", ret);
		goto err_out;
	}

	dev_info(&pdev->dev, "successfully initialized!\n");

	return 0;

err_out:
	amba_device_put(adev);
	return ret;
}

static int amba_platform_remove(struct platform_device *pdev)
{
	struct amba_device *adev = platform_get_drvdata(pdev);

	amba_device_unregister(adev);
	
	dev_info(&pdev->dev, "successfully removed!\n");

	return 0;
}

static const struct of_device_id amba_platform_of_match[] = {
	{ .compatible = "arm,pl022", },
	{ .compatible = "arm,pl011", },
	{},
};
MODULE_DEVICE_TABLE(of, amba_platform_of_match);

static struct platform_driver amba_platform_driver = {
	.driver = {
		.name= DRIVER_NAME,
		.of_match_table = amba_platform_of_match,
	},
	.probe  = amba_platform_probe,
	.remove = amba_platform_remove,
};

static int __init amba_platform_init(void)
{
	pr_info("%s: %s\n", DRIVER_NAME, DRIVER_DESC);
	return platform_driver_register(&amba_platform_driver);
}
subsys_initcall(amba_platform_init);

static void __exit amba_platform_exit(void)
{
	platform_driver_unregister(&amba_platform_driver);
}
module_exit(amba_platform_exit);


MODULE_AUTHOR("Hilscher Gesellschaft fuer Systemautomation mbH");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL v2");
