/*
 * Copyright (c) 2015 MediaTek Inc.
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
 * @file mtk_dsi.h
 * External header of mtk_dsi.c
 */

#ifndef _MTK_DSI_H_
#define _MTK_DSI_H_

#include <linux/soc/mediatek/mtk-cmdq.h>
#include <soc/mediatek/mtk_dsc.h>
#include <video/videomode.h>

/** @ingroup IP_group_dsi_external_enum
 * @brief DSI pixel format option
 */
enum mtk_dsi_pixel_format {
	/** Pixel format is RGB101010 */
	MTK_DSI_FMT_RGB101010,
	/** Pixel format is RGB888 */
	MTK_DSI_FMT_RGB888,
	/** Pixel format is RGB666 */
	MTK_DSI_FMT_RGB666,
	/** Pixel format is RGB666 loosely */
	MTK_DSI_FMT_RGB666_LOOSELY,
	/** Pixel format is RGB565 */
	MTK_DSI_FMT_RGB565,
	/** RGB101010 with compression rate = 1/2 */
	MTK_DSI_FMT_COMPRESSION_30_15,
	/** RGB101010 with compression rate = 1/3 */
	MTK_DSI_FMT_COMPRESSION_30_10,
	/** RGB888 with compression rate = 1/2 */
	MTK_DSI_FMT_COMPRESSION_24_12,
	/** RGB888 with compression rate = 1/3 */
	MTK_DSI_FMT_COMPRESSION_24_8,
	/** Supported total pixel format number */
	MTK_DSI_FMT_NR
};

/** @ingroup IP_group_dsi_external_enum
 * @brief DSI frame rate option
 */
enum mtk_dsi_fps {
	/** frame rate is 30 fps */
	MTK_DSI_FPS_30 = 30,
	/** frame rate is 50 fps */
	MTK_DSI_FPS_50 = 50,
	/** frame rate is 60 fps */
	MTK_DSI_FPS_60 = 60,
	/** frame rate is 90 fps */
	MTK_DSI_FPS_90 = 90,
	/** frame rate is 120 fps */
	MTK_DSI_FPS_120 = 120
};

/** @ingroup IP_group_dsi_external_enum
 * @brief DSI panel index option
 */
enum mtk_dsi_panel_index {
	/** 8 bpc */
	MTK_LCM_35595,
	/** 8 bpc */
	MTK_LCM_35695B,
	/** 8 bpc */
	MTK_LCM_RGB888,
	/** 8 bpc to 8 bpp */
	MTK_LCM_COMPRESSION_24_8,
	/** 8 bpc to 12 bpp */
	MTK_LCM_COMPRESSION_24_12,
	/** 10 bpc to 10 bpp */
	MTK_LCM_COMPRESSION_30_10,
	/** 10 bpc to 15 bpp */
	MTK_LCM_COMPRESSION_30_15,
	/** 8 bpc to 8 bpp */
	MTK_LCM_3840_DSC_24_8,
	/** 10 bpc to 10 bpp */
	MTK_LCM_3840_DSC_30_10,
	/** total lcm number */
	MTK_LCM_NR
};

/** @ingroup IP_group_dsi_external_struct
 * @brief Data Structure for store panel init parameter.
 */
struct lcm_setting_table {
	/** packet command */
	unsigned int cmd;
	/** command parameter count */
	unsigned char count;
	/** command parameter list */
	unsigned char para_list[128];
};

/** @ingroup IP_group_dsi_external_struct
 * @brief Data Structure used for receiving packet data from panel.
 */
struct dsi_read_data {
	u8 byte[4][10];
};

/* for DDP use */
int mtk_dsi_lcm_setting(const struct device *dev,
			const struct lcm_setting_table *table, u32 count);
int mtk_dsi_lcm_vm_setting(const struct device *dev,
			   const struct lcm_setting_table *table, u32 count);
int mtk_dsi_lcm_reset(const struct device *dev);
int mtk_dsi_hw_init(const struct device *dev);
int mtk_dsi_hw_fini(const struct device *dev);
int mtk_dsi_lane_config(const struct device *dev, int lane_num);
int mtk_dsi_lane_swap(const struct device *dev, int lane_swap);
int mtk_dsi_output_self_pattern(const struct device *dev, bool enable,
				u32 color);
int mtk_dsi_output_start(const struct device *dev, struct cmdq_pkt *handle);
int mtk_dsi_output_stop(const struct device *dev);
int mtk_dsi_get_fps(const struct device *dev, u32 *fps);
int mtk_dsi_set_fps(const struct device *dev, enum mtk_dsi_fps fps);
int mtk_dsi_get_lcm_width(const struct device *dev, u32 *width);
int mtk_dsi_get_lcm_height(const struct device *dev, u32 *height);
int mtk_dsi_get_dsc_config(const struct device *dev,
			   struct dsc_config *dsc_cfg);
int mtk_dsi_send_dcs_cmd(const struct device *dev, u32 cmd, u8 count,
			 const u8 *para_list);
int mtk_dsi_read_dcs_cmd(const struct device *dev, u32 cmd, u8 pkt_size,
			 struct dsi_read_data *read_data);
int mtk_dsi_send_vm_cmd(const struct device *dev, u8 data_id, u8 data0,
			u8 data1, const u8 *para_list);
int mtk_dsi_deskew(const struct device *dev);
int mtk_dsi_set_frame_cksm(const struct device *dev, u8 index, bool enable);
int mtk_dsi_get_frame_cksm(const struct device *dev, u8 index, u16 *checksum);
int mtk_dsi_get_phy_cksm(const struct device *dev, u8 index, u16 *checksum);
int mtk_dsi_get_line_cnt(const struct device *dev);
int mtk_dsi_enable_frame_counter(const struct device *dev, u8 index,
				 bool enable);
int mtk_dsi_get_frame_counter(const struct device *dev, u8 index, u32 *counter);
int mtk_dsi_hw_mute_config(const struct device *dev, bool enable);
int mtk_dsi_sw_mute_config(const struct device *dev, bool enable,
			   struct cmdq_pkt *handle);
int mtk_dsi_mute_color(const struct device *dev, u32 color);
int mtk_dsi_reset(const struct device *dev);
int mtk_dsi_ps_config(const struct device *dev,
		      enum mtk_dsi_pixel_format format);
int mtk_dsi_set_datarate(const struct device *dev, u32 rate);
int mtk_dsi_set_lcm_width(const struct device *dev, u32 width);
int mtk_dsi_set_lcm_height(const struct device *dev, u32 height);
int mtk_dsi_set_lcm_index(const struct device *dev,
			  enum mtk_dsi_panel_index index);
int mtk_dsi_get_lcm_index(const struct device *dev, u32 *index);
int mtk_dsi_set_lcm_vfrontporch(const struct device *dev, u32 frontporch);
int mtk_dsi_get_lcm_vfrontporch(const struct device *dev, u32 *frontporch);
int mtk_dsi_get_lcm_vtotal(const struct device *dev, u32 *vtotal);
int mtk_dsi_set_lcm_vblanking(const struct device *dev, u32 synclen,
			      u32 backporch, u32 frontporch);
int mtk_dsi_set_lcm_hblanking(const struct device *dev, u32 datarate,
			      u32 framerate, u32 synclen, u32 backporch);
int mtk_dsi_set_lcm_hblank(const struct device *dev, u32 synclen,
			   u32 backporch, u32 frontporch);
int mtk_dsi_set_display_mode(const struct device *dev, unsigned long mode);
int mtk_dsi_set_format(const struct device *dev,
		       enum mtk_dsi_pixel_format format);
int mtk_dsi_set_panel_params(struct device *dev, unsigned long mode_flags,
			     enum mtk_dsi_pixel_format format,
			     const struct videomode *vm);
int mtk_dsi_vact_event_config(const struct device *dev, u16 offset, u16 period,
			      bool enable);

#endif
