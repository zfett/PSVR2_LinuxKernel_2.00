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

#ifndef __TMRIO_MTK_MT3612_H
#define __TMRIO_MTK_MT3612_H
#ifdef CONFIG_OF
extern void __iomem *tmrio_base;
#undef TMRIO_BASE
#define TMRIO_BASE	tmrio_base
#else
#define TMRIO_BASE	((void __iomem *)0x10040100)
#endif

#define RSUCCESS	0
#define EEXCESSPWMNO	1
#define EPARMNOSUPPORT	2
#define ERROR		3
#define EBADADDR	4
#define EEXCESSBITS	5
#define EINVALID	6
#define TMRIO_CH_OFFSET	12
#define TMR_EN		1

#define TMRIO_DEVICE	"mt-tmrio"

#define TMRIO0_COMP_VAL	(TMRIO_BASE + 0x00)
#define TMRIO0_CNT	(TMRIO_BASE + 0x04)
#define TMRIO0_CON	(TMRIO_BASE + 0x08)

/******************* Register Manipulations*****************/
#define mt_reg_sync_writel(v, a) \
	do {    \
		__raw_writel((v), (void __force __iomem *)((a)));   \
		mb();   /*fix for memory barrier without comment */ \
	} while (0)

#define READREG32(reg)		__raw_readl((void *)reg)
#define WRITEREG32(val, reg)	mt_reg_sync_writel(val, (void *)reg)

#define TMRIO_HLPSEL_SHIFT_BIT	1
#define TMRIO_MODE_SHIFT_BIT	2
#define TMRIO_GPIO_SHIFT_BIT	4
#define TMRIO_DEBEN_SHIFT_BIT	5
#define TMRIO_DEB_SHIFT_BIT	6
#define TMRIO_CKGEN_SHIFT_BIT	10
#define TMRIO_SWRST_SHIFT_BIT	15
#define TMRIO_IRQSTS_SHIFT_BIT	16
#define TMRIO_IRQACK_SHIFT_BIT	17

static irqreturn_t tmrio_handler(int irq, void *dev_id);
#endif /* __TMRIO_MTK_MT3612_H */
