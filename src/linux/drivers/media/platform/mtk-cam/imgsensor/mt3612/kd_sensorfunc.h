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
 * @file kd_sensorfunc.h
 * This is header file of kd_sensorfunc.c
 */

#ifndef _KD_SENSOR_FUNC_H
#define _KD_SENSOR_FUNC_H

extern int i2c_read_data(const struct i2c_client *client,
			 const u8 *send_data, const u16 send_size,
			 u8 *recv_data, u16 recv_size);

extern int i2c_write_data(const struct i2c_client *client,
			  const u8 *data,
			  const u16 count);
#endif

