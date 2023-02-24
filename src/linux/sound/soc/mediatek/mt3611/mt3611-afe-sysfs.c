/**
 * @file mt3611-afe-sysfs.c
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
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <sound/memalloc.h>
#include "mt3611-afe-common.h"
#include "mt3611-afe-regs.h"
#include "mt3611-afe-util.h"
#include "mt3611-afe-sysfs.h"

#ifdef DPTOI2S_SYSFS

#define USE_A3SYS
#define USE_SRAM
#define MT3611_DEDAULT_I2S_MCLK_MULTIPLIER 128
#define REQUEST_SIZE (32*1024)
#define DISABLE 0
#define ENABLE 1
#define delay_time 1000	/*delay us */
#define DIFF_SPEC 32

int sysfs_play_start;
static struct kobject *audio_fs;
static char cmd[30];
static char cmd1[30];
static unsigned int awb_dl12_diff;
struct snd_dma_buffer *dmab;
struct mtk_afe *sysfs_afe;
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
static int mt3611_afe_sysfs_fs(unsigned int sample_rate)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mt3611_afe_i2s_rates); i++)
		if (mt3611_afe_i2s_rates[i].rate == sample_rate)
			return mt3611_afe_i2s_rates[i].regvalue;

	return -EINVAL;
}
/** @ingroup type_group_afe_InFn
 * @par Description
 *     show sysfs dptoi2s_rpoint .
 * @param[in]
 *     kobj: kobject struct
 * @param[in]
 *     attr: kobj_attribute struct
 * @param[in]
 *     buf: sysfs show
 * @return
 *     ssize_t.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static ssize_t dptoi2s_rpoint_show(struct kobject *kobj,
				 struct kobj_attribute *attr,
				 char *buf)
{
	return sprintf(buf, "%s\n", &cmd1[0]);
}
/** @ingroup type_group_afe_InFn
 * @par Description
 *     read AWB/DL12 point.
 * @param[in]
 *     kobj: kobject struct
 * @param[in]
 *     attr: kobj_attribute struct
 * @param[in]
 *     buf: sysfs store info
 * @param[in]
 *     count: size_t
 * @return
 *     count
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static ssize_t dptoi2s_rpoint_store(struct kobject *kobj,
				  struct kobj_attribute *attr,
				  const char *buf, size_t count)
{
	unsigned int va0, va1, va2, va3, diff;
	unsigned int cmd_type;

	sprintf(&cmd1[0], "%s", buf);
	if (sscanf(buf, "%d ", &cmd_type) == 1)
		dev_info(sysfs_afe->dev, "Parameter ready\n");
	else
		dev_err(sysfs_afe->dev, "Parameter error\n");
	regmap_read(sysfs_afe->regmap, AFE_AWB_CUR, &va0);
	regmap_read(sysfs_afe->regmap, AFE_DL12_CUR, &va1);
	regmap_read(sysfs_afe->regmap, AFE_AWB_BASE, &va2);
	regmap_read(sysfs_afe->regmap, AFE_DL12_END, &va3);
	dev_info(sysfs_afe->dev, "AWB current0:%x\n", va0);
	dev_info(sysfs_afe->dev, "DL12 current0:%x\n", va1);
	dev_info(sysfs_afe->dev, "AWB BASE:%x\n", va2);
	dev_info(sysfs_afe->dev, "DL12 END:%x\n", va3);

	if (va0 >= va1)
		diff = va0 - va1;
	else
		diff = (va0 - va2) + (va3 - va1) + 1;
	dev_info(sysfs_afe->dev, "AWB DL12 diff:%x\n", diff);

	if (cmd_type == 0)
		awb_dl12_diff = diff;
	else {
		dev_info(sysfs_afe->dev, "awb_dl12_diff = %x\n", awb_dl12_diff);
		if (awb_dl12_diff > diff) {
			if (awb_dl12_diff - diff > DIFF_SPEC)
			dev_err(sysfs_afe->dev, "AV SYNC point check fail!\n");
			else
			dev_err(sysfs_afe->dev, "AV SYNC point check pass!\n");
		} else {
			if (diff - awb_dl12_diff > DIFF_SPEC)
			dev_err(sysfs_afe->dev, "AV SYNC point check fail!\n");
			else
			dev_err(sysfs_afe->dev, "AV SYNC point check pass!\n");
		}
	}

	return count;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     show sysfs dptoi2s_stop .
 * @param[in]
 *     kobj: kobject struct
 * @param[in]
 *     attr: kobj_attribute struct
 * @param[in]
 *     buf: sysfs show
 * @return
 *     ssize_t.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static ssize_t dptoi2s_stop_show(struct kobject *kobj,
				 struct kobj_attribute *attr,
				 char *buf)
{
	return sprintf(buf, "%s\n", &cmd[0]);
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     store dptoi2s_stop to stop dp to i2s function.
 * @param[in]
 *     kobj: kobject struct
 * @param[in]
 *     attr: kobj_attribute struct
 * @param[in]
 *     buf: sysfs store info
 * @param[in]
 *     count: size_t
 * @return
 *     count
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static ssize_t dptoi2s_stop_store(struct kobject *kobj,
				  struct kobj_attribute *attr,
				  const char *buf, size_t count)
{
	if (sysfs_play_start != 1) {
		dev_err(sysfs_afe->dev, "dptoi2s no start\n");
		return -EINVAL;
	}
	sysfs_play_start = 0;
	dev_info(sysfs_afe->dev, "dptoi2s_stop %s\n", buf);
	/* disable audio codec */
	i2s_setting(DISABLE, 0);
	/* disable I2S out,MPHONE,AWB and DL12 */
	regmap_update_bits(sysfs_afe->regmap, AFE_I2S_CON1,
			   AFE_I2S_CON1_EN, 0);
	mt3611_afe_disable_top_cg(sysfs_afe, MT3611_AFE_CG_I2S_IN);
	mt3611_afe_disable_top_cg(sysfs_afe, MT3611_AFE_CG_I2S_OUT);
	regmap_update_bits(sysfs_afe->regmap, AFE_MPHONE_MULTI_CON0,
			   AFE_MPHONE_MULTI_CON0_EN, 0);
	regmap_update_bits(sysfs_afe->regmap, AFE_DAC_CON0,
			   AFE_AFE_ON | AFE_AWB_ON | AFE_DL12_ON, 0);
	/* disable clock */
	mt3611_afe_disable_a1sys_associated_clk(sysfs_afe);
	mt3611_afe_disable_a2sys_associated_clk(sysfs_afe);
	mt3611_afe_disable_a3sys_associated_clk(sysfs_afe);
	mt3611_afe_disable_clks(sysfs_afe,
				sysfs_afe->clocks[MT3611_CLK_APLL12_DIV1],
				NULL);
	mt3611_afe_disable_clks(sysfs_afe,
				sysfs_afe->clocks[MT3611_CLK_APLL12_DIV0],
				NULL);

	mt3611_afe_disable_main_clk(sysfs_afe);
#ifndef USE_SRAM
	snd_dma_free_pages(dmab);
#endif
	return count;

}
/** @ingroup type_group_afe_InFn
 * @par Description
 *     show sysfs dptoi2s_start .
 * @param[in]
 *     kobj: kobject struct
 * @param[in]
 *     attr: kobj_attribute struct
 * @param[in]
 *     buf: sysfs show
 * @return
 *     ssize_t.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static ssize_t dptoi2s_start_show(struct kobject *kobj,
				  struct kobj_attribute *attr,
				  char *buf)
{
	return sprintf(buf, "%s\n", &cmd[0]);
}
/** @ingroup type_group_afe_InFn
 * @par Description
 *     store dptoi2s_start to start dp to i2s function.
 * @param[in]
 *     kobj: kobject struct
 * @param[in]
 *     attr: kobj_attribute struct
 * @param[in]
 *     buf: sysfs store info
 * @param[in]
 *     count: size_t
 * @return
 *     count
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static ssize_t dptoi2s_start_store(struct kobject *kobj,
				   struct kobj_attribute *attr,
				   const char *buf, size_t count)
{
	unsigned int sample_rate;
	unsigned int con0 = 0;
	unsigned int con1 = 0;
	unsigned int i2s_con = 0;
	int fs;
	unsigned int mclk = 0;

	sprintf(&cmd[0], "%s", buf);
	if (sscanf(buf, "%d ", &sample_rate) == 1)
		dev_info(sysfs_afe->dev, "Parameter ready\n");
	else
		dev_err(sysfs_afe->dev, "Parameter error\n");

	dev_info(sysfs_afe->dev, "dptoi2s_start %d\n", sample_rate);

	fs = mt3611_afe_sysfs_fs(sample_rate);
	if (fs < 0)
		return -EINVAL;
	/* enable clock and set clock DIV. */
	mt3611_afe_enable_main_clk(sysfs_afe);
	mt3611_afe_enable_top_cg(sysfs_afe, MT3611_AFE_CG_DP_RX);

	mt3611_afe_enable_clks(sysfs_afe,
			       sysfs_afe->clocks[MT3611_CLK_APLL12_DIV1],
			       NULL);
	mt3611_afe_enable_clks(sysfs_afe,
			       sysfs_afe->clocks[MT3611_CLK_APLL12_DIV0],
			       NULL);
	mt3611_afe_enable_a3sys_associated_clk(sysfs_afe);
	mt3611_afe_enable_a1sys_associated_clk(sysfs_afe);
	mt3611_afe_enable_a2sys_associated_clk(sysfs_afe);
	mt3611_afe_enable_top_cg(sysfs_afe, MT3611_AFE_CG_I2S_OUT);
	mt3611_afe_enable_top_cg(sysfs_afe, MT3611_AFE_CG_I2S_IN);
	sysfs_play_start = 1;
#ifdef USE_A3SYS
	if (sample_rate % 8000)
		mt3611_afe_set_clks(sysfs_afe,
				    sysfs_afe->clocks[MT3611_CLK_DPAPLL_CK],
				    135475000, NULL, 0);
	else
		mt3611_afe_set_clks(sysfs_afe,
				    sysfs_afe->clocks[MT3611_CLK_DPAPLL_CK],
				    147456000, NULL, 0);
	mt3611_afe_set_parent_clk(sysfs_afe,
		sysfs_afe->clocks[MT3611_CLK_TDMOUT_SEL],
		sysfs_afe->clocks[MT3611_CLK_DPAPLL_CK]);

	mt3611_afe_set_parent_clk(sysfs_afe,
		sysfs_afe->clocks[MT3611_CLK_A3SYS_HP_SEL],
		sysfs_afe->clocks[MT3611_CLK_DPAPLL_D3]);
#else
	if (sample_rate % 8000) {
		mt3611_afe_set_parent_clk(sysfs_afe,
			sysfs_afe->clocks[MT3611_CLK_TDMOUT_SEL],
			sysfs_afe->clocks[MT3611_CLK_APLL2_CK]);

		mt3611_afe_set_parent_clk(sysfs_afe,
			sysfs_afe->clocks[MT3611_CLK_TDMIN_SEL],
			sysfs_afe->clocks[MT3611_CLK_APLL2_CK]);
	} else {
		mt3611_afe_set_parent_clk(sysfs_afe,
			sysfs_afe->clocks[MT3611_CLK_TDMOUT_SEL],
			sysfs_afe->clocks[MT3611_CLK_APLL1_CK]);

		mt3611_afe_set_parent_clk(sysfs_afe,
			sysfs_afe->clocks[MT3611_CLK_TDMIN_SEL],
			sysfs_afe->clocks[MT3611_CLK_APLL1_CK]);
	}
#endif
	mclk = sample_rate * MT3611_DEDAULT_I2S_MCLK_MULTIPLIER;

	mt3611_afe_set_clks(sysfs_afe,
			    sysfs_afe->clocks[MT3611_CLK_APLL12_DIV1],
			    mclk, NULL, 0);
	mt3611_afe_set_clks(sysfs_afe,
			    sysfs_afe->clocks[MT3611_CLK_APLL12_DIV0],
			    mclk, NULL, 0);
	/* set MPHONE parameter */
	con0 |= AFE_MPHONE_MULTI_CON0_CLK_SRC_DPRX;
	con0 |= AFE_MPHONE_MULTI_CON0_SDATA0_SEL_SDATA0 |
		AFE_MPHONE_MULTI_CON0_SDATA1_SEL_SDATA1 |
		AFE_MPHONE_MULTI_CON0_SDATA2_SEL_SDATA2 |
		AFE_MPHONE_MULTI_CON0_SDATA3_SEL_SDATA3;
	con0 |= AFE_MPHONE_MULTI_CON0_24BIT_MODE;

	con1 |= AFE_MPHONE_MULTI_CON1_DATA_SRC_DPRX;
	con1 |= AFE_MPHONE_MULTI_CON1_2_CHANNEL;
	con1 |= AFE_MPHONE_MULTI_CON1_INPUT_SRC_BITS(24);
	con1 |= AFE_MPHONE_MULTI_CON1_NONCOMPACT_MODE;

	con1 |= AFE_MPHONE_MULTI_CON1_MULTI_SYNC_EN |
		AFE_MPHONE_MULTI_CON1_HBR_MODE |
		AFE_MPHONE_MULTI_CON1_DELAY_DATA |
		AFE_MPHONE_MULTI_CON1_LEFT_ALIGN |
		AFE_MPHONE_MULTI_CON1_32_LRCK_CYCLES;

	regmap_update_bits(sysfs_afe->regmap, AFE_MPHONE_MULTI_CON0,
			   AFE_MPHONE_MULTI_CON0_SET_MASK, con0);
	regmap_update_bits(sysfs_afe->regmap, AFE_MPHONE_MULTI_CON1,
			   AFE_MPHONE_MULTI_CON1_SET_MASK, con1);

#ifdef USE_SRAM
	/* set AWB memif */
	/* start */
	regmap_write(sysfs_afe->regmap, AFE_AWB_BASE,
		     sysfs_afe->sram_phy_address);

	/* end */
	regmap_write(sysfs_afe->regmap, AFE_AWB_END,
		     sysfs_afe->sram_phy_address + sysfs_afe->sram_size - 1);

	regmap_update_bits(sysfs_afe->regmap, AFE_MEMIF_HD_MODE,
			   AFE_AWB_HD_MASK,
			   AFE_32_BIT_FORMAT << 8);
	regmap_update_bits(sysfs_afe->regmap, AFE_MEMIF_HDALIGN,
			   AFE_AWB_HD_ALIGN, 1 << 4);

	/* set DL12 memif */
	/* start */
	regmap_write(sysfs_afe->regmap, AFE_DL12_BASE,
		     sysfs_afe->sram_phy_address);

	/* end */
	regmap_write(sysfs_afe->regmap, AFE_DL12_END,
		     sysfs_afe->sram_phy_address + sysfs_afe->sram_size - 1);

#else

	dmab->dev.dev = sysfs_afe->dev;
	dmab->dev.type = SNDRV_DMA_TYPE_DEV;
	snd_dma_alloc_pages(SNDRV_DMA_TYPE_DEV, sysfs_afe->dev,
			    REQUEST_SIZE, dmab);
	dev_info(sysfs_afe->dev, "address %llu\n", dmab->addr);

	/* start */
	regmap_write(sysfs_afe->regmap, AFE_AWB_BASE,
		     dmab->addr);

	/* end */
	regmap_write(sysfs_afe->regmap, AFE_AWB_END,
		     dmab->addr + REQUEST_SIZE - 1);

	regmap_update_bits(sysfs_afe->regmap, AFE_MEMIF_HD_MODE,
			   AFE_AWB_HD_MASK,
			   AFE_32_BIT_FORMAT << 8);
	regmap_update_bits(sysfs_afe->regmap, AFE_MEMIF_HDALIGN,
			   AFE_AWB_HD_ALIGN, 1 << 4);

	/* start */
	regmap_write(sysfs_afe->regmap, AFE_DL12_BASE,
		     dmab->addr);

	/* end */
	regmap_write(sysfs_afe->regmap, AFE_DL12_END,
		     dmab->addr + REQUEST_SIZE - 1);

#endif

	regmap_update_bits(sysfs_afe->regmap, AFE_MEMIF_HD_MODE,
			   AFE_DL12_HD_MASK,
			   AFE_32_BIT_FORMAT << 2);
	regmap_update_bits(sysfs_afe->regmap, AFE_MEMIF_HDALIGN,
			   AFE_AWB_HD_ALIGN, 1 << 1);

	/* set I2S DL parameter */
	regmap_update_bits(sysfs_afe->regmap, AFE_DAC_CON1,
			   AFE_DL12_MODE_MASK,
			   (AFE_DAC_CON1_DL12_A3SYS_MASK | fs) << 0);

	i2s_con = AFE_I2S_CON1_RATE(fs);
	i2s_con |= AFE_I2S_CON1_FORMAT_I2S;
#ifdef USE_A3SYS
	i2s_con |= AFE_I2S_CON1_CLK_A3SYS;
#else
	if (sample_rate % 8000)
		i2s_con |= AFE_I2S_CON1_CLK_A2SYS;
	else
		i2s_con |= AFE_I2S_CON1_CLK_A1SYS;
#endif
	i2s_con |= AFE_I2S_CON1_WLEN_32BIT;
	regmap_update_bits(sysfs_afe->regmap, AFE_I2S_CON1,
			   ~(u32)AFE_I2S_CON1_EN, i2s_con);
	regmap_update_bits(sysfs_afe->regmap, AFE_CONN_MUX_CFG,
			   AFE_CONN_MUX_O0_MASK |
			   AFE_CONN_MUX_O1_MASK |
			   AFE_CONN_MUX_O2_MASK |
			   AFE_CONN_MUX_O3_MASK,
			   AFE_CONN_MUX_O0_I0 |
			   AFE_CONN_MUX_O1_I1 |
			   AFE_CONN_MUX_O2_I2 |
			   AFE_CONN_MUX_O3_I3);

	/* enable MPHONE and AWB */
	regmap_update_bits(sysfs_afe->regmap, AFE_MPHONE_MULTI_CON0,
			   AFE_MPHONE_MULTI_CON0_EN, AFE_MPHONE_MULTI_CON0_EN);
	regmap_update_bits(sysfs_afe->regmap, AFE_DAC_CON0,
			   AFE_AWB_ON, AFE_AWB_ON);
	regmap_update_bits(sysfs_afe->regmap, AFE_DAC_CON0,
			   AFE_AFE_ON, AFE_AFE_ON);

	usleep_range(delay_time, (delay_time * 2));

	/* enable I2S out and DL12 */
	regmap_update_bits(sysfs_afe->regmap, AFE_DAC_CON0,
			   AFE_DL12_ON, AFE_DL12_ON);
	regmap_update_bits(sysfs_afe->regmap, AFE_I2S_CON1,
			   AFE_I2S_CON1_EN, AFE_I2S_CON1_EN);
	/* enable audio codec */
	i2s_setting(ENABLE, mclk);

	return count;
}

static struct kobj_attribute dptoi2s_start_attribute =
	__ATTR(dptoi2s_start, 0664, dptoi2s_start_show, dptoi2s_start_store);
static struct kobj_attribute dptoi2s_stop_attribute =
	__ATTR(dptoi2s_stop, 0664, dptoi2s_stop_show, dptoi2s_stop_store);
static struct kobj_attribute dptoi2s_rpoint_attribute =
	__ATTR(dptoi2s_rpoint, 0664, dptoi2s_rpoint_show, dptoi2s_rpoint_store);

static struct attribute *attrs[] = {
				    &dptoi2s_start_attribute.attr,
				    &dptoi2s_stop_attribute.attr,
				    &dptoi2s_rpoint_attribute.attr,
				    NULL,
				    };
/* need to NULL terminate the list of attributes */
static struct attribute_group attr_group = {
					    .attrs = attrs,
};
#endif

/** @ingroup type_group_afe_InFn
 * @par Description
 *     init and create sysfs.
 * @param[in]
 *     afe: mtk_afe struct
 * @return
 *     0 or -ENOMEM
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mt3611_afe_init_sysfs(struct mtk_afe *afe)
{
#ifdef DPTOI2S_SYSFS
	int retval;

	sysfs_afe = afe;
	audio_fs = kobject_create_and_add("audio_fs", kernel_kobj);
	if (!audio_fs)
		return -ENOMEM;
	/* Create the files associated with this kobject */
	retval = sysfs_create_group(audio_fs, &attr_group);
	if (retval)
		kobject_put(audio_fs);
#ifndef USE_SRAM
	dmab = devm_kzalloc(sysfs_afe->dev, sizeof(*dmab), GFP_KERNEL);
	if (!dmab)
		return -ENOMEM;
#endif
#endif
	return 0;
}

int mt3611_afe_exit_sysfs(struct mtk_afe *afe)
{
#ifdef DPTOI2S_SYSFS
	sysfs_afe = afe;
	kobject_del(audio_fs);
	kobject_put(audio_fs);
#ifndef USE_SRAM
	devm_kfree(sysfs_afe->dev, dmab);
#endif
#endif
	return 0;
}

