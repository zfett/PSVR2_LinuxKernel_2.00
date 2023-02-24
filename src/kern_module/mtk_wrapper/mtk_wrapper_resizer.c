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
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <soc/mediatek/mtk_resizer.h>

#include <sce_km_defs.h>

#include <mtk_wrapper_resizer.h>
#include "mtk_wrapper_util.h"

#define RESIZER_NUM (2)
static struct device *s_resizer_dev[RESIZER_NUM];
static u8 s_resizer_active;

static int mtk_wrapper_resizer_poweron(uint8_t num)
{
	int ret = 0;
	int i;

	if (num > RESIZER_NUM) {
		return -EINVAL;
	}

	for (i = 0; i < num; i++) {
		ret |= mtk_resizer_power_on(s_resizer_dev[i]);
	}
	s_resizer_active = num;
	return  ret;
}

static int mtk_wrapper_resizer_poweroff(void)
{
	int i;
	int ret = 0;

	for (i = 0; i < s_resizer_active; i++) {
		ret |= mtk_resizer_power_off(s_resizer_dev[i]);
	}
	s_resizer_active = 0;
	return ret;
}

static int mtk_wrapper_resizer_start(void)
{
	int i;
	int ret = 0;

	for (i = 0; i < s_resizer_active; i++) {
		ret |= mtk_resizer_start(s_resizer_dev[i], NULL);
	}
	return ret;
}

static int mtk_wrapper_resizer_stop(void)
{
	int i;
	int ret = 0;

	for (i = 0; i < s_resizer_active; i++) {
		ret |= mtk_resizer_stop(s_resizer_dev[i], NULL);
	}
	return ret;
}

static int mtk_wrapper_resizer_set_config(void *user)
{
	struct mtk_resizer_config config = {0};
	args_resizer_set_config args;
	int ret = 0;
	int i;

	ret = copy_from_user(&args, user, sizeof(args_resizer_set_config));
	if (ret != 0) {
		return -EFAULT;
	}

	config.in_width = args.in_width;
	config.in_height = args.in_height;
	config.out_width = args.out_width;
	config.out_height = args.out_height;

	for (i = 0; i < s_resizer_active; i++) {
		ret |= mtk_resizer_config(s_resizer_dev[i], NULL, &config);
	}
	return ret;
}

static int mtk_wrapper_resizer_bypass(void)
{
	int i;
	int ret = 0;

	for (i = 0; i < s_resizer_active; i++) {
		ret |= mtk_resizer_bypass(s_resizer_dev[i], NULL);
	}
	return ret;
}

static long mtk_wrapper_resizer_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case RESIZER_POWER_ON:
		ret = mtk_wrapper_resizer_poweron((uint8_t)arg);
		break;
	case RESIZER_POWER_OFF:
		ret = mtk_wrapper_resizer_poweroff();
		break;
	case RESIZER_START:
		ret = mtk_wrapper_resizer_start();
		break;
	case RESIZER_STOP:
		ret = mtk_wrapper_resizer_stop();
		break;
	case RESIZER_SET_CONFIG:
		ret = mtk_wrapper_resizer_set_config((void *)arg);
		break;
	case RESIZER_BYPASS:
		ret = mtk_wrapper_resizer_bypass();
		break;
	default:
		pr_err("resizer_ioctl: unknown command(%d)\n", cmd);
		return -EINVAL;
	}

	return ret;
}

static const struct file_operations mtk_wrapper_resizer_fops = {
	.unlocked_ioctl = mtk_wrapper_resizer_ioctl
};

int init_module_mtk_wrapper_resizer(void)
{
	int ret;
	int i;

	pr_info("%s start\n", __func__);
	ret = register_chrdev(SCE_KM_CDEV_MAJOR_MTK_WRAPPER_RESIZER, SCE_KM_RESIZER_DEVICE_NAME, &mtk_wrapper_resizer_fops);
	if (ret < 0) {
		pr_err("log: alloc_chrdev(%s) failed(%d).\n", SCE_KM_RESIZER_DEVICE_NAME, ret);
		return ret;
	}

	for (i = 0; i < RESIZER_NUM; i++) {
		ret = mtk_wrapper_display_pipe_probe("mediatek,resizer", i, &s_resizer_dev[i]);
		if (ret < 0) {
			pr_err("can't find display_pipe dsi\n");
			return ret;
		}
	}

	return 0;
}

void cleanup_module_mtk_wrapper_resizer(void)
{
}
