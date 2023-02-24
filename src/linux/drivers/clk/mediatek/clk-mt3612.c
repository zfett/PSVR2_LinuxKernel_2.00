/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Shunli Wang <shunli.wang@mediatek.com>
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

#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <soc/mediatek/mtk_clk-mt3612.h>

#include "clk-mtk.h"
#include "clk-gate.h"

#include <dt-bindings/clock/mt3612-clk.h>
#include <dt-bindings/reset-controller/mt3612-resets.h>

static DEFINE_SPINLOCK(lock);

static void __init mtk_toprgu_init(struct device_node *node)
{
	mtk_register_reset_controller(node, 3, MT3612_TOPRGU_OFS);
}
CLK_OF_DECLARE(mtk_toprgu, "mediatek,mt3612-toprgu", mtk_toprgu_init);


#define MT3612_PLL_FMAX	(1800 * MHZ)
#define CON0_MT3612_RST_BAR	BIT(24)

#define PLL(_id, _name, _reg, _pwr_reg, _en_mask, _flags, _pcwbits, _pd_reg, \
			_pd_shift, _tuner_reg, _pcw_reg, _pcw_shift) {	\
		.id = _id,						\
		.name = _name,						\
		.reg = _reg,						\
		.pwr_reg = _pwr_reg,					\
		.en_mask = _en_mask,					\
		.flags = _flags,					\
		.rst_bar_mask = CON0_MT3612_RST_BAR,			\
		.fmax = MT3612_PLL_FMAX,				\
		.pcwbits = _pcwbits,					\
		.pd_reg = _pd_reg,					\
		.pd_shift = _pd_shift,					\
		.tuner_reg = _tuner_reg,				\
		.pcw_reg = _pcw_reg,					\
		.pcw_shift = _pcw_shift,				\
	}

static const struct mtk_pll_data apmixed_plls[] = {
	PLL(CLK_APMIXED_ARMPLL, "armpll", 0x310, 0x31C, 0xFC555581,
	   HAVE_RST_BAR, 22, 0x314, 28, 0x0, 0x314, 0),
	PLL(CLK_APMIXED_VPUPLL, "vpupll", 0x270, 0x27C, 0xFC000101,
	   HAVE_RST_BAR, 22, 0x274, 28, 0x0, 0x274, 0),
	PLL(CLK_APMIXED_GPUPLL, "gpupll", 0x3D0, 0x3DC, 0xFC000101,
	   HAVE_RST_BAR, 22, 0x3D4, 28, 0x0, 0x3D4, 0),
	PLL(CLK_APMIXED_DPVPLL, "dpvpll", 0x390, 0x39C, 0xFC000101,
	   HAVE_RST_BAR, 22, 0x394, 28, 0x0, 0x394, 0),
	PLL(CLK_APMIXED_DPAPLL, "dpapll", 0x3A0, 0x3AC, 0xFC000101,
	   HAVE_RST_BAR, 22, 0x3A4, 28, 0x0, 0x3A4, 0),
	PLL(CLK_APMIXED_MAINPLL, "mainpll", 0x230, 0x23c, 0xFC000101,
	   HAVE_RST_BAR, 22, 0x234, 28, 0x0, 0x234, 0),
	PLL(CLK_APMIXED_MMPLL, "mmpll", 0x260, 0x26c, 0xFC000101,
	   HAVE_RST_BAR, 22, 0x264, 28, 0x0, 0x264, 0),
	PLL(CLK_APMIXED_UNIVPLL, "univpll", 0x240, 0x24c, 0xFC000101,
	   HAVE_RST_BAR, 22, 0x244, 28, 0x0, 0x244, 0),
	PLL(CLK_APMIXED_EMIPLL, "emipll", 0x290, 0x29c, 0xFC000101,
	   HAVE_RST_BAR, 22, 0x294, 28, 0x0, 0x294, 0),
	PLL(CLK_APMIXED_APLL1, "apll1", 0x2A0, 0x2B8, 0xFC000101,
	   HAVE_RST_BAR, 32, 0x2A4, 28, 0x0, 0x2A8, 0),
	PLL(CLK_APMIXED_APLL2, "apll2", 0x2C0, 0x2D8, 0xFC000101,
	   HAVE_RST_BAR, 32, 0x2C4, 28, 0x0, 0x2C8, 0),
	PLL(CLK_APMIXED_AP27PLL, "ap27pll", 0x2F0, 0x304, 0xFC000101,
	   HAVE_RST_BAR, 32, 0x2F4, 28, 0x0, 0x2F8, 0),
	PLL(CLK_APMIXED_MPLL, "mpll", 0x220, 0x22C, 0xFC000101,
	   HAVE_RST_BAR, 22, 0x224, 28, 0x0, 0x224, 0),
	PLL(CLK_APMIXED_MSDCPLL, "msdcpll", 0x250, 0x25C, 0xFC000101,
	   HAVE_RST_BAR, 22, 0x254, 28, 0x0, 0x254, 0),
};

static void __init mtk_apmixedsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_APMIXED_NR);
	if (!clk_data)
		return;

	mtk_clk_register_plls(node, apmixed_plls, ARRAY_SIZE(apmixed_plls),
								clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}
CLK_OF_DECLARE(mtk_apmixedsys, "mediatek,mt3612-apmixedsys",
	      mtk_apmixedsys_init);

static const struct mtk_fixed_factor top_fixed_divs[] __initconst = {
	FACTOR(CLK_TOP_SYSPLL_CK, "syspll_ck", "mainpll", 1, 1),
	FACTOR(CLK_TOP_SYSPLL_D2, "syspll_d2", "mainpll", 1, 2),
	FACTOR(CLK_TOP_SYSPLL_D3, "syspll_d3", "mainpll", 1, 3),
	FACTOR(CLK_TOP_SYSPLL_D5, "syspll_d5", "mainpll", 1, 5),
	FACTOR(CLK_TOP_SYSPLL_D7, "syspll_d7", "mainpll", 1, 7),
	FACTOR(CLK_TOP_SYSPLL1_D2, "syspll1_d2", "syspll_d2", 1, 2),
	FACTOR(CLK_TOP_SYSPLL1_D4, "syspll1_d4", "syspll_d2", 1, 4),
	FACTOR(CLK_TOP_SYSPLL1_D8, "syspll1_d8", "syspll_d2", 1, 8),
	FACTOR(CLK_TOP_SYSPLL1_D16, "syspll1_d16", "syspll_d2", 1, 16),
	FACTOR(CLK_TOP_SYSPLL2_D2, "syspll2_d2", "syspll_d3", 1, 2),
	FACTOR(CLK_TOP_SYSPLL2_D4, "syspll2_d4", "syspll_d3", 1, 4),
	FACTOR(CLK_TOP_SYSPLL2_D8, "syspll2_d8", "syspll_d3", 1, 8),
	FACTOR(CLK_TOP_SYSPLL3_D2, "syspll3_d2", "syspll_d5", 1, 2),
	FACTOR(CLK_TOP_SYSPLL3_D4, "syspll3_d4", "syspll_d5", 1, 4),
	FACTOR(CLK_TOP_SYSPLL4_D2, "syspll4_d2", "syspll_d7", 1, 2),
	FACTOR(CLK_TOP_SYSPLL4_D4, "syspll4_d4", "syspll_d7", 1, 4),
	FACTOR(CLK_TOP_UNIVPLL_CK, "univpll_ck", "univpll", 1, 2),
	FACTOR(CLK_TOP_UNIVPLL_D2, "univpll_d2", "univpll", 1, 4),
	FACTOR(CLK_TOP_UNIVPLL_D3, "univpll_d3", "univpll", 1, 6),
	FACTOR(CLK_TOP_UNIVPLL_D5, "univpll_d5", "univpll", 1, 10),
	FACTOR(CLK_TOP_UNIVPLL_D26, "univpll_d26", "univpll", 1, 52),
	FACTOR(CLK_TOP_UNIVPLL_D52, "univpll_d52", "univpll", 1, 104),
	FACTOR(CLK_TOP_UNIVPLL_D52_D2, "univpll_d52_d2", "univpll", 1, 208),
	FACTOR(CLK_TOP_UNIVPLL_D52_D4, "univpll_d52_d4", "univpll", 1, 416),
	FACTOR(CLK_TOP_UNIVPLL1_D2, "univpll1_d2", "univpll_d2", 1, 2),
	FACTOR(CLK_TOP_UNIVPLL1_D4, "univpll1_d4", "univpll_d2", 1, 4),
	FACTOR(CLK_TOP_UNIVPLL1_D8, "univpll1_d8", "univpll_d2", 1, 8),
	FACTOR(CLK_TOP_UNIVPLL2_D2, "univpll2_d2", "univpll_d3", 1, 2),
	FACTOR(CLK_TOP_UNIVPLL2_D4, "univpll2_d4", "univpll_d3", 1, 4),
	FACTOR(CLK_TOP_UNIVPLL2_D8, "univpll2_d8", "univpll_d3", 1, 8),
	FACTOR(CLK_TOP_UNIVPLL3_D2, "univpll3_d2", "univpll_d5", 1, 2),
	FACTOR(CLK_TOP_UNIVPLL3_D4, "univpll3_d4", "univpll_d5", 1, 4),
	FACTOR(CLK_TOP_UNIVPLL3_D8, "univpll3_d8", "univpll_d5", 1, 8),
	FACTOR(CLK_TOP_USB_CK, "usb_ck", "univpll", 1, 13),
	FACTOR(CLK_TOP_MMPLL_CK, "mmpll_ck", "mmpll", 1, 1),
	FACTOR(CLK_TOP_MMPLL_D4, "mmpll_d4", "mmpll", 1, 4),
	FACTOR(CLK_TOP_MMPLL_D5, "mmpll_d5", "mmpll", 1, 5),
	FACTOR(CLK_TOP_MMPLL_D7, "mmpll_d7", "mmpll", 1, 7),
	FACTOR(CLK_TOP_MMPLL1_D2, "mmpll1_d2", "mmpll_d4", 1, 2),
	FACTOR(CLK_TOP_MMPLL1_D4, "mmpll1_d4", "mmpll_d4", 1, 4),
	FACTOR(CLK_TOP_MMPLL1_D8, "mmpll1_d8", "mmpll_d4", 1, 8),
	FACTOR(CLK_TOP_MMPLL1_D10, "mmpll1_d10", "mmpll_d4", 1, 10),
	FACTOR(CLK_TOP_MMPLL1_D16, "mmpll1_d16", "mmpll_d4", 1, 16),
	FACTOR(CLK_TOP_MMPLL1_D32, "mmpll1_d32", "mmpll_d4", 1, 32),
	FACTOR(CLK_TOP_MMPLL1_D64, "mmpll1_d64", "mmpll_d4", 1, 64),
	FACTOR(CLK_TOP_MMPLL2_D2, "mmpll2_d2", "mmpll_d5", 1, 2),
	FACTOR(CLK_TOP_MMPLL2_D4, "mmpll2_d4", "mmpll_d5", 1, 4),
	FACTOR(CLK_TOP_MMPLL3_D2, "mmpll3_d2", "mmpll_d7", 1, 2),
	FACTOR(CLK_TOP_MMPLL3_D4, "mmpll3_d4", "mmpll_d7", 1, 4),
	FACTOR(CLK_TOP_APLL1_CK, "apll1_ck", "apll1", 1, 1),
	FACTOR(CLK_TOP_APLL1_D3, "apll1_d3", "apll1", 1, 3),
	FACTOR(CLK_TOP_APLL1_D6, "apll1_d6", "apll1", 1, 6),
	FACTOR(CLK_TOP_APLL2_CK, "apll2_ck", "apll2", 1, 1),
	FACTOR(CLK_TOP_APLL2_D3, "apll2_d3", "apll2", 1, 3),
	FACTOR(CLK_TOP_APLL2_D6, "apll2_d6", "apll2", 1, 6),
	FACTOR(CLK_TOP_EMIPLL_CK, "emipll_ck", "emipll", 1, 1),
	FACTOR(CLK_TOP_EMIPLL_D2, "emipll_d2", "emipll", 1, 2),
	FACTOR(CLK_TOP_EMIPLL_D3, "emipll_d3", "emipll", 1, 3),
	FACTOR(CLK_TOP_EMIPLL_D4, "emipll_d4", "emipll", 1, 4),
	FACTOR(CLK_TOP_EMIPLL_D8, "emipll_d8", "emipll", 1, 8),
	FACTOR(CLK_TOP_EMIPLL_D16, "emipll_d16", "emipll", 1, 16),
	FACTOR(CLK_TOP_ULPOSCPLL_CK, "ulposcpll_ck", "ulposc262m", 1, 1),
	FACTOR(CLK_TOP_ULPOSCPLL_D2, "ulposcpll_d2", "ulposcpll_ck", 1, 2),
	FACTOR(CLK_TOP_ULPOSCPLL_D8, "ulposcpll_d8", "ulposcpll_ck", 1, 8),
	FACTOR(CLK_TOP_DPAPLL_CK, "dpapll_ck", "dpapll", 1, 1),
	FACTOR(CLK_TOP_DPAPLL_D3, "dpapll_d3", "dpapll", 1, 3),
	FACTOR(CLK_TOP_DPAPLL_D6, "dpapll_d6", "dpapll", 1, 6),
	FACTOR(CLK_TOP_MSDCPLL_CK, "msdcpll_ck", "msdcpll", 1, 1),
	FACTOR(CLK_TOP_ARMPLL_CK, "armpll_ck", "armpll", 1, 1),
	FACTOR(CLK_TOP_VPUPLL_CK, "vpupll_ck", "vpupll", 1, 1),
	FACTOR(CLK_TOP_VPUPLL_D3, "vpupll_d3", "vpupll", 1, 3),
	FACTOR(CLK_TOP_VPUPLL_D4, "vpupll_d4", "vpupll", 1, 4),
	FACTOR(CLK_TOP_VPUPLL_D5, "vpupll_d5", "vpupll", 1, 5),
	FACTOR(CLK_TOP_VPUPLL_D7, "vpupll_d7", "vpupll", 1, 7),
	FACTOR(CLK_TOP_VPUPLL1_D2, "vpupll1_d2", "vpupll_d3", 1, 2),
	FACTOR(CLK_TOP_VPUPLL1_D3, "vpupll1_d3", "vpupll_d3", 1, 3),
	FACTOR(CLK_TOP_VPUPLL1_D4, "vpupll1_d4", "vpupll_d3", 1, 4),
	FACTOR(CLK_TOP_VPUPLL2_D2, "vpupll2_d2", "vpupll_d4", 1, 2),
	FACTOR(CLK_TOP_VPUPLL2_D5, "vpupll2_d5", "vpupll_d4", 1, 5),
	FACTOR(CLK_TOP_VPUPLL2_D14, "vpupll2_d14", "vpupll_d4", 1, 14),
	FACTOR(CLK_TOP_VPUPLL2_D28, "vpupll2_d28", "vpupll_d4", 1, 28),
	FACTOR(CLK_TOP_VPUPLL2_D42, "vpupll2_d42", "vpupll_d4", 1, 42),
	FACTOR(CLK_TOP_VPUPLL3_D2, "vpupll3_d2", "vpupll_d5", 1, 2),
	FACTOR(CLK_TOP_GPUPLL_CK, "gpupll_ck", "gpupll", 1, 1),
	FACTOR(CLK_TOP_AD_AP27PLL_CK, "clk27m_ck", "ap27pll", 1, 1),
	FACTOR(CLK_TOP_DDDS1_CK, "ad_ddds1_ck", "ddds624m", 1, 1),
	FACTOR(CLK_TOP_DDDS1_VPS_CK, "ad_ddds1_vps_ck", "ddds320m", 1, 1),
	FACTOR(CLK_TOP_DSI_REF_PLL_CK, "dsi_ref_pll_ck", "dsi_ref_pll_sel",
		1, 1),
	FACTOR(CLK_TOP_DSI_REF_PLL_D2, "dsi_ref_pll_d2", "dsi_ref_pll_sel",
		1, 2),
	FACTOR(CLK_TOP_DSI_REF_PLL_D4, "dsi_ref_pll_d4", "dsi_ref_pll_sel",
		1, 4),
	FACTOR(CLK_TOP_DSI_REF_PLL_D8, "dsi_ref_pll_d8", "dsi_ref_pll_sel",
		1, 8),
	FACTOR(CLK_TOP_DPRX_O_DSC_CK, "dprx_o_dsc_ck", "clk26m", 1, 1),
	FACTOR(CLK_TOP_DPRX_O_VIDEO_CK, "dprx_o_video_ck", "clk32k", 1, 1),
	FACTOR(CLK_TOP_DMPLLCH0_CK, "dmpllch0_ck", "dmpllch0466m", 1, 1),
	FACTOR(CLK_TOP_CLK26M_CK, "clk26m_ck", "clk26m", 1, 1),
	FACTOR(CLK_TOP_CLK26M_D2, "clk26m_d2", "clk26m", 1, 2),
	FACTOR(CLK_TOP_CLK26M_D10, "clk26m_d10", "clk26m", 1, 10),
	FACTOR(CLK_TOP_CLK26M_D26, "clk26m_d26", "clk26m", 1, 26),
	FACTOR(CLK_TOP_CLK66M_CK, "clk66m_ck", "clk66m", 1, 1),
	FACTOR(CLK_TOP_CLK32K_CK, "clk32k_ck", "clk32k", 1, 1),
};

static const char * const dsp_parents[] __initconst = {
	"clk26m_ck",
	"vpupll1_d2",
	"mmpll_d4",
	"vpupll_d4",
	"mmpll_d5",
	"vpupll_d5",
	"syspll_d2",
	"mmpll_d7",
	"univpll_d3",
	"syspll_d3",
	"univpll1_d2",
	"univpll_d5",
	"mmpll1_d4",
	"univpll1_d4",
	"mmpll1_d8"
};


static const char * const dsp1_parents[] __initconst = {
	"clk26m_ck",
	"vpupll1_d2",
	"mmpll_d4",
	"vpupll_d4",
	"mmpll_d5",
	"vpupll_d5",
	"syspll_d2",
	"mmpll_d7",
	"univpll_d3",
	"syspll_d3",
	"univpll1_d2",
	"univpll_d5",
	"mmpll1_d4",
	"univpll1_d4",
	"mmpll1_d8"
};

static const char * const dsp2_parents[] __initconst = {
	"clk26m_ck",
	"vpupll1_d2",
	"mmpll_d4",
	"vpupll_d4",
	"mmpll_d5",
	"vpupll_d5",
	"syspll_d2",
	"mmpll_d7",
	"univpll_d3",
	"syspll_d3",
	"univpll1_d2",
	"univpll_d5",
	"mmpll1_d4",
	"univpll1_d4",
	"mmpll1_d8"
};

static const char * const dsp3_parents[] __initconst = {
	"clk26m_ck",
	"vpupll1_d2",
	"mmpll_d4",
	"vpupll_d4",
	"mmpll_d5",
	"vpupll_d5",
	"syspll_d2",
	"mmpll_d7",
	"univpll_d3",
	"syspll_d3",
	"univpll1_d2",
	"univpll_d5",
	"mmpll1_d4",
	"univpll1_d4",
	"mmpll1_d8"
};

static const char * const ipu_if_parents[] __initconst = {
	"clk26m_ck",
	"vpupll1_d2",
	"mmpll_d4",
	"vpupll_d4",
	"mmpll_d5",
	"vpupll_d5",
	"syspll_d2",
	"mmpll_d7",
	"univpll_d3",
	"syspll_d3",
	"univpll1_d2",
	"univpll_d5",
	"mmpll1_d4",
	"univpll1_d4",
	"mmpll1_d8"
};

static const char * const smi_parents[] __initconst = {
	"clk26m_ck",
	"dmpllch0_ck",
	"emipll_d8",
	"mmpll1_d4",
	"mmpll1_d2",
	"mmpll_d7",
	"emipll_d2",
	"vpupll1_d2"
};

static const char * const emi_parents[] __initconst = {
	"clk26m_ck",
	"dmpllch0_ck",
	"clk26m_ck",
	"emipll_d2",
	"mmpll_d7",
	"mmpll1_d2",
	"vpupll3_d2",
	"mmpll1_d4"
};

static const char * const axi_parents[] __initconst = {
	"clk26m_ck",
	"emipll_d4",
	"emipll_d8",
	"vpupll1_d4",
	"univpll2_d2",
	"emipll_d2",
	"syspll_d7"
};

static const char * const sflash_parents[] __initconst = {
	"clk26m_ck",
	"syspll4_d4",
	"univpll2_d8",
	"univpll3_d4",
	"syspll1_d8",
	"univpll1_d8",
	"syspll2_d4",
	"univpll2_d4"
};

static const char * const pwm_parents[] __initconst = {
	"clk26m_ck",
	"mmpll1_d8",
	"clk32k_ck",
	"univpll3_d4"
};

static const char * const camtg_parents[] __initconst = {
	"clk26m_ck",
	"univpll2_d2",
	"univpll_d26",
	"univpll1_d4",
	"syspll1_d8"
};

static const char * const seninf_parents[] __initconst = {
	"clk26m_ck",
	"univpll1_d2",
	"univpll2_d2",
	"syspll2_d2",
	"univpll1_d4"
};

static const char * const seninf_gaze_parents[] __initconst = {
	"clk26m_ck",
	"univpll1_d2",
	"univpll2_d2",
	"syspll1_d2",
	"univpll1_d4"
};

static const char * const senm0_parents[] __initconst = {
	"clk26m_ck",
	"univpll_d52",
	"univpll2_d8",
	"univpll_d26",
	"clk26m_d2",
	"univpll_d52_d2",
	"vpupll2_d46",
	"vpupll2_d28",
	"univpll_d52_d4"
};

static const char * const senm1_parents[] __initconst = {
	"clk26m_ck",
	"univpll_d52",
	"univpll2_d8",
	"univpll_d26",
	"clk26m_d2",
	"univpll_d52_d2",
	"vpupll2_d46",
	"vpupll2_d28",
	"univpll_d52_d4"
};

static const char * const senm2_parents[] __initconst = {
	"clk26m_ck",
	"univpll_d52",
	"univpll2_d8",
	"univpll_d26",
	"clk26m_d2",
	"univpll_d52_d2",
	"vpupll2_d46",
	"vpupll2_d28",
	"univpll_d52_d4"
};

static const char * const senm3_parents[] __initconst = {
	"clk26m_ck",
	"univpll_d52",
	"univpll2_d8",
	"univpll_d26",
	"clk26m_d2",
	"univpll_d52_d2",
	"vpupll2_d46",
	"vpupll2_d28",
	"univpll_d52_d4"
};

static const char * const senm4_parents[] __initconst = {
	"clk26m_ck",
	"univpll_d52",
	"univpll2_d8",
	"univpll_d26",
	"clk26m_d2",
	"univpll_d52_d2",
	"vpupll2_d46",
	"vpupll2_d28",
	"univpll_d52_d4"
};

static const char * const senm5_parents[] __initconst = {
	"clk26m_ck",
	"univpll_d52",
	"univpll2_d8",
	"univpll_d26",
	"clk26m_d2",
	"univpll_d52_d2",
	"vpupll2_d46",
	"vpupll2_d28",
	"univpll_d52_d4"
};

static const char * const senm6_parents[] __initconst = {
	"clk26m_ck",
	"univpll_d52",
	"univpll2_d8",
	"univpll_d26",
	"clk26m_d2",
	"univpll_d52_d2",
	"vpupll2_d46",
	"vpupll2_d28",
	"univpll_d52_d4"
};

static const char * const senm7_parents[] __initconst = {
	"clk26m_ck",
	"univpll_d52",
	"univpll2_d8",
	"univpll_d26",
	"clk26m_d2",
	"univpll_d52_d2",
	"vpupll2_d46",
	"vpupll2_d28",
	"univpll_d52_d4"
};

static const char * const senm8_parents[] __initconst = {
	"clk26m_ck",
	"univpll_d52",
	"univpll2_d8",
	"univpll_d26",
	"clk26m_d2",
	"univpll_d52_d2",
	"vpupll2_d46",
	"vpupll2_d28",
	"univpll_d52_d4"
};

static const char * const i2c_parents[] __initconst = {
	"clk26m_ck",
	"syspll1_d4",
	"mmpll1_d8",
	"mmpll1_d10"
};

static const char * const uart_parents[] __initconst = {
	"clk26m_ck",
	"univpll2_d8"
};

static const char * const spi_parents[] __initconst = {
	"clk26m_ck",
	"mmpll2_d4",
	"syspll1_d4",
	"mmpll1_d8",
	"univpll3_d2",
	"univpll2_d4",
	"univpll1_d8"
};

static const char * const axi_peri_parents[] __initconst = {
	"clk26m_ck",
	"emipll_d4",
	"emipll_d2",
	"emipll_d8",
	"syspll2_d2",
	"vpupll1_d4",
	"univpll2_d2",
	"syspll_d7"
};

static const char * const usbh_sys_parents[] __initconst = {
	"clk26m_ck",
	"mmpll1_d4",
	"syspll2_d2",
	"univpll3_d2"
};

static const char * const usbh_xhc_parents[] __initconst = {
	"clk26m_ck",
	"mmpll1_d4",
	"syspll2_d2",
	"univpll3_d2"
};

static const char * const usbd_sys_parents[] __initconst = {
	"clk26m_ck",
	"mmpll1_d4",
	"syspll2_d2",
	"mmpll2_d4"
};


static const char * const msdc50_0_parents[] __initconst = {
	"clk26m_ck",
	"mmpll1_d2",
	"univpll2_d2",
	"msdcpll_ck",
	"syspll2_d2",
	"syspll_d3",
	"mmpll1_d4",
	"univpll1_d2"
};

static const char * const msdc50_0_h_parents[] __initconst = {
	"clk26m_ck",
	"vpupll1_d4",
	"syspll2_d2",
	"syspll4_d2",
	"mmpll1_d4",
	"univpll1_d4"
};

static const char * const aes_fde_parents[] __initconst = {
	"clk26m_ck",
	"mmpll_d7",
	"syspll_d2",
	"vpupll1_d2",
	"syspll_d3",
	"vpupll_d7",
	"syspll1_d2",
	"syspll1_d4"
};

static const char * const aud_intbus_parents[] __initconst = {
	"clk26m_ck",
	"syspll1_d4",
	"syspll4_d2",
	"emipll_d4",
	"univpll3_d2",
	"univpll2_d8"
};

static const char * const a1sys_parents[] __initconst = {
	"clk26m_ck",
	"apll1_ck",
	"apll1_d3",
	"apll1_d6"
};

static const char * const a2sys_parents[] __initconst = {
	"clk26m_ck",
	"apll2_ck",
	"apll2_d3",
	"apll2_d6"
};

static const char * const a3sys_parents[] __initconst = {
	"clk26m_ck",
	"dpapll_ck",
	"dpapll_d3",
	"dpapll_d6",
	"clk26m_ck",
	"clk26m_ck",
	"clk26m_ck",
	"syspll1_d2"
};

static const char * const tdmin_parents[] __initconst = {
	"clk26m_ck",
	"apll1_ck",
	"apll2_ck",
	"dpapll_ck",
	"clk26m_ck"
};

static const char * const tdmout_parents[] __initconst = {
	"clk26m_ck",
	"apll1_ck",
	"apll2_ck",
	"dpapll_ck",
	"clk26m_ck"
};

static const char * const pmicspi_parents[] __initconst = {
	"clk26m_ck",
	"syspll1_d8",
	"univpll2_d8",
	"syspll1_d16",
	"univpll3_d4",
	"univpll_d26"
};

static const char * const pmicgspi_parents[] __initconst = {
	"clk26m_ck",
	"mmpll2_d4",
	"syspll1_d4",
	"mmpll1_d8",
	"univpll3_d2",
	"univpll2_d4",
	"univpll_1_d8"
};

static const char * const scp_parents[] __initconst = {
	"clk26m_ck",
	"syspll1_d2",
	"univpll_d5",
	"vpupll1_d4",
	"emipll_d4",
	"vpupll3_d2",
	"univpll1_d2"
};

static const char * const mutexgen_parents[] __initconst = {
	"clk26m_ck",
	"clk26m_d10",
	"clk26m_d2",
	"univpll_d52_d4"
};

static const char * const stc_h27_parents[] __initconst = {
	"clk26m_ck",
	"clk27m_ck"
};

static const char * const mfg_parents[] __initconst = {
	"clk26m_ck",
	"gpupll_ck",
	"univpll_d3",
	"syspll_d3",
	"univpll_d5"
};

static const char * const crypto_parents[] __initconst = {
	"clk26m_ck",
	"syspll_d3",
	"univpll_d3",
	"univpll1_d2",
	"syspll1_d2",
	"univpll_d5",
	"univpll3_d2"
};

static const char * const img_s0_parents[] __initconst = {
	"clk26m_ck",
	"univpll1_d2",
	"univpll1_d4",
	"univpll2_d2",
	"vpupll3_d2",
	"syspll1_d2",
	"univpll_d5",
	"univpll3_d2"
};

static const char * const img_s1_parents[] __initconst = {
	"clk26m_ck",
	"univpll1_d2",
	"univpll1_d4",
	"univpll2_d2",
	"vpupll3_d2",
	"syspll1_d2",
	"univpll_d5",
	"univpll3_d2"
};

static const char * const mmsys_com_parents[] __initconst = {
	"clk26m_ck",
	"mmpll2_d2",
	"mmpll1_d4",
	"syspll_d3",
	"vpupll3_d2",
	"syspll2_d8"
};

static const char * const fm_parents[] __initconst = {
	"clk26m_ck",
	"mmpll1_d2",
	"mmpll2_d2",
	"syspll_d3",
	"mmpll1_d4",
	"syspll2_d8"
};

static const char * const mmsys_core_parents[] __initconst = {
	"clk26m_ck",
	"mmpll2_d2",
	"mmpll2_d4",
	"vpupll1_d3",
	"syspll_d3",
	"mmpll1_d4",
	"emipll_d3"
};

static const char * const mm_sysram0_parents[] __initconst = {
	"clk26m_ck",
	"vpupll2_d2",
	"mmpll1_d2",
	"vpupll_d7",
	"syspll_d3",
	"vpupll3_d2",
	"univpll_d3",
	"mmpll_d7"
};

static const char * const mm_sysram1_parents[] __initconst = {
	"clk26m_ck",
	"vpupll2_d2",
	"mmpll1_d2",
	"vpupll_d7",
	"syspll_d3",
	"vpupll3_d2",
	"univpll_d3",
	"mmpll_d7"
};

static const char * const mm_sysram2_parents[] __initconst = {
	"clk26m_ck",
	"vpupll2_d2",
	"mmpll1_d2",
	"vpupll_d7",
	"syspll_d3",
	"vpupll3_d2",
	"univpll_d3",
	"mmpll_d7"
};

static const char * const cam_s0_parents[] __initconst = {
	"clk26m_ck",
	"univpll1_d2",
	"univpll1_d4",
	"univpll2_d2",
	"vpupll3_d2",
	"syspll1_d2",
	"univpll_d5",
	"univpll3_d2"
};

static const char * const cam_s1_parents[] __initconst = {
	"clk26m_ck",
	"univpll1_d2",
	"univpll1_d4",
	"univpll2_d2",
	"vpupll3_d2",
	"syspll1_d2",
	"univpll_d5",
	"univpll3_d2"
};

static const char * const cam_gz0_parents[] __initconst = {
	"clk26m_ck",
	"univpll1_d2",
	"univpll1_d4",
	"univpll2_d2",
	"vpupll3_d2",
	"syspll1_d2",
	"univpll_d5",
	"univpll3_d2"
};

static const char * const cam_gz1_parents[] __initconst = {
	"clk26m_ck",
	"univpll1_d2",
	"univpll1_d4",
	"univpll2_d2",
	"vpupll3_d2",
	"syspll1_d2",
	"univpll_d5",
	"univpll3_d2"
};

static const char * const mm_syncgen_parents[] __initconst = {
	"clk26m_ck",
	"clk26m_d10",
	"clk26m_d2",
	"univpll_d52_d4"
};

static const char * const atb_parents[] __initconst = {
	"clk26m_ck",
	"syspll1_d4",
	"syspll1_d2"
};

static const char * const disppwm_parents[] __initconst = {
	"clk26m_ck",
	"mmpll2_d4",
	"ulposcpll_d2",
	"ulposcpll_d8",
	"mmpll11_d16"
};

static const char * const header_sys_parents[] __initconst = {
	"clk26m_ck",
	"mmpll_d7",
	"syspll_d3",
	"mmpll2_d2"
};


static const char * const ddds_sys_parents[] __initconst = {
	"clk26m_ck",
	"univpll_d2",
	"ad_ddds1_ck"
};


static const char * const slicer_vid_parents[] __initconst = {
	"clk26m_ck",
	"dprx_o_video_ck",
	"clk26m_ck",
	"univpll_d2"
};

static const char * const slicer_dsc_parents[] __initconst = {
	"clk26m_ck",
	"dprx_o_dsc_ck",
	"clk26m_ck",
	"univpll_d2"
};

static const char * const dsi_ref_parents[] __initconst = {
	"clk26m_ck",
	"dsi_ref_pll_ck",
	"dsi_ref_pll_d2",
	"dsi_ref_pll_d4",
	"dsi_ref_pll_d8"
};

static const char * const dsi_ref_pll_parents[] __initconst = {
	"clk_null",
	"ad_ddds1_vps_ck"
};

static const char * const tpiu_parents[] __initconst = {
	"clk26m_ck",
	"mmpll1_d4"
};

static const char * const ddds_vps_parents[] __initconst = {
	"clk_null",
	"ad_ddds1_vps_ck"
};

static const struct mtk_composite top_muxes[] __initconst = {
	MUX(CLK_TOP_DSP_SEL, "dsp_sel", dsp_parents, 0x0100, 12, 4),
	MUX(CLK_TOP_SMI_SEL, "smi_sel", smi_parents, 0x0100, 8, 3),
	MUX(CLK_TOP_EMI_SEL, "emi_sel", emi_parents, 0x0100, 4, 3),
	MUX(CLK_TOP_AXI_SEL, "axi_sel", axi_parents, 0x0100, 0, 3),
	MUX(CLK_TOP_SFLASH_SEL, "sflash_sel", sflash_parents, 0x0100, 28, 3),
	MUX(CLK_TOP_IPU_IF_SEL, "ipu_if_sel", ipu_if_parents, 0x0100, 16, 4),
	MUX(CLK_TOP_PWM_SEL, "pwm_sel", pwm_parents, 0x0110, 0, 2),
	MUX(CLK_TOP_CAMTG_SEL, "camtg_sel", camtg_parents, 0x0110, 4, 3),
	MUX(CLK_TOP_SENINF_SEL, "seninf_sel", seninf_parents,
		0x0110, 8, 3),
	MUX(CLK_TOP_SENM0_SEL, "senm0_sel", senm0_parents, 0x0110, 12, 4),
	MUX(CLK_TOP_SENM1_SEL, "senm1_sel", senm1_parents, 0x0110, 16, 4),
	MUX(CLK_TOP_SENM2_SEL, "senm2_sel", senm2_parents, 0x0110, 20, 4),
	MUX(CLK_TOP_SENM3_SEL, "senm3_sel", senm3_parents, 0x0110, 24, 4),
	MUX(CLK_TOP_SENM4_SEL, "senm4_sel", senm4_parents, 0x0110, 28, 4),
	MUX(CLK_TOP_SENM5_SEL, "senm5_sel", senm5_parents, 0x0120, 0, 4),
	MUX(CLK_TOP_SENM6_SEL, "senm6_sel", senm6_parents, 0x0120, 4, 4),
	MUX(CLK_TOP_SENM7_SEL, "senm7_sel", senm7_parents, 0x0120, 8, 4),
	MUX(CLK_TOP_SENM8_SEL, "senm8_sel", senm8_parents, 0x0120, 12, 4),
	MUX(CLK_TOP_I2C_SEL, "i2c_sel", i2c_parents, 0x0120, 16, 2),
	MUX(CLK_TOP_UART_SEL, "uart_sel", uart_parents, 0x0120, 20, 1),
	MUX(CLK_TOP_SPI_SEL, "spi_sel", spi_parents, 0x0120, 24, 3),
	MUX(CLK_TOP_AXI_PERI_SEL, "axi_peri_sel", axi_peri_parents,
		0x0120, 28, 3),
	MUX(CLK_TOP_USBH_SYS_SEL, "usbh_sys_sel", usbh_sys_parents,
		0x0130, 0, 2),
	MUX(CLK_TOP_USBH_XHC_SEL, "usbh_xhc_sel", usbh_xhc_parents,
		0x0130, 4, 2),
	MUX(CLK_TOP_USBD_SYS_SEL, "usbd_sys_sel", usbd_sys_parents,
		0x0130, 8, 2),
	MUX(CLK_TOP_MSDC50_0_SEL, "msdc_sel", msdc50_0_parents, 0x0130, 12, 3),
	MUX(CLK_TOP_MSDC50_0_HCLK_SEL, "msdc50_h_sel", msdc50_0_h_parents,
		0x0130, 16, 3),
	MUX(CLK_TOP_AES_FDE_SEL, "aes_fde_sel", aes_fde_parents,
		0x0130, 20, 3),
	MUX(CLK_TOP_AUD_INTBUS_SEL, "aud_intbus_sel", aud_intbus_parents,
		0x0130, 24, 3),
	MUX(CLK_TOP_A1SYS_HP_SEL, "a1sys_hp_sel", a1sys_parents, 0x0130, 28, 2),
	MUX(CLK_TOP_A2SYS_HP_SEL, "a2sys_hp_sel", a2sys_parents, 0x0140, 0, 2),
	MUX(CLK_TOP_A3SYS_HP_SEL, "a3sys_hp_sel", a3sys_parents, 0x0140, 4, 3),
	MUX(CLK_TOP_TDMIN_SEL, "tdmin_sel", tdmin_parents, 0x0140, 8, 3),
	MUX(CLK_TOP_TDMOUT_SEL, "tdmout_sel", tdmout_parents, 0x0140, 12, 3),
	MUX(CLK_TOP_PMICSPI_SEL, "pmicspi_sel", pmicspi_parents, 0x0140, 20, 3),
	MUX(CLK_TOP_SCP_SEL, "scp_sel", scp_parents, 0x0140, 28, 3),
	MUX(CLK_TOP_MUTEXGEN_SEL, "mutexgen_sel", mutexgen_parents,
		0x0150, 0, 2),
	MUX(CLK_TOP_STC_H27_SEL, "stc_h27_sel", stc_h27_parents,
		0x0150, 4, 1),
	MUX(CLK_TOP_MFG_SEL, "mfg_sel", mfg_parents, 0x0150, 12, 3),
	MUX(CLK_TOP_CRYPTO_SEL, "crypto_sel", crypto_parents, 0x0150, 24, 3),
	MUX(CLK_TOP_IMG_S0_SEL, "img_s0_sel", img_s0_parents, 0x0150, 28, 3),
	MUX(CLK_TOP_IMG_S1_SEL, "img_s1_sel", img_s1_parents, 0x0160, 0, 3),
	MUX(CLK_TOP_MMSYS_COM_SEL, "mmsys_com_sel", mmsys_com_parents,
		0x0160, 4, 3),
	MUX(CLK_TOP_FM_SEL, "fm_sel", fm_parents, 0x0160, 8, 3),
	MUX(CLK_TOP_MMSYS_CORE_SEL, "mmsys_core_sel", mmsys_core_parents,
		0x0160, 12, 3),
	MUX(CLK_TOP_MM_SYSRAM0_SEL, "mm_sysram0_sel", mm_sysram0_parents,
		0x0160, 16, 3),
	MUX(CLK_TOP_MM_SYSRAM1_SEL, "mm_sysram1_sel", mm_sysram1_parents,
		0x0160, 20, 3),
	MUX(CLK_TOP_MM_SYSRAM2_SEL, "mm_sysram2_sel", mm_sysram2_parents,
		0x0160, 24, 3),
	MUX(CLK_TOP_SENINF_GAZE_SEL, "seninf_gaze_sel", seninf_gaze_parents,
		0x0170, 12, 3),
	MUX(CLK_TOP_CAM_S0_SEL, "cam_s0_sel", cam_s0_parents,
		0x0170, 16, 3),
	MUX(CLK_TOP_CAM_S1_SEL, "cam_s1_sel", cam_s1_parents,
		0x0170, 20, 3),
	MUX(CLK_TOP_CAM_GZ0_SEL, "cam_gz0_sel", cam_gz0_parents,
		0x0170, 24, 3),
	MUX(CLK_TOP_CAM_GZ1_SEL, "cam_gz1_sel", cam_gz1_parents,
		0x0170, 28, 3),
	MUX(CLK_TOP_MM_SYNCGEN_SEL, "mm_syncgen_sel", mm_syncgen_parents,
		0x0180, 0, 2),
	MUX(CLK_TOP_ATB_SEL, "atb_sel", atb_parents, 0x0180, 16, 2),
	MUX(CLK_TOP_DISPPWM_SEL, "disppwm_sel", disppwm_parents,
		0x0180, 24, 3),
	MUX(CLK_TOP_PMICGSPI_SEL, "pmicgspi_sel", pmicgspi_parents,
		0x01E0, 0, 3),
	MUX(CLK_TOP_DDDS_SYS_SEL, "ddds_sys_sel", ddds_sys_parents,
		0x01E0, 4, 2),
	MUX(CLK_TOP_DSP1_SEL, "dsp1_sel", dsp1_parents, 0x01E0, 8, 4),
	MUX(CLK_TOP_DSP2_SEL, "dsp2_sel", dsp2_parents, 0x01E0, 12, 4),
	MUX(CLK_TOP_DSP3_SEL, "dsp3_sel", dsp3_parents, 0x01E0, 16, 4),
	MUX(CLK_TOP_HEADER_SYS_SEL, "header_sys_sel", header_sys_parents,
		0x01E0, 20, 2),
	MUX(CLK_TOP_SLICER_VID_SEL, "slicer_vid_sel", slicer_vid_parents,
		0x01E0, 23, 2),
	MUX(CLK_TOP_DSI_REF_SEL, "dsi_ref_sel", dsi_ref_parents, 0x01E0, 25,
		3),
	MUX(CLK_TOP_DSI_REF_PLL_SEL, "dsi_ref_pll_sel", dsi_ref_pll_parents,
		0x01E0, 28, 1),
	MUX(CLK_TOP_TPIU_SEL, "tpiu_sel", tpiu_parents, 0x01E0, 29, 1),
	MUX(CLK_TOP_SLICER_DSC_SEL, "slicer_dsc_sel", slicer_dsc_parents,
		0x01F0, 28,	2),
	MUX(CLK_TOP_DDDS_VSP_SEL, "ddds_vsp_sel", ddds_vps_parents, 0x01F0, 0,
		1),
	DIV_GATE(CLK_TOP_APLL12_DIV0, "apll12_div0", "tdmin_cg", 0x0320, 2,
		0x324, 12, 0),
	DIV_GATE(CLK_TOP_APLL12_DIV1, "apll12_div1", "tdmout_cg", 0x0320, 3,
		0x324, 12, 12),
	DIV_GATE(CLK_TOP_APLL12_DIV2, "apll12_div2", "apll12_div0", 0x0320, 4,
		0x324, 8, 24),
	DIV_GATE(CLK_TOP_APLL12_DIV3, "apll12_div3", "apll12_div1", 0x0320, 5,
		0x328, 8, 0),
};

static const struct mtk_gate_regs topckgen0_cg_regs __initconst = {
	.set_ofs = 0x0190,
	.clr_ofs = 0x0190,
	.sta_ofs = 0x0190,
};

static const struct mtk_gate_regs topckgen1_cg_regs __initconst = {
	.set_ofs = 0x01A0,
	.clr_ofs = 0x01A0,
	.sta_ofs = 0x01A0,
};

static const struct mtk_gate_regs topckgen2_cg_regs __initconst = {
	.set_ofs = 0x01B0,
	.clr_ofs = 0x01B0,
	.sta_ofs = 0x01B0,
};

static const struct mtk_gate_regs topckgen3_cg_regs __initconst = {
	.set_ofs = 0x01C0,
	.clr_ofs = 0x01C0,
	.sta_ofs = 0x01C0,
};

static const struct mtk_gate_regs topckgen4_cg_regs __initconst = {
	.set_ofs = 0x01D0,
	.clr_ofs = 0x01D0,
	.sta_ofs = 0x01D0,
};

static const struct mtk_gate_regs topckgen5_cg_regs __initconst = {
	.set_ofs = 0x01E0,
	.clr_ofs = 0x01E0,
	.sta_ofs = 0x01E0,
};

static const struct mtk_gate_regs topckgen6_cg_regs __initconst = {
	.set_ofs = 0x01F0,
	.clr_ofs = 0x01F0,
	.sta_ofs = 0x01F0,
};

static const struct mtk_gate_regs topckgen7_cg_regs __initconst = {
	.set_ofs = 0x041C,
	.clr_ofs = 0x041C,
	.sta_ofs = 0x041C,
};


#define GATE_TOPGEN0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &topckgen0_cg_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
}

#define GATE_TOPGEN1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &topckgen1_cg_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
}

#define GATE_TOPGEN2(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &topckgen2_cg_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
}

#define GATE_TOPGEN3(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &topckgen3_cg_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
}

#define GATE_TOPGEN4(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &topckgen4_cg_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
}

#define GATE_TOPGEN5(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &topckgen5_cg_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
}

#define GATE_TOPGEN6(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &topckgen6_cg_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
}

#define GATE_TOPGEN7(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &topckgen7_cg_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
}

static const struct mtk_gate top_clks[] __initconst = {
	GATE_TOPGEN0(TOPCKGEN_SMI_CG, "smi_cg", "smi_sel", 5),
	GATE_TOPGEN0(TOPCKGEN_DSP_CG, "dsp_cg", "dsp_sel", 7),
	GATE_TOPGEN0(TOPCKGEN_IPU_IF_CG, "ipu_if_cg", "ipu_if_sel", 9),
	GATE_TOPGEN0(TOPCKGEN_SFLASH_CG, "sflash_cg", "sflash_sel", 15),
	GATE_TOPGEN0(TOPCKGEN_PWM_CG, "pwm_cg", "pwm_sel", 17),
	GATE_TOPGEN0(TOPCKGEN_CAMTG_CG, "camtg_cg", "camtg_sel", 19),
	GATE_TOPGEN0(TOPCKGEN_SENINF_CG, "seninf_cg", "seninf_sel", 21),
	GATE_TOPGEN0(TOPCKGEN_SENM0_CG, "senm0_cg", "senm0_sel", 23),
	GATE_TOPGEN0(TOPCKGEN_SENM1_CG, "senm1_cg", "senm1_sel", 25),
	GATE_TOPGEN0(TOPCKGEN_SENM2_CG, "senm2_cg", "senm2_sel", 27),
	GATE_TOPGEN0(TOPCKGEN_SENM3_CG, "senm3_cg", "senm3_sel", 29),
	GATE_TOPGEN0(TOPCKGEN_SENM4_CG, "senm4_cg", "senm4_sel", 31),
	GATE_TOPGEN1(TOPCKGEN_SENM5_CG, "senm5_cg", "senm5_sel", 1),
	GATE_TOPGEN1(TOPCKGEN_SENM6_CG, "senm6_cg", "senm6_sel", 3),
	GATE_TOPGEN1(TOPCKGEN_SENM7_CG, "senm7_cg", "senm7_sel", 5),
	GATE_TOPGEN1(TOPCKGEN_SENM8_CG, "senm8_cg", "senm8_sel", 7),
	GATE_TOPGEN1(TOPCKGEN_I2C_CG, "i2c_cg", "i2c_sel", 9),
	GATE_TOPGEN1(TOPCKGEN_UART_CG, "uart_cg", "uart_sel", 11),
	GATE_TOPGEN1(TOPCKGEN_SPI_CG, "spi_cg", "spi_sel", 13),
	GATE_TOPGEN1(TOPCKGEN_AXI_PERI_CG, "axi_peri_cg", "axi_peri_sel", 15),
	GATE_TOPGEN1(TOPCKGEN_USBH_SYS_CG, "usbh_sys_cg", "usbh_sys_sel", 17),
	GATE_TOPGEN1(TOPCKGEN_USBH_XCH_CG, "usbh_xch_cg", "usbh_xhc_sel", 19),
	GATE_TOPGEN1(TOPCKGEN_USBD_SYS_CG, "usbd_sys_cg", "usbd_sys_sel", 21),
	GATE_TOPGEN1(TOPCKGEN_MSDC_CG, "msdc_cg", "msdc_sel", 23),
	GATE_TOPGEN1(TOPCKGEN_MSDC_HCLK_CG, "msdc_hclk_cg", "msdc50_h_sel",
		    25),
	GATE_TOPGEN1(TOPCKGEN_AES_FDE_CG, "msdc_aes_cg", "aes_fde_sel", 27),
	GATE_TOPGEN1(TOPCKGEN_AUD_INTBUS_CG, "aud_int_cg", "aud_intbus_sel",
		    29),
	GATE_TOPGEN1(TOPCKGEN_A1SYS_HP_CG, "a1sys_cg", "a1sys_hp_sel", 31),
	GATE_TOPGEN2(TOPCKGEN_A2SYS_HP_CG, "a2sys_cg", "a2sys_hp_sel", 1),
	GATE_TOPGEN2(TOPCKGEN_A3SYS_HP_CG, "a3sys_cg", "a3sys_hp_sel", 3),
	GATE_TOPGEN2(TOPCKGEN_TDMIN_CG, "tdmin_cg", "tdmin_sel", 5),
	GATE_TOPGEN2(TOPCKGEN_TDMOUT_CG, "tdmout_cg", "tdmout_sel", 7),
	GATE_TOPGEN2(TOPCKGEN_PMICSPI_CG, "pmicspi_cg", "pmicspi_sel", 11),
	GATE_TOPGEN2(TOPCKGEN_MUTEXGEN_CG, "mutexgen_cg", "mutexgen_sel", 17),
	GATE_TOPGEN2(TOPCKGEN_STC_H27_CG, "stc_h27_cg", "stc_h27_sel", 19),
	GATE_TOPGEN2(TOPCKGEN_MFG_CG, "mfg_cg", "mfg_sel", 23),
	GATE_TOPGEN2(TOPCKGEN_CRYPTO_CG, "crypto_cg", "crypto_sel", 29),
	GATE_TOPGEN2(TOPCKGEN_IMG_S0_CG, "img_s0_cg", "img_s0_sel", 31),
	GATE_TOPGEN3(TOPCKGEN_IMG_S1_CG, "img_s1_cg", "img_s1_sel", 1),
	GATE_TOPGEN3(TOPCKGEN_MMSYS_COM_CG, "mmsys_com_cg", "mmsys_com_sel", 3),
	GATE_TOPGEN3(TOPCKGEN_FM_CG, "fm_cg", "fm_sel", 5),
	GATE_TOPGEN3(TOPCKGEN_MMSYS_CORE_CG, "mmsys_core_cg", "mmsys_core_sel",
		    7),
	GATE_TOPGEN3(TOPCKGEN_MM_SYSRAM0_CG, "mmsys_ram0_cg", "mm_sysram0_sel",
		    9),
	GATE_TOPGEN3(TOPCKGEN_MM_SYSRAM1_CG, "mmsys_ram1_cg", "mm_sysram1_sel",
		    11),
	GATE_TOPGEN3(TOPCKGEN_MM_SYSRAM2_CG, "mmsys_ram2_cg", "mm_sysram2_sel",
		    13),
	GATE_TOPGEN3(TOPCKGEN_SENINF_GAZE_CG, "seninf_gaze_cg",
		    "seninf_gaze_sel", 23),
	GATE_TOPGEN3(TOPCKGEN_CAM_S0_CG, "cam_s0_cg", "cam_s0_sel", 25),
	GATE_TOPGEN3(TOPCKGEN_CAM_S1_CG, "cam_s1_cg", "cam_s1_sel", 27),
	GATE_TOPGEN3(TOPCKGEN_CAM_GZ0_CG, "cam_gz0_cg", "cam_gz0_sel", 29),
	GATE_TOPGEN3(TOPCKGEN_CAM_GZ1_CG, "cam_gz1_cg", "cam_gz1_sel", 31),
	GATE_TOPGEN4(TOPCKGEN_MM_SYNCGEN_CG, "mm_syncgen_cg", "mm_syncgen_sel",
		    1),
	GATE_TOPGEN4(TOPCKGEN_ATB_CG, "atb_cg", "atb_sel", 7),
	GATE_TOPGEN4(TOPCKGEN_DISPPWM_CG, "disppwm_cg", "disppwm_sel", 11),
	GATE_TOPGEN4(TOPCKGEN_PMICGSPI_CG, "pmicgspi_cg", "pmicgspi_sel", 17),
	GATE_TOPGEN4(TOPCKGEN_DDDS_CG, "ddds_cg", "ddds_sys_sel", 19),
	GATE_TOPGEN4(TOPCKGEN_DSP1_CG, "dsp1_cg", "dsp1_sel", 21),
	GATE_TOPGEN4(TOPCKGEN_DSP2_CG, "dsp2_cg", "dsp2_sel", 23),
	GATE_TOPGEN4(TOPCKGEN_DSP3_CG, "dsp3_cg", "dsp3_sel", 25),
	GATE_TOPGEN4(TOPCKGEN_HEADER_SYS_CG, "headers_sys_cg",
		    "header_sys_sel", 27),
	GATE_TOPGEN4(TOPCKGEN_DSI_REF_CG, "dsi_ref_cg", "dsi_ref_sel", 29),
	GATE_TOPGEN4(TOPCKGEN_SLICER_VID_CG, "slicer_vid_cg", "slicer_vid_sel",
			30),
	GATE_TOPGEN4(TOPCKGEN_SLICER_DSC_CG, "slicer_dsc_cg", "slicer_dsc_sel",
			31),
	GATE_TOPGEN5(TOPCKGEN_TPIU_CG, "tpiu_cg", "tpiu_sel", 31),
	GATE_TOPGEN6(TOPCKGEN_DDDS_VSP_CG, "ddds_vsp_cg", "ddds_vsp_sel", 1),
	GATE_TOPGEN6(TOPCKGEN_SCP_P_CK0_CG, "scp_p_ck0_cg", "clk26m", 2),
	GATE_TOPGEN6(TOPCKGEN_SCP_P_CK1_CG, "scp_p_ck1_cg", "clk26m", 3),
	GATE_TOPGEN6(TOPCKGEN_SCP_P_CK2_CG, "scp_p_ck2_cg", "clk26m", 4),
	GATE_TOPGEN7(TOPCKGEN_SSUB_TOP_CK_EN, "ssub_top_ck_en", "clk26m",
		    0),
	GATE_TOPGEN7(TOPCKGEN_SSUB_PHY_CK_EN, "ssub_phy_ck_en", "clk26m",
		    1),
	GATE_TOPGEN7(TOPCKGEN_SSUB_TOP_CK_EN_IP2, "ssub_top_ck_en_ip2",
		     "clk26m", 3),
	GATE_TOPGEN7(TOPCKGEN_SSUB_PHY_CK_EN_IP2, "ssub_phy_ck_ip2", "clk26m",
		    4),
	GATE_TOPGEN7(TOPCKGEN_F26M_DP_REF_CK_EN, "f26m_dp_ref_ck_en", "clk26m",
		    9),
};

static void __init mtk_topckgen_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	base = of_iomap(node, 0);
	if (!base) {
		pr_err("%s(): ioremap failed\n", __func__);
		return;
	}

	clk_data = mtk_alloc_clk_data(CLK_TOP_CK_NR);
	mtk_clk_register_factors(top_fixed_divs, ARRAY_SIZE(top_fixed_divs),
				clk_data);
	mtk_clk_register_composites(top_muxes, ARRAY_SIZE(top_muxes),
				   base, &lock, clk_data);

	mtk_clk_register_gates(node, top_clks, ARRAY_SIZE(top_clks),
			      clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}
CLK_OF_DECLARE(mtk_topckgen, "mediatek,mt3612-topckgen", mtk_topckgen_init);

static const struct mtk_gate_regs infra_cg0_regs __initconst = {
	.set_ofs = 0x0080,
	.clr_ofs = 0x0084,
	.sta_ofs = 0x0090,
};

static const struct mtk_gate_regs infra_cg1_regs __initconst = {
	.set_ofs = 0x0088,
	.clr_ofs = 0x008C,
	.sta_ofs = 0x0094,
};

static const struct mtk_gate_regs infra_cg2_regs __initconst = {
	.set_ofs = 0x00B0,
	.clr_ofs = 0x00B4,
	.sta_ofs = 0x00B8,
};

#define GATE_ICG0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &infra_cg0_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

#define GATE_ICG1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &infra_cg1_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate infra_clks[] __initconst = {
	GATE_ICG0(INFRACFG_AO_PMIC_CG_TMR, "pmic_tmr", "pmicspi_cg", 0),
	GATE_ICG0(INFRACFG_AO_PMIC_CG_AP, "pmic_ap", "pmicspi_cg", 1),
	GATE_ICG0(INFRACFG_AO_PMIC_CG_MD, "pmic_md", "pmicspi_cg", 2),
	GATE_ICG0(INFRACFG_AO_PMIC_CG_CONN, "pmic_conn", "clk26m", 3),
	GATE_ICG0(INFRACFG_AO_SCP_CG, "scp_ck", "scp_sel", 4),
	GATE_ICG0(INFRACFG_AO_SEJ_CG, "sej_ck", "clk26m", 5),
	GATE_ICG0(INFRACFG_AO_APXGPT_CG, "apxgtp_ck", "clk26m", 6),
	GATE_ICG0(INFRACFG_AO_DVFSRC_CG, "dvfsrc_ck", "clk26m", 7),
	GATE_ICG0(INFRACFG_AO_IO_TIMER_CG, "io_timer_ck", "clk26m", 11),
	GATE_ICG0(INFRACFG_AO_DEVAPC_MPU_CG, "devapc_mcu_ck", "clk26m", 20),
	GATE_ICG0(INFRACFG_AO_FHCTL_CG, "fhctl_ck", "clk26m", 30),
	GATE_ICG0(INFRACFG_AO_PMIC_GSPI_CG, "pmic_gspi_ck", "clk26m", 31),
	GATE_ICG1(INFRACFG_AO_AUXADC_CG, "auxadc_ck", "clk26m", 10),
	GATE_ICG1(INFRACFG_AO_DEVICE_APC_CG, "devapc_ck", "clk26m", 20),
	GATE_ICG1(INFRACFG_AO_SMI_L2C_CG, "smi_l2c_ck", "clk26m", 22),
	GATE_ICG1(INFRACFG_AO_AUDIO_CG, "audio_ck", "aud_int_cg", 25),
	GATE_ICG1(INFRACFG_AO_DRAMC_F26M_CG, "dramc_f26_ck", "clk26m", 31),
};

static const struct mtk_fixed_factor infra_fixed_divs[] __initconst = {
	FACTOR(INFRACFG_AO_CLK_13M, "clk13m", "clk26m", 1, 2),
};

static void __init mtk_infrasys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(INFRACFG_AO_NR);

	mtk_clk_register_gates(node, infra_clks, ARRAY_SIZE(infra_clks),
			      clk_data);
	mtk_clk_register_factors(infra_fixed_divs,
				ARRAY_SIZE(infra_fixed_divs), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);

	mtk_register_reset_controller(node, 3, MT3612_INFRACFG_OFS);
}
CLK_OF_DECLARE(mtk_infrasys, "mediatek,mt3612-infracfg", mtk_infrasys_init);

static const struct mtk_gate_regs peri0_cg_regs __initconst = {
	.set_ofs = 0x0270,
	.clr_ofs = 0x0274,
	.sta_ofs = 0x0278,
};

static const struct mtk_gate_regs peri1_cg_regs __initconst = {
	.set_ofs = 0x0280,
	.clr_ofs = 0x0284,
	.sta_ofs = 0x0288,
};

static const struct mtk_gate_regs peri2_cg_regs __initconst = {
	.set_ofs = 0x0290,
	.clr_ofs = 0x0294,
	.sta_ofs = 0x0298,
};

static const struct mtk_gate_regs peri3_cg_regs __initconst = {
	.set_ofs = 0x02A0,
	.clr_ofs = 0x02A4,
	.sta_ofs = 0x02A8,
};

static const struct mtk_gate_regs peri4_cg_regs __initconst = {
	.set_ofs = 0x02B0,
	.clr_ofs = 0x02B4,
	.sta_ofs = 0x02B8,
};

static const struct mtk_gate_regs peri5_cg_regs __initconst = {
	.set_ofs = 0x02C0,
	.clr_ofs = 0x02C4,
	.sta_ofs = 0x02C8,
};



#define GATE_PERI0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &peri0_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
}

#define GATE_PERI1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &peri1_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
}

#define GATE_PERI2(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &peri2_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
}

#define GATE_PERI3(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &peri3_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
}


#define GATE_PERI4(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &peri4_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
}

#define GATE_PERI5(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &peri5_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
}

static const char * const uart_ck_sel_parents[] __initconst = {
	"clk26m_ck",
	"uart_cg",
};

static const char * const i2c_ck_sel_parents[] __initconst = {
	"clk66m_ck",
	"i2c_cg",
};

static const struct mtk_composite peri_clks[] __initconst = {
	MUX(PERISYS_UART0_SEL, "uart0_ck_sel", uart_ck_sel_parents, 0x2f0, 0,
	   1),
	MUX(PERISYS_UART1_SEL, "uart1_ck_sel", uart_ck_sel_parents, 0x2f0, 1,
	   1),
	MUX(PERISYS_UART2_SEL, "uart2_ck_sel", uart_ck_sel_parents, 0x2f0, 2,
	   1),
	MUX(PERISYS_UART3_SEL, "uart3_ck_sel", uart_ck_sel_parents, 0x2f0, 3,
	   1),
	MUX(PERISYS_UART4_SEL, "uart4_ck_sel", uart_ck_sel_parents, 0x2f0, 4,
	   1),
	MUX(PERISYS_UART5_SEL, "uart5_ck_sel", uart_ck_sel_parents, 0x2f0, 5,
	   1),
	MUX(PERISYS_UART6_SEL, "uart6_ck_sel", uart_ck_sel_parents, 0x2f0, 6,
	   1),
	MUX(PERISYS_UART7_SEL, "uart7_ck_sel", uart_ck_sel_parents, 0x2f0, 7,
	   1),
	MUX(PERISYS_UART8_SEL, "uart8_ck_sel", uart_ck_sel_parents, 0x2f0, 8,
	   1),
	MUX(PERISYS_UART9_SEL, "uart9_ck_sel", uart_ck_sel_parents, 0x2f0, 9,
	   1),
	MUX(PERISYS_I2C_SEL, "i2c_ck_sel", i2c_ck_sel_parents, 0x2f0, 18, 1),
};


static const struct mtk_gate peri_gates[] __initconst = {
	GATE_PERI0(PERISYS_I2C9_CK_PDN, "i2c9_ck", "i2c_ck_sel", 25),
	GATE_PERI0(PERISYS_I2C8_CK_PDN, "i2c8_ck", "i2c_ck_sel", 24),
	GATE_PERI0(PERISYS_I2C7_CK_PDN, "i2c7_ck", "i2c_ck_sel", 23),
	GATE_PERI0(PERISYS_I2C6_CK_PDN, "i2c6_ck", "i2c_ck_sel", 22),
	GATE_PERI0(PERISYS_I2C5_CK_PDN, "i2c5_ck", "i2c_ck_sel", 21),
	GATE_PERI0(PERISYS_I2C4_CK_PDN, "i2c4_ck", "i2c_ck_sel", 20),
	GATE_PERI0(PERISYS_I2C3_CK_PDN, "i2c3_ck", "i2c_ck_sel", 19),
	GATE_PERI0(PERISYS_I2C2_CK_PDN, "i2c2_ck", "i2c_ck_sel", 18),
	GATE_PERI0(PERISYS_I2C1_CK_PDN, "i2c1_ck", "i2c_ck_sel", 17),
	GATE_PERI0(PERISYS_I2C0_CK_PDN, "i2c0_ck", "i2c_ck_sel", 16),
	GATE_PERI0(PERISYS_PWM15_CK_PDN, "pwm15_ck", "pwm_ck", 15),
	GATE_PERI0(PERISYS_PWM14_CK_PDN, "pwm14_ck", "pwm_ck", 14),
	GATE_PERI0(PERISYS_PWM13_CK_PDN, "pwm13_ck", "pwm_ck", 13),
	GATE_PERI0(PERISYS_PWM12_CK_PDN, "pwm12_ck", "pwm_ck", 12),
	GATE_PERI0(PERISYS_PWM11_CK_PDN, "pwm11_ck", "pwm_ck", 11),
	GATE_PERI0(PERISYS_PWM10_CK_PDN, "pwm10_ck", "pwm_ck", 10),
	GATE_PERI0(PERISYS_PWM9_CK_PDN, "pwm9_ck", "pwm_ck", 9),
	GATE_PERI0(PERISYS_PWM8_CK_PDN, "pwm8_ck", "pwm_ck", 8),
	GATE_PERI0(PERISYS_PWM7_CK_PDN, "pwm7_ck", "pwm_ck", 7),
	GATE_PERI0(PERISYS_PWM6_CK_PDN, "pwm6_ck", "pwm_ck", 6),
	GATE_PERI0(PERISYS_PWM5_CK_PDN, "pwm5_ck", "pwm_ck", 5),
	GATE_PERI0(PERISYS_PWM4_CK_PDN, "pwm4_ck", "pwm_ck", 4),
	GATE_PERI0(PERISYS_PWM3_CK_PDN, "pwm3_ck", "pwm_ck", 3),
	GATE_PERI0(PERISYS_PWM2_CK_PDN, "pwm2_ck", "pwm_ck", 2),
	GATE_PERI0(PERISYS_PWM1_CK_PDN, "pwm1_ck", "pwm_ck", 1),
	GATE_PERI0(PERISYS_PWM_CK_PDN, "pwm_ck", "pwm_cg", 0),
	GATE_PERI1(PERISYS_SPI9_CK_PDN, "spi9_ck", "spi_cg", 25),
	GATE_PERI1(PERISYS_SPI8_CK_PDN, "spi8_ck", "spi_cg", 24),
	GATE_PERI1(PERISYS_SPI7_CK_PDN, "spi7_ck", "spi_cg", 23),
	GATE_PERI1(PERISYS_SPI6_CK_PDN, "spi6_ck", "spi_cg", 22),
	GATE_PERI1(PERISYS_SPI5_CK_PDN, "spi5_ck", "spi_cg", 21),
	GATE_PERI1(PERISYS_SPI4_CK_PDN, "spi4_ck", "spi_cg", 20),
	GATE_PERI1(PERISYS_SPI3_CK_PDN, "spi3_ck", "spi_cg", 19),
	GATE_PERI1(PERISYS_SPI2_CK_PDN, "spi2_ck", "spi_cg", 18),
	GATE_PERI1(PERISYS_SPI1_CK_PDN, "spi1_ck", "spi_cg", 17),
	GATE_PERI1(PERISYS_SPI0_CK_PDN, "spi0_ck", "spi_cg", 16),
	GATE_PERI1(PERISYS_UART9_CK_PDN, "uart9_ck", "uart9_ck_sel", 9),
	GATE_PERI1(PERISYS_UART8_CK_PDN, "uart8_ck", "uart8_ck_sel", 8),
	GATE_PERI1(PERISYS_UART7_CK_PDN, "uart7_ck", "uart7_ck_sel", 7),
	GATE_PERI1(PERISYS_UART6_CK_PDN, "uart6_ck", "uart6_ck_sel", 6),
	GATE_PERI1(PERISYS_UART5_CK_PDN, "uart5_ck", "uart5_ck_sel", 5),
	GATE_PERI1(PERISYS_UART4_CK_PDN, "uart4_ck", "uart4_ck_sel", 4),
	GATE_PERI1(PERISYS_UART3_CK_PDN, "uart3_ck", "uart3_ck_sel", 3),
	GATE_PERI1(PERISYS_UART2_CK_PDN, "uart2_ck", "uart2_ck_sel", 2),
	GATE_PERI1(PERISYS_UART1_CK_PDN, "uart1_ck", "uart1_ck_sel", 1),
	GATE_PERI1(PERISYS_UART0_CK_PDN, "uart0_ck", "uart0_ck_sel", 0),
	GATE_PERI2(PERISYS_MSDC0_AP_CK_PDN, "msdc0_ck", "msdc_cg", 26),
	GATE_PERI2(PERISYS_FLASH_CK_PDN, "sflash_ck", "sflash_cg", 16),
	GATE_PERI3(PERISYS_USB_CK_PDN, "usb_px_ck", "usbh_xch_cg", 0),
	GATE_PERI4(PERISYS_DEBUGTOP_CK_PDN, "debugtop_ck", "clk26m", 20),
	GATE_PERI4(PERISYS_DEV_APC_CK_PDN, "dev_apc_ck", "clk26m", 9),
	GATE_PERI4(PERISYS_CQ_DMA_CK_PDN, "cq_dma_ck", "clk26m", 6),
	GATE_PERI4(PERISYS_DISP_PWM1_CK_PDN, "disp_pwm1_ck", "disppwm_cg", 4),
	GATE_PERI4(PERISYS_DISP_PWM0_CK_PDN, "disp_pwm0_ck", "disppwm_cg", 3),
	GATE_PERI4(PERISYS_AP_DMA_CK_PDN, "ap_dma_ck", "clk26m", 0),
	GATE_PERI5(PERISYS_PWM32_CK_PDN, "pwm32_ck", "pwm_ck", 31),
	GATE_PERI5(PERISYS_PWM31_CK_PDN, "pwm31_ck", "pwm_ck", 30),
	GATE_PERI5(PERISYS_PWM30_CK_PDN, "pwm30_ck", "pwm_ck", 29),
	GATE_PERI5(PERISYS_PWM29_CK_PDN, "pwm29_ck", "pwm_ck", 28),
	GATE_PERI5(PERISYS_PWM28_CK_PDN, "pwm28_ck", "pwm_ck", 27),
	GATE_PERI5(PERISYS_PWM27_CK_PDN, "pwm27_ck", "pwm_ck", 26),
	GATE_PERI5(PERISYS_PWM26_CK_PDN, "pwm26_ck", "pwm_ck", 25),
	GATE_PERI5(PERISYS_PWM25_CK_PDN, "pwm25_ck", "pwm_ck", 24),
	GATE_PERI5(PERISYS_PWM24_CK_PDN, "pwm24_ck", "pwm_ck", 23),
	GATE_PERI5(PERISYS_PWM23_CK_PDN, "pwm23_ck", "pwm_ck", 22),
	GATE_PERI5(PERISYS_PWM22_CK_PDN, "pwm22_ck", "pwm_ck", 21),
	GATE_PERI5(PERISYS_PWM21_CK_PDN, "pwm21_ck", "pwm_ck", 20),
	GATE_PERI5(PERISYS_PWM20_CK_PDN, "pwm20_ck", "pwm_ck", 19),
	GATE_PERI5(PERISYS_PWM19_CK_PDN, "pwm19_ck", "pwm_ck", 18),
	GATE_PERI5(PERISYS_PWM18_CK_PDN, "pwm18_ck", "pwm_ck", 17),
	GATE_PERI5(PERISYS_PWM17_CK_PDN, "pwm17_ck", "pwm_ck", 16),
	GATE_PERI5(PERISYS_PWM16_CK_PDN, "pwm16_ck", "pwm_ck", 15),
	GATE_PERI5(PERISYS_I2C19_CK_PDN, "i2c19_ck", "i2c_ck_sel", 13),
	GATE_PERI5(PERISYS_I2C18_CK_PDN, "i2c18_ck", "i2c_ck_sel", 12),
	GATE_PERI5(PERISYS_I2C17_CK_PDN, "i2c17_ck", "i2c_ck_sel", 11),
	GATE_PERI5(PERISYS_I2C16_CK_PDN, "i2c16_ck", "i2c_ck_sel", 10),
	GATE_PERI5(PERISYS_I2C15_CK_PDN, "i2c15_ck", "i2c_ck_sel", 9),
	GATE_PERI5(PERISYS_I2C14_CK_PDN, "i2c14_ck", "i2c_ck_sel", 8),
	GATE_PERI5(PERISYS_I2C13_CK_PDN, "i2c13_ck", "i2c_ck_sel", 7),
	GATE_PERI5(PERISYS_I2C12_CK_PDN, "i2c12_ck", "i2c_ck_sel", 6),
	GATE_PERI5(PERISYS_I2C11_CK_PDN, "i2c11_ck", "i2c_ck_sel", 5),
	GATE_PERI5(PERISYS_I2C10_CK_PDN, "i2c10_ck", "i2c_ck_sel", 4),
};

static void __init mtk_pericfg_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	base = of_iomap(node, 0);
	if (!base) {
		pr_err("%s(): ioremap failed\n", __func__);
		return;
	}

	clk_data = mtk_alloc_clk_data(PERISYS_NR);

	mtk_clk_register_gates(node, peri_gates, ARRAY_SIZE(peri_gates),
						clk_data);
	mtk_clk_register_composites(peri_clks, ARRAY_SIZE(peri_clks), base,
			&lock, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);

	mtk_register_reset_controller(node, 5, MT3612_PERICFG_OFS);
}
CLK_OF_DECLARE(mtk_pericfg, "mediatek,mt3612-pericfg", mtk_pericfg_init);

static const struct mtk_gate_regs mmsys_core_cg0_regs __initconst = {
	.set_ofs = 0x0000,
	.clr_ofs = 0x0000,
	.sta_ofs = 0x0000,
};

static const struct mtk_gate_regs mmsys_core_cg1_regs __initconst = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0004,
	.sta_ofs = 0x0004,
};

static const struct mtk_gate_regs mmsys_core_cg2_regs __initconst = {
	.set_ofs = 0x0008,
	.clr_ofs = 0x0008,
	.sta_ofs = 0x0008,
};

#define GATE_MM_CORE0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &mmsys_core_cg0_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr,	\
	}

#define GATE_MM_CORE1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &mmsys_core_cg1_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr,	\
}

#define GATE_MM_CORE2(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &mmsys_core_cg2_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr,	\
}

static const struct mtk_gate mm_core_clks[] __initconst = {
	GATE_MM_CORE0(MMSYS_CORE_SLCR0, "mmsys_core_slrc0", "mmsys_core_cg", 0),
	GATE_MM_CORE0(MMSYS_CORE_SLCR1, "mmsys_core_slrc1", "mmsys_core_cg", 1),
	GATE_MM_CORE0(MMSYS_CORE_SLCR2, "mmsys_core_slrc2", "mmsys_core_cg", 2),
	GATE_MM_CORE0(MMSYS_CORE_SLCR3, "mmsys_core_slrc3", "mmsys_core_cg", 3),
	GATE_MM_CORE0(MMSYS_CORE_MDP_RDMA0, "mmsys_core_mdp_rdma0",
			"mmsys_core_cg", 4),
	GATE_MM_CORE0(MMSYS_CORE_MDP_RDMA1, "mmsys_core_mdp_rdma1",
			"mmsys_core_cg", 5),
	GATE_MM_CORE0(MMSYS_CORE_MDP_RDMA2, "mmsys_core_mdp_rdma2",
			"mmsys_core_cg", 6),
	GATE_MM_CORE0(MMSYS_CORE_MDP_RDMA3, "mmsys_core_mdp_rdma3",
			"mmsys_core_cg", 7),
	GATE_MM_CORE0(MMSYS_CORE_MDP_PVRIC0, "mmsys_core_mdp_pvric0",
			"mmsys_core_cg", 8),
	GATE_MM_CORE0(MMSYS_CORE_MDP_PVRIC1, "mmsys_core_mdp_pvric1",
			"mmsys_core_cg", 9),
	GATE_MM_CORE0(MMSYS_CORE_MDP_PVRIC2, "mmsys_core_mdp_pvric2",
			"mmsys_core_cg", 10),
	GATE_MM_CORE0(MMSYS_CORE_MDP_PVRIC3, "mmsys_core_mdp_pvric3",
			"mmsys_core_cg", 11),
	GATE_MM_CORE0(MMSYS_CORE_DISP_RDMA0, "mmsys_core_disp_rdma0",
			"mmsys_core_cg", 12),
	GATE_MM_CORE0(MMSYS_CORE_DISP_RDMA1, "mmsys_core_disp_rdma1",
			"mmsys_core_cg", 13),
	GATE_MM_CORE0(MMSYS_CORE_DISP_RDMA2, "mmsys_core_disp_rdma2",
			"mmsys_core_cg", 14),
	GATE_MM_CORE0(MMSYS_CORE_DISP_RDMA3, "mmsys_core_disp_rdma3",
			"mmsys_core_cg", 15),
	GATE_MM_CORE0(MMSYS_CORE_DP_PAT_GEN0, "mmsys_core_dp_pat_gen0",
			"mmsys_core_cg", 16),
	GATE_MM_CORE0(MMSYS_CORE_DP_PAT_GEN1, "mmsys_core_dp_pat_gen1",
			"mmsys_core_cg", 17),
	GATE_MM_CORE0(MMSYS_CORE_DP_PAT_GEN2, "mmsys_core_dp_pat_gen2",
			"mmsys_core_cg", 18),
	GATE_MM_CORE0(MMSYS_CORE_DP_PAT_GEN3, "mmsys_core_dp_pat_gen3",
			"mmsys_core_cg", 19),
	GATE_MM_CORE0(MMSYS_CORE_LHC0, "mmsys_core_lhc0", "mmsys_core_cg", 20),
	GATE_MM_CORE0(MMSYS_CORE_LHC1, "mmsys_core_lhc1", "mmsys_core_cg", 21),
	GATE_MM_CORE0(MMSYS_CORE_LHC2, "mmsys_core_lhc2", "mmsys_core_cg", 22),
	GATE_MM_CORE0(MMSYS_CORE_LHC3, "mmsys_core_lhc3", "mmsys_core_cg", 23),
	GATE_MM_CORE0(MMSYS_CORE_DISP_WDMA0, "mmsys_core_disp_wdma0",
			"mmsys_core_cg", 24),
	GATE_MM_CORE0(MMSYS_CORE_DISP_WDMA1, "mmsys_core_disp_wdma1",
			"mmsys_core_cg", 25),
	GATE_MM_CORE0(MMSYS_CORE_DISP_WDMA2, "mmsys_core_disp_wdma2",
			"mmsys_core_cg", 26),
	GATE_MM_CORE0(MMSYS_CORE_DISP_WDMA3, "mmsys_core_disp_wdma3",
			"mmsys_core_cg", 27),
	GATE_MM_CORE0(MMSYS_CORE_DSC_MOUNT0, "mmsys_core_dsc_mout0",
			"mmsys_core_cg", 28),
	GATE_MM_CORE0(MMSYS_CORE_DSC_MOUNT1, "mmsys_core_dsc_mout1",
			"mmsys_core_cg", 29),
	GATE_MM_CORE0(MMSYS_CORE_DSC_MOUNT2, "mmsys_core_dsc_mout2",
			"mmsys_core_cg", 30),
	GATE_MM_CORE0(MMSYS_CORE_DSC_MOUNT3, "mmsys_core_dsc_mout3",
			"mmsys_core_cg", 31),
	GATE_MM_CORE1(MMSYS_CORE_DSI0_MM, "mmsys_core_dsi0_mm",
			"mmsys_core_cg", 0),
	GATE_MM_CORE1(MMSYS_CORE_DSI1_MM, "mmsys_core_dsi1_mm",
			"mmsys_core_cg", 1),
	GATE_MM_CORE1(MMSYS_CORE_DSI2_MM, "mmsys_core_dsi2_mm",
			"mmsys_core_cg", 2),
	GATE_MM_CORE1(MMSYS_CORE_DSI3_MM, "mmsys_core_dsi3_mm",
			"mmsys_core_cg", 3),
	GATE_MM_CORE1(MMSYS_CORE_DSI0_DIG, "mmsys_core_dsi0_dig",
			"mmsys_core_cg", 4),
	GATE_MM_CORE1(MMSYS_CORE_DSI1_DIG, "mmsys_core_dsi1_dig",
			"mmsys_core_cg", 5),
	GATE_MM_CORE1(MMSYS_CORE_DSI2_DIG, "mmsys_core_dsi2_dig",
			"mmsys_core_cg", 6),
	GATE_MM_CORE1(MMSYS_CORE_DSI3_DIG, "mmsys_core_dsi3_dig",
			"mmsys_core_cg", 7),
	GATE_MM_CORE1(MMSYS_CORE_CROP0, "mmsys_core_crop0", "mmsys_core_cg", 8),
	GATE_MM_CORE1(MMSYS_CORE_CROP1, "mmsys_core_crop1", "mmsys_core_cg", 9),
	GATE_MM_CORE1(MMSYS_CORE_CROP2, "mmsys_core_crop2",
			"mmsys_core_cg", 10),
	GATE_MM_CORE1(MMSYS_CORE_CROP3, "mmsys_core_crop3",
			"mmsys_core_cg", 11),
	GATE_MM_CORE1(MMSYS_CORE_RDMA_MOUT0, "mmsys_core_rdma_mout0",
			"mmsys_core_cg", 12),
	GATE_MM_CORE1(MMSYS_CORE_RDMA_MOUT1, "mmsys_core_rdma_mout1",
			"mmsys_core_cg", 13),
	GATE_MM_CORE1(MMSYS_CORE_RDMA_MOUT2, "mmsys_core_rdma_mout2",
			"mmsys_core_cg", 14),
	GATE_MM_CORE1(MMSYS_CORE_RDMA_MOUT3, "mmsys_core_rdma_mout3",
			"mmsys_core_cg", 15),
	GATE_MM_CORE1(MMSYS_CORE_RSZ0, "mmsys_core_rsz0", "mmsys_core_cg", 16),
	GATE_MM_CORE1(MMSYS_CORE_RSZ1, "mmsys_core_rsz1", "mmsys_core_cg", 17),
	GATE_MM_CORE1(MMSYS_CORE_RSZ_MOUT0, "mmsys_core_rsz_mout0",
			"mmsys_core_cg", 18),
	GATE_MM_CORE1(MMSYS_CORE_P2S0, "mmsys_core_p2s0", "mmsys_core_cg", 19),
	GATE_MM_CORE1(MMSYS_CORE_DSC0, "mmsys_core_dsc0", "mmsys_core_cg", 20),
	GATE_MM_CORE1(MMSYS_CORE_DSC1, "mmsys_core_dsc1", "mmsys_core_cg", 21),
	GATE_MM_CORE1(MMSYS_CORE_DSI_LANE_SWAP, "mmsys_core_dsi_lane_swap",
			"mmsys_core_cg", 22),
	GATE_MM_CORE1(MMSYS_CORE_SLICER_MM, "mmsys_core_slicer_mm",
			"mmsys_core_cg", 23),
	GATE_MM_CORE1(MMSYS_CORE_SLICER_VID, "mmsys_core_slicer_vid",
			"slicer_vid_cg", 24),
	GATE_MM_CORE1(MMSYS_CORE_SLICER_DSC, "mmsys_core_slicer_dsc",
			"slicer_dsc_cg", 25),
	GATE_MM_CORE1(MMSYS_CORE_LHC_SWAP, "mmsys_core_lhc_swap",
			"mmsys_core_cg", 26),
	GATE_MM_CORE2(MMSYS_CORE_SMI_LARB0, "mmsys_core_smi_larb0",
			"mmsys_core_cg", 0),
	GATE_MM_CORE2(MMSYS_CORE_SMI_LARB1, "mmsys_core_smi_larb1",
			"mmsys_core_cg", 1),
	GATE_MM_CORE2(MMSYS_CORE_SMI_LARB15, "mmsys_core_smi_larb15",
			"mmsys_core_cg", 2),
	GATE_MM_CORE2(MMSYS_CORE_SMI_LARB16, "mmsys_core_smi_larb16",
			"mmsys_core_cg", 3),
	GATE_MM_CORE2(MMSYS_CORE_LARB0_GALS, "mmsys_core_larb0_gals",
			"mmsys_core_cg", 4),
	GATE_MM_CORE2(MMSYS_CORE_LARB1_GALS, "mmsys_core_larb1_gals",
			"mmsys_core_cg", 5),
	GATE_MM_CORE2(MMSYS_CORE_LARB15_GALS, "mmsys_core_larb15_gals",
			"mmsys_core_cg", 6),
	GATE_MM_CORE2(MMSYS_CORE_LARB16_GALS, "mmsys_core_larb16_gals",
			"mmsys_core_cg", 7),
	GATE_MM_CORE2(MMSYS_CORE_RBFC0, "mmsys_core_rbfc0", "mmsys_core_cg", 8),
	GATE_MM_CORE2(MMSYS_CORE_RBFC1, "mmsys_core_rbfc1", "mmsys_core_cg", 9),
	GATE_MM_CORE2(MMSYS_CORE_RBFC2, "mmsys_core_rbfc2",
			"mmsys_core_cg", 10),
	GATE_MM_CORE2(MMSYS_CORE_RBFC3, "mmsys_core_rbfc3",
			"mmsys_core_cg", 11),
	GATE_MM_CORE2(MMSYS_CORE_SBRC, "mmsys_core_sbrc", "mmsys_core_cg", 12),
	GATE_MM_CORE2(MMSYS_CORE_MM_FAKE_ENG, "mmsys_core_mm_fake_eng",
			"mmsys_core_cg", 13),
	GATE_MM_CORE2(MMSYS_CORE_EVENT_TX, "mmsys_core_event_tx",
			"mmsys_core_cg", 14),
	GATE_MM_CORE2(MMSYS_CORE_CAMERA_SYNC, "mmsys_core_camera_sync",
			"mm_syncgen_cg", 15),
	GATE_MM_CORE2(MMSYS_CORE_MUTEX, "mmsys_core_mutex",
			"mmsys_core_cg", 16),
	GATE_MM_CORE2(MMSYS_CORE_MUTEX_26M, "mmsys_core_mutex_26m",
			"mutexgen_cg", 17),
	GATE_MM_CORE2(MMSYS_CORE_CRC, "mmsys_core_crc", "mmsys_core_cg", 18),
};

static void __init mtk_mmsys_core_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(MMSYS_CORE_NR);

	mtk_clk_register_gates(node, mm_core_clks, ARRAY_SIZE(mm_core_clks),
						clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}
CLK_OF_DECLARE(mtk_mmsys_core, "mediatek,mt3612-mmsys-core",
	      mtk_mmsys_core_init);

static const struct mtk_gate_regs mmsys_com_cg0_regs __initconst = {
	.set_ofs = 0x0104,
	.clr_ofs = 0x0108,
	.sta_ofs = 0x0100,
};

static const struct mtk_gate_regs mmsys_com_cg1_regs __initconst = {
	.set_ofs = 0x0114,
	.clr_ofs = 0x0118,
	.sta_ofs = 0x0110,
};

static const struct mtk_gate_regs mmsys_com_cg2_regs __initconst = {
	.set_ofs = 0x0144,
	.clr_ofs = 0x0148,
	.sta_ofs = 0x0140,
};

#define GATE_MMSYS_CMM0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &mmsys_com_cg0_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

#define GATE_MMSYS_CMM1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &mmsys_com_cg1_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
}

#define GATE_MMSYS_CMM2(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &mmsys_com_cg2_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
}

static const struct mtk_gate mm_cmm_clks[] __initconst = {
	GATE_MMSYS_CMM0(MMSYS_CMM_PADDING0, "mmsys_cmm_pad0",
			"mmsys_com_cg", 0),
	GATE_MMSYS_CMM0(MMSYS_CMM_PADDING1, "mmsys_cmm_pad1",
			"mmsys_com_cg", 1),
	GATE_MMSYS_CMM0(MMSYS_CMM_PADDING2, "mmsys_cmm_pad2",
			"mmsys_com_cg", 2),
	GATE_MMSYS_CMM0(MMSYS_CMM_MDP_WPE_1_tx, "mmsys_cmm_mdp_wpe_1_tx",
		       "mmsys_com_cg", 3),
	GATE_MMSYS_CMM0(MMSYS_CMM_MDP_WPE_1_1_tx, "mmsys_cmm_mdp_wpe_1_1_tx",
		       "mmsys_com_cg", 4),
	GATE_MMSYS_CMM0(MMSYS_CMM_MDP_WPE_1_2_tx, "mmsys_cmm_mdp_wpe_1_2_tx",
		       "mmsys_com_cg", 5),
	GATE_MMSYS_CMM0(MMSYS_CMM_MDP_WPE_1, "mmsys_cmm_mdp_wpe_1",
		       "mmsys_com_cg", 6),
	GATE_MMSYS_CMM0(MMSYS_CMM_MDP_FE, "mmsys_cmm_mdp_fe",
		       "mmsys_com_cg", 7),
	GATE_MMSYS_CMM0(MMSYS_CMM_WDMA_0, "mmsys_cmm_wdma_0",
			"mmsys_com_cg", 8),
	GATE_MMSYS_CMM0(MMSYS_CMM_WDMA_1, "mmsys_cmm_wdma_1",
			"mmsys_com_cg", 9),
	GATE_MMSYS_CMM0(MMSYS_CMM_WDMA_2, "mmsys_cmm_wdma_2",
			"mmsys_com_cg", 10),
	GATE_MMSYS_CMM0(MMSYS_CMM_MDP_FM, "mmsys_cmm_mdp_fm", "fm_cg", 11),
	GATE_MMSYS_CMM0(MMSYS_CMM_RSZ_0, "mmsys_cmm_rsz_0", "mmsys_com_cg", 12),
	GATE_MMSYS_CMM0(MMSYS_CMM_RSZ_1, "mmsys_cmm_rsz_1", "mmsys_com_cg", 13),
	GATE_MMSYS_CMM0(MMSYS_CMM_MUTEX, "mmsys_cmm_mutex", "mutexgen_cg", 14),
	GATE_MMSYS_CMM2(MMSYS_CMM_SMI_COMMON, "mmsys_cmm_smi_com",
		       "mmsys_com_cg", 0),
	GATE_MMSYS_CMM2(MMSYS_CMM_SMI_LARB3, "mmsys_cmm_smi_larb3",
		       "fm_cg", 1),
	GATE_MMSYS_CMM2(MMSYS_CMM_SMI_LARB21, "mmsys_cmm_smi_larb21",
		       "fm_cg", 2),
	GATE_MMSYS_CMM2(MMSYS_CMM_SMI_LARB2, "mmsys_cmm_smi_larb2",
		       "mmsys_com_cg", 3),
	GATE_MMSYS_CMM2(MMSYS_CMM_SMI_LARB20, "mmsys_cmm_smi_larb20",
		       "mmsys_com_cg", 4),
	GATE_MMSYS_CMM2(MMSYS_CMM_RBFC_REN_WPE1, "mmsys_cmm_rbfc_ren_wpe1",
		       "mmsys_com_cg", 5),
	GATE_MMSYS_CMM2(MMSYS_CMM_LARB2_FAKE_ENG, "mmsys_cmm_larb2_fake_eng",
		       "mmsys_com_cg", 6),
	GATE_MMSYS_CMM2(MMSYS_CMM_SIDE_3_IN_CK_TX, "mmsys_cmm_side_3_in_ck_tx",
		       "img_s0_cg", 7),
	GATE_MMSYS_CMM2(MMSYS_CMM_SIDE_2_IN_CK_TX, "mmsys_cmm_side_2_in_ck_tx",
		       "img_s0_cg", 8),
	GATE_MMSYS_CMM2(MMSYS_CMM_SIDE_1_IN_CK_TX, "mmsys_cmm_side_1_in_ck_tx",
		       "img_s0_cg", 9),
	GATE_MMSYS_CMM2(MMSYS_CMM_SIDE_0_IN_CK_TX, "mmsys_cmm_side_0_in_ck_tx",
		       "img_s0_cg", 10),
};

static void __init mtk_mmsys_cmm_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(MMSYS_CMM_NR);
	mtk_clk_register_gates(node, mm_cmm_clks, ARRAY_SIZE(mm_cmm_clks),
			      clk_data);
	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}
CLK_OF_DECLARE(mtk_mmsys_cmm, "mediatek,mt3612-mmsys-cmm",
	      mtk_mmsys_cmm_init);

static const struct mtk_gate_regs mmsys_gaze_cg0_regs __initconst = {
	.set_ofs = 0x0104,
	.clr_ofs = 0x0108,
	.sta_ofs = 0x0100,
};

static const struct mtk_gate_regs mmsys_gaze_cg1_regs __initconst = {
	.set_ofs = 0x0114,
	.clr_ofs = 0x0118,
	.sta_ofs = 0x0110,
};

static const struct mtk_gate_regs mmsys_gaze_cg2_regs __initconst = {
	.set_ofs = 0x0144,
	.clr_ofs = 0x0148,
	.sta_ofs = 0x0140,
};

#define GATE_MMSYS_GAZE0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &mmsys_gaze_cg0_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

#define GATE_MMSYS_GAZE1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &mmsys_gaze_cg1_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
}

#define GATE_MMSYS_GAZE2(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &mmsys_gaze_cg2_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
}

static const struct mtk_gate mm_gaze_clks[] __initconst = {
	GATE_MMSYS_GAZE0(MMSYS_GAZE_WPE_0_tx, "mmsys_gaze_wpe_0_tx",
		"mmsys_com_cg", 0),
	GATE_MMSYS_GAZE0(MMSYS_GAZE_WPE_0, "mmsys_gaze_wpe_0",
		"mmsys_com_cg", 1),
	GATE_MMSYS_GAZE0(MMSYS_GAZE_WDMA_GAZE, "mmsys_gaze_wdma_gaze",
		"mmsys_com_cg", 2),
	GATE_MMSYS_GAZE0(MMSYS_GAZE_MUTEX, "mmsys_gaze_mutex",
		"mutexgen_cg", 3),
	GATE_MMSYS_GAZE2(MMSYS_GAZE_SMI_LARB4, "mmsys_gaze_smi_larb4",
		"mmsys_com_cg", 0),
	GATE_MMSYS_GAZE2(MMSYS_GAZE_SMI_LARB22, "mmsys_gaze_smi_larb22",
		"mmsys_com_cg", 1),
	GATE_MMSYS_GAZE2(MMSYS_GAZE_RBFC_REN_WPE0, "mmsys_gaze_rbfc_ren_wpe0",
		"mmsys_com_cg", 2),
	GATE_MMSYS_GAZE2(MMSYS_GAZE_LARB4_FAKE_ENG, "mmsys_gaze_larb4_fake_eng",
		"mmsys_com_cg", 3),
	GATE_MMSYS_GAZE2(MMSYS_GAZE_GAZE0_IN_CK_TX, "mmsys_gaze_0_in_ck_tx",
		"cam_gz0_cg", 5),
	GATE_MMSYS_GAZE2(MMSYS_GAZE_GAZE1_IN_CK_TX, "mmsys_gaze_1_in_ck_tx",
		"cam_gz1_cg", 6),
};

static void __init mtk_mmsys_gaze_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(MMSYS_CMM_NR);
	mtk_clk_register_gates(node, mm_gaze_clks, ARRAY_SIZE(mm_gaze_clks),
			      clk_data);
	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}
CLK_OF_DECLARE(mtk_mmsys_gaze, "mediatek,mt3612-mmsys-gaze",
	      mtk_mmsys_gaze_init);

static const struct mtk_gate_regs vpusys_vcore_cg0_regs __initconst = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0008,
	.sta_ofs = 0x0000,
};

#define GATE_VPUSYS_VCORE(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &vpusys_vcore_cg0_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate vpu_vcore_clks[] __initconst = {
	GATE_VPUSYS_VCORE(VPUSYS_VCORE_AHB_CG, "vpusys_vcore_ahb_cg",
		"ipu_if_cg", 0),
	GATE_VPUSYS_VCORE(VPUSYS_VCORE_AXI_CG, "vpusys_vcore_axi_cg",
		"ipu_if_cg", 1),
};

static void __init mtk_vpusys_vcore_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(VPUSYS_VCORE_NR);
	mtk_clk_register_gates(node, vpu_vcore_clks, ARRAY_SIZE(vpu_vcore_clks),
			      clk_data);
	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}
CLK_OF_DECLARE(mtk_vpusys_vcore, "mediatek,mt3612-vpusys-vcore",
	      mtk_vpusys_vcore_init);

static const struct mtk_gate_regs vpusys_conn_cg0_regs __initconst = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0008,
	.sta_ofs = 0x0000,
};

#define GATE_VPUSYS_CONN(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &vpusys_conn_cg0_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate vpu_conn_clks[] __initconst = {
	GATE_VPUSYS_CONN(VPUSYS_CONN_AHB_CG, "vpusys_conn_ahb_cg",
		"dsp_cg", 1),
	GATE_VPUSYS_CONN(VPUSYS_CONN_AXI_CG, "vpusys_conn_axi_cg",
		"dsp_cg", 2),
	GATE_VPUSYS_CONN(VPUSYS_CONN_VPU_CG, "vpusys_conn_vpu_cg",
		"dsp_cg", 0),
};

static void __init mtk_vpusys_conn_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(VPUSYS_CONN_NR);
	mtk_clk_register_gates(node, vpu_conn_clks, ARRAY_SIZE(vpu_conn_clks),
			      clk_data);
	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}
CLK_OF_DECLARE(mtk_vpusys_conn, "mediatek,mt3612-vpusys-conn",
	      mtk_vpusys_conn_init);

static const struct mtk_gate_regs vpusys_core0_cg0_regs __initconst = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0008,
	.sta_ofs = 0x0000,
};

#define GATE_VPUSYS_CORE0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &vpusys_core0_cg0_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate vpu_core0_clks[] __initconst = {
	GATE_VPUSYS_CORE0(VPUSYS_CORE_0_VPU_CG, "vpusys_core0_vpu_cg",
		"dsp1_cg", 0),
	GATE_VPUSYS_CORE0(VPUSYS_CORE_0_AXI_M_CG, "vpusys_core0_axi_m_cg",
		"dsp1_cg", 1),
	GATE_VPUSYS_CORE0(VPUSYS_CORE_0_JTAG, "vpusys_core0_jtag_cg",
		"dsp1_cg", 2),
};

static void __init mtk_vpusys_core0_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(VPUSYS_CORE_0_NR);
	mtk_clk_register_gates(node, vpu_core0_clks, ARRAY_SIZE(vpu_core0_clks),
			      clk_data);
	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}
CLK_OF_DECLARE(mtk_vpusys_core0, "mediatek,mt3612-vpusys-core0",
	      mtk_vpusys_core0_init);

static const struct mtk_gate_regs vpusys_core1_cg0_regs __initconst = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0008,
	.sta_ofs = 0x0000,
};

#define GATE_VPUSYS_CORE1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &vpusys_core1_cg0_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate vpu_core1_clks[] __initconst = {
	GATE_VPUSYS_CORE1(VPUSYS_CORE_1_VPU_CG, "vpusys_core1_vpu_cg",
		"dsp2_cg", 0),
	GATE_VPUSYS_CORE1(VPUSYS_CORE_1_AXI_M_CG, "vpusys_core1_axi_m_cg",
		"dsp2_cg", 1),
	GATE_VPUSYS_CORE0(VPUSYS_CORE_1_JTAG, "vpusys_core1_jtag_cg",
		"dsp2_cg", 2),
};

static void __init mtk_vpusys_core1_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(VPUSYS_CORE_1_NR);
	mtk_clk_register_gates(node, vpu_core1_clks, ARRAY_SIZE(vpu_core1_clks),
			      clk_data);
	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}
CLK_OF_DECLARE(mtk_vpusys_core1, "mediatek,mt3612-vpusys-core1",
	      mtk_vpusys_core1_init);

static const struct mtk_gate_regs vpusys_core2_cg0_regs __initconst = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0008,
	.sta_ofs = 0x0000,
};

#define GATE_VPUSYS_CORE2(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &vpusys_core2_cg0_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate vpu_core2_clks[] __initconst = {
	GATE_VPUSYS_CORE1(VPUSYS_CORE_2_VPU_CG, "vpusys_core2_vpu_cg",
		"dsp3_cg", 0),
	GATE_VPUSYS_CORE1(VPUSYS_CORE_2_AXI_M_CG, "vpusys_core2_axi_m_cg",
		"dsp3_cg", 1),
	GATE_VPUSYS_CORE0(VPUSYS_CORE_2_JTAG, "vpusys_core2_jtag_cg",
		"dsp3_cg", 2),
};

static void __init mtk_vpusys_core2_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(VPUSYS_CORE_2_NR);
	mtk_clk_register_gates(node, vpu_core2_clks, ARRAY_SIZE(vpu_core2_clks),
			      clk_data);
	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}
CLK_OF_DECLARE(mtk_vpusys_core2, "mediatek,mt3612-vpusys-core2",
	      mtk_vpusys_core2_init);

static const struct mtk_gate_regs audio_top_cg0_regs __initconst = {
	.set_ofs = 0x0000,
	.clr_ofs = 0x0000,
	.sta_ofs = 0x0000,
};

static const struct mtk_gate_regs audio_top_cg1_regs __initconst = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0004,
	.sta_ofs = 0x0004,
};

#define GATE_ADUIO0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &audio_top_cg0_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr,	\
	}

#define GATE_ADUIO1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &audio_top_cg1_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr,	\
	}

static const struct mtk_gate audio_top_clks[] __initconst = {
	GATE_ADUIO0(AUDIO_TOP_PDN_TDMIN_BCK, "audio_pdn_tdmin_bck",
		   "apll12_div2", 28),
	GATE_ADUIO0(AUDIO_TOP_PDN_A3SYS, "audio_pdn_a3sys", "a3sys_cg",
		   26),
	GATE_ADUIO0(AUDIO_TOP_PDN_A2SYS, "audio_pdn_a2sys", "a2sys_cg",
		   25),
	GATE_ADUIO0(AUDIO_TOP_PDN_A1SYS, "audio_pdn_a1sys", "a1sys_cg",
		   24),
	GATE_ADUIO0(AUDIO_TOP_PDN_DPRX_BCK, "audio_pdn_dprx_bck", "clk26m", 22),
	GATE_ADUIO0(AUDIO_TOP_PDN_TDMOUT_BCK, "audio_pdn_tdmout_bck",
		   "apll12_div3", 20),
	GATE_ADUIO0(AUDIO_TOP_PDN_AFE, "audio_pdn_afe", "clk26m", 2),
	GATE_ADUIO1(AUDIO_TOP_I2S_IN_CLK_CG, "audio_i2s_in_clk_cg", "clk26m",
		   6),
	GATE_ADUIO1(AUDIO_TOP_I2S_OUT_CLK_CG, "audio_i2s_out_clk_cg", "clk26m",
		   5),
};

static void __init mtk_mm_audio_top_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(AUDIO_TOP_NR);

	mtk_clk_register_gates(node, audio_top_clks,
			      ARRAY_SIZE(audio_top_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}
CLK_OF_DECLARE(mtk_mm_audio_top, "mediatek,mt3612-audio-top",
	      mtk_mm_audio_top_init);

static const struct mtk_gate_regs camsys_gz0_cg0_regs __initconst = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0008,
	.sta_ofs = 0x0000,
};

#define GATE_CAM_GZ0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &camsys_gz0_cg0_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate camsys_gz0_clks[] __initconst = {
	GATE_CAM_GZ0(CAMSYS_GZ0_RBFC_RRZO_0_CGPDN, "cam_gz0_rbfc_rrzo_0_cg",
		 "cam_gz0_cg", 15),
	GATE_CAM_GZ0(CAMSYS_GZ0_CAMSV_CGPDN, "cam_gz0_camsv_cg",
		 "cam_gz0_cg", 8),
	GATE_CAM_GZ0(CAMSYS_GZ0_SENINF_CGPDN, "cam_gz0_seninf_cg",
		 "seninf_gaze_cg", 7),
	GATE_CAM_GZ0(CAMSYS_GZ0_CAMTG_CGPDN, "cam_gz0_camtg",
		 "camtg_cg", 6),
	GATE_CAM_GZ0(CAMSYS_GZ0_CAM_0_CGPDN, "cam_gz0_cam_0_cg",
		 "cam_gz0_cg", 4),
	GATE_CAM_GZ0(CAMSYS_GZ0_SYSRAM_LARBX_CGPDN, "cam_gz0_sysram_larbx_cg",
		 "cam_gz0_cg", 1),
	GATE_CAM_GZ0(CAMSYS_GZ0_LARBX_CGPDN, "cam_gz0_larbx_cg",
		 "cam_gz0_cg", 0),
};

static void __init mtk_camsys_gz0_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CAMSYS_GZ0_NR);

	mtk_clk_register_gates(node, camsys_gz0_clks,
			      ARRAY_SIZE(camsys_gz0_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}
CLK_OF_DECLARE(mtk_camsys_gz0, "mediatek,mt3612-camsys-gz0",
	      mtk_camsys_gz0_init);

static const struct mtk_gate_regs camsys_gz1_cg0_regs __initconst = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0008,
	.sta_ofs = 0x0000,
};

#define GATE_CAM_GZ1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &camsys_gz1_cg0_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate camsys_gz1_clks[] __initconst = {
	GATE_CAM_GZ1(CAMSYS_GZ1_RBFC_RRZO_0_CGPDN, "cam_gz1_rbfc_rrzo_0_cg",
		 "cam_gz1_cg", 15),
	GATE_CAM_GZ1(CAMSYS_GZ1_CAMSV_CGPDN, "cam_gz1_camsv_cg",
		 "cam_gz1_cg", 8),
	GATE_CAM_GZ1(CAMSYS_GZ1_SENINF_CGPDN, "cam_gz1_seninf_cg",
		 "seninf_gaze_cg", 7),
	GATE_CAM_GZ1(CAMSYS_GZ1_CAMTG_CGPDN, "cam_gz1_camtg",
		 "camtg_cg", 6),
	GATE_CAM_GZ1(CAMSYS_GZ1_CAM_0_CGPDN, "cam_gz1_cam_0_cg",
		 "cam_gz1_cg", 4),
	GATE_CAM_GZ1(CAMSYS_GZ1_SYSRAM_LARBX_CGPDN, "cam_gz1_sysram_larbx_cg",
		 "cam_gz1_cg", 1),
	GATE_CAM_GZ1(CAMSYS_GZ1_LARBX_CGPDN, "cam_gz1_larbx_cg",
		 "cam_gz1_cg", 0),
};

static void __init mtk_camsys_gz1_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CAMSYS_GZ1_NR);

	mtk_clk_register_gates(node, camsys_gz1_clks,
			      ARRAY_SIZE(camsys_gz1_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}
CLK_OF_DECLARE(mtk_camsys_gz1, "mediatek,mt3612-camsys-gz1",
	      mtk_camsys_gz1_init);

static const struct mtk_gate_regs img_gz0_cg0_regs __initconst = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0008,
	.sta_ofs = 0x0000,
};

#define GATE_IMG_GZ0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &img_gz0_cg0_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate imgsys_gz0_clks[] __initconst = {
	GATE_IMG_GZ0(IMGSYS_GZ0_RBFC_IMG3O_A_CGPDN, "img_gz0_rbfc_img3o_a_cg",
		 "cam_gz0_cg", 12),
	GATE_IMG_GZ0(IMGSYS_GZ0_RBFC_IMGI_A_CGPDN, "img_gz0_rbfc_imgi_a_cg",
		 "cam_gz0_cg", 8),
	GATE_IMG_GZ0(IMGSYS_GZ0_DIP_A_CGPDN, "img_gz0_dip_a_cg",
		 "cam_gz0_cg", 2),
};

static void __init mtk_imgsys_gz0_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(IMGSYS_GZ0_NR);

	mtk_clk_register_gates(node, imgsys_gz0_clks,
			      ARRAY_SIZE(imgsys_gz0_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}
CLK_OF_DECLARE(mtk_imgsys_gz0, "mediatek,mt3612-imgsys-gz0",
	      mtk_imgsys_gz0_init);

static const struct mtk_gate_regs img_gz1_cg0_regs __initconst = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0008,
	.sta_ofs = 0x0000,
};

#define GATE_IMG_GZ1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &img_gz1_cg0_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate imgsys_gz1_clks[] __initconst = {
	GATE_IMG_GZ1(IMGSYS_GZ1_RBFC_IMG3O_A_CGPDN, "img_gz1_rbfc_img3o_a_cg",
		 "cam_gz1_cg", 12),
	GATE_IMG_GZ1(IMGSYS_GZ1_RBFC_IMGI_A_CGPDN, "img_gz1_rbfc_imgi_a_cg",
		 "cam_gz1_cg", 8),
	GATE_IMG_GZ1(IMGSYS_GZ1_DIP_A_CGPDN, "img_gz1_dip_a_cg",
		 "cam_gz1_cg", 2),
};

static void __init mtk_imgsys_gz1_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(IMGSYS_GZ1_NR);

	mtk_clk_register_gates(node, imgsys_gz1_clks,
			      ARRAY_SIZE(imgsys_gz1_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}
CLK_OF_DECLARE(mtk_imgsys_gz1, "mediatek,mt3612-imgsys-gz1",
	      mtk_imgsys_gz1_init);

static const struct mtk_gate_regs camsys_side0_cg0_regs __initconst = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0008,
	.sta_ofs = 0x0000,
};

#define GATE_CAM_SIDE0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &camsys_side0_cg0_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate camsys_side0_clks[] __initconst = {
	GATE_CAM_SIDE0(CAMSYS_SIDE0_BE1_CGPDN, "cam_side0_be1_cg",
		 "cam_s0_cg", 18),
	GATE_CAM_SIDE0(CAMSYS_SIDE0_BE0_CGPDN, "cam_side0_be0_cg",
		 "cam_s0_cg", 17),
	GATE_CAM_SIDE0(CAMSYS_SIDE0_RBFC_RRZO_1_CGPDN,
		 "cam_side0_rbfc_rrzo_1_cg", "cam_s0_cg", 16),
	GATE_CAM_SIDE0(CAMSYS_SIDE0_RBFC_RRZO_0_CGPDN,
		 "cam_side0_rbfc_rrzo_0_cg", "cam_s0_cg", 15),
	GATE_CAM_SIDE0(CAMSYS_SIDE0_CCU_CGPDN, "cam_side0_ccu_cg",
		 "cam_s0_cg", 11),
	GATE_CAM_SIDE0(CAMSYS_SIDE0_CAMSV_CGPDN, "cam_side0_camsv_cg",
		 "cam_s0_cg", 8),
	GATE_CAM_SIDE0(CAMSYS_SIDE0_SENINF_CGPDN, "cam_side0_seninf_cg",
		 "seninf_cg", 7),
	GATE_CAM_SIDE0(CAMSYS_SIDE0_CAMTG_CGPDN, "cam_side0_camtg_cg",
		 "camtg_cg", 6),
	GATE_CAM_SIDE0(CAMSYS_SIDE0_CAM_1_CGPDN, "cam_side0_cam_1_cg",
		 "cam_s0_cg", 5),
	GATE_CAM_SIDE0(CAMSYS_SIDE0_CAM_0_CGPDN, "cam_side0_cam_0_cg",
		 "cam_s0_cg", 4),
	GATE_CAM_SIDE0(CAMSYS_SIDE0_SYSRAM_LARBX_CGPDN,
		 "cam_side0_sysram_larbx_cg", "cam_s0_cg", 1),
	GATE_CAM_SIDE0(CAMSYS_SIDE0_LARBX_CGPDN, "cam_side0_larbx_cg",
		 "cam_s0_cg", 0),
};

static void __init mtk_camsys_side0_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CAMSYS_SIDE0_NR);

	mtk_clk_register_gates(node, camsys_side0_clks,
			      ARRAY_SIZE(camsys_side0_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}
CLK_OF_DECLARE(mtk_camsys_side0, "mediatek,mt3612-camsys-side0",
	      mtk_camsys_side0_init);

static const struct mtk_gate_regs camsys_side1_cg0_regs __initconst = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0008,
	.sta_ofs = 0x0000,
};

#define GATE_CAM_SIDE1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &camsys_side1_cg0_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate camsys_side1_clks[] __initconst = {
	GATE_CAM_SIDE1(CAMSYS_SIDE1_BE1_CGPDN, "cam_side1_be1_cg",
		 "cam_s1_cg", 18),
	GATE_CAM_SIDE1(CAMSYS_SIDE1_BE0_CGPDN, "cam_side1_be0_cg",
		 "cam_s1_cg", 17),
	GATE_CAM_SIDE1(CAMSYS_SIDE1_RBFC_RRZO_1_CGPDN,
		 "cam_side1_rbfc_rrzo_1_cg", "cam_s1_cg", 16),
	GATE_CAM_SIDE1(CAMSYS_SIDE1_RBFC_RRZO_0_CGPDN,
		 "cam_side1_rbfc_rrzo_0_cg", "cam_s1_cg", 15),
	GATE_CAM_SIDE1(CAMSYS_SIDE1_CCU_CGPDN, "cam_side1_ccu_cg",
		 "cam_s1_cg", 11),
	GATE_CAM_SIDE1(CAMSYS_SIDE1_CAMSV_CGPDN, "cam_side1_camsv_cg",
		 "cam_s1_cg", 8),
	GATE_CAM_SIDE1(CAMSYS_SIDE1_SENINF_CGPDN, "cam_side1_seninf_cg",
		 "seninf_cg", 7),
	GATE_CAM_SIDE1(CAMSYS_SIDE1_CAMTG_CGPDN, "cam_side1_camtg_cg",
		 "camtg_cg", 6),
	GATE_CAM_SIDE1(CAMSYS_SIDE1_CAM_1_CGPDN, "cam_side1_cam_1_cg",
		 "cam_s1_cg", 5),
	GATE_CAM_SIDE1(CAMSYS_SIDE1_CAM_0_CGPDN, "cam_side1_cam_0_cg",
		 "cam_s1_cg", 4),
	GATE_CAM_SIDE1(CAMSYS_SIDE1_SYSRAM_LARBX_CGPDN,
		 "cam_side1_sysram_larbx_cg", "cam_s1_cg", 1),
	GATE_CAM_SIDE1(CAMSYS_SIDE1_LARBX_CGPDN, "cam_side1_larbx_cg",
		 "cam_s1_cg", 0),
};

static void __init mtk_camsys_side1_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CAMSYS_SIDE1_NR);

	mtk_clk_register_gates(node, camsys_side1_clks,
			      ARRAY_SIZE(camsys_side1_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}
CLK_OF_DECLARE(mtk_camsys_side1, "mediatek,mt3612-camsys-side1",
	      mtk_camsys_side1_init);

static const struct mtk_gate_regs img_side0_cg0_regs __initconst = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0008,
	.sta_ofs = 0x0000,
};

#define GATE_IMG_SIDE0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &img_side0_cg0_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate imgsys_side0_clks[] __initconst = {
	GATE_IMG_SIDE0(IMGSYS_SIDE0_RBFC_IMG3O_B_CGPDN,
		 "img_side0_rbfc_img3o_b_cg", "img_s0_cg", 13),
	GATE_IMG_SIDE0(IMGSYS_SIDE0_RBFC_IMG3O_A_CGPDN,
		 "img_side0_rbfc_img3o_a_cg", "img_s0_cg", 12),
	GATE_IMG_SIDE0(IMGSYS_SIDE0_RBFC_IMGI_B_CGPDN,
		 "img_side0_rbfc_imgi_b_cg", "cam_s0_cg", 9),
	GATE_IMG_SIDE0(IMGSYS_SIDE0_RBFC_IMGI_A_CGPDN,
		 "img_side0_rbfc_imgi_a_cg", "cam_s0_cg", 8),
	GATE_IMG_SIDE0(IMGSYS_SIDE0_SYSRAM_LARBX_1_CGPDN,
		 "img_side0_sysram_larbx_1_cg", "img_s0_cg", 4),
	GATE_IMG_SIDE0(IMGSYS_SIDE0_DIP_B_CGPDN,
		 "img_side0_dip_b_cg", "img_s0_cg", 3),
	GATE_IMG_SIDE0(IMGSYS_SIDE0_DIP_A_CGPDN,
		 "img_side0_dip_a_cg", "img_s0_cg", 2),
	GATE_IMG_SIDE0(IMGSYS_SIDE0_SYSRAM_LARBX_CGPDN,
		 "img_side0_sysram_larbx_cg", "img_s0_cg", 1),
	GATE_IMG_SIDE0(IMGSYS_SIDE0_LARBX_CGPDN,
		 "img_side0_larbx_cg", "img_s0_cg", 0),
};

static void __init mtk_imgsys_side0_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(IMGSYS_SIDE0_NR);

	mtk_clk_register_gates(node, imgsys_side0_clks,
			      ARRAY_SIZE(imgsys_side0_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}
CLK_OF_DECLARE(mtk_imgsys_side0, "mediatek,mt3612-imgsys-side0",
	      mtk_imgsys_side0_init);

static const struct mtk_gate_regs img_side1_cg0_regs __initconst = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0008,
	.sta_ofs = 0x0000,
};

#define GATE_IMG_SIDE1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &img_side1_cg0_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate imgsys_side1_clks[] __initconst = {
	GATE_IMG_SIDE1(IMGSYS_SIDE1_RBFC_IMG3O_B_CGPDN,
		 "img_side1_rbfc_img3o_b_cg", "img_s0_cg", 13),
	GATE_IMG_SIDE1(IMGSYS_SIDE1_RBFC_IMG3O_A_CGPDN,
		 "img_side1_rbfc_img3o_a_cg", "img_s0_cg", 12),
	GATE_IMG_SIDE1(IMGSYS_SIDE1_RBFC_IMGI_B_CGPDN,
		 "img_side1_rbfc_imgi_b_cg", "cam_s1_cg", 9),
	GATE_IMG_SIDE1(IMGSYS_SIDE1_RBFC_IMGI_A_CGPDN,
		 "img_side1_rbfc_imgi_a_cg", "cam_s1_cg", 8),
	GATE_IMG_SIDE1(IMGSYS_SIDE1_SYSRAM_LARBX_1_CGPDN,
		 "img_side1_sysram_larbx_1_cg", "img_s0_cg", 4),
	GATE_IMG_SIDE1(IMGSYS_SIDE1_DIP_B_CGPDN,
		 "img_side1_dip_b_cg", "img_s0_cg", 3),
	GATE_IMG_SIDE1(IMGSYS_SIDE1_DIP_A_CGPDN,
		 "img_side1_dip_a_cg", "img_s0_cg", 2),
	GATE_IMG_SIDE1(IMGSYS_SIDE1_SYSRAM_LARBX_CGPDN,
		 "img_side1_sysram_larbx_cg", "img_s0_cg", 1),
	GATE_IMG_SIDE1(IMGSYS_SIDE1_LARBX_CGPDN,
		 "img_side1_larbx_cg", "img_s0_cg", 0),
};

static void __init mtk_imgsys_side1_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(IMGSYS_SIDE1_NR);

	mtk_clk_register_gates(node, imgsys_side1_clks,
			      ARRAY_SIZE(imgsys_side1_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
}
CLK_OF_DECLARE(mtk_imgsys_side1, "mediatek,mt3612-imgsys-side1",
	      mtk_imgsys_side1_init);

static void __iomem *topck_base;
int mtk_topckgen_slicer_div(enum slrc_diven div_en, unsigned int m_mod,
	unsigned int n_mod)
{
	struct device_node *node = NULL;
	int val = 0;

	if ((m_mod & (~M_MOD_MASK)) != 0) {
		pr_err("slrc m_mod input error!!!\n");
		return 1;
	}

	if ((n_mod & (~N_MOD_MASK)) != 0) {
		pr_err("slrc m_mod input error!!!\n");
		return 1;
	}

	if (!topck_base) {
		node = of_find_compatible_node(NULL, NULL,
			"mediatek,mt3612-topckgen");
		if (!node) {
			pr_err("mediatek,mt3612-topckgen node failed!!!");
			return 1;
		}
		topck_base = of_iomap(node, 0);
		if (!topck_base) {
			pr_err("%s(): ioremap failed\n", __func__);
			return 1;
		}
	}

	val = (div_en << DIVEN_SHIFT) + (m_mod << M_MOD_SHIFT) + n_mod;
	writel(val, topck_base + CLK_MISC_CFG0);

	return 0;
}
EXPORT_SYMBOL(mtk_topckgen_slicer_div);

