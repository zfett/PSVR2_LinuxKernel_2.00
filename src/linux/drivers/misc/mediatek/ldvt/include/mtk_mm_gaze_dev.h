/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	leji qiu <leji.qiu@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef MTK_GAZE_CAMERA_DEV_H
#define MTK_GAZE_CAMERA_DEV_H

#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/gcd.h>
#include <linux/vmalloc.h>
#include <uapi/drm/drm_fourcc.h>
#include <soc/mediatek/mtk_warpa.h>
#include <soc/mediatek/mtk_wdma.h>
#include <soc/mediatek/mtk_mutex.h>
#include <soc/mediatek/mtk_rbfc.h>
#include <soc/mediatek/mtk_mmsys_gaze_top.h>
#include <soc/mediatek/mtk_mmsys_cfg.h>
#include "mtk_common_util.h"
#include <dt-bindings/gce/mt3612-gce.h>
#include <soc/mediatek/ivp_kservice_api.h>
#include <mtk_disp.h>
#include "mtk_dev_cv_common.h"

struct mtk_mm_gaze {
	struct device *dev;
	struct device *dev_mutex;
	struct device *dev_sysram;
	struct device *dev_rbfc;
	struct device *dev_warpa;
	struct device *dev_gaze_wdma;
	struct device *dev_mmsys_gaze_top;
	struct device *dev_mmsys_core_top;
	struct cmdq_client *cmdq_client;
	struct cmdq_pkt *cmdq_handle;
	struct work_struct trigger_work;
	int polling_wait;
	struct work_struct trigger_work_gce;
	int flush_state;
	struct cmdq_flush_completion gaze_flush_comp;

	/* polling ren rbfc incomlete status */
	struct cmdq_client *cmdq_client_rbfc_ren;
	struct cmdq_pkt *cmdq_handle_rbfc_ren;
	struct work_struct trigger_rbfc_ren;
	int flush_rbfc_ren_state;
	struct cmdq_flush_completion ren_rbfc_flush_comp;

	/* polling gaze0 wen rbfc incomlete status */
	struct cmdq_client *cmdq_client_gaze0_wen;
	struct cmdq_pkt *cmdq_handle_gaze0_wen;
	struct work_struct trigger_gaze0_wen;
	int flush_gaze0_wen_state;
	struct cmdq_flush_completion gaze0_wen_flush_comp;

	/* polling gaze1 wen rbfc incomlete status */
	struct cmdq_client *cmdq_client_gaze1_wen;
	struct cmdq_pkt *cmdq_handle_gaze1_wen;
	struct work_struct trigger_gaze1_wen;
	int flush_gaze1_wen_state;
	struct cmdq_flush_completion gaze1_wen_flush_comp;

	/* check gaze0 isp p2 sof count */
	struct cmdq_client *cmdq_client_gaze0_p2_sof;
	struct cmdq_pkt *cmdq_handle_gaze0_p2_sof;
	struct work_struct trigger_gaze0_p2_sof;
	int flush_gaze0_p2_sof_state;
	struct cmdq_flush_completion gaze0_p2_sof_flush_comp;

	/* for setting camera sync */
	struct cmdq_client *cmdq_client_camera_sync;

	struct cdev chardev;
	struct class *dev_class;
	struct mtk_mutex_res *mutex;
	dev_t devt;
	struct mtk_common_buf
		*buf[MTK_GAZE_CAMERA_BUF_TYPE_MAX];

	int img_num;
	int img_w, img_h;

	u32 warp_input_buf_size;
	u32 warp_input_buf_pitch;
	u32 warpa_in_w;
	u32 warpa_in_h;
	u32 warpa_out_w;
	u32 warpa_out_h;
	u32 warpa_map_w;
	u32 warpa_map_h;
	u32 warpa_align;
	struct mtk_warpa_coef_tbl coef_tbl;
	struct mtk_warpa_border_color border_color;
	u32 proc_mode;
	u32 warpa_out_mode;
	u32 reset;
	u32 warp_map_size;
	u32 warp_map_pitch;

	u32 wdma_buf_format;
	u32 wdma_buf_pitch;
	u32 wdma_buf_size;
	u32 wdma_align;
	u32 wdma_dbg;

	/*conctrl bit for path config*/
	u32 mmgazemask;
	int flow_done;
	int one_shot_dump;

	char dump_path[256];
	u32 sensor_fps;
	u32 delay_cycle;

	/* VPU req struct */
	struct mtkivp_request process_GAZE_WDMA;
};

int mtk_mm_gaze_module_power_on(struct mtk_mm_gaze *mm_gaze);
int mtk_mm_gaze_module_power_off(struct mtk_mm_gaze *mm_gaze);

#endif
