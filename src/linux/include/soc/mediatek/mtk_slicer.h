/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	Wilson Huang <wilson.huang@mediatek.com>
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
  * @file mtk_slicer.h
  * Header of mtk_slicer.c
  */

#ifndef MTK_MM_SLCR_H
#define MTK_MM_SLCR_H

#include <linux/soc/mediatek/mtk-cmdq.h>

#ifdef CONFIG_MTK_DEBUGFS
extern struct dentry *mtk_debugfs_root;
#endif

/** @ingroup IP_group_slicer_external_def
 * @brief Output lane selection
 */
#define SLICE_0_EN (1 << 0)
#define SLICE_1_EN (1 << 1)
#define SLICE_2_EN (1 << 2)
#define SLICE_3_EN (1 << 3)

/** @ingroup IP_group_slicer_external_enum
 * @brief Slicer input format, for configure in_format
 * value is from 0 to 2.
 */
enum slcr_in_format {
	/** DP Video */
	DP_VIDEO = 0,
	/** DP DSC */
	DP_DSC,
	/** DP Video & DSC */
	DP_VIDEO_DSC
};

/** @ingroup IP_group_slicer_external_enum
 * @brief Slicer clock mux select, for configure clock
 * value is from 0 to 3.
 */
enum slcr_clk_sel {
	/* CLK 26M */
	CLK26M = 0,
	/* dprx video clock, 600M */
	DPRX_VIDEO_CK,
	/* dprx dsc clock, 600M */
	DPRX_DSC_CK,
	/* dprx video & dsc clock, 600M */
	DPRX_VIDEO_DSC_CK,
	/* univ pll d2, 624M */
	UNIVPLL_D2
};

/** @ingroup IP_group_slicer_external_enum
 * @brief Slicer dither type, for configure dither_type
 * value is from 0 to 4.
 */
enum slcr_dither_type {
	/** E-Dither */
	E_DITHER = 0,
	/** R-Dither */
	R_DITHER,
	/** Round-Dither */
	ROUND_DITHER,
	/** LFSR Dither */
	LFSR_DITHER
};

/** @ingroup IP_group_slicer_external_enum
 * @brief Slicer dither mode, for configure dither_mode
 * value is from 0 to 3.
 */
enum slcr_dither_mode {
	/** No action */
	NO_DITHER = 0,
	/** 12bits to 10bits */
	DITHER_BIT12_TO_10,
	/** 12bits to 8bits */
	DITHER_BIT12_TO_8,
	/** 12bits to 6bits */
	DITHER_BIT12_TO_6
};

/** @ingroup IP_group_slicer_external_enum
 * @brief Slicer color space conversion matrix.
 * value is from 0 to 0xe.
 */
enum slcr_csc_mode {
	/** Full-range sRGB to full-range BT.601 */
	MTX_FULLSRGB_TO_FULL601 = 0,
	/** Reserved */
	MTX_CSC_RESERVED_0,
	/** Full-range sRGB to limit-range BT.601 */
	MTX_FULLSRGB_TO_LIMIT601,
	/** Full-range sRGB to limit-range BT.709 */
	MTX_FULLSRGB_TO_LIMIT709,
	/** Full-range BT.601 to full-range sRGB */
	MTX_FULL601_TO_FULLSRGB,
	/** Reserved */
	MTX_CSC_RESERVED_1,
	/** Limit-range BT.601 to full-range sRGB */
	MTX_LIMIT601_TO_FULLSRGB,
	/** Limit-range BT.709 to full-range sRGB */
	MTX_LIMIT709_TO_FULLSRGB,
	/** Full-range BT.601 to limit-range BT.601 */
	MTX_FULL601_TO_LIMIT601,
	/** Full-range BT.601 to limit-range BT.709 */
	MTX_FULL601_TO_LIMIT709,
	/** Limit-range BT.601 to full-range BT.601 */
	MTX_LIMIT601_TO_FULL601,
	/** Limit-range BT.709 to full-range BT.601 */
	MTX_LIMIT709_TO_FULL601,
	/** Limit-range BT.709 to limit-range BT.601 */
	MTX_LIMIT709_TO_LIMIT601,
	/** Limit-range BT.601 to limit-range BT.709 */
	MTX_LIMIT601_TO_LIMIT709,
	/** Max. */
	MTX_CSC_MAX
};

/** @ingroup IP_group_slicer_external_enum
 * @brief Slicer output endian.
 * value is from 0 to 1.
 */
enum slcr_dsc_endian {
	/** Big endian */
	BIG_END = 0,
	/** Little endian */
	LITTLE_END
};

/** @ingroup IP_group_slicer_external_enum
 * @brief Slicer sync port selection.
 * value is from 0 to 1.
 */
enum slcr_sync_port {
	/** From original port */
	ORG_PORT = 0,
	/** From timing-regen port */
	TIMING_REGEN_PORT
};

/** @ingroup IP_group_slicer_external_enum
 * @brief Slicer 3D control.
 * value is from 0 to 1.
 */
enum slcr_rs_mode {
	/** Hardware mode */
	HARDWARE_MODE = 0,
	/** Software mode */
	SOFTWARE_MODE
};

/** @ingroup IP_group_slicer_external_enum
 * @brief Slicer pixel swap control.
 * value is from 0 to 1.
 */
enum slcr_2cs_cfg {
	/** No swap */
	NO_SWAP = 0,
	/** Swap */
	SWAP
};

/** @ingroup IP_group_slicer_external_enum
 * @brief Slicer channel swap control.
 * value is from 0 to 3.
 */
enum slcr_3cs_cfg {
	/** Channel 0 */
	CHANNEL_0 = 0,
	/** Channel 1 */
	CHANNEL_1,
	/** Channel 2 */
	CHANNEL_2,
	/** No output */
	NO_OUTPUT
};

/** @ingroup IP_group_slicer_external_enum
 * @brief Slicer GCE event.
 * value is from 0 to 5.
 */
enum slcr_gce_event {
	/** No GCE */
	NO_GCE = 0,
	/** Video/DSC input (blank + active) */
	GCE_INPUT,
	/** Video/DSC output lane 0 (active) */
	GCE_OUTPUT_0,
	/** Video/DSC output lane 1 (active) */
	GCE_OUTPUT_1,
	/** Video/DSC output lane 2 (active) */
	GCE_OUTPUT_2,
	/** Video/DSC output lane 3 (active) */
	GCE_OUTPUT_3
};

/** @ingroup IP_group_slicer_external_enum
 * @brief Slicer pattern generator mode.
 * value is from 0 to 3.
 */
enum slcr_pg_mode {
	/** Free run mode */
	FREE_RUN = 0,
	/** H/W trigger mode */
	HW_TRIG,
	/** S/W trigger mode */
	SW_TRIG,
	/** Input timing mode */
	INPUT_TIMING
};

/** @ingroup IP_group_slicer_external_enum
 * @brief Slicer pattern generator timing.
 * value is from 0 to 7.
 */
enum slcr_pg_timing {
	/** External */
	PAT_EXT = 0,
	/** 3840x2160 */
	PAT_3840_2160,
	/** 2560x1440 */
	PAT_2560_1440,
	/** 1920x1080 */
	PAT_1920_1080,
	/** 1280x720 */
	PAT_1280_720,
	/** 720x576 */
	PAT_720_576,
	/** 720x480 */
	PAT_720_480,
	/** 640x480 */
	PAT_640_480
};

/** @ingroup IP_group_slicer_external_enum
 * @brief Slicer pattern generator frame rate.
 * value is from 0 to 3.
 */
enum slcr_pg_fps {
	/* 23.976 fps */
	PAT_23_976_FPS = 0,
	/* 50 fps */
	PAT_50_FPS,
	/* 59.94 fps */
	PAT_59_94_FPS,
	/* 60 fps */
	PAT_60_FPS,
	/* 89.91 fps */
	PAT_89_91_FPS,
	/* 90 fps */
	PAT_90_FPS,
	/* 119.88 fps */
	PAT_119_88_FPS,
	/* 120 fps */
	PAT_120_FPS
};

/** @ingroup IP_group_slicer_external_struct
 * @brief Slicer dummy line data structure.
 */
struct dummy_line {
	/** Dummy line numbers, range from 0 to 8191 */
	u16 num;
	/** Dummy line data r, range from 0 to 4095 */
	u16 r_value;
	/** Dummy line  data b, range from 0 to 4095 */
	u16 g_value;
	/** Dummy line  data g, range from 0 to 4095 */
	u16 b_value;
};

/** @ingroup IP_group_slicer_external_struct
 * @brief Slicer DP video input data structure.
 */
struct dp_video_input {
	/** Input frame width, range from 0 to 8191 */
	u16 width;
	/** Input frame height, range from 0 to 8191 */
	u16 height;
	/** Input frame HSYNC polarity inversion, 0: no invert, 1: invert */
	bool in_hsync_inv;
	/** Input frame VSYNC polarity inversion, 0: no invert, 1: invert */
	bool in_vsync_inv;
	/** Input pixel swap control, pxl_swap is #slcr_2cs_cfg */
	enum slcr_2cs_cfg pxl_swap;
	/** Input channel 0~2 input selection, in_ch is #slcr_3cs_cfg */
	enum slcr_3cs_cfg in_ch[3];
};

/** @ingroup IP_group_slicer_external_struct
 * @brief Slicer DP DSC input data structure.
 */
struct dp_dsc_input {
	/** Input frame width, range from 0 to 8191 */
	u16 width;
	/** Input frame height, range from 0 to 8191 */
	u16 height;
	/** Input frame HSYNC polarity inversion, 0: no invert, 1: invert */
	bool in_hsync_inv;
	/** Input frame VSYNC polarity inversion, 0: no invert, 1: invert */
	bool in_vsync_inv;
	/** Bit rate, range from 0 to 63 */
	u8 bit_rate;
	/** Chunk numbers, range from 0 to 7 */
	u8 chunk_num;
	/** Input pixel swap control, pxl_swap is #slcr_2cs_cfg */
	enum slcr_2cs_cfg pxl_swap;
};

/** @ingroup IP_group_slicer_external_struct
 * @brief Slicer DP video output data structure.
 */
struct dp_video_output {
	/** Output frame HSYNC polarity inversion, 0: no invert, 1: invert */
	bool out_hsync_inv;
	/** Output frame VSYNC polarity inversion, 0: no invert, 1: invert */
	bool out_vsync_inv;
	/** Output slice 0~3 SOP, range from 0 to 8191 */
	u16 slice_sop[4];
	/** Output slice 0~3 EOP, range from 0 to 8191 */
	u16 slice_eop[4];
	/** Video lane output selection, range from 0 to 15 */
	u8 vid_en[4];
	/**  Struct for Dummy line configuration */
	struct dummy_line dyml[4];
	/** Output channel 0~2 input selection, out_ch is #slcr_3cs_cfg */
	enum slcr_3cs_cfg out_ch[3];
	/** Sync port selection, sync_port is #slcr_sync_port */
	enum slcr_sync_port sync_port;
};

/** @ingroup IP_group_slicer_external_struct
 * @brief Slicer 3D control data structure.
 */
struct slcr_rs_cfg {
	/** 3D Timing Re-shaped enable, 0: disable, 1: enable */
	bool rs_en;
	/** 3D Timing Re-shaped mode, rs_mode is #slcr_rs_mode */
	enum slcr_rs_mode rs_mode;
	/** Vertical front porch, range from 0 to 8191 */
	u16 v_front;
	/** Length of VSYNC, range from 0 to 8191 */
	u16 vsync;
	/** Vertical back porch, range from 0 to 8191 */
	u16 v_back;
	/** Vertical active, range from 0 to 8191 */
	u16 v_active;
};

/** @ingroup IP_group_slicer_external_struct
 * @brief Slicer DP DSC output data structure.
 */
struct dp_dsc_output {
	/** Output frame HSYNC polarity inversion, 0: no invert, 1: invert */
	bool out_hsync_inv;
	/** Output frame VSYNC polarity inversion, 0: no invert, 1: invert */
	bool out_vsync_inv;
	/** Output slice 0~3 SOP, range from 0 to 8191 */
	u16 slice_sop[4];
	/** Output slice 0~3 EOP, range from 0 to 8191 */
	u16 slice_eop[4];
	/** DSC lane output selection, range from 0 to 15 */
	u8 dsc_en[4];
	/** Valid bytes, range from 0 to 63 */
	u8 valid_byte;
	/** Output endian, endian is #slcr_dsc_endian */
	enum slcr_dsc_endian endian;
	/** Sync port selection, sync_port is #slcr_sync_port */
	enum slcr_sync_port sync_port;
};

/** @ingroup IP_group_slicer_external_struct
 * @brief Slicer DP GCE data structure.
 */
struct slcr_gce {
	/** GCE event selection, event is #slcr_gce_event */
	enum slcr_gce_event event;
	/** Height when GCE event triggered, range from 0 to 8191 */
	u16 height;
	/** Width when GCE event triggered, range from 0 to 8191 */
	u16 width;
};

/** @ingroup IP_group_slicer_external_struct
 * @brief Slicer DP video data structure.
 */
struct slcr_dp_video {
	/** Struct for DP-video input configuration */
	struct dp_video_input input;
	/** Struct for DP-video output configuration */
	struct dp_video_output output;
	/** Struct for DP-video GCE configuration */
	struct slcr_gce gce;
	/**  Struct for 3D re-shaped configuration*/
	struct slcr_rs_cfg data_path;
	/** Dither enable, 0: disable, 1: enable */
	bool dither_en;
	/** Color space conversion enable, 0: disable, 1: enable */
	bool csc_en;
	/** Color space conversion matrix, csc_mode is #slcr_csc_mode */
	enum slcr_csc_mode csc_mode;
	/** Dither type, dither_type is #slcr_dither_type */
	enum slcr_dither_type dither_type;
	/** Dither mode, dither_mode is #slcr_dither_mode */
	enum slcr_dither_mode dither_mode;
};

/** @ingroup IP_group_slicer_external_struct
 * @brief Slicer DP DSC data structure.
 */
struct slcr_dp_dsc {
	/** Struct for DP-DSC input configuration */
	struct dp_dsc_input input;
	/** Struct for DP-DSC output configuration */
	struct dp_dsc_output output;
	/** Struct for DP-DSC GCE configuration */
	struct slcr_gce gce;
};

/** @ingroup IP_group_slicer_external_struct
 * @brief Slicer pattern generator frame layout data structure.
 */
struct slcr_pg_frame_cfg {
	/** VSYNC polarity, 0: negative, 1: positive */
	bool vsync_pol;
	/** HSYNC polarity, 0: negative, 1: positive */
	bool hsync_pol;
	/** Length of VSYNC, range from 0 to 8191 */
	u16 vsync;
	/** Length of HSYNC, range from 0 to 8191 */
	u16 hsync;
	/** Vertical back porch, range from 0 to 8191 */
	u16 v_back;
	/** Horizontal back porch, range from 0 to 8191 */
	u16 h_back;
	/** Vertical active, range from 0 to 8191 */
	u16 v_active;
	/** Horizontal active, range from 0 to 8191 */
	u16 h_active;
	/** Vertical front porch, range from 0 to 8191 */
	u16 v_front;
	/** Horizontal front porch, range from 0 to 8191 */
	u16 h_front;
};

/** @ingroup IP_group_slicer_external_struct
 * @brief Slicer pattern generator pattern configuration data structure.
 */
struct slcr_pg_pat_color {
	/** Color bar height, range from 0 to 8191 */
	u16 cbar_height;
	/** Color bar width, range from 0 to 8191 */
	u16 cbar_width;
	/** Initial value, range from 0 to 4095 */
	u16 init_value;
	/** Vertical addend, range from 0 to 4095 */
	u16 v_addend;
	/** Horizontal addend, range from 0 to 4095 */
	u16 h_addend;
};

/** @ingroup IP_group_slicer_external_struct
 * @brief Slicer pattern generator pattern configuration data structure.
 */
struct slcr_pg_pat_cfg {
	/** Struct for pattern R configuration */
	struct slcr_pg_pat_color color_r;
	/** Struct for pattern G configuration */
	struct slcr_pg_pat_color color_g;
	/** Struct for pattern B configuration */
	struct slcr_pg_pat_color color_b;
};

/** @ingroup IP_group_slicer_external_struct
 * @brief Slicer pattern generator DSC data structure.
 */
struct slcr_pg_dsc_cfg {
	/** Bit rate, range from 0 to 63 */
	u8 bit_rate;
	/** Chunk numbers, range from 0 to 7 */
	u8 chunk_num;
};

/** @ingroup IP_group_slicer_external_struct
 * @brief Slicer configuration data structure.
 */
struct slcr_config {
	/** Input frame format, in_format is #slcr_in_format */
	enum slcr_in_format in_format;
	/** Struct for DP video */
	struct slcr_dp_video dp_video;
	/** Struct for DP DSC */
	struct slcr_dp_dsc dp_dsc;
};

/** @ingroup IP_group_slicer_external_struct
 * @brief Slicer pattern generator configuration data structure.
 */
struct slcr_pg_config {
	/** Pattern generator mode selection, pg_mode is #slcr_pg_mode */
	enum slcr_pg_mode pg_mode;
	/** Pattern generator timing selection, pg_timing is #slcr_pg_timing */
	enum slcr_pg_timing pg_timing;
	/** Pattern generator fps selection, pg_timing is #slcr_pg_fps */
	enum slcr_pg_fps pg_fps;
	/** Struct for pattern generator timing configuration of manual mode */
	struct slcr_pg_frame_cfg pg_frame_cfg;
	/** Struct for pattern RGB configuration */
	struct slcr_pg_pat_cfg pg_pat_cfg;
	/** Struct for pattern generator DSC configuration */
	struct slcr_pg_dsc_cfg pg_dsc_cfg;
};

/* API function prototype */
int mtk_slcr_clk_enable(const struct device *dev, const bool enable);
int mtk_slcr_power_on(struct device *dev);
int mtk_slcr_power_off(struct device *dev);
int mtk_slcr_start(const struct device *dev,
		    struct cmdq_pkt **cmdq_handle);
int mtk_slcr_stop(const struct device *dev,
		   struct cmdq_pkt **cmdq_handle);
int mtk_slcr_patgen_start(const struct device *dev,
			   struct cmdq_pkt **cmdq_handle);
int mtk_slcr_patgen_stop(const struct device *dev,
			  struct cmdq_pkt **cmdq_handle);
int mtk_slcr_reset(const struct device *dev,
		    struct cmdq_pkt **cmdq_handle);
int mtk_slcr_patgen_reset(const struct device *dev,
			   struct cmdq_pkt **cmdq_handle);
int mtk_slcr_config(const struct device *dev,
		    struct cmdq_pkt **cmdq_handle,
		    const struct slcr_config *cfg);
int mtk_slcr_patgen_config(const struct device *dev,
			   struct cmdq_pkt **cmdq_handle,
			   const struct slcr_pg_config *pat_cfg);
int mtk_slcr_patgen_fps_config(const struct device *dev,
			u16 h_total, u16 v_total,
			enum slcr_pg_fps pg_fps);
int mtk_slcr_patgen_get_frame_status(const struct device *dev,
			struct slcr_pg_frame_cfg *pg_frame_cfg);
int mtk_slcr_patgen_sw_trig(const struct device *dev,
			     struct cmdq_pkt **cmdq_handle);
int mtk_slcr_set_gce_event(const struct device *dev,
			struct cmdq_pkt **cmdq_handle,
			enum slcr_in_format in_format,
			struct slcr_gce gce);
#endif
