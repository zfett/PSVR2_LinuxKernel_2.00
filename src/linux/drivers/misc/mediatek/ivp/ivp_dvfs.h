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
#ifdef CONFIG_MTK_IVP_DVFS

#ifndef __IVP_DVFS_H__
#define __IVP_DVFS_H__

#include <linux/devfreq.h>

#include "ivp_driver.h"

struct ivp_devfreq {
	struct device *dev;
	struct devfreq *devfreq;

	unsigned long current_freq;

	struct devfreq_simple_ondemand_data ondemand_data;
};

int ivp_init_devfreq(struct ivp_core *ivp_core);
int ivp_deinit_devfreq(struct ivp_core *ivp_core);

#endif /* __IVP_DVFS_H__ */
#endif /* CONFIG_MTK_IVP_DVFS */
