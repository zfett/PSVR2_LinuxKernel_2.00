/*
 * Copyright (c) 2017 MediaTek Inc.
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

#ifndef MTK_DISP_PIPE_H
#define MTK_DISP_PIPE_H

#include <soc/mediatek/mtk_dprx_info.h>
#include <soc/mediatek/mtk_dprx_if.h>

#define RUN_DSI_ANALYZER 0


enum mtk_dispsys_module_list {
	MTK_DISP_DSI,
	MTK_DISP_DSC,
	MTK_DISP_MUTEX,
	MTK_DISP_WDMA0,
	MTK_DISP_WDMA1,
	MTK_DISP_WDMA2,
	MTK_DISP_WDMA3,
	MTK_DISP_MDP_RDMA,
	MTK_DISP_PVRIC_RDMA,
	MTK_DISP_DISP_RDMA,
	MTK_DISP_MMSYS_CFG,
	MTK_DISP_RBFC,
	MTK_DISP_SLCR,
	MTK_DISP_TIMESTAMP,
	MTK_DISP_LHC,
	MTK_DISP_GCE0,
#ifdef CONFIG_MACH_MT3612_A0
	MTK_DISP_DPRX,
	MTK_DISP_DDDS,
	MTK_DISP_FRMTRK,
#endif
	MTK_DISP_MOD_NR
};

struct mtk_dispsys_resolution {
	u32 lcm_w;
	u32 lcm_h;
	u32 framerate;
	u32 bpc;

	/* RDMA input parameter */
	u32 input_format;
	u32 output_format;
	u32 input_height;
	u32 input_width;
};

struct mtk_dispsys_mod {
	int index;
	char name[128];
};

enum mtk_dispsys_cmdq_channel {
	PIPE_0,
	PIPE_1,
	PIPE_2,
	PIPE_3,
	DSI_ERR_MON,
	RESERVED_0,
	RESERVED_1,
	RESERVED_2,
	RESERVED_3,
	RESERVED_4,
	CH_MAX
};

struct mtk_dispsys_frmtrk_param {
	u32 ddds_hlen;
	u32 ddds_step1;
	u32 ddds_step2;
	u32 lock_win;
	u32 turbor_win;
	u32 target_line;
	u32 mask;
};

/* resources shared by all crtcs */
struct mtk_dispsys {
	struct device *dev;
	struct device *disp_dev[MTK_DISP_MOD_NR];
	struct cmdq_client *client[MTK_MM_MODULE_MAX_NUM];
	struct cmdq_client *err_cmdq_client;

	/** data struct use by dsi mutex driver */
	struct mtk_mutex_res *mutex[MUTEX_NR];
	struct mtk_mutex_res *mutex_delay[MUTEX_NR];

	struct dentry *debugfs;

	enum mtk_dispsys_scenario scenario;
	struct mtk_dispsys_resolution resolution;
	struct mtk_dispsys_buf buf;
	dma_addr_t disp_buf[2];
	unsigned long size;

	bool dsc_enable;
	bool dsc_passthru;
	bool lhc_enable;
	bool dump_enable;
	bool fbdc_enable;
	bool dp_enable;
	bool frmtrk;

	u32 disp_partition_nr;
	u32 disp_core_per_wrap;

	wait_queue_head_t lhc_wq;
	spinlock_t lock_irq;
	int lhc_frame_done;
	int encode_rate;

	/** private test context */
	void *mmsys_test;
	void *lhc_test;
	void *mutex_test;

	struct completion dsi_err;
	bool dsi_monitor;

	int (*scenario_callback)(struct device *dev, enum DPRX_NOTIFY_T event);
	struct device *scenario_dev;
	bool dp_video_set;
	bool dp_pps_set;
	u32 edid_sel;
	struct dprx_video_info_s dp_video_info;
	struct pps_sdp_s dp_pps_info;
	struct mtk_dispsys_frmtrk_param frmtrk_param;
};

#endif		/* MTK_DISP_PIPE_H */
