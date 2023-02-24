/*
 * MediaTek xHCI Host Controller Driver
 *
 * Copyright (c) 2015 MediaTek Inc.
 * Author:
 *  Chunfeng Yun <chunfeng.yun@mediatek.com>
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
 * @file xhci-mtk.c
 * MediaTek xHCI Host Controller Driver
 */

/**
 * @defgroup IP_group_usbhost USB_HOST
 *   The dirver use for MTK USB Host.
 *
 *   @{
 *       @defgroup IP_group_usbhost_external EXTERNAL
 *         The external API document for MTK USB Host. \n
 *
 *         @{
 *           @defgroup IP_group_usbhost_external_function 1.function
 *               none. No extra function in MTK USB Host driver.
 *           @defgroup IP_group_usbhost_external_struct 2.Structure
 *               none. No extra Structure in MTK USB Host driver.
 *           @defgroup IP_group_usbhost_external_typedef 3.Typedef
 *               none. No extra Typedef in MTK USB Host driver.
 *           @defgroup IP_group_usbhost_external_enum 4.Enumeration
 *               none. No extra Enumeration in MTK USB Host driver.
 *           @defgroup IP_group_usbhost_external_def 5.Define
 *               none. No extra Define in MTK USB Host driver.
 *         @}
 *
 *       @defgroup IP_group_usbhost_internal INTERNAL
 *         The internal API document for MTK USB Host. \n
 *
 *         @{
 *           @defgroup IP_group_usbhost_internal_function 1.function
 *               Internal function for MTK USB Host initial.
 *           @defgroup IP_group_usbhost_internal_struct 2.Structure
 *               Internal structure used for MTK USB Host driver.
 *           @defgroup IP_group_usbhost_internal_typedef 3.Typedef
 *               none. No extra Typedef in MTK USB Host driver.
 *           @defgroup IP_group_usbhost_internal_enum 4.Enumeration
 *               Internal enumeration used for MTK USB Host driver.
 *           @defgroup IP_group_usbhost_internal_def 5.Define
 *               Internal define used for MTK USB Host driver.
 *         @}
 *    @}
 */


#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>

#include "xhci.h"
#include "xhci-mtk.h"

#ifdef CONFIG_USB_FPGA
#include <linux/i2c.h>
#endif


 /** @ingroup IP_group_usbhost_internal_enum
 * @brief for MTK usb HCD Wakeup state
 *
 */
enum ssusb_wakeup_src {
	SSUSB_WK_IP_SLEEP = 1,
	SSUSB_WK_LINE_STATE = 2,
};

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     MTK usb HCD enable setting.
 * @param[in]
 *     mtk: structure of mtk hcd for getting information.
 * @return
 *     0, function success.\n
 *     error code from readl_poll_timeout().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If readl_poll_timeout() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int xhci_mtk_host_enable(struct xhci_hcd_mtk *mtk)
{
	struct mu3c_ippc_regs __iomem *ippc = mtk->ippc_regs;
	u32 value, check_val;
	int ret = 0;
	int i;

	if (!mtk->has_ippc)
		return 0;

	/* power on host ip */
	value = readl(&ippc->ip_pw_ctr1);
	value &= ~CTRL1_IP_HOST_PDN;
	writel(value, &ippc->ip_pw_ctr1);

	/* power on and enable all u3 ports */
	for (i = 0; i < mtk->num_u3_ports; i++) {
		value = readl(&ippc->u3_ctrl_p[i]);
		value &= ~(CTRL_U3_PORT_PDN | CTRL_U3_PORT_DIS);
		value |= CTRL_U3_PORT_HOST_SEL;
		writel(value, &ippc->u3_ctrl_p[i]);
	}

	/* power on and enable all u2 ports */
	for (i = 0; i < mtk->num_u2_ports; i++) {
		value = readl(&ippc->u2_ctrl_p[i]);
		value &= ~(CTRL_U2_PORT_PDN | CTRL_U2_PORT_DIS);
		value |= CTRL_U2_PORT_HOST_SEL;
		writel(value, &ippc->u2_ctrl_p[i]);
	}

	/*
	 * wait for clocks to be stable, and clock domains reset to
	 * be inactive after power on and enable ports
	 */
	check_val = STS1_SYSPLL_STABLE | STS1_REF_RST |
			STS1_SYS125_RST | STS1_XHCI_RST;

	ret = readl_poll_timeout(&ippc->ip_pw_sts1, value,
			  (check_val == (value & check_val)), 100, 20000);
	if (ret) {
		dev_err(mtk->dev, "clocks are not stable (0x%x)\n", value);
		return ret;
	}

	return 0;
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     MTK usb HCD disable setting.
 * @param[in]
 *     mtk: structure of mtk hcd for getting information.
 * @return
 *     0, function success.\n
 *     error code from readl_poll_timeout().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If readl_poll_timeout() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int xhci_mtk_host_disable(struct xhci_hcd_mtk *mtk)
{
	struct mu3c_ippc_regs __iomem *ippc = mtk->ippc_regs;
	u32 value;
	int ret;
	int i;

	if (!mtk->has_ippc)
		return 0;

	/* power down all u3 ports */
	for (i = 0; i < mtk->num_u3_ports; i++) {
		value = readl(&ippc->u3_ctrl_p[i]);
		value |= CTRL_U3_PORT_PDN;
		writel(value, &ippc->u3_ctrl_p[i]);
	}

	/* power down all u2 ports */
	for (i = 0; i < mtk->num_u2_ports; i++) {
		value = readl(&ippc->u2_ctrl_p[i]);
		value |= CTRL_U2_PORT_PDN;
		writel(value, &ippc->u2_ctrl_p[i]);
	}

	/* power down host ip */
	value = readl(&ippc->ip_pw_ctr1);
	value |= CTRL1_IP_HOST_PDN;
	writel(value, &ippc->ip_pw_ctr1);

	/* wait for host ip to sleep */
	ret = readl_poll_timeout(&ippc->ip_pw_sts1, value,
			  (value & STS1_IP_SLEEP_STS), 100, 100000);
	if (ret) {
		dev_err(mtk->dev, "ip sleep failed!!!\n");
		return ret;
	}
	return 0;
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     MTK usb HCD configuration.
 * @param[in]
 *     mtk: structure of mtk hcd for getting information.
 * @return
 *     0, function success.\n
 *     error code from xhci_mtk_host_enable().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If readl_poll_timeout() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int xhci_mtk_ssusb_config(struct xhci_hcd_mtk *mtk)
{
	struct mu3c_ippc_regs __iomem *ippc = mtk->ippc_regs;
	u32 value;

	if (!mtk->has_ippc)
		return 0;

	/* reset whole ip */
	value = readl(&ippc->ip_pw_ctr0);
	value |= CTRL0_IP_SW_RST;
	writel(value, &ippc->ip_pw_ctr0);
	udelay(1);
	value = readl(&ippc->ip_pw_ctr0);
	value &= ~CTRL0_IP_SW_RST;
	writel(value, &ippc->ip_pw_ctr0);

	/*
	 * device ip is default power-on in fact
	 * power down device ip, otherwise ip-sleep will fail
	 */
	value = readl(&ippc->ip_pw_ctr2);
	value |= CTRL2_IP_DEV_PDN;
	writel(value, &ippc->ip_pw_ctr2);

	value = readl(&ippc->ip_xhci_cap);
	mtk->num_u3_ports = CAP_U3_PORT_NUM(value);
	mtk->num_u2_ports = CAP_U2_PORT_NUM(value);
	dev_dbg(mtk->dev, "%s u2p:%d, u3p:%d\n", __func__,
			mtk->num_u2_ports, mtk->num_u3_ports);

	return xhci_mtk_host_enable(mtk);
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     MTK usb HCD system and ref clock enable.
 * @param[in]
 *     mtk: structure of mtk hcd for getting information.
 * @return
 *     0, function success.\n
 *     -EINVAL, function fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If function fail return -EINVAL error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */

static int xhci_mtk_clks_get(struct xhci_hcd_mtk *mtk)
{
	struct device *dev = mtk->dev;

	mtk->sys_clk = devm_clk_get(dev, "sys_ck");
	if (IS_ERR(mtk->sys_clk)) {
		dev_err(dev, "fail to get sys_ck\n");
		return PTR_ERR(mtk->sys_clk);
	}

	mtk->xhci_clk = devm_clk_get(dev, "xhci_ck");
	if (IS_ERR(mtk->xhci_clk))
		return PTR_ERR(mtk->xhci_clk);

	mtk->ref_clk = devm_clk_get(dev, "ref_ck");
	if (IS_ERR(mtk->ref_clk))
		return PTR_ERR(mtk->ref_clk);

	mtk->mcu_clk = devm_clk_get(dev, "mcu_ck");
	if (IS_ERR(mtk->mcu_clk))
		return PTR_ERR(mtk->mcu_clk);

	mtk->dma_clk = devm_clk_get(dev, "dma_ck");
	return PTR_ERR_OR_ZERO(mtk->dma_clk);

	return 0;
}

static int xhci_mtk_clks_enable(struct xhci_hcd_mtk *mtk)
{
	int ret;

	ret = clk_prepare_enable(mtk->ref_clk);
	if (ret) {
		dev_err(mtk->dev, "failed to enable ref_clk\n");
		goto ref_clk_err;
	}

	ret = clk_prepare_enable(mtk->sys_clk);
	if (ret) {
		dev_err(mtk->dev, "failed to enable sys_clk\n");
		goto sys_clk_err;
	}

	ret = clk_prepare_enable(mtk->xhci_clk);
	if (ret) {
		dev_err(mtk->dev, "failed to enable xhci_clk\n");
		goto xhci_clk_err;
	}

	ret = clk_prepare_enable(mtk->mcu_clk);
	if (ret) {
		dev_err(mtk->dev, "failed to enable mcu_clk\n");
		goto mcu_clk_err;
	}

	ret = clk_prepare_enable(mtk->dma_clk);
	if (ret) {
		dev_err(mtk->dev, "failed to enable dma_clk\n");
		goto dma_clk_err;
	}

	if (mtk->wakeup_src) {
		ret = clk_prepare_enable(mtk->wk_deb_p0);
		if (ret) {
			dev_err(mtk->dev, "failed to enable wk_deb_p0\n");
			goto usb_p0_err;
		}

		ret = clk_prepare_enable(mtk->wk_deb_p1);
		if (ret) {
			dev_err(mtk->dev, "failed to enable wk_deb_p1\n");
			goto usb_p1_err;
		}
	}
	return 0;

usb_p1_err:
	clk_disable_unprepare(mtk->wk_deb_p0);
usb_p0_err:
	clk_disable_unprepare(mtk->dma_clk);
dma_clk_err:
	clk_disable_unprepare(mtk->mcu_clk);
mcu_clk_err:
	clk_disable_unprepare(mtk->xhci_clk);
xhci_clk_err:
	clk_disable_unprepare(mtk->sys_clk);
sys_clk_err:
	clk_disable_unprepare(mtk->ref_clk);
ref_clk_err:
	return -EINVAL;
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     MTK usb HCD system and ref clock disable.
 * @param[in]
 *     mtk: structure of mtk hcd for getting information.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void xhci_mtk_clks_disable(struct xhci_hcd_mtk *mtk)
{
	if (mtk->wakeup_src) {
		clk_disable_unprepare(mtk->wk_deb_p1);
		clk_disable_unprepare(mtk->wk_deb_p0);
	}
	clk_disable_unprepare(mtk->dma_clk);
	clk_disable_unprepare(mtk->mcu_clk);
	clk_disable_unprepare(mtk->xhci_clk);
	clk_disable_unprepare(mtk->sys_clk);
	clk_disable_unprepare(mtk->ref_clk);
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     MTK usb HCD sleep wakeup mode enable.\n
 *     only clocks can be turn off for MTK hcd sleep wakeup mode
 * @param[in]
 *     mtk: structure of mtk hcd for getting information.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void usb_wakeup_ip_sleep_en(struct xhci_hcd_mtk *mtk)
{
	u32 tmp;
	struct regmap *pericfg = mtk->pericfg;

	regmap_read(pericfg, PERI_WK_CTRL1, &tmp);
	tmp &= ~UWK_CTL1_IS_P;
	tmp &= ~(UWK_CTL1_IS_C(0xf));
	tmp |= UWK_CTL1_IS_C(0x8);
	regmap_write(pericfg, PERI_WK_CTRL1, tmp);
	regmap_write(pericfg, PERI_WK_CTRL1, tmp | UWK_CTL1_IS_E);

	regmap_read(pericfg, PERI_WK_CTRL1, &tmp);
	dev_dbg(mtk->dev, "%s(): WK_CTRL1[P6,E25,C26:29]=%#x\n",
		__func__, tmp);
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     MTK usb HCD sleep wakeup mode disable.
 * @param[in]
 *     mtk: structure of mtk hcd for getting information.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void usb_wakeup_ip_sleep_dis(struct xhci_hcd_mtk *mtk)
{
	u32 tmp;

	regmap_read(mtk->pericfg, PERI_WK_CTRL1, &tmp);
	tmp &= ~UWK_CTL1_IS_E;
	regmap_write(mtk->pericfg, PERI_WK_CTRL1, tmp);
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     MTK usb HCD for line-state wakeup mode enable.\n
 *     phy's power should not power-down\n
 *     and only support cable plug in/out
 * @param[in]
 *     mtk: structure of mtk hcd for getting information.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void usb_wakeup_line_state_en(struct xhci_hcd_mtk *mtk)
{
	u32 tmp;
	struct regmap *pericfg = mtk->pericfg;

	/* line-state of u2-port0 */
	regmap_read(pericfg, PERI_WK_CTRL1, &tmp);
	tmp &= ~UWK_CTL1_0P_LS_P;
	tmp &= ~(UWK_CTL1_0P_LS_C(0xf));
	tmp |= UWK_CTL1_0P_LS_C(0x8);
	regmap_write(pericfg, PERI_WK_CTRL1, tmp);
	regmap_read(pericfg, PERI_WK_CTRL1, &tmp);
	regmap_write(pericfg, PERI_WK_CTRL1, tmp | UWK_CTL1_0P_LS_E);

	/* line-state of u2-port1 */
	regmap_read(pericfg, PERI_WK_CTRL0, &tmp);
	tmp &= ~(UWK_CTL1_1P_LS_C(0xf));
	tmp |= UWK_CTL1_1P_LS_C(0x8);
	regmap_write(pericfg, PERI_WK_CTRL0, tmp);
	regmap_write(pericfg, PERI_WK_CTRL0, tmp | UWK_CTL1_1P_LS_E);
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     MTK usb HCD for line-state wakeup mode disable.
 * @param[in]
 *     mtk: structure of mtk hcd for getting information.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void usb_wakeup_line_state_dis(struct xhci_hcd_mtk *mtk)
{
	u32 tmp;
	struct regmap *pericfg = mtk->pericfg;

	/* line-state of u2-port0 */
	regmap_read(pericfg, PERI_WK_CTRL1, &tmp);
	tmp &= ~UWK_CTL1_0P_LS_E;
	regmap_write(pericfg, PERI_WK_CTRL1, tmp);

	/* line-state of u2-port1 */
	regmap_read(pericfg, PERI_WK_CTRL0, &tmp);
	tmp &= ~UWK_CTL1_1P_LS_E;
	regmap_write(pericfg, PERI_WK_CTRL0, tmp);
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     MTK usb HCD wakeup enable function.
 * @param[in]
 *     mtk: structure of mtk hcd for getting information.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void usb_wakeup_enable(struct xhci_hcd_mtk *mtk)
{
	if (mtk->wakeup_src == SSUSB_WK_IP_SLEEP)
		usb_wakeup_ip_sleep_en(mtk);
	else if (mtk->wakeup_src == SSUSB_WK_LINE_STATE)
		usb_wakeup_line_state_en(mtk);
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     MTK usb HCD wakeup disable.
 * @param[in]
 *     mtk: structure of mtk hcd for getting information.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void usb_wakeup_disable(struct xhci_hcd_mtk *mtk)
{
	if (mtk->wakeup_src == SSUSB_WK_IP_SLEEP)
		usb_wakeup_ip_sleep_dis(mtk);
	else if (mtk->wakeup_src == SSUSB_WK_LINE_STATE)
		usb_wakeup_line_state_dis(mtk);
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     Parse usb HOST device tree information about wakeup option value.
 * @param[in]
 *     mtk: structure of mtk hcd for getting information.\n
 *      dn: device_node struct pointer.
 * @return
 *      0, function success.\n
 *      error code from devm_clk_get().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If devm_clk_get() fail, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int usb_wakeup_of_property_parse(struct xhci_hcd_mtk *mtk,
				struct device_node *dn)
{
	struct device *dev = mtk->dev;

	/*
	* wakeup function is optional, so it is not an error if this property
	* does not exist, and in such case, no need to get relative
	* properties anymore.
	*/
	of_property_read_u32(dn, "mediatek,wakeup-src", &mtk->wakeup_src);
	if (!mtk->wakeup_src)
		return 0;

	mtk->wk_deb_p0 = devm_clk_get(dev, "wakeup_deb_p0");
	if (IS_ERR(mtk->wk_deb_p0)) {
		dev_err(dev, "fail to get wakeup_deb_p0\n");
		return PTR_ERR(mtk->wk_deb_p0);
	}

	mtk->wk_deb_p1 = devm_clk_get(dev, "wakeup_deb_p1");
	if (IS_ERR(mtk->wk_deb_p1)) {
		dev_err(dev, "fail to get wakeup_deb_p1\n");
		return PTR_ERR(mtk->wk_deb_p1);
	}

	mtk->pericfg = syscon_regmap_lookup_by_phandle(dn,
						"mediatek,syscon-wakeup");
	if (IS_ERR(mtk->pericfg)) {
		dev_err(dev, "fail to get pericfg regs\n");
		return PTR_ERR(mtk->pericfg);
	}

	return 0;
}

static int xhci_mtk_setup(struct usb_hcd *hcd);
static const struct xhci_driver_overrides xhci_mtk_overrides __initconst = {
	.extra_priv_size = sizeof(struct xhci_hcd),
	.reset = xhci_mtk_setup,
};

static struct hc_driver __read_mostly xhci_mtk_hc_driver;

#ifdef CONFIG_USB_FPGA
#define I2C_W 0
#define I2C_R 1
#define READ_BACK_VRIFY
#endif
/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     MTK usb HOST phy initialization.
 * @param[in]
 *     mtk: structure of mtk hcd for getting information.
 * @return
 *     0, function success.\n
 *     error code from phy_init().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If phy_init() fail, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int xhci_mtk_phy_init(struct xhci_hcd_mtk *mtk)
{
	int i;
	int ret;
	struct mu3c_ippc_regs __iomem *ippc = mtk->ippc_regs;
	u32 value;

#ifdef CONFIG_USB_FPGA
	u32 speed_timing = 0;
	struct i2c_msg msgs_test[3];
	struct i2c_adapter *adap;

	unsigned char write_phyinit[46] = {
		0xff, 0x00, 0x15, 0x40, 0x6a, 0x00, 0x1a, 0x10,	/*i=0,2,4,6 */
		0xff, 0x10, 0x0a, 0x84,	/*i=8,10 */
		0xff, 0x30, 0x03, 0x44, 0x06, 0x20, 0x07, 0x62,	/*i=12~18 */
		0xff, 0x40, 0x38, 0x47, 0x39, 0x00, 0x42, 0x43, 0x43, 0x00,
		0x08, 0x47, 0x09, 0x08, 0x0c, 0xd1, 0x0e, 0x0c, 0x10, 0xc1,
		0x14, 0x2f,/*i=20~40 */
		0xff, 0x60, 0x17, 0x08,	/*i=42,44 */
	};/*for slave addr=0x60 , phy initial setting*/

	unsigned char write_ckphase[6] = { 0xFF, 0x70, 0x9C, 0x05, 0x9D, 0x03 };
	/*for slave addr=0x70 , 0x9c[4:0][9:5] ckphase setting */

	adap = i2c_get_adapter(5);	/*get i2c id=5 adapter struct */

	if (adap == NULL) {
		dev_err(mtk->dev,
			"Can't get USB phy i2c adapter channel ......\n");
		return 0;
	}

	speed_timing = 400000;
	/********** set slave addr=0x60 initial value to phy board *********/
	dev_dbg(mtk->dev, "%s , i2c setting phy initial value ......\n",
		__func__);

	for (i = 0; i < 46; i += 2) {
		msgs_test[0].addr = 0x60;
		msgs_test[0].flags = I2C_W;
		msgs_test[0].len = 2;
		msgs_test[0].buf = write_phyinit + i;
		ret = i2c_transfer(adap, msgs_test, 1);
	}

#ifdef READ_BACK_VRIFY
	/*0. switch bank */
	i = 0;
	msgs_test[0].addr = 0x60;
	msgs_test[0].flags = I2C_W;
	msgs_test[0].len = 2;
	msgs_test[0].buf = write_phyinit + i;
	ret = i2c_transfer(adap, msgs_test, 1);

	for (i = 2; i <= 6; i += 2) {
		/*1. set read address */
		msgs_test[0].addr = 0x60;
		msgs_test[0].flags = I2C_W;
		msgs_test[0].len = 1;
		msgs_test[0].buf = write_phyinit + i;
		ret = i2c_transfer(adap, msgs_test, 1);
		/*2. read back data */
		msgs_test[0].addr = 0x60;
		msgs_test[0].flags = I2C_R;
		msgs_test[0].len = 1;
		ret = i2c_transfer(adap, msgs_test, 1);
		if (write_phyinit[i + 1] != *msgs_test[0].buf)
			dev_err(mtk->dev,
				"i2c verify fail addr=%x,Wdata=%x , Rdate=%x\n",
				write_phyinit[i], write_phyinit[i + 1],
				*msgs_test[0].buf);
	}

	/*0. switch bank */
	i = 8;
	msgs_test[0].addr = 0x60;
	msgs_test[0].flags = I2C_W;
	msgs_test[0].len = 2;
	msgs_test[0].buf = write_phyinit + i;
	ret = i2c_transfer(adap, msgs_test, 1);

	for (i = 10; i <= 10; i += 2) {
		/*1. set read address */
		msgs_test[0].addr = 0x60;
		msgs_test[0].flags = I2C_W;
		msgs_test[0].len = 1;
		msgs_test[0].buf = write_phyinit + i;
		ret = i2c_transfer(adap, msgs_test, 1);
		/*2. read back data */
		msgs_test[0].addr = 0x60;
		msgs_test[0].flags = I2C_R;
		msgs_test[0].len = 1;
		ret = i2c_transfer(adap, msgs_test, 1);
		if (write_phyinit[i + 1] != *msgs_test[0].buf)
			dev_err(mtk->dev,
				"i2c verify fail addr=%x,Wdata=%x , Rdate=%x\n",
				write_phyinit[i], write_phyinit[i + 1],
				*msgs_test[0].buf);
	}

	/*0. switch bank */
	i = 12;
	msgs_test[0].addr = 0x60;
	msgs_test[0].flags = I2C_W;
	msgs_test[0].len = 2;
	msgs_test[0].buf = write_phyinit + i;
	ret = i2c_transfer(adap, msgs_test, 1);

	for (i = 14; i <= 18; i += 2) {
		/*1. set read address */
		msgs_test[0].addr = 0x60;
		msgs_test[0].flags = I2C_W;
		msgs_test[0].len = 1;
		msgs_test[0].buf = write_phyinit + i;
		ret = i2c_transfer(adap, msgs_test, 1);
		/*2. read back data */
		msgs_test[0].addr = 0x60;
		msgs_test[0].flags = I2C_R;
		msgs_test[0].len = 1;
		ret = i2c_transfer(adap, msgs_test, 1);
		if (write_phyinit[i + 1] != *msgs_test[0].buf)
			dev_err(mtk->dev,
				"i2c verify fail addr=%x,Wdata=%x , Rdate=%x\n",
				write_phyinit[i], write_phyinit[i + 1],
				*msgs_test[0].buf);
	}

	/*0. switch bank */
	i = 20;
	msgs_test[0].addr = 0x60;
	msgs_test[0].flags = I2C_W;
	msgs_test[0].len = 2;
	msgs_test[0].buf = write_phyinit + i;
	ret = i2c_transfer(adap, msgs_test, 1);

	for (i = 22; i <= 40; i += 2) {
		/*1. set read address */
		msgs_test[0].addr = 0x60;
		msgs_test[0].flags = I2C_W;
		msgs_test[0].len = 1;
		msgs_test[0].buf = write_phyinit + i;
		ret = i2c_transfer(adap, msgs_test, 1);
		/*2. read back data */
		msgs_test[0].addr = 0x60;
		msgs_test[0].flags = I2C_R;
		msgs_test[0].len = 1;
		ret = i2c_transfer(adap, msgs_test, 1);
		if (write_phyinit[i + 1] != *msgs_test[0].buf)
			dev_err(mtk->dev,
				"i2c verify fail addr=%x,Wdata=%x , Rdate=%x\n",
				write_phyinit[i], write_phyinit[i + 1],
				*msgs_test[0].buf);
	}

	/*0. switch bank */
	i = 42;
	msgs_test[0].addr = 0x60;
	msgs_test[0].flags = I2C_W;
	msgs_test[0].len = 2;
	msgs_test[0].buf = write_phyinit + i;
	ret = i2c_transfer(adap, msgs_test, 1);

	for (i = 44; i <= 44; i += 2) {
		/*1. set read address */
		msgs_test[0].addr = 0x60;
		msgs_test[0].flags = I2C_W;
		msgs_test[0].len = 1;
		msgs_test[0].buf = write_phyinit + i;
		ret = i2c_transfer(adap, msgs_test, 1);
		/*2. read back data */
		msgs_test[0].addr = 0x60;
		msgs_test[0].flags = I2C_R;
		msgs_test[0].len = 1;
		ret = i2c_transfer(adap, msgs_test, 1);
		if (write_phyinit[i + 1] != *msgs_test[0].buf)
			dev_err(mtk->dev,
				"i2c verify fail addr=%x,Wdata=%x , Rdate=%x\n",
				write_phyinit[i], write_phyinit[i + 1],
				*msgs_test[0].buf);
	}

#endif

	/********** set slave addr=0x70 ckphase value to phy board *********/
	dev_dbg(mtk->dev, "%s , i2c setting phy ckphase value ......\n",
		__func__);

	for (i = 0; i < 6; i += 2) {
		msgs_test[0].addr = 0x70;
		msgs_test[0].flags = I2C_W;
		msgs_test[0].len = 2;
		msgs_test[0].buf = write_ckphase + i;
		ret = i2c_transfer(adap, msgs_test, 1);
	}

#ifdef READ_BACK_VRIFY
	/*0. switch bank */
	i = 0;
	msgs_test[0].addr = 0x70;
	msgs_test[0].flags = I2C_W;
	msgs_test[0].len = 2;
	msgs_test[0].buf = write_ckphase + i;
	ret = i2c_transfer(adap, msgs_test, 1);

	for (i = 2; i <= 4; i += 2) {
		/*1. set read address */
		msgs_test[0].addr = 0x70;
		msgs_test[0].flags = I2C_W;
		msgs_test[0].len = 1;
		msgs_test[0].buf = write_ckphase + i;
		ret = i2c_transfer(adap, msgs_test, 1);
		/*2. read back data */
		msgs_test[0].addr = 0x70;
		msgs_test[0].flags = I2C_R;
		msgs_test[0].len = 1;
		ret = i2c_transfer(adap, msgs_test, 1);
		if (write_ckphase[i + 1] != *msgs_test[0].buf)
			dev_err(mtk->dev,
				"i2c verify fail addr=%x,Wdata=%x , Rdate=%x\n",
				write_ckphase[i], write_ckphase[i + 1],
				*msgs_test[0].buf);
	}

#endif

	dev_dbg(mtk->dev, "%s , i2c setting finished .........\n", __func__);

#endif
	for (i = 0; i < mtk->num_phys; i++) {
		ret = phy_init(mtk->phys[i]);
		if (ret) {
			/* ssusb ref_ck gating enable */
			value = readl(&ippc->ip_pw_ctr0);
			value |= REF_CK_GATE_EN;
			writel(value, &ippc->ip_pw_ctr0);
			goto exit_phy;
		}
	}
	return 0;

exit_phy:
	for (; i > 0; i--)
		phy_exit(mtk->phys[i - 1]);

	return ret;
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     MTK usb HOST phy exit.
 * @param[in]
 *     mtk: structure of mtk hcd for getting information.
 * @return
 *     0, always return function success.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int xhci_mtk_phy_exit(struct xhci_hcd_mtk *mtk)
{
	int i;

	for (i = 0; i < mtk->num_phys; i++)
		phy_exit(mtk->phys[i]);

	return 0;
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     MTK usb HOST phy power on.
 * @param[in]
 *     mtk: structure of mtk hcd for getting information.
 * @return
 *     0, function success.\n
 *     error code from phy_power_on().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If phy_power_on() fail, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int xhci_mtk_phy_power_on(struct xhci_hcd_mtk *mtk)
{
	int i;
	int ret;

	for (i = 0; i < mtk->num_phys; i++) {
		ret = phy_power_on(mtk->phys[i]);
		if (ret)
			goto power_off_phy;
	}

	return 0;

power_off_phy:

	for (; i > 0; i--)
		phy_power_off(mtk->phys[i - 1]);

	return ret;
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     MTK usb HOST phy power off.
 * @param[in]
 *     mtk: structure of mtk hcd for getting information.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void xhci_mtk_phy_power_off(struct xhci_hcd_mtk *mtk)
{
	unsigned int i;

	for (i = 0; i < mtk->num_phys; i++)
		phy_power_off(mtk->phys[i]);
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     MTK usb HOST LDO regulator enable.
 * @param[in]
 *     mtk: structure of mtk hcd for getting information.
 * @return
 *     0, function success.\n
 *     error code from regulator_enable().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If regulator_enable fail, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int xhci_mtk_ldos_enable(struct xhci_hcd_mtk *mtk)
{
	int ret;

	ret = regulator_enable(mtk->vbus);
	if (ret) {
		dev_err(mtk->dev, "failed to enable vbus\n");
		return ret;
	}

	ret = regulator_enable(mtk->vusb33);
	if (ret) {
		dev_err(mtk->dev, "failed to enable vusb33\n");
		regulator_disable(mtk->vbus);
		return ret;
	}
	return 0;
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     MTK usb HOST LDO disable.
 * @param[in]
 *     mtk: structure of mtk hcd for getting information.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void xhci_mtk_ldos_disable(struct xhci_hcd_mtk *mtk)
{
	regulator_disable(mtk->vbus);
	regulator_disable(mtk->vusb33);
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     Set xhci quirks flag for MTK USB Host.
 * @param[in]
 *      dev: device struct pointer.\n
 * @param[in]
 *      mtk: xhci_hcd_mtk struct pointer.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void xhci_mtk_quirks(struct device *dev, struct xhci_hcd *xhci)
{
	struct usb_hcd *hcd = xhci_to_hcd(xhci);
	struct xhci_hcd_mtk *mtk = hcd_to_mtk(hcd);

	/*
	 * As of now platform drivers don't provide MSI support so we ensure
	 * here that the generic code does not try to make a pci_dev from our
	 * dev struct in order to setup MSI
	 */
	xhci->quirks |= XHCI_PLAT;
	xhci->quirks |= XHCI_MTK_HOST;
	/*
	 * MTK host controller gives a spurious successful event after a
	 * short transfer. Ignore it.
	 */
	xhci->quirks |= XHCI_SPURIOUS_SUCCESS;
	if (mtk->lpm_support)
		xhci->quirks |= XHCI_LPM_SUPPORT;
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     MTK usb HOST setup.\n
 *     called by during probe() after chip reset completes.
 * @param[in]
 *      hcd: usb_hcd struct for getting information.\n
 * @return
 *      0, function success.\n
 *      return value from xhci_mtk_ssusb_config().\n
 *      return value from xhci_gen_setup().\n
 *      return value from xhci_mtk_sch_init().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If xhci_mtk_ssusb_config() fail, return its error code.\n
 *     2. If xhci_gen_setup() fail, return its error code.\n
 *     3. If xhci_mtk_sch_init() fail, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int xhci_mtk_setup(struct usb_hcd *hcd)
{
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);
	struct xhci_hcd_mtk *mtk = hcd_to_mtk(hcd);
	int ret;

	if (usb_hcd_is_primary_hcd(hcd)) {
		ret = xhci_mtk_ssusb_config(mtk);
		if (ret)
			return ret;
	}

	ret = xhci_gen_setup(hcd, xhci_mtk_quirks);
	if (ret)
		return ret;

	if (usb_hcd_is_primary_hcd(hcd)) {
		mtk->num_u3_ports = xhci->num_usb3_ports;
		mtk->num_u2_ports = xhci->num_usb2_ports;
		ret = xhci_mtk_sch_init(mtk);
		if (ret)
			return ret;
	}

	return ret;
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     MTK usb HOST probe.\n
 * @param[in]
 *     pdev: platform_device struct for getting information.\n
 * @return
 *     0, function success.\n
 *     -ENODEV, usb disabled.\n
 *     -ENOMEM, memory alloc fail.\n
 *     error code from devm_regulator_get().\n
 *     error code from devm_clk_get().\n
 *     error code from usb_wakeup_of_property_parse().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If usb disabled, return -ENODEV.\n
 *     2. If devm_regulator_get() fail, return its error code.\n
 *     3. If devm_clk_get() fail, return its error code.\n
 *     4. If usb_wakeup_of_property_parse() fail, return its error code.\n
 *     5. If memory alloc fail, return -ENOMEM.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int xhci_mtk_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct xhci_hcd_mtk *mtk;
	const struct hc_driver *driver;
	struct xhci_hcd *xhci;
	struct resource *res;
	struct usb_hcd *hcd;
	struct phy *phy;
	int phy_num;
	int ret = -ENODEV;
	int irq;

	if (usb_disabled())
		return -ENODEV;

	driver = &xhci_mtk_hc_driver;
	mtk = devm_kzalloc(dev, sizeof(*mtk), GFP_KERNEL);
	if (!mtk)
		return -ENOMEM;

	dev_dbg(dev, "xhci_mtk_probe start .....................\n");

	dev_dbg(dev, "xhci_mtk_probe dev_name:%s\n", dev_name(dev));



	mtk->dev = dev;
	mtk->vbus = devm_regulator_get(dev, "vbus");
	if (IS_ERR(mtk->vbus)) {
		dev_err(dev, "fail to get vbus\n");
		return PTR_ERR(mtk->vbus);
	}

	mtk->vusb33 = devm_regulator_get(dev, "vusb33");
	if (IS_ERR(mtk->vusb33)) {
		dev_err(dev, "fail to get vusb33\n");
		return PTR_ERR(mtk->vusb33);
	}

	ret = xhci_mtk_clks_get(mtk);
	if (ret)
		return ret;

	mtk->lpm_support = of_property_read_bool(node, "usb3-lpm-capable");

	ret = usb_wakeup_of_property_parse(mtk, node);
	if (ret)
		return ret;

	mtk->num_phys = of_count_phandle_with_args(node,
			"phys", "#phy-cells");
	if (mtk->num_phys > 0) {
		mtk->phys = devm_kcalloc(dev, mtk->num_phys,
					sizeof(*mtk->phys), GFP_KERNEL);
		if (!mtk->phys)
			return -ENOMEM;
	} else {
		mtk->num_phys = 0;
	}

	#ifdef CONFIG_USB_FPGA
		mtk->num_phys = 0;
	#endif

	pm_runtime_enable(dev);
	pm_runtime_get_sync(dev);
	device_enable_async_suspend(dev);

	ret = xhci_mtk_ldos_enable(mtk);
	if (ret)
		goto disable_pm;

	ret = xhci_mtk_clks_enable(mtk);
	if (ret)
		goto disable_ldos;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		ret = irq;
		goto disable_clk;
	}
	dev_dbg(dev, "xhci_mtk_probe-platform_get_irq=irq: %d\n", irq);
	/* Initialize dma_mask and coherent_dma_mask to 32-bits */
	ret = dma_set_coherent_mask(dev, DMA_BIT_MASK(32));
	if (ret)
		goto disable_clk;

	if (!dev->dma_mask)
		dev->dma_mask = &dev->coherent_dma_mask;
	else
		dma_set_mask(dev, DMA_BIT_MASK(32));

	hcd = usb_create_hcd(driver, dev, dev_name(dev));
	if (!hcd) {
		ret = -ENOMEM;
		goto disable_clk;
	}

	/*
	 * USB 2.0 roothub is stored in the platform_device.
	 * Swap it with mtk HCD.
	 */
	mtk->hcd = platform_get_drvdata(pdev);
	platform_set_drvdata(pdev, mtk);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "mac");
	hcd->regs = devm_ioremap_resource(dev, res);
	if (IS_ERR(hcd->regs)) {
		ret = PTR_ERR(hcd->regs);
		goto put_usb2_hcd;
	}
	hcd->rsrc_start = res->start;
	hcd->rsrc_len = resource_size(res);

	mtk->mac_regs = hcd->regs + SSUSB_USB3_MAC_CSR_P0;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ippc");
	if (res) {	/* ippc register is optional */
		mtk->ippc_regs = devm_ioremap_resource(dev, res);
		if (IS_ERR(mtk->ippc_regs)) {
			ret = PTR_ERR(mtk->ippc_regs);
			goto put_usb2_hcd;
		}
		mtk->has_ippc = true;
	} else {
		mtk->has_ippc = false;
	}

	for (phy_num = 0; phy_num < mtk->num_phys; phy_num++) {
		phy = devm_of_phy_get_by_index(dev, node, phy_num);
		if (IS_ERR(phy)) {
			ret = PTR_ERR(phy);
			goto put_usb2_hcd;
		}
		mtk->phys[phy_num] = phy;
	}

	ret = xhci_mtk_phy_init(mtk);
	if (ret)
		goto put_usb2_hcd;

	ret = xhci_mtk_phy_power_on(mtk);
	if (ret)
		goto exit_phys;

	device_init_wakeup(dev, true);

	xhci = hcd_to_xhci(hcd);
	xhci->main_hcd = hcd;
	xhci->shared_hcd = usb_create_shared_hcd(driver, dev,
			dev_name(dev), hcd);
	if (!xhci->shared_hcd) {
		ret = -ENOMEM;
		goto power_off_phys;
	}

	ret = usb_add_hcd(hcd, irq, IRQF_SHARED);
	if (ret)
		goto put_usb3_hcd;

	if (HCC_MAX_PSA(xhci->hcc_params) >= 4)
		xhci->shared_hcd->can_do_streams = 1;

	ret = usb_add_hcd(xhci->shared_hcd, irq, IRQF_SHARED);
	if (ret)
		goto dealloc_usb2_hcd;

	xhci_dbg(xhci, "xhci_mtk_probe ending.....................\n");

	return 0;

dealloc_usb2_hcd:
	usb_remove_hcd(hcd);

put_usb3_hcd:
	xhci_mtk_sch_exit(mtk);
	usb_put_hcd(xhci->shared_hcd);

power_off_phys:
	xhci_mtk_phy_power_off(mtk);
	device_init_wakeup(dev, false);

exit_phys:
	xhci_mtk_phy_exit(mtk);

put_usb2_hcd:
	usb_put_hcd(hcd);

disable_clk:
	xhci_mtk_clks_disable(mtk);

disable_ldos:
	xhci_mtk_ldos_disable(mtk);

disable_pm:
	pm_runtime_put_sync(dev);
	pm_runtime_disable(dev);
	return ret;
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     MTK usb HOST remove.\n
 * @param[in]
 *     pdev: platform_device struct for getting information.\n
 * @return
 *     0, always return function success.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int xhci_mtk_remove(struct platform_device *dev)
{
	struct xhci_hcd_mtk *mtk = platform_get_drvdata(dev);
	struct usb_hcd	*hcd = mtk->hcd;
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);

	usb_remove_hcd(xhci->shared_hcd);
	xhci_mtk_phy_power_off(mtk);
	xhci_mtk_phy_exit(mtk);
	device_init_wakeup(&dev->dev, false);

	usb_remove_hcd(hcd);
	usb_put_hcd(xhci->shared_hcd);
	usb_put_hcd(hcd);
	xhci_mtk_sch_exit(mtk);
	xhci_mtk_clks_disable(mtk);
	xhci_mtk_ldos_disable(mtk);
	pm_runtime_put_sync(&dev->dev);
	pm_runtime_disable(&dev->dev);

	return 0;
}

 /** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     MTK usb HOST suspend function.\n
 * if ip sleep fails, and all clocks are disabled,access register will hang\n
 * AHB bus, so stop polling roothubs to avoid regs access on bus suspend.\n
 * and no need to check whether ip sleep failed or not; this will cause SPM\n
 * to wake up system immediately after system suspend complete if ip sleep\n
 * fails, it is what we wanted.
 * @param[in]
 *      dev: device struct pointer.\n
 * @return
 *     0, always return function success.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int __maybe_unused xhci_mtk_suspend(struct device *dev)
{
	struct xhci_hcd_mtk *mtk = dev_get_drvdata(dev);
	struct usb_hcd *hcd = mtk->hcd;
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);

	xhci_dbg(xhci, "%s: stop port polling\n", __func__);
	clear_bit(HCD_FLAG_POLL_RH, &hcd->flags);
	del_timer_sync(&hcd->rh_timer);
	clear_bit(HCD_FLAG_POLL_RH, &xhci->shared_hcd->flags);
	del_timer_sync(&xhci->shared_hcd->rh_timer);

	xhci_mtk_host_disable(mtk);
	xhci_mtk_phy_power_off(mtk);
	xhci_mtk_clks_disable(mtk);
	usb_wakeup_enable(mtk);
	return 0;
}

 /** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     MTK usb HOST resume.
 * @param[in]
 *      dev: device struct pointer.\n
 * @return
 *     0, always return function success.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int __maybe_unused xhci_mtk_resume(struct device *dev)
{
	struct xhci_hcd_mtk *mtk = dev_get_drvdata(dev);
	struct usb_hcd *hcd = mtk->hcd;
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);

	usb_wakeup_disable(mtk);
	xhci_mtk_clks_enable(mtk);
	xhci_mtk_phy_power_on(mtk);
	xhci_mtk_host_enable(mtk);

	xhci_dbg(xhci, "%s: restart port polling\n", __func__);
	set_bit(HCD_FLAG_POLL_RH, &hcd->flags);
	usb_hcd_poll_rh_status(hcd);
	set_bit(HCD_FLAG_POLL_RH, &xhci->shared_hcd->flags);
	usb_hcd_poll_rh_status(xhci->shared_hcd);
	return 0;
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
*  set mtk xhci HCD power on.
 * Only for system power measurtment usage, not resume function.
 * @param[in]
 *     dev: device struct pointer.\n
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void xhci_mtk_power_on(struct device *dev)
{
	struct xhci_hcd_mtk *mtk = dev_get_drvdata(dev);

	dev_dbg(mtk->dev, "%s\n", __func__);

	xhci_mtk_phy_power_on(mtk);
	xhci_mtk_host_enable(mtk);
}
EXPORT_SYMBOL_GPL(xhci_mtk_power_on);

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 * set mtk xhci HCD power off.
 * Only for system power measurtment usage, not suspand function.
 * @param[in]
 *     dev: device struct pointer.\n
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void xhci_mtk_power_off(struct device *dev)
{
	struct xhci_hcd_mtk *mtk = dev_get_drvdata(dev);

	dev_dbg(mtk->dev, "%s\n", __func__);

	xhci_mtk_host_disable(mtk);
	xhci_mtk_phy_power_off(mtk);
}
EXPORT_SYMBOL_GPL(xhci_mtk_power_off);

/** @ingroup IP_group_usbhost_internal_struct
* @brief MTK usb host support PM ops function struct.
*/
static const struct dev_pm_ops xhci_mtk_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(xhci_mtk_suspend, xhci_mtk_resume)
};

/** @ingroup IP_group_usbhost_internal_typedef
* @brief MTK usb host support PM ops function define.
*/
#define DEV_PM_OPS (IS_ENABLED(CONFIG_PM) ? &xhci_mtk_pm_ops : NULL)

#ifdef CONFIG_OF
/** @ingroup IP_group_usbhost_internal_struct
* @brief MTK usb Host DT match ID define.
*/
static const struct of_device_id mtk_xhci_of_match[] = {
	{.compatible = "mediatek,mt3612-xhci"},
	{},
};

MODULE_DEVICE_TABLE(of, mtk_xhci_of_match);
#endif

/** @ingroup IP_group_usbhost_internal_struct
* @brief MTK usb Host platform driver data structure define.
*/
static struct platform_driver mtk_xhci_driver = {
	.probe	= xhci_mtk_probe,
	.remove	= xhci_mtk_remove,
	.driver	= {
		.name = "xhci-mtk",
		.pm = NULL,
		.of_match_table = of_match_ptr(mtk_xhci_of_match),
	},
};
MODULE_ALIAS("platform:xhci-mtk");

 /** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     MTK usb HOST driver initialization.
 * @par Parameters
 *     none.
 * @return
 *     return from platform_driver_register().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If platform_driver_register() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int __init xhci_mtk_init(void)
{
	xhci_init_driver(&xhci_mtk_hc_driver, &xhci_mtk_overrides);
	return platform_driver_register(&mtk_xhci_driver);
}
module_init(xhci_mtk_init);

 /** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     MTK usb HOST driver exit.
 * @par Parameters
 *     none.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void __exit xhci_mtk_exit(void)
{
	platform_driver_unregister(&mtk_xhci_driver);
}
module_exit(xhci_mtk_exit);

MODULE_AUTHOR("Chunfeng Yun <chunfeng.yun@mediatek.com>");
MODULE_DESCRIPTION("MediaTek xHCI Host Controller Driver");
MODULE_LICENSE("GPL v2");
