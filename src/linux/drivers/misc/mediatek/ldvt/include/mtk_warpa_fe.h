/*
 * Copyright (c) 2017 MediaTek Inc.
 * Authors:
 *	CK Hu <ck.hu@mediatek.com>
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

#ifndef MTK_WARPA_FE_H
#define MTK_WARPA_FE_H

#include <linux/types.h>
#include <linux/ioctl.h>
#include "mtk_common_util.h"
#include "mtk_dev_cv_common.h"
#include <soc/mediatek/ivp_kservice_api.h>
#include <soc/mediatek/mtk_warpa.h>
#include <soc/mediatek/mtk_padding.h>

enum mtk_warpa_fe_flush_mode {
	WARPA_FE_WORK = 0,
	P1_SOF_WORK = 1,
	P2_SOF_WORK = 2,
	BE_WORK = 3,
	BE_WORK_NO_INT = 4,
};

struct mtk_warpa_fe {
	struct device *dev;
	struct device *dev_mmsys_cmmn_top;
	struct device *dev_mmsys_core_top;
	struct device *dev_sysram;
	struct device *dev_mutex;
	struct device *dev_rbfc;
	struct device *dev_warpa;
	struct device *dev_fe;
	struct cmdq_client *cmdq_clients;
	struct cmdq_pkt *cmdq_handles;
	struct device *dev_wdma0, *dev_wdma1, *dev_wdma2;
	struct device *dev_padding0, *dev_padding1, *dev_padding2;
	struct device *dev_p_rsz0, *dev_p_rsz1;

	dev_t devt;
	struct cdev chrdev;
	struct class *dev_class;
	struct mtk_mutex_res *mutex;
	struct mtk_mutex_res *mutex_warpa_delay;
	struct mtk_common_buf *buf[MTK_WARPA_FE_BUF_TYPE_MAX];
	u32 warpa_in_w;
	u32 warpa_in_h;
	u32 warpa_out_w;
	u32 warpa_out_h;
	u32 warpa_map_w;
	u32 warpa_map_h;
	struct mtk_warpa_coef_tbl coef_tbl;
	struct mtk_warpa_border_color border_color;
	u32 proc_mode;
	u32 warpa_out_mode;
	u32 reset;
	u32 fe_w;
	u32 fe_h;
	u32 blk_sz;
	u32 fe_merge_mode;
	u32 fe_flat_enable;
	u32 fe_harris_kappa;
	u32 fe_th_g;
	u32 fe_th_c;
	u32 fe_cr_val_sel;
	u32 warpa_align;
	u32 fe_align;
	u32 wdma_align;
	u32 padding_val_0;
	u32 padding_val_1;
	u32 padding_val_2;
	u32 rsz_0_out_w;
	u32 rsz_0_out_h;
	u32 rsz_1_out_w;
	u32 rsz_1_out_h;
	u32 img_num;
	u32 sensor_fps;
	int warpa_fe_flush_wait;
	int p1_sof_flush_wait;
	int p2_sof_flush_wait;
	int be_flush_wait;
	int one_shot_dump;

	int polling_wait;
	int flow_done;
	int wdma0_cnt;
	int wdma1_cnt;
	int wdma2_cnt;
	int be_ring_buf_cnt;
	int be_en;
	int be_label_img_en;
	int p1vpu_pair;
	int path_mask;

	struct mtk_warpa_fe_debug_test_cam cam_debug_test;

	dma_addr_t p2_sof_cnt_pa;
	u32 *p2_sof_cnt_va;

	dma_addr_t p2_done_cnt_pa;
	u32 *p2_done_cnt_va;

	dma_addr_t p1_sof_cnt_pa;
	u32 *p1_sof_cnt_va;

	dma_addr_t p1_done_cnt_pa;
	u32 *p1_done_cnt_va;

	dma_addr_t be_vpu_done_cnt_pa;
	u32 *be_vpu_done_cnt_va;

	/*p1 flow */
	dma_addr_t P1_et_pa;
	u32 *P1_et_va;

	dma_addr_t P1_et_max_pa;
	u32 *P1_et_max_va;

	/* BE flow */
	dma_addr_t BE_VPU_delay_pa;
	u32 *BE_VPU_delay_va;

	dma_addr_t BE_VPU_delay_max_pa;
	u32 *BE_VPU_delay_max_va;

	dma_addr_t BE_VPU_et_pa;
	u32 *BE_VPU_et_va;

	dma_addr_t BE_VPU_et_max_pa;
	u32 *BE_VPU_et_max_va;

	dma_addr_t P1_VPU_et_pa;
	u32 *P1_VPU_et_va;

	dma_addr_t P1_VPU_et_max_pa;
	u32 *P1_VPU_et_max_va;

	struct cmdq_client *cmdq_client;
	struct cmdq_client *cmdq_client_p1_sof;
	struct cmdq_client *cmdq_client_p2_sof;
	struct cmdq_client *cmdq_client_be;
	struct cmdq_client *cmdq_client_camera_sync;
	u32 cmdq_events[MTK_WARPA_FE_CMDQ_EVENT_MAX];
	struct work_struct trigger_work;
	struct work_struct trigger_work_p1_sof;
	struct work_struct trigger_work_p2_sof;
	struct work_struct trigger_work_be;
	struct cmdq_pkt *cmdq_pkt;
	struct cmdq_pkt *cmdq_pkt_p1_sof;
	struct cmdq_pkt *cmdq_pkt_p2_sof;
	struct cmdq_pkt *cmdq_pkt_be;

	struct mtkivp_request process_be[16];

	struct cmdq_flush_completion warpa_fe_work_comp;
	struct cmdq_flush_completion p1_sof_work_comp;
	struct cmdq_flush_completion p2_sof_work_comp;
	struct cmdq_flush_completion be_work_comp;

	/*conctrl bit for path config*/
	u32 warpafemask;
	u32 sysram_layout;
	u32 in_buf_total;
	u32 in_buf_cnt;
	u32 in_buf_cnt_max;
	/* control warpa_fe ping pong output buffer */
	u32 out_buf_cnt;
};

int mtk_warpa_fe_module_config(struct mtk_warpa_fe *warpa_fe);
int mtk_warpa_fe_module_set_buf(struct mtk_warpa_fe *warpa_fe);
int mtk_warpa_fe_module_start(struct mtk_warpa_fe *warpa_fe);
int mtk_warpa_fe_module_stop(struct mtk_warpa_fe *warpa_fe);
int mtk_warpa_fe_reset(struct mtk_warpa_fe *warpa_fe);
int mtk_warpa_fe_module_power_on(struct mtk_warpa_fe *warpa_fe);
int mtk_warpa_fe_module_power_off(struct mtk_warpa_fe *warpa_fe);
int mtk_warpa_fe_path_connect(struct mtk_warpa_fe *warpa_fe);
int mtk_warpa_fe_path_disconnect(struct mtk_warpa_fe *warpa_fe);
int mtk_warpa_fe_path_trigger(struct mtk_warpa_fe *warpa_fe, bool onoff);
int mtk_warpa_fe_path_trigger_no_wait(struct mtk_warpa_fe *warpa_fe,
	bool onoff);

#endif /* MTK_WARPA_FE_H */
