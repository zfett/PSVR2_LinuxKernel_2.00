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

#undef TRACE_SYSTEM
#define TRACE_SYSTEM met_vpusys_events
#if !defined(_TRACE_MET_VPUSYS_EVENTS_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_MET_VPUSYS_EVENTS_H
#include <linux/tracepoint.h>

#define MX_LEN_STR_DESC (128)

TRACE_EVENT(VPU__cmd_enter,
	TP_PROTO(int core, int freq),
	TP_ARGS(core, freq),
	TP_STRUCT__entry(
		__field(int, core)
		__field(int, freq)
		),
	TP_fast_assign(
		__entry->core = core;
		__entry->freq = freq;
	),
	TP_printk("_id=c%d, vpu_freq=%d",
		  __entry->core, __entry->freq)
);


TRACE_EVENT(VPU__cmd_leave,
	TP_PROTO(int core, int dummy),
	TP_ARGS(core, dummy),
	TP_STRUCT__entry(
		__field(int, core)
		__field(int, dummy)
	),
	TP_fast_assign(
		__entry->core = core;
		__entry->dummy = dummy;
	),
	TP_printk("_id=c%d", __entry->core)
);

TRACE_EVENT(VPU__polling,
	TP_PROTO(int core, int value1, int value2, int value3, int value4),
	TP_ARGS(core, value1, value2, value3, value4),
	TP_STRUCT__entry(
		__field(int, core)
		__field(int, value1)
		__field(int, value2)
		__field(int, value3)
		__field(int, value4)
		),
	TP_fast_assign(
		__entry->core = core;
		__entry->value1 = value1;
		__entry->value2 = value2;
		__entry->value3 = value3;
		__entry->value4 = value4;
	),
	TP_printk(
		"_id=c%d, instruction_cnt=%d, idma_active=%d, uncached_data_stall=%d, icache_miss_stall=%d",
		__entry->core,
		__entry->value1,
		__entry->value2,
		__entry->value3,
		__entry->value4)
);

TRACE_EVENT(VPU__busyrate,
	TP_PROTO(int core, int value1),
	TP_ARGS(core, value1),
	TP_STRUCT__entry(
		__field(int, core)
		__field(int, value1)
	),
	TP_fast_assign(
		__entry->core = core;
		__entry->value1 = value1;
	),
	TP_printk("_id=c%d, busyrate=%d", __entry->core, __entry->value1)
);

TRACE_EVENT(VPU__dvfs,
	TP_PROTO(int voltage, int vpu0_freq, int vpu1_freq, int vpu2_freq),
	TP_ARGS(voltage, vpu0_freq, vpu1_freq, vpu2_freq),
	TP_STRUCT__entry(
		__field(int, voltage)
		__field(int, vpu0_freq)
		__field(int, vpu1_freq)
		__field(int, vpu2_freq)
		),
	TP_fast_assign(
		__entry->voltage = voltage;
		__entry->vpu0_freq = vpu0_freq;
		__entry->vpu1_freq = vpu1_freq;
		__entry->vpu2_freq = vpu2_freq;
	),
	TP_printk(
		"voltage=%d, vpu0_freq=%d,vpu1_freq=%d,vpu2_freq=%d",
		__entry->voltage,
		__entry->vpu0_freq,
		__entry->vpu1_freq,
		__entry->vpu2_freq)
);

#endif /* _TRACE_MET_VPUSYS_EVENTS_H */

/* This part must be outside protection */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE met_vpusys_events
#include <trace/define_trace.h>
