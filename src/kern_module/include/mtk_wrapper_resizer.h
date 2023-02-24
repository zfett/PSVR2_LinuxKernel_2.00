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

#ifndef _MTK_WRAPPER_RESIZER_
#define _MTK_WRAPPER_RESIZER_

#include <linux/ioctl.h>
#include <linux/types.h>

#include "mtk_wrapper_common.h"

#define RESIZER_DEVICE_IOC_TYPE 'Z'

typedef struct _args_resizer_set_config {
	__u32 in_width;
	__u32 in_height;
	__u32 out_width;
	__u32 out_height;
} args_resizer_set_config;

#define RESIZER_POWER_ON     _IOW(RESIZER_DEVICE_IOC_TYPE, 1, uint8_t)
#define RESIZER_POWER_OFF    _IO(RESIZER_DEVICE_IOC_TYPE, 2)
#define RESIZER_START        _IO(RESIZER_DEVICE_IOC_TYPE, 3)
#define RESIZER_STOP         _IO(RESIZER_DEVICE_IOC_TYPE, 4)
#define RESIZER_SET_CONFIG   _IOW(RESIZER_DEVICE_IOC_TYPE, 5, args_resizer_set_config)
#define RESIZER_BYPASS       _IO(RESIZER_DEVICE_IOC_TYPE, 6)

#endif /* _MTK_WRAPPER_RESIZER_ */
