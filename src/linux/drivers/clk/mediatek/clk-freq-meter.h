/*
 * Copyright (c) 2014 MediaTek Inc.
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

#ifndef __DRV_CLK_FREQ_METER_H
#define __DRV_CLK_FREQ_METER_H

/* definition*/
#define CLK_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* struct */
struct clk_param {
	const char *name;
	uint32_t group;
	uint32_t clk_sel;
};

struct mtk_freqmtr_config {
	struct device *dev;
	void __iomem *topck_base;
	void __iomem *mcusys_base;
	void __iomem *ddr_base;
};

#define CH(_name, _group, _clk_sel) {		\
		.name = _name,						\
		.group = _group,					\
		.clk_sel = _clk_sel,				\
}

/* topckgen */
#define OFT_CLK_CFG_9		(0x190)
#define OFT_CLK_CFG_11		(0x1B0)
#define OFT_CLK_CFG_13		(0x1D0)
#define OFT_CLK_CFG_20		(0x210)
#define OFT_CLK_CFG_21		(0x214)
#define OFT_CLK_MISC_CFG_1	(0x414)
#define OFT_CLK26CALI_0		(0x520)
#define OFT_CLK26CALI_1		(0x524)
#define OFT_CLK26CALI_2		(0x528)

/* ddr */
#define OFT_DDR_MISC_CG_CTRL2 (0x28C)

/* mcusys */
#define OFT_BUS_PLL_DIVIDER_CFG	(0x7C0)

#endif /* __DRV_CLK_FREQ_METER_H */
