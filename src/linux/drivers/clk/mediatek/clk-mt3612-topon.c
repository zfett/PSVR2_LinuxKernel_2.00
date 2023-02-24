/*
 * Copyright (c) 2019 MediaTek Inc.
 * Author: Rick Ji <rick.ji@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/clk.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>

static const struct of_device_id clk_on_id_table[] = {
	{ .compatible = "mediatek,infra-peri-cksys-clkon",},
	{ },
};
MODULE_DEVICE_TABLE(of, clk_on_id_table);

static int clk_on_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	char clk_name_buf[16];
	struct clk *clkon;
	int i, ret = 0;
	u32 nums;

	ret = of_property_read_u32(dev->of_node, "numbers", &nums);
	if (ret) {
		dev_err(dev, "Failed to get clock %s\n", clk_name_buf);
		return ret;
	}

	for (i = 0; i < nums; i++) {
		sprintf(clk_name_buf, "%d", i);
		clkon = devm_clk_get(dev, clk_name_buf);
		if (IS_ERR(clkon)) {
			dev_err(dev, "Failed to get clock %s\n", clk_name_buf);
			return PTR_ERR(clkon);
		}
		if (clkon) {
			ret = clk_prepare_enable(clkon);
			if (ret) {
				clk_disable_unprepare(clkon);
				dev_err(dev, "Failed to enable clock\n");
				return ret;
			}
		}
	}
	return ret;
}

static int clk_on_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver clk_on_driver = {
	.probe		= clk_on_probe,
	.remove		= clk_on_remove,
	.driver		= {
		.name	= "topclk_on",
		.owner	= THIS_MODULE,
		.of_match_table = clk_on_id_table,
	},
};

static int __init mtk_clk_on_init(void)
{
	return platform_driver_register(&clk_on_driver);
}
arch_initcall_sync(mtk_clk_on_init);
static void __exit mtk_clk_on_exit(void)
{
	platform_driver_unregister(&clk_on_driver);
}
module_exit(mtk_clk_on_exit);

