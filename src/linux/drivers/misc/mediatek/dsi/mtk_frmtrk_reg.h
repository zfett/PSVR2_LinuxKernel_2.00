/*
 * Copyright (c) 2018 MediaTek Inc.
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
 * @file mtk_frmtrk_reg.h
 * Register definition header of mtk_frmtrk.c
 */

#ifndef __TIMESTAMP_REG_H__
#define __TIMESTAMP_REG_H__

/* ----------------- Register Definitions ------------------- */
#define FRMTRACK_01					0x00000058
	#define FRMTRACK_01_SC_FRM_TRK_LINE		GENMASK(15, 0)
	#define FRMTRACK_01_SC_FRM_MASK_I		GENMASK(23, 20)
	#define FRMTRACK_01_SC_DDDS_TRK_INV		BIT(29)
#define FRMTRACK_02					0x0000005c
	#define FRMTRACK_02_SC_DDDS_TURBO_RGN		GENMASK(15, 0)
	#define FRMTRACK_02_SC_FRM_LOCK_WIN		GENMASK(23, 16)
	#define FRMTRACK_02_SC_FRM_LOCK_TOR		GENMASK(31, 24)
#define FRMTRACK_03					0x00000060
	#define FRMTRACK_03_SC_PNL_VTOTAL_DB		GENMASK(15, 0)
	#define FRMTRACK_03_SC_FRM_MASK_I_SEL		GENMASK(19, 16)
#define FRMTRACK_04					0x00000064
	#define FRMTRACK_04_SC_FRM_TRK_CROSS_VSYNC_EN	BIT(2)
	#define FRMTRACK_04_SC_FRM_TRK_DDDS_EN		BIT(5)
#define FRMTRACK_06					0x0000006c
	#define FRMTRACK_06_SC_STA_FRM_TRK_ABS_DIS	GENMASK(31, 16)

#endif /*__TIMESTAMP_REG_H__*/
