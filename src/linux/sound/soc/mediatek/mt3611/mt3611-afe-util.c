/**
 * @file mt3611-afe-util.c
 * Mediatek audio utility
 *
 * Copyright (c) 2017 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "mt3611-afe-common.h"
#include "mt3611-afe-regs.h"
#include "mt3611-afe-util.h"
#include <linux/device.h>

/** @ingroup type_group_afe_InFn
 * @par Description
 *     get top clock gate mask.
 * @param[in]
 *     cg_type: clock gate type
 * @return
 *     clock gate mask.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static unsigned int get_top_cg_mask(unsigned int cg_type)
{
	switch (cg_type) {
	case MT3611_AFE_CG_AFE:
		return AUD_TCON0_PDN_AFE;
	case MT3611_AFE_CG_TDM_OUT:
		return AUD_TCON0_PDN_TDM_OUT;
	case MT3611_AFE_CG_DP_RX:
		return AUD_TCON0_PDN_DP_RX;
#ifdef HDMI_ENABLE
	case MT3611_AFE_CG_HDMI_RX:
		return AUD_TCON0_PDN_HDMI_RX;
#endif
	case MT3611_AFE_CG_A1SYS:
		return AUD_TCON0_PDN_A1SYS;
	case MT3611_AFE_CG_A2SYS:
		return AUD_TCON0_PDN_A2SYS;
	case MT3611_AFE_CG_A3SYS:
		return AUD_TCON0_PDN_A3SYS;
	case MT3611_AFE_CG_TDM_IN:
		return AUD_TCON0_PDN_TDM_IN;
	case MT3611_AFE_CG_I2S_OUT:
		return AUD_TCON1_PDN_I2S_OUT_CLK;
	case MT3611_AFE_CG_I2S_IN:
		return AUD_TCON1_PDN_I2S_IN_CLK;
	default:
		return 0;
	}
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     get top clock gate register.
 * @param[in]
 *     cg_type: clock gate type
 * @return
 *     clock gate register.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static unsigned int get_top_cg_reg(unsigned int cg_type)
{
	switch (cg_type) {
	case MT3611_AFE_CG_I2S_OUT:
	case MT3611_AFE_CG_I2S_IN:
		return AUDIO_TOP_CON1;
	default:
		return AUDIO_TOP_CON0;
	}
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     get engen mask.
 * @param[in]
 *     engen_type: engen type
 * @return
 *     engen mask.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static unsigned int get_engen_mask(unsigned int engen_type)
{
	switch (engen_type) {
	case MT3611_AFE_ENGEN_A1SYS:
		return AFE_FS_TIMING_CON_A1SYS_ENGEN_EN;
	case MT3611_AFE_ENGEN_A2SYS:
		return AFE_FS_TIMING_CON_A2SYS_ENGEN_EN;
	case MT3611_AFE_ENGEN_A3SYS:
		return AFE_FS_TIMING_CON_A3SYS_ENGEN_EN;
	default:
		return 0;
	}
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     get memory interface's source clock.
 * @param[in]
 *     ctrl: control data
 * @param[in]
 *     id: memory interface id
 * @return
 *     clock source.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int get_memif_clk_src(struct mt3611_afe_control_data *ctrl, int id)
{
	switch (id) {
	case MT3611_AFE_MEMIF_DL12:
		return ctrl->dl12_clk_src;
	case MT3611_AFE_MEMIF_VUL12:
		return ctrl->vul12_clk_src;
	default:
		break;
	}

	return CLK_GET_FALSE;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     probe function for struct snd_soc_platform_driver.
 * @param[in]
 *     ctrl: control data
 * @param[in]
 *     irq_mode: irq mode
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int get_common_irq_clk_src(struct mt3611_afe_control_data *ctrl,
				  int irq_mode)
{
	switch (irq_mode) {
	case MT3611_AFE_IRQ_0:
	case MT3611_AFE_IRQ_1:
	case MT3611_AFE_IRQ_2:
	case MT3611_AFE_IRQ_3:
		return ctrl->common_irq_clk_src[irq_mode];
	default:
		break;
	}

	return CLK_GET_FALSE;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     enable top clock gate.
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     cg_type: clock gate type
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mt3611_afe_enable_top_cg(struct mtk_afe *afe, unsigned int cg_type)
{
	unsigned int mask = get_top_cg_mask(cg_type);
	unsigned int reg = get_top_cg_reg(cg_type);
	unsigned long flags;

	spin_lock_irqsave(&afe->afe_ctrl_lock, flags);

	afe->top_cg_ref_cnt[cg_type]++;
	if (afe->top_cg_ref_cnt[cg_type] == 1)
		regmap_update_bits(afe->regmap, reg, mask, 0x0);

	spin_unlock_irqrestore(&afe->afe_ctrl_lock, flags);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     disable top clock gate.
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     cg_type: clock gate type
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mt3611_afe_disable_top_cg(struct mtk_afe *afe, unsigned cg_type)
{
	unsigned int mask = get_top_cg_mask(cg_type);
	unsigned int reg = get_top_cg_reg(cg_type);
	unsigned long flags;

	spin_lock_irqsave(&afe->afe_ctrl_lock, flags);

	afe->top_cg_ref_cnt[cg_type]--;
	if (afe->top_cg_ref_cnt[cg_type] == 0)
		regmap_update_bits(afe->regmap, reg, mask, mask);
	else if (afe->top_cg_ref_cnt[cg_type] < 0)
		afe->top_cg_ref_cnt[cg_type] = 0;

	spin_unlock_irqrestore(&afe->afe_ctrl_lock, flags);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     enable main clock.
 * @param[in]
 *     afe: AFE data
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mt3611_afe_enable_main_clk(struct mtk_afe *afe)
{
#if defined(COMMON_CLOCK_FRAMEWORK_API)
	int ret;

	ret = clk_prepare_enable(afe->clocks[MT3611_CLK_AUD_MAS_SLV_BCLK]);
	if (ret)
		goto err;

	ret = clk_prepare_enable(afe->clocks[MT3611_CLK_AUD_INTBUS_CG]);
	if (ret)
		goto err;
#endif

	mt3611_afe_enable_top_cg(afe, MT3611_AFE_CG_AFE);
	return 0;

#if defined(COMMON_CLOCK_FRAMEWORK_API)
err:
	dev_err(afe->dev, "%s failed %d\n", __func__, ret);
	return ret;
#endif
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     disable main clock.
 * @param[in]
 *     afe: AFE data
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mt3611_afe_disable_main_clk(struct mtk_afe *afe)
{
	mt3611_afe_disable_top_cg(afe, MT3611_AFE_CG_AFE);
#if defined(COMMON_CLOCK_FRAMEWORK_API)
	clk_disable_unprepare(afe->clocks[MT3611_CLK_AUD_INTBUS_CG]);
	clk_disable_unprepare(afe->clocks[MT3611_CLK_AUD_MAS_SLV_BCLK]);
#endif
	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     enable audio clocks.
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     m_ck: mclk handle
 * @param[in]
 *     b_ck: bck handle
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mt3611_afe_enable_clks(struct mtk_afe *afe,
			   struct clk *m_ck,
			   struct clk *b_ck)
{
#ifdef COMMON_CLOCK_FRAMEWORK_API
	int ret;

	if (m_ck) {
		ret = clk_prepare_enable(m_ck);
		if (ret) {
			dev_err(afe->dev, "Failed to enable m_ck\n");
			return ret;
		}
	}

	if (b_ck) {
		ret = clk_prepare_enable(b_ck);
		if (ret) {
			dev_err(afe->dev, "Failed to enable b_ck\n");
			return ret;
		}
	}
#endif
	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     set audio clock rate.
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     m_ck: mclk handle
 * @param[in]
 *     mck_rate: mclk rate
 * @param[in]
 *     b_ck: bck handle
 * @param[in]
 *     bck_rate: bck rate
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mt3611_afe_set_clks(struct mtk_afe *afe,
			struct clk *m_ck,
			unsigned int mck_rate,
			struct clk *b_ck,
			unsigned int bck_rate)
{
#ifdef COMMON_CLOCK_FRAMEWORK_API
	int ret;

	if (m_ck) {
		ret = clk_set_rate(m_ck, mck_rate);
		if (ret) {
			dev_err(afe->dev, "Failed to set m_ck rate\n");
			return ret;
		}
	}

	if (b_ck) {
		ret = clk_set_rate(b_ck, bck_rate);
		if (ret) {
			dev_err(afe->dev, "Failed to set b_ck rate\n");
			return ret;
		}
	}
#endif
	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     set parent clock source.
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     clk: clock handle
 * @param[in]
 *     parent: parent clock handle
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mt3611_afe_set_parent_clk(struct mtk_afe *afe,
			      struct clk *clk,
			      struct clk *parent)
{
#ifdef COMMON_CLOCK_FRAMEWORK_API
	int ret;

	ret = clk_set_parent(clk, parent);
	if (ret) {
		dev_err(afe->dev, "Failed to set parent clock %d\n", ret);
		return ret;
	}
#endif
	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     disable audio clocks.
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     m_ck: mclk handle
 * @param[in]
 *     b_ck: bck handle
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt3611_afe_disable_clks(struct mtk_afe *afe,
			     struct clk *m_ck,
			     struct clk *b_ck)
{
#ifdef COMMON_CLOCK_FRAMEWORK_API
	if (m_ck)
		clk_disable_unprepare(m_ck);
	if (b_ck)
		clk_disable_unprepare(b_ck);
#endif
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     enable afe on.
 * @param[in]
 *     afe: AFE data
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mt3611_afe_enable_afe_on(struct mtk_afe *afe)
{
	unsigned long flags;

	spin_lock_irqsave(&afe->afe_ctrl_lock, flags);

	afe->afe_on_ref_cnt++;
	if (afe->afe_on_ref_cnt == 1)
		regmap_update_bits(afe->regmap, AFE_DAC_CON0, 0x1, 0x1);

	spin_unlock_irqrestore(&afe->afe_ctrl_lock, flags);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     disable afe on.
 * @param[in]
 *     afe: AFE data
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mt3611_afe_disable_afe_on(struct mtk_afe *afe)
{
	unsigned long flags;

	spin_lock_irqsave(&afe->afe_ctrl_lock, flags);

	afe->afe_on_ref_cnt--;
	if (afe->afe_on_ref_cnt == 0)
		regmap_update_bits(afe->regmap, AFE_DAC_CON0, 0x1, 0x0);
	else if (afe->afe_on_ref_cnt < 0)
		afe->afe_on_ref_cnt = 0;

	spin_unlock_irqrestore(&afe->afe_ctrl_lock, flags);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     enable a1sys and engen clock.
 * @param[in]
 *     afe: AFE data
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mt3611_afe_enable_a1sys_associated_clk(struct mtk_afe *afe)
{
	mt3611_afe_enable_clks(afe,
			       afe->clocks[MT3611_CLK_A1SYS_HP_CG],
			       NULL);

	mt3611_afe_enable_top_cg(afe, MT3611_AFE_CG_A1SYS);

	mt3611_afe_enable_engen(afe, MT3611_AFE_ENGEN_A1SYS);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     disable a1sys and engen clock.
 * @param[in]
 *     afe: AFE data
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mt3611_afe_disable_a1sys_associated_clk(struct mtk_afe *afe)
{
	mt3611_afe_disable_engen(afe, MT3611_AFE_ENGEN_A1SYS);

	mt3611_afe_disable_top_cg(afe, MT3611_AFE_CG_A1SYS);

	mt3611_afe_disable_clks(afe,
				afe->clocks[MT3611_CLK_A1SYS_HP_CG],
				NULL);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     enable a2sys and engen clock.
 * @param[in]
 *     afe: AFE data
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mt3611_afe_enable_a2sys_associated_clk(struct mtk_afe *afe)
{
	mt3611_afe_enable_clks(afe,
			       afe->clocks[MT3611_CLK_A2SYS_HP_CG],
			       NULL);

	mt3611_afe_enable_top_cg(afe, MT3611_AFE_CG_A2SYS);

	mt3611_afe_enable_engen(afe, MT3611_AFE_ENGEN_A2SYS);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     disable a2sys and engen clock.
 * @param[in]
 *     afe: AFE data
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mt3611_afe_disable_a2sys_associated_clk(struct mtk_afe *afe)
{
	mt3611_afe_disable_engen(afe, MT3611_AFE_ENGEN_A2SYS);

	mt3611_afe_disable_top_cg(afe, MT3611_AFE_CG_A2SYS);

	mt3611_afe_disable_clks(afe,
				afe->clocks[MT3611_CLK_A2SYS_HP_CG],
				NULL);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     enable a3sys and engen clock.
 * @param[in]
 *     afe: AFE data
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mt3611_afe_enable_a3sys_associated_clk(struct mtk_afe *afe)
{
	mt3611_afe_enable_clks(afe,
			       afe->clocks[MT3611_CLK_A3SYS_HP_CG],
			       NULL);

	mt3611_afe_enable_top_cg(afe, MT3611_AFE_CG_A3SYS);

	mt3611_afe_enable_engen(afe, MT3611_AFE_ENGEN_A3SYS);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     disable a3sys and engen clock.
 * @param[in]
 *     afe: AFE data
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mt3611_afe_disable_a3sys_associated_clk(struct mtk_afe *afe)
{
	mt3611_afe_disable_engen(afe, MT3611_AFE_ENGEN_A3SYS);

	mt3611_afe_disable_top_cg(afe, MT3611_AFE_CG_A3SYS);

	mt3611_afe_disable_clks(afe,
				afe->clocks[MT3611_CLK_A3SYS_HP_CG],
				NULL);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     enable engen.
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     engen_type: engen type
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mt3611_afe_enable_engen(struct mtk_afe *afe, unsigned int engen_type)
{
	unsigned int mask = get_engen_mask(engen_type);
	unsigned long flags;

	spin_lock_irqsave(&afe->afe_ctrl_lock, flags);

	afe->engen_ref_cnt[engen_type]++;
	if (afe->engen_ref_cnt[engen_type] == 1)
		regmap_update_bits(afe->regmap, AFE_FS_TIMING_CON, mask, mask);

	spin_unlock_irqrestore(&afe->afe_ctrl_lock, flags);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     disable engen.
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     engen_type: engen type
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mt3611_afe_disable_engen(struct mtk_afe *afe, unsigned int engen_type)
{
	unsigned int mask = get_engen_mask(engen_type);
	unsigned long flags;

	spin_lock_irqsave(&afe->afe_ctrl_lock, flags);

	afe->engen_ref_cnt[engen_type]--;
	if (afe->engen_ref_cnt[engen_type] == 0)
		regmap_update_bits(afe->regmap, AFE_FS_TIMING_CON, mask, 0x0);
	else if (afe->engen_ref_cnt[engen_type] < 0)
		afe->engen_ref_cnt[engen_type] = 0;

	spin_unlock_irqrestore(&afe->afe_ctrl_lock, flags);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     enable the source clock of memory interface and irq.
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     id: memory interface id
 * @param[in]
 *     irq_mode: irq mode
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mt3611_afe_enable_memif_irq_src_clk(struct mtk_afe *afe,
					int id,
					int irq_mode)
{
	struct mt3611_afe_control_data *ctrl = &afe->ctrl_data;
	int memif_clk_src = get_memif_clk_src(ctrl, id);
	int irq_clk_src = get_common_irq_clk_src(ctrl, irq_mode);
	const bool need_a1sys = true;
	bool need_a2sys = false;
	bool need_a3sys = false;

	switch (memif_clk_src) {
	case FROM_AUDIO_PLL:
		need_a2sys = true;
		break;
#ifdef HDMI_ENABLE
	case FROM_HDMIRX_PLL:
#endif
	case FROM_DPRX_PLL:
		need_a3sys = true;
		break;
	default:
		break;
	}

	switch (irq_clk_src) {
	case FROM_AUDIO_PLL:
		need_a2sys = true;
		break;
#ifdef HDMI_ENABLE
	case FROM_HDMIRX_PLL:
#endif
	case FROM_DPRX_PLL:
		need_a3sys = true;
		break;
	default:
		break;
	}

	if (need_a1sys)
		mt3611_afe_enable_a1sys_associated_clk(afe);

	if (need_a2sys)
		mt3611_afe_enable_a2sys_associated_clk(afe);

	if (need_a3sys) {
		mt3611_afe_enable_a3sys_associated_clk(afe);

		if ((memif_clk_src == FROM_DPRX_PLL) ||
		    (irq_clk_src == FROM_DPRX_PLL)) {
			mt3611_afe_set_parent_clk(afe,
				afe->clocks[MT3611_CLK_A3SYS_HP_SEL],
				afe->clocks[MT3611_CLK_DPAPLL_D3]);
#ifdef HDMI_ENABLE
		} else if ((memif_clk_src == FROM_HDMIRX_PLL) ||
			   (irq_clk_src == FROM_HDMIRX_PLL)) {
			mt3611_afe_set_parent_clk(afe,
				afe->clocks[MT3611_CLK_A3SYS_HP_SEL],
				afe->clocks[MT3611_CLK_HDMIPLL_D6]);
		}
#else
		}
#endif
	}

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     dsiable the source clock of memory interface and irq.
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     id: memory interface id
 * @param[in]
 *     irq_mode: irq mode
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mt3611_afe_disable_memif_irq_src_clk(struct mtk_afe *afe,
					 int id,
					 int irq_mode)
{
	struct mt3611_afe_control_data *ctrl = &afe->ctrl_data;
	int memif_clk_src = get_memif_clk_src(ctrl, id);
	int irq_clk_src = get_common_irq_clk_src(ctrl, irq_mode);
	const bool need_a1sys = true;
	bool need_a2sys = false;
	bool need_a3sys = false;

	switch (memif_clk_src) {
	case FROM_AUDIO_PLL:
		need_a2sys = true;
		break;
#ifdef HDMI_ENABLE
	case FROM_HDMIRX_PLL:
#endif
	case FROM_DPRX_PLL:
		need_a3sys = true;
		break;
	default:
		break;
	}

	switch (irq_clk_src) {
	case FROM_AUDIO_PLL:
		need_a2sys = true;
		break;
#ifdef HDMI_ENABLE
	case FROM_HDMIRX_PLL:
#endif
	case FROM_DPRX_PLL:
		need_a3sys = true;
		break;
	default:
		break;
	}

	if (need_a1sys)
		mt3611_afe_disable_a1sys_associated_clk(afe);

	if (need_a2sys)
		mt3611_afe_disable_a2sys_associated_clk(afe);

	if (need_a3sys)
		mt3611_afe_disable_a3sys_associated_clk(afe);

	return 0;
}

