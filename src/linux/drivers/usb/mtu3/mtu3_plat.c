/*
 * Copyright (C) 2016 MediaTek Inc.
 *
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
 * @defgroup IP_group_usb USB device Controller
 *   USB Device include USB 2.0 and USB 3.1 Gen 2 PHY and MAC \n
 *   that work separately for different transmission line.\n
 *   They share the same endpoint and buffer management unit since \n
 *   there will be only one link connected to USB host at the same time.\n
 *   The linked-list queue of SSUSB device is basically inherited from \n
 *   MTK Unified USB with similar descriptor definition.\n
 *   On SuperSpeed link, it works on Tx and Rx concurrently.\n
 *
 *   @{
 *       @defgroup IP_group_usb_external EXTERNAL
 *         The external API document for Serial NOR Flash Controller. \n
 *
 *         @{
 *             @defgroup IP_group_usb_external_function 1.function
 *               none. Native Gadget Driver.
 *             @defgroup IP_group_usb_external_struct 2.structure
 *               none. Native Gadget Driver.
 *             @defgroup IP_group_usb_external_typedef 3.typedef
 *               none. Native Gadget Driver.
 *             @defgroup IP_group_usb_external_enum 4.enumeration
 *               none. Native Gadget Driver.
 *             @defgroup IP_group_usb_external_def 5.define
 *               none. Native Gadget Driver.
 *         @}
 *
 *       @defgroup IP_group_usb_internal INTERNAL
 *         The internal API document for USB Controller. \n
 *
 *         @{
 *             @defgroup IP_group_usb_internal_function 1.function
 *               Internal function used for USB Controller.
 *             @defgroup IP_group_usb_internal_struct 2.structure
 *               Internal structure used for USB Controller.
 *             @defgroup IP_group_usb_internal_typedef 3.typedef
 *               Internal typedef used for USB Controller.
 *             @defgroup IP_group_usb_internal_enum 4.enumeration
 *               Internal enumeration used for USB Controller.
 *             @defgroup IP_group_usb_internal_def 5.define
 *               Internal define used for USB Controller.
 *         @}
 *   @}
 */

/**
 * @file mtu3_plat.c
 *
 *  USB API for module platform driver
 *
 */

#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>

#include "mtu3.h"
#include "mtu3_dr.h"

#ifdef CONFIG_USB_FPGA
#include <linux/i2c.h>
#endif

/* u2-port0 should be powered on and enabled; */

/** @ingroup IP_group_usb_internal_function
 * @par Description
 *     ssusb check clocks.
 * @param[in]
 *     ssusb: ssusb structure
 * @param[in]
 *     ex_clks: check clock
 * @return
 *     0, means SUCCESS.\n
 *     ret, if return value isn't 0, means FAIL.
 * @par Boundary case and limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int ssusb_check_clocks(struct ssusb_mtk *ssusb, u32 ex_clks)
{
	void __iomem *ibase = ssusb->ippc_base;
	u32 value, check_val;
	int ret;

	check_val = ex_clks | SSUSB_SYS125_RST_B_STS | SSUSB_SYSPLL_STABLE |
			SSUSB_REF_RST_B_STS;

	ret = readl_poll_timeout(ibase + U3D_SSUSB_IP_PW_STS1, value,
			(check_val == (value & check_val)), 100, 20000);
	if (ret) {
		dev_err(ssusb->dev, "clks of sts1 are not stable!\n");
		return ret;
	}

	ret = readl_poll_timeout(ibase + U3D_SSUSB_IP_PW_STS2, value,
			(value & SSUSB_U2_MAC_SYS_RST_B_STS), 100, 10000);
	if (ret) {
		dev_err(ssusb->dev, "mac2 clock is not stable\n");
		return ret;
	}

	return 0;
}

#ifdef CONFIG_USB_FPGA
#define I2C_W 0
#define I2C_R 1
#define READ_BACK_VRIFY
#endif

/** @ingroup IP_group_usb_internal_function
 * @par Description
 *     ssusb PHY initial.
 * @param[in]
 *     ssusb: ssusb structure
 * @return
 *     0, means SUCCESS.\n
 *     ret, if return value isn't 0, means FAIL.
 * @par Boundary case and limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int ssusb_phy_init(struct ssusb_mtk *ssusb)
{
	int i;
	int ret;

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

	adap = i2c_get_adapter(1);	/*get i2c id=1 adapter struct */

	if (adap == NULL) {
		dev_err(ssusb->dev,
			"Can't get USB phy i2c adapter channel ......\n");
		return 0;
	}

	speed_timing = 400000;
	/********** set slave addr=0x60 initial value to phy board *******/
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
			dev_err(ssusb->dev,
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
			dev_err(ssusb->dev,
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
			dev_err(ssusb->dev,
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
			dev_err(ssusb->dev,
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
			dev_err(ssusb->dev,
				"i2c verify fail addr=%x,Wdata=%x , Rdate=%x\n",
				write_phyinit[i], write_phyinit[i + 1],
				*msgs_test[0].buf);
	}

#endif

	/********** set slave addr=0x70 ckphase value to phy board *********/
	dev_dbg(ssusb->dev, "%s , i2c setting phy ckphase value ......\n",
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
			dev_err(ssusb->dev,
				"i2c verify fail addr=%x,Wdata=%x , Rdate=%x\n",
				write_ckphase[i], write_ckphase[i + 1],
				*msgs_test[0].buf);
	}

#endif

#endif
	for (i = 0; i < ssusb->num_phys; i++) {
		ret = phy_init(ssusb->phys[i]);
		if (ret) {
			/* ssusb ref_ck gating enable */
			mtu3_setbits(ssusb->ippc_base, U3D_SSUSB_REF_CK_CTRL,
				SSUSB_REF_CK_GATE_EN);
			goto exit_phy;
		}
	}
	return 0;

exit_phy:
	for (; i > 0; i--)
		phy_exit(ssusb->phys[i - 1]);

	return ret;
}

/** @ingroup IP_group_usb_internal_function
 * @par Description
 *     ssusb PHY exit.
 * @param[in]
 *     ssusb: ssusb structure
 * @return
 *     0, means SUCCESS.
 * @par Boundary case and limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int ssusb_phy_exit(struct ssusb_mtk *ssusb)
{
	int i;

	for (i = 0; i < ssusb->num_phys; i++)
		phy_exit(ssusb->phys[i]);

	return 0;
}

/** @ingroup IP_group_usb_internal_function
 * @par Description
 *     ssusb PHY power on.
 * @param[in]
 *     ssusb: ssusb structure
 * @return
 *     0, means SUCCESS.\n
 *     ret, if return value isn't 0, means FAIL.
 * @par Boundary case and limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int ssusb_phy_power_on(struct ssusb_mtk *ssusb)
{
	int i;
	int ret;

	for (i = 0; i < ssusb->num_phys; i++) {
		ret = phy_power_on(ssusb->phys[i]);
		if (ret)
			goto power_off_phy;
	}

	return 0;

power_off_phy:

	for (; i > 0; i--)
		phy_power_off(ssusb->phys[i - 1]);

	return ret;
}

/** @ingroup IP_group_usb_internal_function
 * @par Description
 *     ssusb PHY power off.
 * @param[in]
 *     ssusb: ssusb structure
 * @return
 *     none
 * @par Boundary case and limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void ssusb_phy_power_off(struct ssusb_mtk *ssusb)
{
	unsigned int i;

	for (i = 0; i < ssusb->num_phys; i++)
		phy_power_off(ssusb->phys[i]);
}

/** @ingroup IP_group_usb_internal_function
 * @par Description
 *     MTK USB resource initial.
 * @param[in]
 *     ssusb: ssusb structure
 * @return
 *     0, means SUCCESS.\n
 *     ret, if return value isn't 0, means FAIL.
 * @par Boundary case and limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */

static int ssusb_clks_enable(struct ssusb_mtk *ssusb)
{
	int ret;

	ret = clk_prepare_enable(ssusb->sys_clk);
	if (ret) {
		dev_err(ssusb->dev, "failed to enable sys_clk\n");
		goto sys_clk_err;
	}

	ret = clk_prepare_enable(ssusb->ref_clk);
	if (ret) {
		dev_err(ssusb->dev, "failed to enable ref_clk\n");
		goto ref_clk_err;
	}

	ret = clk_prepare_enable(ssusb->mcu_clk);
	if (ret) {
		dev_err(ssusb->dev, "failed to enable mcu_clk\n");
		goto mcu_clk_err;
	}

	ret = clk_prepare_enable(ssusb->dma_clk);
	if (ret) {
		dev_err(ssusb->dev, "failed to enable dma_clk\n");
		goto dma_clk_err;
	}

	return 0;

dma_clk_err:
	clk_disable_unprepare(ssusb->mcu_clk);
mcu_clk_err:
	clk_disable_unprepare(ssusb->ref_clk);
ref_clk_err:
	clk_disable_unprepare(ssusb->sys_clk);
sys_clk_err:
	return ret;
}

static void ssusb_clks_disable(struct ssusb_mtk *ssusb)
{
	clk_disable_unprepare(ssusb->dma_clk);
	clk_disable_unprepare(ssusb->mcu_clk);
	clk_disable_unprepare(ssusb->ref_clk);
	clk_disable_unprepare(ssusb->sys_clk);
}

static int ssusb_rscs_init(struct ssusb_mtk *ssusb)
{
	int ret = 0;

	ret = regulator_enable(ssusb->vusb33);
	if (ret) {
		dev_err(ssusb->dev, "failed to enable vusb33\n");
		goto vusb33_err;
	}

	ret = ssusb_clks_enable(ssusb);
	if (ret)
		goto clks_err;

	ret = ssusb_phy_init(ssusb);
	if (ret) {
		dev_err(ssusb->dev, "failed to init phy\n");
		goto phy_init_err;
	}

	ret = ssusb_phy_power_on(ssusb);
	if (ret) {
		dev_err(ssusb->dev, "failed to power on phy\n");
		goto phy_err;
	}

	return 0;

phy_err:
	ssusb_phy_exit(ssusb);
phy_init_err:
	ssusb_clks_disable(ssusb);
clks_err:
	regulator_disable(ssusb->vusb33);
vusb33_err:

	return ret;
}

/** @ingroup IP_group_usb_internal_function
 * @par Description
 *     MTK USB resource exit.
 * @param[in]
 *     ssusb: ssusb structure
 * @return
 *     none
 * @par Boundary case and limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void ssusb_rscs_exit(struct ssusb_mtk *ssusb)
{
	ssusb_clks_disable(ssusb);
	regulator_disable(ssusb->vusb33);
	ssusb_phy_power_off(ssusb);
	ssusb_phy_exit(ssusb);
}

/** @ingroup IP_group_usb_internal_function
 * @par Description
 *     ssusb IP software reset.
 * @param[in]
 *     ssusb: ssusb structure
 * @return
 *     none
 * @par Boundary case and limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void ssusb_ip_sw_reset(struct ssusb_mtk *ssusb)
{
	/* reset whole ip (xhci & u3d) */
	mtu3_setbits(ssusb->ippc_base, U3D_SSUSB_IP_PW_CTRL0, SSUSB_IP_SW_RST);
	udelay(1);
	mtu3_clrbits(ssusb->ippc_base, U3D_SSUSB_IP_PW_CTRL0, SSUSB_IP_SW_RST);
}

/** @ingroup IP_group_usb_internal_function
 * @par Description
 *     Get iddig pin control.
 * @param[in]
 *     ssusb: ssusb structure
 * @return
 *     0, means SUCCESS.\n
 *     PTR_ERR, pointer type error.
 * @par Boundary case and limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int get_iddig_pinctrl(struct ssusb_mtk *ssusb)
{
	struct otg_switch_mtk *otg_sx = &ssusb->otg_switch;

	otg_sx->id_pinctrl = devm_pinctrl_get(ssusb->dev);
	if (IS_ERR(otg_sx->id_pinctrl)) {
		dev_err(ssusb->dev, "Cannot find id pinctrl!\n");
		return PTR_ERR(otg_sx->id_pinctrl);
	}

	otg_sx->id_float =
		pinctrl_lookup_state(otg_sx->id_pinctrl, "id_float");
	if (IS_ERR(otg_sx->id_float)) {
		dev_err(ssusb->dev, "Cannot find pinctrl id_float!\n");
		return PTR_ERR(otg_sx->id_float);
	}

	otg_sx->id_ground =
		pinctrl_lookup_state(otg_sx->id_pinctrl, "id_ground");
	if (IS_ERR(otg_sx->id_ground)) {
		dev_err(ssusb->dev, "Cannot find pinctrl id_ground!\n");
		return PTR_ERR(otg_sx->id_ground);
	}

	return 0;
}

/** @ingroup IP_group_usb_internal_function
 * @par Description
 *     MTK USB get resource.
 * @param[in]
 *     pdev: platform device structure
 * @param[out]
 *     ssusb: ssusb structure
 * @return
 *     0, means SUCCESS.\n
 *     PTR_ERR, pointer type error.\n
 *     -ENOMEM, Out of memory.\n
 *     -EINVAL, Invalid argument.\n
 *     -EPROBE_DEFER, Driver requests probe retry.
 * @par Boundary case and limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int get_ssusb_rscs(struct platform_device *pdev, struct ssusb_mtk *ssusb)
{
	struct device_node *node = pdev->dev.of_node;
	struct otg_switch_mtk *otg_sx = &ssusb->otg_switch;
	struct device *dev = &pdev->dev;
	struct regulator *vbus;
	struct resource *res;
	int i;
	int ret;

	ssusb->vusb33 = devm_regulator_get(&pdev->dev, "vusb33");
	if (IS_ERR(ssusb->vusb33)) {
		dev_err(dev, "failed to get vusb33\n");
		return PTR_ERR(ssusb->vusb33);
	}

	ssusb->sys_clk = devm_clk_get(dev, "sys_ck");
	if (IS_ERR(ssusb->sys_clk)) {
		dev_err(dev, "failed to get sys clock\n");
		return PTR_ERR(ssusb->sys_clk);
	}

	ssusb->ref_clk = devm_clk_get(dev, "ref_ck");
	if (IS_ERR(ssusb->ref_clk))
		return PTR_ERR(ssusb->ref_clk);

	ssusb->mcu_clk = devm_clk_get(dev, "mcu_ck");
	if (IS_ERR(ssusb->mcu_clk))
		return PTR_ERR(ssusb->mcu_clk);

	ssusb->dma_clk = devm_clk_get(dev, "dma_ck");
	if (IS_ERR(ssusb->dma_clk))
		return PTR_ERR(ssusb->dma_clk);

	ssusb->num_phys = of_count_phandle_with_args(node,
			"phys", "#phy-cells");
	if (ssusb->num_phys > 0) {
		ssusb->phys = devm_kcalloc(dev, ssusb->num_phys,
					sizeof(*ssusb->phys), GFP_KERNEL);
		if (!ssusb->phys)
			return -ENOMEM;
	} else {
		ssusb->num_phys = 0;
	}

	#ifdef CONFIG_USB_FPGA
		ssusb->num_phys = 0;
	#endif

	for (i = 0; i < ssusb->num_phys; i++) {
		ssusb->phys[i] = devm_of_phy_get_by_index(dev, node, i);
		if (IS_ERR(ssusb->phys[i])) {
			dev_err(dev, "failed to get phy-%d\n", i);
			return PTR_ERR(ssusb->phys[i]);
		}
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ippc");
	ssusb->ippc_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(ssusb->ippc_base))
		return PTR_ERR(ssusb->ippc_base);

	ssusb->dr_mode = usb_get_dr_mode(dev);
	if (ssusb->dr_mode == USB_DR_MODE_UNKNOWN) {
		dev_err(dev, "dr_mode is error\n");
		return -EINVAL;
	}

	if (ssusb->dr_mode == USB_DR_MODE_PERIPHERAL)
		return 0;

	/* if host role is supported */
	ret = ssusb_wakeup_of_property_parse(ssusb, node);
	if (ret)
		return ret;

	if (ssusb->dr_mode != USB_DR_MODE_OTG)
		return 0;

	/* if dual-role mode is supported */
	vbus = devm_regulator_get(&pdev->dev, "vbus");
	if (IS_ERR(vbus)) {
		dev_err(dev, "failed to get vbus\n");
		return PTR_ERR(vbus);
	}
	otg_sx->vbus = vbus;

	otg_sx->is_u3_drd = of_property_read_bool(node, "mediatek,usb3-drd");
	otg_sx->manual_drd_enabled =
		of_property_read_bool(node, "enable-manual-drd");

	if (of_property_read_bool(node, "extcon")) {
		otg_sx->edev = extcon_get_edev_by_phandle(ssusb->dev, 0);
		if (IS_ERR(otg_sx->edev)) {
			dev_err(ssusb->dev, "couldn't get extcon device\n");
			return -EPROBE_DEFER;
		}
		if (otg_sx->manual_drd_enabled) {
			ret = get_iddig_pinctrl(ssusb);
			if (ret)
				return ret;
		}
	}

	dev_info(dev, "dr_mode: %d, is_u3_dr: %d\n",
		ssusb->dr_mode, otg_sx->is_u3_drd);

	return 0;
}

/** @ingroup IP_group_usb_internal_function
 * @par Description
 *     MTK USB probe.
 * @param[out]
 *     pdev: platform device structure
 * @return
 *     0, means SUCCESS.\n
 *     ret, if return value isn't 0, means FAIL.\n
 *     -ENOMEM, Out of memory.\n
 *     -ENOTSUPP, Operation is not supported.\n
 *     -EINVAL, Invalid argument.
 * @par Boundary case and limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtu3_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	struct ssusb_mtk *ssusb;
	int ret = -ENOMEM;

	/* all elements are set to ZERO as default value */
	ssusb = devm_kzalloc(dev, sizeof(*ssusb), GFP_KERNEL);
	if (!ssusb)
		return -ENOMEM;

	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (ret) {
		dev_err(dev, "No suitable DMA config available\n");
		return -ENOTSUPP;
	}

	platform_set_drvdata(pdev, ssusb);
	ssusb->dev = dev;

	ret = get_ssusb_rscs(pdev, ssusb);
	if (ret)
		return ret;

	/* enable power domain */
	pm_runtime_enable(dev);
	pm_runtime_get_sync(dev);
	device_enable_async_suspend(dev);

	ret = ssusb_rscs_init(ssusb);
	if (ret)
		goto comm_init_err;

	ssusb_ip_sw_reset(ssusb);

	if (IS_ENABLED(CONFIG_USB_MTU3_HOST))
		ssusb->dr_mode = USB_DR_MODE_HOST;
	else if (IS_ENABLED(CONFIG_USB_MTU3_GADGET))
		ssusb->dr_mode = USB_DR_MODE_PERIPHERAL;

	/* default as host */
	ssusb->is_host = !(ssusb->dr_mode == USB_DR_MODE_PERIPHERAL);

	switch (ssusb->dr_mode) {
	case USB_DR_MODE_PERIPHERAL:
		ret = ssusb_gadget_init(ssusb);
		if (ret) {
			dev_err(dev, "failed to initialize gadget\n");
			goto comm_exit;
		}
		break;
	case USB_DR_MODE_HOST:
		ret = ssusb_host_init(ssusb, node);
		if (ret) {
			dev_err(dev, "failed to initialize host\n");
			goto comm_exit;
		}
		break;
	case USB_DR_MODE_OTG:
		ret = ssusb_gadget_init(ssusb);
		if (ret) {
			dev_err(dev, "failed to initialize gadget\n");
			goto comm_exit;
		}

		ret = ssusb_host_init(ssusb, node);
		if (ret) {
			dev_err(dev, "failed to initialize host\n");
			goto gadget_exit;
		}

		ssusb_otg_switch_init(ssusb);
		break;
	default:
		dev_err(dev, "unsupported mode: %d\n", ssusb->dr_mode);
		ret = -EINVAL;
		goto comm_exit;
	}

	return 0;

gadget_exit:
	ssusb_gadget_exit(ssusb);
comm_exit:
	ssusb_rscs_exit(ssusb);
comm_init_err:
	pm_runtime_put_sync(dev);
	pm_runtime_disable(dev);

	return ret;
}

/** @ingroup IP_group_usb_internal_function
 * @par Description
 *     MTK USB remove.
 * @param[out]
 *     pdev: platform device structure
 * @return
 *     0, means SUCCESS.\n
 *     -EINVAL, Invalid argument.
 * @par Boundary case and limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtu3_remove(struct platform_device *pdev)
{
	struct ssusb_mtk *ssusb = platform_get_drvdata(pdev);

	switch (ssusb->dr_mode) {
	case USB_DR_MODE_PERIPHERAL:
		ssusb_gadget_exit(ssusb);
		break;
	case USB_DR_MODE_HOST:
		ssusb_host_exit(ssusb);
		break;
	case USB_DR_MODE_OTG:
		ssusb_otg_switch_exit(ssusb);
		ssusb_gadget_exit(ssusb);
		ssusb_host_exit(ssusb);
		break;
	default:
		return -EINVAL;
	}

	ssusb_rscs_exit(ssusb);
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	return 0;
}

/** @ingroup IP_group_usb_internal_function
 * @par Description
*  set mtu3 device power on.
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
void mtu3_power_on(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ssusb_mtk *ssusb = platform_get_drvdata(pdev);
	struct mtu3 *mtu = ssusb->u3d;
	int ret;

	dev_dbg(dev, "%s\n", __func__);

	ret = ssusb_phy_power_on(ssusb);
	if (ret)
		dev_err(ssusb->dev, "failed to power on phy\n");
	mtu3_start(mtu);
}
EXPORT_SYMBOL_GPL(mtu3_power_on);

/** @ingroup IP_group_usb_internal_function
 * @par Description
 * set mtu3 device power off.
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
void mtu3_power_off(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ssusb_mtk *ssusb = platform_get_drvdata(pdev);
	struct mtu3 *mtu = ssusb->u3d;

	dev_dbg(dev, "%s\n", __func__);

	mtu3_stop(mtu);
	ssusb_phy_power_off(ssusb);
}
EXPORT_SYMBOL_GPL(mtu3_power_off);


/*
 * when support dual-role mode, we reject suspend when
 * it works as device mode;
 */

/** @ingroup IP_group_usb_internal_function
 * @par Description
 *     MTK USB suspend.
 * @param[in]
 *     dev: The basic device structure
 * @return
 *     0, means SUCCESS.
 * @par Boundary case and limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int __maybe_unused mtu3_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ssusb_mtk *ssusb = platform_get_drvdata(pdev);

	dev_dbg(dev, "%s\n", __func__);

	/* REVISIT: disconnect it for only device mode? */
	if (!ssusb->is_host)
		return 0;

	ssusb_host_disable(ssusb, true);
	ssusb_phy_power_off(ssusb);
	ssusb_clks_disable(ssusb);
	ssusb_wakeup_enable(ssusb);

	return 0;
}

/** @ingroup IP_group_usb_internal_function
 * @par Description
 *     MTK USB resume.
 * @param[in]
 *     dev: The basic device structure
 * @return
 *     0, means SUCCESS.\n
 *     ret, if return value isn't 0, means FAIL.
 * @par Boundary case and limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int __maybe_unused mtu3_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ssusb_mtk *ssusb = platform_get_drvdata(pdev);
	int ret;

	dev_dbg(dev, "%s\n", __func__);

	if (!ssusb->is_host)
		return 0;

	ssusb_wakeup_disable(ssusb);

	ret = ssusb_clks_enable(ssusb);
	if (ret)
		goto clks_err;

	ret = ssusb_phy_power_on(ssusb);
	if (ret)
		goto err_power_on;

	ssusb_host_enable(ssusb);

	return 0;

err_power_on:
	ssusb_clks_disable(ssusb);
clks_err:
	return ret;
}

static const struct dev_pm_ops mtu3_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mtu3_suspend, mtu3_resume)
};

#define DEV_PM_OPS (IS_ENABLED(CONFIG_PM) ? &mtu3_pm_ops : NULL)

#ifdef CONFIG_OF

static const struct of_device_id mtu3_of_match[] = {
	{.compatible = "mediatek,mt3612-mtu3",},
	{},
};

MODULE_DEVICE_TABLE(of, mtu3_of_match);

#endif

static struct platform_driver mtu3_driver = {
	.probe = mtu3_probe,
	.remove = mtu3_remove,
	.driver = {
		.name = MTU3_DRIVER_NAME,
		.pm = DEV_PM_OPS,
		.of_match_table = of_match_ptr(mtu3_of_match),
	},
};

module_platform_driver(mtu3_driver);

MODULE_AUTHOR("Chunfeng Yun <chunfeng.yun@mediatek.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MediaTek USB3 DRD Controller Driver");
