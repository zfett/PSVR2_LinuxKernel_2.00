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

#ifndef _MTK_THERMAL_H_
#define _MTK_THERMAL_H_

/* AUXADC Registers */
#define AUXADC_CON0_V		0x000
#define AUXADC_CON1_V		0x004
#define AUXADC_CON1_SET_V	0x008
#define AUXADC_CON1_CLR_V	0x00c
#define AUXADC_CON2_V		0x010
#define AUXADC_DATA(channel)	(0x14 + (channel) * 4)
#define AUXADC_MISC_V		0x094

#define AUXADC_CON1_CHANNEL(x)	BIT(x)

#define APMIXED_SYS_TS_CON1	0x604

/* Thermal Controller Registers */
#/* Thermal Controller Registers */
/* Bank0 */
#define TEMP_MONCTL0		0x000
#define TEMP_MONCTL1		0x004
#define TEMP_MONCTL2		0x008
#define TEMP_MONIDET0		0x014
#define TEMP_MONIDET1		0x018
#define TEMP_MSRCTL0		0x038
#define TEMP_AHBPOLL		0x040
#define TEMP_AHBTO		0x044
#define TEMP_ADCPNP0		0x048
#define TEMP_ADCPNP1		0x04c
#define TEMP_ADCPNP2		0x050
#define TEMP_ADCPNP3		0x0b4

#define TEMP_ADCMUX		0x054
#define TEMP_ADCEN		0x060
#define TEMP_PNPMUXADDR		0x064
#define TEMP_ADCMUXADDR		0x068
#define TEMP_ADCENADDR		0x074
#define TEMP_ADCVALIDADDR	0x078
#define TEMP_ADCVOLTADDR	0x07c
#define TEMP_RDCTRL		0x080
#define TEMP_ADCVALIDMASK	0x084
#define TEMP_ADCVOLTAGESHIFT	0x088
#define TEMP_ADCWRITECTRL	0x08c
#define TEMP_MSR0		0x090
#define TEMP_MSR1		0x094
#define TEMP_MSR2		0x098
#define TEMP_MSR3		0x0B8
#define TEMP_SPARE0		0x0F0

/* Bank1 */
#define TEMP_MONCTL0_1		0x100
#define TEMP_MONCTL1_1		0x104
#define TEMP_MONCTL2_1		0x108
#define TEMP_MONIDET0_1		0x114
#define TEMP_MONIDET1_1		0x118
#define TEMP_MSRCTL0_1		0x138
#define TEMP_AHBPOLL_1		0x140
#define TEMP_AHBTO_1		0x144
#define TEMP_ADCPNP0_1		0x148
#define TEMP_ADCPNP1_1		0x14c
#define TEMP_ADCPNP2_1		0x150
#define TEMP_ADCPNP3_1		0x1b4

#define TEMP_ADCMUX_1		0x154
#define TEMP_ADCEN_1		0x160
#define TEMP_PNPMUXADDR_1	0x164
#define TEMP_ADCMUXADDR_1	0x168
#define TEMP_ADCENADDR_1	0x174
#define TEMP_ADCVALIDADDR_1	0x178
#define TEMP_ADCVOLTADDR_1	0x17c
#define TEMP_RDCTRL_1		0x180
#define TEMP_ADCVALIDMASK_1	0x184
#define TEMP_ADCVOLTAGESHIFT_1	0x188
#define TEMP_ADCWRITECTRL_1	0x18c
#define TEMP_MSR0_1		0x190
#define TEMP_MSR1_1		0x194
#define TEMP_MSR2_1		0x198
#define TEMP_MSR3_1		0x1B8
#define TEMP_SPARE0_1		0x1F0

#define PTPCORESEL		0x400

#endif /* _MTK_THERMAL_H_ */
