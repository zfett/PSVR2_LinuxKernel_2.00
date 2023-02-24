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

#ifndef _MTK_DPRX_ISR_H_
#define _MTK_DPRX_ISR_H_

/**
 * @file mtk_dprx_isr_int.h
 * Header of internal DPRX isr function
 */

extern bool f_phy_gce;
extern bool f_hdcp2x_en;

/* ====================== Global Structure ========================= */
/** @brief extern dprx_video_info_s structure as dp_vid_status */
extern struct dprx_video_info_s dp_vid_status;
/** @brief extern video_stable_timer_s structure as dprx_vid_stable_chk */
extern struct video_stable_timer_s dprx_vid_stable_chk;

/** @ingroup IP_group_dprx_internal_struct
 * @brief DPRX callback event structure\n
 */
struct callback_sts_s {
	/** MSA update event */
	atomic_t msa_change;
	/** Audio MN change event */
	atomic_t aud_mn_change;
	/** DSC mode change event */
	atomic_t dsc_change;
	/** PPS change event */
	atomic_t pps_change;
	/** Unplug event */
	atomic_t unplug;
	/** Plugin event */
	atomic_t plugin;
	/** Video on event */
	atomic_t video_on;
	/** Video stable event */
	atomic_t video_stable;
	/** Video mute event */
	atomic_t video_mute;
	/** HDR InfoFrame changed*/
	atomic_t hdr_info_change;
	/** Audio InfoFrame changed*/
	atomic_t audio_info_change;
	/** Audio mute event */
	atomic_t audio_mute;
	/** Audio unmute event */
	atomic_t audio_unmute;
	/** Audio overflow event */
	atomic_t audio_overflow;
	/** VSC 3D type changed*/
	atomic_t stereo_type_change;
	/** BW changed*/
	atomic_t bw_change;
	/** SPD InfoFrame changed*/
	atomic_t spd_info_change;
	/** Link Error*/
	atomic_t link_error;
};

/* ====================== Function Prototype ======================= */
void dprx_disable_irq(void);
void dprx_enable_irq(void);
void dprx_drv_mainlink_intr_update(struct mtk_dprx *dprx);
void dprx_drv_dpip_intr_update(struct mtk_dprx *dprx);
void dprx_drv_dpcd_intr_update(struct mtk_dprx *dprx);
void dprx_drv_audio_intr_update(struct mtk_dprx *dprx);
void dprx_drv_video_on_intr_update(struct mtk_dprx *dprx);
void dprx_drv_unplug_intr_update(struct mtk_dprx *dprx);
int dprx_drv_plug_intr_update(struct mtk_dprx *dprx);
void dprx_init_events(void);
int32_t dprx_drv_is_hdr_info_change_event(void);
int32_t dprx_drv_clr_hdr_info_change_event(void);
int32_t dprx_drv_is_vsc_pkg_change_event(void);
int32_t dprx_drv_clr_vsc_pkg_change_event(void);
int32_t dprx_drv_is_audio_info_change_event(void);
int32_t dprx_drv_clr_audio_info_change_event(void);
int32_t dprx_drv_is_spd_info_change_event(void);
int32_t dprx_drv_clr_spd_info_change_event(void);
int32_t dprx_drv_is_video_mute_event(void);
int32_t dprx_drv_clr_video_mute_event(void);
int32_t dprx_drv_is_msa_change_event(void);
int32_t dprx_drv_clr_msa_change_event(void);
int32_t dprx_drv_is_maud_change_event(void);
int32_t dprx_drv_clr_maud_change_event(void);
int32_t dprx_drv_is_compress_fm_rs_event(void);
int32_t dprx_drv_clr_compress_fm_rs_event(void);
int32_t dprx_drv_is_compress_fm_fl_event(void);
int32_t dprx_drv_clr_compress_fm_fl_event(void);
int32_t dprx_drv_is_pps_change_event(void);
int32_t dprx_drv_clr_pps_change_event(void);
int32_t dprx_drv_is_align_status_unlock_event(void);
int32_t dprx_drv_clr_align_status_unlock_event(void);
int32_t dprx_drv_is_cr_training_event(void);
int32_t dprx_drv_clr_cr_training_event(void);
int32_t dprx_drv_is_eq_training_event(void);
int32_t dprx_drv_clr_eq_training_event(void);
int32_t dprx_drv_is_bw_change_event(void);
int32_t dprx_drv_clr_bw_change_event(void);
int32_t dprx_drv_is_aud_ch_status_change_event(void);
int32_t dprx_drv_clr_aud_ch_status_change_event(void);
int32_t dprx_drv_is_aud_vbid_mute_event(void);
int32_t dprx_drv_clr_aud_vbid_mute_event(void);
int32_t dprx_drv_is_aud_fifo_und_event(void);
int32_t dprx_drv_clr_aud_fifo_und_event(void);
int32_t dprx_drv_is_aud_fifo_over_event(void);
int32_t dprx_drv_clr_aud_fifo_over_event(void);
int32_t dprx_drv_is_aud_link_err_event(void);
int32_t dprx_drv_clr_aud_link_err_event(void);
int32_t dprx_drv_is_aud_dec_err_event(void);
int32_t dprx_drv_clr_aud_dec_err_event(void);
int32_t dprx_drv_is_aud_unmute_event(void);
int32_t dprx_drv_clr_aud_unmute_event(void);

irqreturn_t mtk_dprx_irq(int irq, void *dev_id);
int dprx_drv_main_kthread(void *dev_id);
unsigned int dprx_drv_task_init(struct mtk_dprx *dprx);
void dprx_drv_event_set(struct mtk_dprx *dprx);

#ifdef CONFIG_MTK_DPRX_PHY_GCE
int mtk_dprx_cmdq_mbox_create(struct mtk_dprx *dprx);
int mtk_dprx_cmdq_mbox_destroy(struct mtk_dprx *dprx);
int mtk_dprx_gce_phy_config(const struct device *dev,
			    struct cmdq_pkt *handle);
int mtk_dprx_phy_handling_cb_complete(void);
int mtk_dprx_gce_phy_prepare(const struct device *dev);
int mtk_dprx_gce_phy_unprepare(const struct device *dev);
int mtk_dprx_gce_phy_create(const struct device *dev);
int mtk_dprx_gce_phy_destroy(const struct device *dev);
#endif

#endif
