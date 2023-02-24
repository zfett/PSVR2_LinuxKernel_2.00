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

#ifndef _KD_IMGSENSOR_LOG_H_
#define _KD_IMGSENSOR_LOG_H_

/**
 * @file kd_imgsensor_log.h
 * Header file of sensor kernel driver log define.
 */
#define MyTag "[CameraSensor]"

#define LOG_VRB(format, args...)                                               \
	pr_debug(MyTag "[%s] " format "\n", __func__, ##args)

/* #define SENSOR_DEBUG */
#ifdef SENSOR_DEBUG
#define LOG_DBG(format, args...)                                               \
	pr_err(MyTag "[%s] " format "\n", __func__, ##args)
#else
#define LOG_DBG(format, args...)
#endif

#define LOG_INF(format, args...)                                               \
	pr_err(MyTag "[%s] " format "\n", __func__, ##args)
#define LOG_NOTICE(format, args...)                                            \
	pr_notice(MyTag "[%s] " format "\n", __func__, ##args)
#define LOG_WRN(format, args...)                                               \
	pr_warn(MyTag "[%s] " format "\n", __func__, ##args)
#define LOG_ERR(format, args...)                                               \
	pr_err(MyTag "[%s] " format "\n", __func__, ##args)
#define LOG_AST(format, args...)                                               \
	pr_alert(MyTag "[%s] " format "\n", __func__, ##args)

#endif

