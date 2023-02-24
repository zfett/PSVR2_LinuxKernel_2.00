/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

/**
 * @file kd_camera_hw.h
 * Header file of kd_camera_hw.c.
 */

#ifndef _KD_CAMERA_HW_H_
#define _KD_CAMERA_HW_H_

extern int i2c_read_data(const struct i2c_client *client,
			 const u8 *send_data, const u16 send_size,
			 u8 *recv_data, u16 recv_size);

extern int i2c_write_data(const struct i2c_client *client,
			  const u8 *data,
			  const u16 count);
extern u32 fpga_mode;
#endif
