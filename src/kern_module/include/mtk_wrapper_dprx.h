/*
 * Copyright (C) 2021 Sony Interactive Entertainment Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _MTK_WRAPPER_DPRX_
#define _MTK_WRAPPER_DPRX_

#include <linux/ioctl.h>
#include <linux/types.h>
#include "mtk_wrapper_common.h"


#define DPRX_DEVICE_NAME "/dev/dprx"

#define DPRX_DEVICE_IOC_TYPE 'D'

#define DPRX_MAX_CB_BUF_SIZE (64)
#define DPRX_WRAPPER_MAX_LANE_NUM (4)

#define JTOL_STATUS_MASK 0x40
#define JTOL_ENABLED 0x40
#define JTOL_DISABLED 0x00

#define SPD_HB_LEN 2
#define SPD_INFOFRAME_LEN 25

typedef enum {
	DPRX_CBEVENT_NONE = 0,
	DPRX_CBEVENT_TIMING_LOCK,
	DPRX_CBEVENT_TIMING_UNLOCK,
	DPRX_CBEVENT_AUDIO_LOCK,
	DPRX_CBEVENT_AUDIO_UNLOCK,
	DPRX_CBEVENT_DSC_CHANGE,
	DPRX_CBEVENT_PPS_CHANGE,
	DPRX_CBEVENT_BW_CHANGE,
	DPRX_CBEVENT_SPD_INFO_CHANGE,
	DPRX_CBEVENT_LINK_ERROR,
	DPRX_CBEVENT_MAX_NUM
} DPRX_CBEVENT;

typedef enum {
	DPRX_VIDEO_FORMAT_RGB,
	DPRX_VIDEO_FORMAT_YUV444,
	DPRX_VIDEO_FORMAT_YUV422
} DPRX_VIDEO_FORAMT;

typedef struct _dprx_video_info {
	unsigned int h_active;
	unsigned int v_active;
	unsigned int h_polarity;
	unsigned int v_polarity;
	unsigned int h_total;
	unsigned int v_total;
	unsigned int h_frontporch;
	unsigned int v_frontporch;
	unsigned int h_backporch;
	unsigned int v_backporch;
	MTK_WRAPPER_FRAME_FORMAT format;
	unsigned int frame_rate;
	unsigned int bpc;
	bool dsc_enable;
} dprx_video_info;

#define MTK_WRAPPER_DPRX_PPS_SIZE (128)

typedef struct _args_dprx_pps {
	__u8 pps[MTK_WRAPPER_DPRX_PPS_SIZE];
} args_dprx_pps;

typedef struct _args_dprx_link_status {
	__u8 link_bandwidth;
	__u8 lane_count;
	__u8 lane_status[DPRX_WRAPPER_MAX_LANE_NUM];
	__u8 spread_amp;
	__u8 voltage_swing_level[DPRX_WRAPPER_MAX_LANE_NUM];
	__u8 pre_emphasis_level[DPRX_WRAPPER_MAX_LANE_NUM];
} args_dprx_link_status;

typedef enum {
	DPRX_AUDIO_TYPE_UNKNOWN = 0,
	DPRX_AUDIO_TYPE_LPCM = 1,
	DPRX_AUDIO_TYPE_AC3 = 2,
	DPRX_AUDIO_TYPE_MPEG1 = 3,
	DPRX_AUDIO_TYPE_MP3 = 4,
	DPRX_AUDIO_TYPE_MPEG2 = 5,
	DPRX_AUDIO_TYPE_AACLC = 6,
	DPRX_AUDIO_TYPE_DTS = 7,
	DPRX_AUDIO_TYPE_ATRAC = 8,
	DPRX_AUDIO_TYPE_ONE_BIT_AUDIO = 9,
	DPRX_AUDIO_TYPE_EAC3 = 10,
	DPRX_AUDIO_TYPE_DTS_HD = 11,
	DPRX_AUDIO_TYPE_MAT = 12,
	DPRX_AUDIO_TYPE_DST = 13,
	DPRX_AUDIO_TYPE_WMA_PRO = 14,
	DPRX_AUDIO_TYPE_EXT = 15,
} DPRX_AUDIO_CODING_TYPE;

typedef enum {
	DPRX_AUDIO_TYPE_EXT_UNKNOWN = 0,
	DPRX_AUDIO_TYPE_EXT_HEAAC = 4,
	DPRX_AUDIO_TYPE_EXT_HEAAC_V2 = 5,
	DPRX_AUDIO_TYPE_EXT_MPEG4_AACLC = 6,
	DPRX_AUDIO_TYPE_EXT_DRA = 7,
	DPRX_AUDIO_TYPE_EXT_HEAAC_SURROUND = 8,
	DPRX_AUDIO_TYPE_EXT_AACLC_SURROUND = 10,
} DPRX_AUDIO_CODING_TYPE_EXT;

typedef struct _dprx_audio_infoframe {
	int channels;
	DPRX_AUDIO_CODING_TYPE coding_type;
	int bit_width;
	int rate;
	DPRX_AUDIO_CODING_TYPE_EXT coding_type_ext;
} dprx_audio_infoframe;

typedef struct _dprx_audio_channel_status {
	int is_lpcm;
	int channels;
	int rate;
	int bit_width;
} dprx_audio_channel_status;

typedef struct _dprx_audio_status {
	dprx_audio_infoframe audio_info;
	dprx_audio_channel_status ch_sts;
	__u32 mute_reason;
} dprx_audio_status;

typedef struct _dprx_audio_pll_info {
	unsigned int m_aud;
	unsigned int n_aud;
	unsigned int rate;
	__u8 link_speed;
	__u8 audio_clk_sel;
} dprx_audio_pll_info;

typedef struct _dprx_hdcp_info {
	__u8 hdcp13_auth_done;
	__u8 hdcp13_encrypted;
	__u8 hdcp22_auth_status;
	__u8 hdcp22_encrypted;
} dprx_hdcp_info;

typedef struct _dprx_phy_gce_info {
	__u8 status;
} dprx_phy_gce_info;

typedef struct _dprx_spd_info {
	__u8 buf[SPD_HB_LEN + SPD_INFOFRAME_LEN];
} dprx_spd_info;

#define DPRX_POWER_ON  _IOR(DPRX_DEVICE_IOC_TYPE, 1, bool)
#define DPRX_POWER_OFF _IO(DPRX_DEVICE_IOC_TYPE, 2)
#define DPRX_GET_VIDEO_INFO _IOR(DPRX_DEVICE_IOC_TYPE, 3, dprx_video_info)
#define DPRX_GET_PPS _IOR(DPRX_DEVICE_IOC_TYPE, 4, args_dprx_pps)
#define DPRX_GET_LINK_STATUS _IOR(DPRX_DEVICE_IOC_TYPE, 5, args_dprx_link_status)
#define DPRX_GET_AUDIO_STATUS _IOR(DPRX_DEVICE_IOC_TYPE, 6, dprx_audio_status)
#define DPRX_GET_AUDIO_PLL_INFO _IOR(DPRX_DEVICE_IOC_TYPE, 7, dprx_audio_pll_info)
#define DPRX_GET_HDCP_INFO _IOR(DPRX_DEVICE_IOC_TYPE, 8, dprx_hdcp_info)
#define DPRX_GET_PHY_GCE_INFO _IOR(DPRX_DEVICE_IOC_TYPE, 9, dprx_phy_gce_info)
#define DPRX_PHY_GCE_INIT _IO(DPRX_DEVICE_IOC_TYPE, 10)
#define DPRX_PHY_GCE_DEINIT _IO(DPRX_DEVICE_IOC_TYPE, 11)
#define DPRX_GET_SPD_INFO _IO(DPRX_DEVICE_IOC_TYPE, 12)
#define DPRX_RESET_FIFO _IO(DPRX_DEVICE_IOC_TYPE, 13)
#define DPRX_RELEASE_FIFO _IO(DPRX_DEVICE_IOC_TYPE, 14)

#endif /* _MTK_WRAPPER_DPRX_ */
