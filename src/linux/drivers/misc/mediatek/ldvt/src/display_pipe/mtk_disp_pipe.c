/*
 * Copyright (c) 2017 MediaTek Inc.
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
#include <linux/clk.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/semaphore.h>
#include <linux/string.h>
#include <uapi/drm/drm_fourcc.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <video/mipi_display.h>

#include <soc/mediatek/mtk_dsi.h>
#include <soc/mediatek/mtk_dsc.h>
#include <soc/mediatek/mtk_mutex.h>
#include <soc/mediatek/mtk_rdma.h>
#include <soc/mediatek/mtk_mmsys_cfg.h>
#include <soc/mediatek/mtk_rbfc.h>
#include <soc/mediatek/mtk_ts.h>
#include <soc/mediatek/mtk_lhc.h>
#include <soc/mediatek/mtk_slicer.h>
#include <soc/mediatek/mtk_wdma.h>
#include <soc/mediatek/mtk_dprx_info.h>
#include <soc/mediatek/mtk_dprx_if.h>
#include <soc/mediatek/mtk_ddds.h>
#include <soc/mediatek/mtk_frmtrk.h>
#include <dt-bindings/gce/mt3612-gce.h>
#include "mtk_disp.h"
#include "mtk_disp_pipe.h"
#include "mtk_mmsys_test.h"
#include "mtk_lhc_test.h"
#include "mtk_mutex_test.h"

/* DDDS target freq */
#define TARGET_FREQ 26
#define MAX_FREQ 27

#define CHECK_PARA

static int is_test_started;
static struct task_struct *dsi_monitor_task;
static int curr_fps;

static struct mtk_dispsys_mod disp_mod_list[MTK_DISP_MOD_NR] = {
	{ .index = MTK_DISP_DSI, .name = "mediatek,dsi"},
	{ .index = MTK_DISP_DSC, .name = "mediatek,dsc"},
	{ .index = MTK_DISP_MUTEX, .name = "mediatek,mutex"},
	{ .index = MTK_DISP_WDMA0, .name = "mediatek,wdma0"},
	{ .index = MTK_DISP_WDMA1, .name = "mediatek,wdma1"},
	{ .index = MTK_DISP_WDMA2, .name = "mediatek,wdma2"},
	{ .index = MTK_DISP_WDMA3, .name = "mediatek,wdma3"},
	{ .index = MTK_DISP_MDP_RDMA, .name = "mediatek,mdp_rdma"},
	{ .index = MTK_DISP_PVRIC_RDMA, .name = "mediatek,pvric_rdma"},
	{ .index = MTK_DISP_DISP_RDMA, .name = "mediatek,disp_rdma"},
	{ .index = MTK_DISP_MMSYS_CFG, .name = "mediatek,mmsyscfg"},
	{ .index = MTK_DISP_RBFC, .name = "mediatek,rbfc"},
	{ .index = MTK_DISP_SLCR, .name = "mediatek,slcr"},
	{ .index = MTK_DISP_TIMESTAMP, .name = "mediatek,timestamp"},
	{ .index = MTK_DISP_LHC, .name = "mediatek,lhc"},
	{ .index = MTK_DISP_GCE0, .name = "mediatek,gce0"},
#ifdef CONFIG_MACH_MT3612_A0
	{ .index = MTK_DISP_DPRX, .name = "mediatek,dprx"},
	{ .index = MTK_DISP_DDDS, .name = "mediatek,ddds"},
	{ .index = MTK_DISP_FRMTRK, .name = "mediatek,frmtrk"},
#endif
};

#define REGFLAG_DELAY		0xfffc
#define REGFLAG_UDELAY		0xfffb
#define REGFLAG_END_OF_TABLE	0xfffd
#define REGFLAG_RESET_LOW	0xfffe
#define REGFLAG_RESET_HIGH	0xffff
bool nVidia_DSC, amd_DSC;
static const struct lcm_setting_table init_setting_ams336rp01_3_amd[] = {
	/* reset deasserted */
	{0xf0, 2,
	 {0x5a, 0x5a} },
	{0x01, 0, {} },
	{REGFLAG_DELAY, 7, {} },
	{0x9d, 1, {0x01} },
	/* dsc setting if dsc on*/
	/* 88 parameters for pps setting */
	{0x9e, 88,		/* 8 lanes/panel, 10bpc/10bpp, slice h=120 */
	 {0x11, 0x00, 0x00, 0xAB, 0x30, 0xA0, 0x07, 0xF8,
	  0x03, 0xE8, 0x00, 0x78, 0x03, 0xE8, 0x04, 0xE2,
	  0x01, 0x9A, 0x02, 0xF5, 0x00, 0x19, 0x0C, 0x48,
	  0x00, 0x13, 0x00, 0x0F, 0x01, 0x03, 0x00, 0x90,
	  0x16, 0x00, 0x10, 0xEC, 0x07, 0x10, 0x20, 0x00,
	  0x06, 0x0F, 0x0F, 0x33, 0x0E, 0x1C, 0x2A, 0x38,
	  0x46, 0x54, 0x62, 0x69, 0x70, 0x77, 0x79, 0x7B,
	  0x7D, 0x7E, 0x01, 0xC2, 0x22, 0x00, 0x2A, 0x40,
	  0x2A, 0xBE, 0x3A, 0xFC, 0x3A, 0xFA, 0x3A, 0xF8,
	  0x3B, 0x38, 0x3B, 0x78, 0x3B, 0x76, 0x4B, 0xB6,
	  0x4B, 0xB6, 0x4B, 0xF4, 0x63, 0xF4, 0x7C, 0x34} },
	{0xf0, 2,
	 {0xa5, 0xa5} },
	/* lane control */
	{0xf0, 2,
	 {0x5a, 0x5a} },
	{0xc4, 1, {0x02} },	/* 8 lanes/panel */
	{0xf0, 2,
	 {0xa5, 0xa5} },
	{REGFLAG_DELAY, 25, {} },
	{0x00, 0, {} },
	/* refresh rate setting */
	{0xf0, 2,
	 {0x5a, 0x5a} },
	{0xb0, 1, {0x50} },
	{0xc3, 1, {0x00} },	/* frame rate = 120Hz */
	{0xf7, 1, {0x02} },
	{0xf0, 2,
	 {0xa5, 0xa5} },
	/* sleep out */
	{0x11, 0, {} },
	/* hs settle time control */
	{REGFLAG_DELAY, 25, {} },
	{0xf0, 2,
	 {0x5a, 0x5a} },
	{0xe8, 4,
	 {0x64, 0x88, 0x00, 0x14} },	/* settle time > 1.45Gbps/lane */
	{0xf0, 2,
	 {0xa5, 0xa5} },
	{REGFLAG_DELAY, 225, {} },
	/* brightness control */
	{0x53, 1, {0x28} },
	{0x51, 2,
	 {0xff, 0x03} },
	/* sdc ip off */
	{0x80, 1, {0x00} },
	/* color coding select */
	{0xf0, 2,
	 {0x5a, 0x5a} },
	{0xb0, 1, {0x08} },
	{0xf2, 1, {0x00} },	/* dsc on */
	{0xf0, 2,
	 {0xa5, 0xa5} },
	/* send rgb data at least 1 frame */
	/* end with short packet - nop */
	{0x00, 0, {} }
};

static const struct lcm_setting_table init_setting_ams336rp01_3_nVidia[] = {
	/* reset deasserted */
	{0xf0, 2,
	 {0x5a, 0x5a} },
	{0x01, 0, {} },
	{REGFLAG_DELAY, 7, {} },
	{0x9d, 1, {0x01} },
	/* dsc setting if dsc on*/
	/* 88 parameters for pps setting */
	{0x9e, 88,		/* 8 lanes/panel, 10bpc/10bpp, slice h=2040 */
	 {0x11, 0x00, 0x00, 0xAB, 0x30, 0xA0, 0x07, 0xF8,
	  0x03, 0xE8, 0x07, 0xF8, 0x03, 0xE8, 0x04, 0xE2,
	  0x01, 0x9A, 0x02, 0xF5, 0x00, 0x19, 0xC6, 0x08,
	  0x00, 0x13, 0x00, 0x0F, 0x00, 0x10, 0x00, 0x09,
	  0x16, 0x00, 0x10, 0xEC, 0x07, 0x10, 0x20, 0x00,
	  0x06, 0x0F, 0x0F, 0x33, 0x0E, 0x1C, 0x2A, 0x38,
	  0x46, 0x54, 0x62, 0x69, 0x70, 0x77, 0x79, 0x7B,
	  0x7D, 0x7E, 0x01, 0xC2, 0x22, 0x00, 0x2A, 0x40,
	  0x2A, 0xBE, 0x3A, 0xFC, 0x3A, 0xFA, 0x3A, 0xF8,
	  0x3B, 0x38, 0x3B, 0x78, 0x3B, 0x76, 0x4B, 0xB6,
	  0x4B, 0xB6, 0x4B, 0xF4, 0x63, 0xF4, 0x7C, 0x34} },
	{0xf0, 2,
	 {0xa5, 0xa5} },
	/* lane control */
	{0xf0, 2,
	 {0x5a, 0x5a} },
	{0xc4, 1, {0x02} },	/* 8 lanes/panel */
	{0xf0, 2,
	 {0xa5, 0xa5} },
	{0x00, 0, {} },
	{REGFLAG_DELAY, 25, {} },
	/* refresh rate setting */
	{0xf0, 2,
	 {0x5a, 0x5a} },
	{0xb0, 1, {0x50} },
	{0xc3, 1, {0x00} },	/* frame rate = 120Hz */
	{0xf7, 1, {0x02} },
	{0xf0, 2,
	 {0xa5, 0xa5} },
	/* sleep out */
	{0x11, 0, {} },
	/* hs settle time control */
	{REGFLAG_DELAY, 25, {} },
	{0xf0, 2,
	 {0x5a, 0x5a} },
	{0xe8, 4,
	 {0x64, 0x88, 0x00, 0x14} },	/* settle time > 1.45Gbps/lane */
	{0xf0, 2,
	 {0xa5, 0xa5} },
	{REGFLAG_DELAY, 225, {} },
	/* brightness control */
	{0x53, 1, {0x28} },
	{0x51, 2,
	 {0xff, 0x03} },
	/* sdc ip off */
	{0x80, 1, {0x00} },
	/* color coding select */
	{0xf0, 2,
	 {0x5a, 0x5a} },
	{0xb0, 1, {0x08} },
	{0xf2, 1, {0x00} },	/* dsc on */
	{0xf0, 2,
	 {0xa5, 0xa5} },
	/* send rgb data at least 1 frame */
	/* end with short packet - nop */
	{0x00, 0, {} }
};

static const struct lcm_setting_table ams336rp01_3_rgb10_with_dsc[] = {
	/* refresh rate setting */
	{0xf0, 3,
	 {0xf0, 0x5a, 0x5a} },
	{0xb0, 1, {0x50} },
	{0xc3, 1, {0x00} },	/* frame rate = 120Hz */
	{0xf7, 1, {0x02} },
	{0xf0, 3,
	 {0xf0, 0xa5, 0xa5} },
	/* sleep out */
	{0x11, 0, {} },
	/* hs settle time control */
	{REGFLAG_DELAY, 25, {} },
	{0xf0, 3,
	 {0xf0, 0x5a, 0x5a} },
	{0xe8, 5,
	 {0xe8, 0x64, 0x88, 0x00, 0x14} },
	{0xf0, 3,
	 {0xf0, 0xa5, 0xa5} },
	{REGFLAG_DELAY, 225, {} },
	/* brightness control */
	{0x53, 1, {0x28} },
	{0x51, 3,
	 {0x51, 0xff, 0x03} },
	/* sdc ip off */
	{0x80, 1, {0x00} },
	/* color coding select */
	{0xf0, 3,
	 {0xf0, 0x5a, 0x5a} },
	{0xb0, 1, {0x08} },
	{0xf2, 1, {0x00} },	/* dsc on */
	{0xf0, 3,
	 {0xf0, 0xa5, 0xa5} },
	/* send rgb data at least 1 frame */
	/* end with short packet - nop */
	{0x00, 0, {} }
};

static int mtk_dispsys_format_to_bpp(u32 fmt)
{
	switch (fmt) {
	case DRM_FORMAT_ARGB2101010:
	case DRM_FORMAT_ABGR2101010:
	case DRM_FORMAT_RGBA1010102:
	case DRM_FORMAT_BGRA1010102:
	case DRM_FORMAT_AYUV2101010:
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_ARGB8888:
		return 32;
	case DRM_FORMAT_RGB888:
	case DRM_FORMAT_BGR888:
		return 24;
	case DRM_FORMAT_YUYV:
		return 16;
	}

	return -EINVAL;
}

static int mtk_dispsys_format_to_bpp_for_dsc(u32 fmt)
{
	switch (fmt) {
	case DRM_FORMAT_ARGB2101010:
	case DRM_FORMAT_ABGR2101010:
	case DRM_FORMAT_RGBA1010102:
	case DRM_FORMAT_BGRA1010102:
		return 30;
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_RGB888:
	case DRM_FORMAT_BGR888:
		return 24;
	}

	return -EINVAL;
}

static int mtk_dispsys_is_rgb10(u32 fmt)
{
	switch (fmt) {
	case DRM_FORMAT_ARGB2101010:
	case DRM_FORMAT_ABGR2101010:
	case DRM_FORMAT_RGBA1010102:
	case DRM_FORMAT_BGRA1010102:
	case DRM_FORMAT_AYUV2101010:
		return 1;
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_RGB888:
	case DRM_FORMAT_BGR888:
		return 0;
	}

	return -EINVAL;
}

#ifdef CONFIG_MACH_MT3612_A0
static int mtk_dispsys_ddds_config(struct mtk_dispsys *dispsys,
				   u32 target_freq, u32 max_freq, bool xtal)
{
	struct device *ddds_dev = NULL;

	if (!dispsys)
		return -EFAULT;

	ddds_dev = dispsys->disp_dev[MTK_DISP_DDDS];

	/*enable DDDS */
	if (!xtal) {
		mtk_ddds_enable(ddds_dev, true);
		mtk_ddds_set_target_freq(ddds_dev, target_freq);
		mtk_ddds_set_err_limit(ddds_dev, max_freq);
	}
	mtk_ddds_refclk_out(ddds_dev, xtal, true);

	dev_info(ddds_dev, "%s line: %d xtal: %d\n", __func__, __LINE__, xtal);

	return 0;
}

static int mtk_dispsys_ddds_enable(struct mtk_dispsys *dispsys,
				   u32 hsync_len, u32 step1, u32 step2)
{
	struct device *ddds_dev = NULL;
	int i = 0;
	bool lock_result;

	if (!dispsys)
		return -EFAULT;

	ddds_dev = dispsys->disp_dev[MTK_DISP_DDDS];

	dev_info(ddds_dev, "%s line: %d\n", __func__, __LINE__);

	mtk_ddds_set_hlen(ddds_dev, DDDS_FRAMETRACK_LOCK, hsync_len);
	mtk_ddds_set_hlen(ddds_dev, DDDS_FRAMETRACK_FAST1, (hsync_len + step1));
	mtk_ddds_set_hlen(ddds_dev, DDDS_FRAMETRACK_FAST2, (hsync_len + step2));
	mtk_ddds_set_hlen(ddds_dev, DDDS_FRAMETRACK_SLOW1, (hsync_len - step1));
	mtk_ddds_set_hlen(ddds_dev, DDDS_FRAMETRACK_SLOW2, (hsync_len - step2));
	mtk_ddds_close_loop(ddds_dev, true);
	mtk_ddds_frmtrk_mode(ddds_dev, true);
	while (1) {
		lock_result = mtk_ddds_locked(ddds_dev);
		if (!lock_result) {
			if (i < 150) {
				i++;
				msleep(20);
				continue;
			} else {
				dev_info(ddds_dev,
					 "%s ddds can't lock within 1s\n",
					__func__);
				return 0; /* -1; */
			}
		} else {
			dev_info(ddds_dev, "%s ddds lock within count = %d\n",
				 __func__, i);
			return 0;
		}
	}
}

static int mtk_dispsys_ddds_disable(struct mtk_dispsys *dispsys, bool xtal)
{
	struct device *ddds_dev = NULL;

	if (!dispsys)
		return -EFAULT;

	ddds_dev = dispsys->disp_dev[MTK_DISP_DDDS];

	dev_info(ddds_dev, "%s line: %d\n", __func__, __LINE__);

	mtk_ddds_refclk_out(ddds_dev, false, false);
	if (!xtal) {
		mtk_ddds_close_loop(ddds_dev, false);
		mtk_ddds_frmtrk_mode(ddds_dev, false);
		mtk_ddds_enable(ddds_dev, false);
	}

	return 0;
}

static int mtk_dispsys_frmtrk_config(struct mtk_dispsys *dispsys,
				     u32 lock_win, u32 turbo_win,
				     u32 target_line, u32 lcm_vtotal)
{
	struct device *frm_dev = NULL;

	if (!dispsys)
		return -EFAULT;

	frm_dev = dispsys->disp_dev[MTK_DISP_FRMTRK];

	dev_info(frm_dev, "%s line: %d\n", __func__, __LINE__);

	mtk_frmtrk_config_vtotal(frm_dev, (u16)lcm_vtotal, NULL);
	mtk_frmtrk_target_line(frm_dev, (u16)target_line, NULL);
	mtk_frmtrk_lock_window(frm_dev, (u8)lock_win, NULL);
	mtk_frmtrk_turbo_window(frm_dev, (u16)turbo_win, NULL);

	return 0;
}

static int mtk_dispsys_frmtrk_enable(struct mtk_dispsys *dispsys, u32 mask,
				     struct cmdq_pkt *cmdq_handle)
{
	struct device *frm_dev = NULL;

	if (!dispsys)
		return -EFAULT;

	frm_dev = dispsys->disp_dev[MTK_DISP_FRMTRK];

	dev_info(frm_dev, "%s line: %d\n", __func__, __LINE__);

	if (mask > 1)
		mtk_frmtrk_set_mask(frm_dev, mask, mask - 1, cmdq_handle);
	else
		mtk_frmtrk_set_mask(frm_dev, 1, 0, cmdq_handle);

	mtk_frmtrk_start(frm_dev, cmdq_handle);

	return 0;
}

static int mtk_dispsys_frmtrk_lock(struct mtk_dispsys *dispsys)
{
	struct device *frm_dev = NULL;
	int i = 0;
	bool lock_result;

	if (!dispsys)
		return -EFAULT;

	frm_dev = dispsys->disp_dev[MTK_DISP_FRMTRK];

	dev_info(frm_dev, "%s line: %d\n", __func__, __LINE__);

	while (1) {
		lock_result = mtk_frmtrk_locked(frm_dev);
		if (!lock_result) {
			if (i < 200) {
				if (i % 50 == 0)
					pr_err("%s frame fracker is tuning!\n",
						__func__);
				i++;
				msleep(20);
				continue;
			} else {
				pr_err("%s frame fracker can't lock i = %d\n",
						__func__, i);
				return -1;
			}
		} else {
			pr_err("%s frame fracker lock within count = %d\n",
					__func__, i);
			return 0;
		}
	}
}

static int mtk_dispsys_frmtrk_init_state_check(struct mtk_dispsys *dispsys,
					       u32 target_line)
{
	struct device *frm_dev = NULL;
	int i = 0;
	u32 target;

	if (!dispsys)
		return -EFAULT;

	frm_dev = dispsys->disp_dev[MTK_DISP_FRMTRK];

	dev_info(frm_dev, "%s line: %d\n", __func__, __LINE__);

	while (1) {
		mtk_frmtrk_get_trk_dist(frm_dev, &target);
		if (i < 50) {
			if (target == target_line)
				return 0;

			i++;
			msleep(20);
		} else {
			return -EBUSY;
		}
	}
}

static int mtk_dispsys_frmtrk_disable(struct mtk_dispsys *dispsys)
{
	struct device *frm_dev = NULL;

	if (!dispsys)
		return -EFAULT;

	frm_dev = dispsys->disp_dev[MTK_DISP_FRMTRK];

	dev_info(frm_dev, "%s line: %d\n", __func__, __LINE__);

	mtk_frmtrk_stop(frm_dev);
	mtk_frmtrk_set_mask(frm_dev, 1, 0, NULL);

	return 0;
}

#endif

void mtk_dispsys_lhc_cb(struct mtk_mmsys_cb_data *data)
{
	struct mtk_dispsys *dispsys = data->priv_data;
	unsigned long flags;

	if (data->part_id == (1 << dispsys->disp_partition_nr) - 1 &&
	    data->status == LHC_CB_STA_FRAME_DONE) {
		spin_lock_irqsave(&dispsys->lock_irq, flags);
		dispsys->lhc_frame_done = 1;
		spin_unlock_irqrestore(&dispsys->lock_irq, flags);
		wake_up(&dispsys->lhc_wq);
	}
}

int mtk_dispsys_cmdq_mbox_create(struct mtk_dispsys *dispsys)
{
	struct device *dev = dispsys->dev;
	int i;

	for (i = 0; i < dispsys->disp_partition_nr; i++) {
		dispsys->client[i] = cmdq_mbox_create(dev, i);

		if (IS_ERR(dispsys->client[i])) {
			dev_err(dev, "failed to create client[%d]\n", i);
			cmdq_mbox_destroy(dispsys->client[i]);
			return -EINVAL;
		}
		pr_info("%s client[%d]=%p\n", __func__, i, dispsys->client[i]);
	}

	return 0;
}

int mtk_dispsys_cmdq_mbox_destroy(struct mtk_dispsys *dispsys)
{
	int i, err = 0;

	for (i = 0; i < dispsys->disp_partition_nr; i++)
		err |= cmdq_mbox_destroy(dispsys->client[i]);

	return err;
}

static int mtk_dispsys_modules_reset(struct mtk_dispsys *dispsys, int num,
				     enum mtk_mmsys_config_comp_id comp)
{
	struct device *dev = dispsys->dev;
	struct device *mmsys_dev = dispsys->disp_dev[MTK_DISP_MMSYS_CFG];
	int i;
	int ret = 0;

	for (i = 0; i < num; i++) {
		ret = mtk_mmsys_cfg_reset_module(mmsys_dev, NULL, (comp + i));
		if (ret) {
			dev_dbg(dev, "reset module failed %d\n", (comp + i));
			return ret;
		}
	}

	return ret;
}

static int mtk_dispsys_reset(struct mtk_dispsys *dispsys)
{
	int num;
	int ret = 0;

	num = dispsys->disp_partition_nr;

	if (dispsys->scenario == VR_DP_VIDEO_MODE) {
		/* MUTEX */
		mtk_dispsys_modules_reset(dispsys, 1, MMSYSCFG_COMPONENT_MUTEX);
#ifdef CONFIG_MACH_MT3612_A0
		/* Slicer */
		mtk_dispsys_modules_reset(dispsys, num,
					  MMSYSCFG_COMPONENT_SLCR_MOUT0);
		mtk_dispsys_modules_reset(dispsys, 3,
					  MMSYSCFG_COMPONENT_SLICER_MM);
#endif
	}

	/* RBFC */
	mtk_dispsys_modules_reset(dispsys, num, MMSYSCFG_COMPONENT_RBFC0);

	/* RDMA */
	if (dispsys->scenario == DISP_DRAM_MODE ||
	    dispsys->scenario == DISP_SRAM_MODE) {
		if (dispsys->fbdc_enable)
			mtk_dispsys_modules_reset(
					dispsys,
					num,
					MMSYSCFG_COMPONENT_MDP_RDMA_PVRIC0);
		else
			mtk_dispsys_modules_reset(dispsys, num,
						  MMSYSCFG_COMPONENT_MDP_RDMA0);
	} else if (dispsys->scenario == DUMP_ONLY_MODE) {
		mtk_dispsys_modules_reset(dispsys, num,
					  MMSYSCFG_COMPONENT_MDP_RDMA0);
	} else
		mtk_dispsys_modules_reset(dispsys, num,
					  MMSYSCFG_COMPONENT_DISP_RDMA0);

	/* RDMA MOUT */
	mtk_dispsys_modules_reset(dispsys, num, MMSYSCFG_COMPONENT_RDMA_MOUT0);

	/* LHC */
	mtk_dispsys_modules_reset(dispsys, 1,
				  MMSYSCFG_COMPONENT_LHC_SWAP);
	mtk_dispsys_modules_reset(dispsys, num,
				  MMSYSCFG_COMPONENT_LHC0);

	/* DSC */
	if (dispsys->disp_partition_nr > 1)
		mtk_dispsys_modules_reset(dispsys, 2, MMSYSCFG_COMPONENT_DSC0);
	else
		mtk_dispsys_modules_reset(dispsys, 1, MMSYSCFG_COMPONENT_DSC0);

	/* DSC MOUT */
	mtk_dispsys_modules_reset(dispsys, num, MMSYSCFG_COMPONENT_DSC_MOUT0);

	/* WDMA */
	if (dispsys->dump_enable)
		mtk_dispsys_modules_reset(dispsys, num,
					  MMSYSCFG_COMPONENT_DISP_WDMA0);

	/* Link Swap MOUT */
	mtk_dispsys_modules_reset(dispsys, 1, MMSYSCFG_COMPONENT_DSI_LANE_SWAP);

	/* DSI */
	mtk_dispsys_modules_reset(dispsys, num, MMSYSCFG_COMPONENT_DSI0);

	return ret;
}

static int mtk_dispsys_power_on(struct mtk_dispsys *dispsys)
{
	struct device *dev = dispsys->dev;
	struct device *mmsys_dev = dispsys->disp_dev[MTK_DISP_MMSYS_CFG];
	struct device *rdma_dev, *wdma_dev[4];
	struct device *rbfc_dev = dispsys->disp_dev[MTK_DISP_RBFC];
	struct device *lhc_dev = dispsys->disp_dev[MTK_DISP_LHC];
	struct device *slcr_dev = dispsys->disp_dev[MTK_DISP_SLCR];
	struct device *dsc_dev = dispsys->disp_dev[MTK_DISP_DSC];
	int i, ret = 0;

	/* power on mmsys_core */
	ret = mtk_mmsys_cfg_power_on(mmsys_dev);
	if (ret < 0) {
		dev_err(dev, "Failed to power on mmsys_core: %d\n", ret);
		return ret;
	}

	/* reset all module first */
	ret = mtk_dispsys_reset(dispsys);
	if (ret < 0) {
		dev_err(dev, "Failed to reset dispsys: %d\n", ret);
		return ret;
	}

	/* power on mutex */
	ret = mtk_mutex_power_on(dispsys->mutex[MUTEX_DISP]);
	if (ret < 0) {
		dev_err(dev, "Failed to power on mutex: %d\n", ret);
		return ret;
	}

	/* power on wdma */
	if (dispsys->dump_enable) {
		wdma_dev[0] = dispsys->disp_dev[MTK_DISP_WDMA0];
		wdma_dev[1] = dispsys->disp_dev[MTK_DISP_WDMA1];
		wdma_dev[2] = dispsys->disp_dev[MTK_DISP_WDMA2];
		wdma_dev[3] = dispsys->disp_dev[MTK_DISP_WDMA3];

		for (i = 0; i < dispsys->disp_partition_nr; i++) {
			ret = mtk_wdma_power_on(wdma_dev[i]);
			if (ret < 0) {
				dev_err(wdma_dev[i],
					"Failed to power on wdma%d: %d\n",
					i, ret);
				return ret;
			}
		}
	}

	/* power on rdma */
	rdma_dev = dispsys->disp_dev[MTK_DISP_PVRIC_RDMA];
	ret = mtk_rdma_power_on(rdma_dev);
	if (ret < 0) {
		dev_err(rdma_dev, "Failed to power on rdma: %d\n", ret);
		return ret;
	}
	if (dispsys->scenario == DISP_DRAM_MODE ||
	    dispsys->scenario == DISP_SRAM_MODE) {
		if (dispsys->fbdc_enable) {
			mtk_rdma_larb_get(rdma_dev,
			  ((dispsys->scenario == DISP_SRAM_MODE) ? 1 : 0));
		} else {
			rdma_dev = dispsys->disp_dev[MTK_DISP_MDP_RDMA];
			ret = mtk_rdma_power_on(rdma_dev);
			if (ret < 0) {
				dev_err(rdma_dev,
					"Failed to power on rdma: %d\n", ret);
				return ret;
			}
			mtk_rdma_larb_get(rdma_dev,
			  ((dispsys->scenario == DISP_SRAM_MODE) ? 1 : 0));
		}
	} else if (dispsys->scenario == DUMP_ONLY_MODE) {
		rdma_dev = dispsys->disp_dev[MTK_DISP_MDP_RDMA];
		ret = mtk_rdma_power_on(rdma_dev);
		if (ret < 0) {
			dev_err(rdma_dev, "Failed to power on rdma: %d\n", ret);
			return ret;
		}
		mtk_rdma_larb_get(rdma_dev, 0);
	} else if (dispsys->scenario == VR_DP_VIDEO_MODE) {
		rdma_dev = dispsys->disp_dev[MTK_DISP_DISP_RDMA];
		ret = mtk_rdma_power_on(rdma_dev);
		if (ret < 0) {
			dev_err(rdma_dev, "Failed to power on rdma: %d\n", ret);
			return ret;
		}
	} else {
		return -ENODEV;
	}

	/* power on slicer */
	if (dispsys->scenario == VR_DP_VIDEO_MODE)
		mtk_slcr_power_on(slcr_dev);

	/* power on rbfc */
	if ((dispsys->scenario == DISP_DRAM_MODE) ||
	    (dispsys->scenario == DISP_SRAM_MODE) ||
	    (dispsys->scenario == DUMP_ONLY_MODE)) {
		ret = mtk_rbfc_power_on(rbfc_dev);
		if (ret < 0) {
			dev_err(rbfc_dev, "Failed to power on rbfc: %d\n", ret);
			return ret;
		}
		ret = mtk_rbfc_set_plane_num(rbfc_dev, 1);
		if (ret < 0) {
			dev_err(rbfc_dev, "mtk_rbfc_set_plane_num fail!!\n");
			return ret;
		}
	}

	/* power on lhc */
	ret = mtk_lhc_power_on(lhc_dev);
	if (ret < 0) {
		dev_err(lhc_dev, "Failed to power on lhc: %d\n", ret);
		return ret;
	}

	/* power dsc */
	ret = dsc_power_on(dsc_dev);
	if (ret < 0) {
		dev_err(dsc_dev, "Failed to power on dsc: %d\n", ret);
		return ret;
	}

	return ret;
}

static int mtk_dispsys_power_off(struct mtk_dispsys *dispsys)
{
	struct device *mmsys_dev = dispsys->disp_dev[MTK_DISP_MMSYS_CFG];
	struct device *rdma_dev, *wdma_dev[4];
	struct device *rbfc_dev = dispsys->disp_dev[MTK_DISP_RBFC];
	struct device *lhc_dev = dispsys->disp_dev[MTK_DISP_LHC];
	struct device *slcr_dev = dispsys->disp_dev[MTK_DISP_SLCR];
	struct device *dsc_dev = dispsys->disp_dev[MTK_DISP_DSC];
	int i, ret = 0;

	/* power off lhc */
	mtk_lhc_power_off(lhc_dev);

	/* power off dsc */
	ret = dsc_power_off(dsc_dev);
	if (ret < 0)
		dev_err(dsc_dev, "Failed to power off dsc: %d\n", ret);

	/* power off slicer */
	if (dispsys->scenario == VR_DP_VIDEO_MODE)
		mtk_slcr_power_off(slcr_dev);

	/* power off rbfc */
	if ((dispsys->scenario == DISP_DRAM_MODE) ||
	    (dispsys->scenario == DISP_SRAM_MODE) ||
	    (dispsys->scenario == DUMP_ONLY_MODE)) {
		ret = mtk_rbfc_power_off(rbfc_dev);
		if (ret < 0)
			dev_err(rbfc_dev, "mtk_rbfc_power_off fail: %d\n", ret);
	}

	/* power off wdma */
	if (dispsys->scenario == DUMP_ONLY_MODE) {
		wdma_dev[0] = dispsys->disp_dev[MTK_DISP_WDMA0];
		wdma_dev[1] = dispsys->disp_dev[MTK_DISP_WDMA1];
		wdma_dev[2] = dispsys->disp_dev[MTK_DISP_WDMA2];
		wdma_dev[3] = dispsys->disp_dev[MTK_DISP_WDMA3];

		for (i = 0; i < dispsys->disp_partition_nr; i++) {
			ret = mtk_wdma_power_off(wdma_dev[i]);
			if (ret < 0)
				dev_err(wdma_dev[i],
					"Failed to power off wdma%d: %d\n",
					i, ret);
		}
	}

	/* power off rdma */
	if (dispsys->scenario == DISP_DRAM_MODE ||
	    dispsys->scenario == DISP_SRAM_MODE) {
		if (dispsys->fbdc_enable) {
			rdma_dev = dispsys->disp_dev[MTK_DISP_PVRIC_RDMA];

			mtk_rdma_larb_put(rdma_dev,
			  ((dispsys->scenario == DISP_SRAM_MODE) ? 1 : 0));
		} else {
			rdma_dev = dispsys->disp_dev[MTK_DISP_MDP_RDMA];

			mtk_rdma_larb_put(rdma_dev,
			  ((dispsys->scenario == DISP_SRAM_MODE) ? 1 : 0));

			mtk_rdma_power_off(rdma_dev);
		}
	} else if (dispsys->scenario == DUMP_ONLY_MODE) {
		rdma_dev = dispsys->disp_dev[MTK_DISP_MDP_RDMA];

		mtk_rdma_larb_put(rdma_dev, 0);

		mtk_rdma_power_off(rdma_dev);
	} else if (dispsys->scenario == VR_DP_VIDEO_MODE) {
		rdma_dev = dispsys->disp_dev[MTK_DISP_DISP_RDMA];
		mtk_rdma_power_off(rdma_dev);
	} else {
		return -ENODEV;
	}

	rdma_dev = dispsys->disp_dev[MTK_DISP_PVRIC_RDMA];
	mtk_rdma_power_off(rdma_dev);

	/* power off mutex */
	mtk_mutex_power_off(dispsys->mutex[MUTEX_DISP]);

	/* power off mmsys_core */
	mtk_mmsys_cfg_power_off(mmsys_dev);

	return ret;
}

static int mtk_dispsys_mutex_enable(struct mtk_dispsys *dispsys, bool mute)
{
	struct mtk_mutex_res *disp_mutex = dispsys->mutex[MUTEX_DISP];
	struct mtk_mutex_res *sbrc_mutex = dispsys->mutex[MUTEX_SBRC];
	struct mtk_mutex_res *mutex_delay = dispsys->mutex_delay[MUTEX_DISP];
	struct mtk_mutex_res *dp_mutex = dispsys->mutex[MUTEX_DP];
	int ret = 0;

	/* add component */
	mtk_mutex_add_comp(disp_mutex, MUTEX_COMPONENT_DSI0, NULL);
	mtk_mutex_add_comp(disp_mutex, MUTEX_COMPONENT_DSI1, NULL);
	mtk_mutex_add_comp(disp_mutex, MUTEX_COMPONENT_DSI2, NULL);
	mtk_mutex_add_comp(disp_mutex, MUTEX_COMPONENT_DSI3, NULL);

	mtk_mutex_add_comp(disp_mutex, MUTEX_COMPONENT_DSC0, NULL);
	mtk_mutex_add_comp(disp_mutex, MUTEX_COMPONENT_DSC1, NULL);
	mtk_mutex_add_comp(disp_mutex, MUTEX_COMPONENT_DSC2, NULL);
	mtk_mutex_add_comp(disp_mutex, MUTEX_COMPONENT_DSC3, NULL);

	mtk_mutex_add_comp(disp_mutex, MUTEX_COMPONENT_PAT_GEN_0, NULL);
	mtk_mutex_add_comp(disp_mutex, MUTEX_COMPONENT_PAT_GEN_1, NULL);
	mtk_mutex_add_comp(disp_mutex, MUTEX_COMPONENT_PAT_GEN_2, NULL);
	mtk_mutex_add_comp(disp_mutex, MUTEX_COMPONENT_PAT_GEN_3, NULL);

	if (dispsys->lhc_enable) {
		struct mtk_mutex_res *mutex;

		if (dispsys->scenario == VR_DP_VIDEO_MODE)
			mutex = dp_mutex;
		else
			mutex = disp_mutex;

		mtk_mutex_add_comp(mutex, MUTEX_COMPONENT_LHC0, NULL);
		mtk_mutex_add_comp(mutex, MUTEX_COMPONENT_LHC1, NULL);
		mtk_mutex_add_comp(mutex, MUTEX_COMPONENT_LHC2, NULL);
		mtk_mutex_add_comp(mutex, MUTEX_COMPONENT_LHC3, NULL);
	}

	if (dispsys->dump_enable) {
		mtk_mutex_add_comp(disp_mutex,
				   MUTEX_COMPONENT_DISP_WDMA0, NULL);
		mtk_mutex_add_comp(disp_mutex,
				   MUTEX_COMPONENT_DISP_WDMA1, NULL);
		mtk_mutex_add_comp(disp_mutex,
				   MUTEX_COMPONENT_DISP_WDMA2, NULL);
		mtk_mutex_add_comp(disp_mutex,
				   MUTEX_COMPONENT_DISP_WDMA3, NULL);
	}

	if (dispsys->scenario == DISP_DRAM_MODE ||
	    dispsys->scenario == DISP_SRAM_MODE) {
		if (dispsys->fbdc_enable) {
			mtk_mutex_add_comp(disp_mutex,
					   MUTEX_COMPONENT_MDP_RDMA_PVRIC0,
					   NULL);
			mtk_mutex_add_comp(disp_mutex,
					   MUTEX_COMPONENT_MDP_RDMA_PVRIC1,
					   NULL);
			mtk_mutex_add_comp(disp_mutex,
					   MUTEX_COMPONENT_MDP_RDMA_PVRIC2,
					   NULL);
			mtk_mutex_add_comp(disp_mutex,
					   MUTEX_COMPONENT_MDP_RDMA_PVRIC3,
					   NULL);
		} else {
			mtk_mutex_add_comp(disp_mutex,
					   MUTEX_COMPONENT_MDP_RDMA0,
					   NULL);
			mtk_mutex_add_comp(disp_mutex,
					   MUTEX_COMPONENT_MDP_RDMA1,
					   NULL);
			mtk_mutex_add_comp(disp_mutex,
					   MUTEX_COMPONENT_MDP_RDMA2,
					   NULL);
			mtk_mutex_add_comp(disp_mutex,
					   MUTEX_COMPONENT_MDP_RDMA3,
					   NULL);
		}
	} else if (dispsys->scenario == DUMP_ONLY_MODE) {
		mtk_mutex_add_comp(disp_mutex,
				   MUTEX_COMPONENT_MDP_RDMA0,
				   NULL);
		mtk_mutex_add_comp(disp_mutex,
				   MUTEX_COMPONENT_MDP_RDMA1,
				   NULL);
		mtk_mutex_add_comp(disp_mutex,
				   MUTEX_COMPONENT_MDP_RDMA2,
				   NULL);
		mtk_mutex_add_comp(disp_mutex,
				   MUTEX_COMPONENT_MDP_RDMA3,
				   NULL);
	} else if (dispsys->scenario == VR_DP_VIDEO_MODE) {
		mtk_mutex_add_comp(disp_mutex, MUTEX_COMPONENT_DISP_RDMA0,
				   NULL);
		mtk_mutex_add_comp(disp_mutex, MUTEX_COMPONENT_DISP_RDMA1,
				   NULL);
		mtk_mutex_add_comp(disp_mutex, MUTEX_COMPONENT_DISP_RDMA2,
				   NULL);
		mtk_mutex_add_comp(disp_mutex, MUTEX_COMPONENT_DISP_RDMA3,
				   NULL);

		mtk_mutex_add_comp(disp_mutex, MUTEX_COMPONENT_SLICER,
				   NULL);
	}

	/* enable mutex AH function for sbrc */
	/* 06 : strip_buffer0_r_unfinish (from mmsys_core) */
	/* 07 : strip_buffer1_r_unfinish (from mmsys_core) */
	/* 08 : strip_buffer2_r_unfinish (from mmsys_core) */
	/* 09 : strip_buffer3_r_unfinish (from mmsys_core) */

#if 0
	mtk_mutex_add_monitor(disp_mutex,
			      MUTEX_MMSYS_STRIP_BUFFER_R_UNFINISH_0, NULL);
	mtk_mutex_add_monitor(disp_mutex,
			      MUTEX_MMSYS_STRIP_BUFFER_R_UNFINISH_1, NULL);
	mtk_mutex_add_monitor(disp_mutex,
			      MUTEX_MMSYS_STRIP_BUFFER_R_UNFINISH_2, NULL);
	mtk_mutex_add_monitor(disp_mutex,
			      MUTEX_MMSYS_STRIP_BUFFER_R_UNFINISH_3, NULL);

	mtk_mutex_error_monitor_enable(disp_mutex, NULL, true);
	mtk_mutex_add_to_ext_signal(disp_mutex,
				    MMCORE_SBRC_BYPASS_EXT_SIGNAL, NULL);
#endif
	/* add RBFC module */
	mtk_mutex_add_comp(disp_mutex, MUTEX_COMPONENT_RBFC0, NULL);
	mtk_mutex_add_comp(disp_mutex, MUTEX_COMPONENT_RBFC1, NULL);
	mtk_mutex_add_comp(disp_mutex, MUTEX_COMPONENT_RBFC2, NULL);
	mtk_mutex_add_comp(disp_mutex, MUTEX_COMPONENT_RBFC3, NULL);

	/* select sof */
	if (dispsys->scenario == DUMP_ONLY_MODE) {
		mtk_mutex_select_sof(disp_mutex, MUTEX_MMSYS_SOF_SINGLE,
				     NULL, false);
	} else if (dispsys->scenario == DISP_SRAM_MODE) {
		mtk_mutex_set_delay_cycle(mutex_delay, 26000000, 12, NULL);
		mtk_mutex_select_delay_sof(mutex_delay,
					   MUTEX_MMSYS_SOF_DSI0, NULL);
		mtk_mutex_select_sof(disp_mutex, MUTEX_MMSYS_SOF_SYNC_DELAY0,
				     NULL, false);
		mtk_mutex_delay_enable(mutex_delay, NULL);
	} else if (dispsys->scenario == VR_DP_VIDEO_MODE) {
		mtk_mutex_select_sof(disp_mutex, MUTEX_MMSYS_SOF_DSI0,
				     NULL, false);
		mtk_mutex_select_sof(dp_mutex, MUTEX_MMSYS_SOF_DP,
				     NULL, false);
	} else {
		mtk_mutex_select_sof(disp_mutex, MUTEX_MMSYS_SOF_DSI0,
				     NULL, false);
	}

	/* enable mutex */
	if (!mute)
		mtk_mutex_enable(disp_mutex, NULL);

	if (dispsys->scenario == VR_DP_VIDEO_MODE)
		mtk_mutex_enable(dp_mutex, NULL);

	mtk_mutex_add_comp(sbrc_mutex, MUTEX_COMPONENT_SBRC, NULL);
	mtk_mutex_select_sof(sbrc_mutex,
			     MUTEX_MMSYS_SOF_DSI0,
			     NULL, false);
	/* mtk_mutex_enable(sbrc_mutex, NULL); */

	return ret;
}

static int mtk_dispsys_mutex_disable(struct mtk_dispsys *dispsys)
{
	struct mtk_mutex_res *disp_mutex = dispsys->mutex[MUTEX_DISP];
	struct mtk_mutex_res *sbrc_mutex = dispsys->mutex[MUTEX_SBRC];
	struct mtk_mutex_res *dp_mutex = dispsys->mutex[MUTEX_DP];
	int ret = 0;

	mtk_mutex_disable(disp_mutex, NULL);
	mtk_mutex_disable(sbrc_mutex, NULL);
	mtk_mutex_disable(dp_mutex, NULL);

	mtk_mutex_remove_comp(disp_mutex, MUTEX_COMPONENT_DSI0, NULL);
	mtk_mutex_remove_comp(disp_mutex, MUTEX_COMPONENT_DSI1, NULL);
	mtk_mutex_remove_comp(disp_mutex, MUTEX_COMPONENT_DSI2, NULL);
	mtk_mutex_remove_comp(disp_mutex, MUTEX_COMPONENT_DSI3, NULL);

	mtk_mutex_remove_comp(disp_mutex, MUTEX_COMPONENT_DSC0, NULL);
	mtk_mutex_remove_comp(disp_mutex, MUTEX_COMPONENT_DSC1, NULL);
	mtk_mutex_remove_comp(disp_mutex, MUTEX_COMPONENT_DSC2, NULL);
	mtk_mutex_remove_comp(disp_mutex, MUTEX_COMPONENT_DSC3, NULL);

	mtk_mutex_remove_comp(disp_mutex, MUTEX_COMPONENT_PAT_GEN_0, NULL);
	mtk_mutex_remove_comp(disp_mutex, MUTEX_COMPONENT_PAT_GEN_1, NULL);
	mtk_mutex_remove_comp(disp_mutex, MUTEX_COMPONENT_PAT_GEN_2, NULL);
	mtk_mutex_remove_comp(disp_mutex, MUTEX_COMPONENT_PAT_GEN_3, NULL);

	if (dispsys->lhc_enable) {
		struct mtk_mutex_res *mutex;

		if (dispsys->scenario == VR_DP_VIDEO_MODE)
			mutex = dp_mutex;
		else
			mutex = disp_mutex;

		mtk_mutex_remove_comp(mutex, MUTEX_COMPONENT_LHC0, NULL);
		mtk_mutex_remove_comp(mutex, MUTEX_COMPONENT_LHC1, NULL);
		mtk_mutex_remove_comp(mutex, MUTEX_COMPONENT_LHC2, NULL);
		mtk_mutex_remove_comp(mutex, MUTEX_COMPONENT_LHC3, NULL);
	}

	if (dispsys->dump_enable) {
		mtk_mutex_remove_comp(disp_mutex,
				      MUTEX_COMPONENT_DISP_WDMA0, NULL);
		mtk_mutex_remove_comp(disp_mutex,
				      MUTEX_COMPONENT_DISP_WDMA1, NULL);
		mtk_mutex_remove_comp(disp_mutex,
				      MUTEX_COMPONENT_DISP_WDMA2, NULL);
		mtk_mutex_remove_comp(disp_mutex,
				      MUTEX_COMPONENT_DISP_WDMA3, NULL);
	}

	if (dispsys->scenario == DISP_DRAM_MODE ||
	    dispsys->scenario == DISP_SRAM_MODE) {
		if (dispsys->fbdc_enable) {
			mtk_mutex_remove_comp(disp_mutex,
					      MUTEX_COMPONENT_MDP_RDMA_PVRIC0,
					      NULL);
			mtk_mutex_remove_comp(disp_mutex,
					      MUTEX_COMPONENT_MDP_RDMA_PVRIC1,
					      NULL);
			mtk_mutex_remove_comp(disp_mutex,
					      MUTEX_COMPONENT_MDP_RDMA_PVRIC2,
					      NULL);
			mtk_mutex_remove_comp(disp_mutex,
					      MUTEX_COMPONENT_MDP_RDMA_PVRIC3,
					      NULL);
		} else {
			mtk_mutex_remove_comp(disp_mutex,
					      MUTEX_COMPONENT_MDP_RDMA0,
					      NULL);
			mtk_mutex_remove_comp(disp_mutex,
					      MUTEX_COMPONENT_MDP_RDMA1,
					      NULL);
			mtk_mutex_remove_comp(disp_mutex,
					      MUTEX_COMPONENT_MDP_RDMA2,
					      NULL);
			mtk_mutex_remove_comp(disp_mutex,
					      MUTEX_COMPONENT_MDP_RDMA3,
					      NULL);
		}
	} else if (dispsys->scenario == DUMP_ONLY_MODE) {
		mtk_mutex_remove_comp(disp_mutex,
				      MUTEX_COMPONENT_MDP_RDMA0,
				      NULL);
		mtk_mutex_remove_comp(disp_mutex,
				      MUTEX_COMPONENT_MDP_RDMA1,
				      NULL);
		mtk_mutex_remove_comp(disp_mutex,
				      MUTEX_COMPONENT_MDP_RDMA2,
				      NULL);
		mtk_mutex_remove_comp(disp_mutex,
				      MUTEX_COMPONENT_MDP_RDMA3,
				      NULL);
	} else if (dispsys->scenario == VR_DP_VIDEO_MODE) {
		mtk_mutex_remove_comp(disp_mutex,
				      MUTEX_COMPONENT_DISP_RDMA0, NULL);
		mtk_mutex_remove_comp(disp_mutex,
				      MUTEX_COMPONENT_DISP_RDMA1, NULL);
		mtk_mutex_remove_comp(disp_mutex,
				      MUTEX_COMPONENT_DISP_RDMA2, NULL);
		mtk_mutex_remove_comp(disp_mutex,
				      MUTEX_COMPONENT_DISP_RDMA3, NULL);

		mtk_mutex_remove_comp(disp_mutex,
				      MUTEX_COMPONENT_SLICER, NULL);
	}

#if 0
	/* remove rbfc mutex AH function */
	mtk_mutex_remove_from_ext_signal(disp_mutex,
				 MMCORE_SBRC_BYPASS_EXT_SIGNAL, NULL);
	mtk_mutex_error_monitor_disable(disp_mutex, NULL);
	mtk_mutex_remove_monitor(disp_mutex,
				MUTEX_MMSYS_STRIP_BUFFER_R_UNFINISH_0, NULL);
	mtk_mutex_remove_monitor(disp_mutex,
				MUTEX_MMSYS_STRIP_BUFFER_R_UNFINISH_1, NULL);
	mtk_mutex_remove_monitor(disp_mutex,
				MUTEX_MMSYS_STRIP_BUFFER_R_UNFINISH_2, NULL);
	mtk_mutex_remove_monitor(disp_mutex,
				MUTEX_MMSYS_STRIP_BUFFER_R_UNFINISH_3, NULL);
	mtk_mutex_error_clear(disp_mutex, NULL);

	/* remove sbrc mutex AH function */
	mtk_mutex_remove_from_ext_signal(sbrc_mutex,
					 MMCORE_SBRC_BYPASS_EXT_SIGNAL, NULL);
	mtk_mutex_error_monitor_disable(sbrc_mutex, NULL);
	mtk_mutex_remove_monitor(sbrc_mutex,
				 MUTEX_MMSYS_STRIP_BUFFER_W_UNFINISH, NULL);
	mtk_mutex_error_clear(sbrc_mutex, NULL);
#endif
	mtk_mutex_remove_comp(sbrc_mutex, MUTEX_COMPONENT_SBRC, NULL);
	mtk_mutex_remove_comp(disp_mutex, MUTEX_COMPONENT_RBFC0, NULL);
	mtk_mutex_remove_comp(disp_mutex, MUTEX_COMPONENT_RBFC1, NULL);
	mtk_mutex_remove_comp(disp_mutex, MUTEX_COMPONENT_RBFC2, NULL);
	mtk_mutex_remove_comp(disp_mutex, MUTEX_COMPONENT_RBFC3, NULL);

	return ret;
}

static int mtk_dispsys_path_create(struct mtk_dispsys *dispsys)
{
	struct device *mmsys_dev = dispsys->disp_dev[MTK_DISP_MMSYS_CFG];
	enum mtk_mmsys_config_comp_id rdma = MMSYSCFG_COMPONENT_MDP_RDMA0;
	u32 ret = 0;

	/* DSC --> Lane swap */
	if (dispsys->scenario != DUMP_ONLY_MODE) {
		ret |= mtk_mmsys_cfg_connect_comp(
					mmsys_dev,
					MMSYSCFG_COMPONENT_DSC0,
					MMSYSCFG_COMPONENT_DSI_LANE_SWAP,
					NULL);
		ret |= mtk_mmsys_cfg_connect_comp(
					mmsys_dev,
					MMSYSCFG_COMPONENT_DSC0,
					MMSYSCFG_COMPONENT_DSI_LANE_SWAP,
					NULL);
		ret |= mtk_mmsys_cfg_connect_comp(
					mmsys_dev,
					MMSYSCFG_COMPONENT_DSC1,
					MMSYSCFG_COMPONENT_DSI_LANE_SWAP,
					NULL);
		ret |= mtk_mmsys_cfg_connect_comp(
					mmsys_dev,
					MMSYSCFG_COMPONENT_DSC1,
					MMSYSCFG_COMPONENT_DSI_LANE_SWAP,
					NULL);
	}

	if (dispsys->dump_enable) {
		ret |= mtk_mmsys_cfg_connect_comp(
					mmsys_dev,
					MMSYSCFG_COMPONENT_DSC0,
					MMSYSCFG_COMPONENT_DISP_WDMA0,
					NULL);
		ret |= mtk_mmsys_cfg_connect_comp(
					mmsys_dev,
					MMSYSCFG_COMPONENT_DSC0,
					MMSYSCFG_COMPONENT_DISP_WDMA1,
					NULL);
		ret |= mtk_mmsys_cfg_connect_comp(
					mmsys_dev,
					MMSYSCFG_COMPONENT_DSC1,
					MMSYSCFG_COMPONENT_DISP_WDMA2,
					NULL);
		ret |= mtk_mmsys_cfg_connect_comp(
					mmsys_dev,
					MMSYSCFG_COMPONENT_DSC1,
					MMSYSCFG_COMPONENT_DISP_WDMA3,
					NULL);
	}

	if (dispsys->scenario == DISP_DRAM_MODE ||
	    dispsys->scenario == DISP_SRAM_MODE) {
		if (dispsys->fbdc_enable)
			rdma = MMSYSCFG_COMPONENT_MDP_RDMA_PVRIC0;
		else
			rdma = MMSYSCFG_COMPONENT_MDP_RDMA0;
	} else if (dispsys->scenario == DUMP_ONLY_MODE) {
		rdma = MMSYSCFG_COMPONENT_MDP_RDMA0;
	} else if (dispsys->scenario == VR_DP_VIDEO_MODE) {
		rdma = MMSYSCFG_COMPONENT_DISP_RDMA0;
	}

	ret |= mtk_mmsys_cfg_connect_comp(
					mmsys_dev,
					rdma,
					MMSYSCFG_COMPONENT_DP_PAT_GEN0,
					NULL);
	ret |= mtk_mmsys_cfg_connect_comp(
					mmsys_dev,
					rdma + 1,
					MMSYSCFG_COMPONENT_DP_PAT_GEN1,
					NULL);
	ret |= mtk_mmsys_cfg_connect_comp(
					mmsys_dev,
					rdma + 2,
					MMSYSCFG_COMPONENT_DP_PAT_GEN2,
					NULL);
	ret |= mtk_mmsys_cfg_connect_comp(
					mmsys_dev,
					rdma + 3,
					MMSYSCFG_COMPONENT_DP_PAT_GEN3,
					NULL);

	if (dispsys->scenario == VR_DP_VIDEO_MODE) {
		if (dispsys->dsc_passthru) {
			ret |= mtk_mmsys_cfg_connect_comp(
						mmsys_dev,
						MMSYSCFG_COMPONENT_SLICER_DSC,
						rdma,
						NULL);
			ret |= mtk_mmsys_cfg_connect_comp(
						mmsys_dev,
						MMSYSCFG_COMPONENT_SLICER_DSC,
						rdma + 1,
						NULL);
			ret |= mtk_mmsys_cfg_connect_comp(
						mmsys_dev,
						MMSYSCFG_COMPONENT_SLICER_DSC,
						rdma + 2,
						NULL);
			ret |= mtk_mmsys_cfg_connect_comp(
						mmsys_dev,
						MMSYSCFG_COMPONENT_SLICER_DSC,
						rdma + 3,
						NULL);
			ret |= mtk_mmsys_cfg_slicer_to_ddds_select(mmsys_dev,
						MMSYSCFG_SLCR_DSC_TO_DDDS,
						NULL);
		} else {
			ret |= mtk_mmsys_cfg_connect_comp(
						mmsys_dev,
						MMSYSCFG_COMPONENT_SLICER_VID,
						rdma,
						NULL);
			ret |= mtk_mmsys_cfg_connect_comp(
						mmsys_dev,
						MMSYSCFG_COMPONENT_SLICER_VID,
						rdma + 1,
						NULL);
			ret |= mtk_mmsys_cfg_connect_comp(
						mmsys_dev,
						MMSYSCFG_COMPONENT_SLICER_VID,
						rdma + 2,
						NULL);
			ret |= mtk_mmsys_cfg_connect_comp(
						mmsys_dev,
						MMSYSCFG_COMPONENT_SLICER_VID,
						rdma + 3,
						NULL);
			ret |= mtk_mmsys_cfg_slicer_to_ddds_select(mmsys_dev,
						MMSYSCFG_SLCR_VID_TO_DDDS,
						NULL);
		}
	}

	if (dispsys->lhc_enable) {
		if (dispsys->scenario == VR_DP_VIDEO_MODE) {
			ret |= mtk_mmsys_cfg_connect_comp(
						mmsys_dev,
						MMSYSCFG_COMPONENT_SLICER_VID,
						MMSYSCFG_COMPONENT_LHC0,
						NULL);
			ret |= mtk_mmsys_cfg_connect_comp(
						mmsys_dev,
						MMSYSCFG_COMPONENT_SLICER_VID,
						MMSYSCFG_COMPONENT_LHC1,
						NULL);
			ret |= mtk_mmsys_cfg_connect_comp(
						mmsys_dev,
						MMSYSCFG_COMPONENT_SLICER_VID,
						MMSYSCFG_COMPONENT_LHC2,
						NULL);
			ret |= mtk_mmsys_cfg_connect_comp(
						mmsys_dev,
						MMSYSCFG_COMPONENT_SLICER_VID,
						MMSYSCFG_COMPONENT_LHC3,
						NULL);
		} else {
			ret |= mtk_mmsys_cfg_connect_comp(
						mmsys_dev,
						rdma,
						MMSYSCFG_COMPONENT_LHC0,
						NULL);
			ret |= mtk_mmsys_cfg_connect_comp(
						mmsys_dev,
						rdma + 1,
						MMSYSCFG_COMPONENT_LHC1,
						NULL);
			ret |= mtk_mmsys_cfg_connect_comp(
						mmsys_dev,
						rdma + 2,
						MMSYSCFG_COMPONENT_LHC2,
						NULL);
			ret |= mtk_mmsys_cfg_connect_comp(
						mmsys_dev,
						rdma + 3,
						MMSYSCFG_COMPONENT_LHC3,
						NULL);
		}
	}

	return ret;
}

static int mtk_dispsys_path_destroy(struct mtk_dispsys *dispsys)
{
	struct device *mmsys_dev = dispsys->disp_dev[MTK_DISP_MMSYS_CFG];
	enum mtk_mmsys_config_comp_id rdma = MMSYSCFG_COMPONENT_MDP_RDMA0;
	u32 ret = 0;

	/* DSC --> Lane swap */
	if (dispsys->scenario != DUMP_ONLY_MODE) {
		ret |= mtk_mmsys_cfg_disconnect_comp(
					mmsys_dev,
					MMSYSCFG_COMPONENT_DSC0,
					MMSYSCFG_COMPONENT_DSI_LANE_SWAP,
					NULL);
		ret |= mtk_mmsys_cfg_disconnect_comp(
					mmsys_dev,
					MMSYSCFG_COMPONENT_DSC0,
					MMSYSCFG_COMPONENT_DSI_LANE_SWAP,
					NULL);
		ret |= mtk_mmsys_cfg_disconnect_comp(
					mmsys_dev,
					MMSYSCFG_COMPONENT_DSC1,
					MMSYSCFG_COMPONENT_DSI_LANE_SWAP,
					NULL);
		ret |= mtk_mmsys_cfg_disconnect_comp(
					mmsys_dev,
					MMSYSCFG_COMPONENT_DSC1,
					MMSYSCFG_COMPONENT_DSI_LANE_SWAP,
					NULL);
	}

	if (dispsys->dump_enable) {
		ret |= mtk_mmsys_cfg_disconnect_comp(
					mmsys_dev,
					MMSYSCFG_COMPONENT_DSC0,
					MMSYSCFG_COMPONENT_DISP_WDMA0,
					NULL);
		ret |= mtk_mmsys_cfg_disconnect_comp(
					mmsys_dev,
					MMSYSCFG_COMPONENT_DSC0,
					MMSYSCFG_COMPONENT_DISP_WDMA1,
					NULL);
		ret |= mtk_mmsys_cfg_disconnect_comp(
					mmsys_dev,
					MMSYSCFG_COMPONENT_DSC1,
					MMSYSCFG_COMPONENT_DISP_WDMA2,
					NULL);
		ret |= mtk_mmsys_cfg_disconnect_comp(
					mmsys_dev,
					MMSYSCFG_COMPONENT_DSC1,
					MMSYSCFG_COMPONENT_DISP_WDMA3,
					NULL);
	}

	if (dispsys->scenario == DISP_DRAM_MODE ||
	    dispsys->scenario == DISP_SRAM_MODE) {
		if (dispsys->fbdc_enable)
			rdma = MMSYSCFG_COMPONENT_MDP_RDMA_PVRIC0;
		else
			rdma = MMSYSCFG_COMPONENT_MDP_RDMA0;
	} else if (dispsys->scenario == DUMP_ONLY_MODE) {
		rdma = MMSYSCFG_COMPONENT_MDP_RDMA0;
	} else if (dispsys->scenario == VR_DP_VIDEO_MODE) {
		rdma = MMSYSCFG_COMPONENT_DISP_RDMA0;
	}

	ret |= mtk_mmsys_cfg_disconnect_comp(
				mmsys_dev,
				rdma,
				MMSYSCFG_COMPONENT_DP_PAT_GEN0,
				NULL);
	ret |= mtk_mmsys_cfg_disconnect_comp(
				mmsys_dev,
				rdma + 1,
				MMSYSCFG_COMPONENT_DP_PAT_GEN1,
				NULL);
	ret |= mtk_mmsys_cfg_disconnect_comp(
				mmsys_dev,
				rdma + 2,
				MMSYSCFG_COMPONENT_DP_PAT_GEN2,
				NULL);
	ret |= mtk_mmsys_cfg_disconnect_comp(
				mmsys_dev,
				rdma + 3,
				MMSYSCFG_COMPONENT_DP_PAT_GEN3,
				NULL);

	if (dispsys->scenario == VR_DP_VIDEO_MODE) {
		ret |= mtk_mmsys_cfg_slicer_to_ddds_select(mmsys_dev,
						MMSYSCFG_SLCR_VID_TO_DDDS,
						NULL);
		if (dispsys->dsc_passthru) {
			ret |= mtk_mmsys_cfg_disconnect_comp(
						mmsys_dev,
						MMSYSCFG_COMPONENT_SLICER_DSC,
						rdma,
						NULL);
			ret |= mtk_mmsys_cfg_disconnect_comp(
						mmsys_dev,
						MMSYSCFG_COMPONENT_SLICER_DSC,
						rdma + 1,
						NULL);
			ret |= mtk_mmsys_cfg_disconnect_comp(
						mmsys_dev,
						MMSYSCFG_COMPONENT_SLICER_DSC,
						rdma + 2,
						NULL);
			ret |= mtk_mmsys_cfg_disconnect_comp(
						mmsys_dev,
						MMSYSCFG_COMPONENT_SLICER_DSC,
						rdma + 3,
						NULL);
		} else {
			ret |= mtk_mmsys_cfg_disconnect_comp(
						mmsys_dev,
						MMSYSCFG_COMPONENT_SLICER_VID,
						rdma,
						NULL);
			ret |= mtk_mmsys_cfg_disconnect_comp(
						mmsys_dev,
						MMSYSCFG_COMPONENT_SLICER_VID,
						rdma + 1,
						NULL);
			ret |= mtk_mmsys_cfg_disconnect_comp(
						mmsys_dev,
						MMSYSCFG_COMPONENT_SLICER_VID,
						rdma + 2,
						NULL);
			ret |= mtk_mmsys_cfg_disconnect_comp(
						mmsys_dev,
						MMSYSCFG_COMPONENT_SLICER_VID,
						rdma + 3,
						NULL);
		}
	}

	if (dispsys->lhc_enable) {
		if (dispsys->scenario == VR_DP_VIDEO_MODE) {
			ret |= mtk_mmsys_cfg_disconnect_comp(
						mmsys_dev,
						MMSYSCFG_COMPONENT_SLICER_VID,
						MMSYSCFG_COMPONENT_LHC0,
						NULL);
			ret |= mtk_mmsys_cfg_disconnect_comp(
						mmsys_dev,
						MMSYSCFG_COMPONENT_SLICER_VID,
						MMSYSCFG_COMPONENT_LHC1,
						NULL);
			ret |= mtk_mmsys_cfg_disconnect_comp(
						mmsys_dev,
						MMSYSCFG_COMPONENT_SLICER_VID,
						MMSYSCFG_COMPONENT_LHC2,
						NULL);
			ret |= mtk_mmsys_cfg_disconnect_comp(
						mmsys_dev,
						MMSYSCFG_COMPONENT_SLICER_VID,
						MMSYSCFG_COMPONENT_LHC3,
						NULL);
		} else {
			ret |= mtk_mmsys_cfg_disconnect_comp(
						mmsys_dev,
						rdma,
						MMSYSCFG_COMPONENT_LHC0,
						NULL);
			ret |= mtk_mmsys_cfg_disconnect_comp(
						mmsys_dev,
						rdma + 1,
						MMSYSCFG_COMPONENT_LHC1,
						NULL);
			ret |= mtk_mmsys_cfg_disconnect_comp(
						mmsys_dev,
						rdma + 2,
						MMSYSCFG_COMPONENT_LHC2,
						NULL);
			ret |= mtk_mmsys_cfg_disconnect_comp(
						mmsys_dev,
						rdma + 3,
						MMSYSCFG_COMPONENT_LHC3,
						NULL);
		}
	}

	return ret;
}

static int mtk_dispsys_wdma_enable(struct mtk_dispsys *dispsys)
{
	struct device *wdma_dev[4];
	u32 format = dispsys->resolution.output_format;
	u32 width = dispsys->resolution.input_width;
	u32 height = dispsys->resolution.input_height;
	u32 offset, pitch;
	int i, ret = 0;

	wdma_dev[0] = dispsys->disp_dev[MTK_DISP_WDMA0];
	wdma_dev[1] = dispsys->disp_dev[MTK_DISP_WDMA1];
	wdma_dev[2] = dispsys->disp_dev[MTK_DISP_WDMA2];
	wdma_dev[3] = dispsys->disp_dev[MTK_DISP_WDMA3];

	for (i = 0; i < dispsys->disp_partition_nr; i++)
		mtk_wdma_set_active_eye(wdma_dev[i], MTK_WDMA_ACTIVE_EYE_ALL);

	if (dispsys->dsc_enable || dispsys->dsc_passthru) {
		width = dispsys->resolution.input_width /
			dispsys->disp_partition_nr;

		width = (width * mtk_dispsys_format_to_bpp_for_dsc(format) / 8 /
			 dispsys->encode_rate + 2) / 3;

		dev_dbg(wdma_dev[0], "width = %d", width);
		pitch = width * dispsys->disp_partition_nr * 24 / 8;
		dev_dbg(wdma_dev[0], "pitch = %d", pitch);

		for (i = 0; i < dispsys->disp_partition_nr; i++) {
			offset = width * i * 24 / 8;
			mtk_wdma_set_region(wdma_dev[i], NULL,
					    width, height, width, height, 0, 0);

			mtk_wdma_set_multi_out_buf_addr_offset(wdma_dev[i],
					NULL, MTK_WDMA_OUT_BUF_0, offset);

			mtk_wdma_set_out_buf(wdma_dev[i], NULL,
					     dispsys->buf.pa_lcd +
					     (dispsys->buf.size >> 1),
					     pitch, DRM_FORMAT_BGR888);
		}
	} else if (dispsys->fbdc_enable &&
				(dispsys->scenario == DUMP_ONLY_MODE)) {
		offset = (height * width + 127) / 128;
		offset = roundup(offset, 64);
		pitch = dispsys->resolution.input_width *
			mtk_dispsys_format_to_bpp(format) / 8;

		mtk_wdma_set_region(wdma_dev[0], NULL,
				    width, height, width, height, 0, 0);

		mtk_wdma_pvric_enable(wdma_dev[0], true);
		mtk_wdma_set_out_buf(wdma_dev[0], NULL,
				     dispsys->buf.pa_lcd +
				     (dispsys->buf.size >> 1) + offset,
				     pitch, format);
	} else {
		width = width / dispsys->disp_partition_nr;
		pitch = dispsys->resolution.input_width *
			mtk_dispsys_format_to_bpp(format) / 8;

		for (i = 0; i < dispsys->disp_partition_nr; i++) {
			offset = width * i *
				 mtk_dispsys_format_to_bpp(format) / 8;
			mtk_wdma_set_region(wdma_dev[i], NULL,
					    width, height, width, height, 0, 0);
			mtk_wdma_set_multi_out_buf_addr_offset(wdma_dev[i],
					NULL, MTK_WDMA_OUT_BUF_0, offset);
			mtk_wdma_set_out_buf(wdma_dev[i], NULL,
					     dispsys->buf.pa_lcd +
					     (dispsys->buf.size >> 1),
					     pitch, format);
		}
	}

	memset(dispsys->buf.va_lcd + (dispsys->buf.size >> 1), 0,
	       dispsys->buf.size >> 1);

	dev_dbg(wdma_dev[0],
		"wdma_addr = 0x%p and size = 0x%lx\n",
		(void *)dispsys->buf.pa_lcd, (dispsys->buf.size >> 1));

	for (i = 0; i < dispsys->disp_partition_nr; i++)
		mtk_wdma_start(wdma_dev[i], NULL);

	return ret;
}

static int mtk_dispsys_rdma_enable(struct mtk_dispsys *dispsys, int rdma_layout)
{
	struct device *rdma_dev, *pvric_rdma;
	struct mtk_rdma_config *rdma_configs[2];
	struct mtk_rdma_config config[2];
	u8 rdma_type;
	int ret = 0;

	if (dispsys->scenario == DISP_DRAM_MODE ||
	    dispsys->scenario == DISP_SRAM_MODE) {
		if (dispsys->fbdc_enable) {
			rdma_dev = dispsys->disp_dev[MTK_DISP_PVRIC_RDMA];
			rdma_type = RDMA_TYPE_PVRIC;
		} else {
			rdma_dev = dispsys->disp_dev[MTK_DISP_MDP_RDMA];
			rdma_type = RDMA_TYPE_MDP;
		}
	} else if (dispsys->scenario == DUMP_ONLY_MODE) {
		rdma_dev = dispsys->disp_dev[MTK_DISP_MDP_RDMA];
		rdma_type = RDMA_TYPE_MDP;
	} else if (dispsys->scenario == VR_DP_VIDEO_MODE) {
		rdma_dev = dispsys->disp_dev[MTK_DISP_DISP_RDMA];
		rdma_type = RDMA_TYPE_DISP;
	} else
		return -ENODEV;

	pvric_rdma = dispsys->disp_dev[MTK_DISP_PVRIC_RDMA];
	ret = mtk_rdma_sel_sram(pvric_rdma, NULL, rdma_type);

	if (dispsys->scenario == VR_DP_VIDEO_MODE) {
		config[0].format = dispsys->resolution.input_format;

		config[0].mem_addr[0] = 0;
		config[0].mem_addr[1] = 0;
		config[0].mem_ufo_len_addr[0] = 0;
		config[0].mem_ufo_len_addr[1] = 0;
		config[0].y_pitch = 0;
		config[0].uv_pitch = 0;
		if (dispsys->dsc_passthru) {
			int bpp;
			int slice_w;

			bpp = mtk_dispsys_format_to_bpp_for_dsc(
							config[0].format) /
			      dispsys->encode_rate;
			slice_w = (dispsys->resolution.input_width /
				   dispsys->disp_partition_nr) * bpp;
			slice_w = (slice_w + 23) / 24;
			config[0].width = slice_w * dispsys->disp_partition_nr;
		} else {
			config[0].width = dispsys->resolution.input_width;
		}
		config[0].height = dispsys->resolution.input_height;
		config[0].sync_smi_channel = false;

		rdma_configs[0] = &config[0];
		rdma_configs[1] = NULL;
		ret |= mtk_rdma_config_frame(rdma_dev, 1, NULL, rdma_configs);
		ret |= mtk_rdma_start(rdma_dev, NULL);
	} else if (dispsys->scenario == DUMP_ONLY_MODE) {
		config[0].format = dispsys->resolution.input_format;

		config[0].mem_addr[0] = dispsys->disp_buf[0];
		config[0].mem_addr[1] = 0;
		config[0].mem_ufo_len_addr[0] = 0;
		config[0].mem_ufo_len_addr[1] = 0;
		config[0].y_pitch =
			dispsys->resolution.input_width *
			mtk_dispsys_format_to_bpp(config[0].format) / 8;
		config[0].uv_pitch = 0;
		config[0].width = dispsys->resolution.input_width;
		config[0].height = dispsys->resolution.input_height;
		config[0].sync_smi_channel = false;

		rdma_configs[0] = &config[0];
		rdma_configs[1] = NULL;
		ret |= mtk_rdma_config_frame(rdma_dev, 1, NULL, rdma_configs);
		ret |= mtk_rdma_start(rdma_dev, NULL);
	} else {
		int i;

		for (i = 0; i < 2; i++) {
			config[i].format = dispsys->resolution.input_format;

			config[i].mem_addr[0] = dispsys->disp_buf[i];
			config[i].mem_addr[1] = 0;
			config[i].mem_ufo_len_addr[0] = 0;
			config[i].mem_ufo_len_addr[1] = 0;
			if (dispsys->fbdc_enable)
				config[i].y_pitch =
					dispsys->resolution.input_width;
			else
				config[i].y_pitch =
					dispsys->resolution.input_width *
					mtk_dispsys_format_to_bpp(
							config[i].format) / 8;
			config[i].uv_pitch = 0;
			config[i].width = dispsys->resolution.input_width;
			config[i].height = dispsys->resolution.input_height;
			config[i].sync_smi_channel = false;

			if (rdma_layout == 1) {
				config[i].y_pitch = config[i].y_pitch / 2;
				config[i].width = config[i].width / 2;
			}

			rdma_configs[i] = &config[i];
		}

		if (rdma_layout == 1)
			ret |= mtk_rdma_config_frame(rdma_dev, 2, NULL,
						     rdma_configs);
		else
			ret |= mtk_rdma_config_frame(rdma_dev, 1, NULL,
						     rdma_configs);
		ret |= mtk_rdma_start(rdma_dev, NULL);
	}

	return ret;
}

static int mtk_dispsys_slcr_enable(struct mtk_dispsys *dispsys)
{
	struct device *slcr_dev = dispsys->disp_dev[MTK_DISP_SLCR];
	struct slcr_config cfg = {0};
	int slice_unit = 0;
	int i;
	int ret = 0;

	if (dispsys->scenario != VR_DP_VIDEO_MODE)
		return ret;

	/* slicer settings */
	if (dispsys->dsc_passthru) {
		if (dispsys->lhc_enable)
			cfg.in_format = DP_VIDEO_DSC;
		else
			cfg.in_format = DP_DSC;
	} else
		cfg.in_format = DP_VIDEO;

	if ((cfg.in_format == DP_VIDEO) || (cfg.in_format == DP_VIDEO_DSC)) {
		cfg.dp_video.input.width = dispsys->resolution.input_width;
		cfg.dp_video.input.height = dispsys->resolution.input_height;
		cfg.dp_video.input.pxl_swap = NO_SWAP;
		cfg.dp_video.input.in_ch[0] = CHANNEL_0;
		cfg.dp_video.input.in_ch[1] = CHANNEL_1;
		cfg.dp_video.input.in_ch[2] = CHANNEL_2;
		cfg.dp_video.output.sync_port = ORG_PORT;
		cfg.dp_video.output.out_ch[0] = CHANNEL_0;
		cfg.dp_video.output.out_ch[1] = CHANNEL_1;
		cfg.dp_video.output.out_ch[2] = CHANNEL_2;

		/* Should be aligned with DP */
#ifdef CONFIG_MACH_MT3612_A0
		cfg.dp_video.input.in_hsync_inv =
			!(dispsys->dp_video_info.vid_timing_msa.h_polarity);
		cfg.dp_video.input.in_vsync_inv =
			!(dispsys->dp_video_info.vid_timing_msa.v_polarity);

		dev_info(slcr_dev, "DP input hsync polarity= (%d)\n",
			dispsys->dp_video_info.vid_timing_msa.h_polarity);
		dev_info(slcr_dev, "DP input vsync polarity= (%d)\n",
			dispsys->dp_video_info.vid_timing_msa.v_polarity);
		dev_info(slcr_dev, "DP input format= (%d)\n",
			dispsys->dp_video_info.vid_color_fmt);

		if (cfg.in_format == DP_VIDEO) {
			u16 gce_height;
			u16 vtotal, vsync;

			cfg.dp_video.gce.event = GCE_INPUT;

			vtotal = dispsys->dp_video_info.vid_timing_msa.v_total;
			vsync =
			    dispsys->dp_video_info.vid_timing_msa.v_sync_width;
			gce_height = vtotal - dispsys->frmtrk_param.target_line;

			cfg.dp_video.gce.height = gce_height;

			cfg.dp_video.gce.width = 1;
		}

		cfg.dp_video.dither_en = 0;
		cfg.dp_video.csc_en = 0;
		if (dispsys->dp_video_info.vid_color_fmt == YUV444) {
			cfg.dp_video.csc_en = 1;
			cfg.dp_video.csc_mode = MTX_FULL601_TO_FULLSRGB;
			cfg.dp_video.input.in_ch[0] = CHANNEL_2;
			cfg.dp_video.input.in_ch[1] = CHANNEL_0;
			cfg.dp_video.input.in_ch[2] = CHANNEL_1;
		}
#else
		cfg.dp_video.input.in_hsync_inv = 1;
		cfg.dp_video.input.in_vsync_inv = 1;
#endif

		slice_unit = cfg.dp_video.input.width >> 2;

		for (i = 0; i < 4; i++) {
			cfg.dp_video.output.slice_sop[i] = slice_unit * i;
			cfg.dp_video.output.slice_eop[i] = (slice_unit *
							     (i + 1));
			cfg.dp_video.output.vid_en[i] = (0x1 << i);
		}
	}

	if ((cfg.in_format == DP_DSC) || (cfg.in_format == DP_VIDEO_DSC)) {
#ifdef CONFIG_MACH_MT3612_A0
		int pic_height, pic_width;
		int slice_height, slice_width;
		int chunk_size;
		int bpc, bpp;
		int slice_per_line, cycles_per_slice, cycles_per_line;
		int slcr_in_hsize, slcr_in_vsize;
		int valid_byte;
		struct pps_sdp_s *dp_pps_info;
		u16 gce_height;
		u16 vtotal, vsync;

		dp_pps_info = &dispsys->dp_pps_info;

		pic_height = (dp_pps_info->pps_db[6] << 8) |
			     (dp_pps_info->pps_db[7]);
		pic_width = (dp_pps_info->pps_db[8] << 8) |
			    (dp_pps_info->pps_db[9]);
		slice_height = (dp_pps_info->pps_db[10] << 8) |
			       (dp_pps_info->pps_db[11]);
		slice_width = (dp_pps_info->pps_db[12] << 8) |
			      (dp_pps_info->pps_db[13]);
		chunk_size = (dp_pps_info->pps_db[14] << 8) |
			     (dp_pps_info->pps_db[15]);
		bpc = dp_pps_info->pps_db[3] >> 4;
		bpp = ((dp_pps_info->pps_db[4] & 0x03) << 8) |
		      (dp_pps_info->pps_db[5]);
		slice_per_line = pic_width / slice_width;
		cycles_per_slice = (chunk_size % 6 == 0) ?
				   chunk_size / 6 :
				   (chunk_size / 6 + 1);
		cycles_per_line = slice_per_line * cycles_per_slice;
		slcr_in_hsize = cycles_per_line * 2;
		slcr_in_vsize = pic_height;

		slice_unit = cycles_per_slice * 2;

		valid_byte = chunk_size % 6;
		cfg.dp_dsc.output.valid_byte = 0;

		if (valid_byte == 0) {
			cfg.dp_dsc.output.valid_byte = 0x3F;
		} else {
			i = 0;
			while (valid_byte > 0) {
				cfg.dp_dsc.output.valid_byte |= 1 << i;
				i++;
				valid_byte--;
			}
		}

		cfg.dp_dsc.input.width = slcr_in_hsize;
		cfg.dp_dsc.input.height = slcr_in_vsize;
		cfg.dp_dsc.input.bit_rate = bpp / 16;
		cfg.dp_dsc.input.chunk_num = slice_per_line;
		cfg.dp_dsc.input.pxl_swap = NO_SWAP;
		cfg.dp_dsc.output.sync_port = ORG_PORT;

		/* Should be aligned with DP */
		cfg.dp_dsc.input.in_hsync_inv =
			!(dispsys->dp_video_info.vid_timing_msa.h_polarity);
		cfg.dp_dsc.input.in_vsync_inv =
			!(dispsys->dp_video_info.vid_timing_msa.v_polarity);

		dev_info(slcr_dev, "DP input hsync polarity= (%d)\n",
			dispsys->dp_video_info.vid_timing_msa.h_polarity);
		dev_info(slcr_dev, "DP input vsync polarity= (%d)\n",
			dispsys->dp_video_info.vid_timing_msa.v_polarity);
		dev_info(slcr_dev, "DP input format= (%d)\n",
			dispsys->dp_video_info.vid_color_fmt);

		cfg.dp_dsc.gce.event = GCE_INPUT;

		vtotal = dispsys->dp_video_info.vid_timing_msa.v_total;
		vsync = dispsys->dp_video_info.vid_timing_msa.v_sync_width;
		gce_height = vtotal - dispsys->frmtrk_param.target_line;

		cfg.dp_dsc.gce.height = gce_height;

		cfg.dp_dsc.gce.width = 1;
#else
		int bpp;
		int width;
		int valid_byte;

		bpp = mtk_dispsys_format_to_bpp_for_dsc(
					dispsys->resolution.input_format) /
		      dispsys->encode_rate;
		slice_unit = dispsys->resolution.input_width * bpp / 4;
		valid_byte = (slice_unit / 8) % 6;
		cfg.dp_dsc.output.valid_byte = 0;
		i = 0;
		while (valid_byte > 0) {
			cfg.dp_dsc.output.valid_byte |= 1 << i;
			i++;
			valid_byte--;
		}
		slice_unit = ((slice_unit + 47) / 48) * 2;
		width = slice_unit * 4;
		cfg.dp_dsc.input.width = width;
		cfg.dp_dsc.input.height = dispsys->resolution.input_height;
		cfg.dp_dsc.input.bit_rate = bpp;
		cfg.dp_dsc.input.chunk_num = 4;
		cfg.dp_dsc.input.pxl_swap = NO_SWAP;
		cfg.dp_dsc.output.sync_port = ORG_PORT;

		cfg.dp_dsc.input.in_hsync_inv = 1;
		cfg.dp_dsc.input.in_vsync_inv = 1;
#endif

		for (i = 0; i < 4; i++) {
			cfg.dp_dsc.output.slice_sop[i] = slice_unit * i;
			cfg.dp_dsc.output.slice_eop[i] = (slice_unit *
							   (i + 1));
			cfg.dp_dsc.output.dsc_en[i] = (0x1 << i);
		}
	}

	mtk_slcr_config(slcr_dev, NULL, &cfg);
	dprx_if_fifo_reset();
	mtk_slcr_start(slcr_dev, NULL);
	dprx_if_fifo_release();

	return ret;
}

static int mtk_dispsys_rbfc_enable(struct mtk_dispsys *dispsys)
{
	struct device *rbfc_dev = dispsys->disp_dev[MTK_DISP_RBFC];
	int ret = 0;

	if ((dispsys->scenario == DISP_DRAM_MODE) ||
	    (dispsys->scenario == DUMP_ONLY_MODE)) {
		ret = mtk_rbfc_start_mode(rbfc_dev, NULL,
					  MTK_RBFC_DISABLE_MODE);
		if (ret)
			dev_err(rbfc_dev,
				"mtk_rbfc_start_mode DISABLE fail!! %d\n", ret);
		dev_dbg(rbfc_dev, "disable rbfc\n");
	} else if (dispsys->scenario == DISP_SRAM_MODE) {
		ret = mtk_rbfc_start_mode(rbfc_dev, NULL,
					  MTK_RBFC_NORMAL_MODE);
		if (ret)
			dev_err(rbfc_dev,
				"mtk_rbfc_start_mode NOAMAL fail!! %d\n", ret);
		dev_dbg(rbfc_dev, "enable rbfc\n");
	} else
		return -EINVAL;

	return ret;
}

static int mtk_dispsys_dsc_enable(struct mtk_dispsys *dispsys)
{
	struct device *dsi_dev = dispsys->disp_dev[MTK_DISP_DSI];
	struct device *dsc_dev = dispsys->disp_dev[MTK_DISP_DSC];
	struct dsc_config dsc_cfg;
	int ret = 0;

	mtk_dsi_get_dsc_config(dsi_dev, &dsc_cfg);

	if (dispsys->dsc_passthru) {
		int bpp;
		int slice_w;

		bpp = mtk_dispsys_format_to_bpp_for_dsc(
					dispsys->resolution.input_format) /
		      dispsys->encode_rate;
		slice_w = (dispsys->resolution.input_width /
			   dispsys->disp_partition_nr) * bpp;
		slice_w = (slice_w + 23) / 24;
		dsc_cfg.disp_pic_w = slice_w * dispsys->disp_core_per_wrap;
		dsc_cfg.slice_h = 64;  /* to prevent slice size check fail */
		dsc_cfg.pic_format = MTK_DSI_FMT_RGB888;
	} else {
		dsc_cfg.disp_pic_w =
		    (dispsys->resolution.lcm_w / dispsys->disp_partition_nr) *
		    dispsys->disp_core_per_wrap;
	}

	dsc_cfg.disp_pic_h = dispsys->resolution.lcm_h;
	/* for resolution 3840x2160 */
	if (dsc_cfg.disp_pic_h == 2160 &&
	    (dsc_cfg.pic_format == MTK_DSI_FMT_COMPRESSION_24_12 ||
	     dsc_cfg.pic_format == MTK_DSI_FMT_COMPRESSION_30_15))
		dsc_cfg.slice_h = 144;
	if (dispsys->disp_partition_nr == 1)
		dsc_cfg.inout_sel = DSC_1_IN_1_OUT;

	pr_debug("pic_format = %d, dsc_enable = %d, encode_rate = %d\n",
		 dsc_cfg.pic_format, dispsys->dsc_enable, dispsys->encode_rate);

	ret = dsc_hw_init(dsc_dev, dsc_cfg, NULL, NULL);
	/* dsc_mute_color(dsc_dev, 0x3ffffc00, NULL); */
	/* dsc_sw_mute(dsc_dev, 1, NULL); */
	if (ret == 0) {
		if (dsc_cfg.pic_format < MTK_DSI_FMT_COMPRESSION_30_15)
			dsc_relay(dsc_dev, NULL);
		else
			dsc_start(dsc_dev, NULL);
	} else
		dsc_bypass(dsc_dev, NULL);

	return ret;
}

static void mtk_dispsys_err_handling_cb(struct cmdq_cb_data data)
{
	struct mtk_dispsys *dispsys = data.data;

	if (dispsys->dsi_monitor)
		pr_err("\e[1;31;40m====== dsi buffer underrun ======\e[0m\n");

	complete(&dispsys->dsi_err);
}

static int mtk_dispsys_err_monitor_task(void *data)
{
	int ret = 0;
	struct mtk_dispsys *dispsys = data;
	struct cmdq_pkt *cmdq_pkt;
	u32 event;

	/* mutex AH event 8 for DSI underrun event */
	event = CMDQ_EVENT_MMSYS_CORE_AH_EVENT_PIN_8;

	cmdq_pkt_create(&cmdq_pkt);
	cmdq_pkt_clear_event(cmdq_pkt, event);
	cmdq_pkt_wfe(cmdq_pkt, event);

	init_completion(&dispsys->dsi_err);
	while (dispsys->dsi_monitor) {
		cmdq_pkt_flush_async(dispsys->err_cmdq_client,
			cmdq_pkt, mtk_dispsys_err_handling_cb, dispsys);

		ret = wait_for_completion_interruptible(&dispsys->dsi_err);
		reinit_completion(&dispsys->dsi_err);
	}

	cmdq_pkt_destroy(cmdq_pkt);

	return ret;
}

static int mtk_dispsys_err_monitor_enable(struct mtk_dispsys *dispsys)
{
	dsi_monitor_task = kthread_run(mtk_dispsys_err_monitor_task,
				       dispsys, "dispsys_err_monitor");
	if (IS_ERR(dsi_monitor_task)) {
		pr_err("unable to create thread for dispsys err monitor.\n");
		dsi_monitor_task = NULL;
		return -EINVAL;
	}

	return 0;
}

static int mtk_dispsys_err_cmdq_mbox_create(struct mtk_dispsys *dispsys)
{
	struct device *dev = dispsys->dev;

	dispsys->err_cmdq_client = cmdq_mbox_create(dev, DSI_ERR_MON);
	if (IS_ERR(dispsys->err_cmdq_client)) {
		cmdq_mbox_destroy(dispsys->err_cmdq_client);
		dev_err(dev, "dispsys mbox create failed!\n");
		return -EINVAL;
	}

	return 0;
}

static int mtk_dispsys_err_cmdq_mbox_destroy(struct mtk_dispsys *dispsys)
{
	int ret = 0;

	ret |= cmdq_mbox_destroy(dispsys->err_cmdq_client);

	return ret;
}

static int mtk_dispsys_err_monitor_init(struct mtk_dispsys *dispsys)
{
	struct mtk_mutex_res *disp_mutex = dispsys->mutex[MUTEX_DISP];
	int ret = 0;

	mtk_dispsys_err_cmdq_mbox_create(dispsys);

	dsi_monitor_task = NULL;
	dispsys->dsi_monitor = true;

	/* mutex monitor enable */
	ret |= mtk_mutex_add_monitor(disp_mutex, MUTEX_MMSYS_DSI0_UNDERFLOW,
				     NULL);
	ret |= mtk_mutex_add_monitor(disp_mutex, MUTEX_MMSYS_DSI1_UNDERFLOW,
				     NULL);
	ret |= mtk_mutex_add_monitor(disp_mutex, MUTEX_MMSYS_DSI2_UNDERFLOW,
				     NULL);
	ret |= mtk_mutex_add_monitor(disp_mutex, MUTEX_MMSYS_DSI3_UNDERFLOW,
				     NULL);
	ret |= mtk_mutex_add_to_ext_signal(disp_mutex,
					   MMCORE_DSI_0_MUTE_EXT_SIGNAL, NULL);
	ret |= mtk_mutex_add_to_ext_signal(disp_mutex,
					   MMCORE_DSI_1_MUTE_EXT_SIGNAL, NULL);
	ret |= mtk_mutex_add_to_ext_signal(disp_mutex,
					   MMCORE_DSI_2_MUTE_EXT_SIGNAL, NULL);
	ret |= mtk_mutex_add_to_ext_signal(disp_mutex,
					   MMCORE_DSI_3_MUTE_EXT_SIGNAL, NULL);

	ret |= mtk_mutex_error_monitor_enable(disp_mutex, NULL, false);

	return ret;
}

static int mtk_dispsys_err_monitor_deinit(struct mtk_dispsys *dispsys)
{
	struct mtk_mutex_res *disp_mutex = dispsys->mutex[MUTEX_DISP];
	int ret = 0;

	dispsys->dsi_monitor = false;

	ret |= mtk_mutex_error_monitor_disable(disp_mutex, NULL);
	/* monitor release */
	ret |= mtk_mutex_remove_monitor(disp_mutex, MUTEX_MMSYS_DSI0_UNDERFLOW,
					NULL);
	ret |= mtk_mutex_remove_monitor(disp_mutex, MUTEX_MMSYS_DSI1_UNDERFLOW,
					NULL);
	ret |= mtk_mutex_remove_monitor(disp_mutex, MUTEX_MMSYS_DSI2_UNDERFLOW,
					NULL);
	ret |= mtk_mutex_remove_monitor(disp_mutex, MUTEX_MMSYS_DSI3_UNDERFLOW,
					NULL);

	ret |= mtk_mutex_error_clear(disp_mutex, NULL);

	ret |= mtk_mutex_remove_from_ext_signal(disp_mutex,
						MMCORE_DSI_0_MUTE_EXT_SIGNAL,
						NULL);
	ret |= mtk_mutex_remove_from_ext_signal(disp_mutex,
						MMCORE_DSI_1_MUTE_EXT_SIGNAL,
						NULL);
	ret |= mtk_mutex_remove_from_ext_signal(disp_mutex,
						MMCORE_DSI_2_MUTE_EXT_SIGNAL,
						NULL);
	ret |= mtk_mutex_remove_from_ext_signal(disp_mutex,
						MMCORE_DSI_3_MUTE_EXT_SIGNAL,
						NULL);

	ret |= mtk_dispsys_err_cmdq_mbox_destroy(dispsys);

	if (dsi_monitor_task) {
		kthread_stop(dsi_monitor_task);
		dsi_monitor_task = NULL;
	}

	return ret;
}

void *mtk_dispsys_get_dump_vaddr(struct device *dev)
{
	struct mtk_dispsys *dispsys = dev_get_drvdata(dev);

	return (dispsys->buf.va_lcd + (dispsys->buf.size >> 1));
}
EXPORT_SYMBOL(mtk_dispsys_get_dump_vaddr);

int mtk_dispsys_lhc_slices_enable(struct device *dev, int lhc_input_slice_nr,
				  u32 slicer_out_idx,
				  struct mtk_lhc_slice (*config)[4])
{
	struct mtk_dispsys *dispsys = dev_get_drvdata(dev);
	struct device *lhc_dev = dispsys->disp_dev[MTK_DISP_LHC];
	struct device *mmsys_dev = dispsys->disp_dev[MTK_DISP_MMSYS_CFG];
	unsigned long flags;
	int ret = 0;

	pr_debug("%s slicer number %d\n", __func__, lhc_input_slice_nr);

	if (lhc_input_slice_nr == 1) {
		if (slicer_out_idx > 3) {
			pr_err("%s invalid slice index:%d\n", __func__,
				slicer_out_idx);
			return -EINVAL;
		}
		ret = mtk_mmsys_cfg_lhc_swap_config(mmsys_dev,
					slicer_out_idx, slicer_out_idx,
					slicer_out_idx, slicer_out_idx, NULL);
	} else if (lhc_input_slice_nr == 2) {
		ret = mtk_mmsys_cfg_lhc_swap_config(mmsys_dev,
						0, 0, 1, 1, NULL);
	} else if (lhc_input_slice_nr == 4) {
		ret = mtk_mmsys_cfg_lhc_swap_config(mmsys_dev,
						0, 1, 2, 3, NULL);
	} else {
		pr_err("%s don't support slice number %d\n",
			__func__, lhc_input_slice_nr);
		return -EINVAL;
	}

	ret |= mtk_lhc_config_slices(lhc_dev, config, NULL);

	ret |= mtk_lhc_start(lhc_dev, NULL);

	ret |= mtk_lhc_register_cb(lhc_dev, mtk_dispsys_lhc_cb,
				  LHC_CB_STA_FRAME_DONE, dispsys);

	spin_lock_irqsave(&dispsys->lock_irq, flags);
	dispsys->lhc_frame_done = 0;
	spin_unlock_irqrestore(&dispsys->lock_irq, flags);

	return ret;
}
EXPORT_SYMBOL(mtk_dispsys_lhc_slices_enable);

int mtk_dispsys_lhc_enable(struct device *dev, u32 width, u32 height)
{
	struct mtk_dispsys *dispsys = dev_get_drvdata(dev);
	struct mtk_lhc_slice s[4];
	u32 i, cnt;

	cnt = min_t(u32, dispsys->disp_partition_nr, 4);
	if (cnt == 0) {
		pr_err("disp_partition_nr is 0!!!\n");
		return -EINVAL;
	}

	width /= cnt;
	for (i = 0; i < cnt; i++) {
		s[i].w = width;
		s[i].h = height;
		s[i].start = 0;
		s[i].end = width - 1;
	}

	return mtk_dispsys_lhc_slices_enable(dev, 4, 0, &s);
}
EXPORT_SYMBOL(mtk_dispsys_lhc_enable);

int mtk_dispsys_lhc_disable(struct device *dev)
{
	struct mtk_dispsys *dispsys = dev_get_drvdata(dev);
	struct device *lhc_dev = dispsys->disp_dev[MTK_DISP_LHC];
	int ret = 0;

	mtk_lhc_register_cb(lhc_dev, NULL, 0, NULL);
	ret = mtk_lhc_stop(lhc_dev, NULL);

	return ret;
}
EXPORT_SYMBOL(mtk_dispsys_lhc_disable);

int mtk_dispsys_read_lhc(struct device *dev, struct mtk_lhc_hist *data)
{
	struct mtk_dispsys *dispsys = dev_get_drvdata(dev);
	struct device *lhc_dev = dispsys->disp_dev[MTK_DISP_LHC];
	unsigned long flags;
	int ret = 0;
	/* int y, x; */

	spin_lock_irqsave(&dispsys->lock_irq, flags);
	dispsys->lhc_frame_done = 0;
	spin_unlock_irqrestore(&dispsys->lock_irq, flags);

	ret = wait_event_interruptible_timeout(dispsys->lhc_wq,
					       dispsys->lhc_frame_done,
					       msecs_to_jiffies(5000));

	dev_dbg(lhc_dev, "ret=%d, lhc_frame_done=%d\n",
		ret, dispsys->lhc_frame_done);

	if (ret != 0 && ret != (-ERESTARTSYS)) {
		ret = mtk_lhc_read_histogram(lhc_dev, data);
/*		if (ret == 0) {
 *			for (y = 0; y < LHC_BLK_Y_NUM; y++) {
 *				for (x = 0; x < LHC_X_BIN_NUM; x++) {
 *					dev_dbg(lhc_dev, "hist_r[%d][%d] = %X",
 *						y, x,
 *						data->lhc_hist_r[y][x]);
 *					dev_dbg(lhc_dev, "hist_g[%d][%d] = %X",
 *						y, x,
 *						data->lhc_hist_g[y][x]);
 *					dev_dbg(lhc_dev, "hist_b[%d][%d] = %X",
 *						y, x,
 *						data->lhc_hist_b[y][x]);
 *				}
 *			}
 *		}
 */	} else {
		return -EBUSY;
	}

	return ret;
}
EXPORT_SYMBOL(mtk_dispsys_read_lhc);

#ifdef CONFIG_MACH_MT3612_A0
static u32 dsi_data_rate[3][6] = {
	/* 59.94    60     89.91     90   119.88    120 */
	{908307, 909216, 908307, 909216, 907252, 908160}, /* vtotal=2200 */
	{908786, 909696, 908292, 909202, 908786, 909696}, /* vtotal=2060 */
	{909360, 909360, 908820, 908820, 909360, 909360}  /* vtotal=2250 */
};

/* 10bpc, DSC 1/3 */
static u32 dsi_hfp_wc_10bpc_1_3[3][6] = {
	/* 59.94    60     89.91     90   119.88    120 */
	{1780, 1780, 632, 632, 56, 56}, /* vtotal=2200 */
	{2016, 2016, 788, 788, 176, 176}, /* vtotal=2060 */
	{1756, 1756, 632, 632, 72, 72}  /* vtotal=2250 */
};

/* 10bpc, DSC 1/2 */
static u32 dsi_hfp_wc_10bpc_1_2[3][6] = {
	/*59.94 60 89.91 90 119.88 120 */
	{1156, 1156, 44, 44, 44, 44}, /* vtotal=2200 */
	{1392, 1392, 164, 164, 44, 44}, /* vtotal=2060 */
	{1156, 1156, 44, 44, 44, 44}  /* vtotal=2250 */
};

/* 8bpc, DSC 1/3 */
static u32 dsi_hfp_wc_8bpc_1_3[3][6] = {
	/* 59.94    60     89.91     90   119.88    120 */
	{2032, 2032, 884, 884, 308, 308}, /* vtotal=2200 */
	{2268, 2268, 1040, 1040, 428, 428}, /* vtotal=2060 */
	{1996, 1996, 872, 872, 312, 312}  /* vtotal=2250 */
};

/* 8bpc, DSC 1/2 */
static u32 dsi_hfp_wc_8bpc_1_2[3][6] = {
	/*59.94 60 89.91 90 119.88 120 */
	{1532, 1532, 384, 384, 44, 44}, /* vtotal=2200 */
	{1768, 1768, 540, 540, 44, 44}, /* vtotal=2060 */
	{1516, 1516, 392, 392, 44, 44}  /* vtotal=2250 */
};

static int mtk_dispsys_dsi_timing_para(struct mtk_dispsys *dispsys,
				       u32 *data_rate, u32 *hbp_wc, u32 *hfp_wc)
{
	struct device *dsi_dev = dispsys->disp_dev[MTK_DISP_DSI];
	struct device *dev = dispsys->dev;
	u32 format;
	u32 lcm_vtotal;
	u32 fps;
	int i, j;

	format = dispsys->resolution.output_format;
	fps = dispsys->resolution.framerate;
	mtk_dsi_get_lcm_vtotal(dsi_dev, &lcm_vtotal);

	*data_rate = 909216;
	*hfp_wc = 44;
	*hbp_wc = 210;

	if (lcm_vtotal == 2200)		/* 4000 * 2040 */
		i = 0;
	else if (lcm_vtotal == 2060)	/* reduce blanking */
		i = 1;
	else if (lcm_vtotal == 2250)	/* 3840 * 2160 */
		i = 2;
	else {
		dev_dbg(dev, "not supported lcm_vtotal(%d)!!\n", lcm_vtotal);
		return 0;
	}

	if (fps == 59) /* 59.94 */
		j = 0;
	else if (fps == 60)
		j = 1;
	else if (fps == 89) /* 89.91 */
		j = 2;
	else if (fps == 90)
		j = 3;
	else if (fps == 119) /* 119.88 */
		j = 4;
	else if (fps == 120)
		j = 5;
	else {
		dev_dbg(dev, "not support fps(%d)!!\n", fps);
		return 0;
	}

	*data_rate = dsi_data_rate[i][j];

	if ((!dispsys->dsc_enable) && (!dispsys->dsc_passthru)) {
		if (lcm_vtotal == 2200) {		/* 4000 * 2040 */
			*hfp_wc = 52;
			*hbp_wc = 190;
		} else if (lcm_vtotal == 2060) {	/* reduce blanking */
			*hfp_wc = 268;
			*hbp_wc = 210;
		} else {	/* 3840 * 2160 */
			*hfp_wc = 76;
			*hbp_wc = 210;
		}
	} else if (mtk_dispsys_is_rgb10(format)) {
		if (dispsys->encode_rate == 3)
			*hfp_wc = dsi_hfp_wc_10bpc_1_3[i][j];
		else
			*hfp_wc = dsi_hfp_wc_10bpc_1_2[i][j];
	} else {
		if (dispsys->encode_rate == 3)
			*hfp_wc = dsi_hfp_wc_8bpc_1_3[i][j];
		else
			*hfp_wc = dsi_hfp_wc_8bpc_1_2[i][j];
	}

	if (*hfp_wc == 44) {
		dev_dbg(dev, "not support (fps=%d, %dbpc, 1/%d DSC) !!\n",
			fps, (mtk_dispsys_is_rgb10(format) ? 10 : 8),
			dispsys->encode_rate);
	}

	dev_dbg(dev, "data_rate = %d, hfp_wc = %d, hbp_wc = %d\n",
		*data_rate, *hfp_wc, *hbp_wc);

	return 0;
}
#endif

int mtk_dispsys_enable(struct device *dev, bool mute, int rdma_layout)
{
	struct mtk_dispsys *dispsys = dev_get_drvdata(dev);
	struct device *dsi_dev = dispsys->disp_dev[MTK_DISP_DSI];
#ifdef CONFIG_MACH_MT3612_A0
	struct device *mmsys_dev = dispsys->disp_dev[MTK_DISP_MMSYS_CFG];
	struct mtk_mutex_res *disp_mutex = dispsys->mutex[MUTEX_DISP];
	struct cmdq_pkt *pkt;
	u32 event, lcm_index, data_rate, hbp, hfp;
#endif
	int ret = 0;

	if (is_test_started == 1)
		return 0;

	if (dispsys->scenario != DUMP_ONLY_MODE) {
#ifdef CONFIG_MACH_MT3612_A0
		/* fps > 60 and not enable dsc, return fail */
		if ((dispsys->resolution.framerate > 60) &&
		    (!dispsys->dsc_enable) && (!dispsys->dsc_passthru)) {
			dev_err(dev, "FPS cannot be over 60 if DSC off!!\n");
			return -EINVAL;
		}

		/* get dp video info */
		if ((dispsys->dp_enable) && (!dispsys->dp_video_set)) {
			dprx_get_video_info_msa(&dispsys->dp_video_info);
			dispsys->dp_video_set = true;
		}

		/* get dp dsc pps info */
		if ((dispsys->dp_enable) && (dispsys->dp_video_info.dsc_mode) &&
		    (!dispsys->dp_pps_set)) {
			dprx_get_pps_info(&dispsys->dp_pps_info);
			dispsys->dp_pps_set = true;
		}

		if (dispsys->frmtrk) { /* clock from ddds */
			u32 lcm_vtotal;

			mtk_dispsys_ddds_config(dispsys, TARGET_FREQ, MAX_FREQ,
						false);
			mtk_dsi_get_lcm_vtotal(dsi_dev, &lcm_vtotal);
			mtk_dispsys_frmtrk_config(
					dispsys,
					dispsys->frmtrk_param.lock_win,
					dispsys->frmtrk_param.turbor_win,
					dispsys->frmtrk_param.target_line,
					lcm_vtotal);
		} else /* clock from xtal */
			mtk_dispsys_ddds_config(dispsys, TARGET_FREQ, MAX_FREQ,
						true);
#endif
	}
	/* power on dispsys module - mmsys, mutex, rdma */
	ret = mtk_dispsys_power_on(dispsys);
	if (ret < 0) {
		dev_err(dev, "Failed to power on dispsys: %d\n", ret);
		return ret;
	}

	/* mmsys config -- path setting */
	ret = mtk_dispsys_path_create(dispsys);
	if (ret < 0) {
		dev_err(dev, "Failed to create path: %d\n", ret);
		return ret;
	}

	/* rbfc config */
	mtk_dispsys_rbfc_enable(dispsys);

	/* wdma config */
	if (dispsys->dump_enable)
		ret = mtk_dispsys_wdma_enable(dispsys);
	if (ret < 0) {
		dev_err(dev, "Failed to enable wdma: %d\n", ret);
		return ret;
	}

	/* rdma config */
	ret = mtk_dispsys_rdma_enable(dispsys, rdma_layout);
	if (ret < 0) {
		dev_err(dev, "Failed to enable rdma: %d\n", ret);
		return ret;
	}

	/* slicer config */
	ret = mtk_dispsys_slcr_enable(dispsys);
	if (ret < 0) {
		dev_err(dev, "Failed to enable slicer: %d\n", ret);
		return ret;
	}

	/* lhc config */
	if (dispsys->lhc_enable) {
		ret = mtk_dispsys_lhc_enable(dev,
					     dispsys->resolution.lcm_w,
					     dispsys->resolution.lcm_h);
	}
	if (ret < 0) {
		dev_err(dev, "Failed to enable lhc: %d\n", ret);
		return ret;
	}

	/* dsc config */
	mtk_dispsys_dsc_enable(dispsys);

	/* mutex config */
	mtk_dispsys_mutex_enable(dispsys, mute);

	/* dsi err monitor */
	if (dispsys->scenario == VR_DP_VIDEO_MODE) {
		mtk_dispsys_err_monitor_init(dispsys);
		mtk_dispsys_err_monitor_enable(dispsys);
	}

	/* dsi config */
	if (dispsys->scenario != DUMP_ONLY_MODE) {
#ifdef CONFIG_MACH_MT3612_A0
		mtk_dsi_get_lcm_index(dsi_dev, &lcm_index);
		if (lcm_index >= MTK_LCM_RGB888) {
			mtk_mmsys_cfg_dsi_lane_swap_config(mmsys_dev,
							   0, 1, 3, 2, NULL);
#if RUN_DSI_ANALYZER
			mtk_dsi_lane_swap(dsi_dev, 0);
#else
			mtk_dsi_lane_swap(dsi_dev, 3);
#endif
		} else {
			mtk_mmsys_cfg_dsi_lane_swap_config(mmsys_dev,
							   0, 1, 2, 3, NULL);
			mtk_dsi_lane_swap(dsi_dev, 0);
		}

		if ((!dispsys->dp_enable) &&
		    (dispsys->resolution.input_height == 2160))
			mtk_dsi_set_lcm_vblanking(dsi_dev, 1, 79, 10);

		mtk_dispsys_dsi_timing_para(dispsys, &data_rate, &hbp, &hfp);

		if (dispsys->frmtrk) {
			mtk_dispsys_ddds_enable(
					dispsys,
					dispsys->frmtrk_param.ddds_hlen,
					dispsys->frmtrk_param.ddds_step1,
					dispsys->frmtrk_param.ddds_step2);
		}

		mtk_dsi_set_datarate(dsi_dev, data_rate);
		mtk_dsi_lcm_reset(dsi_dev);
		mtk_dsi_hw_init(dsi_dev);
		usleep_range(5000, 6000);
		if (dispsys->dsc_passthru && amd_DSC) {
			mtk_dsi_lcm_setting(dsi_dev,
					    init_setting_ams336rp01_3_amd,
					    11);
			dev_dbg(dev, "Config panel with AMD PPS!\n");
		} else if (dispsys->dsc_passthru && nVidia_DSC) {
			mtk_dsi_lcm_setting(dsi_dev,
					    init_setting_ams336rp01_3_nVidia,
					    11);
			dev_dbg(dev, "Config panel with nVidia PPS!\n");
		} else {
			mtk_dsi_lcm_setting(dsi_dev, NULL, 0);
		}
		/* mtk_dsi_output_self_pattern(dsi_dev, true, 0x3ff00000); */
		mtk_dsi_sw_mute_config(dsi_dev, mute, NULL);
		mtk_dsi_hw_mute_config(dsi_dev, true);

		mtk_dsi_set_lcm_hblank(dsi_dev, 4, hbp, hfp);

		mtk_dsi_vact_event_config(dsi_dev, 0, 2041, true);

		if (dispsys->frmtrk) {
			int frm_lock;

			mtk_dispsys_frmtrk_init_state_check(
					dispsys,
					dispsys->frmtrk_param.target_line);

			ret = cmdq_pkt_create(&pkt);
			if (ret < 0)
				dev_err(dev, "cmdq_pkt_create for dsi start failed\n");

			if (dispsys->dsc_passthru)
				event = CMDQ_EVENT_MMSYS_CORE_SLICER_EVENT_1;
			else
				event = CMDQ_EVENT_MMSYS_CORE_SLICER_EVENT_0;

			cmdq_pkt_clear_event(pkt, event);
			cmdq_pkt_wfe(pkt, event);

			mtk_dsi_output_start(dsi_dev, pkt);

			mtk_dispsys_frmtrk_enable(
					dispsys,
					dispsys->frmtrk_param.mask, pkt);

			cmdq_pkt_flush(dispsys->client[0], pkt);
			cmdq_pkt_destroy(pkt);

			frm_lock = mtk_dispsys_frmtrk_lock(dispsys);

			if (mute && (dispsys->scenario == VR_DP_VIDEO_MODE)) {
				mtk_mutex_enable(disp_mutex, NULL);
				ret = cmdq_pkt_create(&pkt);
				if (ret < 0)
					dev_err(dev, "cmdq_pkt_create for unmute failed\n");

				event = CMDQ_EVENT_MMSYS_CORE_SOF_FOR_33_DSI0;

				cmdq_pkt_clear_event(pkt, event);
				cmdq_pkt_wfe(pkt, event);

				mtk_dsi_sw_mute_config(dsi_dev, false, pkt);

				cmdq_pkt_flush(dispsys->client[0], pkt);
				cmdq_pkt_destroy(pkt);
				dev_info(dev, "Display unmute!!\n");

				if (!frm_lock)
					dev_err(dev, "Display PASS!!\n");
				else
					dev_err(dev, "Display FAIL!!\n");
			}
		} else {
			mtk_dsi_output_start(dsi_dev, NULL);
		}

		curr_fps = dispsys->resolution.framerate;
#else
		mtk_dsi_set_fps(dsi_dev, MTK_DSI_FPS_60);
		mtk_dsi_lcm_reset(dsi_dev);
		mtk_dsi_hw_init(dsi_dev);
		mtk_dsi_lcm_setting(dsi_dev, NULL, 0);
		/* mtk_dsi_output_self_pattern(dsi_dev, true, 0x3ff00000); */
		mtk_dsi_sw_mute_config(dsi_dev, mute, NULL);
		mtk_dsi_hw_mute_config(dsi_dev, true);
		mtk_dsi_output_start(dsi_dev, NULL);
#endif
		if (dispsys->dsc_passthru && amd_DSC) {
			mtk_dsi_lcm_vm_setting(
					dsi_dev,
					ams336rp01_3_rgb10_with_dsc,
					sizeof(ams336rp01_3_rgb10_with_dsc) /
					sizeof(struct lcm_setting_table));
			dev_dbg(dev, "Config panel with AMD PPS!\n");
		} else if (dispsys->dsc_passthru && nVidia_DSC) {
			mtk_dsi_lcm_vm_setting(
					dsi_dev,
					ams336rp01_3_rgb10_with_dsc,
					sizeof(ams336rp01_3_rgb10_with_dsc) /
					sizeof(struct lcm_setting_table));
			dev_dbg(dev, "Config panel with nVidia PPS!\n");
		} else {
			mtk_dsi_lcm_vm_setting(dsi_dev, NULL, 0);
		}

		mtk_dsi_send_vm_cmd(dsi_dev, MIPI_DSI_DCS_SHORT_WRITE,
				    MIPI_DCS_SET_DISPLAY_ON, 0, NULL);
	}

	is_test_started = 1;

	return ret;
}
EXPORT_SYMBOL(mtk_dispsys_enable);

void mtk_dispsys_disable(struct device *dev)
{
	struct mtk_dispsys *dispsys = dev_get_drvdata(dev);
	struct device *dsi_dev = dispsys->disp_dev[MTK_DISP_DSI];
	struct device *dsc_dev = dispsys->disp_dev[MTK_DISP_DSC];
	struct device *wdma_dev[4];
	struct device *rdma_dev = NULL;
	struct device *rbfc_dev = dispsys->disp_dev[MTK_DISP_RBFC];
	struct device *slcr_dev = dispsys->disp_dev[MTK_DISP_SLCR];
	int i, ret = 0;

	if (is_test_started == 0)
		return;

	curr_fps = 0;

	if (dispsys->scenario == VR_DP_VIDEO_MODE)
		mtk_dispsys_err_monitor_deinit(dispsys);

	if (dispsys->scenario != DUMP_ONLY_MODE) {
#ifdef CONFIG_MACH_MT3612_A0
		struct cmdq_pkt *pkt;
		u32 event;

		mtk_dsi_sw_mute_config(dsi_dev, true, NULL);

		ret = cmdq_pkt_create(&pkt);
		if (ret < 0)
			dev_err(dev, "cmdq_pkt_create for dsi stop failed\n");

		event = CMDQ_EVENT_MMSYS_CORE_DSI0_SOF_EVENT;
		cmdq_pkt_clear_event(pkt, event);
		cmdq_pkt_wfe(pkt, event);
		cmdq_pkt_flush(dispsys->client[0], pkt);
		cmdq_pkt_destroy(pkt);

		mtk_dsi_output_stop(dsi_dev);
		if (dispsys->frmtrk)
			mtk_dispsys_frmtrk_disable(dispsys);
		mtk_dsi_send_dcs_cmd(dsi_dev, MIPI_DCS_SET_DISPLAY_OFF, 0,
				     NULL);
		mtk_dsi_send_dcs_cmd(dsi_dev, MIPI_DCS_ENTER_SLEEP_MODE, 0,
				     NULL);
		mtk_dsi_hw_fini(dsi_dev);
		if (dispsys->frmtrk)
			mtk_dispsys_ddds_disable(dispsys, false);
		else
			mtk_dispsys_ddds_disable(dispsys, true);

		mtk_dsi_lcm_reset(dsi_dev);
#else
		mtk_dsi_output_stop(dsi_dev);
		mtk_dsi_send_dcs_cmd(dsi_dev, MIPI_DCS_SET_DISPLAY_OFF, 0,
				     NULL);
		mtk_dsi_send_dcs_cmd(dsi_dev, MIPI_DCS_ENTER_SLEEP_MODE, 0,
				     NULL);
		mtk_dsi_hw_fini(dsi_dev);
#endif
	}

	dsc_bypass(dsc_dev, NULL);

	if (dispsys->dump_enable) {
		wdma_dev[0] = dispsys->disp_dev[MTK_DISP_WDMA0];
		wdma_dev[1] = dispsys->disp_dev[MTK_DISP_WDMA1];
		wdma_dev[2] = dispsys->disp_dev[MTK_DISP_WDMA2];
		wdma_dev[3] = dispsys->disp_dev[MTK_DISP_WDMA3];

		for (i = 0; i < dispsys->disp_partition_nr; i++) {
			mtk_wdma_stop(wdma_dev[i], NULL);
			if (dispsys->fbdc_enable &&
					(dispsys->scenario == DUMP_ONLY_MODE))
				mtk_wdma_pvric_enable(wdma_dev[i], false);
		}
	}

	if (dispsys->scenario == DISP_DRAM_MODE ||
	    dispsys->scenario == DISP_SRAM_MODE) {
		if (dispsys->fbdc_enable)
			rdma_dev = dispsys->disp_dev[MTK_DISP_PVRIC_RDMA];
		else
			rdma_dev = dispsys->disp_dev[MTK_DISP_MDP_RDMA];
	} else if (dispsys->scenario == DUMP_ONLY_MODE) {
		rdma_dev = dispsys->disp_dev[MTK_DISP_MDP_RDMA];
	} else
		rdma_dev = dispsys->disp_dev[MTK_DISP_DISP_RDMA];

	ret = mtk_rdma_stop(rdma_dev, NULL);
	if (ret)
		dev_err(rdma_dev, "mtk_rdma_stop failed!! %d\n", ret);

	if (dispsys->scenario == VR_DP_VIDEO_MODE)
		mtk_slcr_stop(slcr_dev, NULL);

	if (dispsys->lhc_enable)
		mtk_dispsys_lhc_disable(dev);

	if ((dispsys->scenario == DISP_DRAM_MODE) ||
	    (dispsys->scenario == DISP_SRAM_MODE) ||
	    (dispsys->scenario == DUMP_ONLY_MODE)) {
		ret = mtk_rbfc_start_mode(rbfc_dev, NULL,
					  MTK_RBFC_DISABLE_MODE);
		if (ret)
			dev_err(rbfc_dev,
				"mtk_rbfc_start_mode failed!! %d\n", ret);
		ret = mtk_rbfc_finish(rbfc_dev);
		if (ret)
			dev_err(rbfc_dev, "rbfc_finish failed!! %d\n", ret);
	}

	mtk_dispsys_mutex_disable(dispsys);

	mtk_dispsys_path_destroy(dispsys);

	mtk_dispsys_power_off(dispsys);

	dispsys->dp_video_set = false;
	dispsys->dp_pps_set = false;

	is_test_started = 0;
}
EXPORT_SYMBOL(mtk_dispsys_disable);

int mtk_dispsys_mute(struct device *dev, bool enable, struct cmdq_pkt *handle)
{
	struct mtk_dispsys *dispsys;
	struct device *dsi_dev;
	int ret;

	if (!dev)
		return -EFAULT;

	dispsys = dev_get_drvdata(dev);
	dsi_dev = dispsys->disp_dev[MTK_DISP_DSI];
	ret = mtk_dsi_sw_mute_config(dsi_dev, enable, handle);

	return ret;
}
EXPORT_SYMBOL(mtk_dispsys_mute);

static int mtk_dispsys_config_panel(struct mtk_dispsys *dispsys,
				    struct mtk_disp_para *para)
{
	struct device *dev = NULL;
	struct device *dsi_dev = NULL;
	u32 format, lcm_index;

	if (!dispsys)
		return -ENODEV;

	if (!para)
		return -EINVAL;

	dev = dispsys->dev;
	dsi_dev = dispsys->disp_dev[MTK_DISP_DSI];

#ifdef CONFIG_MACH_MT3612_A0
	format = para->output_format;
	if (mtk_dispsys_is_rgb10(format)) {
		if ((para->dsc_enable) || (para->dsc_passthru)) {
			if (para->encode_rate == 2)
				lcm_index = MTK_LCM_COMPRESSION_30_15;
			else
				if (para->input_width == 3840)
					lcm_index = MTK_LCM_3840_DSC_30_10;
				else
					lcm_index = MTK_LCM_COMPRESSION_30_10;
		} else {
			lcm_index = MTK_LCM_RGB888;
			dev_dbg(dev, "[Warning] RGB101010 without DSC is not support!");
		}
	} else {
		if ((para->dsc_enable) || (para->dsc_passthru)) {
			if (para->encode_rate == 2)
				lcm_index = MTK_LCM_COMPRESSION_24_12;
			else
				if (para->input_width == 3840)
					lcm_index = MTK_LCM_3840_DSC_24_8;
				else
					lcm_index = MTK_LCM_COMPRESSION_24_8;
		} else
			lcm_index = MTK_LCM_RGB888;
	}
	mtk_dsi_set_lcm_index(dsi_dev, lcm_index);
	mtk_dsi_get_lcm_width(dsi_dev, &dispsys->resolution.lcm_w);
	mtk_dsi_get_lcm_height(dsi_dev, &dispsys->resolution.lcm_h);
	if ((dispsys->resolution.lcm_w != para->input_width) ||
	    (dispsys->resolution.lcm_h != para->input_height)) {
		dev_dbg(dev,
			"[Warning] Input resolution is different with panel resolution!");
		dev_dbg(dev, "Input width = %d, height = %d",
			para->input_width, para->input_height);
		dev_dbg(dev, "Panel width = %d, height = %d",
			dispsys->resolution.lcm_w, dispsys->resolution.lcm_h);

		if (para->input_width && para->input_height) {
			dispsys->resolution.lcm_w = para->input_width;
			dispsys->resolution.lcm_h = para->input_height;

			mtk_dsi_set_lcm_width(dsi_dev, para->input_width);
			mtk_dsi_set_lcm_height(dsi_dev, para->input_height);
		}
	}
	if (mtk_dispsys_is_rgb10(format) && (lcm_index == MTK_LCM_RGB888))
		mtk_dsi_set_format(dsi_dev, MTK_DSI_FMT_RGB101010);
#else
	mtk_dsi_get_lcm_index(dsi_dev, &lcm_index);
	mtk_dsi_set_lcm_index(dsi_dev, lcm_index);

	mtk_dsi_get_lcm_width(dsi_dev, &dispsys->resolution.lcm_w);
	mtk_dsi_get_lcm_height(dsi_dev, &dispsys->resolution.lcm_h);
	format = para->input_format;

	if ((dispsys->resolution.lcm_w != para->input_width) ||
	    (dispsys->resolution.lcm_h != para->input_height)) {
		dev_dbg(dev,
			"Input resolution is different with panel resolution!");
		dev_dbg(dev, "Input width = %d, height = %d",
			para->input_width, para->input_height);
		dev_dbg(dev, "Panel width = %d, height = %d",
			dispsys->resolution.lcm_w, dispsys->resolution.lcm_h);

		if (para->input_width && para->input_height) {
			dispsys->resolution.lcm_w = para->input_width;
			dispsys->resolution.lcm_h = para->input_height;

			mtk_dsi_set_lcm_width(dsi_dev, para->input_width);
			mtk_dsi_set_lcm_height(dsi_dev, para->input_height);
		}
	}

	if (mtk_dispsys_is_rgb10(format)) {
		if ((para->dsc_enable) || (para->dsc_passthru)) {
			if (para->encode_rate == 2)
				format = MTK_DSI_FMT_COMPRESSION_30_15;
			else
				format = MTK_DSI_FMT_COMPRESSION_30_10;
		} else
			format = MTK_DSI_FMT_RGB101010;
	} else {
		if ((para->dsc_enable) || (para->dsc_passthru)) {
			if (para->encode_rate == 2)
				format = MTK_DSI_FMT_COMPRESSION_24_12;
			else
				format = MTK_DSI_FMT_COMPRESSION_24_8;
		} else
			format = MTK_DSI_FMT_RGB888;
	}

	if ((para->scenario == DUMP_ONLY_MODE) || (para->dsc_enable) ||
	    (para->dsc_passthru))
		mtk_dsi_set_format(dsi_dev, format);
#endif
	return 0;
}

int mtk_dispsys_config(struct device *dev, struct mtk_disp_para *para)
{
	struct mtk_dispsys *dispsys = NULL;

	if (!dev)
		return -ENODEV;

	if (!para)
		return -EINVAL;

	dispsys = dev_get_drvdata(dev);

	if (para->scenario > DISP_SCENARIO_NR)
		return -EINVAL;

	if (para->scenario == VR_DP_VIDEO_MODE)
		para->output_format = para->input_format;
	else if (!para->output_format)
		para->output_format = para->input_format;

	mtk_dispsys_config_panel(dispsys, para);

	dispsys->scenario = para->scenario;
	dispsys->dp_video_set = false;
	dispsys->dp_pps_set = false;

	switch (dispsys->scenario) {
	case DISP_DRAM_MODE:
	case DISP_SRAM_MODE:
		dispsys->resolution.input_format = para->input_format;
		dispsys->resolution.output_format = para->output_format;
		dispsys->resolution.input_height = dispsys->resolution.lcm_h;
		dispsys->resolution.input_width = dispsys->resolution.lcm_w;
		dispsys->resolution.framerate = para->disp_fps;
		dispsys->dsc_enable = para->dsc_enable;
		dispsys->encode_rate = para->encode_rate;
		dispsys->dsc_passthru = false;
		dispsys->lhc_enable = para->lhc_enable;
		dispsys->dump_enable = para->dump_enable;
		dispsys->fbdc_enable = para->fbdc_enable;
#ifdef CONFIG_MACH_MT3612_A0
		dispsys->dp_enable = para->dp_enable;
		dispsys->frmtrk = para->frmtrk;
#else
		dispsys->dp_enable = false;
		dispsys->frmtrk = false;
#endif
		dispsys->edid_sel = para->edid_sel;
		break;
	case DUMP_ONLY_MODE:
		dispsys->resolution.input_format = para->input_format;
		dispsys->resolution.output_format = para->output_format;
		dispsys->resolution.input_height = para->input_height;
		dispsys->resolution.input_width = para->input_width;
		dispsys->resolution.framerate = 0;
		dispsys->dsc_enable = para->dsc_enable;
		dispsys->encode_rate = para->encode_rate;
		dispsys->dsc_passthru = false;
		dispsys->lhc_enable = para->lhc_enable;
		dispsys->dump_enable = true;
		dispsys->fbdc_enable = para->fbdc_enable;
		dispsys->dp_enable = false;
		dispsys->frmtrk = false;
		dispsys->edid_sel = 0;
		break;
	case VR_DP_VIDEO_MODE:
		dispsys->resolution.input_format = para->input_format;
		dispsys->resolution.output_format = para->output_format;
		dispsys->resolution.input_height = para->input_height;
		dispsys->resolution.input_width = para->input_width;
		dispsys->resolution.framerate = para->input_fps;
		dispsys->dsc_enable = para->dsc_enable;
		dispsys->encode_rate = para->encode_rate;
		dispsys->dsc_passthru = para->dsc_passthru;
		dispsys->lhc_enable = para->lhc_enable;
		dispsys->dump_enable = para->dump_enable;
		dispsys->fbdc_enable = false;
		dispsys->dp_enable = para->dp_enable;
		dispsys->frmtrk = para->frmtrk;
		dispsys->edid_sel = para->edid_sel;
		break;
	default:
		dispsys->scenario = DISP_DRAM_MODE;
		dispsys->resolution.input_format = DRM_FORMAT_RGB888;
		dispsys->resolution.output_format = para->output_format;
		dispsys->resolution.input_height = dispsys->resolution.lcm_h;
		dispsys->resolution.input_width = dispsys->resolution.lcm_w;
		dispsys->resolution.framerate = para->disp_fps;
		dispsys->dsc_enable = false;
		dispsys->encode_rate = para->encode_rate;
		dispsys->dsc_passthru = false;
		dispsys->lhc_enable = false;
		dispsys->dump_enable = false;
		dispsys->fbdc_enable = false;
		dispsys->dp_enable = false;
		dispsys->frmtrk = false;
		dispsys->edid_sel = 0;
		break;
	}

	dev_dbg(dev, "Display parameters 0: scenario = %d", dispsys->scenario);
	dev_dbg(dev,
		"Display parameters 1: dsc_enable = %d, encode_rate = %d, dsc_passthru = %d",
		dispsys->dsc_enable, dispsys->encode_rate,
		dispsys->dsc_passthru);
	dev_dbg(dev,
		"Display parameters 2: lhc_enable = %d, dump_enable = %d",
		dispsys->lhc_enable, dispsys->dump_enable);
	dev_dbg(dev,
		"Display parameters 3: fbdc_enable = %d",
		dispsys->fbdc_enable);
	dev_dbg(dev,
		"Display parameters 4: input_format = %x, output_format = %x, fps = %d",
		dispsys->resolution.input_format,
		dispsys->resolution.output_format,
		dispsys->resolution.framerate);
	dev_dbg(dev,
		"Display parameters 5: input_height = %d, input_width = %d",
		dispsys->resolution.input_height,
		dispsys->resolution.input_width);
	dev_dbg(dev,
		"Display parameters 6: dp input = %d, frame tracking = %d",
		dispsys->dp_enable,
		dispsys->frmtrk);

	return 0;
}
EXPORT_SYMBOL(mtk_dispsys_config);

int mtk_dispsys_config_srcaddr(struct device *dev, dma_addr_t srcaddr,
			       struct cmdq_pkt **cmdq_handle)
{
	struct mtk_dispsys *dispsys = dev_get_drvdata(dev);
	struct device *rdma_dev = NULL;
	struct mtk_rdma_src rdma_info[2] = { 0 };
	int fbdc_enable = 0;

	if (dispsys->scenario != DISP_SRAM_MODE && !srcaddr)
		return -ENXIO;

	if (dispsys->scenario == DISP_DRAM_MODE ||
	    dispsys->scenario == DISP_SRAM_MODE) {
		if (dispsys->fbdc_enable) {
			rdma_dev = dispsys->disp_dev[MTK_DISP_PVRIC_RDMA];
			fbdc_enable = 1;
		} else
			rdma_dev = dispsys->disp_dev[MTK_DISP_MDP_RDMA];
	} else if (dispsys->scenario == DUMP_ONLY_MODE) {
		rdma_dev = dispsys->disp_dev[MTK_DISP_MDP_RDMA];
	} else {
		return -ENODEV;
	}

	if (fbdc_enable) {
		u32 header;

		header = (dispsys->resolution.input_height *
			  dispsys->resolution.input_width + 127) / 128;
		header = roundup(header, 64);
		srcaddr += header;
	}
	dispsys->disp_buf[0] = (dma_addr_t)srcaddr;
	dispsys->disp_buf[1] = (dma_addr_t)NULL;
	rdma_info[0].mem_addr[0] = srcaddr;
	rdma_info[0].mem_size[0] = 0;
	rdma_info[1].mem_addr[0] = srcaddr;
	rdma_info[1].mem_size[0] = 0;

	if (is_test_started)
		mtk_rdma_config_srcaddr(rdma_dev, cmdq_handle, rdma_info);

	return 0;
}
EXPORT_SYMBOL(mtk_dispsys_config_srcaddr);

int mtk_dispsys_config_pvric_2buf_srcaddr(struct device *dev,
					  dma_addr_t srcaddr1,
					  dma_addr_t srcaddr2,
					  struct cmdq_pkt **cmdq_handle)
{
	struct mtk_dispsys *dispsys = dev_get_drvdata(dev);
	struct device *rdma_dev = NULL;
	struct mtk_rdma_src rdma_info[2] = { 0 };
	u32 header;

	if (dispsys->scenario == DISP_DRAM_MODE) {
		if (dispsys->fbdc_enable)
			rdma_dev = dispsys->disp_dev[MTK_DISP_PVRIC_RDMA];
		else
			return -ENODEV;
	} else {
		return -ENODEV;
	}

	header = (dispsys->resolution.input_height *
		  (dispsys->resolution.input_width / 2) + 127) / 128;
	header = roundup(header, 256); /* 64->256: improve dram performance */
	srcaddr1 += header;
	srcaddr2 += header;

	dispsys->disp_buf[0] = (dma_addr_t)srcaddr1;
	dispsys->disp_buf[1] = (dma_addr_t)srcaddr2;
	rdma_info[0].mem_addr[0] = srcaddr1;
	rdma_info[0].mem_size[0] = 0;
	rdma_info[1].mem_addr[0] = srcaddr2;
	rdma_info[1].mem_size[0] = 0;

	if (is_test_started)
		mtk_rdma_config_srcaddr(rdma_dev, cmdq_handle, &rdma_info[0]);

	return 0;
}
EXPORT_SYMBOL(mtk_dispsys_config_pvric_2buf_srcaddr);

struct device *mtk_dispsys_get_rdma(struct device *dev)
{
	struct mtk_dispsys *dispsys = NULL;
	struct device *rdma_dev = NULL;

	if (!dev)
		return NULL;

	dispsys = dev_get_drvdata(dev);

	if (dispsys->scenario == DISP_DRAM_MODE ||
	    dispsys->scenario == DISP_SRAM_MODE) {
		if (dispsys->fbdc_enable)
			rdma_dev = dispsys->disp_dev[MTK_DISP_PVRIC_RDMA];
		else
			rdma_dev = dispsys->disp_dev[MTK_DISP_MDP_RDMA];
	} else if (dispsys->scenario == DUMP_ONLY_MODE) {
		rdma_dev = dispsys->disp_dev[MTK_DISP_MDP_RDMA];
	} else {
		return NULL;
	}

	return rdma_dev;
}
EXPORT_SYMBOL(mtk_dispsys_get_rdma);

struct device *mtk_dispsys_get_lhc(struct device *dev)
{
	struct mtk_dispsys *dispsys = NULL;
	struct device *lhc_dev = NULL;

	if (!dev)
		return NULL;

	dispsys = dev_get_drvdata(dev);

	lhc_dev = dispsys->disp_dev[MTK_DISP_LHC];

	return lhc_dev;
}
EXPORT_SYMBOL(mtk_dispsys_get_lhc);

struct device *mtk_dispsys_get_dsi(struct device *dev)
{
	struct mtk_dispsys *dispsys = NULL;
	struct device *dsi_dev = NULL;

	if (!dev)
		return NULL;

	dispsys = dev_get_drvdata(dev);

	dsi_dev = dispsys->disp_dev[MTK_DISP_DSI];

	return dsi_dev;
}
EXPORT_SYMBOL(mtk_dispsys_get_dsi);

struct mtk_mutex_res *mtk_dispsys_get_mutex_res(struct device *dev, int idx)
{
	struct mtk_dispsys *dispsys = NULL;
	struct mtk_mutex_res *mutex_res = NULL;

	if (!dev)
		return NULL;

	dispsys = dev_get_drvdata(dev);

	mutex_res = dispsys->mutex[idx];

	return mutex_res;
}
EXPORT_SYMBOL(mtk_dispsys_get_mutex_res);

int mtk_dispsys_get_current_fps(void)
{
	return curr_fps;
}
EXPORT_SYMBOL(mtk_dispsys_get_current_fps);

struct cmdq_pkt **mtk_dispsys_cmdq_pkt_create(struct device *dev)
{
	struct cmdq_pkt **pkt_ptr;
	struct mtk_dispsys *dispsys = NULL;
	int i;

	if (!dev)
		return (struct cmdq_pkt **)-EFAULT;

	dispsys = dev_get_drvdata(dev);

	pkt_ptr = kcalloc(dispsys->disp_partition_nr,
			  sizeof(*pkt_ptr), GFP_KERNEL);

	if (!pkt_ptr)
		return (struct cmdq_pkt **)-ENOMEM;

	for (i = 0; i < dispsys->disp_partition_nr; i++)
		WARN_ON(cmdq_pkt_create(&pkt_ptr[i]));

	return pkt_ptr;
}
EXPORT_SYMBOL(mtk_dispsys_cmdq_pkt_create);

int mtk_dispsys_cmdq_pkt_flush(struct device *dev, struct cmdq_pkt **pkt)
{
	struct mtk_dispsys *dispsys = NULL;
	int i, err = 0;

	if (!dev)
		return -EFAULT;

	dispsys = dev_get_drvdata(dev);

	for (i = 0; i < dispsys->disp_partition_nr; i++)
		err |= cmdq_pkt_flush(dispsys->client[i], pkt[i]);

	return err;
}
EXPORT_SYMBOL(mtk_dispsys_cmdq_pkt_flush);

int mtk_dispsys_cmdq_pkt_destroy(struct device *dev, struct cmdq_pkt **pkt)
{
	struct mtk_dispsys *dispsys = NULL;
	int i, err = 0;

	if (!dev)
		return -EFAULT;

	dispsys = dev_get_drvdata(dev);

	for (i = 0; i < dispsys->disp_partition_nr; i++)
		err |= cmdq_pkt_destroy(pkt[i]);

	kfree(pkt);

	return err;
}
EXPORT_SYMBOL(mtk_dispsys_cmdq_pkt_destroy);

#ifdef CONFIG_MACH_MT3612_A0
int mtk_dispsys_set_ddds_frmtrk_param(
			struct device *dev,
			u32 hsync_len, u32 ddds_step1, u32 ddds_step2,
			u32 target_line, u32 lock_win, u32 turbo_win, u32 mask)
{
	struct mtk_dispsys *dispsys = NULL;

	if (!dev)
		return -EFAULT;

	dispsys = dev_get_drvdata(dev);

	dispsys->frmtrk_param.ddds_hlen = hsync_len;
	dispsys->frmtrk_param.ddds_step1 = ddds_step1;
	dispsys->frmtrk_param.ddds_step2 = ddds_step2;

	dispsys->frmtrk_param.lock_win = lock_win;
	dispsys->frmtrk_param.turbor_win = turbo_win;
	dispsys->frmtrk_param.target_line = target_line;
	dispsys->frmtrk_param.mask = mask;

	return 0;
}
EXPORT_SYMBOL(mtk_dispsys_set_ddds_frmtrk_param);

int mtk_dispsys_set_dp_video_info(struct device *dev,
				  struct dprx_video_info_s *video)
{
	struct mtk_dispsys *dispsys = NULL;

	if (!dev)
		return -EFAULT;

	dispsys = dev_get_drvdata(dev);

	memcpy(&dispsys->dp_video_info, video, sizeof(dispsys->dp_video_info));
	dispsys->dp_video_set = true;

	return 0;
}
EXPORT_SYMBOL(mtk_dispsys_set_dp_video_info);

int mtk_dispsys_set_dp_pps_info(struct device *dev, struct pps_sdp_s *pps)
{
	struct mtk_dispsys *dispsys = NULL;

	if (!dev)
		return -EFAULT;

	dispsys = dev_get_drvdata(dev);

	memcpy(&dispsys->dp_pps_info, pps, sizeof(dispsys->dp_pps_info));
	dispsys->dp_pps_set = true;

	return 0;
}
EXPORT_SYMBOL(mtk_dispsys_set_dp_pps_info);

static int dispsys_vr_callback(struct device *dev, enum DPRX_NOTIFY_T event)
{
	struct mtk_dispsys *dispsys = NULL;
	struct device *dp_dev = NULL;
	struct device *dsi_dev = NULL;
	struct dprx_video_info_s dp_video_info = {0};
	struct pps_sdp_s dp_pps_info = {0};
	u32 dp_width, dp_vtotal, dp_fps;
	u32 lcm_vfp, lcm_vtotal;
	u32 step1, step2, hlen;
	u32 lock_win, turbo_win, target_line;
#ifndef CHECK_PARA /* Overwrite input parameter */
	struct mtk_disp_para disp_para;
#endif

	if (!dev)
		return -EFAULT;

	dispsys = dev_get_drvdata(dev);
	dp_dev = dispsys->disp_dev[MTK_DISP_DPRX];
	dsi_dev = dispsys->disp_dev[MTK_DISP_DSI];

	if (is_test_started) {
		/* disable display */
		mtk_dispsys_disable(dev);
	}

	if (event == DPRX_RX_VIDEO_STABLE) {
		int ret;

		/*get vedio timing from input side */
		ret = dprx_get_video_info_msa(&dp_video_info);
		if (ret) {
			dev_err(dev, "dprx_get_video_info_msa failed!\n");
			return -EINVAL;
		}

		mtk_dispsys_set_dp_video_info(dev, &dp_video_info);
		dp_width = dp_video_info.vid_timing_msa.h_active;
		dp_vtotal = dp_video_info.vid_timing_msa.v_total;
		dp_fps = dp_video_info.frame_rate;
		nVidia_DSC = false;
		amd_DSC = false;

		if (dprx_get_dsc_mode_status()) {
			int pic_height, pic_width;
			int slice_height, slice_width;
			int chunk_size;
			int bpc, bpp;
			int slice_per_line, cycles_per_slice, cycles_per_line;
			int slcr_in_hsize, slcr_in_vsize;

			/* int i; */

			dprx_get_pps_info(&dp_pps_info);
			mtk_dispsys_set_dp_pps_info(dev, &dp_pps_info);

/*
 *			for (i = 0; i < 128; i++) {
 *				dev_dbg(dev, "DP DSC PPS[%d] = %x\n", i,
 *					dp_pps_info.pps_db[i]);
 *			}
 */
			if (dispsys->edid_sel == 5) {
				amd_DSC = true;
				dev_dbg(dev, "AMD Card!\n");
			} else if ((dp_pps_info.pps_db[22] == 0xC6) &&
			    (dp_pps_info.pps_db[64] == 0x2A) &&
			    (dp_pps_info.pps_db[84] == 0x63)) {
				nVidia_DSC = true;
				dev_dbg(dev, "nVidia Card!\n");
			}

			pic_height = (dp_pps_info.pps_db[6] << 8) |
				     (dp_pps_info.pps_db[7]);
			pic_width = (dp_pps_info.pps_db[8] << 8) |
				    (dp_pps_info.pps_db[9]);
			slice_height = (dp_pps_info.pps_db[10] << 8) |
				       (dp_pps_info.pps_db[11]);
			slice_width = (dp_pps_info.pps_db[12] << 8) |
				      (dp_pps_info.pps_db[13]);
			chunk_size = (dp_pps_info.pps_db[14] << 8) |
				     (dp_pps_info.pps_db[15]);
			bpc = dp_pps_info.pps_db[3] >> 4;
			bpp = ((dp_pps_info.pps_db[4] & 0x03) << 8) |
			      (dp_pps_info.pps_db[5]);

			slice_per_line = pic_width / slice_width;
			cycles_per_slice = (chunk_size % 6 == 0) ?
					   chunk_size / 6 :
					   (chunk_size / 6 + 1);
			cycles_per_line = slice_per_line * cycles_per_slice;
			slcr_in_hsize = cycles_per_line * 2;
			slcr_in_vsize = pic_height;

			dev_dbg(dev, "bpc=%d, bpp=%d\n", bpc, bpp/16);
			dev_dbg(dev, "pic_width=%d, pic_height=%d\n",
				pic_width, pic_height);

			dev_dbg(dev, "slice_width=%d,	slice_height=%d\n",
				slice_width, slice_height);

			dev_dbg(dev, "slice_per_line=%d, cycles_per_slice=%d\n",
				slice_per_line, cycles_per_slice);

			dev_dbg(dev, "slcr_in_hsize=%d, slcr_in_vsize=%d\n",
				slcr_in_hsize, slcr_in_vsize);
#ifdef CHECK_PARA /* Check with input parameter */
			if (dispsys->dsc_passthru) {
				u32 format;

				if (dispsys->encode_rate == 3) {
					if (((bpp / 16) == 12) ||
					    ((bpp / 16) == 15)) {
						dev_err(dev, "DSC ratio is not correct!\n");
						return -EINVAL;
					}
				} else if (dispsys->encode_rate == 2) {
					if (((bpp / 16) == 8) ||
					    ((bpp / 16) == 10)) {
						dev_err(dev, "DSC ratio is not correct!\n");
						return -EINVAL;
					}
				}
				format = dispsys->resolution.input_format;
				if (((bpc == 10) &&
				     (!mtk_dispsys_is_rgb10(format))) ||
				    ((bpc == 8) &&
				     (mtk_dispsys_is_rgb10(format)))) {
					dev_err(dev, "Color depth is not correct!\n");
					return -EINVAL;
				}
			}
#else /* Overwrite input parameter */
			if (dispsys->dsc_passthru) {
				if (((bpp / 16) == 12) || ((bpp / 16) == 15))
					dispsys->encode_rate = 2;
				else if (((bpp / 16) == 8) ||
					 ((bpp / 16) == 10))
					dispsys->encode_rate = 3;
				else
					dev_err(dev, "DP DSC ratio is not correct!\n");

				dispsys->resolution.input_format =
							(bpc == 10) ?
							DRM_FORMAT_ARGB2101010 :
							DRM_FORMAT_RGB888;
			}
#endif
		} else {
			if (dispsys->dsc_passthru) {
				dev_err(dev, "DP input is not DSC stream!\n");
				return -EINVAL;
			}
		}

#ifndef CHECK_PARA /* Overwrite input parameter */
		dispsys->resolution.framerate = dp_fps;
		dispsys->resolution.input_width =
					dp_video_info.vid_timing_msa.h_active;
		dispsys->resolution.input_height =
					dp_video_info.vid_timing_msa.v_active;

		disp_para.input_format = dispsys->resolution.input_format;
		disp_para.input_width = dispsys->resolution.input_width;
		disp_para.input_height = dispsys->resolution.input_height;
		disp_para.encode_rate = dispsys->encode_rate;
		disp_para.dsc_passthru = dispsys->dsc_passthru;
		disp_para.dsc_enable = dispsys->dsc_enable;

		mtk_dispsys_config_panel(dispsys, &disp_para);
#endif

		if (dispsys->resolution.input_height == 2040)
			mtk_dsi_set_lcm_vblanking(dsi_dev, 1, 151, 8);
		else
			mtk_dsi_set_lcm_vblanking(dsi_dev, 1, 79, 10);

		mtk_dsi_get_lcm_vtotal(dsi_dev, &lcm_vtotal);
		mtk_dsi_get_lcm_vfrontporch(dsi_dev, &lcm_vfp);

		if ((dp_fps != dispsys->resolution.framerate) ||
		    (dp_width != dispsys->resolution.input_width) ||
		    (dp_vtotal != lcm_vtotal)) {
			dev_err(dev, "Resolution or fps not match!\n");
			dev_err(dev, "input fps = %d, vtotal = %d, w = %d\n",
				dp_fps, dp_vtotal, dp_width);
			dev_err(dev, "target fps = %d, vtotal = %d, w = %d\n",
				dispsys->resolution.framerate, lcm_vtotal,
				dispsys->resolution.input_width);
			return -EINVAL;
		}

		hlen = dp_fps * dp_vtotal;
		step1 = dp_fps / 2;	/* 0.5 line/frame */
		step2 = dp_fps * 3;	/* 3 line/frame */

		lock_win = 0;
		turbo_win = 6;

		target_line = lcm_vtotal - lcm_vfp +
			      dp_video_info.vid_timing_msa.v_front_porch - 2;

		mtk_dispsys_set_ddds_frmtrk_param(
					dev, hlen, step1, step2,
					target_line, lock_win, turbo_win, 0);

		mtk_dispsys_enable(dev, 1, 0);
	} else {
		/* disable display */
		mtk_dispsys_disable(dev);
	}

	return 0;
}

static int dp_callback(struct device *dev, enum DPRX_NOTIFY_T event)
{
	struct mtk_dispsys *dispsys = NULL;
	struct device *dp_dev = NULL;
	struct device *frm_dev = NULL;

	if (!dev)
		return -EFAULT;

	dispsys = dev_get_drvdata(dev);
	dp_dev = dispsys->disp_dev[MTK_DISP_DPRX];
	frm_dev = dispsys->disp_dev[MTK_DISP_FRMTRK];

	if (event == DPRX_RX_MSA_CHANGE) {
		dev_info(dp_dev, "[DPRX]DP_RX_TIMING_LOCK\n");
	} else if (event == DPRX_RX_PPS_CHANGE) {
		dev_info(dp_dev, "[DPRX]DPRX_RX_PPS_CHANGE\n");
	} else if (event == DPRX_RX_VIDEO_STABLE) {
		dev_info(dp_dev, "[DPRX]DPRX_RX_VIDEO_STABLE\n");
		if (dispsys->scenario_callback) {
			mtk_frmtrk_set_mask(frm_dev, 1, 0, NULL);
			dispsys->scenario_callback(dispsys->scenario_dev,
						   DPRX_RX_VIDEO_STABLE);
		}
	} else if (event == DPRX_RX_VIDEO_NOT_STABLE) {
		dev_info(dp_dev, "[DPRX]DPRX_RX_VIDEO_NOT_STABLE\n");
	} else if (event == DPRX_RX_PLUGIN) {
		dev_info(dp_dev, "[DPRX]DPRX_RX_PLUGIN\n");
	} else if (event == DPRX_RX_UNPLUG) {
		dev_info(dp_dev, "[DPRX]DPRX_RX_UNPLUG\n");
		if (dispsys->scenario_callback)
			dispsys->scenario_callback(dispsys->scenario_dev,
						   DPRX_RX_UNPLUG);
	}

	return 0;
}

u8 edid_4000[] = { /* 4000x2040 */
	0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
	0x36, 0x8B, 0x30, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x07, 0x1A, 0x01, 0x04, 0xB5, 0x3C, 0x22, 0x78,
	0x3A, 0x72, 0x25, 0xAC, 0x50, 0x33, 0xB7, 0x26,
	0x0B, 0x50, 0x54, 0x21, 0x08, 0x00, 0x81, 0x00,
	0xB3, 0x00, 0xD1, 0x00, 0xA9, 0x40, 0x81, 0x80,
	0xD1, 0xC0, 0x01, 0x01, 0x01, 0x01, 0xCB, 0xE7,
	0xA0, 0xF4, 0xF1, 0xF8, 0xA0, 0x70, 0x58, 0x2C,
	0x8A, 0x00, 0x06, 0x44, 0x21, 0x00, 0x00, 0x1E,
	0x00, 0x00, 0x00, 0xFF, 0x00, 0x4D, 0x54, 0x4B,
	0x34, 0x30, 0x30, 0x30, 0x35, 0x39, 0x70, 0x39,
	0x34, 0x0A, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x4D,
	0x54, 0x4B, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xFD,
	0x00, 0x1D, 0x4B, 0x1F, 0xB4, 0x3C, 0x01, 0x0A,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x02, 0xFD,
	0x02, 0x03, 0x28, 0x72, 0x5B, 0x05, 0x04, 0x03,
	0x01, 0x12, 0x13, 0x14, 0x16, 0x07, 0x90, 0x9F,
	0x20, 0x22, 0x5D, 0x5F, 0x60, 0x61, 0x62, 0x64,
	0x65, 0x66, 0x5E, 0x63, 0x02, 0x06, 0x11, 0x15,
	0x23, 0x09, 0x7F, 0x07, 0x83, 0x01, 0x00, 0x00,
	0x02, 0x3A, 0x80, 0x18, 0x71, 0x38, 0x2D, 0x40,
	0x58, 0x2C, 0x45, 0x00, 0xC4, 0x8E, 0x21, 0x00,
	0x00, 0x1E, 0x8C, 0x0A, 0xA0, 0x14, 0x51, 0xF0,
	0x16, 0x00, 0x26, 0x7C, 0x43, 0x00, 0xC4, 0x8E,
	0x21, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA4,
	0x70, 0x12, 0x79, 0x00, 0x00, 0x03, 0x01, 0x14,
	0xB3, 0x5B, 0x01, 0x88, 0x9F, 0x0F, 0xF3, 0x01,
	0x57, 0x80, 0x2B, 0x00, 0xF7, 0x07, 0x9F, 0x00,
	0x07, 0x80, 0x09, 0x00, 0x07, 0x00, 0x0A, 0x08,
	0x81, 0x00, 0x08, 0x04, 0x00, 0x04, 0x02, 0x10,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x39, 0x90,
};

u8 edid_3840[] = { /* 3840x2160 */
	0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
	0x36, 0x8B, 0x30, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x07, 0x1A, 0x01, 0x04, 0xB5, 0x3C, 0x22, 0x78,
	0x3A, 0x72, 0x25, 0xAC, 0x50, 0x33, 0xB7, 0x26,
	0x0B, 0x50, 0x54, 0x21, 0x08, 0x00, 0x81, 0x00,
	0xB3, 0x00, 0xD1, 0x00, 0xA9, 0x40, 0x81, 0x80,
	0xD1, 0xC0, 0x01, 0x01, 0x01, 0x01, 0xCC, 0xE7,
	0x00, 0x30, 0xF2, 0x70, 0x5A, 0x80, 0xB0, 0x58,
	0x8A, 0x00, 0x06, 0x44, 0x21, 0x00, 0x00, 0x1E,
	0x00, 0x00, 0x00, 0xFF, 0x00, 0x4D, 0x54, 0x4B,
	0x33, 0x38, 0x34, 0x30, 0x35, 0x39, 0x70, 0x39,
	0x34, 0x0A, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x4D,
	0x54, 0x4B, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xFD,
	0x00, 0x1D, 0x4B, 0x1F, 0xB4, 0x3C, 0x01, 0x0A,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x02, 0x8E,
	0x02, 0x03, 0x28, 0x72, 0x5B, 0x05, 0x04, 0x03,
	0x01, 0x12, 0x13, 0x14, 0x16, 0x07, 0x90, 0x9F,
	0x20, 0x22, 0x5D, 0x5F, 0x60, 0x61, 0x62, 0x64,
	0x65, 0x66, 0x5E, 0x63, 0x02, 0x06, 0x11, 0x15,
	0x23, 0x09, 0x7F, 0x07, 0x83, 0x01, 0x00, 0x00,
	0x02, 0x3A, 0x80, 0x18, 0x71, 0x38, 0x2D, 0x40,
	0x58, 0x2C, 0x45, 0x00, 0xC4, 0x8E, 0x21, 0x00,
	0x00, 0x1E, 0x8C, 0x0A, 0xA0, 0x14, 0x51, 0xF0,
	0x16, 0x00, 0x26, 0x7C, 0x43, 0x00, 0xC4, 0x8E,
	0x21, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA4,
	0x70, 0x12, 0x79, 0x00, 0x00, 0x03, 0x01, 0x14,
	0xB3, 0x5B, 0x01, 0x88, 0xFF, 0x0E, 0x2F, 0x02,
	0xAF, 0x80, 0x57, 0x00, 0x6F, 0x08, 0x59, 0x00,
	0x07, 0x80, 0x09, 0x00, 0x07, 0x00, 0x0A, 0x08,
	0x81, 0x00, 0x08, 0x04, 0x00, 0x04, 0x02, 0x10,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE6, 0x90,
};

u8 edid_1080[] = { /* 1080p */
	0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
	0x36, 0x8B, 0x30, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x07, 0x1A, 0x01, 0x04, 0xB5, 0x3C, 0x22, 0x78,
	0x3A, 0x72, 0x25, 0xAC, 0x50, 0x33, 0xB7, 0x26,
	0x0B, 0x50, 0x54, 0x21, 0x08, 0x00, 0x81, 0x00,
	0xB3, 0x00, 0xD1, 0x00, 0xA9, 0x40, 0x81, 0x80,
	0xD1, 0xC0, 0x01, 0x01, 0x01, 0x01, 0x08, 0xE8,
	0x00, 0x30, 0xF2, 0x70, 0x5A, 0x80, 0xB0, 0x58,
	0x8A, 0x00, 0x06, 0x44, 0x21, 0x00, 0x00, 0x1E,
	0x00, 0x00, 0x00, 0xFF, 0x00, 0x4D, 0x54, 0x4B,
	0x31, 0x30, 0x38, 0x30, 0x70, 0x0A, 0x20, 0x20,
	0x20, 0x20, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x4D,
	0x54, 0x4B, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xFD,
	0x00, 0x1D, 0x4B, 0x1F, 0xB4, 0x3C, 0x01, 0x0A,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x02, 0xB2,
	0x02, 0x03, 0x28, 0x70, 0x5B, 0x05, 0x04, 0x03,
	0x01, 0x12, 0x13, 0x14, 0x16, 0x07, 0x10, 0x1F,
	0x20, 0x22, 0x5D, 0x5F, 0x60, 0x61, 0x62, 0x64,
	0x65, 0x66, 0x5E, 0x63, 0x02, 0x06, 0x11, 0x15,
	0x23, 0x09, 0x7F, 0x07, 0x83, 0x01, 0x00, 0x00,
	0x30, 0x2A, 0x00, 0x98, 0x51, 0x00, 0x2A, 0x40,
	0x30, 0x70, 0x13, 0x00, 0xC4, 0x8E, 0x21, 0x00,
	0x00, 0x1E, 0x8C, 0x0A, 0xA0, 0x14, 0x51, 0xF0,
	0x16, 0x00, 0x26, 0x7C, 0x43, 0x00, 0xC4, 0x8E,
	0x21, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF9,
	0x70, 0x12, 0x79, 0x00, 0x00, 0x03, 0x01, 0x50,
	0xF3, 0x39, 0x00, 0x84, 0x7F, 0x07, 0x17, 0x01,
	0x57, 0x80, 0x2B, 0x00, 0x37, 0x04, 0x2C, 0x00,
	0x03, 0x80, 0x04, 0x00, 0x02, 0x3A, 0x00, 0x04,
	0x7F, 0x07, 0xCF, 0x02, 0x0F, 0x82, 0x2B, 0x00,
	0x37, 0x04, 0x2C, 0x00, 0x03, 0x80, 0x04, 0x00,
	0xFA, 0x1C, 0x00, 0x04, 0x7F, 0x07, 0x3D, 0x03,
	0x7D, 0x82, 0x2B, 0x00, 0x37, 0x04, 0x2C, 0x00,
	0x03, 0x80, 0x04, 0x00, 0xE6, 0x73, 0x00, 0x04,
	0x7F, 0x07, 0x17, 0x01, 0x57, 0x80, 0x2B, 0x00,
	0x37, 0x04, 0x2C, 0x00, 0x03, 0x80, 0x04, 0x00,
	0x07, 0x00, 0x0A, 0x08, 0x81, 0x00, 0x08, 0x04,
	0x00, 0x04, 0x02, 0x10, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x90,
};

u8 edid_4000_AMD[] = { /* 4000*2040@119.88fps */
	0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
	0x36, 0x8B, 0x12, 0x36, 0x01, 0x00, 0x00, 0x00,
	0x09, 0x1D, 0x01, 0x04, 0xB5, 0x3C, 0x22, 0x78,
	0x3A, 0x72, 0x25, 0xAC, 0x50, 0x33, 0xB7, 0x26,
	0x0B, 0x50, 0x54, 0x21, 0x08, 0x00, 0x81, 0x00,
	0xB3, 0x00, 0xD1, 0x00, 0xA9, 0x40, 0x81, 0x80,
	0xD1, 0xC0, 0x01, 0x01, 0x01, 0x01, 0xCA, 0xE7,
	0xA0, 0xF4, 0xF1, 0xF8, 0xA0, 0x70, 0x58, 0x2C,
	0x8A, 0x00, 0x06, 0x44, 0x21, 0x00, 0x00, 0x1E,
	0x00, 0x00, 0x00, 0xFF, 0x00, 0x4D, 0x54, 0x4B,
	0x34, 0x30, 0x30, 0x30, 0x35, 0x39, 0x70, 0x39,
	0x34, 0x0A, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x4D,
	0x54, 0x4B, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xFD,
	0x00, 0x1D, 0x4B, 0x1F, 0xB4, 0x3C, 0x01, 0x0A,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x02, 0xE1,
	0x02, 0x03, 0x28, 0x72, 0x5B, 0x05, 0x04, 0x03,
	0x01, 0x12, 0x13, 0x14, 0x16, 0x07, 0x90, 0x9F,
	0x20, 0x22, 0x5D, 0x5F, 0x60, 0x61, 0x62, 0x64,
	0x65, 0x66, 0x5E, 0x63, 0x02, 0x06, 0x11, 0x15,
	0x23, 0x09, 0x7F, 0x07, 0x83, 0x01, 0x00, 0x00,
	0x02, 0x3A, 0x80, 0x18, 0x71, 0x38, 0x2D, 0x40,
	0x58, 0x2C, 0x45, 0x00, 0xC4, 0x8E, 0x21, 0x00,
	0x00, 0x1E, 0x8C, 0x0A, 0xA0, 0x14, 0x51, 0xF0,
	0x16, 0x00, 0x26, 0x7C, 0x43, 0x00, 0xC4, 0x8E,
	0x21, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA4,
	0x70, 0x12, 0x79, 0x00, 0x00, 0x03, 0x01, 0x64,
	0x98, 0xCF, 0x01, 0x88, 0x9F, 0x0F, 0xF3, 0x01,
	0x57, 0x80, 0x2B, 0x00, 0xF7, 0x07, 0x9F, 0x00,
	0x07, 0x80, 0x09, 0x00, 0xB3, 0x5B, 0x01, 0x08,
	0x9F, 0x0F, 0xF3, 0x01, 0x57, 0x80, 0x2B, 0x00,
	0xF7, 0x07, 0x9F, 0x00, 0x07, 0x80, 0x09, 0x00,
	0xCD, 0xE7, 0x00, 0x08, 0xFF, 0x0E, 0x2F, 0x02,
	0xAF, 0x80, 0x57, 0x00, 0x6F, 0x08, 0x59, 0x00,
	0x07, 0x80, 0x09, 0x00, 0xB3, 0x5B, 0x01, 0x08,
	0xFF, 0x0E, 0x2F, 0x02, 0xAF, 0x80, 0x57, 0x00,
	0x6F, 0x08, 0x59, 0x00, 0x07, 0x80, 0x09, 0x00,
	0x99, 0xCF, 0x01, 0x08, 0xFF, 0x0E, 0x2F, 0x02,
	0xAF, 0x80, 0x57, 0x00, 0x6F, 0x08, 0x59, 0x00,
	0x07, 0x80, 0x09, 0x00, 0x07, 0x00, 0x0A, 0x08,
	0x81, 0x00, 0x08, 0x04, 0x00, 0x04, 0x02, 0x10,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x90,
};


int mtk_dispsys_dp_start(struct device *dev)
{
	struct mtk_dispsys *dispsys = NULL;
	struct device *dp_dev = NULL;
	int ret;

	if (!dev)
		return -EFAULT;

	dispsys = dev_get_drvdata(dev);
	dp_dev = dispsys->disp_dev[MTK_DISP_DPRX];

	ret = dprx_set_callback(dp_dev, dev, dp_callback);
	if (ret)
		return ret;
	ret = mtk_dprx_power_on_phy(dp_dev);
	if (ret)
		return ret;
	ret = mtk_dprx_power_on(dp_dev);
	if (ret)
		return ret;
	ret = mtk_dprx_drv_init(dp_dev, true);
	if (ret)
		return ret;

	dev_info(dev, "DP EDID Select = %d!\n", dispsys->edid_sel);
	if (dispsys->edid_sel == 1) { /* 4000x2040 1lane for DSC 1/3 */
		ret = dprx_drv_edid_init(edid_4000, sizeof(edid_4000));
		if (ret)
			return ret;
		ret = dprx_set_lane_count(1);
		if (ret)
			return ret;
	} else if (dispsys->edid_sel == 2) { /* 4000x2040 2lane */
		ret = dprx_drv_edid_init(edid_4000, sizeof(edid_4000));
		if (ret)
			return ret;
		ret = dprx_set_lane_count(2);
		if (ret)
			return ret;
	} else if (dispsys->edid_sel == 3) { /* 3840x2160 2lane */
		ret = dprx_drv_edid_init(edid_3840, sizeof(edid_3840));
		if (ret)
			return ret;
		ret = dprx_set_lane_count(2);
		if (ret)
			return ret;
	} else if (dispsys->edid_sel == 4) { /* 1080p 2lane */
		ret = dprx_drv_edid_init(edid_1080, sizeof(edid_1080));
		if (ret)
			return ret;
		ret = dprx_set_lane_count(2);
		if (ret)
			return ret;
	} else if (dispsys->edid_sel == 5) { /* for AMD Graphics Card 2lane */
		ret = dprx_drv_edid_init(edid_4000_AMD, sizeof(edid_4000_AMD));
		if (ret)
			return ret;
		ret = dprx_set_lane_count(2);
		if (ret)
			return ret;
	} else { /* 2lane */
		ret = dprx_set_lane_count(2);
		if (ret)
			return ret;
	}

	if (dispsys->dsc_passthru) {
		ret = dprx_set_dpcd_dsc_value(0x61, 0x11); /* DSC v1.1 */
		if (ret != 0x11) {
			dev_err(dev, "DP DSC V1.1 config error (%x)\n", ret);
			return -EFAULT;
		}
	} else {
		ret = dprx_set_dpcd_dsc_value(0x61, 0x21); /* DSC v1.2 */
		if (ret != 0x21) {
			dev_err(dev, "DP DSC V1.2 config error (%x)\n", ret);
			return -EFAULT;
		}
	}
	mtk_dprx_phy_gce_init(dp_dev);
	mtk_dprx_drv_start();

	return 0;
}
EXPORT_SYMBOL(mtk_dispsys_dp_start);

int mtk_dispsys_dp_stop(struct device *dev)
{
	struct mtk_dispsys *dispsys = NULL;
	struct device *dp_dev = NULL;
	int ret = 0;

	if (!dev)
		return -EFAULT;

	dispsys = dev_get_drvdata(dev);
	dp_dev = dispsys->disp_dev[MTK_DISP_DPRX];

	mtk_dprx_drv_stop();
	mtk_dprx_drv_deinit(dp_dev);
	mtk_dprx_phy_gce_deinit(dp_dev);
	mtk_dprx_power_off(dp_dev);
	mtk_dprx_power_off_phy(dp_dev);

	return ret;
}
EXPORT_SYMBOL(mtk_dispsys_dp_stop);

int mtk_dispsys_register_cb(struct device *dev, struct device *scenario_dev,
			    int (*callback)(struct device *dev,
					    enum DPRX_NOTIFY_T event))
{
	struct mtk_dispsys *dispsys = NULL;

	if (!dev)
		return -EFAULT;

	dispsys = dev_get_drvdata(dev);

	dispsys->scenario_callback = callback;
	dispsys->scenario_dev = scenario_dev;

	return 0;
}
EXPORT_SYMBOL(mtk_dispsys_register_cb);

int mtk_dispsys_vr_start(struct device *dev)
{
	int ret = 0;

	mtk_dispsys_register_cb(dev, dev, dispsys_vr_callback);
	ret = mtk_dispsys_dp_start(dev);

	return ret;
}
EXPORT_SYMBOL(mtk_dispsys_vr_start);

int mtk_dispsys_vr_stop(struct device *dev)
{
	int ret = 0;

	mtk_dispsys_disable(dev);
	ret = mtk_dispsys_dp_stop(dev);

	return ret;
}
EXPORT_SYMBOL(mtk_dispsys_vr_stop);
#endif

#define SOF_TIMEOUT 160000000
static int mtk_check_sof_diff(struct mtk_dispsys *dispsys, u32 src, u32 ref)
{
	struct mtk_mutex_res *mutex;
	struct cmdq_pkt *cmdq_pkt;
	struct mtk_mutex_timer_status time;
	u32 td_sof_id;
	u64 diff_time;
	int ret = 0;

	if (IS_ERR(dispsys))
		return -EINVAL;

	mutex = dispsys->mutex[MUTEX_DISP];
	if (IS_ERR(mutex)) {
		dev_err(dispsys->dev, "Get sof diff mutex NG!!\n");
		return -EIO;
	}

	td_sof_id = CMDQ_EVENT_MMSYS_CORE_TD_EVENT_8;

	cmdq_pkt_create(&cmdq_pkt);
	cmdq_pkt_clear_event(cmdq_pkt, td_sof_id);
	cmdq_pkt_wfe(cmdq_pkt, td_sof_id);

	mtk_mutex_select_timer_sof(mutex, src, ref, NULL);

	mtk_mutex_set_timer_us(mutex, SOF_TIMEOUT, SOF_TIMEOUT, NULL);

	mtk_mutex_timer_enable_ex(mutex, true, MUTEX_TO_EVENT_REF_EDGE, NULL);

	/* reuse unmute channel, run this should alwasy after unmute */
	if (cmdq_pkt_flush(dispsys->client[0], cmdq_pkt) == 0) {

		ret = mtk_mutex_timer_get_status(mutex, &time);

		if (ret) {
			dev_err(dispsys->dev, "Get sof diff status NG!!\n");
			goto RELEASE;
		} else {
			/* to pico seconds than to nano to prevent lost*/
			diff_time = ((u64)((u64)((1000000/320) *
				     (u64)time.ref_time)) / 1000000);

			dev_dbg(dispsys->dev, "diff time = %lld (us)\n",
				diff_time);
			dev_dbg(dispsys->dev, "ref_time = %d tick\n",
				time.ref_time);
		}
	}

RELEASE:
	cmdq_pkt_destroy(cmdq_pkt);
	mtk_mutex_timer_disable(mutex, NULL);
	return 0;
}

static void mtk_vr_mode_test(struct mtk_dispsys *dispsys, char *buf)
{
	struct device *ts_dev = dispsys->disp_dev[MTK_DISP_TIMESTAMP];
	struct mtk_disp_para para;
	u32 mode;
	int ret;

	memset(&para, 0, sizeof(para));

	if (strncmp(buf, "end", 3) == 0) {
		if (is_test_started == 0) {
			pr_err("%s no crtc enabled please call crtc enable\n",
			       __func__);
			return;
		}
		mtk_dispsys_disable(dispsys->dev);
		is_test_started = 0;
	} else if (strncmp(buf, "start:", 6) == 0) {
		ret = sscanf(buf, "start: %d\n", &mode);
		if (ret != 1) {
			pr_err("error to parse cmd %s\n", buf);
			return;
		}
		if (is_test_started == 1) {
			pr_info("test has been started; ");
			pr_info("Call end before starting a session\n");
			return;
		}

		if (mode == 5) {
			para.scenario = DISP_DRAM_MODE;
			para.input_format = DRM_FORMAT_ARGB2101010;
			para.dsc_enable = false;
			para.dsc_passthru = false;
			para.lhc_enable = true;
			para.dump_enable = false;
			para.fbdc_enable = false;
		} else if (mode == 4) {
			para.scenario = DISP_DRAM_MODE;
			para.input_format = DRM_FORMAT_RGBA1010102;
			para.dsc_enable = false;
			para.dsc_passthru = false;
			para.lhc_enable = true;
			para.dump_enable = false;
			para.fbdc_enable = false;
		} else if (mode == 3) {
			para.scenario = DISP_DRAM_MODE;
			para.input_format = DRM_FORMAT_AYUV2101010;
			para.dsc_enable = false;
			para.dsc_passthru = false;
			para.lhc_enable = true;
			para.dump_enable = false;
			para.fbdc_enable = false;
		} else if (mode == 2) {
			para.scenario = DUMP_ONLY_MODE;
			para.input_format = DRM_FORMAT_RGB888;
			para.input_height = 1920;
			para.input_width = 1080;
			para.dsc_enable = false;
			para.dsc_passthru = false;
			para.lhc_enable = false;
			para.dump_enable = false;
			para.fbdc_enable = false;
		} else if (mode == 1) {
			para.scenario = DISP_DRAM_MODE;
			para.input_format = DRM_FORMAT_RGB888;
			para.dsc_enable = false;
			para.dsc_passthru = false;
			para.lhc_enable = true;
			para.dump_enable = false;
			para.fbdc_enable = false;
		} else {
			para.scenario = DISP_DRAM_MODE;
			para.input_format = DRM_FORMAT_RGB888;
			para.dsc_enable = false;
			para.dsc_passthru = false;
			para.lhc_enable = false;
			para.dump_enable = false;
			para.fbdc_enable = false;
		}
		mtk_dispsys_config(dispsys->dev, &para);
		mtk_dispsys_enable(dispsys->dev, 0, 0);

		is_test_started = 1;
	} else if (strncmp(buf, "addr:", 5) == 0) {
		dma_addr_t addr;

		ret = sscanf(buf, "addr: %llx\n", &addr);
		if (ret != 1) {
			pr_err("error to parse cmd %s\n", buf);
			return;
		}

		mtk_dispsys_config_srcaddr(dispsys->dev, addr, NULL);
	} else if (strncmp(buf, "ts:", 3) == 0) {
		u32 dp_cnt, tmp;
		u64 stc, time, time1;

		/* delay(ms)=1000/current frame rate */
		ret = sscanf(buf, "ts:%d\n", &tmp);
		if (ret != 1) {
			pr_err("error to parse cmd %s\n", buf);
			return;
		}
		pr_info("delay(ms) = %d\n", tmp);

		mtk_ts_get_dsi_time(ts_dev, &stc, &dp_cnt);
		time = (stc >> 9) * 300 + (stc & 0x1ff);
		pr_info("stc_0 = 0x%llX, dp_cnt_0 = 0x%X, stc_time_0 = %lld\n",
			stc, dp_cnt, time);

		mdelay(tmp);

		mtk_ts_get_dsi_time(ts_dev, &stc, &dp_cnt);
		time1 = (stc >> 9) * 300 + (stc & 0x1ff);
		pr_info("stc_1 = 0x%llX, dp_cnt_1 = 0x%X, stc_time_1 = %lld\n",
			stc, dp_cnt, time1);

		pr_info("stc_diff = %lld\n", time1 - time);

		tmp = 27000000000 / (time1 - time);
		pr_info("frame rate = %d.%03d(fps)\n", tmp / 1000, tmp % 1000);
	} else if (strncmp(buf, "sof_diff", 8) == 0) {
		u32 src, ref;

		if (dispsys->dsc_passthru) {
			src = MUTEX_MMSYS_SOF_DP_DSC;
			ref = MUTEX_MMSYS_SOF_DP_DSC;
		} else {
			src = MUTEX_MMSYS_SOF_DP;
			ref = MUTEX_MMSYS_SOF_DP;
		}

		pr_debug("DP DIFF\n");
		if (mtk_check_sof_diff(dispsys, src, ref))
			pr_debug("Get DP Diff NG!!!!\n");

		src = MUTEX_MMSYS_SOF_DSI0;
		ref = MUTEX_MMSYS_SOF_DSI0;

		pr_debug("DSI DIFF\n");
		if (mtk_check_sof_diff(dispsys, src, ref))
			pr_debug("Get DSI Diff NG!!!!\n");

		if (dispsys->dsc_passthru)
			src = MUTEX_MMSYS_SOF_DP_DSC;
		else
			src = MUTEX_MMSYS_SOF_DP;
		ref = MUTEX_MMSYS_SOF_DSI0;

		pr_debug("DP/DSI DIFF\n");
		if (mtk_check_sof_diff(dispsys, src, ref))
			pr_debug("Get DP/DSI Diff NG!!!!\n");
	} else if (strncmp(buf, "dsi_fps", 7) == 0) {
		u32 src, ref;

		src = MUTEX_MMSYS_SOF_DSI0;
		ref = MUTEX_MMSYS_SOF_DSI0;

		pr_debug("DSI DIFF\n");
		if (mtk_check_sof_diff(dispsys, src, ref))
			pr_debug("Get DSI Diff NG!!!!\n");
	} else {
		pr_info("not supported command: %s", buf);
	}
}

static int mtk_dispsys_debugfs_open(struct inode *inode,
						struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t mtk_dispsys_debugfs_write(struct file *file,
			const char __user *ubuf, size_t count,
			loff_t *ppos)
{
	char buf[96];
	struct mtk_dispsys *dispsys;
	size_t buf_size = min(count, sizeof(buf) - 1);

	if (copy_from_user(buf, ubuf, buf_size))
		return -EFAULT;

	buf[buf_size] = '\0';
	dispsys = (struct mtk_dispsys *)file->private_data;

	if (strncmp(buf, "vr:", 3) == 0)
		mtk_vr_mode_test(dispsys, &buf[3]);
	else if (strncmp(buf, "mmsys:", 6) == 0)
		mtk_mmsys_test(dispsys, &buf[6]);
	else if (strncmp(buf, "lhc:", 4) == 0)
		mtk_lhc_test(dispsys, &buf[4]);
	else if (strncmp(buf, "mutex:", 6) == 0)
		mtk_mutex_test(dispsys, &buf[6]);
	else
		pr_info("not support command:%s", buf);

	return count;
}

static const struct file_operations mtk_dispsys_debugfs_fops = {
	.open = mtk_dispsys_debugfs_open,
	.read = seq_read,
	.write = mtk_dispsys_debugfs_write,
};
#if defined(CONFIG_DEBUG_FS)
static int mtk_dispsys_debugfs_init(struct device *dev)
{
	struct mtk_dispsys *dispsys = dev_get_drvdata(dev);
	int ret = 0;

	dispsys->debugfs = debugfs_create_file(
					"mtk_dispsys",
					S_IFREG | S_IRUGO | S_IWUSR | S_IWGRP,
					NULL, (void *)dispsys,
					&mtk_dispsys_debugfs_fops);

	if (IS_ERR(dispsys->debugfs))
		return PTR_ERR(dispsys->debugfs);

	return ret;
}
#endif
static void mtk_dispsys_debugfs_deinit(struct device *dev)
{
	struct mtk_dispsys *dispsys = dev_get_drvdata(dev);

	debugfs_remove(dispsys->debugfs);
	dispsys->debugfs = NULL;
}

static int mtk_dispsys_buf_alloc(struct mtk_dispsys *dispsys,
				 struct mtk_dispsys_buf *buf)
{
	struct device *dev = dispsys->disp_dev[MTK_DISP_MDP_RDMA];

	buf->va_lcd = (void *)NULL;
	buf->pa_lcd = (dma_addr_t)NULL;

	init_dma_attrs(&buf->dma_attrs);
	dma_set_attr(DMA_ATTR_WRITE_COMBINE | DMA_ATTR_FORCE_CONTIGUOUS,
		     &buf->dma_attrs);

	buf->va_lcd = dma_alloc_attrs(dev, buf->size, &buf->pa_lcd,
				      GFP_KERNEL, &buf->dma_attrs);
	if (!buf->va_lcd) {
		dev_err(dev,
			"failed to allocate %zx byte dma buffer",
			buf->size);

		return -ENOMEM;
	}

	dev_dbg(dev,
		"dispsys buffer starts kvaddr = 0x%p dma_addr = 0x%p and size = 0x%lx\n",
		buf->va_lcd, (void *)buf->pa_lcd, buf->size);

	return 0;
}

int mtk_dispsys_buf_alloc_ext(struct device *dev, struct mtk_dispsys_buf *buf)
{
	struct mtk_dispsys *dispsys = NULL;

	if (!dev)
		return -ENODEV;

	dispsys = dev_get_drvdata(dev);

	return mtk_dispsys_buf_alloc(dispsys, buf);
}
EXPORT_SYMBOL(mtk_dispsys_buf_alloc_ext);

static void mtk_dispsys_buf_free(struct mtk_dispsys *dispsys,
				 struct mtk_dispsys_buf *buf)
{
	struct device *dev = dispsys->disp_dev[MTK_DISP_MDP_RDMA];

	dma_free_attrs(dev, buf->size, buf->va_lcd, buf->pa_lcd,
		       &buf->dma_attrs);
	buf->va_lcd = (void *)NULL;
	buf->pa_lcd = (dma_addr_t)NULL;
}

int mtk_dispsys_buf_free_ext(struct device *dev, struct mtk_dispsys_buf *buf)
{
	struct mtk_dispsys *dispsys = NULL;

	if (!dev)
		return -ENODEV;

	dispsys = dev_get_drvdata(dev);

	mtk_dispsys_buf_free(dispsys, buf);

	return 0;
}
EXPORT_SYMBOL(mtk_dispsys_buf_free_ext);

static void mtk_dispsys_deinit(struct mtk_dispsys *dispsys)
{
	int i;

	for (i = 0; i < MUTEX_NR; i++) {
		mtk_mutex_put(dispsys->mutex[i]);
		mtk_mutex_delay_put(dispsys->mutex_delay[i]);
	}
	mtk_dispsys_buf_free(dispsys, &dispsys->buf);

	mtk_dispsys_cmdq_mbox_destroy(dispsys);
}


static int mtk_dispsys_init(struct mtk_dispsys *dispsys)
{
	struct device *dev = dispsys->dev;
	struct device *mutex_dev = dispsys->disp_dev[MTK_DISP_MUTEX];
	int i, ret;

	for (i = 0; i < MUTEX_NR; i++) {
		/* display use 8 ~ 15 */
		dispsys->mutex[i] = mtk_mutex_get(mutex_dev, 8 + i);
		if (IS_ERR(dispsys->mutex[i])) {
			ret = PTR_ERR(dispsys->mutex[i]);
			dev_err(dev, "Failed to get mutex res[0]: %d\n", ret);
			goto err_mutex;
		}

		dispsys->mutex_delay[i] = mtk_mutex_delay_get(mutex_dev, i);
		if (IS_ERR(dispsys->mutex_delay)) {
			ret = PTR_ERR(dispsys->mutex_delay);
			dev_err(dispsys->dev,
				"Failed to get mutex delay res: %d\n", ret);
			goto err_mutex;
		}
	}

	dispsys->buf.size =
	  roundup(1080 * 2160 * dispsys->disp_partition_nr * 4 * 2, PAGE_SIZE);
	ret = mtk_dispsys_buf_alloc(dispsys, &dispsys->buf);
	if (ret < 0) {
		dev_err(dev, "Failed to allocate buffer: %d\n", ret);
		goto err_mutex;
	}

	dispsys->disp_buf[0] = dispsys->buf.pa_lcd;
	dispsys->disp_buf[1] = (dma_addr_t)NULL;
	dispsys->size = dispsys->buf.size;

	mtk_dispsys_cmdq_mbox_create(dispsys);

	return 0;

err_mutex:
	while (i--)
		mtk_mutex_put(dispsys->mutex[i]);

	return ret;
}

static int mtk_dispsys_search_dev(struct mtk_dispsys *dispsys)
{
	struct device *dev = dispsys->dev;
	struct device_node *node;
	struct platform_device *ctrl_pdev;
	int i;

	for (i = 0; i < MTK_DISP_MOD_NR; i++) {
		char *comp = &disp_mod_list[i].name[0];
		int index = disp_mod_list[i].index;

		node = of_parse_phandle(dev->of_node, comp, 0);
		if (!node) {
			dev_err(dev, "Failed to get %s node\n", comp);
			return -EINVAL;
		}

		ctrl_pdev = of_find_device_by_node(node);
		of_node_put(node);

		if (!ctrl_pdev) {
			dev_err(dev, "Failed to get %s ctrl_pdev\n", comp);
			return -EINVAL;
		}

		if (!dev_get_drvdata(&ctrl_pdev->dev)) {
			dev_err(dev, "Waiting for device %s\n", comp);
			return -EPROBE_DEFER;
		}
		dispsys->disp_dev[index] = &ctrl_pdev->dev;
	}

	return 0;
}

static int mtk_dispsys_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev = &pdev->dev;
	struct mtk_dispsys *dispsys;

	dev_dbg(dev, "%s %d", __func__, __LINE__);

	dispsys = devm_kzalloc(dev, sizeof(*dispsys), GFP_KERNEL);
	if (!dispsys)
		return -ENOMEM;

	dispsys->dev = dev;

	ret = mtk_dispsys_search_dev(dispsys);
	if (ret)
		return ret;

	of_property_read_u32(dev->of_node, "disp_partition_nr",
			     &dispsys->disp_partition_nr);
	of_property_read_u32(dev->of_node, "disp_core_per_wrap",
			     &dispsys->disp_core_per_wrap);

	platform_set_drvdata(pdev, dispsys);

	ret = mtk_dispsys_init(dispsys);
	if (ret)
		return ret;

#if defined(CONFIG_DEBUG_FS)
	ret = mtk_dispsys_debugfs_init(dev);
	if (ret)
		return ret;
#endif

	init_waitqueue_head(&dispsys->lhc_wq);

	curr_fps = 0;

	dev_dbg(dev, "Dispsys probe done.\n");
	return ret;
}

static int mtk_dispsys_remove(struct platform_device *pdev)
{
	struct mtk_dispsys *dispsys = platform_get_drvdata(pdev);

	mtk_dispsys_deinit(dispsys);
	mtk_dispsys_debugfs_deinit(&pdev->dev);
	/* pm_runtime_disable(&pdev->dev); */

	return 0;
}

static const struct of_device_id dispsys_driver_dt_match[] = {
	{.compatible = "mediatek,mt3612-display"},
	{},
};

MODULE_DEVICE_TABLE(of, dispsys_driver_dt_match);

struct platform_driver mtk_dispsys_driver = {
	.probe = mtk_dispsys_probe,
	.remove = mtk_dispsys_remove,
	.driver = {
		   .name = "mediatek-dispsys",
		   .owner = THIS_MODULE,
		   .of_match_table = dispsys_driver_dt_match,
		   },
};

module_platform_driver(mtk_dispsys_driver);
MODULE_LICENSE("GPL");
