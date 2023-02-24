/*
 * Copyright (c) 2017 MediaTek Inc.
 * Authors:
 *	CK Hu <ck.hu@mediatek.com>
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
 * @defgroup IP_group_warpa WPEA
 *     Warpa is the driver of WPEA which is dedicated for real-time image\n
 *     warping. The interface includes parameter setting, status getting, and\n
 *     control function. Parameter setting includes setting of input and\n
 *     output region, input and output buffer, coefficient table, border\n
 *     color, processing mode, and output mode. Status getting includes\n
 *     getting warpa done status. Control function includes power on and off,\n
 *     start, stop, and reset. Warpa has 4 states: power-off-state,\n
 *     stop-state, active-state, and idle-state. Power-on function change\n
 *     power-off-state to stop-state. Power-off function change stop-state to\n
 *     power-off-state. Start function change stop-state to active-state.\n
 *     In active-state, WPEA is trigger. After WPEA is done, the state is\n
 *     changed from active-state to idle-state. The stop function change\n
 *     idle-state to stop-state.
 *
 *     @{
 *         @defgroup IP_group_warpa_external EXTERNAL
 *             The external API document for warpa.\n
 *
 *             @{
 *                 @defgroup IP_group_warpa_external_function 1.function
 *                     Exported function of warpa
 *                 @defgroup IP_group_warpa_external_struct 2.structure
 *                     Exported structure of warpa
 *                 @defgroup IP_group_warpa_external_typedef 3.typedef
 *                     none.
 *                 @defgroup IP_group_warpa_external_enum 4.enumeration
 *                     Exported enumeration of warpa
 *                 @defgroup IP_group_warpa_external_def 5.define
 *                     Exported definition of warpa
 *             @}
 *
 *         @defgroup IP_group_warpa_internal INTERNAL
 *             The internal API document for warpa.\n
 *
 *             @{
 *                 @defgroup IP_group_warpa_internal_function 1.function
 *                     none.
 *                 @defgroup IP_group_warpa_internal_struct 2.structure
 *                     none.
 *                 @defgroup IP_group_warpa_internal_typedef 3.typedef
 *                     none.
 *                 @defgroup IP_group_warpa_internal_enum 4.enumeration
 *                     none.
 *                 @defgroup IP_group_warpa_internal_def 5.define
 *                     none.
 *             @}
 *     @}
 */

#include <linux/clk.h>
#include <linux/compiler.h>
#include <linux/interrupt.h>
#include <linux/iopoll.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/soc/mediatek/mtk-cmdq.h>
#include <linux/device.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <soc/mediatek/mtk_drv_def.h>
#include <soc/mediatek/mtk_warpa.h>
#include <soc/mediatek/smi.h>
#include "mtk_warpa_reg.h"

#ifdef CONFIG_MTK_DEBUGFS
struct dentry *mtk_warpa_debugfs_root;
#endif


#define RSP_COEF(n)	(RSP_1 + 0x2 * (n))

struct reg_trg {
	void __iomem *regs;
	u32 reg_base;
	struct cmdq_pkt *pkt;
	int subsys;
};

static int reg_write_mask(struct reg_trg *trg, u32 offset, u32 value,
		u32 mask)
{
	int ret = 0;

	if (trg->pkt) {
		ret = cmdq_pkt_write_value(trg->pkt, trg->subsys,
				(trg->reg_base & 0xffff) | offset,
				value, mask);
	} else {
		u32 reg;

		reg = readl(trg->regs + offset);
		reg &= ~mask;
		reg |= value;
		writel(reg, trg->regs + offset);
	}

	return ret;
}

static int reg_write(struct reg_trg *trg, u32 offset, u32 value)
{
	int ret = 0;

	if (trg->pkt)
		ret = reg_write_mask(trg, offset, value, 0xffffffff);
	else
		writel(value, trg->regs + offset);

	return ret;
}

static int mtk_warpa_rbfc_enable(struct reg_trg *trg)
{
	return reg_write_mask(trg, RBFC_1, RBFC_1_RBFC_EN, RBFC_1_RBFC_EN);
}

static int mtk_warpa_rbfc_disable(struct reg_trg *trg)
{
	return reg_write_mask(trg, RBFC_1, 0x0, RBFC_1_RBFC_EN);
}

static int mtk_warpa_rbfc_step(struct reg_trg *trg, u32 in_h, u32 out_h)
{
	return reg_write(trg, RBFC_2, ((in_h * 4 << 16) + out_h - 1) / out_h);
}

static int mtk_warpa_set_out_direct_link_enable(struct reg_trg *trg)
{
	return reg_write_mask(trg, SYS_0, 0, SYS_0_DBG_OUT_STOP);
}

static int mtk_warpa_set_out_direct_link_disable(struct reg_trg *trg)
{
	return reg_write_mask(trg, SYS_0, SYS_0_DBG_OUT_STOP,
			SYS_0_DBG_OUT_STOP);
}

static int mtk_warpa_set_out_dram_enable(struct reg_trg *trg)
{
	return reg_write_mask(trg, SYS_0, SYS_0_DBG_OUT_DUMP_EN,
			SYS_0_DBG_OUT_DUMP_EN);
}

static int mtk_warpa_set_out_dram_disable(struct reg_trg *trg)
{
	return reg_write_mask(trg, SYS_0, 0, SYS_0_DBG_OUT_DUMP_EN);
}

static int mtk_warpa_set_mem_request_enable(struct reg_trg *trg)
{
	return reg_write_mask(trg, SYS_0, 0, SYS_0_DBG_MEM_STOP);
}

static int mtk_warpa_set_mem_request_disable(struct reg_trg *trg)
{
	return reg_write_mask(trg, SYS_0, SYS_0_DBG_MEM_STOP,
			SYS_0_DBG_MEM_STOP);
}

static int mtk_warpa_set_proc_mode_l_only(struct reg_trg *trg)
{
	reg_write_mask(trg, SYS_0, 0, SYS_0_PROC_MODE);
	reg_write_mask(trg, SYS_1, 0, SYS_1_PROC_MODE);
	return 0;
}

static int mtk_warpa_set_proc_mode_lr(struct reg_trg *trg)
{
	reg_write_mask(trg, SYS_0, SYS_0_PROC_MODE, SYS_0_PROC_MODE);
	reg_write_mask(trg, SYS_1, 1, SYS_1_PROC_MODE);
	return 0;
}

static int mtk_warpa_set_proc_mode_quad(struct reg_trg *trg)
{
	reg_write_mask(trg, SYS_0, SYS_0_PROC_MODE, SYS_0_PROC_MODE);
	reg_write_mask(trg, SYS_1, SYS_1_PROC_MODE, SYS_1_PROC_MODE);
	return 0;
}

static int mtk_warpa_set_unmapped_color(struct reg_trg *trg, u8 unmapped_color)
{
	return reg_write_mask(trg, SYS_0, (u32)(unmapped_color << 24),
			SYS_0_PROC_UNMAP_VAL);
}

static int mtk_warpa_set_border_color_enable(struct reg_trg *trg,
		u8 border_color)
{
	return reg_write_mask(trg, SYS_0,
			SYS_0_PROC_EDGE_VAL_SEL | (border_color << 16),
			SYS_0_PROC_EDGE_VAL_SEL | SYS_0_PROC_BORDER_VAL);
}

static int mtk_warpa_set_border_color_disable(struct reg_trg *trg)
{
	return reg_write_mask(trg, SYS_0, 0, SYS_0_PROC_EDGE_VAL_SEL);
}

static int mtk_warpa_coef_tbl_sel(struct reg_trg *trg,
		u32 tbl)
{
	return reg_write_mask(trg, RSP_0, tbl, RSP_0_RSP_TBL_SEL);
}

static int mtk_warpa_set_user_coef(struct reg_trg *trg,
		u32 idx, u16 val0, u16 val1)
{
	return reg_write(trg, RSP_COEF(idx), val0 | (val1 << 16));
}

static int mtk_warpa_enable(struct reg_trg *trg)
{
	return reg_write_mask(trg, SYS_0, SYS_0_ENG_EN_BYSOF,
		SYS_0_ENG_EN_BYSOF);
}

static int mtk_warpa_disable(struct reg_trg *trg)
{
	return reg_write_mask(trg, SYS_0, 0, SYS_0_ENG_EN);
}

static int mtk_warpa_rst(const struct device *dev, void __iomem *regs)
{
	u32 status;
	u32 reg = readl(regs + SYS_0);

	reg |= SYS_0_DBG_OUT_STOP;
	writel(reg, regs + SYS_0);

	reg |= SYS_0_DBG_MEM_STOP;
	writel(reg, regs + SYS_0);

	if (readl_poll_timeout_atomic(regs + WPE_STATUS, status,
			status & WPE_STATUS_WPE_MF_MEM_FIN, 1000,
			1000000)) {
		dev_err(dev, "mtk_warpa_rst: WPEA_MS_FIN timeout!\n");

		return -EBUSY;
	}

	reg |= SYS_0_SW_RST;
	writel(reg, regs + SYS_0);

	reg &= ~SYS_0_DBG_OUT_STOP;
	writel(reg, regs + SYS_0);

	reg &= ~SYS_0_DBG_MEM_STOP;
	writel(reg, regs + SYS_0);

	reg &= ~SYS_0_SW_RST;
	writel(reg, regs + SYS_0);

	dev_err(dev, "mtk_warpa_rst: reset done!\n");

	return 0;
}

static int mtk_warpa_in_size(struct reg_trg *trg,
		u32 w, u32 h)
{
	return reg_write(trg, SRC_IMG_0, w | (h << 16));
}

static int mtk_warpa_in_buf_no0(struct reg_trg *trg,
		dma_addr_t addr, u32 pitch)
{
	int ret;

	ret = reg_write(trg, SRC_IMG_1, addr);
	if (ret)
		return ret;

	return reg_write(trg, SRC_IMG_2, pitch);
}

static int mtk_warpa_in_buf_no1(struct reg_trg *trg,
		dma_addr_t addr, u32 pitch)
{
	int ret;

	ret = reg_write(trg, SRC_IMG_3, addr);
	if (ret)
		return ret;

	return reg_write(trg, SRC_IMG_4, pitch);
}

static int mtk_warpa_in_buf_no2(struct reg_trg *trg,
		dma_addr_t addr, u32 pitch)
{
	int ret;

	ret = reg_write(trg, SRC_IMG_5, addr);
	if (ret)
		return ret;

	return reg_write(trg, SRC_IMG_6, pitch);
}

static int mtk_warpa_in_buf_no3(struct reg_trg *trg,
		dma_addr_t addr, u32 pitch)
{
	int ret;

	ret = reg_write(trg, SRC_IMG_7, addr);
	if (ret)
		return ret;

	return reg_write(trg, SRC_IMG_8, pitch);
}

static int mtk_warpa_out_size(struct reg_trg *trg,
		u32 w, u32 h)
{
	return reg_write(trg, DST_IMG_0, w | (h << 16));
}

static int mtk_warpa_out_buf(struct reg_trg *trg,
		dma_addr_t addr, u32 pitch)
{
	int ret;

	ret = reg_write(trg, DST_IMG_1, addr);
	if (ret)
		return ret;

	return reg_write(trg, DST_IMG_2, pitch);
}

static int mtk_warpa_vld_buf(struct reg_trg *trg,
		dma_addr_t addr, u32 pitch)
{
	int ret;

	ret = reg_write(trg, DST_IMG_3, addr);
	if (ret)
		return ret;

	return reg_write(trg, DST_IMG_4, pitch);
}

static int mtk_warpa_vtx_size(struct reg_trg *trg,
		u32 w, u32 h)
{
	return reg_write(trg, WARPMAP_0, w | (h << 8));
}

static int mtk_warpa_vtx_buf_no0(struct reg_trg *trg,
		dma_addr_t addr, u32 pitch)
{
	int ret;

	ret = reg_write(trg, WARPMAP_1, addr);
	if (ret)
		return ret;

	return reg_write_mask(trg, WARPMAP_2, pitch,
			WARPMAP_2_VTX_TBL_NO0_STRIDE);
}

static int mtk_warpa_vtx_buf_no1(struct reg_trg *trg,
		dma_addr_t addr, u32 pitch)
{
	int ret;

	ret = reg_write(trg, WARPMAP_3, addr);
	if (ret)
		return ret;

	return reg_write_mask(trg, WARPMAP_2, pitch << 16,
			WARPMAP_2_VTX_TBL_NO1_STRIDE);
}

static int mtk_warpa_vtx_buf_no2(struct reg_trg *trg,
		dma_addr_t addr, u32 pitch)
{
	int ret;

	ret = reg_write(trg, WARPMAP_4, addr);
	if (ret)
		return ret;

	return reg_write_mask(trg, WARPMAP_5, pitch,
			WARPMAP_5_VTX_TBL_NO2_STRIDE);
}

static int mtk_warpa_vtx_buf_no3(struct reg_trg *trg,
		dma_addr_t addr, u32 pitch)
{
	int ret;

	ret = reg_write(trg, WARPMAP_6, addr);
	if (ret)
		return ret;

	return reg_write_mask(trg, WARPMAP_5, pitch << 16,
			WARPMAP_5_VTX_TBL_NO3_STRIDE);
}

static bool mtk_warpa_is_wpe_done(void __iomem *regs)
{
	return (readl(regs + WPE_STATUS) & WPE_STATUS_WPE_ALL_IDLE) > 0;
}

struct mtk_warpa {
	struct device *dev;
	struct device *dev_larb;
	struct clk *clk_warpa_1_tx;
	struct clk *clk_warpa_1_1_tx;
	struct clk *clk_warpa_1_2_tx;
	struct clk *clk_warpa_1;
	struct clk *clk_warpa_0_tx;
	struct clk *clk_warpa_0;
	void __iomem *regs;
	u32 reg_base;
	u32 subsys;
	bool out_dram;
	bool out_direct_link;
	u32 map_w;
	u32 map_h;
	u32 in_w;
	u32 in_h;
	u32 out_w;
	u32 out_h;
	spinlock_t spinlock_irq;
	int irq;
	mtk_mmsys_cb cb_func;
	u32 cb_status;
	void *cb_priv_data;
};

/** @ingroup IP_group_warpa_external_function
 * @par Description
 *     Start the functon of WPEA.
 * @param[in]
 *     dev: pointer of warpa device structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @return
 *     1. 0, successfully append command into the cmdq command buffer.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     Call this function in stop-state.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_warpa_start(const struct device *dev, struct cmdq_pkt *handle)
{
	struct mtk_warpa *warpa;
	struct reg_trg trg;
	int ret;

	if (!dev)
		return -EINVAL;
	warpa = dev_get_drvdata(dev);
	if (!warpa)
		return -EINVAL;

	trg.regs = warpa->regs;
	trg.reg_base = warpa->reg_base;
	trg.pkt = (struct cmdq_pkt *)handle;
	trg.subsys = warpa->subsys;

	ret = mtk_warpa_vtx_size(&trg, warpa->map_w, warpa->map_h);
	if (ret)
		return ret;

	ret = mtk_warpa_in_size(&trg, warpa->in_w, warpa->in_h);
	if (ret)
		return ret;

	ret = mtk_warpa_out_size(&trg, warpa->out_w, warpa->out_h);
	if (ret)
		return ret;

	ret = mtk_warpa_rbfc_step(&trg, warpa->in_h, warpa->out_h);
	if (ret)
		return ret;

	if (warpa->out_direct_link) {
		ret = mtk_warpa_set_out_direct_link_enable(&trg);
		if (ret)
			return ret;
	}

	if (warpa->out_dram) {
		ret = mtk_warpa_set_out_dram_enable(&trg);
		if (ret)
			return ret;
	}

	ret = mtk_warpa_set_mem_request_enable(&trg);
	if (ret)
		return ret;

	ret = mtk_warpa_rbfc_enable(&trg);
	if (ret)
		return ret;

	return mtk_warpa_enable(&trg);
}
EXPORT_SYMBOL(mtk_warpa_start);

/** @ingroup IP_group_warpa_external_function
 * @par Description
 *     Stop the functon of WPEA.
 * @param[in]
 *     dev: pointer of warpa device structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @return
 *     1. 0, successfully append command into the cmdq command buffer.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     Call this function in idle-state.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_warpa_stop(const struct device *dev, struct cmdq_pkt *handle)
{
	struct mtk_warpa *warpa;
	struct reg_trg trg;
	int ret;

	if (!dev)
		return -EINVAL;

	warpa = dev_get_drvdata(dev);
	if (!warpa)
		return -EINVAL;

	trg.regs = warpa->regs;
	trg.reg_base = warpa->reg_base;
	trg.pkt = (struct cmdq_pkt *)handle;
	trg.subsys = warpa->subsys;

	ret = mtk_warpa_disable(&trg);
	if (ret)
		return ret;

	ret = mtk_warpa_rbfc_disable(&trg);
	if (ret)
		return ret;

	ret = mtk_warpa_set_mem_request_disable(&trg);
	if (ret)
		return ret;

	ret = mtk_warpa_set_out_dram_disable(&trg);
	if (ret)
		return ret;

	return mtk_warpa_set_out_direct_link_disable(&trg);
}
EXPORT_SYMBOL(mtk_warpa_stop);

/** @ingroup IP_group_warpa_external_function
 * @par Description
 *     Reset WPEA. While WPEA get into a malfunction state, reset it to\n
 *         normal state.
 * @param[in]
 *     dev: pointer of warpa device structure.
 * @return
 *     1. 0, successfully reset.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. -EBUSY, device busy. not in stop-state.
 * @par Boundary case and Limitation
 *     Call this function in stop-state.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_warpa_reset(const struct device *dev)
{
	struct mtk_warpa *warpa;

	if (!dev)
		return -EINVAL;

	warpa = dev_get_drvdata(dev);
	if (!warpa)
		return -EINVAL;

	return mtk_warpa_rst(dev, warpa->regs);
}
EXPORT_SYMBOL(mtk_warpa_reset);

/** @ingroup IP_group_warpa_external_function
 * @par Description
 *     Set the region of warp map.
 * @param[in]
 *     dev: pointer of warpa device structure.
 * @param[in]
 *     w: width.
 * @param[in]
 *     h: height.
 * @return
 *     1. 0, successfully set region.\n
 *     2. -EINVAL, invalid parameter.\n
 * @par Boundary case and Limitation
 *     1. Call this function in stop-state.
 *     2. w should be in the range of 2 to 81.\n
 *        h should be in the range of 2 to 81.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_warpa_set_region_map(const struct device *dev, u32 w, u32 h)
{
	struct mtk_warpa *warpa;

	if (!dev)
		return -EINVAL;

	warpa = dev_get_drvdata(dev);
	if (!warpa)
		return -EINVAL;

	if (w < 2 || w > 81) {
		dev_err(dev, "mtk_warpa_set_region_map: error invalid w %d\n",
				w);
		return -EINVAL;
	}
	if (h < 2 || h > 81) {
		dev_err(dev, "mtk_warpa_set_region_map: error invalid h %d\n",
				h);
		return -EINVAL;
	}

	warpa->map_w = w;
	warpa->map_h = h;

	return 0;
}
EXPORT_SYMBOL(mtk_warpa_set_region_map);

/** @ingroup IP_group_warpa_external_function
 * @par Description
 *     Set the region of warp input source image.
 * @param[in]
 *     dev: pointer of warpa device structure.
 * @param[in]
 *     w: width.
 * @param[in]
 *     h: height.
 * @return
 *     1. 0, successfully set region.\n
 *     2. -EINVAL, invalid parameter.\n
 * @par Boundary case and Limitation
 *     1. Call this function in stop-state.
 *     2. w should be in the range of 16 to 640.\n
 *        h should be in the range of 16 to 640.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_warpa_set_region_in(const struct device *dev, u32 w, u32 h)
{
	struct mtk_warpa *warpa;

	if (!dev)
		return -EINVAL;

	warpa = dev_get_drvdata(dev);
	if (!warpa)
		return -EINVAL;

	if (w < 16 || w > 640) {
		dev_err(dev, "mtk_warpa_set_region_in: error invalid w %d\n",
				w);
		return -EINVAL;
	}
	if (h < 16 || h > 640) {
		dev_err(dev, "mtk_warpa_set_region_in: error invalid h %d\n",
				h);
		return -EINVAL;
	}

	warpa->in_w = w;
	warpa->in_h = h;

	return 0;
}
EXPORT_SYMBOL(mtk_warpa_set_region_in);

/** @ingroup IP_group_warpa_external_function
 * @par Description
 *     Set the region of warp output destination image.
 * @param[in]
 *     dev: pointer of warpa device structure.
 * @param[in]
 *     w: width.
 * @param[in]
 *     h: height.
 * @return
 *     1. 0, successfully set region.\n
 *     2. -EINVAL, invalid parameter.\n
 * @par Boundary case and Limitation
 *     1. Call this function in stop-state.
 *     2. w should be in the range of 16 to 2560.\n
 *        h should be in the range of 16 to 640.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_warpa_set_region_out(const struct device *dev, u32 w, u32 h)
{
	struct mtk_warpa *warpa;

	if (!dev)
		return -EINVAL;

	warpa = dev_get_drvdata(dev);
	if (!warpa)
		return -EINVAL;

	if (w < 16 || w > 2560) {
		dev_err(dev, "mtk_warpa_set_region_out: error invalid w %d\n",
				w);
		return -EINVAL;
	}
	if (h < 16 || h > 640) {
		dev_err(dev, "mtk_warpa_set_region_out: error invalid h %d\n",
				h);
		return -EINVAL;
	}

	warpa->out_w = w;
	warpa->out_h = h;

	return 0;
}
EXPORT_SYMBOL(mtk_warpa_set_region_out);

enum mtk_warpa_buf_type {
	MTK_WARPA_IN_BUF_IMG_NO0 = 0,
	MTK_WARPA_IN_BUF_IMG_NO1,
	MTK_WARPA_IN_BUF_VTX_NO0,
	MTK_WARPA_IN_BUF_VTX_NO1,
	MTK_WARPA_OUT_BUF_IMG,
	MTK_WARPA_OUT_BUF_VLD,
	MTK_WARPA_IN_BUF_IMG_NO2,
	MTK_WARPA_IN_BUF_IMG_NO3,
	MTK_WARPA_IN_BUF_VTX_NO2,
	MTK_WARPA_IN_BUF_VTX_NO3,
};

static int mtk_warpa_set_buf_common(const struct device *dev,
				    struct cmdq_pkt *handle,
				    enum mtk_warpa_buf_type idx,
				    dma_addr_t addr, u32 pitch)
{
	struct mtk_warpa *warpa;
	struct reg_trg trg;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	warpa = dev_get_drvdata(dev);
	if (!warpa)
		return -EINVAL;

	trg.regs = warpa->regs;
	trg.reg_base = warpa->reg_base;
	trg.pkt = (struct cmdq_pkt *)handle;
	trg.subsys = warpa->subsys;

	if (addr & 0x3f) {
		dev_err(dev, "mtk_warpa_set_buf(%d): error invalid addr %pad\n",
				idx, &addr);
		return -EINVAL;
	}

	if (pitch & 0x3f) {
		dev_err(dev, "mtk_warpa_set_buf(%d): error invalid pitch %d\n",
				idx, pitch);
		return -EINVAL;
	}

	switch (idx) {
	case MTK_WARPA_IN_BUF_IMG_NO0:
		ret = mtk_warpa_in_buf_no0(&trg, addr, pitch);
		break;
	case MTK_WARPA_IN_BUF_IMG_NO1:
		ret = mtk_warpa_in_buf_no1(&trg, addr, pitch);
		break;
	case MTK_WARPA_IN_BUF_IMG_NO2:
		ret = mtk_warpa_in_buf_no2(&trg, addr, pitch);
		break;
	case MTK_WARPA_IN_BUF_IMG_NO3:
		ret = mtk_warpa_in_buf_no3(&trg, addr, pitch);
		break;
	case MTK_WARPA_IN_BUF_VTX_NO0:
		ret = mtk_warpa_vtx_buf_no0(&trg, addr, pitch);
		break;
	case MTK_WARPA_IN_BUF_VTX_NO1:
		ret = mtk_warpa_vtx_buf_no1(&trg, addr, pitch);
		break;
	case MTK_WARPA_IN_BUF_VTX_NO2:
		ret = mtk_warpa_vtx_buf_no2(&trg, addr, pitch);
		break;
	case MTK_WARPA_IN_BUF_VTX_NO3:
		ret = mtk_warpa_vtx_buf_no3(&trg, addr, pitch);
		break;
	case MTK_WARPA_OUT_BUF_IMG:
		ret = mtk_warpa_out_buf(&trg, addr, pitch);
		break;
	case MTK_WARPA_OUT_BUF_VLD:
		ret = mtk_warpa_vld_buf(&trg, addr, pitch);
		break;
	default:
		dev_err(dev, "mtk_warpa_set_buf_common: error invalid idx %d\n",
				idx);
		return -EINVAL;
	}

	return ret;
}

/** @ingroup IP_group_warpa_external_function
 * @par Description
 *     Set address and pitch of NO.0 warp map.
 * @param[in]
 *     dev: pointer of warpa device structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @param[in]
 *     addr: the address of this buffer. It is physical address\n
 *         without iommu. It is iova with iommu.
 * @param[in]
 *     pitch: the pitch of this buffer. The pitch means the distance\n
 *         (in bytes) between the first pixel of two consecutive line.
 * @return
 *     1. 0, successfully append command into the cmdq command buffer.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. Call this function in stop-state or idle-state.\n
 *     2. addr should be multiple of 64. pitch should be greater than zero\n
 *         and be multiple of 64.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_warpa_set_buf_map_no0(const struct device *dev, struct cmdq_pkt *handle,
			    dma_addr_t addr, u32 pitch)
{
	if (!dev)
		return -EINVAL;

	return mtk_warpa_set_buf_common(dev, handle, MTK_WARPA_IN_BUF_VTX_NO0,
					addr, pitch);
}
EXPORT_SYMBOL(mtk_warpa_set_buf_map_no0);

/** @ingroup IP_group_warpa_external_function
 * @par Description
 *     Set address and pitch of NO.1 warp map.
 * @param[in]
 *     dev: pointer of warpa device structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @param[in]
 *     addr: the address of this buffer. It is physical address\n
 *         without iommu. It is iova with iommu.
 * @param[in]
 *     pitch: the pitch of this buffer. The pitch means the distance\n
 *         (in bytes) between the first pixel of two consecutive line.
 * @return
 *     1. 0, successfully append command into the cmdq command buffer.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. Call this function in stop-state or idle-state.\n
 *     2. addr should be multiple of 64. pitch should be greater than zero\n
 *         and be multiple of 64.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_warpa_set_buf_map_no1(const struct device *dev, struct cmdq_pkt *handle,
			    dma_addr_t addr, u32 pitch)
{
	if (!dev)
		return -EINVAL;

	return mtk_warpa_set_buf_common(dev, handle, MTK_WARPA_IN_BUF_VTX_NO1,
					addr, pitch);
}
EXPORT_SYMBOL(mtk_warpa_set_buf_map_no1);

/** @ingroup IP_group_warpa_external_function
 * @par Description
 *     Set address and pitch of NO.2 warp map.
 * @param[in]
 *     dev: pointer of warpa device structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @param[in]
 *     addr: the address of this buffer. It is physical address\n
 *         without iommu. It is iova with iommu.
 * @param[in]
 *     pitch: the pitch of this buffer. The pitch means the distance\n
 *         (in bytes) between the first pixel of two consecutive line.
 * @return
 *     1. 0, successfully append command into the cmdq command buffer.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. Call this function in stop-state or idle-state.\n
 *     2. addr should be multiple of 64. pitch should be greater than zero\n
 *         and be multiple of 64.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_warpa_set_buf_map_no2(const struct device *dev, struct cmdq_pkt *handle,
			    dma_addr_t addr, u32 pitch)
{
	if (!dev)
		return -EINVAL;

	return mtk_warpa_set_buf_common(dev, handle, MTK_WARPA_IN_BUF_VTX_NO2,
					addr, pitch);
}
EXPORT_SYMBOL(mtk_warpa_set_buf_map_no2);

/** @ingroup IP_group_warpa_external_function
 * @par Description
 *     Set address and pitch of NO.3 warp map.
 * @param[in]
 *     dev: pointer of warpa device structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @param[in]
 *     addr: the address of this buffer. It is physical address\n
 *         without iommu. It is iova with iommu.
 * @param[in]
 *     pitch: the pitch of this buffer. The pitch means the distance\n
 *         (in bytes) between the first pixel of two consecutive line.
 * @return
 *     1. 0, successfully append command into the cmdq command buffer.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. Call this function in stop-state or idle-state.\n
 *     2. addr should be multiple of 64. pitch should be greater than zero\n
 *         and be multiple of 64.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_warpa_set_buf_map_no3(const struct device *dev, struct cmdq_pkt *handle,
			    dma_addr_t addr, u32 pitch)
{
	return mtk_warpa_set_buf_common(dev, handle, MTK_WARPA_IN_BUF_VTX_NO3,
					addr, pitch);
}
EXPORT_SYMBOL(mtk_warpa_set_buf_map_no3);

/** @ingroup IP_group_warpa_external_function
 * @par Description
 *     Set address and pitch of NO.0 input image buffer.
 * @param[in]
 *     dev: pointer of warpa device structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @param[in]
 *     addr: the address of this buffer. It is physical address\n
 *         without iommu. It is iova with iommu.
 * @param[in]
 *     pitch: the pitch of this buffer. The pitch means the distance\n
 *         (in bytes) between the first pixel of two consecutive line.
 * @return
 *     1. 0, successfully append command into the cmdq command buffer.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. Call this function in stop-state or idle-state.\n
 *     2. addr should be multiple of 64. pitch should be greater than zero\n
 *         and be multiple of 64.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_warpa_set_buf_in_no0(const struct device *dev, struct cmdq_pkt *handle,
			   dma_addr_t addr, u32 pitch)
{
	if (!dev)
		return -EINVAL;

	return mtk_warpa_set_buf_common(dev, handle, MTK_WARPA_IN_BUF_IMG_NO0,
					addr, pitch);
}
EXPORT_SYMBOL(mtk_warpa_set_buf_in_no0);

/** @ingroup IP_group_warpa_external_function
 * @par Description
 *     Set address and pitch of NO.1 input image buffer.
 * @param[in]
 *     dev: pointer of warpa device structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @param[in]
 *     addr: the address of this buffer. It is physical address\n
 *         without iommu. It is iova with iommu.
 * @param[in]
 *     pitch: the pitch of this buffer. The pitch means the distance\n
 *         (in bytes) between the first pixel of two consecutive line.
 * @return
 *     1. 0, successfully append command into the cmdq command buffer.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. Call this function in stop-state or idle-state.\n
 *     2. addr should be multiple of 64. pitch should be greater than zero\n
 *         and be multiple of 64.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_warpa_set_buf_in_no1(const struct device *dev, struct cmdq_pkt *handle,
			   dma_addr_t addr, u32 pitch)
{
	if (!dev)
		return -EINVAL;

	return mtk_warpa_set_buf_common(dev, handle, MTK_WARPA_IN_BUF_IMG_NO1,
					addr, pitch);
}
EXPORT_SYMBOL(mtk_warpa_set_buf_in_no1);

/** @ingroup IP_group_warpa_external_function
 * @par Description
 *     Set address and pitch of NO.2 input image buffer.
 * @param[in]
 *     dev: pointer of warpa device structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @param[in]
 *     addr: the address of this buffer. It is physical address\n
 *         without iommu. It is iova with iommu.
 * @param[in]
 *     pitch: the pitch of this buffer. The pitch means the distance\n
 *         (in bytes) between the first pixel of two consecutive line.
 * @return
 *     1. 0, successfully append command into the cmdq command buffer.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. Call this function in stop-state or idle-state.\n
 *     2. addr should be multiple of 64. pitch should be greater than zero\n
 *         and be multiple of 64.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_warpa_set_buf_in_no2(const struct device *dev, struct cmdq_pkt *handle,
			   dma_addr_t addr, u32 pitch)
{
	if (!dev)
		return -EINVAL;

	return mtk_warpa_set_buf_common(dev, handle, MTK_WARPA_IN_BUF_IMG_NO2,
					addr, pitch);
}
EXPORT_SYMBOL(mtk_warpa_set_buf_in_no2);

/** @ingroup IP_group_warpa_external_function
 * @par Description
 *     Set address and pitch of NO.3 input image buffer.
 * @param[in]
 *     dev: pointer of warpa device structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @param[in]
 *     addr: the address of this buffer. It is physical address\n
 *         without iommu. It is iova with iommu.
 * @param[in]
 *     pitch: the pitch of this buffer. The pitch means the distance\n
 *         (in bytes) between the first pixel of two consecutive line.
 * @return
 *     1. 0, successfully append command into the cmdq command buffer.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. Call this function in stop-state or idle-state.\n
 *     2. addr should be multiple of 64. pitch should be greater than zero\n
 *         and be multiple of 64.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_warpa_set_buf_in_no3(const struct device *dev, struct cmdq_pkt *handle,
			   dma_addr_t addr, u32 pitch)
{
	if (!dev)
		return -EINVAL;

	return mtk_warpa_set_buf_common(dev, handle, MTK_WARPA_IN_BUF_IMG_NO3,
					addr, pitch);
}
EXPORT_SYMBOL(mtk_warpa_set_buf_in_no3);


/** @ingroup IP_group_warpa_external_function
 * @par Description
 *     Set address and pitch of output image buffer.
 * @param[in]
 *     dev: pointer of warpa device structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @param[in]
 *     addr: the address of this buffer. It is physical address\n
 *         without iommu. It is iova with iommu.
 * @param[in]
 *     pitch: the pitch of this buffer. The pitch means the distance\n
 *         (in bytes) between the first pixel of two consecutive line.
 * @return
 *     1. 0, successfully append command into the cmdq command buffer.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. Call this function in stop-state or idle-state.\n
 *     2. addr should be multiple of 64. pitch should be greater than zero\n
 *         and be multiple of 64.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_warpa_set_buf_out_img(const struct device *dev,
			      struct cmdq_pkt *handle,
			      dma_addr_t addr, u32 pitch)
{
	if (!dev)
		return -EINVAL;

	return mtk_warpa_set_buf_common(dev, handle, MTK_WARPA_OUT_BUF_IMG,
					addr, pitch);
}
EXPORT_SYMBOL(mtk_warpa_set_buf_out_img);

/** @ingroup IP_group_warpa_external_function
 * @par Description
 *     Set address and pitch of output valid buffer.
 * @param[in]
 *     dev: pointer of warpa device structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @param[in]
 *     addr: the address of this buffer. It is physical address\n
 *         without iommu. It is iova with iommu.
 * @param[in]
 *     pitch: the pitch of this buffer. The pitch means the distance\n
 *         (in bytes) between the first pixel of two consecutive line.
 * @return
 *     1. 0, successfully append command into the cmdq command buffer.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. Call this function in stop-state or idle-state.\n
 *     2. addr should be multiple of 64. pitch should be greater than zero\n
 *         and be multiple of 64.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_warpa_set_buf_out_vld(const struct device *dev,
			      struct cmdq_pkt *handle,
			      dma_addr_t addr, u32 pitch)
{
	if (!dev)
		return -EINVAL;

	return mtk_warpa_set_buf_common(dev, handle, MTK_WARPA_OUT_BUF_VLD,
					addr, pitch);
}
EXPORT_SYMBOL(mtk_warpa_set_buf_out_vld);

/** @ingroup IP_group_warpa_external_function
 * @par Description
 *     Turn on the power of warpa. Before any configuration, you should\n
 *         turn on the power of warpa.
 * @param[in]
 *     dev: pointer of warpa device structure.
 * @return
 *     1. 0, successfully append command into the cmdq command buffer.\n
 *     2. -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     Call this function in power-off-state.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_warpa_power_on(struct device *dev)
{
	struct mtk_warpa *warpa;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	warpa = dev_get_drvdata(dev);
	if (!warpa)
		return -EINVAL;

	ret = mtk_smi_larb_get(warpa->dev_larb);
	if (ret < 0) {
		dev_err(dev, "Failed to get larb: %d\n", ret);
		goto err_return;
	}

	ret = pm_runtime_get_sync(dev);
	if (ret < 0) {
		dev_err(dev, "Failed to enable power domain: %d\n", ret);
		goto err_larb;
	}

	ret = clk_prepare_enable(warpa->clk_warpa_1_tx);
	if (ret) {
		dev_err(dev, "Failed to enable clk_warpa_1_tx:%d\n", ret);
		goto err_pm;
	}

	ret = clk_prepare_enable(warpa->clk_warpa_1_1_tx);
	if (ret) {
		dev_err(dev, "Failed to enable clk_warpa_1_1_tx:%d\n", ret);
		goto err_clk_warpa_1_tx;
	}

	ret = clk_prepare_enable(warpa->clk_warpa_1_2_tx);
	if (ret) {
		dev_err(dev, "Failed to enable clk_warpa_1_2_tx:%d\n", ret);
		goto err_clk_warpa_1_1_tx;
	}

	ret = clk_prepare_enable(warpa->clk_warpa_1);
	if (ret) {
		dev_err(dev, "Failed to enable clk_warpa_1:%d\n", ret);
		goto err_clk_warpa_1_2_tx;
	}

	ret = clk_prepare_enable(warpa->clk_warpa_0_tx);
	if (ret) {
		dev_err(dev, "Failed to enable clk_warpa_0_tx:%d\n", ret);
		goto err_clk_warpa_1;
	}

	ret = clk_prepare_enable(warpa->clk_warpa_0);
	if (ret) {
		dev_err(dev, "Failed to enable clk_warpa_0:%d\n", ret);
		goto err_clk_warpa_0_tx;
	}

	ret = mtk_warpa_stop(dev, NULL);
	if (ret) {
		dev_err(dev, "Failed to stop warpa:%d\n", ret);
		goto err_clk_warpa_0;
	}

	return 0;

err_clk_warpa_0:
	clk_disable_unprepare(warpa->clk_warpa_0);
err_clk_warpa_0_tx:
	clk_disable_unprepare(warpa->clk_warpa_0_tx);
err_clk_warpa_1:
	clk_disable_unprepare(warpa->clk_warpa_1);
err_clk_warpa_1_2_tx:
	clk_disable_unprepare(warpa->clk_warpa_1_2_tx);
err_clk_warpa_1_1_tx:
	clk_disable_unprepare(warpa->clk_warpa_1_1_tx);
err_clk_warpa_1_tx:
	clk_disable_unprepare(warpa->clk_warpa_1_tx);
err_pm:
	pm_runtime_put(dev);
err_larb:
	mtk_smi_larb_put(warpa->dev_larb);
err_return:
	return ret;

}
EXPORT_SYMBOL(mtk_warpa_power_on);

/** @ingroup IP_group_warpa_external_function
 * @par Description
 *     Turn off the power of warpa.
 * @param[in]
 *     dev: pointer of warpa device structure.
 * @return
 *     1. 0, successfully append command into the cmdq command buffer.\n
 *     2. -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     Call this function in stop-state.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_warpa_power_off(struct device *dev)
{
	struct mtk_warpa *warpa;

	if (!dev)
		return -EINVAL;

	warpa = dev_get_drvdata(dev);
	if (!warpa)
		return -EINVAL;

	clk_disable_unprepare(warpa->clk_warpa_1_tx);
	clk_disable_unprepare(warpa->clk_warpa_1_1_tx);
	clk_disable_unprepare(warpa->clk_warpa_1_2_tx);
	clk_disable_unprepare(warpa->clk_warpa_1);
	clk_disable_unprepare(warpa->clk_warpa_0_tx);
	clk_disable_unprepare(warpa->clk_warpa_0);

	pm_runtime_put(dev);
	mtk_smi_larb_put(warpa->dev_larb);

	return 0;
}
EXPORT_SYMBOL(mtk_warpa_power_off);

/** @ingroup IP_group_warpa_external_function
 * @par Description
 *     Query whether warpa is in done status.
 * @param[in]
 *     dev: pointer of warpa device structure.
 * @return
 *     1. true, WPEA is in done status.\n
 *     2. false, WPEA is in active status.
 * @par Boundary case and Limitation
 *     Call this function in active-state or idle-state.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_warpa_get_wpe_done(const struct device *dev)
{
	struct mtk_warpa *warpa;

	if (!dev)
		return -EINVAL;

	warpa = dev_get_drvdata(dev);
	if (!warpa)
		return -EINVAL;

	return mtk_warpa_is_wpe_done(warpa->regs);
}
EXPORT_SYMBOL(mtk_warpa_get_wpe_done);

/** @ingroup IP_group_warpa_external_function
 * @par Description
 *     Set coefficient table.
 * @param[in]
 *     dev: pointer of warpa device structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @param[in]
 *     coef_tbl: coefficient table.
 * @return
 *     1. 0, successfully append command into the cmdq command buffer.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. Call this function in stop-state or idle-state.
 *     2. coef_tbl could not be NULL. coef_tbl->idx should be in the range of\n
 *         0 to 5. If coef_tbl->idx equals MTK_WARPA_USER_COEF_TABLE,\n
 *         coef_tbl->user_coef_tbl cound not be NULL.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_warpa_set_coef_tbl(const struct device *dev, struct cmdq_pkt *handle,
			   struct mtk_warpa_coef_tbl *coef_tbl)
{
	struct mtk_warpa *warpa;
	struct reg_trg trg;
	int ret;
	int i;

	if (!dev)
		return -EINVAL;

	warpa = dev_get_drvdata(dev);
	if (!warpa)
		return -EINVAL;

	trg.regs = warpa->regs;
	trg.reg_base = warpa->reg_base;
	trg.pkt = (struct cmdq_pkt *)handle;
	trg.subsys = warpa->subsys;

	if (!coef_tbl) {
		dev_err(dev, "mtk_warpa_set_coef_tbl: error invalid coef_tbl = NULL\n");
		return -EINVAL;
	}

	if (coef_tbl->idx > MTK_WARPA_USER_COEF_TABLE) {
		dev_err(dev, "mtk_warpa_set_coef_tbl: error invalid coef_tbl->idx %d\n",
				coef_tbl->idx);
		return -EINVAL;
	}

	ret = mtk_warpa_coef_tbl_sel(&trg, coef_tbl->idx);
	if (ret)
		return ret;

	if (coef_tbl->idx == MTK_WARPA_USER_COEF_TABLE) {
		for (i = 0; i < (MTK_WARPA_COEF_TABLE_SIZE - 1); i += 2) {
			ret = mtk_warpa_set_user_coef(&trg, i,
					coef_tbl->user_coef_tbl[i],
					coef_tbl->user_coef_tbl[i + 1]);
			if (ret)
				return ret;
		}

		ret = mtk_warpa_set_user_coef(&trg, i,
				coef_tbl->user_coef_tbl[i], 0);
		if (ret)
			return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mtk_warpa_set_coef_tbl);

/** @ingroup IP_group_warpa_external_function
 * @par Description
 *     Set border color and unmapped color.
 * @param[in]
 *     dev: pointer of warpa device structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @param[in]
 *     color: information of border color and unmapped color including\n
 *         enable information.
 * @return
 *     1. 0, successfully append command into the cmdq command buffer.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. Call this function in stop-state or idle-state.
 *     2. color could not be NULL.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_warpa_set_border_color(const struct device *dev,
			       struct cmdq_pkt *handle,
			       struct mtk_warpa_border_color *color)
{
	struct mtk_warpa *warpa;
	struct reg_trg trg;
	int ret;

	if (!dev)
		return -EINVAL;
	if (!color) {
	dev_err(dev, "warp set border color: error invalid color = NULL\n");
		return -EINVAL;
	}
	if ((color->border_color > 0xff) || (color->unmapped_color > 0xff)) {
		dev_err(dev, "warp set border color: invalid color setting\n");
		return -EINVAL;
	}

	warpa = dev_get_drvdata(dev);
	if (!warpa)
		return -EINVAL;

	trg.regs = warpa->regs;
	trg.reg_base = warpa->reg_base;
	trg.pkt = (struct cmdq_pkt *)handle;
	trg.subsys = warpa->subsys;

	ret = mtk_warpa_set_unmapped_color(&trg, color->unmapped_color);
	if (ret)
		return ret;

	if (color->enable)
		return mtk_warpa_set_border_color_enable(&trg,
			color->border_color);
	else
		return mtk_warpa_set_border_color_disable(&trg);
}
EXPORT_SYMBOL(mtk_warpa_set_border_color);

/** @ingroup IP_group_warpa_external_function
 * @par Description
 *     Set proccessing mode. Process left eye only or both eye.
 * @param[in]
 *     dev: pointer of warpa device structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @param[in]
 *     proc_mode: should be 0, 1 or 3.\n
 *         0 for one input, 1 for two inputs, and 3 for four inputs.
 * @return
 *     1. 0, successfully append command into the cmdq command buffer.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. Call this function in stop-state or idle-state.
 *     2. proc_mode should be MTK_WARPA_PROC_MODE_L_ONLY or\n
 *         MTK_WARPA_PROC_MODE_LR.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_warpa_set_proc_mode(const struct device *dev, struct cmdq_pkt *handle,
			    u32 proc_mode)
{
	struct mtk_warpa *warpa;
	struct reg_trg trg;

	if (!dev)
		return -EINVAL;

	warpa = dev_get_drvdata(dev);
	if (!warpa)
		return -EINVAL;

	trg.regs = warpa->regs;
	trg.reg_base = warpa->reg_base;
	trg.pkt = (struct cmdq_pkt *)handle;
	trg.subsys = warpa->subsys;

	switch (proc_mode) {
	case MTK_WARPA_PROC_MODE_LR:
		return mtk_warpa_set_proc_mode_lr(&trg);
	case MTK_WARPA_PROC_MODE_L_ONLY:
		return mtk_warpa_set_proc_mode_l_only(&trg);
	case MTK_WARPA_PROC_MODE_QUAD:
		return mtk_warpa_set_proc_mode_quad(&trg);
	default:
		dev_err(dev, "mtk_warpa_set_proc_mode invalid proc_mode %d\n",
				proc_mode);
		return -EINVAL;
	}
}
EXPORT_SYMBOL(mtk_warpa_set_proc_mode);

/** @ingroup IP_group_warpa_external_function
 * @par Description
 *     Set output mode. Output to dram, direct link or both.
 * @param[in]
 *     dev: pointer of warpa device structure.
 * @param[in]
 *     out_mode: MTK_WARPA_OUT_MODE_DRAM for output to dram,\n
 *         MTK_WARPA_OUT_MODE_DIRECT_LINK for output to direct link.\n
 *         MTK_WARPA_OUT_MODE_ALL for output to both.
 * @return
 *     1. 0, successfully set out mode.\n
 *     2. -EINVAL, invalid parameter.\n
 * @par Boundary case and Limitation
 *     1. Call this function in stop-state.\n
 *     2. out_mode should be MTK_WARPA_OUT_MODE_ALL, MTK_WARPA_OUT_MODE_DRAM,\n
 *         or MTK_WARPA_OUT_MODE_DIRECT_LINK.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_warpa_set_out_mode(const struct device *dev, u32 out_mode)
{
	struct mtk_warpa *warpa;

	if (!dev)
		return -EINVAL;

	warpa = dev_get_drvdata(dev);
	if (!warpa)
		return -EINVAL;

	switch (out_mode) {
	case MTK_WARPA_OUT_MODE_ALL:
		warpa->out_direct_link = true;
		warpa->out_dram = true;
		break;
	case MTK_WARPA_OUT_MODE_DRAM:
		warpa->out_direct_link = false;
		warpa->out_dram = true;
		break;
	case MTK_WARPA_OUT_MODE_DIRECT_LINK:
		warpa->out_direct_link = true;
		warpa->out_dram = false;
		break;
	default:
		dev_err(dev, "mtk_warpa_set_out_mode: error invalid out_mode %d\n",
				out_mode);
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(mtk_warpa_set_out_mode);

static irqreturn_t mtk_warpa_irq_handler(int irq, void *dev_id)
{
	struct mtk_warpa *warpa = dev_id;
	u32 status;
	struct mtk_mmsys_cb_data cb_data;
	mtk_mmsys_cb func;
	unsigned long flags;

	status = readl(warpa->regs + WPE_INT_STATUS);
	status = status & WPE_INT_STATUS_WPE_INT_STATUS;
	writel(status << 16, warpa->regs + WPE_INT_STATUS);

	spin_lock_irqsave(&warpa->spinlock_irq, flags);
	func = warpa->cb_func;
	cb_data.status = warpa->cb_status & status;
	cb_data.priv_data = warpa->cb_priv_data;
	spin_unlock_irqrestore(&warpa->spinlock_irq, flags);

	cb_data.part_id = 0;

	if (func && cb_data.status)
		func(&cb_data);

	return IRQ_HANDLED;
}

/** @ingroup IP_group_warpa_external_function
 * @par Description
 *     Register callback function for warpa interrupts.
 * @param[in]
 *     dev: pointer of warpa device structure.
 * @param[in]
 *     func: callback function which is called by warpa interrupt handler.
 * @param[in]
 *     status: bit map of registered interrupts, #mtk_warpa_cb_status.
 * @param[in]
 *     priv_data: caller may have multiple instance, so it could use this\n
 *         private data to distinguish which instance is callbacked.
 * @return
 *     1. 0, successfully register callback function.\n
 *     2. -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_warpa_register_cb(const struct device *dev, mtk_mmsys_cb func,
			  enum mtk_warpa_cb_status status, void *priv_data)
{
	struct mtk_warpa *warpa;
	unsigned long flags;

	if (WARN_ON(!dev))
		return -EINVAL;

	warpa = dev_get_drvdata(dev);
	if (!warpa)
		return -EINVAL;

	spin_lock_irqsave(&warpa->spinlock_irq, flags);
	warpa->cb_func = func;
	warpa->cb_status = status;
	warpa->cb_priv_data = priv_data;
	spin_unlock_irqrestore(&warpa->spinlock_irq, flags);

	/* enable interrupts: clear mask*/
	status = ~status & WPE_INT_CTRL_WPE_INT_MASK;
	writel(status, warpa->regs + WPE_INT_CTRL);

	return 0;
}
EXPORT_SYMBOL(mtk_warpa_register_cb);

#ifdef CONFIG_MTK_DEBUGFS
static int warpa_debug_show(struct seq_file *s, void *unused)
{
	struct mtk_warpa *warpa = s->private;
	u32 reg;

	seq_puts(s, "--------------------- WPE DEUBG ---------------------\n");

	reg = readl(warpa->regs + SYS_0);
	seq_printf(s, "WPE SYS BASIC SETTING 0x0:     0x%x\n", reg);

	reg = readl(warpa->regs + WPE_INT_STATUS);
	seq_printf(s, "WPE INT RAW STATUS:            %d\n",
		   (u32)((reg & (WPE_INT_STATUS_WPE_INT_RAW_STATUS)) >> 16));

	reg = readl(warpa->regs + WPE_STATUS);
	seq_printf(s, "WPE STATUS 0xc[11:0]:          0x%x\n",
			(u32)(reg & 0xFFF));

	reg = readl(warpa->regs + SYS_1);
	seq_printf(s, "WPE PROC MODE:                 %d\n", reg);
	seq_printf(s, "(WPE INPUT %d SINGLE SRC IMG(s))\n", (reg+1));

	reg = readl(warpa->regs + SYS_0);
	seq_printf(s, "WPE DIRECT LINK ENABLE:        %d\n",
		    (u32)((~reg) & (SYS_0_DBG_OUT_STOP)) >> 6);
	seq_printf(s, "WPE OUT DRAM ENABLE:           %d\n",
		    (u32)(reg & (SYS_0_DBG_OUT_DUMP_EN)) >> 7);

	reg = readl(warpa->regs + SRC_IMG_0);
	seq_printf(s, "WPE SINGLE SRC IMG WIDTH:      %d\n",
		   (u32)(reg & (SRC_IMG_0_SRC_IMG_WIDTH)));
	seq_printf(s, "WPE SINGLE SRC IMG HEIGHT:     %d\n",
		   (u32)(reg & (SRC_IMG_0_SRC_IMG_HEIGHT)) >> 16);

	reg = readl(warpa->regs + DST_IMG_0);
	seq_printf(s, "WPE OUTPUT WIDTH:              %d\n",
		   (u32)(reg & (DST_IMG_0_DST_IMG_WIDTH)));
	seq_printf(s, "WPE OUTPUT HEIGHT:             %d\n",
		   (u32)(reg & (DST_IMG_0_DST_IMG_HEIGHT)) >> 16);

	reg = readl(warpa->regs + WARPMAP_0);
	seq_printf(s, "WPE MAP WIDTH:                 %d\n",
		   (u32)(reg & (WARPMAP_0_VTX_TBL_WIDTH)));
	seq_printf(s, "WPE MAP HEIGHT:                %d\n",
		   (u32)(reg & (WARPMAP_0_VTX_TBL_HEIGHT)) >> 8);

	reg = readl(warpa->regs + SRC_IMG_1);
	seq_printf(s, "WPE INPUT_IMG0_ADDR:           0x%08x\n", reg);
	reg = readl(warpa->regs + SRC_IMG_3);
	seq_printf(s, "WPE INPUT_IMG1_ADDR:           0x%08x\n", reg);
	reg = readl(warpa->regs + SRC_IMG_5);
	seq_printf(s, "WPE INPUT_IMG2_ADDR:           0x%08x\n", reg);
	reg = readl(warpa->regs + SRC_IMG_7);
	seq_printf(s, "WPE INPUT_IMG3_ADDR:           0x%08x\n", reg);
	reg = readl(warpa->regs + SRC_IMG_2);
	seq_printf(s, "WPE INPUT_IMG0_STRIDE:         %d\n", reg);
	reg = readl(warpa->regs + SRC_IMG_4);
	seq_printf(s, "WPE INPUT_IMG1_STRIDE:         %d\n", reg);
	reg = readl(warpa->regs + SRC_IMG_6);
	seq_printf(s, "WPE INPUT_IMG2_STRIDE:         %d\n", reg);
	reg = readl(warpa->regs + SRC_IMG_8);
	seq_printf(s, "WPE INPUT_IMG3_STRIDE:         %d\n", reg);

	reg = readl(warpa->regs + DST_IMG_1);
	seq_printf(s, "WPE OUTPUT_IMG_ADDR:           0x%08x\n", reg);
	reg = readl(warpa->regs + DST_IMG_2);
	seq_printf(s, "WPE OUTPUT_IMG_STRIDE:         %d\n", reg);

	reg = readl(warpa->regs + WARPMAP_1);
	seq_printf(s, "WPE MAP_0_ADDR:                0x%08x\n", reg);
	reg = readl(warpa->regs + WARPMAP_3);
	seq_printf(s, "WPE MAP_1_ADDR:                0x%08x\n", reg);
	reg = readl(warpa->regs + WARPMAP_4);
	seq_printf(s, "WPE MAP_2_ADDR:                0x%08x\n", reg);
	reg = readl(warpa->regs + WARPMAP_6);
	seq_printf(s, "WPE MAP_3_ADDR:                0x%08x\n", reg);
	reg = readl(warpa->regs + WARPMAP_2);
	seq_printf(s, "WPE MAP_0_STRIDE:              %d\n",
		   (u32)(reg & (WARPMAP_2_VTX_TBL_NO0_STRIDE)));
	seq_printf(s, "WPE MAP_1_STRIDE:              %d\n",
		   (u32)((reg & (WARPMAP_2_VTX_TBL_NO1_STRIDE)) >> 16));
	reg = readl(warpa->regs + WARPMAP_5);
	seq_printf(s, "WPE MAP_2_STRIDE:              %d\n",
		   (u32)(reg & (WARPMAP_5_VTX_TBL_NO2_STRIDE)));
	seq_printf(s, "WPE MAP_3_STRIDE:              %d\n",
		   (u32)((reg & (WARPMAP_5_VTX_TBL_NO3_STRIDE)) >> 16));

	seq_puts(s, "----------------------------------------------------\n");
	return 0;
}


static int debug_client_open(struct inode *inode, struct file *file)
{
	return single_open(file, warpa_debug_show, inode->i_private);
}

static const struct file_operations warpa_debug_fops = {
	.open = debug_client_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif


static int mtk_warpa_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_warpa *warpa;
	struct resource *regs;
	struct device_node *larb_node;
	struct platform_device *larb_pdev;
	const void *ptr;
	int ret;
#ifdef CONFIG_MTK_DEBUGFS
		struct dentry *debug_dentry = NULL;
#endif

	warpa = devm_kzalloc(dev, sizeof(*warpa), GFP_KERNEL);
	if (!warpa)
		return -ENOMEM;

#ifdef CONFIG_COMMON_CLK_MT3612
	/* side camera warpa clock */
	ret =  of_property_match_string(dev->of_node, "clock-names",
					"clk_warpa_1_tx");
	if (ret >= 0) {
		warpa->clk_warpa_1_tx = devm_clk_get(dev, "clk_warpa_1_tx");
		if (IS_ERR(warpa->clk_warpa_1_tx)) {
			dev_err(dev, "Failed to get clk_warpa_1_tx\n");
			return PTR_ERR(warpa->clk_warpa_1_tx);
		}
	}

	ret =  of_property_match_string(dev->of_node, "clock-names",
					"clk_warpa_1_1_tx");
	if (ret >= 0) {
		warpa->clk_warpa_1_1_tx = devm_clk_get(dev, "clk_warpa_1_1_tx");
		if (IS_ERR(warpa->clk_warpa_1_1_tx)) {
			dev_err(dev, "Failed to get clk_warpa_1_1_tx\n");
			return PTR_ERR(warpa->clk_warpa_1_1_tx);
		}
	}

	ret =  of_property_match_string(dev->of_node, "clock-names",
					"clk_warpa_1_2_tx");
	if (ret >= 0) {
		warpa->clk_warpa_1_2_tx = devm_clk_get(dev, "clk_warpa_1_2_tx");
		if (IS_ERR(warpa->clk_warpa_1_2_tx)) {
			dev_err(dev, "Failed to get clk_warpa_1_2_tx\n");
			return PTR_ERR(warpa->clk_warpa_1_2_tx);
		}
	}

	ret =  of_property_match_string(dev->of_node, "clock-names",
					"clk_warpa_1");
	if (ret >= 0) {
		warpa->clk_warpa_1 = devm_clk_get(dev, "clk_warpa_1");
		if (IS_ERR(warpa->clk_warpa_1)) {
			dev_err(dev, "Failed to get clk_warpa_1\n");
			return PTR_ERR(warpa->clk_warpa_1);
		}
	}

	/* gaze camera warpa clock */
	ret =  of_property_match_string(dev->of_node, "clock-names",
					"clk_warpa_0_tx");
	if (ret >= 0) {
		warpa->clk_warpa_0_tx = devm_clk_get(dev, "clk_warpa_0_tx");
		if (IS_ERR(warpa->clk_warpa_0_tx)) {
			dev_err(dev, "Failed to get clk_warpa_0_tx\n");
			return PTR_ERR(warpa->clk_warpa_0_tx);
		}
	}

	ret =  of_property_match_string(dev->of_node, "clock-names",
					"clk_warpa_0");
	if (ret >= 0) {
		warpa->clk_warpa_0 = devm_clk_get(dev, "clk_warpa_0");
		if (IS_ERR(warpa->clk_warpa_0)) {
			dev_err(dev, "Failed to get clk_warpa_0\n");
			return PTR_ERR(warpa->clk_warpa_0);
		}
	}
#endif

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	warpa->regs = devm_ioremap_resource(dev, regs);
	if (IS_ERR(warpa->regs)) {
		dev_err(dev, "Failed to map warpa registers\n");
		return PTR_ERR(warpa->regs);
	}
	warpa->reg_base = (u32)regs->start;

	spin_lock_init(&warpa->spinlock_irq);

	warpa->irq = platform_get_irq(pdev, 0);
	if (warpa->irq < 0) {
		dev_err(dev, "Failed to get warpa irq: %d\n", warpa->irq);
		return warpa->irq;
	}

	/* mask all interrupts */
	writel(WPE_INT_CTRL_WPE_INT_MASK, warpa->regs + WPE_INT_CTRL);

	ret = devm_request_irq(dev, warpa->irq, mtk_warpa_irq_handler,
			IRQF_TRIGGER_LOW, dev_name(dev), warpa);
	if (ret < 0) {
		dev_err(dev, "Failed to request warpa irq: %d\n", ret);
		return ret;
	}

	ptr = of_get_property(dev->of_node, "gce-subsys", NULL);
	if (ptr)
		warpa->subsys = be32_to_cpup(ptr);
	else
		warpa->subsys = 0;

	of_node_put(dev->of_node);

	larb_node = of_parse_phandle(dev->of_node, "mediatek,larb", 0);
	if (!larb_node) {
		dev_err(dev, "Missing mediadek,larb phandle\n");
		return -EINVAL;
	}

	larb_pdev = of_find_device_by_node(larb_node);
	if (!dev_get_drvdata(&larb_pdev->dev)) {
		dev_warn(dev, "Waiting for larb device %s\n",
			 larb_node->full_name);
		of_node_put(larb_node);
		return -EPROBE_DEFER;
	}
	of_node_put(larb_node);
	warpa->dev_larb = &larb_pdev->dev;

	pm_runtime_enable(dev);

	platform_set_drvdata(pdev, warpa);

#ifdef CONFIG_MTK_DEBUGFS
	if (!mtk_debugfs_root)
		goto debugfs_done;

	/* debug info create */
	if (!mtk_warpa_debugfs_root)
		mtk_warpa_debugfs_root = debugfs_create_dir("warpa",
						mtk_debugfs_root);

	if (!mtk_warpa_debugfs_root) {
		dev_dbg(dev, "failed to create warpa debugfs root directory.\n");
		goto debugfs_done;
	}

	debug_dentry = debugfs_create_file(pdev->name, 0664,
					   mtk_warpa_debugfs_root,
					   warpa, &warpa_debug_fops);

debugfs_done:
	if (!debug_dentry)
		dev_warn(dev, "Failed to create warpa debugfs %s\n",
			pdev->name);

#endif


	return 0;
}

static int mtk_warpa_remove(struct platform_device *pdev)
{
	struct mtk_warpa *warpa = platform_get_drvdata(pdev);

	devm_free_irq(&pdev->dev, warpa->irq, warpa);
	pm_runtime_disable(&pdev->dev);

#ifdef CONFIG_MTK_DEBUGFS
		debugfs_remove_recursive(mtk_warpa_debugfs_root);
		mtk_warpa_debugfs_root = NULL;
#endif

	return 0;
}

static const struct of_device_id warpa_driver_dt_match[] = {
	{ .compatible = "mediatek,mt3612-warpa" },
	{},
};
MODULE_DEVICE_TABLE(of, warpa_driver_dt_match);

struct platform_driver mtk_warpa_driver = {
	.probe		= mtk_warpa_probe,
	.remove		= mtk_warpa_remove,
	.driver		= {
		.name	= "mediatek-warpa",
		.owner	= THIS_MODULE,
		.of_match_table = warpa_driver_dt_match,
	},
};

module_platform_driver(mtk_warpa_driver);
MODULE_LICENSE("GPL");
