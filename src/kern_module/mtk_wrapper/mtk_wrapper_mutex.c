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

#include <soc/mediatek/mtk_mutex.h>

#include <sce_km_defs.h>
#include "mtk_wrapper_mutex.h"
#include "mtk_wrapper_util.h"

static struct mtk_mutex_res *s_mutex_res[MMCORE_MUTEX_MAX_IDX];

static enum mtk_mutex_sof_id convert_to_mutex_sof(enum MTK_WRAPPER_MUTEX_SOF sof)
{
	enum mtk_mutex_sof_id ret = MUTEX_SOF_ID_MAX;

	switch (sof) {
	case MTK_WRAPPER_MUTEX_SOF_SINGLE:
		ret = MUTEX_MMSYS_SOF_SINGLE;
		break;
	case MTK_WRAPPER_MUTEX_SOF_DSI:
		ret = MUTEX_MMSYS_SOF_DSI0;
		break;
	case MTK_WRAPPER_MUTEX_SOF_DP:
		ret = MUTEX_MMSYS_SOF_DP;
		break;
	case MTK_WRAPPER_MUTEX_SOF_DP_DSC:
		ret = MUTEX_MMSYS_SOF_DP_DSC;
		break;
	case MTK_WRAPPER_MUTEX_SOF_SYNC_DELAY:
		ret = MUTEX_MMSYS_SOF_SYNC_DELAY0;
		break;
	default:
		ret = MUTEX_SOF_ID_MAX;
		break;
	}
	return ret;
}

static enum mtk_mutex_comp_id convert_to_mutex_comp_id(DISPLAY_COMPONENT comp, u8 *device_num)
{
	enum mtk_mutex_comp_id ret = MUTEX_COMPONENT_ID_MAX;

	*device_num = 4;
	switch (comp) {
	case DISPLAY_COMPONENT_DSI:
		ret = MUTEX_COMPONENT_DSI0;
		break;
	case DISPLAY_COMPONENT_DSC:
		ret = MUTEX_COMPONENT_DSC0;
		break;
	case DISPLAY_COMPONENT_LHC:
		ret = MUTEX_COMPONENT_LHC0;
		break;
	case DISPLAY_COMPONENT_WDMA:
		ret = MUTEX_COMPONENT_DISP_WDMA0;
		break;
	case DISPLAY_COMPONENT_MDP_RDMA:
		ret = MUTEX_COMPONENT_MDP_RDMA0;
		break;
	case DISPLAY_COMPONENT_PVRIC_RDMA:
		ret = MUTEX_COMPONENT_MDP_RDMA_PVRIC0;
		break;
	case DISPLAY_COMPONENT_DISP_RDMA:
		ret = MUTEX_COMPONENT_DISP_RDMA0;
		break;
	case DISPLAY_COMPONENT_SLICER_DSC:
	case DISPLAY_COMPONENT_SLICER_VID:
		ret = MUTEX_COMPONENT_SLICER;
		*device_num = 1;
		break;
	case DISPLAY_COMPONENT_CROP:
		ret = MUTEX_COMPONENT_CROP0;
		break;
	case DISPLAY_COMPONENT_RESIZER:
		ret = MUTEX_COMPONENT_RSZ0;
		*device_num = 2;
		break;
	case DISPLAY_COMPONENT_P2S:
		ret = MUTEX_COMPONENT_P2D0;
		*device_num = 1;
		break;
	default:
		ret = MUTEX_COMPONENT_ID_MAX;
		*device_num = 0;
		break;
	}
	return ret;
}

static int mtk_wrapper_mutex_get_resource(enum MTK_WRAPPER_MUTEX_INDEX idx)
{
	struct device *dev;
	int ret;

	ret = mtk_wrapper_display_pipe_probe("mediatek,mutex", 0, &dev);
	if (ret < 0) {
		pr_err("can't find display_pipe mutex\n");
		return ret;
	}

	if ((uint32_t)idx >= MMCORE_MUTEX_MAX_IDX) {
		return -EINVAL;
	}

	s_mutex_res[idx] = mtk_mutex_get(dev, idx);
	if (!s_mutex_res[idx]) {
		return -EINVAL;
	}

	return 0;
}

static int mtk_wrapper_mutex_put_resource(enum MTK_WRAPPER_MUTEX_INDEX idx)
{
	if ((uint32_t)idx >= MMCORE_MUTEX_MAX_IDX) {
		return -EINVAL;
	}
	mtk_mutex_put(s_mutex_res[idx]);
	s_mutex_res[idx] = NULL;

	return 0;
}

static int mtk_wrapper_mutex_power_on(enum MTK_WRAPPER_MUTEX_INDEX idx)
{
	if ((uint32_t)idx >= MMCORE_MUTEX_MAX_IDX) {
		return -EINVAL;
	}

	return mtk_mutex_power_on(s_mutex_res[idx]);
}

static int mtk_wrapper_mutex_power_off(enum MTK_WRAPPER_MUTEX_INDEX idx)
{
	if ((uint32_t)idx >= MMCORE_MUTEX_MAX_IDX) {
		return -EINVAL;
	}
	return mtk_mutex_power_off(s_mutex_res[idx]);
}

static int mtk_wrapper_mutex_set_config(void *user)
{
	int ret;
	struct args_mutex_set_config args;
	bool only_vsync;
	enum mtk_mutex_sof_id sof;

	ret = copy_from_user(&args, user, sizeof(args));
	if (ret != 0) {
		return -EFAULT;
	}

	if (args.sof_component == MTK_WRAPPER_MUTEX_SOF_DP ||
	    args.sof_component == MTK_WRAPPER_MUTEX_SOF_DP_DSC) {
		only_vsync = true;
	} else {
		only_vsync = false;
	}

	if ((uint32_t)args.idx >= MMCORE_MUTEX_MAX_IDX) {
		return -EINVAL;
	}

	sof = convert_to_mutex_sof(args.sof_component);
	if (sof == MUTEX_SOF_ID_MAX) {
		return -EINVAL;
	}

	ret |= mtk_mutex_select_sof(s_mutex_res[args.idx], sof, NULL, only_vsync);
	return ret;
}

static int mtk_wrapper_mutex_add_component(void *user)
{
	int ret;
	int i;
	struct args_mutex_add_component args;
	enum mtk_mutex_comp_id id;
	u8 device_num = 0;

	ret = copy_from_user(&args, user, sizeof(args));
	if (ret != 0) {
		return -EFAULT;
	}
	id = convert_to_mutex_comp_id(args.component, &device_num);
	if (id == MUTEX_COMPONENT_ID_MAX) {
		return -EINVAL;
	}
	if ((uint32_t)args.idx >= MMCORE_MUTEX_MAX_IDX) {
		return -EINVAL;
	}

	for (i = 0; i < device_num; i++) {
		ret = mtk_mutex_add_comp(s_mutex_res[args.idx], id + i, NULL);
		if (ret != 0)
			return ret;
		if (args.component == DISPLAY_COMPONENT_MDP_RDMA ||
		    args.component == DISPLAY_COMPONENT_DISP_RDMA ||
		    args.component == DISPLAY_COMPONENT_PVRIC_RDMA) {
			ret = mtk_mutex_add_comp(s_mutex_res[args.idx], MUTEX_COMPONENT_RBFC0 + i, NULL);
			if (ret != 0)
				return ret;
		}
	}

	return ret;
}

static int mtk_wrapper_mutex_remove_component(void *user)
{
	int ret;
	struct args_mutex_remove_component args;
	int i;
	enum mtk_mutex_comp_id id;
	u8 device_num = 0;

	ret = copy_from_user(&args, user, sizeof(args));
	if (ret != 0) {
		return -EFAULT;
	}
	id = convert_to_mutex_comp_id(args.component, &device_num);
	if (id == MUTEX_COMPONENT_ID_MAX) {
		return -EINVAL;
	}
	if ((u32)args.idx >= MMCORE_MUTEX_MAX_IDX) {
		return -EINVAL;
	}

	for (i = 0; i < device_num; i++) {
		ret = mtk_mutex_remove_comp(s_mutex_res[args.idx], id + i, NULL);
		if (ret != 0) {
			return ret;
		}
		if (args.component == DISPLAY_COMPONENT_MDP_RDMA ||
		    args.component == DISPLAY_COMPONENT_DISP_RDMA ||
		    args.component == DISPLAY_COMPONENT_PVRIC_RDMA) {
			ret = mtk_mutex_remove_comp(s_mutex_res[args.idx], MUTEX_COMPONENT_RBFC0 + i, NULL);
			if (ret != 0) {
				return ret;
			}
		}
	}

	return ret;
}

static int mtk_wrapper_mutex_enable(void *user)
{
	int ret;
	struct args_mutex_enable args;

	ret = copy_from_user(&args, user, sizeof(args));
	if (ret != 0) {
		return -EFAULT;
	}

	if ((uint32_t)args.idx >= MMCORE_MUTEX_MAX_IDX) {
		return -EINVAL;
	}

	if (args.enable) {
		ret = mtk_mutex_enable(s_mutex_res[args.idx], NULL);
	} else {
		ret = mtk_mutex_disable(s_mutex_res[args.idx], NULL);
	}

	return ret;
}

static int mtk_wrapper_mutex_open(struct inode *inode, struct file *file)
{
	return 0;
}

static long mtk_wrapper_mutex_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case MUTEX_GET_RESOURCE:
		ret = mtk_wrapper_mutex_get_resource((enum MTK_WRAPPER_MUTEX_INDEX)arg);
		break;
	case MUTEX_PUT_RESOURCE:
		ret = mtk_wrapper_mutex_put_resource((enum MTK_WRAPPER_MUTEX_INDEX)arg);
		break;
	case MUTEX_SET_CONFIG:
		ret = mtk_wrapper_mutex_set_config((void *)arg);
		break;
	case MUTEX_ADD_COMPONENT:
		ret = mtk_wrapper_mutex_add_component((void *)arg);
		break;
	case MUTEX_REMOVE_COMPONENT:
		ret = mtk_wrapper_mutex_remove_component((void *)arg);
		break;
	case MUTEX_POWER_ON:
		ret = mtk_wrapper_mutex_power_on((enum MTK_WRAPPER_MUTEX_INDEX)arg);
		break;
	case MUTEX_POWER_OFF:
		ret = mtk_wrapper_mutex_power_off((enum MTK_WRAPPER_MUTEX_INDEX)arg);
		break;
	case MUTEX_ENABLE:
		ret = mtk_wrapper_mutex_enable((void *)arg);
		break;
	default:
		pr_err("mutex_ioctl: unknown command(%d)\n", cmd);
		return -EINVAL;
	}

	return ret;
}

static const struct file_operations mtk_wrapper_mutex_fops = {
	.open           = mtk_wrapper_mutex_open,
	.unlocked_ioctl = mtk_wrapper_mutex_ioctl
};

int init_module_mtk_wrapper_mutex(void)
{
	int ret;

	pr_info("%s start\n", __func__);
	ret = register_chrdev(SCE_KM_CDEV_MAJOR_MTK_WRAPPER_MUTEX, SCE_KM_MUTEX_DEVICE_NAME, &mtk_wrapper_mutex_fops);
	if (ret < 0) {
		pr_err("log: alloc_chrdev(%s) failed(%d).\n", SCE_KM_MUTEX_DEVICE_NAME, ret);
		return ret;
	}

	return 0;
}

void cleanup_module_mtk_wrapper_mutex(void)
{
}
