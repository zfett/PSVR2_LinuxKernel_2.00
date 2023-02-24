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
 * @file mtk_dsi_int.h
 * Internal header of mtk_dsi.c
 */

#ifndef _MTK_DSI_INT_H_
#define _MTK_DSI_INT_H_

#include <linux/soc/mediatek/mtk-cmdq.h>
#include <soc/mediatek/mtk_dsi.h>
#include <soc/mediatek/mtk_dsc.h>

/** @ingroup IP_group_dsi_internal_def_dsi
 * @{
 */
/** maximum number of dsi module */
#define DSI_MAX_HW_NUM	4
/**
 * @}
 */

/** @ingroup IP_group_dsi_internal_struct_dsi
 * @brief Data Structure for DSI Driver
 */
struct mtk_dsi {
	/** point to dsi device struct */
	struct device *dev;
	/** point to mmsys config device struct */
	struct device *mmsys_cfg_dev;
	/** store 4 dsi register base address */
	void __iomem *regs[DSI_MAX_HW_NUM];

	/** point to mipi tx phy struct */
	struct phy *phy;

#ifdef CONFIG_COMMON_CLK_MT3612
	/** point to dsi mm clk struct */
	struct clk *mm_clk[DSI_MAX_HW_NUM];
	/** point to dsi dsick struct */
	struct clk *dsi_clk[DSI_MAX_HW_NUM];
#endif

	/** dsi active hw number, range is 1~4 from device tree */
	u32 hw_nr;
	/** fpga mode should be 1 when driver runs on fpga */
	u32 fpga_mode;
	/** dsi data rate, unit is Mb/lane */
	u32 data_rate;
	/** LP to HS initial timing */
	u32 data_init_cycle;
	/** dsi support max fps, set by panel driver */
	u32 max_fps;
	/** ssc config data */
	int ssc_data;
	/** panel select(0: 35595, 1: 35695b, 2: ams336rp01, ...) */
	u32 panel_sel;
	/** data use by gce driver */
	u32 gce_subsys;
	/** data use by gce driver */
	u32 gce_subsys_offset[DSI_MAX_HW_NUM];

	/** dsi display mode config data */
	unsigned long mode_flags;
	/** to get or set pixel format */
	enum mtk_dsi_pixel_format format;
	/** to get or set video mode information */
	struct videomode vm;
	/** to get dsc configuration */
	struct dsc_config dsc_cfg;

	/** dsi active lane number, range is 1~4 from device tree */
	u32 lanes;

	/** dsi enable or not */
	bool enabled;
	/** dsi power on or not */
	bool poweron;
	/** dsi irq data */
	int irq_data;

#if defined(CONFIG_DEBUG_FS)
	/** dsi debugfs data */
	struct dentry *debugfs;
#endif
};

void mtk_dsi_dump_registers(const struct mtk_dsi *dsi);

/* for panel use */
void mtk_dsi_panel_reset(const struct mtk_dsi *dsi);
ssize_t mtk_dsi_host_write_cmd(const struct mtk_dsi *dsi, u32 cmd, u8 count,
			       const u8 *para_list);

/* for dsi init use, panel driver provide */
void push_table(const struct mtk_dsi *dsi,
		const struct lcm_setting_table *table, unsigned int count);
void push_table2(const struct mtk_dsi *dsi,
		 const struct lcm_setting_table *table, unsigned int count);
void lcm_init(const struct mtk_dsi *dsi);
void lcm_init2(const struct mtk_dsi *dsi);
void lcm_get_params(struct mtk_dsi *dsi);

#endif
