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
#include <linux/gcd.h>
#include <soc/mediatek/mtk_mmsys_cfg.h>
#include <dt-bindings/gce/mt3612-gce.h>

#include <sce_km_defs.h>

#include "mtk_wrapper_util.h"
#include "mtk_wrapper_mmsyscfg.h"

static struct device *s_mmsyscfg_dev;

static struct cmdq_client *s_cmdq_client[MTK_WRAPPER_MMSYSCFG_CAMERA_MAX];

static enum mtk_mmsys_config_comp_id convert_display_component_to_comp_id(DISPLAY_COMPONENT disp_comp)
{
	enum mtk_mmsys_config_comp_id id;

	switch (disp_comp) {
	case DISPLAY_COMPONENT_DP:
		id = MMSYSCFG_COMPONENT_DP_PAT_GEN0;
		break;
	case DISPLAY_COMPONENT_SLICER_VID:
		id = MMSYSCFG_COMPONENT_SLICER_VID;
		break;
	case DISPLAY_COMPONENT_SLICER_DSC:
		id = MMSYSCFG_COMPONENT_SLICER_DSC;
		break;
	case DISPLAY_COMPONENT_WDMA:
		id = MMSYSCFG_COMPONENT_DISP_WDMA0;
		break;
	case DISPLAY_COMPONENT_MDP_RDMA:
		id = MMSYSCFG_COMPONENT_MDP_RDMA0;
		break;
	case DISPLAY_COMPONENT_DISP_RDMA:
		id = MMSYSCFG_COMPONENT_DISP_RDMA0;
		break;
	case DISPLAY_COMPONENT_PVRIC_RDMA:
		id = MMSYSCFG_COMPONENT_MDP_RDMA_PVRIC0;
		break;
	case DISPLAY_COMPONENT_LHC:
		id = MMSYSCFG_COMPONENT_LHC0;
		break;
	case DISPLAY_COMPONENT_DSI:
		id = MMSYSCFG_COMPONENT_DSI_LANE_SWAP;
		break;
	case DISPLAY_COMPONENT_DSC:
		id = MMSYSCFG_COMPONENT_DSC0;
		break;
	case DISPLAY_COMPONENT_P2S:
		id = MMSYSCFG_COMPONENT_P2S0;
		break;
	case DISPLAY_COMPONENT_CROP:
		id = MMSYSCFG_COMPONENT_CROP0;
		break;
	default:
		pr_info("unknown component %d\n", disp_comp);
		id = MMSYSCFG_COMPONENT_ID_MAX;
		break;
	}
	return id;
}

static enum mtk_mmsys_slicer_to_ddds_sel convert_ddds_sel(DISPLAY_COMPONENT disp_comp)
{
	enum mtk_mmsys_slicer_to_ddds_sel ret = MMSYSCFG_SLCR_TO_DDDS_MAX;

	switch (disp_comp) {
	case DISPLAY_COMPONENT_SLICER_VID:
		ret = MMSYSCFG_SLCR_VID_TO_DDDS;
		break;
	case DISPLAY_COMPONENT_SLICER_DSC:
		ret = MMSYSCFG_SLCR_DSC_TO_DDDS;
		break;
	default:
		pr_err("[%s] Unhandled component(%d)", __func__, disp_comp);
		break;
	}
	return ret;
}

static int mtk_wrapper_mmsyscfg_connect_component(void *user)
{
	args_mmsyscfg_connect_component args;
	int ret = 0;
	int cur;
	int next;
	int i;
	int cur_offset;
	int next_offset;

	ret = copy_from_user(&args, user, sizeof(args_mmsyscfg_connect_component));
	if (ret != 0) {
		return -EFAULT;
	}

	switch (args.cur) {
	case DISPLAY_COMPONENT_DSC:
		cur_offset = 1;
		break;
	case DISPLAY_COMPONENT_DSI:
	case DISPLAY_COMPONENT_SLICER_DSC:
	case DISPLAY_COMPONENT_SLICER_VID:
		cur_offset = 4;
		break;
	default:
		cur_offset = 0;
		break;
	}

	switch (args.next) {
	case DISPLAY_COMPONENT_DSC:
		next_offset = 1;
		break;
	case DISPLAY_COMPONENT_DSI:
		next_offset = 4;
		break;
	default:
		next_offset = 0;
		break;
	}

	if (args.cur == DISPLAY_COMPONENT_RESIZER && args.next == DISPLAY_COMPONENT_P2S) {
		ret = mtk_mmsys_cfg_connect_comp(s_mmsyscfg_dev, MMSYSCFG_COMPONENT_RSZ1, MMSYSCFG_COMPONENT_P2S0, NULL);
	} else if (args.cur == DISPLAY_COMPONENT_P2S && args.next == DISPLAY_COMPONENT_WDMA) {
		ret = mtk_mmsys_cfg_connect_comp(s_mmsyscfg_dev, MMSYSCFG_COMPONENT_P2S0, MMSYSCFG_COMPONENT_DISP_WDMA0, NULL);
	} else if (args.cur == DISPLAY_COMPONENT_CROP && args.next == DISPLAY_COMPONENT_WDMA) {
		ret = mtk_mmsys_cfg_connect_comp(s_mmsyscfg_dev, MMSYSCFG_COMPONENT_P2S0, MMSYSCFG_COMPONENT_DISP_WDMA0, NULL);
		ret |= mtk_mmsys_cfg_connect_comp(s_mmsyscfg_dev, MMSYSCFG_COMPONENT_RSZ1, MMSYSCFG_COMPONENT_DISP_WDMA1, NULL);
		ret |= mtk_mmsys_cfg_connect_comp(s_mmsyscfg_dev, MMSYSCFG_COMPONENT_CROP2, MMSYSCFG_COMPONENT_DISP_WDMA2, NULL);
		ret |= mtk_mmsys_cfg_connect_comp(s_mmsyscfg_dev, MMSYSCFG_COMPONENT_CROP3, MMSYSCFG_COMPONENT_DISP_WDMA3, NULL);
	} else {
		for (i = 0; i < 4; i++) {
			cur =  (int)convert_display_component_to_comp_id(args.cur) + (i >> cur_offset);
			next = (int)convert_display_component_to_comp_id(args.next) + (i >> next_offset);
			if (cur == MMSYSCFG_COMPONENT_ID_MAX || next == MMSYSCFG_COMPONENT_ID_MAX) {
				return -EINVAL;
			}
			ret = mtk_mmsys_cfg_connect_comp(s_mmsyscfg_dev,
							 cur, next,
							 NULL);
			pr_debug("ConnectComponent %d->%d (%d)\n", cur, next, ret);
		}
	}
	return ret;
}

static int mtk_wrapper_mmsyscfg_disconnect_component(void *user)
{
	args_mmsyscfg_disconnect_component args;
	int ret = 0;
	int cur;
	int next;
	int i;
	int cur_offset;
	int next_offset;

	ret = copy_from_user(&args, user, sizeof(args_mmsyscfg_disconnect_component));
	if (ret != 0) {
		return -EFAULT;
	}

	switch (args.cur) {
	case DISPLAY_COMPONENT_DSC:
		cur_offset = 1;
		break;
	case DISPLAY_COMPONENT_DSI:
	case DISPLAY_COMPONENT_SLICER_DSC:
	case DISPLAY_COMPONENT_SLICER_VID:
		cur_offset = 4;
		break;
	default:
		cur_offset = 0;
		break;
	}

	switch (args.next) {
	case DISPLAY_COMPONENT_DSC:
		next_offset = 1;
		break;
	case DISPLAY_COMPONENT_DSI:
		next_offset = 4;
		break;
	default:
		next_offset = 0;
		break;
	}

	if (args.cur == DISPLAY_COMPONENT_RESIZER && args.next == DISPLAY_COMPONENT_P2S) {
		ret = mtk_mmsys_cfg_disconnect_comp(s_mmsyscfg_dev, MMSYSCFG_COMPONENT_RSZ1, MMSYSCFG_COMPONENT_P2S0, NULL);
	} else if (args.cur == DISPLAY_COMPONENT_P2S && args.next == DISPLAY_COMPONENT_WDMA) {
		ret = mtk_mmsys_cfg_disconnect_comp(s_mmsyscfg_dev, MMSYSCFG_COMPONENT_P2S0, MMSYSCFG_COMPONENT_DISP_WDMA0, NULL);
	} else if (args.cur == DISPLAY_COMPONENT_CROP && args.next == DISPLAY_COMPONENT_WDMA) {
		ret = mtk_mmsys_cfg_disconnect_comp(s_mmsyscfg_dev, MMSYSCFG_COMPONENT_P2S0, MMSYSCFG_COMPONENT_DISP_WDMA0, NULL);
		ret |= mtk_mmsys_cfg_disconnect_comp(s_mmsyscfg_dev, MMSYSCFG_COMPONENT_RSZ1, MMSYSCFG_COMPONENT_DISP_WDMA1, NULL);
		ret |= mtk_mmsys_cfg_disconnect_comp(s_mmsyscfg_dev, MMSYSCFG_COMPONENT_CROP2, MMSYSCFG_COMPONENT_DISP_WDMA2, NULL);
		ret |= mtk_mmsys_cfg_disconnect_comp(s_mmsyscfg_dev, MMSYSCFG_COMPONENT_CROP3, MMSYSCFG_COMPONENT_DISP_WDMA3, NULL);
	} else {
		for (i = 0; i < 4; i++) {
			cur =  (int)convert_display_component_to_comp_id(args.cur) + (i >> cur_offset);
			next = (int)convert_display_component_to_comp_id(args.next) + (i >> next_offset);

			pr_debug("DisconnectComponent %d->%d\n", cur, next);
			ret = mtk_mmsys_cfg_disconnect_comp(s_mmsyscfg_dev,
							    cur, next,
							    NULL);
		}
	}
	return ret;
}

static int mtk_wrapper_mmsyscfg_camera_sync_clock_sel(bool arg)
{
	pr_debug("mmsyscfg_camera_sync_clock_sel %d\n", arg);
	return mtk_mmsys_cfg_camera_sync_clock_sel(s_mmsyscfg_dev, (bool)arg);
}

static int mtk_wrapper_mmsyscfg_camera_sync_config(void *user)
{
	args_mmsyscfg_camera_sync_config args;
	int ret = 0;

	ret = copy_from_user(&args, user, sizeof(args_mmsyscfg_camera_sync_config));
	if (ret != 0) {
		return -EFAULT;
	}

	pr_debug("mmsyscfg_camera_sync_config(%d,%d,%d,%d)\n", args.camera_id,
		 args.vsync_cycle, args.delay_cycle, args.vsync_low_active);
	if (args.camera_id == MTK_WRAPPER_MMSYSCFG_CAMERA_SIDE) {
		ret = mtk_mmsys_cfg_camera_sync_config(s_mmsyscfg_dev,
						       MMSYSCFG_CAMERA_SYNC_SIDE01,
						       args.vsync_cycle, args.delay_cycle,
						       args.vsync_low_active, NULL);
		ret |= mtk_mmsys_cfg_camera_sync_config(s_mmsyscfg_dev,
							MMSYSCFG_CAMERA_SYNC_SIDE23,
							args.vsync_cycle, args.delay_cycle,
							args.vsync_low_active, NULL);
	} else if (args.camera_id == MTK_WRAPPER_MMSYSCFG_CAMERA_GAZE) {
		ret = mtk_mmsys_cfg_camera_sync_config(s_mmsyscfg_dev,
						       MMSYSCFG_CAMERA_SYNC_GAZE0,
						       args.vsync_cycle, args.delay_cycle,
						       args.vsync_low_active, NULL);
		ret |= mtk_mmsys_cfg_camera_sync_config(s_mmsyscfg_dev,
							MMSYSCFG_CAMERA_SYNC_GAZE1,
							args.vsync_cycle, args.delay_cycle,
							args.vsync_low_active, NULL);
	} else if (args.camera_id == MTK_WRAPPER_MMSYSCFG_CAMERA_GAZE_LED) {
		ret |= mtk_mmsys_cfg_camera_sync_config(s_mmsyscfg_dev,
							MMSYSCFG_CAMERA_SYNC_GAZE_LED,
							args.vsync_cycle, args.delay_cycle,
							args.vsync_low_active, NULL);
	} else {
		return -EINVAL;
	}

	return ret;
}

static int mtk_wrapper_mmsyscfg_camera_sync_frc(void *user)
{
	args_mmsyscfg_camera_sync_frc args;
	int ret = 0;
	int cd;
	u32 m;
	u32 n;
	MTK_WRAPPER_MMSYSCFG_CAMERA_ID id;

	ret = copy_from_user(&args, user, sizeof(args_mmsyscfg_camera_sync_frc));
	if (ret != 0) {
		return -EFAULT;
	}
	id = args.camera_id;
	cd = gcd(args.panel_fps, args.camera_fps);
	n = args.camera_fps / cd;
	m = args.panel_fps / cd;
	pr_debug("cameraFPS(%d), panelFPS(%d), cd(%d), n(%d), m(%d)\n",
		 args.camera_fps, args.panel_fps, cd, n, m);

	if (id >= MTK_WRAPPER_MMSYSCFG_CAMERA_MAX) {
		return -EINVAL;
	}

	if (id == MTK_WRAPPER_MMSYSCFG_CAMERA_SIDE) {
		ret = mtk_mmsys_cfg_camera_sync_frc(s_mmsyscfg_dev,
						    MMSYSCFG_CAMERA_SYNC_SIDE01,
						    n, m, NULL);
		ret |= mtk_mmsys_cfg_camera_sync_frc(s_mmsyscfg_dev,
						     MMSYSCFG_CAMERA_SYNC_SIDE23,
						     n, m, NULL);
	} else if (id == MTK_WRAPPER_MMSYSCFG_CAMERA_GAZE) {
		ret = mtk_mmsys_cfg_camera_sync_frc(s_mmsyscfg_dev,
						    MMSYSCFG_CAMERA_SYNC_GAZE0,
						    n, m, NULL);
		ret |= mtk_mmsys_cfg_camera_sync_frc(s_mmsyscfg_dev,
						     MMSYSCFG_CAMERA_SYNC_GAZE1,
						     n, m, NULL);
	} else {
		/* id == MTK_WRAPPER_MMSYSCFG_CAMERA_GAZE_LED */
		ret = mtk_mmsys_cfg_camera_sync_frc(s_mmsyscfg_dev,
						    MMSYSCFG_CAMERA_SYNC_GAZE_LED,
						    n, m, NULL);
	}
	return ret;
}

static int mtk_wrapper_mmsyscfg_camera_sync_enable(MTK_WRAPPER_MMSYSCFG_CAMERA_ID id)
{
	int ret;
	u32 event;
	struct cmdq_pkt *pkt;

	pr_debug("mmsyscfg_camera_sync_enable(%d)\n", id);

	event = CMDQ_EVENT_MMSYS_CORE_DSI0_SOF_EVENT;

	if (id >= MTK_WRAPPER_MMSYSCFG_CAMERA_MAX) {
		return -EINVAL;
	}

	cmdq_pkt_create(&pkt);
	cmdq_pkt_clear_event(pkt, event);
	cmdq_pkt_wfe(pkt, event);

	if (id == MTK_WRAPPER_MMSYSCFG_CAMERA_SIDE) {
		ret = mtk_mmsys_cfg_camera_sync_enable(s_mmsyscfg_dev,
						       MMSYSCFG_CAMERA_SYNC_SIDE01,
						       &pkt);
		ret |= mtk_mmsys_cfg_camera_sync_enable(s_mmsyscfg_dev,
							MMSYSCFG_CAMERA_SYNC_SIDE23,
							&pkt);
	} else if (id == MTK_WRAPPER_MMSYSCFG_CAMERA_GAZE) {
		ret = mtk_mmsys_cfg_camera_sync_enable(s_mmsyscfg_dev,
						       MMSYSCFG_CAMERA_SYNC_GAZE0,
						       &pkt);
		ret |= mtk_mmsys_cfg_camera_sync_enable(s_mmsyscfg_dev,
							MMSYSCFG_CAMERA_SYNC_GAZE1,
							&pkt);
	} else {
		/* id == MTK_WRAPPER_MMSYSCFG_CAMERA_GAZE_LED */
		ret = mtk_mmsys_cfg_camera_sync_enable(s_mmsyscfg_dev,
						       MMSYSCFG_CAMERA_SYNC_GAZE_LED,
						       &pkt);
	}

	cmdq_pkt_flush(s_cmdq_client[id], pkt);
	cmdq_pkt_destroy(pkt);

	return ret;
}

static int mtk_wrapper_mmsyscfg_camera_sync_disable(MTK_WRAPPER_MMSYSCFG_CAMERA_ID id)
{
	int ret;
	u32 event;
	struct cmdq_pkt *pkt;

	pr_debug("mmsyscfg_camera_sync_disable(%d)\n", id);

	event = CMDQ_EVENT_MMSYS_CORE_DSI0_SOF_EVENT;

	if (id >= MTK_WRAPPER_MMSYSCFG_CAMERA_MAX) {
		return -EINVAL;
	}

	cmdq_pkt_create(&pkt);
	cmdq_pkt_clear_event(pkt, event);
	cmdq_pkt_wfe(pkt, event);

	if (id == MTK_WRAPPER_MMSYSCFG_CAMERA_SIDE) {
		ret = mtk_mmsys_cfg_camera_sync_disable(s_mmsyscfg_dev,
							MMSYSCFG_CAMERA_SYNC_SIDE01,
							&pkt);
		ret |= mtk_mmsys_cfg_camera_sync_disable(s_mmsyscfg_dev,
							 MMSYSCFG_CAMERA_SYNC_SIDE23,
							 &pkt);
	} else if (id == MTK_WRAPPER_MMSYSCFG_CAMERA_GAZE) {
		ret = mtk_mmsys_cfg_camera_sync_disable(s_mmsyscfg_dev,
							MMSYSCFG_CAMERA_SYNC_GAZE0,
							&pkt);

		ret |= mtk_mmsys_cfg_camera_sync_disable(s_mmsyscfg_dev,
							 MMSYSCFG_CAMERA_SYNC_GAZE1,
							 &pkt);
	} else {
		/* id == MTK_WRAPPER_MMSYSCFG_CAMERA_GAZE_LED */
		ret = mtk_mmsys_cfg_camera_sync_disable(s_mmsyscfg_dev,
							MMSYSCFG_CAMERA_SYNC_GAZE_LED,
							&pkt);
	}
	cmdq_pkt_flush(s_cmdq_client[id], pkt);
	cmdq_pkt_destroy(pkt);

	return ret;
}

static int mtk_wrapper_mmsyscfg_dsi_lane_swap_config(void *user)
{
	args_mmsyscfg_dsi_lane_swap_config args;
	int ret = 0;

	ret = copy_from_user(&args, user, sizeof(args_mmsyscfg_dsi_lane_swap_config));
	if (ret != 0) {
		return -EFAULT;
	}
	ret = mtk_mmsys_cfg_dsi_lane_swap_config(s_mmsyscfg_dev,
						 args.swap_config[0],
						 args.swap_config[1],
						 args.swap_config[2],
						 args.swap_config[3], NULL);
	return ret;
}

static int mtk_wrapper_mmsyscfg_slcr_to_ddds_select(DISPLAY_COMPONENT comp)
{
	enum mtk_mmsys_slicer_to_ddds_sel ddds;
	int ret;

	ddds = convert_ddds_sel(comp);
	if (ddds == MMSYSCFG_SLCR_TO_DDDS_MAX) {
		return -EINVAL;
	}
	ret = mtk_mmsys_cfg_slicer_to_ddds_select(s_mmsyscfg_dev, ddds, NULL);
	return ret;
}

static int mtk_wrapper_mmsyscfg_lhc_swap_cfg(void *user)
{
	args_mmsyscfg_lhc_swap_config args;
	int ret = 0;

	ret = copy_from_user(&args, user, sizeof(args_mmsyscfg_lhc_swap_config));
	if (ret != 0) {
		return -EFAULT;
	}

	ret = mtk_mmsys_cfg_lhc_swap_config(s_mmsyscfg_dev,
					    args.swap_config[0],
					    args.swap_config[1],
					    args.swap_config[2],
					    args.swap_config[3], NULL);

	return ret;
}

static int mtk_wrapper_mmsyscfg_modules_reset(enum mtk_mmsys_config_comp_id id, int num) {
	int i;
	int ret = 0;
	for (i = 0; i < num; i++) {
		ret |= mtk_mmsys_cfg_reset_module(s_mmsyscfg_dev, NULL, id+i);
	}
	return ret;
}

static int mtk_wrapper_mmsyscfg_reset(void)
{
	int ret = 0;
	ret |= mtk_wrapper_mmsyscfg_modules_reset(MMSYSCFG_COMPONENT_MUTEX, 1);
	ret |= mtk_wrapper_mmsyscfg_modules_reset(MMSYSCFG_COMPONENT_SLCR_MOUT0, 4);
	ret |= mtk_wrapper_mmsyscfg_modules_reset(MMSYSCFG_COMPONENT_SLICER_MM, 3);
	ret |= mtk_wrapper_mmsyscfg_modules_reset(MMSYSCFG_COMPONENT_RBFC0, 4);
	ret |= mtk_wrapper_mmsyscfg_modules_reset(MMSYSCFG_COMPONENT_MDP_RDMA0, 4);
	ret |= mtk_wrapper_mmsyscfg_modules_reset(MMSYSCFG_COMPONENT_MDP_RDMA_PVRIC0, 4);
	ret |= mtk_wrapper_mmsyscfg_modules_reset(MMSYSCFG_COMPONENT_DISP_RDMA0, 4);
	ret |= mtk_wrapper_mmsyscfg_modules_reset(MMSYSCFG_COMPONENT_RDMA_MOUT0, 4);
	ret |= mtk_wrapper_mmsyscfg_modules_reset(MMSYSCFG_COMPONENT_LHC_SWAP, 1);
	ret |= mtk_wrapper_mmsyscfg_modules_reset(MMSYSCFG_COMPONENT_LHC0, 4);
	ret |= mtk_wrapper_mmsyscfg_modules_reset(MMSYSCFG_COMPONENT_DSC0, 2);
	ret |= mtk_wrapper_mmsyscfg_modules_reset(MMSYSCFG_COMPONENT_DSC_MOUT0, 4);
	ret |= mtk_wrapper_mmsyscfg_modules_reset(MMSYSCFG_COMPONENT_DISP_WDMA0, 4);
	ret |= mtk_wrapper_mmsyscfg_modules_reset(MMSYSCFG_COMPONENT_DSI_LANE_SWAP, 1);
	ret |= mtk_wrapper_mmsyscfg_modules_reset(MMSYSCFG_COMPONENT_DSI0, 4);
	return ret;
}

static int mtk_wrapper_mmsys_open(struct inode *inode, struct file *file)
{
	return 0;
}

static long mtk_wrapper_mmsys_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case MMSYSCFG_CONNECT_COMPONENT:
		ret = mtk_wrapper_mmsyscfg_connect_component((void *)arg);
		break;
	case MMSYSCFG_DISCONNECT_COMPONENT:
		ret = mtk_wrapper_mmsyscfg_disconnect_component((void *)arg);
		break;
	case MMSYSCFG_RESET_MODULE:
		ret = mtk_wrapper_mmsyscfg_reset();
		break;
	case MMSYSCFG_CAMERA_SYNC_CLOCK_SEL:
		ret = mtk_wrapper_mmsyscfg_camera_sync_clock_sel((bool)arg);
		break;
	case MMSYSCFG_CAMERA_SYNC_CONFIG:
		ret = mtk_wrapper_mmsyscfg_camera_sync_config((void *)arg);
		break;
	case MMSYSCFG_CAMERA_SYNC_FRC:
		ret = mtk_wrapper_mmsyscfg_camera_sync_frc((void *)arg);
		break;
	case MMSYSCFG_CAMERA_SYNC_ENABLE:
		ret = mtk_wrapper_mmsyscfg_camera_sync_enable((MTK_WRAPPER_MMSYSCFG_CAMERA_ID)arg);
		break;
	case MMSYSCFG_CAMERA_SYNC_DISABLE:
		ret = mtk_wrapper_mmsyscfg_camera_sync_disable((MTK_WRAPPER_MMSYSCFG_CAMERA_ID)arg);
		break;
	case MMSYSCFG_DSI_LANE_SWAP_CONFIG:
		ret = mtk_wrapper_mmsyscfg_dsi_lane_swap_config((void *)arg);
		break;
	case MMSYSCFG_SLCR_TO_DDDS_SELECT:
		ret = mtk_wrapper_mmsyscfg_slcr_to_ddds_select((DISPLAY_COMPONENT)arg);
		break;
	case MMSYSCFG_LHC_SWAP_CONFIG:
		ret = mtk_wrapper_mmsyscfg_lhc_swap_cfg((void *)arg);
		break;
	default:
		pr_err("mmsys_ioctl: unknown command(%d)\n", cmd);
		return -EINVAL;
	}

	return ret;
}

static const struct file_operations mtk_wrapper_mmsys_fops = {
	.open           = mtk_wrapper_mmsys_open,
	.unlocked_ioctl = mtk_wrapper_mmsys_ioctl
};

int init_module_mtk_wrapper_mmsyscfg(void)
{
	int ret;
	struct platform_device *pdev;
	struct device *disp_pipe = NULL;
	int i;

	ret = mtk_wrapper_display_pipe_probe("mediatek,mmsyscfg", 0, &s_mmsyscfg_dev);
	if (ret < 0) {
		pr_err("can't find display_pipe mmsyscfg\n");
		return ret;
	}
	pr_info("%s start\n", __func__);
	ret = register_chrdev(SCE_KM_CDEV_MAJOR_MTK_WRAPPER_MMSYS, SCE_KM_MMSYS_DEVICE_NAME, &mtk_wrapper_mmsys_fops);
	if (ret < 0) {
		pr_err("log: alloc_chrdev(%s) failed(%d).\n", SCE_KM_MMSYS_DEVICE_NAME, ret);
		return ret;
	}

	/* mmsyscfg always power on */
	ret = mtk_mmsys_cfg_power_on(s_mmsyscfg_dev);
	if (ret < 0) {
		pr_err("mmsys_cfg power on failed(%d)\n", ret);
		return ret;
	}

	pdev = pdev_find_dt("sie,disp-pipe");
	if (pdev) {
		disp_pipe = &pdev->dev;
	}

	for (i = 0; i < MTK_WRAPPER_MMSYSCFG_CAMERA_MAX; i++) {
		s_cmdq_client[i] = cmdq_mbox_create(disp_pipe, 6 + i);
		if (!s_cmdq_client[i] ||
		    s_cmdq_client[i] == (struct cmdq_client *)-ENOMEM ||
		    s_cmdq_client[i] == (struct cmdq_client *)-EINVAL) {
			return -ENOMEM;
		}
	}

	return 0;
}

void cleanup_module_mtk_wrapper_mmsyscfg(void)
{
	mtk_mmsys_cfg_power_off(s_mmsyscfg_dev);
}
