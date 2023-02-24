/*
 * Copyright (C) 2015 MediaTek Inc.
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

/** @defgroup IP_group_linux_eMMC Linux_eMMC
 *    The MSDC is the eMMC host controller in MT3611.\n
 *    Related feature is compatible with JEDEC eMMC 5.1 specification.\n
 *    @{
 *       @defgroup type_group_linux_eMMC_def 1.Define
 *          This is Linux_eMMC Define.
 *          @{
 *             @defgroup type_group_linux_eMMC_01 1.1.Register/Macro Define
 *                PMIC wrap SW register and macro define.
 *             @defgroup type_group_linux_eMMC_02 1.2.Error Code Define
 *                Error handle code definition.
 *          @}
 *       @defgroup type_group_linux_eMMC_enum 2.Enumeration
 *          This is Linux_eMMC Enumeration.
 *       @defgroup type_group_linux_eMMC_struct 3.Structure
 *          This is Linux_eMMC Structure.
 *       @defgroup type_group_linux_eMMC_typedef 4.Typedef
 *          This is Linux eMMC Typedef.
 *       @defgroup type_group_linux_eMMC_API 5.API Reference
 *          This is Linux_eMMC API Reference.
 *       @defgroup type_group_linux_eMMC_InFn 6.Internal Function
 *          This is Linux_eMMC Internal Function.
 *    @}
 */

/**
 * @file Msdc_cust.c
 * The Msdc_cust.c file include function for eMMC host controller clock,
 * power and IO pad control.\n
 */

#ifdef pr_fmt
#undef pr_fmt
#endif

#define pr_fmt(fmt) "["KBUILD_MODNAME"]" fmt

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/slab.h>

#include "mtk_sd.h"
#include "dbg.h"


struct msdc_host *mtk_msdc_host[] = { NULL};
EXPORT_SYMBOL(mtk_msdc_host);

int g_dma_debug[HOST_MAX_NUM] = { 0};
u32 latest_int_status[HOST_MAX_NUM] = { 0};

unsigned int msdc_latest_transfer_mode[HOST_MAX_NUM] = {
	/* 0 for PIO; 1 for DMA; 3 for nothing */
	MODE_NONE
};

unsigned int msdc_latest_op[HOST_MAX_NUM] = {
	/* 0 for read; 1 for write; 2 for nothing */
	OPER_TYPE_NUM
};

/* for debug zone */
unsigned int sd_debug_zone[HOST_MAX_NUM] = {
	0
};
/* for enable/disable register dump */
unsigned int sd_register_zone[HOST_MAX_NUM] = {
	1
};
/* mode select */
u32 dma_size[HOST_MAX_NUM] = {
	512
};

u32 drv_mode[HOST_MAX_NUM] = {
	MODE_SIZE_DEP /* using DMA or not depend on the size */
};

u8 msdc_clock_src[HOST_MAX_NUM] = {
	1
};

/**************************************************************/
/* Section 1: Device Tree Global Variables                    */
/**************************************************************/
const struct of_device_id msdc_of_ids[] = {
	{   .compatible = DT_COMPATIBLE_NAME, },
	{ },
};

void __iomem *msdc_io_cfg_bases[HOST_MAX_NUM];

/**************************************************************/
/* Section 2: Power                                           */
/**************************************************************/
#if !defined(CONFIG_MACH_FPGA)
/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Set pad driving/TDSRL/RDSEL setting for eMMC controller
 *     when eMMC power on.\n
 * @param *host: msdc host structure.
 * @param on: 1: turn on eMMC host, 0: turn off eMMC host
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void msdc_emmc_power(struct msdc_host *host, u32 on)
{
	void __iomem *base = host->base;

	if (on == 0) {
		if ((MSDC_READ32(MSDC_PS) & 0x10000) != 0x10000)
			emmc_sleep_failed = 1;
	} else {
		msdc_set_driving(host, &host->hw->driving);
		msdc_set_tdsel(host, MSDC_TDRDSEL_1V8, 0);
		msdc_set_rdsel(host, MSDC_TDRDSEL_1V8, 0);
	}
}
#endif /*if !defined(CONFIG_MACH_FPGA)*/

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Assign eMMC power control function.
 * @param *host: msdc host structure.
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void msdc_set_host_power_control(struct msdc_host *host)
{
}

/**************************************************************/
/* Section 3: Clock                                           */
/**************************************************************/
#if !defined(CONFIG_MACH_FPGA)
u32 hclks_msdc50[] = { MSDC0_SRC_0, MSDC0_SRC_1, MSDC0_SRC_2, MSDC0_SRC_3,
		       MSDC0_SRC_4, MSDC0_SRC_5, MSDC0_SRC_6, MSDC0_SRC_7};

u32 *hclks_msdc_all[] = {
	hclks_msdc50,
};
u32 *hclks_msdc;

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Enable eMMC host controller clock.
 * @param *pdev: platform device pointer
 * @param *host: msdc host pointer
 * @return
 *     If return value is 0 , clocke enable successfully.\n
 *     If return value is 1 , clock enable fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int msdc_get_ccf_clk_pointer(struct platform_device *pdev,
	struct msdc_host *host)
{
	static char const * const clk_names[] = {
		MSDC0_CLK_NAME
	};
	host->clock_control = devm_clk_get(&pdev->dev, clk_names[0]);

	if (IS_ERR(host->clock_control)) {
		pr_err("[msdc%d] can not get clock control %s\n", pdev->id,
		 clk_names[0]);
		/* return 1; */
	}

	if (clk_prepare(host->clock_control)) {
		pr_err("[msdc%d] can not prepare clock control\n", pdev->id);
		return 1;
	}

	return 0;
}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Select eMMC host clock frequency.
 * @param *host: msdc host structure.
 * @param clksrc: target frequency of host clock.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void msdc_select_clksrc(struct msdc_host *host, int clksrc)
{
	if (host->id != 0) {
		pr_err("[msdc%d] NOT Support switch pll souce[%s]%d\n",
			host->id, __func__, __LINE__);
		return;
	}

	host->hclk = msdc_get_hclk(host->id, clksrc);
	host->hw->clk_src = clksrc;

	pr_err("[%s]: msdc%d select clk_src as %d(%dKHz)\n", __func__,
		host->id, clksrc, host->hclk/1000);
}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Dump current register setting of clock mux,MSDCPLL
 *     for eMMC host controller.
 * @param *host: msdc host structure.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void msdc_dump_clock_sts(struct msdc_host *host)
{

}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Check eMMC host clock status.
 * @param *status: eMMC host clock status.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void msdc_clk_status(int *status)
{
	int g_clk_gate = 0;
	int i = 0;
	/* unsigned long flags; */

	for (i = 0; i < HOST_MAX_NUM; i++) {
		if (!mtk_msdc_host[i])
			continue;
	}
	*status = g_clk_gate;
}
#endif /*if !defined(CONFIG_MACH_FPGA)*/

/**************************************************************/
/* Section 4: GPIO and Pad                                    */
/**************************************************************/
#if !defined(CONFIG_MACH_FPGA)
/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Dump current register setting of IO pad(IES/SMT/TDSEL/RDSEL
 *     /Driving current/PU PD resistor for eMMC host controller.
 * @par Parameters
 *     none.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void msdc_dump_io_padctl(void)
{

}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Set pinmux mode for all pad of eMMC host controller.
 * @param *host: msdc host structure
 *     none.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void msdc_set_pin_mode(struct msdc_host *host)
{

}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Set IES setting for all pad of eMMC host controller.
 * @param set_ies: ies setting for all pad of eMMC host controller
 *     none.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void msdc_set_io_ies(int set_ies)
{
		MSDC_SET_FIELD(MSDC0_GPIO_IES_ADDR, MSDC0_IES_ALL_MASK,
			(set_ies ? 0xFFF : 0));
}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Set SMT setting for all pad of eMMC host controller.
 * @param set_smt: smt setting for all pad of eMMC host controller
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void msdc_set_io_smt(int set_smt)
{
		MSDC_SET_FIELD(MSDC0_GPIO_SMT_ADDR, MSDC0_SMT_ALL_MASK,
			(set_smt ? 0xFFF : 0));
}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Set TDSEL setting for all pad of eMMC host controller.
 * @param flag: setting mode
 * @param value: tdsel setting
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void msdc_set_io_tdsel(u32 flag, u32 value)
{
	u32 cust_val;

	if (flag == MSDC_TDRDSEL_CUST)
		cust_val = value;
	else
		cust_val = 0;

	MSDC_SET_FIELD(MSDC0_GPIO_TDSEL1_ADDR, MSDC0_TDSEL1_CLK_MASK, cust_val);
	MSDC_SET_FIELD(MSDC0_GPIO_TDSEL1_ADDR, MSDC0_TDSEL1_DSL_MASK, cust_val);
	MSDC_SET_FIELD(MSDC0_GPIO_TDSEL1_ADDR, MSDC0_TDSEL1_CMD_MASK, cust_val);
	MSDC_SET_FIELD(MSDC0_GPIO_TDSEL1_ADDR, MSDC0_TDSEL1_DAT0_MASK,
		       cust_val);
	MSDC_SET_FIELD(MSDC0_GPIO_TDSEL1_ADDR, MSDC0_TDSEL1_DAT1_MASK,
		       cust_val);
	MSDC_SET_FIELD(MSDC0_GPIO_TDSEL1_ADDR, MSDC0_TDSEL1_DAT2_MASK,
		       cust_val);
	MSDC_SET_FIELD(MSDC0_GPIO_TDSEL2_ADDR, MSDC0_TDSEL2_DAT3_MASK,
		       cust_val);
	MSDC_SET_FIELD(MSDC0_GPIO_TDSEL2_ADDR, MSDC0_TDSEL2_DAT4_MASK,
		       cust_val);
	MSDC_SET_FIELD(MSDC0_GPIO_TDSEL2_ADDR, MSDC0_TDSEL2_DAT5_MASK,
		       cust_val);
	MSDC_SET_FIELD(MSDC0_GPIO_TDSEL2_ADDR, MSDC0_TDSEL2_DAT6_MASK,
		       cust_val);
	MSDC_SET_FIELD(MSDC0_GPIO_TDSEL2_ADDR, MSDC0_TDSEL2_DAT7_MASK,
		       cust_val);
	MSDC_SET_FIELD(MSDC0_GPIO_TDSEL2_ADDR, MSDC0_TDSEL2_RSTB_MASK,
		       cust_val);
}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Set RDSEL setting for all pad of eMMC host controller.
 * @param flag: setting mode
 * @param value: rdsel setting
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void msdc_set_io_rdsel(u32 flag, u32 value)
{
	u32 cust_val;

	if (flag == MSDC_TDRDSEL_CUST)
		cust_val = value;
	else
		cust_val = 0;

	MSDC_SET_FIELD(MSDC0_GPIO_RDSEL0_ADDR, MSDC0_RDSEL0_CLK_MASK, cust_val);
	MSDC_SET_FIELD(MSDC0_GPIO_RDSEL0_ADDR, MSDC0_RDSEL0_DSL_MASK, cust_val);
	MSDC_SET_FIELD(MSDC0_GPIO_RDSEL1_ADDR, MSDC0_RDSEL1_CMD_MASK, cust_val);
	MSDC_SET_FIELD(MSDC0_GPIO_RDSEL1_ADDR, MSDC0_RDSEL1_DAT0_MASK,
		       cust_val);
	MSDC_SET_FIELD(MSDC0_GPIO_RDSEL1_ADDR, MSDC0_RDSEL1_DAT1_MASK,
		       cust_val);
	MSDC_SET_FIELD(MSDC0_GPIO_RDSEL1_ADDR, MSDC0_RDSEL1_DAT2_MASK,
		       cust_val);
	MSDC_SET_FIELD(MSDC0_GPIO_RDSEL1_ADDR, MSDC0_RDSEL1_DAT3_MASK,
		       cust_val);
	MSDC_SET_FIELD(MSDC0_GPIO_RDSEL2_ADDR, MSDC0_RDSEL2_DAT4_MASK,
		       cust_val);
	MSDC_SET_FIELD(MSDC0_GPIO_RDSEL2_ADDR, MSDC0_RDSEL2_DAT5_MASK,
		       cust_val);
	MSDC_SET_FIELD(MSDC0_GPIO_RDSEL2_ADDR, MSDC0_RDSEL2_DAT6_MASK,
		       cust_val);
	MSDC_SET_FIELD(MSDC0_GPIO_RDSEL2_ADDR, MSDC0_RDSEL2_DAT7_MASK,
		       cust_val);
	MSDC_SET_FIELD(MSDC0_GPIO_RDSEL2_ADDR, MSDC0_RDSEL2_RST_MASK,
		       cust_val);
}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Get tdsel setting for all pad of eMMC host controller.
 * @param *value: returned tdsel setting
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void msdc_get_io_tdsel(u32 *value)
{
	MSDC_GET_FIELD(MSDC0_GPIO_TDSEL1_ADDR, MSDC0_TDSEL1_ALL_MASK,
				*value);
}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Get rdsel setting for all pad of eMMC host controller.
 * @param *value: returned rdsel setting
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void msdc_get_io_rdsel(u32 *value)
{
	MSDC_GET_FIELD(MSDC0_GPIO_RDSEL0_ADDR, MSDC0_RDSEL0_ALL_MASK,
				*value);
}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Set driving current for all pad of eMMC host controller.
 * @param *driving: assigned driving setting
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void msdc_set_io_driving(struct msdc_hw_driving *driving)
{
	MSDC_SET_FIELD(MSDC0_GPIO_DRV1_ADDR, MSDC0_DRV1_DSL_MASK,
		       driving->ds_drv);
	MSDC_SET_FIELD(MSDC0_GPIO_DRV1_ADDR, MSDC0_DRV1_CMD_MASK,
		       driving->cmd_drv);
	MSDC_SET_FIELD(MSDC0_GPIO_DRV1_ADDR, MSDC0_DRV1_CLK_MASK,
		       driving->clk_drv);
	MSDC_SET_FIELD(MSDC0_GPIO_DRV1_ADDR, MSDC0_DRV1_DAT0_MASK,
		       driving->dat_drv);
	MSDC_SET_FIELD(MSDC0_GPIO_DRV1_ADDR, MSDC0_DRV1_DAT1_MASK,
		       driving->dat_drv);
	MSDC_SET_FIELD(MSDC0_GPIO_DRV1_ADDR, MSDC0_DRV1_DAT2_MASK,
		       driving->dat_drv);
	MSDC_SET_FIELD(MSDC0_GPIO_DRV2_ADDR, MSDC0_DRV2_DAT3_MASK,
		       driving->dat_drv);
	MSDC_SET_FIELD(MSDC0_GPIO_DRV2_ADDR, MSDC0_DRV2_DAT4_MASK,
		       driving->dat_drv);
	MSDC_SET_FIELD(MSDC0_GPIO_DRV2_ADDR, MSDC0_DRV2_DAT5_MASK,
		       driving->dat_drv);
	MSDC_SET_FIELD(MSDC0_GPIO_DRV2_ADDR, MSDC0_DRV2_DAT6_MASK,
		       driving->dat_drv);
	MSDC_SET_FIELD(MSDC0_GPIO_DRV2_ADDR, MSDC0_DRV2_DAT7_MASK,
		       driving->dat_drv);
	MSDC_SET_FIELD(MSDC0_GPIO_DRV2_ADDR, MSDC0_DRV2_RSTB_MASK,
		       driving->rst_drv);
}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Get driving setting for all pad of eMMC host controller.
 * @param *driving: returned driving value
 *     none.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void msdc_get_io_driving(struct msdc_hw_driving *driving)
{
	MSDC_GET_FIELD(MSDC0_GPIO_DRV1_ADDR, MSDC0_DRV1_DSL_MASK,
		       driving->ds_drv);
	MSDC_GET_FIELD(MSDC0_GPIO_DRV1_ADDR, MSDC0_DRV1_CMD_MASK,
		       driving->cmd_drv);
	MSDC_GET_FIELD(MSDC0_GPIO_DRV1_ADDR, MSDC0_DRV1_CLK_MASK,
		       driving->clk_drv);
	MSDC_GET_FIELD(MSDC0_GPIO_DRV1_ADDR, MSDC0_DRV1_DAT1_MASK,
		       driving->dat_drv);
	MSDC_GET_FIELD(MSDC0_GPIO_DRV2_ADDR, MSDC0_DRV2_RSTB_MASK,
		       driving->rst_drv);
}


/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Set Pull-up/Pull-dn mode/resistor for all pad of eMMC host
 * @param mode: PU/PD/non-Pull mode
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void msdc_pin_io_config(u32 mode)
{
	/* Attention: don't pull CLK high; Don't toggle RST to prevent
	 * from entering boot mode
	 */
	if (mode == MSDC_PIN_PULL_NONE) {
		/* Switch MSDC0_* to no ohm PU,
		 * MSDC0_RSTB to 50K ohm PU
		 * 0x11C10700[32:0] = 0x00000000
		 * 0x11C10710[15:0] = 0x2000
		 */
		MSDC_SET_FIELD(MSDC0_GPIO_PUPD0_ADDR, MSDC0_PUPD0_MASK,
			       0x00000000);
		MSDC_SET_FIELD(MSDC0_GPIO_PUPD1_ADDR, MSDC0_PUPD1_MASK,
			       0x2000);
	} else if (mode == MSDC_PIN_PULL_DOWN) {
		/* Switch MSDC0_* to 50K ohm PD, MSDC0_RSTB to 50K ohm PU
		* 0x11C10700[32:0] = 0x66666666
		* 0x11C10710[15:0] = 0x2666
		*/
		MSDC_SET_FIELD(MSDC0_GPIO_PUPD0_ADDR, MSDC0_PUPD0_MASK,
			       0x66666666);
		MSDC_SET_FIELD(MSDC0_GPIO_PUPD1_ADDR, MSDC0_PUPD1_MASK,
			       0x2666);
	} else if (mode == MSDC_PIN_PULL_UP) {
		/* Switch MSDC0_CLK to 50K ohm PD,
		 * MSDC0_CMD/MSDC0_DAT* to 10K ohm PU,
		 * MSDC0_RSTB to 50K ohm PU, MSDC0_DSL to 50K ohm PD
		 * 0x11C10700[31:0] = 0x11111166
		 * 0x11C10710[14:0] = 0x2111
		 */
		MSDC_SET_FIELD(MSDC0_GPIO_PUPD0_ADDR, MSDC0_PUPD0_MASK,
			       0x11111166);
		MSDC_SET_FIELD(MSDC0_GPIO_PUPD1_ADDR, MSDC0_PUPD1_MASK,
			       0x2111);
	}
}
#endif /*if !defined(CONFIG_MACH_FPGA)*/


/**************************************************************/
/* Section 5: Device Tree Init function                       */
/*            This function is placed here so that all	      */
/*            functions and variables used by it has already  */
/*            been declared                                   */
/**************************************************************/
/*
 * parse pinctl settings
 * Driver strength
 */
/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Get driving setting for each speed mode for all pad
 *     of eMMC host from device tree file
 * @param *host: msdc host structure
 * @param *np: device node
 * @return
 *     0
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
#if !defined(CONFIG_MACH_FPGA)
static int msdc_get_pinctl_settings(struct msdc_host *host,
	struct device_node *np)
{
	struct device_node *pinctl_node, *pins_node;
	static char const * const pinctl_names[] = {
		"pinctl", "pinctl_sdr104", "pinctl_sdr50", "pinctl_ddr50"
	};

	/* sequence shall be the same as sequence in msdc_hw_driving */
	static char const * const pins_names[] = {
		"pins_cmd", "pins_dat", "pins_clk", "pins_rst", "pins_ds"
	};
	unsigned char *pin_drv;
	int i, j;

	host->hw->driving_applied = &host->hw->driving;
	for (i = 0; i < ARRAY_SIZE(pinctl_names); i++) {
		pinctl_node = of_parse_phandle(np, pinctl_names[i], 0);

		if (strcmp(pinctl_names[i], "pinctl") == 0)
			pin_drv = (unsigned char *)&host->hw->driving;
		else if (strcmp(pinctl_names[i], "pinctl_sdr104") == 0)
			pin_drv = (unsigned char *)&host->hw->driving_sd_sdr104;
		else if (strcmp(pinctl_names[i], "pinctl_sdr50") == 0)
			pin_drv = (unsigned char *)&host->hw->driving_sd_sdr50;
		else if (strcmp(pinctl_names[i], "pinctl_ddr50") == 0)
			pin_drv = (unsigned char *)&host->hw->driving_sd_ddr50;
		else
			continue;

		for (j = 0; j < ARRAY_SIZE(pins_names); j++) {
			pins_node = of_get_child_by_name(pinctl_node,
				pins_names[j]);

			if (pins_node)
				of_property_read_u8(pins_node,
					"drive-strength", pin_drv);
			pin_drv++;
		}
	}

	return 0;
}
#endif

/* Get msdc register settings
 * 1. internal data delay for tuning, FIXME: can be removed when use data tune?
 * 2. sample edge
 */
/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Get sampling edge setting for eMMC host from device tree file
 * @param *host: msdc host structure
 * @param *np: device node
 * @return
 *     0
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int msdc_get_register_settings(struct msdc_host *host,
struct device_node *np)
{
	struct device_node *register_setting_node = NULL;

	/* parse hw property settings */
	register_setting_node = of_parse_phandle(np, "register_setting", 0);
	if (register_setting_node) {
		of_property_read_u8(register_setting_node, "cmd_edge",
				&host->hw->cmd_edge);
		of_property_read_u8(register_setting_node, "rdata_edge",
				&host->hw->rdata_edge);
		of_property_read_u8(register_setting_node, "wdata_edge",
				&host->hw->wdata_edge);
	} else {
		pr_err("[msdc%d] register_setting is not found in DT\n",
			host->id);
	}

	return 0;
}

/*
 *	msdc_of_parse() - parse host's device-tree node
 *	@host: host whose node should be parsed.
 *
 */
/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     parse host's device-tree node
 * @param *mmc: host whose node should be parsed
 * @return
 *     If return 0,  parse host's device-tree node ok. \n
 *     If value < 0, parse host's device-tree node error.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int msdc_of_parse(struct mmc_host *mmc)
{
	struct device_node *np;
	struct msdc_host *host = mmc_priv(mmc);
	int ret = 0;
	int len = 0;

	ret = mmc_of_parse(mmc);
	if (ret) {
		pr_err("%s: mmc of parse error!!: %d\n", __func__, ret);
		return ret;
	}

	np = mmc->parent->of_node; /* mmcx node in project dts */

	host->mmc = mmc;
	host->hw = kzalloc(sizeof(struct msdc_hw), GFP_KERNEL);

	/* iomap register */
	host->base = of_iomap(np, 0);
	if (!host->base) {
		pr_err("[msdc%d] of_iomap failed\n", mmc->index);
		return -ENOMEM;
	}

	/* get irq # */
	host->irq = irq_of_parse_and_map(np, 0);
	pr_err("[msdc%d] get irq # %d\n", mmc->index, host->irq);
	WARN_ON(host->irq < 0);

#if !defined(CONFIG_MACH_FPGA)
	/* get clk_src */
	if (of_property_read_u8(np, "clk_src", &host->hw->clk_src)) {
		pr_err("[msdc%d] error: clk_src isn't found in device tree.\n",
			host->id);
		WARN_ON(1);
	}
#endif

	/* get msdc flag(caps)*/
	if (of_find_property(np, "msdc-sys-suspend", &len))
		host->hw->flags |= MSDC_SYS_SUSPEND;

	/* Returns 0 on success, -EINVAL if the property does not exist,
	 * -ENODATA if property does not have a value, and -EOVERFLOW if the
	 * property data isn't large enough.
	 */
	if (of_property_read_u8(np, "host_function", &host->hw->host_function))
		pr_err("[msdc%d] host_function isn't found in device tree\n",
			host->id);

	if (of_find_property(np, "bootable", &len))
		host->hw->boot = 1;

	/* get cd_gpio and cd_level */
	if (of_property_read_u32_index(np, "cd-gpios", 1, &cd_gpio) == 0) {
		if (of_property_read_u8(np, "cd_level", &host->hw->cd_level))
			pr_err("[msdc%d] cd_level isn't found in device tree\n",
				host->id);
	}

	msdc_get_register_settings(host, np);

#if !defined(CONFIG_MACH_FPGA)
	msdc_get_pinctl_settings(host, np);
	np = of_find_compatible_node(NULL, NULL, "mediatek,msdc_top");
	host->base_top = of_iomap(np, 0);
	pr_debug("of_iomap for MSDC%d TOP base @ 0x%p\n",
			host->id, host->base_top);
#endif

	return 0;
}


/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Match host with device-tree node.And get related information for driver
 * @param *pdev: platform device for match
 * @param *host: host whose node should be match
 * @return
 *     If return 0,  device tree initial ok.
 *     If value < 0, device tree initial error.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int msdc_dt_init(struct platform_device *pdev, struct mmc_host *mmc)
{
	struct msdc_host *host = mmc_priv(mmc);
	unsigned int id = 0;
	int ret;
	static char const * const msdc_names[] = {
		"msdc0"};
#ifndef CONFIG_MACH_FPGA
	static char const * const ioconfig_names[] = {
		MSDC0_IOCFG_NAME
	};
	struct device_node *np;
#endif

	pr_err("DT probe %s!\n", pdev->dev.of_node->name);

	for (id = 0; id < HOST_MAX_NUM; id++) {
		if (strcmp(pdev->dev.of_node->name, msdc_names[id]) == 0) {
			pdev->id = id;
			break;
		}
	}

	if (id == HOST_MAX_NUM) {
		pr_err("%s: Can not find msdc host\n", __func__);
		return 1;
	}

	ret = msdc_of_parse(mmc);
	if (ret) {
		pr_err("%s: msdc%d of parse error!!: %d\n", __func__, id, ret);
		return ret;
	}

	host = mmc_priv(mmc);
	host->id = id;

#ifndef CONFIG_MACH_FPGA
	if (msdc_io_cfg_bases[id] == NULL) {
		np = of_find_compatible_node(NULL, NULL, ioconfig_names[id]);
		msdc_io_cfg_bases[id] = of_iomap(np, 0);
		pr_debug("of_iomap for MSDC%d IOCFG base @ 0x%p\n",
			id, msdc_io_cfg_bases[id]);
	}
#endif

	return 0;
}

/**************************************************************/
/* Section 7: For msdc register dump                          */
/**************************************************************/
u16 msdc_offsets[] = {
	OFFSET_MSDC_CFG,
	OFFSET_MSDC_IOCON,
	OFFSET_MSDC_PS,
	OFFSET_MSDC_INT,
	OFFSET_MSDC_INTEN,
	OFFSET_MSDC_FIFOCS,
	OFFSET_SDC_CFG,
	OFFSET_SDC_CMD,
	OFFSET_SDC_ARG,
	OFFSET_SDC_STS,
	OFFSET_SDC_RESP0,
	OFFSET_SDC_RESP1,
	OFFSET_SDC_RESP2,
	OFFSET_SDC_RESP3,
	OFFSET_SDC_BLK_NUM,
	OFFSET_SDC_VOL_CHG,
	OFFSET_SDC_CSTS,
	OFFSET_SDC_CSTS_EN,
	OFFSET_SDC_DCRC_STS,
	OFFSET_SDC_CMD_STS,
	OFFSET_EMMC_CFG0,
	OFFSET_EMMC_CFG1,
	OFFSET_EMMC_STS,
	OFFSET_EMMC_IOCON,
	OFFSET_SDC_ACMD_RESP,
	OFFSET_SDC_ACMD19_TRG,
	OFFSET_SDC_ACMD19_STS,
	OFFSET_MSDC_DMA_SA_HIGH,
	OFFSET_MSDC_DMA_SA,
	OFFSET_MSDC_DMA_CA,
	OFFSET_MSDC_DMA_CTRL,
	OFFSET_MSDC_DMA_CFG,
	OFFSET_MSDC_DMA_LEN,
	OFFSET_MSDC_DBG_SEL,
	OFFSET_MSDC_DBG_OUT,
	OFFSET_MSDC_PATCH_BIT0,
	OFFSET_MSDC_PATCH_BIT1,
	OFFSET_MSDC_PATCH_BIT2,

	OFFSET_MSDC_PAD_TUNE0,
	OFFSET_MSDC_PAD_TUNE1,
	OFFSET_MSDC_DAT_RDDLY0,
	OFFSET_MSDC_DAT_RDDLY1,
	OFFSET_MSDC_DAT_RDDLY2,
	OFFSET_MSDC_DAT_RDDLY3,
	OFFSET_MSDC_HW_DBG,
	OFFSET_MSDC_VERSION,

	OFFSET_EMMC50_PAD_DS_TUNE,
	OFFSET_EMMC50_PAD_CMD_TUNE,
	OFFSET_EMMC50_PAD_DAT01_TUNE,
	OFFSET_EMMC50_PAD_DAT23_TUNE,
	OFFSET_EMMC50_PAD_DAT45_TUNE,
	OFFSET_EMMC50_PAD_DAT67_TUNE,
	OFFSET_EMMC51_CFG0,
	OFFSET_EMMC50_CFG0,
	OFFSET_EMMC50_CFG1,
	OFFSET_EMMC50_CFG2,
	OFFSET_EMMC50_CFG3,
	OFFSET_EMMC50_CFG4,
	OFFSET_SDC_FIFO_CFG,

	OFFSET_MSDC_IOCON_1,
	OFFSET_MSDC_PATCH_BIT0_1,
	OFFSET_MSDC_PATCH_BIT1_1,
	OFFSET_MSDC_PATCH_BIT2_1,
	OFFSET_MSDC_PAD_TUNE0_1,
	OFFSET_MSDC_PAD_TUNE1_1,
	OFFSET_MSDC_DAT_RDDLY0_1,
	OFFSET_MSDC_DAT_RDDLY1_1,
	OFFSET_MSDC_DAT_RDDLY2_1,
	OFFSET_MSDC_DAT_RDDLY3_1,
	OFFSET_MSDC_IOCON_2,
	OFFSET_MSDC_PATCH_BIT0_2,
	OFFSET_MSDC_PATCH_BIT1_2,
	OFFSET_MSDC_PATCH_BIT2_2,
	OFFSET_MSDC_PAD_TUNE0_2,
	OFFSET_MSDC_PAD_TUNE1_2,
	OFFSET_MSDC_DAT_RDDLY0_2,
	OFFSET_MSDC_DAT_RDDLY1_2,
	OFFSET_MSDC_DAT_RDDLY2_2,
	OFFSET_MSDC_DAT_RDDLY3_2,

#ifdef CONFIG_MTK_EMMC_CQ_SUPPORT
	OFFSET_EMMC51_CFG0,
	OFFSET_EMMC50_CFG0,
	OFFSET_EMMC50_CFG1,
	OFFSET_EMMC50_CFG2,
	OFFSET_EMMC50_CFG3,
	OFFSET_EMMC50_CFG4,
#endif

	0xFFFF /*as mark of end */
};
