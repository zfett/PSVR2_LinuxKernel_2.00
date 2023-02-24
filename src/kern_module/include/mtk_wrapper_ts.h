/*
 * Copyright (C) 2020 Sony Interactive Entertainment Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _MTK_WRAPPER_TS_
#define _MTK_WRAPPER_TS_

#include <linux/ioctl.h>
#include <linux/types.h>

#define TS_DEVICE_IOC_TYPE 'T'

typedef struct _ts_info {
	__u32 dp_counter;
	__u32 vts;
	__u64 stc;
} ts_info;

typedef struct _ts_av_info {
	__u32 dp_counter;
	__u64 stc;
} ts_av_info;

typedef struct _ts_vision_info {
	__u32 dp_counter;
	__u32 vts;
} ts_vision_info;

typedef struct _ts_imu_info {
	__u32 dp_counter;
	__u32 vts;
	__u16 imu_counter;
} ts_imu_info;

typedef struct _ts_stc_set {
	__u32 stc_base_31_0;
	__u32 stc_base_32;
	__u32 stc_ext_8_0;
} ts_stc_setting;

#define TS_SET_VTS                        _IOW(TS_DEVICE_IOC_TYPE, 1, uint32_t)
#define TS_SET_STC                        _IOW(TS_DEVICE_IOC_TYPE, 2, ts_stc_setting)
#define TS_GET_CUR_TIME                   _IOR(TS_DEVICE_IOC_TYPE, 3, ts_info)
#define TS_GET_DSI_TIME                   _IOR(TS_DEVICE_IOC_TYPE, 4, ts_av_info)
#define TS_GET_AUDIO_HDMIDP_TIME          _IOR(TS_DEVICE_IOC_TYPE, 5, ts_av_info)
#define TS_GET_AUDIO_COMMON_TIME          _IOR(TS_DEVICE_IOC_TYPE, 6, ts_av_info)
#define TS_GET_AUDIO_TDMOUT_TIME          _IOR(TS_DEVICE_IOC_TYPE, 7, ts_av_info)
#define TS_GET_DPRX_VIDEO_TIME            _IOR(TS_DEVICE_IOC_TYPE, 8, ts_av_info)
#define TS_GET_SIDE0_A_P1_VISION_TIME     _IOR(TS_DEVICE_IOC_TYPE, 9, ts_vision_info)
#define TS_GET_SIDE0_B_P1_VISION_TIME     _IOR(TS_DEVICE_IOC_TYPE, 10, ts_vision_info)
#define TS_GET_SIDE1_A_P1_VISION_TIME     _IOR(TS_DEVICE_IOC_TYPE, 11, ts_vision_info)
#define TS_GET_SIDE1_B_P1_VISION_TIME     _IOR(TS_DEVICE_IOC_TYPE, 12, ts_vision_info)
#define TS_GET_SIDE0_A_P2_VISION_TIME     _IOR(TS_DEVICE_IOC_TYPE, 13, ts_vision_info)
#define TS_GET_SIDE0_B_P2_VISION_TIME     _IOR(TS_DEVICE_IOC_TYPE, 14, ts_vision_info)
#define TS_GET_SIDE1_A_P2_VISION_TIME     _IOR(TS_DEVICE_IOC_TYPE, 15, ts_vision_info)
#define TS_GET_SIDE1_B_P2_VISION_TIME     _IOR(TS_DEVICE_IOC_TYPE, 16, ts_vision_info)
#define TS_GET_GAZE0_VISION_TIME          _IOR(TS_DEVICE_IOC_TYPE, 17, ts_vision_info)
#define TS_GET_GAZE1_VISION_TIME          _IOR(TS_DEVICE_IOC_TYPE, 18, ts_vision_info)
#define TS_GET_SIDE0_CAMERA_SYNC_TIME     _IOR(TS_DEVICE_IOC_TYPE, 19, ts_vision_info)
#define TS_GET_SIDE1_CAMERA_SYNC_TIME     _IOR(TS_DEVICE_IOC_TYPE, 20, ts_vision_info)
#define TS_GET_GAZE0_CAMERA_SYNC_TIME     _IOR(TS_DEVICE_IOC_TYPE, 21, ts_vision_info)
#define TS_GET_GAZE1_CAMERA_SYNC_TIME     _IOR(TS_DEVICE_IOC_TYPE, 22, ts_vision_info)
#define TS_GET_GAZE2_CAMERA_SYNC_TIME     _IOR(TS_DEVICE_IOC_TYPE, 23, ts_vision_info)
#define TS_GET_IMU0_TIME                  _IOR(TS_DEVICE_IOC_TYPE, 24, ts_imu_info)
#define TS_GET_IMU1_TIME                  _IOR(TS_DEVICE_IOC_TYPE, 25, ts_imu_info)

#endif /* _MTK_WRAPPER_TS_*/
