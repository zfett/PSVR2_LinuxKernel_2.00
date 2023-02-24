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

#include <soc/mediatek/mtk_p2s.h>

#include <mtk_wrapper_p2s.h>
#include "mtk_wrapper_util.h"

static struct device *s_p2s;

static uint8_t convert_p2s_mode(MTK_WRAPPER_P2S_MODE mode)
{
	int ret = MTK_P2S_PROC_MODE_L_ONLY;

	switch (mode) {
	case MTK_WRAPPER_P2S_MODE_L_ONLY:
		ret = MTK_P2S_PROC_MODE_L_ONLY;
		break;
	case MTK_WRAPPER_P2S_MODE_LR:
		ret = MTK_P2S_PROC_MODE_LR;
		break;
	default:
		pr_err("Unknown mode(%d) treat as L_ONLY\n", mode);
		break;
	}
	return ret;

}

static int mtk_wrapper_p2s_power_on(void)
{
	return mtk_p2s_power_on(s_p2s);
}

static int mtk_wrapper_p2s_power_off(void)
{
	return mtk_p2s_power_off(s_p2s);
}

static int mtk_wrapper_p2s_set_mode(MTK_WRAPPER_P2S_MODE mode)
{
	return mtk_p2s_set_proc_mode(s_p2s, NULL, convert_p2s_mode(mode));
}

static int mtk_wrapper_p2s_set_region(void *user)
{
	int ret = 0;
	args_p2s_set_region args;

	ret = copy_from_user(&args, user, sizeof(args_p2s_set_region));

	if (ret != 0) {
		return -EFAULT;
	}
	ret = mtk_p2s_set_region(s_p2s, NULL, args.width, args.height);
	return ret;
}

static int mtk_wrapper_p2s_start(void)
{
	return mtk_p2s_start(s_p2s, NULL);
}

static int mtk_wrapper_p2s_stop(void)
{
	return mtk_p2s_stop(s_p2s, NULL);
}

static long mtk_wrapper_p2s_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case P2S_POWER_ON:
		ret = mtk_wrapper_p2s_power_on();
		break;
	case P2S_POWER_OFF:
		ret = mtk_wrapper_p2s_power_off();
		break;
	case P2S_SET_MODE:
		ret = mtk_wrapper_p2s_set_mode((MTK_WRAPPER_P2S_MODE)arg);
		break;
	case P2S_SET_REGION:
		ret = mtk_wrapper_p2s_set_region((void *)arg);
		break;
	case P2S_START:
		ret = mtk_wrapper_p2s_start();
		break;
	case P2S_STOP:
		ret = mtk_wrapper_p2s_stop();
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static const struct file_operations mtk_wrapper_p2s_fops = {
	.unlocked_ioctl = mtk_wrapper_p2s_ioctl
};

int init_module_mtk_wrapper_p2s(void)
{
	int ret;

	pr_info("%s start\n", __func__);
	ret = register_chrdev(SCE_KM_CDEV_MAJOR_MTK_WRAPPER_P2S, SCE_KM_P2S_DEVICE_NAME, &mtk_wrapper_p2s_fops);
	if (ret < 0) {
		pr_err("log: alloc_chrdev(%s) failed(%d).\n", SCE_KM_P2S_DEVICE_NAME, ret);
		return ret;
	}

	ret = mtk_wrapper_display_pipe_probe("mediatek,p2s", 0, &s_p2s);
	if (ret < 0) {
		pr_err("can't find display_pipe p2s\n");
		return ret;
	}

	return 0;
}

void cleanup_module_mtk_wrapper_p2s(void)
{
}
