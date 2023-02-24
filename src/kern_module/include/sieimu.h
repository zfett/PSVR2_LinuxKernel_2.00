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

#pragma once

#include <linux/ioctl.h>
#include <linux/types.h>
#include "sce_km_defs.h"

#define SIE_IMU_DEVICE_IOC_TYPE 'I'
#define SIE_IMU_DEVCE_NAME "imu"

#define SIE_IMU_INVALID_VALUE ((__s16)0x8000)  /* INVALID_VALUE_FIFO */
#define SIE_IMU_STATUS_INVALID BIT(0)

#define SIE_IMU_RINGBUFFER_SIZE (2000) /* 1sec@2KHz buffer */

struct sieimu_data {
	__s16 accel[3];
	__s16 gyro[3];
	__u32 vts;
	__u16 dp_frame_cnt;
	__u16 dp_line_cnt;
	__u16 imu_ts;
	__u16 status;
};

struct sieimu_get_buf_data {
	__u32 buf_cnt;
	struct sieimu_data *data;
};

struct sieimu_get_write_pos {
	__u32 write_pos;
	__u32 wrap_cnt;
};

#define SIE_IMU_IO_GET_WRITE_POS \
	(_IOR(SIE_IMU_DEVICE_IOC_TYPE, 1, struct sieimu_get_write_pos))
#define SIE_IMU_IO_GET_TEMPERATURE \
	(_IOR(SIE_IMU_DEVICE_IOC_TYPE, 2, __s8))

#ifdef __KERNEL__
struct sieimu_data *sieimuGetFifoAddress(void *handle);
int sieimuGetFifoSize(void *handle);
int sieimuRequestCallBack(void *handle,
			  void (*callback)(struct sieimu_data *ring_buffer));
#endif
