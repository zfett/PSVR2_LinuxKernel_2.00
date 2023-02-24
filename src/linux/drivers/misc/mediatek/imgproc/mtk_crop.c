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
 * @defgroup IP_group_crop CROP
 *     Crop is the driver of CROP which is dedicated for image cropping\n
 *     function. The interface includes parameter setting and control\n
 *     function. Parameter setting includes setting of region. Control\n
 *     function includes power on/off, start, stop, and reset.
 *
 *     @{
 *         @defgroup IP_group_crop_external EXTERNAL
 *             The external API document for crop.\n
 *
 *             @{
 *                 @defgroup IP_group_crop_external_function 1.function
 *                     Exported function of crop
 *                 @defgroup IP_group_crop_external_struct 2.structure
 *                     none.
 *                 @defgroup IP_group_crop_external_typedef 3.typedef
 *                     none.
 *                 @defgroup IP_group_crop_external_enum 4.enumeration
 *                     none.
 *                 @defgroup IP_group_crop_external_def 5.define
 *                     none.
 *             @}
 *
 *         @defgroup IP_group_crop_internal INTERNAL
 *             The internal API document for crop.\n
 *
 *             @{
 *                 @defgroup IP_group_crop_internal_function 1.function
 *                     none.
 *                 @defgroup IP_group_crop_internal_struct 2.structure
 *                     none.
 *                 @defgroup IP_group_crop_internal_typedef 3.typedef
 *                     none.
 *                 @defgroup IP_group_crop_internal_enum 4.enumeration
 *                     none.
 *                 @defgroup IP_group_crop_internal_def 5.define
 *                     none.
 *             @}
 *     @}
 */

#include <asm-generic/io.h>
#include <linux/clk.h>
#include <linux/compiler.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/soc/mediatek/mtk-cmdq.h>
#include <soc/mediatek/mtk_crop.h>
#include <soc/mediatek/mtk_drv_def.h>
#include <linux/debugfs.h>
#include "mtk_crop_reg.h"


#ifdef CONFIG_MTK_DEBUGFS
struct dentry *mtk_crop_debugfs_root;
#define CHECK_BIT(var, pos) ((var) & (1<<(pos)))

#endif

#define MAX_HW_NR		1

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

static void reg_write(struct reg_trg *trg, u32 offset, u32 value)
{
	if (trg->pkt)
		reg_write_mask(trg, offset, value, 0xffffffff);
	else
		writel(value, trg->regs + offset);
}

static void mtk_crop_enable(struct reg_trg *trg)
{
	reg_write(trg, DISP_CROP_EN, 1);
}

static void mtk_crop_disable(struct reg_trg *trg)
{
	reg_write(trg, DISP_CROP_EN, 0);
}

static void mtk_crop_rst(void __iomem *regs)
{
	writel(1, regs + DISP_CROP_RESET);
	writel(0, regs + DISP_CROP_RESET);
}

static void mtk_crop_region(struct reg_trg *trg, u32 in_w, u32 in_h,
		u32 crop_x, u32 crop_y, u32 out_w, u32 out_h)
{
	reg_write_mask(trg, DISP_CROP_DEBUG, BYPASS_SHADOW, BYPASS_SHADOW);
	reg_write(trg, DISP_CROP_SRC, in_w | (in_h << 16));
	reg_write(trg, DISP_CROP_OFFSET, crop_x | (crop_y << 16));
	reg_write(trg, DISP_CROP_ROI, out_w | (out_h << 16));
}

struct mtk_crop {
	struct device			*dev;
	struct clk			*clk[MAX_HW_NR];
	void __iomem			*regs[MAX_HW_NR];
	u32 reg_base[MAX_HW_NR];
	u32 subsys[MAX_HW_NR];
	u32 hw_nr;
};

/** @ingroup IP_group_crop_external_function
 * @par Description
 *     Start the functon of CROP.
 * @param[in]
 *     dev: pointer of crop device structure.
 * @param[in]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @return
 *     1. 0, success.\n
 *     2. -EINVAL, invalid argument.\n
 *     3. -ENODEV,No such device.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_crop_start(const struct device *dev, struct cmdq_pkt **handle)
{
	struct mtk_crop *crop;
	struct reg_trg trg;
	u32 hw_id = 0;

	if (!dev)
		return -EINVAL;

	crop = dev_get_drvdata(dev);
	if (!crop)
		return -ENODEV;

	trg.regs = crop->regs[hw_id];
	trg.reg_base = crop->reg_base[hw_id];
	trg.pkt = handle ? handle[hw_id] : NULL;
	trg.subsys = crop->subsys[hw_id];

	mtk_crop_enable(&trg);

	return 0;
}
EXPORT_SYMBOL(mtk_crop_start);

/** @ingroup IP_group_crop_external_function
 * @par Description
 *     Stop the functon of CROP.
 * @param[in]
 *     dev: pointer of crop device structure.
 * @param[in]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @return
 *     1. 0, success.\n
 *     2. -EINVAL, Invalid argument.\n
 *     3. -ENODEV,No such device.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_crop_stop(const struct device *dev, struct cmdq_pkt **handle)
{
	struct mtk_crop *crop;
	struct reg_trg trg;
	u32 hw_id = 0;

	if (!dev)
		return -EINVAL;

	crop = dev_get_drvdata(dev);
	if (!crop)
		return -ENODEV;


	trg.regs = crop->regs[hw_id];
	trg.reg_base = crop->reg_base[hw_id];
	trg.pkt = handle ? handle[hw_id] : NULL;
	trg.subsys = crop->subsys[hw_id];

	mtk_crop_disable(&trg);

	return 0;
}
EXPORT_SYMBOL(mtk_crop_stop);

/** @ingroup IP_group_crop_external_function
 * @par Description
 *     Reset the functon of CROP.
 * @param[in]
 *     dev: pointer of crop device structure.
 * @param[in]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @return
 *     1. 0, success.\n
 *     2. -EINVAL, Invalid argument.\n
 *     3. -ENODEV,No such device.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_crop_reset(const struct device *dev, struct cmdq_pkt **handle)
{
	struct mtk_crop *crop;
	u32 hw_id = 0;

	if (!dev)
		return -EINVAL;

	crop = dev_get_drvdata(dev);
	if (!crop)
		return -ENODEV;

	mtk_crop_rst(crop->regs[hw_id]);

	return 0;
}
EXPORT_SYMBOL(mtk_crop_reset);

/** @ingroup IP_group_crop_external_function
 * @par Description
 *     Set the region of crop. It includes input region, output region, and\n
 *         crop position. For a input image with the resolution of (w, h),\n
 *         to crop a rectange whose top-left position is (x1,y1) and\n
 *         bottom-right position is (x2, y2), the setting should be\n
 *         (in_w, in_h) = (w, h), (out_w, out_h) = (x2 - x1 + 1, y2 - y1 + 1)\n
 *         , and (crop_x, crop_y) = (x1, y1).
 * @param[in]
 *     dev: pointer of crop device structure.
 * @param[in]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @param[in]
 *     in_w: input width of crop, range 1 to 4000.
 * @param[in]
 *     in_h: input height of crop, range 1 to 2205 .
 * @param[in]
 *     out_w: output width of crop (cropped width), range 1 to 4000.
 * @param[in]
 *     out_h: output height of crop (cropped height), range 1 to 2205.
 * @param[in]
 *     crop_x: horizontal position of the top-left corner of the cropped\n
 *         region, range 1 to 3999.
 * @param[in]
 *     crop_y: verticall position of the top-left corner of the cropped\n
 *         region, range 1 to 2204.
 * @return
 *     1. 0, success.\n
 *     2. -EINVAL, Invalid argument.\n
 *     3. -ENODEV,No such device.\n
 * @par Boundary case and Limitation
 *     in_w should be in the range of 1 to 4000.\n
 *     in_h should be in the range of 1 to 2205.\n
 *     out_w should be in the range of 0 to 4000.\n
 *     out_h should be in the range of 0 to 2205.\n
 *     crop_x should be in the range of 0 to 3999.\n
 *     crop_y should be in the range of 0 to 2204.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_crop_set_region(const struct device *dev, struct cmdq_pkt **handle,
		u32 in_w, u32 in_h, u32 out_w, u32 out_h,
		u32 crop_x, u32 crop_y)
{
	struct mtk_crop *crop;
	struct reg_trg trg;
	u32 in_x1, in_x2, out_x1, out_x2;
	u32 hw_id = 0;

	if (!dev)
		return -EINVAL;

	if ((in_w < 1 || in_w > 4000) || (in_h < 1 || in_h > 2205))
		return -EINVAL;

	if ((out_w > 4000) || (out_h > 2205))
		return -EINVAL;

	if ((crop_x > 3999) || (crop_y > 2204))
		return -EINVAL;

	crop = dev_get_drvdata(dev);
	if (!crop)
		return -ENODEV;

	trg.regs = crop->regs[hw_id];
	trg.reg_base = crop->reg_base[hw_id];
	trg.pkt = handle ? handle[hw_id] : NULL;
	trg.subsys = crop->subsys[hw_id];

	in_x1 = (in_w) * 0;
	in_x2 = (in_w) * (1);
	out_x1 = crop_x;
	out_x2 = crop_x + out_w;
	out_x1 = min(out_x1, in_x2);
	out_x1 = max(out_x1, in_x1);
	out_x2 = min(out_x2, in_x2);
	out_x2 = max(out_x2, in_x1);
	mtk_crop_region(&trg, in_w, in_h, out_x1, crop_y,
					out_x2 - out_x1, out_h);

	return 0;
}
EXPORT_SYMBOL(mtk_crop_set_region);

static int mtk_crop_clock_on(struct device *dev, struct mtk_crop *crop)
{
#if defined(CONFIG_COMMON_CLK_MT3612)
	int i, ret;

	for (i = 0; i < crop->hw_nr; i++) {
		ret = clk_prepare_enable(crop->clk[i]);
		if (ret) {
			dev_err(dev, "Failed to enable clock hw %d: ret %d\n",
					i, ret);
			goto err;
		}
	}
	return 0;
err:
	while (--i >= 0)
		clk_disable_unprepare(crop->clk[i]);

	return ret;
#else
	return 0;
#endif
}

/** @ingroup IP_group_crop_external_function
 * @par Description
 *     Turn on the power of CROP. Before any configuration, you should\n
 *         turn on the power of CROP.
 * @param[in]
 *     dev: pointer of crop device structure.
 * @return
 *     1. 0, success.\n
 *     2. -EINVAL, Invalid argument.\n
 *     3. -ENODEV,No such device.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_crop_power_on(struct device *dev)
{
#if CONFIG_PM
	struct mtk_crop *crop;
	int ret;

	if (!dev)
		return -EINVAL;

	crop = dev_get_drvdata(dev);
	if (!crop)
		return -ENODEV;

	ret = pm_runtime_get_sync(dev);
	if (ret < 0) {
		dev_err(dev, "Failed to enable power domain: %d\n", ret);
		return ret;
	}

	ret = mtk_crop_clock_on(dev, crop);
	if (ret) {
		pm_runtime_put(dev);
		return ret;
	}
#endif
	return 0;

}
EXPORT_SYMBOL(mtk_crop_power_on);

static void mtk_crop_clock_off(struct mtk_crop *crop)
{
#if defined(CONFIG_COMMON_CLK_MT3612)
	int i;
	for (i = 0; i < crop->hw_nr; i++)
		clk_disable_unprepare(crop->clk[i]);
#endif
}

/** @ingroup IP_group_crop_external_function
 * @par Description
 *     Turn off the power of CROP.
 * @param[in]
 *     dev: pointer of crop device structure.
 * @return
 *     1. 0, success.\n
 *     2. -EINVAL, Invalid argument.\n
 *     3. -ENODEV,No such device.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_crop_power_off(struct device *dev)
{
#if CONFIG_PM
	struct mtk_crop *crop;

	if (!dev)
		return -EINVAL;

	crop = dev_get_drvdata(dev);
	if (!crop)
		return -ENODEV;

	mtk_crop_clock_off(crop);
	pm_runtime_put(dev);
#endif
	return 0;
}
EXPORT_SYMBOL(mtk_crop_power_off);

#ifdef CONFIG_MTK_DEBUGFS
static int crop_debug_show(struct seq_file *s, void *unused)
{
	struct mtk_crop *crop = s->private;
	u32 reg;

	reg = readl(crop->regs[0] + DISP_CROP_EN);

	seq_puts(s, "-------------------- CROP DEUBG --------------------\n");
	seq_printf(s, "ENABLE:         :%11x\n", CHECK_BIT(reg, 0) ? 1 : 0);

	reg = readl(crop->regs[0] + DISP_CROP_RESET);
	seq_printf(s, "CROP_RESET      :%11x\n", CHECK_BIT(reg, 0) ? 1 : 0);

	reg = readl(crop->regs[0] + DISP_CROP_ROI);
	seq_printf(s, "ROI_W           :%11d\n", (reg & 0x1FFF));
	seq_printf(s, "ROI_H           :%11d\n", (reg & 0x1FFF0000) >> 16);

	reg = readl(crop->regs[0] + DISP_CROP_SRC);
	seq_printf(s, "SRC_W           :%11d\n", (reg & 0x1FFF));
	seq_printf(s, "SRC_H           :%11d\n", (reg & 0x1FFF0000) >> 16);

	reg = readl(crop->regs[0] + DISP_CROP_OFFSET);
	seq_printf(s, "OFF_X           :%11d\n", (reg & 0x1FFF));
	seq_printf(s, "OFF_Y           :%11d\n", (reg & 0x1FFF0000) >> 16);

	reg = readl(crop->regs[0] + DISP_CROP_MON1);
	seq_printf(s, "POS_X           :%11d\n", (reg & 0x1FFF));
	seq_printf(s, "POS_Y           :%11d\n", (reg & 0x1FFF0000) >> 16);

	reg = readl(crop->regs[0] + DISP_CROP_MON0);
	seq_printf(s, "CROP_STATE      :%11x\n", (reg & 0x3F));
	seq_printf(s, "IN_VALID        :%11x\n", CHECK_BIT(reg, 16) ? 1 : 0);
	seq_printf(s, "IN_READY        :%11x\n", CHECK_BIT(reg, 17) ? 1 : 0);
	seq_printf(s, "OUT_VALID       :%11x\n", CHECK_BIT(reg, 18) ? 1 : 0);
	seq_printf(s, "OUT_READY       :%11x\n", CHECK_BIT(reg, 19) ? 1 : 0);

	reg = readl(crop->regs[0] + DISP_CROP_DEBUG);
	seq_printf(s, "FORCE_COMMIT    :%11x\n", CHECK_BIT(reg, 0) ? 1 : 0);
	seq_printf(s, "BYPASS_SHADOW   :%11x\n", CHECK_BIT(reg, 1) ? 1 : 0);
	seq_printf(s, "READ_WRK_REG    :%11x\n", CHECK_BIT(reg, 2) ? 1 : 0);

	return 0;
}

static int crop_debug_client_open(struct inode *inode, struct file *file)
{
	return single_open(file, crop_debug_show, inode->i_private);
}

static const struct file_operations crop_debug_fops = {
	.open = crop_debug_client_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

static int mtk_crop_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_crop *crop;
	struct resource *regs;
#ifdef CONFIG_MTK_DEBUGFS
	struct dentry *debug_dentry;
#endif
	int i;

	crop = devm_kzalloc(dev, sizeof(*crop), GFP_KERNEL);
	if (!crop)
		return -ENOMEM;

	crop->hw_nr = MAX_HW_NR;

#if defined(CONFIG_COMMON_CLK_MT3612)
	for (i = 0; i < crop->hw_nr; i++) {
		crop->clk[i] = of_clk_get(dev->of_node, i);
		if (IS_ERR(crop->clk[i])) {
			dev_err(dev, "Failed to get crop hw %d clock\n", i);
			return PTR_ERR(crop->clk[i]);
		}
	}
#endif
	for (i = 0; i < crop->hw_nr; i++) {
		regs = platform_get_resource(pdev, IORESOURCE_MEM, i);
		crop->regs[i] = devm_ioremap_resource(dev, regs);
		if (IS_ERR(crop->regs[i])) {
			dev_err(dev, "Failed to map crop registers\n");
			return PTR_ERR(crop->regs[i]);
		}
		crop->reg_base[i] = (u32)regs->start;
	}

	of_property_read_u32_array(dev->of_node, "gce-subsys",
			crop->subsys, crop->hw_nr);

	of_node_put(dev->of_node);

#if CONFIG_PM
	pm_runtime_enable(dev);
#endif
	platform_set_drvdata(pdev, crop);
#ifdef CONFIG_MTK_DEBUGFS
	if (!mtk_debugfs_root)
		goto debugfs_done;

	/* debug info create */
	if (!mtk_crop_debugfs_root)
		mtk_crop_debugfs_root = debugfs_create_dir("crop",
							mtk_debugfs_root);

	if (!mtk_crop_debugfs_root) {
		dev_err(dev, "failed to create crop debugfs root directory.\n");
		goto debugfs_done;
	}

	debug_dentry = debugfs_create_file(pdev->name, 0664,
					   mtk_crop_debugfs_root,
					   crop, &crop_debug_fops);

debugfs_done:
	if (!debug_dentry)
		dev_err(dev, "Failed to create crop debugfs %s\n",
			pdev->name);
#endif

	return 0;
}

static int mtk_crop_remove(struct platform_device *pdev)
{
#if CONFIG_PM
	pm_runtime_disable(&pdev->dev);
#endif

#ifdef CONFIG_MTK_DEBUGFS
	debugfs_remove_recursive(mtk_crop_debugfs_root);
	mtk_crop_debugfs_root = NULL;
#endif

	return 0;
}

static const struct of_device_id crop_driver_dt_match[] = {
	{ .compatible = "mediatek,mt3612-crop" },
	{},
};
MODULE_DEVICE_TABLE(of, crop_driver_dt_match);

struct platform_driver mtk_crop_driver = {
	.probe		= mtk_crop_probe,
	.remove		= mtk_crop_remove,
	.driver		= {
		.name	= "mediatek-crop",
		.owner	= THIS_MODULE,
		.of_match_table = crop_driver_dt_match,
	},
};

module_platform_driver(mtk_crop_driver);
MODULE_LICENSE("GPL");
