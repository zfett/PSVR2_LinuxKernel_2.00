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
 * @file kd_sensorfunc.c
 * sensor driver function.
 */

#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/mutex.h>

#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_log.h"
#include "kd_sensorfunc.h"

/* Mutex taken to protect cam_pinctrlt */
DEFINE_MUTEX(cam_pinctrl_mutex);

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Read sensor register.
 * @param[in]
 *     client: I2C client.
 * @param[in]
 *     addr: I2C address.
 * @param[out]
 *     data: I2C returned data.
 * @return
 *     0, function success.\n
 *     EFAULT, function fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int read_cmos_sensor(const struct i2c_client *client,
			    const unsigned short addr,
			    unsigned char *data)
{
	int ret = 0;
	char pusendcmd[2] = {(char)(addr >> 8), (char)(addr & 0xff)};

	/* parameters sanity check */
	if (client == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	ret = i2c_read_data(client, pusendcmd, 2, data, 1);

	return ret;
}

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Write sensor register.
 * @param[in]
 *     client: I2C client.
 * @param[in]
 *     addr: I2C address.
 * @param[in]
 *     para: I2C data, 2 bytes.
 * @return
 *     0, function success.\n
 *     EFAULT, function fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int write_cmos_sensor_16(const struct i2c_client *client,
				const unsigned short addr,
				const unsigned short para)
{
	int ret = 0;
	char pusendcmd[4] = {(char)(addr >> 8), (char)(addr & 0xff),
			     (char)(para >> 8), (char)(para & 0xff)};

	/* parameters sanity check */
	if (client == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	ret = i2c_write_data(client, pusendcmd, 4);

	return ret;
}

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Write sensor register.
 * @param[in]
 *     client: I2C client.
 * @param[in]
 *     addr: I2C address.
 * @param[in]
 *     para: I2C data, 1 byte.
 * @return
 *     0, function success.\n
 *     EFAULT, function fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int write_cmos_sensor(const struct i2c_client *client,
			     const unsigned short addr,
			     const unsigned char para)
{
	int ret = 0;
	char pusendcmd[4] = {(char)(addr >> 8), (char)(addr & 0xff),
			     (char)(para & 0xff)};

	/* parameters sanity check */
	if (client == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	ret = i2c_write_data(client, pusendcmd, 3);

	return ret;
}

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Write sensor register by table transfer.
 * @param[in]
 *     client: I2C client.
 * @param[in]
 *     pTable: Pointer of sensor information table
 * @return
 *     0, function success.
 *     EFAULT, function fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int write_cmos_sensor_table(const struct i2c_client *client,
				   const unsigned short *pTable)
{
	int ret = 0;
	int i = 0;

	/* parameters sanity check */
	if (client == NULL || pTable == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	while ((*(pTable + i * 2) != 0xffff) ||
			(*(pTable + i * 2 + 1) != 0xff)) {
		ret = write_cmos_sensor(client,
			*(pTable + i * 2), *(pTable + i * 2 + 1));
		LOG_DBG("pTable[%d][0]:0x%x, pTable[%d][1]:0x%x",
			i, *(pTable + i * 2), i, *(pTable + i * 2 + 1));
		if (ret != 0) {
			LOG_ERR("fail, pTable[%d][0]:0x%x, pTable[%d][1]:0x%x",
				i, *(pTable + i * 2), i, *(pTable + i * 2 + 1));
			break;
		}
		i++;
	}

	return ret;
}

static int write_cmos_sensor_table_ex(const struct i2c_client *client,
				   const unsigned short *pTable)
{
	int ret = 0;
	int i = 0, j = 0;
	unsigned char buff[32] = {};

	/* parameters sanity check */
	if (client == NULL || pTable == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	for (;; i ++) {
		unsigned short addr = *(pTable + i * 2);
		unsigned char  data = *(pTable + i * 2 + 1);

		if (addr == 0xffff && data == 0xff) {
			break; /* End point of data */
		}

		if (j == 0) {
			buff[j ++] = (unsigned char)(addr >> 8);
			buff[j ++] = (unsigned char)(addr & 0xff);
		}
		buff[j ++] = data;

		if (addr + 1 != *(pTable + (i + 1) * 2) || j == 16) {
			int k;
			for (k = 0; k < j; k ++) {
				LOG_DBG("buff[%d]: 0x%02x", k, buff[k]);
			}
			ret = i2c_write_data(client, buff, j);
			if (ret != 0) {
				for (k = 0; k < j; k ++) {
					LOG_ERR("fail, buff[%d]: 0x%02x", k, buff[k]);
				}
				break;
			}
			j = 0;
		}
	}

	return ret;
}

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Sensor sync configuration.
 * @param[in]
 *     mtk_cam: mtk camera info.
 * @param[in]
 *     sensor_sync: sensor sync info struct.
 * @return
 *     0, function success.\n
 *     Others, function fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int sensor_sync_config(struct mtk_camera *mtk_cam,
			const struct ACDK_SENSOR_SYNC_SET_STRUCT *sensor_sync)
{
	int ret = 0;
	struct i2c_client *client = NULL;

	/* parameters sanity check */
	if (mtk_cam == NULL || sensor_sync == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	LOG_DBG("SyncMode:0x%x", sensor_sync->SyncMode);

	mutex_lock(&cam_pinctrl_mutex);
	client = mtk_cam->client;
	mtk_cam->pctrl = devm_pinctrl_get(&client->dev);
	if (IS_ERR(mtk_cam->pctrl)) {
		dev_err(&client->dev, "cannot find camera pinctrl!");
		ret = PTR_ERR(mtk_cam->pctrl);
	}

	if (sensor_sync->SyncMode == DSI_SYNC) {
		/* dsi sync mode */
		mtk_cam->dsi_sync_pstate =
		pinctrl_lookup_state(mtk_cam->pctrl, "cam_dsi_sync");
		if (IS_ERR(mtk_cam->dsi_sync_pstate)) {
			ret = PTR_ERR(mtk_cam->dsi_sync_pstate);
			LOG_ERR("%s:pinctrl err, dsi_sync", __func__);
		}
		ret = pinctrl_select_state(mtk_cam->pctrl,
					   mtk_cam->dsi_sync_pstate);
	} else {
		/* pair sync mode */
		mtk_cam->pair_sync_pstate =
		pinctrl_lookup_state(mtk_cam->pctrl, "cam_pair_sync");
		if (IS_ERR(mtk_cam->pair_sync_pstate)) {
			ret = PTR_ERR(mtk_cam->pair_sync_pstate);
			LOG_ERR("%s:pinctrl err, pair_sync", __func__);
		}
		ret = pinctrl_select_state(mtk_cam->pctrl,
					   mtk_cam->pair_sync_pstate);
	}

	devm_pinctrl_put(mtk_cam->pctrl);
	mutex_unlock(&cam_pinctrl_mutex);

	return ret;
}

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Setup sensor feature.
 * @param[in]
 *     mtk_cam: Pointer of mtk_camera.
 * @param[in]
 *     feature_id: Feature ID.
 * @param[in]
 *     feature_para: Feature parameter.
 * @param[in]
 *     feature_para_len: Length of feature parameter.
 * @return
 *     0, function success.
 *     Others, function fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static unsigned int
feature_control(struct mtk_camera *mtk_cam,
		enum ACDK_SENSOR_FEATURE_ENUM feature_id,
		unsigned char *feature_para,
		unsigned int *feature_para_len)
{
	struct ACDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data =
	    (struct ACDK_SENSOR_REG_INFO_STRUCT *)feature_para;
	struct ACDK_SENSOR_SYNC_SET_STRUCT *sensor_sync =
	    (struct ACDK_SENSOR_SYNC_SET_STRUCT *)feature_para;
	unsigned short *sensor_table = (unsigned short *)feature_para;
	unsigned char reg_data = 0;
	int ret = 0;

	/* parameters sanity check */
	if (mtk_cam == NULL ||
		feature_para == NULL ||
		feature_para_len == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	LOG_DBG("feature_id:%d, feature_para:0x%x, feature_para_len:0x%x",
		feature_id, *feature_para, *feature_para_len);

	switch (feature_id) {
	case SENSOR_FEATURE_SET_CAMERA_SYNC:
		LOG_DBG("SENSOR_FEATURE_SET_CAMERA_SYNC");
		sensor_sync_config(mtk_cam, sensor_sync);
		break;
	case SENSOR_FEATURE_SET_REGISTER:
		LOG_DBG("SENSOR_FEATURE_SET_REGISTER");
		if ((sensor_reg_data->RegData >> 8) > 0)
			ret = write_cmos_sensor_16(mtk_cam->client,
						   sensor_reg_data->RegAddr,
						   sensor_reg_data->RegData);
		else
			ret = write_cmos_sensor(mtk_cam->client,
						sensor_reg_data->RegAddr,
						sensor_reg_data->RegData);
		break;
	case SENSOR_FEATURE_SET_REGISTER_TABLE_XFER:
		LOG_DBG("SENSOR_FEATURE_SET_REGISTER_TABLE_XFER");
		ret = write_cmos_sensor_table(mtk_cam->client, sensor_table);
		break;
	case SENSOR_FEATURE_GET_REGISTER:
		LOG_DBG("SENSOR_FEATURE_GET_REGISTER");
		ret = read_cmos_sensor(mtk_cam->client,
				       sensor_reg_data->RegAddr,
				       &reg_data);
		if (ret == 0)
			sensor_reg_data->RegData = reg_data;
		break;
	case SENSOR_FEATURE_SET_REGISTER_TABLE_XFER_EX:
		LOG_DBG("SENSOR_FEATURE_SET_REGISTER_TABLE_XFER_EX");
		ret = write_cmos_sensor_table_ex(mtk_cam->client, sensor_table);
		break;
	default:
		break;
	}

	return ret;
}

static struct SENSOR_FUNCTION_STRUCT sensor_func = {feature_control};

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Sensor functions initialization.
 * @param[out]
 *     pfFunc: Pointer of SENSOR_FUNCTION_STRUCT.
 * @return
 *     0, function success.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
unsigned int sensor_func_init(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	if (pfFunc != NULL)
		*pfFunc = &sensor_func;
	return 0;
}

