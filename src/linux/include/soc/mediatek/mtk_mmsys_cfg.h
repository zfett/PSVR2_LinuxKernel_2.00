/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	Houlong Wei <houlong.wei@mediatek.com>
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
 * @file mtk_mmsys_cfg.h
 * Header of mtk_mmsys_cfg.c
 */

#ifndef MTK_MMSYS_CFG_H
#define MTK_MMSYS_CFG_H

#include <linux/soc/mediatek/mtk-cmdq.h>
#include <soc/mediatek/mtk_drv_def.h>

/** @ingroup IP_group_mmsys_cfg_external_enum
 * @brief Mmsys_cfg Camera Sync ID\n
 * value is from 0 to 4
 */
enum mtk_mmsys_camera_sync_id {
	MMSYSCFG_CAMERA_SYNC_SIDE01 = 0,
	MMSYSCFG_CAMERA_SYNC_SIDE23,
	MMSYSCFG_CAMERA_SYNC_GAZE0,
	MMSYSCFG_CAMERA_SYNC_GAZE1,
	MMSYSCFG_CAMERA_SYNC_GAZE_LED,
	MMSYSCFG_CAMERA_SYNC_MAX
};

/** @ingroup IP_group_mmsys_external_def
 * @brief Mmsys_cfg Camera Sync IRQ flags
 * @{
 */
#define MMSYSCFG_CAMERA_SYNC_IRQ_SIDE01		BIT(0)
#define MMSYSCFG_CAMERA_SYNC_IRQ_SIDE23		BIT(1)
#define MMSYSCFG_CAMERA_SYNC_IRQ_GAZE0		BIT(2)
#define MMSYSCFG_CAMERA_SYNC_IRQ_GAZE1		BIT(3)
#define MMSYSCFG_CAMERA_SYNC_IRQ_GAZE_LED	BIT(4)
#define MMSYSCFG_CAMERA_SYNC_IRQ_TAIL_SIDE01	BIT(5)
#define MMSYSCFG_CAMERA_SYNC_IRQ_TAIL_SIDE23	BIT(6)
#define MMSYSCFG_CAMERA_SYNC_IRQ_TAIL_GAZE0	BIT(7)
#define MMSYSCFG_CAMERA_SYNC_IRQ_TAIL_GAZE1	BIT(8)
#define MMSYSCFG_CAMERA_SYNC_IRQ_TAIL_GAZE_LED	BIT(9)
/*
 * @}
 */

/** @ingroup IP_group_mmsys_cfg_external_enum
 * @brief Mmsys_cfg Component ID\n
 * MMSYS_CG0, MMSYS_CG1, MMSYS_CG2 of MMSYS_CORE_CONFIG,
 *  and value is from 0 to 81
 */
enum mtk_mmsys_config_comp_id {
	/*! cg0_B0, multiple output muxer of module slicer 0 */
	MMSYSCFG_COMPONENT_SLCR_MOUT0 = 0,
	/*! cg0_B1, multiple output muxer of module slicer 1 */
	MMSYSCFG_COMPONENT_SLCR_MOUT1,
	/*! cg0_B2, multiple output muxer of module slicer 2 */
	MMSYSCFG_COMPONENT_SLCR_MOUT2,
	/*! cg0_B3, multiple output muxer of module slicer 3 */
	MMSYSCFG_COMPONENT_SLCR_MOUT3,
	/*! cg0_B4, module mdp_rdma 0 */
	MMSYSCFG_COMPONENT_MDP_RDMA0,
	/*! cg0_B5, module mdp_rdma 1 */
	MMSYSCFG_COMPONENT_MDP_RDMA1,
	/*! cg0_B6, module mdp_rdma 2 */
	MMSYSCFG_COMPONENT_MDP_RDMA2,
	/*! cg0_B7, module mdp_rdma 3 */
	MMSYSCFG_COMPONENT_MDP_RDMA3,
	/*! cg0_B8, module mdp_rdma_pvric 0 */
	MMSYSCFG_COMPONENT_MDP_RDMA_PVRIC0,
	/*! cg0_B9, module mdp_rdma_pvric 1 */
	MMSYSCFG_COMPONENT_MDP_RDMA_PVRIC1,
	/*! cg0_B10, module mdp_rdma_pvric 2 */
	MMSYSCFG_COMPONENT_MDP_RDMA_PVRIC2,
	/*! cg0_B11, module mdp_rdma_pvric 3 */
	MMSYSCFG_COMPONENT_MDP_RDMA_PVRIC3,
	/*! cg0_B12, module disp_rdma 0 */
	MMSYSCFG_COMPONENT_DISP_RDMA0,
	/*! cg0_B13, module disp_rdma 1 */
	MMSYSCFG_COMPONENT_DISP_RDMA1,
	/*! cg0_B14, module disp_rdma 2 */
	MMSYSCFG_COMPONENT_DISP_RDMA2,
	/*! cg0_B15, module disp_rdma 3 */
	MMSYSCFG_COMPONENT_DISP_RDMA3,
	/*! cg0_B16, module pattern generator 0 */
	MMSYSCFG_COMPONENT_DP_PAT_GEN0,
	/*! cg0_B17, module pattern generator 1 */
	MMSYSCFG_COMPONENT_DP_PAT_GEN1,
	/*! cg0_B18, module pattern generator 2 */
	MMSYSCFG_COMPONENT_DP_PAT_GEN2,
	/*! cg0_B19, module pattern generator 3 */
	MMSYSCFG_COMPONENT_DP_PAT_GEN3,
	/*! cg0_B20, module lhc 0 */
	MMSYSCFG_COMPONENT_LHC0,
	/*! cg0_B21, module lhc 1 */
	MMSYSCFG_COMPONENT_LHC1,
	/*! cg0_B22, module lhc 2 */
	MMSYSCFG_COMPONENT_LHC2,
	/*! cg0_B23, module lhc 3 */
	MMSYSCFG_COMPONENT_LHC3,
	/*! cg0_B24, module disp_wdma 0 */
	MMSYSCFG_COMPONENT_DISP_WDMA0,
	/*! cg0_B25, module disp_wdma 1 */
	MMSYSCFG_COMPONENT_DISP_WDMA1,
	/*! cg0_B26, module disp_wdma 2 */
	MMSYSCFG_COMPONENT_DISP_WDMA2,
	/*! cg0_B27, module disp_wdma 3 */
	MMSYSCFG_COMPONENT_DISP_WDMA3,
	/*! cg0_B28, multiple output muxer of module dsc 0 */
	MMSYSCFG_COMPONENT_DSC_MOUT0,
	/*! cg0_B29, multiple output muxer of module dsc 1 */
	MMSYSCFG_COMPONENT_DSC_MOUT1,
	/*! cg0_B30, multiple output muxer of module dsc 2 */
	MMSYSCFG_COMPONENT_DSC_MOUT2,
	/*! cg0_B31, multiple output muxer of module dsc 3 */
	MMSYSCFG_COMPONENT_DSC_MOUT3,
	/*! cg1_B0, module dsi 0 */
	MMSYSCFG_COMPONENT_DSI0 = 32,
	/*! cg1_B1, module dsi 1 */
	MMSYSCFG_COMPONENT_DSI1,
	/*! cg1_B2, module dsi 2 */
	MMSYSCFG_COMPONENT_DSI2,
	/*! cg1_B3, module dsi 3 */
	MMSYSCFG_COMPONENT_DSI3,
	/*! cg1_B8, module crop 0 */
	MMSYSCFG_COMPONENT_CROP0 = 40,
	/*! cg1_B9, module crop 1 */
	MMSYSCFG_COMPONENT_CROP1,
	/*! cg1_B10, module crop 2 */
	MMSYSCFG_COMPONENT_CROP2,
	/*! cg1_B11, module crop 3 */
	MMSYSCFG_COMPONENT_CROP3,
	/*! cg1_B12, multiple output muxer of module rdma 0 */
	MMSYSCFG_COMPONENT_RDMA_MOUT0,
	/*! cg1_B13, multiple output muxer of module rdma 1 */
	MMSYSCFG_COMPONENT_RDMA_MOUT1,
	/*! cg1_B14, multiple output muxer of module rdma 2 */
	MMSYSCFG_COMPONENT_RDMA_MOUT2,
	/*! cg1_B15, multiple output muxer of module rdma 3 */
	MMSYSCFG_COMPONENT_RDMA_MOUT3,
	/*! cg1_B16, module resizer 0 */
	MMSYSCFG_COMPONENT_RSZ0,
	/*! cg1_B17, module resizer 1 */
	MMSYSCFG_COMPONENT_RSZ1,
	/*! cg1_B18, multiple output muxer of module resizer 0 */
	MMSYSCFG_COMPONENT_RSZ_MOUT0,
	/*! cg1_B19, module p2s 0 */
	MMSYSCFG_COMPONENT_P2S0,
	/*! cg1_B20, module dsc 0 */
	MMSYSCFG_COMPONENT_DSC0,
	/*! cg1_B21, module dsc 1 */
	MMSYSCFG_COMPONENT_DSC1,
	/*! cg1_B22, dsi lane swap */
	MMSYSCFG_COMPONENT_DSI_LANE_SWAP,
	/*! cg1_B23, MM clock for slicer input */
	MMSYSCFG_COMPONENT_SLICER_MM,
	/*! cg1_B24, video clock for slicer input */
	MMSYSCFG_COMPONENT_SLICER_VID,
	/*! cg1_B25, dsc clock for slicer input */
	MMSYSCFG_COMPONENT_SLICER_DSC,
	/*! cg1_B26, lhc swap */
	MMSYSCFG_COMPONENT_LHC_SWAP,
	/*! cg2_B8, module rbfc 0 */
	MMSYSCFG_COMPONENT_RBFC0 = 72,
	/*! cg2_B9, module rbfc 1 */
	MMSYSCFG_COMPONENT_RBFC1,
	/*! cg2_B10, module rbfc 2 */
	MMSYSCFG_COMPONENT_RBFC2,
	/*! cg2_B11, module rbfc 3 */
	MMSYSCFG_COMPONENT_RBFC3,
	/*! cg2_B12, module sbrc */
	MMSYSCFG_COMPONENT_SBRC,
	/*! cg2_B13, module fake engine is used for debugging dram access */
	MMSYSCFG_COMPONENT_MM_FAKE_ENG,
	/*! cg2_B14, clock for event transport */
	MMSYSCFG_COMPONENT_EVENT_TX,
	/*! cg2_B15, clock for module camera sync */
	MMSYSCFG_COMPONENT_CAMERA_SYNC,
	/*! cg2_B16, clock for module mutex */
	MMSYSCFG_COMPONENT_MUTEX,
	MMSYSCFG_COMPONENT_ID_MAX
};

/** @ingroup IP_group_mmsys_cfg_external_enum
 * @brief Mmsys_cfg slicer to ddds selection\n
 * value is from 0 to 1.
 */
enum mtk_mmsys_slicer_to_ddds_sel {
	/*! select slcr_vid_vs(hs) to ddds */
	MMSYSCFG_SLCR_VID_TO_DDDS = 0,
	/*! select slcr_dsc_vs(hs) to ddds */
	MMSYSCFG_SLCR_DSC_TO_DDDS,
	MMSYSCFG_SLCR_TO_DDDS_MAX
};

/** @ingroup IP_group_mmsys_cfg_external_enum
 * @brief Mmsys_cfg pattern generator ID\n
 * value is from 0 to 3. MMSYSCFG_PAT_GEN_0 stands for the pattern generator
 * which is after RDMA partition 0, MMSYSCFG_PAT_GEN_1 stands for the one which
 * is after RDMA partition 1, and so on.
 */
enum mtk_mmsys_pat_gen_id {
	MMSYSCFG_PAT_GEN_0 = 0,
	MMSYSCFG_PAT_GEN_1,
	MMSYSCFG_PAT_GEN_2,
	MMSYSCFG_PAT_GEN_3,
	MMSYSCFG_PAT_GEN_ID_MAX
};

/** @ingroup IP_group_mmsys_cfg_external_enum
 * @brief Csc transform mode ID\n
 * value is from 0 to 2
 * mode BT601_TO_RGB is BT601 transform to RGB with internal matrix
 * mode BT709_TO_RGB is BT709 transform to RGB with internal matrix
 * mode SWAP_RB is R/B swap transform with external matrix
 */
enum mtk_csc_trans_mode {
	CSC_TRANS_MODE_BT601_TO_RGB = 0,
	CSC_TRANS_MODE_BT709_TO_RGB,
	CSC_TRANS_MODE_SWAP_RB,
	CSC_TRANS_MODE_MAX
};

/** @ingroup IP_group_mmsys_external_def
 * @brief Mmsys_cfg CRC enable flags
 * @{
 */
#define MMSYS_CRC_SLCR_VID_0_TO_CROP		BIT(0)
#define MMSYS_CRC_SLCR_VID_1_TO_CROP		BIT(1)
#define MMSYS_CRC_SLCR_VID_2_TO_CROP		BIT(2)
#define MMSYS_CRC_SLCR_VID_3_TO_CROP		BIT(3)
#define MMSYS_CRC_CROP_0_TO_RSZ			BIT(4)
#define MMSYS_CRC_CROP_1_TO_RSZ			BIT(5)
#define MMSYS_CRC_RSZ_0_TO_P2S_0		BIT(6)
#define MMSYS_CRC_RSZ_1_TO_P2S_0		BIT(7)
#define MMSYS_CRC_WDMA_IN_0			BIT(8)
#define MMSYS_CRC_WDMA_IN_1			BIT(9)
#define MMSYS_CRC_WDMA_IN_2			BIT(10)
#define MMSYS_CRC_WDMA_IN_3			BIT(11)
#define MMSYS_CRC_RDMA_IN_0			BIT(12)
#define MMSYS_CRC_RDMA_IN_1			BIT(13)
#define MMSYS_CRC_RDMA_IN_2			BIT(14)
#define MMSYS_CRC_RDMA_IN_3			BIT(15)
#define MMSYS_CRC_LHC_IN_0			BIT(16)
#define MMSYS_CRC_LHC_IN_1			BIT(17)
#define MMSYS_CRC_LHC_IN_2			BIT(18)
#define MMSYS_CRC_LHC_IN_3			BIT(19)
#define MMSYS_CRC_PAT_0_TO_DSC			BIT(20)
#define MMSYS_CRC_PAT_1_TO_DSC			BIT(21)
#define MMSYS_CRC_PAT_2_TO_DSC			BIT(22)
#define MMSYS_CRC_PAT_3_TO_DSC			BIT(23)
#define MMSYS_CRC_DSI_SWAP_0_TO_DIGITAL		BIT(24)
#define MMSYS_CRC_DSI_SWAP_1_TO_DIGITAL		BIT(25)
#define MMSYS_CRC_DSI_SWAP_2_TO_DIGITAL		BIT(26)
#define MMSYS_CRC_DSI_SWAP_3_TO_DIGITAL		BIT(27)
#define MMSYS_CRC_RDMA_0_TO_PAT			BIT(28)
#define MMSYS_CRC_RDMA_1_TO_PAT			BIT(29)
#define MMSYS_CRC_RDMA_2_TO_PAT			BIT(30)
#define MMSYS_CRC_RDMA_3_TO_PAT			BIT(31)
#define MMSYS_CRC_ALL				0xFFFFFFFFu
/*
 * @}
 */

/** @ingroup IP_group_mmsys_cfg_external_def
 * @brief Mmsys Color Format
 * @{
 */
#define MMSYS_FORMAT_RGB565		0
#define MMSYS_FORMAT_BGR565		1
#define MMSYS_FORMAT_RGB888		2
#define MMSYS_FORMAT_BGR888		3
#define MMSYS_FORMAT_RGBX8888		4
#define MMSYS_FORMAT_RGBA8888		5
#define MMSYS_FORMAT_BGRX8888		6
#define MMSYS_FORMAT_BGRA8888		7
#define MMSYS_FORMAT_XRGB8888		8
#define MMSYS_FORMAT_ARGB8888		9
#define MMSYS_FORMAT_XBGR8888		10
#define MMSYS_FORMAT_ABGR8888		11
#define MMSYS_FORMAT_YUYV		12
#define MMSYS_FORMAT_YVYU		13
#define MMSYS_FORMAT_UYVY		14
#define MMSYS_FORMAT_VYUY		15
#define MMSYS_FORMAT_ARGB4444		16
#define MMSYS_FORMAT_XRGB4444		17
#define MMSYS_FORMAT_NV12		18
#define MMSYS_FORMAT_Y_ONLY		19
#define MMSYS_FORMAT_Y_10BIT		20
#define MMSYS_FORMAT_RGB10		23
#define MMSYS_FORMAT_ARGB810		24
#define MMSYS_FORMAT_ARGB210		25
#define MMSYS_FORMAT_AYUV10		26
#define MMSYS_FORMAT_YUV		27
/**
 * @}
 */

/** @ingroup IP_group_mmsys_cfg_external_def
 * @brief Mmsys LCM reset flags
 * @{
 */
#define MMSYS_LCM0_RST			1
#define MMSYS_LCM1_RST			2
#define MMSYS_LCM2_RST			4
#define MMSYS_LCM3_RST			8
/**
 * @}
 */

int mtk_csc_reset(const struct device *mmsys_cfg_dev,
		struct cmdq_pkt **handle);
int mtk_csc_config(const struct device *mmsys_cfg_dev,
		struct cmdq_pkt **handle,
		bool trans_en,
		enum mtk_csc_trans_mode trans_mode);
int mtk_mmsys_cfg_power_on(const struct device *mmsys_cfg_dev);
int mtk_mmsys_cfg_power_off(const struct device *mmsys_cfg_dev);
int mtk_mmsys_cfg_reset_module(const struct device *mmsys_cfg_dev,
				struct cmdq_pkt **handle,
				enum mtk_mmsys_config_comp_id module);
int mtk_mmsys_cfg_reset_lcm(const struct device *mmsys_cfg_dev, bool reset,
			    u32 flags);
int mtk_mmsys_cfg_connect_comp(const struct device *mmsys_cfg_dev,
			       enum mtk_mmsys_config_comp_id cur,
			       enum mtk_mmsys_config_comp_id next,
			       struct cmdq_pkt **handle);
int mtk_mmsys_cfg_disconnect_comp(const struct device *mmsys_cfg_dev,
				  enum mtk_mmsys_config_comp_id cur,
				  enum mtk_mmsys_config_comp_id next,
				  struct cmdq_pkt **handle);
int mtk_mmsys_cfg_camera_sync_clock_sel(const struct device *mmsys_cfg_dev,
					bool use_26m);
int mtk_mmsys_cfg_camera_sync_dsi_control(const struct device *mmsys_cfg_dev,
					  bool dsi_vsync_high_acive);
int mtk_mmsys_cfg_camera_sync_config(const struct device *mmsys_cfg_dev,
				     enum mtk_mmsys_camera_sync_id vsgen_id,
				     u32 vsync_cycle, u32 delay_cycle,
				     bool vsync_low_acive,
				     struct cmdq_pkt **handle);
int mtk_mmsys_cfg_camera_sync_frc(const struct device *mmsys_cfg_dev,
				  enum mtk_mmsys_camera_sync_id vsgen_id,
				  unsigned char n, unsigned char m,
				  struct cmdq_pkt **handle);
int mtk_mmsys_cfg_camera_sync_register_cb(struct device *mmsys_cfg_dev,
					  mtk_mmsys_cb cb, u32 status,
					  void *priv_data);
int mtk_mmsys_cfg_camera_sync_enable(const struct device *mmsys_cfg_dev,
				     enum mtk_mmsys_camera_sync_id vsgen_id,
				     struct cmdq_pkt **handle);
int mtk_mmsys_cfg_camera_sync_disable(const struct device *mmsys_cfg_dev,
				      enum mtk_mmsys_camera_sync_id vsgen_id,
				      struct cmdq_pkt **handle);
int mtk_mmsys_cfg_dsi_lane_swap_config(const struct device *mmsys_cfg_dev,
				       u32 to_dsi0, u32 to_dsi1, u32 to_dsi2,
				       u32 to_dsi3, struct cmdq_pkt **handle);
int mtk_mmsys_cfg_lhc_swap_config(const struct device *mmsys_cfg_dev,
				  u32 to_lhc0, u32 to_lhc1, u32 to_lhc2,
				  u32 to_lhc3, struct cmdq_pkt **handle);
int mtk_mmsys_cfg_slicer_to_ddds_select(const struct device *mmsys_cfg_dev,
					enum mtk_mmsys_slicer_to_ddds_sel sel,
					struct cmdq_pkt **handle);
int mtk_mmsys_cfg_pat_config_frame_size(const struct device *mmsys_cfg_dev,
					enum mtk_mmsys_pat_gen_id id,
					u32 width, u32 height,
					struct cmdq_pkt **handle);
int mtk_mmsys_cfg_pat_config_border_size(const struct device *mmsys_cfg_dev,
					enum mtk_mmsys_pat_gen_id id,
					u32 width, u32 height,
					struct cmdq_pkt **handle);
int mtk_mmsys_cfg_pat_config_fg_color(const struct device *mmsys_cfg_dev,
				      enum mtk_mmsys_pat_gen_id id,
				      u32 color_y, u32 color_u, u32 color_v,
				      struct cmdq_pkt **handle);
int mtk_mmsys_cfg_pat_config_bg_color(const struct device *mmsys_cfg_dev,
				      enum mtk_mmsys_pat_gen_id id,
				      u32 color_y, u32 color_u, u32 color_v,
				      struct cmdq_pkt **handle);
int mtk_mmsys_cfg_pat_type_sel_pure_color(const struct device *mmsys_cfg_dev,
					  enum mtk_mmsys_pat_gen_id id,
					  struct cmdq_pkt **handle);
int mtk_mmsys_cfg_pat_type_sel_cursor(const struct device *mmsys_cfg_dev,
				      enum mtk_mmsys_pat_gen_id id,
				      bool show_cursor, u32 pos_x, u32 pos_y,
				      struct cmdq_pkt **handle);
int mtk_mmsys_cfg_pat_type_sel_ramp(const struct device *mmsys_cfg_dev,
				    enum mtk_mmsys_pat_gen_id id,
				    bool y_en, bool u_en, bool v_en,
				    u32 width, u32 step,
				    struct cmdq_pkt **handle);
int mtk_mmsys_cfg_pat_type_sel_grid(const struct device *mmsys_cfg_dev,
				    enum mtk_mmsys_pat_gen_id id,
				    u32 size, bool show_boundary,
				    bool show_grid,
				    struct cmdq_pkt **handle);
int mtk_mmsys_cfg_pat_gen_eanble(const struct device *mmsys_cfg_dev,
				 enum mtk_mmsys_pat_gen_id id,
				 bool force_input_valid,
				 struct cmdq_pkt **handle);
int mtk_mmsys_cfg_pat_gen_disable(const struct device *mmsys_cfg_dev,
				 enum mtk_mmsys_pat_gen_id id,
				 struct cmdq_pkt **handle);
int mtk_mmsys_cfg_crc_enable(const struct device *mmsys_cfg_dev, u32 flags);
int mtk_mmsys_cfg_crc_disable(const struct device *mmsys_cfg_dev, u32 flags);
int mtk_mmsys_cfg_crc_clear(const struct device *mmsys_cfg_dev, u32 flags);
int mtk_mmsys_cfg_crc_read(const struct device *mmsys_cfg_dev, u32 flags,
			  u32 *crc_out, u32 *crc_num, u32 mod_cnt);

#endif
