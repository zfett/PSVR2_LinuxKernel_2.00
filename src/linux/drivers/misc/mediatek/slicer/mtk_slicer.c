/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	Wilson Huang <wilson.huang@mediatek.com>
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
 * @file mtk_slicer.c
 * This slicer driver is used to control MTK slicer hardware module.\n
 * It includes DP video and DSC path selection, frame and slice configuration.
 */

/**
 * @defgroup IP_group_slicer SLICER
 *     Slicer driver includes DP video and DSC path selection,
 *     frame and slice configuration.
 *
 *   @{
 *       @defgroup IP_group_slicer_external EXTERNAL
 *         The external API document for slicer. \n
 *
 *         @{
 *            @defgroup IP_group_slicer_external_function 1.function
 *              This is slicer external API.
 *            @defgroup IP_group_slicer_external_struct 2.structure
 *              This is slicer external structure.
 *            @defgroup IP_group_slicer_external_typedef 3.typedef
 *              This is slicer external typedef.
 *            @defgroup IP_group_slicer_external_enum 4.enumeration
 *              This is slicer external enumeration.
 *            @defgroup IP_group_slicer_external_def 5.define
 *              This is slicer external define.
 *         @}
 *
 *       @defgroup IP_group_slicer_internal INTERNAL
 *         The internal API document for slicer. \n
 *
 *         @{
 *            @defgroup IP_group_slicer_internal_function 1.function
 *              Internal function used for slicer.
 *            @defgroup IP_group_slicer_internal_struct 2.structure
 *              Internal structure used for slicer.
 *            @defgroup IP_group_slicer_internal_typedef 3.typedef
 *              Internal typedef used for slicer.
 *            @defgroup IP_group_slicer_internal_enum 4.enumeration
 *              Internal enumeration used for slicer.
 *            @defgroup IP_group_slicer_internal_def 5.define
 *              Internal define used for slicer.
 *         @}
 *     @}
 */

#include <mtk_slicer_reg.h>
#include <soc/mediatek/mtk_slicer.h>
#include <soc/mediatek/mtk_dprx_info.h>
#include <soc/mediatek/mtk_dprx_if.h>
#include <soc/mediatek/mtk_clk-mt3612.h>
#include <asm-generic/io.h>
#include <linux/compiler.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/of_device.h>
#include <linux/seq_file.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#ifdef CONFIG_MTK_DEBUGFS
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#endif

#ifdef CONFIG_MTK_DEBUGFS
/** @ingroup IP_group_slicer_internal_struct
 * @brief dentry structure for debugfs use.
 */
struct dentry *mtk_slcr_debugfs_root;

/** @ingroup IP_group_slicer_internal_def
 * @brief Check bit macro for debugfs use.
 */
#define CHECK_BIT(var, pos) ((var) & (1 << (pos)))
#endif

/** @ingroup IP_group_slicer_internal_def
 * @brief Check value in range.
 */
#define INRANGE(x, var) (((x) >= (var)) ? true : false)

/** @ingroup IP_group_slicer_internal_struct
 * @brief Slicer configuration data structure for driver internal use.
 */
struct mtk_slcr_reg_cfg {
	/** Slicer input selection. */
	enum slcr_in_format in_format;
	/** Slicer 3D re-shaped control register 0~3 */
	u32 slcr_3d_ctrl[4];
	/** Slicer dummy line control register 0~8 */
	u32 slcr_dmyl_ctrl[8];
	/** Slicer DP video control register 0~8 */
	u32 slcr_vid_ctrl[9];
	/** Slicer DP DSC control register 0~8 */
	u32 slcr_dsc_ctrl[9];
	/** Slicer GCE control register */
	u32 slcr_gce_ctrl[4];
	/** Slicer pattern generator control register */
	u32 slcr_pg_ctrl[15];
};

/** @ingroup IP_group_slicer_internal_struct
 * @brief Slicer color space conversion data structure for driver internal use.
 */
struct mtk_slcr_csc_set {
	/** Color space conversion enable */
	bool enable;
	/** Color space conversion mode */
	enum slcr_csc_mode csc_mode;
};

/** @ingroup IP_group_slicer_internal_struct
 * @brief Slicer dither data structure for driver internal use.
 */
struct mtk_slcr_dither_set {
	/** Dither enable */
	bool enable;
	/** Dither type */
	enum slcr_dither_type dithertype;
	/** Dither mode */
	enum slcr_dither_mode dithermode;
};

/** @ingroup IP_group_slicer_internal_struct
 * @brief Slicer driver data structure for driver internal use.
 */
struct mtk_slcr {
	struct device *dev;
	void __iomem *regs;
	u32 cmdq_subsys;
	u32 cmdq_offset;
#ifdef CONFIG_COMMON_CLK_MT3612
	/** Point to top video clk select struct */
	struct clk *slcr_vid_sel;
	/** Point to top dsc clk select struct */
	struct clk *slcr_dsc_sel;
	/** Point to top clk 26M struct */
	struct clk *clk26m_ck;
	/** Point to top clk dprx video struct */
	struct clk *dprx_vid_ck;
	/** Point to top clk dprx dsc struct */
	struct clk *dprx_dsc_ck;
	/** Point to top clk univpll d2 struct */
	struct clk *univpll_d2;
	/** Point to core clk gate struct */
	struct clk *core_slcr[4];
	/** Point to mm clk gate struct */
	struct clk *slcr_mm;
	/** Point to video clk gate struct */
	struct clk *slcr_vid;
	/** Point to dsc clk gate struct */
	struct clk *slcr_dsc;
#endif
};

/** @ingroup IP_group_slicer_internal_struct
 * @brief Slicer register data structure for driver internal use.
 */
struct reg_info {
	void __iomem *regs;
	u32 cmdq_subsys;
	u32 cmdq_offset;
	struct cmdq_pkt *pkt;
};

/** @ingroup IP_group_slicer_internal_function
 * @par Description
 *     Setup slicer register with mask.
 * @param[in]
 *     rif: Slicer register data structure.
 * @param[in]
 *     value: Value to be set.
 * @param[in]
 *     offset: Register offset.
 * @param[in]
 *     mask: Register mask.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void reg_write_mask(const struct reg_info rif,
			   const u32 value,
			   const u32 offset,
			   const u32 mask)
{
	if (rif.pkt) {
		cmdq_pkt_write_value(rif.pkt, rif.cmdq_subsys,
				     rif.cmdq_offset | offset, value, mask);
	} else {
		u32 reg;

		reg = readl(rif.regs + offset);
		reg &= ~mask;
		reg |= (value & mask);
		writel(reg, rif.regs + offset);
	}
}

/** @ingroup IP_group_slicer_internal_function
 * @par Description
 *     Setup slicer register.
 * @param[in]
 *     rif: Slicer register data structure.
 * @param[in]
 *     value: Value to be set.
 * @param[in]
 *     offset: Register offset.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void reg_write(const struct reg_info rif,
		      const u32 value,
		      const u32 offset)
{
	if (rif.pkt)
		reg_write_mask(rif, value, offset, 0xffffffff);
	else
		writel(value, rif.regs + offset);
}

/** @ingroup IP_group_slicer_internal_function
 * @par Description
 *     Setup register of mtk_slcr_config().
 * @param[in]
 *     dev: Slicer driver data structure pointer.
 * @param[in]
 *     cmdq_handle: Command queue handle pointer.
 * @param[in]
 *     reg_cfg: Slicer parameter data structure pointer.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_slcr_config_reg(const struct device *dev,
				struct cmdq_pkt **cmdq_handle,
				const struct mtk_slcr_reg_cfg *reg_cfg)
{
	struct mtk_slcr *slcr = dev_get_drvdata(dev);
	struct reg_info rif;

	if (cmdq_handle == NULL)
		rif.pkt = NULL;
	else
		rif.pkt = cmdq_handle[0];

	rif.regs = slcr->regs;
	rif.cmdq_subsys = slcr->cmdq_subsys;
	rif.cmdq_offset = slcr->cmdq_offset;

	switch (reg_cfg->in_format) {
	case DP_VIDEO:
		reg_write(rif, reg_cfg->slcr_vid_ctrl[0], SLCR_VID_CTRL_0);
		reg_write(rif, reg_cfg->slcr_dsc_ctrl[0], SLCR_DSC_CTRL_0);
		reg_write(rif, reg_cfg->slcr_vid_ctrl[1], SLCR_VID_CTRL_1);
		reg_write(rif, reg_cfg->slcr_vid_ctrl[2], SLCR_VID_CTRL_2);
		reg_write(rif, reg_cfg->slcr_vid_ctrl[3], SLCR_VID_CTRL_3);
		reg_write(rif, reg_cfg->slcr_vid_ctrl[4], SLCR_VID_CTRL_4);
		reg_write(rif, reg_cfg->slcr_vid_ctrl[5], SLCR_VID_CTRL_5);
		reg_write(rif, reg_cfg->slcr_vid_ctrl[6], SLCR_VID_CTRL_6);
		reg_write(rif, reg_cfg->slcr_vid_ctrl[7], SLCR_VID_CTRL_7);
		reg_write(rif, reg_cfg->slcr_vid_ctrl[8], SLCR_VID_CTRL_8);
		break;
	case DP_DSC:
		reg_write(rif, reg_cfg->slcr_vid_ctrl[0], SLCR_VID_CTRL_0);
		reg_write(rif, reg_cfg->slcr_dsc_ctrl[0], SLCR_DSC_CTRL_0);
		reg_write(rif, reg_cfg->slcr_dsc_ctrl[1], SLCR_DSC_CTRL_1);
		reg_write(rif, reg_cfg->slcr_dsc_ctrl[2], SLCR_DSC_CTRL_2);
		reg_write(rif, reg_cfg->slcr_dsc_ctrl[3], SLCR_DSC_CTRL_3);
		reg_write(rif, reg_cfg->slcr_dsc_ctrl[4], SLCR_DSC_CTRL_4);
		reg_write(rif, reg_cfg->slcr_dsc_ctrl[5], SLCR_DSC_CTRL_5);
		reg_write(rif, reg_cfg->slcr_dsc_ctrl[6], SLCR_DSC_CTRL_6);
		reg_write(rif, reg_cfg->slcr_dsc_ctrl[7], SLCR_DSC_CTRL_7);
		reg_write(rif, reg_cfg->slcr_dsc_ctrl[8], SLCR_DSC_CTRL_8);
		break;
	case DP_VIDEO_DSC:
		reg_write(rif, reg_cfg->slcr_vid_ctrl[0], SLCR_VID_CTRL_0);
		reg_write(rif, reg_cfg->slcr_vid_ctrl[1], SLCR_VID_CTRL_1);
		reg_write(rif, reg_cfg->slcr_vid_ctrl[2], SLCR_VID_CTRL_2);
		reg_write(rif, reg_cfg->slcr_vid_ctrl[3], SLCR_VID_CTRL_3);
		reg_write(rif, reg_cfg->slcr_vid_ctrl[4], SLCR_VID_CTRL_4);
		reg_write(rif, reg_cfg->slcr_vid_ctrl[5], SLCR_VID_CTRL_5);
		reg_write(rif, reg_cfg->slcr_vid_ctrl[6], SLCR_VID_CTRL_6);
		reg_write(rif, reg_cfg->slcr_vid_ctrl[7], SLCR_VID_CTRL_7);
		reg_write(rif, reg_cfg->slcr_vid_ctrl[8], SLCR_VID_CTRL_8);
		reg_write(rif, reg_cfg->slcr_dsc_ctrl[0], SLCR_DSC_CTRL_0);
		reg_write(rif, reg_cfg->slcr_dsc_ctrl[1], SLCR_DSC_CTRL_1);
		reg_write(rif, reg_cfg->slcr_dsc_ctrl[2], SLCR_DSC_CTRL_2);
		reg_write(rif, reg_cfg->slcr_dsc_ctrl[3], SLCR_DSC_CTRL_3);
		reg_write(rif, reg_cfg->slcr_dsc_ctrl[4], SLCR_DSC_CTRL_4);
		reg_write(rif, reg_cfg->slcr_dsc_ctrl[5], SLCR_DSC_CTRL_5);
		reg_write(rif, reg_cfg->slcr_dsc_ctrl[6], SLCR_DSC_CTRL_6);
		reg_write(rif, reg_cfg->slcr_dsc_ctrl[7], SLCR_DSC_CTRL_7);
		reg_write(rif, reg_cfg->slcr_dsc_ctrl[8], SLCR_DSC_CTRL_8);
		break;
	}

	reg_write(rif, reg_cfg->slcr_gce_ctrl[0], SLCR_GCE_0);
	reg_write(rif, reg_cfg->slcr_gce_ctrl[1], SLCR_GCE_1);
	reg_write(rif, reg_cfg->slcr_gce_ctrl[2], SLCR_GCE_2);
	reg_write(rif, reg_cfg->slcr_gce_ctrl[3], SLCR_GCE_3);

	reg_write(rif, reg_cfg->slcr_3d_ctrl[0], SLCR_3D_CTRL_0);
	reg_write(rif, reg_cfg->slcr_3d_ctrl[1], SLCR_3D_CTRL_1);
	reg_write(rif, reg_cfg->slcr_3d_ctrl[2], SLCR_3D_CTRL_2);
	reg_write(rif, reg_cfg->slcr_3d_ctrl[3], SLCR_3D_CTRL_3);

	reg_write(rif, reg_cfg->slcr_dmyl_ctrl[0], SLCR_DMYL_0);
	reg_write(rif, reg_cfg->slcr_dmyl_ctrl[1], SLCR_DMYL_1);
	reg_write(rif, reg_cfg->slcr_dmyl_ctrl[2], SLCR_DMYL_2);
	reg_write(rif, reg_cfg->slcr_dmyl_ctrl[3], SLCR_DMYL_3);
	reg_write(rif, reg_cfg->slcr_dmyl_ctrl[4], SLCR_DMYL_4);
	reg_write(rif, reg_cfg->slcr_dmyl_ctrl[5], SLCR_DMYL_5);
	reg_write(rif, reg_cfg->slcr_dmyl_ctrl[6], SLCR_DMYL_6);
	reg_write(rif, reg_cfg->slcr_dmyl_ctrl[7], SLCR_DMYL_7);

}

/** @ingroup IP_group_slicer_internal_function
 * @par Description
 *     Setup register of mtk_slcr_patgen_config().
 * @param[in]
 *     dev: Slicer driver data structure pointer.
 * @param[in]
 *     cmdq_handle: Command queue handle pointer.
 * @param[in]
 *     reg_cfg: Slicer parameter data structure pointer.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_slcr_config_patgen_reg(const struct device *dev,
				       struct cmdq_pkt **cmdq_handle,
				       const struct mtk_slcr_reg_cfg *reg_cfg)
{
	struct mtk_slcr *slcr = dev_get_drvdata(dev);
	struct reg_info rif;

	if (cmdq_handle == NULL)
		rif.pkt = NULL;
	else
		rif.pkt = cmdq_handle[0];

	rif.regs = slcr->regs;
	rif.cmdq_subsys = slcr->cmdq_subsys;
	rif.cmdq_offset = slcr->cmdq_offset;

	reg_write(rif, reg_cfg->slcr_pg_ctrl[0], SLCR_PATG_2);
	reg_write(rif, reg_cfg->slcr_pg_ctrl[1], SLCR_PATG_3);
	reg_write(rif, reg_cfg->slcr_pg_ctrl[2], SLCR_PATG_4);
	reg_write(rif, reg_cfg->slcr_pg_ctrl[3], SLCR_PATG_5);
	reg_write(rif, reg_cfg->slcr_pg_ctrl[4], SLCR_PATG_6);
	reg_write(rif, reg_cfg->slcr_pg_ctrl[5], SLCR_PATG_7);
	reg_write(rif, reg_cfg->slcr_pg_ctrl[6], SLCR_PATG_8);
	reg_write(rif, reg_cfg->slcr_pg_ctrl[7], SLCR_PATG_9);
	reg_write(rif, reg_cfg->slcr_pg_ctrl[8], SLCR_PATG_10);
	reg_write(rif, reg_cfg->slcr_pg_ctrl[9], SLCR_PATG_12);
	reg_write(rif, reg_cfg->slcr_pg_ctrl[10], SLCR_PATG_13);
	reg_write(rif, reg_cfg->slcr_pg_ctrl[11], SLCR_PATG_14);
	reg_write(rif, reg_cfg->slcr_pg_ctrl[12], SLCR_PATG_16);
	reg_write(rif, reg_cfg->slcr_pg_ctrl[13], SLCR_PATG_17);
	reg_write(rif, reg_cfg->slcr_pg_ctrl[14], SLCR_PATG_18);
}

/** @ingroup IP_group_slicer_internal_function
 * @par Description
 *     Setup color space conversion register of mtk_slcr_config().
 * @param[in]
 *     dev: Slicer driver data structure pointer.
 * @param[in]
 *     cmdq_handle: Command queue handle pointer.
 * @param[in]
 *     slcr_csc: Slicer color space conversion parameter data structure pointer.
 * @return
 *     0, configuration is done.
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_slcr_config_csc_reg(const struct device *dev,
				   struct cmdq_pkt **cmdq_handle,
				   const struct mtk_slcr_csc_set *slcr_csc)
{
	struct mtk_slcr *slcr = dev_get_drvdata(dev);
	struct reg_info rif;

	if (cmdq_handle == NULL)
		rif.pkt = NULL;
	else
		rif.pkt = cmdq_handle[0];

	rif.regs = slcr->regs;
	rif.cmdq_subsys = slcr->cmdq_subsys;
	rif.cmdq_offset = slcr->cmdq_offset;

	if (slcr_csc->enable == true) {
		/* Enable color space conversion */
		reg_write_mask(rif, (0x1 << 0),	SLCR_CSC_0, CONVERSION_EN);
		dev_dbg(dev, "Enable slicer color space conversion.\n");
	} else {
		/* Disable color space conversion */
		reg_write_mask(rif, (0x0 << 0),	SLCR_CSC_0, CONVERSION_EN);
		dev_dbg(dev, "Disable slicer color space conversion.\n");
		return 0;
	}

	if ((slcr_csc->csc_mode < MTX_CSC_MAX) &&
	    (slcr_csc->csc_mode != MTX_CSC_RESERVED_0) &&
	    (slcr_csc->csc_mode != MTX_CSC_RESERVED_1)) {
		reg_write_mask(rif, (slcr_csc->csc_mode << 16),
				SLCR_CSC_0, INT_MATRIX_SEL);
	} else {
		dev_err(dev, "Slicer color space conversion matrix error: %d\n",
			slcr_csc->csc_mode);
		return -EINVAL;
	}

	dev_dbg(dev, "Slicer color space conversion matrix: %d\n",
		slcr_csc->csc_mode);
	return 0;
}

/** @ingroup IP_group_slicer_internal_function
 * @par Description
 *     Setup dither register of mtk_slcr_config().
 * @param[in]
 *     dev: Slicer driver data structure pointer.
 * @param[in]
 *     cmdq_handle: Command queue handle pointer.
 * @param[in]
 *     slcrdither: Slicer dither parameter data structure pointer.
 * @return
 *     0, configuration is done.
 *     -EINVAL, unknown dither type or mode.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_slcr_dither_config_reg(const struct device *dev,
				struct cmdq_pkt **cmdq_handle,
				const struct mtk_slcr_dither_set *slcrdither)
{
	u8 DrMode;
	struct mtk_slcr *slcr = dev_get_drvdata(dev);
	struct reg_info rif;

	if (cmdq_handle == NULL)
		rif.pkt = NULL;
	else
		rif.pkt = cmdq_handle[0];

	rif.regs = slcr->regs;
	rif.cmdq_subsys = slcr->cmdq_subsys;
	rif.cmdq_offset = slcr->cmdq_offset;

	if (slcrdither->enable == false) {
		/* Disable dither */
		dev_dbg(dev, "Disable slicer dither.\n");
		reg_write_mask(rif, (0x0 << 0),
				SLCR_DITHER_0, ROUNDED_EN);
		reg_write_mask(rif, (0x0 << 4),
				SLCR_DITHER_0, RDITHER_EN);
		reg_write_mask(rif, (0x0 << 8),
				SLCR_DITHER_0, LFSR_EN);
		reg_write_mask(rif, (0x0 << 12),
				SLCR_DITHER_0, EDITHER_EN);
	} else {
		/* Enable dither */
		dev_dbg(dev, "Enable slicer dither.\n");
		switch (slcrdither->dithertype) {
		case E_DITHER:
			reg_write_mask(rif, (0x0 << 0),
				SLCR_DITHER_0, ROUNDED_EN);
			reg_write_mask(rif, (0x0 << 4),
				SLCR_DITHER_0, RDITHER_EN);
			reg_write_mask(rif, (0x0 << 8),
				SLCR_DITHER_0, LFSR_EN);
			reg_write_mask(rif, (0x1 << 12),
				SLCR_DITHER_0, EDITHER_EN);
			break;
		case LFSR_DITHER:
			reg_write_mask(rif, (0x0 << 0),
				SLCR_DITHER_0, ROUNDED_EN);
			reg_write_mask(rif, (0x0 << 4),
				SLCR_DITHER_0, RDITHER_EN);
			reg_write_mask(rif, (0x1 << 8),
				SLCR_DITHER_0, LFSR_EN);
			reg_write_mask(rif, (0x0 << 12),
				SLCR_DITHER_0, EDITHER_EN);
			break;
		case R_DITHER:
			reg_write_mask(rif, (0x0 << 0),
				SLCR_DITHER_0, ROUNDED_EN);
			reg_write_mask(rif, (0x1 << 4),
				SLCR_DITHER_0, RDITHER_EN);
			reg_write_mask(rif, (0x0 << 8),
				SLCR_DITHER_0, LFSR_EN);
			reg_write_mask(rif, (0x0 << 12),
				SLCR_DITHER_0, EDITHER_EN);
			break;
		case ROUND_DITHER:
			reg_write_mask(rif, (0x1 << 0),
				SLCR_DITHER_0, ROUNDED_EN);
			reg_write_mask(rif, (0x0 << 4),
				SLCR_DITHER_0, RDITHER_EN);
			reg_write_mask(rif, (0x0 << 8),
				SLCR_DITHER_0, LFSR_EN);
			reg_write_mask(rif, (0x0 << 12),
				SLCR_DITHER_0, EDITHER_EN);
			break;
		default:
			/* Disable dither */
			dev_err(dev, "Unknown dither type, dither off\n");
			reg_write_mask(rif, (0x0 << 0),
				SLCR_DITHER_0, ROUNDED_EN);
			reg_write_mask(rif, (0x0 << 4),
				SLCR_DITHER_0, RDITHER_EN);
			reg_write_mask(rif, (0x0 << 8),
				SLCR_DITHER_0, LFSR_EN);
			reg_write_mask(rif, (0x0 << 12),
				SLCR_DITHER_0, EDITHER_EN);
			return -EINVAL;
		}

		switch (slcrdither->dithermode) {
		case DITHER_BIT12_TO_10:
			DrMode = 1;
			break;
		case DITHER_BIT12_TO_8:
			DrMode = 2;
			break;
		case DITHER_BIT12_TO_6:
			DrMode = 3;
			break;
		case NO_DITHER:
			DrMode = 0;
			break;
		default:
			/* Disable dither */
			dev_err(dev, "Unknown dither mode, dither off\n");
			reg_write_mask(rif, (0x0 << 0),
				SLCR_DITHER_0, ROUNDED_EN);
			reg_write_mask(rif, (0x0 << 4),
				SLCR_DITHER_0, RDITHER_EN);
			reg_write_mask(rif, (0x0 << 8),
				SLCR_DITHER_0, LFSR_EN);
			reg_write_mask(rif, (0x0 << 12),
				SLCR_DITHER_0, EDITHER_EN);
			return -EINVAL;
		}

		reg_write_mask(rif, (DrMode << 4), SLCR_DITHER_1, DRMOD_R);
		reg_write_mask(rif, (DrMode << 8), SLCR_DITHER_1, DRMOD_G);
		reg_write_mask(rif, (DrMode << 12), SLCR_DITHER_1, DRMOD_B);
	}

	return 0;
}

#ifdef CONFIG_MACH_MT3612_A0
/** @ingroup IP_group_slicer_internal_function
 * @par Description
 *     Calculate and setup slicer clock divider parameter.
 * @param[in]
 *     dev: Slicer driver data structure pointer.
 * @param[in]
 *     h_total: horizontal total.
 * @param[in]
 *     v_total: veritical total.
 * @param[in]
 *     fps: frame rate of pattern generator
 * @return
 *     0, configuration is done.
 *     -EINVAL, no found suitable parameters.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_slcr_cal_divider_para(u16 h_total, u16 v_total, int fps)
{
	int pxl_clk;
	int m_mod, n_mod;
	int calv, diff;
	unsigned int idx = 0;
	unsigned int i;
	int m_set[5] = {0};
	int n_set[5] = {0};
	int diff_set[5] = {0};
	int limit = 2000;
	int diff_min = limit;
	int ret = 1;

	pxl_clk = (((h_total/2) * v_total) / 100) * (fps/10);

	for (n_mod = 1; n_mod < 1024; n_mod++) {
		for (m_mod = 1; m_mod < 1024; m_mod++) {
			calv = (624000000 / m_mod) * n_mod;
			diff = pxl_clk - calv;
			if ((diff < limit) && (diff >= 0)) {
				ret = 0;
				if (idx < 5) {
					m_set[idx] = m_mod;
					n_set[idx] = n_mod;
					diff_set[idx] = diff;
					idx++;
				}
			}
		}
	}
	for (i = 0; i < 5; i++) {
		if (m_set[i] != 0) {
			if (diff_set[i] < diff_min) {
				idx = i;
				diff_min = diff_set[i];
			}
		}
	}
	mtk_topckgen_slicer_div(1, m_set[idx], n_set[idx]);

	if (ret) {
		pr_debug("no found suitable para.\n");
		return ret;
	}
	return ret;

}
#endif

/** @ingroup IP_group_slicer_external_function
 * @par Description
 *     Setup fps of slicer pattern generator.
 * @param[in] dev: Slicer device node.
 * @param[in] h_total: horizontal total.
 * @param[in] v_total: veritical total.
 * @param[in] pg_fps: frame rate of pattern generator.
 * @return
 *     0, function success.
 *     -ENODEV, null dev pointer.
 *     -EINVAL, invalid point or invalid pg_fps.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_slcr_patgen_fps_config(
				const struct device *dev,
				u16 h_total, u16 v_total,
				enum slcr_pg_fps pg_fps)
{
	struct mtk_slcr *slcr = NULL;
	int ret = 0;

	if (!dev)
		return -EFAULT;

	slcr = dev_get_drvdata(dev);

	if (!slcr)
		return -ENODEV;

	if (!INRANGE(8191, h_total)) {
		dev_err(dev, "%s:unknown h_total!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(8191, v_total)) {
		dev_err(dev, "%s:unknown v_total!\n", __func__);
		return -EINVAL;
	}

#ifdef CONFIG_MACH_MT3612_A0
	switch (pg_fps) {
	case PAT_23_976_FPS:
		mtk_slcr_cal_divider_para(h_total, v_total, 23976);
		break;
	case PAT_50_FPS:
		mtk_slcr_cal_divider_para(h_total, v_total, 50000);
		break;
	case PAT_59_94_FPS:
		mtk_slcr_cal_divider_para(h_total, v_total, 59940);
		break;
	case PAT_60_FPS:
		mtk_slcr_cal_divider_para(h_total, v_total, 60000);
		break;
	case PAT_89_91_FPS:
		mtk_slcr_cal_divider_para(h_total, v_total, 89910);
		break;
	case PAT_90_FPS:
		mtk_slcr_cal_divider_para(h_total, v_total, 90000);
		break;
	case PAT_119_88_FPS:
		mtk_slcr_cal_divider_para(h_total, v_total, 119880);
		break;
	case PAT_120_FPS:
		mtk_slcr_cal_divider_para(h_total, v_total, 120000);
		break;
	default:
		dev_err(dev, "error patgen fps.\n");
		ret = -EINVAL;
		break;
	}
#endif
	return ret;
}
EXPORT_SYMBOL(mtk_slcr_patgen_fps_config);

/** @ingroup IP_group_slicer_internal_function
 * @par Description
 *     Select mtk slicer input clock source.
 * @param[in]
 *     dev: Slicer driver data structure pointer.
 * @param[in]
 *     clk_sel: Slicer clock mux selection.
 * @return
 *     0, function success.
 *     -EFAULT, invalid point.
 *     -EINVAL, invalid clk_sel.
 *     error code from clk_set_parent().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_slcr_config_clk_sel(
			const struct device *dev,
			enum slcr_clk_sel clk_sel)
{
	struct mtk_slcr *slcr = NULL;
#ifdef CONFIG_COMMON_CLK_MT3612
	struct clk *vid_clk;
	struct clk *dsc_clk;
#endif
	int ret = 0;

	if (!dev)
		return -EFAULT;

	slcr = dev_get_drvdata(dev);

	if (!slcr)
		return -ENODEV;
#ifdef CONFIG_COMMON_CLK_MT3612
	if (clk_sel > UNIVPLL_D2) {
		dev_err(dev, "Unknown clock selection.\n");
		return -EINVAL;
	}

	switch (clk_sel) {
	case CLK26M:
		vid_clk = slcr->clk26m_ck;
		dsc_clk = slcr->clk26m_ck;
		break;
	case DPRX_VIDEO_CK:
		vid_clk = slcr->dprx_vid_ck;
		dsc_clk = slcr->clk26m_ck;
		break;
	case DPRX_DSC_CK:
		vid_clk = slcr->clk26m_ck;
		dsc_clk = slcr->dprx_dsc_ck;
		break;
	case DPRX_VIDEO_DSC_CK:
		vid_clk = slcr->dprx_vid_ck;
		dsc_clk = slcr->dprx_dsc_ck;
		break;
	case UNIVPLL_D2:
		vid_clk = slcr->univpll_d2;
		dsc_clk = slcr->univpll_d2;
		break;
	}
	ret = clk_set_parent(slcr->slcr_vid_sel, vid_clk);
	if (ret)
		return ret;

	ret = clk_set_parent(slcr->slcr_dsc_sel, dsc_clk);
	if (ret)
		return ret;
#endif

	return ret;
}

/** @ingroup IP_group_slicer_external_function
 * @par Description
 *     Slicer clock enable/disable function.
 *     This function enable related clock for Slicer.
 * @param[in] dev: Slicer device node.
 * @param[in] enable: 0: clock disable, 1: clock enable
 * @return
 *     0, function success.
 *     -ENODEV, null dev pointer.
 *     -EINVAL, invalid point.
 *     error code from clk_prepare_enable().
 * @par Boundary case and Limitation
 *     Please call this function to
 *     enable clock before you want to use this hw or
 *     disable clock after you do not use this hw.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_slcr_clk_enable(const struct device *dev, const bool enable)
{
	struct mtk_slcr *slcr = NULL;
	int ret = 0;
#ifdef CONFIG_COMMON_CLK_MT3612
	int i;
#endif

	if (!dev)
		return -EINVAL;

	slcr = dev_get_drvdata(dev);

	if (!slcr)
		return -ENODEV;

#ifdef CONFIG_COMMON_CLK_MT3612
	if (enable) {
		ret = clk_prepare_enable(slcr->slcr_mm);
		if (ret)
			return ret;

		ret = clk_prepare_enable(slcr->slcr_vid);
		if (ret)
			return ret;

		ret = clk_prepare_enable(slcr->slcr_dsc);
		if (ret)
			return ret;

		for (i = 0; i < 4; i++) {
			ret = clk_prepare_enable(slcr->core_slcr[i]);
			if (ret)
				return ret;
		}
	} else {
		/* mux park to UNIVPLL_D2 */
		mtk_slcr_config_clk_sel(dev, UNIVPLL_D2);

		for (i = 0; i < 4; i++)
			clk_disable_unprepare(slcr->core_slcr[i]);

		clk_disable_unprepare(slcr->slcr_mm);
		clk_disable_unprepare(slcr->slcr_vid);
		clk_disable_unprepare(slcr->slcr_dsc);
	}
#endif
	return ret;
}
EXPORT_SYMBOL(mtk_slcr_clk_enable);

/** @ingroup IP_group_slicer_external_function
 * @par Description
 *     Turn on the power of SLICER.
 * @param[in] dev: Slicer device node.
 * @return
 *     1. 0, success.\n
 *     2. -EFAULT, invalid point.
 *     3. error code from mtk_slcr_clk_enable().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_slcr_power_on(struct device *dev)
{
	if (!dev)
		return -EINVAL;

	return mtk_slcr_clk_enable(dev, true);
}
EXPORT_SYMBOL(mtk_slcr_power_on);

/** @ingroup IP_group_slicer_external_function
 * @par Description
 *     Turn off the power of SLICER.
 * @param[in] dev: Slicer device node.
 * @return
 *     1. 0, success.\n
 *     2. -EFAULT, invalid point.
 *     3. error code from mtk_slcr_clk_enable().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_slcr_power_off(struct device *dev)
{
	if (!dev)
		return -EINVAL;

	return mtk_slcr_clk_enable(dev, false);
}
EXPORT_SYMBOL(mtk_slcr_power_off);

/** @ingroup IP_group_slicer_external_function
 * @par Description
 *     Enable slicer module.
 * @param[in]
 *     dev: Slicer device node.
 * @param[in]
 *     cmdq_handle: Command queue handle pointer.
 * @return
 *     0, slicer start is done.
 *     -ENODEV, null dev pointer.
 *     -EINVAL, invalid argument.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_slcr_start(const struct device *dev,
		    struct cmdq_pkt **cmdq_handle)
{
	struct mtk_slcr *slcr = NULL;
	struct reg_info rif;

	if (!dev)
		return -EINVAL;

	slcr = dev_get_drvdata(dev);

	if (!slcr)
		return -ENODEV;


	if (cmdq_handle == NULL)
		rif.pkt = NULL;
	else
		rif.pkt = cmdq_handle[0];

	rif.regs = slcr->regs;
	rif.cmdq_subsys = slcr->cmdq_subsys;
	rif.cmdq_offset = slcr->cmdq_offset;

	reg_write_mask(rif, (0x1 << 0), SLCR_ENABLE, SLCR_EN);

	return 0;
}
EXPORT_SYMBOL(mtk_slcr_start);

/** @ingroup IP_group_slicer_external_function
 * @par Description
 *     Disable slicer module.
 * @param[in]
 *     dev: Slicer device node.
 * @param[in]
 *     cmdq_handle: Command queue handle pointer.
 * @return
 *     0, slicer stop is done.
 *     -ENODEV, null dev pointer.
 *     -EINVAL, invalid argument.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_slcr_stop(const struct device *dev,
		   struct cmdq_pkt **cmdq_handle)
{
	struct mtk_slcr *slcr = NULL;
	struct reg_info rif;

	if (!dev)
		return -EINVAL;

	slcr = dev_get_drvdata(dev);

	if (!slcr)
		return -ENODEV;


	if (cmdq_handle == NULL)
		rif.pkt = NULL;
	else
		rif.pkt = cmdq_handle[0];

	rif.regs = slcr->regs;
	rif.cmdq_subsys = slcr->cmdq_subsys;
	rif.cmdq_offset = slcr->cmdq_offset;

	reg_write_mask(rif, (0x0 << 0), SLCR_ENABLE, SLCR_EN);

	return 0;
}
EXPORT_SYMBOL(mtk_slcr_stop);

/** @ingroup IP_group_slicer_external_function
 * @par Description
 *     Enable slicer pattern generator module.
 * @param[in]
 *     dev: Slicer device node.
 * @param[in]
 *     cmdq_handle: Command queue handle pointer.
 * @return
 *     0, pattern generate start is done.
 *     -ENODEV, null dev pointer.
 *     -EINVAL, invalid argument.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_slcr_patgen_start(const struct device *dev,
			   struct cmdq_pkt **cmdq_handle)
{
	struct mtk_slcr *slcr = NULL;
	struct reg_info rif;

	if (!dev)
		return -EINVAL;

	slcr = dev_get_drvdata(dev);

	if (!slcr)
		return -ENODEV;

	if (cmdq_handle == NULL)
		rif.pkt = NULL;
	else
		rif.pkt = cmdq_handle[0];

	rif.regs = slcr->regs;
	rif.cmdq_subsys = slcr->cmdq_subsys;
	rif.cmdq_offset = slcr->cmdq_offset;

	reg_write_mask(rif, (0x1 << 0), SLCR_PATG_0, PATG_EN);

	return 0;
}
EXPORT_SYMBOL(mtk_slcr_patgen_start);

/** @ingroup IP_group_slicer_external_function
 * @par Description
 *     Disable slicer pattern generator module.
 * @param[in]
 *     dev: Slicer device node.
 * @param[in]
 *     cmdq_handle: Command queue handle pointer.
 * @return
 *     0, pattern generate stop is done.
 *     -ENODEV, null dev pointer.
 *     -EINVAL, invalid argument.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_slcr_patgen_stop(const struct device *dev,
			  struct cmdq_pkt **cmdq_handle)
{
	struct mtk_slcr *slcr = NULL;
	struct reg_info rif;

	if (!dev)
		return -EINVAL;

	slcr = dev_get_drvdata(dev);

	if (!slcr)
		return -ENODEV;

	if (cmdq_handle == NULL)
		rif.pkt = NULL;
	else
		rif.pkt = cmdq_handle[0];

	rif.regs = slcr->regs;
	rif.cmdq_subsys = slcr->cmdq_subsys;
	rif.cmdq_offset = slcr->cmdq_offset;

	reg_write_mask(rif, (0x0 << 0), SLCR_PATG_0, PATG_EN);

	return 0;
}
EXPORT_SYMBOL(mtk_slcr_patgen_stop);

/** @ingroup IP_group_slicer_external_function
 * @par Description
 *     Reset slicer module.
 * @param[in]
 *     dev: Slicer device node.
 * @param[in]
 *     cmdq_handle: Command queue handle pointer.
 * @return
 *     0, slicer reset is done.
 *     -ENODEV, null dev pointer.
 *     -EINVAL, invalid argument.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_slcr_reset(const struct device *dev,
		    struct cmdq_pkt **cmdq_handle)
{
	struct mtk_slcr *slcr = NULL;
	struct reg_info rif;

	if (!dev)
		return -EINVAL;

	slcr = dev_get_drvdata(dev);

	if (!slcr)
		return -ENODEV;

	if (cmdq_handle == NULL)
		rif.pkt = NULL;
	else
		rif.pkt = cmdq_handle[0];

	rif.regs = slcr->regs;
	rif.cmdq_subsys = slcr->cmdq_subsys;
	rif.cmdq_offset = slcr->cmdq_offset;

	reg_write_mask(rif, (0x1 << 0), SLCR_RESET, SLCR_RST);
	reg_write_mask(rif, (0x0 << 0), SLCR_RESET, SLCR_RST);

	return 0;
}
EXPORT_SYMBOL(mtk_slcr_reset);

/** @ingroup IP_group_slicer_external_function
 * @par Description
 *     Reset slicer pattern generator module.
 * @param[in]
 *     dev: Slicer device node.
 * @param[in]
 *     cmdq_handle: Command queue handle pointer.
 * @return
 *     0, pattern generate reset is done.
 *     -ENODEV, null dev pointer.
 *     -EINVAL, invalid argument.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_slcr_patgen_reset(const struct device *dev,
			   struct cmdq_pkt **cmdq_handle)
{
	struct mtk_slcr *slcr = NULL;
	struct reg_info rif;

	if (!dev)
		return -EINVAL;

	slcr = dev_get_drvdata(dev);

	if (!slcr)
		return -ENODEV;

	if (cmdq_handle == NULL)
		rif.pkt = NULL;
	else
		rif.pkt = cmdq_handle[0];

	rif.regs = slcr->regs;
	rif.cmdq_subsys = slcr->cmdq_subsys;
	rif.cmdq_offset = slcr->cmdq_offset;

	reg_write_mask(rif, (0x1 << 0), SLCR_PATG_1, PATG_RST);
	reg_write_mask(rif, (0x0 << 0), SLCR_PATG_1, PATG_RST);

	return 0;
}
EXPORT_SYMBOL(mtk_slcr_patgen_reset);

/** @ingroup IP_group_slicer_internal_function
 * @par Description
 *     Config slicer video path.
 * @param[in]
 *     reg_cfg: Structure pointer of mtk_slcr_reg_cfg.
 * @param[in]
 *     cfg: Structure pointer of slcr_config.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_slcr_video_config(struct mtk_slcr_reg_cfg *reg_cfg,
				  const struct slcr_config *cfg)
{
	/* Video 3D re-shaped control */
	reg_cfg->slcr_3d_ctrl[0] = cfg->dp_video.data_path.rs_en;
	reg_cfg->slcr_3d_ctrl[1] = cfg->dp_video.data_path.rs_mode;
	reg_cfg->slcr_3d_ctrl[2] =
		(cfg->dp_video.data_path.vsync << 0) |
		(cfg->dp_video.data_path.v_front << 16);
	reg_cfg->slcr_3d_ctrl[3] =
		(cfg->dp_video.data_path.v_active << 0) |
		(cfg->dp_video.data_path.v_back << 16);
	/* Video input frame width/height */
	reg_cfg->slcr_vid_ctrl[1] =
		(cfg->dp_video.input.width << 0) |
		(cfg->dp_video.input.height << 16);
	/* Video input/output HSYNC/VSYNC polarity inverse */
	reg_cfg->slcr_vid_ctrl[2] =
		(1 << 12) |
		(cfg->dp_video.input.in_hsync_inv << 4) |
		(cfg->dp_video.input.in_vsync_inv << 5) |
		(cfg->dp_video.output.out_hsync_inv << 8) |
		(cfg->dp_video.output.out_vsync_inv << 9);
	/* Video pixel swap, channel swap, sync port selection control */
	reg_cfg->slcr_vid_ctrl[3] =
		(cfg->dp_video.output.sync_port << 28) |
		(cfg->dp_video.output.out_ch[2] << 24) |
		(cfg->dp_video.output.out_ch[1] << 20) |
		(cfg->dp_video.output.out_ch[0] << 16) |
		(cfg->dp_video.input.in_ch[2] << 12) |
		(cfg->dp_video.input.in_ch[1] << 8) |
		(cfg->dp_video.input.in_ch[0] << 4) |
		(cfg->dp_video.input.pxl_swap);
	/* Video slice 0 SOP and EOP */
	reg_cfg->slcr_vid_ctrl[4] =
		(((cfg->dp_video.output.slice_sop[0] + 1) & 0x1fff) << 0) |
		(((cfg->dp_video.output.slice_eop[0]) & 0x1fff) << 16);
	/* Video slice 1 SOP and EOP */
	reg_cfg->slcr_vid_ctrl[5] =
		(((cfg->dp_video.output.slice_sop[1] + 1) & 0x1fff) << 0) |
		(((cfg->dp_video.output.slice_eop[1]) & 0x1fff) << 16);
	/* Video slice 2 SOP and EOP */
	reg_cfg->slcr_vid_ctrl[6] =
		(((cfg->dp_video.output.slice_sop[2] + 1) & 0x1fff) << 0) |
		(((cfg->dp_video.output.slice_eop[2]) & 0x1fff) << 16);
	/* Video slice 3 SOP and EOP */
	reg_cfg->slcr_vid_ctrl[7] =
		(((cfg->dp_video.output.slice_sop[3] + 1) & 0x1fff) << 0) |
		(((cfg->dp_video.output.slice_eop[3]) & 0x1fff) << 16);
	/* Video lane 0~3 output selection */
	reg_cfg->slcr_vid_ctrl[8] =
		(cfg->dp_video.output.vid_en[0] << 0) |
		(cfg->dp_video.output.vid_en[1] << 4) |
		(cfg->dp_video.output.vid_en[2] << 8) |
		(cfg->dp_video.output.vid_en[3] << 12);
	/* Video dummy line control */
	reg_cfg->slcr_dmyl_ctrl[0] =
		(cfg->dp_video.output.dyml[0].num << 0) |
		(cfg->dp_video.output.dyml[0].r_value << 16);
	reg_cfg->slcr_dmyl_ctrl[1] =
		(cfg->dp_video.output.dyml[0].g_value << 0) |
		(cfg->dp_video.output.dyml[0].b_value << 16);
	reg_cfg->slcr_dmyl_ctrl[2] =
		(cfg->dp_video.output.dyml[1].num << 0) |
		(cfg->dp_video.output.dyml[1].r_value << 16);
	reg_cfg->slcr_dmyl_ctrl[3] =
		(cfg->dp_video.output.dyml[1].g_value << 0) |
		(cfg->dp_video.output.dyml[1].b_value << 16);
	reg_cfg->slcr_dmyl_ctrl[4] =
		(cfg->dp_video.output.dyml[2].num << 0) |
		(cfg->dp_video.output.dyml[2].r_value << 16);
	reg_cfg->slcr_dmyl_ctrl[5] =
		(cfg->dp_video.output.dyml[2].g_value << 0) |
		(cfg->dp_video.output.dyml[2].b_value << 16);
	reg_cfg->slcr_dmyl_ctrl[6] =
		(cfg->dp_video.output.dyml[3].num << 0) |
		(cfg->dp_video.output.dyml[3].r_value << 16);
	reg_cfg->slcr_dmyl_ctrl[7] =
		(cfg->dp_video.output.dyml[3].g_value << 0) |
		(cfg->dp_video.output.dyml[3].b_value << 16);
	/* Video GCE event */
	reg_cfg->slcr_gce_ctrl[0] = cfg->dp_video.gce.event;
	reg_cfg->slcr_gce_ctrl[1] =
		((cfg->dp_video.gce.height & 0x1fff) << 16) |
		 (cfg->dp_video.gce.width & 0x1fff);
}

/** @ingroup IP_group_slicer_internal_function
 * @par Description
 *     Config slicer DSC path.
 * @param[in]
 *     reg_cfg: Structure pointer of mtk_slcr_reg_cfg.
 * @param[in]
 *     cfg: Structure pointer of slcr_config.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_slcr_dsc_config(struct mtk_slcr_reg_cfg *reg_cfg,
				const struct slcr_config *cfg)
{
	/* DSC input frame width/height */
	reg_cfg->slcr_dsc_ctrl[1] =
		(cfg->dp_dsc.input.width << 0) |
		(cfg->dp_dsc.input.height << 16);
	/* DSC input/output HSYNC/VSYNC polarity inverse */
	reg_cfg->slcr_dsc_ctrl[2] =
		(cfg->dp_dsc.input.in_hsync_inv << 4) |
		(cfg->dp_dsc.input.in_vsync_inv << 5) |
		(cfg->dp_dsc.output.out_hsync_inv << 8) |
		(cfg->dp_dsc.output.out_vsync_inv << 9) |
		(cfg->dp_dsc.output.endian << 12) |
		((cfg->dp_dsc.input.chunk_num & 0x7) << 20) |
		((cfg->dp_dsc.input.bit_rate & 0x3f) << 24);
	/* DSC pixel swap, valid byte, sync port selection control */
	reg_cfg->slcr_dsc_ctrl[3] =
		(cfg->dp_dsc.output.sync_port << 28) |
		((cfg->dp_dsc.output.valid_byte & 0x3f) << 16) |
		(cfg->dp_dsc.input.pxl_swap);
	/* DSC slice 0 SOP and EOP */
	reg_cfg->slcr_dsc_ctrl[4] =
		(((cfg->dp_dsc.output.slice_sop[0] + 1) & 0x1fff) << 0) |
		(((cfg->dp_dsc.output.slice_eop[0]) & 0x1fff) << 16);
	/* DSC slice 1 SOP and EOP */
	reg_cfg->slcr_dsc_ctrl[5] =
		(((cfg->dp_dsc.output.slice_sop[1] + 1) & 0x1fff) << 0) |
		(((cfg->dp_dsc.output.slice_eop[1]) & 0x1fff) << 16);
	/* DSC slice 2 SOP and EOP */
	reg_cfg->slcr_dsc_ctrl[6] =
		(((cfg->dp_dsc.output.slice_sop[2] + 1) & 0x1fff) << 0) |
		(((cfg->dp_dsc.output.slice_eop[2]) & 0x1fff) << 16);
	/* DSC slice 3 SOP and EOP */
	reg_cfg->slcr_dsc_ctrl[7] =
		(((cfg->dp_dsc.output.slice_sop[3] + 1) & 0x1fff) << 0) |
		(((cfg->dp_dsc.output.slice_eop[3]) & 0x1fff) << 16);
	/* DSC lane 0~3 output selection */
	reg_cfg->slcr_dsc_ctrl[8] =
		(cfg->dp_dsc.output.dsc_en[0] << 0) |
		(cfg->dp_dsc.output.dsc_en[1] << 4) |
		(cfg->dp_dsc.output.dsc_en[2] << 8) |
		(cfg->dp_dsc.output.dsc_en[3] << 12);
	/* DSC GCE event */
	reg_cfg->slcr_gce_ctrl[2] = cfg->dp_dsc.gce.event;
	reg_cfg->slcr_gce_ctrl[3] =
		((cfg->dp_dsc.gce.height & 0x1fff) << 16) |
		 (cfg->dp_dsc.gce.width & 0x1fff);
}

/** @ingroup IP_group_slicer_internal_function
 * @par Description
 *     Check the gce event of slicer if meet the Slicer hardware limitation.
 * @param[in]
 *     dev: Slicer device node.
 * @param[in]
 *     gce: Structure of slcr_gce.
 * @return
 *     0, configuration success.
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     gce event, ragne from 0 to GCE_OUTPUT_3.
 * @par Error case and Error handling
 *     If gce event is unknown, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */

static int mtk_slcr_gce_event_check(const struct device *dev,
				struct slcr_gce gce)
{
	if (gce.event > GCE_OUTPUT_3) {
		dev_err(dev, "%s:unknown gce.event!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(8191, gce.height)) {
		dev_err(dev, "%s:unknown gce.height!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(8191, gce.width)) {
		dev_err(dev, "%s:unknown gce.width!\n", __func__);
		return -EINVAL;
	}

	return 0;
}

/** @ingroup IP_group_slicer_internal_function
 * @par Description
 *     Check the video config of slicer if meet the Slicer hardware limitation.
 * @param[in]
 *     dev: Slicer device node.
 * @param[in]
 *     cfg: Structure pointer of slcr_config.
 * @return
 *     0, configuration success.
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     1. video input width should be in the range of 0 to 8191.
 *     2. video input height should be in the range of 0 to 8191.
 *     3. video input pixel swap should be in the range from 0 to SWAP.
 *     4. video input channel swap, range from 0 to NO_OUTPUT.
 *     5. video output slice 0~3 SOP, range from 0 to 8191.
 *     6. video output slice 0~3 EOP, range from 0 to 8191.
 *     7. video lane output selection, range from 0 to 15.
 *     8. video dummy line numbers, range from 0 to 8191.
 *     9. video dummy line data r, range from 0 to 4095.
 *     10. video dummy line data g, range from 0 to 4095.
 *     11. video dummy line data b, range from 0 to 4095.
 *     12. video sync port, range from 0 to TIMING_REGEN_PORT.
 *     13. video height when gce event triggered, range from 0 to 8191.
 *     14. video width when gce event triggered, range from 0 to 8191.
 *     15. video 3D re-shaped mode, range from 0 to SOFTWARE_MODE.
 *     16. vertical front porch of 3D re-shaped, range from 0 to 8191.
 *     17. vertical sync of 3D re-shaped, range from 0 to 8191.
 *     18. vertical back porch of 3D re-shaped, range from 0 to 8191.
 *     19. vertical active of 3D re-shaped, range from 0 to 8191.
 * @par Error case and Error handling
 *     1. If video input width is unknown, return -EINVAL.
 *     2. If video input height is unknown, return -EINVAL.
 *     3. If video input pixel swap is unknown, return -EINVAL.
 *     4. If video input/output channel swap is unknown, return -EINVAL.
 *     5. If video output slice 0~3 SOP are unknown, return -EINVAL.
 *     6. If video output slice 0~3 EOP are unknown, return -EINVAL.
 *     7. If video lane output selection are unknown, return -EINVAL.
 *     8. If video dummy line numbers is unknown, return -EINVAL.
 *     9. If  video dummy line data r is unknown, return -EINVAL.
 *     10. If video dummy line data g is unknown, return -EINVAL.
 *     11. If video dummy line data b is unknown, return -EINVAL.
 *     12. If video sync port is unknown, return -EINVAL.
 *     13. If video height when gce event triggered is unknown, return -EINVAL.
 *     14. If video width when gce event triggered is unknown, return -EINVAL.
 *     15. If video 3D re-shaped mode is unknown, return -EINVAL.
 *     16. If vertical front porch of 3D re-shaped is unknown, return -EINVAL.
 *     17. If vertical sync of 3D re-shaped is unknown, return -EINVAL.
 *     18. If vertical back porch of 3D re-shaped is unknown, return -EINVAL.
 *     19. If vertical active of 3D re-shaped is unknown, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_slcr_vid_config_check(const struct device *dev,
				const struct slcr_config *cfg)
{
	int i;

	/* Video input frame width/height */
	if (!INRANGE(8191, cfg->dp_video.input.width)) {
		dev_err(dev, "%s:unknown video.input.width!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(8191, cfg->dp_video.input.height)) {
		dev_err(dev, "%s:unknown video.input.height!\n", __func__);
		return -EINVAL;
	}
	/* Video pixel swap, channel swap selection control */
	if (cfg->dp_video.input.pxl_swap > SWAP) {
		dev_err(dev, "%s:unknown video.input.pxl_swap!\n", __func__);
		return -EINVAL;
	}
	for (i = 0; i < 3; i++) {
		if (cfg->dp_video.input.in_ch[i] > NO_OUTPUT) {
			dev_err(dev, "%s:unknown video.input.in_ch[%d]!\n",
				__func__, i);
			return -EINVAL;
		}
		if (cfg->dp_video.output.out_ch[i] > NO_OUTPUT) {
			dev_err(dev, "%s:unknown video.output.out_ch[%d]!\n",
				__func__, i);
			return -EINVAL;
		}
	}
	for (i = 0; i < 4; i++) {
		/* Video slice 0~3 SOP and EOP */
		if (!INRANGE(8191, cfg->dp_video.output.slice_sop[i])) {
			dev_err(dev, "%s:unknown video.output.slice_sop[%d]!\n",
				__func__, i);
			return -EINVAL;
		}
		if (!INRANGE(8191, cfg->dp_video.output.slice_eop[i])) {
			dev_err(dev, "%s:unknown video.output.slice_eop[%d]!\n",
				__func__, i);
			return -EINVAL;
		}
		/* Video lane 0~3 output selection */
		if (!INRANGE(15, cfg->dp_video.output.vid_en[i])) {
			dev_err(dev, "%s:unknown video.output.vid_en[%d]: %d!\n",
				__func__, i,
				cfg->dp_video.output.vid_en[i]);
			return -EINVAL;
		}
		/* Video dummy line control */
		if (!INRANGE(8191, cfg->dp_video.output.dyml[i].num)) {
			dev_err(dev, "%s:unknown video.output.dyml[%d].num!\n",
				__func__, i);
			return -EINVAL;
		}
		if (!INRANGE(1023, cfg->dp_video.output.dyml[i].r_value)) {
			dev_err(dev, "%s:unknown video.output.dyml[%d].r_value!\n",
				__func__, i);
			return -EINVAL;
		}
		if (!INRANGE(1023, cfg->dp_video.output.dyml[i].g_value)) {
			dev_err(dev, "%s:unknown video.output.dyml[%d].g_value!\n",
				__func__, i);
			return -EINVAL;
		}
		if (!INRANGE(1023, cfg->dp_video.output.dyml[i].b_value)) {
			dev_err(dev, "%s:unknown video.output.dyml[%d].b_value!\n",
				__func__, i);
			return -EINVAL;
		}
	}
	/* Video sync port selection control */
	if (cfg->dp_video.output.sync_port > TIMING_REGEN_PORT) {
		dev_err(dev, "%s:unknown video.output.sync_port!\n", __func__);
		return -EINVAL;
	}

	/* Video GCE event */
	mtk_slcr_gce_event_check(dev, cfg->dp_video.gce);

	/* Video 3D re-shaped control */
	if (cfg->dp_video.data_path.rs_mode > SOFTWARE_MODE) {
		dev_err(dev, "%s:unknown rs_mode!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(8191, cfg->dp_video.data_path.v_front)) {
		dev_err(dev, "%s:unknownvideo.data_path.v_front!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(8191, cfg->dp_video.data_path.vsync)) {
		dev_err(dev, "%s:unknownvideo.data_path.vsync!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(8191, cfg->dp_video.data_path.v_back)) {
		dev_err(dev, "%s:unknownvideo.data_path.v_back!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(8191, cfg->dp_video.data_path.v_active)) {
		dev_err(dev, "%s:unknownvideo.data_path.v_active!\n", __func__);
		return -EINVAL;
	}

	return 0;
}

/** @ingroup IP_group_slicer_internal_function
 * @par Description
 *     Check the dsc config of slicer if meet the Slicer hardware limitation.
 * @param[in]
 *     dev: Slicer device node.
 * @param[in]
 *     cfg: Structure pointer of slcr_config.
 * @return
 *     0, configuration success.
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     1. dsc input width should be in the range of 0 to 8191.
 *     2. dsc input height should be in the range of 0 to 8191.
 *     3. dsc bit rate, range from 0 to 63.
 *     4. dsc numbers of chunk, range from 0 to 7.
 *     5. dsc input pixel swap should be in the range from 0 to SWAP.
 *     6. valid byte for dsc stream, range from 0 to 63.
 *     7. dsc output endian, range from 0 to LITTLE_END.
 *     8. dsc sync port, range from 0 to TIMING_REGEN_PORT.
 *     9. dsc output slice 0~3 SOP, range from 0 to 8191.
 *     10. dsc output slice 0~3 EOP, range from 0 to 8191.
 *     11. dsc lane output selection, range from 0 to 15.
 *     12. dsc height when gce event triggered, range from 0 to 8191.
 *     13. dsc width when gce event triggered, range from 0 to 8191.
 * @par Error case and Error handling
 *     1. If dsc input width is unknown, return -EINVAL.
 *     2. If dsc input height is unknown, return -EINVAL.
 *     3. If dsc bit rate is unknown, return -EINVAL.
 *     4. If dsc numbers of chunk is unknown, return -EINVAL.
 *     5. If dsc input pixel swap is unknown, return -EINVAL.
 *     6. If valid byte for dsc stream is unknown, return -EINVAL.
 *     7. If dsc output endian is unknown, return -EINVAL.
 *     8. If dsc sync port is unknown, return -EINVAL.
 *     9. If dsc output slice 0~3 SOP are unknown, return -EINVAL.
 *     10. If dsc output slice 0~3 EOP are unknown, return -EINVAL.
 *     11. If dsc lane output selection are unknown, return -EINVAL.
 *     12. If dsc height when gce event triggered is unknown, return -EINVAL.
 *     13. If dsc width when gce event triggered is unknown, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_slcr_dsc_config_check(const struct device *dev,
				const struct slcr_config *cfg)
{
	int i;

	/* DSC input frame width/height */
	if (!INRANGE(8191, cfg->dp_dsc.input.width)) {
		dev_err(dev, "%s:unknown dsc.input.width!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(8191, cfg->dp_dsc.input.height)) {
		dev_err(dev, "%s:unknown dsc.input.height!\n", __func__);
		return -EINVAL;
	}
	/* DSC bit rate/numbers of chunk */
	if (!INRANGE(63, cfg->dp_dsc.input.bit_rate)) {
		dev_err(dev, "%s:unknown dsc.input.bit_rate!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(7, cfg->dp_dsc.input.chunk_num)) {
		dev_err(dev, "%s:unknown dsc.input.chunk_num!\n", __func__);
		return -EINVAL;
	}
	/* DSC pixel swap */
	if (cfg->dp_dsc.input.pxl_swap > SWAP) {
		dev_err(dev, "%s:unknown dsc.input.pxl_swap!\n", __func__);
		return -EINVAL;
	}
	/* DSC valid byte */
	if (!INRANGE(63, cfg->dp_dsc.output.valid_byte)) {
		dev_err(dev, "%s:unknown output valid_byte: %d!\n", __func__,
		cfg->dp_dsc.output.valid_byte);
		return -EINVAL;
	}
	/* DSC output endian */
	if (cfg->dp_dsc.output.endian > LITTLE_END) {
		dev_err(dev, "%s:unknown output endian!\n", __func__);
		return -EINVAL;
	}
	/* DSC sync port selection control */
	if (cfg->dp_dsc.output.sync_port > TIMING_REGEN_PORT) {
		dev_err(dev, "%s:unknown dsc.output.sync_port!\n", __func__);
		return -EINVAL;
	}
	for (i = 0; i < 4; i++) {
		/* DSC slice 0~3 SOP and EOP */
		if (!INRANGE(8191, cfg->dp_dsc.output.slice_sop[i])) {
			dev_err(dev, "%s:unknown dsc.output.slice_sop[%d]!\n",
				__func__, i);
			return -EINVAL;
		}
		if (!INRANGE(8191, cfg->dp_dsc.output.slice_eop[i])) {
			dev_err(dev, "%s:unknown dsc.output.slice_eop[%d]!\n",
				__func__, i);
			return -EINVAL;
		}
		/* DSC lane 0~3 output selection */
		if (!INRANGE(15, cfg->dp_dsc.output.dsc_en[i])) {
			dev_err(dev, "%s:unknown dsc.output.dsc_en[%d]: %d!\n",
				__func__, i, cfg->dp_dsc.output.dsc_en[i]);
			return -EINVAL;
		}
	}

	/* DSC GCE event */
	mtk_slcr_gce_event_check(dev, cfg->dp_dsc.gce);

	return 0;
}

/** @ingroup IP_group_slicer_internal_function
 * @par Description
 *     Check the patgen config of slicer if meet the Slicer hardware limitation.
 * @param[in]
 *     dev: Slicer device node.
 * @param[in]
 *     pat_cfg: Structure pointer of slcr_pg_config.
 * @return
 *     0, configuration success.
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     1. patgen mode, range from 0 to INPUT_TIMING.
 *     2. patgen timing, range from 0 to PAT_640_480.
 *     3. vertical sync of patg, range from 0 to 8191.
 *     4. horizontal sync of patg, range from 0 to 8191.
 *     5. vertical back porch of patg, range from 0 to 8191.
 *     6. vertical active, range from 0 to 8191.
 *     7. vertical active, range from 0 to 8191.
 *     8. horizontal active, range from 0 to 8191.
 *     9. vertical front porch, range from 0 to 8191.
 *     10. horizontal front porch, range from 0 to 8191.
 *     11. color bar height r/g/b, range from 0 to 8191.
 *     12. color bar width r/g/b, range from 0 to 8191.
 *     13. initial value of data r/g/b, range from 0 to 4095.
 *     14. vertical addend of data r/g/b, range from 0 to 4095.
 *     15. horizontal addend of data r/g/b, range from 0 to 4095.
 *     16. dsc bit rate, range from 0 to 63.
 *     17. dsc numbers of chunk, range from 0 to 7.
 * @par Error case and Error handling
 *     1. If patgen mode is unknown, return -EINVAL.
 *     2. If patgen timing is unknown, return -EINVAL.
 *     3. If vertical sync of patg is unknown, return -EINVAL.
 *     4. If horizontal sync of patg is unknown, return -EINVAL.
 *     5. If vertical back porch of patg is unknown, return -EINVAL.
 *     6. If vertical active is unknown, return -EINVAL.
 *     7. If vertical active is unknown, return -EINVAL.
 *     8. If horizontal active is unknown, return -EINVAL.
 *     9. If vertical front porch is unknown, return -EINVAL.
 *     10. If horizontal front porch is unknown, return -EINVAL.
 *     11. If color bar height r/g/b is unknown, return -EINVAL.
 *     12. If color bar width r/g/b is unknown, return -EINVAL.
 *     13. If initial value of data r/g/b is unknown, return -EINVAL.
 *     14. If vertical addend of data r/g/b is unknown, return -EINVAL.
 *     15. If horizontal addend of data r/g/b is unknown, return -EINVAL.
 *     16. If dsc bit rate is unknown, return -EINVAL.
 *     17. If dsc numbers of chunk is unknown, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_slcr_patgen_config_check(const struct device *dev,
					const struct slcr_pg_config *pat_cfg)
{
	if (pat_cfg->pg_mode > INPUT_TIMING) {
		dev_err(dev, "%s:unknown patgen mode!\n", __func__);
		return -EINVAL;
	}
	if (pat_cfg->pg_timing > PAT_640_480) {
		dev_err(dev, "%s:unknown patgen timing!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(8191, pat_cfg->pg_frame_cfg.vsync)) {
		dev_err(dev, "%s:unknown patgen vsync!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(8191, pat_cfg->pg_frame_cfg.hsync)) {
		dev_err(dev, "%s:unknown patgen hsync!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(8191, pat_cfg->pg_frame_cfg.v_back)) {
		dev_err(dev, "%s:unknown patgen v_back!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(8191, pat_cfg->pg_frame_cfg.h_back)) {
		dev_err(dev, "%s:unknown patgen h_back!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(8191, pat_cfg->pg_frame_cfg.v_active)) {
		dev_err(dev, "%s:unknown patgen v_active!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(8191, pat_cfg->pg_frame_cfg.h_active)) {
		dev_err(dev, "%s:unknown patgen h_active!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(8191, pat_cfg->pg_frame_cfg.v_front)) {
		dev_err(dev, "%s:unknown patgen v_front!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(8191, pat_cfg->pg_frame_cfg.h_front)) {
		dev_err(dev, "%s:unknown patgen h_front!\n", __func__);
		return -EINVAL;
	}

	/* r */
	if (!INRANGE(8191, pat_cfg->pg_pat_cfg.color_r.cbar_height)) {
		dev_err(dev, "%s:unknown patgen r.cbar_height!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(8191, pat_cfg->pg_pat_cfg.color_r.cbar_width)) {
		dev_err(dev, "%s:unknown patgen r.cbar_width!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(4095, pat_cfg->pg_pat_cfg.color_r.init_value)) {
		dev_err(dev, "%s:unknown patgen r.init_value!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(4095, pat_cfg->pg_pat_cfg.color_r.v_addend)) {
		dev_err(dev, "%s:unknown patgen r.v_addend!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(4095, pat_cfg->pg_pat_cfg.color_r.h_addend)) {
		dev_err(dev, "%s:unknown patgen r.h_addend!\n", __func__);
		return -EINVAL;
	}

	/* g */
	if (!INRANGE(8191, pat_cfg->pg_pat_cfg.color_g.cbar_height)) {
		dev_err(dev, "%s:unknown patgen g.cbar_height!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(8191, pat_cfg->pg_pat_cfg.color_g.cbar_width)) {
		dev_err(dev, "%s:unknown patgen g.cbar_width!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(4095, pat_cfg->pg_pat_cfg.color_g.init_value)) {
		dev_err(dev, "%s:unknown patgen g.init_value!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(4095, pat_cfg->pg_pat_cfg.color_g.v_addend)) {
		dev_err(dev, "%s:unknown patgen g.v_addend!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(4095, pat_cfg->pg_pat_cfg.color_g.h_addend)) {
		dev_err(dev, "%s:unknown patgen g.h_addend!\n", __func__);
		return -EINVAL;
	}

	/* b */
	if (!INRANGE(8191, pat_cfg->pg_pat_cfg.color_b.cbar_height)) {
		dev_err(dev, "%s:unknown patgen b.cbar_height!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(8191, pat_cfg->pg_pat_cfg.color_b.cbar_width)) {
		dev_err(dev, "%s:unknown patgen b.cbar_width!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(4095, pat_cfg->pg_pat_cfg.color_b.init_value)) {
		dev_err(dev, "%s:unknown patgen b.init_value!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(4095, pat_cfg->pg_pat_cfg.color_b.v_addend)) {
		dev_err(dev, "%s:unknown patgen b.v_addend!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(4095, pat_cfg->pg_pat_cfg.color_b.h_addend)) {
		dev_err(dev, "%s:unknown patgen b.h_addend!\n", __func__);
		return -EINVAL;
	}


	if (!INRANGE(63, pat_cfg->pg_dsc_cfg.bit_rate)) {
		dev_err(dev, "%s:unknown pg_dsc_cfg.bit_rate!\n", __func__);
		return -EINVAL;
	}
	if (!INRANGE(7, pat_cfg->pg_dsc_cfg.chunk_num)) {
		dev_err(dev, "%s:unknown pg_dsc_cfg.chunk_num!\n", __func__);
		return -EINVAL;
	}

	return 0;
}

/** @ingroup IP_group_slicer_external_function
 * @par Description
 *     Setup slicer.
 * @param[in]
 *     dev: Slicer device node.
 * @param[in]
 *     cmdq_handle: Command queue handle pointer.
 * @param[in]
 *     cfg: Slicer configuration data structure pointer.
 * @return
 *     0, configuration is done.
 *     -ENODEV, null dev pointer.
 *     -EINVAL, cfg pointer invalid.
 *     -EINVAL, unsupported input format.
 *     error code from mtk_slcr_config_check().
 *     error code from mtk_slcr_config_clk_sel().
 *     error code from mtk_slcr_config_csc_reg().
 *     error code from mtk_slcr_dither_config_reg().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If dev is NULL, return -EINVAL.
 *     2. If cfg pointer is invalid, return -EINVAL.
 *     3. If video input format is unknown, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_slcr_config(const struct device *dev,
		    struct cmdq_pkt **cmdq_handle,
		    const struct slcr_config *cfg)
{
	struct mtk_slcr_reg_cfg reg_cfg = {0, {0}, {0}, {0}, {0}, {0}, {0} };
	struct mtk_slcr_csc_set reg_csc_cfg = {false, 0};
	struct mtk_slcr_dither_set reg_dither_cfg = {false, 0, 0};
	int ret = 0;

	if (!dev)
		return -EINVAL;

	if (!cfg)
		return -EINVAL;

	ret = mtk_slcr_vid_config_check(dev, cfg);
	if (ret)
		return ret;

	ret = mtk_slcr_dsc_config_check(dev, cfg);
	if (ret)
		return ret;

	reg_cfg.in_format = cfg->in_format;

#ifdef CONFIG_MACH_MT3612_A0
	/* Disable clock divider */
	mtk_topckgen_slicer_div(0, 0, 0);
#endif

	/* Setup input parameters */
	switch (reg_cfg.in_format) {
	case DP_VIDEO:
		/* Enable video path */
		reg_cfg.slcr_vid_ctrl[0] = VID_EN;
		/* Disable DSC path */
		reg_cfg.slcr_dsc_ctrl[0] = 0;
		/* Disable DSC GCE event */
		reg_cfg.slcr_gce_ctrl[2] = 0;
		/* Config video path */
		mtk_slcr_video_config(&reg_cfg, cfg);
		/* Setup clk select */
		mtk_slcr_config_clk_sel(dev, DPRX_VIDEO_CK);
		break;

	case DP_DSC:
		/* Disable video path */
		reg_cfg.slcr_vid_ctrl[0] = 0;
		/* Enable DSC path */
		reg_cfg.slcr_dsc_ctrl[0] = DSC_EN;
		/* Disable video GCE event */
		reg_cfg.slcr_gce_ctrl[0] = 0;
		/* Config DSC path */
		mtk_slcr_dsc_config(&reg_cfg, cfg);
		/* Setup clk select */
		ret = mtk_slcr_config_clk_sel(dev, DPRX_DSC_CK);
		if (ret)
			return ret;
		break;

	case DP_VIDEO_DSC:
		/* Enable video path */
		reg_cfg.slcr_vid_ctrl[0] = VID_EN;
		/* Enable DSC path */
		reg_cfg.slcr_dsc_ctrl[0] = DSC_EN;
		/* Config video path */
		mtk_slcr_video_config(&reg_cfg, cfg);
		/* Config DSC path */
		mtk_slcr_dsc_config(&reg_cfg, cfg);
		/* Setup clk select */
		ret = mtk_slcr_config_clk_sel(dev, DPRX_VIDEO_DSC_CK);
		if (ret)
			return ret;
		break;

	default:
		dev_err(dev, "Unsupported input format!\n");
		return -EINVAL;
	}

	/* Setup PQ part */
	reg_csc_cfg.enable = cfg->dp_video.csc_en;
	reg_csc_cfg.csc_mode = cfg->dp_video.csc_mode;
	reg_dither_cfg.enable = cfg->dp_video.dither_en;
	reg_dither_cfg.dithermode = cfg->dp_video.dither_mode;
	reg_dither_cfg.dithertype = cfg->dp_video.dither_type;

	/* Setup slicer registers */
	mtk_slcr_config_reg(dev, cmdq_handle, &reg_cfg);

	/* Setup csc registers */
	ret = mtk_slcr_config_csc_reg(dev, cmdq_handle, &reg_csc_cfg);
	if (ret)
		return ret;

	/* Setup dither registers */
	ret = mtk_slcr_dither_config_reg(dev, cmdq_handle, &reg_dither_cfg);
	if (ret)
		return ret;

	return ret;
}
EXPORT_SYMBOL(mtk_slcr_config);

/** @ingroup IP_group_slicer_external_function
 * @par Description
 *     Setup slicer pattern generator.
 * @param[in]
 *     dev: Slicer device node.
 * @param[in]
 *     cmdq_handle: Command queue handle pointer.
 * @param[in]
 *     pg_cfg: Slicer pattern generator configuration data structure pointer.
 * @return
 *     0, configuration is done.
 *     -1, unsupported source format.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If dev is NULL, return -EINVAL.
 *     2. If pg_cfg pointer is invalid, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_slcr_patgen_config(const struct device *dev,
			   struct cmdq_pkt **cmdq_handle,
			   const struct slcr_pg_config *pg_cfg)
{
	struct mtk_slcr_reg_cfg reg_cfg = {0, {0}, {0}, {0} };
	int ret = 0;

	if (!dev)
		return -EINVAL;

	if (!pg_cfg)
		return -EINVAL;

	ret = mtk_slcr_patgen_config_check(dev, pg_cfg);
	if (ret)
		return ret;

	reg_cfg.slcr_pg_ctrl[0] = (pg_cfg->pg_dsc_cfg.chunk_num << 20) |
				  (pg_cfg->pg_dsc_cfg.bit_rate << 24) |
				  (1 << 8) |
				  (pg_cfg->pg_mode << 0);
	reg_cfg.slcr_pg_ctrl[1] = (pg_cfg->pg_frame_cfg.vsync_pol << 12) |
				  (pg_cfg->pg_frame_cfg.hsync_pol << 8) |
				  ((pg_cfg->pg_timing & 0xf) << 0);

	if (pg_cfg->pg_timing == PAT_EXT) {
		reg_cfg.slcr_pg_ctrl[2] =
			((pg_cfg->pg_frame_cfg.vsync & 0x1fff) << 16) |
			((pg_cfg->pg_frame_cfg.hsync & 0x1fff) << 0);
		reg_cfg.slcr_pg_ctrl[3] =
			((pg_cfg->pg_frame_cfg.v_back & 0x1fff) << 16) |
			((pg_cfg->pg_frame_cfg.h_back & 0x1fff) << 0);
		reg_cfg.slcr_pg_ctrl[4] =
			((pg_cfg->pg_frame_cfg.v_active & 0x1fff) << 16) |
			((pg_cfg->pg_frame_cfg.h_active & 0x1fff) << 0);
		reg_cfg.slcr_pg_ctrl[5] =
			((pg_cfg->pg_frame_cfg.v_front & 0x1fff) << 16) |
			((pg_cfg->pg_frame_cfg.h_front & 0x1fff) << 0);
	}

	/* Setup RGB pattern characteristic */
	/* R pattern */
	reg_cfg.slcr_pg_ctrl[6] =
		((pg_cfg->pg_pat_cfg.color_r.cbar_height & 0x1fff) << 16) |
		((pg_cfg->pg_pat_cfg.color_r.cbar_width & 0x1fff) << 0);

	reg_cfg.slcr_pg_ctrl[7] =
		(pg_cfg->pg_pat_cfg.color_r.init_value & 0xfff);

	reg_cfg.slcr_pg_ctrl[8] =
		((pg_cfg->pg_pat_cfg.color_r.v_addend & 0xfff) << 16) |
		((pg_cfg->pg_pat_cfg.color_r.h_addend  & 0xfff) << 0);

	/* G pattern */
	reg_cfg.slcr_pg_ctrl[9] =
		((pg_cfg->pg_pat_cfg.color_g.cbar_height & 0x1fff) << 16) |
		((pg_cfg->pg_pat_cfg.color_g.cbar_width & 0x1fff) << 0);

	reg_cfg.slcr_pg_ctrl[10] =
		(pg_cfg->pg_pat_cfg.color_g.init_value & 0xfff);

	reg_cfg.slcr_pg_ctrl[11] =
		((pg_cfg->pg_pat_cfg.color_g.v_addend & 0xfff) << 16) |
		((pg_cfg->pg_pat_cfg.color_g.h_addend  & 0xfff) << 0);

	/* B pattern */
	reg_cfg.slcr_pg_ctrl[12] =
		((pg_cfg->pg_pat_cfg.color_b.cbar_height & 0x1fff) << 16) |
		((pg_cfg->pg_pat_cfg.color_b.cbar_width & 0x1fff) << 0);

	reg_cfg.slcr_pg_ctrl[13] =
		(pg_cfg->pg_pat_cfg.color_b.init_value & 0xfff);

	reg_cfg.slcr_pg_ctrl[14] =
		((pg_cfg->pg_pat_cfg.color_b.v_addend & 0xfff) << 16) |
		((pg_cfg->pg_pat_cfg.color_b.h_addend  & 0xfff) << 0);


	/* Setup clk select */
	mtk_slcr_config_clk_sel(dev, UNIVPLL_D2);
	/* Setup pattern generator registers */
	mtk_slcr_config_patgen_reg(dev, cmdq_handle, &reg_cfg);

	return ret;
}
EXPORT_SYMBOL(mtk_slcr_patgen_config);

/** @ingroup IP_group_slicer_external_function
 * @par Description
 *     Get slicer pattern generator frame status.
 * @param[in]
 *     dev: Slicer device node.
 * @param[out]
 *     pg_frame_cfg: Slicer pattern generator frame status
			data structure pointer.
 * @return
 *     0, get frame status is done.
 *     -ENODEV, null dev pointer.
 *     -EINVAL, invalid argument.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_slcr_patgen_get_frame_status(const struct device *dev,
			struct slcr_pg_frame_cfg *pg_frame_cfg)
{
	u32 reg;
	struct mtk_slcr *slcr = NULL;

	if (!dev)
		return -EINVAL;

	slcr = dev_get_drvdata(dev);

	if (!slcr)
		return -ENODEV;

	if (!pg_frame_cfg)
		return -EINVAL;

	reg = readl(slcr->regs + SLCR_PATG_19);
	pg_frame_cfg->vsync_pol = (reg & 0x0001000) >> 12;
	pg_frame_cfg->hsync_pol = (reg & 0x0000100) >> 8;

	reg = readl(slcr->regs + SLCR_PATG_20);
	pg_frame_cfg->vsync = (reg & 0x1fff0000) >> 16;
	pg_frame_cfg->hsync = (reg & 0x00001fff) >> 0;

	reg = readl(slcr->regs + SLCR_PATG_21);
	pg_frame_cfg->v_back = (reg & 0x1fff0000) >> 16;
	pg_frame_cfg->h_back = (reg & 0x00001fff) >> 0;

	reg = readl(slcr->regs + SLCR_PATG_22);
	pg_frame_cfg->v_active = (reg & 0x1fff0000) >> 16;
	pg_frame_cfg->h_active = (reg & 0x00001fff) >> 0;

	reg = readl(slcr->regs + SLCR_PATG_23);
	pg_frame_cfg->v_front = (reg & 0x1fff0000) >> 16;
	pg_frame_cfg->h_front = (reg & 0x00001fff) >> 0;

	return 0;
}
EXPORT_SYMBOL(mtk_slcr_patgen_get_frame_status);

/** @ingroup IP_group_slicer_external_function
 * @par Description
 *     S/W trigger when pattern generator mode set as software trigger.
 * @param[in]
 *     dev: Slicer device node.
 * @param[in]
 *     cmdq_handle: Command queue handle pointer.
 * @return
 *     0, software trigger is done.
 *     -ENODEV, null dev pointer.
 *     -EINVAL, cfg pointer invalid.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_slcr_patgen_sw_trig(const struct device *dev,
			     struct cmdq_pkt **cmdq_handle)
{
	struct mtk_slcr *slcr = NULL;
	struct reg_info rif;

	if (!dev)
		return -EINVAL;

	slcr = dev_get_drvdata(dev);

	if (!slcr)
		return -ENODEV;


	if (cmdq_handle == NULL)
		rif.pkt = NULL;
	else
		rif.pkt = cmdq_handle[0];

	rif.regs = slcr->regs;
	rif.cmdq_subsys = slcr->cmdq_subsys;
	rif.cmdq_offset = slcr->cmdq_offset;

	reg_write_mask(rif, (0x1 << 4), SLCR_PATG_2, PATG_SW_TRIGGER);

	return 0;
}
EXPORT_SYMBOL(mtk_slcr_patgen_sw_trig);

/** @ingroup IP_group_slicer_external_function
 * @par Description
 *     Setup slicer gce event.
 * @param[in]
 *     dev: Slicer device node.
 * @param[in]
 *     cmdq_handle: Command queue handle pointer.
 * @param[in]
 *     in_format: Slicer input selection.
 * @param[in]
 *     gce: Slicer gce event selection.
 * @return
 *     0, set gce event is done.
 *     -ENODEV, null dev pointer.
 *     -EINVAL, invalid argument.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_slcr_set_gce_event(const struct device *dev,
			struct cmdq_pkt **cmdq_handle,
			enum slcr_in_format in_format,
			struct slcr_gce gce)
{
	struct mtk_slcr *slcr = NULL;
	struct reg_info rif;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	slcr = dev_get_drvdata(dev);

	if (!slcr)
		return -ENODEV;

	if (in_format > DP_DSC) {
		dev_err(dev, "%s:unknown in_format!\n", __func__);
		return -EINVAL;
	}

	if (cmdq_handle == NULL)
		rif.pkt = NULL;
	else
		rif.pkt = cmdq_handle[0];

	ret = mtk_slcr_gce_event_check(dev, gce);
	if (ret)
		return ret;

	rif.regs = slcr->regs;
	rif.cmdq_subsys = slcr->cmdq_subsys;
	rif.cmdq_offset = slcr->cmdq_offset;

	if (in_format == DP_VIDEO) {
		reg_write_mask(rif, (gce.event << 0), SLCR_GCE_0,
							GCE_VID_EVENT_SEL);
		reg_write_mask(rif, (gce.width << 0), SLCR_GCE_1,
							GCE_VID_WIDTH);
		reg_write_mask(rif, (gce.height << 16), SLCR_GCE_1,
							GCE_VID_HEIGHT);
	} else if  (in_format == DP_DSC) {
		reg_write_mask(rif, (gce.event << 0), SLCR_GCE_2,
							GCE_DSC_EVENT_SEL);
		reg_write_mask(rif, (gce.width << 0), SLCR_GCE_3,
							GCE_DSC_WIDTH);
		reg_write_mask(rif, (gce.height << 16), SLCR_GCE_3,
							GCE_DSC_HEIGHT);
	}

	return 0;
}
EXPORT_SYMBOL(mtk_slcr_set_gce_event);

#ifdef CONFIG_MTK_DEBUGFS
/** @ingroup IP_group_slicer_internal_function
 * @par Description
 *     Slicer dump debug message function.
 * @param[in]
 *     s: Structure pointer of seq_file.
 * @param[in]
 *     unused: Structure pointer of void.
 * @return
 *     0, function success.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_slcr_debug_show(struct seq_file *s, void *unused)
{
	struct mtk_slcr *slcr = s->private;
	u32 reg;

	/* Enable debug function */
	writel(DBG_EN, slcr->regs + SLCR_DBG_EN);

	seq_puts(s, "-------------------- SLICER DEUBG --------------------\n");

	seq_puts(s, "-------------------- COMMON PART --------------------\n");
	reg = readl(slcr->regs + SLCR_DBG_CLK);
	seq_printf(s, "APB clock free run counter      :%8d\n",
			(reg & 0xff000000) >> 24);
	seq_printf(s, "MMSYS clock free run counter    :%8d\n",
			(reg & 0xff0000) >> 16);
	seq_printf(s, "DSC clock free run counter      :%8d\n",
			(reg & 0xff00) >> 8);
	seq_printf(s, "Video clock free run counter    :%8d\n",
			(reg & 0xff));

	seq_puts(s, "--------------------- PATG PART ---------------------\n");
	reg = readl(slcr->regs + SLCR_PATG_0);
	seq_printf(s, "Enable pattern generator        :%8d\n",
			(reg & 0x1));
	reg = readl(slcr->regs + SLCR_PATG_2);
	seq_printf(s, "Pattern generator mode          :%8d,",
			(reg & 0x3));
	switch (reg & 0x3) {
	case 0:
		seq_puts(s, "free run\n");
		break;
	case 1:
		seq_puts(s, "hardware trigger(one frame)\n");
		break;
	case 2:
		seq_puts(s, "software trigger(one frame)\n");
		break;
	case 3:
		seq_puts(s, "using input timing\n");
		break;
	}
	reg = readl(slcr->regs + SLCR_PATG_9);
	seq_printf(s, "Initial value of R              :%8d\n",
			(reg & 0x1fff));
	reg = readl(slcr->regs + SLCR_PATG_13);
	seq_printf(s, "Initial value of G              :%8d\n",
			(reg & 0x1fff));
	reg = readl(slcr->regs + SLCR_PATG_17);
	seq_printf(s, "Initial value of B              :%8d\n",
			(reg & 0x1fff));

	reg = readl(slcr->regs + SLCR_PATG_10);
	seq_printf(s, "Addend of R(HxV)                :%8dx%d\n",
			(reg & 0x1fff),
			(reg & 0x1fff0000) >> 16);
	reg = readl(slcr->regs + SLCR_PATG_14);
	seq_printf(s, "Addend of G(HxV)                :%8dx%d\n",
			(reg & 0x1fff),
			(reg & 0x1fff0000) >> 16);
	reg = readl(slcr->regs + SLCR_PATG_18);
	seq_printf(s, "Addend of B(HxV)                :%8dx%d\n",
			(reg & 0x1fff),
			(reg & 0x1fff0000) >> 16);

	reg = readl(slcr->regs + SLCR_PATG_8);
	seq_printf(s, "Cbar of R(widthxheight)         :%8dx%d\n",
			(reg & 0x1fff),
			(reg & 0x1fff0000) >> 16);
	reg = readl(slcr->regs + SLCR_PATG_12);
	seq_printf(s, "Cbar of G(widthxheight)         :%8dx%d\n",
			(reg & 0x1fff),
			(reg & 0x1fff0000) >> 16);
	reg = readl(slcr->regs + SLCR_PATG_16);
	seq_printf(s, "Cbar of B(widthxheight)         :%8dx%d\n",
			(reg & 0x1fff),
			(reg & 0x1fff0000) >> 16);

	seq_puts(s, "--------------------- VIDEO PART ---------------------\n");
	reg = readl(slcr->regs + SLCR_DBG_VID_0);
	seq_printf(s, "Video input size(HxV)           :%8dx%d\n",
			(reg & 0x1fff),
			(reg & 0x1fff0000) >> 16);

	reg = readl(slcr->regs + SLCR_DBG_VID_2);
	seq_printf(s, "Video input interface vs        :%8x\n",
			CHECK_BIT(reg, 8) ? 1:0);
	seq_printf(s, "Video input interface hs        :%8x\n",
			CHECK_BIT(reg, 4) ? 1:0);
	seq_printf(s, "Video input interface de        :%8x\n",
			CHECK_BIT(reg, 0) ? 1:0);

	reg = readl(slcr->regs + SLCR_DBG_VID_4);
	seq_printf(s, "Video PQ size(HxV)              :%8dx%d\n",
				(reg & 0x1fff),
				(reg & 0x1fff0000) >> 16);

	reg = readl(slcr->regs + SLCR_DBG_VID_6);
	seq_printf(s, "Video PQ interface vs           :%8x\n",
			CHECK_BIT(reg, 8) ? 1:0);
	seq_printf(s, "Video PQ interface hs           :%8x\n",
			CHECK_BIT(reg, 4) ? 1:0);
	seq_printf(s, "Video PQ interface de           :%8x\n",
			CHECK_BIT(reg, 0) ? 1:0);

	reg = readl(slcr->regs + SLCR_DBG_VID_12);
	seq_printf(s, "Video output 0 size(HxV)        :%8dx%d\n",
				(reg & 0x1fff),
				(reg & 0x1fff0000) >> 16);

	reg = readl(slcr->regs + SLCR_DBG_VID_20);
	seq_printf(s, "Video output 0 interface valid  :%8x\n",
			CHECK_BIT(reg, 4) ? 1:0);
	seq_printf(s, "Video output 0 interface ready  :%8x\n",
			CHECK_BIT(reg, 0) ? 1:0);

	reg = readl(slcr->regs + SLCR_DBG_VID_13);
	seq_printf(s, "Video output 1 size(HxV)        :%8dx%d\n",
				(reg & 0x1fff),
				(reg & 0x1fff0000) >> 16);

	reg = readl(slcr->regs + SLCR_DBG_VID_21);
	seq_printf(s, "Video output 1 interface valid  :%8x\n",
			CHECK_BIT(reg, 4) ? 1:0);
	seq_printf(s, "Video output 1 interface ready  :%8x\n",
			CHECK_BIT(reg, 0) ? 1:0);

	reg = readl(slcr->regs + SLCR_DBG_VID_14);
	seq_printf(s, "Video output 2 size(HxV)        :%8dx%d\n",
				(reg & 0x1fff),
				(reg & 0x1fff0000) >> 16);

	reg = readl(slcr->regs + SLCR_DBG_VID_22);
	seq_printf(s, "Video output 2 interface valid  :%8x\n",
			CHECK_BIT(reg, 4) ? 1:0);
	seq_printf(s, "Video output 2 interface ready  :%8x\n",
			CHECK_BIT(reg, 0) ? 1:0);

	reg = readl(slcr->regs + SLCR_DBG_VID_15);
	seq_printf(s, "Video output 3 size(HxV)        :%8dx%d\n",
				(reg & 0x1fff),
				(reg & 0x1fff0000) >> 16);

	reg = readl(slcr->regs + SLCR_DBG_VID_23);
	seq_printf(s, "Video output 3 interface valid  :%8x\n",
			CHECK_BIT(reg, 4) ? 1:0);
	seq_printf(s, "Video output 3 interface ready  :%8x\n",
			CHECK_BIT(reg, 0) ? 1:0);

	seq_puts(s, "--------------------- DSC PART ---------------------\n");
	reg = readl(slcr->regs + SLCR_DBG_DSC_0);
	seq_printf(s, "DSC input size(HxV)             :%8dx%d\n",
			(reg & 0x1fff),
			(reg & 0x1fff0000) >> 16);

	reg = readl(slcr->regs + SLCR_DBG_DSC_2);
	seq_printf(s, "DSC input interface vs          :%8x\n",
			CHECK_BIT(reg, 8) ? 1:0);
	seq_printf(s, "DSC input interface hs          :%8x\n",
			CHECK_BIT(reg, 4) ? 1:0);
	seq_printf(s, "DSC input interface de          :%8x\n",
			CHECK_BIT(reg, 0) ? 1:0);

	reg = readl(slcr->regs + SLCR_DBG_DSC_4);
	seq_printf(s, "DSC PQ size(HxV)                :%8dx%d\n",
				(reg & 0x1fff),
				(reg & 0x1fff0000) >> 16);

	reg = readl(slcr->regs + SLCR_DBG_DSC_6);
	seq_printf(s, "DSC PQ interface vs             :%8x\n",
			CHECK_BIT(reg, 8) ? 1:0);
	seq_printf(s, "DSC PQ interface hs             :%8x\n",
			CHECK_BIT(reg, 4) ? 1:0);
	seq_printf(s, "DSC PQ interface de             :%8x\n",
			CHECK_BIT(reg, 0) ? 1:0);

	reg = readl(slcr->regs + SLCR_DBG_DSC_12);
	seq_printf(s, "DSC output 0 size(HxV)          :%8dx%d\n",
				(reg & 0x1fff),
				(reg & 0x1fff0000) >> 16);

	reg = readl(slcr->regs + SLCR_DBG_DSC_20);
	seq_printf(s, "DSC output 0 interface valid    :%8x\n",
			CHECK_BIT(reg, 4) ? 1:0);
	seq_printf(s, "DSC output 0 interface ready    :%8x\n",
			CHECK_BIT(reg, 0) ? 1:0);

	reg = readl(slcr->regs + SLCR_DBG_DSC_13);
	seq_printf(s, "DSC output 1 size(HxV)          :%8dx%d\n",
				(reg & 0x1fff),
				(reg & 0x1fff0000) >> 16);

	reg = readl(slcr->regs + SLCR_DBG_DSC_21);
	seq_printf(s, "DSC output 1 interface valid    :%8x\n",
			CHECK_BIT(reg, 4) ? 1:0);
	seq_printf(s, "DSC output 1 interface ready    :%8x\n",
			CHECK_BIT(reg, 0) ? 1:0);

	reg = readl(slcr->regs + SLCR_DBG_DSC_14);
	seq_printf(s, "DSC output 2 size(HxV)          :%8dx%d\n",
				(reg & 0x1fff),
				(reg & 0x1fff0000) >> 16);

	reg = readl(slcr->regs + SLCR_DBG_DSC_22);
	seq_printf(s, "DSC output 2 interface valid    :%8x\n",
			CHECK_BIT(reg, 4) ? 1:0);
	seq_printf(s, "DSC output 2 interface ready    :%8x\n",
			CHECK_BIT(reg, 0) ? 1:0);

	reg = readl(slcr->regs + SLCR_DBG_DSC_15);
	seq_printf(s, "DSC output 3 size(HxV)          :%8dx%d\n",
				(reg & 0x1fff),
				(reg & 0x1fff0000) >> 16);

	reg = readl(slcr->regs + SLCR_DBG_DSC_23);
	seq_printf(s, "DSC output 3 interface valid    :%8x\n",
			CHECK_BIT(reg, 4) ? 1:0);
	seq_printf(s, "DSC output 3 interface ready    :%8x\n",
			CHECK_BIT(reg, 0) ? 1:0);

	seq_puts(s, "----------------------------------------------------\n");

	/* Disable debug function */
	writel(0, slcr->regs + SLCR_DBG_EN);

	return 0;
}

/** @ingroup IP_group_slicer_internal_function
 * @par Description
 *     Slicer debugfs client open function.
 * @param[in]
 *     inode: Structure pointer of inode.
 * @param[in]
 *     file: Structure pointer of file.
 * @return
 *     result of single_open().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int debug_client_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtk_slcr_debug_show, inode->i_private);
}

/** @ingroup IP_group_slicer_internal_function
 * @par Description
 *     Slicer pattern generator test function.
 * @param[in]
 *     dev: Slicer device node.
 * @param[in]
 *     buf: user space buffer pointer
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void ut0006_patg_test(struct device *dev, char *buf)
{
	const char sp[] = " ";
	char *token;
	int ret;
	unsigned int en;
	unsigned int r_data, g_data, b_data;
	unsigned int h, v;
	unsigned int width, height;
	struct mtk_slcr *slcr = dev_get_drvdata(dev);
	struct reg_info rif;

	rif.pkt = NULL;
	rif.regs = slcr->regs;
	rif.cmdq_subsys = slcr->cmdq_subsys;
	rif.cmdq_offset = slcr->cmdq_offset;

	token = strsep(&buf, sp);

	if (strncmp(token, "enable", 6) == 0) {
		ret = kstrtouint(buf, 10, &en);
		dev_err(dev, "patg enable: %d\n", en);
		reg_write_mask(rif, (en << 0), SLCR_PATG_0, PATG_EN);
	} else if (strncmp(token, "init", 4) == 0) {
		ret = sscanf(buf, "%d %d %d", &r_data, &g_data, &b_data);
		dev_err(dev, "patg init: r=%d, g=%d, b=%d\n",
						r_data, g_data, b_data);
		reg_write_mask(rif, r_data, SLCR_PATG_9, PATG_R_INIT);
		reg_write_mask(rif, g_data, SLCR_PATG_13, PATG_G_INIT);
		reg_write_mask(rif, b_data, SLCR_PATG_17, PATG_B_INIT);
	} else if (strncmp(token, "addend", 6) == 0) {
		token = strsep(&buf, sp);
		ret = sscanf(buf, "%d %d", &h, &v);
		dev_err(dev, "patg addend: h=%d, v=%d ", h, v);
		switch (token[0]) {
		case 'r':
			/* SLCR_PATG_10  */
			dev_err(dev, "r\n");
			reg_write_mask(rif, (h << 0), SLCR_PATG_10,
				PATG_R_H_ADDEND);
			reg_write_mask(rif, (v << 16), SLCR_PATG_10,
				PATG_R_V_ADDEND);
			break;
		case 'g':
			/* SLCR_PATG_14  */
			dev_err(dev, "g\n");
			reg_write_mask(rif, (h << 0), SLCR_PATG_14,
				PATG_G_H_ADDEND);
			reg_write_mask(rif, (v << 16), SLCR_PATG_14,
				PATG_G_V_ADDEND);
			break;
		case 'b':
			/* SLCR_PATG_18 */
			dev_err(dev, "b\n");
			reg_write_mask(rif, (h << 0), SLCR_PATG_18,
				PATG_B_H_ADDEND);
			reg_write_mask(rif, (v << 16), SLCR_PATG_18,
				PATG_B_V_ADDEND);
			break;
		default:
			dev_err(dev, "error rbg parameter\n");
			break;
		}
	} else if (strncmp(token, "cbar", 4) == 0) {
		token = strsep(&buf, sp);
		ret = sscanf(buf, "%d %d", &width, &height);
		dev_err(dev, "patg cbar: width=%d, height=%d ", width, height);
		switch (token[0]) {
		case 'r':
			dev_err(dev, "r\n");
			reg_write_mask(rif, (width << 0), SLCR_PATG_8,
				PATG_R_CBAR_WIDTH);
			reg_write_mask(rif, (height << 16), SLCR_PATG_8,
				PATG_R_CBAR_HEIGHT);
			break;
		case 'g':
			dev_err(dev, "g\n");
			reg_write_mask(rif, (width << 0), SLCR_PATG_12,
				PATG_G_CBAR_WIDTH);
			reg_write_mask(rif, (height << 16), SLCR_PATG_12,
				PATG_G_CBAR_HEIGHT);
			break;
		case 'b':
			dev_err(dev, "b\n");
			reg_write_mask(rif, (width << 0), SLCR_PATG_16,
				PATG_B_CBAR_WIDTH);
			reg_write_mask(rif, (height << 16), SLCR_PATG_16,
				PATG_B_CBAR_HEIGHT);
			break;
		default:
			dev_err(dev, "error rbg parameter\n");
			break;
		}
	}
}

/** @ingroup IP_group_slicer_internal_function
 * @par Description
 *     Slicer debugfs client write function.
 * @param[in]
 *     file: Structure pointer of file.
 * @param[in]
 *     ubuf: user space buffer pointer
 * @param[in]
 *     count: number of data to write
 * @param[in, out]
 *     ppos: offset
 * @return
 *     Negative, file write failed. \n
 *     Others, number of data wrote.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static ssize_t debug_client_write(struct file *file, const char __user *ubuf,
				size_t count, loff_t *ppos)
{
	char *buf = vmalloc(count+1);
	char *vbuf = buf;

	struct seq_file *seq_ptr;
	struct mtk_slcr *slicer;

	const char sp[] = " ";
	char *token;

	if (!vbuf)
		goto out;

	if (copy_from_user(buf, ubuf, count)) {
		vfree(vbuf);
		return -EFAULT;
	}
	seq_ptr = (struct seq_file *)file->private_data;
	slicer = (struct mtk_slcr *)seq_ptr->private;

	buf[count] = '\0';

	token = strsep(&buf, sp);

	/* slicer settings */
	mtk_slcr_reset(slicer->dev, NULL);
	mtk_slcr_patgen_reset(slicer->dev, NULL);

	if (strncmp(token, "patg", 4) == 0)
		ut0006_patg_test(slicer->dev, buf);

	mtk_slcr_start(slicer->dev, NULL);

out:
	vfree(vbuf);
	return count;

}

/** @ingroup IP_group_slicer_internal_struct
 * @brief Slicer file operation struct for debugfs use.
 */
static const struct file_operations slcr_debug_fops = {
	.open = debug_client_open,
	.read = seq_read,
	.write = debug_client_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

/** @ingroup IP_group_slicer_internal_function
 * @par Description
 *     Get Necessary Hardware Information from Device Tree.
 * @param[in]
 *     pdev: platform device node.
 * @return
 *     0, function success.
 *     Otherwise, slicer probe failed.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If there is any error in the probe flow,
 *     system error value will be returned.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_slcr_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_slcr *slcr = NULL;
	struct resource *regs;
#ifdef CONFIG_COMMON_CLK_MT3612
	int i;
#endif
#ifdef CONFIG_MTK_DEBUGFS
	struct dentry *debug_dentry;
#endif

	slcr = devm_kzalloc(dev, sizeof(*slcr), GFP_KERNEL);
	if (!slcr)
		return -ENOMEM;

	slcr->dev = dev;
	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	slcr->regs = devm_ioremap_resource(dev, regs);
	if (IS_ERR(slcr->regs)) {
		dev_err(dev, "Failed to map slicer registers\n");
		return PTR_ERR(slcr->regs);
	}
	slcr->cmdq_offset = (u32) regs->start & 0x0000F000;

	of_property_read_u32(dev->of_node, "gce-subsys",
			&slcr->cmdq_subsys);

#ifdef CONFIG_COMMON_CLK_MT3612
	slcr->slcr_vid_sel = devm_clk_get(dev, "slcr_vid_sel");
	if (IS_ERR(slcr->slcr_vid_sel)) {
		dev_err(dev, "Failed to get slicer video select\n");
		return PTR_ERR(slcr->slcr_vid_sel);
	}

	slcr->slcr_dsc_sel = devm_clk_get(dev, "slcr_dsc_sel");
	if (IS_ERR(slcr->slcr_dsc_sel)) {
		dev_err(dev, "Failed to get slicer dsc select\n");
		return PTR_ERR(slcr->slcr_dsc_sel);
	}

	slcr->clk26m_ck = devm_clk_get(dev, "clk26m_ck");
	if (IS_ERR(slcr->clk26m_ck)) {
		dev_err(dev, "Failed to get 26M clock\n");
		return PTR_ERR(slcr->clk26m_ck);
	}

	slcr->dprx_vid_ck = devm_clk_get(dev, "dprx_vid_ck");
	if (IS_ERR(slcr->dprx_vid_ck)) {
		dev_err(dev, "Failed to get dprx video clock\n");
		return PTR_ERR(slcr->dprx_vid_ck);
	}

	slcr->dprx_dsc_ck = devm_clk_get(dev, "dprx_dsc_ck");
	if (IS_ERR(slcr->dprx_dsc_ck)) {
		dev_err(dev, "Failed to get dprx dsc clock\n");
		return PTR_ERR(slcr->dprx_dsc_ck);
	}

	slcr->univpll_d2 = devm_clk_get(dev, "univpll_d2");
	if (IS_ERR(slcr->univpll_d2)) {
		dev_err(dev, "Failed to get univpll d2 clock\n");
		return PTR_ERR(slcr->univpll_d2);
	}

	for (i = 0; i < 4; i++) {
		char clk[9];

		sprintf(clk, "slcr%d", i);
		slcr->core_slcr[i] = devm_clk_get(dev, clk);
		if (IS_ERR(slcr->core_slcr[i])) {
			dev_err(dev, "Failed to get slcr core %s cll\n", clk);
			return PTR_ERR(slcr->core_slcr[i]);
		}
	}

	slcr->slcr_mm = devm_clk_get(dev, "slcr_mm");
	if (IS_ERR(slcr->slcr_mm)) {
		dev_err(dev, "Failed to get slicer mm cg\n");
		return PTR_ERR(slcr->slcr_mm);
	}

	slcr->slcr_vid = devm_clk_get(dev, "slcr_vid");
	if (IS_ERR(slcr->slcr_vid)) {
		dev_err(dev, "Failed to get slicer video cg\n");
		return PTR_ERR(slcr->slcr_vid);
	}

	slcr->slcr_dsc = devm_clk_get(dev, "slcr_dsc");
	if (IS_ERR(slcr->slcr_dsc)) {
		dev_err(dev, "Failed to get slicer dsc cg\n");
		return PTR_ERR(slcr->slcr_dsc);
	}

#endif
	platform_set_drvdata(pdev, slcr);

#ifdef CONFIG_MTK_DEBUGFS
	if (!mtk_debugfs_root)
		goto debugfs_done;

	/* debug info create */
	if (!mtk_slcr_debugfs_root)
		mtk_slcr_debugfs_root = debugfs_create_dir("slcr",
							    mtk_debugfs_root);

	if (!mtk_slcr_debugfs_root) {
		dev_dbg(dev,
			"failed to create slcr debugfs root directory.\n");
		goto debugfs_done;
	}

	debug_dentry = debugfs_create_file(pdev->name, 0664,
					   mtk_slcr_debugfs_root,
					   slcr, &slcr_debug_fops);

debugfs_done:
	if (!debug_dentry)
		dev_dbg(dev, "Failed to create slcr debugfs %s\n", pdev->name);
#endif

	dev_dbg(dev, "slicer probe done\n");

	return 0;
}

/** @ingroup IP_group_slicer_internal_function
 * @par Description
 *     Do Nothing.
 * @param[in]
 *     pdev: platform device node.
 * @return
 *     return value is 0.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_slcr_remove(struct platform_device *pdev)
{
#ifdef CONFIG_MTK_DEBUGFS
	debugfs_remove_recursive(mtk_slcr_debugfs_root);
	mtk_slcr_debugfs_root = NULL;
#endif

	return 0;
}

/** @ingroup IP_group_slicer_internal_struct
 * @brief Slicer Open Framework Device ID.
 * This structure is used to attach specific names to
 * platform device for use with device tree.
 */
static const struct of_device_id slcr_driver_dt_match[] = {
	{.compatible = "mediatek,mt3612-slcr"},
	{},
};

MODULE_DEVICE_TABLE(of, slcr_driver_dt_match);

/** @ingroup IP_group_slicer_internal_struct
 * @brief Slicer platform driver structure.
 * This structure is used to register itself.
 */
struct platform_driver mtk_slcr_driver = {
	.probe = mtk_slcr_probe,
	.remove = mtk_slcr_remove,
	.driver = {
		   .name = "mediatek-slcr",
		   .owner = THIS_MODULE,
		   .of_match_table = slcr_driver_dt_match,
		   },
};

/** @ingroup IP_group_slicer_internal_function
 * @par Description
 *     Register slicer platform driver.
 * @par Parameters
 *     none.
 * @return
 *     error code from platform_driver_register().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int __init mtk_slicer_init(void)
{
	int ret = 0;

	pr_debug("mtk slicer drv init!\n");

	ret = platform_driver_register(&mtk_slcr_driver);
	if (ret) {
		pr_alert("platform_driver_register failed!\n");
		return ret;
	}
	pr_debug("mtk slicer drv init ok!\n");
	return ret;
}

/** @ingroup IP_group_slicer_internal_function
 * @par Description
 *     Unregister slicer platform driver.
 * @par Parameters
 *     none.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void __exit mtk_slicer_exit(void)
{
	pr_debug("mtk slicer drv exit!\n");
	platform_driver_unregister(&mtk_slcr_driver);
}

module_init(mtk_slicer_init);
module_exit(mtk_slicer_exit);

MODULE_LICENSE("GPL v2");
