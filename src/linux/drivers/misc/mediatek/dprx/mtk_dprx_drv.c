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
 * @defgroup IP_group_dprx DP (DisplayPort) RX
 *     DisplayPort is an open industry standard from VESA that accommodates\n
 *     the broad adoption of digital display technology within the PC and\n
 *     consumer electronics industries. It offers the highest resolution and\n
 *     power performance for both internal (panels) and external displays\n
 *     (monitors and televisions).\n
 *     The DP Receiver  consists of DisplayPort Main Link, AUX Channel, HPD\n
 *     signal. The Main Link is a unidirectional channel used to transport\n
 *     isochronous data streams accepts 1,2 or 4 lanes at either 1.62, 2.7,\n
 *     5.4 or 8.1 Gbps link rate, in compliance with the DisplayPort v.1.4\n
 *     specification. The auxiliary channel acts as a bidirectional\n
 *     communication link, supporting the protocol with Native AUX and I2C\n
 *     over AUX, as well as the dedicated DisplayPort link training and\n
 *     device manage functions.\n
 *     The DP Receiver features HDCP1.3/HDCP2.2 content protection scheme,\n
 *     for secure reception of digital Audio-Video content. The DP receiver\n
 *     also supports the Display Stream Decompression (DSC) feature, which\n
 *     complies with the VESA  DSC1.2a specification, besides provides the\n
 *     error correction during compressed Video data transport by Forword\n
 *     Error Correction(FEC).\n
 *
 *    @{
 *       @defgroup IP_group_dprx_external EXTERNAL
 *         The external API document for dprx.\n
 *         The dprx driver follow native Linux framework
 *
 *         @{
 *            @defgroup IP_group_dprx_external_function 1.function
 *              This is DPRX External Function.
 *            @defgroup IP_group_dprx_external_struct 2.structure
 *              External structure used for DPRX driver.
 *            @defgroup IP_group_dprx_external_typedef 3.typedef
 *              none. No typedef in DPRX driver.
 *            @defgroup IP_group_dprx_external_enum 4.enumeration
 *              External enumeration used for DPRX driver.
 *            @defgroup IP_group_dprx_external_def 5.define
 *              External define used for DPRX driver.
 *         @}
 *
 *       @defgroup IP_group_dprx_internal INTERNAL
 *         The internal API document for dprx.\n
 *
 *         @{
 *            @defgroup IP_group_dprx_internal_function 1.function
 *              This is DPRX Internal Function.
 *            @defgroup IP_group_dprx_internal_struct 2.structure
 *              Internal structure used for DPRX driver.
 *            @defgroup IP_group_dprx_internal_typedef 3.typedef
 *              none. No typedef in DPRX driver.
 *            @defgroup IP_group_dprx_internal_enum 4.enumeration
 *              Internal enumeration used for DPRX driver.
 *            @defgroup IP_group_dprx_internal_def 5.define
 *              Internal define used for DPRX driver.
 *         @}
 *    @}
*/

/**
 * @file mtk_dprx_drv.c
 *
 *  DPRX driver operation related function
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
#include <linux/arm-smccc.h>

/* ====================== Global Variable ========================== */
bool f_hdcp2x_en;

/* ====================== Function ================================= */
#ifdef CONFIG_MTK_DPRX_EDID
u8 edid_mtk[] = {
	0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
	0x36, 0x8B, 0x19, 0x41, 0x4C, 0x35, 0x30, 0x30,
	0x29, 0x1B, 0x01, 0x04, 0xC5, 0x3C, 0x22, 0x78,
	0x2A, 0x27, 0x15, 0xAC, 0x51, 0x35, 0xB5, 0x26,
	0x0E, 0x50, 0x54, 0xA5, 0x4B, 0x00, 0xD1, 0x00,
	0xD1, 0xC0, 0xB3, 0x00, 0xA9, 0x40, 0x81, 0x80,
	0x81, 0x00, 0x71, 0x4F, 0xE1, 0xC0, 0x08, 0xE8,
	0x00, 0x30, 0xF2, 0x70, 0x5A, 0x80, 0x30, 0x20,
	0x35, 0x00, 0x55, 0x50, 0x21, 0x00, 0x00, 0x1A,
	0x00, 0x00, 0x00, 0xFF, 0x00, 0x48, 0x46, 0x4A,
	0x4A, 0x54, 0x37, 0x41, 0x44, 0x30, 0x30, 0x35,
	0x4C, 0x0A, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x4D,
	0x54, 0x4B, 0x20, 0x4D, 0x6F, 0x6E, 0x69, 0x74,
	0x6F, 0x72, 0x0A, 0x20, 0x00, 0x00, 0x00, 0xFD,
	0x00, 0x1D, 0x56, 0x1E, 0x8C, 0x36, 0x01, 0x0A,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x1D,
	0x02, 0x03, 0x24, 0xE1, 0x4C, 0x5F, 0x10, 0x1F,
	0x05, 0x14, 0x04, 0x13, 0x12, 0x11, 0x03, 0x02,
	0x01, 0x23, 0x09, 0x5F, 0x07, 0x83, 0x01, 0x00,
	0x00, 0xE3, 0x05, 0xFF, 0x01, 0xE6, 0x06, 0x07,
	0x01, 0x8B, 0x60, 0x11, 0xA3, 0x66, 0x00, 0xA0,
	0xF0, 0x70, 0x1F, 0x80, 0x30, 0x20, 0x35, 0x00,
	0x55, 0x50, 0x21, 0x00, 0x00, 0x1A, 0x56, 0x5E,
	0x00, 0xA0, 0xA0, 0xA0, 0x29, 0x50, 0x30, 0x20,
	0x35, 0x00, 0x55, 0x50, 0x21, 0x00, 0x00, 0x1A,
	0x11, 0x44, 0x00, 0xA0, 0x80, 0x00, 0x1F, 0x50,
	0x30, 0x20, 0x36, 0x00, 0x55, 0x50, 0x21, 0x00,
	0x00, 0x1A, 0xBF, 0x16, 0x00, 0xA0, 0x80, 0x38,
	0x13, 0x40, 0x30, 0x20, 0x3A, 0x00, 0x55, 0x50,
	0x21, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22,
};
#endif

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX initial related setting
 * @param[in]
 *     dprx: DPRX device structure
 * @param[in]
 *     hdcp2x: hdcp2x control enable/disable
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dprx_hw_init(struct mtk_dprx *dprx, bool hdcp2x)
{
	int ret = 0;

	pr_debug("dprx hw init\n");

	#if IS_ENABLED(CONFIG_PHY_MTK_DPRX_PHY)
	ret = dprx_phy_init(dprx);
	#endif
	dprx_lt_table_setting();
	MTK_DPRX_REG_WRITE_MASK(ADDR_PWD_SEL, 1, AUX_PWD_SW_EN_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_VID_STABLE_DET, 3, VSYNC_DET_CNT_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_RST_SEL_0, 1, RXTOP_AUTO_RST_FLDMASK);
	#ifdef CONFIG_MTK_DPRX_EDID
	ret = dprx_drv_edid_init(edid_mtk, sizeof(edid_mtk));
	#endif
	dprx_drv_dpcd_init();
	/* HDCP1.3 need to load key in secure mode first */
	dprx_drv_hdcp1x_init();
	/* HDCP2.2 need to load key & FW in secure mode first */
	dprx_drv_hdcp2x_init(hdcp2x);
	dprx_drv_dsc_init(dprx);

	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX deinit related setting
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
void mtk_dprx_hw_deinit(struct mtk_dprx *dprx)
{
	pr_debug("dprx hw deinit\n");

	#if IS_ENABLED(CONFIG_PHY_MTK_DPRX_PHY)
	dprx_phy_exit(dprx);
	#endif
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX DPCD initialization function
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
void dprx_drv_dpcd_init(void)
{
	pr_debug("dprx_drv_dpcd_init\n");
	MTK_DPRX_REG_WRITE_MASK(ADDR_SINK_COUNT,  1, SINK_COUNT_FLDMASK);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX HDCP1.3 key/ HDCP2.2 Key/ HDCP2.2 FW initialization function
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
void dprx_drv_hdcp_key_init(void)
{
	struct arm_smccc_res res;

	pr_debug("dprx_drv_hdcp_key_init\n");
	arm_smccc_smc(TSP_HDCP_DP_KEY, 0, 0, 0, 0, 0, 0, 0, &res);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX HDCP1.3 initialization function
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
void dprx_drv_hdcp1x_init(void)
{
	pr_debug("HDCP1.3 configure\n");
#ifdef CONFIG_MTK_DPRX_HDCP_KEY
	/* HDCP1.3 intr mask disable */
	MTK_DPRX_REG_WRITE_MASK(ADDR_HDCP_INT_MASK, 0,
				REG_LINK_FAIL_INT_FLDMASK);
	/* HDCP r0 available mask disable */
	MTK_DPRX_REG_WRITE_MASK(ADDR_HPD_IRQ_MASK_H, 0,
				R0_AVAILABLE_BEGIN_INTR_MASK_FLDMASK);
#endif
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX HDCP2.2 initialization function
 * @param[in]
 *     hdcp2x: hdcp2x control enable/disable
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprx_drv_hdcp2x_init(bool hdcp2x)
{
	pr_debug("HDCP2.2 configure\n");

	f_hdcp2x_en = hdcp2x;
	MTK_DPRX_REG_PHYD_WRITE_MASK(DPRX_MBIST_9C, 0,
		RG_MAC_HDCP22_SETTING_DONE_FLDMASK);
#ifdef CONFIG_MTK_DPRX_HDCP_KEY
	if (f_hdcp2x_en) {
		MTK_DPRX_REG_PHYD_WRITE_MASK(DPRX_MBIST_9C, 1,
			RG_MAC_HDCP22_SETTING_DONE_FLDMASK);
	}
#endif
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX HDCP2.2 enable function
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
void dprx_drv_hdcp2x_enable(void)
{
	pr_debug("HDCP2.2 enable fw\n");
#ifdef CONFIG_MTK_DPRX_HDCP_KEY
	if (f_hdcp2x_en) {
		MTK_DPRX_REG_WRITE_MASK(ADDR_HDCP2_CTRL,
					1,
					HDCP2_FW_EN_FLDMASK);
	}
#endif
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX HDCP2.2 disable function
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
void dprx_drv_hdcp2x_disable(void)
{
	pr_debug("HDCP2.2 disable fw\n");
	MTK_DPRX_REG_WRITE_MASK(ADDR_HDCP2_CTRL, 0, HDCP2_FW_EN_FLDMASK);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX HDCP2.2 authentication status clear.\n
 *     Authentication status should be cleared after unplug.
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
void dprx_drv_hdcp2x_auth_status_clear(void)
{
	pr_debug("HDCP2.2 authentication status clear\n");
#ifdef CONFIG_MTK_DPRX_HDCP_KEY
	MTK_DPRX_REG_WRITE_MASK(ADDR_HDCP2_CTRL, 0, HDCP2_FW_EN_FLDMASK);
	if (f_hdcp2x_en) {
		MTK_DPRX_REG_WRITE_MASK(ADDR_HDCP2_CTRL,
					1,
					HDCP2_FW_EN_FLDMASK);
	}
#endif
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX SW HPD ready control function.\n
 *     After DPRX initialization and related interrupts enabled.
 * @param[in]
 *     val: HPD ready control
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprx_drv_sw_hpd_ready_ctrl(bool val)
{
	pr_debug("dprx_drv_sw_hpd_ready_ctrl: %d\n", val);
	MTK_DPRX_REG_PHYD_WRITE_MASK(DPRX_WRAPCTL_28, val,
				     RG_MAC_WRAP_SW_HPD_RDY_FLDMASK);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX DSC initialization
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
void dprx_drv_dsc_init(struct mtk_dprx *dprx)
{
	pr_debug("DSC configure\n");
	MTK_DPRX_REG_WRITE_MASK(ADDR_MAIN_LINK_INTR3_MASK, 0,
				COMPRESSED_FRAME_RS_INTR_FLDMASK);
	MTK_DPRX_REG_WRITE_MASK(ADDR_MAIN_LINK_INTR3_MASK, 0,
				COMPRESSED_FRAME_FL_INTR_FLDMASK);
	if (dprx->early_porting) {
		MTK_DPRX_REG_WRITE_MASK(ADDR_MAIN_LINK_INTR3_MASK, 1,
					PPS_CHANGE_FLDMASK);
	} else {
		MTK_DPRX_REG_WRITE_MASK(ADDR_MAIN_LINK_INTR3_MASK, 0,
					PPS_CHANGE_FLDMASK);
	}
	MTK_DPRX_REG_PHYD_WRITE_MASK(DPRX_TXDETECT_04, 1,
				     RG_MAC_BYPASS_DPCD_DSC_EN_FLDMASK);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX link training table setting function
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
void dprx_lt_table_setting(void)
{
	pr_debug("DPRX link training table setting\n");

	MTK_DPRX_PHYD_WRITE32(DPRX_LT_50, 0x00066662);
	MTK_DPRX_PHYD_WRITE32(DPRX_LT_54, 0x00066662);
	MTK_DPRX_PHYD_WRITE32(DPRX_LT_58, 0x00066662);
	MTK_DPRX_PHYD_WRITE32(DPRX_LT_5C, 0x00066662);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX align status unlock interrupt enable function
 * @param[in]
 *     en: align status unlock interrupt enable
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprx_align_status_unlock_intr_enable(bool en)
{
	bool temp;
	pr_debug("dprx_align_status_unlock_intr_enable = %d\n", en);

	temp = MTK_DPRX_REG_READ_MASK(ADDR_DPIP_INTR_MASK,
				      ALIGN_STATUS_UNLOCK_INTR_FLDMASK);
	pr_debug("ALIGN_STATUS_UNLOCK_INTR_MASK = %d\n", temp);

	if (en && temp) {
		MTK_DPRX_REG_WRITE_MASK(ADDR_DPIP_INTR,
					1,
					ALIGN_STATUS_UNLOCK_INTR_FLDMASK);
		MTK_DPRX_REG_WRITE_MASK(ADDR_DPIP_INTR_MASK,
					0,
					ALIGN_STATUS_UNLOCK_INTR_FLDMASK);
	} else if ((!en) && (!temp)) {
		MTK_DPRX_REG_WRITE_MASK(ADDR_DPIP_INTR_MASK,
					1,
					ALIGN_STATUS_UNLOCK_INTR_FLDMASK);
	}
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX DPCD interrupt config function
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
void dprx_dpcd_int_config(void)
{
	pr_debug("dprx_dpcd_int_config\n");

	MTK_DPRX_REG_WRITE_MASK(ADDR_INTR_MASK, 0, DPCD_INTR_FLDMASK);
	mtk_dprx_write_mask(ADDR_DPCD_INT_1_MASK,
			    0,
			    BW_CHANGE_FLDMASK |
			    CLK_RC_TRAINING_FLDMASK |
			    EQ_TRAINING_FLDMASK,
			    0);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX PHY initialization function
 * @param[in]
 *     dprx: DPRX device structure
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
int dprx_phy_init(struct mtk_dprx *dprx)
{
	int i;
	int ret;

	pr_debug("DPRX PHY initial\n");
	for (i = 0; i < dprx->num_phys; i++) {
		ret = phy_init(dprx->phys[i]);
		if (ret)
			goto exit_phy;
	}
	return 0;

exit_phy:
	for (; i > 0; i--)
		phy_exit(dprx->phys[i - 1]);

	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX PHY exit function
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
void dprx_phy_exit(struct mtk_dprx *dprx)
{
	int i;

	pr_debug("DPRX PHY exit\n");

	for (i = 0; i < dprx->num_phys; i++)
		phy_exit(dprx->phys[i]);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX PHY power on function
 * @param[in]
 *     dprx: DPRX device structure
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
int dprx_phy_power_on(struct mtk_dprx *dprx)
{
	int i;
	int ret;

	pr_debug("DPRX PHY power on\n");
	for (i = 0; i < dprx->num_phys; i++) {
		ret = phy_power_on(dprx->phys[i]);
		if (ret)
			goto power_off_phy;
	}
	return 0;

power_off_phy:
	for (; i > 0; i--)
		phy_power_off(dprx->phys[i - 1]);

	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX PHY power off function
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
void dprx_phy_power_off(struct mtk_dprx *dprx)
{
	int i;

	pr_debug("DPRX PHY power off\n");

	for (i = 0; i < dprx->num_phys; i++)
		phy_power_off(dprx->phys[i]);
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX 26m clock enable function
 * @param[in]
 *     dprx: DPRX device structure
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
int dprx_26m_clk_enable(struct mtk_dprx *dprx)
{
	int ret = 0;

#ifdef CONFIG_COMMON_CLK_MT3612
	ret = clk_prepare(dprx->ref_clk);
	if (ret != 0) {
		pr_info("clk_prepare fail error: %d\n", ret);
		ret = 1;
		return ret;
	}

	ret = clk_enable(dprx->ref_clk);
	if (ret != 0) {
		pr_info("clk_enable fail error: %d\n", ret);
		ret = 1;
	}
#endif

	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX 26m clock disable function
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
void dprx_26m_clk_disable(struct mtk_dprx *dprx)
{
#ifdef CONFIG_COMMON_CLK_MT3612
	clk_disable(dprx->ref_clk);
	clk_unprepare(dprx->ref_clk);
#endif
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX reset assert function
 * @param[in]
 *     dprx: DPRX device structure
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
int dprx_rst_assert(struct mtk_dprx *dprx)
{
	int ret = 0;

#ifdef CONFIG_COMMON_CLK_MT3612
	ret = reset_control_assert(dprx->rstc);
	ret = ret | reset_control_assert(dprx->apb_rstc);
#endif

	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX reset deassert function
 * @param[in]
 *     dprx: DPRX device structure
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
int dprx_rst_deassert(struct mtk_dprx *dprx)
{
	int ret = 0;

#ifdef CONFIG_COMMON_CLK_MT3612
	ret = reset_control_deassert(dprx->apb_rstc);
	ret = ret | reset_control_deassert(dprx->rstc);
#endif

	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX power on before work function
 * @par Parameters
 *     none
 * @return
 *     >= 0, function success.\n
 *     negative value, function fail.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_power_on_before_work(void)
{
#ifdef CONFIG_COMMON_CLK_MT3612
	return pm_runtime_get_sync(&my_pdev->dev);
#else
	return 0;
#endif
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX power off after work function
 * @par Parameters
 *     none
 * @return
 *     >= 0, function success.\n
 *     negative value, function fail.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprx_power_off_after_work(void)
{
#ifdef CONFIG_COMMON_CLK_MT3612
	return pm_runtime_put_sync(&my_pdev->dev);
#else
	return 0;
#endif
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX power on sequence before DP work function
 * @param[in]
 *     dprx: DPRX device structure
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
int dprx_power_on_seq(struct mtk_dprx *dprx)
{
	int ret = 0;

	ret = dprx_power_on_before_work();
	if (ret < 0) {
		ret = 1;
		return ret;
	}
	if (f_pwr_on_initial) {
		dprx_drv_hdcp_key_init();
		f_pwr_on_initial = 0;
	}

	ret = dprx_26m_clk_enable(dprx);
	if (ret < 0) {
		ret = 1;
		return ret;
	}
	ret = dprx_rst_deassert(dprx);

	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX power off sequence function
 * @param[in]
 *     dprx: DPRX device structure
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
int dprx_power_off_seq(struct mtk_dprx *dprx)
{
	int ret = 0;

	ret = dprx_rst_assert(dprx);
	if (ret < 0) {
		ret = 1;
		return ret;
	}
	dprx_26m_clk_disable(dprx);
	udelay(10);
	ret = dprx_power_off_after_work();
	if (ret < 0)
		ret = 1;
	return ret;
}
