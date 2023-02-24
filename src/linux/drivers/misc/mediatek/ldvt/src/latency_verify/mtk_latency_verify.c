/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	Alpha.Liang <alpha.liang@mediatek.com>
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

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/compiler.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/soc/mediatek/mtk-cmdq.h>
#include <linux/uaccess.h>
#include <dt-bindings/gce/mt3612-gce.h>
#include <soc/mediatek/mtk_mutex.h>
#include <linux/completion.h>
#include <linux/kthread.h>
#include <soc/mediatek/mtk_dsi.h>
#include <soc/mediatek/mtk_slicer.h>
#include <soc/mediatek/mtk_rdma.h>
#include <soc/mediatek/mtk_wdma.h>
#include <soc/mediatek/mtk_mutex.h>
#include <mtk_disp.h>
#include "../display_pipe/mtk_disp_pipe.h"
#include <mtk_fm_dev.h>


enum mtk_latency_dev_cmdq_event {
	MTK_LATENCY_CMDQ_EVENT_MMSYS_CORE_SOF_FOR_1_SLICER = 0,
	MTK_LATENCY_CMDQ_EVENT_MMSYS_CORE_SOF_FOR_33_DSI0,
	MTK_LATENCY_CMDQ_EVENT_IMG_SIDE0_CAM0_P2_SOF,
	MTK_LATENCY_CMDQ_EVENT_CAM_SIDE0_CAM0_BE_FRAME_DONE,
	MTK_LATENCY_DEV_CMDQ_EVENT_MAX,
};

enum mtk_latency_cmdq_channel {
	PIPE0_CH = 0,
	PIPE1_CH,
	PIPE2_CH,
	PIPE3_CH,
	PIPE4_CH,
	PIPE5_CH,
	PIPE6_CH,
	PIPE7_CH,
	PIPE8_CH,
	PIPE9_CH,
	PIPE10_CH,
	CH_MAX_VALUE
};

enum mtk_latency_cv_1st_result {
	ISP_P2_SOF = 0,
	WPE_SOF,
	FE_DONE,
	WDMA0_DONE,
	WDMA1_DONE,
	WDMA2_DONE,
	ISP_P1_SOF,
	BE_DONE,
	FRM_TRK,
	DSI_SOF,
	SLICER_EVENT,
	DSI_VACTL_EVENT
};

enum mtk_latency_cv_2nd_result {
	WARPA_DONE_INT = 0,
	FE_DONE_INT,
	FMS_DONE_INT,
	FET_1_16_DONE_INT,
	FET_1_4_DONE_INT,
	FET_1_1_DONE_INT,
	FMT_1_16_DONE_INT,
	FMT_1_4_DONE_INT,
	FMT_1_1_DONE_INT,
	FMT_1_16_DELAY,
	FMT_1_4_DELAY,
	FMT_1_1_DELAY,
	SC_1_16_DONE_INT,
	SC_1_4_DONE_INT,
	SC_1_1_DONE_INT,
	FMS_DELAY,
	FET_DELAY,
	SC_1_16_DELAY,
	SC_1_4_DELAY,
	SC_1_1_DELAY,
	FM_VPU_DONE_INT,
	POSE_DONE_INT,
	POSE_DELAY
};

enum dsi_task_sync {
	DSI_INITIAL = 0,
	SLCR_RUN,
	DSI_RECORD,
	SLCR_SLEEP,
	ISP_P2_SOF_RUN,
	BE_DONE_RUN,
};

enum event_task_sync {
	EVENT_STOP = 0,
	EVENT_RUN,
	EVENT_RECEIVE
};

struct mtk_latency_verify {
	struct device		*dev;
	struct dentry		*debugfs;
	u32 cmdq_events[MTK_LATENCY_DEV_CMDQ_EVENT_MAX];
	struct cmdq_client *cmdq_client[CH_MAX_VALUE];
	struct device *dev_mutex;
	struct device *dev_fm_dev;
	struct mtk_fm_dev *fm_dev;
	struct device *dsi_dev;
	struct device *slcr_dev;
	struct device *wdma_dev;
	struct device *rdma_dev;
	struct device *pvric_rdma_dev;
	struct device *dev_disp_dev;
	struct mtk_dispsys *disp_dev;
	struct mutex lock;
};

#define GCE_WPE_toekn_sof_ts	GCE_CPR(113)
#define GCE_FE_token_done_ts	GCE_CPR(114)
#define GCE_WDMA0_token_done_ts	GCE_CPR(115)
#define GCE_WDMA1_token_done_ts	GCE_CPR(116)
#define GCE_WDMA2_token_done_ts	GCE_CPR(117)

#define TEST_TIMES 25
#define MAX_TEST_TIMES 50 /*For DSI SOF*/
#define SOF_TIMEOUT 160000000
#define GCE_EVENT_NUM 12
#define CV_INFO_NUM 25

static struct task_struct *latency_thread[GCE_EVENT_NUM];
u32 task_result[GCE_EVENT_NUM][MAX_TEST_TIMES];
u32 cv_result[CV_INFO_NUM][TEST_TIMES];
u32 Test_Number[2];
u32 gce_event[GCE_EVENT_NUM];
u8 gce_event_receive[GCE_EVENT_NUM];
u32 need_thread_num;
bool bVRtrackingTest;
bool bFrmtrk;
u32 mutex_src;
bool bUse3C;
int sync_flag, sync_flag_2, sync_flag_3;
u32 dsi_counter;
static u32 cv_frame_num;
static u32 dsi_frame_num[3];
static u8 dsi_frame_num_idx;
static int counter_reset;
int cv_all;
int for_initial_frame;

static u32 check_time_diff(u32 *ptr1, u32 *ptr2, bool bDSIcase)
{
	u32 val = 0;

	if (bDSIcase) {
		if ((*ptr1 - *ptr2) > 8000) {
			/*For DSI SOF ~ ISP SOF*/
			int i;
			for (i = 0 ; i < MAX_TEST_TIMES; i++) {
				u32 r1;

				r1 = *ptr1 - *(ptr2+i);
				if (r1 < 8000) {
					val = r1;
					break;
				}
			}
		} else {
			u32 r1;
			r1 = (*ptr1 - *(ptr2));
			if (r1 < 8000)
				val = r1;
		}

		return val;
	} else {
		if (*ptr1 > *ptr2) {
			if ((*ptr1 - *ptr2) < 8000) {
				pr_err("	%8u		%8u		%8u\n",
						(*ptr1 - *ptr2),
						*(ptr1+1) - *(ptr2+1),
						*(ptr1+2) - *(ptr2+2));
			}
		} else if (*ptr1  < *ptr2) {
			if (*(ptr1+1) - *ptr2 < 8000) {
				pr_err("	%8u		%8u		%8u\n",
						*(ptr1+1) - *ptr2,
						*(ptr1+2) - *(ptr2+1),
						*(ptr1+3) - *(ptr2+2));
			}
		}

	}

	return 0;
}

void show_3_data(u32 *ptr)
{
	pr_err("	%8u		%8u		%8u\n", *ptr,
		*(ptr + 1), *(ptr + 2));
}

static void list_latency_time(int is_initial_frame)
{
	int i = 0;
	u32 data1, data2, data3;

	if (is_initial_frame) {
		dsi_frame_num[0] = 0;
		pr_err("DSI frame number:\n");
		pr_err("	%8u		%8u		%8u\n",
				dsi_frame_num[0],
				dsi_frame_num[0] + 1,
				dsi_frame_num[0] + 2);

		pr_err("the latency from DP SOF to DSI SOF(frametrack):");
		for (i = 0; i < TEST_TIMES; i++) {
			if (task_result[FRM_TRK][i] != 0) {
				show_3_data(&task_result[FRM_TRK][i]);
				break;
			}
		}

		pr_err("the latency from DSI SOF to ISP SOF:\n");
		data1 = check_time_diff(&task_result[ISP_P2_SOF][0],
					&task_result[DSI_SOF][0], true);
		data2 = check_time_diff(&task_result[ISP_P2_SOF][1],
					&task_result[DSI_SOF][1], true);
		data3 = check_time_diff(&task_result[ISP_P2_SOF][2],
					&task_result[DSI_SOF][2], true);
		pr_err("	%8u		%8u		%8u\n", data1,
				data2, data3);
	} else {
		pr_err("DSI frame number:");
		pr_err("	%8u		%8u		%8u\n",
				dsi_frame_num[dsi_frame_num_idx],
				dsi_frame_num[dsi_frame_num_idx] + 1,
				dsi_frame_num[dsi_frame_num_idx] + 2);

		pr_err("the latency from DP SOF to DSI SOF(frametrack):");
		for (i = 2; i < TEST_TIMES; i++) {
			if (task_result[FRM_TRK][i] != 0) {
				show_3_data(&task_result[FRM_TRK][i]);
				break;
			}
		}

		pr_err("the latency from DSI SOF to ISP SOF:\n");
		data1 = check_time_diff(&task_result[ISP_P2_SOF][2],
					&task_result[DSI_SOF][2], true);
		data2 = check_time_diff(&task_result[ISP_P2_SOF][3],
					&task_result[DSI_SOF][3], true);
		data3 = check_time_diff(&task_result[ISP_P2_SOF][4],
					&task_result[DSI_SOF][4], true);
		pr_err("	%8u		%8u		%8u\n", data1,
				data2, data3);
	}

	dsi_frame_num_idx++;

	if (cv_all) {
		pr_err("the latency from Slicer 1st target line to DSI 1st target line:");
		data1 = check_time_diff(&task_result[DSI_VACTL_EVENT][0],
					&task_result[SLICER_EVENT][0], true);
		data2 = check_time_diff(&task_result[DSI_VACTL_EVENT][1],
					&task_result[SLICER_EVENT][1], true);
		data3 = check_time_diff(&task_result[DSI_VACTL_EVENT][2],
					&task_result[SLICER_EVENT][2], true);
		pr_err("	%8u		%8u		%8u\n", data1,
				data2, data3);
	}

	if (is_initial_frame) {
		i = 1;

		pr_err("CV frame number:\n");
		cv_frame_num = 0;
		pr_err("	%8u		%8u		%8u\n",
				cv_frame_num, cv_frame_num + 1,
				cv_frame_num + 2);

		pr_err("List the latency from ISP SOF to Warpa SOF:\n");
		check_time_diff(&task_result[WPE_SOF][0],
						&task_result[ISP_P2_SOF][0], 0);
		pr_err("List the latency from Warpa SOF to FE Frame Done:\n");
		show_3_data(&task_result[FE_DONE][0]);

		pr_err("List the latency from Warpa SOF to WDMA0 Frame Done:\n");
		show_3_data(&task_result[WDMA0_DONE][0]);

		if (cv_all) {
			pr_err("List the latency from Warpa SOF to WDMA1 Frame Done:\n");
			show_3_data(&task_result[WDMA1_DONE][0]);

			pr_err("List the latency from Warpa SOF to WDMA2 Frame Done:\n");
			show_3_data(&task_result[WDMA2_DONE][0]);

			pr_err("List the latency from ISP pass 1 SOF to BE Frame Done:\n");
			check_time_diff(&task_result[BE_DONE][0],
				&task_result[ISP_P1_SOF][0], 0);
		}
	} else {
		i = 3;

		pr_err("CV frame number:");
		pr_err("	%8u		%8u		%8u\n",
			cv_frame_num, cv_frame_num+1,
			cv_frame_num+2);

		pr_err("List the latency from ISP SOF to Warpa SOF:\n");
		check_time_diff(&task_result[WPE_SOF][2],
						&task_result[ISP_P2_SOF][2], 0);

		pr_err("List the latency from Warpa SOF to FE Frame Done:\n");
		show_3_data(&task_result[FE_DONE][2]);

		pr_err("List the latency from Warpa SOF to WDMA0 Frame Done:\n");
		show_3_data(&task_result[WDMA0_DONE][2]);

		if (cv_all) {
			pr_err("List the latency from Warpa SOF to WDMA1 Frame Done:\n");
			show_3_data(&task_result[WDMA1_DONE][2]);

			pr_err("List the latency from Warpa SOF to WDMA2 Frame Done:\n");
			show_3_data(&task_result[WDMA2_DONE][2]);

			pr_err("List the latency from ISP pass 1 SOF to BE Frame Done:\n");
			check_time_diff(&task_result[BE_DONE][2],
				&task_result[ISP_P1_SOF][2], 0);
		}
	}

	pr_err("List more CV information:\n");
	pr_err("WARPA_DONE_INT:\n");
	show_3_data(&cv_result[WARPA_DONE_INT][i]);

	pr_err("FE_DONE_INT:\n");
	show_3_data(&cv_result[FE_DONE_INT][i]);

	pr_err("FMS_DONE_INT:\n");
	show_3_data(&cv_result[FMS_DONE_INT][i]);

	pr_err("FMS_DELAY:\n");
	show_3_data(&cv_result[FMS_DELAY][i]);

	pr_err("FET_1_1_DONE_INT:\n");
	show_3_data(&cv_result[FET_1_1_DONE_INT][i]);

	pr_err("FET_DELAY:\n");
	show_3_data(&cv_result[FET_DELAY][i]);

	if (cv_all) {
		pr_err("1/16FET_DONE_INT:\n");
		show_3_data(&cv_result[FET_1_16_DONE_INT][i]);

		pr_err("1/4FET_DONE_INT:\n");
		show_3_data(&cv_result[FET_1_4_DONE_INT][i]);

		pr_err("1/16FMT_DONE_INT:\n");
		show_3_data(&cv_result[FMT_1_16_DONE_INT][i]);

		pr_err("1/4FMT_DONE_INT:\n");
		show_3_data(&cv_result[FMT_1_4_DONE_INT][i]);

		pr_err("1/16FMT_DELAY:\n");
		show_3_data(&cv_result[FMT_1_16_DELAY][i]);

		pr_err("1/4FMT_DELAY:\n");
			show_3_data(&cv_result[FMT_1_4_DELAY][i]);
	}

	pr_err("1/1FMT_DONE_INT:\n");
	show_3_data(&cv_result[FMT_1_1_DONE_INT][i]);

	pr_err("1/1FMT_DELAY:\n");
	show_3_data(&cv_result[FMT_1_1_DELAY][i]);

	pr_err("POSE_DONE_INT:\n");
	show_3_data(&cv_result[POSE_DONE_INT][i]);

	pr_err("POSE_DELAY:\n");
	show_3_data(&cv_result[POSE_DELAY][i]);

	pr_err("FM_VPU_DONE_INT:\n");
	show_3_data(&cv_result[FM_VPU_DONE_INT][i]);

	if (cv_all) {
		pr_err("1/16SC_DONE_INT:\n");
		show_3_data(&cv_result[SC_1_16_DONE_INT][i]);

		pr_err("1/16SC_DELAY:\n");
		show_3_data(&cv_result[SC_1_16_DELAY][i]);

		pr_err("1/4SC_DONE_INT:\n");
		show_3_data(&cv_result[SC_1_4_DONE_INT][i]);

		pr_err("1/4SC_DELAY:\n");
		show_3_data(&cv_result[SC_1_4_DELAY][i]);
	}

	pr_err("1/1SC_DONE_INT:\n");
	show_3_data(&cv_result[SC_1_1_DONE_INT][i]);

	pr_err("1/1SC_DELAY:\n");
	show_3_data(&cv_result[SC_1_1_DELAY][i]);
}


static int mtk_ts_cmd_gce_get_vts(struct mtk_latency_verify *latency_dev,
				struct cmdq_pkt *cmdq_pkt, dma_addr_t pa)
{
	u8 subsys = SUBSYS_102DXXXX;
	/* subsys could be SUBSYS_102DXXXX, GCE4_SUBSYS_102DXXXX or
	 * GCE5_SUBSYS_102DXXXX based on your GCE core
	 */

	mutex_lock(&latency_dev->lock);

	/* GCE_REG[GCE_SPR1] = 0x102d3020(us) */
	cmdq_pkt_read(cmdq_pkt, subsys, 0x3020, GCE_SPR1);
	/* GCE_REG[GCE_SPR0] = pa */
	cmdq_pkt_assign_command(cmdq_pkt, GCE_SPR0, pa);
	/* *GCE_REG[GCE_SPR0] = GCE_REG[GCE_SPR1] */
	cmdq_pkt_store_reg(cmdq_pkt, GCE_SPR0, GCE_SPR1, 0xffffffff);

	mutex_unlock(&latency_dev->lock);

	return 0;
}

static int mtk_check_sof_diff(struct mtk_dispsys *dispsys,
					int counter, u32 src, u32 ref)
{
	struct mtk_mutex_res *mutex;
	struct cmdq_pkt *cmdq_pkt;
	struct mtk_mutex_timer_status time;
	u32 td_sof_id;
	u64 diff_time;
	int ret = 0;

	if (IS_ERR(dispsys))
		return -EINVAL;

	#if 1
	mutex = dispsys->mutex[0];
	if (IS_ERR(mutex)) {
		pr_info("Get sof diff mutex NG!!\n");
		return -EIO;
	}
	#else
	mutex = mtk_mutex_get(ct->dev_mutex, SOF_DIFF);
	if (IS_ERR(mutex)) {
		dev_err(cc->dev, "Get sof diff mutex NG!!\n");
		return -EIO;
	}
	#endif

	td_sof_id = CMDQ_EVENT_MMSYS_CORE_TD_EVENT_8;

	cmdq_pkt_create(&cmdq_pkt);
	cmdq_pkt_clear_event(cmdq_pkt, td_sof_id);
	cmdq_pkt_wfe(cmdq_pkt, td_sof_id);

	mtk_mutex_select_timer_sof(mutex, src, ref, NULL);
	mtk_mutex_select_timer_sof_timing(mutex, true, true, NULL);
	mtk_mutex_set_timer_us(mutex, SOF_TIMEOUT, SOF_TIMEOUT, NULL);
	mtk_mutex_timer_enable_ex(mutex, true, MUTEX_TO_EVENT_REF_EDGE, NULL);

	if (cmdq_pkt_flush(dispsys->client[0], cmdq_pkt) == 0) {
		ret = mtk_mutex_timer_get_status(mutex, &time);
		if (ret) {
			dev_err(dispsys->dev, "Get sof diff status NG!!\n");
			goto RELEASE;
		} else {
			/* to pico seconds than to nano to prevent lost*/
			diff_time = ((u64)((u64)((1000000/320) *
				     (u64)time.ref_time)) / 1000000);

			task_result[8][counter] = (u32)diff_time;
			pr_info("frmtrk time = %lld (us)\n",
				diff_time);
		}
	}

RELEASE:
	cmdq_pkt_destroy(cmdq_pkt);
	mtk_mutex_timer_disable(mutex, NULL);

	return 0;
}

static int setFrameCount(struct mtk_latency_verify *latency_dev, int idx)
{
	cv_frame_num =
		*latency_dev->fm_dev->fm_vpu_cnt_va;

	dsi_frame_num[idx] = dsi_counter;
	pr_info("dsi_frame_num[%d] = %d\n", idx,
		dsi_frame_num[idx]);
	idx++;
	return idx;
}

static void printFrameNum(struct mtk_latency_verify *latency_dev)
{
	pr_err("dsi frame = %u	cv frame = %u\n",
		dsi_counter,
		*latency_dev->fm_dev->fm_vpu_cnt_va);
}

/* note that hard code to use SPR0 to assign pa */
static void append_exec_time_gce_cmd(struct cmdq_pkt *handle, u16 sof_idx,
	u16 done_idx, u16 exec_idx, dma_addr_t pa)
{
	struct cmdq_operand left_operand, right_operand;

	/* exec_idx = done_idx - sof_idx */
	cmdq_pkt_logic_command(handle, CMDQ_LOGIC_SUBTRACT, exec_idx,
		cmdq_operand_reg(&left_operand, done_idx),
		cmdq_operand_reg(&right_operand, sof_idx));
	/* pa = exec_idx */
	cmdq_pkt_assign_command(handle, GCE_SPR0, pa);
	cmdq_pkt_store_reg(handle, GCE_SPR0, exec_idx, 0xffffffff);
}

static u32 *init_buffer(struct device *dev, dma_addr_t *pa)
{
	struct dma_attrs dma_attrs;
	u32 *va;

	init_dma_attrs(&dma_attrs);
	dma_set_attr(DMA_ATTR_WRITE_COMBINE | DMA_ATTR_FORCE_CONTIGUOUS,
			&dma_attrs);

	va = (u32 *)dma_alloc_attrs(dev, sizeof(u32), pa, GFP_KERNEL,
				    &dma_attrs);
	*va = 0;

	return va;
}

static void deinit_buffer(struct device *dev, dma_addr_t pa, u32 *va)
{
	struct dma_attrs dma_attrs;

	init_dma_attrs(&dma_attrs);
	dma_set_attr(DMA_ATTR_WRITE_COMBINE | DMA_ATTR_FORCE_CONTIGUOUS,
		&dma_attrs);

	dma_free_attrs(dev, sizeof(u32), va, pa, &dma_attrs);
}

void thread0_top_2nd_func(struct mtk_latency_verify *latency_dev, u32 *va,
					dma_addr_t pa, u32 gce_id, int *counter,
					int *idx)
{
	printFrameNum(latency_dev);
	pr_info("Updated value: %u (0x%llx) for GCE event: %d\n",
		*va, pa, gce_id);

	if (*counter == 2)
		*idx = setFrameCount(latency_dev, *idx);
	task_result[ISP_P2_SOF][*counter] = *va;
	(*counter) = (*counter) + 1;
	if (*counter == Test_Number[0]) {
		gce_event_receive[ISP_P2_SOF] = EVENT_RUN;
		*counter = 0;
	}
}

void thread0_top_1st_func(struct mtk_latency_verify *latency_dev,
				u32 *va, dma_addr_t pa, u32 gce_id,
				int *counter, int *idx)
{
	if (gce_event_receive[0] == EVENT_RECEIVE) {
		if (for_initial_frame) {
			thread0_top_2nd_func(latency_dev, va, pa,
				gce_id, counter, idx);
		} else {
			if (*counter == 0) {
				sync_flag_2 = ISP_P2_SOF_RUN;
				/*pr_err("sync_flag_2 is ISP_P2_SOF_RUN\n");*/
			}

			if (sync_flag_2 == ISP_P2_SOF_RUN &&
				sync_flag_3 == BE_DONE_RUN) {
				thread0_top_2nd_func(latency_dev, va, pa,
					gce_id, counter, idx);
			}
		}
	}
}


void thread0_bottom_func(struct mtk_latency_verify *latency_dev, u32 *va,
					dma_addr_t pa, u32 gce_id, int *counter)
{
	pr_info("Updated value: %u (0x%llx) for GCE event: %d\n",
		*va, pa, gce_id);
	task_result[0][*counter] = *va;
	(*counter)++;
	if (*counter == Test_Number[0]) {
		gce_event_receive[0] = 0;
		*counter = 0;
	}

}

static int thread0_task(void *data)
{
	struct mtk_latency_verify *latency_dev;
	struct cmdq_pkt *cmdq_pkt;
	u32 gce_id;
	struct dma_attrs dma_attrs;
	u32 *va;
	dma_addr_t pa;
	int counter = 0;
	int idx = 0;

	latency_dev = (struct mtk_latency_verify *) data;
	gce_id = gce_event[0];

	pr_err("try to receive GCE Event %d\n", gce_id);

	init_dma_attrs(&dma_attrs);
	dma_set_attr(DMA_ATTR_WRITE_COMBINE | DMA_ATTR_FORCE_CONTIGUOUS,
		&dma_attrs);
	va = (u32 *)dma_alloc_attrs(latency_dev->dev, sizeof(u32),
		&pa, GFP_KERNEL, &dma_attrs);
	*va = 0;
	dev_err(latency_dev->dev, "Init value: %d (0x%llx) for GCE event: %d\n",
			*va, pa, gce_id);

	cmdq_pkt_create(&cmdq_pkt);
	cmdq_pkt_clear_event(cmdq_pkt, gce_id);
	cmdq_pkt_wfe(cmdq_pkt, gce_id);

	mtk_ts_cmd_gce_get_vts(latency_dev, cmdq_pkt, pa);

	if (!bVRtrackingTest) {
		/*for vr_mode_tc_p1 and vr_mode_tc_p2*/
		while (!kthread_should_stop()) {
			if (gce_event_receive[0]) {
				if (cmdq_pkt_flush(
					latency_dev->cmdq_client[PIPE8_CH],
					cmdq_pkt) != 0) {
				/*pr_info(\"=== get event timeout! ===\n");*/
				} else {
					pr_info("Updated value: %u (0x%llx) for GCE event: %d\n",
					*va, pa, gce_id);
					task_result[0][counter] = *va;
					counter++;
					if (counter == Test_Number[0]) {
						gce_event_receive[0] = 0;
						counter = 0;
					}
				}
			}
		}
	} else {/*for the latency from isp sof to warpa sof*/
		while (!kthread_should_stop()) {
			if (gce_event_receive[0]) {
				if (cmdq_pkt_flush(
					latency_dev->cmdq_client[PIPE0_CH],
					cmdq_pkt) != 0) {
				/*pr_info("=== get event timeout! ===\n");*/
				} else {
					if (need_thread_num >= 4) {
					thread0_top_1st_func(latency_dev,
						va, pa, gce_id,
						&counter, &idx);
					} else {
						thread0_bottom_func(latency_dev,
							va, pa, gce_id,
							&counter);
					}
				}
			} else {
				usleep_range(1000, 2000);
			}
		}
	}

	cmdq_pkt_destroy(cmdq_pkt);
	dma_free_attrs(latency_dev->dev, sizeof(u32),
		va, pa, &dma_attrs);
	dev_err(latency_dev->dev, "=== sof thread 0 dies ===\n");
	return 0;
}

static int thread1_task(void *data)
{
	struct mtk_latency_verify *latency_dev;
	struct cmdq_pkt *cmdq_pkt;
	u32 gce_id;
	struct dma_attrs dma_attrs;
	u32 *va;
	dma_addr_t pa;
	int counter = 0;

	latency_dev = (struct mtk_latency_verify *) data;
	gce_id = gce_event[1];

	pr_err("try to receive GCE Event %d\n", gce_id);

	init_dma_attrs(&dma_attrs);
	dma_set_attr(DMA_ATTR_WRITE_COMBINE | DMA_ATTR_FORCE_CONTIGUOUS,
					&dma_attrs);
	va = (u32 *)dma_alloc_attrs(latency_dev->dev,
			sizeof(u32), &pa, GFP_KERNEL, &dma_attrs);
	*va = 0;
	dev_err(latency_dev->dev, "Init value: %d (0x%llx) for GCE event: %d\n",
			*va, pa, gce_id);

	cmdq_pkt_create(&cmdq_pkt);
	cmdq_pkt_clear_event(cmdq_pkt, gce_id);
	cmdq_pkt_wfe(cmdq_pkt, gce_id);

	mtk_ts_cmd_gce_get_vts(latency_dev, cmdq_pkt, pa);

	if (!bVRtrackingTest) {
		/*for vr_mode_tc_p1 and vr_mode_tc_p2*/
		while (!kthread_should_stop()) {
			if (gce_event_receive[1]) {
				if (cmdq_pkt_flush(
					latency_dev->cmdq_client[PIPE9_CH],
					cmdq_pkt) != 0) {
				/*pr_info("=== get event timeout! ===\n");*/
				} else {
					pr_info("Updated value: %u (0x%llx) for GCE event: %d\n",
					*va, pa, gce_id);
					task_result[1][counter] = *va;
					counter++;
					if (counter == Test_Number[1]) {
						gce_event_receive[1] = 0;
						counter = 0;
					}
				}
			}
		}
	} else {
		/*or the latency from isp sof to warpa sof*/
		while (!kthread_should_stop()) {
			if (gce_event_receive[1]) {
				if (cmdq_pkt_flush(
				latency_dev->cmdq_client[PIPE1_CH],
				cmdq_pkt) != 0) {
				/*pf_info("=== get event timeout! ===\n");*/
				} else {
					pr_info("Updated value: %u (0x%llx) for GCE event: %d\n",
						*va, pa, gce_id);
					task_result[1][counter] = *va;
					counter++;
					if (counter == Test_Number[1]) {
						gce_event_receive[1] = 0;
						counter = 0;
					}
				}
			} else {
				usleep_range(2000, 5000);
			}
		}
	}

	cmdq_pkt_destroy(cmdq_pkt);
	dma_free_attrs(latency_dev->dev, sizeof(u32),
			va, pa, &dma_attrs);
	dev_err(latency_dev->dev, "=== sof thread 1 dies ===\n");
	return 0;
}

u32 *wpe_token_sof_va, *fe_done_va;
u32 *wdma0_done_va, *wdma1_done_va;
u32 *wdma2_done_va;
dma_addr_t wpe_token_sof_pa, fe_done_pa;
dma_addr_t wdma0_done_pa, wdma1_done_pa;
dma_addr_t wdma2_done_pa;

void thread2_func(struct mtk_latency_verify *latency_dev,
					struct mtk_fm_dev *fm_dev, int gce_id,
					int *counter, bool *bContinue)
{
	pr_info("Updated value: %u (0x%llx) for GCE event: %d\n",
				*wpe_token_sof_va,
				wpe_token_sof_pa,
				gce_id);
	pr_info("Updated value: %u (0x%llx) for WPE SOF to FE Done\n",
			*fe_done_va, fe_done_pa);
	pr_info("Updated value: %u (0x%llx) for WPE SOF to WDMA0 Done\n",
			*wdma0_done_va, wdma0_done_pa);

	if (need_thread_num >= 8) {
		pr_info("Updated value: %u (0x%llx) for WPE SOF to WDMA1 Done\n",
				*wdma1_done_va, wdma1_done_pa);
		pr_info("Updated value: %u (0x%llx) for WPE SOF to WDMA2 Done\n",
				*wdma2_done_va, wdma2_done_pa);
	}

	if (need_thread_num >= 4) {
		cv_result[WARPA_DONE_INT][*counter] =
			*fm_dev->wpe_et_va;
		cv_result[FE_DONE_INT][*counter] =
			*fm_dev->fe_et_va;
		cv_result[FMS_DONE_INT][*counter] =
			*fm_dev->fms_et_va;
		cv_result[FET_1_1_DONE_INT][*counter] =
			*fm_dev->feT_et_va;
		cv_result[FMT_1_1_DONE_INT][*counter] =
			*fm_dev->fmt_et_va;
		cv_result[POSE_DONE_INT][*counter] =
			*fm_dev->head_pose_et_va;
		cv_result[FM_VPU_DONE_INT][*counter] =
			*fm_dev->fm_vpu_et_va;
		cv_result[SC_1_1_DONE_INT][*counter] =
			*fm_dev->sc_et_va;

		cv_result[FMS_DELAY][*counter] =
			*fm_dev->fms_delay_va;
		cv_result[FET_DELAY][*counter] =
			*fm_dev->feT_delay_va;
		cv_result[SC_1_1_DELAY][*counter] =
			*fm_dev->sc_delay_va;
		cv_result[FMT_1_1_DELAY][*counter] =
			*fm_dev->fmt_delay_va;
		cv_result[POSE_DELAY][*counter] =
			*fm_dev->head_pose_delay_va;
	}

	if (need_thread_num >= 8) {
		cv_result[FET_1_16_DONE_INT][*counter] =
			*fm_dev->feT_1_16_et_va;
		cv_result[FET_1_4_DONE_INT][*counter] =
			*fm_dev->feT_1_4_et_va;
		cv_result[FMT_1_16_DONE_INT][*counter] =
			*fm_dev->fmt_1_16_et_va;
		cv_result[FMT_1_4_DONE_INT][*counter] =
			*fm_dev->fmt_1_4_et_va;
		cv_result[FMT_1_16_DELAY][*counter] =
			*fm_dev->fmt_1_16_delay_va;
		cv_result[FMT_1_4_DELAY][*counter] =
			*fm_dev->fmt_1_4_delay_va;
		cv_result[SC_1_16_DONE_INT][*counter] =
			*fm_dev->sc_1_16_et_va;
		cv_result[SC_1_4_DONE_INT][*counter] =
			*fm_dev->sc_1_4_et_va;
		cv_result[SC_1_16_DELAY][*counter] =
			*fm_dev->sc_1_16_delay_va;
		cv_result[SC_1_4_DELAY][*counter] =
			*fm_dev->sc_1_4_delay_va;
	}

	task_result[WPE_SOF][*counter] =
					*wpe_token_sof_va;
	task_result[FE_DONE][*counter] = *fe_done_va;
	task_result[WDMA0_DONE][*counter] = *wdma0_done_va;

	if (cv_all) {
		task_result[WDMA1_DONE][*counter] = *wdma1_done_va;
		task_result[WDMA2_DONE][*counter] = *wdma2_done_va;
	}

	(*counter)++;
	if (*counter == Test_Number[0]) {
		gce_event_receive[FE_DONE] = EVENT_RUN;
		*counter = 0;
		bContinue = false;
	}
}


static int thread2_task(void *data)
{
	struct mtk_latency_verify *latency_dev;
	struct cmdq_pkt *cmdq_pkt;

	int counter = 0;
	bool bContinue = true;
	u32 gce_id_1 = gce_event[WPE_SOF],
		gce_id_2 = gce_event[FE_DONE],
		gce_id_3 = gce_event[WDMA0_DONE],
		gce_id_4 = gce_event[WDMA1_DONE],
		gce_id_5 = gce_event[WDMA2_DONE];
	struct mtk_fm_dev *fm_dev;

	pr_err("try to receive GCE Event %d %d %d %d %d\n", gce_id_1, gce_id_2,
				gce_id_3, gce_id_4, gce_id_5);

	latency_dev = (struct mtk_latency_verify *) data;
	fm_dev = latency_dev->fm_dev;

	wpe_token_sof_va = init_buffer(latency_dev->dev, &wpe_token_sof_pa);
	fe_done_va  = init_buffer(latency_dev->dev, &fe_done_pa);
	wdma0_done_va = init_buffer(latency_dev->dev, &wdma0_done_pa);
	if (cv_all) {
		wdma1_done_va = init_buffer(latency_dev->dev, &wdma1_done_pa);
		wdma2_done_va = init_buffer(latency_dev->dev, &wdma2_done_pa);
	}

	dev_err(latency_dev->dev, "Init value: %u(0x%llx) for GCE event: %d %d\n",
			*fe_done_va, fe_done_pa, gce_id_1, gce_id_2);
	dev_err(latency_dev->dev, "Init value: %u(0x%llx) for GCE event: %d %d\n",
			*wdma0_done_va, wdma0_done_pa, gce_id_1, gce_id_3);
	if (cv_all) {
		dev_err(latency_dev->dev, "Init value: %u(0x%llx) for GCE event: %d %d\n",
			*wdma1_done_va, wdma1_done_pa, gce_id_1, gce_id_4);
		dev_err(latency_dev->dev, "Init value: %u(0x%llx) for GCE event: %d %d\n",
			*wdma2_done_va, wdma2_done_pa, gce_id_1, gce_id_5);
	}

	cmdq_pkt_create(&cmdq_pkt);

	cmdq_pkt_clear_event(cmdq_pkt, gce_id_1);
	cmdq_pkt_clear_event(cmdq_pkt, gce_id_2);
	cmdq_pkt_clear_event(cmdq_pkt, gce_id_3);
	if (cv_all) {
		cmdq_pkt_clear_event(cmdq_pkt, gce_id_4);
		cmdq_pkt_clear_event(cmdq_pkt, gce_id_5);
	}

	cmdq_pkt_wfe(cmdq_pkt, gce_id_1);
	cmdq_pkt_read(cmdq_pkt, SUBSYS_102DXXXX, 0x3020, GCE_WPE_toekn_sof_ts);
	cmdq_pkt_assign_command(cmdq_pkt, GCE_SPR0, wpe_token_sof_pa);
	cmdq_pkt_store_reg(cmdq_pkt, GCE_SPR0,
					GCE_WPE_toekn_sof_ts, 0xffffffff);

	cmdq_pkt_wfe(cmdq_pkt, gce_id_2);
	cmdq_pkt_read(cmdq_pkt, SUBSYS_102DXXXX, 0x3020, GCE_FE_token_done_ts);
	append_exec_time_gce_cmd(cmdq_pkt, GCE_WPE_toekn_sof_ts,
						GCE_FE_token_done_ts, GCE_SPR1,
						fe_done_pa);

	cmdq_pkt_wfe(cmdq_pkt, gce_id_3);
	cmdq_pkt_read(cmdq_pkt, SUBSYS_102DXXXX, 0x3020,
					GCE_WDMA0_token_done_ts);
	append_exec_time_gce_cmd(cmdq_pkt, GCE_WPE_toekn_sof_ts,
					GCE_WDMA0_token_done_ts, GCE_SPR2,
					wdma0_done_pa);

	if (cv_all) {
		cmdq_pkt_wfe(cmdq_pkt, gce_id_4);
		cmdq_pkt_read(cmdq_pkt, SUBSYS_102DXXXX, 0x3020,
					GCE_WDMA1_token_done_ts);
		append_exec_time_gce_cmd(cmdq_pkt, GCE_WPE_toekn_sof_ts,
					GCE_WDMA1_token_done_ts, GCE_SPR3,
					wdma1_done_pa);

		cmdq_pkt_wfe(cmdq_pkt, gce_id_5);
		cmdq_pkt_read(cmdq_pkt, SUBSYS_102DXXXX, 0x3020,
					GCE_WDMA2_token_done_ts);
		append_exec_time_gce_cmd(cmdq_pkt, GCE_WPE_toekn_sof_ts,
					GCE_WDMA2_token_done_ts, GCE_SPR1,
					wdma2_done_pa);
	}

	while (!kthread_should_stop()) {
		if (gce_event_receive[FE_DONE]) {
			if (cmdq_pkt_flush(latency_dev->cmdq_client[PIPE2_CH],
							cmdq_pkt) != 0) {
				/*pf_info("=== get event timeout! ===\n");*/
			} else {
				if (for_initial_frame == 0 &&
					sync_flag_2 == ISP_P2_SOF_RUN &&
					sync_flag_3 == BE_DONE_RUN)
					bContinue = true;

				if (gce_event_receive[FE_DONE] ==
					EVENT_RECEIVE && bContinue) {
					thread2_func(latency_dev, fm_dev,
						gce_id_1, &counter, &bContinue);
				}
			}
		} else {
			usleep_range(1000, 2000);
		}
	}

	cmdq_pkt_destroy(cmdq_pkt);

	deinit_buffer(latency_dev->dev, wpe_token_sof_pa,  wpe_token_sof_va);
	deinit_buffer(latency_dev->dev, fe_done_pa,  fe_done_va);
	deinit_buffer(latency_dev->dev, wdma0_done_pa, wdma0_done_va);
	if (cv_all) {
		deinit_buffer(latency_dev->dev, wdma1_done_pa, wdma1_done_va);
		deinit_buffer(latency_dev->dev, wdma2_done_pa, wdma2_done_va);
	}

	dev_err(latency_dev->dev, "=== sof thread 2 dies ===\n");

	return 0;
}



void thread_p1_func(struct mtk_latency_verify *latency_dev, u32 *va,
				dma_addr_t pa, u32 gce_id, int *counter)
{
	if (for_initial_frame) {
		pr_info("Updated value: %u (0x%llx) for GCE event: %d\n",
				*va, pa, gce_id);
		task_result[ISP_P1_SOF][*counter] = *va;
		(*counter)++;
		if (*counter == Test_Number[0]) {
			gce_event_receive[ISP_P1_SOF] =
						EVENT_RUN;
			*counter = 0;
		}
	} else {
		if (gce_event_receive[ISP_P1_SOF] == EVENT_RECEIVE &&
			sync_flag_2 == ISP_P2_SOF_RUN &&
			sync_flag_3 == BE_DONE_RUN) {
			pr_info("Updated value: %u (0x%llx) for GCE event: %d\n",
				*va, pa, gce_id);
			task_result[ISP_P1_SOF][*counter] = *va;
			(*counter)++;
			if (*counter == Test_Number[0]) {
				gce_event_receive[ISP_P1_SOF] =
							EVENT_RUN;
				*counter = 0;
			}
		}
	}
}

static int thread_p1_task(void *data)
{
	struct mtk_latency_verify *latency_dev;
	struct cmdq_pkt *cmdq_pkt;
	struct dma_attrs dma_attrs;
	u32 *va;
	dma_addr_t pa;
	int counter = 0;
	u32 gce_id = gce_event[ISP_P1_SOF];

	pr_err("try to receive GCE Event %d\n", gce_id);

	latency_dev = (struct mtk_latency_verify *) data;

	init_dma_attrs(&dma_attrs);
	dma_set_attr(DMA_ATTR_WRITE_COMBINE | DMA_ATTR_FORCE_CONTIGUOUS,
					&dma_attrs);
	va = (u32 *)dma_alloc_attrs(latency_dev->dev,
			sizeof(u32), &pa, GFP_KERNEL, &dma_attrs);
	*va = 0;
	dev_err(latency_dev->dev, "Init value: %u(0x%llx) for GCE event: %d\n",
			*va, pa, gce_id);

	cmdq_pkt_create(&cmdq_pkt);
	cmdq_pkt_clear_event(cmdq_pkt, gce_id);
	cmdq_pkt_wfe(cmdq_pkt, gce_id);

	mtk_ts_cmd_gce_get_vts(latency_dev, cmdq_pkt, pa);

	while (!kthread_should_stop()) {
		if (gce_event_receive[ISP_P1_SOF]) {
			if (cmdq_pkt_flush(latency_dev->cmdq_client[PIPE3_CH],
				cmdq_pkt) != 0) {
				/*pf_info("=== get event timeout! ===\n");*/
			} else {
				if (gce_event_receive[ISP_P1_SOF] ==
					EVENT_RECEIVE) {
					thread_p1_func(latency_dev, va, pa,
						gce_id, &counter);
				}
			}
		} else {
			usleep_range(1000, 2000);
		}
	}

	cmdq_pkt_destroy(cmdq_pkt);
	dma_free_attrs(latency_dev->dev, sizeof(u32),
					va, pa, &dma_attrs);
	dev_err(latency_dev->dev, "=== sof thread p1 dies ===\n");

	return 0;
}


void thread_be_func(struct mtk_latency_verify *latency_dev, u32 *va,
				dma_addr_t pa, u32 gce_id, int *counter)
{
	if (for_initial_frame) {
		pr_info("Updated value: %u (0x%llx) for GCE event: %d\n",
				*va, pa, gce_id);
		task_result[BE_DONE][*counter] = *va;
		(*counter)++;
		if (*counter == Test_Number[0]) {
			gce_event_receive[BE_DONE] = EVENT_RUN;
			*counter = 0;
		}
	} else {
		if (*counter == 0) {
			sync_flag_3 = BE_DONE_RUN;
			pr_info("sync_flag_3 is BE_DONE_RUN\n");
		}

		if (sync_flag_2 == ISP_P2_SOF_RUN &&
			sync_flag_3 == BE_DONE_RUN) {
			pr_info("Updated value: %u (0x%llx) for GCE event: %d\n",
					*va, pa, gce_id);
			task_result[BE_DONE][*counter] = *va;
			(*counter)++;
			if (*counter == Test_Number[0]) {
				gce_event_receive[BE_DONE] =
							EVENT_RUN;
				*counter = 0;
			}
		}
	}
}

static int thread_be_task(void *data)
{
	struct mtk_latency_verify *latency_dev;
	struct cmdq_pkt *cmdq_pkt;
	struct dma_attrs dma_attrs;
	u32 *va;
	dma_addr_t pa;
	int counter = 0;
	u32 gce_id = gce_event[BE_DONE];

	pr_err("try to receive GCE Event %d\n", gce_id);

	latency_dev = (struct mtk_latency_verify *) data;

	init_dma_attrs(&dma_attrs);
	dma_set_attr(DMA_ATTR_WRITE_COMBINE | DMA_ATTR_FORCE_CONTIGUOUS,
					&dma_attrs);
	va = (u32 *)dma_alloc_attrs(latency_dev->dev,
			sizeof(u32), &pa, GFP_KERNEL, &dma_attrs);
	*va = 0;
	dev_err(latency_dev->dev, "Init value: %u(0x%llx) for GCE event: %d\n",
			*va, pa, gce_id);

	cmdq_pkt_create(&cmdq_pkt);
	cmdq_pkt_clear_event(cmdq_pkt, gce_id);
	cmdq_pkt_wfe(cmdq_pkt, gce_id);

	mtk_ts_cmd_gce_get_vts(latency_dev, cmdq_pkt, pa);

	while (!kthread_should_stop()) {
		if (gce_event_receive[BE_DONE]) {
			if (cmdq_pkt_flush(latency_dev->cmdq_client[PIPE4_CH],
				cmdq_pkt) != 0) {
				/*pf_info("=== get event timeout! ===\n");*/
			} else {
				if (gce_event_receive[BE_DONE] ==
					EVENT_RECEIVE) {
					thread_be_func(latency_dev, va, pa,
							gce_id, &counter);
				}
			}
		} else {
			usleep_range(1000, 2000);
		}
	}

	cmdq_pkt_destroy(cmdq_pkt);
	dma_free_attrs(latency_dev->dev, sizeof(u32),
					va, pa, &dma_attrs);
	dev_err(latency_dev->dev, "=== sof thread be dies ===\n");

	return 0;
}

static int thread_frmtrk_task(void *data)
{
	struct mtk_latency_verify *latency_dev;

	int timeout = 10000;
	int counter = 0;
	latency_dev = (struct mtk_latency_verify *) data;

	while (!sync_flag && timeout--)
		usleep_range(5000, 10000);

	while (!kthread_should_stop()) {
		if (gce_event_receive[FRM_TRK]) {
			mtk_check_sof_diff(latency_dev->disp_dev,
				counter, mutex_src, MUTEX_MMSYS_SOF_DSI0);
			counter++;
			if (counter == Test_Number[0]) {
				gce_event_receive[FRM_TRK] = 0;
				counter = 0;
			}
		} else {
			usleep_range(1000, 2000);
		}
	}
	dev_dbg(latency_dev->dev, "=== sof thread frmtrk dies ===\n");

	return 0;
}


void thread_dsi_sof_func(struct mtk_latency_verify *latency_dev, u32 *va,
					dma_addr_t pa, u32 gce_id, int *counter)
{
	int test_time;

	pr_info("Updated value: %u (0x%llx) for GCE event: %d\n",
			*va, pa, gce_id);
	task_result[DSI_SOF][*counter] = *va;
	(*counter)++;

	if (cv_all)
		test_time = TEST_TIMES;
	else
		test_time = MAX_TEST_TIMES;

	if (*counter == test_time) {
		gce_event_receive[DSI_SOF] = EVENT_RUN;
		*counter = 0;
	}
}

static int thread_dsi_sof_task(void *data)
{
	struct mtk_latency_verify *latency_dev;
	struct cmdq_pkt *cmdq_pkt;
	struct dma_attrs dma_attrs;
	u32 *va;
	dma_addr_t pa;
	int dsi_fps = 0;
	int timeout = 10000;
	int counter = 0;
	u32 gce_id = gce_event[DSI_SOF];

	pr_err("try to receive GCE Event %d\n", gce_id);

	latency_dev = (struct mtk_latency_verify *) data;

	dsi_fps = mtk_dispsys_get_current_fps();
	while (!dsi_fps && timeout--) {
		/* wait for 5~10 sec for dp stable & frame tracking lock */
		usleep_range(5000, 10000);
		dsi_fps = mtk_dispsys_get_current_fps();
	}

	init_dma_attrs(&dma_attrs);
	dma_set_attr(DMA_ATTR_WRITE_COMBINE | DMA_ATTR_FORCE_CONTIGUOUS,
					&dma_attrs);
	va = (u32 *)dma_alloc_attrs(latency_dev->dev,
			sizeof(u32), &pa, GFP_KERNEL, &dma_attrs);
	*va = 0;
	dev_err(latency_dev->dev, "Init value: %u(0x%llx) for GCE event: %d\n",
			*va, pa, gce_id);

	cmdq_pkt_create(&cmdq_pkt);
	cmdq_pkt_clear_event(cmdq_pkt, gce_id);
	cmdq_pkt_wfe(cmdq_pkt, gce_id);

	mtk_ts_cmd_gce_get_vts(latency_dev, cmdq_pkt, pa);

	while (!kthread_should_stop()) {
		if (gce_event_receive[DSI_SOF]) {
			if (cmdq_pkt_flush(latency_dev->cmdq_client[PIPE8_CH],
				cmdq_pkt) != 0) {
				/*pf_info("=== get event timeout! ===\n");*/
			} else {
				if (gce_event_receive[DSI_SOF] ==
						EVENT_RECEIVE) {
					thread_dsi_sof_func(latency_dev, va, pa,
						gce_id, &counter);
				}
			}
		} else {
			usleep_range(1000, 2000);
		}
	}

	cmdq_pkt_destroy(cmdq_pkt);
	dma_free_attrs(latency_dev->dev, sizeof(u32),
					va, pa, &dma_attrs);
	dev_err(latency_dev->dev, "=== sof thread dsi sof dies ===\n");

	return 0;
}

static int thread_slcr_task(void *data)
{
	struct mtk_latency_verify *latency_dev;
	struct cmdq_pkt *cmdq_pkt;
	struct dma_attrs dma_attrs;
	u32 *va;
	dma_addr_t pa;
	int timeout = 10000;
	int counter = 0;
	u32 gce_id = gce_event[SLICER_EVENT];
	struct slcr_gce gce = {0};

	pr_err("try to receive GCE Event %d\n", gce_id);

	latency_dev = (struct mtk_latency_verify *) data;

	while (!sync_flag && timeout--)
		usleep_range(5000, 10000);

	gce.event = GCE_INPUT;
	gce.height = 143;
	gce.width = 1;

	if (mutex_src == MUTEX_MMSYS_SOF_DP_DSC)
		mtk_slcr_set_gce_event(latency_dev->slcr_dev,
				NULL, DP_DSC, gce);
	else
		mtk_slcr_set_gce_event(latency_dev->slcr_dev,
				NULL, DP_VIDEO, gce);

	init_dma_attrs(&dma_attrs);
	dma_set_attr(DMA_ATTR_WRITE_COMBINE | DMA_ATTR_FORCE_CONTIGUOUS,
					&dma_attrs);
	va = (u32 *)dma_alloc_attrs(latency_dev->dev,
			sizeof(u32), &pa, GFP_KERNEL, &dma_attrs);
	*va = 0;
	dev_err(latency_dev->dev, "Init value: %u(0x%llx) for GCE event: %d\n",
			*va, pa, gce_id);

	cmdq_pkt_create(&cmdq_pkt);
	cmdq_pkt_clear_event(cmdq_pkt, gce_id);
	cmdq_pkt_wfe(cmdq_pkt, gce_id);

	mtk_ts_cmd_gce_get_vts(latency_dev, cmdq_pkt, pa);

	while (!kthread_should_stop()) {
		if (gce_event_receive[SLICER_EVENT]) {
			if (cmdq_pkt_flush(latency_dev->cmdq_client[PIPE9_CH],
				cmdq_pkt) != 0) {
				/*pf_info("=== get event timeout! ===\n");*/
			} else {
				sync_flag = DSI_RECORD;
				pr_info("Updated value: %u (0x%llx) for GCE event: %d\n",
					*va, pa, gce_id);

				task_result[SLICER_EVENT][counter] = *va;
				counter++;
				if (counter == Test_Number[0]) {
					gce_event_receive[SLICER_EVENT] = 0;
					counter = 0;
					sync_flag = SLCR_SLEEP;
				}
			}
		} else {
			usleep_range(2000, 5000);
		}
	}

	cmdq_pkt_destroy(cmdq_pkt);
	dma_free_attrs(latency_dev->dev, sizeof(u32),
					va, pa, &dma_attrs);
	dev_err(latency_dev->dev, "=== sof thread slcr dies ===\n");

	return 0;
}


static int thread_dsi_vactl_task(void *data)
{
	struct mtk_latency_verify *latency_dev;
	struct cmdq_pkt *cmdq_pkt;
	struct dma_attrs dma_attrs;
	u32 *va;
	dma_addr_t pa;
	int counter = 0;
	u32 gce_id = gce_event[DSI_VACTL_EVENT];
	/*int idx = 0;*/

	pr_err("try to receive GCE Event %d\n", gce_id);

	latency_dev = (struct mtk_latency_verify *) data;

	init_dma_attrs(&dma_attrs);
	dma_set_attr(DMA_ATTR_WRITE_COMBINE | DMA_ATTR_FORCE_CONTIGUOUS,
					&dma_attrs);
	va = (u32 *)dma_alloc_attrs(latency_dev->dev,
			sizeof(u32), &pa, GFP_KERNEL, &dma_attrs);
	*va = 0;
	dev_err(latency_dev->dev, "Init value: %u(0x%llx) for GCE event: %d\n",
			*va, pa, gce_id);

	cmdq_pkt_create(&cmdq_pkt);
	cmdq_pkt_clear_event(cmdq_pkt, gce_id);
	cmdq_pkt_wfe(cmdq_pkt, gce_id);

	mtk_ts_cmd_gce_get_vts(latency_dev, cmdq_pkt, pa);

	while (!kthread_should_stop()) {
		if (gce_event_receive[DSI_VACTL_EVENT]) {
			if (cmdq_pkt_flush(latency_dev->cmdq_client[PIPE10_CH],
				cmdq_pkt) != 0) {
				/*pf_info("=== get event timeout! ===\n");*/
			} else {
				dsi_counter++;
				/*pr_err("dsi_counter=%d\n", dsi_counter);*/
				if (sync_flag == DSI_INITIAL)
					sync_flag = SLCR_RUN;

				if (cv_all) {
					if (sync_flag == DSI_RECORD &&
							counter_reset) {
						counter = 0;
						counter_reset = 0;
					}

					if (sync_flag != SLCR_SLEEP)
					task_result[DSI_VACTL_EVENT][counter] =
									*va;

					counter++;
					if (counter == Test_Number[0]) {
					/*gce_event_receive[11] = 0;*/
					counter = 0;
					}
				} else {
					/*to do*/
				}
			}
		} else {
			usleep_range(2000, 5000);
		}
	}

	cmdq_pkt_destroy(cmdq_pkt);
	dma_free_attrs(latency_dev->dev, sizeof(u32),
					va, pa, &dma_attrs);
	dev_err(latency_dev->dev, "=== sof thread dsi vactl dies ===\n");

	return 0;
}


static int mtk_measure_latency(struct mtk_latency_verify *latency_dev,
						bool onoff)
{
	if (onoff) {
		if (bFrmtrk) {
			latency_thread[FRM_TRK] =
					kthread_run(thread_frmtrk_task,
						(void *)latency_dev,
						"thread frmtrk task");
			if (IS_ERR(latency_thread[FRM_TRK])) {
				dev_err(latency_dev->dev, "thread frmtrk startup failed\n");
				latency_thread[FRM_TRK] = NULL;
				return -EPERM;
			}
			return 0;
		} else if (!bVRtrackingTest) {
			pr_info("use PIPE8_CH and PIPE9_CH\n");

			latency_dev->cmdq_client[PIPE8_CH] =
				cmdq_mbox_create(latency_dev->dev, PIPE8_CH);
			if (IS_ERR(latency_dev->cmdq_client[PIPE8_CH])) {
				dev_err(latency_dev->dev, "PIPE8_CH mbox create failed\n");
				return PTR_ERR(
					latency_dev->cmdq_client[PIPE8_CH]);
			}

			latency_dev->cmdq_client[PIPE9_CH] =
				cmdq_mbox_create(latency_dev->dev, PIPE9_CH);
			if (IS_ERR(latency_dev->cmdq_client[PIPE9_CH])) {
				dev_err(latency_dev->dev, "PIPE9_CH mbox create failed\n");
				return PTR_ERR(
					latency_dev->cmdq_client[PIPE9_CH]);
			}
		} else {
			pr_info("use PIPE0_CH and PIPE1_CH\n");

			latency_dev->cmdq_client[PIPE0_CH] =
			cmdq_mbox_create(latency_dev->dev, PIPE0_CH);
			if (IS_ERR(latency_dev->cmdq_client[PIPE0_CH])) {
				dev_err(latency_dev->dev, "PIPE0_CH mbox create failed\n");
				return PTR_ERR(
					latency_dev->cmdq_client[PIPE0_CH]);
			}

			latency_dev->cmdq_client[PIPE1_CH] =
				cmdq_mbox_create(latency_dev->dev, PIPE1_CH);
			if (IS_ERR(latency_dev->cmdq_client[PIPE1_CH])) {
				dev_err(latency_dev->dev, "PIPE1_CH mbox create failed\n");
				return PTR_ERR(
					latency_dev->cmdq_client[PIPE1_CH]);
			}
		}

		latency_thread[0] = kthread_run(thread0_task,
				(void *)latency_dev, "thread0 task");
		if (IS_ERR(latency_thread[0])) {
			dev_err(latency_dev->dev, "thread0 startup failed\n");
			latency_thread[0] = NULL;
			return -EPERM;
		}

		latency_thread[1] = kthread_run(thread1_task,
				(void *)latency_dev, "thread1 task");
		if (IS_ERR(latency_thread[1])) {
			dev_err(latency_dev->dev, "thread1 startup failed\n");
			latency_thread[1] = NULL;
			return -EPERM;
		}
	} else {
		if (!bVRtrackingTest) {
			cmdq_mbox_destroy(latency_dev->cmdq_client[PIPE8_CH]);
			cmdq_mbox_destroy(latency_dev->cmdq_client[PIPE9_CH]);
		} else {
			cmdq_mbox_destroy(latency_dev->cmdq_client[PIPE0_CH]);
			cmdq_mbox_destroy(latency_dev->cmdq_client[PIPE1_CH]);
		}
	}

	return 0;
}


static int mtk_measure_CV_latency(struct mtk_latency_verify *latency_dev,
						bool onoff)
{
	if (onoff) {
		latency_dev->cmdq_client[PIPE0_CH] =
		cmdq_mbox_create(latency_dev->dev, PIPE0_CH);
		if (IS_ERR(latency_dev->cmdq_client[PIPE0_CH])) {
			dev_err(latency_dev->dev, "PIPE0_CH mbox create failed\n");
			return PTR_ERR(
				latency_dev->cmdq_client[PIPE0_CH]);
		}

		latency_dev->cmdq_client[PIPE2_CH] =
		cmdq_mbox_create(latency_dev->dev, PIPE2_CH);
		if (IS_ERR(latency_dev->cmdq_client[PIPE2_CH])) {
			dev_err(latency_dev->dev, "PIPE2_CH mbox create failed\n");
			return PTR_ERR(
				latency_dev->cmdq_client[PIPE2_CH]);
		}

		if (cv_all) {
			latency_dev->cmdq_client[PIPE3_CH] =
				cmdq_mbox_create(latency_dev->dev, PIPE3_CH);
			if (IS_ERR(latency_dev->cmdq_client[PIPE3_CH])) {
				dev_err(latency_dev->dev, "PIPE3_CH mbox create failed\n");
				return PTR_ERR(
					latency_dev->cmdq_client[PIPE3_CH]);
			}

			latency_dev->cmdq_client[PIPE4_CH] =
			cmdq_mbox_create(latency_dev->dev, PIPE4_CH);
			if (IS_ERR(latency_dev->cmdq_client[PIPE4_CH])) {
				dev_err(latency_dev->dev, "PIPE4_CH mbox create failed\n");
				return PTR_ERR(
					latency_dev->cmdq_client[PIPE4_CH]);
			}

			latency_dev->cmdq_client[PIPE9_CH] =
			cmdq_mbox_create(latency_dev->dev, PIPE9_CH);
			if (IS_ERR(latency_dev->cmdq_client[PIPE9_CH])) {
			dev_err(latency_dev->dev, "PIPE9_CH mbox create failed\n");
			return PTR_ERR(
			latency_dev->cmdq_client[PIPE9_CH]);
			}
		}

		latency_dev->cmdq_client[PIPE8_CH] =
			cmdq_mbox_create(latency_dev->dev, PIPE8_CH);
		if (IS_ERR(latency_dev->cmdq_client[PIPE8_CH])) {
			dev_err(latency_dev->dev, "PIPE8_CH mbox create failed\n");
			return PTR_ERR(
				latency_dev->cmdq_client[PIPE8_CH]);
		}

		latency_dev->cmdq_client[PIPE10_CH] =
			cmdq_mbox_create(latency_dev->dev, PIPE10_CH);
		if (IS_ERR(latency_dev->cmdq_client[PIPE10_CH])) {
			dev_err(latency_dev->dev, "PIPE10_CH mbox create failed\n");
			return PTR_ERR(
				latency_dev->cmdq_client[PIPE10_CH]);
		}

		latency_thread[0] = kthread_run(thread0_task,
				(void *)latency_dev, "thread0 task");
		if (IS_ERR(latency_thread[0])) {
			dev_err(latency_dev->dev, "thread0 startup failed\n");
			latency_thread[0] = NULL;
			return -EPERM;
		}

		latency_thread[2] = kthread_run(thread2_task,
			(void *)latency_dev, "thread2 task");
		if (IS_ERR(latency_thread[2])) {
			dev_err(latency_dev->dev, "thread2 startup failed\n");
			latency_thread[2] = NULL;
			return -EPERM;
		}

		if (cv_all) {
			latency_thread[ISP_P1_SOF] = kthread_run(thread_p1_task,
				(void *)latency_dev, "thread p1 task");
			if (IS_ERR(latency_thread[ISP_P1_SOF])) {
				dev_err(latency_dev->dev, "thread p1 startup failed\n");
				latency_thread[ISP_P1_SOF] = NULL;
				return -EPERM;
			}

			latency_thread[BE_DONE] = kthread_run(thread_be_task,
				(void *)latency_dev, "thread be task");
			if (IS_ERR(latency_thread[BE_DONE])) {
				dev_err(latency_dev->dev, "thread be startup failed\n");
				latency_thread[BE_DONE] = NULL;
				return -EPERM;
			}

			latency_thread[SLICER_EVENT] =
				kthread_run(thread_slcr_task,
						(void *)latency_dev,
						"thread slcr task");
			if (IS_ERR(latency_thread[SLICER_EVENT])) {
				dev_err(latency_dev->dev, "thread slcr startup failed\n");
				latency_thread[SLICER_EVENT] = NULL;
				return -EPERM;
			}
		}

		latency_thread[DSI_SOF] = kthread_run(thread_dsi_sof_task,
			(void *)latency_dev, "thread dsi sof task");
		if (IS_ERR(latency_thread[DSI_SOF])) {
			dev_err(latency_dev->dev, "thread dis sof startup failed\n");
			latency_thread[DSI_SOF] = NULL;
			return -EPERM;
		}

		latency_thread[DSI_VACTL_EVENT] =
			kthread_run(thread_dsi_vactl_task,
			(void *)latency_dev, "thread dsi vactl task");
		if (IS_ERR(latency_thread[DSI_VACTL_EVENT])) {
			dev_err(latency_dev->dev, "thread dsi vactl startup failed\n");
			latency_thread[DSI_VACTL_EVENT] = NULL;
			return -EPERM;
		}
	} else {
		cmdq_mbox_destroy(latency_dev->cmdq_client[PIPE0_CH]);
		cmdq_mbox_destroy(latency_dev->cmdq_client[PIPE2_CH]);
		if (cv_all) {
			cmdq_mbox_destroy(latency_dev->cmdq_client[PIPE3_CH]);
			cmdq_mbox_destroy(latency_dev->cmdq_client[PIPE4_CH]);
			cmdq_mbox_destroy(latency_dev->cmdq_client[PIPE9_CH]);
		}
		cmdq_mbox_destroy(latency_dev->cmdq_client[PIPE8_CH]);
		cmdq_mbox_destroy(latency_dev->cmdq_client[PIPE10_CH]);
	}

	return 0;
}

static int mtk_latency_debugfs_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t mtk_latency_debugfs_write(struct file *file,
					    const char __user *ubuf,
					    size_t count, loff_t *ppos)
{
	char *buf = vmalloc(count);
	struct mtk_latency_verify *latency_dev;
	struct slcr_gce gce = {0};

	if (!buf)
		goto out;

	if (copy_from_user(buf, ubuf, count)) {
		vfree(buf);
		return -EFAULT;
	}

	latency_dev = (struct mtk_latency_verify *)file->private_data;

	if (strncmp(buf, "vr_mode_tc_p1", 13) == 0) {
		/*for slicer dsc*/
		gce_event[0] = CMDQ_EVENT_MMSYS_CORE_SLICER_EVENT_1;
		gce_event[1] = CMDQ_EVENT_MMSYS_CORE_DSI0_VACTL_EVENT;

		gce.event = GCE_INPUT;
		gce.height = 143;
		gce.width = 1;
		bVRtrackingTest = false;
		bFrmtrk = false;
		Test_Number[0] = Test_Number[1] = 7;
		need_thread_num = 2;

		mtk_slcr_set_gce_event(latency_dev->slcr_dev,
					NULL, DP_DSC, gce);
		mtk_dsi_vact_event_config(latency_dev->dsi_dev,
					0, 2041, true);

		memset(gce_event_receive, 0x1, GCE_EVENT_NUM * sizeof(u8));
		mtk_measure_latency(latency_dev, true);
	} else if (strncmp(buf, "vr_mode_tc_p2", 13) == 0) {
		/*for slicer Video*/
		gce_event[0] = CMDQ_EVENT_MMSYS_CORE_SLICER_EVENT_0;
		gce_event[1] = CMDQ_EVENT_MMSYS_CORE_DSI0_VACTL_EVENT;

		gce.event = GCE_INPUT;
		gce.height = 143;
		gce.width = 1;
		bVRtrackingTest = false;
		bFrmtrk = false;
		Test_Number[0] = Test_Number[1] = 7;
		need_thread_num = 2;

		mtk_slcr_set_gce_event(latency_dev->slcr_dev,
					NULL, DP_VIDEO, gce);
		mtk_dsi_vact_event_config(latency_dev->dsi_dev,
					0, 2041, true);

		memset(gce_event_receive, 0x1, GCE_EVENT_NUM * sizeof(u8));
		mtk_measure_latency(latency_dev, true);
	} else if (strncmp(buf, "st0005", 6) == 0) {
		gce_event[0] = GCE_SW_TOKEN_SIDE_P1_SOF;
		gce_event[1] = CMDQ_EVENT_CAM_SIDE0_CAM0_BE_FRAME_DONE;
		bVRtrackingTest = true;
		bUse3C = false;
		bFrmtrk = false;
		Test_Number[0] = Test_Number[1] = 1;
		need_thread_num = 2;

		memset(gce_event_receive, 0x1, GCE_EVENT_NUM * sizeof(u8));
		mtk_measure_latency(latency_dev, true);
	} else if (strncmp(buf, "isp_latency", 11) == 0) {
		gce_event[0] = GCE_SW_TOKEN_SIDE_P2_SOF;
		gce_event[1] = GCE_SW_TOKEN_SIDE_WPE_SOF;
		bVRtrackingTest = true;
		bUse3C = false;
		bFrmtrk = false;
		Test_Number[0] = Test_Number[1] = 1;
		need_thread_num = 2;

		memset(gce_event_receive, 0x1, GCE_EVENT_NUM * sizeof(u8));
		mtk_measure_latency(latency_dev, true);
	} else if (strncmp(buf, "warpa_latency", 13) == 0) {
		gce_event[0] = GCE_SW_TOKEN_SIDE_WPE_SOF;
		gce_event[1] = GCE_SW_TOKEN_FE_DONE;
		bVRtrackingTest = true;
		bUse3C = true;
		bFrmtrk = false;
		Test_Number[0] = 1;
		Test_Number[1] = 2;
		need_thread_num = 2;

		memset(gce_event_receive, 0x1, GCE_EVENT_NUM * sizeof(u8));
		mtk_measure_latency(latency_dev, true);
	} else if (strncmp(buf, "st0007_0", 8) == 0) {
		gce_event[0] = GCE_SW_TOKEN_SIDE_WPE_SOF;
		gce_event[1] = GCE_SW_TOKEN_WDMA0_DONE;
		bVRtrackingTest = true;
		bUse3C = true;
		bFrmtrk = false;
		Test_Number[0] = 1;
		Test_Number[1] = 2;
		need_thread_num = 2;

		memset(gce_event_receive, 0x1, GCE_EVENT_NUM * sizeof(u8));
		mtk_measure_latency(latency_dev, true);
	} else if (strncmp(buf, "st0007_1", 8) == 0) {
		gce_event[0] = GCE_SW_TOKEN_SIDE_WPE_SOF;
		gce_event[1] = GCE_SW_TOKEN_WDMA1_DONE;

		bVRtrackingTest = true;
		bUse3C = true;
		bFrmtrk = false;
		Test_Number[0] = 1;
		Test_Number[1] = 2;
		need_thread_num = 2;

		memset(gce_event_receive, 0x1, GCE_EVENT_NUM * sizeof(u8));
		mtk_measure_latency(latency_dev, true);
	} else if (strncmp(buf, "st0007_2", 8) == 0) {
		gce_event[0] = GCE_SW_TOKEN_SIDE_WPE_SOF;
		gce_event[1] = GCE_SW_TOKEN_WDMA2_DONE;
		bVRtrackingTest = true;
		bUse3C = true;
		bFrmtrk = false;
		Test_Number[0] = 1;
		Test_Number[1] = 2;
		need_thread_num = 2;

		memset(gce_event_receive, 0x1, GCE_EVENT_NUM * sizeof(u8));
		mtk_measure_latency(latency_dev, true);
	} else if (strncmp(buf, "cinematic_front:", 16) == 0) {
		u32 height = 1;
		u32 event = GCE_INPUT;
		int ret;

		gce_event[0] = CMDQ_EVENT_MMSYS_CORE_SLICER_EVENT_0;
		gce_event[1] =
			CMDQ_EVENT_MMSYS_CORE_DISP_WDMA0_TARGET_LINE_DONE;

		ret = sscanf(buf, "cinematic_front:%u %u", &event, &height);
		pr_err("parse parameter(%d), %u %u\n", ret, event, height);

		if (event == 0)
			gce.event = GCE_INPUT;
		else if (event == 1)
			gce.event = GCE_OUTPUT_0;

		gce.height = height;/*42;*/
		gce.width = 1;
		bVRtrackingTest = false;
		bFrmtrk = false;
		need_thread_num = 2;
		Test_Number[0] = Test_Number[1] = 7;

		mtk_slcr_set_gce_event(latency_dev->slcr_dev,
						NULL, DP_VIDEO, gce);
		ret = mtk_wdma_set_target_line(latency_dev->wdma_dev,
						NULL, 1);
		pr_err("ret val = (%d)\n", ret);

		memset(gce_event_receive, 0x1, GCE_EVENT_NUM * sizeof(u8));
		mtk_measure_latency(latency_dev, true);
	} else if (strncmp(buf, "cinematic_back:", 15) == 0) {
		u32 pvric_option = 0, target_line;
		int ret;

		ret = sscanf(buf, "cinematic_back:%u %u",
						&pvric_option, &target_line);
		pr_err("parse parameter(%d), %u %u\n", ret,
						pvric_option, target_line);

		if (pvric_option == 0) {
			/*for PVR IC off*/
			gce_event[0] =
		CMDQ_EVENT_MMSYS_CORE_MDP_RDMA0_TARGET_LINE_DONE;
		} else if (pvric_option == 1) {
			/*for PVR IC on*/
			gce_event[0] =
		CMDQ_EVENT_MMSYS_CORE_MDP_RDMA_PVRIC0_TARGET_LINE_DONE;
		}

		gce_event[1] = CMDQ_EVENT_MMSYS_CORE_DSI0_VACTL_EVENT;

		bVRtrackingTest = false;
		need_thread_num = 2;
		bFrmtrk = false;
		Test_Number[0] = Test_Number[1] = 7;

		ret = mtk_rdma_set_target_line(latency_dev->rdma_dev,
						NULL, target_line);
		pr_err("ret val = (%d)\n", ret);
		mtk_dsi_vact_event_config(latency_dev->dsi_dev,
						target_line, 0, true);

		memset(gce_event_receive, 0x1,
				GCE_EVENT_NUM * sizeof(u8));
		mtk_measure_latency(latency_dev, true);
	} else if (strncmp(buf, "off", 3) == 0) {
		int i;

		mtk_measure_latency(latency_dev, false);

		if (!bVRtrackingTest) {
			u32 r1 = task_result[1][3];
			u32 r2 = task_result[0][3];

			if (r1 < r2) {
				if (r1 - task_result[0][2] < 8000)
					pr_err("latency time = %u\n",
						r1 - task_result[0][2]);
			} else if ((r1 > r2) && (r1 - r2 < 8000)) {
				pr_err("latency time = %u\n",
							r1 - r2);
			} else if ((r1 > r2) && (r1 - r2 > 8000)) {
				for (i = 4; i < 7; i++) {
					if (r1 - task_result[0][i] < 8000)
						pr_err("latency time = %u\n",
						r1 - task_result[0][i]);
				}
			}
		} else {
			if (!bUse3C) {
				pr_err("latency time  = %d\n",
					task_result[1][0] - task_result[0][0]);
			} else {
				/*for 3c case*/
				u32 r1 = task_result[1][1];
				u32 r2 = task_result[0][0];

				if (r1 - r2 < 8000)
					pr_err("latency time = %u\n",
						r1 - r2);
				else if (r1 - r2 > 8000)
					pr_err("latency time = %u\n",
						task_result[1][0] - r2);
			}
		}

		memset(gce_event, 0x0, GCE_EVENT_NUM * sizeof(u32));
		memset(gce_event_receive, 0x0, GCE_EVENT_NUM * sizeof(u8));
		memset(task_result, 0,
			sizeof(task_result[0][0]) * GCE_EVENT_NUM * TEST_TIMES);
		memset(cv_result, 0,
			sizeof(cv_result[0][0]) * CV_INFO_NUM * TEST_TIMES);

		bVRtrackingTest = false;
		bUse3C = false;
		bFrmtrk = false;

		for (i = 0; i < 2; i++) {
			if (latency_thread[i]) {
				kthread_stop(latency_thread[i]);
				latency_thread[i] = NULL;
			}
		}
	} else if (strncmp(buf, "cv_latency_all:", 15) == 0) {
		u32 src_option = 1;
		u32 create_thread = 0;
		u32 checkFrmtrk = 0;
		int ret;

		cv_all = 0;
		sync_flag = DSI_INITIAL;
		sync_flag_2 = DSI_INITIAL;
		sync_flag_3 = DSI_INITIAL;
		counter_reset = 1;

		ret = sscanf(buf, "cv_latency_all:%u %u %u %u",
			&cv_all, &src_option, &create_thread, &checkFrmtrk);
		pr_err("parse parameter(%d), %u\n", ret, src_option);

		if (src_option == 1)
			mutex_src = MUTEX_MMSYS_SOF_DP_DSC;
		else if (src_option == 2)
			mutex_src = MUTEX_MMSYS_SOF_DP;
		else
			pr_err("wron src_option\n");

		gce_event[ISP_P2_SOF] = GCE_SW_TOKEN_SIDE_P2_SOF;
		gce_event[WPE_SOF] = GCE_SW_TOKEN_SIDE_WPE_SOF;
		gce_event[FE_DONE] = GCE_SW_TOKEN_FE_DONE;
		gce_event[WDMA0_DONE] = GCE_SW_TOKEN_WDMA0_DONE;
		gce_event[DSI_SOF] = CMDQ_EVENT_MMSYS_CORE_SOF_FOR_34_DSI1;


		if (cv_all) {
			gce_event[WDMA1_DONE] = GCE_SW_TOKEN_WDMA1_DONE;
			gce_event[WDMA2_DONE] = GCE_SW_TOKEN_WDMA2_DONE;
			gce_event[ISP_P1_SOF] = GCE_SW_TOKEN_SIDE_P1_SOF;
			gce_event[BE_DONE] =
					CMDQ_EVENT_CAM_SIDE0_CAM0_BE_FRAME_DONE;

			if (mutex_src == MUTEX_MMSYS_SOF_DP_DSC)
				gce_event[SLICER_EVENT] =
					CMDQ_EVENT_MMSYS_CORE_SLICER_EVENT_1;
			else
				gce_event[SLICER_EVENT] =
					CMDQ_EVENT_MMSYS_CORE_SLICER_EVENT_0;

			gce_event[DSI_VACTL_EVENT] =
					CMDQ_EVENT_MMSYS_CORE_DSI0_VACTL_EVENT;
			need_thread_num = 8;
		} else {
			sync_flag_3 = BE_DONE_RUN;
			gce_event[DSI_VACTL_EVENT] =
				CMDQ_EVENT_MMSYS_CORE_DSI0_VACTL_EVENT;
			need_thread_num = 4;
		}

		Test_Number[0] = Test_Number[1] = TEST_TIMES;
		bVRtrackingTest = true;

		memset(gce_event_receive, 0x2, GCE_EVENT_NUM * sizeof(u8));
		memset(dsi_frame_num, 0x0, sizeof(u32) * 3);


		if (!checkFrmtrk)
			bFrmtrk = false;
		else if (create_thread && checkFrmtrk) {
			bFrmtrk = true;
			mtk_measure_latency(latency_dev, true);
		} else if (!create_thread && checkFrmtrk) {
			bFrmtrk = true;
		}

		if (create_thread) {
			for_initial_frame = 1;
			mtk_measure_CV_latency(latency_dev, true);
		} else
			for_initial_frame = 0;
	} else if (strncmp(buf, "cv_off:", 7) == 0) {
		u32 stop_thread = 0, is_initial_frame = 0;
		int ret;
		int i;

		is_initial_frame = 0;

		ret = sscanf(buf, "cv_off:%u %u", &stop_thread,
					&is_initial_frame);
		pr_err("parse parameter(%d), %u %u\n", ret,
					stop_thread, is_initial_frame);

		memset(gce_event, 0x0, GCE_EVENT_NUM * sizeof(u32));

		bVRtrackingTest = false;
		bFrmtrk = false;

		if (is_initial_frame)
			list_latency_time(1);
		else
			list_latency_time(0);

		memset(task_result, 0,
			sizeof(task_result[0][0]) * GCE_EVENT_NUM * TEST_TIMES);
		memset(cv_result, 0,
			sizeof(cv_result[0][0]) * CV_INFO_NUM * TEST_TIMES);

		if (stop_thread) {
			dsi_frame_num_idx = 0;
			memset(dsi_frame_num, 0x0, sizeof(u32) * 3);
			cv_frame_num = 0;
			dsi_counter = 0;

			memset(gce_event_receive, 0x0,
					GCE_EVENT_NUM * sizeof(u8));
			usleep_range(500, 1000);

			for (i = 0; i < GCE_EVENT_NUM; i++) {
				if (latency_thread[i]) {
					kthread_stop(latency_thread[i]);
					latency_thread[i] = NULL;
				}
			}

			mtk_measure_CV_latency(latency_dev, false);
		}
	}
	vfree(buf);

out:
	return count;
}


static const struct file_operations mtk_latency_verify_debugfs_fops = {
	.open = mtk_latency_debugfs_open,
	.read = seq_read,
	.write = mtk_latency_debugfs_write,
};


static void mtk_latency_debugfs_init(struct mtk_latency_verify *latency_dev)
{
	latency_dev->debugfs =
		debugfs_create_file("mtk_latency_verify",
			S_IFREG | S_IRUGO | S_IWUSR | S_IWGRP, NULL,
			(void *)latency_dev,
			&mtk_latency_verify_debugfs_fops);
	dev_dbg(latency_dev->dev, "mtk_latency_verify_debugfs_init done");
}

static void mtk_latency_debugfs_deinit(struct mtk_latency_verify *latency_dev)
{
	debugfs_remove(latency_dev->debugfs);
	latency_dev->debugfs = NULL;
}

static int mtk_latency_test_get_device(struct device *dev,
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

static int mtk_latency_verify_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_latency_verify *latency_dev;
	int ret, i;


	latency_dev = devm_kzalloc(dev, sizeof(*latency_dev), GFP_KERNEL);
	if (!latency_dev)
		return -ENOMEM;
	latency_dev->dev = dev;
	dev_dbg(latency_dev->dev, "latency_verify");

	mtk_latency_debugfs_init(latency_dev);

	ret = mtk_latency_test_get_device(dev, "mediatek,fm-dev", 0,
			&latency_dev->dev_fm_dev);
	if (ret) {
		dev_dbg(dev, "get fm_dev failed!! %d\n", ret);
		return ret;
	}
	latency_dev->fm_dev = dev_get_drvdata(latency_dev->dev_fm_dev);

	ret = mtk_latency_test_get_device(dev, "mediatek,dsi", 0,
			&latency_dev->dsi_dev);
	if (ret) {
		dev_dbg(dev, "get dsi failed!! %d\n", ret);
		return ret;
	}

	ret = mtk_latency_test_get_device(dev, "mediatek,slcr", 0,
			&latency_dev->slcr_dev);
	if (ret) {
		dev_dbg(dev, "get slcr failed!! %d\n", ret);
		return ret;
	}

	ret = mtk_latency_test_get_device(dev, "mediatek,wdma", 0,
					    &latency_dev->wdma_dev);
	if (ret) {
		dev_dbg(dev, "get wdma failed!! %d\n", ret);
		return ret;
	}

	ret = mtk_latency_test_get_device(dev, "mediatek,mdp_rdma", 0,
						&latency_dev->rdma_dev);
	if (ret) {
		dev_dbg(dev, "get mdp_rdma failed!! %d\n", ret);
		return ret;
	}

	ret = mtk_latency_test_get_device(dev, "mediatek,pvric_rdma", 0,
			&latency_dev->pvric_rdma_dev);
	if (ret) {
		dev_dbg(dev, "get pvric_rdma failed!! %d\n", ret);
		return ret;
	}

	ret = mtk_latency_test_get_device(dev, "mediatek,disp", 0,
			&latency_dev->dev_disp_dev);
	if (ret) {
		dev_dbg(dev, "get disp failed!! %d\n", ret);
		return ret;
	}
	latency_dev->disp_dev = dev_get_drvdata(latency_dev->dev_disp_dev);

	platform_set_drvdata(pdev, latency_dev);

	of_property_read_u32_array(dev->of_node, "gce-events",
			latency_dev->cmdq_events,
			MTK_LATENCY_DEV_CMDQ_EVENT_MAX);

	mutex_init(&latency_dev->lock);

	memset(gce_event, 0, GCE_EVENT_NUM * sizeof(u32));
	memset(gce_event_receive, 0, GCE_EVENT_NUM * sizeof(u8));

	dev_dbg(latency_dev->dev, "Latency probe successfully\n");

	dsi_counter = 0;
	dsi_frame_num_idx = 0;
	cv_all = 0;

	for (i = 0; i < GCE_EVENT_NUM; i++)
		latency_thread[i] = NULL;

	return 0;
}

static int mtk_latency_verify_remove(struct platform_device *pdev)
{
	struct mtk_latency_verify *latency_dev = platform_get_drvdata(pdev);

	mtk_latency_debugfs_deinit(latency_dev);

	return 0;
}

static const struct of_device_id latency_verify_driver_dt_match[] = {
		{ .compatible = "mediatek,mt3612_latency_verify" },
	{},
};


MODULE_DEVICE_TABLE(of, latency_verify_driver_dt_match);

struct platform_driver mtk_latency_verify_driver = {
	.probe		= mtk_latency_verify_probe,
	.remove		= mtk_latency_verify_remove,
	.driver		= {
		.name	= "mediatek-latency_verify",
		.owner	= THIS_MODULE,
		.of_match_table = latency_verify_driver_dt_match,
	},
};
module_platform_driver(mtk_latency_verify_driver);
MODULE_LICENSE("GPL");

