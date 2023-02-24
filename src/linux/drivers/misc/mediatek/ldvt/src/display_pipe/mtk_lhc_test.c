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

#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/time.h>
#include <soc/mediatek/mtk_lhc.h>

#include "mtk_lhc_test.h"

struct mtk_lhc_test_ctx {
	wait_queue_head_t lhc_wq;
	int lhc_frame_done;
	struct mtk_lhc_color_trans ct;
};

static void mtk_lhc_test_init(struct mtk_dispsys *dispsys)
{
	struct mtk_lhc_test_ctx *ctx;

	if (!dispsys) {
		pr_err("Invalid parameter @%s, %d\n", __func__, __LINE__);
		return;
	}

	if (dispsys->lhc_test) {
		pr_debug("lhc test context has been initialized!\n");
		return;
	}

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	init_waitqueue_head(&ctx->lhc_wq);

	dispsys->lhc_test = ctx;

	pr_debug("lhc test context is initialized!\n");
}

static void mtk_lhc_test_deinit(struct mtk_dispsys *dispsys)
{
	if (!dispsys) {
		pr_err("Invalid parameter @%s, %d\n", __func__, __LINE__);
		return;
	}

	if (!dispsys->lhc_test) {
		pr_debug("lhc test has not been initialized yet!\n");
		return;
	}

	kfree(dispsys->lhc_test);
	dispsys->lhc_test = NULL;

	pr_debug("lhc test context is de-initialized!\n");
}

static void mtk_lhc_test_irq_status_cb(struct mtk_mmsys_cb_data *data)
{
	struct mtk_lhc_test_ctx *ctx;
	struct mtk_dispsys *dispsys = data->priv_data;

	if (!dispsys) {
		pr_err("Invalid parameter @%s, %d\n", __func__, __LINE__);
		return;
	}

	ctx = (struct mtk_lhc_test_ctx *)dispsys->lhc_test;
	pr_debug("~lhc_cb: data.part_id=0x%02x, status=0x%02x\n",
		data->part_id, data->status);

	if (data->part_id == (1 << dispsys->disp_partition_nr) - 1 &&
	    data->status == LHC_CB_STA_FRAME_DONE) {
		ctx->lhc_frame_done = 1;
		wake_up(&ctx->lhc_wq);
	}
}

static void mtk_lhc_test_print_histogram(struct mtk_lhc_hist *d)
{
	int y, x, b;

	if (!d) {
		pr_err("Invalid parameter @%s, %d\n", __func__, __LINE__);
		return;
	}

	for (y = 0; y < LHC_BLK_Y_NUM; y++) {
		for (x = 0; x < LHC_BLK_X_NUM * d->module_num; x++) {
			pr_debug("==========blk_y: %d  blk_x: %d==========\n",
				y, x);
			pr_debug("%13c%8c%8c\n", 'R', 'G', 'B');
			for (b = 0; b < LHC_BLK_BIN_NUM; b++) {
				pr_debug("%5d%8d%8d%8d\n",
					b, d->r[y][x][b], d->g[y][x][b],
					d->b[y][x][b]);
			}
		}
	}
}

static void time_diff(struct timeval *t1, struct timeval *t2,
			struct timeval *diff)
{
	if (t2->tv_usec >= t1->tv_usec) {
		if (t2->tv_sec >= t1->tv_sec) {
			diff->tv_sec = t2->tv_sec - t1->tv_sec;
			diff->tv_usec = t2->tv_usec - t1->tv_usec;
		} else {
			diff->tv_sec = t1->tv_sec - t2->tv_sec - 1;
			diff->tv_usec = t1->tv_usec + 1 * 1000 * 1000
					- t2->tv_usec;
		}
	} else {
		if (t1->tv_sec >= t2->tv_sec) {
			diff->tv_sec = t1->tv_sec - t2->tv_sec;
			diff->tv_usec = t1->tv_usec - t2->tv_usec;
		} else {
			diff->tv_sec = t2->tv_sec - t1->tv_sec - 1;
			diff->tv_usec = t2->tv_usec + 1 * 1000 * 1000
					- t1->tv_usec;
		}
	}
}

static void mtk_lhc_test_read_histogram(struct mtk_dispsys *dispsys,
				u32 irq_status, u32 cb_times, u32 log_times)
{
	struct mtk_lhc_test_ctx *ctx;
	struct device *dev;
	struct device *lhc_dev;
	struct mtk_lhc_hist *lhc_data;
	int ret = 0;
	struct timeval t1, t2, t3;

	if (!dispsys) {
		pr_err("Invalid parameter @%s, %d\n", __func__, __LINE__);
		return;
	}

	dev = dispsys->dev;
	lhc_dev = dispsys->disp_dev[MTK_DISP_LHC];

	if (!dispsys->lhc_test) {
		pr_debug("lhc test has not been initialized yet!\n");
		return;
	}
	if (cb_times == 0) {
		pr_debug("lhc callback times is 0, set to 1\n");
		cb_times = 1;
	}

	ctx = (struct mtk_lhc_test_ctx *)dispsys->lhc_test;

	lhc_data = kzalloc(sizeof(*lhc_data), GFP_KERNEL);
	if (lhc_data == NULL)
		return;

	ret = mtk_lhc_register_cb(lhc_dev, mtk_lhc_test_irq_status_cb,
				  irq_status, dispsys);

	while (cb_times > 0 || log_times > 0) {
		ret = wait_event_interruptible_timeout(ctx->lhc_wq,
						       ctx->lhc_frame_done,
						       msecs_to_jiffies(5000));
		ctx->lhc_frame_done = 0;

		dev_dbg(dev, "lhc wait ret=%d, cb_times=%u, log_times=%u\n",
			ret, cb_times, log_times);

		if (ret != 0 && ret != (-ERESTARTSYS)) {
			do_gettimeofday(&t1);
			ret = mtk_lhc_read_histogram(lhc_dev, lhc_data);
			do_gettimeofday(&t2);
			time_diff(&t1, &t2, &t3);
			pr_info("[LHC] read histogram takes %lu.%03lu ms\n",
				t3.tv_sec * 1000 + t3.tv_usec / 1000,
				t3.tv_usec % 1000);
			if (ret == 0 && log_times > 0)
				mtk_lhc_test_print_histogram(lhc_data);
		}

		if (cb_times > 0)
			cb_times--;
		if (log_times > 0)
			log_times--;
	}

	ret = mtk_lhc_register_cb(lhc_dev, NULL, 0, NULL);
	kfree(lhc_data);
}

static void mtk_lhc_test_sram_rw_test(struct mtk_dispsys *dispsys,
	u32 seed, int module_cnt)
{
	struct device *lhc_dev;
	struct mtk_lhc_hist *lhc_wd = NULL;
	struct mtk_lhc_hist *lhc_rd = NULL;
	int ret = 0;
	int y, x, b = 0;
	int lhc_blk_x_num_total;

	if (!dispsys) {
		pr_err("Invalid parameter @%s, %d\n", __func__, __LINE__);
		return;
	}

	lhc_dev = dispsys->disp_dev[MTK_DISP_LHC];

	lhc_wd = kzalloc(sizeof(*lhc_wd), GFP_KERNEL);
	if (lhc_wd == NULL)
		return;
	lhc_rd = kzalloc(sizeof(*lhc_rd), GFP_KERNEL);
	if (lhc_rd == NULL) {
		kfree(lhc_wd);
		return;
	}

	lhc_rd->module_num = module_cnt;
	lhc_wd->module_num = module_cnt;
	lhc_blk_x_num_total = LHC_BLK_X_NUM * lhc_wd->module_num;
	for (y = 0; y < LHC_BLK_Y_NUM; y++) {
		for (x = 0; x < lhc_blk_x_num_total; x++) {
			for (b = 0; b < LHC_BLK_BIN_NUM; b++) {
				lhc_wd->r[y][x][b] = ((seed >> b) + x) & 0xFF;
				lhc_wd->g[y][x][b] = ((seed >> b) * x) & 0xFF;
				lhc_wd->b[y][x][b] = ((seed >> b) + b) & 0xFF;
			}
		}
	}

	ret = mtk_lhc_write_histogram(lhc_dev, lhc_wd);
	ret = mtk_lhc_read_histogram(lhc_dev, lhc_rd);

	for (y = 0; y < LHC_BLK_Y_NUM; y++) {
		for (x = 0; x < lhc_blk_x_num_total; x++) {
			for (b = 0; b < LHC_BLK_BIN_NUM; b++) {
				if (lhc_rd->r[y][x][b] != lhc_wd->r[y][x][b] ||
				    lhc_rd->g[y][x][b] != lhc_wd->g[y][x][b] ||
				    lhc_rd->b[y][x][b] != lhc_wd->b[y][x][b]) {
					pr_err("LHC SRAM R/W test failed\n");
					pr_err("LHC writed data:\n");
					mtk_lhc_test_print_histogram(lhc_wd);
					pr_err("LHC read data:\n");
					mtk_lhc_test_print_histogram(lhc_rd);
					x = LHC_BLK_X_NUM;
					y = LHC_BLK_Y_NUM;
					break;
				}
			}
		}
	}

	if (b == LHC_BLK_BIN_NUM)
		pr_debug("LHC SRAM R/W test pass.\n");

	kfree(lhc_wd);
	kfree(lhc_rd);
}

static void mtk_lhc_test_set_color_transform(struct mtk_dispsys *dispsys,
					u32 y2r_en, u32 external_matrix_en)
{
	struct mtk_lhc_test_ctx *ctx;
	struct device *lhc_dev;

	if (!dispsys) {
		pr_err("Invalid parameter @%s, %d\n", __func__, __LINE__);
		return;
	}

	lhc_dev = dispsys->disp_dev[MTK_DISP_LHC];
	ctx = (struct mtk_lhc_test_ctx *)dispsys->lhc_test;

	if (y2r_en == 1)
		ctx->ct.enable = 1;
	else
		ctx->ct.enable = 0;

	if (external_matrix_en == 1)
		ctx->ct.external_matrix = 1;
	else
		ctx->ct.external_matrix = 0;

	if (ctx->ct.enable == 1 &&
	    ctx->ct.external_matrix == 0 &&
	    ctx->ct.int_matrix >= LHC_INT_TBL_MAX) {
		pr_err("Please set valid internal matrix table firstly!\n");
		return;
	}

	mtk_lhc_set_color_transform(lhc_dev, &ctx->ct, NULL);

	if (ctx->ct.enable) {
		pr_debug("LHC Y2R conversion enabled.\n");
		if (ctx->ct.external_matrix == 0) {
			pr_debug("LHC use internal matrix: %u\n",
				ctx->ct.int_matrix);
		} else {
			pr_debug("LHC use external matrix:\n");
			pr_debug("y2r_c00:%20u\n", ctx->ct.coff[0][0]);
			pr_debug("y2r_c01:%20u\n", ctx->ct.coff[0][1]);
			pr_debug("y2r_c02:%20u\n", ctx->ct.coff[0][2]);
			pr_debug("y2r_c10:%20u\n", ctx->ct.coff[1][0]);
			pr_debug("y2r_c11:%20u\n", ctx->ct.coff[1][1]);
			pr_debug("y2r_c12:%20u\n", ctx->ct.coff[1][2]);
			pr_debug("y2r_c20:%20u\n", ctx->ct.coff[2][0]);
			pr_debug("y2r_c21:%20u\n", ctx->ct.coff[2][1]);
			pr_debug("y2r_c22:%20u\n", ctx->ct.coff[2][2]);
			pr_debug("y2r_pre_add_0:%14u\n", ctx->ct.pre_add[0]);
			pr_debug("y2r_pre_add_1:%14u\n", ctx->ct.pre_add[1]);
			pr_debug("y2r_pre_add_2:%14u\n", ctx->ct.pre_add[2]);
			pr_debug("y2r_post_add_0:%14u\n", ctx->ct.post_add[0]);
			pr_debug("y2r_post_add_1:%14u\n", ctx->ct.post_add[1]);
			pr_debug("y2r_post_add_2:%14u\n", ctx->ct.post_add[2]);
		}
	} else {
		pr_debug("LHC Y2R conversion disabled.\n");
	}
}

void mtk_lhc_test(struct mtk_dispsys *dispsys, char *buf)
{
	int ret;

	if (!dispsys || !buf) {
		pr_err("Invalid parameter @%s, %d\n", __func__, __LINE__);
		return;
	}

	if (strncmp(buf, "init", 4) == 0) {
		mtk_lhc_test_init(dispsys);
	} else if (strncmp(buf, "deinit", 6) == 0) {
		mtk_lhc_test_deinit(dispsys);
	} else if (strncmp(buf, "rh:", 3) == 0) {
		u32 irq_status = LHC_CB_STA_FRAME_DONE;
		u32 cb_times = 1, log_times = 1;

		ret = sscanf(buf, "rh: %x %u %u",
				&irq_status, &cb_times,	&log_times);
		pr_debug("parse read histogram parameter(%d), 0x%x %u %u\n",
			ret, irq_status, cb_times, log_times);

		mtk_lhc_test_read_histogram(dispsys, irq_status, cb_times,
						log_times);
	} else if (strncmp(buf, "rw:", 3) == 0) {
		u32 rand_v;
		int module_cnt = 1;

		ret = sscanf(buf, "rw: %x %d", &rand_v, &module_cnt);
		pr_debug("parse sram rw parameter(%d), 0x%08x, %d\n", ret,
			rand_v, module_cnt);
		if (module_cnt != 1 && module_cnt != 4) {
			pr_err("Invalid parameter @%s, %d\n",
				__func__, __LINE__);
			return;
		}
		mtk_lhc_test_sram_rw_test(dispsys, rand_v, module_cnt);
	} else if (strncmp(buf, "im:", 3) == 0) {
		struct mtk_lhc_test_ctx *ctx;
		u32 idx;

		ret = sscanf(buf, "im: %x", &idx);
		pr_debug("parse internal matrix table index(%d), %u\n",
			ret, idx);

		ctx = (struct mtk_lhc_test_ctx *)dispsys->lhc_test;
		if (idx >= LHC_INT_TBL_MAX)
			pr_err("Invalid internal matirx table : %u\n", idx);
		else
			ctx->ct.int_matrix = idx;
	} else if (strncmp(buf, "em:", 3) == 0) {
		struct mtk_lhc_test_ctx *ctx;
		const char *fmt;

		ctx = (struct mtk_lhc_test_ctx *)dispsys->lhc_test;
		fmt =  "em: %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u";
		ret = sscanf(buf, fmt,
			&ctx->ct.coff[0][0], &ctx->ct.coff[0][1],
			&ctx->ct.coff[0][2],
			&ctx->ct.coff[1][0], &ctx->ct.coff[1][1],
			&ctx->ct.coff[1][2],
			&ctx->ct.coff[2][0], &ctx->ct.coff[2][1],
			&ctx->ct.coff[2][2],
			&ctx->ct.pre_add[0], &ctx->ct.pre_add[1],
			&ctx->ct.pre_add[2],
			&ctx->ct.post_add[0], &ctx->ct.post_add[1],
			&ctx->ct.post_add[2]);
		pr_debug("parse external matrix parameters (%d)\n", ret);
		if (ret < 15)
			pr_err("Invalid external matrix parameters\n");
	} else if (strncmp(buf, "y2r:", 4) == 0) {
		u32 y2r_en = 0, ext_en = 0;

		ret = sscanf(buf, "y2r: %u %u", &y2r_en, &ext_en);
		pr_debug("parse color transform parameter(%d), %u %u\n",
			ret, y2r_en, ext_en);

		mtk_lhc_test_set_color_transform(dispsys, y2r_en, ext_en);
	} else {
		pr_err("unsupported lhc command: %s", buf);
	}
}
