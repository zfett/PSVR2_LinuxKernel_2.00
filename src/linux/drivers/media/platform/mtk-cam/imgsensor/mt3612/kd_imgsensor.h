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
 * @file kd_imgsensor.h
 * Header file of camera sensor definition.
 */

#ifndef _KD_IMGSENSOR_H
#define _KD_IMGSENSOR_H

#include <linux/ioctl.h>

#ifndef ASSERT
#define ASSERT(expr) WARN_ON(!(expr))
#endif

#define IMGSENSORMAGIC 'i'
/* IOCTRL(inode * ,file * ,cmd ,arg ) */
/* S means "set through a ptr" */
/* T means "tell by a arg value" */
/* G means "get by a ptr" */
/* Q means "get by return a value" */
/* X means "switch G and S atomically" */
/* H means "switch T and Q atomically" */

/******************************************************************************
*
*******************************************************************************/

/** @ingroup IP_group_sensor_internal_def */
/** sensorFeatureControl */
#define KDIMGSENSORIOC_X_FEATURECONCTROL                                       \
	_IOWR(IMGSENSORMAGIC, 15, struct ACDK_SENSOR_FEATURECONTROL_STRUCT)
/** Set sensor driver */
#define KDIMGSENSORIOC_X_SET_DRIVER                                            \
	_IOWR(IMGSENSORMAGIC, 35, struct SENSOR_DRIVER_INDEX_STRUCT)
/** Set sensor power on */
#define KDIMGSENSORIOC_X_SET_POWER_ON                                          \
	_IOW(IMGSENSORMAGIC, 40, struct SENSOR_PWR_SEQ_INFO)
/** Set sensor power off */
#define KDIMGSENSORIOC_X_SET_POWER_OFF                                         \
	_IOW(IMGSENSORMAGIC, 45, struct SENSOR_PWR_SEQ_INFO)
/** Setup FPGA DPHY chip */
#define KDIMGSENSORIOC_T_SET_A60919                                            \
	_IO(IMGSENSORMAGIC, 90)

/******************************************************************************
*
*******************************************************************************/
/* CAMERA DRIVER NAME */
#define CAMERA_HW_DEVNAME "kd_camera_hw"
#define CAMERA_HW_DEVNAME_SIDELL "kd_camera_hw_sidell"
#define CAMERA_HW_DEVNAME_SIDELR "kd_camera_hw_sidelr"
#define CAMERA_HW_DEVNAME_SIDERL "kd_camera_hw_siderl"
#define CAMERA_HW_DEVNAME_SIDERR "kd_camera_hw_siderr"
#define CAMERA_HW_DEVNAME_GAZEL "kd_camera_hw_gazel"
#define CAMERA_HW_DEVNAME_GAZER "kd_camera_hw_gazer"

#endif /* _KD_IMGSENSOR_H */
