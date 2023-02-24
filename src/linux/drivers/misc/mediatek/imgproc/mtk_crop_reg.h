/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	CK Hu <ck.hu@mediatek.com>
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
 * @file mtk_crop_reg.h
 * Register definition header of mtk_crop.c
 */

#ifndef __MTK_CROP_REG_H__
#define __MTK_CROP_REG_H__

/* ----------------- Register Definitions ------------------- */
#define DISP_CROP_EN					0x00000000
	#define CROP_EN					BIT(0)
#define DISP_CROP_RESET					0x00000004
	#define CROP_RESET				BIT(0)
#define DISP_CROP_ROI					0x00000008
	#define ROI_W					GENMASK(12, 0)
	#define ROI_H					GENMASK(28, 16)
#define DISP_CROP_SRC					0x0000000c
	#define SRC_W					GENMASK(12, 0)
	#define SRC_H					GENMASK(28, 16)
#define DISP_CROP_OFFSET				0x00000010
	#define OFF_X					GENMASK(12, 0)
	#define OFF_Y					GENMASK(28, 16)
#define DISP_CROP_MON1					0x00000018
	#define POS_X					GENMASK(12, 0)
	#define POS_Y					GENMASK(28, 16)
#define DISP_CROP_MON0					0x00000024
	#define CROP_STATE				GENMASK(5, 0)
	#define IN_VALID				BIT(16)
	#define IN_READY				BIT(17)
	#define OUT_VALID				BIT(18)
	#define OUT_READY				BIT(19)
#define DISP_CROP_DEBUG					0x00000034
	#define FORCE_COMMIT				BIT(0)
	#define BYPASS_SHADOW				BIT(1)
	#define READ_WRK_REG				BIT(2)
#define DISP_CROP_DUMMY					0x000000c0
	#define DUMMY					GENMASK(31, 0)

#endif /*__MTK_CROP_REG_H__*/
