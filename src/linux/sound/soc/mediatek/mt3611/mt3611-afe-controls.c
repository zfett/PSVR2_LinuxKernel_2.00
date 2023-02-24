/**
 * @file mt3611-afe-controls.c
 * Mediatek Platform driver ALSA contorls
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

#include "mt3611-afe-controls.h"
#include "mt3611-afe-common.h"
#include "mt3611-afe-regs.h"
#include "mt3611-afe-util.h"
#include <sound/soc.h>

#define ENUM_TO_STR(enum) #enum

/** @ingroup type_group_afe_enum
 * @brief Mixer Control Function
 */
enum mixer_control_func {
	/** Sine Gen Enable */
	CTRL_SGEN_EN = 0,
	/** Sine Gen Frequency */
	CTRL_SGEN_FS,
	/** General Clock Source */
	CTRL_CLK_SRC,
	/** Mphone Multi Clock Source */
	CTRL_CLK_SRC_MPHONE,
	/** TDM Out Mode */
	CTRL_TDM_OUT_MODE,
};

/** @ingroup type_group_afe_enum
 * @brief Sine Gen Port
 */
enum afe_sgen_port {
	AFE_SGEN_OFF = 0,
	AFE_SGEN_O0,
	AFE_SGEN_O1,
	AFE_SGEN_O0O1,
	AFE_SGEN_O2,
	AFE_SGEN_O3,
	AFE_SGEN_O2O3,
	AFE_SGEN_O4,
	AFE_SGEN_O5,
	AFE_SGEN_O4O5,
	AFE_SGEN_O6,
	AFE_SGEN_O7,
	AFE_SGEN_O6O7,
	AFE_SGEN_AWB,
};

/** @ingroup type_group_afe_enum
 * @brief Sine Gen Frequency
 */
enum afe_sgen_freq {
	AFE_SGEN_8K = 0,
	AFE_SGEN_11K,
	AFE_SGEN_12K,
	AFE_SGEN_16K,
	AFE_SGEN_22K,
	AFE_SGEN_24K,
	AFE_SGEN_32K,
	AFE_SGEN_44K,
	AFE_SGEN_48K,
	AFE_SGEN_88K,
	AFE_SGEN_96K,
	AFE_SGEN_176K,
	AFE_SGEN_192K,
};

static const char *const sgen_func[] = {
	ENUM_TO_STR(AFE_SGEN_OFF),
	ENUM_TO_STR(AFE_SGEN_O0),
	ENUM_TO_STR(AFE_SGEN_O1),
	ENUM_TO_STR(AFE_SGEN_O0O1),
	ENUM_TO_STR(AFE_SGEN_O2),
	ENUM_TO_STR(AFE_SGEN_O3),
	ENUM_TO_STR(AFE_SGEN_O2O3),
	ENUM_TO_STR(AFE_SGEN_O4),
	ENUM_TO_STR(AFE_SGEN_O5),
	ENUM_TO_STR(AFE_SGEN_O4O5),
	ENUM_TO_STR(AFE_SGEN_O6),
	ENUM_TO_STR(AFE_SGEN_O7),
	ENUM_TO_STR(AFE_SGEN_O6O7),
	ENUM_TO_STR(AFE_SGEN_AWB),
};

static const char *const sgen_fs_func[] = {
	ENUM_TO_STR(AFE_SGEN_8K),
	ENUM_TO_STR(AFE_SGEN_11K),
	ENUM_TO_STR(AFE_SGEN_12K),
	ENUM_TO_STR(AFE_SGEN_16K),
	ENUM_TO_STR(AFE_SGEN_22K),
	ENUM_TO_STR(AFE_SGEN_24K),
	ENUM_TO_STR(AFE_SGEN_32K),
	ENUM_TO_STR(AFE_SGEN_44K),
	ENUM_TO_STR(AFE_SGEN_48K),
	ENUM_TO_STR(AFE_SGEN_88K),
	ENUM_TO_STR(AFE_SGEN_96K),
	ENUM_TO_STR(AFE_SGEN_176K),
	ENUM_TO_STR(AFE_SGEN_192K),
};

static const char *const clk_src_func[] = {
	ENUM_TO_STR(FROM_AUDIO_PLL),
#ifdef HDMI_ENABLE
	ENUM_TO_STR(FROM_HDMIRX_PLL),
#endif
	ENUM_TO_STR(FROM_DPRX_PLL),
};

static const char *const clk_src_mphone_multi_func[] = {
#ifdef HDMI_ENABLE
	ENUM_TO_STR(MPHONE_FROM_HDMIRX),
#endif
	ENUM_TO_STR(MPHONE_FROM_DPRX),
	ENUM_TO_STR(MPHONE_FROM_TDM_OUT),
};

static const char *const tdm_out_modes[] = {
	ENUM_TO_STR(AFE_TDM_OUT_NORMAL),
	ENUM_TO_STR(AFE_TDM_OUT_TO_MPHONE),
};

/** @ingroup type_group_afe_InFn
 * @par Description
 *     get sine gen type.
 * @param[in]
 *     kcontrol: mixer control
 * @param[in]
 *     ucontrol: control element information
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_sgen_get(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt3611_afe_control_data *data = &afe->ctrl_data;

	ucontrol->value.integer.value[0] = data->sinegen_type;

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     set sine gen type.
 * @param[in]
 *     kcontrol: mixer control
 * @param[in]
 *     ucontrol: control element information
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_sgen_put(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt3611_afe_control_data *data = &afe->ctrl_data;
	unsigned int new_sgen_type = ucontrol->value.integer.value[0];

	if (data->sinegen_type == new_sgen_type)
		return 0;

	mt3611_afe_enable_main_clk(afe);

	if (data->sinegen_type != AFE_SGEN_OFF) {
		regmap_update_bits(afe->regmap, AFE_SGEN_CON0, 1 << 26, 0x0);
		mt3611_afe_disable_a1sys_associated_clk(afe);
		mt3611_afe_disable_a2sys_associated_clk(afe);
	}

	switch (data->sinegen_type) {
	case AFE_SGEN_O0:
		regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
				   AFE_CONN_MUX_O0_MASK, AFE_CONN_MUX_O0_I0);
		break;
	case AFE_SGEN_O1:
		regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
				   AFE_CONN_MUX_O1_MASK, AFE_CONN_MUX_O1_I1);
		break;
	case AFE_SGEN_O0O1:
		regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
				   AFE_CONN_MUX_O0_MASK | AFE_CONN_MUX_O1_MASK,
				   AFE_CONN_MUX_O0_I0 | AFE_CONN_MUX_O1_I1);
		break;
	case AFE_SGEN_O2:
		regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
				   AFE_CONN_MUX_O2_MASK, AFE_CONN_MUX_O2_I2);
		break;
	case AFE_SGEN_O3:
		regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
				   AFE_CONN_MUX_O3_MASK, AFE_CONN_MUX_O3_I3);
		break;
	case AFE_SGEN_O2O3:
		regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
				   AFE_CONN_MUX_O2_MASK | AFE_CONN_MUX_O3_MASK,
				   AFE_CONN_MUX_O2_I2 | AFE_CONN_MUX_O3_I3);
		break;
	case AFE_SGEN_O4:
		regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
				   AFE_CONN_MUX_O4_MASK, AFE_CONN_MUX_O4_I4);
		break;
	case AFE_SGEN_O5:
		regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
				   AFE_CONN_MUX_O5_MASK, AFE_CONN_MUX_O5_I5);
		break;
	case AFE_SGEN_O4O5:
		regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
				   AFE_CONN_MUX_O4_MASK | AFE_CONN_MUX_O5_MASK,
				   AFE_CONN_MUX_O4_I4 | AFE_CONN_MUX_O5_I5);
		break;
	case AFE_SGEN_O6:
		regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
				   AFE_CONN_MUX_O6_MASK, AFE_CONN_MUX_O6_I6);
		break;
	case AFE_SGEN_O7:
		regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
				   AFE_CONN_MUX_O7_MASK, AFE_CONN_MUX_O7_I7);
		break;
	case AFE_SGEN_O6O7:
		regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
				   AFE_CONN_MUX_O6_MASK | AFE_CONN_MUX_O7_MASK,
				   AFE_CONN_MUX_O6_I6 | AFE_CONN_MUX_O7_I7);
		break;
	case AFE_SGEN_AWB:
		regmap_update_bits(afe->regmap, AFE_SGEN_CON2,
				   AFE_SGEN_CON2_EN_MASK, 0x0);
		break;
	default:
		break;
	}

	switch (new_sgen_type) {
	case AFE_SGEN_O0:
		regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
				   AFE_CONN_MUX_O0_MASK, AFE_CONN_MUX_O0_I8);
		break;
	case AFE_SGEN_O1:
		regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
				   AFE_CONN_MUX_O1_MASK, AFE_CONN_MUX_O1_I9);
		break;
	case AFE_SGEN_O0O1:
		regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
				   AFE_CONN_MUX_O0_MASK | AFE_CONN_MUX_O1_MASK,
				   AFE_CONN_MUX_O0_I8 | AFE_CONN_MUX_O1_I9);
		break;
	case AFE_SGEN_O2:
		regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
				   AFE_CONN_MUX_O2_MASK, AFE_CONN_MUX_O2_I8);
		break;
	case AFE_SGEN_O3:
		regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
				   AFE_CONN_MUX_O3_MASK, AFE_CONN_MUX_O3_I9);
		break;
	case AFE_SGEN_O2O3:
		regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
				   AFE_CONN_MUX_O2_MASK | AFE_CONN_MUX_O3_MASK,
				   AFE_CONN_MUX_O2_I8 | AFE_CONN_MUX_O3_I9);
		break;
	case AFE_SGEN_O4:
		regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
				   AFE_CONN_MUX_O4_MASK, AFE_CONN_MUX_O4_I8);
		break;
	case AFE_SGEN_O5:
		regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
				   AFE_CONN_MUX_O5_MASK, AFE_CONN_MUX_O5_I9);
		break;
	case AFE_SGEN_O4O5:
		regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
				   AFE_CONN_MUX_O4_MASK | AFE_CONN_MUX_O5_MASK,
				   AFE_CONN_MUX_O4_I8 | AFE_CONN_MUX_O5_I9);
		break;
	case AFE_SGEN_O6:
		regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
				   AFE_CONN_MUX_O6_MASK, AFE_CONN_MUX_O6_I8);
		break;
	case AFE_SGEN_O7:
		regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
				   AFE_CONN_MUX_O7_MASK, AFE_CONN_MUX_O7_I9);
		break;
	case AFE_SGEN_O6O7:
		regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
				   AFE_CONN_MUX_O6_MASK | AFE_CONN_MUX_O7_MASK,
				   AFE_CONN_MUX_O6_I8 | AFE_CONN_MUX_O7_I9);
		break;
	case AFE_SGEN_AWB:
		regmap_update_bits(afe->regmap, AFE_SGEN_CON2,
				   AFE_SGEN_CON2_EN_MASK,
				   AFE_SGEN_CON2_EN);
		break;
	default:
		regmap_update_bits(afe->regmap, AFE_SGEN_CON0, 1 << 26, 0x0);
		mt3611_afe_disable_a1sys_associated_clk(afe);
		mt3611_afe_disable_a2sys_associated_clk(afe);
		break;
	}

	if (new_sgen_type != AFE_SGEN_OFF) {
		mt3611_afe_enable_a1sys_associated_clk(afe);
		mt3611_afe_enable_a2sys_associated_clk(afe);
		regmap_update_bits(afe->regmap, AFE_SGEN_CON0,
				   AFE_SGEN_CON0_EN_MASK,
				   AFE_SGEN_CON0_EN);
	}

	mt3611_afe_disable_main_clk(afe);

	data->sinegen_type = new_sgen_type;

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     get sine gen frequency.
 * @param[in]
 *     kcontrol: mixer control
 * @param[in]
 *     ucontrol: control element information
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_sgen_fs_get(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt3611_afe_control_data *data = &afe->ctrl_data;

	ucontrol->value.integer.value[0] = data->sinegen_fs;

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     set sine gen frequency.
 * @param[in]
 *     kcontrol: mixer control
 * @param[in]
 *     ucontrol: control element information
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_sgen_fs_put(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt3611_afe_control_data *data = &afe->ctrl_data;

	mt3611_afe_enable_main_clk(afe);

	switch (ucontrol->value.integer.value[0]) {
	case AFE_SGEN_8K:
		regmap_update_bits(afe->regmap, AFE_SGEN_CON0,
				   AFE_SGEN_CON0_SINE_CH1_MASK |
				   AFE_SGEN_CON0_SINE_CH2_MASK,
				   AFE_SGEN_CON0_SINE_CH1_8K |
				   AFE_SGEN_CON0_SINE_CH2_8K);
		break;
	case AFE_SGEN_11K:
		regmap_update_bits(afe->regmap, AFE_SGEN_CON0,
				   AFE_SGEN_CON0_SINE_CH1_MASK |
				   AFE_SGEN_CON0_SINE_CH2_MASK,
				   AFE_SGEN_CON0_SINE_CH1_11K |
				   AFE_SGEN_CON0_SINE_CH2_11K);
		break;
	case AFE_SGEN_12K:
		regmap_update_bits(afe->regmap, AFE_SGEN_CON0,
				   AFE_SGEN_CON0_SINE_CH1_MASK |
				   AFE_SGEN_CON0_SINE_CH2_MASK,
				   AFE_SGEN_CON0_SINE_CH1_12K |
				   AFE_SGEN_CON0_SINE_CH2_12K);
		break;
	case AFE_SGEN_16K:
		regmap_update_bits(afe->regmap, AFE_SGEN_CON0,
				   AFE_SGEN_CON0_SINE_CH1_MASK |
				   AFE_SGEN_CON0_SINE_CH2_MASK,
				   AFE_SGEN_CON0_SINE_CH1_16K |
				   AFE_SGEN_CON0_SINE_CH2_16K);
		break;
	case AFE_SGEN_22K:
		regmap_update_bits(afe->regmap, AFE_SGEN_CON0,
				   AFE_SGEN_CON0_SINE_CH1_MASK |
				   AFE_SGEN_CON0_SINE_CH2_MASK,
				   AFE_SGEN_CON0_SINE_CH1_22K |
				   AFE_SGEN_CON0_SINE_CH2_22K);
		break;
	case AFE_SGEN_24K:
		regmap_update_bits(afe->regmap, AFE_SGEN_CON0,
				   AFE_SGEN_CON0_SINE_CH1_MASK |
				   AFE_SGEN_CON0_SINE_CH2_MASK,
				   AFE_SGEN_CON0_SINE_CH1_24K |
				   AFE_SGEN_CON0_SINE_CH2_24K);
		break;
	case AFE_SGEN_32K:
		regmap_update_bits(afe->regmap, AFE_SGEN_CON0,
				   AFE_SGEN_CON0_SINE_CH1_MASK |
				   AFE_SGEN_CON0_SINE_CH2_MASK,
				   AFE_SGEN_CON0_SINE_CH1_32K |
				   AFE_SGEN_CON0_SINE_CH2_32K);
		break;
	case AFE_SGEN_44K:
		regmap_update_bits(afe->regmap, AFE_SGEN_CON0,
				   AFE_SGEN_CON0_SINE_CH1_MASK |
				   AFE_SGEN_CON0_SINE_CH2_MASK,
				   AFE_SGEN_CON0_SINE_CH1_44K |
				   AFE_SGEN_CON0_SINE_CH2_44K);
		break;
	case AFE_SGEN_48K:
		regmap_update_bits(afe->regmap, AFE_SGEN_CON0,
				   AFE_SGEN_CON0_SINE_CH1_MASK |
				   AFE_SGEN_CON0_SINE_CH2_MASK,
				   AFE_SGEN_CON0_SINE_CH1_48K |
				   AFE_SGEN_CON0_SINE_CH2_48K);
		break;
	case AFE_SGEN_88K:
		regmap_update_bits(afe->regmap, AFE_SGEN_CON0,
				   AFE_SGEN_CON0_SINE_CH1_MASK |
				   AFE_SGEN_CON0_SINE_CH2_MASK,
				   AFE_SGEN_CON0_SINE_CH1_88K |
				   AFE_SGEN_CON0_SINE_CH2_88K);
		break;
	case AFE_SGEN_96K:
		regmap_update_bits(afe->regmap, AFE_SGEN_CON0,
				   AFE_SGEN_CON0_SINE_CH1_MASK |
				   AFE_SGEN_CON0_SINE_CH2_MASK,
				   AFE_SGEN_CON0_SINE_CH1_96K |
				   AFE_SGEN_CON0_SINE_CH2_96K);
		break;
	case AFE_SGEN_176K:
		regmap_update_bits(afe->regmap, AFE_SGEN_CON0,
				   AFE_SGEN_CON0_SINE_CH1_MASK |
				   AFE_SGEN_CON0_SINE_CH2_MASK,
				   AFE_SGEN_CON0_SINE_CH1_176K |
				   AFE_SGEN_CON0_SINE_CH2_176K);
		break;
	case AFE_SGEN_192K:
		regmap_update_bits(afe->regmap, AFE_SGEN_CON0,
				   AFE_SGEN_CON0_SINE_CH1_MASK |
				   AFE_SGEN_CON0_SINE_CH2_MASK,
				   AFE_SGEN_CON0_SINE_CH1_192K |
				   AFE_SGEN_CON0_SINE_CH2_192K);
		break;
	default:
		break;
	}

	mt3611_afe_disable_main_clk(afe);

	data->sinegen_fs = ucontrol->value.integer.value[0];

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     get clock source.
 * @param[in]
 *     kcontrol: mixer control
 * @param[in]
 *     ucontrol: control element information
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_clk_src_get(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt3611_afe_control_data *data = &afe->ctrl_data;
	unsigned int idx;

	if (!strcmp(kcontrol->id.name, "DL12_ClockSource_Select"))
		ucontrol->value.integer.value[0] = data->dl12_clk_src;
	else if (!strcmp(kcontrol->id.name, "VUL12_ClockSource_Select"))
		ucontrol->value.integer.value[0] = data->vul12_clk_src;
	else if (!strcmp(kcontrol->id.name, "IRQ0_ClockSource_Select"))
		ucontrol->value.integer.value[0] =
			data->common_irq_clk_src[MT3611_AFE_IRQ_0];
	else if (!strcmp(kcontrol->id.name, "IRQ1_ClockSource_Select"))
		ucontrol->value.integer.value[0] =
			data->common_irq_clk_src[MT3611_AFE_IRQ_1];
	else if (!strcmp(kcontrol->id.name, "IRQ2_ClockSource_Select"))
		ucontrol->value.integer.value[0] =
			data->common_irq_clk_src[MT3611_AFE_IRQ_2];
	else if (!strcmp(kcontrol->id.name, "IRQ3_ClockSource_Select"))
		ucontrol->value.integer.value[0] =
			data->common_irq_clk_src[MT3611_AFE_IRQ_3];
	else if (!strcmp(kcontrol->id.name, "I2S_ClockSource_Select")) {
		idx = MT3611_AFE_IO_I2S - MT3611_AFE_BACKEND_BASE;
		ucontrol->value.integer.value[0] = data->backend_clk_src[idx];
	} else if (!strcmp(kcontrol->id.name, "TDM_ClockSource_Select")) {
		idx = MT3611_AFE_IO_TDM - MT3611_AFE_BACKEND_BASE;
		ucontrol->value.integer.value[0] = data->backend_clk_src[idx];
	} else if (!strcmp(kcontrol->id.name, "MPHONE_ClockSource_Select")) {
		idx = MT3611_AFE_IO_MPHONE_MULTI - MT3611_AFE_BACKEND_BASE;
		ucontrol->value.integer.value[0] = data->backend_clk_src[idx];
	}

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     set clock source.
 * @param[in]
 *     kcontrol: mixer control
 * @param[in]
 *     ucontrol: control element information
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_clk_src_put(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt3611_afe_control_data *data = &afe->ctrl_data;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int idx;

	if (ucontrol->value.integer.value[0] >= e->items)
		return -EINVAL;

	if (!strcmp(kcontrol->id.name, "DL12_ClockSource_Select"))
		data->dl12_clk_src = ucontrol->value.integer.value[0];
	else if (!strcmp(kcontrol->id.name, "VUL12_ClockSource_Select"))
		data->vul12_clk_src = ucontrol->value.integer.value[0];
	else if (!strcmp(kcontrol->id.name, "IRQ0_ClockSource_Select"))
		data->common_irq_clk_src[MT3611_AFE_IRQ_0] =
			ucontrol->value.integer.value[0];
	else if (!strcmp(kcontrol->id.name, "IRQ1_ClockSource_Select"))
		data->common_irq_clk_src[MT3611_AFE_IRQ_1] =
			ucontrol->value.integer.value[0];
	else if (!strcmp(kcontrol->id.name, "IRQ2_ClockSource_Select"))
		data->common_irq_clk_src[MT3611_AFE_IRQ_2] =
			ucontrol->value.integer.value[0];
	else if (!strcmp(kcontrol->id.name, "IRQ3_ClockSource_Select"))
		data->common_irq_clk_src[MT3611_AFE_IRQ_3] =
			ucontrol->value.integer.value[0];
	else if (!strcmp(kcontrol->id.name, "I2S_ClockSource_Select")) {
		idx = MT3611_AFE_IO_I2S - MT3611_AFE_BACKEND_BASE;
		data->backend_clk_src[idx] = ucontrol->value.integer.value[0];
	}  else if (!strcmp(kcontrol->id.name, "TDM_ClockSource_Select")) {
		idx = MT3611_AFE_IO_TDM - MT3611_AFE_BACKEND_BASE;
		data->backend_clk_src[idx] = ucontrol->value.integer.value[0];
	} else if (!strcmp(kcontrol->id.name, "MPHONE_ClockSource_Select")) {
		idx = MT3611_AFE_IO_MPHONE_MULTI - MT3611_AFE_BACKEND_BASE;
		data->backend_clk_src[idx] = ucontrol->value.integer.value[0];
	}

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     get tdm out mode.
 * @param[in]
 *     kcontrol: mixer control
 * @param[in]
 *     ucontrol: control element information
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_tdm_out_mode_get(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt3611_afe_control_data *data = &afe->ctrl_data;

	ucontrol->value.integer.value[0] = data->tdm_out_mode;

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     set tdm out mode.
 * @param[in]
 *     kcontrol: mixer control
 * @param[in]
 *     ucontrol: control element information
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_tdm_out_mode_put(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt3611_afe_control_data *data = &afe->ctrl_data;

	data->tdm_out_mode = ucontrol->value.integer.value[0];

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     disable tdm out sine gen.
 * @param[in]
 *     kcontrol: mixer control
 * @param[in]
 *     ucontrol: control element information
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_tdm_out_sgen_get(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int val = 0;

	mt3611_afe_enable_main_clk(afe);

	regmap_read(afe->regmap, AFE_SINEGEN_CON_TDM, &val);

	mt3611_afe_disable_main_clk(afe);

	ucontrol->value.integer.value[0] = (val & AFE_SINEGEN_CON_TDM_OUT_EN);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     enable tdm out sine gen.
 * @param[in]
 *     kcontrol: mixer control
 * @param[in]
 *     ucontrol: control element information
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_tdm_out_sgen_put(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(plat);

	mt3611_afe_enable_main_clk(afe);

	if (ucontrol->value.integer.value[0])
		regmap_update_bits(afe->regmap, AFE_SINEGEN_CON_TDM,
				   GENMASK(31, 0),
				   AFE_SINEGEN_CON_EN_SET);
	else
		regmap_update_bits(afe->regmap, AFE_SINEGEN_CON_TDM,
				   GENMASK(31, 0),
				   AFE_SINEGEN_CON_EN_RESET);

	mt3611_afe_disable_main_clk(afe);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     disable tdm in sine gen.
 * @param[in]
 *     kcontrol: mixer control
 * @param[in]
 *     ucontrol: control element information
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_tdm_in_sgen_get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int val = 0;

	mt3611_afe_enable_main_clk(afe);

	regmap_read(afe->regmap, AFE_SINEGEN_CON_TDM_IN, &val);

	mt3611_afe_disable_main_clk(afe);

	ucontrol->value.integer.value[0] = (val & AFE_SINEGEN_CON_TDM_IN_EN);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     enable tdm in sine gen.
 * @param[in]
 *     kcontrol: mixer control
 * @param[in]
 *     ucontrol: control element information
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_tdm_in_sgen_put(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(plat);

	mt3611_afe_enable_main_clk(afe);

	if (ucontrol->value.integer.value[0])
		regmap_update_bits(afe->regmap, AFE_SINEGEN_CON_TDM_IN,
				   GENMASK(31, 0),
				   AFE_SINEGEN_CON_TDM_IN_EN_SET);
	else
		regmap_update_bits(afe->regmap, AFE_SINEGEN_CON_TDM_IN,
				   GENMASK(31, 0),
				   AFE_SINEGEN_CON_TDM_IN_EN_RESET);

	mt3611_afe_disable_main_clk(afe);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     disable i2s in loopback.
 * @param[in]
 *     kcontrol: mixer control
 * @param[in]
 *     ucontrol: control element information
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_i2s_in_loopback_get(struct snd_kcontrol *kcontrol,
					  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int val = 0;

	mt3611_afe_enable_main_clk(afe);

	regmap_read(afe->regmap, AFE_I2S_CON2, &val);

	mt3611_afe_disable_main_clk(afe);

	ucontrol->value.integer.value[0] = (val & AFE_I2S_CON2_LOOPBACK_EN);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     enable i2s in loopback.
 * @param[in]
 *     kcontrol: mixer control
 * @param[in]
 *     ucontrol: control element information
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_i2s_in_loopback_put(struct snd_kcontrol *kcontrol,
					  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(plat);

	mt3611_afe_enable_main_clk(afe);

	if (ucontrol->value.integer.value[0])
		regmap_update_bits(afe->regmap, AFE_I2S_CON2,
				   GENMASK(20, 20),
				   AFE_I2S_CON2_LOOPBACK_EN);
	else
		regmap_update_bits(afe->regmap, AFE_I2S_CON2,
				   GENMASK(20, 20), 0x0);

	mt3611_afe_disable_main_clk(afe);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     disable tdm in loopback.
 * @param[in]
 *     kcontrol: mixer control
 * @param[in]
 *     ucontrol: control element information
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_tdm_in_loopback_get(struct snd_kcontrol *kcontrol,
					  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int val = 0;

	mt3611_afe_enable_main_clk(afe);

	regmap_read(afe->regmap, AFE_TDM_IN_CON1, &val);

	mt3611_afe_disable_main_clk(afe);

	ucontrol->value.integer.value[0] = (val & AFE_TDM_IN_CON1_LOOPBACK_EN);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     enable tdm in loopback.
 * @param[in]
 *     kcontrol: mixer control
 * @param[in]
 *     ucontrol: control element information
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_tdm_in_loopback_put(struct snd_kcontrol *kcontrol,
					  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(plat);

	mt3611_afe_enable_main_clk(afe);

	if (ucontrol->value.integer.value[0])
		regmap_update_bits(afe->regmap, AFE_TDM_IN_CON1,
				   GENMASK(20, 20),
				   AFE_TDM_IN_CON1_LOOPBACK_EN);
	else
		regmap_update_bits(afe->regmap, AFE_TDM_IN_CON1,
				   GENMASK(20, 20), 0x0);

	mt3611_afe_disable_main_clk(afe);

	return 0;
}

static const struct soc_enum mt3611_afe_soc_enums[] = {
	[CTRL_SGEN_EN] =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(sgen_func), sgen_func),
	[CTRL_SGEN_FS] =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(sgen_fs_func), sgen_fs_func),
	[CTRL_CLK_SRC] =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(clk_src_func), clk_src_func),
	[CTRL_CLK_SRC_MPHONE] =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(clk_src_mphone_multi_func),
				    clk_src_mphone_multi_func),
	[CTRL_TDM_OUT_MODE] =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(tdm_out_modes), tdm_out_modes),
};

static const struct snd_kcontrol_new mt3611_afe_controls[] = {
	SOC_ENUM_EXT("Audio_SideGen_Switch",
		     mt3611_afe_soc_enums[CTRL_SGEN_EN],
		     mt3611_afe_sgen_get,
		     mt3611_afe_sgen_put),
	SOC_ENUM_EXT("Audio_SideGen_SampleRate",
		     mt3611_afe_soc_enums[CTRL_SGEN_FS],
		     mt3611_afe_sgen_fs_get,
		     mt3611_afe_sgen_fs_put),
	SOC_ENUM_EXT("DL12_ClockSource_Select",
		     mt3611_afe_soc_enums[CTRL_CLK_SRC],
		     mt3611_afe_clk_src_get,
		     mt3611_afe_clk_src_put),
	SOC_ENUM_EXT("VUL12_ClockSource_Select",
		     mt3611_afe_soc_enums[CTRL_CLK_SRC],
		     mt3611_afe_clk_src_get,
		     mt3611_afe_clk_src_put),
	SOC_ENUM_EXT("IRQ0_ClockSource_Select",
		     mt3611_afe_soc_enums[CTRL_CLK_SRC],
		     mt3611_afe_clk_src_get,
		     mt3611_afe_clk_src_put),
	SOC_ENUM_EXT("IRQ1_ClockSource_Select",
		     mt3611_afe_soc_enums[CTRL_CLK_SRC],
		     mt3611_afe_clk_src_get,
		     mt3611_afe_clk_src_put),
	SOC_ENUM_EXT("IRQ2_ClockSource_Select",
		     mt3611_afe_soc_enums[CTRL_CLK_SRC],
		     mt3611_afe_clk_src_get,
		     mt3611_afe_clk_src_put),
	SOC_ENUM_EXT("IRQ3_ClockSource_Select",
		     mt3611_afe_soc_enums[CTRL_CLK_SRC],
		     mt3611_afe_clk_src_get,
		     mt3611_afe_clk_src_put),
	SOC_ENUM_EXT("I2S_ClockSource_Select",
		     mt3611_afe_soc_enums[CTRL_CLK_SRC],
		     mt3611_afe_clk_src_get,
		     mt3611_afe_clk_src_put),
	SOC_ENUM_EXT("TDM_ClockSource_Select",
		     mt3611_afe_soc_enums[CTRL_CLK_SRC],
		     mt3611_afe_clk_src_get,
		     mt3611_afe_clk_src_put),
	SOC_ENUM_EXT("MPHONE_ClockSource_Select",
		     mt3611_afe_soc_enums[CTRL_CLK_SRC_MPHONE],
		     mt3611_afe_clk_src_get,
		     mt3611_afe_clk_src_put),
	SOC_ENUM_EXT("TDM_Out_Mode_Select",
		     mt3611_afe_soc_enums[CTRL_TDM_OUT_MODE],
		     mt3611_afe_tdm_out_mode_get,
		     mt3611_afe_tdm_out_mode_put),
	SOC_SINGLE_BOOL_EXT("TDM_Out_Sgen_Switch",
			    0,
			    mt3611_afe_tdm_out_sgen_get,
			    mt3611_afe_tdm_out_sgen_put),
	SOC_SINGLE_BOOL_EXT("TDM_In_Sgen_Switch",
			    0,
			    mt3611_afe_tdm_in_sgen_get,
			    mt3611_afe_tdm_in_sgen_put),
	SOC_SINGLE_BOOL_EXT("I2S_In_Loopback_Switch",
			    0,
			    mt3611_afe_i2s_in_loopback_get,
			    mt3611_afe_i2s_in_loopback_put),
	SOC_SINGLE_BOOL_EXT("TDM_In_Loopback_Switch",
			    0,
			    mt3611_afe_tdm_in_loopback_get,
			    mt3611_afe_tdm_in_loopback_put),
};

/** @ingroup type_group_afe_InFn
 * @par Description
 *     add mixer controls into platform.
 * @param[in]
 *     platform: platform to add controls to
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mt3611_afe_add_controls(struct snd_soc_platform *platform)
{
	return snd_soc_add_platform_controls(platform, mt3611_afe_controls,
					     ARRAY_SIZE(mt3611_afe_controls));
}

