/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#define BRINGUP_CLK_NUM_BOUND 94
#define BRINGUP_CLK_NUM 652
#define MT3612_POWER_DOMAIN_NR 21

struct mtk_clock_allon {
	/** point to mtk_power_allon device struct */
	struct device *dev;
	struct clk *clk[BRINGUP_CLK_NUM];
};

struct mtk_power_allon {
	/** point to mtk_power_allon device struct */
	struct device *dev;
	u32 default_on;
};

int mtk_subsys_all_clock_on(struct mtk_clock_allon *allclk_on);
int mtk_subsys_all_power_on(struct mtk_power_allon *allpwr_on);
