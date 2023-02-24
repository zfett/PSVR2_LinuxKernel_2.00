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
 * @file kd_sensorlist.h
 * This is header file of kd_sensorlist.c
 */

extern int kd_cis_module_poweronoff(const struct mtk_camera *mtk_cam,
				    const bool on, void *buf);

extern int fpga_dtb_init(const struct i2c_client *client);

unsigned int sensor_func_init(struct SENSOR_FUNCTION_STRUCT **pfFunc);

struct ACDK_KD_SENSOR_INIT_FUNCTION_STRUCT
	kd_sensorlist[MAX_NUM_OF_SUPPORT_SENSOR + 1] = {
	/* Side LL: sensorIdx = 0 */
	{"SIDE-LL", sensor_func_init},
	/* Side LR: sensorIdx = 1 */
	{"SIDE-LR", sensor_func_init},
	/* Side RL: sensorIdx = 2 */
	{"SIDE-RL", sensor_func_init},
	/* Side RR: sensorIdx = 3 */
	{"SIDE-RR", sensor_func_init},
	/* Gaze L: sensorIdx = 4 */
	{"GAZE-L", sensor_func_init},
	/* Gaze R: sensorIdx = 5 */
	{"GAZE-R", sensor_func_init},
	/* ADD sensor driver before this line */
	{{0}, NULL}, /*end of list*/
};

