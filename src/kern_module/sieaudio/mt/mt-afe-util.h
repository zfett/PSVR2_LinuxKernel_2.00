/**
 * @file mt-afe-util.h
 * Mediatek audio utility
 *
 * Copyright (c) 2017 MediaTek Inc.
 *
 * Copyright (C) 2021 Sony Interactive Entertainment Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _MT_AFE_UTILITY_H_
#define _MT_AFE_UTILITY_H_

#define CLK_GET_FALSE	-1
struct mtk_afe;
struct clk;

int mt_afe_enable_top_cg(struct mtk_afe *afe, unsigned int cg_type);

int mt_afe_disable_top_cg(struct mtk_afe *afe, unsigned int cg_type);

int mt_afe_enable_main_clk(struct mtk_afe *afe);

int mt_afe_disable_main_clk(struct mtk_afe *afe);

int mt_afe_enable_clks(struct mtk_afe *afe,
		       struct clk *m_ck,
		       struct clk *b_ck);

int mt_afe_set_clks(struct mtk_afe *afe,
		    struct clk *m_ck,
		    unsigned int mck_rate,
		    struct clk *b_ck,
		    unsigned int bck_rate);

int mt_afe_set_parent_clk(struct mtk_afe *afe,
			  struct clk *clk,
			  struct clk *parent);

void mt_afe_disable_clks(struct mtk_afe *afe,
			 struct clk *m_ck,
			 struct clk *b_ck);

int mt_afe_enable_afe_on(struct mtk_afe *afe);

int mt_afe_disable_afe_on(struct mtk_afe *afe);

int mt_afe_enable_a1sys_associated_clk(struct mtk_afe *afe);

int mt_afe_disable_a1sys_associated_clk(struct mtk_afe *afe);

int mt_afe_enable_a2sys_associated_clk(struct mtk_afe *afe);

int mt_afe_disable_a2sys_associated_clk(struct mtk_afe *afe);

int mt_afe_enable_a3sys_associated_clk(struct mtk_afe *afe);

int mt_afe_disable_a3sys_associated_clk(struct mtk_afe *afe);

int mt_afe_enable_memif_irq_src_clk(struct mtk_afe *afe,
				    int id,
				    int irq_mode);

int mt_afe_disable_memif_irq_src_clk(struct mtk_afe *afe,
				     int id,
				     int irq_mode);

#endif
