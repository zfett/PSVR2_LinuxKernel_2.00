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
 * @file mtk_mipi_tx.c
 * MTK MIPI TX Linux Driver.\n
 * This driver is used to configure MTK mipi tx hardware module.\n
 * It includes pll setting, lane setting...
 */

/**
 * @defgroup IP_group_mipi_tx MIPI_TX
 *   This is mipi tx linux driver. \n
 *   It can configure MTK mipi tx hardware module to generate clock for
 *   clock/data lane and can set lane sequence. \n
 *   Before enable dsi, please initial mipi tx.
 *
 *   @{
 *       @defgroup IP_group_mipi_tx_external EXTERNAL
 *         The external API document for mipi tx.
 *
 *         @{
 *             @defgroup IP_group_mipi_tx_external_function 1.function
 *               None. No external function, use Linux PHY framework.
 *             @defgroup IP_group_mipi_tx_external_struct 2.structure
 *               External structure used for mipi tx.
 *             @defgroup IP_group_mipi_tx_external_typedef 3.typedef
 *               None. No external typedef used in mipi tx driver.
 *             @defgroup IP_group_mipi_tx_external_enum 4.enumeration
 *               None. No external enumeration used in mipi tx driver.
 *             @defgroup IP_group_mipi_tx_external_def 5.define
 *               This is mipi tx external define.
 *         @}
 *
 *       @defgroup IP_group_mipi_tx_internal INTERNAL
 *         The internal API document for mipi tx.
 *
 *         @{
 *             @defgroup IP_group_mipi_tx_internal_function 1.function
 *               This is mipi tx internal function and module init.
 *             @defgroup IP_group_mipi_tx_internal_struct 2.structure
 *               None. No internal structure used in mipi tx driver.
 *             @defgroup IP_group_mipi_tx_internal_typedef 3.typedef
 *               None. No internal typedef used in mipi tx driver.
 *             @defgroup IP_group_mipi_tx_internal_enum 4.enumeration
 *               None. No internal enumeration used in mipi tx driver.
 *             @defgroup IP_group_mipi_tx_internal_def 5.define
 *               This is mipi tx internal define.
 *         @}
 *   @}
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/phy/phy.h>
#include <linux/nvmem-consumer.h>
#include <linux/slab.h>

#include <soc/mediatek/mtk_mipi_tx.h>
#include "mtk_mipi_tx_reg.h"

/** @ingroup IP_group_mipi_tx_internal_def
 * @{
 */
#define IMPEDANCE_0_DEFAULT		0x10101010
#define IMPEDANCE_1_DEFAULT		0x10101010

#define RG_DSI0_CKMODE_MASK		GENMASK(20, 16)
#define RG_DSI0_CKMODE(x)		((x) << 16)
#define D3N_RT_CODE_MASK		RG_DSI0_D3N_RT_CODE
#define D3N_RT_CODE_SET(x)		((x) << 8)
#define D3P_RT_CODE_MASK		RG_DSI0_D3P_RT_CODE
#define D3P_RT_CODE_SET(x)		((x) << 0)

#define RG_DSI0_HSTX_LDO_REF_SEL_MASK	RG_DSI0_HSTX_LDO_REF_SEL
#define RG_DSI0_HSTX_LDO_REF_SEL_SET(x)	((x) << 6)

/* DSI_PLL_CON1 */
#define RG_DSI0_PLL_POSDIV_MASK		RG_DSI0_PLL_POSDIV
#define RG_DSI0_PLL_POSDIV_SET(x)	((x) << 8)

/* DSI_PLL_CON2 */
#define RG_DSI0_PLL_SDM_SSC_PRD_MASK	RG_DSI0_PLL_SDM_SSC_PRD
#define RG_DSI0_PLL_SDM_SSC_PRD_SET(x)	((x) << 16)

/* DSI_PLL_CON3 */
#define PLL_SDM_SSC_DELTA_MASK		RG_DSI0_PLL_SDM_SSC_DELTA
#define PLL_SDM_SSC_DELTA_SET(x)	((x) << 16)
#define PLL_SDM_SSC_DELTA1_MASK		RG_DSI0_PLL_SDM_SSC_DELTA1
#define PLL_SDM_SSC_DELTA1_SET(x)	((x) << 0)

#define PHY_SEL0_DPHY_DEFAULT		0x65432100
#define PHY_SEL0_DPHY_SWAP		0x25476980
#define PHY_SEL0_DPHY_SWAP2		0x74523010
#define PHY_SEL0_DPHY_SWAP3		0x25476100
#define PHY_SEL0_CPHY_DEFAULT		0x65432101
#define PHY_SEL0_CPHY_SWAP		0x05438761

#define PHY_SEL1_DPHY_DEFAULT		0x24232987
#define PHY_SEL1_DPHY_SWAP		0x24276103
#define PHY_SEL1_DPHY_SWAP2		0x24223896
#define PHY_SEL1_DPHY_SWAP3		0x24276983
#define PHY_SEL1_CPHY_DEFAULT		0x24210987
#define PHY_SEL1_CPHY_SWAP		0x06876921

#define PHY_SEL2_DPHY_DEFAULT		0x76543210
#define PHY_SEL2_DPHY_SWAP		0x72543618
#define PHY_SEL2_DPHY_SWAP2		0x76543210
#define PHY_SEL2_DPHY_SWAP3		0x72543610
#define PHY_SEL2_CPHY_DEFAULT		0x68543102
#define PHY_SEL2_CPHY_SWAP		0x02543768

#define PHY_SEL3_DPHY_DEFAULT		0x00000008
#define PHY_SEL3_DPHY_SWAP		0x00000000
/* #define PHY_SEL3_DPHY_SWAP2		0x00001f08 */
#define PHY_SEL3_DPHY_SWAP2		0x00000008
#define PHY_SEL3_DPHY_SWAP3		0x00000008
#define PHY_SEL3_CPHY_DEFAULT		0x00000007
#define PHY_SEL3_CPHY_SWAP		0x00000001
/**
 * @}
 */

/** @ingroup IP_group_mipi_tx_internal_def
 * @{
 */
/** Default reference clock for PLL (MHz) */
#define REFCK_DEFAULT		26

/** Number of parameter for test chip setting, only used when fpga_mode=1 */
#define DTB_PARAM_NO		26
/**
 * @}
 */

/** @ingroup IP_group_mipi_tx_internal_function
 * @par Description
 *     DTB register write function. \n
 *     This function is only for fpga, \n
 *     and related i2c hw is also only available on fpga.
 * @param[in] reg: fpga used i2c controller base address
 * @param[in] slave_addr: DTB i2c slave address
 * @param[in] write_addr: DTB register address
 * @param[in] write_data: write data value
 * @return
 *     0, DTB register configure success \n
 *     -1, DTB register configure failed
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mipitx_writedtb(void __iomem *reg, u32 slave_addr, u32 write_addr,
			   u32 write_data)
{
	u32 read_data;
	int ret;

	writel(0x2, reg + 0x14);
	writel(0x1, reg + 0x18);
	writel(((u32)slave_addr << 0x1), reg + 0x04);
	writel(write_addr, reg + 0x0);
	writel(write_data, reg + 0x0);
	writel(0x1, reg + 0x24);
	while ((readl(reg + 0xc) & 0x1) != 0x1)
		;
	writel(0xff, reg + 0xc);

	writel(0x1, reg + 0x14);
	writel(0x1, reg + 0x18);
	writel(((u32)slave_addr << 0x1), reg + 0x04);
	writel(write_addr, reg + 0x0);
	writel(0x1, reg + 0x24);
	while ((readl(reg + 0xc) & 0x1) != 0x1)
		;
	writel(0xff, reg + 0xc);

	writel(0x1, reg + 0x14);
	writel(0x1, reg + 0x18);
	writel(((u32)slave_addr << 0x1) + 1, reg + 0x04);
	writel(0x1, reg + 0x24);
	while ((readl(reg + 0xc) & 0x1) != 0x1)
		;
	writel(0xff, reg + 0xc);

	read_data = readl(reg);

	if (read_data == write_data) {
		ret = 0;
	} else {
		pr_alert("MIPI write fail\n");
		pr_alert("MIPI write data = 0x%x, read data = 0x%x\n",
			 write_data, read_data);
		ret = -1;
	}

	return ret;
}

/** @ingroup IP_group_mipi_tx_internal_function
 * @par Description
 *     DTB initialize function. \n
 *     This function is only for fpga.
 * @param[in] phy: mipi tx phy driver data struct
 * @return
 *     0, DTB configure success \n
 *     -1, DTB configure failed
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mipitx_dtb_power_on(struct phy *phy)
{
	struct mtk_mipi_tx *mipi_tx = phy_get_drvdata(phy);
	/*
	 *----------------------------------------------------------------------
	 * PHY Timing Control
	 *----------------------------------------------------------------------
	 *PLL freq = 26x(fbkdiv)x(prediv)/(posdiv)
	 * prediv = 0:/1, 1:/2, 2,3:/4
	 * posdiv = 0:/1, 1:/2, 2:/4, 3:/8, 4:/16
	 * txdiv0,txdiv1 is removed from BDTB
	 */
	const unsigned char fbkdiv = 0x04;
	const unsigned char prediv = 0x00;
	const unsigned char posdiv = 0x03;

	const unsigned char para[DTB_PARAM_NO][3] = {
		{0x18, 0x00, 0x10},  /* 0: DSI GPIO mux select */
		{0x20, 0x42, 0x01},  /* 1: ABB reference clock LPF */
		{0x20, 0x43, 0x01},  /* 2: Reference clock from ARMPLL */
		{0x20, 0x05, 0x01},  /* 3: Reference clock from ARMPLL */
		{0x20, 0x22, 0x01},  /* 4: Reference clock from ARMPLL */
		{0x30, 0x44, 0x83},  /* 5: MIPI TX Config, DSI_BG_CON */
		{0x30, 0x40, 0x82},  /* 6: MIPI TX Config, DSI_TOP_CON */
		{0x30, 0x00, 0x03},  /* 7: MIPI TX Config, DSI0_CON */
		{0x30, 0x68, 0x03},  /* 8: MIPI TX Config, DSI_PLL_PWR */
				     /* (pwr_on=1) */
		{0x30, 0x68, 0x01},  /* 9: MIPI TX Config, DSI_PLL_PWR */
				     /* (iso_en=0) */
		{0x30, 0x50, ((posdiv & 0x1) << 0x7) | (prediv << 0x1)},
				     /* 10: MIPI TX Config, DSI_PLL0_CON0 */
		{0x30, 0x51, (posdiv >> 0x1) & 0x3},
				     /* 11: MIPI TX Config, DSI_PLL0_CON0 */
				     /* - [2:1] prediv */
				     /* - [4:3] txdiv0 (not used) */
				     /* - [6:5] txdiv1 (not used) */
				     /* - [9:7] posdiv */
		{0x30, 0x54, 0x01},  /* 12: MIPI TX Config, DSI_PLL0_CON1 */
				     /* - [0] fra_en */
				     /* - [2] ssc_en */
		{0x30, 0x58, 0x00},  /* 13: MIPI TX Config, DSI_PLL0_CON2 */
		{0x30, 0x59, 0x00},  /* 14: MIPI TX Config */
				     /* - [23:0]  pcw (fractional) */
		{0x30, 0x5a, 0x00},  /* 15: MIPI TX Config */
				     /* - [23:0]  pcw (fractional) */
		{0x30, 0x5b, (fbkdiv << 2)},  /* 16: MIPI TX Config */
				     /* - [30:24] pcw (integer) */
		{0x30, 0x04, 0x11},  /* 17: MIPI TX Config, DSI_CLOCK_LANE */
				     /* [4]:CKLANE_EN=1 */
		{0x30, 0x08, 0x01},  /* 18: MIPI TX Config, DSI_DATA_LANE_0 */
		{0x30, 0x0c, 0x01},  /* 19: MIPI TX Config, DSI_DATA_LANE_1 */
		{0x30, 0x10, 0x01},  /* 20: MIPI TX Config, DSI_DATA_LANE_2 */
		{0x30, 0x14, 0x01},  /* 21: MIPI TX Config, DSI_DATA_LANE_3 */
		{0x30, 0x64, 0x20},  /* 22: MIPI TX Config, DSI_PLL_TOP */
				     /* - [9:8] mppll_preserve (s2q_div) */
		{0x30, 0x50, ((posdiv & 0x1) << 0x7) | (prediv << 0x1) | 0x1},
				     /* 23: MPPLL starts, DSI_PLL0_CON0 */
				     /* - [0] pll_en */
		{0x30, 0x28, 0x00},  /* 24: MIPI TX source control */
		{0x18, 0x27, 0x70}   /* 25: GPIO12 DSI_BCLK driving */
	};
	int i, ret = 0;

	for (i = 0; i < DTB_PARAM_NO; i++) {
		ret = mipitx_writedtb(mipi_tx->regs[0], para[i][0], para[i][1],
				      para[i][2]);
		if (ret)
			return -1;
	}

	return 0;
}

/** @ingroup IP_group_mipi_tx_internal_function
 * @par Description
 *     MIPI TX reigster mask write common function.
 * @param[in] reg: mipi tx register base address
 * @param[in] offset: mipi tx register offset
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
static void mtk_mipitx_mask(void __iomem *reg, u32 offset, u32 mask, u32 data)
{
	u32 temp = readl(reg + offset);

	writel((temp & ~mask) | (data & mask), reg + offset);
}

/** @ingroup IP_group_mipi_tx_internal_function
 * @par Description
 *     Config MIPI TX pad impedance. \n
 *     Read pad impedance data from e-fuse and set into related register.
 * @param[in] phy: mipi tx phy driver data struct
 * @return none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     If efuse is blank, use default setting.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mipitx_impedance_config(struct phy *phy)
{
	struct mtk_mipi_tx *mipi_tx = phy_get_drvdata(phy);
	u32 i, j;
	u32 *buf;
	u32 tmp_efuse;
	u8 tmp_imp[10];
	u32 imp0, imp1;
	u8 imp2_d3n, imp2_d3p;
	struct	nvmem_cell *cell;
	size_t len;
	u32 reg_offset = 0;
	u32 bit_offset = 0;

	cell = nvmem_cell_get(&phy->dev, "phy-calibration");
	if (IS_ERR_OR_NULL(cell))
		goto err_set_default;

	buf = (u32 *)nvmem_cell_read(cell, &len);

	/* cannot read or efuse is blank */
	if (IS_ERR(buf) || (buf[reg_offset] == 0)) {
		nvmem_cell_put(cell);
		kfree(buf);
		goto err_set_default;
	}

	for (i = 0; i < mipi_tx->hw_nr; i++) {
		for (j = 0; j < 10; j++) {
			if (bit_offset == 0)
				tmp_efuse = buf[reg_offset];
			tmp_imp[j] = (u8)(tmp_efuse & 0x0000001f);
			tmp_efuse = tmp_efuse >> 5;
			bit_offset += 5;
			if (bit_offset > 25) {
				bit_offset = 0;
				reg_offset++;
			}
		}
		/* tmp_imp[10] = ck_p, ck_n, d0_p, d0_n, d1_p,...d3_p, d3_n */
		imp0 = ((u32)tmp_imp[3] << 24) | ((u32)tmp_imp[2] << 16) |
		       ((u32)tmp_imp[7] << 8) | ((u32)tmp_imp[6] << 0);
		imp1 = ((u32)tmp_imp[5] << 24) | ((u32)tmp_imp[4] << 16) |
		       ((u32)tmp_imp[1] << 8) | ((u32)tmp_imp[0] << 0);
		imp2_d3n = tmp_imp[9];
		imp2_d3p = tmp_imp[8];

		writel(imp0, mipi_tx->regs[i] + DSI_IMPENDANCE_0);
		writel(imp1, mipi_tx->regs[i] + DSI_IMPENDANCE_1);
		mtk_mipitx_mask(mipi_tx->regs[i], DSI_IMPENDANCE_2,
				D3N_RT_CODE_MASK, D3N_RT_CODE_SET(imp2_d3n));
		mtk_mipitx_mask(mipi_tx->regs[i], DSI_IMPENDANCE_2,
				D3P_RT_CODE_MASK, D3P_RT_CODE_SET(imp2_d3p));
	}
	nvmem_cell_put(cell);
	kfree(buf);
	return;

err_set_default:
	for (i = 0; i < mipi_tx->hw_nr; i++) {
		/* use default setting */
		writel(IMPEDANCE_0_DEFAULT,
		       mipi_tx->regs[i] + DSI_IMPENDANCE_0);
		writel(IMPEDANCE_1_DEFAULT,
		       mipi_tx->regs[i] + DSI_IMPENDANCE_1);
		mtk_mipitx_mask(mipi_tx->regs[i], DSI_IMPENDANCE_2,
				D3N_RT_CODE_MASK, D3N_RT_CODE_SET(0x10));
		mtk_mipitx_mask(mipi_tx->regs[i], DSI_IMPENDANCE_2,
				D3P_RT_CODE_MASK, D3P_RT_CODE_SET(0x10));
	}
}

/** @ingroup IP_group_mipi_tx_internal_function
 * @par Description
 *     Config MIPI D-PHY TX lane setting. \n
 *     Support two lane sequence and can be chose by lane_swap in dts.
 * @param[in] phy: mipi tx phy driver data struct
 * @return none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mipitx_dphy_lane_config(struct phy *phy)
{
	struct mtk_mipi_tx *mipi_tx = phy_get_drvdata(phy);
	u32 i;

	for (i = 0; i < mipi_tx->hw_nr; i++) {
		if (mipi_tx->lane_swap == 1) {
			mtk_mipitx_mask(mipi_tx->regs[i], DSI_IMPENDANCE_2,
					RG_DSI0_CKMODE_MASK, RG_DSI0_CKMODE(4));
			writel(PHY_SEL0_DPHY_SWAP,
			       mipi_tx->regs[i] + DSI_PHY_SEL0);
			writel(PHY_SEL1_DPHY_SWAP,
			       mipi_tx->regs[i] + DSI_PHY_SEL1);
			writel(PHY_SEL2_DPHY_SWAP,
			       mipi_tx->regs[i] + DSI_PHY_SEL2);
			writel(PHY_SEL3_DPHY_SWAP,
			       mipi_tx->regs[i] + DSI_PHY_SEL3);
		} else if (mipi_tx->lane_swap == 2  && (i&0x01) == 0) {
			mtk_mipitx_mask(mipi_tx->regs[i], DSI_IMPENDANCE_2,
					RG_DSI0_CKMODE_MASK, RG_DSI0_CKMODE(4));
			mtk_mipitx_mask(mipi_tx->regs[i], DSI_IMPENDANCE_2,
					GENMASK(28, 24), 0x1F<<24);
			writel(PHY_SEL0_DPHY_SWAP2,
			       mipi_tx->regs[i] + DSI_PHY_SEL0);
			writel(PHY_SEL1_DPHY_SWAP2,
			       mipi_tx->regs[i] + DSI_PHY_SEL1);
			writel(PHY_SEL2_DPHY_SWAP2,
			       mipi_tx->regs[i] + DSI_PHY_SEL2);
			writel(PHY_SEL3_DPHY_SWAP2,
			       mipi_tx->regs[i] + DSI_PHY_SEL3);
		} else if (mipi_tx->lane_swap == 3) {
			mtk_mipitx_mask(mipi_tx->regs[i], DSI_IMPENDANCE_2,
					RG_DSI0_CKMODE_MASK, RG_DSI0_CKMODE(4));
			writel(PHY_SEL0_DPHY_SWAP3,
			       mipi_tx->regs[i] + DSI_PHY_SEL0);
			writel(PHY_SEL1_DPHY_SWAP3,
			       mipi_tx->regs[i] + DSI_PHY_SEL1);
			writel(PHY_SEL2_DPHY_SWAP3,
			       mipi_tx->regs[i] + DSI_PHY_SEL2);
			writel(PHY_SEL3_DPHY_SWAP3,
			       mipi_tx->regs[i] + DSI_PHY_SEL3);
		} else {
			mtk_mipitx_mask(mipi_tx->regs[i], DSI_IMPENDANCE_2,
					RG_DSI0_CKMODE_MASK, RG_DSI0_CKMODE(4));
			writel(PHY_SEL0_DPHY_DEFAULT,
			       mipi_tx->regs[i] + DSI_PHY_SEL0);
			writel(PHY_SEL1_DPHY_DEFAULT,
			       mipi_tx->regs[i] + DSI_PHY_SEL1);
			writel(PHY_SEL2_DPHY_DEFAULT,
			       mipi_tx->regs[i] + DSI_PHY_SEL2);
			writel(PHY_SEL3_DPHY_DEFAULT,
			       mipi_tx->regs[i] + DSI_PHY_SEL3);
		}
		mtk_mipitx_mask(mipi_tx->regs[i], DSI_LANE_CON,
				(RG_DSI0_BG_CORE_EN | RG_DSI0_BG_LPF_EN |
				 RG_DSI0_CPHY_EN), RG_DSI0_BG_CORE_EN);
	}
	udelay(5); /* minimum is 1us */
	for (i = 0; i < mipi_tx->hw_nr; i++) {
		mtk_mipitx_mask(mipi_tx->regs[i], DSI_LANE_CON,
				RG_DSI0_BG_LPF_EN, RG_DSI0_BG_LPF_EN);
		mtk_mipitx_mask(mipi_tx->regs[i], DSI_VOLTAGE_SEL,
				RG_DSI0_HSTX_LDO_REF_SEL_MASK,
				RG_DSI0_HSTX_LDO_REF_SEL_SET(0x8));
	}
	udelay(1); /* minimum is 30ns */
}

/** @ingroup IP_group_mipi_tx_internal_function
 * @par Description
 *     Config MIPI C-PHY TX lane setting. \n
 *     Support two lane sequence and can be chose by lane_swap in dts.
 * @param[in] phy: mipi tx phy driver data struct
 * @return none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mipitx_cphy_lane_config(struct phy *phy)
{
	struct mtk_mipi_tx *mipi_tx = phy_get_drvdata(phy);
	u32 i;

	for (i = 0; i < mipi_tx->hw_nr; i++) {
		if (mipi_tx->lane_swap == 1) {
			mtk_mipitx_mask(mipi_tx->regs[i], DSI_IMPENDANCE_2,
					RG_DSI0_CKMODE_MASK, RG_DSI0_CKMODE(0));
			writel(PHY_SEL0_CPHY_SWAP,
			       mipi_tx->regs[i] + DSI_PHY_SEL0);
			writel(PHY_SEL1_CPHY_SWAP,
			       mipi_tx->regs[i] + DSI_PHY_SEL1);
			writel(PHY_SEL2_CPHY_SWAP,
			       mipi_tx->regs[i] + DSI_PHY_SEL2);
			writel(PHY_SEL3_CPHY_SWAP,
			       mipi_tx->regs[i] + DSI_PHY_SEL3);
		} else {
			mtk_mipitx_mask(mipi_tx->regs[i], DSI_IMPENDANCE_2,
					RG_DSI0_CKMODE_MASK, RG_DSI0_CKMODE(0));
			writel(PHY_SEL0_CPHY_DEFAULT,
			       mipi_tx->regs[i] + DSI_PHY_SEL0);
			writel(PHY_SEL1_CPHY_DEFAULT,
			       mipi_tx->regs[i] + DSI_PHY_SEL1);
			writel(PHY_SEL2_CPHY_DEFAULT,
			       mipi_tx->regs[i] + DSI_PHY_SEL2);
			writel(PHY_SEL3_CPHY_DEFAULT,
			       mipi_tx->regs[i] + DSI_PHY_SEL3);
		}
		mtk_mipitx_mask(mipi_tx->regs[i], DSI_LANE_CON,
				(RG_DSI0_BG_CORE_EN | RG_DSI0_BG_LPF_EN |
				 RG_DSI0_CPHY_EN),
				(RG_DSI0_BG_CORE_EN | RG_DSI0_CPHY_EN));
	}
	udelay(5); /* minimum is 1us */
	for (i = 0; i < mipi_tx->hw_nr; i++) {
		mtk_mipitx_mask(mipi_tx->regs[i], DSI_LANE_CON,
				RG_DSI0_BG_LPF_EN, RG_DSI0_BG_LPF_EN);
		mtk_mipitx_mask(mipi_tx->regs[i], DSI_VOLTAGE_SEL,
				RG_DSI0_HSTX_LDO_REF_SEL_MASK,
				RG_DSI0_HSTX_LDO_REF_SEL_SET(0xd));
	}
	udelay(1); /* minimum is 30ns */
}

/** @ingroup IP_group_mipi_tx_internal_function
 * @par Description
 *     Get reference clock, default is 26MHz.
 * @param[in] phy: mipi tx phy driver data struct
 * @return reference clock frequency, unit is MHz
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static u32 mipitx_get_refck(struct phy *phy)
{
	return REFCK_DEFAULT;
}

/** @ingroup IP_group_mipi_tx_internal_function
 * @par Description
 *     Initialize MIPI TX PLL. \n
 *     PLL PCW config \n
 *     PCW bit 24~30 = floor(pcw) \n
 *     PCW bit 16~23 = (pcw - floor(pcw))*256 \n
 *     PCW bit 8~15 = (pcw*256 - floor(pcw)*256)*256 \n
 *     PCW bit 8~15 = (pcw*256*256 - floor(pcw)*256*256)*256 \n
 *     And if enable SSC, current SSC spec in driver is \n
 *     Modulation_Rate = 30k \n
 *     SSC Deviation = 5000ppm
 * @param[in] phy: mipi tx phy driver data struct
 * @param[in] ssc_mode_in: SSC mode select (1, 2, 3)
 * @param[in] ssc_dev_in: SSC deviation (1, 2, 3, 4, 5)
 * @return none
 * @par Boundary case and Limitation
 *     Maximum frequency is 2.5GHz.
 * @par Error case and Error handling
 *     If clock is over 2.5GHz, it will set to 2.5GHz.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mipitx_pll_init(struct phy *phy, u32 ssc_mode_in, u32 ssc_dev_in)
{
	struct mtk_mipi_tx *mipi_tx = phy_get_drvdata(phy);
	u32 ref_ck;
	u32 pll_ck;
	u32 pcw_ratio = 0;
	u32 pcw = 0;
	u32 posdiv = 0;
	u32 tmp;
	u32 ssc_prd, ssc_delta, div_delta;
	u32 i;

	if (mipi_tx->phy_type == MTK_MIPI_DPHY)
		pll_ck = mipi_tx->data_rate;
	else
		pll_ck = (mipi_tx->data_rate * 7 + 15) / 16;

	dev_info(&phy->dev, "pll_ck=%d KHz, mipitx's data rate=%d.%03d MHz\n",
		 pll_ck, mipi_tx->data_rate/1000, mipi_tx->data_rate%1000);

	for (i = 0; i < mipi_tx->hw_nr; i++) {
		/* PLL power on */
		mtk_mipitx_mask(mipi_tx->regs[i], DSI_PLL_PWR,
				(DA_DSI0_PLL_SDM_ISO_EN |
				 DA_DSI0_PLL_SDM_PWR_ON),
				(DA_DSI0_PLL_SDM_ISO_EN |
				 DA_DSI0_PLL_SDM_PWR_ON));
		udelay(5); /* minimum is 1us */
		mtk_mipitx_mask(mipi_tx->regs[i], DSI_PLL_PWR,
				DA_DSI0_PLL_SDM_ISO_EN, 0);
		udelay(1); /* minimum is 30ns */

		if (pll_ck > 2500000) {
			dev_err(&phy->dev,
				"mipitx Data Rate exceed limitation(%d)\n",
				pll_ck);
			pll_ck = 2500000;
			pcw_ratio = 1;
			posdiv    = 0;
		} else if (pll_ck >= 2000000) { /* 2G ~ 2.5G */
			pcw_ratio = 1;
			posdiv    = 0;
		} else if (pll_ck >= 1000000) { /* 1G ~ 2G */
			pcw_ratio = 2;
			posdiv    = 1;
		} else if (pll_ck >= 500000) { /* 500M ~ 1G */
			pcw_ratio = 4;
			posdiv    = 2;
		} else if (pll_ck > 250000) { /* 250M ~ 500M */
			pcw_ratio = 8;
			posdiv    = 3;
		} else if (pll_ck >= 125000) { /* 125M ~ 250M */
			pcw_ratio = 16;
			posdiv    = 4;
		} else {
			dev_err(&phy->dev, "dataRate is too low(%d)\n", pll_ck);
			pll_ck = 125000;
			pcw_ratio = 16;
			posdiv    = 4;
		}

		/* PLL PCW config */
		/*
		 * PCW bit 24~30 = floor(pcw)
		 * PCW bit 16~23 = (pcw - floor(pcw))*256
		 * PCW bit 8~15 = (pcw*256 - floor(pcw)*256)*256
		 * PCW bit 8~15 = (pcw*256*256 - floor(pcw)*256*256)*256
		 */
		ref_ck = mipitx_get_refck(phy) * 1000;
		tmp = pll_ck * pcw_ratio;
		pcw =
		    (((tmp / ref_ck) & 0xFF) << 24) |
		    (((256 * (tmp % ref_ck) / ref_ck) & 0xFF) << 16) |
		    (((256 * (256 * (tmp % ref_ck) % ref_ck) / ref_ck) & 0xFF)
		    << 8) | ((256 * (256 * (256 * (tmp % ref_ck) % ref_ck) %
		    ref_ck) / ref_ck) & 0xFF);

		writel(pcw, mipi_tx->regs[i] + DSI_PLL_CON0);
		mtk_mipitx_mask(mipi_tx->regs[i], DSI_PLL_CON1,
				RG_DSI0_PLL_POSDIV_MASK,
				RG_DSI0_PLL_POSDIV_SET(posdiv));

		udelay(5); /* minimum is 1us */

		/* PLL enable */
		mtk_mipitx_mask(mipi_tx->regs[i], DSI_PLL_CON1, RG_DSI0_PLL_EN,
				RG_DSI0_PLL_EN);

		usleep_range(25, 30); /* minimum is 20us */

		/* SSC enable */
		if (ssc_mode_in > 1) {
			/*
			 * Modified accroding to the SSC modes design,
			 * satisfy API Spec and calculation precision.
			 * ------------------------
			 * SSC Spec
			 * ssc_mode = 1, 2, 3;
			 * 1: ssc off
			 * 2: ssc on: 0 ~ ssc_dev
			 * 3: ssc on: -(1/2)*ssc_dev ~ (1/2)*ssc_dev
			 * Modulation_Rate = 30k
			 * ssc_dev = 1, 2, 3, 4, 5; 1: 1000ppm, etc.
			 */
			if (ssc_dev_in > 5) {
				dev_err(&phy->dev,
					"ssc_dev is out of range(%d)\n",
					ssc_dev_in);
				ssc_dev_in = 5;
				dev_err(&phy->dev,
					"ssc_dev is maximum set(%d)\n",
					ssc_dev_in);
			} else if (ssc_dev_in < 1) {
				dev_err(&phy->dev,
					"ssc_dev is out of range(%d)\n",
					ssc_dev_in);
				ssc_dev_in = 1;
				dev_err(&phy->dev,
					"ssc_dev is minimum set(%d)\n",
					ssc_dev_in);
			}

			ssc_prd = ref_ck / 30 / 2;
			ssc_delta = (((pll_ck * pcw_ratio * ssc_dev_in) /
				1000) * 1024) / ssc_prd * 256 / ref_ck;

			if (ssc_mode_in == 2) {
				div_delta = ssc_delta;
			} else if (ssc_mode_in == 3) {
				ssc_delta = ssc_delta & 0xFFFFFFFE;
				div_delta = ssc_delta / 2;
			} else {
				dev_err(&phy->dev,
					"ssc_mode is out of range(%d)\n",
					ssc_mode_in);
				ssc_mode_in = 2;
				dev_err(&phy->dev,
					"ssc_mode is mode 2 set(%d)\n",
					ssc_mode_in);
				div_delta = ssc_delta;
			}

			mtk_mipitx_mask(mipi_tx->regs[i], DSI_PLL_CON2,
					RG_DSI0_PLL_SDM_SSC_PRD_MASK,
					RG_DSI0_PLL_SDM_SSC_PRD_SET(ssc_prd));
			mtk_mipitx_mask(mipi_tx->regs[i], DSI_PLL_CON3,
					PLL_SDM_SSC_DELTA_MASK,
					PLL_SDM_SSC_DELTA_SET(ssc_delta));
			mtk_mipitx_mask(mipi_tx->regs[i], DSI_PLL_CON3,
					PLL_SDM_SSC_DELTA1_MASK,
					PLL_SDM_SSC_DELTA1_SET(div_delta));

			mtk_mipitx_mask(mipi_tx->regs[i], DSI_PLL_CON2,
					RG_DSI0_PLL_SDM_SSC_EN,
					RG_DSI0_PLL_SDM_SSC_EN);
		}
	}
}

/** @ingroup IP_group_mipi_tx_internal_function
 * @par Description
 *     Power on mipi tx. \n
 *     Step: \n
 *     1. Setup tx output impedance. \n
 *     2. Configure lane sequence then power on tx output. \n
 *     3. Enable and configure PLL to output target frequence.
 * @param[in] phy: mipi tx phy driver data struct
 * @return
 *     0, power on success \n
 *     Others, powre on failed
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mipitx_power_on(struct phy *phy)
{
	struct mtk_mipi_tx *mipi_tx = phy_get_drvdata(phy);

	mipitx_impedance_config(phy);

	if (mipi_tx->phy_type == MTK_MIPI_DPHY)
		mipitx_dphy_lane_config(phy);
	else
		mipitx_cphy_lane_config(phy);

	mipitx_pll_init(phy, mipi_tx->ssc_mode, mipi_tx->ssc_dev);

	return 0;
}

/** @ingroup IP_group_mipi_tx_internal_function
 * @par Description
 *     Power on mipi tx (top layer).
 * @param[in] phy: mipi tx phy driver data struct
 * @return
 *     0, power on success \n
 *     Others, powre on failed \n
 *     error code from mipitx_dtb_power_on() and mipitx_power_on().
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_mipi_tx_power_on(struct phy *phy)
{
	struct mtk_mipi_tx *mipi_tx = phy_get_drvdata(phy);

	if (mipi_tx->fpga_mode == 1)
		return mipitx_dtb_power_on(phy);
	else
		return mipitx_power_on(phy);
}

/** @ingroup IP_group_mipi_tx_internal_function
 * @par Description
 *     Power off mipi tx. \n
 *     Step: \n
 *     1. Disable SSC. \n
 *     2. Disable PLL. \n
 *     3. Power off tx output.
 * @param[in] phy: mipi tx phy driver data struct
 * @return
 *     0, power off success \n
 *     Others, powre off failed
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_mipi_tx_power_off(struct phy *phy)
{
	struct mtk_mipi_tx *mipi_tx = phy_get_drvdata(phy);
	u32 i;

	if (mipi_tx->fpga_mode == 0) {
		for (i = 0; i < mipi_tx->hw_nr; i++) {
			/* SSC disable */
			mtk_mipitx_mask(mipi_tx->regs[i], DSI_PLL_CON2,
					RG_DSI0_PLL_SDM_SSC_EN, 0);
			/* PLL disable */
			mtk_mipitx_mask(mipi_tx->regs[i], DSI_PLL_CON1,
					RG_DSI0_PLL_EN, 0);
			/* SDM_ISO_EN=1 */
			mtk_mipitx_mask(mipi_tx->regs[i], DSI_PLL_PWR,
					DA_DSI0_PLL_SDM_ISO_EN, 1);
			/* SDM_RWR_ON=0 */
			mtk_mipitx_mask(mipi_tx->regs[i], DSI_PLL_PWR,
					DA_DSI0_PLL_SDM_PWR_ON, 0);
			/* BG_LPF_EN=0 */
			mtk_mipitx_mask(mipi_tx->regs[i], DSI_LANE_CON,
					RG_DSI0_BG_LPF_EN, 0);
			/* BG_CORE_EN=0 */
			mtk_mipitx_mask(mipi_tx->regs[i], DSI_LANE_CON,
					RG_DSI0_BG_CORE_EN, 0);
		}
	}

	return 0;
}

/**
 * @brief a phy operation structure for mipi tx driver
 */
static struct phy_ops mtk_mipi_tx_ops = {
	.power_on = mtk_mipi_tx_power_on,
	.power_off = mtk_mipi_tx_power_off,
	.owner = THIS_MODULE,
};

/**
 * @brief a device id structure for mipi tx driver
 */
static const struct of_device_id mtk_mipi_tx_match[] = {
	{ .compatible = "mediatek,mt3612-mipi-tx" },
	{},
};
MODULE_DEVICE_TABLE(of, mtk_mipi_tx_match);

/** @ingroup IP_group_mipi_tx_internal_function
 * @par Description
 *     Platform device probe.
 * @param[in] pdev: platform device
 * @return
 *     0, driver probe success \n
 *     Others, driver probe failed
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_mipi_tx_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_mipi_tx *mipi_tx;
	struct resource *mem;
	struct phy *phy;
	struct phy_provider *phy_provider;
	int ret, i;

	mipi_tx = devm_kzalloc(dev, sizeof(*mipi_tx), GFP_KERNEL);
	if (!mipi_tx)
		return -ENOMEM;

	of_property_read_u32(dev->of_node, "hw-number", &mipi_tx->hw_nr);
	of_property_read_u32(dev->of_node, "fpga-mode", &mipi_tx->fpga_mode);
	of_property_read_u32(dev->of_node, "phy-type", &mipi_tx->phy_type);
	of_property_read_u32(dev->of_node, "lane-swap", &mipi_tx->lane_swap);
	of_property_read_u32(dev->of_node, "ssc-mode", &mipi_tx->ssc_mode);
	of_property_read_u32(dev->of_node, "ssc-deviation", &mipi_tx->ssc_dev);

	for (i = 0; i < mipi_tx->hw_nr; i++) {
		int idx = i;

		if ((i == 1) && (mipi_tx->hw_nr == 2))
			idx = 2;

		mem = platform_get_resource(pdev, IORESOURCE_MEM, idx);
		mipi_tx->regs[i] = devm_ioremap_resource(dev, mem);
		if (IS_ERR(mipi_tx->regs[i])) {
			ret = PTR_ERR(mipi_tx->regs[i]);
			dev_err(dev,
				"Failed to get mipitx%d memory resource: %d\n",
				i, ret);
			return ret;
		}
	}

	phy = devm_phy_create(dev, NULL, &mtk_mipi_tx_ops);
	if (IS_ERR(phy)) {
		dev_err(dev, "Failed to create MIPI D-PHY\n");
		return PTR_ERR(phy);
	}
	phy_set_drvdata(phy, mipi_tx);

	phy_provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);

	return PTR_ERR_OR_ZERO(phy_provider);
}

/** @ingroup IP_group_mipi_tx_internal_function
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
static int mtk_mipi_tx_remove(struct platform_device *pdev)
{
	return 0;
}

/**
 * @brief a platform driver structure for mipi tx driver
 */
static struct platform_driver mtk_mipi_tx_driver = {
	.probe = mtk_mipi_tx_probe,
	.remove = mtk_mipi_tx_remove,
	.driver = {
		.name = "mtk-mipi-tx",
		.of_match_table = mtk_mipi_tx_match,
	},
};

/** @ingroup IP_group_mipi_tx_internal_function
 * @par Description
 *     MIPI TX driver init function.
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int __init mtk_mipi_tx_init(void)
{
	int ret = 0;

	pr_info("mtk mipi_tx drv init!\n");

	ret = platform_driver_register(&mtk_mipi_tx_driver);
	if (ret) {
		pr_alert(" platform_driver_register failed!\n");
		return ret;
	}
	pr_info("mtk mipi_tx drv init ok!\n");
	return ret;
}

/** @ingroup IP_group_mipi_tx_internal_function
 * @par Description
 *     MIPI TX driver exit function.
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void __exit mtk_mipi_tx_exit(void)
{
	pr_info("mtk mipi_tx drv exit!\n");
	platform_driver_unregister(&mtk_mipi_tx_driver);
}

module_init(mtk_mipi_tx_init);
module_exit(mtk_mipi_tx_exit);

MODULE_LICENSE("GPL v2");
