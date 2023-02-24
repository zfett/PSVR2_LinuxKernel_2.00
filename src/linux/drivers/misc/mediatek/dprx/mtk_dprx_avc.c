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
* @file mtk_dprx_avc.c
*
*  DPRX Video/Audio related function
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

#define AUDIO_UNMUTE_CNT (0x180 * 10)

struct video_stable_timer_s dprx_vid_stable_chk;
static u32 vid_m, vid_n;
u32 naud_162m[7] = {101250, 90000, 67500, 45000, 33750, 22500, 16875};
u32 naud_270m[7] = {168750, 150000, 112500, 75000, 56250, 37500, 28125};
u32 naud_540m[7] = {337500, 300000, 225000, 150000, 112500, 75000, 56250};
u32 naud_810m[7] = {506250, 450000, 337500, 225000, 168750, 112500, 84375};
u32 maud_0[2] = {10240, 12544};
u32 maud_1[2] = {10266, 12575};

/* ====================== Function ================================= */
/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     Get DPRX Video stable status
 * @par Parameters
 *     none
 * @return
 *     Video stable status\n
 *     0, Video is not stable\n
 *     1, Video is stable\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
bool mtk_dprx_is_video_stable(void)
{
	return MTK_DPRX_REG_READ_MASK(
		ADDR_VID_STABLE_DET, VID_STRM_STABLE_STATUS_FLDMASK);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     Get DPRX dsc status
 * @par Parameters
 *     none
 * @return
 *     Video dsc status\n
 *     0, Video is not in dsc mode\n
 *     1, Video is in dsc mode\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dprx_is_in_dsc_mode(void)
{
	int ret;
	u8 dsc_en;

	dsc_en = MTK_DPRX_REG_READ_MASK(ADDR_DSC_ENABLE, DSC_EN_FLDMASK);

	if (dsc_en == 0) {
		ret = 0;
	} else {
		ret = MTK_DPRX_REG_READ_MASK(ADDR_VB_ID_INFO,
					     VB_ID_FINAL_FLDMASK);
		ret = (ret & BIT_6) >> BIT_6_POS;
	}
	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX InfoFrame change interrupts enable
 * @par Parameters
 *     none
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprx_config_infoframe_change(void)
{
	pr_debug("InfoFrame change configure\n");

	MTK_DPRX_REG_WRITE_MASK(ADDR_INTR_MASK, 0, MAIN_LINK_INTR_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_MAIN_LINK_INTR0_MASK,
				0,
				HDR_INFO_CHANGE_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_MAIN_LINK_INTR0_MASK,
				0,
				AUDIO_INFO_CHANGE_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_MAIN_LINK_INTR0_MASK, 0, SPD_INFO_CHANGE_FLDMASK);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX VSC package change interrupts enable
 * @par Parameters
 *     none
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprx_config_vsc_pkg_change(void)
{
	pr_debug("InfoFrame change configure\n");

	MTK_DPRX_REG_WRITE_MASK(ADDR_INTR_MASK, 0, MAIN_LINK_INTR_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_MAIN_LINK_INTR0_MASK,
				0,
				VSC_PKG_CHANGE_FLDMASK);
}


/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Video related interrupts enable
 * @par Parameters
 *     none
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprx_config_video(void)
{
	pr_debug("Video stream configure\n");

	MTK_DPRX_REG_WRITE_MASK(ADDR_INTR_MASK, 0, MAIN_LINK_INTR_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_MAIN_LINK_INTR1_MASK, 0,
				MSA_UPDATE_INTR_FLDMASK);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Video PLL update by MSA packet
 * @par Parameters
 *     none
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprx_set_video_pll(void)
{
	u32 mvid = 0, nvid = 0, mvid_pll, link_speed = 0;
	u8 vpll_k, ref_clk_freq;
	u32 _u4_temp = 0;
	u32 video_freq;
	u32 htotal_msa;
	u32 htotal_ls_byte_num;

	ref_clk_freq = SYS_REF_CLK;

	mvid = MTK_DPRX_READ32(ADDR_VID_M_RPT_23_16);
	mvid = (mvid << BYTE1_POS) + MTK_DPRX_READ32(ADDR_VID_M_RPT_15_8);
	mvid = (mvid << BYTE1_POS) + MTK_DPRX_READ32(ADDR_VID_M_RPT_7_0);
	vid_m = mvid;
	pr_debug("Mvid: 0x%x = %d\n ", mvid, mvid);

	nvid = MTK_DPRX_READ32(ADDR_NVID_23_16);
	nvid = (nvid << BYTE1_POS) + MTK_DPRX_READ32(ADDR_NVID_15_8);
	nvid = (nvid << BYTE1_POS) + MTK_DPRX_READ32(ADDR_NVID_7_0);
	vid_n = nvid;
	pr_debug("Nvid: 0x%x = %d\n", nvid, nvid);
	htotal_msa = MTK_DPRX_READ32(ADDR_HTOTAL15_8_DBG);
	htotal_msa = (htotal_msa << BYTE1_POS) +
				MTK_DPRX_READ32(ADDR_HTOTAL7_0_DBG);
	pr_debug("htotal_msa: 0x%x = %d\n", htotal_msa, htotal_msa);

	link_speed =
	    MTK_DPRX_REG_READ_MASK(ADDR_LINK_BW_SET, LINK_BW_SET_FLDMASK);

	video_freq = (u32) (link_speed * LINK_RATE_UNIT * mvid) / (u32) nvid;
	pr_debug("video_freq: %d, link_speed =%d\n", video_freq, link_speed);

	/* video_int_config */
	pr_debug("Video int configure link_spd = %d\n", link_speed);
	htotal_ls_byte_num = (u32) (link_speed * htotal_msa / video_freq);

	mvid_pll = (u32) (video_freq * nvid / ref_clk_freq);
	pr_debug("mvid_pll= %d\n", mvid_pll);

	while (mvid_pll > 0xffffff) {
		mvid_pll = mvid_pll / 2;
		nvid = nvid / 2;
		pr_debug("modified mvid_pll = %d\n", mvid_pll);
		pr_debug("modified nvid= %d\n", nvid);
	}

	/* write video M value */
	MTK_DPRX_WRITE32(ADDR_M_FORCE_VALUE_3,
			 ((mvid_pll & BYTE_2) >> BYTE2_POS));
	MTK_DPRX_WRITE32(ADDR_M_FORCE_VALUE_2,
			 ((mvid_pll & BYTE_1) >> BYTE1_POS));
	MTK_DPRX_WRITE32(ADDR_M_FORCE_VALUE_1,
			 ((mvid_pll & BYTE_0) >> BYTE0_POS));
	/* write video N value */
	MTK_DPRX_WRITE32(ADDR_N_FORCE_VALUE_3, ((nvid & BYTE_2) >> BYTE2_POS));
	MTK_DPRX_WRITE32(ADDR_N_FORCE_VALUE_2, ((nvid & BYTE_1) >> BYTE1_POS));
	MTK_DPRX_WRITE32(ADDR_N_FORCE_VALUE_1, ((nvid & BYTE_0) >> BYTE0_POS));

	vpll_k = 0;
	_u4_temp = video_freq;

	if (_u4_temp >= VIDEO_PLL_TH_1)
		vpll_k = VIDEO_PLL_1;
	else if (_u4_temp >= VIDEO_PLL_TH_2)
		vpll_k = VIDEO_PLL_2;
	else if (_u4_temp >= VIDEO_PLL_TH_3)
		vpll_k = VIDEO_PLL_3;
	else if (_u4_temp >= VIDEO_PLL_TH_4)
		vpll_k = VIDEO_PLL_4;
	else if (_u4_temp >= VIDEO_PLL_TH_5)
		vpll_k = VIDEO_PLL_5;
	else if (_u4_temp >= VIDEO_PLL_TH_6)
		vpll_k = VIDEO_PLL_6;
	else
		pr_info("incorrect video frequence\n");

	/* Set post divider */
	MTK_DPRX_REG_WRITE_MASK(ADDR_VPLL_CTRL_0, vpll_k, VPLL_K_CNFG_FLDMASK);
	/* Set mn ready */
	MTK_DPRX_REG_WRITE_MASK(ADDR_MAIN_LINK_CTRL, 0, FORCE_MN_READY_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_MAIN_LINK_CTRL, 1, FORCE_MN_READY_FLDMASK);
	/* Enable PHY PLL_EN */
	MTK_DPRX_REG_WRITE_MASK(ADDR_VPLL_CTRL_1, 1, VID_PLL_EN_FLDMASK);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Audio related interrupts enable
 * @par Parameters
 *     none
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprx_config_audio(void)
{
	pr_debug("Function call dprx_config_audio\n");

	MTK_DPRX_REG_PHYD_WRITE_MASK(DPRX_AUD_MUTE_84,
				     AUDIO_UNMUTE_CNT,
				     RG_MAC_AUD_UNMUTE_CNT_SEL_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_INTR_MASK, 0, MAIN_LINK_INTR_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_MAIN_LINK_INTR1_MASK, 0,
				AUDIO_M_N_CHANGE_INT_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_AFC_SHIFT_B_H, 0x2, AFC_SHIFT_B2_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_AFC_SHIFT_B_H, 0x2, AFC_SHIFT_B3_FLDMASK);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Audio PLL update by MSA packet
 * @param[in]
 *     aud_work: struct work_struct*
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprx_audio_pll_config(struct work_struct *aud_work)
{
	u8 reg, link_speed, audio_clk_sel = 3, m2_value;
	u32 m_aud = 0, n_aud = 0, u4_temp, audio_freq_tmp;
	u32 audio_freq;
	u32 naud[7] = {0}, m_aud_f = maud_1[0], n_aud_f = naud_810m[2];
	u32 m_mod, maud_up = maud_0[0], maud_low = maud_1[0];
	u8 vbid4 = 0, vbid4_retry = 0, vbid4_sts, i;

	vbid4_retry = (MTK_DPRX_READ32(ADDR_VB_ID_INFO) & BIT_4) >> BIT_4_POS;
	vbid4_sts = MTK_DPRX_REG_READ_MASK(ADDR_AUD_INTR,
					   VBID_AUD_MUTE_INTR_FLDMASK);

	m2_value = MTK_DPRX_READ32(ADDR_DBG_MAUD_INI_23_16);
	m_aud |= m2_value;
	reg = MTK_DPRX_READ32(ADDR_DBG_MAUD_INI_15_8);
	m_aud = (m_aud << BYTE1_POS) | reg;
	reg = MTK_DPRX_READ32(ADDR_DBG_MAUD_INI_7_0);
	m_aud = (m_aud << BYTE1_POS) | reg;

	vbid4_retry |= (MTK_DPRX_READ32(ADDR_VB_ID_INFO) & BIT_4) >> BIT_4_POS;

	reg = MTK_DPRX_READ32(ADDR_DBG_NAUD_INI_23_16);
	n_aud |= reg;
	reg = MTK_DPRX_READ32(ADDR_DBG_NAUD_INI_15_8);
	n_aud = (n_aud << BYTE1_POS) | reg;
	reg = MTK_DPRX_READ32(ADDR_DBG_NAUD_INI_7_0);
	n_aud = (n_aud << BYTE1_POS) | reg;

	MTK_DPRX_REG_WRITE_MASK(ADDR_MAIN_LINK_INTR1_MASK,
				1,
				AUDIO_M_N_CHANGE_INT_FLDMASK);

	/* Maud/Naud = 512 * fs/f_LS_CLK from spec DP1.4 section 2.2.5.2 */
	/* Audio Fs can be calculated from above formula. */
	link_speed =
	    MTK_DPRX_REG_READ_MASK(ADDR_LINK_BW_SET, LINK_BW_SET_FLDMASK);
	pr_debug("m_aud = %d, n_aud = %d\n", m_aud, n_aud);

	if (link_speed == MTK_DPRX_LINK_RATE_HBR3)
		memcpy(naud, &naud_810m, sizeof(naud_810m));
	else if (link_speed == MTK_DPRX_LINK_RATE_HBR2)
		memcpy(naud, &naud_540m, sizeof(naud_540m));
	else if (link_speed == MTK_DPRX_LINK_RATE_HBR)
		memcpy(naud, &naud_270m, sizeof(naud_270m));
	else if (link_speed == MTK_DPRX_LINK_RATE_RBR)
		memcpy(naud, &naud_162m, sizeof(naud_162m));

	audio_freq = (u32) (link_speed * LINK_RATE_UNIT * 10 * m_aud);
	audio_freq = (u32) audio_freq / (u32) n_aud;
	u4_temp = audio_freq;
	pr_debug("audio_freq =  %d\n", u4_temp);
	audio_freq = (audio_freq * 100 + 256) / 512; /*256 for round up */
	u4_temp = audio_freq;
	pr_debug("audio_freq/512 =  %d\n", u4_temp);

	audio_freq_tmp = audio_freq;

	if (audio_freq_tmp > AUDIO_CLK_SEL_TH_7) {
		audio_clk_sel = AUDIO_CLK_SEL_7;
		maud_up = maud_0[0];
		maud_low = maud_1[0];
		n_aud_f = naud[6];
	} else if (audio_freq_tmp > AUDIO_CLK_SEL_TH_6) {
		audio_clk_sel = AUDIO_CLK_SEL_6;
		maud_up = maud_0[1];
		maud_low = maud_1[1];
		n_aud_f = naud[5];
	} else if (audio_freq_tmp > AUDIO_CLK_SEL_TH_5) {
		audio_clk_sel = AUDIO_CLK_SEL_5;
		maud_up = maud_0[0];
		maud_low = maud_1[0];
		n_aud_f = naud[4];
	} else if (audio_freq_tmp > AUDIO_CLK_SEL_TH_4) {
		audio_clk_sel = AUDIO_CLK_SEL_4;
		maud_up = maud_0[1];
		maud_low = maud_1[1];
		n_aud_f = naud[3];
	} else if (audio_freq_tmp > AUDIO_CLK_SEL_TH_3) {
		audio_clk_sel = AUDIO_CLK_SEL_3;
		maud_up = maud_0[0];
		maud_low = maud_1[0];
		n_aud_f = naud[2];
	} else if (audio_freq_tmp > AUDIO_CLK_SEL_TH_2) {
		audio_clk_sel = AUDIO_CLK_SEL_2;
		maud_up = maud_0[1];
		maud_low = maud_1[1];
		n_aud_f = naud[1];
	} else if (audio_freq_tmp > AUDIO_CLK_SEL_TH_1) {
		audio_clk_sel = AUDIO_CLK_SEL_1;
		maud_up = maud_0[0];
		maud_low = maud_1[0];
		n_aud_f = naud[0];
	}

	pr_debug("audio_freq: %d, link_speed =%d, audio_clk_sel = %d\n",
		 u4_temp, link_speed, audio_clk_sel);

	u4_temp = 0xffffffff / n_aud_f;
	m_mod = (n_aud_f * u4_temp / n_aud) * m_aud / u4_temp;
	if (m_mod < maud_up)
		m_aud_f = maud_up;
	else if (m_mod > maud_low)
		m_aud_f = maud_low;
	else
		m_aud_f = m_mod;

	pr_debug("m_aud_f =%d, n_aud_f = %d\n", m_aud_f, n_aud_f);

	if (m2_value == 0) {
		MTK_DPRX_REG_WRITE_MASK(ADDR_AUD_ACR_CTRL_1,
					1,
					NAUD_SEL_FLDMASK);
		reg = n_aud_f & BYTE_0;
		MTK_DPRX_WRITE32(ADDR_NAUD_SVAL_7_0, reg);
		reg = (n_aud_f & BYTE_1) >> BYTE1_POS;
		MTK_DPRX_WRITE32(ADDR_NAUD_SVAL_15_8, reg);
		reg = (n_aud_f & BYTE_2) >> BYTE2_POS;
		MTK_DPRX_WRITE32(ADDR_NAUD_SVAL_23_16, reg);
		MTK_DPRX_REG_WRITE_MASK(ADDR_AUD_ACR_CTRL_1,
					1,
					MAUD_SEL_FLDMASK);
		for (i = 0; i < 3; i++) {
			mdelay(1);
			vbid4_retry |=
				(MTK_DPRX_READ32(ADDR_VB_ID_INFO) & BIT_4) >>
				BIT_4_POS;
		}
	}

	if ((m2_value == 0) &&
	    ((MTK_DPRX_READ32(ADDR_VB_ID_INFO) & BIT_4) != BIT_4)) {
		reg = m_aud_f & BYTE_0;
		MTK_DPRX_WRITE32(ADDR_MAUD_SVAL_7_0, reg);
		reg = (m_aud_f & BYTE_1) >> BYTE1_POS;
		MTK_DPRX_WRITE32(ADDR_MAUD_SVAL_15_8, reg);
		reg = (m_aud_f & BYTE_2) >> BYTE2_POS;
		MTK_DPRX_WRITE32(ADDR_MAUD_SVAL_23_16, reg);
		vbid4 = 0;
	} else if (m2_value == 0) {
		m_aud = 0;
		vbid4 = 1;
		pr_debug("Audio Mute = 1 before set byte 2\n");
	}
	if ((m2_value == 0) &&
	    (vbid4 == 0) &&
	    ((MTK_DPRX_READ32(ADDR_VB_ID_INFO) & BIT_4) != BIT_4)) {
		for (i = 0; i < 6; i++) {
			mdelay(1);
			vbid4_retry |=
				(MTK_DPRX_READ32(ADDR_VB_ID_INFO) & BIT_4) >>
				BIT_4_POS;
		}
	} else if ((m2_value == 0) &&
		   (vbid4 == 0) &&
		   ((MTK_DPRX_READ32(ADDR_VB_ID_INFO) & BIT_4) == BIT_4)) {
		m_aud = 0;
		MTK_DPRX_REG_WRITE_MASK(ADDR_RST_CTRL_2,
					1,
					RST_AUDIOMNADJ_RESET_FLDMASK);
		MTK_DPRX_REG_WRITE_MASK(ADDR_RST_CTRL_2,
					1,
					RST_AUDIOPLL_RESET_FLDMASK);

		MTK_DPRX_REG_WRITE_MASK(ADDR_RST_CTRL_2,
					0,
					RST_AUDIOMNADJ_RESET_FLDMASK);
		MTK_DPRX_REG_WRITE_MASK(ADDR_RST_CTRL_2,
					0,
					RST_AUDIOPLL_RESET_FLDMASK);
		pr_debug("Audio Mute = 0 -> 1 after set byte 2\n");
	} else if (m2_value == 0) {
		m_aud = 0;
		pr_debug("Audio Mute = 1 -> 0 / 1 -> 1 after set byte 2\n");
	}

	/* configure audio clock select */
	MTK_DPRX_REG_WRITE_MASK(
		ADDR_APLL_CLK_SEL, audio_clk_sel, AUD_CLK_SEL_FLDMASK);
	/* release audio M/N adjust reset */
	MTK_DPRX_REG_WRITE_MASK(
		ADDR_RST_CTRL_2, 0, RST_AUDIOMNADJ_RESET_FLDMASK);
	/* Enable PHY PLL_EN */
	MTK_DPRX_REG_WRITE_MASK(ADDR_APLL_CTRL, 1, AUD_PLL_EN_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_RST_SEL_1, 0, AUDIOPLL_AUTO_RST_FLDMASK);

	vbid4_retry |= (MTK_DPRX_READ32(ADDR_VB_ID_INFO) & BIT_4) >> BIT_4_POS;
	if (!vbid4_sts) {
		vbid4_sts = MTK_DPRX_REG_READ_MASK(ADDR_AUD_INTR,
						   VBID_AUD_MUTE_INTR_FLDMASK);
		vbid4_retry |= vbid4_sts;
	}

	if (vbid4_retry) {
		m_aud = 0;
		pr_err("Retry for Audio Mute happened!!\n");
	}

	if (m2_value == 0) {
		MTK_DPRX_REG_WRITE_MASK(ADDR_MAIN_LINK_INTR1,
					1,
					AUDIO_M_N_CHANGE_INT_FLDMASK);
		reg = n_aud & BYTE_0;
		MTK_DPRX_WRITE32(ADDR_NAUD_SVAL_7_0, reg);
		reg = (n_aud  & BYTE_1) >> BYTE1_POS;
		MTK_DPRX_WRITE32(ADDR_NAUD_SVAL_15_8, reg);
		reg = (n_aud & BYTE_2) >> BYTE2_POS;
		MTK_DPRX_WRITE32(ADDR_NAUD_SVAL_23_16, reg);
		reg = m_aud & BYTE_0;
		MTK_DPRX_WRITE32(ADDR_MAUD_SVAL_7_0, reg);
		reg = (m_aud & BYTE_1) >> BYTE1_POS;
		MTK_DPRX_WRITE32(ADDR_MAUD_SVAL_15_8, reg);
		MTK_DPRX_WRITE32(ADDR_AUD_ACR_CTRL_1, 0);
	}
	MTK_DPRX_REG_WRITE_MASK(ADDR_MAIN_LINK_INTR1_MASK,
				0,
				AUDIO_M_N_CHANGE_INT_FLDMASK);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Video mute interrupt enable
 * @par Parameters
 *     none
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprx_config_video_mute_intr(void)
{
	pr_debug("Video mute intr configure\n");

	MTK_DPRX_REG_WRITE_MASK(ADDR_INTR_MASK_VID, 0, MAIN_LINK_INTR_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_MAIN_LINK_INTR1_MASK_VID, 0,
				VIDEO_MUTE_INTR_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_INTR_PLUG_MASK, 0,
				TX_UNPLUG_VMUTE_MASK_FLDMASK);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Video unmute interrupt enable
 * @par Parameters
 *     none
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprx_config_video_unmute_intr(void)
{
	pr_debug("Video unmute intr configure\n");

	MTK_DPRX_REG_WRITE_MASK(ADDR_INTR_MASK, 0, VIDEO_ON_INT_FLDMASK);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Video mute process if video mute interrupt occur
 * @par Parameters
 *     none
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprx_video_mute_process(void)
{
	pr_debug("Video mute process\n");
	dprx_video_sw_mute_enable_ctrl(ENABLE);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Video unmute process if video unmute interrupt occur
 * @par Parameters
 *     none
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprx_video_unmute_process(void)
{
	pr_debug("Video unmute process\n");
	dprx_video_sw_mute_enable_ctrl(DISABLE);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Video mute by SW control
 * @param[in]
 *     enable: Video mute SW control enable
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprx_video_sw_mute_enable_ctrl(bool enable)
{
	MTK_DPRX_REG_PHYD_WRITE_MASK(DPRX_INTCTL_40, enable,
				     RG_MAC_WRAP_VIDEO_MUTE_SW_FLDMASK);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Audio mute interrupt enable
 * @par Parameters
 *     none
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprx_config_audio_mute_intr(void)
{
	pr_debug("Audio mute intr configure\n");
	/* audio mute interrupt */
	MTK_DPRX_REG_WRITE_MASK(ADDR_AUD_INTR_MASK_AUD, 0,
				AUD_CH_STATUS_CHANGE_INT_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_AUD_INTR_MASK_AUD, 0,
				VBID_AUD_MUTE_INTR_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_AUD_INTR_MASK_AUD, 0,
				AUD_FIFO_UND_INT_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_AUD_INTR_MASK_AUD, 0,
				AUD_FIFO_OVER_INT_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_AUD_INTR_MASK_AUD, 0,
				AUD_LINKERR_INT_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_AUD_INTR_MASK_AUD, 0,
				AUD_DECERR_INT_FLDMASK);

	MTK_DPRX_REG_WRITE_MASK(ADDR_AUD_INTR_MASK, 0,
				AUD_CH_STATUS_CHANGE_INT_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_AUD_INTR_MASK, 0,
				VBID_AUD_MUTE_INTR_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_AUD_INTR_MASK, 0,
				AUD_FIFO_UND_INT_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_AUD_INTR_MASK, 0,
				AUD_FIFO_OVER_INT_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_AUD_INTR_MASK, 0,
				AUD_LINKERR_INT_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_AUD_INTR_MASK, 0,
				AUD_DECERR_INT_FLDMASK);

	MTK_DPRX_REG_WRITE_MASK(ADDR_INTR_MASK_AUD, 0, AUDIO_INTR_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_INTR_PLUG_MASK, 0,
				TX_UNPLUG_AMUTE_MASK_FLDMASK);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Audio mute interrupt disable
 * @par Parameters
 *     none
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprx_deconfig_audio_mute_intr(void)
{
	pr_debug("Audio mute intr deconfigure\n");
	/* audio mute interrupt disable */
	MTK_DPRX_WRITE32(ADDR_AUD_INTR_MASK_AUD, 0xff);
	MTK_DPRX_WRITE32(ADDR_AUD_INTR_MASK, 0xff);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Audio unmute interrupt enable
 * @par Parameters
 *     none
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprx_config_audio_unmute_intr(void)
{
	pr_debug("Audio unmute intr configure\n");

	MTK_DPRX_REG_WRITE_MASK(ADDR_AUD_ADD_INTR_MASK, 0,
				AUD_UNMUTE_INT_FLDMASK);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Audio mute process if audio mute interrupt occur
 * @par Parameters
 *     none
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprx_audio_mute_process(void)
{
	pr_debug("Audio mute process\n");
	dprx_audio_sw_mute_enable_ctrl(ENABLE);
	dprx_audio_unmute_cnt_enable_ctrl(DISABLE);
	dprx_audio_unmute_cnt_enable_ctrl(ENABLE);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Audio unmute process if audio unmute interrupt occur
 * @par Parameters
 *     none
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprx_audio_unmute_process(void)
{
	pr_debug("Audio unmute process\n");
	dprx_audio_unmute_cnt_enable_ctrl(DISABLE);
	dprx_audio_sw_mute_enable_ctrl(DISABLE);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Audio mute by SW control
 * @param[in]
 *     enable: Audio mute SW control enable
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprx_audio_sw_mute_enable_ctrl(bool enable)
{
	MTK_DPRX_REG_PHYD_WRITE_MASK(DPRX_INTCTL_40, enable,
				     RG_MAC_WRAP_AUDIO_MUTE_SW_FLDMASK);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Audio unmute count function enable
 * @param[in]
 *     enable: Audio unmute count control enable
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprx_audio_unmute_cnt_enable_ctrl(bool enable)
{
	MTK_DPRX_REG_PHYD_WRITE_MASK(DPRX_AUD_MUTE_84, enable,
				     RG_MAC_AUD_UNMUTE_CNT_EN_FLDMASK);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Video timing information
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
int dprx_get_video_timing_msa(void)
{
	u16 reg;
	u32 hvstart;
	int ret = 0;
	struct dprx_video_timing_s *msa_timing = &dp_vid_status.vid_timing_msa;

	reg = MTK_DPRX_READ32(ADDR_HTOTAL15_8_DBG);
	reg = (reg << BYTE1_POS) + MTK_DPRX_READ32(ADDR_HTOTAL7_0_DBG);
	msa_timing->h_total = reg;

	reg = MTK_DPRX_READ32(ADDR_HWIDTH15_8_DBG);
	reg = (reg << BYTE1_POS) + MTK_DPRX_READ32(ADDR_HWIDTH7_0_DBG);
	msa_timing->h_active = reg;

	reg = MTK_DPRX_READ32(ADDR_HSW15_8_DBG);
	reg = (reg << BYTE1_POS) + MTK_DPRX_READ32(ADDR_HSW7_0_DBG);
	msa_timing->h_sync_width = reg & (~BIT_15);
	msa_timing->h_polarity = (reg & BIT_15) >> BIT_15_POS;

	hvstart = MTK_DPRX_READ32(ADDR_HSTART15_8_DBG);
	hvstart = (hvstart << BYTE1_POS) + MTK_DPRX_READ32(ADDR_HSTART7_0_DBG);
	msa_timing->h_front_porch = msa_timing->h_total -
				    msa_timing->h_active -
				    hvstart;
	msa_timing->h_back_porch = hvstart - msa_timing->h_sync_width;

	reg = MTK_DPRX_READ32(ADDR_VTOTAL_15_8_DBG);
	reg = (reg << BYTE1_POS) + MTK_DPRX_READ32(ADDR_VTOTAL_7_0_DBG);
	msa_timing->v_total = reg;

	reg = MTK_DPRX_READ32(ADDR_VHEIGHT15_8_DBG);
	reg = (reg << BYTE1_POS) + MTK_DPRX_READ32(ADDR_VHEIGHT7_0_DBG);
	msa_timing->v_active = reg;

	reg = MTK_DPRX_READ32(ADDR_VSW15_8_DBG);
	reg = (reg << BYTE1_POS) + MTK_DPRX_READ32(ADDR_VSW7_0_DBG);
	msa_timing->v_sync_width = reg & (~BIT_15);
	msa_timing->v_polarity = (reg & BIT_15) >> BIT_15_POS;

	hvstart = MTK_DPRX_READ32(ADDR_VSTART15_8_DBG);
	hvstart = (hvstart << BYTE1_POS) + MTK_DPRX_READ32(ADDR_VSTART7_0_DBG);
	msa_timing->v_front_porch = msa_timing->v_total -
				    msa_timing->v_active -
				    hvstart;
	msa_timing->v_back_porch = hvstart - msa_timing->v_sync_width;

	pr_debug("Get Video timing from MSA\n");
	pr_debug("hTotal = %d\n", msa_timing->h_total);
	pr_debug("hActive = %d\n", msa_timing->h_active);
	pr_debug("hFrontPorch = %d\n", msa_timing->h_front_porch);
	pr_debug("vTotal = %d\n", msa_timing->v_total);
	pr_debug("vActive = %d\n", msa_timing->v_active);
	pr_debug("vFrontPorch = %d\n", msa_timing->v_front_porch);
	pr_debug("hSyncwidth =%d\n", msa_timing->h_sync_width);
	pr_debug("hPolarity= %d\n", msa_timing->h_polarity);
	pr_debug("vSyncwidth =%d\n", msa_timing->v_sync_width);
	pr_debug("vPolarity= %d\n", msa_timing->v_polarity);

	ret = dprx_get_video_frame_rate_msa();
	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX output video timing information
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
int dprx_get_video_timing_dpout(void)
{
	u16 reg;
	int ret = 0;
	struct dprx_video_timing_s *dpout_timing =
					&dp_vid_status.vid_timing_dpout;

	reg = MTK_DPRX_REG_READ_MASK(ADDR_H_RES_HIGH, H_RES_HIGH_FLDMASK);
	reg = (reg << BYTE1_POS) + MTK_DPRX_READ32(ADDR_H_RES_LOW);
	dpout_timing->h_total = reg;

	reg = MTK_DPRX_READ32(ADDR_ACT_PIX_HIGH);
	reg = (reg << BYTE1_POS) + MTK_DPRX_READ32(ADDR_ACT_PIX_LOW);
	dpout_timing->h_active = reg;

	reg = MTK_DPRX_REG_READ_MASK(ADDR_HSYNC_WIDTH_HIGH,
				     HSYNC_WIDTH_HIGH_FLDMASK);
	reg = (reg << BYTE1_POS) + MTK_DPRX_READ32(ADDR_HSYNC_WIDTH_LOW);
	dpout_timing->h_sync_width = reg;
	reg = MTK_DPRX_REG_READ_MASK(ADDR_SYNC_STATUS, HSYNC_POL_FLDMASK);
	dpout_timing->h_polarity = reg;

	reg = MTK_DPRX_READ32(ADDR_H_F_PORCH_HIGH);
	reg = (reg << BYTE1_POS) + MTK_DPRX_READ32(ADDR_H_F_PORCH_LOW);
	dpout_timing->h_front_porch = reg;
	dpout_timing->h_back_porch = dpout_timing->h_total -
				     dpout_timing->h_active -
				     dpout_timing->h_front_porch -
				     dpout_timing->h_sync_width;

	reg = MTK_DPRX_REG_READ_MASK(ADDR_V_RES_HIGH, V_RES_HIGH_FLDMASK);
	reg = (reg << BYTE1_POS) + MTK_DPRX_READ32(ADDR_V_RES_LOW);
	dpout_timing->v_total = reg;

	reg = MTK_DPRX_READ32(ADDR_ACT_LINE_HIGH);
	reg = (reg << BYTE1_POS) + MTK_DPRX_READ32(ADDR_ACT_LINE_LOW);
	dpout_timing->v_active = reg;

	reg = MTK_DPRX_READ32(ADDR_VSYNC_TO_ACT_LINE);
	dpout_timing->v_sync_width = reg;
	reg = MTK_DPRX_REG_READ_MASK(ADDR_SYNC_STATUS, VSYNC_POL_FLDMASK);
	dpout_timing->v_polarity = reg;

	reg = MTK_DPRX_READ32(ADDR_ACT_LINE_TO_VSYNC);
	dpout_timing->v_front_porch = reg;
	dpout_timing->v_back_porch = dpout_timing->v_total -
				     dpout_timing->v_active -
				     dpout_timing->v_front_porch -
				     dpout_timing->v_sync_width;

	pr_debug("Get Video timing for DPRX output\n");
	pr_debug("hTotal = %d\n", dpout_timing->h_total);
	pr_debug("hActive = %d\n", dpout_timing->h_active);
	pr_debug("hFrontPorch = %d\n", dpout_timing->h_front_porch);
	pr_debug("vTotal = %d\n", dpout_timing->v_total);
	pr_debug("vActive = %d\n", dpout_timing->v_active);
	pr_debug("vFrontPorch = %d\n", dpout_timing->v_front_porch);
	pr_debug("hSyncwidth =%d\n", dpout_timing->h_sync_width);
	pr_debug("hPolarity= %d\n", dpout_timing->h_polarity);
	pr_debug("vSyncwidth =%d\n", dpout_timing->v_sync_width);
	pr_debug("vPolarity= %d\n", dpout_timing->v_polarity);

	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Video frame rate information
 * @par Parameters
 *     none
 * @return
 *     0, function success.\n
 *     1, function fail.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_get_video_frame_rate_msa(void)
{
	u64 mvid = 0, nvid = 0, link_speed = 0;
	u64 video_fr;
	int ret = 0;
	u8 retry, f_global;
	u8 frame_rate=1;

	retry = 2;
	f_global = 0;

fps_retry:
	if (f_global == 0) {
		mvid = MTK_DPRX_READ32(ADDR_VID_M_RPT_23_16);
		mvid = (mvid << BYTE1_POS) +
			MTK_DPRX_READ32(ADDR_VID_M_RPT_15_8);
		mvid = (mvid << BYTE1_POS) +
			MTK_DPRX_READ32(ADDR_VID_M_RPT_7_0);
		nvid = MTK_DPRX_READ32(ADDR_NVID_23_16);
		nvid = (nvid << BYTE1_POS) + MTK_DPRX_READ32(ADDR_NVID_15_8);
		nvid = (nvid << BYTE1_POS) + MTK_DPRX_READ32(ADDR_NVID_7_0);
		f_global = 1;
	} else {
		mvid = vid_m;
		nvid = vid_n;
		f_global = 0;
	}

	link_speed =
	    MTK_DPRX_REG_READ_MASK(ADDR_LINK_BW_SET, LINK_BW_SET_FLDMASK);

	video_fr = (u64) (link_speed * LINK_RATE_UNIT * mvid * 100) /
				(u64) (dp_vid_status.vid_timing_msa.v_total);
	video_fr = (u64) (video_fr * 1000) /
				(u64) (dp_vid_status.vid_timing_msa.h_total);
	video_fr = (u64) (video_fr * 1000) / (u64) (nvid);

	if ((video_fr >= 11640) && (video_fr <= 12360))
		frame_rate = 120;
	else if ((video_fr >= 9850) && (video_fr <= 10150))
		frame_rate = 100;
	else if ((video_fr <= 6800) && (video_fr >= 6500))
		frame_rate = 67;
	else if ((video_fr <= 7100) && (video_fr >= 6900))
		frame_rate = 70;
	else if ((video_fr <= 7300) && (video_fr >= 7100))
		frame_rate = 72;
	else if ((video_fr <= 7600) && (video_fr >= 7400))
		frame_rate = 75;
	else if ((video_fr <= 8600) && (video_fr >= 8400))
		frame_rate = 85;
	else if ((video_fr <= 9150) && (video_fr >= 8850))
		frame_rate = 90;
	else if ((video_fr >= 5900) && (video_fr <= 6100))
		frame_rate = 60;
	else if ((video_fr <= 5700) && (video_fr >= 5500))
		frame_rate = 56;
	else if ((video_fr >= 4950) && (video_fr <= 5050))
		frame_rate = 50;
	else if ((video_fr >= 2350) && (video_fr <= 2450))
		frame_rate = 24;
	else if ((video_fr >= 2450) && (video_fr <= 2550))
		frame_rate = 25;
	else if ((video_fr >= 2950) && (video_fr <= 3050))
		frame_rate = 30;
	pr_debug("frame_rate = %d\n", frame_rate);

	if ((frame_rate == 1) & (retry == 0)) {
		ret = -EPERM;
	} else if (frame_rate == 1) {
		retry--;
		goto fps_retry;
	}

	dp_vid_status.frame_rate = frame_rate;

	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Video color format information
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
int dprx_get_video_format_msa(void)
{
	u32 rx_vsc, vsc_rev;
	u8 msa_misc1;
	char *colorformat = "Unknown";
	char *colordepth = "Unknown";
	char *yuvtype = "Unknown";

	msa_misc1 = MTK_DPRX_READ32(ADDR_MISC1_DBG);
	pr_debug(" Get color format  from SDP: MSAMisc1 = 0x%x\n", msa_misc1);

	if ((msa_misc1 & BIT_6) != 0) {
		pr_debug(" Get color format  from VSC SDP\n");
		rx_vsc = MTK_DPRX_REG_READ_MASK(
				ADDR_EXT_DPRX_FEATURE_ENUM_LIST,
				VSC_SDP_EXT_FOR_COLORIMETRY_SUPPORT_FLDMASK);
		vsc_rev = MTK_DPRX_REG_READ_MASK(
				ADDR_VSC_PKG_HB2, VSC_PKG_HB_REV_FLDMASK);
		pr_debug("VSC_Ext= 0x%x, VSC_Rev = 0x%x", rx_vsc, vsc_rev);
		if ((rx_vsc != 0) && (vsc_rev >= VSC_HB2_REV_CF))
			dprx_get_video_format_vsc();
	} else {
		pr_debug(" Get color format  from MSA\n");
		dprx_get_video_format_not_vsc();
	}

	if (dp_vid_status.vid_color_fmt  == RGB444)
		colorformat = "RGB444";
	else if (dp_vid_status.vid_color_fmt == YUV444)
		colorformat = "YCbCr444";
	else if (dp_vid_status.vid_color_fmt == YUV422)
		colorformat = "YCbCr422";
	else if (dp_vid_status.vid_color_fmt == YUV420)
		colorformat = "YCbCr420";
	else if (dp_vid_status.vid_color_fmt == YONLY)
		colorformat = "YOnly";
	else if (dp_vid_status.vid_color_fmt == RAW)
		colorformat = "RAW";

	if (dp_vid_status.vid_yuv_type == BT601)
		yuvtype = "BT601";
	else if (dp_vid_status.vid_yuv_type == BT709)
		yuvtype = "BT709";

	if (dp_vid_status.vid_color_depth == VID6BIT)
		colordepth = "6bpc";
	else if (dp_vid_status.vid_color_depth == VID7BIT)
		colordepth = "7bpc";
	else if (dp_vid_status.vid_color_depth == VID8BIT)
		colordepth = "8bpc";
	else if (dp_vid_status.vid_color_depth == VID10BIT)
		colordepth = "10bpc";
	else if (dp_vid_status.vid_color_depth == VID12BIT)
		colordepth = "12bpc";
	else if (dp_vid_status.vid_color_depth == VID14BIT)
		colordepth = "14bpc";
	else if (dp_vid_status.vid_color_depth == VID16BIT)
		colordepth = "16bpc";

	pr_debug("Get Video Color Format = %s, ", colorformat);
	pr_debug("Get Video YUV Type = %s,\n", yuvtype);
	pr_debug("Video Color Depth = %s\n", colordepth);

	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Video color format information (vsc mode)
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
void dprx_get_video_format_vsc(void)
{
	u8 db16, db17, db17_bpc, db16_type;

	db16 = MTK_DPRX_READ32(ADDR_VSC_PKG_DATA_135_128);
	db17 = MTK_DPRX_READ32(ADDR_VSC_PKG_DATA_143_136);

	switch (db16 >> BIT_4_POS) {
	case 0:
		dp_vid_status.vid_color_fmt = RGB444;
		break;
	case 1:
		dp_vid_status.vid_color_fmt = YUV444;
		break;
	case 2:
		dp_vid_status.vid_color_fmt = YUV422;
		break;
	case 3:
		dp_vid_status.vid_color_fmt = YUV420;
		break;
	case 4:
		dp_vid_status.vid_color_fmt = YONLY;
		break;
	case 5:
		dp_vid_status.vid_color_fmt = RAW;
		break;
	default:
		break;
	}

	db16_type = db16 & (BIT_3 | BIT_2 | BIT_1 | BIT_0);
	dp_vid_status.vid_yuv_type = NOT_DEFINED;
	if ((dp_vid_status.vid_color_fmt == YUV444) ||
	    (dp_vid_status.vid_color_fmt == YUV422) ||
	    (dp_vid_status.vid_color_fmt == YUV420)) {
		switch (db16_type) {
		case 0:
			dp_vid_status.vid_yuv_type = BT601;
			break;
		case 1:
			dp_vid_status.vid_yuv_type = BT709;
			break;
		default:
			break;
		}
	}

	db17_bpc = db17 & (BIT_0|BIT_1|BIT_2);
	if (dp_vid_status.vid_color_fmt == RGB444) {
		if (db17_bpc == 0)
			dp_vid_status.vid_color_depth = VID6BIT;
		else if (db17_bpc == 1)
			dp_vid_status.vid_color_depth = VID8BIT;
		else if (db17_bpc == 2)
			dp_vid_status.vid_color_depth = VID10BIT;
		else if (db17_bpc == 3)
			dp_vid_status.vid_color_depth = VID12BIT;
		else if (db17_bpc == 4)
			dp_vid_status.vid_color_depth = VID16BIT;
	} else if (dp_vid_status.vid_color_fmt == RAW) {
		if (db17_bpc == 1)
			dp_vid_status.vid_color_depth = VID6BIT;
		else if (db17_bpc == 2)
			dp_vid_status.vid_color_depth = VID7BIT;
		else if (db17_bpc == 3)
			dp_vid_status.vid_color_depth = VID8BIT;
		else if (db17_bpc == 4)
			dp_vid_status.vid_color_depth = VID10BIT;
		else if (db17_bpc == 5)
			dp_vid_status.vid_color_depth = VID12BIT;
		else if (db17_bpc == 6)
			dp_vid_status.vid_color_depth = VID14BIT;
		else if (db17_bpc == 7)
			dp_vid_status.vid_color_depth = VID16BIT;
	} else {
		if (db17_bpc == 1)
			dp_vid_status.vid_color_depth = VID8BIT;
		else if (db17_bpc == 2)
			dp_vid_status.vid_color_depth = VID10BIT;
		else if (db17_bpc == 3)
			dp_vid_status.vid_color_depth = VID12BIT;
		else if (db17_bpc == 4)
			dp_vid_status.vid_color_depth = VID16BIT;
	}
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Video color format information (non vsc mode)
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
void dprx_get_video_format_not_vsc(void)
{
	u8 msa_misc0, msa_misc1, depth, misc0_21, misc0_4;

	msa_misc0 = MTK_DPRX_READ32(ADDR_MISC0_DBG);
	msa_misc1 = MTK_DPRX_READ32(ADDR_MISC1_DBG);
	pr_debug(" Get color format  from SDP: MSAMisc0 = 0x%x\n", msa_misc0);

	depth = (msa_misc0 & (BIT_7|BIT_6|BIT_5)) >> BIT_5_POS;
	misc0_21 = (msa_misc0 & (BIT_2|BIT_1)) >> BIT_1_POS;
	misc0_4 = (msa_misc0 & BIT_4) >> BIT_4_POS;
	dp_vid_status.vid_yuv_type = NOT_DEFINED;

	if ((msa_misc1 & BIT_7) != 0) {
		if ((msa_misc0 & BIT_1) != 0) {
			dp_vid_status.vid_color_fmt = RAW;
			switch (depth) {
			case 1:
				dp_vid_status.vid_color_depth = VID6BIT;
				break;
			case 2:
				dp_vid_status.vid_color_depth = VID7BIT;
				break;
			case 3:
				dp_vid_status.vid_color_depth = VID8BIT;
				break;
			case 4:
				dp_vid_status.vid_color_depth = VID10BIT;
				break;
			case 5:
				dp_vid_status.vid_color_depth = VID12BIT;
				break;
			case 6:
				dp_vid_status.vid_color_depth = VID14BIT;
				break;
			case 7:
				dp_vid_status.vid_color_depth = VID16BIT;
				break;
			default:
				break;
			}
		} else {
			dp_vid_status.vid_color_fmt = YONLY;
			switch (depth) {
			case 1:
				dp_vid_status.vid_color_depth = VID8BIT;
				break;
			case 2:
				dp_vid_status.vid_color_depth = VID10BIT;
				break;
			case 3:
				dp_vid_status.vid_color_depth = VID12BIT;
				break;
			case 4:
				dp_vid_status.vid_color_depth = VID16BIT;
				break;
			default:
				break;
			}
		}
	} else {
		switch (misc0_21) {
		case 0:
			dp_vid_status.vid_color_fmt = RGB444;
			if (depth == 0)
				dp_vid_status.vid_color_depth = VID6BIT;
			else if (depth == 1)
				dp_vid_status.vid_color_depth = VID8BIT;
			else if (depth == 2)
				dp_vid_status.vid_color_depth = VID10BIT;
			else if (depth == 3)
				dp_vid_status.vid_color_depth = VID12BIT;
			else if (depth == 4)
				dp_vid_status.vid_color_depth = VID16BIT;
			break;
		case 1:
			dp_vid_status.vid_color_fmt = YUV422;
			if (misc0_4 == 0)
				dp_vid_status.vid_yuv_type = BT601;
			else
				dp_vid_status.vid_yuv_type = BT709;
			if (depth == 1)
				dp_vid_status.vid_color_depth = VID8BIT;
			else if (depth == 2)
				dp_vid_status.vid_color_depth = VID10BIT;
			else if (depth == 3)
				dp_vid_status.vid_color_depth = VID12BIT;
			else if (depth == 4)
				dp_vid_status.vid_color_depth = VID16BIT;
			break;
		case 2:
			dp_vid_status.vid_color_fmt = YUV444;
			if (misc0_4 == 0)
				dp_vid_status.vid_yuv_type = BT601;
			else
				dp_vid_status.vid_yuv_type = BT709;
			if (depth == 1)
				dp_vid_status.vid_color_depth = VID8BIT;
			else if (depth == 2)
				dp_vid_status.vid_color_depth = VID10BIT;
			else if (depth == 3)
				dp_vid_status.vid_color_depth = VID12BIT;
			else if (depth == 4)
				dp_vid_status.vid_color_depth = VID16BIT;
			break;
		default:
			break;
		}
	}
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX 1 frame delay function
 * @param[in]
 *     count: delay frame number
 * @return
 *     total delay time
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_1frame_delay(u8 count)
{
	int ret = 0;
	u32 delay;

	/*  1 frame delay time */
	ret = dprx_get_video_timing_msa();
	delay = (1000 / dp_vid_status.frame_rate) + 1;

	/*  count frame delay */
	mdelay(delay * count);

	ret = (int) (delay * count);
	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX video stable timer check function
 * @param[in]
 *     dprx: struct mtk_dprx *dprx
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprx_video_stable_check(struct mtk_dprx *dprx)
{
	u32 expire;

	del_timer_sync(&dprx_vid_stable_chk.timer);
	expire = dprx_1frame_delay(5);
	init_timer(&dprx_vid_stable_chk.timer);
	dprx_vid_stable_chk.index = 20;
	dprx_vid_stable_chk.dprx = dprx;
	dprx_vid_stable_chk.timer.expires = jiffies + msecs_to_jiffies(expire);
	dprx_vid_stable_chk.timer.data = 0;
	dprx_vid_stable_chk.timer.function = dprx_vid_stable_chk_cb;
	add_timer(&dprx_vid_stable_chk.timer);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX callback function for video stable timer check
 * @param[in]
 *     arg: unsigned long arg
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprx_vid_stable_chk_cb(unsigned long arg)
{
	if (dprx_vid_stable_chk.index <= 0) {
		atomic_set(&dprx_cb_sts.video_stable, 1);
		dprx_vid_stable_chk.vid_stable_sts = 0;
		dprx_drv_event_set(dprx_vid_stable_chk.dprx);
	} else if (mtk_dprx_is_video_stable()) {
		atomic_set(&dprx_cb_sts.video_stable, 1);
		dprx_vid_stable_chk.vid_stable_sts = 1;
		dprx_drv_event_set(dprx_vid_stable_chk.dprx);
	} else {
		dprx_vid_stable_chk.index--;
		mod_timer(&dprx_vid_stable_chk.timer, jiffies +
			  msecs_to_jiffies(dprx_1frame_delay(1)));
	}
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX video stable timer check function
 * @param[in]
 *     dprx: struct mtk_dprx *dprx
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprx_stereo_type_update(struct mtk_dprx *dprx)
{
	u8 vsc_stereo_type;
	u8 vsc_hb2, vsc_hb3;

	vsc_hb2 = MTK_DPRX_READ32(ADDR_VSC_PKG_HB2);
	vsc_hb3 = MTK_DPRX_READ32(ADDR_VSC_PKG_HB3);
	if ((vsc_hb2 != 0x01) && (vsc_hb2 != 0x05)) {
		pr_info("VSC HB2 is wrong: %d\n", vsc_hb2);
		return;
	} else if ((vsc_hb2 == 0x01) && (vsc_hb3 != 0x01)) {
		pr_info("VSC HB2 = 1, data length is wrong : %d\n", vsc_hb3);
		return;
	} else if ((vsc_hb2 == 0x05) && (vsc_hb3 != 0x13)) {
		pr_info("VSC HB2 = 5, data length is wrong : %d\n", vsc_hb3);
		return;
	}

	vsc_stereo_type = MTK_DPRX_READ32(ADDR_VSC_PKG_DATA_7_0);
	if (vsc_stereo_type != dp_vid_status.stereo_type) {
		dp_vid_status.stereo_type = vsc_stereo_type;
		atomic_set(&dprx_cb_sts.stereo_type_change, 1);
		dprx_drv_event_set(dprx);
		pr_debug("Stereo type changed: %d\n", vsc_stereo_type);
	} else {
		pr_debug("Stereo type not changed\n");
	}
}
