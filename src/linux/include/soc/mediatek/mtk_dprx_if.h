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

#ifndef _MTK_DPRX_IF_H_
#define _MTK_DPRX_IF_H_
/**
 * @file mtk_dprx_if.h
 * Header of external DPRX API
 */

extern bool f_phy_gce;
extern u8 f_unplug;
extern struct dprx_video_crc_s dp_vid_crc;

/* ====================== Function Prototype ======================= */
int mtk_dprx_drv_init(struct device *dev, bool hdcp2x);
int mtk_dprx_drv_deinit(struct device *dev);
int mtk_dprx_phy_gce_init(struct device *dev);
int mtk_dprx_phy_gce_deinit(struct device *dev);
int mtk_dprx_drv_start(void);
int mtk_dprx_drv_stop(void);
int mtk_dprx_power_on(struct device *dev);
int mtk_dprx_power_off(struct device *dev);
int mtk_dprx_power_on_phy(struct device *dev);
int mtk_dprx_power_off_phy(struct device *dev);
int dprx_get_video_info_msa(struct dprx_video_info_s *p);
int dprx_get_audio_pll_info(struct dprx_audio_pll_info_s *info);
int dprx_get_audio_info(u8 *buf, u8 size);
int dprx_drv_audio_get_mute_reason(u32 *info);
int dprx_get_audio_ch_status(u8 *buf, u8 size);
int dprx_get_avi_info(u8 *buf, u8 size);
int dprx_get_mpeg_info(u8 *buf, u8 size);
int dprx_get_hdr_info(struct hdr_infoframe_s *p);
int dprx_get_spd_info(u8 *buf, u8 size);
int dprx_get_pps_info(struct pps_sdp_s *p);
int dprx_drv_edid_init(u8 *data, u32 length);
int dprx_get_dpcd_value(u32 dpcd_offset);
int dprx_set_dpcd_dsc_value(u32 dpcd_offset, u8 value);
int dprx_get_power_status(void);
int dprx_set_lane_count(u8 lane_count);
int dprx_get_phy_eq_setting(struct dprx_lt_status_s *p);
int dprx_get_hdcp_status(struct dprx_hdcp_status_s *p);
int dprx_get_video_pll_status(void);
int dprx_get_audio_pll_status(void);
int dprx_set_callback(struct device *dev,
		      struct device *ap_dev,
		      int (*callback)(struct device *dev,
		      enum DPRX_NOTIFY_T event));

int dprx_get_unplug_status(void);
int dprx_get_video_crc(void);
bool dprx_get_video_stable_status(void);
int dprx_get_dsc_mode_status(void);
int dprx_get_stereo_type(void);
int dprx_get_phy_gce_status(void);
int dprx_hdcp2x_disable(void);
int dprx_set_hdcp1x_capability(bool enable);
void dprx_if_fifo_reset(void);
void dprx_if_fifo_release(void);

#endif
