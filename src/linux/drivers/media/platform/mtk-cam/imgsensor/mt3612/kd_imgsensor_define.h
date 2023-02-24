/*
 * Copyright (C) 2015 MediaTek Inc.
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
 * @file kd_imgsensor_define.h
 * Header file of camera sensor definition.
 */

#ifndef _KD_IMGSENSOR_DATA_H
#define _KD_IMGSENSOR_DATA_H

#include <linux/of_gpio.h>
#include <linux/gpio.h>

/******************************************************************************
*
*******************************************************************************/
/*  */
#define MAX_NUM_OF_SUPPORT_SENSOR 6
#define SENSOR_FEATURE_START 3000

/** @ingroup IP_group_sensor_internal_enum
 * @brief Enum for camera sensor index.
 */
enum SENSOR_IDX {
	SENSOR_IDX_MIN_NUM = 0,
	SENSOR_IDX_SIDE_MIN = SENSOR_IDX_MIN_NUM,
	SENSOR_IDX_SIDE_LL = SENSOR_IDX_SIDE_MIN,
	SENSOR_IDX_SIDE_LR,
	SENSOR_IDX_SIDE_RL,
	SENSOR_IDX_SIDE_RR,
	SENSOR_IDX_GAZE_L,
	SENSOR_IDX_GAZE_R,
	SENSOR_IDX_GAZE_MAX,
	SENSOR_IDX_MAX_NUM = SENSOR_IDX_GAZE_MAX,
	SENSOR_IDX_NONE,
};

/** @ingroup IP_group_sensor_internal_enum
 * @brief Enum for camera sensor feature.
 */
enum ACDK_SENSOR_FEATURE_ENUM {
	SENSOR_FEATURE_BEGIN = SENSOR_FEATURE_START,
	SENSOR_FEATURE_SET_REGISTER,
	SENSOR_FEATURE_SET_REGISTER_TABLE_XFER,
	SENSOR_FEATURE_GET_REGISTER,
	SENSOR_FEATURE_SET_CAMERA_SYNC,
	SENSOR_FEATURE_SET_REGISTER_TABLE_XFER_EX,
	SENSOR_FEATURE_MAX
};

/** @ingroup IP_group_sensor_internal_enum
 * @brief Enum for camera sensor sync type.
 */
enum SENSOR_SYNC_TYPE_ENUM {
	DSI_SYNC = 0,
	XVS_SYNC_MST,
	XVS_SYNC_SLV,
	ASYNC
};

/** @ingroup IP_group_sensor_internal_struct
 * @brief Struct for camera sensor register info.
 */
struct ACDK_SENSOR_REG_INFO_STRUCT {
	unsigned short RegAddr;
	unsigned short RegData;
};

/** @ingroup IP_group_sensor_internal_struct
 * @brief Struct for camera sensor streaming setting.
 */
struct ACDK_SENSOR_STREAMING_SET_STRUCT {
	unsigned short TestPat_Mode;
	unsigned char Streaming_En;
};

/** @ingroup IP_group_sensor_internal_struct
 * @brief Struct for camera sensor sync setting.
 */
struct ACDK_SENSOR_SYNC_SET_STRUCT {
	enum SENSOR_SYNC_TYPE_ENUM SyncMode;
};

/******************************************************************************
*
*******************************************************************************/
/** @ingroup IP_group_sensor_internal_struct
 * @brief Struct for camera sensor feature control.
 */
struct ACDK_SENSOR_FEATURECONTROL_STRUCT {
	enum ACDK_SENSOR_FEATURE_ENUM FeatureId;
	unsigned char *pFeaturePara;
	unsigned int *pFeatureParaLen;
};

/** @ingroup IP_group_sensor_internal_enum
 * @brief Enum for camera sensor power type.
 */
enum PWR_TYPE_ENUM {
	NONE,
	PDN,
	RST,
	SENSOR_MCLK,
	SENSOR_LDO
};

/** @ingroup IP_group_sensor_internal_enum
 * @brief Enum for camera sensor voltage.
 */
enum VOLTAGE_ENUM {
	VOL_LOW = 0,
	VOL_HIGH = 1
};

/** @ingroup IP_group_sensor_internal_struct
 * @brief Struct for camera sensor power information.
 */
struct SENSOR_PWR_SEQ_INFO {
	enum PWR_TYPE_ENUM power_type;
	enum VOLTAGE_ENUM voltage;
	unsigned int delay;
};

/** @ingroup IP_group_sensor_internal_struct
 * @brief Struct for mtk_camera.
 */
struct mtk_camera {
	struct i2c_client *client;
	char name[32];
	enum SENSOR_IDX sensorIdx;

	/** Sensor data */
	char sensor_name[32];
	bool enable_driver;
	struct SENSOR_FUNCTION_STRUCT *invoke_sensorfunc;

	/** SENSOR PRIVATE STRUCT FOR VARIABLES */
	unsigned short i2c_addr;

	/** char dev data */
	dev_t devno;
	struct cdev i_cdev;

	/** class dev data */
	struct class *sensor_class;

	/** Pin control for power on/off */
	int sensor_rst;
	int sensor_pdn;

	/** Pin control */
	struct pinctrl *pctrl;
	struct pinctrl_state *dsi_sync_pstate;
	struct pinctrl_state *pair_sync_pstate;
	struct pinctrl_state *sensor_ldo_disable_pstate;
	struct pinctrl_state *sensor_ldo_enable_pstate;

#ifdef CONFIG_COMMON_CLK_MT3612
	/** point to top clk select struct */
	struct clk *clk_sel;
	/** point to top clk gen struct */
	struct clk *top_ck;
	/** point to top clk 26M struct */
	struct clk *clk_26M;
	/** point to top clk D52 struct */
	struct clk *pll_D52;
#endif

#if defined(CONFIG_DEBUG_FS)
	struct dentry *debugfs;
#endif
};

/******************************************************************************
*
*******************************************************************************/
struct SENSOR_FUNCTION_STRUCT {
	unsigned int (*SensorFeatureControl)(
	    struct mtk_camera *mtk_cam,
	    enum ACDK_SENSOR_FEATURE_ENUM FeatureId,
	    unsigned char *pFeaturePara,
	    unsigned int *pFeatureParaLen);
};

struct ACDK_KD_SENSOR_INIT_FUNCTION_STRUCT {
	unsigned char drvname[32];
	unsigned int (*SensorInit)(struct SENSOR_FUNCTION_STRUCT **pfFunc);
};

struct SENSOR_DRIVER_INDEX_STRUCT {
	unsigned int drvIndex;
};

#endif /* _KD_IMGSENSOR_DATA_H */
