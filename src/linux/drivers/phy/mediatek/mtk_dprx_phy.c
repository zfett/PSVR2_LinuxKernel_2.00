/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: Chunfeng Yun <chunfeng.yun@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/**
 * @file mtk_dprx_phy.c
 * MediaTek DP RX PHY Driver
 */

/**
 * @defgroup IP_group_dprxphy DPRX_PHY
 *   The dirver use for MTK DP RX PHY.
 *
 *   @{
 *       @defgroup IP_group_dprxphy_external EXTERNAL
 *         The external API document for DP RX PHY. \n
 *
 *         @{
 *           @defgroup IP_group_dprxphy_external_function 1.function
 *               none. No extra function in DP RX PHY driver.
 *           @defgroup IP_group_dprxphy_external_struct 2.Structure
 *               none. No extra Structure in DP RX PHY driver.
 *           @defgroup IP_group_dprxphy_external_typedef 3.Typedef
 *               none. No extra Typedef in DP RX PHY driver.
 *           @defgroup IP_group_dprxphy_external_enum 4.Enumeration
 *               none. No extra Enumeration in DP RX PHY driver.
 *           @defgroup IP_group_dprxphy_external_def 5.Define
 *               none. No extra Define in DP RX PHY driver.
 *         @}
 *
 *       @defgroup IP_group_dprxphy_internal INTERNAL
 *         The internal API document for DP RX PHY. \n
 *
 *         @{
 *           @defgroup IP_group_dprxphy_internal_function 1.function
 *               Internal function for MTK DP RX PHY initial.
 *           @defgroup IP_group_dprxphy_internal_struct 2.Structure
 *               Internal structure used for DP RX PHY driver.
 *           @defgroup IP_group_dprxphy_internal_typedef 3.Typedef
 *               none. No extra Typedef in DP RX PHY driver.
 *           @defgroup IP_group_dprxphy_internal_enum 4.Enumeration
 *               none. No extra enumeration used for DP RX PHY driver.
 *           @defgroup IP_group_dprxphy_internal_def 5.Define
 *               Internal define used for DP RX PHY driver.
 *         @}
 *    @}
 */

#include <dt-bindings/phy/phy.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/nvmem-consumer.h>
#include <linux/slab.h>

/** @ingroup IP_group_dprxphy_internal_def
 * @brief use for mtk dprxphy register define.
 * @{
 */

/* DPRX phy banks Base:0x11E60000 */
#define DPRX_DIG_GLB				0x000

/* DPRX phy banks Base:0x11E66000 */
#define DPRX_DIG_LN0_RX2			0x000
#define DPRX_DIG_LN1_RX2			0x100
#define DPRX_DIG_LN2_RX2			0x200
#define DPRX_DIG_LN3_RX2			0x300

/* DPRX phy banks Base:0x11E69000 */
#define DPRX_SIFSLV_ANA_GLB			0x000

/* DPRX phy banks Base:0x11E6A000 */
#define DPRX_SIFSLV_ANA_LN0_RX			0x000
#define DPRX_SIFSLV_ANA_LN1_RX			0x100
#define DPRX_SIFSLV_ANA_LN2_RX			0x200
#define DPRX_SIFSLV_ANA_LN3_RX			0x300

/* XTP CKM banks Base:0x11E50000 */
#define XTP_CKM_DP_SIFSLV_CORE			0x000
#define XTP_CKM_FORCE_REGISTER_20		0x020
#define RG_CKM_AVD10_ON				GENMASK(1, 1)
#define RG_CKM_AVD10_ON_VAL(x)			((0x01 & (x)) << 1)
#define RG_CKM_FRC_AVD10_ON			GENMASK(2, 2)
#define RG_CKM_FRC_AVD10_ON_VAL(x)		((0x01 & (x)) << 2)

#define XTP_CKM_FORCE_REGISTER_3C		0x03C
#define RG_CKM_FRC_EN				GENMASK(5, 5)
#define RG_CKM_FRC_EN_VAL(x)			((0x01 & (x)) << 5)

/* CR define for module name: DPRX_DIG_GLB */
#define DPRX_DIG_GLB_10				0x010
#define RG_XTP_RESET				GENMASK(0, 0)
#define RG_XTP_RESET_VAL(x)			((0x01 & (x)) << 0)

/* CR define for module name: DPRX_DIG_LN_RX2 */
#define DPRX_DIG_LN_RX2_14			0x014
#define RG_XTP_LN_RX_CDR_GEN3_PDKUNIT		GENMASK(13, 12)
#define RG_XTP_LN_RX_CDR_GEN3_PDKUNIT_VAL(x)	((0x03 & (x)) << 12)
#define RG_XTP_LN_RX_CDR_GEN4_PDKUNIT		GENMASK(15, 14)
#define RG_XTP_LN_RX_CDR_GEN4_PDKUNIT_VAL(x)	((0x03 & (x)) << 14)

#define DPRX_DIG_LN_RX2_1C			0x01C
#define RG_XTP_LN_RX_CDR_GEN1_PDKKPC		GENMASK(4, 0)
#define RG_XTP_LN_RX_CDR_GEN1_PDKKPC_VAL(x)	((0x1f & (x)) << 0)
#define RG_XTP_LN_RX_CDR_GEN3_PDKKPC		GENMASK(20, 16)
#define RG_XTP_LN_RX_CDR_GEN3_PDKKPC_VAL(x)	((0x1f & (x)) << 16)
#define RG_XTP_LN_RX_CDR_GEN4_PDKKPC		GENMASK(28, 24)
#define RG_XTP_LN_RX_CDR_GEN4_PDKKPC_VAL(x)	((0x1f & (x)) << 24)

#define DPRX_DIG_LN_RX2_54			0x054
#define RG_DIG_LN_RX2_CTLE_HBR3			GENMASK(28, 24)
#define RG_DIG_LN_RX2_CTLE_HBR3_VAL(x)		((0x1f & (x)) << 24)
#define RG_DIG_LN_RX2_CTLE_HBR2			GENMASK(20, 16)
#define RG_DIG_LN_RX2_CTLE_HBR2_VAL(x)		((0x1f & (x)) << 16)
#define RG_DIG_LN_RX2_CTLE_HBR			GENMASK(12, 8)
#define RG_DIG_LN_RX2_CTLE_HBR_VAL(x)		((0x1f & (x)) << 8)
#define RG_DIG_LN_RX2_CTLE_RBR			GENMASK(4, 0)
#define RG_DIG_LN_RX2_CTLE_RBR_VAL(x)		((0x1f & (x)) << 0)

/* CR define for module name: DPRX_SIFSLV_ANA_GLB */
#define DPRX_ANA_GLB_00				0x000
#define RG_XTP_GLB_BIAS_INTR_CTRL		GENMASK(29, 24)
#define RG_XTP_GLB_BIAS_INTR_CTRL_VAL(x)	((0x3F & (x)) << 24)

#define DPRX_ANA_GLB_30				0x030
#define RG_XTP_GLB_TPLL1_TCL_EN			GENMASK(1, 1)
#define RG_XTP_GLB_TPLL1_TCL_EN_VAL(x)		((0x01 & (x)) << 1)

/* CR define for module name: DPRX_SIFSLV_ANA_LN0_RX */
#define DPRX_ANA_LN_RX_3C			0x03C
#define RG_XTP_LN_RX_IMPSEL			GENMASK(4, 0)
#define RG_XTP_LN_RX_IMPSEL_VAL(x)		((0x1f & (x)) << 0)

/* CR define for module name: XTP CKM */
#define XTP_CKM_REG_00				0x000
#define RG_CKM_BIAS_INTR_CTRL			GENMASK(21, 16)
#define RG_CKM_BIAS_INTR_CTRL_VAL(x)		((0x3F & (x)) << 16)

#define XTP_CKM_REG_14				0x014
#define RG_CKM_SPLL_TCL_EN			GENMASK(4, 4)
#define RG_CKM_SPLL_TCL_EN_VAL(x)		((0x01 & (x)) << 4)

#define XTP_CKM_REG_18				0x018
#define RG_CKM_DPAUX_TX_IMPSEL			GENMASK(7, 3)
#define RG_CKM_DPAUX_TX_IMPSEL_VAL(x)		((0x1F & (x)) << 3)

/* CR define for module name: XTP COMMON2 */
#ifdef CONFIG_MT3612_DPRX_EFUSE
#define EFUSE_M_ANALOG13			0x1B4
#define CKMDP_VLD				GENMASK(31, 31)
#define CKMDP_EFUSE_DPAUX_TX_IMPSEL		GENMASK(26, 22)
#define CKMDP_EFUSE_BIAS_INTR_CTRL		GENMASK(21, 16)

#define EFUSE_M_ANALOG14			0x1B8
#define DPRX_VLD				GENMASK(31, 31)
#define DPRX_GLB_BIAS_INTR_CTRL			GENMASK(5, 0)
#define DPRX_LN0_RX_IMPSEL			GENMASK(10, 6)
#define DPRX_LN1_RX_IMPSEL			GENMASK(15, 11)
#define DPRX_LN2_RX_IMPSEL			GENMASK(20, 16)
#define DPRX_LN3_RX_IMPSEL			GENMASK(25, 21)
#endif

#define PHY_HW_NUMBER 5

/**
 * @}
 */

/** @ingroup IP_group_dprxphy_internal_struct
 * @brief MTK DP RX PHY register bank structure
 */
struct dprxphy_banks {
	/** DPRX phy banks Base:0x11E60000 */
	void __iomem *dig_glb;
	/** DPRX phy banks Base:0x11E66000 */
	void __iomem *dig_ln0_rx2;
	/** DPRX phy banks Base:0x11E66100 */
	void __iomem *dig_ln1_rx2;
	/** DPRX phy banks Base:0x11E66200 */
	void __iomem *dig_ln2_rx2;
	/** DPRX phy banks Base:0x11E66300 */
	void __iomem *dig_ln3_rx2;
	/** DPRX phy banks Base:0x11E69000 */
	void __iomem *ana_glb;
	/** DPRX phy banks Base:0x11E6A000 */
	void __iomem *ana_ln0;
	/** DPRX phy banks Base:0x11E6A100 */
	void __iomem *ana_ln1;
	/** DPRX phy banks Base:0x11E6A200 */
	void __iomem *ana_ln2;
	/** DPRX phy banks Base:0x11E6A300 */
	void __iomem *ana_ln3;
	/* XTP CKM banks Base:0x11E50000 */
	void __iomem *xtp_ckm;
};

/** @ingroup IP_group_dprxphy_internal_struct
* @brief MTK one phy data information.
*/
struct mtk_phy_instance {
	/** DPRX phy struct of MTK phy */
	struct phy *phy;
	/** DPRX phy register base address */
	void __iomem *regs[PHY_HW_NUMBER];
	/** DPRX phy banks */
	struct dprxphy_banks dprx_banks;
	/** clk struct of MTK phy */
	struct clk *ref_clk;
	/** phy number */
	u32 index;
	/** phy type */
	u8 type;
};

/** @ingroup IP_group_dprxphy_internal_struct
* @brief MTK DPRX phy data information.
*/
struct mtk_dprxphy {
	/** device struct of MTK DP RX PHY */
	struct device *dev;
	/** mtk_phy_instance struct */
	struct mtk_phy_instance **phys;
	/** total dprxphy numbers */
	int nphys;
	/** total dprxphy register banks' number */
	u32 phy_hw_nr;
	/** store dprx phy calibration e-fuse data */
	u32 efuse_data[2];
};

struct platform_device *phy_pdev;

/** @ingroup IP_group_dprxphy_internal_function
 * @par Description
 *     DPRX PHY initialization.
 * @param[in]
 *     dprxphy: structure of dprxphy for getting information.
 * @param[in]
 *     instance: structure of instance dprxphy for getting information.
 * @return
 *     none
 * @par Boundary case and Limitation
 *     If efuse_data is zero. return nothing to do.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void dprx_phy_instance_init(struct mtk_dprxphy *dprxphy,
				   struct mtk_phy_instance *instance)
{
	struct dprxphy_banks *dprx_banks = &instance->dprx_banks;
	void __iomem *rg_xtp_ckm = dprx_banks->xtp_ckm;
	void __iomem *rg_dig_glb = dprx_banks->dig_glb;
	void __iomem *rg_glb = dprx_banks->ana_glb;
	void __iomem *rg_ln0 = dprx_banks->ana_ln0;
	void __iomem *rg_ln1 = dprx_banks->ana_ln1;
	void __iomem *rg_ln2 = dprx_banks->ana_ln2;
	void __iomem *rg_ln3 = dprx_banks->ana_ln3;
	void __iomem *rg_dig_ln0_rx2 = dprx_banks->dig_ln0_rx2;
	void __iomem *rg_dig_ln1_rx2 = dprx_banks->dig_ln1_rx2;
	void __iomem *rg_dig_ln2_rx2 = dprx_banks->dig_ln2_rx2;
	void __iomem *rg_dig_ln3_rx2 = dprx_banks->dig_ln3_rx2;
	u32 tmp, tmp1, mask, val;
	u32 efuse_cal_val[2];

	pr_debug("dprx_phy_instance_init\n");

	tmp1 = ioread32(rg_glb + DPRX_ANA_GLB_30);
	tmp1 &= ~(RG_XTP_GLB_TPLL1_TCL_EN);
	iowrite32(tmp1, rg_glb + DPRX_ANA_GLB_30);

	tmp1 = ioread32(rg_xtp_ckm + XTP_CKM_REG_14);
	tmp1 &= ~(RG_CKM_SPLL_TCL_EN);
	iowrite32(tmp1, rg_xtp_ckm + XTP_CKM_REG_14);

	tmp1 = ioread32(rg_dig_ln0_rx2 + DPRX_DIG_LN_RX2_54);
	tmp1 &= ~(RG_DIG_LN_RX2_CTLE_HBR2);
	tmp1 &= ~(RG_DIG_LN_RX2_CTLE_HBR);
	tmp1 &= ~(RG_DIG_LN_RX2_CTLE_RBR);
	tmp1 |= RG_DIG_LN_RX2_CTLE_HBR2_VAL(0x0a);
	tmp1 |= RG_DIG_LN_RX2_CTLE_HBR_VAL(0x08);
	tmp1 |= RG_DIG_LN_RX2_CTLE_RBR_VAL(0x08);
	iowrite32(tmp1, rg_dig_ln0_rx2 + DPRX_DIG_LN_RX2_54);
	tmp1 = ioread32(rg_dig_ln1_rx2 + DPRX_DIG_LN_RX2_54);
	tmp1 &= ~(RG_DIG_LN_RX2_CTLE_HBR2);
	tmp1 &= ~(RG_DIG_LN_RX2_CTLE_HBR);
	tmp1 &= ~(RG_DIG_LN_RX2_CTLE_RBR);
	tmp1 |= RG_DIG_LN_RX2_CTLE_HBR2_VAL(0x0a);
	tmp1 |= RG_DIG_LN_RX2_CTLE_HBR_VAL(0x08);
	tmp1 |= RG_DIG_LN_RX2_CTLE_RBR_VAL(0x08);
	iowrite32(tmp1, rg_dig_ln1_rx2 + DPRX_DIG_LN_RX2_54);
	tmp1 = ioread32(rg_dig_ln2_rx2 + DPRX_DIG_LN_RX2_54);
	tmp1 &= ~(RG_DIG_LN_RX2_CTLE_HBR2);
	tmp1 &= ~(RG_DIG_LN_RX2_CTLE_HBR);
	tmp1 &= ~(RG_DIG_LN_RX2_CTLE_RBR);
	tmp1 |= RG_DIG_LN_RX2_CTLE_HBR2_VAL(0x0a);
	tmp1 |= RG_DIG_LN_RX2_CTLE_HBR_VAL(0x08);
	tmp1 |= RG_DIG_LN_RX2_CTLE_RBR_VAL(0x08);
	iowrite32(tmp1, rg_dig_ln2_rx2 + DPRX_DIG_LN_RX2_54);
	tmp1 = ioread32(rg_dig_ln3_rx2 + DPRX_DIG_LN_RX2_54);
	tmp1 &= ~(RG_DIG_LN_RX2_CTLE_HBR2);
	tmp1 &= ~(RG_DIG_LN_RX2_CTLE_HBR);
	tmp1 &= ~(RG_DIG_LN_RX2_CTLE_RBR);
	tmp1 |= RG_DIG_LN_RX2_CTLE_HBR2_VAL(0x0a);
	tmp1 |= RG_DIG_LN_RX2_CTLE_HBR_VAL(0x08);
	tmp1 |= RG_DIG_LN_RX2_CTLE_RBR_VAL(0x08);
	iowrite32(tmp1, rg_dig_ln3_rx2 + DPRX_DIG_LN_RX2_54);

	mask = RG_XTP_LN_RX_CDR_GEN1_PDKKPC |
	       RG_XTP_LN_RX_CDR_GEN3_PDKKPC |
	       RG_XTP_LN_RX_CDR_GEN4_PDKKPC;
	val = RG_XTP_LN_RX_CDR_GEN1_PDKKPC_VAL(0x0D) |
	      RG_XTP_LN_RX_CDR_GEN3_PDKKPC_VAL(0x06) |
	      RG_XTP_LN_RX_CDR_GEN4_PDKKPC_VAL(0x05);
	tmp1 = ioread32(rg_dig_ln0_rx2 + DPRX_DIG_LN_RX2_1C);
	tmp1 &= ~mask;
	tmp1 |= val;
	iowrite32(tmp1, rg_dig_ln0_rx2 + DPRX_DIG_LN_RX2_1C);
	tmp1 = ioread32(rg_dig_ln1_rx2 + DPRX_DIG_LN_RX2_1C);
	tmp1 &= ~mask;
	tmp1 |= val;
	iowrite32(tmp1, rg_dig_ln1_rx2 + DPRX_DIG_LN_RX2_1C);
	tmp1 = ioread32(rg_dig_ln2_rx2 + DPRX_DIG_LN_RX2_1C);
	tmp1 &= ~mask;
	tmp1 |= val;
	iowrite32(tmp1, rg_dig_ln2_rx2 + DPRX_DIG_LN_RX2_1C);
	tmp1 = ioread32(rg_dig_ln3_rx2 + DPRX_DIG_LN_RX2_1C);
	tmp1 &= ~mask;
	tmp1 |= val;
	iowrite32(tmp1, rg_dig_ln3_rx2 + DPRX_DIG_LN_RX2_1C);

	mask = RG_XTP_LN_RX_CDR_GEN3_PDKUNIT |
	       RG_XTP_LN_RX_CDR_GEN4_PDKUNIT;
	val = RG_XTP_LN_RX_CDR_GEN3_PDKUNIT_VAL(0x02) |
	      RG_XTP_LN_RX_CDR_GEN4_PDKUNIT_VAL(0x01);
	tmp1 = ioread32(rg_dig_ln0_rx2 + DPRX_DIG_LN_RX2_14);
	tmp1 &= ~mask;
	tmp1 |= val;
	iowrite32(tmp1, rg_dig_ln0_rx2 + DPRX_DIG_LN_RX2_14);
	tmp1 = ioread32(rg_dig_ln1_rx2 + DPRX_DIG_LN_RX2_14);
	tmp1 &= ~mask;
	tmp1 |= val;
	iowrite32(tmp1, rg_dig_ln1_rx2 + DPRX_DIG_LN_RX2_14);
	tmp1 = ioread32(rg_dig_ln2_rx2 + DPRX_DIG_LN_RX2_14);
	tmp1 &= ~mask;
	tmp1 |= val;
	iowrite32(tmp1, rg_dig_ln2_rx2 + DPRX_DIG_LN_RX2_14);
	tmp1 = ioread32(rg_dig_ln3_rx2 + DPRX_DIG_LN_RX2_14);
	tmp1 &= ~mask;
	tmp1 |= val;
	iowrite32(tmp1, rg_dig_ln3_rx2 + DPRX_DIG_LN_RX2_14);

	tmp1 = ioread32(rg_dig_glb + DPRX_DIG_GLB_10);
	tmp1 &= ~(RG_XTP_RESET);
	tmp1 |= RG_XTP_RESET_VAL(1);
	iowrite32(tmp1, rg_dig_glb + DPRX_DIG_GLB_10);
	tmp1 = ioread32(rg_dig_glb + DPRX_DIG_GLB_10);
	tmp1 &= ~(RG_XTP_RESET);
	tmp1 |= RG_XTP_RESET_VAL(0);
	iowrite32(tmp1, rg_dig_glb + DPRX_DIG_GLB_10);

	efuse_cal_val[0] = dprxphy->efuse_data[0];
	efuse_cal_val[1] = dprxphy->efuse_data[1];

#ifdef CONFIG_MT3612_DPRX_EFUSE
	pr_debug("efuse_reg_base + 0x1B4 = 0x%x\n", efuse_cal_val[0]);
	pr_debug("efuse_reg_base + 0x1B8 = 0x%x\n", efuse_cal_val[1]);

	if ((efuse_cal_val[0] & CKMDP_VLD) == 0x80000000) {
		tmp = (efuse_cal_val[0] & CKMDP_EFUSE_DPAUX_TX_IMPSEL) >> 22;
		tmp1 = ioread32(rg_xtp_ckm + XTP_CKM_REG_18);
		tmp1 &= ~(RG_CKM_DPAUX_TX_IMPSEL);
		tmp1 |= RG_CKM_DPAUX_TX_IMPSEL_VAL(tmp);
		iowrite32(tmp1, rg_xtp_ckm + XTP_CKM_REG_18);

		tmp = (efuse_cal_val[0] & CKMDP_EFUSE_BIAS_INTR_CTRL) >> 16;
		tmp1 = ioread32(rg_xtp_ckm + XTP_CKM_REG_00);
		tmp1 &= ~(RG_CKM_BIAS_INTR_CTRL);
		tmp1 |= RG_CKM_BIAS_INTR_CTRL_VAL(tmp);
		iowrite32(tmp1, rg_xtp_ckm + XTP_CKM_REG_00);
	}

	if ((efuse_cal_val[1] & DPRX_VLD) == 0x80000000) {
		tmp = efuse_cal_val[1] & DPRX_GLB_BIAS_INTR_CTRL;
		tmp1 = ioread32(rg_glb + DPRX_ANA_GLB_00);
		tmp1 &= ~(RG_XTP_GLB_BIAS_INTR_CTRL);
		tmp1 |= RG_XTP_GLB_BIAS_INTR_CTRL_VAL(tmp);
		iowrite32(tmp1, rg_glb + DPRX_ANA_GLB_00);

		tmp = (efuse_cal_val[1] & DPRX_LN0_RX_IMPSEL) >> 6;
		tmp1 = ioread32(rg_ln0 + DPRX_ANA_LN_RX_3C);
		tmp1 &= ~(RG_XTP_LN_RX_IMPSEL);
		tmp1 |= RG_XTP_LN_RX_IMPSEL_VAL(tmp);
		iowrite32(tmp1, rg_ln0 + DPRX_ANA_LN_RX_3C);

		tmp = (efuse_cal_val[1] & DPRX_LN1_RX_IMPSEL) >> 11;
		tmp1 = ioread32(rg_ln1 + DPRX_ANA_LN_RX_3C);
		tmp1 &= ~(RG_XTP_LN_RX_IMPSEL);
		tmp1 |= RG_XTP_LN_RX_IMPSEL_VAL(tmp);
		iowrite32(tmp1, rg_ln1 + DPRX_ANA_LN_RX_3C);

		tmp = (efuse_cal_val[1] & DPRX_LN2_RX_IMPSEL) >> 16;
		tmp1 = ioread32(rg_ln2 + DPRX_ANA_LN_RX_3C);
		tmp1 &= ~(RG_XTP_LN_RX_IMPSEL);
		tmp1 |= RG_XTP_LN_RX_IMPSEL_VAL(tmp);
		iowrite32(tmp1, rg_ln2 + DPRX_ANA_LN_RX_3C);

		tmp = (efuse_cal_val[1] & DPRX_LN3_RX_IMPSEL) >> 21;
		tmp1 = ioread32(rg_ln3 + DPRX_ANA_LN_RX_3C);
		tmp1 &= ~(RG_XTP_LN_RX_IMPSEL);
		tmp1 |= RG_XTP_LN_RX_IMPSEL_VAL(tmp);
		iowrite32(tmp1, rg_ln3 + DPRX_ANA_LN_RX_3C);
	}
#endif
}

/** @ingroup IP_group_dprxphy_internal_function
 * @par Description
 *     DPRX PHY exit.
 * @param[in]
 *     dprxphy: structure of dprxphy for getting information.
 * @param[in]
 *     instance: structure of instance dprxphy for getting information.
 * @return
 *     none
 * @par Boundary case and Limitation
 *     If efuse_data is zero. return nothing to do.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void dprx_phy_instance_exit(struct mtk_dprxphy *dprxphy,
				   struct mtk_phy_instance *instance)
{
	pr_debug("dprx_phy_instance_exit\n");
}

/** @ingroup IP_group_dprxphy_internal_function
 * @par Description
 *     DPRX PHY power on.
 * @param[in]
 *     dprxphy: structure of dprxphy for getting information.
 * @param[in]
 *     instance: structure of instance dprxphy for getting information.
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void dprx_phy_instance_power_on(struct mtk_dprxphy *dprxphy,
	struct mtk_phy_instance *instance)
{
	pr_debug("dprx_phy_instance_power_on\n");

	pm_runtime_get_sync(&phy_pdev->dev);
}

/** @ingroup IP_group_dprxphy_internal_function
 * @par Description
 *     DPRX PHY power off.
 * @param[in]
 *     dprxphy: structure of dprxphy for getting information.
 * @param[in]
 *     instance: structure of instance dprxphy for getting information.
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void dprx_phy_instance_power_off(struct mtk_dprxphy *dprxphy,
	struct mtk_phy_instance *instance)
{
	pr_debug("dprx_phy_instance_power_off\n");

	pm_runtime_put_sync(&phy_pdev->dev);
}

/** @ingroup IP_group_dprxphy_internal_function
 * @par Description
 *     DPRX PHY bank address veriables initialization.
 * @param[in]
 *     dprxphy: structure of dprxphy for getting information.
 * @param[in]
 *     instance: structure of instance dprxphy for getting information.
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void phy_v2_banks_init(struct mtk_dprxphy *dprxphy,
			      struct mtk_phy_instance *instance)
{
	struct dprxphy_banks *dprx_banks = &instance->dprx_banks;

	pr_debug("phy_v2_banks_init\n");

	switch (instance->type) {
	case PHY_TYPE_DPRX:
		dprx_banks->xtp_ckm =
			instance->regs[0] + XTP_CKM_DP_SIFSLV_CORE;
		dprx_banks->dig_glb =
			instance->regs[1] + DPRX_DIG_GLB;
		dprx_banks->dig_ln0_rx2 =
			instance->regs[2] + DPRX_DIG_LN0_RX2;
		dprx_banks->dig_ln1_rx2 =
			instance->regs[2] + DPRX_DIG_LN1_RX2;
		dprx_banks->dig_ln2_rx2 =
			instance->regs[2] + DPRX_DIG_LN2_RX2;
		dprx_banks->dig_ln3_rx2 =
			instance->regs[2] + DPRX_DIG_LN3_RX2;
		dprx_banks->ana_glb =
			instance->regs[3] + DPRX_SIFSLV_ANA_GLB;
		dprx_banks->ana_ln0 =
			instance->regs[4] + DPRX_SIFSLV_ANA_LN0_RX;
		dprx_banks->ana_ln1 =
			instance->regs[4] + DPRX_SIFSLV_ANA_LN1_RX;
		dprx_banks->ana_ln2 =
			instance->regs[4] + DPRX_SIFSLV_ANA_LN2_RX;
		dprx_banks->ana_ln3 =
			instance->regs[4] + DPRX_SIFSLV_ANA_LN3_RX;
		break;
	default:
		dev_err(dprxphy->dev, "incompatible PHY type\n");
		return;
	}
}

/** @ingroup IP_group_dprxphy_internal_function
 * @par Description
 *     DPRX PHY initialization function.
 * @param[in]
 *     phy: structure of phy for getting information.
 * @return
 *     0, function success.\n
 *     -EINVAL, wrong parameter.\n
 *     error code from clk_prepare_enable().
 * @par Boundary case and Limitation
 *     1. If wrong parameter, return -EINVAL.\n
 *     2. If clk_prepare_enable() fail, return its error code.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_dprx_phy_init(struct phy *phy)
{
	struct mtk_phy_instance *instance = phy_get_drvdata(phy);
	struct mtk_dprxphy *dprxphy = dev_get_drvdata(phy->dev.parent);
	int ret;

	pr_debug("mtk_dprx_phy_init\n");

	ret = clk_prepare_enable(instance->ref_clk);
	if (ret) {
		dev_err(dprxphy->dev, "failed to enable ref_clk\n");
		return ret;
	}

	switch (instance->type) {
	case PHY_TYPE_DPRX:
		dprx_phy_instance_init(dprxphy, instance);
		break;
	default:
		dev_err(dprxphy->dev, "incompatible PHY type\n");
		return -EINVAL;
	}

	return 0;
}

/** @ingroup IP_group_dprxphy_internal_function
 * @par Description
 *     DPRX PHY power on function.
 * @param[in]
 *     phy: structure of phy for getting information.
 * @return
 *     0, always return function success.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_dprx_phy_power_on(struct phy *phy)
{
	struct mtk_phy_instance *instance = phy_get_drvdata(phy);
	struct mtk_dprxphy *dprxphy = dev_get_drvdata(phy->dev.parent);

	pr_debug("mtk_dprx_phy_power_on\n");
	dprx_phy_instance_power_on(dprxphy, instance);

	return 0;
}

/** @ingroup IP_group_dprxphy_internal_function
 * @par Description
 *     DPRX PHY power off function.
 * @param[in]
 *     phy: structure of phy for getting information.
 * @return
 *     0, always return function success.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_dprx_phy_power_off(struct phy *phy)
{
	struct mtk_phy_instance *instance = phy_get_drvdata(phy);
	struct mtk_dprxphy *dprxphy = dev_get_drvdata(phy->dev.parent);

	pr_debug("mtk_dprx_phy_power_off\n");
	dprx_phy_instance_power_off(dprxphy, instance);

	return 0;
}

/** @ingroup IP_group_dprxphy_internal_function
 * @par Description
 *     DPRX PHY exit function.
 * @param[in]
 *     phy: structure of phy for getting information.
 * @return
 *     0, always return function success.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_dprx_phy_exit(struct phy *phy)
{
	struct mtk_phy_instance *instance = phy_get_drvdata(phy);
	struct mtk_dprxphy *dprxphy = dev_get_drvdata(phy->dev.parent);

	pr_debug("mtk_dprx_phy_exit\n");

	dprx_phy_instance_exit(dprxphy, instance);
	clk_disable_unprepare(instance->ref_clk);
	return 0;
}

/** @ingroup IP_group_dprxphy_internal_function
 * @par Description
 *     DPRX PHY exit function.
 * @param[in]
 *     dev: structure of device for getting information.
 * @param[in]
 *    args: of_phandle_args struct pointer for getting information.\n
 * @return
 *     phy, If get information success return phy struct.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If wrong parameter, return -EINVAL.\n
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static struct phy *mtk_phy_xlate(struct device *dev,
				 struct of_phandle_args *args)
{
	struct mtk_dprxphy *dprxphy = dev_get_drvdata(dev);
	struct mtk_phy_instance *instance = NULL;
	struct device_node *phy_np = args->np;
	int index;

	pr_debug("mtk_phy_xlate\n");

	if (args->args_count != 1) {
		dev_err(dev, "invalid number of cells in 'phy' property\n");
		return ERR_PTR(-EINVAL);
	}

	for (index = 0; index < dprxphy->nphys; index++)
		if (phy_np == dprxphy->phys[index]->phy->dev.of_node) {
			instance = dprxphy->phys[index];
			break;
		}

	if (!instance) {
		dev_err(dev, "failed to find appropriate phy\n");
		return ERR_PTR(-EINVAL);
	}

	instance->type = args->args[0];
	if (!(instance->type == PHY_TYPE_DPRX)) {
		dev_err(dev, "unsupported device type: %d\n", instance->type);
		return ERR_PTR(-EINVAL);
	}

	phy_v2_banks_init(dprxphy, instance);

	return instance->phy;
}

/** @ingroup IP_group_dprxphy_internal_struct
* @brief MTK DP RX phy support ops function define.
*/
static const struct phy_ops mtk_dprx_phy_ops = {
	.init		= mtk_dprx_phy_init,
	.exit		= mtk_dprx_phy_exit,
	.power_on	= mtk_dprx_phy_power_on,
	.power_off	= mtk_dprx_phy_power_off,
	.owner		= THIS_MODULE,
};

/** @ingroup IP_group_dprxphy_internal_struct
* @brief MTK DP RX phy DT match ID define.
*/
static const struct of_device_id mtk_dprx_phy_id_table[] = {
	{ .compatible = "mediatek,mt3611-dprxphy"},
	{ .compatible = "mediatek,mt3612-dprxphy"},
	{ },
};
MODULE_DEVICE_TABLE(of, mtk_dprx_phy_id_table);

/** @ingroup IP_group_dprxphy_internal_function
 * @par Description
 *     MTK DP RX phy probe.
 * @param[in]
 *     pdev: structure of platform device for getting information.
 * @return
 *     0, function success.\n
 *     -EINVAL, device tree name not match.\n
 *     -ENOMEM, memory alloc fail.\n
 *     error code from devm_of_phy_provider_register().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If device tree name not match, return -EINVAL.\n
 *     2. If memory alloc fail, return -ENOMEM.\n
 *     3. If devm_of_phy_provider_register() fail, return its error code.\n
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_dprx_phy_probe(struct platform_device *pdev)
{
	const struct of_device_id *match;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *child_np;
	struct phy_provider *provider;
	struct mtk_dprxphy *dprxphy;
	struct resource *res;
	int port, retval;
	int i, ret;
	u32 *e_buf;
	struct nvmem_cell *cell;
	size_t e_len;
	struct mtk_phy_instance *instance;
	struct phy *phy;

	dev_dbg(dev, "mtk_dprx_phy_probe start ...\n");

	match = of_match_node(mtk_dprx_phy_id_table, pdev->dev.of_node);
	if (!match)
		return -EINVAL;

	dprxphy = devm_kzalloc(dev, sizeof(*dprxphy), GFP_KERNEL);
	if (!dprxphy)
		return -ENOMEM;

	dprxphy->nphys = 1;
	dprxphy->phys = devm_kcalloc(dev, dprxphy->nphys,
				     sizeof(*dprxphy->phys), GFP_KERNEL);
	if (!dprxphy->phys)
		return -ENOMEM;

	dprxphy->dev = dev;
	platform_set_drvdata(pdev, dprxphy);

	ret = of_property_read_u32(dev->of_node, "phy_hw_nr",
				   &dprxphy->phy_hw_nr);
	if (!ret) {
		dev_dbg(dev, "parsing phy_hw_nr: %d", dprxphy->phy_hw_nr);
	} else {
		dev_err(dev, "parsing phy_hw_nr error: %d\n", ret);
		return -EINVAL;
	}

	pm_runtime_enable(&pdev->dev);
	phy_pdev = pdev;

	port = 0;

	instance = devm_kzalloc(dev, sizeof(*instance), GFP_KERNEL);
	if (!instance) {
		retval = -ENOMEM;
		goto put_child;
	}

	dprxphy->phys[port] = instance;

	phy = devm_phy_create(dev, np, &mtk_dprx_phy_ops);
	if (IS_ERR(phy)) {
		dev_err(dev, "failed to create phy\n");
		retval = PTR_ERR(phy);
		goto put_child;
	}

	for (i = 0; i < dprxphy->phy_hw_nr; i++) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		instance->regs[i] = devm_ioremap_resource(&phy->dev, res);

		if (IS_ERR(instance->regs[i])) {
			retval = PTR_ERR(instance->regs[i]);
			dev_err(dev,
				"Failed to ioremap phy %d registers: %d\n", i,
				retval);
			goto put_child;
		}
		dev_dbg(dev, "phy adr 0x%lx --> 0x%lx\n",
			(unsigned long int)(res->start),
			(unsigned long int)(instance->regs[i]));
	}

	instance->phy = phy;
	instance->index = port;
	phy_set_drvdata(phy, instance);
	port++;

	instance->ref_clk = devm_clk_get(&phy->dev, "ref");
	if (IS_ERR(instance->ref_clk)) {
		dev_err(dev, "failed to get ref_clk(id-%d)\n", port);
		retval = PTR_ERR(instance->ref_clk);
		goto put_child;
	}

	/* get phy e-fuse data */
	cell = nvmem_cell_get(dev, "phy-dprx");
	if (IS_ERR_OR_NULL(cell)) {
		dprxphy->efuse_data[0] = 0;
		dprxphy->efuse_data[1] = 0;
	} else {
		e_buf = (u32 *)nvmem_cell_read(cell, &e_len);
		if (IS_ERR(e_buf)) {
			dprxphy->efuse_data[0] = 0;
			dprxphy->efuse_data[1] = 0;
		} else {
			dprxphy->efuse_data[0] = e_buf[0];
			dprxphy->efuse_data[1] = e_buf[1];
		}
		nvmem_cell_put(cell);
		kfree(e_buf);
		e_buf = NULL;
	}

	provider = devm_of_phy_provider_register(dev, mtk_phy_xlate);
	dev_dbg(dev, "mtk_dprx_phy_probe end ...\n");

	return PTR_ERR_OR_ZERO(provider);
put_child:
	of_node_put(child_np);
	return retval;
}

/** @ingroup IP_group_dprxphy_internal_function
 * @par Description
 *     DPRX PHY driver remove function
 * @param[in]
 *     pdev: platform_device
 * @return
 *     0, function is done
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_dprx_phy_remove(struct platform_device *pdev)
{
	pr_debug("dprx phy remove start...\n");

	pm_runtime_disable(&pdev->dev);

	pr_debug("dprx phy remove complete...\n");

	return 0;
}

/** @ingroup IP_group_dprxphy_internal_struct
* @brief MTK DP RX phy platform driver data structure define.
*/
static struct platform_driver mtk_dprx_phy_driver = {
	.probe		= mtk_dprx_phy_probe,
	.remove		= mtk_dprx_phy_remove,
	.driver		= {
		.name	= "mtk-dprxphy",
		.of_match_table = mtk_dprx_phy_id_table,
	},
};
module_platform_driver(mtk_dprx_phy_driver);

MODULE_DESCRIPTION("MediaTek DPRX PHY driver");
MODULE_LICENSE("GPL v2");
