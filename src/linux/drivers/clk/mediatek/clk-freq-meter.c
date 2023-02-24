/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	Houlong Wei <houlong.wei@mediatek.com>
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

#include <linux/uaccess.h>
#include <linux/bitops.h>
#include <linux/debugfs.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <soc/mediatek/mtk_freq_meter.h>
#include "clk-freq-meter.h"

/* Force enable VPU, GPU clock by disable CG */
/* #define FORCE_CKSYS_CLK_ON */


static struct mtk_freqmtr_config *freqmtrcfg;
static const struct clk_param freq_mux[] = {
	CH("CPU", 1, 40),
	CH("SDRAM", 1, 65),
	CH("AXI", 2, 0),
	CH("VPU", 2, 3),
	CH("VPU_IF", 2, 4),
	CH("VPU1", 2, 74),
	CH("VPU2", 2, 75),
	CH("VPU3", 2, 76),
	CH("GPU", 2, 43),
};


uint32_t freq_meter_group1(uint32_t clk_mux)
{
	uint32_t freq;
	uint32_t i, val;
	void __iomem *base;

	/* printk(KERN_INFO "freq_meter_group1 clk_mux(%u)\n", clk_mux); */
	freq = 0;
	base = freqmtrcfg->topck_base;

	val = readl(base + OFT_CLK_CFG_20) & 0xffff00ff;
	writel(0x80, base + OFT_CLK26CALI_0);
	writel(val | 0x80000000, base + OFT_CLK_CFG_20);
	writel(val | clk_mux << 8, base + OFT_CLK_CFG_20);
	writel(0xFFFFFF00, base + OFT_CLK_MISC_CFG_1);
	writel(0x03FF0000, base + OFT_CLK26CALI_1);

	writel(0x81, base + OFT_CLK26CALI_0);
	i = 0;
	while (readl(base + OFT_CLK26CALI_0) == 0x81) {
		usleep_range(50, 100);
		if (i++ > 10) {
			writel(0x0, base + OFT_CLK26CALI_0);
			return 0;
		}
	}

	/* freq kHz */
	freq = ((readl(base + OFT_CLK26CALI_1) & 0xFFFF) * 26000 / 1024);
	writel(0x00, base + OFT_CLK26CALI_0);

	return freq;
}

uint32_t freq_meter_group2(uint32_t clk_mux)
{
	uint32_t freq;
	uint32_t i, val;
	void __iomem *base;
#ifdef FORCE_CKSYS_CLK_ON
	uint32_t reg[10];
#endif

	/* printk(KERN_INFO "freq_meter_group2 clk_mux(%u)\n", clk_mux); */
	freq = 0;
	base = freqmtrcfg->topck_base;

#ifdef FORCE_CKSYS_CLK_ON
	/* Force VPU clk on */
	reg[0] = readl(base + OFT_CLK_CFG_9);
	writel(reg[0] | 0x00000AA0, base + OFT_CLK_CFG_9);

	/* Force GPU clk on */
	reg[1] = readl(base + OFT_CLK_CFG_11);
	writel(reg[1] | 0x00A00000, base + OFT_CLK_CFG_11);

	/* Force VPU1~3 clk on */
	reg[2] = readl(base + OFT_CLK_CFG_13);
	writel(reg[2] | 0x0AA00000, base + OFT_CLK_CFG_13);
#endif

	val = readl(base + OFT_CLK_CFG_21) & 0xff00ffff;
	writel(0x80, base + OFT_CLK26CALI_0);
	writel(val | clk_mux << 16, base + OFT_CLK_CFG_21);
	writel(0xFF00FFFF, base + OFT_CLK_MISC_CFG_1);
	writel(0x03FF0000, base + OFT_CLK26CALI_2);

	writel(0x90, base + OFT_CLK26CALI_0);
	i = 0;
	while (readl(base + OFT_CLK26CALI_0) == 0x90) {
		usleep_range(50, 100);
		if (i++ > 10) {
			writel(0x0, base + OFT_CLK26CALI_0);
#ifdef FORCE_CKSYS_CLK_ON
			writel(reg[0], base + OFT_CLK_CFG_9);
			writel(reg[1], base + OFT_CLK_CFG_11);
			writel(reg[2], base + OFT_CLK_CFG_13);
#endif
			return 0;
		}
	}

	 /* freq kHz */
	freq = ((readl(base + OFT_CLK26CALI_2) & 0xFFFF) * 26000 / 1024);
	writel(0x00, base + OFT_CLK26CALI_0);

#ifdef FORCE_CKSYS_CLK_ON
	writel(reg[0], base + OFT_CLK_CFG_9);
	writel(reg[1], base + OFT_CLK_CFG_11);
#endif

	return freq;
}

uint32_t mtk_freqmtr_measure_clk(char *clk_name)
{
	uint32_t freq;
	uint32_t i, val;
	size_t len;
	char *str_cpu = "CPU";
	char *str_dram = "SDRAM";
	void __iomem *mcu_base;
	void __iomem *ddr_base;

	mcu_base = freqmtrcfg->mcusys_base;
	ddr_base = freqmtrcfg->ddr_base;

	freq = 0;
	len = strlen(clk_name)-1;

	for (i = 0; i < CLK_ARRAY_SIZE(freq_mux); i++) {
		if (strncmp(clk_name, freq_mux[i].name, len) == 0)
			break;
	}

	if (i >= CLK_ARRAY_SIZE(freq_mux))
		return 0;

	if (strncmp(str_cpu, freq_mux[i].name, len) == 0) {

		val = readl(mcu_base + OFT_BUS_PLL_DIVIDER_CFG);
		writel((val & 0xFFFFFF00) | 0x01,
			mcu_base + OFT_BUS_PLL_DIVIDER_CFG);
		freq = freq_meter_group1(freq_mux[i].clk_sel);
		freq = freq * 2;
		writel(val, mcu_base + OFT_BUS_PLL_DIVIDER_CFG);
	} else if (strncmp(str_dram, freq_mux[i].name, len) == 0) {
		writel(0x806003FE, ddr_base + OFT_DDR_MISC_CG_CTRL2);
		writel(0x806003FE, ddr_base + OFT_DDR_MISC_CG_CTRL2);
		writel(0x806003FF, ddr_base + OFT_DDR_MISC_CG_CTRL2);
		writel(0x806003FE, ddr_base + OFT_DDR_MISC_CG_CTRL2);
		freq = freq_meter_group1(freq_mux[i].clk_sel);
		writel(0x806003BE, ddr_base + OFT_DDR_MISC_CG_CTRL2);
		writel(0x806003BE, ddr_base + OFT_DDR_MISC_CG_CTRL2);
		writel(0x806003BF, ddr_base + OFT_DDR_MISC_CG_CTRL2);
		writel(0x806003BE, ddr_base + OFT_DDR_MISC_CG_CTRL2);
		freq = freq * 8;
	} else if (freq_mux[i].group == 1) {
		freq = freq_meter_group1(freq_mux[i].clk_sel);
	} else {
		freq = freq_meter_group2(freq_mux[i].clk_sel);
	}

	return freq;
}
EXPORT_SYMBOL(mtk_freqmtr_measure_clk);

static ssize_t store_freq_meter(struct device *dev
	, struct device_attribute *attr, const char *buf, size_t size)
{
	char temp_buf[32], *pbuf;
	uint32_t freq;

	strncpy(temp_buf, buf, sizeof(temp_buf));
	temp_buf[sizeof(temp_buf) - 1] = 0;
	pbuf = temp_buf;
	freq = mtk_freqmtr_measure_clk(pbuf);
	dev_err(dev, "freq = %u\n", freq);

	return size;
}

static ssize_t show_freq_meter(struct device *dev
	, struct device_attribute *attr, char *buf)
{
	return 0;
}

static DEVICE_ATTR(freq_meter, 0664, show_freq_meter, store_freq_meter);

static int mtk_freq_mtr_drv_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = NULL;
	int ret = 0;

	freqmtrcfg = devm_kzalloc(dev, sizeof(*freqmtrcfg), GFP_KERNEL);
	if (!freqmtrcfg)
		return -ENOMEM;

	if (device_create_file(&(pdev->dev), &dev_attr_freq_meter))
		dev_err(dev, "device_create_file err.\n");

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt3612-mcusubsys");
	if (!node)
		return -1;

	freqmtrcfg->mcusys_base = of_iomap(node, 0);
	if (!freqmtrcfg->mcusys_base)
		return -1;

	node = NULL;
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt3612-topckgen");
	if (!node)
		return -1;

	freqmtrcfg->topck_base = of_iomap(node, 0);
	if (!freqmtrcfg->topck_base)
		return -1;

	node = NULL;
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt3612-lpddr");
	if (!node)
		return -1;

	freqmtrcfg->ddr_base = of_iomap(node, 0);
	if (!freqmtrcfg->ddr_base)
		return -1;

	freqmtrcfg->dev = dev;
	return ret;
}

static int mtk_freq_mtr_drv_remove(struct platform_device *pdev)
{
	devm_kfree(&pdev->dev, freqmtrcfg);

	return 0;
}

static const struct of_device_id cksys_freq_mtr_dt_match[] = {
	{.compatible = "mediatek,mt3612-freq-meter"},
	{},
};

MODULE_DEVICE_TABLE(of, cksys_freq_mtr_dt_match);

struct platform_driver mtk_cksys_freq_meter_driver = {
	.probe = mtk_freq_mtr_drv_probe,
	.remove = mtk_freq_mtr_drv_remove,
	.driver = {
		   .name = "mediatek-cksys-freq-mtr",
		   .owner = THIS_MODULE,
		   .of_match_table = cksys_freq_mtr_dt_match,
	},
};

module_platform_driver(mtk_cksys_freq_meter_driver);
MODULE_LICENSE("GPL");

