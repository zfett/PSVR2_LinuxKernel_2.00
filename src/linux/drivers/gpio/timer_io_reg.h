/*
 * Copyright (C) 2017 MediaTek Inc.
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

#ifndef __TIMER_IO_REG_H__
#define __TIMER_IO_REG_H__

/* ----------------- Register Definitions ------------------- */
#define TMR_EN0			BIT(0)
#define HLP_SEL0		BIT(1)
#define PWN_SEL0		BIT(2)
#define INOUT_MODE0		BIT(3)
#define GPIO_SEL0		BIT(4)
#define DEB_EN0			BIT(5)
#define DEB_SEL0		GENMASK(9, 6)
#define CK_GEN_0		GENMASK(14, 10)
#define TIMER0_SW_RST		BIT(15)
#define TIMER0_IRQ_ACK		BIT(16)
#define TIMER0_IRQ_CLR		BIT(17)
#endif /*__TIMER_IO_REG_H__*/
