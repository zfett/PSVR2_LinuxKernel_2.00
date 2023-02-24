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

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/vmalloc.h>
#include <linux/mutex.h>
#include <mtk_common_util.h>
#include "mtk_cinematic.h"
#include "mtk_cinematic_ctrl.h"
#include <mtk_disp.h>
#include <soc/mediatek/mtk_dsi.h>
#include <soc/mediatek/mtk_mmsys_cfg.h>
#include <soc/mediatek/mtk_mutex.h>
#include <soc/mediatek/mtk_slicer.h>
#include <soc/mediatek/mtk_wdma.h>
#include <soc/mediatek/mtk_rdma.h>
#include <linux/list.h>
#include <soc/mediatek/mtk_rbfc.h>
#include <soc/mediatek/mtk_sbrc.h>
#include <uapi/drm/drm_fourcc.h>
#include <dt-bindings/display/mt3612-display.h>
#include <dt-bindings/gce/mt3612-gce.h>

#ifdef CONFIG_MTK_GPU_DEDICATED_FW_MEMORY
#define GPU_SRAM_SIZE (144 * 1024)
#endif

/* reduce capture data */
/*#define REDUCE_W_OP_CNT*/
#define GPU_DRAM_MODE 0
#define GPU_SRAM_STRIP_MODE 1
#define GPU_DRAM_STRIP_MODE 2
#define INSTRUCT_SIZE 8

/* timeout waiting for the dp to respond */
#define DP_TIMEOUT (msecs_to_jiffies(300000))
/* timeout waiting for notifying user space callback */
#define CB_TIMEOUT (msecs_to_jiffies(2000))

static char *read_buffer;
struct completion read_complete;
struct semaphore event_sema;
static int read_size;
#if 0
void __iomem *reg;
#endif

static struct task_struct *cc_thread;
static struct task_struct *cc_dsi_thread;
static struct task_struct *cc_err_thread;
static struct task_struct *cc_vsync_thread;
static struct task_struct *cc_sof_thread;
static struct task_struct *cc_rdma_thread;

static struct cc_queue *mtk_jobs_q;
struct cmdq_operand *left_operand;
struct cmdq_operand *right_operand;

/* record sbrc performance */
/* diff cnt */
u32 *rec_va_d;
dma_addr_t rec_pa_d;
/* w_op_cnt */
u32 *rec_va_w;
dma_addr_t rec_pa_w;
/* r_op_cnt */
u32 *rec_va_r;
dma_addr_t rec_pa_r;
/* end of sbrc record */

/* w_op_cnt */
u32 *rec_va_w1;
dma_addr_t rec_pa_w1;
/* rdma_sta */
u32 *rec_va_rdma_st;
dma_addr_t rec_pa_rdma_st;

/* dsi_ng_cnt */
u32 *rec_va_dnc;
dma_addr_t rec_pa_dnc;
/* dsi input counter recorder */
u32 *rec_va_dsi_in;
dma_addr_t rec_pa_dsi_in;
/* dsi output counter recorder */
u32 *rec_va_dsi_out;
dma_addr_t rec_pa_dsi_out;

/* skip next dsi out counter check */
u32 *skip_next_va;
dma_addr_t skip_next_pa;

#define SBRC_FORCE_READ_CTRL
#ifdef SBRC_FORCE_READ_CTRL
/* forced buf id */
u32 *forced_buf_id_va;
dma_addr_t forced_buf_id_pa;
#endif

union cmd_data_t {
	struct mtk_common_buf_handle handle;
	struct mtk_common_put_handle put_handle;
	struct mtk_common_set_buf set_buf;
	struct cinematic_parameter para;
	struct cc_job usr_job;
	enum pipe_cmd_list cmd;
	int fb_id;
	unsigned long long wdma_frame_cnt;
};

static struct mtk_cinematic_test *get_active_pipe(struct mtk_cinematic_ctrl *cc)
{
	if (IS_ERR(cc->ct[cc->act_inst])) {
		dev_err(cc->dev, "get pipe err %ld\n",
			PTR_ERR(cc->ct[cc->act_inst]));
		return NULL;
	} else
		return cc->ct[cc->act_inst];
}

static int enqueue(struct cc_queue *Q, struct cc_job element)
{
	struct cc_queue *newQ;

	if (Q->size == Q->capacity) {
		pr_debug("Queue is Full. No element added.\n");
		return -EPERM;
	}

	Q->size++;
	newQ = kzalloc(sizeof(struct cc_queue), GFP_KERNEL);
	newQ->e = element;
	list_add_tail(&(newQ->list), &(Q->list));

	return 0;
}

static int dequeue(struct cc_queue *Q)
{
	struct cc_queue *tmp;

	if (list_empty(&(Q->list))) {
		pr_debug("Queue is Empty.\n");
		return -EPERM;
	}

	Q->size--;
	tmp = list_entry(Q->list.next, struct cc_queue, list);
	list_del(Q->list.next);
	kfree(tmp);

	return 0;
}

static int receive_job(struct cc_queue *Q, struct cc_job *rsz_jobs)
{
	struct cc_queue *first_element;
	struct list_head *first;

	if (list_empty(&(Q->list)))
		return -EPERM;

	first = Q->list.next;
	first_element = list_entry(first, struct cc_queue, list);

	*rsz_jobs = first_element->e;

	return 0;
}

static void flush_queue(struct cc_queue *Q)
{
	while (!list_empty(&(Q->list)))
		dequeue(Q);
}

int check_src_validation(uint w, uint h)
{
	/* check src timing combination */
	if (w == 640 && h == 480)
		return 0;
	else if (w == 960 && h == 2160)
		return 0;
	else if (w == 1280 && h == 720)
		return 0;
	else if (w == 1280 && h == 1470)
		return 0;
	else if (w == 1920 && h == 1080)
		return 0;
	else if (w == 1920 && h == 2205)
		return 0;
	else if (w == 1080 && h == 1920)
		return 0;
	else if (w == 3840 && h == 2160)
		return 0;
	else if (w == 4000 && h == 2040)
		return 0;
	else
		return -1;

}
EXPORT_SYMBOL(check_src_validation);

void mtk_cinematic_ctrl_cb(struct mtk_mmsys_cb_data *data)
{
	struct mtk_cinematic_ctrl *cc =
		(struct mtk_cinematic_ctrl *)data->priv_data;
	unsigned long flags;

	dev_dbg(cc->dev,
		"[wdma_cb] data.part_id: %d, data.status: %d\n",
		data->part_id, data->status);

	/* should NOT use this */
	if (data->status & MTK_WDMA_FRAME_COMPLETE) {
		spin_lock_irqsave(&cc->spinlock_irq, flags);
		cc->frame_done = FRAME_DONE;
		cc->vsync = 1;
		spin_unlock_irqrestore(&cc->spinlock_irq, flags);

		wake_up_interruptible(&cc->vsync_wait);
	}
}

static int mtk_cinematic_cb_event(struct mtk_cinematic_test *ct, int type,
				  void *arg, int size)
{
	int *type_addr = (int *)read_buffer;
	int total_size = size + sizeof(type);

	/* prevent buffer overflow */
	if (total_size > CB_BUF_SIZE)
		return -EINVAL;

	if (IS_ERR(ct))
		return -EINVAL;

	/* only callback to user when gpu mode */
	if (ct->para.gp.bypass)
		return -EINVAL;

	/* make sure event not loss */
	down(&event_sema);
	*type_addr = type;

	/*
	 *   4 bytes          up to 124 bytes
	 * [event_type] + [ ----private data ----]
	 *
	 */
	if (arg && size)
		memcpy(read_buffer + sizeof(type), arg, size);
	read_size += total_size;

	/* awake user */
	complete(&read_complete);

	return 0;
}

static int mtk_cinematic_config_rbfc(struct mtk_cinematic_ctrl *cc,
				     enum MTK_RBFC_MODE mode)
{
	struct mtk_cinematic_test *ct;
	struct mtk_common_buf *buf_handle;
	dma_addr_t pa = 0;
	int ret = 0;
	int bpp = 0;
	u32 w, h, rbfc_in_pitch;
	u32 strip_total_h;
	u32 r_th[1] = {1}, w_th[1] = {(0 << 16) | 1};
	u32 rbfc_act_nr[2] = {0x1, 0xf};

	ct = get_active_pipe(cc);

	if (IS_ERR(ct))
		return -EINVAL;
#ifdef CONFIG_MACH_MT3612_A0
	w = ct->para.gp.out_w / 4;
#else
	w = ct->para.gp.out_w;
#endif
	h = ct->para.gp.out_h;
	bpp = mtk_format_to_bpp(ct->para.gp.out_fmt);

	strip_total_h = ct->para.gp.strip_h * ct->para.gp.strip_buf_num;

	if (ct->para.gp.strip_mode == GPU_DRAM_STRIP_MODE) {
		buf_handle = cc->rdma_ion_buf[0];
		pa = buf_handle->dma_addr;
	}

#ifdef CONFIG_MTK_GPU_DEDICATED_FW_MEMORY
	if (ct->para.gp.strip_mode == GPU_SRAM_STRIP_MODE)
		pa += GPU_SRAM_SIZE;
#endif
	rbfc_in_pitch = ct->para.gp.out_w * bpp >> 3;

	/* sram/dram alignment */
	if (ct->para.gp.strip_mode == GPU_SRAM_STRIP_MODE)
		rbfc_in_pitch = roundup(rbfc_in_pitch, 16);
#if 0
	else
		rbfc_in_pitch = roundup(rbfc_in_pitch, 256);
#endif

	ret = mtk_rbfc_set_plane_num(cc->dev_rbfc, 1);
	if (unlikely(ret))
		dev_err(cc->dev,
			"mtk_rbfc_set_plane_num NG!! %d\n", ret);

	ret = mtk_rbfc_set_active_num(cc->dev_rbfc, NULL, rbfc_act_nr);
	if (unlikely(ret))
		dev_err(cc->dev,
			"mtk_rbfc_set_active_num NG!! %d\n", ret);

	ret = mtk_rbfc_set_region_multi(cc->dev_rbfc, NULL, 0, w, h);
	if (unlikely(ret))
		dev_err(cc->dev,
			"mtk_rbfc_set_region_multi NG!! %d\n", ret);

	if (ct->para.gp.strip_mode == GPU_SRAM_STRIP_MODE)
		ret = mtk_rbfc_set_target(cc->dev_rbfc, NULL,
					  MTK_RBFC_TO_SYSRAM);
	else
		ret = mtk_rbfc_set_target(cc->dev_rbfc, NULL, MTK_RBFC_TO_DRAM);
	if (unlikely(ret))
		dev_err(cc->dev,
			"mtk_rbfc_set_target NG!! %d\n", ret);

	if (mode == MTK_RBFC_NORMAL_MODE) {
		ret = mtk_rbfc_set_read_thd(cc->dev_rbfc, NULL, r_th);
		if (unlikely(ret))
			dev_err(cc->dev,
				"mtk_rbfc_set_read_thd NG!! %d\n", ret);

		ret = mtk_rbfc_set_write_thd(cc->dev_rbfc, NULL, w_th);
		if (unlikely(ret))
			dev_err(cc->dev,
				"mtk_rbfc_set_write_thd NG!! %d\n", ret);

		ret = mtk_rbfc_set_ring_buf_multi(cc->dev_rbfc,
						  NULL,
						  0, pa,
						  rbfc_in_pitch,
						  strip_total_h);
		if (unlikely(ret))
			dev_err(cc->dev,
				"mtk_rbfc_set_ring_buf_multi NG!! %d\n", ret);

		ret = mtk_rbfc_start_mode(cc->dev_rbfc,
			NULL, MTK_RBFC_NORMAL_MODE);
		if (unlikely(ret))
			dev_err(cc->dev,
				"mtk_rbfc_start_mode NG!! %d\n", ret);

		dev_dbg(cc->dev, "enable rbfc\n");
	} else {
		ret = mtk_rbfc_start_mode(cc->dev_rbfc,
			NULL, MTK_RBFC_DISABLE_MODE);
		if (unlikely(ret))
			dev_err(cc->dev,
				"mtk_rbfc_start_mode NG!! %d\n", ret);

		dev_dbg(cc->dev, "disable rbfc\n");
	}

	return ret;
}

static int mtk_cinematic_config_sbrc(struct mtk_cinematic_ctrl *cc)
{
	struct mtk_cinematic_test *ct;
	int ret = 0;
	int w, h, w_in_bytes;
	int bpp;

	ct = get_active_pipe(cc);

	if (IS_ERR(ct))
		return -EINVAL;

	w = ct->para.gp.out_w;
	h = ct->para.gp.out_h;
	bpp = mtk_format_to_bpp(ct->para.gp.out_fmt);

	ret = mtk_sbrc_drv_init(cc->dev_sbrc);
	if (unlikely(ret))
		dev_err(cc->dev, "Failed to initialize sbrc driver\n");

	ret = mtk_sbrc_set_buffer_num(cc->dev_sbrc, ct->para.gp.strip_buf_num);
	if (unlikely(ret))
		dev_err(cc->dev, "### cinematic set sbrc buffer num NG!!\n");

	ret = mtk_sbrc_set_buffer_depth(cc->dev_sbrc, h, ct->para.gp.strip_h);
	if (unlikely(ret))
		dev_err(cc->dev, "### cinematic set sbrc buffer depth NG!!\n");

	w_in_bytes = w * bpp >> 3;

	ret = mtk_sbrc_set_buffer_size(cc->dev_sbrc, w_in_bytes);
	if (unlikely(ret))
		dev_err(cc->dev, "### cinematic set sbrc buffer size NG!!\n");

	return ret;
}

static void disable_sbrc_monitor(struct cmdq_cb_data data)
{
	struct mtk_cinematic_ctrl *cc = data.data;
	struct mtk_cinematic_test *ct;
	int inst;

	ct = get_active_pipe(cc);

	if (IS_ERR(ct)) {
		pr_err("sbrc monitor cb, ct handle null\n");
		return;
	}

	if (cc->cmd != DEINIT_PATH)
		dev_dbg(cc->dev, "NOT Deinit flow but GPU_CH callbacked!!\n");

	inst = cc->act_inst;
	cmdq_pkt_destroy(cc->cmdq_pkt[inst]);
	dev_dbg(cc->dev, "== deinit GPU_CH destroyed ==\n");
}

static void mtk_cinematic_dsi_underrun(struct cmdq_cb_data data)
{
	struct mtk_cinematic_ctrl *cc = data.data;
	struct mtk_cinematic_test *ct;
	struct device *dsi_dev;

	ct = get_active_pipe(cc);

	if (IS_ERR(ct)) {
		pr_err("sbrc err cb, ct handle null\n");
		return;
	}

	if (cc->cmd == DEINIT_PATH) {
		dev_dbg(cc->dev, "deinit flow, unlock d_err monitor thread\n");
		goto BYPASS_DSI_ERR;
	}

	dsi_dev = mtk_dispsys_get_dsi(cc->disp_dev);

	dev_dbg(cc->dev, "=== AH EVENT (dsi underflow) [%d] ===",
		*(rec_va_dnc));
	dev_dbg(cc->dev, "w_op cnt = %d\n", *(rec_va_w1));

	if (unlikely((*(rec_va_rdma_st) >> 8) == 0x1))
		dev_err(cc->dev, "@@@ rdma_sta in idle !! @@@");
	else
		dev_dbg(cc->dev, "rdma_sta = 0x%x\n", *(rec_va_rdma_st) >> 8);

	dev_dbg(cc->dev, "dsi_in   = %d\n", *(rec_va_dsi_in) >> 16);
	dev_dbg(cc->dev, "dsi_out  = %d\n\n", *(rec_va_dsi_out));

BYPASS_DSI_ERR:
	complete(&cc->dsi_err);
}

static void disable_dsi_monitor(struct cmdq_cb_data data)
{
	struct mtk_cinematic_ctrl *cc = data.data;
	struct mtk_cinematic_test *ct;

	ct = get_active_pipe(cc);

	if (IS_ERR(ct)) {
		pr_err("gce dsi err cb, ct handle null\n");
		return;
	}

	if (cc->cmd != DEINIT_PATH)
		dev_err(cc->dev, "NOT Deinit flow but DSI_CH callbacked!!\n");

	cmdq_pkt_destroy(cc->cmdq_pkt[DSI_UNDERRUN_CH]);
	dev_dbg(cc->dev, "== deinit GCE_DSI_CH destroyed ==\n");
}

static void mtk_cinematic_sbrc_err_handle(struct cmdq_cb_data data)
{
	struct mtk_cinematic_ctrl *cc = data.data;
	struct mtk_cinematic_test *ct;
#if 0
	u32 test_rg;
#endif

	ct = get_active_pipe(cc);

	if (IS_ERR(ct)) {
		pr_err("sbrc err cb, ct handle null\n");
		return;
	}

	if (cc->cmd == DEINIT_PATH) {
		dev_dbg(cc->dev, "deinit flow, unlock err handle thread\n");
		goto EXIT_SKIP_DONE;
	}
#if 0
	test_rg = readl(reg + 0x100);
#endif

#ifndef SBRC_FORCE_READ_CTRL
	dev_dbg(cc->dev, "=== sbrc skip frame done[%d] ===\n", *(skip_next_va));
#else
	dev_dbg(cc->dev, "=== skip frame done line_cnt[%d]\tid[%d] ===\n",
		*(skip_next_va), (*(forced_buf_id_va)) >> 1);
#endif

	if (cc->cmd != DEINIT_PATH)
		cc->skip_frame_cnt++;

EXIT_SKIP_DONE:
	complete(&cc->sbrc_err);
}

static int mtk_cinematic_init_srbc_err_monitor(struct mtk_cinematic_ctrl *cc)
{
	struct mtk_cinematic_test *ct;
	struct mtk_mutex_res *disp_mutex;
	int ret = 0;

	ct = get_active_pipe(cc);

	if (IS_ERR(ct))
		return -EINVAL;

	disp_mutex = mtk_dispsys_get_mutex_res(cc->disp_dev, MUTEX_DISP);

	cc->sbrc_monitor = true;
#if 0
	mtk_mutex_add_monitor(disp_mutex,
			      MUTEX_MMSYS_STRIP_BUFFER_R_UNFINISH_0, NULL);
	mtk_mutex_add_monitor(disp_mutex,
			      MUTEX_MMSYS_STRIP_BUFFER_R_UNFINISH_1, NULL);
	mtk_mutex_add_monitor(disp_mutex,
			      MUTEX_MMSYS_STRIP_BUFFER_R_UNFINISH_2, NULL);
	mtk_mutex_add_monitor(disp_mutex,
			      MUTEX_MMSYS_STRIP_BUFFER_R_UNFINISH_3, NULL);

	ret |= mtk_mutex_add_monitor(disp_mutex,
				     MUTEX_MMSYS_STRIP_BUFFER_W_UNFINISH,
				     NULL);
#endif

	ret |= mtk_mutex_add_monitor(disp_mutex, MUTEX_MMSYS_DSI0_UNDERFLOW,
				     NULL);
	ret |= mtk_mutex_add_monitor(disp_mutex, MUTEX_MMSYS_DSI1_UNDERFLOW,
				     NULL);
	ret |= mtk_mutex_add_monitor(disp_mutex, MUTEX_MMSYS_DSI2_UNDERFLOW,
				     NULL);
	ret |= mtk_mutex_add_monitor(disp_mutex, MUTEX_MMSYS_DSI3_UNDERFLOW,
				     NULL);

	if (ct->para.sbrc_recover) {

		ret |= mtk_mutex_error_monitor_enable(disp_mutex, NULL, false);

		ret |= mtk_mutex_add_to_ext_signal(
			disp_mutex, MMCORE_SBRC_BYPASS_EXT_SIGNAL, NULL);

		ret |= mtk_mutex_add_to_ext_signal(
			disp_mutex, MMCORE_DSI_0_MUTE_EXT_SIGNAL, NULL);

		ret |= mtk_mutex_add_to_ext_signal(
			disp_mutex, MMCORE_DSI_1_MUTE_EXT_SIGNAL, NULL);

		ret |= mtk_mutex_add_to_ext_signal(
			disp_mutex, MMCORE_DSI_2_MUTE_EXT_SIGNAL, NULL);

		ret |= mtk_mutex_add_to_ext_signal(
			disp_mutex, MMCORE_DSI_3_MUTE_EXT_SIGNAL, NULL);
	} else {
		ret |= mtk_mutex_error_monitor_enable(disp_mutex, NULL, false);
	}
	return ret;
}

static int mtk_cinematic_deinit_srbc_err_monitor(struct mtk_cinematic_ctrl *cc)
{
	struct mtk_cinematic_test *ct;
	struct mtk_mutex_res *disp_mutex;
	int ret = 0;

	ct = get_active_pipe(cc);

	if (IS_ERR(ct))
		return -EINVAL;

	disp_mutex = mtk_dispsys_get_mutex_res(cc->disp_dev, MUTEX_DISP);

	cc->sbrc_monitor = false;
#if 0
	mtk_mutex_remove_monitor(disp_mutex,
				 MUTEX_MMSYS_STRIP_BUFFER_R_UNFINISH_0, NULL);
	mtk_mutex_remove_monitor(disp_mutex,
				 MUTEX_MMSYS_STRIP_BUFFER_R_UNFINISH_1, NULL);
	mtk_mutex_remove_monitor(disp_mutex,
				 MUTEX_MMSYS_STRIP_BUFFER_R_UNFINISH_2, NULL);
	mtk_mutex_remove_monitor(disp_mutex,
				 MUTEX_MMSYS_STRIP_BUFFER_R_UNFINISH_3, NULL);
	mtk_mutex_remove_monitor(disp_mutex,
			      MUTEX_MMSYS_STRIP_BUFFER_W_UNFINISH,
			      NULL);
#endif
	ret |= mtk_mutex_remove_monitor(disp_mutex, MUTEX_MMSYS_DSI0_UNDERFLOW,
					NULL);
	ret |= mtk_mutex_remove_monitor(disp_mutex, MUTEX_MMSYS_DSI1_UNDERFLOW,
					NULL);
	ret |= mtk_mutex_remove_monitor(disp_mutex, MUTEX_MMSYS_DSI2_UNDERFLOW,
					NULL);
	ret |= mtk_mutex_remove_monitor(disp_mutex, MUTEX_MMSYS_DSI3_UNDERFLOW,
					NULL);
	ret |= mtk_mutex_error_monitor_disable(disp_mutex, NULL);

	if (ct->para.sbrc_recover) {
		ret |= mtk_mutex_remove_from_ext_signal(
			disp_mutex, MMCORE_SBRC_BYPASS_EXT_SIGNAL, NULL);

		ret |= mtk_mutex_remove_from_ext_signal(
			disp_mutex, MMCORE_DSI_0_MUTE_EXT_SIGNAL, NULL);

		ret |= mtk_mutex_remove_from_ext_signal(
			disp_mutex, MMCORE_DSI_1_MUTE_EXT_SIGNAL, NULL);

		ret |= mtk_mutex_remove_from_ext_signal(
			disp_mutex, MMCORE_DSI_2_MUTE_EXT_SIGNAL, NULL);

		ret |= mtk_mutex_remove_from_ext_signal(
			disp_mutex, MMCORE_DSI_3_MUTE_EXT_SIGNAL, NULL);
	}
	return ret;
}

static int get_wdma_frame_done_id(struct mtk_cinematic_ctrl *cc)
{
	if (IS_ERR(cc))
		return -EINVAL;

	switch (cc->act_inst) {
	case PIPE_0:
		return cc->cmdq_events[MTK_MM_CORE_WDMA0_FRAME_DONE];
	case PIPE_1:
		return cc->cmdq_events[MTK_MM_CORE_WDMA1_FRAME_DONE];
	case PIPE_2:
		return cc->cmdq_events[MTK_MM_CORE_WDMA2_FRAME_DONE];
	case PIPE_3:
		return cc->cmdq_events[MTK_MM_CORE_WDMA3_FRAME_DONE];
	default:
		/* should never go here */
		dev_err(cc->dev, "error active inst\n");
		return -EINVAL;
	}
}

static int get_wdma_sof_id(struct mtk_cinematic_ctrl *cc)
{
	if (IS_ERR(cc))
		return -EINVAL;

	switch (cc->act_inst) {
	case PIPE_0:
		return cc->cmdq_events[MTK_MM_CORE_WDMA0_SOF];
	case PIPE_1:
		return cc->cmdq_events[MTK_MM_CORE_WDMA1_SOF];
	case PIPE_2:
		return cc->cmdq_events[MTK_MM_CORE_WDMA2_SOF];
	case PIPE_3:
		return cc->cmdq_events[MTK_MM_CORE_WDMA3_SOF];
	default:
		/* should never go here */
		dev_err(cc->dev, "error active inst\n");
		return -EINVAL;
	}
}

int dump_monitor_data(struct mtk_cinematic_ctrl *cc)
{
	u32 total_rec = 0, i = 0;

	if (IS_ERR(cc))
		return -EINVAL;

#ifdef CACHED_BUF
	dma_sync_single_for_cpu(cc->dev, rec_pa_d, sizeof(u32) * REC_NUM,
				DMA_FROM_DEVICE);
	dma_sync_single_for_cpu(cc->dev, rec_pa_r, sizeof(u32) * REC_NUM,
				DMA_FROM_DEVICE);
	dma_sync_single_for_cpu(cc->dev, rec_pa_w, sizeof(u32) * REC_NUM,
				DMA_FROM_DEVICE);
	dma_sync_single_for_cpu(cc->dev, cc->sd.d_pa[F_COUNT],
				sizeof(u32) * REC_NUM, DMA_FROM_DEVICE);
#endif

	total_rec = *cc->sd.d_va[F_COUNT];

	for (i = 0; i < total_rec; i++) {
		pr_err("[%d] diff     = %d\n", i, *(rec_va_d + i));
#ifdef REDUCE_W_OP_CNT
		pr_err("[%d] w_op cnt = %d\n", i, *(rec_va_d + i) +
		       *(rec_va_r + i));
#else
		pr_err("[%d] w_op cnt = %d\n", i, *(rec_va_w + i));
#endif
		pr_err("[%d] r_op cnt = %d\n", i, *(rec_va_r + i));
		pr_err("\n");
	}
	return 0;
}
EXPORT_SYMBOL(dump_monitor_data);

static int get_wdma_target_line_id(struct mtk_cinematic_ctrl *cc)
{
	if (IS_ERR(cc))
		return -EINVAL;

	switch (cc->act_inst) {
	case PIPE_0:
		return cc->cmdq_events[MTK_MM_CORE_WDMA0_TARGET_LINE_DONE];
	case PIPE_1:
		return cc->cmdq_events[MTK_MM_CORE_WDMA1_TARGET_LINE_DONE];
	case PIPE_2:
		return cc->cmdq_events[MTK_MM_CORE_WDMA2_TARGET_LINE_DONE];
	case PIPE_3:
		return cc->cmdq_events[MTK_MM_CORE_WDMA3_TARGET_LINE_DONE];
	default:
		/* should never go here */
		dev_err(cc->dev, "error active inst\n");
		return -EINVAL;
	}
}

#ifdef CONFIG_MACH_MT3612_A0
static int mtk_cinematic_set_frmtrk(struct mtk_cinematic_ctrl *cc)
{
	struct device *dsi_dev;
	struct mtk_cinematic_test *ct;
	u32 dp_vtotal, dp_fps, lcm_vtotal, line_time;
	u32 step1, step2, turbo_win, lock_win, target_line, hlen;
	u32 i_fps, o_fps;
	u32 ratio[RATIO_SIZE];
	int ret = 0;

	if (IS_ERR(cc))
		return -EINVAL;

	ct = get_active_pipe(cc);

	if (IS_ERR(ct))
		return -EINVAL;

	dsi_dev = mtk_dispsys_get_dsi(cc->disp_dev);

	if (IS_ERR(dsi_dev))
		return -EINVAL;

	if (!ct->dp_video_set)
		return -EINVAL;

	/* input and output fps */
	o_fps = ct->para.disp_fps;
	i_fps = ct->para.input_fps;

	/* calc heln */
	dp_vtotal = ct->dp_info.vid_timing_msa.v_total;
	dp_fps = ct->dp_info.frame_rate;

	ret = mtk_dsi_get_lcm_vtotal(dsi_dev, &lcm_vtotal);
	if (unlikely(ret)) {
		dev_dbg(cc->dev, "DSI get vtotal NG!!\n");
		return ret;
	}

	hlen = dp_fps * dp_vtotal;

	/* 1000000/fps/vtotal = per line time in us*/
	switch (o_fps) {
	case FPS_120:
		/* fall through */
	case FPS_119:
		/* 3.78 us per line */
		line_time = 1000000 * 100 / FPS_120 / lcm_vtotal;
		break;
	case FPS_90:
		/* fall through */
	case FPS_89:
		/* 5.05 us per line */
		line_time = 1000000 * 100 / FPS_90 / lcm_vtotal;
		break;
	case FPS_60:
		/* fall through */
	case FPS_59:
		/* 7.57 us per line */
		line_time = 1000000 * 100 / FPS_60 / lcm_vtotal;
		break;
	default:
		line_time = 757;
		dev_err(cc->dev, "=== disp fps wrong !!! ===\n");
	}

	ret = mtk_get_fps_ratio(ratio, RATIO_SIZE, i_fps, o_fps);
	if (unlikely(ret)) {
		dev_dbg(cc->dev, "get fps ratio NG!!\n");
		return ret;
	}

	if (ratio[1] >= 1 && ratio[1] <= 3) {

		/* 60:60 / 60:90 / 60:120 / 120:120 */
		step1 = dp_fps * 3 / 2 / ratio[1];
		step2 = dp_fps * 3 / ratio[1];
		lock_win = 2;
		turbo_win = 8;

	} else if (ratio[1] >= 4 && ratio[1] <= 6) {

		/* 24:60 / 24:120 / 50:60 */
		step1 = dp_fps * 3 / ratio[1];
		step2 = dp_fps * 6 / ratio[1];
		lock_win = 4;
		turbo_win = 16;

	} else if (ratio[1] >= 7 && ratio[1] <= 15) {

		/* 50:90 / 50:120 / 24:90 */
		step1 = dp_fps * 6 / ratio[1];
		step2 = dp_fps * 12 / ratio[1];

		lock_win = 8;
		turbo_win = 32;
	}

	/* lcm_vtotal - distance(dsi unit) */
	target_line = lcm_vtotal - ((cc->frmtrk_dist * 100) / line_time);
	ct->sof_line_diff = (cc->frmtrk_dist * 100) / line_time;
	ct->lcm_vtotal = lcm_vtotal;
	ct->frmtrk_target_line = target_line;

	dev_dbg(cc->dev, "=========== frame tracking settings ==========\n");
	dev_dbg(cc->dev, "x_latch = %d, input_fps = %d, output_fps = %d\n",
		ratio[0], i_fps, o_fps);

	dev_dbg(cc->dev, "line_time = %d, lcm_vtotal = %d\n", line_time,
		lcm_vtotal);
	dev_dbg(cc->dev, "step1 = %d, step2 = %d, y = %d\n", step1, step2,
		ratio[1]);
	dev_dbg(cc->dev, "lock_win = %d, turbo_win = %d\n", lock_win,
		turbo_win);

	ret = mtk_dispsys_set_ddds_frmtrk_param(cc->disp_dev, hlen, step1,
		step2, target_line, lock_win, turbo_win, ratio[0]);
	if (unlikely(ret)) {
		dev_dbg(cc->dev, "dispsys_set_ddds_frmtrk_param NG!!\n");
		return ret;
	}

	return 0;
}
#endif

static int mtk_cinematic_cfg_disp(struct mtk_cinematic_ctrl *cc)
{
	struct mtk_disp_para para;
	struct mtk_cinematic_test *ct;
	int ret = 0;

	ct = get_active_pipe(cc);

	if (IS_ERR(ct))
		return -EINVAL;

	/*
	 * strip dram/sram mode need to set scenario = DISP_SRAM_MODE
	 * for disp_pipe to set mutex delay = 12T
	 */

	if ((ct->para.gp.bypass == 0) && ct->para.gp.strip_mode)
		para.scenario = DISP_SRAM_MODE;
	else
		para.scenario = DISP_DRAM_MODE;

	/* assign gpu output format to rdma */
	para.input_format = ct->para.gp.out_fmt;
	para.output_format = ct->para.dsi_out_fmt;

	/* lhc active from rdma */
	if (ct->para.lhc_en && (ct->para.lhc_sync_path == FROM_RDMA))
		para.lhc_enable = true;
	else
		para.lhc_enable = false;

	para.dsc_enable = ct->para.dsc_enable;
	para.encode_rate = ct->para.encode_rate;
	para.dsc_passthru = false;
	para.dump_enable = false;

	if (ct->para.pvric_en & COMPRESS_OUTPUT_ENABLE)
		para.fbdc_enable = true;
	else
		para.fbdc_enable = false;

#ifdef CONFIG_MACH_MT3612_A0
	para.input_width = SOC_PANEL_W;
	para.input_height = SOC_PANEL_H;
	para.disp_fps = ct->para.disp_fps;
	para.dp_enable = ct->para.input_from_dp;
	para.frmtrk = ct->para.frmtrk_en;
	para.edid_sel = ct->para.edid_sel;

	dev_dbg(cc->dev, "=== config dispsys ===\n");
	dev_dbg(cc->dev, "panel w/h = %d / %d\n",
		para.input_width, para.input_height);
	dev_dbg(cc->dev, "disp.fps = %d,  disp.dp_enable %d, disp.frmtrk %d\n",
		para.disp_fps, para.dp_enable, para.frmtrk);
#else
	/* config RDMA width/height */
	para.input_width = FPGA_PANEL_W;

	if ((ct->para.gp.bypass == 0) ||
			(ct->para.input_height > FPGA_PANEL_H))
		para.input_height = FPGA_PANEL_H;
	else
		para.input_height = ct->para.input_height;
#endif
	/* config dispsys */
	ret = mtk_dispsys_config(cc->disp_dev, &para);
	if (unlikely(ret)) {
		dev_err(cc->dev, "mtk_dispsys config NG!!\n");
		return ret;
	}
	return 0;
}

static int mtk_cinematic_enable_disp(struct mtk_cinematic_ctrl *cc)
{
	struct mtk_cinematic_test *ct;
#ifdef CONFIG_MACH_MT3612_A0
	struct device *dsi_dev;
#endif
	int ret = 0;

	ct = get_active_pipe(cc);

	if (IS_ERR(ct))
		return -EINVAL;

#ifdef CONFIG_MACH_MT3612_A0
	dsi_dev = mtk_dispsys_get_dsi(cc->disp_dev);

#ifdef REDUCE_VB
	mtk_dsi_set_lcm_vblanking(dsi_dev, 1, 13, 6);
#else
	mtk_dsi_set_lcm_vblanking(dsi_dev, 1, 151, 8);
#endif
	if (ct->para.frmtrk_en) {

		ret = mtk_cinematic_set_frmtrk(cc);
		if (unlikely(ret)) {
			dev_err(cc->dev, "mtk_frmtrk config NG!!\n");
			return ret;
		}

		ret = mtk_cinematic_set_frmtrk_height(ct);
		if (unlikely(ret)) {
			dev_err(cc->dev, "mtk_slicr frm-trk config NG!!\n");
			return ret;
		}

	}
#endif
	/*
	 * plz make sure rdma buf addr has been set before goes here
	 * if is strip sram mode, need to set addr = 0x0 to RDMA
	 * RBFC will help to set related addr for RDMA
	 */
	if (ct->para.gp.strip_mode) {
		ret = mtk_dispsys_config_srcaddr(cc->disp_dev, 0, NULL);
		if (unlikely(ret)) {
			dev_err(cc->dev, "mtk_dispsys config addr NG!!\n");
			return ret;
		}
	}

	dev_dbg(cc->dev, "=== init disp pipe & mute dsi ===\n");

	if (ct->para.pvric_en & COMPRESS_OUTPUT_ENABLE)
		ret = mtk_dispsys_enable(cc->disp_dev, 1, 1);
	else
		ret = mtk_dispsys_enable(cc->disp_dev, 1, 0);

	if (unlikely(ret)) {
		dev_err(cc->dev, "mtk_dispsys enable  NG!!\n");
		return ret;
	}

	return 0;
}

static int cc_monitor_dsi_task(void *data)
{
	struct mtk_cinematic_ctrl *cc;
	struct mtk_cinematic_test *ct;
	struct cmdq_pkt *cmdq_pkt;
	int rc;

	cc = (struct mtk_cinematic_ctrl *)data;
	ct = get_active_pipe(cc);

	if (IS_ERR(ct))
		return -EINVAL;


	cmdq_pkt_create(&cmdq_pkt);
	cmdq_pkt_clear_event(cmdq_pkt, GCE_SW_TOKEN_UNDERFLOW);
	cmdq_pkt_wfe(cmdq_pkt, GCE_SW_TOKEN_UNDERFLOW);

	init_completion(&cc->dsi_err);

	while (cc->sbrc_monitor) {

		cmdq_pkt_flush_async(cc->cmdq_client[DSI_UNDERRUN_CPU_CH],
				     cmdq_pkt, mtk_cinematic_dsi_underrun, cc);

		rc = wait_for_completion_interruptible(&cc->dsi_err);
		if (rc < 0)
			goto EXIT_DSI_ERR;

		reinit_completion(&cc->dsi_err);

		if (cc->cmd != DEINIT_PATH)
			mtk_cinematic_cb_event(ct, CB_DSI_UNDERFLOW, NULL, 0);
	}

EXIT_DSI_ERR:
	cmdq_pkt_destroy(cmdq_pkt);
	dev_dbg(cc->dev, "=== dsi underflow monitor thread terminated ===\n");
	return 0;
}

static int enable_dsi_gce_monitor(struct mtk_cinematic_ctrl *cc)
{
	struct mtk_cinematic_test *ct;
	struct cmdq_pkt **cmdq_pkt;
	struct device *dma_dev;
	struct mtk_mutex_res *mutex;
	struct cmdq_operand left_operand;
	struct cmdq_operand right_operand;
	u32 d_err, ring_jump_offset;
	int ret = 0;

	ct = get_active_pipe(cc);

	if (IS_ERR(ct))
		return -EINVAL;

	cmdq_pkt = &cc->cmdq_pkt[DSI_UNDERRUN_CH];
	dma_dev = mtk_dispsys_get_rdma(cc->disp_dev);
	mutex = mtk_dispsys_get_mutex_res(cc->disp_dev, MUTEX_DISP);

	/* AH event mutex group 8 */
	d_err = cc->cmdq_events[MTK_MM_CORE_AH_EVENT_PIN_8];

	cmdq_pkt_create(cmdq_pkt);

	/* GCE while (1) */
	cmdq_pkt_assign_command(*cmdq_pkt, GCE_CPR10, rec_pa_dsi_out);
	cmdq_pkt_assign_command(*cmdq_pkt, GCE_CPR11, rec_pa_w1);
	cmdq_pkt_assign_command(*cmdq_pkt, GCE_CPR12, rec_pa_dnc);
	cmdq_pkt_assign_command(*cmdq_pkt, GCE_CPR13, rec_pa_rdma_st);
	cmdq_pkt_assign_command(*cmdq_pkt, GCE_CPR14, rec_pa_dsi_in);

	cmdq_pkt_clear_event(*cmdq_pkt, d_err);
	cmdq_pkt_wfe(*cmdq_pkt, d_err);

	/* notify cpu dsi underflow occurred */
	cmdq_pkt_set_token(*cmdq_pkt, GCE_SW_TOKEN_UNDERFLOW, SUBSYS_1028XXXX);

	/* count AH number, prevent cpu busy lost count */
	cmdq_pkt_logic_command(*cmdq_pkt, CMDQ_LOGIC_ADD, GCE_SPR0,
			       cmdq_operand_reg(&left_operand, GCE_SPR0),
			       cmdq_operand_immediate(&right_operand, 1));
	cmdq_pkt_store_reg(*cmdq_pkt, GCE_CPR12, GCE_SPR0, 0xffffffff);

	/* capture instant err status, for later cpu check  */
	/* rbfc w op */
	cmdq_pkt_read(*cmdq_pkt, SUBSYS_1402XXXX, 0x8000 | 0xb0, GCE_SPR2);
	cmdq_pkt_store_reg(*cmdq_pkt, GCE_CPR11, GCE_SPR2, 0x7fff);
	/* dsi in */
	cmdq_pkt_read(*cmdq_pkt, SUBSYS_1402XXXX, 0x0000 | 0x1c0, GCE_SPR2);
	cmdq_pkt_store_reg(*cmdq_pkt, GCE_CPR14, GCE_SPR2, 0xffffffff);
	/* dsi out */
	cmdq_pkt_read(*cmdq_pkt, SUBSYS_1402XXXX, 0x0000 | 0x198, GCE_SPR2);
	cmdq_pkt_store_reg(*cmdq_pkt, GCE_CPR10, GCE_SPR2, 0x7fff);
	/* rdma st */
	cmdq_pkt_read(*cmdq_pkt, SUBSYS_1400XXXX, 0x4000 | 0x408, GCE_SPR2);
	cmdq_pkt_store_reg(*cmdq_pkt, GCE_CPR13, GCE_SPR2, 0x7ffff);

	/* end of capture */
	/* reset rdma */
	ret = mtk_rdma_reset_split_start(dma_dev, *cmdq_pkt);
	if (unlikely(ret))
		dev_err(cc->dev, "mtk_rmda_reset_split start NG!!\n");
	/* make sure reset done */
	ret = mtk_rdma_reset_split_polling(dma_dev, *cmdq_pkt);
	if (unlikely(ret))
		dev_err(cc->dev, "mtk_rmda_reset_split poll NG!!\n");
	/* remove r_sof */
	ret = mtk_mutex_disable_ex(mutex, cmdq_pkt, false);
	if (unlikely(ret))
		dev_err(cc->dev, "mtk_mutex_disable_ex NG!!\n");

	ring_jump_offset = (2 * INSTRUCT_SIZE) -
			    cmdq_get_cmd_buf_size(*cmdq_pkt);

	cmdq_pkt_jump(*cmdq_pkt, ring_jump_offset);
	/* loop back */

	cmdq_pkt_flush_async(cc->cmdq_client[DSI_UNDERRUN_CH], *cmdq_pkt,
			     disable_dsi_monitor, cc);

	dev_dbg(cc->dev, "=== dsi underflow monitor GCE thread started ===\n");
	return 0;
}

static int cc_monitor_err_task(void *data)
{
	struct mtk_cinematic_ctrl *cc;
	struct mtk_cinematic_test *ct;
	struct cmdq_operand left_operand;
	struct cmdq_operand right_operand;

	struct cmdq_pkt *cmdq_pkt;
	struct mtk_mutex_res *mutex;
	u32 sbrc_skip_done;
	int rc;

	cc = (struct mtk_cinematic_ctrl *)data;
	ct = get_active_pipe(cc);

	if (IS_ERR(ct))
		return -EINVAL;

	sbrc_skip_done = cc->cmdq_events[MTK_MM_CORE_SBRC_SKIP_DONE];

	cmdq_pkt_create(&cmdq_pkt);
	cmdq_pkt_assign_command(cmdq_pkt, GCE_SPR3, 26 * INSTRUCT_SIZE);
	cmdq_pkt_assign_command(cmdq_pkt, GCE_CPR5, skip_next_pa);

	/* reset jump, make sure jump offset correct */
#if 0
	cmdq_pkt_write_value(cmdq_pkt, 4, 0x8100, 0, 0xffffffff);
#endif
	cmdq_pkt_assign_command(cmdq_pkt, GCE_CPR6, forced_buf_id_pa);
	cmdq_pkt_assign_command(cmdq_pkt, GCE_SPR1, cc->safe_line);
	cmdq_pkt_clear_event(cmdq_pkt, sbrc_skip_done);
	cmdq_pkt_wfe(cmdq_pkt, sbrc_skip_done);

	mutex = mtk_dispsys_get_mutex_res(cc->disp_dev, MUTEX_DISP);

	cmdq_pkt_read(cmdq_pkt, SUBSYS_1402XXXX, 0x0000 | 0x198, GCE_SPR2);

	cmdq_pkt_logic_command(cmdq_pkt, CMDQ_LOGIC_AND,
				GCE_SPR2,
				cmdq_operand_reg(&left_operand, GCE_SPR2),
				cmdq_operand_immediate(&right_operand, 0x7fff));

	cmdq_pkt_store_reg(cmdq_pkt, GCE_CPR5, GCE_SPR2, 0x7fff);
#if 0
	pr_err("100 buf_size = %d\n", cmdq_get_cmd_buf_size(cmdq_pkt));
#endif
	cmdq_pkt_cond_jump(cmdq_pkt, GCE_SPR3,
			   cmdq_operand_reg(&left_operand, GCE_SPR2),
			   cmdq_operand_reg(&right_operand, GCE_SPR1),
			   CMDQ_GREATER_THAN);

	/* 2 instructions */
	mtk_mutex_error_clear(mutex, &cmdq_pkt);

	#ifdef SBRC_FORCE_READ_CTRL
	cmdq_pkt_read(cmdq_pkt, SUBSYS_1402XXXX, 0xc000 | 0x034, GCE_SPR2);

	cmdq_pkt_logic_command(cmdq_pkt, CMDQ_LOGIC_AND,
				GCE_SPR2,
				cmdq_operand_reg(&left_operand, GCE_SPR2),
				cmdq_operand_immediate(&right_operand, 0x3ff));
	/* 2 instructions, because using mask */
	cmdq_pkt_store_reg(cmdq_pkt, GCE_CPR6, GCE_SPR2, 0x3ff);
	/* 4 instructions */
	mtk_sbrc_sw_control_read(cc->dev_sbrc, cmdq_pkt);
	#endif

	/* 4 instructions */
	mtk_sbrc_clr_bypass(cc->dev_sbrc, cmdq_pkt);
	/* 5 * 2 instructions */
	mtk_mutex_enable(mutex, &cmdq_pkt);
#if 0
	cmdq_pkt_write_value(cmdq_pkt, 4, 0x8100, 1, 0xffffffff);
#endif
	/* dummy cmd protect jump */
	cmdq_pkt_assign_command(cmdq_pkt, GCE_SPR0, 0);
	cmdq_pkt_assign_command(cmdq_pkt, GCE_SPR1, 0);
	cmdq_pkt_assign_command(cmdq_pkt, GCE_SPR2, 0);
	cmdq_pkt_assign_command(cmdq_pkt, GCE_SPR3, 0);
	cmdq_pkt_assign_command(cmdq_pkt, GCE_SPR3, 0);

	init_completion(&cc->sbrc_err);

	while (cc->sbrc_monitor) {
		cmdq_pkt_flush_async(cc->cmdq_client[SBRC_SKIP_DONE_CH],
				cmdq_pkt, mtk_cinematic_sbrc_err_handle, cc);

		rc = wait_for_completion_interruptible(&cc->sbrc_err);
		if (rc < 0)
			goto EXIT_MONITOR;

		reinit_completion(&cc->sbrc_err);

		if (cc->cmd != DEINIT_PATH)
			mtk_cinematic_cb_event(ct, CB_GPU_SKIP_DONE, NULL, 0);
	}

EXIT_MONITOR:
	cmdq_pkt_destroy(cmdq_pkt);
	dev_dbg(cc->dev, "=== sbrc error recover thread terminated ===\n");
	return 0;
}

static int enable_strip_monitor(struct mtk_cinematic_ctrl *cc)
{
	struct mtk_cinematic_test *ct;
	u32 td_id;
	int jump_offset;
	int ring_jump_offset;
	int inst;
	u32 jump_h;

	ct = get_active_pipe(cc);

	if (IS_ERR(ct))
		return -EINVAL;

	inst = cc->act_inst;

	if (!ct->para.gp.strip_h) {
		dev_dbg(cc->dev, "strip_h = 0, monitor enable NG!!\n");
		return -EINVAL;
	}

	jump_h = (u16)(ct->para.gp.out_h >> 4) * 15;
	dev_dbg(cc->dev, "==== jump_h = %d\n", jump_h);

	left_operand = kzalloc(sizeof(*left_operand), GFP_KERNEL);
	if (!left_operand)
		goto L_MALLOC_FAIL;

	right_operand = kzalloc(sizeof(*right_operand), GFP_KERNEL);
	if (!right_operand)
		goto R_MALLOC_FAIL;

	/* Timer debugger */
	td_id = cc->cmdq_events[MTK_MM_CORE_TD_EVENT0];
	cmdq_pkt_create(&cc->cmdq_pkt[inst]);

	/* min */
	cmdq_pkt_assign_command(cc->cmdq_pkt[inst], GCE_SPR0, 4000);
	/* max */
	cmdq_pkt_assign_command(cc->cmdq_pkt[inst], GCE_SPR3, 0);
	/* reset record */
	cmdq_pkt_assign_command(cc->cmdq_pkt[inst], GCE_GPRR4, rec_pa_d);
	cmdq_pkt_assign_command(cc->cmdq_pkt[inst], GCE_GPRR8, rec_pa_w);
	cmdq_pkt_assign_command(cc->cmdq_pkt[inst], GCE_GPRR9, rec_pa_r);
	cmdq_pkt_assign_command(cc->cmdq_pkt[inst], GCE_CPR4, REC_NUM - 1);

	/* reset accumulation */
	cmdq_pkt_assign_command(cc->cmdq_pkt[inst], GCE_GPRR2, 0);
	/* reset accumulation times */
	cmdq_pkt_assign_command(cc->cmdq_pkt[inst], GCE_GPRR3, 0);
	/* cond of not 0 jump */

#ifdef REDUCE_W_OP_CNT
	cmdq_pkt_assign_command(cc->cmdq_pkt[inst],
				GCE_CPR1, 22 * INSTRUCT_SIZE);
#else
	cmdq_pkt_assign_command(cc->cmdq_pkt[inst],
				GCE_CPR1, 25 * INSTRUCT_SIZE);
#endif
	/* cond of frame complete jump */
	cmdq_pkt_assign_command(cc->cmdq_pkt[inst],
				GCE_CPR2, 5 * INSTRUCT_SIZE);
	/* cond of min jump */
	cmdq_pkt_assign_command(cc->cmdq_pkt[inst],
				GCE_CPR3, 4 * INSTRUCT_SIZE);

	/* while (1)*/
	/* loop body */
	/* using mutex delay to poll */
	cmdq_pkt_clear_event(cc->cmdq_pkt[inst], td_id);
	cmdq_pkt_wfe(cc->cmdq_pkt[inst], td_id);

	/* get RBFC W / R count diff */
	cmdq_pkt_read(cc->cmdq_pkt[inst], cc->rbfc_subsys, 0x8000 | 0xb8,
		      GCE_SPR2);

	/* get line count by AND Logic */
	cmdq_pkt_logic_command(cc->cmdq_pkt[inst], CMDQ_LOGIC_AND,
				GCE_SPR2,
				cmdq_operand_reg(left_operand, GCE_SPR2),
				cmdq_operand_immediate(right_operand, 0x7fff));

	cmdq_pkt_assign_command(cc->cmdq_pkt[inst], GCE_SPR1,
				cc->sd.d_pa[CUR_DIFF]);
	cmdq_pkt_store_reg(cc->cmdq_pkt[inst], GCE_SPR1, GCE_SPR2, 0xffffffff);

	/* if rbfc cnt != 0 */
	cmdq_pkt_cond_jump(cc->cmdq_pkt[inst], GCE_CPR1,
			   cmdq_operand_reg(left_operand, GCE_SPR2),
			   cmdq_operand_immediate(right_operand, 0x0),
			   CMDQ_EQUAL);

	/* get RBFC W OP_CNT */
	cmdq_pkt_read(cc->cmdq_pkt[inst], cc->rbfc_subsys, 0x8000 | 0xb0,
		      GCE_SPR3);

	cmdq_pkt_logic_command(cc->cmdq_pkt[inst], CMDQ_LOGIC_AND,
				GCE_SPR3,
				cmdq_operand_reg(left_operand, GCE_SPR3),
				cmdq_operand_immediate(right_operand, 0x3fff));
	/* accumulate */
	cmdq_pkt_logic_command(cc->cmdq_pkt[inst], CMDQ_LOGIC_ADD,
			       GCE_GPRR2,
			       cmdq_operand_reg(left_operand, GCE_GPRR2),
			       cmdq_operand_reg(right_operand, GCE_SPR2));

	cmdq_pkt_assign_command(cc->cmdq_pkt[inst], GCE_SPR1,
				cc->sd.d_pa[SUM_DIFF]);
	cmdq_pkt_store_reg(cc->cmdq_pkt[inst], GCE_SPR1, GCE_GPRR2, 0xffffffff);

	cmdq_pkt_logic_command(cc->cmdq_pkt[inst], CMDQ_LOGIC_ADD,
			       GCE_GPRR3,
			       cmdq_operand_reg(left_operand, GCE_GPRR3),
			       cmdq_operand_immediate(right_operand, 1));

	cmdq_pkt_assign_command(cc->cmdq_pkt[inst], GCE_SPR1,
				cc->sd.d_pa[F_COUNT]);
	cmdq_pkt_store_reg(cc->cmdq_pkt[inst], GCE_SPR1, GCE_GPRR3, 0xffffffff);

	/* record[i] = current R/W diff */
	cmdq_pkt_store_reg(cc->cmdq_pkt[inst], GCE_GPRR4, GCE_SPR2, 0xffffffff);
	cmdq_pkt_logic_command(cc->cmdq_pkt[inst], CMDQ_LOGIC_ADD,
				GCE_GPRR4,
				cmdq_operand_reg(left_operand, GCE_GPRR4),
				cmdq_operand_immediate(right_operand, 4));
	/* W_OP_CNT  store */
#ifdef REDUCE_W_OP_CNT
	/* DO NOT store w_opt */
#else
	cmdq_pkt_read(cc->cmdq_pkt[inst], cc->rbfc_subsys, 0x8000 | 0xb0,
		      GCE_GPRR10);
	cmdq_pkt_store_reg(cc->cmdq_pkt[inst], GCE_GPRR8, GCE_GPRR10, 0x7fff);
	cmdq_pkt_logic_command(cc->cmdq_pkt[inst], CMDQ_LOGIC_ADD,
				GCE_GPRR8,
				cmdq_operand_reg(left_operand, GCE_GPRR8),
				cmdq_operand_immediate(right_operand, 4));
#endif
	/* R_OP_CNT store */
	cmdq_pkt_read(cc->cmdq_pkt[inst], cc->rbfc_subsys, 0x8000 | 0xb4,
		      GCE_GPRR10);
	cmdq_pkt_store_reg(cc->cmdq_pkt[inst], GCE_GPRR9, GCE_GPRR10, 0x7fff);
	cmdq_pkt_logic_command(cc->cmdq_pkt[inst], CMDQ_LOGIC_ADD,
				GCE_GPRR9,
				cmdq_operand_reg(left_operand, GCE_GPRR9),
				cmdq_operand_immediate(right_operand, 4));

	/* if rbfc already received total strips, skip min count */
	cmdq_pkt_cond_jump(cc->cmdq_pkt[inst], GCE_CPR2,
			   cmdq_operand_reg(left_operand, GCE_SPR3),
			   cmdq_operand_immediate(right_operand, jump_h),
			   CMDQ_GREATER_THAN);

	/* if current rbfc_cnt < min */
	cmdq_pkt_cond_jump(cc->cmdq_pkt[inst], GCE_CPR3,
			   cmdq_operand_reg(left_operand, GCE_SPR0),
			   cmdq_operand_reg(right_operand, GCE_SPR2),
			   CMDQ_LESS_THAN);

	/* min record PA */
	cmdq_pkt_assign_command(cc->cmdq_pkt[inst], GCE_SPR1,
				cc->sd.d_pa[MIN_DIFF]);
	cmdq_pkt_store_reg(cc->cmdq_pkt[inst], GCE_SPR1, GCE_SPR2, 0xffffffff);
	cmdq_pkt_load(cc->cmdq_pkt[inst], GCE_SPR0, GCE_SPR1);

	/* end of loop body */

	/* dummy for jump safe */
	cmdq_pkt_assign_command(cc->cmdq_pkt[inst], GCE_SPR1,
				cc->sd.d_pa[F_COUNT]);
	cmdq_pkt_assign_command(cc->cmdq_pkt[inst], GCE_SPR1,
				cc->sd.d_pa[F_COUNT]);
	cmdq_pkt_assign_command(cc->cmdq_pkt[inst], GCE_SPR1,
				cc->sd.d_pa[F_COUNT]);
	cmdq_pkt_assign_command(cc->cmdq_pkt[inst], GCE_SPR1,
				cc->sd.d_pa[F_COUNT]);
	cmdq_pkt_assign_command(cc->cmdq_pkt[inst], GCE_SPR2,
				2 * INSTRUCT_SIZE);
	cmdq_pkt_load(cc->cmdq_pkt[inst], GCE_SPR1, GCE_SPR1);

	/* process ring buffer */
	cmdq_pkt_cond_jump(cc->cmdq_pkt[inst], GCE_SPR2,
			   cmdq_operand_reg(left_operand, GCE_SPR1),
			   cmdq_operand_reg(right_operand, GCE_CPR4),
			   CMDQ_LESS_THAN);

	ring_jump_offset = (2 * INSTRUCT_SIZE) -
			    cmdq_get_cmd_buf_size(cc->cmdq_pkt[inst]);

	cmdq_pkt_jump(cc->cmdq_pkt[inst], ring_jump_offset);

	jump_offset = (10 * INSTRUCT_SIZE) -
		       cmdq_get_cmd_buf_size(cc->cmdq_pkt[inst]);

	cmdq_pkt_jump(cc->cmdq_pkt[inst], jump_offset);
	/* loop back */

	cmdq_pkt_flush_async(cc->cmdq_client[GPU_CH], cc->cmdq_pkt[inst],
			     disable_sbrc_monitor, cc);

	kfree(right_operand);
R_MALLOC_FAIL:
	kfree(left_operand);
L_MALLOC_FAIL:

	dev_dbg(cc->dev, "===  trigger monitor sbrc GCE Thread ===\n");
	return 0;
}

static int cc_sof_task(void *data)
{
	struct mtk_cinematic_ctrl *cc;
	struct mtk_cinematic_test *ct;
	struct cmdq_pkt *cmdq_pkt;
	u32 sof_id;

	cc = (struct mtk_cinematic_ctrl *)data;
	ct = get_active_pipe(cc);

	if (IS_ERR(ct))
		return -EINVAL;

	cc->wdma_sof_cnt = 0;
	/* wdma0 = 23, DSI sof = 150 */
	sof_id = get_wdma_sof_id(cc);

	cmdq_pkt_create(&cmdq_pkt);
	cmdq_pkt_clear_event(cmdq_pkt, sof_id);
	cmdq_pkt_wfe(cmdq_pkt, sof_id);

	while (!kthread_should_stop()) {

		if (cc->cmd == PAUSE_PATH) {
			msleep(1000);
			mtk_reset_output_port_buf(ct);
			continue;
		}

		if (cmdq_pkt_flush(cc->cmdq_client[WDMA_CH], cmdq_pkt) != 0)
			dev_dbg(cc->dev, "=== sof timeout! ===\n");
#if 0
		if (cc->wdma_frame_cnt > 0) {
			mtk_cinematic_get_disp_fb(ct, cc->disp_fb);
			mtk_cinematic_addr_update(ct);
		}
#endif
		if (cc->cmd != DEINIT_PATH)
			cc->wdma_sof_cnt++;
		mtk_cinematic_get_disp_fb(ct, cc->disp_fb);
		mtk_cinematic_addr_update(cc->ct);
	}

	cmdq_pkt_destroy(cmdq_pkt);
	dev_dbg(cc->dev, "=== wdma buf update thread dies ===\n");
	return 0;
}

static void reset_dump_frame_done(struct cmdq_cb_data data)
{
	struct mtk_cinematic_ctrl *cc = data.data;
	struct mtk_cinematic_test *ct;

	ct = get_active_pipe(cc);

	if (IS_ERR(ct)) {
		pr_err("%s, ct handle null\n", __func__);
		return;
	}

	dev_dbg(cc->dev, " == reset dump frame done == !!!\n");

	cmdq_pkt_destroy(cc->cmdq_pkt[UNMUTE_CH]);
}

int mtk_dump_rdma_buf(struct mtk_cinematic_ctrl *cc, int buf_idx)
{
	struct device *dma_dev;
	struct mtk_cinematic_test *ct;
	struct mtk_common_buf *ion_buf;
	char k_buf[128];
	int size = 0, bpp = 0, pitch = 0, pvric_hdr_offset = 0;
	int alignment = 0;
	int out_w, out_h;

	ct = get_active_pipe(cc);

	if (IS_ERR(ct)) {
		pr_err("%s, ct handle null\n", __func__);
		return -EIO;
	}

	if (!ct->para.gp.bypass) {

		out_w = ct->para.gp.out_w;
		out_h = ct->para.gp.out_h;

		dma_dev = mtk_dispsys_get_rdma(cc->disp_dev);
		ion_buf = cc->rdma_ion_buf[buf_idx];
		bpp = mtk_format_to_bpp(ct->para.gp.out_fmt);

		if (ct->para.pvric_en & COMPRESS_OUTPUT_ENABLE) {

			int image_offset;

			memset(k_buf, 0, sizeof(k_buf));
			snprintf(k_buf, sizeof(k_buf), "/tmp/rdma_%d_0.bin",
				 buf_idx);

			/* pvric force 32 bits */
			pitch = ((out_w * 32) >> 3) >> 1;
			/* one img size */
			size = pitch * out_h;
			/* one image hdr */
			pvric_hdr_offset = ((out_w * out_h + 127) >> 7) >> 1;

			/* for half image */
			alignment = pvric_hdr_offset;

			pr_info("half header data section size = %d(0x%x)\n",
				pvric_hdr_offset >> 1, pvric_hdr_offset >> 1);

			/* for hw real hdr size is align 256 */
			pvric_hdr_offset = ALIGN(pvric_hdr_offset, 256);

			/* for tool dump, remove alignment */
			alignment = pvric_hdr_offset - alignment;
			pr_info("alignment offset = %d (0x%x)\n", alignment,
				alignment);

			/* calculate right image offset */
			image_offset = size + pvric_hdr_offset;
			/* alignment 8k */
			image_offset = (image_offset + 0x2000 - 1) >> 13 << 13;

			/* acutal dump size */
			size += pvric_hdr_offset - alignment;

			/* dump left */
			pa_to_vaddr_dev(dma_dev, ion_buf, size, true);
			mtk_common_write_file(ion_buf->kvaddr + alignment,
					      k_buf, size);
			pa_to_vaddr_dev(dma_dev, ion_buf, size, false);

			/* reset naming */
			memset(k_buf, 0, sizeof(k_buf));
			snprintf(k_buf, sizeof(k_buf), "/tmp/rdma_%d_1.bin",
				 buf_idx);

			/* dump right */
			pa_to_vaddr_dev(dma_dev, ion_buf, size, true);
			mtk_common_write_file(ion_buf->kvaddr + image_offset +
					      alignment, k_buf, size);
			pa_to_vaddr_dev(dma_dev, ion_buf, size, false);

		} else {
			memset(k_buf, 0, sizeof(k_buf));
			snprintf(k_buf, sizeof(k_buf), "/tmp/rdma_%d.bin",
				 buf_idx);

			pitch = ((out_w * bpp) >> 3);
			size = pitch * out_h;

			/* dump left & right */
			pa_to_vaddr_dev(dma_dev, ion_buf, size, true);
			mtk_common_write_file(ion_buf->kvaddr, k_buf, size);
			pa_to_vaddr_dev(dma_dev, ion_buf, size, false);

			pr_info("dump idx:%d, size:%d @%p\n", buf_idx, size,
				(void *)ion_buf->kvaddr);
		}

	} else {
		return -EFAULT;
	}
	return 0;
}

int dump_next_frame(struct mtk_cinematic_ctrl *cc)
{
	struct mtk_cinematic_test *ct;
	u32 eof_id;
	u32 mmsys_subsys;

	ct = get_active_pipe(cc);

	if (IS_ERR(ct))
		return -EINVAL;

	/* dsi0 eof */
	eof_id = cc->cmdq_events[MTK_MM_CORE_DSI0_FRAME_DONE];
	mmsys_subsys = 3;

	cmdq_pkt_create(&cc->cmdq_pkt[UNMUTE_CH]);
	cmdq_pkt_clear_event(cc->cmdq_pkt[UNMUTE_CH], eof_id);
	cmdq_pkt_wfe(cc->cmdq_pkt[UNMUTE_CH], eof_id);

	/* for PAPA sram first dump */
	cmdq_pkt_write_value(cc->cmdq_pkt[UNMUTE_CH], mmsys_subsys, 0xfe0,
			     0x02f76000, 0xffffffff);
	/* reset dump counter */
	cmdq_pkt_write_value(cc->cmdq_pkt[UNMUTE_CH], mmsys_subsys, 0xfe0,
			     0x0af76000, 0xffffffff);
	/* start capture next frame */
	cmdq_pkt_write_value(cc->cmdq_pkt[UNMUTE_CH], mmsys_subsys, 0xfe0,
			     0x06f76000, 0xffffffff);

	/* async to avoid blocking console shell */
	cmdq_pkt_flush_async(cc->cmdq_client[UNMUTE_CH],
			     cc->cmdq_pkt[UNMUTE_CH],
			     reset_dump_frame_done, cc);
	return 0;
}
EXPORT_SYMBOL(dump_next_frame);

static int dsi_unmute(struct mtk_cinematic_ctrl *cc)
{
	struct mtk_cinematic_test *ct;
	struct cmdq_pkt *cmdq_pkt;
	struct mtk_mutex_res *d_mutex;
	struct mtk_mutex_res *s_mutex;
	struct mtk_mutex_res *td_mutex;
	u32 eof_id;

	ct = get_active_pipe(cc);

	if (IS_ERR(ct))
		return -EINVAL;

	/* dsi0 eof */
	eof_id = cc->cmdq_events[MTK_MM_CORE_DSI0_FRAME_DONE];

	s_mutex = mtk_dispsys_get_mutex_res(cc->disp_dev, MUTEX_SBRC);
	d_mutex = mtk_dispsys_get_mutex_res(cc->disp_dev, MUTEX_DISP);
	td_mutex = ct->mutex;

	cmdq_pkt_create(&cmdq_pkt);
	cmdq_pkt_clear_event(cmdq_pkt, eof_id);
	cmdq_pkt_wfe(cmdq_pkt, eof_id);

	mtk_mutex_enable(d_mutex, &cmdq_pkt);
	if (ct->para.gp.strip_mode)
		mtk_mutex_enable(s_mutex, &cmdq_pkt);
	mtk_dispsys_mute(cc->disp_dev, 0, cmdq_pkt);

	if (ct->para.gp.strip_mode && cc->sampling_rate) {

		mtk_mutex_select_timer_sof(td_mutex, MUTEX_MMSYS_SOF_SINGLE,
					   MUTEX_MMSYS_SOF_SINGLE, &cmdq_pkt);

		mtk_mutex_set_timer_us(td_mutex, cc->sampling_rate, 200,
				       &cmdq_pkt);

		mtk_mutex_timer_enable_ex(td_mutex, true,
					  MUTEX_TO_EVENT_SRC_OVERFLOW,
					  &cmdq_pkt);
		dev_dbg(cc->dev, "===  enable monitor sbrc ===\n");
	}

	if (cmdq_pkt_flush(cc->cmdq_client[UNMUTE_CH], cmdq_pkt) == 0)
		dev_dbg(cc->dev, "=== EOF / MTX EN !! ===\n");

	cmdq_pkt_destroy(cmdq_pkt);
	return 0;
}

#ifdef SLICER_DBG
static int slicer_interrupt_en(struct mtk_cinematic_ctrl *cc)
{
	struct mtk_cinematic_test *ct;
	struct cmdq_pkt *cmdq_pkt;

	ct = get_active_pipe(cc);

	if (IS_ERR(ct))
		return -EINVAL;

	cmdq_pkt_create(&cmdq_pkt);

	/* mmsys crc en */
	cmdq_pkt_write_value(cmdq_pkt, 3, 0x0214, 0xffffffff, 0xffffffff);
	/* slicer int en */
	cmdq_pkt_write_value(cmdq_pkt, 3, 0x21a0, 0x1ff, 0xffffffff);
	/* crop bypass shadow */
	cmdq_pkt_write_value(cmdq_pkt, 4, 0x4034, 0x1, 0xffffffff);
	/* wdma crc en */
	cmdq_pkt_write_value(cmdq_pkt, 4, 0x8008, 0x80010505, 0xffffffff);
#if 0
	/* disable dcm */
	cmdq_pkt_write_value(cmdq_pkt, 3, 0x0010, 0x100, 0xffffffff);
	cmdq_pkt_write_value(cmdq_pkt, 3, 0x0014, 0xffffffff, 0xffffffff);
	/* disable dcm */
	cmdq_pkt_write_value(cmdq_pkt, 3, 0x0018, 0x100, 0xffffffff);
	cmdq_pkt_write_value(cmdq_pkt, 3, 0x001c, 0xffffffff, 0xffffffff);
	cmdq_pkt_write_value(cmdq_pkt, 4, 0x8038, 0xD00001F0, 0xffffffff);
#endif
	if (cmdq_pkt_flush(cc->cmdq_client[UNMUTE_CH], cmdq_pkt) == 0)
		dev_err(cc->dev, "=== EN Slicer Interrupt !! ===\n");

	cmdq_pkt_destroy(cmdq_pkt);

	return 0;
}
#endif

static int get_vsync_delay(struct mtk_cinematic_ctrl *cc, u32 *delay)
{
	struct mtk_cinematic_test *ct;

	if (IS_ERR(cc))
		return -EINVAL;

	ct = get_active_pipe(cc);

	if (IS_ERR(ct) || IS_ERR(delay))
		return -EINVAL;

	/* early vsync time */
	switch (ct->para.disp_fps) {
	case FPS_120:
		/* fall through */
	case FPS_119:
		*delay = 8000 - SOF_GAP;
		break;
	case FPS_90:
		/* fall through */
	case FPS_89:
		*delay = 10800 - SOF_GAP;
		break;
	case FPS_60:
		/* fall through */
	case FPS_59:
		*delay = 16300 - SOF_GAP;
		break;
	default:
		*delay = 8000 - SOF_GAP;
		dev_err(cc->dev, "=== Disp FPS wrong !!! ===\n");
	}
	dev_dbg(cc->dev, "delay = %d", *delay);
	return 0;
}

static int vsync_task(void *data)
{
	struct mtk_cinematic_ctrl *cc;
	struct mtk_cinematic_test *ct;
	struct cmdq_pkt *cmdq_pkt;
	unsigned long flags;
	u32 td_vsync_id, vsync_delay;

	cc = (struct mtk_cinematic_ctrl *)data;
	ct = get_active_pipe(cc);

	if (IS_ERR(ct))
		return -EINVAL;

	td_vsync_id = cc->cmdq_events[MTK_MM_CORE_TD_EVENT5];

	mtk_mutex_select_timer_sof(cc->vsync_mutex, MUTEX_MMSYS_SOF_DSI0,
				   MUTEX_MMSYS_SOF_SINGLE, NULL);

	if (get_vsync_delay(cc, &vsync_delay)) {
		dev_dbg(cc->dev, "Get VSYNC delay NG!!\n");
		return -EINVAL;
	}

	mtk_mutex_set_timer_us(cc->vsync_mutex, vsync_delay, vsync_delay, NULL);

	/* enable timer to force delayed vsync event generated */
	mtk_mutex_timer_enable_ex(cc->vsync_mutex, true,
				  MUTEX_TO_EVENT_REF_OVERFLOW, NULL);

	cmdq_pkt_create(&cmdq_pkt);
	cmdq_pkt_clear_event(cmdq_pkt, td_vsync_id);
	cmdq_pkt_wfe(cmdq_pkt, td_vsync_id);
	cmdq_pkt_set_token(cmdq_pkt, GCE_SW_TOKEN_GPU_VSYNC, SUBSYS_1028XXXX);

	while (!kthread_should_stop()) {

		if (cmdq_pkt_flush(cc->cmdq_client[VSYNC_CH], cmdq_pkt) != 0) {
			dev_dbg(cc->dev, "=== VSYNC TD timeout! ===\n");
			continue;
		}

		/* ensure GPU can get frame */
		if (cc->wdma_frame_cnt > 0) {
			LOG_VSYNC("Delayed VSYNC\n");

			spin_lock_irqsave(&cc->spinlock_irq, flags);
			cc->vsync = 1;
			spin_unlock_irqrestore(&cc->spinlock_irq, flags);

			wake_up_interruptible(&cc->vsync_wait);
		}
	}
	cmdq_pkt_destroy(cmdq_pkt);
	mtk_mutex_timer_disable(cc->vsync_mutex, NULL);

	return 0;
}

static int cc_task(void *data)
{
	struct mtk_cinematic_ctrl *cc;
	struct mtk_cinematic_test *ct;
	struct cmdq_pkt *cmdq_pkt;
	struct mtk_mutex_res *mutex;
	struct cc_job rsz_job = {0};
	struct rsz_para rp;
	u32 pre_b_cnt = 0, total_b_cnt = 0, s_comp_ratio = 0;
	u32 wdma_done, cur_comp_ratio = 0;
	int inst;

	cc = (struct mtk_cinematic_ctrl *)data;
	ct = get_active_pipe(cc);
	inst = cc->act_inst;

	if (IS_ERR(ct))
		return -EINVAL;

	if (ct->target_line == 0)
		wdma_done = get_wdma_frame_done_id(cc);
	else
		wdma_done = get_wdma_target_line_id(cc);

	cc->wdma_frame_cnt = 0;
	cmdq_pkt_create(&cmdq_pkt);
	cmdq_pkt_clear_event(cmdq_pkt, wdma_done);
	cmdq_pkt_wfe(cmdq_pkt, wdma_done);

	while (!kthread_should_stop()) {

		if (ct->para.sof_sel == SINGLE_SOF && cc->wdma_frame_cnt) {
			msleep(3000);
			continue;
		}

		if (cc->cmd == PAUSE_PATH) {
			/* sleep here prevent from wait sof timeout */
			msleep(300);
			mtk_cinematic_pause(ct);
			msleep(300);
			continue;
		} else if (cc->cmd == RESUME_PATH) {
			mtk_cinematic_resume(ct);
		} else if (cc->cmd == DEINIT_PATH) {
			msleep(300);
			continue;
		}

		if (cmdq_pkt_flush(cc->cmdq_client[inst], cmdq_pkt) != 0) {
			dev_dbg(cc->dev, "=== wdma timeout! ===\n");
			continue;
		}

		/* restore status and inform gpu ready to go */
		if (cc->cmd == RESUME_PATH) {
			cc->cmd = DISP_PATH;
			mtk_cinematic_cb_event(ct, CB_DISP_DISPLAY, NULL, 0);
		}

		rp = ct->para.rp;
		total_b_cnt = rp.out_w * rp.out_h * ct->output_port.bpp >> 3;

		LOG_IN("output dp frames %lld\n", cc->wdma_frame_cnt);

		if (ct->seq_chg) {
			cc->frame_done = SEQ_CHG_FRAME_DONE;
			mtk_cinematic_cb_event(ct, CB_SEQ_CHG, NULL, 0);
		} else {
			cc->frame_done = FRAME_DONE;
		}

		/*mtk_cinematic_get_disp_fb(ct, cc->disp_fb);*/
		/*mtk_cinematic_swap_buf(ct);*/

		if (!cc->wdma_frame_cnt)
			dev_dbg(cc->dev, "Get first wdma frame !!\n");

		if (cc->cmd != DEINIT_PATH) {
			cc->wdma_frame_cnt++;
			/* notify first frame */
			if (cc->wdma_frame_cnt == 1)
				mtk_cinematic_cb_event(ct, CB_WDMA_FIRST_FRAME,
						       NULL, 0);
		}

		/* measure average compression ratio */
		if (ct->para.pvric_en && cc->act_inst == 0) {
			mtk_wdma_get_pvric_frame_comp_pre(ct->dev_wdma,
							  &pre_b_cnt);
			cur_comp_ratio = pre_b_cnt * 100 / total_b_cnt;
			s_comp_ratio += cur_comp_ratio;
			cc->avg_wdma_comp_ratio = s_comp_ratio /
						  cc->wdma_frame_cnt;
			if (cur_comp_ratio > cc->max_wdma_comp_ratio)
				cc->max_wdma_comp_ratio = cur_comp_ratio;
		}

		/* update rdma src addr for gpu bypass */
		if (ct->para.gp.bypass) {
			mtk_dispsys_config_srcaddr(cc->disp_dev,
						   cc->disp_fb->pa, NULL);
			/*
			 * only for byp gpu we enable disp in kernel
			 * otherwise, display was enabled in userspace
			 */
			if (!cc->displayed) {
				cc->displayed = true;
				dev_dbg(cc->dev, "### unmute dsi for byp gpu case ###\n");
				mutex = mtk_dispsys_get_mutex_res(cc->disp_dev,
						MUTEX_DISP);
				mtk_mutex_enable(mutex, NULL);
				mtk_dispsys_mute(cc->disp_dev, 0, NULL);
			}
		}

		/* special mode for dynamic pat gen color change */
		if (ct->para.pat_sel == 3 && !ct->para.input_from_dp) {
			mtk_dynamic_pat_chg(ct);
			continue;
		}

		mutex_lock(&cc->queue_lock);
		if (receive_job(mtk_jobs_q, &rsz_job) == 0) {
			u32 eof_id = 0;

			dequeue(mtk_jobs_q);
			eof_id = cc->cmdq_events[MTK_MM_CORE_CROP0_EOF];
			mtk_dynamic_seq_chg(ct, rsz_job, eof_id);
		}
		mutex_unlock(&cc->queue_lock);
	}

	cmdq_pkt_destroy(cmdq_pkt);

	return 0;
}

static int Is4Kresolution(int width, int height)
{
	int ret = 0;

	if (((width == 3840) && (height == 2160)) ||
	   ((width == 4000) && (height == 2040))) {
		ret = 1;
	}

	return ret;
}

#ifdef CONFIG_MACH_MT3612_A0
static int mtk_check_dp_video(struct mtk_cinematic_test *ct,
			      struct dprx_video_info_s *video)
{
	if (IS_ERR(ct))
		return -EINVAL;

	if (IS_ERR(video))
		return -EINVAL;

	/* check resolution */
	if (ct->para.input_width != video->vid_timing_dpout.h_active) {
		dev_err(ct->dev, "input_w = %d, dp.h_active = %d\n",
			ct->para.input_width, video->vid_timing_dpout.h_active);
		dev_err(ct->dev, "dp output width mismatched with cfg\n");
		return -EINVAL;
	}

	if (ct->para.input_height != video->vid_timing_dpout.v_active) {
		dev_err(ct->dev, "input_h = %d, dp.v_active = %d\n",
			ct->para.input_height,
			video->vid_timing_dpout.v_active);
		dev_err(ct->dev, "dp output height mismatched with cfg\n");
		return -EINVAL;
	}

	if (ct->is_3d_tag) {
		if (!((video->vid_timing_dpout.h_active  == 1920 &&
				video->vid_timing_dpout.v_active == 2205) ||
			  (video->vid_timing_dpout.h_active  == 1280 &&
				video->vid_timing_dpout.v_active == 1470))) {
			dev_err(ct->dev, "3d flag is on, but resultion is not supported!!");
			return -EINVAL;
		}
	}

	/* check input frame rate */
	if (ct->para.input_fps != video->frame_rate) {
		dev_err(ct->dev, "input_fps = %d, dp.frame_rate = %d\n",
			ct->para.input_fps, video->frame_rate);
		dev_err(ct->dev, "dp output frame rate mismatched with cfg\n");
		return -EINVAL;
	}

	/* bypass format & bit-depth check for gpu performance evaluation */
	if (ct->para.check_byp || ct->para.wdma_csc_en)
		return 0;

	/* check color format */
	switch (video->vid_color_fmt) {
	case RGB444:
		dev_err(ct->dev, "gp.in_fmt = %d, dp.vid_color_fmt = %s",
			ct->para.gp.in_fmt, "RGB444");
		if (ct->para.gp.in_fmt == DRM_FORMAT_AYUV2101010) {
			dev_err(ct->dev, "!expected YUV but RGB!");
			return -EINVAL;
		}
		break;
	case YUV444:
		dev_err(ct->dev, "gp.in_fmt = %d, dp.vid_color_fmt = %s",
			ct->para.gp.in_fmt, "YUV444");
		if (ct->para.gp.in_fmt != DRM_FORMAT_AYUV2101010) {
			dev_err(ct->dev, "!expected RGB but YUV!");
			return -EINVAL;
		}
		break;
	case YUV422:
		dev_err(ct->dev, "Not support YUV422!");
		return -EINVAL;
	case YUV420:
		dev_err(ct->dev, "Not support YUV420!");
		return -EINVAL;
	case YONLY:
		dev_err(ct->dev, "!Not support Y-ONLY!");
		return -EINVAL;
	default:
		dev_err(ct->dev, "!Not support RAW!");
		return -EINVAL;
	}

	/* check bits depth */
	if (ct->para.gp.in_fmt == DRM_FORMAT_ABGR8888) {
		if (video->vid_color_depth != VID8BIT) {
			dev_err(ct->dev, "cfg bit depth 8, dp bits %d(%d)\n",
				video->vid_color_depth == 2 ? 8:10,
				video->vid_color_depth);
			dev_err(ct->dev, "dp bits depth mismatched with cfg\n");
			return -EINVAL;
		}

	} else if (ct->para.gp.in_fmt == DRM_FORMAT_BGR888) {
		if (video->vid_color_depth != VID8BIT) {
			dev_err(ct->dev, "cfg bit depth 8, dp bits %d(%d)\n",
				video->vid_color_depth == 2 ? 8:10,
				video->vid_color_depth);
			dev_err(ct->dev, "dp bits depth mismatched with cfg\n");
			return -EINVAL;
		}
	} else if (ct->para.gp.in_fmt == DRM_FORMAT_RGBA1010102) {
		if (video->vid_color_depth != VID10BIT) {
			dev_err(ct->dev, "cfg bit depth 10, dp bits %d(%d)\n",
				video->vid_color_depth == 2 ? 8:10,
				video->vid_color_depth);
			dev_err(ct->dev, "dp bits depth mismatched with cfg\n");
			return -EINVAL;
		}
	} else if (ct->para.gp.in_fmt == DRM_FORMAT_ABGR2101010) {
		if (video->vid_color_depth != VID10BIT) {
			dev_err(ct->dev, "cfg bit depth 10, dp bits %d(%d)\n",
				video->vid_color_depth == 2 ? 8:10,
				video->vid_color_depth);
			dev_err(ct->dev, "dp bits depth mismatched with cfg\n");
			return -EINVAL;
		}
	} else if (ct->para.gp.in_fmt == DRM_FORMAT_AYUV2101010) {

		if (video->vid_color_depth != VID10BIT) {
			dev_err(ct->dev, "cfg bit depth 10/8, dp bits %d(%d)\n",
				video->vid_color_depth == 2 ? 8:10,
				video->vid_color_depth);
			dev_err(ct->dev, "dp bits depth mismatched with cfg\n");
		}
	}

	return 0;
}

static int mtk_cinematic_dp_cb(struct device *dev, enum DPRX_NOTIFY_T event)
{
	struct mtk_cinematic_ctrl *cc = NULL;
	struct mtk_cinematic_test *ct = NULL;
	struct dprx_video_info_s dp_video_info;
	int stereo_type;

	if (IS_ERR(dev)) {
		dev_err(dev, "dp_cb dev NULL NG!!\n");
		return -EINVAL;
	}

	cc = dev_get_drvdata(dev);

	if (IS_ERR(cc)) {
		dev_err(cc->dev, "dp_cb cc NULL NG!!\n");
		return -EINVAL;
	}

	ct = get_active_pipe(cc);

	if (IS_ERR(ct)) {
		dev_err(cc->dev, "dp_cb ct NULL NG!!\n");
		goto DP_EXIT;
	}

	if (event == DPRX_RX_VIDEO_STABLE) {
		/* get dp timing */
		dprx_get_video_info_msa(&dp_video_info);

		/* check dp input timing match with cfg file config */
		if (mtk_check_dp_video(ct, &dp_video_info)) {
			dev_err(cc->dev,
				"\033[1;31mDPRX cb setting mismatched\033[m\n");
			dev_err(cc->dev,
				"\033[1;31mPlease check dp settings\033[m\n");

			goto DP_EXIT;
		}

		dev_dbg(cc->dev, "!! mtk_cinematic_video_stable cb !!");

		/* set dp info for active pipe use */
		mtk_cinematic_set_dp_video(ct, &dp_video_info);

		/* unlock init flow */
		complete(&cc->dp_stable);
	} else if (event == DPRX_RX_UNPLUG) {
		/* handle unplug here */
		/* using another kernel thread to dispatch cmd */
	} else if (event == DPRX_RX_STEREO_TYPE_CHANGE) {
		/* get stereo type */
		stereo_type = dprx_get_stereo_type();
		if (stereo_type == 0x2) {
			dev_err(cc->dev, "3D type\n");
			ct->is_3d_tag = true;
		} else if (stereo_type == 0x0)
			dev_err(cc->dev, "not 3D type\n");
	}

DP_EXIT:
	return 0;
}
#endif

static void mtk_cinematic_wdma_cb(struct mtk_mmsys_cb_data *data)
{
	struct mtk_cinematic_ctrl *cc =
		(struct mtk_cinematic_ctrl *)data->priv_data;

	dev_err(cc->dev,
		"[wdma_cb] data.part_id: %d, data.status: %d\n",
		data->part_id, data->status);

	if (cc->cmd != DEINIT_PATH)
		cc->wdma_inc_cnt++;
}

static int mtk_swap_rdma_buf(struct mtk_cinematic_ctrl *cc,
			     struct cmdq_pkt **cmdq_pkt)
{
	struct mtk_cinematic_test *ct;
	dma_addr_t rdma_pa = 0;
	u32 offset = 0, eof_id;
	int ret = 0;

	ct = get_active_pipe(cc);

	if (IS_ERR(ct)) {
		pr_err("swap done ng, ct handle null\n");
		return -EIO;
	}

	if (cc->cmd == DEINIT_PATH) {
		pr_debug("deinit, skip swap rdma buf\n");
		return 0;
	}

	eof_id = cc->cmdq_events[MTK_MM_CORE_DSI1_FRAME_DONE];

	if (cc->inserted_rdma_buf <= cc->rdma_buf_id) {
		dev_err(cc->dev, "rdma buffer id err, ion %d <= rdma %d!!\n",
			cc->inserted_rdma_buf, cc->rdma_buf_id);
		return -EIO;
	}

	rdma_pa = cc->rdma_ion_buf[cc->rdma_buf_id]->dma_addr;
	offset = cc->rdma_ion_buf[cc->rdma_buf_id]->offset;

	cmdq_pkt_multiple_reset(cmdq_pkt, DISPLAY_PARTITION_NR);

	cmdq_pkt_clear_event(cmdq_pkt[0], eof_id);
	cmdq_pkt_wfe(cmdq_pkt[0], eof_id);

	if (ct->para.pvric_en & COMPRESS_OUTPUT_ENABLE) {
		ret = mtk_dispsys_config_pvric_2buf_srcaddr(
					cc->disp_dev, rdma_pa,
					rdma_pa + offset, cmdq_pkt);
		if (unlikely(ret)) {
			dev_err(cc->dev, "config pvric 2 buf NG!!!!!\n");
			return -EFAULT;
		}
	} else {
		ret = mtk_dispsys_config_srcaddr(cc->disp_dev, rdma_pa,
						 cmdq_pkt);

		if (unlikely(ret)) {
			dev_err(cc->dev, "config config src addr NG!!\n");
			return -EFAULT;
		}
	}

	if (cmdq_pkt_flush(cc->cmdq_client[RDMA_CH], cmdq_pkt[0]) != 0)
		dev_dbg(cc->dev, "=== dsi1 eof timeout ===\n");

	return 0;
}

static int cc_rdma_swap_task(void *data)
{
	struct mtk_cinematic_ctrl *cc;
	struct mtk_cinematic_test *ct;
	struct cmdq_pkt *cmdq_pkt[DISPLAY_PARTITION_NR];
	struct cmdq_pkt *cmdq_pkt_vsync;
	int i = 0, ret = 0;

	cc = (struct mtk_cinematic_ctrl *)data;
	ct = get_active_pipe(cc);

	if (IS_ERR(ct))
		return -EINVAL;

	cmdq_pkt_create(&cmdq_pkt[0]);

	for (i = 1; i < DISPLAY_PARTITION_NR; i++)
		cmdq_pkt[i] = cmdq_pkt[0];

	cmdq_pkt_create(&cmdq_pkt_vsync);
	cmdq_pkt_clear_token(cmdq_pkt_vsync, GCE_SW_TOKEN_GPU_VSYNC,
			     SUBSYS_1028XXXX);
	cmdq_pkt_wfe(cmdq_pkt_vsync, GCE_SW_TOKEN_GPU_VSYNC);

	while (!kthread_should_stop()) {
		if (cmdq_pkt_flush(cc->cmdq_client[RDMA_CH],
				   cmdq_pkt_vsync) != 0) {
			dev_dbg(cc->dev, "=== VSYNC timeout! ===\n");
			continue;
		}
		ret = mtk_swap_rdma_buf(cc, cmdq_pkt);
		if (unlikely(ret))
			dev_err(cc->dev, "mtk_swap_rdma_buf NG!!\n");
	}

	cmdq_pkt_destroy(cmdq_pkt[0]);
	cmdq_pkt_destroy(cmdq_pkt_vsync);
	dev_dbg(cc->dev, "=== rdma swap buffer thread terminated ===\n");
	return 0;
}

static int mtk_cinematic_init_path(struct mtk_cinematic_ctrl *cc)
{
	int w, h, inst;
	int ret = 0;
	struct mtk_cinematic_test *ct;
#ifdef CONFIG_MACH_MT3612_A0
	int bpp;
	struct timeval start;
	struct timeval done;
	suseconds_t diff;
#endif

	ct = get_active_pipe(cc);
	inst = cc->act_inst;

	if (IS_ERR(ct))
		return -EINVAL;

	w = ct->para.input_width;
	h = ct->para.input_height;
	cc->wdma_frame_cnt = 0;
	cc->wdma_sof_cnt = 0;
	cc->gpu_frame_cnt = 0;
	cc->skip_frame_cnt = 0;
	cc->wdma_inc_cnt = 0;
	cc->avg_wdma_comp_ratio = 0;
	cc->max_wdma_comp_ratio = 0;
	cc->rdma_buf_id = 0;
	cc->dual_mode = false;

#ifdef CONFIG_MACH_MT3612_A0
	if (ct->para.input_from_dp) {

		init_completion(&cc->dp_stable);

		ret = mtk_dispsys_register_cb(cc->disp_dev, cc->dev,
					      mtk_cinematic_dp_cb);
		if (unlikely(ret)) {
			dev_err(cc->dev, "register dp callback NG!!");
			return -EIO;
		}

		ret = mtk_dispsys_dp_start(cc->disp_dev);
		if (unlikely(ret)) {
			dev_err(cc->dev, "cinematic dp start NG!!");
			return -EIO;
		}
		do_gettimeofday(&start);
		mtk_cinematic_cb_event(ct, CB_WAIT_DP, NULL, 0);
	}
#endif
	/* request main gce channel */
	cc->cmdq_client[inst] = cmdq_mbox_create(cc->dev, inst);
	if (IS_ERR(cc->cmdq_client[inst])) {
		dev_err(cc->dev, "mbox create NG!!\n");
		return -EIO;
	}

	/* for dsi sram dump frame swap  && unmute usage */
	cc->cmdq_client[UNMUTE_CH] = cmdq_mbox_create(cc->dev, UNMUTE_CH);
	if (IS_ERR(cc->cmdq_client[UNMUTE_CH])) {
		dev_err(cc->dev, "eof mbox create NG!!\n");
		goto RL_MAIN;
	}

	/* for sbrc performance monitoring */
	cc->cmdq_client[GPU_CH] = cmdq_mbox_create(cc->dev, GPU_CH);
	if (IS_ERR(cc->cmdq_client[GPU_CH])) {
		dev_err(cc->dev, "mutex timer mbox create NG!!\n");
		goto RL_UNMUTE_CH;
	}

	/* for delayed vsync */
	cc->cmdq_client[VSYNC_CH] = cmdq_mbox_create(cc->dev, VSYNC_CH);
	if (IS_ERR(cc->cmdq_client[VSYNC_CH])) {
		dev_err(cc->dev, "vsync mbox create NG!!\n");
		goto RL_GPU_CH;
	}

	/* for wdma sof address update */
	cc->cmdq_client[WDMA_CH] = cmdq_mbox_create(cc->dev, WDMA_CH);
	if (IS_ERR(cc->cmdq_client[WDMA_CH])) {
		dev_err(cc->dev, "wdma sof mbox create NG!!\n");
		return PTR_ERR(cc->cmdq_client[WDMA_CH]);
	}

	/* for rdma address update */
	if (!ct->para.gp.strip_mode) {
		cc->cmdq_client[RDMA_CH] = cmdq_mbox_create(cc->dev, RDMA_CH);
		if (IS_ERR(cc->cmdq_client[RDMA_CH])) {
			dev_err(cc->dev, "rdma eof mbox create NG!!\n");
			return PTR_ERR(cc->cmdq_client[RDMA_CH]);
		}
	}

	/* cinematic mode front end using 0-7 */
	cc->vsync_mutex = mtk_mutex_get(ct->dev_mutex, VSYNC_DELAY);

	if (IS_ERR(cc->vsync_mutex)) {
		dev_err(cc->dev, "Get vsync mutex NG!!\n");
		goto RL_VSYNC_CH;
	}

	/* if enable sbrc error detect create gce monitor channel */
	if (ct->para.err_detect) {
		cc->cmdq_client[DSI_UNDERRUN_CH] = cmdq_mbox_create(cc->dev,
							DSI_UNDERRUN_CH);
		if (IS_ERR(cc->cmdq_client[DSI_UNDERRUN_CH])) {
			dev_err(cc->dev, "dsi_underrun mbox create NG!!\n");
			goto RL_MUTEX;
		}
		cc->cmdq_client[DSI_UNDERRUN_CPU_CH] = cmdq_mbox_create(cc->dev,
							DSI_UNDERRUN_CPU_CH);
		if (IS_ERR(cc->cmdq_client[DSI_UNDERRUN_CPU_CH])) {
			dev_err(cc->dev, "dsi_underrun cpu mbox create NG!!\n");
			goto RL_UNDERRUN_CH;
		}

	}

	/* if enable sbrc error recover create recover gce */
	if (ct->para.sbrc_recover && ct->para.gp.strip_mode) {
		cc->cmdq_client[SBRC_SKIP_DONE_CH] = cmdq_mbox_create(cc->dev,
							SBRC_SKIP_DONE_CH);
		if (IS_ERR(cc->cmdq_client[SBRC_SKIP_DONE_CH])) {
			dev_err(cc->dev, "skip frame mbox create NG!!\n");
			goto RL_UNDERRUN_CPU_CH;
		}
	}

#ifdef CONFIG_MACH_MT3612_A0
	if (ct->para.input_from_dp) {

		/* wait dp event to get video timing info */
		ret = wait_for_completion_interruptible_timeout(&cc->dp_stable,
								DP_TIMEOUT);

		if (ret < 0) {
			dev_err(cc->dev, "wait DP stable NG!!\n");
			goto DP_FAIL;
		} else if (ret == 0) {
			dev_err(cc->dev, "wait DP stable timeout!!\n");
			goto DP_FAIL;
		} else {
			do_gettimeofday(&done);
			diff = ((done.tv_sec - start.tv_sec) * 1000000L +
				done.tv_usec) - start.tv_usec;
			dev_dbg(cc->dev, "dprx elapsed time: %lu us\n", diff);
		}

		ret = 0;

		/* check active pipe settings match with dp src */
		if (!ct->dp_video_set) {
			dev_err(ct->dev, "dp src mismatch with cfg!!!\n");
			goto DP_FAIL;
		}
		mtk_cinematic_cb_event(ct, CB_DP_STABLE, NULL, 0);
	}
#endif

#if 0
	ct->target_line = (ct->para.rp.out_h >> 4) * 13;
#endif
	if (!Is4Kresolution(w, h)) {
		ret = mtk_single_pipe_config(ct);
		if (unlikely(ret)) {
			dev_err(ct->dev, "construct single pipe NG\n");
			goto RL_SKIP_DONE_CH;
		}
	} else {
		ct = cc->ct[MAIN_PIPE];
		if (!ct->para.quadruple_pipe) {
			cc->dual_mode = true;
			ret = mtk_dual_pipe_config(cc->ct);
			if (unlikely(ret)) {
				dev_err(ct->dev, "construct dual pipe NG!!\n");
				goto RL_SKIP_DONE_CH;
			}
		} else {
			ret = mtk_quad_pipe_config(cc->ct);
			if (unlikely(ret)) {
				dev_err(ct->dev, "construct quad pipe NG!!\n");
				goto RL_SKIP_DONE_CH;
			}
		}
	}

	if (ct->para.err_detect) {
		ret = mtk_cinematic_init_srbc_err_monitor(cc);
		if (unlikely(ret)) {
			dev_err(cc->dev, "init sbrc_err monitor NG!!\n");
			goto RL_SKIP_DONE_CH;
		}
	}

#ifdef CONFIG_COMMON_CLK_MT3612
	ret = mtk_sbrc_power_on(cc->dev_sbrc);
	if (unlikely(ret)) {
		dev_err(cc->dev, "Failed to power on sbrc: %d\n", ret);
		goto RL_SKIP_DONE_CH;
	}

	cc->sysram_on = 0;
	if (ct->para.gp.strip_mode == GPU_SRAM_STRIP_MODE) {
#ifdef CONFIG_MTK_GPU_DEDICATED_FW_MEMORY
		cc->sysram_base = GPU_SRAM_SIZE;
#else
		cc->sysram_base = 0;
#endif

		bpp = mtk_format_to_bpp(ct->para.gp.out_fmt) >> 3;
		cc->sysram_size = ct->para.gp.out_w * ct->para.gp.strip_h *
				  bpp * ct->para.gp.strip_buf_num;
		ret = mtk_mm_sysram_power_on(cc->dev_sysram,
					     cc->sysram_base,
					     cc->sysram_size);
		if (unlikely(ret)) {
			dev_err(cc->dev,
				"Failed to power on sysram: %d\n", ret);
			goto RL_SKIP_DONE_CH;
		} else {
			cc->sysram_on = 1;
		}
	}
#endif

	return 0;

	/* error process release resource allocated */
#ifdef CONFIG_MACH_MT3612_A0
DP_FAIL:
	mtk_dispsys_register_cb(cc->disp_dev, NULL, NULL);
	mtk_dispsys_dp_stop(cc->disp_dev);
#endif
RL_SKIP_DONE_CH:
	cmdq_mbox_destroy(cc->cmdq_client[SBRC_SKIP_DONE_CH]);
RL_UNDERRUN_CPU_CH:
	cmdq_mbox_destroy(cc->cmdq_client[DSI_UNDERRUN_CPU_CH]);
RL_UNDERRUN_CH:
	cmdq_mbox_destroy(cc->cmdq_client[DSI_UNDERRUN_CH]);
RL_MUTEX:
	mtk_mutex_put(cc->vsync_mutex);
RL_VSYNC_CH:
	cmdq_mbox_destroy(cc->cmdq_client[VSYNC_CH]);
RL_GPU_CH:
	cmdq_mbox_destroy(cc->cmdq_client[GPU_CH]);
RL_UNMUTE_CH:
	cmdq_mbox_destroy(cc->cmdq_client[UNMUTE_CH]);
RL_MAIN:
	cmdq_mbox_destroy(cc->cmdq_client[inst]);

	return -EIO;
}

static int mtk_cinemaitc_trigger_path(struct mtk_cinematic_ctrl *cc)
{
	struct mtk_cinematic_test *ct;
	u32 i;

	ct = get_active_pipe(cc);

	if (ct == NULL)
		return -EINVAL;

	if ((!ct->para.gp.bypass) && ct->para.gp.strip_mode) {
		mtk_cinematic_config_rbfc(cc, MTK_RBFC_NORMAL_MODE);
		mtk_cinematic_config_sbrc(cc);
		mtk_sbrc_enable(cc->dev_sbrc);
		dev_dbg(cc->dev, "cinematic RBFC normal mode\n");
	} else {
		mtk_cinematic_config_rbfc(cc, MTK_RBFC_DISABLE_MODE);
		mtk_sbrc_disable(cc->dev_sbrc);
		dev_dbg(cc->dev, "cinematic RBFC disable mode\n");
	}

	cc_thread = kthread_run(cc_task, (void *)cc, "cmt_frame_done");
	if (IS_ERR(cc_thread)) {
		dev_err(cc->dev, "cc thread startup NG!!\n");
		cc_thread = NULL;
		return -EPERM;
	}

	cc_vsync_thread = kthread_run(vsync_task, (void *)cc,
				      "cmt_vsync");
	if (IS_ERR(cc_vsync_thread)) {
		dev_err(cc->dev, "!! vsync thread startup NG!!\n");
		cc_vsync_thread = NULL;
		return -EPERM;
	}

	cc_sof_thread = kthread_run(cc_sof_task,
			(void *)cc, "cmt_wdma_sof");
	if (IS_ERR(cc_sof_thread)) {
		dev_err(cc->dev, "err thread startup NG!!\n");
		cc_sof_thread = NULL;
		return -EPERM;
	}

	if (!ct->para.gp.strip_mode) {
		cc_rdma_thread = kthread_run(cc_rdma_swap_task, (void *)cc,
					     "cmt_rdma");
		if (IS_ERR(cc_rdma_thread)) {
			dev_err(cc->dev, "err rdma thread startup NG!!\n");
			cc_rdma_thread = NULL;
			return -EPERM;
		}
	} else {
		enable_strip_monitor(cc);
	}

	mtk_wdma_register_cb(ct->dev_wdma, mtk_cinematic_wdma_cb,
			     MTK_WDMA_FRAME_UNDERRUN, cc);
	if (ct->para.quadruple_pipe) {
		for (i = 1; i < 4; i++) {
			mtk_wdma_register_cb(cc->ct[i]->dev_wdma,
					mtk_cinematic_wdma_cb,
					MTK_WDMA_FRAME_UNDERRUN, cc);
		}
	}

	if (ct->para.err_detect) {

		if (enable_dsi_gce_monitor(cc)) {
			dev_err(cc->dev, "err gce dsi thread startup NG!!\n");
			return -EPERM;
		}

		cc_dsi_thread = kthread_run(cc_monitor_dsi_task,
				(void *)cc, "cmt_dsi_err");
		if (IS_ERR(cc_dsi_thread)) {
			dev_err(cc->dev, "err thread startup NG!!\n");
			cc_dsi_thread = NULL;
			return -EPERM;
		}
	}

	if (ct->para.sbrc_recover && ct->para.gp.strip_mode) {
		cc_err_thread = kthread_run(cc_monitor_err_task,
				(void *)cc, "cmt_sbrc_recover");
		if (IS_ERR(cc_err_thread)) {
			dev_err(cc->dev, "err thread startup NG!!\n");
			cc_err_thread = NULL;
			return -EPERM;
		}
	}
	return 0;
}

static int mtk_cinematic_deinit_path(struct mtk_cinematic_ctrl *cc)
{
	int inst;
	int i;
	struct mtk_cinematic_test *ct;
	struct mtk_mutex_res *td_mutex;
	struct exit_cb_data cb_data;
#ifdef CONFIG_MACH_MT3612_A0
	int ret;
#endif

	ct = get_active_pipe(cc);
	inst = cc->act_inst;

	if (ct == NULL)
		return -EINVAL;

	mtk_wdma_register_cb(ct->dev_wdma, NULL, 0, cc);
	if (ct->para.quadruple_pipe) {
		for (i = 1; i < 4; i++) {
			mtk_wdma_register_cb(cc->ct[i]->dev_wdma,
					NULL, 0, cc);
		}
	}

	kthread_stop(cc_sof_thread);
	cmdq_mbox_destroy(cc->cmdq_client[WDMA_CH]);

	if (!ct->para.gp.strip_mode) {
		kthread_stop(cc_rdma_thread);
		cmdq_mbox_destroy(cc->cmdq_client[RDMA_CH]);
		cc->inserted_rdma_buf = 0;
	}

	kthread_stop(cc_vsync_thread);
	cmdq_mbox_destroy(cc->cmdq_client[VSYNC_CH]);

	kthread_stop(cc_thread);
	cmdq_mbox_destroy(cc->cmdq_client[inst]);

	flush_queue(mtk_jobs_q);

	cmdq_mbox_destroy(cc->cmdq_client[UNMUTE_CH]);

	td_mutex = ct->mutex;
	mtk_mutex_timer_disable(td_mutex, NULL);
	cmdq_mbox_destroy(cc->cmdq_client[GPU_CH]);

	if (ct->para.err_detect) {
		mtk_cinematic_deinit_srbc_err_monitor(cc);
		cmdq_mbox_destroy(cc->cmdq_client[DSI_UNDERRUN_CH]);
		cmdq_mbox_destroy(cc->cmdq_client[DSI_UNDERRUN_CPU_CH]);
		kthread_stop(cc_dsi_thread);
	}

	if (ct->para.sbrc_recover && ct->para.gp.strip_mode) {
		cmdq_mbox_destroy(cc->cmdq_client[SBRC_SKIP_DONE_CH]);
		kthread_stop(cc_err_thread);
	}
	/* disable lhc */
	mtk_dispsys_lhc_disable(cc->disp_dev);
	/* disable rdma to dsi */
	mtk_dispsys_disable(cc->disp_dev);
	cc->displayed = false;
	mtk_sbrc_disable(cc->dev_sbrc);

	/* disable slicer to wdma */
	if (Is4Kresolution(ct->w, ct->h)) {
		if (!ct->para.quadruple_pipe)
			mtk_cinematic_dual_stop(cc->ct);
		else
			mtk_cinematic_quad_stop(cc->ct);
	} else {
		mtk_cinematic_stop(ct);
	}
	/* release vsync mutex once exit */
	mtk_mutex_put(cc->vsync_mutex);

	/* parepare exit data for user space program */
	cb_data.wdma_sof_cnt = cc->wdma_sof_cnt;
	cb_data.wdma_frame_cnt = cc->wdma_frame_cnt;
	cb_data.skip_frame_cnt = cc->skip_frame_cnt;
	mtk_cinematic_cb_event(ct, CB_EXIT, (void *)&cb_data,
			       sizeof(struct exit_cb_data));

#ifdef CONFIG_MACH_MT3612_A0
	/* power off sbrc */
	ret = mtk_sbrc_power_off(cc->dev_sbrc);
	if (unlikely(ret))
		dev_err(cc->dev, "Failed to power off sbrc: %d\n", ret);
	/* power off sysram buffer*/
	if (cc->sysram_on) {
		ret = mtk_mm_sysram_power_off(cc->dev_sysram,
					     cc->sysram_base,
					     cc->sysram_size);

		if (unlikely(ret))
			dev_err(cc->dev,
				"Failed to power off sysram: %d\n", ret);
	}

	if (ct->para.input_from_dp) {
		mtk_dispsys_register_cb(cc->disp_dev, NULL, NULL);
		mtk_dispsys_dp_stop(cc->disp_dev);
	}
#endif
	return 0;
}

int mtk_cinematic_dispatch_cmd(struct mtk_cinematic_ctrl *cc)
{
	int inst;
	int ret = 0;
	struct mtk_cinematic_test *ct;

	ct = get_active_pipe(cc);
	inst = cc->act_inst;

	if (ct == NULL)
		return -EINVAL;

	switch (cc->cmd) {
	case INIT_PATH:
		/* initialize path */
		dev_dbg(cc->dev, "### cmd: construct single pipe[%d] ###\n",
			inst);

		ret = mtk_cinematic_init_path(cc);
		if (unlikely(ret)) {
			dev_err(cc->dev, "cinematic_init_path NG!!\n");
			return ret;
		}

		ret = mtk_cinematic_enable_disp(cc);
		if (unlikely(ret)) {
			dev_err(cc->dev, "cinematic_enable_disp NG!!\n");
			return ret;
		}

		/* lhc from DP config */
		if (ct->para.lhc_en && (ct->para.lhc_sync_path == FROM_DP)) {
			int lhc_input_slice_nr;

			if (ct->w >= 3840) {
				if (ct->para.quadruple_pipe)
					lhc_input_slice_nr = 4;
				else
					lhc_input_slice_nr = 2;
			} else {
				lhc_input_slice_nr = 1;
			}

			mtk_dispsys_lhc_slices_enable(cc->disp_dev,
				lhc_input_slice_nr, ct->disp_path,
				&ct->lhc_slice_cfg);
#if RUN_BIT_TRUE
{
	struct device *lhc_dev;
	struct mtk_lhc_color_trans lct = {0};
	int ret = -1;

	lct.enable = 1;
	lct.external_matrix = 0;
	lct.int_matrix = LHC_INT_TBL_BT601F;

	lhc_dev = mtk_dispsys_get_lhc(cc->disp_dev);
	ret = mtk_lhc_set_color_transform(lhc_dev, &lct, NULL);
	if (ret != 0) {
		dev_err(cc->dev, "Enable LHC CSC FAIL!!!\n");
		return ret;
	}
}
#endif

		}
		break;
	case TRIG_PATH:
		dev_dbg(cc->dev, "### cmd: trig single pipe[%d] ###\n", inst);

		ret = mtk_cinemaitc_trigger_path(cc);
		if (unlikely(ret)) {
			dev_err(cc->dev, "cinematic_trig_path NG!!\n");
			return ret;
		}

#ifdef SLICER_DBG
		slicer_interrupt_en(cc);
#endif
		ret = mtk_cinematic_trigger_no_wait(ct);
		if (unlikely(ret)) {
			dev_err(cc->dev, "cinematic_trig_on_wait NG!!\n");
			return ret;
		}

		break;
	case RESET_PATH:
		dev_dbg(cc->dev, "### cmd: hw reset pipe[%d] ###\n", inst);
		ret = mtk_reset_core_modules(ct);
		if (unlikely(ret)) {
			dev_err(cc->dev, "ioctl reset core NG!!\n");
			return ret;
		}

		break;
	case DISP_PATH:
		dev_dbg(cc->dev, "### cmd: set display pipe[%d] ###\n", inst);

		ret = dsi_unmute(cc);
		if (unlikely(ret)) {
			dev_err(cc->dev, "DSI UNMUTE NG!!\n");
			return ret;
		}

		cc->displayed = true;
		mtk_cinematic_cb_event(ct, CB_DISP_DISPLAY, NULL, 0);
		dev_dbg(cc->dev, "### unmute dsi for gpu on case ###\n");

		break;
	case DEINIT_PATH:
		dev_dbg(cc->dev, "### cmd: disable display pipe[%d] ###\n",
			inst);
		ret = mtk_cinematic_deinit_path(cc);
		if (unlikely(ret)) {
			dev_err(cc->dev, "cinematic_deinit NG!!\n");
			return ret;
		}

		break;
	case PAUSE_PATH:
		mtk_cinematic_cb_event(ct, CB_DISP_PAUSE, NULL, 0);
		break;
	case RESUME_PATH:
		/* do nothing here */
		break;
	default:
		dev_dbg(cc->dev, "### Unknown CMD  ###\n");
		return -EINVAL;
	}
	return 0;
}
EXPORT_SYMBOL(mtk_cinematic_dispatch_cmd);

static int mtk_check_command(struct mtk_cinematic_ctrl *cc,
			     enum pipe_cmd_list usr_cmd)
{
	struct mtk_cinematic_test *ct;

	if (IS_ERR(cc))
		return -EINVAL;

	ct = get_active_pipe(cc);

	switch (usr_cmd) {
	case PAUSE_PATH:
		if (cc->cmd != DISP_PATH) {

			/* quad do not display, we accept pause for dump */
			if (!ct->para.quadruple_pipe) {
				dev_err(cc->dev,
					"error only display mode can pause\n");
				return -EIO;
			}
		}
		dev_dbg(cc->dev, "paused cmd received\n");
		break;
	case RESUME_PATH:
		if (cc->cmd != PAUSE_PATH) {
			dev_err(cc->dev, "error! only pause mode can resume\n");
			return -EIO;
		}
		dev_dbg(cc->dev, "resume cmd received\n");
		break;
	default:
		return 0;
	}
	return 0;
}

static long mtk_cinematic_ioctl(struct file *filp, unsigned int cmd,
				unsigned long arg)
{
	struct mtk_cinematic_ctrl *cc;
	struct mtk_cinematic_test *ct;
	struct mtk_common_buf *buf_handle;
	struct device *dev;
	struct device *dma_dev;
	char k_buf[128];
	union cmd_data_t data;
	unsigned int dir;
	int ret = 0;
	int dsi_mute = 0;
	int put_inst = 0;
	u32 i = 0;


	cc = filp->private_data;
	dev = cc->dev;

	ct = get_active_pipe(cc);

	/* if no active pipe return invalid */
	if (ct == NULL)
		return -EINVAL;

	dir = _IOC_DIR(cmd);

	if (_IOC_SIZE(cmd) > sizeof(data))
		return -EINVAL;

	if (dir & _IOC_WRITE)
		if (copy_from_user(&data, (void __user *)arg, _IOC_SIZE(cmd)))
			return -EFAULT;

	switch (cmd) {
	case IOCTL_READ:
		/* to do: */
		break;
	case IOCTL_WRITE:
		/* to do: */
		break;
	case IOCTL_MUTE_DSI:
		/* mute dsi, re-use union members */
		dsi_mute = data.fb_id;
		dev_dbg(dev, "MUTE_DSI IOCTL %d\n", dsi_mute);

		if (dsi_mute)
			ct->gpu_fb_id = 0xff;

		ret = mtk_dispsys_mute(cc->disp_dev, dsi_mute, NULL);
		if (unlikely(ret)) {
			dev_err(dev, "MUTE DSI NG!!!!!\n");
			return -EFAULT;
		}
		break;
	case IOCTL_SET_PIPE:
		ret = mtk_check_command(cc, data.cmd);
		if (unlikely(ret))
			return -EIO;

		/* get pipe cmd */
		cc->cmd = data.cmd;
		/* pipe to work */
		ret = mtk_cinematic_dispatch_cmd(cc);
		if (unlikely(ret)) {
			dev_err(dev, "dispatch cmd NG!!!!!\n");
			return -EFAULT;
		}
		break;
	case IOCTL_QUEUE_JOBS:
		ct = get_active_pipe(cc);
		/* if no active pipe return invalid */
		if (ct == NULL)
			return -EINVAL;

		/* DUAL PIPE Do NOT support job insertion */
		if (Is4Kresolution(ct->w, ct->h))
			return -EPERM;

		/* en queue job */
		if (mutex_lock_interruptible(&cc->queue_lock))
			return -ERESTARTSYS;
		ret = enqueue(mtk_jobs_q, data.usr_job);
		mutex_unlock(&cc->queue_lock);

		if (unlikely(ret))
			return -EPERM;
		break;
	case IOCTL_GET_WDMA_FRAME_CNT:
		data.wdma_frame_cnt = cc->wdma_frame_cnt;
		break;
	case IOCTL_GET_WDMA_FRAME_ID:

		data.fb_id = cc->disp_fb->fb_id;
		mtk_cinematic_fb_to_gpu(ct, data.fb_id);
#if 0
		dev_dbg(cc->dev, "disp_fb = %d, cur_fb = %d\n",
			data.fb_id, ct->cur_fb_id);
#endif
		LOG_FB("disp_fb = %d, cur_fb = %d\n",
		       data.fb_id, ct->cur_fb_id);

		/* only if input fps > disp_fps */
		if (data.fb_id == ct->cur_fb_id)
			dev_err(ct->dev, "disp_fb == cur_fb %d!!", data.fb_id);

		if (ct->seq_chg)
			ct->seq_chg = false;
		break;
	case IOCTL_GET_RBFC_WR_AVG_DIFF:
		/* re-use fb_id as AVG return */
#ifdef CAHCED_BUF
		dma_sync_single_for_cpu(ct->dev, cc->sd.d_pa[SUM_DIFF],
					sizeof(u32), DMA_FROM_DEVICE);
		dma_sync_single_for_cpu(ct->dev, cc->sd.d_pa[F_COUNT],
					sizeof(u32), DMA_FROM_DEVICE);
#endif
		data.fb_id = *cc->sd.d_va[SUM_DIFF] / *cc->sd.d_va[F_COUNT];
		break;
	case IOCTL_IMPORT_FD_TO_HANDLE:
		dev_dbg(dev, "FD_TO_HANDLE called\n");
		dev_dbg(dev, "handle.index: %d\n", data.handle.index);
		dev_dbg(dev, "handle.buf_type: %d\n", data.handle.buf_type);

		switch (data.handle.buf_type) {
		case CINEMATIC_WDMA0:
			dma_dev = cc->ct[PIPE_0]->dev_wdma;
			break;
		case CINEMATIC_WDMA1:
			dma_dev = cc->ct[PIPE_1]->dev_wdma;
			break;
		case CINEMATIC_WDMA2:
			dma_dev = cc->ct[PIPE_2]->dev_wdma;
			break;
		case CINEMATIC_WDMA3:
			dma_dev = cc->ct[PIPE_3]->dev_wdma;
			break;
		case CINEMATIC_RDMA0:
			dma_dev = mtk_dispsys_get_rdma(cc->disp_dev);
			break;
		default:
			dev_dbg(dev, "## finding cinematic dma_dev fail\n");
			return -EINVAL;
		}

		ret = mtk_common_fd_to_handle_offset(dma_dev,
				data.handle.fd, &(data.handle.handle),
				data.handle.offset);
		if (unlikely(ret)) {
			dev_dbg(dev, "FD_TO_HANDLE: import buf failed,ret=%d\n",
				ret);
			return -EFAULT;
		}

		dev_dbg(dev, "common fd to handle success\n");
		break;
	case IOCTL_SET_BUF:
		dev_dbg(dev, "IOCTL set BUF\n");

		buf_handle = (struct mtk_common_buf *)data.set_buf.handle;
		buf_handle->pitch = data.set_buf.pitch;
		buf_handle->format = data.set_buf.format;
		buf_handle->offset = data.set_buf.offset;

		dev_dbg(dev, "kernel set buf dma_addr = %p\n",
				(void *)buf_handle->dma_addr);

		dev_dbg(dev, "set_buf.buf_type = %d , set_buf.index = %d\n",
			data.set_buf.buf_type, data.set_buf.dev_index);
		dev_dbg(dev, "## set_buf.offset = %d(0x%x) ##",
			data.set_buf.offset, data.set_buf.offset);

		/* determine path to assign buffer */
		switch (data.set_buf.buf_type) {
		case CINEMATIC_WDMA0:
			cc->wdma_ion_buf[data.set_buf.dev_index] = buf_handle;
			mtk_cinematic_set_buf_mode(cc->ct[PIPE_0], ION_BUF);
			mtk_cinematic_set_ion_buf(cc->ct[PIPE_0], buf_handle,
						  data.set_buf.dev_index);
			break;
		case CINEMATIC_WDMA1:
			cc->wdma_ion_buf[data.set_buf.dev_index] = buf_handle;
			mtk_cinematic_set_buf_mode(cc->ct[PIPE_1], ION_BUF);
			mtk_cinematic_set_ion_buf(cc->ct[PIPE_1], buf_handle,
						  data.set_buf.dev_index);
			break;
		case CINEMATIC_WDMA2:
			cc->wdma_ion_buf[data.set_buf.dev_index] = buf_handle;
			mtk_cinematic_set_buf_mode(cc->ct[PIPE_2], ION_BUF);
			mtk_cinematic_set_ion_buf(cc->ct[PIPE_2], buf_handle,
						  data.set_buf.dev_index);
			break;
		case CINEMATIC_WDMA3:
			cc->wdma_ion_buf[data.set_buf.dev_index] = buf_handle;
			mtk_cinematic_set_buf_mode(cc->ct[PIPE_3], ION_BUF);
			mtk_cinematic_set_ion_buf(cc->ct[PIPE_3], buf_handle,
						  data.set_buf.dev_index);
			break;
		case CINEMATIC_RDMA0:
			cc->rdma_ion_buf[data.set_buf.dev_index] = buf_handle;
			cc->inserted_rdma_buf++;
			break;
		default:
			dev_err(dev, "SET_BUF:buf type not found NG!!\n");
			return -EINVAL;
		}
		break;
	case IOCTL_PUT_HANDLE:
		dev_dbg(dev, "PUT_HANDLE\n");
		dev_dbg(dev, "handle.index: %d\n", data.put_handle.index);
		dev_dbg(dev, "handle.buf_type: %d\n", data.put_handle.buf_type);

		ret = mtk_common_put_handle(data.put_handle.handle);

		if (unlikely(ret)) {
			dev_dbg(dev, "put_handle.handle is NULL!!\n");
			/*return ret;*/
		}

		put_inst = data.put_handle.buf_type;

		switch (data.put_handle.buf_type) {
		case CINEMATIC_WDMA0:
			cc->wdma_ion_buf[data.put_handle.index] = NULL;
			mtk_cinematic_set_ion_buf(cc->ct[PIPE_0], NULL,
						  data.put_handle.index);
			break;
		case CINEMATIC_WDMA1:
			cc->wdma_ion_buf[data.put_handle.index] = NULL;
			mtk_cinematic_set_ion_buf(cc->ct[PIPE_1], NULL,
						  data.put_handle.index);
			break;
		case CINEMATIC_WDMA2:
			cc->wdma_ion_buf[data.put_handle.index] = NULL;
			mtk_cinematic_set_ion_buf(cc->ct[PIPE_2], NULL,
						  data.put_handle.index);
			break;
		case CINEMATIC_WDMA3:
			cc->wdma_ion_buf[data.put_handle.index] = NULL;
			mtk_cinematic_set_ion_buf(cc->ct[PIPE_3], NULL,
						  data.put_handle.index);
			break;
		case CINEMATIC_RDMA0:
			cc->rdma_ion_buf[data.put_handle.index] = NULL;
			break;
		default:
			dev_err(dev, "PUT_HANDLE:DEV index failed,ret=%d\n",
			ret);
			return -EINVAL;
		}

		if (put_inst <= CINEMATIC_WDMA3)
			if (!cc->ct[put_inst]->inserted_buf)
				mtk_cinematic_set_buf_mode(cc->ct[put_inst],
							   DRAM_MODE);

	break;
	case IOCTL_SET_PARAMETER:
		cc->act_inst = data.para.pipe_idx;
		dev_dbg(dev, "IOCTL_SET_PARAMETER\n");

		if (check_src_validation(data.para.input_width,
					 data.para.input_height)) {
			dev_dbg(cc->dev,
				"src timing not support, using 1280x720\n");
			data.para.input_width = 1280;
			data.para.input_height = 720;
		}

		/* get twin mode configuration */
		#if 1
		for (i = 0; i < CINEMATIC_MODULE_CNT ; i++) {
			dev_dbg(cc->dev, "in0 [%d %d]\n in1 [%d %d]\n\n",
			data.para.cinematic_cfg[i].in_start[0],
			data.para.cinematic_cfg[i].in_end[0],
			data.para.cinematic_cfg[i].in_start[1],
			data.para.cinematic_cfg[i].in_end[1]);
		}
		#endif
		cc->ct[cc->act_inst]->para = data.para;

		ret = mtk_cinematic_cfg_disp(cc);
		if (unlikely(ret)) {
			dev_err(cc->dev, "cinematic_cfg_disp NG!!\n");
			return ret;
		}

		break;
	case IOCTL_SET_DISP_ID:
		LOG_OUT("gpu set Disp_ID[%d]\n", data.fb_id);

		if (cc->cmd == DISP_PATH || cc->cmd == TRIG_PATH) {

			cc->rdma_buf_id = data.fb_id;

			if (!cc->gpu_frame_cnt)
				dev_dbg(cc->dev, "Get first GPU frame !!\n");

			if (cc->cmd != DEINIT_PATH)
				cc->gpu_frame_cnt++;
		} else {
			dev_dbg(cc->dev, "set rdma buf in wrong state\n");
			return -EIO;
		}
		break;
	case IOCTL_DUMP_WDMA:

		if (data.fb_id >= ct->inserted_buf) {
			dev_err(cc->dev, "dump index invalid\n");
			return -EIO;
		}

		memset(k_buf, 0, sizeof(k_buf));
		snprintf(k_buf, sizeof(k_buf), "/tmp/wdma_%d.bin", data.fb_id);

		/* quad do not display, we accept dump flow */
		if (cc->displayed || ct->para.quadruple_pipe)
			mtk_dump_buf(ct, data.fb_id, ct->image_output_size,
				     k_buf);
		else
			dev_dbg(dev, "=== buf not ready skip saving ===\n");
		break;
	case IOCTL_DUMP_RDMA:

		if (data.fb_id >= cc->inserted_rdma_buf) {
			dev_err(cc->dev, "dump index invalid\n");
			return -EIO;
		}

		if (cc->displayed)
			mtk_dump_rdma_buf(cc, data.fb_id);
		else
			dev_dbg(dev, "=== buf not ready skip saving ===\n");

		break;
#ifdef SBRC_FORCE_READ_CTRL
	case IOCTL_SET_STRIP_BUF_ID:
		mtk_sbrc_set_sw_control_read_idx(cc->dev_sbrc,
						 data.fb_id, NULL);
		break;
#endif
	default:
		return -ENOTTY;
	}

	if (dir & _IOC_READ)
		if (copy_to_user((void __user *)arg, &data, _IOC_SIZE(cmd)))
			return -EFAULT;

	return 0;
}

static int cinematic_char_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
	struct mtk_cinematic_ctrl *cc = container_of(inode->i_cdev,
			struct mtk_cinematic_ctrl, chardev);

	filp->private_data = cc;
	cc->frame_done = 0;
	cc->vsync = 0;
	read_size = 0;

	/* prepare kernel event cb */
	init_completion(&read_complete);
	sema_init(&event_sema, 1);

	return ret;
}

static int cinematic_char_release(struct inode *inode, struct file *filp)
{
	/* we do nothing here */
	return 0;
}

static ssize_t cinematic_char_write(struct file *filp, const char *buffer,
				    size_t len, loff_t *offset)
{
	struct mtk_cinematic_ctrl *cc;
	struct mtk_cinematic_test *ct;
	struct device *dev;
	char *k_buf = vmalloc(len+1);

	cc = filp->private_data;
	dev = cc->dev;

	if (!k_buf)
		goto out;

	ct = get_active_pipe(cc);
	if (ct == NULL) {
		vfree(k_buf);
		return -EINVAL;
	}

	if (copy_from_user(k_buf, buffer, len)) {
		vfree(k_buf);
		return -EFAULT;
	}
	k_buf[len] = '\0';

	if (cc->displayed)
		mtk_dump_buf(ct, cc->disp_fb->fb_id, ct->image_output_size,
			     k_buf);
	else
		dev_dbg(dev, "=== buf not ready skip saving ===\n");

	vfree(k_buf);
	return len;
out:
	dev_err(dev, "vmalloc NG!!\n");
	return 0;
}

static ssize_t cinematic_char_read(struct file *filp, char *buffer, size_t len,
				   loff_t *offset)
{
	struct mtk_cinematic_ctrl *cc;
	int read_cnt = 0, rc = 0;

	cc = filp->private_data;

	if (len > CB_BUF_SIZE)
		return -EINVAL;

	rc = wait_for_completion_interruptible(&read_complete);
	if (rc < 0)
		goto out;

	if (copy_to_user(buffer, read_buffer, read_size) != 0) {
		up(&event_sema);
		return -EFAULT;
	}

	read_cnt = read_size;
	read_size = 0;
	up(&event_sema);

	return read_cnt;
out:
	return -ERESTARTSYS;
}

unsigned int mtk_cinematic_poll(struct file *filp, poll_table *wait)
{
	unsigned int mask = 0;
	unsigned long flags;
	struct mtk_cinematic_ctrl *cc;

	cc = filp->private_data;

	mutex_lock(&cc->poll_lock);

	poll_wait(filp, &cc->vsync_wait, wait);

	/* we have a vsync*/
	if (cc->vsync) {

		mask |= POLLIN;

		spin_lock_irqsave(&cc->spinlock_irq, flags);
		cc->vsync = 0;
		spin_unlock_irqrestore(&cc->spinlock_irq, flags);
	}

	mutex_unlock(&cc->poll_lock);

	return mask;
}

static const struct file_operations cinematic_ctrl_fops = {
	.owner = THIS_MODULE,
	.open = cinematic_char_open,
	.write = cinematic_char_write,
	.read = cinematic_char_read,
	.release = cinematic_char_release,
	.poll = mtk_cinematic_poll,
	.unlocked_ioctl = mtk_cinematic_ioctl,
};

static int reg_chardev(struct mtk_cinematic_ctrl *cc)
{
	struct device *dev;
	char dev_name[] = "mtk_cinematic";
	int ret = 0;

	ret = alloc_chrdev_region(&cc->devt, 0, 1,
				dev_name);
	if (ret < 0) {
		pr_err(" alloc_chrdev_region failed, %d\n",
				ret);
		return ret;
	}

	/* Attatch file operation. */
	cdev_init(&cc->chardev, &cinematic_ctrl_fops);
	cc->chardev.owner = THIS_MODULE;

	/* Add to system */
	ret = cdev_add(&cc->chardev, cc->devt, 1);
	if (ret < 0) {
		pr_err("attatch file operation failed, %d\n",
			ret);
		unregister_chrdev_region(cc->devt, 1);
		return -EIO;
	}

	/* Create class register */
	cc->dev_class = class_create(THIS_MODULE, dev_name);
	if (IS_ERR(cc->dev_class)) {
		ret = PTR_ERR(cc->dev_class);
		pr_err("Unable to create class, err = %d\n", ret);
		return -EIO;
	}

	dev = device_create(cc->dev_class, NULL, cc->devt,
			    NULL, dev_name);
	if (IS_ERR(dev)) {
		ret = PTR_ERR(dev);
		pr_err("Failed to create device: /dev/%s, err = %d",
				dev_name, ret);
		return -EIO;
	}

	read_buffer = kmalloc(CB_BUF_SIZE, GFP_KERNEL);

	if (IS_ERR(read_buffer)) {
		pr_err("read buffer allocated fail\n");
		return -EIO;
	}

	dev_dbg(cc->dev, "mtk_reg_chardev cinematic done\n");
	return 0;
}

static int mtk_cinematic_ctrl_get_device(struct device *dev,
		char *compatible, int idx, struct device **child_dev)
{
	struct platform_device *pdev;
	struct device_node *node;

	node = of_parse_phandle(dev->of_node, compatible, idx);
	if (!node) {
		dev_err(dev, "mtk_get_device: could not find %s %d\n",
				compatible, idx);
		return -ENODEV;
	}

	pdev = of_find_device_by_node(node);
	if (!pdev || !pdev->dev.driver) {
		dev_warn(dev, "mtk__get_device: waiting for device %s\n",
			node->full_name);

		return -EPROBE_DEFER;
	}

	*child_dev = &pdev->dev;
	return 0;
}

static int mtk_cinematic_ctrl_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device *worker;
	struct device *dsi_dev;
	struct mtk_cinematic_ctrl *cc;
#ifndef CACHED_BUF
	struct dma_attrs dma_attrs;
#endif
	int ret = 0;
	int i = 0, buf_num = 0;
	u32 lcm_vtotal;

	cc = devm_kzalloc(dev, sizeof(struct mtk_cinematic_ctrl), GFP_KERNEL);

	if (!cc)
		return -ENOMEM;

	cc->dev = dev;
	dev_dbg(cc->dev, "cc addr %p\n", cc);

	/* init usr jobs q */
	mtk_jobs_q = devm_kzalloc(dev, sizeof(struct cc_queue), GFP_KERNEL);

	if (!mtk_jobs_q)
		return -ENOMEM;

	cc->disp_fb = devm_kzalloc(dev, sizeof(*cc->disp_fb), GFP_KERNEL);

	if (!cc->disp_fb)
		return -ENOMEM;

	/* prepare strip monitor dma */
#ifdef CACHED_BUF
	for (i = 0; i < DIFF_COUNT_MAX; i++) {
		cc->sd.d_va[i] = devm_kzalloc(dev, sizeof(u32), GFP_KERNEL);
		if (IS_ERR(cc->sd.d_va[i]))
			return -ENOMEM;
	}

	for (i = 0; i < DIFF_COUNT_MAX; i++) {
		cc->sd.d_pa[i] = dma_map_single(dev, cc->sd.d_va[i],
						sizeof(u32), DMA_FROM_DEVICE);
		if (dma_mapping_error(dev, cc->sd.d_pa[i])) {
			dev_err(dev, "mapping strip monitor dma failed!\n");
			goto free_resource;
		}
	}

	rec_va_d = (u32 *)devm_kzalloc(dev, sizeof(u32) * REC_NUM, GFP_KERNEL);
	if (IS_ERR(rec_va_d))
		goto free_resource;

	rec_va_w = (u32 *)devm_kzalloc(dev, sizeof(u32) * REC_NUM, GFP_KERNEL);
	if (IS_ERR(rec_va_w))
		goto free_resource;

	rec_va_r = (u32 *)devm_kzalloc(dev, sizeof(u32) * REC_NUM, GFP_KERNEL);
	if (IS_ERR(rec_va_r))
		goto free_resource;

	rec_pa_d = dma_map_single(dev, (void *)rec_va_d, sizeof(u32) * REC_NUM,
				  DMA_FROM_DEVICE);
	if (dma_mapping_error(dev, rec_pa_d)) {
		dev_err(dev, "mapping strip monitor dma diff failed!\n");
		goto free_resource_d;
	}

	rec_pa_w = dma_map_single(dev, (void *)rec_va_w, sizeof(u32) * REC_NUM,
				  DMA_FROM_DEVICE);
	if (dma_mapping_error(dev, rec_pa_w)) {
		dev_err(dev, "mapping strip monitor dma write failed!\n");
		goto free_resource_w;
	}

	rec_pa_r = dma_map_single(dev, (void *)rec_va_r, sizeof(u32) * REC_NUM,
				  DMA_FROM_DEVICE);
	if (dma_mapping_error(dev, rec_pa_r)) {
		dev_err(dev, "mapping strip monitor dma read failed!\n");
		goto free_resource_r;
	}
	dev_dbg(dev, "cached buffer allocated!!!\n");
#else
	init_dma_attrs(&dma_attrs);
	dma_set_attr(DMA_ATTR_WRITE_COMBINE | DMA_ATTR_FORCE_CONTIGUOUS,
		     &dma_attrs);

	for (i = 0; i < DIFF_COUNT_MAX; i++) {
		cc->sd.d_va[i] = (u32 *)dma_alloc_attrs(cc->dev, sizeof(u32),
				&cc->sd.d_pa[i], GFP_KERNEL, &dma_attrs);
		if (IS_ERR(cc->sd.d_va[i]))
			goto free_dma_alloc;
	}

	rec_va_d  = (u32 *)dma_alloc_attrs(cc->dev, sizeof(u32) * REC_NUM,
					   &rec_pa_d, GFP_KERNEL, &dma_attrs);
	if (IS_ERR(rec_va_d))
		goto free_dma_alloc_d;

	rec_va_w  = (u32 *)dma_alloc_attrs(cc->dev, sizeof(u32) * REC_NUM,
					   &rec_pa_w, GFP_KERNEL, &dma_attrs);
	if (IS_ERR(rec_va_w))
		goto free_dma_alloc_w;

	rec_va_r  = (u32 *)dma_alloc_attrs(cc->dev, sizeof(u32) * REC_NUM,
					   &rec_pa_r, GFP_KERNEL, &dma_attrs);
	if (IS_ERR(rec_va_r))
		goto free_dma_alloc_r;

	/*---------AH USED---------------*/
	rec_va_rdma_st  = (u32 *)dma_alloc_attrs(cc->dev, sizeof(u32),
						 &rec_pa_rdma_st,
						 GFP_KERNEL, &dma_attrs);
	if (IS_ERR(rec_va_rdma_st))
		goto free_dma_alloc_rdma;

	rec_va_dsi_out  = (u32 *)dma_alloc_attrs(cc->dev, sizeof(u32),
						 &rec_pa_dsi_out,
						 GFP_KERNEL, &dma_attrs);
	if (IS_ERR(rec_va_dsi_out))
		goto free_dma_alloc_dsi_out;

	rec_va_w1  = (u32 *)dma_alloc_attrs(cc->dev, sizeof(u32),
					   &rec_pa_w1, GFP_KERNEL, &dma_attrs);
	if (IS_ERR(rec_va_w1))
		goto free_dma_alloc_w1;

	rec_va_dnc  = (u32 *)dma_alloc_attrs(cc->dev, sizeof(u32),
					   &rec_pa_dnc, GFP_KERNEL, &dma_attrs);
	if (IS_ERR(rec_va_dnc))
		goto free_dma_alloc_dnc;

	rec_va_dsi_in  = (u32 *)dma_alloc_attrs(cc->dev, sizeof(u32),
						&rec_pa_dsi_in, GFP_KERNEL,
						&dma_attrs);
	if (IS_ERR(rec_va_dsi_in))
		goto free_dma_alloc_dsi_in;

	/*-----------------------------*/

	skip_next_va = (u32 *)dma_alloc_attrs(cc->dev, sizeof(u32),
					&skip_next_pa, GFP_KERNEL, &dma_attrs);
	if (IS_ERR(skip_next_va))
		goto free_dma_alloc_s;

#ifdef SBRC_FORCE_READ_CTRL
	forced_buf_id_va = (u32 *)dma_alloc_attrs(cc->dev, sizeof(u32),
				&forced_buf_id_pa, GFP_KERNEL, &dma_attrs);
	if (IS_ERR(forced_buf_id_va))
		goto free_dma_alloc_f;
#endif

#endif
	dev_dbg(dev, "=== tmp  va = %p, pa = %pad\n", rec_va_d, &rec_pa_d);

	/* end of prepare */

	mtk_jobs_q->size = 0;
	mtk_jobs_q->capacity = JOBS_MAX;
	INIT_LIST_HEAD(&mtk_jobs_q->list);

	/* init wait frame done */
	init_waitqueue_head(&cc->vsync_wait);

	/* init poll lock */
	mutex_init(&cc->poll_lock);
	spin_lock_init(&cc->spinlock_irq);

	/* init queue lock */
	mutex_init(&cc->queue_lock);

	/* get pipe instances */
	for (i = 0; i < CU_PATH_MAX; i++) {

		ret = mtk_cinematic_ctrl_get_device(
			dev, "mediatek,cinematic_path", i, &worker);
		if (unlikely(ret))
			return ret;

		cc->ct[i] = dev_get_drvdata(worker);
	}

	ret = mtk_cinematic_ctrl_get_device(dev, "mediatek,disp", 0,
					    &worker);
	if (unlikely(ret))
		return ret;

	cc->disp_dev = worker;

	of_property_read_u32_array(dev->of_node, "gce-events", cc->cmdq_events,
				   MTK_MM_CORE_EVENT_MAX);

	of_property_read_u32_array(dev->of_node, "gce-subsys", &cc->rbfc_subsys,
				   1);

	ret = mtk_cinematic_ctrl_get_device(dev, "mediatek,rbfc", 0,
					    &worker);
	if (unlikely(ret)) {
		dev_err(cc->dev, "get rbfc device NG!!\n");
		return ret;
	}
	cc->dev_rbfc = worker;

	ret = mtk_cinematic_ctrl_get_device(dev, "mediatek,sbrc", 0,
					    &worker);
	if (unlikely(ret)) {
		dev_err(cc->dev, "get sbrc device NG!!\n");
		return ret;
	}
	cc->dev_sbrc = worker;

	ret = mtk_cinematic_ctrl_get_device(dev, "mediatek,sysram", 0,
					    &worker);
	if (unlikely(ret)) {
		dev_err(cc->dev, "get sysram device NG!!\n");
		return ret;
	}
	cc->dev_sysram = worker;

	cc->sampling_rate = 0;
	cc->frmtrk_dist = SOF_GAP + 1000;
#if 0
	reg = ioremap_nocache(0x14018000, 0x1000);
#endif

	dsi_dev = mtk_dispsys_get_dsi(cc->disp_dev);
	mtk_dsi_get_lcm_vtotal(dsi_dev, &lcm_vtotal);
	cc->safe_line = lcm_vtotal - 5;
	dev_dbg(cc->dev, "get dsi vtotal: %d\tsafe line: %d\n", lcm_vtotal,
		cc->safe_line);

	reg_chardev(cc);
#ifdef CONFIG_MTK_DEBUGFS
	mtk_cinematic_debugfs_init(cc);
#endif
	platform_set_drvdata(pdev, cc);
	return 0;

#ifdef CACHED_BUF
free_resource_r:
	dma_unmap_single(cc->dev, rec_pa_r, sizeof(u32) * REC_NUM,
			 DMA_FROM_DEVICE);
free_resource_w:
	dma_unmap_single(cc->dev, rec_pa_w, sizeof(u32) * REC_NUM,
			 DMA_FROM_DEVICE);
free_resource_d:
	dma_unmap_single(cc->dev, rec_pa_d, sizeof(u32) * REC_NUM,
			 DMA_FROM_DEVICE);
free_resource:
	for (buf_num = 0; buf_num < i; buf_num++)
		dma_unmap_single(cc->dev, cc->sd.d_pa[buf_num], sizeof(u32),
				 DMA_FROM_DEVICE);
#else

#ifdef SBRC_FORCE_READ_CTRL
free_dma_alloc_f:
	dma_free_attrs(cc->dev, sizeof(u32), forced_buf_id_va, forced_buf_id_pa,
		       &dma_attrs);
#endif

free_dma_alloc_s:
	dma_free_attrs(cc->dev, sizeof(u32), skip_next_va, skip_next_pa,
		       &dma_attrs);
free_dma_alloc_dsi_in:
	dma_free_attrs(cc->dev, sizeof(u32), rec_va_dsi_in, rec_pa_dsi_in,
		       &dma_attrs);
free_dma_alloc_dnc:
	dma_free_attrs(cc->dev, sizeof(u32), rec_va_dsi_out, rec_pa_dsi_out,
		       &dma_attrs);
free_dma_alloc_w1:
	dma_free_attrs(cc->dev, sizeof(u32), rec_va_w1, rec_pa_w1,
		       &dma_attrs);
free_dma_alloc_dsi_out:
	dma_free_attrs(cc->dev, sizeof(u32), rec_va_dsi_out, rec_pa_dsi_out,
		       &dma_attrs);
free_dma_alloc_rdma:
	dma_free_attrs(cc->dev, sizeof(u32), rec_va_rdma_st, rec_pa_rdma_st,
		       &dma_attrs);
free_dma_alloc_r:
	dma_free_attrs(cc->dev, sizeof(u32) * REC_NUM, rec_va_r, rec_pa_r,
		       &dma_attrs);
free_dma_alloc_w:
	dma_free_attrs(cc->dev, sizeof(u32) * REC_NUM, rec_va_w, rec_pa_w,
		       &dma_attrs);
free_dma_alloc_d:
	dma_free_attrs(cc->dev, sizeof(u32) * REC_NUM, rec_va_d, rec_pa_d,
		       &dma_attrs);
free_dma_alloc:
	for (buf_num = 0; buf_num < i; buf_num++)
		dma_free_attrs(cc->dev, sizeof(u32), cc->sd.d_va[buf_num],
			       cc->sd.d_pa[buf_num], &dma_attrs);
#endif
	return -ENOMEM;
}

static int mtk_cinematic_ctrl_remove(struct platform_device *pdev)
{
	struct mtk_cinematic_ctrl *cc = platform_get_drvdata(pdev);
#ifndef CACHED_BUF
	struct dma_attrs dma_attrs;
#endif
	int i;

#ifdef CONFIG_MTK_DEBUGFS
	mtk_cinematic_debugfs_deinit(cc);
#endif

	/* release dma */
#ifdef CACHED_BUF
	for (i = 0; i < DIFF_COUNT_MAX; i++)
		dma_unmap_single(cc->dev, cc->sd.d_pa[i], sizeof(u32),
				 DMA_FROM_DEVICE);

	dma_unmap_single(cc->dev, rec_pa_d, sizeof(u32) * REC_NUM,
			 DMA_FROM_DEVICE);
	dma_unmap_single(cc->dev, rec_pa_r, sizeof(u32) * REC_NUM,
			 DMA_FROM_DEVICE);
	dma_unmap_single(cc->dev, rec_pa_w, sizeof(u32) * REC_NUM,
			 DMA_FROM_DEVICE);
#else
	init_dma_attrs(&dma_attrs);
	dma_set_attr(DMA_ATTR_WRITE_COMBINE | DMA_ATTR_FORCE_CONTIGUOUS,
		     &dma_attrs);

	for (i = 0; i < DIFF_COUNT_MAX; i++)
		dma_free_attrs(cc->dev, sizeof(u32), cc->sd.d_va[i],
			       cc->sd.d_pa[i], &dma_attrs);

	dma_free_attrs(cc->dev, sizeof(u32) * REC_NUM, rec_va_d, rec_pa_d,
		       &dma_attrs);
	dma_free_attrs(cc->dev, sizeof(u32) * REC_NUM, rec_va_w, rec_pa_w,
		       &dma_attrs);
	dma_free_attrs(cc->dev, sizeof(u32) * REC_NUM, rec_va_r, rec_pa_r,
		       &dma_attrs);
	dma_free_attrs(cc->dev, sizeof(u32), skip_next_va, skip_next_pa,
		       &dma_attrs);
	dma_free_attrs(cc->dev, sizeof(u32), rec_va_dsi_in, rec_pa_dsi_in,
		       &dma_attrs);
	dma_free_attrs(cc->dev, sizeof(u32), rec_va_dsi_out, rec_pa_dsi_out,
		       &dma_attrs);
	dma_free_attrs(cc->dev, sizeof(u32), rec_va_dnc, rec_pa_dnc,
		       &dma_attrs);
	dma_free_attrs(cc->dev, sizeof(u32), rec_va_w1, rec_pa_w1,
		       &dma_attrs);
	dma_free_attrs(cc->dev, sizeof(u32), rec_va_rdma_st, rec_pa_rdma_st,
		       &dma_attrs);

	#ifdef SBRC_FORCE_READ_CTRL
	dma_free_attrs(cc->dev, sizeof(u32), forced_buf_id_va, forced_buf_id_pa,
	       &dma_attrs);
	#endif

#endif
	/* end of release monitor sbrc dma */

	kfree(read_buffer);
	device_destroy(cc->dev_class, cc->devt);
	class_destroy(cc->dev_class);
	unregister_chrdev_region(cc->devt, 1);
	dev_dbg(cc->dev, "cinematic mode unloaded\n");
	return 0;
}

static const struct of_device_id cinematic_ctrl_driver_dt_match[] = {
	{ .compatible = "mediatek,mt3612-cinematic-ctrl" },
	{},
};
MODULE_DEVICE_TABLE(of, cinematic_ctrl_driver_dt_match);

struct platform_driver mtk_cinematic_ctrl_driver = {
	.probe		= mtk_cinematic_ctrl_probe,
	.remove		= mtk_cinematic_ctrl_remove,
	.driver		= {
		.name	= "mediatek-cinematic-ctrl",
		.owner	= THIS_MODULE,
		.of_match_table = cinematic_ctrl_driver_dt_match,
	},
};

module_platform_driver(mtk_cinematic_ctrl_driver);
MODULE_LICENSE("GPL");

