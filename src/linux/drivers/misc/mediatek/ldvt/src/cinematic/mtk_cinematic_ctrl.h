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

#ifndef MTK_CINEMATIC_CTRL_H
#define MTK_CINEMATIC_CTRL_H

#include <linux/cdev.h>
#include <linux/soc/mediatek/mtk-cmdq.h>
#include <uapi/mediatek/cinematic_para.h>
#include <soc/mediatek/mtk_dprx_info.h>
#include <soc/mediatek/mtk_dprx_if.h>

#define USING_ION 1
#define BUF_NUM 15
#define CINEMATIC_MODULE_CNT 5
#define JOBS_MAX 10
/* around 10 minutes ring buffer (100us) */
#define REC_NUM 6000000
/*#define CACHED_BUF*/
#define SOF_DIFF 4
#define VSYNC_DELAY 5
#define SOF_TIMEOUT 160000000

#define GCE_SW_TOKEN_GPU_VSYNC 900
#define GCE_SW_TOKEN_UNDERFLOW 901
#define GCE_CPR0 0x8000
#define GCE_CPR1 0x8001
#define GCE_CPR2 0x8002
#define GCE_CPR3 0x8003
#define GCE_CPR4 0x8004
#define GCE_CPR5 0x8005
#define GCE_CPR6 0x8006
#define GCE_CPR7 0x8007
#define GCE_CPR8 0x8008
#define GCE_CPR9 0x8009
#define GCE_CPR10 0x800A
#define GCE_CPR11 0x800B
#define GCE_CPR12 0x800C
#define GCE_CPR13 0x800D
#define GCE_CPR14 0x800E
#define GCE_CPR15 0x800F

extern u32 *rec_va_d;
extern u32 *rec_va_w;
extern u32 *rec_va_r;
extern dma_addr_t rec_pa_d;
extern dma_addr_t rec_pa_w;
extern dma_addr_t rec_pa_r;

enum {
	CC_LOG1 = 1 << 0,
	CC_LOG2 = 1 << 1,
	CC_LOG3 = 1 << 2,
	CC_LOG4 = 1 << 3
};

#define LOG_IN(...) \
	LOG(CC_LOG1, mtk_cinematic_get_dbg(), __VA_ARGS__)
#define LOG_OUT(...) \
	LOG(CC_LOG2, mtk_cinematic_get_dbg(), __VA_ARGS__)
#define LOG_VSYNC(...) \
	LOG(CC_LOG3, mtk_cinematic_get_dbg(), __VA_ARGS__)
#define LOG_FB(...) \
	LOG(CC_LOG4, mtk_cinematic_get_dbg(), __VA_ARGS__)


#define LOG(type, level, ...)			\
	do {					\
		if (level & type) {		\
			pr_debug(__VA_ARGS__);	\
		}				\
	} while (0)

enum using_ion {
	DRAM_MODE,
	ION_BUF
};

enum perf_log {
	MIN_DIFF,
	SUM_DIFF,
	CUR_DIFF,
	F_COUNT,
	DIFF_COUNT_MAX
};

enum mtk_frame_st {
	FRAME_DONE = 1,
	SEQ_CHG_FRAME_DONE
};

#if 0
enum mtk_cinematic_cmdq_sbrc_err {
	SBRC_W_INC,
	SBRC_SKIP_DONE
};
#endif

enum mtk_cinematic_cmdq_channel {
	PIPE0_CH,
	PIPE1_CH,
	PIPE2_CH,
	PIPE3_CH,
	UNMUTE_CH,
	VSYNC_CH,
	WDMA_CH,
	RDMA_CH,
	DSI_UNDERRUN_CH,
	DSI_UNDERRUN_CPU_CH,
	SBRC_SKIP_DONE_CH,
	GPU_CH,
	CH_MAX
};

/* cinematic GCE events */
enum mtk_cinematic_cmdq_event {
	MTK_MM_CORE_WDMA0_FRAME_DONE,
	MTK_MM_CORE_WDMA1_FRAME_DONE,
	MTK_MM_CORE_WDMA2_FRAME_DONE,
	MTK_MM_CORE_WDMA3_FRAME_DONE,
	MTK_MM_CORE_WDMA0_TARGET_LINE_DONE,
	MTK_MM_CORE_WDMA1_TARGET_LINE_DONE,
	MTK_MM_CORE_WDMA2_TARGET_LINE_DONE,
	MTK_MM_CORE_WDMA3_TARGET_LINE_DONE,
	MTK_MM_CORE_RBFC0_R_INC,
	MTK_MM_CORE_RBFC1_R_INC,
	MTK_MM_CORE_RBFC2_R_INC,
	MTK_MM_CORE_RBFC3_R_INC,
	MTK_MM_CORE_SBRC_SKIP_DONE,
	MTK_MM_CORE_AH_EVENT_PIN_8,
	MTK_MM_CORE_DSI0_FRAME_DONE,
	MTK_MM_CORE_DSI1_FRAME_DONE,
	MTK_MM_CORE_TD_EVENT0,
	MTK_MM_CORE_TD_EVENT4,
	MTK_MM_CORE_TD_EVENT5,
	MTK_MM_CORE_WDMA0_SOF,
	MTK_MM_CORE_WDMA1_SOF,
	MTK_MM_CORE_WDMA2_SOF,
	MTK_MM_CORE_WDMA3_SOF,
	MTK_MM_CORE_CROP0_EOF,
	MTK_MM_CORE_SBRC_W_INC,
	MTK_MM_CORE_EVENT_MAX,
};

struct cc_queue {
	int capacity;
	int size;
	struct cc_job e;
	struct list_head list;
};

struct strip_diff {
	u32 *d_va[DIFF_COUNT_MAX];
	dma_addr_t d_pa[DIFF_COUNT_MAX];
};


struct mtk_cinematic_ctrl {
	struct mtk_cinematic_test *ct[CU_PATH_MAX];
	struct dentry *debugfs;
	struct cdev chardev;
	struct class *dev_class;
	struct device *dev;
	struct device *disp_dev;
	struct device *dev_sbrc;
	struct device *dev_sysram;
	struct device *dev_rbfc;
	struct mtk_mutex_res *vsync_mutex;
	struct mtk_common_buf *wdma_ion_buf[BUF_NUM];
	struct mtk_common_buf *rdma_ion_buf[BUF_NUM];
	struct mtk_disp_fb *disp_fb;
	struct completion sbrc_err;
	struct completion dsi_err;
	struct completion dp_stable;
	wait_queue_head_t vsync_wait;
	spinlock_t spinlock_irq;
	struct mutex poll_lock;
	struct mutex queue_lock;
	struct strip_diff sd;
	dev_t devt;
	enum pipe_cmd_list cmd;
	int act_inst;
	int frame_done;
	int vsync;
	struct cmdq_client *cmdq_client[CH_MAX];
	struct cmdq_pkt *cmdq_pkt[CH_MAX];
	u32 cmdq_events[MTK_MM_CORE_EVENT_MAX];
	bool dual_mode;
	bool displayed;
	bool sbrc_monitor;
	u32 sampling_rate;
	u32 frmtrk_dist;
	u32 skip_frame_cnt;
	u32 safe_line;
	u32 inserted_rdma_buf;
	u32 wdma_inc_cnt;
	u32 rdma_buf_id;
	u32 rbfc_subsys;
	u32 avg_wdma_comp_ratio;
	u32 max_wdma_comp_ratio;
	unsigned long long wdma_frame_cnt;
	unsigned long long wdma_sof_cnt;
	unsigned long long gpu_frame_cnt;
	int sysram_on;
	int sysram_base;
	int sysram_size;
};

int check_src_validation(uint w, uint h);
int mtk_cinematic_dispatch_cmd(struct mtk_cinematic_ctrl *cc);
int dump_next_frame(struct mtk_cinematic_ctrl *cc);
int dump_monitor_data(struct mtk_cinematic_ctrl *cc);
void mtk_cinematic_debugfs_init(struct mtk_cinematic_ctrl *cc);
void mtk_cinematic_debugfs_deinit(struct mtk_cinematic_ctrl *cc);
#endif
