/*
 * Copyright (C) 2016 MediaTek Inc.
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
#include <linux/kernel.h>
#include "ivp_driver.h"
#include "ivp_met_event.h"
#include "ivp_profile.h"
#include "ivp_reg.h"

#define DEFAULT_POLLING_PERIOD_NS (1000000)

#define IVP_PMU_DUMP

char trace_buffer[PAGE_SIZE];

#ifndef IVP_PMU_DUMP
#define ivp_pmu_printf(format, args...)
#else
#define ivp_pmu_printf(format, args...)    pr_debug(format, ##args)
#endif

/* define pmu control pair */
struct pmu_setting {
	const char *name;
	unsigned int selector;
	unsigned int mask;
};

struct pmu_setting setting[MAX_PMU_COUNTER] = {
	 /* cycle is using for time stamp */
	{"cycle", XTPERF_CNT_CYCLES, XTPERF_MASK_CYCLES},
	{"instructions", XTPERF_CNT_INSN, XTPERF_MASK_INSN_ALL},
	{"idma active",  XTPERF_CNT_IDMA, XTPERF_MASK_IDMA_ACTIVE_CYCLES},
	{"data load", XTPERF_CNT_D_STALL, XTPERF_MASK_D_STALL_UNCACHED_LOAD},
	{"inst miss", XTPERF_CNT_I_STALL, XTPERF_MASK_I_STALL_CACHE_MISS},
	{NULL, 0, 0},
	{NULL, 0, 0},
	{NULL, 0, 0},
};

static unsigned int make_pm_ctrl(unsigned int selector, unsigned int mask)
{
	unsigned int ctrl = PERF_PMCTRL_TRACELEVEL;

	ctrl |= selector << PERF_PMCTRL_SELECT_SHIFT;
	ctrl |= mask << PERF_PMCTRL_MASK_SHIFT;
	return ctrl;
}

static void ivp_profiler_core_config(struct ivp_core *ivp_core)
{
	int i;
	void __iomem *debug_base = ivp_core->ivpdebug_base;

	if (!debug_base)
		return;

	ivp_write_reg32(debug_base, 0x1000, 0x0);
	for (i = 0; i < MAX_PMU_COUNTER; i++) {
		unsigned int ctrl;

		if (setting[i].name == NULL)
			break;
		ctrl = make_pm_ctrl(setting[i].selector, setting[i].mask);
		ivp_write_reg32(debug_base, 0x1100 + i * 4, ctrl);
		ivp_write_reg32(debug_base, 0x1180 + i * 4, 0);
		ivp_write_reg32(debug_base, 0x1080 + i * 4, 0);

		ivp_core->pmu_counter[i] = 0;
	}

	ivp_write_reg32(debug_base, 0x1000, 0x1);
}

void ivp_profiler_core_read(struct ivp_core *ivp_core)
{
	int i, count = 0;
	void __iomem *debug_base = ivp_core->ivpdebug_base;
	char *buf = &trace_buffer[0];
	unsigned int diff[MAX_PMU_COUNTER] = {0};

	if (!debug_base)
		return;

	count += scnprintf(buf + count, PAGE_SIZE,
			 "vpu pmu core%d ", ivp_core->core);

	for (i = 0; i < MAX_PMU_COUNTER; i++) {
		unsigned int value;

		if (setting[i].name == NULL)
			break;
		value = ivp_read_reg32(debug_base, 0x1080 + i * 4);

		diff[i] = value - ivp_core->pmu_counter[i];
		count += scnprintf(buf + count, PAGE_SIZE,
				   "%s %d, ", setting[i].name, diff[i]);

		ivp_core->pmu_counter[i] = value;
	}

	ivp_pmu_printf("%s\n", buf);

	if (ivp_core->ivp_device->trace_func & IVP_TRACE_EVENT_PMU)
		ivp_met_event_pmu_polling(ivp_core->core,
					  diff[1], diff[2], diff[3], diff[4]);
}

static enum hrtimer_restart ivp_profiler_polling(struct hrtimer *timer)
{
	int core;

	struct ivp_profiler *profiler = container_of(timer,
						     struct ivp_profiler,
						     timer);

	struct ivp_device *ivp_device = container_of(profiler,
						     struct ivp_device,
						     profiler);

	/*call functions need to be called periodically*/
	for (core = 0; core < ivp_device->ivp_core_num; core++) {
		struct ivp_core *ivp_core = ivp_device->ivp_core[core];

		if (!ivp_core)
			continue;
		if (ivp_core->state == IVP_ACTIVE)
			ivp_profiler_core_read(ivp_core);
	}

	hrtimer_forward_now(&profiler->timer,
			    ns_to_ktime(DEFAULT_POLLING_PERIOD_NS));
	return HRTIMER_RESTART;
}

static int ivp_profiler_timer_start(struct ivp_device *ivp_device)
{
	if (!ivp_device->profiler.enable_timer)
		return -1;

	mutex_lock(&ivp_device->profiler.mutex);
	if (!atomic_read(&ivp_device->profiler.refcount)) {
		hrtimer_start(&ivp_device->profiler.timer,
			      ns_to_ktime(DEFAULT_POLLING_PERIOD_NS),
			      HRTIMER_MODE_REL);
		ivp_device->profiler.enabled = 1;
	}
	mutex_unlock(&ivp_device->profiler.mutex);

	atomic_inc(&ivp_device->profiler.refcount);
	return 0;
}

static int ivp_profiler_timer_stop(struct ivp_device *ivp_device)
{
	if (!ivp_device->profiler.enable_timer)
		return -1;

	atomic_dec(&ivp_device->profiler.refcount);
	mutex_lock(&ivp_device->profiler.mutex);
	if (atomic_read(&ivp_device->profiler.refcount) <= 0) {
		hrtimer_cancel(&ivp_device->profiler.timer);
		ivp_device->profiler.enabled = 0;
	}

	mutex_unlock(&ivp_device->profiler.mutex);
	return 0;
}

void ivp_profiler_core_enable(struct ivp_core *ivp_core)
{
	ivp_profiler_core_config(ivp_core);
	ivp_profiler_timer_start(ivp_core->ivp_device);
}

void ivp_profiler_core_disable(struct ivp_core *ivp_core)
{
	ivp_profiler_timer_stop(ivp_core->ivp_device);
}

int ivp_init_profiler(struct ivp_device *ivp_device, int enable_timer)
{
	if (enable_timer) {
		ivp_device->profiler.enable_timer = 1;
		mutex_init(&ivp_device->profiler.mutex);
		atomic_set(&ivp_device->profiler.refcount, 0);

		hrtimer_init(&ivp_device->profiler.timer,
			     CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ivp_device->profiler.timer.function = ivp_profiler_polling;
	}

	return 0;
}

int ivp_deinit_profiler(struct ivp_device *ivp_device)
{
	if (ivp_device->profiler.enabled)
		hrtimer_cancel(&ivp_device->profiler.timer);

	return 0;
}

