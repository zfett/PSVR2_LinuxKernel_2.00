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

#include <soc/mediatek/mtk_dsi.h>
#include <soc/mediatek/mtk_dsc.h>

#include <mtk_wrapper_dscenc.h>
#include "mtk_wrapper_util.h"

static struct device *s_dscenc;

static unsigned int mtk_wrapper_dscenc_convert_inout_config(MTK_WRAPPER_DSCENC_INOUT_CONFIG config)
{
	unsigned int ret = DSC_1_IN_1_OUT;

	switch (config) {
	case MTK_WRAPPER_DSCENC_1_IN_1_OUT:
		ret = DSC_1_IN_1_OUT;
		break;
	case MTK_WRAPPER_DSCENC_2_IN_1_OUT:
		ret = DSC_2_IN_1_OUT;
		break;
	case MTK_WRAPPER_DSCENC_2_IN_2_OUT:
		ret = DSC_2_IN_2_OUT;
		break;
	default:
		ret = UINT_MAX;
		break;
	}
	return ret;
}

static unsigned int mtk_wrapper_dscenc_convert_dsc_version(MTK_WRAPPER_DSCENC_VERSION version)
{
	unsigned int ret = DSC_V1P2;

	switch (version) {
	case MTK_WRAPPER_DSCENC_VERSION_1_1:
		ret = DSC_V1P1;
		break;
	case MTK_WRAPPER_DSCENC_VERSION_1_2:
		ret = DSC_V1P2;
		break;
	default:
		ret = UINT_MAX;
		break;
	}
	return ret;
}

static unsigned int mtk_wrapper_dscenc_convert_ich_line_clear(MTK_WRAPPER_DSCENC_ICH_LINE_CLEAR ich_line_clear)
{
	unsigned int ret = DSC_ICH_LINE_CLEAR_AUTO;

	switch (ich_line_clear) {
	case MTK_WRAPPER_ICH_LINE_CLEAR_AUTO:
		ret = DSC_ICH_LINE_CLEAR_AUTO;
		break;
	case MTK_WRAPPER_ICH_LINE_CLEAR_AT_LINE:
		ret = DSC_ICH_LINE_CLEAR_AT_LINE;
		break;
	case MTK_WRAPPER_ICH_LINE_CLEAR_AT_SLICE:
		ret = DSC_ICH_LINE_CLEAR_AT_SLICE;
		break;
	default:
		ret = UINT_MAX;
		break;
	}
	return ret;
}

enum mtk_dsi_pixel_format mtk_wrapper_dsi_convert_format(MTK_WRAPPER_DSI_FORMAT format);

static int mtk_wrapper_dscenc_set_config(void *user)
{
	args_dsc_init_hw args;
	struct dsc_config cfg;
	int ret;
	u32 pps[DSCENC_PPS_SIZE];

	ret = copy_from_user(&args, user, sizeof(args_dsc_init_hw));
	if (ret != 0) {
		return -EFAULT;
	}

	cfg.pic_format = mtk_wrapper_dsi_convert_format(args.config.format);
	cfg.disp_pic_w = args.config.disp_pic_w;
	cfg.disp_pic_h = args.config.disp_pic_h;
	cfg.slice_h    = args.config.slice_h;
	cfg.inout_sel  = mtk_wrapper_dscenc_convert_inout_config(args.config.inout_sel);
	if (cfg.inout_sel == UINT_MAX) {
		return -EINVAL;
	}
	cfg.version    = mtk_wrapper_dscenc_convert_dsc_version(args.config.version);
	if (cfg.version == UINT_MAX) {
		return -EINVAL;
	}

	cfg.ich_line_clear = mtk_wrapper_dscenc_convert_ich_line_clear(args.config.ich_line_clear);
	if (cfg.ich_line_clear == UINT_MAX) {
		return -EINVAL;
	}

	if (!args.config.pps) {
		ret = dsc_hw_init(s_dscenc, cfg, NULL, NULL);
	} else {
		ret = copy_from_user(&pps, args.config.pps, sizeof(pps));
		if (ret != 0) {
			return -EFAULT;
		}
		ret = dsc_hw_init(s_dscenc, cfg, pps, NULL);
	}

	return ret;
}

static int mtk_wrapper_dscenc_power_on(void)
{
	return dsc_power_on(s_dscenc);
}

static int mtk_wrapper_dscenc_power_off(void)
{
	return dsc_power_off(s_dscenc);
}

static int mtk_wrapper_dscenc_start(void)
{
	return dsc_start(s_dscenc, NULL);
}

static int mtk_wrapper_dscenc_reset(void)
{
	return dsc_reset(s_dscenc, NULL);
}

static int mtk_wrapper_dscenc_relay(void)
{
	return dsc_relay(s_dscenc, NULL);
}

static long mtk_wrapper_dscenc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case DSCENC_POWER_ON:
		ret = mtk_wrapper_dscenc_power_on();
		break;
	case DSCENC_POWER_OFF:
		ret = mtk_wrapper_dscenc_power_off();
		break;
	case DSCENC_INIT_HW:
		ret = mtk_wrapper_dscenc_set_config((void *)arg);
		break;
	case DSCENC_START:
		ret = mtk_wrapper_dscenc_start();
		break;
	case DSCENC_RESET:
		ret = mtk_wrapper_dscenc_reset();
		break;
	case DSCENC_RELAY:
		ret = mtk_wrapper_dscenc_relay();
		break;
	default:
		pr_err("scenc_ioctl: unknown command(%d)\n", cmd);
		ret = -EINVAL;
		break;
	}
	return ret;
}

static const struct file_operations mtk_wrapper_dscenc_fops = {
	.unlocked_ioctl = mtk_wrapper_dscenc_ioctl
};

int init_module_mtk_wrapper_dscenc(void)
{
	int ret;

	pr_info("%s start\n", __func__);
	ret = register_chrdev(SCE_KM_CDEV_MAJOR_MTK_WRAPPER_DSCENC, SCE_KM_DSCENC_DEVICE_NAME, &mtk_wrapper_dscenc_fops);
	if (ret < 0) {
		pr_err("log: alloc_chrdev(%s) failed(%d).\n", SCE_KM_DSCENC_DEVICE_NAME, ret);
		return ret;
	}

	ret = mtk_wrapper_display_pipe_probe("mediatek,dsc", 0, &s_dscenc);
	if (ret < 0) {
		pr_err("can't find display_pipe dsc\n");
		return ret;
	}

	return 0;
}

void cleanup_module_mtk_wrapper_dscenc(void)
{
}
