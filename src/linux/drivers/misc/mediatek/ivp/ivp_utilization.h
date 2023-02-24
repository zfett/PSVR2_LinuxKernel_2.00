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

#ifndef __IVP_UTILIZATION_H__
#define __IVP_UTILIZATION_H__

#include <linux/hrtimer.h>
#include <linux/spinlock.h>

#include "ivp_driver.h"

struct ivp_util {
	struct device *dev;
	struct ivp_device *ivp_device;

	spinlock_t lock;

	bool active;
	struct hrtimer timer;
	ktime_t period_time;
	ktime_t period_start;
	unsigned long prev_total;
};
int ivp_utilization_compute_enter(const struct ivp_device *ivp_device,
				  int coreid);
int ivp_utilization_compute_leave(const struct ivp_device *ivp_device,
				  int coreid);
int ivp_dvfs_get_usage(const struct ivp_core *ivp_core,
		       unsigned long *total_time, unsigned long *busy_time);
int ivp_init_util(struct ivp_device *ivp_device);
int ivp_deinit_util(struct ivp_device *ivp_device);

#endif /* __IVP_UTILIZATION_H__ */
