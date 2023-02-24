/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	Bibby Hsieh <bibby.hsieh@mediatek.com>
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
 * @file mtk_resizer_ioctl.h
 * Heander of mtk_resizer.c
 */

#ifndef MTK_RESIZER_IOCTL_H
#define MTK_RESIZER_IOCTL_H

/** @ingroup IP_group_resizer_external_struct
 * @brief Resizer ioctl structure
 */
struct rsz_scale_config {
	/** reszier enable */
	unsigned char	rsz_enable;
	/** horizontal enable */
	unsigned char	hor_enable;
	/** vertical enable */
	unsigned char	ver_enable;
	/** scaling up or down */
	unsigned char	scaling_up;
	/** horizontal algorithm select */
	unsigned char	hor_algo;
	/** vertical algorithm select */
	unsigned char	ver_algo;
	/** horizontal table select */
	unsigned char	hor_table;
	/** vertical table select */
	unsigned char	ver_table;
	/** coefficient step x */
	unsigned int	coeff_step_x;
	/** coefficient step y */
	unsigned int	coeff_step_y;
	/** in-band-signal-enhancement enable */
	unsigned char	ibse_enable;
	/** tap-adaptive enable */
	unsigned char	tap_adapt_enable;

	/** truncation of horizontal coefficients */
	unsigned char	hor_trunc_bit;
	/** truncation of vertical coefficients */
	unsigned char	ver_trunc_bit;

	/** PQ parameter */
	unsigned char	pq_g;
	/** in-band-signal-enhancement gain value */
	unsigned char	ibse_gaincontrol_gain;
	/** PQ parameter  */
	unsigned char	pq_t;
	/** PQ parameter */
	unsigned char	pq_r;
	/** Edge-preserving interpolation level */
	unsigned char	tap_adapt_slope;
};

/** @ingroup IP_group_resizer_external_enum
 * @brief ioctl command number
 */
enum MTK_RESIZER_IOCTL {
	MTK_RESIZER_IOCTL_SCALE_CONFIG,
};

/** @ingroup IP_group_resizer_external_def
 * @brief ioctl function define
 * @{
 */
#define MTK_RESIZER_MAGIC		'R'
#define IOCTL_RESIZER_SCALE_CONFIG _IOW(MTK_RESIZER_MAGIC, \
					MTK_RESIZER_IOCTL_SCALE_CONFIG, \
					struct rsz_scale_config)
/**
 * @}
 */

#endif
