/*
 * Copyright (c) 2018 MediaTek Inc.
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

#ifndef MTK_TS_H
#define MTK_TS_H

#include <linux/device.h>
#include <linux/types.h>

/** @ingroup IP_group_timestamp_external_enum
 * @brief IMU trigger edge
 */
enum mtk_ts_imu_trigger_edge {
	/** 0: fall+rise edge */
	MTK_TS_IMU_FALL_RISE_EDGE = 0,
	/** 1: fall edge */
	MTK_TS_IMU_FALL_EDGE = 1,
	/** 2: fall+rise edge */
	MTK_TS_IMU_FALL_RISE_EDGE2 = 2,
	/** 3: rise edge */
	MTK_TS_IMU_RISE_EDGE = 3,
};

int mtk_ts_set_vts(const struct device *dev, const u32 vts);
int mtk_ts_set_stc(const struct device *dev,
		   const u32 stc_base_31_0,
		   const u32 stc_base_32,
		   const u32 stc_ext_8_0);
int mtk_ts_select_imu_0_trigger_edge(const struct device *dev,
				    const enum mtk_ts_imu_trigger_edge edge);
int mtk_ts_select_imu_1_trigger_edge(const struct device *dev,
				    const enum mtk_ts_imu_trigger_edge edge);
int mtk_ts_get_current_time(const struct device *dev,
		u32 *vts, u64 *stc, u32 *dp_counter);
int mtk_ts_get_dsi_time(const struct device *dev,
		u64 *stc, u32 *dp_counter);
int mtk_ts_get_audio_hdmidp_time(const struct device *dev,
		u64 *stc, u32 *dp_counter);
int mtk_ts_get_audio_common_time(const struct device *dev,
		u64 *stc, u32 *dp_counter);
int mtk_ts_get_audio_tdmout_time(const struct device *dev,
		u64 *stc, u32 *dp_counter);
int mtk_ts_get_dprx_video_time(const struct device *dev,
		u64 *stc, u32 *dp_counter);
int mtk_ts_get_side_0_a_p1_vision_time(const struct device *dev,
		u32 *vts, u32 *dp_counter);
int mtk_ts_get_side_0_b_p1_vision_time(const struct device *dev,
		u32 *vts, u32 *dp_counter);
int mtk_ts_get_side_1_a_p1_vision_time(const struct device *dev,
		u32 *vts, u32 *dp_counter);
int mtk_ts_get_side_1_b_p1_vision_time(const struct device *dev,
		u32 *vts, u32 *dp_counter);
int mtk_ts_get_side_0_a_p2_vision_time(const struct device *dev,
		u32 *vts, u32 *dp_counter);
int mtk_ts_get_side_0_b_p2_vision_time(const struct device *dev,
		u32 *vts, u32 *dp_counter);
int mtk_ts_get_side_1_a_p2_vision_time(const struct device *dev,
		u32 *vts, u32 *dp_counter);
int mtk_ts_get_side_1_b_p2_vision_time(const struct device *dev,
		u32 *vts, u32 *dp_counter);
int mtk_ts_get_gaze_0_vision_time(const struct device *dev,
		u32 *vts, u32 *dp_counter);
int mtk_ts_get_gaze_1_vision_time(const struct device *dev,
		u32 *vts, u32 *dp_counter);
int mtk_ts_get_side_0_camera_sync_time(const struct device *dev,
		u32 *vts, u32 *dp_counter);
int mtk_ts_get_side_1_camera_sync_time(const struct device *dev,
		u32 *vts, u32 *dp_counter);
int mtk_ts_get_gaze_0_camera_sync_time(const struct device *dev,
		u32 *vts, u32 *dp_counter);
int mtk_ts_get_gaze_1_camera_sync_time(const struct device *dev,
		u32 *vts, u32 *dp_counter);
int mtk_ts_get_gaze_2_camera_sync_time(const struct device *dev,
		u32 *vts, u32 *dp_counter);
int mtk_ts_get_imu_0_time(const struct device *dev,
		u32 *vts, u32 *dp_counter, u16 *imu_counter);
int mtk_ts_get_imu_1_time(const struct device *dev,
		u32 *vts, u32 *dp_counter, u16 *imu_counter);

#endif /* MTK_TS_H */
