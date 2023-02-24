/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	Shan Lin <Shan.Lin@mediatek.com>
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

#include <linux/clk.h>
#include <linux/compiler.h>
#include <linux/iopoll.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/device.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <linux/soc/mediatek/mtk-cmdq.h>
#include <soc/mediatek/mtk_fe.h>
#include <soc/mediatek/smi.h>
#include "mtk_fe_reg.h"

#ifdef CONFIG_MTK_DEBUGFS
struct dentry *mtk_fe_debugfs_root;
#endif

/** @ingroup IP_group_fe_internal_def
 * @brief check bit value definition
 */
#define CHECK_BIT(var, pos) ((var) & (1 << (pos)))
#define FE_REG_MAX_OFFSET	0x68

/**
 * @defgroup IP_group_fe FE
 *     Fe is the driver of FE which is dedicated for feature extraction.\n
 *     The interface includes parameter setting, status getting, and\n
 *     control function. Parameter setting includes setting of input and\n
 *     output region, output buffer, and block size. Status getting includes\n
 *     getting fe done status. Control function includes power on and off,\n
 *     start, stop, and reset. Fe has 4 states: power-off-state,\n
 *     stop-state, active-state, and idle-state. Power-on function change\n
 *     power-off-state to stop-state. Power-off function change stop-state to\n
 *     power-off-state. Start function change stop-state to active-state.\n
 *     In active-state, FE is trigger. After FE is done, the state is\n
 *     changed from active-state to idle-state. The stop function change\n
 *     idle-state to stop-state.
 *
 *     @{
 *         @defgroup IP_group_fe_external EXTERNAL
 *             The external API document for fe.\n
 *
 *             @{
 *                 @defgroup IP_group_fe_external_function 1.function
 *                     Exported function of fe
 *                 @defgroup IP_group_fe_external_struct 2.structure
 *                     Exported structure of fe
 *                 @defgroup IP_group_fe_external_typedef 3.typedef
 *                     none.
 *                 @defgroup IP_group_fe_external_enum 4.enumeration
 *                     Exported enumeration of fe
 *                 @defgroup IP_group_fe_external_def 5.define
 *                     Exported definition of fe
 *             @}
 *
 *         @defgroup IP_group_fe_internal INTERNAL
 *             The internal API document for fe.\n
 *
 *             @{
 *                 @defgroup IP_group_fe_internal_function 1.function
 *                     none.
 *                 @defgroup IP_group_fe_internal_struct 2.structure
 *                     none.
 *                 @defgroup IP_group_fe_internal_typedef 3.typedef
 *                     none.
 *                 @defgroup IP_group_fe_internal_enum 4.enumeration
 *                     none.
 *                 @defgroup IP_group_fe_internal_def 5.define
 *                     none.
 *             @}
 *     @}
 */

struct reg_trg {
	void __iomem *regs;
	u32 reg_base;
	struct cmdq_pkt *pkt;
	int subsys;
};

static int reg_write_mask(struct reg_trg *trg, u32 offset, u32 value, u32 mask)
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

static bool mtk_fe_is_fe_done(void __iomem *regs)
{
	u32 reg = readl(regs + FE_RST);
	u32 done_flag = FEO_FE_DONE;

	return (reg & done_flag) == done_flag;
}

static int mtk_fe_enable(struct reg_trg *trg)
{
	int ret = 0;

	ret = reg_write_mask(trg,
		FE_CTRL2, FE_EN | FE_DESCR_EN,
		FE_EN | FE_DESCR_EN);
	if (ret)
		return ret;
	ret = reg_write_mask(trg, FE_WDMA_CTRL, FE_WDMA_EN, FE_WDMA_EN);

	return ret;
}

static int mtk_fe_disable(struct reg_trg *trg)
{
	return reg_write_mask(trg, FE_CTRL2, 0, FE_EN | FE_DESCR_EN);
}

static void mtk_fe_rst(void __iomem *regs)
{
	u32 reg = readl(regs + FE_RST);

	reg |= SW_RST;
	writel(reg, regs + FE_RST);

	reg &= ~SW_RST;
	writel(reg, regs + FE_RST);
}

static int mtk_fe_params(struct reg_trg *trg,
	u32 flat_enable, u32 harris_kappa, u32 th_grad, u32 th_cr,
	u32 cr_val_sel_mode)
{
	int ret = 0;

	ret = reg_write_mask(trg, FE_CTRL1, flat_enable << 9, FE_FLT_EN);
	if (ret)
		return ret;

	ret = reg_write_mask(trg, FE_CTRL1, harris_kappa << 2, FE_PARAM);
	if (ret)
		return ret;

	ret = reg_write_mask(trg, FE_CTRL1, th_grad << 10, FE_TH_G);
	if (ret)
		return ret;

	ret = reg_write_mask(trg, FE_CTRL1, th_cr << 18, FE_TH_C);
	if (ret)
		return ret;

	ret = reg_write_mask(trg, FE_CTRL2, cr_val_sel_mode << 6,
			     FE_CR_VALUE_SEL);
	if (ret)
		return ret;

	return 0;
}

static int mtk_fe_crop(struct reg_trg *trg, u32 in_w, u32 in_h,
		       u32 in_crop_x, u32 in_crop_y, u32 in_crop_w,
		       u32 in_crop_h, u32 blk_sz, u32 mode)
{
	int ret = 0;

	ret = reg_write(trg, FE_SIZE, in_w | (in_h << 16));
	if (ret)
		return ret;
	ret = reg_write(trg, FE_CROP_CTRL1, in_crop_x | (in_crop_y << 16));
	if (ret)
		return ret;
	ret = reg_write(trg, FE_CROP_CTRL2, in_crop_w | (in_crop_h << 16));
	if (ret)
		return ret;

	ret = reg_write_mask(trg, FE_CTRL2, mode << 7, FE_MERGE_MODE);
	if (ret)
		return ret;

	return 0;
}

static int mtk_fe_out_point_buf(struct reg_trg *trg, u32 idx, dma_addr_t addr)
{
	int ret = 0;

	switch (idx) {
	case 0:
		ret = reg_write(trg, FE_WDMA_BASE_ADDR0_CFG, addr);
		break;
	case 1:
		ret = reg_write(trg, FE_WDMA_BASE_ADDR1_CFG, addr);
		break;
	case 2:
		ret = reg_write(trg, FE_WDMA_BASE_ADDR2_CFG, addr);
		break;
	case 3:
		ret = reg_write(trg, FE_WDMA_BASE_ADDR3_CFG, addr);
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int mtk_fe_out_descriptor_buf(struct reg_trg *trg, u32 idx,
				     dma_addr_t addr)
{
	int ret = 0;

	switch (idx) {
	case 0:
		ret = reg_write(trg, FE_WDMA_BASE_ADDR4_CFG, addr);
		break;
	case 1:
		ret = reg_write(trg, FE_WDMA_BASE_ADDR5_CFG, addr);
		break;
	case 2:
		ret = reg_write(trg, FE_WDMA_BASE_ADDR6_CFG, addr);
		break;
	case 3:
		ret = reg_write(trg, FE_WDMA_BASE_ADDR7_CFG, addr);
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

/** @ingroup IP_group_fe_internal_enum
 * @brief Distinguish between FPGA and A0 code
 */
enum dev_stage {
	FPGA = 0,
	A0 = 1,
};

/** @ingroup IP_group_fe_internal_enum
 * @brief define for valid functions
 */
#define LARB_IOMMU (1 << 0)
#define CLK (1 << 1)

/** @ingroup IP_group_fe_internal_struct
 * @brief FE Driver Data Structure.
 */
struct mtk_fe {
	struct device			*dev;
	struct device			*dev_larb;
	struct clk			*clk;
	void __iomem			*regs;
	u32				blk_sz;
	u32 reg_base;
	u32 subsys;
	u32 valid_func;
};

/** @ingroup IP_group_fe_external_function
 * @par Description
 *     Start the functon of FE.
 * @param[in]
 *     dev: pointer of fe device structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @return
 *     0, successfully start fe.
 *     -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If fe dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fe_start(const struct device *dev, struct cmdq_pkt *handle)
{
	struct mtk_fe *fe;
	struct reg_trg trg;

	if (!dev)
		return -EINVAL;
	fe = dev_get_drvdata(dev);
	if (!fe)
		return -EINVAL;

	trg.regs = fe->regs;
	trg.reg_base = fe->reg_base;
	trg.pkt = handle;
	trg.subsys = fe->subsys;

	return mtk_fe_enable(&trg);
}
EXPORT_SYMBOL(mtk_fe_start);

/** @ingroup IP_group_fe_external_function
 * @par Description
 *     Stop the functon of FE.
 * @param[in]
 *     dev: pointer of wdma device structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @return
 *     0, successfully stop fe.
 *     -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If fe dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fe_stop(const struct device *dev, struct cmdq_pkt *handle)
{
	struct mtk_fe *fe;
	struct reg_trg trg;

	if (!dev)
		return -EINVAL;

	fe = dev_get_drvdata(dev);
	if (!fe)
		return -EINVAL;

	trg.regs = fe->regs;
	trg.reg_base = fe->reg_base;
	trg.pkt = handle;
	trg.subsys = fe->subsys;

	return mtk_fe_disable(&trg);
}
EXPORT_SYMBOL(mtk_fe_stop);

/** @ingroup IP_group_fe_external_function
 * @par Description
 *     Reset the functon of FE.
 * @param[in]
 *     dev: pointer of fe device structure.
 * @return
 *     0, successfully reset fe.
 *     -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If fe dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fe_reset(const struct device *dev)
{
	struct mtk_fe *fe;

	if (!dev)
		return -EINVAL;
	fe = dev_get_drvdata(dev);

	if (!fe)
		return -EINVAL;

	mtk_fe_rst(fe->regs);
	return 0;
}
EXPORT_SYMBOL(mtk_fe_reset);

/** @ingroup IP_group_fe_external_function
 * @par Description
 *     set fe params, low-pass filter enable, harris kappa value,
 *     gradient threshold for calculate CR, CR threshold for valid corner.
 * @param[in]
 *     dev: pointer of fe device structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration
 *         to register directly. Otherwise, write the configuration to
 *         command queue packet.
 * @param[in]
 *     flat_enable: apply low-pass filter or not, 1 to apply or 0 not apply.
 * @param[in]
 *     harris_kappa: harris kappa value, k is generated by
 *         (harris_kappa + 1) >> 7, range 0 to 127.
 * @param[in]
 *     th_grad: gradient threshold only when image gradients larger than
 *         th_grad will calculate CR, range 0 to 255.
 * @param[in]
 *     th_cr: if max CR in a tile is smaller than th_er, there is no valid
 *         coner in this tile, range 0 to 16383.
 * @param[in]
 *     cr_val_sel_mode: the cr select mode. 0 for origial mode, 1 for abs
 *         value select mode.
 * @return
 *     0, successfully set region.\n
 *     -EINVAL, invalid parameter.\n
 * @par Boundary case and Limitation
 *     flat_enable should be 1 or 0.
 *     harris_kappa should be in the range of 0 to 127.
 *     th_grad should be in the range of 0 to 255.
 *     th_cr should be in the range of 0 to 16383.
 *     cr_val_sel_mode should be 1 or 0.
 * @par Error case and Error handling
 *     If fe dev is NULL, return -EINVAL.
 *     If input parameter is invalid, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fe_set_params(const struct device *dev, struct cmdq_pkt *handle,
	u32 flat_enable, u32 harris_kappa, u32 th_grad, u32 th_cr,
	u32 cr_val_sel_mode)
{
	struct mtk_fe *fe;
	struct reg_trg trg;

	if (!dev)
		return -EINVAL;

	fe = dev_get_drvdata(dev);
	trg.regs = fe->regs;
	trg.reg_base = fe->reg_base;
	trg.pkt = handle;
	trg.subsys = fe->subsys;

	if (flat_enable > 0x1) {
		dev_err(dev,
			"mtk_fe_set_params: error flat_enable %d\n",
			flat_enable);
		return -EINVAL;
	}

	if (harris_kappa > 0x7F) {
		dev_err(dev,
			"mtk_fe_set_params: error harris_kappa %d\n",
			harris_kappa);
		return -EINVAL;
	}

	if (th_grad > 0xFF) {
		dev_err(dev,
			"mtk_fe_set_params: error th_grad %d\n",
			th_grad);
		return -EINVAL;
	}

	if (th_cr > 0x3FFF) {
		dev_err(dev,
			"mtk_fe_set_params: error th_cr %d\n",
			th_cr);
		return -EINVAL;
	}

	if (cr_val_sel_mode > 0x1) {
		dev_err(dev, "mtk_fe_set_params: error cr_val_sel_mode %d\n",
			cr_val_sel_mode);
		return -EINVAL;
	}

	return mtk_fe_params(&trg, flat_enable, harris_kappa, th_grad, th_cr,
		cr_val_sel_mode);
}
EXPORT_SYMBOL(mtk_fe_set_params);


/** @ingroup IP_group_fe_external_function
 * @par Description
 *     Set the region of fe. It includes input region, and merge mode.\n
 *         For a input image with the resolution of (w, h),\n
 *         FE hw will deal with feo region depends on merge_mode.\n
 *         And FE do feature extraction without cropping,\n
 *         so the total feo region for feature extraction\n
 *         is equal to input(w, h).\n
 * @param[in]
 *     dev: pointer of fe device structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @param[in]
 *     w: input width of fe, range 160 to 2560.
 * @param[in]
 *     h: input height of fe, range 160 to 640.
 * @param[in]
 *     merge_mode: define input merge mode(1, 2, 4 merged frame input),\n
 *         #mtk_fe_frame_merge_mode
 * @return
 *     0, successfully set region.\n
 *     -EINVAL, invalid parameter.\n
 * @par Boundary case and Limitation
 *     w should be in the range of 160 to 2560.
 *     h should be in the range of 160 to 640.
 *     merge_mode should be #mtk_fe_frame_merge_mode.
 * @par Error case and Error handling
 *     If fe dev is NULL, return -EINVAL.
 *     If input parameter is invalid, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fe_set_region(const struct device *dev, struct cmdq_pkt *handle,
		      u32 w, u32 h, enum mtk_fe_frame_merge_mode merge_mode)
{
	struct mtk_fe *fe;
	struct reg_trg trg;

	if (!dev)
		return -EINVAL;

	fe = dev_get_drvdata(dev);
	trg.regs = fe->regs;
	trg.reg_base = fe->reg_base;
	trg.pkt = handle;
	trg.subsys = fe->subsys;

	if (w < 160 || w > 2560) {
		dev_err(dev, "mtk_fe_set_region: error w %d\n", w);
		return -EINVAL;
	}

	if (h < 160 || h > 640) {
		dev_err(dev, "mtk_fe_set_region: error h %d\n", h);
		return -EINVAL;
	}

	if (merge_mode >= MTK_FE_MERGE_MAX) {
		dev_err(dev, "mtk_fe_set_region: error merge mode %d\n",
			merge_mode);
	}

	return mtk_fe_crop(&trg, w, h, 0, 0, w, h, fe->blk_sz, merge_mode);
}
EXPORT_SYMBOL(mtk_fe_set_region);

/** @ingroup IP_group_fe_external_function
 * @par Description
 *     Set address and pitch of multiple output buffer. The output buffer\n
 *         could be point buffer or descriptor buffer.
 * @param[in]
 *     dev: pointer of fe device structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @param[in]
 *     idx: the index of multiple output buffer, range 0 to 3.
 * @param[in]
 *     type: the type of multiple output buffer, #mtk_fe_out_buf
 * @param[in]
 *     addr: the address of this output buffer. It is physical address\n
 *         without iommu. It is iova with iommu. buffer should be allocated\n
 *         by kernel dma function, so that address boundary is cared,\n
 *         and the buffer address need to be 128-aligned for fm to input\n
 * @param[in]
 *     pitch: the pitch of this output buffer. The pitch means the distance\n
 *         (in bytes) between the first pixel of two consecutive line.
 * @return
 *     0, successfully set fe output buffer.
 *     -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     1. Call this function in stop-state or idle-state.\n
 *     2. type should be in #mtk_fe_out_buf.\n
 *     3. idx should be output index of feo.
 * @par Error case and Error handling
 *     If fe dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fe_set_multi_out_buf(const struct device *dev, struct cmdq_pkt *handle,
			     u32 idx, enum mtk_fe_out_buf type, dma_addr_t addr,
			     u32 pitch)
{
	int ret = 0;
	struct mtk_fe *fe;
	struct reg_trg trg;

	if (!dev)
		return -EINVAL;

	fe = dev_get_drvdata(dev);
	if (!fe)
		return -EINVAL;

	if (addr & 0x7f) {
		dev_err(dev, "mtk_fe_set_multi_out_buf: invalid addr %pad\n",
			&addr);
		return -EINVAL;
	}

	trg.regs = fe->regs;
	trg.reg_base = fe->reg_base;
	trg.pkt = handle;
	trg.subsys = fe->subsys;

	switch (type) {
	case MTK_FE_OUT_BUF_POINT:
		ret = mtk_fe_out_point_buf(&trg, idx, addr);
		break;
	case MTK_FE_OUT_BUF_DESCRIPTOR:
		ret = mtk_fe_out_descriptor_buf(&trg, idx, addr);
		break;
	default:
		dev_err(dev, "mtk_fe_set_multi_out_buf: unsupported type (%d).\n",
			idx);
		ret = -EINVAL;
		break;
	}

	return ret;
}
EXPORT_SYMBOL(mtk_fe_set_multi_out_buf);

/** @ingroup IP_group_fe_external_function
 * @par Description
 *     write value to specific fe registers.
 * @param[in]
 *     dev: pointer of fe device structure.
 * @param[in]
 *     offset: base address offset
 * @param[in]
 *     value: write value, unsigned int.
 * @return
 *     0, successfully write register.
 *     -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     1. offset up to 0x1000.
 *     2. only for debug use.
 * @par Error case and Error handling
 *     If fe dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fe_write_register(const struct device *dev,
	u32 offset, u32 value)
{
	struct mtk_fe *fe;
	struct reg_trg trg;

	if (!dev)
		return -EINVAL;

	fe = dev_get_drvdata(dev);
	if (!fe)
		return -EINVAL;
	if (offset > FE_REG_MAX_OFFSET)
		return -EINVAL;

	trg.regs = fe->regs;
	trg.reg_base = fe->reg_base;
	trg.pkt = NULL;
	trg.subsys = fe->subsys;

	return reg_write(&trg, offset, value);
}
EXPORT_SYMBOL(mtk_fe_write_register);

/** @ingroup IP_group_fe_external_function
 * @par Description
 *     read value from specific fe registers.
 * @param[in]
 *     dev: pointer of fe device structure.
 * @param[in]
 *     offset: base address offset
 * @param[out]
 *     value: read value, unsigned int.
 * @return
 *     0, successfully write register.
 *     -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     1. offset up to 0x1000.
 *     2. only for debug use.
 * @par Error case and Error handling
 *     If fe dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fe_read_register(const struct device *dev, u32 offset, u32 *value)
{
	struct mtk_fe *fe;

	if (!dev)
		return -EINVAL;

	fe = dev_get_drvdata(dev);
	if (!fe || !value)
		return -EINVAL;
	if (offset > FE_REG_MAX_OFFSET)
		return -EINVAL;

	*value = readl(fe->regs + offset);

	return 0;
}
EXPORT_SYMBOL(mtk_fe_read_register);


/** @ingroup IP_group_fe_external_function
 * @par Description
 *     Turn on the power of FE. Before any configuration, you should
 *         turn on the power of FE.
 * @param[in]
 *     dev: pointer of fe device structure.
 * @return
 *    0, successfully append command into the cmdq command buffer.
 *    -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If fe dev is NULL, return -EINVAL
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fe_power_on(struct device *dev)
{
	struct mtk_fe *fe;
	int ret = 0;

	if (WARN_ON(!dev))
		return -EINVAL;

	fe = dev_get_drvdata(dev);
	if (!fe)
		return -EINVAL;
	if (fe->valid_func & LARB_IOMMU) {
		ret = mtk_smi_larb_get(fe->dev_larb);
		if (ret < 0) {
			dev_err(dev, "Failed to get larb: %d\n", ret);
			goto err_return;
		}
	}

	if (fe->valid_func & CLK) {
#ifdef CONFIG_COMMON_CLK_MT3612
		ret = pm_runtime_get_sync(dev);
		if (ret < 0) {
			dev_err(dev,
				"Failed to enable power domain: %d\n", ret);
			goto err_larb;
		}

		ret = clk_prepare_enable(fe->clk);
		if (ret) {
			dev_err(dev, "Failed to enable clk:%d\n", ret);
			goto err_pm;
		}
#endif
	}

	return 0;

#ifdef CONFIG_COMMON_CLK_MT3612
err_pm:
	pm_runtime_put(dev);
err_larb:
	mtk_smi_larb_put(fe->dev_larb);
#endif
err_return:
	return ret;
}
EXPORT_SYMBOL(mtk_fe_power_on);

/** @ingroup IP_group_fe_external_function
 * @par Description
 *     Turn off the power of FE.
 * @param[in]
 *     dev: pointer of fe device structure.
 * @return
 *     0, successfully append command into the cmdq command buffer.\n
 *     -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If fe dev is NULL, return -EINVAL
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fe_power_off(struct device *dev)
{
	struct mtk_fe *fe;

	if (WARN_ON(!dev))
		return -EINVAL;

	fe = dev_get_drvdata(dev);
	if (!fe)
		return -EINVAL;
	if (fe->valid_func & CLK) {
#ifdef CONFIG_COMMON_CLK_MT3612
		clk_disable_unprepare(fe->clk);
		pm_runtime_put(dev);
#endif
	}
	if (fe->valid_func & LARB_IOMMU)
		mtk_smi_larb_put(fe->dev_larb);

	return 0;
}
EXPORT_SYMBOL(mtk_fe_power_off);

/** @ingroup IP_group_fe_external_function
 * @par Description
 *     Query whether fe is in done status.
 * @param[in]
 *     dev: pointer of fe device structure.
 * @return
 *     true, FE is in done status.
 *     false, FE is in active status.
 * @par Boundary case and Limitation
 *     Call this function in active-state or idle-state.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
bool mtk_fe_get_fe_done(const struct device *dev)
{
	struct mtk_fe *fe;

	if (!dev)
		return -EINVAL;

	fe = dev_get_drvdata(dev);

	return mtk_fe_is_fe_done(fe->regs);
}
EXPORT_SYMBOL(mtk_fe_get_fe_done);

/** @ingroup IP_group_fe_external_function
 * @par Description
 *     Set block size.
 * @param[in]
 *     dev: pointer of fe device structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration
 *         to register directly. Otherwise, write the configuration to
 *         command queue packet.
 * @param[in]
 *     blk_sz: block size enum(8x8, 16x16, 32x32), #mtk_fe_block_size
 * @return
 *     0, set block size success.
 *     -EINVAL, invalid dev.
 * @par Boundary case and Limitation
 *     1. Call this function in stop-state or idle-state.
 *     2. Block size should be #mtk_fe_block_size
 * @par Error case and Error handling
 *     If fe dev is NULL, return -EINVAL.
 *     If blk_sz is invalid, set blk_sz to 32x32.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fe_set_block_size(const struct device *dev, struct cmdq_pkt *handle,
			  enum mtk_fe_block_size blk_sz)
{
	struct mtk_fe *fe;
	struct reg_trg trg;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	fe = dev_get_drvdata(dev);
	if (!fe)
		return -EINVAL;

	trg.regs = fe->regs;
	trg.reg_base = fe->reg_base;
	trg.pkt = handle;
	trg.subsys = fe->subsys;

	switch (blk_sz) {
	case MTK_FE_BLK_SZ_32x32:
		ret = reg_write_mask(&trg, FE_CTRL1, MTK_FE_BLK_SZ_32x32,
				     FE_MODE);
		fe->blk_sz = 32;
		break;
	case MTK_FE_BLK_SZ_16x16:
		ret = reg_write_mask(&trg, FE_CTRL1, MTK_FE_BLK_SZ_16x16,
				     FE_MODE);
		fe->blk_sz = 16;
		break;
	case MTK_FE_BLK_SZ_8x8:
		ret = reg_write_mask(&trg, FE_CTRL1, MTK_FE_BLK_SZ_8x8,
				     FE_MODE);
		fe->blk_sz = 8;
		break;
	default:
		dev_err(dev,
			"not support block size %d\n, set to 32x32\n", blk_sz);
		ret = reg_write_mask(&trg, FE_CTRL1, MTK_FE_BLK_SZ_32x32,
				     FE_MODE);
		fe->blk_sz = 32;
		break;
	}
	return ret;
}
EXPORT_SYMBOL(mtk_fe_set_block_size);

static const enum dev_stage fpga = FPGA;
static const enum dev_stage a0 = A0;

static const struct of_device_id fe_driver_dt_match[] = {
	{ .compatible = "mediatek,mt3612-fe-fpga", .data = (void *)&fpga},
	{ .compatible = "mediatek,mt3612-fe", .data = (void *)&a0},
	{},
};

MODULE_DEVICE_TABLE(of, fe_driver_dt_match);

#ifdef CONFIG_MTK_DEBUGFS
static int fe_debug_show(struct seq_file *s, void *unused)
{
	struct mtk_fe *fe = s->private;
	u32 reg;
	u32 val;
	u32 blk_sz = 0;
	u32 frame_num = 0;

	reg = readl(fe->regs + FE_CTRL1);
	seq_puts(s, "--------------------- FE DEUBG ---------------------\n");
	seq_printf(s, "FE TH_G:                 :0x%.8x\n",
		   (u32)(reg & (FE_TH_G)) >> 10);
	seq_printf(s, "FE TH_C:                 :0x%.8x\n",
		   (u32)(reg & (FE_TH_C)) >> 18);
	seq_printf(s, "FE KAPPA:                :0x%.8x\n",
		   (u32)(reg & (FE_PARAM)) >> 2);
	seq_printf(s, "FE FLAT_EN:              :%10x\n",
		   (u32)(reg & (FE_FLT_EN)) >> 9);

	val = (u32)(reg & (FE_MODE));
	switch (val) {
	case MTK_FE_BLK_SZ_32x32:
		blk_sz = 32;
		break;
	case MTK_FE_BLK_SZ_16x16:
		blk_sz = 16;
		break;
	case MTK_FE_BLK_SZ_8x8:
		blk_sz = 8;
		break;
	default:
		pr_err("FE_BLK_SIZE error: %d\n", val);
		break;
	}
	seq_printf(s, "FE BLK_SIZE:             :%10x\n", blk_sz);

	reg = readl(fe->regs + FE_CROP_CTRL1);
	seq_printf(s, "FE CROP_START_X:         :0x%.8x\n",
		   (u32)(reg & (FE_START_X)));
	seq_printf(s, "FE CROP_START_Y:         :0x%.8x\n",
		   (u32)(reg & (FE_START_Y)) >> 16);

	reg = readl(fe->regs + FE_CROP_CTRL2);
	seq_printf(s, "FE CROP_WIDTH:           :0x%.8x\n",
		   (u32)(reg & (FE_CROP_WD)));
	seq_printf(s, "FE CROP_HEIGHT:          :0x%.8x\n",
		   (u32)(reg & (FE_CROP_HT)) >> 16);

	reg = readl(fe->regs + FE_CTRL2);
	val = (u32)(reg & (FE_MERGE_MODE)) >> 7;
	switch (val) {
	case MTK_FE_MERGE_SINGLE_FRAME:
		frame_num = 1;
		break;
	case MTK_FE_MERGE_DOUBLE_FRAME:
		frame_num = 2;
		break;
	case MTK_FE_MERGE_QUAD_FRAME:
		frame_num = 4;
		break;
	default:
		pr_err("FE_MERGE_MODE error: %d\n", val);
		break;
	}
	seq_printf(s, "FE MERGE_MODE:           :%4x_frame\n", frame_num);
	seq_printf(s, "FE ENABLE:               :%10x\n",
		   (u32)(reg & (FE_EN)) >> 5);

	val = (u32)(reg & (FE_CR_VALUE_SEL)) >> 6;
	if (val)
		seq_puts(s, "FE CR_SEL_MODE:          : ABS of CR\n");
	else
		seq_puts(s, "FE CR_SEL_MODE:          :    NORMAL\n");

	seq_printf(s, "FE DESC_EN:              :%10x\n",
		   (u32)(reg & (FE_DESCR_EN)) >> 4);

	reg = readl(fe->regs + FE_SIZE);
	seq_printf(s, "FE IN_WIDTH:             :0x%.8x\n",
		   (u32)(reg & (FE_IN_WD)));
	seq_printf(s, "FE IN_HEIGHT:            :0x%.8x\n",
		   (u32)(reg & (FE_IN_HT)) >> 16);

	reg = readl(fe->regs + FE_RST);
	seq_printf(s, "FE FE_DONE:              :%10x\n",
		   (u32)(reg & (FEO_FE_DONE)) >> 8);
	seq_printf(s, "FE INCOMP_ERR_STATUS:    :%10x\n",
		   (u32)(reg & (FE_ERR_ST)) >> 9);
	seq_printf(s, "FE DMA_STATUS:           :%10x\n",
		   (u32)(reg & (DMA_STOP_STATUS)) >> 1);
	seq_printf(s, "FE REQ:                  :%10x\n",
		   (u32)(reg & (FE_REQ_ST)) >> 11);
	seq_printf(s, "FE RDY:                  :%10x\n",
		   (u32)(reg & (FE_RDY_ST)) >> 10);

	reg = readl(fe->regs + FE_DBG);
	seq_printf(s, "FE input line num:       :0x%.8x\n",
		   (u32)(reg & (IN_FE_Y_CNT)));
	seq_printf(s, "FE input pixel num:      :0x%.8x\n",
		   (u32)(reg & (IN_FE_X_CONT)) >> 16);

	reg = readl(fe->regs + FE_WDMA_CTRL);
	seq_printf(s, "FE WDMA_EN:              :%10x\n",
		   (u32)(reg & (FE_WDMA_EN)));

	reg = readl(fe->regs + FE_WDMA_BASE_ADDR0_CFG);
	seq_printf(s, "FE IMG0_FE_POINT_BASE:   :0x%.8x\n", reg);

	reg = readl(fe->regs + FE_WDMA_BASE_ADDR4_CFG);
	seq_printf(s, "FE IMG0_FE_DESC_BASE:    :0x%.8x\n", reg);

	if (frame_num > 1) {
		reg = readl(fe->regs + FE_WDMA_BASE_ADDR1_CFG);
		seq_printf(s, "FE IMG1_FE_POINT_BASE:   :0x%.8x\n", reg);

		reg = readl(fe->regs + FE_WDMA_BASE_ADDR5_CFG);
		seq_printf(s, "FE IMG1_FE_DESC_BASE:    :0x%.8x\n", reg);
	}

	if (frame_num > 2) {
		reg = readl(fe->regs + FE_WDMA_BASE_ADDR2_CFG);
		seq_printf(s, "FE IMG2_FE_POINT_BASE:   :0x%.8x\n", reg);

		reg = readl(fe->regs + FE_WDMA_BASE_ADDR6_CFG);
		seq_printf(s, "FE IMG2_FE_DESC_BASE:    :0x%.8x\n", reg);

		reg = readl(fe->regs + FE_WDMA_BASE_ADDR3_CFG);
		seq_printf(s, "FE IMG3_FE_POINT_BASE:   :0x%.8x\n", reg);

		reg = readl(fe->regs + FE_WDMA_BASE_ADDR7_CFG);
		seq_printf(s, "FE IMG3_FE_DESC_BASE:    :0x%.8x\n", reg);
	}

	seq_puts(s, "----------------------------------------------------\n");
	return 0;
}

static int debug_client_open(struct inode *inode, struct file *file)
{
	return single_open(file, fe_debug_show, inode->i_private);
}

static const struct file_operations fe_debug_fops = {
	.open = debug_client_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif


static int mtk_fe_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_fe *fe;
	struct resource *regs;
	struct device_node *larb_node;
	struct platform_device *larb_pdev;
	const void *ptr;
	const struct of_device_id *of_id;
	u32 stage;
#ifdef CONFIG_MTK_DEBUGFS
	struct dentry *debug_dentry = NULL;
#endif
	int ret;

	dev_dbg(dev, "start fe probe!!\n");

	fe = devm_kzalloc(dev, sizeof(*fe), GFP_KERNEL);
	if (!fe)
		return -ENOMEM;

	of_id = of_match_device(fe_driver_dt_match, &pdev->dev);
	if (!of_id)
		return -ENODEV;

	stage = *(enum dev_stage *)of_id->data;
	dev_dbg(dev, "dt match fe dev_stage: %d\n",
		stage);

	if (stage != FPGA) {
#ifdef CONFIG_COMMON_CLK_MT3612
		fe->clk = devm_clk_get(dev, "mmsys_cmm_mdp_fe");
		if (IS_ERR(fe->clk)) {
			dev_err(dev, "Failed to get clk\n");
			return PTR_ERR(fe->clk);
		}
		fe->valid_func = LARB_IOMMU | CLK;
#endif
	} else {
		fe->valid_func = LARB_IOMMU;
	}

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	fe->regs = devm_ioremap_resource(dev, regs);
	if (IS_ERR(fe->regs)) {
		dev_err(dev, "Failed to map fe registers\n");
		return PTR_ERR(fe->regs);
	}
	fe->reg_base = (u32)regs->start;

	fe->blk_sz = 32;

	ptr = of_get_property(dev->of_node, "gce-subsys", NULL);
	if (ptr)
		fe->subsys = be32_to_cpup(ptr);
	else
		fe->subsys = 0;

	of_node_put(dev->of_node);
	if (fe->valid_func & LARB_IOMMU) {
		larb_node = of_parse_phandle(dev->of_node,
			"mediatek,larbs", 0);
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
		fe->dev_larb = &larb_pdev->dev;
		ret = mtk_smi_larb_get(fe->dev_larb);
		if (ret < 0)
			dev_err(dev, "Failed to get larb: %d\n", ret);

		pr_debug("fe smi larb get\n");
	}
	if (fe->valid_func & CLK) {
#ifdef CONFIG_COMMON_CLK_MT3612
		pm_runtime_enable(dev);
#endif
	}


	platform_set_drvdata(pdev, fe);
#ifdef CONFIG_MTK_DEBUGFS
	if (!mtk_debugfs_root)
		goto debugfs_done;

	/* debug info create */
	if (!mtk_fe_debugfs_root)
		mtk_fe_debugfs_root = debugfs_create_dir("fe",
							 mtk_debugfs_root);

	if (!mtk_fe_debugfs_root) {
		dev_dbg(dev, "failed to create fe debugfs root directory.\n");
		goto debugfs_done;
	}

	debug_dentry = debugfs_create_file(pdev->name, 0664,
					   mtk_fe_debugfs_root,
					   fe, &fe_debug_fops);

debugfs_done:
	if (!debug_dentry)
		dev_warn(dev, "Failed to create fe debugfs %s\n", pdev->name);

#endif


	dev_dbg(dev, "fe probe done!!\n");
	return 0;
}

static int mtk_fe_remove(struct platform_device *pdev)
{
	struct mtk_fe *fe = platform_get_drvdata(pdev);

	if (fe->valid_func & CLK) {
#ifdef CONFIG_COMMON_CLK_MT3612
		pm_runtime_disable(&pdev->dev);
#endif
	}
	if (fe->valid_func & LARB_IOMMU)
		mtk_smi_larb_put(fe->dev_larb);

#ifdef CONFIG_MTK_DEBUGFS
	debugfs_remove_recursive(mtk_fe_debugfs_root);
	mtk_fe_debugfs_root = NULL;
#endif

	return 0;
}

struct platform_driver mtk_fe_driver = {
	.probe		= mtk_fe_probe,
	.remove		= mtk_fe_remove,
	.driver		= {
		.name	= "mediatek-fe",
		.owner	= THIS_MODULE,
		.of_match_table = fe_driver_dt_match,
	},
};

module_platform_driver(mtk_fe_driver);
MODULE_LICENSE("GPL");
