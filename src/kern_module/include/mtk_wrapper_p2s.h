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

#ifndef _MTK_WRAPPER_P2S_
#define _MTK_WRAPPER_P2S_

#include <linux/ioctl.h>
#include <linux/types.h>

#define P2S_DEVICE_IOC_TYPE 'P'

typedef enum {
	MTK_WRAPPER_P2S_MODE_L_ONLY,
	MTK_WRAPPER_P2S_MODE_LR,
} MTK_WRAPPER_P2S_MODE;

typedef struct _args_p2s_set_region {
	__u32 width;
	__u32 height;
} args_p2s_set_region;

#define P2S_POWER_ON     _IO(P2S_DEVICE_IOC_TYPE, 1)
#define P2S_POWER_OFF    _IO(P2S_DEVICE_IOC_TYPE, 2)
#define P2S_SET_MODE     _IOW(P2S_DEVICE_IOC_TYPE, 3, MTK_WRAPPER_P2S_MODE)
#define P2S_SET_REGION   _IOW(P2S_DEVICE_IOC_TYPE, 4, args_p2s_set_region)
#define P2S_START        _IO(P2S_DEVICE_IOC_TYPE, 5)
#define P2S_STOP         _IO(P2S_DEVICE_IOC_TYPE, 6)
#endif /* _MTK_WRAPPER_P2S_ */
