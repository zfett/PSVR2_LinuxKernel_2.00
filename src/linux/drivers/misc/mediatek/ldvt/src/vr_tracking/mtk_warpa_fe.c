/*
 * Copyright (c) 2017 MediaTek Inc.
 * Authors:
 *	CK Hu <ck.hu@mediatek.com>
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
#include <linux/gcd.h>
#include <mtk_warpa_fe.h>
#include <mtk_common_util.h>
#include <mtk_disp.h>
#include <soc/mediatek/mtk_fe.h>
#include <soc/mediatek/mtk_mmsys_cmmn_top.h>
#include <soc/mediatek/mtk_mmsys_cfg.h>
#include <soc/mediatek/mtk_mutex.h>
#include <soc/mediatek/mtk_rbfc.h>
#include <soc/mediatek/mtk_warpa.h>
#include <soc/mediatek/mtk_wdma.h>
#include <soc/mediatek/mtk_resizer.h>

#define RUN_FE_BY_DELAYED_SOF 0
#define REGISTER_WARPA_CB 0
#define FLOW_TIMEOUT 2000 /* usleep_range(500, 1000) */

#define MAX_WPE_DISTORT 20
#define FEO_GOLEDN_PATH_LEN 200

static void mtk_wdma0_cb(struct mtk_mmsys_cb_data *data)
{
	struct mtk_warpa_fe *warpa_fe = data->priv_data;

	warpa_fe->wdma0_cnt++;

#if 0
	struct mtk_common_buf *buf =
		warpa_fe->buf[MTK_WARPA_FE_BUF_TYPE_WDMA_1_0];
	void *va;

	pa_to_vaddr_dev(warpa_fe->dev_wdma0, buf, buf->pitch * 640, true);

	va = kmalloc(640*640, GFP_KERNEL);
	move_sram_to_va(warpa_fe->dev_sysram, 0x0, va, 640*640);
	mtk_common_write_file(va, "p2_sysram_dump", 640*640);
	kfree(va);
#endif

	LOG(CV_LOG_DBG, "wdma0 cb works frame done!\n");
}

static void mtk_wdma1_cb(struct mtk_mmsys_cb_data *data)
{
	struct mtk_warpa_fe *warpa_fe = data->priv_data;

	warpa_fe->wdma1_cnt++;
#if 0
	struct mtk_common_buf *buf =
		warpa_fe->buf[MTK_WARPA_FE_BUF_TYPE_WDMA_1_0];
	void *va;

	pa_to_vaddr_dev(warpa_fe->dev_wdma0, buf, buf->pitch * 640, true);

	va = kmalloc(640*640, GFP_KERNEL);
	move_sram_to_va(warpa_fe->dev_sysram, 0x0, va, 640*640);
	mtk_common_write_file(va, "p2_sysram_dump", 640*640);
	kfree(va);
#endif

	LOG(CV_LOG_DBG, "wdma1 cb works frame done!\n");
}

static void mtk_wdma2_cb(struct mtk_mmsys_cb_data *data)
{
	struct mtk_warpa_fe *warpa_fe = data->priv_data;

	warpa_fe->wdma2_cnt++;
#if 0
	struct mtk_common_buf *buf =
		warpa_fe->buf[MTK_WARPA_FE_BUF_TYPE_WDMA_1_0];
	void *va;

	pa_to_vaddr_dev(warpa_fe->dev_wdma0, buf, buf->pitch * 640, true);

	va = kmalloc(640*640, GFP_KERNEL);
	move_sram_to_va(warpa_fe->dev_sysram, 0x0, va, 640*640);
	mtk_common_write_file(va, "p2_sysram_dump", 640*640);
	kfree(va);
#endif

	LOG(CV_LOG_DBG, "wdma2 cb works frame done!\n");
}



static void warpa_fe_work_cb(struct cmdq_cb_data data)
{
	struct mtk_warpa_fe *warpa_fe = data.data;
	int ret;

	ret = cmdq_pkt_destroy(warpa_fe->cmdq_pkt);
	if (ret)
		dev_err(warpa_fe->dev, "cmdq_pkt_destroy fail!\n");
	warpa_fe->cmdq_pkt = NULL;
	complete(&warpa_fe->warpa_fe_work_comp.cmplt);
}

static void be_work_cb(struct cmdq_cb_data data)
{
	struct mtk_warpa_fe *warpa_fe = data.data;

	complete(&warpa_fe->be_work_comp.cmplt);
}

static void p1_sof_work_cb(struct cmdq_cb_data data)
{
	struct mtk_warpa_fe *warpa_fe = data.data;
	int ret;

	ret = cmdq_pkt_destroy(warpa_fe->cmdq_pkt_p1_sof);
	if (ret)
		dev_err(warpa_fe->dev, "cmdq_pkt_p1_sof_destroy fail!\n");
	warpa_fe->cmdq_pkt_p1_sof = NULL;
	complete(&warpa_fe->p1_sof_work_comp.cmplt);
}


static void p2_sof_work_cb(struct cmdq_cb_data data)
{
	struct mtk_warpa_fe *warpa_fe = data.data;
	int ret;

	ret = cmdq_pkt_destroy(warpa_fe->cmdq_pkt_p2_sof);
	if (ret)
		dev_err(warpa_fe->dev, "cmdq_pkt_p2_sof_destroy fail!\n");
	warpa_fe->cmdq_pkt_p2_sof = NULL;
	complete(&warpa_fe->p2_sof_work_comp.cmplt);
}

static int mtk_cv_enable(struct device *dev, u32 camerea_id, u32 vsync_cycle,
			 u32 delay_cycle, u8 n, u8 m, struct cmdq_pkt **handle)
{
	mtk_mmsys_cfg_camera_sync_clock_sel(dev, true);
	mtk_mmsys_cfg_camera_sync_config(dev, camerea_id, vsync_cycle,
					delay_cycle, true, handle);
	mtk_mmsys_cfg_camera_sync_frc(dev, camerea_id, n, m, handle);
	mtk_mmsys_cfg_camera_sync_enable(dev, camerea_id, handle);

	return 0;
}

static int mtk_cv_disable(struct device *dev, u32 camerea_id,
			struct cmdq_pkt **handle)
{
	mtk_mmsys_cfg_camera_sync_disable(dev, camerea_id, handle);

	return 0;
}


#if REGISTER_WARPA_CB
static void mtk_warpa_fe_warpa_cb(struct mtk_mmsys_cb_data *data)
{
	struct mtk_warpa_fe *warpa_fe = data->priv_data;
	u32 status = data->status;

	if (status & 0x1F)
		dev_err(warpa_fe->dev, "mtk_warpa_fe_warpa_cb: status %x\n",
			data->status);
}
#endif
static void dump_buf(struct mtk_warpa_fe *warpa_fe, int buf_idx, int size,
	char *file_path)
{
	pa_to_vaddr_dev(warpa_fe->dev_warpa, warpa_fe->buf[buf_idx], size,
			true);
	LOG(CV_LOG_INFO, "dump idx:%d, size:%d @%p\n", buf_idx, size,
		(void *)warpa_fe->buf[buf_idx]->kvaddr);
	mtk_common_write_file(warpa_fe->buf[buf_idx]->kvaddr, file_path, size);
	pa_to_vaddr_dev(warpa_fe->dev_warpa, warpa_fe->buf[buf_idx], size,
			false);
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

static void create_warpa_fe_statistic_buf(struct mtk_warpa_fe *warpa_fe)
{
	warpa_fe->p1_done_cnt_va = init_buffer(warpa_fe->dev,
			&warpa_fe->p1_done_cnt_pa);

	warpa_fe->P1_et_va = init_buffer(warpa_fe->dev,
			&warpa_fe->P1_et_pa);

	warpa_fe->P1_et_max_va = init_buffer(warpa_fe->dev,
			&warpa_fe->P1_et_max_pa);

	warpa_fe->BE_VPU_delay_va = init_buffer(warpa_fe->dev,
			&warpa_fe->BE_VPU_delay_pa);

	warpa_fe->BE_VPU_delay_max_va = init_buffer(warpa_fe->dev,
			&warpa_fe->BE_VPU_delay_max_pa);

	warpa_fe->BE_VPU_et_va = init_buffer(warpa_fe->dev,
			&warpa_fe->BE_VPU_et_pa);

	warpa_fe->BE_VPU_et_max_va = init_buffer(warpa_fe->dev,
			&warpa_fe->BE_VPU_et_max_pa);

	warpa_fe->P1_VPU_et_va = init_buffer(warpa_fe->dev,
			&warpa_fe->P1_VPU_et_pa);

	warpa_fe->P1_VPU_et_max_va = init_buffer(warpa_fe->dev,
			&warpa_fe->P1_VPU_et_max_pa);

	warpa_fe->be_vpu_done_cnt_va = init_buffer(warpa_fe->dev,
			&warpa_fe->be_vpu_done_cnt_pa);

	warpa_fe->p1_sof_cnt_va = init_buffer(warpa_fe->dev,
			&warpa_fe->p1_sof_cnt_pa);

	warpa_fe->p2_sof_cnt_va = init_buffer(warpa_fe->dev,
			&warpa_fe->p2_sof_cnt_pa);

	warpa_fe->p2_done_cnt_va = init_buffer(warpa_fe->dev,
			&warpa_fe->p2_done_cnt_pa);
}
static void destroy_warpa_fe_statistic_buf(struct mtk_warpa_fe *warpa_fe)
{
	deinit_buffer(warpa_fe->dev, warpa_fe->p1_done_cnt_pa,
			warpa_fe->p1_done_cnt_va);
	deinit_buffer(warpa_fe->dev, warpa_fe->P1_et_pa, warpa_fe->P1_et_va);
	deinit_buffer(warpa_fe->dev, warpa_fe->P1_et_max_pa,
			warpa_fe->P1_et_max_va);
	deinit_buffer(warpa_fe->dev, warpa_fe->BE_VPU_delay_pa,
			warpa_fe->BE_VPU_delay_va);
	deinit_buffer(warpa_fe->dev, warpa_fe->BE_VPU_delay_max_pa,
			warpa_fe->BE_VPU_delay_max_va);
	deinit_buffer(warpa_fe->dev, warpa_fe->BE_VPU_et_pa,
			warpa_fe->BE_VPU_et_va);
	deinit_buffer(warpa_fe->dev, warpa_fe->BE_VPU_et_max_pa,
			warpa_fe->BE_VPU_et_max_va);
	deinit_buffer(warpa_fe->dev, warpa_fe->P1_VPU_et_pa,
			warpa_fe->P1_VPU_et_va);
	deinit_buffer(warpa_fe->dev, warpa_fe->P1_VPU_et_max_pa,
			warpa_fe->P1_VPU_et_max_va);
	deinit_buffer(warpa_fe->dev, warpa_fe->be_vpu_done_cnt_pa,
			warpa_fe->be_vpu_done_cnt_va);
	deinit_buffer(warpa_fe->dev, warpa_fe->p1_sof_cnt_pa,
			warpa_fe->p1_sof_cnt_va);
	deinit_buffer(warpa_fe->dev, warpa_fe->p2_sof_cnt_pa,
			warpa_fe->p2_sof_cnt_va);
	deinit_buffer(warpa_fe->dev, warpa_fe->p2_done_cnt_pa,
			warpa_fe->p2_done_cnt_va);
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

#if 0
static void print_done_interval(char *module_name, struct timeval prev,
	struct timeval cur)
{
	suseconds_t diff;

	diff = cur.tv_sec * 1000000 - prev.tv_sec * 1000000 +
		cur.tv_usec - prev.tv_usec;
	LOG(CV_LOG_INFO, "%s Done intarval: %lu us\n", module_name, diff);
}
#endif

static void dump_ir_buffers(struct mtk_warpa_fe *warpa_fe, int frame_index)
{
	char file_path[256];
	int r_idx = frame_index % warpa_fe->be_ring_buf_cnt;

	LOG(CV_LOG_INFO, "start to dump p1 img\n");
	if (warpa_fe->p1vpu_pair == 1) {
		snprintf(file_path, sizeof(file_path),
			 "/mnt/sda1/vr_tracking/3a/p1_img_%d.raw",
			 *warpa_fe->be_vpu_done_cnt_va);
		dump_buf(warpa_fe, MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R0 +
			 r_idx, warpa_fe->buf[
			 MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R0 + r_idx]->pitch,
			 file_path);
	} else {
		if (warpa_fe->path_mask & (1 << 0)) {
			snprintf(file_path, sizeof(file_path),
			"/mnt/sda1/vr_tracking/3a/p1_img_sidell_%d.raw",
			*warpa_fe->be_vpu_done_cnt_va);
			dump_buf(warpa_fe,
				 MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R0 +
				 r_idx, warpa_fe->buf[
				 MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R0 +
				 r_idx]->pitch,
				 file_path);
		}

		if (warpa_fe->path_mask & (1 << 2)) {
			snprintf(file_path, sizeof(file_path),
			"/mnt/sda1/vr_tracking/3a/p1_img_sidelr_%d.raw",
			*warpa_fe->be_vpu_done_cnt_va);
			dump_buf(warpa_fe,
				 MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R0 +
				 r_idx, warpa_fe->buf[
				 MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R0 +
				 r_idx]->pitch,
				 file_path);
		}

		if (warpa_fe->path_mask & (1 << 4)) {
			snprintf(file_path, sizeof(file_path),
			"/mnt/sda1/vr_tracking/3a/p1_img_siderl_%d.raw",
			*warpa_fe->be_vpu_done_cnt_va);
			dump_buf(warpa_fe,
				 MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R0 +
				 r_idx, warpa_fe->buf[
				 MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R0 +
				 r_idx]->pitch,
				 file_path);
		}

		if (warpa_fe->path_mask & (1 << 6)) {
			snprintf(file_path, sizeof(file_path),
			"/mnt/sda1/vr_tracking/3a/p1_img_siderr_%d.raw",
			*warpa_fe->be_vpu_done_cnt_va);
			dump_buf(warpa_fe,
				 MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R0 +
				 r_idx, warpa_fe->buf[
				 MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R0 +
				 r_idx]->pitch,
				 file_path);
		}
	}

	if (warpa_fe->be_en == 1) {
		LOG(CV_LOG_INFO, "start to dump be data info\n");
		if (warpa_fe->path_mask & (1 << 0)) {
			snprintf(file_path, sizeof(file_path),
			"/mnt/sda1/vr_tracking/3a/be_datainfo_sidell_%d.bin",
			*warpa_fe->be_vpu_done_cnt_va);
			dump_buf(warpa_fe,
				 MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R0 +
				 r_idx, warpa_fe->buf[
				 MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R0 +
				 r_idx]->pitch,
				 file_path);
		}

		if (warpa_fe->path_mask & (1 << 2)) {
			snprintf(file_path, sizeof(file_path),
			"/mnt/sda1/vr_tracking/3a/be_datainfo_sidelr_%d.bin",
			*warpa_fe->be_vpu_done_cnt_va);
			dump_buf(warpa_fe,
				 MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R0 +
				 r_idx, warpa_fe->buf[
				 MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R0 +
				 r_idx]->pitch,
				 file_path);
		}

		if (warpa_fe->path_mask & (1 << 4)) {
			snprintf(file_path, sizeof(file_path),
			"/mnt/sda1/vr_tracking/3a/be_datainfo_siderl_%d.bin",
			*warpa_fe->be_vpu_done_cnt_va);
			dump_buf(warpa_fe,
				 MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R0 +
				 r_idx, warpa_fe->buf[
				 MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R0 +
				 r_idx]->pitch,
				 file_path);
		}

		if (warpa_fe->path_mask & (1 << 6)) {
			snprintf(file_path, sizeof(file_path),
			"/mnt/sda1/vr_tracking/3a/be_datainfo_siderr_%d.bin",
			*warpa_fe->be_vpu_done_cnt_va);
			dump_buf(warpa_fe,
				 MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R0 +
				 r_idx, warpa_fe->buf[
				 MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R0 +
				 r_idx]->pitch,
				 file_path);
		}

		if (warpa_fe->be_label_img_en == 1) {
			if (warpa_fe->path_mask & (1 << 0)) {
			snprintf(file_path, sizeof(file_path),
			"/mnt/sda1/vr_tracking/3a/be_label_img_sidell_%d.raw",
			*warpa_fe->be_vpu_done_cnt_va);
			dump_buf(warpa_fe,
				 MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R0 +
				 r_idx, warpa_fe->buf[
				 MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R0 +
				 r_idx]->pitch,
				 file_path);
			}

			if (warpa_fe->path_mask & (1 << 2)) {
			snprintf(file_path, sizeof(file_path),
			"/mnt/sda1/vr_tracking/3a/be_label_img_sidelr_%d.raw",
			*warpa_fe->be_vpu_done_cnt_va);
			dump_buf(warpa_fe,
				 MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R0 +
				 r_idx, warpa_fe->buf[
				 MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R0 +
				 r_idx]->pitch,
				 file_path);
			}

			if (warpa_fe->path_mask & (1 << 4)) {
			snprintf(file_path, sizeof(file_path),
			"/mnt/sda1/vr_tracking/3a/be_label_img_siderl_%d.raw",
			*warpa_fe->be_vpu_done_cnt_va);
			dump_buf(warpa_fe,
				 MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R0 +
				 r_idx, warpa_fe->buf[
				 MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R0 +
				 r_idx]->pitch,
				 file_path);
			}

			if (warpa_fe->path_mask & (1 << 6)) {
			snprintf(file_path, sizeof(file_path),
			"/mnt/sda1/vr_tracking/3a/be_label_img_siderr_%d.raw",
			*warpa_fe->be_vpu_done_cnt_va);
			dump_buf(warpa_fe,
				 MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R0 +
				 r_idx, warpa_fe->buf[
				 MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R0 +
				 r_idx]->pitch,
				 file_path);
			}
		}
	}

	snprintf(file_path, sizeof(file_path),
		"/mnt/sda1/vr_tracking/3a/vpu_edge_detect_%d.raw",
		*warpa_fe->be_vpu_done_cnt_va);
	dump_buf(warpa_fe, MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R0 +
		r_idx, warpa_fe->buf[
		MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R0 + r_idx]->pitch,
		file_path);

	LOG(CV_LOG_INFO, "ir path file dump completed!, idx:%d\n",
		*warpa_fe->be_vpu_done_cnt_va);
	warpa_fe->one_shot_dump = 0;
}

static int mtk_warpa_fe_compare(char *feo1_golden, char *feo1,
	char *feo2_golden, char *feo2, u32 blk_num)
{
	u32 i, j;
	u32 idx;
	u32 err_cnt = 0;

	pr_debug("fe compare function\n");

	for (i = 0; i < blk_num; i++) {
		/* check valid bit of current blk */
		if (feo1_golden[i * 8 + 4] & 0x1) {
			for (j = 0; j < 8; j++) {
				idx = i * 8 + j;
				if (feo1_golden[idx] != feo1[idx]) {
					pr_err(
						"feo1 diff @%d, golden=%d, feo1=%d\n",
						idx, feo1_golden[idx],
						feo1[idx]);
					err_cnt++;
				}
			}
			for (j = 0; j < 64; j++) {
				idx = i * 64 + j;
				if (feo2_golden[idx] != feo2[idx]) {
					pr_err(
						"feo2 diff @%d, golden=%d, feo2=%d\n",
						idx, feo2_golden[idx],
						feo2[idx]);
					err_cnt++;
				}
			}
			if (err_cnt > 20) {
				pr_err("err_cnt > 20, stop compare!\n");
				goto out;
			}
		} else
			pr_debug("fe blk %d invalid, pass it\n", i);
	}
	if (err_cnt > 0)
		goto out;
	pr_err("fe file compare pass!\n");
	return 0;
out:
	pr_err("fe file compare fail, err_cnt = %d\n", err_cnt);
	return -EIO;
}

static int mtk_warpa_fe_compare_feo(struct mtk_warpa_fe *warpa_fe,
	char *feoP_golden_path, char *feoD_golden_path,
	int feo1_idx, int feo2_idx, int feoP_size, int feoD_size)
{
	int ret = 0;
	void *feo1_golden_va, *feo2_golden_va;
	char feo1_golden[FEO_GOLEDN_PATH_LEN], feo2_golden[FEO_GOLEDN_PATH_LEN];

	memset(feo1_golden, 0, FEO_GOLEDN_PATH_LEN);
	memset(feo2_golden, 0, FEO_GOLEDN_PATH_LEN);

	if (strlen(feoP_golden_path) > FEO_GOLEDN_PATH_LEN) {
		pr_err("file path length is not enough!\n");
		return -EINVAL;
	}
	mtk_common_file_path(feo1_golden, feoP_golden_path);

	if (strlen(feoD_golden_path) > FEO_GOLEDN_PATH_LEN) {
		pr_err("file path length is not enough!\n");
		return -EINVAL;
	}
	mtk_common_file_path(feo2_golden, feoD_golden_path);

	feo1_golden_va = kmalloc(feoP_size, GFP_KERNEL);
	if (!feo1_golden_va)
		return -ENOMEM;
	feo2_golden_va = kmalloc(feoD_size, GFP_KERNEL);
	if (!feo2_golden_va) {
		kfree(feo1_golden_va);
		return -ENOMEM;
	}

	mtk_common_read_file(feo1_golden_va, feo1_golden, feoP_size);
	if (!feo1_golden_va) {
		pr_err("feo1_golden_va is NULL!\n");
		return -EFAULT;
	}
	mtk_common_read_file(feo2_golden_va, feo2_golden, feoD_size);
	if (!feo2_golden_va) {
		pr_err("feo2_golden_va is NULL!\n");
		kfree(feo1_golden_va);
		return -EFAULT;
	}

	pr_debug("compare fe output\n");

	pa_to_vaddr_dev(warpa_fe->dev_fe, warpa_fe->buf[feo1_idx], feoP_size,
			true);
	pa_to_vaddr_dev(warpa_fe->dev_fe, warpa_fe->buf[feo2_idx], feoD_size,
			true);

	pr_debug("start to compare feo1 with %s\n", feoP_golden_path);
	pr_debug("start to compare feo2 with %s\n", feoD_golden_path);
	ret = mtk_warpa_fe_compare(feo1_golden_va,
				   warpa_fe->buf[feo1_idx]->kvaddr,
				   feo2_golden_va,
				   warpa_fe->buf[feo2_idx]->kvaddr,
				   feoP_size / 8);
	pa_to_vaddr_dev(warpa_fe->dev_fe, warpa_fe->buf[feo1_idx], feoP_size,
			false);
	pa_to_vaddr_dev(warpa_fe->dev_fe, warpa_fe->buf[feo2_idx], feoD_size,
			false);

	pr_debug("fe golden compare done!\n");

	kfree(feo1_golden_va);
	kfree(feo2_golden_va);

	return ret;
}
static int mtk_warpa_fe_config_rbfc(struct mtk_warpa_fe *warpa_fe)
{
	int ret = 0;
	u32 r_th[1] = {0};
	u32 rbfc_act_nr[2] = { 1, 1};
	u32 rbfc_in_pitch;
	u32 rbfc_w, rbfc_h;

	ret = mtk_rbfc_set_plane_num(warpa_fe->dev_rbfc, 1);
	if (ret) {
		dev_err(warpa_fe->dev,
			"rbfc_set_plane_num fail!! %d\n", ret);
		return ret;
	}

	switch (warpa_fe->img_num) {
	case 1:
		rbfc_act_nr[0] = 0x1;
		rbfc_w = warpa_fe->warpa_in_w;
		rbfc_h = warpa_fe->warpa_in_h;
		break;
	case 2:
		rbfc_act_nr[0] = 0x3;
		rbfc_w = warpa_fe->warpa_in_w * 2;
		rbfc_h = warpa_fe->warpa_in_h;
		break;
	case 4:
		rbfc_act_nr[0] = 0xf;
		rbfc_w = warpa_fe->warpa_in_w * 4;
		rbfc_h = warpa_fe->warpa_in_h;
		break;
	default:
		pr_err("err in img_num:%d\n", warpa_fe->img_num);
		return -EINVAL;
	}

	r_th[0] = (roundup(rbfc_h * MAX_WPE_DISTORT, 100) / 100) + 8;

	rbfc_in_pitch = roundup(warpa_fe->warpa_in_w, warpa_fe->warpa_align) *
			warpa_fe->img_num;
	ret = mtk_rbfc_set_active_num(warpa_fe->dev_rbfc, NULL, rbfc_act_nr);
	if (ret) {
		dev_err(warpa_fe->dev,
			"rbfc_set_active_num fail!! %d\n", ret);
		return ret;
	}

	ret = mtk_rbfc_set_region_multi(warpa_fe->dev_rbfc, NULL, 0,
		rbfc_w, rbfc_h);
	if (ret) {
		dev_err(warpa_fe->dev,
			"rbfc_set_region_multi fail!! %d\n", ret);
		return ret;
	}
	if (warpa_fe->warpafemask & WARPA_FE_WPE_INPUT_FROM_SYSRAM) {
		ret = mtk_rbfc_set_target(warpa_fe->dev_rbfc, NULL,
			MTK_RBFC_TO_SYSRAM);
		if (ret) {
			dev_err(warpa_fe->dev,
				"rbfc_set_SYSRAM_target fail!! %d\n", ret);
			return ret;
		}
		if (warpa_fe->warpafemask & WARPA_FE_HALF_BUFFER_MASK)
			ret = mtk_rbfc_set_ring_buf_multi(
				warpa_fe->dev_rbfc,
				NULL,
				0,
				WARPA_IN_SRAM_ADDR_SHARE,
				rbfc_in_pitch,
				warpa_fe->warpa_in_h / 2);
		else if (warpa_fe->warpafemask & WARPA_FE_LINE_BUFFER_MASK)
			/* when warp map with max distortion 20% */
			/* need to set drop cnt as 20% frame height +7 */
			/* and set read thd as 20% frame height +8 */
			/* so set 40% frame height + 16 for line mode */
			ret = mtk_rbfc_set_ring_buf_multi(
				warpa_fe->dev_rbfc,
				NULL,
				0, WARPA_IN_SRAM_ADDR_SHARE,
				rbfc_in_pitch,
				(roundup(rbfc_h * MAX_WPE_DISTORT * 2, 100) /
				100) + 16);
		else
			ret = mtk_rbfc_set_ring_buf_multi(
				warpa_fe->dev_rbfc,
				NULL,
				0, WARPA_IN_SRAM_ADDR,
				rbfc_in_pitch,
				warpa_fe->warpa_in_h);
		if (ret) {
			dev_err(warpa_fe->dev,
				"rbfc_set_ring_buf_multi fail!! %d\n", ret);
			return ret;
		}
	} else {
		ret = mtk_rbfc_set_target(warpa_fe->dev_rbfc, NULL,
			MTK_RBFC_TO_DRAM);
		if (ret) {
			dev_err(warpa_fe->dev,
				"rbfc_set_DRAM_target fail!! %d\n", ret);
			return ret;
		}
		if (warpa_fe->warpafemask & WARPA_FE_HALF_BUFFER_MASK)
			ret = mtk_rbfc_set_ring_buf_multi(
				warpa_fe->dev_rbfc,
				NULL,
				0,
				warpa_fe->buf[
				MTK_WARPA_FE_BUF_TYPE_WARPA_IMG_BUF_0]->
				dma_addr,
				rbfc_in_pitch,
				warpa_fe->warpa_in_h / 2);
		else if (warpa_fe->warpafemask & WARPA_FE_LINE_BUFFER_MASK)
			ret = mtk_rbfc_set_ring_buf_multi(
				warpa_fe->dev_rbfc,
				NULL,
				0,
				warpa_fe->buf[
				MTK_WARPA_FE_BUF_TYPE_WARPA_IMG_BUF_0]->
				dma_addr,
				rbfc_in_pitch,
				256);
		else
			ret = mtk_rbfc_set_ring_buf_multi(
				warpa_fe->dev_rbfc,
				NULL,
				0,
				0x0,
				rbfc_in_pitch,
				warpa_fe->warpa_in_h);
		if (ret) {
			dev_err(warpa_fe->dev,
				"rbfc_set_ring_buf_multi fail!! %d\n", ret);
			return ret;
		}
	}

	if (warpa_fe->warpafemask & WARPA_FE_RBFC_ENABLE_MASK) {
		ret = mtk_rbfc_set_read_thd(warpa_fe->dev_rbfc, NULL, r_th);
		if (ret) {
			dev_err(warpa_fe->dev,
				"rbfc_set_read_thd fail!! %d\n", ret);
			return ret;
		}

		ret = mtk_rbfc_start_mode(warpa_fe->dev_rbfc,
			NULL, MTK_RBFC_NORMAL_MODE);
		if (ret) {
			dev_err(warpa_fe->dev,
				"rbfc_start_mode fail!! %d\n", ret);
			return ret;
		}

		LOG(CV_LOG_DBG, "enable rbfc\n");
	} else {
		ret = mtk_rbfc_start_mode(warpa_fe->dev_rbfc,
			NULL, MTK_RBFC_DISABLE_MODE);
		if (ret) {
			dev_err(warpa_fe->dev,
				"rbfc_start_mode fail!! %d\n", ret);
			return ret;
		}

		LOG(CV_LOG_DBG, "disable rbfc\n");
	}
	return ret;
}

static int mtk_warpa_fe_config_warpa(struct mtk_warpa_fe *warpa_fe)
{
	int ret = 0;

	LOG(CV_LOG_DBG, "warpa_fe->warpa_in_w %d\n", warpa_fe->warpa_in_w);
	LOG(CV_LOG_DBG, "warpa_fe->warpa_in_h %d\n", warpa_fe->warpa_in_h);
	LOG(CV_LOG_DBG, "warpa_fe->warpa_out_w %d\n", warpa_fe->warpa_out_w);
	LOG(CV_LOG_DBG, "warpa_fe->warpa_out_h %d\n", warpa_fe->warpa_out_h);
	LOG(CV_LOG_DBG, "warpa_fe->warpa_map_w %d\n", warpa_fe->warpa_map_w);
	LOG(CV_LOG_DBG, "warpa_fe->warpa_map_h %d\n", warpa_fe->warpa_map_h);
	LOG(CV_LOG_DBG, "warpa_fe->warpa_align %d\n", warpa_fe->warpa_align);
	LOG(CV_LOG_DBG, "warpa_fe->warpa_proc_mode %d\n", warpa_fe->proc_mode);
	LOG(CV_LOG_DBG, "warpa_fe->warpa_out_mode %d\n",
			warpa_fe->warpa_out_mode);
	LOG(CV_LOG_DBG, "warpa_fe->blk_sz %d\n", warpa_fe->blk_sz);

	switch (warpa_fe->proc_mode) {
	case MTK_WARPA_PROC_MODE_LR:
		warpa_fe->img_num = 2;
		break;
	case MTK_WARPA_PROC_MODE_L_ONLY:
		warpa_fe->img_num = 1;
		break;
	case MTK_WARPA_PROC_MODE_QUAD:
		warpa_fe->img_num = 4;
		break;
	default:
		dev_err(warpa_fe->dev, "invalid proc_mode %d\n",
			warpa_fe->proc_mode);
		return -EINVAL;
	}

#if REGISTER_WARPA_CB
	ret = mtk_warpa_register_cb(warpa_fe->dev_warpa, mtk_warpa_fe_warpa_cb,
				0x7, warpa_fe);
	if (ret) {
		dev_err(warpa_fe->dev_warpa, "warpa cb register fail!\n");
		return ret;
	}
#endif

	ret = mtk_warpa_set_region_in(warpa_fe->dev_warpa,
		warpa_fe->warpa_in_w, warpa_fe->warpa_in_h);
	if (ret) {
		dev_err(warpa_fe->dev,
			"warpa_set_region_in fail!! %d\n", ret);
		return ret;
	}
	ret = mtk_warpa_set_region_out(warpa_fe->dev_warpa,
		warpa_fe->warpa_out_w, warpa_fe->warpa_out_h);
	if (ret) {
		dev_err(warpa_fe->dev,
			"warpa_set_region_out fail!! %d\n", ret);
		return ret;
	}
	ret = mtk_warpa_set_region_map(warpa_fe->dev_warpa,
		warpa_fe->warpa_map_w, warpa_fe->warpa_map_h);
	if (ret) {
		dev_err(warpa_fe->dev,
			"warpa_set_region_map fail!! %d\n", ret);
		return ret;
	}
	ret = mtk_warpa_set_coef_tbl(warpa_fe->dev_warpa, NULL,
		&warpa_fe->coef_tbl);
	if (ret) {
		dev_err(warpa_fe->dev,
			"warpa_set_coef_tbl fail!! %d\n", ret);
		return ret;
	}
	ret = mtk_warpa_set_border_color(warpa_fe->dev_warpa, NULL,
		&warpa_fe->border_color);
	if (ret) {
		dev_err(warpa_fe->dev,
			"warpa_set_border_color fail!! %d\n", ret);
		return ret;
	}
	ret = mtk_warpa_set_proc_mode(warpa_fe->dev_warpa, NULL,
		warpa_fe->proc_mode);
	if (ret) {
		dev_err(warpa_fe->dev,
			"warpa_set_proc_mode fail!! %d\n", ret);
		return ret;
	}

	ret = mtk_warpa_set_out_mode(
			warpa_fe->dev_warpa,
			warpa_fe->warpa_out_mode);
	if (ret) {
		dev_err(warpa_fe->dev,
			"warpa_set_out_mode fail!! %d\n", ret);
		return ret;
	}

	return ret;
}

static int mtk_warpa_fe_config_wdma(struct mtk_warpa_fe *warpa_fe)
{
	int ret = 0;
	int w, h;

	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_WDMA) {
		w = warpa_fe->warpa_out_w;
		h = warpa_fe->warpa_out_h;
		LOG(CV_LOG_DBG, "wdma_0 region w:%d h:%d\n", w, h);
		ret = mtk_wdma_set_region(warpa_fe->dev_wdma0, NULL,
			w, h, w, h, 0, 0);
		if (ret) {
			dev_err(warpa_fe->dev_wdma0,
				"set wdma0 region fail!\n");
			return ret;
		}
		ret = mtk_wdma_register_cb(warpa_fe->dev_wdma0, &mtk_wdma0_cb,
			MTK_WDMA_FRAME_COMPLETE, warpa_fe);
		if (ret) {
			dev_err(warpa_fe->dev_wdma0,
				"register wdma0 cb fail!\n");
			return ret;
		}
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_P_SCALE) {
		w = warpa_fe->rsz_0_out_w +
			warpa_fe->padding_val_1 * (warpa_fe->img_num - 1);
		h = warpa_fe->rsz_0_out_h;

		LOG(CV_LOG_DBG, "wdma_1 region w:%d h:%d\n", w, h);
		ret = mtk_wdma_set_region(warpa_fe->dev_wdma1, NULL,
			w, h, w, h, 0, 0);
		if (ret) {
			dev_err(warpa_fe->dev_wdma1,
				"set wdma1 region fail!\n");
			return ret;
		}
		ret = mtk_wdma_register_cb(warpa_fe->dev_wdma1, &mtk_wdma1_cb,
			MTK_WDMA_FRAME_COMPLETE, warpa_fe);
		if (ret) {
			dev_err(warpa_fe->dev_wdma1,
				"register wdma1 cb fail!\n");
			return ret;
		}

		w = warpa_fe->rsz_1_out_w +
			warpa_fe->padding_val_2 * (warpa_fe->img_num - 1);
		h = warpa_fe->rsz_1_out_h;

		LOG(CV_LOG_DBG, "wdma_2 region w:%d h:%d\n", w, h);
		ret = mtk_wdma_set_region(warpa_fe->dev_wdma2, NULL,
			w, h, w, h, 0, 0);
		if (ret) {
			dev_err(warpa_fe->dev_wdma2,
				"set wdma2 region fail!\n");
			return ret;
		}
		ret = mtk_wdma_register_cb(warpa_fe->dev_wdma2, &mtk_wdma2_cb,
			MTK_WDMA_FRAME_COMPLETE, warpa_fe);
		if (ret) {
			dev_err(warpa_fe->dev_wdma2,
				"register wdma2 cb fail!\n");
			return ret;
		}
	}

	return ret;
}

static int mtk_warpa_fe_config_fe(struct mtk_warpa_fe *warpa_fe)
{
	int ret = 0;

	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_FE) {
		warpa_fe->fe_w = warpa_fe->warpa_out_w;
		warpa_fe->fe_h = warpa_fe->warpa_out_h;
		LOG(CV_LOG_DBG, "fe region w:%d h:%d\n",
			warpa_fe->fe_w, warpa_fe->fe_h);
		LOG(CV_LOG_DBG, "fe blk_size:%d merge_mode:%d\n",
			warpa_fe->blk_sz, warpa_fe->fe_merge_mode);
		LOG(CV_LOG_DBG,
			"fe flat_en:%d, harris:%d, th_g:%d, th_c:%d, cr_val:%d\n",
			warpa_fe->fe_flat_enable, warpa_fe->fe_harris_kappa,
			warpa_fe->fe_th_g, warpa_fe->fe_th_c,
			warpa_fe->fe_cr_val_sel);

		ret = mtk_fe_set_block_size(warpa_fe->dev_fe,
			NULL, warpa_fe->blk_sz);
		if (ret) {
			dev_err(warpa_fe->dev_fe,
				"fe set block_size fail!\n");
			return ret;
		}

		ret = mtk_fe_set_region(warpa_fe->dev_fe, NULL,
			warpa_fe->fe_w, warpa_fe->fe_h,
			warpa_fe->fe_merge_mode);
		if (ret) {
			dev_err(warpa_fe->dev_fe,
				"fe set region fail!\n");
			return ret;
		}

		ret = mtk_fe_set_params(warpa_fe->dev_fe, NULL,
			warpa_fe->fe_flat_enable, warpa_fe->fe_harris_kappa,
			warpa_fe->fe_th_g, warpa_fe->fe_th_c,
			warpa_fe->fe_cr_val_sel);
		if (ret) {
			dev_err(warpa_fe->dev_fe,
				"fe set params fail!\n");
			return ret;
		}
	}
	return ret;
}

static int mtk_warpa_fe_config_rsz(struct mtk_warpa_fe *warpa_fe)
{
	int ret = 0;
	struct mtk_resizer_config config_p_scalar0 = { 0 };
	struct mtk_resizer_config config_p_scalar1 = { 0 };
	/*resizer cofing */

	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_P_SCALE) {
		LOG(CV_LOG_DBG, "rsz0 out region w:%d h:%d\n",
			warpa_fe->rsz_0_out_w, warpa_fe->rsz_0_out_h);
		LOG(CV_LOG_DBG, "rsz1 out region w:%d h:%d\n",
			warpa_fe->rsz_1_out_w, warpa_fe->rsz_1_out_h);

		config_p_scalar0.in_width = warpa_fe->warpa_out_w +
			warpa_fe->padding_val_0 * (warpa_fe->img_num - 1);
		config_p_scalar0.in_height = warpa_fe->warpa_out_h;
		if (warpa_fe->warpafemask & WARPA_FE_RSZ_0_BYPASS_MASK) {
			config_p_scalar0.out_width =
				config_p_scalar0.in_width;
			config_p_scalar0.out_height =
				config_p_scalar0.in_height;
			LOG(CV_LOG_DBG,
				"bypass rsz0 with in/out w:%d h:%d\n",
				config_p_scalar0.in_width,
				config_p_scalar0.in_height);
		} else {
			config_p_scalar0.out_width = warpa_fe->rsz_0_out_w;
			config_p_scalar0.out_height = warpa_fe->rsz_0_out_h;
		}

		ret = mtk_resizer_config(warpa_fe->dev_p_rsz0,
			&(warpa_fe->cmdq_handles), &config_p_scalar0);
		if (ret) {
			dev_err(warpa_fe->dev_p_rsz0,
				"rsz0 set region fail!\n");
			return ret;
		}

		config_p_scalar1.in_width = warpa_fe->rsz_0_out_w;
		config_p_scalar1.in_height = warpa_fe->rsz_0_out_h;
		if (warpa_fe->warpafemask & WARPA_FE_RSZ_1_BYPASS_MASK) {
			config_p_scalar1.out_width =
				config_p_scalar1.in_width;
			config_p_scalar1.out_height =
				config_p_scalar1.in_height;
			LOG(CV_LOG_DBG,
				"bypass rsz1 with in/out w:%d h:%d\n",
				config_p_scalar1.in_width,
				config_p_scalar1.in_height);
		} else {
			config_p_scalar1.out_width = warpa_fe->rsz_1_out_w;
			config_p_scalar1.out_height = warpa_fe->rsz_1_out_h;
		}
		ret = mtk_resizer_config(warpa_fe->dev_p_rsz1,
			&(warpa_fe->cmdq_handles), &config_p_scalar1);
		if (ret) {
			dev_err(warpa_fe->dev_p_rsz1,
				"rsz1 set region fail!\n");
			return ret;
		}
	}

	return ret;
}

static int mtk_warpa_fe_config_padding(struct mtk_warpa_fe *warpa_fe)
{
	int ret = 0;
	int padding_mode = warpa_fe->img_num == 2 ? 1 : 0;

	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_P_SCALE) {
		LOG(CV_LOG_DBG, "padding val 0:%d 1:%d 2:%d\n",
			warpa_fe->padding_val_0,
			warpa_fe->padding_val_1,
			warpa_fe->padding_val_2);

		if (warpa_fe->warpafemask & WARPA_FE_PADDING_0_BYPASS_MASK) {
			warpa_fe->padding_val_0 = 0;
			LOG(CV_LOG_DBG, "bypass padding 0\n");
		}
		if (warpa_fe->warpafemask & WARPA_FE_PADDING_1_BYPASS_MASK) {
			warpa_fe->padding_val_1 = 0;
			LOG(CV_LOG_DBG, "bypass padding 1\n");
		}
		if (warpa_fe->warpafemask & WARPA_FE_PADDING_2_BYPASS_MASK) {
			warpa_fe->padding_val_2 = 0;
			LOG(CV_LOG_DBG, "bypass padding 2\n");
		}

		ret = mtk_padding_config(warpa_fe->dev_padding0,
			warpa_fe->cmdq_handles,
			padding_mode, warpa_fe->padding_val_0,
			warpa_fe->warpa_out_w, warpa_fe->warpa_out_h);
		if (ret) {
			dev_err(warpa_fe->dev_padding0,
				"padding0 config fail!\n");
			return ret;
		}
		ret = mtk_padding_config(warpa_fe->dev_padding1,
			warpa_fe->cmdq_handles,
			padding_mode, warpa_fe->padding_val_1,
			warpa_fe->rsz_0_out_w, warpa_fe->rsz_0_out_h);
		if (ret) {
			dev_err(warpa_fe->dev_padding1,
				"padding1 config fail!\n");
			return ret;
		}
		ret = mtk_padding_config(warpa_fe->dev_padding2,
			warpa_fe->cmdq_handles,
			padding_mode, warpa_fe->padding_val_2,
			warpa_fe->rsz_1_out_w, warpa_fe->rsz_1_out_h);
		if (ret) {
			dev_err(warpa_fe->dev_padding2,
				"padding2 config fail!\n");
			return ret;
		}

		ret = mtk_padding_set_bypass(warpa_fe->dev_padding0,
			warpa_fe->cmdq_handles,
			warpa_fe->padding_val_0 > 0 ? 0 : 1);
		if (ret) {
			dev_err(warpa_fe->dev_padding0,
				"padding0 set bypass fail!\n");
			return ret;
		}
		ret = mtk_padding_set_bypass(warpa_fe->dev_padding1,
			warpa_fe->cmdq_handles,
			warpa_fe->padding_val_1 > 0 ? 0 : 1);
		if (ret) {
			dev_err(warpa_fe->dev_padding1,
				"padding1 set bypass fail!\n");
			return ret;
		}
		ret = mtk_padding_set_bypass(warpa_fe->dev_padding2,
			warpa_fe->cmdq_handles,
			warpa_fe->padding_val_2 > 0 ? 0 : 1);
		if (ret) {
			dev_err(warpa_fe->dev_padding2,
				"padding2 set bypass fail!\n");
			return ret;
		}
	}

	return ret;
}

int mtk_warpa_fe_config_camera_sync(struct mtk_warpa_fe *warpa_fe, int delay,
				int ps)
{
	u32 vsync_cycle = 0x10, delay_cycle = 100/*, irq_status = 0*/;
	/*u32 irq_cb_last_time = 1;*/ /* second */
	u32 n = 0, m = 0;
	int ret = 0;
	int dsi_fps = 0;
	int cd;
	int timeout = 0;
	struct cmdq_pkt *pkt;
	u32 mutex_td_event =
		warpa_fe->cmdq_events[MTK_WARPA_FE_CMDQ_EVENT_MUTEX_TD_EVENT0];

	if (delay > 0)
		delay_cycle = delay;

	if (ps)
		timeout = 10000;

	dsi_fps = mtk_dispsys_get_current_fps();
	while (!dsi_fps && timeout--) {
		/* wait for 5~10 sec for dp stable & frame tracking lock */
		usleep_range(500, 1000);
		dsi_fps = mtk_dispsys_get_current_fps();
	}
	/* handle dsi dram in 59.94 89.91 119.88 case */
	if (dsi_fps % 10 == 9)
		dsi_fps += 1;

	if (dsi_fps == 0) {
		dev_warn(warpa_fe->dev, "DSI not ready, set default value\n");
		dsi_fps = 60;
	}

	n = warpa_fe->sensor_fps;
	m = dsi_fps;
	cd = gcd(n, m);
	n /= cd;
	m /= cd;
	LOG(CV_LOG_INFO, "set camera sync n:%d, m:%d\n", n, m);
	LOG(CV_LOG_INFO, "set sensor fps:%d, dsi fps:%d\n",
			 warpa_fe->sensor_fps, dsi_fps);

#if 0
	mtk_mmsys_cfg_camera_sync_register_cb(warpa_fe->dev_mmsys_core_top,
					      mtk_cv_irq_cb, irq_status,
					      warpa_fe);
	pr_debug("camera vsync irq cb enabled!\n");
#endif
	/* because gce4 can't receive dsi vsync */
	/* use mutex timer event to cover it */
	/* set src = dsi vsync, ref = single sof (never come)*/
	/* and minimal timeout*/
	cmdq_pkt_create(&pkt);
	mtk_mutex_select_timer_sof(warpa_fe->mutex,
				MUTEX_COMMON_SOF_DSI0,
				MUTEX_COMMON_SOF_SINGLE, &pkt);
	mtk_mutex_set_timer_us(warpa_fe->mutex, 100000, 10, &pkt);
	mtk_mutex_timer_enable_ex(warpa_fe->mutex, false,
		MUTEX_TO_EVENT_REF_OVERFLOW, &pkt);
	cmdq_pkt_wfe(pkt, mutex_td_event);
	mtk_cv_enable(warpa_fe->dev_mmsys_core_top, MMSYSCFG_CAMERA_SYNC_SIDE01,
		      vsync_cycle, delay_cycle, (u8)n, (u8)m, &pkt);
	mtk_cv_enable(warpa_fe->dev_mmsys_core_top, MMSYSCFG_CAMERA_SYNC_SIDE23,
		      vsync_cycle, delay_cycle, (u8)n, (u8)m, &pkt);
	cmdq_pkt_flush(warpa_fe->cmdq_client_camera_sync, pkt);
	cmdq_pkt_destroy(pkt);

#if 0
	msleep_interruptible(irq_cb_last_time * 1000);
	mtk_mmsys_cfg_camera_sync_register_cb(warpa_fe->dev_mmsys_core_top,
					      NULL, 0, warpa_fe);
	pr_debug("camera vsync irq cb disabled!\n");
#endif
	return ret;
}

int mtk_warpa_fe_module_config(struct mtk_warpa_fe *warpa_fe)
{
	int ret = 0;

	ret = mtk_warpa_fe_config_warpa(warpa_fe);
	if (ret) {
		dev_err(warpa_fe->dev, "config warpa fail");
		return ret;
	}
	ret = mtk_warpa_fe_config_rbfc(warpa_fe);
	if (ret) {
		dev_err(warpa_fe->dev, "config rbfc fail");
		return ret;
	}
	ret = mtk_warpa_fe_config_wdma(warpa_fe);
	if (ret) {
		dev_err(warpa_fe->dev, "config wdma fail");
		return ret;
	}
	ret = mtk_warpa_fe_config_fe(warpa_fe);
	if (ret) {
		dev_err(warpa_fe->dev, "config fe fail");
		return ret;
	}
	ret = mtk_warpa_fe_config_rsz(warpa_fe);
	if (ret) {
		dev_err(warpa_fe->dev, "config rsz fail");
		return ret;
	}
	ret = mtk_warpa_fe_config_padding(warpa_fe);
	if (ret) {
		dev_err(warpa_fe->dev, "config padding fail");
		return ret;
	}

	return ret;
}
EXPORT_SYMBOL(mtk_warpa_fe_module_config);

int mtk_warpa_fe_module_set_buf(struct mtk_warpa_fe *warpa_fe)
{
	int ret = 0;
	int i = 0;
	int warpa_idx, point_idx, desc_idx, wdma0_idx, wdma1_idx, wdma2_idx;
	dma_addr_t addr, wpe_addr;
	int share_buf_flag;
	int full_flag;

	share_buf_flag = (warpa_fe->sysram_layout == VT_SYSRAM_LAYOUT_CINE);
	full_flag = (warpa_fe->sysram_layout == VT_SYSRAM_LAYOUT_FULL);

	if (share_buf_flag)
		wpe_addr = 0x0;
	else
		wpe_addr = WARPA_IN_SRAM_ADDR;

	LOG(CV_LOG_DBG, "warpa_fe set_buf, mask = %d\n",
		warpa_fe->warpafemask);
	LOG(CV_LOG_DBG, "warpa_fe_module_set_buf:%d\n", warpa_fe->in_buf_cnt);

	warpa_idx = MTK_WARPA_FE_BUF_TYPE_WARPA_IMG_BUF_0;

	if (warpa_fe->warpafemask & WARPA_FE_WPE_INPUT_FROM_SYSRAM)
		addr = wpe_addr;
	else if ((warpa_fe->warpafemask & WARPA_FE_HALF_BUFFER_MASK) ||
		 (warpa_fe->warpafemask & WARPA_FE_LINE_BUFFER_MASK))
		addr = 0x0;
	else
		addr = warpa_fe->buf[warpa_idx]->dma_addr;

	LOG(CV_LOG_DBG,
		"set buf MTK_WARPA_FE_BUF_TYPE_WARPA_IMG_BUF_0 cnt:%d @0x%llx\n",
		warpa_fe->in_buf_cnt, addr);
	ret = mtk_warpa_set_buf_in_no0(warpa_fe->dev_warpa, NULL,
		addr, warpa_fe->buf[warpa_idx]->pitch);
	if (ret) {
		dev_err(warpa_fe->dev,
			"MTK_WARPA_FE_BUF_TYPE_WARPA_IMG_BUF_0 fail!! %d\n",
			ret);
		return ret;
	}

	if (warpa_fe->warpafemask & WARPA_FE_WPE_INPUT_FROM_SYSRAM)
		addr = wpe_addr + roundup(warpa_fe->warpa_in_w,
					  warpa_fe->warpa_align);
	else if ((warpa_fe->warpafemask & WARPA_FE_HALF_BUFFER_MASK) ||
		 (warpa_fe->warpafemask & WARPA_FE_LINE_BUFFER_MASK))
		addr = roundup(warpa_fe->warpa_in_w, warpa_fe->warpa_align);
	else
		addr = warpa_fe->buf[warpa_idx]->dma_addr +
			roundup(warpa_fe->warpa_in_w, warpa_fe->warpa_align);

	LOG(CV_LOG_DBG,
		"set buf MTK_WARPA_FE_BUF_TYPE_WARPA_IMG_BUF_1 cnt:%d @0x%llx\n",
		warpa_fe->in_buf_cnt, addr);
	ret = mtk_warpa_set_buf_in_no1(warpa_fe->dev_warpa, NULL,
		addr, warpa_fe->buf[warpa_idx]->pitch);
	if (ret) {
		dev_err(warpa_fe->dev,
			"MTK_WARPA_FE_BUF_TYPE_WARPA_IMG_BUF_1 fail!! %d\n",
			ret);
		return ret;
	}

	if (warpa_fe->warpafemask & WARPA_FE_WPE_INPUT_FROM_SYSRAM)
		addr = wpe_addr +
			roundup(warpa_fe->warpa_in_w, warpa_fe->warpa_align) *
				2;
	else if ((warpa_fe->warpafemask & WARPA_FE_HALF_BUFFER_MASK) ||
		 (warpa_fe->warpafemask & WARPA_FE_LINE_BUFFER_MASK))
		addr = roundup(warpa_fe->warpa_in_w, warpa_fe->warpa_align) * 2;
	else
		addr = warpa_fe->buf[warpa_idx]->dma_addr +
			roundup(warpa_fe->warpa_in_w, warpa_fe->warpa_align) *
			2;

	LOG(CV_LOG_DBG,
		"set buf MTK_WARPA_FE_BUF_TYPE_WARPA_IMG_BUF_2 cnt:%d @0x%llx\n",
		warpa_fe->in_buf_cnt, addr);
	ret = mtk_warpa_set_buf_in_no2(warpa_fe->dev_warpa, NULL,
		addr, warpa_fe->buf[warpa_idx]->pitch);
	if (ret) {
		dev_err(warpa_fe->dev,
			"MTK_WARPA_FE_BUF_TYPE_WARPA_IMG_BUF_2 fail!! %d\n",
			ret);
		return ret;
	}

	if (warpa_fe->warpafemask & WARPA_FE_WPE_INPUT_FROM_SYSRAM)
		addr = wpe_addr +
			roundup(warpa_fe->warpa_in_w, warpa_fe->warpa_align) *
				3;
	else if ((warpa_fe->warpafemask & WARPA_FE_HALF_BUFFER_MASK) ||
		 (warpa_fe->warpafemask & WARPA_FE_LINE_BUFFER_MASK))
		addr = roundup(warpa_fe->warpa_in_w, warpa_fe->warpa_align) * 3;
	else
		addr = warpa_fe->buf[warpa_idx]->dma_addr +
			roundup(warpa_fe->warpa_in_w, warpa_fe->warpa_align) *
				3;

	LOG(CV_LOG_DBG,
		"set buf MTK_WARPA_FE_BUF_TYPE_WARPA_IMG_BUF_3 cnt:%d @0x%llx\n",
		warpa_fe->in_buf_cnt, addr);
	ret = mtk_warpa_set_buf_in_no3(warpa_fe->dev_warpa, NULL,
		addr, warpa_fe->buf[warpa_idx]->pitch);
	if (ret) {
		dev_err(warpa_fe->dev,
			"MTK_WARPA_FE_BUF_TYPE_WARPA_IMG_BUF_3 fail!! %d\n",
			ret);
		return ret;
	}

	LOG(CV_LOG_DBG, "set buf MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_0 @0x%llx\n",
		warpa_fe->buf[MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_0]->dma_addr);
	ret = mtk_warpa_set_buf_map_no0(warpa_fe->dev_warpa, NULL,
		warpa_fe->buf[MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_0]->dma_addr,
		warpa_fe->buf[MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_0]->pitch);
	if (ret) {
		dev_err(warpa_fe->dev,
			"MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_0 fail!! %d\n", ret);
		return ret;
	}

	LOG(CV_LOG_DBG, "set buf MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_1 @0x%llx\n",
		warpa_fe->buf[MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_1]->dma_addr);
	ret = mtk_warpa_set_buf_map_no1(warpa_fe->dev_warpa, NULL,
		warpa_fe->buf[MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_1]->dma_addr,
		warpa_fe->buf[MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_1]->pitch);
	if (ret) {
		dev_err(warpa_fe->dev,
			"MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_1 fail!! %d\n", ret);
		return ret;
	}

	LOG(CV_LOG_DBG, "set buf MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_2 @0x%llx\n",
		warpa_fe->buf[MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_2]->dma_addr);
	ret = mtk_warpa_set_buf_map_no2(warpa_fe->dev_warpa, NULL,
		warpa_fe->buf[MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_2]->dma_addr,
		warpa_fe->buf[MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_2]->pitch);
	if (ret) {
		dev_err(warpa_fe->dev,
			"MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_2 fail!! %d\n", ret);
		return ret;
	}

	LOG(CV_LOG_DBG, "set buf MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_3 @0x%llx\n",
		warpa_fe->buf[MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_3]->dma_addr);
	ret = mtk_warpa_set_buf_map_no3(warpa_fe->dev_warpa, NULL,
		warpa_fe->buf[MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_3]->dma_addr,
		warpa_fe->buf[MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_3]->pitch);
	if (ret) {
		dev_err(warpa_fe->dev,
			"MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_3 fail!! %d\n", ret);
		return ret;
	}

	LOG(CV_LOG_DBG,
		"set buf MTK_WARPA_FE_BUF_TYPE_WARPA_DBG_OUT_IMG @0x%llx\n",
		warpa_fe->buf[MTK_WARPA_FE_BUF_TYPE_WARPA_DBG_OUT_IMG]->
		dma_addr);
	ret = mtk_warpa_set_buf_out_img(warpa_fe->dev_warpa, NULL,
		warpa_fe->buf[MTK_WARPA_FE_BUF_TYPE_WARPA_DBG_OUT_IMG]->
		dma_addr,
		warpa_fe->buf[MTK_WARPA_FE_BUF_TYPE_WARPA_DBG_OUT_IMG]->pitch);
	if (ret) {
		dev_err(warpa_fe->dev,
			"MTK_WARPA_FE_BUF_TYPE_WARPA_DBG_OUT_IMG fail!! %d\n",
			ret);
		return ret;
	}

	LOG(CV_LOG_DBG,
		"set buf MTK_WARPA_FE_BUF_TYPE_WARPA_DBG_OUT_VLD @0x%llx\n",
		warpa_fe->buf[MTK_WARPA_FE_BUF_TYPE_WARPA_DBG_OUT_VLD]->
		dma_addr);

	LOG(CV_LOG_DBG,
		"set buf MTK_WARPA_FE_BUF_TYPE_WARPA_DBG_OUT_VLD pitch @%d\n",
		warpa_fe->buf[MTK_WARPA_FE_BUF_TYPE_WARPA_DBG_OUT_VLD]->pitch);

	ret = mtk_warpa_set_buf_out_vld(warpa_fe->dev_warpa, NULL,
		warpa_fe->buf[MTK_WARPA_FE_BUF_TYPE_WARPA_DBG_OUT_VLD]->
		dma_addr,
		warpa_fe->buf[MTK_WARPA_FE_BUF_TYPE_WARPA_DBG_OUT_VLD]->pitch);
	if (ret) {
		dev_err(warpa_fe->dev,
			"MTK_WARPA_FE_BUF_TYPE_WARPA_DBG_OUT_VLD fail!! %d\n",
			ret);
		return ret;
	}

	for (i = 0; i < 4; i++) {
		point_idx = MTK_WARPA_FE_BUF_TYPE_FE_POINT_0_R0 +
			warpa_fe->out_buf_cnt * 8 + i;
		if (warpa_fe->warpafemask & WARPA_FE_FE_OUTPUT_TO_SYSRAM) {
			switch (i) {
			case 0:
				addr = FEO_0_P_OUT_SRAM_ADDR;
			break;
			case 1:
				addr = FEO_1_P_OUT_SRAM_ADDR;
			break;
			case 2:
				addr = FEO_2_P_OUT_SRAM_ADDR;
			break;
			case 3:
				addr = FEO_3_P_OUT_SRAM_ADDR;
			break;
			default:
				break;
			}
		} else
			addr = warpa_fe->buf[point_idx]->dma_addr;
		LOG(CV_LOG_DBG,
			"set buf MTK_WARPA_FE_BUF_TYPE_FE_L_POINT_%d_%d @0x%llx\n",
			warpa_fe->out_buf_cnt, i, addr);
		ret = mtk_fe_set_multi_out_buf(warpa_fe->dev_fe, NULL,
			i, MTK_FE_OUT_BUF_POINT, addr,
			warpa_fe->buf[point_idx]->pitch);
		if (ret) {
			dev_err(warpa_fe->dev,
				"MTK_WARPA_FE_BUF_TYPE_FE_POINT_%d_%d fail!! %d\n",
				warpa_fe->out_buf_cnt, i, ret);
			return ret;
		}

		desc_idx = MTK_WARPA_FE_BUF_TYPE_FE_DESC_0_R0 +
			warpa_fe->out_buf_cnt * 8 + i;
		if (warpa_fe->warpafemask & WARPA_FE_FE_OUTPUT_TO_SYSRAM) {
			switch (i) {
			case 0:
				addr = FEO_0_D_OUT_SRAM_ADDR;
			break;
			case 1:
				addr = FEO_1_D_OUT_SRAM_ADDR;
			break;
			case 2:
				addr = FEO_2_D_OUT_SRAM_ADDR;
			break;
			case 3:
				addr = FEO_3_D_OUT_SRAM_ADDR;
			break;
			default:
				break;
			}
		} else
			addr = warpa_fe->buf[desc_idx]->dma_addr;
		LOG(CV_LOG_DBG,
			"set buf MTK_WARPA_FE_BUF_TYPE_FE_L_DESC_%d_%d @0x%llx\n",
			warpa_fe->out_buf_cnt, i, addr);
		ret = mtk_fe_set_multi_out_buf(warpa_fe->dev_fe, NULL,
			i, MTK_FE_OUT_BUF_DESCRIPTOR, addr,
			warpa_fe->buf[desc_idx]->pitch);
		if (ret) {
			dev_err(warpa_fe->dev,
				"MTK_WARPA_FE_BUF_TYPE_FE_DESC_%d_%d fail!! %d\n",
				warpa_fe->out_buf_cnt, i, ret);
			return ret;
		}
	}
	wdma0_idx = MTK_WARPA_FE_BUF_TYPE_WDMA_0_R0 +
		warpa_fe->out_buf_cnt * 3;
	if (warpa_fe->warpafemask & WARPA_FE_WPE_OUTPUT_TO_SYSRAM) {
		if (share_buf_flag && warpa_fe->out_buf_cnt == 0)
			addr = WDMA_0_R0_OUT_SRAM_ADDR_SHARE;
		else if (share_buf_flag && warpa_fe->out_buf_cnt == 1)
			addr = WDMA_0_R1_OUT_SRAM_ADDR_SHARE;
		else if (full_flag)
			addr = WDMA_0_R0_OUT_SRAM_ADDR_FULL;
		else
			addr = WDMA_0_R0_OUT_SRAM_ADDR;
	} else
		addr = warpa_fe->buf[wdma0_idx]->dma_addr;
	LOG(CV_LOG_DBG, "set buf MTK_WARPA_FE_BUF_TYPE_WDMA_%d_0 @0x%llx\n",
		warpa_fe->out_buf_cnt, addr);
	ret = mtk_wdma_set_out_buf(warpa_fe->dev_wdma0, NULL,
		addr, warpa_fe->buf[wdma0_idx]->pitch,
		warpa_fe->buf[wdma0_idx]->format);
	if (ret) {
		dev_err(warpa_fe->dev,
			"MTK_WARPA_FE_BUF_TYPE_WDMA_%d_0 fail!! %d\n",
			warpa_fe->out_buf_cnt, ret);
		return ret;
	}

	wdma1_idx = MTK_WARPA_FE_BUF_TYPE_WDMA_1_R0 +
		warpa_fe->out_buf_cnt * 3;
	if (warpa_fe->warpafemask & WARPA_FE_PADDING_OUTPUT_TO_SYSRAM) {
		if (full_flag)
			addr = WDMA_1_R0_OUT_SRAM_ADDR_FULL;
		else
			addr = WDMA_1_R0_OUT_SRAM_ADDR;
	} else {
		addr = warpa_fe->buf[wdma1_idx]->dma_addr;
	}
	LOG(CV_LOG_DBG, "set buf MTK_WARPA_FE_BUF_TYPE_WDMA_%d_1 @0x%llx\n",
		warpa_fe->out_buf_cnt, addr);
	ret = mtk_wdma_set_out_buf(warpa_fe->dev_wdma1, NULL,
		addr, warpa_fe->buf[wdma1_idx]->pitch,
		warpa_fe->buf[wdma1_idx]->format);
	if (ret) {
		dev_err(warpa_fe->dev,
			"MTK_WARPA_FE_BUF_TYPE_WDMA_%d_1 fail!! %d\n",
			warpa_fe->out_buf_cnt, ret);
		return ret;
	}

	wdma2_idx = MTK_WARPA_FE_BUF_TYPE_WDMA_2_R0 +
		warpa_fe->out_buf_cnt * 3;
	if (warpa_fe->warpafemask & WARPA_FE_PADDING_OUTPUT_TO_SYSRAM) {
		if (full_flag)
			addr = WDMA_2_R0_OUT_SRAM_ADDR_FULL;
		else
			addr = WDMA_2_R0_OUT_SRAM_ADDR;
	} else {
		addr = warpa_fe->buf[wdma2_idx]->dma_addr;
	}
	LOG(CV_LOG_DBG, "set buf MTK_WARPA_FE_BUF_TYPE_WDMA_%d_2 @0x%llx\n",
		warpa_fe->out_buf_cnt, addr);
	ret = mtk_wdma_set_out_buf(warpa_fe->dev_wdma2, NULL,
		addr, warpa_fe->buf[wdma2_idx]->pitch,
		warpa_fe->buf[wdma2_idx]->format);
	if (ret) {
		dev_err(warpa_fe->dev,
			"MTK_WARPA_FE_BUF_TYPE_WDMA_%d_2 fail!! %d\n",
			warpa_fe->out_buf_cnt, ret);
		return ret;
	}

	return ret;
}
EXPORT_SYMBOL(mtk_warpa_fe_module_set_buf);

int mtk_warpa_fe_module_start(struct mtk_warpa_fe *warpa_fe)
{
	int ret = 0;

	LOG(CV_LOG_DBG, "warpa_fe_module_start\n");

	ret = mtk_warpa_start(warpa_fe->dev_warpa, NULL);
	if (ret) {
		dev_err(warpa_fe->dev,
			"warpa_start fail!! %d\n", ret);
		return ret;
	}

	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_FE) {
		ret = mtk_fe_start(warpa_fe->dev_fe, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"fe_start fail!! %d\n", ret);
			return ret;
		}
	}

	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_WDMA) {
		ret = mtk_wdma_start(warpa_fe->dev_wdma0, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"wdma0_start fail!! %d\n", ret);
			return ret;
		}
	}

	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_P_SCALE) {
		ret = mtk_padding_start(warpa_fe->dev_padding0,
					warpa_fe->cmdq_handles);
		if (ret) {
			dev_err(warpa_fe->dev,
				"padding0_start fail!! %d\n", ret);
			return ret;
		}
		ret = mtk_resizer_start(warpa_fe->dev_p_rsz0,
				&(warpa_fe->cmdq_handles));
		if (ret) {
			dev_err(warpa_fe->dev,
				"rsz0_start fail!! %d\n", ret);
			return ret;
		}
		ret = mtk_padding_start(warpa_fe->dev_padding1,
				warpa_fe->cmdq_handles);
		if (ret) {
			dev_err(warpa_fe->dev,
				"padding1_start fail!! %d\n", ret);
			return ret;
		}
		ret = mtk_wdma_start(warpa_fe->dev_wdma1, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"wdma1_start fail!! %d\n", ret);
			return ret;
		}
		ret = mtk_resizer_start(warpa_fe->dev_p_rsz1,
				&(warpa_fe->cmdq_handles));
		if (ret) {
			dev_err(warpa_fe->dev,
				"rsz1_start fail!! %d\n", ret);
			return ret;
		}
		ret = mtk_padding_start(warpa_fe->dev_padding2,
				warpa_fe->cmdq_handles);
		if (ret) {
			dev_err(warpa_fe->dev,
				"padding2_start fail!! %d\n", ret);
			return ret;
		}
		ret = mtk_wdma_start(warpa_fe->dev_wdma2, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"wdma2_start fail!! %d\n", ret);
			return ret;
		}
	}

	return 0;
}
EXPORT_SYMBOL(mtk_warpa_fe_module_start);

int mtk_warpa_fe_module_stop(struct mtk_warpa_fe *warpa_fe)
{
	int ret = 0;

	/* when warpa_fe triggered by sof */
	/* need to disable mutex before stop */
	warpa_fe->polling_wait = 0;
	if (warpa_fe->warpafemask & WARPA_FE_RBFC_ENABLE_MASK) {
		ret = mtk_mutex_disable(warpa_fe->mutex, NULL);
		if (ret) {
			dev_err(warpa_fe->dev, "mutex_disable fail!! %d\n",
				ret);
			return ret;
		}
		if (RUN_FE_BY_DELAYED_SOF) {
			ret = mtk_mutex_delay_disable(
				warpa_fe->mutex_warpa_delay, NULL);
			if (ret) {
				dev_err(warpa_fe->dev,
					"mutex_delay_disable fail!! %d\n", ret);
				return ret;
			}
		}
	}

	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_FE) {
		ret = mtk_fe_stop(warpa_fe->dev_fe, NULL);
		if (ret) {
			dev_err(warpa_fe->dev, "fe_stop fail!! %d\n", ret);
			return ret;
		}
	}

	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_WDMA) {
		ret = mtk_wdma_stop(warpa_fe->dev_wdma0, NULL);
		if (ret) {
			dev_err(warpa_fe->dev, "wdma0_stop fail!! %d\n", ret);
			return ret;
		}
	}

	ret = mtk_warpa_stop(warpa_fe->dev_warpa, NULL);
	if (ret) {
		dev_err(warpa_fe->dev, "warpa_stop fail!! %d\n", ret);
		return ret;
	}

	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_P_SCALE) {
		ret = mtk_padding_stop(warpa_fe->dev_padding0, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"padding0_stop fail!! %d\n", ret);
			return ret;
		}
		ret = mtk_resizer_stop(warpa_fe->dev_p_rsz0, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"rsz0_stop fail!! %d\n", ret);
			return ret;
		}
		ret = mtk_padding_stop(warpa_fe->dev_padding1, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"padding1_stop fail!! %d\n", ret);
			return ret;
		}
		ret = mtk_wdma_stop(warpa_fe->dev_wdma1, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"wdma1_stop fail!! %d\n", ret);
			return ret;
		}
		ret = mtk_resizer_stop(warpa_fe->dev_p_rsz1, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"rsz1_stop fail!! %d\n", ret);
			return ret;
		}
		ret = mtk_padding_stop(warpa_fe->dev_padding2, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"padding2_stop fail!! %d\n", ret);
			return ret;
		}
		ret = mtk_wdma_stop(warpa_fe->dev_wdma2, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"wdma2_stop fail!! %d\n", ret);
			return ret;
		}
	}
	if (warpa_fe->warpafemask & WARPA_FE_RBFC_ENABLE_MASK) {
		ret = mtk_rbfc_finish(warpa_fe->dev_rbfc);
		if (ret) {
			dev_err(warpa_fe->dev, "rbfc_finish fail!! %d\n", ret);
			return ret;
		}
	}

	return 0;
}
EXPORT_SYMBOL(mtk_warpa_fe_module_stop);

int mtk_warpa_fe_module_power_on(struct mtk_warpa_fe *warpa_fe)
{
	int ret = 0;

	ret = mtk_mmsys_cmmn_top_power_on(warpa_fe->dev_mmsys_cmmn_top);
	if (ret) {
		dev_err(warpa_fe->dev, "cmmn_top_power_on fail!! %d\n", ret);
		return ret;
	}
	ret = mtk_mmsys_cfg_power_on(warpa_fe->dev_mmsys_core_top);
	if (ret) {
		dev_err(warpa_fe->dev, "core_top_power_on fail!! %d\n", ret);
		return ret;
	}
	ret = mtk_warpa_power_on(warpa_fe->dev_warpa);
	if (ret) {
		dev_err(warpa_fe->dev, "warpa_power_on fail!! %d\n", ret);
		return ret;
	}
	ret = mtk_rbfc_power_on(warpa_fe->dev_rbfc);
	if (ret) {
		dev_err(warpa_fe->dev, "rbfc_power_on fail!! %d\n", ret);
		return ret;
	}

	warpa_fe->mutex = mtk_mutex_get(warpa_fe->dev_mutex,
			0);
	if (IS_ERR(warpa_fe->mutex)) {
		ret = PTR_ERR(warpa_fe->mutex);
		dev_err(warpa_fe->dev, "mutex_get fail!! %d\n", ret);
		return ret;
	}

	ret = mtk_mutex_power_on(warpa_fe->mutex);
	if (ret) {
		dev_err(warpa_fe->dev, "mutex_power_on fail!! %d\n", ret);
		return ret;
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_FE) {
		ret = mtk_fe_power_on(warpa_fe->dev_fe);
		if (ret) {
			dev_err(warpa_fe->dev, "fe_power_on fail!! %d\n", ret);
			return ret;
		}
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_WDMA) {
		ret = mtk_wdma_power_on(warpa_fe->dev_wdma0);
		if (ret) {
			dev_err(warpa_fe->dev, "wdma_power_on 0 fail!! %d\n",
				ret);
			return ret;
		}
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_P_SCALE) {
		ret = mtk_padding_power_on(warpa_fe->dev_padding0);
		if (ret) {
			dev_err(warpa_fe->dev, "padding_power_on 0 fail!! %d\n",
				ret);
			return ret;
		}

		ret = mtk_resizer_power_on(warpa_fe->dev_p_rsz0);
		if (ret) {
			dev_err(warpa_fe->dev, "resizer_power_on 0 fail!! %d\n",
				ret);
			return ret;
		}

		ret = mtk_padding_power_on(warpa_fe->dev_padding1);
		if (ret) {
			dev_err(warpa_fe->dev, "padding_power_on 1 fail!! %d\n",
				ret);
			return ret;
		}

		ret = mtk_wdma_power_on(warpa_fe->dev_wdma1);
		if (ret) {
			dev_err(warpa_fe->dev, "wdma_power_on 1 fail!! %d\n",
				ret);
			return ret;
		}

		ret = mtk_resizer_power_on(warpa_fe->dev_p_rsz1);
		if (ret) {
			dev_err(warpa_fe->dev, "resizer_power_on 1 fail!! %d\n",
				ret);
			return ret;
		}

		ret = mtk_padding_power_on(warpa_fe->dev_padding2);
		if (ret) {
			dev_err(warpa_fe->dev, "padding_power_on 2 fail!! %d\n",
				ret);
			return ret;
		}

		ret = mtk_wdma_power_on(warpa_fe->dev_wdma2);
		if (ret) {
			dev_err(warpa_fe->dev, "wdma_power_on 2 fail!! %d\n",
				ret);
			return ret;
		}
	}

	/* sysram power on */
	switch (warpa_fe->sysram_layout) {
	case VT_SYSRAM_LAYOUT_NONE:
		/* no need to turn sysram power on */
		break;
	case VT_SYSRAM_LAYOUT_FULL:
		ret = mtk_mm_sysram_power_on(warpa_fe->dev_sysram,
					     WDMA_0_R0_OUT_SRAM_ADDR_FULL,
					     CV_END_ADDR -
					     WDMA_0_R0_OUT_SRAM_ADDR_FULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"FULL mode sysram power on failed!\n");
			return ret;
		}
		break;
	case VT_SYSRAM_LAYOUT_VR:
		ret = mtk_mm_sysram_power_on(warpa_fe->dev_sysram,
					     WARPA_IN_SRAM_ADDR,
					     VR_END_ADDR -
					     WARPA_IN_SRAM_ADDR);
		if (ret) {
			dev_err(warpa_fe->dev,
				"VR mode sysram power on failed!\n");
			return ret;
		}
		break;
	case VT_SYSRAM_LAYOUT_CINE:
		ret = mtk_mm_sysram_power_on(warpa_fe->dev_sysram,
					     WARPA_IN_SRAM_ADDR_SHARE,
					     WARPA_IN_SRAM_ADDR_END -
					     WARPA_IN_SRAM_ADDR_SHARE);
		if (ret) {
			dev_err(warpa_fe->dev,
				"Cinematic mode wpe sysram power on failed!\n"
				);
			return ret;
		}

		if (warpa_fe->warpafemask & WARPA_FE_WPE_OUTPUT_TO_SYSRAM) {
			/* P3 case, cv need to go sram path */
			ret = mtk_mm_sysram_power_on(warpa_fe->dev_sysram,
						WDMA_0_R0_OUT_SRAM_ADDR_SHARE,
						CV_END_ADDR -
						WDMA_0_R0_OUT_SRAM_ADDR_SHARE);
			if (ret) {
				dev_err(warpa_fe->dev,
				"Cinematic mode wpe sysram power on failed!\n"
				);
				return ret;
			}
		}
		break;
	default:
		dev_err(warpa_fe->dev,
			"unknown sysram layout: %d\n", warpa_fe->sysram_layout);
		return -EINVAL;
	}
	return 0;
}
EXPORT_SYMBOL(mtk_warpa_fe_module_power_on);


int mtk_warpa_fe_module_power_off(struct mtk_warpa_fe *warpa_fe)
{
	int ret = 0;

	ret = mtk_mmsys_cmmn_top_power_off(warpa_fe->dev_mmsys_cmmn_top);
	if (ret) {
		dev_err(warpa_fe->dev,
			"cmmn_top_power_off fail!! %d\n",
			ret);
		return ret;
	}
	ret = mtk_mmsys_cfg_power_off(warpa_fe->dev_mmsys_core_top);
	if (ret) {
		dev_err(warpa_fe->dev, "core_top_power_off fail!! %d\n", ret);
		return ret;
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_FE) {
		ret = mtk_fe_power_off(warpa_fe->dev_fe);
		if (ret) {
			dev_err(warpa_fe->dev, "fe_power_off fail!! %d\n", ret);
			return ret;
		}
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_WDMA) {
		ret = mtk_wdma_power_off(warpa_fe->dev_wdma0);
		if (ret) {
			dev_err(warpa_fe->dev, "wdma_power_off 0 fail!! %d\n",
				ret);
			return ret;
		}
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_P_SCALE) {
		ret = mtk_padding_power_off(warpa_fe->dev_padding0);
		if (ret) {
			dev_err(warpa_fe->dev,
				"padding_power_off 0 fail!! %d\n", ret);
			return ret;
		}

		ret = mtk_resizer_power_off(warpa_fe->dev_p_rsz0);
		if (ret) {
			dev_err(warpa_fe->dev,
				"resizer_power_off 0 fail!! %d\n", ret);
			return ret;
		}

		ret = mtk_padding_power_off(warpa_fe->dev_padding1);
		if (ret) {
			dev_err(warpa_fe->dev,
				"padding_power_off 1 fail!! %d\n", ret);
			return ret;
		}

		ret = mtk_wdma_power_off(warpa_fe->dev_wdma1);
		if (ret) {
			dev_err(warpa_fe->dev, "wdma_power_off 1 fail!! %d\n",
				ret);
			return ret;
		}

		ret = mtk_resizer_power_off(warpa_fe->dev_p_rsz1);
		if (ret) {
			dev_err(warpa_fe->dev,
				"resizer_power_off 1 fail!! %d\n", ret);
			return ret;
		}

		ret = mtk_padding_power_off(warpa_fe->dev_padding2);
		if (ret) {
			dev_err(warpa_fe->dev,
				"padding_power_off 2 fail!! %d\n", ret);
			return ret;
		}

		ret = mtk_wdma_power_off(warpa_fe->dev_wdma2);
		if (ret) {
			dev_err(warpa_fe->dev,
				"wdma_power_off 2 fail!! %d\n", ret);
			return ret;
		}
	}

	ret = mtk_warpa_power_off(warpa_fe->dev_warpa);
	if (ret) {
		dev_err(warpa_fe->dev, "warpa_power_off fail!! %d\n", ret);
		return ret;
	}
	ret = mtk_rbfc_power_off(warpa_fe->dev_rbfc);
	if (ret) {
		dev_err(warpa_fe->dev, "rbfc_power_off fail!! %d\n", ret);
		return ret;
	}

	ret = mtk_mutex_power_off(warpa_fe->mutex);
	if (ret) {
		dev_err(warpa_fe->dev, "mutex_power_off fail!! %d\n", ret);
		return ret;
	}
	if ((warpa_fe->warpafemask & WARPA_FE_RBFC_ENABLE_MASK) &&
	    RUN_FE_BY_DELAYED_SOF) {
		ret = mtk_mutex_delay_put(warpa_fe->mutex_warpa_delay);
		if (ret) {
			dev_err(warpa_fe->dev, "mutex_delay_put fail!! %d\n",
				ret);
			return ret;
		}
	}
	ret = mtk_mutex_put(warpa_fe->mutex);
	if (ret) {
		dev_err(warpa_fe->dev, "mutex_put fail!! %d\n", ret);
		return ret;
	}

	/* sysram power off */
	switch (warpa_fe->sysram_layout) {
	case VT_SYSRAM_LAYOUT_NONE:
		/* no need to turn sysram power on */
		break;
	case VT_SYSRAM_LAYOUT_FULL:
		ret = mtk_mm_sysram_power_off(warpa_fe->dev_sysram,
					     WDMA_0_R0_OUT_SRAM_ADDR_FULL,
					     CV_END_ADDR -
					     WDMA_0_R0_OUT_SRAM_ADDR_FULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"FULL mode sysram power off failed!\n");
			return ret;
		}
		break;
	case VT_SYSRAM_LAYOUT_VR:
		ret = mtk_mm_sysram_power_off(warpa_fe->dev_sysram,
					     WARPA_IN_SRAM_ADDR,
					     VR_END_ADDR -
					     WARPA_IN_SRAM_ADDR);
		if (ret) {
			dev_err(warpa_fe->dev,
				"VR mode sysram power off failed!\n");
			return ret;
		}
		break;
	case VT_SYSRAM_LAYOUT_CINE:
		ret = mtk_mm_sysram_power_off(warpa_fe->dev_sysram,
					     WARPA_IN_SRAM_ADDR_SHARE,
					     WARPA_IN_SRAM_ADDR_END -
					     WARPA_IN_SRAM_ADDR_SHARE);
		if (ret) {
			dev_err(warpa_fe->dev,
				"Cinematic mode wpe sysram power off failed!\n"
				);
			return ret;
		}

		if (warpa_fe->warpafemask & WARPA_FE_WPE_OUTPUT_TO_SYSRAM) {
			/* P3 case, cv need to go sram path */
			ret = mtk_mm_sysram_power_off(warpa_fe->dev_sysram,
						WDMA_0_R0_OUT_SRAM_ADDR_SHARE,
						CV_END_ADDR -
						WDMA_0_R0_OUT_SRAM_ADDR_SHARE);
			if (ret) {
				dev_err(warpa_fe->dev,
				"Cinematic mode wpe sysram power off failed!\n"
				);
				return ret;
			}
		}
		break;
	default:
		dev_err(warpa_fe->dev,
			"unknown sysram layout: %d\n", warpa_fe->sysram_layout);
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(mtk_warpa_fe_module_power_off);

int mtk_warpa_fe_path_connect(struct mtk_warpa_fe *warpa_fe)
{
	int ret = 0;

	warpa_fe->polling_wait = 1;
	warpa_fe->flow_done = 0;
	warpa_fe->warpa_fe_flush_wait = 0;
	warpa_fe->be_flush_wait = 0;
	warpa_fe->p1_sof_flush_wait = 0;
	warpa_fe->p2_sof_flush_wait = 0;
	warpa_fe->wdma0_cnt = 0;
	warpa_fe->wdma1_cnt = 0;
	warpa_fe->wdma2_cnt = 0;
	*warpa_fe->p1_done_cnt_va = 0;
	*warpa_fe->P1_et_va = 0;
	*warpa_fe->P1_et_max_va = 0;
	*warpa_fe->BE_VPU_delay_va = 0;
	*warpa_fe->BE_VPU_delay_max_va = 0;
	*warpa_fe->BE_VPU_et_va = 0;
	*warpa_fe->BE_VPU_et_max_va = 0;
	*warpa_fe->P1_VPU_et_va = 0;
	*warpa_fe->P1_VPU_et_max_va = 0;
	*warpa_fe->be_vpu_done_cnt_va = 0;
	*warpa_fe->p1_sof_cnt_va = 0;
	*warpa_fe->p2_sof_cnt_va = 0;
	*warpa_fe->p2_done_cnt_va = 0;

	warpa_fe->cmdq_client = cmdq_mbox_create(warpa_fe->dev, 0);
	if (IS_ERR(warpa_fe->cmdq_client)) {
		dev_err(warpa_fe->dev, "fail to create mailbox cmdq_client\n");
		return PTR_ERR(warpa_fe->cmdq_client);
	}
	warpa_fe->cmdq_client_be = cmdq_mbox_create(warpa_fe->dev, 1);
	if (IS_ERR(warpa_fe->cmdq_client_be)) {
		dev_err(warpa_fe->dev,
			"fail to create mailbox cmdq_client_be\n");
		return PTR_ERR(warpa_fe->cmdq_client_be);
	}
	warpa_fe->cmdq_client_p1_sof = cmdq_mbox_create(warpa_fe->dev, 2);
	if (IS_ERR(warpa_fe->cmdq_client_p1_sof)) {
		dev_err(warpa_fe->dev,
			"fail to create mailbox cmdq_client_p1_sof\n");
		return PTR_ERR(warpa_fe->cmdq_client_p1_sof);
	}
	warpa_fe->cmdq_client_p2_sof = cmdq_mbox_create(warpa_fe->dev, 3);
	if (IS_ERR(warpa_fe->cmdq_client_p2_sof)) {
		dev_err(warpa_fe->dev,
			"fail to create mailbox cmdq_client_p2_sof\n");
		return PTR_ERR(warpa_fe->cmdq_client_p2_sof);
	}
	warpa_fe->cmdq_client_camera_sync = cmdq_mbox_create(warpa_fe->dev, 4);
	if (IS_ERR(warpa_fe->cmdq_client_camera_sync)) {
		dev_err(warpa_fe->dev,
			"fail to create mailbox cmdq_client_camera_sync\n");
		return PTR_ERR(warpa_fe->cmdq_client_camera_sync);
	}
	schedule_work(&warpa_fe->trigger_work);
	schedule_work(&warpa_fe->trigger_work_p1_sof);
	schedule_work(&warpa_fe->trigger_work_p2_sof);
	schedule_work(&warpa_fe->trigger_work_be);

	if ((warpa_fe->warpafemask & WARPA_FE_FROM_SENSOR_MASK)
		&& RUN_FE_BY_DELAYED_SOF) {
		LOG(CV_LOG_DBG, "warpa_fe select delayed side isp sof!\n");
		warpa_fe->mutex_warpa_delay =
			mtk_mutex_delay_get(warpa_fe->dev_mutex, 0);
		if (IS_ERR(warpa_fe->mutex_warpa_delay)) {
			ret = PTR_ERR(warpa_fe->mutex_warpa_delay);
			dev_err(warpa_fe->dev, "mutex_delay_get fail!! %d\n",
				ret);
			return ret;
		}
		ret = mtk_mutex_set_delay_us(warpa_fe->mutex_warpa_delay,
					     200000,
					     NULL);
		if (ret) {
			dev_err(warpa_fe->dev, "mutex_set_delay_us fail!! %d\n",
				ret);
			return ret;
		}

		ret = mtk_mutex_select_delay_sof(warpa_fe->mutex_warpa_delay,
						 MUTEX_COMMON_SOF_IMG_SIDE0_0,
						 NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"mutex_select_delay_sof fail!! %d\n", ret);
			return ret;
		}
		ret = mtk_mutex_select_sof(warpa_fe->mutex,
					   MUTEX_COMMON_SOF_SYNC_DELAY0,
					   NULL, true);
		if (ret) {
			dev_err(warpa_fe->dev,
				"mutex_select_sof fail!! %d\n",
				ret);
			return ret;
		}

		ret = mtk_mutex_delay_enable(warpa_fe->mutex_warpa_delay, NULL);
		if (ret) {
			dev_err(warpa_fe->dev, "mutex_delay_enable fail!! %d\n",
				ret);
			return ret;
		}
		ret = mtk_mutex_enable(warpa_fe->mutex, NULL);
		if (ret) {
			dev_err(warpa_fe->dev, "mutex_enable fail!! %d\n", ret);
			return ret;
		}
	} else if ((warpa_fe->warpafemask & WARPA_FE_FROM_SENSOR_MASK) &&
		!(warpa_fe->warpafemask & WARPA_FE_KICK_AFTER_P2_DONE)) {
		LOG(CV_LOG_DBG, "warpa_fe select wen w_ov_th sof!\n");
		ret = mtk_mutex_select_sof(warpa_fe->mutex,
			MUTEX_COMMON_SOF_RBFC_SIDE0_0_WPEA, NULL, false);
#if 0
		ret = mtk_mutex_select_sof_ex(warpa_fe->mutex,
			MUTEX_COMMON_SOF_RBFC_SIDE0_0_WPEA,
			NULL, MUTEX_VSYNC_RISING, MUTEX_VSYNC_FALLING);
#endif
		if (ret) {
			dev_err(warpa_fe->dev, "mutex_select_sof fail!! %d\n",
				ret);
			return ret;
		}
	} else {
		LOG(CV_LOG_DBG, "warpa_fe select single sof!\n");
		ret = mtk_mutex_select_sof(warpa_fe->mutex,
					   MUTEX_COMMON_SOF_SINGLE,
					   NULL, false);
		if (ret) {
			dev_err(warpa_fe->dev, "mutex_select_sof fail!! %d\n",
				ret);
			return ret;
		}
	}

	LOG(CV_LOG_DBG, "warpa fe path conn\n");
	LOG(CV_LOG_DBG, "warpafemask:%d\n", warpa_fe->warpafemask);

	ret = mtk_mutex_add_comp(warpa_fe->mutex,
				 MUTEX_COMPONENT_RBFC_WPEA1, NULL);
	if (ret) {
		dev_err(warpa_fe->dev, "mutex_add_comp rbfc fail!! %d\n",
			ret);
		return ret;
	}

	ret = mtk_mutex_add_comp(warpa_fe->mutex, MUTEX_COMPONENT_WPEA1, NULL);
	if (ret) {
		dev_err(warpa_fe->dev, "mutex_add_comp wpea fail!! %d\n",
			ret);
		return ret;
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_FE) {
		ret = mtk_mutex_add_comp(warpa_fe->mutex,
					 MUTEX_COMPONENT_FE, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"mutex_add_comp fe fail!! %d\n",
				ret);
			return ret;
		}
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_WDMA) {
		ret = mtk_mutex_add_comp(warpa_fe->mutex,
					 MUTEX_COMPONENT_WDMA1, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"mutex_add_comp wdma 0 fail!! %d\n",
				ret);
			return ret;
		}
	}

	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_P_SCALE) {
		ret = mtk_mutex_add_comp(warpa_fe->mutex,
				MUTEX_COMPONENT_PADDING_0, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"mutex_add_comp padding 0 fail!! %d\n",
				ret);
			return ret;
		}

		ret = mtk_mutex_add_comp(warpa_fe->mutex,
				MUTEX_COMPONENT_MMCOMMON_RSZ0, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"mutex_add_comp rsz 0 fail!! %d\n",
				ret);
			return ret;
		}

		ret = mtk_mutex_add_comp(warpa_fe->mutex,
				MUTEX_COMPONENT_PADDING_1, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"mutex_add_comp padding 1 fail!! %d\n",
				ret);
			return ret;
		}

		ret = mtk_mutex_add_comp(warpa_fe->mutex,
				MUTEX_COMPONENT_WDMA2, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"mutex_add_comp wdma 2 fail!! %d\n",
				ret);
			return ret;
		}

		ret = mtk_mutex_add_comp(warpa_fe->mutex,
				MUTEX_COMPONENT_MMCOMMON_RSZ1, NULL);
		if (ret)
			dev_err(warpa_fe->dev,
				"mutex_add_comp rsz 1 fail!! %d\n",
				ret);

		ret = mtk_mutex_add_comp(warpa_fe->mutex,
				MUTEX_COMPONENT_PADDING_2, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"mutex_add_comp padding 2 fail!! %d\n",
				ret);
			return ret;
		}

		ret = mtk_mutex_add_comp(warpa_fe->mutex,
				MUTEX_COMPONENT_WDMA3, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"mutex_add_comp wdma 3 fail!! %d\n",
				ret);
			return ret;
		}
	}

	if (warpa_fe->warpafemask & WARPA_FE_WPE_OUTPUT_TO_SYSRAM)
		ret = mtk_mmsys_cmmn_top_sel_mem(warpa_fe->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_WDMA_0, MTK_MMSYS_CMMN_SYSRAM);
	else
		ret = mtk_mmsys_cmmn_top_sel_mem(warpa_fe->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_WDMA_0, MTK_MMSYS_CMMN_DRAM);
	if (ret) {
		dev_err(warpa_fe->dev,
			"cmmn_top_sel_mem wdma0 fail!! %d\n",
			ret);
		return ret;
	}

	if (warpa_fe->warpafemask & WARPA_FE_FE_OUTPUT_TO_SYSRAM)
		ret = mtk_mmsys_cmmn_top_sel_mem(warpa_fe->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_FE, MTK_MMSYS_CMMN_SYSRAM);
	else
		ret = mtk_mmsys_cmmn_top_sel_mem(warpa_fe->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_FE, MTK_MMSYS_CMMN_DRAM);
	if (ret) {
		dev_err(warpa_fe->dev,
			"cmmn_top_sel_mem fe fail!! %d\n",
			ret);
		return ret;
	}

	if (warpa_fe->warpafemask & WARPA_FE_PADDING_OUTPUT_TO_SYSRAM) {
		ret = mtk_mmsys_cmmn_top_sel_mem(warpa_fe->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_WDMA_1, MTK_MMSYS_CMMN_SYSRAM);
		if (ret) {
			dev_err(warpa_fe->dev,
				"cmmn_top_sel_mem wdma1 fail!! %d\n",
				ret);
			return ret;
		}
		ret = mtk_mmsys_cmmn_top_sel_mem(warpa_fe->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_WDMA_2, MTK_MMSYS_CMMN_SYSRAM);
		if (ret) {
			dev_err(warpa_fe->dev,
				"cmmn_top_sel_mem wdma2 fail!! %d\n",
				ret);
			return ret;
		}
	} else {
		ret = mtk_mmsys_cmmn_top_sel_mem(warpa_fe->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_WDMA_1, MTK_MMSYS_CMMN_DRAM);
		if (ret) {
			dev_err(warpa_fe->dev,
				"cmmn_top_sel_mem wdma1 fail!! %d\n",
				ret);
			return ret;
		}
		ret = mtk_mmsys_cmmn_top_sel_mem(warpa_fe->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_WDMA_2, MTK_MMSYS_CMMN_DRAM);
		if (ret) {
			dev_err(warpa_fe->dev,
				"cmmn_top_sel_mem wdma2 fail!! %d\n",
				ret);
			return ret;
		}
	}

	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_FE) {
		ret = mtk_mmsys_cmmn_top_conn(
			warpa_fe->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_MOD_WPEA,
			MTK_MMSYS_CMMN_MOD_FE);
		if (ret) {
			dev_err(warpa_fe->dev,
				"cmmn_top_conn wpea fe fail!! %d\n",
				ret);
			return ret;
		}
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_WDMA) {
		ret = mtk_mmsys_cmmn_top_conn(
			warpa_fe->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_MOD_WPEA,
			MTK_MMSYS_CMMN_MOD_WDMA);
		if (ret) {
			dev_err(warpa_fe->dev,
				"cmmn_top_conn wpea wdma0 fail!! %d\n",
				ret);
			return ret;
		}
	}

	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_P_SCALE) {
		ret = mtk_mmsys_cmmn_top_conn(
			warpa_fe->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_MOD_WPEA,
			MTK_MMSYS_CMMN_MOD_PADDING_0);
		if (ret) {
			dev_err(warpa_fe->dev,
				"cmmn_top_conn wpea padding_0 fail!! %d\n",
				ret);
			return ret;
		}

		ret = mtk_mmsys_cmmn_top_conn(
			warpa_fe->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_MOD_RSZ_0,
			MTK_MMSYS_CMMN_MOD_PADDING_1);
		if (ret) {
			dev_err(warpa_fe->dev,
				"cmmn_top_conn RESIZER_0 padding_1 fail!! %d\n",
				ret);
			return ret;
		}

		ret = mtk_mmsys_cmmn_top_conn(
			warpa_fe->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_MOD_RSZ_0,
			MTK_MMSYS_CMMN_MOD_PADDING_2);
		if (ret) {
			dev_err(warpa_fe->dev,
				"cmmn_top_conn RESIZER_0 padding_2 fail!! %d\n",
				ret);
		}
	}
	return 0;
}
EXPORT_SYMBOL(mtk_warpa_fe_path_connect);

int mtk_warpa_fe_path_disconnect(struct mtk_warpa_fe *warpa_fe)
{
	int ret = 0;
	int i = 0;

	LOG(CV_LOG_DBG, "warpa fe path disconn\n");
	warpa_fe->polling_wait = 0;

	/* wait for work queue into sync flush*/
	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (warpa_fe->p1_sof_flush_wait)
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait p1_sof_flush_wait timeout!\n");
	ret = cmdq_mbox_destroy(warpa_fe->cmdq_client_p1_sof);
	if (ret) {
		dev_err(warpa_fe->dev, "fail to destroy mailbox cmdq_client_p1_sof\n");
		return PTR_ERR(warpa_fe->cmdq_client_p1_sof);
	}

	/* wait for work queue into sync flush*/
	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (warpa_fe->p2_sof_flush_wait)
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait p2_sof_flush_wait timeout!\n");
	ret = cmdq_mbox_destroy(warpa_fe->cmdq_client_p2_sof);
	if (ret) {
		dev_err(warpa_fe->dev, "fail to destroy mailbox cmdq_client_p2_sof\n");
		return PTR_ERR(warpa_fe->cmdq_client_p2_sof);
	}

	if (warpa_fe->warpafemask & WARPA_FE_KICK_AFTER_P2_DONE) {
		/* wait for work queue into sync flush*/
		for (i = 0; i < FLOW_TIMEOUT; i++) {
			if (warpa_fe->warpa_fe_flush_wait)
				break;
			usleep_range(500, 1000);
		}
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait warpa_fe_flush_wait timeout!\n");
	ret = cmdq_mbox_destroy(warpa_fe->cmdq_client);
	if (ret) {
		dev_err(warpa_fe->dev, "fail to destroy mailbox cmdq_client\n");
		return PTR_ERR(warpa_fe->cmdq_client);
	}

	/* wait for work queue into sync flush*/
	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (warpa_fe->be_flush_wait)
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait be_flush_wait timeout!\n");
	ret = cmdq_mbox_destroy(warpa_fe->cmdq_client_be);
	if (ret) {
		dev_err(warpa_fe->dev, "fail to destroy mailbox cmdq_client_be\n");
		return PTR_ERR(warpa_fe->cmdq_client_be);
	}

	ret = cmdq_mbox_destroy(warpa_fe->cmdq_client_camera_sync);
	if (ret) {
		dev_err(warpa_fe->dev, "fail to destroy mailbox cmdq_client_camera_sync\n");
		return PTR_ERR(warpa_fe->cmdq_client_camera_sync);
	}

	/* wait for work queue stop done */
	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (!work_busy(&warpa_fe->trigger_work))
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait warap_fe trigger work stop timeout!\n");

	/* wait for work queue stop done */
	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (!work_busy(&warpa_fe->trigger_work_be))
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait be trigger work stop timeout!\n");

	/* wait for work queue stop done */
	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (!work_busy(&warpa_fe->trigger_work_p2_sof))
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait p2_sof trigger work stop timeout!\n");

	/* wait for work queue stop done */
	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (!work_busy(&warpa_fe->trigger_work_p1_sof))
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT)
		pr_err("wait p1_sof trigger work stop timeout!\n");

	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_FE) {
		ret = mtk_mmsys_cmmn_top_disconn(
			warpa_fe->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_MOD_WPEA,
			MTK_MMSYS_CMMN_MOD_FE);
		if (ret) {
			dev_err(warpa_fe->dev,
				"cmmn_top_disconn wpea fe fail!! %d\n", ret);
			return ret;
		}
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_WDMA) {
		ret = mtk_mmsys_cmmn_top_disconn(
			warpa_fe->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_MOD_WPEA,
			MTK_MMSYS_CMMN_MOD_WDMA);
		if (ret) {
			dev_err(warpa_fe->dev,
				"cmmn_top_disconn wpea wdma0 fail!! %d\n", ret);
			return ret;
		}
	}

	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_P_SCALE) {
		ret = mtk_mmsys_cmmn_top_disconn(
			warpa_fe->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_MOD_WPEA,
			MTK_MMSYS_CMMN_MOD_PADDING_0);
		if (ret) {
			dev_err(warpa_fe->dev,
				"cmmn_top_disconn wpea padding_0 fail!! %d\n",
				ret);
			return ret;
		}

		ret = mtk_mmsys_cmmn_top_disconn(
			warpa_fe->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_MOD_RSZ_0,
			MTK_MMSYS_CMMN_MOD_PADDING_1);
		if (ret) {
			dev_err(warpa_fe->dev,
				"cmmn_top_disconn RESIZER_0 padding_1 fail!! %d\n",
				ret);
			return ret;
		}

		ret = mtk_mmsys_cmmn_top_disconn(
			warpa_fe->dev_mmsys_cmmn_top,
			MTK_MMSYS_CMMN_MOD_RSZ_0,
			MTK_MMSYS_CMMN_MOD_PADDING_2);
		if (ret) {
			dev_err(warpa_fe->dev,
				"cmmn_top_disconn RESIZER_0 padding_2 fail!! %d\n",
				ret);
			return ret;
		}
	}
	ret = mtk_mutex_remove_comp(warpa_fe->mutex,
			MUTEX_COMPONENT_WPEA1, NULL);
	if (ret) {
		dev_err(warpa_fe->dev, "mutex_remove_comp wpea fail!! %d\n",
			ret);
		return ret;
	}

	ret = mtk_mutex_remove_comp(warpa_fe->mutex,
			MUTEX_COMPONENT_RBFC_WPEA1, NULL);
	if (ret) {
		dev_err(warpa_fe->dev, "mutex_remove_comp rbfc fail!! %d\n",
			ret);
		return ret;
	}

	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_FE) {
		ret = mtk_mutex_remove_comp(warpa_fe->mutex,
				MUTEX_COMPONENT_FE, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"mutex_remove_comp fe fail!! %d\n", ret);
			return ret;
		}
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_WDMA) {
		ret = mtk_mutex_remove_comp(warpa_fe->mutex,
			MUTEX_COMPONENT_WDMA1, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"mutex_remove_comp wdma 1 fail!! %d\n", ret);
			return ret;
		}
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_P_SCALE) {
		ret = mtk_mutex_remove_comp(warpa_fe->mutex,
				MUTEX_COMPONENT_PADDING_0, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"mutex_remove_comp padding 0 fail!! %d\n", ret);
			return ret;
		}

		ret = mtk_mutex_remove_comp(warpa_fe->mutex,
				MUTEX_COMPONENT_MMCOMMON_RSZ0, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"mutex_remove_comp rsz 0 fail!! %d\n", ret);
			return ret;
		}

		ret = mtk_mutex_remove_comp(warpa_fe->mutex,
				MUTEX_COMPONENT_PADDING_1, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"mutex_remove_comp padding 1 fail!! %d\n", ret);
			return ret;
		}

		ret = mtk_mutex_remove_comp(warpa_fe->mutex,
				MUTEX_COMPONENT_WDMA2, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"mutex_remove_comp wdma 2 fail!! %d\n", ret);
			return ret;
		}

		ret = mtk_mutex_remove_comp(warpa_fe->mutex,
				MUTEX_COMPONENT_MMCOMMON_RSZ1, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"mutex_remove_comp rsz 1 fail!! %d\n", ret);
			return ret;
		}

		ret = mtk_mutex_remove_comp(warpa_fe->mutex,
				MUTEX_COMPONENT_PADDING_2, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"mutex_remove_comp padding 2 fail!! %d\n", ret);
			return ret;
		}

		ret = mtk_mutex_remove_comp(warpa_fe->mutex,
				MUTEX_COMPONENT_WDMA3, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"mutex_remove_comp wdma 3 fail!! %d\n", ret);
			return ret;
		}
	}
	return 0;
}
EXPORT_SYMBOL(mtk_warpa_fe_path_disconnect);

static int flush_check(struct mtk_warpa_fe *warpa_fe,
	struct cmdq_client *client, struct cmdq_pkt *pkt,
	enum mtk_warpa_fe_flush_mode mode)
{
	switch (mode) {
	case WARPA_FE_WORK:
		cmdq_pkt_flush_async(client, pkt, warpa_fe_work_cb, warpa_fe);
		warpa_fe->warpa_fe_flush_wait = 1;
		wait_for_completion_interruptible(
			&warpa_fe->warpa_fe_work_comp.cmplt);
		warpa_fe->warpa_fe_flush_wait = 0;
		if (!warpa_fe->polling_wait) {
			LOG(CV_LOG_DBG, "warpa_fe quit flow!\n");
			return -1;
		}
		break;
	case P1_SOF_WORK:
		cmdq_pkt_flush_async(client, pkt, p1_sof_work_cb, warpa_fe);
		warpa_fe->p1_sof_flush_wait = 1;
		wait_for_completion_interruptible(
			&warpa_fe->p1_sof_work_comp.cmplt);
		warpa_fe->p1_sof_flush_wait = 0;
		if (!warpa_fe->polling_wait) {
			dev_dbg(warpa_fe->dev, "p1_sof quit flow!\n");
			return -1;
		}
		break;
	case P2_SOF_WORK:
		cmdq_pkt_flush_async(client, pkt, p2_sof_work_cb, warpa_fe);
		warpa_fe->p2_sof_flush_wait = 1;
		wait_for_completion_interruptible(
			&warpa_fe->p2_sof_work_comp.cmplt);
		warpa_fe->p2_sof_flush_wait = 0;
		if (!warpa_fe->polling_wait) {
			LOG(CV_LOG_DBG, "p2_sof quit flow!\n");
			return -1;
		}
		break;
	case BE_WORK:
		cmdq_pkt_flush_async(client, pkt, be_work_cb, warpa_fe);
		warpa_fe->be_flush_wait = 1;
		wait_for_completion_interruptible(
			&warpa_fe->be_work_comp.cmplt);
		warpa_fe->be_flush_wait = 0;
		if (!warpa_fe->polling_wait) {
			LOG(CV_LOG_DBG, "be quit flow!\n");
			return -1;
		}
		break;
	case BE_WORK_NO_INT:
		cmdq_pkt_flush_async(client, pkt, be_work_cb, warpa_fe);
		wait_for_completion_interruptible(
			&warpa_fe->be_work_comp.cmplt);
		if (!warpa_fe->polling_wait) {
			warpa_fe->be_flush_wait = 1;
			LOG(CV_LOG_DBG, "be quit flow!\n");
			return -1;
		}
		break;
	default:
		dev_err(warpa_fe->dev, "error flush mode %d", mode);
		return -EINVAL;
	}
	return 0;
}


static void mtk_warpa_fe_trigger_work_be(struct work_struct *work)
{
	struct mtk_warpa_fe *warpa_fe =
			container_of(work, struct mtk_warpa_fe,
			trigger_work_be);
#ifdef CONFIG_MACH_MT3612_A0
	u32 vpu_done_event =
		warpa_fe->cmdq_events[MTK_WARPA_FE_CMDQ_EVENT_VPU_CORE1_DONE];
#else
	u32 vpu_done_event =
		warpa_fe->cmdq_events[MTK_WARPA_FE_CMDQ_EVENT_VPU_CORE0_DONE];
#endif
	/*s32 j_offset = 0;*/
	struct device_node *node = NULL;
	struct platform_device *device = NULL;
	int ret = 0;
	int r_idx;
#if 0
	struct timeval prev_frame;
	struct timeval cur_frame;
#endif
	void __iomem *side0_be0_reg, *side0_be1_reg;
	void __iomem *side1_be0_reg, *side1_be1_reg;
	u8 subsys = SUBSYS_102DXXXX;

	node = of_find_compatible_node(NULL, NULL,
		"mediatek,mt3612-ivp");
	device = of_find_device_by_node(node);

	/* enable be debug reg */
	side0_be0_reg = ioremap(0x18072000, 0x1000);
	side0_be1_reg = ioremap(0x18073000, 0x1000);
	side1_be0_reg = ioremap(0x23072000, 0x1000);
	side1_be1_reg = ioremap(0x23073000, 0x1000);
	writel(0x10701, side0_be0_reg + 0x10);
	writel(0x10701, side0_be1_reg + 0x10);
	writel(0x10701, side1_be0_reg + 0x10);
	writel(0x10701, side1_be1_reg + 0x10);

	init_completion(&warpa_fe->be_work_comp.cmplt);

	cmdq_pkt_create(&warpa_fe->cmdq_pkt_be);
	cmdq_pkt_assign_command(warpa_fe->cmdq_pkt_be, GCE_BE_VPU_sof_ts, 0);
	cmdq_pkt_assign_command(warpa_fe->cmdq_pkt_be, GCE_BE_VPU_done_ts, 0);
	cmdq_pkt_assign_command(warpa_fe->cmdq_pkt_be, GCE_P1_VPU_max_exec, 0);
	cmdq_pkt_assign_command(warpa_fe->cmdq_pkt_be, GCE_BE_VPU_max_exec, 0);
	cmdq_pkt_assign_command(warpa_fe->cmdq_pkt_be, GCE_BE_VPU_max_delay, 0);
	cmdq_pkt_assign_command(warpa_fe->cmdq_pkt_be, GCE_P1_VPU_done_cnt, 0);
	ret = flush_check(warpa_fe, warpa_fe->cmdq_client_be,
			  warpa_fe->cmdq_pkt_be, BE_WORK);
	while (warpa_fe->polling_wait) {
		LOG(CV_LOG_DBG, "warpa_fe_trigger_work_be: start wait:%d\n",
			GCE_SW_TOKEN_KICK_BE_VPU);
		r_idx = *warpa_fe->be_vpu_done_cnt_va %
			warpa_fe->be_ring_buf_cnt;
		reinit_completion(&warpa_fe->be_work_comp.cmplt);
		cmdq_pkt_multiple_reset(&warpa_fe->cmdq_pkt_be, 1);
		cmdq_pkt_clear_event(warpa_fe->cmdq_pkt_be,
					GCE_SW_TOKEN_KICK_BE_VPU);
		cmdq_pkt_clear_event(warpa_fe->cmdq_pkt_be, vpu_done_event);
		cmdq_pkt_wfe(warpa_fe->cmdq_pkt_be, GCE_SW_TOKEN_KICK_BE_VPU);
		cmdq_pkt_read(warpa_fe->cmdq_pkt_be, subsys, 0x3020,
				GCE_BE_VPU_KICK_ts);
		ret = flush_check(warpa_fe, warpa_fe->cmdq_client_be,
				  warpa_fe->cmdq_pkt_be, BE_WORK);
		if (ret)
			break;
		LOG(CV_LOG_DBG, "pass1 done!\n");

		/* side0 */
		warpa_fe->cam_debug_test.side[0].be[0].label_dbg =
			readl(side0_be0_reg + 0x3c);
		warpa_fe->cam_debug_test.side[0].be[0].data_dbg =
			readl(side0_be0_reg + 0x40);
		warpa_fe->cam_debug_test.side[0].be[0].merge_table_dbg =
			readl(side0_be0_reg + 0x44);

		warpa_fe->cam_debug_test.side[0].be[1].label_dbg =
			readl(side0_be1_reg + 0x3c);
		warpa_fe->cam_debug_test.side[0].be[1].data_dbg =
			readl(side0_be1_reg + 0x40);
		warpa_fe->cam_debug_test.side[0].be[1].merge_table_dbg =
			readl(side0_be1_reg + 0x44);

		/* side1 */
		warpa_fe->cam_debug_test.side[1].be[0].label_dbg =
			readl(side1_be0_reg + 0x3c);
		warpa_fe->cam_debug_test.side[1].be[0].data_dbg =
			readl(side1_be0_reg + 0x40);
		warpa_fe->cam_debug_test.side[1].be[0].merge_table_dbg =
			readl(side1_be0_reg + 0x44);

		warpa_fe->cam_debug_test.side[1].be[1].label_dbg =
			readl(side1_be1_reg + 0x3c);
		warpa_fe->cam_debug_test.side[1].be[1].data_dbg =
			readl(side1_be1_reg + 0x40);
		warpa_fe->cam_debug_test.side[1].be[1].merge_table_dbg =
			readl(side1_be1_reg + 0x44);

		LOG(CV_LOG_DBG, "be req id:%d, handle:%ld, r_idx:%d\n",
				warpa_fe->process_be[r_idx].coreid,
				(long)warpa_fe->process_be[r_idx].handle,
				r_idx);
		reinit_completion(&warpa_fe->be_work_comp.cmplt);
		cmdq_pkt_multiple_reset(&warpa_fe->cmdq_pkt_be, 1);
		cmdq_pkt_read(warpa_fe->cmdq_pkt_be, subsys, 0x3020,
				GCE_BE_VPU_sof_ts);
		/* SPR2 = be vpu sof - P1 done */
		append_exec_time_gce_cmd(warpa_fe->cmdq_pkt_be,
				GCE_BE_VPU_KICK_ts, GCE_BE_VPU_sof_ts, GCE_SPR2,
				warpa_fe->BE_VPU_delay_pa);
		/* if SPR2 > GCE_BE_VPU_max_delay, update it */
		append_max_exec_time_gce_cmd(warpa_fe->cmdq_pkt_be, GCE_SPR2,
			GCE_BE_VPU_max_delay, warpa_fe->BE_VPU_delay_max_pa);
		mtk_send_ivp_request(device, warpa_fe->process_be[r_idx].handle,
				     warpa_fe->process_be[r_idx].coreid,
				     warpa_fe->cmdq_pkt_be);
#if 0
		j_offset = (0 - cmdq_get_cmd_buf_size(warpa_fe->cmdq_pkt_be));
		cmdq_pkt_jump(warpa_fe->cmdq_pkt_be, j_offset);
#endif
		cmdq_pkt_wfe(warpa_fe->cmdq_pkt_be, vpu_done_event);
		mtk_clr_ivp_request(device, warpa_fe->process_be[r_idx].handle,
				    warpa_fe->process_be[r_idx].coreid,
				    warpa_fe->cmdq_pkt_be);
		cmdq_pkt_read(warpa_fe->cmdq_pkt_be, subsys, 0x3020,
				GCE_BE_VPU_done_ts);
		/* SPR2 = BE vpu done - BE vpu sof */
		append_exec_time_gce_cmd(warpa_fe->cmdq_pkt_be,
					 GCE_BE_VPU_sof_ts,
					 GCE_BE_VPU_done_ts,
					 GCE_SPR2,
					 warpa_fe->BE_VPU_et_pa);
		/* if SPR2 > max exec time GCE_BE_VPU_max_exec, update it */
		append_max_exec_time_gce_cmd(warpa_fe->cmdq_pkt_be, GCE_SPR2,
			GCE_BE_VPU_max_exec, warpa_fe->BE_VPU_et_max_pa);
		/* SPR2 = BE vpu done - P1 sof */
		append_exec_time_gce_cmd(warpa_fe->cmdq_pkt_be,
					 GCE_P1_sof_ts,
					 GCE_BE_VPU_done_ts,
					 GCE_SPR2,
					 warpa_fe->P1_VPU_et_pa);
		/* if SPR2 > max exec time GCE_P1_VPU_max_exec, update it */
		append_max_exec_time_gce_cmd(warpa_fe->cmdq_pkt_be, GCE_SPR2,
			GCE_P1_VPU_max_exec, warpa_fe->P1_VPU_et_max_pa);
		/* p1 done cnt +1 */
		append_inc_frame_cnt_cmd(warpa_fe->cmdq_pkt_be,
			GCE_P1_VPU_done_cnt, warpa_fe->be_vpu_done_cnt_pa);
		ret = flush_check(warpa_fe, warpa_fe->cmdq_client_be,
				  warpa_fe->cmdq_pkt_be, BE_WORK_NO_INT);
		if (ret)
			break;

		LOG(CV_LOG_DBG, "be vpu compute done!\n");
#if 0
		prev_frame = cur_frame;
		do_gettimeofday(&cur_frame);
		if (warpa_fe->flow_done)
			print_done_interval("be vpu", prev_frame, cur_frame);
#endif
		if (warpa_fe->polling_wait &&
		    ((warpa_fe->warpafemask & WARPA_FE_DUMP_BE_FILE_MASK) ||
		      warpa_fe->one_shot_dump))
			dump_ir_buffers(warpa_fe,
					*warpa_fe->be_vpu_done_cnt_va - 1);
		warpa_fe->flow_done = 1;
	}
	ret = cmdq_pkt_destroy(warpa_fe->cmdq_pkt_be);
	if (ret)
		dev_err(warpa_fe->dev, "cmdq_pkt_destroy be fail!\n");
	warpa_fe->cmdq_pkt_be = NULL;
	LOG(CV_LOG_DBG, "mtk_warpa_fe_trigger_work_be: exit\n");
}

static void mtk_warpa_fe_trigger_work_p1_sof(struct work_struct *work)
{
	struct mtk_warpa_fe *warpa_fe =
			container_of(work, struct mtk_warpa_fe,
				     trigger_work_p1_sof);
	u32 p1_sof_event =
		warpa_fe->cmdq_events[MTK_WARPA_FE_CMDQ_EVENT_ISP_P1_SOF];
	u32 p1_done_event =
		warpa_fe->cmdq_events[MTK_WARPA_FE_CMDQ_EVENT_ISP_P1_DONE];
	s32 j_offset = 0;
	int ret = 0;
	u8 subsys = SUBSYS_102DXXXX;
	u8 gce4_subsys = SUBSYS_1031XXXX;

	init_completion(&warpa_fe->p1_sof_work_comp.cmplt);
	dev_dbg(warpa_fe->dev,
		"warpa_fe_trigger_work: start wait:%d\n", p1_sof_event);
	cmdq_pkt_create(&warpa_fe->cmdq_pkt_p1_sof);
	cmdq_pkt_assign_command(warpa_fe->cmdq_pkt_p1_sof, GCE_P1_IR_sof_cnt,
				0);
	cmdq_pkt_assign_command(warpa_fe->cmdq_pkt_p1_sof, GCE_P1_sof_ts, 0);
	cmdq_pkt_assign_command(warpa_fe->cmdq_pkt_p1_sof, GCE_P1_IR_done_cnt,
				0);
	cmdq_pkt_assign_command(warpa_fe->cmdq_pkt_p1_sof, GCE_P1_done_ts, 0);
	cmdq_pkt_assign_command(warpa_fe->cmdq_pkt_p1_sof, GCE_P1_max_exec, 0);
	cmdq_pkt_clear_event(warpa_fe->cmdq_pkt_p1_sof, p1_sof_event);
	cmdq_pkt_clear_event(warpa_fe->cmdq_pkt_p1_sof, p1_done_event);
	cmdq_pkt_wfe(warpa_fe->cmdq_pkt_p1_sof, p1_sof_event);
	cmdq_pkt_set_token(warpa_fe->cmdq_pkt_p1_sof, GCE_SW_TOKEN_SIDE_P1_SOF,
			gce4_subsys);
	/* get p1 sof done start ts -> CPR */
	cmdq_pkt_read(warpa_fe->cmdq_pkt_p1_sof, subsys, 0x3020, GCE_P1_sof_ts);
	append_inc_frame_cnt_cmd(warpa_fe->cmdq_pkt_p1_sof, GCE_P1_IR_sof_cnt,
			warpa_fe->p1_sof_cnt_pa);
	cmdq_pkt_wfe(warpa_fe->cmdq_pkt_p1_sof, p1_done_event);
	cmdq_pkt_set_token(warpa_fe->cmdq_pkt_p1_sof, GCE_SW_TOKEN_KICK_BE_VPU,
			gce4_subsys);
	/* get P1 done ts -> CPR */
	cmdq_pkt_read(warpa_fe->cmdq_pkt_p1_sof, subsys, 0x3020,
			GCE_P1_done_ts);
	/* SPR2 = P1 done - P1 sof */
	append_exec_time_gce_cmd(warpa_fe->cmdq_pkt_p1_sof, GCE_P1_sof_ts,
		GCE_P1_done_ts, GCE_SPR2, warpa_fe->P1_et_pa);
	/* if SPR2 > max exec time GCE_P1_max_exec, update it */
	append_max_exec_time_gce_cmd(warpa_fe->cmdq_pkt_p1_sof, GCE_SPR2,
		GCE_P1_max_exec, warpa_fe->P1_et_max_pa);
	/* p1 done cnt +1 */
	append_inc_frame_cnt_cmd(warpa_fe->cmdq_pkt_p1_sof,
		GCE_P1_IR_done_cnt, warpa_fe->p1_done_cnt_pa);
	j_offset = (5 * CMDQ_INST_SIZE -
		cmdq_get_cmd_buf_size(warpa_fe->cmdq_pkt_p1_sof));
	cmdq_pkt_jump(warpa_fe->cmdq_pkt_p1_sof, j_offset);
	ret = flush_check(warpa_fe, warpa_fe->cmdq_client_p1_sof,
			  warpa_fe->cmdq_pkt_p1_sof, P1_SOF_WORK);
	dev_dbg(warpa_fe->dev, "warpa_fe_trigger_work_p1_sof: exit\n");
}


static void mtk_warpa_fe_trigger_work_p2_sof(struct work_struct *work)
{
	struct mtk_warpa_fe *warpa_fe =
			container_of(work, struct mtk_warpa_fe,
				     trigger_work_p2_sof);
	u32 p2_sof_event =
		warpa_fe->cmdq_events[MTK_WARPA_FE_CMDQ_EVENT_ISP_P2_SOF];
	u32 p2_done_event =
		warpa_fe->cmdq_events[MTK_WARPA_FE_CMDQ_EVENT_ISP_P2_DONE];
	s32 j_offset = 0;
	int ret = 0;
	u8 gce4_subsys = SUBSYS_1031XXXX;

	init_completion(&warpa_fe->p2_sof_work_comp.cmplt);
	LOG(CV_LOG_DBG, "warpa_fe_trigger_work: start wait:%d\n", p2_sof_event);
	cmdq_pkt_create(&warpa_fe->cmdq_pkt_p2_sof);
	cmdq_pkt_assign_command(warpa_fe->cmdq_pkt_p2_sof, GCE_P2_sof_cnt, 0);
	cmdq_pkt_assign_command(warpa_fe->cmdq_pkt_p2_sof, GCE_P2_done_cnt, 0);
	cmdq_pkt_clear_event(warpa_fe->cmdq_pkt_p2_sof, p2_sof_event);
	cmdq_pkt_wfe(warpa_fe->cmdq_pkt_p2_sof, p2_sof_event);
	cmdq_pkt_set_token(warpa_fe->cmdq_pkt_p2_sof, GCE_SW_TOKEN_SIDE_P2_SOF,
			gce4_subsys);
	append_inc_frame_cnt_cmd(warpa_fe->cmdq_pkt_p2_sof, GCE_P2_sof_cnt,
			warpa_fe->p2_sof_cnt_pa);
	cmdq_pkt_wfe(warpa_fe->cmdq_pkt_p2_sof, p2_done_event);
	cmdq_pkt_set_token(warpa_fe->cmdq_pkt_p2_sof, GCE_SW_TOKEN_SIDE_P2_DONE,
			gce4_subsys);
	if (warpa_fe->warpafemask & WARPA_FE_KICK_AFTER_P2_DONE)
		cmdq_pkt_set_token(warpa_fe->cmdq_pkt_p2_sof,
				GCE_SW_TOKEN_KICK_WPE_AFTER_P2_DONE,
				gce4_subsys);
	append_inc_frame_cnt_cmd(warpa_fe->cmdq_pkt_p2_sof, GCE_P2_done_cnt,
			warpa_fe->p2_done_cnt_pa);
	j_offset = (2 * CMDQ_INST_SIZE -
		cmdq_get_cmd_buf_size(warpa_fe->cmdq_pkt_p2_sof));
	cmdq_pkt_jump(warpa_fe->cmdq_pkt_p2_sof, j_offset);
	ret = flush_check(warpa_fe, warpa_fe->cmdq_client_p2_sof,
			  warpa_fe->cmdq_pkt_p2_sof, P2_SOF_WORK);
	LOG(CV_LOG_DBG, "warpa_fe_trigger_work_p2_sof: exit\n");
}


static void mtk_warpa_fe_trigger_work(struct work_struct *work)
{
	struct mtk_warpa_fe *warpa_fe =
			container_of(work, struct mtk_warpa_fe, trigger_work);
	s32 j_offset = 0;
	int ret = 0;

	init_completion(&warpa_fe->warpa_fe_work_comp.cmplt);
	LOG(CV_LOG_DBG, "warpa_fe_trigger_work: start wait:%d\n",
			GCE_SW_TOKEN_KICK_WPE_AFTER_P2_DONE);
	cmdq_pkt_create(&warpa_fe->cmdq_pkt);
	cmdq_pkt_clear_event(warpa_fe->cmdq_pkt,
		GCE_SW_TOKEN_KICK_WPE_AFTER_P2_DONE);
	cmdq_pkt_wfe(warpa_fe->cmdq_pkt, GCE_SW_TOKEN_KICK_WPE_AFTER_P2_DONE);
	if (warpa_fe->warpafemask & WARPA_FE_KICK_AFTER_P2_DONE)
		mtk_mutex_enable(warpa_fe->mutex, &warpa_fe->cmdq_pkt);
	j_offset = (0 - cmdq_get_cmd_buf_size(warpa_fe->cmdq_pkt));
	cmdq_pkt_jump(warpa_fe->cmdq_pkt, j_offset);
	ret = flush_check(warpa_fe, warpa_fe->cmdq_client,
			  warpa_fe->cmdq_pkt, WARPA_FE_WORK);
	LOG(CV_LOG_DBG, "warpa_fe_trigger_work: exit\n");
}

int mtk_warpa_fe_path_trigger_no_wait(struct mtk_warpa_fe *warpa_fe,
	bool onoff)
{
	int ret = 0;

	if (onoff)
		schedule_work(&warpa_fe->trigger_work);
	else
		ret = mtk_mutex_disable(warpa_fe->mutex, NULL);
	if (ret) {
		dev_err(warpa_fe->dev, "mutex_disable fail!! %d\n", ret);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mtk_warpa_fe_path_trigger_no_wait);

int mtk_warpa_fe_path_trigger(struct mtk_warpa_fe *warpa_fe,
	bool onoff)
{
	bool fe_done;
	int i, ret = 0;

	LOG(CV_LOG_DBG, "warpa_fe_path_trigger!!\n");
	if (warpa_fe->warpafemask & WARPA_FE_KICK_AFTER_P2_DONE) {
		if (onoff) {
			/* wait for work queue into sync flush*/
			for (i = 0; i < FLOW_TIMEOUT; i++) {
				if (warpa_fe->warpa_fe_flush_wait)
					break;
				usleep_range(500, 1000);
			}
			if (i == FLOW_TIMEOUT)
				pr_err("wait warpa_fe_flush_wait timeout!\n");
		}
	} else {
		if (onoff)
			ret = mtk_mutex_enable(warpa_fe->mutex, NULL);
		else
			ret = mtk_mutex_disable(warpa_fe->mutex, NULL);
		if (ret) {
			dev_err(warpa_fe->dev,
				"mutex_enable/disable fail!! %d\n", ret);
			return ret;
		}
		LOG(CV_LOG_DBG, "warpa_fe_path_trigger end\n");

		if (onoff && (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_FE) &&
		!(warpa_fe->warpafemask & WARPA_FE_FROM_SENSOR_MASK)) {
			for (i = 0; i < 100; i++) {
				fe_done = mtk_fe_get_fe_done(warpa_fe->dev_fe);
				if (fe_done)
					break;

				msleep(50);
			}

			if (i == 100)
			dev_err(warpa_fe->dev,
				"warpa_fe_path_trigger: FE timeout\n");
		}
	}

	return 0;
}
EXPORT_SYMBOL(mtk_warpa_fe_path_trigger);

int mtk_warpa_fe_reset(struct mtk_warpa_fe *warpa_fe)
{
	int ret = 0;

	LOG(CV_LOG_DBG, "warpa_fe_reset\n");

	/*mtk_warpa_fe_path_disconnect(warpa_fe);*/

	/* sw reset*/
#if 0
	ret = mtk_warpa_reset(warpa_fe->dev_warpa);
	if (ret)
		dev_err(warpa_fe->dev, "warpa_reset fail !! %d\n", ret);
	ret = mtk_wdma_reset(warpa_fe->dev_wdma0);
	if (ret)
		dev_err(warpa_fe->dev, "wdma_reset 0 fail !! %d\n", ret);
	ret = mtk_wdma_reset(warpa_fe->dev_wdma1);
	if (ret)
		dev_err(warpa_fe->dev, "wdma_reset 1 fail !! %d\n", ret);
	ret = mtk_wdma_reset(warpa_fe->dev_wdma2);
	if (ret)
		dev_err(warpa_fe->dev, "wdma_reset 2 fail !! %d\n", ret);

	mtk_padding_reset(warpa_fe->dev_padding0, NULL);
	mtk_padding_reset(warpa_fe->dev_padding1, NULL);
	mtk_padding_reset(warpa_fe->dev_padding2, NULL);

	ret = mtk_fe_reset(warpa_fe->dev_fe);
	if (ret)
		dev_err(warpa_fe->dev, "fe_reset fail !! %d\n", ret);
#endif
	/* hw reset*/

	ret = mtk_mmsys_cmmn_top_mod_reset(warpa_fe->dev_mmsys_cmmn_top,
					   MDP_WPEA_1);
	if (ret) {
		dev_err(warpa_fe->dev, "MDP_WPEA_1 cmmn top reset fail! %d\n",
			ret);
		return ret;
	}

	ret = mtk_mmsys_cmmn_top_mod_reset(warpa_fe->dev_mmsys_cmmn_top,
					   MDP_FE);
	if (ret) {
		dev_err(warpa_fe->dev, "MDP_FE cmmn top reset fail! %d\n", ret);
		return ret;
	}

	ret = mtk_mmsys_cmmn_top_mod_reset(warpa_fe->dev_mmsys_cmmn_top,
					   DISP_WDMA_1);
	if (ret) {
		dev_err(warpa_fe->dev,
			"DISP_WDMA_1 cmmn top reset fail! %d\n", ret);
		return ret;
	}

	ret = mtk_mmsys_cmmn_top_mod_reset(warpa_fe->dev_mmsys_cmmn_top,
					   DISP_WDMA_2);
	if (ret) {
		dev_err(warpa_fe->dev,
			"DISP_WDMA_2 cmmn top reset fail! %d\n", ret);
		return ret;
	}

	ret = mtk_mmsys_cmmn_top_mod_reset(warpa_fe->dev_mmsys_cmmn_top,
					   DISP_WDMA_3);
	if (ret) {
		dev_err(warpa_fe->dev,
			"DISP_WDMA_3 cmmn top reset fail! %d\n", ret);
		return ret;
	}

	ret = mtk_mmsys_cmmn_top_mod_reset(warpa_fe->dev_mmsys_cmmn_top,
		padding_0);
	if (ret) {
		dev_err(warpa_fe->dev,
			"padding_0 cmmn top reset fail! %d\n", ret);
		return ret;
	}

	ret = mtk_mmsys_cmmn_top_mod_reset(warpa_fe->dev_mmsys_cmmn_top,
		padding_1);
	if (ret) {
		dev_err(warpa_fe->dev,
			"padding_1 cmmn top reset fail! %d\n", ret);
		return ret;
	}

	ret = mtk_mmsys_cmmn_top_mod_reset(warpa_fe->dev_mmsys_cmmn_top,
		padding_2);
	if (ret) {
		dev_err(warpa_fe->dev,
			"padding_2 cmmn top reset fail! %d\n", ret);
		return ret;
	}

	ret = mtk_mmsys_cmmn_top_mod_reset(warpa_fe->dev_mmsys_cmmn_top,
		RESIZER_0);
	if (ret) {
		dev_err(warpa_fe->dev,
			"RESIZER_0 cmmn top reset fail! %d\n", ret);
		return ret;
	}

	ret = mtk_mmsys_cmmn_top_mod_reset(warpa_fe->dev_mmsys_cmmn_top,
		RESIZER_1);
	if (ret) {
		dev_err(warpa_fe->dev,
			"RESIZER_1 cmmn top reset fail! %d\n", ret);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mtk_warpa_fe_reset);

static int mtk_warpa_fe_get_dev(struct device **dev,
				struct mtk_warpa_fe *warpa_fe,
				int idx)
{
	switch (idx) {
	case MTK_WARPA_FE_BUF_TYPE_WARPA_IMG_BUF_0:
	case MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_0:
	case MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_1:
	case MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_2:
	case MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_3:
	case MTK_WARPA_FE_BUF_TYPE_WARPA_DBG_OUT_IMG:
	case MTK_WARPA_FE_BUF_TYPE_WARPA_DBG_OUT_VLD:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R0:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R1:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R2:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R3:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R4:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R5:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R6:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R7:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R8:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R9:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R10:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R11:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R12:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R13:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R14:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R15:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R0:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R1:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R2:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R3:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R4:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R5:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R6:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R7:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R8:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R9:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R10:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R11:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R12:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R13:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R14:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R15:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R0:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R1:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R2:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R3:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R4:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R5:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R6:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R7:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R8:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R9:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R10:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R11:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R12:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R13:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R14:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R15:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R0:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R1:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R2:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R3:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R4:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R5:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R6:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R7:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R8:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R9:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R10:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R11:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R12:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R13:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R14:
	case MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R15:

	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R0:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R1:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R2:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R3:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R4:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R5:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R6:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R7:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R8:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R9:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R10:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R11:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R12:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R13:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R14:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R15:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R0:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R1:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R2:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R3:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R4:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R5:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R6:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R7:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R8:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R9:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R10:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R11:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R12:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R13:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R14:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R15:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R0:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R1:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R2:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R3:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R4:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R5:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R6:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R7:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R8:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R9:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R10:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R11:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R12:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R13:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R14:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R15:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R0:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R1:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R2:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R3:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R4:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R5:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R6:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R7:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R8:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R9:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R10:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R11:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R12:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R13:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R14:
	case MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R15:

	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R0:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R1:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R2:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R3:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R4:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R5:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R6:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R7:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R8:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R9:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R10:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R11:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R12:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R13:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R14:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R15:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R0:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R1:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R2:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R3:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R4:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R5:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R6:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R7:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R8:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R9:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R10:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R11:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R12:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R13:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R14:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R15:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R0:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R1:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R2:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R3:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R4:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R5:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R6:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R7:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R8:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R9:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R10:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R11:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R12:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R13:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R14:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R15:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R0:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R1:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R2:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R3:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R4:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R5:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R6:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R7:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R8:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R9:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R10:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R11:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R12:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R13:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R14:
	case MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R15:

	case MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R0:
	case MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R1:
	case MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R2:
	case MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R3:
	case MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R4:
	case MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R5:
	case MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R6:
	case MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R7:
	case MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R8:
	case MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R9:
	case MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R10:
	case MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R11:
	case MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R12:
	case MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R13:
	case MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R14:
	case MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R15:
		*dev = warpa_fe->dev_warpa;
		break;
	case MTK_WARPA_FE_BUF_TYPE_FE_POINT_0_R0:
	case MTK_WARPA_FE_BUF_TYPE_FE_POINT_1_R0:
	case MTK_WARPA_FE_BUF_TYPE_FE_POINT_2_R0:
	case MTK_WARPA_FE_BUF_TYPE_FE_POINT_3_R0:
	case MTK_WARPA_FE_BUF_TYPE_FE_DESC_0_R0:
	case MTK_WARPA_FE_BUF_TYPE_FE_DESC_1_R0:
	case MTK_WARPA_FE_BUF_TYPE_FE_DESC_2_R0:
	case MTK_WARPA_FE_BUF_TYPE_FE_DESC_3_R0:
	case MTK_WARPA_FE_BUF_TYPE_FE_POINT_0_R1:
	case MTK_WARPA_FE_BUF_TYPE_FE_POINT_1_R1:
	case MTK_WARPA_FE_BUF_TYPE_FE_POINT_2_R1:
	case MTK_WARPA_FE_BUF_TYPE_FE_POINT_3_R1:
	case MTK_WARPA_FE_BUF_TYPE_FE_DESC_0_R1:
	case MTK_WARPA_FE_BUF_TYPE_FE_DESC_1_R1:
	case MTK_WARPA_FE_BUF_TYPE_FE_DESC_2_R1:
	case MTK_WARPA_FE_BUF_TYPE_FE_DESC_3_R1:
		*dev = warpa_fe->dev_fe;
		break;
	case MTK_WARPA_FE_BUF_TYPE_WDMA_0_R0:
	case MTK_WARPA_FE_BUF_TYPE_WDMA_0_R1:
		*dev = warpa_fe->dev_wdma0;
		break;
	case MTK_WARPA_FE_BUF_TYPE_WDMA_1_R0:
	case MTK_WARPA_FE_BUF_TYPE_WDMA_1_R1:
		*dev = warpa_fe->dev_wdma1;
		break;
	case MTK_WARPA_FE_BUF_TYPE_WDMA_2_R0:
	case MTK_WARPA_FE_BUF_TYPE_WDMA_2_R1:
		*dev = warpa_fe->dev_wdma2;
		break;
	default:
		dev_err(warpa_fe->dev, "get_dev: buf type %d error\n", idx);
		return -EINVAL;
	}

	return 0;
}

static int mtk_warpa_fe_ioctl_import_fd_to_handle(struct mtk_warpa_fe *warpa_fe,
	unsigned long arg)
{
	struct mtk_common_buf_handle handle;
	struct device *dev;
	int ret = 0;

	ret = copy_from_user((void *)&handle, (void *)arg, sizeof(handle));
	if (ret) {
		dev_err(warpa_fe->dev, "FD_TO_HANDLE: get params fail, ret=%d\n",
			ret);
		return ret;
	}

	ret = mtk_warpa_fe_get_dev(&dev, warpa_fe, handle.buf_type);
	if (ret) {
		dev_err(warpa_fe->dev, "FD_TO_HANDLE: get dev fail, ret=%d\n",
			ret);
		return ret;
	}
	ret = mtk_common_fd_to_handle_offset(dev,
		handle.fd, &(handle.handle), handle.offset);
	if (ret) {
		dev_err(warpa_fe->dev, "FD_TO_HANDLE: import buf fail, ret=%d\n",
			ret);
		return ret;
	}
	ret = copy_to_user((void *)arg, &handle, sizeof(handle));
	if (ret) {
		dev_err(warpa_fe->dev, "FD_TO_HANDLE: update params fail, ret=%d\n",
			ret);
		return ret;
	}

	return ret;
}

static int mtk_warpa_fe_ioctl_set_ctrl_mask(struct mtk_warpa_fe *warpa_fe,
	unsigned long arg)
{
	struct mtk_warpa_fe_set_mask config_mask;
	int ret = 0;

	ret = copy_from_user((void *)&config_mask, (void *)arg,
			sizeof(config_mask));
	if (ret) {
		dev_err(warpa_fe->dev, "SET_MASK: get params fail, ret=%d\n",
			ret);
		return ret;
	}
	warpa_fe->warpafemask = config_mask.warpafemask;
	warpa_fe->sysram_layout = config_mask.sysram_layout;

	return ret;
}

static int mtk_warpa_fe_ioctl_config_warpa(struct mtk_warpa_fe *warpa_fe,
	unsigned long arg)
{
	struct mtk_warpa_fe_config_warpa config_warpa;
	int ret = 0;

	ret = copy_from_user(&config_warpa, (void *)arg,
			sizeof(config_warpa));
	if (ret) {
		dev_err(warpa_fe->dev, "CONFIG_WARPA: get params fail, ret=%d\n",
			ret);
		return ret;
	}

	warpa_fe->coef_tbl.idx = config_warpa.coef_tbl_idx;
	memcpy(warpa_fe->coef_tbl.user_coef_tbl,
		config_warpa.user_coef_tbl,
		sizeof(warpa_fe->coef_tbl.user_coef_tbl));
	warpa_fe->border_color.enable =
		config_warpa.border_color_enable;
	warpa_fe->border_color.border_color =
		config_warpa.border_color;
	warpa_fe->border_color.unmapped_color =
		config_warpa.unmapped_color;
	warpa_fe->proc_mode = config_warpa.proc_mode;
	warpa_fe->warpa_out_mode = config_warpa.out_mode;
	warpa_fe->reset = config_warpa.reset;
	warpa_fe->warpa_in_w = config_warpa.warpa_in_w;
	warpa_fe->warpa_in_h = config_warpa.warpa_in_h;
	warpa_fe->warpa_out_w = config_warpa.warpa_out_w;
	warpa_fe->warpa_out_h = config_warpa.warpa_out_h;
	warpa_fe->warpa_map_w = config_warpa.warpa_map_w;
	warpa_fe->warpa_map_h = config_warpa.warpa_map_h;
	warpa_fe->warpa_align = config_warpa.warpa_align;

	return ret;
}

static int mtk_warpa_fe_ioctl_config_fe(struct mtk_warpa_fe *warpa_fe,
	unsigned long arg)
{
	struct mtk_warpa_fe_config_fe config_fe;
	int ret = 0;

	ret = copy_from_user(&config_fe, (void *)arg,
			sizeof(config_fe));
	if (ret) {
		dev_err(warpa_fe->dev, "CONFIG_FE: get params fail, ret=%d\n",
			ret);
		return ret;
	}
	warpa_fe->blk_sz = config_fe.blk_sz;
	warpa_fe->fe_w = config_fe.fe_w;
	warpa_fe->fe_h = config_fe.fe_h;
	warpa_fe->fe_merge_mode = config_fe.fe_merge_mode;
	warpa_fe->fe_flat_enable = config_fe.fe_flat_enable;
	warpa_fe->fe_harris_kappa = config_fe.fe_harris_kappa;
	warpa_fe->fe_th_g = config_fe.fe_th_g;
	warpa_fe->fe_th_c = config_fe.fe_th_c;
	warpa_fe->fe_cr_val_sel = config_fe.fe_cr_val_sel;
	warpa_fe->fe_align = config_fe.fe_align;

	return ret;
}

static int mtk_warpa_fe_ioctl_config_wdma(struct mtk_warpa_fe
	*warpa_fe, unsigned long arg)
{
	struct mtk_warpa_fe_config_wdma config_wdma;
	int ret = 0;

	ret = copy_from_user(&config_wdma, (void *)arg,
			sizeof(config_wdma));
	if (ret) {
		dev_err(warpa_fe->dev, "CONFIG_FE: get params fail, ret=%d\n",
			ret);
		return ret;
	}
	warpa_fe->wdma_align = config_wdma.wdma_align;

	return ret;
}

static int mtk_warpa_fe_ioctl_config_rsz(struct mtk_warpa_fe *warpa_fe,
	unsigned long arg)
{
	struct mtk_warpa_fe_config_rsz config_rsz;
	int ret = 0;

	ret = copy_from_user(&config_rsz, (void *)arg,
				sizeof(config_rsz));
	if (ret) {
		dev_err(warpa_fe->dev, "CONFIG_FE: get params fail, ret=%d\n",
			ret);
		return ret;
	}
	warpa_fe->rsz_0_out_w = config_rsz.rsz_0_out_w;
	warpa_fe->rsz_0_out_h = config_rsz.rsz_0_out_h;
	warpa_fe->rsz_1_out_w = config_rsz.rsz_1_out_w;
	warpa_fe->rsz_1_out_h = config_rsz.rsz_1_out_h;

	return ret;
}

static int mtk_warpa_fe_ioctl_config_padding(struct mtk_warpa_fe
	*warpa_fe, unsigned long arg)
{
	struct mtk_warpa_fe_config_padding config_padding;
	int ret = 0;

	ret = copy_from_user(&config_padding, (void *)arg,
			sizeof(config_padding));
	if (ret) {
		dev_err(warpa_fe->dev, "CONFIG_FE: get params fail, ret=%d\n",
			ret);
		return ret;
	}
	warpa_fe->padding_val_0 = config_padding.padding_val_0;
	warpa_fe->padding_val_1 = config_padding.padding_val_1;
	warpa_fe->padding_val_2 = config_padding.padding_val_2;

	return ret;
}

static int mtk_warpa_fe_ioctl_set_buf(struct mtk_warpa_fe *warpa_fe,
	unsigned long arg)
{
	struct mtk_common_set_buf buf;
	struct mtk_common_buf *buf_handle;
	int ret = 0;

	ret = copy_from_user((void *)&buf, (void *)arg, sizeof(buf));
	if (ret) {
		dev_err(warpa_fe->dev, "SET_BUF: get params fail, ret=%d\n",
			ret);
		return ret;
	}
	if (buf.buf_type >= MTK_WARPA_FE_BUF_TYPE_MAX) {
		dev_err(warpa_fe->dev, "MTK_WARPA_FE:SET_BUF: buf type %d error\n",
			buf.buf_type);
		return -EINVAL;
	}

	buf_handle = (struct mtk_common_buf *)buf.handle;
	buf_handle->pitch = buf.pitch;
	buf_handle->format = buf.format;
	LOG(CV_LOG_DBG, "set WARPA_FE buf %d\n", buf.buf_type);
	LOG(CV_LOG_DBG, "buf_handle->kvaddr = %p\n",
		(void *)buf_handle->kvaddr);
	LOG(CV_LOG_DBG, "buf_handle->dma_addr = %pad\n", &buf_handle->dma_addr);
	LOG(CV_LOG_DBG, "buf_handle->pitch:%d\n", buf_handle->pitch);
	LOG(CV_LOG_DBG, "buf_handle->format:%d\n", buf_handle->format);
	warpa_fe->buf[buf.buf_type] = buf_handle;

	return ret;
}

static int mtk_warpa_fe_ioctl_streamon(struct mtk_warpa_fe *warpa_fe)
{
	int ret = 0;

#if POWER_READY
	mtk_warpa_fe_module_power_on(warpa_fe);
	if (warpa_fe->reset) {
		ret = mtk_warpa_fe_reset(warpa_fe);
		if (ret)
			dev_err(warpa_fe->dev, "RESET: fail, ret=%d\n", ret);
	}
#else
	/* call wdma power on to enable iommu */
	/* tmp solution, need to remove after ccf enabled! */
	mtk_wdma_power_on(warpa_fe->dev_wdma0);
	mtk_wdma_power_on(warpa_fe->dev_wdma1);
	mtk_wdma_power_on(warpa_fe->dev_wdma2);
#endif
	LOG(CV_LOG_DBG, "stream on start!\n");
	ret = mtk_warpa_fe_module_config(warpa_fe);
	if (ret) {
		dev_err(warpa_fe->dev, "CONFIG: fail, ret=%d\n", ret);
		return ret;
	}
	ret = mtk_warpa_fe_path_connect(warpa_fe);
	if (ret) {
		dev_err(warpa_fe->dev, "CONNECT: fail, ret=%d\n", ret);
		return ret;
	}
	ret = mtk_warpa_fe_module_start(warpa_fe);
	if (ret) {
		dev_err(warpa_fe->dev, "START: fail, ret=%d\n", ret);
		return ret;
	}

	LOG(CV_LOG_DBG, "stream on done!\n");

	return ret;
}

static int mtk_warpa_fe_ioctl_streamoff(struct mtk_warpa_fe *warpa_fe)
{
	int ret = 0;

	LOG(CV_LOG_DBG, "stream off start!\n");
	ret = mtk_warpa_fe_path_trigger(warpa_fe, false);
	if (ret) {
		dev_err(warpa_fe->dev, "TRIGGER DISABLE: fail, ret=%d\n", ret);
		return ret;
	}
	ret = mtk_warpa_fe_module_stop(warpa_fe);
	if (ret) {
		dev_err(warpa_fe->dev, "STOP: fail, ret=%d\n", ret);
		return ret;
	}
	ret = mtk_warpa_fe_path_disconnect(warpa_fe);
	if (ret) {
		dev_err(warpa_fe->dev, "DISCONNECT: fail, ret=%d\n", ret);
		return ret;
	}
	LOG(CV_LOG_DBG, "stream off done!\n");
#if 0
	if (*warpa_fe->be_vpu_done_cnt_va > 0) {
		dump_ir_buffers(warpa_fe, *warpa_fe->be_vpu_done_cnt_va);
		dev_dbg(warpa_fe->dev, "exit dump ir buffer done!\n");
	}
#endif
#if POWER_READY
	mtk_warpa_fe_module_power_off(warpa_fe);
#endif

	return ret;
}

static int mtk_warpa_fe_ioctl_trigger(struct mtk_warpa_fe *warpa_fe)
{
	int ret = 0;

	ret = mtk_warpa_fe_module_set_buf(warpa_fe);
	if (ret) {
		dev_err(warpa_fe->dev, "SET_BUF: fail, ret=%d\n", ret);
		return ret;
	}
	ret = mtk_warpa_fe_path_trigger(warpa_fe, true);
	if (ret) {
		dev_err(warpa_fe->dev, "TRIGGER: fail, ret=%d\n", ret);
		return ret;
	}

	return 0;
}

static int mtk_warpa_fe_ioctl_reset(struct mtk_warpa_fe *warpa_fe)
{
	int ret = 0;

	ret = mtk_warpa_fe_reset(warpa_fe);
	if (ret) {
		dev_err(warpa_fe->dev, "RESET: fail, ret=%d\n", ret);
		return ret;
	}

	return ret;
}

static int mtk_warpa_fe_ioctl_comp(struct mtk_warpa_fe *warpa_fe,
	unsigned long arg)
{
	struct mtk_warpa_fe_golden_path path;
	int ret = 0;
	int err_flag = 0;

	memset(path.feo0_P_golden_path, 0,
				sizeof(path.feo0_P_golden_path));
	memset(path.feo0_D_golden_path, 0,
		sizeof(path.feo0_D_golden_path));
	memset(path.feo1_P_golden_path, 0,
		sizeof(path.feo1_P_golden_path));
	memset(path.feo1_D_golden_path, 0,
		sizeof(path.feo1_D_golden_path));
	memset(path.feo2_P_golden_path, 0,
		sizeof(path.feo2_P_golden_path));
	memset(path.feo2_D_golden_path, 0,
		sizeof(path.feo2_D_golden_path));
	memset(path.feo3_P_golden_path, 0,
		sizeof(path.feo3_P_golden_path));
	memset(path.feo3_D_golden_path, 0,
		sizeof(path.feo3_D_golden_path));
	path.feoP_size = 0;
	path.feoD_size = 0;
	ret = copy_from_user((void *)&path, (void *)arg, sizeof(path));
	if (ret) {
		dev_err(warpa_fe->dev,
			"MTK_WARPA_FE_IOCTL_COMP: get params fail, ret=%d\n",
			ret);
		return ret;
	}
	if (path.feoP_size > 0x1000 || path.feoD_size > 0x6400 ||
		path.feoP_size < 0x100 || path.feoD_size < 0x640)
		return -EINVAL;

	if (strlen(path.feo0_P_golden_path) > 0 &&
		strlen(path.feo0_D_golden_path) > 0) {
		LOG(CV_LOG_DBG, "FEO compare path 0\n");
		LOG(CV_LOG_DBG, "feo0_p_golden %s\n", path.feo0_P_golden_path);
		LOG(CV_LOG_DBG, "feo0_D_golden %s\n", path.feo0_D_golden_path);
		err_flag = mtk_warpa_fe_compare_feo(warpa_fe,
			path.feo0_P_golden_path,
			path.feo0_D_golden_path,
			MTK_WARPA_FE_BUF_TYPE_FE_POINT_0_R0,
			MTK_WARPA_FE_BUF_TYPE_FE_DESC_0_R0,
			path.feoP_size, path.feoD_size);
		if (err_flag)
			return -EIO;
	}

	if (strlen(path.feo1_P_golden_path) > 0 &&
		strlen(path.feo1_P_golden_path) > 0) {
		err_flag = mtk_warpa_fe_compare_feo(warpa_fe,
			path.feo1_P_golden_path,
			path.feo1_D_golden_path,
			MTK_WARPA_FE_BUF_TYPE_FE_POINT_1_R0,
			MTK_WARPA_FE_BUF_TYPE_FE_DESC_1_R0,
			path.feoP_size, path.feoD_size);
		if (err_flag)
			return -EIO;
	}

	if (strlen(path.feo2_P_golden_path) > 0 &&
		strlen(path.feo2_D_golden_path) > 0) {
		err_flag = mtk_warpa_fe_compare_feo(warpa_fe,
			path.feo2_P_golden_path,
			path.feo2_D_golden_path,
			MTK_WARPA_FE_BUF_TYPE_FE_POINT_2_R0,
			MTK_WARPA_FE_BUF_TYPE_FE_DESC_2_R0,
			path.feoP_size, path.feoD_size);
		if (err_flag)
			return -EIO;
	}

	if (strlen(path.feo3_P_golden_path) > 0 &&
		strlen(path.feo3_D_golden_path) > 0) {
		err_flag = mtk_warpa_fe_compare_feo(warpa_fe,
			path.feo3_P_golden_path,
			path.feo3_D_golden_path,
			MTK_WARPA_FE_BUF_TYPE_FE_POINT_3_R0,
			MTK_WARPA_FE_BUF_TYPE_FE_DESC_3_R0,
			path.feoP_size, path.feoD_size);
		if (err_flag)
			return -EIO;
	}

	return ret;
}

static int mtk_warpa_fe_ioctl_put_handle(struct mtk_warpa_fe *warpa_fe,
	unsigned long arg)
{
	struct mtk_common_put_handle put_handle;
	int ret = 0;

	ret = copy_from_user(&put_handle, (void *)arg,
				sizeof(put_handle));
	if (ret) {
		dev_err(warpa_fe->dev,
			"PUT_HANDLE: get params fail, ret=%d\n", ret);
		return ret;
	}
	ret = mtk_common_put_handle(put_handle.handle);
	if (ret) {
		dev_err(warpa_fe->dev,
			"PUT_HANDLE: put handle fail, ret=%d\n", ret);
		return ret;
	}
	if (put_handle.buf_type < MTK_WARPA_FE_BUF_TYPE_MAX)
		warpa_fe->buf[put_handle.buf_type] = NULL;

	return ret;
}

static int mtk_warpa_fe_ioctl_set_vpu_req(struct mtk_warpa_fe *warpa_fe,
	unsigned long arg)
{
	struct mtk_warpa_fe_vpu_req req;
	int ret = 0;
	int i = 0;

	ret = copy_from_user(&req, (void *)arg,
				sizeof(req));
	if (ret) {
		dev_err(warpa_fe->dev,
			"VPU_REQ: get params fail, ret=%d\n", ret);
		return ret;
	}
	for (i = 0; i < 16; i++) {
		if (req.process_p1_img[i] != NULL) {
			ret = copy_from_user(&warpa_fe->process_be[i],
				(void *)req.process_p1_img[i],
				sizeof(warpa_fe->process_be[i]));
			if (ret) {
				dev_err(warpa_fe->dev,
					"VPU_REQ: get p1_img_%d fail, ret=%d\n",
					i, ret);
				return ret;
			}

			LOG(CV_LOG_DBG, "copy req id:%d, handle:%ld\n",
				warpa_fe->process_be[i].coreid,
				(long)warpa_fe->process_be[i].handle);
		}
	}

	warpa_fe->be_ring_buf_cnt = req.be_ring_buf_cnt;
	warpa_fe->be_en = req.be_en;
	warpa_fe->be_label_img_en = req.be_label_img_en;
	warpa_fe->p1vpu_pair = req.p1vpu_pair;
	warpa_fe->path_mask = req.path_mask;

	return ret;

}

static int mtk_warpa_fe_ioctl_check_flow_done(struct mtk_warpa_fe
	*warpa_fe)
{
	int i = 0;

	/* wait up to timeout * img_num sec */
	for (i = 0; i < FLOW_TIMEOUT; i++) {
		if (warpa_fe->flow_done)
			break;
		msleep(100);
	}

	warpa_fe->flow_done = 0;
	if (i == FLOW_TIMEOUT)
		return -ETIME;
	else
		return 0;

}

static int mtk_warpa_fe_ioctl_get_statistics(struct mtk_warpa_fe
	*warpa_fe, unsigned long arg)
{
	struct mtk_warpa_fe_statistics stat;
	int ret = 0;

	ret = copy_from_user((void *)&stat, (void *)arg, sizeof(stat));
	if (ret) {
		dev_err(warpa_fe->dev, "GET_STAT: get params fail, ret=%d\n",
			ret);
		return ret;
	}
	stat.p2_sof_cnt = *warpa_fe->p2_sof_cnt_va;
	stat.p2_done_cnt = *warpa_fe->p2_done_cnt_va;
	stat.p1_sof_cnt = *warpa_fe->p1_sof_cnt_va;
	stat.p1_done_cnt = *warpa_fe->p1_done_cnt_va;
	stat.be_vpu_cnt = *warpa_fe->be_vpu_done_cnt_va;
	stat.wdma0_cnt = warpa_fe->wdma0_cnt;
	stat.wdma1_cnt = warpa_fe->wdma1_cnt;
	stat.wdma2_cnt = warpa_fe->wdma2_cnt;
	stat.P1_exec_cur = *warpa_fe->P1_et_va;
	stat.P1_exec_max = *warpa_fe->P1_et_max_va;
	stat.BE_VPU_delay_cur = *warpa_fe->BE_VPU_delay_va;
	stat.BE_VPU_delay_max = *warpa_fe->BE_VPU_delay_max_va;
	stat.BE_VPU_exec_cur = *warpa_fe->BE_VPU_et_va;
	stat.BE_VPU_exec_max = *warpa_fe->BE_VPU_et_max_va;
	stat.P1_VPU_exec_cur = *warpa_fe->P1_VPU_et_va;
	stat.P1_VPU_exec_max = *warpa_fe->P1_VPU_et_max_va;
	ret = copy_to_user((void *)arg, &stat, sizeof(stat));
	if (ret) {
		dev_err(warpa_fe->dev, "GET_STAT: update params fail, ret=%d\n",
			ret);
		return ret;
	}

	return ret;
}

static int mtk_warpa_fe_ioctl_set_log_level(unsigned long arg)
{
	int ret = 0;

	if (arg > CV_LOG_DBG)
		return -EINVAL;
	kern_log_level = (int)arg;
	pr_info("set warpa_fe kernel log level %d\n", kern_log_level);

	return ret;
}

static int mtk_warpa_fe_ioctl_debug_test_cam(struct mtk_warpa_fe *warpa_fe,
	unsigned long arg)
{
	struct mtk_warpa_fe_debug_test_cam debug_test_cam;
	void __iomem *side0_camsv0_reg, *side0_camsv1_reg;
	void __iomem *side1_camsv0_reg, *side1_camsv1_reg;
	void __iomem *gaze0_camsv0_reg, *gaze1_camsv0_reg;
	int i, j;
	int ret = 0;

	ret = copy_from_user((void *)&debug_test_cam, (void *)arg,
		sizeof(debug_test_cam));
	if (ret) {
		dev_err(warpa_fe->dev, "GET_TEST_CAM: get params failed, ret=%d\n",
			ret);
		return ret;
	}

	for (i = 0; i < FLOW_TIMEOUT * warpa_fe->img_num; i++) {
		if (*warpa_fe->be_vpu_done_cnt_va > 0)
			break;
		usleep_range(500, 1000);
	}
	if (i == FLOW_TIMEOUT * warpa_fe->img_num) {
		pr_err("wait be frame done timeout\n");
		return -ETIME;
	}
	side0_camsv0_reg = ioremap(0x18030000, 0x1000);
	side0_camsv1_reg = ioremap(0x18031000, 0x1000);
	side1_camsv0_reg = ioremap(0x23030000, 0x1000);
	side1_camsv1_reg = ioremap(0x23031000, 0x1000);
	gaze0_camsv0_reg = ioremap(0x22030000, 0x1000);
	gaze1_camsv0_reg = ioremap(0x17030000, 0x1000);

	/* camsv checksum */
	/* side 0 */
	writel(0x80b, side0_camsv0_reg + 0x48c);
	writel(0x80b, side0_camsv1_reg + 0x48c);
	writel(0x0, side0_camsv0_reg + 0x34);
	warpa_fe->cam_debug_test.side[0].camsv_dbg[0] =
		readl(side0_camsv0_reg);
	writel(0x3, side0_camsv0_reg + 0x34);
	warpa_fe->cam_debug_test.side[0].camsv_dbg[1] =
		readl(side0_camsv0_reg);

	/* side 1 */
	writel(0x80b, side1_camsv0_reg + 0x48c);
	writel(0x80b, side1_camsv1_reg + 0x48c);
	writel(0x0, side1_camsv0_reg + 0x34);
	warpa_fe->cam_debug_test.side[1].camsv_dbg[0] =
		readl(side1_camsv0_reg);
	writel(0x3, side1_camsv0_reg + 0x34);
	warpa_fe->cam_debug_test.side[1].camsv_dbg[1] =
		readl(side1_camsv0_reg);

	/* gaze 0 */
	writel(0x80b, gaze0_camsv0_reg + 0x48c);
	writel(0x0, gaze0_camsv0_reg + 0x34);
	warpa_fe->cam_debug_test.gaze[0].camsv_dbg =
		readl(gaze0_camsv0_reg);

	/* gaze 0 */
	writel(0x80b, gaze1_camsv0_reg + 0x48c);
	writel(0x0, gaze1_camsv0_reg + 0x34);
	warpa_fe->cam_debug_test.gaze[1].camsv_dbg =
		readl(gaze1_camsv0_reg);

	/* be checksum */
	for (i = 0; i < MTK_SIDE_IDX_MAX; i++)
		for (j = 0; j < 2; j++)
			dev_err(warpa_fe->dev,
				"side%d_camsv%d_dbg = 0x%08x\n", i, j,
				warpa_fe->cam_debug_test.side[i].camsv_dbg[j]);

	for (i = 0; i < MTK_GAZE_IDX_MAX; i++)
		dev_err(warpa_fe->dev, "gaze%d_camsv0_dbg = 0x%08x\n", i,
			warpa_fe->cam_debug_test.gaze[i].camsv_dbg);

	for (i = 0; i < MTK_SIDE_IDX_MAX; i++) {
		for (j = 0; j < 2; j++) { /* 2 BE pipe */
			dev_err(warpa_fe->dev,
				"side%d_be%d_label_dbg = 0x%08x\n", i, j,
				warpa_fe->cam_debug_test.side[i].
				be[j].label_dbg);
			dev_err(warpa_fe->dev,
				"side%d_be%d_data_dbg = 0x%08x\n", i, j,
				warpa_fe->cam_debug_test.side[i].
				be[j].data_dbg);
			dev_err(warpa_fe->dev,
				"side%d_be%d_merge_dbg = 0x%08x\n", i, j,
				warpa_fe->cam_debug_test.side[i].
				be[j].merge_table_dbg);
		}
	}
	/* check camsv side */
	for (i = 0; i < MTK_SIDE_IDX_MAX; i++) {
		for (j = 0; j < 2; j++) {
			if (debug_test_cam.side[i].camsv_dbg[j] !=
			warpa_fe->cam_debug_test.side[i].camsv_dbg[j]) {
				pr_err("side%d camsv%d dbg compare failed! %08x:%08x\n",
					i, j, debug_test_cam.side[i].
					camsv_dbg[j],
					warpa_fe->cam_debug_test.side[i].
					camsv_dbg[j]);
				goto CRC_FAIL;
			}
		}
	}

	/* check camsv gaze */
	for (i = 0; i < MTK_GAZE_IDX_MAX; i++)
	if (debug_test_cam.gaze[i].camsv_dbg !=
		warpa_fe->cam_debug_test.gaze[i].camsv_dbg) {
		pr_err("gaze%d camsv dbg compare failed! %08x:%08x\n",
			i, debug_test_cam.gaze[i].camsv_dbg,
			warpa_fe->cam_debug_test.gaze[i].camsv_dbg);
		goto CRC_FAIL;
	}

	/* check side be */
	for (i = 0; i < MTK_SIDE_IDX_MAX; i++) {
		for (j = 0; j < 2; j++) { /* 2 BE pipe */
			if (debug_test_cam.side[i].be[j].label_dbg !=
				warpa_fe->cam_debug_test.side[i].be[j].
				label_dbg) {
				pr_err("side%d be%d label dbg compare failed! %08x:%08x\n",
					i, j, debug_test_cam.side[i].
					be[j].label_dbg,
					warpa_fe->cam_debug_test.side[i].
					be[j].label_dbg);
				goto CRC_FAIL;
			}

			if (debug_test_cam.side[i].be[j].data_dbg !=
				warpa_fe->cam_debug_test.side[i].be[j].
				data_dbg) {
				pr_err("side%d be%d data dbg compare failed! %08x:%08x\n",
					i, j, debug_test_cam.side[i].
					be[j].data_dbg,
					warpa_fe->cam_debug_test.side[i].
					be[j].data_dbg);
				goto CRC_FAIL;
			}

			if (debug_test_cam.side[i].be[j].merge_table_dbg
				!= warpa_fe->cam_debug_test.side[i].be[j].
				merge_table_dbg) {
				pr_err("side%d be%d merge table dbg compare failed! %08x:%08x\n",
					i, j, debug_test_cam.side[i].
					be[j].merge_table_dbg,
					warpa_fe->cam_debug_test.side[i].
					be[j].merge_table_dbg);
				goto CRC_FAIL;
			}
		}
	}

	iounmap(side0_camsv0_reg);
	iounmap(side0_camsv1_reg);
	iounmap(side1_camsv0_reg);
	iounmap(side1_camsv1_reg);
	iounmap(gaze0_camsv0_reg);
	iounmap(gaze1_camsv0_reg);
	return ret;
CRC_FAIL:
	iounmap(side0_camsv0_reg);
	iounmap(side0_camsv1_reg);
	iounmap(side1_camsv0_reg);
	iounmap(side1_camsv1_reg);
	iounmap(gaze0_camsv0_reg);
	iounmap(gaze1_camsv0_reg);
	return -EINVAL;
}


static int mtk_warpa_fe_ioctl_enable_camera_sync(struct mtk_warpa_fe *warpa_fe,
	unsigned long arg)
{
	struct mtk_warpa_fe_camera_sync camera_sync;
	int ret = 0;

	ret = copy_from_user((void *)&camera_sync, (void *)arg,
				sizeof(camera_sync));
	if (ret) {
		dev_err(warpa_fe->dev,
			"ENABLE_CAMERA_SYNC: get params fail, ret=%d\n", ret);
		return ret;
	}
	LOG(CV_LOG_DBG, "enable camera sync!\n");
	if (camera_sync.sensor_fps > 120 || camera_sync.sensor_fps < 15)
		return -EINVAL;
	warpa_fe->sensor_fps = camera_sync.sensor_fps;
	ret = mtk_warpa_fe_config_camera_sync(warpa_fe, camera_sync.delay,
					      camera_sync.power_flag);
	if (ret) {
		dev_err(warpa_fe->dev, "config camera sync fail\n");
		return ret;
	}

	return ret;
}

static int mtk_warpa_fe_ioctl_disable_camera_sync(struct mtk_warpa_fe *warpa_fe)
{
	int ret = 0;

	ret = mtk_cv_disable(warpa_fe->dev_mmsys_core_top,
			MMSYSCFG_CAMERA_SYNC_SIDE01, NULL);
	if (ret)
		dev_err(warpa_fe->dev,
			"disable camera sync id: %d failed\n",
			MMSYSCFG_CAMERA_SYNC_SIDE01);

	ret = mtk_cv_disable(warpa_fe->dev_mmsys_core_top,
		MMSYSCFG_CAMERA_SYNC_SIDE23, NULL);
	if (ret)
		dev_err(warpa_fe->dev,
			"disable camera sync id: %d failed\n",
			MMSYSCFG_CAMERA_SYNC_SIDE23);

	return ret;
}

static int mtk_warpa_fe_ioctl_flush_cache(struct mtk_warpa_fe *warpa_fe)
{
	struct device *dev;
	int ret = 0, i;
	struct mtk_common_buf *buf;

	for (i = 0; i < MTK_WARPA_FE_BUF_TYPE_MAX; i++) {
		buf = warpa_fe->buf[i];
		ret = mtk_warpa_fe_get_dev(&dev, warpa_fe, i);
		if (ret) {
			dev_err(warpa_fe->dev, "get dev fail\n");
			return ret;
		}
		if (buf != NULL)
			dma_sync_sg_for_device(dev, buf->sg->sgl,
				buf->sg->nents, DMA_TO_DEVICE);
	}

	return ret;
}

static int mtk_warpa_fe_ioctl_one_shot_dump(struct mtk_warpa_fe *warpa_fe)
{
	int i = 0;

	warpa_fe->one_shot_dump = 1;

	/* wait up to timeout * img_num sec */
	for (i = 0; i < FLOW_TIMEOUT * warpa_fe->img_num; i++) {
		if (!warpa_fe->one_shot_dump)
			break;
		usleep_range(500, 1000);
	}

	warpa_fe->one_shot_dump = 0;
	if (i == FLOW_TIMEOUT * warpa_fe->img_num)
		return -ETIME;
	else
		return 0;
}

static long mtk_warpa_fe_ioctl(struct file *flip, unsigned int cmd,
		unsigned long arg)
{
	struct mtk_warpa_fe *warpa_fe;
	int ret = 0;

	if (!flip) {
		pr_err("mtk_warpa_fe_ioctl flip is NULL!\n");
		return -EFAULT;
	}

	warpa_fe = flip->private_data;

	switch (cmd) {
	case MTK_WARPA_FE_IOCTL_IMPORT_FD_TO_HANDLE:
		ret = mtk_warpa_fe_ioctl_import_fd_to_handle(warpa_fe, arg);
		break;
	case MTK_WARPA_FE_IOCTL_SET_CTRL_MASK:
		ret = mtk_warpa_fe_ioctl_set_ctrl_mask(warpa_fe, arg);
		break;
	case MTK_WARPA_FE_IOCTL_CONFIG_WARPA:
		ret = mtk_warpa_fe_ioctl_config_warpa(warpa_fe, arg);
		break;
	case MTK_WARPA_FE_IOCTL_CONFIG_FE:
		ret = mtk_warpa_fe_ioctl_config_fe(warpa_fe, arg);
		break;
	case MTK_WARPA_FE_IOCTL_CONFIG_WDMA:
		ret = mtk_warpa_fe_ioctl_config_wdma(warpa_fe, arg);
		break;
	case MTK_WARPA_FE_IOCTL_CONFIG_RSZ:
		ret = mtk_warpa_fe_ioctl_config_rsz(warpa_fe, arg);
		break;
	case MTK_WARPA_FE_IOCTL_CONFIG_PADDING:
		ret = mtk_warpa_fe_ioctl_config_padding(warpa_fe, arg);
		break;
	case MTK_WARPA_FE_IOCTL_SET_BUF:
		ret = mtk_warpa_fe_ioctl_set_buf(warpa_fe, arg);
		break;
	case MTK_WARPA_FE_IOCTL_STREAMON:
		ret = mtk_warpa_fe_ioctl_streamon(warpa_fe);
		break;
	case MTK_WARPA_FE_IOCTL_STREAMOFF:
		ret = mtk_warpa_fe_ioctl_streamoff(warpa_fe);
		break;
	case MTK_WARPA_FE_IOCTL_TRIGGER:
		ret = mtk_warpa_fe_ioctl_trigger(warpa_fe);
		break;
	case MTK_WARPA_FE_IOCTL_RESET:
		ret = mtk_warpa_fe_ioctl_reset(warpa_fe);
		break;
	case MTK_WARPA_FE_IOCTL_COMP:
		ret = mtk_warpa_fe_ioctl_comp(warpa_fe, arg);
		break;
	case MTK_WARPA_FE_IOCTL_PUT_HANDLE:
		ret = mtk_warpa_fe_ioctl_put_handle(warpa_fe, arg);
		break;
	case MTK_WARPA_FE_IOCTL_SET_VPU_REQ:
		ret = mtk_warpa_fe_ioctl_set_vpu_req(warpa_fe, arg);
		break;
	case MTK_WARPA_FE_IOCTL_CHECK_FLOW_DONE:
		ret = mtk_warpa_fe_ioctl_check_flow_done(warpa_fe);
		return ret;
	case MTK_WARPA_FE_IOCTL_GET_STATISTICS:
		ret = mtk_warpa_fe_ioctl_get_statistics(warpa_fe, arg);
		break;
	case MTK_WARPA_FE_IOCTL_SET_LOG_LEVEL:
		ret = mtk_warpa_fe_ioctl_set_log_level(arg);
		break;
	case MTK_WARPA_FE_IOCTL_DEBUG_TEST_CAM:
		ret = mtk_warpa_fe_ioctl_debug_test_cam(warpa_fe, arg);
		break;
	case MTK_WARPA_FE_ENABLE_CAMERA_SYNC:
		ret = mtk_warpa_fe_ioctl_enable_camera_sync(warpa_fe, arg);
		break;
	case MTK_WARPA_FE_DISABLE_CAMERA_SYNC:
		ret = mtk_warpa_fe_ioctl_disable_camera_sync(warpa_fe);
		break;
	case MTK_WARPA_FE_IOCTL_FLUSH_BUF_CACHE:
		ret = mtk_warpa_fe_ioctl_flush_cache(warpa_fe);
		break;
	case MTK_WARPA_FE_IOCTL_ONE_SHOT_DUMP:
		ret = mtk_warpa_fe_ioctl_one_shot_dump(warpa_fe);
		break;
	default:
		dev_err(warpa_fe->dev, "warpa_fe_ioctl: no such command!\n");
		return -EINVAL;
	}

	return ret;
}


static int mtk_warpa_fe_open(struct inode *inode, struct file *flip)
{
	int ret = 0;
	/*struct ivp_user *user;*/
	struct mtk_warpa_fe *warpa_fe = container_of(inode->i_cdev,
			struct mtk_warpa_fe, chrdev);

	flip->private_data = warpa_fe;

	return ret;
}

static int mtk_warpa_fe_release(struct inode *inode, struct file *flip)
{
	/*struct mtk_warpa_fe *warpa_fe = flip->private_data;*/

	return 0;
}

static const struct file_operations mtk_warpa_fe_fops = {
	.owner = THIS_MODULE,
	.open = mtk_warpa_fe_open,
	.release = mtk_warpa_fe_release,
	.unlocked_ioctl = mtk_warpa_fe_ioctl,
};

static int mtk_warpa_fe_reg_chardev(struct mtk_warpa_fe *warpa_fe)
{
	struct device *dev;
	char warpa_fe_dev_name[] = "mtk_warpa_fe";
	int ret;

	pr_debug("warpa_fe_reg_chardev\n");

	ret = alloc_chrdev_region(&warpa_fe->devt, 0, 1,
			warpa_fe_dev_name);
	if (ret < 0) {
		pr_err("warpa_fe_reg_chardev: alloc_chrdev_region fail, %d\n",
			ret);
		return ret;
	}

	pr_debug("warpa_fe_reg_chardev: MAJOR/MINOR = 0x%08x\n",
		warpa_fe->devt);

	/* Attatch file operation. */
	cdev_init(&warpa_fe->chrdev, &mtk_warpa_fe_fops);
	warpa_fe->chrdev.owner = THIS_MODULE;

	/* Add to system */
	ret = cdev_add(&warpa_fe->chrdev, warpa_fe->devt, 1);
	if (ret < 0) {
		pr_err("warpa_fe_reg_chardev: attatch file operation fail, %d\n",
			ret);
		unregister_chrdev_region(warpa_fe->devt, 1);
		return -EIO;
	}

	/* Create class register */
	warpa_fe->dev_class = class_create(THIS_MODULE, warpa_fe_dev_name);
	if (IS_ERR(warpa_fe->dev_class)) {
		ret = PTR_ERR(warpa_fe->dev_class);
		pr_err("Unable to create class, err = %d\n", ret);
		return -EIO;
	}

	dev = device_create(warpa_fe->dev_class, NULL, warpa_fe->devt,
			    NULL, warpa_fe_dev_name);
	if (IS_ERR(dev)) {
		ret = PTR_ERR(dev);
		pr_err("Failed to create device: /dev/%s, err = %d",
			warpa_fe_dev_name, ret);
		return -EIO;
	}

	pr_debug("warpa_fe_reg_chardev done\n");

	return 0;
}

static void mtk_warpa_fe_unreg_chardev(struct mtk_warpa_fe *warpa_fe)
{
	device_destroy(warpa_fe->dev_class, warpa_fe->devt);
	class_destroy(warpa_fe->dev_class);
	cdev_del(&warpa_fe->chrdev);
	unregister_chrdev_region(warpa_fe->devt, 1);
}

static int mtk_warpa_fe_get_device(struct device *dev,
		char *compatible, int idx, struct device **child_dev)
{
	struct platform_device *pdev;
	struct device_node *node;

	node = of_parse_phandle(dev->of_node, compatible, idx);
	if (!node) {
		dev_err(dev, "warpa_fe_get_device: could not find %s %d\n",
			compatible, idx);
		return -ENODEV;
	}

	pdev = of_find_device_by_node(node);
	if (!pdev || !pdev->dev.driver) {
		dev_warn(dev,
			"warpa_fe_get_device: waiting for device %s\n",
			 node->full_name);
		return -EPROBE_DEFER;
	}

	*child_dev = &pdev->dev;

	return 0;
}

static int mtk_warpa_fe_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_warpa_fe *warpa_fe;
	int ret;

	pr_debug("warpa_fe probe start\n");

	warpa_fe = devm_kzalloc(dev, sizeof(*warpa_fe), GFP_KERNEL);
	if (!warpa_fe)
		return -ENOMEM;

	warpa_fe->dev = dev;
	warpa_fe->polling_wait = 1;
	kern_log_level = CV_LOG_ERR;

	ret = mtk_warpa_fe_get_device(dev, "mediatek,mmsys_common_top",
			0, &warpa_fe->dev_mmsys_cmmn_top);
	if (ret)
		return ret;

	ret = mtk_warpa_fe_get_device(dev, "mediatek,mmsys_core_top",
			0, &warpa_fe->dev_mmsys_core_top);
	if (ret)
		return ret;

	ret = mtk_warpa_fe_get_device(dev, "mediatek,mutex", 0,
			&warpa_fe->dev_mutex);
	if (ret)
		return ret;

	ret = mtk_warpa_fe_get_device(dev, "mediatek,sysram", 0,
			&warpa_fe->dev_sysram);
	if (ret)
		return ret;

	ret = mtk_warpa_fe_get_device(dev, "mediatek,rbfc", 0,
			&warpa_fe->dev_rbfc);
	if (ret)
		return ret;

	ret = mtk_warpa_fe_get_device(dev, "mediatek,warpa", 0,
			&warpa_fe->dev_warpa);
	if (ret)
		return ret;

	ret = mtk_warpa_fe_get_device(dev, "mediatek,fe", 0,
			&warpa_fe->dev_fe);
	if (ret)
		return ret;

	ret = mtk_warpa_fe_get_device(dev, "mediatek,wdma", 0,
			&warpa_fe->dev_wdma0);
	if (ret)
		return ret;

	ret = mtk_warpa_fe_get_device(dev, "mediatek,wdma", 1,
			&warpa_fe->dev_wdma1);
	if (ret)
		return ret;


	ret = mtk_warpa_fe_get_device(dev, "mediatek,wdma", 2,
			&warpa_fe->dev_wdma2);
	if (ret)
		return ret;

	ret = mtk_warpa_fe_get_device(dev, "mediatek,padding", 0,
			&warpa_fe->dev_padding0);
	if (ret)
		return ret;

	ret = mtk_warpa_fe_get_device(dev, "mediatek,padding", 1,
			&warpa_fe->dev_padding1);
	if (ret)
		return ret;

	ret = mtk_warpa_fe_get_device(dev, "mediatek,padding", 2,
			&warpa_fe->dev_padding2);
	if (ret)
		return ret;

	ret = mtk_warpa_fe_get_device(dev, "mediatek,resizer", 0,
			&warpa_fe->dev_p_rsz0);
	if (ret)
		return ret;

	ret = mtk_warpa_fe_get_device(dev, "mediatek,resizer", 1,
			&warpa_fe->dev_p_rsz1);
	if (ret)
		return ret;

	ret = mtk_warpa_fe_reg_chardev(warpa_fe);
	if (ret)
		return ret;

	of_property_read_u32_array(dev->of_node, "gce-events",
			warpa_fe->cmdq_events,
			MTK_WARPA_FE_CMDQ_EVENT_MAX);


	INIT_WORK(&warpa_fe->trigger_work, mtk_warpa_fe_trigger_work);
	INIT_WORK(&warpa_fe->trigger_work_be, mtk_warpa_fe_trigger_work_be);
	INIT_WORK(&warpa_fe->trigger_work_p1_sof,
				mtk_warpa_fe_trigger_work_p1_sof);
	INIT_WORK(&warpa_fe->trigger_work_p2_sof,
			mtk_warpa_fe_trigger_work_p2_sof);

	warpa_fe->coef_tbl.idx = 4;
	warpa_fe->border_color.enable = false;
	warpa_fe->proc_mode = MTK_WARPA_PROC_MODE_LR;
	warpa_fe->warpa_out_mode = MTK_WARPA_OUT_MODE_DIRECT_LINK;
	warpa_fe->reset = 0;
	warpa_fe->blk_sz = 32;
	warpa_fe->out_buf_cnt = 0;

	warpa_fe->fe_flat_enable = 1;
	warpa_fe->fe_harris_kappa = 4;
	warpa_fe->fe_th_g = 2;
	warpa_fe->fe_th_c = 0;
	warpa_fe->fe_cr_val_sel = 0;

	warpa_fe->warpa_fe_flush_wait = 0;
	warpa_fe->be_flush_wait = 0;

	create_warpa_fe_statistic_buf(warpa_fe);
#if 0
	warpa_fe->mutex = mtk_mutex_get(warpa_fe->dev_mutex, 0);
	if (IS_ERR(warpa_fe->mutex)) {
		dev_err(warpa_fe->dev, "mutex_get fail!!\n");
		return PTR_ERR(warpa_fe->mutex);
	}
#endif
	platform_set_drvdata(pdev, warpa_fe);
	pr_debug("warpa_fe probe success!\n");

	return 0;
}

static int mtk_warpa_fe_remove(struct platform_device *pdev)
{
	struct mtk_warpa_fe *warpa_fe = platform_get_drvdata(pdev);

	destroy_warpa_fe_statistic_buf(warpa_fe);
	mtk_warpa_fe_unreg_chardev(warpa_fe);

	return 0;
}

static const struct of_device_id warpa_fe_driver_dt_match[] = {
	{ .compatible = "mediatek,mt3612-warpa-fe" },
	{},
};
MODULE_DEVICE_TABLE(of, warpa_fe_driver_dt_match);

struct platform_driver mtk_warpa_fe_driver = {
	.probe		= mtk_warpa_fe_probe,
	.remove		= mtk_warpa_fe_remove,
	.driver		= {
		.name	= "mediatek-warpa-fe",
		.owner	= THIS_MODULE,
		.of_match_table = warpa_fe_driver_dt_match,
	},
};

module_platform_driver(mtk_warpa_fe_driver);
MODULE_LICENSE("GPL");

