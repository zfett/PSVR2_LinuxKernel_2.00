/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	Peng Liu <peng.liu@mediatek.com>
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
  * @file mtk_padding_reg.h
  * Register definition header of mtk_padding.c
  */

#ifndef __MMSYS_COMMON_PADDING_REG_H__
#define __MMSYS_COMMON_PADDING_REG_H__

/* ----------------- Register Definitions ------------------- */
#define PADDING_CON				0x00000000
	#define PADDING_NUMBER			GENMASK(6, 0)
	#define PADDING_BYPASS			BIT(8)
	#define PADDING_EN				BIT(9)
	#define ONE_PADDING_MODE			BIT(10)
#define PADDING_PIC_SIZE				0x00000004
	#define PIC_WIDTH				GENMASK(11, 0)
	#define PIC_HEIGHT_M1			GENMASK(25, 16)
#define PADDING_IN_CNT			0x00000008
#define PADDING_OUT_CNT			0x0000000c
#define PADDING_INT_CON			0x00000010
	#define PADDING_SW_RESET		BIT(0)
	#define ABNORMAL_EOF_INTEN		BIT(1)
	#define PADDING_DONE_INTEN		BIT(2)
	#define ABNORMAL_EOF_INTACK		BIT(3)
	#define PADDING_DONE_INTACK		BIT(4)
#define PADDING_STA				0x00000014
	#define ABNORMAL_EOF_INTSTA		BIT(0)
	#define PADDING_DONE_INTSTA		BIT(1)
	#define PADDING_BUSY			BIT(2)
#define PADDING_SHADOW			0x00000018
	#define FORCE_COMMIT			BIT(0)
	#define BYPASS_SHADOW			BIT(1)
	#define READ_WORKING			BIT(2)

#endif /*__MMSYS_COMMON_PADDING_REG_H__*/
