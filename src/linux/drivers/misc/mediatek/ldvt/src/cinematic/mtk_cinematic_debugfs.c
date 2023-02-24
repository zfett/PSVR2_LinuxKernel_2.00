/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	Peggy Jao <peggy.jao@mediatek.com>
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

/**
 * @file mtk_cinematic_debugfs.c
 * cinematic debugfs driver.
 * ex: echo path 0 1920 1080 /sys/kernel/debug/mediatek/cinematic/debug
 */

#include <linux/debugfs.h>
#include <linux/dma-attrs.h>
#include <linux/dma-mapping.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <uapi/drm/drm_fourcc.h>
#include <soc/mediatek/mtk_mutex.h>
#include <soc/mediatek/mtk_wdma.h>
#include <mtk_common_util.h>
#include "mtk_cinematic.h"
#include "mtk_cinematic_ctrl.h"

static void setting_parameters(struct mtk_cinematic_test *ct)
{
	/* gpu-out-w */
	ct->para.gp.out_w = 1000;
	/* gpu-out-h */
	ct->para.gp.out_h = 1920;
	/* gpu-in-format
	 * DRM_FORMAT_AYUV2101010,
	 * DRM_FORMAT_ABGR8888,
	 * DRM_FORMAT_ABGR2101010,
	 * DRM_FORMAT_BGR888
	 */
	ct->para.gp.in_fmt = DRM_FORMAT_ABGR8888;
	/* gpu-out-format */
	ct->para.gp.out_fmt = DRM_FORMAT_ABGR8888;
	/* strip-mode */
	ct->para.gp.strip_mode = 0;
	/* strip-height */
	ct->para.gp.strip_h = 128;
	/* strip-buf-num */
	ct->para.gp.strip_buf_num = 5;
	/* pat-sel */
	ct->para.pat_sel = 2;
	/* lhc-en */
	ct->para.lhc_en = 0;
	/* lhc-sync-path */
	ct->para.lhc_sync_path = 1;
	/* pvric-en */
	ct->para.pvric_en = 0;
	/* sof-sel  0:single; 1:continue*/
	ct->para.sof_sel = 1;
	/* crop-x */
	ct->para.cp.x = 0;
	/* crop-y */
	ct->para.cp.y = 0;
	/* input-from-dp */
	ct->para.input_from_dp = 0;
	/* sbrc-err */
	ct->para.err_detect = 0;
	/* error recover */
	ct->para.sbrc_recover = 0;
}

/** @ingroup IP_group_cinematic_internal_function_debug
 * @par Description
 *     This is cmd parsing function.
 * @param[in] cc: cinematic_ctrl device
 * @param[in] cmd_buf: cmd from user
 * @return
 *     0, file open success. \n
 *     Negative, file open failed.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int parse_cmd(struct mtk_cinematic_ctrl *cc, char *token, char *cmd_buf)
{
	const char sp[] = " ";
	char file_path[32] = {0};
	int idx = 0;
	int input_parm[8] = {0};
	struct mtk_cinematic_test *ct;

	if (token == NULL)
		goto err;

	if (strncmp(token, "path", 4) == 0) {
		token = strsep(&cmd_buf, sp);

		if (kstrtoint(token, 10, &cc->act_inst))
			goto err;
		token = strsep(&cmd_buf, sp);
		while (token != NULL) {
			if (kstrtoint(token, 10, &input_parm[idx]))
				goto err;
			token = strsep(&cmd_buf, sp);
			idx++;
		}
		ct = cc->ct[cc->act_inst];
		/* pipe-idx */
		ct->para.pipe_idx = cc->act_inst;

		setting_parameters(ct);
		/* crop and resizer bypass */
		/* crop-out-width, rsz-out-width */
		ct->para.input_width =
		ct->para.cp.out_w = ct->para.rp.out_w = input_parm[0];
		/* crop-out-height, rsz-out-height */
		ct->para.input_height =
		ct->para.cp.out_h = ct->para.rp.out_h = input_parm[1];
		ct->para.gp.bypass = input_parm[2];
		ct->para.sof_sel = input_parm[3];

		if (check_src_validation(ct->para.input_width,
					 ct->para.input_height)) {
			dev_err(cc->dev,
				"src timing not support, using 1280x720\n");
			ct->para.input_width = 1280;
			ct->para.input_height = 720;
		}
		dev_err(cc->dev, "### cmd: parsing usr command ###\n");
	} else if (strncmp(token, "init", 4) == 0) {
		cc->cmd = INIT_PATH;
		dev_err(cc->dev, "### cmd: init single pipe[%d] ###\n",
			cc->act_inst);
	} else if (strncmp(token, "tpath", 5) == 0) {
		cc->cmd = TRIG_PATH;
		dev_err(cc->dev, "### cmd: trigger single pipe[%d] ###\n",
			cc->act_inst);
	} else if (strncmp(token, "reset", 5) == 0) {
		cc->cmd = RESET_PATH;
		dev_err(cc->dev, "### cmd: reset single pipe[%d] ###\n",
			cc->act_inst);
	} else if (strncmp(token, "disp", 4) == 0) {
		cc->cmd = DISP_PATH;
		dev_err(cc->dev, "### cmd: display single pipe[%d] ###\n",
			cc->act_inst);
	} else if (strncmp(token, "end", 3) == 0) {
		cc->cmd = DEINIT_PATH;
		dev_err(cc->dev, "### cmd: deinit single pipe[%d] ###\n",
		cc->act_inst);
	} else if (strncmp(token, "dump", 4) == 0) {
		dev_err(cc->dev, "### dum image ###\n");
		strncpy(file_path, cmd_buf, strlen(cmd_buf)-1);
		if (cc->displayed) {
			ct = cc->ct[cc->act_inst];
			mtk_dump_buf(ct, cc->disp_fb->fb_id,
			    ct->image_output_size, file_path);
		} else
			dev_err(cc->dev, "=== buf not ready skip saving ===\n");

	} else {
		goto err;
	}

	return 0;

err:
	dev_err(cc->dev, "### get instruction err ###\n");
	return -EINVAL;
}

static int cinematic_dbg_print(struct seq_file *s, void *unused)
{
	struct mtk_cinematic_ctrl *cc = s->private;
	struct mtk_cinematic_test *ct = cc->ct[cc->act_inst];
	struct rsz_para rp;
	u32 pre_b_cnt, total_b_cnt, comp_ratio;
	u32 idx;
#ifdef CACHED_BUF
	u32 i = 0;
#endif

	rp = ct->para.rp;
	/* bytes count */
	total_b_cnt = rp.out_w * rp.out_h * ct->output_port.bpp >> 3;

	seq_puts(s, "----------------- Cinematic DEUBG -----------------\n");
	seq_printf(s, "Act_Inst          :%11d\n", cc->act_inst);
	switch (cc->cmd) {
	case INIT_PATH:
		seq_printf(s, "Current_State     :%11s\n", "Init");
		break;
	case TRIG_PATH:
		seq_printf(s, "Current_State     :%11s\n", "Trigger");
		break;
	case DISP_PATH:
		seq_printf(s, "Current_State     :%11s\n", "Display");
		break;
	case RESET_PATH:
		seq_printf(s, "Current_State     :%11s\n", "Reset");
		break;
	case DEINIT_PATH:
		seq_printf(s, "Current_State     :%11s\n", "DeInit");
		break;
	case PAUSE_PATH:
		seq_printf(s, "Current_State     :%11s\n", "Paused");
		break;
	case RESUME_PATH:
		seq_printf(s, "Current_State     :%11s\n", "Resume");
		break;
	case UNKNOWN_CMD:
		seq_printf(s, "Current_State     :%11s\n", "Err_CMD");
		break;
	}
	seq_printf(s, "Dual Mode         :%11d\n", cc->dual_mode);
	seq_printf(s, "GPU_BYPASS        :%11d\n", ct->para.gp.bypass);

	if (ct->para.gp.strip_mode) {
		seq_printf(s, "Mode              :%11s\n", "SRAM");
		seq_printf(s, "strip_h           :%11d\n", ct->para.gp.strip_h);
		seq_printf(s, "strip_num         :%11d\n",
			   ct->para.gp.strip_buf_num);
#ifdef CACHED_BUF
		for (i = 0; i < DIFF_COUNT_MAX; i++)
			dma_sync_single_for_cpu(ct->dev, cc->sd.d_pa[i],
						sizeof(u32), DMA_FROM_DEVICE);
#endif
		seq_printf(s, "Min R/W Diff      :%11d\n",
			   *cc->sd.d_va[MIN_DIFF]);
		seq_printf(s, "AVG R/W Diff      :%11d\n",
			   *cc->sd.d_va[SUM_DIFF] / *cc->sd.d_va[F_COUNT]);
#if 0
		seq_printf(s, "Sum            :%11d\n", *cc->sd.d_va[SUM_DIFF]);
		seq_printf(s, "F_Count        :%11d\n", *cc->sd.d_va[F_COUNT]);
#endif
		seq_printf(s, "Current Diff      :%11d\n",
			   *cc->sd.d_va[CUR_DIFF]);
	} else {
		seq_printf(s, "Mode              :%11s\n", "DRAM");
	}

	seq_printf(s, "pvric_en          :%11d\n", ct->para.pvric_en);

	if ((ct->para.pvric_en & COMPRESS_INPUT_ENABLE) && cc->act_inst == 0) {
		mtk_wdma_get_pvric_frame_comp_pre(ct->dev_wdma, &pre_b_cnt);
		comp_ratio = pre_b_cnt * 100 / total_b_cnt;
		seq_printf(s, "Comp_Ratio        :%11d %%\n", comp_ratio);
		seq_printf(s, "MAX_Comp_Ratio    :%11d %%\n",
			   cc->max_wdma_comp_ratio);
		seq_printf(s, "AVG_Comp_Ratio    :%11d %%\n",
			   cc->avg_wdma_comp_ratio);
	}

	switch (ct->para.gp.in_fmt) {
	case DRM_FORMAT_BGR888:
		seq_printf(s, "GPU_IN FORMAT     :%11s\n", "DRM_BGR888");
		break;
	case DRM_FORMAT_ABGR2101010:
		seq_printf(s, "GPU_IN FORMAT     :%11s\n", "DRM_ABGR2101010");
		break;
	case DRM_FORMAT_AYUV2101010:
		seq_printf(s, "GPU_IN FORMAT     :%11s\n", "DRM_AYUV2101010");
		break;
	case DRM_FORMAT_ABGR8888:
		seq_printf(s, "GPU_IN FORMAT     :%11s\n", "DRM_ABGR8888");
		break;
	}

	switch (ct->para.gp.out_fmt) {
	case DRM_FORMAT_BGR888:
		seq_printf(s, "GPU_OUT FORMAT    :%11s\n", "DRM_BGR888");
		break;
	case DRM_FORMAT_ABGR2101010:
		seq_printf(s, "GPU_OUT FORMAT    :%11s\n", "DRM_ABGR2101010");
		break;
	case DRM_FORMAT_AYUV2101010:
		seq_printf(s, "GPU_OUT FORMAT    :%11s\n", "DRM_AYUV2101010");
		break;
	case DRM_FORMAT_ABGR8888:
		seq_printf(s, "GPU_OUT FORMAT    :%11s\n", "DRM_ABGR8888");
		break;
	}

	for (idx = 0; idx < ct->inserted_buf; idx++)
		seq_printf(s, "WDMA BUFNUM[%d]    :%11llx\n", idx,
			   ct->output_port.buf[idx].ion_buf->dma_addr);
	for (idx = 0; idx < cc->inserted_rdma_buf; idx++)
		seq_printf(s, "RDMA BUFNUM[%d]    :%11llx\n", idx,
			   cc->rdma_ion_buf[idx]->dma_addr);

	seq_printf(s, "Input W           :%11d\n", ct->w);
	seq_printf(s, "Input H           :%11d\n", ct->h);
	if (ct->para.frmtrk_en)
		seq_printf(s, "frmtrk dist(us)   :%11d\n", cc->frmtrk_dist);
	seq_printf(s, "Crop Out_W        :%11d\n", ct->para.cp.out_w);
	seq_printf(s, "Crop Out_H        :%11d\n", ct->para.cp.out_h);
	seq_printf(s, "RSZ Out_W         :%11d\n", ct->para.rp.out_w);
	seq_printf(s, "RSZ Out_H         :%11d\n", ct->para.rp.out_h);
	seq_printf(s, "wdma T line       :%11d\n", ct->target_line);
	seq_printf(s, "wdma buf num      :%11d\n", ct->inserted_buf);
	seq_printf(s, "gpu skip count    :%11d\n", cc->skip_frame_cnt);
	seq_printf(s, "wdma frame cnt    :%11lld\n", cc->wdma_frame_cnt);
	seq_printf(s, "wdma sof cnt      :%11lld\n", cc->wdma_sof_cnt);
	seq_printf(s, "wdma inc cnt      :%11d\n", cc->wdma_inc_cnt);
	if (!ct->para.gp.strip_mode)
		seq_printf(s, "gpu frame cnt     :%11lld\n", cc->gpu_frame_cnt);
	seq_printf(s, "dbg_level         :%11d\n", mtk_cinematic_get_dbg());
	seq_puts(s, "----------------------------------------------------\n");
	return 0;
}

static void mtk_write_perf_log(u32 *va_d, u32 *va_w, u32 *va_r, char *file_path,
			       u32 total_rec)
{
	struct file *fp;
	char buf[64];
	mm_segment_t oldfs;
	loff_t pos = 0;
	loff_t i = 0;

	pr_info("mtk_write_perf_log: %s\n", file_path);

	oldfs = get_fs();
	set_fs(get_ds());

	fp = filp_open(file_path, O_WRONLY | O_CREAT, 0644);

	if (IS_ERR(fp)) {
		pr_err("mtk_write_perf_log open %s fail!\n", file_path);
		goto out;
	}

	fp->f_pos = pos;
	memset(buf, 0, sizeof(buf));

	for (i = 0; i < total_rec; i++) {

		snprintf(buf, sizeof(buf) - 1, "[%lld] diff     = %d\n",
			 i, *(va_d + i));
		vfs_write(fp, buf, strlen(buf), &pos);
		pos += fp->f_pos;

		snprintf(buf, sizeof(buf) - 1, "[%lld] w_op cnt = %d\n",
			 i, *(va_w + i));
		vfs_write(fp, buf, strlen(buf), &pos);
		pos += fp->f_pos;

		snprintf(buf, sizeof(buf) - 1, "[%lld] r_op cnt = %d\n\n",
			 i, *(va_r + i));
		vfs_write(fp, buf, strlen(buf), &pos);
		pos += fp->f_pos;
	}

	filp_close(fp, 0);
	pr_info("mtk_write_perf_log: open %s success!\n", file_path);
out:
	set_fs(oldfs);
}

static int mtk_check_sof_diff(struct mtk_cinematic_ctrl *cc, u32 src, u32 ref)
{
	struct mtk_cinematic_test *ct;
	struct mtk_mutex_res *mutex;
	struct cmdq_pkt *cmdq_pkt;
	struct mtk_mutex_timer_status time;
	u32 td_sof_id;
	u64 diff_time;
	int ret = 0;

	if (IS_ERR(cc))
		return -EINVAL;

	ct = cc->ct[cc->act_inst];

	if (IS_ERR(ct))
		return -EINVAL;

	/* using mutex 4 */
	mutex = mtk_mutex_get(ct->dev_mutex, SOF_DIFF);
	if (IS_ERR(mutex)) {
		dev_err(cc->dev, "Get sof diff mutex NG!!\n");
		return -EIO;
	}

	td_sof_id = cc->cmdq_events[MTK_MM_CORE_TD_EVENT4];

	cmdq_pkt_create(&cmdq_pkt);
	cmdq_pkt_clear_event(cmdq_pkt, td_sof_id);
	cmdq_pkt_wfe(cmdq_pkt, td_sof_id);

	mtk_mutex_select_timer_sof(mutex, src, ref, NULL);

	mtk_mutex_set_timer_us(mutex, SOF_TIMEOUT, SOF_TIMEOUT, NULL);

	mtk_mutex_timer_enable_ex(mutex, true, MUTEX_TO_EVENT_REF_EDGE, NULL);

	/* reuse unmute channel, run this should alwasy after unmute */
	if (cmdq_pkt_flush(cc->cmdq_client[UNMUTE_CH], cmdq_pkt) == 0) {

		ret = mtk_mutex_timer_get_status(mutex, &time);

		if (ret) {
			dev_err(cc->dev, "Get sof diff status NG!!\n");
			goto RELEASE;
		} else {
			/* to pico seconds than to nano to prevent lost*/
			diff_time = ((u64)((u64)((1000000/320) *
				     (u64)time.ref_time)) / 1000000);

			dev_err(cc->dev, "diff time = %lld (us)\n", diff_time);
			dev_err(cc->dev, "ref_time = %d tick\n", time.ref_time);
		}
	}

RELEASE:
	cmdq_pkt_destroy(cmdq_pkt);
	mtk_mutex_timer_disable(mutex, NULL);
	mtk_mutex_put(mutex);
	return 0;
}

/** @ingroup IP_group_cinematic_internal_function_debug
 * @par Description
 *     This is cinematic debugfs file open function.
 * @param[in] inode: device inode
 * @param[in] file: file pointer
 * @return
 *     0, file open success. \n
 *     Negative, file open failed.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_cinematic_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, cinematic_dbg_print, inode->i_private);
}

/** @ingroup IP_group_cinematic_internal_function_debug
 * @par Description
 *     This is cinematic debugfs file write function.
 * @param[in]
 *     file: file pointer
 * @param[in]
 *     ubuf: user space buffer pointer
 * @param[in]
 *     count: number of data to write
 * @param[in,out]
 *     ppos: offset
 * @return
 *     Negative, file write failed. \n
 *     Others, number of data wrote.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static ssize_t mtk_cinematic_debugfs_write(struct file *file,
					   const char __user *ubuf,
					   size_t count, loff_t *ppos)
{
	char *readbuf = vmalloc(count+1);
	char *rptr;
	struct mtk_cinematic_ctrl *cc;
	struct mtk_cinematic_test *ct;
	struct seq_file *seq_ptr;
	const char sp[] = " ";
	char *token;
	u32 total_rec;

	if (!readbuf)
		goto out;

	if (copy_from_user(readbuf, ubuf, count)) {
		vfree(readbuf);
		return -EFAULT;
	}

	seq_ptr = (struct seq_file *)file->private_data;

	readbuf[count] = '\0';
	cc = (struct mtk_cinematic_ctrl *)seq_ptr->private;
	ct = cc->ct[cc->act_inst];

	rptr = readbuf;
	/* set parameters */
	token = strsep(&rptr, sp);

	/* dbg print */
	if (strncmp(token, "dbg", 3) == 0) {
		int dbg_mode;

		token = strsep(&rptr, sp);
		if (token == NULL)
			goto out;

		if (kstrtouint(token, 10, &dbg_mode))
			goto out;

		mtk_cinematic_set_dbg(dbg_mode);
		pr_err("debug mask set :%d!!\n", dbg_mode);

	} else if (strncmp(token, "s_rate", 6) == 0) {

		token = strsep(&rptr, sp);
		if (token == NULL)
			goto out;

		if (kstrtouint(token, 10, &cc->sampling_rate))
			goto out;

		mtk_mutex_set_timer_us(ct->mutex, cc->sampling_rate, 200, NULL);
		pr_err("SBRC Monitoring Rate:%d us!!\n", cc->sampling_rate);

	} else if (strncmp(token, "track_dist", 10) == 0) {

		token = strsep(&rptr, sp);
		if (token == NULL)
			goto out;

		if (kstrtouint(token, 10, &cc->frmtrk_dist))
			goto out;

		pr_err("dp and dsi track dist:%d us!!\n", cc->frmtrk_dist);

	} else if (strncmp(token, "sw_f", 4) == 0) {
		/* for emulator usage */
		dump_next_frame(cc);

	} else if (strncmp(token, "perf_save", 9) == 0) {

#ifdef CACHED_BUF
		dma_sync_single_for_cpu(cc->dev, cc->sd.d_pa[F_COUNT],
					sizeof(u32) * REC_NUM, DMA_FROM_DEVICE);
		dma_sync_single_for_cpu(cc->dev, rec_pa_d,
					sizeof(u32) * REC_NUM, DMA_FROM_DEVICE);
		dma_sync_single_for_cpu(cc->dev, rec_pa_r,
					sizeof(u32) * REC_NUM, DMA_FROM_DEVICE);
		dma_sync_single_for_cpu(cc->dev, rec_pa_w,
					sizeof(u32) * REC_NUM, DMA_FROM_DEVICE);
#endif
		total_rec = *cc->sd.d_va[F_COUNT];

		if (total_rec)
			mtk_write_perf_log(rec_va_d, rec_va_w, rec_va_r,
					   "/tmp/perf.log", total_rec);
		else
			pr_err("do NOT have any perf record in buffer!!!\n");

	} else if (strncmp(token, "perf_disp", 9) == 0) {

		total_rec = *cc->sd.d_va[F_COUNT];

		if (total_rec)
			dump_monitor_data(cc);
		else
			pr_err("do NOT have any perf record in buffer!!!\n");

	} else if (strncmp(token, "sof_diff", 8) == 0) {
		u32 src, ref;

		src = MUTEX_MMSYS_SOF_DP;
		ref = MUTEX_MMSYS_SOF_DP;

		pr_err("DP DIFF\n");
		if (mtk_check_sof_diff(cc, src, ref))
			pr_err("Get DP Diff NG!!!!\n");

		src = MUTEX_MMSYS_SOF_DSI0;
		ref = MUTEX_MMSYS_SOF_DSI0;

		pr_err("DSI DIFF\n");
		if (mtk_check_sof_diff(cc, src, ref))
			pr_err("Get DSI Diff NG!!!!\n");

		src = MUTEX_MMSYS_SOF_DP;
		ref = MUTEX_MMSYS_SOF_DSI0;

		pr_err("DP/DSI DIFF\n");
		if (mtk_check_sof_diff(cc, src, ref))
			pr_err("Get DP/DSI Diff NG!!!!\n");

		src = MUTEX_MMSYS_SOF_DP;
		ref = MUTEX_MMSYS_SOF_SYNC_DELAY0 + DISPSYS_DELAY_NUM +
			ct->disp_path;

		pr_err("DP/Delay DP DIFF\n");
		if (mtk_check_sof_diff(cc, src, ref))
			pr_err("Get DP/Delay DP Diff NG!!!!\n");

	} else if (strncmp(token, "reset_slcr", 10) == 0) {
		/* TODO: provide reset slicer feature here */
	} else if (strncmp(token, "pause", 5) == 0) {
		if (cc->cmd == DISP_PATH) {
			dev_err(cc->dev, "paused cmd received\n");
			cc->cmd = PAUSE_PATH;

		} else {
			dev_err(cc->dev, "error only display mode can pause\n");
		}
	} else if (strncmp(token, "resume", 6) == 0) {
		if (cc->cmd == PAUSE_PATH) {
			dev_err(cc->dev, "resume cmd received\n");
			cc->cmd = RESUME_PATH;

		} else {
			dev_err(cc->dev, "error! only pause mode can resume\n");
		}
	} else if (strncmp(token, "safe", 4) == 0) {

		token = strsep(&rptr, sp);
		if (token == NULL)
			goto out;

		if (kstrtouint(token, 10, &cc->safe_line))
			goto out;

		dev_err(cc->dev, "safe line cmd set [%d]\n", cc->safe_line);
	} else {
		if (parse_cmd(cc, token, rptr) < 0)
			goto out;

		if (strncmp(token, "path", 4) != 0)
			mtk_cinematic_dispatch_cmd(cc);
	}
out:
	vfree(readbuf);
	return count;
}

/**
 * @brief a file operation structure for cinematic_debugfs driver
 */
static const struct file_operations mtk_cinematic_debugfs_fops = {
	.open = mtk_cinematic_debugfs_open,
	.read = seq_read,
	.write = mtk_cinematic_debugfs_write,
	.release = single_release,
};

/** @ingroup IP_group_cinematic_internal_function_debug
 * @par Description
 *     Register this test function and create file entry in debugfs.
 * @param[in]
 *     cc: cinematic_ctrl driver data struct
 * @return
 *     0, debugfs init success \n
 *     Others, debugfs init failed
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mtk_cinematic_debugfs_init(struct mtk_cinematic_ctrl *cc)
{
	cc->debugfs = debugfs_create_file("cinematic_debug", 0664, NULL,
			(void *)cc, &mtk_cinematic_debugfs_fops);
}
EXPORT_SYMBOL(mtk_cinematic_debugfs_init);

/** @ingroup IP_group_cinematic_internal_function_debug
 * @par Description
 *     Unregister this test function and remove file entry from debugfs.
 * @param[in]
 *     cc: cinematic_ctrl driver data struct
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mtk_cinematic_debugfs_deinit(struct mtk_cinematic_ctrl *cc)
{
	debugfs_remove(cc->debugfs);
	cc->debugfs = NULL;
}
EXPORT_SYMBOL(mtk_cinematic_debugfs_deinit);
