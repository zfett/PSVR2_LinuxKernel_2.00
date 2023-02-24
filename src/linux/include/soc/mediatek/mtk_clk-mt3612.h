/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#define CLK_MISC_CFG0	0x410
#define M_MOD_MASK	0x3ff
#define N_MOD_MASK	0x3ff
#define M_MOD_SHIFT	10
#define DIVEN_SHIFT	20

enum slrc_diven {
	SLRC_DIV_DIS = 0,
	SLRC_DIV_EN
};

int mtk_topckgen_slicer_div(enum slrc_diven div_en, unsigned int m_mod,
	unsigned int n_mod);
