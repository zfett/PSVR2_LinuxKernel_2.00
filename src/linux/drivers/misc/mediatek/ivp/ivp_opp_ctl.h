/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: Mao Lin <Zih-Ling.Lin@mediatek.com>
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

#ifndef __IVP_OPP_CTL_H__
#define __IVP_OPP_CTL_H__

#include "ivp_driver.h"

struct ivp_opp_clk_table {
	unsigned long volt;
	unsigned long freq;
	struct ivp_pll_data *pll_data;
	int volt_idx;
};

int ivp_find_clk_index(struct ivp_device *ivp_device,
		       int clk_type, unsigned long *target_freq);
int ivp_set_clk(struct ivp_device *ivp_device, struct clk *clk,
		struct ivp_pll_data *pll_data);
int ivp_set_freq(struct ivp_core *ivp_core, int clk_type,
		 unsigned long freq);
int ivp_set_voltage(struct ivp_device *ivp_device, unsigned long voltage);
void ivp_device_dvfs_report(struct ivp_device *ivp_device);
int ivp_init_opp_clk_table(struct ivp_device *ivp_device);
int ivp_core_set_init_frequency(struct ivp_core *ivp_core);

#endif /* __IVP_OPP_CTL_H__ */
