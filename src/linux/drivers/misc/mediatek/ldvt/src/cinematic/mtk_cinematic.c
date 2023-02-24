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
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/gcd.h>
#include <soc/mediatek/mtk_p2s.h>
#include <soc/mediatek/mtk_wdma.h>
#include <soc/mediatek/mtk_slicer.h>
#include <soc/mediatek/mtk_crop.h>
#include <soc/mediatek/mtk_dprx_info.h>
#include <soc/mediatek/mtk_dprx_if.h>
#include <uapi/drm/drm_fourcc.h>
#include <soc/mediatek/mtk_resizer.h>
#include <mtk_common_util.h>
#include "mtk_cinematic.h"

static int cc_dbg_mode;
static int mtk_cinematic_cfg_fps_to_slcr_fps(struct mtk_cinematic_test *ct,
					     enum slcr_pg_fps *slcr_fps);

static int mtk_util_buf_create(struct device *dev, u32 size, u32 align,
				void **va, dma_addr_t *pa)
{
	struct dma_attrs dma_attrs;

	init_dma_attrs(&dma_attrs);
	dma_set_attr(DMA_ATTR_WRITE_COMBINE | DMA_ATTR_FORCE_CONTIGUOUS,
		     &dma_attrs);
	*va = dma_alloc_attrs(dev, roundup(size, align), pa, GFP_KERNEL,
			      &dma_attrs);

	if ((*va == NULL) || (*pa == 0)) {
		dev_err(dev, "dma_alloc_attrs NG!!\n");
		return -ENOMEM;
	}

	dev_dbg(dev, "dma_alloc_attrs size %d  va = %p, pa = %pad\n",
		roundup(size, align), *va, pa);

	return 0;
}

static void mtk_util_buf_destroy(struct device *dev, u32 size, u32 align,
				void *va, dma_addr_t pa)
{
	struct dma_attrs dma_attrs;

	init_dma_attrs(&dma_attrs);
	dma_set_attr(DMA_ATTR_WRITE_COMBINE | DMA_ATTR_FORCE_CONTIGUOUS,
		     &dma_attrs);
	dma_free_attrs(dev, roundup(size, align), va, pa, &dma_attrs);
}

static int mtk_cinematic_power_on(struct mtk_cinematic_test *ct)
{
	int ret = 0;

	ret = mtk_mmsys_cfg_power_on(ct->dev_mmsys_core_top);
	if (unlikely(ret)) {
		dev_err(ct->dev, "Failed to power on mmsys_core: %d\n", ret);
		return ret;
	}

	ret = mtk_mutex_power_on(ct->mutex);
	if (unlikely(ret)) {
		dev_err(ct->dev, "Failed to power on mutex: %d\n", ret);
		return ret;
	}

	ret = mtk_slcr_power_on(ct->dev_slicer);
	if (unlikely(ret)) {
		dev_err(ct->dev, "Failed to power on slicer: %d\n", ret);
		return ret;
	}

	ret =  mtk_crop_power_on(ct->dev_crop);
	if (unlikely(ret)) {
		dev_err(ct->dev, "Failed to power on slicer: %d\n", ret);
		return ret;
	}

	if (ct->disp_path < 2) {
		ret = mtk_resizer_power_on(ct->dev_resizer);
		if (unlikely(ret)) {
			dev_err(ct->dev, "Failed to power on resizer: %d\n",
				ret);
			return ret;
		}

		ret = mtk_p2s_power_on(ct->dev_p2s);
		if (unlikely(ret)) {
			dev_err(ct->dev, "Failed to power on p2s: %d\n", ret);
			return ret;
		}
	}
	ret = mtk_wdma_power_on(ct->dev_wdma);
	if (unlikely(ret)) {
		dev_err(ct->dev, "Failed to power on wdma: %d\n", ret);
		return ret;
	}

	return 0;
}

static int mtk_cinematic_power_off(struct mtk_cinematic_test *ct)
{
	int ret = 0;

	ret = mtk_mmsys_cfg_power_off(ct->dev_mmsys_core_top);
	if (unlikely(ret)) {
		dev_err(ct->dev, "Failed to power on mmsys_core: %d\n", ret);
		return ret;
	}

	ret = mtk_mutex_power_off(ct->mutex);
	if (unlikely(ret)) {
		dev_err(ct->dev, "Failed to power on mutex: %d\n", ret);
		return ret;
	}

	ret = mtk_slcr_power_off(ct->dev_slicer);
	if (unlikely(ret)) {
		dev_err(ct->dev, "Failed to power on slicer: %d\n", ret);
		return ret;
	}

	ret =  mtk_crop_power_off(ct->dev_crop);
	if (unlikely(ret)) {
		dev_err(ct->dev, "Failed to power on slicer: %d\n", ret);
		return ret;
	}

	if (ct->disp_path < 2) {
		ret = mtk_resizer_power_off(ct->dev_resizer);
		if (unlikely(ret)) {
			dev_err(ct->dev, "Failed to power on resizer: %d\n",
				ret);
			return ret;
		}

		ret = mtk_p2s_power_off(ct->dev_p2s);
		if (unlikely(ret)) {
			dev_err(ct->dev, "Failed to power on p2s: %d\n", ret);
			return ret;
		}
	}
	ret = mtk_wdma_power_off(ct->dev_wdma);
	if (unlikely(ret)) {
		dev_err(ct->dev, "Failed to power on wdma: %d\n", ret);
		return ret;
	}

	return 0;
}

static int mtk_cinematic_get_delay(struct mtk_cinematic_test *ct, int *delay)
{
	u32 i_fps, dp_vtotal, vfp, vbp, vsync, line_time, v_active;

	i_fps = ct->para.input_fps;

	dp_vtotal = ct->dp_info.vid_timing_msa.v_total;
	vfp = ct->dp_info.vid_timing_msa.v_front_porch;
	vbp = ct->dp_info.vid_timing_msa.v_back_porch;
	vsync = ct->dp_info.vid_timing_msa.v_sync_width;
	v_active = ct->dp_info.vid_timing_msa.v_active;

	dev_err(ct->dev, "vbp %d, vsync %d, vfp %d, v_total %d, v_act %d\n",
		vbp, vsync, vfp, dp_vtotal, v_active);

	switch (i_fps) {
	case FPS_120:
		/* fall through */
	case FPS_119:
		line_time = 1000000 * 100 / FPS_120 / dp_vtotal;
		break;
	case FPS_60:
		/* fall through */
	case FPS_59:
		line_time = 1000000 * 100 / FPS_60 / dp_vtotal;
		break;
	case FPS_50:
		line_time = 1000000 * 100 / FPS_50 / dp_vtotal;
		break;
	case FPS_24:
		/* fall through */
	case FPS_23:
		line_time = 1000000 * 100 / FPS_24 / dp_vtotal;
		break;

	default:
		line_time = 740;
	}

	if (vbp > 5)
		*delay = (line_time * 3) / 100;
	else
		*delay = 2;

	dev_err(ct->dev, "Front SOF delay = %d us\n", *delay);
	return 0;
}

void mtk_reset_output_port_buf(struct mtk_cinematic_test *ct)
{
	u32 i;

	for (i = 0; i < WDMA_BUF; i++)
		ct->output_port.buf[i].ref_count = 0;

	ct->cur_fb_id = 0;
	ct->done_fb_id = 0;
	ct->next_fb_id = 0;
	ct->gpu_fb_id = 0xff;
}
EXPORT_SYMBOL(mtk_reset_output_port_buf);

static int mtk_cinematic_quad_path_connect(struct mtk_cinematic_test **ct)
{
	struct mtk_mutex_res *mutex;
	u32 i;
	u32 lhc_en, lhc_sync_path;
	u32 ret = 0;

	mutex = ct[MAIN_PIPE]->mutex;
	lhc_en = ct[MAIN_PIPE]->para.lhc_en;
	lhc_sync_path = ct[MAIN_PIPE]->para.lhc_sync_path;

	ret |= mtk_mutex_add_comp(mutex, MUTEX_COMPONENT_SLICER, NULL);
	ret |= mtk_mutex_add_comp(mutex, MUTEX_COMPONENT_P2D0, NULL);

	/*for path 0, 1*/
	for (i = 0; i < 2; i++) {
		ret |= mtk_mutex_add_comp(mutex, ct[i]->disp_mutex.rsz_id,
					  NULL);
		ret |= mtk_mutex_add_comp(mutex, ct[i]->disp_mutex.crop_id,
					  NULL);
		ret |= mtk_mutex_add_comp(mutex, ct[i]->disp_mutex.wdma_id,
					  NULL);

		ret |= mtk_mmsys_cfg_connect_comp(ct[i]->dev_mmsys_core_top,
						  MMSYSCFG_COMPONENT_SLICER_VID,
						  ct[i]->disp_mmsys.crop_id,
						  NULL);
	}

	ret |= mtk_mmsys_cfg_connect_comp(ct[MAIN_PIPE]->dev_mmsys_core_top,
					  MMSYSCFG_COMPONENT_P2S0,
					  MMSYSCFG_COMPONENT_DISP_WDMA0, NULL);
	ret |= mtk_mmsys_cfg_connect_comp(ct[1]->dev_mmsys_core_top,
					  MMSYSCFG_COMPONENT_RSZ1,
					  MMSYSCFG_COMPONENT_DISP_WDMA1, NULL);

	/*for path 2, 3*/
	for (i = 2; i < 4; i++) {
		ret |= mtk_mutex_add_comp(mutex, ct[i]->disp_mutex.crop_id,
						NULL);
		ret |= mtk_mutex_add_comp(mutex, ct[i]->disp_mutex.wdma_id,
						NULL);
		ret |= mtk_mmsys_cfg_connect_comp(ct[i]->dev_mmsys_core_top,
						  MMSYSCFG_COMPONENT_SLICER_VID,
						  ct[i]->disp_mmsys.crop_id,
						  NULL);
		ret |= mtk_mmsys_cfg_connect_comp(ct[i]->dev_mmsys_core_top,
						  ct[i]->disp_mmsys.crop_id,
						  ct[i]->disp_mmsys.wdma_id,
						  NULL);
	}

	if (lhc_en && lhc_sync_path == FROM_DP) {
		ret |= mtk_mutex_add_comp(mutex, MUTEX_COMPONENT_LHC0, NULL);
#ifdef CONFIG_MACH_MT3612_A0
		ret |= mtk_mutex_add_comp(mutex, MUTEX_COMPONENT_LHC1, NULL);
		ret |= mtk_mutex_add_comp(mutex, MUTEX_COMPONENT_LHC2, NULL);
		ret |= mtk_mutex_add_comp(mutex, MUTEX_COMPONENT_LHC3, NULL);
#endif
		ret |= mtk_mmsys_cfg_connect_comp(
				ct[MAIN_PIPE]->dev_mmsys_core_top,
				MMSYSCFG_COMPONENT_SLICER_VID,
				MMSYSCFG_COMPONENT_LHC0, NULL);
#ifdef CONFIG_MACH_MT3612_A0
		ret |= mtk_mmsys_cfg_connect_comp(
				ct[MAIN_PIPE]->dev_mmsys_core_top,
				MMSYSCFG_COMPONENT_SLICER_VID,
				MMSYSCFG_COMPONENT_LHC1, NULL);

		ret |= mtk_mmsys_cfg_connect_comp(
				ct[MAIN_PIPE]->dev_mmsys_core_top,
				MMSYSCFG_COMPONENT_SLICER_VID,
				MMSYSCFG_COMPONENT_LHC2, NULL);

		ret |= mtk_mmsys_cfg_connect_comp(
				ct[MAIN_PIPE]->dev_mmsys_core_top,
				MMSYSCFG_COMPONENT_SLICER_VID,
				MMSYSCFG_COMPONENT_LHC3, NULL);
#endif
	}

	return ret;
}

static int mtk_cinematic_quad_path_disconnect(struct mtk_cinematic_test **ct)
{
	struct mtk_mutex_res *mutex;
	u32 i;
	u32 lhc_en, lhc_sync_path;
	u32 ret = 0;

	mutex = ct[MAIN_PIPE]->mutex;
	lhc_en = ct[MAIN_PIPE]->para.lhc_en;
	lhc_sync_path = ct[MAIN_PIPE]->para.lhc_sync_path;

	ret |= mtk_mutex_remove_comp(mutex, MUTEX_COMPONENT_SLICER, NULL);
	ret |= mtk_mutex_remove_comp(mutex, MUTEX_COMPONENT_P2D0, NULL);

	/*for path 0, 1*/
	for (i = 0; i < 2; i++) {
		ret |= mtk_mutex_remove_comp(mutex, ct[i]->disp_mutex.rsz_id,
					  NULL);
		ret |= mtk_mutex_remove_comp(mutex, ct[i]->disp_mutex.crop_id,
					  NULL);
		ret |= mtk_mutex_remove_comp(mutex, ct[i]->disp_mutex.wdma_id,
					  NULL);

		ret |= mtk_mmsys_cfg_disconnect_comp(ct[i]->dev_mmsys_core_top,
						  MMSYSCFG_COMPONENT_SLICER_VID,
						  ct[i]->disp_mmsys.crop_id,
						  NULL);
	}

	ret |= mtk_mmsys_cfg_disconnect_comp(ct[MAIN_PIPE]->dev_mmsys_core_top,
					  MMSYSCFG_COMPONENT_P2S0,
					  MMSYSCFG_COMPONENT_DISP_WDMA0, NULL);
	ret |= mtk_mmsys_cfg_disconnect_comp(ct[1]->dev_mmsys_core_top,
					  MMSYSCFG_COMPONENT_RSZ1,
					  MMSYSCFG_COMPONENT_DISP_WDMA1, NULL);

	/*for path 2, 3*/
	for (i = 2; i < 4; i++) {
		ret |= mtk_mutex_remove_comp(mutex, ct[i]->disp_mutex.crop_id,
						NULL);
		ret |= mtk_mutex_remove_comp(mutex, ct[i]->disp_mutex.wdma_id,
						NULL);
		ret |= mtk_mmsys_cfg_disconnect_comp(ct[i]->dev_mmsys_core_top,
						  MMSYSCFG_COMPONENT_SLICER_VID,
						  ct[i]->disp_mmsys.crop_id,
						  NULL);
		ret |= mtk_mmsys_cfg_disconnect_comp(ct[i]->dev_mmsys_core_top,
						  ct[i]->disp_mmsys.crop_id,
						  ct[i]->disp_mmsys.wdma_id,
						  NULL);
	}

	if (lhc_en && lhc_sync_path == FROM_DP) {
		ret |= mtk_mutex_remove_comp(mutex, MUTEX_COMPONENT_LHC0, NULL);
#ifdef CONFIG_MACH_MT3612_A0
		ret |= mtk_mutex_remove_comp(mutex, MUTEX_COMPONENT_LHC0, NULL);
		ret |= mtk_mutex_remove_comp(mutex, MUTEX_COMPONENT_LHC0, NULL);
		ret |= mtk_mutex_remove_comp(mutex, MUTEX_COMPONENT_LHC0, NULL);
#endif
		ret |= mtk_mmsys_cfg_disconnect_comp(
			ct[MAIN_PIPE]->dev_mmsys_core_top,
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_LHC0, NULL);
#ifdef CONFIG_MACH_MT3612_A0
		ret |= mtk_mmsys_cfg_disconnect_comp(
			ct[MAIN_PIPE]->dev_mmsys_core_top,
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_LHC1, NULL);
		ret |= mtk_mmsys_cfg_disconnect_comp(
			ct[MAIN_PIPE]->dev_mmsys_core_top,
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_LHC2, NULL);
		ret |= mtk_mmsys_cfg_disconnect_comp(
			ct[MAIN_PIPE]->dev_mmsys_core_top,
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_LHC3, NULL);
#endif
	}

	return ret;
}


static int mtk_cinematic_dual_path_connect(struct mtk_cinematic_test **ct)
{
	struct mtk_mutex_res *mutex;
	u32 i;
	u32 lhc_en, lhc_sync_path;
	u32 ret = 0;

	mutex = ct[MAIN_PIPE]->mutex;
	lhc_en = ct[MAIN_PIPE]->para.lhc_en;
	lhc_sync_path = ct[MAIN_PIPE]->para.lhc_sync_path;

	ret |= mtk_mutex_add_comp(mutex, MUTEX_COMPONENT_SLICER, NULL);
	ret |= mtk_mutex_add_comp(mutex, MUTEX_COMPONENT_P2D0, NULL);
	ret |= mtk_mutex_add_comp(mutex, MUTEX_COMPONENT_DISP_WDMA0, NULL);

	for (i = 0; i < 2; i++) {
		ret |= mtk_mutex_add_comp(mutex, ct[i]->disp_mutex.rsz_id,
					  NULL);
		ret |= mtk_mutex_add_comp(mutex, ct[i]->disp_mutex.crop_id,
					  NULL);
		ret |= mtk_mmsys_cfg_connect_comp(ct[i]->dev_mmsys_core_top,
						  MMSYSCFG_COMPONENT_SLICER_VID,
						  ct[i]->disp_mmsys.crop_id,
						  NULL);
	}

	ret |= mtk_mmsys_cfg_connect_comp(ct[MAIN_PIPE]->dev_mmsys_core_top,
					  MMSYSCFG_COMPONENT_P2S0,
					  MMSYSCFG_COMPONENT_DISP_WDMA0, NULL);

	ret |= mtk_mmsys_cfg_connect_comp(ct[MAIN_PIPE]->dev_mmsys_core_top,
					  MMSYSCFG_COMPONENT_RSZ1,
					  MMSYSCFG_COMPONENT_P2S0, NULL);

	if (lhc_en && lhc_sync_path == FROM_DP) {

		ret |= mtk_mutex_add_comp(mutex, MUTEX_COMPONENT_LHC0, NULL);
#ifdef CONFIG_MACH_MT3612_A0
		ret |= mtk_mutex_add_comp(mutex, MUTEX_COMPONENT_LHC1, NULL);
		ret |= mtk_mutex_add_comp(mutex, MUTEX_COMPONENT_LHC2, NULL);
		ret |= mtk_mutex_add_comp(mutex, MUTEX_COMPONENT_LHC3, NULL);
#endif
		ret |= mtk_mmsys_cfg_connect_comp(
			ct[MAIN_PIPE]->dev_mmsys_core_top,
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_LHC0, NULL);
#ifdef CONFIG_MACH_MT3612_A0
		ret |= mtk_mmsys_cfg_connect_comp(
			ct[MAIN_PIPE]->dev_mmsys_core_top,
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_LHC1, NULL);

		ret |= mtk_mmsys_cfg_connect_comp(
			ct[MAIN_PIPE]->dev_mmsys_core_top,
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_LHC2, NULL);

		ret |= mtk_mmsys_cfg_connect_comp(
			ct[MAIN_PIPE]->dev_mmsys_core_top,
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_LHC3, NULL);
#endif
	}

	return ret;
}

static int mtk_cinematic_dual_path_disconnect(struct mtk_cinematic_test **ct)
{
	struct mtk_mutex_res *mutex;
	u32 i;
	u32 lhc_en, lhc_sync_path;
	u32 ret = 0;

	mutex = ct[MAIN_PIPE]->mutex;
	lhc_en = ct[MAIN_PIPE]->para.lhc_en;
	lhc_sync_path = ct[MAIN_PIPE]->para.lhc_sync_path;

	ret |= mtk_mutex_remove_comp(mutex, MUTEX_COMPONENT_SLICER, NULL);
	ret |= mtk_mutex_remove_comp(mutex, MUTEX_COMPONENT_P2D0, NULL);
	ret |= mtk_mutex_remove_comp(mutex, MUTEX_COMPONENT_DISP_WDMA0, NULL);

	for (i = 0; i < 2; i++) {
		ret |= mtk_mutex_remove_comp(mutex, ct[i]->disp_mutex.rsz_id,
					     NULL);
		ret |= mtk_mutex_remove_comp(mutex, ct[i]->disp_mutex.crop_id,
					     NULL);

		ret |= mtk_mmsys_cfg_disconnect_comp(ct[i]->dev_mmsys_core_top,
			MMSYSCFG_COMPONENT_SLICER_VID,
			ct[i]->disp_mmsys.crop_id, NULL);
	}

	mtk_mmsys_cfg_disconnect_comp(ct[MAIN_PIPE]->dev_mmsys_core_top,
				   MMSYSCFG_COMPONENT_P2S0,
				   MMSYSCFG_COMPONENT_DISP_WDMA0, NULL);

	mtk_mmsys_cfg_disconnect_comp(ct[MAIN_PIPE]->dev_mmsys_core_top,
				   MMSYSCFG_COMPONENT_RSZ1,
				   MMSYSCFG_COMPONENT_P2S0, NULL);

	if (lhc_en && lhc_sync_path == FROM_DP) {
		ret |= mtk_mutex_remove_comp(mutex, MUTEX_COMPONENT_LHC0, NULL);
#ifdef CONFIG_MACH_MT3612_A0
		ret |= mtk_mutex_remove_comp(mutex, MUTEX_COMPONENT_LHC0, NULL);
		ret |= mtk_mutex_remove_comp(mutex, MUTEX_COMPONENT_LHC0, NULL);
		ret |= mtk_mutex_remove_comp(mutex, MUTEX_COMPONENT_LHC0, NULL);
#endif
		ret |= mtk_mmsys_cfg_disconnect_comp(
			ct[MAIN_PIPE]->dev_mmsys_core_top,
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_LHC0, NULL);
#ifdef CONFIG_MACH_MT3612_A0
		ret |= mtk_mmsys_cfg_disconnect_comp(
			ct[MAIN_PIPE]->dev_mmsys_core_top,
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_LHC1, NULL);
		ret |= mtk_mmsys_cfg_disconnect_comp(
			ct[MAIN_PIPE]->dev_mmsys_core_top,
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_LHC2, NULL);
		ret |= mtk_mmsys_cfg_disconnect_comp(
			ct[MAIN_PIPE]->dev_mmsys_core_top,
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_LHC3, NULL);
#endif
	}

	return ret;
}

static int mtk_cinematic_path_connect(struct mtk_cinematic_test *ct)
{
	struct mtk_mutex_res *mutex;
	u32 lhc_en, lhc_sync_path;
	u32 ret = 0;

	mutex = ct->mutex;
	lhc_en = ct->para.lhc_en;
	lhc_sync_path = ct->para.lhc_sync_path;

	if (ct->disp_path < 2) {
		ret |= mtk_mutex_add_comp(mutex, MUTEX_COMPONENT_SLICER, NULL);
		ret |= mtk_mutex_add_comp(mutex, MUTEX_COMPONENT_P2D0, NULL);
		ret |= mtk_mutex_add_comp(mutex, ct->disp_mutex.rsz_id, NULL);
		ret |= mtk_mutex_add_comp(mutex, ct->disp_mutex.crop_id, NULL);
		ret |= mtk_mutex_add_comp(mutex, ct->disp_mutex.wdma_id, NULL);
	} else {
		ret |= mtk_mutex_add_comp(mutex, MUTEX_COMPONENT_SLICER, NULL);
		ret |= mtk_mutex_add_comp(mutex, ct->disp_mutex.crop_id, NULL);
		ret |= mtk_mutex_add_comp(mutex, ct->disp_mutex.wdma_id, NULL);
	}

	ret |= mtk_mmsys_cfg_connect_comp(ct->dev_mmsys_core_top,
					  MMSYSCFG_COMPONENT_SLICER_VID,
					  ct->disp_mmsys.crop_id, NULL);

	if (ct->disp_path == 0) {
		ret |= mtk_mmsys_cfg_connect_comp(ct->dev_mmsys_core_top,
						  MMSYSCFG_COMPONENT_P2S0,
						  MMSYSCFG_COMPONENT_DISP_WDMA0,
						  NULL);
	} else if (ct->disp_path == 1) {
		ret |= mtk_mmsys_cfg_connect_comp(ct->dev_mmsys_core_top,
						  ct->disp_mmsys.rsz_id,
						  ct->disp_mmsys.wdma_id, NULL);
	} else {
		ret |= mtk_mmsys_cfg_connect_comp(ct->dev_mmsys_core_top,
						  ct->disp_mmsys.crop_id,
						  ct->disp_mmsys.wdma_id, NULL);
	}

	if (lhc_en && lhc_sync_path == FROM_DP) {

		ret |= mtk_mutex_add_comp(mutex, MUTEX_COMPONENT_LHC0, NULL);
#ifdef CONFIG_MACH_MT3612_A0
		ret |= mtk_mutex_add_comp(mutex, MUTEX_COMPONENT_LHC1, NULL);
		ret |= mtk_mutex_add_comp(mutex, MUTEX_COMPONENT_LHC2, NULL);
		ret |= mtk_mutex_add_comp(mutex, MUTEX_COMPONENT_LHC3, NULL);
#endif
		ret |= mtk_mmsys_cfg_connect_comp(ct->dev_mmsys_core_top,
						  MMSYSCFG_COMPONENT_SLICER_VID,
						  MMSYSCFG_COMPONENT_LHC0,
						  NULL);
#ifdef CONFIG_MACH_MT3612_A0
		ret |= mtk_mmsys_cfg_connect_comp(ct->dev_mmsys_core_top,
						  MMSYSCFG_COMPONENT_SLICER_VID,
						  MMSYSCFG_COMPONENT_LHC1,
						  NULL);
		ret |= mtk_mmsys_cfg_connect_comp(ct->dev_mmsys_core_top,
						  MMSYSCFG_COMPONENT_SLICER_VID,
						  MMSYSCFG_COMPONENT_LHC2,
						  NULL);
		ret |= mtk_mmsys_cfg_connect_comp(ct->dev_mmsys_core_top,
						  MMSYSCFG_COMPONENT_SLICER_VID,
						  MMSYSCFG_COMPONENT_LHC3,
						  NULL);
#endif
	}

	return ret;
}

static int mtk_cinematic_path_disconnect(struct mtk_cinematic_test *ct)
{
	struct mtk_mutex_res *mutex;
	u32 lhc_en, lhc_sync_path;
	u32 ret = 0;

	mutex = ct->mutex;
	lhc_en = ct->para.lhc_en;
	lhc_sync_path = ct->para.lhc_sync_path;

	if (ct->disp_path < 2) {
		ret |= mtk_mutex_remove_comp(mutex, MUTEX_COMPONENT_SLICER,
					     NULL);
		ret |= mtk_mutex_remove_comp(mutex, ct->disp_mutex.rsz_id,
					     NULL);
		ret |= mtk_mutex_remove_comp(mutex, ct->disp_mutex.crop_id,
					     NULL);
		ret |= mtk_mutex_remove_comp(mutex, ct->disp_mutex.wdma_id,
					     NULL);
		ret |= mtk_mutex_remove_comp(mutex, MUTEX_COMPONENT_P2D0, NULL);
	} else {
		ret |= mtk_mutex_remove_comp(mutex, MUTEX_COMPONENT_SLICER,
					     NULL);
		ret |= mtk_mutex_remove_comp(mutex, ct->disp_mutex.crop_id,
					     NULL);
		ret |= mtk_mutex_remove_comp(mutex, ct->disp_mutex.wdma_id,
					     NULL);
	}

	if (ct->disp_path == 0) {
		ret |= mtk_mmsys_cfg_disconnect_comp(
			ct->dev_mmsys_core_top,
			MMSYSCFG_COMPONENT_P2S0,
			MMSYSCFG_COMPONENT_DISP_WDMA0, NULL);
	} else if (ct->disp_path == 1) {
		ret |= mtk_mmsys_cfg_disconnect_comp(ct->dev_mmsys_core_top,
						     ct->disp_mmsys.rsz_id,
						     ct->disp_mmsys.wdma_id,
						     NULL);
	} else {
		ret |= mtk_mmsys_cfg_disconnect_comp(ct->dev_mmsys_core_top,
						     ct->disp_mmsys.crop_id,
						     ct->disp_mmsys.wdma_id,
						     NULL);
	}

	ret |= mtk_mmsys_cfg_disconnect_comp(ct->dev_mmsys_core_top,
					     MMSYSCFG_COMPONENT_SLICER_VID,
					     ct->disp_mmsys.crop_id, NULL);

	if (lhc_en && lhc_sync_path == FROM_DP) {

		ret |= mtk_mutex_remove_comp(mutex, MUTEX_COMPONENT_LHC0, NULL);
#ifdef CONFIG_MACH_MT3612_A0
		ret |= mtk_mutex_remove_comp(mutex, MUTEX_COMPONENT_LHC1, NULL);
		ret |= mtk_mutex_remove_comp(mutex, MUTEX_COMPONENT_LHC2, NULL);
		ret |= mtk_mutex_remove_comp(mutex, MUTEX_COMPONENT_LHC3, NULL);
#endif
		ret |= mtk_mmsys_cfg_disconnect_comp(ct->dev_mmsys_core_top,
						  MMSYSCFG_COMPONENT_SLICER_VID,
						  MMSYSCFG_COMPONENT_LHC0,
						  NULL);
#ifdef CONFIG_MACH_MT3612_A0
		ret |= mtk_mmsys_cfg_disconnect_comp(ct->dev_mmsys_core_top,
						  MMSYSCFG_COMPONENT_SLICER_VID,
						  MMSYSCFG_COMPONENT_LHC1,
						  NULL);
		ret |= mtk_mmsys_cfg_disconnect_comp(ct->dev_mmsys_core_top,
						  MMSYSCFG_COMPONENT_SLICER_VID,
						  MMSYSCFG_COMPONENT_LHC2,
						  NULL);
		ret |= mtk_mmsys_cfg_disconnect_comp(ct->dev_mmsys_core_top,
						  MMSYSCFG_COMPONENT_SLICER_VID,
						  MMSYSCFG_COMPONENT_LHC3,
						  NULL);
#endif
	}

	return ret;
}

#ifdef CONFIG_MACH_MT3612_A0
int mtk_cinematic_set_dp_video(struct mtk_cinematic_test *ct,
			       struct dprx_video_info_s *video)
{
	if (IS_ERR(ct))
		return -EINVAL;


	memcpy(&ct->dp_info, video, sizeof(ct->dp_info));
	ct->dp_video_set = true;

	return 0;
}
EXPORT_SYMBOL(mtk_cinematic_set_dp_video);
#endif

int mtk_format_to_bpp(u32 fmt)
{
	switch (fmt) {
	case DRM_FORMAT_ARGB2101010:
	case DRM_FORMAT_ABGR2101010:
	case DRM_FORMAT_AYUV2101010:
	case DRM_FORMAT_RGBA1010102:
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_ARGB8888:
		return 32;
	case DRM_FORMAT_NV12:
		return 8;
	case DRM_FORMAT_NV12_10BIT:
		return 10;
	case DRM_FORMAT_C8:
	case DRM_FORMAT_R8:
		return 8;
	case DRM_FORMAT_R10:
		return 10;
	case DRM_FORMAT_RGB888:
	case DRM_FORMAT_BGR888:
		return 24;
	case DRM_FORMAT_YUYV:
		return 16;
	}

	return 0;
}
EXPORT_SYMBOL(mtk_format_to_bpp);

int mtk_cinematic_set_dbg(int dbg)
{
	cc_dbg_mode = dbg;
	pr_err("dbg_mode = %d\n", cc_dbg_mode);
	return 0;
}
EXPORT_SYMBOL(mtk_cinematic_set_dbg);

int mtk_cinematic_get_dbg(void)
{
	return  cc_dbg_mode;
}
EXPORT_SYMBOL(mtk_cinematic_get_dbg);

int mtk_dump_buf(struct mtk_cinematic_test *ct, int buf_idx, int size,
		 char *file_path)
{
	struct mtk_common_buf *ion_buf;
	struct mtk_dma_buf *k_buf;
	int alignment = 0;

	if (IS_ERR(ct))
		return -EINVAL;

	if (ct->para.pvric_en & COMPRESS_INPUT_ENABLE) {
		alignment = ((ct->para.rp.out_w * ct->para.rp.out_h +
			      127)) >> 7;
		alignment = ct->output_port.pvric_hdr_offset - alignment;
		size -= alignment;
	}

	if (!ct->para.gp.bypass) {
#if RUN_BIT_TRUE
	buf_idx = 1;
#endif
		ion_buf = ct->output_port.buf[buf_idx].ion_buf;

		pa_to_vaddr_dev(ct->dev_wdma, ion_buf, size, true);

		pr_info("dump idx:%d, size:%d @%p\n", buf_idx, size,
			(void *)ion_buf->kvaddr);

		mtk_common_write_file(ion_buf->kvaddr + alignment,
				      file_path, size);

		pa_to_vaddr_dev(ct->dev_wdma, ion_buf, size, false);
	} else {
		k_buf = ct->output_port.buf[buf_idx].k_buf;

		dma_sync_single_for_device(ct->dev_wdma, k_buf->pa + alignment,
					   size, DMA_TO_DEVICE);

		mtk_common_write_file(k_buf->va + alignment, file_path, size);
	}
	return 0;
}
EXPORT_SYMBOL(mtk_dump_buf);

int mtk_cinematic_start(struct mtk_cinematic_test *ct)
{
	/* activate hw waiting for SOF trigger */
	if (ct->disp_path < 2) {
		mtk_p2s_start(ct->dev_p2s, NULL);
		mtk_resizer_start(ct->dev_resizer, NULL);
	}

	mtk_wdma_start(ct->dev_wdma, NULL);
	mtk_crop_start(ct->dev_crop, NULL);

	return 0;
}
EXPORT_SYMBOL(mtk_cinematic_start);

int mtk_cinematic_stop(struct mtk_cinematic_test *ct)
{
	ct->dp_video_set = false;
	ct->is_3d_tag = false;
	mtk_mutex_disable(ct->mutex, NULL);
	mtk_mutex_delay_disable(ct->mutex_delay, NULL);
	mtk_wdma_reset(ct->dev_wdma);
	mtk_wdma_stop(ct->dev_wdma, NULL);
	mtk_wdma_pvric_enable(ct->dev_wdma, false);
	mtk_crop_stop(ct->dev_crop, NULL);
	mtk_slcr_stop(ct->dev_slicer, NULL);

	if (ct->para.input_from_dp == 0)
		mtk_slcr_patgen_stop(ct->dev_slicer, NULL);

	if (ct->disp_path < 2) {
		mtk_p2s_stop(ct->dev_p2s, NULL);
		mtk_resizer_stop(ct->dev_resizer, NULL);
	}

	memset(&ct->dp_info, 0, sizeof(ct->dp_info));
	mtk_reset_output_port_buf(ct);
	mtk_cinematic_path_disconnect(ct);
	mtk_cinematic_power_off(ct);

	return 0;
}
EXPORT_SYMBOL(mtk_cinematic_stop);

int mtk_cinematic_dual_stop(struct mtk_cinematic_test **ct)
{
	struct mtk_cinematic_test *ct_m;
	u32 i;

	ct_m = ct[MAIN_PIPE];

	ct_m->dp_video_set = false;
	ct_m->is_3d_tag = false;
	mtk_mutex_disable(ct_m->mutex, NULL);
	mtk_slcr_stop(ct_m->dev_slicer, NULL);
		if (ct_m->para.input_from_dp == 0)
			mtk_slcr_patgen_stop(ct_m->dev_slicer, NULL);

	for (i = 0; i < DUAL_PIPE; i++) {
		mtk_wdma_reset(ct[i]->dev_wdma);
		mtk_wdma_stop(ct[i]->dev_wdma, NULL);
		mtk_crop_stop(ct[i]->dev_crop, NULL);
		mtk_p2s_stop(ct[i]->dev_p2s, NULL);
		mtk_resizer_stop(ct[i]->dev_resizer, NULL);
	}

	mtk_wdma_pvric_enable(ct_m->dev_wdma, false);
	mtk_reset_output_port_buf(ct_m);
	mtk_cinematic_dual_path_disconnect(ct);

	ct_m->cur_fb_id = 0;
	for (i = 0; i < DUAL_PIPE; i++)
		mtk_cinematic_power_off(ct[i]);

	return 0;
}
EXPORT_SYMBOL(mtk_cinematic_dual_stop);

int mtk_cinematic_quad_stop(struct mtk_cinematic_test **ct)
{
	struct mtk_cinematic_test *ct_m;
	u32 i;

	ct_m = ct[MAIN_PIPE];

	ct_m->dp_video_set = false;
	mtk_mutex_disable(ct_m->mutex, NULL);

	mtk_slcr_stop(ct_m->dev_slicer, NULL);
	if (ct_m->para.input_from_dp == 0)
		mtk_slcr_patgen_stop(ct_m->dev_slicer, NULL);

	for (i = 0; i < QUADRUPLE_PIPE; i++) {
		mtk_wdma_reset(ct[i]->dev_wdma);
		mtk_wdma_stop(ct[i]->dev_wdma, NULL);
		mtk_crop_stop(ct[i]->dev_crop, NULL);
		if (ct[i]->disp_path < 2) {
			mtk_p2s_stop(ct[i]->dev_p2s, NULL);
			mtk_resizer_stop(ct[i]->dev_resizer, NULL);
		}
	}

	mtk_reset_output_port_buf(ct_m);
	mtk_cinematic_quad_path_disconnect(ct);

	ct_m->cur_fb_id = 0;
	for (i = 0; i < QUADRUPLE_PIPE; i++)
		mtk_cinematic_power_off(ct[i]);

	return 0;
}
EXPORT_SYMBOL(mtk_cinematic_quad_stop);

void mtk_cinematic_test_cb(struct mtk_mmsys_cb_data *data)
{
	struct mtk_cinematic_test *ct =
		(struct mtk_cinematic_test *)data->priv_data;

	dev_dbg(ct->dev,
		"[wdma_cb] data.part_id: %d, data.status: %d\n",
		data->part_id, data->status);

	complete(&ct->wdma_done);
}

static int mtk_cinematic_test_init_wait(struct mtk_cinematic_test *ct)
{
	reinit_completion(&ct->wdma_done);

	mtk_wdma_register_cb(ct->dev_wdma, mtk_cinematic_test_cb,
				MTK_WDMA_FRAME_COMPLETE, ct);
#if 0
	mtk_wdma_register_cb(ct->dev_wdma, mtk_cinematic_test_cb,
			     ct->underrun ? MTK_WDMA_FRAME_UNDERRUN :
			     MTK_WDMA_FRAME_COMPLETE, ct);
#endif
	return 0;
}

static int mtk_cinematic_wait_done(struct mtk_cinematic_test *ct)
{
	if (!wait_for_completion_timeout(&ct->wdma_done,
				msecs_to_jiffies(500))) {
		dev_err(ct->dev,
			"!!!! wait_for_completion_timeout !!!!\n");
	} else {
		dev_dbg(ct->dev, "@@@ wait wdma done is ok!\n");
	}

	/* clear */
	mtk_wdma_register_cb(ct->dev_wdma, mtk_cinematic_test_cb, 0, ct);

	return 0;
}

int mtk_cinematic_trigger(struct mtk_cinematic_test *ct)
{
	dev_dbg(ct->dev, "@@@ wait wdma done...\n");
	mtk_mutex_enable(ct->mutex, NULL);
	mtk_cinematic_wait_done(ct);

	mtk_cinematic_stop(ct);

	return 0;
}
EXPORT_SYMBOL(mtk_cinematic_trigger);

int mtk_cinematic_trigger_no_wait(struct mtk_cinematic_test *ct)
{
	dev_dbg(ct->dev, "@@@ trigger nowait wdma done...\n");
	mtk_mutex_enable(ct->mutex, NULL);

	return 0;
}
EXPORT_SYMBOL(mtk_cinematic_trigger_no_wait);

int  mtk_cinematic_set_buf_mode(struct mtk_cinematic_test *ct, int using_ion)
{
	ct->output_port.using_ion_buf = using_ion;
	return 0;
}
EXPORT_SYMBOL(mtk_cinematic_set_buf_mode);

int mtk_cinematic_set_ion_buf(struct mtk_cinematic_test *ct,
			      struct mtk_common_buf *buf,
			      int buf_idx)
{
	if (!ct->output_port.using_ion_buf)
		return -EINVAL;

	ct->output_port.buf[buf_idx].ion_buf = buf;
	if (buf)
		ct->inserted_buf++;
	else
		ct->inserted_buf--;

	return 0;
}
EXPORT_SYMBOL(mtk_cinematic_set_ion_buf);

static dma_addr_t mtk_cinematic_get_empty_fb(struct mtk_cinematic_test *ct)
{
	dma_addr_t pa;
#if 0
	u32 i;

	for (i = 0; i < WDMA_BUF; i++)
		if (ct->output_port.buf[i].ref_count == 0)
			break;

	if (i < WDMA_BUF) {
		ct->output_port.buf[i].ref_count = 1;
		ct->cur_fb_id = i;
	} else {
		dev_dbg(ct->dev, "buffer occupied by GPU\n");
	}
	ct->output_port.buf[0].ref_count = 1;
#endif

	if (!ct->output_port.using_ion_buf) {

		ct->cur_fb_id %= BYP_GPU_WDMA_BUF;
		pa = ct->output_port.buf[ct->cur_fb_id].k_buf->pa;

	} else {
		ct->cur_fb_id %= ct->inserted_buf;
		pa = ct->output_port.buf[ct->cur_fb_id].ion_buf->dma_addr;
	}

	/*dev_dbg(ct->dev, "get_empty_fb_pa: %llx\n", pa);*/
	return pa;
}

int mtk_cinematic_fb_to_gpu(struct mtk_cinematic_test *ct, u32 given_fb)
{
	u32 last_fb;

	mutex_lock(&ct->fb_lock);

	last_fb = ct->gpu_fb_id;
	ct->gpu_fb_id = given_fb;

	/* return buffer */
	if (last_fb < ct->inserted_buf) {
		if (last_fb != given_fb)
			ct->output_port.buf[last_fb].ref_count = 0;
		else
			LOG_FB("repeat %d %d", last_fb, given_fb);
	}

	mutex_unlock(&ct->fb_lock);
	return 0;
}
EXPORT_SYMBOL(mtk_cinematic_fb_to_gpu);

dma_addr_t mtk_cinematic_get_next_fb(struct mtk_cinematic_test *ct)
{
	dma_addr_t pa;

	mutex_lock(&ct->fb_lock);

	ct->next_fb_id++;
	if (!ct->output_port.using_ion_buf) {
		ct->next_fb_id %= BYP_GPU_WDMA_BUF;
		pa = ct->output_port.buf[ct->next_fb_id].k_buf->pa;
	} else {
		ct->next_fb_id %= ct->inserted_buf;
		pa = ct->output_port.buf[ct->next_fb_id].ion_buf->dma_addr;
	}
	ct->done_fb_id = ct->cur_fb_id;

	mutex_unlock(&ct->fb_lock);

	return pa;
}

int mtk_cinematic_get_disp_fb(struct mtk_cinematic_test *ct,
			      struct mtk_disp_fb *fb)
{
	u32 act_id;
	/*u32 i;*/

	mutex_lock(&ct->fb_lock);

	act_id = ct->done_fb_id;

	/* in quad case, gpu processing time is not enough, ignore check */
	if (!ct->para.quadruple_pipe) {
		/* only if input fps > disp_fps */
		if (ct->gpu_fb_id == ct->cur_fb_id)
			dev_err(ct->dev, "gpu_fb == cur_fb [%d]!!\n",
				ct->gpu_fb_id);
	}

	/* set current to disp_fb */
	if (!ct->output_port.using_ion_buf)
		fb->pa = ct->output_port.buf[act_id].k_buf->pa;
	else
		fb->pa = ct->output_port.buf[act_id].ion_buf->dma_addr;

#if 0
	/* release previous disp buf do not taken by gpu */
	/* make sure gpu alwasy get latest disp buf */
	if (ct->gpu_fb_id < ct->inserted_buf)
		for (i = 0; i < ct->inserted_buf; i++)
			if (ct->output_port.buf[i].ref_count && i != act_id)
				if (ct->gpu_fb_id != i)
					ct->output_port.buf[i].ref_count = 0;
#endif
	fb->fb_id = act_id;

	mutex_unlock(&ct->fb_lock);
	return 0;
}
EXPORT_SYMBOL(mtk_cinematic_get_disp_fb);

int mtk_cinematic_swap_buf(struct mtk_cinematic_test *ct)
{
#if 0
	ct->cur_fb_id++;
	ct->cur_fb_id %= ct->inserted_buf;
#endif
	ct->cur_fb_id = ct->next_fb_id;
	return 0;
}
EXPORT_SYMBOL(mtk_cinematic_swap_buf);

static int mtk_cinematic_get_buf(struct mtk_cinematic_test *ct)
{
	int ret = 0;
	int i = 0;
	int buf_num = 0;

	if (!ct->output_port.using_ion_buf && !ct->buffer_allocated) {

		if (!ct->para.gp.bypass) {
			dev_dbg(ct->dev, "No ION buffer with GPU mode!!!\n");
			return -EINVAL;
		}
		for (i = 0; i < BYP_GPU_WDMA_BUF; i++) {
			ret = mtk_util_buf_create(ct->dev_wdma,
					ct->output_port.buf_size,
					16, &ct->output_port.buf[i].k_buf->va,
					&ct->output_port.buf[i].k_buf->pa);
			if (unlikely(ret))
				goto buf_alloc_fail;
		}
		ct->buffer_allocated = true;

	} else if (ct->output_port.using_ion_buf) {
		dev_dbg(ct->dev, "### kernel using ion, check ion buffer");
		if (!ct->inserted_buf)
			ret = -ENOMEM;
	}

	return ret;

buf_alloc_fail:
	for (buf_num = 0; buf_num < i; buf_num++) {
		mtk_util_buf_destroy(ct->dev_wdma,
				ct->output_port.buf_size,
				16, ct->output_port.buf[buf_num].k_buf->va,
				ct->output_port.buf[buf_num].k_buf->pa);
	}
	return ret;
}

static int mtk_cinematic_init(struct mtk_cinematic_test *ct)
{
	int max_w, max_h, max_pitch, bpp, buf_size;

	int ret = 0;

	init_completion(&ct->wdma_done);

	/* cinematic mode front end using 0-7 */
	ct->mutex = mtk_mutex_get(ct->dev_mutex, ct->disp_path);

	if (IS_ERR(ct->mutex)) {
		ret = PTR_ERR(ct->mutex);
		dev_err(ct->dev, "Failed to get mutex res[0]: %d\n", ret);
		return ret;
	}

	ct->mutex_delay = mtk_mutex_delay_get(ct->dev_mutex, ct->disp_path +
					      DISPSYS_DELAY_NUM);

	if (IS_ERR(ct->mutex_delay)) {
		ret = PTR_ERR(ct->mutex_delay);
		dev_err(ct->dev, "Failed to get mutex delay[0]: %d\n", ret);
		return ret;
	}

	/* allocate max size for single pipe */
	max_w = 1920;
	max_h = 2400;
	bpp = 32;
	max_pitch = roundup(max_w * bpp / 8, 16);
	buf_size = max_pitch * max_h;

	/* TODO USER */
	ct->output_port.buf_size = buf_size;
	/* assign correlated component accroding to path */
	switch (ct->disp_path) {
	case 0:
		ct->disp_mmsys.crop_id = MMSYSCFG_COMPONENT_CROP0;
		ct->disp_mmsys.rsz_id = MMSYSCFG_COMPONENT_RSZ0;
		ct->disp_mmsys.wdma_id = MMSYSCFG_COMPONENT_DISP_WDMA0;

		ct->disp_mutex.wdma_id = MUTEX_COMPONENT_DISP_WDMA0;
		ct->disp_mutex.rsz_id = MUTEX_COMPONENT_RSZ0;
		ct->disp_mutex.crop_id = MUTEX_COMPONENT_CROP0;
		break;
	case 1:
		ct->disp_mmsys.crop_id = MMSYSCFG_COMPONENT_CROP1;
		ct->disp_mmsys.rsz_id = MMSYSCFG_COMPONENT_RSZ1;
		ct->disp_mmsys.wdma_id = MMSYSCFG_COMPONENT_DISP_WDMA1;

		ct->disp_mutex.wdma_id = MUTEX_COMPONENT_DISP_WDMA1;
		ct->disp_mutex.rsz_id = MUTEX_COMPONENT_RSZ1;
		ct->disp_mutex.crop_id = MUTEX_COMPONENT_CROP1;
		break;
	case 2:
		ct->disp_mmsys.crop_id = MMSYSCFG_COMPONENT_CROP2;
		ct->disp_mmsys.wdma_id = MMSYSCFG_COMPONENT_DISP_WDMA2;

		ct->disp_mutex.wdma_id = MUTEX_COMPONENT_DISP_WDMA2;
		ct->disp_mutex.crop_id = MUTEX_COMPONENT_CROP2;
		break;
	case 3:
		ct->disp_mmsys.crop_id = MMSYSCFG_COMPONENT_CROP3;
		ct->disp_mmsys.wdma_id = MMSYSCFG_COMPONENT_DISP_WDMA3;

		ct->disp_mutex.wdma_id = MUTEX_COMPONENT_DISP_WDMA3;
		ct->disp_mutex.crop_id = MUTEX_COMPONENT_CROP3;
		break;
	default:
		dev_dbg(ct->dev, "disp_path error\n");
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int mtk_single_path_reset(struct mtk_cinematic_test *ct, bool dual_mode)
{
	int err;
	int i;
	struct wdma_buffer *buf;

	dev_dbg(ct->dev, "### single path reset\n");
	/* reset module so that we can repeat */
	err = mtk_reset_core_modules(ct);

	if (err) {
		dev_err(ct->dev, "failed to reset hw-number modules %d\n", err);
		return err;
	}

	memset(ct->slcr_cfg, 0, sizeof(struct slcr_config));
	memset(ct->slcr_pg_cfg, 0, sizeof(struct  slcr_pg_config));

	if (ct->disp_path > 0 && dual_mode)
		goto PATH_RESET_DONE;

	if (!ct->buffer_allocated && !ct->inserted_buf) {
		dev_dbg(ct->dev, "buffer not ready, do NOT reset\n");
		return -EINVAL;
	}

	/* reset dram , confirm coherent or not to determine flush cache !! */
	if (!ct->output_port.using_ion_buf) {

		for (i = 0; i < BYP_GPU_WDMA_BUF; i++) {
			buf = &ct->output_port.buf[i];
			memset(buf->k_buf->va, 0, ct->output_port.buf_size);
			dma_sync_single_for_device(ct->dev_wdma,
					buf->k_buf->pa,
					ct->output_port.buf_size,
					DMA_TO_DEVICE);
		}
	} else {

		for (i = 0; i < ct->inserted_buf; i++) {
			buf = &ct->output_port.buf[i];

			buf->ion_buf->kvaddr = pa_to_vaddr(buf->ion_buf,
					ct->image_output_size, true);

			memset(buf->ion_buf->kvaddr, 0, ct->image_output_size);

			dma_sync_sg_for_device(ct->dev_wdma,
					buf->ion_buf->sg->sgl,
					buf->ion_buf->sg->nents, DMA_TO_DEVICE);

			buf->ion_buf->kvaddr = pa_to_vaddr(buf->ion_buf,
					ct->image_output_size, false);
		}
	}
PATH_RESET_DONE:
	dev_dbg(ct->dev, "### single path reset  ### done\n");
	return 0;
}

static int pat_sel(int pat_sel, int direction)
{
	int value = 2;
	static int inc_val;
	static int phase = 1;

	if (pat_sel == 0) /* vertical bar */
		value = ((direction == 1) ? 0 : 256);
	else if (pat_sel == 1) /* horizontal bar */
		value = ((direction == 1) ? 256 : 0);
	else if (pat_sel == 2) /* color bar */
		value = 256;
	else if (pat_sel == 3) /* color bar */
		value = inc_val;
	else
		pr_debug("Please select pattern between 0 ~ 2!\n");

	if (phase)
		inc_val++;
	else
		inc_val--;

	if (inc_val == 256)
		phase = 0;
	else if (inc_val == 0)
		phase = 1;

	return value;
}

static struct twin_algo_module_para *mtk_get_twin(struct mtk_cinematic_test *ct,
						  enum twin_algo_module_type tp)
{
	u32 i;

	for (i = 0; i < CINEMATIC_MODULE_CNT ; i++) {
		if (ct->para.cinematic_cfg[i].type == tp)
			return &ct->para.cinematic_cfg[i];
	}
	return NULL;
}


static int mtk_set_slicer_patgen_fps(const struct device *dev,
			struct mtk_cinematic_test *ct)
{
	int ret = 0;

#ifdef CONFIG_MACH_MT3612_A0
	u16 h_total = 0;
	u16 v_total = 0;

	if (ct->slcr_pg_cfg->pg_timing != PAT_EXT) {
		ret = mtk_slcr_patgen_get_frame_status(dev,
				&ct->slcr_pg_cfg->pg_frame_cfg);
		if (unlikely(ret)) {
			dev_dbg(ct->dev, "get frame status NG!!\n");
			return ret;
		}
	}

	h_total = ct->slcr_pg_cfg->pg_frame_cfg.hsync
		+ ct->slcr_pg_cfg->pg_frame_cfg.h_back
		+ ct->slcr_pg_cfg->pg_frame_cfg.h_active
		+ ct->slcr_pg_cfg->pg_frame_cfg.h_front;
	v_total = ct->slcr_pg_cfg->pg_frame_cfg.vsync
		+ ct->slcr_pg_cfg->pg_frame_cfg.v_back
		+ ct->slcr_pg_cfg->pg_frame_cfg.v_active
		+ ct->slcr_pg_cfg->pg_frame_cfg.v_front;

	dev_dbg(ct->dev, "h_total=%d, v_total=%d.\n", h_total, v_total);

	ret = mtk_slcr_patgen_fps_config(dev, h_total, v_total,
					ct->slcr_pg_cfg->pg_fps);
#endif
	return ret;
}

#ifdef CONFIG_MACH_MT3612_A0
int mtk_get_fps_ratio(u32 *ratio, u32 size, u32 i_fps, u32 o_fps)
{
	u32 common_divisor;

	if (size < 2)
		return -EINVAL;

	common_divisor  = gcd(i_fps, o_fps);
	ratio[0] = i_fps / common_divisor;
	ratio[1] = o_fps / common_divisor;

	pr_debug("ratio[0] = %d, ratio[1] = %d\n", ratio[0], ratio[1]);
	return 0;
}
EXPORT_SYMBOL(mtk_get_fps_ratio);

static int mtk_slicer_get_gce_height(struct mtk_cinematic_test *ct,
				     u16 *gce_height, u32 vsync_width)
{
	u32 i_fps, o_fps, h, dp_vtotal;
	u32 ratio[RATIO_SIZE] = {0};
	int ret = 0;

	if (IS_ERR(ct))
		return -EINVAL;

	/* input and output fps */
	o_fps = ct->para.disp_fps;
	i_fps = ct->para.input_fps;
	dp_vtotal = ct->dp_info.vid_timing_msa.v_total;

	ret = mtk_get_fps_ratio(ratio, RATIO_SIZE, i_fps, o_fps);
	if (unlikely(ret)) {
		dev_dbg(ct->dev, "get fps ratio NG!!\n");
		return ret;
	}

	dev_dbg(ct->dev, "dp_vtotal =  %d\n", dp_vtotal);
	dev_dbg(ct->dev, "ct->lcm_vtotal =  %d\n", ct->lcm_vtotal);

	h = (dp_vtotal * 1000 * ratio[0]) / (ct->lcm_vtotal * ratio[1]);
	dev_dbg(ct->dev, "h1 =  %d\n", h);
	h = (h * ct->sof_line_diff) / 1000;
	dev_dbg(ct->dev, "h2 =  %d\n", h);
	dev_dbg(ct->dev, "vsync_width  %d\n", vsync_width);
	/*h *= (ct->lcm_vtotal - ct->frmtrk_target_line) / 100;*/

	dev_dbg(ct->dev, "h3 =  %d\n", h);
	*gce_height = h - vsync_width;

	dev_dbg(ct->dev, "slicer gce_h = %d\n", *gce_height);
	return 0;
}

int mtk_cinematic_set_frmtrk_height(struct mtk_cinematic_test *ct)
{
	int ret = 0;

	/* caculate slicer line event */
	u16 gce_height;
	u16 dp_vtotal, vsync;

	dp_vtotal = ct->dp_info.vid_timing_msa.v_total;
	vsync = ct->dp_info.vid_timing_msa.v_sync_width;

	ct->slcr_cfg->dp_video.gce.event = GCE_INPUT;
	ct->slcr_cfg->dp_video.gce.width = 1;

	mtk_slicer_get_gce_height(ct, &gce_height, vsync);
	ct->slcr_cfg->dp_video.gce.height = gce_height;

	ret = mtk_slcr_config(ct->dev_slicer, NULL, ct->slcr_cfg);
	if (unlikely(ret)) {
		dev_err(ct->dev, "mtk_slcr_config for frmtrk NG!!\n");
		return ret;
	}
	return ret;
}
EXPORT_SYMBOL(mtk_cinematic_set_frmtrk_height);
#endif

static int mtk_slicer_get_output_width(struct slcr_config *cfg,
					struct mtk_cinematic_test *ct)
{
	u32 i;
	int out_w = 0;
	u8 vid_en;

	vid_en = cfg->dp_video.output.vid_en[ct->disp_path];
	for (i = 0; i < 4; i++) {
		if (vid_en & 0x1) {
			out_w += (cfg->dp_video.output.slice_eop[i] -
				cfg->dp_video.output.slice_sop[i]);
		}
		vid_en >>= 1;
	}
	return out_w;
}

static void mtk_slicer_cfg(struct slcr_config *cfg,
			   struct slcr_pg_config *pg_cfg,
			   struct mtk_cinematic_test *ct)
{
	int unit = 0;
	int i = 0;
	int vblocks = 40;
	int hblocks = 40;
	int pattern = 2;
	int ret = 0;
	struct twin_algo_module_para *t_cfg;

#if RUN_BIT_TRUE
	int is_reshape = (((ct->para.pvric_en & COMPRESS_INPUT_ENABLE) &&
			(ct->w == 1920) && (ct->h == 2205) &&
			(ct->para.cp.out_h == 2176))) ||
			((ct->para.pvric_en & COMPRESS_INPUT_ENABLE)  &&
			(ct->w == 1280) && (ct->h == 1470));
#else
	int is_reshape = (ct->w == 1920 && ct->h == 2205) ||
			 (ct->w == 1280 && ct->h == 1470);
#endif

	ret = mtk_cinematic_cfg_fps_to_slcr_fps(ct, &pg_cfg->pg_fps);
	if (unlikely(ret))
		dev_err(ct->dev, "cfg_fps_to_slcr_fps NG!!\n");

	/* SOF type selection */
	if (ct->para.sof_sel == CONTINUOUS_SOF)
		pg_cfg->pg_mode = FREE_RUN;
	else
		pg_cfg->pg_mode = HW_TRIG;

	if ((ct->w == 640) && (ct->h == 480)) {
		pg_cfg->pg_timing = PAT_640_480;
		vblocks += 8;
	} else if ((ct->w == 960) && (ct->h == 2160)) {
		pg_cfg->pg_timing = PAT_EXT;
		vblocks += 176;
		hblocks += 20;
		pg_cfg->pg_frame_cfg.vsync_pol = 0;
		pg_cfg->pg_frame_cfg.hsync_pol = 0;
		pg_cfg->pg_frame_cfg.vsync = 8;
		pg_cfg->pg_frame_cfg.hsync = 44;
		pg_cfg->pg_frame_cfg.v_back = 72;
		pg_cfg->pg_frame_cfg.h_back = 148;
		pg_cfg->pg_frame_cfg.v_active = 2160;
		pg_cfg->pg_frame_cfg.h_active = 960;
		pg_cfg->pg_frame_cfg.v_front = 8;
		pg_cfg->pg_frame_cfg.h_front = 1048;
	} else if (ct->w == 1280 && (ct->h == 720)) {
		pg_cfg->pg_timing = PAT_1280_720;
		vblocks += 32;
		hblocks += 40;
	} else if ((ct->w == 1280) && (ct->h == 1470)) {
		pg_cfg->pg_timing = PAT_EXT;
		vblocks += 64;
		hblocks += 40;
		pg_cfg->pg_frame_cfg.vsync_pol = 1;
		pg_cfg->pg_frame_cfg.hsync_pol = 1;
		if (pg_cfg->pg_fps == PAT_50_FPS) {
			pg_cfg->pg_frame_cfg.vsync = 5;
			pg_cfg->pg_frame_cfg.hsync = 40;
			pg_cfg->pg_frame_cfg.v_back = 20;
			pg_cfg->pg_frame_cfg.h_back = 220;
			pg_cfg->pg_frame_cfg.v_active = 1470;
			pg_cfg->pg_frame_cfg.h_active = 1280;
			pg_cfg->pg_frame_cfg.v_front = 5;
			pg_cfg->pg_frame_cfg.h_front = 440;
		} else if (pg_cfg->pg_fps == PAT_59_94_FPS) {
			pg_cfg->pg_frame_cfg.vsync = 5;
			pg_cfg->pg_frame_cfg.hsync = 40;
			pg_cfg->pg_frame_cfg.v_back = 20;
			pg_cfg->pg_frame_cfg.h_back = 220;
			pg_cfg->pg_frame_cfg.v_active = 1470;
			pg_cfg->pg_frame_cfg.h_active = 1280;
			pg_cfg->pg_frame_cfg.v_front = 5;
			pg_cfg->pg_frame_cfg.h_front = 110;
		}
		if (is_reshape) {
			/** 3D reshaped */
			cfg->dp_video.data_path.rs_en = 1;
			cfg->dp_video.data_path.rs_mode = 1;
			cfg->dp_video.data_path.vsync = 5;
			cfg->dp_video.data_path.v_front = 5;
			cfg->dp_video.data_path.v_active = 720;
			cfg->dp_video.data_path.v_back = 20;

			/** Dummy line */
			cfg->dp_video.output.dyml[0].num = 8;
			cfg->dp_video.output.dyml[1].num = 8;
			cfg->dp_video.output.dyml[2].num = 8;
			cfg->dp_video.output.dyml[3].num = 8;
		}
	} else if (ct->w == 1920 && (ct->h == 1080)) {
		vblocks += 68;
		hblocks += 80;
		if (pg_cfg->pg_fps == PAT_23_976_FPS) {
			pg_cfg->pg_timing = PAT_EXT;
			pg_cfg->pg_frame_cfg.vsync_pol = 1;
			pg_cfg->pg_frame_cfg.hsync_pol = 1;
			pg_cfg->pg_frame_cfg.vsync = 5;
			pg_cfg->pg_frame_cfg.hsync = 44;
			pg_cfg->pg_frame_cfg.v_back = 36;
			pg_cfg->pg_frame_cfg.h_back = 148;
			pg_cfg->pg_frame_cfg.v_active = 1080;
			pg_cfg->pg_frame_cfg.h_active = 1920;
			pg_cfg->pg_frame_cfg.v_front = 4;
			pg_cfg->pg_frame_cfg.h_front = 638;
		} else if (pg_cfg->pg_fps == PAT_50_FPS) {
			pg_cfg->pg_timing = PAT_EXT;
			pg_cfg->pg_frame_cfg.vsync_pol = 1;
			pg_cfg->pg_frame_cfg.hsync_pol = 1;
			pg_cfg->pg_frame_cfg.vsync = 5;
			pg_cfg->pg_frame_cfg.hsync = 44;
			pg_cfg->pg_frame_cfg.v_back = 36;
			pg_cfg->pg_frame_cfg.h_back = 148;
			pg_cfg->pg_frame_cfg.v_active = 1080;
			pg_cfg->pg_frame_cfg.h_active = 1920;
			pg_cfg->pg_frame_cfg.v_front = 4;
			pg_cfg->pg_frame_cfg.h_front = 528;
		} else if (pg_cfg->pg_fps == PAT_59_94_FPS ||
			 pg_cfg->pg_fps == PAT_119_88_FPS) {
			pg_cfg->pg_timing = PAT_1920_1080;
		}
	} else if ((ct->w == 1920) && (ct->h == 2205)) {
		pg_cfg->pg_timing = PAT_EXT;
		vblocks += 136;
		hblocks += 80;
		pg_cfg->pg_frame_cfg.vsync_pol = 1;
		pg_cfg->pg_frame_cfg.hsync_pol = 1;
		pg_cfg->pg_frame_cfg.vsync = 5;
		pg_cfg->pg_frame_cfg.hsync = 44;
		pg_cfg->pg_frame_cfg.v_back = 36;
		pg_cfg->pg_frame_cfg.h_back = 148;
		pg_cfg->pg_frame_cfg.v_active = 2205;
		pg_cfg->pg_frame_cfg.h_active = 1920;
		pg_cfg->pg_frame_cfg.v_front = 4;
		pg_cfg->pg_frame_cfg.h_front = 638;

		if (is_reshape) {
			/** 3D reshaped */
			cfg->dp_video.data_path.rs_en = 1;
			cfg->dp_video.data_path.rs_mode = 1;
			cfg->dp_video.data_path.vsync = 5;
			cfg->dp_video.data_path.v_front = 4;
			cfg->dp_video.data_path.v_active = 1080;
			cfg->dp_video.data_path.v_back = 36;

			/** Dummy line */
			cfg->dp_video.output.dyml[0].num = 8;
			cfg->dp_video.output.dyml[1].num = 8;
			cfg->dp_video.output.dyml[2].num = 8;
			cfg->dp_video.output.dyml[3].num = 8;
		}
	} else if (ct->w == 3840 && (ct->h == 2160)) {
		pg_cfg->pg_timing = PAT_3840_2160;
		vblocks += 176;
		hblocks += 200;
	} else if (ct->w == 4000 && (ct->h == 2040)) {
		pg_cfg->pg_timing = PAT_EXT;
		vblocks += 164;
		hblocks += 210;
		pg_cfg->pg_frame_cfg.vsync_pol = 1;
		pg_cfg->pg_frame_cfg.hsync_pol = 1;
		pg_cfg->pg_frame_cfg.vsync = 10;
		pg_cfg->pg_frame_cfg.hsync = 44;
		pg_cfg->pg_frame_cfg.v_back = 142;
		pg_cfg->pg_frame_cfg.h_back = 368;
		pg_cfg->pg_frame_cfg.v_active = 2040;
		pg_cfg->pg_frame_cfg.h_active = 4000;
		pg_cfg->pg_frame_cfg.v_front = 8;
		pg_cfg->pg_frame_cfg.h_front = 88;
	} else if (ct->w == 1080) {
		vblocks += 80;
		hblocks += 68;
		pg_cfg->pg_timing = PAT_EXT;
		pg_cfg->pg_frame_cfg.vsync_pol = 0;
		pg_cfg->pg_frame_cfg.hsync_pol = 0;
		pg_cfg->pg_frame_cfg.vsync = 0x20;
		pg_cfg->pg_frame_cfg.hsync = 0x40;
		pg_cfg->pg_frame_cfg.v_back = 0x20;
		pg_cfg->pg_frame_cfg.h_back = 0x40;
		pg_cfg->pg_frame_cfg.v_active = 0x780;
		pg_cfg->pg_frame_cfg.h_active = 0x438;
		pg_cfg->pg_frame_cfg.v_front = 0x20;
		pg_cfg->pg_frame_cfg.h_front = 0x40;
	}

	if (ct->disp_path == 0) {
		pattern = ct->para.pat_sel;
		pg_cfg->pg_pat_cfg.color_b.h_addend = pat_sel(pattern, 0);
		pg_cfg->pg_pat_cfg.color_g.v_addend = pat_sel(pattern, 1);
		#if 1
		pg_cfg->pg_pat_cfg.color_b.cbar_width = hblocks;
		pg_cfg->pg_pat_cfg.color_g.cbar_height = vblocks;
		#else
		pg_cfg->pg_pat_cfg.color_r.cbar_width = 128;
		pg_cfg->pg_pat_cfg.color_r.cbar_height = 72;
		pg_cfg->pg_pat_cfg.color_r.init_value = 1024;
		pg_cfg->pg_pat_cfg.color_r.h_addend = 2048;
		pg_cfg->pg_pat_cfg.color_r.v_addend = 2048;

		pg_cfg->pg_pat_cfg.color_g.cbar_width = 128;
		pg_cfg->pg_pat_cfg.color_g.cbar_height = 72;
		pg_cfg->pg_pat_cfg.color_g.init_value = 1024;
		pg_cfg->pg_pat_cfg.color_g.h_addend = 2048;
		pg_cfg->pg_pat_cfg.color_g.v_addend = 2048;

		pg_cfg->pg_pat_cfg.color_b.cbar_width = 128;
		pg_cfg->pg_pat_cfg.color_b.cbar_height = 72;
		pg_cfg->pg_pat_cfg.color_b.init_value = 1024;
		pg_cfg->pg_pat_cfg.color_b.h_addend = 2048;
		pg_cfg->pg_pat_cfg.color_b.v_addend = 2048;
		#endif
	} else if (ct->disp_path == 1) {
		pg_cfg->pg_pat_cfg.color_r.h_addend = 256;
		pg_cfg->pg_pat_cfg.color_g.v_addend = 256;
		pg_cfg->pg_pat_cfg.color_r.cbar_width = hblocks;
		pg_cfg->pg_pat_cfg.color_g.cbar_height = vblocks;
	} else if (ct->disp_path == 2) {
		pg_cfg->pg_pat_cfg.color_r.h_addend = 256;
		pg_cfg->pg_pat_cfg.color_b.v_addend = 256;
		pg_cfg->pg_pat_cfg.color_r.cbar_width = hblocks;
		pg_cfg->pg_pat_cfg.color_b.cbar_height = vblocks;
	} else if (ct->disp_path == 3) {
		pg_cfg->pg_pat_cfg.color_g.h_addend = 256;
		pg_cfg->pg_pat_cfg.color_b.v_addend = 256;
		pg_cfg->pg_pat_cfg.color_g.cbar_width = hblocks;
		pg_cfg->pg_pat_cfg.color_b.cbar_height = vblocks;
	}

	/* slicer settings */
	cfg->in_format = DP_VIDEO;

	if (is_reshape && (ct->w == 1280) && (ct->h == 1470)) {
		cfg->dp_video.input.width = 1280;
		cfg->dp_video.input.height = 720;
	} else if (is_reshape && (ct->w == 1920) && (ct->h == 2205)) {
		cfg->dp_video.input.width = 1920;
		cfg->dp_video.input.height = 1080;
	} else {
		cfg->dp_video.input.width = ct->w;
		cfg->dp_video.input.height = ct->h;
	}

	cfg->dp_video.output.sync_port = ORG_PORT;
	cfg->dp_video.input.pxl_swap = NO_SWAP;

	if (ct->para.input_from_dp && (!ct->para.wdma_csc_en) &&
		(ct->output_port.format == DRM_FORMAT_AYUV2101010)) {
		cfg->dp_video.input.in_ch[0] = CHANNEL_2;
		cfg->dp_video.input.in_ch[1] = CHANNEL_0;
		cfg->dp_video.input.in_ch[2] = CHANNEL_1;
	} else {
		cfg->dp_video.input.in_ch[0] = CHANNEL_0;
		cfg->dp_video.input.in_ch[1] = CHANNEL_1;
		cfg->dp_video.input.in_ch[2] = CHANNEL_2;
	}
	cfg->dp_video.output.out_ch[0] = CHANNEL_0;
	cfg->dp_video.output.out_ch[1] = CHANNEL_1;
	cfg->dp_video.output.out_ch[2] = CHANNEL_2;

#if RUN_BIT_TRUE
	cfg->dp_video.csc_en = 0;
	if (ct->dp_info.vid_color_fmt == RGB444) {
		cfg->dp_video.csc_en = 1;
		cfg->dp_video.csc_mode = MTX_FULLSRGB_TO_FULL601;
	}
#endif

#ifdef CONFIG_MACH_MT3612_A0
	if (ct->para.input_from_dp) {
		cfg->dp_video.input.in_hsync_inv =
			!(ct->dp_info.vid_timing_msa.h_polarity);
		cfg->dp_video.input.in_vsync_inv =
			!(ct->dp_info.vid_timing_msa.v_polarity);

		dev_info(ct->dev, "DP input hsync polarity= (%d)\n",
			 ct->dp_info.vid_timing_msa.h_polarity);
		dev_info(ct->dev, "DP input vsync polarity= (%d)\n",
			 ct->dp_info.vid_timing_msa.v_polarity);
		dev_info(ct->dev, "DP input format= (%d)\n",
			 ct->dp_info.vid_color_fmt);
	} else {
		switch (pg_cfg->pg_timing) {
		case PAT_1920_1080:
		case PAT_1280_720:
		case PAT_EXT:
			cfg->dp_video.input.in_hsync_inv = 1;
			cfg->dp_video.input.in_vsync_inv = 1;
			break;
		case PAT_3840_2160:
		case PAT_640_480:
		case PAT_720_480:
		case PAT_720_576:
			cfg->dp_video.input.in_hsync_inv = 0;
			cfg->dp_video.input.in_vsync_inv = 0;
			break;
		case PAT_2560_1440:
			cfg->dp_video.input.in_hsync_inv = 1;
			cfg->dp_video.input.in_vsync_inv = 0;
			break;
		default:
			cfg->dp_video.input.in_hsync_inv = 1;
			cfg->dp_video.input.in_vsync_inv = 1;
		}
	}
#else
	switch (pg_cfg->pg_timing) {
	case PAT_1920_1080:
	case PAT_1280_720:
	case PAT_EXT:
		cfg->dp_video.input.in_hsync_inv = 1;
		cfg->dp_video.input.in_vsync_inv = 1;
		break;
	case PAT_3840_2160:
	case PAT_640_480:
	case PAT_720_480:
	case PAT_720_576:
		cfg->dp_video.input.in_hsync_inv = 0;
		cfg->dp_video.input.in_vsync_inv = 0;
		break;
	case PAT_2560_1440:
		cfg->dp_video.input.in_hsync_inv = 1;
		cfg->dp_video.input.in_vsync_inv = 0;
		break;
	default:
		cfg->dp_video.input.in_hsync_inv = 1;
		cfg->dp_video.input.in_vsync_inv = 1;
	}
#endif
	if (ct->para.sof_sel == CONTINUOUS_SOF) {
		cfg->dp_video.output.out_vsync_inv = 1;
		cfg->dp_video.output.out_hsync_inv = 0;
	}

	unit = ct->w >> 2;

	for (i = 0; i < 4; i++) {
		cfg->dp_video.output.slice_sop[i] = (unit * i);
		cfg->dp_video.output.slice_eop[i] = (unit * (i + 1));
	}
#if 0
						    (previous frame last line)
(60:60)
1:1	slicer gce line event, (1125  / 2200) * 200 - input vsync width;
(60:120)
1:2	slicer gce line event, (1125  / 2200*2) * 200 - input vsync width;
#endif

	if (ct->para.quadruple_pipe) {
		cfg->dp_video.output.vid_en[0] = 0x1;
		cfg->dp_video.output.vid_en[1] = 0x2;
		cfg->dp_video.output.vid_en[2] = 0x4;
		cfg->dp_video.output.vid_en[3] = 0x8;
		for (i = 0; i < 4; i++) {
			ct->lhc_slice_cfg[i].w = unit;
			ct->lhc_slice_cfg[i].h = ct->h;
			ct->lhc_slice_cfg[i].start = 0;
			ct->lhc_slice_cfg[i].end = unit - 1;
		}
	} else if (ct->w >= 3840) {
		int half, ext;

		t_cfg = mtk_get_twin(ct, TAMT_SLICER);
		dev_dbg(ct->dev, "in_end[0] = %d, in_start[1] = %d\n",
			t_cfg->in_end[0], t_cfg->in_start[1]);

		cfg->dp_video.output.slice_eop[1] = t_cfg->in_end[0] + 1;
		cfg->dp_video.output.slice_sop[2] = t_cfg->in_start[1];

		cfg->dp_video.output.vid_en[0] = 0x3;
		cfg->dp_video.output.vid_en[1] = 0xc;

		/* LHC 4 slices configuration parameters */
		/* left half with overlap */
		half = t_cfg->out_end[0] - t_cfg->out_start[0] + 1;
		for (i = 0; i < 2; i++) {
			ct->lhc_slice_cfg[i].w = half;
			ct->lhc_slice_cfg[i].h = ct->h;
			ct->lhc_slice_cfg[i].start = unit * i;
			ct->lhc_slice_cfg[i].end = (unit * (i + 1)) - 1;
		}
		/* right half with overlap */
		half = t_cfg->out_end[1] - t_cfg->out_start[1] + 1;
		/* ext is overlap width from right half to left */
		ext = half - ct->w / 2;
		for (i = 2; i < 4; i++) {
			ct->lhc_slice_cfg[i].w = half;
			ct->lhc_slice_cfg[i].h = ct->h;
			ct->lhc_slice_cfg[i].start = ext + unit * (i - 2);
			ct->lhc_slice_cfg[i].end = ext + (unit * (i - 1)) - 1;
		}
	} else {
		/* lane 0 full frame output  */
		cfg->dp_video.output.vid_en[ct->disp_path] = 0xf;

		/* LHC 4 slices configuration parameters */
		for (i = 0; i < 4; i++) {
			ct->lhc_slice_cfg[i].w = ct->w;
			if (is_reshape) {
				ct->lhc_slice_cfg[i].h =
					cfg->dp_video.data_path.v_active << 1;
				ct->lhc_slice_cfg[i].h +=
					cfg->dp_video.output.dyml[i].num << 1;
			} else {
				ct->lhc_slice_cfg[i].h = ct->h;
			}
			ct->lhc_slice_cfg[i].start = unit * i;
			ct->lhc_slice_cfg[i].end = (unit * (i + 1)) - 1;
		}
		ct->dummy_line = cfg->dp_video.output.dyml[0].num;
	}
}

static int mtk_crop_quad_cfg(struct crop_output cp_out[QUADRUPLE_PIPE],
		u32 in_w, u32 in_h, u32 out_w, u32 out_h,
		u32 crop_x, u32 crop_y)
{
	u16 gt2 = 0;
	int unit = 0;
	int slice_sop[QUADRUPLE_PIPE];
	int slice_eop[QUADRUPLE_PIPE];
	int i;
	int x0, x1;
	int total = 0;

	unit = in_w >> 2;
	for (i = 0; i < QUADRUPLE_PIPE; i++) {
		slice_sop[i] = (unit * i);
		slice_eop[i] = (unit * (i + 1));
		cp_out[i].sop = slice_sop[i];
		cp_out[i].eop = slice_eop[i] - 1;
	}

	x0 = crop_x;
	x1 = crop_x + out_w - 1;

	for (i = 0; i < QUADRUPLE_PIPE; i++) {
		if (x0 >= cp_out[i].sop &&
		x0 <= cp_out[i].eop) {
			cp_out[i].crop_type = CP_HEAD;
			gt2++;
		} else {
			if (gt2 == 0) {
				cp_out[i].crop_type = CP_EMPTY;
			} else {
				cp_out[i].crop_type = CP_MIDDLE;
				gt2++;
			}
		}
		if (x1 >= cp_out[i].sop &&
		x1 <= cp_out[i].eop) {
			if (cp_out[i].crop_type == CP_HEAD) {
				cp_out[i].crop_type = CP_HEAD_REAR;
				gt2 = 0;
			} else {
				cp_out[i].crop_type = CP_REAR;
				gt2 = 0;
			}
		}
	}

	for (i = 0; i < QUADRUPLE_PIPE; i++) {
		if (cp_out[i].crop_type == CP_EMPTY) {
			cp_out[i].x_oft = 0;
			cp_out[i].y_oft = 0;
			cp_out[i].out_w = 0;
			cp_out[i].out_h = 0;
		} else if (cp_out[i].crop_type == CP_HEAD) {
			cp_out[i].x_oft = x0 - cp_out[i].sop;
			cp_out[i].y_oft = crop_y;
			cp_out[i].out_w = cp_out[i].eop - x0 + 1;
			cp_out[i].out_h = out_h;
		} else if (cp_out[i].crop_type == CP_MIDDLE) {
			cp_out[i].x_oft = 0;
			cp_out[i].y_oft = crop_y;
			cp_out[i].out_w = unit;
			cp_out[i].out_h = out_h;
		} else if (cp_out[i].crop_type == CP_REAR) {
			cp_out[i].x_oft = 0;
			cp_out[i].y_oft = crop_y;
			cp_out[i].out_w = x1 - cp_out[i].sop + 1;
			cp_out[i].out_h = out_h;
		} else if (cp_out[i].crop_type == CP_HEAD_REAR) {
			cp_out[i].x_oft = x0 - cp_out[i].sop;
			cp_out[i].y_oft = crop_y;
			cp_out[i].out_w = out_w;
			cp_out[i].out_h = out_h;
		}
		pr_info("x_oft[%d] = %d\n", i, cp_out[i].x_oft);
		pr_info("y_oft[%d] = %d\n", i, cp_out[i].y_oft);
		pr_info("out_w[%d] = %d\n", i, cp_out[i].out_w);
		pr_info("out_h[%d] = %d\n", i, cp_out[i].out_h);
		total += cp_out[i].out_w;
	}

	if (total != out_w)
		return 1;
	else
		return 0;
}

static void mtk_cinematic_dynamic_patgen_cb(struct cmdq_cb_data data)
{
	struct mtk_cinematic_test *ct =
		(struct mtk_cinematic_test *)data.data;

	cmdq_pkt_destroy(ct->cmdq_pkt);
	ct->cmdq_pkt = NULL;
}

static void mtk_cinematic_dynamic_cb(struct cmdq_cb_data data)
{
	struct mtk_cinematic_test *ct =
		(struct mtk_cinematic_test *)data.data;

	ct->seq_chg = true;
	dev_dbg(ct->dev, "dynamic rsz done\n");

	cmdq_pkt_destroy(ct->cmdq_pkt);
	ct->cmdq_pkt = NULL;
}

static void mtk_cinematic_update_addr_cb(struct cmdq_cb_data data)
{
	struct mtk_cinematic_test *ct =
		(struct mtk_cinematic_test *)data.data;

	mtk_cinematic_swap_buf(ct);

	cmdq_pkt_destroy(ct->cmdq_pkt);
	ct->cmdq_pkt = NULL;
}

static int mtk_cinematic_cfg_fps_to_slcr_fps(struct mtk_cinematic_test *ct,
					     enum slcr_pg_fps *slcr_fps)
{
	if (IS_ERR(ct))
		return -EINVAL;

	switch (ct->para.input_fps) {
	case FPS_120:
		/* fall through */
	case FPS_119:
		*slcr_fps = PAT_119_88_FPS;
		break;
	case FPS_90:
		/* fall through */
	case FPS_89:
		/* TBD */
		*slcr_fps = 0;
		break;
	case FPS_60:
		/* fall through */
	case FPS_59:
		*slcr_fps = PAT_59_94_FPS;
		break;
	case FPS_50:
		*slcr_fps = PAT_50_FPS;
		break;
	case FPS_24:
		/* fall through */
	case FPS_23:
		*slcr_fps = PAT_23_976_FPS;
		break;
	default:
		*slcr_fps = PAT_59_94_FPS;
		dev_err(ct->dev, "=== input slicer FPS wrong !!! ===\n");
	}
	return 0;
}

/* sample code for customer */
#if 0
static int mtk_cinematic_rsz_thruput_chk(const struct device *dev,
		struct mtk_resizer_config *config)
{
	struct dprx_video_info_s dprx_timing;
	u32 htotal, vtotal, hac, hfp, hbp, infr, vert_ratio, clock_ratio;
	u16 chk_thrput = 0;

	if (!dev || !config)
		return -EINVAL;

	if (dprx_get_video_info_msa(&dprx_timing) != 0) {
		dev_dbg(dev, "dprx_get_video_info_msa fail\n");
		return -EINVAL;
	}

	htotal = dprx_timing.vid_timing_dpout.h_total;
	vtotal = dprx_timing.vid_timing_dpout.v_total;
	hac    = dprx_timing.vid_timing_dpout.h_active;
	hfp    = dprx_timing.vid_timing_dpout.h_front_porch;
	hbp    = dprx_timing.vid_timing_dpout.h_back_porch;
	infr   = dprx_timing.frame_rate;
	vert_ratio = config->out_height / config->in_height * 100;
	clock_ratio = 320000000 / (htotal * vtotal * infr) * 100;

	pr_info("rsz info %dx%d -> %dx%d\n",
			config->in_width, config->in_height,
			config->out_width, config->out_height);
	pr_info("DP info %d %d %d %d %d %d %d %d\n",
			htotal, vtotal, hac, hfp, hbp, infr,
			vert_ratio, clock_ratio);
	/*checking limited by peak throughput*/
	if (chk_thrput == 1) {
		if (vert_ratio >= clock_ratio)
			goto err_throughput_rule1;
		if ((hac + hbp + hfp - 20) * clock_ratio <=
					config->out_width * vert_ratio)
			goto err_throughput_rule2;
		if ((config->in_width + hbp - 20) * clock_ratio <=
					config->out_width * vert_ratio)
			goto err_throughput_rule3;
	}
	return 0;

err_throughput_rule1:
	dev_err(dev, "ERROR! Please check throughput_rule1\n");
err_throughput_rule2:
	dev_err(dev, "ERROR! Please check throughput_rule2\n");
err_throughput_rule3:
	dev_err(dev, "ERROR! Please check throughput_rule3\n");
	return -EINVAL;
}
#endif

/*  Apply mmsys reset here */
int mtk_reset_core_modules(struct mtk_cinematic_test *ct)
{
	int ret = 0;

	ret = mtk_mmsys_cfg_reset_module(ct->dev_mmsys_core_top,
					 NULL, MMSYSCFG_COMPONENT_MUTEX);
	if (unlikely(ret)) {
		dev_dbg(ct->dev, "reset module failed %d\n",
			MMSYSCFG_COMPONENT_MUTEX);
		return ret;
	}

	ret = mtk_mmsys_cfg_reset_module(ct->dev_mmsys_core_top,
					 NULL, MMSYSCFG_COMPONENT_SLICER_VID);
	if (unlikely(ret)) {
		dev_dbg(ct->dev, "reset module failed %d\n",
			MMSYSCFG_COMPONENT_SLICER_VID);
		return ret;
	}

	ret = mtk_mmsys_cfg_reset_module(ct->dev_mmsys_core_top,
					 NULL, MMSYSCFG_COMPONENT_SLICER_MM);
	if (unlikely(ret)) {
		dev_dbg(ct->dev, "reset module failed %d\n",
			MMSYSCFG_COMPONENT_SLICER_MM);
		return ret;
	}

	ret = mtk_mmsys_cfg_reset_module(ct->dev_mmsys_core_top,
					 NULL, MMSYSCFG_COMPONENT_SLICER_DSC);
	if (unlikely(ret)) {
		dev_dbg(ct->dev, "reset module failed %d\n",
			MMSYSCFG_COMPONENT_SLICER_DSC);
		return ret;
	}

	ret = mtk_mmsys_cfg_reset_module(ct->dev_mmsys_core_top,
					 NULL, ct->disp_mmsys.crop_id);
	if (unlikely(ret)) {
		dev_dbg(ct->dev, "reset module failed %d\n",
			ct->disp_mmsys.crop_id);
		return ret;
	}

	ret = mtk_mmsys_cfg_reset_module(ct->dev_mmsys_core_top,
					 NULL, ct->disp_mmsys.wdma_id);
	if (unlikely(ret)) {
		dev_dbg(ct->dev, "reset module failed %d\n",
			ct->disp_mmsys.wdma_id);
		return ret;
	}

	ret = mtk_mmsys_cfg_reset_module(ct->dev_mmsys_core_top,
					 NULL, MMSYSCFG_COMPONENT_MUTEX);
	if (unlikely(ret)) {
		dev_dbg(ct->dev, "reset module failed %d\n",
				MMSYSCFG_COMPONENT_MUTEX);
		return ret;
	}

	ret = mtk_mmsys_cfg_reset_module(ct->dev_mmsys_core_top,
					 NULL, MMSYSCFG_COMPONENT_SBRC);
	if (unlikely(ret)) {
		dev_dbg(ct->dev, "reset module failed %d\n",
			MMSYSCFG_COMPONENT_SBRC);
		return ret;
	}

	if (ct->disp_path  < 2) {

		ret = mtk_mmsys_cfg_reset_module(ct->dev_mmsys_core_top,
				NULL, MMSYSCFG_COMPONENT_P2S0);
		if (unlikely(ret)) {
			dev_dbg(ct->dev, "reset module failed %d\n",
					MMSYSCFG_COMPONENT_P2S0);
			return ret;
		}
		/* resizer reset should be here */
		ret = mtk_mmsys_cfg_reset_module(ct->dev_mmsys_core_top,
				NULL, ct->disp_mmsys.rsz_id);
		if (unlikely(ret)) {
			dev_dbg(ct->dev, "reset module failed %d\n",
				ct->disp_mmsys.rsz_id);
			return ret;
		}
	}

	return 0;
}
EXPORT_SYMBOL(mtk_reset_core_modules);

int mtk_cinematic_pause(struct mtk_cinematic_test *ct)
{
	mtk_slcr_stop(ct->dev_slicer, NULL);
	mtk_wdma_reset(ct->dev_wdma);
	return 0;
}
EXPORT_SYMBOL(mtk_cinematic_pause);

int mtk_cinematic_resume(struct mtk_cinematic_test *ct)
{
	dprx_if_fifo_reset();
	mtk_slcr_start(ct->dev_slicer, NULL);
	dprx_if_fifo_release();
	return 0;
}
EXPORT_SYMBOL(mtk_cinematic_resume);

int mtk_cinematic_addr_update(struct mtk_cinematic_test **ct)
{
	struct mtk_cinematic_test *ct_m;
	dma_addr_t buf_pa;
	int ret = 0;
	u32 i;
	u32 offset = 0;

	ct_m = ct[MAIN_PIPE];

	if (IS_ERR(ct_m)) {
		pr_err("wdma addr ct ptr err\n");
		return PTR_ERR(ct_m);
	}

	cmdq_pkt_create(&ct_m->cmdq_pkt);

	buf_pa = mtk_cinematic_get_next_fb(ct_m);
#if 0
	dev_dbg(ct->dev, "[update addr] NID = %d, CID = %d\n", ct->next_fb_id,
		ct->cur_fb_id);
#endif
	LOG_FB("[update addr] NID = %d, CID = %d\n", ct_m->next_fb_id,
	       ct_m->cur_fb_id);

	/*dev_dbg(ct->dev, "update addr %llx\n", buf_pa);*/

	if (ct_m->para.quadruple_pipe) {
		for (i = 0; i < QUADRUPLE_PIPE; i++) {
			ret = mtk_wdma_set_out_buf(ct[i]->dev_wdma, NULL,
				buf_pa + offset*(ct_m->output_port.bpp >> 3),
				ct_m->output_port.pitch,
				ct_m->output_port.format);
			offset += ct_m->output_port.config_crop[i].out_w;
		}

	} else {
		if (ct_m->para.pvric_en & COMPRESS_INPUT_ENABLE)
			ret = mtk_wdma_set_out_buf(ct_m->dev_wdma,
				&ct_m->cmdq_pkt,
				buf_pa + ct_m->output_port.pvric_hdr_offset,
				ct_m->output_port.pitch,
				ct_m->output_port.format);
	else
			ret = mtk_wdma_set_out_buf(ct_m->dev_wdma,
				&ct_m->cmdq_pkt, buf_pa,
				ct_m->output_port.pitch,
				ct_m->output_port.format);
	}

	if (ret) {
		dev_err(ct_m->dev, "cinematic wdma addr update NG!!\n");
		cmdq_pkt_destroy(ct_m->cmdq_pkt);
		ct_m->cmdq_pkt = NULL;
		return ret;
	}

	cmdq_pkt_flush_async(ct_m->cmdq_client, ct_m->cmdq_pkt,
			     mtk_cinematic_update_addr_cb, ct_m);
	return 0;
}
EXPORT_SYMBOL(mtk_cinematic_addr_update);

int mtk_dynamic_pat_chg(struct mtk_cinematic_test *ct)
{
	struct rsz_para rp;
	int ret = 0;

	if (IS_ERR(ct)) {
		pr_err("wdma addr ct ptr err\n");
		return PTR_ERR(ct);
	}

	rp = ct->para.rp;

	cmdq_pkt_create(&ct->cmdq_pkt);

	mtk_slicer_cfg(ct->slcr_cfg, ct->slcr_pg_cfg, ct);
	ret = mtk_slcr_patgen_config(ct->dev_slicer, &ct->cmdq_pkt,
				     ct->slcr_pg_cfg);
	if (unlikely(ret)) {
		dev_err(ct->dev, "mtk_slcr_patgen_config NG!!\n");
		goto SEQ_CHG_ERR;
	}

	cmdq_pkt_flush_async(ct->cmdq_client, ct->cmdq_pkt,
			     mtk_cinematic_dynamic_patgen_cb, ct);

	return 0;

SEQ_CHG_ERR:
	cmdq_pkt_destroy(ct->cmdq_pkt);
	ct->cmdq_pkt = NULL;
	return ret;
}
EXPORT_SYMBOL(mtk_dynamic_pat_chg);

int mtk_dynamic_seq_chg(struct mtk_cinematic_test *ct, struct cc_job job,
			u32 eof_id)
{
	struct mtk_resizer_config config_resizer = {0};
	dma_addr_t buf_pa;
	u32 pvric_hdr_offset;
	int ret = 0;

	if (IS_ERR(ct)) {
		pr_err("wdma addr ct ptr err\n");
		return PTR_ERR(ct);
	}

	dev_dbg(ct->dev, "dynamic rsz w %d / h %d\n",
		job.ct_rsz.w, job.ct_rsz.h);

	dev_dbg(ct->dev, "dynamic crop w %d / h %d\n",
		job.ct_crop.w, job.ct_crop.h);

	ct->output_port.pitch = ((job.ct_rsz.w * ct->output_port.bpp) >> 3);

	/* crop0 eof id = 349 */
	cmdq_pkt_create(&ct->cmdq_pkt);

	cmdq_pkt_clear_event(ct->cmdq_pkt, eof_id);
	cmdq_pkt_wfe(ct->cmdq_pkt, eof_id);

	ret = mtk_crop_set_region(ct->dev_crop, &ct->cmdq_pkt, ct->w, ct->h,
				  job.ct_crop.w, job.ct_crop.h, 0, 0);
	if (unlikely(ret)) {
		dev_err(ct->dev, "mtk_crop_set_region NG!!\n");
		goto SEQ_CHG_ERR;
	}

	if (ct->disp_path  < 2) {
		config_resizer.in_width = job.ct_crop.w;
		config_resizer.in_height = job.ct_crop.h;
		config_resizer.out_width = job.ct_rsz.w;
		config_resizer.out_height = job.ct_rsz.h;

		ret = mtk_resizer_config(ct->dev_resizer, &ct->cmdq_pkt,
					 &config_resizer);
		if (unlikely(ret)) {
			dev_err(ct->dev, "mtk_resizer_config NG!!\n");
			goto SEQ_CHG_ERR;
		}

		ret = mtk_p2s_set_proc_mode(ct->dev_p2s, ct->cmdq_pkt,
					    MTK_P2S_PROC_MODE_L_ONLY);
		if (unlikely(ret)) {
			dev_err(ct->dev, "mtk_p2s_set_proc NG!!\n");
			goto SEQ_CHG_ERR;
		}
		mtk_p2s_set_region(ct->dev_p2s, ct->cmdq_pkt, job.ct_rsz.w,
				   job.ct_rsz.h);
	}

	buf_pa = mtk_cinematic_get_empty_fb(ct);

	if (ct->para.pvric_en & COMPRESS_INPUT_ENABLE) {

		if (job.ct_rsz.w % 8 != 0 || job.ct_rsz.h % 8 != 0) {
			dev_err(ct->dev, "PVRIC W/H should be divided by 8!\n");
			ret = -EINVAL;
			goto SEQ_CHG_ERR;
		}
		pvric_hdr_offset = ((job.ct_rsz.w * job.ct_rsz.h + 127) >> 7);
		pvric_hdr_offset = ALIGN(pvric_hdr_offset, 256);
		ct->image_output_size += pvric_hdr_offset;
		ct->output_port.pvric_hdr_offset = pvric_hdr_offset;
		ret = mtk_wdma_set_out_buf(ct->dev_wdma, &ct->cmdq_pkt,
					   buf_pa + pvric_hdr_offset,
					   ct->output_port.pitch,
					   ct->output_port.format);
		if (unlikely(ret)) {
			dev_err(ct->dev, "mtk_wdma_set_out_buf NG!!\n");
			goto SEQ_CHG_ERR;
		}
	} else {
		ret = mtk_wdma_set_out_buf(ct->dev_wdma, &ct->cmdq_pkt, buf_pa,
					   ct->output_port.pitch,
					   ct->output_port.format);
		if (unlikely(ret)) {
			dev_err(ct->dev, "mtk_wdma_set_out_buf NG!!\n");
			goto SEQ_CHG_ERR;
		}
	}

	ret = mtk_wdma_set_region(ct->dev_wdma, &ct->cmdq_pkt, job.ct_rsz.w,
				  job.ct_rsz.h, job.ct_rsz.w, job.ct_rsz.h,
				  0, 0);

	if (unlikely(ret)) {
		dev_err(ct->dev, "mtk_wdma_set_region NG!!\n");
		goto SEQ_CHG_ERR;
	}

	if (ct->target_line != 0) {
		ct->target_line = (job.ct_rsz.h >> 4) * 15;
		ret = mtk_wdma_set_target_line(ct->dev_wdma, &ct->cmdq_pkt,
					       ct->target_line);
		if (unlikely(ret)) {
			dev_err(ct->dev, "mtk_wdma_set_target_line NG!!\n");
			goto SEQ_CHG_ERR;
		}
	}
	/* in gpu case, we do NOT need this */
	if (ct->para.gp.bypass) {
		memset(ct->output_port.buf[ct->cur_fb_id].k_buf->va, 0,
		       ct->output_port.buf_size);
		dma_sync_single_for_device(ct->dev_wdma,
				buf_pa, ct->image_output_size, DMA_TO_DEVICE);
	}

	/* record */
	ct->para.cp.out_w = job.ct_crop.w;
	ct->para.cp.out_h = job.ct_crop.h;
	ct->para.rp.out_w = job.ct_rsz.w;
	ct->para.rp.out_h = job.ct_rsz.h;

	cmdq_pkt_flush_async(ct->cmdq_client, ct->cmdq_pkt,
			     mtk_cinematic_dynamic_cb, ct);
	return 0;

SEQ_CHG_ERR:
	cmdq_pkt_destroy(ct->cmdq_pkt);
	ct->cmdq_pkt = NULL;

	return ret;
}
EXPORT_SYMBOL(mtk_dynamic_seq_chg);


int mtk_quad_pipe_config(struct mtk_cinematic_test **ct)
{
	struct mtk_cinematic_test *ct_m;
	struct mtk_resizer_config config_resizer = {0};
	struct crop_output config_crop[QUADRUPLE_PIPE] = {0};
	int crop_w, crop_h;
	dma_addr_t buf_pa;
	u32 i;
	u32 format, pitch, bpp;
	int ret = 0;
	u32 width, height;
	u32 offset = 0;

	ct_m = ct[MAIN_PIPE];

	crop_w = ct_m->para.cp.out_w;
	crop_h = ct_m->para.cp.out_h;

	/* set input size */
	ct_m->w = ct_m->para.input_width;
	ct_m->h = ct_m->para.input_height;

	width = ct_m->w/4;
	height = ct_m->h;

	dev_dbg(ct_m->dev, "SRC TIMING (%d, %d)\n", ct_m->w, ct_m->h);
	dev_dbg(ct_m->dev, "CROP TIMING (%d, %d)\n", crop_w, crop_h);

	/* get output buffer */
	if (mtk_cinematic_get_buf(ct_m)) {
		dev_dbg(ct_m->dev, "get wmda out buf error\n");
		return -ENOMEM;
	}

	format = ct_m->para.gp.in_fmt;
	bpp = mtk_format_to_bpp(format);
	pitch = (crop_w*bpp) >> 3;
	ct_m->output_port.format = format;
	ct_m->output_port.bpp = bpp;
	ct_m->output_port.pitch = pitch;

	ct_m->image_output_size = pitch * crop_h;

	dev_dbg(ct_m->dev, "image_output_size = 0x%x\n",
		ct_m->image_output_size);

	for (i = 0; i < QUADRUPLE_PIPE; i++) {
		ret = mtk_cinematic_power_on(ct[i]);
		if (unlikely(ret)) {
			dev_err(ct_m->dev, "cinemati_power_on %d NG!!\n", i);
			return ret;
		}
	}

	for (i = 0; i < QUADRUPLE_PIPE; i++) {
		ret = mtk_single_path_reset(ct[i], true);
		if (unlikely(ret)) {
			dev_err(ct_m->dev, "single_path_reset %d NG!!\n", i);
			return ret;
		}
	}

	/* slicer settings */
	mtk_slcr_reset(ct_m->dev_slicer, NULL);
	if (ct_m->para.input_from_dp == 0)
		mtk_slcr_patgen_reset(ct_m->dev_slicer, NULL);

	/* slicer cfg & pattern gen cfg */
	mtk_slicer_cfg(ct_m->slcr_cfg, ct_m->slcr_pg_cfg, ct_m);
	ret = mtk_slcr_config(ct_m->dev_slicer, NULL, ct_m->slcr_cfg);
	if (unlikely(ret)) {
		dev_err(ct_m->dev, "mtk_slcr_config NG!!\n");
		return ret;
	}

	if (ct_m->para.input_from_dp == 0) {
		ret = mtk_slcr_patgen_config(ct_m->dev_slicer, NULL,
					     ct_m->slcr_pg_cfg);
		if (unlikely(ret)) {
			dev_err(ct_m->dev, "mtk_slcr_patgen_config NG!!\n");
			return ret;
		}

		ret = mtk_set_slicer_patgen_fps(ct_m->dev_slicer, ct_m);
		if (unlikely(ret)) {
			dev_err(ct_m->dev, "set_slicer_patgen_fps NG!!\n");
			return ret;
		}
	}

	/* crop settings */
	mtk_crop_quad_cfg(config_crop, ct_m->w, ct_m->h,
			crop_w, crop_h,
			ct_m->para.cp.x, ct_m->para.cp.y);

	for (i = 0; i < QUADRUPLE_PIPE; i++) {
		mtk_crop_reset(ct[i]->dev_crop, NULL);
		ret = mtk_crop_set_region(ct[i]->dev_crop, NULL, width, height,
				  config_crop[i].out_w, config_crop[i].out_h,
				  config_crop[i].x_oft, config_crop[i].y_oft);
		if (unlikely(ret)) {
			dev_err(ct_m->dev, "mtk_crop_set_region %d NG!!\n", i);
			return ret;
		}
		ct_m->output_port.config_crop[i].out_w = config_crop[i].out_w;
	}

	for (i = 0; i < DUAL_PIPE; i++) {
		config_resizer.in_width = config_crop[i].out_w;
		config_resizer.in_height = config_crop[i].out_h;
		config_resizer.out_width = config_crop[i].out_w;
		config_resizer.out_height = config_crop[i].out_h;

		ret = mtk_resizer_config(ct[i]->dev_resizer, NULL,
					 &config_resizer);
		if (unlikely(ret)) {
			dev_err(ct_m->dev, "mtk_resizer_config %d NG!!\n", i);
			return ret;
		}
	}

	/* p2s settings*/
	ret = mtk_p2s_set_proc_mode(ct_m->dev_p2s, NULL,
					    MTK_P2S_PROC_MODE_L_ONLY);
	if (unlikely(ret)) {
		dev_err(ct_m->dev, "mtk_p2s_set_proc NG!!\n");
		return ret;
	}

	mtk_p2s_set_region(ct_m->dev_p2s, NULL,
			config_crop[0].out_w, config_crop[0].out_h);

	buf_pa = mtk_cinematic_get_empty_fb(ct_m);

	for (i = 0; i < QUADRUPLE_PIPE; i++) {
		ret = mtk_wdma_set_region(ct[i]->dev_wdma, NULL,
				config_crop[i].out_w, config_crop[i].out_h,
				config_crop[i].out_w, config_crop[i].out_h,
				0, 0);
		if (unlikely(ret)) {
			dev_err(ct_m->dev, "mtk_wdma_set_region NG!!\n");
			return ret;
		}

		mtk_wdma_set_out_buf(ct[i]->dev_wdma, NULL,
				buf_pa + offset*(bpp >> 3),
				ct_m->output_port.pitch,
				ct_m->output_port.format);
		offset += config_crop[i].out_w;

		ret  = mtk_wdma_bypass_shadow_disable(ct[i]->dev_wdma, true);
		if (unlikely(ret)) {
			dev_err(ct_m->dev, "mtk_enable_shadow NG!!\n");
			return ret;
		}

		if (ct_m->target_line != 0) {
			ret = mtk_wdma_set_target_line(ct[i]->dev_wdma, NULL,
					       ct_m->target_line);
			if (unlikely(ret)) {
				dev_err(ct_m->dev, "mtk_wdma_set_target_line NG!!\n");
				return ret;
			}
		}
	}

	dev_dbg(ct_m->dev, "actual buf dump size = 0x%x\n",
		ct_m->image_output_size);

	ret = mtk_cinematic_quad_path_connect(ct);
	if (unlikely(ret)) {
		dev_err(ct_m->dev, "mtk_cinematic_dual_path_connect NG!!\n");
		return ret;
	}

	/* mutex settings */
	if (ct_m->para.sof_sel == CONTINUOUS_SOF)
		ret = mtk_mutex_select_sof(ct_m->mutex, MUTEX_MMSYS_SOF_DP,
					   NULL, true);
	else
		ret = mtk_mutex_select_sof(ct_m->mutex, MUTEX_MMSYS_SOF_SINGLE,
					   NULL, true);
	if (unlikely(ret)) {
		dev_err(ct_m->dev, "mtk_mutex_select_sof NG!!\n");
		return ret;
	}

	for (i = 0; i < QUADRUPLE_PIPE; i++) {
		ret = mtk_cinematic_start(ct[i]);
		if (unlikely(ret)) {
			dev_err(ct_m->dev, "mtk_cinematic_start %d NG!!\n", i);
			return ret;
		}
	}

	/* slicer settings */
	dprx_if_fifo_reset();
	mtk_slcr_start(ct_m->dev_slicer, NULL);
	dprx_if_fifo_release();
	if (ct_m->para.input_from_dp == 0)
		mtk_slcr_patgen_start(ct_m->dev_slicer, NULL);

	return 0;
}
EXPORT_SYMBOL(mtk_quad_pipe_config);

int mtk_dual_pipe_config(struct mtk_cinematic_test **ct)
{
	struct mtk_cinematic_test *ct_m;
	struct twin_algo_module_para *t_cfg;
	struct mtk_resizer_config config_resizer = {0};
	struct mtk_resizer_quarter_config config_q = {0};
	int rsz_w, rsz_h;
	dma_addr_t buf_pa;
	u32 pvric_hdr_offset, i;
	u32 format, pitch;
	int ret = 0;

	ct_m = ct[MAIN_PIPE];

	rsz_w = ct_m->para.rp.out_w;
	rsz_h = ct_m->para.rp.out_h;

	/* set input size */
	ct_m->w = ct_m->para.input_width;
	ct_m->h = ct_m->para.input_height;

	dev_dbg(ct_m->dev, "SRC TIMING (%d, %d)\n", ct_m->w, ct_m->h);
	dev_dbg(ct_m->dev, "RSZ TIMING (%d, %d)\n", rsz_w, rsz_h);

	/* get output buffer */
	if (mtk_cinematic_get_buf(ct_m)) {
		dev_dbg(ct_m->dev, "get wmda out buf error\n");
		return -ENOMEM;
	}

	if (!ct_m->output_port.using_ion_buf) {
		ct_m->output_port.format = ct_m->para.gp.in_fmt;
		ct_m->output_port.bpp = mtk_format_to_bpp(ct_m->para.gp.in_fmt);
		ct_m->output_port.pitch = ((1920 * ct_m->output_port.bpp) >> 3);
	} else {
		pitch = ct_m->output_port.buf[0].ion_buf->pitch;
		format = ct_m->para.gp.in_fmt;
		ct_m->output_port.format = format;
		ct_m->output_port.pitch = pitch;
		ct_m->output_port.bpp = mtk_format_to_bpp(format);
	}

	ct_m->image_output_size = ct_m->output_port.pitch * rsz_h;

	for (i = 0; i < DUAL_PIPE; i++) {
		ret = mtk_cinematic_power_on(ct[i]);
		if (unlikely(ret)) {
			dev_err(ct_m->dev, "cinemati_power_on %d NG!!\n", i);
			return ret;
		}

		ret = mtk_single_path_reset(ct[i], true);
		if (unlikely(ret)) {
			dev_err(ct_m->dev, "single_path_reset %d NG!!\n", i);
			return ret;
		}
	}

	/* slicer settings */
	mtk_slcr_reset(ct_m->dev_slicer, NULL);
	if (ct_m->para.input_from_dp == 0)
		mtk_slcr_patgen_reset(ct_m->dev_slicer, NULL);

	/* slicer cfg & pattern gen cfg */
	mtk_slicer_cfg(ct_m->slcr_cfg, ct_m->slcr_pg_cfg, ct_m);
	ret = mtk_slcr_config(ct_m->dev_slicer, NULL, ct_m->slcr_cfg);
	if (unlikely(ret)) {
		dev_err(ct_m->dev, "mtk_slcr_config NG!!\n");
		return ret;
	}

	if (ct_m->para.input_from_dp == 0) {
		ret = mtk_slcr_patgen_config(ct_m->dev_slicer, NULL,
					     ct_m->slcr_pg_cfg);
		if (unlikely(ret)) {
			dev_err(ct_m->dev, "mtk_slcr_patgen_config NG!!\n");
			return ret;
		}

		ret = mtk_set_slicer_patgen_fps(ct_m->dev_slicer, ct_m);
		if (unlikely(ret)) {
			dev_err(ct_m->dev, "set_slicer_patgen_fps NG!!\n");
			return ret;
		}
	}

	t_cfg = mtk_get_twin(ct_m, TAMT_CROP);

	/* crop settings */
	for (i = 0; i < DUAL_PIPE; i++) {
		u32 crop_in_w = t_cfg->in_end[i] + 1 - t_cfg->in_start[i];
		u32 crop_out_w = t_cfg->out_end[i] + 1 - t_cfg->out_start[i];
		u32 crop_x_oft = (i == 0 ? ct_m->para.cp.x : 0);
		u32 crop_y_oft = ct_m->para.cp.y;
		u32 crop_out_h = ct_m->para.cp.out_h;

		mtk_crop_reset(ct[i]->dev_crop, NULL);
		ret = mtk_crop_set_region(ct[i]->dev_crop, NULL, crop_in_w,
					  ct_m->h, crop_out_w, crop_out_h,
					  crop_x_oft, crop_y_oft);
		if (unlikely(ret)) {
			dev_err(ct_m->dev, "mtk_crop_set_region %d NG!!\n", i);
			return ret;
		}

		dev_dbg(ct_m->dev, "crop[%d] in width  = %d [%d-%d]\n",
			i, crop_in_w, t_cfg->in_start[i], t_cfg->in_end[i]);
		dev_dbg(ct_m->dev, "crop[%d] out width = %d [%d-%d]\n",
			i, crop_out_w, t_cfg->out_start[i], t_cfg->out_end[i]);
		dev_dbg(ct_m->dev, "crop[%d] x_oft = %d y_oft = %d out height = %d\n",
			i, crop_x_oft, crop_y_oft, crop_out_h);
	}

	t_cfg = mtk_get_twin(ct_m, TAMT_RESIZER);

	for (i = 0; i < DUAL_PIPE; i++) {
		config_resizer.in_width = t_cfg->in_end[i] + 1 -
					  t_cfg->in_start[i];
		config_resizer.in_height = ct_m->para.cp.out_h;
		config_resizer.out_width = t_cfg->out_end[i] + 1 -
					   t_cfg->out_start[i];
		config_resizer.out_height = ct_m->para.rp.out_h;
		if (i == 0)
		config_resizer.overlap = config_resizer.in_width -
			ct_m->para.cp.out_w / 2 - ct_m->para.cp.x;
		else
		config_resizer.overlap = config_resizer.in_width -
			ct_m->para.cp.out_w / 2 -
			(ct_m->w - ct_m->para.cp.out_w - ct_m->para.cp.x);

		if (ct_m->output_port.format == DRM_FORMAT_AYUV2101010)
			config_resizer.rsz_in_format = RESIZER_INPUT_YUV;
		else
			config_resizer.rsz_in_format = RESIZER_INPUT_RGB;


		/* config quarter for offset */
		config_q.in_x_left[0] = t_cfg->in_start[i];
		config_q.in_x_right[0] = t_cfg->in_end[i];
		config_q.in_height = ct[0]->para.cp.out_h;
		config_q.out_x_left[0] = t_cfg->out_start[i];
		config_q.out_x_right[0] = t_cfg->out_end[i];
		config_q.out_height = ct[0]->para.rp.out_h;
		config_q.luma_x_int_off[0] = t_cfg->in_int_offset[i];
		config_q.luma_x_sub_off[0] = t_cfg->in_sub_offset[i];

		dev_dbg(ct_m->dev, "rsz[%d] overlap = %d\n",
			i, config_resizer.overlap);

		dev_dbg(ct_m->dev, "luma_int[%d] = %d luma_sub[%d] = %d\n",
			i, t_cfg->in_int_offset[i], i, t_cfg->in_sub_offset[i]);

		dev_dbg(ct_m->dev, "rsz[%d] in width = %d  in height = %d\n",
			i, t_cfg->in_end[i] + 1 - t_cfg->in_start[i],
			config_resizer.in_height);

		dev_dbg(ct_m->dev, "rsz[%d] out width = %d out height = %d\n",
			i, t_cfg->out_end[i] + 1 - t_cfg->out_start[i],
			ct[0]->para.rp.out_h);

		ret = mtk_resizer_config(ct[i]->dev_resizer, NULL,
					 &config_resizer);
		if (unlikely(ret)) {
			dev_err(ct_m->dev, "mtk_resizer_config %d NG!!\n", i);
			return ret;
		}

		ret = mtk_resizer_config_quarter(ct[i]->dev_resizer, NULL,
						 &config_q);
		if (unlikely(ret)) {
			dev_err(ct_m->dev, "mtk_resizer_config_Q %d NG!!\n", i);
			return ret;
		}
	}

	ret = mtk_p2s_set_proc_mode(ct_m->dev_p2s, NULL, MTK_P2S_PROC_MODE_LR);
	if (unlikely(ret)) {
		dev_err(ct_m->dev, "mtk_p2s_set_proc NG!!\n");
		return ret;
	}

	/* considering add w/h protection when pvric on */
	mtk_p2s_set_region(ct_m->dev_p2s, NULL, 960, 1080);

	/* Setup WDMA */
	ret = mtk_wdma_set_region(ct_m->dev_wdma, NULL, 1920, 1080,
				  1920, 1080, 0, 0);
	if (unlikely(ret)) {
		dev_err(ct_m->dev, "mtk_wdma_set_region NG!!\n");
		return ret;
	}

	buf_pa = mtk_cinematic_get_empty_fb(ct_m);

	if (ct_m->para.pvric_en & COMPRESS_INPUT_ENABLE) {
		pvric_hdr_offset = ((1920 * 1080 + 127) >> 7);
		pvric_hdr_offset = ALIGN(pvric_hdr_offset, 256);
		ct_m->output_port.pvric_hdr_offset = pvric_hdr_offset;
		ct_m->image_output_size += pvric_hdr_offset;
		ret = mtk_wdma_pvric_enable(ct_m->dev_wdma, true);
		if (unlikely(ret)) {
			dev_err(ct_m->dev, "mtk_wdma_pvric_enable NG!!\n");
			return ret;
		}

		ret = mtk_wdma_set_out_buf(ct_m->dev_wdma, NULL,
					   buf_pa + pvric_hdr_offset,
					   ct_m->output_port.pitch,
					   ct_m->output_port.format);
		if (unlikely(ret)) {
			dev_err(ct_m->dev, "mtk_wdma_set_out_buf NG!!\n");
			return ret;
		}
		dev_dbg(ct_m->dev, "pvric header offset = 0x%x\n",
			pvric_hdr_offset);
	} else {
		ret = mtk_wdma_set_out_buf(ct_m->dev_wdma, NULL, buf_pa,
					   ct_m->output_port.pitch,
					   ct_m->output_port.format);
		if (unlikely(ret)) {
			dev_err(ct_m->dev, "mtk_wdma_set_out_buf NG!!\n");
			return ret;
		}
	}

	ret  = mtk_wdma_bypass_shadow_disable(ct_m->dev_wdma, true);

	if (unlikely(ret)) {
		dev_err(ct_m->dev, "mtk_enable_shadow NG!!\n");
		return ret;
	}

	if (ct_m->target_line != 0) {
		ret = mtk_wdma_set_target_line(ct_m->dev_wdma, NULL,
					       ct_m->target_line);
		if (unlikely(ret)) {
			dev_err(ct_m->dev, "mtk_wdma_set_target_line NG!!\n");
			return ret;
		}
	}

	dev_dbg(ct_m->dev, "actual buf dump size = 0x%x\n",
		ct_m->image_output_size);

	ret = mtk_cinematic_dual_path_connect(ct);
	if (unlikely(ret)) {
		dev_err(ct_m->dev, "mtk_cinematic_dual_path_connect NG!!\n");
		return ret;
	}

	/* mutex settings */
	if (ct_m->para.sof_sel == CONTINUOUS_SOF)
		ret = mtk_mutex_select_sof(ct_m->mutex, MUTEX_MMSYS_SOF_DP,
					   NULL, true);
	else
		ret = mtk_mutex_select_sof(ct_m->mutex, MUTEX_MMSYS_SOF_SINGLE,
					   NULL, true);
	if (unlikely(ret)) {
		dev_err(ct_m->dev, "mtk_mutex_select_sof NG!!\n");
		return ret;
	}

	for (i = 0; i < DUAL_PIPE; i++) {
		ret = mtk_cinematic_start(ct[i]);
		if (unlikely(ret)) {
			dev_err(ct_m->dev, "mtk_cinematic_start %d NG!!\n", i);
			return ret;
		}
	}
	/* slicer settings */
	dprx_if_fifo_reset();
	mtk_slcr_start(ct_m->dev_slicer, NULL);
	dprx_if_fifo_release();
	if (ct_m->para.input_from_dp == 0)
		mtk_slcr_patgen_start(ct_m->dev_slicer, NULL);

	return 0;
}
EXPORT_SYMBOL(mtk_dual_pipe_config);

int mtk_single_pipe_config(struct mtk_cinematic_test *ct)
{
	struct mtk_resizer_config config_resizer = {0};
	struct mtk_wdma_color_trans color_trans = {0};
	struct crop_para cp;
	struct rsz_para rp;
	dma_addr_t buf_pa;
	u32 pvric_hdr_offset;
	u32 pitch, format;
	u32 slcr_out_w, slcr_out_h;
	int ret = 0;
	int reshape = true;


	if (IS_ERR(ct)) {
		pr_err("wdma addr ct ptr err\n");
		return PTR_ERR(ct);
	}

	/* get output buffer */
	if (mtk_cinematic_get_buf(ct)) {
		dev_dbg(ct->dev, "get wmda out buf error\n");
		return -ENOMEM;
	}

	rp = ct->para.rp;
	cp = ct->para.cp;

	/* set input size */
	ct->w = ct->para.input_width;
	ct->h = ct->para.input_height;

	dev_dbg(ct->dev, "SRC TIMING!! (%d, %d)\n", ct->w, ct->h);
	dev_dbg(ct->dev, "RSZ TIMING!! (%d, %d)\n", rp.out_w, rp.out_h);

#if RUN_BIT_TRUE
	reshape = (((ct->para.pvric_en & COMPRESS_INPUT_ENABLE) &&
			(ct->w == 1920) && (ct->h == 2205) &&
			(ct->para.cp.out_h == 2176))) ||
			((ct->para.pvric_en & COMPRESS_INPUT_ENABLE) &&
			(ct->w == 1280) && (ct->h == 1470));
#endif

	if (!ct->output_port.using_ion_buf) {
		ct->output_port.format = ct->para.gp.in_fmt;
		ct->output_port.bpp = mtk_format_to_bpp(ct->para.gp.in_fmt);
		ct->output_port.pitch = ((rp.out_w * ct->output_port.bpp) >> 3);
	} else {
		pitch = ct->output_port.buf[0].ion_buf->pitch;
		format = ct->para.gp.in_fmt;
		ct->output_port.format = format;
		ct->output_port.pitch = pitch;
		ct->output_port.bpp = mtk_format_to_bpp(format);
	}

	ct->image_output_size = ct->output_port.pitch * rp.out_h;

	dev_dbg(ct->dev, "actual buf dump size = 0x%x\n",
		ct->image_output_size);

	ret = mtk_cinematic_power_on(ct);

	if (unlikely(ret)) {
		dev_err(ct->dev, "mtk_cinemati_power_on NG!!\n");
		return ret;
	}

	/* reset dram , related parameters, core modules */
	ret = mtk_single_path_reset(ct, false);

	if (unlikely(ret)) {
		dev_err(ct->dev, "mtk_single_path_reset NG!!\n");
		return ret;
	}

	/* slicer settings */
	mtk_slcr_reset(ct->dev_slicer, NULL);
	if (ct->para.input_from_dp == 0)
		mtk_slcr_patgen_reset(ct->dev_slicer, NULL);

	/* slicer cfg & pattern gen cfg */
	mtk_slicer_cfg(ct->slcr_cfg, ct->slcr_pg_cfg, ct);
#if 0
	ct->slcr_cfg->dp_video.gce.event = GCE_INPUT;
	ct->slcr_cfg->dp_video.gce.width = 1;
	ct->slcr_cfg->dp_video.gce.height = 1;
#endif
	ret = mtk_slcr_config(ct->dev_slicer, NULL, ct->slcr_cfg);
	if (unlikely(ret)) {
		dev_err(ct->dev, "mtk_slcr_config NG!!\n");
		return ret;
	}

	/* cfg slicer patter generator fps */
	if (ct->para.input_from_dp == 0) {
		ret = mtk_slcr_patgen_config(ct->dev_slicer, NULL,
					     ct->slcr_pg_cfg);
		if (unlikely(ret)) {
			dev_err(ct->dev, "mtk_slcr_patgen_config NG!!\n");
			return ret;
		}

		ret = mtk_set_slicer_patgen_fps(ct->dev_slicer, ct);
		if (unlikely(ret)) {
			dev_err(ct->dev, "set_slicer_patgen_fps NG!!\n");
			return ret;
		}
	}

	slcr_out_w = mtk_slicer_get_output_width(ct->slcr_cfg, ct);

	/* 3d reshape , only single pipe apply */
	if (reshape && (ct->w == 1920) && (ct->h == 2205))
		slcr_out_h = (1080 + ct->dummy_line) * 2;
	else if (reshape && (ct->w == 1280) && (ct->h == 1470))
		slcr_out_h = (720 + ct->dummy_line) * 2;
	else
		slcr_out_h = ct->h + ct->dummy_line;

	/* crop settings */
	mtk_crop_reset(ct->dev_crop, NULL);
	ret = mtk_crop_set_region(ct->dev_crop, NULL, slcr_out_w, slcr_out_h,
				  cp.out_w, cp.out_h, cp.x, cp.y);
	if (unlikely(ret)) {
		dev_err(ct->dev, "mtk_crop_set_region NG!!\n");
		return ret;
	}

	/* rsz settings by pass resizer actually we can do NOTHING!!! */
	config_resizer.in_width = cp.out_w;
	config_resizer.in_height = cp.out_h;
	config_resizer.out_width = rp.out_w;
	config_resizer.out_height = rp.out_h;

	if (ct->output_port.format == DRM_FORMAT_AYUV2101010)
		config_resizer.rsz_in_format = RESIZER_INPUT_YUV;
	else
		config_resizer.rsz_in_format = RESIZER_INPUT_RGB;

	/* sample code for customer */
	#if 0
	if (mtk_cinematic_rsz_thruput_chk(ct->dev_resizer,
			&config_resizer) != 0) {
		dev_dbg(ct->dev, "mtk_cinematic_rsz_thruput_chk fail\n");
		return -EINVAL;
	}
	#endif

	if (ct->disp_path  < 2) {
		ret = mtk_resizer_config(ct->dev_resizer, NULL,
					 &config_resizer);
		if (unlikely(ret)) {
			dev_err(ct->dev, "mtk_resizer_config NG!!\n");
			return ret;
		}
	}

	/* p2s settings , left in relay */
	if (ct->disp_path == 0) {
		ret = mtk_p2s_set_proc_mode(ct->dev_p2s, NULL,
					    MTK_P2S_PROC_MODE_L_ONLY);
		if (unlikely(ret)) {
			dev_err(ct->dev, "mtk_p2s_set_proc NG!!\n");
			return ret;
		}

		mtk_p2s_set_region(ct->dev_p2s, NULL, rp.out_w, rp.out_h);
	}

	buf_pa = mtk_cinematic_get_empty_fb(ct);

	/* Setup WDMA */
	if (ct->para.pvric_en & COMPRESS_INPUT_ENABLE) {

		if (rp.out_w % 8 != 0 || rp.out_h % 8 != 0) {
			dev_err(ct->dev, "PVRIC W/H should be divided by 8!\n");
			return -EINVAL;
		}
		pvric_hdr_offset = ((rp.out_w * rp.out_h + 127) >> 7);
		pvric_hdr_offset = ALIGN(pvric_hdr_offset, 256);
		ct->output_port.pvric_hdr_offset = pvric_hdr_offset;
		ct->image_output_size += pvric_hdr_offset;
		ret = mtk_wdma_pvric_enable(ct->dev_wdma, true);
		if (unlikely(ret)) {
			dev_err(ct->dev, "mtk_wdma_pvric_enable NG!!\n");
			return ret;
		}

		ret = mtk_wdma_set_out_buf(ct->dev_wdma, NULL,
					   buf_pa + pvric_hdr_offset,
					   ct->output_port.pitch,
					   ct->output_port.format);
		if (unlikely(ret)) {
			dev_err(ct->dev, "mtk_wdma_set_out_buf NG!!\n");
			return ret;
		}
		dev_dbg(ct->dev, "PVRIC ON: HEADER size = %d(0x%x) Bytes.\n",
			pvric_hdr_offset, pvric_hdr_offset);
		dev_dbg(ct->dev_wdma, "The beginning of PVRIC HEADER are %d(0x%x) Bytes alignment 0.\n",
			(u8)(((u64)(((void *)buf_pa + pvric_hdr_offset -
			((rp.out_w * rp.out_h + 127) >> 7)))) & 0xff),
			(u8)(((u64)(((void *)buf_pa + pvric_hdr_offset -
			((rp.out_w * rp.out_h + 127) >> 7)))) & 0xff));
	} else {
		ret = mtk_wdma_set_out_buf(ct->dev_wdma, NULL, buf_pa,
					   ct->output_port.pitch,
					   ct->output_port.format);
		if (unlikely(ret)) {
			dev_err(ct->dev, "mtk_wdma_set_out_buf NG!!\n");
			return ret;
		}
	}

	ret = mtk_wdma_set_region(ct->dev_wdma, NULL, rp.out_w, rp.out_h,
				  rp.out_w, rp.out_h, 0, 0);

	if (unlikely(ret)) {
		dev_err(ct->dev, "mtk_wdma_set_region NG!!\n");
		return ret;
	}

	ret  = mtk_wdma_bypass_shadow_disable(ct->dev_wdma, true);

	if (unlikely(ret)) {
		dev_err(ct->dev, "mtk_enable_shadow NG!!\n");
		return ret;
	}

	if (ct->target_line != 0) {
		ret = mtk_wdma_set_target_line(ct->dev_wdma, NULL,
					       ct->target_line);
		if (unlikely(ret)) {
			dev_err(ct->dev, "mtk_wdma_set_target_line NG!!\n");
			return ret;
		}
	}

	if (ct->para.wdma_csc_en == 0)
		color_trans.enable = false;
	else if (ct->para.wdma_csc_en == 1) {
		color_trans.enable = true;
		color_trans.external_matrix = false;
		color_trans.int_matrix = MTK_WDMA_INT_COLOR_TRANS_RGB_TO_BT601;
		mtk_wdma_set_color_transform(ct->dev_wdma, NULL, &color_trans);
	}

	dev_dbg(ct->dev, "actual buf dump size = %d(0x%x)\n",
		ct->image_output_size, ct->image_output_size);

	/* connect disp path according to disp_path */
	ret = mtk_cinematic_path_connect(ct);
	if (unlikely(ret)) {
		dev_err(ct->dev, "mtk_cinematic_path_connect NG!!\n");
		return ret;
	}

	/* mutex settings */
	if (ct->para.sof_sel == CONTINUOUS_SOF) {
		int delay;

		ret = mtk_cinematic_get_delay(ct, &delay);
		if (unlikely(ret)) {
			dev_err(ct->dev, "mtk_mutex_get delay NG!!\n");
			return ret;
		}

		ret = mtk_mutex_set_delay_us(ct->mutex_delay, delay, NULL);
		if (unlikely(ret)) {
			dev_err(ct->dev, "mtk_mutex_set delay NG!!\n");
			return ret;
		}

		ret = mtk_mutex_select_delay_sof(ct->mutex_delay,
						 MUTEX_MMSYS_SOF_DP, NULL);
		if (unlikely(ret)) {
			dev_err(ct->dev, "mtk_mutex_select_delay_sof NG!!\n");
			return ret;
		}

		ret = mtk_mutex_delay_enable(ct->mutex_delay, NULL);
		if (unlikely(ret)) {
			dev_err(ct->dev, "mtk_mutex_delay_enable NG!!\n");
			return ret;
		}

		ret = mtk_mutex_select_sof(ct->mutex,
					   MUTEX_MMSYS_SOF_SYNC_DELAY0 +
					   ct->disp_path + DISPSYS_DELAY_NUM,
					   NULL, true);
		if (unlikely(ret)) {
			dev_err(ct->dev, "mtk_mutex_select_sof NG!!\n");
			return ret;
		}

	} else {
		ret = mtk_mutex_select_sof(ct->mutex, MUTEX_MMSYS_SOF_SINGLE,
					   NULL, true);
		if (unlikely(ret)) {
			dev_err(ct->dev, "mtk_mutex_select_sof NG!!\n");
			return ret;
		}
	}
#if 0
	mtk_mutex_set_comp_eof_mask(ct->dev_mutex, ct->disp_mutex.crop_id,
				    false, NULL);
#endif

	/*if (!ct->output_port.using_ion_buf)*/
	if (0)
		mtk_cinematic_test_init_wait(ct);

	ret = mtk_cinematic_start(ct);
	if (unlikely(ret)) {
		dev_err(ct->dev, "mtk_cinematic_start NG!!\n");
		return ret;
	}
	/* slicer settings */
	dprx_if_fifo_reset();
	mtk_slcr_start(ct->dev_slicer, NULL);
	dprx_if_fifo_release();
	if (ct->para.input_from_dp == 0)
		mtk_slcr_patgen_start(ct->dev_slicer, NULL);

	return 0;
}
EXPORT_SYMBOL(mtk_single_pipe_config);

static int mtk_cinematic_test_get_device(struct device *dev,
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

static int mtk_cinematic_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_cinematic_test *ct;
	struct mtk_dma_buf **k_buf;
	int ret;
	u32 i;

	ct = devm_kzalloc(dev, sizeof(*ct), GFP_KERNEL);
	if (!ct)
		return -ENOMEM;

	ct->dev = dev;

	dev_dbg(ct->dev, "addr %p\n", ct);

	/* get node cinematic display path */
	if (of_property_read_u32(dev->of_node, "path", &ct->disp_path) < 0) {
		dev_dbg(ct->dev, "get disp_path\n");
		return -EINVAL;
	}

	ct->slcr_cfg = devm_kzalloc(dev, sizeof(*ct->slcr_cfg), GFP_KERNEL);

	if (!ct->slcr_cfg)
		return -ENOMEM;

	ct->slcr_pg_cfg = devm_kzalloc(dev, sizeof(*ct->slcr_pg_cfg),
				       GFP_KERNEL);

	if (!ct->slcr_pg_cfg)
		return -ENOMEM;

	for (i = 0; i < WDMA_BUF; i++) {

		k_buf = &ct->output_port.buf[i].k_buf;
		*k_buf = devm_kzalloc(dev, sizeof(**k_buf), GFP_KERNEL);

		if (!(*k_buf))
			return -ENOMEM;
	}

	/* request cmdq channel 0 */
	if (ct->disp_path < 2) {
		ct->cmdq_client = cmdq_mbox_create(ct->dev, 0);

		if (IS_ERR(ct->cmdq_client)) {
			dev_err(ct->dev, "mbox create failed\n");
			return PTR_ERR(ct->cmdq_client);
		}
	}

	ct->output_port.using_ion_buf = false;
	ct->gpu_fb_id = 0xff;
	ct->is_3d_tag = false;

	/* init queue lock */
	mutex_init(&ct->fb_lock);

	/* only disp 0 / disp 1 has there */
	if (ct->disp_path < 2) {
		ret = mtk_cinematic_test_get_device(dev, "mediatek,cp2s", 0,
						    &ct->dev_p2s);
		if (unlikely(ret))
			return ret;

		ret = mtk_cinematic_test_get_device(dev, "mediatek,resizer", 0,
						    &ct->dev_resizer);
		if (unlikely(ret))
			return ret;
	}

	ret = mtk_cinematic_test_get_device(dev, "mediatek,mmsys_core_top", 0,
					    &ct->dev_mmsys_core_top);
	if (unlikely(ret))
		return ret;

	ret = mtk_cinematic_test_get_device(dev, "mediatek,mutex", 0,
					    &ct->dev_mutex);
	if (unlikely(ret))
		return ret;

	ret = mtk_cinematic_test_get_device(dev, "mediatek,wdma", 0,
					    &ct->dev_wdma);
	if (unlikely(ret))
		return ret;

	ret = mtk_cinematic_test_get_device(dev, "mediatek,crop", 0,
					    &ct->dev_crop);
	if (unlikely(ret))
		return ret;

	ret = mtk_cinematic_test_get_device(dev, "mediatek,slicer", 0,
					    &ct->dev_slicer);
	if (unlikely(ret))
		return ret;

	ret = mtk_cinematic_init(ct);

	if (unlikely(ret))
		return ret;

	platform_set_drvdata(pdev, ct);
	return 0;
}

static int mtk_cinematic_test_remove(struct platform_device *pdev)
{
	struct mtk_cinematic_test *ct = platform_get_drvdata(pdev);
	int i = 0;

	if (ct->buffer_allocated) {

		for (i = 0; i < BYP_GPU_WDMA_BUF; i++) {
			mtk_util_buf_destroy(ct->dev_wdma,
					ct->output_port.buf_size,
					16, ct->output_port.buf[i].k_buf->va,
					ct->output_port.buf[i].k_buf->pa);

		}
		ct->buffer_allocated = false;
	}
	mtk_mutex_put(ct->mutex);
	mtk_mutex_delay_put(ct->mutex_delay);

	if (ct->disp_path < 2)
		cmdq_mbox_destroy(ct->cmdq_client);

	dev_dbg(ct->dev, "cinematic mode ut unloaded\n");
	return 0;
}

static const struct of_device_id cinematic_test_driver_dt_match[] = {
	{ .compatible = "mediatek,mt3612-cinematic-test" },
	{},
};
MODULE_DEVICE_TABLE(of, cinematic_test_driver_dt_match);

struct platform_driver mtk_cinematic_test_driver = {
	.probe		= mtk_cinematic_probe,
	.remove		= mtk_cinematic_test_remove,
	.driver		= {
		.name	= "mediatek-cinematic-test",
		.owner	= THIS_MODULE,
		.of_match_table = cinematic_test_driver_dt_match,
	},
};

module_platform_driver(mtk_cinematic_test_driver);
MODULE_LICENSE("GPL");

