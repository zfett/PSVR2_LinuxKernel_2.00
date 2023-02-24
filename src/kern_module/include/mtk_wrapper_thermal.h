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

#ifndef _MTK_WRAPPER_THERMAL_
#define _MTK_WRAPPER_THERMAL_

#include <linux/ioctl.h>
#include <linux/types.h>

#define THERMAL_DEVICE_IOC_TYPE 'T'

/* bank_id */
typedef enum _THERMAL_BANK_ID {
	THERMAL_BANK0 = 0,
	THERMAL_BANK_MAX,
} THERMAL_BANK_ID;

/* sensor id */
typedef enum _THERMAL_SENSOR_ID {
	THERMAL_SENSOR1 = 0,
	THERMAL_SENSOR2,
	THERMAL_SENSOR3,
	THERMAL_SENSOR4,
	THERMAL_SENSOR5,
	THERMAL_SENSOR6,
	THERMAL_SENSOR7,
	THERMAL_SENSOR8,
	THERMAL_SENSOR_MAX
} THERMAL_SENSOR_ID;

/* args for ioctl command */
typedef struct _args_thermal_get_from_id {
	THERMAL_BANK_ID in_bank_id;			 /* [in] bank_id . [0] */
	THERMAL_SENSOR_ID in_sensor_id;		 /* [in] sensor id. [0-7] */
	int out_temperature;				/* [out] temperature. */
} args_thermal_get_from_id;

typedef struct _args_thermal_get {
	THERMAL_BANK_ID out_bank_id;		  /* [out] bank_id . [0] */
	THERMAL_SENSOR_ID out_sensor_id;	  /* [out] sensor id. [0-7] */
	int out_temperature;				 /* [out] max temperature. */
} args_thermal_get;

/* ioctl commands */
#define THERMAL_GET_FROM_ID	_IOR(THERMAL_DEVICE_IOC_TYPE, 1, args_thermal_get_from_id)
#define THERMAL_GET_MAX		_IOR(THERMAL_DEVICE_IOC_TYPE, 2, args_thermal_get)
#define THERMAL_GET_MIN		_IOR(THERMAL_DEVICE_IOC_TYPE, 3, args_thermal_get)

#endif /* _MTK_WRAPPER_DFU_ */
