/*
 * mt65xx pinctrl driver based on Allwinner A1X pinctrl driver.
 * Copyright (c) 2014 MediaTek Inc.
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
 * @file pintrl-mtk-common.c
 * MTK pinctrl driver is used to configure and control GPIO pins.\n
 * It provides the interfaces which will be used for GPIO configure\n
 * and control.
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

#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/bitops.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/pm.h>
#include <dt-bindings/pinctrl/mt65xx.h>

#include "../core.h"
#include "../pinconf.h"
#include "../pinctrl-utils.h"
#include "pinctrl-mtk-common.h"

#define MAX_GPIO_MODE_PER_REG 8
#define GPIO_MODE_BITS        4
#define GPIO_MODE_PREFIX "GPIO"

/**
 * @brief define gpio pinmux function.
 */
static const char * const mtk_gpio_functions[] = {
	"func0", "func1", "func2", "func3",
	"func4", "func5", "func6", "func7",
};

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Get the regmap information of gpio pin.
 * @param[in]
 *     pctl: mtk_pinctrl pointer, struct mtk_pinctrl contains gpio\n
 *     pin controller data.
 * @param[in]
 *     pin: gpio pin number.
 * @return
 *     the regmap information.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If gpio pin number not valid, return -EINVAL.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static struct regmap *mtk_get_regmap(struct mtk_pinctrl *pctl,
		unsigned long pin)
{
	if (!pctl->reg_pinctrl)
		return pctl->regmap0;

	if (pctl->reg_pinctrl) {
		if (pin >= pctl->devdata->type1_start &&
			pin <= pctl->devdata->type1_end)
			return pctl->regmap1;
		else if (pin >= pctl->devdata->type2_start &&
			pin <= pctl->devdata->type2_end)
			return pctl->regmap2;
		else if (pin >= pctl->devdata->type3_start &&
			pin <= pctl->devdata->type3_end)
			return pctl->regmap3;
		else if (pin >= pctl->devdata->type4_start &&
			pin <= pctl->devdata->type4_end)
			return pctl->regmap4;
		else if (pin >= pctl->devdata->type5_start &&
			pin <= pctl->devdata->type5_end)
			return pctl->regmap5;
		else if (pin >= pctl->devdata->type6_start &&
			pin <= pctl->devdata->type6_end)
			return pctl->regmap6;
		else if (pin >= pctl->devdata->type7_start &&
			pin <= pctl->devdata->type7_end)
			return pctl->regmap7;
	}
	return NULL;
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Get the gpio register address offset.
 * @param[in]
 *     pctl: mtk_pinctrl pointer, struct mtk_pinctrl contains gpio\n
 *     pin controller data.
 * @param[in]
 *     pin: gpio pin number.
 * @return
 *     the gpio register address offset.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static unsigned int mtk_get_port(struct mtk_pinctrl *pctl, unsigned long pin)
{
	/* Different SoC has different mask and port shift. */
	return ((pin / pctl->devdata->max_gpio_num_per_reg)
		& pctl->devdata->port_mask)
		<< pctl->devdata->port_shf;
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Get the real gpio pin number in different base address.
 * @param[in]
 *     pctl: mtk_pinctrl pointer, struct mtk_pinctrl contains gpio\n
 *     pin controller data.
 * @param[in]
 *     pin: gpio pin number.
 * @return
 *     the real gpio pin number in different base address.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
unsigned long mtk_get_pin(struct mtk_pinctrl *pctl, unsigned int pin)
{
	if (pin >= pctl->devdata->type1_start &&
		pin <= pctl->devdata->type1_end)
		return (pin - pctl->devdata->type1_start);
	else if (pin >= pctl->devdata->type2_start &&
		pin <= pctl->devdata->type2_end)
		return (pin - pctl->devdata->type2_start);
	else if (pin >= pctl->devdata->type3_start &&
		pin <= pctl->devdata->type3_end)
		return (pin - pctl->devdata->type3_start);
	else if (pin >= pctl->devdata->type4_start &&
		pin <= pctl->devdata->type4_end)
		return (pin - pctl->devdata->type4_start);
	else if (pin >= pctl->devdata->type5_start &&
		pin <= pctl->devdata->type5_end)
		return (pin - pctl->devdata->type5_start);
	else if (pin >= pctl->devdata->type6_start &&
		pin <= pctl->devdata->type6_end)
		return (pin - pctl->devdata->type6_start);
	else if (pin >= pctl->devdata->type7_start &&
		pin <= pctl->devdata->type7_end)
		return (pin - pctl->devdata->type7_start);

	return 0;
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Set the direction of gpio pin.
 * @param[in]
 *     pctldev: pinctrl_dev pointer, struct pinctrl_dev contains mtk_pinctrl\n
 *     information.
 * @param[in]
 *     range: not used now.
 * @param[in]
 *     offset: gpio pin number.
 * @param[in]
 *     input: whether the direction of gpio pin is input or output.
 * @return
 *     0, set the direction of gpio pin done.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_pmx_gpio_set_direction(struct pinctrl_dev *pctldev,
			struct pinctrl_gpio_range *range, unsigned offset,
			bool input)
{
	unsigned int reg_addr;
	unsigned int bit;
	struct mtk_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);

	if (pctl->devdata->num_base_address == 1) {
		if (mtk_get_port(pctl, offset) == 0x8)
			reg_addr = 0x6 + pctl->devdata->dir_offset;
		else
			reg_addr = mtk_get_port(pctl, offset)
				+ pctl->devdata->dir_offset;
		bit = BIT(offset & 0xf);
	} else {
		reg_addr = mtk_get_port(pctl, offset)
			+ pctl->devdata->dir_offset;
		bit = BIT(offset & 0x1f);
	}

	if (input)
		/* Different SoC has different alignment offset. */
		reg_addr = CLR_ADDR(reg_addr, pctl);
	else
		reg_addr = SET_ADDR(reg_addr, pctl);

	pctl->reg_pinctrl = false;
	regmap_write(mtk_get_regmap(pctl, offset), reg_addr, bit);
	return 0;
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Set the output value of gpio pin.
 * @param[in]
 *     chip: gpio_chip pointer, struct gpio_chip contains mtk_pinctrl\n
 *     information.
 * @param[in]
 *     offset: gpio pin number.
 * @param[in]
 *     value: the output value of gpio pin.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static void mtk_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	unsigned int reg_addr;
	unsigned int bit;
	struct mtk_pinctrl *pctl = dev_get_drvdata(chip->dev);

	if (pctl->devdata->num_base_address == 1) {
		if (mtk_get_port(pctl, offset) == 0x8)
			reg_addr = 0x6 + pctl->devdata->dout_offset;
		else
			reg_addr = mtk_get_port(pctl, offset) +
				pctl->devdata->dout_offset;
		bit = BIT(offset & 0xf);
	} else {
		reg_addr = mtk_get_port(pctl, offset) +
			pctl->devdata->dout_offset;
		bit = BIT(offset & 0x1f);
	}

	if (value)
		reg_addr = SET_ADDR(reg_addr, pctl);
	else
		reg_addr = CLR_ADDR(reg_addr, pctl);

	pctl->reg_pinctrl = false;
	regmap_write(mtk_get_regmap(pctl, offset), reg_addr, bit);
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Set gpio IES control register or SMT control register.
 * @param[in]
 *     pctl: mtk_pinctrl pointer, struct mtk_pinctrl contains gpio\n
 *     pin controller data.
 * @param[in]
 *     pin: the gpio pin number.
 * @param[in]
 *     value: enable or disable gpio IES or SMT.
 * @param[in]
 *     arg: the gpio pin configure parameters.
 * @return
 *     0, set gpio IES and SMT done.
 * @par Boundary case and Limitation
 *     Parameter, arg, is only support\n
 *     PIN_CONFIG_INPUT_ENABLE, PIN_CONFIG_INPUT_SCHMITT_ENABLE.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_pconf_set_ies_smt(struct mtk_pinctrl *pctl, unsigned pin,
		int value, enum pin_config_param arg)
{
	unsigned int reg_addr, offset;
	unsigned int bit;

	bit = BIT(mtk_get_pin(pctl, pin) & 0x1f);

	if (arg == PIN_CONFIG_INPUT_ENABLE)
		offset = pctl->devdata->ies_offset;
	else
		offset = pctl->devdata->smt_offset;

	if (value)
		reg_addr = SET_ADDR(mtk_get_port(pctl,
			mtk_get_pin(pctl, pin)) + offset, pctl);
	else
		reg_addr = CLR_ADDR(mtk_get_port(pctl,
			mtk_get_pin(pctl, pin)) + offset, pctl);

	if (pctl->devdata->num_base_address == 1)
		pctl->reg_pinctrl = false;
	else
	pctl->reg_pinctrl = true;
	regmap_write(mtk_get_regmap(pctl, pin), reg_addr, bit);
	return 0;
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Find the gpio pin driving group.
 * @param[in]
 *     pctl: mtk_pinctrl pointer, struct mtk_pinctrl contains gpio\n
 *     pin controller data.
 * @param[in]
 *     pin: the gpio pin number.
 * @return
 *     pin_drv, gpio pin driving info pointer.
 *     NULL, not find the pin driving group.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static const struct mtk_pin_drv_grp *mtk_find_pin_drv_grp_by_pin(
		struct mtk_pinctrl *pctl,  unsigned long pin) {
	int i;

	for (i = 0; i < pctl->devdata->n_pin_drv_grps; i++) {
		const struct mtk_pin_drv_grp *pin_drv =
				pctl->devdata->pin_drv_grp + i;
		if (pin == pin_drv->pin)
			return pin_drv;
	}

	return NULL;
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Set the gpio pin driving.
 * @param[in]
 *     pctl: mtk_pinctrl pointer, struct mtk_pinctrl contains gpio\n
 *     pin drving group information.
 * @param[in]
 *     pin: the gpio pin number.
 * @param[in]
 *     driving: the driving current value.
 * @return
 *     return value from regmap_update_bits().
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_pconf_set_driving(struct mtk_pinctrl *pctl,
		unsigned int pin, unsigned char driving)
{
	const struct mtk_pin_drv_grp *pin_drv;
	unsigned int val;
	unsigned int bits, mask, shift;
	const struct mtk_drv_group_desc *drv_grp;
	int ret;

	if (pctl->devdata->spec_driving_set) {
		pctl->reg_pinctrl = true;
		ret = pctl->devdata->spec_driving_set(mtk_get_regmap(pctl, pin),
			pin, pctl->devdata->port_align, false, driving);
		if (!ret)
			return 0;
	}

	if (pin >= pctl->devdata->npins)
		return -EINVAL;

	pin_drv = mtk_find_pin_drv_grp_by_pin(pctl, pin);
	if (!pin_drv || pin_drv->grp > pctl->devdata->n_grp_cls)
		return -EINVAL;

	drv_grp = pctl->devdata->grp_desc + pin_drv->grp;
	if (driving >= drv_grp->min_drv && driving <= drv_grp->max_drv
		&& !(driving % drv_grp->step)) {
		val = driving / drv_grp->step - 1;
		bits = drv_grp->high_bit - drv_grp->low_bit + 1;
		mask = BIT(bits) - 1;
		shift = pin_drv->bit + drv_grp->low_bit;
		mask <<= shift;
		val <<= shift;
		pctl->reg_pinctrl = true;
		return regmap_update_bits(mtk_get_regmap(pctl, pin),
				pin_drv->offset, mask, val);
	}

	return -EINVAL;
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Set the specail pin pull up/down.
 * @param[in]
 *     pctl: mtk_pinctrl pointer, struct mtk_pinctrl contains gpio\n
 *     pin drving group information.
 * @param[in]
 *     regmap: gpio base register.
 * @param[in]
 *     pupd_infos: pin pull up/down information struct.
 * @param[in]
 *     info_num: the number of specail pin pull up/down.
 * @param[in]
 *     pin: the gpio pin number.
 * @param[in]
 *     align: the value is 0x4, it is used for reset and set register.
 * @param[in]
 *     isup: enable specail pin pull up.
 * @param[in]
 *     r1r0: the specail pin pull up/down resistance current value.
 * @return
 *     0: set specail pin pull up/down resistance  successfully.
 *     -EINVAL: set specail pin pull up/down resistance  fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
int mtk_pctrl_spec_pull_set_samereg(struct mtk_pinctrl *pctl,
		struct regmap *regmap,
		const struct mtk_pin_spec_pupd_set_samereg *pupd_infos,
		unsigned int info_num, unsigned int pin,
		unsigned char align, bool isup, unsigned int r1r0)
{
	unsigned int i;
	unsigned int reg_pupd, reg_set, reg_rst;
	unsigned int bit_pupd, bit_r0, bit_r1;
	const struct mtk_pin_spec_pupd_set_samereg *spec_pupd_pin;
	bool find = false;

	for (i = 0; i < info_num; i++) {
		if (pin == pupd_infos[i].pin) {
			find = true;
			break;
		}
	}

	if (!find)
		return -EINVAL;

	spec_pupd_pin = pupd_infos + i;
	reg_set = spec_pupd_pin->offset + align;
	reg_rst = spec_pupd_pin->offset + (align << 1);

	if (isup)
		reg_pupd = reg_rst;
	else
		reg_pupd = reg_set;

	bit_pupd = BIT(spec_pupd_pin->pupd_bit);
	regmap_write(mtk_get_regmap(pctl, pin), reg_pupd, bit_pupd);

	bit_r0 = BIT(spec_pupd_pin->r0_bit);
	bit_r1 = BIT(spec_pupd_pin->r1_bit);

	switch (r1r0) {
	case MTK_PUPD_SET_R1R0_00:
		regmap_write(mtk_get_regmap(pctl, pin), reg_rst, bit_r0);
		regmap_write(mtk_get_regmap(pctl, pin), reg_rst, bit_r1);
		break;
	case MTK_PUPD_SET_R1R0_01:
		regmap_write(mtk_get_regmap(pctl, pin), reg_set, bit_r0);
		regmap_write(mtk_get_regmap(pctl, pin), reg_rst, bit_r1);
		break;
	case MTK_PUPD_SET_R1R0_10:
		regmap_write(mtk_get_regmap(pctl, pin), reg_rst, bit_r0);
		regmap_write(mtk_get_regmap(pctl, pin), reg_set, bit_r1);
		break;
	case MTK_PUPD_SET_R1R0_11:
		regmap_write(mtk_get_regmap(pctl, pin), reg_set, bit_r0);
		regmap_write(mtk_get_regmap(pctl, pin), reg_set, bit_r1);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/** @ingroup IP_group_gpio_external_function
 * @par Description
 *     Set the specail i2c pin driving.
 * @param[in]
 *     regmap: gpio base register.
 * @param[in]
 *     driving_infos: drving information struct.
 * @param[in]
 *     info_num: the number of specail pin drving.
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
int mtk_pctrl_spec_driving_set_samereg(struct regmap *regmap,
		const struct mtk_pin_spec_driving_set_samereg *driving_infos,
		unsigned int info_num, unsigned int pin,
		unsigned char align, bool isup, unsigned int d1d0)
{
	unsigned int i;
	unsigned int reg_driving, reg_set, reg_rst;
	unsigned int bit_driving, bit_d0, bit_d1;
	const struct mtk_pin_spec_driving_set_samereg *spec_driving_pin;
	bool find = false;

	for (i = 0; i < info_num; i++) {
		if (pin == driving_infos[i].pin) {
			find = true;
			break;
		}
	}

	if (!find)
		return -EINVAL;

	if (d1d0 < MTK_DRIVING_SET_D1D0_00)
		return -EINVAL;

	spec_driving_pin = driving_infos + i;
	reg_set = spec_driving_pin->offset + align;
	reg_rst = spec_driving_pin->offset + (align << 1);

	if (isup)
		reg_driving = reg_rst;
	else
		reg_driving = reg_set;

	bit_driving = BIT(spec_driving_pin->driv_bit);
	regmap_write(regmap, reg_driving, bit_driving);

	bit_d0 = BIT(spec_driving_pin->d0_bit);
	bit_d1 = BIT(spec_driving_pin->d1_bit);

	switch (d1d0) {
	case MTK_DRIVING_SET_D1D0_00:
		regmap_write(regmap, reg_rst, bit_d0);
		regmap_write(regmap, reg_rst, bit_d1);
		break;
	case MTK_DRIVING_SET_D1D0_01:
		regmap_write(regmap, reg_set, bit_d0);
		regmap_write(regmap, reg_rst, bit_d1);
		break;
	case MTK_DRIVING_SET_D1D0_10:
		regmap_write(regmap, reg_rst, bit_d0);
		regmap_write(regmap, reg_set, bit_d1);
		break;
	case MTK_DRIVING_SET_D1D0_11:
		regmap_write(regmap, reg_set, bit_d0);
		regmap_write(regmap, reg_set, bit_d1);
		break;
	default:
		regmap_write(regmap, reg_set, bit_d0);
		regmap_write(regmap, reg_rst, bit_d1);
		break;
	}

	return 0;
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Set the specail i2c pin pull up/down resistance.
 * @param[in]
 *     regmap: gpio base register.
 * @param[in]
 *     rsel_infos: i2c special pin pull up/down information struct.
 * @param[in]
 *     info_num: the number of specail pin pull up/down.
 * @param[in]
 *     pin: the gpio pin number.
 * @param[in]
 *     align: the value is 0x4, it is used for reset and set register.
 * @param[in]
 *     isup: enable i2c specail pull up resistance.
 * @param[in]
 *     s1s0: the pull up/down resistance current value.
 * @return
 *     0: set i2c specail pull up/down resistance successfully.
 *     -EINVAL: set specail pull up/down resistance fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
int mtk_pctrl_spec_rsel_set_samereg(struct regmap *regmap,
		const struct mtk_pin_spec_rsel_set_samereg *rsel_infos,
		unsigned int info_num, unsigned int pin,
		unsigned char align, bool isup, unsigned int s1s0)
{
	unsigned int i;
	unsigned int reg_rsel, reg_set, reg_rst;
	unsigned int bit_rsel, bit_s0, bit_s1;
	const struct mtk_pin_spec_rsel_set_samereg *spec_rsel_pin;
	bool find = false;

	for (i = 0; i < info_num; i++) {
		if (pin == rsel_infos[i].pin) {
			find = true;
			break;
		}
	}

	if (!find)
		return -EINVAL;

	spec_rsel_pin = rsel_infos + i;
	reg_set = spec_rsel_pin->offset + align;
	reg_rst = spec_rsel_pin->offset + (align << 1);

	if (isup)
		reg_rsel = reg_rst;
	else
		reg_rsel = reg_set;

	bit_rsel = BIT(spec_rsel_pin->rsel_bit);

	bit_s0 = BIT(spec_rsel_pin->s0_bit);
	bit_s1 = BIT(spec_rsel_pin->s1_bit);

	switch (s1s0) {
	case MTK_RSEL_SET_S1S0_00:
		regmap_write(regmap, reg_rst, bit_s0);
		regmap_write(regmap, reg_rst, bit_s1);
		break;
	case MTK_RSEL_SET_S1S0_01:
		regmap_write(regmap, reg_set, bit_s0);
		regmap_write(regmap, reg_rst, bit_s1);
		break;
	case MTK_RSEL_SET_S1S0_10:
		regmap_write(regmap, reg_rst, bit_s0);
		regmap_write(regmap, reg_set, bit_s1);
		break;
	case MTK_RSEL_SET_S1S0_11:
		regmap_write(regmap, reg_set, bit_s0);
		regmap_write(regmap, reg_set, bit_s1);
		break;
	default:
		regmap_write(regmap, reg_set, bit_s0);
		regmap_write(regmap, reg_set, bit_s1);
		break;
	}

	return 0;
}


/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Set the gpio pin pull up or pull down.
 * @param[in]
 *     pctl: mtk_pinctrl pointer, struct mtk_pinctrl contains gpio\n
 *     pin controller data.
 * @param[in]
 *     pin: the gpio pin number.
 * @param[in]
 *     enable: whether enable or disable gpio pull up and pull down.
 * @param[in]
 *     arg: for generic pull config.
 * @return
 *     0, set the gpio pull up or pull down done.
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_pconf_set_pull_select(struct mtk_pinctrl *pctl,
		unsigned int pin, bool enable, bool isup, unsigned int arg)
{
	unsigned int bit;
	unsigned int reg_pullen, reg_pullsel;
	int ret;

	if (pctl->devdata->num_base_address == 1)
		pctl->reg_pinctrl = false;
	else
		pctl->reg_pinctrl = true;
	if (pctl->devdata->spec_pull_set) {
		ret = pctl->devdata->spec_pull_set(pctl,
			mtk_get_regmap(pctl, pin), pin,
			pctl->devdata->port_align, isup, arg);
		if (!ret)
			return 0;
	}

	if (pctl->devdata->spec_rsel_set) {
		ret = pctl->devdata->spec_rsel_set(mtk_get_regmap(pctl, pin),
			pin, pctl->devdata->port_align, isup, arg);
	}

	if (pctl->devdata->num_base_address == 1) {
		if (mtk_get_port(pctl, pin) == 0x8) {
			if (enable)
				reg_pullen = SET_ADDR(0x6 +
					pctl->devdata->pullen_offset, pctl);
			else
				reg_pullen = CLR_ADDR(0x6 +
					pctl->devdata->pullen_offset, pctl);

			if (isup)
				reg_pullsel = SET_ADDR(0x6 +
					pctl->devdata->pullsel_offset, pctl);
			else
				reg_pullsel = CLR_ADDR(0x6 +
					pctl->devdata->pullsel_offset, pctl);
		} else {
			if (enable)
				reg_pullen = SET_ADDR(mtk_get_port(pctl,
					mtk_get_pin(pctl, pin)) +
					pctl->devdata->pullen_offset, pctl);
			else
				reg_pullen = CLR_ADDR(mtk_get_port(pctl,
					mtk_get_pin(pctl, pin)) +
					pctl->devdata->pullen_offset, pctl);

			if (isup)
				reg_pullsel = SET_ADDR(mtk_get_port(pctl,
					mtk_get_pin(pctl, pin)) +
					pctl->devdata->pullsel_offset, pctl);
			else
				reg_pullsel = CLR_ADDR(mtk_get_port(pctl,
					mtk_get_pin(pctl, pin)) +
					pctl->devdata->pullsel_offset, pctl);
		}
		bit = BIT(pin & 0xf);
	} else {
		bit = BIT(mtk_get_pin(pctl, pin) & 0x1f);
		if (enable)
			reg_pullen = SET_ADDR(mtk_get_port(pctl,
				mtk_get_pin(pctl, pin)) +
				pctl->devdata->pullen_offset, pctl);
		else
			reg_pullen = CLR_ADDR(mtk_get_port(pctl,
				mtk_get_pin(pctl, pin)) +
				pctl->devdata->pullen_offset, pctl);

		if (isup)
			reg_pullsel = SET_ADDR(mtk_get_port(pctl,
				mtk_get_pin(pctl, pin)) +
				pctl->devdata->pullsel_offset, pctl);
		else
			reg_pullsel = CLR_ADDR(mtk_get_port(pctl,
				mtk_get_pin(pctl, pin)) +
				pctl->devdata->pullsel_offset, pctl);
	}

	regmap_write(mtk_get_regmap(pctl, pin), reg_pullen, bit);
	regmap_write(mtk_get_regmap(pctl, pin), reg_pullsel, bit);
	return 0;
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Configure the gpio pin.
 * @param[in]
 *     pctldev: pinctrl_dev pointer, struct pinctrl_dev contains mtk_pinctrl\n
 *     information.
 * @param[in]
 *     pin: the gpio pin number.
 * @param[in]
 *     param: pin configure parameter type.
 * @param[in]
 *     arg: pin configure argument.
 * @return
 *     return value from one of the following function:\n
 *     mtk_pconf_set_pull_select(), mtk_pconf_set_ies_smt()\n
 *     mtk_pmx_gpio_set_direction(), mtk_pconf_set_driving().
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     Parameter, param, is only support
 *     PIN_CONFIG_BIAS_DISABLE, PIN_CONFIG_BIAS_PULL_UP,
 *     PIN_CONFIG_BIAS_PULL_DOWN, PIN_CONFIG_INPUT_ENABLE,
 *     PIN_CONFIG_OUTPUT, PIN_CONFIG_INPUT_SCHMITT_ENABLE,
 *     PIN_CONFIG_DRIVE_STRENGTH.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_pconf_parse_conf(struct pinctrl_dev *pctldev,
		unsigned int pin, enum pin_config_param param,
		enum pin_config_param arg)
{
	int ret = 0;
	struct mtk_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);

	switch (param) {
	case PIN_CONFIG_BIAS_DISABLE:
		ret = mtk_pconf_set_pull_select(pctl, pin, false, false, arg);
		break;
	case PIN_CONFIG_BIAS_PULL_UP:
		ret = mtk_pconf_set_pull_select(pctl, pin, true, true, arg);
		break;
	case PIN_CONFIG_BIAS_PULL_DOWN:
		ret = mtk_pconf_set_pull_select(pctl, pin, true, false, arg);
		break;
	case PIN_CONFIG_INPUT_ENABLE:
		ret = mtk_pconf_set_ies_smt(pctl, pin, arg, param);
		break;
	case PIN_CONFIG_OUTPUT:
		mtk_gpio_set(pctl->chip, pin, arg);
		ret = mtk_pmx_gpio_set_direction(pctldev, NULL, pin, false);
		break;
	case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
		ret = mtk_pconf_set_ies_smt(pctl, pin, arg, param);
		break;
	case PIN_CONFIG_DRIVE_STRENGTH:
		ret = mtk_pconf_set_driving(pctl, pin, arg);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Get the gpio pin group configure.
 * @param[in]
 *     pctldev: pinctrl_dev pointer, struct pinctrl_dev contains mtk_pinctrl\n
 *     information.
 * @param[in]
 *     group: the gpio pin group number.
 * @param[out]
 *     config: a pointer points to gpio pin group configure.
 * @param[in]
 *     arg: pin configure parameter.
 * @return
 *     0, get the gpio pin group configure done.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_pconf_group_get(struct pinctrl_dev *pctldev,
				 unsigned group,
				 unsigned long *config)
{
	struct mtk_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);

	*config = pctl->groups[group].config;

	return 0;
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Set the gpio pin group configure.
 * @param[in]
 *     pctldev: pinctrl_dev pointer, struct pinctrl_dev contains mtk_pinctrl\n
 *     information.
 * @param[in]
 *     group: the gpio pin group number.
 * @param[in]
 *     configs: a pointer points to gpio pin group configs.
 * @param[in]
 *     num_configs: the number of gpio pin group configs.
 * @return
 *     0, get the gpio pin group configure done.\n
 *     error code from mtk_pconf_parse_conf().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_pconf_group_set(struct pinctrl_dev *pctldev, unsigned group,
				 unsigned long *configs, unsigned num_configs)
{
	struct mtk_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	struct mtk_pinctrl_group *g = &pctl->groups[group];
	int i, ret;

	for (i = 0; i < num_configs; i++) {
		ret = mtk_pconf_parse_conf(pctldev, g->pin,
			pinconf_to_config_param(configs[i]),
			pinconf_to_config_argument(configs[i]));
		if (ret < 0)
			return ret;

		g->config = configs[i];
	}

	return 0;
}

/**
 * @brief Define pin config operations.
 */
static const struct pinconf_ops mtk_pconf_ops = {
	.pin_config_group_get	= mtk_pconf_group_get,
	.pin_config_group_set	= mtk_pconf_group_set,
};

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Find the gpio pin group.
 * @param[in]
 *     pctl: mtk_pinctrl pointer, struct mtk_pinctrl contains gpio\n
 *     pin controller data.
 * @param[in]
 *     pin: the gpio pin number.
 * @return
 *     grp, the gpio pin group pointer.
 *     NULL, not find the gpio pin group.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static struct mtk_pinctrl_group *
mtk_pctrl_find_group_by_pin(struct mtk_pinctrl *pctl, u32 pin)
{
	int i;

	for (i = 0; i < pctl->ngroups; i++) {
		struct mtk_pinctrl_group *grp = pctl->groups + i;

		if (grp->pin == pin)
			return grp;
	}

	return NULL;
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Find the gpio pin funciton.
 * @param[in]
 *     pctl: mtk_pinctrl pointer, struct mtk_pinctrl contains gpio\n
 *     pin group information.
 * @param[in]
 *     pin_num: the gpio pin number.
 * @param[in]
 *     fnum: the gpio pin function number.
 * @return
 *     func, the gpio pin function pointer.
 *     NULL, not find the gpio pin function.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static const struct mtk_desc_function *mtk_pctrl_find_function_by_pin(
		struct mtk_pinctrl *pctl, u32 pin_num, u32 fnum)
{
	const struct mtk_desc_pin *pin = pctl->devdata->pins + pin_num;
	const struct mtk_desc_function *func = pin->functions;

	while (func && func->name) {
		if (func->muxval == fnum)
			return func;
		func++;
	}

	return NULL;
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Check the gpio pin function whether valid.
 * @param[in]
 *     pctl: mtk_pinctrl pointer, struct mtk_pinctrl contains gpio\n
 *     pin controller data.
 * @param[in]
 *     pin_num: the gpio pin number.
 * @param[in]
 *     fnum: the gpio pin function number.
 * @return
 *     true, the gpio pin function is valid.
 *     false, the gpio pin function is not valid.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static bool mtk_pctrl_is_function_valid(struct mtk_pinctrl *pctl,
		u32 pin_num, u32 fnum)
{
	int i;

	for (i = 0; i < pctl->devdata->npins; i++) {
		const struct mtk_desc_pin *pin = pctl->devdata->pins + i;

		if (pin->pin.number == pin_num) {
			const struct mtk_desc_function *func =
					pin->functions;

			while (func && func->name) {
				if (func->muxval == fnum)
					return true;
				func++;
			}

			break;
		}
	}

	return false;
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Build the gpio pin map info.
 * @param[in]
 *     pctl: mtk_pinctrl pointer, struct mtk_pinctrl contains gpio\n
 *     pin controller data.
 * @param[in]
 *     pin: the gpio pin number.
 * @param[in]
 *     fnum: the gpio pin function number.
 * @param[in]
 *     grp: point to the gpio pin group info.
 * @param[out]
 *     map: point to the gpio pin map info.
 * @param[in]
 *     reserved_maps: point to the reserved gpio pin map index.
 * @param[out]
 *     num_maps: point to the current gpio pin map index.
 * @return
 *     0, build the gpio pin map info done.
 *     -ENOSPC, wrong parameter.
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     1. If the current gpio pin map index is equal to the reserved gpio pin\n
 *     map index, return -ENOSPC.
 *     2. If mtk_pctrl_is_function_valid() return false, return -EINVAL.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_pctrl_dt_node_to_map_func(struct mtk_pinctrl *pctl,
		u32 pin, u32 fnum, struct mtk_pinctrl_group *grp,
		struct pinctrl_map **map, unsigned *reserved_maps,
		unsigned *num_maps)
{
	bool ret;

	if (*num_maps == *reserved_maps)
		return -ENOSPC;

	(*map)[*num_maps].type = PIN_MAP_TYPE_MUX_GROUP;
	(*map)[*num_maps].data.mux.group = grp->name;

	ret = mtk_pctrl_is_function_valid(pctl, pin, fnum);
	if (!ret) {
		dev_err(pctl->dev, "invalid function %d on pin %d .\n",
				fnum, pin);
		return -EINVAL;
	}

	(*map)[*num_maps].data.mux.function = mtk_gpio_functions[fnum];
	(*num_maps)++;

	return 0;
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Parse a device node, and then build the gpio pin map info.
 * @param[in]
 *     pctldev: pinctrl_dev pointer, struct pinctrl_dev contains mtk_pinctrl\n
 *     information.
 * @param[in]
 *     node: device tree node pointer.
 * @param[out]
 *     map: point to the gpio pin map info.
 * @param[in]
 *     reserved_maps: point to the reserved gpio pin map index.
 * @param[out]
 *     num_maps: point to the current gpio pin map index.
 * @return
 *     error code from pinctrl_utils_reserve_map().
 *     error code from of_property_read_u32_index().
 *     error code from mtk_pctrl_dt_node_to_map_func().
 *     error code from pinctrl_utils_add_map_configs().
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     1. If pinctrl_utils_reserve_map()or of_property_read_u32_index()\n
 *     or mtk_pctrl_dt_node_to_map_func()or pinctrl_utils_add_map_configs()\n
 *     fails, return the error code from these functions.
 *     2. If wrong paremeter, return -EINVAL.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_pctrl_dt_subnode_to_map(struct pinctrl_dev *pctldev,
				      struct device_node *node,
				      struct pinctrl_map **map,
				      unsigned *reserved_maps,
				      unsigned *num_maps)
{
	struct property *pins;
	u32 pinfunc, pin, func;
	int num_pins, num_funcs, maps_per_pin;
	unsigned long *configs;
	unsigned int num_configs;
	bool has_config = 0;
	int i, err;
	unsigned reserve = 0;
	struct mtk_pinctrl_group *grp;
	struct mtk_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);

	pins = of_find_property(node, "pinmux", NULL);
	if (!pins) {
		dev_err(pctl->dev, "missing pins property in node %s .\n",
				node->name);
		return -EINVAL;
	}

	err = pinconf_generic_parse_dt_config(node, pctldev, &configs,
		&num_configs);
	if (num_configs)
		has_config = 1;

	num_pins = pins->length / sizeof(u32);
	num_funcs = num_pins;
	maps_per_pin = 0;
	if (num_funcs)
		maps_per_pin++;
	if (has_config && num_pins >= 1)
		maps_per_pin++;

	if (!num_pins || !maps_per_pin)
		return -EINVAL;

	reserve = num_pins * maps_per_pin;

	err = pinctrl_utils_reserve_map(pctldev, map,
			reserved_maps, num_maps, reserve);
	if (err < 0)
		goto fail;

	for (i = 0; i < num_pins; i++) {
		err = of_property_read_u32_index(node, "pinmux",
				i, &pinfunc);
		if (err)
			goto fail;

		pin = MTK_GET_PIN_NO(pinfunc);
		func = MTK_GET_PIN_FUNC(pinfunc);

		if (pin >= pctl->devdata->npins ||
				func >= ARRAY_SIZE(mtk_gpio_functions)) {
			dev_err(pctl->dev, "invalid pins value.\n");
			err = -EINVAL;
			goto fail;
		}

		grp = mtk_pctrl_find_group_by_pin(pctl, pin);
		if (!grp) {
			dev_err(pctl->dev, "unable to match pin %d to group\n",
					pin);
			return -EINVAL;
		}

		err = mtk_pctrl_dt_node_to_map_func(pctl, pin, func, grp, map,
				reserved_maps, num_maps);
		if (err < 0)
			goto fail;

		if (has_config) {
			err = pinctrl_utils_add_map_configs(pctldev, map,
					reserved_maps, num_maps, grp->name,
					configs, num_configs,
					PIN_MAP_TYPE_CONFIGS_GROUP);
			if (err < 0)
				goto fail;
		}
	}

	kfree(configs);
	return 0;

fail:
	kfree(configs);
	return err;
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Parse all of device nodes, and then build gpio pins map info.
 * @param[in]
 *     pctldev: pinctrl_dev pointer, struct pinctrl_dev contains mtk_pinctrl\n
 *     information.
 * @param[in]
 *     np_config: device tree nodes pointer.
 * @param[out]
 *     map: point to the gpio pin map info.
 * @param[out]
 *     num_maps: point to the gpio pin map index.
 * @return
 *     0, build gpio pins map info done.
 *     error code from mtk_pctrl_dt_subnode_to_map().
 * @par Boundary case and Limitation
 *     If mtk_pctrl_dt_subnode_to_map() fails, return the error code.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_pctrl_dt_node_to_map(struct pinctrl_dev *pctldev,
				 struct device_node *np_config,
				 struct pinctrl_map **map, unsigned *num_maps)
{
	struct device_node *np;
	unsigned reserved_maps;
	int ret;

	*map = NULL;
	*num_maps = 0;
	reserved_maps = 0;

	for_each_child_of_node(np_config, np) {
		ret = mtk_pctrl_dt_subnode_to_map(pctldev, np, map,
				&reserved_maps, num_maps);
		if (ret < 0) {
			pinctrl_utils_dt_free_map(pctldev, *map, *num_maps);
			return ret;
		}
	}

	return 0;
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Get gpio pin groups count.
 * @param[in]
 *     pctldev: pinctrl_dev pointer, struct pinctrl_dev contains mtk_pinctrl\n
 *     information.
 * @return
 *     pctl->ngroups, gpio pin groups count.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_pctrl_get_groups_count(struct pinctrl_dev *pctldev)
{
	struct mtk_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);

	return pctl->ngroups;
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Get the gpio pin group name.
 * @param[in]
 *     pctldev: pinctrl_dev pointer, struct pinctrl_dev contains mtk_pinctrl\n
 *     information.
 * @param[in]
 *     group: the gpio pin group index.
 * @return
 *     pctl->groups[group].name, the gpio pin group name.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static const char *mtk_pctrl_get_group_name(struct pinctrl_dev *pctldev,
					      unsigned group)
{
	struct mtk_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);

	return pctl->groups[group].name;
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Get gpio pins info from a gpio pin group.
 * @param[in]
 *     pctldev: pinctrl_dev pointer, struct pinctrl_dev contains mtk_pinctrl\n
 *     information.
 * @param[in]
 *     group: the gpio pin group index.
 * @param[out]
 *     pins: the gpio pins info.
 * @param[out]
 *     num_pins: the number of gpio pins.
 * @return
 *     0, get gpio pins info from a gpio pin group done.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_pctrl_get_group_pins(struct pinctrl_dev *pctldev,
				      unsigned group,
				      const unsigned **pins,
				      unsigned *num_pins)
{
	struct mtk_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);

	*pins = (unsigned *)&pctl->groups[group].pin;
	*num_pins = 1;

	return 0;
}

/** @ingroup IP_group_gpio_internal_struct
 * @brief Register pinctrl operations.
 */
static const struct pinctrl_ops mtk_pctrl_ops = {
	.dt_node_to_map		= mtk_pctrl_dt_node_to_map,
	.dt_free_map		= pinctrl_utils_dt_free_map,
	.get_groups_count	= mtk_pctrl_get_groups_count,
	.get_group_name		= mtk_pctrl_get_group_name,
	.get_group_pins		= mtk_pctrl_get_group_pins,
};

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Get gpio pin functions count.
 * @param[in]
 *     pctldev: pinctrl_dev pointer, struct pinctrl_dev contains mtk_pinctrl\n
 *     information.
 * @return
 *     ARRAY_SIZE(mtk_gpio_functions), gpio pin functions count.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_pmx_get_funcs_cnt(struct pinctrl_dev *pctldev)
{
	return ARRAY_SIZE(mtk_gpio_functions);
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Get a gpio pin function.
 * @param[in]
 *     pctldev: pinctrl_dev pointer, struct pinctrl_dev contains mtk_pinctrl\n
 *     information.
 * @param[in]
 *     selector: the gpio pin function index.
 * @return
 *     mtk_gpio_functions[selector], the gpio pin function.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static const char *mtk_pmx_get_func_name(struct pinctrl_dev *pctldev,
					   unsigned selector)
{
	return mtk_gpio_functions[selector];
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Get gpio pins groups info.
 * @param[in]
 *     pctldev: pinctrl_dev pointer, struct pinctrl_dev contains mtk_pinctrl\n
 *     information.
 * @param[in]
 *     function: the gpio pin function index.
 * @param[out]
 *     groups: point to gpio pins groups.
 * @param[out]
 *     num_groups: point to the number of gpio pins groups.
 * @return
 *     0, get gpio pins groups info done.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_pmx_get_func_groups(struct pinctrl_dev *pctldev,
				     unsigned function,
				     const char * const **groups,
				     unsigned * const num_groups)
{
	struct mtk_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);

	*groups = pctl->grp_names;
	*num_groups = pctl->ngroups;

	return 0;
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Set gpio mode.
 * @param[out]
 *     pctldev: pinctrl_dev pointer, struct pinctrl_dev contains mtk_pinctrl\n
 *     information.
 * @param[in]
 *     pin: the gpio pin number.
 * @param[in]
 *     mode: the gpio pin mode.
 * @return
 *     return value from regmap_update_bits().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_pmx_set_mode(struct pinctrl_dev *pctldev,
		unsigned long pin, unsigned long mode)
{
	unsigned int reg_addr;
	unsigned char bit;
	unsigned int val;
	struct mtk_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	unsigned int mask = (1L << pctl->devdata->gpio_mode_bits) - 1;

	if (pctl->devdata->num_base_address == 1) {
		reg_addr = ((pin / pctl->devdata->max_gpio_mode_per_reg) * 0x6)
			+ pctl->devdata->pinmux_offset;
	} else {
		reg_addr = ((pin / pctl->devdata->max_gpio_mode_per_reg)
				<< pctl->devdata->port_shf)
				+ pctl->devdata->pinmux_offset;
	}

	mode &= mask;
	bit = pin % pctl->devdata->max_gpio_mode_per_reg;
	mask <<= (pctl->devdata->gpio_mode_bits * bit);
	val = (mode << (pctl->devdata->gpio_mode_bits * bit));
	pctl->reg_pinctrl = false;
	return regmap_update_bits(mtk_get_regmap(pctl, pin),
			reg_addr, mask, val);
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Find the gpio pin info by eint number.
 * @param[in]
 *     pctl: mtk_pinctrl pointer, struct mtk_pinctrl contains gpio\n
 *     pin controller data.
 * @param[in]
 *     eint_num: the eint number.
 * @return
 *     pin, the gpio pin info pointer.
 *     NULL, not find the pin info.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static const struct mtk_desc_pin *
mtk_find_pin_by_eint_num(struct mtk_pinctrl *pctl, unsigned int eint_num)
{
	int i;
	const struct mtk_desc_pin *pin;

	for (i = 0; i < pctl->devdata->npins; i++) {
		pin = pctl->devdata->pins + i;
		if (pin->eint.eintnum == eint_num)
			return pin;
	}

	return NULL;
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Set the gpio pinumux function.
 * @param[out]
 *     pctldev: pinctrl_dev pointer, struct pinctrl_dev contains mtk_pinctrl\n
 *     information.
 * @param[in]
 *     function: the gpio pinmux function.
 * @param[in]
 *     group: the gpio pin group.
 * @return
 *     0, set the gpio pinmux function done.
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If mtk_pctrl_is_function_valid() or mtk_pctrl_find_function_by_pin()\n
 *     fails, return -EINVAL.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_pmx_set_mux(struct pinctrl_dev *pctldev,
			    unsigned function,
			    unsigned group)
{
	bool ret;
	const struct mtk_desc_function *desc;
	struct mtk_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	struct mtk_pinctrl_group *g = pctl->groups + group;

	ret = mtk_pctrl_is_function_valid(pctl, g->pin, function);
	if (!ret) {
		dev_err(pctl->dev, "invalid function %d on group %d .\n",
				function, group);
		return -EINVAL;
	}

	desc = mtk_pctrl_find_function_by_pin(pctl, g->pin, function);
	if (!desc)
		return -EINVAL;
	mtk_pmx_set_mode(pctldev, g->pin, desc->muxval);
	return 0;
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Find the gpio mode.
 * @param[in]
 *     pctl: mtk_pinctrl pointer, struct mtk_pinctrl contains gpio\n
 *     pin controller data.
 * @param[in]
 *     offset: the gpio pin number.
 * @return
 *     func->muxval, gpio mode.
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If not find the gpio mode, return -EINVAL.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_pmx_find_gpio_mode(struct mtk_pinctrl *pctl,
				unsigned offset)
{
	const struct mtk_desc_pin *pin = pctl->devdata->pins + offset;
	const struct mtk_desc_function *func = pin->functions;

	while (func && func->name) {
		if (!strncmp(func->name, GPIO_MODE_PREFIX,
			sizeof(GPIO_MODE_PREFIX)-1))
			return func->muxval;
		func++;
	}
	return -EINVAL;
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Enable the gpio output and input.
 * @param[out]
 *     pctldev: pinctrl_dev pointer, struct pinctrl_dev contains mtk_pinctrl\n
 *     information.
 * @param[in]
 *     range: gpio range pointer.
 * @param[in]
 *     offset: the gpio pin number.
 * @return
 *     0, enable the gpio output and input done.
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If not find the gpio mode, return -EINVAL.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_pmx_gpio_request_enable(struct pinctrl_dev *pctldev,
				    struct pinctrl_gpio_range *range,
				    unsigned offset)
{
	int muxval;
	struct mtk_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);

	muxval = mtk_pmx_find_gpio_mode(pctl, offset);

	if (muxval < 0) {
		dev_err(pctl->dev, "invalid gpio pin %d.\n", offset);
		return -EINVAL;
	}

	mtk_pmx_set_mode(pctldev, offset, muxval);
	mtk_pconf_set_ies_smt(pctl, offset, 1, PIN_CONFIG_INPUT_ENABLE);

	return 0;
}

/** @ingroup IP_group_gpio_internal_struct
 * @brief Register gpio pinmux operations.
 */
static const struct pinmux_ops mtk_pmx_ops = {
	.get_functions_count	= mtk_pmx_get_funcs_cnt,
	.get_function_name	= mtk_pmx_get_func_name,
	.get_function_groups	= mtk_pmx_get_func_groups,
	.set_mux		= mtk_pmx_set_mux,
	.gpio_set_direction	= mtk_pmx_gpio_set_direction,
	.gpio_request_enable	= mtk_pmx_gpio_request_enable,
};

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Set the gpio input.
 * @param[in]
 *     chip: gpio_chip pointer.
 * @param[in]
 *     offset: the gpio pin number.
 * @return
 *     return value from pinctrl_gpio_direction_input().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_gpio_direction_input(struct gpio_chip *chip,
					unsigned offset)
{
	return pinctrl_gpio_direction_input(chip->base + offset);
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Set the gpio output and output value.
 * @param[in]
 *     chip: gpio_chip pointer.
 * @param[in]
 *     offset: the gpio pin number.
 * @param[in]
 *     value: the value what gpio output.
 * @return
 *     return value from pinctrl_gpio_direction_output().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_gpio_direction_output(struct gpio_chip *chip,
					unsigned offset, int value)
{
	mtk_gpio_set(chip, offset, value);
	return pinctrl_gpio_direction_output(chip->base + offset);
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Get the gpio direction.
 * @param[in]
 *     chip: gpio_chip pointer.
 * @param[in]
 *     offset: the gpio pin number.
 * @return
 *     !(read_val & bit), gpio direction value.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_gpio_get_direction(struct gpio_chip *chip, unsigned offset)
{
	unsigned int reg_addr;
	unsigned int bit;
	unsigned int read_val = 0;

	struct mtk_pinctrl *pctl = dev_get_drvdata(chip->dev);

	if (pctl->devdata->num_base_address == 1) {
		if (mtk_get_port(pctl, offset) == 0x8)
			reg_addr = 0x6 + pctl->devdata->dir_offset;
		else
			reg_addr = mtk_get_port(pctl, offset)
				+ pctl->devdata->dir_offset;
		bit = BIT(offset & 0xf);
	} else {
		reg_addr =  mtk_get_port(pctl, offset)
			+ pctl->devdata->dir_offset;
		bit = BIT(offset & 0x1f);
	}

	pctl->reg_pinctrl = false;
	regmap_read(pctl->regmap0, reg_addr, &read_val);
	return !(read_val & bit);
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Get the gpio input value.
 * @param[in]
 *     chip: gpio_chip pointer.
 * @param[in]
 *     offset: the gpio pin number.
 * @return
 *     !!(read_val & bit), gpio input value.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	unsigned int reg_addr;
	unsigned int bit;
	unsigned int read_val = 0;
	struct mtk_pinctrl *pctl = dev_get_drvdata(chip->dev);

	reg_addr = mtk_get_port(pctl, offset) +
		pctl->devdata->din_offset;

	bit = BIT(offset & 0x1f);
	regmap_read(pctl->regmap0, reg_addr, &read_val);
	return !!(read_val & bit);
}

static int mtk_gpio_to_irq(struct gpio_chip *chip, unsigned offset)
{
	const struct mtk_desc_pin *pin;
	struct mtk_pinctrl *pctl = dev_get_drvdata(chip->dev);
	int irq;

	pin = pctl->devdata->pins + offset;
	if (pin->eint.eintnum == NO_EINT_SUPPORT)
		return -EINVAL;

	irq = irq_find_mapping(pctl->domain, pin->eint.eintnum);
	if (!irq)
		return -EINVAL;

	return irq;
}

static int mtk_pinctrl_irq_request_resources(struct irq_data *d)
{
	struct mtk_pinctrl *pctl = irq_data_get_irq_chip_data(d);
	const struct mtk_desc_pin *pin;
	int ret;

	pin = mtk_find_pin_by_eint_num(pctl, d->hwirq);

	if (!pin) {
		dev_err(pctl->dev, "Can not find pin\n");
		return -EINVAL;
	}

	ret = gpiochip_lock_as_irq(pctl->chip, pin->pin.number);
	if (ret) {
		dev_err(pctl->dev, "unable to lock HW IRQ %lu for IRQ\n",
			irqd_to_hwirq(d));
		return ret;
	}

	/* set mux to INT mode */
	mtk_pmx_set_mode(pctl->pctl_dev, pin->pin.number, pin->eint.eintmux);
	/* set gpio direction to input */
	mtk_pmx_gpio_set_direction(pctl->pctl_dev, NULL, pin->pin.number, true);
	/* set input-enable */
	mtk_pconf_set_ies_smt(pctl, pin->pin.number, 1,
				    PIN_CONFIG_INPUT_ENABLE);

	return 0;
}

static void mtk_pinctrl_irq_release_resources(struct irq_data *d)
{
	struct mtk_pinctrl *pctl = irq_data_get_irq_chip_data(d);
	const struct mtk_desc_pin *pin;

	pin = mtk_find_pin_by_eint_num(pctl, d->hwirq);

	if (!pin) {
		dev_err(pctl->dev, "Can not find pin\n");
		return;
	}

	gpiochip_unlock_as_irq(pctl->chip, pin->pin.number);
}

/** @ingroup IP_group_eint_internal_function
 * @par Description
 *     Get the register offset.
 * @param[in]
 *     pctl: mtk_pinctrl pointer, struct mtk_pinctrl contains gpio\n
 *     pin controller data.
 * @param[in]
 *     eint_num: the eint number.
 * @return
 *     the register offset.
 * @par Boundary case and Limitation
 *     input eint_num must < ap_number.
 * @par Error case and Error handling
 *     if eint number not valid, consider it equal ap_number \n
 *     and return its offset.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static void __iomem *mtk_eint_get_offset(struct mtk_pinctrl *pctl,
	unsigned int eint_num, unsigned int offset)
{
	unsigned int eint_base = 0;
	void __iomem *reg;

	if (eint_num >= pctl->devdata->ap_num)
		eint_base = pctl->devdata->ap_num;

	reg = pctl->eint_reg_base + offset + ((eint_num - eint_base) / 32) * 4;

	return reg;
}

/** @ingroup IP_group_eint_internal_function
 * @par Description
 *     check if the eint can enable debounce.
 * @param[in]
 *     pctl: mtk_pinctrl pointer, struct mtk_pinctrl contains gpio\n
 *     pin controller data.
 * @param[in]
 *     eint_num: the eint number.
 * @return
 *     0, this eint does not support debounce and can not enable it.\n
 *     1, this eint supports debounce and can enable it.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static unsigned int mtk_eint_can_en_debounce(struct mtk_pinctrl *pctl,
	unsigned int eint_num)
{
	unsigned int sens;
	unsigned int bit = BIT(eint_num % 32);
	const struct mtk_eint_offsets *eint_offsets =
		&pctl->devdata->eint_offsets;

	void __iomem *reg = mtk_eint_get_offset(pctl, eint_num,
			eint_offsets->sens);

	if (readl(reg) & bit)
		sens = MT_LEVEL_SENSITIVE;
	else
		sens = MT_EDGE_SENSITIVE;

	if ((eint_num < pctl->devdata->db_cnt) && (sens != MT_EDGE_SENSITIVE))
		return 1;
	else
		return 0;
}

/** @ingroup IP_group_eint_internal_function
 * @par Description
 *     get eint mask status.
 * @param[in]
 *     pctl: mtk_pinctrl pointer, struct mtk_pinctrl contains gpio\n
 *     pin controller data.
 * @param[in]
 *     eint_num: the eint number.
 * @return
 *     0, this eint is not masked.\n
 *     1, this eint is masked.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static unsigned int mtk_eint_get_mask(struct mtk_pinctrl *pctl,
	unsigned int eint_num)
{
	unsigned int bit = BIT(eint_num % 32);
	const struct mtk_eint_offsets *eint_offsets =
		&pctl->devdata->eint_offsets;

	void __iomem *reg = mtk_eint_get_offset(pctl, eint_num,
			eint_offsets->mask);

	return !!(readl(reg) & bit);
}

/** @ingroup IP_group_eint_internal_function
 * @par Description
 *     This function is used to support both edge(Rising & Falling).\n
 *     Flip eint trigger type, from rising edge to falling edge or reverse.
 * @param[in]
 *     pctl: mtk_pinctrl pointer, struct mtk_pinctrl contains eint\n
 *     controller data.
 * @param[in]
 *     hwirq: hardware irq, can be condsider as eint number.
 * @return
 *     0, the input to hwirq(eint) is low level.\n
 *     1, the input ot hwirq(eint) is high level.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_eint_flip_edge(struct mtk_pinctrl *pctl, int hwirq)
{
	int start_level, curr_level;
	unsigned int reg_offset;
	const struct mtk_eint_offsets *eint_offsets =
						&(pctl->devdata->eint_offsets);
	u32 mask = BIT(hwirq & 0x1f);
	u32 port = (hwirq >> 5) & eint_offsets->port_mask;
	void __iomem *reg = pctl->eint_reg_base + (port << 2);
	const struct mtk_desc_pin *pin;

	pin = mtk_find_pin_by_eint_num(pctl, hwirq);
	curr_level = mtk_gpio_get(pctl->chip, pin->pin.number);
	do {
		start_level = curr_level;
		if (start_level)
			reg_offset = eint_offsets->pol_clr;
		else
			reg_offset = eint_offsets->pol_set;
		writel(mask, reg + reg_offset);

		curr_level = mtk_gpio_get(pctl->chip, pin->pin.number);
	} while (start_level != curr_level);

	return start_level;
}

/** @ingroup IP_group_eint_internal_function
 * @par Description
 *     This function register into Linux interrupt framework to mask
 *     the eint and eint controller stop forward masked eint.
 * @param[in]
 *     d: structure of irq_data for getting irq chip information.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static void mtk_eint_mask(struct irq_data *d)
{
	struct mtk_pinctrl *pctl = irq_data_get_irq_chip_data(d);
	const struct mtk_eint_offsets *eint_offsets =
			&pctl->devdata->eint_offsets;
	u32 mask = BIT(d->hwirq & 0x1f);
	void __iomem *reg = mtk_eint_get_offset(pctl, d->hwirq,
			eint_offsets->mask_set);

	writel(mask, reg);
}

/** @ingroup IP_group_eint_internal_function
 * @par Description
 *     This function register into Linux interrupt framework to unmask
 *     the eint and eint controller start to forward unmasked eint.
 * @param[in]
 *     d: structure of irq_data for getting irq chip information.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static void mtk_eint_unmask(struct irq_data *d)
{
	struct mtk_pinctrl *pctl = irq_data_get_irq_chip_data(d);
	const struct mtk_eint_offsets *eint_offsets =
		&pctl->devdata->eint_offsets;
	u32 mask = BIT(d->hwirq & 0x1f);
	void __iomem *reg = mtk_eint_get_offset(pctl, d->hwirq,
			eint_offsets->mask_clr);

	writel(mask, reg);

	if (pctl->eint_dual_edges[d->hwirq])
		mtk_eint_flip_edge(pctl, d->hwirq);
}

/** @ingroup IP_group_eint_internal_function
 * @par Description
 *     This function is used to calculate the debounce index.
 * @param[in]
 *     dbnc_infos: debounce array data.
 * @param[in]
 *     dbnc_infos_num: debounce array length.
 * @param[in]
 *     debounce: target debounce to set.
 * @return
 *     debounce index.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
unsigned int mtk_gpio_debounce_select(const unsigned int *dbnc_infos,
	int dbnc_infos_num, unsigned int debounce)
{
	unsigned int i;
	unsigned int dbnc = dbnc_infos_num;

	for (i = 0; i < dbnc_infos_num; i++) {
		if (debounce <= dbnc_infos[i]) {
			dbnc = i;
			break;
		}
	}
	return dbnc;
}

/** @ingroup IP_group_eint_internal_function
 * @par Description
 *     This function register into Linux gpio framework to set debounce.
 * @param[in]
 *     chip: gpio_chip pointer.
 * @param[in]
 *     offset: the gpio pin number.
 * @param[in]
 *     debounce: target debounce to set.
 * @return
 *     0, success to set debounce.\n
 *     error code from gpio_set_debounce().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_gpio_set_debounce(struct gpio_chip *chip, unsigned offset,
	unsigned debounce)
{
	struct mtk_pinctrl *pctl = dev_get_drvdata(chip->dev);
	int eint_num, virq, eint_offset;
	unsigned int set_offset, bit, clr_bit, clr_offset, rst, unmask, dbnc;
	static const unsigned int debounce_time[] = {
				  500, 1000, 16000, 32000, 64000,
				  128000, 256000};
	const struct mtk_desc_pin *pin;
	struct irq_data *d;

	pin = pctl->devdata->pins + offset;
	if (pin->eint.eintnum == NO_EINT_SUPPORT)
		return -EINVAL;

	eint_num = pin->eint.eintnum;
	virq = irq_find_mapping(pctl->domain, eint_num);
	eint_offset = (eint_num % 4) * 8;
	d = irq_get_irq_data(virq);

	set_offset = (eint_num / 4) * 4 + pctl->devdata->eint_offsets.dbnc_set;
	clr_offset = (eint_num / 4) * 4 + pctl->devdata->eint_offsets.dbnc_clr;
	if (!mtk_eint_can_en_debounce(pctl, eint_num))
		return -EINVAL;

	if (pctl->devdata->spec_debounce_select)
		dbnc = pctl->devdata->spec_debounce_select(debounce);
	else
		dbnc = mtk_gpio_debounce_select(debounce_time,
			ARRAY_SIZE(debounce_time), debounce);

	if (!mtk_eint_get_mask(pctl, eint_num)) {
		mtk_eint_mask(d);
		unmask = 1;
	} else {
		unmask = 0;
	}

	clr_bit = 0xff << eint_offset;
	writel(clr_bit, pctl->eint_reg_base + clr_offset);

	bit = ((dbnc << EINT_DBNC_SET_DBNC_BITS) | EINT_DBNC_SET_EN) <<
		eint_offset;
	rst = EINT_DBNC_RST_BIT << eint_offset;
	writel(rst | bit, pctl->eint_reg_base + set_offset);

	/* Delay a while (more than 2T) to wait for hw debounce counter reset */
	/* work correctly */
	udelay(1);
	if (unmask == 1)
		mtk_eint_unmask(d);

	return 0;
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Get the gpio pinmux function.
 * @param[in]
 *     chip: gpio_chip pointer.
 * @param[in]
 *     offset: the gpio pin number.
 * @return
 *     ((pinmux & mask) >> (GPIO_MODE_BITS * bit)), gpio pinmux function.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_pinmux_get(struct gpio_chip *chip, unsigned offset)
{
	unsigned int reg_addr;
	unsigned char bit;
	unsigned int pinmux = 0;
	struct mtk_pinctrl *pctl = dev_get_drvdata(chip->dev);
	unsigned int mask = (1L << pctl->devdata->gpio_mode_bits) - 1;

	if (pctl->devdata->num_base_address == 1) {
		reg_addr = ((offset / pctl->devdata->max_gpio_mode_per_reg)
			* 0x6) + pctl->devdata->pinmux_offset;
	} else {
		reg_addr = ((offset / pctl->devdata->max_gpio_mode_per_reg)
			<< pctl->devdata->port_shf)
			+ pctl->devdata->pinmux_offset;
	}

	bit = offset % pctl->devdata->max_gpio_mode_per_reg;
	mask <<= (pctl->devdata->gpio_mode_bits * bit);
	pctl->reg_pinctrl = false;
	regmap_read(mtk_get_regmap(pctl, offset), reg_addr, &pinmux);
	return ((pinmux & mask) >> (pctl->devdata->gpio_mode_bits * bit));
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Get the gpio input value.
 * @param[in]
 *     chip: gpio_chip pointer.
 * @param[in]
 *     offset: the gpio pin number.
 * @return
 *     !!(read_val & bit), gpio input value.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_gpio_get_in(struct gpio_chip *chip, unsigned offset)
{
	unsigned int reg_addr;
	unsigned int bit;
	unsigned int read_val = 0;
	struct mtk_pinctrl *pctl = dev_get_drvdata(chip->dev);

	if (pctl->devdata->num_base_address == 1) {
		if (mtk_get_port(pctl, offset) == 0x8)
			reg_addr = 0x6 + pctl->devdata->din_offset;
		else
			reg_addr = mtk_get_port(pctl, offset) +
			pctl->devdata->din_offset;
		bit = BIT(offset & 0xf);
	} else {
		reg_addr = mtk_get_port(pctl, offset) +
			pctl->devdata->din_offset;
		bit = BIT(offset & 0x1f);
	}

	pctl->reg_pinctrl = false;
	regmap_read(mtk_get_regmap(pctl, offset), reg_addr, &read_val);
	return !!(read_val & bit);
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Get the gpio output value.
 * @param[in]
 *     chip: gpio_chip pointer.
 * @param[in]
 *     offset: the gpio pin number.
 * @return
 *     !!(read_val & bit), gpio output value.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_gpio_get_out(struct gpio_chip *chip, unsigned offset)
{
	unsigned int reg_addr;
	unsigned int bit;
	unsigned int read_val = 0;
	struct mtk_pinctrl *pctl = dev_get_drvdata(chip->dev);

	if (pctl->devdata->num_base_address == 1) {
		if (mtk_get_port(pctl, offset) == 0x8)
			reg_addr = 0x6 + pctl->devdata->dout_offset;
		else
			reg_addr = mtk_get_port(pctl, offset) +
			pctl->devdata->dout_offset;
		bit = BIT(offset & 0xf);
	} else {
		reg_addr = mtk_get_port(pctl, offset) +
			pctl->devdata->dout_offset;
		bit = BIT(offset & 0x1f);
	}

	pctl->reg_pinctrl = false;
	regmap_read(mtk_get_regmap(pctl, offset), reg_addr, &read_val);
	return !!(read_val & bit);
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Get the gpio pull enable or pull disable status.
 * @param[in]
 *     chip: gpio_chip pointer.
 * @param[in]
 *     offset: the gpio pin number.
 * @return
 *     !!(pull_en & bit), pull enable or pull disable status.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_pullen_get(struct gpio_chip *chip, unsigned offset)
{
	unsigned int reg_addr;
	unsigned int bit;
	unsigned int pull_en = 0;
	struct mtk_pinctrl *pctl = dev_get_drvdata(chip->dev);

	if (pctl->devdata->num_base_address == 1) {
		if (mtk_get_port(pctl, offset) == 0x8)
			reg_addr = 0x6 + pctl->devdata->pullen_offset;
		else
			reg_addr = mtk_get_port(pctl, offset) +
			pctl->devdata->pullen_offset;
		bit = BIT(offset & 0xf);
	} else {
		reg_addr = mtk_get_port(pctl, mtk_get_pin(pctl, offset))
					+ pctl->devdata->pullen_offset;
		bit =  BIT(mtk_get_pin(pctl, offset) & 0x1f);
	}

	if (pctl->devdata->num_base_address == 1)
		pctl->reg_pinctrl = false;
	else
		pctl->reg_pinctrl = true;
	regmap_read(mtk_get_regmap(pctl, offset), reg_addr, &pull_en);
	return !!(pull_en & bit);
}

int mtk_spec_driving_get_samereg(struct regmap *regmap,
		const struct mtk_pin_spec_driving_set_samereg *driving_infos,
		unsigned int info_num, unsigned int pin)
{
	unsigned int i;
	unsigned int reg_driving;
	unsigned int val = 0, bit_driving, bit_d0, bit_d1;
	const struct mtk_pin_spec_driving_set_samereg *spec_driving_pin;
	bool find = false;

	for (i = 0; i < info_num; i++) {
		if (pin == driving_infos[i].pin) {
			find = true;
			break;
		}
	}

	if (!find)
		return -1;

	spec_driving_pin = driving_infos + i;
	reg_driving = spec_driving_pin->offset;

	regmap_read(regmap, reg_driving, &val);
	bit_driving = !(val & BIT(spec_driving_pin->driv_bit));
	bit_d0 = !!(val & BIT(spec_driving_pin->d0_bit));
	bit_d1 = !!(val & BIT(spec_driving_pin->d1_bit));

	return (bit_driving)|(bit_d0<<1)|(bit_d1<<2)|(1<<3);
}

int mtk_spec_rsel_get_samereg(struct regmap *regmap,
		const struct mtk_pin_spec_rsel_set_samereg *rsel_infos,
		unsigned int info_num, unsigned int pin)
{
	unsigned int i;
	unsigned int reg_rsel;
	unsigned int val = 0, bit_rsel, bit_s0, bit_s1;
	const struct mtk_pin_spec_rsel_set_samereg *spec_rsel_pin;
	bool find = false;

	for (i = 0; i < info_num; i++) {
		if (pin == rsel_infos[i].pin) {
			find = true;
			break;
		}
	}

	if (!find)
		return -1;

	spec_rsel_pin = rsel_infos + i;
	reg_rsel = spec_rsel_pin->offset;

	regmap_read(regmap, reg_rsel, &val);
	bit_rsel = !(val & BIT(spec_rsel_pin->rsel_bit));
	bit_s0 = !!(val & BIT(spec_rsel_pin->s0_bit));
	bit_s1 = !!(val & BIT(spec_rsel_pin->s1_bit));

	return (bit_s0<<1)|(bit_s1<<2)|(1<<3);
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Get the gpio pull up or pull down status.
 * @param[in]
 *     chip: gpio_chip pointer.
 * @param[in]
 *     offset: the gpio pin number.
 * @return
 *     !!(pull_sel & bit), pull up or pull down status.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_pullsel_get(struct gpio_chip *chip, unsigned offset)
{
	unsigned int reg_addr;
	unsigned int bit;
	unsigned int pull_sel = 0;
	struct mtk_pinctrl *pctl = dev_get_drvdata(chip->dev);

	if (pctl->devdata->num_base_address == 1) {
		if (mtk_get_port(pctl, offset) == 0x8)
			reg_addr = 0x6 + pctl->devdata->pullsel_offset;
		else
			reg_addr = mtk_get_port(pctl, offset) +
			pctl->devdata->pullsel_offset;
		bit = BIT(offset & 0xf);
	} else {
		reg_addr = mtk_get_port(pctl, mtk_get_pin(pctl, offset))
			+ pctl->devdata->pullsel_offset;
		bit =  BIT(mtk_get_pin(pctl, offset) & 0x1f);
	}

	if (pctl->devdata->num_base_address == 1)
		pctl->reg_pinctrl = false;
	else
		pctl->reg_pinctrl = true;
	regmap_read(mtk_get_regmap(pctl, offset), reg_addr, &pull_sel);
	return !!(pull_sel & bit);
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Get the gpio IES status.
 * @param[in]
 *     chip: gpio_chip pointer.
 * @param[in]
 *     offset: the gpio pin number.
 * @return
 *     !!(ies & bit), gpio IES status.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_ies_get(struct gpio_chip *chip, unsigned offset)
{
	unsigned int reg_addr;
	unsigned int bit;
	unsigned int ies;
	struct mtk_pinctrl *pctl = dev_get_drvdata(chip->dev);

	reg_addr = mtk_get_port(pctl, mtk_get_pin(pctl, offset))
		+ pctl->devdata->ies_offset;

	bit =  BIT(mtk_get_pin(pctl, offset) & 0x1f);
	if (pctl->devdata->num_base_address == 1)
		pctl->reg_pinctrl = false;
	else
		pctl->reg_pinctrl = true;
	regmap_read(mtk_get_regmap(pctl, offset), reg_addr, &ies);
	return !!(ies & bit);
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Get the gpio SMT status.
 * @param[in]
 *     chip: gpio_chip pointer.
 * @param[in]
 *     offset: the gpio pin number.
 * @return
 *     !!(smt & bit), gpio SMT status.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_smt_get(struct gpio_chip *chip, unsigned offset)
{
	unsigned int reg_addr;
	unsigned int bit;
	unsigned int smt;
	struct mtk_pinctrl *pctl = dev_get_drvdata(chip->dev);

	reg_addr = mtk_get_port(pctl, mtk_get_pin(pctl, offset))
		+ pctl->devdata->smt_offset;

	bit =  BIT(mtk_get_pin(pctl, offset) & 0x1f);
	if (pctl->devdata->num_base_address == 1)
		pctl->reg_pinctrl = false;
	else
		pctl->reg_pinctrl = true;
	regmap_read(mtk_get_regmap(pctl, offset), reg_addr, &smt);
	return !!(smt & bit);
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Get the gpio driving value.
 * @param[in]
 *     chip: gpio_chip pointer.
 * @param[in]
 *     offset: the gpio pin number.
 * @return
 *     ((val & mask) >> shift), gpio driving value.
 *     -1, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If the gpio pin number not valid, return -1.
 *     2. If the gpio pin driving group not valid, return -1.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_driving_get(struct gpio_chip *chip, unsigned offset)
{
	const struct mtk_pin_drv_grp *pin_drv;
	unsigned int val = 0;
	unsigned int bits, mask, shift;
	const struct mtk_drv_group_desc *drv_grp;
	struct mtk_pinctrl *pctl = dev_get_drvdata(chip->dev);

	if (offset >= pctl->devdata->npins)
		return -1;

	pin_drv = mtk_find_pin_drv_grp_by_pin(pctl, offset);
	if (!pin_drv || pin_drv->grp > pctl->devdata->n_grp_cls)
		return -1;

	drv_grp = pctl->devdata->grp_desc + pin_drv->grp;
	bits = drv_grp->high_bit - drv_grp->low_bit + 1;
	mask = BIT(bits) - 1;
	shift = pin_drv->bit + drv_grp->low_bit;
	mask <<= shift;
	if (pctl->devdata->num_base_address == 1)
		pctl->reg_pinctrl = false;
	else
		pctl->reg_pinctrl = true;
	regmap_read(mtk_get_regmap(pctl, offset), pin_drv->offset, &val);
	return ((val & mask) >> shift);
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Show the gpio pin info for debug.
 * @param[in]
 *     dev: device pointer.
 * @param[in]
 *     attr: device_attribute pointer.
 * @param[out]
 *     buf: store gpio pin info.
 * @return
 *     len, the gpio pin info length.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If the start gpio pin number is not valid, return -EINVAL.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static ssize_t mtk_gpio_show_pin(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int len = 0;
	int bufLen = (int)PAGE_SIZE;
	struct mtk_pinctrl *pctl = dev_get_drvdata(dev);
	struct gpio_chip *chip = pctl->chip;
	unsigned int i;
	int pull_val;

	len += snprintf(buf+len, bufLen-len,
		"gpio base is %d, total num is %d\n",
		chip->base, pctl->chip->ngpio);

	len += snprintf(buf+len, bufLen-len,
		"PIN: [MODE] [DIR] [DOUT] [DIN] [PULL_EN] [PULL_SEL] [IES] [SMT] [DRIVE] ( [R1] [R0] )\n");

	if (pctl->dbg_start >= pctl->chip->ngpio) {
		len += snprintf(buf+len, bufLen-len,
		"wrong gpio-range: start should less than %d!\n",
		pctl->chip->ngpio);
		return len;
	}

	for (i = pctl->dbg_start; i < pctl->chip->ngpio; i++) {
		pull_val = mtk_pullsel_get(chip, i);
		if ((len+32) >= bufLen)
			break;

		len += snprintf(buf+len, bufLen-len,
				"%4d:% d% d% d% d% d% d% d% d% d",
				i,
				mtk_pinmux_get(chip, i),
				!mtk_gpio_get_direction(chip, i),
				mtk_gpio_get_out(chip, i),
				mtk_gpio_get_in(chip, i),
				mtk_pullen_get(chip, i),
				(pull_val >= 0) ? (pull_val & 1) : -1,
				mtk_ies_get(chip, i),
				mtk_smt_get(chip, i),
				mtk_driving_get(chip, i));
		if (((pull_val & 8) != 0) && (pull_val >= 0))
			len += snprintf(buf+len, bufLen-len, " %d %d",
					!!(pull_val & 4), !!(pull_val & 2));
		len += snprintf(buf+len, bufLen-len, "\n");
	}
	return len;
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Configure the gpio function.
 * @param[in]
 *     dev: device pointer.
 * @param[in]
 *     attr: device_attribute pointer.
 * @param[in]
 *     buf: store gpio configure info.
 * @param[in]
 *     count: the length of gpio configure info buf.
 * @return
 *     count, the length of gpio configure info buf.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static ssize_t mtk_gpio_store_pin(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int val;
	unsigned int pin, reg_addr;
	int val_set;
	struct mtk_pinctrl *pctl = dev_get_drvdata(dev);
	struct pinctrl_dev *pctldev = pctl->pctl_dev;

	if ((strncmp(buf, "mode", 4) == 0)
	    && (sscanf(buf+4, "%d %d", &pin, &val) == 2)) {
		val_set = mtk_pmx_set_mode(pctldev, pin, (unsigned long)val);
	} else if ((strncmp(buf, "dir", 3) == 0)
		   && (sscanf(buf+3, "%d %d", &pin, &val) == 2)) {
		val_set  = mtk_pmx_gpio_set_direction(pctldev,
			NULL, pin, !(bool)val);
	} else if ((strncmp(buf, "out", 3) == 0)
		   && (sscanf(buf+3, "%d %d", &pin, &val) == 2)) {
		val_set  = mtk_pmx_gpio_set_direction(pctldev,
			NULL, pin, false);
		mtk_gpio_set(pctl->chip, pin, val);
	} else if ((strncmp(buf, "pullen", 6) == 0)
		   && (sscanf(buf+6, "%d %d", &pin, &val) == 2)) {
		val_set = mtk_pconf_set_pull_select(pctl, pin, !!(bool)val,
			false, ((u32)MTK_PUPD_SET_R1R0_00 + (u32)val));
	} else if ((strncmp(buf, "pullsel", 7) == 0)
		   && (sscanf(buf+7, "%d %d", &pin, &val) == 2)) {
		val_set = mtk_pconf_set_pull_select(pctl,
			pin, true, !!(bool)val, 0);
	} else if ((strncmp(buf, "ies", 3) == 0)
		   && (sscanf(buf+3, "%d %d", &pin, &val) == 2)) {
		val_set = mtk_pconf_set_ies_smt(pctl,
			pin, val, PIN_CONFIG_INPUT_ENABLE);
	} else if ((strncmp(buf, "smt", 3) == 0)
		   && (sscanf(buf+3, "%d %d", &pin, &val) == 2)) {
		val_set = mtk_pconf_set_ies_smt(pctl,
			pin, val, PIN_CONFIG_INPUT_SCHMITT_ENABLE);
	} else if ((strncmp(buf, "start", 5) == 0)
		   && (sscanf(buf+5, "%d", &val) == 1)) {
		pctl->dbg_start = (u32)val;
	} else if (!strncmp(buf, "rr", 2)
		   && (sscanf(buf+2, "%d %x", &pin, &reg_addr) == 2)) {
		if (pctl->devdata->num_base_address == 1)
			pctl->reg_pinctrl = false;
		else
			pctl->reg_pinctrl = true;
		regmap_read(mtk_get_regmap(pctl, pin), reg_addr, &val);
		pr_err("reg(%p) : 0x%x : 0x%x\n",
			mtk_get_regmap(pctl, pin), reg_addr, val);
	} else if (!strncmp(buf, "wr", 2)
		   && (sscanf(buf+2, "%d %x %x",
			      &pin, &reg_addr, &val) == 3)) {
		pr_err("reg : 0x%x : 0x%x\n", reg_addr, val);
		if (pctl->devdata->num_base_address == 1)
			pctl->reg_pinctrl = false;
		else
			pctl->reg_pinctrl = true;
		regmap_write(mtk_get_regmap(pctl, pin), reg_addr, val);
		pr_err("reg(%p) : 0x%x : 0x%x\n",
			mtk_get_regmap(pctl, pin), reg_addr, val);
	} else {
		pr_err("invaild argument!\n");
	}

	return (ssize_t)count;
}

static DEVICE_ATTR(mt_gpio, 0664, mtk_gpio_show_pin, mtk_gpio_store_pin);

/** @ingroup IP_group_gpio_internal_struct
 * @brief Register dev_attr_mt_gpio.
 */
static struct device_attribute *gpio_attr_list[] = {
	&dev_attr_mt_gpio,
};

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *      Create sysfs attribute file.
 * @param[in]
 *     dev: device pointer.
 * @return
 *     0, create sysfs attribute file done.\n
 *     -EINVAL, wrong parameter.
 *     error code from device_create_file().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If device pointer NULL, return -EINVAL.
 *     2. If device_create_file() fails, return error code from it.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mt_gpio_create_attr(struct device *dev)
{
	int idx, err = 0;
	int num = ARRAY_SIZE(gpio_attr_list);

	if (!dev)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		err = device_create_file(dev, gpio_attr_list[idx]);
		if (err)
			break;
	}

	return err;
}

/** @ingroup IP_group_gpio_internal_struct
 * @brief Register gpio_chip.
 */
static struct gpio_chip mtk_gpio_chip = {
	.owner			= THIS_MODULE,
	.request		= gpiochip_generic_request,
	.free			= gpiochip_generic_free,
	.get_direction		= mtk_gpio_get_direction,
	.direction_input	= mtk_gpio_direction_input,
	.direction_output	= mtk_gpio_direction_output,
	.get			= mtk_gpio_get,
	.set			= mtk_gpio_set,
	.to_irq			= mtk_gpio_to_irq,
	.set_debounce		= mtk_gpio_set_debounce,
};

/** @ingroup IP_group_eint_internal_function
 * @par Description
 *     This function register into Linux interrupt framework to
 *     set irq polarity like High/Low or Rising/Falling trigger.
 * @param[in]
 *     d: structure of irq_data for getting irq chip information.
 * @param[in]
 *     type: irq type to set. Types are defined in include/linux/irq.h .
 * @return
 *     0, set type is done.\n
 *     error code from irq_set_type().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_eint_set_type(struct irq_data *d,
				      unsigned int type)
{
	struct mtk_pinctrl *pctl = irq_data_get_irq_chip_data(d);
	const struct mtk_eint_offsets *eint_offsets =
		&pctl->devdata->eint_offsets;
	u32 mask = BIT(d->hwirq & 0x1f);
	void __iomem *reg;

	if (((type & IRQ_TYPE_EDGE_BOTH) && (type & IRQ_TYPE_LEVEL_MASK)) ||
		((type & IRQ_TYPE_LEVEL_MASK) == IRQ_TYPE_LEVEL_MASK)) {
		dev_err(pctl->dev, "Can't configure IRQ%d (EINT%lu) for type 0x%X\n",
			d->irq, d->hwirq, type);
		return -EINVAL;
	}

	if ((type & IRQ_TYPE_EDGE_BOTH) == IRQ_TYPE_EDGE_BOTH)
		pctl->eint_dual_edges[d->hwirq] = 1;
	else
		pctl->eint_dual_edges[d->hwirq] = 0;

	if (type & (IRQ_TYPE_LEVEL_LOW | IRQ_TYPE_EDGE_FALLING)) {
		reg = mtk_eint_get_offset(pctl, d->hwirq,
			eint_offsets->pol_clr);
		writel(mask, reg);
	} else {
		reg = mtk_eint_get_offset(pctl, d->hwirq,
			eint_offsets->pol_set);
		writel(mask, reg);
	}

	if (type & (IRQ_TYPE_EDGE_RISING | IRQ_TYPE_EDGE_FALLING)) {
		reg = mtk_eint_get_offset(pctl, d->hwirq,
			eint_offsets->sens_clr);
		writel(mask, reg);
	} else {
		reg = mtk_eint_get_offset(pctl, d->hwirq,
			eint_offsets->sens_set);
		writel(mask, reg);
	}

	if (pctl->eint_dual_edges[d->hwirq])
		mtk_eint_flip_edge(pctl, d->hwirq);

	return 0;
}

static int mtk_eint_set_affinity(struct irq_data *d,
		const struct cpumask *mask_val, bool force)
{
	struct mtk_pinctrl *pctl = irq_data_get_irq_chip_data(d);

	irq_set_affinity(pctl->irq, mask_val);

	return IRQ_SET_MASK_OK;
}

/** @ingroup IP_group_eint_internal_function
 * @par Description
 *     This function register into Linux interrupt framework to
 *     set irq as a wakeup source used in system suspend.
 * @param[in]
 *     d: structure of irq_data for getting irq chip information.
 * @param[in]
 *     on: 1, eanble wakeup source; 0, disable wakeup source.
 * @return
 *     0, set wakeup is done.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_eint_irq_set_wake(struct irq_data *d, unsigned int on)
{
	struct mtk_pinctrl *pctl = irq_data_get_irq_chip_data(d);
	int shift = d->hwirq & 0x1f;
	int reg = d->hwirq >> 5;

	if (on)
		pctl->wake_mask[reg] |= BIT(shift);
	else
		pctl->wake_mask[reg] &= ~BIT(shift);

	return 0;
}

/** @ingroup IP_group_eint_internal_function
 * @par Description
 *     This function used to enable or disable eint according to
 *     record status.
 * @param[in]
 *     chip: structure of mtk_eint_offsets for getting eint
 *     controller information.
 * @param[in]
 *     eint_reg_base: eint controller register offset.
 * @param[in]
 *     buf: recording status of eint mask.
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static void mtk_eint_chip_write_mask(const struct mtk_eint_offsets *chip,
		void __iomem *eint_reg_base, u32 *buf)
{
	int port;
	void __iomem *reg;

	for (port = 0; port < chip->ports; port++) {
		reg = eint_reg_base + (port << 2);
		writel_relaxed(~buf[port], reg + chip->mask_set);
		writel_relaxed(buf[port], reg + chip->mask_clr);
	}
}

/** @ingroup IP_group_eint_internal_function
 * @par Description
 *     This function used to readback eint mask status during
 *     systemsuspend
 * @param[in]
 *     chip: structure of mtk_eint_offsets for getting eint
 *     controller information.
 * @param[in]
 *     eint_reg_base: eint controller register offset.
 * @param[in]
 *     buf: recording status of eint mask.
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static void mtk_eint_chip_read_mask(const struct mtk_eint_offsets *chip,
		void __iomem *eint_reg_base, u32 *buf)
{
	int port;
	void __iomem *reg;

	for (port = 0; port < chip->ports; port++) {
		reg = eint_reg_base + chip->mask + (port << 2);
		buf[port] = ~readl_relaxed(reg);
		/* Mask is 0 when irq is enabled, and 1 when disabled. */
	}
}

/** @ingroup IP_group_eint_internal_function
 * @par Description
 *     This function register into Linux power management framework
 *     during system suspend,this function will be called to record current
 *     mask status and enable wakeup source.
 * @param[in]
 *     device: structure of device for suspend.
 * @return
 *     0, suspend done.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_eint_suspend(struct device *device)
{
	void __iomem *reg;
	struct mtk_pinctrl *pctl = dev_get_drvdata(device);
	const struct mtk_eint_offsets *eint_offsets =
			&pctl->devdata->eint_offsets;

	reg = pctl->eint_reg_base;
	mtk_eint_chip_read_mask(eint_offsets, reg, pctl->cur_mask);
	mtk_eint_chip_write_mask(eint_offsets, reg, pctl->wake_mask);

	return 0;
}

/** @ingroup IP_group_eint_internal_function
 * @par Description
 *     This function register into Linux power management framework
 *     during system suspend,this function will be called to restore
 *     mask status that suspend store.
 * @param[in]
 *     device: structure of device for suspend.
 * @return
 *     0, resume done.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_eint_resume(struct device *device)
{
	struct mtk_pinctrl *pctl = dev_get_drvdata(device);
	const struct mtk_eint_offsets *eint_offsets =
			&pctl->devdata->eint_offsets;

	mtk_eint_chip_write_mask(eint_offsets,
			pctl->eint_reg_base, pctl->cur_mask);

	return 0;
}

/** @ingroup IP_group_eint_internal_struct
 * @brief define mt3612 eint power management operations.
 */
const struct dev_pm_ops mtk_eint_pm_ops = {
	.suspend_noirq = mtk_eint_suspend,
	.resume_noirq = mtk_eint_resume,
};

/** @ingroup IP_group_eint_internal_function
 * @par Description
 *     This function register into Linux interrupt framework to acknowledge
 *     the interrupt, after acknowledgment pending status will be cleared.
 * @param[in]
 *     d: structure of irq_data for getting irq chip information.
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static void mtk_eint_ack(struct irq_data *d)
{
	struct mtk_pinctrl *pctl = irq_data_get_irq_chip_data(d);
	const struct mtk_eint_offsets *eint_offsets =
		&pctl->devdata->eint_offsets;
	u32 mask = BIT(d->hwirq & 0x1f);
	void __iomem *reg = mtk_eint_get_offset(pctl, d->hwirq,
			eint_offsets->ack);

	writel(mask, reg);
}

/** @ingroup IP_group_eint_internal_struct
 * @brief Irq chip data for register into linux interrupt framework.
 */
static struct irq_chip mtk_pinctrl_irq_chip = {
	.name = "mt-eint",
	.irq_disable = mtk_eint_mask,
	.irq_mask = mtk_eint_mask,
	.irq_unmask = mtk_eint_unmask,
	.irq_ack = mtk_eint_ack,
	.irq_set_type = mtk_eint_set_type,
	.irq_set_affinity = mtk_eint_set_affinity,
	.irq_set_wake = mtk_eint_irq_set_wake,
	.irq_request_resources = mtk_pinctrl_irq_request_resources,
	.irq_release_resources = mtk_pinctrl_irq_release_resources,
};

/** @ingroup IP_group_eint_internal_function
 * @par Description
 *     Show the eint info for debug.
 * @param[in]
 *     dev: device pointer.
 * @param[in]
 *     attr: device_attribute pointer.
 * @param[out]
 *     buf: store eint info.
 * @return
 *     len, the eint info length.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static ssize_t mtk_eint_show(struct device *dev, struct device_attribute *attr,
	char *buf)
{
	int i, j, irq, len, bufLen;
	u32 mask, msk, sen, pol, val;
	void __iomem *reg;
	struct irq_desc *desc;
	struct irqaction	*action;
	const struct mtk_desc_pin *pin;
	char * const irq_type[] = {"NONE", "R", "F", "RF", "H", "RH", "FH",
		"RFH", "L", "RL", "FL", "RFL", "HL", "RHL", "FHL", "RFHL"};
	struct mtk_pinctrl *pctl = dev_get_drvdata(dev);
	const struct mtk_eint_offsets *eint_offsets =
			&pctl->devdata->eint_offsets;

	len = 0;
	bufLen = PAGE_SIZE;

	len += snprintf(buf+len, bufLen-len,
		"eint \tpin \tirq \tdis \tdep \ttrig \twake \tmask \tsen \tpol \tisr\n");
	for (i = 0; i < pctl->devdata->ap_num; i++) {
		int shift = i & 0x1f;
		int offset = i >> 5;

		irq = irq_find_mapping(pctl->domain, i);
		if (!irq)
			continue;

		desc = irq_to_desc(irq);
		if (!desc || !irqd_get_trigger_type(&desc->irq_data))
			continue;
		action = desc->action;

		mask = BIT(i & 0x1f);
		reg = mtk_eint_get_offset(pctl, i, eint_offsets->mask);
		val = readl(reg);
		msk = val & mask;

		reg = mtk_eint_get_offset(pctl, i, eint_offsets->sens);
		val = readl(reg);
		sen = val & mask;

		reg = mtk_eint_get_offset(pctl, i, eint_offsets->pol);
		val = readl(reg);
		pol = val & mask;

		for (j = 0; j < pctl->devdata->npins; j++) {
			pin = pctl->devdata->pins + j;
			if (pin->eint.eintnum == i)
				break;
		}
		len += snprintf(buf+len, bufLen-len,
			"%3d \t%3d \t%3d \t%3d \t%3d \t%4s \t%4d \t%4d \t%3d \t%3d \t%s\n",
			i, j, irq,
			irqd_irq_disabled(&desc->irq_data) ? 1 : 0,
			desc->depth,
			irq_type[irqd_get_trigger_type(&desc->irq_data)],
			(pctl->wake_mask[offset] & BIT(shift)) ? 1 : 0,
			msk ? 1 : 0,
			sen ? 1 : 0,
			pol ? 1 : 0,
			action ? (action->name ?
				  action->name : "NONE") : "NONE");
	}

	return len;
}

/** @ingroup IP_group_eint_internal_function
 * @par Description
 *     Configure the eint function.
 * @param[in]
 *     dev: device pointer.
 * @param[in]
 *     attr: device_attribute pointer.
 * @param[in]
 *     buf: store eint configure info.
 * @param[in]
 *     count: the length of eint configure info buf.
 * @return
 *     count, the length of eint configure info buf.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static ssize_t mtk_eint_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	int eint, irq;
	struct irq_desc *desc;
	struct mtk_pinctrl *pctl = dev_get_drvdata(dev);
	const struct mtk_eint_offsets *eint_offsets =
		&pctl->devdata->eint_offsets;
	u32 mask;
	void __iomem *reg;

	if (!strncmp(buf, "mask", 4) && (sscanf(buf+4, "%d", &eint) == 1)) {
		irq = irq_find_mapping(pctl->domain, eint);
		desc = irq_to_desc(irq);
		if (!desc)
			return count;
		mtk_eint_mask(&desc->irq_data);
	} else if (!strncmp(buf, "unmask", 6) &&
				 (sscanf(buf + 6, "%d", &eint) == 1)) {
		irq = irq_find_mapping(pctl->domain, eint);
		desc = irq_to_desc(irq);
		if (!desc)
			return count;
		mtk_eint_unmask(&desc->irq_data);
	} else if (!strncmp(buf, "trigger", 7) &&
				 (sscanf(buf + 7, "%d", &eint) == 1)) {
		if (eint >= pctl->devdata->ap_num)
			return count;

		mask = BIT(eint & 0x1f);
		reg = mtk_eint_get_offset(pctl, eint, eint_offsets->soft_set);
		writel(mask, reg);
	}
	return count;
}

static DEVICE_ATTR(mtk_eint, 0664, mtk_eint_show, mtk_eint_store);

/** @ingroup IP_group_eint_internal_struct
 * @brief Register dev_attr_mtk_eint.
 */
static struct device_attribute *eint_attr_list[] = {
	&dev_attr_mtk_eint,
};


/** @ingroup IP_group_eint_internal_function
 * @par Description
 *      Create sysfs attribute file.
 * @param[in]
 *     dev: device pointer.
 * @return
 *     0, create sysfs attribute file done.\n
 *     -EINVAL, wrong parameter.
 *     error code from device_create_file().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If device pointer NULL, return -EINVAL.
 *     2. If device_create_file() fails, return error code from it.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_eint_create_attr(struct device *dev)
{
	int idx, err = 0;
	int num = ARRAY_SIZE(eint_attr_list);

	if (!dev)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		err = device_create_file(dev, eint_attr_list[idx]);
		if (err)
			break;
	}

	return err;
}

/** @ingroup IP_group_eint_internal_function
 * @par Description
 *     initialize eint global control register.
 * @param[in]
 *     pctl: mtk_pinctrl pointer, struct mtk_pinctrl contains eint
 *     controller data.
 * @return
 *     0, eint init done.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static unsigned int mtk_eint_init(struct mtk_pinctrl *pctl)
{
	const struct mtk_eint_offsets *eint_offsets =
		&pctl->devdata->eint_offsets;
	void __iomem *reg = pctl->eint_reg_base + eint_offsets->dom_en;
	unsigned int i;

	for (i = 0; i < pctl->devdata->ap_num; i += 32) {
		writel(0xffffffff, reg);
		reg += 4;
	}

	return 0;
}

/** @ingroup IP_group_eint_internal_function
 * @par Description
 *     eint debounce process.
 * @param[in]
 *     pctl: mtk_pinctrl pointer, struct mtk_pinctrl contains eint
 *     controller data.
 * @param[in]
 *     index: eint number.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static inline void
mtk_eint_debounce_process(struct mtk_pinctrl *pctl, int index)
{
	unsigned int rst, ctrl_offset;
	unsigned int bit, dbnc;
	const struct mtk_eint_offsets *eint_offsets =
		&pctl->devdata->eint_offsets;

	ctrl_offset = (index / 4) * 4 + eint_offsets->dbnc_ctrl;
	dbnc = readl(pctl->eint_reg_base + ctrl_offset);
	bit = EINT_DBNC_SET_EN << ((index % 4) * 8);
	if ((bit & dbnc) > 0) {
		ctrl_offset = (index / 4) * 4 + eint_offsets->dbnc_set;
		rst = EINT_DBNC_RST_BIT << ((index % 4) * 8);
		writel(rst, pctl->eint_reg_base + ctrl_offset);
	}
}

/** @ingroup IP_group_eint_internal_function
 * @par Description
 *     This function register into Linux interrupt framework to
 *     handle each eint when triggerd.
 * @param[in]
 *     desc: irq_desc pointer, contain irq_chip information.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static void mtk_eint_irq_handler(struct irq_desc *desc)
{
	struct irq_chip *chip = irq_desc_get_chip(desc);
	struct mtk_pinctrl *pctl = irq_desc_get_handler_data(desc);
	unsigned int status, eint_num;
	int offset, index, virq;
	const struct mtk_eint_offsets *eint_offsets =
		&pctl->devdata->eint_offsets;
	void __iomem *reg =  mtk_eint_get_offset(pctl, 0, eint_offsets->stat);
	int dual_edges, start_level, curr_level;
	const struct mtk_desc_pin *pin;

	chained_irq_enter(chip, desc);
	for (eint_num = 0;
	     eint_num < pctl->devdata->ap_num;
	     eint_num += 32, reg += 4) {
		status = readl(reg);
		while (status) {
			offset = __ffs(status);
			index = eint_num + offset;
			virq = irq_find_mapping(pctl->domain, index);
			status &= ~BIT(offset);

			dual_edges = pctl->eint_dual_edges[index];
			if (dual_edges) {
				/* Clear soft-irq in case we raised it */
				/* last time */
				writel(BIT(offset), reg - eint_offsets->stat +
					eint_offsets->soft_clr);

				pin = mtk_find_pin_by_eint_num(pctl, index);
				start_level = mtk_gpio_get(pctl->chip,
							   pin->pin.number);
			}

			generic_handle_irq(virq);

			if (dual_edges) {
				curr_level = mtk_eint_flip_edge(pctl, index);

				/* If level changed, we might lost one edge */
				/* interrupt, raised it through soft-irq */
				if (start_level != curr_level)
					writel(BIT(offset), reg -
						eint_offsets->stat +
						eint_offsets->soft_set);
			}

			if (index < pctl->devdata->db_cnt)
				mtk_eint_debounce_process(pctl, index);
		}
	}
	chained_irq_exit(chip, desc);
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     Build gpio pin group for each pin.
 * @param[in]
 *     pdev: platform_device pointer.
 * @return
 *     0, build gpio pin group for each pin done.\n
 *     -ENOMEM, no memory available.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If devm_kcalloc() fails, return -ENOMEM.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_pctrl_build_state(struct platform_device *pdev)
{
	struct mtk_pinctrl *pctl = platform_get_drvdata(pdev);
	int i;

	pctl->ngroups = pctl->devdata->npins;

	/* Allocate groups */
	pctl->groups = devm_kcalloc(&pdev->dev, pctl->ngroups,
				    sizeof(*pctl->groups), GFP_KERNEL);
	if (!pctl->groups)
		return -ENOMEM;

	/* We assume that one pin is one group, use pin name as group name. */
	pctl->grp_names = devm_kcalloc(&pdev->dev, pctl->ngroups,
				       sizeof(*pctl->grp_names), GFP_KERNEL);
	if (!pctl->grp_names)
		return -ENOMEM;

	for (i = 0; i < pctl->devdata->npins; i++) {
		const struct mtk_desc_pin *pin = pctl->devdata->pins + i;
		struct mtk_pinctrl_group *group = pctl->groups + i;

		group->name = pin->pin.name;
		group->pin = pin->pin.number;

		pctl->grp_names[i] = pin->pin.name;
	}

	return 0;
}

/** @ingroup IP_group_gpio_internal_function
 * @par Description
 *     GPIO pinctrl driver init.
 * @param[out]
 *     pdev: platform device pointer.
 * @param[in]
 *     data: GPIO related data pointer.
 * @param[in]
 *     regmap: regmap pointer.
 * @return
 *     0, GPIO pinctrl driver init done.\n
 *     -ENOMEM, no memory available.\n
 *     -EINVAL, wrong parameter.\n
 *     error code from PTR_ERR().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If devm_kzalloc() fails, return -ENOMEM.
 *     2. If of_find_property() or of_parse_phandle() or\n
 *     mtk_pctrl_build_state() or gpiochip_add() or gpiochip_add_pin_range()\n
 *     or platform_get_resource() fails, return -EINVAL.
 *     3. If IS_ERR(pctl->regmap0) or IS_ERR(pctl->regmap1) or\n
 *     IS_ERR(pctl->regmap2) or IS_ERR(pctl->regmap3) or IS_ERR(pctl->regmap4)\n
 *     or IS_ERR(pctl->regmap5) or IS_ERR(pctl->regmap6) fails, return the\n
 *     error code from PTR_ERR().
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
int mtk_pctrl_init(struct platform_device *pdev,
		const struct mtk_pinctrl_devdata *data,
		struct regmap *regmap)
{
	struct pinctrl_pin_desc *pins;
	struct mtk_pinctrl *pctl;
	struct device_node *np = pdev->dev.of_node, *node;
	struct property *prop;
	struct resource *res;
	int i, ret, irq, ports_buf;

	pctl = devm_kzalloc(&pdev->dev, sizeof(*pctl), GFP_KERNEL);
	if (!pctl)
		return -ENOMEM;

	platform_set_drvdata(pdev, pctl);

	prop = of_find_property(np, "pins-are-numbered", NULL);
	if (!prop) {
		dev_err(&pdev->dev, "only support pins-are-numbered format\n");
		return -EINVAL;
	}

	node = of_parse_phandle(np, "mediatek,pctl-regmap", 0);
	if (node) {
		pctl->regmap0 = syscon_node_to_regmap(node);
		if (IS_ERR(pctl->regmap0))
			return PTR_ERR(pctl->regmap0);
	} else if (regmap) {
		pctl->regmap0  = regmap;
		dev_info(&pdev->dev, "pctl->regmap0 = %p\n", pctl->regmap0);
	} else {
		dev_err(&pdev->dev, "Pinctrl node has not register regmap.\n");
		return -EINVAL;
	}

	/* Only 8135 has two base addr, other SoCs have only one. */
	/* MT3612 has 7 base addr, follow 8135 flow */
	node = of_parse_phandle(np, "mediatek,pctl-regmap", 1);
	if (node) {
		pctl->regmap1 = syscon_node_to_regmap(node);
		if (IS_ERR(pctl->regmap1))
			return PTR_ERR(pctl->regmap1);
	}
	node = of_parse_phandle(np, "mediatek,pctl-regmap", 2);
	if (node) {
		pctl->regmap2 = syscon_node_to_regmap(node);
		if (IS_ERR(pctl->regmap2))
			return PTR_ERR(pctl->regmap2);
	}
	node = of_parse_phandle(np, "mediatek,pctl-regmap", 3);
	if (node) {
		pctl->regmap3 = syscon_node_to_regmap(node);
		if (IS_ERR(pctl->regmap3))
			return PTR_ERR(pctl->regmap3);
	}
	node = of_parse_phandle(np, "mediatek,pctl-regmap", 4);
	if (node) {
		pctl->regmap4 = syscon_node_to_regmap(node);
		if (IS_ERR(pctl->regmap4))
			return PTR_ERR(pctl->regmap4);
	}
	node = of_parse_phandle(np, "mediatek,pctl-regmap", 5);
	if (node) {
		pctl->regmap5 = syscon_node_to_regmap(node);
		if (IS_ERR(pctl->regmap5))
			return PTR_ERR(pctl->regmap5);
	}
	node = of_parse_phandle(np, "mediatek,pctl-regmap", 6);
	if (node) {
		pctl->regmap6 = syscon_node_to_regmap(node);
		if (IS_ERR(pctl->regmap6))
			return PTR_ERR(pctl->regmap6);
	}

	node = of_parse_phandle(np, "mediatek,pctl-regmap", 7);
	if (node) {
		pctl->regmap7 = syscon_node_to_regmap(node);
		if (IS_ERR(pctl->regmap7))
			return PTR_ERR(pctl->regmap7);
	}

	pctl->devdata = data;
	ret = mtk_pctrl_build_state(pdev);
	if (ret) {
		dev_err(&pdev->dev, "build state failed: %d\n", ret);
		return -EINVAL;
	}

	pins = devm_kcalloc(&pdev->dev, pctl->devdata->npins, sizeof(*pins),
			    GFP_KERNEL);
	if (!pins)
		return -ENOMEM;

	for (i = 0; i < pctl->devdata->npins; i++)
		pins[i] = pctl->devdata->pins[i].pin;

	pctl->pctl_desc.name = dev_name(&pdev->dev);
	pctl->pctl_desc.owner = THIS_MODULE;
	pctl->pctl_desc.pins = pins;
	pctl->pctl_desc.npins = pctl->devdata->npins;
	pctl->pctl_desc.confops = &mtk_pconf_ops;
	pctl->pctl_desc.pctlops = &mtk_pctrl_ops;
	pctl->pctl_desc.pmxops = &mtk_pmx_ops;
	pctl->dev = &pdev->dev;

	pctl->pctl_dev = pinctrl_register(&pctl->pctl_desc, &pdev->dev, pctl);
	if (IS_ERR(pctl->pctl_dev)) {
		dev_err(&pdev->dev, "couldn't register pinctrl driver\n");
		return PTR_ERR(pctl->pctl_dev);
	}

	pctl->chip = devm_kzalloc(&pdev->dev, sizeof(*pctl->chip), GFP_KERNEL);
	if (!pctl->chip) {
		ret = -ENOMEM;
		goto pctrl_error;
	}

	*pctl->chip = mtk_gpio_chip;
	pctl->chip->ngpio = pctl->devdata->npins;
	pctl->chip->label = dev_name(&pdev->dev);
	pctl->chip->dev = &pdev->dev;
	pctl->chip->base = -1;

	ret = gpiochip_add(pctl->chip);
	if (ret) {
		ret = -EINVAL;
		goto pctrl_error;
	}

	/* Register the GPIO to pin mappings. */
	ret = gpiochip_add_pin_range(pctl->chip, dev_name(&pdev->dev),
			0, 0, pctl->devdata->npins);
	if (ret) {
		ret = -EINVAL;
		goto chip_error;
	}

	if (mt_gpio_create_attr(&pdev->dev))
		dev_warn(&pdev->dev, "mt_gpio create attribute error\n");

	if (!of_property_read_bool(np, "interrupt-controller"))
		return 0;

	/* Get EINT register base from dts. */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res) {
		dev_err(&pdev->dev, "Unable to get Pinctrl resource\n");
		ret = -EINVAL;
		goto chip_error;
	}

	pctl->eint_reg_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pctl->eint_reg_base)) {
		ret = -EINVAL;
		goto chip_error;
	}

	ports_buf = pctl->devdata->eint_offsets.ports;
	pctl->wake_mask = devm_kcalloc(&pdev->dev, ports_buf,
					sizeof(*pctl->wake_mask), GFP_KERNEL);
	if (!pctl->wake_mask) {
		ret = -ENOMEM;
		goto chip_error;
	}

	pctl->cur_mask = devm_kcalloc(&pdev->dev, ports_buf,
					sizeof(*pctl->cur_mask), GFP_KERNEL);
	if (!pctl->cur_mask) {
		ret = -ENOMEM;
		goto chip_error;
	}

	pctl->eint_dual_edges = devm_kcalloc(&pdev->dev, pctl->devdata->ap_num,
					     sizeof(int), GFP_KERNEL);
	if (!pctl->eint_dual_edges) {
		ret = -ENOMEM;
		goto chip_error;
	}

	irq = irq_of_parse_and_map(np, 0);
	if (!irq) {
		dev_err(&pdev->dev, "couldn't parse and map irq\n");
		ret = -EINVAL;
		goto chip_error;
	}

	pctl->domain = irq_domain_add_linear(np,
		pctl->devdata->ap_num, &irq_domain_simple_ops, NULL);
	if (!pctl->domain) {
		dev_err(&pdev->dev, "Couldn't register IRQ domain\n");
		ret = -ENOMEM;
		goto chip_error;
	}

	mtk_eint_init(pctl);
	for (i = 0; i < pctl->devdata->ap_num; i++) {
		int virq = irq_create_mapping(pctl->domain, i);

		irq_set_chip_and_handler(virq, &mtk_pinctrl_irq_chip,
			handle_level_irq);
		irq_set_chip_data(virq, pctl);
	}

	pctl->irq = irq;
	irq_set_chained_handler_and_data(irq, mtk_eint_irq_handler, pctl);
	if (mtk_eint_create_attr(&pdev->dev))
		dev_warn(&pdev->dev, "mtk_eint create attribute error\n");
	return 0;

chip_error:
	gpiochip_remove(pctl->chip);
pctrl_error:
	pinctrl_unregister(pctl->pctl_dev);
	return ret;
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek Pinctrl Driver");
MODULE_AUTHOR("Hongzhou Yang <hongzhou.yang@mediatek.com>");
