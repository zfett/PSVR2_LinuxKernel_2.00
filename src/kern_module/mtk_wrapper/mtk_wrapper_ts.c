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

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/of_platform.h>
#include <linux/uaccess.h>

#include <sce_km_defs.h>
#include <mtk_wrapper_ts.h>

#include <soc/mediatek/mtk_ts.h>

#include "mtk_wrapper_util.h"

static struct device *s_ts;

static int mtk_wrapper_ts_set_vts(uint32_t vts)
{
	return mtk_ts_set_vts(s_ts, vts);
}

static long mtk_wrapper_ts_set_stc(void *user)
{
	int ret;
	ts_stc_setting tsset;

	ret = copy_from_user(&tsset, user, sizeof(tsset));
	if (ret != 0) {
		return -EFAULT;
	}

	ret = mtk_ts_set_stc(s_ts, tsset.stc_base_31_0, tsset.stc_base_32, tsset.stc_ext_8_0);
	if (ret != 0) {
		pr_err("Can't set stc\n");
		return -EFAULT;
	}

	return 0;
}

static long mtk_wrapper_ts_get_current_time(void *user)
{
	int ret;
	ts_info tsinfo = {0};

	ret = mtk_ts_get_current_time(s_ts, &tsinfo.vts, &tsinfo.stc, &tsinfo.dp_counter);
	if (ret != 0) {
		pr_err("Can't get ts info\n");
		return -EFAULT;
	}

	ret = copy_to_user(user, &tsinfo, sizeof(tsinfo));
	if (ret != 0) {
		pr_err("copy_to_user error\n");
		return -EFAULT;
	}

	return 0;
}

static long mtk_wrapper_ts_get_av_time(unsigned int cmd, void *user)
{
	int ret;
	ts_av_info tsinfo = {0};

	switch (cmd) {
	case TS_GET_DSI_TIME:
		ret = mtk_ts_get_dsi_time(s_ts, &tsinfo.stc, &tsinfo.dp_counter);
		break;
	case TS_GET_AUDIO_HDMIDP_TIME:
		ret = mtk_ts_get_audio_hdmidp_time(s_ts, &tsinfo.stc, &tsinfo.dp_counter);
		break;
	case TS_GET_AUDIO_COMMON_TIME:
		ret = mtk_ts_get_audio_common_time(s_ts, &tsinfo.stc, &tsinfo.dp_counter);
		break;
	case TS_GET_AUDIO_TDMOUT_TIME:
		ret = mtk_ts_get_audio_tdmout_time(s_ts, &tsinfo.stc, &tsinfo.dp_counter);
		break;
	case TS_GET_DPRX_VIDEO_TIME:
		ret = mtk_ts_get_dprx_video_time(s_ts, &tsinfo.stc, &tsinfo.dp_counter);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	if (ret != 0) {
		pr_err("Can't get ts av info\n");
		return -EFAULT;
	}

	ret = copy_to_user(user, &tsinfo, sizeof(tsinfo));
	if (ret != 0) {
		pr_err("copy_to_user error\n");
		return -EFAULT;
	}

	return 0;
}

static long mtk_wrapper_ts_get_vision_time(unsigned int cmd, void *user)
{
	int ret;
	ts_vision_info tsinfo = {0};

	switch (cmd) {
	case TS_GET_SIDE0_A_P1_VISION_TIME:
		ret = mtk_ts_get_side_0_a_p1_vision_time(s_ts, &tsinfo.vts, &tsinfo.dp_counter);
		break;
	case TS_GET_SIDE0_B_P1_VISION_TIME:
		ret = mtk_ts_get_side_0_b_p1_vision_time(s_ts, &tsinfo.vts, &tsinfo.dp_counter);
		break;
	case TS_GET_SIDE1_A_P1_VISION_TIME:
		ret = mtk_ts_get_side_1_a_p1_vision_time(s_ts, &tsinfo.vts, &tsinfo.dp_counter);
		break;
	case TS_GET_SIDE1_B_P1_VISION_TIME:
		ret = mtk_ts_get_side_1_b_p1_vision_time(s_ts, &tsinfo.vts, &tsinfo.dp_counter);
		break;
	case TS_GET_SIDE0_A_P2_VISION_TIME:
		ret = mtk_ts_get_side_0_a_p2_vision_time(s_ts, &tsinfo.vts, &tsinfo.dp_counter);
		break;
	case TS_GET_SIDE0_B_P2_VISION_TIME:
		ret = mtk_ts_get_side_0_b_p2_vision_time(s_ts, &tsinfo.vts, &tsinfo.dp_counter);
		break;
	case TS_GET_SIDE1_A_P2_VISION_TIME:
		ret = mtk_ts_get_side_1_a_p2_vision_time(s_ts, &tsinfo.vts, &tsinfo.dp_counter);
		break;
	case TS_GET_SIDE1_B_P2_VISION_TIME:
		ret = mtk_ts_get_side_1_b_p2_vision_time(s_ts, &tsinfo.vts, &tsinfo.dp_counter);
		break;
	case TS_GET_GAZE0_VISION_TIME:
		ret = mtk_ts_get_gaze_0_vision_time(s_ts, &tsinfo.vts, &tsinfo.dp_counter);
		break;
	case TS_GET_GAZE1_VISION_TIME:
		ret = mtk_ts_get_gaze_1_vision_time(s_ts, &tsinfo.vts, &tsinfo.dp_counter);
		break;
	case TS_GET_SIDE0_CAMERA_SYNC_TIME:
		ret = mtk_ts_get_side_0_camera_sync_time(s_ts, &tsinfo.vts, &tsinfo.dp_counter);
		break;
	case TS_GET_SIDE1_CAMERA_SYNC_TIME:
		ret = mtk_ts_get_side_1_camera_sync_time(s_ts, &tsinfo.vts, &tsinfo.dp_counter);
		break;
	case TS_GET_GAZE0_CAMERA_SYNC_TIME:
		ret = mtk_ts_get_gaze_0_camera_sync_time(s_ts, &tsinfo.vts, &tsinfo.dp_counter);
		break;
	case TS_GET_GAZE1_CAMERA_SYNC_TIME:
		ret = mtk_ts_get_gaze_1_camera_sync_time(s_ts, &tsinfo.vts, &tsinfo.dp_counter);
		break;
	case TS_GET_GAZE2_CAMERA_SYNC_TIME:
		ret = mtk_ts_get_gaze_2_camera_sync_time(s_ts, &tsinfo.vts, &tsinfo.dp_counter);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	if (ret != 0) {
		pr_err("Can't get ts vision info\n");
		return -EFAULT;
	}

	ret = copy_to_user(user, &tsinfo, sizeof(tsinfo));
	if (ret != 0) {
		pr_err("copy_to_user error\n");
		return -EFAULT;
	}

	return 0;
}

static long mtk_wrapper_ts_get_imu_time(unsigned int cmd, void *user)
{
	int ret;
	ts_imu_info tsinfo = {0};

	switch (cmd) {
	case TS_GET_IMU0_TIME:
		ret = mtk_ts_get_imu_0_time(s_ts, &tsinfo.vts, &tsinfo.dp_counter, &tsinfo.imu_counter);
		break;
	case TS_GET_IMU1_TIME:
		ret = mtk_ts_get_imu_1_time(s_ts, &tsinfo.vts, &tsinfo.dp_counter, &tsinfo.imu_counter);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	if (ret != 0) {
		pr_err("Can't get ts imu info\n");
		return -EFAULT;
	}

	ret = copy_to_user(user, &tsinfo, sizeof(tsinfo));
	if (ret != 0) {
		pr_err("copy_to_user error\n");
		return -EFAULT;
	}

	return 0;
}

static long mtk_wrapper_ts_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret;

	switch (cmd) {
	case TS_SET_VTS:
		ret = mtk_wrapper_ts_set_vts((uint32_t)arg);
		break;
	case TS_SET_STC:
		ret = mtk_wrapper_ts_set_stc((void *)arg);
		break;
	case TS_GET_CUR_TIME:
		ret = mtk_wrapper_ts_get_current_time((void *)arg);
		break;
	case TS_GET_DSI_TIME:
	case TS_GET_AUDIO_HDMIDP_TIME:
	case TS_GET_AUDIO_COMMON_TIME:
	case TS_GET_AUDIO_TDMOUT_TIME:
	case TS_GET_DPRX_VIDEO_TIME:
		ret = mtk_wrapper_ts_get_av_time(cmd, (void *)arg);
		break;
	case TS_GET_SIDE0_A_P1_VISION_TIME:
	case TS_GET_SIDE0_B_P1_VISION_TIME:
	case TS_GET_SIDE1_A_P1_VISION_TIME:
	case TS_GET_SIDE1_B_P1_VISION_TIME:
	case TS_GET_SIDE0_A_P2_VISION_TIME:
	case TS_GET_SIDE0_B_P2_VISION_TIME:
	case TS_GET_SIDE1_A_P2_VISION_TIME:
	case TS_GET_SIDE1_B_P2_VISION_TIME:
	case TS_GET_GAZE0_VISION_TIME:
	case TS_GET_GAZE1_VISION_TIME:
	case TS_GET_SIDE0_CAMERA_SYNC_TIME:
	case TS_GET_SIDE1_CAMERA_SYNC_TIME:
	case TS_GET_GAZE0_CAMERA_SYNC_TIME:
	case TS_GET_GAZE1_CAMERA_SYNC_TIME:
	case TS_GET_GAZE2_CAMERA_SYNC_TIME:
		ret = mtk_wrapper_ts_get_vision_time(cmd, (void *)arg);
		break;
	case TS_GET_IMU0_TIME:
	case TS_GET_IMU1_TIME:
		ret = mtk_wrapper_ts_get_imu_time(cmd, (void *)arg);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static const struct file_operations mtk_wrapper_ts_fops = {
	.unlocked_ioctl = mtk_wrapper_ts_ioctl
};

int init_module_mtk_wrapper_ts(void)
{
	int ret;

	pr_info("%s start\n", __func__);
	ret = register_chrdev(SCE_KM_CDEV_MAJOR_MTK_WRAPPER_TS, SCE_KM_TS_DEVICE_NAME, &mtk_wrapper_ts_fops);
	if (ret < 0) {
		pr_err("log: alloc_chrdev(%s) failed(%d).\n", SCE_KM_TS_DEVICE_NAME, ret);
		return ret;
	}
	ret = mtk_wrapper_display_pipe_probe("mediatek,ts", 0, &s_ts);
	if (ret < 0) {
		pr_err("Can't find timestamp\n");
		return -EINVAL;
	}

#ifdef CONFIG_SIE_DEVELOP_BUILD
	mtk_ts_set_vts(s_ts, 0xF0000000);
#endif

	return 0;
}

void cleanup_module_mtk_wrapper_ts(void)
{
}
