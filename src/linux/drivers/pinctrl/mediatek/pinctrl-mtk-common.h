/*
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

#ifndef __PINCTRL_MTK_COMMON_H
#define __PINCTRL_MTK_COMMON_H

#include <linux/pinctrl/pinctrl.h>
#include <linux/regmap.h>
#include <linux/pinctrl/pinconf-generic.h>

#define NO_EINT_SUPPORT    255
#define MT_EDGE_SENSITIVE           0
#define MT_LEVEL_SENSITIVE          1
#define EINT_DBNC_SET_DBNC_BITS     4
#define EINT_DBNC_RST_BIT           (0x1 << 1)
#define EINT_DBNC_SET_EN            (0x1 << 0)

#define MTK_PINCTRL_NOT_SUPPORT	(0xffff)

/** @ingroup IP_group_gpio_internal_struct
 * @brief gpio function description struct.
 */
struct mtk_desc_function {
	/** GPIO function name. */
	const char *name;
	/** GPIO function value. */
	unsigned char muxval;
};

/** @ingroup IP_group_gpio_internal_struct
 * @brief eint description struct.
 */
struct mtk_desc_eint {
	/** EINT select. */
	unsigned char eintmux;
	/** EINT number. */
	unsigned char eintnum;
};

/** @ingroup IP_group_gpio_internal_struct
 * @brief gpio pin description struct.
 */
struct mtk_desc_pin {
	/** GPIO pin info description. */
	struct pinctrl_pin_desc	pin;
	/** EINT info description. */
	const struct mtk_desc_eint eint;
	/** GPIO function description. */
	const struct mtk_desc_function	*functions;
};

#define MTK_PIN(_pin, _pad, _chip, _eint, ...)		\
	{							\
		.pin = _pin,					\
		.eint = _eint,					\
		.functions = (struct mtk_desc_function[]){	\
			__VA_ARGS__, { } },			\
	}

#define MTK_EINT_FUNCTION(_eintmux, _eintnum)				\
	{							\
		.eintmux = _eintmux,					\
		.eintnum = _eintnum,					\
	}

#define MTK_FUNCTION(_val, _name)				\
	{							\
		.muxval = _val,					\
		.name = _name,					\
	}

#define SET_ADDR(x, y)  (x + (y->devdata->port_align))
#define CLR_ADDR(x, y)  (x + (y->devdata->port_align << 1))

/** @ingroup IP_group_gpio_internal_struct
 * @brief gpio pin group struct.
 */
struct mtk_pinctrl_group {
	/** GPIO pin group name. */
	const char	*name;
	/** GPIO pin group config. */
	unsigned long	config;
	/** The number of gpio pins in group. */
	unsigned	pin;
};

/** @ingroup IP_group_gpio_internal_struct
 * @brief gpio pin driving group data struct.
 */
struct mtk_drv_group_desc {
	/** The maximum current of this group. */
	unsigned char min_drv;
	/** The minimum current of this group. */
	unsigned char max_drv;
	/** The lowest bit of this group. */
	unsigned char low_bit;
	/** The highest bit of this group. */
	unsigned char high_bit;
	/** The step current of this group. */
	unsigned char step;
};

#define MTK_DRV_GRP(_min, _max, _low, _high, _step)	\
	{	\
		.min_drv = _min,	\
		.max_drv = _max,	\
		.low_bit = _low,	\
		.high_bit = _high,	\
		.step = _step,		\
	}

/** @ingroup IP_group_gpio_internal_struct
 * @brief gpio pin driving info struct.
 */
struct mtk_pin_drv_grp {
	/** The pin number. */
	unsigned short pin;
	/** The offset of driving register for this pin. */
	unsigned short offset;
	/** The bit of driving register for this pin. */
	unsigned char bit;
	/** The group for this pin belongs to. */
	unsigned char grp;
};

#define MTK_PIN_DRV_GRP(_pin, _offset, _bit, _grp)	\
	{	\
		.pin = _pin,	\
		.offset = _offset,	\
		.bit = _bit,	\
		.grp = _grp,	\
	}

/**
 * struct mtk_pin_spec_pupd_set_samereg
 * - For special pins' pull up/down setting which resides in same register
 * @pin: The pin number.
 * @offset: The offset of special pull up/down setting register.
 * @pupd_bit: The pull up/down bit in this register.
 * @r0_bit: The r0 bit of pull resistor.
 * @r1_bit: The r1 bit of pull resistor.
 */
struct mtk_pin_spec_pupd_set_samereg {
	unsigned short pin;
	unsigned short offset;
	unsigned char pupd_bit;
	unsigned char r1_bit;
	unsigned char r0_bit;
};

#define MTK_PIN_PUPD_SPEC_SR(_pin, _offset, _pupd, _r1, _r0)	\
	{	\
		.pin = _pin,	\
		.offset = _offset,	\
		.pupd_bit = _pupd,	\
		.r1_bit = _r1,		\
		.r0_bit = _r0,		\
	}

/**
 * struct mtk_pin_spec_driving_set_samereg
 * - For special pins' driving strength setting.
 * @pin: The pin number.
 * @offset: The offset of special driving strength setting register.
 * @driv_bit: The driving strength enable bit in this register.
 * @d0_bit: The d0 bit of driving strength.
 * @d1_bit: The d1 bit of driving strength.
 */

struct mtk_pin_spec_driving_set_samereg {
	unsigned short pin;
	unsigned short offset;
	unsigned char driv_bit;
	unsigned char d1_bit;
	unsigned char d0_bit;
};

#define MTK_PIN_SPEC_DRIVING(_pin, _offset, _driv_bit, _d1, _d0)	\
	{	\
		.pin = _pin,	\
		.offset = _offset,	\
		.driv_bit = _driv_bit,	\
		.d1_bit = _d1,		\
		.d0_bit = _d0,		\
	}

/**
 * struct mtk_pin_spec_rsel_set_samereg
 * - For special pins' pull up/down resistance setting of I2C.
 * @pin: The pin number.
 * @offset: The offset of special resistance value setting register.
 * @pupd_bit: The driving strength enable bit in this register.
 * @s0_bit: The s0 bit of resistance value.
 * @s1_bit: The s1 bit of resistance value.
 */

struct mtk_pin_spec_rsel_set_samereg {
	unsigned short pin;
	unsigned short offset;
	unsigned char rsel_bit;
	unsigned char s1_bit;
	unsigned char s0_bit;
};

#define MTK_PIN_SPEC_RSEL(_pin, _offset, _rsel_bit, _s1, _s0)	\
	{	\
		.pin = _pin,	\
		.offset = _offset,	\
		.rsel_bit = _rsel_bit,	\
		.s1_bit = _s1,		\
		.s0_bit = _s0,		\
	}

/**
 * struct mtk_pin_ies_set - For special pins' ies and smt setting.
 * @start: The start pin number of those special pins.
 * @end: The end pin number of those special pins.
 * @offset: The offset of special setting register.
 * @bit: The bit of special setting register.
 */
struct mtk_pin_ies_smt_set {
	unsigned short start;
	unsigned short end;
	unsigned short offset;
	unsigned char bit;
};

#define MTK_PIN_IES_SMT_SPEC(_start, _end, _offset, _bit)	\
	{	\
		.start = _start,	\
		.end = _end,	\
		.bit = _bit,	\
		.offset = _offset,	\
	}

/** @ingroup IP_group_eint_internal_struct
 * @brief eint controller data struct.
 */
struct mtk_eint_offsets {
	/** The eint controller name. */
	const char *name;
	/** The start offset of eint pending status register. */
	unsigned int  stat;
	/** The start offset of eint acknowledge register. */
	unsigned int  ack;
	/** The start offset of eint mask status register. */
	unsigned int  mask;
	/** The start offset of eint set mask register. */
	unsigned int  mask_set;
	/** The start offset of eint clear mask register. */
	unsigned int  mask_clr;
	/** The start offset of eint sensitivity status register. */
	unsigned int  sens;
	/** The start offset of eint set sensitivity register. */
	unsigned int  sens_set;
	/** The start offset of eint clear sensitivity register. */
	unsigned int  sens_clr;
	/** The start offset of eint software pending status register. */
	unsigned int  soft;
	/** The start offset of eint set software pending register. */
	unsigned int  soft_set;
	/** The start offset of eint clear software pending register. */
	unsigned int  soft_clr;
	/** The start offset of eint polarity status register. */
	unsigned int  pol;
	/** The start offset of eint set polarity register. */
	unsigned int  pol_set;
	/** The start offset of eint clear polarity register. */
	unsigned int  pol_clr;
	/** The start offset of eint domain register. */
	unsigned int  dom_en;
	/** The start offset of eint debounce status register. */
	unsigned int  dbnc_ctrl;
	/** The start offset of eint set debounce register. */
	unsigned int  dbnc_set;
	/** The start offset of eint clear debounce register. */
	unsigned int  dbnc_clr;
	/** For ports operation. */
	u8  port_mask;
	/** The number of eint ports, each ports represent 32 eint. */
	u8  ports;
};

/** @ingroup IP_group_gpio_internal_struct
 * @brief gpio pin controller data struct.
 */
struct mtk_pinctrl {
	/** The first regmap to record gpio register address info. */
	struct regmap	*regmap0;
	/** The second regmap to record gpio register address info. */
	struct regmap	*regmap1;
	/** The third regmap to record gpio register address info. */
	struct regmap	*regmap2;
	/** The fourth regmap to record gpio register address info. */
	struct regmap	*regmap3;
	/** The fifth regmap to record gpio register address info. */
	struct regmap	*regmap4;
	/** The sixth regmap to record gpio register address info. */
	struct regmap	*regmap5;
	/** The seventh regmap to record gpio register address info. */
	struct regmap	*regmap6;
	/** GPIO pin controller descriptor. */
	struct regmap	*regmap7;
	/** GPIO pin controller descriptor. */
	struct pinctrl_desc pctl_desc;
	/** GPIO device abstract. */
	struct device           *dev;
	/** GPIO pin controller abstract. */
	struct gpio_chip	*chip;
	/** GPIO pins groups. */
	struct mtk_pinctrl_group	*groups;
	/** The number of gpio pins groups. */
	unsigned			ngroups;
	/** GPIO pins groups names. */
	const char          **grp_names;
	/** GPIO register select. */
	unsigned int         reg_pinctrl;
	/** GPIO pin control class device abstract. */
	struct pinctrl_dev      *pctl_dev;
	/** GPIO pin controller related data. */
	const struct mtk_pinctrl_devdata  *devdata;
	/** gpio pin number start from for debug. */
	unsigned int dbg_start;
	/** for sw test pin0. */
	int pinctrl_test_pin0;
	/** for sw test pin1. */
	int pinctrl_test_pin1;
	/** The eint controller register offset. */
	void __iomem		*eint_reg_base;
	/** Linux framework prototype */
	struct irq_domain	*domain;
	/** Buffer used to store if the eint is woking in both edge. */
	int			*eint_dual_edges;
	/** Buffer used to store system wakeup source. */
	u32 *wake_mask;
	/** Buffer used to store current mask status. */
	u32 *cur_mask;
	int irq;
};

/** @ingroup IP_group_gpio_internal_struct
 * @brief gpio pin controller related data struct.
 */
struct mtk_pinctrl_devdata {
	/** An array describing all pins the pin controller affects. */
	const struct mtk_desc_pin	*pins;
	/** The number of entries in pins. */
	unsigned int				npins;
	/** The driving group info. */
	const struct mtk_drv_group_desc	*grp_desc;
	/** The number of pin driving groups kinds. */
	unsigned int	n_grp_cls;
	/** The driving group for all pins. */
	const struct mtk_pin_drv_grp	*pin_drv_grp;
	/** The number of pin driving groups. */
	unsigned int	n_pin_drv_grps;
	/** @spec_pull_set:
	* Each SoC may have special pins for pull up/down setting,
	* these pins' pull setting are very different,
	* they have separate pull up/down bit,
	* R0 and R1 resistor bit, so they need special pull setting.
	* If special setting is success, this should return 0,
	* otherwise it should return non-zero value.
	*/
	int (*spec_pull_set)(struct mtk_pinctrl *pctl,
		struct regmap *reg, unsigned int pin,
		unsigned char align, bool isup, unsigned int arg);
	int (*spec_driving_set)(struct regmap *reg, unsigned int pin,
			unsigned char align, bool isup, unsigned int arg);
	int (*spec_rsel_set)(struct regmap *reg, unsigned int pin,
			unsigned char align, bool isup, unsigned int arg);
	int (*spec_driving_get)(struct regmap *reg, unsigned int pin);
	int (*spec_rsel_get)(struct regmap *reg, unsigned int pin);
	/** @spec_ies_smt_set:
	* Some pins are irregular, their input enable and smt
	* control register are discontinuous,
	* but they are mapping together.
	* That means when user set smt,
	* input enable is set at the same time. So they
	* also need special control. If special control is success,
	*  this should return 0, otherwise return non-zero value.
	*/
	int (*spec_ies_smt_set)(struct regmap *reg, unsigned int pin,
		unsigned char align, int value, enum pin_config_param arg);
	/** @spec_debounce_select:
	* Some platforms support different debounce that our driver
	* currently not support, if you want support different debounce
	* this function should be implement.
	*/
	unsigned int (*spec_debounce_select)(unsigned debounce);
	/** The direction register offset. */
	unsigned int dir_offset;
	/** The IES control register offset. */
	unsigned int ies_offset;
	/** The SMT control register offset. */
	unsigned int smt_offset;
	/** The pull-up/pull-down enable register offset. */
	unsigned int pullen_offset;
	/** The pull-up/pull-down select register offset. */
	unsigned int pullsel_offset;
	/** The driving control register offset. */
	unsigned int drv_offset;
	/** The data output register offset. */
	unsigned int dout_offset;
	/** The data input register offset. */
	unsigned int din_offset;
	/** The pinmux register offset. */
	unsigned int pinmux_offset;
	/** The start of pin number to use the second pull select address. */
	unsigned short type1_start;
	/** The end of pin number to use the second pull select address. */
	unsigned short type1_end;
	/** The start of pin number to use the third pull select address. */
	unsigned short type2_start;
	/** The end of pin number to use the third pull select address. */
	unsigned short type2_end;
	/** The start of pin number to use the fourth pull select address. */
	unsigned short type3_start;
	/** The end of pin number to use the fourth pull select address. */
	unsigned short type3_end;
	/** The start of pin number to use the fifth pull select address. */
	unsigned short type4_start;
	/** The end of pin number to use the fifth pull select address. */
	unsigned short type4_end;
	/** The start of pin number to use the sixth pull select address. */
	unsigned short type5_start;
	/** The end of pin number to use the sixth pull select address. */
	unsigned short type5_end;
	/** The start of pin number to use the seventh pull select address. */
	unsigned short type6_start;
	/** The end of pin number to use the seventh pull select address. */
	unsigned short type6_end;
	/** The start of pin number to use the seventh pull select address. */
	unsigned short type7_start;
	/** The end of pin number to use the seventh pull select address. */
	unsigned short type7_end;
	/** The shift between two registers. */
	unsigned char  port_shf;
	/** The mask of register. */
	unsigned char  port_mask;
	/** Provide clear register and set register step. */
	unsigned char  port_align;
	/** The number of gpio mode include. */
	unsigned int  gpio_mode_bits;
	/** The max gpio mode in per reg. */
	unsigned int  max_gpio_mode_per_reg;
	/** The max gpio number in per reg. */
	unsigned int  max_gpio_num_per_reg;
	/** The eint controller information. */
	struct mtk_eint_offsets eint_offsets;
	/** The total eint number system support. */
	unsigned int	ap_num;
	/** The total eint number that support debounce. */
	unsigned int	db_cnt;
	/** The number of gpio base register. */
	unsigned int	num_base_address;
};

int mtk_pctrl_init(struct platform_device *pdev,
		const struct mtk_pinctrl_devdata *data,
		struct regmap *regmap);

int mtk_pctrl_spec_pull_set_samereg(struct mtk_pinctrl *pctl,
		struct regmap *regmap,
		const struct mtk_pin_spec_pupd_set_samereg *pupd_infos,
		unsigned int info_num, unsigned int pin,
		unsigned char align, bool isup, unsigned int r1r0);

unsigned int mtk_gpio_debounce_select(const unsigned int *dbnc_infos,
		int dbnc_infos_num, unsigned int debounce);
extern const struct dev_pm_ops mtk_eint_pm_ops;

int mtk_pctrl_spec_driving_set_samereg(struct regmap *regmap,
		const struct mtk_pin_spec_driving_set_samereg *driving_infos,
		unsigned int info_num, unsigned int pin,
		unsigned char align, bool isup, unsigned int r1r0);

int mtk_spec_driving_get_samereg(struct regmap *regmap,
		const struct mtk_pin_spec_driving_set_samereg *driving_infos,
		unsigned int info_num, unsigned int pin);

int mtk_pctrl_spec_rsel_set_samereg(struct regmap *regmap,
		const struct mtk_pin_spec_rsel_set_samereg *driving_infos,
		unsigned int info_num, unsigned int pin,
		unsigned char align, bool isup, unsigned int r1r0);

int mtk_spec_rsel_get_samereg(struct regmap *regmap,
		const struct mtk_pin_spec_rsel_set_samereg *driving_infos,
		unsigned int info_num, unsigned int pin);

#endif /* __PINCTRL_MTK_COMMON_H */
