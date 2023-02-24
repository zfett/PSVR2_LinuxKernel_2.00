/*
 * Copyright (c) 2017 MediaTek Inc.
 * Authors:
 *	Randy Lin <randy.lin@mediatek.com>
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
 * @file mtk_dsc.h
 * Header of mtk_dsc.c
 */
#ifndef MTK_DISP_DSC_H
#define MTK_DISP_DSC_H

#include <linux/soc/mediatek/mtk-cmdq.h>

/** @ingroup IP_group_dsc_external_def
 * @{
 */
/** dsc maximum wrapper number*/
#define DSC_MAX_WRAPPER_NUM	2
/** 10 bit per channel*/
#define DSC_BPC_10_BIT		10
/** 8 bit per channel*/
#define DSC_BPC_8_BIT		8

/** 10 bit per pixel*/
#define DSC_BPP_10_BIT		10
/** 8 bit per pixel*/
#define DSC_BPP_8_BIT		8
/** 15 bit per pixel*/
#define DSC_BPP_15_BIT		15
/** 12 bit per pixel*/
#define DSC_BPP_12_BIT		12
/** No compression*/
#define DSC_NO_COMPRESS		0xf0

/** single input and single output*/
#define DSC_1_IN_1_OUT		0
/** dual input and single output*/
#define DSC_2_IN_1_OUT		1
/** dual input and dual output*/
#define DSC_2_IN_2_OUT		2

/** dsc v1.2*/
#define DSC_V1P2		2
/** dsc v1.1*/
#define DSC_V1P1		1

/** ich line clear auto */
#define DSC_ICH_LINE_CLEAR_AUTO		0
/** ich line clear at line */
#define DSC_ICH_LINE_CLEAR_AT_LINE	1
/** ich line clear at slice */
#define DSC_ICH_LINE_CLEAR_AT_SLICE	2


/**
 * @}
 */

/** @ingroup IP_group_dsc_external_struct
 * @brief Data Struct for DSC Driver
 */
struct dsc_config {
	/** to set picture formant\n
	 * support
	 * 1. 10 bit per channel\n
	 * 2. 8 bit per channel
	 */
	unsigned int pic_format;
	/** to set picture width*/
	unsigned int disp_pic_w;
	/** to set picture height*/
	unsigned int disp_pic_h;
	/** to set dsc encode slice height\n
	 * and\n
	 * slice width = disp_pic_w / (slice_mode + 1)\n
	 * where slice_mode is decided by inout_sel
	 */
	unsigned int slice_h;
	/** to set dsc core input/output selection\n
	 * support
	 * 1. single-in and single-out\n
	 * 2. dual-in and single-out\n
	 * 3. dual-in and dual-out
	 */
	unsigned int inout_sel;
	/** to set dsc encode version\n
	 * support
	 * 1. v1.1\n
	 * 2. v1.2(default)
	 */
	unsigned int version;
	/** to set ich line_clear\n
	 * 0. auto
	 * 1. will clear at line
	 * 2. will clear at slice
	 */
	unsigned int ich_line_clear;
};

int dsc_reset(const struct device *dev, struct cmdq_pkt **handle);
int dsc_bypass(const struct device *dev, struct cmdq_pkt **handle);
int dsc_relay(const struct device *dev, struct cmdq_pkt **handle);
int dsc_start(const struct device *dev, struct cmdq_pkt **handle);
int dsc_mute_color(const struct device *dev,
		    u32 color, struct cmdq_pkt **handle);
int dsc_sw_mute(const struct device *dev,
		 bool enable, struct cmdq_pkt **handle);
int dsc_hw_mute(const struct device *dev,
		 bool enable, struct cmdq_pkt **handle);
int dsc_hw_init(const struct device *dev,
		struct dsc_config config,
		const u32 *external_pps,
		struct cmdq_pkt **handle);
int dsc_power_off(struct device *dev);
int dsc_power_on(struct device *dev);

#endif
