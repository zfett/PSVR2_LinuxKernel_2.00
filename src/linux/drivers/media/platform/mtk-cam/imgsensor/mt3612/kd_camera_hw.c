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
 * @file kd_camera_hw.c
 * Camera sensor hw related function.
 */

#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/clk.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_log.h"

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Sensor reset setting.
 * @param[in]
 *     mtk_cam: Pointer of mtk_camera.
 * @param[in]
 *     val: reset value.
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
int mtkcam_rst_set(struct mtk_camera *mtk_cam, const int val)
{
	int ret = 0;

	/* parameters sanity check */
	if (mtk_cam == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	LOG_DBG("sensorIdx:%d", mtk_cam->sensorIdx);

	switch (mtk_cam->sensorIdx) {
	case SENSOR_IDX_SIDE_LL:
	case SENSOR_IDX_SIDE_LR:
	case SENSOR_IDX_SIDE_RL:
	case SENSOR_IDX_SIDE_RR:
		if (val == VOL_LOW) {
			gpio_direction_output(mtk_cam->sensor_rst, VOL_LOW);
		} else if (val == VOL_HIGH) {
			gpio_direction_output(mtk_cam->sensor_rst, VOL_HIGH);
		} else {
			LOG_ERR("%s: pinctrl err, val:%d", __func__, val);
			ret = -EIO;
		}
		break;
	case SENSOR_IDX_GAZE_L:
	case SENSOR_IDX_GAZE_R:
		if (val == VOL_LOW) {
			gpio_direction_output(mtk_cam->sensor_rst, VOL_LOW);
		} else if (val == VOL_HIGH) {
			gpio_direction_output(mtk_cam->sensor_rst, VOL_HIGH);
		} else {
			LOG_ERR("%s: pinctrl err, val:%d", __func__, val);
			ret = -EIO;
		}
		break;
	default:
		LOG_ERR("%s: sensorIdx err, sensorIdx:%d", __func__,
			mtk_cam->sensorIdx);
		ret = -EINVAL;
		break;
	}

	return ret;
}

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Sensor power on/off setting.
 * @param[in]
 *     mtk_cam: Pointer of mtk_camera.
 * @param[in]
 *     val: power on/off value.
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
int mtkcam_pdn_set(const struct mtk_camera *mtk_cam, const int val)
{
	int ret = 0;

	/* parameters sanity check */
	if (mtk_cam == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	LOG_DBG("sensorIdx:%d", mtk_cam->sensorIdx);

	switch (mtk_cam->sensorIdx) {
	case SENSOR_IDX_SIDE_LL:
	case SENSOR_IDX_SIDE_LR:
	case SENSOR_IDX_SIDE_RL:
	case SENSOR_IDX_SIDE_RR:
	case SENSOR_IDX_GAZE_L:
	case SENSOR_IDX_GAZE_R:
		if (val == VOL_LOW) {
			gpio_direction_output(mtk_cam->sensor_pdn, VOL_LOW);
			mtk_cam->client->addr = mtk_cam->i2c_addr;
		} else if (val == VOL_HIGH) {
			gpio_direction_output(mtk_cam->sensor_pdn, VOL_HIGH);
		} else {
			LOG_ERR("%s: pinctrl err, val:%d", __func__, val);
			ret = -EIO;
		}
		break;
	default:
		LOG_ERR("%s: sensorIdx err, sensorIdx:%d", __func__,
			mtk_cam->sensorIdx);
		ret = -EINVAL;
		break;
	}

	return ret;
}

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Sensor LDO enable/disable setting.
 * @param[in]
 *     mtk_cam: Pointer of mtk_camera.
 * @param[in]
 *     val: LDO enable/disable value.
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
int mtkcam_ldo_set(struct mtk_camera *mtk_cam, const int val)
{
	int ret = 0;
	struct i2c_client *client = NULL;

	/* parameters sanity check */
	if (mtk_cam == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	LOG_DBG("sensorIdx:%d", mtk_cam->sensorIdx);

	client = mtk_cam->client;
	mtk_cam->pctrl = devm_pinctrl_get(&client->dev);
	if (IS_ERR(mtk_cam->pctrl)) {
		dev_err(&client->dev, "cannot find camera pinctrl!");
		ret = PTR_ERR(mtk_cam->pctrl);
	}

	if (val == VOL_HIGH) {
		/* ldo enable */
		mtk_cam->sensor_ldo_enable_pstate =
		pinctrl_lookup_state(mtk_cam->pctrl, "cam_ldo_enable");
		if (IS_ERR(mtk_cam->sensor_ldo_enable_pstate)) {
			ret = PTR_ERR(mtk_cam->sensor_ldo_enable_pstate);
			LOG_ERR("%s:pinctrl err, sensor ldo enable", __func__);
		}
		ret = pinctrl_select_state(mtk_cam->pctrl,
					   mtk_cam->sensor_ldo_enable_pstate);
	} else {
		/* ldo disable */
		mtk_cam->sensor_ldo_disable_pstate =
		pinctrl_lookup_state(mtk_cam->pctrl, "cam_ldo_disable");
		if (IS_ERR(mtk_cam->sensor_ldo_disable_pstate)) {
			ret = PTR_ERR(mtk_cam->sensor_ldo_disable_pstate);
			LOG_ERR("%s:pinctrl err, ensor ldo disable", __func__);
		}
		ret = pinctrl_select_state(mtk_cam->pctrl,
					   mtk_cam->sensor_ldo_disable_pstate);
	}

	devm_pinctrl_put(mtk_cam->pctrl);

	return ret;
}

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Sensor GPIO setting.
 * @param[in]
 *     mtk_cam: Pointer of mtk_camera.
 * @param[in]
 *     pwr_type: GPIO type.
 * @param[in]
 *     val: GPIO value.
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
int mtkcam_gpio_set(struct mtk_camera *mtk_cam,
		    const int pwr_type,
		    const int val)
{
	int ret = 0;

	/* parameters sanity check */
	if (mtk_cam == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	LOG_DBG("pwr_type:%d, val:%d", pwr_type, val);

	switch (pwr_type) {
	case RST:
		ret = mtkcam_rst_set(mtk_cam, val);
		break;
	case PDN:
		ret = mtkcam_pdn_set(mtk_cam, val);
		break;
	case SENSOR_LDO:
		ret = mtkcam_ldo_set(mtk_cam, val);
		break;
	default:
		LOG_ERR("pwr_type:%d is invalid!", pwr_type);
		ret = -EINVAL;
		break;
	};

	return ret;
}

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Sensor power on/off setting.
 * @param[in]
 *     mtk_cam: Pointer of mtk_camera.
 * @param[in]
 *     on: Power on/off.
 * @param[in]
 *     pwInfo: Pointer of SENSOR_PWR_SEQ_INFO.
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
int hwpoweronoff(struct mtk_camera *mtk_cam,
		 const bool on,
		 const struct SENSOR_PWR_SEQ_INFO *pwInfo)
{
	int ret = 0;

	/* parameters sanity check */
	if (mtk_cam == NULL || pwInfo == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	if (pwInfo->power_type == PDN) {
		LOG_DBG("PDN:%d", pwInfo->voltage);
		if (pwInfo->voltage == VOL_HIGH)
			ret = mtkcam_gpio_set(mtk_cam, PDN, VOL_HIGH);
		else
			ret = mtkcam_gpio_set(mtk_cam, PDN, VOL_LOW);

		if (ret)
			LOG_ERR("set PDN failed!");
	} else if (pwInfo->power_type == RST) {
		LOG_DBG("RST %d", pwInfo->voltage);
		if (pwInfo->voltage == VOL_HIGH)
			ret = mtkcam_gpio_set(mtk_cam, RST, VOL_HIGH);
		else
			ret = mtkcam_gpio_set(mtk_cam, RST, VOL_LOW);

		if (ret)
			LOG_ERR("set RST failed!");
	} else if (pwInfo->power_type == SENSOR_MCLK) {
		LOG_DBG("SENSOR_MCLK %d", on);
#ifdef CONFIG_COMMON_CLK_MT3612
		if (on) {
			ret = clk_set_parent(mtk_cam->clk_sel,
					     mtk_cam->pll_D52);
			if (ret)
				return ret;
			ret = clk_prepare_enable(mtk_cam->top_ck);
			if (ret)
				return ret;
		} else {
			clk_disable_unprepare(mtk_cam->top_ck);
		}
#endif
	} else if (pwInfo->power_type == SENSOR_LDO) {
		LOG_DBG("SENSOR_LDO %d", on);
		if (on)
			ret = mtkcam_gpio_set(mtk_cam, SENSOR_LDO, VOL_HIGH);
		else
			ret = mtkcam_gpio_set(mtk_cam, SENSOR_LDO, VOL_LOW);
	} else {
	}

	if (pwInfo->delay > 0) {
#ifdef CONFIG_VIDEO_MEDIATEK_ISP_TINY
		LOG_DBG("sleep:%d", pwInfo->delay);
		usleep_range(pwInfo->delay * 1000, pwInfo->delay * 1000 + 100);
#else
		LOG_DBG("delay:%d", pwInfo->delay);
		mdelay(pwInfo->delay);
#endif
	}

	return ret;
}

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Sensor power on/off function.
 * @param[in]
 *     mtk_cam: Pointer of mtk_camera.
 * @param[in]
 *     on: Power on/off.
 * @param[in]
 *     buf: Pointer of SENSOR_PWR_SEQ_INFO.
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
int kd_cis_module_poweronoff(struct mtk_camera *mtk_cam,
			     const bool on,
			     void *buf)
{
	int ret = 0;
	struct SENSOR_PWR_SEQ_INFO *pwr_info =
				(struct SENSOR_PWR_SEQ_INFO *)buf;

	/* parameters sanity check */
	if (mtk_cam == NULL || pwr_info == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	if ((mtk_cam->enable_driver) && (!fpga_mode)) {
		if (on) {
			LOG_DBG("PowerOn: sensorIdx:%d", mtk_cam->sensorIdx);
			ret = hwpoweronoff(mtk_cam, true, pwr_info);
			if (ret)
				return ret;
		} else {
			LOG_DBG("PowerOff: sensorIdx:%d", mtk_cam->sensorIdx);
			ret = hwpoweronoff(mtk_cam, false, pwr_info);
			if (ret)
				return ret;
		}
	}

	return ret;
}
EXPORT_SYMBOL(kd_cis_module_poweronoff);

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Write sensor register via I2C.
 * @param[in]
 *     client: I2C client.
 * @param[in]
 *     slave_id: Slave ID.
 * @param[in]
 *     addr: Address.
 * @param[in]
 *     para: Data.
 * @return
 *     0, function success.\n
 *     -EFAULT, function fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int fpga_write_cmos_sensor(struct i2c_client *client,
				  unsigned short slave_id,
				  unsigned char addr,
				  unsigned char para)
{
	unsigned char out_buff[2];
	unsigned short tmp_addr;

	/* parameters sanity check */
	if (client == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	slave_id = (slave_id) & 0xfe;
	out_buff[0] = addr;
	out_buff[1] = para;

	tmp_addr = client->addr;
	client->addr = (slave_id >> 1);

	if (i2c_write_data(client, (u8 *)out_buff, (u16)sizeof(out_buff)) != 0)
		LOG_ERR("fpga_write_cmos_sensor");

	client->addr = tmp_addr;

	return 0;
}

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     FPGA D-PHY DTB initialization.
 * @param[in]
 *     client: I2C client.
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
int fpga_dtb_init(struct i2c_client *client)
{
	/* parameters sanity check */
	if (client == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	fpga_write_cmos_sensor(client, 0x40, 0x4e, 0x01);
	fpga_write_cmos_sensor(client, 0x40, 0x2e, 0x82);
	fpga_write_cmos_sensor(client, 0x40, 0x31, 0x03);
	mdelay(10);
	fpga_write_cmos_sensor(client, 0x40, 0x30, 0x01);
	mdelay(10);
	fpga_write_cmos_sensor(client, 0x40, 0x4b, 0x01);
	fpga_write_cmos_sensor(client, 0x70, 0x80, 0x02);
	fpga_write_cmos_sensor(client, 0x70, 0x80, 0x02);
	fpga_write_cmos_sensor(client, 0x80, 0x24, 0x00);
	fpga_write_cmos_sensor(client, 0x80, 0x24, 0x01);
	fpga_write_cmos_sensor(client, 0x80, 0x20, 0x03);
	fpga_write_cmos_sensor(client, 0x80, 0x22, 0x03);
	mdelay(10);
	fpga_write_cmos_sensor(client, 0x80, 0x00, 0x01);
	fpga_write_cmos_sensor(client, 0x80, 0x04, 0x01);
	fpga_write_cmos_sensor(client, 0x80, 0x08, 0x01);
	fpga_write_cmos_sensor(client, 0x80, 0x0c, 0x01);
	fpga_write_cmos_sensor(client, 0x80, 0x10, 0x01);
	fpga_write_cmos_sensor(client, 0x80, 0x21, 0xe0);
	mdelay(1);
	fpga_write_cmos_sensor(client, 0x80, 0x00, 0x09);
	fpga_write_cmos_sensor(client, 0x80, 0x04, 0x09);
	fpga_write_cmos_sensor(client, 0x80, 0x08, 0x09);
	fpga_write_cmos_sensor(client, 0x80, 0x0c, 0x09);
	fpga_write_cmos_sensor(client, 0x80, 0x10, 0x09);

	/* slave 0x70 , bank0 */
	fpga_write_cmos_sensor(client, 0x70, 0x80, 0x00);
	fpga_write_cmos_sensor(client, 0x70, 0x11, 0x80);
	fpga_write_cmos_sensor(client, 0x70, 0x10, 0x01);

	/* slave 0x70 , bank1 */
	fpga_write_cmos_sensor(client, 0x70, 0x80, 0x01);
	fpga_write_cmos_sensor(client, 0x70, 0x20, 0x60);
	fpga_write_cmos_sensor(client, 0x70, 0x28, 0x00);
	fpga_write_cmos_sensor(client, 0x70, 0x29, 0x05);
	fpga_write_cmos_sensor(client, 0x70, 0x20, 0x7f);
	fpga_write_cmos_sensor(client, 0x70, 0x22, 0x89);
	fpga_write_cmos_sensor(client, 0x70, 0x30, 0xff);
	fpga_write_cmos_sensor(client, 0x70, 0x31, 0xff);

	/*Force enable data/clk lane */
	fpga_write_cmos_sensor(client, 0x70, 0x58, 0x1f);

	/*Debug */
	fpga_write_cmos_sensor(client, 0x70, 0x38, 0x28);
	fpga_write_cmos_sensor(client, 0x70, 0x38, 0x29);
	fpga_write_cmos_sensor(client, 0x70, 0x38, 0x12);
	mdelay(10);
	fpga_write_cmos_sensor(client, 0x70, 0x38, 0x12);

	/*GPIO62/63 enable */
	fpga_write_cmos_sensor(client, 0x30, 0x00, 0x40);
	fpga_write_cmos_sensor(client, 0x80, 0x23, 0x10);

	return 0;
}
EXPORT_SYMBOL(fpga_dtb_init);
