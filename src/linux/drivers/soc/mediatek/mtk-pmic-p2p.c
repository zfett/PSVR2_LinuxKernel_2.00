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
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/regmap.h>
#include <soc/mediatek/mtk_pmic_p2p.h>

static const struct of_device_id pmicw_p2p_id_table[] = {
	{ .compatible = "mediatek,mt3612ep-pmicw-p2p",},
	{ },
};
MODULE_DEVICE_TABLE(of, pmicw_p2p_id_table);

uint32_t mtk_get_vpu_avs_dac(struct platform_device *pdev, uint8_t idx)
{
	struct mtk_pmic_p2p *pmicw_p2p;

	if (!dev_get_drvdata(&pdev->dev)) {
		pr_err("Waiting for pmicw p2p on device\n");
		return 0;
	}
	pmicw_p2p = dev_get_drvdata(&pdev->dev);
	switch (idx) {
	case 0:
		return pmicw_p2p->vpu_100m_dac;

	case 1:
		return pmicw_p2p->vpu_312m_dac;

	case 2:
		return pmicw_p2p->vpu_604m_dac;

	case 3:
		return pmicw_p2p->vpu_756m_dac;

	default:
		return 0;
	}
}
EXPORT_SYMBOL(mtk_get_vpu_avs_dac);

static int pmicw_p2p_probe(struct platform_device *pdev)
{
	struct mtk_pmic_p2p *pmicw_p2p;
	struct device *dev = &pdev->dev;
	struct resource *res;

	pmicw_p2p = devm_kzalloc(dev, sizeof(*pmicw_p2p), GFP_KERNEL);
	if (!pmicw_p2p)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pmicw_p2p->dev = dev;
	pmicw_p2p->base = devm_ioremap_resource(dev, res);
	pmicw_p2p->vpu_100m_dac = readl(pmicw_p2p->base + PWRAP_DVFS_WDATA30);
	pmicw_p2p->vpu_312m_dac = readl(pmicw_p2p->base + PWRAP_DVFS_WDATA29);
	pmicw_p2p->vpu_604m_dac = readl(pmicw_p2p->base + PWRAP_DVFS_WDATA28);
	pmicw_p2p->vpu_756m_dac = readl(pmicw_p2p->base + PWRAP_DVFS_WDATA27);
	pr_info("vpu 100m dac:0x%x\n", pmicw_p2p->vpu_100m_dac);
	pr_info("vpu 312m dac:0x%x\n", pmicw_p2p->vpu_312m_dac);
	pr_info("vpu 604m dac:0x%x\n", pmicw_p2p->vpu_604m_dac);
	pr_info("vpu 756m dac:0x%x\n", pmicw_p2p->vpu_756m_dac);
	platform_set_drvdata(pdev, pmicw_p2p);
	return 0;
}

static int pmicw_p2p_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver pmicwrap_p2p = {
	.probe		= pmicw_p2p_probe,
	.remove		= pmicw_p2p_remove,
	.driver		= {
		.name	= "pmicwrap_p2p",
		.owner	= THIS_MODULE,
		.of_match_table = pmicw_p2p_id_table,
	},
};
module_platform_driver(pmicwrap_p2p);
