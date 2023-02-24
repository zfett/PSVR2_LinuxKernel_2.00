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

#ifndef MTK_DISP_H
#define MTK_DISP_H

#include <soc/mediatek/mtk_dprx_info.h>
#include <soc/mediatek/mtk_dprx_if.h>
#include <soc/mediatek/mtk_lhc.h>

enum mtk_dispsys_scenario {
	DISP_DRAM_MODE,
	DISP_SRAM_MODE,
	VR_DP_VIDEO_MODE,
	DUMP_ONLY_MODE = 5,
	DISP_SCENARIO_NR
};

enum mtk_dispsys_mutex_list {
	MUTEX_DISP,
	MUTEX_DP,
	MUTEX_SBRC,
	MUTEX_NR
};

struct mtk_disp_para {
	enum mtk_dispsys_scenario scenario;

	u32 input_format;
	u32 output_format;
	u32 input_height;
	u32 input_width;
	u32 input_fps;
	u32 disp_fps;
	u32 encode_rate;
	u32 edid_sel;

	bool dp_enable;
	bool frmtrk;
	bool dsc_enable;
	bool dsc_passthru;
	bool lhc_enable;
	bool dump_enable;
	bool fbdc_enable;
};

struct mtk_dispsys_buf {
	void __iomem *va_lcd;
	dma_addr_t pa_lcd;
	struct dma_attrs dma_attrs;
	unsigned long size;
};

int mtk_dispsys_enable(struct device *dev, bool mute, int rdma_layout);
void mtk_dispsys_disable(struct device *dev);
int mtk_dispsys_mute(struct device *dev, bool enable, struct cmdq_pkt *handle);
int mtk_dispsys_lhc_slices_enable(struct device *dev, int lhc_input_slice_nr,
				  u32 slicer_out_idx,
				  struct mtk_lhc_slice (*config)[4]);
int mtk_dispsys_lhc_enable(struct device *dev, u32 width, u32 height);
int mtk_dispsys_lhc_disable(struct device *dev);
int mtk_dispsys_read_lhc(struct device *dev, struct mtk_lhc_hist *data);
int mtk_dispsys_config(struct device *dev, struct mtk_disp_para *para);
int mtk_dispsys_config_srcaddr(struct device *dev, dma_addr_t srcaddr,
			       struct cmdq_pkt **cmdq_handle);
int mtk_dispsys_config_pvric_2buf_srcaddr(struct device *dev,
					  dma_addr_t srcaddr1,
					  dma_addr_t srcaddr2,
					  struct cmdq_pkt **cmdq_handle);
struct device *mtk_dispsys_get_rdma(struct device *dev);
struct device *mtk_dispsys_get_lhc(struct device *dev);
struct device *mtk_dispsys_get_dsi(struct device *dev);
int mtk_dispsys_buf_alloc_ext(struct device *dev, struct mtk_dispsys_buf *buf);
int mtk_dispsys_buf_free_ext(struct device *dev, struct mtk_dispsys_buf *buf);
void *mtk_dispsys_get_dump_vaddr(struct device *dev);
struct cmdq_pkt **mtk_dispsys_cmdq_pkt_create(struct device *dev);
int mtk_dispsys_cmdq_pkt_flush(struct device *dev, struct cmdq_pkt **pkt);
int mtk_dispsys_cmdq_pkt_destroy(struct device *dev, struct cmdq_pkt **pkt);
struct mtk_mutex_res *mtk_dispsys_get_mutex_res(struct device *dev, int idx);
int mtk_dispsys_get_current_fps(void);
#ifdef CONFIG_MACH_MT3612_A0
int mtk_dispsys_set_ddds_frmtrk_param(
			struct device *dev,
			u32 hsync_len, u32 ddds_step1, u32 ddds_step2,
			u32 target_line, u32 lock_win, u32 turbo_win, u32 mask);
int mtk_dispsys_set_dp_video_info(struct device *dev,
				  struct dprx_video_info_s *video);
int mtk_dispsys_set_dp_pps_info(struct device *dev, struct pps_sdp_s *pps);
int mtk_dispsys_dp_start(struct device *dev);
int mtk_dispsys_dp_stop(struct device *dev);
int mtk_dispsys_register_cb(struct device *dev, struct device *scenario_dev,
			    int (*callback)(struct device *dev,
					    enum DPRX_NOTIFY_T event));
int mtk_dispsys_vr_start(struct device *dev);
int mtk_dispsys_vr_stop(struct device *dev);
#endif
#endif	/* MTK_DISP_H */
