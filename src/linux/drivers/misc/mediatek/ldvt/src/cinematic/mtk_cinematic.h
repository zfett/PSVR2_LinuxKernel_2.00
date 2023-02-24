/*
 * Copyright (c) 2018 MediaTek Inc.
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

#ifndef MTK_CINEMATIC_H
#define MTK_CINEMATIC_H

#include <linux/soc/mediatek/mtk-cmdq.h>
#include <soc/mediatek/mtk_mutex.h>
#include <soc/mediatek/mtk_mmsys_cfg.h>
#include <soc/mediatek/mtk_lhc.h>
#include <uapi/mediatek/cinematic_para.h>
#include "mtk_cinematic_ctrl.h"
#include <soc/mediatek/mtk_dprx_info.h>
#include <soc/mediatek/mtk_dprx_if.h>

#define PIPE_MAX_W 1920
#define PIPE_MAX_H 1080
#define FPGA_PANEL_W 1080
#define FPGA_PANEL_H 1920
#define SOC_PANEL_W 4000
#define SOC_PANEL_H 2040
#define CINEMATIC_MODULE_CNT 5
#define WDMA_BUF 5
#define DISPSYS_DELAY_NUM 4
#define BYP_GPU_WDMA_BUF 1
#define MAIN_PIPE 0
#define DUAL_PIPE 2
#define QUADRUPLE_PIPE 4
#define RUN_BIT_TRUE 0
#define RATIO_SIZE 2

/* Define FPS */
#define SOF_GAP	2000
#define FPS_120	120
#define FPS_119	119
#define FPS_90	90
#define FPS_89	89
#define FPS_60	60
#define FPS_59	59
#define FPS_50  50
#define FPS_24	24
#define FPS_23	23


enum lhc_sync_path {
	FROM_DP,
	FROM_RDMA,
};

enum sof_type {
	SINGLE_SOF,
	CONTINUOUS_SOF,
};

enum crop_region_type {
	CP_EMPTY = 0,
	CP_HEAD,
	CP_MIDDLE,
	CP_REAR,
	CP_HEAD_REAR,
};

struct mtk_disp_fb {
	dma_addr_t pa;
	int fb_id;
};

struct mtk_disp_path_mutex {
	enum mtk_mutex_comp_id rsz_id;
	enum mtk_mutex_comp_id crop_id;
	enum mtk_mutex_comp_id wdma_id;
};

struct mtk_disp_path_mmsys {
	enum mtk_mmsys_config_comp_id rsz_id;
	enum mtk_mmsys_config_comp_id crop_id;
	enum mtk_mmsys_config_comp_id wdma_id;
};

/* bypass gpu info */
struct mtk_dma_buf {
	void *va;
	dma_addr_t pa;
};

struct wdma_buffer {
	struct mtk_dma_buf *k_buf;
	struct mtk_common_buf *ion_buf;
	u32 ref_count;
};

struct crop_output {
	u16 sop;
	u16 eop;
	u16 x_oft;
	u16 y_oft;
	u16 out_w;
	u16 out_h;
	enum crop_region_type crop_type;
};

struct wdma_port {
	struct wdma_buffer buf[WDMA_BUF];
	u32 format;
	u32 bpp;
	u32 pitch;
	u32 buf_size;
	u32 pvric_hdr_offset;
	bool using_ion_buf;
	struct crop_output config_crop[QUADRUPLE_PIPE];
};

struct mtk_cinematic_test {
	struct device *dev;
	struct device *dev_mmsys_core_top;
	struct device *dev_mutex;
	struct device *dev_slicer;
	struct device *dev_resizer;
	struct device *dev_crop;
	struct device *dev_wdma;
	struct device *dev_p2s;
	struct dentry *debugfs;

	struct slcr_config *slcr_cfg;
	struct slcr_pg_config *slcr_pg_cfg;
	struct mtk_mutex_res *mutex;
	struct mtk_mutex_res *mutex_delay;
	struct completion wdma_done;
	struct wdma_port output_port;
	struct mtk_disp_path_mutex disp_mutex;
	struct mtk_disp_path_mmsys disp_mmsys;
	struct mutex fb_lock;

	/* kernel internal path (bypass GPU) */
	u32 image_output_size;

	bool buffer_allocated;
	bool seq_chg;
	bool dp_video_set;

	/* parameter config */
	struct cinematic_parameter para;
	u32 w;
	u32 h;
	u32 disp_path;
	u32 inserted_buf;
	u32 cur_fb_id;
	u32 next_fb_id;
	u32 done_fb_id;
	u32 target_line;
	u32 dummy_line;
	u32 sof_line_diff;
	u32 lcm_vtotal;
	u32 frmtrk_target_line;
	u32 gpu_fb_id;
	bool is_3d_tag;

	struct cmdq_client *cmdq_client;
	struct cmdq_pkt *cmdq_pkt;
	struct mtk_lhc_slice lhc_slice_cfg[4];
	struct dprx_video_info_s dp_info;
};

int mtk_cinematic_start(struct mtk_cinematic_test *ct);
int mtk_cinematic_stop(struct mtk_cinematic_test *ct);
int mtk_cinematic_fb_to_gpu(struct mtk_cinematic_test *ct, u32 given_fb);
int mtk_format_to_bpp(u32 fmt);
int mtk_cinematic_set_buf_mode(struct mtk_cinematic_test *ct, int using_ion);
int mtk_cinematic_set_ion_buf(struct mtk_cinematic_test *ct,
			      struct mtk_common_buf *buf, int buf_idx);
int mtk_reset_core_modules(struct mtk_cinematic_test *ct);
int mtk_single_pipe_config(struct mtk_cinematic_test *ct);
int mtk_dual_pipe_config(struct mtk_cinematic_test **ct);
int mtk_quad_pipe_config(struct mtk_cinematic_test **ct);
int mtk_cinematic_trigger(struct mtk_cinematic_test *ct);
int mtk_cinematic_trigger_no_wait(struct mtk_cinematic_test *ct);
int mtk_dynamic_seq_chg(struct mtk_cinematic_test *ct, struct cc_job job,
			u32 eof_id);
int mtk_dynamic_pat_chg(struct mtk_cinematic_test *ct);
int mtk_cinematic_addr_update(struct mtk_cinematic_test **ct);
int mtk_cinematic_swap_buf(struct mtk_cinematic_test *ct);
int mtk_cinematic_get_disp_fb(struct mtk_cinematic_test *ct,
			      struct mtk_disp_fb *fb);
int mtk_cinematic_dual_stop(struct mtk_cinematic_test **ct);
int mtk_cinematic_quad_stop(struct mtk_cinematic_test **ct);
int mtk_cinematic_resume(struct mtk_cinematic_test *ct);
int mtk_cinematic_pause(struct mtk_cinematic_test *ct);
void mtk_reset_output_port_buf(struct mtk_cinematic_test *ct);
int mtk_cinematic_set_dbg(int dbg);
int mtk_cinematic_get_dbg(void);

#ifdef CONFIG_MACH_MT3612_A0
int mtk_cinematic_set_dp_video(struct mtk_cinematic_test *ct,
			       struct dprx_video_info_s *video);
int mtk_get_fps_ratio(u32 *ratio, u32 size, u32 i_fps, u32 o_fps);
int mtk_cinematic_set_frmtrk_height(struct mtk_cinematic_test *ct);
#endif
int mtk_dump_buf(struct mtk_cinematic_test *ct, int buf_idx, int size,
		 char *file_path);
#endif

