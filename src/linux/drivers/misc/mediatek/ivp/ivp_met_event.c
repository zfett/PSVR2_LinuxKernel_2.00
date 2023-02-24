/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: Chiawen Lee <chiawen.lee@mediatek.com>
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


#include "ivp_driver.h"

#define CREATE_TRACE_POINTS
#include "met_vpusys_events.h"

void ivp_met_event_cmd_enter(int coreid, long freq)
{
	int freq_khz = freq / 1000;

	trace_VPU__cmd_enter(coreid, freq_khz);
}

void ivp_met_event_cmd_leave(int coreid, int dummy)
{
	trace_VPU__cmd_leave(coreid, 0);
}

void ivp_met_event_busyrate(int coreid, int busy, int total)
{
	int busyrate = 100 * busy / USEC_PER_MSEC;

	if (total)
		busyrate = busyrate / total;
	else
		busyrate = 100;

	trace_VPU__busyrate(coreid, busyrate);
}

void ivp_met_event_dvfs_trace(int voltage, int freq0, int freq1, int freq2)
{
	trace_VPU__dvfs(voltage, freq0, freq1, freq2);
}

void ivp_met_event_pmu_polling(int coreid, int instcnt,
			       int dmacnt, int datastall, int icachemiss)
{
	trace_VPU__polling(coreid, instcnt, dmacnt, datastall, icachemiss);
}
