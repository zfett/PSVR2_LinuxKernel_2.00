/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	Peng Liu <peng.liu@mediatek.com>
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
 * @defgroup IP_group_padding PADDING
 *     Padding is the driver of PADDING which is dedicated for image padding\n
 *     function. The interface includes parameter setting and control\n
 *     function. Parameter setting includes setting of region. Control\n
 *     function includes power on/off, start, stop, and reset.
 *
 *     @{
 *         @defgroup IP_group_padding_external EXTERNAL
 *             The external API document for padding.\n
 *
 *             @{
 *                 @defgroup IP_group_padding_external_function 1.function
 *                     Exported function of padding
 *                 @defgroup IP_group_padding_external_struct 2.structure
 *                     none.
 *                 @defgroup IP_group_padding_external_typedef 3.typedef
 *                     none.
 *                 @defgroup IP_group_padding_external_enum 4.enumeration
 *                     none.
 *                 @defgroup IP_group_padding_external_def 5.define
 *                     none.
 *             @}
 *
 *         @defgroup IP_group_padding_internal INTERNAL
 *             The internal API document for padding.\n
 *
 *             @{
 *                 @defgroup IP_group_padding_internal_function 1.function
 *                     none.
 *                 @defgroup IP_group_padding_internal_struct 2.structure
 *                     none.
 *                 @defgroup IP_group_padding_internal_typedef 3.typedef
 *                     none.
 *                 @defgroup IP_group_padding_internal_enum 4.enumeration
 *                     none.
 *                 @defgroup IP_group_padding_internal_def 5.define
 *                     none.
 *             @}
 *     @}
 */

#include <asm-generic/io.h>
#include <linux/clk.h>
#include <linux/compiler.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/soc/mediatek/mtk-cmdq.h>
#include <soc/mediatek/mtk_padding.h>
#include "mtk_padding_reg.h"

#ifdef CONFIG_MTK_DEBUGFS
struct dentry *mtk_padding_debugfs_root;
#endif

struct mtk_padding {
	struct device	*dev;
	struct clk		*clk;
	void __iomem	*regs;
	u32 irq;
	spinlock_t spinlock_irq;
	mtk_mmsys_cb cb_func;
	u32 cb_status;
	void *cb_priv_data;
	u32 reg_base;
	u32 subsys;
	struct cmdq_pkt *pkt;
};

static void reg_write_mask(struct mtk_padding *padding, u32 offset,
			u32 value, u32 mask)
{
	if (padding->pkt) {
		cmdq_pkt_write_value(padding->pkt, padding->subsys,
				(padding->reg_base & 0xffff) | offset,
				value, mask);
	} else {
		u32 reg;

		reg = readl(padding->regs + offset);
		reg &= ~mask;
		reg |= value;
		writel(reg, padding->regs + offset);
	}
}

static void reg_write(struct mtk_padding *padding, u32 offset, u32 value)
{
	if (padding->pkt)
		reg_write_mask(padding, offset, value, 0xffffffff);
	else
		writel(value, padding->regs + offset);
}

static void mtk_padding_set_num(struct mtk_padding *padding, u32 num)
{
	reg_write_mask(padding, PADDING_CON, num, PADDING_NUMBER);
}

static void mtk_padding_set_mode(struct mtk_padding *padding, u32 mode)
{
	reg_write_mask(padding, PADDING_CON, mode, ONE_PADDING_MODE);
}

static void mtk_padding_bypass(struct mtk_padding *padding, u32 bypass)
{
	reg_write_mask(padding, PADDING_CON, bypass, PADDING_BYPASS);
}

static void mtk_padding_enable(struct mtk_padding *padding, u32 enable)
{
	reg_write_mask(padding, PADDING_CON, enable, PADDING_EN);
}

static void mtk_padding_rst(struct mtk_padding *padding)
{
	reg_write_mask(padding, PADDING_INT_CON, 1, PADDING_SW_RESET);
	reg_write_mask(padding, PADDING_INT_CON, 0, PADDING_SW_RESET);
}

static void mtk_padding_pic_size(struct mtk_padding *padding,
			u32 width, u32 height)
{
	/**reg_write_mask(padding, PADDING_SHADOW, BYPASS_SHADOW,
	*		BYPASS_SHADOW);
	**/
	reg_write(padding, PADDING_PIC_SIZE, width | (height << 16));
}

/** @ingroup IP_group_padding_external_function
 * @par Description
 *     Start the functon of padding.
 * @param[in]
 *     dev: pointer of padding device structure.
 * @param[in]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @return
 *     0, success.\n
 *     -EINVAL, Invalid argument.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_padding_start(const struct device *dev, struct cmdq_pkt *handle)
{
	struct mtk_padding *padding;

	if (!dev)
	return -EINVAL;

	padding = dev_get_drvdata(dev);

	if (!padding)
		return -ENODEV;

	padding->pkt = handle;
	mtk_padding_enable(padding, PADDING_EN);
	return 0;
}
EXPORT_SYMBOL(mtk_padding_start);

/** @ingroup IP_group_padding_external_function
 * @par Description
 *     Stop the functon of padding.
 * @param[in]
 *     dev: pointer of padding device structure.
 * @param[in]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @return
 *     0, success.\n
 *     -EINVAL, Invalid argument.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_padding_stop(const struct device *dev, struct cmdq_pkt *handle)
{
	struct mtk_padding *padding;

	if (!dev)
	return -EINVAL;

	padding = dev_get_drvdata(dev);

	if (!padding)
		return -ENODEV;

	padding->pkt = handle;
	mtk_padding_enable(padding, 0);
	return 0;
}
EXPORT_SYMBOL(mtk_padding_stop);

/** @ingroup IP_group_padding_external_function
 * @par Description
 *     bypass the functon of padding.
 * @param[in]
 *     dev: pointer of padding device structure.
 * @param[in]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @param[in]
 *     bypass: bypass padding or not.
 * @return
 *     0, success.\n
 *     -EINVAL, Invalid argument.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_padding_set_bypass(const struct device *dev,
			struct cmdq_pkt *handle, bool bypass)
{
	struct mtk_padding *padding;

	if (!dev)
	return -EINVAL;

	padding = dev_get_drvdata(dev);

	if (!padding)
		return -ENODEV;

	padding->pkt = handle;
	mtk_padding_bypass(padding, bypass == true ? PADDING_BYPASS : 0);
	return 0;
}
EXPORT_SYMBOL(mtk_padding_set_bypass);

/** @ingroup IP_group_padding_external_function
 * @par Description
 *     Reset the functon of padding.
 * @param[in]
 *     dev: pointer of padding device structure.
 * @param[in]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @return
 *     0, success.\n
 *     -EINVAL, Invalid argument.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_padding_reset(const struct device *dev, struct cmdq_pkt *handle)
{
	struct mtk_padding *padding;

	if (!dev)
	return -EINVAL;

	padding = dev_get_drvdata(dev);

	if (!padding)
		return -ENODEV;

	padding->pkt = handle;
	mtk_padding_rst(padding);
	return 0;
}
EXPORT_SYMBOL(mtk_padding_reset);

/** @ingroup IP_group_padding_external_function
 * @par Description
 *     Set the padding num. and padding pic width and height\n
 * @param[in]
 *     dev: pointer of padding device structure.
 * @param[in]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @param[in]
 *     mode: padding mode, #mtk_padding_mode\n
 * @param[in]
 *     num: padding num, range 0 to 126.
 * @param[in]
 *     width: input width of pic, range 0 to 2560.
 * @param[in]
 *     height: input height of pic, range 0 to 640.
 * @return
 *     0, success.\n
 *     -EINVAL, Invalid argument.\n
 * @par Boundary case and Limitation
 *     mode should be in #mtk_padding_mode.\n
 *     num should be in the range of 0 to 126.\n
 *     width should be in the range of 0 to 2560.\n
 *     height should be in the range of 0 to 640.\n
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_padding_config(const struct device *dev, struct cmdq_pkt *handle,
		enum mtk_padding_mode mode, u32 num, u32 width, u32 height)
{
	struct mtk_padding *padding;

	if (!dev)
	return -EINVAL;

	if (((num & 0x1) == 1) || (num > 126)) {
		dev_err(dev, "ERROR:Set Padding num %u\n", num);
		dev_err(dev, "Padding num must be EVEN and smaller 127\n");
		return -EINVAL;
	}
	if (mode != MTK_PADDING_MODE_QUARTER && mode != MTK_PADDING_MODE_HALF) {
		dev_err(dev, "ERROR:Set Padding mode %u\n", mode);
		dev_err(dev, "Padding mode must be 0 or 1\n");
		return -EINVAL;
	}
	if (width > 2560) {
		dev_err(dev, "ERROR:Set Padding width %u\n", width);
		dev_err(dev, "Padding width must be smaller 2560\n");
		return -EINVAL;
	}
	if (height > 2560) {
		dev_err(dev, "ERROR:Set Padding height %u\n", height);
		dev_err(dev, "Padding height must be smaller 2560\n");
		return -EINVAL;
	}
	if (mode == MTK_PADDING_MODE_QUARTER && ((width & 0x3) != 0)) {
		dev_err(dev, "ERROR:Set Padding width %u\n", width);
		dev_err(dev, "Padding mode 0,width must be 4 align\n");
		return -EINVAL;
	}
	if (mode == MTK_PADDING_MODE_HALF && ((width & 0x1) != 0)) {
		dev_err(dev, "ERROR:Set Padding width %u\n", width);
		dev_err(dev, "Padding mode 1,width must be 2 align\n");
		return -EINVAL;
	}
	padding = dev_get_drvdata(dev);

	if (!padding)
		return -ENODEV;

	padding->pkt = handle;
	mtk_padding_set_mode(padding,
		mode == MTK_PADDING_MODE_HALF ? ONE_PADDING_MODE : 0);
	mtk_padding_set_num(padding, num);
	mtk_padding_pic_size(padding, width, height - 1);
	return 0;
}
EXPORT_SYMBOL(mtk_padding_config);

/** @ingroup IP_group_padding_external_function
 * @par Description
 *     Turn on the power of padding. Before any configuration, you should\n
 *         turn on the power of padding.
 * @param[in]
 *     dev: pointer of padding device structure.
 * @return
 *     0, success.\n
 *     -EINVAL, Invalid argument.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_padding_power_on(struct device *dev)
{
#ifdef CONFIG_COMMON_CLK_MT3612
	struct mtk_padding *padding;
	int ret;

	if (!dev)
		return -EINVAL;

	padding = dev_get_drvdata(dev);

	if (!padding)
		return -ENODEV;

	ret = pm_runtime_get_sync(dev);
	if (ret < 0) {
		dev_err(dev, "Failed to enable power domain: %d\n", ret);
		return ret;
	}

	if (!padding->clk)
		return -EINVAL;

	ret = clk_prepare_enable(padding->clk);
	if (ret) {
		dev_err(dev, "Failed to enable clock ret %d\n",
				ret);
		goto err;
	}
#endif
	return 0;
#ifdef CONFIG_COMMON_CLK_MT3612
err:
	clk_disable_unprepare(padding->clk);
	pm_runtime_put(dev);
	return ret;
#endif
}
EXPORT_SYMBOL(mtk_padding_power_on);

/** @ingroup IP_group_padding_external_function
 * @par Description
 *     Turn off the power of padding.
 * @param[in]
 *     dev: pointer of padding device structure.
 * @return
 *     0, success.\n
 *     -EINVAL, Invalid argument.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_padding_power_off(struct device *dev)
{
#ifdef CONFIG_COMMON_CLK_MT3612
	struct mtk_padding *padding;

	if (!dev)
		return -EINVAL;

	padding = dev_get_drvdata(dev);

	if (!padding)
		return -ENODEV;

	if (!padding->clk)
		return -EINVAL;

	clk_disable_unprepare(padding->clk);
	pm_runtime_put(dev);
#endif
	return 0;
}
EXPORT_SYMBOL(mtk_padding_power_off);

/** @ingroup IP_group_padding_external_function
 * @par Description
 *     Register callback function for padding status.
 * @param[in]
 *     dev: pointer of padding device structure.
 * @param[in]
 *     func: callback function which is called by padding interrupt handler.
 * @param[in]
 *     status: status that user want to monitor.
 * @param[in]
 *     priv_data: caller may have multiple instance, so it could use this\n
 *         private data to distinguish which instance is callbacked.
 * @return
 *     0, success.\n
 *     -EINVAL, Invalid argument.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_padding_register_cb(const struct device *dev, mtk_mmsys_cb func,
			u32 status, void *priv_data)
{
	struct mtk_padding *padding;
	unsigned long flags;

	if (!dev)
		return -EINVAL;

	padding = dev_get_drvdata(dev);

	if (!padding)
		return -ENODEV;

	if (status & 0xFFFFFFF9) {
		dev_err(dev, "ERROR:Set Padding status %u\n", status);
		return -EINVAL;
	}

	spin_lock_irqsave(&padding->spinlock_irq, flags);
	padding->cb_func = func;
	padding->cb_status = status;
	padding->cb_priv_data = priv_data;
	spin_unlock_irqrestore(&padding->spinlock_irq, flags);

	padding->pkt = NULL;

	if (status & ABNORMAL_EOF_INTEN)
		reg_write_mask(padding, PADDING_INT_CON,
				ABNORMAL_EOF_INTEN, ABNORMAL_EOF_INTEN);

	if (status & PADDING_DONE_INTEN)
		reg_write_mask(padding, PADDING_INT_CON,
				PADDING_DONE_INTEN, PADDING_DONE_INTEN);

	return 0;
}
EXPORT_SYMBOL(mtk_padding_register_cb);

static irqreturn_t mtk_padding_irq_handler(int irq, void *dev_id)
{
	struct mtk_padding *padding = dev_id;
	struct mtk_mmsys_cb_data cb_data;
	mtk_mmsys_cb func;
	unsigned long flags;
	u32 reg, reg1 = 0, status = 0;

	reg  = readl(padding->regs + PADDING_STA);
	reg1 = readl(padding->regs + PADDING_INT_CON);

	if (!(reg & (ABNORMAL_EOF_INTSTA | PADDING_DONE_INTSTA)))
		return IRQ_NONE;

	spin_lock_irqsave(&padding->spinlock_irq, flags);
	func = padding->cb_func;
	status = padding->cb_status;
	cb_data.priv_data = padding->cb_priv_data;
	cb_data.status = 0;
	spin_unlock_irqrestore(&padding->spinlock_irq, flags);

	cb_data.part_id = 0;

	if (reg & ABNORMAL_EOF_INTSTA) {
		writel(reg1 | ABNORMAL_EOF_INTACK,
				padding->regs + PADDING_INT_CON);
		writel(reg1, padding->regs + PADDING_INT_CON);
		if (status & ABNORMAL_EOF_INTEN)
		cb_data.status |= ABNORMAL_EOF_INTEN;
	}

	if (reg & PADDING_DONE_INTSTA) {
		writel(reg1 | PADDING_DONE_INTACK,
				padding->regs + PADDING_INT_CON);
		writel(reg1, padding->regs + PADDING_INT_CON);
		if (status & PADDING_DONE_INTEN)
		cb_data.status |= PADDING_DONE_INTEN;
	}

	if (func && cb_data.status)
		func(&cb_data);

	return IRQ_HANDLED;
}

#ifdef CONFIG_MTK_DEBUGFS
static int padding_debug_show(struct seq_file *s, void *unused)
{
	struct mtk_padding *padding = s->private;
	u32 reg;

	seq_printf(s, "---- padding name: %s ----\n", dev_name(padding->dev));
	reg = readl(padding->regs + PADDING_CON);
	seq_printf(s, "padding number %d\n", (u32)(reg & PADDING_NUMBER));
	seq_printf(s, "padding bypass %d\n",
			(u32)(reg & PADDING_BYPASS) >> 8);
	seq_printf(s, "padding enable %d\n",
			(u32)(reg & PADDING_EN) >> 9);
	seq_printf(s, "padding mode   %d\n",
			(u32)(reg & ONE_PADDING_MODE) >> 10);
	reg = readl(padding->regs + PADDING_PIC_SIZE);
	seq_printf(s, "padding input w x h = %d x %d\n",
			(u32)(reg & PIC_WIDTH),
			(u32)(((reg & PIC_HEIGHT_M1) >> 16) + 1));
	reg = readl(padding->regs + PADDING_IN_CNT);
	seq_printf(s, "padding input  cnt %d\n", reg);
	reg = readl(padding->regs + PADDING_OUT_CNT);
	seq_printf(s, "padding output cnt %d\n", reg);
	reg = readl(padding->regs + PADDING_INT_CON);
	seq_printf(s, "padding int cfg rst %d eof %d done %d\n",
			(u32)(reg & PADDING_SW_RESET),
			(u32)(reg & ABNORMAL_EOF_INTEN) >> 1,
			(u32)(reg & PADDING_DONE_INTEN) >> 2);
	seq_puts(s, "----------------------------------\n");

	return 0;
}

static int debug_client_open(struct inode *inode, struct file *file)
{
	return single_open(file, padding_debug_show, inode->i_private);
}

static const struct file_operations padding_debug_fops = {
	.open = debug_client_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

static int mtk_padding_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_padding *padding;
	struct resource *regs;
	int irq, ret;
#ifdef CONFIG_MTK_DEBUGFS
	struct dentry *debug_dentry = NULL;
#endif

	padding = devm_kzalloc(dev, sizeof(*padding), GFP_KERNEL);
	if (!padding)
		return -ENOMEM;

	padding->dev = dev;

#ifdef CONFIG_COMMON_CLK_MT3612
	padding->clk = devm_clk_get(dev, "PADDING");
	if (IS_ERR(padding->clk)) {
		dev_err(dev, "Failed to get padding clock\n");
		return PTR_ERR(padding->clk);
	}
#endif

	spin_lock_init(&padding->spinlock_irq);

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	padding->regs = devm_ioremap_resource(dev, regs);
	if (IS_ERR(padding->regs)) {
		dev_err(dev, "Failed to map padding registers\n");
		return PTR_ERR(padding->regs);
	}
	padding->reg_base = (u32)regs->start;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(dev, "Failed to get padding irq.\n");
		return irq;
	}

	ret = devm_request_irq(dev, irq, mtk_padding_irq_handler,
		       IRQF_SHARED, dev_name(dev), padding);
	if (ret < 0) {
		dev_err(dev, "Failed to request padding irq %d: %d\n",
				irq, ret);
		return ret;
	}
	padding->irq = irq;

	of_property_read_u32(dev->of_node, "gce-subsys", &padding->subsys);

	of_node_put(dev->of_node);

#ifdef CONFIG_COMMON_CLK_MT3612
	pm_runtime_enable(dev);
#endif

	platform_set_drvdata(pdev, padding);

#ifdef CONFIG_MTK_DEBUGFS
	if (!mtk_debugfs_root)
		goto debugfs_done;

	/* debug info create */
	if (!mtk_padding_debugfs_root)
		mtk_padding_debugfs_root = debugfs_create_dir("padding",
						mtk_debugfs_root);

	if (!mtk_padding_debugfs_root) {
		dev_dbg(dev, "failed to create padding debugfs root directory.\n");
		goto debugfs_done;
	}

	debug_dentry = debugfs_create_file(pdev->name, 0664,
					   mtk_padding_debugfs_root,
					   padding, &padding_debug_fops);
debugfs_done:
		if (!debug_dentry)
			dev_warn(dev, "Failed to create padding debugfs %s\n",
				pdev->name);
#endif

	dev_dbg(dev, "padding probe done!\n");

	return 0;
}

static int mtk_padding_remove(struct platform_device *pdev)
{
#ifdef CONFIG_COMMON_CLK_MT3612
	pm_runtime_disable(&pdev->dev);
#endif
#ifdef CONFIG_MTK_DEBUGFS
	debugfs_remove_recursive(mtk_padding_debugfs_root);
	mtk_padding_debugfs_root = NULL;
#endif

	return 0;
}

static const struct of_device_id padding_driver_dt_match[] = {
	{ .compatible = "mediatek,mt3612-padding" },
	{},
};
MODULE_DEVICE_TABLE(of, padding_driver_dt_match);

struct platform_driver mtk_padding_driver = {
	.probe		= mtk_padding_probe,
	.remove		= mtk_padding_remove,
	.driver		= {
		.name	= "mediatek-padding",
		.owner	= THIS_MODULE,
		.of_match_table = padding_driver_dt_match,
	},
};

module_platform_driver(mtk_padding_driver);
MODULE_LICENSE("GPL");
