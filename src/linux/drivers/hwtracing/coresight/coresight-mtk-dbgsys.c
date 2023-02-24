/* Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/device.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/clk.h>

#define DBGSYS_APBAP_EN		0x00000044
#define DBGSYS_APBEN		0x1
#define DBGSYS_APBAP_EN_LOCK	0x0000004C
#define DBGSYS_APB_EN_LOCK	0x1
#define DBGSYS_ATB_CON		0x00000070
#define DBGSYS_ATB_EN		0x1
#define DBGSYS_ATB_SW_RST	0x8

struct mtk_dbgsys_drvdata {
	struct device		*dev;
	void __iomem		*base;
	struct clk		*atb_cg;
	struct clk		*atb_sel;
	struct clk		*syspll;
};

static const struct of_device_id mtk_dbgsys_match[] = {
	{ .compatible = "mediatek,mt3612-dbgsys", },
	{},
};

static struct mtk_dbgsys_drvdata *dbgsys = &(struct mtk_dbgsys_drvdata) {
	.base = NULL,
};

static ssize_t mtk_show_enable_atb(struct device *dev,
			    struct device_attribute *attr,
			    char *buf)
{
	void __iomem *base;
	struct mtk_dbgsys_drvdata *dbgsys_drvdata;

	dbgsys_drvdata = (struct mtk_dbgsys_drvdata *)dev->driver_data;
	base = dbgsys_drvdata->base;
	return scnprintf(buf, PAGE_SIZE, "%x\n", readl(base + DBGSYS_ATB_CON));
}

static ssize_t mtk_store_enable_atb(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t len)
{
	void __iomem *base;
	struct mtk_dbgsys_drvdata *dbgsys_drvdata;
	unsigned long set_val;

	dbgsys_drvdata = (struct mtk_dbgsys_drvdata *)dev->driver_data;
	base = dbgsys_drvdata->base;

	if (kstrtoul(buf, 0, &set_val))
		return -EINVAL;

	writel(set_val, base + DBGSYS_ATB_CON);

	return len;
}

static DEVICE_ATTR(enable_atb, S_IRUGO | S_IWUSR, mtk_show_enable_atb,
		mtk_store_enable_atb);

static struct attribute *mtk_dbgsys_attributes[] = {
	&dev_attr_enable_atb.attr,
	NULL,
};

static const struct attribute_group mtk_dbgsys_attr_group = {
	.attrs = mtk_dbgsys_attributes,
};

static int mtk_dbgsys_probe(struct platform_device *pdev)
{
	int ret;
	void __iomem *base = dbgsys->base;

	if (base == NULL)
		return -ENXIO;

#ifdef CONFIG_COMMON_CLK_MT3612
	dbgsys->atb_cg = devm_clk_get(&pdev->dev, "atb_cg");
	if (!IS_ERR(dbgsys->atb_cg)) {
		ret = clk_prepare_enable(dbgsys->atb_cg);
		if (ret)
			return ret;
	}

	dbgsys->atb_sel = devm_clk_get(&pdev->dev, "atb_sel");
	if (!IS_ERR(dbgsys->atb_sel)) {
		dbgsys->syspll = devm_clk_get(&pdev->dev, "syspll_d2");
		if (!IS_ERR(dbgsys->syspll)) {
			ret = clk_set_parent(dbgsys->atb_sel, dbgsys->syspll);
			if (ret) {
				pr_err("failed to re-parent, err: %d\n", ret);
				return ret;
			}
		}
	}
#endif

	dbgsys->dev = &pdev->dev;
	platform_set_drvdata(pdev, dbgsys);

	ret = sysfs_create_group(&pdev->dev.kobj, &mtk_dbgsys_attr_group);
	if (ret) {
		pr_err("failed to register sysfs. err: %d\n", ret);
		return ret;
	}

	return 0;
}

static struct platform_driver mtk_dbgsys_driver = {
	.driver = {
		.name = "mtk_dbgsys",
		.of_match_table = mtk_dbgsys_match,
		.suppress_bind_attrs = true,
	},
	.probe = mtk_dbgsys_probe,
};
builtin_platform_driver(mtk_dbgsys_driver);

static int __init mtk_dbgsys_init(void)
{
	struct device_node *np;

	np = of_find_matching_node(NULL, mtk_dbgsys_match);
	if (np) {
		void __iomem *base = of_iomap(np, 0);

		if (base) {
			dbgsys->base = base;
			writel(DBGSYS_ATB_EN, base + DBGSYS_ATB_CON);
		} else {
			pr_err("failed to map clock registers\n");
			return -ENXIO;
		}
	}

	return 0;
}
early_initcall(mtk_dbgsys_init);
