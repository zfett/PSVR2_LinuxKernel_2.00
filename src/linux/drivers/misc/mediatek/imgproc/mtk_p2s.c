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

#include <asm-generic/io.h>
#include <linux/clk.h>
#include <linux/compiler.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/soc/mediatek/mtk-cmdq.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <soc/mediatek/mtk_p2s.h>
#include "mtk_p2s_reg.h"

/**
 * @defgroup IP_group_p2s P2S
 *     P2s is the driver of P2S which is dedicated for mergin left image\n
 *     and right image to an side-by-side image. The interface includes\n
 *     parameter setting and control function. Parameter setting includes\n
 *     setting of region. Control function includes power on/off, start,\n
 *     stop, and reset.
 *
 *     @{
 *         @defgroup IP_group_p2s_external EXTERNAL
 *             The external API document for p2s.\n
 *
 *             @{
 *                 @defgroup IP_group_p2s_external_function 1.function
 *                     Exported function of p2s
 *                 @defgroup IP_group_p2s_external_struct 2.structure
 *                     none.
 *                 @defgroup IP_group_p2s_external_typedef 3.typedef
 *                     none.
 *                 @defgroup IP_group_p2s_external_enum 4.enumeration
 *                     none.
 *                 @defgroup IP_group_p2s_external_def 5.define
 *                     none.
 *             @}
 *
 *         @defgroup IP_group_p2s_internal INTERNAL
 *             The internal API document for p2s.\n
 *
 *             @{
 *                 @defgroup IP_group_p2s_internal_function 1.function
 *                     none.
 *                 @defgroup IP_group_p2s_internal_struct 2.structure
 *                     none.
 *                 @defgroup IP_group_p2s_internal_typedef 3.typedef
 *                     none.
 *                 @defgroup IP_group_p2s_internal_enum 4.enumeration
 *                     none.
 *                 @defgroup IP_group_p2s_internal_def 5.define
 *                     none.
 *             @}
 *     @}
 */

#ifdef CONFIG_MTK_DEBUGFS
struct dentry *mtk_p2s_debugfs_root;
#endif

#define CHECK_BIT(var, pos) ((var) & (1<<(pos)))
#define GET_BIT(var, pos) ((var>>pos) & 1)
#define GET_VAL(reg, mask) ((reg & mask) >> __builtin_ctz(mask))
#define SET_VAL(value, mask) (value << __builtin_ctz(mask))

struct reg_trg {
	void __iomem *regs;
	u32 reg_base;
	struct cmdq_pkt *pkt;
	int subsys;
};

static void reg_write_mask(struct reg_trg *trg, u32 offset, u32 value,
		u32 mask)
{
	if (trg->pkt) {
		cmdq_pkt_write_value(trg->pkt, trg->subsys,
				(trg->reg_base & 0xffff) | offset,
				value, mask);
	} else {
		u32 reg;

		reg = readl(trg->regs + offset);
		reg &= ~mask;
		reg |= value;
		writel(reg, trg->regs + offset);
	}
}

#ifdef CONFIG_MTK_DEBUGFS
static void reg_set_val(struct reg_trg *trg, u32 offset, u32 value, u32 mask)
{
	reg_write_mask(trg, offset, SET_VAL(value, mask), mask);
}

static void reg_get_val(struct reg_trg *trg, u32 offset, u32 *value, u32 mask)
{
	u32 reg;

	reg = readl(trg->regs + offset);
	*value = GET_VAL(reg, mask);
}
#endif

static void mtk_p2s_enable(struct reg_trg *trg)
{
	reg_write_mask(trg, CP2S_ENABLE, CP2S_EN,
		CP2S_EN);
}

static void mtk_p2s_disable(struct reg_trg *trg)
{
	reg_write_mask(trg, CP2S_ENABLE, 0,
		CP2S_EN);
}

static void mtk_p2s_rst(struct reg_trg *trg)
{
	u32 reg;

	reg = readl(trg->regs + CP2S_RESET);

	reg |= CP2S_RST;
	reg_write_mask(trg, CP2S_RESET, reg, CP2S_RST);

	reg &= ~CP2S_RST;
	reg_write_mask(trg, CP2S_RESET, reg, CP2S_RST);
}

static void mtk_p2s_pat_gen_enable(struct reg_trg *trg)
{
	reg_write_mask(trg, CP2S_PATG_EN, PATG_EN, PATG_EN);
}

static void mtk_p2s_eyes(struct reg_trg *trg, bool l_only)
{
	if (l_only) {
		reg_write_mask(trg, CP2S_CTRL_0, LEFT_EN,
				LEFT_EN);
		reg_write_mask(trg, CP2S_CTRL_1, 0,
				RIGHT_EN);
	} else {
		reg_write_mask(trg, CP2S_CTRL_0, LEFT_EN,
				LEFT_EN);
		reg_write_mask(trg, CP2S_CTRL_1, RIGHT_EN,
				RIGHT_EN);
	}
}

static void mtk_p2s_size(struct reg_trg *trg, u32 w, u32 h)
{
	reg_write_mask(trg, CP2S_CTRL_0, w, FWIDTH_0);
	reg_write_mask(trg, CP2S_CTRL_1, w, FWIDTH_1);
	reg_write_mask(trg, CP2S_CTRL_2, h, FHEIGHT);
}

struct mtk_p2s_data {
	bool l_only;
};

struct mtk_p2s_data mt3612_p2s_data = {
	.l_only = false,
};

struct mtk_p2s {
	struct device *dev;
	struct clk *clk;
	void __iomem *regs;
	u32 reg_base;
	u32 subsys;
	bool l_only;
	const struct mtk_p2s_data *data;
};

/** @ingroup IP_group_p2s_external_function
 * @par Description
 *     Set the left & right input w & h. The output will be\n
 *     merged left and right image.If relay mode output size will be\n
 *     only left or right image.
 * @param[in]
 *     dev: pointer of p2s device structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @param[in]
 *     w: input width of p2s.\n
 *     relay mode should be in the range of 1 to 1920.\n
 *     non-relay mode should be in the range of 1 to 960.
 * @param[in]
 *     h: input height of p2s.\n
 *     for both relay or non-relay mode should be in the range of 1 to 1080.
 * @return
 *     1. 0, successfully append command into the cmdq command buffer.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. -ENODEV, No such device.\n
 * @par Boundary case and Limitation
 *     w should be in the range of 1 to 1920. h should be in the range\n
 *         of 1 to 1080.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_p2s_set_region(const struct device *dev, struct cmdq_pkt *handle,
		u32 w, u32 h)
{
	struct mtk_p2s *p2s;
	struct reg_trg trg;

	if (!dev)
		return -EINVAL;

	p2s = dev_get_drvdata(dev);

	if (!p2s)
		return -ENODEV;

	trg.regs = p2s->regs;
	trg.reg_base = p2s->reg_base;
	trg.pkt = handle;
	trg.subsys = p2s->subsys;

	if (p2s->l_only && w > 1920)
		return -EINVAL;

	if (!p2s->l_only && w > 960)
		return -EINVAL;

	if (h > 2205)
		return -EINVAL;

	mtk_p2s_size(&trg, w, h);

	return 0;
}
EXPORT_SYMBOL(mtk_p2s_set_region);

/** @ingroup IP_group_p2s_external_function
 * @par Description
 *     Setup p2s pattern generator.
 * @param[in]
 *     dev: p2s device node.
 * @param[in]
 *     handle: Command queue handle pointer.
 * @param[in]
 *     cfg: pattern generator configuration data structure pointer.
 * @return
 *     1. 0, configuration is done.
 *     2. -EINVAL, invalid parameter.\n
 *     3. -ENODEV, No such device.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_p2s_patgen_config(const struct device *dev, struct cmdq_pkt *handle,
			  const struct p2s_pg_cfg *cfg)
{
	struct mtk_p2s *p2s;
	struct reg_trg trg;

	if (!dev)
		return -EINVAL;

	p2s = dev_get_drvdata(dev);

	if (!p2s)
		return -ENODEV;

	if (!cfg)
		return -EINVAL;

	trg.regs = p2s->regs;
	trg.reg_base = p2s->reg_base;
	trg.pkt = handle;
	trg.subsys = p2s->subsys;

	if (p2s->l_only && cfg->width > 1920)
		return -EINVAL;

	if (!p2s->l_only && cfg->width > 960)
		return -EINVAL;

	if (cfg->height > 2205)
		return -EINVAL;

	/* set pattern gen */
	reg_write_mask(&trg, CP2S_PATG_1, cfg->width, PATG_0_FWIDTH);
	reg_write_mask(&trg, CP2S_PATG_2, cfg->width, PATG_1_FWIDTH);
	reg_write_mask(&trg, CP2S_PATG_3, cfg->height, PATG_FHEIGHT);

	/* pattern settings */
	reg_write_mask(&trg, CP2S_PATG_4,
		       cfg->left.cbar_width, PATG_0_BAR_WIDTH);
	reg_write_mask(&trg, CP2S_PATG_4,
		       cfg->right.cbar_width << 16, PATG_1_BAR_WIDTH);

	reg_write_mask(&trg, CP2S_PATG_5,
		       cfg->left.color_r.init_value, PATG_0_DATA_R_INIT);
	reg_write_mask(&trg, CP2S_PATG_5,
		       cfg->left.color_r.addend << 16, PATG_0_DATA_R_ADDEND);

	reg_write_mask(&trg, CP2S_PATG_6,
		       cfg->left.color_g.init_value, PATG_0_DATA_G_INIT);
	reg_write_mask(&trg, CP2S_PATG_6,
		       cfg->left.color_g.addend << 16, PATG_0_DATA_G_ADDEND);

	reg_write_mask(&trg, CP2S_PATG_7,
		       cfg->left.color_b.init_value, PATG_0_DATA_B_INIT);
	reg_write_mask(&trg, CP2S_PATG_7,
		       cfg->left.color_b.addend << 16, PATG_0_DATA_B_ADDEND);

	reg_write_mask(&trg, CP2S_PATG_8,
		       cfg->right.color_r.init_value, PATG_1_DATA_R_INIT);
	reg_write_mask(&trg, CP2S_PATG_8,
		       cfg->right.color_r.addend << 16, PATG_1_DATA_R_ADDEND);

	reg_write_mask(&trg, CP2S_PATG_9,
		       cfg->right.color_g.init_value, PATG_1_DATA_G_INIT);
	reg_write_mask(&trg, CP2S_PATG_9,
		       cfg->right.color_g.addend << 16, PATG_1_DATA_G_ADDEND);

	reg_write_mask(&trg, CP2S_PATG_10,
		       cfg->right.color_b.init_value, PATG_1_DATA_B_INIT);
	reg_write_mask(&trg, CP2S_PATG_10,
		       cfg->right.color_b.addend << 16, PATG_1_DATA_B_ADDEND);

	return 0;
}
EXPORT_SYMBOL(mtk_p2s_patgen_config);

/** @ingroup IP_group_p2s_external_function
 * @par Description
 *     Enable p2s pattern generator.
 * @param[in]
 *     dev: p2s device node.
 * @param[in]
 *     handle: Command queue handle pointer.
 * @return
 *     1. 0, enable is done.
 *     2. -EINVAL, invalid parameter.\n
 *     3. -ENODEV, No such device.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_p2s_patgen_enable(const struct device *dev, struct cmdq_pkt *handle)
{
	struct mtk_p2s *p2s;
	struct reg_trg trg;

	if (!dev)
		return -EINVAL;

	p2s = dev_get_drvdata(dev);

	if (!p2s)
		return -ENODEV;

	trg.regs = p2s->regs;
	trg.reg_base = p2s->reg_base;
	trg.pkt = handle;
	trg.subsys = p2s->subsys;

	/* enable pattern gen */
	mtk_p2s_pat_gen_enable(&trg);
	return 0;
}
EXPORT_SYMBOL(mtk_p2s_patgen_enable);

/** @ingroup IP_group_p2s_external_function
 * @par Description
 *     Start the functon of P2S.
 * @param[in]
 *     dev: pointer of P2S device structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @return
 *     1. 0, successfully append command into the cmdq command buffer.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. -ENODEV, No such device.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_p2s_start(const struct device *dev, struct cmdq_pkt *handle)
{
	struct mtk_p2s *p2s;
	struct reg_trg trg;

	if (!dev)
		return -EINVAL;

	p2s = dev_get_drvdata(dev);

	if (!p2s)
		return -ENODEV;

	trg.regs = p2s->regs;
	trg.reg_base = p2s->reg_base;
	trg.pkt = handle;
	trg.subsys = p2s->subsys;

	mtk_p2s_eyes(&trg, p2s->l_only);
	mtk_p2s_enable(&trg);

	return 0;
}
EXPORT_SYMBOL(mtk_p2s_start);

/** @ingroup IP_group_p2s_external_function
 * @par Description
 *     Stop the functon of P2S.
 * @param[in]
 *     dev: pointer of p2s device structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @return
 *     1. 0, successfully append command into the cmdq command buffer.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. -ENODEV, No such device.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_p2s_stop(const struct device *dev, struct cmdq_pkt *handle)
{
	struct mtk_p2s *p2s;
	struct reg_trg trg;

	if (!dev)
		return -EINVAL;

	p2s = dev_get_drvdata(dev);

	if (!p2s)
		return -ENODEV;

	trg.regs = p2s->regs;
	trg.reg_base = p2s->reg_base;
	trg.pkt = handle;
	trg.subsys = p2s->subsys;

	mtk_p2s_disable(&trg);

	return 0;
}
EXPORT_SYMBOL(mtk_p2s_stop);

/** @ingroup IP_group_p2s_external_function
 * @par Description
 *     Reset the functon of P2S.
 * @param[in]
 *     dev: pointer of crop device structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @return
 *     1. 0, successfully append command into the cmdq command buffer.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. -ENODEV, No such device.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_p2s_reset(const struct device *dev, struct cmdq_pkt *handle)
{
	struct mtk_p2s *p2s;
	struct reg_trg trg;

	if (!dev)
		return -EINVAL;

	p2s = dev_get_drvdata(dev);

	if (!p2s)
		return -ENODEV;

	trg.regs = p2s->regs;
	trg.reg_base = p2s->reg_base;
	trg.pkt = handle;
	trg.subsys = p2s->subsys;

	mtk_p2s_rst(&trg);

	return 0;
}
EXPORT_SYMBOL(mtk_p2s_reset);

static int mtk_p2s_clock_on(struct device *dev)
{
#if defined(CONFIG_COMMON_CLK_MT3612)
	struct mtk_p2s *p2s;
	int ret;

	if (!dev)
		return -EINVAL;

	p2s = dev_get_drvdata(dev);

	if (!p2s)
		return -ENODEV;

	ret = clk_prepare_enable(p2s->clk);
	if (ret) {
		dev_err(dev, "Failed to enable clock: ret %d\n", ret);
		goto err;
	}
	return 0;
err:
	clk_disable_unprepare(p2s->clk);

	return ret;
#else
	return 0;
#endif
}

/** @ingroup IP_group_p2s_external_function
 * @par Description
 *     Turn on the power of P2S. Before any configuration, you should\n
 *         turn on the power of P2S.
 * @param[in]
 *     dev: pointer of p2s device structure.
 * @return
 *     1. 0, successfully append command into the cmdq command buffer.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. -ENODEV, No such device.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_p2s_power_on(struct device *dev)
{
#if CONFIG_PM
	struct mtk_p2s *p2s;
	int ret;

	if (!dev)
		return -EINVAL;

	p2s = dev_get_drvdata(dev);

	if (!p2s)
		return -ENODEV;

	ret = pm_runtime_get_sync(dev);
	if (ret < 0) {
		dev_err(dev, "Failed to enable power domain: %d\n", ret);
		return ret;
	}

	ret = mtk_p2s_clock_on(dev);
	if (ret) {
		pm_runtime_put(dev);
		return ret;
	}
#endif
	return 0;
}
EXPORT_SYMBOL(mtk_p2s_power_on);

static int mtk_p2s_clock_off(struct device *dev)
{
#if defined(CONFIG_COMMON_CLK_MT3612)
	struct mtk_p2s *p2s;

	if (!dev)
		return -EINVAL;

	p2s = dev_get_drvdata(dev);

	if (!p2s)
		return -ENODEV;

	clk_disable_unprepare(p2s->clk);
#endif
	return 0;
}

/** @ingroup IP_group_p2s_external_function
 * @par Description
 *     Turn off the power of P2S.
 * @param[in]
 *     dev: pointer of p2s device structure.
 * @return
 *     1. 0, successfully append command into the cmdq command buffer.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. -ENODEV, No such device.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_p2s_power_off(struct device *dev)
{
#if CONFIG_PM
	struct mtk_p2s *p2s;
	int ret;

	if (!dev)
		return -EINVAL;

	p2s = dev_get_drvdata(dev);

	if (!p2s)
		return -ENODEV;

	ret = pm_runtime_put_sync(dev);
	if (ret < 0) {
		dev_err(dev, "Failed to put sync of power domain: %d\n", ret);
		return ret;
	}

	ret = mtk_p2s_clock_off(dev);

	if (!ret) {
		pm_runtime_put(dev);
		return ret;
	}
#endif
	return 0;
}
EXPORT_SYMBOL(mtk_p2s_power_off);

/** @ingroup IP_group_p2s_external_function
 * @par Description
 *     Set proccessing mode. Process left eye only or both eye.
 * @param[in]
 *     dev: pointer of p2s device structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @param[in]
 *     proc_mode: MTK_P2S_PROC_MODE_L_ONLY for left eye only,\n
 *         MTK_P2S_PROC_MODE_LR for both eye.
 * @return
 *     1. 0, successfully append command into the cmdq command buffer.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. -ENODEV, No such device.\n
 * @par Boundary case and Limitation
 *     1. Call this function in stop-state or idle-state.
 *     2. proc_mode should be MTK_P2S_PROC_MODE_L_ONLY or\n
 *         MTK_P2S_PROC_MODE_LR.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_p2s_set_proc_mode(const struct device *dev, struct cmdq_pkt *handle,
		u32 proc_mode)
{
	struct mtk_p2s *p2s;

	if (!dev)
		return -EINVAL;

	p2s = dev_get_drvdata(dev);

	if (!p2s)
		return -ENODEV;

	switch (proc_mode) {
	case MTK_P2S_PROC_MODE_LR:
		p2s->l_only = false;
		break;
	case MTK_P2S_PROC_MODE_L_ONLY:
		p2s->l_only = true;
		break;
	default:
		dev_err(dev, "mtk_p2s_set_proc_mode invalid proc_mode %d\n",
			proc_mode);
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(mtk_p2s_set_proc_mode);

#ifdef CONFIG_MTK_DEBUGFS
static int p2s_debug_show(struct seq_file *s, void *unused)
{
	struct mtk_p2s *p2s = s->private;
	u32 reg;

	reg = readl(p2s->regs + CP2S_ENABLE);

	seq_puts(s, "-------------------- P2S DEUBG --------------------\n");

	seq_printf(s, "ENABLE       :%8ld\n", GET_VAL(reg, CP2S_EN));

	reg = readl(p2s->regs + CP2S_CTRL_0);

	seq_printf(s, "LEFT WIDTH   :%8ld\n", GET_VAL(reg, FWIDTH_0));
	seq_printf(s, "LEFT ENABLE  :%8ld\n", GET_VAL(reg, LEFT_EN));

	reg = readl(p2s->regs + CP2S_CTRL_1);

	seq_printf(s, "RIGHT WIDTH  :%8ld\n", GET_VAL(reg, FWIDTH_1));
	seq_printf(s, "RIGHT ENABLE :%8ld\n", GET_VAL(reg, RIGHT_EN));

	reg = readl(p2s->regs + CP2S_CTRL_2);

	seq_printf(s, "HEIGHT       :%8ld\n", GET_VAL(reg, FHEIGHT));
	seq_printf(s, "SRAM SHARE   :%8ld\n", GET_VAL(reg, SRAM_SHARE));

	reg = readl(p2s->regs + CP2S_DBG_0);

	seq_printf(s, "IN 0 WIDTH   :%8ld\n", GET_VAL(reg, DBG_IN_0_H_SIZE));
	seq_printf(s, "IN 0 HEIGHT  :%8ld\n", GET_VAL(reg, DBG_IN_0_V_SIZE));

	reg = readl(p2s->regs + CP2S_DBG_1);

	seq_printf(s, "IN 0 X CNT   :%8ld\n", GET_VAL(reg, DBG_IN_0_H_COUNT));
	seq_printf(s, "IN 0 Y CNT   :%8ld\n", GET_VAL(reg, DBG_IN_0_V_COUNT));

	reg = readl(p2s->regs + CP2S_DBG_4);

	seq_printf(s, "IN 1 WIDTH   :%8ld\n", GET_VAL(reg, DBG_IN_0_H_SIZE));
	seq_printf(s, "IN 1 HEIGHT  :%8ld\n", GET_VAL(reg, DBG_IN_0_V_SIZE));

	reg = readl(p2s->regs + CP2S_DBG_5);

	seq_printf(s, "IN 1 X CNT   :%8ld\n", GET_VAL(reg, DBG_IN_1_H_COUNT));
	seq_printf(s, "IN 1 Y CNT   :%8ld\n", GET_VAL(reg, DBG_IN_1_V_COUNT));

	reg = readl(p2s->regs + CP2S_DBG_8);

	seq_printf(s, "OUT WIDTH    :%8ld\n", GET_VAL(reg, DBG_OUT_H_SIZE));
	seq_printf(s, "OUT HEIGHT   :%8ld\n", GET_VAL(reg, DBG_OUT_V_SIZE));

	reg = readl(p2s->regs + CP2S_DBG_9);

	seq_printf(s, "OUT X CNT    :%8ld\n", GET_VAL(reg, DBG_OUT_H_COUNT));
	seq_printf(s, "OUT Y CNT    :%8ld\n", GET_VAL(reg, DBG_OUT_V_COUNT));

	seq_puts(s, "----------------------------------------------------\n");

	return 0;
}

static int mtk_p2s_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, p2s_debug_show, inode->i_private);
}

static ssize_t mtk_p2s_debugfs_write(struct file *file,
					   const char __user *ubuf,
					   size_t count, loff_t *ppos)
{
	char *buf = vmalloc(count+1);
	char *vbuf = buf;
	struct mtk_p2s *p2s;
	struct reg_trg trg;
	struct seq_file *seq_ptr;
	const char sp[] = " ";
	char *token;
	char op[2] = {0};
	u32 offset;
	u32 mask_msb;
	u32 mask_lsb;
	u32 value;

	if (!buf)
		goto out;

	if (copy_from_user(buf, ubuf, count))
		goto out;

	seq_ptr = (struct seq_file *)file->private_data;

	pr_err("[p2s][debugfs] %s\n", buf);
	buf[count] = '\0';

	p2s = (struct mtk_p2s *)seq_ptr->private;

	if (!p2s)
		goto out;

	trg.regs = p2s->regs;
	trg.reg_base = p2s->reg_base;
	trg.pkt = NULL;
	trg.subsys = p2s->subsys;

	token = strsep(&buf, sp);

	if (strncmp(token, "reg", 3) == 0) {
		token = strsep(&buf, sp);
		strncpy(op, token, 1);

		token = strsep(&buf, sp);
		if (strncmp(token, "0x", 2) != 0)
			goto out;
		if (kstrtouint(token+2, 16, &offset))
			goto out;
		token = strsep(&buf, sp);
		if (kstrtouint(token, 10, &mask_msb))
			goto out;
		token = strsep(&buf, sp);
		if (kstrtouint(token, 10, &mask_lsb))
			goto out;
		token = strsep(&buf, sp);
		if (strncmp(op, "w", 1) == 0)
			if (kstrtouint(token, 10, &value))
				goto out;

		if (strncmp(op, "w", 1) == 0) {
			reg_set_val(&trg, offset, value,
					GENMASK(mask_msb, mask_lsb));
		} else if (strncmp(op, "r", 1) == 0) {
			reg_get_val(&trg, offset, &value,
					GENMASK(mask_msb, mask_lsb));
		}
		pr_err("reg '%s' 0x%03x [%d:%d] 0x%x\n",
			op, offset, mask_msb, mask_lsb, value);
	} else if (strncmp(token, "test", 4) == 0) {
		token = strsep(&buf, sp);
		if (strncmp(token, "pat", 3) == 0) {
			token = strsep(&buf, sp);
			if (strncmp(token, "config", 6) == 0) {
				reg_set_val(&trg, 0x34, 1920, GENMASK(12, 0));
				reg_set_val(&trg, 0x38, 1920, GENMASK(12, 0));
				reg_set_val(&trg, 0x3C, 1080, GENMASK(12, 0));
				reg_set_val(&trg, 0x40, 128, GENMASK(12, 0));
				reg_set_val(&trg, 0x40, 128, GENMASK(28, 16));
				reg_set_val(&trg, 0x44, 16, GENMASK(9, 0));
				reg_set_val(&trg, 0x44, 64, GENMASK(25, 16));
				reg_set_val(&trg, 0x48, 16, GENMASK(9, 0));
				reg_set_val(&trg, 0x48, 64, GENMASK(25, 16));
				reg_set_val(&trg, 0x4C, 16, GENMASK(9, 0));
				reg_set_val(&trg, 0x4C, 64, GENMASK(25, 16));
				reg_set_val(&trg, 0x50, 16, GENMASK(9, 0));
				reg_set_val(&trg, 0x50, 64, GENMASK(25, 16));
				reg_set_val(&trg, 0x54, 16, GENMASK(9, 0));
				reg_set_val(&trg, 0x54, 64, GENMASK(25, 16));
				reg_set_val(&trg, 0x58, 16, GENMASK(9, 0));
				reg_set_val(&trg, 0x58, 64, GENMASK(25, 16));
				pr_err("test pat config\n");
			} else if (strncmp(token, "enable", 6) == 0) {
				reg_set_val(&trg, 0x20, 1, GENMASK(0, 0));
				pr_err("test pat enable\n");
			} else if (strncmp(token, "disable", 7) == 0) {
				reg_set_val(&trg, 0x20, 0, GENMASK(0, 0));
				pr_err("test pat disable\n");
			}
		}
	}

out:
	vfree(vbuf);
	return count;
}

static const struct file_operations p2s_debug_fops = {
	.open = mtk_p2s_debugfs_open,
	.read = seq_read,
	.write = mtk_p2s_debugfs_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

static int mtk_p2s_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_p2s *p2s;
	struct resource *regs;
	const void *ptr;
#ifdef CONFIG_MTK_DEBUGFS
	struct dentry *debug_dentry;
#endif

	p2s = devm_kzalloc(dev, sizeof(*p2s), GFP_KERNEL);
	if (!p2s)
		return -ENOMEM;

#if 0
	p2s->clk = of_clk_get(dev->of_node, 0);
	if (IS_ERR(p2s->clk)) {
		dev_err(dev, "Failed to get p2s clock\n");
		return PTR_ERR(p2s->clk);
	}
#elif defined(CONFIG_COMMON_CLK_MT3612)
	p2s->clk = devm_clk_get(dev, "mmsys_core_p2s0");
	if (IS_ERR(p2s->clk)) {
		dev_err(dev, "Failed to get clk\n");
		return PTR_ERR(p2s->clk);
	}
#endif

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	p2s->regs = devm_ioremap_resource(dev, regs);
	if (IS_ERR(p2s->regs)) {
		dev_err(dev, "Failed to map p2s registers\n");
		return PTR_ERR(p2s->regs);
	}
	p2s->reg_base = (u32)regs->start;

	ptr = of_get_property(dev->of_node, "gce-subsys", NULL);

	if (ptr)
		p2s->subsys = be32_to_cpup(ptr);
	else
		p2s->subsys = 0;

	of_node_put(dev->of_node);

	p2s->data = of_device_get_match_data(dev);

	p2s->l_only = p2s->data->l_only;

#if CONFIG_PM
	pm_runtime_enable(dev);
#endif

	platform_set_drvdata(pdev, p2s);

#ifdef CONFIG_MTK_DEBUGFS

	if (!mtk_debugfs_root)
		goto debugfs_done;

	/* debug info create */
	if (!mtk_p2s_debugfs_root)
		mtk_p2s_debugfs_root = debugfs_create_dir("p2s",
							  mtk_debugfs_root);

	if (!mtk_p2s_debugfs_root) {
		dev_dbg(dev, "failed to create p2s debugfs root directory.\n");
		goto debugfs_done;
	}

	debug_dentry = debugfs_create_file(pdev->name, 0664,
					   mtk_p2s_debugfs_root,
					   p2s, &p2s_debug_fops);
debugfs_done:
	if (!debug_dentry)
		dev_dbg(dev, "Failed to create p2s debugfs %s\n", pdev->name);

#endif

	dev_dbg(dev, "probe done...\n");
	return 0;
}

static int mtk_p2s_remove(struct platform_device *pdev)
{
#if CONFIG_PM
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
#endif

#ifdef CONFIG_MTK_DEBUGFS
	debugfs_remove_recursive(mtk_p2s_debugfs_root);
	mtk_p2s_debugfs_root = NULL;
#endif

	return 0;
}

static const struct of_device_id p2s_driver_dt_match[] = {
	{ .compatible = "mediatek,mt3612-p2s",
	  .data = &mt3612_p2s_data },
	{},
};
MODULE_DEVICE_TABLE(of, p2s_driver_dt_match);

struct platform_driver mtk_p2s_driver = {
	.probe		= mtk_p2s_probe,
	.remove		= mtk_p2s_remove,
	.driver		= {
		.name		= "mediatek-p2s",
		.owner		= THIS_MODULE,
		.of_match_table = p2s_driver_dt_match,
	},
};

module_platform_driver(mtk_p2s_driver);
MODULE_LICENSE("GPL");
