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
 * @file pintrl-mt3612.c
 * MT3612 pinctrl driver is used to init mt3612 external interrupt controller.\n
 */

/**
 * @defgroup IP_group_eint EINT
 *
 *   @{
 *       @defgroup IP_group_eint_external EXTERNAL
 *         The external API document for EINT. \n
 *
 *         @{
 *            @defgroup IP_group_eint_external_function 1.function
 *              none.
 *            @defgroup IP_group_eint_external_struct 2.structure
 *              none.
 *            @defgroup IP_group_eint_external_typedef 3.typedef
 *              none.
 *            @defgroup IP_group_eint_external_enum 4.enumeration
 *              none.
 *            @defgroup IP_group_eint_external_def 5.define
 *              none.
 *         @}
 *
 *       @defgroup IP_group_eint_internal INTERNAL
 *         The internal API document for EINT. \n
 *
 *         @{
 *            @defgroup IP_group_eint_internal_function 1.function
 *              Internal function in eint driver.
 *            @defgroup IP_group_eint_internal_struct 2.structure
 *              Internal structure in eint driver.
 *            @defgroup IP_group_eint_internal_typedef 3.typedef
 *              none.
 *            @defgroup IP_group_eint_internal_enum 4.enumeration
 *              Internal enumeration in eint driver.
 *            @defgroup IP_group_eint_internal_def 5.define
 *              Internal define in eint driver.
 *         @}
 *   @}
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/regmap.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <dt-bindings/pinctrl/mt65xx.h>

#include "pinctrl-mtk-common.h"
#include "pinctrl-mtk-mt3612.h"

/** @ingroup IP_group_gpio_internal_struct
 * @brief define mt3612 special i2c pin drving.
 */
static const struct mtk_pin_spec_driving_set_samereg mt3612_spec_driving[] = {
	MTK_PIN_SPEC_DRIVING(8, 0x800, 0, 2, 1),
	MTK_PIN_SPEC_DRIVING(9, 0x800, 5, 7, 6),
	MTK_PIN_SPEC_DRIVING(12, 0x800, 10, 12, 11),
	MTK_PIN_SPEC_DRIVING(13, 0x800, 15, 17, 16),
	MTK_PIN_SPEC_DRIVING(16, 0x800, 20, 22, 21),
	MTK_PIN_SPEC_DRIVING(17, 0x800, 25, 27, 26),
	MTK_PIN_SPEC_DRIVING(26, 0x810, 0, 2, 1),
	MTK_PIN_SPEC_DRIVING(27, 0x810, 5, 7, 6),
	MTK_PIN_SPEC_DRIVING(38, 0x810, 10, 12, 11),
	MTK_PIN_SPEC_DRIVING(39, 0x810, 15, 17, 16),
	MTK_PIN_SPEC_DRIVING(40, 0x810, 20, 22, 21),
	MTK_PIN_SPEC_DRIVING(41, 0x810, 25, 27, 26),
	MTK_PIN_SPEC_DRIVING(42, 0x820, 0, 2, 1),
	MTK_PIN_SPEC_DRIVING(43, 0x820, 5, 7, 6),

	MTK_PIN_SPEC_DRIVING(54, 0x800, 0, 2, 1),
	MTK_PIN_SPEC_DRIVING(55, 0x800, 5, 7, 6),
	MTK_PIN_SPEC_DRIVING(56, 0x800, 10, 12, 11),
	MTK_PIN_SPEC_DRIVING(57, 0x800, 15, 17, 16),
	MTK_PIN_SPEC_DRIVING(65, 0x800, 20, 22, 21),
	MTK_PIN_SPEC_DRIVING(66, 0x800, 25, 27, 26),
	MTK_PIN_SPEC_DRIVING(67, 0x810, 0, 2, 1),
	MTK_PIN_SPEC_DRIVING(68, 0x810, 5, 7, 6),

	MTK_PIN_SPEC_DRIVING(48, 0x800, 0, 2, 1),
	MTK_PIN_SPEC_DRIVING(49, 0x800, 5, 7, 6),

	MTK_PIN_SPEC_DRIVING(76, 0x800, 0, 2, 1),
	MTK_PIN_SPEC_DRIVING(77, 0x800, 5, 7, 6),

	MTK_PIN_SPEC_DRIVING(109, 0x800, 0, 2, 1),
	MTK_PIN_SPEC_DRIVING(110, 0x800, 5, 7, 6),
	MTK_PIN_SPEC_DRIVING(111, 0x800, 10, 12, 11),
	MTK_PIN_SPEC_DRIVING(112, 0x800, 15, 17, 16),
	MTK_PIN_SPEC_DRIVING(113, 0x800, 20, 22, 21),
	MTK_PIN_SPEC_DRIVING(114, 0x800, 25, 27, 26),
	MTK_PIN_SPEC_DRIVING(115, 0x810, 0, 2, 1),
	MTK_PIN_SPEC_DRIVING(116, 0x810, 5, 7, 6),
	MTK_PIN_SPEC_DRIVING(117, 0x810, 10, 12, 11),
	MTK_PIN_SPEC_DRIVING(118, 0x810, 15, 17, 16),
	MTK_PIN_SPEC_DRIVING(119, 0x810, 20, 22, 21),
	MTK_PIN_SPEC_DRIVING(120, 0x810, 25, 27, 26),
	MTK_PIN_SPEC_DRIVING(121, 0x820, 0, 2, 1),
	MTK_PIN_SPEC_DRIVING(122, 0x820, 5, 7, 6),
	MTK_PIN_SPEC_DRIVING(123, 0x820, 10, 12, 11),
	MTK_PIN_SPEC_DRIVING(124, 0x820, 15, 17, 16),
	MTK_PIN_SPEC_DRIVING(125, 0x820, 20, 22, 21),
	MTK_PIN_SPEC_DRIVING(128, 0x820, 25, 27, 26),
	MTK_PIN_SPEC_DRIVING(129, 0x830, 0, 2, 1),
	MTK_PIN_SPEC_DRIVING(132, 0x830, 5, 7, 6),
	MTK_PIN_SPEC_DRIVING(133, 0x830, 10, 12, 11),
	MTK_PIN_SPEC_DRIVING(135, 0x830, 15, 17, 16),

	MTK_PIN_SPEC_DRIVING(89, 0x800, 0, 2, 1),
	MTK_PIN_SPEC_DRIVING(90, 0x800, 5, 7, 6),
	MTK_PIN_SPEC_DRIVING(93, 0x800, 10, 12, 11),
	MTK_PIN_SPEC_DRIVING(94, 0x800, 15, 17, 16),
	MTK_PIN_SPEC_DRIVING(95, 0x800, 20, 22, 21),
	MTK_PIN_SPEC_DRIVING(96, 0x800, 25, 27, 26),
};

/** @ingroup IP_group_gpio_external_function
 * @par Description
 *     Set the specail i2c pin driving.
 * @param[in]
 *     regmap: gpio base register.
 * @param[in]
 *     pin: the gpio pin number.
 * @param[in]
 *     align: the value is 0x4, it is used for reset and set register.
 * @param[in]
 *     isup: enable specail drving.
 * @param[in]
 *     d1d0: the driving current value.
 * @return
 *     0: set specail i2c driving successfully.
 *     -EINVAL: set specail i2c driving fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mt3612_spec_driving_set(struct regmap *regmap, unsigned int pin,
		unsigned char align, bool isup, unsigned int d1d0)
{
	return mtk_pctrl_spec_driving_set_samereg(regmap, mt3612_spec_driving,
		ARRAY_SIZE(mt3612_spec_driving), pin, align, isup, d1d0);
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Get the specail i2c pin driving.
 * @param[in]
 *     regmap: gpio base register.
 * @param[in]
 *     pin: the gpio pin number.
 * @return
 *     int value: get specail i2c driving successfully.
 *     -1: get specail i2c driving fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mt3612_spec_driving_get(struct regmap *regmap, unsigned int pin)
{
	return mtk_spec_driving_get_samereg(regmap, mt3612_spec_driving,
		ARRAY_SIZE(mt3612_spec_driving), pin);
}

/** @ingroup IP_group_gpio_internal_struct
 * @brief define mt3612 special i2c pin pull up/down resistance.
 */
static const struct mtk_pin_spec_rsel_set_samereg mt3612_spec_rsel[] = {
	MTK_PIN_SPEC_RSEL(8, 0x800, 0, 4, 3),
	MTK_PIN_SPEC_RSEL(9, 0x800, 5, 9, 8),
	MTK_PIN_SPEC_RSEL(12, 0x800, 10, 14, 13),
	MTK_PIN_SPEC_RSEL(13, 0x800, 15, 19, 18),
	MTK_PIN_SPEC_RSEL(16, 0x800, 20, 24, 23),
	MTK_PIN_SPEC_RSEL(17, 0x800, 25, 29, 28),
	MTK_PIN_SPEC_RSEL(26, 0x810, 0, 4, 3),
	MTK_PIN_SPEC_RSEL(27, 0x810, 5, 9, 8),
	MTK_PIN_SPEC_RSEL(38, 0x810, 10, 14, 13),
	MTK_PIN_SPEC_RSEL(39, 0x810, 15, 19, 18),
	MTK_PIN_SPEC_RSEL(40, 0x810, 20, 24, 23),
	MTK_PIN_SPEC_RSEL(41, 0x810, 25, 29, 28),
	MTK_PIN_SPEC_RSEL(42, 0x820, 0, 4, 3),
	MTK_PIN_SPEC_RSEL(43, 0x820, 5, 9, 8),

	MTK_PIN_SPEC_RSEL(54, 0x800, 0, 4, 3),
	MTK_PIN_SPEC_RSEL(55, 0x800, 5, 9, 8),
	MTK_PIN_SPEC_RSEL(56, 0x800, 10, 14, 13),
	MTK_PIN_SPEC_RSEL(57, 0x800, 15, 19, 18),
	MTK_PIN_SPEC_RSEL(65, 0x800, 20, 24, 23),
	MTK_PIN_SPEC_RSEL(66, 0x800, 25, 29, 28),
	MTK_PIN_SPEC_RSEL(67, 0x810, 0, 4, 3),
	MTK_PIN_SPEC_RSEL(68, 0x810, 5, 9, 8),

	MTK_PIN_SPEC_RSEL(48, 0x800, 0, 4, 3),
	MTK_PIN_SPEC_RSEL(49, 0x800, 5, 9, 8),

	MTK_PIN_SPEC_RSEL(76, 0x800, 0, 4, 3),
	MTK_PIN_SPEC_RSEL(77, 0x800, 5, 9, 8),

	MTK_PIN_SPEC_RSEL(109, 0x800, 0, 4, 3),
	MTK_PIN_SPEC_RSEL(110, 0x800, 5, 9, 8),
	MTK_PIN_SPEC_RSEL(111, 0x800, 10, 14, 13),
	MTK_PIN_SPEC_RSEL(112, 0x800, 15, 19, 18),
	MTK_PIN_SPEC_RSEL(113, 0x800, 20, 24, 23),
	MTK_PIN_SPEC_RSEL(114, 0x800, 25, 29, 28),
	MTK_PIN_SPEC_RSEL(115, 0x810, 0, 4, 3),
	MTK_PIN_SPEC_RSEL(116, 0x810, 5, 9, 8),
	MTK_PIN_SPEC_RSEL(117, 0x810, 10, 14, 13),
	MTK_PIN_SPEC_RSEL(118, 0x810, 15, 19, 18),
	MTK_PIN_SPEC_RSEL(119, 0x810, 20, 24, 23),
	MTK_PIN_SPEC_RSEL(120, 0x810, 25, 29, 28),
	MTK_PIN_SPEC_RSEL(121, 0x820, 0, 4, 3),
	MTK_PIN_SPEC_RSEL(122, 0x820, 5, 9, 8),
	MTK_PIN_SPEC_RSEL(123, 0x820, 10, 14, 13),
	MTK_PIN_SPEC_RSEL(124, 0x820, 15, 19, 18),
	MTK_PIN_SPEC_RSEL(125, 0x820, 20, 24, 23),
	MTK_PIN_SPEC_RSEL(128, 0x820, 25, 29, 28),
	MTK_PIN_SPEC_RSEL(129, 0x830, 0, 4, 3),
	MTK_PIN_SPEC_RSEL(132, 0x830, 5, 9, 8),
	MTK_PIN_SPEC_RSEL(133, 0x830, 10, 14, 13),
	MTK_PIN_SPEC_RSEL(135, 0x830, 15, 19, 18),

	MTK_PIN_SPEC_RSEL(89, 0x800, 0, 4, 3),
	MTK_PIN_SPEC_RSEL(90, 0x800, 5, 9, 8),
	MTK_PIN_SPEC_RSEL(93, 0x800, 10, 14, 13),
	MTK_PIN_SPEC_RSEL(94, 0x800, 15, 19, 18),
	MTK_PIN_SPEC_RSEL(95, 0x800, 20, 24, 23),
	MTK_PIN_SPEC_RSEL(96, 0x800, 25, 29, 28),
};

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Set the specail i2c pin pull up/down resistance.
 * @param[in]
 *     regmap: gpio base register.
 * @param[in]
 *     pin: the gpio pin number.
 * @param[in]
 *     align: the value is 0x4, it is used for reset and set register.
 * @param[in]
 *     isup: enable specail drving.
 * @param[in]
 *     d1d0: the driving current value.
 * @return
 *     0: set specail i2c pull up/down resistance successfully.
 *     -EINVAL: set specail i2c pull up/down resistance fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mt3612_spec_rsel_set(struct regmap *regmap, unsigned int pin,
		unsigned char align, bool isup, unsigned int s1s0)
{
	return mtk_pctrl_spec_rsel_set_samereg(regmap, mt3612_spec_rsel,
		ARRAY_SIZE(mt3612_spec_rsel), pin, align, isup, s1s0);
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Get the specail i2c pin pull up/down resistance.
 * @param[in]
 *     regmap: gpio base register.
 * @param[in]
 *     pin: the gpio pin number.
 * @return
 *     int value: get specail i2c driving successfully.
 *     -1: get specail i2c driving fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mt3612_spec_rsel_get(struct regmap *regmap, unsigned int pin)
{
	return mtk_spec_rsel_get_samereg(regmap, mt3612_spec_rsel,
		ARRAY_SIZE(mt3612_spec_rsel), pin);
}

/** @ingroup IP_group_gpio_internal_struct
 * @brief define mt3612 gpio pin driving group data struct.
 */
static const struct mtk_drv_group_desc mt3612_drv_grp[] =  {
	/* 0E4E8SR 4/8/12/16 */
	MTK_DRV_GRP(4, 16, 1, 2, 4),
	/* 0E2E4SR  2/4/6/8 */
	MTK_DRV_GRP(2, 8, 1, 2, 2),
	/* E8E4E2  2/4/6/8/10/12/14/16 */
	MTK_DRV_GRP(2, 16, 0, 2, 2)
};

/** @ingroup IP_group_gpio_internal_struct
 * @brief define mt3612 special gpio drving.
 */
static const struct mtk_pin_drv_grp mt3612_pin_drv[] = {
	MTK_PIN_DRV_GRP(0, 0x600, 0, 2),
	MTK_PIN_DRV_GRP(1, 0x600, 4, 2),
	MTK_PIN_DRV_GRP(2, 0x600, 8, 2),
	MTK_PIN_DRV_GRP(3, 0x600, 12, 2),
	MTK_PIN_DRV_GRP(4, 0x600, 16, 2),
	MTK_PIN_DRV_GRP(5, 0x600, 20, 2),
	MTK_PIN_DRV_GRP(6, 0x600, 24, 2),
	MTK_PIN_DRV_GRP(7, 0x600, 28, 2),
	MTK_PIN_DRV_GRP(8, 0x610, 0, 2),
	MTK_PIN_DRV_GRP(9, 0x610, 4, 2),
	MTK_PIN_DRV_GRP(10, 0x610, 8, 2),
	MTK_PIN_DRV_GRP(11, 0x610, 12, 2),
	MTK_PIN_DRV_GRP(12, 0x610, 16, 2),
	MTK_PIN_DRV_GRP(13, 0x610, 20, 2),
	MTK_PIN_DRV_GRP(14, 0x610, 24, 2),
	MTK_PIN_DRV_GRP(15, 0x610, 28, 2),
	MTK_PIN_DRV_GRP(16, 0x620, 0, 2),
	MTK_PIN_DRV_GRP(17, 0x620, 4, 2),
	MTK_PIN_DRV_GRP(18, 0x620, 8, 2),
	MTK_PIN_DRV_GRP(19, 0x620, 12, 2),
	MTK_PIN_DRV_GRP(20, 0x620, 16, 2),
	MTK_PIN_DRV_GRP(21, 0x620, 20, 2),
	MTK_PIN_DRV_GRP(22, 0x620, 24, 2),
	MTK_PIN_DRV_GRP(23, 0x620, 28, 2),
	MTK_PIN_DRV_GRP(24, 0x630, 0, 2),
	MTK_PIN_DRV_GRP(25, 0x630, 4, 2),
	MTK_PIN_DRV_GRP(26, 0x630, 8, 2),
	MTK_PIN_DRV_GRP(27, 0x630, 12, 2),
	MTK_PIN_DRV_GRP(28, 0x630, 16, 2),
	MTK_PIN_DRV_GRP(29, 0x630, 20, 2),
	MTK_PIN_DRV_GRP(30, 0x630, 24, 2),
	MTK_PIN_DRV_GRP(31, 0x630, 28, 2),
	MTK_PIN_DRV_GRP(32, 0x640, 0, 2),
	MTK_PIN_DRV_GRP(33, 0x640, 4, 2),
	MTK_PIN_DRV_GRP(34, 0x640, 8, 2),
	MTK_PIN_DRV_GRP(35, 0x640, 12, 2),
	MTK_PIN_DRV_GRP(36, 0x640, 16, 2),
	MTK_PIN_DRV_GRP(37, 0x640, 20, 2),
	MTK_PIN_DRV_GRP(38, 0x640, 24, 2),
	MTK_PIN_DRV_GRP(39, 0x640, 28, 2),
	MTK_PIN_DRV_GRP(40, 0x650, 0, 2),
	MTK_PIN_DRV_GRP(41, 0x650, 4, 2),
	MTK_PIN_DRV_GRP(42, 0x650, 8, 2),
	MTK_PIN_DRV_GRP(43, 0x650, 12, 2),

	MTK_PIN_DRV_GRP(44, 0x600, 0, 2),
	MTK_PIN_DRV_GRP(45, 0x600, 4, 2),
	MTK_PIN_DRV_GRP(46, 0x600, 8, 2),
	MTK_PIN_DRV_GRP(47, 0x600, 12, 2),
	MTK_PIN_DRV_GRP(48, 0x600, 16, 2),
	MTK_PIN_DRV_GRP(49, 0x600, 20, 2),
	MTK_PIN_DRV_GRP(50, 0x600, 24, 2),
	MTK_PIN_DRV_GRP(51, 0x600, 28, 2),
	MTK_PIN_DRV_GRP(52, 0x610, 0, 2),
	MTK_PIN_DRV_GRP(53, 0x610, 4, 2),

	MTK_PIN_DRV_GRP(54, 0x600, 0, 2),
	MTK_PIN_DRV_GRP(55, 0x600, 4, 2),
	MTK_PIN_DRV_GRP(56, 0x600, 8, 2),
	MTK_PIN_DRV_GRP(57, 0x600, 12, 2),
	MTK_PIN_DRV_GRP(58, 0x600, 16, 2),
	MTK_PIN_DRV_GRP(59, 0x600, 20, 2),
	MTK_PIN_DRV_GRP(60, 0x600, 24, 2),
	MTK_PIN_DRV_GRP(61, 0x600, 28, 2),
	MTK_PIN_DRV_GRP(62, 0x610, 0, 2),
	MTK_PIN_DRV_GRP(63, 0x610, 4, 2),
	MTK_PIN_DRV_GRP(64, 0x610, 8, 2),
	MTK_PIN_DRV_GRP(65, 0x610, 12, 2),
	MTK_PIN_DRV_GRP(66, 0x610, 16, 2),
	MTK_PIN_DRV_GRP(67, 0x610, 20, 2),
	MTK_PIN_DRV_GRP(68, 0x610, 24, 2),
	MTK_PIN_DRV_GRP(69, 0x610, 28, 2),
	MTK_PIN_DRV_GRP(70, 0x620, 0, 2),
	MTK_PIN_DRV_GRP(71, 0x620, 4, 2),
	MTK_PIN_DRV_GRP(72, 0x620, 8, 2),
	MTK_PIN_DRV_GRP(73, 0x620, 12, 2),
	MTK_PIN_DRV_GRP(74, 0x620, 16, 2),
	MTK_PIN_DRV_GRP(75, 0x620, 20, 2),

	MTK_PIN_DRV_GRP(76, 0x600, 0, 2),
	MTK_PIN_DRV_GRP(77, 0x600, 4, 2),
	MTK_PIN_DRV_GRP(78, 0x600, 8, 2),
	MTK_PIN_DRV_GRP(79, 0x600, 12, 2),
	MTK_PIN_DRV_GRP(80, 0x600, 16, 2),
	MTK_PIN_DRV_GRP(81, 0x600, 20, 2),
	MTK_PIN_DRV_GRP(82, 0x600, 24, 2),
	MTK_PIN_DRV_GRP(83, 0x600, 28, 2),
	MTK_PIN_DRV_GRP(84, 0x610, 0, 2),
	MTK_PIN_DRV_GRP(85, 0x610, 4, 2),
	MTK_PIN_DRV_GRP(86, 0x610, 8, 2),

	MTK_PIN_DRV_GRP(87, 0x600, 0, 2),
	MTK_PIN_DRV_GRP(88, 0x600, 4, 2),
	MTK_PIN_DRV_GRP(89, 0x600, 8, 2),
	MTK_PIN_DRV_GRP(90, 0x600, 12, 2),
	MTK_PIN_DRV_GRP(91, 0x600, 16, 2),
	MTK_PIN_DRV_GRP(92, 0x600, 20, 2),
	MTK_PIN_DRV_GRP(93, 0x600, 24, 2),
	MTK_PIN_DRV_GRP(94, 0x600, 28, 2),
	MTK_PIN_DRV_GRP(95, 0x610, 0, 2),
	MTK_PIN_DRV_GRP(96, 0x610, 4, 2),
	MTK_PIN_DRV_GRP(97, 0x610, 8, 2),
	MTK_PIN_DRV_GRP(98, 0x610, 12, 2),
	MTK_PIN_DRV_GRP(99, 0x610, 16, 2),
	MTK_PIN_DRV_GRP(100, 0x610, 20, 2),
	MTK_PIN_DRV_GRP(101, 0x610, 24, 2),
	MTK_PIN_DRV_GRP(102, 0x610, 28, 2),
	MTK_PIN_DRV_GRP(103, 0x620, 0, 2),
	MTK_PIN_DRV_GRP(104, 0x620, 4, 2),
	MTK_PIN_DRV_GRP(105, 0x620, 8, 2),
	MTK_PIN_DRV_GRP(106, 0x620, 12, 2),
	MTK_PIN_DRV_GRP(107, 0x620, 16, 2),
	MTK_PIN_DRV_GRP(108, 0x620, 20, 2),

	MTK_PIN_DRV_GRP(109, 0x600, 0, 2),
	MTK_PIN_DRV_GRP(110, 0x600, 4, 2),
	MTK_PIN_DRV_GRP(111, 0x600, 8, 2),
	MTK_PIN_DRV_GRP(112, 0x600, 12, 2),
	MTK_PIN_DRV_GRP(113, 0x600, 16, 2),
	MTK_PIN_DRV_GRP(114, 0x600, 20, 2),
	MTK_PIN_DRV_GRP(115, 0x600, 24, 2),
	MTK_PIN_DRV_GRP(116, 0x600, 28, 2),
	MTK_PIN_DRV_GRP(117, 0x610, 0, 2),
	MTK_PIN_DRV_GRP(118, 0x610, 4, 2),
	MTK_PIN_DRV_GRP(119, 0x610, 8, 2),
	MTK_PIN_DRV_GRP(120, 0x610, 12, 2),
	MTK_PIN_DRV_GRP(121, 0x610, 16, 2),
	MTK_PIN_DRV_GRP(122, 0x610, 20, 2),
	MTK_PIN_DRV_GRP(123, 0x610, 24, 2),
	MTK_PIN_DRV_GRP(124, 0x610, 28, 2),
	MTK_PIN_DRV_GRP(125, 0x620, 0, 2),
	MTK_PIN_DRV_GRP(126, 0x620, 4, 2),
	MTK_PIN_DRV_GRP(127, 0x620, 8, 2),
	MTK_PIN_DRV_GRP(128, 0x620, 12, 2),
	MTK_PIN_DRV_GRP(129, 0x620, 16, 2),
	MTK_PIN_DRV_GRP(130, 0x620, 20, 2),
	MTK_PIN_DRV_GRP(131, 0x620, 24, 2),
	MTK_PIN_DRV_GRP(132, 0x620, 28, 2),
	MTK_PIN_DRV_GRP(133, 0x630, 0, 2),
	MTK_PIN_DRV_GRP(134, 0x630, 4, 2),
	MTK_PIN_DRV_GRP(135, 0x630, 8, 2),
	MTK_PIN_DRV_GRP(136, 0x630, 12, 2),
	MTK_PIN_DRV_GRP(137, 0x630, 16, 2),
	MTK_PIN_DRV_GRP(138, 0x630, 20, 2),
	MTK_PIN_DRV_GRP(139, 0x630, 24, 2),
	MTK_PIN_DRV_GRP(140, 0x630, 28, 2),
	MTK_PIN_DRV_GRP(141, 0x640, 0, 2),
	MTK_PIN_DRV_GRP(142, 0x640, 4, 2),
	MTK_PIN_DRV_GRP(143, 0x640, 8, 2),
	MTK_PIN_DRV_GRP(144, 0x640, 12, 2),
	MTK_PIN_DRV_GRP(145, 0x640, 16, 2),
	MTK_PIN_DRV_GRP(146, 0x640, 20, 2),
	MTK_PIN_DRV_GRP(147, 0x640, 24, 2),
	MTK_PIN_DRV_GRP(148, 0x640, 28, 2),

	MTK_PIN_DRV_GRP(149, 0x600, 0, 2),
	MTK_PIN_DRV_GRP(150, 0x600, 4, 2),
	MTK_PIN_DRV_GRP(151, 0x600, 8, 2),
	MTK_PIN_DRV_GRP(152, 0x600, 12, 2),
	MTK_PIN_DRV_GRP(153, 0x600, 16, 2),
	MTK_PIN_DRV_GRP(154, 0x600, 20, 2),
	MTK_PIN_DRV_GRP(155, 0x600, 24, 2),
	MTK_PIN_DRV_GRP(156, 0x600, 28, 2),
	MTK_PIN_DRV_GRP(157, 0x610, 0, 2),
	MTK_PIN_DRV_GRP(158, 0x610, 4, 2),
	MTK_PIN_DRV_GRP(159, 0x610, 8, 2),
	MTK_PIN_DRV_GRP(160, 0x610, 12, 2),
	MTK_PIN_DRV_GRP(161, 0x610, 16, 2),
	MTK_PIN_DRV_GRP(162, 0x610, 20, 2),
	MTK_PIN_DRV_GRP(163, 0x610, 24, 2),
	MTK_PIN_DRV_GRP(164, 0x610, 28, 2),
	MTK_PIN_DRV_GRP(165, 0x620, 0, 2),
	MTK_PIN_DRV_GRP(166, 0x620, 4, 2),
	MTK_PIN_DRV_GRP(167, 0x620, 8, 2),
	MTK_PIN_DRV_GRP(168, 0x620, 12, 2),
	MTK_PIN_DRV_GRP(169, 0x620, 16, 2),
	MTK_PIN_DRV_GRP(170, 0x620, 20, 2),
	MTK_PIN_DRV_GRP(171, 0x620, 24, 2),
	MTK_PIN_DRV_GRP(172, 0x620, 28, 2),
	MTK_PIN_DRV_GRP(173, 0x630, 0, 2),
	MTK_PIN_DRV_GRP(174, 0x630, 4, 2),
	MTK_PIN_DRV_GRP(175, 0x630, 8, 2),
	MTK_PIN_DRV_GRP(176, 0x630, 12, 2),
	MTK_PIN_DRV_GRP(177, 0x630, 16, 2),
	MTK_PIN_DRV_GRP(178, 0x630, 20, 2),
	MTK_PIN_DRV_GRP(179, 0x630, 24, 2),
	MTK_PIN_DRV_GRP(180, 0x630, 28, 2),
	MTK_PIN_DRV_GRP(181, 0x640, 0, 2),
	MTK_PIN_DRV_GRP(182, 0x640, 4, 2),
	MTK_PIN_DRV_GRP(183, 0x640, 8, 2),
	MTK_PIN_DRV_GRP(184, 0x640, 12, 2),
	MTK_PIN_DRV_GRP(185, 0x640, 16, 2),
	MTK_PIN_DRV_GRP(186, 0x640, 20, 2),
	MTK_PIN_DRV_GRP(187, 0x640, 24, 2),
	MTK_PIN_DRV_GRP(188, 0x640, 28, 2),
	MTK_PIN_DRV_GRP(189, 0x650, 0, 2),
	MTK_PIN_DRV_GRP(190, 0x650, 4, 2),
	MTK_PIN_DRV_GRP(191, 0x650, 8, 2),
};

/** @ingroup IP_group_gpio_internal_struct
 * @brief define mt3612 special gpio pin pull/down.
 */
static const struct mtk_pin_spec_pupd_set_samereg mt3612_spec_pupd[] = {
	MTK_PIN_PUPD_SPEC_SR(97, 0x700, 2, 1, 0),
	MTK_PIN_PUPD_SPEC_SR(98, 0x700, 6, 5, 4),
	MTK_PIN_PUPD_SPEC_SR(99, 0x700, 10, 9, 8),
	MTK_PIN_PUPD_SPEC_SR(100, 0x700, 14, 13, 12),
	MTK_PIN_PUPD_SPEC_SR(101, 0x700, 18, 17, 16),
	MTK_PIN_PUPD_SPEC_SR(102, 0x700, 22, 21, 20),
	MTK_PIN_PUPD_SPEC_SR(103, 0x700, 26, 25, 24),
	MTK_PIN_PUPD_SPEC_SR(104, 0x700, 30, 29, 28),
	MTK_PIN_PUPD_SPEC_SR(105, 0x710, 2, 1, 0),
	MTK_PIN_PUPD_SPEC_SR(106, 0x710, 6, 5, 4),
	MTK_PIN_PUPD_SPEC_SR(107, 0x710, 10, 9, 8),
	MTK_PIN_PUPD_SPEC_SR(108, 0x710, 14, 13, 12),
	MTK_PIN_PUPD_SPEC_SR(137, 0x700, 2, 1, 0),
	MTK_PIN_PUPD_SPEC_SR(138, 0x700, 6, 5, 4),
	MTK_PIN_PUPD_SPEC_SR(139, 0x700, 10, 9, 8),
	MTK_PIN_PUPD_SPEC_SR(140, 0x700, 14, 13, 12),
	MTK_PIN_PUPD_SPEC_SR(141, 0x700, 18, 17, 16),
	MTK_PIN_PUPD_SPEC_SR(142, 0x700, 22, 21, 20),
};

static int mt3612_spec_pull_set(struct mtk_pinctrl *pctl,
		struct regmap *regmap, unsigned int pin,
		unsigned char align, bool isup, unsigned int r1r0)
{
	return mtk_pctrl_spec_pull_set_samereg(pctl, regmap, mt3612_spec_pupd,
		ARRAY_SIZE(mt3612_spec_pupd), pin, align, isup, r1r0);
}

/** @ingroup IP_group_eint_internal_struct
 * @brief define debounce mt3612 supports.
 */
static const unsigned int mt3612_debounce_data[] = {
	128, 256, 500, 1000, 16000,
	32000, 64000, 128000, 256000, 512000
};

static unsigned int mt3612_spec_debounce_select(unsigned debounce)
{
	return mtk_gpio_debounce_select(mt3612_debounce_data,
		ARRAY_SIZE(mt3612_debounce_data), debounce);
}

/** @ingroup IP_group_gpio_internal_struct
 * @brief define mt3612 gpio HW related data struct.
 */
static const struct mtk_pinctrl_devdata mt3612_pinctrl_data = {
	.pins = mtk_pins_mt3612,
	.npins = ARRAY_SIZE(mtk_pins_mt3612),
	.grp_desc = mt3612_drv_grp,
	.n_grp_cls = ARRAY_SIZE(mt3612_drv_grp),
	.pin_drv_grp = mt3612_pin_drv,
	.n_pin_drv_grps = ARRAY_SIZE(mt3612_pin_drv),
	.spec_pull_set = mt3612_spec_pull_set,
	.spec_debounce_select = mt3612_spec_debounce_select,
	.spec_driving_set = mt3612_spec_driving_set,
	.spec_driving_get = mt3612_spec_driving_get,
	.spec_rsel_set = mt3612_spec_rsel_set,
	.spec_rsel_get = mt3612_spec_rsel_get,
	.dir_offset = 0x0000,
	.dout_offset = 0x0100,
	.din_offset = 0x0200,
	.pinmux_offset = 0x0300,
	.ies_offset = 0x0000,
	.smt_offset = 0x100,
	.pullen_offset = 0x0400,
	.pullsel_offset = 0x0500,
	.drv_offset = 0x600,
	.type1_start = 0,
	.type1_end = 43,
	.type2_start = 54,
	.type2_end = 75,
	.type3_start = 44,
	.type3_end = 53,
	.type4_start = 76,
	.type4_end = 86,
	.type5_start = 109,
	.type5_end = 148,
	.type6_start = 149,
	.type6_end = 191,
	.type7_start = 87,
	.type7_end = 108,
	.port_shf = 4,
	.port_mask = 0xf,
	.port_align = 4,
	.gpio_mode_bits = 4,
	.max_gpio_mode_per_reg = 8,
	.max_gpio_num_per_reg = 32,
	.eint_offsets = {
		.name = "mt3612_eint",
		.stat      = 0x000,
		.ack       = 0x040,
		.mask      = 0x080,
		.mask_set  = 0x0c0,
		.mask_clr  = 0x100,
		.sens      = 0x140,
		.sens_set  = 0x180,
		.sens_clr  = 0x1c0,
		.soft      = 0x200,
		.soft_set  = 0x240,
		.soft_clr  = 0x280,
		.pol       = 0x300,
		.pol_set   = 0x340,
		.pol_clr   = 0x380,
		.dom_en    = 0x400,
		.dbnc_ctrl = 0x500,
		.dbnc_set  = 0x600,
		.dbnc_clr  = 0x700,
		.port_mask = 0x7,
		.ports     = 7,
	},
	.ap_num = 192,
	.db_cnt = 16,
	.num_base_address = 8,
};

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     GPIO pinctrl driver probe for mt3612.
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
static int mt3612_pinctrl_probe(struct platform_device *pdev)
{
	return mtk_pctrl_init(pdev, &mt3612_pinctrl_data, NULL);
}

/** @ingroup IP_group_gpio_internal_struct
 * @brief define mt3612 of_device_id struct to match device.
 */
static const struct of_device_id mt3612_pctrl_match[] = {
	{
		.compatible = "mediatek,mt3611-pinctrl",
		.compatible = "mediatek,mt3612-pinctrl",
	},
	{ }
};
MODULE_DEVICE_TABLE(of, mt3612_pctrl_match);

/** @ingroup IP_group_gpio_internal_struct
 * @brief define mt3612 gpio platform driver.
 */
static struct platform_driver mtk_pinctrl_driver = {
	.probe = mt3612_pinctrl_probe,
	.driver = {
		.name = "mediatek,mt3612-pinctrl",
		.of_match_table = mt3612_pctrl_match,
		.pm = &mtk_eint_pm_ops,
	},
};

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     register mt3612 gpio platform driver.
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
	return platform_driver_register(&mtk_pinctrl_driver);
}

arch_initcall(mtk_pinctrl_init);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MediaTek Pinctrl Driver");
MODULE_AUTHOR("Zhiyong Tao <zhiyong.tao@mediatek.com>");
