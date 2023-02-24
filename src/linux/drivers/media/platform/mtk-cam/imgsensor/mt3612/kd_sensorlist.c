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
 * @file kd_sensorlist.c
 * This is adapter layer of sensor driver to\n
 * communicate with char device driver and I2C device driver.\n
 */

/**
 * @defgroup IP_group_sensor CAMERA_SENSOR
 *     This is camera sensor kernel driver.\n
 *
 *   @{
 *       @defgroup IP_group_sensor_external EXTERNAL
 *         The external API document for camera sensor. \n
 *
 *         @{
 *            @defgroup IP_group_sensor_external_function 1.function
 *              This is camera sensor external API.
 *            @defgroup IP_group_sensor_external_struct 2.structure
 *              This is camera sensor external structure.
 *            @defgroup IP_group_sensor_external_typedef 3.typedef
 *              This is camera sensor external typedef.
 *            @defgroup IP_group_sensor_external_enum 4.enumeration
 *              This is camera sensor external enumeration.
 *            @defgroup IP_group_sensor_external_def 5.define
 *              This is camera sensor external define.
 *         @}
 *
 *       @defgroup IP_group_sensor_internal INTERNAL
 *         The internal API document for camera sensor. \n
 *
 *         @{
 *            @defgroup IP_group_sensor_internal_function 1.function
 *              Internal function used for camera sensor.
 *            @defgroup IP_group_sensor_internal_struct 2.structure
 *              Internal structure used for camera sensor.
 *            @defgroup IP_group_sensor_internal_typedef 3.typedef
 *              Internal typedef used for camera sensor.
 *            @defgroup IP_group_sensor_internal_enum 4.enumeration
 *              Internal enumeration used for camera sensor.
 *            @defgroup IP_group_sensor_internal_def 5.define
 *              Internal define used for camera sensor.
 *         @}
 *     @}
 */

#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/clk.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_log.h"
#include "kd_sensorlist.h"

#include <dt-bindings/camera.h>

const char mtk_camera_name[MTK_CAMERA_MAX_NUM][32] = {
	{"kd_camera_side_ll"},
	{"kd_camera_side_lr"},
	{"kd_camera_side_rl"},
	{"kd_camera_side_rr"},
	{"kd_camera_gaze_l"},
	{"kd_camera_gaze_r"}
};

static dev_t camera_hw_devno;
#if defined(CONFIG_DEBUG_FS)
struct dentry *g_debugdir;
#endif
u32 fpga_mode;

#define FEATURE_CONTROL_MAX_DATA_SIZE 128000

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Read sensor register via I2C.
 * @param[in]
 *     client: I2C client.
 * @param[in]
 *     send_data: Pointer of I2C address.
 * @param[in]
 *     send_size: Pointer of I2C expected data size.
 * @param[out]
 *     recv_data: Pointer of I2C received data.
 * @param[in]
 *     recv_size: Pointer of I2C received data size.
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
int i2c_read_data(const struct i2c_client *client,
		  const u8 *send_data, const u16 send_size,
		  u8 *recv_data, u16 recv_size)
{
	int ret = 0;

	/* parameters sanity check */
	if (client == NULL || send_data == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	ret = i2c_master_send(client, send_data, send_size);
	if (ret != send_size) {
		LOG_ERR("I2C send failed!!, addr:0x%x", send_data[0]);
		return -EIO;
	}

	ret = i2c_master_recv(client, (char *)recv_data, recv_size);
	if (ret != recv_size) {
		LOG_ERR("I2C read failed!!");
		return -EIO;
	}

	return 0;
}

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Write sensor register via I2C.
 * @param[in]
 *     client: I2C client.
 * @param[in]
 *     data: Pointer of I2C data.
 * @param[in]
 *     count: data size.
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
int i2c_write_data(const struct i2c_client *client,
		   const u8 *data,
		   const u16 count)
{
	int ret = 0;
	int retry = 3;

	/* parameters sanity check */
	if (client == NULL || data == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	do {
		ret = i2c_master_send(client, data, count);

		if (ret != count) {
			LOG_ERR(
				"I2C send failed!!, addr:0x%x, data[0]:0x%x, data[1]:0x%x",
				client->addr, data[0], data[1]);
			ret = -EIO;
		} else {
			ret = 0;
			break;
		}
		udelay(50);
	} while ((retry--) > 0);

	return ret;
}

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Get sensor init. function list pointer.
 * @param[out]
 *     sensorlist: Pointer of sensor init. function list.
 * @return
 *     0, function success.\n
 *     1, function fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static unsigned int
kd_get_sensor_initfunclist(struct ACDK_KD_SENSOR_INIT_FUNCTION_STRUCT
			   **sensorlist)
{
	if (sensorlist == NULL) {
		LOG_ERR("NULL sensorlist");
		return 1;
	}
	*sensorlist = kd_sensorlist;
	return 0;
}

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Link user space and kernel space sensor driver
 * @param[out]
 *     mtk_cam: Pointer of mtk_camera.
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
static int kd_setdriver(struct mtk_camera *mtk_cam)
{
	struct ACDK_KD_SENSOR_INIT_FUNCTION_STRUCT *sensorlist = NULL;

	/* parameters sanity check */
	if (mtk_cam == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	if (kd_get_sensor_initfunclist(&sensorlist) != 0) {
		LOG_ERR("kd_get_sensor_initfunclist()");
		return -EIO;
	}

	mtk_cam->enable_driver = false;
	if (mtk_cam->sensorIdx < MAX_NUM_OF_SUPPORT_SENSOR) {
		if (sensorlist[mtk_cam->sensorIdx].SensorInit == NULL) {
			LOG_ERR("NULL SensorInit");
			return -EFAULT;
		}
		sensorlist[mtk_cam->sensorIdx].SensorInit(
			&mtk_cam->invoke_sensorfunc);
		if (mtk_cam->invoke_sensorfunc == NULL) {
			LOG_ERR("NULL invoke_sensorfunc");
			return -EFAULT;
		}
		mtk_cam->enable_driver = true;
		strlcpy(mtk_cam->sensor_name,
			sensorlist[mtk_cam->sensorIdx].drvname,
			sizeof(mtk_cam->sensor_name));

		return 0;
	}
	return -EIO;
}

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Access sensor via ACDK_SENSOR_FEATURECONTROL_STRUCT info.
 * from user space.
 * @param[in]
 *     mtk_cam: Pointer of mtk_camera.
 * @param[out]
 *     buf: Pointer of ACDK_SENSOR_FEATURECONTROL_STRUCT.
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
static int adopt_camera_hw_featurecontrol(struct mtk_camera *mtk_cam, void *buf)
{
	struct SENSOR_FUNCTION_STRUCT *sensorfunc = NULL;
	struct ACDK_SENSOR_FEATURECONTROL_STRUCT *feature_ctrl =
	    (struct ACDK_SENSOR_FEATURECONTROL_STRUCT *)buf;
	unsigned int para_len = 0;
	void *feature_para = NULL;

	/* parameters sanity check */
	if (mtk_cam == NULL ||
	    feature_ctrl == NULL ||
	    feature_ctrl->pFeaturePara == NULL ||
	    feature_ctrl->pFeatureParaLen == NULL) {
		LOG_ERR("NULL arg.");
		return -EFAULT;
	}

	sensorfunc = mtk_cam->invoke_sensorfunc;

	/* copy feature_ctrl->pFeatureParaLen from user */
	if (copy_from_user((void *)&para_len,
			   (void *)feature_ctrl->pFeatureParaLen,
			   sizeof(unsigned int))) {
		LOG_ERR("ioctl copy from user failed");
		return -EFAULT;
	}

	/* check para_len size */
	if (para_len > FEATURE_CONTROL_MAX_DATA_SIZE) {
		LOG_ERR("exceed data size limitation");
		return -EFAULT;
	}

	/* feature_para memory allocate and initialize */
	feature_para = kmalloc(para_len, GFP_KERNEL);
	if (feature_para == NULL) {
		LOG_ERR("ioctl allocate memory failed");
		return -ENOMEM;
	}
	memset(feature_para, 0x0, para_len);

	/* copy feature_ctrl->pFeaturePara from user */
	switch (feature_ctrl->FeatureId) {
	case SENSOR_FEATURE_SET_REGISTER:
	case SENSOR_FEATURE_SET_REGISTER_TABLE_XFER:
	case SENSOR_FEATURE_GET_REGISTER:
	case SENSOR_FEATURE_SET_CAMERA_SYNC:
	case SENSOR_FEATURE_SET_REGISTER_TABLE_XFER_EX:
		if (copy_from_user((void *)feature_para,
				   (void *)feature_ctrl->pFeaturePara,
				   para_len)) {
			kfree(feature_para);
			LOG_ERR("[feature_para]ioctl copy from user failed");
			return -EFAULT;
		}
		break;
	/* copy to user */
	default:
		break;
	}

	/* in case that some structure are passed from user sapce by ptr */
	if (sensorfunc->SensorFeatureControl(
		  mtk_cam, feature_ctrl->FeatureId,
		  (unsigned char *)feature_para,
		  (unsigned int *)&para_len)) {
		kfree(feature_para);
		LOG_ERR("[SensorFeatureControl]failed");
		return -EFAULT;
	}

	/* copy to user */
	switch (feature_ctrl->FeatureId) {
	case SENSOR_FEATURE_GET_REGISTER:
		if (copy_to_user((void __user *)feature_ctrl->pFeaturePara,
				 (void *)feature_para, para_len)) {
			kfree(feature_para);
			LOG_ERR("[pSensorRegData]ioctl copy to user failed");
			return -EFAULT;
		}
		break;
	default:
	/* do nothing */
		break;
	}

	kfree(feature_para);
	if (copy_to_user((void __user *)feature_ctrl->pFeatureParaLen,
			 (void *)&para_len, sizeof(unsigned int))) {
		LOG_ERR("[para_len]ioctl copy to user failed");
		return -EFAULT;
	}

	return 0;
}

#ifdef CONFIG_COMMON_CLK_MT3612
/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Get sensor CCF clock.
 * @param[in]
 *     mtk_cam: Pointer of mtk_camera.
 * @param[in]
 *     dev: Pointer of device.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static inline void Get_ccf_clk(struct mtk_camera *mtk_cam, struct device *dev)
{
	if (dev == NULL || mtk_cam == NULL) {
		LOG_ERR("class_dev or mtk_cam is null");
		return;
	}

	mtk_cam->clk_sel = devm_clk_get(dev, "senm_sel");
	if (IS_ERR(mtk_cam->clk_sel)) {
		dev_err(dev, "Failed to get top clock sel\n");
		return;
	}

	mtk_cam->top_ck = devm_clk_get(dev, "senm_cg");
	if (IS_ERR(mtk_cam->top_ck)) {
		dev_err(dev, "Failed to get top clock gen\n");
		return;
	}

	mtk_cam->clk_26M = devm_clk_get(dev, "clk26m");
	if (IS_ERR(mtk_cam->clk_26M)) {
		dev_err(dev, "Failed to get clock 26M\n");
		return;
	}

	mtk_cam->pll_D52 = devm_clk_get(dev, "univpll_d52");
	if (IS_ERR(mtk_cam->pll_D52)) {
		dev_err(dev, "Failed to get PLL D52\n");
		return;
	}
}
#endif

#ifndef CONFIG_VIDEO_MEDIATEK_ISP_TINY
/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Initialize FPGA camera D-PHY DTB.
 * @param[in]
 *     mtk_cam: Pointer of mtk_camera.
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
static int adopt_camera_hw_set_dtb(const struct mtk_camera *mtk_cam)
{
	int ret = 0;

	/* parameters sanity check */
	if (mtk_cam == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	if (fpga_mode)
		ret = fpga_dtb_init(mtk_cam->client);

	return ret;
}
#endif

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     camera_hw_open.
 * @param[in]
 *     inode: Pointer of inode.
 * @param[in]
 *     file: Pointer of file.
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
static int camera_hw_open(struct inode *inode, struct file *file)
{
	struct mtk_camera *mtk_cam = NULL;

	/* parameters sanity check */
	if (inode == NULL || file == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	mtk_cam = container_of(inode->i_cdev, struct mtk_camera, i_cdev);
	file->private_data = mtk_cam;

	LOG_INF("Camera File Open! Camera Name:%s", mtk_cam->name);

	return 0;
}

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     camera_hw_release.
 * @param[in]
 *     inode: Pointer of inode.
 * @param[in]
 *     file: Pointer of file.
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
static int camera_hw_release(struct inode *inode, struct file *file)
{
	LOG_INF("Camera File Close!");

	/* parameters sanity check */
	if (inode == NULL || file == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	return 0;
}

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Access sensor via ioctl.
 * @param[in]
 *     file: Pointer of file.
 * @param[in]
 *     cmd: Command ID.
 * @param[in]
 *     param: Command parameter.
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
static long camera_hw_ioctl(struct file *file, const unsigned int cmd,
			    const unsigned long param)
{
	struct mtk_camera *mtk_cam = NULL;
	int ret = 0;
	void *buf = NULL;
	u32 *idx = NULL;

	/* parameters sanity check */
	if (file == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	mtk_cam = (struct mtk_camera *)file->private_data;

	if (_IOC_DIR(cmd) == _IOC_NONE) {
	} else {
		buf = kmalloc(_IOC_SIZE(cmd), GFP_KERNEL);

		if (buf == NULL) {
			LOG_ERR("ioctl allocate mem failed");
			ret = -ENOMEM;
			goto ioctl_exit;
		}

		if (_IOC_WRITE & _IOC_DIR(cmd)) {
			if (copy_from_user(buf, (void *)param,
					   _IOC_SIZE(cmd))) {
				kfree(buf);
				LOG_ERR("ioctl copy from user failed");
				ret = -EFAULT;
				goto ioctl_exit;
			}
		}
	}

	switch (cmd) {
	case KDIMGSENSORIOC_X_SET_DRIVER:
		LOG_DBG("KDIMGSENSORIOC_X_SET_DRIVER");
		if (buf != NULL) {
			idx = (u32 *)buf;
			ret = kd_setdriver(mtk_cam);
			*idx = mtk_cam->sensorIdx;
		} else {
			ret = -EFAULT;
			goto ioctl_exit;
		}
		break;
	case KDIMGSENSORIOC_X_SET_POWER_ON:
		LOG_DBG("KDIMGSENSORIOC_X_SET_POWER_ON");
		if (buf != NULL) {
			ret = kd_cis_module_poweronoff(mtk_cam, true, buf);
		} else {
			ret = -EFAULT;
			goto ioctl_exit;
		}
		break;
	case KDIMGSENSORIOC_X_SET_POWER_OFF:
		LOG_DBG("KDIMGSENSORIOC_X_SET_POWER_OFF");
		if (buf != NULL) {
			ret = kd_cis_module_poweronoff(mtk_cam, false, buf);
		} else {
			ret = -EFAULT;
			goto ioctl_exit;
		}
		break;
	case KDIMGSENSORIOC_X_FEATURECONCTROL:
		LOG_DBG("KDIMGSENSORIOC_X_FEATURECONCTROL");
		if (buf != NULL) {
			ret = adopt_camera_hw_featurecontrol(mtk_cam, buf);
		} else {
			ret = -EFAULT;
			goto ioctl_exit;
		}
		break;
#ifndef CONFIG_VIDEO_MEDIATEK_ISP_TINY
	case KDIMGSENSORIOC_T_SET_A60919:
		LOG_DBG("KDIMGSENSORIOC_T_SET_A60919");
		ret = adopt_camera_hw_set_dtb(mtk_cam);
		break;
#endif
	default:
		LOG_ERR("Invalid command %d", cmd);
		ret = -EPERM;
		if (buf != NULL)
			kfree(buf);
		goto ioctl_exit;
	}

	if (_IOC_READ & _IOC_DIR(cmd)) {
		if (copy_to_user((void __user *)param, buf, _IOC_SIZE(cmd))) {
			kfree(buf);
			LOG_ERR("ioctl copy to user failed");
			ret = -EFAULT;
			goto ioctl_exit;
		}
	}

	if (buf != NULL)
		kfree(buf);

ioctl_exit:

	return ret;
}

static const struct file_operations mtk_camera_hw_fops = {
	.owner = THIS_MODULE,
	.open = camera_hw_open,
	.release = camera_hw_release,
	.unlocked_ioctl = camera_hw_ioctl,
};

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     For debugfs use.
 * @param[in]
 *     inode: Pointer of inode.
 * @param[out]
 *     file: Pointer of file.
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
static int mtk_camera_debug_open(struct inode *inode, struct file *file)
{
	/* parameters sanity check */
	if (inode == NULL || file == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	file->private_data = inode->i_private;

	return 0;
}

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     For debugfs use.
 * @param[in]
 *     file: Pointer of file.
 * @param[in]
 *     ubuf: Pointer of char.
 * @param[in]
 *     count: Data size.
 * @param[in]
 *     ppos: Pointer of loff_t.
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
static ssize_t mtk_camera_debug_read(struct file *file, char __user *ubuf,
				     size_t count, loff_t *ppos)
{
	/* parameters sanity check */
	if (file == NULL || ubuf == NULL || ppos == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	return 0;
}

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     For debugfs use.
 * @param[in]
 *     file: Pointer of file.
 * @param[in]
 *     ubuf: Pointer of char.
 * @param[in]
 *     count: Data size.
 * @param[in]
 *     ppos: Pointer of loff_t.
 * @return
 *     Data size.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static ssize_t mtk_camera_debug_write(struct file *file,
				      const char __user *ubuf,
				      size_t count, loff_t *ppos)
{
	char str_buf[64] = {'\0'};
	u32 CopyBufSize = (count < (sizeof(str_buf) - 1)) ?
			  (count) : (sizeof(str_buf) - 1);
	unsigned int size = 0;
	struct mtk_camera *mtk_cam = NULL;
	struct ACDK_SENSOR_REG_INFO_STRUCT sensor_reg;
	struct SENSOR_FUNCTION_STRUCT *sensorfunc;

	/* parameters sanity check */
	if (file == NULL || ubuf == NULL || ppos == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	mtk_cam = file->private_data;
	memset(&sensor_reg, 0, sizeof(struct ACDK_SENSOR_REG_INFO_STRUCT));
	size = sizeof(struct ACDK_SENSOR_REG_INFO_STRUCT);

	if (copy_from_user(str_buf, ubuf, CopyBufSize))
		return -EFAULT;

	sensorfunc = mtk_cam->invoke_sensorfunc;

	if (sscanf(str_buf, "%hx %hx", &sensor_reg.RegAddr,
		   &sensor_reg.RegData) == 2) {
		if (sensorfunc) {
			sensorfunc->SensorFeatureControl(
				mtk_cam, SENSOR_FEATURE_SET_REGISTER,
				(unsigned char *)&sensor_reg, &size);

			sensorfunc->SensorFeatureControl(
				mtk_cam, SENSOR_FEATURE_GET_REGISTER,
				(unsigned char *)&sensor_reg, &size);
			LOG_INF("write addr:0x%08x, data:0x%08x",
				sensor_reg.RegAddr, sensor_reg.RegData);
		}
	} else if (kstrtou16(str_buf, 0, &sensor_reg.RegAddr) == 0) {
		if (sensorfunc) {
			sensorfunc->SensorFeatureControl(
				mtk_cam, SENSOR_FEATURE_GET_REGISTER,
				(unsigned char *)&sensor_reg, &size);
			LOG_INF("read addr:0x%08x, data:0x%08x",
				sensor_reg.RegAddr, sensor_reg.RegData);
		}
	}

	return count;
}

static const struct file_operations mtk_camera_debug_fops = {
	.read = mtk_camera_debug_read,
	.write = mtk_camera_debug_write,
	.open = mtk_camera_debug_open,
};

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Probe sensor I2C char device driver.
 * @param[in]
 *     client: I2C client.
 * @param[in]
 *     id: I2C device ID.
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
static int mtk_camera_i2c_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct mtk_camera *mtk_cam = NULL;
	struct device *class_dev = NULL;
	int ret = 0;

	LOG_VRB("mtk camera sensor i2c driver init!");

	/* parameters sanity check */
	if (client == NULL) {
		LOG_ERR("client is NULL");
		return -EFAULT;
	}

	mtk_cam = devm_kzalloc(&client->dev, sizeof(*mtk_cam), GFP_KERNEL);
	if (mtk_cam == NULL) {
		LOG_ERR("mtk_cam is NULL");
		return -EFAULT;
	}
	of_property_read_u32(client->dev.of_node, "sensorIdx",
			     &mtk_cam->sensorIdx);
	strlcpy(mtk_cam->name, mtk_camera_name[mtk_cam->sensorIdx],
		sizeof(mtk_cam->name));

	mtk_cam->client = client;
	i2c_set_clientdata(client, mtk_cam);
	mtk_cam->i2c_addr = client->addr;

	LOG_VRB("mtk camera sensor i2c driver addr:0x%x, sensorIdx:%d",
		client->addr, mtk_cam->sensorIdx);

	/* register chr dev */
	mtk_cam->devno = MKDEV(MAJOR(camera_hw_devno), mtk_cam->sensorIdx);
	cdev_init(&mtk_cam->i_cdev, &mtk_camera_hw_fops);
	mtk_cam->i_cdev.owner = THIS_MODULE;
	ret = cdev_add(&mtk_cam->i_cdev, mtk_cam->devno, 1);
	if (ret)
		goto cdev_error;

	/* register class */
	mtk_cam->sensor_class = class_create(THIS_MODULE, mtk_cam->name);
	if (IS_ERR(mtk_cam->sensor_class)) {
		ret = PTR_ERR(mtk_cam->sensor_class);
		goto cdev_error;
	}
	class_dev = device_create(mtk_cam->sensor_class, NULL, mtk_cam->devno,
				  NULL, mtk_cam->name);
	if (IS_ERR(class_dev)) {
		ret = PTR_ERR(class_dev);
		goto class_error;
	}

#if defined(CONFIG_DEBUG_FS)
	/* register debug fs */
	mtk_cam->debugfs =
	    debugfs_create_file(mtk_cam->name, S_IFREG | S_IRUGO |
				S_IWUSR | S_IWGRP, g_debugdir, (void *)mtk_cam,
				&mtk_camera_debug_fops);
	if (IS_ERR(mtk_cam->debugfs)) {
		ret = PTR_ERR(mtk_cam->debugfs);
		goto class_error;
	}
#endif

#ifdef CONFIG_COMMON_CLK_MT3612
	Get_ccf_clk(mtk_cam, &client->dev);
#endif

	if (!fpga_mode) {
		/* GPIO init */
		mtk_cam->sensor_rst =
			of_get_named_gpio(client->dev.of_node, "cd-gpios", 0);
		mtk_cam->sensor_pdn =
			of_get_named_gpio(client->dev.of_node, "cd-gpios", 1);
	}

	return 0;

class_error:
	class_destroy(mtk_cam->sensor_class);

cdev_error:
	cdev_del(&mtk_cam->i_cdev);

	return ret;
}

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Remove sensor I2C char device driver.
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
static int mtk_camera_i2c_remove(struct i2c_client *client)
{
	struct mtk_camera *mtk_cam = NULL;

	/* parameters sanity check */
	if (client == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	mtk_cam = i2c_get_clientdata(client);

#if defined(CONFIG_DEBUG_FS)
	debugfs_remove(mtk_cam->debugfs);
#endif
	device_destroy(mtk_cam->sensor_class, mtk_cam->devno);
	class_destroy(mtk_cam->sensor_class);
	cdev_del(&mtk_cam->i_cdev);

	return 0;
}

static const struct of_device_id mtk_camera_i2c_of_ids[] = {
	{ .compatible = "mediatek,camera-sensor", },
	{ },
};
MODULE_DEVICE_TABLE(of, mtk_camera_i2c_of_ids);

static const struct i2c_device_id camera_hw_i2c_id[] = {
	{ CAMERA_HW_DEVNAME_SIDELL, 0 },
	{ CAMERA_HW_DEVNAME_SIDELR, 0 },
	{ CAMERA_HW_DEVNAME_SIDERL, 0 },
	{ CAMERA_HW_DEVNAME_SIDERR, 0 },
	{ CAMERA_HW_DEVNAME_GAZEL, 0 },
	{ CAMERA_HW_DEVNAME_GAZER, 0 },
	{ }
};

struct i2c_driver mtk_camera_i2c_driver = {
	.probe = mtk_camera_i2c_probe,
	.remove = mtk_camera_i2c_remove,
	.driver = {
		.name = CAMERA_HW_DEVNAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(mtk_camera_i2c_of_ids),
	},
	.id_table = camera_hw_i2c_id,
};

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Probe sensor driver.
 * @param[in]
 *     pdev: Pointer of platform_device.
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
static int mtk_camera_probe(struct platform_device *pdev)
{
	int ret = 0;

	/* parameters sanity check */
	if (pdev == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	of_property_read_u32(pdev->dev.of_node, "fpga-mode", &fpga_mode);

	ret = alloc_chrdev_region(&camera_hw_devno, 0, MTK_CAMERA_MAX_NUM,
				  "kd_camera");
	if (ret < 0)
		return ret;

#if defined(CONFIG_DEBUG_FS)
	g_debugdir = debugfs_create_dir("kd_camera", NULL);
	if (IS_ERR(g_debugdir)) {
		ret = PTR_ERR(g_debugdir);
		LOG_ERR("Unable to create debugfs for module");
		goto debugfs_err;
	}
#endif

	if (i2c_add_driver(&mtk_camera_i2c_driver))
		goto i2c_err;

	return 0;
i2c_err:
#if defined(CONFIG_DEBUG_FS)
	debugfs_remove_recursive(g_debugdir);

debugfs_err:
#endif
	unregister_chrdev_region(camera_hw_devno, MTK_CAMERA_MAX_NUM);

	return ret;
}

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Remove sensor driver.
 * @param[in]
 *     pdev: Pointer of platform_device.
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
static int mtk_camera_remove(struct platform_device *pdev)
{
	/* parameters sanity check */
	if (pdev == NULL) {
		LOG_ERR("NULL arg");
		return -EFAULT;
	}

	i2c_del_driver(&mtk_camera_i2c_driver);

#if defined(CONFIG_DEBUG_FS)
	debugfs_remove_recursive(g_debugdir);
#endif

	unregister_chrdev_region(camera_hw_devno, MTK_CAMERA_MAX_NUM);

	return 0;
}

static const struct of_device_id mtk_camera_of_match[] = {
	{ .compatible = "mediatek,camera_hw" },
	{ },
};
MODULE_DEVICE_TABLE(of, mtk_camera_of_match);

struct platform_driver mtk_camera_driver = {
	.probe = mtk_camera_probe,
	.remove = mtk_camera_remove,
	.driver = {
		.name = "mtk-camera",
		.owner = THIS_MODULE,
		.of_match_table = mtk_camera_of_match,
	},
};

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Register sensor driver.
 *  @par Parameters
 *     none.
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
static int mtk_camera_driver_init(void)
{
	int ret = 0;

	pr_info("mtk camera driver init!");

	ret = platform_driver_register(&mtk_camera_driver);
	if (ret) {
		pr_alert(" platform_driver_register failed!");
		return ret;
	}
	pr_info("mtk camera driver init ok!");
	return ret;
}

/** @ingroup IP_group_sensor_internal_function
 * @par Description
 *     Unregister sensor driver.
 *  @par Parameters
 *     none.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_camera_driver_exit(void)
{
	pr_info("mtk camera driver exit!");
	platform_driver_unregister(&mtk_camera_driver);
}

module_init(mtk_camera_driver_init);
module_exit(mtk_camera_driver_exit);

MODULE_DESCRIPTION("MTK Camera Sensor Driver");
MODULE_AUTHOR("MTK");
MODULE_LICENSE("GPL");
