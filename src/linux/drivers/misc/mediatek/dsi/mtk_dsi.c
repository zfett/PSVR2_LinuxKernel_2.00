/*
 * Copyright (c) 2015 MediaTek Inc.
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
 * @file mtk_dsi.c
 * MTK DSI Linux Driver. \n
 * This driver is used to configure MTK dsi hardware module. \n
 * It include frame size, video & physical timing, data format and lane setting.
 */

/**
 * @defgroup IP_group_dsi DSI
 *   There are total 4 dsi in MT3612. \n
 *   Each dsi support RGB888/RGB101010/Compression data format. \n
 *   And also support 4 dsi display at same time.
 *
 *   @{
 *       @defgroup IP_group_dsi_external EXTERNAL
 *         The external API document for dsi.
 *
 *         @{
 *             @defgroup IP_group_dsi_external_function 1.function
 *               This is dsi external function.
 *             @defgroup IP_group_dsi_external_struct 2.structure
 *               None. No external structure used in dsi driver.
 *             @defgroup IP_group_dsi_external_typedef 3.typedef
 *               None. No external typedef used in dsi driver.
 *             @defgroup IP_group_dsi_external_enum 4.enumeration
 *               This is dsi external enumeration.
 *             @defgroup IP_group_dsi_external_def 5.define
 *               None. No external define used in dsi driver.
 *         @}
 *
 *       @defgroup IP_group_dsi_internal INTERNAL
 *         The internal API document for dsi.
 *
 *         @{
 *             @defgroup IP_group_dsi_internal_function 1.function
 *               This is dsi internal function and module init.
 *               @{
 *                   @defgroup IP_group_dsi_internal_function_dsi dsi
 *                     This is dsi internal common function and module init.
 *                   @defgroup IP_group_dsi_internal_function_panel panel
 *                     This is dsi panel common function.
 *                   @defgroup IP_group_dsi_internal_function_debug debug
 *                     This is dsi debug function, using debugfs.
 *               @}
 *             @defgroup IP_group_dsi_internal_struct 2.structure
 *               This is dsi internal structure.
 *               @{
 *                   @defgroup IP_group_dsi_internal_struct_dsi dsi
 *                     This is dsi internal structure.
 *                   @defgroup IP_group_dsi_internal_struct_panel panel
 *                     This is dsi panel internal structure.
 *               @}
 *             @defgroup IP_group_dsi_internal_typedef 3.typedef
 *               None. No internal typedef used in dsi driver.
 *             @defgroup IP_group_dsi_internal_enum 4.enumeration
 *               This is dsi internal enumeration.
 *             @defgroup IP_group_dsi_internal_def 5.define
 *               This is dsi internal define.
 *               @{
 *                   @defgroup IP_group_dsi_internal_def_dsi dsi
 *                     This is dsi internal define.
 *                   @defgroup IP_group_dsi_internal_def_panel panel
 *                     This is dsi panel internal define.
 *                   @defgroup IP_group_dsi_internal_def_debug debug
 *                     This is dsi debug internal define.
 *               @}
 *         @}
 *   @}
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/phy/phy.h>
#include <video/videomode.h>
#include <video/mipi_display.h>
#include <drm/drm_mipi_dsi.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>

#include "mtk_dsi_int.h"
#include "mtk_dsi_reg.h"
#include <soc/mediatek/mtk_dsi.h>
#include <soc/mediatek/mtk_mipi_tx.h>
#include <soc/mediatek/mtk_mmsys_cfg.h>
#include "mtk_dsi_debugfs.h"

/** @ingroup IP_group_dsi_internal_def_dsi
 * @{
 */
/* DSI_MODE_CON - MODE_CON */
#define CMD_MODE			(0)
#define SYNC_PULSE_MODE			(1)
#define SYNC_EVENT_MODE			(2)
#define BURST_MODE			(3)

/* DSI_PSCON - DSI_PS_SEL */
#define PACKED_PS_16BIT_RGB565		(0 << 16)
#define LOOSELY_PS_24BIT_RGB666		(1 << 16)
#define PACKED_PS_18BIT_RGB666		(2 << 16)
#define PACKED_PS_24BIT_RGB888		(3 << 16)
#define PACKED_PS_30BIT_RGB101010	(4 << 16)
#define COMPRESSION			(5 << 16)

/* DSI_PHY_LCPAT - LC_HSTX_CK_PAT */
#define PHY_LCPAT_NORMAL		0x000000aa
#define PHY_LCPAT_INV			0x00000055

/* DSI_PHY_SYNCON */
#define HS_SYNC_CODE_SET(x)		((x) << 0)
#define HS_SYNC_CODE2_SET(x)		((x) << 8)
#define HS_SKEWCAL_PAT_SET(x)		((x) << 16)

/* DSI_PHY_TIMCON0 */
#define LPX_SET(x)			((x) << 0)
#define DA_HS_PRPR_SET(x)		((x) << 8)
#define DA_HS_ZERO_SET(x)		((x) << 16)
#define DA_HS_TRAIL_SET(x)		((x) << 24)

/* DSI_PHY_TIMCON1 */
#define TA_GO_SET(x)			((x) << 0)
#define TA_SURE_SET(x)			((x) << 8)
#define TA_GET_SET(x)			((x) << 16)
#define DA_HS_EXIT_SET(x)		((x) << 24)

/* DSI_PHY_TIMCON2 */
#define DA_HS_SYNC_MASK			DA_HS_SYNC
#define DA_HS_SYNC_SET(x)		((x) << 8)
#define CLK_HS_ZERO_SET(x)		((x) << 16)
#define CLK_HS_TRAIL_SET(x)		((x) << 24)

/* DSI_PHY_TIMCON3 */
#define CLK_HS_PRPR_SET(x)		((x) << 0)
#define CLK_HS_POST_SET(x)		((x) << 8)
#define CLK_HS_EXIT_SET(x)		((x) << 16)

/* DSI_VM_CMD_CON */
#define LONG_PKT_SET(x)			((x) << 1)
#define CM_DATA_ID_SET(x)		((x) << 8)
#define CM_DATA_0_SET(x)		((x) << 16)
#define CM_DATA_1_SET(x)		((x) << 24)

#define FIFO_MEMORY_TYPE_SET(x)		((x) << 12)

#define VACTL_EVENT_OFFSET_SET(x)	((x) << 0)
#define VACTL_EVENT_PERIOD_SET(x)	((x) << 16)

/**
 * @}
 */

/** @ingroup IP_group_dsi_internal_def_dsi
 * @{
 */
/** calculate how many dsi_clk by input ns */
#define NS_TO_CYCLE(n, c)    ((n) / (c) + (((n) % (c)) ? 1 : 0))
/**
 * @}
 */

/** @ingroup IP_group_dsi_internal_struct_dsi
 * @brief Data Structure used for sending type 0 command
 */
struct dsi_cmd_t0 {
	/** config field for type 0 command setting */
	u8 config;
	/** type field for type 0 command setting */
	u8 type;
	/** data 0 field for type 0 command setting */
	u8 data0;
	/** data 1 field for type 0 command setting */
	u8 data1;
};

/** @ingroup IP_group_dsi_internal_struct_dsi
 * @brief Data Structure used for sending type 2 command
 */
struct dsi_cmd_t2 {
	/** config field for type 1 command setting */
	u8 config;
	/** type field for type 1 command setting */
	u8 type;
	/** word count field for type 1 command setting */
	u16 wc16;
	/** data point for type 1 command setting */
	u8 *pdata;
};

/** @ingroup IP_group_dsi_internal_struct_dsi
 * @brief Data Structure used for receiving packet data
 */
struct dsi_rx_data {
	/** byte 0 field for receive data register */
	u8 byte0;
	/** byte 1 field for receive data register */
	u8 byte1;
	/** byte 2 field for receive data register */
	u8 byte2;
	/** byte 3 field for receive data register */
	u8 byte3;
};

/** @ingroup IP_group_dsi_internal_struct_dsi
 * @brief Data Structure used for sending packet data
 */
struct dsi_tx_cmdq {
	/** byte 0 field for common command queue register */
	u8 byte0;
	/** byte 1 field for common command queue register */
	u8 byte1;
	/** byte 2 field for common command queue register */
	u8 byte2;
	/** byte 3 field for common command queue register */
	u8 byte3;
};

/** @ingroup IP_group_dsi_internal_struct_dsi
 * @brief Data Structure used for setting DSI_CMDQ
 */
struct dsi_tx_cmdq_regs {
	struct dsi_tx_cmdq data[128];
};

/** @ingroup IP_group_dsi_internal_enum
 * @brief DSI Interrupt Status
 */
enum {
	/** sleep out done */
	DSI_INT_SLEEPOUT_DONE_FLAG	= BIT(6),
	/** send vm command done */
	DSI_INT_VM_CMD_DONE_FLAG	= BIT(5),
	/** frame transmission done */
	DSI_INT_FRAME_DONE_FLAG		= BIT(4),
	/** video mode done */
	DSI_INT_VM_DONE_FLAG		= BIT(3),
	/** TE ready done */
	DSI_INT_TE_RDY_FLAG		= BIT(2),
	/** send command done */
	DSI_INT_CMD_DONE_FLAG		= BIT(1),
	/** LP Rx read ready */
	DSI_INT_LPRX_RD_RDY_FLAG	= BIT(0),
	/** config all bits */
	DSI_INT_ALL_BITS		= (0x7f)
};

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     DSI reigster mask write common function.
 * @param[out] reg: dsi register base address
 * @param[in] offset: dsi register offset
 * @param[in] mask: write data mask value
 * @param[in] data: write data value
 * @return none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsi_mask(void __iomem *reg, u32 offset, u32 mask, u32 data)
{
	u32 temp = readl(reg + offset);

	writel((temp & ~mask) | (data & mask), reg + offset);
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     DSI CMDQ reigster write function. \n
 *     This function is used to set CMDQ register \n
 *     when host need to send packet to panel.
 * @param[out] reg: dsi CMDQ register address
 * @param[in] mask: write data mask value
 * @param[in] data: write data value
 * @return none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsi_set_cmdq(void __iomem *reg, u32 mask, u32 data)
{
	u32 temp = readl(reg);

	writel((temp & ~mask) | (data & mask), reg);
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Dump all dsi reigsters on the console.
 * @param[in] dsi: dsi driver data struct
 * @return none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mtk_dsi_dump_registers(const struct mtk_dsi *dsi)
{
	u32 i, j;

	for (i = 0; i < dsi->hw_nr; i++) {
		dev_dbg(dsi->dev, "---- Start dump DSI%d registers ----\n", i);

		for (j = 0; j < 0x200; j += 0x10) {
			dev_dbg(dsi->dev,
				"DSI+%04x: 0x%08x 0x%08x 0x%08x 0x%08x\n",
				j, readl(dsi->regs[i] + j),
				readl(dsi->regs[i] + j + 0x4),
				readl(dsi->regs[i] + j + 0x8),
				readl(dsi->regs[i] + j + 0xc));
		}

		for (j = 0; j < 0x80; j += 0x10) {
			dev_dbg(dsi->dev,
				"DSI_CMD+%04x: 0x%08x 0x%08x 0x%08x 0x%08x\n",
				j, readl(dsi->regs[i] + DSI_CMDQ + j),
				readl(dsi->regs[i] + DSI_CMDQ + j + 0x4),
				readl(dsi->regs[i] + DSI_CMDQ + j + 0x8),
				readl(dsi->regs[i] + DSI_CMDQ + j + 0xc));
		}
	}
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Setup dsi cd-phy related timing. \n
 *     Uses the data_rate to calculate the phy related timing \n
 *     -- LPX, DA_HS_PREP, DA_HS_ZERO, DA_HS_TRAIL...
 * @param[in] dsi: dsi driver data struct
 * @return none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsi_phy_timconfig(struct mtk_dsi *dsi)
{
	struct mtk_mipi_tx *mipi_tx = phy_get_drvdata(dsi->phy);
	u32 timcon0, timcon1, timcon2, timcon3;
	u32 ui, cycle_time;
	u32 lpx, hs_prep, hs_zero, hs_trail;
	u32 hs_exit, ta_go, ta_get, ta_sure;
	u32 clk_hs_prep, clk_zero, clk_trail, clk_exit, clk_post;
	u32 i;

	if (mipi_tx->phy_type == MTK_MIPI_DPHY) {
		ui = (1000000 + dsi->data_rate - 1) / dsi->data_rate;
		cycle_time = (8000000 + dsi->data_rate - 1) / dsi->data_rate;
	} else {
		ui =
		 ((((16000000 + dsi->data_rate - 1) / dsi->data_rate) + 6) / 7);
		cycle_time = (16000000 + dsi->data_rate - 1) / dsi->data_rate;
	}

	/* lpx >= 50ns (spec) */
	/* lpx = 60ns */
	lpx = NS_TO_CYCLE(60, cycle_time);
	if (lpx < 2)
		lpx = 2;

	/* hs_prep = 40ns+4*UI ~ 85ns+6*UI (spec) */
	/* hs_prep = 48ns+5*UI */
	hs_prep = NS_TO_CYCLE((48 + 5 * ui), cycle_time);

	/* hs_zero + hs_prep > 145ns+10*UI (spec) */
	/* hs_zero = (170+10*UI) - hs_prep */
	hs_zero = NS_TO_CYCLE((170 + 10 * ui), cycle_time) - hs_prep;

	if (hs_zero < 1)
		hs_zero = 1;

	/* hs_trail > max(8*UI, 60ns+4*UI) (spec) */
	/* hs_trail = 96ns+4*UI */
	hs_trail = 96 + 4 * ui;
	if (hs_trail > cycle_time)
		hs_trail = NS_TO_CYCLE(hs_trail, cycle_time);
	else
		hs_trail = 2;

	/* hs_exit > 100ns (spec) */
	/* hs_exit = 120ns */
	hs_exit = NS_TO_CYCLE(120, cycle_time);

	/* ta_go = 4*lpx (spec) */
	ta_go = 4 * lpx;

	/* ta_get = 5*lpx (spec) */
	ta_get = 5 * lpx;

	/* ta_sure = lpx ~ 2*lpx (spec) */
	ta_sure = (3 * lpx) / 2;

	/* clk_hs_prep = 38ns ~ 95ns (spec) */
	/* clk_hs_prep = 64ns */
	clk_hs_prep = NS_TO_CYCLE(64, cycle_time);

	/* clk_zero + clk_hs_prep > 300ns (spec) */
	/* clk_zero = 400ns - clk_hs_prep */
	clk_zero = NS_TO_CYCLE(400, cycle_time) - clk_hs_prep;
	if (clk_zero < 1)
		clk_zero = 1;

	/* clk_trail > 60ns (spec) */
	/* clk_trail = 96ns */
	clk_trail = NS_TO_CYCLE(96, cycle_time);
	if (clk_trail < 2)
		clk_trail = 2;

	/* clk_exit > 100ns (spec) */
	/* clk_exit = 200ns */
	clk_exit = NS_TO_CYCLE(200, cycle_time);

	/* clk_post > 60ns+52*UI (spec) */
	/* clk_post = 64ns+52*UI */
	clk_post = NS_TO_CYCLE((64 + 52 * ui), cycle_time);

	if (mipi_tx->phy_type == MTK_MIPI_DPHY) {
		timcon0 = LPX_SET(lpx) | DA_HS_PRPR_SET(hs_prep) |
			  DA_HS_ZERO_SET(hs_zero) | DA_HS_TRAIL_SET(hs_trail);
		timcon1 = TA_GO_SET(ta_go) | TA_SURE_SET(ta_sure) |
			  TA_GET_SET(ta_get) | DA_HS_EXIT_SET(hs_exit);
		timcon2 = DA_HS_SYNC_SET(1) | CLK_HS_ZERO_SET(clk_zero) |
			  CLK_HS_TRAIL_SET(clk_trail);
		timcon3 = CLK_HS_PRPR_SET(clk_hs_prep) |
			  CLK_HS_POST_SET(clk_post) | CLK_HS_EXIT_SET(clk_exit);

		dsi->data_init_cycle = hs_exit + lpx + hs_prep + hs_zero + 2;
	} else {
		/* hs_prep(t3_prepare) = 38ns ~ 95ns (spec) */
		hs_prep = NS_TO_CYCLE(64, cycle_time);

		timcon0 = LPX_SET(lpx) | DA_HS_PRPR_SET(hs_prep) |
			  DA_HS_ZERO_SET(0x20) | DA_HS_TRAIL_SET(0x10);
		timcon1 = TA_GO_SET(ta_go) | TA_SURE_SET(ta_sure) |
			  TA_GET_SET(ta_get) | DA_HS_EXIT_SET(hs_exit);
		timcon2 = DA_HS_SYNC_SET(clk_hs_prep) | CLK_HS_ZERO_SET(0) |
			  CLK_HS_TRAIL_SET(0);
		timcon3 = CLK_HS_PRPR_SET(0) | CLK_HS_POST_SET(0) |
			  CLK_HS_EXIT_SET(0);

		/*
		 * tHS-EXIT + tLPX + tPREPARE + tPREBEGIN(0x40) + tPREEND(0x01)
		 * + tSYNC(0x01) + tPOST(0x20)
		 */
		dsi->data_init_cycle = hs_exit + lpx + clk_hs_prep + 0x42 +
				       0x20;
	}

	dev_dbg(dsi->dev, "timcon0: 0x%x\n", timcon0);
	dev_dbg(dsi->dev, "timcon1: 0x%x\n", timcon1);
	dev_dbg(dsi->dev, "timcon2: 0x%x\n", timcon2);
	dev_dbg(dsi->dev, "timcon3: 0x%x\n", timcon3);

	for (i = 0; i < dsi->hw_nr; i++) {
		writel(timcon0, dsi->regs[i] + DSI_PHY_TIMCON0);
		writel(timcon1, dsi->regs[i] + DSI_PHY_TIMCON1);
		writel(timcon2, dsi->regs[i] + DSI_PHY_TIMCON2);
		writel(timcon3, dsi->regs[i] + DSI_PHY_TIMCON3);
	}

	if (mipi_tx->phy_type == MTK_MIPI_DPHY) {
		for (i = 0; i < dsi->hw_nr; i++) {
			mtk_dsi_mask(dsi->regs[i], DSI_CPHY_CON0,
				     SETTLE_SKIP_EN, 0);
			mtk_dsi_mask(dsi->regs[i], DSI_CPHY_CON0, CPHY_EN, 0);
		}
	} else {
		for (i = 0; i < dsi->hw_nr; i++) {
			mtk_dsi_mask(dsi->regs[i], DSI_CPHY_CON0,
				     SETTLE_SKIP_EN, SETTLE_SKIP_EN);
			mtk_dsi_mask(dsi->regs[i], DSI_CPHY_CON0, CPHY_EN,
				     CPHY_EN);
		}
	}
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     DSI reset function. \n
 *     Reset dsi state machine to idel state
 * @param[in] dsi: dsi driver data struct
 * @return none
 * @par Boundary case and Limitation
 *     Do not reset dsi during write dsi register.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsi_reset_engine(const struct mtk_dsi *dsi)
{
	u32 i;

	for (i = 0; i < dsi->hw_nr; i++) {
		mtk_dsi_mask(dsi->regs[i], DSI_COM_CON, DSI_RESET, DSI_RESET);
		mtk_dsi_mask(dsi->regs[i], DSI_COM_CON, DSI_RESET, 0);
	}
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Power on dsi hardware. \n
 *     1. power on phy and setup pll according to data_rate \n
 *     2. clear dsi clock gating \n
 *     3. setup phy timing
 * @param[in] dsi: dsi driver data struct
 * @return
 *     0, power on success. \n
 *     Negative, power on fail.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_dsi_poweron(struct mtk_dsi *dsi)
{
	struct device *dev = dsi->dev;
	struct mtk_mipi_tx *mipi_tx = phy_get_drvdata(dsi->phy);
	int ret = 0;
#ifdef CONFIG_COMMON_CLK_MT3612
	int i;
#endif

	if (dsi->poweron)
		return 0;

	if (dsi->fpga_mode == 1)
		dsi->data_rate = 26000;

	if (WARN_ON(dsi->data_rate == 0))
		return -1;

#ifdef CONFIG_COMMON_CLK_MT3612
	ret = pm_runtime_get_sync(dev);
	if (ret < 0) {
		dev_err(dev, "Failed to enable power domain: %d\n", ret);
		return ret;
	}
#endif

	mipi_tx->data_rate = dsi->data_rate;
	ret = phy_power_on(dsi->phy);
	if (ret < 0) {
		dev_err(dev, "phy can't power on %d\n", ret);
		goto err_phy_power_off;
	}

	/* clear clock gating */
#ifdef CONFIG_COMMON_CLK_MT3612
	for (i = 0; i < dsi->hw_nr; i++) {
		ret = clk_prepare_enable(dsi->mm_clk[i]);
		if (ret)
			goto err_clk_off;
		ret = clk_prepare_enable(dsi->dsi_clk[i]);
		if (ret)
			goto err_clk_off;
	}
#endif

	mtk_dsi_phy_timconfig(dsi);

	dsi->poweron = true;

	return ret;

#ifdef CONFIG_COMMON_CLK_MT3612
err_clk_off:
	for (i = 0; i < dsi->hw_nr; i++) {
		clk_disable_unprepare(dsi->mm_clk[i]);
		clk_disable_unprepare(dsi->dsi_clk[i]);
	}
#endif

err_phy_power_off:
	phy_power_off(dsi->phy);
#ifdef CONFIG_COMMON_CLK_MT3612
	pm_runtime_put(dev);
#endif

	return ret;
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Let clock lane enter ulps mode.
 * @param[in] dsi: dsi driver data struct
 * @return none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsi_clk_ulp_mode_enter(const struct mtk_dsi *dsi)
{
	u32 i;

	for (i = 0; i < dsi->hw_nr; i++) {
		mtk_dsi_mask(dsi->regs[i], DSI_PHY_LCCON, LC_HSTX_EN, 0);
		mtk_dsi_mask(dsi->regs[i], DSI_PHY_LCCON, LC_ULPM_EN,
			     LC_ULPM_EN);
	}
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Let clock lane exit ulps mode.
 * @param[in] dsi: dsi driver data struct
 * @return none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsi_clk_ulp_mode_leave(const struct mtk_dsi *dsi)
{
	u32 i;

	for (i = 0; i < dsi->hw_nr; i++) {
		mtk_dsi_mask(dsi->regs[i], DSI_PHY_LCCON, LC_ULPM_EN, 0);
		mtk_dsi_mask(dsi->regs[i], DSI_PHY_LCCON, LC_WAKEUP_EN,
			     LC_WAKEUP_EN);
		mtk_dsi_mask(dsi->regs[i], DSI_PHY_LCCON, LC_WAKEUP_EN, 0);
	}
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Let data lane 0 enter ulps mode.
 * @param[in] dsi: dsi driver data struct
 * @return none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsi_lane0_ulp_mode_enter(const struct mtk_dsi *dsi)
{
	u32 i;

	for (i = 0; i < dsi->hw_nr; i++)
		mtk_dsi_mask(dsi->regs[i], DSI_PHY_LD0CON, L0_ULPM_EN,
			     L0_ULPM_EN);
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Let data lane 0 exit ulps mode.
 * @param[in] dsi: dsi driver data struct
 * @return none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsi_lane0_ulp_mode_leave(const struct mtk_dsi *dsi)
{
	u32 i;

	for (i = 0; i < dsi->hw_nr; i++) {
		mtk_dsi_mask(dsi->regs[i], DSI_PHY_LD0CON, L0_ULPM_EN, 0);
		mtk_dsi_mask(dsi->regs[i], DSI_PHY_LD0CON, L0_WAKEUP_EN,
			     L0_WAKEUP_EN);
		mtk_dsi_mask(dsi->regs[i], DSI_PHY_LD0CON, L0_WAKEUP_EN, 0);
	}
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Check clock lane HS mode is enable or not.
 * @param[in] dsi: dsi driver data struct
 * @param[in] index: which dsi (dsi0 ~ dsi3)
 * @return
 *     True, clock lane HS mode is enable. \n
 *     False, clock lane HS mode is disable.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static bool mtk_dsi_clk_hs_state(const struct mtk_dsi *dsi, u32 index)
{
	u32 tmp_reg1;

	tmp_reg1 = readl(dsi->regs[index] + DSI_PHY_LCCON);
	return ((tmp_reg1 & LC_HSTX_EN) == 1) ? true : false;
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Setup clock lane HS mode.
 * @param[in] dsi: dsi driver data struct
 * @param[in] enter: enable clock lane HS mode or not
 * @return none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsi_clk_hs_mode(const struct mtk_dsi *dsi, bool enter)
{
	u32 i;

	for (i = 0; i < dsi->hw_nr; i++) {
		if (enter && !mtk_dsi_clk_hs_state(dsi, i))
			mtk_dsi_mask(dsi->regs[i], DSI_PHY_LCCON, LC_HSTX_EN,
				     LC_HSTX_EN);
		else if (!enter && mtk_dsi_clk_hs_state(dsi, i))
			mtk_dsi_mask(dsi->regs[i], DSI_PHY_LCCON, LC_HSTX_EN,
				     0);
	}
	if (enter)
		usleep_range(2, 4);
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Setup dsi operation mode. \n
 *     CMD_MODE: used for command mode display or send dsi packet \n
 *     SYNC_PULSE_MODE: used for video mode display \n
 *     SYNC_EVENT_MODE: used for video mode display \n
 *     BURST_MODE: used for video mode display
 * @param[in] dsi: dsi driver data struct
 * @return none
 * @par Boundary case and Limitation
 *     Which mode is used need to check panel spec, \n
 *     and set mode_flags in panel driver. \n
 *     Please call lcm_get_params() to get parameter before setup dsi.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsi_set_mode(const struct mtk_dsi *dsi)
{
	u32 vid_mode = CMD_MODE;
	u32 i;

	if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO) {
		if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO_BURST)
			vid_mode = BURST_MODE;
		else if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE)
			vid_mode = SYNC_PULSE_MODE;
		else
			vid_mode = SYNC_EVENT_MODE;
	}

	for (i = 0; i < dsi->hw_nr; i++)
		writel(vid_mode, dsi->regs[i] + DSI_MODE_CON);
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Enable send command in VFP during video mode display. \n
 *     Enable this function when you want to send command to panel during dsi \n
 *     in action(video) mode.
 * @param[in] dsi: dsi driver data struct
 * @return none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsi_set_vm_cmd(const struct mtk_dsi *dsi)
{
	u32 i;

	for (i = 0; i < dsi->hw_nr; i++) {
		mtk_dsi_mask(dsi->regs[i], DSI_VM_CMD_CON, VM_CMD_EN,
			     VM_CMD_EN);
		mtk_dsi_mask(dsi->regs[i], DSI_VM_CMD_CON, TS_VFP_EN,
			     TS_VFP_EN);
		mtk_dsi_mask(dsi->regs[i], DSI_VM_CMD_CON, TIME_SEL,
			     TIME_SEL);
	}
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Setup frame size related register.
 * @param[in] dsi: dsi driver data struct
 * @return none
 * @par Boundary case and Limitation
 *     Please call lcm_get_params() to get parameter before setup dsi.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsi_ps_control_vact(struct mtk_dsi *dsi)
{
	struct videomode *vm = &dsi->vm;
	u32 dsi_buf_bpp, ps_wc, hsize;
	u32 ps_bpp_mode;
	u32 i;

	hsize = vm->hactive;
	dsi_buf_bpp = 24;
	if (dsi->format == MTK_DSI_FMT_COMPRESSION_30_15)
		dsi_buf_bpp = 30 / 2;
	else if (dsi->format == MTK_DSI_FMT_COMPRESSION_30_10)
		dsi_buf_bpp = 30 / 3;
	else if (dsi->format == MTK_DSI_FMT_COMPRESSION_24_12)
		dsi_buf_bpp = 24 / 2;
	else if (dsi->format == MTK_DSI_FMT_COMPRESSION_24_8)
		dsi_buf_bpp = 24 / 3;
	else if (dsi->format == MTK_DSI_FMT_RGB565)
		dsi_buf_bpp = 16;
	else if (dsi->format == MTK_DSI_FMT_RGB666)
		dsi_buf_bpp = 18;
	else if (dsi->format == MTK_DSI_FMT_RGB666_LOOSELY)
		dsi_buf_bpp = 24;
	else if (dsi->format == MTK_DSI_FMT_RGB101010)
		dsi_buf_bpp = 30;

	ps_wc = (hsize * dsi_buf_bpp) / 8;
	ps_bpp_mode = ps_wc;

	switch (dsi->format) {
	case MTK_DSI_FMT_RGB101010:
		ps_bpp_mode |= PACKED_PS_30BIT_RGB101010;
		break;
	case MTK_DSI_FMT_RGB888:
		ps_bpp_mode |= PACKED_PS_24BIT_RGB888;
		break;
	case MTK_DSI_FMT_RGB666_LOOSELY:
		ps_bpp_mode |= LOOSELY_PS_24BIT_RGB666;
		break;
	case MTK_DSI_FMT_RGB666:
		ps_bpp_mode |= PACKED_PS_18BIT_RGB666;
		break;
	case MTK_DSI_FMT_RGB565:
		ps_bpp_mode |= PACKED_PS_16BIT_RGB565;
		break;
	case MTK_DSI_FMT_COMPRESSION_30_15:
	case MTK_DSI_FMT_COMPRESSION_30_10:
	case MTK_DSI_FMT_COMPRESSION_24_12:
	case MTK_DSI_FMT_COMPRESSION_24_8:
		hsize = (ps_wc + 2) / 3;
		ps_bpp_mode |= COMPRESSION;
		break;
	default:
		ps_bpp_mode |= PACKED_PS_24BIT_RGB888;
		break;
	}

	for (i = 0; i < dsi->hw_nr; i++) {
		writel(vm->vactive, dsi->regs[i] + DSI_VACT_NL);
		writel(ps_bpp_mode, dsi->regs[i] + DSI_PSCON);
		writel(ps_wc, dsi->regs[i] + DSI_HSTX_CKLP_WC);
		writel(((hsize) | (vm->vactive << 0x10)),
		       dsi->regs[i] + DSI_SIZE_CON);
	}
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Config lane and some dsi basic function. \n
 *     - non continuius clock \n
 *     - eotp
 * @param[in] dsi: dsi driver data struct
 * @return none
 * @par Boundary case and Limitation
 *     Please call lcm_get_params() to get parameter before setup dsi.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsi_rxtx_control(const struct mtk_dsi *dsi)
{
	struct mtk_mipi_tx *mipi_tx = phy_get_drvdata(dsi->phy);
	u32 tmp_reg = 0;
	u32 i;

	switch (dsi->lanes) {
	case 1:
		tmp_reg = 1 << 2;
		break;
	case 2:
		tmp_reg = 3 << 2;
		break;
	case 3:
		tmp_reg = 7 << 2;
		break;
	case 4:
		tmp_reg = 0xf << 2;
		break;
	default:
		tmp_reg = 0xf << 2;
		break;
	}

	tmp_reg |= (dsi->mode_flags & MIPI_DSI_CLOCK_NON_CONTINUOUS) << 6;

	if (mipi_tx->phy_type == MTK_MIPI_DPHY)
		tmp_reg |= (dsi->mode_flags & MIPI_DSI_MODE_EOT_PACKET) >> 3;
	else
		tmp_reg |= HSTX_DIS_EOT;

	for (i = 0; i < dsi->hw_nr; i++) {
		writel(tmp_reg, dsi->regs[i] + DSI_TXRX_CON);
		mtk_dsi_mask(dsi->regs[i], DSI_DEBUG_SEL, FIFO_MEMORY_TYPE,
			     FIFO_MEMORY_TYPE_SET(2));
		mtk_dsi_mask(dsi->regs[i], DSI_BIST_CON, FIFOPRE_CNT_CLR,
			     FIFOPRE_CNT_CLR);
		mtk_dsi_mask(dsi->regs[i], DSI_BIST_CON, SELF_PAT_READY_ON,
			     SELF_PAT_READY_ON);

		if (dsi->fpga_mode == 1)
			writel(PHY_LCPAT_INV, dsi->regs[i] + DSI_PHY_LCPAT);
	}
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Setup video mode display timing. \n
 *     For example : D-PHY with sync pulse mode \n
 *     HSA_WC = HSA * BPP - 10 \n
 *     HBP_WC = HBP * BPP - 10 \n
 *     HFP_WC = HFP * BPP - 12 - data_init_cycle * lane_num \n
 *     data_init_cycle = T_hs-exit + T_lpx + T_hs-prep + T_hs-zero + 2
 * @param[in] dsi: dsi driver data struct
 * @return none
 * @par Boundary case and Limitation
 *     Please call lcm_get_params() to get parameter before setup dsi.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsi_config_vdo_timing(struct mtk_dsi *dsi)
{
	struct mtk_mipi_tx *mipi_tx = phy_get_drvdata(dsi->phy);
	u32 horizontal_sync_active_byte;
	u32 horizontal_backporch_byte;
	u32 horizontal_frontporch_byte;
	u32 horizontal_blanklowpower_byte;
	u32 dsi_tmp_buf_bpp;
	u32 data_init_byte;
	u32 i;

	struct videomode *vm = &dsi->vm;

	if (dsi->format == MTK_DSI_FMT_RGB565)
		dsi_tmp_buf_bpp = 16;
	else if ((dsi->format == MTK_DSI_FMT_RGB101010) ||
		 (dsi->format == MTK_DSI_FMT_COMPRESSION_30_15) ||
		 (dsi->format == MTK_DSI_FMT_COMPRESSION_30_10))
		dsi_tmp_buf_bpp = 30;
	else
		dsi_tmp_buf_bpp = 24;

	if (mipi_tx->phy_type == MTK_MIPI_DPHY) {
		data_init_byte = dsi->data_init_cycle * dsi->lanes;

		horizontal_sync_active_byte =
		      (((vm->hsync_len * dsi_tmp_buf_bpp) / 8) - 10);

		if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE)
			horizontal_backporch_byte =
			      (((vm->hback_porch * dsi_tmp_buf_bpp) / 8) - 10);
		else
			horizontal_backporch_byte =
			      ((((vm->hback_porch + vm->hsync_len) *
			      dsi_tmp_buf_bpp) / 8) - 10);

		horizontal_frontporch_byte =
			      (((vm->hfront_porch * dsi_tmp_buf_bpp) / 8) - 12);

		horizontal_blanklowpower_byte = 16 * dsi->lanes;
	} else {
		data_init_byte = dsi->data_init_cycle * 2 * dsi->lanes;

		horizontal_sync_active_byte =
			      (((vm->hsync_len * dsi_tmp_buf_bpp) / 8) - 36);

		if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE)
			horizontal_backporch_byte =
			      (((vm->hback_porch * dsi_tmp_buf_bpp) / 8) - 38);
		else
			horizontal_backporch_byte =
			      ((((vm->hback_porch + vm->hsync_len) *
			      dsi_tmp_buf_bpp) / 8) - 38);

		horizontal_frontporch_byte =
			      (((vm->hfront_porch * dsi_tmp_buf_bpp) / 8) - 38);

		horizontal_blanklowpower_byte = 16 * dsi->lanes; /* need CHK */
	}

	if (horizontal_frontporch_byte > data_init_byte) {
		horizontal_frontporch_byte -= data_init_byte;
	} else {
		horizontal_frontporch_byte = 4;
		dev_warn(dsi->dev, "hfp is too short!\n");
	}

	for (i = 0; i < dsi->hw_nr; i++) {
		writel(vm->vsync_len, dsi->regs[i] + DSI_VSA_NL);
		writel(vm->vback_porch, dsi->regs[i] + DSI_VBP_NL);
		writel(vm->vfront_porch, dsi->regs[i] + DSI_VFP_NL);
		writel(vm->vactive, dsi->regs[i] + DSI_VACT_NL);

		writel(horizontal_sync_active_byte, dsi->regs[i] + DSI_HSA_WC);
		writel(horizontal_backporch_byte, dsi->regs[i] + DSI_HBP_WC);
		writel(horizontal_frontporch_byte, dsi->regs[i] + DSI_HFP_WC);
		writel(horizontal_blanklowpower_byte,
		       dsi->regs[i] + DSI_BLLP_WC);
	}

	mtk_dsi_ps_control_vact(dsi);
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Enable multi-dsi mode when use more than one dsi. \n
 *     First dsi is master, others are slave and need enable DUAL_EN
 * @param[in] dsi: dsi driver data struct
 * @param[in] enable: enable or disable multi-dsi mode
 * @return none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsi_multi(const struct mtk_dsi *dsi, bool enable)
{
	u32 i;

	if (dsi->hw_nr <= 1)
		return;

	for (i = 1; i < dsi->hw_nr; i++) {
		if (enable)
			mtk_dsi_mask(dsi->regs[i], DSI_COM_CON, DSI_DUAL_EN,
				     DSI_DUAL_EN);
		else
			mtk_dsi_mask(dsi->regs[i], DSI_COM_CON, DSI_DUAL_EN,
				     0);
	}
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     DSI hw mute function.
 * @param[in] dsi: dsi driver data struct
 * @param[in] enable: enable or disable hw mute function
 * @return none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsi_hw_mute(const struct mtk_dsi *dsi, bool enable)
{
	u32 i;

	for (i = 0; i < dsi->hw_nr; i++) {
		if (enable)
			mtk_dsi_mask(dsi->regs[i], DSI_BIST_CON, HW_MUTE_EN,
				     HW_MUTE_EN);
		else
			mtk_dsi_mask(dsi->regs[i], DSI_BIST_CON, HW_MUTE_EN, 0);
	}
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     DSI sw mute function.
 * @param[in] dsi: dsi driver data struct
 * @param[in] enable: enable or disable sw mute function
 * @param[in] handle: command queue handler
 * @return none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsi_sw_mute(const struct mtk_dsi *dsi, bool enable,
			    struct cmdq_pkt *handle)
{
	u32 i;

	for (i = 0; i < dsi->hw_nr; i++) {
		if (!handle) {
			if (enable)
				mtk_dsi_mask(dsi->regs[i], DSI_BIST_CON,
					     SW_MUTE_EN, SW_MUTE_EN);
			else
				mtk_dsi_mask(dsi->regs[i], DSI_BIST_CON,
					     SW_MUTE_EN, 0);
		} else {
			if (enable)
				cmdq_pkt_write_value(
				      handle, dsi->gce_subsys,
				      dsi->gce_subsys_offset[i] + DSI_BIST_CON,
				      SW_MUTE_EN,  SW_MUTE_EN);
			else
				cmdq_pkt_write_value(
				      handle, dsi->gce_subsys,
				      dsi->gce_subsys_offset[i] + DSI_BIST_CON,
				      0, SW_MUTE_EN);
		}
	}
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     DSI interrupt enable function.
 * @param[in] dsi: dsi driver data struct
 * @return none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsi_set_interrupt_enable(const struct mtk_dsi *dsi)
{
	u32 inten = DSI_INT_ALL_BITS;

	if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO)
		inten &= ~(DSI_INT_TE_RDY_FLAG);

	writel(inten, dsi->regs[0] + DSI_INTEN);
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     DSI Start function. \n
 *     In video mode, start dsi to output video data. \n
 *     In command mode, start dsi to output command packet data.
 * @param[in] dsi: dsi driver data struct
 * @param[in] handle: command queue handler
 * @return none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsi_start(const struct mtk_dsi *dsi, struct cmdq_pkt *handle)
{
	if (!handle) {
		writel(0, dsi->regs[0] + DSI_START);
		writel(1, dsi->regs[0] + DSI_START);
	} else {
		cmdq_pkt_write_value(handle, dsi->gce_subsys,
				     dsi->gce_subsys_offset[0] + DSI_START,
				     0,  0xffffffff);
		cmdq_pkt_write_value(handle, dsi->gce_subsys,
				     dsi->gce_subsys_offset[0] + DSI_START,
				     1,  0xffffffff);
	}
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Stop dsi output data.
 * @param[in] dsi: dsi driver data struct
 * @return none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsi_stop(const struct mtk_dsi *dsi)
{
	writel(0, dsi->regs[0] + DSI_START);
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Enable dsi command mode. \n
 *     This mode use for Command mode display \n
 *     and send packet by using mtk_dsi_host_write_cmd function
 * @param[in] dsi: dsi driver data struct
 * @return none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsi_set_cmd_mode(const struct mtk_dsi *dsi)
{
	u32 i;

	for (i = 0; i < dsi->hw_nr; i++)
		mtk_dsi_mask(dsi->regs[i], DSI_MODE_CON, MODE_CON, CMD_MODE);
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Wait dsi previous operation done.
 * @param[in] dsi: dsi driver data struct
 * @return none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsi_wait_for_idle(const struct mtk_dsi *dsi)
{
	u32 timeout_ms = 500000; /* total 1s ~ 2s timeout */

	while (timeout_ms) {
		if (!(readl(dsi->regs[0] + DSI_INTSTA) & DSI_BUSY))
			break;

		usleep_range(2, 4);
		timeout_ms--;
	}

	if (timeout_ms == 0) {
		dev_err(dsi->dev, "polling dsi wait not busy timeout!\n");

		mtk_dsi_reset_engine(dsi);
		mtk_dsi_dump_registers(dsi);
	}
}
/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Power off phy and clock.
 * @param[in] dsi: dsi driver data struct
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsi_poweroff(struct mtk_dsi *dsi)
{
	u32 i;

	if (!dsi->poweron)
		return;

	mtk_dsi_reset_engine(dsi);
	mtk_dsi_lane0_ulp_mode_enter(dsi);
	mtk_dsi_clk_ulp_mode_enter(dsi);
	mdelay(1);
	for (i = 0; i < dsi->hw_nr; i++)
		mtk_dsi_mask(dsi->regs[i], DSI_TXRX_CON, LANE_NUM, 0);

#ifdef CONFIG_COMMON_CLK_MT3612
	for (i = 0; i < dsi->hw_nr; i++) {
		clk_disable_unprepare(dsi->mm_clk[i]);
		clk_disable_unprepare(dsi->dsi_clk[i]);
	}
#endif

	phy_power_off(dsi->phy);

#ifdef CONFIG_COMMON_CLK_MT3612
	pm_runtime_put(dsi->dev);
#endif

	dsi->poweron = false;
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Enable dsi hw - this is dsi initial function. \n
 *     In this function, it power on and config mipi tx and dsi. \n
 *     After this function initial done, dsi is ready to display. \n
 *     Before calling this api, please be sure that the reference clock \n
 *     of mipi tx is already configured and enabled by calling ddds api.
 * @param[in] dsi: dsi driver data struct
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsi_enable(struct mtk_dsi *dsi)
{
	int ret;

	if (dsi->enabled)
		return;

	ret = mtk_dsi_poweron(dsi);
	if (ret < 0) {
		dev_err(dsi->dev, "failed to power on dsi\n");
		return;
	}

	usleep_range(20000, 21000);

	mtk_dsi_rxtx_control(dsi);
	mtk_dsi_ps_control_vact(dsi);
	mtk_dsi_set_vm_cmd(dsi);
	mtk_dsi_config_vdo_timing(dsi);
	mtk_dsi_set_interrupt_enable(dsi);
	mtk_dsi_hw_mute(dsi, 0);

	mtk_dsi_clk_ulp_mode_leave(dsi);
	mtk_dsi_lane0_ulp_mode_leave(dsi);
	mtk_dsi_clk_hs_mode(dsi, 0);

	mtk_dsi_multi(dsi, 1);

	mtk_dsi_set_mode(dsi);
	mtk_dsi_clk_hs_mode(dsi, 1);

	dsi->enabled = true;
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Disable dsi hw - this is dsi finalize function. \n
 *     In this function, it power off dsi. \n
 * @param[in] dsi: dsi driver data struct
 * @return
 *     none
 * @par Boundary case and Limitation
 *     You have to stop display before call this function.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsi_disable(struct mtk_dsi *dsi)
{
	if (!dsi->enabled)
		return;

	mtk_dsi_multi(dsi, 0);
	mtk_dsi_clk_hs_mode(dsi, 0);
	mtk_dsi_hw_mute(dsi, 0);
	mtk_dsi_poweroff(dsi);

	dsi->enabled = false;
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Setup self pattern mode.
 * @param[in] dsi: dsi driver data struct
 * @param[in] enable: enable self pattern mode
 * @param[in] color: color pattern
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsi_self_pattern_mode(const struct mtk_dsi *dsi, bool enable,
				      u32 color)
{
	u32 i;

	for (i = 0; i < dsi->hw_nr; i++) {
		if (enable) {
			writel(color, dsi->regs[i] + DSI_BIST_PATTERN);
			mtk_dsi_mask(dsi->regs[i], DSI_BIST_CON,
				     SELF_PAT_PRE_MODE, SELF_PAT_PRE_MODE);
		} else {
			mtk_dsi_mask(dsi->regs[i], DSI_BIST_CON,
				     SELF_PAT_PRE_MODE, 0);
		}
	}
}

/*
 * for panel use
 */
/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Panel reset function. \n
 *     Control GPIO to reset panel.
 * @param[in] dsi: dsi driver data struct
 * @return none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mtk_dsi_panel_reset(const struct mtk_dsi *dsi)
{
	/* Reserved for ASIC */
	u32 flags;

	flags = MMSYS_LCM0_RST | MMSYS_LCM2_RST;

	mtk_mmsys_cfg_reset_lcm(dsi->mmsys_cfg_dev, false, flags);
	udelay(50);
	mtk_mmsys_cfg_reset_lcm(dsi->mmsys_cfg_dev, true, flags);
	udelay(50);
	mtk_mmsys_cfg_reset_lcm(dsi->mmsys_cfg_dev, false, flags);
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Send dsi packet function. \n
 *     This function can send dcs short/long packet.
 * @param[in] dsi: dsi driver data struct
 * @param[in] cmd: dcs command
 * @param[in] count: dcs parameter length
 * @param[in] para_list: dcs parameter
 * @return
 *     0, it means packet is send.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
ssize_t mtk_dsi_host_write_cmd(const struct mtk_dsi *dsi, u32 cmd, u8 count,
			       const u8 *para_list)
{
	u32 i, j;
	size_t mask_para, set_para, reg_val;
	size_t goto_addr;
	void __iomem *cmdq_reg;
	struct dsi_cmd_t0 t0;
	struct dsi_cmd_t2 t2;
	struct dsi_tx_cmdq_regs *dsi_cmd_reg;

	mtk_dsi_wait_for_idle(dsi);

	for (i = 0; i < dsi->hw_nr; i++) {
		dsi_cmd_reg =
		    (struct dsi_tx_cmdq_regs *)(dsi->regs[i] + DSI_CMDQ);

		if (count > 1) {
			t2.config = 2;
			t2.type = MIPI_DSI_DCS_LONG_WRITE;
			t2.wc16 = count + 1;

			reg_val = ((u32)t2.wc16 << 16) |
				  (t2.type << 8) | t2.config;

			writel(reg_val, &dsi_cmd_reg->data[0]);

			goto_addr = (size_t)(&dsi_cmd_reg->data[1].byte0);
			mask_para = (0xff << ((goto_addr & 0x3) * 8));
			set_para = (cmd << ((goto_addr & 0x3) * 8));
			cmdq_reg = (void __iomem *)(goto_addr & (~0x3));
			mtk_dsi_set_cmdq(cmdq_reg, mask_para, set_para);

			for (j = 0; j < count; j++) {
				goto_addr =
				    (size_t)(&dsi_cmd_reg->data[1].byte1) + j;
				mask_para = (0xff << ((goto_addr & 0x3) * 8));
				set_para =
				 ((u32)para_list[j] << ((goto_addr & 0x3) * 8));
				cmdq_reg = (void __iomem *)(goto_addr & (~0x3));
				mtk_dsi_set_cmdq(cmdq_reg, mask_para, set_para);
			}

			mtk_dsi_mask(dsi->regs[i], DSI_CMDQ_CON, CMDQ_SIZE,
				     2 + (count) / 4);
		} else {
			t0.config = 0;
			t0.data0 = cmd;
			if (count) {
				t0.type = MIPI_DSI_DCS_SHORT_WRITE_PARAM;
				t0.data1 = para_list[0];
			} else {
				t0.type = MIPI_DSI_DCS_SHORT_WRITE;
				t0.data1 = 0;
			}

			reg_val = ((u32)t0.data1 << 24) | (t0.data0 << 16) |
				  (t0.type << 8) | t0.config;

			writel(reg_val, &dsi_cmd_reg->data[0]);
			mtk_dsi_mask(dsi->regs[i], DSI_CMDQ_CON, CMDQ_SIZE, 1);
		}
	}

	mtk_dsi_start(dsi, NULL);
	mtk_dsi_wait_for_idle(dsi);

	return 0;
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Read dsi packet function. \n
 *     This function can read dcs short packet.
 * @param[in] dsi: dsi driver data struct.
 * @param[in] cmd: dcs command.
 * @param[in] pkt_size: set return packet size.
 * @param[in] read_data: dcs read data.
 * @return
 *     0, packet is read.
 *     -1, dsi wait not busy timeout.
 *     -2, acknowledge & error report.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
ssize_t mtk_dsi_host_read_cmd(const struct mtk_dsi *dsi, u32 cmd, u8 pkt_size,
			      struct dsi_read_data *read_data)
{
	u32 i, j, timeout;
	u32 data[4];
	u8 type = 0;
	u16 count;
	int try_cnt, ret = 0;
	size_t reg[2];
	struct dsi_cmd_t0 t0[2];
	struct dsi_tx_cmdq_regs *dsi_cmd_reg;

	mtk_dsi_wait_for_idle(dsi);
	mtk_dsi_multi(dsi, false);

	pkt_size = pkt_size > 10 ? 10 : pkt_size;

	t0[0].config = 0;
	t0[0].data0 = pkt_size;
	t0[0].type = MIPI_DSI_SET_MAXIMUM_RETURN_PACKET_SIZE;
	t0[0].data1 = 0;

	t0[1].config = 0x04;  /* BTA */
	t0[1].data0 = cmd;
	t0[1].type = MIPI_DSI_DCS_READ;
	t0[1].data1 = 0;

	reg[0] = ((u32)t0[0].data1 << 24) | (t0[0].data0 << 16) |
		 (t0[0].type << 8) | t0[0].config;

	reg[1] = ((u32)t0[1].data1 << 24) | (t0[1].data0 << 16) |
		 (t0[1].type << 8) | t0[1].config;

	for (i = 0; i < dsi->hw_nr; i++) {
		dsi_cmd_reg =
		    (struct dsi_tx_cmdq_regs *)(dsi->regs[i] + DSI_CMDQ);

		try_cnt = 3;

		do {
			writel(reg[0], &dsi_cmd_reg->data[0]);
			writel(reg[1], &dsi_cmd_reg->data[1]);
			mtk_dsi_mask(dsi->regs[i], DSI_CMDQ_CON, CMDQ_SIZE, 2);
			mtk_dsi_mask(dsi->regs[i], DSI_INTSTA, DSI_INT_ALL_BITS,
				     0);

			writel(0, dsi->regs[i] + DSI_START);
			writel(1, dsi->regs[i] + DSI_START);

			timeout = 500000; /* total 1s ~ 2s timeout */

			while (timeout) {
				if (!(readl(dsi->regs[i] + DSI_INTSTA)
				      & DSI_BUSY)) {
					break;
				}
				mtk_dsi_mask(dsi->regs[i], DSI_RX_RACK, RACK,
					     RACK);

				usleep_range(2, 4);
				timeout--;
			}

			writel(0, dsi->regs[i] + DSI_START);
			mtk_dsi_mask(dsi->regs[i], DSI_INTSTA, DSI_INT_ALL_BITS,
				     0);

			if (timeout == 0) {
				dev_err(dsi->dev,
					"dsi[%d] wait not busy timeout!\n", i);
				ret = -1;
				break;
			}

			data[0] = readl(dsi->regs[i] + DSI_RX_DATA03);
			data[1] = readl(dsi->regs[i] + DSI_RX_DATA47);
			data[2] = readl(dsi->regs[i] + DSI_RX_DATA8B);
			data[3] = readl(dsi->regs[i] + DSI_RX_DATAC);

			type = data[0];
			count = data[0] >> 8;

			dev_dbg(dsi->dev,
				"dsi[%d], type = 0x%x, count = %d, try_cnt = %d\n",
				i, type, count, try_cnt);

			for (j = 0; j < pkt_size; j++) {
				read_data->byte[i][j] = data[j / 4 + 1]
							>> (8 * (j % 4));
			}
		} while (type == 0x02 && --try_cnt > 0);
	}

	mtk_dsi_multi(dsi, true);

	if (type == 0x02 && ret != -1)  /* 0x02: acknowledge & error report */
		ret = -2;

	return ret;
}

/*
 * for DDP use
 */
/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Initial LCM. \n
 *     This function will send init parameter to panel.
 * @param[in] dev: dsi device data struct.
 * @param[in] table: panel init patameter table, null for default table.
 * @param[in] count: number of init patameter (should be equal to size of \n
 *     table).
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EIO, device not enable.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. The device struct is not assigned, return -EFAULT. \n
 *     2. The device not enable, return -EIO.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_lcm_setting(const struct device *dev,
			const struct lcm_setting_table *table, u32 count)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);

	if (!dsi->enabled)
		return -EIO;

	mtk_dsi_set_cmd_mode(dsi); /* switch to command mode */

	if (!table)
		lcm_init(dsi);
	else
		push_table(dsi, table, count);

	mtk_dsi_set_mode(dsi); /* switch back to original mode */

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_lcm_setting);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Initial LCM. \n
 *     This function will send init parameter to panel by vm cmd mode.
 * @param[in] dev: dsi device data struct.
 * @param[in] table: panel init patameter table, null for default table.
 * @param[in] count: number of init patameter (should be equal to size of \n
 *     table).
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EIO, device not enable.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. The device struct is not assigned, return -EFAULT. \n
 *     2. The device not enable, return -EIO.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_lcm_vm_setting(const struct device *dev,
			   const struct lcm_setting_table *table, u32 count)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);

	if (!dsi->enabled)
		return -EIO;

	if (!table)
		lcm_init2(dsi);
	else
		push_table2(dsi, table, count);

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_lcm_vm_setting);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     LCM reset function. \n
 *     Control GPIO to reset LCM.
 * @param[in] dev: dsi device data struct.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     The device struct is not assigned, return -EFAULT.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_lcm_reset(const struct device *dev)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);

	mtk_dsi_panel_reset(dsi);

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_lcm_reset);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Initial and enable dsi subsystem. \n
 *     This function will initial mutex, dsi, mipi tx. \n
 *     User can call this function to initial dsi subsystem but not \n
 *     output frame data. \n
 *     If user want to output frame data after call this function, \n
 *     please call mtk_dsi_output_start().
 * @param[in] dev: dsi device data struct.
 * @return
 *     0, it means initial flow is done. \n
 *     -EFAULT, invalid dev point. \n
 *     error code from mtk_dsi_mutex_init().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. Device struct is not assigned, return -EFAULT. \n
 *     2. If mtk_dsi_mutex_init() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_hw_init(const struct device *dev)
{
	struct mtk_dsi *dsi;
	int ret = 0;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);

	mtk_dsi_enable(dsi);

	return ret;
}
EXPORT_SYMBOL(mtk_dsi_hw_init);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Disable dsi subsystem. \n
 *     This function will disable mutex, dsi, mipi tx. \n
 *     User call this function to disable dsi subsystem when not to use or \n
 *     need to re-initial dsi subsystem (e.q. switch mode).
 * @param[in] dev: dsi device data struct.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address.
 * @par Boundary case and Limitation
 *     Stop dsi output before call this function.
 * @par Error case and Error handling
 *     Device struct is not assigned, return -EFAULT.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_hw_fini(const struct device *dev)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);

	mtk_dsi_disable(dsi);
	return 0;
}
EXPORT_SYMBOL(mtk_dsi_hw_fini);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Config lane number. \n
 *     This function can let upper layer configure lane num. \n
 *     Default lane number is configured by device tree.
 * @param[in] dev: dsi device data struct.
 * @param[in] lane_num: dsi driver data struct.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     Call this function before mtk_dsi_hw_init(). \n
 *     Parameter, lane_num, range is 1 ~ 4.
 * @par Error case and Error handling
 *     1. The device struct is not assigned, return -EFAULT. \n
 *     2. The parameter, lane_num, is not 1 ~ 4, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_lane_config(const struct device *dev, int lane_num)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);

	if ((lane_num > 4) || (lane_num <= 0))
		return -EINVAL;

	dsi->lanes = lane_num;
	return 0;
}
EXPORT_SYMBOL(mtk_dsi_lane_config);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Config lane swap. \n
 *     This function can let upper layer configure mipi lane swap. \n
 *     Default lane swap is configured by device tree.
 * @param[in] dev: dsi device data struct.
 * @param[in] lane_swap: lane swap index.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -ENODATA, no data available. \n
 *     -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     Call this function before mtk_dsi_hw_init(). \n
 *     Parameter, lane_swap, range is 0 ~ 3.
 * @par Error case and Error handling
 *     1. The device struct is not assigned, return -EFAULT. \n
 *     2. Struct phy is not available, return -ENODATA. \n
 *     3. The parameter, lane_num, is not 1 ~ 4, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_lane_swap(const struct device *dev, int lane_swap)
{
	struct mtk_dsi *dsi;
	struct mtk_mipi_tx *mipi_tx;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);

	if (!dsi->phy)
		return -ENODATA;

	mipi_tx = phy_get_drvdata(dsi->phy);

	if ((lane_swap > 3) || (lane_swap < 0))
		return -EINVAL;

	mipi_tx->lane_swap = lane_swap;
	return 0;
}
EXPORT_SYMBOL(mtk_dsi_lane_swap);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Setup self pattern mode. \n
 *     User can use this function to output single color pattern.
 * @param[in] dev: dsi device data struct.
 * @param[in] enable: enable self pattern mode.
 * @param[in] color: color pattern (30 bit valid value).
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EIO, device not enable.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. The device struct is not assigned, return -EFAULT. \n
 *     2. The device not enable, return -EIO.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_output_self_pattern(const struct device *dev, bool enable,
				u32 color)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);

	if (!dsi->enabled)
		return -EIO;

	mtk_dsi_self_pattern_mode(dsi, enable, color);

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_output_self_pattern);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     DSI start to display. \n
 *     User call this function to start output frame data.
 * @param[in] dev: dsi device data struct.
 * @param[in] handle: command queue handler.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EIO, device not enable.
 * @par Boundary case and Limitation
 *     This function could be called after hw initialized.
 * @par Error case and Error handling
 *     1. The device struct is not assigned, return -EFAULT. \n
 *     2. The device not enable, return -EIO.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_output_start(const struct device *dev, struct cmdq_pkt *handle)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);

	if (!dsi->enabled)
		return -EIO;

	mtk_dsi_start(dsi, handle);

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_output_start);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     DSI stop to display. \n
 *     User call this function to stop output frame data.
 * @param[in] dev: dsi device data struct.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EIO, device not enable.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. The device struct is not assigned, return -EFAULT. \n
 *     2. The device not enable, return -EIO.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_output_stop(const struct device *dev)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);

	if (!dsi->enabled)
		return -EIO;

	mtk_dsi_stop(dsi);

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_output_stop);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Get roughly frame rate. \n
 *     This function use current data rate to calculate fps.
 * @param[in] dev: dsi device data struct.
 * @param[out] fps: frame rate, unit is Hz.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EINVAL, invalid parameter or setting.
 * @par Boundary case and Limitation
 *     Call this function after dsi init finished.
 * @par Error case and Error handling
 *     1. The device struct is not assigned, return -EFAULT. \n
 *     2. The lanes, data_rate are not assigned, return -EINVAL. \n
 *     3. The frame_time is zero, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_get_fps(const struct device *dev, u32 *fps)
{
	struct mtk_dsi *dsi;
	struct videomode vm;
	u32 hactive;
	u32 dsi_buf_bpp;
	u32 line_time;
	u32 frame_time;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);
	vm = dsi->vm;

	if (WARN_ON(dsi->lanes == 0))
		return -EINVAL;
	if (WARN_ON(dsi->data_rate == 0))
		return -EINVAL;

	if ((dsi->format == MTK_DSI_FMT_COMPRESSION_30_15) ||
	    (dsi->format == MTK_DSI_FMT_COMPRESSION_24_12))
		hactive = vm.hactive / 2;
	else if ((dsi->format == MTK_DSI_FMT_COMPRESSION_30_10) ||
		 (dsi->format == MTK_DSI_FMT_COMPRESSION_24_8))
		hactive = vm.hactive / 3;
	else
		hactive = vm.hactive;

	if (dsi->format == MTK_DSI_FMT_RGB565)
		dsi_buf_bpp = 16;
	else if ((dsi->format == MTK_DSI_FMT_RGB101010) ||
		 (dsi->format == MTK_DSI_FMT_COMPRESSION_30_15) ||
		 (dsi->format == MTK_DSI_FMT_COMPRESSION_30_10))
		dsi_buf_bpp = 30;
	else
		dsi_buf_bpp = 24;

	line_time =
	    (hactive + vm.hsync_len + vm.hback_porch + vm.hfront_porch) *
	    dsi_buf_bpp;
	frame_time =
	    line_time *
	    (vm.vactive + vm.vsync_len + vm.vback_porch + vm.vfront_porch);

	if (WARN_ON(frame_time == 0))
		return -EINVAL;

	*fps = DIV_ROUND_CLOSEST(dsi->data_rate * 1000,
				 frame_time / dsi->lanes);

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_get_fps);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Set frame rate. \n
 *     This function will use fps to calculate data rate per lane. \n
 *     User call this function to setup data rate by input fps, \n
 *     then phy pll will generate clock according to this data rate \n
 *     in hw initial flow (mtk_dsi_hw_init()).
 * @param[in] dev: dsi device data struct.
 * @param[in] fps: target frame per second (less than or equal to 120).
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     Call this function before mtk_dsi_hw_init().
 * @par Error case and Error handling
 *     1. The device struct is not assigned, return -EFAULT. \n
 *     2. The lanes is not assigned or fps over 120, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_set_fps(const struct device *dev, enum mtk_dsi_fps fps)
{
	struct mtk_dsi *dsi;
	struct videomode vm;
	u32 hactive, dsi_buf_bpp;
	u32 line_time, frame_time;

	if (!dev)
		return -EFAULT;

	if ((fps > MTK_DSI_FPS_120) || (fps == 0))
		return -EINVAL;

	dsi = dev_get_drvdata(dev);
	vm = dsi->vm;

	if (WARN_ON(dsi->lanes == 0))
		return -EINVAL;

	if ((dsi->format == MTK_DSI_FMT_COMPRESSION_30_15) ||
	    (dsi->format == MTK_DSI_FMT_COMPRESSION_24_12))
		hactive = vm.hactive / 2;
	else if ((dsi->format == MTK_DSI_FMT_COMPRESSION_30_10) ||
		 (dsi->format == MTK_DSI_FMT_COMPRESSION_24_8))
		hactive = vm.hactive / 3;
	else
		hactive = vm.hactive;

	if (dsi->format == MTK_DSI_FMT_RGB565)
		dsi_buf_bpp = 16;
	else if ((dsi->format == MTK_DSI_FMT_RGB101010) ||
		 (dsi->format == MTK_DSI_FMT_COMPRESSION_30_15) ||
		 (dsi->format == MTK_DSI_FMT_COMPRESSION_30_10))
		dsi_buf_bpp = 30;
	else
		dsi_buf_bpp = 24;

	line_time =
	    (hactive + vm.hsync_len + vm.hback_porch + vm.hfront_porch) *
	    dsi_buf_bpp;
	frame_time =
	    line_time *
	    (vm.vactive + vm.vsync_len + vm.vback_porch + vm.vfront_porch);

	dsi->data_rate =
	    ((frame_time / dsi->lanes) * ((u32)fps) + 999) / 1000;

	dev_dbg(dev, "fps=%d, lanes=%d, data rate=%d.%03d MHz\n",
		fps, dsi->lanes, dsi->data_rate / 1000, dsi->data_rate % 1000);

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_set_fps);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Get lcm width (pixel). \n
 *     Real display width is (1 dsi display width) * (total dsi number) \n
 *     This value should be set in panel driver and \n
 *     user call this function to get lcm width.
 * @param[in] dev: dsi device data struct.
 * @param[out] width: lcm width, unit is pixel.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Device struct is not assigned, return -EFAULT.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_get_lcm_width(const struct device *dev, u32 *width)
{
	struct mtk_dsi *dsi;
	struct videomode vm;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);
	vm = dsi->vm;
	*width = (vm.hactive * dsi->hw_nr);

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_get_lcm_width);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Get lcm height (line). \n
 *     This value should be set in panel driver and \n
 *     user call this function to get lcm height.
 * @param[in] dev: dsi device data struct.
 * @param[out] height: lcm height, unit is line.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Device struct is not assigned, return -EFAULT.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_get_lcm_height(const struct device *dev, u32 *height)
{
	struct mtk_dsi *dsi;
	struct videomode vm;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);
	vm = dsi->vm;
	*height = vm.vactive;

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_get_lcm_height);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Get lcm vfrontporch (line). \n
 *     This value should be set in panel driver and \n
 *     user call this function to get lcm vfrontporch.
 * @param[in] dev: dsi device data struct.
 * @param[out] frontporch: lcm vfrontporch, unit is line.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Device struct is not assigned, return -EFAULT.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_get_lcm_vfrontporch(const struct device *dev,
				u32 *frontporch)
{
	struct mtk_dsi *dsi;
	struct videomode vm;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);
	vm = dsi->vm;
	*frontporch = vm.vfront_porch;

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_get_lcm_vfrontporch);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Get lcm vtotal (line). \n
 *     This value should be set in panel driver and \n
 *     user call this function to get lcm vtotal.
 * @param[in] dev: dsi device data struct.
 * @param[out] vtotal: lcm vtotal, unit is line.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Device struct is not assigned, return -EFAULT.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_get_lcm_vtotal(const struct device *dev,
			   u32 *vtotal)
{
	struct mtk_dsi *dsi;
	struct videomode vm;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);
	vm = dsi->vm;
	*vtotal = vm.vfront_porch + vm.vactive +
		vm.vback_porch + vm.vsync_len;

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_get_lcm_vtotal);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Set lcm width (pixel). \n
 *     User can call this function to set lcm width.
 * @param[in] dev: dsi device data struct.
 * @param[in] width: lcm width, unit is pixel (less than or equal to 4320).
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. Device struct is not assigned, return -EFAULT. \n
 *     2. Dsi hw number is zero, return -EINVAL. \n
 *     3. The input parameters are invalid(should not be zero), return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_set_lcm_width(const struct device *dev, u32 width)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);

	if (dsi->hw_nr == 0)
		return -EINVAL;

	if (width == 0 || width > 4320)
		return -EINVAL;

	dsi->vm.hactive = width / dsi->hw_nr;

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_set_lcm_width);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Set lcm height (line). \n
 *     User can call this function to set lcm height.
 * @param[in] dev: dsi device data struct.
 * @param[in] height: lcm height, unit is line (less than or equal to 2160).
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. The device struct is not assigned, return -EFAULT. \n
 *     2. The input parameters are invalid(should not be zero), return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_set_lcm_height(const struct device *dev, u32 height)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	if (height == 0 || height > 2160)
		return -EINVAL;

	dsi = dev_get_drvdata(dev);
	dsi->vm.vactive = height;

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_set_lcm_height);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Set lcm vbackporch (line). \n
 *     User can call this function to set lcm vbackporch.
 * @param[in] dev: dsi device data struct.
 * @param[in] frontporch: lcm frontporch, unit is line.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. The device struct is not assigned, return -EFAULT. \n
 *     2. The input parameters are invalid(should not be zero), return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_set_lcm_vfrontporch(const struct device *dev,
				u32 frontporch)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	if (frontporch == 0)
		return -EINVAL;

	dsi = dev_get_drvdata(dev);
	dsi->vm.vfront_porch = frontporch;

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_set_lcm_vfrontporch);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Set lcm vsynclen, vbackporch and vfrontporch (line). \n
 *     User can call this function to set lcm vsync, vbp and vfp.
 * @param[in] dev: dsi device data struct.
 * @param[in] synclen: lcm vsynclen, unit is line.
 * @param[in] backporch: lcm vbackporch, unit is line.
 * @param[in] frontporch: lcm vfrontporch, unit is line.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. The device struct is not assigned, return -EFAULT. \n
 *     2. The input parameters are invalid(should not be zero), return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_set_lcm_vblanking(const struct device *dev, u32 synclen,
			      u32 backporch, u32 frontporch)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	if (synclen == 0 || backporch == 0 || frontporch == 0)
		return -EINVAL;

	dsi = dev_get_drvdata(dev);
	dsi->vm.vsync_len = synclen;
	dsi->vm.vback_porch = backporch;
	dsi->vm.vfront_porch = frontporch;

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_set_lcm_vblanking);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Set data rate, frame rate, lcm hsynclen and hbackporch(byte). \n
 *     hfrontporch would be calculated automatically.
 *     User can call this function to set lcm hsync, hbp and hfp.
 * @param[in] dev: dsi device data struct.
 * @param[in] datarate: current data rate, unit is kHz.
 * @param[in] framerate: current frame rate, unit is fps.
 * @param[in] synclen: lcm hsynclen, unit is byte/lane.
 * @param[in] backporch: lcm hbackporch, unit is byte/lane.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address.
 *     -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. The device struct is not assigned, return -EFAULT. \n
 *     2. The input parameters are invalid(dsi mode or format), return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_set_lcm_hblanking(const struct device *dev, u32 datarate,
			   u32 framerate, u32 synclen, u32 backporch)
{
	struct mtk_dsi *dsi;
	struct mtk_mipi_tx *mipi_tx;
	u32 buf_bpp, init_byte;
	int vtotal, htotal, hact, frontporch, depth, dscrate;
	int i, hs_wc, hbp_wc, hfp_wc, hs_trail, min_hfp;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);

	if (dsi->mode_flags == 0 ||
	    dsi->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE) {
		dev_err(dev, "not support cmd or sync pulse mode!\n");
		return -EINVAL;
	}

	mipi_tx = phy_get_drvdata(dsi->phy);

	if (mipi_tx->phy_type != MTK_MIPI_DPHY) {
		dev_err(dev, "not support non-dphy mode!\n");
		return -EINVAL;
	}

	if ((datarate < 125000) || (datarate > 2500000))
		return -EINVAL;

	if ((framerate > MTK_DSI_FPS_120) || (framerate == 0))
		return -EINVAL;

	vtotal = dsi->vm.vactive + dsi->vm.vsync_len + dsi->vm.vback_porch +
		 dsi->vm.vfront_porch;

	if (vtotal == 0)
		return -EINVAL;

	htotal = datarate * 1000 / framerate / vtotal / 8;

	if (dsi->format == MTK_DSI_FMT_COMPRESSION_30_15) {
		depth = 10;
		dscrate = 2;
	} else if (dsi->format == MTK_DSI_FMT_COMPRESSION_30_10) {
		depth = 10;
		dscrate = 3;
	} else if (dsi->format == MTK_DSI_FMT_COMPRESSION_24_12) {
		depth = 8;
		dscrate = 2;
	} else if (dsi->format == MTK_DSI_FMT_COMPRESSION_24_8) {
		depth = 8;
		dscrate = 3;
	} else if (dsi->format == MTK_DSI_FMT_RGB888) {
		depth = 8;
		dscrate = 1;
	} else {
		dev_err(dev, "not support format!\n");
		return -EINVAL;
	}

	hact = (dsi->vm.hactive * 3 * depth + (8 * dscrate * dsi->lanes - 1)) /
	       (8 * dscrate * dsi->lanes) + 1;  /* round up */

	frontporch = htotal - hact - backporch - synclen;

	hs_wc = synclen * dsi->lanes;
	hbp_wc = (synclen + backporch) * dsi->lanes - 10;
	hfp_wc = (frontporch - dsi->data_init_cycle + 1) * dsi->lanes - 12;

	hs_trail = readl(dsi->regs[0] + DSI_PHY_TIMCON0) >> 24;
	min_hfp = (hs_trail + 2) * dsi->lanes - 12;

	dev_dbg(dev, "hbp_wc=%d, hfp_wc=%d, min_hfp=%d\n",
		hbp_wc, hfp_wc, min_hfp);

	if (min_hfp > hfp_wc) {
		hfp_wc = min_hfp;
		dev_dbg(dev, "hfp_wc replaced by min_hfp\n");
	}
	for (i = 0; i < dsi->hw_nr; i++) {
		writel(hs_wc, dsi->regs[i] + DSI_HSA_WC);
		writel(hbp_wc, dsi->regs[i] + DSI_HBP_WC);
		writel(hfp_wc, dsi->regs[i] + DSI_HFP_WC);
	}

	/* byte transfer to pixel */
	if (dsi->format == MTK_DSI_FMT_RGB565)
		buf_bpp = 16;
	else if ((dsi->format == MTK_DSI_FMT_RGB101010) ||
		 (dsi->format == MTK_DSI_FMT_COMPRESSION_30_15) ||
		 (dsi->format == MTK_DSI_FMT_COMPRESSION_30_10))
		buf_bpp = 30;
	else
		buf_bpp = 24;

	if (mipi_tx->phy_type == MTK_MIPI_DPHY) {
		init_byte = dsi->data_init_cycle * dsi->lanes;

		dsi->vm.hsync_len = (hs_wc + 10) * 8 / buf_bpp;

		if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE)
			dsi->vm.hback_porch = (hbp_wc + 10) * 8 / buf_bpp;
		else
			dsi->vm.hback_porch = (hbp_wc + 10) * 8 / buf_bpp -
					      dsi->vm.hsync_len;

		dsi->vm.hfront_porch = (hfp_wc + init_byte + 12) * 8 / buf_bpp;
	} else {
		init_byte = dsi->data_init_cycle * 2 * dsi->lanes;

		dsi->vm.hsync_len = (hs_wc + 36) * 8 / buf_bpp;

		if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE)
			dsi->vm.hback_porch = (hbp_wc + 38) * 8 / buf_bpp;
		else
			dsi->vm.hback_porch = (hbp_wc + 38) * 8 / buf_bpp -
					      dsi->vm.hsync_len;

		dsi->vm.hfront_porch = (hfp_wc + init_byte + 38) * 8 / buf_bpp;
	}

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_set_lcm_hblanking);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Set lcm hsynclen, hbackporch and hfrontporch (byte). \n
 *     User can call this function to set lcm hsync, hbp and hfp.
 * @param[in] dev: dsi device data struct.
 * @param[in] synclen: lcm hsynclen, unit is byte.
 * @param[in] backporch: lcm hbackporch, unit is byte.
 * @param[in] frontporch: lcm hfrontporch, unit is byte.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address.
 *     -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. The device struct is not assigned, return -EFAULT. \n
 *     2. The input parameters are invalid(should not be zero), return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_set_lcm_hblank(const struct device *dev, u32 synclen,
			   u32 backporch, u32 frontporch)
{
	struct mtk_dsi *dsi;
	struct mtk_mipi_tx *mipi_tx;
	u32 buf_bpp, init_byte;
	int i;

	if (!dev)
		return -EFAULT;

	if (synclen == 0 || backporch == 0 || frontporch == 0)
		return -EINVAL;

	dsi = dev_get_drvdata(dev);

	mipi_tx = phy_get_drvdata(dsi->phy);

	for (i = 0; i < dsi->hw_nr; i++) {
		writel(synclen, dsi->regs[i] + DSI_HSA_WC);
		writel(backporch, dsi->regs[i] + DSI_HBP_WC);
		writel(frontporch, dsi->regs[i] + DSI_HFP_WC);
	}

	/* byte transfer to pixel */
	if (dsi->format == MTK_DSI_FMT_RGB565)
		buf_bpp = 16;
	else if ((dsi->format == MTK_DSI_FMT_RGB101010) ||
		 (dsi->format == MTK_DSI_FMT_COMPRESSION_30_15) ||
		 (dsi->format == MTK_DSI_FMT_COMPRESSION_30_10))
		buf_bpp = 30;
	else
		buf_bpp = 24;

	if (mipi_tx->phy_type == MTK_MIPI_DPHY) {
		init_byte = dsi->data_init_cycle * dsi->lanes;

		dsi->vm.hsync_len = (synclen + 10) * 8 / buf_bpp;

		if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE)
			dsi->vm.hback_porch = (backporch + 10) * 8 / buf_bpp;
		else
			dsi->vm.hback_porch = (backporch + 10) * 8 / buf_bpp -
						dsi->vm.hsync_len;

		dsi->vm.hfront_porch = (frontporch + init_byte + 12) * 8 /
					buf_bpp;
	} else {
		init_byte = dsi->data_init_cycle * 2 * dsi->lanes;

		dsi->vm.hsync_len = (synclen + 36) * 8 / buf_bpp;

		if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE)
			dsi->vm.hback_porch = (backporch + 38) * 8 / buf_bpp;
		else
			dsi->vm.hback_porch = (backporch + 38) * 8 / buf_bpp -
						dsi->vm.hsync_len;

		dsi->vm.hfront_porch = (frontporch + init_byte + 38) * 8 /
					buf_bpp;
	}

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_set_lcm_hblank);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Set dsi display mode. \n
 *     User can call this function to set display mode.
 * @param[in] dev: dsi device data struct.
 * @param[in] mode: display mode parameter.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Device struct is not assigned, return -EFAULT.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_set_display_mode(const struct device *dev, unsigned long mode)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);
	dsi->mode_flags = (mode & MIPI_DSI_MODE_VIDEO) |
			  (mode & MIPI_DSI_MODE_VIDEO_BURST) |
			  (mode & MIPI_DSI_MODE_VIDEO_SYNC_PULSE) |
			  (mode & MIPI_DSI_MODE_EOT_PACKET) |
			  (mode & MIPI_DSI_CLOCK_NON_CONTINUOUS);

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_set_display_mode);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Set dsi format. \n
 *     User can call this function to set dsi display format.
 * @param[in] dev: dsi device data struct.
 * @param[in] format: display format.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. Device struct is not assigned, return -EFAULT. \n
 *     2. The parameter, format, is not in mtk_dsi_pixel_format, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_set_format(const struct device *dev,
		       enum mtk_dsi_pixel_format format)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	if (format >= MTK_DSI_FMT_NR)
		return -EINVAL;

	dsi = dev_get_drvdata(dev);
	dsi->format = format;
	dsi->dsc_cfg.pic_format = format;

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_set_format);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Set lcm index. \n
 *     User can call this function to set lcm index.
 * @param[in] dev: dsi device data struct.
 * @param[in] index: lcm index (current support 0 ~ 3).
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. Device struct is not assigned, return -EFAULT. \n
 *     2. The parameter, index, is not in mtk_dsi_panel_index, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_set_lcm_index(const struct device *dev,
			  enum mtk_dsi_panel_index index)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	if (index >= MTK_LCM_NR)
		return -EINVAL;

	dsi = dev_get_drvdata(dev);
	dsi->panel_sel = index;
	lcm_get_params(dsi);

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_set_lcm_index);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Get lcm index. \n
 *     User can call this function to get lcm index.
 * @param[in] dev: dsi device data struct.
 * @param[out] index: lcm index.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Device struct is not assigned, return -EFAULT.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_get_lcm_index(const struct device *dev, u32 *index)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);
	*index = dsi->panel_sel;

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_get_lcm_index);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Get dsc configuration. \n
 *     This configuration should be set in panel driver and \n
 *     user call this function to get dsc configuration.
 * @param[in] dev: dsi device data struct
 * @param[out] dsc_cfg: dsc configuration struct.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Device struct is not assigned, return -EFAULT.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_get_dsc_config(const struct device *dev, struct dsc_config *dsc_cfg)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);
	*dsc_cfg = dsi->dsc_cfg;

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_get_dsc_config);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Send dcs cmd function. \n
 *     User call this function to send dcs command packet to panel. \n
 *     This function will stop dsi and switch to command mode \n
 *     to send dcs packet. If user do not want to stop display, \n
 *     please use mtk_dsi_send_vm_cmd().
 * @param[in] dev: dsi device data struct.
 * @param[in] cmd: dcs command.
 * @param[in] count: dcs parameter length.
 * @param[in] para_list: dcs parameter.
 * @return
 *     0, it means packet is send. \n
 *     -EFAULT, invalid dev address. \n
 *     -EINVAL, invalid parameter. \n
 *     -EIO, device not enable. \n
 *     error code from mtk_dsi_host_write_cmd().
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     1. Device struct is not assigned, return -EFAULT. \n
 *     2. Parameter, para_list, is null, return -EINVAL. \n
 *     3. The device not enable, return -EIO. \n
 *     4. If mtk_dsi_host_write_cmd() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_send_dcs_cmd(const struct device *dev, u32 cmd, u8 count,
			 const u8 *para_list)
{
	struct mtk_dsi *dsi;
	int ret = 0;

	if (!dev)
		return -EFAULT;

	if (!para_list && count)
		return -EINVAL;

	dsi = dev_get_drvdata(dev);

	if (!dsi->enabled)
		return -EIO;

	mtk_dsi_stop(dsi);
	mtk_dsi_wait_for_idle(dsi);
	mtk_dsi_set_cmd_mode(dsi);
	ret = mtk_dsi_host_write_cmd(dsi, cmd, count, para_list);
	mtk_dsi_set_mode(dsi);

	return ret;
}
EXPORT_SYMBOL(mtk_dsi_send_dcs_cmd);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Read dcs cmd function. \n
 *     User call this function to read dcs command packet from panel. \n
 *     This function will stop dsi and switch to command mode \n
 *     to send/read dcs packet.
 * @param[in] dev: dsi device data struct.
 * @param[in] cmd: dcs command.
 * @param[in] pkt_size: set return packet size.
 * @param[in] read_data: dcs read data return from panel.
 * @return
 *     0, packet is read. \n
 *     -EFAULT, invalid dev address. \n
 *     -EINVAL, invalid parameter. \n
 *     -EIO, device not enable. \n
 *     error code from mtk_dsi_host_read_cmd().
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     1. Device struct is not assigned, return -EFAULT. \n
 *     2. Parameter, para_list, is null, return -EINVAL. \n
 *     3. The device not enable, return -EIO. \n
 *     4. If mtk_dsi_host_read_cmd() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_read_dcs_cmd(const struct device *dev, u32 cmd, u8 pkt_size,
			 struct dsi_read_data *read_data)
{
	struct mtk_dsi *dsi;
	int ret = 0;

	if (!dev)
		return -EFAULT;

	if (!pkt_size)
		return -EINVAL;

	dsi = dev_get_drvdata(dev);

	if (!dsi->enabled)
		return -EIO;

	mtk_dsi_stop(dsi);
	mtk_dsi_wait_for_idle(dsi);
	mtk_dsi_set_cmd_mode(dsi);
	ret = mtk_dsi_host_read_cmd(dsi, cmd, pkt_size, read_data);
	mtk_dsi_set_mode(dsi);

	return ret;
}
EXPORT_SYMBOL(mtk_dsi_read_dcs_cmd);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Send vm cmd function. \n
 *     User can call this function to send dsi packet to panel \n
 *     during video mode display. The packet was sended in vblanking area.
 * @param[in] dev: dsi device data struct.
 * @param[in] data_id: cmd data id.
 * @param[in] data0: cmd data0.
 * @param[in] data1: cmd data1.
 * @param[in] para_list: cmd parameter.
 * @return
 *     0, cmd send success. \n
 *     -1, not support this cmd type. \n
 *     -2, long pkt cmd parameters error. \n
 *     -3, wait cmd send timeout. \n
 *     -EFAULT, invalid dev address. \n
 *     -EINVAL, invalid parameter. \n
 *     -EIO, device not enable.
 * @par Boundary case and Limitation
 *     Max parameter length is 64 bytes.
 * @par Error case and Error handling
 *     1. Packet type not support, return -1. \n
 *     2. Long packet parameter length over limitation, return -2. \n
 *     3. Send packet timeout, return -3. \n
 *     4. Device struct is not assigned, return -EFAULT. \n
 *     5. Parameter, para_list, is null, return -EINVAL. \n
 *     6. The device not enable, return -EIO.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_send_vm_cmd(const struct device *dev, u8 data_id, u8 data0,
			u8 data1, const u8 *para_list)
{
	struct mtk_dsi *dsi;
	u32 i, j, mask, mask1, offset, tmp, tmp1, timeout, long_pkt, ret = 0;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);

	if (!dsi->enabled)
		return -EIO;

	switch (data_id) {
	case MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM:
	case MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM:
	case MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM:
	case MIPI_DSI_DCS_SHORT_WRITE:
	case MIPI_DSI_DCS_SHORT_WRITE_PARAM:
		long_pkt = 0;
		break;

	case MIPI_DSI_GENERIC_LONG_WRITE:
	case MIPI_DSI_DCS_LONG_WRITE:
		long_pkt = 1;
		break;

	default:
		dev_err(dsi->dev, "not support this cmd type!\n");
		return -1;
	}

	if (long_pkt && (!para_list))
		return -EINVAL;

	tmp = LONG_PKT_SET(long_pkt) | CM_DATA_ID_SET(data_id) |
	      CM_DATA_0_SET(data0) | CM_DATA_1_SET(data1);

	mask = LONG_PKT | CM_DATA_ID | CM_DATA_0 | CM_DATA_1;

	for (i = 0; i < dsi->hw_nr; i++) {
		if (long_pkt == 1) {
			if (data0 >= 1 && data0 <= 64) {
				for (j = 0; j < data0; j++) {
					mask1 = 0xff << ((j & 3) * 8);
					tmp1 = para_list[j] << ((j & 3) * 8);

					if (j >= 32)
						offset = DSI_VM_CMD_DATA20 +
							 (((j - 32) >> 2) << 2);
					else if (j >= 16)
						offset = DSI_VM_CMD_DATA10 +
							 (((j - 16) >> 2) << 2);
					else
						offset = DSI_VM_CMD_DATA0 +
							 ((j >> 2) << 2);

					mtk_dsi_mask(dsi->regs[i], offset,
						     mask1, tmp1);
				}
			} else {
				dev_err(dsi->dev,
					"long pkt cmd parameters error!\n");
				return -2;
			}
		}
		mtk_dsi_mask(dsi->regs[i], DSI_START, VM_CMD_START, 0);
		mtk_dsi_mask(dsi->regs[i], DSI_INTSTA, VM_CMD_DONE_INT_FLAG, 0);

		mtk_dsi_mask(dsi->regs[i], DSI_VM_CMD_CON, mask, tmp);
		mtk_dsi_mask(dsi->regs[i], DSI_START, VM_CMD_START,
			     VM_CMD_START);
	}

	for (i = 0; i < dsi->hw_nr; i++) {
		timeout = 500000; /* total 1s ~ 2s timeout */

		while (timeout) {
			if (readl(dsi->regs[i] + DSI_INTSTA) &
			    VM_CMD_DONE_INT_FLAG) {
				break;
			}
			usleep_range(2, 4);
			timeout--;
		}

		if (timeout == 0) {
			dev_err(dsi->dev,
				"polling dsi[%d] wait vm cmd timeout!\n", i);

			mtk_dsi_mask(dsi->regs[i], DSI_COM_CON, DSI_RESET,
				     DSI_RESET);
			mtk_dsi_mask(dsi->regs[i], DSI_COM_CON, DSI_RESET, 0);
			ret = -3;
		}

		mtk_dsi_mask(dsi->regs[i], DSI_START, VM_CMD_START, 0);
		mtk_dsi_mask(dsi->regs[i], DSI_INTSTA, VM_CMD_DONE_INT_FLAG, 0);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_dsi_send_vm_cmd);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     DSI skew calibration. \n
 *     This function can send deskew pattern to panel side. \n
 *     Panel can use this deskew pattern to calibration its link.
 * @param[in] dev: dsi device data struct.
 * @return
 *     0, skew calibration done. \n
 *     -1, phy type error. \n
 *     -2, wait not busy timeout. \n
 *     -3, wait skew cal timeout. \n
 *     -EFAULT, invalid dev address. \n
 *     -EIO, device not enable. \n
 *     -ENODATA, no data available.
 * @par Boundary case and Limitation
 *     This function is D-PHY only.
 * @par Error case and Error handling
 *     1. Current PHY type is not D_PHY, return -1. \n
 *     2. DSI can not start deskew, return -2. \n
 *     3. Deskew can not stop, return -3. \n
 *     4. Device struct is not assigned, return -EFAULT. \n
 *     5. The device not enable, return -EIO. \n
 *     6. Struct phy is not available, return -ENODATA.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_deskew(const struct device *dev)
{
	struct mtk_dsi *dsi;
	struct mtk_mipi_tx *mipi_tx;
	u32 ret = 0, index = 0, i, timeout, syncon;
	u32 syncon_bak[DSI_MAX_HW_NUM], timecon2_bak[DSI_MAX_HW_NUM];

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);

	if (!dsi->enabled)
		return -EIO;

	if (!dsi->phy)
		return -ENODATA;

	mipi_tx = phy_get_drvdata(dsi->phy);

	if (mipi_tx->phy_type != MTK_MIPI_DPHY) {
		dev_err(dsi->dev,
			"error! this function is only used for D-phy.\n");
		return -1;
	}

	syncon = HS_SYNC_CODE_SET(0xff) | HS_SYNC_CODE2_SET(0xff) |
		 HS_SKEWCAL_PAT_SET(0xaa);

	for (i = 0; i < dsi->hw_nr; i++) {
		writel(0, dsi->regs[i] + DSI_START);

		syncon_bak[i] = readl(dsi->regs[i] + DSI_PHY_SYNCON);
		timecon2_bak[i] = readl(dsi->regs[i] + DSI_PHY_TIMCON2);

		timeout = 500000; /* total 1s ~ 2s timeout */

		while (timeout) {
			if (!(readl(dsi->regs[i] + DSI_INTSTA) & DSI_BUSY))
				break;

			usleep_range(2, 4);
			timeout--;
		}

		if (timeout == 0) {
			dev_err(dsi->dev,
				"polling dsi[%d] wait not busy timeout!\n", i);

			mtk_dsi_mask(dsi->regs[i], DSI_COM_CON, DSI_RESET,
				     DSI_RESET);
			mtk_dsi_mask(dsi->regs[i], DSI_COM_CON, DSI_RESET, 0);
			ret = -2;
			break;
		}

		writel(syncon, dsi->regs[i] + DSI_PHY_SYNCON);
		mtk_dsi_mask(dsi->regs[i], DSI_PHY_SYNCON, HS_DB_SYNC_EN,
			     HS_DB_SYNC_EN);
		mtk_dsi_mask(dsi->regs[i], DSI_PHY_TIMCON2, DA_HS_SYNC_MASK,
			     DA_HS_SYNC_SET(2));
		mtk_dsi_mask(dsi->regs[i], DSI_INTSTA, SKEWCAL_DONE_INT_FLAG,
			     0);
		mtk_dsi_mask(dsi->regs[i], DSI_START, SKEWCAL_START,
			     SKEWCAL_START);

		index++; /* record how many dsi do skew cal */
	}

	for (i = 0; i < index; i++) {
		timeout = 500000; /* total 1s ~ 2s timeout */

		while (timeout) {
			if (readl(dsi->regs[i] + DSI_INTSTA) &
			    SKEWCAL_DONE_INT_FLAG) {
				break;
			}
			usleep_range(2, 4);
			timeout--;
		}

		if (timeout == 0) {
			dev_err(dsi->dev,
				"polling dsi[%d] wait skew cal timeout!\n", i);

			mtk_dsi_mask(dsi->regs[i], DSI_COM_CON, DSI_RESET,
				     DSI_RESET);
			mtk_dsi_mask(dsi->regs[i], DSI_COM_CON, DSI_RESET, 0);
			ret = -3;
		}

		mtk_dsi_mask(dsi->regs[i], DSI_INTSTA, SKEWCAL_DONE_INT_FLAG,
			     0);
		writel(syncon_bak[i], dsi->regs[i] + DSI_PHY_SYNCON);
		writel(timecon2_bak[i], dsi->regs[i] + DSI_PHY_TIMCON2);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_dsi_deskew);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Enable vm frame checksum function. \n
 *     User call this function to enable/disable frame checksum function. \n
 *     If enable this function, hw will calculate the checksum of one frame.
 * @param[in] dev: dsi device data struct.
 * @param[in] index: index of dsi driver.
 * @param[in] enable: enable or disable frame checksum.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EINVAL, invalid parameter. \n
 *     -EIO, device not enable.
 * @par Boundary case and Limitation
 *     Parameter, index, range is 0 ~ 3.
 * @par Error case and Error handling
 *     1. Device struct is not assigned, return -EFAULT. \n
 *     2. The parameter, index, is not in 0 ~ 3, return -EINVAL. \n
 *     3. The device not enable, return -EIO.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_set_frame_cksm(const struct device *dev, u8 index, bool enable)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);

	if (WARN_ON(index >= dsi->hw_nr))
		return -EINVAL;

	if (!dsi->enabled)
		return -EIO;

	if (enable) {
		mtk_dsi_mask(dsi->regs[index], DSI_DEBUG_SEL, CHKSUM_REC_EN,
			     CHKSUM_REC_EN);
		mtk_dsi_mask(dsi->regs[index], DSI_DEBUG_SEL,
			     VM_FRAME_CKSM_EN, VM_FRAME_CKSM_EN);
	} else {
		mtk_dsi_mask(dsi->regs[index], DSI_DEBUG_SEL, CHKSUM_REC_EN,
			     0);
		mtk_dsi_mask(dsi->regs[index], DSI_DEBUG_SEL,
			     VM_FRAME_CKSM_EN, 0);
	}

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_set_frame_cksm);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Get DSI frame checksum. \n
 *     After enable checksum function - mtk_dsi_set_frame_cksm(), \n
 *     user can call this function to get checksum value.
 * @param[in] dev: dsi device data struct.
 * @param[in] index: index of dsi driver.
 * @param[out] checksum: checksum value.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EINVAL, invalid parameter. \n
 *     -EIO, device not enable.
 * @par Boundary case and Limitation
 *     Parameter, index, range is 0 ~ 3.
 * @par Error case and Error handling
 *     1. Device struct is not assigned, return -EFAULT. \n
 *     2. The parameter, index, is not in 0 ~ 3, return -EINVAL. \n
 *     3. The device not enable, return -EIO.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_get_frame_cksm(const struct device *dev, u8 index, u16 *checksum)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);

	if (WARN_ON(index >= dsi->hw_nr))
		return -EINVAL;

	if (!dsi->enabled)
		return -EIO;

	*checksum = (u16)(readl(dsi->regs[index] + DSI_STATE_DBG7) >> 16);

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_get_frame_cksm);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Get DSI phy checksum. \n
 *     After enable checksum function - mtk_dsi_set_frame_cksm(), \n
 *     user can call this function to get checksum value.
 * @param[in] dev: dsi device data struct.
 * @param[in] index: index of dsi driver.
 * @param[out] checksum: checksum value.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EINVAL, invalid parameter. \n
 *     -EIO, device not enable.
 * @par Boundary case and Limitation
 *     Parameter, index, range is 0 ~ 3.
 * @par Error case and Error handling
 *     1. Device struct is not assigned, return -EFAULT. \n
 *     2. The parameter, index, is not in 0 ~ 3, return -EINVAL. \n
 *     3. The device not enable, return -EIO.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_get_phy_cksm(const struct device *dev, u8 index, u16 *checksum)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);

	if (WARN_ON(index >= dsi->hw_nr))
		return -EINVAL;

	if (!dsi->enabled)
		return -EIO;

	*checksum = (u16)(readl(dsi->regs[index] + DSI_DEBUG_MBIST) >> 16);

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_get_phy_cksm);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Print current input line counter. \n
 *     User call this function to print current input line counter \n
 *     for debug use.
 * @param[in] dev: dsi device data struct.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Device struct is not assigned, return -EFAULT.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_get_line_cnt(const struct device *dev)
{
	struct mtk_dsi *dsi;
	u32 regval, i;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);

	for (i = 0; i < dsi->hw_nr; i++) {
		regval = readl(dsi->regs[i] + DSI_LINE_DEBUG3);
		pr_debug("DSI[%d] Input Line Cnt = %d (0x%08x)\n",
			 i, regval >> 16, regval >> 16);
	}

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_get_line_cnt);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Enable vm frame counter function. \n
 *     User call this function to enable/disable frame counter function. \n
 *     If enable this function, hw will calculate the counter of one frame.
 * @param[in] dev: dsi device data struct.
 * @param[in] index: index of dsi driver.
 * @param[in] enable: enable or disable frame counter.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EINVAL, invalid parameter. \n
 *     -EIO, device not enable.
 * @par Boundary case and Limitation
 *     Parameter, index, range is 0 ~ 3.
 * @par Error case and Error handling
 *     1. Device struct is not assigned, return -EFAULT. \n
 *     2. The parameter, index, is not in 0 ~ 3, return -EINVAL. \n
 *     3. The device not enable, return -EIO.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_enable_frame_counter(const struct device *dev, u8 index,
				 bool enable)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);

	if (WARN_ON(index >= dsi->hw_nr))
		return -EINVAL;

	if (!dsi->enabled)
		return -EIO;

	if (enable)
		mtk_dsi_mask(dsi->regs[index], DSI_FRAME_CNT, FRAME_CNT_EN,
			     FRAME_CNT_EN);
	else
		mtk_dsi_mask(dsi->regs[index], DSI_FRAME_CNT, FRAME_CNT_EN, 0);

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_enable_frame_counter);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Get DSI frame counter. \n
 *     After enable counter function - mtk_dsi_enable_frame_counter(), \n
 *     user can call this function to get counter value.
 * @param[in] dev: dsi device data struct.
 * @param[in] index: index of dsi driver.
 * @param[out] counter: counter value.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EINVAL, invalid parameter. \n
 *     -EIO, device not enable.
 * @par Boundary case and Limitation
 *     Parameter, index, range is 0 ~ 3.
 * @par Error case and Error handling
 *     1. Device struct is not assigned, return -EFAULT. \n
 *     2. The parameter, index, is not in 0 ~ 3, return -EINVAL. \n
 *     3. The device not enable, return -EIO.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_get_frame_counter(const struct device *dev, u8 index, u32 *counter)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);

	if (WARN_ON(index >= dsi->hw_nr))
		return -EINVAL;

	if (!dsi->enabled)
		return -EIO;

	*counter = readl(dsi->regs[index] + DSI_FRAME_CNT) & 0x7ffffff;

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_get_frame_counter);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     DSI hw mute config function. \n
 *     User call this function to enable or disable hw mute function. \n
 *     After enable this function, dsi will output single color pattern \n
 *     when some error happened, like buffer underrun. \n
 *     The color pattern is setted by mtk_dsi_mute_color().
 * @param[in] dev: dsi device data struct.
 * @param[in] enable: enable or disable hw mute.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EIO, device not enable.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. Device struct is not assigned, return -EFAULT. \n
 *     2. The device not enable, return -EIO.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_hw_mute_config(const struct device *dev, bool enable)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);

	if (!dsi->enabled)
		return -EIO;

	mtk_dsi_hw_mute(dsi, enable);

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_hw_mute_config);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     DSI sw mute config function. \n
 *     User call this function to enable or disable sw mute function. \n
 *     After enable this function, dsi will output single color pattern \n
 *     which setted by mtk_dsi_mute_color().
 * @param[in] dev: dsi device data struct.
 * @param[in] enable: enable or disable sw mute.
 * @param[in] handle: command queue handler.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EIO, device not enable.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. Device struct is not assigned, return -EFAULT. \n
 *     2. The device not enable, return -EIO.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_sw_mute_config(const struct device *dev, bool enable,
			   struct cmdq_pkt *handle)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);

	if (!dsi->enabled)
		return -EIO;

	mtk_dsi_sw_mute(dsi, enable, handle);

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_sw_mute_config);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     DSI mute color setting function. \n
 *     User call this function to setup output color pattern when mute is on.
 * @param[in] dev: dsi device data struct.
 * @param[in] color: color display during mute.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EIO, device not enable.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. Device struct is not assigned, return -EFAULT. \n
 *     2. The device not enable, return -EIO.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_mute_color(const struct device *dev, u32 color)
{
	struct mtk_dsi *dsi;
	u32 i;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);

	if (!dsi->enabled)
		return -EIO;

	for (i = 0; i < dsi->hw_nr; i++)
		writel(color, dsi->regs[i] + DSI_BIST_PATTERN);

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_mute_color);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     DSI hw reset function. \n
 *     User call this function to reset dsi hw when hw is abnormal.
 * @param[in] dev: dsi device data struct.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EIO, device not enable.
 * @par Boundary case and Limitation
 *     Do not reset dsi during write dsi register.
 * @par Error case and Error handling
 *     1. Device struct is not assigned, return -EFAULT. \n
 *     2. The device not enable, return -EIO.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_reset(const struct device *dev)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);

	if (!dsi->enabled)
		return -EIO;

	mtk_dsi_reset_engine(dsi);

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_reset);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Config pixel type & re-calculate related patameter. \n
 *     User call this function to change the pixel format and also update the \n
 *     blank timing setting.
 * @param[in] dev: dsi device data struct.
 * @param[in] format: pixel format define in enum #mtk_dsi_pixel_format.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EINVAL, invalid parameter. \n
 *     -EIO, device not enable.
 * @par Boundary case and Limitation
 *     Before call this function, please stop dsi.
 * @par Error case and Error handling
 *     1. Device struct is not assigned, return -EFAULT. \n
 *     2. The device not enable, return -EIO. \n
 *     3. The parameter, format, is not in mtk_dsi_pixel_format, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_ps_config(const struct device *dev,
		      enum mtk_dsi_pixel_format format)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	if (format >= MTK_DSI_FMT_NR)
		return -EINVAL;

	dsi = dev_get_drvdata(dev);

	if (!dsi->enabled)
		return -EIO;

	dsi->format = format;
	mtk_dsi_config_vdo_timing(dsi);

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_ps_config);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Set data rate. \n
 *     This function is used to set data rate directly.
 * @param[in] dev: dsi device data struct.
 * @param[in] rate: target data rate, unit is kHz (more than or equal to 125MHz
 *     and less than or equal to 2.5GHz).
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     Call this function before mtk_dsi_hw_init().
 * @par Error case and Error handling
 *     1. The device struct is not assigned, return -EFAULT. \n
 *     2. The data rate is over spec, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_set_datarate(const struct device *dev, u32 rate)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);

	if ((rate >= 125000) && (rate <= 2500000))
		dsi->data_rate = rate;
	else
		return -EINVAL;

	dev_dbg(dev, "current data rate = %d\n", dsi->data_rate);

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_set_datarate);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Set panel parameters. \n
 *     User can call this function to set panel parameters.
 * @param[in] dev: dsi device data struct.
 * @param[in] mode_flags: dsi mode flags.
 * @param[in] format: display format.
 * @param[in] vm: videomode struct.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. The device struct is not assigned, return -EFAULT. \n
 *     2. The vm struct is invalid, return -EINVAL. \n
 *     3. The parameter, format, is not in mtk_dsi_pixel_format, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_set_panel_params(struct device *dev, unsigned long mode_flags,
			     enum mtk_dsi_pixel_format format,
			     const struct videomode *vm)
{
	struct mtk_dsi *dsi;

	if (!dev)
		return -EFAULT;

	if (!vm)
		return -EINVAL;

	if (format >= MTK_DSI_FMT_NR)
		return -EINVAL;

	dsi = dev_get_drvdata(dev);

	dsi->vm.hactive = vm->hactive;
	dsi->vm.vactive = vm->vactive;
	dsi->vm.vsync_len = vm->vsync_len;
	dsi->vm.vback_porch = vm->vback_porch;
	dsi->vm.vfront_porch = vm->vfront_porch;
	dsi->vm.hsync_len = vm->hsync_len;
	dsi->vm.hback_porch = vm->hback_porch;
	dsi->vm.hfront_porch = vm->hfront_porch;

	dsi->mode_flags = mode_flags;
	dsi->format = format;

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_set_panel_params);

/** @ingroup IP_group_dsi_external_function
 * @par Description
 *     Enable/disable line event to gce in video mode. \n
 *     If VACTL_EVENT_EN is turned on, DSI would send an event to gce \n
 *     when getting end of the target line in video mode at vact period. \n
 *     The target line would be VACTL_EVENT_OFFSET + VACTL_EVENT_PERIOD * n \n
 *     (n is integer and n=0.1.2.3....).
 * @param[in] dev: dsi device data struct.
 * @param[in] offset: configure line event offset.
 * @param[in] period: configure line event period.
 * @param[in] enable: enable or disable line event.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EIO, device not enable. \n
 *     -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     1. Device struct is not assigned, return -EFAULT. \n
 *     2. The device not enable, return -EIO. \n
 *     3. The parameter, offset or period is invalid, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_dsi_vact_event_config(const struct device *dev, u16 offset, u16 period,
			      bool enable)
{
	struct mtk_dsi *dsi;
	u32 reg;
	int i;

	if (!dev)
		return -EFAULT;

	dsi = dev_get_drvdata(dev);

	if (!dsi->enabled)
		return -EIO;

	if (offset > 0xffff || period > 0xffff)
		return -EINVAL;

	reg = VACTL_EVENT_OFFSET_SET(offset) | VACTL_EVENT_PERIOD_SET(period);

	for (i = 0; i < dsi->hw_nr; i++) {
		writel(reg, dsi->regs[i] + DSI_VACTL_EVENT);

		if (enable)
			mtk_dsi_mask(dsi->regs[i], DSI_VACTL_EVENT,
				     VACTL_EVENT_EN, VACTL_EVENT_EN);
		else
			mtk_dsi_mask(dsi->regs[i], DSI_VACTL_EVENT,
				     VACTL_EVENT_EN, 0);
	}

	return 0;
}
EXPORT_SYMBOL(mtk_dsi_vact_event_config);

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Platform device probe.
 * @param[in] pdev: platform device.
 * @return
 *     0, driver probe success. \n
 *     Others, driver probe failed.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_dsi_probe(struct platform_device *pdev)
{
	struct mtk_dsi *dsi;
	struct device *dev = &pdev->dev;
	struct resource *regs;
	struct device_node *node;
	struct platform_device *ctrl_pdev;
	struct device_node *soc_node;
	int ret, i;
	bool splash;

	dsi = devm_kzalloc(dev, sizeof(*dsi), GFP_KERNEL);
	if (!dsi)
		return -ENOMEM;

	dsi->dev = dev;

	of_property_read_u32(dev->of_node, "hw-number", &dsi->hw_nr);
	of_property_read_u32(dev->of_node, "fpga-mode", &dsi->fpga_mode);
	of_property_read_u32(dev->of_node, "panel-sel", &dsi->panel_sel);

	if ((dsi->hw_nr == 0) || (dsi->hw_nr > 4)) {
		dev_err(dev, "Dsi hw number is wrong: %d\n", dsi->hw_nr);
		return -EINVAL;
	}

	if (dsi->panel_sel >= MTK_LCM_NR) {
		dev_err(dev, "Panel selection is wrong: %d\n", dsi->panel_sel);
		return -EINVAL;
	}

	for (i = 0; i < dsi->hw_nr; i++) {
		int idx = i;

		if ((i == 1) && (dsi->hw_nr == 2))
			idx = 2;

		regs = platform_get_resource(pdev, IORESOURCE_MEM, idx);
		dsi->regs[i] = devm_ioremap_resource(dev, regs);
		if (IS_ERR(dsi->regs[i])) {
			ret = PTR_ERR(dsi->regs[i]);
			dev_err(dev, "Failed to ioremap dsi%d registers: %d\n",
				i, ret);
			return PTR_ERR(dsi->regs[i]);
		}
		dsi->gce_subsys_offset[i] = regs->start & 0xffff;
	}

	ret = of_property_read_u32(dev->of_node, "dsi-lanes", &dsi->lanes);
	if (ret) {
		dev_err(dev, "parsing dsi-lanes error: %d\n", ret);
		return -EINVAL;
	}
	if ((dsi->lanes == 0) || (dsi->lanes > 4)) {
		dev_err(dev, "lane number is wrong: %d\n", dsi->lanes);
		return -EINVAL;
	}

	/* mmsys cfg */
	node = of_parse_phandle(dev->of_node, "mediatek,mmsyscfg", 0);
	if (!node) {
		dev_err(dev, "Failed to get mmsyscfg node\n");
		of_node_put(node);
		return -EINVAL;
	}

	ctrl_pdev = of_find_device_by_node(node);
	if (!ctrl_pdev) {
		dev_err(dev, "Device dose not enable %s\n",
			node->full_name);
		of_node_put(node);
		return -ENODEV;
	}
	if (!dev_get_drvdata(&ctrl_pdev->dev)) {
		dev_err(dev, "Waiting for mmsyscfg device %s\n",
			node->full_name);
		of_node_put(node);
		return -EPROBE_DEFER;
	}
	dsi->mmsys_cfg_dev = &ctrl_pdev->dev;
	if (!dsi->mmsys_cfg_dev) {
		dev_err(dev, "Failed to find mmsyscfg platform device\n");
		of_node_put(node);
		return -EINVAL;
	}
	of_node_put(node);

	/* GCE Init */
	ret = of_property_read_u32(dev->of_node, "gce-subsys",
				   &dsi->gce_subsys);
	if (ret) {
		dev_err(dev, "parsing gce-subsys error: %d\n", ret);
		return -EINVAL;
	}

	/* PHY Init */
	dsi->phy = devm_phy_get(dev, "dphy");
	if (IS_ERR(dsi->phy)) {
		ret = PTR_ERR(dsi->phy);
		dev_err(dev, "not found mipi-dphy: %d\n", ret);
		return ret;
	}

	/* Clock Init */
#ifdef CONFIG_COMMON_CLK_MT3612
	for (i = 0; i < dsi->hw_nr; i++) {
		char clk[10];
		int idx = i;

		if ((i == 1) && (dsi->hw_nr == 2))
			idx = 2;

		snprintf(clk, sizeof(clk), "mm-clk%d", idx);
		dsi->mm_clk[i] = devm_clk_get(dev, clk);
		if (IS_ERR(dsi->mm_clk[i])) {
			dev_err(dev, "cannot get %s clock\n", clk);
			return PTR_ERR(dsi->mm_clk[i]);
		}
		snprintf(clk, sizeof(clk), "dsi-clk%d", idx);
		dsi->dsi_clk[i] = devm_clk_get(dev, clk);
		if (IS_ERR(dsi->dsi_clk[i])) {
			dev_err(dev, "cannot get %s clock\n", clk);
			return PTR_ERR(dsi->dsi_clk[i]);
		}
	}
	pm_runtime_enable(dev);
#endif

	dsi->irq_data = 0;

	lcm_get_params(dsi);

	platform_set_drvdata(pdev, dsi);

	soc_node = of_find_node_by_path("/soc");
	if (!soc_node) {
		dev_err(dev, "%s cannot find soc node\n", __func__);
		return -ENODEV;
	}

	splash = of_property_read_bool(soc_node, "splash_screen");
	if (splash) {
#ifdef CONFIG_COMMON_CLK_MT3612
		ret = pm_runtime_get_sync(dev);
		if (ret < 0) {
			dev_err(dev, "Failed to enable power domain:%d\n", ret);
			return ret;
		}
		for (i = 0; i < dsi->hw_nr; i++) {
			ret = clk_prepare_enable(dsi->mm_clk[i]);
			if (ret)
				break;
			ret = clk_prepare_enable(dsi->dsi_clk[i]);
			if (ret)
				break;
		}
		if (ret) {
			for (i = 0; i < dsi->hw_nr; i++) {
				clk_disable_unprepare(dsi->mm_clk[i]);
				clk_disable_unprepare(dsi->dsi_clk[i]);
			}
			pm_runtime_put(dev);
			return ret;
		}
#endif
		dsi->enabled = 1;
		dsi->poweron = 1;
		dsi->phy->power_count = 1;
	}

	splash = of_property_read_bool(soc_node, "splash_on");
	if (splash) {
		dsi->enabled = 1;
		dsi->poweron = 1;
		dsi->phy->power_count = 1;
	}

	ret = mtk_drm_dsi_debugfs_init(dsi);
	if (ret) {
		dev_err(dev, "Failed to initialize dsi debugfs\n");
		return ret;
	}

	return 0;
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     Platform device remove.
 * @param[in] pdev: platform device
 * @return
 *     0, driver remove success \n
 *     Others, driver remove failed
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_dsi_remove(struct platform_device *pdev)
{
	struct mtk_dsi *dsi = platform_get_drvdata(pdev);

	mtk_dsi_stop(dsi);
	mtk_dsi_disable(dsi);
	mtk_drm_dsi_debugfs_exit(dsi);

#ifdef CONFIG_COMMON_CLK_MT3612
	pm_runtime_disable(&pdev->dev);
#endif

	return 0;
}

/**
 * @brief a device id structure for dsi driver
 */
static const struct of_device_id mtk_dsi_of_match[] = {
	{ .compatible = "mediatek,mt3612-dsi" },
	{ },
};
MODULE_DEVICE_TABLE(of, mtk_dsi_of_match);

/**
 * @brief a platform driver structure for dsi driver
 */
static struct platform_driver mtk_dsi_driver = {
	.probe = mtk_dsi_probe,
	.remove = mtk_dsi_remove,
	.driver = {
		.name = "mtk-dsi",
		.owner = THIS_MODULE,
		.of_match_table = mtk_dsi_of_match,
	},
};

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     DSI driver init function.
 * @return
 *     0, driver init success \n
 *     Others, driver init failed
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int __init mtk_dsi_init(void)
{
	int ret = 0;

	pr_debug("mtk dsi drv init!\n");

	ret = platform_driver_register(&mtk_dsi_driver);
	if (ret) {
		pr_err(" platform_driver_register failed!\n");
		return ret;
	}
	pr_debug("mtk dsi drv init ok!\n");
	return ret;
}

/** @ingroup IP_group_dsi_internal_function_dsi
 * @par Description
 *     DSI driver exit function.
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void __exit mtk_dsi_exit(void)
{
	pr_debug("mtk dsi drv exit!\n");
	platform_driver_unregister(&mtk_dsi_driver);
}

module_init(mtk_dsi_init);
module_exit(mtk_dsi_exit);

MODULE_LICENSE("GPL v2");
