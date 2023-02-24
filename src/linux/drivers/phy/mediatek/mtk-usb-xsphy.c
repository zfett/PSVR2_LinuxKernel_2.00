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
 * @file mtk-usb-xsphy.c
 * MediaTek USB xsphy Driver
 */

/**
 * @defgroup IP_group_usbphy USB_PHY
 *   The dirver use for MTK U2 and U3 USB xsphy.
 *
 *   @{
 *       @defgroup IP_group_usbphy_external EXTERNAL
 *         The external API document for xsphy. \n
 *
 *         @{
 *           @defgroup IP_group_usbphy_external_function 1.function
 *               none. No extra function in xsphy driver.
 *           @defgroup IP_group_usbphy_external_struct 2.Structure
 *               none. No extra Structure in xsphy driver.
 *           @defgroup IP_group_usbphy_external_typedef 3.Typedef
 *               none. No extra Typedef in xsphy driver.
 *           @defgroup IP_group_usbphy_external_enum 4.Enumeration
 *               none. No extra Enumeration in xsphy driver.
 *           @defgroup IP_group_usbphy_external_def 5.Define
 *               none. No extra Define in xsphy driver.
 *         @}
 *
 *       @defgroup IP_group_usbphy_internal INTERNAL
 *         The internal API document for xsphy. \n
 *
 *         @{
 *           @defgroup IP_group_usbphy_internal_function 1.function
 *               Internal function for MTK U2 and U3 usb xsphy initial.
 *           @defgroup IP_group_usbphy_internal_struct 2.Structure
 *               Internal structure used for xsphy driver.
 *           @defgroup IP_group_usbphy_internal_typedef 3.Typedef
 *               none. No extra Typedef in xsphy driver.
 *           @defgroup IP_group_usbphy_internal_enum 4.Enumeration
 *               Internal enumeration used for xsphy driver.
 *           @defgroup IP_group_usbphy_internal_def 5.Define
 *               Internal define used for xsphy driver.
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

/** @ingroup IP_group_usbphy_internal_typedef
 * @brief use for mtk xsphy register define.
 * @{
 */
/** u2 phy banks */
#define SSUSB_SIFSLV_U2FREQ		0x100
#define SSUSB_SIFSLV_U2PHY_COM		0x300

/** u3 phy banks */
#define SSPXTP_SIFSLV_PHYA_GLB		0x100
#define SSPXTP_SIFSLV_DIG_LN_TOP	0x400
#define SSPXTP_SIFSLV_DIG_LN_TX0	0x500
#define SSPXTP_SIFSLV_DIG_LN_RX0	0x600
#define SSPXTP_SIFSLV_DIG_LN_DAIF	0x700
#define SSPXTP_SIFSLV_PHYA_LN		0x800

/** CR define for module name: ssusb_sifslv_fmreg */
#define U3P_U2FREQ_FMCR0	0x00
#define P2F_RG_FREQDET_EN	BIT(24)
#define P2F_RG_CYCLECNT		GENMASK(23, 0)
#define P2F_RG_CYCLECNT_VAL(x)	((P2F_RG_CYCLECNT) & (x))

#define U3P_U2FMMONR0  0x0c

#define U3P_U2FREQ_FMMONR1	0x10
#define P2F_RG_FRCK_EN		BIT(8)

#define U3P_REF_CLK		26/* MHZ */
#define U3P_SLEW_RATE_COEF	17
#define U3P_SR_COEF_DIVISOR	1000
#define U3P_FM_DET_CYCLE_CNT	1024


/** CR define for module name: ssusb_sifslv_u2phy_com */
#define U3P_USBPHYACR0		0x000
#define PA0_RG_USB20_INTR_EN	BIT(5)

#define U3P_USBPHYACR1		0x004
#define RG_USB20_INTR_CAL	GENMASK(23, 19)
#define RG_USB20_INTR_CAL_VAL(x)	((0x1f & (x)) << 19)
#define RG_USB20_INTR_CAL_DEFAULT_VAL	0x14

#define U3P_USBPHYACR5			0x014
#define PA5_RG_U2_HSTX_SRCAL_EN		BIT(15)
#define PA5_RG_U2_HSTX_SRCTRL		GENMASK(14, 12)
#define PA5_RG_U2_HSTX_SRCTRL_VAL(x)	((0x7 & (x)) << 12)
#define HSTX_SRCTRL_DEF_VAL	0x04

#define U3P_USBPHYACR6		0x018
#define PA6_RG_U2_BC11_SW_EN	BIT(23)
#define PA6_RG_U2_DISCTH	GENMASK(7, 4)
#define PA6_RG_U2_DISCTH_VAL(x)	(0xf & (x))
#define PA6_RG_U2_SQTH		GENMASK(3, 0)
#define PA6_RG_U2_SQTH_VAL(x)	(0xf & (x))

#define U3P_U2PHYDTM1		0x06C
#define P2C_rg_force_vbusvalid	BIT(13)
#define P2C_rg_force_sessend	BIT(12)
#define P2C_rg_force_bvalid	BIT(11)
#define P2C_rg_force_avalid	BIT(10)
#define P2C_rg_force_iddig		BIT(9)
#define P2C_RG_VBUSVALID	BIT(5)
#define P2C_RG_SESSEND		BIT(4)
#define P2C_RG_BVALID		BIT(3)
#define P2C_RG_AVALID		BIT(2)
#define P2C_RG_IDDIG		BIT(1)

/** CR define for module name: sspxtp_sifslv_phya_glb */
#define SSPXTP_PHYA_GLB_00	0x000
#define RG_XTP_GLB_BIAS_INTR_CTRL		GENMASK(21, 16)
#define RG_XTP_GLB_BIAS_INTR_CTRL_VAL(x)	((0x3f & (x)) << 16)
#define RG_XTP_GLB_BIAS_INTR_CTRL_DEFAULT_VAL	0x1c

/** CR define for module name: sspxtp_sifslv_dig_ln_top */
#define SSPXTP_DIG_LN_TOP_04	0x004

/** CR define for module name: sspxtp_sifslv_dig_ln_tx0 */
#define SSPXTP_DIG_LN_TX0_0C	0x00c

#define SSPXTP_DIG_LN_TX0_2C	0x02c

/** CR define for module name: sspxtp_sifslv_dig_ln_rx0 */
#define SSPXTP_DIG_LN_RX0_60	0x060

#define SSPXTP_DIG_LN_RX0_88	0x088

/** CR define for module name: sspxtp_sifslv_dig_ln_daif */
#define SSPXTP_DIG_LN_DAIF_28	0x028
#define RG_DAIF_LN_G1_RX_CDR_PDKKPC	GENMASK(4, 0)
#define RG_DAIF_LN_G1_RX_CDR_PDKKPC_DEFAULT_VAL	0x9

/** CR define for module name: sspxtp_sifslv_phya_ln */
#define SSPXTP_PHYA_LN_04	0x004
#define RG_XTP_LN0_TX_IMPSEL_NMOS		GENMASK(10, 6)
#define RG_XTP_LN0_TX_IMPSEL_PMOS		GENMASK(5, 1)
#define RG_XTP_LN0_TX_IMPSEL_VAL_NMOS(x)	((0x1f & (x)) << 6)
#define RG_XTP_LN0_TX_IMPSEL_VAL_PMOS(x)	((0x1f & (x)) << 1)
#define RG_XTP_LN0_TX_IMPSEL_DEFAULT_VAL_NMOS	0xb
#define RG_XTP_LN0_TX_IMPSEL_DEFAULT_VAL_PMOS	0x12

#define SSPXTP_PHYA_LN_14	0x014
#define SSPXTP_PHYA_LN_58	0x058
#define RG_XTP_LN0_RX_IMPSEL		GENMASK(4, 0)
#define RG_XTP_LN0_RX_IMPSEL_VAL(x)	(0x1f & (x))
#define RG_XTP_LN0_RX_IMPSEL_DEFAULT_VAL	0x14

/** CR define for module name: xtp_ckm_u3_sifslv_core */
#define XTP_CKM_DA_REG_20	0x020
#define RG_CKM_FRC_AVD10_ON	BIT(2)
#define RG_CKM_AVD10_ON	BIT(1)

#define XTP_CKM_DA_REG_3C	0x03c
#define RG_CKM_FRC_EN		BIT(5)



/** EFuse Calibration Result Mask */
#define EFUSE_USB20_H_VALID_MASK	0x10000000
#define EFUSE_USB20_D_VALID_MASK	 0x8000000

#define USB20_EFUSE_P0_HOST_INTR_MASK 0x00001f00
#define USB20_EFUSE_P2_DEVICE_INTR_MASK 0x0000001f

#define EFUSE_SSUSB_D_VALID_MASK	0x80000000

#define SSPXTP_EFUSE_GLB_BIAS_INTR_CTRL_P0_MASK 0x0000003f
#define SSPXTP_EFUSE_LN0_TX_IMPSEL_P0 0x0001f000
#define SSPXTP_EFUSE_LN0_RX_IMPSEL_P0 0x003e0000

#define SSPXTP_EFUSE_GLB_BIAS_INTR_CTRL_P2 0x0000003F
#define SSPXTP_EFUSE_LN0_TX_IMPSEL_P2_NMOS 0x0000F800
#define SSPXTP_EFUSE_LN0_TX_IMPSEL_P2_PMOS 0x000007C0
#define SSPXTP_EFUSE_LN0_RX_IMPSEL_P2 0x001F0000

/** EFuse offset address for usb phy */
#define EFUSE_U2_DATA_OFF	0x184
#define EFUSE_U3_DATA_OFF	0x188

/**
 * @}
 */

/** @ingroup IP_group_usbphy_internal_enum
 * @brief for MTK xsphy version.
 *
 */
enum mtk_phy_version {
/** phy version p0 for mtk usb host usage */
	MTK_PHY_P0 = 1,
/** phy version p2 for mtk usb device usage */
	MTK_PHY_P2,
};

/** @ingroup IP_group_usbphy_internal_struct
* @brief MTK xsphy data information.
*/
/**
 * struct mtk_phy_pdata: MTK phy data informationn.\n
 * avoid_rx_sen_degradation: for special usage.\n
 * version: mtk phy varsion.\n
 */
struct mtk_phy_pdata {
	/* avoid RX sensitivity level degradation only for mt8173 */
	bool avoid_rx_sen_degradation;
	enum mtk_phy_version version;
};

/** @ingroup IP_group_usbphy_internal_struct
* @brief MTK phy u2 register bank.
*/
/**
 * struct u2phy_banks: phy u2 register bank.\n
 * misc: mtk u2 phy register misc bank\n
 * fmreg: mtk u2 phy register fmreg bank\n
 * com: mtk u2 phy register com bank\n
 */
struct u2phy_banks {
	void __iomem *misc;
	void __iomem *fmreg;
	void __iomem *com;
};

/** @ingroup IP_group_usbphy_internal_struct
* @brief MTK phy u3 register bank.
*/
/**
 * struct u3phy_banks: phy u3 register bank.\n
 * phya_glb: mtk u3 phy register phya_glb bank\n
 * dig_ln_top: mtk u3 phy register dig_ln_top bank\n
 * dig_ln_tx0: mtk u3 phy register dig_ln_tx0 bank\n
 * dig_ln_rx0: mtk u3 phy register dig_ln_rx0 bank\n
 * phya_ln: mtk u3 phy register phya_ln bank\n
 */
struct u3phy_banks {
	void __iomem *phya_glb;
	void __iomem *dig_ln_top;
	void __iomem *dig_ln_tx0;
	void __iomem *dig_ln_rx0;
	void __iomem *dig_ln_daif;
	void __iomem *phya_ln;
};

/** @ingroup IP_group_usbphy_internal_struct
* @brief MTK one phy data information.
*/
/**
 * struct mtk_phy_instance: MTK one phy data information.\n
 * phy: phy struct of MTK phy\n
 * port_base: phy register base address of MTK phy.\n
 * u2_banks , u3_banks: u2 or u3 phy bank struct.\n
 * ref_clk: clk struct of MTK phy.\n
 * index: phy number\n
 * type: phy type. PHY_TYPE_USB2 or PHY_TYPE_USB3 \n
 */
struct mtk_phy_instance {
	struct phy *phy;
	void __iomem *port_base;
	union {
		struct u2phy_banks u2_banks;
		struct u3phy_banks u3_banks;
	};
	struct clk *ref_clk;	/* reference clock of anolog phy */
	u32 index;
	u8 type;
	void __iomem *xtp_clk_u3_base;
};

/** @ingroup IP_group_usbphy_internal_struct
* @brief MTK total usb xsphy data information.
*/
/**
 * struct mtk_xsphy: MTK total usb xsphy data information.\n
 * device *dev: device struct of MTK XSPHY.\n
 * sif_base: shard sif bank base address.\n
 * u3phya_ref: reference clock of usb3 anolog phy.\n
 * pdata: mtk_phy_pdata struct.\n
 * phys: mtk_phy_instance struct.\n
 * nphys: total xsphy numbers\n
 * u2_efuse_data: store usb phy u2 calibration e-fuse data.\n
 * u3_efuse_data: store usb phy u3 calibration e-fuse data.\n
 */
struct mtk_xsphy {
	struct device *dev;
	void __iomem *sif_base;	/* only shared sif */
	/* deprecated, use @ref_clk instead in phy instance */
	struct clk *u3phya_ref;	/* reference clock of usb3 anolog phy */
	const struct mtk_phy_pdata *pdata;
	struct mtk_phy_instance **phys;
	int nphys;
	u32 u2_efuse_data;
	u32 u3_efuse_data;
};

/** @ingroup IP_group_usbphy_internal_function
 * @par Description
 *     MTK usb u2 phy slew rete calibration.
 * @param
 * xsphy: mtk_xsphy struct pointer.\n
 * instance: mtk_phy_instance struct pointer.\n
 *
 * @param[in]
 *     xsphy: structure of all xsphy for getting information.
 * @param[in]
 *     instance: structure of instance xsphy for getting information.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void hs_slew_rate_calibrate(struct mtk_xsphy *xsphy,
	struct mtk_phy_instance *instance)
{
	struct u2phy_banks *u2_banks = &instance->u2_banks;
	void __iomem *com = u2_banks->com;
	int calibration_val = HSTX_SRCTRL_DEF_VAL;
	u32 tmp;

	/* set HS slew rate */
	tmp = readl(com + U3P_USBPHYACR5);
	tmp &= ~PA5_RG_U2_HSTX_SRCTRL;
	tmp |= PA5_RG_U2_HSTX_SRCTRL_VAL(calibration_val);
	writel(tmp, com + U3P_USBPHYACR5);
}

/** @ingroup IP_group_usbphy_internal_function
 * @par Description
 *     MTK usb u3 xsphy initialization.
 * @param[in]
 *     xsphy: structure of all xsphy for getting information.
 * @param[in]
 *     instance: structure of instance xsphy for getting information.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     If u3_efuse_data is zero. return nothing to do.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void u3_phy_instance_init(struct mtk_xsphy *xsphy,
	struct mtk_phy_instance *instance)
{
	struct u3phy_banks *u3_banks = &instance->u3_banks;
	u32 read_value;
	u32 intr_ctrl;
	u32 tx_impsel_nmos;
	u32 tx_impsel_pmos;
	u32 rx_impsel;
#ifdef CONFIG_MTK_USB_PHY_PGAIN
	void __iomem *dig_ln_daif;
#endif

	if (xsphy->pdata->version == MTK_PHY_P2) {
		if (xsphy->u3_efuse_data & EFUSE_SSUSB_D_VALID_MASK) {
			/* Set INTR */
			intr_ctrl = ((xsphy->u3_efuse_data) &
				(SSPXTP_EFUSE_GLB_BIAS_INTR_CTRL_P2)) >> 0;
			/* Set TX_Impedance NMOS */
			tx_impsel_nmos = ((xsphy->u3_efuse_data) &
				(SSPXTP_EFUSE_LN0_TX_IMPSEL_P2_NMOS)) >> 11;
			/* Set TX_Impedance PMOS */
			tx_impsel_pmos = ((xsphy->u3_efuse_data) &
				(SSPXTP_EFUSE_LN0_TX_IMPSEL_P2_PMOS)) >> 6;
			/* Set RX_Impedance */
			rx_impsel = ((xsphy->u3_efuse_data) &
				(SSPXTP_EFUSE_LN0_RX_IMPSEL_P2)) >> 16;
		} else {
			intr_ctrl = RG_XTP_GLB_BIAS_INTR_CTRL_DEFAULT_VAL;
			tx_impsel_nmos = RG_XTP_LN0_TX_IMPSEL_DEFAULT_VAL_NMOS;
			tx_impsel_pmos = RG_XTP_LN0_TX_IMPSEL_DEFAULT_VAL_PMOS;
			rx_impsel = RG_XTP_LN0_RX_IMPSEL_DEFAULT_VAL;
		}

		read_value = readl(u3_banks->phya_glb + SSPXTP_PHYA_GLB_00);
		read_value &= ~RG_XTP_GLB_BIAS_INTR_CTRL;
		read_value |= RG_XTP_GLB_BIAS_INTR_CTRL_VAL(intr_ctrl);
		writel(read_value, u3_banks->phya_glb + SSPXTP_PHYA_GLB_00);

		read_value = readl(u3_banks->phya_ln + SSPXTP_PHYA_LN_58);
		read_value &= ~RG_XTP_LN0_TX_IMPSEL_NMOS;
		read_value |= RG_XTP_LN0_TX_IMPSEL_VAL_NMOS(tx_impsel_nmos);
		writel(read_value, u3_banks->phya_ln + SSPXTP_PHYA_LN_58);

		read_value = readl(u3_banks->phya_ln + SSPXTP_PHYA_LN_58);
		read_value &= ~RG_XTP_LN0_TX_IMPSEL_PMOS;
		read_value |= RG_XTP_LN0_TX_IMPSEL_VAL_PMOS(tx_impsel_pmos);
		writel(read_value, u3_banks->phya_ln + SSPXTP_PHYA_LN_58);

		read_value = readl(u3_banks->phya_ln + SSPXTP_PHYA_LN_14);
		read_value &= ~RG_XTP_LN0_RX_IMPSEL;
		read_value |= RG_XTP_LN0_RX_IMPSEL_VAL(rx_impsel);
		writel(read_value, u3_banks->phya_ln + SSPXTP_PHYA_LN_14);

#ifdef CONFIG_MTK_USB_PHY_PGAIN
		dig_ln_daif = u3_banks->dig_ln_daif + SSPXTP_DIG_LN_DAIF_28;
		read_value = readl(dig_ln_daif);
		read_value &= ~RG_DAIF_LN_G1_RX_CDR_PDKKPC;
		read_value |= RG_DAIF_LN_G1_RX_CDR_PDKKPC_DEFAULT_VAL;
		writel(read_value, dig_ln_daif);
#endif
	}

	dev_dbg(xsphy->dev, "%s(%d)\n", __func__, instance->index);
}

/** @ingroup IP_group_usbphy_internal_function
 * @par Description
 *     MTK usb u2 xsphy initialization.
 * @param[in]
 *     xsphy: structure of all xsphy for getting information.
 * @param[in]
 *     instance: structure of instance xsphy for getting information.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     If u2_efuse_data is zero. return nothing to do.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void u2_phy_instance_init(struct mtk_xsphy *xsphy,
	struct mtk_phy_instance *instance)
{
	struct u2phy_banks *u2_banks = &instance->u2_banks;
	void __iomem *com = u2_banks->com;
	u32 tmp;
	u32 efuse_cal_val;

	tmp = readl(com + U3P_USBPHYACR6);
	tmp &= ~PA6_RG_U2_BC11_SW_EN;	/* DP/DM BC1.1 path Disable */
	writel(tmp, com + U3P_USBPHYACR6);

	if (xsphy->pdata->version == MTK_PHY_P2) {
		tmp = readl(com + U3P_U2PHYDTM1);
		tmp |= P2C_RG_VBUSVALID;
		writel(tmp, com + U3P_U2PHYDTM1);

		tmp = readl(com + U3P_U2PHYDTM1);
		tmp &= ~P2C_RG_SESSEND;
		writel(tmp, com + U3P_U2PHYDTM1);

		tmp = readl(com + U3P_U2PHYDTM1);
		tmp |= P2C_RG_BVALID;
		writel(tmp, com + U3P_U2PHYDTM1);

		tmp = readl(com + U3P_U2PHYDTM1);
		tmp |= P2C_RG_AVALID;
		writel(tmp, com + U3P_U2PHYDTM1);

		tmp = readl(com + U3P_U2PHYDTM1);
		tmp |= P2C_RG_IDDIG;
		writel(tmp, com + U3P_U2PHYDTM1);

		tmp = readl(com + U3P_U2PHYDTM1);
		tmp |= P2C_rg_force_vbusvalid;
		writel(tmp, com + U3P_U2PHYDTM1);

		tmp = readl(com + U3P_U2PHYDTM1);
		tmp |= P2C_rg_force_sessend;
		writel(tmp, com + U3P_U2PHYDTM1);

		tmp = readl(com + U3P_U2PHYDTM1);
		tmp |= P2C_rg_force_bvalid;
		writel(tmp, com + U3P_U2PHYDTM1);

		tmp = readl(com + U3P_U2PHYDTM1);
		tmp |= P2C_rg_force_avalid;
		writel(tmp, com + U3P_U2PHYDTM1);

		tmp = readl(com + U3P_U2PHYDTM1);
		tmp |= P2C_rg_force_iddig;
		writel(tmp, com + U3P_U2PHYDTM1);
		mdelay(2);	/* wait device MAC ready */
	}

	efuse_cal_val = xsphy->u2_efuse_data;

	if (xsphy->pdata->version == MTK_PHY_P0) {
		/* Set Current calibration result */
		if (efuse_cal_val & EFUSE_USB20_H_VALID_MASK) {
			efuse_cal_val = ((efuse_cal_val &
				USB20_EFUSE_P0_HOST_INTR_MASK) >> 8);
		} else {
			efuse_cal_val = RG_USB20_INTR_CAL_DEFAULT_VAL;
		}

		tmp = readl(com + U3P_USBPHYACR1);
		tmp &= ~RG_USB20_INTR_CAL;
		tmp |= RG_USB20_INTR_CAL_VAL(efuse_cal_val);
		writel(tmp, com + U3P_USBPHYACR1);
	} else {
		/* Set Current calibration result */
		if (efuse_cal_val&EFUSE_USB20_D_VALID_MASK) {
			efuse_cal_val = ((efuse_cal_val &
				USB20_EFUSE_P2_DEVICE_INTR_MASK) >> 0);
		} else {
			efuse_cal_val = RG_USB20_INTR_CAL_DEFAULT_VAL;
		}

		tmp = readl(com + U3P_USBPHYACR1);
		tmp &= ~RG_USB20_INTR_CAL;
		tmp |= RG_USB20_INTR_CAL_VAL(efuse_cal_val);
		writel(tmp, com + U3P_USBPHYACR1);
	}
}

/** @ingroup IP_group_usbphy_internal_function
 * @par Description
 *     MTK usb u2 phy power on.
 * @param[in]
 *     xsphy: structure of all xsphy for getting information.
 * @param[in]
 *     instance: structure of instance xsphy for getting information.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void u2_phy_instance_power_on(struct mtk_xsphy *xsphy,
	struct mtk_phy_instance *instance)
{
	u32 index = instance->index;

	struct u2phy_banks *u2_banks = &instance->u2_banks;
	void __iomem *com = u2_banks->com;
	u32 tmp;

	tmp = readl(com + U3P_USBPHYACR6);
	tmp &= ~PA6_RG_U2_BC11_SW_EN;	/* DP/DM BC1.1 path Disable */
	writel(tmp, com + U3P_USBPHYACR6);

	dev_dbg(xsphy->dev, "%s(%d)\n", __func__, index);
}

/** @ingroup IP_group_usbphy_internal_function
 * @par Description
 *     MTK usb u2 phy power off.
 * @param[in]
 *     xsphy: structure of all xsphy for getting information.
 * @param[in]
 *     instance: structure of instance xsphy for getting information.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void u2_phy_instance_power_off(struct mtk_xsphy *xsphy,
	struct mtk_phy_instance *instance)
{
	u32 index = instance->index;
	struct u2phy_banks *u2_banks = &instance->u2_banks;
	void __iomem *com = u2_banks->com;
	u32 tmp;

	tmp = readl(com + U3P_USBPHYACR6);
	tmp |= PA6_RG_U2_BC11_SW_EN;	/* DP/DM BC1.1 path Enable */
	writel(tmp, com + U3P_USBPHYACR6);


	dev_dbg(xsphy->dev, "%s(%d)\n", __func__, index);
}

/** @ingroup IP_group_usbphy_internal_function
 * @par Description
 *     MTK usb u2 phy exit.
 * @param[in]
 *     xsphy: structure of all xsphy for getting information.
 * @param[in]
 *     instance: structure of instance xsphy for getting information.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void u2_phy_instance_exit(struct mtk_xsphy *xsphy,
	struct mtk_phy_instance *instance)
{

}

/** @ingroup IP_group_usbphy_internal_function
 * @par Description
 *     MTK usb phy bank address veriables initialization.
 * @param[in]
 *     xsphy: structure of all xsphy for getting information.
 * @param[in]
 *     instance: structure of instance xsphy for getting information.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void phy_v2_banks_init(struct mtk_xsphy *xsphy,
			      struct mtk_phy_instance *instance)
{
	struct u2phy_banks *u2_banks = &instance->u2_banks;
	struct u3phy_banks *u3_banks = &instance->u3_banks;

	switch (instance->type) {
	case PHY_TYPE_USB2:
		u2_banks->misc = instance->port_base;
		u2_banks->fmreg = instance->port_base + SSUSB_SIFSLV_U2FREQ;
		u2_banks->com = instance->port_base + SSUSB_SIFSLV_U2PHY_COM;
		break;
	case PHY_TYPE_USB3:
		u3_banks->phya_glb =
			instance->port_base + SSPXTP_SIFSLV_PHYA_GLB;
		u3_banks->dig_ln_top =
			instance->port_base + SSPXTP_SIFSLV_DIG_LN_TOP;
		u3_banks->dig_ln_tx0 =
			instance->port_base + SSPXTP_SIFSLV_DIG_LN_TX0;
		u3_banks->dig_ln_rx0 =
			instance->port_base + SSPXTP_SIFSLV_DIG_LN_RX0;
		u3_banks->dig_ln_daif =
			instance->port_base + SSPXTP_SIFSLV_DIG_LN_DAIF;
		u3_banks->phya_ln =
			instance->port_base + SSPXTP_SIFSLV_PHYA_LN;
		break;
	default:
		dev_err(xsphy->dev, "incompatible PHY type\n");
		return;
	}
}

/** @ingroup IP_group_usbphy_internal_function
 * @par Description
 *     MTK usb phy initialization function.
 * @param[in]
 *     phy: structure of phy for getting information.
 * @return
 *     0, function success.\n
 *     -EINVAL, wrong parameter.\n
 *     error code from clk_prepare_enable().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If wrong parameter, return -EINVAL.\n
 *     2. If clk_prepare_enable() fail, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_phy_init(struct phy *phy)
{
	struct mtk_phy_instance *instance = phy_get_drvdata(phy);
	struct mtk_xsphy *xsphy = dev_get_drvdata(phy->dev.parent);
	int ret;

	ret = clk_prepare_enable(xsphy->u3phya_ref);
	if (ret) {
		dev_err(xsphy->dev, "failed to enable u3phya_ref\n");
		return ret;
	}

	ret = clk_prepare_enable(instance->ref_clk);
	if (ret) {
		dev_err(xsphy->dev, "failed to enable ref_clk\n");
		return ret;
	}

	switch (instance->type) {
	case PHY_TYPE_USB2:
		u2_phy_instance_init(xsphy, instance);
		break;
	case PHY_TYPE_USB3:
		u3_phy_instance_init(xsphy, instance);
		break;
	default:
		dev_err(xsphy->dev, "incompatible PHY type\n");
		return -EINVAL;
	}

	return 0;
}

/** @ingroup IP_group_usbphy_internal_function
 * @par Description
 *     MTK usb phy power on function.
 * @param[in]
 *     phy: structure of phy for getting information.
 * @return
 *     0, always return function success.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_phy_power_on(struct phy *phy)
{
	struct mtk_phy_instance *instance = phy_get_drvdata(phy);
	struct mtk_xsphy *xsphy = dev_get_drvdata(phy->dev.parent);
	void __iomem *com = instance->xtp_clk_u3_base;
	u32 tmp;

	if (instance->type == PHY_TYPE_USB2) {
		u2_phy_instance_power_on(xsphy, instance);
		hs_slew_rate_calibrate(xsphy, instance);
	}

	if ((xsphy->pdata->version == MTK_PHY_P0) && com) {

		tmp = readl(com + XTP_CKM_DA_REG_20);
		tmp &= ~RG_CKM_FRC_AVD10_ON;
		tmp |= RG_CKM_AVD10_ON;
		writel(tmp, com + XTP_CKM_DA_REG_20);

		tmp = readl(com + XTP_CKM_DA_REG_3C);
		tmp &= ~RG_CKM_FRC_EN;
		writel(tmp, com + XTP_CKM_DA_REG_3C);
	}
	return 0;
}

/** @ingroup IP_group_usbphy_internal_function
 * @par Description
 *     MTK usb phy power off function.
 * @param[in]
 *     phy: structure of phy for getting information.
 * @return
 *     0, always return function success.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_phy_power_off(struct phy *phy)
{
	struct mtk_phy_instance *instance = phy_get_drvdata(phy);
	struct mtk_xsphy *xsphy = dev_get_drvdata(phy->dev.parent);
	void __iomem *com = instance->xtp_clk_u3_base;
	u32 tmp;

	if (instance->type == PHY_TYPE_USB2)
		u2_phy_instance_power_off(xsphy, instance);
	if ((xsphy->pdata->version == MTK_PHY_P0) && com) {

		tmp = readl(com + XTP_CKM_DA_REG_3C);
		tmp |= RG_CKM_FRC_EN;
		writel(tmp, com + XTP_CKM_DA_REG_3C);

		tmp = readl(com + XTP_CKM_DA_REG_20);
		tmp |= RG_CKM_FRC_AVD10_ON;
		tmp &= ~RG_CKM_AVD10_ON;
		writel(tmp, com + XTP_CKM_DA_REG_20);
	}

	return 0;
}

/** @ingroup IP_group_usbphy_internal_function
 * @par Description
 *     MTK usb phy exit function.
 * @param[in]
 *     phy: structure of phy for getting information.
 * @return
 *     0, always return function success.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_phy_exit(struct phy *phy)
{
	struct mtk_phy_instance *instance = phy_get_drvdata(phy);
	struct mtk_xsphy *xsphy = dev_get_drvdata(phy->dev.parent);

	if (instance->type == PHY_TYPE_USB2)
		u2_phy_instance_exit(xsphy, instance);

	clk_disable_unprepare(instance->ref_clk);
	clk_disable_unprepare(xsphy->u3phya_ref);

	return 0;
}

/** @ingroup IP_group_usbphy_internal_function
 * @par Description
 *     Get MTK usb mtk_phy_instance data.
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
	struct mtk_xsphy *xsphy = dev_get_drvdata(dev);
	struct mtk_phy_instance *instance = NULL;
	struct device_node *phy_np = args->np;
	int index;

	if (args->args_count != 1) {
		dev_err(dev, "invalid number of cells in 'phy' property\n");
		return ERR_PTR(-EINVAL);
	}

	for (index = 0; index < xsphy->nphys; index++)
		if (phy_np == xsphy->phys[index]->phy->dev.of_node) {
			instance = xsphy->phys[index];
			break;
		}

	if (!instance) {
		dev_err(dev, "failed to find appropriate phy\n");
		return ERR_PTR(-EINVAL);
	}

	instance->type = args->args[0];
	if (!(instance->type == PHY_TYPE_USB2 ||
		instance->type == PHY_TYPE_USB3)) {
		dev_err(dev, "unsupported device type: %d\n", instance->type);
		return ERR_PTR(-EINVAL);
	}

	if ((xsphy->pdata->version == MTK_PHY_P0) ||
		(xsphy->pdata->version == MTK_PHY_P2)) {
		phy_v2_banks_init(xsphy, instance);
	} else {
		dev_err(dev, "phy version is not supported\n");
		return ERR_PTR(-EINVAL);
	}

	return instance->phy;
}

/** @ingroup IP_group_usbphy_internal_struct
* @brief MTK phy support ops function define.
*/
static const struct phy_ops mtk_xsphy_ops = {
	.init		= mtk_phy_init,
	.exit		= mtk_phy_exit,
	.power_on	= mtk_phy_power_on,
	.power_off	= mtk_phy_power_off,
	.owner		= THIS_MODULE,
};

/** @ingroup IP_group_usbphy_internal_struct
* @brief MTK version 1 phy data define.
*/
static const struct mtk_phy_pdata xsphy_p0_pdata = {
	.avoid_rx_sen_degradation = false,
	.version = MTK_PHY_P0,
};

/** @ingroup IP_group_usbphy_internal_struct
* @brief MTK version 2 phy data define.
*/
static const struct mtk_phy_pdata xsphy_p2_pdata = {
	.avoid_rx_sen_degradation = false,
	.version = MTK_PHY_P2,
};

/** @ingroup IP_group_usbphy_internal_struct
* @brief MTK usb phy DT match ID define.
*/
static const struct of_device_id mtk_xsphy_id_table[] = {
	{ .compatible = "mediatek,mt3611-u3phy0", .data = &xsphy_p0_pdata },
	{ .compatible = "mediatek,mt3611-u3phy2", .data = &xsphy_p2_pdata },
	{ .compatible = "mediatek,mt3612-u3phy0", .data = &xsphy_p0_pdata },
	{ .compatible = "mediatek,mt3612-u3phy2", .data = &xsphy_p2_pdata },
	{ },
};
MODULE_DEVICE_TABLE(of, mtk_xsphy_id_table);

/** @ingroup IP_group_usbphy_internal_function
 * @par Description
 *     MTK usb xsphy probe.
 * @param[in]
 *     pdev: structure of platform device for getting information.
 * @return
 *     0, function success.\n
 *     -EINVAL, device tree name not match.\n
 *     -ENOMEM, memory alloc fail.\n
 *     -EPROBE_DEFER, driver requests probe retry.\n
 *     error code from devm_of_phy_provider_register().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If device tree name not match, return -EINVAL.\n
 *     2. If memory alloc fail, return -ENOMEM.\n
 *     3. If driver requests probe retry, return -EPROBE_DEFER.\n
 *     4. If devm_of_phy_provider_register() fail, return its error code.\n
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_xsphy_probe(struct platform_device *pdev)
{
	const struct of_device_id *match;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *child_np;
	struct phy_provider *provider;
	struct mtk_xsphy *xsphy;
	struct resource res;
	int port, retval;
	u32 *e_buf;
	struct nvmem_cell *cell;
	size_t e_len;


	match = of_match_node(mtk_xsphy_id_table, pdev->dev.of_node);
	if (!match)
		return -EINVAL;

	xsphy = devm_kzalloc(dev, sizeof(*xsphy), GFP_KERNEL);
	if (!xsphy)
		return -ENOMEM;

	xsphy->pdata = match->data;
	xsphy->nphys = of_get_child_count(np);
	xsphy->phys = devm_kcalloc(dev, xsphy->nphys,
				       sizeof(*xsphy->phys), GFP_KERNEL);
	if (!xsphy->phys)
		return -ENOMEM;

	xsphy->dev = dev;
	platform_set_drvdata(pdev, xsphy);

	/* it's deprecated, make it optional for backward compatibility */
	/*
	* xsphy->u3phya_ref = devm_clk_get(dev, "u3phya_ref");
	* if (IS_ERR(xsphy->u3phya_ref)) {
	*	if (PTR_ERR(xsphy->u3phya_ref) == -EPROBE_DEFER)
	*		return -EPROBE_DEFER;
	*
	*	xsphy->u3phya_ref = NULL;
	* }
	*/
	xsphy->u3phya_ref = NULL;

	port = 0;
	for_each_child_of_node(np, child_np) {
		struct mtk_phy_instance *instance;
		struct phy *phy;

		instance = devm_kzalloc(dev, sizeof(*instance), GFP_KERNEL);
		if (!instance) {
			retval = -ENOMEM;
			goto put_child;
		}

		xsphy->phys[port] = instance;

		phy = devm_phy_create(dev, child_np, &mtk_xsphy_ops);
		if (IS_ERR(phy)) {
			dev_err(dev, "failed to create phy\n");
			retval = PTR_ERR(phy);
			goto put_child;
		}

		retval = of_address_to_resource(child_np, 0, &res);
		if (retval) {
			dev_err(dev, "failed to get address resource(id-%d)\n",
				port);
			goto put_child;
		}

		instance->port_base = devm_ioremap_resource(&phy->dev, &res);
		if (IS_ERR(instance->port_base)) {
			dev_err(dev, "failed to remap phy regs\n");
			retval = PTR_ERR(instance->port_base);
			goto put_child;
		}

		retval = of_address_to_resource(child_np, 1, &res);
		if (retval) {
			dev_dbg(dev, "failed to get xtp address resource(id-%d)\n",
				port);
			instance->xtp_clk_u3_base = NULL;
		} else {
			instance->xtp_clk_u3_base =
				devm_ioremap_resource(&phy->dev, &res);
			if (IS_ERR(instance->xtp_clk_u3_base)) {
				dev_err(dev, "failed to remap xtp phy regs\n");
				instance->xtp_clk_u3_base = NULL;
			}
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
	}

	/* get phy e-fuse data */
	cell = nvmem_cell_get(dev, "phy-usb");
	if (IS_ERR_OR_NULL(cell)) {
		xsphy->u2_efuse_data = 0;
		xsphy->u3_efuse_data = 0;
	} else {
		e_buf = (u32 *)nvmem_cell_read(cell, &e_len);
		if (IS_ERR(e_buf)) {
			xsphy->u2_efuse_data = 0;
			xsphy->u3_efuse_data = 0;
		} else {
			xsphy->u2_efuse_data = e_buf[0];
			xsphy->u3_efuse_data = e_buf[2];
		}
		nvmem_cell_put(cell);
		kfree(e_buf);
		e_buf = NULL;
	}

	provider = devm_of_phy_provider_register(dev, mtk_phy_xlate);

	return PTR_ERR_OR_ZERO(provider);
put_child:
	of_node_put(child_np);
	return retval;
}

/** @ingroup IP_group_usbphy_internal_struct
* @brief MTK xsphy platform driver data structure define.
*/
static struct platform_driver mtk_xsphy_driver = {
	.probe		= mtk_xsphy_probe,
	.driver		= {
		.name	= "mtk-xsphy",
		.of_match_table = mtk_xsphy_id_table,
	},
};

module_platform_driver(mtk_xsphy_driver);

MODULE_AUTHOR("Chunfeng Yun <chunfeng.yun@mediatek.com>");
MODULE_DESCRIPTION("MediaTek USB XS-PHY driver");
MODULE_LICENSE("GPL v2");
