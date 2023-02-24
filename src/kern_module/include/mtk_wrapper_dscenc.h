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

#ifndef _MTK_WRAPPER_DSCENC_
#define _MTK_WRAPPER_DSCENC_

#include <linux/ioctl.h>
#include <linux/types.h>
#include "mtk_wrapper_dsi.h"

#define DSCENC_DEVICE_IOC_TYPE 'E'

#define DSCENC_PPS_SIZE (19)

typedef enum {
	MTK_WRAPPER_DSCENC_1_IN_1_OUT,
	MTK_WRAPPER_DSCENC_2_IN_1_OUT,
	MTK_WRAPPER_DSCENC_2_IN_2_OUT,
} MTK_WRAPPER_DSCENC_INOUT_CONFIG;

typedef enum {
	MTK_WRAPPER_DSCENC_VERSION_1_1,
	MTK_WRAPPER_DSCENC_VERSION_1_2,
} MTK_WRAPPER_DSCENC_VERSION;

typedef enum {
	MTK_WRAPPER_ICH_LINE_CLEAR_AUTO,
	MTK_WRAPPER_ICH_LINE_CLEAR_AT_LINE,
	MTK_WRAPPER_ICH_LINE_CLEAR_AT_SLICE,
} MTK_WRAPPER_DSCENC_ICH_LINE_CLEAR;

typedef struct _dscenc_config {
	unsigned int disp_pic_w;
	unsigned int disp_pic_h;
	unsigned int slice_h;
	MTK_WRAPPER_DSCENC_INOUT_CONFIG inout_sel;
	MTK_WRAPPER_DSCENC_VERSION version;
	MTK_WRAPPER_DSCENC_ICH_LINE_CLEAR ich_line_clear;
	MTK_WRAPPER_DSI_FORMAT format;
	const __u32 *pps;
} dscenc_config;

typedef struct _args_dsc_init_hw {
	dscenc_config config;
} args_dsc_init_hw;

#define DSCENC_POWER_ON     _IO(DSCENC_DEVICE_IOC_TYPE, 1)
#define DSCENC_INIT_HW      _IOW(DSCENC_DEVICE_IOC_TYPE, 2, args_dsc_init_hw)
#define DSCENC_START        _IO(DSCENC_DEVICE_IOC_TYPE, 3)
#define DSCENC_RESET        _IO(DSCENC_DEVICE_IOC_TYPE, 4)
#define DSCENC_RELAY        _IO(DSCENC_DEVICE_IOC_TYPE, 5)
#define DSCENC_POWER_OFF    _IO(DSCENC_DEVICE_IOC_TYPE, 6)

#endif /* _MTK_WRAPPER_DSCENC_*/
