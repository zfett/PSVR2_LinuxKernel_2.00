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

#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/dma-buf.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <soc/mediatek/mtk_fm.h>
#include <soc/mediatek/mtk_mmsys_cmmn_top.h>

#include <mtk_fm_dev.h>

#define DUMP_BUFFER 1
#define ZNCC_THRESHOLD 0x7148
#define FILE_EOF_WIN 0
#define FLOW_TIMEOUT 2000 /* usleep_range(500, 1000) */
#define SIZE_PER_BLOCK 10
#define FMO_GOLEDN_PATH_LEN 200


/* hack kernel struct for CRC test */
struct mtk_fm {
	/** fm device node */
	struct device *dev;
	/** fm mdp clock node */
	struct clk *clk_mdp;
	/** fm register base */
	void __iomem *regs;
	/** fm register start address */
	u32 reg_base;
	/** gce subsys */
	u32 subsys;
	/** larb device */
	struct device *larbdev;
	/** spinlock for irq control */
	spinlock_t spinlock_irq;
	/** irq number */
	int irq;
	/** callback function */
	mtk_mmsys_cb cb_func;
	/** callback data */
	void *cb_priv_data;
	/** development stage */
	u32 valid_func;
	u32 blk_size;
};

#define MTK_MMSYS_CMMN_TOP_CLK_SET_NUM				8

struct mtk_mmsys_cmmn_top {
	/** mmsys_cmmn_top device node */
	struct device *dev;
	/** mmsys_cmmn_top clock node */
	struct clk *clk_sync[MTK_MMSYS_CMMN_TOP_CLK_SET_NUM];
	/** mmsys_cmmn_top clock node */
	struct clk *clk_dl[MTK_MMSYS_CMMN_TOP_CLK_SET_NUM];
	/** mmsys_cmmn_top register base */
	void __iomem *regs;
};

static void mtk_fm_cb(struct mtk_mmsys_cb_data *data)
{
	LOG(CV_LOG_DBG, "fm frame done,part_id: %d status: %d\n",
		data->part_id, data->status);
}

static void warpa_work_cb(struct cmdq_cb_data data)
{
	struct mtk_fm_dev *fm_dev = data.data;
	int ret;

	ret = cmdq_pkt_destroy(fm_dev->cmdq_pkt_warpa);
	if (ret)
		dev_err(fm_dev->dev, "cmdq_pkt_warpa_destroy fail!\n");
	fm_dev->cmdq_pkt_warpa = NULL;
	complete(&fm_dev->warpa_work_comp.cmplt);
}

static void fe_work_cb(struct cmdq_cb_data data)
{
	struct mtk_fm_dev *fm_dev = data.data;
	int ret;

	ret = cmdq_pkt_destroy(fm_dev->cmdq_pkt_fe);
	if (ret)
		dev_err(fm_dev->dev, "cmdq_pkt_fe_destroy fail!\n");
	fm_dev->cmdq_pkt_fe = NULL;
	complete(&fm_dev->fe_work_comp.cmplt);
}

static void fm_work_cb(struct cmdq_cb_data data)
{
	struct mtk_fm_dev *fm_dev = data.data;

	complete(&fm_dev->fm_work_comp.cmplt);
}

static void wen0_work_cb(struct cmdq_cb_data data)
{
	struct mtk_fm_dev *fm_dev = data.data;
	int ret;

	ret = cmdq_pkt_destroy(fm_dev->cmdq_pkt_wen0);
	if (ret)
		dev_err(fm_dev->dev, "cmdq_pkt_wen0_destroy fail!\n");
	fm_dev->cmdq_pkt_wen0 = NULL;
	complete(&fm_dev->wen0_work_comp.cmplt);
}

static void wen1_work_cb(struct cmdq_cb_data data)
{
	struct mtk_fm_dev *fm_dev = data.data;
	int ret;

	ret = cmdq_pkt_destroy(fm_dev->cmdq_pkt_wen1);
	if (ret)
		dev_err(fm_dev->dev, "cmdq_pkt_wen1_destroy fail!\n");
	fm_dev->cmdq_pkt_wen1 = NULL;
	complete(&fm_dev->wen1_work_comp.cmplt);
}

static void wen2_work_cb(struct cmdq_cb_data data)
{
	struct mtk_fm_dev *fm_dev = data.data;
	int ret;

	ret = cmdq_pkt_destroy(fm_dev->cmdq_pkt_wen2);
	if (ret)
		dev_err(fm_dev->dev, "cmdq_pkt_wen2_destroy fail!\n");
	fm_dev->cmdq_pkt_wen2 = NULL;
	complete(&fm_dev->wen2_work_comp.cmplt);
}

static void wen3_work_cb(struct cmdq_cb_data data)
{
	struct mtk_fm_dev *fm_dev = data.data;
	int ret;

	ret = cmdq_pkt_destroy(fm_dev->cmdq_pkt_wen3);
	if (ret)
		dev_err(fm_dev->dev, "cmdq_pkt_wen3_destroy fail!\n");
	fm_dev->cmdq_pkt_wen3 = NULL;
	complete(&fm_dev->wen3_work_comp.cmplt);
}

static void ren_work_cb(struct cmdq_cb_data data)
{
	struct mtk_fm_dev *fm_dev = data.data;
	int ret;

	ret = cmdq_pkt_destroy(fm_dev->cmdq_pkt_ren);
	if (ret)
		dev_err(fm_dev->dev, "cmdq_pkt_ren_destroy fail!\n");
	fm_dev->cmdq_pkt_ren = NULL;
	complete(&fm_dev->ren_work_comp.cmplt);
}

static void be0_work_cb(struct cmdq_cb_data data)
{
	struct mtk_fm_dev *fm_dev = data.data;
	int ret;

	ret = cmdq_pkt_destroy(fm_dev->cmdq_pkt_be0);
	if (ret)
		dev_err(fm_dev->dev, "cmdq_pkt_be0_destroy fail!\n");
	fm_dev->cmdq_pkt_be0 = NULL;
	complete(&fm_dev->be0_work_comp.cmplt);
}

static void be1_work_cb(struct cmdq_cb_data data)
{
	struct mtk_fm_dev *fm_dev = data.data;
	int ret;

	ret = cmdq_pkt_destroy(fm_dev->cmdq_pkt_be1);
	if (ret)
		dev_err(fm_dev->dev, "cmdq_pkt_be1_destroy fail!\n");
	fm_dev->cmdq_pkt_be1 = NULL;
	complete(&fm_dev->be1_work_comp.cmplt);
}


static void be2_work_cb(struct cmdq_cb_data data)
{
	struct mtk_fm_dev *fm_dev = data.data;
	int ret;

	ret = cmdq_pkt_destroy(fm_dev->cmdq_pkt_be2);
	if (ret)
		dev_err(fm_dev->dev, "cmdq_pkt_be2_destroy fail!\n");
	fm_dev->cmdq_pkt_be2 = NULL;
	complete(&fm_dev->be2_work_comp.cmplt);
}

static void be3_work_cb(struct cmdq_cb_data data)
{
	struct mtk_fm_dev *fm_dev = data.data;
	int ret;

	ret = cmdq_pkt_destroy(fm_dev->cmdq_pkt_be3);
	if (ret)
		dev_err(fm_dev->dev, "cmdq_pkt_be3_destroy fail!\n");
	fm_dev->cmdq_pkt_be3 = NULL;
	complete(&fm_dev->be3_work_comp.cmplt);
}


static void mtk_fm_dev_load_mask(
	void *va, char *filename, u32 w, u32 h)
{
	struct file *fp;
	mm_segment_t oldfs;
	loff_t pos = 0;
	u8 *line;
	int x, y;
	int blk_w = (w + 3) / 4;
	bool blk_w_pedding = false;
	u32 *va_addr = (u32 *)va;

	/*pr_err("demo_cv_read_msk: %s\n", filename);*/

	if ((blk_w % 2) == 1) {
		blk_w_pedding = true;
		blk_w++;
	}

	oldfs = get_fs();
	set_fs(get_ds());

	LOG(CV_LOG_DBG, "Open file %s\n", filename);
	fp = filp_open(filename, O_RDONLY, 0644);
	if (IS_ERR(fp)) {
		pr_err("fm_dev: open mask file %s fail!\n", filename);
		goto out;
	}

	line = kzalloc(w * 2, GFP_KERNEL);

	/*pr_err("blk_w %d blk_h %d, pedding %d\n", blk_w, h, blk_w % 2);*/

	for (y = 0; y < h; y++) {
		int blk_y = y / 8;
		int blk_y_off = y % 8;

		pos = y * (w * 2 + 1);
		vfs_read(fp, line, w * 2, &pos);
		for (x = 0; x < w; x++) {
			if (line[x * 2] == 49) {
				int blk_x = x / 4;
				int blk_idx = blk_y * blk_w + blk_x;
				int blk_x_off = x % 4;
				int blk_bit = blk_y_off * 4 + blk_x_off;

				va_addr[blk_idx] |= (1 << blk_bit);
			}
		}

		if (blk_w_pedding)
			va_addr[blk_y * blk_w + blk_w - 1] = 0;
	}
	filp_close(fp, 0);
	/*pr_err("Close file %s\n", filename);*/
	kfree((void *)line);
out:
	set_fs(oldfs);
}
static void mtk_fm_dev_compute_search_center(u32 *src, u32 *dst,
	dma_addr_t ref_frame, void *imu_data, u32 size)
{
	memcpy(dst, src, size * sizeof(u32));
}
static void mtk_fm_dev_buf_va(struct mtk_fm_dev *fm_dev, int flag)
{
	pa_to_vaddr_dev(fm_dev->dev_fm, fm_dev->buf[
			MTK_FM_DEV_BUF_TYPE_FE_L_POINT_R0],
			fm_dev->blk_nr * 8, flag);

	pa_to_vaddr_dev(fm_dev->dev_fm, fm_dev->buf[
			MTK_FM_DEV_BUF_TYPE_FE_L_DESC_R0], fm_dev->blk_nr * 64,
			flag);
	if (fm_dev->img_num > 1) {
		pa_to_vaddr_dev(fm_dev->dev_fm, fm_dev->buf[
				MTK_FM_DEV_BUF_TYPE_FE_R_POINT_R0],
				fm_dev->blk_nr * 8, flag);

		pa_to_vaddr_dev(fm_dev->dev_fm, fm_dev->buf[
				MTK_FM_DEV_BUF_TYPE_FE_R_DESC_R0],
				fm_dev->blk_nr * 64, flag);
	}

	pa_to_vaddr_dev(fm_dev->dev_fm, fm_dev->buf[MTK_FM_DEV_BUF_TYPE_FET_P],
			fm_dev->blk_nr * 8, flag);

	pa_to_vaddr_dev(fm_dev->dev_fm, fm_dev->buf[MTK_FM_DEV_BUF_TYPE_FET_D],
			fm_dev->blk_nr * 64, flag);

	pa_to_vaddr_dev(fm_dev->dev_fm, fm_dev->buf[
			MTK_FM_DEV_BUF_TYPE_WDMA_R0],
			fm_dev->buf[MTK_FM_DEV_BUF_TYPE_WDMA_R0]->pitch *
			fm_dev->fe_h, flag);
#if 0
	pa_to_vaddr_dev(fm_dev->dev_fm, fm_dev->buf[
		MTK_FM_DEV_BUF_TYPE_WDMA_R1],
		fm_dev->buf[MTK_FM_DEV_BUF_TYPE_WDMA_R1]->pitch * fm_dev->fe_h,
		flag);
#endif
	pa_to_vaddr_dev(fm_dev->dev_fm, fm_dev->buf[
			MTK_FM_DEV_BUF_TYPE_1_4_FET_P], fm_dev->blk_nr * 8,
			flag);

	pa_to_vaddr_dev(fm_dev->dev_fm, fm_dev->buf[
			MTK_FM_DEV_BUF_TYPE_1_4_FET_D], fm_dev->blk_nr * 64,
			flag);

	pa_to_vaddr_dev(fm_dev->dev_fm, fm_dev->buf[
			MTK_FM_DEV_BUF_TYPE_1_4_WDMA_R0],
			fm_dev->buf[MTK_FM_DEV_BUF_TYPE_1_4_WDMA_R0]->pitch *
			fm_dev->fe_h / 2, flag);

	pa_to_vaddr_dev(fm_dev->dev_fm, fm_dev->buf[
			MTK_FM_DEV_BUF_TYPE_1_4_WDMA_R1],
			fm_dev->buf[MTK_FM_DEV_BUF_TYPE_1_4_WDMA_R1]->pitch *
			fm_dev->fe_h / 2, flag);

	pa_to_vaddr_dev(fm_dev->dev_fm, fm_dev->buf[
			MTK_FM_DEV_BUF_TYPE_1_16_FET_P], fm_dev->blk_nr * 8,
			flag);

	pa_to_vaddr_dev(fm_dev->dev_fm, fm_dev->buf[
			MTK_FM_DEV_BUF_TYPE_1_16_FET_D], fm_dev->blk_nr * 64,
			flag);

	pa_to_vaddr_dev(fm_dev->dev_fm, fm_dev->buf[
			MTK_FM_DEV_BUF_TYPE_1_16_WDMA_R0],
			fm_dev->buf[MTK_FM_DEV_BUF_TYPE_1_16_WDMA_R0]->pitch *
			fm_dev->fe_h / 4, flag);

	pa_to_vaddr_dev(fm_dev->dev_fm, fm_dev->buf[
			MTK_FM_DEV_BUF_TYPE_1_16_WDMA_R1],
			fm_dev->buf[MTK_FM_DEV_BUF_TYPE_1_16_WDMA_R1]->pitch *
			fm_dev->fe_h / 4, flag);

	pa_to_vaddr_dev(fm_dev->dev_fm, fm_dev->buf[
			MTK_FM_DEV_BUF_TYPE_FM_ZNCC_S_R0], fm_dev->blk_nr * 64,
			flag);

	pa_to_vaddr_dev(fm_dev->dev_fm, fm_dev->buf[
			MTK_FM_DEV_BUF_TYPE_FM_ZNCC_T_R0], fm_dev->blk_nr * 64,
			flag);

	pa_to_vaddr_dev(fm_dev->dev_fm, fm_dev->buf[
			MTK_FM_DEV_BUF_TYPE_1_4_FM_ZNCC_T_R0],
			fm_dev->blk_nr * 64, flag);

	pa_to_vaddr_dev(fm_dev->dev_fm, fm_dev->buf[
			MTK_FM_DEV_BUF_TYPE_1_16_FM_ZNCC_T_R0],
			fm_dev->blk_nr * 64, flag);
}
static void mtk_fm_dev_compute_search_center_t(u32 *src, u32 *dst,
	dma_addr_t fmo1_t, dma_addr_t fmo2_t, u32 size)
{
	memcpy(dst, src, size * sizeof(u32));
}
static void mtk_fm_dev_load_search_center_common(
	void *va, char *filename, u32 blk_num, u32 offset)
{
	struct file *fp;
	mm_segment_t oldfs;
	loff_t pos = 0;
	u8 *line;
	u32 *va_addr = (u32 *)va;
	int i;
	int valid, x, y;
	int ret;

	/*pr_err("demo_cv_read_msk: %s\n", filename);*/

	oldfs = get_fs();
	set_fs(get_ds());

	LOG(CV_LOG_DBG,  "Open file %s\n", filename);
	fp = filp_open(filename, O_RDONLY, 0644);
	if (IS_ERR(fp)) {
		pr_err("fm_dev: open sc file %s fail!\n", filename);
		goto out;
	}

	line = kzalloc(14, GFP_KERNEL);
	for (i = 0; i < blk_num; i++) {
		pos = (loff_t)i * offset;
		vfs_read(fp, line, 14, &pos);
		ret = sscanf(line, "%d%d%d", &x, &y, &valid);
		/*pr_debug("valid:%d, x:%d, y:%d\n", valid, x, y);*/
		va_addr[i] = valid << 31 | y << 16 | x;
		/*pr_debug("va[%d]=0x%x\n", i, va_addr[i]);*/
	}
	filp_close(fp, 0);
	kfree((void *)line);
out:
	set_fs(oldfs);
}

static void mtk_fm_dev_load_search_center_win(
	void *va, char *filename, u32 blk_num)
{
	mtk_fm_dev_load_search_center_common(va, filename, blk_num, 16);
}

static void mtk_fm_dev_load_search_center(
	void *va, char *filename, u32 blk_num)
{
	mtk_fm_dev_load_search_center_common(va, filename, blk_num, 15);
}

#if 0
static void mtk_fm_dev_load_search_center(
	void *va, char *filename, u32 blk_num)
{
	struct file *fp;
	mm_segment_t oldfs;
	loff_t pos = 0;
	u8 *line;
	u32 *va_addr = (u32 *)va;
	int i;
	int valid, x, y;
	int ret;

	/*pr_err("demo_cv_read_msk: %s\n", filename);*/

	oldfs = get_fs();
	set_fs(get_ds());

	pr_debug("Open file %s\n", filename);
	fp = filp_open(filename, O_RDONLY, 0644);
	if (IS_ERR(fp)) {
		pr_err("fm_dev: open sc file %s fail!\n",
				filename);
		goto out;
	}

	line = kzalloc(14, GFP_KERNEL);
	for (i = 0; i < blk_num; i++) {
		pos = i * 16;
		vfs_read(fp, line, 14, &pos);
		ret = sscanf(line, "%d%d%d", &x, &y, &valid);
		/*pr_debug("valid:%d, x:%d, y:%d\n", valid, x, y);*/
		va_addr[i] = valid << 31 | y << 16 | x;
		/*pr_debug("va[%d]=0x%x\n", i, va_addr[i]);*/
	}
	filp_close(fp, 0);
	kfree((void *)line);
out:
	set_fs(oldfs);
}
#endif

static void check_mask_sram(struct mtk_fm_dev *fm_dev,
	struct mtk_fm_mask_tbl mask_tbl_write)
{
	struct mtk_fm_mask_tbl mask_tbl_read;
	int i, ret = 0;

	mask_tbl_read.size = mask_tbl_write.size;
	mask_tbl_read.mask_tbl = kzalloc(roundup(mask_tbl_write.size,
			PAGE_SIZE), GFP_KERNEL);
	LOG(CV_LOG_DBG, "check mask size %d\n", mask_tbl_write.size);

	ret = mtk_fm_get_mask_tbl(fm_dev->dev_fm, &mask_tbl_read);
	if (ret) {
		dev_err(fm_dev->dev, "fm_get_mask fail!\n");
		goto out;
	}
	for (i = 0; i < mask_tbl_write.size; i++) {
		/*pr_debug("mask %d @%d\n", i, mask_tbl_read.mask_tbl[i]);*/
		if (mask_tbl_write.mask_tbl[i] - mask_tbl_read.mask_tbl[i])
			pr_err("mask different @%d, w:%x, r:%x\n",
				i, mask_tbl_write.mask_tbl[i],
				mask_tbl_read.mask_tbl[i]);
	}
	LOG(CV_LOG_DBG, "mask sram write success!\n");
out:
	kfree(mask_tbl_read.mask_tbl);
}

static void check_sc_sram(struct mtk_fm_dev *fm_dev,
	struct mtk_fm_search_center sc_write)
{
	struct mtk_fm_search_center sc_read;
	int i, ret = 0;

	sc_read.size = sc_write.size;
	sc_read.sc = kzalloc(roundup(sc_read.size,
			PAGE_SIZE), GFP_KERNEL);
	ret = mtk_fm_get_search_center(fm_dev->dev_fm, &sc_read);
	if (ret) {
		dev_err(fm_dev->dev, "fm_get_sc fail!\n");
		goto out;
	}
	for (i = 0; i < sc_write.size; i++) {
		if (sc_write.sc[i] !=
			sc_read.sc[i])
			pr_err("sc different @%d, w:%x, r:%x\n",
			i, sc_write.sc[i],
			sc_read.sc[i]);
	}
	LOG(CV_LOG_DBG, "sc sram write success!\n");
out:
	kfree(sc_read.sc);
}

int mtk_fm_dev_module_config(struct mtk_fm_dev *fm_dev)
{
	struct mtk_fm_mask_tbl mask_tbl;
	struct mtk_fm_search_center sc;
	int ret = 0;

	/* set blk_sz */
	LOG(CV_LOG_DBG, "set blk:%d\n", fm_dev->blk_type);
	ret = mtk_fm_set_blk_type(fm_dev->dev_fm, NULL, fm_dev->blk_type);
	if (ret) {
		dev_err(fm_dev->dev_fm, "set blk type fail!\n");
		return ret;
	}

	/* set region */
	LOG(CV_LOG_DBG, "set region w:%d h:%d, blk_nr:%d\n",
		fm_dev->fe_w, fm_dev->fe_h, fm_dev->blk_nr);
	ret = mtk_fm_set_region(fm_dev->dev_fm, NULL,
			fm_dev->fe_w, fm_dev->fe_h, fm_dev->blk_nr);
	if (ret) {
		dev_err(fm_dev->dev_fm, "set region fail!\n");
		return ret;
	}

	/* set sr type */
	LOG(CV_LOG_DBG, "set sr:%d\n", fm_dev->sr_type);
	ret = mtk_fm_set_sr_type(fm_dev->dev_fm, NULL, fm_dev->sr_type);
	if (ret) {
		dev_err(fm_dev->dev_fm, "set sr type fail!\n");
		return ret;
	}

	/* load mask */
	if (fm_dev->mask_path_s[0] != 0) {
		mtk_fm_dev_load_mask(fm_dev->mask_va_s, fm_dev->mask_path_s,
				fm_dev->fm_msk_w_s, fm_dev->fm_msk_h_s);

		LOG(CV_LOG_DBG, "fm_set:fm_dev->mask_path_s %s\n",
			fm_dev->mask_path_s);
		LOG(CV_LOG_DBG, "fm_set:fm_dev->msk_tbl_size_s %d\n",
			fm_dev->msk_tbl_size_s);
		LOG(CV_LOG_DBG, "fm_set:fm_dev->mask_va_s %p\n",
			fm_dev->mask_va_s);
	}
	if (fm_dev->mask_path_tp[0] != 0) {
		mtk_fm_dev_load_mask(fm_dev->mask_va_tp, fm_dev->mask_path_tp,
				fm_dev->fm_msk_w_tp, fm_dev->fm_msk_h_tp);

		LOG(CV_LOG_DBG, "fm_set:fm_dev->mask_path_tp %s\n",
			fm_dev->mask_path_tp);
		LOG(CV_LOG_DBG, "fm_set:fm_dev->msk_tbl_size_tp %d\n",
			fm_dev->msk_tbl_size_tp);
		LOG(CV_LOG_DBG, "fm_set:fm_dev->mask_va_tp %p\n",
			fm_dev->mask_va_tp);
	}

	if (fm_dev->sc_path[0] != 0) {
		if (FILE_EOF_WIN)
			mtk_fm_dev_load_search_center_win(fm_dev->sc_va,
				fm_dev->sc_path, fm_dev->sc_size);
		else
			mtk_fm_dev_load_search_center(fm_dev->sc_va,
				fm_dev->sc_path, fm_dev->sc_size);

		LOG(CV_LOG_DBG, "fm_set:fm_dev->sc_path %s\n",
			fm_dev->sc_path);
		LOG(CV_LOG_DBG, "fm_set:fm_dev->sc_size %d\n",
			fm_dev->sc_size);
		LOG(CV_LOG_DBG, "fm_set:fm_dev->sc_va %p\n",
			fm_dev->sc_va);
	}

	if (fm_dev->sc_path_1_4[0] != 0) {
		if (FILE_EOF_WIN)
			mtk_fm_dev_load_search_center_win(fm_dev->sc_va_1_4,
				fm_dev->sc_path_1_4, fm_dev->sc_size);
		else
			mtk_fm_dev_load_search_center(fm_dev->sc_va_1_4,
				fm_dev->sc_path_1_4, fm_dev->sc_size);

		LOG(CV_LOG_DBG, "fm_set:fm_dev->sc_path_1_4 %s\n",
			fm_dev->sc_path_1_4);
		LOG(CV_LOG_DBG, "fm_set:fm_dev->sc_size %d\n",
			fm_dev->sc_size);
		LOG(CV_LOG_DBG, "fm_set:fm_dev->sc_va_1_4 %p\n",
			fm_dev->sc_va_1_4);
	}

	if (fm_dev->sc_path_1_16[0] != 0) {
		if (FILE_EOF_WIN)
			mtk_fm_dev_load_search_center_win(fm_dev->sc_va_1_16,
				fm_dev->sc_path_1_16, fm_dev->sc_size);
		else
			mtk_fm_dev_load_search_center(fm_dev->sc_va_1_16,
				fm_dev->sc_path_1_16, fm_dev->sc_size);

		LOG(CV_LOG_DBG, "fm_set:fm_dev->sc_path_1_16 %s\n",
			fm_dev->sc_path_1_16);
		LOG(CV_LOG_DBG, "fm_set:fm_dev->sc_size %d\n",
			fm_dev->sc_size);
		LOG(CV_LOG_DBG, "fm_set:fm_dev->sc_va_1_16 %p\n",
			fm_dev->sc_va_1_16);
	}

	/* set tbl */
	if (fm_dev->sr_type == MTK_FM_SR_TYPE_SPATIAL) {
		if (!fm_dev->mask_va_s || !fm_dev->msk_tbl_size_s)
			LOG(CV_LOG_DBG, "fm config spatial mask error!\n");
		mask_tbl.mask_tbl = (u32 *)fm_dev->mask_va_s;
		mask_tbl.size = fm_dev->msk_tbl_size_s;
		ret = mtk_fm_set_mask_tbl_sw_idx(fm_dev->dev_fm, NULL, 0);
		if (ret) {
			dev_err(fm_dev->dev_fm, "set mask tbl sw idx fail!\n");
			return ret;
		}
		ret = mtk_fm_set_mask_tbl_hw_idx(fm_dev->dev_fm, NULL, 0);
		if (ret) {
			dev_err(fm_dev->dev_fm, "set mask tbl hw idx fail!\n");
			return ret;
		}
		ret = mtk_fm_set_mask_tbl(fm_dev->dev_fm, NULL, &mask_tbl);
		if (ret) {
			dev_err(fm_dev->dev_fm, "set mask tbl fail!\n");
			return ret;
		}
		LOG(CV_LOG_DBG, "fm config spatial mask\n");
		check_mask_sram(fm_dev, mask_tbl);
	} else if (fm_dev->sr_type == MTK_FM_SR_TYPE_TEMPORAL_PREDICTION) {
		if (!fm_dev->mask_va_tp || !fm_dev->msk_tbl_size_tp) {
			dev_err(fm_dev->dev,
			"fm config temporal prediction mask error!\n");
			return -EINVAL;
		}
		mask_tbl.mask_tbl = (u32 *)fm_dev->mask_va_tp;
		mask_tbl.size = fm_dev->msk_tbl_size_tp;
		ret = mtk_fm_set_mask_tbl_sw_idx(fm_dev->dev_fm, NULL, 0);
		if (ret) {
			dev_err(fm_dev->dev_fm, "set mask tbl sw idx fail!\n");
			return ret;
		}
		ret = mtk_fm_set_mask_tbl_hw_idx(fm_dev->dev_fm, NULL, 0);
		if (ret) {
			dev_err(fm_dev->dev_fm, "set mask tbl hw idx fail!\n");
			return ret;
		}
		ret = mtk_fm_set_mask_tbl(fm_dev->dev_fm, NULL, &mask_tbl);
		if (ret) {
			dev_err(fm_dev->dev_fm, "set mask tbl fail!\n");
			return ret;
		}
		LOG(CV_LOG_DBG, "fm config temporal prediction mask\n");
		check_mask_sram(fm_dev, mask_tbl);
		if (!fm_dev->sc_va || !fm_dev->sc_size) {
			dev_err(fm_dev->dev,
				"fm null sc config!\n");
		} else {
			sc.sc = (u32 *)fm_dev->sc_va;
			sc.size = fm_dev->sc_size;
			ret = mtk_fm_set_search_center(fm_dev->dev_fm,
				NULL, &sc);
			if (ret) {
				dev_err(fm_dev->dev_fm, "set sc fail!\n");
				return ret;
			}
			LOG(CV_LOG_DBG, "fm config search center!\n");
			check_sc_sram(fm_dev, sc);
		}
	}


	/* Set req interval*/
	LOG(CV_LOG_DBG, "set img_req_interval: 0x%x\n",
		fm_dev->req_interval.img_req_interval);
	LOG(CV_LOG_DBG, "set desc_req_interval: 0x%x\n",
		fm_dev->req_interval.img_req_interval);
	LOG(CV_LOG_DBG, "set point_req_interval: 0x%x\n",
		fm_dev->req_interval.point_req_interval);
	LOG(CV_LOG_DBG, "set fmo_req_interval: 0x%x\n",
		fm_dev->req_interval.point_req_interval);
	LOG(CV_LOG_DBG, "set zncc_req_interval: 0x%x\n",
		fm_dev->req_interval.zncc_req_interval);
	ret = mtk_fm_set_req_interval(fm_dev->dev_fm, NULL,
		fm_dev->req_interval.img_req_interval,
		fm_dev->req_interval.desc_req_interval,
		fm_dev->req_interval.point_req_interval,
		fm_dev->req_interval.fmo_req_interval,
		fm_dev->req_interval.zncc_req_interval);
	if (ret) {
		dev_err(fm_dev->dev_fm, "set req interval fail!\n");
		return ret;
	}

	ret = mtk_fm_register_cb(fm_dev->dev_fm, &mtk_fm_cb, fm_dev);
	if (ret) {
		dev_err(fm_dev->dev_fm, "register fm cb fail!\n");
		return ret;
	}

	LOG(CV_LOG_DBG, "fm_set:fm_dev->fe_w %d\n", fm_dev->fe_w);
	LOG(CV_LOG_DBG, "fm_set:fm_dev->fe_h %d\n", fm_dev->fe_h);
	LOG(CV_LOG_DBG, "fm_set:fm_dev->sr_type %d\n", fm_dev->sr_type);
	return ret;
}
EXPORT_SYMBOL(mtk_fm_dev_module_config);

int mtk_fm_dev_path_connect(struct mtk_fm_dev *fm_dev)
{
	int ret = 0;
	int i = 0;

	LOG(CV_LOG_DBG, "fm0_path_connect\n");
	fm_dev->polling_wait = 1;
	fm_dev->flow_done = 0;
	fm_dev->fm_flush_wait = 0;
	fm_dev->warpa_flush_wait = 0;
	fm_dev->fe_flush_wait = 0;
	fm_dev->wen0_flush_wait = 0;
	fm_dev->wen1_flush_wait = 0;
	fm_dev->wen2_flush_wait = 0;
	fm_dev->wen3_flush_wait = 0;
	fm_dev->ren_flush_wait = 0;
	fm_dev->be0_flush_wait = 0;
	fm_dev->be1_flush_wait = 0;
	fm_dev->be2_flush_wait = 0;
	fm_dev->be3_flush_wait = 0;
	/* wpe */
	*fm_dev->wpe_et_va = 0;
	*fm_dev->wpe_et_max_va = 0;
	*fm_dev->wpe_cnt_va = 0;
	/* fe */
	*fm_dev->fe_et_va = 0;
	*fm_dev->fe_et_max_va = 0;
	*fm_dev->fe_cnt_va = 0;
	/* fms */
	*fm_dev->fms_delay_va = 0;
	*fm_dev->fms_delay_max_va = 0;
	*fm_dev->fms_et_va = 0;
	*fm_dev->fms_et_max_va = 0;
	/* feT */
	*fm_dev->feT_delay_va = 0;
	*fm_dev->feT_delay_max_va = 0;
	*fm_dev->feT_et_va = 0;
	*fm_dev->feT_et_max_va = 0;
	*fm_dev->feT_1_4_et_va = 0;
	*fm_dev->feT_1_4_et_max_va = 0;
	*fm_dev->feT_1_16_et_va = 0;
	*fm_dev->feT_1_16_et_max_va = 0;
	/* 1/16 sc */
	*fm_dev->sc_1_16_delay_va = 0;
	*fm_dev->sc_1_16_delay_max_va = 0;
	*fm_dev->sc_1_16_et_va = 0;
	*fm_dev->sc_1_16_et_max_va = 0;
	/* 1/16 fmT */
	*fm_dev->fmt_1_16_delay_va = 0;
	*fm_dev->fmt_1_16_delay_max_va = 0;
	*fm_dev->fmt_1_16_et_va = 0;
	*fm_dev->fmt_1_16_et_max_va = 0;
	/* 1/4 sc */
	*fm_dev->sc_1_4_delay_va = 0;
	*fm_dev->sc_1_4_delay_max_va = 0;
	*fm_dev->sc_1_4_et_va = 0;
	*fm_dev->sc_1_4_et_max_va = 0;
	/* 1/4 fmT */
	*fm_dev->fmt_1_4_delay_va = 0;
	*fm_dev->fmt_1_4_delay_max_va = 0;
	*fm_dev->fmt_1_4_et_va = 0;
	*fm_dev->fmt_1_4_et_max_va = 0;
	/* 1/1 sc */
	*fm_dev->sc_delay_va = 0;
	*fm_dev->sc_delay_max_va = 0;
	*fm_dev->sc_et_va = 0;
	*fm_dev->sc_et_max_va = 0;
	/* 1/1 fmT */
	*fm_dev->fmt_delay_va = 0;
	*fm_dev->fmt_delay_max_va = 0;
	*fm_dev->fmt_et_va = 0;
	*fm_dev->fmt_et_max_va = 0;
	/* headpose */
	*fm_dev->head_pose_delay_va = 0;
	*fm_dev->head_pose_delay_max_va = 0;
	*fm_dev->head_pose_et_va = 0;
	*fm_dev->head_pose_et_max_va = 0;
	/* fm vpu flow overall */
	*fm_dev->fm_vpu_et_va = 0;
	*fm_dev->fm_vpu_et_max_va = 0;
	*fm_dev->fm_vpu_cnt_va = 0;

	fm_dev->cmdq_client = cmdq_mbox_create(fm_dev->dev, 0);
	if (IS_ERR(fm_dev->cmdq_client)) {
		dev_err(fm_dev->dev, "failed to create mailbox cmdq_client\n");
		return PTR_ERR(fm_dev->cmdq_client);
	}
	fm_dev->cmdq_client_fe = cmdq_mbox_create(fm_dev->dev, 1);
	if (IS_ERR(fm_dev->cmdq_client_fe)) {
		dev_err(fm_dev->dev,
			"failed to create mailbox cmdq_client_fe\n");
		return PTR_ERR(fm_dev->cmdq_client_fe);
	}
	fm_dev->cmdq_client_warpa = cmdq_mbox_create(fm_dev->dev, 2);
	if (IS_ERR(fm_dev->cmdq_client_warpa)) {
		dev_err(fm_dev->dev,
			"failed to create mailbox cmdq_client_warpa\n");
		return PTR_ERR(fm_dev->cmdq_client_warpa);
	}
	fm_dev->cmdq_client_wen0 = cmdq_mbox_create(fm_dev->dev, 3);
	if (IS_ERR(fm_dev->cmdq_client_wen0)) {
		dev_err(fm_dev->dev,
			"failed to create mailbox cmdq_client_wen0\n");
		return PTR_ERR(fm_dev->cmdq_client_wen0);
	}
	fm_dev->cmdq_client_wen1 = cmdq_mbox_create(fm_dev->dev, 4);
	if (IS_ERR(fm_dev->cmdq_client_wen1)) {
		dev_err(fm_dev->dev,
			"failed to create mailbox cmdq_client_wen1\n");
		return PTR_ERR(fm_dev->cmdq_client_wen1);
	}
	fm_dev->cmdq_client_wen2 = cmdq_mbox_create(fm_dev->dev, 5);
	if (IS_ERR(fm_dev->cmdq_client_wen2)) {
		dev_err(fm_dev->dev,
			"failed to create mailbox cmdq_client_wen2\n");
		return PTR_ERR(fm_dev->cmdq_client_wen2);
	}
	fm_dev->cmdq_client_wen3 = cmdq_mbox_create(fm_dev->dev, 6);
	if (IS_ERR(fm_dev->cmdq_client_wen3)) {
		dev_err(fm_dev->dev,
			"failed to create mailbox cmdq_client_wen3\n");
		return PTR_ERR(fm_dev->cmdq_client_wen3);
	}
	fm_dev->cmdq_client_ren = cmdq_mbox_create(fm_dev->dev, 7);
	if (IS_ERR(fm_dev->cmdq_client_ren)) {
		dev_err(fm_dev->dev,
			"failed to create mailbox cmdq_client_ren\n");
		return PTR_ERR(fm_dev->cmdq_client_ren);
	}
	fm_dev->cmdq_client_be0 = cmdq_mbox_create(fm_dev->dev, 8);
	if (IS_ERR(fm_dev->cmdq_client_be0)) {
		dev_err(fm_dev->dev,
			"failed to create mailbox cmdq_client_be0\n");
		return PTR_ERR(fm_dev->cmdq_client_be0);
	}
	fm_dev->cmdq_client_be1 = cmdq_mbox_create(fm_dev->dev, 9);
	if (IS_ERR(fm_dev->cmdq_client_be1)) {
		dev_err(fm_dev->dev,
			"failed to create mailbox cmdq_client_be1\n");
		return PTR_ERR(fm_dev->cmdq_client_be1);
	}
	fm_dev->cmdq_client_be2 = cmdq_mbox_create(fm_dev->dev, 10);
	if (IS_ERR(fm_dev->cmdq_client_be2)) {
		dev_err(fm_dev->dev,
			"failed to create mailbox cmdq_client_be2\n");
		return PTR_ERR(fm_dev->cmdq_client_be2);
	}
	fm_dev->cmdq_client_be3 = cmdq_mbox_create(fm_dev->dev, 11);
	if (IS_ERR(fm_dev->cmdq_client_be3)) {
		dev_err(fm_dev->dev,
			"failed to create mailbox cmdq_client_be3\n");
		return PTR_ERR(fm_dev->cmdq_client_be3);
	}
	schedule_work(&fm_dev->trigger_work);
	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (fm_dev->fm_flush_wait)
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait fm_flush_wait timeout!\n");
	schedule_work(&fm_dev->fe_et_work);
	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (fm_dev->fe_flush_wait)
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait fe_flush_wait timeout!\n");
	schedule_work(&fm_dev->warpa_et_work);
	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (fm_dev->warpa_flush_wait)
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait warpa_flush_wait timeout!\n");
	schedule_work(&fm_dev->wen0_incomp_work);
	schedule_work(&fm_dev->wen1_incomp_work);
	schedule_work(&fm_dev->wen2_incomp_work);
	schedule_work(&fm_dev->wen3_incomp_work);
	schedule_work(&fm_dev->ren_incomp_work);
	schedule_work(&fm_dev->be0_lim_work);
	schedule_work(&fm_dev->be1_lim_work);
	schedule_work(&fm_dev->be2_lim_work);
	schedule_work(&fm_dev->be3_lim_work);

	if (fm_dev->fmdevmask & FM_DEV_FEO_SYSRAM_MODE) {
		ret = mtk_mmsys_cmmn_top_sel_mem(fm_dev->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_FM_FD, MTK_MMSYS_CMMN_SYSRAM);
		if (ret) {
			dev_err(fm_dev->dev,
				"cmmn_top_sel_mem fm fd failed!! %d\n",
				ret);
			return ret;
		}
		ret = mtk_mmsys_cmmn_top_sel_mem(fm_dev->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_FM_FP, MTK_MMSYS_CMMN_SYSRAM);
		if (ret) {
			dev_err(fm_dev->dev,
				"cmmn_top_sel_mem fm fp failed!! %d\n",
				ret);
			return ret;
		}
	} else {
		ret = mtk_mmsys_cmmn_top_sel_mem(fm_dev->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_FM_FD, MTK_MMSYS_CMMN_DRAM);
		if (ret) {
			dev_err(fm_dev->dev,
				"cmmn_top_sel_mem fm fd failed!! %d\n",
				ret);
			return ret;
		}
		ret = mtk_mmsys_cmmn_top_sel_mem(fm_dev->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_FM_FP, MTK_MMSYS_CMMN_DRAM);
		if (ret) {
			dev_err(fm_dev->dev,
				"cmmn_top_sel_mem fm fp failed!! %d\n",
				ret);
			return ret;
		}
	}

	if (fm_dev->fmdevmask & FM_DEV_IMG_SYSRAM_MODE) {
		ret = mtk_mmsys_cmmn_top_sel_mem(fm_dev->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_FM_IMG, MTK_MMSYS_CMMN_SYSRAM);
		if (ret) {
			dev_err(fm_dev->dev,
				"cmmn_top_sel_mem fm ing failed!! %d\n",
				ret);
			return ret;
		}
	} else {
		ret = mtk_mmsys_cmmn_top_sel_mem(fm_dev->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_FM_IMG, MTK_MMSYS_CMMN_DRAM);
		if (ret) {
			dev_err(fm_dev->dev,
				"cmmn_top_sel_mem fm ing failed!! %d\n",
				ret);
			return ret;
		}
	}

	if (fm_dev->fmdevmask & FM_DEV_FMO_SYSRAM_MODE) {
		ret = mtk_mmsys_cmmn_top_sel_mem(fm_dev->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_FM_FMO, MTK_MMSYS_CMMN_SYSRAM);
		if (ret) {
			dev_err(fm_dev->dev,
				"cmmn_top_sel_mem fm fmo failed!! %d\n",
				ret);
			return ret;
		}
		ret = mtk_mmsys_cmmn_top_sel_mem(fm_dev->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_FM_ZNCC, MTK_MMSYS_CMMN_SYSRAM);
		if (ret) {
			dev_err(fm_dev->dev,
				"cmmn_top_sel_mem fm zncc failed!! %d\n",
				ret);
			return ret;
		}
	} else {
		ret = mtk_mmsys_cmmn_top_sel_mem(fm_dev->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_FM_FMO, MTK_MMSYS_CMMN_DRAM);
		if (ret) {
			dev_err(fm_dev->dev,
				"cmmn_top_sel_mem fm fmo failed!! %d\n",
				ret);
			return ret;
		}
		ret = mtk_mmsys_cmmn_top_sel_mem(fm_dev->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_FM_ZNCC, MTK_MMSYS_CMMN_DRAM);
		if (ret) {
			dev_err(fm_dev->dev,
				"cmmn_top_sel_mem fm zncc failed!! %d\n",
				ret);
			return ret;
		}
	}
	return ret;
}
EXPORT_SYMBOL(mtk_fm_dev_path_connect);

int mtk_fm_dev_path_disconnect(struct mtk_fm_dev *fm_dev)
{
	int ret = 0;
	int i = 0;

	LOG(CV_LOG_DBG, "fm0_path_disconnect\n");
	fm_dev->polling_wait = 0;

	/* wait for work queue into sync flush*/
	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (fm_dev->fe_flush_wait)
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait fe_flush_wait timeout!\n");
	ret = cmdq_mbox_destroy(fm_dev->cmdq_client_fe);
	if (ret) {
		dev_err(fm_dev->dev,
			"failed to destroy mailbox cmdq_client_fe\n");
		return PTR_ERR(fm_dev->cmdq_client_fe);
	}

	/* wait for work queue into sync flush*/
	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (fm_dev->warpa_flush_wait)
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait warpa_flush_wait timeout!\n");
	ret = cmdq_mbox_destroy(fm_dev->cmdq_client_warpa);
	if (ret) {
		dev_err(fm_dev->dev,
			"failed to destroy mailbox cmdq_client_warpa\n");
		return PTR_ERR(fm_dev->cmdq_client_warpa);
	}

	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (fm_dev->wen0_flush_wait)
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait wen0_flush_wait timeout!\n");
	ret = cmdq_mbox_destroy(fm_dev->cmdq_client_wen0);
	if (ret) {
		dev_err(fm_dev->dev,
			"failed to destroy mailbox cmdq_client_wen0\n");
		return PTR_ERR(fm_dev->cmdq_client_wen0);
	}

	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (fm_dev->wen1_flush_wait)
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait wen1_flush_wait timeout!\n");
	ret = cmdq_mbox_destroy(fm_dev->cmdq_client_wen1);
	if (ret) {
		dev_err(fm_dev->dev,
			"failed to destroy mailbox cmdq_client_wen1\n");
		return PTR_ERR(fm_dev->cmdq_client_wen1);
	}

	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (fm_dev->wen2_flush_wait)
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait wen2_flush_wait timeout!\n");
	ret = cmdq_mbox_destroy(fm_dev->cmdq_client_wen2);
	if (ret) {
		dev_err(fm_dev->dev,
			"failed to destroy mailbox cmdq_client_wen2\n");
		return PTR_ERR(fm_dev->cmdq_client_wen2);
	}

	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (fm_dev->wen3_flush_wait)
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait wen3_flush_wait timeout!\n");
	ret = cmdq_mbox_destroy(fm_dev->cmdq_client_wen3);
	if (ret) {
		dev_err(fm_dev->dev,
			"failed to destroy mailbox cmdq_client_wen3\n");
		return PTR_ERR(fm_dev->cmdq_client_wen3);
	}

	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (fm_dev->ren_flush_wait)
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait ren_flush_wait timeout!\n");
	ret = cmdq_mbox_destroy(fm_dev->cmdq_client_ren);
	if (ret) {
		dev_err(fm_dev->dev,
			"failed to destroy mailbox cmdq_client_ren\n");
		return PTR_ERR(fm_dev->cmdq_client_ren);
	}

	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (fm_dev->be0_flush_wait)
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait be0_flush_wait timeout!\n");
	ret = cmdq_mbox_destroy(fm_dev->cmdq_client_be0);
	if (ret) {
		dev_err(fm_dev->dev,
			"failed to destroy mailbox cmdq_client_be0\n");
		return PTR_ERR(fm_dev->cmdq_client_be0);
	}

	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (fm_dev->be1_flush_wait)
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait be1_flush_wait timeout!\n");
	ret = cmdq_mbox_destroy(fm_dev->cmdq_client_be1);
	if (ret) {
		dev_err(fm_dev->dev,
			"failed to destroy mailbox cmdq_client_be1\n");
		return PTR_ERR(fm_dev->cmdq_client_be1);
	}

	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (fm_dev->be2_flush_wait)
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait be2_flush_wait timeout!\n");
	ret = cmdq_mbox_destroy(fm_dev->cmdq_client_be2);
	if (ret) {
		dev_err(fm_dev->dev,
			"failed to destroy mailbox cmdq_client_be2\n");
		return PTR_ERR(fm_dev->cmdq_client_be2);
	}

	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (fm_dev->be3_flush_wait)
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait be3_flush_wait timeout!\n");
	ret = cmdq_mbox_destroy(fm_dev->cmdq_client_be3);
	if (ret) {
		dev_err(fm_dev->dev,
			"failed to destroy mailbox cmdq_client_be3\n");
		return PTR_ERR(fm_dev->cmdq_client_be3);
	}

	/* wait for work queue into sync flush*/
	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (fm_dev->fm_flush_wait)
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait fm_flush_wait timeout!\n");
	ret = cmdq_mbox_destroy(fm_dev->cmdq_client);
	if (ret) {
		dev_err(fm_dev->dev, "failed to destroy mailbox cmdq_client\n");
		return PTR_ERR(fm_dev->cmdq_client);
	}

	/* wait for work queue stop done */
	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (!work_busy(&fm_dev->trigger_work))
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait fm_trigger_work stop timeout!\n");

	/* wait for work queue stop done */
	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (!work_busy(&fm_dev->fe_et_work))
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait fe_et_work stop timeout!\n");

	/* wait for work queue stop done */
	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (!work_busy(&fm_dev->warpa_et_work))
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait warpa_et_work stop timeout!\n");

	/* wait for work queue stop done */
	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (!work_busy(&fm_dev->wen0_incomp_work))
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait wen0_incomp_work stop timeout!\n");

	/* wait for work queue stop done */
	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (!work_busy(&fm_dev->wen1_incomp_work))
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait wen1_incomp_work stop timeout!\n");

	/* wait for work queue stop done */
	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (!work_busy(&fm_dev->wen2_incomp_work))
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait wen2_incomp_work stop timeout!\n");

	/* wait for work queue stop done */
	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (!work_busy(&fm_dev->wen3_incomp_work))
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait wen3_incomp_work stop timeout!\n");

	/* wait for work queue stop done */
	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (!work_busy(&fm_dev->ren_incomp_work))
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait ren_incomp_work stop timeout!\n");

	/* wait for work queue stop done */
	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (!work_busy(&fm_dev->be0_lim_work))
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait be0_incomp_work stop timeout!\n");

	/* wait for work queue stop done */
	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (!work_busy(&fm_dev->be1_lim_work))
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait be1_incomp_work stop timeout!\n");

	/* wait for work queue stop done */
	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (!work_busy(&fm_dev->be2_lim_work))
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait be2_incomp_work stop timeout!\n");

	/* wait for work queue stop done */
	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (!work_busy(&fm_dev->be3_lim_work))
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait be3_incomp_work stop timeout!\n");

	return 0;
}
EXPORT_SYMBOL(mtk_fm_dev_path_disconnect);

int mtk_fm_dev_module_set_buf(
	struct mtk_fm_dev *fm_dev,
	struct cmdq_pkt *handle,
	u32 img_type, u32 sr_type)
{
	int ret = 0;
	int img_idx, point_idx, desc_idx, fmo_idx;
	int diff;
	dma_addr_t addr, addr_img, addr_point, addr_desc;
	dma_addr_t offset_addr;
	u32 *va;
	int share_flag;
	int full_flag;

	share_flag = (fm_dev->sysram_layout == VT_SYSRAM_LAYOUT_CINE);
	full_flag = (fm_dev->sysram_layout == VT_SYSRAM_LAYOUT_FULL);
	diff = MTK_FM_DEV_BUF_TYPE_WDMA_R1 - MTK_FM_DEV_BUF_TYPE_WDMA_R0;

	if (sr_type == MTK_FM_SR_TYPE_SPATIAL) {
		/* set spatial input buffer */
		img_idx = MTK_FM_DEV_BUF_TYPE_WDMA_R0 +
				fm_dev->hdr_idx_cur * diff +
				img_type;
		point_idx = MTK_FM_DEV_BUF_TYPE_FE_L_POINT_R0 +
			fm_dev->hdr_idx_cur * diff;
		desc_idx = MTK_FM_DEV_BUF_TYPE_FE_L_DESC_R0 +
			fm_dev->hdr_idx_cur * diff;
		LOG(CV_LOG_DBG, "set spatial fm buffer addr\n");
		if (fm_dev->fmdevmask & FM_DEV_FEO_SYSRAM_MODE) {
			switch (img_type) {
			case MTK_FM_DEV_IMG_TYPE_NORMAL:
				addr_point = FEO_0_P_OUT_SRAM_ADDR;
				addr_desc = FEO_0_D_OUT_SRAM_ADDR;
				break;
			case MTK_FM_DEV_IMG_TYPE_1_4_WDMA:
				/* shall not get in here */
				dev_warn(fm_dev->dev,
					"err img_type:%d, do not support 1/4fms\n",
					img_type);
				addr_point = FET_1_P_OUT_SRAM_ADDR;
				addr_desc = FET_1_D_OUT_SRAM_ADDR;
				break;
			case MTK_FM_DEV_IMG_TYPE_1_16_WDMA:
				/* should not get in here */
				dev_warn(fm_dev->dev,
					"err img_type:%d, do not support 1/16fms\n",
					img_type);
				addr_point = FET_2_P_OUT_SRAM_ADDR;
				addr_desc = FET_2_D_OUT_SRAM_ADDR;
				break;
			default:
				dev_err(fm_dev->dev,
					"err img_type:%d in fm set sram buf flow\n",
					img_type);
				return -EINVAL;
			}
		} else {
			addr_point = fm_dev->buf[point_idx]->dma_addr;
			addr_desc = fm_dev->buf[desc_idx]->dma_addr;
		}

		if (fm_dev->fmdevmask & FM_DEV_IMG_SYSRAM_MODE) {
			switch (img_type) {
			case MTK_FM_DEV_IMG_TYPE_NORMAL:
				if (share_flag)
					addr_img =
						WDMA_0_R0_OUT_SRAM_ADDR_SHARE;
				else if (full_flag)
					addr_img = WDMA_0_R0_OUT_SRAM_ADDR_FULL;
				else
					addr_img = WDMA_0_R0_OUT_SRAM_ADDR;
				break;
			case MTK_FM_DEV_IMG_TYPE_1_4_WDMA:
				/* shall not get in here */
				dev_warn(fm_dev->dev,
					"err img_type:%d, do not support 1/4fms\n",
					img_type);
				if (full_flag)
					addr_img = WDMA_1_R0_OUT_SRAM_ADDR_FULL;
				else
					addr_img = WDMA_1_R0_OUT_SRAM_ADDR;
				break;
			case MTK_FM_DEV_IMG_TYPE_1_16_WDMA:
				/* should not get in here */
				dev_warn(fm_dev->dev,
					"err img_type:%d, do not support 1/16fms\n",
					img_type);
				if (full_flag)
					addr_img = WDMA_2_R0_OUT_SRAM_ADDR_FULL;
				else
					addr_img = WDMA_2_R0_OUT_SRAM_ADDR;
				break;
			default:
				dev_err(fm_dev->dev,
					"err img_type:%d in fm set sram buf flow\n",
					img_type);
				return -EINVAL;
			}
		} else {
			addr_img = fm_dev->buf[img_idx]->dma_addr;
		}
		if (fm_dev->img_num > 1)
			offset_addr = addr_img + fm_dev->fe_w;
		else
			offset_addr = addr_img;
		LOG(CV_LOG_DBG, "set buf MTK_FM_IN_BUF_IMG @0x%llx, pitch:%d\n",
				offset_addr, fm_dev->buf[img_idx]->pitch);
		ret = mtk_fm_set_multi_in_buf(
				fm_dev->dev_fm, handle,
				MTK_FM_IN_BUF_IMG,
				offset_addr,
				fm_dev->buf[img_idx]->pitch);
		if (ret) {
			dev_err(fm_dev->dev, "set img buf fail!\n");
			return ret;
		}

		LOG(CV_LOG_DBG, "set buf MTK_FM_IN_BUF_DESCRIPTOR @0x%llx\n",
				addr_desc);
		ret = mtk_fm_set_multi_in_buf(
				fm_dev->dev_fm, handle,
				MTK_FM_IN_BUF_DESCRIPTOR,
				addr_desc,
				fm_dev->buf[desc_idx]->pitch);
		if (ret) {
			dev_err(fm_dev->dev, "set desc buf fail!\n");
			return ret;
		}
		LOG(CV_LOG_DBG, "set buf MTK_FM_IN_BUF_POINT @0x%llx\n",
				addr_point);
		ret = mtk_fm_set_multi_in_buf(
				fm_dev->dev_fm, handle,
				MTK_FM_IN_BUF_POINT,
				addr_point,
				fm_dev->buf[point_idx]->pitch);
		if (ret) {
			dev_err(fm_dev->dev, "set point buf fail!\n");
			return ret;
		}
	} else {
		/* set temporal input buffer */
		img_idx = MTK_FM_DEV_BUF_TYPE_WDMA_R0 +
				fm_dev->hdr_idx_cur * diff +
				img_type;
		switch (img_type) {
		case MTK_FM_DEV_IMG_TYPE_NORMAL:
			point_idx = MTK_FM_DEV_BUF_TYPE_FET_P;
			desc_idx = MTK_FM_DEV_BUF_TYPE_FET_D;
			break;
		case MTK_FM_DEV_IMG_TYPE_1_4_WDMA:
			point_idx = MTK_FM_DEV_BUF_TYPE_1_4_FET_P;
			desc_idx = MTK_FM_DEV_BUF_TYPE_1_4_FET_D;
			break;
		case MTK_FM_DEV_IMG_TYPE_1_16_WDMA:
			point_idx = MTK_FM_DEV_BUF_TYPE_1_16_FET_P;
			desc_idx = MTK_FM_DEV_BUF_TYPE_1_16_FET_D;
			break;
		default:
			dev_err(fm_dev->dev,
				"err img_type:%d in fm set buf flow\n",
				img_type);
			return -EINVAL;
		}

		if (fm_dev->fmdevmask & FM_DEV_FEO_SYSRAM_MODE) {
			switch (img_type) {
			case MTK_FM_DEV_IMG_TYPE_NORMAL:
				addr_point = FET_0_P_OUT_SRAM_ADDR;
				addr_desc = FET_0_D_OUT_SRAM_ADDR;
				break;
			case MTK_FM_DEV_IMG_TYPE_1_4_WDMA:
				addr_point = FET_1_P_OUT_SRAM_ADDR;
				addr_desc = FET_1_D_OUT_SRAM_ADDR;
				break;
			case MTK_FM_DEV_IMG_TYPE_1_16_WDMA:
				addr_point = FET_2_P_OUT_SRAM_ADDR;
				addr_desc = FET_2_D_OUT_SRAM_ADDR;
				break;
			default:
				dev_err(fm_dev->dev,
					"err img_type:%d in fm set sram buf flow\n",
					img_type);
				return -EINVAL;
			}
		} else {
			addr_point = fm_dev->buf[point_idx]->dma_addr;
			addr_desc = fm_dev->buf[desc_idx]->dma_addr;
		}
		if (fm_dev->fmdevmask & FM_DEV_IMG_SYSRAM_MODE) {
			switch (img_type) {
			case MTK_FM_DEV_IMG_TYPE_NORMAL:
				if (share_flag)
					addr_img =
						WDMA_0_R0_OUT_SRAM_ADDR_SHARE;
				else if (full_flag)
					addr_img = WDMA_0_R0_OUT_SRAM_ADDR_FULL;
				else
					addr_img = WDMA_0_R0_OUT_SRAM_ADDR;
				break;
			case MTK_FM_DEV_IMG_TYPE_1_4_WDMA:
				if (full_flag)
					addr_img = WDMA_1_R0_OUT_SRAM_ADDR_FULL;
				else
					addr_img = WDMA_1_R0_OUT_SRAM_ADDR;
				break;
			case MTK_FM_DEV_IMG_TYPE_1_16_WDMA:
				if (full_flag)
					addr_img = WDMA_2_R0_OUT_SRAM_ADDR_FULL;
				else
					addr_img = WDMA_2_R0_OUT_SRAM_ADDR;
				break;
			default:
				dev_err(fm_dev->dev,
					"err img_type:%d in fm set sram buf flow\n",
					img_type);
				return -EINVAL;
			}
		} else {
			addr_img = fm_dev->buf[img_idx]->dma_addr;
		}
		LOG(CV_LOG_DBG, "set temporal fm buffer addr\n");
		LOG(CV_LOG_DBG, "pre %d, cur %d\n",
				fm_dev->hdr_idx_pre,
				fm_dev->hdr_idx_cur);
		LOG(CV_LOG_DBG, "set buf MTK_FM_IN_BUF_IMG @0x%llx\n",
				addr_img);
		ret = mtk_fm_set_multi_in_buf(fm_dev->dev_fm, handle,
				MTK_FM_IN_BUF_IMG, addr_img,
				fm_dev->buf[img_idx]->pitch);
		if (ret) {
			dev_err(fm_dev->dev, "set img buf fail!\n");
			return ret;
		}
		LOG(CV_LOG_DBG, "set buf MTK_FM_IN_BUF_DESCRIPTOR @0x%llx\n",
				addr_desc);
		ret = mtk_fm_set_multi_in_buf(fm_dev->dev_fm, handle,
				MTK_FM_IN_BUF_DESCRIPTOR, addr_desc, 0);
		if (ret) {
			dev_err(fm_dev->dev, "set desc buf fail!\n");
			return ret;
		}
		LOG(CV_LOG_DBG, "set buf MTK_FM_IN_BUF_POINT @0x%llx\n",
				addr_point);
		ret = mtk_fm_set_multi_in_buf(fm_dev->dev_fm, handle,
				MTK_FM_IN_BUF_POINT, addr_point, 0);
		if (ret) {
			dev_err(fm_dev->dev, "set point buf fail!\n");
			return ret;
		}
	}

	/* set output buffer */
	diff = MTK_FM_DEV_BUF_TYPE_FM_OUT_S_R1 -
		MTK_FM_DEV_BUF_TYPE_FM_OUT_S_R0;
	/** add hdr information here **/
	if (sr_type == MTK_FM_SR_TYPE_SPATIAL) {
		va = (u32 *)fm_dev->buf[MTK_FM_DEV_BUF_TYPE_FM_OUT_S_R0 +
				fm_dev->hdr_idx_cur * diff]->kvaddr;
		va[0] = fm_dev->hdr_idx_cur;
		va[1] = fm_dev->hdr_idx_cur;

		if (fm_dev->fmdevmask & FM_DEV_FMO_SYSRAM_MODE)
			offset_addr = FMO_S_R0_OUT_SRAM_ADDR + 0x80;
		else
			offset_addr =
			fm_dev->buf[MTK_FM_DEV_BUF_TYPE_FM_OUT_S_R0 +
				fm_dev->hdr_idx_cur * diff]->dma_addr + 0x80;
		LOG(CV_LOG_DBG,
			"set buf MTK_FM_DEV_BUF_TYPE_FM_OUT_S @0x%llx\n",
			offset_addr);
		ret = mtk_fm_set_multi_out_buf(fm_dev->dev_fm, handle,
			MTK_FM_OUT_BUF_FMO, offset_addr);
		if (ret) {
			dev_err(fm_dev->dev, "set fmo buf fail!\n");
			return ret;
		}
		if (fm_dev->fmdevmask & FM_DEV_FMO_SYSRAM_MODE)
			addr = FMO_S_R0_ZNCC_SRAM_ADDR;
		else
			addr = fm_dev->buf[MTK_FM_DEV_BUF_TYPE_FM_ZNCC_S_R0 +
				fm_dev->hdr_idx_cur * diff]->dma_addr;
		LOG(CV_LOG_DBG,
			"set buf MTK_FM_DEV_BUF_TYPE_FM_ZNCC_S @0x%llx\n",
			addr);
		ret = mtk_fm_set_multi_out_buf(fm_dev->dev_fm, handle,
			MTK_FM_OUT_BUF_ZNCC_SUBPIXEL, addr);
		if (ret) {
			dev_err(fm_dev->dev, "set zncc_sub buf fail!\n");
			return ret;
		}
	} else {
		fmo_idx = MTK_FM_DEV_BUF_TYPE_FM_OUT_T_R0 + 4 * img_type;
		va = (u32 *)fm_dev->buf[fmo_idx]->kvaddr;
		va[0] = fm_dev->hdr_idx_pre;
		va[1] = fm_dev->hdr_idx_cur;

		if (fm_dev->fmdevmask & FM_DEV_FMO_SYSRAM_MODE) {
			switch (img_type) {
			case MTK_FM_DEV_IMG_TYPE_NORMAL:
				addr = FMO_T_R0_OUT_SRAM_ADDR;
				break;
			case MTK_FM_DEV_IMG_TYPE_1_4_WDMA:
				addr = FMO_1_4_T_R0_OUT_SRAM_ADDR;
				break;
			case MTK_FM_DEV_IMG_TYPE_1_16_WDMA:
				addr = FMO_1_16_T_R0_OUT_SRAM_ADDR;
				break;
			default:
				dev_err(fm_dev->dev,
					"err img_type:%d in fm set sram buf flow\n",
					img_type);
				return -EINVAL;
			}
		} else {
			addr = fm_dev->buf[fmo_idx]->dma_addr;
		}
		offset_addr = addr + 0x80;
		LOG(CV_LOG_DBG,
			"set buf MTK_FM_DEV_BUF_TYPE_FM_OUT_T @0x%llx\n",
			offset_addr);
		ret = mtk_fm_set_multi_out_buf(fm_dev->dev_fm, handle,
			MTK_FM_OUT_BUF_FMO, offset_addr);
		if (ret) {
			dev_err(fm_dev->dev, "set fmo buf fail!\n");
			return ret;
		}
		fmo_idx = MTK_FM_DEV_BUF_TYPE_FM_ZNCC_T_R0 + 2 * img_type;
		if (fm_dev->fmdevmask & FM_DEV_FMO_SYSRAM_MODE) {
			switch (img_type) {
			case MTK_FM_DEV_IMG_TYPE_NORMAL:
				addr = FMO_T_R0_ZNCC_SRAM_ADDR;
				break;
			case MTK_FM_DEV_IMG_TYPE_1_4_WDMA:
				addr = FMO_1_4_T_R0_ZNCC_SRAM_ADDR;
				break;
			case MTK_FM_DEV_IMG_TYPE_1_16_WDMA:
				addr = FMO_1_16_T_R0_ZNCC_SRAM_ADDR;
				break;
			default:
				dev_err(fm_dev->dev,
					"err img_type:%d in fm set sram buf flow\n",
					img_type);
				return -EINVAL;
			}
		} else {
			addr = fm_dev->buf[fmo_idx]->dma_addr;
		}
		LOG(CV_LOG_DBG,
			"set buf MTK_FM_DEV_BUF_TYPE_FM_ZNCC_T @0x%llx\n",
			addr);
		ret = mtk_fm_set_multi_out_buf(fm_dev->dev_fm, handle,
			MTK_FM_OUT_BUF_ZNCC_SUBPIXEL, addr);
		if (ret) {
			dev_err(fm_dev->dev, "set zncc_sub buf fail!\n");
			return ret;
		}
	}

	return ret;
}
EXPORT_SYMBOL(mtk_fm_dev_module_set_buf);

int mtk_fm_dev_module_power_on(struct mtk_fm_dev *fm_dev)
{
	int msk_blk_w;
	int msk_blk_h;

	msk_blk_w = (fm_dev->fm_msk_w_s + 3) / 4;
	msk_blk_w = ((msk_blk_w + 1) / 2) * 2;
	msk_blk_h = (fm_dev->fm_msk_h_s + 7) / 8;
	fm_dev->msk_tbl_size_s = msk_blk_w * msk_blk_h;
	fm_dev->mask_va_s = kzalloc(roundup(fm_dev->msk_tbl_size_s,
			PAGE_SIZE), GFP_KERNEL);

	msk_blk_w = (fm_dev->fm_msk_w_tp + 3) / 4;
	msk_blk_w = ((msk_blk_w + 1) / 2) * 2;
	msk_blk_h = (fm_dev->fm_msk_h_tp + 7) / 8;
	fm_dev->msk_tbl_size_tp = msk_blk_w * msk_blk_h;
	fm_dev->mask_va_tp = kzalloc(roundup(fm_dev->msk_tbl_size_tp,
			PAGE_SIZE), GFP_KERNEL);

	fm_dev->sc_va = kzalloc(roundup(fm_dev->sc_size,
			PAGE_SIZE), GFP_KERNEL);

	fm_dev->sc_va_1_4 = kzalloc(roundup(fm_dev->sc_size,
			PAGE_SIZE), GFP_KERNEL);

	fm_dev->sc_va_1_16 = kzalloc(roundup(fm_dev->sc_size,
			PAGE_SIZE), GFP_KERNEL);

	return mtk_fm_power_on(fm_dev->dev_fm);
}
EXPORT_SYMBOL(mtk_fm_dev_module_power_on);

int mtk_fm_dev_module_power_off(struct mtk_fm_dev *fm_dev)
{
	if (fm_dev->mask_va_s)
		kfree(fm_dev->mask_va_s);
	if (fm_dev->mask_va_tp)
		kfree(fm_dev->mask_va_tp);
	if (fm_dev->sc_va)
		kfree(fm_dev->sc_va);
	if (fm_dev->sc_va_1_4)
		kfree(fm_dev->sc_va_1_4);
	if (fm_dev->sc_va_1_16)
		kfree(fm_dev->sc_va_1_16);

	return mtk_fm_power_off(fm_dev->dev_fm);
}
EXPORT_SYMBOL(mtk_fm_dev_module_power_off);

int mtk_fm_dev_module_start(struct mtk_fm_dev *fm_dev)
{
	return mtk_fm_start(fm_dev->dev_fm, NULL);
}
EXPORT_SYMBOL(mtk_fm_dev_module_start);

int mtk_fm_dev_module_stop(struct mtk_fm_dev *fm_dev)
{
	int ret = 0;

	fm_dev->polling_wait = 0;

	ret = mtk_fm_stop(fm_dev->dev_fm, NULL);
	if (ret) {
		dev_err(fm_dev->dev_fm, "fm stop fail!\n");
		return ret;
	}

	ret = mtk_fm_reset(fm_dev->dev_fm);
	if (ret) {
		dev_err(fm_dev->dev_fm, "fm reset fail!\n");
		return ret;
	}

	/* when change fm blk_num, need to do this */
#if 0
	ret = mtk_mmsys_cmmn_top_mod_reset(fm_dev->dev_mmsys_cmmn_top, MDP_FM);
	if (ret) {
		dev_err(fm_dev->dev_fm, "fm hw reset fail!\n");
		return ret;
	}
#endif

	ret = mtk_fm_get_dma_idle(fm_dev->dev_fm, NULL);
	if (ret) {
		dev_err(fm_dev->dev_fm, "fm get dma idle fail!\n");
		return ret;
	}

	return ret;
}
EXPORT_SYMBOL(mtk_fm_dev_module_stop);

static int mtk_fm_dev_open(struct inode *inode, struct file *flip)
{
	int ret = 0;
	struct mtk_fm_dev *fm = container_of(inode->i_cdev,
	struct mtk_fm_dev, chrdev);

	flip->private_data = fm;

	return ret;
}

static int mtk_fm_dev_release(struct inode *inode, struct file *flip)
{
	/*struct mtk_fm_dev *fm = flip->private_data;*/

	return 0;
}

static int append_config_spatial_fm_gce_cmd(struct mtk_fm_dev *fm_dev,
	struct cmdq_pkt *handle)
{
	struct mtk_fm_mask_tbl mask_tbl;
	int blk_sz, blk_nr;
	int ret = 0;

	blk_sz = 32;
	blk_nr = ((fm_dev->fe_w + blk_sz - 1) / blk_sz) *
		((fm_dev->fe_h + blk_sz - 1) / blk_sz);

	ret = mtk_fm_stop(fm_dev->dev_fm, handle);
	if (ret) {
		dev_err(fm_dev->dev_fm, "append fm stop fail!\n");
		return ret;
	}
	ret = mtk_fm_get_dma_idle(fm_dev->dev_fm, handle);
	if (ret) {
		dev_err(fm_dev->dev_fm, "append get fm dma idle fail!\n");
		return ret;
	}
	/* no need? */
	/*mtk_fm_reset(fm_dev->dev_fm, handle);*/

	ret = mtk_fm_set_blk_type(fm_dev->dev_fm, handle,
				  MTK_FM_BLK_SIZE_TYPE_32x32);
	if (ret) {
		dev_err(fm_dev->dev_fm, "append set fm blk type fail!\n");
		return ret;
	}

	ret = mtk_fm_set_region(fm_dev->dev_fm, handle, fm_dev->fe_w,
				fm_dev->fe_h, blk_nr);
	if (ret) {
		dev_err(fm_dev->dev_fm, "append set fm region fail!\n");
		return ret;
	}

	ret = mtk_fm_set_sr_type(fm_dev->dev_fm, handle,
				 MTK_FM_SR_TYPE_SPATIAL);
	if (ret) {
		dev_err(fm_dev->dev_fm, "append set fm sr type fail!\n");
		return ret;
	}

	ret = mtk_fm_dev_module_set_buf(fm_dev, handle,
					MTK_FM_DEV_IMG_TYPE_NORMAL,
					MTK_FM_SR_TYPE_SPATIAL);
	if (ret) {
		dev_err(fm_dev->dev_fm, "append set fm buf fail!\n");
		return ret;
	}

	mask_tbl.mask_tbl = (u32 *)fm_dev->mask_va_s;
	mask_tbl.size = fm_dev->msk_tbl_size_s;
	ret = mtk_fm_set_mask_tbl(fm_dev->dev_fm, handle, &mask_tbl);
	if (ret) {
		dev_err(fm_dev->dev_fm, "append set fm mask tbl fail!\n");
		return ret;
	}

	ret = mtk_fm_set_zncc_threshold(fm_dev->dev_fm, handle,
		fm_dev->zncc_th);
	if (ret) {
		dev_err(fm_dev->dev_fm, "append set fm zncc threshold fail!\n");
		return ret;
	}

	return ret;
}
static int append_config_temporal_fm_gce_cmd(struct mtk_fm_dev *fm_dev,
	struct cmdq_pkt *handle, u32 img_type, void *search_center)
{
	struct mtk_fm_mask_tbl mask_tbl;
	struct mtk_fm_search_center sc;
	int blk_sz, blk_nr;
	int w, h;
	int ret = 0;

	ret = mtk_fm_stop(fm_dev->dev_fm, handle);
	if (ret) {
		dev_err(fm_dev->dev_fm, "append fm stop fail!\n");
		return ret;
	}
	ret = mtk_fm_get_dma_idle(fm_dev->dev_fm, handle);
	if (ret) {
		dev_err(fm_dev->dev_fm, "append get fm dma idle fail!\n");
		return ret;
	}
	/* no need? */
	/*mtk_fm_reset(fm_dev->dev_fm, handle);*/

	switch (img_type) {
	case MTK_FM_DEV_IMG_TYPE_NORMAL:
		ret = mtk_fm_set_blk_type(fm_dev->dev_fm, handle,
			MTK_FM_BLK_SIZE_TYPE_32x32);
		blk_sz = 32;
		w = fm_dev->fe_w;
		h = fm_dev->fe_h;
		break;
	case MTK_FM_DEV_IMG_TYPE_1_4_WDMA:
		ret = mtk_fm_set_blk_type(fm_dev->dev_fm, handle,
			MTK_FM_BLK_SIZE_TYPE_16x16);
		blk_sz = 16;
		w = fm_dev->fe_w / 2;
		h = fm_dev->fe_h / 2;
		break;
	case MTK_FM_DEV_IMG_TYPE_1_16_WDMA:
		ret = mtk_fm_set_blk_type(fm_dev->dev_fm, handle,
			MTK_FM_BLK_SIZE_TYPE_8x8);
		blk_sz = 8;
		w = fm_dev->fe_w / 4;
		h = fm_dev->fe_h / 4;
		break;
	default:
		dev_err(fm_dev->dev,
			"err img_type:%d in temporal fm gce flow\n", img_type);
		return -EINVAL;
	}
	if (ret) {
		dev_err(fm_dev->dev_fm, "append set fm blk type fail!\n");
		return ret;
	}
	blk_nr = ((w + blk_sz - 1) / blk_sz) * ((h + blk_sz - 1) / blk_sz);
	ret = mtk_fm_set_region(fm_dev->dev_fm, handle, w, h, blk_nr);
	if (ret) {
		dev_err(fm_dev->dev_fm, "append set fm region fail!\n");
		return ret;
	}

	ret = mtk_fm_set_sr_type(fm_dev->dev_fm, handle,
				 MTK_FM_SR_TYPE_TEMPORAL_PREDICTION);
	if (ret) {
		dev_err(fm_dev->dev_fm, "append set fm sr type fail!\n");
		return ret;
	}

	ret = mtk_fm_dev_module_set_buf(fm_dev, handle, img_type,
					MTK_FM_SR_TYPE_TEMPORAL_PREDICTION);
	if (ret) {
		dev_err(fm_dev->dev_fm, "append set fm buf fail!\n");
		return ret;
	}

	mask_tbl.mask_tbl = (u32 *)fm_dev->mask_va_tp;
	mask_tbl.size = fm_dev->msk_tbl_size_tp;
	ret = mtk_fm_set_mask_tbl(fm_dev->dev_fm, handle, &mask_tbl);
	if (ret) {
		dev_err(fm_dev->dev_fm, "append set fm mask tbl fail!\n");
		return ret;
	}

	sc.sc = (u32 *)search_center;
	sc.size = fm_dev->sc_size;
	ret = mtk_fm_set_search_center(fm_dev->dev_fm, handle, &sc);
	if (ret) {
		dev_err(fm_dev->dev_fm, "append set fm sc fail!\n");
		return ret;
	}

	ret = mtk_fm_set_zncc_threshold(fm_dev->dev_fm, handle,
		fm_dev->zncc_th);
	if (ret) {
		dev_err(fm_dev->dev_fm, "append set fm zncc threshold fail!\n");
		return ret;
	}

	return ret;
}

static void dump_buf(struct mtk_fm_dev *fm_dev, int buf_idx, int size,
	char *file_path)
{
	int pa = -1;

	if (fm_dev->fmdevmask & FM_DEV_FEO_SYSRAM_MODE) {
		switch (buf_idx) {
		case MTK_FM_DEV_BUF_TYPE_FE_L_POINT_R0:
			pa = FEO_0_P_OUT_SRAM_ADDR;
			break;
		case MTK_FM_DEV_BUF_TYPE_FE_L_DESC_R0:
			pa = FEO_0_D_OUT_SRAM_ADDR;
			break;
		case MTK_FM_DEV_BUF_TYPE_FE_R_POINT_R0:
			pa = FEO_1_P_OUT_SRAM_ADDR;
			break;
		case MTK_FM_DEV_BUF_TYPE_FE_R_DESC_R0:
			pa = FEO_1_D_OUT_SRAM_ADDR;
			break;
		case MTK_FM_DEV_BUF_TYPE_FET_P:
			pa = FET_0_P_OUT_SRAM_ADDR;
			break;
		case MTK_FM_DEV_BUF_TYPE_FET_D:
			pa = FET_0_D_OUT_SRAM_ADDR;
			break;
		case MTK_FM_DEV_BUF_TYPE_1_4_FET_P:
			pa = FET_1_P_OUT_SRAM_ADDR;
			break;
		case MTK_FM_DEV_BUF_TYPE_1_4_FET_D:
			pa = FET_1_D_OUT_SRAM_ADDR;
			break;
		case MTK_FM_DEV_BUF_TYPE_1_16_FET_P:
			pa = FET_2_P_OUT_SRAM_ADDR;
			break;
		case MTK_FM_DEV_BUF_TYPE_1_16_FET_D:
			pa = FET_2_D_OUT_SRAM_ADDR;
			break;
		default:
			break;
		}
	}
	if (fm_dev->fmdevmask & FM_DEV_IMG_SYSRAM_MODE) {
		switch (buf_idx) {
		case MTK_FM_DEV_BUF_TYPE_WDMA_R0:
			if (fm_dev->sysram_layout == VT_SYSRAM_LAYOUT_CINE)
				pa = WDMA_0_R0_OUT_SRAM_ADDR_SHARE;
			else if (fm_dev->sysram_layout == VT_SYSRAM_LAYOUT_FULL)
				pa = WDMA_0_R0_OUT_SRAM_ADDR_FULL;
			else
				pa = WDMA_0_R0_OUT_SRAM_ADDR;
			break;
		case MTK_FM_DEV_BUF_TYPE_1_4_WDMA_R0:
			if (fm_dev->sysram_layout == VT_SYSRAM_LAYOUT_FULL)
				pa = WDMA_1_R0_OUT_SRAM_ADDR_FULL;
			else
				pa = WDMA_1_R0_OUT_SRAM_ADDR;
			break;
		case MTK_FM_DEV_BUF_TYPE_1_16_WDMA_R0:
			if (fm_dev->sysram_layout == VT_SYSRAM_LAYOUT_FULL)
				pa = WDMA_2_R0_OUT_SRAM_ADDR_FULL;
			else
				pa = WDMA_2_R0_OUT_SRAM_ADDR;
			break;
		default:
			break;
		}
	}
	if (fm_dev->fmdevmask & FM_DEV_FMO_SYSRAM_MODE) {
		switch (buf_idx) {
		case MTK_FM_DEV_BUF_TYPE_FM_OUT_S_R0:
			pa = FMO_S_R0_OUT_SRAM_ADDR;
			break;
		case MTK_FM_DEV_BUF_TYPE_FM_ZNCC_S_R0:
			pa = FMO_S_R0_ZNCC_SRAM_ADDR;
			break;
		case MTK_FM_DEV_BUF_TYPE_FM_OUT_T_R0:
			pa = FMO_T_R0_OUT_SRAM_ADDR;
			break;
		case MTK_FM_DEV_BUF_TYPE_FM_ZNCC_T_R0:
			pa = FMO_T_R0_ZNCC_SRAM_ADDR;
			break;
		case MTK_FM_DEV_BUF_TYPE_1_4_FM_OUT_T_R0:
			pa = FMO_1_4_T_R0_OUT_SRAM_ADDR;
			break;
		case MTK_FM_DEV_BUF_TYPE_1_4_FM_ZNCC_T_R0:
			pa = FMO_1_4_T_R0_ZNCC_SRAM_ADDR;
			break;
		case MTK_FM_DEV_BUF_TYPE_1_16_FM_OUT_T_R0:
			pa = FMO_1_16_T_R0_OUT_SRAM_ADDR;
			break;
		case MTK_FM_DEV_BUF_TYPE_1_16_FM_ZNCC_T_R0:
			pa = FMO_1_16_T_R0_ZNCC_SRAM_ADDR;
			break;
		default:
			break;
		}
	}
	if (pa >= 0)
		move_sram_to_va(fm_dev->dev_sysram, pa,
				(void *)fm_dev->buf[buf_idx]->kvaddr,
				size);
	LOG(CV_LOG_INFO, "dump idx:%d, size:%d @%p\n", buf_idx, size,
		(void *)fm_dev->buf[buf_idx]->kvaddr);
	mtk_common_write_file(fm_dev->buf[buf_idx]->kvaddr, file_path, size);
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

/* note that hard code to use SPR0 to assign pa, SPR2, SPR3 to do if */
static void append_max_exec_time_gce_cmd(struct cmdq_pkt *handle, u16 exec_idx,
	u16 max_idx, dma_addr_t pa)
{
	struct cmdq_operand left_operand, right_operand;

	/* store jump offset -> SPR0*/
	cmdq_pkt_assign_command(handle, GCE_SPR0, 2 * CMDQ_INST_SIZE);
	/* because GPRR can't be use for cond_jump operand */
	/* so move to SPR first */
	cmdq_pkt_logic_command(handle, CMDQ_LOGIC_ADD, GCE_SPR2,
		cmdq_operand_reg(&left_operand, exec_idx),
		cmdq_operand_immediate(&right_operand, 0));
	cmdq_pkt_logic_command(handle, CMDQ_LOGIC_ADD, GCE_SPR3,
		cmdq_operand_reg(&left_operand, max_idx),
		cmdq_operand_immediate(&right_operand, 0));
	/* if current exec time > max, jump offset */
	cmdq_pkt_cond_jump(handle, GCE_SPR0,
			   cmdq_operand_reg(&left_operand, GCE_SPR2),
			   cmdq_operand_reg(&right_operand, GCE_SPR3),
			   CMDQ_GREATER_THAN);
	/* if cur exec time <= max, skip assign */
	cmdq_pkt_jump(handle, 2 * CMDQ_INST_SIZE);
	/* assign SPR3 = SPR2 (max = cur exec time) */
	cmdq_pkt_logic_command(handle, CMDQ_LOGIC_ADD, max_idx,
		cmdq_operand_reg(&left_operand, exec_idx),
		cmdq_operand_immediate(&right_operand, 0));
	/* pa = max exec time */
	cmdq_pkt_assign_command(handle, GCE_SPR0, pa);
	cmdq_pkt_store_reg(handle, GCE_SPR0, max_idx, 0xffffffff);
}

/* note that hard code to use SPR0 to assign pa */
static void append_inc_frame_cnt_cmd(struct cmdq_pkt *handle, u16 cnt_idx,
		dma_addr_t pa)
{
	struct cmdq_operand left_operand, right_operand;

	cmdq_pkt_logic_command(handle, CMDQ_LOGIC_ADD, cnt_idx,
		cmdq_operand_reg(&left_operand, cnt_idx),
		cmdq_operand_immediate(&right_operand, 1));
	/* pa = wpe frame cnt */
	cmdq_pkt_assign_command(handle, GCE_SPR0, pa);
	cmdq_pkt_store_reg(handle, GCE_SPR0, cnt_idx, 0xffffffff);
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

static void create_fm_dev_statistic_buf(struct mtk_fm_dev *fm_dev)
{
	/* wpe */
	fm_dev->wpe_et_va = init_buffer(fm_dev->dev, &fm_dev->wpe_et_pa);

	fm_dev->wpe_et_max_va = init_buffer(fm_dev->dev,
					&fm_dev->wpe_et_max_pa);

	fm_dev->wpe_cnt_va = init_buffer(fm_dev->dev, &fm_dev->wpe_cnt_pa);

	/* fe */
	fm_dev->fe_et_va = init_buffer(fm_dev->dev, &fm_dev->fe_et_pa);

	fm_dev->fe_et_max_va = init_buffer(fm_dev->dev, &fm_dev->fe_et_max_pa);

	fm_dev->fe_cnt_va = init_buffer(fm_dev->dev, &fm_dev->fe_cnt_pa);

	/* fms */
	fm_dev->fms_delay_va = init_buffer(fm_dev->dev, &fm_dev->fms_delay_pa);

	fm_dev->fms_delay_max_va = init_buffer(fm_dev->dev,
					       &fm_dev->fms_delay_max_pa);

	fm_dev->fms_et_va = init_buffer(fm_dev->dev, &fm_dev->fms_et_pa);

	fm_dev->fms_et_max_va = init_buffer(fm_dev->dev,
					    &fm_dev->fms_et_max_pa);

	/* feT */
	fm_dev->feT_delay_va = init_buffer(fm_dev->dev, &fm_dev->feT_delay_pa);

	fm_dev->feT_delay_max_va = init_buffer(fm_dev->dev,
					    &fm_dev->feT_delay_max_pa);

	fm_dev->feT_et_va = init_buffer(fm_dev->dev, &fm_dev->feT_et_pa);

	fm_dev->feT_et_max_va = init_buffer(fm_dev->dev,
					    &fm_dev->feT_et_max_pa);

	fm_dev->feT_1_4_et_va = init_buffer(fm_dev->dev,
					    &fm_dev->feT_1_4_et_pa);

	fm_dev->feT_1_4_et_max_va = init_buffer(fm_dev->dev,
						&fm_dev->feT_1_4_et_max_pa);

	fm_dev->feT_1_16_et_va = init_buffer(fm_dev->dev,
					     &fm_dev->feT_1_16_et_pa);

	fm_dev->feT_1_16_et_max_va = init_buffer(fm_dev->dev,
						 &fm_dev->feT_1_16_et_max_pa);

	/* 1/16 sc */
	fm_dev->sc_1_16_delay_va = init_buffer(fm_dev->dev,
					     &fm_dev->sc_1_16_delay_pa);

	fm_dev->sc_1_16_delay_max_va = init_buffer(fm_dev->dev,
					&fm_dev->sc_1_16_delay_max_pa);

	fm_dev->sc_1_16_et_va = init_buffer(fm_dev->dev,
					     &fm_dev->sc_1_16_et_pa);

	fm_dev->sc_1_16_et_max_va = init_buffer(fm_dev->dev,
						 &fm_dev->sc_1_16_et_max_pa);

	/* 1/16 fmT */
	fm_dev->fmt_1_16_delay_va = init_buffer(fm_dev->dev,
					     &fm_dev->fmt_1_16_delay_pa);

	fm_dev->fmt_1_16_delay_max_va = init_buffer(fm_dev->dev,
					&fm_dev->fmt_1_16_delay_max_pa);

	fm_dev->fmt_1_16_et_va = init_buffer(fm_dev->dev,
					     &fm_dev->fmt_1_16_et_pa);

	fm_dev->fmt_1_16_et_max_va = init_buffer(fm_dev->dev,
						 &fm_dev->fmt_1_16_et_max_pa);

	/* 1/4 sc */
	fm_dev->sc_1_4_delay_va = init_buffer(fm_dev->dev,
					    &fm_dev->sc_1_4_delay_pa);

	fm_dev->sc_1_4_delay_max_va = init_buffer(fm_dev->dev,
						&fm_dev->sc_1_4_delay_max_pa);

	fm_dev->sc_1_4_et_va = init_buffer(fm_dev->dev,
					    &fm_dev->sc_1_4_et_pa);

	fm_dev->sc_1_4_et_max_va = init_buffer(fm_dev->dev,
						&fm_dev->sc_1_4_et_max_pa);

	/* 1/4 fmT */
	fm_dev->fmt_1_4_delay_va = init_buffer(fm_dev->dev,
					    &fm_dev->fmt_1_4_delay_pa);

	fm_dev->fmt_1_4_delay_max_va = init_buffer(fm_dev->dev,
						&fm_dev->fmt_1_4_delay_max_pa);

	fm_dev->fmt_1_4_et_va = init_buffer(fm_dev->dev,
					    &fm_dev->fmt_1_4_et_pa);

	fm_dev->fmt_1_4_et_max_va = init_buffer(fm_dev->dev,
						&fm_dev->fmt_1_4_et_max_pa);

	/* 1/1 sc */
	fm_dev->sc_delay_va = init_buffer(fm_dev->dev, &fm_dev->sc_delay_pa);

	fm_dev->sc_delay_max_va = init_buffer(fm_dev->dev,
					    &fm_dev->sc_delay_max_pa);

	fm_dev->sc_et_va = init_buffer(fm_dev->dev, &fm_dev->sc_et_pa);

	fm_dev->sc_et_max_va = init_buffer(fm_dev->dev,
					    &fm_dev->sc_et_max_pa);

	/* 1/1 fmT */
	fm_dev->fmt_delay_va = init_buffer(fm_dev->dev, &fm_dev->fmt_delay_pa);

	fm_dev->fmt_delay_max_va = init_buffer(fm_dev->dev,
					    &fm_dev->fmt_delay_max_pa);

	fm_dev->fmt_et_va = init_buffer(fm_dev->dev, &fm_dev->fmt_et_pa);

	fm_dev->fmt_et_max_va = init_buffer(fm_dev->dev,
					    &fm_dev->fmt_et_max_pa);

	/* headpose */
	fm_dev->head_pose_delay_va = init_buffer(fm_dev->dev,
					      &fm_dev->head_pose_delay_pa);

	fm_dev->head_pose_delay_max_va = init_buffer(fm_dev->dev,
					&fm_dev->head_pose_delay_max_pa);

	fm_dev->head_pose_et_va = init_buffer(fm_dev->dev,
					      &fm_dev->head_pose_et_pa);

	fm_dev->head_pose_et_max_va = init_buffer(fm_dev->dev,
						  &fm_dev->head_pose_et_max_pa);

	/* fm vpu flow overall */
	fm_dev->fm_vpu_et_va = init_buffer(fm_dev->dev, &fm_dev->fm_vpu_et_pa);

	fm_dev->fm_vpu_et_max_va = init_buffer(fm_dev->dev,
					       &fm_dev->fm_vpu_et_max_pa);

	fm_dev->fm_vpu_cnt_va = init_buffer(fm_dev->dev,
					&fm_dev->fm_vpu_cnt_pa);
}
static void destroy_fm_dev_statistic_buf(struct mtk_fm_dev *fm_dev)
{
	/* wpe */
	deinit_buffer(fm_dev->dev, fm_dev->wpe_et_pa, fm_dev->wpe_et_va);
	deinit_buffer(fm_dev->dev, fm_dev->wpe_et_max_pa,
			fm_dev->wpe_et_max_va);
	deinit_buffer(fm_dev->dev, fm_dev->wpe_cnt_pa, fm_dev->wpe_cnt_va);
	/* fe */
	deinit_buffer(fm_dev->dev, fm_dev->fe_et_pa, fm_dev->fe_et_va);

	deinit_buffer(fm_dev->dev, fm_dev->fe_et_max_pa, fm_dev->fe_et_max_va);

	deinit_buffer(fm_dev->dev, fm_dev->fe_cnt_pa, fm_dev->fe_cnt_va);

	/* fms */
	deinit_buffer(fm_dev->dev, fm_dev->fms_delay_pa, fm_dev->fms_delay_va);

	deinit_buffer(fm_dev->dev, fm_dev->fms_delay_max_pa,
			fm_dev->fms_delay_max_va);

	deinit_buffer(fm_dev->dev, fm_dev->fms_et_pa, fm_dev->fms_et_va);

	deinit_buffer(fm_dev->dev, fm_dev->fms_et_max_pa,
			fm_dev->fms_et_max_va);

	/* feT */
	deinit_buffer(fm_dev->dev, fm_dev->feT_delay_pa, fm_dev->feT_delay_va);

	deinit_buffer(fm_dev->dev, fm_dev->feT_delay_max_pa,
			fm_dev->feT_delay_max_va);

	deinit_buffer(fm_dev->dev, fm_dev->feT_et_pa, fm_dev->feT_et_va);

	deinit_buffer(fm_dev->dev, fm_dev->feT_et_max_pa,
			fm_dev->feT_et_max_va);

	deinit_buffer(fm_dev->dev, fm_dev->feT_1_4_et_pa,
		      fm_dev->feT_1_4_et_va);

	deinit_buffer(fm_dev->dev, fm_dev->feT_1_4_et_max_pa,
		      fm_dev->feT_1_4_et_max_va);

	deinit_buffer(fm_dev->dev, fm_dev->feT_1_16_et_pa,
		      fm_dev->feT_1_16_et_va);

	deinit_buffer(fm_dev->dev, fm_dev->feT_1_16_et_max_pa,
		      fm_dev->feT_1_16_et_max_va);

	/* 1/16 sc */
	deinit_buffer(fm_dev->dev, fm_dev->sc_1_16_delay_pa,
			fm_dev->sc_1_16_delay_va);

	deinit_buffer(fm_dev->dev, fm_dev->sc_1_16_delay_max_pa,
			fm_dev->sc_1_16_delay_max_va);

	deinit_buffer(fm_dev->dev, fm_dev->sc_1_16_et_pa,
			fm_dev->sc_1_16_et_va);

	deinit_buffer(fm_dev->dev, fm_dev->sc_1_16_et_max_pa,
			fm_dev->sc_1_16_et_max_va);

	/* 1/16 fmt */
	deinit_buffer(fm_dev->dev, fm_dev->fmt_1_16_delay_pa,
			fm_dev->fmt_1_16_delay_va);

	deinit_buffer(fm_dev->dev, fm_dev->fmt_1_16_delay_max_pa,
			fm_dev->fmt_1_16_delay_max_va);

	deinit_buffer(fm_dev->dev, fm_dev->fmt_1_16_et_pa,
			fm_dev->fmt_1_16_et_va);

	deinit_buffer(fm_dev->dev, fm_dev->fmt_1_16_et_max_pa,
			fm_dev->fmt_1_16_et_max_va);

	/* 1/4 sc */
	deinit_buffer(fm_dev->dev, fm_dev->sc_1_4_delay_pa,
			fm_dev->sc_1_4_delay_va);

	deinit_buffer(fm_dev->dev, fm_dev->sc_1_4_delay_max_pa,
			fm_dev->sc_1_4_delay_max_va);

	deinit_buffer(fm_dev->dev, fm_dev->sc_1_4_et_pa,
			fm_dev->sc_1_4_et_va);

	deinit_buffer(fm_dev->dev, fm_dev->sc_1_4_et_max_pa,
			fm_dev->sc_1_4_et_max_va);

	/* 1/4 fmt */
	deinit_buffer(fm_dev->dev, fm_dev->fmt_1_4_delay_pa,
			fm_dev->fmt_1_4_delay_va);

	deinit_buffer(fm_dev->dev, fm_dev->fmt_1_4_delay_max_pa,
			fm_dev->fmt_1_4_delay_max_va);

	deinit_buffer(fm_dev->dev, fm_dev->fmt_1_4_et_pa,
			fm_dev->fmt_1_4_et_va);

	deinit_buffer(fm_dev->dev, fm_dev->fmt_1_4_et_max_pa,
			fm_dev->fmt_1_4_et_max_va);

	/* 1/1 sc */
	deinit_buffer(fm_dev->dev, fm_dev->sc_delay_pa, fm_dev->sc_delay_va);

	deinit_buffer(fm_dev->dev, fm_dev->sc_delay_max_pa,
			fm_dev->sc_delay_max_va);

	deinit_buffer(fm_dev->dev, fm_dev->sc_et_pa, fm_dev->sc_et_va);

	deinit_buffer(fm_dev->dev, fm_dev->sc_et_max_pa,
			fm_dev->sc_et_max_va);

	/* 1/1 fmt */
	deinit_buffer(fm_dev->dev, fm_dev->fmt_delay_pa, fm_dev->fmt_delay_va);

	deinit_buffer(fm_dev->dev, fm_dev->fmt_delay_max_pa,
			fm_dev->fmt_delay_max_va);

	deinit_buffer(fm_dev->dev, fm_dev->fmt_et_pa, fm_dev->fmt_et_va);

	deinit_buffer(fm_dev->dev, fm_dev->fmt_et_max_pa,
			fm_dev->fmt_et_max_va);

	/* headpose */
	deinit_buffer(fm_dev->dev, fm_dev->head_pose_delay_pa,
			fm_dev->head_pose_delay_va);

	deinit_buffer(fm_dev->dev, fm_dev->head_pose_delay_max_pa,
			fm_dev->head_pose_delay_max_va);

	deinit_buffer(fm_dev->dev, fm_dev->head_pose_et_pa,
			fm_dev->head_pose_et_va);

	deinit_buffer(fm_dev->dev, fm_dev->head_pose_et_max_pa,
			fm_dev->head_pose_et_max_va);

	/* fm vpu flow overall */
	deinit_buffer(fm_dev->dev, fm_dev->fm_vpu_et_pa, fm_dev->fm_vpu_et_va);

	deinit_buffer(fm_dev->dev, fm_dev->fm_vpu_et_max_pa,
			fm_dev->fm_vpu_et_max_va);

	deinit_buffer(fm_dev->dev, fm_dev->fm_vpu_cnt_pa,
			fm_dev->fm_vpu_cnt_va);
}

#if 0
static void print_exec_time(char *module_name, struct timeval sof,
	struct timeval frame_done, int expected)
{
	suseconds_t diff;

	diff = frame_done.tv_sec * 1000000 - sof.tv_sec * 1000000 +
		frame_done.tv_usec - sof.tv_usec;
	if (diff > expected)
		pr_err("%s Elapsed time: %lu us, exceeds %d\n",
			module_name, diff, expected);
	LOG(CV_LOG_INFO, "%s Elapsed time: %lu us\n", module_name, diff);
}

static void print_done_interval(char *module_name, struct timeval prev,
	struct timeval cur)
{
	suseconds_t diff;

	diff = cur.tv_sec * 1000000 - prev.tv_sec * 1000000 +
		cur.tv_usec - prev.tv_usec;
	LOG(CV_LOG_INFO, "%s Done intarval: %lu us\n", module_name, diff);
	if (diff > 30000)
		pr_err("flow done interval:%lu !!\n", diff);
}
#endif

static void dump_cv_buffers(struct mtk_fm_dev *fm_dev, char *dump_path,
			    int frame_idx)
{
	char file_path[256];

	/* start to dump buffer */
	snprintf(file_path, sizeof(file_path), "%s/feo/dump_fe1_L_%d.bin",
		 dump_path, frame_idx);
	dump_buf(fm_dev, MTK_FM_DEV_BUF_TYPE_FE_L_POINT_R0, fm_dev->blk_nr * 8,
		 file_path);
	snprintf(file_path, sizeof(file_path), "%s/feo/dump_fe2_L_%d.bin",
		 dump_path, frame_idx);
	dump_buf(fm_dev, MTK_FM_DEV_BUF_TYPE_FE_L_DESC_R0, fm_dev->blk_nr * 64,
		 file_path);
	if (fm_dev->img_num > 1) {
		snprintf(file_path, sizeof(file_path),
			 "%s/feo/dump_fe1_R_%d.bin", dump_path,
			 frame_idx);
		dump_buf(fm_dev, MTK_FM_DEV_BUF_TYPE_FE_R_POINT_R0,
			 fm_dev->blk_nr * 8, file_path);
		snprintf(file_path, sizeof(file_path),
			 "%s/feo/dump_fe2_R_%d.bin", dump_path,
			 frame_idx);
		dump_buf(fm_dev, MTK_FM_DEV_BUF_TYPE_FE_R_DESC_R0,
			 fm_dev->blk_nr * 64, file_path);
	}
	snprintf(file_path, sizeof(file_path), "%s/feT/dump_feT1_%d.bin",
		 dump_path, frame_idx);
	dump_buf(fm_dev, MTK_FM_DEV_BUF_TYPE_FET_P, fm_dev->blk_nr * 8,
		 file_path);
	snprintf(file_path, sizeof(file_path), "%s/feT/dump_feT2_%d.bin",
		 dump_path, frame_idx);
	dump_buf(fm_dev, MTK_FM_DEV_BUF_TYPE_FET_D, fm_dev->blk_nr * 64,
		 file_path);
	snprintf(file_path, sizeof(file_path), "%s/wdma/dump_wdma0_%d.bin",
		 dump_path, frame_idx);
	dump_buf(fm_dev, MTK_FM_DEV_BUF_TYPE_WDMA_R0,
		 fm_dev->buf[MTK_FM_DEV_BUF_TYPE_WDMA_R0]->pitch * fm_dev->fe_h,
		 file_path);
#if 0
	snprintf(file_path, sizeof(file_path), "dump_wdma0_dummy_%d.bin",
		 frame_idx);
	dump_buf(fm_dev, MTK_FM_DEV_BUF_TYPE_WDMA_1,
		fm_dev->buf[MTK_FM_DEV_BUF_TYPE_WDMA_1]->pitch * 640,
		file_path);
#endif
	if (fm_dev->fmdevmask & FM_DEV_SCALED_FMT_ENABLE) {
	snprintf(file_path, sizeof(file_path), "%s/feT/dump_1_4_feT1_%d.bin",
		 dump_path, frame_idx);
	dump_buf(fm_dev, MTK_FM_DEV_BUF_TYPE_1_4_FET_P, fm_dev->blk_nr * 8,
		 file_path);
	snprintf(file_path, sizeof(file_path), "%s/feT/dump_1_4_feT2_%d.bin",
		 dump_path, frame_idx);
	dump_buf(fm_dev, MTK_FM_DEV_BUF_TYPE_1_4_FET_D, fm_dev->blk_nr * 64,
		 file_path);
	snprintf(file_path, sizeof(file_path), "%s/wdma/dump_wdma1_%d.bin",
		 dump_path, frame_idx);
	dump_buf(fm_dev, MTK_FM_DEV_BUF_TYPE_1_4_WDMA_R0,
		 fm_dev->buf[MTK_FM_DEV_BUF_TYPE_1_4_WDMA_R0]->pitch *
		 fm_dev->fe_h / 2, file_path);
	snprintf(file_path, sizeof(file_path),
		 "%s/wdma/dump_wdma1_dummy_%d.bin", dump_path, frame_idx);
	dump_buf(fm_dev, MTK_FM_DEV_BUF_TYPE_1_4_WDMA_R1,
		 fm_dev->buf[MTK_FM_DEV_BUF_TYPE_1_4_WDMA_R1]->pitch *
		 fm_dev->fe_h / 2, file_path);

	snprintf(file_path, sizeof(file_path), "%s/feT/dump_1_16_feT1_%d.bin",
		 dump_path, frame_idx);
	dump_buf(fm_dev, MTK_FM_DEV_BUF_TYPE_1_16_FET_P, fm_dev->blk_nr * 8,
		 file_path);
	snprintf(file_path, sizeof(file_path), "%s/feT/dump_1_16_feT2_%d.bin",
		 dump_path, frame_idx);
	dump_buf(fm_dev, MTK_FM_DEV_BUF_TYPE_1_16_FET_D, fm_dev->blk_nr * 64,
		 file_path);
	snprintf(file_path, sizeof(file_path), "%s/wdma/dump_wdma2_%d.bin",
		 dump_path, frame_idx);
	dump_buf(fm_dev, MTK_FM_DEV_BUF_TYPE_1_16_WDMA_R0,
		 fm_dev->buf[MTK_FM_DEV_BUF_TYPE_1_16_WDMA_R0]->pitch *
		 fm_dev->fe_h / 4, file_path);
	snprintf(file_path, sizeof(file_path),
		"%s/wdma/dump_wdma2_dummy_%d.bin", dump_path, frame_idx);
	dump_buf(fm_dev, MTK_FM_DEV_BUF_TYPE_1_16_WDMA_R1,
		 fm_dev->buf[MTK_FM_DEV_BUF_TYPE_1_16_WDMA_R1]->pitch *
		 fm_dev->fe_h / 4, file_path);
	}

	snprintf(file_path, sizeof(file_path), "%s/fmo/dump_fmso1_%d.bin",
		 dump_path, frame_idx);
	dump_buf(fm_dev, MTK_FM_DEV_BUF_TYPE_FM_OUT_S_R0,
		 roundup(fm_dev->blk_nr * 10 + 128, 128), file_path);
	snprintf(file_path, sizeof(file_path), "%s/fmo/dump_fmso2_%d.bin",
		 dump_path, frame_idx);
	dump_buf(fm_dev, MTK_FM_DEV_BUF_TYPE_FM_ZNCC_S_R0, fm_dev->blk_nr * 64,
		 file_path);

	snprintf(file_path, sizeof(file_path), "%s/fmo/dump_fmto1_%d.bin",
		 dump_path, frame_idx);
	dump_buf(fm_dev, MTK_FM_DEV_BUF_TYPE_FM_OUT_T_R0,
		 roundup(fm_dev->blk_nr * 10 + 128, 128), file_path);
	snprintf(file_path, sizeof(file_path), "%s/fmo/dump_fmto2_%d.bin",
		 dump_path, frame_idx);
	dump_buf(fm_dev, MTK_FM_DEV_BUF_TYPE_FM_ZNCC_T_R0, fm_dev->blk_nr * 64,
		 file_path);

	if (fm_dev->fmdevmask & FM_DEV_SCALED_FMT_ENABLE) {
	snprintf(file_path, sizeof(file_path), "%s/fmo/dump_1_4_fmto1_%d.bin",
		dump_path, frame_idx);
	dump_buf(fm_dev, MTK_FM_DEV_BUF_TYPE_1_4_FM_OUT_T_R0,
		 roundup(fm_dev->blk_nr * 10 + 128, 128), file_path);
	snprintf(file_path, sizeof(file_path), "%s/fmo/dump_1_4_fmto2_%d.bin",
		 dump_path, frame_idx);
	dump_buf(fm_dev, MTK_FM_DEV_BUF_TYPE_1_4_FM_ZNCC_T_R0,
		 fm_dev->blk_nr * 64, file_path);

	snprintf(file_path, sizeof(file_path), "%s/fmo/dump_1_16_fmto1_%d.bin",
		 dump_path, frame_idx);
	dump_buf(fm_dev, MTK_FM_DEV_BUF_TYPE_1_16_FM_OUT_T_R0,
		 roundup(fm_dev->blk_nr * 10 + 128, 128), file_path);
	snprintf(file_path, sizeof(file_path), "%s/fmo/dump_1_16_fmto2_%d.bin",
		 dump_path, frame_idx);
	dump_buf(fm_dev, MTK_FM_DEV_BUF_TYPE_1_16_FM_ZNCC_T_R0,
		 fm_dev->blk_nr * 64, file_path);
	}
	fm_dev->one_shot_dump = 0;
}

static int flush_check(struct mtk_fm_dev *fm_dev, struct cmdq_client *client,
	struct cmdq_pkt *pkt, enum mtk_fm_dev_flush_mode mode)
{
	switch (mode) {
	case WARPA_WORK:
		cmdq_pkt_flush_async(client, pkt, warpa_work_cb, fm_dev);
		fm_dev->warpa_flush_wait = 1;
		wait_for_completion_interruptible(
			&fm_dev->warpa_work_comp.cmplt);
		fm_dev->warpa_flush_wait = 0;
		if (!fm_dev->polling_wait) {
			LOG(CV_LOG_DBG, "warpa et quit flow!\n");
			return -1;
		}
		break;
	case FE_WORK:
		cmdq_pkt_flush_async(client, pkt, fe_work_cb, fm_dev);
		fm_dev->fe_flush_wait = 1;
		wait_for_completion_interruptible(
			&fm_dev->fe_work_comp.cmplt);
		fm_dev->fe_flush_wait = 0;
		if (!fm_dev->polling_wait) {
			LOG(CV_LOG_DBG, "fe et quit flow!\n");
			return -1;
		}
		break;
	case FM_WORK:
		cmdq_pkt_flush_async(client, pkt, fm_work_cb, fm_dev);
		fm_dev->fm_flush_wait = 1;
		wait_for_completion_interruptible(
			&fm_dev->fm_work_comp.cmplt);
		fm_dev->fm_flush_wait = 0;
		if (!fm_dev->polling_wait) {
			LOG(CV_LOG_DBG, "fm work quit flow!\n");
			return -1;
		}
		break;
	case FM_WORK_NO_INT:
		cmdq_pkt_flush_async(client, pkt, fm_work_cb, fm_dev);
		wait_for_completion_interruptible(
			&fm_dev->fm_work_comp.cmplt);
		if (!fm_dev->polling_wait) {
			fm_dev->fm_flush_wait = 1;
			LOG(CV_LOG_DBG, "fm work quit flow!\n");
			return -1;
		}
		break;
	case WEN0_WORK:
		cmdq_pkt_flush_async(client, pkt, wen0_work_cb, fm_dev);
		fm_dev->wen0_flush_wait = 1;
		wait_for_completion_interruptible(
			&fm_dev->wen0_work_comp.cmplt);
		fm_dev->wen0_flush_wait = 0;
		if (!fm_dev->polling_wait) {
			LOG(CV_LOG_DBG, "wen0 work quit flow!\n");
			return -1;
		}
		break;
	case WEN1_WORK:
		cmdq_pkt_flush_async(client, pkt, wen1_work_cb, fm_dev);
		fm_dev->wen1_flush_wait = 1;
		wait_for_completion_interruptible(
			&fm_dev->wen1_work_comp.cmplt);
		fm_dev->wen1_flush_wait = 0;
		if (!fm_dev->polling_wait) {
			LOG(CV_LOG_DBG, "wen1 work quit flow!\n");
			return -1;
		}
		break;
	case WEN2_WORK:
		cmdq_pkt_flush_async(client, pkt, wen2_work_cb, fm_dev);
		fm_dev->wen2_flush_wait = 1;
		wait_for_completion_interruptible(
			&fm_dev->wen2_work_comp.cmplt);
		fm_dev->wen2_flush_wait = 0;
		if (!fm_dev->polling_wait) {
			LOG(CV_LOG_DBG, "wen2 work quit flow!\n");
			return -1;
		}
		break;
	case WEN3_WORK:
		cmdq_pkt_flush_async(client, pkt, wen3_work_cb, fm_dev);
		fm_dev->wen3_flush_wait = 1;
		wait_for_completion_interruptible(
			&fm_dev->wen3_work_comp.cmplt);
		fm_dev->wen3_flush_wait = 0;
		if (!fm_dev->polling_wait) {
			LOG(CV_LOG_DBG, "wen3 work quit flow!\n");
			return -1;
		}
		break;
	case REN_WORK:
		cmdq_pkt_flush_async(client, pkt, ren_work_cb, fm_dev);
		fm_dev->ren_flush_wait = 1;
		wait_for_completion_interruptible(
			&fm_dev->ren_work_comp.cmplt);
		fm_dev->ren_flush_wait = 0;
		if (!fm_dev->polling_wait) {
			LOG(CV_LOG_DBG, "ren work quit flow!\n");
			return -1;
		}
		break;
	case BE0_WORK:
		cmdq_pkt_flush_async(client, pkt, be0_work_cb, fm_dev);
		fm_dev->be0_flush_wait = 1;
		wait_for_completion_interruptible(
			&fm_dev->be0_work_comp.cmplt);
		fm_dev->be0_flush_wait = 0;
		if (!fm_dev->polling_wait) {
			dev_dbg(fm_dev->dev, "be0 work quit flow!\n");
			return -1;
		}
		break;
	case BE1_WORK:
		cmdq_pkt_flush_async(client, pkt, be1_work_cb, fm_dev);
		fm_dev->be1_flush_wait = 1;
		wait_for_completion_interruptible(
			&fm_dev->be1_work_comp.cmplt);
		fm_dev->be1_flush_wait = 0;
		if (!fm_dev->polling_wait) {
			dev_dbg(fm_dev->dev, "be1 work quit flow!\n");
			return -1;
		}
		break;
	case BE2_WORK:
		cmdq_pkt_flush_async(client, pkt, be2_work_cb, fm_dev);
		fm_dev->be2_flush_wait = 1;
		wait_for_completion_interruptible(
			&fm_dev->be2_work_comp.cmplt);
		fm_dev->be2_flush_wait = 0;
		if (!fm_dev->polling_wait) {
			dev_dbg(fm_dev->dev, "be2 work quit flow!\n");
			return -1;
		}
		break;
	case BE3_WORK:
		cmdq_pkt_flush_async(client, pkt, be3_work_cb, fm_dev);
		fm_dev->be3_flush_wait = 1;
		wait_for_completion_interruptible(
			&fm_dev->be3_work_comp.cmplt);
		fm_dev->be3_flush_wait = 0;
		if (!fm_dev->polling_wait) {
			dev_dbg(fm_dev->dev, "be3 work quit flow!\n");
			return -1;
		}
		break;
	default:
		dev_err(fm_dev->dev, "error flush mode %d", mode);
		return -EINVAL;
	}
	return 0;
}

static void mtk_rbfc_wen0_incomp_work(struct work_struct *work)
{
	struct mtk_fm_dev *fm_dev =
			container_of(work, struct mtk_fm_dev, wen0_incomp_work);
	u32 wen_incomp_event =
		fm_dev->cmdq_events[MTK_FM_DEV_CMDQ_EVENT_P2_WEN0_INCOMP];
	int ret = 0;

	init_completion(&fm_dev->wen0_work_comp.cmplt);
	while (fm_dev->polling_wait) {
		LOG(CV_LOG_DBG, "wen0_work: start wait:%d\n",
			wen_incomp_event);

		reinit_completion(&fm_dev->wen0_work_comp.cmplt);
		cmdq_pkt_create(&fm_dev->cmdq_pkt_wen0);
		/* wait for fe done */
		cmdq_pkt_clear_event(fm_dev->cmdq_pkt_wen0, wen_incomp_event);
		cmdq_pkt_wfe(fm_dev->cmdq_pkt_wen0, wen_incomp_event);
		ret = flush_check(fm_dev, fm_dev->cmdq_client_wen0,
			fm_dev->cmdq_pkt_wen0, WEN0_WORK);
		if (ret)
			break;
		LOG(CV_LOG_ERR, "rbfc wen0 incomplete occurred!\n");
	}
	LOG(CV_LOG_DBG, "wen0_incomp_work: exit\n");
}

static void mtk_rbfc_wen1_incomp_work(struct work_struct *work)
{
	struct mtk_fm_dev *fm_dev =
			container_of(work, struct mtk_fm_dev, wen1_incomp_work);
	u32 wen_incomp_event =
		fm_dev->cmdq_events[MTK_FM_DEV_CMDQ_EVENT_P2_WEN1_INCOMP];
	int ret = 0;

	init_completion(&fm_dev->wen1_work_comp.cmplt);
	while (fm_dev->polling_wait) {
		LOG(CV_LOG_DBG, "wen1_work: start wait:%d\n",
			wen_incomp_event);

		reinit_completion(&fm_dev->wen1_work_comp.cmplt);
		cmdq_pkt_create(&fm_dev->cmdq_pkt_wen1);
		/* wait for fe done */
		cmdq_pkt_clear_event(fm_dev->cmdq_pkt_wen1, wen_incomp_event);
		cmdq_pkt_wfe(fm_dev->cmdq_pkt_wen1, wen_incomp_event);
		ret = flush_check(fm_dev, fm_dev->cmdq_client_wen1,
			fm_dev->cmdq_pkt_wen1, WEN1_WORK);
		if (ret)
			break;
		LOG(CV_LOG_ERR, "rbfc wen1 incomplete occurred!\n");
	}
	LOG(CV_LOG_DBG, "wen1_incomp_work: exit\n");
}

static void mtk_rbfc_wen2_incomp_work(struct work_struct *work)
{
	struct mtk_fm_dev *fm_dev =
			container_of(work, struct mtk_fm_dev, wen2_incomp_work);
	u32 wen_incomp_event =
		fm_dev->cmdq_events[MTK_FM_DEV_CMDQ_EVENT_P2_WEN2_INCOMP];
	int ret = 0;

	init_completion(&fm_dev->wen2_work_comp.cmplt);
	while (fm_dev->polling_wait) {
		LOG(CV_LOG_DBG, "wen2_work: start wait:%d\n",
			wen_incomp_event);

		reinit_completion(&fm_dev->wen2_work_comp.cmplt);
		cmdq_pkt_create(&fm_dev->cmdq_pkt_wen2);
		/* wait for fe done */
		cmdq_pkt_clear_event(fm_dev->cmdq_pkt_wen2, wen_incomp_event);
		cmdq_pkt_wfe(fm_dev->cmdq_pkt_wen2, wen_incomp_event);
		ret = flush_check(fm_dev, fm_dev->cmdq_client_wen2,
			fm_dev->cmdq_pkt_wen2, WEN2_WORK);
		if (ret)
			break;
		LOG(CV_LOG_ERR, "rbfc wen2 incomplete occurred!\n");
	}
	LOG(CV_LOG_DBG, "wen2_incomp_work: exit\n");
}

static void mtk_rbfc_wen3_incomp_work(struct work_struct *work)
{
	struct mtk_fm_dev *fm_dev =
			container_of(work, struct mtk_fm_dev, wen3_incomp_work);
	u32 wen_incomp_event =
		fm_dev->cmdq_events[MTK_FM_DEV_CMDQ_EVENT_P2_WEN3_INCOMP];
	int ret = 0;

	init_completion(&fm_dev->wen3_work_comp.cmplt);
	while (fm_dev->polling_wait) {
		LOG(CV_LOG_DBG, "wen3_work: start wait:%d\n",
			wen_incomp_event);

		reinit_completion(&fm_dev->wen3_work_comp.cmplt);
		cmdq_pkt_create(&fm_dev->cmdq_pkt_wen3);
		/* wait for fe done */
		cmdq_pkt_clear_event(fm_dev->cmdq_pkt_wen3, wen_incomp_event);
		cmdq_pkt_wfe(fm_dev->cmdq_pkt_wen3, wen_incomp_event);
		ret = flush_check(fm_dev, fm_dev->cmdq_client_wen3,
			fm_dev->cmdq_pkt_wen3, WEN3_WORK);
		if (ret)
			break;
		LOG(CV_LOG_ERR, "rbfc wen3 incomplete occurred!\n");
	}
	LOG(CV_LOG_DBG, "wen3_incomp_work: exit\n");
}

static void mtk_rbfc_ren_incomp_work(struct work_struct *work)
{
	struct mtk_fm_dev *fm_dev =
			container_of(work, struct mtk_fm_dev, ren_incomp_work);
	u32 ren_incomp_event =
		fm_dev->cmdq_events[MTK_FM_DEV_CMDQ_EVENT_WPE_REN_INCOMP];
	int ret = 0;

	init_completion(&fm_dev->ren_work_comp.cmplt);
	while (fm_dev->polling_wait) {
		LOG(CV_LOG_DBG, "ren_work: start wait:%d\n",
			ren_incomp_event);

		reinit_completion(&fm_dev->ren_work_comp.cmplt);
		cmdq_pkt_create(&fm_dev->cmdq_pkt_ren);
		/* wait for fe done */
		cmdq_pkt_clear_event(fm_dev->cmdq_pkt_ren, ren_incomp_event);
		cmdq_pkt_wfe(fm_dev->cmdq_pkt_ren, ren_incomp_event);
		ret = flush_check(fm_dev, fm_dev->cmdq_client_ren,
			fm_dev->cmdq_pkt_ren, REN_WORK);
		if (ret)
			break;
		LOG(CV_LOG_ERR, "rbfc ren incomplete occurred!\n");
	}
	dev_dbg(fm_dev->dev, "ren_incomp_work: exit\n");
}

static void mtk_be0_lim_work(struct work_struct *work)
{
	struct mtk_fm_dev *fm_dev =
			container_of(work, struct mtk_fm_dev, be0_lim_work);
	u32 be_lim_event =
		fm_dev->cmdq_events[MTK_FM_DEV_CMDQ_EVENT_BE0_LIM];
	int ret = 0;

	init_completion(&fm_dev->be0_work_comp.cmplt);
	fm_dev->be0_lim_cnt = 0;
	while (fm_dev->polling_wait) {
		LOG(CV_LOG_DBG, "be0_work: start wait:%d\n",
			be_lim_event);

		reinit_completion(&fm_dev->be0_work_comp.cmplt);
		cmdq_pkt_create(&fm_dev->cmdq_pkt_be0);
		cmdq_pkt_clear_event(fm_dev->cmdq_pkt_be0, be_lim_event);
		cmdq_pkt_wfe(fm_dev->cmdq_pkt_be0, be_lim_event);
		ret = flush_check(fm_dev, fm_dev->cmdq_client_be0,
			fm_dev->cmdq_pkt_be0, BE0_WORK);
		if (ret)
			break;
		LOG(CV_LOG_ERR, "be0 algo reaches limitation!\n");
		fm_dev->be0_lim_cnt++;
	}
	dev_dbg(fm_dev->dev, "be0_incomp_work: exit\n");
}

static void mtk_be1_lim_work(struct work_struct *work)
{
	struct mtk_fm_dev *fm_dev =
			container_of(work, struct mtk_fm_dev, be1_lim_work);
	u32 be_lim_event =
		fm_dev->cmdq_events[MTK_FM_DEV_CMDQ_EVENT_BE1_LIM];
	int ret = 0;

	init_completion(&fm_dev->be1_work_comp.cmplt);
	fm_dev->be1_lim_cnt = 0;
	while (fm_dev->polling_wait) {
		LOG(CV_LOG_DBG, "be1_work: start wait:%d\n",
			be_lim_event);

		reinit_completion(&fm_dev->be1_work_comp.cmplt);
		cmdq_pkt_create(&fm_dev->cmdq_pkt_be1);
		cmdq_pkt_clear_event(fm_dev->cmdq_pkt_be1, be_lim_event);
		cmdq_pkt_wfe(fm_dev->cmdq_pkt_be1, be_lim_event);
		ret = flush_check(fm_dev, fm_dev->cmdq_client_be1,
			fm_dev->cmdq_pkt_be1, BE1_WORK);
		if (ret)
			break;
		LOG(CV_LOG_ERR, "be1 algo reaches limitation!\n");
		fm_dev->be1_lim_cnt++;
	}
	dev_dbg(fm_dev->dev, "be1_incomp_work: exit\n");
}

static void mtk_be2_lim_work(struct work_struct *work)
{
	struct mtk_fm_dev *fm_dev =
			container_of(work, struct mtk_fm_dev, be2_lim_work);
	u32 be_lim_event =
		fm_dev->cmdq_events[MTK_FM_DEV_CMDQ_EVENT_BE2_LIM];
	int ret = 0;

	init_completion(&fm_dev->be2_work_comp.cmplt);
	fm_dev->be2_lim_cnt = 0;
	while (fm_dev->polling_wait) {
		LOG(CV_LOG_DBG, "be2_work: start wait:%d\n",
			be_lim_event);

		reinit_completion(&fm_dev->be2_work_comp.cmplt);
		cmdq_pkt_create(&fm_dev->cmdq_pkt_be2);
		cmdq_pkt_clear_event(fm_dev->cmdq_pkt_be2, be_lim_event);
		cmdq_pkt_wfe(fm_dev->cmdq_pkt_be2, be_lim_event);
		ret = flush_check(fm_dev, fm_dev->cmdq_client_be2,
			fm_dev->cmdq_pkt_be2, BE2_WORK);
		if (ret)
			break;
		LOG(CV_LOG_ERR, "be2 algo reaches limitation!\n");
		fm_dev->be2_lim_cnt++;
	}
	dev_dbg(fm_dev->dev, "be2_incomp_work: exit\n");
}

static void mtk_be3_lim_work(struct work_struct *work)
{
	struct mtk_fm_dev *fm_dev =
			container_of(work, struct mtk_fm_dev, be3_lim_work);
	u32 be_lim_event =
		fm_dev->cmdq_events[MTK_FM_DEV_CMDQ_EVENT_BE3_LIM];
	int ret = 0;

	init_completion(&fm_dev->be3_work_comp.cmplt);
	fm_dev->be3_lim_cnt = 0;
	while (fm_dev->polling_wait) {
		LOG(CV_LOG_DBG, "be3_work: start wait:%d\n",
			be_lim_event);

		reinit_completion(&fm_dev->be3_work_comp.cmplt);
		cmdq_pkt_create(&fm_dev->cmdq_pkt_be3);
		cmdq_pkt_clear_event(fm_dev->cmdq_pkt_be3, be_lim_event);
		cmdq_pkt_wfe(fm_dev->cmdq_pkt_be3, be_lim_event);
		ret = flush_check(fm_dev, fm_dev->cmdq_client_be3,
			fm_dev->cmdq_pkt_be3, BE3_WORK);
		if (ret)
			break;
		LOG(CV_LOG_ERR, "be3 algo reaches limitation!\n");
		fm_dev->be3_lim_cnt++;
	}
	dev_dbg(fm_dev->dev, "be3_incomp_work: exit\n");
}


static void mtk_warpa_execution_time_work(struct work_struct *work)
{
	struct mtk_fm_dev *fm_dev =
			container_of(work, struct mtk_fm_dev, warpa_et_work);
	u32 warpa_sof_event =
		fm_dev->cmdq_events[MTK_FM_DEV_CMDQ_EVENT_SOF_WARPA];
	u32 warpa_done_event =
		fm_dev->cmdq_events[MTK_FM_DEV_CMDQ_EVENT_FRAME_DONE_WARPA];
	int ret = 0;
	s32 j_offset = 0;
	u8 subsys = SUBSYS_102DXXXX;
	u8 gce4_subsys = SUBSYS_1031XXXX;

	init_completion(&fm_dev->warpa_work_comp.cmplt);
	while (fm_dev->polling_wait) {
		LOG(CV_LOG_DBG, "warpa_et_work: start wait:%d\n",
			warpa_sof_event);

		reinit_completion(&fm_dev->warpa_work_comp.cmplt);
		cmdq_pkt_create(&fm_dev->cmdq_pkt_warpa);
		/* clear max exec time */
		cmdq_pkt_assign_command(fm_dev->cmdq_pkt_warpa,
					GCE_wpe_max_exec, 0);
		/* clear wpe frame cnt */
		cmdq_pkt_assign_command(fm_dev->cmdq_pkt_warpa,
					GCE_wpe_done_cnt, 0);
		/* clear wpe sof & done event */
		cmdq_pkt_clear_event(fm_dev->cmdq_pkt_warpa, warpa_sof_event);
		cmdq_pkt_clear_event(fm_dev->cmdq_pkt_warpa, warpa_done_event);
		/* wait wpe sof event */
		cmdq_pkt_wfe(fm_dev->cmdq_pkt_warpa, warpa_sof_event);
		cmdq_pkt_set_token(fm_dev->cmdq_pkt_warpa,
				GCE_SW_TOKEN_SIDE_WPE_SOF, gce4_subsys);
		/* get ts -> SPR0 */
		cmdq_pkt_read(fm_dev->cmdq_pkt_warpa, subsys, 0x3020, GCE_SPR0);
		/* wait wpe done event */
		cmdq_pkt_wfe(fm_dev->cmdq_pkt_warpa, warpa_done_event);
		cmdq_pkt_set_token(fm_dev->cmdq_pkt_warpa,
				GCE_SW_TOKEN_SIDE_WPE_DONE, gce4_subsys);
		/* get ts -> SPR1 */
		cmdq_pkt_read(fm_dev->cmdq_pkt_warpa, subsys, 0x3020, GCE_SPR1);
		/* append exec time cmd */
		append_exec_time_gce_cmd(fm_dev->cmdq_pkt_warpa, GCE_SPR0,
			GCE_SPR1, GCE_SPR2, fm_dev->wpe_et_pa);
		/* update max exec time */
		append_max_exec_time_gce_cmd(fm_dev->cmdq_pkt_warpa, GCE_SPR2,
			GCE_wpe_max_exec, fm_dev->wpe_et_max_pa);
		/* wpe frame cnt++ */
		append_inc_frame_cnt_cmd(fm_dev->cmdq_pkt_warpa,
			GCE_wpe_done_cnt, fm_dev->wpe_cnt_pa);
		/* while loop above cmds, skip clear max & frame cnt */
		j_offset = (2 * CMDQ_INST_SIZE -
			cmdq_get_cmd_buf_size(fm_dev->cmdq_pkt_warpa));
		cmdq_pkt_jump(fm_dev->cmdq_pkt_warpa, j_offset);
		ret = flush_check(fm_dev, fm_dev->cmdq_client_warpa,
			fm_dev->cmdq_pkt_warpa, WARPA_WORK);
		if (ret)
			break;
	}

	LOG(CV_LOG_DBG, "warpa_et_work: exit\n");
}

static void mtk_fe_execution_time_work(struct work_struct *work)
{
	struct mtk_fm_dev *fm_dev =
			container_of(work, struct mtk_fm_dev, fe_et_work);
	u32 fe_sof_event =
		fm_dev->cmdq_events[MTK_FM_DEV_CMDQ_EVENT_SOF_FE];
	u32 fe_done_event =
		fm_dev->cmdq_events[MTK_FM_DEV_CMDQ_EVENT_FRAME_DONE_FE];
	u32 wdma0_done_event =
		fm_dev->cmdq_events[MTK_FM_DEV_CMDQ_EVENT_FRAME_DONE_WDMA0];
	u32 wdma1_done_event =
		fm_dev->cmdq_events[MTK_FM_DEV_CMDQ_EVENT_FRAME_DONE_WDMA1];
	u32 wdma2_done_event =
		fm_dev->cmdq_events[MTK_FM_DEV_CMDQ_EVENT_FRAME_DONE_WDMA2];
	int ret = 0;
	s32 j_offset = 0;
	u8 subsys = SUBSYS_102DXXXX;
	u8 gce4_subsys = SUBSYS_1031XXXX;

	init_completion(&fm_dev->fe_work_comp.cmplt);
	while (fm_dev->polling_wait) {
		LOG(CV_LOG_DBG, "fe_et_work: start wait:%d\n",
				fe_sof_event);
		reinit_completion(&fm_dev->fe_work_comp.cmplt);
		cmdq_pkt_create(&fm_dev->cmdq_pkt_fe);
		/* clear fe max exec time */
		cmdq_pkt_assign_command(fm_dev->cmdq_pkt_fe,
					GCE_fe_max_exec, 0);
		/* clear fe frame cnt */
		cmdq_pkt_assign_command(fm_dev->cmdq_pkt_fe,
					GCE_fe_done_cnt, 0);
		/* wait for fe sof */
		cmdq_pkt_clear_event(fm_dev->cmdq_pkt_fe, fe_sof_event);
		/* wait for fe done */
		cmdq_pkt_clear_event(fm_dev->cmdq_pkt_fe, fe_done_event);
		/* wait for wdma0 done */
		cmdq_pkt_clear_event(fm_dev->cmdq_pkt_fe, wdma0_done_event);
		/* wait for wdma1 done */
		cmdq_pkt_clear_event(fm_dev->cmdq_pkt_fe, wdma1_done_event);
		/* wait for wdma2 done */
		cmdq_pkt_clear_event(fm_dev->cmdq_pkt_fe, wdma2_done_event);
		cmdq_pkt_wfe(fm_dev->cmdq_pkt_fe, fe_sof_event);
		cmdq_pkt_set_token(fm_dev->cmdq_pkt_fe, GCE_SW_TOKEN_FE_SOF,
				gce4_subsys);
		cmdq_pkt_read(fm_dev->cmdq_pkt_fe, subsys, 0x3020,
				GCE_fe_sof_ts);
		cmdq_pkt_wfe(fm_dev->cmdq_pkt_fe, fe_done_event);
		cmdq_pkt_set_token(fm_dev->cmdq_pkt_fe, GCE_SW_TOKEN_FE_DONE,
				gce4_subsys);
		/* get fe done start ts -> CPR (fm_vpu flow start) */
		cmdq_pkt_read(fm_dev->cmdq_pkt_fe, subsys, 0x3020,
				GCE_fe_done_ts);
		/* SPR1 = fe done - fe sof */
		append_exec_time_gce_cmd(fm_dev->cmdq_pkt_fe, GCE_fe_sof_ts,
			GCE_fe_done_ts, GCE_SPR1, fm_dev->fe_et_pa);
		/* if SPR1 > max exec time GCE_fe_max_exec, update it */
		append_max_exec_time_gce_cmd(fm_dev->cmdq_pkt_fe, GCE_SPR1,
			GCE_fe_max_exec, fm_dev->fe_et_max_pa);
		/* fe done cnt +1 */
		append_inc_frame_cnt_cmd(fm_dev->cmdq_pkt_fe, GCE_fe_done_cnt,
			fm_dev->fe_cnt_pa);
		cmdq_pkt_wfe(fm_dev->cmdq_pkt_fe, wdma0_done_event);
		cmdq_pkt_set_token(fm_dev->cmdq_pkt_fe, GCE_SW_TOKEN_WDMA0_DONE,
				gce4_subsys);
		if (fm_dev->fmdevmask & FM_DEV_SCALED_FMT_ENABLE) {
			cmdq_pkt_wfe(fm_dev->cmdq_pkt_fe, wdma1_done_event);
			cmdq_pkt_set_token(fm_dev->cmdq_pkt_fe,
					GCE_SW_TOKEN_WDMA1_DONE, gce4_subsys);
			cmdq_pkt_wfe(fm_dev->cmdq_pkt_fe, wdma2_done_event);
			cmdq_pkt_set_token(fm_dev->cmdq_pkt_fe,
					GCE_SW_TOKEN_WDMA2_DONE, gce4_subsys);
		}
		/* KICK FM_VPU */
		cmdq_pkt_set_token(fm_dev->cmdq_pkt_fe,
					GCE_SW_TOKEN_KICK_FM_VPU, gce4_subsys);
		j_offset = (2 * CMDQ_INST_SIZE -
			cmdq_get_cmd_buf_size(fm_dev->cmdq_pkt_fe));
		cmdq_pkt_jump(fm_dev->cmdq_pkt_fe, j_offset);
		ret = flush_check(fm_dev, fm_dev->cmdq_client_fe,
			fm_dev->cmdq_pkt_fe, FE_WORK);
		if (ret)
			break;
	}
	LOG(CV_LOG_DBG, "fe_et_work: exit\n");
}

static void mtk_fm_dev_trigger_work(struct work_struct *work)
{
	struct mtk_fm_dev *fm_dev =
			container_of(work, struct mtk_fm_dev, trigger_work);
	u32 fm_done_event =
		fm_dev->cmdq_events[MTK_FM_DEV_CMDQ_EVENT_FRAME_DONE_FM];
	u32 vpu_done_event =
		fm_dev->cmdq_events[MTK_FM_DEV_CMDQ_EVENT_FRAME_DONE_VPU_CORE0];
	struct device_node *node = NULL;
	struct platform_device *device = NULL;
	int frame_idx = 0;
	int ret = 0;
	void *dummy_sc_va = NULL;
	void *imu_data = NULL;
	struct mtk_fm *fm = dev_get_drvdata(fm_dev->dev_fm);
	struct mtk_mmsys_cmmn_top *top =
		dev_get_drvdata(fm_dev->dev_mmsys_cmmn_top);
	u8 subsys = SUBSYS_102DXXXX;

	node = of_find_compatible_node(NULL, NULL,
		"mediatek,mt3612-ivp");
	device = of_find_device_by_node(node);
	dummy_sc_va = kzalloc(roundup(fm_dev->sc_size, PAGE_SIZE), GFP_KERNEL);

	/*** init gce counter ***/
	init_completion(&fm_dev->fm_work_comp.cmplt);
	cmdq_pkt_create(&fm_dev->cmdq_pkt);
	/* clear fms max delay time */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_fms_max_delay, 0);
	/* clear fms max exec time */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_fms_max_exec, 0);
	/* clear feT max delay time */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_feT_max_delay, 0);
	/* clear feT max exec time */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_feT_max_exec, 0);
	/* clear 1/4 feT max exec time */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_feT_1_4_max_exec, 0);
	/* clear 1/16 feT max exec time */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_feT_1_16_max_exec, 0);
	/* clear 1/16 sc max delay time */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_sc_1_16_max_delay, 0);
	/* clear 1/16 sc max exec time */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_sc_1_16_max_exec, 0);
	/* clear 1/16 fmt max delay time */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_fmt_1_16_max_delay, 0);
	/* clear 1/16 fmt max exec time */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_fmt_1_16_max_exec, 0);
	/* clear 1/4 sc max delay time */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_sc_1_4_max_delay, 0);
	/* clear 1/4 sc max exec time */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_sc_1_4_max_exec, 0);
	/* clear 1/4 fmt max delay time */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_fmt_1_4_max_delay, 0);
	/* clear 1/4 fmt max exec time */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_fmt_1_4_max_exec, 0);
	/* clear 1/1 sc max delay time */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_sc_max_delay, 0);
	/* clear 1/1 sc max exec time */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_sc_max_exec, 0);
	/* clear 1/1 fmt max delay time */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_fmt_max_delay, 0);
	/* clear 1/1 fmt max exec time */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_fmt_max_exec, 0);
	/* clear headpose max delay time */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_headpose_max_delay, 0);
	/* clear headpose max exec time */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_headpose_max_exec, 0);
	/* clear fm vpu done time */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_fm_vpu_done_ts, 0);
	/* clear fm vpu max exec time */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_fm_vpu_max_exec, 0);
	/* clear fms sof ts */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_fms_sof_ts, 0);
	/* clear fms done ts */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_fms_done_ts, 0);
	/* clear feT sof ts */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_feT_sof_ts, 0);
	/* clear feT done ts */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_feT_done_ts, 0);
	/* clear 1/4 feT done ts */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_feT_1_4_done_ts, 0);
	/* clear 1/16 feT done ts */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_feT_1_16_done_ts, 0);
	/* clear 1/16 sc sof ts */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_sc_1_16_sof_ts, 0);
	/* clear 1/16 sc done ts */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_sc_1_16_done_ts, 0);
	/* clear 1/16 fmt sof ts */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_fmt_1_16_sof_ts, 0);
	/* clear 1/16 fmt done ts */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_fmt_1_16_done_ts, 0);
	/* clear 1/4 sc sof ts */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_sc_1_4_sof_ts, 0);
	/* clear 1/4 sc done ts */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_sc_1_4_done_ts, 0);
	/* clear 1/4 fmt sof ts */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_fmt_1_4_sof_ts, 0);
	/* clear 1/4 fmt done ts */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_fmt_1_4_done_ts, 0);
	/* clear 1/1 sc sof ts */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_sc_sof_ts, 0);
	/* clear 1/1 sc done ts */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_sc_done_ts, 0);
	/* clear 1/1 fmt sof ts */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_fmt_sof_ts, 0);
	/* clear 1/1 fmt done ts */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_fmt_done_ts, 0);
	/* clear headpose sof ts */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_headpose_sof_ts, 0);
	/* clear fm vpu frame cnt */
	cmdq_pkt_assign_command(fm_dev->cmdq_pkt, GCE_fm_vpu_done_cnt, 0);
	ret = flush_check(fm_dev, fm_dev->cmdq_client,
				fm_dev->cmdq_pkt, FM_WORK_NO_INT);
	if (ret)
		fm_dev->polling_wait = 0;

	while (fm_dev->polling_wait) {
		LOG(CV_LOG_DBG,
			"fm_dev_trigger_work: start wait:%d, frame:%d\n",
			GCE_SW_TOKEN_KICK_FM_VPU, frame_idx);
		/* wait for fe & wdma0 done */
		/* clear all wdma done event in the v-blanking */
		reinit_completion(&fm_dev->fm_work_comp.cmplt);
		cmdq_pkt_multiple_reset(&fm_dev->cmdq_pkt, 1);
		cmdq_pkt_wfe(fm_dev->cmdq_pkt, GCE_SW_TOKEN_KICK_FM_VPU);
		cmdq_pkt_read(fm_dev->cmdq_pkt, subsys, 0x3020,
				GCE_FM_VPU_KICK_ts);
		ret = flush_check(fm_dev, fm_dev->cmdq_client,
				fm_dev->cmdq_pkt, FM_WORK);
		if (ret)
			break;
		/*mtk_fe_reset(fm_dev->dev_fe);*/
		/* call back for interriptible point */
		reinit_completion(&fm_dev->fm_work_comp.cmplt);
		cmdq_pkt_multiple_reset(&fm_dev->cmdq_pkt, 1);
		/* trigger spatial fm */
		cmdq_pkt_clear_event(fm_dev->cmdq_pkt, fm_done_event);
		append_config_spatial_fm_gce_cmd(fm_dev, fm_dev->cmdq_pkt);
		/* get ts -> CPR */
		cmdq_pkt_read(fm_dev->cmdq_pkt, subsys, 0x3020, GCE_fms_sof_ts);
		/* SPR2 = SPR3(fm start timing) - fe done(CPR59) */
		append_exec_time_gce_cmd(fm_dev->cmdq_pkt, GCE_fe_done_ts,
			GCE_fms_sof_ts, GCE_SPR2, fm_dev->fms_delay_pa);
		/* if SPR2 > max delay time GCE_fe_max_exec, update it */
		append_max_exec_time_gce_cmd(fm_dev->cmdq_pkt, GCE_SPR2,
			GCE_fms_max_delay, fm_dev->fms_delay_max_pa);
		/* trigger fm */
		mtk_fm_start(fm_dev->dev_fm, fm_dev->cmdq_pkt);
		/* check spatial fm done or not */
		cmdq_pkt_wfe(fm_dev->cmdq_pkt, fm_done_event);
		/* get fms done ts -> CPR */
		cmdq_pkt_read(fm_dev->cmdq_pkt, subsys, 0x3020,
			      GCE_fms_done_ts);
		/* SPR2 =  FMS exec time*/
		append_exec_time_gce_cmd(fm_dev->cmdq_pkt, GCE_fms_sof_ts,
			GCE_fms_done_ts, GCE_SPR2, fm_dev->fms_et_pa);
		/* check if FMS exec time > max FMS exec time */
		append_max_exec_time_gce_cmd(fm_dev->cmdq_pkt, GCE_SPR2,
			GCE_fms_max_exec, fm_dev->fms_et_max_pa);

		/* prepare feT, 1/4 feT, 1/16 feT, at the same time */
		/* wait 1/4, 1/16 wdma done for computing 1/4, 1/16 feT */
		/* feT */
		/* get feT start ts -> CPR */
		cmdq_pkt_read(fm_dev->cmdq_pkt, subsys, 0x3020, GCE_feT_sof_ts);
		/* SPR2 = SPR3(feT start timing) - fms done(GCE_SPR1) */
		append_exec_time_gce_cmd(fm_dev->cmdq_pkt, GCE_fms_done_ts,
			GCE_feT_sof_ts, GCE_SPR2, fm_dev->feT_delay_pa);
		/* if SPR2 > max delay time GCE_feT_max_delay, update it */
		append_max_exec_time_gce_cmd(fm_dev->cmdq_pkt, GCE_SPR2,
			GCE_feT_max_delay, fm_dev->feT_delay_max_pa);
		cmdq_pkt_clear_event(fm_dev->cmdq_pkt, vpu_done_event);
		mtk_send_ivp_request(device, fm_dev->process_feT1.handle,
			fm_dev->process_feT1.coreid, fm_dev->cmdq_pkt);
		cmdq_pkt_wfe(fm_dev->cmdq_pkt, vpu_done_event);
		mtk_clr_ivp_request(device, fm_dev->process_feT1.handle,
			fm_dev->process_feT1.coreid, fm_dev->cmdq_pkt);
		cmdq_pkt_clear_event(fm_dev->cmdq_pkt, vpu_done_event);
		mtk_send_ivp_request(device, fm_dev->process_feT2.handle,
			fm_dev->process_feT2.coreid, fm_dev->cmdq_pkt);
		cmdq_pkt_wfe(fm_dev->cmdq_pkt, vpu_done_event);
		mtk_clr_ivp_request(device, fm_dev->process_feT2.handle,
			fm_dev->process_feT2.coreid, fm_dev->cmdq_pkt);
		/* get feT done ts -> CPR */
		cmdq_pkt_read(fm_dev->cmdq_pkt, subsys, 0x3020,
			      GCE_feT_done_ts);
		/* SPR2 = feT exec int */
		append_exec_time_gce_cmd(fm_dev->cmdq_pkt, GCE_feT_sof_ts,
			GCE_feT_done_ts, GCE_SPR2, fm_dev->feT_et_pa);
		/* check feT max exec interval */
		append_max_exec_time_gce_cmd(fm_dev->cmdq_pkt, GCE_SPR2,
			GCE_feT_max_exec, fm_dev->feT_et_max_pa);
		if (fm_dev->fmdevmask & FM_DEV_SCALED_FMT_ENABLE) {
			/* 1/4 feT */
			cmdq_pkt_clear_event(fm_dev->cmdq_pkt, vpu_done_event);
			mtk_send_ivp_request(device,
					     fm_dev->process_1_4_feT1.handle,
					     fm_dev->process_1_4_feT1.coreid,
					     fm_dev->cmdq_pkt);
			cmdq_pkt_wfe(fm_dev->cmdq_pkt, vpu_done_event);
			mtk_clr_ivp_request(device,
					     fm_dev->process_1_4_feT1.handle,
					     fm_dev->process_1_4_feT1.coreid,
					     fm_dev->cmdq_pkt);
			cmdq_pkt_clear_event(fm_dev->cmdq_pkt, vpu_done_event);
			mtk_send_ivp_request(device,
					     fm_dev->process_1_4_feT2.handle,
					     fm_dev->process_1_4_feT2.coreid,
					     fm_dev->cmdq_pkt);
			cmdq_pkt_wfe(fm_dev->cmdq_pkt, vpu_done_event);
			mtk_clr_ivp_request(device,
					     fm_dev->process_1_4_feT2.handle,
					     fm_dev->process_1_4_feT2.coreid,
					     fm_dev->cmdq_pkt);
			/* get 1/4 feT end ts -> CPR */
			cmdq_pkt_read(fm_dev->cmdq_pkt, subsys, 0x3020,
					GCE_feT_1_4_done_ts);
			/* SPR2 = 1/4 feT exec int */
			append_exec_time_gce_cmd(fm_dev->cmdq_pkt,
						 GCE_feT_done_ts,
						 GCE_feT_1_4_done_ts,
						 GCE_SPR2,
						 fm_dev->feT_1_4_et_pa);
			/* check 1/4 feT max exec interval */
			append_max_exec_time_gce_cmd(fm_dev->cmdq_pkt,
						     GCE_SPR2,
						     GCE_feT_1_4_max_exec,
						     fm_dev->feT_1_4_et_max_pa);
			/* 1/16 feT */
			cmdq_pkt_clear_event(fm_dev->cmdq_pkt, vpu_done_event);
			mtk_send_ivp_request(device,
					     fm_dev->process_1_16_feT1.handle,
					     fm_dev->process_1_16_feT1.coreid,
					     fm_dev->cmdq_pkt);
			cmdq_pkt_wfe(fm_dev->cmdq_pkt, vpu_done_event);
			mtk_clr_ivp_request(device,
					     fm_dev->process_1_16_feT1.handle,
					     fm_dev->process_1_16_feT1.coreid,
					     fm_dev->cmdq_pkt);
			cmdq_pkt_clear_event(fm_dev->cmdq_pkt, vpu_done_event);
			mtk_send_ivp_request(device,
					     fm_dev->process_1_16_feT2.handle,
					     fm_dev->process_1_16_feT2.coreid,
					     fm_dev->cmdq_pkt);
			cmdq_pkt_wfe(fm_dev->cmdq_pkt, vpu_done_event);
			mtk_clr_ivp_request(device,
					     fm_dev->process_1_16_feT2.handle,
					     fm_dev->process_1_16_feT2.coreid,
					     fm_dev->cmdq_pkt);
			/* get 1/16 feT end ts -> CPR */
			cmdq_pkt_read(fm_dev->cmdq_pkt, subsys, 0x3020,
					GCE_feT_1_16_done_ts);
			/* SPR2 = 1/16 feT exec int */
			append_exec_time_gce_cmd(fm_dev->cmdq_pkt,
						 GCE_feT_1_4_done_ts,
						 GCE_feT_1_16_done_ts,
						 GCE_SPR2,
						 fm_dev->feT_1_16_et_pa);
			/* check 1/16 feT max exec interval */
			append_max_exec_time_gce_cmd(fm_dev->cmdq_pkt,
						GCE_SPR2,
						GCE_feT_1_16_max_exec,
						fm_dev->feT_1_16_et_max_pa);
		}
		ret = flush_check(fm_dev, fm_dev->cmdq_client,
				fm_dev->cmdq_pkt, FM_WORK_NO_INT);
		if (ret)
			break;

		fm_dev->cv_debug_test.fm[MTK_FM_FLOW_SPATIAL][0] =
			readl(fm->regs + 0x2A4);
		fm_dev->cv_debug_test.fm[MTK_FM_FLOW_SPATIAL][1] =
			readl(fm->regs + 0x2AC);

		fm_dev->cv_debug_test.wdma_dbg[0] = readl(top->regs + 0x430);
		fm_dev->cv_debug_test.wdma_dbg[1] = readl(top->regs + 0x460);
		fm_dev->cv_debug_test.wdma_dbg[2] = readl(top->regs + 0x468);
		if (fm_dev->fmdevmask & FM_DEV_SCALED_FMT_ENABLE) {
			reinit_completion(&fm_dev->fm_work_comp.cmplt);
			cmdq_pkt_multiple_reset(&fm_dev->cmdq_pkt, 1);
			/* get 1/16 sc start ts -> CPR */
			cmdq_pkt_read(fm_dev->cmdq_pkt, subsys, 0x3020,
				      GCE_sc_1_16_sof_ts);
			append_exec_time_gce_cmd(fm_dev->cmdq_pkt,
				GCE_feT_1_16_done_ts, GCE_sc_1_16_sof_ts,
				GCE_SPR2, fm_dev->sc_1_16_delay_pa);
			/* check 1/16 sc max delay interval */
			append_max_exec_time_gce_cmd(fm_dev->cmdq_pkt,
						GCE_SPR2,
						GCE_sc_1_16_max_delay,
						fm_dev->sc_1_16_delay_max_pa);
			ret = flush_check(fm_dev, fm_dev->cmdq_client,
					fm_dev->cmdq_pkt, FM_WORK);
			if (ret)
				break;
			mtk_fm_dev_compute_search_center(
					fm_dev->sc_va_1_16,
					dummy_sc_va,
					fm_dev->buf[
					MTK_FM_DEV_BUF_TYPE_1_16_WDMA_R0]->
					dma_addr,
					imu_data, fm_dev->sc_size);
			reinit_completion(&fm_dev->fm_work_comp.cmplt);
			cmdq_pkt_multiple_reset(&fm_dev->cmdq_pkt, 1);
			/* get 1/16 sc end ts -> CPR */
			cmdq_pkt_read(fm_dev->cmdq_pkt, subsys, 0x3020,
					GCE_sc_1_16_done_ts);
			append_exec_time_gce_cmd(fm_dev->cmdq_pkt,
						 GCE_sc_1_16_sof_ts,
						 GCE_sc_1_16_done_ts,
						 GCE_SPR2,
						 fm_dev->sc_1_16_et_pa);
			/* check 1/16 sc max exec interval */
			append_max_exec_time_gce_cmd(fm_dev->cmdq_pkt,
						     GCE_SPR2,
						     GCE_sc_1_16_max_exec,
						     fm_dev->sc_1_16_et_max_pa);
			ret = flush_check(fm_dev, fm_dev->cmdq_client,
					  fm_dev->cmdq_pkt, FM_WORK);
			if (ret)
				break;
			reinit_completion(&fm_dev->fm_work_comp.cmplt);
			cmdq_pkt_multiple_reset(&fm_dev->cmdq_pkt, 1);
			/* do 1/16 temporal fm */
			cmdq_pkt_clear_event(fm_dev->cmdq_pkt, fm_done_event);
			append_config_temporal_fm_gce_cmd(
				fm_dev, fm_dev->cmdq_pkt,
				MTK_FM_DEV_IMG_TYPE_1_16_WDMA,
				fm_dev->sc_va_1_16);
			/* get 1/16 fmt start ts -> CPR */
			cmdq_pkt_read(fm_dev->cmdq_pkt, subsys, 0x3020,
				      GCE_fmt_1_16_sof_ts);
			/* SPR2 = 1/16 fmt delay time */
			append_exec_time_gce_cmd(fm_dev->cmdq_pkt,
						 GCE_sc_1_16_done_ts,
						 GCE_fmt_1_16_sof_ts,
						 GCE_SPR2,
						 fm_dev->fmt_1_16_delay_pa);
			/* check max 1/16 fmt delay time */
			append_max_exec_time_gce_cmd(fm_dev->cmdq_pkt,
						     GCE_SPR2,
						     GCE_fmt_1_16_max_delay,
						     fm_dev->
						     fmt_1_16_delay_max_pa);
			mtk_fm_start(fm_dev->dev_fm, fm_dev->cmdq_pkt);
			cmdq_pkt_wfe(fm_dev->cmdq_pkt, fm_done_event);
			/* get 1/16 fmt done ts -> CPR */
			cmdq_pkt_read(fm_dev->cmdq_pkt, subsys, 0x3020,
				GCE_fmt_1_16_done_ts);
			/* SPR2 = 1/16 fmt exec time */
			append_exec_time_gce_cmd(fm_dev->cmdq_pkt,
						 GCE_fmt_1_16_sof_ts,
						 GCE_fmt_1_16_done_ts,
						 GCE_SPR2,
						 fm_dev->fmt_1_16_et_pa);
			/* check max 1/16 fmt exec time */
			append_max_exec_time_gce_cmd(fm_dev->cmdq_pkt,
						     GCE_SPR2,
						     GCE_fmt_1_16_max_exec,
						     fm_dev->
						     fmt_1_16_et_max_pa);
			ret = flush_check(fm_dev, fm_dev->cmdq_client,
					  fm_dev->cmdq_pkt, FM_WORK);
			if (ret)
				break;
			reinit_completion(&fm_dev->fm_work_comp.cmplt);
			cmdq_pkt_multiple_reset(&fm_dev->cmdq_pkt, 1);
			/* get 1/4 sc start ts -> CPR */
			cmdq_pkt_read(fm_dev->cmdq_pkt, subsys, 0x3020,
					GCE_sc_1_4_sof_ts);
			/* SPR2 = 1/4 sc delay time */
			append_exec_time_gce_cmd(fm_dev->cmdq_pkt,
						 GCE_fmt_1_16_done_ts,
						 GCE_sc_1_4_sof_ts,
						 GCE_SPR2,
						 fm_dev->sc_1_4_delay_pa);
			/* check 1/16 sc max exec interval */
			append_max_exec_time_gce_cmd(fm_dev->cmdq_pkt,
						     GCE_SPR2,
						     GCE_sc_1_4_max_delay,
						     fm_dev->
						     sc_1_4_delay_max_pa);
			ret = flush_check(fm_dev, fm_dev->cmdq_client,
					  fm_dev->cmdq_pkt, FM_WORK);
			if (ret)
				break;
			mtk_fm_dev_compute_search_center_t(
				fm_dev->sc_va_1_4, dummy_sc_va, fm_dev->buf[
				MTK_FM_DEV_BUF_TYPE_1_16_FM_OUT_T_R0]->dma_addr,
				fm_dev->buf[
				MTK_FM_DEV_BUF_TYPE_1_16_FM_ZNCC_T_R0]->
				dma_addr, fm_dev->sc_size);
			reinit_completion(&fm_dev->fm_work_comp.cmplt);
			cmdq_pkt_multiple_reset(&fm_dev->cmdq_pkt, 1);
			/* get 1/4 sc end ts -> CPR */
			cmdq_pkt_read(fm_dev->cmdq_pkt, subsys, 0x3020,
					GCE_sc_1_4_done_ts);
			/* SPR2 = 1/4 sc exec time */
			append_exec_time_gce_cmd(fm_dev->cmdq_pkt,
						 GCE_sc_1_4_sof_ts,
						 GCE_sc_1_4_done_ts,
						 GCE_SPR2,
						 fm_dev->sc_1_4_et_pa);
			/* check 1/16 sc max exec interval */
			append_max_exec_time_gce_cmd(fm_dev->cmdq_pkt, GCE_SPR2,
				GCE_sc_1_4_max_exec, fm_dev->sc_1_4_et_max_pa);
			ret = flush_check(fm_dev, fm_dev->cmdq_client,
					fm_dev->cmdq_pkt, FM_WORK);
			if (ret)
				break;
			fm_dev->cv_debug_test.fm[MTK_FM_FLOW_1_16_TEMPORAL][0] =
				readl(fm->regs + 0x2A4);
			fm_dev->cv_debug_test.fm[MTK_FM_FLOW_1_16_TEMPORAL][1] =
				readl(fm->regs + 0x2AC);
			reinit_completion(&fm_dev->fm_work_comp.cmplt);
			cmdq_pkt_multiple_reset(&fm_dev->cmdq_pkt, 1);

			cmdq_pkt_clear_event(fm_dev->cmdq_pkt, fm_done_event);
			append_config_temporal_fm_gce_cmd(
				fm_dev, fm_dev->cmdq_pkt,
				MTK_FM_DEV_IMG_TYPE_1_4_WDMA,
				fm_dev->sc_va_1_4);
			/* get 1/4 fmt start ts -> CPR */
			cmdq_pkt_read(fm_dev->cmdq_pkt, subsys, 0x3020,
				GCE_fmt_1_4_sof_ts);
			/* SPR2 = 1/4 fmt delay time */
			append_exec_time_gce_cmd(fm_dev->cmdq_pkt,
						 GCE_sc_1_4_done_ts,
						 GCE_fmt_1_4_sof_ts,
						 GCE_SPR2,
						 fm_dev->fmt_1_4_delay_pa);
			/* check max 1/4 fmt delay time */
			append_max_exec_time_gce_cmd(fm_dev->cmdq_pkt,
						     GCE_SPR2,
						     GCE_fmt_1_4_max_delay,
						     fm_dev->
						     fmt_1_4_delay_max_pa);
			mtk_fm_start(fm_dev->dev_fm, fm_dev->cmdq_pkt);
			cmdq_pkt_wfe(fm_dev->cmdq_pkt, fm_done_event);
			/* get 1/4 fmt done ts -> CPR */
			cmdq_pkt_read(fm_dev->cmdq_pkt, subsys, 0x3020,
				      GCE_fmt_1_4_done_ts);
			/* SPR2 = 1/4 fmt exec time */
			append_exec_time_gce_cmd(fm_dev->cmdq_pkt,
						 GCE_fmt_1_4_sof_ts,
						 GCE_fmt_1_4_done_ts,
						 GCE_SPR2,
						 fm_dev->fmt_1_4_et_pa);
			/* check max 1/4 fmt exec time */
			append_max_exec_time_gce_cmd(fm_dev->cmdq_pkt,
						     GCE_SPR2,
						     GCE_fmt_1_4_max_exec,
						     fm_dev->fmt_1_4_et_max_pa);
			ret = flush_check(fm_dev, fm_dev->cmdq_client,
				fm_dev->cmdq_pkt, FM_WORK);
			if (ret)
				break;
		}
		reinit_completion(&fm_dev->fm_work_comp.cmplt);
		cmdq_pkt_multiple_reset(&fm_dev->cmdq_pkt, 1);
		/* get sc start ts -> CPR */
		cmdq_pkt_read(fm_dev->cmdq_pkt, subsys, 0x3020, GCE_sc_sof_ts);
		/* SPR2 = sc delay time */
		if (fm_dev->fmdevmask & FM_DEV_SCALED_FMT_ENABLE)
			append_exec_time_gce_cmd(fm_dev->cmdq_pkt,
						 GCE_fmt_1_4_done_ts,
						 GCE_sc_sof_ts,
						 GCE_SPR2,
						 fm_dev->sc_delay_pa);
		else
			append_exec_time_gce_cmd(fm_dev->cmdq_pkt,
						 GCE_feT_done_ts,
						 GCE_sc_sof_ts,
						 GCE_SPR2,
						 fm_dev->sc_delay_pa);
		/* check sc max exec interval */
		append_max_exec_time_gce_cmd(fm_dev->cmdq_pkt,
					     GCE_SPR2,
					     GCE_sc_max_delay,
					     fm_dev->sc_delay_max_pa);
		ret = flush_check(fm_dev, fm_dev->cmdq_client,
				fm_dev->cmdq_pkt, FM_WORK);
		if (ret)
			break;
		mtk_fm_dev_compute_search_center_t(fm_dev->sc_va,
			dummy_sc_va, fm_dev->buf[
			MTK_FM_DEV_BUF_TYPE_1_4_FM_OUT_T_R0]->dma_addr,
			fm_dev->buf[
			MTK_FM_DEV_BUF_TYPE_1_4_FM_ZNCC_T_R0]->dma_addr,
			fm_dev->sc_size);
		reinit_completion(&fm_dev->fm_work_comp.cmplt);
		cmdq_pkt_multiple_reset(&fm_dev->cmdq_pkt, 1);
		/* get sc end ts -> CPR */
		cmdq_pkt_read(fm_dev->cmdq_pkt, subsys, 0x3020, GCE_sc_done_ts);
		/* SPR2 = sc exec time */
		append_exec_time_gce_cmd(fm_dev->cmdq_pkt, GCE_sc_sof_ts,
				GCE_sc_done_ts, GCE_SPR2, fm_dev->sc_et_pa);
		/* check sc max exec interval */
		append_max_exec_time_gce_cmd(fm_dev->cmdq_pkt, GCE_SPR2,
				GCE_sc_max_exec, fm_dev->sc_et_max_pa);
		ret = flush_check(fm_dev, fm_dev->cmdq_client,
				fm_dev->cmdq_pkt, FM_WORK);
		if (ret)
			break;
		fm_dev->cv_debug_test.fm[MTK_FM_FLOW_1_4_TEMPORAL][0] =
			readl(fm->regs + 0x2A4);
		fm_dev->cv_debug_test.fm[MTK_FM_FLOW_1_4_TEMPORAL][1] =
			readl(fm->regs + 0x2AC);
		reinit_completion(&fm_dev->fm_work_comp.cmplt);
		cmdq_pkt_multiple_reset(&fm_dev->cmdq_pkt, 1);
		cmdq_pkt_clear_event(fm_dev->cmdq_pkt, fm_done_event);
		append_config_temporal_fm_gce_cmd(fm_dev, fm_dev->cmdq_pkt,
			MTK_FM_DEV_IMG_TYPE_NORMAL, fm_dev->sc_va);
		/* get fmt start ts -> CPR */
		cmdq_pkt_read(fm_dev->cmdq_pkt, subsys, 0x3020,
			GCE_fmt_sof_ts);
		/* SPR2 = fmt delay time */
		append_exec_time_gce_cmd(fm_dev->cmdq_pkt, GCE_sc_done_ts,
			GCE_fmt_sof_ts, GCE_SPR2, fm_dev->fmt_delay_pa);
		/* check fmt max delay time */
		append_max_exec_time_gce_cmd(fm_dev->cmdq_pkt, GCE_SPR2,
			GCE_fmt_max_delay, fm_dev->fmt_delay_max_pa);
		mtk_fm_start(fm_dev->dev_fm, fm_dev->cmdq_pkt);
		cmdq_pkt_wfe(fm_dev->cmdq_pkt, fm_done_event);
		/* get fmt done ts -> CPR */
		cmdq_pkt_read(fm_dev->cmdq_pkt, subsys, 0x3020,
			GCE_fmt_done_ts);
		/* SPR2 = fmt exec time */
		append_exec_time_gce_cmd(fm_dev->cmdq_pkt, GCE_fmt_sof_ts,
			GCE_fmt_done_ts, GCE_SPR2, fm_dev->fmt_et_pa);
		/* check fmt max exec time */
		append_max_exec_time_gce_cmd(fm_dev->cmdq_pkt, GCE_SPR2,
			GCE_fmt_max_exec, fm_dev->fmt_et_max_pa);

		/* compute headpose & move wdma 1,2 */
		/* get headpose & move wdma start ts -> CPR */
		cmdq_pkt_read(fm_dev->cmdq_pkt, subsys, 0x3020,
			GCE_headpose_sof_ts);
		/* SPR2 = head_pose & move wdma delay time */
		append_exec_time_gce_cmd(fm_dev->cmdq_pkt, GCE_fmt_done_ts,
					 GCE_headpose_sof_ts, GCE_SPR2,
					 fm_dev->head_pose_delay_pa);
		/* check head_pose & move wdma max delay time */
		append_max_exec_time_gce_cmd(fm_dev->cmdq_pkt, GCE_SPR2,
			GCE_headpose_max_exec, fm_dev->head_pose_delay_max_pa);
		cmdq_pkt_clear_event(fm_dev->cmdq_pkt, vpu_done_event);
		mtk_send_ivp_request(device, fm_dev->process_headpose.handle,
			fm_dev->process_headpose.coreid, fm_dev->cmdq_pkt);
		cmdq_pkt_wfe(fm_dev->cmdq_pkt, vpu_done_event);
		mtk_clr_ivp_request(device, fm_dev->process_headpose.handle,
			fm_dev->process_headpose.coreid, fm_dev->cmdq_pkt);
		if (fm_dev->fmdevmask & FM_DEV_SCALED_FMT_ENABLE) {
			/* move wdma0_1 to wdma1_1 */
			cmdq_pkt_clear_event(fm_dev->cmdq_pkt, vpu_done_event);
			mtk_send_ivp_request(device,
					     fm_dev->process_1_4_WDMA.handle,
					     fm_dev->process_1_4_WDMA.coreid,
					     fm_dev->cmdq_pkt);
			cmdq_pkt_wfe(fm_dev->cmdq_pkt, vpu_done_event);
			mtk_clr_ivp_request(device,
					     fm_dev->process_1_4_WDMA.handle,
					     fm_dev->process_1_4_WDMA.coreid,
					     fm_dev->cmdq_pkt);
			/* move wdma0_2 to wdma1_2 */
			cmdq_pkt_clear_event(fm_dev->cmdq_pkt, vpu_done_event);
			mtk_send_ivp_request(device,
					     fm_dev->process_1_16_WDMA.handle,
					     fm_dev->process_1_16_WDMA.coreid,
					     fm_dev->cmdq_pkt);
			cmdq_pkt_wfe(fm_dev->cmdq_pkt, vpu_done_event);
			mtk_clr_ivp_request(device,
					     fm_dev->process_1_16_WDMA.handle,
					     fm_dev->process_1_16_WDMA.coreid,
					     fm_dev->cmdq_pkt);
		}
		/* get headpose & move wdma done ts -> CPR */
		cmdq_pkt_read(fm_dev->cmdq_pkt, subsys, 0x3020,
				GCE_fm_vpu_done_ts);
		/* SPR2 = head_pose & move wdma exec time */
		append_exec_time_gce_cmd(fm_dev->cmdq_pkt, GCE_headpose_sof_ts,
			GCE_fm_vpu_done_ts, GCE_SPR2, fm_dev->head_pose_et_pa);
		/* check head_pose & move wdma max exec time */
		append_max_exec_time_gce_cmd(fm_dev->cmdq_pkt, GCE_SPR2,
			GCE_headpose_max_exec, fm_dev->head_pose_et_max_pa);
		/* SPR2 = fm vpu overall flow exec time*/
		append_exec_time_gce_cmd(fm_dev->cmdq_pkt, GCE_FM_VPU_KICK_ts,
			GCE_fm_vpu_done_ts, GCE_SPR2, fm_dev->fm_vpu_et_pa);
		/* check fm vpu overall flow max exec time*/
		append_max_exec_time_gce_cmd(fm_dev->cmdq_pkt, GCE_SPR2,
			GCE_fm_vpu_max_exec, fm_dev->fm_vpu_et_max_pa);
		/* fm_vpu frame cnt++ */
		append_inc_frame_cnt_cmd(fm_dev->cmdq_pkt, GCE_fm_vpu_done_cnt,
			fm_dev->fm_vpu_cnt_pa);
#if 0
		/* move wdma0_0 to sram */
		cmdq_pkt_clear_event(fm_dev->cmdq_pkt, vpu_done_event);
		mtk_send_ivp_request(device, fm_dev->process_DRAM_TO_SRAM.
			handle, fm_dev->process_DRAM_TO_SRAM.coreid,
			fm_dev->cmdq_pkt);
		cmdq_pkt_wfe(fm_dev->cmdq_pkt, vpu_done_event);
		/* move sram to wdma1_0 */
		cmdq_pkt_clear_event(fm_dev->cmdq_pkt, vpu_done_event);
		mtk_send_ivp_request(device, fm_dev->process_SRAM_TO_DRAM.
			handle, fm_dev->process_SRAM_TO_DRAM.coreid,
			fm_dev->cmdq_pkt);
		cmdq_pkt_wfe(fm_dev->cmdq_pkt, vpu_done_event);
#endif
		do_gettimeofday(&fm_dev->vpu_start);
		ret = flush_check(fm_dev, fm_dev->cmdq_client,
				fm_dev->cmdq_pkt, FM_WORK_NO_INT);
		if (ret)
			break;
		do_gettimeofday(&fm_dev->vpu_done);
		fm_dev->cv_debug_test.fm[MTK_FM_FLOW_TEMPORAL][0] =
			readl(fm->regs + 0x2A4);
		fm_dev->cv_debug_test.fm[MTK_FM_FLOW_TEMPORAL][1] =
			readl(fm->regs + 0x2AC);
		if (((fm_dev->fmdevmask & FM_DEV_DUMP_FILE_ENABLE) ||
			fm_dev->one_shot_dump) &&
			fm_dev->polling_wait)
			dump_cv_buffers(fm_dev, fm_dev->dump_path, frame_idx);
		frame_idx++;
		fm_dev->flow_done = 1;
	}
	ret = cmdq_pkt_destroy(fm_dev->cmdq_pkt);
	if (ret)
		dev_err(fm_dev->dev, "cmdq_pkt_fm_destroy fail!\n");
	fm_dev->cmdq_pkt = NULL;

	kfree(dummy_sc_va);
	LOG(CV_LOG_DBG, "fm_dev_trigger_work: exit\n");
}


int mtk_fm_dev_path_wait_done(struct mtk_fm_dev *fm_dev)
{
	u32 fm_done;
	int i;

	LOG(CV_LOG_DBG, "start wait fm done!\n");

	for (i = 0; i < 300; i++) {
		mtk_fm_get_fm_done(fm_dev->dev_fm, &fm_done);
		if (fm_done)
			break;

		/*usleep_range(500, 1000);*/
		msleep(100);
	}

	if (i == 300)
		dev_err(fm_dev->dev,
			"fm_trigger: MTK_FM_PROP_FM_DONE timeout\n");
	else
		LOG(CV_LOG_DBG, "fm done %d!\n", i);
	return 0;
}
EXPORT_SYMBOL(mtk_fm_dev_path_wait_done);

int mtk_fm_dev_path_trigger_no_wait(struct mtk_fm_dev *fm_dev)
{
	/*schedule_work(&fm_dev->trigger_work);*/

	return 0;
}
EXPORT_SYMBOL(mtk_fm_dev_path_trigger_no_wait);

int mtk_fm_dev_path_trigger_start_no_wait(struct mtk_fm_dev *fm_dev,
	bool onoff)
{
	if (onoff)
		mtk_fm_dev_path_trigger_no_wait(fm_dev);

	return 0;
}
EXPORT_SYMBOL(mtk_fm_dev_path_trigger_start_no_wait);

int mtk_fm_dev_path_trigger_start(struct mtk_fm_dev *fm_dev, bool onoff)
{
	int ret = 0;

	LOG(CV_LOG_DBG, "fm_dev_trigger_start: start %d\n", onoff);
	if (onoff) {
		ret = mtk_fm_set_zncc_threshold(fm_dev->dev_fm, NULL,
			fm_dev->zncc_th);
		if (ret) {
			dev_err(fm_dev->dev_fm,
				"mtk_fm_set_zncc_threshold fail! %d\n", ret);
			return ret;
		}
		ret = mtk_fm_dev_module_start(fm_dev);
		if (ret) {
			dev_err(fm_dev->dev_fm,
				"mtk_fm_dev_module_start fail! %d\n", ret);
			return ret;
		}
		ret = mtk_fm_dev_path_wait_done(fm_dev);
		if (ret) {
			dev_err(fm_dev->dev_fm,
				"mtk_fm_dev_path_wait_done fail! %d\n", ret);
			return ret;
		}
		ret = mtk_fm_dev_module_stop(fm_dev);
		if (ret) {
			dev_err(fm_dev->dev_fm,
				"mtk_fm_dev_module_stop fail! %d\n", ret);
			return ret;
		}
	}
	LOG(CV_LOG_DBG, "fm_dev_trigger_start: end %d\n", onoff);
	return ret;
}
EXPORT_SYMBOL(mtk_fm_dev_path_trigger_start);

static int mtk_fm_dev_get_device(
	struct device *dev, char *compatible,
	int idx, struct device **child_dev)
{
	struct platform_device *pdev;
	struct device_node *node;

	node = of_parse_phandle(dev->of_node, compatible, idx);
	if (!node) {
		dev_err(dev,
		"fm_dev_get_device: could not find %s %d\n",
				compatible, idx);
		return -ENODEV;
	}

	pdev = of_find_device_by_node(node);
	if (!pdev || !pdev->dev.driver) {
		dev_warn(dev,
		"fm_dev_get_device: waiting for device %s\n",
			 node->full_name);
		return -EPROBE_DEFER;
	}

	*child_dev = &pdev->dev;

	return 0;
}

static int mtk_fm_dev_ioctl_import_fd_to_handle(struct device *dev,
	unsigned long arg)
{
	struct mtk_common_buf_handle handle;
	int ret = 0;

	ret = copy_from_user((void *)&handle, (void *)arg, sizeof(handle));
	if (ret) {
		dev_err(dev,
		"MTK_FM_DEV_IOCTL_IMPORT_FD_TO_HANDLE: get params failed, ret=%d\n",
			ret);
		return ret;
	}

	ret = mtk_common_fd_to_handle(dev, handle.fd,
			&(handle.handle));
	if (ret) {
		dev_err(dev,
		"MTK_FM_DEV_IOCTL_IMPORT_FD_TO_HANDLE: import buf failed, ret=%d\n",
			ret);
		return ret;
	}
	ret = copy_to_user((void *)arg, &handle, sizeof(handle));
	if (ret) {
		dev_err(dev,
		"MTK_FM_DEV_IOCTL_IMPORT_FD_TO_HANDLE: update params failed, ret=%d\n",
				ret);
		return ret;
	}

	return ret;
}

static int mtk_fm_dev_ioctl_set_buf(struct mtk_fm_dev *fm_dev,
	unsigned long arg)
{
	struct mtk_common_set_buf buf;
	struct mtk_common_buf *buf_handle;
	struct device *dev = fm_dev->dev_fm;
	int ret = 0;

	ret = copy_from_user((void *)&buf, (void *)arg, sizeof(buf));
	if (ret) {
		dev_err(dev, "get params failed, ret=%d\n", ret);
		return ret;
	}
	if (buf.buf_type >= MTK_FM_DEV_BUF_TYPE_MAX) {
		dev_err(dev,
		"MTK_FM_DEV_IOCTL_SET_BUF: buf type %d error\n",
			buf.buf_type);
		return -EINVAL;
	}
	buf_handle = (struct mtk_common_buf *)buf.handle;
	buf_handle->pitch = buf.pitch;
	buf_handle->format = buf.format;
	LOG(CV_LOG_DBG, "set FM_DEV buf %d\n", buf.buf_type);
	LOG(CV_LOG_DBG, "buf_handle->kvaddr = %p\n",
		(void *)buf_handle->kvaddr);
	LOG(CV_LOG_DBG, "buf_handle->dma_addr = %pad\n",
		&buf_handle->dma_addr);
	LOG(CV_LOG_DBG, "buf_handle->pitch:%d\n", buf_handle->pitch);
	LOG(CV_LOG_DBG, "buf_handle->format:%d\n", buf_handle->format);
	fm_dev->buf[buf.buf_type] = buf_handle;

	return ret;
}

static int mtk_fm_dev_ioctl_set_fm_prop(struct mtk_fm_dev *fm_dev,
	unsigned long arg)
{
	struct mtk_fm_dev_fm_parm parm;
	int ret = 0;

	ret = copy_from_user((void *)&parm, (void *)arg, sizeof(parm));
	if (ret) {
		dev_err(fm_dev->dev,
			"FM_SET_FM_PROP: get params failed, ret= %d\n", ret);
		return ret;
	}
	fm_dev->fmdevmask = parm.fmdevmask;
	fm_dev->sysram_layout = parm.sysram_layout;
	fm_dev->img_num = parm.img_num;
	fm_dev->sr_type = parm.sr_type;
	fm_dev->blk_type = parm.blk_type;
	fm_dev->fe_w = parm.fe_w;
	fm_dev->fe_h = parm.fe_h;
	fm_dev->fm_msk_w_s = parm.fm_msk_w_s;
	fm_dev->fm_msk_h_s = parm.fm_msk_h_s;
	fm_dev->fm_msk_w_tp = parm.fm_msk_w_tp;
	fm_dev->fm_msk_h_tp = parm.fm_msk_h_tp;
	fm_dev->hdr_idx_cur = parm.start_hdr_idx_cur;
	fm_dev->hdr_idx_pre = parm.start_hdr_idx_pre;
	fm_dev->zncc_th = parm.zncc_th;

	/** Setting req interval */
	fm_dev->req_interval.img_req_interval =
	parm.req_interval.img_req_interval;
	fm_dev->req_interval.desc_req_interval =
	parm.req_interval.desc_req_interval;
	fm_dev->req_interval.point_req_interval =
	parm.req_interval.point_req_interval;
	fm_dev->req_interval.fmo_req_interval =
	parm.req_interval.fmo_req_interval;
	fm_dev->req_interval.zncc_req_interval =
	parm.req_interval.zncc_req_interval;

	parm.mask_path_s[sizeof(parm.mask_path_s) - 1] = '\0';
	parm.mask_path_tp[sizeof(parm.mask_path_tp) - 1] = '\0';
	parm.sc_path[sizeof(parm.sc_path) - 1] = '\0';
	parm.sc_path_1_4[sizeof(parm.sc_path_1_4) - 1] = '\0';
	parm.sc_path_1_16[sizeof(parm.sc_path_1_16) - 1] = '\0';
	parm.dump_path[sizeof(parm.dump_path) - 1] = '\0';

	strncpy(fm_dev->mask_path_s, parm.mask_path_s,
		sizeof(fm_dev->mask_path_s));
	strncpy(fm_dev->mask_path_tp, parm.mask_path_tp,
		sizeof(fm_dev->mask_path_tp));
	strncpy(fm_dev->sc_path, parm.sc_path,
		sizeof(fm_dev->sc_path));
	strncpy(fm_dev->sc_path_1_4, parm.sc_path_1_4,
		sizeof(fm_dev->sc_path_1_4));
	strncpy(fm_dev->sc_path_1_16, parm.sc_path_1_16,
		sizeof(fm_dev->sc_path_1_16));
	strncpy(fm_dev->dump_path, parm.dump_path,
		sizeof(fm_dev->dump_path));

	switch (fm_dev->blk_type) {
	case MTK_FM_BLK_SIZE_TYPE_32x32:
		fm_dev->blk_sz = 32;
		break;
	case MTK_FM_BLK_SIZE_TYPE_16x16:
		fm_dev->blk_sz = 16;
		break;
	case MTK_FM_BLK_SIZE_TYPE_8x8:
		fm_dev->blk_sz = 8;
		break;
	default:
		dev_err(fm_dev->dev,
			"fm_dev_set_prop: error invalid blk_type %d\n",
			fm_dev->blk_type);
		return -EINVAL;
	}
	fm_dev->blk_nr = ((fm_dev->fe_w + fm_dev->blk_sz - 1) / fm_dev->blk_sz)
			 *
			 ((fm_dev->fe_h + fm_dev->blk_sz - 1) / fm_dev->blk_sz);
	fm_dev->sc_size = (fm_dev->fe_w / fm_dev->blk_sz) *
			  (fm_dev->fe_h / fm_dev->blk_sz);
	return ret;
}
static int mtk_fm_dev_ioctl_streamon(struct mtk_fm_dev *fm_dev)
{
	struct device *dev = fm_dev->dev_fm;
	int ret = 0;

#if POWER_READY
	mtk_fm_dev_module_power_on(fm_dev);
#endif
	LOG(CV_LOG_DBG, "stream on start!\n");
	/* pa to vaddr of all fmo buffer for header use */
	if (fm_dev->fmdevmask & FM_DEV_DUMP_FILE_ENABLE)
		mtk_fm_dev_buf_va(fm_dev, true);

	pa_to_vaddr_dev(fm_dev->dev_fm,
		fm_dev->buf[MTK_FM_DEV_BUF_TYPE_FM_OUT_S_R0],
		roundup(fm_dev->blk_nr * 10 + 128, 128), true);
	pa_to_vaddr_dev(fm_dev->dev_fm,
		fm_dev->buf[MTK_FM_DEV_BUF_TYPE_FM_OUT_S_R1],
		roundup(fm_dev->blk_nr * 10 + 128, 128), true);
	pa_to_vaddr_dev(fm_dev->dev_fm,
		fm_dev->buf[MTK_FM_DEV_BUF_TYPE_FM_OUT_T_R0],
		roundup(fm_dev->blk_nr * 10 + 128, 128), true);
	pa_to_vaddr_dev(fm_dev->dev_fm,
		fm_dev->buf[MTK_FM_DEV_BUF_TYPE_FM_OUT_T_R1],
		roundup(fm_dev->blk_nr * 10 + 128, 128), true);
	pa_to_vaddr_dev(fm_dev->dev_fm,
		fm_dev->buf[MTK_FM_DEV_BUF_TYPE_1_4_FM_OUT_T_R0],
		roundup(fm_dev->blk_nr * 10 + 128, 128), true);
	pa_to_vaddr_dev(fm_dev->dev_fm,
		fm_dev->buf[MTK_FM_DEV_BUF_TYPE_1_4_FM_OUT_T_R1],
		roundup(fm_dev->blk_nr * 10 + 128, 128), true);
	pa_to_vaddr_dev(fm_dev->dev_fm,
		fm_dev->buf[MTK_FM_DEV_BUF_TYPE_1_16_FM_OUT_T_R0],
		roundup(fm_dev->blk_nr * 10 + 128, 128), true);
	pa_to_vaddr_dev(fm_dev->dev_fm,
		fm_dev->buf[MTK_FM_DEV_BUF_TYPE_1_16_FM_OUT_T_R1],
		roundup(fm_dev->blk_nr * 10 + 128, 128), true);

	ret = mtk_fm_dev_module_config(fm_dev);
	if (ret) {
		dev_err(dev,
			"mtk_fm_dev_module_config fail!, ret=%d\n", ret);
		return ret;
	}
	ret = mtk_fm_dev_module_set_buf(fm_dev, NULL,
		MTK_FM_DEV_IMG_TYPE_NORMAL, fm_dev->sr_type);
	if (ret) {
		dev_err(dev, "mtk_fm_dev_module_set_buf fail!, ret=%d\n",
			ret);
		return ret;
	}
	ret = mtk_fm_dev_path_connect(fm_dev);
	if (ret) {
		dev_err(dev, "mtk_fm_dev_path_connect fail!, ret=%d\n",
			ret);
		return ret;
	}
	LOG(CV_LOG_DBG, "stream on done!\n");

	return ret;
}

static int mtk_fm_dev_ioctl_streamoff(struct mtk_fm_dev *fm_dev)
{
	int ret = 0;
	char exit_dump_path[256];

	LOG(CV_LOG_DBG, "stream off start!\n");

	ret = mtk_fm_dev_path_disconnect(fm_dev);
	if (ret) {
		dev_err(fm_dev->dev,
			"mtk_fm_dev_path_disconnect fail!, ret=%d\n",
			ret);
		return ret;
	}

	ret = mtk_fm_dev_module_stop(fm_dev);
	if (ret) {
		dev_err(fm_dev->dev,
			"mtk_fm_dev_module_stop fail!, ret=%d\n",
			ret);
		return ret;
	}

	snprintf(exit_dump_path, sizeof(exit_dump_path), "/tmp");

	if (fm_dev->fmdevmask & FM_DEV_DUMP_FILE_ENABLE)
		mtk_fm_dev_buf_va(fm_dev, false);

	/* invalid all fmo buffer va */
	pa_to_vaddr_dev(fm_dev->dev_fm,
		fm_dev->buf[MTK_FM_DEV_BUF_TYPE_FM_OUT_S_R0],
		roundup(fm_dev->blk_nr * 10 + 128, 128), false);
	pa_to_vaddr_dev(fm_dev->dev_fm,
		fm_dev->buf[MTK_FM_DEV_BUF_TYPE_FM_OUT_S_R1],
		roundup(fm_dev->blk_nr * 10 + 128, 128), false);
	pa_to_vaddr_dev(fm_dev->dev_fm,
		fm_dev->buf[MTK_FM_DEV_BUF_TYPE_FM_OUT_T_R0],
		roundup(fm_dev->blk_nr * 10 + 128, 128), false);
	pa_to_vaddr_dev(fm_dev->dev_fm,
		fm_dev->buf[MTK_FM_DEV_BUF_TYPE_FM_OUT_T_R1],
		roundup(fm_dev->blk_nr * 10 + 128, 128), false);
	pa_to_vaddr_dev(fm_dev->dev_fm,
		fm_dev->buf[MTK_FM_DEV_BUF_TYPE_1_4_FM_OUT_T_R0],
		roundup(fm_dev->blk_nr * 10 + 128, 128), false);
	pa_to_vaddr_dev(fm_dev->dev_fm,
		fm_dev->buf[MTK_FM_DEV_BUF_TYPE_1_4_FM_OUT_T_R1],
		roundup(fm_dev->blk_nr * 10 + 128, 128), false);
	pa_to_vaddr_dev(fm_dev->dev_fm,
		fm_dev->buf[MTK_FM_DEV_BUF_TYPE_1_16_FM_OUT_T_R0],
		roundup(fm_dev->blk_nr * 10 + 128, 128), false);
	pa_to_vaddr_dev(fm_dev->dev_fm,
		fm_dev->buf[MTK_FM_DEV_BUF_TYPE_1_16_FM_OUT_T_R1],
		roundup(fm_dev->blk_nr * 10 + 128, 128), false);
	LOG(CV_LOG_DBG, "stream off done!\n");
#if 0
	if (fm_dev->fm_vpu_frame_cnt > 0) {
		dump_cv_buffers(fm_dev, exit_dump_path, 0);
		dev_dbg(fm_dev->dev, "exit dump file done!\n");
	}
#endif
#if POWER_READY
	mtk_fm_dev_module_power_off(fm_dev);
#endif
	return ret;
}

static int mtk_fm_dev_ioctl_comp(struct mtk_fm_dev *fm_dev, unsigned long arg)
{
	struct mtk_fm_dev_fm_golden_path path;
	struct device *dev = fm_dev->dev_fm;
	void *fmo1_golden_va, *fmo2_golden_va;
	char fmo1_golden[FMO_GOLEDN_PATH_LEN], fmo2_golden[FMO_GOLEDN_PATH_LEN];
	int err_flag = 0;
	int ret = 0;

	memset(path.fmo1_golden_path, 0, sizeof(path.fmo1_golden_path));
	memset(path.fmo2_golden_path, 0, sizeof(path.fmo2_golden_path));
	memset(fmo1_golden, 0, FMO_GOLEDN_PATH_LEN);
	memset(fmo2_golden, 0, FMO_GOLEDN_PATH_LEN);
	path.fmo1_size = 0;
	path.fmo2_size = 0;
	ret = copy_from_user((void *)&path, (void *)arg, sizeof(path));
	if (ret) {
		dev_err(dev,
			"MTK_FM_DEV_IOCTL_COMP: get params failed, ret=%d\n",
			ret);
		return ret;
	}
	if (path.fmo1_size > 0x1080 || path.fmo2_size > 0x6400 ||
		path.fmo1_size < 0x100 || path.fmo2_size < 0x640)
		return -EINVAL;

	if (strlen(path.fmo1_golden_path) > FMO_GOLEDN_PATH_LEN) {
		pr_err("file path length is not enough!\n");
		return -EINVAL;
	}
	mtk_common_file_path(fmo1_golden, path.fmo1_golden_path);

	if (strlen(path.fmo2_golden_path) > FMO_GOLEDN_PATH_LEN) {
		pr_err("file path length is not enough!\n");
		return -EINVAL;
	}
	mtk_common_file_path(fmo2_golden, path.fmo2_golden_path);

	fmo1_golden_va = kmalloc(path.fmo1_size, GFP_KERNEL);
	if (!fmo1_golden_va)
		return -ENOMEM;
	fmo2_golden_va = kmalloc(path.fmo2_size, GFP_KERNEL);
	if (!fmo2_golden_va) {
		kfree(fmo1_golden_va);
		return -ENOMEM;
	}

	mtk_common_read_file(fmo1_golden_va, fmo1_golden,
		path.fmo1_size);
	mtk_common_read_file(fmo2_golden_va, fmo2_golden,
		path.fmo2_size);

	if (fm_dev->sr_type == MTK_FM_SR_TYPE_SPATIAL) {
		dev_dbg(dev, "compare spatial fm\n");

		pa_to_vaddr_dev(fm_dev->dev_fm,
			fm_dev->buf[MTK_FM_DEV_BUF_TYPE_FM_OUT_S_R0],
			path.fmo1_size, true);
		pa_to_vaddr_dev(fm_dev->dev_fm,
			fm_dev->buf[MTK_FM_DEV_BUF_TYPE_FM_ZNCC_S_R0],
			path.fmo2_size, true);

		dev_dbg(dev, "start to compare fmo1 with %s\n",
			path.fmo1_golden_path);
		ret = mtk_common_compare(fmo1_golden_va + 8,
			fm_dev->buf[
			MTK_FM_DEV_BUF_TYPE_FM_OUT_S_R0]->kvaddr + 128,
			fm_dev->blk_nr * SIZE_PER_BLOCK);
		if (ret)
			err_flag = 1;

		dev_dbg(dev, "start to compare fmo2 with %s\n",
			path.fmo2_golden_path);
		ret = mtk_common_compare(fmo2_golden_va,
			fm_dev->buf[
			MTK_FM_DEV_BUF_TYPE_FM_ZNCC_S_R0]->kvaddr,
			path.fmo2_size);
		if (ret)
			err_flag = 1;
		pa_to_vaddr_dev(fm_dev->dev_fm,
			fm_dev->buf[MTK_FM_DEV_BUF_TYPE_FM_OUT_S_R0],
			path.fmo1_size, false);
		pa_to_vaddr_dev(fm_dev->dev_fm,
			fm_dev->buf[MTK_FM_DEV_BUF_TYPE_FM_ZNCC_S_R0],
			path.fmo2_size, false);
	} else {
		dev_dbg(dev, "compare temporal fm\n");

		pa_to_vaddr_dev(fm_dev->dev_fm,
			fm_dev->buf[MTK_FM_DEV_BUF_TYPE_FM_OUT_T_R0],
			path.fmo1_size, true);
		pa_to_vaddr_dev(fm_dev->dev_fm,
			fm_dev->buf[MTK_FM_DEV_BUF_TYPE_FM_ZNCC_T_R0],
			path.fmo2_size, true);

		dev_dbg(dev, "start to compare fmo1 with %s\n",
			path.fmo1_golden_path);
		ret = mtk_common_compare(fmo1_golden_va + 8,
			fm_dev->buf[
			MTK_FM_DEV_BUF_TYPE_FM_OUT_T_R0]->kvaddr + 128,
			fm_dev->blk_nr * SIZE_PER_BLOCK);
		if (ret)
			err_flag = 1;

		dev_dbg(dev, "start to compare fmo2 with %s\n",
			path.fmo2_golden_path);
		ret = mtk_common_compare(fmo2_golden_va,
			fm_dev->buf[
			MTK_FM_DEV_BUF_TYPE_FM_ZNCC_T_R0]->kvaddr,
			path.fmo2_size);
		if (ret)
			err_flag = 1;
		pa_to_vaddr_dev(fm_dev->dev_fm,
			fm_dev->buf[MTK_FM_DEV_BUF_TYPE_FM_OUT_T_R0],
			path.fmo1_size, false);
		pa_to_vaddr_dev(fm_dev->dev_fm,
			fm_dev->buf[MTK_FM_DEV_BUF_TYPE_FM_ZNCC_T_R0],
			path.fmo2_size, false);
	}
	dev_dbg(dev, "fm golden compare done!\n");

	kfree(fmo1_golden_va);
	kfree(fmo2_golden_va);
	if (err_flag)
		return -EIO;

	return ret;
}

static int mtk_fm_dev_ioctl_put_handle(struct device *dev,
	unsigned long arg)
{
	struct mtk_common_put_handle put_handle;
	int ret = 0;

	ret = copy_from_user(&put_handle, (void *)arg, sizeof(put_handle));
	if (ret) {
		dev_err(dev,
			"PUT_HANDLE: get params failed, ret=%d\n",
			ret);
		return ret;
	}
	ret = mtk_common_put_handle(put_handle.handle);
	if (ret) {
		dev_err(dev,
			"mtk_common_put_handle failed, ret=%d\n",
			ret);
		return ret;
	}

	return ret;
}

static int mtk_fm_dev_ioctl_set_vpu_req(struct mtk_fm_dev *fm_dev,
	unsigned long arg)
{
	struct mtk_fm_dev_vpu_req req;
	int ret = 0;

	ret = copy_from_user(&req, (void *)arg,
				sizeof(req));
	if (ret) {
		dev_err(fm_dev->dev,
			"VPU_REQ: get params failed, ret=%d\n",
			ret);
		return ret;
	}
	if (req.process_feT1 != NULL) {
		ret = copy_from_user(&fm_dev->process_feT1,
				     (void *)req.process_feT1,
				     sizeof(fm_dev->process_feT1));
		if (ret) {
			dev_err(fm_dev->dev,
				"VPU_REQ: get feT1 failed, ret=%d\n",
				ret);
			return ret;
		}
	}

	if (req.process_feT2 != NULL) {
		ret = copy_from_user(&fm_dev->process_feT2,
				     (void *)req.process_feT2,
				     sizeof(fm_dev->process_feT2));
		if (ret) {
			dev_err(fm_dev->dev,
				"VPU_REQ: get feT2 failed, ret=%d\n",
				ret);
			return ret;
		}
	}

	if (req.process_1_4_feT1 != NULL) {
		ret = copy_from_user(&fm_dev->process_1_4_feT1,
				     (void *)req.process_1_4_feT1,
				     sizeof(fm_dev->process_1_4_feT1));
		if (ret) {
			dev_err(fm_dev->dev,
				"VPU_REQ: get 1_4_feT1 failed, ret=%d\n",
				ret);
			return ret;
		}
	}

	if (req.process_1_4_feT2 != NULL) {
		ret = copy_from_user(&fm_dev->process_1_4_feT2,
				     (void *)req.process_1_4_feT2,
				     sizeof(fm_dev->process_1_4_feT2));
		if (ret) {
			dev_err(fm_dev->dev,
				"VPU_REQ: get 1_4_feT2 failed, ret=%d\n",
				ret);
			return ret;
		}
	}

	if (req.process_1_16_feT1 != NULL) {
		ret = copy_from_user(&fm_dev->process_1_16_feT1,
				     (void *)req.process_1_16_feT1,
				     sizeof(fm_dev->process_1_16_feT1));
		if (ret) {
			dev_err(fm_dev->dev,
				"VPU_REQ: get 1_16_feT1 failed, ret=%d\n",
				ret);
			return ret;
		}
	}

	if (req.process_1_16_feT2 != NULL) {
		ret = copy_from_user(&fm_dev->process_1_16_feT2,
				     (void *)req.process_1_16_feT2,
				     sizeof(fm_dev->process_1_16_feT2));
		if (ret) {
			dev_err(fm_dev->dev,
				"VPU_REQ: get 1_16_feT2 failed, ret=%d\n",
				ret);
			return ret;
		}
	}

	if (req.process_headpose != NULL) {
		ret = copy_from_user(&fm_dev->process_headpose,
				     (void *)req.process_headpose,
				     sizeof(fm_dev->process_headpose));
		if (ret) {
			dev_err(fm_dev->dev,
				"VPU_REQ: get headpose failed, ret=%d\n",
				ret);
			return ret;
		}
	}

	if (req.process_1_4_WDMA != NULL) {
		ret = copy_from_user(&fm_dev->process_1_4_WDMA,
				     (void *)req.process_1_4_WDMA,
				     sizeof(fm_dev->process_1_4_WDMA));
		if (ret) {
			dev_err(fm_dev->dev,
				"VPU_REQ: get 1_4_WDMA failed, ret=%d\n",
				ret);
			return ret;
		}
	}

	if (req.process_1_16_WDMA != NULL) {
		ret = copy_from_user(&fm_dev->process_1_16_WDMA,
				     (void *)req.process_1_16_WDMA,
				     sizeof(fm_dev->process_1_16_WDMA));
		if (ret) {
			dev_err(fm_dev->dev,
				"VPU_REQ: get 1_16_WDMA failed, ret=%d\n",
				ret);
			return ret;
		}
	}

	if (req.process_DRAM_TO_SRAM != NULL) {
		ret = copy_from_user(&fm_dev->process_DRAM_TO_SRAM,
				     (void *)req.process_DRAM_TO_SRAM,
				     sizeof(fm_dev->process_DRAM_TO_SRAM));
		if (ret) {
			dev_err(fm_dev->dev,
				"VPU_REQ: get DRAM_TO_SRAM failed, ret=%d\n",
				ret);
			return ret;
		}
	}

	if (req.process_SRAM_TO_DRAM != NULL) {
		ret = copy_from_user(&fm_dev->process_SRAM_TO_DRAM,
				     (void *)req.process_SRAM_TO_DRAM,
				     sizeof(fm_dev->process_SRAM_TO_DRAM));
		if (ret) {
			dev_err(fm_dev->dev,
				"VPU_REQ: get SRAM_TO_DRAM failed, ret=%d\n",
				ret);
			return ret;
		}
	}

	return ret;
}

static int mtk_fm_dev_ioctl_check_flow_done(struct mtk_fm_dev *fm_dev)
{
	int i = 0;

	/* wait up to timeout * img_num * 50ms */
	for (i = 0; i < FLOW_TIMEOUT * fm_dev->img_num; i++) {
		if (fm_dev->flow_done)
			break;
		msleep(50);
	}

	fm_dev->flow_done = 0;
	if (i == FLOW_TIMEOUT * fm_dev->img_num)
		return -ETIME;
	else
		return 0;
}

static int mtk_fm_dev_ioctl_get_statistic(struct mtk_fm_dev *fm_dev,
	unsigned long arg)
{
	struct mtk_fm_dev_statistics stat;
	int ret = 0;

	ret = copy_from_user((void *)&stat, (void *)arg,
				sizeof(stat));
	if (ret) {
		dev_err(fm_dev->dev, "GET_STAT: get params failed, ret=%d\n",
			ret);
		return ret;
	}
	stat.warpa_frame_cnt = *fm_dev->wpe_cnt_va;
	stat.warpa_done_int_cur = *fm_dev->wpe_et_va;
	stat.warpa_done_int_max = *fm_dev->wpe_et_max_va;
	stat.fm_vpu_frame_cnt = *fm_dev->fm_vpu_cnt_va;
	stat.fm_vpu_done_int_cur = *fm_dev->fm_vpu_et_va;
	stat.fm_vpu_done_int_max = *fm_dev->fm_vpu_et_max_va;
	stat.fe_frame_cnt = *fm_dev->fe_cnt_va;
	stat.fe_done_int_cur = *fm_dev->fe_et_va;
	stat.fe_done_int_max = *fm_dev->fe_et_max_va;

	stat.fms_delay_cur = *fm_dev->fms_delay_va;
	stat.fms_delay_max = *fm_dev->fms_delay_max_va;
	stat.fms_done_int_cur = *fm_dev->fms_et_va;
	stat.fms_done_int_max = *fm_dev->fms_et_max_va;

	stat.feT_delay_cur = *fm_dev->feT_delay_va;
	stat.feT_delay_max = *fm_dev->feT_delay_max_va;
	stat.feT_done_int_cur = *fm_dev->feT_et_va;
	stat.feT_done_int_max = *fm_dev->feT_et_max_va;
	stat.feT_1_4_done_int_cur = *fm_dev->feT_1_4_et_va;
	stat.feT_1_4_done_int_max = *fm_dev->feT_1_4_et_max_va;
	stat.feT_1_16_done_int_cur = *fm_dev->feT_1_16_et_va;
	stat.feT_1_16_done_int_max = *fm_dev->feT_1_16_et_max_va;

	stat.sc_1_16_delay_cur = *fm_dev->sc_1_16_delay_va;
	stat.sc_1_16_delay_max = *fm_dev->sc_1_16_delay_max_va;
	stat.sc_1_16_exec_cur = *fm_dev->sc_1_16_et_va;
	stat.sc_1_16_exec_max = *fm_dev->sc_1_16_et_max_va;

	stat.fmt_1_16_delay_cur = *fm_dev->fmt_1_16_delay_va;
	stat.fmt_1_16_delay_max = *fm_dev->fmt_1_16_delay_max_va;
	stat.fmt_1_16_done_int_cur = *fm_dev->fmt_1_16_et_va;
	stat.fmt_1_16_done_int_max = *fm_dev->fmt_1_16_et_max_va;

	stat.sc_1_4_delay_cur = *fm_dev->sc_1_4_delay_va;
	stat.sc_1_4_delay_max = *fm_dev->sc_1_4_delay_max_va;
	stat.sc_1_4_exec_cur = *fm_dev->sc_1_4_et_va;
	stat.sc_1_4_exec_max = *fm_dev->sc_1_4_et_max_va;

	stat.fmt_1_4_delay_cur = *fm_dev->fmt_1_4_delay_va;
	stat.fmt_1_4_delay_max = *fm_dev->fmt_1_4_delay_max_va;
	stat.fmt_1_4_done_int_cur = *fm_dev->fmt_1_4_et_va;
	stat.fmt_1_4_done_int_max = *fm_dev->fmt_1_4_et_max_va;

	stat.sc_delay_cur = *fm_dev->sc_delay_va;
	stat.sc_delay_max = *fm_dev->sc_delay_max_va;
	stat.sc_exec_cur = *fm_dev->sc_et_va;
	stat.sc_exec_max = *fm_dev->sc_et_max_va;

	stat.fmt_delay_cur = *fm_dev->fmt_delay_va;
	stat.fmt_delay_max = *fm_dev->fmt_delay_max_va;
	stat.fmt_done_int_cur = *fm_dev->fmt_et_va;
	stat.fmt_done_int_max = *fm_dev->fmt_et_max_va;

	stat.head_pose_delay_cur = *fm_dev->head_pose_delay_va;
	stat.head_pose_delay_max = *fm_dev->head_pose_delay_max_va;
	stat.head_pose_done_int_cur = *fm_dev->head_pose_et_va;
	stat.head_pose_done_int_max = *fm_dev->head_pose_et_max_va;
	stat.fm_vpu_done_int_cur = *fm_dev->fm_vpu_et_va;
	stat.fm_vpu_done_int_max = *fm_dev->fm_vpu_et_max_va;
	stat.fm_vpu_frame_cnt = *fm_dev->fm_vpu_cnt_va;
	stat.be0_lim_cnt = fm_dev->be0_lim_cnt;
	stat.be1_lim_cnt = fm_dev->be1_lim_cnt;
	stat.be2_lim_cnt = fm_dev->be2_lim_cnt;
	stat.be3_lim_cnt = fm_dev->be3_lim_cnt;
	ret = copy_to_user((void *)arg, &stat, sizeof(stat));
	if (ret) {
		dev_err(fm_dev->dev, "GET_STAT: update params failed, ret=%d\n",
			ret);
		return ret;
	}

	return ret;
}

static int mtk_fm_dev_ioctl_set_log_level(unsigned long arg)
{
	if (arg > CV_LOG_DBG)
		return -1;
	kern_log_level = (int)arg;
	pr_info("set fm_dev kernel log level %d\n", kern_log_level);

	return 0;
}

static int mtk_fm_dev_ioctl_debug_test(struct mtk_fm_dev *fm_dev,
	unsigned long arg)
{
	struct mtk_fm_dev_debug_test debug_test;
	struct device *dev = fm_dev->dev_fm;
	int i, j;
	int ret = 0;

	ret = copy_from_user((void *)&debug_test, (void *)arg,
				sizeof(debug_test));
	if (ret) {
		dev_err(dev, "GET_TEST: get params failed, ret=%d\n",
			ret);
		return ret;
	}

	for (i = 0; i < FLOW_TIMEOUT * fm_dev->img_num; i++) {
		if (*fm_dev->fm_vpu_cnt_va > 0)
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT * fm_dev->img_num) {
		pr_err("wait vr_tracking frame done timeout\n");
		return -ETIME;
	}

	dev_err(dev, "fmso1_dbg = 0x%08x\n",
		fm_dev->cv_debug_test.fm[0][0]);
	dev_err(dev, "fmso2_dbg = 0x%08x\n",
		fm_dev->cv_debug_test.fm[0][1]);
	dev_err(dev, "fmto1_dbg = 0x%08x\n",
		fm_dev->cv_debug_test.fm[1][0]);
	dev_err(dev, "fmto2_dbg = 0x%08x\n",
		fm_dev->cv_debug_test.fm[1][1]);
	dev_err(dev, "fmto1_1_4_dbg = 0x%08x\n",
		fm_dev->cv_debug_test.fm[2][0]);
	dev_err(dev, "fmto2_1_4_dbg = 0x%08x\n",
		fm_dev->cv_debug_test.fm[2][1]);
	dev_err(dev, "fmto1_1_16_dbg = 0x%08x\n",
		fm_dev->cv_debug_test.fm[3][0]);
	dev_err(dev, "fmto2_1_16_dbg = 0x%08x\n",
		fm_dev->cv_debug_test.fm[3][1]);
	for (i = 0; i < MTK_FM_WDMA_IDX_MAX; i++)
		dev_err(fm_dev->dev, "wdma%d_dbg = 0x%08x\n", i,
			fm_dev->cv_debug_test.wdma_dbg[i]);

	/* check fmo */
	dev_dbg(dev, "check fmo...\n");
	for (i = 0; i < MTK_FM_FLOW_MAX; i++) {
		for (j = 0; j < 2; j++) {
			if (debug_test.fm[i][j] !=
				fm_dev->cv_debug_test.fm[i][j]) {
				switch (i) {
				case MTK_FM_FLOW_SPATIAL:
					pr_err("fms%d dbg compare failed! %08x:%08x\n",
						j + 1,
						debug_test.fm[i][j],
						fm_dev->
						cv_debug_test.fm[i][j]);
					return -EINVAL;
				case MTK_FM_FLOW_TEMPORAL:
					pr_err("fmt%d dbg compare failed! %08x:%08x\n",
						j + 1,
						debug_test.fm[i][j],
						fm_dev->
						cv_debug_test.fm[i][j]);
					return -EINVAL;
				case MTK_FM_FLOW_1_4_TEMPORAL:
					pr_err("1_4_fmt%d dbg compare failed! %08x:%08x\n",
						j + 1,
						debug_test.fm[i][j],
						fm_dev->
						cv_debug_test.fm[i][j]);
					return -EINVAL;
				case MTK_FM_FLOW_1_16_TEMPORAL:
					pr_err("1_16_fmt%d dbg compare failed! %08x:%08x\n",
						j + 1,
						debug_test.fm[i][j],
						fm_dev->
						cv_debug_test.fm[i][j]);
					return -EINVAL;
				default:
					break;
				}
			}
		}
	}

	/* check wdma */
	dev_dbg(dev, "check wdma...\n");
	for (i = 0; i < MTK_FM_WDMA_IDX_MAX; i++) {
		if (debug_test.wdma_dbg[i] !=
			fm_dev->cv_debug_test.wdma_dbg[i]) {
			pr_err("wdma%d dbg compare failed! %08x:%08x\n",
				i, debug_test.wdma_dbg[i],
				fm_dev->cv_debug_test.wdma_dbg[i]);
			return -EINVAL;
		}
	}
	dev_dbg(dev, "all pass!\n");

	return ret;
}

static int mtk_fm_dev_ioctl_flush_cache(struct mtk_fm_dev *fm_dev)
{
	int ret = 0, i;
	struct mtk_common_buf *buf;

	for (i = 0; i < MTK_FM_DEV_BUF_TYPE_MAX; i++) {
		buf = fm_dev->buf[i];
		if (buf != NULL)
			dma_sync_sg_for_device(fm_dev->dev_fm, buf->sg->sgl,
				       buf->sg->nents, DMA_TO_DEVICE);
	}

	return ret;
}

static int mtk_fm_dev_ioctl_one_shot_dump(struct mtk_fm_dev *fm_dev)
{
	int i = 0;

	fm_dev->one_shot_dump = 1;
	if (!(fm_dev->fmdevmask & FM_DEV_DUMP_FILE_ENABLE))
		mtk_fm_dev_buf_va(fm_dev, true);

	/* wait up to timeout * img_num * 10 sec */
	for (i = 0; i < FLOW_TIMEOUT * fm_dev->img_num * 10; i++) {
		if (!fm_dev->one_shot_dump)
			break;
		usleep_range(500, 1000);
	}

	fm_dev->one_shot_dump = 0;
	if (!(fm_dev->fmdevmask & FM_DEV_DUMP_FILE_ENABLE))
		mtk_fm_dev_buf_va(fm_dev, false);
	if (i == FLOW_TIMEOUT * fm_dev->img_num)
		return -ETIME;
	else
		return 0;
}

static long mtk_fm_dev_ioctl(struct file *flip, unsigned int cmd,
	unsigned long arg)
{
	struct mtk_fm_dev *fm_dev;
	struct device *dev;
	int ret = 0;

	if (!flip) {
		pr_err("mtk_fm_dev_ioctl flip is NULL!\n");
		return -EFAULT;
	}

	fm_dev = flip->private_data;
	dev = fm_dev->dev_fm;

	switch (cmd) {
	case MTK_FM_DEV_IOCTL_IMPORT_FD_TO_HANDLE:
		ret = mtk_fm_dev_ioctl_import_fd_to_handle(dev, arg);
		break;
	case MTK_FM_DEV_IOCTL_SET_BUF:
		ret = mtk_fm_dev_ioctl_set_buf(fm_dev, arg);
		break;
	case MTK_FM_DEV_IOCTL_SET_FM_PROP:
		ret = mtk_fm_dev_ioctl_set_fm_prop(fm_dev, arg);
		break;
	case MTK_FM_DEV_IOCTL_STREAMON:
		ret = mtk_fm_dev_ioctl_streamon(fm_dev);
		break;
	case MTK_FM_DEV_IOCTL_STREAMOFF:
		ret = mtk_fm_dev_ioctl_streamoff(fm_dev);
		break;
	case MTK_FM_DEV_IOCTL_TRIGGER:
		ret = mtk_fm_dev_path_trigger_start(fm_dev, true);
		break;
	case MTK_FM_DEV_IOCTL_COMP:
		ret = mtk_fm_dev_ioctl_comp(fm_dev, arg);
		break;
	case MTK_FM_DEV_IOCTL_PUT_HANDLE:
		ret = mtk_fm_dev_ioctl_put_handle(dev, arg);
		break;
	case MTK_FM_DEV_IOCTL_SET_VPU_REQ:
		ret = mtk_fm_dev_ioctl_set_vpu_req(fm_dev, arg);
		break;
	case MTK_FM_DEV_IOCTL_CHECK_FLOW_DONE:
		ret = mtk_fm_dev_ioctl_check_flow_done(fm_dev);
		break;
	case MTK_FM_DEV_IOCTL_GET_STATISTICS:
		ret = mtk_fm_dev_ioctl_get_statistic(fm_dev, arg);
		break;
	case MTK_FM_DEV_IOCTL_SET_LOG_LEVEL:
		ret = mtk_fm_dev_ioctl_set_log_level(arg);
		break;
	case MTK_FM_DEV_IOCTL_DEBUG_TEST:
		ret = mtk_fm_dev_ioctl_debug_test(fm_dev, arg);
		break;
	case MTK_FM_DEV_IOCTL_FLUSH_BUF_CACHE:
		ret = mtk_fm_dev_ioctl_flush_cache(fm_dev);
		break;
	case MTK_FM_DEV_IOCTL_ONE_SHOT_DUMP:
		ret = mtk_fm_dev_ioctl_one_shot_dump(fm_dev);
		break;
	default:
		dev_err(dev, "fm_dev_ioctl: no such command!\n");
		return -EINVAL;
	}

	return ret;
}

static const struct file_operations mtk_fm_dev_fops = {
	.owner = THIS_MODULE,
	.open = mtk_fm_dev_open,
	.release = mtk_fm_dev_release,
	.unlocked_ioctl = mtk_fm_dev_ioctl,
};

static int mtk_fm_dev_reg_chardev(struct mtk_fm_dev *fm)
{
	struct device *dev;
	char fm_dev_name[] = "mtk_fm_dev";
	int ret;

	dev_dbg(fm->dev, "fm_dev_reg_chardev\n");

	ret = alloc_chrdev_region(&fm->devt, 0, 1,
			fm_dev_name);
	if (ret < 0) {
		pr_err("fm_dev_reg_chardev: alloc_chrdev_region failed, %d\n",
				ret);
		return ret;
	}

	dev_dbg(fm->dev, "fm_dev_reg_chardev: MAJOR/MINOR = 0x%08x\n",
		fm->devt);

	/* Attatch file operation. */
	cdev_init(&fm->chrdev, &mtk_fm_dev_fops);
	fm->chrdev.owner = THIS_MODULE;

	/* Add to system */
	ret = cdev_add(&fm->chrdev, fm->devt, 1);
	if (ret < 0) {
		pr_err("fm_dev_reg_chardev: attatch file operation failed, %d\n",
				ret);
		unregister_chrdev_region(fm->devt, 1);
		return -EIO;
	}

	/* Create class register */
	fm->dev_class = class_create(THIS_MODULE, fm_dev_name);
	if (IS_ERR(fm->dev_class)) {
		ret = PTR_ERR(fm->dev_class);
		pr_err("Unable to create class, err = %d\n", ret);
		return -EIO;
	}

	dev = device_create(fm->dev_class, NULL, fm->devt,
				NULL, fm_dev_name);
	if (IS_ERR(dev)) {
		ret = PTR_ERR(dev);
		pr_err("Failed to create device: /dev/%s, err = %d",
				fm_dev_name, ret);
		return -EIO;
	}

	dev_dbg(fm->dev, "fm_dev_reg_chardev done\n");

	return 0;
}

static int mtk_fm_dev_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_fm_dev *fm;
	int ret;

	fm = devm_kzalloc(dev, sizeof(*fm), GFP_KERNEL);
	if (!fm)
		return -ENOMEM;

	fm->dev = dev;

	fm->polling_wait = 1;
	fm->hdr_idx_cur = 0;
	fm->hdr_idx_cur = 0;
	fm->flow_done = 0;
	fm->fm_flush_wait = 0;
	fm->warpa_flush_wait = 0;
	fm->fe_flush_wait = 0;
	kern_log_level = CV_LOG_INFO;

	ret = mtk_fm_dev_get_device(dev, "mediatek,mmsys_common_top",
			0, &fm->dev_mmsys_cmmn_top);
	if (ret)
		return ret;

	ret = mtk_fm_dev_get_device(dev, "mediatek,sysram", 0, &fm->dev_sysram);
	if (ret)
		return ret;

	ret = mtk_fm_dev_get_device(dev, "mediatek,fm", 0, &fm->dev_fm);
	if (ret)
		return ret;

	ret = mtk_fm_dev_reg_chardev(fm);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, fm);

	of_property_read_u32_array(dev->of_node, "gce-events", fm->cmdq_events,
			MTK_FM_DEV_CMDQ_EVENT_MAX);

	INIT_WORK(&fm->trigger_work, mtk_fm_dev_trigger_work);
	INIT_WORK(&fm->warpa_et_work, mtk_warpa_execution_time_work);
	INIT_WORK(&fm->fe_et_work, mtk_fe_execution_time_work);
	INIT_WORK(&fm->wen0_incomp_work, mtk_rbfc_wen0_incomp_work);
	INIT_WORK(&fm->wen1_incomp_work, mtk_rbfc_wen1_incomp_work);
	INIT_WORK(&fm->wen2_incomp_work, mtk_rbfc_wen2_incomp_work);
	INIT_WORK(&fm->wen3_incomp_work, mtk_rbfc_wen3_incomp_work);
	INIT_WORK(&fm->ren_incomp_work, mtk_rbfc_ren_incomp_work);
	INIT_WORK(&fm->be0_lim_work, mtk_be0_lim_work);
	INIT_WORK(&fm->be1_lim_work, mtk_be1_lim_work);
	INIT_WORK(&fm->be2_lim_work, mtk_be2_lim_work);
	INIT_WORK(&fm->be3_lim_work, mtk_be3_lim_work);
	create_fm_dev_statistic_buf(fm);
	return 0;
}

static int mtk_fm_dev_remove(struct platform_device *pdev)
{
	struct mtk_fm_dev *fm = platform_get_drvdata(pdev);

	destroy_fm_dev_statistic_buf(fm);

	device_destroy(fm->dev_class, fm->devt);
	class_destroy(fm->dev_class);
	cdev_del(&fm->chrdev);
	unregister_chrdev_region(fm->devt, 1);
	return 0;
}

static const struct of_device_id fm_dev_driver_dt_match[] = {
	{ .compatible = "mediatek,mt3612-fm-dev" },
	{},
};
MODULE_DEVICE_TABLE(of, fm_dev_driver_dt_match);

struct platform_driver mtk_fm_dev_driver = {
	.probe		= mtk_fm_dev_probe,
	.remove		= mtk_fm_dev_remove,
	.driver		= {
		.name	= "mediatek-fm-dev",
		.owner	= THIS_MODULE,
		.of_match_table = fm_dev_driver_dt_match,
	},
};

module_platform_driver(mtk_fm_dev_driver);
MODULE_LICENSE("GPL");
