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

#ifndef _MTK_DPRX_INFO_H_
#define _MTK_DPRX_INFO_H_
/**
 * @file mtk_dprx_info.h
 * Header of external DPRX API
 */

/* ====================== Global Variable ========================== */
/** @ingroup IP_group_dprx_external_def
 * @{
 */
#define INVALID_FRAME_RATE                1
#define AVI_HB_LEN                        2
#define AVI_INFOFRAME_LEN                 13
#define AUDIO_INFOFRAME_LEN               10
#define IEC_CH_STATUS_LEN                 5
#define SPD_HB_LEN                        2
#define SPD_INFOFRAME_LEN                 25
#define MPEG_HB_LEN                       2
#define MPEG_INFOFRAME_LEN                10
#define HDR_HB_LEN                        2
#define HDR_INFOFRAME_LEN                 26
#define PPS_SDP_LEN                       128

#define AUTH_IDLE                         0x00
#define AUTH_AKE0                         0x01
#define AUTH_AKE1                         0x02
#define AUTH_AKE2                         0x03
#define AUTH_LC                           0x04
#define AUTH_SKE                          0x05
#define AUTH_RPT                          0x06
#define AUTH_RCS                          0x07
#define AUTH_DONE                         0x08
#define AUTH_LICHECK                      0x09
/** @}
 */

/* ====================== Global Structure ========================= */
/** @ingroup IP_group_dprx_external_enum
 * @brief DPRX video color format mode
 */
enum DPRX_VID_COLOR_FORMAT {
	/** RGB444 mode */
	RGB444,
	/** YUV444 mode */
	YUV444,
	/** YUV422 mode */
	YUV422,
	/** YUV420 mode */
	YUV420,
	/** YONLY mode */
	YONLY,
	/** RAW mode */
	RAW,
};

/** @ingroup IP_group_dprx_external_enum
 * @brief DPRX video YUV tpye
 */
enum DPRX_VID_YUV_TYPE {
	/** YUV BT601 mode */
	BT601,
	/** YUV BT709 mode */
	BT709,
	/** Not defined */
	NOT_DEFINED,
};

/** @ingroup IP_group_dprx_external_enum
 * @brief DPRX video bpc mode
 */
enum DPRX_VID_COLOR_DEPTH {
	/** Video 6 bpc */
	VID6BIT,
	/** Video 7 bpc */
	VID7BIT,
	/** Video 8 bpc */
	VID8BIT,
	/** Video 10 bpc */
	VID10BIT,
	/** Video 12 bpc */
	VID12BIT,
	/** Video 14 bpc */
	VID14BIT,
	/** Video 16 bpc */
	VID16BIT,
};

/** @ingroup IP_group_dprx_external_struct
 * @brief DPRX video crc value structure
 */
struct dprx_video_crc_s {
	/** Video CRC R Cr - low byte */
	u8 lb_crc_r_cr;
	/** Video CRC R Cr - high byte */
	u8 hb_crc_r_cr;
	/** Video CRC G Y - low byte */
	u8 lb_crc_g_y;
	/** Video CRC G Y - high byte */
	u8 hb_crc_g_y;
	/** Video CRC B Cb - low byte */
	u8 lb_crc_b_cb;
	/** Video CRC B Cb - high byte */
	u8 hb_crc_b_cb;
};

/** @ingroup IP_group_dprx_external_struct
 * @brief DPRX Link training status structure
 */
struct dprx_lt_status_s {
	/** Lane 0 Swing */
	u8 l0_swing;
	/** Lane 0 Pre-emphasis */
	u8 l0_pre_emphasis;
	/** Lane 1 Swing */
	u8 l1_swing;
	/** Lane 1 Pre-emphasis */
	u8 l1_pre_emphasis;
	/** Lane 2 Swing */
	u8 l2_swing;
	/** Lane 2 Pre-emphasis */
	u8 l2_pre_emphasis;
	/** Lane 3 Swing */
	u8 l3_swing;
	/** Lane 3 Pre-emphasis */
	u8 l3_pre_emphasis;
};

/** @ingroup IP_group_dprx_external_struct
 * @brief DPRX HDCP related status structure
 */
struct dprx_hdcp_status_s {
	/** HDCP1.3 Authentication status indication */
	u8 hdcp13_auth_done;
	/** HDCP1.3 Encrypted frame */
	u8 hdcp13_encrypted;
	/** HDCP2.2 Authentication status */
	u8 hdcp22_auth_status;
	/** HDCP2.2 Encrypted frame */
	u8 hdcp22_encrypted;
};

/** @ingroup IP_group_dprx_external_struct
 * @brief DPRX video timing inforsmation structure
 */
struct dprx_video_timing_s {
	/** V Total */
	u16 v_total;
	/** V Active */
	u16 v_active;
	/** V Front Porch */
	u16 v_front_porch;
	/** V Sync Width */
	u16 v_sync_width;
	/** V Back Porch */
	u16 v_back_porch;
	/** V Polarity */
	u16 v_polarity;
	/** H Total */
	u16 h_total;
	/** H Active */
	u16 h_active;
	/** H Front Porch */
	u16 h_front_porch;
	/** H Sync Width */
	u16 h_sync_width;
	/** H Back Porch */
	u16 h_back_porch;
	/** H Polarity */
	u16 h_polarity;
};

/** @ingroup IP_group_dprx_external_struct
 * @brief DPRX video inforsmation structure
 */
struct dprx_video_info_s {
	/** Video Frame Rate */
	u8 frame_rate;
	/** DSC Mode Information */
	u8 dsc_mode;
	/** PPS Change Information */
	u8 pps_change;
	/** Stereo Type */
	u8 stereo_type;
	/** Video Timing Structure */
	struct dprx_video_timing_s vid_timing_msa;
	/** Video Color Format */
	enum DPRX_VID_COLOR_FORMAT vid_color_fmt;
	/** Video YUV type */
	enum DPRX_VID_YUV_TYPE vid_yuv_type;
	/** Video Color Depth */
	enum DPRX_VID_COLOR_DEPTH vid_color_depth;
	/** Video Timing Structure for DP output */
	struct dprx_video_timing_s vid_timing_dpout;
};

/** @ingroup IP_group_dprx_external_struct
 * @brief DPRX HDR inforsmation structure
 */
struct hdr_infoframe_s {
	/** HDR InfoFrame Header byte array */
	u8 hdr_hb[HDR_HB_LEN];
	/** HDR InfoFrame Data byte array */
	u8 hdr_db[HDR_INFOFRAME_LEN];
};

/** @ingroup IP_group_dprx_external_struct
 * @brief DPRX PPS inforsmation structure
 */
struct pps_sdp_s {
	/** PPS Data byte array */
	u8 pps_db[PPS_SDP_LEN];
};

/** @ingroup IP_group_dprx_external_struct
 * @brief DPRX AVI infoframe structure\n
 * 2 structure: information structure and packet byte structure
 */
union avi_infoframe {
	struct __packed {
		/** InfoFrame Version number */
		u8 ver;
		/** Length of AVI InfoFrame */
		u8 len;

		/** Scan information */
		u8 scan:2;
		/** Bar Data preasent */
		u8 bar_info:2;
		/** Active Format Information preasent */
		u8 active_fmt_info_present:1;
		/** Color mode */
		u8 color_mode:2;
		/** Reserved */
		u8 fu1:1;

		/** Active Portion Aspect Ratio */
		u8 active_format_aspect_ratio:4;
		/** Picture Aspect Ratio */
		u8 picture_aspect_ratio:2;
		/** Colorimetry */
		u8 colorimetry:2;

		/** Non-Uniform Picture Scaling */
		u8 scaling:2;
		/** Reserved */
		u8 fu2:6;

		/** Video Identification Code */
		u8 vic:7;
		/** Reserved */
		u8 fu3:1;

		/** Pixel Repetition Factor */
		u8 pixel_repetition:4;
		/** Reserved */
		u8 fu4:4;

		/** Line No. of End of Top Bar */
		u16 ln_end_top;
		/** Line No. of Start of Bottom Bar */
		u16 ln_start_bottom;
		/** Pixel No. of End of Left Bar */
		u16 pix_end_left;
		/** Pixel No. of Start of Right Bar */
		u16 pix_start_right;
	} info;
	struct __packed {
		/** AVI InfoFrame Header byte array */
		u8 avi_hb[AVI_HB_LEN];
		/** AVI InfoFrame Data byte array */
		u8 avi_db[AVI_INFOFRAME_LEN];
	} pktbyte;
};

/** @ingroup IP_group_dprx_external_struct
 * @brief DPRX Audio infoframe structure\n
 * 2 structure: information structure and packet byte structure
 */
union audio_infoframe {
	struct __packed {
		/** Audio Channel Count */
		unsigned char aud_ch_cnt:3;
		/** Reserved */
		unsigned char rsvd1:1;
		/** Audio Coding Type */
		unsigned char aud_coding_type:4;

		/** Sample Size */
		unsigned char sample_size:2;
		/** Sampling Frequency */
		unsigned char sample_freq:3;
		/** Reserved */
		unsigned char rsvd2:3;

		/** Audio Format Type */
		unsigned char fmt_coding;

		/** Speaker Location */
		unsigned char speaker_placement;

		/** Reserved */
		unsigned char rsvd3:3;
		/** Level Shift Value */
		unsigned char level_shift_value:4;
		/** Down-Mix Inhibit Flag */
		unsigned char dm_inh:1;
	} info;

	struct __packed {
		/** Audio InfoFrame Data byte array */
		unsigned char aud_db[AUDIO_INFOFRAME_LEN];
	} pktbyte;

};

/** @ingroup IP_group_dprx_external_struct
 * @brief DPRX Audio channel structure structure\n
 * 2 structure: information structure and packet byte structure
 */
union dprx_audio_chsts {
	struct {
		/** Use of Channel Status Block */
		u8 rev :1;
		/** Linear PCM Identification */
		u8 is_lpcm :1;
		/** Copyright Information */
		u8 copy_right : 1;
		/** Additional Format Information */
		u8 addition_format_info :3;
		/** Channel Status Mode */
		u8 channel_status_mode :2;

		/** Category Code */
		u8 category_code;

		/** Source Number */
		u8 source_number :4;
		/** Channel Number */
		u8 channel_number :4;

		/** Sampling Frequency */
		u8 sampling_freq :4;
		/** Clock Accuracy */
		u8 clock_accurary :2;
		/** Reserved */
		u8 rev2 : 2;

		/** Word Length */
		u8 word_len : 4;
		/** Original Sampling Frequency */
		u8 original_sampling_freq :4;
	} iec_ch_sts;
	/** Audio Channel Status byte array */
	u8 aud_ch_sts[IEC_CH_STATUS_LEN];
};

/** @ingroup IP_group_dprx_external_struct
 * @brief DPRX AUDIO PLL status structure\n
 * 2 structure: AUDIO PLL status structure
 */
struct dprx_audio_pll_info_s {
	u32 m_aud;
	u32 n_aud;
	u8 link_speed;
	u8 audio_clk_sel;
};

/** @ingroup IP_group_dprx_external_struct
 * @brief DPRX MPEG infoframe structure\n
 * 2 structure: information structure and packet byte structure
 */
union mpeg_infoframe {
	struct __packed {
		/** InfoFrame Version number */
		u8 ver;
		/** Length of MPEG InfoFrame */
		u8 len;

		/** MPEG Bit Rate */
		u64 mpeg_bit_rate;

		/** MPEG Frame */
		u8 mpeg_frame:2;
		/** Reserved */
		u8 rvsd1:2;
		/** Field Repeat */
		u8 field_repeat:1;
		/** Reserved */
		u8 rvsd2:3;
	} info;
	struct __packed {
		/** MPEG InfoFrame Header byte array */
		u8 mpg_hb[MPEG_HB_LEN];
		/** MPEG InfoFrame Data byte array */
		u8 mpg_db[MPEG_INFOFRAME_LEN];
	} pktbyte;
};

/** @ingroup IP_group_dprx_external_struct
 * @brief DPRX SPD infoframe structure\n
 * 2 structure: information structure and packet byte structure
 */
union spd_infoframe {
	struct __packed {
		/** InfoFrame Version number */
		u8 ver;
		/** Length of SPD InfoFrame */
		u8 len;

		/** Vendor Name Character */
		char vn[8];
		/** Product Description Character */
		char pd[16];
		/** Source Information */
		u8 source_device_information;
	} info;
	struct __packed {
		/** SPD InfoFrame Header byte array */
		u8 spd_hb[SPD_HB_LEN];
		/** SPD InfoFrame Data byte array */
		u8 spd_db[SPD_INFOFRAME_LEN];
	} pktbyte;
};

/** @ingroup IP_group_dprx_external_enum
 * @brief DPRX callback event
 */
enum DPRX_NOTIFY_T {
	DPRX_RX_MSA_CHANGE = 0,
	DPRX_RX_AUD_MN_CHANGE,
	DPRX_RX_PPS_CHANGE,
	DPRX_RX_DSC_CHANGE,
	DPRX_RX_UNPLUG,
	DPRX_RX_PLUGIN,
	DPRX_RX_VIDEO_STABLE,
	DPRX_RX_VIDEO_NOT_STABLE,
	DPRX_RX_VIDEO_MUTE,
	DPRX_RX_AUDIO_INFO_CHANGE,
	DPRX_RX_HDR_INFO_CHANGE,
	DPRX_RX_AUDIO_MUTE,
	DPRX_RX_AUDIO_UNMUTE,
	DPRX_RX_STEREO_TYPE_CHANGE,
	DPRX_RX_BW_CHANGE,
	DPRX_RX_SPD_INFO_CHANGE,
	DPRX_RX_LINK_ERROR,
};

#endif
