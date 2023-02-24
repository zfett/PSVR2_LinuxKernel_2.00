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

#ifndef _MTK_DPRX_AVC_H_
#define _MTK_DPRX_AVC_H_

/**
 * @file mtk_dprx_avc_int.h
 * Header of internal DPRX function
 */

/* ====================== Global Variable ========================== */
/** @brief extern dprx_video_info_s structure as dp_vid_status */
extern struct dprx_video_info_s dp_vid_status;
/** @brief extern avi_infoframe union as dp_avi_info */
extern union avi_infoframe dp_avi_info;
/** @brief extern dprx_audio_chsts union as dp_aud_ch_sts */
extern union dprx_audio_chsts dp_aud_ch_sts;
/** @brief extern audio_infoframe union as dp_aud_info */
extern union audio_infoframe dp_aud_info;
/** @brief extern spd_infoframe union as dp_spd_info */
extern union spd_infoframe dp_spd_info;
/** @brief extern mpeg_infoframe union as dp_mpeg_info */
extern union mpeg_infoframe dp_mpeg_info;
/** @brief extern hdr_infoframe_s structure as dp_hdr_info */
extern struct hdr_infoframe_s dp_hdr_info;
/** @brief extern pps_sdp_s structure as dp_pps_info */
extern struct pps_sdp_s dp_pps_info;
/** @brief extern callback_sts_s structure as dprx_cb_sts */
extern struct callback_sts_s dprx_cb_sts;

/* ====================== Global Definition ======================== */
/** @ingroup IP_group_dprx_internal_def
 * @{
 */
#define ENABLE 1
#define DISABLE 0

#define LINK_RATE_UNIT 27

#define VIDEO_PLL_1 2
#define VIDEO_PLL_2 4
#define VIDEO_PLL_3 8
#define VIDEO_PLL_4 16
#define VIDEO_PLL_5 32
#define VIDEO_PLL_6 64

#define VIDEO_PLL_TH_1 800
#define VIDEO_PLL_TH_2 400
#define VIDEO_PLL_TH_3 200
#define VIDEO_PLL_TH_4 100
#define VIDEO_PLL_TH_5 50
#define VIDEO_PLL_TH_6 25

#define AUDIO_CLK_SEL_1 1
#define AUDIO_CLK_SEL_2 2
#define AUDIO_CLK_SEL_3 3
#define AUDIO_CLK_SEL_4 4
#define AUDIO_CLK_SEL_5 5
#define AUDIO_CLK_SEL_6 6
#define AUDIO_CLK_SEL_7 7

#define AUDIO_CLK_SEL_TH_1 30
#define AUDIO_CLK_SEL_TH_2 42
#define AUDIO_CLK_SEL_TH_3 46
#define AUDIO_CLK_SEL_TH_4 85
#define AUDIO_CLK_SEL_TH_5 94
#define AUDIO_CLK_SEL_TH_6 174
#define AUDIO_CLK_SEL_TH_7 190

#define VSC_HB2_REV_CF 0x05
/** @}
 */

/* ====================== Global Structure ========================= */
/** @ingroup IP_group_dprx_internal_struct
 * @brief DPRX video stable check timer structure\n
 */
struct video_stable_timer_s {
	/** timer_list structure*/
	struct timer_list timer;
	/** device structure*/
	struct mtk_dprx *dprx;
	/** index*/
	uint32_t index;
	uint32_t vid_stable_sts;
};

/* ====================== Function Prototype ======================= */

bool mtk_dprx_is_video_stable(void);
int mtk_dprx_is_in_dsc_mode(void);
void dprx_config_infoframe_change(void);
void dprx_config_vsc_pkg_change(void);
void dprx_config_video(void);
void dprx_set_video_pll(void);
void dprx_config_audio(void);
void dprx_audio_pll_config(struct work_struct *aud_work);
void dprx_config_video_mute_intr(void);
void dprx_config_video_unmute_intr(void);
void dprx_video_mute_process(void);
void dprx_video_unmute_process(void);
void dprx_video_sw_mute_enable_ctrl(bool enable);
void dprx_config_audio_mute_intr(void);
void dprx_deconfig_audio_mute_intr(void);
void dprx_config_audio_unmute_intr(void);
void dprx_audio_mute_process(void);
void dprx_audio_unmute_process(void);
void dprx_audio_sw_mute_enable_ctrl(bool enable);
void dprx_audio_unmute_cnt_enable_ctrl(bool enable);
int dprx_get_video_timing_msa(void);
int dprx_get_video_frame_rate_msa(void);
int dprx_get_video_format_msa(void);
int dprx_get_video_timing_dpout(void);
void dprx_get_video_format_vsc(void);
void dprx_get_video_format_not_vsc(void);
int dprx_1frame_delay(u8 count);
void dprx_video_stable_check(struct mtk_dprx *dprx);
void dprx_vid_stable_chk_cb(unsigned long arg);
void dprx_stereo_type_update(struct mtk_dprx *dprx);

#endif
