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
#include <linux/uaccess.h>

#include <sce_km_defs.h>
#include <mtk_wrapper_slicer.h>

#include <soc/mediatek/mtk_slicer.h>

#include "mtk_wrapper_util.h"

static struct device *s_dev;

static enum slcr_in_format convert_input_select(MTK_WRAPPER_SLICER_INPUT_SELECT inp_sel)
{
	enum slcr_in_format ret = DP_VIDEO;

	switch (inp_sel) {
	case MTK_WRAPPER_SLICER_INPUT_SELECT_VIDEO:
		ret = DP_VIDEO;
		break;
	case MTK_WRAPPER_SLICER_INPUT_SELECT_DSC:
		ret = DP_DSC;
		break;
	case MTK_WRAPPER_SLICER_INPUT_SELECT_VIDEO_DSC:
		ret = DP_VIDEO_DSC;
		break;
	default:
		pr_err("unknown input(%d)\n", inp_sel);
		ret = DP_VIDEO;
		break;
	}
	return ret;
}

static int mtk_wrapper_slicer_power_on(void)
{
	return mtk_slcr_power_on(s_dev);
}

static int mtk_wrapper_slicer_power_off(void)
{
	return mtk_slcr_power_off(s_dev);
}

static void mtk_wrapper_slicer_start(void)
{
	mtk_slcr_start(s_dev, NULL);
}

static void mtk_wrapper_slicer_stop(void)
{
	mtk_slcr_stop(s_dev, NULL);
}

static void mtk_wrapper_slicer_reset(void)
{
	mtk_slcr_reset(s_dev, NULL);
}

static int mtk_wrapper_slicer_set_config(const void *user)
{
	args_slicer_set_config args;
	int ret = 0;
	struct slcr_config config = {0};
	int i;

	ret = copy_from_user(&args, user, sizeof(args_slicer_set_config));
	if (ret != 0) {
		return -EFAULT;
	}

	config.in_format = convert_input_select(args.inp_sel);

	config.dp_video.input.width  = args.vid.width;
	config.dp_video.input.height = args.vid.height;
	config.dp_video.input.in_hsync_inv  = args.vid.in_hsync_inv;
	config.dp_video.input.in_vsync_inv  = args.vid.in_vsync_inv;

	config.dp_video.input.pxl_swap = NO_SWAP;
	config.dp_video.input.in_ch[0] = CHANNEL_0;
	config.dp_video.input.in_ch[1] = CHANNEL_1;
	config.dp_video.input.in_ch[2] = CHANNEL_2;

	config.dp_video.output.out_hsync_inv = false;
	config.dp_video.output.out_vsync_inv = true;
	config.dp_video.output.sync_port     = ORG_PORT;

	config.dp_video.output.out_ch[0] = CHANNEL_0;
	config.dp_video.output.out_ch[1] = CHANNEL_1;
	config.dp_video.output.out_ch[2] = CHANNEL_2;

	for (i = 0; i < 4; i++) {
		config.dp_video.output.slice_sop[i] = args.vid.sop[i];
		config.dp_video.output.slice_eop[i] = args.vid.eop[i];
		config.dp_video.output.vid_en[i]    = args.vid.vid_en[i];
	}
	config.dp_dsc.input.width         = args.dsc.width;
	config.dp_dsc.input.height        = args.dsc.height;
	config.dp_dsc.input.in_hsync_inv  = args.dsc.in_hsync_inv;
	config.dp_dsc.input.in_vsync_inv  = args.dsc.in_vsync_inv;
	config.dp_dsc.input.bit_rate      = args.dsc.bit_rate;
	config.dp_dsc.input.chunk_num     = args.dsc.chunk_num;
	config.dp_dsc.input.pxl_swap      = NO_SWAP;

	config.dp_dsc.output.out_hsync_inv = false;
	config.dp_dsc.output.out_vsync_inv = false;
	config.dp_dsc.output.sync_port     = ORG_PORT;
	config.dp_dsc.output.valid_byte    = args.dsc.valid_byte;

	for (i = 0; i < 4; i++) {
		config.dp_dsc.output.slice_sop[i] = args.dsc.sop[i];
		config.dp_dsc.output.slice_eop[i] = args.dsc.eop[i];
		config.dp_dsc.output.dsc_en[i] = (0x1 << i);
	}
	config.dp_dsc.output.endian = BIG_END;

	if (args.inp_sel == MTK_WRAPPER_SLICER_INPUT_SELECT_VIDEO ||
	    args.inp_sel == MTK_WRAPPER_SLICER_INPUT_SELECT_VIDEO_DSC) {
		config.dp_video.gce.event = GCE_INPUT;
		config.dp_video.gce.width = args.gce.width;
		config.dp_video.gce.height = args.gce.height;
	}
	if (args.inp_sel == MTK_WRAPPER_SLICER_INPUT_SELECT_DSC ||
	    args.inp_sel == MTK_WRAPPER_SLICER_INPUT_SELECT_VIDEO_DSC) {
		config.dp_dsc.gce.event = GCE_INPUT;
		config.dp_dsc.gce.width = args.gce.width;
		config.dp_dsc.gce.height = args.gce.height;
	}

	ret = mtk_slcr_config(s_dev, NULL, &config);
	return ret;
}

static long mtk_wrapper_slicer_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case SLICER_POWER_ON:
		ret = mtk_wrapper_slicer_power_on();
		break;
	case SLICER_POWER_OFF:
		ret = mtk_wrapper_slicer_power_off();
		break;
	case SLICER_START:
		mtk_wrapper_slicer_start();
		break;
	case SLICER_STOP:
		mtk_wrapper_slicer_stop();
		break;
	case SLICER_RESET:
		mtk_wrapper_slicer_reset();
		break;
	case SLICER_SET_CONFIG:
		mtk_wrapper_slicer_set_config((void *)arg);
		break;
	default:
		pr_err("slicer_ioctl: unknown command(%d)\n", cmd);
		return -EINVAL;
	}
	return ret;
}

static const struct file_operations mtk_wrapper_slicer_fops = {
	.open           = NULL,
	.unlocked_ioctl = mtk_wrapper_slicer_ioctl
};

int init_module_mtk_wrapper_slicer(void)
{
	int ret;

	pr_info("%s start\n", __func__);
	ret = register_chrdev(SCE_KM_CDEV_MAJOR_MTK_WRAPPER_SLICER, SCE_KM_SLICER_DEVICE_NAME, &mtk_wrapper_slicer_fops);
	if (ret < 0) {
		pr_err("log: alloc_chrdev(%s) failed(%d).\n", SCE_KM_MMSYS_DEVICE_NAME, ret);
		return ret;
	}

	ret = mtk_wrapper_display_pipe_probe("mediatek,slicer", 0, &s_dev);
	if (ret < 0) {
		pr_err("can't find display_pipe slicer\n");
		return ret;
	}

	return 0;
}

void cleanup_module_mtk_wrapper_slicer(void)
{
}
