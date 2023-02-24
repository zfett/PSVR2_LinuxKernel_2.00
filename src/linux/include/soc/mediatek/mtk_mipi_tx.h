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
 * @file mtk_mipi_tx.h
 * Header of mtk_mipi_tx.c
 */

#ifndef _MTK_MIPI_TX_H_
#define _MTK_MIPI_TX_H_

/** @ingroup IP_group_mipi_tx_external_def
 * @{
 */
/** maximum number of mipi tx module */
#define MIPITX_MAX_HW_NUM	4
/** mipi tx CD-phy constant definition */
#define MTK_MIPI_DPHY	0
/** mipi tx CD-phy constant definition */
#define MTK_MIPI_CPHY	1
/**
 * @}
 */

/** @ingroup IP_group_mipi_tx_external_struct
 * @brief Data Structure for MIPI TX Driver
 */
struct mtk_mipi_tx {
	/** mipi tx register base address */
	void __iomem *regs[MIPITX_MAX_HW_NUM];
	/** number of mipi tx module */
	u32 hw_nr;
	/** fpga mode should be 1 when driver runs on fpga */
	u32 fpga_mode;
	/** mipi tx phy type: D-PHY or C-PHY */
	u32 phy_type;
	/** lane swap enable or not */
	u32 lane_swap;
	/** ssc mode */
	u32 ssc_mode;
	/** ssc deviation */
	u32 ssc_dev;
	/** data transfer rate per lane */
	u32 data_rate;
};

#endif
