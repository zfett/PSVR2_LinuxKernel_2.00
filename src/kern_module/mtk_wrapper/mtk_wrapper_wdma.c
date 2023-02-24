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
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <soc/mediatek/mtk_wdma.h>

#include <sce_km_defs.h>

#include <mtk_wrapper_wdma.h>
#include "mtk_wrapper_util.h"

#define WDMA_DEVICE_NUM (4)
#define WAIT_SYNC_TIMEOUT (msecs_to_jiffies(100))

struct wdma_buf_info {
	u32 pitch;
	u32 format;
	u32 pvric_header_offset;
	struct mtk_wrapper_dma_info dma_info[WDMA_MAX_BUFFER_NUM];
};

struct wdma_dev_info {
	struct device *dev[WDMA_DEVICE_NUM];
	unsigned int active_num;
	unsigned int buffer_last;
	unsigned int buffer_wp;
	unsigned int buffer_num;
	struct wdma_buf_info buf;
	wait_queue_head_t wait_stop_queue;
	int wait_stop_flag;
};

static struct wdma_dev_info s_wdma_dev_info;

static int wdma_update_buffer_address(struct wdma_dev_info *dev_info)
{
	int ret, i;
	struct wdma_buf_info *buf_info = &dev_info->buf;
	dma_addr_t addr = buf_info->dma_info[s_wdma_dev_info.buffer_wp].dma_addr + buf_info->pvric_header_offset;

	for (ret = i = 0; i < dev_info->active_num; i++) {
		ret |= mtk_wdma_set_out_buf(dev_info->dev[i], NULL, addr, buf_info->pitch, buf_info->format);
	}

	i = dev_info->buffer_wp;
	if (++i >= dev_info->buffer_num) {
		i = 0;
	}
	dev_info->buffer_wp = i;
	smp_mb();
	return ret;
}

static void wdma_cb_func(struct mtk_mmsys_cb_data *data)
{
	struct wdma_dev_info *dev_info = data->priv_data;
	/* update wp in interrupt -> current wp buffer draw while next V */
	if (dev_info->buffer_last == dev_info->buffer_wp) {
		wdma_update_buffer_address(dev_info);
	}
	s_wdma_dev_info.wait_stop_flag = 0;
	wake_up_interruptible(&s_wdma_dev_info.wait_stop_queue);
}

static int mtk_wrapper_wdma_poweron(u8 num)
{
	int ret = 0;
	int i;

	if (num > WDMA_DEVICE_NUM) {
		return -EINVAL;
	}

	for (i = 0; i < num; i++) {
		ret |= mtk_wdma_power_on(s_wdma_dev_info.dev[i]);
		ret |= mtk_wdma_set_out_mode(s_wdma_dev_info.dev[i], MTK_WDMA_OUT_MODE_DRAM);
	}
	s_wdma_dev_info.active_num = num;

	return ret;
}

static int mtk_wrapper_wdma_poweroff(void)
{
	int ret = 0;
	int i;

	for (i = 0; i < s_wdma_dev_info.active_num; i++) {
		ret |= mtk_wdma_power_off(s_wdma_dev_info.dev[i]);
	}
	s_wdma_dev_info.active_num = 0;

	return ret;
}

static int mtk_wrapper_wdma_reset(void)
{
	int ret = 0;
	int i;

	for (i = 0; i < s_wdma_dev_info.active_num; i++) {
		ret |= mtk_wdma_reset(s_wdma_dev_info.dev[i]);
	}
	return ret;
}

static int mtk_wrapper_wdma_start(void)
{
	int i;
	int ret = 0;

	if (s_wdma_dev_info.active_num == 1) {
		mtk_wdma_register_cb(s_wdma_dev_info.dev[0], wdma_cb_func,
				     0x01, /* frame complete interrupt */
				     &s_wdma_dev_info);
	} else {
		/* cancel callback */
		mtk_wdma_register_cb(s_wdma_dev_info.dev[0], NULL,
				     0x00,
				     NULL);
	}
	for (i = 0; i < s_wdma_dev_info.active_num; i++) {
		mtk_wdma_bypass_shadow_disable(s_wdma_dev_info.dev[i], true);
		ret |= mtk_wdma_start(s_wdma_dev_info.dev[i], NULL);
	}
	return ret;
}

static int mtk_wrapper_wdma_stop(void *user)
{
	args_wdma_stop args;
	int i;
	int ret = 0;

	ret = copy_from_user(&args, user, sizeof(args_wdma_stop));
	if (ret != 0) {
		pr_err("wdma_stop:copy_to_user error\n");
		return -EFAULT;
	}

	if (args.sync_stop) {
		s_wdma_dev_info.wait_stop_flag = 1;
		ret = wait_event_interruptible_timeout(s_wdma_dev_info.wait_stop_queue, s_wdma_dev_info.wait_stop_flag == 0, WAIT_SYNC_TIMEOUT);
		if (ret == 0) {
			pr_err("wdma_stop:wait event timeout\n");
		}
	}

	for (i = 0; i < s_wdma_dev_info.active_num; i++) {
		ret |= mtk_wdma_reset(s_wdma_dev_info.dev[i]);
		ret |= mtk_wdma_stop(s_wdma_dev_info.dev[i], NULL);
	}
	return ret;
}

static int mtk_wrapper_wdma_wait_next_sync(void)
{
	int ret = 0;

	s_wdma_dev_info.wait_stop_flag = 1;
	ret = wait_event_interruptible_timeout(s_wdma_dev_info.wait_stop_queue, s_wdma_dev_info.wait_stop_flag == 0, WAIT_SYNC_TIMEOUT);
	if (ret == 0) {
		pr_err("mtk_wrapper_wdma_wait_next_sync:wait event timeout\n");
	}
	return 0;
}

static int mtk_wrapper_wdma_set_region(void *user)
{
	args_wdma_set_region args;
	int ret;
	int i;

	ret = copy_from_user(&args, user, sizeof(args_wdma_set_region));
	if (ret != 0) {
		return -EFAULT;
	}

	for (i = 0; i < s_wdma_dev_info.active_num; i++) {
		ret |= mtk_wdma_set_region(s_wdma_dev_info.dev[i], NULL, args.in_w, args.in_h,
					   args.out_w, args.out_h, args.crop_w, args.crop_h);
	}
	return ret;
}

static int mtk_wrapper_wdma_set_out_buffer(void *user)
{
	args_wdma_set_output_buffer args;
	int ret;
	int i;

	ret = copy_from_user(&args, user, sizeof(args_wdma_set_output_buffer));
	if (ret != 0) {
		return -EFAULT;
	}

	if ((unsigned)args.buffer_num > WDMA_MAX_BUFFER_NUM) {
		return -EINVAL;
	}
	s_wdma_dev_info.buffer_num = args.buffer_num;
	s_wdma_dev_info.buffer_wp  = 0;
	s_wdma_dev_info.buffer_last = 0;

	s_wdma_dev_info.buf.pitch = args.pitch;
	s_wdma_dev_info.buf.format = args.format;
	s_wdma_dev_info.buf.pvric_header_offset = args.pvric_header_offset;

	for (i = 0; i < s_wdma_dev_info.buffer_num; i++) {
		if (args.ionfd[i] <= 0) {
			return -EINVAL;
		}
		mtk_wrapper_ionfd_to_dmainfo(s_wdma_dev_info.dev[0], args.ionfd[i],
					     &s_wdma_dev_info.buf.dma_info[i]);
	}

	if (s_wdma_dev_info.active_num == 1) {
		/* set first buffer */
		ret = wdma_update_buffer_address(&s_wdma_dev_info);
	} else {
		for (i = 0; i < s_wdma_dev_info.active_num; i++) {
			u32 offset = (args.pitch / s_wdma_dev_info.active_num) * i;

			ret |= mtk_wdma_set_multi_out_buf_addr_offset(s_wdma_dev_info.dev[i], NULL,
								      MTK_WDMA_OUT_BUF_0, offset);
			ret |= mtk_wdma_set_out_buf(s_wdma_dev_info.dev[i], NULL,
						    s_wdma_dev_info.buf.dma_info[0].dma_addr,
						    args.pitch, args.format);
		}
	}
	return ret;
}

static int mtk_wrapper_wdma_enable_pvric(void)
{
	if (s_wdma_dev_info.active_num != 1) {
		return -EINVAL;
	}

	return mtk_wdma_pvric_enable(s_wdma_dev_info.dev[0], true);
}

static int mtk_wrapper_wdma_disable_pvric(void)
{
	if (s_wdma_dev_info.active_num != 1) {
		return -EINVAL;
	}

	return mtk_wdma_pvric_enable(s_wdma_dev_info.dev[0], false);
}

static ssize_t mtk_wrapper_wdma_read(struct file *file, char __user *buff, size_t count, loff_t *pos)
{
	struct wdma_dev_info *dev_info = &s_wdma_dev_info;
	/* return next buffer id to user */
	int last_buf_id = dev_info->buffer_wp;

	if (count < sizeof(last_buf_id) ||
	    copy_to_user(buff, &last_buf_id, sizeof(last_buf_id)) != 0) {
		pr_err("wdma_read:copy_to_user error\n");
		return 0;
	}
	dev_info->buffer_last = last_buf_id;
	smp_mb();
	return sizeof(last_buf_id);
}

static long mtk_wrapper_wdma_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case WDMA_POWER_ON:
		ret = mtk_wrapper_wdma_poweron((u8)arg);
		break;
	case WDMA_POWER_OFF:
		ret = mtk_wrapper_wdma_poweroff();
		break;
	case WDMA_RESET:
		ret = mtk_wrapper_wdma_reset();
		break;
	case WDMA_START:
		ret = mtk_wrapper_wdma_start();
		break;
	case WDMA_STOP:
		ret = mtk_wrapper_wdma_stop((void *)arg);
		break;
	case WDMA_SET_REGION:
		ret = mtk_wrapper_wdma_set_region((void *)arg);
		break;
	case WDMA_SET_OUT_BUFFER:
		ret = mtk_wrapper_wdma_set_out_buffer((void *)arg);
		break;
	case WDMA_ENABLE_PVRIC:
		ret = mtk_wrapper_wdma_enable_pvric();
		break;
	case WDMA_DISABLE_PVRIC:
		ret = mtk_wrapper_wdma_disable_pvric();
		break;
	case WDMA_WAIT_NEXT_SYNC:
		ret = mtk_wrapper_wdma_wait_next_sync();
		break;
	default:
		pr_err("wdma_ioctl: unknown command(%d)\n", cmd);
		return -EINVAL;
	}

	return ret;
}

static const struct file_operations mtk_wrapper_wdma_fops = {
	.read           = mtk_wrapper_wdma_read,
	.unlocked_ioctl = mtk_wrapper_wdma_ioctl,
};

int init_module_mtk_wrapper_wdma(void)
{
	int ret;
	struct wdma_dev_info *wdma_info;
	int i;

	pr_info("%s start\n", __func__);
	ret = register_chrdev(SCE_KM_CDEV_MAJOR_MTK_WRAPPER_WDMA, SCE_KM_WDMA_DEVICE_NAME, &mtk_wrapper_wdma_fops);
	if (ret < 0) {
		pr_err("log: alloc_chrdev(%s) failed(%d).\n", SCE_KM_WDMA_DEVICE_NAME, ret);
		return ret;
	}

	wdma_info = &s_wdma_dev_info;
	memset(wdma_info, 0, sizeof(struct wdma_dev_info));

	for (i = 0; i < WDMA_DEVICE_NUM; i++) {
		ret = mtk_wrapper_display_pipe_probe("mediatek,wdma", i, &wdma_info->dev[i]);
		if (ret < 0) {
			pr_err("can't find display_pipe wdma[%d]\n", i);
			return ret;
		}
	}

	s_wdma_dev_info.wait_stop_flag = 0;
	init_waitqueue_head(&s_wdma_dev_info.wait_stop_queue);

	return 0;
}

void cleanup_module_mtk_wrapper_wdma(void)
{
}
