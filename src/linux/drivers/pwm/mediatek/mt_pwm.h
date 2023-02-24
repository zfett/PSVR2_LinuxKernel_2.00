/*
 * Copyright (C) 2018 MediaTek Inc.
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
 * @file mt_pwm.h
 * Heander of mt_pwm.c
 */

#ifndef __MT_PWM_H__
#define __MT_PWM_H__

#include <linux/types.h>
#include <linux/io.h>
#include <asm/cacheflush.h>
#include <linux/platform_device.h>

/** @ingroup IP_group_pwm_internal_struct
 * @brief Pwm platform device  structure.
 */

struct mtk_pwm {
	/** pwm abstract  controller */
	struct pwm_chip chip;
	/** pwm to set or get device name */
	const char *name;
	/** pwm to set or get aomic reference */
	atomic_t ref;
	/** pwm to set or get device number */
	dev_t devno;
	/** pwm to set or get spinlock info */
	spinlock_t lock;
	/** pwm present power flag */
	unsigned long power_flag;
#ifdef CONFIG_COMMON_CLK_MT3612
	/** pwm group clk */
	struct clk *groupclk;
	struct clk *fpwmclk;
	struct clk *pwmclk;
	/** pwm1~ pwm32 clk */
	struct clk *pwm1clk;
	struct clk *pwm2clk;
	struct clk *pwm3clk;
	struct clk *pwm4clk;
	struct clk *pwm5clk;
	struct clk *pwm6clk;
	struct clk *pwm7clk;
	struct clk *pwm8clk;
	struct clk *pwm9clk;
	struct clk *pwm10clk;
	struct clk *pwm11clk;
	struct clk *pwm12clk;
	struct clk *pwm13clk;
	struct clk *pwm14clk;
	struct clk *pwm15clk;
	struct clk *pwm16clk;
	struct clk *pwm17clk;
	struct clk *pwm18clk;
	struct clk *pwm19clk;
	struct clk *pwm20clk;
	struct clk *pwm21clk;
	struct clk *pwm22clk;
	struct clk *pwm23clk;
	struct clk *pwm24clk;
	struct clk *pwm25clk;
	struct clk *pwm26clk;
	struct clk *pwm27clk;
	struct clk *pwm28clk;
	struct clk *pwm29clk;
	struct clk *pwm30clk;
	struct clk *pwm31clk;
	struct clk *pwm32clk;
#endif
	/** pwm to set or get hardware device structure */
	struct device dev;
};

s32 pwm_set_spec_config(const struct pwm_spec_config *conf);
void mt_pwm_26M_clk_enable(u32 enable);

#endif
