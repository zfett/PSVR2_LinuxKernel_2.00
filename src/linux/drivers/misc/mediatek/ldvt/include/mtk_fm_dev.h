/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	Shan Lin <Shan.Lin@mediatek.com>
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

#ifndef MTK_FM_DEV_H
#define MTK_FM_DEV_H

#include <linux/types.h>
#include <linux/ioctl.h>
#include "mtk_common_util.h"
#include "mtk_dev_cv_common.h"
#include <soc/mediatek/ivp_kservice_api.h>

/* ring buffer control num*/
#define FM_RING_BUF_NR 3

enum mtk_fm_dev_flush_mode {
	WARPA_WORK = 0,
	FE_WORK = 1,
	FM_WORK = 2,
	FM_WORK_NO_INT = 3,
	WEN0_WORK = 4,
	WEN1_WORK = 5,
	WEN2_WORK = 6,
	WEN3_WORK = 7,
	REN_WORK = 8,
	BE0_WORK = 9,
	BE1_WORK = 10,
	BE2_WORK = 11,
	BE3_WORK = 12,
};

struct mtk_fm_dev {
	struct device *dev;
	struct device *dev_fm;
	struct device *dev_ivp;
	struct device *dev_mmsys_cmmn_top;
	struct device *dev_sysram;
	dev_t devt;
	struct cdev chrdev;
	struct class *dev_class;
	struct mtk_common_buf *buf[MTK_FM_DEV_BUF_TYPE_MAX];
	u32 fmdevmask;
	u32 sysram_layout;
	int img_num;
	int fe_w;
	int fe_h;
	int blk_sz;
	int blk_nr;
	int fm_msk_w_s;
	int fm_msk_h_s;
	int fm_msk_w_tp;
	int fm_msk_h_tp;
	int fps_type;
	int sr_mode; /* to control for s+t fm mode */
	int sr_type;
	int blk_type;
	char mask_path_s[256];
	void *mask_va_s;
	char mask_path_tp[256];
	void *mask_va_tp;
	char sc_path[256];
	char sc_path_1_4[256];
	char sc_path_1_16[256];
	char dump_path[256];
	void *sc_va;
	void *sc_va_1_4;
	void *sc_va_1_16;
	int msk_tbl_size_s;
	int msk_tbl_size_tp;
	int sc_size;
	int polling_wait;
	int flow_done;
	int fe_align;
	int wdma_align;
	int fm_align;
	int hdr_idx_cur; /* for control ping pong buffer */
	int hdr_idx_pre;
	int fm_flush_wait;
	int warpa_flush_wait;
	int fe_flush_wait;
	int wen0_flush_wait;
	int wen1_flush_wait;
	int wen2_flush_wait;
	int wen3_flush_wait;
	int ren_flush_wait;
	int be0_flush_wait;
	int be1_flush_wait;
	int be2_flush_wait;
	int be3_flush_wait;
	int be0_lim_cnt;
	int be1_lim_cnt;
	int be2_lim_cnt;
	int be3_lim_cnt;

	int one_shot_dump;
	u32 zncc_th;
	struct mtk_fm_dev_debug_test cv_debug_test;
	struct mtk_fm_reqinterval req_interval;
	struct cmdq_client *cmdq_client;
	struct cmdq_client *cmdq_client_fe;
	struct cmdq_client *cmdq_client_warpa;
	struct cmdq_client *cmdq_client_wen0;
	struct cmdq_client *cmdq_client_wen1;
	struct cmdq_client *cmdq_client_wen2;
	struct cmdq_client *cmdq_client_wen3;
	struct cmdq_client *cmdq_client_ren;
	struct cmdq_client *cmdq_client_be0;
	struct cmdq_client *cmdq_client_be1;
	struct cmdq_client *cmdq_client_be2;
	struct cmdq_client *cmdq_client_be3;
	u32 cmdq_events[MTK_FM_DEV_CMDQ_EVENT_MAX];
	struct work_struct trigger_work;
	struct work_struct warpa_et_work;
	struct work_struct fe_et_work;
	struct work_struct wen0_incomp_work;
	struct work_struct wen1_incomp_work;
	struct work_struct wen2_incomp_work;
	struct work_struct wen3_incomp_work;
	struct work_struct ren_incomp_work;
	struct work_struct be0_lim_work;
	struct work_struct be1_lim_work;
	struct work_struct be2_lim_work;
	struct work_struct be3_lim_work;
	struct cmdq_pkt *cmdq_pkt;
	struct cmdq_pkt *cmdq_pkt_fe;
	struct cmdq_pkt *cmdq_pkt_warpa;
	struct cmdq_pkt *cmdq_pkt_wen0;
	struct cmdq_pkt *cmdq_pkt_wen1;
	struct cmdq_pkt *cmdq_pkt_wen2;
	struct cmdq_pkt *cmdq_pkt_wen3;
	struct cmdq_pkt *cmdq_pkt_ren;
	struct cmdq_pkt *cmdq_pkt_be0;
	struct cmdq_pkt *cmdq_pkt_be1;
	struct cmdq_pkt *cmdq_pkt_be2;
	struct cmdq_pkt *cmdq_pkt_be3;

	/*wpe*/
	dma_addr_t wpe_et_max_pa;
	u32 *wpe_et_max_va;

	dma_addr_t wpe_et_pa;
	u32 *wpe_et_va;

	dma_addr_t wpe_cnt_pa;
	u32 *wpe_cnt_va;

	/*fe*/
	dma_addr_t fe_et_max_pa;
	u32 *fe_et_max_va;

	dma_addr_t fe_et_pa;
	u32 *fe_et_va;

	dma_addr_t fe_cnt_pa;
	u32 *fe_cnt_va;

	/*fms*/
	dma_addr_t fms_delay_max_pa;
	u32 *fms_delay_max_va;

	dma_addr_t fms_delay_pa;
	u32 *fms_delay_va;

	dma_addr_t fms_et_max_pa;
	u32 *fms_et_max_va;

	dma_addr_t fms_et_pa;
	u32 *fms_et_va;

	/*feT*/
	dma_addr_t feT_delay_pa;
	u32 *feT_delay_va;

	dma_addr_t feT_delay_max_pa;
	u32 *feT_delay_max_va;

	dma_addr_t feT_et_pa;
	u32 *feT_et_va;

	dma_addr_t feT_et_max_pa;
	u32 *feT_et_max_va;

	dma_addr_t feT_1_4_et_pa;
	u32 *feT_1_4_et_va;

	dma_addr_t feT_1_4_et_max_pa;
	u32 *feT_1_4_et_max_va;

	dma_addr_t feT_1_16_et_pa;
	u32 *feT_1_16_et_va;

	dma_addr_t feT_1_16_et_max_pa;
	u32 *feT_1_16_et_max_va;

	/*sc*/
	dma_addr_t sc_delay_max_pa;
	u32 *sc_delay_max_va;

	dma_addr_t sc_delay_pa;
	u32 *sc_delay_va;

	dma_addr_t sc_et_max_pa;
	u32 *sc_et_max_va;

	dma_addr_t sc_et_pa;
	u32 *sc_et_va;

	/*fmt*/
	dma_addr_t fmt_delay_max_pa;
	u32 *fmt_delay_max_va;

	dma_addr_t fmt_delay_pa;
	u32 *fmt_delay_va;

	dma_addr_t fmt_et_max_pa;
	u32 *fmt_et_max_va;

	dma_addr_t fmt_et_pa;
	u32 *fmt_et_va;

	/*1/4sc*/
	dma_addr_t sc_1_4_delay_max_pa;
	u32 *sc_1_4_delay_max_va;

	dma_addr_t sc_1_4_delay_pa;
	u32 *sc_1_4_delay_va;

	dma_addr_t sc_1_4_et_max_pa;
	u32 *sc_1_4_et_max_va;

	dma_addr_t sc_1_4_et_pa;
	u32 *sc_1_4_et_va;

	/*1/4fmt*/
	dma_addr_t fmt_1_4_delay_max_pa;
	u32 *fmt_1_4_delay_max_va;

	dma_addr_t fmt_1_4_delay_pa;
	u32 *fmt_1_4_delay_va;

	dma_addr_t fmt_1_4_et_max_pa;
	u32 *fmt_1_4_et_max_va;

	dma_addr_t fmt_1_4_et_pa;
	u32 *fmt_1_4_et_va;

	/*1/16sc*/
	dma_addr_t sc_1_16_delay_max_pa;
	u32 *sc_1_16_delay_max_va;

	dma_addr_t sc_1_16_delay_pa;
	u32 *sc_1_16_delay_va;

	dma_addr_t sc_1_16_et_max_pa;
	u32 *sc_1_16_et_max_va;

	dma_addr_t sc_1_16_et_pa;
	u32 *sc_1_16_et_va;

	/*1/16fmt*/
	dma_addr_t fmt_1_16_delay_max_pa;
	u32 *fmt_1_16_delay_max_va;

	dma_addr_t fmt_1_16_delay_pa;
	u32 *fmt_1_16_delay_va;

	dma_addr_t fmt_1_16_et_max_pa;
	u32 *fmt_1_16_et_max_va;

	dma_addr_t fmt_1_16_et_pa;
	u32 *fmt_1_16_et_va;

	/*headpose*/
	dma_addr_t head_pose_delay_max_pa;
	u32 *head_pose_delay_max_va;

	dma_addr_t head_pose_delay_pa;
	u32 *head_pose_delay_va;

	dma_addr_t head_pose_et_max_pa;
	u32 *head_pose_et_max_va;

	dma_addr_t head_pose_et_pa;
	u32 *head_pose_et_va;

	/*fm vpu flow overall*/
	dma_addr_t fm_vpu_et_max_pa;
	u32 *fm_vpu_et_max_va;

	dma_addr_t fm_vpu_et_pa;
	u32 *fm_vpu_et_va;

	dma_addr_t fm_vpu_cnt_pa;
	u32 *fm_vpu_cnt_va;

	struct timeval vpu_start;
	struct timeval vpu_done;

	struct cmdq_flush_completion warpa_work_comp;
	struct cmdq_flush_completion fe_work_comp;
	struct cmdq_flush_completion fm_work_comp;
	struct cmdq_flush_completion wen0_work_comp;
	struct cmdq_flush_completion wen1_work_comp;
	struct cmdq_flush_completion wen2_work_comp;
	struct cmdq_flush_completion wen3_work_comp;
	struct cmdq_flush_completion ren_work_comp;
	struct cmdq_flush_completion be0_work_comp;
	struct cmdq_flush_completion be1_work_comp;
	struct cmdq_flush_completion be2_work_comp;
	struct cmdq_flush_completion be3_work_comp;

	/* VPU req struct */
	struct mtkivp_request process_feT1;
	struct mtkivp_request process_feT2;
	struct mtkivp_request process_1_4_feT1;
	struct mtkivp_request process_1_4_feT2;
	struct mtkivp_request process_1_16_feT1;
	struct mtkivp_request process_1_16_feT2;
	struct mtkivp_request process_headpose;
	struct mtkivp_request process_1_4_WDMA;
	struct mtkivp_request process_1_16_WDMA;
	struct mtkivp_request process_DRAM_TO_SRAM;
	struct mtkivp_request process_SRAM_TO_DRAM;
};

int mtk_fm_dev_module_config(struct mtk_fm_dev *fm_dev);

int mtk_fm_dev_path_connect(struct mtk_fm_dev *fm_dev);

int mtk_fm_dev_path_disconnect(struct mtk_fm_dev *fm_dev);

int mtk_fm_dev_path_trigger_start(struct mtk_fm_dev *fm_dev, bool onoff);

int mtk_fm_dev_path_trigger_start_no_wait(struct mtk_fm_dev *fm_dev,
	bool onoff);

int mtk_fm_dev_module_set_buf(struct mtk_fm_dev *fm_dev,
	struct cmdq_pkt *handle, u32 img_type, u32 sr_type);

int mtk_fm_dev_module_power_on(struct mtk_fm_dev *fm_dev);

int mtk_fm_dev_module_power_off(struct mtk_fm_dev *fm_dev);

int mtk_fm_dev_module_start(struct mtk_fm_dev *fm_dev);

int mtk_fm_dev_module_stop(struct mtk_fm_dev *fm_dev);

#endif /* MTK_FM_DEV_H */
