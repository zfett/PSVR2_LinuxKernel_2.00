/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: James Liao <jamesjj.liao@mediatek.com>
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
#include <soc/mediatek/mtk_allon.h>

static const struct of_device_id bring_up_id_table[] = {
	{ .compatible = "mediatek,clk-bring-up",},
	{ },
};
MODULE_DEVICE_TABLE(of, bring_up_id_table);

int mtk_subsys_all_clock_on(struct mtk_clock_allon *allclk_on)
{
	const int NR_CLKS = BRINGUP_CLK_NUM;
	int i;
	int ret = 0;
	char clk_name_buf[16];

	for (i = BRINGUP_CLK_NUM_BOUND; i < NR_CLKS; i++) {
		sprintf(clk_name_buf, "%d", i);
		allclk_on->clk[i] = devm_clk_get(allclk_on->dev, clk_name_buf);
		if (IS_ERR(allclk_on->clk[i])) {
			dev_err(allclk_on->dev, "Failed to get clock all on clock: %s\n",
				clk_name_buf);
			return PTR_ERR(allclk_on->clk[i]);
		}
		if (allclk_on->clk[i]) {
			ret = clk_prepare_enable(allclk_on->clk[i]);
			if (ret) {
				clk_disable_unprepare(allclk_on->clk[i]);
				dev_err(allclk_on->dev,
				       "Failed to enable clock:%d\n", ret);
				return ret;
			}
		}
	}
	return 0;
}
EXPORT_SYMBOL(mtk_subsys_all_clock_on);

static int bring_up_probe(struct platform_device *pdev)
{
	struct mtk_clock_allon *allclk_on;
	struct device *dev = &pdev->dev;
	const int NR_CLKS = BRINGUP_CLK_NUM_BOUND;
	char clk_name_buf[16];
	int i;

	allclk_on = devm_kzalloc(dev, sizeof(*allclk_on), GFP_KERNEL);
	allclk_on->dev = dev;

	for (i = 0; i < NR_CLKS; i++) {
		sprintf(clk_name_buf, "%d", i);
		allclk_on->clk[i] = devm_clk_get(dev, clk_name_buf);
		if (IS_ERR(allclk_on->clk[i])) {
			dev_err(dev, "Failed to get clock all on clock: %s\n",
				clk_name_buf);
			return PTR_ERR(allclk_on->clk[i]);
		}
		if (i < BRINGUP_CLK_NUM_BOUND)
			clk_prepare_enable(allclk_on->clk[i]);
	}
	platform_set_drvdata(pdev, allclk_on);
	return 0;
}

static int bring_up_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver bring_up = {
	.probe		= bring_up_probe,
	.remove		= bring_up_remove,
	.driver		= {
		.name	= "bring_up",
		.owner	= THIS_MODULE,
		.of_match_table = bring_up_id_table,
	},
};

module_platform_driver(bring_up);
MODULE_LICENSE("GPL");
