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
#include <soc/mediatek/mtk_rdma.h>
#include <soc/mediatek/mtk_rbfc.h>

#include <uapi/drm/drm_fourcc.h>

#include <sce_km_defs.h>

#include <mtk_wrapper_common.h>
#include <mtk_wrapper_rdma.h>
#include "mtk_wrapper_util.h"

#define RDMA_TIMEOUT 100  //100 ms

struct rdma_dev_info {
	enum rdma_device_id id;
	bool running;
	bool rbfc_enable;
	u32 pvric_header_offset;
	struct mtk_wrapper_dma_info dma_info[RDMA_MAX_BUFFER_NUM];
	struct device *dev;
	struct device *rbfc;
	wait_queue_head_t wait_queue;
	int buffer_num;
	int buffer_next;
	u32 status;
};

static struct device *s_pvric_rdma;

static u32 mtk_wrapper_convert_format(MTK_WRAPPER_FRAME_FORMAT format)
{
	int ret;

	switch (format) {
	case MTK_WRAPPER_FRAME_FORMAT_RGB:
		ret = DRM_FORMAT_RGB888;
		break;
	case MTK_WRAPPER_FRAME_FORMAT_Y_ONLY:
		ret = DRM_FORMAT_R8;
		break;
	default:
		ret = UINT_MAX;
		break;
	}
	return ret;
}

static u8 convert_rdma_type(enum MTK_WRAPPER_RDMA_TYPE type)
{
	u8 ret;

	switch (type) {
	case MTK_WRAPPER_RDMA_TYPE_MDP:
		ret = RDMA_TYPE_MDP;
		break;
	case MTK_WRAPPER_RDMA_TYPE_PVRIC:
		ret = RDMA_TYPE_PVRIC;
		break;
	case MTK_WRAPPER_RDMA_TYPE_DISP:
		ret = RDMA_TYPE_DISP;
		break;
	default:
		pr_err("unknown rdma type(%d) treat as TYPE_MDP\n", type);
		ret = RDMA_TYPE_MDP;
		break;
	}
	return ret;
}

static void rdma_cb_func(struct mtk_mmsys_cb_data *data)
{
	/* call from interrupt */
	struct rdma_dev_info *dev_info = (struct rdma_dev_info *)data->priv_data;
	struct mtk_rdma_src src_addr[4] = {0};

	int buffer_next = dev_info->buffer_next;

	if ((unsigned)buffer_next < dev_info->buffer_num) {
		src_addr[0].mem_addr[0] = src_addr[1].mem_addr[0]
			= dev_info->dma_info[buffer_next].dma_addr + dev_info->pvric_header_offset;
		mtk_rdma_config_srcaddr(dev_info->dev, NULL, src_addr);
		dev_info->buffer_next = -1;
	}
	wake_up_interruptible(&dev_info->wait_queue);
}

static int mtk_wrapper_rdma_poweron(struct rdma_dev_info *dev_info)
{
	return mtk_rdma_power_on(dev_info->dev);
}

static int mtk_wrapper_rdma_poweroff(struct rdma_dev_info *dev_info)
{
	return mtk_rdma_power_off(dev_info->dev);
}

static int mtk_wrapper_rdma_reset(struct rdma_dev_info *dev_info)
{
	return mtk_rdma_reset(dev_info->dev);
}

static int mtk_wrapper_rdma_start(struct rdma_dev_info *dev_info)
{
	int ret;

	dev_info->running = true;
	dev_info->buffer_next = -1;
	mtk_rdma_register_cb(dev_info->dev, rdma_cb_func, MTK_RDMA_FRAME_COMPLETE, dev_info);

	ret = mtk_rdma_start(dev_info->dev, NULL);
	if (ret != 0) {
		pr_err("RDMA start error");
		return ret;
	}

	return ret;
}

static int mtk_wrapper_rdma_stop(struct rdma_dev_info *dev_info)
{
	int ret;

	ret = mtk_rdma_stop(dev_info->dev, NULL);
	dev_info->running = false;
	dev_info->buffer_next = -1;

	if (dev_info->rbfc_enable) {
		ret = mtk_rbfc_start_mode(dev_info->rbfc, NULL, MTK_RBFC_DISABLE_MODE);
		if (ret) {
			pr_err("mtk_rbfc_start_mode error %d\n", ret);
			return ret;
		}
		ret = mtk_rbfc_finish(dev_info->rbfc);
		if (ret) {
			pr_err("mtk_rbfc_finish error %d\n", ret);
			return ret;
		}
		ret = mtk_rbfc_power_off(dev_info->rbfc);
		if (ret < 0) {
			return ret;
		}
		dev_info->rbfc_enable = false;
	}

	wake_up_interruptible(&dev_info->wait_queue);
	return ret;
}

static int mtk_wrapper_rdma_config_frame(struct rdma_dev_info *dev_info, void *user)
{
	struct args_rdma_set_config args;
	int ret;
	u64 addr;
	u32 pitch;
	int i;
	struct mtk_rdma_config *rdma_configs[2];
	struct mtk_rdma_config config[2];

	ret = copy_from_user(&args, user, sizeof(args));
	if (ret != 0) {
		return -EFAULT;
	}
	if ((unsigned)args.buffer_num > RDMA_MAX_BUFFER_NUM) {
		return -EINVAL;
	}

	dev_info->buffer_num  = args.buffer_num;
	dev_info->buffer_next = -1;
	dev_info->pvric_header_offset = args.pvric_header_offset;
	if (!args.use_sram) {
		for (i = 0; i < dev_info->buffer_num; i++) {
			mtk_wrapper_ionfd_to_dmainfo(dev_info->dev, args.ionfd[i], &dev_info->dma_info[i]);
		}
	}

	pitch = args.pitch;
	memset(&rdma_configs, 0, sizeof(rdma_configs));

	config[0].format = mtk_wrapper_convert_format(args.format);
	if (config[0].format == UINT_MAX) {
		return -EINVAL;
	}
	config[0].y_pitch = pitch;
	config[0].width = args.width;
	config[0].height = args.height;
	config[0].sync_smi_channel = false;

	addr = dev_info->dma_info[0].dma_addr + dev_info->pvric_header_offset;
	config[0].mem_addr[0] = addr;
	config[0].mem_addr[1] = 0;

	rdma_configs[0] = &config[0];
	memcpy(&config[1], &config[0], sizeof(struct mtk_rdma_config));
	config[1].mem_addr[0] = dev_info->dma_info[0].dma_addr;
	rdma_configs[1] = &config[1];

	ret = mtk_rdma_config_frame(dev_info->dev, 1, NULL, rdma_configs);
	if (ret < 0) {
		return ret;
	}

	if (args.rbfc_setting) {
		u32 rdthd[1] = {0x20};
		u32 wrthd[1] = {0x1};
		u32 active[2] = {0xF, 0xF};
		dma_addr_t left_side_addr;
		dma_addr_t right_side_addr;
		int height;
		enum MTK_RBFC_TARGET target;
		enum MTK_RBFC_MODE   mode;

		left_side_addr  = config[0].mem_addr[0];
		right_side_addr = config[1].mem_addr[0];
		height = config[0].height;
		target = MTK_RBFC_TO_DRAM;
		mode   = MTK_RBFC_DISABLE_MODE;

		ret = mtk_rbfc_power_on(dev_info->rbfc);
		if (ret < 0) {
			return ret;
		}
		ret = mtk_rbfc_set_plane_num(dev_info->rbfc, 1);
		if (ret < 0) {
			return ret;
		}
		ret = mtk_rbfc_set_target(dev_info->rbfc, NULL, target);
		if (ret < 0) {
			return ret;
		}
		ret = mtk_rbfc_set_read_thd(dev_info->rbfc, NULL, rdthd);
		if (ret < 0) {
			return ret;
		}
		ret = mtk_rbfc_set_write_thd(dev_info->rbfc, NULL, wrthd);
		if (ret < 0) {
			return ret;
		}
		ret = mtk_rbfc_set_active_num(dev_info->rbfc, NULL, active);
		if (ret < 0) {
			return ret;
		}
		ret = mtk_rbfc_set_ring_buf_multi(dev_info->rbfc, NULL, 0, left_side_addr, pitch, height);
		if (ret < 0) {
			return ret;
			}
		ret = mtk_rbfc_set_2nd_ring_buf_multi(dev_info->rbfc, NULL, 0, right_side_addr, pitch, height);
		if (ret < 0) {
			return ret;
		}
		ret = mtk_rbfc_set_region_multi(dev_info->rbfc, NULL, 0, args.width, args.height);
		if (ret < 0) {
			return ret;
		}
		ret = mtk_rbfc_start_mode(dev_info->rbfc, NULL, mode);
		if (ret < 0) {
			return ret;
		}
		dev_info->rbfc_enable = true;
	}
	return ret;
}

static int mtk_wrapper_rdma_larb_get(struct rdma_dev_info *dev_info, bool is_sram)
{
	return mtk_rdma_larb_get(dev_info->dev, is_sram);
}

static int mtk_wrapper_rdma_larb_put(struct rdma_dev_info *dev_info, bool is_sram)
{
	return mtk_rdma_larb_put(dev_info->dev, is_sram);
}

static int mtk_wrapper_rdma_type_sel(struct rdma_dev_info *dev_info, enum MTK_WRAPPER_RDMA_TYPE type)
{
	if (type >= MTK_WRAPPER_RDMA_TYPE_MAX) {
		return -EINVAL;
	}
	/* sel_sram target fixed pvric */
	return mtk_rdma_sel_sram(s_pvric_rdma, NULL, convert_rdma_type(type));
}

static int mtk_wrapper_rdma_update_wp(struct rdma_dev_info *dev_info, int buf_id)
{
	long ret;
	if (!dev_info->running) {
		return 0;
	}
	if ((unsigned)buf_id >= dev_info->buffer_num) {
		return -EINVAL;
	}
	dev_info->buffer_next = buf_id;
	dev_info->status = 0;
	ret = wait_event_interruptible_timeout(dev_info->wait_queue,
					   (dev_info->buffer_next < 0 || dev_info->status != 0),
					   msecs_to_jiffies(RDMA_TIMEOUT));
	if (ret == 0) {
		return -ETIMEDOUT;
	}

	if (ret < 0) {
		return ret;
	}
	return 0;
}

static int mtk_wrapper_rdma_open(struct inode *inode, struct file *file)
{
	unsigned int minor = iminor(inode);
	struct rdma_dev_info *rdma_info;
	int ret;

	if (minor >= RDMA_ID_MAX) {
		return -ENODEV;
	}

	rdma_info = kzalloc(sizeof(*rdma_info), GFP_KERNEL);
	if (!rdma_info) {
		return -ENOMEM;
	}
	rdma_info->id = minor;
	ret = mtk_wrapper_display_pipe_probe("mediatek,rdma", minor, &rdma_info->dev);
	if (ret < 0) {
		pr_err("can't find display_pipe rdma(%d)\n", minor);
		kfree(rdma_info);
		return ret;
	}

	if (minor == RDMA0) {
		ret = mtk_wrapper_display_pipe_probe("mediatek,rbfc", 0, &rdma_info->rbfc);
		if (ret < 0) {
			pr_err("can't find display_pipe rbfc(%d)\n", 0);
			kfree(rdma_info);
			return ret;
		}
	}
	rdma_info->rbfc_enable = false;
	init_waitqueue_head(&rdma_info->wait_queue);
	file->private_data = rdma_info;
	return 0;
}

static long mtk_wrapper_rdma_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case RDMA_POWER_ON:
		ret = mtk_wrapper_rdma_poweron((struct rdma_dev_info *)file->private_data);
		break;
	case RDMA_POWER_OFF:
		ret = mtk_wrapper_rdma_poweroff((struct rdma_dev_info *)file->private_data);
		break;
	case RDMA_RESET:
		ret = mtk_wrapper_rdma_reset((struct rdma_dev_info *)file->private_data);
		break;
	case RDMA_START:
		ret = mtk_wrapper_rdma_start((struct rdma_dev_info *)file->private_data);
		break;
	case RDMA_STOP:
		ret = mtk_wrapper_rdma_stop((struct rdma_dev_info *)file->private_data);
		break;
	case RDMA_CONFIG_FRAME:
		ret = mtk_wrapper_rdma_config_frame((struct rdma_dev_info *)file->private_data, (void *)arg);
		break;
	case RDMA_LARB_GET:
		ret = mtk_wrapper_rdma_larb_get((struct rdma_dev_info *)file->private_data, (bool)arg);
		break;
	case RDMA_LARB_PUT:
		ret = mtk_wrapper_rdma_larb_put((struct rdma_dev_info *)file->private_data, (bool)arg);
		break;
	case RDMA_TYPE_SEL:
		ret = mtk_wrapper_rdma_type_sel((struct rdma_dev_info *)file->private_data, (enum MTK_WRAPPER_RDMA_TYPE)arg);
		break;
	case RDMA_UPDATE_WP:
		ret = mtk_wrapper_rdma_update_wp((struct rdma_dev_info *)file->private_data, (int)arg);
		break;
	default:
		pr_err("rdma_ioctl: unknown command(%d)\n", cmd);
		break;
	}
	return ret;
}

static const struct file_operations mtk_wrapper_rdma_fops = {
	.open           = mtk_wrapper_rdma_open,
	.unlocked_ioctl = mtk_wrapper_rdma_ioctl,
};

int init_module_mtk_wrapper_rdma(void)
{
	int ret;

	pr_info("%s start\n", __func__);
	ret = register_chrdev(SCE_KM_CDEV_MAJOR_MTK_WRAPPER_RDMA, SCE_KM_RDMA_DEVICE_NAME, &mtk_wrapper_rdma_fops);
	if (ret < 0) {
		pr_err("log: alloc_chrdev(%s) failed(%d).\n", SCE_KM_RDMA_DEVICE_NAME, ret);
		return ret;
	}

	ret = mtk_wrapper_display_pipe_probe("mediatek,rdma", 1, &s_pvric_rdma);
	if (ret < 0) {
		pr_err("can't find display_pipe rdma-pvric\n");
		return ret;
	}

	return 0;
}

void cleanup_module_mtk_wrapper_rdma(void)
{
}
