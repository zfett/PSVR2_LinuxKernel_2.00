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

#ifndef _MTK_WRAPPER_MMSYSCFG_H_
#define _MTK_WRAPPER_MMSYSCFG_H_

#include <linux/ioctl.h>
#include "mtk_wrapper_common.h"

#define MMSYSCFG_DEVICE_IOC_TYPE 'M'

typedef enum {
	MTK_WRAPPER_MMSYSCFG_CAMERA_SIDE,
	MTK_WRAPPER_MMSYSCFG_CAMERA_GAZE,
	MTK_WRAPPER_MMSYSCFG_CAMERA_GAZE_LED,
	MTK_WRAPPER_MMSYSCFG_CAMERA_MAX,
} MTK_WRAPPER_MMSYSCFG_CAMERA_ID;

typedef struct _args_mmsyscfg_connect_component {
	DISPLAY_COMPONENT cur;
	DISPLAY_COMPONENT next;
} args_mmsyscfg_connect_component;

typedef struct _args_mmsyscfg_disconnect_component {
	DISPLAY_COMPONENT cur;
	DISPLAY_COMPONENT next;
} args_mmsyscfg_disconnect_component;

typedef struct _args_mmsyscfg_camera_sync_config {
	MTK_WRAPPER_MMSYSCFG_CAMERA_ID camera_id;
	__u32 vsync_cycle;
	__u32 delay_cycle;
	bool vsync_low_active;
} args_mmsyscfg_camera_sync_config;

typedef struct _args_mmsyscfg_camera_sync_frc {
	MTK_WRAPPER_MMSYSCFG_CAMERA_ID camera_id;
	int panel_fps;
	int camera_fps;
} args_mmsyscfg_camera_sync_frc;

typedef struct _args_mmsyscfg_dsi_lane_swap_config {
	__u32 swap_config[4];
} args_mmsyscfg_dsi_lane_swap_config;

typedef struct _args_mmsyscfg_lhc_swap_config {
	__u32 swap_config[4];
} args_mmsyscfg_lhc_swap_config;

#define MMSYSCFG_CONNECT_COMPONENT _IOW(MMSYSCFG_DEVICE_IOC_TYPE, 1, args_mmsyscfg_connect_component)
#define MMSYSCFG_DISCONNECT_COMPONENT _IOW(MMSYSCFG_DEVICE_IOC_TYPE, 2, args_mmsyscfg_connect_component)
#define MMSYSCFG_RESET_MODULE      _IO(MMSYSCFG_DEVICE_IOC_TYPE, 3)
#define MMSYSCFG_CAMERA_SYNC_CLOCK_SEL _IOW(MMSYSCFG_DEVICE_IOC_TYPE, 4, bool)
#define MMSYSCFG_CAMERA_SYNC_CONFIG _IOW(MMSYSCFG_DEVICE_IOC_TYPE, 5, args_mmsyscfg_camera_sync_config)
#define MMSYSCFG_CAMERA_SYNC_FRC    _IOW(MMSYSCFG_DEVICE_IOC_TYPE, 6, args_mmsyscfg_camera_sync_frc)
#define MMSYSCFG_CAMERA_SYNC_ENABLE _IOW(MMSYSCFG_DEVICE_IOC_TYPE, 7, MTK_WRAPPER_MMSYSCFG_CAMERA_ID)
#define MMSYSCFG_CAMERA_SYNC_DISABLE _IOW(MMSYSCFG_DEVICE_IOC_TYPE, 8, MTK_WRAPPER_MMSYSCFG_CAMERA_ID)
#define MMSYSCFG_DSI_LANE_SWAP_CONFIG _IOW(MMSYSCFG_DEVICE_IOC_TYPE, 9, args_mmsyscfg_dsi_lane_swap_config)
#define MMSYSCFG_SLCR_TO_DDDS_SELECT _IOW(MMSYSCFG_DEVICE_IOC_TYPE, 10, DISPLAY_COMPONENT)
#define MMSYSCFG_LHC_SWAP_CONFIG _IOW(MMSYSCFG_DEVICE_IOC_TYPE, 11, args_mmsyscfg_lhc_swap_config)

#endif /* _MKT_WRAPPER_MMSYSCFG_H_ */
