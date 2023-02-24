/*
 * Copyright (c) 2019 MediaTek Inc.
 * Authors: David Yeh <david.yeh@mediatek.com>
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

/**
 * @file mtk_ts_int.h
 *     Internal header for mtk_ts.c
 */

#ifndef MTK_TS_INT_H
#define MTK_TS_INT_H

#include "timestamp_reg.h"

/** @ingroup IP_group_timestamp_internal_def
 * @{
 */
#define VTS_CLK_1M			0
#define VTS_CLK_27M			TS_TS_CLK_SEL
#define VTS_DISABLE			0
#define VTS_ENABLE			TS_VTSC_EN
#define STC_MSB_SHIFTER			10
#define STC_LOAD_VALUE_BASE_32_SHIFTER	31
#define STA_IMU0_CURRENT_CNT_SHIFTER	16
#define STA_IMU1_CURRENT_CNT_SHIFTER	0
#define IMU_TRIG_EDGE_SHIFTER		6

/* STC CNTL SET/ACK */
#define STC_CNTL_CLR			0
#define STC_CNTL_LOAD			2
#define ACK_STC_CNTL_LOAD		(2 << 16)
#define ACK_STC_CNTL_POLLING_LOOP	100
/** @} */

/** @ingroup IP_group_timestamp_internal_struct
 * @brief Private device data structure for timestamp driver
 */
struct mtk_ts {
	/** device node */
	struct device	*dev;
	/** register base */
	void __iomem	*regs;
};

#endif /* MTK_TS_INT_H */
