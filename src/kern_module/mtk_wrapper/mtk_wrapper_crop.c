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

#include <soc/mediatek/mtk_crop.h>
#include "mtk_wrapper_crop.h"
#include "mtk_wrapper_util.h"

#define CROP_NUM (4)

static struct device *s_crop[CROP_NUM];
static u8 s_active_num;

static int mtk_wrapper_crop_power_on(uint8_t num)
{
	int i;
	int ret = 0;

	if (num > CROP_NUM) {
		return -EINVAL;
	}

	for (i = 0; i < num; i++) {
		ret |= mtk_crop_power_on(s_crop[i]);
	}
	s_active_num = num;
	return ret;
}

static int mtk_wrapper_crop_power_off(void)
{
	int i;
	int ret = 0;

	for (i = 0; i < s_active_num; i++) {
		ret |= mtk_crop_power_off(s_crop[i]);
	}
	s_active_num = 0;
	return  ret;
}

static int mtk_wrapper_crop_reset(void)
{
	int i;
	int ret = 0;

	for (i = 0; i < s_active_num; i++) {
		ret |= mtk_crop_reset(s_crop[i], NULL);
	}
	return  ret;
}

static int mtk_wrapper_crop_start(void)
{
	int i;
	int ret = 0;

	for (i = 0; i < s_active_num; i++) {
		ret |= mtk_crop_start(s_crop[i], NULL);
	}
	return  ret;
}

static int mtk_wrapper_crop_stop(void)
{
	int i;
	int ret = 0;

	for (i = 0; i < s_active_num; i++) {
		ret |= mtk_crop_stop(s_crop[i], NULL);
	}
	return  ret;
}

static int mtk_wrapper_crop_set_region(void *user)
{
	int i;
	int ret = 0;
	args_crop_set_region args;

	ret = copy_from_user(&args, user, sizeof(args_crop_set_region));
	if (ret != 0) {
		return -EFAULT;
	}

	for (i = 0; i < s_active_num; i++) {
		ret |= mtk_crop_set_region(s_crop[i], NULL,
					   args.region.in_w,
					   args.region.in_h,
					   args.region.out_w,
					   args.region.out_h,
					   args.region.out_x,
					   args.region.out_y);
	}
	return ret;
}

static long mtk_wrapper_crop_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case CROP_POWER_ON:
		ret = mtk_wrapper_crop_power_on((uint8_t)arg);
		break;
	case CROP_POWER_OFF:
		ret = mtk_wrapper_crop_power_off();
		break;
	case CROP_RESET:
		ret = mtk_wrapper_crop_reset();
		break;
	case CROP_SET_REGION:
		ret = mtk_wrapper_crop_set_region((void *)arg);
		break;
	case CROP_START:
		ret = mtk_wrapper_crop_start();
		break;
	case CROP_STOP:
		ret = mtk_wrapper_crop_stop();
		break;
	default:
		pr_err("crop_ioctl: unknown command(%d)\n", cmd);
		ret = -EINVAL;
		break;
	}
	return ret;
}

static const struct file_operations mtk_wrapper_crop_fops = {
	.unlocked_ioctl = mtk_wrapper_crop_ioctl
};

int init_module_mtk_wrapper_crop(void)
{
	int ret;
	int i;

	pr_info("%s start\n", __func__);
	ret = register_chrdev(SCE_KM_CDEV_MAJOR_MTK_WRAPPER_CROP,
			      SCE_KM_CROP_DEVICE_NAME,
			      &mtk_wrapper_crop_fops);
	if (ret < 0) {
		pr_err("log: alloc_chrdev(%s) failed(%d).\n",
		       SCE_KM_CROP_DEVICE_NAME, ret);
		return ret;
	}

	for (i = 0; i < CROP_NUM; i++) {
		ret = mtk_wrapper_display_pipe_probe("mediatek,crop", i, &s_crop[i]);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

void cleanup_module_mtk_wrapper_crop(void)
{
}
