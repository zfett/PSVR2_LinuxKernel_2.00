/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	Bibby Hsieh <ibby.hsieh@mediatek.com>
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
 * @file mtk_resizer_reg.h
 * Register definition header of mtk_resizer.c
 */

#ifndef __MTK_RESIZER_REG_H__
#define __MTK_RESIZER_REG_H__

/* ----------------- Register Definitions ------------------- */
#define RSZ_ENABLE			0x00000000
	#define RSZ_EN			BIT(0)
#define RSZ_CONTROL_1			0x00000004
	#define HORIZONTAL_EN		BIT(0)
	#define VERTICAL_EN		BIT(1)
	#define SCALING_UP		BIT(4)
	#define HORIZONTAL_ALGO		GENMASK(6, 5)
	#define VERTICAL_ALGO		GENMASK(8, 7)
	#define HORIZONTAL_TABLE	GENMASK(20, 16)
	#define VERTICAL_TABLE		GENMASK(25, 21)
	#define CONTROL1_MASK		(HORIZONTAL_EN | VERTICAL_EN | \
					SCALING_UP | HORIZONTAL_ALGO | \
					VERTICAL_ALGO | HORIZONTAL_TABLE | \
					VERTICAL_TABLE)
	#define RSZ_INTEN		GENMASK(30, 28)
	#define RSZ_FRAME_START_INT	BIT(28)
	#define RSZ_FRAME_END_INT	BIT(29)
	#define RSZ_ERR_INT		BIT(30)
	#define RSZ_INT_WCLR_EN		BIT(31)
#define RSZ_CONTROL_2			0x00000008
	#define PRESHP_EN		BIT(4)
	#define EDGE_PRESERVE_EN	BIT(7)
#define RSZ_INT_FLAG			0x0000000c
	#define RSZ_FRAME_START		BIT(0)
	#define RSZ_FRAME_END		BIT(1)
	#define RSZ_SIZE_ERR		BIT(4)
#define RSZ_INPUT_IMAGE			0x00000010
	#define UR_INPUT_IMAGE_W	GENMASK(15, 0)
	#define UR_INPUT_IMAGE_H	GENMASK(31, 16)
#define RSZ_OUTPUT_IMAGE		0x00000014
	#define UR_OUTPUT_IMAGE_W	GENMASK(15, 0)
	#define UR_OUTPUT_IMAGE_H	GENMASK(31, 16)
#define RSZ_HORI_COEFF_STEP		0x00000018
#define RSZ_VERT_COEFF_STEP		0x0000001c
	#define COEFF_STEP_MASK		GENMASK(22, 0)
#define RSZ_LUMA_HORI_INT		0x00000020
	#define INT_OFFSET_MASK		GENMASK(15, 0)
#define RSZ_LUMA_HORI_SUBPIXEL		0x00000024
	#define SUB_OFFSET_MASK		GENMASK(20, 0)
#define RSZ_LUMA_VERT_INT		0x00000028
#define RSZ_LUMA_VERT_SUBPIXEL		0x0000002c
#define RSZ_DEBUG_SEL			0x00000044
	#define VALID_READY		BIT(1)
#define RSZ_DEBUG			0x00000048
#define RSZ_TAP_ADAPT			0x0000004c
	#define EDGE_PRESERVE_SLOPE	GENMASK(3, 0)
#define RSZ_IBSE_PQ			0x00000050
	#define IBSE_PQ_MASK		GENMASK(19, 2)
#define RSZ_IBSE_GAINCONTROL_1		0x00000068
	#define PRE_SHP_GAIN		GENMASK(7, 0)
#define RSZ_OUT_BIT_CONTROL		0x00000084
	#define RSZ_CLIP_THRES		GENMASK(9, 0)
	#define RSZ_OUT_BIT_EN		BIT(16)
	#define OUT_BIT_MODE_CLIP	BIT(18)
	#define RSZ_OUT_BIT_MODE	GENMASK(18, 17)

#endif /*__MTK_RESIZER_REG_H__*/
