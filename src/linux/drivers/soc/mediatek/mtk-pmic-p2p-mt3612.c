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
#include <linux/clk.h>
#include <soc/mediatek/mtk_mt3612_pmic_p2p.h>

#define NUM_SCP_CLKS			1
static struct clk *scp_clks[NUM_SCP_CLKS];

static const struct of_device_id pmicw_p2p_id_table[] = {
	{ .compatible = "mediatek,mt3612-pmicw-p2p",},
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
		return pmicw_p2p->vpu_200m_dac;

	case 1:
		return pmicw_p2p->vpu_312m_dac;

	case 2:
		return pmicw_p2p->vpu_457m_dac;

	case 3:
		return pmicw_p2p->vpu_604m_dac;

	case 4:
		return pmicw_p2p->vpu_756m_dac;

	default:
		return 0;
	}
}
EXPORT_SYMBOL(mtk_get_vpu_avs_dac);

uint32_t mtk_get_gpu_avs_dac(struct platform_device *pdev, uint8_t idx)
{
	struct mtk_pmic_p2p *pmicw_p2p;

	if (!dev_get_drvdata(&pdev->dev)) {
		pr_err("Waiting for pmicw p2p on device\n");
		return 0;
	}
	pmicw_p2p = dev_get_drvdata(&pdev->dev);
	switch (idx) {
	case 0:
		return pmicw_p2p->gpu_300m_dac;

	case 1:
		return pmicw_p2p->gpu_400m_dac;

	case 2:
		return pmicw_p2p->gpu_570m_dac;

	case 3:
		return pmicw_p2p->gpu_665m_dac;

	case 4:
		return pmicw_p2p->gpu_750m_dac;

	case 5:
		return pmicw_p2p->gpu_815m_dac;

	case 6:
		return pmicw_p2p->gpu_870m_dac;

	default:
		return 0;
	}
}
EXPORT_SYMBOL(mtk_get_gpu_avs_dac);

static const char *scp_clk_name[NUM_SCP_CLKS] = {
	"top-scp-sel",
};

static int pmicw_p2p_probe(struct platform_device *pdev)
{
	struct mtk_pmic_p2p *pmicw_p2p;
	struct device *dev = &pdev->dev;
	struct resource *res;
	int ret = 0, i;
	struct clk *clk;

	for (i = 0; i < NUM_SCP_CLKS; i++) {
		clk = devm_clk_get(&pdev->dev, scp_clk_name[i]);
		if (IS_ERR(clk)) {
			if (of_find_property(pdev->dev.of_node, "clocks",
				NULL)) {
				dev_err(&pdev->dev, "fail to get scp clock\n");
				return -ENODEV;
			}
			dev_info(&pdev->dev, "No available clocks in DTS\n");
		} else {
			scp_clks[i] = clk;
			ret = clk_prepare_enable(clk);
			dev_info(&pdev->dev, "scp clock:%s enable\n",
				scp_clk_name[i]);
			if (ret) {
				dev_err(&pdev->dev, "failed to enable scp clock\n");
				return ret;
			}
		}
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pmicw_p2p = devm_kzalloc(dev, sizeof(*pmicw_p2p), GFP_KERNEL);
	if (!pmicw_p2p)
		return -ENOMEM;
	pmicw_p2p->dev = dev;
	pmicw_p2p->base = devm_ioremap_resource(dev, res);
	pr_info("PMICW_P2P base = %llx (%p)\n", res->start, pmicw_p2p->base);

	pmicw_p2p->vpu_200m_dac = readl(pmicw_p2p->base + PWRAP_DVFS_WDATA24);
	pmicw_p2p->vpu_312m_dac = readl(pmicw_p2p->base + PWRAP_DVFS_WDATA23);
	pmicw_p2p->vpu_457m_dac = readl(pmicw_p2p->base + PWRAP_DVFS_WDATA22);
	pmicw_p2p->vpu_604m_dac = readl(pmicw_p2p->base + PWRAP_DVFS_WDATA21);
	pmicw_p2p->vpu_756m_dac = readl(pmicw_p2p->base + PWRAP_DVFS_WDATA20);
	pmicw_p2p->gpu_300m_dac = readl(pmicw_p2p->base + PWRAP_DVFS_WDATA31);
	pmicw_p2p->gpu_400m_dac = readl(pmicw_p2p->base + PWRAP_DVFS_WDATA30);
	pmicw_p2p->gpu_570m_dac = readl(pmicw_p2p->base + PWRAP_DVFS_WDATA29);
	pmicw_p2p->gpu_665m_dac = readl(pmicw_p2p->base + PWRAP_DVFS_WDATA28);
	pmicw_p2p->gpu_750m_dac = readl(pmicw_p2p->base + PWRAP_DVFS_WDATA27);
	pmicw_p2p->gpu_815m_dac = readl(pmicw_p2p->base + PWRAP_DVFS_WDATA26);
	pmicw_p2p->gpu_870m_dac = readl(pmicw_p2p->base + PWRAP_DVFS_WDATA25);
	pr_info("vpu 200m voltage(mv):%d\n", pmicw_p2p->vpu_200m_dac);
	pr_info("vpu 312m voltage(mv):%d\n", pmicw_p2p->vpu_312m_dac);
	pr_info("vpu 457m voltage(mv):%d\n", pmicw_p2p->vpu_457m_dac);
	pr_info("vpu 604m voltage(mv):%d\n", pmicw_p2p->vpu_604m_dac);
	pr_info("vpu 756m voltage(mv):%d\n", pmicw_p2p->vpu_756m_dac);
	pr_info("gpu 300m voltage(mv):%d\n", pmicw_p2p->gpu_300m_dac);
	pr_info("gpu 400m voltage(mv):%d\n", pmicw_p2p->gpu_400m_dac);
	pr_info("gpu 570m voltage(mv):%d\n", pmicw_p2p->gpu_570m_dac);
	pr_info("gpu 665m voltage(mv):%d\n", pmicw_p2p->gpu_665m_dac);
	pr_info("gpu 750m voltage(mv):%d\n", pmicw_p2p->gpu_750m_dac);
	pr_info("gpu 815m voltage(mv):%d\n", pmicw_p2p->gpu_815m_dac);
	pr_info("gpu 870m voltage(mv):%d\n", pmicw_p2p->gpu_870m_dac);
	platform_set_drvdata(pdev, pmicw_p2p);
	return 0;
}

static int pmicw_p2p_remove(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < NUM_SCP_CLKS; i++) {
		if (IS_ERR(scp_clks[i]))
			continue;
		clk_disable_unprepare(scp_clks[i]);
	}

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
