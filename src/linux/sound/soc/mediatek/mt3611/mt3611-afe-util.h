/**
 * @file mt3611-afe-util.h
 * Mediatek audio utility
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

#ifndef _MT3611_AFE_UTILITY_H_
#define _MT3611_AFE_UTILITY_H_

#define CLK_GET_FALSE	-1
struct mtk_afe;
struct clk;

int mt3611_afe_enable_top_cg(struct mtk_afe *afe, unsigned int cg_type);

int mt3611_afe_disable_top_cg(struct mtk_afe *afe, unsigned int cg_type);

int mt3611_afe_enable_main_clk(struct mtk_afe *afe);

int mt3611_afe_disable_main_clk(struct mtk_afe *afe);

int mt3611_afe_enable_clks(struct mtk_afe *afe,
			   struct clk *m_ck,
			   struct clk *b_ck);

int mt3611_afe_set_clks(struct mtk_afe *afe,
			struct clk *m_ck,
			unsigned int mck_rate,
			struct clk *b_ck,
			unsigned int bck_rate);

int mt3611_afe_set_parent_clk(struct mtk_afe *afe,
			      struct clk *clk,
			      struct clk *parent);

void mt3611_afe_disable_clks(struct mtk_afe *afe,
			     struct clk *m_ck,
			     struct clk *b_ck);

int mt3611_afe_enable_afe_on(struct mtk_afe *afe);

int mt3611_afe_disable_afe_on(struct mtk_afe *afe);

int mt3611_afe_enable_a1sys_associated_clk(struct mtk_afe *afe);

int mt3611_afe_disable_a1sys_associated_clk(struct mtk_afe *afe);

int mt3611_afe_enable_a2sys_associated_clk(struct mtk_afe *afe);

int mt3611_afe_disable_a2sys_associated_clk(struct mtk_afe *afe);

int mt3611_afe_enable_a3sys_associated_clk(struct mtk_afe *afe);

int mt3611_afe_disable_a3sys_associated_clk(struct mtk_afe *afe);

int mt3611_afe_enable_engen(struct mtk_afe *afe, unsigned int engen_type);

int mt3611_afe_disable_engen(struct mtk_afe *afe, unsigned int engen_type);

int mt3611_afe_enable_memif_irq_src_clk(struct mtk_afe *afe,
					int id,
					int irq_mode);

int mt3611_afe_disable_memif_irq_src_clk(struct mtk_afe *afe,
					 int id,
					 int irq_mode);

#endif
