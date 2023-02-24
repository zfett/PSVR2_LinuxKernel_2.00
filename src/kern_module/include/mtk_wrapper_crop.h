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

#ifndef _MTK_WRAPPER_CROP_
#define _MTK_WRAPPER_CROP_

#include <linux/ioctl.h>
#include <linux/types.h>

#define CROP_DEVICE_IOC_TYPE 'C'

typedef struct _mtk_wrapper_crop_region {
	__u32 in_w;
	__u32 in_h;
	__u32 out_x;
	__u32 out_y;
	__u32 out_w;
	__u32 out_h;
} mtk_wrapper_crop_region;

typedef struct _args_crop_set_region {
	mtk_wrapper_crop_region region;
} args_crop_set_region;

#define CROP_POWER_ON     _IOW(CROP_DEVICE_IOC_TYPE, 1, uint8_t)
#define CROP_POWER_OFF    _IO(CROP_DEVICE_IOC_TYPE, 2)
#define CROP_RESET        _IO(CROP_DEVICE_IOC_TYPE, 3)
#define CROP_SET_REGION   _IOW(CROP_DEVICE_IOC_TYPE, 4, args_crop_set_region)
#define CROP_START        _IO(CROP_DEVICE_IOC_TYPE, 5)
#define CROP_STOP         _IO(CROP_DEVICE_IOC_TYPE, 6)

#endif /* _MTK_WRAPPER_CROP_*/
