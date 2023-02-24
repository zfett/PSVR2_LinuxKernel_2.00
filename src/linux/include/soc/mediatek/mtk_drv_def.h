/*
 * Copyright (c) 2017 MediaTek Inc.
 * Authors:
 *	Panda Lee <panda.lee@mediatek.com>
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
 * @file mtk_drv_def.h
 */

/**
 * @defgroup IP_group_mm_module MM_MODULE
 *
 *   @{
 *       @defgroup IP_group_mm_module_external EXTERNAL
 *         The external API document for mm_module. \n
 *
 *         @{
 *            @defgroup IP_group_mm_module_external_function 1.function
 *              none. Native Linux Driver.
 *            @defgroup IP_group_mm_module_external_struct 2.structure
 *              none. Native Linux Driver.
 *            @defgroup IP_group_mm_module_external_typedef 3.typedef
 *              none. Native Linux Driver.
 *            @defgroup IP_group_mm_module_external_enum 4.enumeration
 *              none. Native Linux Driver.
 *            @defgroup IP_group_mm_module_external_def 5.define
 *              none. Native Linux Driver.
 *         @}
 *
 *       @defgroup IP_group_mm_module_internal INTERNAL
 *         The internal API document for mm_module. \n
 *
 *         @{
 *            @defgroup IP_group_mm_module_internal_function 1.function
 *              Internal function for Linux interrupt framework and module init.
 *            @defgroup IP_group_mm_module_internal_struct 2.structure
 *              Internal structure used for mm_module.
 *            @defgroup IP_group_mm_module_internal_typedef 3.typedef
 *              none. Follow Linux coding style, no typedef used.
 *            @defgroup IP_group_mm_module_internal_enum 4.enumeration
 *              none. No enumeration in mm_module driver.
 *            @defgroup IP_group_mm_module_internal_def 5.define
 *              none. No extra define in mm_module driver.
 *         @}
 *     @}
 */

#ifndef MTK_DRV_DEF_H
#define MTK_DRV_DEF_H

#ifdef CONFIG_MTK_DEBUGFS
extern struct dentry *mtk_debugfs_root;
#endif

#define MTK_MM_MODULE_MAX_NUM	4

#define CLOCK_26M		26
#define MM_CLOCK		320

unsigned int mtk_ddp_fmt_bpp_in_byte(unsigned int fmt);

/** @ingroup IP_group_mm_module_external_struct
* @brief Callback Data.
*/
struct mtk_mmsys_cb_data {
	/** module status, refer to module private definition */
	u32	status;
	/** hardware partition index */
	u32	part_id;
	/** callback private data */
	void	*priv_data;
};

/** @ingroup IP_group_mm_module_external_typedef
* @brief Callback Function Pointer.
*/
typedef void (*mtk_mmsys_cb)(struct mtk_mmsys_cb_data *data);

/** @ingroup IP_group_mm_module_external_struct
 * @brief Positions and height of module in/out Data Structure.
 */
struct mtk_mm_frame_quarter_config {
	/** input start positions */
	int in_x_left[4];
	/** input end positions */
	int in_x_right[4];
	/** input width */
	int in_width;
	/** input height */
	int in_height;

	/** output start positions */
	int out_x_left[4];
	/** output end positions */
	int out_x_right[4];
	/** output height */
	int out_height;

	/** luma start positions offset */
	int luma_x_offset[4];
	/** luma start positions sub-offset */
	int luma_x_subpixel[4];

	/** multiple partitions */
	u32 partition_cnt;
};

#endif
