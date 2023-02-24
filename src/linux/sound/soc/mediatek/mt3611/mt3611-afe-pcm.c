/**
 * @file mt3611-afe-pcm.c
 * Mediatek ALSA SoC AFE platform driver
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
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/pm_runtime.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include "mt3611-afe-common.h"
#include "mt3611-afe-regs.h"
#include "mt3611-afe-util.h"
#include "mt3611-afe-controls.h"
#include "mt3611-afe-debug.h"
#include "mt3611-afe-sysfs.h"

#define MT3611_DEDAULT_I2S_MCLK_MULTIPLIER 256
#define MT3611_DEDAULT_TDM_MCLK_MULTIPLIER 256
#define DPCLOCK_135M 135475000
#define DPCLOCK_147M 147456000

static const unsigned int mt3611_afe_backup_list[] = {
	AUDIO_TOP_CON0,
	AFE_CONN_MUX_CFG,
	AFE_CONN_TDMOUT,
	AFE_DAC_CON0,
	AFE_DAC_CON1,
	AFE_DL12_BASE,
	AFE_DL12_END,
	AFE_VUL12_BASE,
	AFE_VUL12_END,
	AFE_AWB_BASE,
	AFE_AWB_END,
	AFE_TDM_OUT_BASE,
	AFE_TDM_OUT_END,
	AFE_TDM_IN_BASE,
	AFE_TDM_IN_END,
	AFE_MEMIF_MSB,
};

static const struct snd_pcm_hardware mt3611_afe_hardware = {
	.info = SNDRV_PCM_INFO_MMAP |
		SNDRV_PCM_INFO_INTERLEAVED |
		SNDRV_PCM_INFO_MMAP_VALID |
		SNDRV_PCM_INFO_NO_PERIOD_WAKEUP,
	.buffer_bytes_max = 1024 * 1024,
	.period_bytes_min = 256,
	.period_bytes_max = 512 * 1024,
	.periods_min = 2,
	.periods_max = 256,
	.fifo_size = 0,
};

static const unsigned int channels_1_2_4[] = {
	1, 2, 4
};

static const unsigned int channels_1_2_4_6_8[] = {
	1, 2, 4, 6, 8
};

static const unsigned int channels_2_4_6_8[] = {
	2, 4, 6, 8
};

static const struct snd_pcm_hw_constraint_list i2s_channels = {
	.count = ARRAY_SIZE(channels_1_2_4),
	.list = channels_1_2_4,
	.mask = 0,
};

static const struct snd_pcm_hw_constraint_list tdm_channels = {
	.count = ARRAY_SIZE(channels_1_2_4_6_8),
	.list = channels_1_2_4_6_8,
	.mask = 0,
};

static const struct snd_pcm_hw_constraint_list mphone_multi_channels = {
	.count = ARRAY_SIZE(channels_2_4_6_8),
	.list = channels_2_4_6_8,
	.mask = 0,
};

#ifdef FPGA_ONLY
#include <linux/io.h>

#define TOPCKGEN_BASE		(0x10210000)
#define TOPCKGEN_CLK_AUDDIV_0	(0x0320)
#define TOPCKGEN_CLK_AUDDIV_1	(0x0324)
#define TOPCKGEN_CLK_AUDDIV_2	(0x0328)
#define TDM_CLKDIV_PDN_BITS	GENMASK(5, 2)

static unsigned int mt3611_topckgen_read_register(struct mtk_afe *afe,
						  unsigned int offset)
{
	unsigned int val = 0;
	void __iomem *addr = ((char *)afe->topckgen_base_addr + offset);

	if (afe->topckgen_base_addr)
		val = readl(addr);

	return val;
}

static void mt3611_topckgen_write_register(struct mtk_afe *afe,
					   unsigned int offset,
					   unsigned int val)
{
	void __iomem *addr = ((char *)afe->topckgen_base_addr + offset);

	if (afe->topckgen_base_addr)
		writel(val, addr);
}

static void mt3611_fpga_enable_tdm_clk_divider(struct mtk_afe *afe)
{
	unsigned int val;

	val = mt3611_topckgen_read_register(afe, TOPCKGEN_CLK_AUDDIV_0);

	val &= (~TDM_CLKDIV_PDN_BITS);

	mt3611_topckgen_write_register(afe, TOPCKGEN_CLK_AUDDIV_0, val);
}

static void mt3611_fpga_disable_tdm_clk_divider(struct mtk_afe *afe)
{
	unsigned int val;

	val = mt3611_topckgen_read_register(afe, TOPCKGEN_CLK_AUDDIV_0);

	val |= TDM_CLKDIV_PDN_BITS;

	mt3611_topckgen_write_register(afe, TOPCKGEN_CLK_AUDDIV_0, val);
}

static void mt3611_fpga_set_apll12_div0_div(struct mtk_afe *afe,
					    unsigned int divisor)
{
	unsigned int val;

	if (divisor < 1)
		divisor = 1;

	val = mt3611_topckgen_read_register(afe, TOPCKGEN_CLK_AUDDIV_1);

	val &= ~GENMASK(11, 0);
	val |= (divisor - 1);

	mt3611_topckgen_write_register(afe, TOPCKGEN_CLK_AUDDIV_1, val);
}

static void mt3611_fpga_set_apll12_div1_div(struct mtk_afe *afe,
					    unsigned int divisor)
{
	unsigned int val;

	if (divisor < 1)
		divisor = 1;

	val = mt3611_topckgen_read_register(afe, TOPCKGEN_CLK_AUDDIV_1);

	val &= ~GENMASK(23, 12);
	val |= (divisor - 1) << 12;

	mt3611_topckgen_write_register(afe, TOPCKGEN_CLK_AUDDIV_1, val);
}

static void mt3611_fpga_set_apll12_div2_div(struct mtk_afe *afe,
					    unsigned int divisor)
{
	unsigned int val;

	if (divisor < 1)
		divisor = 1;

	val = mt3611_topckgen_read_register(afe, TOPCKGEN_CLK_AUDDIV_1);

	val &= ~GENMASK(31, 24);
	val |= (divisor - 1) << 24;

	mt3611_topckgen_write_register(afe, TOPCKGEN_CLK_AUDDIV_1, val);
}

static void mt3611_fpga_set_apll12_div3_div(struct mtk_afe *afe,
					    unsigned int divisor)
{
	unsigned int val;

	if (divisor < 1)
		divisor = 1;

	val = mt3611_topckgen_read_register(afe, TOPCKGEN_CLK_AUDDIV_2);

	val &= ~GENMASK(7, 0);
	val |= (divisor - 1);

	mt3611_topckgen_write_register(afe, TOPCKGEN_CLK_AUDDIV_2, val);
}
#endif

/** @ingroup type_group_afe_InFn
 * @par Description
 *     pointer function for struct snd_pcm_ops.
 * @param[in]
 *     substream: the pcm substream
 * @return
 *     pointer position in frames
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static snd_pcm_uframes_t mt3611_afe_pcm_pointer
			 (struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt3611_afe_memif *memif = &afe->memif[rtd->cpu_dai->id];
	unsigned int hw_ptr;
	int ret;

	ret = regmap_read(afe->regmap, memif->data->reg_ofs_cur, &hw_ptr);
	if (ret || hw_ptr == 0) {
		dev_err(afe->dev, "%s hw_ptr err ret = %d\n", __func__, ret);
		hw_ptr = memif->phys_buf_addr;
	} else if (memif->use_sram) {
		/* enforce natural alignment to 8 bytes */
		hw_ptr &= ~7;
	}

	return bytes_to_frames(substream->runtime,
			       hw_ptr - memif->phys_buf_addr);
}


static const struct snd_pcm_ops mt3611_afe_pcm_ops = {
	.ioctl = snd_pcm_lib_ioctl,
	.pointer = mt3611_afe_pcm_pointer,
};

/** @ingroup type_group_afe_InFn
 * @par Description
 *     probe function for struct snd_soc_platform_driver.
 * @param
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
static int mt3611_afe_pcm_probe(struct snd_soc_platform *platform)
{
	return mt3611_afe_add_controls(platform);
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     pcm_new function for struct snd_soc_platform_driver.
 * @param[in]
 *     rtd: ASoC PCM runtime
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_card *card = rtd->card->snd_card;
	struct snd_pcm *pcm = rtd->pcm;
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	size_t size = afe->memif[rtd->cpu_dai->id].data->prealloc_size;
	struct snd_pcm_substream *substream;
	int stream;

	for (stream = 0; stream < 2; stream++) {
		substream = pcm->streams[stream].substream;
		if (substream) {
			struct snd_dma_buffer *buf = &substream->dma_buffer;

			buf->dev.type = SNDRV_DMA_TYPE_DEV;
			buf->dev.dev = card->dev;
			buf->private_data = NULL;
		}
	}

	if (size > 0)
		return snd_pcm_lib_preallocate_pages_for_all(pcm,
							     SNDRV_DMA_TYPE_DEV,
							     card->dev,
							     size, size);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     pcm_free function for struct snd_soc_platform_driver.
 * @param[in]
 *     pcm: the pcm instance
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mt3611_afe_pcm_free(struct snd_pcm *pcm)
{
	snd_pcm_lib_preallocate_free_for_all(pcm);
}

static const struct snd_soc_platform_driver mt3611_afe_pcm_platform = {
	.probe = mt3611_afe_pcm_probe,
	.pcm_new = mt3611_afe_pcm_new,
	.pcm_free = mt3611_afe_pcm_free,
	.ops = &mt3611_afe_pcm_ops,
};

struct mt3611_afe_rate {
	unsigned int rate;
	unsigned int regvalue;
};

static const struct mt3611_afe_rate mt3611_afe_i2s_rates[] = {
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

static const struct mt3611_afe_rate mt3611_afe_data_rates[] = {
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
static int mt3611_afe_i2s_fs(unsigned int sample_rate)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mt3611_afe_i2s_rates); i++)
		if (mt3611_afe_i2s_rates[i].rate == sample_rate)
			return mt3611_afe_i2s_rates[i].regvalue;

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
static int mt3611_afe_data_fs(unsigned int sample_rate)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mt3611_afe_data_rates); i++)
		if (mt3611_afe_data_rates[i].rate == sample_rate)
			return mt3611_afe_data_rates[i].regvalue;

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
static int mt3611_afe_get_memif_clk_src_mask(struct mtk_afe *afe,
					     int id,
					     unsigned int rate)
{
	struct mt3611_afe_control_data *data = &afe->ctrl_data;
	int mask = 0;

	if (id == MT3611_AFE_MEMIF_DL12) {
		if (data->dl12_clk_src == FROM_AUDIO_PLL) {
			mask = (rate % 8000) ?
				AFE_DAC_CON1_DL12_A2SYS_MASK :
				AFE_DAC_CON1_DL12_A1SYS_MASK;
#ifdef HDMI_ENABLE
		} else if ((data->dl12_clk_src == FROM_HDMIRX_PLL) ||
			   (data->dl12_clk_src == FROM_DPRX_PLL)) {
			mask = AFE_DAC_CON1_DL12_A3SYS_MASK;
		}
#else
		} else if (data->dl12_clk_src == FROM_DPRX_PLL) {
			mask = AFE_DAC_CON1_DL12_A3SYS_MASK;
		}
#endif
	} else if (id == MT3611_AFE_MEMIF_VUL12) {
		if (data->vul12_clk_src == FROM_AUDIO_PLL) {
			mask = (rate % 8000) ?
				AFE_DAC_CON0_VUL12_A2SYS_MASK :
				AFE_DAC_CON0_VUL12_A1SYS_MASK;
#ifdef HDMI_ENABLE
		} else if ((data->vul12_clk_src == FROM_HDMIRX_PLL) ||
			   (data->vul12_clk_src == FROM_DPRX_PLL)) {
			mask = AFE_DAC_CON0_VUL12_A3SYS_MASK;
		}
#else
		} else if (data->vul12_clk_src == FROM_DPRX_PLL) {
			mask = AFE_DAC_CON0_VUL12_A3SYS_MASK;
		}
#endif
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
static int mt3611_afe_get_irq_clk_src_mask(struct mtk_afe *afe,
					   int irq,
					   unsigned int rate)
{
	struct mt3611_afe_control_data *data = &afe->ctrl_data;
	int mask = 0;

	if ((irq >= MT3611_AFE_IRQ_0) && (irq < MT3611_AFE_COMMON_IRQ_NUM)) {
		unsigned int clk_src = data->common_irq_clk_src[irq];

		if (clk_src == FROM_AUDIO_PLL) {
			mask = (rate % 8000) ?
				AFE_COMMON_IRQ_A2SYS_MASK :
				AFE_COMMON_IRQ_A1SYS_MASK;
#ifdef HDMI_ENABLE
		} else if ((clk_src == FROM_HDMIRX_PLL) ||
			   (clk_src == FROM_DPRX_PLL)) {
			mask = AFE_COMMON_IRQ_A3SYS_MASK;
		}
#else
		} else if (clk_src == FROM_DPRX_PLL) {
			mask = AFE_COMMON_IRQ_A3SYS_MASK;
		}
#endif
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
static int mt3611_afe_set_i2s_out(struct mtk_afe *afe,
				  unsigned int rate,
				  int bit_width)
{
	struct mt3611_afe_control_data *data = &afe->ctrl_data;
	int be_idx = MT3611_AFE_IO_I2S - MT3611_AFE_BACKEND_BASE;
	struct mt3611_afe_be_dai_data *be = &afe->be_data[be_idx];
	int fs = mt3611_afe_i2s_fs(rate);
	unsigned int i2s_con = 0;
	unsigned int shift_con = 0;

	if (fs < 0)
		return -EINVAL;

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
#ifdef HDMI_ENABLE
	} else if ((data->backend_clk_src[be_idx] == FROM_HDMIRX_PLL) ||
		   (data->backend_clk_src[be_idx] == FROM_DPRX_PLL)) {
		i2s_con |= AFE_I2S_CON1_CLK_A3SYS;
	}
#else
	} else if (data->backend_clk_src[be_idx] == FROM_DPRX_PLL) {
		i2s_con |= AFE_I2S_CON1_CLK_A3SYS;
	}
#endif
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
static int mt3611_afe_set_i2s_in(struct mtk_afe *afe,
				 unsigned int rate,
				 int bit_width)
{
	struct mt3611_afe_control_data *data = &afe->ctrl_data;
	int be_idx = MT3611_AFE_IO_I2S - MT3611_AFE_BACKEND_BASE;
	struct mt3611_afe_be_dai_data *be = &afe->be_data[be_idx];
	int fs = mt3611_afe_i2s_fs(rate);
	unsigned int i2s_con = 0;
	unsigned int shift_con = 0;

	if (fs < 0)
		return -EINVAL;

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
#ifdef HDMI_ENABLE
	} else if ((data->backend_clk_src[be_idx] == FROM_HDMIRX_PLL) ||
		   (data->backend_clk_src[be_idx] == FROM_DPRX_PLL)) {
		i2s_con |= AFE_I2S_CON2_CLK_A3SYS;
	}
#else
	} else if (data->backend_clk_src[be_idx] == FROM_DPRX_PLL) {
		i2s_con |= AFE_I2S_CON2_CLK_A3SYS;
	}
#endif
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
static void mt3611_afe_set_i2s_enable(struct mtk_afe *afe, bool enable)
{
	unsigned int val;

	regmap_read(afe->regmap, AFE_I2S_CON1, &val);
	if (!!(val & AFE_I2S_CON1_EN) == enable)
		return;

	/* input */
	regmap_update_bits(afe->regmap, AFE_I2S_CON2, 0x1, enable);

	/* output */
	regmap_update_bits(afe->regmap, AFE_I2S_CON1, 0x1, enable);
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     get tdm fixup channel count.
 * @param[in]
 *     channels: channel count
 * @return
 *     fixup channel count
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static unsigned int mt3611_afe_tdm_ch_fixup(unsigned int channels)
{
	if (channels > 4)
		return 8;
	else if (channels > 2)
		return 4;
	else
		return 2;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     enable tdm.
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
static void mt3611_afe_set_tdm_enable(struct mtk_afe *afe, bool enable)
{
	unsigned int val;

	regmap_read(afe->regmap, AFE_TDM_CON1, &val);
	if (!!(val & AFE_TDM_CON1_EN) == enable)
		return;

	/* input */
	regmap_update_bits(afe->regmap, AFE_TDM_IN_CON1, 0x1, enable);

	/* output */
	regmap_update_bits(afe->regmap, AFE_TDM_CON1, 0x1, enable);
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
static void mt3611_afe_set_mphone_multi_enable(struct mtk_afe *afe, bool enable)
{
	unsigned int val;

	regmap_read(afe->regmap, AFE_MPHONE_MULTI_CON0, &val);
	if (!!(val & AFE_MPHONE_MULTI_CON0_EN) == enable)
		return;

	regmap_update_bits(afe->regmap, AFE_MPHONE_MULTI_CON0, 0x1, enable);
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     set tdm out configuration.
 * @param[in]
 *     substream: the pcm substream
 * @param[in]
 *     dai: the soc dai
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_tdm_out_prepare(struct snd_pcm_substream *substream,
				      struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_pcm_runtime * const runtime = substream->runtime;
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt3611_afe_be_dai_data *be =
		&afe->be_data[dai->id - MT3611_AFE_BACKEND_BASE];
	struct mt3611_afe_control_data *data = &afe->ctrl_data;
	const unsigned int channels = runtime->channels;
	const unsigned int tdm_channels = mt3611_afe_tdm_ch_fixup(channels);
	const int bit_width = snd_pcm_format_width(runtime->format);
	const int phy_width = snd_pcm_format_physical_width(runtime->format);
	unsigned int tdm_con1 = 0;
	unsigned int tdm_con2 = 0;
	unsigned int shift_con = 0;
	unsigned int bck_inverse = 0;

	tdm_con1 = AFE_TDM_CON1_LEFT_ALIGNED;
	bck_inverse = AUD_TCON1_TDMOUT_BCK_TO_PAD_INV;

	switch (be->fmt_mode & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_DSP_A:
		tdm_con1 |= AFE_TDM_CON1_1_BCK_DELAY;
		break;
	case SND_SOC_DAIFMT_I2S:
		tdm_con1 |= AFE_TDM_CON1_1_BCK_DELAY |
			    AFE_TDM_CON1_LRCK_INV;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		shift_con |= AFE_TDM_SHIFT_CON0_OUT_RJ_EN;
		shift_con |= AFE_TDM_SHIFT_CON0_OUT_SHIFT_NUM(32 - bit_width);
		break;
	default:
		break;
	}

	switch (be->fmt_mode & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_IB_IF:
		tdm_con1 ^= AFE_TDM_CON1_LRCK_INV;
		bck_inverse ^= AUD_TCON1_TDMOUT_BCK_TO_PAD_INV;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		tdm_con1 ^= AFE_TDM_CON1_LRCK_INV;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		bck_inverse ^= AUD_TCON1_TDMOUT_BCK_TO_PAD_INV;
		break;
	default:
		break;
	}

	if (data->tdm_out_mode == AFE_TDM_OUT_TO_MPHONE)
		tdm_con1 |= AFE_TDM_CON1_1_BCK_DELAY | AFE_TDM_CON1_LRCK_INV;

	/* bit width related */
	if (bit_width > 16 || data->tdm_out_mode == AFE_TDM_OUT_TO_MPHONE) {
		tdm_con1 |= AFE_TDM_CON1_WLEN_32BIT |
			    AFE_TDM_CON1_32_BCK_CYCLES;
	} else {
		tdm_con1 |= AFE_TDM_CON1_WLEN_16BIT |
			    AFE_TDM_CON1_16_BCK_CYCLES;
	}

	if (afe->tdm_slot_width > 0)
		tdm_con1 |= AFE_TDM_CON1_LRCK_WIDTH(afe->tdm_slot_width);
	else
		tdm_con1 |= AFE_TDM_CON1_LRCK_WIDTH(phy_width);

	/* channel per sdata */
	if (data->tdm_out_mode == AFE_TDM_OUT_TO_MPHONE)
		tdm_con1 |= AFE_TDM_CON1_2CH_PER_SDATA;
	else if (tdm_channels == 8)
		tdm_con1 |= AFE_TDM_CON1_8CH_PER_SDATA;
	else if (tdm_channels == 4)
		tdm_con1 |= AFE_TDM_CON1_4CH_PER_SDATA;
	else
		tdm_con1 |= AFE_TDM_CON1_2CH_PER_SDATA;

	/* set tdm2 config */
	if (data->tdm_out_mode == AFE_TDM_OUT_TO_MPHONE)
		tdm_con2 = AFE_TDM_CH_START_CH01 |
			(AFE_TDM_CH_START_CH23 << 4) |
			(AFE_TDM_CH_START_CH45 << 8) |
			(AFE_TDM_CH_START_CH67 << 12);
	else
		tdm_con2 = AFE_TDM_CH_START_CH01 |
			(AFE_TDM_CH_ZERO << 4) |
			(AFE_TDM_CH_ZERO << 8) |
			(AFE_TDM_CH_ZERO << 12);

	regmap_update_bits(afe->regmap, AFE_TDM_CON1,
			   0xff1f3f7e, tdm_con1);

	regmap_update_bits(afe->regmap, AFE_TDM_CON2,
			   AFE_TDM_CON2_SOUT_MASK, tdm_con2);

	regmap_update_bits(afe->regmap, AUDIO_TOP_CON1,
			   AUD_TCON1_TDMOUT_BCK_TO_PAD_INV, bck_inverse);

	regmap_update_bits(afe->regmap, AFE_TDM_OUT_CON0,
			   AFE_TDM_OUT_CON0_CH_MASK, channels << 4);

	regmap_update_bits(afe->regmap, AFE_TDM_SHIFT_CON0,
			   AFE_TDM_SHIFT_CON0_OUT_MASK, shift_con);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     set tdm in configuration.
 * @param[in]
 *     substream: the pcm substream
 * @param[in]
 *     dai: the soc dai
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_tdm_in_prepare(struct snd_pcm_substream *substream,
				     struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_pcm_runtime * const runtime = substream->runtime;
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt3611_afe_be_dai_data *be =
		&afe->be_data[dai->id - MT3611_AFE_BACKEND_BASE];
	const unsigned int channels = runtime->channels;
	const int bit_width = snd_pcm_format_width(runtime->format);
	const int phy_width = snd_pcm_format_physical_width(runtime->format);
	unsigned int con1 = 0;
	unsigned int con2 = 0;
	unsigned int shift_con = 0;
	unsigned int bck_inverse = 0;

	con1 = AFE_TDM_IN_CON1_SYNC_MODE_ON;
	bck_inverse = AUD_TCON1_TDMIN_BCK_TO_PAD_INV;

	switch (be->fmt_mode & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_DSP_A:
		con1 |= AFE_TDM_IN_CON1_I2S;
		break;
	case SND_SOC_DAIFMT_I2S:
		con1 |= AFE_TDM_IN_CON1_I2S |
			AFE_TDM_IN_CON1_LRCK_INV;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		shift_con |= AFE_TDM_SHIFT_CON0_IN_RJ_EN;
		shift_con |= AFE_TDM_SHIFT_CON0_IN_SHIFT_NUM(32 - bit_width);
		break;
	default:
		break;
	}

	switch (be->fmt_mode & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_IB_IF:
		con1 ^= AFE_TDM_IN_CON1_LRCK_INV;
		bck_inverse ^= AUD_TCON1_TDMIN_BCK_TO_PAD_INV;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		con1 ^= AFE_TDM_IN_CON1_LRCK_INV;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		bck_inverse ^= AUD_TCON1_TDMIN_BCK_TO_PAD_INV;
		break;
	default:
		break;
	}

	/* bit width related */
	if (bit_width > 16) {
		con1 |= AFE_TDM_IN_CON1_WLEN_32BIT |
			AFE_TDM_IN_CON1_FAST_LRCK_CYCLE_32BCK;
	} else {
		con1 |= AFE_TDM_IN_CON1_WLEN_16BIT |
			AFE_TDM_IN_CON1_FAST_LRCK_CYCLE_16BCK;
	}

	if (afe->tdm_slot_width > 0)
		con1 |= AFE_TDM_IN_CON1_LRCK_WIDTH(afe->tdm_slot_width);
	else
		con1 |= AFE_TDM_IN_CON1_LRCK_WIDTH(phy_width);

	switch (channels) {
	case 1:
		con1 |= AFE_TDM_IN_CON1_2CH_PER_SDATA;
		con2 |= AFE_TDM_IN_CON2_ODD_CFG_CH01;
		break;
	case 2:
		con1 |= AFE_TDM_IN_CON1_2CH_PER_SDATA;
		break;
	case 4:
		con1 |= AFE_TDM_IN_CON1_4CH_PER_SDATA;
		break;
	case 6:
		con1 |= AFE_TDM_IN_CON1_8CH_PER_SDATA;
		con2 |= AFE_TDM_IN_CON2_DISABLE_CH67;
		break;
	case 8:
		con1 |= AFE_TDM_IN_CON1_8CH_PER_SDATA;
		break;
	default:
		break;
	}

	regmap_update_bits(afe->regmap, AFE_TDM_IN_CON1,
			   AFE_TDM_IN_CON1_SET_MASK, con1);

	regmap_update_bits(afe->regmap, AFE_TDM_IN_CON2,
			   AFE_TDM_IN_CON2_SET_MASK, con2);

	regmap_update_bits(afe->regmap, AUDIO_TOP_CON1,
			   AUD_TCON1_TDMIN_BCK_TO_PAD_INV, bck_inverse);

	regmap_update_bits(afe->regmap, AFE_TDM_SHIFT_CON0,
			   AFE_TDM_SHIFT_CON0_IN_MASK, shift_con);

	return 0;
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
static int mt3611_afe_enable_irq(struct mtk_afe *afe,
				 struct mt3611_afe_memif *memif)
{
	int irq_mode = memif->data->irq_mode;
	unsigned long flags;

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
static int mt3611_afe_disable_irq(struct mtk_afe *afe,
				  struct mt3611_afe_memif *memif)
{
	int irq_mode = memif->data->irq_mode;
	unsigned long flags;

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
static int mt3611_afe_set_irq_delay_counter(struct mtk_afe *afe,
					    struct mt3611_afe_memif *memif,
					    int counter)
{
	int irq_mode = memif->data->irq_mode;

	switch (irq_mode) {
	case MT3611_AFE_IRQ_0:
		regmap_update_bits(afe->regmap, AFE_IRQ_DELAY_CNT0,
				   0xffff, (counter & 0xffff));
		break;
	case MT3611_AFE_IRQ_1:
		regmap_update_bits(afe->regmap, AFE_IRQ_DELAY_CNT0,
				   0xffff << 16, (counter & 0xffff) << 16);
		break;
	case MT3611_AFE_IRQ_2:
		regmap_update_bits(afe->regmap, AFE_IRQ_DELAY_CNT1,
				   0xffff, (counter & 0xffff));
		break;
	case MT3611_AFE_IRQ_3:
		regmap_update_bits(afe->regmap, AFE_IRQ_DELAY_CNT1,
				   0xffff << 16, (counter & 0xffff) << 16);
		break;
	case MT3611_AFE_IRQ_4:
		regmap_update_bits(afe->regmap, AFE_IRQ_DELAY_CNT2,
				   0xffff, (counter & 0xffff));
		break;
	case MT3611_AFE_IRQ_5:
		regmap_update_bits(afe->regmap, AFE_IRQ_DELAY_CNT2,
				   0xffff << 16, (counter & 0xffff) << 16);
		break;
	case MT3611_AFE_IRQ_6:
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
 *     back end DAI's set_sysclk function for struct snd_soc_dai_ops.
 * @param[in]
 *     dai: the soc dai
 * @param[in]
 *     clk_id: clock id
 * @param[in]
 *     freq: frequency
 * @param[in]
 *     dir: direction
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_be_set_sysclk(struct snd_soc_dai *dai,
				    int clk_id,
				    unsigned int freq,
				    int dir)
{
	struct mtk_afe *afe = snd_soc_dai_get_drvdata(dai);
	struct mt3611_afe_be_dai_data *be =
		&afe->be_data[dai->id - MT3611_AFE_BACKEND_BASE];

	be->mclk_freq = freq;

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     i2s startup function for struct snd_soc_dai_ops.
 * @param[in]
 *     substream: the pcm substream
 * @param[in]
 *     dai: the soc dai
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_i2s_startup(struct snd_pcm_substream *substream,
				  struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_pcm_runtime * const runtime = substream->runtime;
	const int be_idx = dai->id - MT3611_AFE_BACKEND_BASE;
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt3611_afe_control_data *data = &afe->ctrl_data;

	dev_dbg(afe->dev, "%s '%s'\n",
		__func__, snd_pcm_stream_str(substream));

	snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_CHANNELS,
				   &i2s_channels);

	if (dai->active)
		return 0;

	mt3611_afe_enable_main_clk(afe);

	mt3611_afe_enable_clks(afe, afe->clocks[MT3611_CLK_APLL12_DIV1], NULL);
	mt3611_afe_enable_clks(afe, afe->clocks[MT3611_CLK_APLL12_DIV0], NULL);

	if (data->backend_clk_src[be_idx] == FROM_AUDIO_PLL) {
		mt3611_afe_enable_a1sys_associated_clk(afe);
		mt3611_afe_enable_a2sys_associated_clk(afe);
#ifdef HDMI_ENABLE
	} else if (data->backend_clk_src[be_idx] == FROM_HDMIRX_PLL ||
		   data->backend_clk_src[be_idx] == FROM_DPRX_PLL) {
		mt3611_afe_enable_a3sys_associated_clk(afe);
	}
#else
	} else if (data->backend_clk_src[be_idx] == FROM_DPRX_PLL) {
		mt3611_afe_enable_a3sys_associated_clk(afe);
	}
#endif
	mt3611_afe_enable_top_cg(afe, MT3611_AFE_CG_I2S_OUT);
	mt3611_afe_enable_top_cg(afe, MT3611_AFE_CG_I2S_IN);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     i2s shutdown function for struct snd_soc_dai_ops.
 * @param[in]
 *     substream: the pcm substream
 * @param[in]
 *     dai: the soc dai
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mt3611_afe_i2s_shutdown(struct snd_pcm_substream *substream,
				    struct snd_soc_dai *dai)
{
	const int be_idx = dai->id - MT3611_AFE_BACKEND_BASE;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt3611_afe_be_dai_data *be = &afe->be_data[be_idx];
	struct mt3611_afe_control_data *data = &afe->ctrl_data;

	dev_dbg(afe->dev, "%s '%s' active(%u)\n",
		__func__, snd_pcm_stream_str(substream), dai->active);

	if (dai->active)
		return;

	if (be->prepared) {
		mt3611_afe_set_i2s_enable(afe, false);
		be->prepared = false;
	}

	mt3611_afe_disable_top_cg(afe, MT3611_AFE_CG_I2S_IN);
	mt3611_afe_disable_top_cg(afe, MT3611_AFE_CG_I2S_OUT);

	if (data->backend_clk_src[be_idx] == FROM_AUDIO_PLL) {
		mt3611_afe_disable_a1sys_associated_clk(afe);
		mt3611_afe_disable_a2sys_associated_clk(afe);

#ifdef FPGA_ONLY
		regmap_update_bits(afe->regmap, FPGA_CFG1,
				   FPGA_CFG1_A1SYS_CLK_DIV_MASK,
				   FPGA_CFG1_A1SYS_CLK_DIV(1));
		regmap_update_bits(afe->regmap, FPGA_CFG1,
				   FPGA_CFG1_A2SYS_CLK_DIV_MASK,
				   FPGA_CFG1_A2SYS_CLK_DIV(1));
#endif
#ifdef HDMI_ENABLE
	} else if (data->backend_clk_src[be_idx] == FROM_HDMIRX_PLL ||
		   data->backend_clk_src[be_idx] == FROM_DPRX_PLL) {
		mt3611_afe_disable_a3sys_associated_clk(afe);
#else
	} else if (data->backend_clk_src[be_idx] == FROM_DPRX_PLL) {
		mt3611_afe_disable_a3sys_associated_clk(afe);
#endif

#ifdef FPGA_ONLY
		regmap_update_bits(afe->regmap, FPGA_CFG1,
				   FPGA_CFG1_A3SYS_CLK_DIV_MASK,
				   FPGA_CFG1_A3SYS_CLK_DIV(1));
#endif
	}

	mt3611_afe_disable_clks(afe, afe->clocks[MT3611_CLK_APLL12_DIV1],
				NULL);
	mt3611_afe_disable_clks(afe, afe->clocks[MT3611_CLK_APLL12_DIV0],
				NULL);

	mt3611_afe_disable_main_clk(afe);
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     i2s prepare function for struct snd_soc_dai_ops.
 * @param[in]
 *     substream: the pcm substream
 * @param[in]
 *     dai: the soc dai
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_i2s_prepare(struct snd_pcm_substream *substream,
				  struct snd_soc_dai *dai)
{
	const int be_idx = dai->id - MT3611_AFE_BACKEND_BASE;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt3611_afe_be_dai_data *be = &afe->be_data[be_idx];
	struct mt3611_afe_control_data *data = &afe->ctrl_data;
	const unsigned int rate = runtime->rate;
	const int bit_width = snd_pcm_format_width(runtime->format);
	unsigned int mclk = 0;
	int ret;

	if (dai->playback_widget->power || dai->capture_widget->power) {
		dev_dbg(afe->dev, "%s '%s' widget powered(%u-%u) already\n",
			__func__, snd_pcm_stream_str(substream),
			dai->playback_widget->power,
			dai->capture_widget->power);
		return 0;
	}

	if (be->prepared) {
		if ((be->cached_rate != rate) ||
		    (be->cached_format != runtime->format)) {
			mt3611_afe_set_i2s_enable(afe, false);
			be->prepared = false;
		} else {
			dev_dbg(afe->dev, "%s '%s' prepared already\n",
				__func__, snd_pcm_stream_str(substream));
			return 0;
		}
	}

	ret = mt3611_afe_set_i2s_out(afe, rate, bit_width);
	if (ret)
		return ret;

	ret = mt3611_afe_set_i2s_in(afe, rate, bit_width);
	if (ret)
		return ret;

	if (data->backend_clk_src[be_idx] == FROM_AUDIO_PLL) {
		if (rate % 8000) {
			mt3611_afe_set_parent_clk(afe,
				afe->clocks[MT3611_CLK_TDMOUT_SEL],
				afe->clocks[MT3611_CLK_APLL2_CK]);

			mt3611_afe_set_parent_clk(afe,
				afe->clocks[MT3611_CLK_TDMIN_SEL],
				afe->clocks[MT3611_CLK_APLL2_CK]);
		} else {
			mt3611_afe_set_parent_clk(afe,
				afe->clocks[MT3611_CLK_TDMOUT_SEL],
				afe->clocks[MT3611_CLK_APLL1_CK]);

			mt3611_afe_set_parent_clk(afe,
				afe->clocks[MT3611_CLK_TDMIN_SEL],
				afe->clocks[MT3611_CLK_APLL1_CK]);
		}
#ifdef HDMI_ENABLE
	} else if (data->backend_clk_src[be_idx] == FROM_HDMIRX_PLL) {
		mt3611_afe_set_parent_clk(afe,
			afe->clocks[MT3611_CLK_TDMOUT_SEL],
			afe->clocks[MT3611_CLK_HDMIPLL_CK]);

		mt3611_afe_set_parent_clk(afe,
			afe->clocks[MT3611_CLK_A3SYS_HP_SEL],
			afe->clocks[MT3611_CLK_HDMIPLL_D6]);
#endif
	} else if (data->backend_clk_src[be_idx] == FROM_DPRX_PLL) {
		if (rate % 8000)
			mt3611_afe_set_clks(afe,
				    afe->clocks[MT3611_CLK_DPAPLL_CK],
				    DPCLOCK_135M, NULL, 0);
		else
			mt3611_afe_set_clks(afe,
				    afe->clocks[MT3611_CLK_DPAPLL_CK],
				    DPCLOCK_147M, NULL, 0);
		mt3611_afe_set_parent_clk(afe,
			afe->clocks[MT3611_CLK_TDMOUT_SEL],
			afe->clocks[MT3611_CLK_DPAPLL_CK]);

		mt3611_afe_set_parent_clk(afe,
			afe->clocks[MT3611_CLK_A3SYS_HP_SEL],
			afe->clocks[MT3611_CLK_DPAPLL_D3]);
	}

	mclk = (be->mclk_freq > 0) ? be->mclk_freq :
	       rate * MT3611_DEDAULT_I2S_MCLK_MULTIPLIER;

	mt3611_afe_set_clks(afe, afe->clocks[MT3611_CLK_APLL12_DIV1],
			    mclk, NULL, 0);
	mt3611_afe_set_clks(afe, afe->clocks[MT3611_CLK_APLL12_DIV0],
			    mclk, NULL, 0);

#ifdef FPGA_ONLY
	if ((rate * runtime->channels) > 192000) {
		struct mt3611_afe_control_data *data = &afe->ctrl_data;
		int be_idx = MT3611_AFE_IO_I2S - MT3611_AFE_BACKEND_BASE;

		if (data->backend_clk_src[be_idx] == FROM_AUDIO_PLL) {
			regmap_update_bits(afe->regmap, FPGA_CFG1,
					   FPGA_CFG1_A1SYS_CLK_DIV_MASK,
					   FPGA_CFG1_A1SYS_CLK_DIV(4));
			regmap_update_bits(afe->regmap, FPGA_CFG1,
					   FPGA_CFG1_A2SYS_CLK_DIV_MASK,
					   FPGA_CFG1_A2SYS_CLK_DIV(4));
#ifdef HDMI_ENABLE
		} else if ((data->backend_clk_src[be_idx] == FROM_HDMIRX_PLL) ||
			   (data->backend_clk_src[be_idx] == FROM_DPRX_PLL)) {
			regmap_update_bits(afe->regmap, FPGA_CFG1,
					   FPGA_CFG1_A3SYS_CLK_DIV_MASK,
					   FPGA_CFG1_A3SYS_CLK_DIV(4));
		}
#else
		} else if (data->backend_clk_src[be_idx] == FROM_DPRX_PLL) {
			regmap_update_bits(afe->regmap, FPGA_CFG1,
					   FPGA_CFG1_A3SYS_CLK_DIV_MASK,
					   FPGA_CFG1_A3SYS_CLK_DIV(4));
		}
#endif
	}
#endif

	be->prepared = true;
	be->cached_rate = rate;
	be->cached_channels = runtime->channels;
	be->cached_format = runtime->format;

	mt3611_afe_set_i2s_enable(afe, true);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     i2s trigger function for struct snd_soc_dai_ops.
 * @param[in]
 *     substream: the pcm substream
 * @param[in]
 *     cmd: triggering command
 * @param[in]
 *     dai: the soc dai
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_i2s_trigger(struct snd_pcm_substream *substream,
				  int cmd,
				  struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	const unsigned int stream = substream->stream;
	const unsigned int channels = substream->runtime->channels;

	dev_info(afe->dev, "%s '%s' cmd=%d\n", __func__,
		 snd_pcm_stream_str(substream), cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
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
			if (channels > 2)
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
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
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
	default:
		return -EINVAL;
	}

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     tdm set_tdm_slot function for struct snd_soc_dai_ops.
 * @param[in]
 *     dai: the soc dai
 * @param[in]
 *     tx_mask: mask for tx slots
 * @param[in]
 *     rx_mask: mask for rx slots
 * @param[in]
 *     slots: tdm slots
 * @param[in]
 *     slot_width: tdm slot width
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_tdm_set_tdm_slot(struct snd_soc_dai *dai,
				       unsigned int tx_mask,
				       unsigned int rx_mask,
				       int slots,
				       int slot_width)
{
	struct mtk_afe *afe = snd_soc_dai_get_drvdata(dai);

	afe->tdm_slot_width = slot_width;

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     tdm startup function for struct snd_soc_dai_ops.
 * @param[in]
 *     substream: the pcm substream
 * @param[in]
 *     dai: the soc dai
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_tdm_startup(struct snd_pcm_substream *substream,
				  struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_pcm_runtime * const runtime = substream->runtime;
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt3611_afe_control_data *data = &afe->ctrl_data;
	const int be_idx = dai->id - MT3611_AFE_BACKEND_BASE;

	snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_CHANNELS,
				   &tdm_channels);

	if (dai->active)
		return 0;

	mt3611_afe_enable_main_clk(afe);

#ifdef FPGA_ONLY
	mt3611_fpga_enable_tdm_clk_divider(afe);
	mt3611_fpga_set_apll12_div0_div(afe, 4);
	mt3611_fpga_set_apll12_div1_div(afe, 4);
#endif

	mt3611_afe_enable_clks(afe, NULL, afe->clocks[MT3611_CLK_APLL12_DIV3]);
	mt3611_afe_enable_clks(afe, NULL, afe->clocks[MT3611_CLK_APLL12_DIV2]);

	if (data->backend_clk_src[be_idx] == FROM_AUDIO_PLL) {
		mt3611_afe_enable_a1sys_associated_clk(afe);
		mt3611_afe_enable_a2sys_associated_clk(afe);
#ifdef HDMI_ENABLE
	} else if (data->backend_clk_src[be_idx] == FROM_HDMIRX_PLL ||
		   data->backend_clk_src[be_idx] == FROM_DPRX_PLL) {
		mt3611_afe_enable_a3sys_associated_clk(afe);
	}
#else
	} else if (data->backend_clk_src[be_idx] == FROM_DPRX_PLL) {
		mt3611_afe_enable_a3sys_associated_clk(afe);
	}
#endif
	mt3611_afe_enable_top_cg(afe, MT3611_AFE_CG_TDM_OUT);
	mt3611_afe_enable_top_cg(afe, MT3611_AFE_CG_TDM_IN);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     tdm shutdown function for struct snd_soc_dai_ops.
 * @param[in]
 *     substream: the pcm substream
 * @param[in]
 *     dai: the soc dai
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mt3611_afe_tdm_shutdown(struct snd_pcm_substream *substream,
				    struct snd_soc_dai *dai)
{
	const int be_idx = dai->id - MT3611_AFE_BACKEND_BASE;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt3611_afe_be_dai_data *be = &afe->be_data[be_idx];
	struct mt3611_afe_control_data *data = &afe->ctrl_data;

	dev_dbg(afe->dev, "%s '%s' active(%u)\n",
		__func__, snd_pcm_stream_str(substream), dai->active);

	if (dai->active)
		return;

	if (be->prepared) {
		mt3611_afe_set_tdm_enable(afe, false);
		be->prepared = false;
	}

	mt3611_afe_disable_top_cg(afe, MT3611_AFE_CG_TDM_IN);
	mt3611_afe_disable_top_cg(afe, MT3611_AFE_CG_TDM_OUT);

	if (data->backend_clk_src[be_idx] == FROM_AUDIO_PLL) {
		mt3611_afe_disable_a1sys_associated_clk(afe);
		mt3611_afe_disable_a2sys_associated_clk(afe);
#ifdef HDMI_ENABLE
	} else if (data->backend_clk_src[be_idx] == FROM_HDMIRX_PLL ||
		   data->backend_clk_src[be_idx] == FROM_DPRX_PLL) {
		mt3611_afe_disable_a3sys_associated_clk(afe);
	}
#else
	} else if (data->backend_clk_src[be_idx] == FROM_DPRX_PLL) {
		mt3611_afe_disable_a3sys_associated_clk(afe);
	}
#endif

#ifdef FPGA_ONLY
	mt3611_fpga_disable_tdm_clk_divider(afe);
#endif

	mt3611_afe_disable_clks(afe, NULL,
				afe->clocks[MT3611_CLK_APLL12_DIV3]);
	mt3611_afe_disable_clks(afe, NULL,
				afe->clocks[MT3611_CLK_APLL12_DIV2]);

	mt3611_afe_disable_main_clk(afe);
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     tdm prepare function for struct snd_soc_dai_ops.
 * @param[in]
 *     substream: the pcm substream
 * @param[in]
 *     dai: the soc dai
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_tdm_prepare(struct snd_pcm_substream *substream,
				  struct snd_soc_dai *dai)
{
	const int be_idx = dai->id - MT3611_AFE_BACKEND_BASE;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_pcm_runtime * const runtime = substream->runtime;
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt3611_afe_be_dai_data *be = &afe->be_data[be_idx];
	struct mt3611_afe_control_data *data = &afe->ctrl_data;
	const unsigned int rate = runtime->rate;
	const unsigned int channels = runtime->channels;
	const unsigned int tdm_channels = mt3611_afe_tdm_ch_fixup(channels);
	const int phy_width = snd_pcm_format_physical_width(runtime->format);
	unsigned int mclk = 0;
	unsigned int bck;

	if (dai->playback_widget->power || dai->capture_widget->power) {
		dev_dbg(afe->dev, "%s '%s' widget powered(%u-%u) already\n",
			__func__, snd_pcm_stream_str(substream),
			dai->playback_widget->power,
			dai->capture_widget->power);
		return 0;
	}

	if (be->prepared) {
		if ((be->cached_rate != rate) ||
		    (be->cached_channels != channels) ||
		    (be->cached_format != runtime->format)) {
			mt3611_afe_set_tdm_enable(afe, false);
			be->prepared = false;
		} else {
			dev_dbg(afe->dev, "%s '%s' prepared already\n",
				__func__, snd_pcm_stream_str(substream));
			return 0;
		}
	}

#ifdef FPGA_ONLY
	{
		struct mt3611_afe_control_data *data = &afe->ctrl_data;
		int bck_div = 0;

		bck = rate * phy_width;

		if (data->tdm_out_mode == AFE_TDM_OUT_TO_MPHONE)
			bck *= 2;
		else
			bck *= channels;

		bck_div = (6000000 + bck - 1) / bck;

		if (data->tdm_out_mode == AFE_TDM_OUT_TO_MPHONE) {
			if ((rate * channels) > 1152000)
				bck_div += 2;
			else if ((rate * channels) > 384000)
				bck_div += 1;
		}

		if ((rate * channels) > 192000)
			bck_div += 2;
		else if ((rate * channels) > 96000)
			bck_div += 1;
		else if ((rate * channels) < 32000)
			bck_div = 12;

		mt3611_fpga_set_apll12_div2_div(afe, bck_div);
		mt3611_fpga_set_apll12_div3_div(afe, bck_div);
	}

	dev_info(afe->dev, "%s AUDDIV_0(0x%x) AUDDIV_1(0x%x) AUDDIV_2(0x%x)\n",
		 __func__,
		 mt3611_topckgen_read_register(afe, TOPCKGEN_CLK_AUDDIV_0),
		 mt3611_topckgen_read_register(afe, TOPCKGEN_CLK_AUDDIV_1),
		 mt3611_topckgen_read_register(afe, TOPCKGEN_CLK_AUDDIV_2));
#endif

	if (data->backend_clk_src[be_idx] == FROM_AUDIO_PLL) {
		if (rate % 8000) {
			mt3611_afe_set_parent_clk(afe,
				afe->clocks[MT3611_CLK_TDMOUT_SEL],
				afe->clocks[MT3611_CLK_APLL2_CK]);

			mt3611_afe_set_parent_clk(afe,
				afe->clocks[MT3611_CLK_TDMIN_SEL],
				afe->clocks[MT3611_CLK_APLL2_CK]);
		} else {
			mt3611_afe_set_parent_clk(afe,
				afe->clocks[MT3611_CLK_TDMOUT_SEL],
				afe->clocks[MT3611_CLK_APLL1_CK]);

			mt3611_afe_set_parent_clk(afe,
				afe->clocks[MT3611_CLK_TDMIN_SEL],
				afe->clocks[MT3611_CLK_APLL1_CK]);
		}
#ifdef HDMI_ENABLE
	} else if (data->backend_clk_src[be_idx] == FROM_HDMIRX_PLL) {
		mt3611_afe_set_parent_clk(afe,
			afe->clocks[MT3611_CLK_TDMOUT_SEL],
			afe->clocks[MT3611_CLK_HDMIPLL_CK]);

		mt3611_afe_set_parent_clk(afe,
			afe->clocks[MT3611_CLK_A3SYS_HP_SEL],
			afe->clocks[MT3611_CLK_HDMIPLL_D6]);
#endif
	} else if (data->backend_clk_src[be_idx] == FROM_DPRX_PLL) {
		mt3611_afe_set_parent_clk(afe,
			afe->clocks[MT3611_CLK_TDMOUT_SEL],
			afe->clocks[MT3611_CLK_DPAPLL_CK]);

		mt3611_afe_set_parent_clk(afe,
			afe->clocks[MT3611_CLK_A3SYS_HP_SEL],
			afe->clocks[MT3611_CLK_DPAPLL_D3]);
	}

	mclk = (be->mclk_freq > 0) ? be->mclk_freq :
	       rate * MT3611_DEDAULT_TDM_MCLK_MULTIPLIER;

	if (data->tdm_out_mode == AFE_TDM_OUT_TO_MPHONE)
		bck = rate * 2 * phy_width;
	else
		bck = rate * tdm_channels * phy_width;

	mt3611_afe_set_clks(afe,
			    afe->clocks[MT3611_CLK_APLL12_DIV1],
			    mclk,
			    afe->clocks[MT3611_CLK_APLL12_DIV3],
			    bck);

	mt3611_afe_set_clks(afe,
			    afe->clocks[MT3611_CLK_APLL12_DIV0],
			    mclk,
			    afe->clocks[MT3611_CLK_APLL12_DIV2],
			    bck);

	mt3611_afe_tdm_out_prepare(substream, dai);

	mt3611_afe_tdm_in_prepare(substream, dai);

	be->prepared = true;
	be->cached_rate = rate;
	be->cached_channels = channels;
	be->cached_format = runtime->format;

	mt3611_afe_set_tdm_enable(afe, true);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     i2s or tdm set_fmt function for struct snd_soc_dai_ops.
 * @param[in]
 *     dai: the soc dai
 * @param[in]
 *     fmt: audio format
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_i2s_tdm_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct mtk_afe *afe = snd_soc_dai_get_drvdata(dai);
	struct mt3611_afe_be_dai_data *be =
		&afe->be_data[dai->id - MT3611_AFE_BACKEND_BASE];

	be->fmt_mode = 0;

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_DSP_A:
	case SND_SOC_DAIFMT_I2S:
	case SND_SOC_DAIFMT_LEFT_J:
	case SND_SOC_DAIFMT_RIGHT_J:
		be->fmt_mode |= fmt & SND_SOC_DAIFMT_FORMAT_MASK;
		break;
	default:
		dev_err(afe->dev, "invalid dai format %u\n", fmt);
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
	case SND_SOC_DAIFMT_NB_IF:
	case SND_SOC_DAIFMT_IB_NF:
	case SND_SOC_DAIFMT_IB_IF:
		be->fmt_mode |= fmt & SND_SOC_DAIFMT_INV_MASK;
		break;
	default:
		dev_err(afe->dev, "invalid dai format %u\n", fmt);
		return -EINVAL;
	}

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     tdm trigger function for struct snd_soc_dai_ops.
 * @param[in]
 *     substream: the pcm substream
 * @param[in]
 *     cmd: triggering command
 * @param[in]
 *     dai: the soc dai
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_tdm_trigger(struct snd_pcm_substream *substream,
				  int cmd,
				  struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	const unsigned int stream = substream->stream;

	dev_info(afe->dev, "%s '%s' cmd=%d\n", __func__,
		 snd_pcm_stream_str(substream), cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_write(afe->regmap, AFE_CONN_TDMOUT,
				     AFE_CONN_TDMOUT_O0_I0 |
				     AFE_CONN_TDMOUT_O1_I1 |
				     AFE_CONN_TDMOUT_O2_I2 |
				     AFE_CONN_TDMOUT_O3_I3 |
				     AFE_CONN_TDMOUT_O4_I4 |
				     AFE_CONN_TDMOUT_O5_I5 |
				     AFE_CONN_TDMOUT_O6_I6 |
				     AFE_CONN_TDMOUT_O7_I7);
		}
		return 0;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		return 0;
	default:
		return -EINVAL;
	}

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     mphone multi startup function for struct snd_soc_dai_ops.
 * @param[in]
 *     substream: the pcm substream
 * @param[in]
 *     dai: the soc dai
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_mphone_multi_startup(struct snd_pcm_substream *substream,
					   struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_pcm_runtime * const runtime = substream->runtime;
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);

	snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_CHANNELS,
				   &mphone_multi_channels);

	mt3611_afe_enable_main_clk(afe);

#ifdef HDMI_ENABLE
	mt3611_afe_enable_top_cg(afe, MT3611_AFE_CG_HDMI_RX);
#endif
	mt3611_afe_enable_top_cg(afe, MT3611_AFE_CG_DP_RX);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     mphone multi shutdown function for struct snd_soc_dai_ops.
 * @param[in]
 *     substream: the pcm substream
 * @param[in]
 *     dai: the soc dai
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mt3611_afe_mphone_multi_shutdown(
	struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);

#ifdef HDMI_ENABLE
	mt3611_afe_disable_top_cg(afe, MT3611_AFE_CG_HDMI_RX);
#endif
	mt3611_afe_disable_top_cg(afe, MT3611_AFE_CG_DP_RX);

	mt3611_afe_disable_main_clk(afe);
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     mphone multi prepare function for struct snd_soc_dai_ops.
 * @param[in]
 *     substream: the pcm substream
 * @param[in]
 *     dai: the soc dai
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_mphone_multi_prepare(struct snd_pcm_substream *substream,
					   struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_pcm_runtime * const runtime = substream->runtime;
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt3611_afe_control_data *data = &afe->ctrl_data;
	const unsigned int channels = runtime->channels;
	const int bit_width = snd_pcm_format_width(runtime->format);
	int be_idx = MT3611_AFE_IO_MPHONE_MULTI - MT3611_AFE_BACKEND_BASE;
	unsigned int con0 = 0;
	unsigned int con1 = 0;

	con0 |= AFE_MPHONE_MULTI_CON0_SDATA0_SEL_SDATA0 |
		AFE_MPHONE_MULTI_CON0_SDATA1_SEL_SDATA1 |
		AFE_MPHONE_MULTI_CON0_SDATA2_SEL_SDATA2 |
		AFE_MPHONE_MULTI_CON0_SDATA3_SEL_SDATA3;

	con1 |= AFE_MPHONE_MULTI_CON1_MULTI_SYNC_EN |
		AFE_MPHONE_MULTI_CON1_HBR_MODE |
		AFE_MPHONE_MULTI_CON1_DELAY_DATA |
		AFE_MPHONE_MULTI_CON1_LEFT_ALIGN |
		AFE_MPHONE_MULTI_CON1_32_LRCK_CYCLES;

#ifdef HDMI_ENABLE
	if (data->backend_clk_src[be_idx] == MPHONE_FROM_HDMIRX) {
		con0 |= AFE_MPHONE_MULTI_CON0_CLK_SRC_HDMIRX;
		con1 |= AFE_MPHONE_MULTI_CON1_DATA_SRC_HDMIRX;
	} else if (data->backend_clk_src[be_idx] == MPHONE_FROM_DPRX) {
		con0 |= AFE_MPHONE_MULTI_CON0_CLK_SRC_DPRX;
		con1 |= AFE_MPHONE_MULTI_CON1_DATA_SRC_DPRX;
#else
	if (data->backend_clk_src[be_idx] == MPHONE_FROM_DPRX) {
		con0 |= AFE_MPHONE_MULTI_CON0_CLK_SRC_DPRX;
		con1 |= AFE_MPHONE_MULTI_CON1_DATA_SRC_DPRX;
#endif
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
 *     mphone multi trigger function for struct snd_soc_dai_ops.
 * @param[in]
 *     substream: the pcm substream
 * @param[in]
 *     cmd: triggering command
 * @param[in]
 *     dai: the soc dai
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_mphone_multi_trigger(struct snd_pcm_substream *substream,
					   int cmd,
					   struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);

	dev_info(afe->dev, "%s '%s' cmd=%d\n", __func__,
		 snd_pcm_stream_str(substream), cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		mt3611_afe_set_mphone_multi_enable(afe, true);
		return 0;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		mt3611_afe_set_mphone_multi_enable(afe, false);
		return 0;
	default:
		return -EINVAL;
	}

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     front end dais startup function for struct snd_soc_dai_ops.
 * @param[in]
 *     substream: the pcm substream
 * @param[in]
 *     dai: the soc dai
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_dais_startup(struct snd_pcm_substream *substream,
				   struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mt3611_afe_memif *memif = &afe->memif[rtd->cpu_dai->id];
	const struct mt3611_afe_memif_data *data = memif->data;
	int ret;

	dev_dbg(afe->dev, "%s %s\n", __func__, memif->data->name);

	snd_soc_set_runtime_hwparams(substream, &mt3611_afe_hardware);

	snd_pcm_hw_constraint_step(runtime,
				   0,
				   SNDRV_PCM_HW_PARAM_BUFFER_BYTES,
				   16);

	ret = snd_pcm_hw_constraint_integer(runtime,
					    SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0) {
		dev_err(afe->dev, "snd_pcm_hw_constraint_integer failed\n");
		return ret;
	}

	memif->substream = substream;

	mt3611_afe_enable_main_clk(afe);
	mt3611_afe_enable_memif_irq_src_clk(afe, data->id, data->irq_mode);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     front end dais shutdown function for struct snd_soc_dai_ops.
 * @param[in]
 *     substream: the pcm substream
 * @param[in]
 *     dai: the soc dai
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mt3611_afe_dais_shutdown(struct snd_pcm_substream *substream,
				     struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt3611_afe_memif *memif = &afe->memif[rtd->cpu_dai->id];
	const struct mt3611_afe_memif_data *data = memif->data;

	dev_dbg(afe->dev, "%s %s\n", __func__, memif->data->name);

	if (memif->prepared) {
		mt3611_afe_disable_afe_on(afe);
		memif->prepared = false;
	}

	memif->substream = NULL;

	mt3611_afe_disable_memif_irq_src_clk(afe, data->id, data->irq_mode);
	mt3611_afe_disable_main_clk(afe);
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     front end dais hw_params function for struct snd_soc_dai_ops.
 * @param[in]
 *     substream: the pcm substream
 * @param[in]
 *     params: hw_params
 * @param[in]
 *     dai: the soc dai
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_dais_hw_params(struct snd_pcm_substream *substream,
				     struct snd_pcm_hw_params *params,
				     struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	const int dai_id = rtd->cpu_dai->id;
	struct mt3611_afe_memif *memif = &afe->memif[dai_id];
	const struct mt3611_afe_memif_data *data = memif->data;
	const size_t request_size = params_buffer_bytes(params);
	int ret;
	int format_val = 0;
	int format_align_val = -1;
	unsigned int rate = params_rate(params);

	dev_dbg(afe->dev,
		"%s %s period = %u rate = %u channels = %u size = %lu\n",
		__func__, data->name, params_period_size(params),
		rate, params_channels(params), request_size);

	if (request_size > data->max_sram_size) {
		ret = snd_pcm_lib_malloc_pages(substream, request_size);
		if (ret < 0) {
			dev_err(afe->dev,
				"%s %s malloc pages %zu bytes failed %d\n",
				__func__, data->name, request_size, ret);
			return ret;
		}

		memif->use_sram = false;
	} else {
		struct snd_dma_buffer *dma_buf = &substream->dma_buffer;

		dma_buf->dev.type = SNDRV_DMA_TYPE_DEV;
		dma_buf->dev.dev = substream->pcm->card->dev;
		dma_buf->area = ((unsigned char *)afe->sram_address) +
				 data->sram_offset;
		dma_buf->addr = afe->sram_phy_address + data->sram_offset;
		dma_buf->bytes = request_size;
		snd_pcm_set_runtime_buffer(substream, dma_buf);

		memif->use_sram = true;
	}

	memif->phys_buf_addr = substream->runtime->dma_addr;
	memif->buffer_size = substream->runtime->dma_bytes;

	/* start */
	regmap_write(afe->regmap, data->reg_ofs_base,
		     memif->phys_buf_addr);

	/* end */
	regmap_write(afe->regmap, data->reg_ofs_end,
		     memif->phys_buf_addr + memif->buffer_size - 1);

	/* set channel, let AWB apply fixed 2ch */
	if (data->mono_shift >= 0 && dai_id != MT3611_AFE_MEMIF_AWB) {
		unsigned int mono = (params_channels(params) == 1) ? 1 : 0;

		regmap_update_bits(afe->regmap, data->data_mono_reg,
				   1 << data->mono_shift,
				   mono << data->mono_shift);
	}

	if (data->four_ch_shift >= 0) {
		unsigned int four_ch = (params_channels(params) == 4) ? 1 : 0;

		regmap_update_bits(afe->regmap, AFE_MEMIF_PBUF_SIZE,
				   1 << data->four_ch_shift,
				   four_ch << data->four_ch_shift);
	}

	if (dai_id == MT3611_AFE_MEMIF_AWB) {
		switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_S16_LE:
		case SNDRV_PCM_FORMAT_S24_3LE:
			format_val = 2;
			format_align_val = -1;
			break;
		case SNDRV_PCM_FORMAT_S32_LE:
			format_val = 1;
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
		switch (params_format(params)) {
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
		int fs = mt3611_afe_data_fs(rate);

		if (fs < 0)
			return -EINVAL;

		fs |= mt3611_afe_get_memif_clk_src_mask(afe, dai_id, rate);

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
 *     substream: the pcm substream
 * @param[in]
 *     dai: the soc dai
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_dais_hw_free(struct snd_pcm_substream *substream,
				   struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt3611_afe_memif *memif = &afe->memif[rtd->cpu_dai->id];
	int ret = 0;

	dev_dbg(afe->dev, "%s %s\n", __func__, memif->data->name);

	if (memif->use_sram)
		snd_pcm_set_runtime_buffer(substream, NULL);
	else
		ret = snd_pcm_lib_free_pages(substream);

	return ret;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     front end dais prepare function for struct snd_soc_dai_ops.
 * @param[in]
 *     substream: the pcm substream
 * @param[in]
 *     dai: the soc dai
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_dais_prepare(struct snd_pcm_substream *substream,
				   struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt3611_afe_memif *memif = &afe->memif[rtd->cpu_dai->id];

	if (!memif->prepared) {
		mt3611_afe_enable_afe_on(afe);
		memif->prepared = true;
	}

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     front end dais trigger function for struct snd_soc_dai_ops.
 * @param[in]
 *     substream: the pcm substream
 * @param[in]
 *     cmd: triggering command
 * @param[in]
 *     dai: the soc dai
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_dais_trigger(struct snd_pcm_substream *substream,
				   int cmd,
				   struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_pcm_runtime * const runtime = substream->runtime;
	struct mtk_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt3611_afe_memif *memif = &afe->memif[rtd->cpu_dai->id];
	const struct mt3611_afe_memif_data *data = memif->data;
	unsigned int counter = runtime->period_size;
	int irq_delay_counter = data->irq_delay_cnt;

	dev_info(afe->dev, "%s %s cmd = %d\n", __func__, data->name, cmd);

	if (data->id == MT3611_AFE_MEMIF_AWB) {
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
	case SNDRV_PCM_TRIGGER_RESUME:
		/* set irq counter */
		regmap_update_bits(afe->regmap,
				   data->irq_reg_cnt,
				   0x3ffff << data->irq_cnt_shift,
				   counter << data->irq_cnt_shift);

		/* set irq fs */
		if (data->irq_fs_shift >= 0) {
			int fs = mt3611_afe_data_fs(runtime->rate);

			if (fs < 0)
				return -EINVAL;

			fs |= mt3611_afe_get_irq_clk_src_mask(afe,
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
			mt3611_afe_set_irq_delay_counter(afe, memif,
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

			mt3611_afe_enable_irq(afe, memif);

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

			mt3611_afe_enable_irq(afe, memif);
		}
		return 0;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		if (data->enable_shift >= 0)
			regmap_update_bits(afe->regmap, data->enable_reg,
					   1 << data->enable_shift, 0);

		if (!runtime->no_period_wakeup) {
			mt3611_afe_disable_irq(afe, memif);

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
	default:
		return -EINVAL;
	}
}

/* FE DAIs */
static const struct snd_soc_dai_ops mt3611_afe_dai_ops = {
	.startup	= mt3611_afe_dais_startup,
	.shutdown	= mt3611_afe_dais_shutdown,
	.hw_params	= mt3611_afe_dais_hw_params,
	.hw_free	= mt3611_afe_dais_hw_free,
	.prepare	= mt3611_afe_dais_prepare,
	.trigger	= mt3611_afe_dais_trigger,
};

/* BE DAIs */
static const struct snd_soc_dai_ops mt3611_afe_i2s_ops = {
	.set_sysclk	= mt3611_afe_be_set_sysclk,
	.startup	= mt3611_afe_i2s_startup,
	.shutdown	= mt3611_afe_i2s_shutdown,
	.prepare	= mt3611_afe_i2s_prepare,
	.trigger	= mt3611_afe_i2s_trigger,
	.set_fmt	= mt3611_afe_i2s_tdm_set_fmt,
};

static const struct snd_soc_dai_ops mt3611_afe_tdm_ops = {
	.set_sysclk	= mt3611_afe_be_set_sysclk,
	.set_tdm_slot	= mt3611_afe_tdm_set_tdm_slot,
	.startup	= mt3611_afe_tdm_startup,
	.shutdown	= mt3611_afe_tdm_shutdown,
	.prepare	= mt3611_afe_tdm_prepare,
	.trigger	= mt3611_afe_tdm_trigger,
	.set_fmt	= mt3611_afe_i2s_tdm_set_fmt,
};

static const struct snd_soc_dai_ops mt3611_afe_mphone_multi_ops = {
	.startup	= mt3611_afe_mphone_multi_startup,
	.shutdown	= mt3611_afe_mphone_multi_shutdown,
	.prepare	= mt3611_afe_mphone_multi_prepare,
	.trigger	= mt3611_afe_mphone_multi_trigger,
};

static int mt3611_afe_suspend(struct device *dev);
static int mt3611_afe_resume(struct device *dev);

/** @ingroup type_group_afe_InFn
 * @par Description
 *     front end dais suspend function for struct snd_soc_dai_driver.
 * @param[in]
 *     dai: the soc dai
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_dai_suspend(struct snd_soc_dai *dai)
{
	struct mtk_afe *afe = snd_soc_dai_get_drvdata(dai);

	dev_dbg(afe->dev, "%s id %d suspended %d\n",
		__func__, dai->id, afe->suspended);

	if (afe->suspended)
		return 0;

	return mt3611_afe_suspend(afe->dev);
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     front end dais resume function for struct snd_soc_dai_driver.
 * @param[in]
 *     dai: the soc dai
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_dai_resume(struct snd_soc_dai *dai)
{
	struct mtk_afe *afe = snd_soc_dai_get_drvdata(dai);

	dev_dbg(afe->dev, "%s id %d suspended %d\n",
		__func__, dai->id, afe->suspended);

	if (!afe->suspended)
		return 0;

	mt3611_afe_resume(afe->dev);

	return 0;
}

static struct snd_soc_dai_driver mt3611_afe_pcm_dais[] = {
	/* FE DAIs: memory intefaces to CPU */
	{
		.name = "DL",
		.id = MT3611_AFE_MEMIF_DL12,
		.suspend = mt3611_afe_dai_suspend,
		.resume = mt3611_afe_dai_resume,
		.playback = {
			.stream_name = "DL",
			.channels_min = 1,
			.channels_max = 4,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S24_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt3611_afe_dai_ops,
	}, {
		.name = "VUL",
		.id = MT3611_AFE_MEMIF_VUL12,
		.suspend = mt3611_afe_dai_suspend,
		.resume = mt3611_afe_dai_resume,
		.capture = {
			.stream_name = "VUL",
			.channels_min = 1,
			.channels_max = 4,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S24_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt3611_afe_dai_ops,
	}, {
		.name = "AWB",
		.id = MT3611_AFE_MEMIF_AWB,
		.suspend = mt3611_afe_dai_suspend,
		.resume = mt3611_afe_dai_resume,
		.capture = {
			.stream_name = "AWB",
			.channels_min = 2,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S24_LE |
				   SNDRV_PCM_FMTBIT_S24_3LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt3611_afe_dai_ops,
	}, {
		.name = "TDM_OUT",
		.id = MT3611_AFE_MEMIF_TDM_OUT,
		.suspend = mt3611_afe_dai_suspend,
		.resume = mt3611_afe_dai_resume,
		.playback = {
			.stream_name = "TDM_OUT",
			.channels_min = 1,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S24_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt3611_afe_dai_ops,
	}, {
		.name = "TDM_IN",
		.id = MT3611_AFE_MEMIF_TDM_IN,
		.suspend = mt3611_afe_dai_suspend,
		.resume = mt3611_afe_dai_resume,
		.capture = {
			.stream_name = "TDM_IN",
			.channels_min = 1,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S24_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt3611_afe_dai_ops,
	}, {
	/* BE DAIs */
		.name = "I2S",
		.id = MT3611_AFE_IO_I2S,
		.playback = {
			.stream_name = "I2S Playback",
			.channels_min = 1,
			.channels_max = 4,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S24_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.capture = {
			.stream_name = "I2S Capture",
			.channels_min = 1,
			.channels_max = 4,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S24_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt3611_afe_i2s_ops,
		.symmetric_rates = 1,
		.symmetric_samplebits = 1,
	}, {
	/* BE DAIs */
		.name = "TDM_IO",
		.id = MT3611_AFE_IO_TDM,
		.playback = {
			.stream_name = "TDM OUT Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S24_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.capture = {
			.stream_name = "TDM IN Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S24_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt3611_afe_tdm_ops,
		.symmetric_rates = 1,
		.symmetric_samplebits = 1,
	}, {
		/* BE DAIs */
		.name = "MPHONE_MULTI",
		.id = MT3611_AFE_IO_MPHONE_MULTI,
		.capture = {
			.stream_name = "MULTI LINEIN Capture",
			.channels_min = 2,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S24_LE |
				   SNDRV_PCM_FMTBIT_S24_3LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt3611_afe_mphone_multi_ops,
	},
};

/** @ingroup type_group_afe_InFn
 * @par Description
 *     handle afe clk widget event.
 * @param[in]
 *     w: dapm widget
 * @param[in]
 *     kcontrol: mixer control
 * @param[in]
 *     event: stream event
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_clk_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *kcontrol,
				int event)
{
	struct snd_soc_component *comp = snd_soc_dapm_to_component(w->dapm);
	struct mtk_afe *afe = snd_soc_component_get_drvdata(comp);

	dev_dbg(afe->dev, "%s, event %d\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		mt3611_afe_enable_main_clk(afe);
		mt3611_afe_enable_afe_on(afe);
		break;
	case SND_SOC_DAPM_POST_PMD:
		mt3611_afe_disable_afe_on(afe);
		mt3611_afe_disable_main_clk(afe);
		break;
	default:
		break;
	}

	return 0;
}

static const struct snd_soc_dapm_widget mt3611_afe_pcm_widgets[] = {
	SND_SOC_DAPM_SUPPLY_S("AFE_CLK", 1, SND_SOC_NOPM, 0, 0,
			      mt3611_afe_clk_event,
			      SND_SOC_DAPM_PRE_PMU |
			      SND_SOC_DAPM_POST_PMD),
};

static const struct snd_soc_dapm_route mt3611_afe_pcm_routes[] = {
	/* downlink */
	{"I2S Playback", NULL, "DL"},
	{"TDM OUT Playback", NULL, "TDM_OUT"},

	/* uplink */
	{"VUL", NULL, "I2S Capture"},
	{"TDM_IN", NULL, "TDM IN Capture"},
	{"AWB", NULL, "MULTI LINEIN Capture"},
};

static const struct snd_soc_component_driver mt3611_afe_pcm_dai_component = {
	.name = "mtk-afe-pcm-dai",
	.dapm_widgets = mt3611_afe_pcm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(mt3611_afe_pcm_widgets),
	.dapm_routes = mt3611_afe_pcm_routes,
	.num_dapm_routes = ARRAY_SIZE(mt3611_afe_pcm_routes),
};

#ifdef COMMON_CLOCK_FRAMEWORK_API
static const char *aud_clks[MT3611_CLK_NUM] = {
	[MT3611_CLK_AUD_INTBUS_CG] = "aud_intbus_cg",
	[MT3611_CLK_APLL1_CK] = "apll1_ck",
	[MT3611_CLK_APLL2_CK] = "apll2_ck",
#ifdef HDMI_ENABLE
	[MT3611_CLK_HDMIPLL_CK] = "hdmipll_ck",
	[MT3611_CLK_HDMIPLL_D6] = "hdmipll_d6",
#endif
	[MT3611_CLK_DPAPLL_CK] = "dpapll_ck",
	[MT3611_CLK_DPAPLL_D3] = "dpapll_d3",
	[MT3611_CLK_A1SYS_HP_CG] = "a1sys_hp_cg",
	[MT3611_CLK_A2SYS_HP_CG] = "a2sys_hp_cg",
	[MT3611_CLK_A3SYS_HP_CG] = "a3sys_hp_cg",
	[MT3611_CLK_A3SYS_HP_SEL] = "a3sys_hp_sel",
	[MT3611_CLK_TDMOUT_SEL] = "tdmout_sel",
	[MT3611_CLK_TDMIN_SEL] = "tdmin_sel",
	[MT3611_CLK_APLL12_DIV0] =  "apll12_div0",
	[MT3611_CLK_APLL12_DIV1] =  "apll12_div1",
	[MT3611_CLK_APLL12_DIV2] =  "apll12_div2",
	[MT3611_CLK_APLL12_DIV3] =  "apll12_div3",
	[MT3611_CLK_AUD_MAS_SLV_BCLK] =  "aud_mas_slv_bclk",
};
#endif

static const struct mt3611_afe_memif_data memif_data[MT3611_AFE_MEMIF_NUM] = {
	{
		.name = "DL",
		.id = MT3611_AFE_MEMIF_DL12,
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
		.irq_mode = MT3611_AFE_IRQ_0,
		.irq_fs_reg = AFE_IRQ0_MCU_CNT,
		.irq_fs_shift = 20,
		.irq_clr_shift = 0,
		.irq_delay_cnt = 0,
		.irq_delay_en_shift = 0,
		.irq_delay_src = 2,
		.irq_delay_src_shift = 8,
		.max_sram_size = 0,
		.sram_offset = 0,
		.format_reg = AFE_MEMIF_HD_MODE,
		.format_shift = 2,
		.format_mask = 0x3,
		.format_align_shift = 1,
		.prealloc_size = 128 * 1024,
	}, {
		.name = "VUL",
		.id = MT3611_AFE_MEMIF_VUL12,
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
		.irq_mode = MT3611_AFE_IRQ_1,
		.irq_fs_reg = AFE_IRQ1_MCU_CNT,
		.irq_fs_shift = 20,
		.irq_clr_shift = 1,
		.irq_delay_cnt = 0,
		.irq_delay_en_shift = 1,
		.irq_delay_src = 3,
		.irq_delay_src_shift = 12,
		.max_sram_size = 0,
		.sram_offset = 0,
		.format_reg = AFE_MEMIF_HD_MODE,
		.format_shift = 12,
		.format_mask = 0x3,
		.format_align_shift = 6,
		.prealloc_size = 32 * 1024,
	}, {
		.name = "AWB",
		.id = MT3611_AFE_MEMIF_AWB,
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
		.irq_mode = MT3611_AFE_IRQ_4,
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
		.id = MT3611_AFE_MEMIF_TDM_OUT,
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
		.irq_mode = MT3611_AFE_IRQ_6,
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
		.id = MT3611_AFE_MEMIF_TDM_IN,
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
		.irq_mode = MT3611_AFE_IRQ_5,
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

static const struct regmap_config mt3611_afe_regmap_config = {
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
static irqreturn_t mt3611_afe_irq_handler(int irq, void *dev_id)
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

	for (i = 0; i < MT3611_AFE_MEMIF_NUM; i++) {
		struct mt3611_afe_memif *memif = &afe->memif[i];
		struct snd_pcm_substream *substream = memif->substream;

		if (!substream || !(value & (1 << memif->data->irq_clr_shift)))
			continue;

		if (memif->data->enable_shift >= 0 &&
		    !((1 << memif->data->enable_shift) & memif_status))
			continue;

		snd_pcm_period_elapsed(substream);
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
static void mt3611_afe_init_registers(struct mtk_afe *afe)
{
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
 *     backup register settings while suspending.
 * @param[in]
 *     dev: device
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_suspend(struct device *dev)
{
	struct mtk_afe *afe = dev_get_drvdata(dev);
	int i;

	dev_info(dev, "%s >>\n", __func__);

	mt3611_afe_enable_main_clk(afe);

	for (i = 0; i < ARRAY_SIZE(mt3611_afe_backup_list); i++)
		regmap_read(afe->regmap, mt3611_afe_backup_list[i],
			    &afe->backup_regs[i]);

	mt3611_afe_disable_main_clk(afe);

	afe->suspended = true;

	dev_info(dev, "%s <<\n", __func__);

	return 0;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     restore register settings while resuming.
 * @param[in]
 *     dev: device
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_afe_resume(struct device *dev)
{
	struct mtk_afe *afe = dev_get_drvdata(dev);
	int i;

	dev_info(dev, "%s >>\n", __func__);

	mt3611_afe_enable_main_clk(afe);

	mt3611_afe_init_registers(afe);

	for (i = 0; i < ARRAY_SIZE(mt3611_afe_backup_list); i++)
		regmap_write(afe->regmap, mt3611_afe_backup_list[i],
			     afe->backup_regs[i]);

	mt3611_afe_disable_main_clk(afe);

	afe->suspended = false;

	dev_info(dev, "%s <<\n", __func__);

	return 0;
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
static int mt3611_afe_init_audio_clk(struct mtk_afe *afe)
{
#ifdef COMMON_CLOCK_FRAMEWORK_API
	size_t i;

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
static int mt3611_afe_pcm_dev_probe(struct platform_device *pdev)
{
	int ret, i;
	unsigned int irq_id;
	struct mtk_afe *afe;
	struct resource *res;
	struct device_node *np = pdev->dev.of_node;

	afe = devm_kzalloc(&pdev->dev, sizeof(*afe), GFP_KERNEL);
	if (!afe)
		return -ENOMEM;

	afe->backup_regs = kzalloc(ARRAY_SIZE(mt3611_afe_backup_list) *
				   sizeof(unsigned int), GFP_KERNEL);
	if (!afe->backup_regs)
		return -ENOMEM;

	spin_lock_init(&afe->afe_ctrl_lock);
	mutex_init(&afe->afe_clk_mutex);

	afe->dev = &pdev->dev;

	irq_id = platform_get_irq(pdev, 0);
	if (!irq_id) {
		dev_err(afe->dev, "np %s no irq\n", np->name);
		return -ENXIO;
	}

	ret = devm_request_irq(afe->dev, irq_id, mt3611_afe_irq_handler,
			       0, "Afe_ISR_Handle", (void *)afe);
	if (ret) {
		dev_err(afe->dev, "could not request_irq\n");
		return ret;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	afe->base_addr = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(afe->base_addr))
		return PTR_ERR(afe->base_addr);

	afe->regmap = devm_regmap_init_mmio(&pdev->dev, afe->base_addr,
		&mt3611_afe_regmap_config);
	if (IS_ERR(afe->regmap))
		return PTR_ERR(afe->regmap);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	afe->sram_address = devm_ioremap_resource(&pdev->dev, res);
	if (!IS_ERR(afe->sram_address)) {
		afe->sram_phy_address = res->start;
		afe->sram_size = resource_size(res);
	}

#ifdef FPGA_ONLY
	afe->topckgen_base_addr = ioremap_nocache(TOPCKGEN_BASE, 0x1000);
	if (!afe->topckgen_base_addr)
		dev_err(afe->dev, "%s ioremap_nocache TOPCKGEN_BASE fail\n",
			__func__);
#endif

	/* initial audio related clock */
	ret = mt3611_afe_init_audio_clk(afe);
	if (ret) {
		dev_err(afe->dev, "%s init audio clk fail\n", __func__);
		return ret;
	}

	for (i = 0; i < MT3611_AFE_MEMIF_NUM; i++)
		afe->memif[i].data = &memif_data[i];

	platform_set_drvdata(pdev, afe);

	ret = snd_soc_register_platform(&pdev->dev, &mt3611_afe_pcm_platform);
	if (ret)
		goto err_platform;

	ret = snd_soc_register_component(&pdev->dev,
					 &mt3611_afe_pcm_dai_component,
					 mt3611_afe_pcm_dais,
					 ARRAY_SIZE(mt3611_afe_pcm_dais));
	if (ret)
		goto err_component;

	mt3611_afe_enable_main_clk(afe);

	mt3611_afe_init_registers(afe);

	mt3611_afe_disable_main_clk(afe);

	mt3611_afe_init_debugfs(afe);

#ifdef DPTOI2S_SYSFS
	ret = mt3611_afe_init_sysfs(afe);
	if (ret)
		dev_err(afe->dev, "%s SYSFS fail\n", __func__);
#endif
	dev_info(&pdev->dev, "MTK AFE driver initialized.\n");
	return 0;

err_component:
	snd_soc_unregister_platform(&pdev->dev);
err_platform:
	return ret;
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
static int mt3611_afe_pcm_dev_remove(struct platform_device *pdev)
{
	struct mtk_afe *afe = platform_get_drvdata(pdev);

	mt3611_afe_cleanup_debugfs(afe);

	if (afe && afe->backup_regs)
		kfree(afe->backup_regs);

	snd_soc_unregister_component(&pdev->dev);
	snd_soc_unregister_platform(&pdev->dev);
#ifdef DPTOI2S_SYSFS
	mt3611_afe_exit_sysfs(afe);
#endif
	return 0;
}

static const struct of_device_id mt3611_afe_pcm_dt_match[] = {
	{ .compatible = "mediatek,mt3611-afe-pcm", },
	{ }
};
MODULE_DEVICE_TABLE(of, mt3611_afe_pcm_dt_match);

static struct platform_driver mt3611_afe_pcm_driver = {
	.driver = {
		   .name = "mtk-afe-pcm",
		   .of_match_table = mt3611_afe_pcm_dt_match,
	},
	.probe = mt3611_afe_pcm_dev_probe,
	.remove = mt3611_afe_pcm_dev_remove,
};

module_platform_driver(mt3611_afe_pcm_driver);

MODULE_DESCRIPTION("Mediatek ALSA SoC AFE platform driver");
MODULE_LICENSE("GPL v2");
