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

#ifndef _MTK_WRAPPER_SLICER_
#define _MTK_WRAPPER_SLICER_

#include <linux/ioctl.h>
#include <linux/types.h>

#define SLICER_DEVICE_IOC_TYPE 'S'

typedef enum _MTK_WRAPPER_SLICER_INPUT_SELECT {
	MTK_WRAPPER_SLICER_INPUT_SELECT_VIDEO,
	MTK_WRAPPER_SLICER_INPUT_SELECT_DSC,
	MTK_WRAPPER_SLICER_INPUT_SELECT_VIDEO_DSC,
} MTK_WRAPPER_SLICER_INPUT_SELECT;

typedef struct _mtk_wrapper_slicer_video {
	int width;
	int height;
	bool in_hsync_inv;
	bool in_vsync_inv;
	int sop[4];
	int eop[4];
	int vid_en[4];
} mtk_wrapper_slicer_video;

typedef struct _mtk_wrapper_slicer_gce {
	int width;
	int height;
} mtk_wrapper_slicer_gce;

typedef struct _mtk_wrapper_slicer_dsc {
	int width;
	int height;
	bool in_hsync_inv;
	bool in_vsync_inv;
	int sop[4];
	int eop[4];
	__u8 bit_rate;
	__u8 chunk_num;
	__u8 valid_byte;
} mtk_wrapper_slicer_dsc;

typedef struct _args_mtk_wrapper_slicer_set_config {
	MTK_WRAPPER_SLICER_INPUT_SELECT inp_sel;
	mtk_wrapper_slicer_video vid;
	mtk_wrapper_slicer_dsc   dsc;
	mtk_wrapper_slicer_gce   gce;
} args_slicer_set_config;

#define SLICER_POWER_ON     _IO(SLICER_DEVICE_IOC_TYPE, 1)
#define SLICER_POWER_OFF    _IO(SLICER_DEVICE_IOC_TYPE, 2)
#define SLICER_START        _IO(SLICER_DEVICE_IOC_TYPE, 3)
#define SLICER_STOP         _IO(SLICER_DEVICE_IOC_TYPE, 4)
#define SLICER_RESET        _IO(SLICER_DEVICE_IOC_TYPE, 5)
#define SLICER_SET_CONFIG _IOW(SLICER_DEVICE_IOC_TYPE, 6, args_slicer_set_config)

#endif /* _MTK_WRAPPER_SLICER_ */
