/*
 * Copyright (c) 2014-2015 MediaTek Inc.
 * Author: Hongzhou.Yang <hongzhou.yang@mediatek.com>
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
 * @file pintrl-mt3615.c
 * MT3612 pinctrl driver is used to init mt3615 gpio pin controller.\n
 */

/**
 * @defgroup IP_group_gpio GPIO
 *
 *   @{
 *       @defgroup IP_group_gpio_external EXTERNAL
 *         The external API document for GPIO. \n
 *
 *         @{
 *            @defgroup IP_group_gpio_external_function 1.function
 *              none.
 *            @defgroup IP_group_gpio_external_struct 2.structure
 *              none.
 *            @defgroup IP_group_gpio_external_typedef 3.typedef
 *              none.
 *            @defgroup IP_group_gpio_external_enum 4.enumeration
 *              none.
 *            @defgroup IP_group_gpio_external_def 5.define
 *              none.
 *         @}
 *
 *       @defgroup IP_group_gpio_internal INTERNAL
 *         The internal API document for GPIO. \n
 *
 *         @{
 *            @defgroup IP_group_gpio_internal_function 1.function
 *              Internal function in gpio driver.
 *            @defgroup IP_group_gpio_internal_struct 2.structure
 *              Internal structure in gpio driver.
 *            @defgroup IP_group_gpio_internal_typedef 3.typedef
 *              none.
 *            @defgroup IP_group_gpio_internal_enum 4.enumeration
 *              Internal enumeration in gpio driver.
 *            @defgroup IP_group_gpio_internal_def 5.define
 *              Internal define in gpio driver.
 *         @}
 *   @}
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/mfd/mt3615/core.h>
#include <linux/mfd/syscon.h>

#include "pinctrl-mtk-common.h"
#include "pinctrl-mtk-mt3615.h"


/** @ingroup IP_group_gpio_internal_struct
 * @brief define mt3615 gpio HW related data struct.
 */
 #define MT3615_PIN_REG_BASE  0x0000

static const struct mtk_pinctrl_devdata mt3615_pinctrl_data = {
	.pins = mtk_pins_mt3615,
	.npins = ARRAY_SIZE(mtk_pins_mt3615),
	.dir_offset = (MT3615_PIN_REG_BASE + 0x0088),
	.ies_offset = MTK_PINCTRL_NOT_SUPPORT,
	.smt_offset = MTK_PINCTRL_NOT_SUPPORT,
	.pullen_offset = (MT3615_PIN_REG_BASE + 0x0094),
	.pullsel_offset = (MT3615_PIN_REG_BASE + 0x00A0),
	.dout_offset = (MT3615_PIN_REG_BASE + 0x00B8),
	.din_offset = (MT3615_PIN_REG_BASE + 0x00C4),
	.pinmux_offset = (MT3615_PIN_REG_BASE + 0x00CC),
	.gpio_mode_bits = 3,
	.max_gpio_mode_per_reg = 4,
	.type1_start = 0,
	.type1_end = 22,
	.type2_start = 23,
	.type2_end = 23,
	.type3_start = 23,
	.type3_end = 23,
	.type4_start = 23,
	.type4_end = 23,
	.type5_start = 23,
	.type5_end = 23,
	.type6_start = 23,
	.type6_end = 23,
	.port_shf = 3,
	.port_mask = 0x3,
	.port_align = 2,
	.max_gpio_num_per_reg = 16,
	.num_base_address = 1,
};

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     GPIO pinctrl driver probe for mt3615.
 * @param[out]
 *     pdev: platform device pointer.
 * @return
 *     return value from mtk_pctrl_init().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mt3615_pinctrl_probe(struct platform_device *pdev)
{
	struct mt3615_chip *mt3615;

	mt3615 = dev_get_drvdata(pdev->dev.parent);
	dev_info(&pdev->dev, "mt3615->regmap = %p\n", mt3615->regmap);

	return mtk_pctrl_init(pdev, &mt3615_pinctrl_data, mt3615->regmap);
}

/** @ingroup IP_group_gpio_internal_struct
 * @brief define mt3615 of_device_id struct to match device.
 */
static const struct of_device_id mt3615_pctrl_match[] = {
	{
		.compatible = "mediatek,mt3615-pinctrl",
	},
	{ }
};
MODULE_DEVICE_TABLE(of, mt3615_pctrl_match);

/** @ingroup IP_group_gpio_internal_struct
 * @brief define mt3615 gpio platform driver.
 */
static struct platform_driver mtk_pinctrl_driver = {
	.probe = mt3615_pinctrl_probe,
	.driver = {
		.name = "mediatek-mt3615-pinctrl",
		.of_match_table = mt3615_pctrl_match,
	},
};

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     register mt3615 gpio platform driver.
 * @par parameters
 *     none.
 * @return
 *     return value from platform_driver_register().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int __init mtk_pinctrl_init(void)
{
	pr_info("Func(%s),line(%d)\n", __func__, __LINE__);
	return platform_driver_register(&mtk_pinctrl_driver);
}

arch_initcall(mtk_pinctrl_init);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MediaTek Pinctrl Driver");
MODULE_AUTHOR("Tony Chang <tony.chang@mediatek.com>");
