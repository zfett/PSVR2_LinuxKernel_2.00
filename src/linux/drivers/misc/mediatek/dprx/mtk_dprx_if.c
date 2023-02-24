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
 * @file mtk_dprx_if.c
 *
 *  DPRX API related function
 *
 */

/* include file  */
#include "mtk_dprx_os_int.h"
#include <soc/mediatek/mtk_dprx_info.h>
#include "mtk_dprx_drv_int.h"
#include "mtk_dprx_avc_int.h"
#include "mtk_dprx_isr_int.h"
#include "mtk_dprx_reg_int.h"
#include <soc/mediatek/mtk_dprx_if.h>

/* ====================== Global Structure ========================= */
struct dprx_video_info_s dp_vid_status;
/** @brief Some of the union of the declaration
 * @{
 */
union avi_infoframe dp_avi_info;
union dprx_audio_chsts dp_aud_ch_sts;
union audio_infoframe dp_aud_info;
union spd_infoframe dp_spd_info;
union mpeg_infoframe dp_mpeg_info;
/**
 * @}
 */
struct hdr_infoframe_s dp_hdr_info;
struct pps_sdp_s dp_pps_info;
struct dprx_lt_status_s dp_link_training_status;
struct dprx_hdcp_status_s dp_hdcp_status;

struct dprx_video_crc_s dp_vid_crc;

/* ====================== Function ================================= */

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX initial function
 * @param[in]
 *     dev: struct device
 * @param[in]
 *     hdcp2x: hdcp2x control enable/disable
 * @return
 *     0, function success.\n
 *     1, function fail.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     struct device *dev = NULL, return 1
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dprx_drv_init(struct device *dev, bool hdcp2x)
{
	struct mtk_dprx *dprx;
	int ret = 0;

	if (dev == NULL) {
		pr_info("[DPRX]mtk_dprx_drv_init fail\n");
		ret = 1;
		return ret;
	}

	dprx = dev_get_drvdata(dev);
	ret = mtk_dprx_hw_init(dprx, hdcp2x);
	return ret;
}
EXPORT_SYMBOL(mtk_dprx_drv_init);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX deinit function
 * @par Parameters
 *     none
 * @return
 *     0, function success.\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     struct device *dev = NULL, return 1
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dprx_drv_deinit(struct device *dev)
{
	struct mtk_dprx *dprx;
	int ret = 0;

	if (dev == NULL) {
		pr_info("[DPRX]mtk_dprx_drv_deinit fail\n");
		ret = 1;
		return ret;
	}

	dprx = dev_get_drvdata(dev);
	mtk_dprx_hw_deinit(dprx);
	return ret;
}
EXPORT_SYMBOL(mtk_dprx_drv_deinit);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX PHY GCE initial function
 * @par Parameters
 *     none
 * @return
 *     0, function success.\n
 *     1, function fail.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     struct device *dev = NULL, return 1
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dprx_phy_gce_init(struct device *dev)
{
	int ret = 0;

	if (dev == NULL) {
		pr_info("[DPRX]mtk_dprx_drv_init fail\n");
		ret = 1;
		return ret;
	}
#ifdef CONFIG_MTK_DPRX_PHY_GCE
	ret = mtk_dprx_gce_phy_create(dev);
#endif
	return ret;
}
EXPORT_SYMBOL(mtk_dprx_phy_gce_init);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX PHY GCE deinit function
 * @par Parameters
 *     none
 * @return
 *     0, function success.\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     struct device *dev = NULL, return 1
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dprx_phy_gce_deinit(struct device *dev)
{
	int ret = 0;

	if (dev == NULL) {
		pr_info("[DPRX]mtk_dprx_drv_deinit fail\n");
		ret = 1;
		return ret;
	}
#ifdef CONFIG_MTK_DPRX_PHY_GCE
	ret = mtk_dprx_gce_phy_destroy(dev);
#endif
	return ret;
}
EXPORT_SYMBOL(mtk_dprx_phy_gce_deinit);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX start function
 * @par Parameters
 *     none
 * @return
 *     0
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dprx_drv_start(void)
{
	dprx_drv_hdcp2x_enable();
	dprx_enable_irq();
	dprx_drv_sw_hpd_ready_ctrl(DISABLE);
	dprx_drv_sw_hpd_ready_ctrl(ENABLE);
	return 0;
}
EXPORT_SYMBOL(mtk_dprx_drv_start);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX stop function
 * @par Parameters
 *     none
 * @return
 *     0
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dprx_drv_stop(void)
{
	dprx_drv_sw_hpd_ready_ctrl(DISABLE);
	dprx_disable_irq();
	dprx_drv_hdcp2x_disable();
	return 0;
}
EXPORT_SYMBOL(mtk_dprx_drv_stop);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX power on function
 * @param[in]
 *	   dev: device.
 * @return
 *     0, function success.\n
 *     1, function fail.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     struct device *dev = NULL, return 1
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dprx_power_on(struct device *dev)
{
	struct mtk_dprx *dprx;
	int ret = 0;

	if (dev == NULL) {
		pr_info("[DPRX]mtk_dprx_power_on fail\n");
		ret = 1;
		return ret;
	}

	dprx = dev_get_drvdata(dev);
	ret = dprx_power_on_seq(dprx);
	return ret;
}
EXPORT_SYMBOL(mtk_dprx_power_on);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX power off function
 * @param[in]
 *	   dev: device.
 * @return
 *     0, function success.\n
 *     1, function fail.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     struct device *dev = NULL, return 1
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dprx_power_off(struct device *dev)
{
	struct mtk_dprx *dprx;
	int ret = 0;

	if (dev == NULL) {
		pr_info("[DPRX]mtk_dprx_power_off fail\n");
		ret = 1;
		return ret;
	}

	dprx = dev_get_drvdata(dev);
	ret = dprx_power_off_seq(dprx);
	return ret;
}
EXPORT_SYMBOL(mtk_dprx_power_off);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX phy power on function
 * @param[in]
 *	   dev: device.
 * @return
 *     0, function success.\n
 *     1, function fail.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     struct device *dev = NULL, return 1
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dprx_power_on_phy(struct device *dev)
{
	struct mtk_dprx *dprx;
	int ret = 0;

	if (dev == NULL) {
		pr_info("[DPRX]mtk_dprx_phy_power_on fail\n");
		ret = 1;
		return ret;
	}

	dprx = dev_get_drvdata(dev);
	ret = dprx_phy_power_on(dprx);
	return ret;
}
EXPORT_SYMBOL(mtk_dprx_power_on_phy);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX power off function
 * @param[in]
 *	   dev: device.
 * @return
 *     0, function success.\n
 *     1, function fail.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     struct device *dev = NULL, return 1
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dprx_power_off_phy(struct device *dev)
{
	struct mtk_dprx *dprx;
	int ret = 0;

	if (dev == NULL) {
		pr_info("[DPRX]mtk_dprx_power_off fail\n");
		ret = 1;
		return ret;
	}

	dprx = dev_get_drvdata(dev);
	dprx_phy_power_off(dprx);
	return ret;
}
EXPORT_SYMBOL(mtk_dprx_power_off_phy);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX get Video information from DP MSA Packet
 * @param[out]
 *     p: DP RX video data structure
 * @return
 *     0, function success.\n
 *     -EINVAL, invalid parameter.\n
 *     -EPERM, operation not permitted.\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     struct dprx_video_info_s *p = NULL, return -EINVAL
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_get_video_info_msa(struct dprx_video_info_s *p)
{
	int ret = 0;

	if (p == NULL) {
		pr_info("[DPRX]dprx_get_video_info_msa fail\n");
		ret = -EINVAL;
		return ret;
	}

	ret = dprx_get_video_timing_msa();
	ret = ret | dprx_get_video_format_msa();
	ret = ret | dprx_get_video_timing_dpout();
	if (ret == 0)
		memcpy(p, &dp_vid_status, sizeof(dp_vid_status));
	return ret;
}
EXPORT_SYMBOL(dprx_get_video_info_msa);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX get Audio InfoFrame information
 * @param[out]
 *     buf: Buffer array to store Audio InfoFrame information
 * @param[in]
 *     size: Buffer array size (AUDIO_INFOFRAME_LEN ~ 255)
 * @return
 *     0, function success.\n
 *     1, aud_info_rdy = 0\n
 *     -EINVAL, invalid parameter.\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     1. aud_info_rdy = 0, return 1
 *     2. buf = NULL, return -EINVAL
 *     3. size is not enough, return -EINVAL
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_get_audio_info(u8 *buf, u8 size)
{
	bool aud_info_rdy;
	int ret = 1, i;

	aud_info_rdy = MTK_DPRX_REG_READ_MASK(
			ADDR_INFORFRAM_READ_EN, AUDIO_INFO_READ_EN_FLDMASK);

	if (aud_info_rdy && (size >= AUDIO_INFOFRAME_LEN) && (buf != NULL)) {
		for (i = 0; i < AUDIO_INFOFRAME_LEN ; i++) {
			dp_aud_info.pktbyte.aud_db[i] =
				MTK_DPRX_READ32(ADDR_AUDIO_INFO_7_0 + (i * 4));
			pr_debug("aud_db[%d] = 0x%x\n", i,
				 dp_aud_info.pktbyte.aud_db[i]);
			memcpy(buf, dp_aud_info.pktbyte.aud_db,
			       AUDIO_INFOFRAME_LEN);
		}
		ret = 0;
	} else if (size < AUDIO_INFOFRAME_LEN) {
		pr_info("DP_AUDIO_INFO buffer size is not enough\n");
		ret = -EINVAL;
	} else if (buf == NULL) {
		pr_info("DP_AUDIO_INFO buffer is NULL\n");
		ret = -EINVAL;
	} else {
		pr_debug("Audio Infor not ready\n");
		ret = 1;
	}
	return ret;
}
EXPORT_SYMBOL(dprx_get_audio_info);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX get Audio Mute Reason information
 * @param[out]
 *     info: DP RX Audio Mute Reason
 *             0x80: AUD_CH_STATUS_CHANGE
 *             0x40: VBID_AUD_MUTE
 *             0x20: AUD_FIFO_UND
 *             0x10: AUD_FIFO_OVER
 *             0x08: AUD_LINKERR
 *             0x04: AUD_DECERR
 * @return
 *     0, function success.\n
 *     -EINVAL, invalid parameter.\n
 *     -EPERM, operation not permitted.\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     u32 *info = NULL, return -EINVAL
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_drv_audio_get_mute_reason(u32 *info)
{
	*info = MTK_DPRX_READ32(ADDR_AUD_INTR);
#ifdef DPRX_RESET_AUDIO_MUTE_REASON
	if (MTK_DPRX_READ32(ADDR_AUD_INTR_MASK) == AUD_INTR_MASK_FLDMASK) {
		MTK_DPRX_WRITE32(ADDR_INTR, *info);
	}
#endif
	return 0;
}
EXPORT_SYMBOL(dprx_drv_audio_get_mute_reason);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX get Audio Channel Status information
 * @param[out]
 *     buf: Buffer array to store Audio Channel Status information
 * @param[in]
 *     size: Buffer array size (IEC_CH_STATUS_LEN ~ 255)
 * @return
 *     0, function success.\n
 *     -EINVAL, invalid parameter.\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     1. buf = NULL, return -EINVAL
 *     2. size is not enough, return -EINVAL
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_get_audio_ch_status(u8 *buf, u8 size)
{
	int ret = 0;

	if (buf == NULL) {
		pr_info("DP_AUDIO_CH_STS buffer is NULL\n");
		ret = -EINVAL;
		return ret;
	}

	if (size >= IEC_CH_STATUS_LEN) {
		dp_aud_ch_sts.aud_ch_sts[0] =
				MTK_DPRX_READ32(ADDR_DBG_AUD_CS_0);
		dp_aud_ch_sts.aud_ch_sts[1] =
				MTK_DPRX_READ32(ADDR_DBG_AUD_CS_1);
		dp_aud_ch_sts.aud_ch_sts[2] =
				MTK_DPRX_READ32(ADDR_DBG_AUD_CS_2);
		dp_aud_ch_sts.aud_ch_sts[3] =
				MTK_DPRX_READ32(ADDR_DBG_AUD_CS_3);
		dp_aud_ch_sts.aud_ch_sts[4] =
				MTK_DPRX_READ32(ADDR_DBG_AUD_CS_4);
		memcpy(buf, dp_aud_ch_sts.aud_ch_sts, IEC_CH_STATUS_LEN);
		ret = 0;
	} else {
		pr_info("DP_AUDIO_CH_STS buffer size is not enough\n");
		ret = -EINVAL;
	}
	return ret;
}
EXPORT_SYMBOL(dprx_get_audio_ch_status);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX get Audio PLL status
 * @param[out]
 *     info: Audio PLL status
 * @return
 *     0, function success.\n
 *     -EINVAL, invalid parameter.\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_get_audio_pll_info(struct dprx_audio_pll_info_s *info)
{
	u8 reg;
	u8 link_speed;
	u8 audio_clk_sel = 1;
	u32 m_aud = 0, n_aud = 0;
	int ret = 0;

	if (info == NULL) {
		pr_info("[DPRX]dprx_get_audio_pll_info fail\n");
		ret = -EINVAL;
		return ret;
	}

	reg = MTK_DPRX_READ32(ADDR_DBG_MAUD_INI_23_16);
	m_aud |= reg;
	reg = MTK_DPRX_READ32(ADDR_DBG_MAUD_INI_15_8);
	m_aud = (m_aud << BYTE1_POS) | reg;
	reg = MTK_DPRX_READ32(ADDR_DBG_MAUD_INI_7_0);
	m_aud = (m_aud << BYTE1_POS) | reg;

	reg = MTK_DPRX_READ32(ADDR_DBG_NAUD_INI_23_16);
	n_aud |= reg;
	reg = MTK_DPRX_READ32(ADDR_DBG_NAUD_INI_15_8);
	n_aud = (n_aud << BYTE1_POS) | reg;
	reg = MTK_DPRX_READ32(ADDR_DBG_NAUD_INI_7_0);
	n_aud = (n_aud << BYTE1_POS) | reg;

	link_speed =
		MTK_DPRX_REG_READ_MASK(ADDR_LINK_BW_SET, LINK_BW_SET_FLDMASK);

	audio_clk_sel =
		MTK_DPRX_REG_READ_MASK(ADDR_APLL_CLK_SEL, AUD_CLK_SEL_FLDMASK);

	info->m_aud = m_aud;
	info->n_aud = n_aud;
	info->link_speed = link_speed;
	info->audio_clk_sel = audio_clk_sel;

	return ret;
}
EXPORT_SYMBOL(dprx_get_audio_pll_info);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX get AVI InfoFrame information
 * @param[out]
 *     buf: Buffer array to store AVI InfoFrame information
 * @param[in]
 *     size: Buffer array size ((AVI_HB_LEN + AVI_INFOFRAME_LEN) ~ 255)
 * @return
 *     0, function success.\n
 *     1, avi_info_rdy = 0.\n
 *     -EINVAL, invalid parameter.\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     1. avi_info_rdy = 0, return 1
 *     2. buf = NULL, return -EINVAL
 *     3. size is not enough, return -EINVAL
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_get_avi_info(u8 *buf, u8 size)
{
	bool avi_info_rdy;
	int ret = 0, i;

	avi_info_rdy = MTK_DPRX_REG_READ_MASK(
			ADDR_INFORFRAM_READ_EN, AVI_INFO_READ_EN_FLDMASK);

	if (avi_info_rdy &&
	    (size >= (AVI_HB_LEN + AVI_INFOFRAME_LEN)) &&
	    (buf != NULL)) {
		dp_avi_info.pktbyte.avi_hb[0] =
					MTK_DPRX_READ32(ADDR_AVI_INFO_7_0);
		dp_avi_info.pktbyte.avi_hb[1] =
					MTK_DPRX_READ32(ADDR_AVI_INFO_15_8);
		pr_debug("avi_hb[0] = 0x%x\n", dp_avi_info.pktbyte.avi_hb[0]);
		pr_debug("avi_hb[1] = 0x%x\n", dp_avi_info.pktbyte.avi_hb[1]);
		for (i = 0; i < AVI_INFOFRAME_LEN ; i++) {
			dp_avi_info.pktbyte.avi_db[i] =
				MTK_DPRX_READ32(ADDR_AVI_INFO_23_16 + (i * 4));
			pr_debug("avi_db[%d] = 0x%x\n", i,
				 dp_avi_info.pktbyte.avi_db[i]);
		}
		memcpy(buf, &(dp_avi_info.pktbyte),
		       (AVI_HB_LEN + AVI_INFOFRAME_LEN));
		ret = 0;
	} else if (size < (AVI_HB_LEN + AVI_INFOFRAME_LEN)) {
		pr_info("DP_AVI_INFO buffer size is not enough\n");
		ret = -EINVAL;
	} else if (buf == NULL) {
		pr_info("DP_AVI_INFO buffer is NULL\n");
		ret = -EINVAL;
	} else {
		pr_info("AVI Infor not ready\n");
		ret = 1;
	}
	return ret;
}
EXPORT_SYMBOL(dprx_get_avi_info);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX get MPEG InfoFrame information
 * @param[out]
 *     buf: Buffer array to store MPEG InfoFrame information
 * @param[in]
 *     size: Buffer array size ((MPEG_HB_LEN + MPEG_INFOFRAME_LEN) ~ 255)
 * @return
 *     0, function success.\n
 *     1, mpeg_info_rdy = 0.\n
 *     -EINVAL, invalid parameter.\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     1. mpeg_info_rdy = 0, return 1
 *     2. buf = NULL, return -EINVAL
 *     3. size is not enough, return -EINVAL
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_get_mpeg_info(u8 *buf, u8 size)
{
	bool mpeg_info_rdy;
	int ret = 0, i;

	mpeg_info_rdy = MTK_DPRX_REG_READ_MASK(
			ADDR_INFORFRAM_READ_EN, MPEG_INFO_READ_EN_FLDMASK);

	if (mpeg_info_rdy &&
	    (size >= (MPEG_HB_LEN + MPEG_INFOFRAME_LEN)) &&
	    (buf != NULL)) {
		dp_mpeg_info.pktbyte.mpg_hb[0] =
					MTK_DPRX_READ32(ADDR_MPEG_INFO_7_0);
		dp_mpeg_info.pktbyte.mpg_hb[1] =
					MTK_DPRX_READ32(ADDR_MPEG_INFO_15_8);
		for (i = 0; i < MPEG_INFOFRAME_LEN ; i++) {
			dp_mpeg_info.pktbyte.mpg_db[i] =
				MTK_DPRX_READ32(ADDR_MPEG_INFO_23_16 +
				(i * 4));
			pr_debug("mpg_db[%d] = 0x%x\n", i,
				 dp_mpeg_info.pktbyte.mpg_db[i]);
		}
		memcpy(buf, &(dp_mpeg_info.pktbyte),
		       (MPEG_HB_LEN + MPEG_INFOFRAME_LEN));
		ret = 0;
	} else if (size < (MPEG_HB_LEN + MPEG_INFOFRAME_LEN)) {
		pr_info("DP_MPEG_INFO buffer size is not enough\n");
		ret = -EINVAL;
	} else if (buf == NULL) {
		pr_info("DP_MPEG_INFO buffer is NULL\n");
		ret = -EINVAL;
	} else {
		pr_info("MEPG Info not ready\n");
		ret = 1;
	}
	return ret;
}
EXPORT_SYMBOL(dprx_get_mpeg_info);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX get HDR InfoFrame information
 * @param[out]
 *     p: HDR InfoFrame data structure
 * @return
 *     0, function success.\n
 *     1, hdr_info_rdy = 0.\n
 *     -EINVAL, invalid parameter.\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     1. hdr_info_rdy = 0, return 1
 *     2. struct hdr_infoframe_s *p = NULL, return -EINVAL
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_get_hdr_info(struct hdr_infoframe_s *p)
{
	bool hdr_info_rdy;
	int ret = 0, i;

	hdr_info_rdy = MTK_DPRX_REG_READ_MASK(
			ADDR_INFORFRAM_READ_EN, HDR_INFO_READ_EN_FLDMASK);

	if (hdr_info_rdy && (p != NULL)) {
		dp_hdr_info.hdr_hb[0] = MTK_DPRX_READ32(ADDR_HDR_INFO_7_0);
		dp_hdr_info.hdr_hb[1] = MTK_DPRX_READ32(ADDR_HDR_INFO_15_8);
		pr_debug("hdr_hb[0] = 0x%x\n", dp_hdr_info.hdr_hb[0]);
		pr_debug("hdr_hb[1] = 0x%x\n", dp_hdr_info.hdr_hb[1]);

		for (i = 0; i < HDR_INFOFRAME_LEN ; i++) {
			dp_hdr_info.hdr_db[i] =
				MTK_DPRX_READ32(ADDR_HDR_INFO_23_16 + (i * 4));
			pr_debug("hdr_db[%d] = 0x%x\n", i,
				 dp_hdr_info.hdr_db[i]);
		}
		memcpy(p, &dp_hdr_info, sizeof(dp_hdr_info));
		ret = 0;
	} else if (p == NULL) {
		pr_info("DP_HDR_INFO structure pointer is NULL\n");
		ret = -EINVAL;
	} else {
		pr_info("HDR Info not ready\n");
		ret = 1;
	}
	return ret;
}
EXPORT_SYMBOL(dprx_get_hdr_info);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX get SPD InfoFrame information
 * @param[out]
 *     buf: Buffer array to store SPD InfoFrame information
 * @param[in]
 *     size: Buffer array size ((SPD_HB_LEN + SPD_INFOFRAME_LEN) ~ 255)
 * @return
 *     0, function success.\n
 *     1, spd_info_rdy = 0.\n
 *     -EINVAL, invalid parameter.\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     1. spd_info_rdy = 0, return 1
 *     2. buf = NULL, return -EINVAL
 *     3. size is not enough, return -EINVAL
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_get_spd_info(u8 *buf, u8 size)
{
	bool spd_info_rdy;
	int ret = 1, i;

	spd_info_rdy = MTK_DPRX_REG_READ_MASK(
			ADDR_INFORFRAM_READ_EN, SPD_INFO_READ_EN_FLDMASK);

	if (spd_info_rdy &&
	    (size >= (SPD_HB_LEN + SPD_INFOFRAME_LEN)) &&
	    (buf != NULL)) {
		dp_spd_info.pktbyte.spd_hb[0] =
					MTK_DPRX_READ32(ADDR_SPD_INFO_7_0);
		dp_spd_info.pktbyte.spd_hb[1] =
					MTK_DPRX_READ32(ADDR_SPD_INFO_15_8);

		pr_debug("spd_hb[0] = 0x%x\n", dp_spd_info.pktbyte.spd_hb[0]);
		pr_debug("spd_hb[1] = 0x%x\n", dp_spd_info.pktbyte.spd_hb[1]);

		for (i = 0; i < SPD_INFOFRAME_LEN ; i++) {
			dp_spd_info.pktbyte.spd_db[i] =
				MTK_DPRX_READ32(ADDR_SPD_INFO_23_16 + (i * 4));
			pr_debug("spd_db[%d] = 0x%x\n", i,
				 dp_spd_info.pktbyte.spd_db[i]);
		}
		memcpy(buf, &(dp_spd_info.pktbyte),
		       (SPD_HB_LEN + SPD_INFOFRAME_LEN));
		ret = 0;
	} else if (size < (SPD_HB_LEN + SPD_INFOFRAME_LEN)) {
		pr_info("DP_SPD_INFO buffer size is not enough\n");
		ret = -EINVAL;
	} else if (buf == NULL) {
		pr_info("DP_SPD_INFO buffer is NULL\n");
		ret = -EINVAL;
	} else {
		pr_info("SPD Info not ready\n");
		ret = 1;
	}
	return ret;
}
EXPORT_SYMBOL(dprx_get_spd_info);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX get PPS information
 * @param[out]
 *     p: PPS buffer
 * @return
 *     0, function success.\n
 *     -EINVAL, invalid parameter.\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     struct pps_sdp_s *p = NULL, return  -EINVAL
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_get_pps_info(struct pps_sdp_s *p)
{
	int i, ret = 0;

	if (p == NULL) {
		pr_info("[DPRX]DP_PPS_INFO structure pointer is NULL\n");
		ret =  -EINVAL;
		return ret;
	}

	for (i = 0; i < PPS_SDP_LEN ; i++) {
		dp_pps_info.pps_db[i] = MTK_DPRX_READ32(
				ADDR_PPS_PKG_DATA_BYTE_0 + (i * 4));
	}
	memcpy(p, &dp_pps_info, sizeof(dp_pps_info));
	dp_vid_status.pps_change = 0;
	ret = 0;
	return ret;
}
EXPORT_SYMBOL(dprx_get_pps_info);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX EDID initial setting
 * @param[in]
 *     data: edid data array
 * @param[in]
 *     length: edid data length (should be 128, 256, 384, or 512)
 * @return
 *     0, pass\n
 *     1, fail\n
 *     -EINVAL, invalid parameter.\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     length > 512, return -EINVAL
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_drv_edid_init(u8 *data, u32 length)
{
	u32 edid_data = 0;
	u8 checksum = 0;
	u32 i = 0;

	pr_debug("dprx_drv_edid_init\n");
	if (data == NULL) {
		pr_info("dprx_drv edid table is NULL\n");
		return -EINVAL;
	}
	if (length > 512) {
		pr_info("dprx_drv edid table size err %d bytes\n", length);
		return -EINVAL;
	}

	for (i = 0; i < length; i++) {
		if (i == EDID_BLOCK_0) {
			checksum = 0x100 - checksum;
			pr_debug("block 0 checksum: 0x%x , %s\n", checksum,
				 checksum == data[i] ? "good" : "error");
			if (checksum != data[i])
				return 1;
			checksum = 0;
		} else if (i == EDID_BLOCK_1) {
			checksum = 0x100 - checksum;
			pr_debug("block 1 checksum: 0x%x , %s\n", checksum,
				 checksum == data[i] ? "good" : "error");
			if (checksum != data[i])
				return 1;
			checksum = 0;
		} else if (i == EDID_BLOCK_2) {
			checksum = 0x100 - checksum;
			pr_debug("block 2 checksum: 0x%x , %s\n", checksum,
				 checksum == data[i] ? "good" : "error");
			if (checksum != data[i])
				return 1;
			checksum = 0;
		} else if (i == EDID_BLOCK_3) {
			checksum = 0x100 - checksum;
			pr_debug("block 3 checksum: 0x%x, %s\n", checksum,
				 checksum == data[i] ? "good" : "error");
			if (checksum != data[i])
				return 1;
			checksum = 0;
		} else {
			checksum += data[i];
		}
	}

	/* Write EDID */
	length = length / sizeof(u32);
	for (i = 0; i < length; i++) {
		edid_data =
		    ((u32)data[4 * i + 3] << BYTE3_POS) +
		    ((u32)data[4 * i + 2] << BYTE2_POS) +
		    ((u32)data[4 * i + 1] << BYTE1_POS) +
		    ((u32)data[4 * i] << BYTE0_POS);
		pr_debug("[WR]addr: 0x%x , edid_data:0x%x\n", 0x20800000 + 4 * i,
			edid_data);
		mtk_dprx_sram_write32(4 * i, edid_data);
	}
	/* Verify EDID */
	for (i = 0; i < length; i++) {
		edid_data =
		    ((u32)data[4 * i + 3] << BYTE3_POS) +
		    ((u32)data[4 * i + 2] << BYTE2_POS) +
		    ((u32)data[4 * i + 1] << BYTE1_POS) +
		    ((u32)data[4 * i] << BYTE0_POS);
		if (mtk_dprx_sram_read32(4 * i) != edid_data) {
			pr_info
			    ("Verify Fail!");
			pr_info
			    ("[RD %d]addr+4*i: 0x%x , edid_data[i]:0x%x\n",
			     i, 0x20800000 + 4 * i,
			     mtk_dprx_sram_read32(4 * i));
			return 1;
		}
	}
	return 0;
}
EXPORT_SYMBOL(dprx_drv_edid_init);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX get DPCD value
 * @param[in]
 *     dpcd_offset: DPCD address (must comply with DP1.4 spec)
 *                        00000h~00090h
 *                        00100h~001C2h
 *                        00200h~00282h
 *                        00400h~0040Bh
 *                        02200h~02213h
 * @return
 *     DPCD value @ dpcd_offset, function pass.\n
 *     0x100, offset is in wrong range.\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     offset is in wrong range, return 0x100
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_get_dpcd_value(u32 dpcd_offset)
{
	int ret = 0;

	if ((dpcd_offset <= 0x90)  ||
	    ((dpcd_offset >= 0x100) && (dpcd_offset <= 0x1c2))  ||
	    ((dpcd_offset >= 0x200) && (dpcd_offset <= 0x282))  ||
	    ((dpcd_offset >= 0x400) && (dpcd_offset <= 0x40b))  ||
	    (dpcd_offset == 0x600)  ||
	    ((dpcd_offset >= 0x2200) && (dpcd_offset <= 0x2213))) {
		ret = MTK_DPRX_READ32(SLAVE_ID_01 + (dpcd_offset * 4));
		pr_debug("DPCD %5xh = 0x%x\n", dpcd_offset, ret);
	} else {
		pr_err("DPCD offset is in wrong range");
		ret = 0x100;
	}
	return ret;
}
EXPORT_SYMBOL(dprx_get_dpcd_value);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX set DPCD DSC related value (0x60~0x6F)
 * @param[in]
 *     dpcd_offset: DPCD address (dpcd_offset: 0x60~0x6F)
 * @param[in]
 *     value: set value for DPCD address "dpcd_offset" (must comply with spec)
 * @return
 *     DPCD value @ dpcd_offset. \n
 *     0x100, DPCD offset is not allowed to access.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     (dpcd_offset < 0x60) || (dpcd_offset > 0x6F), rerutn -EPERM
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_set_dpcd_dsc_value(u32 dpcd_offset, u8 value)
{
	int ret = 0x100;
	u32 value_set;

	if (dpcd_offset != 0x90 && ((dpcd_offset < 0x60) || (dpcd_offset > 0x6F))) {
		pr_err("DSC DPCD offset is in wrong range: 0x60~0x6F or 0x90");
		return ret;
	}

	value_set = (u32) value;
	pr_debug("value: 0x%x\n", value_set);
	MTK_DPRX_WRITE32((SLAVE_ID_01 + (dpcd_offset * 4)), value_set);
	return MTK_DPRX_READ32((SLAVE_ID_01 + (dpcd_offset * 4)));
}
EXPORT_SYMBOL(dprx_set_dpcd_dsc_value);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX get power status
 * @par Parameters
 *     none
 * @return
 *     Power Status\n
 *     1, D0 (normal operation mode)\n
 *     2, D3 (power-down mode)\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_get_power_status(void)
{
	return MTK_DPRX_REG_READ_MASK(ADDR_SET_POWER, SET_POWER_FLDMASK);
}
EXPORT_SYMBOL(dprx_get_power_status);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX set lane count
 * @param[in]
 *     lane_count: lane count (lane_count should be 1, 2, or 4)
 * @return
 *     0, Success\n
 *     -EINVAL, Lane count is not reasonable\n
 * @par Boundary case and Limitation
 *     Input parameter should be 1, 2, or 4
 * @par Error case and Error handling
 *     Input parameter is not 1, 2, or 4, return -EINVAL
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_set_lane_count(u8 lane_count)
{
	int ret;
	int extended_cap_field;

	if (!(lane_count == 1 || lane_count == 2 || lane_count == 4)) {
		ret = -EINVAL;
		pr_info("Lane count is not reasonable\n");
	} else {
		MTK_DPRX_REG_WRITE_MASK(ADDR_MAX_LANE_COUNT, lane_count,
						MAX_LANE_COUNT_FLDMASK);
		extended_cap_field = MTK_DPRX_REG_READ_MASK(
					ADDR_TRAINING_AUX_RD_INTERVAL,
					EXTENDED_RX_CAP_FIELD_PRESENT_FLDMASK);
		if (extended_cap_field)
			MTK_DPRX_REG_WRITE_MASK(ADDR_EXT_MAX_LANE_COUNT,
						lane_count,
						EXT_MAX_LANE_COUNT_FLDMASK);
		ret = 0;
		pr_debug("Set lane count is %d lane\n", lane_count);
	}
	return ret;
}
EXPORT_SYMBOL(dprx_set_lane_count);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX get PHY EQ setting
 * @param[out]
 *     p: link training stutas structure.
 * @return
 *     0, function pass.\n
 *     -EINVAL, invalid parameter.\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     struct dprx_lt_status_s *p = NULL, return  -EINVAL
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_get_phy_eq_setting(struct dprx_lt_status_s *p)
{
	if (p == NULL) {
		pr_info("[DPRX]dprx_get_phy_eq_setting fail\n");
		return  -EINVAL;
	}

	dp_link_training_status.l0_swing = MTK_DPRX_REG_READ_MASK(
		ADDR_TRAINING_LANE0_SET, DRIVE_CURRENT_SET_LANE0_FLDMASK);
	dp_link_training_status.l0_pre_emphasis = MTK_DPRX_REG_READ_MASK
		(ADDR_TRAINING_LANE0_SET, PRE_EMPHASIS_SET_LANE0_FLDMASK);
	dp_link_training_status.l1_swing = MTK_DPRX_REG_READ_MASK(
		ADDR_TRAINING_LANE1_SET, DRIVE_CURRENT_SET_LANE1_FLDMASK);
	dp_link_training_status.l1_pre_emphasis = MTK_DPRX_REG_READ_MASK
		(ADDR_TRAINING_LANE1_SET, PRE_EMPHASIS_SET_LANE1_FLDMASK);
	dp_link_training_status.l2_swing = MTK_DPRX_REG_READ_MASK(
		ADDR_TRAINING_LANE2_SET, DRIVE_CURRENT_SET_LANE2_FLDMASK);
	dp_link_training_status.l2_pre_emphasis = MTK_DPRX_REG_READ_MASK
		(ADDR_TRAINING_LANE2_SET, PRE_EMPHASIS_SET_LANE2_FLDMASK);
	dp_link_training_status.l3_swing = MTK_DPRX_REG_READ_MASK(
		ADDR_TRAINING_LANE3_SET, DRIVE_CURRENT_SET_LANE3_FLDMASK);
	dp_link_training_status.l3_pre_emphasis = MTK_DPRX_REG_READ_MASK
		(ADDR_TRAINING_LANE3_SET, PRE_EMPHASIS_SET_LANE3_FLDMASK);
	memcpy(p, &dp_link_training_status, sizeof(dp_link_training_status));
	return 0;
}
EXPORT_SYMBOL(dprx_get_phy_eq_setting);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX get HDCP status
 * @param[out]
 *     p: hdcp stutas structure.
 * @return
 *     0, function pass.\n
 *     -EINVAL, invalid parameter.\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     struct dprx_hdcp_status_s *p = NULL, return -EINVAL
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_get_hdcp_status(struct dprx_hdcp_status_s *p)
{
	if (p == NULL) {
		pr_info("[DPRX]dprx_get_hdcp_status fail\n");
		return -EINVAL;
	}

	dp_hdcp_status.hdcp13_auth_done = MTK_DPRX_REG_READ_MASK(
		ADDR_HDCP_STATUS, HDCP_REG_AUTH_STATUS_FLDMASK);
	dp_hdcp_status.hdcp13_encrypted = MTK_DPRX_REG_READ_MASK(
		ADDR_HDCP_STATUS, HDCP_REG_DECRYPT_FLDMASK);
	dp_hdcp_status.hdcp22_auth_status = MTK_DPRX_REG_READ_MASK(
		ADDR_HDCP2_DEBUG_0, HDCP2_DEBUG_0_FLDMASK) >> BIT_4_POS;
	dp_hdcp_status.hdcp22_encrypted = MTK_DPRX_REG_READ_MASK(
		ADDR_HDCP2_STATUS, ENCRYPT_STATUS_FLDMASK);

	pr_debug("dp_hdcp_status.hdcp13_auth_done = 0x%x\n",
		 dp_hdcp_status.hdcp13_auth_done);
	pr_debug("dp_hdcp_status.hdcp13_encrypted = 0x%x\n",
		 dp_hdcp_status.hdcp13_encrypted);
	pr_debug("dp_hdcp_status.hdcp22_auth_status = 0x%x\n",
		 dp_hdcp_status.hdcp22_auth_status);
	pr_debug("dp_hdcp_status.hdcp22_encrypted = 0x%x\n",
		 dp_hdcp_status.hdcp22_encrypted);

	memcpy(p, &dp_hdcp_status, sizeof(dp_hdcp_status));
	return 0;
}
EXPORT_SYMBOL(dprx_get_hdcp_status);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX get video PLL clock stable status
 * @par Parameters
 *     none
 * @return
 *     0, Video PLL clock is not stable\n
 *     1, Video PLL clock is stable\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_get_video_pll_status(void)
{
	return MTK_DPRX_REG_READ_MASK(ADDR_VPLL_STATUS,
		VPLL_DIV_START_FLDMASK);
}
EXPORT_SYMBOL(dprx_get_video_pll_status);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX get audio PLL clock stable status
 * @par Parameters
 *     none
 * @return
 *     0, Audio PLL clock is not stable\n
 *     1, Audio PLL clock is stable\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_get_audio_pll_status(void)
{
	return MTK_DPRX_REG_READ_MASK(ADDR_APLL_STATUS,
		APLL_DIV_START_FLDMASK);
}
EXPORT_SYMBOL(dprx_get_audio_pll_status);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX set callback function
 * @param[in]
 *	   dev: device.
 * @param[in]
 *	   ap_dev: AP device.
 * @param[in]
 *	   callback: call function.
 * @return
 *     0
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     1. struct device *dev = NULL, return 1
 *     2. struct device *ap_dev = NULL, return 1
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int  dprx_set_callback(struct device *dev,
	struct device *ap_dev,
	int (*callback)(struct device *dev,
		enum DPRX_NOTIFY_T event))
{
	struct mtk_dprx *dprx;
	int ret = 0;

	if (dev == NULL) {
		pr_info("[DPRX]dprx_set_callback dev is NULL pointer\n");
		ret = 1;
		return ret;
	}

	if (ap_dev == NULL) {
		pr_info("[DPRX]dprx_set_callback ap_dev is NULL pointer\\n");
		ret = 1;
		return ret;
	}

	dprx = dev_get_drvdata(dev);
	dprx->ap_dev = ap_dev;
	dprx->callback = callback;

	pr_debug("[DP RX]register callback, dev:%lx\n",
		 (unsigned long)dev);
	pr_debug("[DP RX]register callback, ap_dev:%lx\n",
		 (unsigned long)ap_dev);
	pr_debug("[DP RX]register callback, callback:%lx\n",
		 (unsigned long)callback);

	return ret;
}
EXPORT_SYMBOL(dprx_set_callback);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX get plug/unplug status
 * @par Parameters
 *     none
 * @return
 *     0, plug in\n
 *     1, unplug\n
 *     2, unknown status\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_get_unplug_status(void)
{
	return f_unplug;
}
EXPORT_SYMBOL(dprx_get_unplug_status);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX get video crc value
 * @par Parameters
 *     none
 * @return
 *     0, get crc successfully\n
 *     1, get crc fail\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_get_video_crc(void)
{
	int ret = 0, delay;
	u8 bcrc_count, crc_frame;
	u32 timeout;
	u32 crc_r_cr_out_7_0, crc_g_y_out_7_0, crc_b_cb_out_7_0;
	u32 crc_r_cr_out_15_8, crc_g_y_out_15_8, crc_b_cb_out_15_8;

	/*  1 frame delay */
	delay = dprx_1frame_delay(1);
	timeout = (u32) (delay * 10);

	/* enable check video crc function */
	MTK_DPRX_REG_WRITE_MASK(ADDR_TEST_SINK, 1, TEST_SINK_START_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_CHK_CRC_CTRL, 1, CRC_EN_FLDMASK);

	crc_frame = 5;
	do {
		bcrc_count = MTK_DPRX_REG_READ_MASK(ADDR_CHK_CRC_CTRL,
						    CRC_COUNT_FLDMASK);
		mdelay(1);
		timeout--;
	} while ((bcrc_count < crc_frame) && (timeout > 0));

	if (timeout <= 0) {
		pr_info("[DPRX]Video get crc timeout\n");
		ret = 1;
		return ret;
	}

	crc_r_cr_out_7_0 = MTK_DPRX_READ32(ADDR_CRC_R_CR_OUT_7_0);
	crc_r_cr_out_15_8 = MTK_DPRX_READ32(ADDR_CRC_R_CR_OUT_15_8);

	crc_g_y_out_7_0 = MTK_DPRX_READ32(ADDR_CRC_G_Y_OUT_7_0);
	crc_g_y_out_15_8 = MTK_DPRX_READ32(ADDR_CRC_G_Y_OUT_15_8);

	crc_b_cb_out_7_0 = MTK_DPRX_READ32(ADDR_CRC_B_CB_OUT_7_0);
	crc_b_cb_out_15_8 = MTK_DPRX_READ32(ADDR_CRC_B_CB_OUT_15_8);

	dp_vid_crc.lb_crc_r_cr = crc_r_cr_out_7_0;
	dp_vid_crc.hb_crc_r_cr = crc_r_cr_out_15_8;
	dp_vid_crc.lb_crc_g_y = crc_g_y_out_7_0;
	dp_vid_crc.hb_crc_g_y = crc_g_y_out_15_8;
	dp_vid_crc.lb_crc_b_cb = crc_b_cb_out_7_0;
	dp_vid_crc.hb_crc_b_cb = crc_b_cb_out_15_8;

	pr_debug("[DPRX]Video CRC R Cr Out = 0x%x : 0x%x\n",
		 crc_r_cr_out_7_0, crc_r_cr_out_15_8);
	pr_debug("[DPRX]Video CRC G Y  Out  = 0x%x : 0x%x\n",
		 crc_g_y_out_7_0, crc_g_y_out_15_8);
	pr_debug("[DPRX]Video CRC B Cb Out = 0x%x: 0x%x\n",
		 crc_b_cb_out_7_0, crc_b_cb_out_15_8);

	/* disable check video crc function */
	MTK_DPRX_REG_WRITE_MASK(ADDR_CHK_CRC_CTRL, 0, CRC_EN_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_TEST_SINK, 0, TEST_SINK_START_FLDMASK);

	return ret;
}
EXPORT_SYMBOL(dprx_get_video_crc);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX get video stable status
 * @par Parameters
 *     none
 * @return
 *     0, video is not stable\n
 *     1, video is stable\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
bool dprx_get_video_stable_status(void)
{
	return mtk_dprx_is_video_stable();
}
EXPORT_SYMBOL(dprx_get_video_stable_status);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX get dsc mode status
 * @par Parameters
 *     none
 * @return
 *     0, video is not in dsc mode\n
 *     1, video is in dsc mode\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_get_dsc_mode_status(void)
{
	return mtk_dprx_is_in_dsc_mode();
}
EXPORT_SYMBOL(dprx_get_dsc_mode_status);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX get stereo type
 * @par Parameters
 *     none
 * @return
 *     0x00, Non Stereo Video\n
 *     0x01, Frame/Field Sequential\n
 *     0x02, Stacked Frame\n
 *     0x03, Pixel Interleaved\n
 *     0x04, Side-by-side\n
 *     others, Reserved\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_get_stereo_type(void)
{
	return dp_vid_status.stereo_type;
}
EXPORT_SYMBOL(dprx_get_stereo_type);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX get phy gce status
 * @par Parameters
 *     none
 * @return
 *     1 byte value, definition is as following\n
 *     bit[3:0] = 0x03, non-HBR3 setting.\n
 *     bit[3:0] = 0x0c, HBR3 setting.\n
 *     bit[5:4] != 0, GCE setting in Link Training progress.\n
 *     bit[6] = 1, GCE is in enable status.\n
 *     bit[7] , reserved.\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_get_phy_gce_status(void)
{
	return MTK_DPRX_READ32(ADDR_RX_GTC_PHASE_SKEW_OFFSET_0);
}
EXPORT_SYMBOL(dprx_get_phy_gce_status);

int dprx_hdcp2x_disable(void)
{
	dprx_drv_hdcp2x_disable();

	return 0;
}
EXPORT_SYMBOL(dprx_hdcp2x_disable);

/** @ingroup IP_group_dprx_external_function
 * @par Description
 *     DPRX enable/disable HDCP1.x capability
 * @param[in]
 *     enable: enable/disable HDCP1.x capability
 * @return
 *     0, function pass.\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_set_hdcp1x_capability(bool enable)
{
	if (enable) {
		MTK_DPRX_REG_WRITE_MASK(ADDR_BCAPS,
					1,
					HDCP_CAPABLE_FLDMASK);
		MTK_DPRX_REG_WRITE_MASK(ADDR_PWD_SEL,
					0,
					HDCP1_PWD_SW_EN_FLDMASK);
	} else {
		MTK_DPRX_REG_WRITE_MASK(ADDR_PWD_SEL,
					1,
					HDCP1_PWD_SW_EN_FLDMASK);
		MTK_DPRX_REG_WRITE_MASK(ADDR_BCAPS,
					0,
					HDCP_CAPABLE_FLDMASK);
	}
	return 0;
}
EXPORT_SYMBOL(dprx_set_hdcp1x_capability);

void dprx_if_fifo_reset(void)
{
	mtk_dprx_phyd_write_mask(DPRX_AFIFO_44,
				 0,
				 RG_MAC_VID_WR_SW_RSTB_FLDMASK |
				 RG_MAC_VID_RD_SW_RSTB_FLDMASK |
				 RG_MAC_DSC_WR_SW_RSTB_FLDMASK |
				 RG_MAC_DSC_RD_SW_RSTB_FLDMASK,
				 0);

}
EXPORT_SYMBOL(dprx_if_fifo_reset);

void dprx_if_fifo_release(void)
{
	mtk_dprx_phyd_write_mask(DPRX_AFIFO_44,
				 0x1 << RG_MAC_VID_WR_SW_RSTB_FLDMASK_POS |
				 0x1 << RG_MAC_VID_RD_SW_RSTB_FLDMASK_POS |
				 0x1 << RG_MAC_DSC_WR_SW_RSTB_FLDMASK_POS |
				 0x1 << RG_MAC_DSC_RD_SW_RSTB_FLDMASK_POS,
				 RG_MAC_VID_WR_SW_RSTB_FLDMASK |
				 RG_MAC_VID_RD_SW_RSTB_FLDMASK |
				 RG_MAC_DSC_WR_SW_RSTB_FLDMASK |
				 RG_MAC_DSC_RD_SW_RSTB_FLDMASK,
				 0);
}
EXPORT_SYMBOL(dprx_if_fifo_release);

