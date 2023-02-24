/**
 * @file mt3611-evb.c
 * MT3611 EVB ALSA SoC machine driver
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

#include <linux/module.h>
#include <linux/delay.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include "mt3611-afe-common.h"

#ifndef FPGA_ONLY
#define CODEC_I2S 1
/*#define CODEC_TDM 1*/
#endif
#define MT3611_I2S_MCLK_MULTIPLIER 256
#define MT3611_TDM_MCLK_MULTIPLIER 256

/** @ingroup type_group_afe_enum
 * @brief Pin State
 */
enum PINCTRL_PIN_STATE {
	PIN_STATE_DEFAULT = 0,
	PIN_STATE_I2S_DEFAULT,
	PIN_STATE_TDM_DEFAULT,
	PIN_STATE_MAX
};

/** @ingroup type_group_afe_struct
 * @brief Machine Data
 */
struct mt3611_evb_priv {
	struct pinctrl *pinctrl;
	struct pinctrl_state *pin_states[PIN_STATE_MAX];
};

static const char * const mt3611_evb_pinctrl_pin_str[PIN_STATE_MAX] = {
	"default", "i2s", "tdm",
};

static const struct snd_soc_dapm_widget mt3611_evb_dapm_widgets[] = {
};

static const struct snd_soc_dapm_route mt3611_evb_audio_map[] = {
};
/** @ingroup type_group_afe_InFn
 * @par Description
 *     i2s startup function for struct snd_soc_ops.
 * @param[in]
 *     substream: the pcm substream
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_evb_i2s_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct mt3611_evb_priv *card_data = snd_soc_card_get_drvdata(card);
#ifndef CONFIG_MACH_FPGA
	int ret = 0;
#endif

	if (IS_ERR(card_data->pin_states[PIN_STATE_I2S_DEFAULT]))
		return 0;

	/* default state */
#ifndef CONFIG_MACH_FPGA
	ret = pinctrl_select_state(card_data->pinctrl,
			card_data->pin_states[PIN_STATE_I2S_DEFAULT]);
	if (ret)
		dev_err(card->dev, "%s failed to select state %d\n",
			__func__, ret);

	return ret;
#else
	return 0;
#endif

}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     i2s hw_params function for struct snd_soc_ops.
 * @param[in]
 *     substream: the pcm substream
 * @param[in]
 *     params: hw_params
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_evb_i2s_hw_params(struct snd_pcm_substream *substream,
				    struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
#ifdef CODEC_I2S
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
#endif
	unsigned int mclk = 0;
	int ret;

	mclk = MT3611_I2S_MCLK_MULTIPLIER * params_rate(params);

	ret = snd_soc_dai_set_sysclk(cpu_dai, 0, mclk, SND_SOC_CLOCK_OUT);
	if (ret < 0)
		return ret;
#ifdef CODEC_I2S
	/* codec mclk */
	ret = snd_soc_dai_set_sysclk(codec_dai, 0, mclk, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;
#endif
	return 0;
}
/** @ingroup type_group_afe_InFn
 * @par Description
 *     tdm startup function for struct snd_soc_ops.
 * @param[in]
 *     substream: the pcm substream
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_evb_tdm_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct mt3611_evb_priv *card_data = snd_soc_card_get_drvdata(card);
#ifndef CONFIG_MACH_FPGA
	int ret = 0;
#endif

	if (IS_ERR(card_data->pin_states[PIN_STATE_TDM_DEFAULT]))
		return 0;

	/* default state */
#ifndef CONFIG_MACH_FPGA
	ret = pinctrl_select_state(card_data->pinctrl,
			card_data->pin_states[PIN_STATE_TDM_DEFAULT]);
	if (ret)
		dev_err(card->dev, "%s failed to select state %d\n",
			__func__, ret);

	return ret;
#else
	return 0;
#endif

}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     tdm hw_params function for struct snd_soc_ops.
 * @param[in]
 *     substream: the pcm substream
 * @param[in]
 *     params: hw_params
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_evb_tdm_hw_params(struct snd_pcm_substream *substream,
				    struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
#ifdef CODEC_TDM
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
#endif
	unsigned int mclk = 0;
	int slot = 0;
	int slot_width = 0;
	unsigned int slot_bitmask = 0;
	int ret;

	mclk = MT3611_TDM_MCLK_MULTIPLIER * params_rate(params);

	ret = snd_soc_dai_set_sysclk(cpu_dai, 0, mclk, SND_SOC_CLOCK_OUT);
	if (ret < 0)
		return ret;

#ifdef CODEC_TDM
	/* codec mclk */
	ret = snd_soc_dai_set_sysclk(codec_dai, 0, mclk, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;
#endif
	slot = params_channels(params);
	slot_width = 1;
	slot_bitmask = GENMASK(slot - 1, 0);

	ret = snd_soc_dai_set_tdm_slot(cpu_dai, slot_bitmask, slot_bitmask,
				       slot, slot_width);
	if (ret < 0)
		return ret;

	return 0;
}

static struct snd_soc_ops mt3611_evb_i2s_ops = {
	.startup = mt3611_evb_i2s_startup,
	.hw_params = mt3611_evb_i2s_hw_params,
};

static struct snd_soc_ops mt3611_evb_tdm_ops = {
	.startup = mt3611_evb_tdm_startup,
	.hw_params = mt3611_evb_tdm_hw_params,
};

/* Digital audio interface glue - connects codec <---> CPU */
static struct snd_soc_dai_link mt3611_evb_dais[] = {
	/* Front End DAI links */
	{
		.name = "DL Playback",
		.stream_name = "DL_Playback",
		.cpu_dai_name = "DL",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST
		},
		.dynamic = 1,
		.dpcm_playback = 1,
	},
	{
		.name = "VUL Capture",
		.stream_name = "VUL_Capture",
		.cpu_dai_name = "VUL",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST
		},
		.dynamic = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "TDM Playback",
		.stream_name = "TDM_Playback",
		.cpu_dai_name = "TDM_OUT",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST
		},
		.dynamic = 1,
		.dpcm_playback = 1,
	},
	{
		.name = "TDM Capture",
		.stream_name = "TDM_Capture",
		.cpu_dai_name = "TDM_IN",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST
		},
		.dynamic = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "AWB Capture",
#ifdef HDMI_ENABLE
		.stream_name = "HDMI_DP_Capture",
#else
		.stream_name = "DP_Capture",
#endif
		.cpu_dai_name = "AWB",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.trigger = {
			SND_SOC_DPCM_TRIGGER_PRE,
			SND_SOC_DPCM_TRIGGER_PRE
		},
		.dynamic = 1,
		.dpcm_capture = 1,
	},
	/* Back End DAI links */
	{
		.name = "EXT Codec",
		.cpu_dai_name = "I2S",
		.no_pcm = 1,
#ifdef CODEC_I2S
		.codec_dai_name = "cs42448",
#else
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
#endif
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
			   SND_SOC_DAIFMT_CBS_CFS,
		.ops = &mt3611_evb_i2s_ops,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "TDM BE",
		.cpu_dai_name = "TDM_IO",
		.no_pcm = 1,
#ifdef CODEC_TDM
		.codec_dai_name = "cs42448",
#else
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
#endif
		.dai_fmt = SND_SOC_DAIFMT_DSP_A | SND_SOC_DAIFMT_NB_NF |
			   SND_SOC_DAIFMT_CBS_CFS,
		.ops = &mt3611_evb_tdm_ops,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "MPHONE_MULTI BE",
		.cpu_dai_name = "MPHONE_MULTI",
		.no_pcm = 1,
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dpcm_capture = 1,
	},
};

static struct snd_soc_card mt3611_evb_card = {
	.name = "mt3611-snd-card",
	.owner = THIS_MODULE,
	.dai_link = mt3611_evb_dais,
	.num_links = ARRAY_SIZE(mt3611_evb_dais),
	.dapm_widgets = mt3611_evb_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(mt3611_evb_dapm_widgets),
	.dapm_routes = mt3611_evb_audio_map,
	.num_dapm_routes = ARRAY_SIZE(mt3611_evb_audio_map),
};
#ifndef CONFIG_MACH_FPGA

/** @ingroup type_group_afe_InFn
 * @par Description
 *     probe function for gpio configuration.
 * @param[in]
 *     card: soc sound card
 * @return
 *     0 for success, else error.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_evb_gpio_probe(struct snd_soc_card *card)
{
	struct mt3611_evb_priv *card_data = snd_soc_card_get_drvdata(card);
	int ret = 0;
	int i;

	card_data->pinctrl = devm_pinctrl_get(card->dev);
	if (IS_ERR(card_data->pinctrl)) {
		ret = PTR_ERR(card_data->pinctrl);
		dev_err(card->dev, "%s pinctrl_get failed %d\n",
			__func__, ret);
		return ret;
	}

	for (i = 0 ; i < PIN_STATE_MAX ; i++) {
		card_data->pin_states[i] =
			pinctrl_lookup_state(card_data->pinctrl,
					     mt3611_evb_pinctrl_pin_str[i]);
		if (IS_ERR(card_data->pin_states[i])) {
			ret = PTR_ERR(card_data->pin_states[i]);
			dev_warn(card->dev, "%s Can't find pin state %s %d\n",
				 __func__, mt3611_evb_pinctrl_pin_str[i], ret);
		}
	}

	if (IS_ERR(card_data->pin_states[PIN_STATE_DEFAULT]))
		return 0;

	/* default state */
	ret = pinctrl_select_state(card_data->pinctrl,
				   card_data->pin_states[PIN_STATE_DEFAULT]);
	if (ret)
		dev_err(card->dev, "%s failed to select state %d\n",
			__func__, ret);

	return ret;
}
#endif

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
static int mt3611_evb_dev_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &mt3611_evb_card;
	struct device *dev = &pdev->dev;
	struct device_node *platform_node;
#if defined(CODEC_I2S) || defined(CODEC_TDM)
	struct device_node *codec_node;
#endif
	int ret, i;
	struct mt3611_evb_priv *card_data;

	platform_node = of_parse_phandle(dev->of_node, "mediatek,platform", 0);
	if (!platform_node) {
		dev_err(dev, "Property 'platform' missing or invalid\n");
		return -EINVAL;
	}

	for (i = 0; i < card->num_links; i++) {
		if (mt3611_evb_dais[i].platform_name)
			continue;
		mt3611_evb_dais[i].platform_of_node = platform_node;
	}
#if defined(CODEC_I2S) || defined(CODEC_TDM)

	codec_node = of_parse_phandle(dev->of_node, "mediatek,audio-codec", 0);
	if (!codec_node) {
		dev_err(dev, "Property 'audio-codec' missing or invalid\n");
		return -EINVAL;
	}
	for (i = 0; i < card->num_links; i++) {
		if (mt3611_evb_dais[i].codec_name)
			continue;
		mt3611_evb_dais[i].codec_of_node = codec_node;
	}
#endif

	card->dev = dev;

	card_data = devm_kzalloc(dev, sizeof(struct mt3611_evb_priv),
				 GFP_KERNEL);
	if (!card_data) {
		ret = -ENOMEM;
		dev_err(dev, "%s allocate card private data fail %d\n",
			__func__, ret);
		return ret;
	}

	snd_soc_card_set_drvdata(card, card_data);

#ifndef CONFIG_MACH_FPGA
	mt3611_evb_gpio_probe(card);
#endif
	ret = devm_snd_soc_register_card(dev, card);
	if (ret)
		dev_err(dev, "%s snd_soc_register_card fail %d\n",
			__func__, ret);

	return ret;
}

static const struct of_device_id mt3611_evb_dt_match[] = {
	{ .compatible = "mediatek,mt3612-afe-machine", },
	{ .compatible = "mediatek,mt3611-afe-machine", },
	{ }
};
MODULE_DEVICE_TABLE(of, mt3611_evb_dt_match);

static struct platform_driver mt3611_evb_mach_driver = {
	.driver = {
		   .name = "mt3611-afe-machine",
		   .of_match_table = mt3611_evb_dt_match,
#ifdef CONFIG_PM
		   .pm = &snd_soc_pm_ops,
#endif
	},
	.probe = mt3611_evb_dev_probe,
};

module_platform_driver(mt3611_evb_mach_driver);

/* Module information */
MODULE_DESCRIPTION("MT3611 ALSA SoC machine driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:mt3611-afe-machine");

