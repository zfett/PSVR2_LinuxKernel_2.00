/*
* Copyright (c) 2018 MediaTek Inc.
* Authors:
* leji qiu <leji.qiu@mediatek.com>
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

#include <mtk_mm_gaze_dev.h>

#define GAZE_FOR_DEBUG		0
#define REG_GAZE_WDMA_CB	1
#define RBFC_INCOMPLETE_DEBUG	1
#define FLOW_TIMEOUT 100 /* 0.1 seconds */
static int frame_idx;
static int p2_sof_idx;

#if GAZE_FOR_DEBUG
static int count;
#endif

static int mtk_mm_gaze_cmdq_init(struct mtk_mm_gaze *mm_gaze)
{
	if (mm_gaze->cmdq_client) {
		cmdq_mbox_destroy(mm_gaze->cmdq_client);
		mm_gaze->cmdq_client = NULL;
	}
	if (mm_gaze->cmdq_client_gaze0_p2_sof) {
		cmdq_mbox_destroy(mm_gaze->cmdq_client_gaze0_p2_sof);
		mm_gaze->cmdq_client_gaze0_p2_sof = NULL;
	}
	if (mm_gaze->cmdq_client_camera_sync) {
		cmdq_mbox_destroy(mm_gaze->cmdq_client_camera_sync);
		mm_gaze->cmdq_client_camera_sync = NULL;
	}
#if RBFC_INCOMPLETE_DEBUG
	if (mm_gaze->cmdq_client_rbfc_ren) {
		cmdq_mbox_destroy(mm_gaze->cmdq_client_rbfc_ren);
		mm_gaze->cmdq_client_rbfc_ren = NULL;
	}
	if (mm_gaze->cmdq_client_gaze0_wen) {
		cmdq_mbox_destroy(mm_gaze->cmdq_client_gaze0_wen);
		mm_gaze->cmdq_client_gaze0_wen = NULL;
	}
	if (mm_gaze->cmdq_client_gaze1_wen) {
		cmdq_mbox_destroy(mm_gaze->cmdq_client_gaze1_wen);
		mm_gaze->cmdq_client_gaze1_wen = NULL;
	}
#endif
	mm_gaze->cmdq_client =
		cmdq_mbox_create(mm_gaze->dev, 0);
	if (IS_ERR(mm_gaze->cmdq_client)) {
		dev_err(mm_gaze->dev, "fail to create mailbox\n");
		mm_gaze->cmdq_client = NULL;
	}

#if RBFC_INCOMPLETE_DEBUG
	mm_gaze->cmdq_client_rbfc_ren =
		cmdq_mbox_create(mm_gaze->dev, 1);
	if (IS_ERR(mm_gaze->cmdq_client_rbfc_ren)) {
		dev_err(mm_gaze->dev, "fail to create ren rbfc mailbox\n");
		mm_gaze->cmdq_client_rbfc_ren = NULL;
	}
	mm_gaze->cmdq_client_gaze0_wen =
		cmdq_mbox_create(mm_gaze->dev, 2);
	if (IS_ERR(mm_gaze->cmdq_client_gaze0_wen)) {
		dev_err(mm_gaze->dev, "fail to create gaze0 wen mailbox\n");
		mm_gaze->cmdq_client_gaze0_wen = NULL;
	}
	mm_gaze->cmdq_client_gaze1_wen =
		cmdq_mbox_create(mm_gaze->dev, 3);
	if (IS_ERR(mm_gaze->cmdq_client_gaze1_wen)) {
		dev_err(mm_gaze->dev, "fail to create gaze1 wen mailbox\n");
		mm_gaze->cmdq_client_gaze1_wen = NULL;
	}
#endif
	mm_gaze->cmdq_client_gaze0_p2_sof =
		cmdq_mbox_create(mm_gaze->dev, 4);
	if (IS_ERR(mm_gaze->cmdq_client_gaze0_p2_sof)) {
		dev_err(mm_gaze->dev, "fail to create gaze0 p2 sof mailbox\n");
		mm_gaze->cmdq_client_gaze0_p2_sof = NULL;
	}
	mm_gaze->cmdq_client_camera_sync =
		cmdq_mbox_create(mm_gaze->dev, 5);
	if (IS_ERR(mm_gaze->cmdq_client_camera_sync)) {
		dev_err(mm_gaze->dev, "fail to create camera_sync mailbox\n");
		mm_gaze->cmdq_client_camera_sync = NULL;
	}

	dev_dbg(mm_gaze->dev, "cmdq init done.\n");

	return 0;
}

static int mtk_mm_gaze_cmdq_deinit(struct mtk_mm_gaze *mm_gaze)
{
	if (mm_gaze->cmdq_client) {
		cmdq_mbox_destroy(mm_gaze->cmdq_client);
		mm_gaze->cmdq_client = NULL;
	}

#if RBFC_INCOMPLETE_DEBUG
	if (mm_gaze->cmdq_client_rbfc_ren) {
		cmdq_mbox_destroy(mm_gaze->cmdq_client_rbfc_ren);
		mm_gaze->cmdq_client_rbfc_ren = NULL;
	}
	if (mm_gaze->cmdq_client_gaze0_wen) {
		cmdq_mbox_destroy(mm_gaze->cmdq_client_gaze0_wen);
		mm_gaze->cmdq_client_gaze0_wen = NULL;
	}
	if (mm_gaze->cmdq_client_gaze1_wen) {
		cmdq_mbox_destroy(mm_gaze->cmdq_client_gaze1_wen);
		mm_gaze->cmdq_client_gaze1_wen = NULL;
	}
#endif
	if (mm_gaze->cmdq_client_gaze0_p2_sof) {
		cmdq_mbox_destroy(mm_gaze->cmdq_client_gaze0_p2_sof);
		mm_gaze->cmdq_client_gaze0_p2_sof = NULL;
	}
	if (mm_gaze->cmdq_client_camera_sync) {
		cmdq_mbox_destroy(mm_gaze->cmdq_client_camera_sync);
		mm_gaze->cmdq_client_camera_sync = NULL;
	}

	return 0;
}

#if 0
int move_va_to_sram(struct device *dev, void *va, u32 pa, int size)
{
	int ret, i;

	for (i = 0; i < size; i += 16) {
		ret = mtk_mm_sysram_fill_128b(dev, pa + i, 16, va + i);
		if (ret < 0) {
			dev_err(dev, "sram fill fail @%d, ret = %d\n", i, ret);
			return ret;
		}
	}

	return ret;
}

int move_sram_to_va(struct device *dev, u32 pa, void *va, int size)
{
	int ret, i;

	for (i = 0; i < size; i += 16) {
		ret = mtk_mm_sysram_read(dev, pa + i, va + i);
		if (ret < 0) {
			dev_err(dev, "sram read fail @%d, ret = %d\n", i, ret);
			return ret;
		}
	}

	return ret;
}
#endif

#if REG_GAZE_WDMA_CB
void mtk_gaze_wdma_test_cb(struct mtk_mmsys_cb_data *data)
{
	struct mtk_mm_gaze *mm_gaze =
		(struct mtk_mm_gaze *)data->priv_data;

	dev_dbg(mm_gaze->dev,
		"[wdma_cb] data.part_id: %d, data.status: %d\n",
		data->part_id, data->status);
	dev_dbg(mm_gaze->dev,
		"[wdma_cb] wdma frame done !!!\n");
}
#endif

void mtk_gaze_warpa_test_cb(struct mtk_mmsys_cb_data *data)
{
	struct mtk_mm_gaze *mm_gaze =
		(struct mtk_mm_gaze *)data->priv_data;

	if (data->status & 0x1F)
		dev_err(mm_gaze->dev, "mtk_gaze_warpa_cb: status %x\n",
			data->status);
}

static int mtk_gaze_register_cb
		(struct mtk_mm_gaze *mm_gaze)
{
	if (!mm_gaze || !mm_gaze->dev)
		return -EINVAL;

#if REG_GAZE_WDMA_CB
	/* clear cb */
	mtk_wdma_register_cb(mm_gaze->dev_gaze_wdma,
				mtk_gaze_wdma_test_cb, 0, mm_gaze);

	pr_debug("mtk_wdma_register_cb\n");
	mtk_wdma_register_cb(mm_gaze->dev_gaze_wdma,
		mtk_gaze_wdma_test_cb,
		MTK_WDMA_FRAME_COMPLETE, mm_gaze);
#endif

#if 1
	pr_debug("mtk_warpa_register_cb\n");
	mtk_warpa_register_cb(mm_gaze->dev_warpa,
		mtk_gaze_warpa_test_cb, 0x01, mm_gaze);
#endif

	return 0;
}

static int mtk_cv_enable(struct device *dev, u32 camerea_id, u32 vsync_cycle,
			 u32 delay_cycle, u8 n, u8 m, struct cmdq_pkt **handle)
{
	mtk_mmsys_cfg_camera_sync_clock_sel(dev, true);
	mtk_mmsys_cfg_camera_sync_config(dev, camerea_id, vsync_cycle,
					delay_cycle, false, handle);
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

static int mtk_mm_gaze_config_rbfc(struct mtk_mm_gaze *mm_gaze)
{
	int ret = 0;
	u32 r_th[1] = {0};
	/* u32 w_th[1] = {0}; */
	u32 rbfc_act_nr[2] = {1, 1};
	u32 rbfc_in_pitch;

	if (!mm_gaze || !mm_gaze->dev)
		return -EINVAL;

	r_th[0] = roundup(mm_gaze->warpa_in_h * 15, 100) / 100 + 8;
	/* w_th[0] = {((r_th[0] - 1) << 16) | 1}; */

	if (mm_gaze->mmgazemask & DEV_GAZE_CAMERA_2EYES_TEST_MASK)
		rbfc_act_nr[0] = 3;

	rbfc_in_pitch = roundup(mm_gaze->warpa_in_w,
		mm_gaze->warpa_align) * mm_gaze->img_num;
	dev_dbg(mm_gaze->dev,
			"gaze wpea_rbfc pitch = %d\n", rbfc_in_pitch);

#if RBFC_INCOMPLETE_DEBUG /* make for ren rbfc incomplete debug */
	if (mm_gaze->mmgazemask & DEV_GAZE_CAMERA_FOR_DEBUG_1) {
		mm_gaze->warpa_in_h += 1;
		dev_dbg(mm_gaze->dev,
			"for ren rbfc incomplete debug: rbfc height = %d\n",
			mm_gaze->warpa_in_h);
	}
#endif

	ret = mtk_rbfc_set_plane_num(mm_gaze->dev_rbfc, 1);
	if (ret) {
		dev_err(mm_gaze->dev,
			"mtk_rbfc_set_plane_num fail!! %d\n", ret);
		return ret;
	}

	ret = mtk_rbfc_set_active_num(mm_gaze->dev_rbfc,
				NULL, rbfc_act_nr);
	if (ret) {
		dev_err(mm_gaze->dev,
			"mtk_rbfc_set_active_num fail!! %d\n", ret);
		return ret;
	}

	ret = mtk_rbfc_set_region_multi(mm_gaze->dev_rbfc,
			NULL, 0, mm_gaze->warpa_in_w,
			mm_gaze->warpa_in_h);
	if (ret) {
		dev_err(mm_gaze->dev,
			"mtk_rbfc_set_region_multi fail!! %d\n", ret);
		return ret;
	}

	if (mm_gaze->mmgazemask & DEV_GAZE_CAMERA_RBFC_FROM_SYSRAM_MASK) {
		dev_dbg(mm_gaze->dev, "mm gaze rbfc set target: SYSRAM\n");
		ret = mtk_rbfc_set_target(mm_gaze->dev_rbfc,
					NULL, MTK_RBFC_TO_SYSRAM);
		if (ret) {
			dev_err(mm_gaze->dev,
				"mtk_rbfc_set_target fail!! %d\n", ret);
			return ret;
		}

		ret = mtk_rbfc_set_ring_buf_multi(mm_gaze->dev_rbfc,
				  NULL, 0, GAZE_WARPA_IN_SRAM_ADDR,
				  rbfc_in_pitch, mm_gaze->warpa_in_h);
		if (ret) {
			dev_err(mm_gaze->dev,
				"rbfc_set_ring_buf_multi fail!! %d\n", ret);
			return ret;
		}
	} else {
		dev_dbg(mm_gaze->dev, "mm gaze rbfc set target: DRAM\n");
		ret = mtk_rbfc_set_target(mm_gaze->dev_rbfc,
					NULL, MTK_RBFC_TO_DRAM);
		if (ret) {
			dev_err(mm_gaze->dev,
				"mtk_rbfc_set_target fail!! %d\n", ret);
			return ret;
		}

		ret = mtk_rbfc_set_ring_buf_multi(mm_gaze->dev_rbfc,
			NULL, 0, 0x0, rbfc_in_pitch,
			mm_gaze->warpa_in_h);
		if (ret) {
			dev_err(mm_gaze->dev,
				"rbfc_set_ring_buf_multi fail!! %d\n", ret);
			return ret;
		}
	}

	if (mm_gaze->mmgazemask & DEV_GAZE_CAMERA_ENABLE_RBFC_MASK) {
		ret = mtk_rbfc_set_read_thd(mm_gaze->dev_rbfc, NULL, r_th);
		if (ret) {
			dev_err(mm_gaze->dev,
				"rbfc_set_read_thd fail!! %d\n", ret);
			return ret;
		}

#if 0
		ret = mtk_rbfc_set_write_thd(mm_gaze->dev_rbfc, NULL, w_th);
		if (ret) {
			dev_err(mm_gaze->dev,
				"rbfc_set_write_thd fail!! %d\n", ret);
			return ret;
		}
#endif

		ret = mtk_rbfc_start_mode(mm_gaze->dev_rbfc,
			NULL, MTK_RBFC_NORMAL_MODE);
		if (ret) {
			dev_err(mm_gaze->dev,
				"rbfc_start_mode fail!! %d\n", ret);
			return ret;
		}

		dev_dbg(mm_gaze->dev,
			"mm gaze path enable rbfc\n");
	} else {
		ret = mtk_rbfc_start_mode(mm_gaze->dev_rbfc,
			NULL, MTK_RBFC_DISABLE_MODE);
		if (ret) {
			dev_err(mm_gaze->dev,
				"mtk_rbfc_start_mode fail!! %d\n", ret);
			return ret;
		}

		dev_dbg(mm_gaze->dev, "mm gaze path disable rbfc\n");
	}

#if RBFC_INCOMPLETE_DEBUG /* make for ren rbfc incomplete debug */
	if (mm_gaze->mmgazemask & DEV_GAZE_CAMERA_FOR_DEBUG_1)
		mm_gaze->warpa_in_h -= 1;
#endif

	return ret;
}

static int mtk_mm_gaze_config_warpa
		(struct mtk_mm_gaze *mm_gaze)
{
	int ret = 0;

	if (!mm_gaze || !mm_gaze->dev)
		return -EINVAL;

	dev_dbg(mm_gaze->dev, "mm_gaze->warpa_in_w %d\n",
		mm_gaze->warpa_in_w);
	dev_dbg(mm_gaze->dev, "mm_gaze->warpa_in_h %d\n",
		mm_gaze->warpa_in_h);
	dev_dbg(mm_gaze->dev, "mm_gaze->warpa_out_w %d\n",
		mm_gaze->warpa_out_w);
	dev_dbg(mm_gaze->dev, "mm_gaze->warpa_out_h %d\n",
		mm_gaze->warpa_out_h);
	dev_dbg(mm_gaze->dev, "mm_gaze->warpa_map_w %d\n",
		mm_gaze->warpa_map_w);
	dev_dbg(mm_gaze->dev, "mm_gaze->warpa_map_h %d\n",
		mm_gaze->warpa_map_h);
	dev_dbg(mm_gaze->dev, "mm_gaze->warpa_map_size %d\n",
		mm_gaze->warp_map_size);
	dev_dbg(mm_gaze->dev, "mm_gaze->warpa_align %d\n",
		mm_gaze->warpa_align);
	dev_dbg(mm_gaze->dev, "mm_gaze->warpa_proc_mode %d\n",
		mm_gaze->proc_mode);
	dev_dbg(mm_gaze->dev, "mm_gaze->warpa_out_mode %d\n",
		mm_gaze->warpa_out_mode);

	switch (mm_gaze->proc_mode) {
	case MTK_WARPA_PROC_MODE_LR:
		mm_gaze->img_num = 2;
		break;
	case MTK_WARPA_PROC_MODE_L_ONLY:
		mm_gaze->img_num = 1;
		break;
	default:
		dev_err(mm_gaze->dev, "invalid proc_mode %d\n",
			mm_gaze->proc_mode);
		return -EINVAL;
	}

	ret =
	mtk_warpa_set_region_in(mm_gaze->dev_warpa,
		mm_gaze->warpa_in_w, mm_gaze->warpa_in_h);
	if (ret) {
		dev_err(mm_gaze->dev,
			"mtk_warpa_set_region_in fail!! %d\n", ret);
		return ret;
	}
	ret =
	mtk_warpa_set_region_out(mm_gaze->dev_warpa,
		mm_gaze->warpa_out_w, mm_gaze->warpa_out_h);
	if (ret) {
		dev_err(mm_gaze->dev,
			"mtk_warpa_set_region_out fail!! %d\n", ret);
		return ret;
	}
	ret =
	mtk_warpa_set_region_map(mm_gaze->dev_warpa,
		mm_gaze->warpa_map_w, mm_gaze->warpa_map_h);
	if (ret) {
		dev_err(mm_gaze->dev,
			"mtk_warpa_set_region_map fail!! %d\n", ret);
		return ret;
	}
	ret =
	mtk_warpa_set_coef_tbl(mm_gaze->dev_warpa, NULL,
		&mm_gaze->coef_tbl);
	if (ret) {
		dev_err(mm_gaze->dev,
			"mtk_warpa_set_coef_tbl fail!! %d\n", ret);
		return ret;
	}
	ret =
	mtk_warpa_set_border_color(mm_gaze->dev_warpa, NULL,
		&mm_gaze->border_color);
	if (ret) {
		dev_err(mm_gaze->dev,
			"mtk_warpa_set_border_color fail!! %d\n", ret);
		return ret;
	}
	ret =
	mtk_warpa_set_proc_mode(mm_gaze->dev_warpa, NULL,
		mm_gaze->proc_mode);
	if (ret) {
		dev_err(mm_gaze->dev,
			"mtk_warpa_set_proc_mode fail!! %d\n", ret);
		return ret;
	}

	ret = mtk_warpa_set_out_mode(
			mm_gaze->dev_warpa,
			mm_gaze->warpa_out_mode);
	if (ret) {
		dev_err(mm_gaze->dev,
			"mtk_warpa_set_out_mode fail!! %d\n", ret);
		return ret;
	}

	return ret;
}
static int mtk_mm_gaze_config_wdma
		(struct mtk_mm_gaze *mm_gaze)
{
	int ret = 0;
	int w, h;

	if (!mm_gaze || !mm_gaze->dev)
		return -EINVAL;

	w = mm_gaze->warpa_out_w;
	h = mm_gaze->warpa_out_h;
	dev_dbg(mm_gaze->dev, "gaze_wdma region w:%d h:%d\n",
		w, h);
	ret = mtk_wdma_set_region(mm_gaze->dev_gaze_wdma, NULL,
		w, h, w, h, 0, 0);
	if (ret) {
		dev_err(mm_gaze->dev,
			"set gaze wdma region fail!\n");
		return ret;
	}

	return ret;
}

int mtk_mm_gaze_config_camera_sync(struct mtk_mm_gaze *mm_gaze, int ps)
{
	u32 vsync_cycle = 0x10, delay_cycle = 100;
	u32 n = 0, m = 0;
	int ret = 0;
	int dsi_fps = 0;
	int cd;
	int timeout = 0;
	struct cmdq_pkt *pkt;

	if (ps)
		timeout = 1000;

	dsi_fps = mtk_dispsys_get_current_fps();
	while (!dsi_fps && timeout--) {
		/* wait for 5~10 sec for dp stable & frame tracking lock */
		usleep_range(5000, 10000);
		dsi_fps = mtk_dispsys_get_current_fps();
	}

	if (dsi_fps == 0) {
		dev_warn(mm_gaze->dev, "DSI not ready, set default value\n");
		dsi_fps = 60;
	}

	if (dsi_fps % 10 == 9)
		dsi_fps += 1;

	n = mm_gaze->sensor_fps;
	m = dsi_fps;
	cd = gcd(n, m);
	n /= cd;
	m /= cd;
	pr_info("set gaze camera sync n:%d, m:%d\n", n, m);
	pr_info("set gaze sensor fps:%d, dsi fps:%d\n", mm_gaze->sensor_fps,
						  dsi_fps);

	if (mm_gaze->delay_cycle)
		delay_cycle = mm_gaze->delay_cycle;
	pr_info("set gaze camera sync delay_cycle = %d\n", delay_cycle);

	/* because gce4 can't receive dsi vsync */
	/* use mutex timer event to cover it */
	/* set src = dsi vsync, ref = single sof (never come)*/
	/* and minimal timeout*/
	cmdq_pkt_create(&pkt);
	mtk_mutex_select_timer_sof(mm_gaze->mutex,
				MUTEX_GAZE_SOF_DSI0,
				MUTEX_GAZE_SOF_SINGLE, &pkt);
	mtk_mutex_set_timer_us(mm_gaze->mutex, 100000, 10, &pkt);
	mtk_mutex_timer_enable_ex(mm_gaze->mutex, false,
		MUTEX_TO_EVENT_REF_OVERFLOW, &pkt);
	cmdq_pkt_wfe(pkt, CMDQ_EVENT_MMSYS_GAZE_MUTEX_TD_EVENT_0);
	mtk_cv_enable(mm_gaze->dev_mmsys_core_top, MMSYSCFG_CAMERA_SYNC_GAZE0,
		      vsync_cycle, delay_cycle, (u8)n, (u8)m, &pkt);
	mtk_cv_enable(mm_gaze->dev_mmsys_core_top, MMSYSCFG_CAMERA_SYNC_GAZE1,
		      vsync_cycle, delay_cycle, (u8)n, (u8)m, &pkt);
	cmdq_pkt_flush(mm_gaze->cmdq_client_camera_sync, pkt);
	cmdq_pkt_destroy(pkt);

	return ret;
}

static int mtk_mm_gaze_module_config
		(struct mtk_mm_gaze *mm_gaze)
{
	int ret = 0;

	if (!mm_gaze || !mm_gaze->dev)
		return -EINVAL;

	mm_gaze->polling_wait = 1;
	frame_idx = 0;
	p2_sof_idx = 0;

	if (mm_gaze->mmgazemask & DEV_GAZE_CAMERA_WARPA_KICK_P2_DONE_GCE) {
		mtk_mm_gaze_cmdq_init(mm_gaze);
		mm_gaze->flush_state = 0;
		mm_gaze->flush_gaze0_p2_sof_state = 0;
#if RBFC_INCOMPLETE_DEBUG
		mm_gaze->flush_rbfc_ren_state = 0;
		mm_gaze->flush_gaze0_wen_state = 0;
		mm_gaze->flush_gaze1_wen_state = 0;
#endif
		mm_gaze->flow_done = 0;
	} else {
		schedule_work(&mm_gaze->trigger_work);
		mtk_gaze_register_cb(mm_gaze);
	}

	ret = mtk_mm_gaze_config_warpa(mm_gaze);
	if (ret) {
		dev_err(mm_gaze->dev, "mm gaze config warpa fail");
		return ret;
	}

	ret = mtk_mm_gaze_config_rbfc(mm_gaze);
	if (ret) {
		dev_err(mm_gaze->dev, "mm gaze config rbfc fail");
		return ret;
	}

	ret = mtk_mm_gaze_config_wdma(mm_gaze);
	if (ret) {
		dev_err(mm_gaze->dev, "mm gaze config wdma fail");
		return ret;
	}

	return ret;
}

static int mtk_mm_gaze_path_connect
		(struct mtk_mm_gaze *mm_gaze)
{
	int ret = 0;
	bool timing = false;

	if (!mm_gaze || !mm_gaze->dev)
		return -EINVAL;

	if (mm_gaze->mmgazemask & DEV_GAZE_CAMERA_MUTEX_EOF_TIMING) {
		dev_dbg(
			mm_gaze->dev, "select mutex eof timing is rasing\n");
		timing = true;
	}

	if (mm_gaze->mmgazemask & DEV_GAZE_CAMERA_FROM_SENSOR_MASK) {
		dev_dbg(mm_gaze->dev,
			"mm_gaze select wen w_ov_th sof!\n");
		ret = mtk_mutex_select_sof(mm_gaze->mutex,
			MUTEX_GAZE_SOF_RBFC_GAZE0_WPEA, NULL, timing);
		if (ret) {
			dev_err(mm_gaze->dev,
				"mutex_select_sof failed!! %d\n",
				ret);
			return ret;
		}
	} else {
		dev_dbg(mm_gaze->dev, "mm_gaze select single sof!\n");
		ret = mtk_mutex_select_sof(mm_gaze->mutex,
			MUTEX_GAZE_SOF_SINGLE, NULL, timing);
		if (ret) {
			dev_err(mm_gaze->dev,
				"mtk_mutex_select_sof failed!! %d\n",
				ret);
			return ret;
		}
	}

	dev_dbg(mm_gaze->dev, "mm gaze path connect\n");

	dev_dbg(mm_gaze->dev, "add mutex: RBFC_WPEA0\n");
	ret = mtk_mutex_add_comp(mm_gaze->mutex,
		MUTEX_COMPONENT_RBFC_WPEA0, NULL);
	if (ret) {
		dev_err(mm_gaze->dev,
			"mtk_mutex_add_comp rbfc failed!! %d\n",
			ret);
		return ret;
	}

	dev_dbg(mm_gaze->dev, "add mutex: WPEA0\n");
	ret = mtk_mutex_add_comp(mm_gaze->mutex,
		MUTEX_COMPONENT_WPEA0, NULL);
	if (ret) {
		dev_err(mm_gaze->dev,
			"mtk_mutex_add_comp wpea failed!! %d\n",
			ret);
		return ret;
	}

	dev_dbg(mm_gaze->dev, "add mutex: WDMA_GAZE\n");
	ret = mtk_mutex_add_comp(mm_gaze->mutex,
			MUTEX_COMPONENT_WDMA_GAZE, NULL);
	if (ret) {
		dev_err(mm_gaze->dev,
			"mtk_mutex_add_comp gazw_wdma failed!! %d\n",
			ret);
		return ret;
	}

#if GAZE_FOR_DEBUG
	ret = mtk_mutex_debug_enable(mm_gaze->dev_mutex, NULL);
	if (ret) {
		dev_err(mm_gaze->dev,
			"mtk_mutex_debug_enable gazw_wdma failed!! %d\n",
			ret);
		return ret;
	}
#endif

	return 0;
}

static int mtk_mm_gaze_path_disconnect
		(struct mtk_mm_gaze *mm_gaze)
{
	int ret = 0;

	if (!mm_gaze || !mm_gaze->dev)
		return -EINVAL;

	dev_dbg(mm_gaze->dev, "mm gaze path disconn\n");

	ret = mtk_mutex_remove_comp(mm_gaze->mutex,
			MUTEX_COMPONENT_WPEA0, NULL);
	if (ret) {
		dev_err(mm_gaze->dev,
			"mtk_mutex_remove_comp wpea failed!! %d\n",
			ret);
		return ret;
	}

	ret = mtk_mutex_remove_comp(mm_gaze->mutex,
			MUTEX_COMPONENT_RBFC_WPEA0, NULL);
	if (ret) {
		dev_err(mm_gaze->dev,
			"mtk_mutex_remove_comp rbfc failed!! %d\n",
			ret);
		return ret;
	}

	ret = mtk_mutex_remove_comp(mm_gaze->mutex,
			MUTEX_COMPONENT_WDMA_GAZE, NULL);
	if (ret) {
		dev_err(mm_gaze->dev,
			"mtk_mutex_remove_comp gaze_wdma failed!! %d\n",
			ret);
		return ret;
	}

	if (mm_gaze->mmgazemask & DEV_GAZE_CAMERA_WARPA_KICK_P2_DONE_GCE)
		mtk_mm_gaze_cmdq_deinit(mm_gaze);

	return 0;
}

int mtk_mm_gaze_module_power_on(struct mtk_mm_gaze *mm_gaze)
{
	int ret = 0;

	ret = mtk_warpa_power_on(mm_gaze->dev_warpa);
	if (ret) {
		dev_err(mm_gaze->dev,
			"warpa_power_on failed!! %d\n",
			ret);
		return ret;
	}

	ret = mtk_rbfc_power_on(mm_gaze->dev_rbfc);
	if (ret) {
		dev_err(mm_gaze->dev,
			"rbfc_power_on failed!! %d\n",
			ret);
		return ret;
	}

	mm_gaze->mutex = mtk_mutex_get(mm_gaze->dev_mutex,
			0);
	if (IS_ERR(mm_gaze->mutex)) {
		ret =
		PTR_ERR(mm_gaze->mutex);
		dev_err(mm_gaze->dev,
			"mutex_get failed!! %d\n",
			ret);
		return ret;
	}

	ret = mtk_mutex_power_on(mm_gaze->mutex);
	if (ret) {
		dev_err(mm_gaze->dev,
			"mutex_power_on failed!! %d\n",
			ret);
		return ret;
	}

	ret = mtk_wdma_power_on(mm_gaze->dev_gaze_wdma);
	if (ret) {
		dev_err(mm_gaze->dev,
			"wdma_power_on failed!! %d\n",
			ret);
		return ret;
	}

	if (mm_gaze->mmgazemask & DEV_GAZE_CAMERA_RBFC_FROM_SYSRAM_MASK) {
		ret = mtk_mm_sysram_power_on(mm_gaze->dev_sysram,
					     GAZE_WARPA_IN_SRAM_ADDR,
					     GAZE_SYSRAM1_END_ADDR);
		if (ret)
			dev_err(mm_gaze->dev,
				"mmsys gaze sysram1 power on failed! %d\n",
				ret);
		return ret;
	}

	return 0;
}

int mtk_mm_gaze_module_power_off(struct mtk_mm_gaze *mm_gaze)
{
	int ret = 0;

	ret = mtk_wdma_power_off(mm_gaze->dev_gaze_wdma);
	if (ret) {
		dev_err(mm_gaze->dev,
			"wdma_power_off failed!! %d\n",
			ret);
		return ret;
	}

	ret = mtk_warpa_power_off(mm_gaze->dev_warpa);
	if (ret) {
		dev_err(mm_gaze->dev,
			"warpa_power_off failed!! %d\n",
			ret);
		return ret;
	}

	ret = mtk_rbfc_power_off(mm_gaze->dev_rbfc);
	if (ret) {
		dev_err(mm_gaze->dev,
			"rbfc_power_off failed!! %d\n",
			ret);
		return ret;
	}

	ret = mtk_mutex_power_off(mm_gaze->mutex);
	if (ret) {
		dev_err(mm_gaze->dev,
			"mutex_power_off fail!! %d\n",
			ret);
		return ret;
	}

	ret = mtk_mutex_put(mm_gaze->mutex);
	if (ret) {
		dev_err(mm_gaze->dev,
			"mutex_put failed!! %d\n",
			ret);
		return ret;
	}
	mm_gaze->mutex = NULL;

	if (mm_gaze->mmgazemask & DEV_GAZE_CAMERA_RBFC_FROM_SYSRAM_MASK) {
		ret = mtk_mm_sysram_power_off(mm_gaze->dev_sysram,
						 GAZE_WARPA_IN_SRAM_ADDR,
						 GAZE_SYSRAM1_END_ADDR);
		if (ret) {
			dev_err(mm_gaze->dev,
				"mmsys gaze sysram1 power off failed! %d\n",
				ret);
			return ret;
		}
	}

	return 0;
}

static int mtk_mm_gaze_module_start
		(struct mtk_mm_gaze *mm_gaze)
{
	int ret = 0;

	if (!mm_gaze || !mm_gaze->dev)
		return -EINVAL;

	dev_dbg(mm_gaze->dev,
		"mtk_mm_gaze_module_start\n");

	ret = mtk_warpa_start(mm_gaze->dev_warpa, NULL);
	if (ret) {
		dev_err(mm_gaze->dev,
			"mtk_warpa_start failed!! %d\n", ret);
		return ret;
	}

	ret = mtk_wdma_start(mm_gaze->dev_gaze_wdma, NULL);
	if (ret) {
		dev_err(mm_gaze->dev,
			"mtk_wdma_start failed!! %d\n", ret);
		return ret;
	}

	return ret;
}

static void wdma_dump_buf(struct mtk_mm_gaze *mm_gaze, int buf_idx, int size,
	char *file_path)
{
	pa_to_vaddr_dev(mm_gaze->dev_gaze_wdma,
				mm_gaze->buf[buf_idx], size, true);

	GZ_LOG(CV_LOG_INFO, "dump idx:%d, size:%d @%p\n",
		buf_idx, size, (void *)mm_gaze->buf[buf_idx]->kvaddr);
	mtk_common_write_file(mm_gaze->buf[buf_idx]->kvaddr, file_path, size);
	pa_to_vaddr_dev(mm_gaze->dev_gaze_wdma,
				mm_gaze->buf[buf_idx], size, false);
}

static void warpa_dump_buf(struct mtk_mm_gaze *mm_gaze, int buf_idx, int size,
	char *file_path)
{
	pa_to_vaddr_dev(mm_gaze->dev_warpa, mm_gaze->buf[buf_idx], size, true);
	if ((mm_gaze->mmgazemask & DEV_GAZE_CAMERA_RBFC_FROM_SYSRAM_MASK) &&
		buf_idx < 2) {
		move_sram_to_va(mm_gaze->dev_sysram, GAZE_WARPA_IN_SRAM_ADDR,
			(void *)mm_gaze->buf[buf_idx]->kvaddr, size);
		GZ_LOG(CV_LOG_INFO, "move sram_to_va done!\n");
	}

	GZ_LOG(CV_LOG_INFO, "dump idx:%d, size:%d @%p\n",
		buf_idx, size, (void *)mm_gaze->buf[buf_idx]->kvaddr);
	mtk_common_write_file(mm_gaze->buf[buf_idx]->kvaddr, file_path, size);
	pa_to_vaddr_dev(mm_gaze->dev_warpa, mm_gaze->buf[buf_idx], size, false);
}

static void gaze_dump_buf(struct mtk_mm_gaze *mm_gaze, int frame_idx)
{
	char file_path[256];

	if (mm_gaze->mmgazemask & DEV_GAZE_CAMERA_WARPA_INPUT_DUMP) {
		/* start to dump warpa input buffer */
		if (mm_gaze->mmgazemask &
			DEV_GAZE_CAMERA_RBFC_FROM_SYSRAM_MASK) {
			sprintf(file_path, "%s/dump_warp_input0_sram_%d.bin",
				mm_gaze->dump_path, frame_idx);
			GZ_LOG(CV_LOG_DBG, "start to dump_warp_input0_sram\n");
		} else {
			sprintf(file_path, "%s/dump_warp_input0_dram_%d.bin",
				mm_gaze->dump_path, frame_idx);
			GZ_LOG(CV_LOG_DBG, "start to dump_warp_input0_dram\n");
		}
		warpa_dump_buf(mm_gaze,
			MTK_GAZE_CAMERA_BUF_TYPE_WARPA_IMG_BUF_0,
			mm_gaze->warp_input_buf_size, file_path);
	}

	if (mm_gaze->mmgazemask & DEV_GAZE_CAMERA_WARPA_MAP_BUF_DUMP) {
		/* start to dump warpa map buffer */
		sprintf(file_path, "%s/dump_gaze_warp_map_0.bin",
			mm_gaze->dump_path);
		GZ_LOG(CV_LOG_DBG, "start to dump_gaze_warp_map_0\n");
		warpa_dump_buf(mm_gaze,
			MTK_GAZE_CAMERA_BUF_TYPE_WARPA_MAP_0,
			mm_gaze->warp_map_size, file_path);

		sprintf(file_path, "%s/dump_gaze_warp_map_1.bin",
			mm_gaze->dump_path);
		GZ_LOG(CV_LOG_DBG, "start to dump_gaze_warp_map_1\n");
		warpa_dump_buf(mm_gaze,
			MTK_GAZE_CAMERA_BUF_TYPE_WARPA_MAP_1,
			mm_gaze->warp_map_size, file_path);
	}

	if ((mm_gaze->mmgazemask & DEV_GAZE_CAMERA_DUMP_FILE_ENABLE) ||
		mm_gaze->one_shot_dump) {
		/* start to dump buffer */
		sprintf(file_path, "%s/dump_gaze_wdma_%dx%d_%d.bin",
			mm_gaze->dump_path, mm_gaze->wdma_buf_pitch,
			mm_gaze->warpa_out_h, frame_idx);
		GZ_LOG(CV_LOG_DBG, "start to gaze_dump_buf\n");
		wdma_dump_buf(mm_gaze, MTK_GAZE_CAMERA_BUF_TYPE_WDMA,
			mm_gaze->wdma_buf_size, file_path);

		if (mm_gaze->mmgazemask & DEV_GAZE_CAMERA_WAIT_VPU_DONE) {
			sprintf(file_path,
				"%s/dump_gaze_wdma_dummy_%dx%d_%d.bin",
				mm_gaze->dump_path, mm_gaze->wdma_buf_pitch,
				mm_gaze->warpa_out_h, frame_idx);
			GZ_LOG(CV_LOG_DBG, "start to gaze_dump_dummy_buf\n");
			wdma_dump_buf(mm_gaze,
				MTK_GAZE_CAMERA_BUF_TYPE_WDMA_DUMMY,
				mm_gaze->wdma_buf_size, file_path);
		}
		mm_gaze->one_shot_dump = 0;
	}

	if (mm_gaze->mmgazemask & DEV_GAZE_CAMERA_DBG_TEST_MASK)
		mtk_mmsys_gaze_top_get_cksm(mm_gaze->dev_mmsys_gaze_top,
			&mm_gaze->wdma_dbg);
}

static void mtk_mm_gaze_trigger_work(struct work_struct *work)
{
	struct mtk_mm_gaze *mm_gaze =
			container_of(work, struct mtk_mm_gaze, trigger_work);

	GZ_LOG(CV_LOG_DBG,
				"mm_gaze_trigger_work: start wait!!\n");
	while (mm_gaze->polling_wait) {
		if (!mm_gaze->polling_wait) {
			GZ_LOG(CV_LOG_DBG, "polling quit flow!\n");
			break;
		}
		usleep_range(5000, 10000);
	}
	GZ_LOG(CV_LOG_DBG, "mm_gaze_trigger_work: wdma done! & exit\n");
}

static void print_done_interval(char *module_name, struct timeval prev,
	struct timeval cur)
{
	suseconds_t diff;

	diff = cur.tv_sec * 1000000 - prev.tv_sec * 1000000 +
		cur.tv_usec - prev.tv_usec;
	GZ_LOG(CV_LOG_DBG, "%s Done intarval: %lu us\n", module_name, diff);
}

static void gaze_work_cb(struct cmdq_cb_data data)
{
	struct mtk_mm_gaze *mm_gaze  = data.data;

	complete(&mm_gaze->gaze_flush_comp.cmplt);
}
static void mtk_mm_gaze_trigger_work_gce(struct work_struct *work)
{
	struct mtk_mm_gaze *mm_gaze =
		container_of(work, struct mtk_mm_gaze, trigger_work_gce);
	struct device_node *node = NULL;
	struct platform_device *device = NULL;
	struct timeval prev_frame;
	struct timeval cur_frame;
	u8 gce4_subsys = SUBSYS_1031XXXX;
#ifdef CONFIG_MACH_MT3612_A0
	u32 vpu_done_event = CMDQ_EVENT_VPU_2_IRQ;
#else
	/* FPGA only have ivpcore 0*/
	u32 vpu_done_event = CMDQ_EVENT_VPU_0_IRQ;
#endif
	node = of_find_compatible_node(NULL, NULL,
			"mediatek,mt3612-ivp");
	device = of_find_device_by_node(node);

	if (mm_gaze->wdma_buf_size <= 0)
		mm_gaze->wdma_buf_size =
			mm_gaze->wdma_buf_pitch * mm_gaze->warpa_out_h;
	GZ_LOG(CV_LOG_DBG,
			"mm_gaze_trigger_work: start wait!!\n");
	init_completion(&mm_gaze->gaze_flush_comp.cmplt);
	cmdq_pkt_create(&mm_gaze->cmdq_handle);

	while (mm_gaze->polling_wait) {
		/* wait for wdma frame done */
		reinit_completion(&mm_gaze->gaze_flush_comp.cmplt);
		cmdq_pkt_multiple_reset(&mm_gaze->cmdq_handle, 1);
		cmdq_pkt_clear_event(mm_gaze->cmdq_handle,
			CMDQ_EVENT_MMSYS_GAZE_FRAME_DONE_1_WDMA_GAZE);
		cmdq_pkt_clear_token(mm_gaze->cmdq_handle,
			GCE_SW_TOKEN_GZ_WDMA_DONE, gce4_subsys);
		GZ_LOG(CV_LOG_DBG, "Clear GAZE wdma Frame done\n");

		cmdq_pkt_wfe(mm_gaze->cmdq_handle,
			CMDQ_EVENT_MMSYS_GAZE_FRAME_DONE_1_WDMA_GAZE);
		cmdq_pkt_set_token(mm_gaze->cmdq_handle,
			GCE_SW_TOKEN_GZ_WDMA_DONE, gce4_subsys);
		GZ_LOG(CV_LOG_DBG, "Waiting for GAZE WDMA Frame done...\n");
		cmdq_pkt_flush_async(mm_gaze->cmdq_client, mm_gaze->cmdq_handle,
			gaze_work_cb, mm_gaze);
		mm_gaze->flush_state = 1;
		wait_for_completion_interruptible(
			&mm_gaze->gaze_flush_comp.cmplt);
		mm_gaze->flush_state = 0;
		prev_frame = cur_frame;
		do_gettimeofday(&cur_frame);
		if (mm_gaze->flow_done)
			print_done_interval("gaze", prev_frame, cur_frame);
		GZ_LOG(CV_LOG_DBG, "GAZE WDMA Frame flush done\n");
		if (!mm_gaze->polling_wait)
			break;

		/* wait for vpu frame done */
		if (mm_gaze->mmgazemask & DEV_GAZE_CAMERA_WAIT_VPU_DONE) {
			reinit_completion(&mm_gaze->gaze_flush_comp.cmplt);
			cmdq_pkt_multiple_reset(&mm_gaze->cmdq_handle, 1);
			cmdq_pkt_clear_event(mm_gaze->cmdq_handle,
				vpu_done_event);
			GZ_LOG(CV_LOG_DBG, "Clear vpu event ID = %d\n",
					vpu_done_event);
			mtk_send_ivp_request(device,
				mm_gaze->process_GAZE_WDMA.handle,
				mm_gaze->process_GAZE_WDMA.coreid,
				mm_gaze->cmdq_handle);
			GZ_LOG(CV_LOG_DBG,
				"Waiting for GAZE VPU process done...\n");
			cmdq_pkt_wfe(mm_gaze->cmdq_handle,
				vpu_done_event);
			mtk_clr_ivp_request(device,
				mm_gaze->process_GAZE_WDMA.handle,
				mm_gaze->process_GAZE_WDMA.coreid,
				mm_gaze->cmdq_handle);
			cmdq_pkt_flush_async(mm_gaze->cmdq_client,
				mm_gaze->cmdq_handle, gaze_work_cb, mm_gaze);
			mm_gaze->flush_state = 1;
			wait_for_completion_interruptible(
				&mm_gaze->gaze_flush_comp.cmplt);
			mm_gaze->flush_state = 0;
			GZ_LOG(CV_LOG_DBG, "GAZE VPU flush done\n");
			if (!mm_gaze->polling_wait)
				break;
		}

		gaze_dump_buf(mm_gaze, frame_idx);
		if (!mm_gaze->polling_wait)
			break;
		frame_idx++;
		mm_gaze->flow_done = 1;
	}
	cmdq_pkt_destroy(mm_gaze->cmdq_handle);
	mm_gaze->cmdq_handle = NULL;
	GZ_LOG(CV_LOG_INFO, "gaze_trigger_work_gce: polling quit flow!\n");
}

#if RBFC_INCOMPLETE_DEBUG
static void ren_rbfc_incomplete_cb(struct cmdq_cb_data data)
{
	struct mtk_mm_gaze *mm_gaze  = data.data;

	cmdq_pkt_destroy(mm_gaze->cmdq_handle_rbfc_ren);
	mm_gaze->cmdq_handle_rbfc_ren = NULL;
	complete(&mm_gaze->ren_rbfc_flush_comp.cmplt);
}
static void mtk_gaze_wpe_ren_rbfc_incomplete_detect(struct work_struct *work)
{
	struct mtk_mm_gaze *mm_gaze =
		container_of(work, struct mtk_mm_gaze, trigger_rbfc_ren);
	u32 rbfc_incomplete_event;
	int incomplete_cnt = 0;

	rbfc_incomplete_event =
		CMDQ_EVENT_MMSYS_GAZE_RBFC_REN_WPE_0_RBFC_R_INCOMPLETE_EVENT;

	init_completion(&mm_gaze->ren_rbfc_flush_comp.cmplt);
	GZ_LOG(CV_LOG_DBG,
			"mtk_gaze_wpe_ren_rbfc_incomplete_detect: start!\n");

	while (mm_gaze->polling_wait) {
		/* detect wpe ren rbfc incomplete */
		reinit_completion(&mm_gaze->ren_rbfc_flush_comp.cmplt);
		cmdq_pkt_create(&mm_gaze->cmdq_handle_rbfc_ren);
		cmdq_pkt_clear_event(mm_gaze->cmdq_handle_rbfc_ren,
			rbfc_incomplete_event);
		GZ_LOG(CV_LOG_DBG,
			"Clear GAZE wpe ren rbfc incomplete event id:%d\n",
			rbfc_incomplete_event);

		cmdq_pkt_wfe(mm_gaze->cmdq_handle_rbfc_ren,
			rbfc_incomplete_event);
		GZ_LOG(CV_LOG_DBG,
			"Waiting for ren rbfc incomplete event id:%d...\n",
			rbfc_incomplete_event);

		cmdq_pkt_flush_async(mm_gaze->cmdq_client_rbfc_ren,
			mm_gaze->cmdq_handle_rbfc_ren,
			ren_rbfc_incomplete_cb, mm_gaze);
		mm_gaze->flush_rbfc_ren_state = 1;
		wait_for_completion_interruptible(
			&mm_gaze->ren_rbfc_flush_comp.cmplt);
		mm_gaze->flush_rbfc_ren_state = 0;
		if (!mm_gaze->polling_wait)
			break;

		GZ_LOG(CV_LOG_ERR,
			"ERROR: gaze_wpe_ren_rbfc_incomplete happen counts: %d\n",
			incomplete_cnt);
		incomplete_cnt++;
	}
	GZ_LOG(CV_LOG_DBG, "gaze_wpe_ren_rbfc_incomplete_detect: done\n");
	GZ_LOG(CV_LOG_DBG,
			"mtk_gaze_wpe_ren_rbfc_incomplete_detect: polling quit flow!!\n");
}

static void gaze0_wen_incomplete_cb(struct cmdq_cb_data data)
{
	struct mtk_mm_gaze *mm_gaze  = data.data;

	cmdq_pkt_destroy(mm_gaze->cmdq_handle_gaze0_wen);
	mm_gaze->cmdq_handle_gaze0_wen = NULL;
	complete(&mm_gaze->gaze0_wen_flush_comp.cmplt);
}
static void mtk_gaze0_wen_rbfc_incomplete_detect(struct work_struct *work)
{
	struct mtk_mm_gaze *mm_gaze =
		container_of(work, struct mtk_mm_gaze, trigger_gaze0_wen);
	u32 rbfc_incomplete_event;
	int incomplete_cnt = 0;

	rbfc_incomplete_event =
		CMDQ_EVENT_ISP_GAZE0_CAM0_RBFC_IMG3O_W_RBFC_W_INCOMPLETE_EVENT;

	init_completion(&mm_gaze->gaze0_wen_flush_comp.cmplt);
	GZ_LOG(CV_LOG_DBG,
			"mtk_gaze0_wen_rbfc_incomplete_detect: start!\n");

	while (mm_gaze->polling_wait) {
		/* detect gaze0 wen rbfc incomplete */
		reinit_completion(&mm_gaze->gaze0_wen_flush_comp.cmplt);
		cmdq_pkt_create(&mm_gaze->cmdq_handle_gaze0_wen);
		cmdq_pkt_clear_event(mm_gaze->cmdq_handle_gaze0_wen,
			rbfc_incomplete_event);
		GZ_LOG(CV_LOG_DBG,
			"Clear GAZE0 wen incomplete event id:%d\n",
			rbfc_incomplete_event);

		cmdq_pkt_wfe(mm_gaze->cmdq_handle_gaze0_wen,
			rbfc_incomplete_event);
		GZ_LOG(CV_LOG_DBG,
			"Waiting for GAZE0 wen incomplete event id:%d...\n",
			rbfc_incomplete_event);

		cmdq_pkt_flush_async(mm_gaze->cmdq_client_gaze0_wen,
			mm_gaze->cmdq_handle_gaze0_wen,
			gaze0_wen_incomplete_cb, mm_gaze);
		mm_gaze->flush_gaze0_wen_state = 1;
		wait_for_completion_interruptible(
			&mm_gaze->gaze0_wen_flush_comp.cmplt);
		mm_gaze->flush_gaze0_wen_state = 0;
		if (!mm_gaze->polling_wait)
			break;

		GZ_LOG(CV_LOG_ERR,
			"ERROR: gaze0_wen_rbfc_incomplete happen counts: %d\n",
			incomplete_cnt);
		incomplete_cnt++;
	}
	GZ_LOG(CV_LOG_DBG, "gaze0_wen_rbfc_incomplete_detect: done\n");
	GZ_LOG(CV_LOG_DBG,
			"mtk_gaze0_wen_rbfc_incomplete_detect: polling quit flow!!\n");
}

static void gaze1_wen_incomplete_cb(struct cmdq_cb_data data)
{
	struct mtk_mm_gaze *mm_gaze  = data.data;

	cmdq_pkt_destroy(mm_gaze->cmdq_handle_gaze1_wen);
	mm_gaze->cmdq_handle_gaze1_wen = NULL;
	complete(&mm_gaze->gaze1_wen_flush_comp.cmplt);
}
static void mtk_gaze1_wen_rbfc_incomplete_detect(struct work_struct *work)
{
	struct mtk_mm_gaze *mm_gaze =
		container_of(work, struct mtk_mm_gaze, trigger_gaze1_wen);
	u32 rbfc_incomplete_event;
	int incomplete_cnt = 0;

	rbfc_incomplete_event =
		CMDQ_EVENT_ISP_GAZE1_CAM1_RBFC_IMG3O_W_RBFC_W_INCOMPLETE_EVENT;

	init_completion(&mm_gaze->gaze1_wen_flush_comp.cmplt);
	GZ_LOG(CV_LOG_DBG,
			"mtk_gaze1_wen_rbfc_incomplete_detect: start!\n");

	while (mm_gaze->polling_wait) {
		/* detect gaze1 wen rbfc incomplete */
		reinit_completion(&mm_gaze->gaze1_wen_flush_comp.cmplt);
		cmdq_pkt_create(&mm_gaze->cmdq_handle_gaze1_wen);
		cmdq_pkt_clear_event(mm_gaze->cmdq_handle_gaze1_wen,
			rbfc_incomplete_event);
		GZ_LOG(CV_LOG_DBG,
			"Clear GAZE1 wen incomplete event id:%d\n",
			rbfc_incomplete_event);

		cmdq_pkt_wfe(mm_gaze->cmdq_handle_gaze1_wen,
			rbfc_incomplete_event);
		GZ_LOG(CV_LOG_DBG,
			"Waiting for GAZE1 wen incomplete event id:%d...\n",
			rbfc_incomplete_event);

		cmdq_pkt_flush_async(mm_gaze->cmdq_client_gaze1_wen,
			mm_gaze->cmdq_handle_gaze1_wen,
			gaze1_wen_incomplete_cb, mm_gaze);
		mm_gaze->flush_gaze1_wen_state = 1;
		wait_for_completion_interruptible(
			&mm_gaze->gaze1_wen_flush_comp.cmplt);
		mm_gaze->flush_gaze1_wen_state = 0;
		if (!mm_gaze->polling_wait)
			break;

		GZ_LOG(CV_LOG_ERR,
			"ERROR: gaze1_wen_rbfc_incomplete happen counts: %d\n",
			incomplete_cnt);
		incomplete_cnt++;
	}
	GZ_LOG(CV_LOG_DBG, "gaze1_wen_rbfc_incomplete_detect: done\n");
	GZ_LOG(CV_LOG_DBG,
			"mtk_gaze1_wen_rbfc_incomplete_detect: polling quit flow!!\n");
}
#endif
static void gaze0_p2_sof_cb(struct cmdq_cb_data data)
{
	struct mtk_mm_gaze *mm_gaze  = data.data;

	complete(&mm_gaze->gaze0_p2_sof_flush_comp.cmplt);
}
static void mtk_gaze0_p2_sof_check(struct work_struct *work)
{
	struct mtk_mm_gaze *mm_gaze =
		container_of(work, struct mtk_mm_gaze, trigger_gaze0_p2_sof);
	u32 gaze0_p2_sof;
	u8 gce4_subsys = SUBSYS_1031XXXX;

	gaze0_p2_sof = CMDQ_EVENT_ISP_GAZE0_CAM0_P2_SOF_DONE;

	init_completion(&mm_gaze->gaze0_p2_sof_flush_comp.cmplt);
	GZ_LOG(CV_LOG_DBG,
			"mtk_gaze0_p2_sof_check: start!\n");
	cmdq_pkt_create(&mm_gaze->cmdq_handle_gaze0_p2_sof);

	while (mm_gaze->polling_wait) {
		/* check gaze0 isp p2 sof count */
		reinit_completion(&mm_gaze->gaze0_p2_sof_flush_comp.cmplt);
		cmdq_pkt_multiple_reset(&mm_gaze->cmdq_handle_gaze0_p2_sof, 1);
		cmdq_pkt_clear_event(mm_gaze->cmdq_handle_gaze0_p2_sof,
			gaze0_p2_sof);
		cmdq_pkt_clear_token(mm_gaze->cmdq_handle_gaze0_p2_sof,
			GCE_SW_TOKEN_GZ_P2_SOF, gce4_subsys);
		GZ_LOG(CV_LOG_DBG,
			"Clear gaze0 p2 sof id:%d\n",
			gaze0_p2_sof);

		cmdq_pkt_wfe(mm_gaze->cmdq_handle_gaze0_p2_sof,
			gaze0_p2_sof);
		cmdq_pkt_set_token(mm_gaze->cmdq_handle_gaze0_p2_sof,
			GCE_SW_TOKEN_GZ_P2_SOF, gce4_subsys);
		GZ_LOG(CV_LOG_DBG,
			"Waiting for gaze0 p2 sof event id:%d...\n",
			gaze0_p2_sof);

		cmdq_pkt_flush_async(mm_gaze->cmdq_client_gaze0_p2_sof,
			mm_gaze->cmdq_handle_gaze0_p2_sof,
			gaze0_p2_sof_cb, mm_gaze);
		mm_gaze->flush_gaze0_p2_sof_state = 1;
		wait_for_completion_interruptible(
			&mm_gaze->gaze0_p2_sof_flush_comp.cmplt);
		mm_gaze->flush_gaze0_p2_sof_state = 0;
		if (!mm_gaze->polling_wait)
			break;

		GZ_LOG(CV_LOG_DBG, "gaze0 p2 sof counts: %d\n",
			p2_sof_idx);
		p2_sof_idx++;
	}
	cmdq_pkt_destroy(mm_gaze->cmdq_handle_gaze0_p2_sof);
	mm_gaze->cmdq_handle_gaze0_p2_sof = NULL;
	GZ_LOG(CV_LOG_DBG, "mtk_gaze0_p2_sof_check: done\n");
	GZ_LOG(CV_LOG_DBG,
			"mtk_gaze0_p2_sof_check: polling quit flow!!\n");
}
static int mtk_mm_gaze_path_stop
		(struct mtk_mm_gaze *mm_gaze)
{
	int ret = 0;

	if (!mm_gaze || !mm_gaze->dev)
		return -EINVAL;

	dev_dbg(mm_gaze->dev,
		"mtk_mm_gaze_path_stop\n");
#if 0
	ret = mtk_mutex_disable(mm_gaze->mutex, NULL);
#endif
	ret = mtk_warpa_stop(mm_gaze->dev_warpa, NULL);
	if (ret) {
		dev_err(mm_gaze->dev,
			"mtk_warpa_stop failed!! %d\n", ret);
		return ret;
	}

	ret = mtk_wdma_stop(mm_gaze->dev_gaze_wdma, NULL);
	if (ret) {
		dev_err(mm_gaze->dev,
			"mtk_wdma_stop failed!! %d\n", ret);
		return ret;
	}

	if (mm_gaze->mmgazemask & DEV_GAZE_CAMERA_ENABLE_RBFC_MASK)
		ret = mtk_rbfc_finish(mm_gaze->dev_rbfc);
	if (ret) {
		dev_err(mm_gaze->dev,
			"rbfc_finish failed!! %d\n", ret);
		return ret;
	}

	return ret;
}

static int mtk_mm_gaze_path_trigger
		(struct mtk_mm_gaze *mm_gaze, bool onoff)
{
	int ret = 0;

	if (!mm_gaze || !mm_gaze->dev)
		return -EINVAL;

	if (onoff) {
		dev_dbg(mm_gaze->dev,
		"gaze_camera_path_trigger enable\n");
		ret = mtk_mutex_enable(mm_gaze->mutex, NULL);
		if (ret) {
			dev_err(mm_gaze->dev, "mutex_enable fail!! %d\n",
				ret);
			return ret;
		}
	} else {
		dev_dbg(mm_gaze->dev,
		"gaze_camera_path_trigger disable\n");
		ret = mtk_mutex_disable(mm_gaze->mutex, NULL);
		if (ret) {
			dev_err(mm_gaze->dev, "mutex_disable fail!! %d\n",
				ret);
			return ret;
		}

		mm_gaze->polling_wait = 0;

		/* clear cb */
		mtk_wdma_register_cb(mm_gaze->dev_gaze_wdma,
				mtk_gaze_wdma_test_cb, 0, mm_gaze);
		mtk_warpa_register_cb(mm_gaze->dev_warpa,
				mtk_gaze_warpa_test_cb, 0, mm_gaze);
	}

	return ret;
}

static int mtk_mm_gaze_path_gce_trigger
		(struct mtk_mm_gaze *mm_gaze, bool onoff)
{
	int ret = 0;
	int i;

	if (!mm_gaze || !mm_gaze->dev)
		return -EINVAL;

	if (onoff) {
		dev_dbg(mm_gaze->dev, "enable trigger_work_gce\n");
		schedule_work(&mm_gaze->trigger_work_gce);
		schedule_work(&mm_gaze->trigger_gaze0_p2_sof);
#if RBFC_INCOMPLETE_DEBUG
		schedule_work(&mm_gaze->trigger_rbfc_ren);
		schedule_work(&mm_gaze->trigger_gaze0_wen);
		schedule_work(&mm_gaze->trigger_gaze1_wen);
#endif
		ret = mtk_mutex_enable(mm_gaze->mutex, NULL);
		if (ret) {
			dev_err(mm_gaze->dev, "mutex_enable fail!! %d\n",
				ret);
			return ret;
		}
	} else {
		dev_dbg(mm_gaze->dev, "disable trigger_work_gce\n");
		/* wait for work queue into sync flush*/
		mm_gaze->polling_wait = 0;
		for (i = 0; i < FLOW_TIMEOUT; i++) {
			if (mm_gaze->flush_state)
				break;
			usleep_range(5000, 10000);
		}
#if RBFC_INCOMPLETE_DEBUG
		complete(&mm_gaze->ren_rbfc_flush_comp.cmplt);
		for (i = 0; i < FLOW_TIMEOUT; i++) {
			if (mm_gaze->flush_rbfc_ren_state == 0)
				break;
			usleep_range(5000, 10000);
		}

		complete(&mm_gaze->gaze0_wen_flush_comp.cmplt);
		for (i = 0; i < FLOW_TIMEOUT; i++) {
			if (mm_gaze->flush_gaze0_wen_state == 0)
				break;
			usleep_range(5000, 10000);
		}

		complete(&mm_gaze->gaze1_wen_flush_comp.cmplt);
		for (i = 0; i < FLOW_TIMEOUT; i++) {
			if (mm_gaze->flush_gaze1_wen_state == 0)
				break;
			usleep_range(5000, 10000);
		}
#endif
		ret = mtk_mutex_disable(mm_gaze->mutex, NULL);
		if (ret) {
			dev_err(mm_gaze->dev, "mutex_disable fail!! %d\n",
				ret);
			return ret;
		}

		dev_dbg(mm_gaze->dev, "disable trigger_work_gce done\n");
	}

	return ret;
}

static int mtk_mm_gaze_module_set_buf(struct mtk_mm_gaze *mm_gaze)
{
	int ret = 0;
	dma_addr_t tmp_pa;

	if (!mm_gaze || !mm_gaze->dev)
		return -EINVAL;

	pr_debug("set buf: w = %d, h = %d, size = %d\n",
		 mm_gaze->warpa_in_w,
		 mm_gaze->warpa_in_h,
		 mm_gaze->warp_input_buf_size);

	if (mm_gaze->mmgazemask & DEV_GAZE_CAMERA_RBFC_FROM_SYSRAM_MASK) {
		tmp_pa = GAZE_WARPA_IN_SRAM_ADDR;
		dev_dbg(mm_gaze->dev,
			"set buf MTK_GAZE_CAMERA_BUF_TYPE_WARPA_IMG_BUF_0 @0x%llx, pitch:%d\n",
			tmp_pa,
			mm_gaze->buf
			[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_IMG_BUF_0]->pitch);
		ret = mtk_warpa_set_buf_in_no0(mm_gaze->dev_warpa, NULL,
			tmp_pa, mm_gaze->
			buf[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_IMG_BUF_0]->pitch);
	} else {
		dev_dbg(mm_gaze->dev,
			"set buf MTK_GAZE_CAMERA_BUF_TYPE_WARPA_IMG_BUF_0 @0x%llx, pitch:%d\n",
			mm_gaze->buf
			[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_IMG_BUF_0]->dma_addr,
			mm_gaze->buf
			[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_IMG_BUF_0]->pitch);
		ret = mtk_warpa_set_buf_in_no0(mm_gaze->dev_warpa, NULL,
			mm_gaze->buf[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_IMG_BUF_0]->
			dma_addr, mm_gaze->
			buf[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_IMG_BUF_0]->pitch);
	}
	if (ret) {
		dev_err(mm_gaze->dev,
		"MTK_GAZE_CAMERA_BUF_TYPE_WARPA_IMG_BUF_0 fail!! %d\n", ret);
		return ret;
	}

	if (mm_gaze->mmgazemask & DEV_GAZE_CAMERA_RBFC_FROM_SYSRAM_MASK) {
		tmp_pa = GAZE_WARPA_IN_SRAM_ADDR +
			roundup(mm_gaze->warpa_in_w, mm_gaze->warpa_align);
		dev_dbg(mm_gaze->dev,
			"set buf MTK_GAZE_CAMERA_BUF_TYPE_WARPA_IMG_BUF_1 @0x%llx, pitch:%d\n",
			tmp_pa,
			mm_gaze->buf
			[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_IMG_BUF_1]->pitch);
		ret = mtk_warpa_set_buf_in_no1(mm_gaze->dev_warpa, NULL,
			tmp_pa, mm_gaze->
			buf[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_IMG_BUF_1]->pitch);
	} else {
		dev_dbg(mm_gaze->dev,
		"set buf MTK_GAZE_CAMERA_BUF_TYPE_WARPA_IMG_BUF_1 @0x%llx, pitch:%d\n",
			mm_gaze->
			buf[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_IMG_BUF_1]->dma_addr,
			mm_gaze->
			buf[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_IMG_BUF_1]->pitch);
		ret = mtk_warpa_set_buf_in_no1(mm_gaze->dev_warpa, NULL,
			mm_gaze->
			buf[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_IMG_BUF_1]->dma_addr,
			mm_gaze->
			buf[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_IMG_BUF_1]->pitch);
	}
	if (ret) {
		dev_err(mm_gaze->dev,
		"MTK_GAZE_CAMERA_BUF_TYPE_WARPA_IMG_BUF_1 fail!! %d\n", ret);
		return ret;
	}

	dev_dbg(mm_gaze->dev,
		"set buf MTK_GAZE_CAMERA_BUF_TYPE_WARPA_MAP_0 @0x%llx\n",
		mm_gaze->
		buf[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_MAP_0]->dma_addr);
	ret = mtk_warpa_set_buf_map_no0(mm_gaze->dev_warpa, NULL,
		mm_gaze->
		buf[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_MAP_0]->dma_addr,
		mm_gaze->
		buf[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_MAP_0]->pitch);
	if (ret) {
		dev_err(mm_gaze->dev,
		"MTK_GAZE_CAMERA_BUF_TYPE_WARPA_MAP_0 fail!! %d\n", ret);
		return ret;
	}

	dev_dbg(mm_gaze->dev,
	"set buf MTK_GAZE_CAMERA_BUF_TYPE_WARPA_MAP_1 @0x%llx\n",
		mm_gaze->
		buf[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_MAP_1]->dma_addr);
	ret = mtk_warpa_set_buf_map_no1(mm_gaze->dev_warpa, NULL,
		mm_gaze->
		buf[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_MAP_1]->dma_addr,
		mm_gaze->
		buf[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_MAP_1]->pitch);
	if (ret) {
		dev_err(mm_gaze->dev,
		"MTK_GAZE_CAMERA_BUF_TYPE_WARPA_MAP_1 fail!! %d\n", ret);
		return ret;
	}

	dev_dbg(mm_gaze->dev,
		"set buf MTK_GAZE_CAMERA_BUF_TYPE_WARPA_DBG_OUT_IMG @0x%llx, pitch:%d\n",
		mm_gaze->
		buf[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_DBG_OUT_IMG]->dma_addr,
		mm_gaze->buf
		[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_DBG_OUT_IMG]->pitch);
	ret = mtk_warpa_set_buf_out_img(mm_gaze->dev_warpa, NULL,
		mm_gaze->
		buf[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_DBG_OUT_IMG]->dma_addr,
		mm_gaze->
		buf[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_DBG_OUT_IMG]->pitch);
	if (ret) {
		dev_err(mm_gaze->dev,
			"MTK_GAZE_CAMERA_BUF_TYPE_WARPA_DBG_OUT_IMG fail!! %d\n",
			ret);
		return ret;
	}

	dev_dbg(mm_gaze->dev,
		"set buf MTK_GAZE_CAMERA_BUF_TYPE_WARPA_DBG_OUT_VLD @0x%llx\n",
		mm_gaze->
		buf[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_DBG_OUT_VLD]->dma_addr);
	dev_dbg(mm_gaze->dev,
		"set buf MTK_GAZE_CAMERA_BUF_TYPE_WARPA_DBG_OUT_VLD pitch @%d\n",
		mm_gaze->
		buf[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_DBG_OUT_VLD]->pitch);
	ret = mtk_warpa_set_buf_out_vld(mm_gaze->dev_warpa, NULL,
		mm_gaze->
		buf[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_DBG_OUT_VLD]->dma_addr,
		mm_gaze->
		buf[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_DBG_OUT_VLD]->pitch);
	if (ret) {
		dev_err(mm_gaze->dev,
			"MTK_GAZE_CAMERA_BUF_TYPE_WARPA_DBG_OUT_VLD fail!! %d\n",
			ret);
		return ret;
	}

	dev_dbg(mm_gaze->dev,
		"set buf MTK_GAZE_CAMERA_BUF_TYPE_WDMA @0x%llx, pitch:%d\n",
		mm_gaze->buf[MTK_GAZE_CAMERA_BUF_TYPE_WDMA]->dma_addr,
		mm_gaze->buf[MTK_GAZE_CAMERA_BUF_TYPE_WDMA]->pitch);
	mtk_wdma_set_out_buf(mm_gaze->dev_gaze_wdma, NULL,
		mm_gaze->buf[MTK_GAZE_CAMERA_BUF_TYPE_WDMA]->dma_addr,
		mm_gaze->buf[MTK_GAZE_CAMERA_BUF_TYPE_WDMA]->pitch,
		mm_gaze->buf[MTK_GAZE_CAMERA_BUF_TYPE_WDMA]->format);

	return ret;
}

static int mtk_mm_gaze_reset
		(struct mtk_mm_gaze *mm_gaze)
{
	int ret = 0;

	if (!mm_gaze || !mm_gaze->dev)
		return -EINVAL;

	dev_dbg(mm_gaze->dev, "mm gaze path reset!\n");

	/* sw reset */
#if 0
	ret = mtk_warpa_reset(mm_gaze->dev_warpa);
	if (ret) {
		dev_err(mm_gaze->dev,
			"mtk_warpa_reset failed !! %d\n", ret);
		return ret;
	}

	ret = mtk_wdma_reset(mm_gaze->dev_gaze_wdma);
	if (ret) {
		dev_err(mm_gaze->dev,
			"mtk_wdma_reset 0 failed !! %d\n", ret);
		return ret;
	}
#endif

	/* hw reset */
	ret = mtk_mmsys_gaze_top_mod_reset(mm_gaze->dev_mmsys_gaze_top,
			   MDP_WPEA_0);
	if (ret) {
		dev_err(mm_gaze->dev, "MDP_WPEA_0 gaze top reset fail! %d\n",
				ret);
		return ret;
	}

	ret = mtk_mmsys_gaze_top_mod_reset(mm_gaze->dev_mmsys_gaze_top,
			   WDMA_GAZE);
	if (ret) {
		dev_err(mm_gaze->dev, "WDMA_GAZE gaze top reset fail! %d\n",
				ret);
		return ret;
	}

	if (mm_gaze->mmgazemask & DEV_GAZE_CAMERA_WARPA_KICK_P2_DONE_GCE)
		mtk_mm_gaze_cmdq_deinit(mm_gaze);

	return ret;
}

static int mtk_mm_gaze_ioctl_flush_cache(struct mtk_mm_gaze *mm_gaze)
{
	struct mtk_common_buf *buf;
	struct device *dev;
	int i;
	int ret = 0;

	dev_dbg(mm_gaze->dev, "gaze ioctl flush cache\n");
	for (i = 0; i < MTK_GAZE_CAMERA_BUF_TYPE_MAX; i++) {
		buf = mm_gaze->buf[i];
		switch (i) {
		case MTK_GAZE_CAMERA_BUF_TYPE_WARPA_IMG_BUF_0:
		case MTK_GAZE_CAMERA_BUF_TYPE_WARPA_IMG_BUF_1:
		case MTK_GAZE_CAMERA_BUF_TYPE_WARPA_MAP_0:
		case MTK_GAZE_CAMERA_BUF_TYPE_WARPA_MAP_1:
		case MTK_GAZE_CAMERA_BUF_TYPE_WARPA_DBG_OUT_IMG:
		case MTK_GAZE_CAMERA_BUF_TYPE_WARPA_DBG_OUT_VLD:
			dev = mm_gaze->dev_warpa;
			break;
		case MTK_GAZE_CAMERA_BUF_TYPE_WDMA:
		case MTK_GAZE_CAMERA_BUF_TYPE_WDMA_DUMMY:
			dev = mm_gaze->dev_gaze_wdma;
			break;
		}
		if (mm_gaze->buf[i] != NULL)
			dma_sync_sg_for_device(dev, buf->sg->sgl,
				buf->sg->nents, DMA_TO_DEVICE);
	}

	return ret;
}

static int mtk_mm_gaze_ioctl_one_shot_dump(struct mtk_mm_gaze *mm_gaze)
{
	int i = 0;

	mm_gaze->one_shot_dump = 1;

	/* wait up to timeout * img_num * 10 sec  */
	for (i = 0; i < FLOW_TIMEOUT * mm_gaze->img_num * 10; i++) {
		if (!mm_gaze->one_shot_dump)
			break;
		usleep_range(500, 1000);
	}

	mm_gaze->one_shot_dump = 0;
	if (i == FLOW_TIMEOUT * mm_gaze->img_num)
		return -ETIME;
	else
		return 0;
}

static int mtk_mm_gaze_ioctl_import_fd_to_handle(struct mtk_mm_gaze *mm_gaze,
	unsigned long arg)
{
	struct mtk_common_buf_handle handle;
	struct device *dev;
	int ret = 0;

	ret = copy_from_user((void *)&handle, (void *)arg,
			sizeof(handle));
	if (ret) {
		dev_err(mm_gaze->dev, "FD_TO_HANDLE: get params fail, ret=%d\n",
			ret);
		return ret;
	}

	switch (handle.buf_type) {
	case MTK_GAZE_CAMERA_BUF_TYPE_WARPA_IMG_BUF_0:
	case MTK_GAZE_CAMERA_BUF_TYPE_WARPA_IMG_BUF_1:
	case MTK_GAZE_CAMERA_BUF_TYPE_WARPA_MAP_0:
	case MTK_GAZE_CAMERA_BUF_TYPE_WARPA_MAP_1:
	case MTK_GAZE_CAMERA_BUF_TYPE_WARPA_DBG_OUT_IMG:
	case MTK_GAZE_CAMERA_BUF_TYPE_WARPA_DBG_OUT_VLD:
		dev = mm_gaze->dev_warpa;
		break;
	case MTK_GAZE_CAMERA_BUF_TYPE_WDMA:
	case MTK_GAZE_CAMERA_BUF_TYPE_WDMA_DUMMY:
		dev = mm_gaze->dev_gaze_wdma;
		break;
	default:
		dev_err(mm_gaze->dev, "FD_TO_HANDLE: buf type %d error\n",
				handle.buf_type);
		return -EINVAL;
	}
	ret = mtk_common_fd_to_handle_offset(dev,
		handle.fd, &(handle.handle), handle.offset);
	if (ret) {
		dev_err(mm_gaze->dev, "FD_TO_HANDLE: import buf fail, ret=%d\n",
			ret);
		return ret;
	}
	ret = copy_to_user((void *)arg, &handle, sizeof(handle));
	if (ret) {
		dev_err(mm_gaze->dev, "FD_TO_HANDLE: update params fail, ret=%d\n",
			ret);
		return ret;
	}

	return ret;
}

static int mtk_mm_gaze_ioctl_set_buf(struct mtk_mm_gaze *mm_gaze,
	unsigned long arg)
{
	struct mtk_common_set_buf buf;
	struct mtk_common_buf *buf_handle;
	int ret = 0;

	ret = copy_from_user((void *)&buf, (void *)arg, sizeof(buf));
	if (ret) {
		dev_err(mm_gaze->dev, "SET_BUF: get params failed, ret=%d\n",
			ret);
		return ret;
	}
	if (buf.buf_type >= MTK_GAZE_CAMERA_BUF_TYPE_MAX) {
		dev_err(mm_gaze->dev, "MTK_MM_GAZE:SET_BUF: buf type %d error\n",
			buf.buf_type);
		return -EINVAL;
	}

	buf_handle = (struct mtk_common_buf *)buf.handle;
	buf_handle->pitch = buf.pitch;
	buf_handle->format = buf.format;
	dev_dbg(mm_gaze->dev, "set MM_GAZE buf %d\n", buf.buf_type);
	dev_dbg(mm_gaze->dev, "buf_handle->kvaddr = %p\n",
		(void *)buf_handle->kvaddr);
	dev_dbg(mm_gaze->dev, "buf_handle->dma_addr = %p\n",
		(void *)buf_handle->dma_addr);
	dev_dbg(mm_gaze->dev, "buf_handle->pitch:%d\n", buf_handle->pitch);
	dev_dbg(mm_gaze->dev, "buf_handle->format:%d\n", buf_handle->format);
	mm_gaze->buf[buf.buf_type] = buf_handle;

	return ret;
}

static int mtk_mm_gaze_ioctl_set_prop(struct mtk_mm_gaze *mm_gaze,
	unsigned long arg)
{
	struct mtk_gaze_camera_set_prop set_prop;
	int ret = 0;

	ret = copy_from_user((void *)&set_prop, (void *)arg, sizeof(set_prop));
	if (ret) {
		dev_err(mm_gaze->dev, "SET_PROP: get params failed, ret=%d\n",
			ret);
		return ret;
	}
	mm_gaze->mmgazemask = set_prop.mmgazemask;
	set_prop.dump_path[sizeof(set_prop.dump_path) - 1] = '\0';
	strncpy(mm_gaze->dump_path, set_prop.dump_path,
		sizeof(mm_gaze->dump_path));

	return ret;
}

static int mtk_mm_gaze_ioctl_config_warpa(struct mtk_mm_gaze *mm_gaze,
	unsigned long arg)
{
	struct mtk_gaze_camera_config_warpa config_warpa;
	int ret = 0;

	ret = copy_from_user(&config_warpa, (void *)arg, sizeof(config_warpa));
	if (ret) {
		dev_err(mm_gaze->dev, "CONFIG_WARPA: get params failed, ret=%d\n",
			ret);
		return ret;
	}

	mm_gaze->coef_tbl.idx = config_warpa.coef_tbl_idx;
	memcpy(mm_gaze->coef_tbl.user_coef_tbl, config_warpa.user_coef_tbl,
		sizeof(mm_gaze->coef_tbl.user_coef_tbl));
	mm_gaze->border_color.enable = config_warpa.border_color_enable;
	mm_gaze->border_color.border_color = config_warpa.border_color;
	mm_gaze->border_color.unmapped_color = config_warpa.unmapped_color;
	mm_gaze->proc_mode = config_warpa.proc_mode;
	mm_gaze->warpa_out_mode = config_warpa.out_mode;
	mm_gaze->reset = config_warpa.reset;
	mm_gaze->warpa_in_w = config_warpa.warpa_in_w;
	mm_gaze->warpa_in_h = config_warpa.warpa_in_h;
	mm_gaze->warpa_out_w = config_warpa.warpa_out_w;
	mm_gaze->warpa_out_h = config_warpa.warpa_out_h;
	mm_gaze->warpa_map_w = config_warpa.warpa_map_w;
	mm_gaze->warpa_map_h = config_warpa.warpa_map_h;
	mm_gaze->warpa_align = config_warpa.warpa_align;
	mm_gaze->warp_input_buf_pitch = mm_gaze->
		buf[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_IMG_BUF_0]->pitch;
	mm_gaze->warp_input_buf_size =
		mm_gaze->warp_input_buf_pitch * mm_gaze->warpa_in_h;
	mm_gaze->warp_map_pitch = mm_gaze->
		buf[MTK_GAZE_CAMERA_BUF_TYPE_WARPA_MAP_0]->pitch;
	mm_gaze->warp_map_size =
		mm_gaze->warp_map_pitch * mm_gaze->warpa_map_h;

	return ret;
}

static int mtk_mm_gaze_ioctl_config_wdma(struct mtk_mm_gaze *mm_gaze,
	unsigned long arg)
{
	struct mtk_gaze_camera_config_wdma config_wdma;
	int ret = 0;

	ret = copy_from_user(&config_wdma, (void *)arg, sizeof(config_wdma));
	if (ret) {
		dev_err(mm_gaze->dev, "CONFIG_WDMA: get params failed, ret=%d\n",
			ret);
		return ret;
	}
	mm_gaze->wdma_align = config_wdma.wdma_align;
	mm_gaze->wdma_buf_pitch =
		mm_gaze->buf[MTK_GAZE_CAMERA_BUF_TYPE_WDMA]->pitch;
	mm_gaze->wdma_buf_size =
		mm_gaze->wdma_buf_pitch * mm_gaze->warpa_out_h;

	return ret;
}

static int mtk_mm_gaze_ioctl_streamon(struct mtk_mm_gaze *mm_gaze)
{
	int ret = 0;

#ifdef CONFIG_COMMON_CLK_MT3612
	ret = mtk_mm_gaze_module_power_on(mm_gaze);
	if (ret) {
		dev_err(mm_gaze->dev, "POWER ON: failed!! %d\n", ret);
		return ret;
	}

	if (mm_gaze->reset) {
		ret = mtk_mm_gaze_reset(mm_gaze);
		if (ret) {
			dev_err(mm_gaze->dev, "RESET: fail, ret=%d\n", ret);
			return ret;
		}
	}
#endif
	ret = mtk_mm_gaze_module_config(mm_gaze);
	if (ret) {
		dev_err(mm_gaze->dev, "CONFIG: failed!! %d\n", ret);
		return ret;
	}

	ret = mtk_mm_gaze_path_connect(mm_gaze);
	if (ret) {
		dev_err(mm_gaze->dev, "CONNECT: failed!! %d\n", ret);
		return ret;
	}

	ret = mtk_mm_gaze_module_start(mm_gaze);
	if (ret) {
		dev_err(mm_gaze->dev, "START: failed!! %d\n", ret);
		return ret;
	}

	ret = mtk_mm_gaze_module_set_buf(mm_gaze);
	if (ret) {
		dev_err(mm_gaze->dev, "SET BUF: failed!! %d\n", ret);
		return ret;
	}

	return ret;
}

static int mtk_mm_gaze_ioctl_streamoff(struct mtk_mm_gaze *mm_gaze)
{
	int ret = 0;

	ret = mtk_mm_gaze_path_gce_trigger(mm_gaze, false);
	if (ret) {
		dev_err(mm_gaze->dev, "TRIGGER DISABLE: failed!! %d\n", ret);
		return ret;
	}

	ret = mtk_mm_gaze_path_stop(mm_gaze);
	if (ret) {
		dev_err(mm_gaze->dev, "STOP: failed!! %d\n", ret);
		return ret;
	}

	ret = mtk_mm_gaze_path_disconnect(mm_gaze);
	if (ret) {
		dev_err(mm_gaze->dev, "DISCONNECT: failed!! %d\n", ret);
		return ret;
	}

#ifdef CONFIG_COMMON_CLK_MT3612
	ret = mtk_mm_gaze_module_power_off(mm_gaze);
	if (ret) {
		dev_err(mm_gaze->dev, "POWER OFF: failed!! %d\n", ret);
		return ret;
	}
#endif

	return ret;
}

static int mtk_mm_gaze_ioctl_trigger(struct mtk_mm_gaze *mm_gaze)
{
	int ret = 0;

	if (mm_gaze->mmgazemask & DEV_GAZE_CAMERA_WARPA_KICK_P2_DONE_GCE)
		ret = mtk_mm_gaze_path_gce_trigger(mm_gaze, true);
	else
		ret = mtk_mm_gaze_path_trigger(mm_gaze, true);
	if (ret) {
		dev_err(mm_gaze->dev, "TRIGGER ENABLE: failed!! %d\n", ret);
		return ret;
	}

	return ret;
}

static int mtk_mm_gaze_ioctl_reset(struct mtk_mm_gaze *mm_gaze)
{
	int ret = 0;

	ret = mtk_mm_gaze_reset(mm_gaze);
	if (ret) {
		dev_err(mm_gaze->dev, "RESET: failed!! %d\n", ret);
		return ret;
	}

	return ret;
}

static int mtk_mm_gaze_ioctl_put_handle(struct mtk_mm_gaze *mm_gaze,
	unsigned long arg)
{
	struct mtk_common_put_handle put_handle;
	int ret = 0;

	ret = copy_from_user(&put_handle, (void *)arg, sizeof(put_handle));
	if (ret) {
		dev_err(mm_gaze->dev,
			"PUT_HANDLE: get params failed, ret=%d\n",
			ret);
		return ret;
	}
	ret = mtk_common_put_handle(put_handle.handle);
	if (ret) {
		dev_err(mm_gaze->dev,
			"PUT_HANDLE: put handle fail, ret=%d\n",
			ret);
		return ret;
	}

	return ret;
}

static int mtk_mm_gaze_ioctl_set_vpu_req(struct mtk_mm_gaze *mm_gaze,
	unsigned long arg)
{
	struct mtk_mm_gaze_vpu_req req;
	int ret = 0;

	ret = copy_from_user(&req, (void *)arg, sizeof(req));
	if (ret) {
		dev_err(mm_gaze->dev, "VPU_REQ: get params failed, ret=%d\n",
			ret);
		return ret;
	}
	if (req.process_GAZE_WDMA != NULL)
		mm_gaze->process_GAZE_WDMA = *req.process_GAZE_WDMA;

	return ret;
}

static int mtk_mm_gaze_check_flow_done(struct mtk_mm_gaze *mm_gaze)
{
	int i;

	/* wait up to timeout * img_num sec */
	for (i = 0; i < FLOW_TIMEOUT * mm_gaze->img_num; i++) {
		if (mm_gaze->flow_done)
			break;
		usleep_range(5000, 10000);
	}
	mm_gaze->flow_done = 0;

	if (i == FLOW_TIMEOUT * mm_gaze->img_num)
		return -ETIME;
	else
		return 0;
}

static int mtk_mm_gaze_ioctl_set_log_level(struct mtk_mm_gaze *mm_gaze,
	unsigned long arg)
{
	int ret = 0;

	if (arg > CV_LOG_DBG)
		return -EINVAL;

	gaze_kern_log_level = (int)arg;
	pr_info("set gaze kernel log level %d\n", gaze_kern_log_level);

	return ret;
}

static int mtk_mm_gaze_ioctl_get_statistics(struct mtk_mm_gaze *mm_gaze,
	unsigned long arg)
{
	struct mtk_gaze_statistics stat;
	int ret = 0;

	ret = copy_from_user((void *)&stat, (void *)arg, sizeof(stat));
	if (ret) {
		dev_err(mm_gaze->dev, "GET_STAT: get params failed, ret=%d\n",
			ret);
		return ret;
	}
	stat.gaze_frame_cnt = frame_idx;
	stat.gaze_p2_sof_cnt = p2_sof_idx;
	ret = copy_to_user((void *)arg, &stat, sizeof(stat));
	if (ret) {
		dev_err(mm_gaze->dev, "GET_STAT: update params failed, ret=%d\n",
			ret);
		return ret;
	}

	return ret;
}

static int mtk_mm_gaze_ioctl_dbg_cksm(struct mtk_mm_gaze *mm_gaze,
	unsigned long arg)
{
	struct mtk_gaze_camera_dbg_test dbg_test;
	int ret = 0;
	int i;

	ret = copy_from_user((void *)&dbg_test, (void *)arg, sizeof(dbg_test));
	if (ret) {
		dev_err(mm_gaze->dev, "GET_TEST: get params failed, ret=%d\n",
			ret);
		return ret;
	}

	for (i = 0; i < FLOW_TIMEOUT * mm_gaze->img_num; i++) {
		if (frame_idx > 0)
			break;
		usleep_range(5000, 10000);
	}
	if (i == FLOW_TIMEOUT * mm_gaze->img_num) {
		dev_err(mm_gaze->dev, "wait gaze tracking frame done timeout\n");
		return -ETIME;
	}

	GZ_LOG(CV_LOG_DBG, "gaze wdma dbg cksm = 0x%08x\n", mm_gaze->wdma_dbg);

	/* for dbg */
	if (dbg_test.gaze_wdma_dbg != mm_gaze->wdma_dbg) {
		pr_err("gaze wdma dbg compare failed! %08x:%08x\n",
			dbg_test.gaze_wdma_dbg, mm_gaze->wdma_dbg);
		return -EINVAL;
	}

	return ret;
}

static int mtk_mm_gaze_ioctl_enable_camera_sync(struct mtk_mm_gaze *mm_gaze,
	unsigned long arg)
{
	struct mtk_gaze_camera_sync camera_sync;
	int ret = 0;

	ret = copy_from_user((void *)&camera_sync, (void *)arg,
			sizeof(camera_sync));
	if (ret) {
		dev_err(mm_gaze->dev,
			"ENABLE_CAMERA_SYNC: get params fail, ret=%d\n",
			ret);
		return ret;
	}
	GZ_LOG(CV_LOG_DBG, "enable gaze camera sync!\n");

	if (camera_sync.sensor_fps > 120 || camera_sync.sensor_fps < 15)
		return -EINVAL;

	mm_gaze->sensor_fps = camera_sync.sensor_fps;
	ret = mtk_mm_gaze_config_camera_sync(mm_gaze, camera_sync.power_flag);
	if (ret) {
		dev_err(mm_gaze->dev, "config camera sync fail!\n");
		return ret;
	}

	return ret;
}

static int mtk_mm_gaze_ioctl_disable_camera_sync(struct mtk_mm_gaze *mm_gaze)
{
	int ret = 0;

	GZ_LOG(CV_LOG_DBG, "disable gaze camera sync!\n");
	ret = mtk_cv_disable(mm_gaze->dev_mmsys_core_top,
			MMSYSCFG_CAMERA_SYNC_GAZE0, NULL);
	if (ret) {
		dev_err(mm_gaze->dev, "disable camera sync id: %d fail!!\n",
			MMSYSCFG_CAMERA_SYNC_GAZE0);
		return ret;
	}

	ret = mtk_cv_disable(mm_gaze->dev_mmsys_core_top,
		MMSYSCFG_CAMERA_SYNC_GAZE1, NULL);
	if (ret) {
		dev_err(mm_gaze->dev, "disable camera sync id: %d fail!!\n",
			MMSYSCFG_CAMERA_SYNC_GAZE1);
		return ret;
	}

	return ret;
}

static long mtk_mm_gaze_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	struct mtk_mm_gaze *mm_gaze;
	struct device *dev;
	int ret = 0;

	if (!filp) {
		pr_err("mtk_mm_gaze_ioctl flip is NULL!\n");
		return -EFAULT;
	}

	mm_gaze = filp->private_data;
	dev = mm_gaze->dev;

	switch (cmd) {
	case MTK_GAZE_CAMERA_IOCTL_IMPORT_FD_TO_HANDLE:
		ret = mtk_mm_gaze_ioctl_import_fd_to_handle(mm_gaze, arg);
		break;
	case MTK_GAZE_CAMERA_IOCTL_SET_BUF:
		ret = mtk_mm_gaze_ioctl_set_buf(mm_gaze, arg);
		break;
	case MTK_GAZE_CAMERA_IOCTL_SET_GAZE_PROP:
		ret = mtk_mm_gaze_ioctl_set_prop(mm_gaze, arg);
		break;
	case MTK_GAZE_CAMERA_IOCTL_CONFIG_WARPA:
		ret = mtk_mm_gaze_ioctl_config_warpa(mm_gaze, arg);
		break;
	case MTK_GAZE_CAMERA_IOCTL_CONFIG_WDMA:
		ret = mtk_mm_gaze_ioctl_config_wdma(mm_gaze, arg);
		break;
	case MTK_GAZE_CAMERA_IOCTL_STREAMON:
		ret = mtk_mm_gaze_ioctl_streamon(mm_gaze);
		break;
	case MTK_GAZE_CAMERA_IOCTL_STREAMOFF:
		ret = mtk_mm_gaze_ioctl_streamoff(mm_gaze);
		break;
	case MTK_GAZE_CAMERA_IOCTL_TRIGGER:
		ret = mtk_mm_gaze_ioctl_trigger(mm_gaze);
		break;
	case MTK_GAZE_CAMERA_IOCTL_RESET:
		ret = mtk_mm_gaze_ioctl_reset(mm_gaze);
		break;
	case MTK_GAZE_CAMERA_IOCTL_PUT_HANDLE:
		ret = mtk_mm_gaze_ioctl_put_handle(mm_gaze, arg);
		break;
	case MTK_GAZE_GAMERA_IOCTL_SET_VPU_REQ:
		ret = mtk_mm_gaze_ioctl_set_vpu_req(mm_gaze, arg);
		break;
	case MTK_GAZE_GAMERA_IOCTL_CHECK_FLOW_DONE:
		ret = mtk_mm_gaze_check_flow_done(mm_gaze);
		break;
	case MTK_GAZE_GAMERA_IOCTL_SET_LOG_LEVEL:
		ret = mtk_mm_gaze_ioctl_set_log_level(mm_gaze, arg);
		break;
	case MTK_GAZE_GAMERA_IOCTL_GET_STATISTICS:
		ret = mtk_mm_gaze_ioctl_get_statistics(mm_gaze, arg);
		break;
	case MTK_GAZE_GAMERA_IOCTL_DBG_TEST:
		ret = mtk_mm_gaze_ioctl_dbg_cksm(mm_gaze, arg);
		break;
	case MTK_GAZE_GAMERA_ENABLE_CAMERA_SYNC:
		ret = mtk_mm_gaze_ioctl_enable_camera_sync(mm_gaze, arg);
		break;
	case MTK_GAZE_GAMERA_DISABLE_CAMERA_SYNC:
		mtk_mm_gaze_ioctl_disable_camera_sync(mm_gaze);
		break;
	case MTK_GAZE_GAMERA_IOCTL_FLUSH_BUF_CACHE:
		ret = mtk_mm_gaze_ioctl_flush_cache(mm_gaze);
		break;
	case MTK_GAZE_GAMERA_IOCTL_ONE_SHOT_DUMP:
		ret = mtk_mm_gaze_ioctl_one_shot_dump(mm_gaze);
		break;
	default:
		dev_err(dev, "mtk_gaze_ioctl: no such command!\n");
		ret = -ENOTTY;
		break;
	}
	return ret;
}

static int mm_gaze_open(struct inode *inode, struct file *filp)
{
	/*struct ivp_user *user;*/
	struct mtk_mm_gaze *mm_gaze =
			container_of(inode->i_cdev,
			struct mtk_mm_gaze, chardev);
	filp->private_data = mm_gaze;
	return 0;
}

static int mm_gaze_release(struct inode *inode,
	struct file *filp)
{
	/* we do nothing here */
	return 0;
}

static ssize_t mtk_gaze_dev_write
		(struct file *filp, const char *buffer,
		size_t len, loff_t *offset)
{
	return len;
}

static const struct file_operations mm_gaze_fops = {
	.owner = THIS_MODULE,
	.open = mm_gaze_open,
	.write = mtk_gaze_dev_write,
	.release = mm_gaze_release,
	.unlocked_ioctl = mtk_mm_gaze_ioctl,
};

static int mtk_mm_gaze_reg_chardev
		(struct mtk_mm_gaze *mm_gaze)
{
	struct device *dev;
	char dev_name[] = "mtk_mm_gaze";
	int ret = 0;

	if (!mm_gaze || !mm_gaze->dev)
		return -EINVAL;

	pr_debug("mtk_mm_gaze_reg_chardev\n");

	ret = alloc_chrdev_region(&mm_gaze->devt, 0, 1,
				dev_name);
	if (ret < 0) {
		pr_err(" alloc_chrdev_region failed, %d\n",
				ret);
		return ret;
	}

	pr_debug("MAJOR/MINOR = 0x%08x\n", mm_gaze->devt);

	/* Attatch file operation. */
	cdev_init(&mm_gaze->chardev, &mm_gaze_fops);
	mm_gaze->chardev.owner = THIS_MODULE;

	/* Add to system */
	ret = cdev_add(&mm_gaze->chardev, mm_gaze->devt, 1);
	if (ret < 0) {
		pr_err("attatch file operation failed, %d\n",
			ret);
		unregister_chrdev_region(mm_gaze->devt, 1);
		return -EIO;
	}

	/* Create class register */
	mm_gaze->dev_class = class_create(THIS_MODULE, dev_name);
	if (IS_ERR(mm_gaze->dev_class)) {
		ret = PTR_ERR(mm_gaze->dev_class);
		pr_err("Unable to create class, err = %d\n", ret);
		return -EIO;
	}

	dev = device_create(mm_gaze->dev_class, NULL,
			mm_gaze->devt, NULL, dev_name);
	if (IS_ERR(dev)) {
		ret = PTR_ERR(dev);
		pr_err("Failed to create device: /dev/%s, err = %d",
				dev_name, ret);
		return -EIO;
	}

	pr_debug("mtk_mm_gaze_reg_chardev done\n");

	return 0;
}
static void mtk_mm_gaze_unreg_chardev(struct mtk_mm_gaze *mm_gaze)
{
	device_destroy(mm_gaze->dev_class, mm_gaze->devt);
	class_destroy(mm_gaze->dev_class);
	cdev_del(&mm_gaze->chardev);
	unregister_chrdev_region(mm_gaze->devt, 1);
}
static int mtk_mm_gaze_get_device(struct device *dev,
		char *compatible, int idx, struct device **child_dev)
{
	struct platform_device *pdev;
	struct device_node *node;

	node = of_parse_phandle(dev->of_node, compatible, idx);
	if (!node) {
		dev_err(dev, "mtk_get_device: could not find %s, %d\n",
				compatible, idx);
		return -ENODEV;
	}

	pdev = of_find_device_by_node(node);
	if (!pdev || !pdev->dev.driver) {
		dev_warn(dev, "mtk_get_device: waiting for device %s\n",
			node->full_name);

		return -EPROBE_DEFER;
	}

	*child_dev = &pdev->dev;

	return 0;
}

static int mtk_mm_gaze_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_mm_gaze *mm_gaze;
	int ret;

	dev_dbg(dev, "start mm_gaze probe!!\n");

	mm_gaze = devm_kzalloc(dev,
					sizeof(*mm_gaze), GFP_KERNEL);
	if (!mm_gaze)
		return -ENOMEM;

	mm_gaze->dev = dev;
	dev_dbg(mm_gaze->dev, "mm_gaze addr %p\n",
		    mm_gaze);

	ret = mtk_mm_gaze_get_device(dev, "mediatek,mmsys_gaze_top", 0,
			&mm_gaze->dev_mmsys_gaze_top);
	if (ret) {
		dev_dbg(dev, "get dev_mmsys_gaze_top failed!! %d\n", ret);
		return ret;
	}

	ret = mtk_mm_gaze_get_device(dev, "mediatek,mmsys_core_top",
			0, &mm_gaze->dev_mmsys_core_top);
	if (ret) {
		dev_dbg(dev, "get dev_mmsys_core_top failed!! %d\n", ret);
		return ret;
	}

	ret = mtk_mm_gaze_get_device(dev, "mediatek,mutex", 0,
			&mm_gaze->dev_mutex);
	if (ret) {
		dev_dbg(dev, "get gaze dev_mutex failed!! %d\n", ret);
		return ret;
	}

	ret = mtk_mm_gaze_get_device(dev, "mediatek,rbfc", 0,
			&mm_gaze->dev_rbfc);
	if (ret) {
		dev_dbg(dev, "get gaze dev_rbfc failed!! %d\n", ret);
		return ret;
	}

	ret = mtk_mm_gaze_get_device(dev, "mediatek,warpa", 0,
			&mm_gaze->dev_warpa);
	if (ret) {
		dev_dbg(dev, "get gaze dev_warpa failed!! %d\n", ret);
		return ret;
	}

	ret = mtk_mm_gaze_get_device(dev, "mediatek,wdma", 0,
			&mm_gaze->dev_gaze_wdma);
	if (ret) {
		dev_dbg(dev, "get gaze dev_wdma failed!! %d\n", ret);
		return ret;
	}

	ret = mtk_mm_gaze_get_device(dev, "mediatek,sysram", 0,
			&mm_gaze->dev_sysram);
	if (ret) {
		dev_dbg(dev, "get gaze dev_sysram failed!! %d\n", ret);
		return ret;
	}

	ret = mtk_mm_gaze_reg_chardev(mm_gaze);
	if (ret)
		return ret;

	gaze_kern_log_level = CV_LOG_INFO;
	mm_gaze->polling_wait = 0;
	INIT_WORK(&mm_gaze->trigger_work, mtk_mm_gaze_trigger_work);
	INIT_WORK(&mm_gaze->trigger_work_gce, mtk_mm_gaze_trigger_work_gce);
	INIT_WORK(&mm_gaze->trigger_gaze0_p2_sof, mtk_gaze0_p2_sof_check);
#if RBFC_INCOMPLETE_DEBUG
	INIT_WORK(&mm_gaze->trigger_rbfc_ren,
			mtk_gaze_wpe_ren_rbfc_incomplete_detect);
	INIT_WORK(&mm_gaze->trigger_gaze0_wen,
			mtk_gaze0_wen_rbfc_incomplete_detect);
	INIT_WORK(&mm_gaze->trigger_gaze1_wen,
			mtk_gaze1_wen_rbfc_incomplete_detect);
#endif

	platform_set_drvdata(pdev, mm_gaze);
	dev_dbg(dev, "gaze_camera_probe success!\n");
	return 0;
}

static int mtk_mm_gaze_remove(struct platform_device *pdev)
{
	struct mtk_mm_gaze *mm_gaze =
				platform_get_drvdata(pdev);

	mtk_mm_gaze_unreg_chardev(mm_gaze);

	return 0;
}

static const struct of_device_id mm_gaze_driver_dt_match[] = {
	{ .compatible = "mediatek,mt3612-mm-gaze-dev" },
	{},
};

MODULE_DEVICE_TABLE(of, mm_gaze_driver_dt_match);

struct platform_driver mtk_mm_gaze_driver = {
	.probe		= mtk_mm_gaze_probe,
	.remove		= mtk_mm_gaze_remove,
	.driver		= {
		.name	= "mt3612-mm-gaze-dev",
		.owner	= THIS_MODULE,
		.of_match_table = mm_gaze_driver_dt_match,
	},
};

module_platform_driver(mtk_mm_gaze_driver);
MODULE_LICENSE("GPL");

