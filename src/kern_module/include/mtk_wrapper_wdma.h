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

#ifndef _MTK_WRAPPER_WDMA_
#define _MTK_WRAPPER_WDMA_

#include <linux/ioctl.h>

#include <linux/types.h>

#ifdef __KERNEL__
#include <linux/dma-mapping.h>
#include <uapi/drm/drm_fourcc.h>
#else
#include <stdbool.h>
#include <stdint.h>
#endif

#define WDMA_DEVICE_IOC_TYPE 'W'
#define WDMA_MAX_BUFFER_NUM (5)

typedef struct _args_wdma_set_region {
	__u32 in_w;
	__u32 in_h;
	__u32 out_w;
	__u32 out_h;
	__u32 crop_w;
	__u32 crop_h;
} args_wdma_set_region;

typedef struct _args_wdma_set_output_buffer {
	__u32 pitch;
	__u32 format;
	__u32 size;
	__u32 height;
	int buffer_num;
	__u32 pvric_header_offset;
	int ionfd[WDMA_MAX_BUFFER_NUM];
} args_wdma_set_output_buffer;

typedef struct _args_wdma_stop {
	__u32 sync_stop;
} args_wdma_stop;

#define WDMA_POWER_ON     _IO(WDMA_DEVICE_IOC_TYPE, 1)
#define WDMA_POWER_OFF    _IO(WDMA_DEVICE_IOC_TYPE, 2)
#define WDMA_RESET        _IO(WDMA_DEVICE_IOC_TYPE, 3)
#define WDMA_START        _IO(WDMA_DEVICE_IOC_TYPE, 4)
#define WDMA_STOP            _IOW(WDMA_DEVICE_IOC_TYPE, 5, args_wdma_stop)
#define WDMA_SET_REGION      _IOW(WDMA_DEVICE_IOC_TYPE, 6, args_wdma_set_region)
#define WDMA_SET_OUT_BUFFER     _IOW(WDMA_DEVICE_IOC_TYPE, 7, args_wdma_set_output_buffer)
#define WDMA_ENABLE_PVRIC  _IO(WDMA_DEVICE_IOC_TYPE, 9)
#define WDMA_DISABLE_PVRIC _IO(WDMA_DEVICE_IOC_TYPE, 10)
#define WDMA_WAIT_NEXT_SYNC _IO(WDMA_DEVICE_IOC_TYPE, 11)

#endif /* _MTK_WRAPPER_WDMA_ */
