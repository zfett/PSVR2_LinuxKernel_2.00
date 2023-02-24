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

#ifndef _IVP_MET_EVENT_H_
#define _IVP_MET_EVENT_H_

#define IVP_TRACE_EVENT_EXECUTE  0x01
#define IVP_TRACE_EVENT_DVFS     0x02
#define IVP_TRACE_EVENT_BUSYRATE 0x04
#define IVP_TRACE_EVENT_PMU      0x08

#define IVP_TRACE_EVENT_MASK     0x0f

void ivp_met_event_cmd_enter(int coreid, long freq);
void ivp_met_event_cmd_leave(int coreid, int dummy);
void ivp_met_event_busyrate(int coreid, int busy, int total);
void ivp_met_event_dvfs_trace(int voltage, int freq0, int freq1, int freq2);
void ivp_met_event_pmu_polling(int coreid, int instcnt,
			       int dmacnt, int datastall, int icachemiss);

#endif /*_IVP_MET_EVENT_H_ */
