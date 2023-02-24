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

#include <linux/devfreq.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/timekeeper_internal.h>

#include "ivp_driver.h"
#include "ivp_profile.h"
#include "ivp_utilization.h"
#include "ivp_met_event.h"

#define IVP_TIMER_PERIOD_NS 200000000 /* ns = 200 ms */
#define IVP_TIMER_PERIOD_MS 200

static int ivp_sum_usage(struct ivp_core *ivp_core, ktime_t now)
{
	s64 diff;

	diff = ktime_us_delta(now, ivp_core->working_start);
	if (diff < 0)
		goto reset;

	if (ivp_core->state == IVP_ACTIVE)
		ivp_core->acc_busy += (unsigned long) diff;

reset:
	ivp_core->working_start = now;

	return 0;
}

int ivp_utilization_compute_enter(const struct ivp_device *ivp_device,
				  int coreid)
{
	unsigned long flags;
	struct ivp_core *ivp_core = ivp_device->ivp_core[coreid];
	struct ivp_util *ivp_util = ivp_device->ivp_util;
	int ret = 0;

	if (ivp_core) {
		spin_lock_irqsave(&ivp_util->lock, flags);
		ret = ivp_sum_usage(ivp_core, ktime_get());
		if (ivp_core->state == IVP_IDLE)
			ivp_core->state = IVP_ACTIVE;
		spin_unlock_irqrestore(&ivp_util->lock, flags);
		if (!ivp_device->profiler.enable_timer)
			ivp_profiler_core_read(ivp_core);
		if (ivp_device->trace_func & IVP_TRACE_EVENT_EXECUTE)
			ivp_met_event_cmd_enter(ivp_core->core,
						ivp_core->current_freq);
	}

	return ret;
}

int ivp_utilization_compute_leave(const struct ivp_device *ivp_device,
				  int coreid)
{
	unsigned long flags;
	struct ivp_core *ivp_core = ivp_device->ivp_core[coreid];
	struct ivp_util *ivp_util = ivp_device->ivp_util;
	int ret = 0;

	if (ivp_core) {
		spin_lock_irqsave(&ivp_util->lock, flags);
		ret = ivp_sum_usage(ivp_core, ktime_get());
		if (ivp_core->state == IVP_ACTIVE)
			ivp_core->state = IVP_IDLE;
		spin_unlock_irqrestore(&ivp_util->lock, flags);
		if (!ivp_device->profiler.enable_timer)
			ivp_profiler_core_read(ivp_core);
		if (ivp_device->trace_func & IVP_TRACE_EVENT_EXECUTE)
			ivp_met_event_cmd_leave(ivp_core->core, 0);
	}

	return ret;
}

int ivp_dvfs_get_usage(const struct ivp_core *ivp_core,
		       unsigned long *total_time, unsigned long *busy_time)
{
	unsigned long flags;
	unsigned long busy = 0;
	unsigned long total;
	struct ivp_device *ivp_device = ivp_core->ivp_device;
	struct ivp_util *ivp_util = ivp_device->ivp_util;
	int ret = 0;

	spin_lock_irqsave(&ivp_util->lock, flags);
	busy = ivp_core->prev_busy;
	total = ivp_util->prev_total;
	spin_unlock_irqrestore(&ivp_util->lock, flags);

	*busy_time = busy / USEC_PER_MSEC;
	*total_time = total;
	if (*busy_time > *total_time)
		*busy_time = *total_time;

	return ret;
}

static int ivp_timer_get_usage(struct ivp_util *ivp_util)
{
	ktime_t now = ktime_get();
	struct ivp_device *ivp_device = ivp_util->ivp_device;
	struct ivp_core *ivp_core;
	unsigned long flags;
	u32 core_id;
	s64 diff;
	int ret = 0;

	spin_lock_irqsave(&ivp_util->lock, flags);
	diff = ktime_ms_delta(now, ivp_util->period_start);
	if (diff < 0) {
		ivp_util->prev_total = 0;
		goto reset_usage;
	}

	ivp_util->prev_total = (unsigned long) diff;

	for (core_id = 0; core_id < ivp_device->ivp_core_num; core_id++) {
		ivp_core = ivp_device->ivp_core[core_id];
		if (!ivp_core)
			continue;
		ret = ivp_sum_usage(ivp_core, now);
		ivp_core->prev_busy = ivp_core->acc_busy;
	}

reset_usage:
	ivp_util->period_start = now;
	for (core_id = 0; core_id < ivp_device->ivp_core_num ; core_id++) {
		ivp_core = ivp_device->ivp_core[core_id];
		if (!ivp_core)
			continue;
		ivp_core->acc_busy = 0;
		ivp_core->working_start = ivp_util->period_start;
		if (ivp_device->trace_func & IVP_TRACE_EVENT_BUSYRATE)
			ivp_met_event_busyrate(core_id,
					       ivp_core->prev_busy,
					       ivp_util->prev_total);
	}

	spin_unlock_irqrestore(&ivp_util->lock, flags);

	return ret;
}

static enum hrtimer_restart ivp_timer_callback(struct hrtimer *timer)
{
	struct ivp_util *ivp_util;
	int ret = 0;

	ivp_util = container_of(timer, struct ivp_util, timer);

	ret = ivp_timer_get_usage(ivp_util);

	if (ivp_util->active) {
		hrtimer_start(&ivp_util->timer, ivp_util->period_time,
			      HRTIMER_MODE_REL);
	}

	return HRTIMER_NORESTART;
}

int ivp_init_util(struct ivp_device *ivp_device)
{
	struct device *dev = ivp_device->dev;
	struct ivp_util *ivp_util;
	struct ivp_core *ivp_core;
	ktime_t now = ktime_get();
	int core_id;
	int ret = 0;

	ivp_util = devm_kzalloc(dev, sizeof(*ivp_util), GFP_KERNEL);
	if (!ivp_util)
		return -ENOMEM;

	ivp_util->dev = dev;
	ivp_util->ivp_device = ivp_device;

	ivp_util->period_time = ktime_set(0, IVP_TIMER_PERIOD_NS);
	ivp_util->period_start = now;

	for (core_id = 0; core_id < ivp_device->ivp_core_num ; core_id++) {
		ivp_core = ivp_device->ivp_core[core_id];
		if (!ivp_core)
			continue;
		ivp_core->prev_busy = 0;
		ivp_core->acc_busy = 0;
		ivp_core->working_start = ivp_util->period_start;
	}
	ivp_util->prev_total = 0;

	spin_lock_init(&ivp_util->lock);

	ivp_util->active = true;
	hrtimer_init(&ivp_util->timer, CLOCK_MONOTONIC,
		     HRTIMER_MODE_REL);

	ivp_util->timer.function = ivp_timer_callback;
	hrtimer_start(&ivp_util->timer, ivp_util->period_time,
		      HRTIMER_MODE_REL);

	ivp_device->ivp_util = ivp_util;

	return ret;
}

int ivp_deinit_util(struct ivp_device *ivp_device)
{
	struct ivp_util *ivp_util = ivp_device->ivp_util;
	unsigned long flags;
	int ret = 1;

	spin_lock_irqsave(&ivp_util->lock, flags);
	ivp_util->active = false;
	spin_unlock_irqrestore(&ivp_util->lock, flags);

	while (ret > 0)
		ret = hrtimer_cancel(&ivp_util->timer);

	return ret;
}
