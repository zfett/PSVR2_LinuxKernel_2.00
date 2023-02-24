/**
 * @file mt-afe-pcm.c
 * Mediatek ALSA SoC AFE platform driver
 *
 * Copyright (c) 2017 MediaTek Inc.
 *
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

/**
 * @defgroup IP_group_afe AFE
 *     Audio Front End.\n
 *     @{
 *           @defgroup type_group_afe_def 1.Define
 *               This is AFE Define.
 *           @defgroup type_group_afe_enum 2.Enumeration
 *               This is AFE Enumeration.
 *           @defgroup type_group_afe_struct 3.Structure
 *               This is AFE Structure.
 *           @defgroup type_group_afe_typedef 4.Typedef
 *               none
 *           @defgroup type_group_afe_API 5.API Reference
 *               none
 *           @defgroup type_group_afe_InFn 6.Internal Function
 *               This is AFE Internal Function.
 *     @}
 */

#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/poll.h>
#include <linux/regmap.h>
#include <linux/sched.h>

#include "mt-afe-common.h"
#include "mt-afe-regs.h"
#include "mt-afe-util.h"
#include "mt-afe-debug.h"
#include "mt-afe-pcm.h"

#define MT_DEDAULT_I2S_MCLK_MULTIPLIER 256
#define MT_DEDAULT_TDM_MCLK_MULTIPLIER 128 /* Changed 256->128 to adapt to DP clk src */
#define DPCLOCK_135M 135475200
#define DPCLOCK_147M 147456000

#define USE_SRAM
#define TRANSFER_DELAY_TIME 1000 /*delay us */

#define SND_SOC_DAIFMT_I2S          1 /* I2S mode */
#define SND_SOC_DAIFMT_RIGHT_J      2 /* Right Justified mode */
#define SND_SOC_DAIFMT_LEFT_J       3 /* Left Justified mode */

#define SND_SOC_DAIFMT_NB_IF        (2 << 8) /* normal BCLK + inv FRM */
#define SND_SOC_DAIFMT_IB_NF        (3 << 8) /* invert BCLK + nor FRM */
#define SND_SOC_DAIFMT_IB_IF        (4 << 8) /* invert BCLK + FRM */

#define SND_SOC_DAIFMT_CBS_CFS      (4 << 12) /* codec clk & FRM slave */

#define SND_SOC_DAIFMT_FORMAT_MASK  0x000f
#define SND_SOC_DAIFMT_INV_MASK     0x0f00

#define SNDRV_PCM_TRIGGER_STOP          0
#define SNDRV_PCM_TRIGGER_START         1
#define SNDRV_PCM_TRIGGER_STOP_NOIRQ    2
#define SNDRV_PCM_TRIGGER_START_NOIRQ   3

struct afe_private_data {
	bool prepared;
	enum mt_afe_memif_io callback_capture_io;
	bool enable_capture_callback;
	void (*usb_callback)(void);
	bool transfer_start;
	enum afe_input_port_type transfer_port;
	int trigger_start_cnt[MT_AFE_MEMIF_NUM];
#ifdef CONFIG_SIE_DEVELOP_BUILD
	unsigned int captured_pos[MT_AFE_MEMIF_NUM];
	wait_queue_head_t capture_waitq[MT_AFE_MEMIF_NUM];
	bool playback_start;
	unsigned int playback_pos;
	unsigned int playback_cnt;
	wait_queue_head_t playback_waitq;
	bool tonegen_start;
#endif
};

static struct afe_private_data afe_data = {
	.prepared = false,
	.callback_capture_io = MT_AFE_MEMIF_NUM,
	.enable_capture_callback = false,
	.usb_callback = NULL,
	.transfer_start = false,
	.transfer_port = AFE_INPUT_PORT_NONE,
};

static inline enum afe_input_port_type get_transfer_port(void)
{
	return afe_data.transfer_port;
}

void  *mt_afe_get_capture_dma_address(struct device *dev, const int dai_id)
{
	struct mtk_afe *afe = dev_get_drvdata(dev);
	static struct mt_afe_memif *memif;

	memif = &afe->memif[dai_id];
	return memif->virt_buf_addr;
}

unsigned int mt_afe_get_capture_buffer_size(struct device *dev, const int dai_id)
{
	struct mtk_afe *afe = dev_get_drvdata(dev);
	static struct mt_afe_memif *memif;

	memif = &afe->memif[dai_id];
	return memif->buffer_size;
}

int mt_afe_regist_usb_callback(struct device *dev, void (*callback)(void))
{
	afe_data.usb_callback = callback;
	return 0;
}

void mt_afe_enable_usb_callback(int dai_id)
{
	afe_data.callback_capture_io = dai_id;
	afe_data.enable_capture_callback = true;
}

void mt_afe_disable_usb_callback(void)
{
	afe_data.enable_capture_callback = false;
}

struct mt_afe_dai_runtime {
	int dai_id;
	unsigned int rate;
	int channels;
	snd_pcm_format_t format;
	snd_pcm_uframes_t period_size;
	unsigned int no_period_wakeup: 1;
};

static struct mt_afe_dai_runtime dai_runtime[MT_AFE_MEMIF_NUM] = {
	[MT_AFE_MEMIF_DL12] = {
		.dai_id = MT_AFE_MEMIF_DL12,
		.rate = 48000,
		.channels = 2,
		.format = SNDRV_PCM_FORMAT_S32_LE,
		.period_size = 128, /* min =2 / max=256 */
#ifdef CONFIG_SIE_DEVELOP_BUILD
		.no_period_wakeup = 0
#else
		.no_period_wakeup = 1
#endif
	},
	[MT_AFE_MEMIF_VUL12] = {
		.dai_id = MT_AFE_MEMIF_VUL12,
		.rate = 48000,
		.channels = 1,
		.format = SNDRV_PCM_FORMAT_S16_LE,
		.period_size = 48, /* 48 = 1ms@48KHz */ /* min =2 / max=256 */
		.no_period_wakeup = 0
	},
	[MT_AFE_MEMIF_AWB] = {
		.dai_id = MT_AFE_MEMIF_AWB,
		.rate = 48000,
		.channels = 2,
		.format = SNDRV_PCM_FORMAT_S32_LE,
		.period_size = 128, /* min =2 / max=256 */
#ifdef CONFIG_SIE_DEVELOP_BUILD
		.no_period_wakeup = 0
#else
		.no_period_wakeup = 1
#endif
	}
};

struct mt_afe_rate {
	unsigned int rate;
	unsigned int regvalue;
};

static const struct mt_afe_rate mt_afe_i2s_rates[] = {
	{ .rate = 7350, .regvalue = 0 },
	{ .rate = 8000, .regvalue = 0 },
	{ .rate = 11025, .regvalue = 1 },
	{ .rate = 12000, .regvalue = 1 },
	{ .rate = 14700, .regvalue = 2 },
	{ .rate = 16000, .regvalue = 2 },
	{ .rate = 22050, .regvalue = 3 },
	{ .rate = 24000, .regvalue = 3 },
	{ .rate = 29400, .regvalue = 4 },
	{ .rate = 32000, .regvalue = 4 },
	{ .rate = 44100, .regvalue = 5 },
	{ .rate = 48000, .regvalue = 5 },
	{ .rate = 88200, .regvalue = 6 },
	{ .rate = 96000, .regvalue = 6 },
	{ .rate = 176400, .regvalue = 7 },
	{ .rate = 192000, .regvalue = 7 },
};

static const struct mt_afe_rate mt_afe_data_rates[] = {
	{ .rate = 7350, .regvalue = 0 },
	{ .rate = 8000, .regvalue = 0 },
	{ .rate = 11025, .regvalue = 1 },
	{ .rate = 12000, .regvalue = 1 },
	{ .rate = 14700, .regvalue = 2 },
	{ .rate = 16000, .regvalue = 2 },
	{ .rate = 22050, .regvalue = 3 },
	{ .rate = 24000, .regvalue = 3 },
	{ .rate = 29400, .regvalue = 4 },
	{ .rate = 32000, .regvalue = 4 },
	{ .rate = 44100, .regvalue = 5 },
	{ .rate = 48000, .regvalue = 5 },
	{ .rate = 88200, .regvalue = 6 },
	{ .rate = 96000, .regvalue = 6 },
	{ .rate = 176400, .regvalue = 7 },
	{ .rate = 192000, .regvalue = 7 },
	{ .rate = 352000, .regvalue = 8 },
	{ .rate = 384000, .regvalue = 8 },
};

/** @ingroup type_group_afe_InFn
 * @par Description
 *     get i2s rate index.
 * @param[in]
 *     sample_rate: rate
 * @return
 *     rate index.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt_afe_i2s_fs(unsigned int sample_rate)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mt_afe_i2s_rates); i++)
		if (mt_afe_i2s_rates[i].rate == sample_rate)
			return mt_afe_i2s_rates[i].regvalue;

	return -EINVAL;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     get data rate index.
 * @param[in]
 *     sample_rate: rate
 * @return
 *     rate index.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt_afe_data_fs(unsigned int sample_rate)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mt_afe_data_rates); i++)
		if (mt_afe_data_rates[i].rate == sample_rate)
			return mt_afe_data_rates[i].regvalue;

	return -EINVAL;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     get memory interface clock source mask.
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     id: memory interface id
 * @param[in]
 *     rate: clock rate
 * @return
 *     mask.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt_afe_get_memif_clk_src_mask(struct mtk_afe *afe,
					 int id,
					 unsigned int rate)
{
	struct mt_afe_control_data *data = &afe->ctrl_data;
	int mask = 0;

	if (id == MT_AFE_MEMIF_DL12) {
		if (data->dl12_clk_src == FROM_AUDIO_PLL) {
			mask = (rate % 8000) ?
				AFE_DAC_CON1_DL12_A2SYS_MASK :
				AFE_DAC_CON1_DL12_A1SYS_MASK;
		} else if (data->dl12_clk_src == FROM_DPRX_PLL) {
			mask = AFE_DAC_CON1_DL12_A3SYS_MASK;
		}
	} else if (id == MT_AFE_MEMIF_VUL12) {
		if (data->vul12_clk_src == FROM_AUDIO_PLL) {
			mask = (rate % 8000) ?
				AFE_DAC_CON0_VUL12_A2SYS_MASK :
				AFE_DAC_CON0_VUL12_A1SYS_MASK;
		} else if (data->vul12_clk_src == FROM_DPRX_PLL) {
			mask = AFE_DAC_CON0_VUL12_A3SYS_MASK;
		}
	}

	return mask;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     get irq clock source mask.
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     irq: irq id
 * @param[in]
 *     rate: clock rate
 * @return
 *     mask.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt_afe_get_irq_clk_src_mask(struct mtk_afe *afe,
				       int irq,
				       unsigned int rate)
{
	struct mt_afe_control_data *data = &afe->ctrl_data;
	int mask = 0;

	if ((irq >= MT_AFE_IRQ_0) && (irq < MT_AFE_COMMON_IRQ_NUM)) {
		unsigned int clk_src = data->common_irq_clk_src[irq];

		if (clk_src == FROM_AUDIO_PLL) {
			mask = (rate % 8000) ?
				AFE_COMMON_IRQ_A2SYS_MASK :
				AFE_COMMON_IRQ_A1SYS_MASK;
		} else if (clk_src == FROM_DPRX_PLL) {
			mask = AFE_COMMON_IRQ_A3SYS_MASK;
		}
	}

	return mask;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     set i2s out configuration.
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     rate: word select rate
 * @param[in]
 *     bit_width: bit width
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt_afe_set_i2s_out(struct mtk_afe *afe,
			      unsigned int rate,
			      int bit_width)
{
	struct mt_afe_control_data *data = &afe->ctrl_data;
	int be_idx = MT_AFE_IO_I2S - MT_AFE_BACKEND_BASE;
	struct mt_afe_be_dai_data *be = &afe->be_data[be_idx];
	int fs = mt_afe_i2s_fs(rate);
	unsigned int i2s_con = 0;
	unsigned int shift_con = 0;

	dev_dbg(afe->dev, "%s rate=%u bit_width=%d\n", __func__, rate, bit_width);

	if (fs < 0) {
		dev_err(afe->dev, "%s fs error fs:%d rate:%u\n", __func__, fs, rate);
		return -EINVAL;
	}

	i2s_con = AFE_I2S_CON1_RATE(fs);

	switch (be->fmt_mode & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		i2s_con |= AFE_I2S_CON1_FORMAT_I2S;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		i2s_con |= AFE_I2S_CON1_LRCK_INV;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		i2s_con |= AFE_I2S_CON1_LRCK_INV;
		shift_con |= AFE_I2S_SHIFT_CON0_OUT_RJ_EN;
		shift_con |= AFE_I2S_SHIFT_CON0_OUT_SHIFT_NUM(32 - bit_width);
		break;
	default:
		break;
	}

	switch (be->fmt_mode & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_IB_IF:
		i2s_con |= AFE_I2S_CON1_BCK_INV;
		i2s_con ^= AFE_I2S_CON1_LRCK_INV;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		i2s_con ^= AFE_I2S_CON1_LRCK_INV;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		i2s_con |= AFE_I2S_CON1_BCK_INV;
		break;
	default:
		break;
	}

	if (data->backend_clk_src[be_idx] == FROM_AUDIO_PLL) {
		if (rate % 8000)
			i2s_con |= AFE_I2S_CON1_CLK_A2SYS;
		else
			i2s_con |= AFE_I2S_CON1_CLK_A1SYS;
	} else if (data->backend_clk_src[be_idx] == FROM_DPRX_PLL) {
		i2s_con |= AFE_I2S_CON1_CLK_A3SYS;
	}
	if (bit_width > 16)
		i2s_con |= AFE_I2S_CON1_WLEN_32BIT;

	regmap_update_bits(afe->regmap, AFE_I2S_CON1,
			   ~(u32)AFE_I2S_CON1_EN, i2s_con);

	regmap_update_bits(afe->regmap, AFE_I2S_SHIFT_CON0,
			   AFE_I2S_SHIFT_CON0_OUT_MASK, shift_con);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     set i2s in configuration.
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     rate: word select rate
 * @param[in]
 *     bit_width: bit width
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt_afe_set_i2s_in(struct mtk_afe *afe,
			     unsigned int rate,
			     int bit_width)
{
	struct mt_afe_control_data *data = &afe->ctrl_data;
	int be_idx = MT_AFE_IO_I2S - MT_AFE_BACKEND_BASE;
	struct mt_afe_be_dai_data *be = &afe->be_data[be_idx];
	int fs = mt_afe_i2s_fs(rate);
	unsigned int i2s_con = 0;
	unsigned int shift_con = 0;

	dev_dbg(afe->dev, "%s rate=%u bit_width=%d\n", __func__, rate, bit_width);

	if (fs < 0) {
		dev_err(afe->dev, "%s fs error fs:%d rate:%u\n", __func__, fs, rate);
		return -EINVAL;
	}

	i2s_con = AFE_I2S_CON2_RATE(fs);

	switch (be->fmt_mode & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		i2s_con |= AFE_I2S_CON2_FORMAT_I2S;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		i2s_con |= AFE_I2S_CON2_LRCK_INV;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		i2s_con |= AFE_I2S_CON2_LRCK_INV;
		shift_con |= AFE_I2S_SHIFT_CON0_IN_RJ_EN;
		shift_con |= AFE_I2S_SHIFT_CON0_IN_SHIFT_NUM(32 - bit_width);
		break;
	default:
		break;
	}

	switch (be->fmt_mode & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_IB_IF:
		i2s_con |= AFE_I2S_CON2_BCK_INV;
		i2s_con ^= AFE_I2S_CON2_LRCK_INV;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		i2s_con ^= AFE_I2S_CON2_LRCK_INV;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		i2s_con |= AFE_I2S_CON2_BCK_INV;
		break;
	default:
		break;
	}

	if (data->backend_clk_src[be_idx] == FROM_AUDIO_PLL) {
		if (rate % 8000)
			i2s_con |= AFE_I2S_CON2_CLK_A2SYS;
		else
			i2s_con |= AFE_I2S_CON2_CLK_A1SYS;
	} else if (data->backend_clk_src[be_idx] == FROM_DPRX_PLL) {
		i2s_con |= AFE_I2S_CON2_CLK_A3SYS;
	}

	if (bit_width > 16)
		i2s_con |= AFE_I2S_CON2_WLEN_32BIT;

	regmap_update_bits(afe->regmap, AFE_I2S_CON2, 0xfffe, i2s_con);

	regmap_update_bits(afe->regmap, AFE_I2S_SHIFT_CON0,
			   AFE_I2S_SHIFT_CON0_IN_MASK, shift_con);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     enable i2s.
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     enable: true or false
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mt_afe_set_i2s_enable(struct mtk_afe *afe, bool enable)
{
	unsigned int val;

	dev_dbg(afe->dev, "%s enable=%s\n", __func__, enable ? "true" : "false");

	regmap_read(afe->regmap, AFE_I2S_CON1, &val);
	if (!!(val & AFE_I2S_CON1_EN) != enable) {
		/* output */
		regmap_update_bits(afe->regmap, AFE_I2S_CON1, 0x1, enable);
	}

	if (!!(val & AFE_I2S_CON2_EN) != enable) {
		/* input */
		regmap_update_bits(afe->regmap, AFE_I2S_CON2, 0x1, enable);
	}
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     enable mphone multi.
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     enable: true or false
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mt_afe_set_mphone_multi_enable(struct mtk_afe *afe, bool enable)
{
	unsigned int val;

	dev_dbg(afe->dev, "%s enable=%s\n", __func__, enable ? "true" : "false");

	regmap_read(afe->regmap, AFE_MPHONE_MULTI_CON0, &val);
	if (!!(val & AFE_MPHONE_MULTI_CON0_EN) == enable)
		return;

	regmap_update_bits(afe->regmap, AFE_MPHONE_MULTI_CON0, 0x1, enable);
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     enable irq.
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     memif: memif data
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt_afe_enable_irq(struct mtk_afe *afe,
			     struct mt_afe_memif *memif)
{
	int irq_mode = memif->data->irq_mode;
	unsigned long flags;

	dev_dbg(afe->dev, "%s memif=%s\n", __func__, memif->data->name);

	spin_lock_irqsave(&afe->afe_ctrl_lock, flags);

	afe->irq_mode_ref_cnt[irq_mode]++;
	if (afe->irq_mode_ref_cnt[irq_mode] > 1) {
		spin_unlock_irqrestore(&afe->afe_ctrl_lock, flags);
		return 0;
	}

	regmap_update_bits(afe->regmap, AFE_IRQ_MCU_CON, 1 << irq_mode,
			   1 << irq_mode);

	spin_unlock_irqrestore(&afe->afe_ctrl_lock, flags);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     disable irq.
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     memif: memif data
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt_afe_disable_irq(struct mtk_afe *afe,
			      struct mt_afe_memif *memif)
{
	int irq_mode = memif->data->irq_mode;
	unsigned long flags;

	dev_dbg(afe->dev, "%s memif=%s\n", __func__, memif->data->name);

	spin_lock_irqsave(&afe->afe_ctrl_lock, flags);

	afe->irq_mode_ref_cnt[irq_mode]--;
	if (afe->irq_mode_ref_cnt[irq_mode] > 0) {
		spin_unlock_irqrestore(&afe->afe_ctrl_lock, flags);
		return 0;
	} else if (afe->irq_mode_ref_cnt[irq_mode] < 0) {
		afe->irq_mode_ref_cnt[irq_mode] = 0;
		spin_unlock_irqrestore(&afe->afe_ctrl_lock, flags);
		return 0;
	}

	regmap_update_bits(afe->regmap, AFE_IRQ_MCU_CON, 1 << irq_mode,
			   0 << irq_mode);

	regmap_write(afe->regmap, AFE_IRQ_MCU_CLR, 1 << irq_mode);

	spin_unlock_irqrestore(&afe->afe_ctrl_lock, flags);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     set delay counter for irq.
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     memif: memif data
 * @param[in]
 *     counter: counter in samples.
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt_afe_set_irq_delay_counter(struct mtk_afe *afe,
					struct mt_afe_memif *memif,
					int counter)
{
	int irq_mode = memif->data->irq_mode;

	dev_dbg(afe->dev, "%s memif=%s\n counter=%d", __func__, memif->data->name, counter);

	switch (irq_mode) {
	case MT_AFE_IRQ_0:
		regmap_update_bits(afe->regmap, AFE_IRQ_DELAY_CNT0,
				   0xffff, (counter & 0xffff));
		break;
	case MT_AFE_IRQ_1:
		regmap_update_bits(afe->regmap, AFE_IRQ_DELAY_CNT0,
				   0xffff << 16, (counter & 0xffff) << 16);
		break;
	case MT_AFE_IRQ_2:
		regmap_update_bits(afe->regmap, AFE_IRQ_DELAY_CNT1,
				   0xffff, (counter & 0xffff));
		break;
	case MT_AFE_IRQ_3:
		regmap_update_bits(afe->regmap, AFE_IRQ_DELAY_CNT1,
				   0xffff << 16, (counter & 0xffff) << 16);
		break;
	case MT_AFE_IRQ_4:
		regmap_update_bits(afe->regmap, AFE_IRQ_DELAY_CNT2,
				   0xffff, (counter & 0xffff));
		break;
	case MT_AFE_IRQ_5:
		regmap_update_bits(afe->regmap, AFE_IRQ_DELAY_CNT2,
				   0xffff << 16, (counter & 0xffff) << 16);
		break;
	case MT_AFE_IRQ_6:
		regmap_update_bits(afe->regmap, AFE_IRQ_DELAY_CNT3,
				   0xffff, (counter & 0xffff));
		break;
	default:
		break;
	}

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     i2s startup function for struct snd_soc_dai_ops.
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
static int mt_afe_i2s_startup(struct mtk_afe *afe)
{

	dev_dbg(afe->dev, "%s\n", __func__);

	mt_afe_enable_main_clk(afe);

	mt_afe_enable_clks(afe, afe->clocks[MT_CLK_APLL12_DIV1], NULL);
	mt_afe_enable_clks(afe, afe->clocks[MT_CLK_APLL12_DIV0], NULL);

	mt_afe_enable_a1sys_associated_clk(afe);
	mt_afe_enable_a2sys_associated_clk(afe);
	mt_afe_enable_a3sys_associated_clk(afe);

	mt_afe_enable_top_cg(afe, MT_AFE_CG_I2S_OUT);
	mt_afe_enable_top_cg(afe, MT_AFE_CG_I2S_IN);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     i2s shutdown function for struct snd_soc_dai_ops.
 * @param[in]
 *     afe: AFE data
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mt_afe_i2s_shutdown(struct mtk_afe *afe)
{
	const int be_idx = MT_AFE_IO_I2S - MT_AFE_BACKEND_BASE;
	struct mt_afe_be_dai_data *be = &afe->be_data[be_idx];

	dev_dbg(afe->dev, "%s\n", __func__);

	if (be->prepared) {
		mt_afe_set_i2s_enable(afe, false);
		be->prepared = false;
	}

	mt_afe_disable_top_cg(afe, MT_AFE_CG_I2S_IN);
	mt_afe_disable_top_cg(afe, MT_AFE_CG_I2S_OUT);

	mt_afe_disable_a1sys_associated_clk(afe);
	mt_afe_disable_a2sys_associated_clk(afe);
	mt_afe_disable_a3sys_associated_clk(afe);

	mt_afe_disable_clks(afe, afe->clocks[MT_CLK_APLL12_DIV1],
			    NULL);
	mt_afe_disable_clks(afe, afe->clocks[MT_CLK_APLL12_DIV0],
			    NULL);

	mt_afe_disable_main_clk(afe);
}

/* we do lots of calculations on snd_pcm_format_t; shut up sparse */
#define INT	__force int

struct pcm_format_data {
	unsigned char width;	/* bit width */
	unsigned char phys;	/* physical bit width */
	signed char le;	/* 0 = big-endian, 1 = little-endian, -1 = others */
	signed char signd;	/* 0 = unsigned, 1 = signed, -1 = others */
	unsigned char silence[8];	/* silence data to fill */
};

static struct pcm_format_data pcm_formats[(INT)SNDRV_PCM_FORMAT_LAST + 1] = {
	[SNDRV_PCM_FORMAT_S8] = {
		.width = 8, .phys = 8, .le = -1, .signd = 1,
		.silence = {},
	},
	[SNDRV_PCM_FORMAT_U8] = {
		.width = 8, .phys = 8, .le = -1, .signd = 0,
		.silence = { 0x80 },
	},
	[SNDRV_PCM_FORMAT_S16_LE] = {
		.width = 16, .phys = 16, .le = 1, .signd = 1,
		.silence = {},
	},
	[SNDRV_PCM_FORMAT_S16_BE] = {
		.width = 16, .phys = 16, .le = 0, .signd = 1,
		.silence = {},
	},
	[SNDRV_PCM_FORMAT_U16_LE] = {
		.width = 16, .phys = 16, .le = 1, .signd = 0,
		.silence = { 0x00, 0x80 },
	},
	[SNDRV_PCM_FORMAT_U16_BE] = {
		.width = 16, .phys = 16, .le = 0, .signd = 0,
		.silence = { 0x80, 0x00 },
	},
	[SNDRV_PCM_FORMAT_S24_LE] = {
		.width = 24, .phys = 32, .le = 1, .signd = 1,
		.silence = {},
	},
	[SNDRV_PCM_FORMAT_S24_BE] = {
		.width = 24, .phys = 32, .le = 0, .signd = 1,
		.silence = {},
	},
	[SNDRV_PCM_FORMAT_U24_LE] = {
		.width = 24, .phys = 32, .le = 1, .signd = 0,
		.silence = { 0x00, 0x00, 0x80 },
	},
	[SNDRV_PCM_FORMAT_U24_BE] = {
		.width = 24, .phys = 32, .le = 0, .signd = 0,
		.silence = { 0x00, 0x80, 0x00, 0x00 },
	},
	[SNDRV_PCM_FORMAT_S32_LE] = {
		.width = 32, .phys = 32, .le = 1, .signd = 1,
		.silence = {},
	},
	[SNDRV_PCM_FORMAT_S32_BE] = {
		.width = 32, .phys = 32, .le = 0, .signd = 1,
		.silence = {},
	},
	[SNDRV_PCM_FORMAT_U32_LE] = {
		.width = 32, .phys = 32, .le = 1, .signd = 0,
		.silence = { 0x00, 0x00, 0x00, 0x80 },
	},
	[SNDRV_PCM_FORMAT_U32_BE] = {
		.width = 32, .phys = 32, .le = 0, .signd = 0,
		.silence = { 0x80, 0x00, 0x00, 0x00 },
	},
};

static int snd_pcm_format_width(snd_pcm_format_t format)
{
	int val;

	if ((INT)format < 0 || (INT)format > (INT)SNDRV_PCM_FORMAT_LAST)
		return -EINVAL;
	val = pcm_formats[(INT)format].width;
	if (val == 0)
		return -EINVAL;
	return val;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     i2s prepare function for struct snd_soc_dai_ops.
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
static int mt_afe_i2s_prepare(struct mtk_afe *afe)
{
	int dai_id = MT_AFE_MEMIF_DL12;
	struct mt_afe_dai_runtime *runtime = &dai_runtime[dai_id];
	const int be_idx = MT_AFE_IO_I2S - MT_AFE_BACKEND_BASE;
	struct mt_afe_be_dai_data *be = &afe->be_data[be_idx];
	struct mt_afe_control_data *data = &afe->ctrl_data;
	const unsigned int rate = runtime->rate;
	const int bit_width = snd_pcm_format_width(runtime->format);
	unsigned int mclk = 0;
	int ret;

	dev_dbg(afe->dev, "%s\n", __func__);

	if (be->prepared) {
		if ((be->cached_rate != rate) ||
		    (be->cached_format != runtime->format) ||
		    (be->cached_clk_src != data->backend_clk_src[be_idx] /* afe_config.clk_src*/)) {
																 /* Reset when clk_src changed */
			mt_afe_set_i2s_enable(afe, false);
			be->prepared = false;
		} else {
			dev_dbg(afe->dev, "%s '%s' is already prepared\n",
				__func__, afe->memif[dai_id].data->name);
			return 0;
		}
	}
	ret = mt_afe_set_i2s_out(afe, rate, bit_width);
	if (ret)
		return ret;

	ret = mt_afe_set_i2s_in(afe, rate, bit_width);
	if (ret)
		return ret;

	if (data->backend_clk_src[be_idx] == FROM_AUDIO_PLL) {
		if (rate % 8000) {
			mt_afe_set_parent_clk(afe,
					      afe->clocks[MT_CLK_TDMOUT_SEL],
					      afe->clocks[MT_CLK_APLL2_CK]);

			mt_afe_set_parent_clk(afe,
					      afe->clocks[MT_CLK_TDMIN_SEL],
					      afe->clocks[MT_CLK_APLL2_CK]);
		} else {
			mt_afe_set_parent_clk(afe,
					      afe->clocks[MT_CLK_TDMOUT_SEL],
					      afe->clocks[MT_CLK_APLL1_CK]);

			mt_afe_set_parent_clk(afe,
					      afe->clocks[MT_CLK_TDMIN_SEL],
					      afe->clocks[MT_CLK_APLL1_CK]);
		}
	} else if (data->backend_clk_src[be_idx] == FROM_DPRX_PLL) {
		if (rate % 8000)
			mt_afe_set_clks(afe,
					afe->clocks[MT_CLK_DPAPLL_CK],
					DPCLOCK_135M, NULL, 0);
		else
			mt_afe_set_clks(afe,
					afe->clocks[MT_CLK_DPAPLL_CK],
					DPCLOCK_147M, NULL, 0);
		mt_afe_set_parent_clk(afe,
				      afe->clocks[MT_CLK_TDMOUT_SEL],
				      afe->clocks[MT_CLK_DPAPLL_CK]);

		mt_afe_set_parent_clk(afe,
				      afe->clocks[MT_CLK_TDMIN_SEL],
				      afe->clocks[MT_CLK_DPAPLL_CK]);

		mt_afe_set_parent_clk(afe,
				      afe->clocks[MT_CLK_A3SYS_HP_SEL],
				      afe->clocks[MT_CLK_DPAPLL_D3]);
	}

	mclk = (be->mclk_freq > 0) ? be->mclk_freq :
	       rate * MT_DEDAULT_I2S_MCLK_MULTIPLIER;
	dev_info(afe->dev, "be->mclk_freq: %d\n", be->mclk_freq);
	dev_info(afe->dev, "mclk :%d\n",  mclk);

	mt_afe_set_clks(afe, afe->clocks[MT_CLK_APLL12_DIV1],
			mclk, NULL, 0);
	mt_afe_set_clks(afe, afe->clocks[MT_CLK_APLL12_DIV0],
			mclk, NULL, 0);

	be->prepared = true;
	be->cached_rate = rate;
	be->cached_channels = runtime->channels;
	be->cached_format = runtime->format;
	be->cached_clk_src = data->backend_clk_src[be_idx];

	mt_afe_set_i2s_enable(afe, true);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     i2s trigger function for struct snd_soc_dai_ops.
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     dai_id: the dai ID
 * @param[in]
 *    enable: true = enable dai/ false = disable dai
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt_afe_i2s_trigger(struct mtk_afe *afe, int dai_id, bool enable)
{
	struct mt_afe_dai_runtime *runtime = &dai_runtime[dai_id];

	dev_dbg(afe->dev, "%s dai_id=%d(%s) enable:%s\n",
		__func__, dai_id, afe->memif[dai_id].data->name, enable ? "true" : "false");

	if (dai_id != MT_AFE_MEMIF_DL12 && dai_id != MT_AFE_MEMIF_VUL12) {
		return -EINVAL;
	}

	if (enable) {
		if (dai_id == MT_AFE_MEMIF_DL12) {
			regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
					   AFE_CONN_MUX_O0_MASK |
					   AFE_CONN_MUX_O1_MASK |
					   AFE_CONN_MUX_O2_MASK |
					   AFE_CONN_MUX_O3_MASK,
					   AFE_CONN_MUX_O0_I0 |
					   AFE_CONN_MUX_O1_I1 |
					   AFE_CONN_MUX_O2_I2 |
					   AFE_CONN_MUX_O3_I3);
		} else {
			if (runtime->channels > 2)
				regmap_update_bits(afe->regmap,
						   AFE_CONN_MUX_CFG,
						   AFE_CONN_MUX_O4_MASK |
						   AFE_CONN_MUX_O5_MASK |
						   AFE_CONN_MUX_O6_MASK |
						   AFE_CONN_MUX_O7_MASK,
						   AFE_CONN_MUX_O4_I4 |
						   AFE_CONN_MUX_O5_I5 |
						   AFE_CONN_MUX_O6_I6 |
						   AFE_CONN_MUX_O7_I7);
			else
				regmap_update_bits(afe->regmap,
						   AFE_CONN_MUX_CFG,
						   AFE_CONN_MUX_O4_MASK |
						   AFE_CONN_MUX_O5_MASK |
						   AFE_CONN_MUX_O6_MASK |
						   AFE_CONN_MUX_O7_MASK,
						   AFE_CONN_MUX_O4_NULL |
						   AFE_CONN_MUX_O5_NULL |
						   AFE_CONN_MUX_O6_I4 |
						   AFE_CONN_MUX_O7_I5);
		}
		return 0;
	}
	if (dai_id == MT_AFE_MEMIF_DL12) {
		regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
				   AFE_CONN_MUX_O0_MASK |
				   AFE_CONN_MUX_O1_MASK |
				   AFE_CONN_MUX_O2_MASK |
				   AFE_CONN_MUX_O3_MASK,
				   AFE_CONN_MUX_O0_NULL |
				   AFE_CONN_MUX_O1_NULL |
				   AFE_CONN_MUX_O2_NULL |
				   AFE_CONN_MUX_O3_NULL);
	} else {
		regmap_update_bits(afe->regmap, AFE_CONN_MUX_CFG,
				   AFE_CONN_MUX_O4_MASK |
				   AFE_CONN_MUX_O5_MASK |
				   AFE_CONN_MUX_O6_MASK |
				   AFE_CONN_MUX_O7_MASK,
				   AFE_CONN_MUX_O4_NULL |
				   AFE_CONN_MUX_O5_NULL |
				   AFE_CONN_MUX_O6_NULL |
				   AFE_CONN_MUX_O7_NULL);
	}

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     mphone multi startup function for struct snd_soc_dai_ops.
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
static int mt_afe_mphone_multi_startup(struct mtk_afe *afe)
{
	dev_dbg(afe->dev, "%s\n", __func__);

	mt_afe_enable_main_clk(afe);
	mt_afe_enable_top_cg(afe, MT_AFE_CG_DP_RX);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     mphone multi shutdown function for struct snd_soc_dai_ops.
 * @param[in]
 *     afe: AFE data
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */

static void mt_afe_mphone_multi_shutdown(struct mtk_afe *afe)
{
	dev_dbg(afe->dev, "%s\n", __func__);

	mt_afe_disable_top_cg(afe, MT_AFE_CG_DP_RX);
	mt_afe_disable_main_clk(afe);
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     mphone multi prepare function for struct snd_soc_dai_ops.
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
static int mt_afe_mphone_multi_prepare(struct mtk_afe *afe)
{
	struct mt_afe_dai_runtime *runtime = &dai_runtime[MT_AFE_MEMIF_DL12];
	struct mt_afe_control_data *data = &afe->ctrl_data;
	const unsigned int channels = runtime->channels;
	const int bit_width = snd_pcm_format_width(runtime->format);
	int be_idx = MT_AFE_IO_MPHONE_MULTI - MT_AFE_BACKEND_BASE;
	unsigned int con0 = 0;
	unsigned int con1 = 0;

	dev_dbg(afe->dev, "%s\n", __func__);

	con0 |= AFE_MPHONE_MULTI_CON0_SDATA0_SEL_SDATA0 |
		AFE_MPHONE_MULTI_CON0_SDATA1_SEL_SDATA1 |
		AFE_MPHONE_MULTI_CON0_SDATA2_SEL_SDATA2 |
		AFE_MPHONE_MULTI_CON0_SDATA3_SEL_SDATA3;

	con1 |= AFE_MPHONE_MULTI_CON1_MULTI_SYNC_EN |
		AFE_MPHONE_MULTI_CON1_HBR_MODE |
		AFE_MPHONE_MULTI_CON1_DELAY_DATA |
		AFE_MPHONE_MULTI_CON1_LEFT_ALIGN |
		AFE_MPHONE_MULTI_CON1_32_LRCK_CYCLES;

	if (data->backend_clk_src[be_idx] == MPHONE_FROM_DPRX) {
		con0 |= AFE_MPHONE_MULTI_CON0_CLK_SRC_DPRX;
		con1 |= AFE_MPHONE_MULTI_CON1_DATA_SRC_DPRX;
	} else if (data->backend_clk_src[be_idx] == MPHONE_FROM_TDM_OUT) {
		con0 |= AFE_MPHONE_MULTI_CON0_CLK_SRC_TDM_OUT;
		con1 |= AFE_MPHONE_MULTI_CON1_DATA_SRC_TDM_OUT;
	}

	if (bit_width > 16) {
		con0 |= AFE_MPHONE_MULTI_CON0_24BIT_MODE;
		con1 |= AFE_MPHONE_MULTI_CON1_INPUT_SRC_BITS(24);
	} else {
		con0 |= AFE_MPHONE_MULTI_CON0_16BIT_MODE;
		con1 |= AFE_MPHONE_MULTI_CON1_INPUT_SRC_BITS(16);
	}

	if (runtime->format == SNDRV_PCM_FORMAT_S16_LE) {
		con0 |= AFE_MPHONE_MULTI_CON0_16BIT_SWAP;
		con1 |= AFE_MPHONE_MULTI_CON1_COMPACT_MODE;
	} else if (runtime->format == SNDRV_PCM_FORMAT_S24_3LE) {
		con1 |= AFE_MPHONE_MULTI_CON1_COMPACT_MODE |
			AFE_MPHONE_MULTI_CON1_24BIT_SWAP_PASS;
	} else {
		con1 |= AFE_MPHONE_MULTI_CON1_NONCOMPACT_MODE;
	}

	if (channels == 8)
		con1 |= AFE_MPHONE_MULTI_CON1_8_CHANNEL;
	else if (channels == 6)
		con1 |= AFE_MPHONE_MULTI_CON1_6_CHANNEL;
	else if (channels == 4)
		con1 |= AFE_MPHONE_MULTI_CON1_4_CHANNEL;
	else
		con1 |= AFE_MPHONE_MULTI_CON1_2_CHANNEL;

	regmap_update_bits(afe->regmap, AFE_MPHONE_MULTI_CON0,
			   AFE_MPHONE_MULTI_CON0_SET_MASK, con0);
	regmap_update_bits(afe->regmap, AFE_MPHONE_MULTI_CON1,
			   AFE_MPHONE_MULTI_CON1_SET_MASK, con1);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     front end dais startup function for struct snd_soc_dai_ops.
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     dai_id: the dai ID
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt_afe_dais_startup(struct mtk_afe *afe, int dai_id)
{
	const struct mt_afe_memif_data *data;
	static struct mt_afe_memif *memif;

	memif = &afe->memif[dai_id];
	data = memif->data;

	dev_dbg(afe->dev, "%s dai_id=%d(%s)\n", __func__, dai_id, data->name);

	mt_afe_enable_main_clk(afe);
	mt_afe_enable_memif_irq_src_clk(afe, data->id, data->irq_mode);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     front end dais shutdown function for struct snd_soc_dai_ops.
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     dai_id: the dai ID
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mt_afe_dais_shutdown(struct mtk_afe *afe, int dai_id)
{
	struct mt_afe_memif *memif = &afe->memif[dai_id];
	const struct mt_afe_memif_data *data = memif->data;

	dev_dbg(afe->dev, "%s dai_id=%d(%s)\n", __func__, dai_id, data->name);

	if (memif->prepared) {
		mt_afe_disable_afe_on(afe);
		memif->prepared = false;
	}

	memif->substream = NULL;

	mt_afe_disable_memif_irq_src_clk(afe, data->id, data->irq_mode);
	mt_afe_disable_main_clk(afe);
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     front end dais hw_params function for struct snd_soc_dai_ops.
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     dai_id: the dai ID
 * @param[in]
 *     request_size: the size of buffer for dai
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt_afe_dais_hw_params(struct mtk_afe *afe, int dai_id, size_t request_size)
{
	struct mt_afe_memif *memif = &afe->memif[dai_id];
	const struct mt_afe_memif_data *data = memif->data;
	struct mt_afe_dai_runtime *runtime = &dai_runtime[dai_id];
	void *vaddr;
	dma_addr_t dma_addr;
	int format_val = 0;
	int format_align_val = -1;
	unsigned int rate = runtime->rate;

	dev_dbg(afe->dev,
		"%s dai_id=%d(%s) period = %lu rate = %u channels = %u size = %lu\n",
		__func__, dai_id, data->name, runtime->period_size,
		rate, runtime->channels, request_size);

	if (request_size > data->max_sram_size) {
		dev_info(afe->dev, "%s %s Use DRAM: request_size is too big: %lu\n",
			 __func__, data->name, request_size);

		vaddr = dma_alloc_coherent(afe->dev, PAGE_SIZE << get_order(request_size), &dma_addr,
					   GFP_KERNEL | __GFP_COMP | __GFP_NORETRY | __GFP_NOWARN);
		if (!vaddr) {
			dev_err(afe->dev,
				"%s %s malloc pages %zu bytes failed.\n",
				__func__, data->name, request_size);
			return -ENOMEM;
		}
		memif->virt_buf_addr = vaddr;
		memif->phys_buf_addr = dma_addr;

		memif->use_sram = false;
	} else if (request_size < MT_AFE_MEMIF_NUM) {
		dev_info(afe->dev, "%s %s Share with other dai buffer\n",
			 __func__, data->name);

		memif->virt_buf_addr = afe->memif[request_size].virt_buf_addr;
		memif->phys_buf_addr = afe->memif[request_size].phys_buf_addr;
		request_size = afe->memif[request_size].buffer_size;
		memif->use_shared_buffer = true;
	} else {
		dev_info(afe->dev, "%s %s Use SRAM: size:%lu\n",
			 __func__, data->name, request_size);

		memif->virt_buf_addr = ((unsigned char *)afe->sram_address) +
				 data->sram_offset;
		memif->phys_buf_addr = afe->sram_phy_address + data->sram_offset;

		memif->use_sram = true;
	}

	memif->buffer_size = request_size;

	/* start */
	regmap_write(afe->regmap, data->reg_ofs_base,
		     memif->phys_buf_addr);

	/* end */
	regmap_write(afe->regmap, data->reg_ofs_end,
		     memif->phys_buf_addr + memif->buffer_size - 1);

	/* set channel, let AWB apply fixed 2ch */
	if (data->mono_shift >= 0 && dai_id != MT_AFE_MEMIF_AWB) {
		unsigned int mono = (runtime->channels == 1) ? 1 : 0;

		regmap_update_bits(afe->regmap, data->data_mono_reg,
				   1 << data->mono_shift,
				   mono << data->mono_shift);
	}

	if (data->four_ch_shift >= 0) {
		unsigned int four_ch = (runtime->channels == 4) ? 1 : 0;

		regmap_update_bits(afe->regmap, AFE_MEMIF_PBUF_SIZE,
				   1 << data->four_ch_shift,
				   four_ch << data->four_ch_shift);
	}

	if (dai_id == MT_AFE_MEMIF_AWB) {
		switch (runtime->format) {
		case SNDRV_PCM_FORMAT_S16_LE:
			format_val = 0;
			format_align_val = -1;
			break;
		case SNDRV_PCM_FORMAT_S32_LE:
		case SNDRV_PCM_FORMAT_S24_3LE:
			format_val = 2;
			if (data->format_align_shift >= 0)
				format_align_val = 1;
			break;
		case SNDRV_PCM_FORMAT_S24_LE:
			format_val = 1;
			if (data->format_align_shift >= 0)
				format_align_val = 0;
			break;
		default:
			return -EINVAL;
		}
	} else if (data->format_shift >= 0) {
		switch (runtime->format) {
		case SNDRV_PCM_FORMAT_S16_LE:
			format_val = 0;
			format_align_val = -1;
			break;
		case SNDRV_PCM_FORMAT_S32_LE:
			format_val = 2;
			format_align_val = -1;
			break;
		case SNDRV_PCM_FORMAT_S24_LE:
			format_val = 1;
			if (data->format_align_shift >= 0)
				format_align_val = 0;
			break;
		default:
			return -EINVAL;
		}
	}

	if (data->format_shift >= 0)
		regmap_update_bits(afe->regmap, data->format_reg,
				   data->format_mask << data->format_shift,
				   format_val << data->format_shift);

	if (format_align_val >= 0)
		regmap_update_bits(afe->regmap, AFE_MEMIF_HDALIGN,
				   1 << data->format_align_shift,
				   format_align_val <<
				   data->format_align_shift);

	/* set rate */
	if (data->fs_shift >= 0) {
		int fs = mt_afe_data_fs(rate);

		if (fs < 0)
			return -EINVAL;

		fs |= mt_afe_get_memif_clk_src_mask(afe, dai_id, rate);

		regmap_update_bits(afe->regmap, data->data_fs_reg,
				   0x7f << data->fs_shift,
				   fs << data->fs_shift);
	}

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     front end dais hw_free function for struct snd_soc_dai_ops.
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     dai_id: the dai ID
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt_afe_dais_hw_free(struct mtk_afe *afe, int dai_id)
{
	struct mt_afe_memif *memif = &afe->memif[dai_id];
	int ret = 0;
	int pg;

	dev_dbg(afe->dev, "%s dai_id=%d(%s)\n", __func__, dai_id, memif->data->name);

	if (memif->use_shared_buffer) {
		memif->use_shared_buffer = false;
	} else if (memif->use_sram) {
		memif->use_sram = false;
	} else {
		if (memif->virt_buf_addr) {
			pg = get_order(memif->buffer_size);
			dma_free_coherent(afe->dev, PAGE_SIZE << pg, memif->virt_buf_addr, memif->phys_buf_addr);
		}
	}

	memif->virt_buf_addr = NULL;
	memif->phys_buf_addr = 0;
	memif->buffer_size = 0;

	return ret;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     front end dais prepare function for struct snd_soc_dai_ops.
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     dai_id: the dai ID
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt_afe_dais_prepare(struct mtk_afe *afe, int dai_id)
{
	struct mt_afe_memif *memif = &afe->memif[dai_id];

	dev_dbg(afe->dev, "%s dai_id=%d(%s)\n", __func__, dai_id, memif->data->name);

	if (!memif->prepared) {
		mt_afe_enable_afe_on(afe);
		memif->prepared = true;
	}

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     front end dais trigger function for struct snd_soc_dai_ops.
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     dai_id: the dai ID
 * @param[in]
 *     cmd: triggering command
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt_afe_dais_trigger(struct mtk_afe *afe, int dai_id, int cmd)
{
	struct mt_afe_dai_runtime *runtime = &dai_runtime[dai_id];
	struct mt_afe_memif *memif = &afe->memif[dai_id];
	const struct mt_afe_memif_data *data = memif->data;
	unsigned int counter = runtime->period_size;
	int irq_delay_counter = data->irq_delay_cnt;

	dev_dbg(afe->dev, "%s dai_id=%d(%s) cmd=%d\n", __func__, dai_id, data->name, cmd);

	if (data->id == MT_AFE_MEMIF_AWB) {
		if (runtime->format == SNDRV_PCM_FORMAT_S16_LE)
			counter = counter >> 1;
		else if (runtime->format == SNDRV_PCM_FORMAT_S24_3LE)
			counter = (counter >> 2) * 3;

		if (runtime->channels > 6)
			counter *= 4;
		else if (runtime->channels > 4)
			counter *= 3;
		else if (runtime->channels > 2)
			counter *= 2;

		irq_delay_counter = irq_delay_counter * counter /
			runtime->period_size;
	}

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		/* set irq counter */
		regmap_update_bits(afe->regmap,
				   data->irq_reg_cnt,
				   0x3ffff << data->irq_cnt_shift,
				   counter << data->irq_cnt_shift);

		/* set irq fs */
		if (data->irq_fs_shift >= 0) {
			int fs = mt_afe_data_fs(runtime->rate);

			if (fs < 0)
				return -EINVAL;

			fs |= mt_afe_get_irq_clk_src_mask(afe,
							  data->irq_mode, runtime->rate);

			regmap_update_bits(afe->regmap,
					   data->irq_fs_reg,
					   0x7f << data->irq_fs_shift,
					   fs << data->irq_fs_shift);
		}

		if (runtime->no_period_wakeup) {
			if (data->enable_shift >= 0)
				regmap_update_bits(afe->regmap,
						   data->enable_reg,
						   1 << data->enable_shift,
						   1 << data->enable_shift);
		} else if (irq_delay_counter > 0) {
			mt_afe_set_irq_delay_counter(afe, memif,
						     irq_delay_counter);

			if (data->irq_delay_src_shift > 0)
				regmap_update_bits(afe->regmap,
						   AFE_IRQ_DELAY_CON,
						   0x7 <<
						   data->irq_delay_src_shift,
						   data->irq_delay_src <<
						   data->irq_delay_src_shift);

			regmap_update_bits(afe->regmap, AFE_IRQ_DELAY_CON,
					   1 << data->irq_delay_en_shift,
					   1 << data->irq_delay_en_shift);

			mt_afe_enable_irq(afe, memif);

			if (data->enable_shift >= 0)
				regmap_update_bits(afe->regmap,
						   data->enable_reg,
						   1 << data->enable_shift,
						   1 << data->enable_shift);
		} else {
			if (data->enable_shift >= 0)
				regmap_update_bits(afe->regmap,
						   data->enable_reg,
						   1 << data->enable_shift,
						   1 << data->enable_shift);

			mt_afe_enable_irq(afe, memif);
		}
		return 0;

	case SNDRV_PCM_TRIGGER_START_NOIRQ:
		if (data->enable_shift >= 0)
			regmap_update_bits(afe->regmap,
					   data->enable_reg,
					   1 << data->enable_shift,
					   1 << data->enable_shift);
		return 0;
	case SNDRV_PCM_TRIGGER_STOP:
		if (data->enable_shift >= 0)
			regmap_update_bits(afe->regmap, data->enable_reg,
					   1 << data->enable_shift, 0);

		if (!runtime->no_period_wakeup) {
			mt_afe_disable_irq(afe, memif);

			if (irq_delay_counter > 0) {
				regmap_update_bits(afe->regmap,
						   AFE_IRQ_DELAY_CON,
						   1 <<
						   data->irq_delay_en_shift, 0);

				if (data->irq_delay_src_shift > 0)
					regmap_update_bits(afe->regmap,
							   AFE_IRQ_DELAY_CON,
							   0x7 <<
							   data->irq_delay_src_shift,
							   0 <<
							   data->irq_delay_src_shift);
			}
		}
		return 0;
	case SNDRV_PCM_TRIGGER_STOP_NOIRQ:
		if (data->enable_shift >= 0)
			regmap_update_bits(afe->regmap, data->enable_reg,
					   1 << data->enable_shift, 0);
		return 0;
	default:
		return -EINVAL;
	}
}

#ifdef COMMON_CLOCK_FRAMEWORK_API
static const char *aud_clks[MT_CLK_NUM] = {
	[MT_CLK_AUD_INTBUS_CG] = "aud_intbus_cg",
	[MT_CLK_APLL1_CK] = "apll1_ck",
	[MT_CLK_APLL2_CK] = "apll2_ck",
	[MT_CLK_DPAPLL_CK] = "dpapll_ck",
	[MT_CLK_DPAPLL_D3] = "dpapll_d3",
	[MT_CLK_A1SYS_HP_CG] = "a1sys_hp_cg",
	[MT_CLK_A2SYS_HP_CG] = "a2sys_hp_cg",
	[MT_CLK_A3SYS_HP_CG] = "a3sys_hp_cg",
	[MT_CLK_A3SYS_HP_SEL] = "a3sys_hp_sel",
	[MT_CLK_TDMOUT_SEL] = "tdmout_sel",
	[MT_CLK_TDMIN_SEL] = "tdmin_sel",
	[MT_CLK_APLL12_DIV0] =  "apll12_div0",
	[MT_CLK_APLL12_DIV1] =  "apll12_div1",
	[MT_CLK_APLL12_DIV2] =  "apll12_div2",
	[MT_CLK_APLL12_DIV3] =  "apll12_div3",
	[MT_CLK_AUD_MAS_SLV_BCLK] =  "aud_mas_slv_bclk",
};
#endif

static const struct mt_afe_memif_data memif_data[MT_AFE_MEMIF_NUM] = {
	{
		.name = "DL",
		.id = MT_AFE_MEMIF_DL12,
		.reg_ofs_base = AFE_DL12_BASE,
		.reg_ofs_end = AFE_DL12_END,
		.reg_ofs_cur = AFE_DL12_CUR,
		.data_fs_reg = AFE_DAC_CON1,
		.fs_shift = 0,
		.data_mono_reg = AFE_DAC_CON1,
		.mono_shift = 20,
		.four_ch_shift = 16,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 8,
		.irq_reg_cnt = AFE_IRQ0_MCU_CNT,
		.irq_cnt_shift = 0,
		.irq_mode = MT_AFE_IRQ_0,
		.irq_fs_reg = AFE_IRQ0_MCU_CNT,
		.irq_fs_shift = 20,
		.irq_clr_shift = 0,
		.irq_delay_cnt = 0,
		.irq_delay_en_shift = 0,
		.irq_delay_src = 2,
		.irq_delay_src_shift = 8,
		.max_sram_size = 0x1200,
		.sram_offset = 0,
		.format_reg = AFE_MEMIF_HD_MODE,
		.format_shift = 2,
		.format_mask = 0x3,
		.format_align_shift = 1,
		.prealloc_size = 128 * 1024,
	}, {
		.name = "VUL",
		.id = MT_AFE_MEMIF_VUL12,
		.reg_ofs_base = AFE_VUL12_BASE,
		.reg_ofs_end = AFE_VUL12_END,
		.reg_ofs_cur = AFE_VUL12_CUR,
		.data_fs_reg = AFE_DAC_CON0,
		.fs_shift = 16,
		.data_mono_reg = AFE_DAC_CON0,
		.mono_shift = 10,
		.four_ch_shift = 17,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 9,
		.irq_reg_cnt = AFE_IRQ1_MCU_CNT,
		.irq_cnt_shift = 0,
		.irq_mode = MT_AFE_IRQ_1,
		.irq_fs_reg = AFE_IRQ1_MCU_CNT,
		.irq_fs_shift = 20,
		.irq_clr_shift = 1,
		.irq_delay_cnt = 4,
		.irq_delay_en_shift = 1,
		.irq_delay_src = 3,
		.irq_delay_src_shift = 12,
		.max_sram_size = 0x1200,
		.sram_offset = 0x1200,
		.format_reg = AFE_MEMIF_HD_MODE,
		.format_shift = 12,
		.format_mask = 0x3,
		.format_align_shift = 6,
		.prealloc_size = 32 * 1024,
	}, {
		.name = "AWB",
		.id = MT_AFE_MEMIF_AWB,
		.reg_ofs_base = AFE_AWB_BASE,
		.reg_ofs_end = AFE_AWB_END,
		.reg_ofs_cur = AFE_AWB_CUR,
		.data_fs_reg = -1,
		.fs_shift = -1,
		.data_mono_reg = AFE_DAC_CON1,
		.mono_shift = 24,
		.four_ch_shift = -1,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 6,
		.irq_reg_cnt = AFE_IRQ_MCU_CNT4,
		.irq_cnt_shift = 0,
		.irq_mode = MT_AFE_IRQ_4,
		.irq_fs_reg = -1,
		.irq_fs_shift = -1,
		.irq_clr_shift = 4,
		.irq_delay_cnt = 0,
		.irq_delay_en_shift = 4,
		.irq_delay_src = -1,
		.irq_delay_src_shift = -1,
		.max_sram_size = 0,
		.sram_offset = 0,
		.format_reg = AFE_MEMIF_HD_MODE,
		.format_shift = 8,
		.format_mask = 0x3,
		.format_align_shift = 4,
		.prealloc_size = 0,
	}, {
		.name = "TDM_OUT",
		.id = MT_AFE_MEMIF_TDM_OUT,
		.reg_ofs_base = AFE_TDM_OUT_BASE,
		.reg_ofs_end = AFE_TDM_OUT_END,
		.reg_ofs_cur = AFE_TDM_OUT_CUR,
		.data_fs_reg = -1,
		.fs_shift = -1,
		.data_mono_reg = -1,
		.mono_shift = -1,
		.four_ch_shift = -1,
		.enable_reg = AFE_TDM_OUT_CON0,
		.enable_shift = 0,
		.irq_reg_cnt = AFE_IRQ6_MCU_CNT,
		.irq_cnt_shift = 0,
		.irq_mode = MT_AFE_IRQ_6,
		.irq_fs_reg = -1,
		.irq_fs_shift = -1,
		.irq_clr_shift = 6,
		.irq_delay_cnt = 0,
		.irq_delay_en_shift = 6,
		.irq_delay_src = -1,
		.irq_delay_src_shift = -1,
		.max_sram_size = 0,
		.sram_offset = 0,
		.format_reg = AFE_MEMIF_HD_MODE,
		.format_shift = 20,
		.format_mask = 0x3,
		.format_align_shift = 10,
		.prealloc_size = 0,
	}, {
		.name = "TDM_IN",
		.id = MT_AFE_MEMIF_TDM_IN,
		.reg_ofs_base = AFE_TDM_IN_BASE,
		.reg_ofs_end = AFE_TDM_IN_END,
		.reg_ofs_cur = AFE_TDM_IN_WR_CUR,
		.data_fs_reg = -1,
		.fs_shift = -1,
		.data_mono_reg = -1,
		.mono_shift = -1,
		.four_ch_shift = -1,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 12,
		.irq_reg_cnt = AFE_IRQ5_MCU_CNT,
		.irq_cnt_shift = 0,
		.irq_mode = MT_AFE_IRQ_5,
		.irq_fs_reg = -1,
		.irq_fs_shift = -1,
		.irq_clr_shift = 5,
		.irq_delay_cnt = 2,
		.irq_delay_en_shift = 5,
		.irq_delay_src = -1,
		.irq_delay_src_shift = -1,
		.max_sram_size = 0,
		.sram_offset = 0,
		.format_reg = AFE_MEMIF_HD_MODE,
		.format_shift = 30,
		.format_mask = 0x3,
		.format_align_shift = 15,
		.prealloc_size = 0,
	},
};

static const struct regmap_config mt_afe_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = AFE_MAX_REGISTER,
	.cache_type = REGCACHE_NONE,
};

/** @ingroup type_group_afe_InFn
 * @par Description
 *     interrupt handler function.
 * @param[in]
 *     irq: interrupt number
 * @param[in]
 *     dev_id: cookie to identify the device
 * @return
 *     irq handled status.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static irqreturn_t mt_afe_irq_handler_capture(int irq, void *dev_id)
{
	struct mtk_afe *afe = dev_id;
	unsigned int value;
	unsigned int memif_status;
	int i, ret;

	if (!afe->regmap) {
		dev_err(afe->dev, "%s invalid regmap\n", __func__);
		return IRQ_NONE;
	}

	ret = regmap_read(afe->regmap, AFE_IRQ_MCU_STATUS, &value);
	if (ret) {
		dev_err(afe->dev, "%s irq status err\n", __func__);
		value = AFE_IRQ_STATUS_BITS;
		goto err_irq;
	}

	ret = regmap_read(afe->regmap, AFE_DAC_CON0, &memif_status);
	if (ret) {
		dev_err(afe->dev, "%s memif status err\n", __func__);
		value = AFE_IRQ_STATUS_BITS;
		goto err_irq;
	}

	for (i = 0; i < MT_AFE_MEMIF_NUM; i++) {
		struct mt_afe_memif *memif = &afe->memif[i];

		if (!(value & (1 << memif->data->irq_clr_shift)))
			continue;

		if (memif->data->enable_shift >= 0 &&
		    !((1 << memif->data->enable_shift) & memif_status))
			continue;

#ifdef CONFIG_SIE_DEVELOP_BUILD
		if (i == MT_AFE_MEMIF_DL12) {
			u32 sample_num;
			u32 period_size;

			period_size = dai_runtime[i].period_size;
			regmap_read(afe->regmap, AFE_DL12_RD_COUNTER, &sample_num);
			if (sample_num - afe_data.playback_pos >= period_size) {
				wake_up(&afe_data.playback_waitq);
				afe_data.playback_pos += period_size;
			}
		}

		wake_up(&afe_data.capture_waitq[i]);
#endif

		if (i == afe_data.callback_capture_io && afe_data.enable_capture_callback) {
			if (afe_data.usb_callback) {
				afe_data.usb_callback();
			}
		}
	}

err_irq:
	/* clear irq */
	regmap_write(afe->regmap, AFE_IRQ_MCU_CLR, value & AFE_IRQ_STATUS_BITS);

	return IRQ_HANDLED;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     initialize hardware registers.
 * @param[in]
 *     afe: AFE data
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mt_afe_init_registers(struct mtk_afe *afe)
{
	dev_dbg(afe->dev, "%s\n", __func__);

	regmap_update_bits(afe->regmap, AFE_IRQ_MCU_EN, GENMASK(31, 0),
			   AFE_IRQ_MCU_EN_ALL);

	regmap_update_bits(afe->regmap,
			   AFE_MEMIF_MSB,
			   AFE_MEMIF_MSB_TDM_IN_SIGN_MASK |
			   AFE_MEMIF_MSB_VUL12_SIGN_MASK |
			   AFE_MEMIF_MSB_AWB_SIGN_MASK,
			   0x0);
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     get audio clock handle.
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
static int mt_afe_init_audio_clk(struct mtk_afe *afe)
{
#ifdef COMMON_CLOCK_FRAMEWORK_API
	size_t i;

	dev_dbg(afe->dev, "%s\n", __func__);

	for (i = 0; i < ARRAY_SIZE(aud_clks); i++) {
		afe->clocks[i] = devm_clk_get(afe->dev, aud_clks[i]);
		if (IS_ERR(afe->clocks[i])) {
			dev_err(afe->dev, "%s devm_clk_get %s fail\n",
				__func__, aud_clks[i]);
			return PTR_ERR(afe->clocks[i]);
		}
	}
#endif
	return 0;
}

int mt_afe_open(struct device *dev)
{
	dev_dbg(dev, "%s\n", __func__);
	/* Do nothing */

	return 0;
}

struct mt_afe_clk_source_name {
	char *name;
};

static struct mt_afe_clk_source_name mt_afe_source_pll_name[] = {
	[FROM_AUDIO_PLL] = { .name = "FROM_AUDIO_PLL" },
	[FROM_DPRX_PLL] = { .name = "FROM_DPRX_PLL" }
};

static int mt_afe_set_clk_src(struct mtk_afe *afe, unsigned int clk_src)
{
	/* clk_src = FROM_AUDIO_PLL / FROM_DPRX_PLL, */

	struct mt_afe_control_data *ctrl_data = &afe->ctrl_data;

	dev_dbg(afe->dev, "%s clk_src=%d(%s)\n", __func__, clk_src, mt_afe_source_pll_name[clk_src].name);

	ctrl_data->vul12_clk_src = clk_src;
	ctrl_data->dl12_clk_src = clk_src;
	ctrl_data->backend_clk_src[MT_AFE_IO_I2S - MT_AFE_BACKEND_BASE] = clk_src;

	return 0;
}

static struct mt_afe_clk_source_name  mt_afe_mphone_source_name[] = {
	[MPHONE_FROM_DPRX] = { .name = "MPHONE_FROM_DPRX" },
	[MPHONE_FROM_TDM_OUT] = { .name = "MPHONE_FROM_TDM_OUT" }
};

static int mt_afe_set_mphone_clk_src(struct mtk_afe *afe, unsigned int clk_src)
{
	/* clk_src = MPHONE_FROM_DPRX / MPHONE_FROM_TDM_OUT */

	struct mt_afe_control_data *ctrl_data = &afe->ctrl_data;

	dev_dbg(afe->dev, "%s clk_src=%d(%s)\n", __func__, clk_src, mt_afe_mphone_source_name[clk_src].name);

	ctrl_data->backend_clk_src[MT_AFE_IO_MPHONE_MULTI - MT_AFE_BACKEND_BASE] = clk_src;

	return 0;
}

int mt_afe_prepare(struct device *dev)
{
	struct mtk_afe *afe = dev_get_drvdata(dev);
	const struct mt_afe_memif_data *data;
	static struct mt_afe_memif *memif;

	memif = &afe->memif[MT_AFE_MEMIF_VUL12];
	data = &memif_data[MT_AFE_MEMIF_VUL12];
	memif->data = data;

	if (!afe_data.prepared) {
		mt_afe_set_clk_src(afe, FROM_AUDIO_PLL);
		mt_afe_set_mphone_clk_src(afe, MPHONE_FROM_DPRX);
	}

#ifdef USE_SRAM
	mt_afe_dais_hw_params(afe, MT_AFE_MEMIF_DL12, afe->memif[MT_AFE_MEMIF_DL12].data->max_sram_size);
	mt_afe_dais_hw_params(afe, MT_AFE_MEMIF_VUL12, afe->memif[MT_AFE_MEMIF_VUL12].data->max_sram_size);
#else
	/* Use default buffer size (prealloc_size) */
	mt_afe_dais_hw_params(afe, MT_AFE_MEMIF_DL12, afe->memif[MT_AFE_MEMIF_DL12].data->prealloc_size);
	mt_afe_dais_hw_params(afe, MT_AFE_MEMIF_VUL12, afe->memif[MT_AFE_MEMIF_VUL12].data->prealloc_size);
#endif
	mt_afe_dais_hw_params(afe, MT_AFE_MEMIF_AWB, MT_AFE_MEMIF_DL12);

	mt_afe_i2s_prepare(afe);

	mt_afe_mphone_multi_prepare(afe);

	mt_afe_dais_prepare(afe, MT_AFE_MEMIF_DL12);
	mt_afe_dais_prepare(afe, MT_AFE_MEMIF_VUL12);

	afe_data.prepared = true;

	return 0;
}

int mt_afe_transfer_start(struct device *dev, enum afe_input_port_type port_type, unsigned int rate)
{
	struct mtk_afe *afe = dev_get_drvdata(dev);
	struct mt_afe_dai_runtime *runtime = &dai_runtime[MT_AFE_MEMIF_DL12];

	dev_dbg(afe->dev, "%s port_type=%d rate=%d\n", __func__, port_type, rate);

	if (!afe_data.prepared) {
		dev_err(afe->dev, "%s: audio driver is not prepared.\n", __func__);
		return -EINVAL;
	}

	if (rate != runtime->rate) {
		dev_info(afe->dev, "Sampling rate is changed from %u to %u\n", runtime->rate, rate);
		dai_runtime[MT_AFE_MEMIF_DL12].rate = rate;
		dai_runtime[MT_AFE_MEMIF_VUL12].rate = rate;
		dai_runtime[MT_AFE_MEMIF_AWB].rate = rate;
	} else {
		dev_info(afe->dev, "Sampling rate is same as previous. %u\n", runtime->rate);
	}

	if (afe_data.transfer_port == port_type) {
		dev_info(afe->dev, "%s: no port change. port=%d\n", __func__, port_type);
		mt_afe_i2s_trigger(afe, MT_AFE_MEMIF_DL12, true); /* Enable */
		return 0;
	}

	afe_data.transfer_start = true;
	afe_data.transfer_port = port_type;

	mt_afe_i2s_trigger(afe, MT_AFE_MEMIF_VUL12, false);  /* Avoid pop noise issue */

	mt_afe_set_mphone_multi_enable(afe, false);
	mt_afe_set_i2s_enable(afe, false);

	if (afe_data.transfer_port == AFE_INPUT_PORT_DP) {
		mt_afe_set_clk_src(afe, FROM_DPRX_PLL);
	} else {
		dev_err(afe->dev, "%s: unsupported transfer port.\n", __func__);
	}

	if (afe_data.transfer_port == AFE_INPUT_PORT_DP) {
		mt_afe_set_mphone_clk_src(afe, MPHONE_FROM_DPRX);
	} else {
		dev_err(afe->dev, "%s: unsupported transfer port.\n", __func__);
	}

	mt_afe_prepare(dev);
	mt_afe_mphone_multi_prepare(afe);
	mt_afe_i2s_prepare(afe);

	/* enable MPHONE and AWB */
	mt_afe_enable_afe_on(afe);
	mt_afe_set_mphone_multi_enable(afe, true);
	mt_afe_dais_trigger(afe, MT_AFE_MEMIF_AWB, SNDRV_PCM_TRIGGER_START_NOIRQ);
	afe_data.trigger_start_cnt[MT_AFE_MEMIF_AWB] = 1;

	usleep_range(TRANSFER_DELAY_TIME, (TRANSFER_DELAY_TIME * 2));

	/* enable I2S out and DL12 */
	mt_afe_set_i2s_enable(afe, true);
	mt_afe_i2s_trigger(afe, MT_AFE_MEMIF_DL12, true); /* Enable */
	mt_afe_dais_trigger(afe, MT_AFE_MEMIF_DL12, SNDRV_PCM_TRIGGER_START_NOIRQ);

	if (afe_data.trigger_start_cnt[MT_AFE_MEMIF_VUL12] != 0) {
		mt_afe_i2s_trigger(afe, MT_AFE_MEMIF_VUL12, true);  /* Avoid pop noise issue */
	}

	return 0;
}

int mt_afe_transfer_stop(struct device *dev)
{
	struct mtk_afe *afe = dev_get_drvdata(dev);

	dev_dbg(afe->dev, "%s\n", __func__);

	mt_afe_i2s_trigger(afe, MT_AFE_MEMIF_DL12, false); /* Disable */

	if (!afe_data.transfer_start) {
		/* Do nothing */
		return 0;
	}

	afe_data.transfer_start = false;

	return 0;
}

int mt_afe_capture_start(struct device *dev, int dai_id)
{
	struct mtk_afe *afe = dev_get_drvdata(dev);

	dev_dbg(afe->dev, "%s\n", __func__);

	if (!afe_data.prepared) {
		dev_err(afe->dev, "%s: audio driver is not prepared.\n", __func__);
		return -EINVAL;
	}

	if (dai_id >= MT_AFE_MEMIF_NUM) {
		dev_err(afe->dev, "%s: dai_id error. dai_id:%d\n", __func__, dai_id);
		return -EINVAL;
	}

	afe_data.trigger_start_cnt[dai_id]++;
	if (afe_data.trigger_start_cnt[dai_id] != 1) {
		dev_dbg(afe->dev, "%s: dai_id=%d has already triggered.\n", __func__, dai_id);
		return 0;
	}
	mt_afe_i2s_trigger(afe, dai_id, true);
	mt_afe_dais_trigger(afe, dai_id, SNDRV_PCM_TRIGGER_START);

	return 0;
}

int mt_afe_capture_stop(struct device *dev, int dai_id)
{
	struct mtk_afe *afe = dev_get_drvdata(dev);

	dev_dbg(afe->dev, "%s\n", __func__);

	if (!afe_data.prepared) {
		dev_err(afe->dev, "%s: audio driver is not prepared.\n", __func__);
		return -EINVAL;
	}

	if (dai_id >= MT_AFE_MEMIF_NUM) {
		dev_err(afe->dev, "%s: dai_id error. dai_id:%d\n", __func__, dai_id);
		return -EINVAL;
	}

	afe_data.trigger_start_cnt[dai_id]--;
	if (afe_data.trigger_start_cnt[dai_id] > 0) {
		dev_dbg(afe->dev, "%s:SNDRV_PCM_TRIGGER_STOP: dai_id=%d keep trigger start. cnt:%d.\n",
			__func__, dai_id, afe_data.trigger_start_cnt[dai_id]);
		return 0;
	}
	afe_data.trigger_start_cnt[dai_id] = 0;
	mt_afe_i2s_trigger(afe, dai_id, false);
	mt_afe_dais_trigger(afe, dai_id, SNDRV_PCM_TRIGGER_STOP);

	return 0;
}

unsigned int mt_afe_capture_pointer(struct device *dev, const int dai_id)
{
	struct mtk_afe *afe = dev_get_drvdata(dev);
	struct mt_afe_memif *memif = &afe->memif[dai_id];
	unsigned int hw_ptr;
	int ret;

	ret = regmap_read(afe->regmap, memif->data->reg_ofs_cur, &hw_ptr);
#ifdef DEBUG
	dev_dbg(afe->dev, "%s: hw_ptr = 0x%x\n", __func__, hw_ptr);
#endif
	if (ret || hw_ptr == 0) {
		dev_err(afe->dev, "%s hw_ptr err ret = %d\n", __func__, ret);
		hw_ptr = memif->phys_buf_addr;
	} else if (memif->use_sram) {
		/* enforce natural alignment to 8 bytes */
		hw_ptr &= ~7;
	}

	return hw_ptr - memif->phys_buf_addr;
}

#ifdef CONFIG_SIE_DEVELOP_BUILD
int mt_afe_capture_read(struct device *dev, void *buf, unsigned int size, const int dai_id)
{
	struct mtk_afe *afe = dev_get_drvdata(dev);
	struct mt_afe_memif *memif = &afe->memif[dai_id];
	unsigned int current_pos, available;
	wait_queue_t wait;

	dev_dbg(afe->dev, "%s\n", __func__);

	init_waitqueue_entry(&wait, current);
	set_current_state(TASK_INTERRUPTIBLE);
	add_wait_queue(&afe_data.capture_waitq[dai_id], &wait);

	while (size > 0) {
		current_pos = mt_afe_capture_pointer(dev, dai_id);
		if (afe_data.captured_pos[dai_id] > current_pos) {
			current_pos = memif->buffer_size;
		}
		available = (current_pos - afe_data.captured_pos[dai_id]);
		available = available > size ? size : available;
		if (available > 0) {
			dev_dbg(dev, "%s: available: 0x%x size: 0x%x\n", __func__, available, size);
			dev_dbg(dev, "    afe_data.captured_pos: 0x%x current_pos: 0x%x\n", afe_data.captured_pos[dai_id], current_pos);

			if (copy_to_user(buf, (void *)((char *)(memif->virt_buf_addr) + afe_data.captured_pos[dai_id]), available)) {
				dev_err(afe->dev, "%s: copy_to_user faile\n", __func__);
				break;
			}
			size -= available;
			buf = (char *)buf + available;
			if (current_pos == memif->buffer_size) {
				afe_data.captured_pos[dai_id] = 0;
			} else {
				afe_data.captured_pos[dai_id] += available;
			}
		}
		schedule_timeout(MAX_SCHEDULE_TIMEOUT);
		set_current_state(TASK_INTERRUPTIBLE);
	}
	set_current_state(TASK_RUNNING);
	remove_wait_queue(&afe_data.capture_waitq[dai_id], &wait);

	return 0;
}

int mt_afe_playback_start(struct device *dev)
{
	struct mtk_afe *afe = dev_get_drvdata(dev);

	dev_dbg(afe->dev, "%s\n", __func__);

	if (!afe_data.prepared) {
		dev_err(afe->dev, "%s: audio driver is not prepared.\n", __func__);
		return -EINVAL;
	}

	if (afe_data.transfer_start) {
		dev_err(afe->dev, "%s: DP transfer is started. Can not start Playback.\n", __func__);
		return -EINVAL;
	}
	mt_afe_i2s_trigger(afe, MT_AFE_MEMIF_DL12, true);
	mt_afe_dais_trigger(afe, MT_AFE_MEMIF_DL12, SNDRV_PCM_TRIGGER_START);

	regmap_read(afe->regmap, AFE_DL12_RD_COUNTER, &afe_data.playback_pos);
	afe_data.transfer_port = AFE_INPUT_PORT_NONE;
	afe_data.playback_start = true;

	return 0;
}

int mt_afe_playback_stop(struct device *dev)
{
	struct mtk_afe *afe = dev_get_drvdata(dev);

	dev_dbg(afe->dev, "%s\n", __func__);

	if (!afe_data.playback_start) {
		return 0;
	}

	mt_afe_i2s_trigger(afe, MT_AFE_MEMIF_DL12, false);
	mt_afe_dais_trigger(afe, MT_AFE_MEMIF_DL12, SNDRV_PCM_TRIGGER_STOP);
	afe_data.playback_start = false;
	regmap_write(afe->regmap, AFE_DL_UL_COUNTER_CON, 1 << 1);

	return 0;
}

int mt_afe_playback_write(struct device *dev, void *buf, unsigned int size)
{
	struct mtk_afe *afe = dev_get_drvdata(dev);
	struct mt_afe_memif *memif = &afe->memif[MT_AFE_MEMIF_DL12];
	static u32 current_pos;
	wait_queue_t wait;
	unsigned int remain_size;
	u32 period_size;
	u32 channels;
	u32 bytes_per_sample;

	period_size = dai_runtime[MT_AFE_MEMIF_DL12].period_size;
	channels = dai_runtime[MT_AFE_MEMIF_DL12].channels;
	bytes_per_sample = snd_pcm_format_width(dai_runtime[MT_AFE_MEMIF_DL12].format) / 8;
	remain_size = memif->data->max_sram_size - current_pos;
	dev_dbg(afe->dev, "%s\n", __func__);

	if (afe_data.playback_start) {
		if (size > period_size * channels * bytes_per_sample) {
			dev_err(afe->dev, "%s: write_size error%d\n", __func__, size);
			return -EINVAL;
		}
		init_waitqueue_entry(&wait, current);
		set_current_state(TASK_INTERRUPTIBLE);
		add_wait_queue(&afe_data.playback_waitq, &wait);
		if (remain_size < size) {
			if (copy_from_user((void *)((uint8_t *)(memif->virt_buf_addr) + current_pos), buf, remain_size)) {
				dev_err(afe->dev, "%s: copy_from_user_fail\n", __func__);
			}
			if (copy_from_user((void *)(uint8_t *)(memif->virt_buf_addr),
					   (void *)((uint8_t *)(buf) + remain_size), size - remain_size)) {
				dev_err(afe->dev, "%s: copy_from_user_fail\n", __func__);
			}
			current_pos = size - remain_size;
		} else {
			if (copy_from_user((void *)((uint8_t *)(memif->virt_buf_addr) + current_pos), buf, size)) {
				dev_err(afe->dev, "%s: copy_from_user_fail\n", __func__);
			}
			current_pos = (current_pos + size) % memif->data->max_sram_size;
		}
		schedule_timeout(MAX_SCHEDULE_TIMEOUT);
		set_current_state(TASK_RUNNING);
		remove_wait_queue(&afe_data.playback_waitq, &wait);
	} else {
		if (size > memif->buffer_size) {
			dev_err(afe->dev, "%s: write_size error%d\n", __func__, size);
			return -EINVAL;
		}
		current_pos = 0;
		if (copy_from_user((void *)((uint8_t *)(memif->virt_buf_addr) + current_pos), buf, size)) {
			dev_err(afe->dev, "%s: copy_from_user fail\n", __func__);
		}
		current_pos += size;
	}
	return size;
}

int mt_afe_start_tonegen(struct device *dev)
{
	struct mtk_afe *afe = dev_get_drvdata(dev);

	if (!afe_data.prepared) {
		dev_err(afe->dev, "%s: audio driver is not prepared.\n", __func__);
		return -EINVAL;
	}

	if (afe_data.transfer_start) {
		dev_err(afe->dev, "%s: DP transfer is started. Can not start tone generator.\n", __func__);
		return -EINVAL;
	}

	regmap_update_bits(afe->regmap, AFE_SGEN_CON2, AFE_SGEN_CON2_EN_MASK, AFE_SGEN_CON2_EN);
	regmap_write(afe->regmap, AFE_SGEN_CON0, AFE_SGEN_CON0_EN_CUST);

	mt_afe_enable_afe_on(afe);
	mt_afe_set_mphone_multi_enable(afe, true);
	mt_afe_dais_trigger(afe, MT_AFE_MEMIF_AWB, SNDRV_PCM_TRIGGER_START_NOIRQ);
	afe_data.trigger_start_cnt[MT_AFE_MEMIF_AWB] = 1;

	usleep_range(TRANSFER_DELAY_TIME, (TRANSFER_DELAY_TIME * 2));

	mt_afe_i2s_trigger(afe, MT_AFE_MEMIF_DL12, true);
	mt_afe_dais_trigger(afe, MT_AFE_MEMIF_DL12, SNDRV_PCM_TRIGGER_START_NOIRQ);

	afe_data.transfer_port = AFE_INPUT_PORT_NONE;
	afe_data.tonegen_start = true;

	return 0;
}

int mt_afe_stop_tonegen(struct device *dev)
{
	struct mtk_afe *afe = dev_get_drvdata(dev);

	if (!afe_data.tonegen_start) {
		return 0;
	}
	regmap_update_bits(afe->regmap, AFE_SGEN_CON2, AFE_SGEN_CON2_EN_MASK, AFE_SGEN_CON2_DISABLE);
	mt_afe_i2s_trigger(afe, MT_AFE_MEMIF_DL12, false);
	afe_data.tonegen_start = false;

	return 0;
}
#endif

int mt_afe_close(struct device *dev)
{
	dev_dbg(dev, "%s\n", __func__);

	return 0;
}

static int set_dai_fmt(struct mtk_afe *afe, unsigned int dai_id, unsigned int format)
{
	int be_idx = dai_id - MT_AFE_BACKEND_BASE;
	struct mt_afe_be_dai_data *be;

	if (dai_id < MT_AFE_BACKEND_BASE || be_idx >= MT_AFE_BACKEND_NUM) {
		return -EINVAL;
	}

	be = &afe->be_data[be_idx];
	be->fmt_mode = format;

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     probe function for struct platform_driver.
 * @param[in]
 *     pdev: platform device
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mt_afe_pcm_dev_probe(struct platform_device *pdev)
{
	int ret, i;
	unsigned int irq_id;
	struct mtk_afe *afe;
	struct resource *res;
	struct device_node *np = pdev->dev.of_node;

	dev_dbg(&pdev->dev, "%s\n", __func__);

	afe = devm_kzalloc(&pdev->dev, sizeof(*afe), GFP_KERNEL);
	if (!afe)
		return -ENOMEM;

	spin_lock_init(&afe->afe_ctrl_lock);
	mutex_init(&afe->afe_clk_mutex);

	afe->dev = &pdev->dev;

	irq_id = platform_get_irq(pdev, 0);
	if (!irq_id) {
		dev_err(afe->dev, "np %s no irq\n", np->name);
		return -ENXIO;
	}

#ifdef CONFIG_SIE_DEVELOP_BUILD
	for (i = 0; i < MT_AFE_MEMIF_NUM; i++) {
		init_waitqueue_head(&afe_data.capture_waitq[i]);
	}
	init_waitqueue_head(&afe_data.playback_waitq);
#endif

	ret = devm_request_irq(afe->dev, irq_id, mt_afe_irq_handler_capture,
			       0, "Afe_Capture_ISR_Handle", (void *)afe);
	if (ret) {
		dev_err(afe->dev, "could not request_irq\n");
		return ret;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	afe->base_addr = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(afe->base_addr))
		return PTR_ERR(afe->base_addr);

	afe->regmap = devm_regmap_init_mmio(&pdev->dev, afe->base_addr,
		&mt_afe_regmap_config);
	if (IS_ERR(afe->regmap))
		return PTR_ERR(afe->regmap);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	afe->sram_address = devm_ioremap_resource(&pdev->dev, res);
	if (!IS_ERR(afe->sram_address)) {
		afe->sram_phy_address = res->start;
		afe->sram_size = resource_size(res);
	}

	/* initial audio related clock */
	ret = mt_afe_init_audio_clk(afe);
	if (ret) {
		dev_err(afe->dev, "%s init audio clk fail\n", __func__);
		return ret;
	}

	for (i = 0; i < MT_AFE_MEMIF_NUM; i++)
		afe->memif[i].data = &memif_data[i];

	set_dai_fmt(afe, MT_AFE_IO_I2S, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS);

	platform_set_drvdata(pdev, afe);

	mt_afe_enable_main_clk(afe);

	mt_afe_init_registers(afe);

	mt_afe_disable_main_clk(afe);

	mt_afe_init_debugfs(afe);

	mt_afe_i2s_startup(afe);
	mt_afe_mphone_multi_startup(afe);
	mt_afe_dais_startup(afe, MT_AFE_MEMIF_VUL12);

	dev_info(&pdev->dev, "MTK AFE driver initialized.\n");
	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     remove function for struct platform_driver.
 * @param[in]
 *     pdev: platform device
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mt_afe_pcm_dev_remove(struct platform_device *pdev)
{
	struct mtk_afe *afe = platform_get_drvdata(pdev);

	dev_dbg(afe->dev, "%s\n", __func__);

	afe_data.enable_capture_callback = false;
	afe_data.usb_callback = NULL;

	mt_afe_dais_shutdown(afe, MT_AFE_MEMIF_VUL12);
	mt_afe_mphone_multi_shutdown(afe);
	mt_afe_i2s_shutdown(afe);

	mt_afe_cleanup_debugfs(afe);

	mt_afe_dais_hw_free(afe, MT_AFE_MEMIF_DL12);
	mt_afe_dais_hw_free(afe, MT_AFE_MEMIF_AWB);
	mt_afe_dais_hw_free(afe, MT_AFE_MEMIF_VUL12);

	return 0;
}

MODULE_DESCRIPTION("Mediatek ALSA SoC AFE platform driver");
MODULE_LICENSE("GPL v2");
