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

#include <linux/string.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/soc/mediatek/mtk-cmdq.h>
#include <soc/mediatek/mtk_mmsys_cfg.h>
#include <soc/mediatek/mtk_ts.h>

#include "mtk_mmsys_test.h"

#define CV_IRQ_NR (MMSYSCFG_CAMERA_SYNC_MAX * 2)
#define CAM_VSYNC_0_EVENT	176

struct mtk_cv_thread_param {
	u32 irq_idx;
	void *dispsys;
};

struct mtk_mmsys_test_ctx {
	struct cmdq_client *client[CV_IRQ_NR];
	wait_queue_head_t wq[CV_IRQ_NR];
	atomic_t start[CV_IRQ_NR];
	struct task_struct *cv_we_task[CV_IRQ_NR];
	struct mtk_cv_thread_param cv_thread_param[CV_IRQ_NR];
	u32 wait_event_cnt[CV_IRQ_NR];
};

static int mtk_cv_wait_event_kthread(void *data)
{
	struct mtk_cv_thread_param *param = (struct mtk_cv_thread_param *)data;
	struct mtk_dispsys *dispsys = (struct mtk_dispsys *)param->dispsys;
	struct mtk_mmsys_test_ctx *ctx;
	u32 event_idx = param->irq_idx;
	u32 event = CAM_VSYNC_0_EVENT + event_idx;
	struct cmdq_pkt *pkt;
	int ret = 0;

	ctx = (struct mtk_mmsys_test_ctx *)dispsys->mmsys_test;

	pr_err("%s[%u]: event=%d\n", __func__, event_idx, event);

	cmdq_pkt_create(&pkt);
	cmdq_pkt_clear_event(pkt, event);
	cmdq_pkt_wfe(pkt, event);

	while (1) {
		ret = wait_event_interruptible(ctx->wq[event_idx],
				atomic_read(&ctx->start[event_idx]));
		if (ret) {
			pr_info("%s[%u]:  wait event failure(%d)\n",
				__func__, event_idx, ret);
			break;
		}
		atomic_set(&ctx->start[event_idx], 0);

		while (ctx->wait_event_cnt[event_idx]--) {
			ret = cmdq_pkt_flush(ctx->client[event_idx], pkt);
			if (ret)
				pr_err("~cam_vsync[%u] wait event %u timeout, remain[%u]\n",
					event_idx, event,
					ctx->wait_event_cnt[event_idx]);
			else
				pr_err("~cam_vsync[%u] got event %u, remain[%u]\n",
					event_idx, event,
					ctx->wait_event_cnt[event_idx]);
		}

		if (kthread_should_stop())
			break;
	}

	cmdq_pkt_destroy(pkt);

	return 0;
}

static void mtk_mmsys_test_init(struct mtk_dispsys *dispsys, int gce_thr_id)
{
	u32 i;
	char thread_name[20];
	struct mtk_mmsys_test_ctx *ctx;

	if (dispsys->mmsys_test) {
		pr_err("mmsys core test context has been initialized!\n");
		return;
	}

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return;

	dispsys->mmsys_test = ctx;

	for (i = 0; i < CV_IRQ_NR; i++) {
		ctx->client[i] = cmdq_mbox_create(dispsys->dev, i + gce_thr_id);
		if (IS_ERR(ctx->client[i])) {
			pr_err("%s: create cmdq mbox[%u] failed (%ld)!\n",
				__func__, i, PTR_ERR(ctx->client[i]));
			if (i > 0) {
				u32 j;

				for (j = 0; j < i; j++)
					cmdq_mbox_destroy(ctx->client[j]);
			}
			dispsys->mmsys_test = NULL;
			return;
		}

		ctx->cv_thread_param[i].irq_idx = i;
		ctx->cv_thread_param[i].dispsys = (void *)dispsys;
		init_waitqueue_head(&ctx->wq[i]);
		atomic_set(&ctx->start[i], 0);
		sprintf(thread_name, "cv_we_thread%u", i);
		ctx->cv_we_task[i] = kthread_create(mtk_cv_wait_event_kthread,
					&ctx->cv_thread_param[i], thread_name);
		wake_up_process(ctx->cv_we_task[i]);
	}

	pr_err("mmsys core test context is initialized!\n");
}

static void mtk_cv_irq_cb(struct mtk_mmsys_cb_data *data)
{
	u32 i, vts, dp_counter;
	u32 irq_sta = data->status;
	struct mtk_dispsys *dispsys = data->priv_data;
	struct device *ts_dev = dispsys->disp_dev[MTK_DISP_TIMESTAMP];

	for (i = 0 ; i < CV_IRQ_NR; i++) {
		if ((irq_sta & BIT(i)) == 0)
			continue;

		switch (i%MMSYSCFG_CAMERA_SYNC_MAX) {
		case MMSYSCFG_CAMERA_SYNC_SIDE01:
			mtk_ts_get_side_0_camera_sync_time(ts_dev, &vts,
							&dp_counter);
			break;
		case MMSYSCFG_CAMERA_SYNC_SIDE23:
			mtk_ts_get_side_1_camera_sync_time(ts_dev, &vts,
							&dp_counter);
			break;
		case MMSYSCFG_CAMERA_SYNC_GAZE0:
			mtk_ts_get_gaze_0_camera_sync_time(ts_dev, &vts,
							&dp_counter);
			break;
		case MMSYSCFG_CAMERA_SYNC_GAZE1:
			mtk_ts_get_gaze_1_camera_sync_time(ts_dev, &vts,
							&dp_counter);
			break;
		case MMSYSCFG_CAMERA_SYNC_GAZE_LED:
		default:
			mtk_ts_get_gaze_2_camera_sync_time(ts_dev, &vts,
							&dp_counter);
			break;
		}

		if ((dp_counter & 0xffff) != ((dp_counter >> 16) & 0xffff)) {
			pr_err("cam_vsync[%d] vts=%u, dp_counter=0x%08x NG\n",
				i, vts, dp_counter);
		} else {
			pr_err("cam_vsync[%d] vts=%u, dp_counter=0x%08x\n",
				i, vts, dp_counter);
		}
	}
}

static int mtk_cv_enable(struct device *dev, u32 camerea_id, u32 vsync_cycle,
			 u32 delay_cycle, u8 n, u8 m, bool vsync_low_active)
{
	struct cmdq_pkt **handle = NULL;

	mtk_mmsys_cfg_camera_sync_clock_sel(dev, true);
	mtk_mmsys_cfg_camera_sync_config(dev, camerea_id, vsync_cycle,
					delay_cycle, vsync_low_active, handle);
	mtk_mmsys_cfg_camera_sync_frc(dev, camerea_id, n, m, handle);
	mtk_mmsys_cfg_camera_sync_enable(dev, camerea_id, handle);

	return 0;
}

static int mtk_cv_disable(struct device *dev, u32 camerea_id)
{
	struct cmdq_pkt **handle = NULL;

	mtk_mmsys_cfg_camera_sync_disable(dev, camerea_id, handle);

	return 0;
}

static void mtk_mmsys_camera_vsync_test(struct mtk_dispsys *dispsys, char *buf)
{
	int ret;
	struct device *dev;

	if (!dispsys || !buf)
		return;

	dev = dispsys->disp_dev[MTK_DISP_MMSYS_CFG];

	if (strncmp(buf, "init:", 5) == 0) {
		int thread_start = -1;

		ret = sscanf(buf, "init: %d", &thread_start);
		pr_err("parse cv init param(%d), %d\n", ret, thread_start);
		if (ret <= 0 || thread_start < 0 || thread_start >= 21) {
			pr_err("invalid parameters\n");
			return;
		}
		mtk_mmsys_test_init(dispsys, thread_start);
	} else if (strncmp(buf, "dsi_ctrl:", 9) == 0) {
		int dsi_vsync_high_acive = 0;

		ret = sscanf(buf, "dsi_ctrl: %d", &dsi_vsync_high_acive);
		pr_err("parse cv dsi control param(%d), %d\n",
			 ret, dsi_vsync_high_acive);
		ret = mtk_mmsys_cfg_camera_sync_dsi_control(dev,
						(bool)dsi_vsync_high_acive);
	} else if (strncmp(buf, "e:", 2) == 0) {
		u32 id = 0, vsync_cycle = 0, delay_cycle = 0, irq_status = 0;
		u32 irq_cb_last_time = 1; /* second */
		u32 n = 32, m = 32;
		int vsync_low_active = false;

		ret = sscanf(buf, "e: %u %u %u %u %u %x %u %d", &id,
			&vsync_cycle, &delay_cycle, &n, &m, &irq_status,
			&irq_cb_last_time, &vsync_low_active);
		pr_err("parse cv en param(%d), %u %u %u %u %u 0x%x %u %d\n",
			ret, id, vsync_cycle, delay_cycle, n, m, irq_status,
			irq_cb_last_time, vsync_low_active);

		mtk_mmsys_cfg_camera_sync_register_cb(dev, mtk_cv_irq_cb,
						  irq_status, dispsys);
		pr_err("camera vsync irq cb enabled!\n");
		mtk_cv_enable(dev, id, vsync_cycle, delay_cycle, (u8)n, (u8)m,
			      (bool)vsync_low_active);
		msleep_interruptible(irq_cb_last_time * 1000);
		mtk_mmsys_cfg_camera_sync_register_cb(dev, NULL, 0, dispsys);
		pr_err("camera vsync irq cb disabled!\n");
	} else if (strncmp(buf, "ee:", 3) == 0) {
		struct mtk_mmsys_test_ctx *ctx;
		u32 id = 0, vsync_cycle = 0, delay_cycle = 0;
		u32 wait_event_cnt = 1;
		u32 n = 32, m = 32;
		int vsync_low_active = false;

		ctx = (struct mtk_mmsys_test_ctx *)dispsys->mmsys_test;
		if (!ctx) {
			pr_err("mmsys test has not been initialized yet!\n");
			return;
		}

		ret = sscanf(buf, "ee: %u %u %u %u %u %u %d", &id,
			&vsync_cycle, &delay_cycle, &n, &m, &wait_event_cnt,
			&vsync_low_active);
		pr_err("parse cv wait event param(%d),%u %u %u %u %u %u %d\n",
			ret, id, vsync_cycle, delay_cycle, n, m,
			wait_event_cnt,	vsync_low_active);

		if (id >= MMSYSCFG_CAMERA_SYNC_MAX)
			return;

		mtk_cv_enable(dev, id, vsync_cycle, delay_cycle, (u8)n, (u8)m,
			      (bool)vsync_low_active);

		/* vsync start */
		ctx->wait_event_cnt[id] = wait_event_cnt;
		atomic_set(&ctx->start[id], 1);
		wake_up(&ctx->wq[id]);
		/* vsync tail */
		id += MMSYSCFG_CAMERA_SYNC_MAX;
		ctx->wait_event_cnt[id] = wait_event_cnt;
		atomic_set(&ctx->start[id], 1);
		wake_up(&ctx->wq[id]);
	} else if (strncmp(buf, "r:", 2) == 0) {
		int id;
		struct mtk_mmsys_cb_data data;

		ret = sscanf(buf, "r: %d", &id);
		pr_err("parse cv read parameter(%d), %d\n", ret, id);
		data.priv_data = dispsys;
		data.part_id = 0;
		data.status = BIT(id) | BIT(id + MMSYSCFG_CAMERA_SYNC_MAX);

		mtk_cv_irq_cb(&data);

	} else if (strncmp(buf, "d:", 2) == 0) {
		int id = 0;

		ret = sscanf(buf, "d: %d", &id);
		pr_err("parse cv disable parameter(%d), %d\n", ret, id);
		mtk_cv_disable(dev, id);
	}
}

static char *s_crc_out_module[] = {
	"slcr_vid_0_to_crop",
	"slcr_vid_1_to_crop",
	"slcr_vid_2_to_crop",
	"slcr_vid_3_to_crop",
	"crop_0_to_rsz",
	"crop_1_to_rsz",
	"rsz_0_to_p2s_0",
	"rsz_1_to_p2s_0",
	"wdma_in_0",
	"wdma_in_1",
	"wdma_in_2",
	"wdma_in_3",
	"rdma_in_0",
	"rdma_in_1",
	"rdma_in_2",
	"rdma_in_3",
	"lhc_in_0",
	"lhc_in_1",
	"lhc_in_2",
	"lhc_in_3",
	"pat_0_to_dsc",
	"pat_1_to_dsc",
	"pat_2_to_dsc",
	"pat_3_to_dsc",
	"dsi_swap_0_to_digital",
	"dsi_swap_1_to_digital",
	"dsi_swap_2_to_digital",
	"dsi_swap_3_to_digital",
	"rdma_0_to_pat",
	"rdma_1_to_pat",
	"rdma_2_to_pat",
	"rdma_3_to_pat"
};

static void mtk_mmsys_crc_test(struct mtk_dispsys *dispsys, char *buf)
{
	int ret;
	u32 flags;
	struct device *dev;

	if (!dispsys || !buf)
		return;

	dev = dispsys->disp_dev[MTK_DISP_MMSYS_CFG];

	if (strncmp(buf, "e:", 2) == 0) {
		ret = sscanf(buf, "e: %x", &flags);
		pr_err("parse CRC enable parameter(%d), 0x%x\n", ret, flags);
		ret = mtk_mmsys_cfg_crc_enable(dev, flags);
		if (ret != 0)
			pr_err("mmsys CRC enable failed (%d)\n", ret);
	} else if (strncmp(buf, "d:", 2) == 0) {
		ret = sscanf(buf, "d: %x", &flags);
		pr_err("parse CRC disable parameter(%d), 0x%x\n", ret, flags);
		ret = mtk_mmsys_cfg_crc_disable(dev, flags);
		if (ret != 0)
			pr_err("mmsys CRC disable failed (%d)\n", ret);
	} else if (strncmp(buf, "c:", 2) == 0) {
		ret = sscanf(buf, "c: %x", &flags);
		pr_err("parse CRC clear parameter(%d), 0x%x\n", ret, flags);
		ret = mtk_mmsys_cfg_crc_clear(dev, flags);
		if (ret != 0)
			pr_err("mmsys CRC clear failed (%d)\n", ret);
	} else if (strncmp(buf, "r:", 2) == 0) {
		u32 *crc_out, *crc_num;
		u32 mod_cnt = 0;
		u32 i, j;

		ret = sscanf(buf, "r: %x %u", &flags, &mod_cnt);
		if (mod_cnt == 0 || mod_cnt > 32)
			mod_cnt = 32;
		pr_err("parse CRC read parameter(%d), 0x%x %u\n", ret, flags,
			mod_cnt);

		crc_out = kcalloc(mod_cnt, sizeof(u32), GFP_KERNEL);
		if (!crc_out)
			return;
		crc_num = kcalloc(mod_cnt, sizeof(u32), GFP_KERNEL);
		if (!crc_num) {
			kfree(crc_out);
			return;
		}

		ret = mtk_mmsys_cfg_crc_read(dev, flags, crc_out, crc_num,
					     mod_cnt);
		if (ret != 0)
			pr_err("mmsys CRC read failed (%d)\n", ret);

		for (i = 0, j = 0; i < 32; i++) {
			if ((flags & BIT(i)) == 0)
				continue;
			pr_err("CRC_OUT[%u]=%08x, CRC_NUM[%u]=%08x (%s)\n",
				 i, crc_out[j], i, crc_num[j],
				 s_crc_out_module[i]);
			j++;
		}

		kfree(crc_out);
		kfree(crc_num);
	} else if (strncmp(buf, "h", 1) == 0) {
		u32 i, cnt;

		cnt = ARRAY_SIZE(s_crc_out_module);
		pr_err("MMSYS CRC modules list:\n");
		for (i = 0; i < cnt; i++)
			pr_err("CRC_OUT[%u]: %s\n", i, s_crc_out_module[i]);

	} else {
		pr_err("not supported command: %s\n", buf);
	}
}

static void mtk_mmsys_pat_gen_test(struct mtk_dispsys *dispsys, char *buf)
{
	int ret;
	int id = 0;
	struct device *dev;

	if (!dispsys || !buf)
		return;

	dev = dispsys->disp_dev[MTK_DISP_MMSYS_CFG];

	if (strncmp(buf, "e:", 2) == 0) {
		int force_valid = 1;

		ret = sscanf(buf, "e: %d %d", &id, &force_valid);
		pr_err("pat gen enable parameter(%d), %d %d\n",
			ret, id, force_valid);
		ret = mtk_mmsys_cfg_pat_gen_eanble(dev, id, (bool)force_valid,
						NULL);
		if (ret != 0)
			pr_err("mmsys pat gen enable failed (%d)\n", ret);
	} else if (strncmp(buf, "d:", 2) == 0) {
		ret = sscanf(buf, "d: %d", &id);
		pr_err("pat gen disable parameter(%d), %d\n", ret, id);
		ret = mtk_mmsys_cfg_pat_gen_disable(dev, id, NULL);
		if (ret != 0)
			pr_err("mmsys pat gen disable failed (%d)\n", ret);
	} else if (strncmp(buf, "fs:", 3) == 0) {
		u32 w = 0, h = 0;

		ret = sscanf(buf, "fs: %d %u %u", &id, &w, &h);
		pr_err("pat gen frame size parameter(%d), %d %u %u\n",
			ret, id, w, h);
		ret = mtk_mmsys_cfg_pat_config_frame_size(dev, id, w, h, NULL);
		if (ret != 0)
			pr_err("mmsys pat gen config frame size failed (%d)\n",
				ret);
	} else if (strncmp(buf, "bs:", 3) == 0) {
		u32 w = 0, h = 0;

		ret = sscanf(buf, "bs: %d %u %u", &id, &w, &h);
		pr_err("pat gen border size parameter(%d), %d %u %u\n",
			ret, id, w, h);
		ret = mtk_mmsys_cfg_pat_config_border_size(dev, id, w, h, NULL);
		if (ret != 0)
			pr_err("mmsys pat gen config border size failed (%d)\n",
				ret);
	} else if (strncmp(buf, "fg:", 3) == 0) {
		u32 y = 0, u = 0, v = 0;

		ret = sscanf(buf, "fg: %d %u %u %u", &id, &y, &u, &v);
		pr_err("pat gen fg color parameter(%d), %d %u %u %u\n",
			ret, id, y, u, v);
		ret = mtk_mmsys_cfg_pat_config_fg_color(dev, id, y, u, v, NULL);
		if (ret != 0)
			pr_err("mmsys pat gen config fg color failed (%d)\n",
				ret);
	} else if (strncmp(buf, "bg:", 3) == 0) {
		u32 y = 0, u = 0, v = 0;

		ret = sscanf(buf, "bg: %d %u %u %u", &id, &y, &u, &v);
		pr_err("pat gen bg color parameter(%d), %d %u %u %u\n",
			ret, id, y, u, v);
		ret = mtk_mmsys_cfg_pat_config_bg_color(dev, id, y, u, v, NULL);
		if (ret != 0)
			pr_err("mmsys pat gen config bg color failed (%d)\n",
				ret);
	} else if (strncmp(buf, "tpc:", 4) == 0) {
		ret = sscanf(buf, "tpc: %d", &id);
		pr_err("pat gen pure color parameter(%d), %d\n", ret, id);
		ret = mtk_mmsys_cfg_pat_type_sel_pure_color(dev, id, NULL);
		if (ret != 0)
			pr_err("mmsys pat gen sel pure color failed (%d)\n",
				ret);
	} else if (strncmp(buf, "tc:", 3) == 0) {
		u32 show = 1, x = 0, y = 0;

		ret = sscanf(buf, "tc: %d %u %u %u", &id, &show, &x, &y);
		pr_err("pat gen cursor parameter(%d), %d %u %u %u\n",
			ret, id, show, x, y);
		ret = mtk_mmsys_cfg_pat_type_sel_cursor(dev, id, (bool)show,
							x, y, NULL);
		if (ret != 0)
			pr_err("mmsys pat gen sel cursor failed (%d)\n",
				ret);
	} else if (strncmp(buf, "tr:", 3) == 0) {
		u32 y = 0, u = 0, v = 0, w = 0, step = 0;

		ret = sscanf(buf, "tr: %d %u %u %u %u %u", &id, &y, &u, &v,
				&w, &step);
		pr_err("pat gen ramp parameter(%d), %d %u %u %u %u %u\n",
			ret, id, y, u, v, w, step);
		ret = mtk_mmsys_cfg_pat_type_sel_ramp(dev, id, (bool)y, (bool)u,
						(bool)v, w, step, NULL);
		if (ret != 0)
			pr_err("mmsys pat gen sel ramp failed (%d)\n", ret);
	} else if (strncmp(buf, "tg:", 3) == 0) {
		u32 s = 1, b = 1, g = 1;

		ret = sscanf(buf, "tg: %d %u %u %u", &id, &s, &b, &g);
		pr_err("pat gen grid parameter(%d), %d %u %u %u\n",
			ret, id, s, b, g);
		ret = mtk_mmsys_cfg_pat_type_sel_grid(dev, id, s,
						(bool)b, (bool)g, NULL);
		if (ret != 0)
			pr_err("mmsys pat gen sel grid failed (%d)\n", ret);
	} else {
		pr_err("not supported command: %s\n", buf);
	}
}

static void mtk_mmsys_dsi_swap_test(struct mtk_dispsys *dispsys, char *buf)
{
	int ret;
	u32 to_dsi[4] = {0, 0, 0, 0};
	struct device *dev;

	if (!dispsys || !buf)
		return;

	dev = dispsys->disp_dev[MTK_DISP_MMSYS_CFG];

	ret = sscanf(buf, "%u %u %u %u", &to_dsi[0], &to_dsi[1], &to_dsi[2],
			&to_dsi[3]);
	pr_err("dsi lane swap parameter(%d), %u %u %u %u\n",
		ret, to_dsi[0], to_dsi[1], to_dsi[2], to_dsi[3]);
	ret = mtk_mmsys_cfg_dsi_lane_swap_config(dev, to_dsi[0], to_dsi[1],
					to_dsi[2], to_dsi[3], NULL);
	if (ret != 0)
		pr_err("mmsys dsi lane swap failed (%d)\n", ret);
}

static void mtk_mmsys_lhc_swap_test(struct mtk_dispsys *dispsys, char *buf)
{
	int ret;
	u32 to_lhc[4] = {0, 0, 0, 0};
	struct device *dev;

	if (!dispsys || !buf)
		return;

	dev = dispsys->disp_dev[MTK_DISP_MMSYS_CFG];

	ret = sscanf(buf, "%u %u %u %u", &to_lhc[0], &to_lhc[1], &to_lhc[2],
			&to_lhc[3]);
	pr_err("lhc swap parameter(%d), %u %u %u %u\n",
		ret, to_lhc[0], to_lhc[1], to_lhc[2], to_lhc[3]);
	ret = mtk_mmsys_cfg_lhc_swap_config(dev, to_lhc[0], to_lhc[1],
					to_lhc[2], to_lhc[3], NULL);
	if (ret != 0)
		pr_err("mmsys lhc swap failed (%d)\n", ret);
}

void mtk_mmsys_test(struct mtk_dispsys *dispsys, char *buf)
{
	size_t len;

	if (!dispsys || !buf)
		return;
	len = strlen(buf);
	if (len > 3 && strncmp(buf, "cv:", 3) == 0)
		mtk_mmsys_camera_vsync_test(dispsys, buf + 3);
	else if (len > 4 && strncmp(buf, "crc:", 4) == 0)
		mtk_mmsys_crc_test(dispsys, buf + 4);
	else if (len > 4 && strncmp(buf, "pat:", 4) == 0)
		mtk_mmsys_pat_gen_test(dispsys, buf + 4);
	else if (len > 9 && strncmp(buf, "dsi_swap:", 9) == 0)
		mtk_mmsys_dsi_swap_test(dispsys, buf + 9);
	else if (len > 9 && strncmp(buf, "lhc_swap:", 9) == 0)
		mtk_mmsys_lhc_swap_test(dispsys, buf + 9);
	else
		pr_err("not supported command: %s\n", buf);
}
