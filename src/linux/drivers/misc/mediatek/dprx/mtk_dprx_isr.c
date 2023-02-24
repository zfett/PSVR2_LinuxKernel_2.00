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
* @file mtk_dprx_isr.c
*
*  DPRX isr related function
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

/* ====================== Global Variable ========================== */
u32 irq_count;
u8 f_unplug;
struct callback_sts_s dprx_cb_sts;
bool audio_mute_status = false;
struct completion phy_handle_cb;
static struct work_struct aud_work;

/* timeout waiting for the dp to respond */
#define PHY_CB_TIMEOUT (msecs_to_jiffies(100))
#define PHY_POLLING_TIME 98

/* ====================== Global Structure ========================= */
/** @ingroup IP_group_dprx_internal_struct
 * @brief DPRX main link interrupt 0 structure\n
 * 2 group: information structure and uint32_t value
 */
union main_link_intr0_s {
	struct {
		/* start with lsb, little endiam */
		/** Audio InfoFrame has changed */
		uint32_t audio_info_change:1;
		/** other kind of InfoFrame was received */
		uint32_t other_info_received:1;

		/** MPEG InfoFrame has changed */
		uint32_t mpeg_info_change:1;
		/** AVI InfoFrame has changed */
		uint32_t avi_info_change:1;
		/** SPD InfoFrame has changed */
		uint32_t spd_info_change:1;
		/** VSC Package has changed */
		uint32_t vsc_pkg_change:1;
		/** Audio M/N value is ready */
		uint32_t m_n_aud_ind:1;
		/** HDR InfoFrame has changed */
		uint32_t hdr_info_change:1;
		/* note: be ware of size, should be within 32-bit */
	} info;
	uint32_t value;
};

/** @ingroup IP_group_dprx_internal_struct
 * @brief DPRX main link interrupt 1 structure\n
 * 2 group: information structure and uint32_t value
 */
union main_link_intr1_s {
	struct {
		/* start with lsb, little endiam */
		/** main FIFO is underflow */
		uint32_t main_fifo_uf:1;
		/** main FIFO is overflow */
		uint32_t main_fifo_of:1;
		/** dummy */
		uint32_t dummy2:1;
		/** main stream attribute is lost */
		uint32_t main_lost:1;
		/** audio M/N value changed */
		uint32_t audio_mn_chg:1;
		/** audio channel number value changed */
		uint32_t audio_ch_cnt_chg:1;
		/** MSA info is updated */
		uint32_t msa_update_intr:1;
		/** video is muted */
		uint32_t video_mute_intr:1;
		/* note: be ware of size, should be within 32-bit */
	} info;
	uint32_t value;
};

/** @ingroup IP_group_dprx_internal_struct
 * @brief DPRX main link interrupt 3 structure\n
 * 2 group: information structure and uint32_t value
 */
union main_link_intr3_s {
	struct {
		/* start with lsb, little endiam */
		/** Audio ISRC change interrupt */
		uint32_t isrc_chg:1;
		/** Audio CopyManagement change interrupt */
		uint32_t aud_cm_chg:1;
		/** PPS SDP packet change interrupt */
		uint32_t pps_chg:1;
		/** dummy */
		uint32_t dummy1:1;
		/** DSC mode rising interrupt */
		uint32_t dsc_rs_intr:1;
		/** DSC mode falling interrupt */
		uint32_t dsc_fl_intr:1;
		/** dummy */
		uint32_t dummy2:1;
		/** dummy */
		uint32_t dummy3:1;
		/* note: be ware of size, should be within 32-bit */
	} info;
	uint32_t value;
};

/** @ingroup IP_group_dprx_internal_struct
 * @brief DPRX dpip structure\n
 * 2 group: information structure and uint32_t value
 */
union dpip_intr_s {
	struct {
		/* start with lsb, little endiam */
		/** dummy */
		uint32_t dummy0 : 1;
		/** dummy */
		uint32_t dummy1:1;
		/** dummy */
		uint32_t dummy2:1;
		/** dummy */
		uint32_t dummy3:1;
		/** align status unlock interrupt */
		uint32_t align_status_unlock:1;
		/** dummy */
		uint32_t dummy5:1;
		/** dummy */
		uint32_t dummy6:1;
		/** dummy */
		uint32_t dummy7:1;
		/* note: be ware of size, should be within 32-bit */
	} info;
	uint32_t value;
};

/** @ingroup IP_group_dprx_internal_struct
 * @brief DPRX audio structure\n
 * 2 group: information structure and uint32_t value
 */
union audio_intr_s {
	struct {
		/* start with lsb, little endiam */
		/** dummy */
		uint32_t dummy0:1;
		/** dummy */
		uint32_t dummy1:1;
		/** Audio rs decoder error interrupt */
		uint32_t aud_dec_err:1;
		/** Audio link error interrupt */
		uint32_t aud_link_err:1;
		/** Audio FIFO overflow interrupt */
		uint32_t aud_fifo_over:1;
		/** Audio FIFO underflow interrupt */
		uint32_t aud_fifo_und:1;
		/** VBID bit 4 (Audio Mute) has been set */
		uint32_t aud_vbid_mute:1;
		/** the audio channel status value changed */
		uint32_t aud_ch_status_change:1;
		/* note: be ware of size, should be within 32-bit */
	} info;
	uint32_t value;
};

/** @ingroup IP_group_dprx_internal_struct
 * @brief DPRX audio add structure\n
 * 2 group: information structure and uint32_t value
 */
union audio_add_intr_s {
	struct {
		/* start with lsb, little endiam */
		/** Audio unmute interrupt */
		uint32_t aud_unmute_intr:1;
		/** dummy */
		uint32_t dummy1:1;
		/** dummy */
		uint32_t dummy2:1;
		/** dummy */
		uint32_t dummy3:1;
		/** dummy */
		uint32_t dummy4:1;
		/** dummy */
		uint32_t dummy5:1;
		/** dummy */
		uint32_t dummy6:1;
		/** dummy */
		uint32_t dummy7:1;
		/* note: be ware of size, should be within 32-bit */
	} info;
	uint32_t value;
};

/** @ingroup IP_group_dprx_internal_struct
 * @brief DPRX DPCD 1 interrupt structure\n
 * 2 group: information structure and uint32_t value
 */
union dpcd_intr1_s {
	struct {
		/* start with lsb, little endiam */
		/** dummy */
		uint32_t dummy0:1;
		/** CR training interrupt */
		uint32_t cr_training:1;
		/** EQ training interrupt */
		uint32_t eq_training:1;
		/** dummy */
		uint32_t dummy3:1;
		/** dummy */
		uint32_t dummy4:1;
		/** dummy */
		uint32_t dummy5:1;
		/** BW change interrupt */
		uint32_t bw_change:1;
		/** dummy */
		uint32_t dummy7:1;
		/* note: be ware of size, should be within 32-bit */
	} info;
	uint32_t value;
};

/** @brief Some of the union of the declaration
 * @{
 */
static union main_link_intr0_s main_link_intr0;
static union main_link_intr1_s main_link_intr1;
static union main_link_intr3_s main_link_intr3;
static union dpip_intr_s dpip_intr;
static union audio_intr_s audio_intr;
static union audio_add_intr_s audio_add_intr;
static union dpcd_intr1_s dpcd_intr1;
/**
 * @}
 */

/* ====================== Function ================================= */
static void dprx_audio_mute_cb_process(struct mtk_dprx *dprx)
{
	dprx_audio_mute_process();
	if (audio_mute_status != true) {
		atomic_set(&dprx_cb_sts.audio_mute, 1);
		dprx_drv_event_set(dprx);
		audio_mute_status = true;
		dprx_deconfig_audio_mute_intr();
	}
}

static void dprx_audio_unmute_cb_process(struct mtk_dprx *dprx)
{
	dprx_audio_unmute_process();
	if (audio_mute_status != false) {
		atomic_set(&dprx_cb_sts.audio_unmute, 1);
		dprx_drv_event_set(dprx);
		audio_mute_status = false;
		dprx_config_audio_mute_intr();
	}
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX interrupt disable function
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
void dprx_disable_irq(void)
{
	MTK_DPRX_REG_PHYD_WRITE_MASK(DPRX_INTCTL_40,
				     1,
				     RG_MAC_WRAP_INT_HW_MASK_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_INTR_MASK, 0xff, INTR_MASK_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_SW_INTR_CTRL, 1, PLL_INT_MASK_FLDMASK);

	MTK_DPRX_REG_WRITE_MASK(ADDR_INTR_PLUG_MASK, 1,
				TX_PLUG_INT_MASK_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_INTR_PLUG_MASK, 1,
				TX_UNPLUG_INT_MASK_FLDMASK);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX interrupt enable function.\n
 *     Include video/audio normal interrupt and mute/unmute interrupt
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
void dprx_enable_irq(void)
{
	irq_count = 0;

	dprx_init_events();
	MTK_DPRX_REG_PHYD_WRITE_MASK(DPRX_INTCTL_40,
				     1,
				     RG_MAC_WRAP_INT_HW_MASK_FLDMASK);
	dprx_config_video();
	dprx_config_audio();
	dprx_config_video_mute_intr();
	dprx_config_video_unmute_intr();
	dprx_config_audio_mute_intr();
	dprx_config_audio_unmute_intr();
	dprx_config_infoframe_change();
	dprx_config_vsc_pkg_change();
	dprx_dpcd_int_config();

	MTK_DPRX_REG_WRITE_MASK(ADDR_INTR_MASK, 0, INTR_MASK_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_SW_INTR_CTRL, 0, PLL_INT_MASK_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_INTR_PLUG_MASK, 0,
				TX_PLUG_INT_MASK_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_INTR_PLUG_MASK, 0,
				TX_UNPLUG_INT_MASK_FLDMASK);
	MTK_DPRX_REG_PHYD_WRITE_MASK(DPRX_INTCTL_40,
				     0,
				     RG_MAC_WRAP_INT_HW_MASK_FLDMASK);
	INIT_WORK(&aud_work, dprx_audio_pll_config);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX top layer's interrupt - main link interrupt update function
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
void dprx_drv_mainlink_intr_update(struct mtk_dprx *dprx)
{
	u32 _mainlink_intr0_status = 0;
	u32 _mainlink_intr1_status = 0;
	u32 _mainlink_intr2_status = 0;
	u32 _mainlink_intr3_status = 0;

	_mainlink_intr0_status =
		MTK_DPRX_READ32(ADDR_MAIN_LINK_INTR0) | main_link_intr0.value;
	main_link_intr0.value = _mainlink_intr0_status;
	_mainlink_intr1_status =
		MTK_DPRX_READ32(ADDR_MAIN_LINK_INTR1) | main_link_intr1.value;
	main_link_intr1.value = _mainlink_intr1_status;
	_mainlink_intr2_status = MTK_DPRX_READ32(ADDR_MAIN_LINK_INTR2);
	_mainlink_intr3_status =
		MTK_DPRX_READ32(ADDR_MAIN_LINK_INTR3) | main_link_intr3.value;
	main_link_intr3.value = _mainlink_intr3_status;

	MTK_DPRX_WRITE32(ADDR_MAIN_LINK_INTR0, _mainlink_intr0_status);
	MTK_DPRX_WRITE32(ADDR_MAIN_LINK_INTR1,
			 (_mainlink_intr1_status & (~BIT_7)));
	MTK_DPRX_WRITE32(ADDR_MAIN_LINK_INTR2, _mainlink_intr2_status);
	MTK_DPRX_WRITE32(ADDR_MAIN_LINK_INTR3, _mainlink_intr3_status);

	pr_debug("mainlink intr 0 update: main_link_intr0: 0x%x:,0x%x\n",
		 ADDR_MAIN_LINK_INTR0, _mainlink_intr0_status);
	pr_debug("mainlink intr 1 update: main_link_intr1: 0x%x:,0x%x\n",
		 ADDR_MAIN_LINK_INTR1, _mainlink_intr1_status);
	pr_debug("mainlink intr 3 update: main_link_intr3: 0x%x:,0x%x\n",
		 ADDR_MAIN_LINK_INTR3, _mainlink_intr3_status);

	if (dprx_drv_is_video_mute_event()) {	/* Video mute process */
		dprx_video_mute_process();
		dprx_drv_clr_video_mute_event();
		atomic_set(&dprx_cb_sts.video_mute, 1);
		dprx_drv_event_set(dprx);
	}
	MTK_DPRX_WRITE32(ADDR_MAIN_LINK_INTR1,
			 (_mainlink_intr1_status & BIT_7));
	if (dprx_drv_is_msa_change_event()) {
		dprx_set_video_pll();
		dprx_drv_clr_msa_change_event();
		atomic_set(&dprx_cb_sts.msa_change, 1);
		dprx_drv_event_set(dprx);
	}
	if (dprx_drv_is_maud_change_event()) {
		schedule_work(&aud_work);
		dprx_drv_clr_maud_change_event();
		atomic_set(&dprx_cb_sts.aud_mn_change, 1);
		dprx_drv_event_set(dprx);
	}
	if (dprx_drv_is_compress_fm_rs_event()) {
		dp_vid_status.dsc_mode = 1;
		dprx_drv_clr_compress_fm_rs_event();
		atomic_set(&dprx_cb_sts.dsc_change, 1);
		dprx_drv_event_set(dprx);
	}
	if (dprx_drv_is_compress_fm_fl_event()) {
		dp_vid_status.dsc_mode = 0;
		dprx_drv_clr_compress_fm_fl_event();
		atomic_set(&dprx_cb_sts.dsc_change, 1);
		dprx_drv_event_set(dprx);
	}
	if (dprx_drv_is_pps_change_event()) {
		dp_vid_status.pps_change = 1;
		dprx_drv_clr_pps_change_event();
		atomic_set(&dprx_cb_sts.pps_change, 1);
		dprx_drv_event_set(dprx);
	}
	if (dprx_drv_is_hdr_info_change_event()) {
		atomic_set(&dprx_cb_sts.hdr_info_change, 1);
		dprx_drv_clr_hdr_info_change_event();
		dprx_drv_event_set(dprx);
	}
	if (dprx_drv_is_vsc_pkg_change_event()) {
		dprx_drv_clr_vsc_pkg_change_event();
		dprx_stereo_type_update(dprx);
	}
	if (dprx_drv_is_audio_info_change_event()) {
		atomic_set(&dprx_cb_sts.audio_info_change, 1);
		dprx_drv_clr_audio_info_change_event();
		dprx_drv_event_set(dprx);
	}
	if (dprx_drv_is_spd_info_change_event()) {
		atomic_set(&dprx_cb_sts.spd_info_change, 1);
		dprx_drv_clr_spd_info_change_event();
		dprx_drv_event_set(dprx);
	}
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX top layer's interrupt - dpip interrupt update function
 * @param[in]
 *     dprx: DPRX device structure
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprx_drv_dpip_intr_update(struct mtk_dprx *dprx)
{
	u32 _dpip_status  = 0;

	_dpip_status = MTK_DPRX_READ32(ADDR_DPIP_INTR) | dpip_intr.value;
	dpip_intr.value = _dpip_status;

	MTK_DPRX_WRITE32(ADDR_DPIP_INTR, _dpip_status);
	pr_debug("dpip intr update: dpipIntr: 0x%x:,0x%x\n",
		 ADDR_DPIP_INTR, _dpip_status);

	if (dprx_drv_is_align_status_unlock_event()) {
		dprx_align_status_unlock_intr_enable(0);
		pr_debug("Link error happened.\n");
		atomic_set(&dprx_cb_sts.link_error, 1);
		dprx_drv_event_set(dprx);
		dprx_drv_clr_align_status_unlock_event();
	}
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX top layer's interrupt - dpcd interrupt update function
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
void dprx_drv_dpcd_intr_update(struct mtk_dprx *dprx)
{
	u32 _dpcd1_status = 0;

	_dpcd1_status = MTK_DPRX_READ32(ADDR_DPCD_INT_1) | dpcd_intr1.value;
	dpcd_intr1.value = _dpcd1_status;

	MTK_DPRX_WRITE32(ADDR_DPCD_INT_1, _dpcd1_status);

	pr_debug("dpcd intr update: dpcdIntr1: 0x%x:,0x%x\n",
		 ADDR_DPCD_INT_1, _dpcd1_status);

	if (dprx_drv_is_bw_change_event()) {
		pr_debug("Link rate is changed.\n");
		atomic_set(&dprx_cb_sts.bw_change, 1);
		dprx_drv_event_set(dprx);
		dprx_drv_clr_bw_change_event();
	}
	if (dprx_drv_is_cr_training_event()) {
		pr_debug("Link Training - CR is performed.\n");
		dprx_drv_clr_cr_training_event();
	}
	if (dprx_drv_is_eq_training_event()) {
		pr_debug("Link Training - EQ is performed.\n");
		dprx_drv_clr_eq_training_event();
	}
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX top layer's interrupt - audio interrupt update function
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
void dprx_drv_audio_intr_update(struct mtk_dprx *dprx)
{
	u32 _audio_status = 0;
	u32 _audio_unmute_status = 0;

	_audio_status = MTK_DPRX_READ32(ADDR_AUD_INTR) | audio_intr.value;
	_audio_unmute_status =
	    MTK_DPRX_READ32(ADDR_AUD_ADD_INTR) | audio_add_intr.value;
	audio_intr.value = _audio_status;
	audio_add_intr.value = _audio_unmute_status;

	pr_debug("aud intr update: audio intr: 0x%x:,0x%x\n",
		 ADDR_AUD_INTR, _audio_status);
	pr_debug("aud intr update: audio intr: 0x%x:,0x%x\n",
		 ADDR_AUD_ADD_INTR, _audio_unmute_status);

	/* Audio mute process */
	if (dprx_drv_is_aud_ch_status_change_event()) {
		dprx_audio_mute_cb_process(dprx);
		dprx_drv_clr_aud_ch_status_change_event();
	}
	if (dprx_drv_is_aud_vbid_mute_event()) {
		dprx_audio_mute_cb_process(dprx);
		dprx_drv_clr_aud_vbid_mute_event();
	}
	if (dprx_drv_is_aud_fifo_und_event()) {
		dprx_audio_mute_cb_process(dprx);
		dprx_drv_clr_aud_fifo_und_event();
	}
	if (dprx_drv_is_aud_fifo_over_event()) {
		dprx_audio_mute_cb_process(dprx);
		atomic_set(&dprx_cb_sts.audio_overflow, 1);
		dprx_drv_event_set(dprx);
		dprx_drv_clr_aud_fifo_over_event();
	}
	if (dprx_drv_is_aud_link_err_event()) {
		dprx_audio_mute_cb_process(dprx);
		dprx_drv_clr_aud_link_err_event();
	}
	if (dprx_drv_is_aud_dec_err_event()) {
		dprx_audio_mute_cb_process(dprx);
		dprx_drv_clr_aud_dec_err_event();
	}

	/* Audio unmute process */
	if (dprx_drv_is_aud_unmute_event()) {
		dprx_audio_unmute_cb_process(dprx);
		dprx_drv_clr_aud_unmute_event();
	}

	MTK_DPRX_WRITE32(ADDR_AUD_INTR, _audio_status);
	MTK_DPRX_WRITE32(ADDR_AUD_ADD_INTR, _audio_unmute_status);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX top layer's interrupt - unplug interrupt update function
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
void dprx_drv_unplug_intr_update(struct mtk_dprx *dprx)
{
	pr_debug("unplug intr update\n");
	dprx_audio_mute_cb_process(dprx);
	dprx_video_mute_process();
	dprx_drv_hdcp2x_auth_status_clear();
	f_unplug = 1;
	atomic_set(&dprx_cb_sts.unplug, 1);
	dprx_drv_event_set(dprx);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX top layer's interrupt - plug interrupt update function
 * @par Parameters
 *     none
 * @return
 *     0, function success.\n
 *     1, dev is null.\n
 *     < 0, function fail.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_drv_plug_intr_update(struct mtk_dprx *dprx)
{
	struct device *dev = dprx->dev;
	int ret = 0;

	pr_debug("plug intr update\n");

	f_unplug = 0;
	atomic_set(&dprx_cb_sts.plugin, 1);
	dprx_drv_event_set(dprx);
	MTK_DPRX_REG_WRITE_MASK(ADDR_RST_SEL_1, 1, AUDIOPLL_AUTO_RST_FLDMASK);
	if (f_phy_gce == 0)
		ret = mtk_dprx_phy_gce_init(dev);
	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX top layer's interrupt - video on interrupt update function
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
void dprx_drv_video_on_intr_update(struct mtk_dprx *dprx)
{
	dprx_align_status_unlock_intr_enable(1);
	dprx_video_unmute_process();
	atomic_set(&dprx_cb_sts.video_on, 1);
	dprx_drv_event_set(dprx);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX HDR InfoFrame changed status
 * @par Parameters
 *     none
 * @return
 *     main_link_intr0.info.hdr_info_change status
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t dprx_drv_is_hdr_info_change_event(void)
{
	return main_link_intr0.info.hdr_info_change;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX HDR InfoFrame changed status clear
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
int32_t dprx_drv_clr_hdr_info_change_event(void)
{
	main_link_intr0.info.hdr_info_change = 0;
	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX VSC package changed status
 * @par Parameters
 *     none
 * @return
 *     main_link_intr0.info.hdr_info_change status
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t dprx_drv_is_vsc_pkg_change_event(void)
{
	return main_link_intr0.info.vsc_pkg_change;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX VSC package changed status clear
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
int32_t dprx_drv_clr_vsc_pkg_change_event(void)
{
	main_link_intr0.info.vsc_pkg_change = 0;
	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Audio InfoFrame changed status
 * @par Parameters
 *     none
 * @return
 *     main_link_intr0.info.hdr_info_change status
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t dprx_drv_is_audio_info_change_event(void)
{
	return main_link_intr0.info.audio_info_change;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Audio InfoFrame changed status clear
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
int32_t dprx_drv_clr_audio_info_change_event(void)
{
	main_link_intr0.info.audio_info_change = 0;
	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX SPD InfoFrame changed status
 * @par Parameters
 *     none
 * @return
 *     main_link_intr0.info.spd_info_change status
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t dprx_drv_is_spd_info_change_event(void)
{
	return main_link_intr0.info.spd_info_change;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX SPD InfoFrame changed status clear
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
int32_t dprx_drv_clr_spd_info_change_event(void)
{
	main_link_intr0.info.spd_info_change = 0;
	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX video mute interrupt status
 * @par Parameters
 *     none
 * @return
 *     main_link_intr1.video_mute_intr status
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t dprx_drv_is_video_mute_event(void)
{
	return main_link_intr1.info.video_mute_intr;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX video mute interrupt status clear
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
int32_t dprx_drv_clr_video_mute_event(void)
{
	main_link_intr1.info.video_mute_intr = 0;
	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX MSA change interrupt status
 * @par Parameters
 *     none
 * @return
 *     main_link_intr1.msa_update_intr status
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t dprx_drv_is_msa_change_event(void)
{
	return main_link_intr1.info.msa_update_intr;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX MSA change interrupt status clear
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
int32_t dprx_drv_clr_msa_change_event(void)
{
	main_link_intr1.info.msa_update_intr = 0;
	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX audio M/N value changed interrupt status
 * @par Parameters
 *     none
 * @return
 *     main_link_intr1.audio_mn_chg
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t dprx_drv_is_maud_change_event(void)
{
	return main_link_intr1.info.audio_mn_chg;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX audio M/N value changed interrupt status clear
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
int32_t dprx_drv_clr_maud_change_event(void)
{
	main_link_intr1.info.audio_mn_chg = 0;
	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX compressed frame(VB-ID bit6) rising edge interrupt status
 * @par Parameters
 *     none
 * @return
 *     main_link_intr3.dsc_rs_intr
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t dprx_drv_is_compress_fm_rs_event(void)
{
	return main_link_intr3.info.dsc_rs_intr;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *      DPRX compressed frame(VB-ID bit6) rising edge interrupt status clear
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
int32_t dprx_drv_clr_compress_fm_rs_event(void)
{
	main_link_intr3.info.dsc_rs_intr = 0;
	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX compressed frame(VB-ID bit6) falling edge interrupt status
 * @par Parameters
 *     none
 * @return
 *     main_link_intr3.dsc_fl_intr
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t dprx_drv_is_compress_fm_fl_event(void)
{
	return main_link_intr3.info.dsc_fl_intr;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *      DPRX compressed frame(VB-ID bit6) falling edge interrupt status clear
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
int32_t dprx_drv_clr_compress_fm_fl_event(void)
{
	main_link_intr3.info.dsc_rs_intr = 0;
	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX PPS changed interrupt status
 * @par Parameters
 *     none
 * @return
 *     main_link_intr3.pps_chg
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t dprx_drv_is_pps_change_event(void)
{
	return main_link_intr3.info.pps_chg;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX PPS changed interrupt status clear
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
int32_t dprx_drv_clr_pps_change_event(void)
{
	main_link_intr3.info.pps_chg = 0;
	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX align status unlock interrupt status
 * @par Parameters
 *     none
 * @return
 *     main_link_intr3.pps_chg
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t dprx_drv_is_align_status_unlock_event(void)
{
	return dpip_intr.info.align_status_unlock;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX align status unlock interrupt status clear
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
int32_t dprx_drv_clr_align_status_unlock_event(void)
{
	dpip_intr.info.align_status_unlock = 0;
	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX CR training performed interrupt status
 * @par Parameters
 *     none
 * @return
 *     dpcd_intr1.info.cr_training
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t dprx_drv_is_cr_training_event(void)
{
	return dpcd_intr1.info.cr_training;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX CR training performed interrupt status clear
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
int32_t dprx_drv_clr_cr_training_event(void)
{
	dpcd_intr1.info.cr_training = 0;
	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX EQ training performed interrupt status
 * @par Parameters
 *     none
 * @return
 *     dpcd_intr1.info.eq_training
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t dprx_drv_is_eq_training_event(void)
{
	return dpcd_intr1.info.eq_training;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX EQ training performed interrupt status clear
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
int32_t dprx_drv_clr_eq_training_event(void)
{
	dpcd_intr1.info.eq_training = 0;
	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX BW change status
 * @par Parameters
 *     none
 * @return
 *     dpcd_intr1.info.bw_change
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t dprx_drv_is_bw_change_event(void)
{
	return dpcd_intr1.info.bw_change;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX BW change status clear
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
int32_t dprx_drv_clr_bw_change_event(void)
{
	dpcd_intr1.info.bw_change = 0;
	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX audio channel status value changed interrupt status
 * @par Parameters
 *     none
 * @return
 *     audio_intr.aud_ch_status_change
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t dprx_drv_is_aud_ch_status_change_event(void)
{
	return audio_intr.info.aud_ch_status_change;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX audio channel status value changed interrupt status clear
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
int32_t dprx_drv_clr_aud_ch_status_change_event(void)
{
	audio_intr.info.aud_ch_status_change = 0;
	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX VBID bit 4 (Audio Mute) interrupt status
 * @par Parameters
 *     none
 * @return
 *     audio_intr.aud_vbid_mute
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t dprx_drv_is_aud_vbid_mute_event(void)
{
	return audio_intr.info.aud_vbid_mute;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX VBID bit 4 (Audio Mute) interrupt status clear
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
int32_t dprx_drv_clr_aud_vbid_mute_event(void)
{
	audio_intr.info.aud_vbid_mute = 0;
	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Audio FIFO underflow interrupt status
 * @par Parameters
 *     none
 * @return
 *     audio_intr.aud_fifo_und
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t dprx_drv_is_aud_fifo_und_event(void)
{
	return audio_intr.info.aud_fifo_und;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Audio FIFO underflow interrupt status clear
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
int32_t dprx_drv_clr_aud_fifo_und_event(void)
{
	audio_intr.info.aud_fifo_und = 0;
	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Audio FIFO overflow interrupt status
 * @par Parameters
 *     none
 * @return
 *     audio_intr.aud_fifo_over
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t dprx_drv_is_aud_fifo_over_event(void)
{
	return audio_intr.info.aud_fifo_over;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Audio FIFO overflow interrupt status clear
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
int32_t dprx_drv_clr_aud_fifo_over_event(void)
{
	audio_intr.info.aud_fifo_over = 0;
	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Audio link error interrupt status
 * @par Parameters
 *     none
 * @return
 *     audio_intr.aud_link_err
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t dprx_drv_is_aud_link_err_event(void)
{
	return audio_intr.info.aud_link_err;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Audio link error interrupt status clear
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
int32_t dprx_drv_clr_aud_link_err_event(void)
{
	audio_intr.info.aud_link_err = 0;
	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Audio rs decoder error interrupt status
 * @par Parameters
 *     none
 * @return
 *     audio_intr.aud_dec_err
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t dprx_drv_is_aud_dec_err_event(void)
{
	return audio_intr.info.aud_dec_err;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Audio rs decoder error interrupt status clear
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
int32_t dprx_drv_clr_aud_dec_err_event(void)
{
	audio_intr.info.aud_dec_err = 0;
	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Audio unmute interrupt status
 * @par Parameters
 *     none
 * @return
 *     audio_intr.aud_dec_err
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t dprx_drv_is_aud_unmute_event(void)
{
	return audio_add_intr.info.aud_unmute_intr;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX Audio unmute interrupt status clear
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
int32_t dprx_drv_clr_aud_unmute_event(void)
{
	audio_add_intr.info.aud_unmute_intr = 0;
	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX interrupt related structure initialization
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
void dprx_init_events(void)
{
	main_link_intr0.value = 0;
	main_link_intr1.value = 0;
	main_link_intr3.value = 0;
	audio_intr.value = 0;
	audio_add_intr.value = 0;
	dpcd_intr1.value = 0;
	f_unplug = 2;
	atomic_set(&dprx_cb_sts.video_on, 0);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX irq handler
 * @param[in]
 *     irq: irq number
 * @param[in]
 *     dev_id: dprx device id content
 * @return
 *     IRQ_HANDLED
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
irqreturn_t mtk_dprx_irq(int irq, void *dev_id)
{
	u32 _u4top_intr = 0;
	u32 _u4plug_intr = 0;
	struct mtk_dprx *dprx = dev_id;

	irq_count++;
	pr_debug("Enter mtk_dprx_irq, irq_count = %d\n", irq_count);

	MTK_DPRX_REG_WRITE_MASK(ADDR_INTR_MASK, 0xff, INTR_MASK_FLDMASK);

	_u4top_intr = MTK_DPRX_READ32(ADDR_INTR);
	MTK_DPRX_WRITE32(ADDR_INTR, _u4top_intr);
	_u4plug_intr = MTK_DPRX_READ32(ADDR_INTR_PLUG_MASK);
	MTK_DPRX_WRITE32(ADDR_INTR_PLUG_MASK, _u4plug_intr);
	pr_debug("ADDR_INTR = 0x%x, ADDR_INTR_PLUG_MASK = 0x%x\n",
		 _u4top_intr,
		 _u4plug_intr);

	if (_u4top_intr & MAIN_LINK_INTR_FLDMASK)
		dprx_drv_mainlink_intr_update(dprx);
	if (_u4top_intr & DP_IP_INTR_FLDMASK)
		dprx_drv_dpip_intr_update(dprx);
	if (_u4top_intr & DPCD_INTR_FLDMASK)
		dprx_drv_dpcd_intr_update(dprx);
	if (_u4top_intr & AUDIO_INTR_FLDMASK)
		dprx_drv_audio_intr_update(dprx);
	if (_u4top_intr & VIDEO_ON_INT_FLDMASK)
		dprx_drv_video_on_intr_update(dprx);
	if (_u4plug_intr & TX_UNPLUG_INT_FLDMASK)
		dprx_drv_unplug_intr_update(dprx);
	if (_u4plug_intr & TX_PLUG_INT_FLDMASK)
		dprx_drv_plug_intr_update(dprx);

	if (atomic_read(&dprx->main_event))
		wake_up_interruptible(&dprx->main_wq);

	MTK_DPRX_REG_WRITE_MASK(ADDR_INTR_MASK, 0x00, INTR_MASK_FLDMASK);

	return IRQ_HANDLED;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX callback function
 * @param[in]
 *     dprx: struct mtk_dprx *dprx
 * @param[in]
 *     notify: event ID
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     dprx->callback = NULL, function return
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprx_state_callback(struct mtk_dprx *dprx, enum DPRX_NOTIFY_T notify)
{
	pr_debug("DP callback notify = %d\n", notify);

	if (dprx->callback == NULL) {
		pr_info("dprx callback == NULL, err\n");
		return;
	}

	switch (notify) {
	case DPRX_RX_MSA_CHANGE:
		dprx->callback(dprx->ap_dev, DPRX_RX_MSA_CHANGE);
		break;
	case DPRX_RX_AUD_MN_CHANGE:
		dprx->callback(dprx->ap_dev, DPRX_RX_AUD_MN_CHANGE);
		break;
	case DPRX_RX_DSC_CHANGE:
		dprx->callback(dprx->ap_dev, DPRX_RX_DSC_CHANGE);
		break;
	case DPRX_RX_PPS_CHANGE:
		dprx->callback(dprx->ap_dev, DPRX_RX_PPS_CHANGE);
		break;
	case DPRX_RX_UNPLUG:
		dprx->callback(dprx->ap_dev, DPRX_RX_UNPLUG);
		break;
	case DPRX_RX_PLUGIN:
		dprx->callback(dprx->ap_dev, DPRX_RX_PLUGIN);
		break;
	case DPRX_RX_VIDEO_STABLE:
		dprx->callback(dprx->ap_dev, DPRX_RX_VIDEO_STABLE);
		break;
	case DPRX_RX_VIDEO_NOT_STABLE:
		dprx->callback(dprx->ap_dev, DPRX_RX_VIDEO_NOT_STABLE);
		break;
	case DPRX_RX_VIDEO_MUTE:
		dprx->callback(dprx->ap_dev, DPRX_RX_VIDEO_MUTE);
		break;
	case DPRX_RX_AUDIO_INFO_CHANGE:
		dprx->callback(dprx->ap_dev, DPRX_RX_AUDIO_INFO_CHANGE);
		break;
	case DPRX_RX_HDR_INFO_CHANGE:
		dprx->callback(dprx->ap_dev, DPRX_RX_HDR_INFO_CHANGE);
		break;
	case DPRX_RX_AUDIO_MUTE:
		dprx->callback(dprx->ap_dev, DPRX_RX_AUDIO_MUTE);
		break;
	case DPRX_RX_AUDIO_UNMUTE:
		dprx->callback(dprx->ap_dev, DPRX_RX_AUDIO_UNMUTE);
		break;
	case DPRX_RX_BW_CHANGE:
		dprx->callback(dprx->ap_dev, DPRX_RX_BW_CHANGE);
		break;
	case DPRX_RX_SPD_INFO_CHANGE:
		dprx->callback(dprx->ap_dev, DPRX_RX_SPD_INFO_CHANGE);
		break;
	case DPRX_RX_LINK_ERROR:
		dprx->callback(dprx->ap_dev, DPRX_RX_LINK_ERROR);
		break;
	default:
		break;
	}
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX thread excute function
 * @param[in]
 *     dev_id: struct mtk_dprx *dprx
 * @return
 *     0
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_drv_main_kthread(void *dev_id)
{
	struct sched_param param = {.sched_priority = DEFAULT_PRIO};
	struct mtk_dprx *dprx = dev_id;
#ifdef CONFIG_MTK_DPRX_PHY_GCE
	struct device *dev = dprx->dev;
#endif
	static int audio_overflow_counter = 0;

	pr_debug("DP callback thread excute function\n");
	sched_setscheduler(current, SCHED_RR, &param);

	while (dprx->is_rx_init) {
		if (atomic_read(&dprx->main_event) == 0) {
			wait_event_interruptible(
				dprx->main_wq, atomic_read(&dprx->main_event));
		}
		atomic_set(&dprx->main_event, 0);

		if (atomic_cmpxchg(&dprx_cb_sts.unplug, 1, 0)) {
			pr_debug("DP callback - unplug\n");
			dprx_state_callback(dprx, DPRX_RX_UNPLUG);
		}
		if (atomic_cmpxchg(&dprx_cb_sts.plugin, 1, 0)) {
			pr_debug("DP callback - plugin\n");
			dprx_state_callback(dprx, DPRX_RX_PLUGIN);
		}
		if (atomic_cmpxchg(&dprx_cb_sts.link_error, 1, 0)) {
			pr_debug("DP callback - link error\n");
			dprx_state_callback(dprx, DPRX_RX_LINK_ERROR);
		}
		if (atomic_cmpxchg(&dprx_cb_sts.bw_change, 1, 0)) {
			pr_debug("DP callback - Link rate is changed.\n");
			dprx_state_callback(dprx, DPRX_RX_BW_CHANGE);
		}
		if (atomic_cmpxchg(&dprx_cb_sts.video_mute, 1, 0)) {
			pr_err("DP callback - video is mute\n");
			dprx_state_callback(dprx, DPRX_RX_VIDEO_MUTE);
		}
		if (atomic_cmpxchg(&dprx_cb_sts.msa_change, 1, 0)) {
			pr_debug("DP callback - MSA update\n");
			dprx_state_callback(dprx, DPRX_RX_MSA_CHANGE);
		}
		if (atomic_cmpxchg(&dprx_cb_sts.aud_mn_change, 1, 0)) {
			pr_debug("DP callback - audio mn change update\n");
			dprx_state_callback(dprx, DPRX_RX_AUD_MN_CHANGE);
		}
		if (atomic_cmpxchg(&dprx_cb_sts.pps_change, 1, 0)) {
			pr_debug("DP callback - PPS change update\n");
			dprx_state_callback(dprx, DPRX_RX_PPS_CHANGE);
		}
		if (atomic_cmpxchg(&dprx_cb_sts.dsc_change, 1, 0)) {
			pr_debug("DP callback - DSC mode change update\n");
			dprx_state_callback(dprx, DPRX_RX_DSC_CHANGE);
		}
		if (atomic_cmpxchg(&dprx_cb_sts.video_on, 1, 0)) {
			pr_debug("DP callback - video is on\n");
			dprx_video_stable_check(dprx);
		}
		if (atomic_cmpxchg(&dprx_cb_sts.video_stable, 1, 0)) {
			if (dprx_vid_stable_chk.vid_stable_sts) {
				pr_debug("DP callback - video is stable\n");
				dprx_state_callback(dprx,
						    DPRX_RX_VIDEO_STABLE);
			} else {
				pr_debug("DP callback - video is not stable\n");
				dprx_state_callback(dprx,
						    DPRX_RX_VIDEO_NOT_STABLE);
			}
		}
		if (atomic_cmpxchg(&dprx_cb_sts.audio_info_change, 1, 0)) {
			pr_debug("DP callback - audio_info_change\n");
			dprx_state_callback(dprx, DPRX_RX_AUDIO_INFO_CHANGE);
		}
		if (atomic_cmpxchg(&dprx_cb_sts.spd_info_change, 1, 0)) {
			pr_debug("DP callback - spd_info_change\n");
			dprx_state_callback(dprx, DPRX_RX_SPD_INFO_CHANGE);
		}
		if (atomic_cmpxchg(&dprx_cb_sts.hdr_info_change, 1, 0)) {
			pr_debug("DP callback - hdr_info_change\n");
			dprx_state_callback(dprx, DPRX_RX_HDR_INFO_CHANGE);
		}
		if (atomic_cmpxchg(&dprx_cb_sts.audio_mute, 1, 0)) {
			pr_debug("DP callback - audio mute\n");
			dprx_state_callback(dprx, DPRX_RX_AUDIO_MUTE);
		}
		if (atomic_cmpxchg(&dprx_cb_sts.stereo_type_change, 1, 0)) {
			pr_debug("DP callback - stereo type change\n");
			dprx_state_callback(dprx, DPRX_RX_STEREO_TYPE_CHANGE);
		}
		if (atomic_cmpxchg(&dprx_cb_sts.audio_unmute, 1, 0)) {
			pr_debug("DP callback - audio unmute\n");
			dprx_state_callback(dprx, DPRX_RX_AUDIO_UNMUTE);
		}
		if (atomic_cmpxchg(&dprx_cb_sts.audio_overflow, 1, 0)) {
			pr_debug("DP callback - audio overflow\n");
			audio_overflow_counter++;
			if (audio_overflow_counter > 10) {
				audio_overflow_counter = 0;
				pr_warn("DP audio overflow continues to happen over 10 times.\n");
				pr_err("#### Temporary DP power off/on to recover audio overflow.\n");

				/* DP Power off sequence */
				/*  from mtk_dprx_drv_stop(); */
				dprx_drv_sw_hpd_ready_ctrl(DISABLE);
				dprx_disable_irq();
				dprx_drv_hdcp2x_disable();
				/*  from mtk_dprx_drv_deinit(); */
				mtk_dprx_hw_deinit(dprx);
				/*  mtk_dprx_phy_gce_deinit(dev); */
#ifdef CONFIG_MTK_DPRX_PHY_GCE
				mtk_dprx_gce_phy_destroy(dev);
#endif
				/*  from mtk_dprx_power_off(dprx); */
				dprx_power_off_seq(dprx);

				/* DP Power on sequence */
				/*  from mtk_dprx_power_on(dprx); */
				dprx_power_on_seq(dprx);
				/*  from mtk_dprx_drv_init(dprx); */
				mtk_dprx_hw_init(dprx, f_hdcp2x_en);
				/*  mtk_dprx_phy_gce_init(dev); */
#ifdef CONFIG_MTK_DPRX_PHY_GCE
				mtk_dprx_gce_phy_create(dev);
#endif
				/*  from mtk_dprx_drv_start(); */
				dprx_drv_hdcp2x_enable();
				dprx_enable_irq();
				dprx_drv_sw_hpd_ready_ctrl(DISABLE);
				dprx_drv_sw_hpd_ready_ctrl(ENABLE);
			}
		} else {
			audio_overflow_counter = 0;
		}
		if (kthread_should_stop())
			break;
	}

	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX thread create function
 * @param[in]
 *     dprx: struct mtk_dprx *dprx
 * @return
 *     0
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
unsigned int dprx_drv_task_init(struct mtk_dprx *dprx)
{
	pr_debug("DP callback thread create\n");

	init_waitqueue_head(&dprx->main_wq);
	dprx->main_task = kthread_create(
		dprx_drv_main_kthread, dprx, "dprx_event_thread");
	dprx->is_rx_init = 1;
	wake_up_process(dprx->main_task);

	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX set event for thread
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
void dprx_drv_event_set(struct mtk_dprx *dprx)
{
	atomic_set(&dprx->main_event, 1);
	wake_up_interruptible(&dprx->main_wq);
}

#ifdef CONFIG_MTK_DPRX_PHY_GCE
/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX create cmdq mbox
 * @param[in]
 *     dprx: struct mtk_dprx *dprx
 * @return
 *     0, Success\n
 *     non zero, Fail\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dprx_cmdq_mbox_create(struct mtk_dprx *dprx)
{
	struct device *dev = dprx->dev;

	pr_debug("%s\n", __func__);
	dprx->phy_client = cmdq_mbox_create(dev, 0);
	if (IS_ERR(dprx->phy_client)) {
		dev_err(dev, "failed to create phy client\n");
		cmdq_mbox_destroy(dprx->phy_client);
		return -EINVAL;
	}
	pr_debug("%s client=%p\n", __func__, dprx->phy_client);
	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX destroy cmdq mbox
 * @param[in]
 *     dprx: struct mtk_dprx *dprx
 * @return
 *     0, Success\n
 *     non zero, Fail\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dprx_cmdq_mbox_destroy(struct mtk_dprx *dprx)
{
	int err = 0;

	pr_debug("%s\n", __func__);
	err = cmdq_mbox_destroy(dprx->phy_client);
	if (err)
		pr_err("failed to destroy phy client\n");
	return err;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX PHY GCE patch
 * @param[in]
 *     dev: struct device *dev
 * @param[in]
 *     handle: struct cmdq_pkt *handle
 * @return
 *     0, Success\n
 *     non zero, Fail\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dprx_gce_phy_config(const struct device *dev, struct cmdq_pkt *handle)
{
	int ret = 0;
	struct mtk_dprx *dprx = dev_get_drvdata(dev);
	struct cmdq_operand left_operand;
	struct cmdq_operand right_operand;
	u16 td_event;

	pr_debug("%s\n", __func__);
	cmdq_pkt_assign_command(handle, GCE_CPR(8), 0x00);
	dprx->phy_mutex = NULL;

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000040);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_GCE_STATUS);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000040);

	td_event = 303;
	dprx->phy_mutex = mtk_mutex_get(dprx->mutex_dev, 1);
	ret |= mtk_mmsys_cfg_power_on(dprx->mmsys_dev);
	td_event += mtk_mutex_res2id(dprx->phy_mutex);
	ret |= mtk_mutex_power_on(dprx->phy_mutex);
	ret |= mtk_mutex_select_timer_sof_w_gce5(dprx->phy_mutex,
						 MUTEX_MMSYS_SOF_SINGLE,
						 MUTEX_MMSYS_SOF_SINGLE,
						 GCE_SPR0,
						 GCE_SPR1,
						 &handle);

	ret |= mtk_mutex_set_timer_us_w_gce5(dprx->phy_mutex,
					     PHY_POLLING_TIME,
					     PHY_POLLING_TIME,
					     GCE_SPR0,
					     GCE_SPR1,
					     &handle);
	ret |= mtk_mutex_timer_enable_ex_w_gce5(dprx->phy_mutex,
						false,
						MUTEX_TO_EVENT_SRC_OVERFLOW,
						GCE_SPR0,
						GCE_SPR1,
						&handle);
	ret |= cmdq_pkt_wfe(handle, td_event);
	ret |= mtk_mutex_timer_disable_w_gce5(dprx->phy_mutex,
					      GCE_SPR0,
					      GCE_SPR1,
					      &handle);

	cmdq_pkt_assign_command(handle, GCE_SPR0, LINK_BW_SET_GCE);
	cmdq_pkt_load(handle, GCE_CPR(9), GCE_SPR0);
	cmdq_pkt_assign_command(handle, GCE_SPR2, -29*CMDQ_INST_SIZE);
	cmdq_pkt_cond_jump(handle,
			   GCE_SPR2,
			   cmdq_operand_reg(&left_operand, GCE_CPR(9)),
			   cmdq_operand_reg(&right_operand, GCE_CPR(8)),
			   CMDQ_EQUAL);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_GCE_STATUS);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x000000bf);

	cmdq_pkt_assign_command(handle, GCE_SPR1, 0x1e);
	cmdq_pkt_assign_command(handle, GCE_SPR2, 439*CMDQ_INST_SIZE);
	cmdq_pkt_cond_jump(handle,
			   GCE_SPR2,
			   cmdq_operand_reg(&left_operand, GCE_CPR(9)),
			   cmdq_operand_reg(&right_operand, GCE_SPR1),
			   CMDQ_EQUAL);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000001);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_GCE_STATUS);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000001);

	cmdq_pkt_assign_command(handle, GCE_SPR0, TRAINING_PATTERN_GCE);
	cmdq_pkt_load(handle, GCE_CPR(10), GCE_SPR0);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00100007);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_GLB_18);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00300007);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00200000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_GLB_18);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00200000);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000100);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L0_TRX_60);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000100);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x0000ff00);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L0_TRX_64);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x0001ff00);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000100);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L1_TRX_60);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000100);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x0000ff00);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L1_TRX_64);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x0001ff00);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000100);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L2_TRX_60);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000100);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x0000ff00);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L2_TRX_64);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x0001ff00);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000100);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L3_TRX_60);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000100);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x0000ff00);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L3_TRX_64);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x0001ff00);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000400);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L0_TRX_34);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000400);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000400);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L1_TRX_34);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000400);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000400);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L2_TRX_34);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000400);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000400);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L3_TRX_34);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000400);

	ret |= mtk_mutex_set_timer_us_w_gce5(dprx->phy_mutex,
					     17,
					     17,
					     GCE_SPR0,
					     GCE_SPR1,
					     &handle);
	ret |= mtk_mutex_timer_enable_ex_w_gce5(dprx->phy_mutex,
						false,
						MUTEX_TO_EVENT_SRC_OVERFLOW,
						GCE_SPR0,
						GCE_SPR1,
						&handle);
	ret |= cmdq_pkt_wfe(handle, td_event);
	ret |= mtk_mutex_timer_disable_w_gce5(dprx->phy_mutex,
					      GCE_SPR0,
					      GCE_SPR1,
					      &handle);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x40000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L0_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x40000000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x80000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L0_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x80000000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x40000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L1_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x40000000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x80000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L1_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x80000000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x40000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L2_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x40000000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x80000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L2_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x80000000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x40000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L3_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x40000000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x80000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L3_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x80000000);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00030000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L0_RX2_34);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x000f0000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00030000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L1_RX2_34);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x000f0000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00030000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L2_RX2_34);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x000f0000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00030000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L3_RX2_34);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x000f0000);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x0b000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L0_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x1f000000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x20000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L0_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x20000000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x0b000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L1_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x1f000000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x20000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L1_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x20000000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x0b000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L2_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x1f000000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x20000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L2_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x20000000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x0b000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L3_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x1f000000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x20000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L3_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x20000000);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00002500);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L0_TRX_84);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00007f00);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00002500);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L1_TRX_84);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00007f00);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00002500);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L2_TRX_84);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00007f00);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00002500);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L3_TRX_84);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00007f00);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000800);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L0_TRX_70);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000f00);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000800);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L1_TRX_70);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000f00);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000800);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L2_TRX_70);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000f00);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000800);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L3_TRX_70);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000f00);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000003);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_GLB_28);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000003);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00001000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L0_TRX_24);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00001000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00002000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L0_TRX_24);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00002000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00001000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L1_TRX_24);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00001000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00002000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L1_TRX_24);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00002000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00001000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L2_TRX_24);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00001000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00002000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L2_TRX_24);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00002000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00001000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L3_TRX_24);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00001000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00002000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L3_TRX_24);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00002000);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000010);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L0_RX_30);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000010);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000020);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L0_RX_30);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000020);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000010);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L1_RX_30);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000010);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000020);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L1_RX_30);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000020);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000010);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L2_RX_30);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000010);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000020);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L2_RX_30);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000020);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000010);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L3_RX_30);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000010);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000020);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L3_RX_30);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000020);

	ret |= mtk_mutex_set_timer_us_w_gce5(dprx->phy_mutex,
					     10,
					     10,
					     GCE_SPR0,
					     GCE_SPR1,
					     &handle);
	ret |= mtk_mutex_timer_enable_ex_w_gce5(dprx->phy_mutex,
						false,
						MUTEX_TO_EVENT_SRC_OVERFLOW,
						GCE_SPR0,
						GCE_SPR1,
						&handle);
	ret |= cmdq_pkt_wfe(handle, td_event);
	ret |= mtk_mutex_timer_disable_w_gce5(dprx->phy_mutex,
					      GCE_SPR0,
					      GCE_SPR1,
					      &handle);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_GLB_28);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000002);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_GLB_28);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000001);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L0_RX_30);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000010);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L0_RX_30);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000020);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L1_RX_30);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000010);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L1_RX_30);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000020);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L2_RX_30);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000010);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L2_RX_30);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000020);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L3_RX_30);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000010);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L3_RX_30);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000020);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_GLB_18);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00200000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_GLB_18);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00100000);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L0_TRX_24);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00001000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L0_TRX_24);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00002000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L1_TRX_24);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00001000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L1_TRX_24);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00002000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L2_TRX_24);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00001000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L2_TRX_24);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00002000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L3_TRX_24);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00001000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L3_TRX_24);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00002000);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L0_TRX_60);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000100);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L1_TRX_60);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000100);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L2_TRX_60);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000100);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L3_TRX_60);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000100);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L0_TRX_34);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000400);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L1_TRX_34);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000400);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L2_TRX_34);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000400);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L3_TRX_34);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000400);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_GLB_18);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000001);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000001);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_GLB_10);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000001);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_GLB_10);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000001);

	cmdq_operand_reg(&left_operand, GCE_CPR(9));
	cmdq_operand_immediate(&right_operand, 0);
	cmdq_pkt_logic_command(handle,
			       CMDQ_LOGIC_ADD,
			       GCE_CPR(8),
			       &left_operand,
			       &right_operand);

	cmdq_pkt_assign_command(handle, GCE_SPR0, TRAINING_PATTERN_GCE);
	cmdq_pkt_load(handle, GCE_CPR(11), GCE_SPR0);

	cmdq_pkt_assign_command(handle, GCE_SPR1, 0);
	cmdq_pkt_assign_command(handle, GCE_SPR2, 4*CMDQ_INST_SIZE);
	cmdq_pkt_cond_jump(handle,
			   GCE_SPR2,
			   cmdq_operand_reg(&left_operand, GCE_CPR(10)),
			   cmdq_operand_reg(&right_operand, GCE_SPR1),
			   CMDQ_EQUAL);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000010);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_GCE_STATUS);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000010);

	cmdq_pkt_assign_command(handle, GCE_SPR1, 0);
	cmdq_pkt_assign_command(handle, GCE_SPR2, 4*CMDQ_INST_SIZE);
	cmdq_pkt_cond_jump(handle,
			   GCE_SPR2,
			   cmdq_operand_reg(&left_operand, GCE_CPR(11)),
			   cmdq_operand_reg(&right_operand, GCE_SPR1),
			   CMDQ_EQUAL);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000020);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_GCE_STATUS);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000020);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000002);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_GCE_STATUS);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000002);

	cmdq_pkt_jump(handle, -474*CMDQ_INST_SIZE);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000004);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_GCE_STATUS);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000004);

	cmdq_pkt_assign_command(handle, GCE_SPR0, TRAINING_PATTERN_GCE);
	cmdq_pkt_load(handle, GCE_CPR(10), GCE_SPR0);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L0_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x40000000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L0_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x80000000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L1_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x40000000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L1_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x80000000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L2_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x40000000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L2_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x80000000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L3_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x40000000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L3_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x80000000);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00080000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L0_RX2_34);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x000f0000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00080000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L1_RX2_34);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x000f0000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00080000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L2_RX2_34);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x000f0000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00080000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L3_RX2_34);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x000f0000);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L0_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x20000000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L1_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x20000000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L2_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x20000000);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000000);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_DIG_L3_TRX_38);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x20000000);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00003f00);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L0_TRX_84);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00007f00);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00003f00);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L1_TRX_84);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00007f00);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00003f00);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L2_TRX_84);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00007f00);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00003f00);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L3_TRX_84);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00007f00);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000400);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L0_TRX_70);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000f00);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000400);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L1_TRX_70);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000f00);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000400);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L2_TRX_70);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000f00);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000400);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_ANA_L3_TRX_70);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000f00);

	cmdq_operand_reg(&left_operand, GCE_CPR(9));
	cmdq_operand_immediate(&right_operand, 0);
	cmdq_pkt_logic_command(handle,
			       CMDQ_LOGIC_ADD,
			       GCE_CPR(8),
			       &left_operand,
			       &right_operand);

	cmdq_pkt_assign_command(handle, GCE_SPR0, TRAINING_PATTERN_GCE);
	cmdq_pkt_load(handle, GCE_CPR(11), GCE_SPR0);

	cmdq_pkt_assign_command(handle, GCE_SPR1, 0);
	cmdq_pkt_assign_command(handle, GCE_SPR2, 4*CMDQ_INST_SIZE);
	cmdq_pkt_cond_jump(handle,
			   GCE_SPR2,
			   cmdq_operand_reg(&left_operand, GCE_CPR(10)),
			   cmdq_operand_reg(&right_operand, GCE_SPR1),
			   CMDQ_EQUAL);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000010);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_GCE_STATUS);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000010);

	cmdq_pkt_assign_command(handle, GCE_SPR1, 0);
	cmdq_pkt_assign_command(handle, GCE_SPR2, 4*CMDQ_INST_SIZE);
	cmdq_pkt_cond_jump(handle,
			   GCE_SPR2,
			   cmdq_operand_reg(&left_operand, GCE_CPR(11)),
			   cmdq_operand_reg(&right_operand, GCE_SPR1),
			   CMDQ_EQUAL);
	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000020);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_GCE_STATUS);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000020);

	cmdq_pkt_assign_command(handle, GCE_SPR0, 0x00000008);
	cmdq_pkt_assign_command(handle, GCE_SPR1, DPRX_GCE_STATUS);
	cmdq_pkt_store_reg(handle, GCE_SPR1, GCE_SPR0, 0x00000008);

	cmdq_pkt_jump(handle, -598*CMDQ_INST_SIZE);
	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX GCE flush async callback function
 * @param[in]
 *     data: struct cmdq_cb_data dat
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dprx_phy_handling_cb(struct cmdq_cb_data data)
{
	struct mtk_dprx *dprx = data.data;

	pr_debug("====== mtk_dprx_phy_handling_cb ======\n");
	cmdq_pkt_destroy(dprx->gce_pkt);
	complete(&phy_handle_cb);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX waiting async callback function complete
 * @par Parameters
 *     none
 * @return
 *     0, Success\n
 *     non zero, Fail\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dprx_phy_handling_cb_complete(void)
{
	int ret;

	pr_debug("mtk_dprx_phy_handling_cb_complete start...\n");
	ret = wait_for_completion_interruptible_timeout(&phy_handle_cb,
							PHY_CB_TIMEOUT);

	if (ret <= 0) {
		pr_err("wait DP PHY Handling CB NG!!\n");
		ret = -ETIME;
	} else {
		pr_debug("mtk_dprx_phy_handling_cb_complete end...\n");
		ret = 0;
	}
	return ret;
}


/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX PHY GCE flush prepare
 * @param[in]
 *     dev: struct device *dev
 * @return
 *     0, Success\n
 *     non zero, Fail\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dprx_gce_phy_prepare(const struct device *dev)
{
	struct mtk_dprx *dprx = dev_get_drvdata(dev);
	int ret = 0;

	if (f_phy_gce == 1) {
		pr_err("mtk_dprx_gce_phy_prepare fail\n");
		return -EPERM;
	}

	ret = cmdq_pkt_create(&dprx->gce_pkt);
	if (ret < 0) {
		dev_err(dev, "cmdq_pkt_create for dprx start failed\n");
		return ret;
	}
	ret = mtk_dprx_gce_phy_config(dev, dprx->gce_pkt);
	f_phy_gce = 1;
	if (ret < 0) {
		dev_err(dev, "mtk_dprx_gce_phy_config failed\n");
		mtk_dprx_gce_phy_unprepare(dev);
		return ret;
	}
	init_completion(&phy_handle_cb);
	ret = cmdq_pkt_flush_async(dprx->phy_client,
				   dprx->gce_pkt,
				   mtk_dprx_phy_handling_cb,
				   dprx);
	if (ret < 0) {
		dev_err(dev, "cmdq_pkt_flush_async failed\n");
		mtk_dprx_gce_phy_unprepare(dev);
		return ret;
	}
	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX PHY GCE flush unprepare
 * @param[in]
 *     dev: struct device *dev
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dprx_gce_phy_unprepare(const struct device *dev)
{
	struct mtk_dprx *dprx = dev_get_drvdata(dev);
	int ret = 0;

	if (f_phy_gce == 0) {
		pr_err("mtk_dprx_gce_phy_unprepare fail\n");
		return -EPERM;
	}
	ret = mtk_mutex_power_off(dprx->phy_mutex);
	ret = ret | mtk_mutex_put(dprx->phy_mutex);
	ret = ret | mtk_mmsys_cfg_power_off(dprx->mmsys_dev);
	f_phy_gce = 0;
	MTK_DPRX_WRITE32(ADDR_RX_GTC_PHASE_SKEW_OFFSET_0, 0x00);
	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX PHY GCE Create
 * @param[in]
 *     dev: struct device *dev
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dprx_gce_phy_create(const struct device *dev)
{
	struct mtk_dprx *dprx = dev_get_drvdata(dev);
	int ret = 0;

	ret = mtk_dprx_cmdq_mbox_create(dprx);
	if (ret == 0)
		ret = mtk_dprx_gce_phy_prepare(dev);
	else
		return ret;

	if (ret < 0) {
		pr_err("mtk_dprx_gce_phy_create fail!\n");
		mtk_dprx_cmdq_mbox_destroy(dprx);
	}
	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX PHY GCE Destroy
 * @param[in]
 *     dev: struct device *dev
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dprx_gce_phy_destroy(const struct device *dev)
{
	struct mtk_dprx *dprx = dev_get_drvdata(dev);
	int ret = 0;

	if (f_phy_gce == 0) {
		pr_err("No need to destroy\n");
		return -EPERM;
	}

	ret = mtk_dprx_cmdq_mbox_destroy(dprx);
	ret = ret | mtk_dprx_gce_phy_unprepare(dev);
	ret = ret | mtk_dprx_phy_handling_cb_complete();
	return ret;
}
#endif
