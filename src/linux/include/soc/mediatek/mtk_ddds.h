/*
 * Copyright (c) 2015 MediaTek Inc.
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
 * @file mtk_ddds.h
 * Header of mtk_ddds.c
 */

#ifndef _MTK_DDDS_H_
#define _MTK_DDDS_H_

/** @ingroup IP_group_ddds_external_enum
 * @brief DDDS frame tracking region
 */
enum mtk_ddds_frmtck_region {
	/** 0: Configure DDDS lock region */
	DDDS_FRAMETRACK_LOCK,
	/** 1: Configure DDDS fast region */
	DDDS_FRAMETRACK_FAST1,
	/** 2: Configure DDDS very fast region */
	DDDS_FRAMETRACK_FAST2,
	/** 3: Configure DDDS slow region */
	DDDS_FRAMETRACK_SLOW1,
	/** 4: Configure DDDS very slow region */
	DDDS_FRAMETRACK_SLOW2
};

int mtk_ddds_set_target_freq(const struct device *dev, u32 target_freq);
int mtk_ddds_target_freq_adjust(const struct device *dev, s8 adj_cw);
int mtk_ddds_set_err_limit(const struct device *dev, u32 max_freq);
int mtk_ddds_set_hlen(const struct device *dev,
		      enum mtk_ddds_frmtck_region rgn, u32 hsync_freq);
int mtk_ddds_frmtrk_mode(const struct device *dev, bool enable);
int mtk_ddds_close_loop(const struct device *dev, bool enable);
bool mtk_ddds_locked(const struct device *dev);
int mtk_ddds_enable(const struct device *dev, bool enable);
int mtk_ddds_refclk_out(const struct device *dev, bool xtal, bool enable);

int mtk_ddds_dump_registers(const struct device *dev);

#endif
