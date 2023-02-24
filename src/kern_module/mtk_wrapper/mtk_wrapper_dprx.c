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

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/of_platform.h>
#include <linux/uaccess.h>
#include <soc/mediatek/mtk_dprx_info.h>
#include <soc/mediatek/mtk_dprx_if.h>

#include <sce_km_defs.h>
#include <mtk_wrapper_common.h>
#include <mtk_wrapper_dprx.h>

#include "mtk_wrapper_util.h"
#include "mtk_wrapper_edid.h"

static struct device *s_dprx;

static wait_queue_head_t dprx_wait_queue;
static u32 s_dprx_cb_video;
static u32 s_dprx_cb_audio;
static u32 s_dprx_cb_bw;
static u32 s_dprx_cb_spd_info;
static u32 s_dprx_cb_link_error;

static MTK_WRAPPER_FRAME_FORMAT convert_format(enum DPRX_VID_COLOR_FORMAT format)
{
	MTK_WRAPPER_FRAME_FORMAT ret = MTK_WRAPPER_FRAME_FORMAT_RGB;

	switch (format) {
	case RGB444:
		ret = MTK_WRAPPER_FRAME_FORMAT_RGB;
		break;
	case YUV444:
		ret = MTK_WRAPPER_FRAME_FORMAT_YUV444;
		break;
	case YUV422:
	case YUV420:
		ret = MTK_WRAPPER_FRAME_FORMAT_YUV422;
		break;
	case YONLY:
	case RAW:
	default:
		ret = MTK_WRAPPER_FRAME_FORMAT_UNKNOWN;
		break;
	}
	return ret;
}

static const char * const dprx_event_name[] = {
	[DPRX_RX_MSA_CHANGE] = "DPRX_RX_MSA_CHANGE",
	[DPRX_RX_AUD_MN_CHANGE] = "DPRX_RX_AUD_MN_CHANGE",
	[DPRX_RX_DSC_CHANGE] = "DPRX_RX_DSC_CHANGE",
	[DPRX_RX_PPS_CHANGE] = "DPRX_RX_PPS_CHANGE",
	[DPRX_RX_UNPLUG] = "DPRX_RX_UNPLUG",
	[DPRX_RX_PLUGIN] = "DPRX_RX_PLUGIN",
	[DPRX_RX_VIDEO_STABLE] = "DPRX_RX_VIDEO_STABLE",
	[DPRX_RX_VIDEO_NOT_STABLE] = "DPRX_RX_VIDEO_NOT_STABLE",
	[DPRX_RX_VIDEO_MUTE] = "DPRX_RX_VIDEO_MUTE",
	[DPRX_RX_AUDIO_INFO_CHANGE] = "DPRX_RX_AUDIO_INFO_CHANGE",
	[DPRX_RX_HDR_INFO_CHANGE] = "DPRX_RX_HDR_INFO_CHANGE",
	[DPRX_RX_AUDIO_MUTE] = "DPRX_RX_AUDIO_MUTE",
	[DPRX_RX_AUDIO_UNMUTE] = "DPRX_RX_AUDIO_UNMUTE",
	[DPRX_RX_STEREO_TYPE_CHANGE] = "DPRX_RX_STEREO_TYPE_CHANGE",
	[DPRX_RX_BW_CHANGE] = "DPRX_RX_BW_CHANGE",
	[DPRX_RX_SPD_INFO_CHANGE] = "DPRX_RX_SPD_INFO_CHANGE",
	[DPRX_RX_LINK_ERROR] = "DPRX_RX_LINK_ERROR",
};

/*
 * Callback Function
 */
static int callback_dprx_event(struct device *dev, enum DPRX_NOTIFY_T event)
{
	/* too many callbacks DSC_CHANGE ignore. */
	if (event != DPRX_RX_DSC_CHANGE) {
		if (event > ARRAY_SIZE(dprx_event_name) - 1) {
			pr_info("DPRX Callback Unknown Event(%d)\n", event);
		} else {
			pr_info("DPRX Callback event [%s](%d)\n", dprx_event_name[event], event);
		}
	}

	if (event == DPRX_RX_VIDEO_STABLE) {
		s_dprx_cb_video = DPRX_CBEVENT_TIMING_LOCK;
		wake_up_interruptible(&dprx_wait_queue);
	} else if (event == DPRX_RX_VIDEO_NOT_STABLE ||
			   event == DPRX_RX_VIDEO_MUTE) {
		s_dprx_cb_video = DPRX_CBEVENT_TIMING_UNLOCK;
		wake_up_interruptible(&dprx_wait_queue);
	} else if (event == DPRX_RX_UNPLUG) {
		s_dprx_cb_video = DPRX_CBEVENT_TIMING_UNLOCK;
		wake_up_interruptible(&dprx_wait_queue);
	} else if (event == DPRX_RX_AUDIO_MUTE) {
		s_dprx_cb_audio = DPRX_CBEVENT_AUDIO_UNLOCK;
		wake_up_interruptible(&dprx_wait_queue);
	} else if (event == DPRX_RX_AUDIO_UNMUTE) {
		s_dprx_cb_audio = DPRX_CBEVENT_AUDIO_LOCK;
		wake_up_interruptible(&dprx_wait_queue);
	} else if (event == DPRX_RX_PPS_CHANGE) {
		s_dprx_cb_video = DPRX_CBEVENT_PPS_CHANGE;
		wake_up_interruptible(&dprx_wait_queue);
	} else if (event == DPRX_RX_BW_CHANGE) {
		s_dprx_cb_bw = DPRX_CBEVENT_BW_CHANGE;
		wake_up_interruptible(&dprx_wait_queue);
	} else if (event == DPRX_RX_SPD_INFO_CHANGE) {
		s_dprx_cb_spd_info = DPRX_CBEVENT_SPD_INFO_CHANGE;
		wake_up_interruptible(&dprx_wait_queue);
	} else if (event == DPRX_RX_LINK_ERROR) {
		s_dprx_cb_link_error = DPRX_CBEVENT_LINK_ERROR;
		wake_up_interruptible(&dprx_wait_queue);
	}

	return 0;
}

struct dpcd_setting_table {
	u8 offset;
	u8 val;
};

static void dprx_dpcd_setting(void)
{
	int ret;
	u8 offset;
	u8 val;
	int i;

	static const struct dpcd_setting_table dpcd[] = {
#if defined(CONFIG_MTK_WRAPPER_FACTORY) || defined(CONFIG_MTK_WRAPPER_GAZE_DEBUG) || defined(CONFIG_MTK_WRAPPER_DEBUG)
		{ 0x61, 0x11 },	   /* DSC Version 1.1 */
		{ 0x62, 0x00 },	   /* RC Buffer Block Size */
		{ 0x64, 0x08 },	   /* DSC SLICE CAPABILITIES 1 */
		{ 0x69, 0x01 },	   /* DSC DECODER COLOR FORMAT CAPABILITIES */
		{ 0x6B, 0x01 },	   /* Peak DSC Throughput */
		{ 0x6C, 0x08 },	   /* DSC Maximum Slice Width */
#else
		{ 0x60, 0x00 },	   /* DSC Version 1.1 */
		{ 0x61, 0x00 },	   /* DSC Version 1.1 */
		{ 0x62, 0x00 },	   /* RC Buffer Block Size */
		{ 0x64, 0x00 },	   /* DSC SLICE CAPABILITIES 1 */
		{ 0x65, 0x00 },	   /* DSC LINE BUFFER BIT DEPTH */
		{ 0x66, 0x00 },	   /* DSC BLOCK PREDICTION SUPPORT */
		{ 0x69, 0x00 },	   /* DSC DECODER COLOR FORMAT CAPABILITIES */
		{ 0x6A, 0x00 },	   /* DSC COLOR DEPTH CAPABILITIES */
		{ 0x6B, 0x00 },	   /* Peak DSC Throughput */
		{ 0x6C, 0x00 },	   /* DSC Maximum Slice Width */
		{ 0x90, 0x00 },	   /* FEC */
#endif
	};
	for (i = 0; i < ARRAY_SIZE(dpcd); i++) {
		offset = dpcd[i].offset;
		val = dpcd[i].val;
		ret = dprx_set_dpcd_dsc_value(offset, val);
		if (ret != val) {
			pr_err("dpcd set failed(%08x)\n", offset);
		}
	}
}

static int dprx_power_on(bool hdcp2)
{
	s_dprx_cb_video = DPRX_CBEVENT_NONE;
	s_dprx_cb_audio = DPRX_CBEVENT_NONE;
	s_dprx_cb_bw = DPRX_CBEVENT_NONE;
	s_dprx_cb_spd_info = DPRX_CBEVENT_NONE;
	s_dprx_cb_link_error = DPRX_CBEVENT_NONE;

	dprx_set_callback(s_dprx, s_dprx, callback_dprx_event);
	mtk_dprx_power_on_phy(s_dprx);
	mtk_dprx_power_on(s_dprx);
	mtk_dprx_drv_init(s_dprx, hdcp2);
	dprx_set_hdcp1x_capability(false);
	dprx_set_lane_count(2);
	dprx_dpcd_setting();
	dprx_drv_edid_init(s_edid_sie_vrh, sizeof(s_edid_sie_vrh));
	mtk_dprx_phy_gce_init(s_dprx);
	mtk_dprx_drv_start();

	return 0;
}

static int dprx_power_off(void)
{
	int ret;

	ret = mtk_dprx_drv_stop();
	if (ret != 0) {
		pr_err("dprx stop error:%d\n", ret);
		return ret;
	}
	ret = mtk_dprx_drv_deinit(s_dprx);
	if (ret != 0) {
		pr_err("dprx deinit error:%d\n", ret);
		return ret;
	}
	ret = mtk_dprx_phy_gce_deinit(s_dprx);
	if (ret != 0) {
		pr_err("dprx phy gce deinit error:%d\n", ret);
		return ret;
	}
	ret = mtk_dprx_power_off(s_dprx);
	if (ret != 0) {
		pr_err("dprx power_off error:%d\n", ret);
		return ret;
	}
	ret = mtk_dprx_power_off_phy(s_dprx);
	if (ret != 0) {
		pr_err("dprx power_off_phy error:%d\n", ret);
		return ret;
	}

	return 0;
}

static int dprx_get_video_info(void *arg)
{
	struct dprx_video_info_s info = {0};
	dprx_video_info vinfo;
	int ret;

	ret = dprx_get_video_info_msa(&info);
	if (ret != 0) {
		pr_debug("Can't get video info\n");
		return -EFAULT;
	}

	vinfo.h_active = info.vid_timing_msa.h_active;
	vinfo.v_active = info.vid_timing_msa.v_active;
	vinfo.h_polarity = info.vid_timing_msa.h_polarity;
	vinfo.v_polarity = info.vid_timing_msa.v_polarity;
	vinfo.h_total    = info.vid_timing_msa.h_total;
	vinfo.v_total    = info.vid_timing_msa.v_total;
	vinfo.h_frontporch = info.vid_timing_msa.h_front_porch;
	vinfo.v_frontporch = info.vid_timing_msa.v_front_porch;
	vinfo.h_backporch = info.vid_timing_msa.h_back_porch;
	vinfo.v_backporch = info.vid_timing_msa.v_back_porch;
	vinfo.format = convert_format(info.vid_color_fmt);
	vinfo.bpc    = info.vid_color_depth;

	vinfo.frame_rate = info.frame_rate;

	vinfo.dsc_enable = (dprx_get_dsc_mode_status() == 0) ? false : true;

	ret = copy_to_user(arg, &vinfo, sizeof(dprx_video_info));
	if (ret != 0) {
		return -EFAULT;
	}
	return 0;
}

static int dprx_get_pps(void *user)
{
	struct pps_sdp_s p;
	args_dprx_pps args;
	int ret;

	ret = dprx_get_pps_info(&p);
	if (ret != 0) {
		return ret;
	}

	memcpy(args.pps, p.pps_db, sizeof(args.pps));

	ret = copy_to_user(user, &args, sizeof(args_dprx_pps));
	if (ret != 0) {
		return -EFAULT;
	}
	return 0;
}

static int dprx_get_link_status(void *user)
{
	args_dprx_link_status st;
	int tmp;
	int ret;
	int i;

	st.link_bandwidth = (dprx_get_dpcd_value(0x100) & 0xFF);
	st.lane_count     = (dprx_get_dpcd_value(0x101) & 0x1F);
	tmp = dprx_get_dpcd_value(0x202);
	st.lane_status[0] = (tmp & 0x0F);
	st.lane_status[1] = ((tmp >> 4) & 0x0F);
	tmp = dprx_get_dpcd_value(0x203);
	st.lane_status[2] = (tmp & 0x0F);
	st.lane_status[3] = ((tmp >> 4) & 0x0F);
	tmp = dprx_get_dpcd_value(0x107);
	st.spread_amp = ((tmp >> 4) & 0x01);

	for (i = 0; i < DPRX_WRAPPER_MAX_LANE_NUM; i++) {
		tmp = dprx_get_dpcd_value(0x103 + i);
		st.voltage_swing_level[i] = (tmp & 0x03);
		st.pre_emphasis_level[i] = ((tmp >> 3) & 0x03);
	}

	ret = copy_to_user(user, &st, sizeof(args_dprx_link_status));
	if (ret != 0) {
		return -EFAULT;
	}

	return 0;
}

static const unsigned int audio_infoframe_SF[] = {
	0, 32000, 44100, 48000, 88200, 96000, 176400, 192000
};

static const unsigned int audio_infoframe_SS[] = {
	0, 16, 20, 24
};

static const unsigned int audio_chsts_SF[] = {
	44100, 0, 48000, 32000, 22050, 0, 24000, 0,
	88200, 768000, 96000, 0, 176400, 0, 192000, 0
};

static const unsigned int audio_chsts_SS[] = {
	0, 0, 16, 20, 18, 22, 0, 0, 19, 23, 20, 24, 17, 21, 0, 0
};

static int dprx_get_audio_status(void *user)
{
	dprx_audio_status st;
	int ret;
	union audio_infoframe infoframe;
	union dprx_audio_chsts chsts;
	u32 mute_reason;

	ret = dprx_get_audio_info(infoframe.pktbyte.aud_db, AUDIO_INFOFRAME_LEN);
	if (ret != 0 || infoframe.info.aud_ch_cnt == 0) {
		st.audio_info.channels = 0;
		st.audio_info.coding_type = DPRX_AUDIO_TYPE_UNKNOWN;
		st.audio_info.rate = 0;
		st.audio_info.bit_width = 0;
		st.audio_info.coding_type_ext = DPRX_AUDIO_TYPE_EXT_UNKNOWN;
	} else {
		st.audio_info.channels = infoframe.info.aud_ch_cnt + 1;
		st.audio_info.coding_type = infoframe.info.aud_coding_type;
		st.audio_info.rate = audio_infoframe_SF[infoframe.info.sample_freq];
		st.audio_info.bit_width = audio_infoframe_SS[infoframe.info.sample_size];
		st.audio_info.coding_type_ext = infoframe.info.fmt_coding;
	}

	ret = dprx_get_audio_ch_status(chsts.aud_ch_sts, IEC_CH_STATUS_LEN);
	if (ret != 0 || chsts.iec_ch_sts.source_number == 0) {
		st.ch_sts.is_lpcm = 0;
		st.ch_sts.channels = 0;
		st.ch_sts.rate = 0;
		st.ch_sts.bit_width = 0;
	} else {
		st.ch_sts.is_lpcm = (chsts.iec_ch_sts.is_lpcm == 0);
		st.ch_sts.channels = chsts.iec_ch_sts.source_number;
		st.ch_sts.rate = audio_chsts_SF[chsts.iec_ch_sts.sampling_freq];
		st.ch_sts.bit_width = audio_chsts_SS[chsts.iec_ch_sts.word_len];
	}
	ret = dprx_drv_audio_get_mute_reason(&mute_reason);
	if (ret != 0) {
		st.mute_reason = 0;
	} else {
		st.mute_reason = mute_reason;
	}

	ret = copy_to_user(user, &st, sizeof(dprx_audio_status));
	if (ret != 0) {
		return -EFAULT;
	}

	return 0;
}

static const unsigned int convert_audio_clk_sel_to_rate[] = {
	0,
	32000,
	44100,
	48000,
	88200,
	96000,
	176400,
	192000,
};

static int dprx_get_audio_pll_information(void *user)
{
	int ret;
	struct dprx_audio_pll_info_s pll;
	dprx_audio_pll_info info;

	ret = dprx_get_audio_pll_info(&pll);
	if (ret != 0) {
		return -EFAULT;
	}

	info.m_aud =  pll.m_aud;
	info.n_aud =  pll.n_aud;
	info.link_speed =  pll.link_speed;
	info.audio_clk_sel =  pll.audio_clk_sel;

	if (pll.audio_clk_sel >= ARRAY_SIZE(convert_audio_clk_sel_to_rate)) {
		info.rate = 0;
	} else {
		info.rate = convert_audio_clk_sel_to_rate[pll.audio_clk_sel];
	}

	ret = copy_to_user(user, &info, sizeof(dprx_audio_pll_info));
	if (ret != 0) {
		return -EFAULT;
	}

	return 0;
}

static int dprx_get_hdcp_info(void *user)
{
	int ret;
	struct dprx_hdcp_status_s status;
	dprx_hdcp_info info;

	ret = dprx_get_hdcp_status(&status);
	if (ret != 0) {
		return -EFAULT;
	}
	info.hdcp13_auth_done = status.hdcp13_auth_done;
	info.hdcp13_encrypted = status.hdcp13_encrypted;
	info.hdcp22_auth_status = status.hdcp22_auth_status;
	info.hdcp22_encrypted = status.hdcp22_encrypted;

	ret = copy_to_user(user, &info, sizeof(dprx_hdcp_info));
	if (ret != 0) {
		return -EFAULT;
	}

	return 0;
}

static int dprx_get_phy_gce_info(void *user)
{
	int ret;
	dprx_phy_gce_info info;

	ret = dprx_get_phy_gce_status();

	if ((ret & JTOL_STATUS_MASK) == JTOL_ENABLED) {
		info.status = 1;
	} else if ((ret & JTOL_STATUS_MASK) == JTOL_DISABLED) {
		info.status = 0;
	}
	ret = copy_to_user(user, &info, sizeof(dprx_phy_gce_info));
	if (ret != 0) {
		return -EFAULT;
	}

	return 0;
}

static int dprx_phy_gce_init(void)
{
	int ret;

	ret = mtk_dprx_phy_gce_init(s_dprx);
	if (ret != 0) {
		return -EFAULT;
	}

	return 0;
}

static int dprx_phy_gce_deinit(void)
{
	int ret;

	ret = mtk_dprx_phy_gce_deinit(s_dprx);
	if (ret != 0) {
		return -EFAULT;
	}

	return 0;
}

static int dprx_get_spd_infoframe(void *user)
{
	int ret;
	dprx_spd_info info;

	ret = dprx_get_spd_info(info.buf, 27);
	if (ret != 0) {
		if (ret == 1) {
			return 1;
		} else {
			return -EFAULT;
		}
	}
	ret = copy_to_user(user, &info, sizeof(dprx_spd_info));
	if (ret != 0) {
		return -EFAULT;
	}

	return 0;
}

static int dprx_reset_fifo(void)
{
	dprx_if_fifo_reset();
	return 0;
}

static int dprx_release_fifo(void)
{
	dprx_if_fifo_release();
	return 0;
}

static int mtk_wrapper_dprx_open(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t mtk_wrapper_dprx_read(struct file *file, char *buff, size_t count, loff_t *pos)
{
	int ret;
	u32 event;

	if (count < sizeof(DPRX_CBEVENT)) {
		return -EINVAL;
	}

	ret = wait_event_interruptible(dprx_wait_queue,
				       s_dprx_cb_video != DPRX_CBEVENT_NONE ||
				       s_dprx_cb_audio != DPRX_CBEVENT_NONE ||
				       s_dprx_cb_bw != DPRX_CBEVENT_NONE ||
				       s_dprx_cb_spd_info != DPRX_CBEVENT_NONE ||
				       s_dprx_cb_link_error != DPRX_CBEVENT_NONE);

	if (ret < 0) {
		return -ERESTARTSYS;
	}
	if (s_dprx_cb_bw == DPRX_CBEVENT_BW_CHANGE) {
		event = s_dprx_cb_bw;
		s_dprx_cb_bw = DPRX_CBEVENT_NONE;
	} else if (s_dprx_cb_video != DPRX_CBEVENT_NONE) {
		event = s_dprx_cb_video;
		s_dprx_cb_video = DPRX_CBEVENT_NONE;
	} else if (s_dprx_cb_audio != DPRX_CBEVENT_NONE) {
		event = s_dprx_cb_audio;
		s_dprx_cb_audio = DPRX_CBEVENT_NONE;
	} else if (s_dprx_cb_spd_info == DPRX_CBEVENT_SPD_INFO_CHANGE) {
		event = s_dprx_cb_spd_info;
		s_dprx_cb_spd_info = DPRX_CBEVENT_NONE;
	} else if (s_dprx_cb_link_error == DPRX_CBEVENT_LINK_ERROR) {
		event = s_dprx_cb_link_error;
		s_dprx_cb_link_error = DPRX_CBEVENT_NONE;
	} else {
		return 0;
	}

	ret = copy_to_user(buff, &event, sizeof(event));
	if (ret <  0) {
		return -EFAULT;
	}

	return sizeof(event);
}

static long mtk_wrapper_dprx_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case DPRX_POWER_ON:
		dprx_power_on((void *)arg);
		break;
	case DPRX_POWER_OFF:
		dprx_power_off();
		break;
	case DPRX_GET_VIDEO_INFO:
		ret = dprx_get_video_info((void *)arg);
		break;
	case DPRX_GET_PPS:
		ret = dprx_get_pps((void *)arg);
		break;
	case DPRX_GET_LINK_STATUS:
		ret = dprx_get_link_status((void *)arg);
		break;
	case DPRX_GET_AUDIO_STATUS:
		ret = dprx_get_audio_status((void *)arg);
		break;
	case DPRX_GET_AUDIO_PLL_INFO:
		ret = dprx_get_audio_pll_information((void *)arg);
		break;
	case DPRX_GET_HDCP_INFO:
		ret = dprx_get_hdcp_info((void *)arg);
		break;
	case DPRX_GET_PHY_GCE_INFO:
		ret = dprx_get_phy_gce_info((void *)arg);
		break;
	case DPRX_PHY_GCE_INIT:
		ret = dprx_phy_gce_init();
		break;
	case DPRX_PHY_GCE_DEINIT:
		ret = dprx_phy_gce_deinit();
		break;
	case DPRX_GET_SPD_INFO:
		ret = dprx_get_spd_infoframe((void *)arg);
		break;
	case DPRX_RESET_FIFO:
		ret = dprx_reset_fifo();
		break;
	case DPRX_RELEASE_FIFO:
		ret = dprx_release_fifo();
		break;
	default:
		dev_err(s_dprx, "dprx_ioctl: unknown command(%d)\n", cmd);
		return -EINVAL;
	}

	return ret;
}

const static struct file_operations mtk_wrapper_dprx_fops = {
	.write          = NULL,
	.read           = mtk_wrapper_dprx_read,
	.unlocked_ioctl = mtk_wrapper_dprx_ioctl,
	.mmap           = NULL,
	.open           = mtk_wrapper_dprx_open,
	.release        = NULL,
};

int init_module_mtk_wrapper_dprx(void)
{
	int ret;

	pr_info("%s start\n", __func__);
	ret = register_chrdev(SCE_KM_CDEV_MAJOR_MTK_WRAPPER_DPRX, SCE_KM_DPRX_DEVICE_NAME, &mtk_wrapper_dprx_fops);
	if (ret < 0) {
		pr_err("log: alloc_chrdev(%s) failed(%d).\n", SCE_KM_DPRX_DEVICE_NAME, ret);
		return ret;
	}

	init_waitqueue_head(&dprx_wait_queue);
	ret = mtk_wrapper_display_pipe_probe("mediatek,dprx", 0, &s_dprx);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

void cleanup_module_mtk_wrapper_dprx(void)
{
	pr_info("%s start\n", __func__);
	unregister_chrdev(SCE_KM_CDEV_MAJOR_MTK_WRAPPER_DPRX, SCE_KM_DPRX_DEVICE_NAME);
}
