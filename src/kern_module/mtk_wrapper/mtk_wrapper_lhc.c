/*
 * Copyright (C) 2021 Sony Interactive Entertainment Inc.
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

#include <soc/mediatek/mtk_lhc.h>

#include <mtk_wrapper_lhc.h>
#include "mtk_wrapper_util.h"

static struct device *s_lhc;
static wait_queue_head_t s_wait_queue;
static int s_frame_done;

#define LHC_NUM (4)

static int mtk_wrapper_lhc_power_on(void)
{
	return mtk_lhc_power_on(s_lhc);
}

static int mtk_wrapper_lhc_power_off(void)
{
	return mtk_lhc_power_off(s_lhc);
}

static int mtk_wrapper_lhc_config_slice(void *user)
{
	args_lhc_config_slice args;
	int ret = 0;
	int i;
	struct mtk_lhc_slice conf[LHC_NUM];

	ret = copy_from_user(&args, user, sizeof(args_lhc_config_slice));

	if (ret != 0) {
		return -EFAULT;
	}

	for (i = 0; i < ARRAY_SIZE(conf); i++) {
		conf[i].w = args.slice[i].w;
		conf[i].h = args.slice[i].h;
		conf[i].start = args.slice[i].start;
		conf[i].end = args.slice[i].end;
	}

	ret = mtk_lhc_config_slices(s_lhc, &conf, NULL);
	return ret;
}

static void mtk_wrapper_lhc_cb(struct mtk_mmsys_cb_data *arg)
{
	s_frame_done = 1;
	wake_up_interruptible(&s_wait_queue);
}

static int mtk_wrapper_lhc_start(void)
{
	mtk_lhc_register_cb(s_lhc, mtk_wrapper_lhc_cb, LHC_CB_STA_FRAME_DONE, NULL);
	return mtk_lhc_start(s_lhc, NULL);
}

static int mtk_wrapper_lhc_stop(void)
{
	return mtk_lhc_stop(s_lhc, NULL);
}

static ssize_t mtk_wrapper_lhc_read(struct file *file, char __user *buff, size_t count, loff_t *pos)
{
	static struct {
		u8 pad[3];
		struct mtk_lhc_hist hist;
	} s;
	int ret;

	/* blocking until frame done */
	ret = wait_event_interruptible_timeout(s_wait_queue, s_frame_done != 0, msecs_to_jiffies(100));
	if (ret < 0) {
		return ret;
	} else if (ret == 0) {
		return -ETIMEDOUT;
	}

	if (count <= sizeof(struct mtk_lhc_hist)) {
		mtk_lhc_read_histogram(s_lhc, &s.hist);
		ret = copy_to_user(buff, &s.hist, sizeof(struct mtk_lhc_hist));
		if (ret != 0) {
			pr_err("LHC copy_to_use_error\n");
			s_frame_done  = 0;
			return 0;
		}
		s_frame_done  = 0;
		return sizeof(struct mtk_lhc_hist);
	}
	s_frame_done  = 0;
	return -EINVAL;
}

static long mtk_wrapper_lhc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case LHC_POWER_ON:
		ret = mtk_wrapper_lhc_power_on();
		break;
	case LHC_POWER_OFF:
		ret = mtk_wrapper_lhc_power_off();
		break;
	case LHC_CONFIG_SLICE:
		ret = mtk_wrapper_lhc_config_slice((void *)arg);
		break;
	case LHC_START:
		ret = mtk_wrapper_lhc_start();
		break;
	case LHC_STOP:
		ret = mtk_wrapper_lhc_stop();
		break;
	}
	return ret;
}

static const struct file_operations mtk_wrapper_lhc_fops = {
	.read = mtk_wrapper_lhc_read,
	.unlocked_ioctl = mtk_wrapper_lhc_ioctl
};

int init_module_mtk_wrapper_lhc(void)
{
	int ret;

	if (sizeof(mtk_wrapper_lhc_hist) != sizeof(struct mtk_lhc_hist)) {
		return -EINVAL;
	}

	pr_info("%s start\n", __func__);
	ret = register_chrdev(SCE_KM_CDEV_MAJOR_MTK_WRAPPER_LHC, SCE_KM_LHC_DEVICE_NAME, &mtk_wrapper_lhc_fops);

	if (ret < 0) {
		pr_err("log: alloc_chrdev(%s) failed(%d).\n", SCE_KM_LHC_DEVICE_NAME, ret);
		return ret;
	}

	ret = mtk_wrapper_display_pipe_probe("mediatek,lhc", 0, &s_lhc);
	if (ret < 0) {
		pr_err("can't find display_pipe lhc\n");
		return ret;
	}

	init_waitqueue_head(&s_wait_queue);

	return 0;
}

void cleanup_module_mtk_wrapper_lhc(void)
{
}
