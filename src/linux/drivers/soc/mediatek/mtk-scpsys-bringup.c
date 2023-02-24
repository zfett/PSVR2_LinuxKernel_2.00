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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>
#include <soc/mediatek/mtk_allon.h>

static const struct of_device_id scpsys_bring_up_id_table[] = {
	{ .compatible = "mediatek,scpsys-bring-up0",},
	{ .compatible = "mediatek,scpsys-bring-up1",},
	{ .compatible = "mediatek,scpsys-bring-up2",},
	{ .compatible = "mediatek,scpsys-bring-up3",},
	{ .compatible = "mediatek,scpsys-bring-up4",},
	{ .compatible = "mediatek,scpsys-bring-up5",},
	{ .compatible = "mediatek,scpsys-bring-up6",},
	{ .compatible = "mediatek,scpsys-bring-up7",},
	{ .compatible = "mediatek,scpsys-bring-up8",},
	{ .compatible = "mediatek,scpsys-bring-up9",},
	{ .compatible = "mediatek,scpsys-bring-up10",},
	{ .compatible = "mediatek,scpsys-bring-up11",},
	{ .compatible = "mediatek,scpsys-bring-up12",},
	{ .compatible = "mediatek,scpsys-bring-up13",},
	{ .compatible = "mediatek,scpsys-bring-up14",},
	{ .compatible = "mediatek,scpsys-bring-up15",},
	{ .compatible = "mediatek,scpsys-bring-up16",},
	{ .compatible = "mediatek,scpsys-bring-up17",},
	{ .compatible = "mediatek,scpsys-bring-up18",},
	{ .compatible = "mediatek,scpsys-bring-up19",},
	{ .compatible = "mediatek,scpsys-bring-up20",},
	{ .compatible = "mediatek,scpsys-bring-up21",},
	{ },
};
MODULE_DEVICE_TABLE(of, scpsys_bring_up_id_table);

int mtk_subsys_all_power_on(struct mtk_power_allon *allpwr_on)
{
	struct device *dev = allpwr_on->dev;

	pm_runtime_get_sync(dev);
	return 0;
}
EXPORT_SYMBOL(mtk_subsys_all_power_on);

static int scpsys_bring_up_probe(struct platform_device *pdev)
{
	struct mtk_power_allon *allpwr_on;
	struct device *dev = &pdev->dev;

	allpwr_on = devm_kzalloc(dev, sizeof(*allpwr_on), GFP_KERNEL);
	if (!allpwr_on)
		return -ENOMEM;
	allpwr_on->dev = dev;
	of_property_read_u32(dev->of_node, "default-on",
			    &allpwr_on->default_on);

	if (!pdev->dev.pm_domain)
		return -EPROBE_DEFER;

	pm_runtime_enable(dev);
	if (allpwr_on->default_on)
		mtk_subsys_all_power_on(allpwr_on);
	return 0;
}

static int scpsys_bring_up_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver scpsys_bring_up = {
	.probe		= scpsys_bring_up_probe,
	.remove		= scpsys_bring_up_remove,
	.driver		= {
		.name	= "scpsys_bring_up",
		.owner	= THIS_MODULE,
		.of_match_table = scpsys_bring_up_id_table,
	},
};
module_platform_driver(scpsys_bring_up);
MODULE_LICENSE("GPL");
