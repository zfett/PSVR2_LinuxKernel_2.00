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

/**
 * @file mtk_tmrio.h
 * Heander of tmrio-mt3612.c
 */
#ifndef _MT_TMRIO_H_
#define _MT_TMRIO_H_

#include <linux/types.h>

/** @ingroup IP_group_timerio_internal_enum
* Timer I/O operation mode\n
* value is from 0 to 2.
*/
enum TMRIO_MODE {
/** 0: Input Pulse Width Mode */
	TMRIO_PULSE_WIDTH_MODE = 0,
/** 1: Input Pulse COUNT Mode */
	TMRIO_PULSE_COUNT_MODE = 1,
/** 2: Output Mode */
	TMRIO_OUTPUT_MODE = 2,
/** Indicate Timer I/O mode max number*/
	TMRIO_MODE_MAX,
};

/** @ingroup IP_group_timerio_internal_enum
* Timer I/O Channel Number\n
* value is from 0 to 15.
*/
enum TMRIO_CH {
/** Select Timer I/O channel 0*/
	TMRIO_0 = 0,
/** Select Timer I/O channel 1*/
	TMRIO_1,
/** Select Timer I/O channel 2*/
	TMRIO_2,
/** Select Timer I/O channel 3*/
	TMRIO_3,
/** Select Timer I/O channel 4*/
	TMRIO_4,
/** Select Timer I/O channel 5*/
	TMRIO_5,
/** Select Timer I/O channel 6*/
	TMRIO_6,
/** Select Timer I/O channel 7*/
	TMRIO_7,
/** Select Timer I/O channel 8*/
	TMRIO_8,
/** Select Timer I/O channel 9*/
	TMRIO_9,
/** Select Timer I/O channel 10*/
	TMRIO_10,
/** Select Timer I/O channel 11*/
	TMRIO_11,
/** Select Timer I/O channel 12*/
	TMRIO_12,
/** Select Timer I/O channel 13*/
	TMRIO_13,
/** Select Timer I/O channel 14*/
	TMRIO_14,
/** Select Timer I/O channel 15*/
	TMRIO_15,
/** Indicate Timer I/O channel max number*/
	TMRIO_MAX,
};

/** @ingroup IP_group_timerio_internal_enum
* Timer I/O CKGen setting value\n
* value is from 0:31
* Please refer to Register Map for details
*/
enum TMRIO_CKGEN {
/** Sets 16-Counter clock frequency of 26.00MHz */
	TMRIO_CKGEN_0 = 0,
/** Sets 16-Counter clock frequency of 13.00MHz */
	TMRIO_CKGEN_1,
/** Sets 16-Counter clock frequency of  6.50MHz */
	TMRIO_CKGEN_2,
/** Sets 16-Counter clock frequency of  4.33MHz */
	TMRIO_CKGEN_3,
/** Sets 16-Counter clock frequency of  3.25MHz */
	TMRIO_CKGEN_4,
/** Sets 16-Counter clock frequency of  2.60MHz */
	TMRIO_CKGEN_5,
/** Sets 16-Counter clock frequency of  2.17MHz */
	TMRIO_CKGEN_6,
/** Sets 16-Counter clock frequency of  1.86MHz */
	TMRIO_CKGEN_7,
/** Sets 16-Counter clock frequency of  1.63MHz */
	TMRIO_CKGEN_8,
/** Sets 16-Counter clock frequency of  1.44MHz */
	TMRIO_CKGEN_9,
/** Sets 16-Counter clock frequency of  1.30MHz */
	TMRIO_CKGEN_10,
/** Sets 16-Counter clock frequency of  1.18MHz */
	TMRIO_CKGEN_11,
/** Sets 16-Counter clock frequency of  1.08MHz */
	TMRIO_CKGEN_12,
/** Sets 16-Counter clock frequency of  1.00MHz */
	TMRIO_CKGEN_13,
/** Sets 16-Counter clock frequency of  0.81MHz */
	TMRIO_CKGEN_14,
/** Sets 16-Counter clock frequency of  0.41MHz */
	TMRIO_CKGEN_15,
/** Sets 16-Counter clock frequency of 32.02KHz */
	TMRIO_CKGEN_16,
/** Sets 16-Counter clock frequency of 16.01KHz */
	TMRIO_CKGEN_17,
/** Sets 16-Counter clock frequency of  8.00KHz */
	TMRIO_CKGEN_18,
/** Sets 16-Counter clock frequency of  5.34KHz */
	TMRIO_CKGEN_19,
/** Sets 16-Counter clock frequency of  4.00KHz */
	TMRIO_CKGEN_20,
/** Sets 16-Counter clock frequency of  3.20KHz */
	TMRIO_CKGEN_21,
/** Sets 16-Counter clock frequency of  2.67KHz */
	TMRIO_CKGEN_22,
/** Sets 16-Counter clock frequency of  2.29KHz */
	TMRIO_CKGEN_23,
/** Sets 16-Counter clock frequency of  2.00KHz */
	TMRIO_CKGEN_24,
/** Sets 16-Counter clock frequency of  1.78KHz */
	TMRIO_CKGEN_25,
/** Sets 16-Counter clock frequency of  1.60KHz */
	TMRIO_CKGEN_26,
/** Sets 16-Counter clock frequency of  1.46KHz */
	TMRIO_CKGEN_27,
/** Sets 16-Counter clock frequency of  1.33KMHz */
	TMRIO_CKGEN_28,
/** Sets 16-Counter clock frequency of  1.23KHz */
	TMRIO_CKGEN_29,
/** Sets 16-Counter clock frequency of  1.00KHz */
	TMRIO_CKGEN_30,
/** Sets 16-Counter clock frequency of  0.50KHz */
	TMRIO_CKGEN_31
};

/** @ingroup IP_group_timerio_internal_enum
* Timer I/O Debounce time setting value\n
* value is from 0 to 9.
* Please refer to Register Map for details
*/
enum TMRIO_DEB {
/** Sets input signal debounce time of 0.125ms */
	TMRIO_DEB_0 = 0,
/** Sets input signal debounce time of 0.25ms */
	TMRIO_DEB_1,
/** Sets input signal debounce time of 0.50ms */
	TMRIO_DEB_2,
/** Sets input signal debounce time of 1.00ms */
	TMRIO_DEB_3,
/** Sets input signal debounce time of 16ms */
	TMRIO_DEB_4,
/** Sets input signal debounce time of 32ms */
	TMRIO_DEB_5,
/** Sets input signal debounce time of 64ms */
	TMRIO_DEB_6,
/** Sets input signal debounce time of 128ms */
	TMRIO_DEB_7,
/** Sets input signal debounce time of 256ms */
	TMRIO_DEB_8,
/** Sets input signal debounce time of 512ms */
	TMRIO_DEB_9,
};

int tmrio_enable(enum TMRIO_CH tmrio_ch);
int tmrio_disable(enum TMRIO_CH tmrio_ch);
int tmrio_reset(enum TMRIO_CH tmrio_ch);
int tmrio_reset_clear(enum TMRIO_CH tmrio_ch);
int tmrio_set_mode(enum TMRIO_CH tmrio_ch, enum TMRIO_MODE mode,
		     unsigned int hlp_sel, unsigned int gpio_sel);
int tmrio_ack_irq(enum TMRIO_CH tmrio_ch);
bool tmrio_is_irq_pending(enum TMRIO_CH tmrio_ch);
int tmrio_debounce_function(enum TMRIO_CH tmrio_ch, unsigned int deben,
			      enum TMRIO_DEB debouncetime);
int tmrio_ckgen_function(enum TMRIO_CH tmrio_ch, enum TMRIO_CKGEN ckgen);
int tmrio_get_cnt(enum TMRIO_CH tmrio_ch);
int tmrio_set_cmp(enum TMRIO_CH tmrio_ch, unsigned short comp);
int tmrio_get_cmp(enum TMRIO_CH tmrio_ch);

#endif
