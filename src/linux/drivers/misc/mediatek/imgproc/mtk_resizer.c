/*
 * Copyright (c) 2017 MediaTek Inc.
 * Authors:
 *	Bibby Hsieh <bibby.hsieh@mediatek.com>
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
 * @defgroup IP_group_resizer RESIZER
 *   This Resizer driver is used to config Mediatek Resizer hardware engine.\n
 *   Resizer can perform video resizing to a target video width and height.\n
 *   It consists of a High Fidelity Scaler. The High Fidelity Scaler\n
 *   preserves signal energy while up-scaling the content of video.
 *
 *   @{
 *       @defgroup IP_group_resizer_external EXTERNAL
 *         The external API document for Resizer.
 *
 *         @{
 *            @defgroup IP_group_resizer_external_function 1.function
 *              External function for Resizer.
 *            @defgroup IP_group_resizer_external_struct 2.structure
 *              External structure for Resizer.
 *            @defgroup IP_group_resizer_external_typedef 3.typedef
 *              none.
 *            @defgroup IP_group_resizer_external_enum 4.enumeration
 *              none.
 *            @defgroup IP_group_resizer_external_def 5.define
 *              none.
 *         @}
 *
 *       @defgroup IP_group_resizer_internal INTERNAL
 *         The internal API document for Resizer.
 *
 *         @{
 *            @defgroup IP_group_resizer_internal_function 1.function
 *              Internal function for Resizer.
 *            @defgroup IP_group_resizer_internal_struct 2.structure
 *              Internal structure for Resizer.
 *            @defgroup IP_group_resizer_internal_typedef 3.typedef
 *              none.
 *            @defgroup IP_group_resizer_internal_enum 4.enumeration
 *              none.
 *            @defgroup IP_group_resizer_internal_def 5.define
 *              Internal define for Resizer.
 *         @}
 *     @}
 */

#include <asm-generic/io.h>
#include <linux/cdev.h>
#include <linux/clk.h>
#include <linux/compiler.h>
#include <linux/completion.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/soc/mediatek/mtk-cmdq.h>
#include <soc/mediatek/mtk_resizer.h>
#include <uapi/mediatek/mtk_resizer_ioctl.h>
#include "mtk_resizer_reg.h"

/** @ingroup IP_group_resizer_internal_def
 * @brief Resizer constant definition
 * @{
 */
#define CLIP_THRES_8B_SHIFT     2
#define RSZ0                    "rsz0"
#define RSZ1                    "rsz1"
#define RSZ2                    "rsz2"
#define RSZ3                    "rsz3"
#define RSZ_MIN_SIZE            16
#define RSZ_MAX_SIZE            65535
#define RSZ_LINE_BUF_SIZE       992
#define RSZ_LINE_BUF_SIZE_EX    1120
#define P_RSZ1_LINE_BUF_SIZE    1328
#define P_RSZ2_LINE_BUF_SIZE    664
/**
 * @}
 */

#ifdef CONFIG_MTK_DEBUGFS
struct dentry *mtk_resizer_debugfs_root;
#define CHECK_BIT(var, pos) ((var) & (1<<(pos)))
#endif

/** @ingroup IP_group_resizer_internal_struct
 * @brief Resizer driver data structure
 */
struct mtk_resizer {
	/** Resizer device node */
	struct device			*dev;
	int				irq[MTK_MM_MODULE_MAX_NUM];
	/** Resizer clock node */
	struct clk			*clk[MTK_MM_MODULE_MAX_NUM];
	/** Resizer register base */
	void __iomem			*regs[MTK_MM_MODULE_MAX_NUM];
	/** GCE packet */
	struct cmdq_pkt			**cmdq_handle;
	/** GCE offset */
	u32				cmdq_offset[MTK_MM_MODULE_MAX_NUM];
	/** GCE subsys */
	u32				cmdq_subsys[MTK_MM_MODULE_MAX_NUM];
	/** module count */
	u32				module_cnt;
	dev_t				devno;
	struct cdev			cdev;
	struct class			*class;
	wait_queue_head_t		event_wait;
	int				event_request;
	int				request_res[5];
	struct rsz_scale_config		*fw_output;
	struct completion		cmplt;
	mtk_mmsys_cb			cb_func;
	struct mtk_mmsys_cb_data	cb_data;
	spinlock_t			spinlock_irq;
	u8				framedone;
	u8				framestart;
	/** resizer input format, RGB/YUV */
	u16				rsz_in_format;
};

/** @ingroup IP_group_resizer_internal_function
 * @par Description
 *    Resizer driver irq handler. Resizer will send the interrupt\n
 *    when size error occurs. We detect the size error in this handler.
 * @param[in]
 *     irq: irq information.
 * @param[in]
 *     dev_id: reiszer driver data.
 * @return
 *     irqreturn_t.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static irqreturn_t mtk_resizer_irq_handler(int irq, void *dev_id)
{
	mtk_mmsys_cb cb;
	struct mtk_mmsys_cb_data cb_data;
	struct mtk_resizer *resizer = dev_id;
	u32 val = 0;
	u32 m_idx, cb_status;
	unsigned long flags;

	for (m_idx = 0; m_idx < resizer->module_cnt; m_idx++) {
		if (irq == resizer->irq[m_idx]) {
			val = readl(resizer->regs[m_idx] + RSZ_INT_FLAG);
			writel(0, resizer->regs[m_idx] + RSZ_INT_FLAG);
			break;
		}
	}
	if (m_idx == resizer->module_cnt)
		return IRQ_NONE;

	spin_lock_irqsave(&resizer->spinlock_irq, flags);
	cb_status = resizer->cb_data.status;
	cb_data.priv_data = resizer->cb_data.priv_data;
	cb = resizer->cb_func;
	spin_unlock_irqrestore(&resizer->spinlock_irq, flags);

	if (val && cb) {
		bool frame_done_cb = false, frame_start_cb = false;
		const u32 frame_done_val = (1 << resizer->module_cnt) - 1;

		if ((cb_status & RSZ_FRAME_END_INT) && (val & RSZ_FRAME_END)) {
			spin_lock_irqsave(&resizer->spinlock_irq, flags);
			resizer->framedone |= (1 << m_idx);
			if (resizer->framedone == frame_done_val) {
				resizer->framedone = 0;
				frame_done_cb = true;
			}
			spin_unlock_irqrestore(&resizer->spinlock_irq, flags);
			val &= ~RSZ_FRAME_END;
		}

		if ((cb_status & RSZ_FRAME_START_INT) &&
					(val & RSZ_FRAME_START)) {
			spin_lock_irqsave(&resizer->spinlock_irq, flags);
			resizer->framestart |= (1 << m_idx);
			if (resizer->framestart == frame_done_val) {
				resizer->framestart = 0;
				frame_start_cb = true;
			}
			spin_unlock_irqrestore(&resizer->spinlock_irq, flags);
			val &= ~RSZ_FRAME_START;
		}

		if ((cb_status & RSZ_ERR_INT) && (val & RSZ_SIZE_ERR)) {
			cb_data.part_id = m_idx;
			cb_data.status = RSZ_ERR_INT;
			cb(&cb_data);
			val &= ~RSZ_SIZE_ERR;
		}

		if (frame_start_cb) {
			cb_data.part_id = frame_done_val;
			cb_data.status = RSZ_FRAME_START_INT;
			cb(&cb_data);
		}

		if (frame_done_cb) {
			cb_data.part_id = frame_done_val;
			cb_data.status = RSZ_FRAME_END_INT;
			cb(&cb_data);
		}
	}
	return IRQ_HANDLED;
}

/** @ingroup IP_group_resizer_internal_function
 * @par Description
 *     Update only one Resizer hardware register with mask.
 * @param[in]
 *     resizer: Resizer driver data structure.
 * @param[in]
 *     idx: hardware index.
 * @param[in]
 *     offset: Resizer register offset.
 * @param[in]
 *     value: the specified target register 32bits value.
 * @param[in]
 *     mask: the specified target register mask.
 * @return
 *     0, configuration success.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int resizer_single_reg_write(struct mtk_resizer *resizer, u32 idx,
				    u32 offset, u32 value, u32 mask)
{
	if (resizer->cmdq_handle && resizer->cmdq_handle[idx]) {
		return cmdq_pkt_write_value(resizer->cmdq_handle[idx],
					    resizer->cmdq_subsys[idx],
					    resizer->cmdq_offset[idx] | offset,
					    value, mask);
	} else {
		u32 tmp = readl(resizer->regs[idx] + offset);

		tmp = (tmp & ~mask) | (value & mask);
		writel(tmp, resizer->regs[idx] + offset);

		return 0;
	}
}

/** @ingroup IP_group_resizer_internal_function
 * @par Description
 *     Update the multiple Resizer hardware register with mask.
 * @param[in]
 *     resizer: Resizer driver data structure.
 * @param[in]
 *     offset: Resizer register offset.
 * @param[in]
 *     value: the specified target register 32bits value.
 * @param[in]
 *     mask: the specified target register mask.
 * @return
 *     0, configuration success.\n
 *     error code from resizer_single_reg_write().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If resizer_single_reg_write() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int resizer_reg_write(struct mtk_resizer *resizer,
			     u32 offset, u32 value, u32 mask)
{
	int idx, ret = 0;

	for (idx = 0; idx < resizer->module_cnt; idx++)
		ret |= resizer_single_reg_write(resizer, idx, offset,
						value, mask);

	return ret;
}

/** @ingroup IP_group_resizer_external_function
 * @par Description
 *     Enable the power and clock of Resizer engine.
 * @param[out]
 *     dev: Resizer device node.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from pm_runtime_get_sync().\n
 *     error code from clk_prepare_enable().
 * @par Boundary case and Limitation
 *     dev can not be NULL.
 * @par Error case and Error handling
 *     1. If dev is NULL, return -EINVAL.\n
 *     2. If pm_runtime_get_sync() fails, return its error code.
 *     3. If clk_prepare_enable() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_resizer_power_on(struct device *dev)
{
#if defined(CONFIG_COMMON_CLK_MT3612)
	struct mtk_resizer *resizer;
	int i, ret = 0;

	if (!dev)
		return -EINVAL;

	resizer = dev_get_drvdata(dev);

	if (!resizer)
		return -ENODEV;

	if (resizer->module_cnt > 4)
		return -EINVAL;

	ret = pm_runtime_get_sync(dev);
	if (ret < 0) {
		dev_err(resizer->dev,
			"Failed to enable power domain: %d\n", ret);
		return ret;
	}

	for (i = 0; i < resizer->module_cnt; i++) {
		if (resizer->clk[i]) {
		ret = clk_prepare_enable(resizer->clk[i]);
		if (ret) {
			dev_err(resizer->dev,
				"Failed to enable clock %d: %d\n", i, ret);
			goto err;
		}
		}
	}
#endif
	return 0;
#if defined(CONFIG_COMMON_CLK_MT3612)
err:
	while (--i >= 0) {
		if (resizer->clk[i])
		clk_disable_unprepare(resizer->clk[i]);
	}
	pm_runtime_put(dev);
	return ret;
#endif
}
EXPORT_SYMBOL(mtk_resizer_power_on);

/** @ingroup IP_group_resizer_external_function
 * @par Description
 *     Disable the power and clock of Resizer engine.
 * @param[out]
 *     dev: Resizer device node.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     dev can not be NULL.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_resizer_power_off(struct device *dev)
{
#if defined(CONFIG_COMMON_CLK_MT3612)
	struct mtk_resizer *resizer;
	int i;

	if (!dev)
		return -EINVAL;

	resizer = dev_get_drvdata(dev);

	if (!resizer)
		return -ENODEV;

	if (resizer->module_cnt > 4)
		return -EINVAL;

	for (i = 0; i < resizer->module_cnt; i++) {
		if (resizer->clk[i])
		clk_disable_unprepare(resizer->clk[i]);
	}
	pm_runtime_put(dev);
#endif
	return 0;
}
EXPORT_SYMBOL(mtk_resizer_power_off);

/** @ingroup IP_group_resizer_external_function
 * @par Description
 *     Enable Resizer engine.
 * @param[in]
 *     dev: Resizer device node.
 * @param[out]
 *     cmdq_handle: CMDQ command buffer packet.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from resizer_reg_write().
 * @par Boundary case and Limitation
 *     dev can not be NULL.
 * @par Error case and Error handling
 *     1. If dev is NULL, return -EINVAL.\n
 *     2. If resizer_single_reg_write() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_resizer_start(const struct device *dev, struct cmdq_pkt **cmdq_handle)
{
	struct mtk_resizer *resizer;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	resizer = dev_get_drvdata(dev);

	if (!resizer)
		return -ENODEV;

	resizer->cmdq_handle = cmdq_handle;
	ret = resizer_reg_write(resizer, RSZ_ENABLE, RSZ_EN, RSZ_EN);

	return ret;
}
EXPORT_SYMBOL(mtk_resizer_start);

/** @ingroup IP_group_resizer_external_function
 * @par Description
 *     Disable Resizer engine.
 * @param[in]
 *     dev: Resizer device node.
 * @param[out]
 *     cmdq_handle: CMDQ command buffer packet.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from resizer_reg_write().
 * @par Boundary case and Limitation
 *     dev can not be NULL.
 * @par Error case and Error handling
 *     1. If dev is NULL, return -EINVAL.\n
 *     2. If resizer_single_reg_write() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_resizer_stop(const struct device *dev, struct cmdq_pkt **cmdq_handle)
{
	struct mtk_resizer *resizer;

	if (!dev)
		return -EINVAL;

	resizer = dev_get_drvdata(dev);

	if (!resizer)
		return -ENODEV;

	resizer->cmdq_handle = cmdq_handle;
	return resizer_reg_write(resizer, RSZ_ENABLE, 0, RSZ_EN);
}
EXPORT_SYMBOL(mtk_resizer_stop);

/** @ingroup IP_group_resizer_external_function
 * @par Description
 *     Bypass Resizer engine.
 * @param[in]
 *     dev: Resizer device node.
 * @param[out]
 *     cmdq_handle: CMDQ command buffer packet.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from resizer_reg_write().
 * @par Boundary case and Limitation
 *     dev can not be NULL.
 * @par Error case and Error handling
 *     1. If dev is NULL, return -EINVAL.\n
 *     2. If resizer_single_reg_write() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_resizer_bypass(const struct device *dev, struct cmdq_pkt **cmdq_handle)
{
	struct mtk_resizer *resizer;

	if (!dev)
		return -EINVAL;

	resizer = dev_get_drvdata(dev);

	if (!resizer)
		return -ENODEV;

	resizer->cmdq_handle = cmdq_handle;
	return resizer_reg_write(resizer, RSZ_CONTROL_1, 0,
				 VERTICAL_EN | HORIZONTAL_EN);
}
EXPORT_SYMBOL(mtk_resizer_bypass);

/** @ingroup IP_group_resizer_internal_function
 * @par Description
 *     Check the input and output size if meet the pyramid resizer1 hardware\n
 *     limitation.\n
 * @param[in]
 *     resizer: Resizer driver data structure.
 * @param[in]
 *     in_width: input width.
 * @param[in]
 *     in_height: input height.
 * @param[in]
 *     out_width: output width.
 * @param[in]
 *     out_height: out height.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     The input/output data format is described in widthxheight.\n
 *     pyramid resizer1 -scaling case:\n
 *        16x16 <= input resolution <= 2656x65535\n
 *        output resolution: (in_width/2, in_height/2).
 * @par Error case and Error handling
 *        in_width < 16 or in_width > 2656, return -EINVAL\n
 *        in_height < 16 or in_height > 65535: return -EINVAL\n
 *        downscaling ratio not equals to 2: return -EINVAL
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_resizer_size_check_pyramid_rsz1(struct mtk_resizer *resizer,
				  u16 in_width, u16 in_height,
				  u16 out_width, u16 out_height)
{
	if (in_width == out_width && in_height == out_height) {
		dev_dbg(resizer->dev,
			 "Input width/height = Output width/height,\n"
			 "Resizer will be bypassed.\n");
		return 0;
	}

	/* support size check */
	if (in_width < RSZ_MIN_SIZE || in_width > P_RSZ1_LINE_BUF_SIZE * 2)
		goto err_in_width;
	else if (in_height < RSZ_MIN_SIZE || in_height > RSZ_MAX_SIZE)
		goto err_in_height;

	/* scaling ratio check */
	if ((in_width / out_width) != 2 || (in_height / out_height) != 2)
		goto err_scaling_ratio;

	dev_dbg(resizer->dev,
		 "Input Resolution = %dx%d, Output Resolution = %dx%d.\n",
		 in_width, in_height, out_width, out_height);

	return 0;

err_in_width:
	dev_err(resizer->dev, "Please check the in_width boundary\n");
	return -EINVAL;
err_in_height:
	dev_err(resizer->dev, "Please check the in_height boundary\n");
	return -EINVAL;
err_scaling_ratio:
	dev_err(resizer->dev, "Please check scaling ratio\n");
	return -EINVAL;
}

/** @ingroup IP_group_resizer_internal_function
 * @par Description
 *     Check the input and output size if meet the pyramid resizer2 hardware\n
 *     limitation.\n
 * @param[in]
 *     resizer: Resizer driver data structure.
 * @param[in]
 *     in_width: input width.
 * @param[in]
 *     in_height: input height.
 * @param[in]
 *     out_width: output width.
 * @param[in]
 *     out_height: out height.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     The input/output data format is described in widthxheight.\n
 *     pyramid resizer2 -scaling case:\n
 *        16x16 <= input resolution <= 1328x65535\n
 *        output resolution: (in_width/2, in_height/2).
 * @par Error case and Error handling
 *        in_width < 16 or in_width > 1328, return -EINVAL\n
 *        in_height < 16 or in_height > 65535: return -EINVAL\n
 *        downscaling ratio not equals to 2: return -EINVAL
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_resizer_size_check_pyramid_rsz2(struct mtk_resizer *resizer,
				  u16 in_width, u16 in_height,
				  u16 out_width, u16 out_height)
{
	if (in_width == out_width && in_height == out_height) {
		dev_dbg(resizer->dev,
			 "Input width/height = Output width/height,\n"
			 "Resizer will be bypassed.\n");
		return 0;
	}

	/* support size check */
	if (in_width < RSZ_MIN_SIZE || in_width > P_RSZ2_LINE_BUF_SIZE * 2)
		goto err_in_width;
	else if (in_height < RSZ_MIN_SIZE || in_height > RSZ_MAX_SIZE)
		goto err_in_height;

	/* scaling ratio check */
	if ((in_width / out_width) != 2 || (in_height / out_height) != 2)
		goto err_scaling_ratio;

	dev_dbg(resizer->dev,
		 "Input Resolution = %dx%d, Output Resolution = %dx%d.\n",
		 in_width, in_height, out_width, out_height);

	return 0;

err_in_width:
	dev_err(resizer->dev, "Please check the in_width boundary\n");
	return -EINVAL;
err_in_height:
	dev_err(resizer->dev, "Please check the in_height boundary\n");
	return -EINVAL;
err_scaling_ratio:
	dev_err(resizer->dev, "Please check scaling ratio\n");
	return -EINVAL;
}

/** @ingroup IP_group_resizer_internal_function
 * @par Description
 *     Check the input and output size if meet normal resizer hardware\n
 *     limitation.\n
 * @param[in]
 *     resizer: Resizer driver data structure.
 * @param[in]
 *     in_width: input width.
 * @param[in]
 *     in_height: input height.
 * @param[in]
 *     out_width: output width.
 * @param[in]
 *     out_height: out height.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     The input/output data format is described in widthxheight.\n
 *     1. In up-scaling case:\n
 *        16x16 <= input resolution <= 1984x65535\n
 *        input resolution <= output resolution <= 65535x65535\n
 *     2. In down-scaling case:\n
 *        16x16 <= input resolution <= 65535x65535\n
 *        if scaling ration as " 1/2<=ratio <1" range:\n
 *        output resolution <= min(input resolution, 1984x65535)\n
 *        else: output resolution <= min(input resolution, 992x65535)
 * @par Error case and Error handling
 *     1. In up-scaling case:\n
 *        in_width < 16 or in_width > 1984 or\n
 *        in_height < 16 or in_height > 65535: return -EINVAL
 *     2. In down-scaling case:\n
 *        in_width < 16 or out_width > 992(1984 for special case)\n
 *        or in_height < 16 or in_height> 65535: return -EINVAL
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_resizer_size_check_normal_rsz(struct mtk_resizer *resizer,
				  u16 in_width, u16 in_height,
				  u16 out_width, u16 out_height)
{
	dev_dbg(resizer->dev,
		"Input Resolution = %dx%d, Output Resolution = %dx%d.\n",
		in_width, in_height, out_width, out_height);

	if (in_width == out_width && in_height == out_height) {
		dev_dbg(resizer->dev,
			"Input width/height = Output width/height,\n"
			"Resizer will be bypassed.\n");
		return 0;
	}

	/* support size check */
	if (in_width < RSZ_MIN_SIZE || in_width > RSZ_MAX_SIZE)
		goto err_in_width;
	else if (in_height < RSZ_MIN_SIZE || in_height > RSZ_MAX_SIZE)
		goto err_in_height;

	if (out_width < RSZ_MIN_SIZE || out_width > RSZ_MAX_SIZE)
		goto err_out_width;
	else if (out_height < RSZ_MIN_SIZE || out_height > RSZ_MAX_SIZE)
		goto err_out_height;

	/* scaling width check */
	if (in_width > out_width) {
		if (in_width <= 2 * out_width) {
			if (out_width > RSZ_LINE_BUF_SIZE * 2)
				goto err_out_width;
		} else if (out_width > RSZ_LINE_BUF_SIZE)
			goto err_out_width;
		else if (in_width > 128 * out_width)
			goto err_scaling_ratio;
	} else if (in_width < out_width) {
		if (in_width > RSZ_LINE_BUF_SIZE * 2)
			goto err_in_width;
		else if (out_width > 64 * in_width)
			goto err_scaling_ratio;
	}

	/* scaling height check */
	if (in_height > 128 * out_height)
		goto err_scaling_ratio;
	else if (out_height > 64 * in_height)
		goto err_scaling_ratio;

	if ((in_width > out_width && in_height < out_height) ||
		(in_width < out_width && in_height > out_height))
		goto err_scaling_config;

	return 0;

err_in_width:
	dev_err(resizer->dev, "ERROR! Please check the in_width boundary\n");
	return -EINVAL;
err_out_width:
	dev_err(resizer->dev, "ERROR! Please check the out_width boundary\n");
	return -EINVAL;
err_in_height:
	dev_err(resizer->dev, "ERROR! Please check the in_height boundary\n");
	return -EINVAL;
err_out_height:
	dev_err(resizer->dev, "ERROR! Please check the out_height boundary\n");
	return -EINVAL;
err_scaling_ratio:
	dev_err(resizer->dev, "ERROR! Please check scaling ratio\n");
	return -EINVAL;
err_scaling_config:
	dev_err(resizer->dev, "ERROR! Not support up and down at same time\n");
	return -EINVAL;
}

/** @ingroup IP_group_resizer_internal_function
 * @par Description
 *     Check the input and output size if meet the Resizer hardware\n
 *     limitation.\n
 * @param[in]
 *     resizer: Resizer driver data structure.
 * @param[in]
 *     in_width: input width.
 * @param[in]
 *     in_height: input height.
 * @param[in]
 *     out_width: output width.
 * @param[in]
 *     out_height: out height.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     The input/output data format is described in widthxheight.\n
 *     check with API: mtk_resizer_size_check_normal_rsz\n
 *     mtk_resizer_size_check_pyramid_rsz1\n
 *     mtk_resizer_size_check_pyramid_rsz2
 * @par Error case and Error handling
 *     check with API: mtk_resizer_size_check_normal_rsz\n
 *     mtk_resizer_size_check_pyramid_rsz1\n
 *     mtk_resizer_size_check_pyramid_rsz2
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_resizer_size_check(struct mtk_resizer *resizer,
				  u16 in_width, u16 in_height,
				  u16 out_width, u16 out_height)
{
	int ret = 0;
	const char *rsz_name = NULL;

	rsz_name = dev_name(resizer->dev);
	dev_dbg(resizer->dev,
		 "dev_name:%s\n", rsz_name);

	if (strstr(rsz_name, RSZ0) || strstr(rsz_name, RSZ1)) {
		ret = mtk_resizer_size_check_normal_rsz(resizer,
			     (in_width / resizer->module_cnt),
			     in_height,
			     (out_width / resizer->module_cnt),
			     out_height);
	} else if (strstr(rsz_name, RSZ2)) {
		ret = mtk_resizer_size_check_pyramid_rsz1(resizer,
			     (in_width / resizer->module_cnt),
			     in_height,
			     (out_width / resizer->module_cnt),
			     out_height);
	} else if (strstr(rsz_name, RSZ3)) {
		ret = mtk_resizer_size_check_pyramid_rsz2(resizer,
			     (in_width / resizer->module_cnt),
			     in_height,
			     (out_width / resizer->module_cnt),
			     out_height);
	} else {
		dev_err(resizer->dev,
			"resizer device not support!\n");
		ret = -EINVAL;
	}
	return ret;
}

/** @ingroup IP_group_resizer_external_function
 * @par Description
 *     Config Resizer engine. In this function, Resizer driver needs\n
 *     Resizer firmware to calculate the parameter Resizer need. Caller\n
 *     provides the input and output width/height to this API. This API will\n
 *     pass the parameter to Resizer firmware. Resizer firmware will calculate\n
 *     the configurations.
 * @param[in]
 *     dev: Resizer device node.
 * @param[out]
 *     cmdq_handle: CMDQ command buffer packet.
 * @param[in]
 *     config: Resizer general config information.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from mtk_resizer_size_check().
 * @par Boundary case and Limitation
 *     1. Same as limitation of mtk_resizer_size_check function.\n
 *     2. Needs Resizer firmware.\n
 *     3. dev can not be NULL.
 * @par Error case and Error handling
 *     1. If mtk_resizer_size_check() fails, return its error code.\n
 *     2. If dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_resizer_config(const struct device *dev,
		       struct cmdq_pkt **cmdq_handle,
		       const struct mtk_resizer_config *config)
{
	struct mtk_resizer *resizer;
	int ret = 0;
	u32 coeff_step_x, coeff_step_y;
	u32 control_val, ibse_value;

	if (!dev)
		return -EINVAL;

	resizer = dev_get_drvdata(dev);

	if (!resizer)
		return -ENODEV;

	if (!config || resizer->module_cnt == 0)
		return -EINVAL;

	ret = mtk_resizer_size_check(resizer,
			(config->in_width / resizer->module_cnt),
			config->in_height,
			(config->out_width / resizer->module_cnt),
			config->out_height);
	if (ret != 0)
		return ret;

	resizer->request_res[0] = config->in_width - config->overlap;
	resizer->request_res[1] = config->in_height;
	resizer->request_res[2] = config->out_width;
	resizer->request_res[3] = config->out_height;
	resizer->request_res[4] = config->rsz_in_format;
	resizer->event_request = 1;
	wake_up_interruptible(&resizer->event_wait);
	ret = wait_for_completion_interruptible(&resizer->cmplt);
	if (ret != 0)
		return ret;

	control_val = ((resizer->fw_output->hor_enable << 0) |
		      (resizer->fw_output->ver_enable << 1) |
		      (resizer->fw_output->scaling_up << 4) |
		      (resizer->fw_output->hor_algo << 5) |
		      (resizer->fw_output->ver_algo << 7) |
		      (resizer->fw_output->hor_table << 16) |
		      (resizer->fw_output->ver_table << 21));
	coeff_step_x = resizer->fw_output->coeff_step_x;
	coeff_step_y = resizer->fw_output->coeff_step_y;
	ibse_value = ((resizer->fw_output->pq_g << 2) |
		     (resizer->fw_output->pq_t << 7) |
		     (resizer->fw_output->pq_r << 15));
	resizer->cmdq_handle = cmdq_handle;

	ret |= resizer_reg_write(resizer, RSZ_CONTROL_1,
				 control_val, CONTROL1_MASK);
	ret |= resizer_reg_write(resizer, RSZ_CONTROL_2,
				 resizer->fw_output->ibse_enable ?
				 PRESHP_EN : 0, PRESHP_EN);
	ret |= resizer_reg_write(resizer, RSZ_CONTROL_2,
				 resizer->fw_output->tap_adapt_enable ?
				 EDGE_PRESERVE_EN : 0, EDGE_PRESERVE_EN);
	ret |= resizer_reg_write(resizer, RSZ_INPUT_IMAGE, (config->in_width |
				 (config->in_height << 16)), 0xffffffff);
	ret |= resizer_reg_write(resizer, RSZ_OUTPUT_IMAGE, (config->out_width |
				 (config->out_height << 16)), 0xffffffff);
	ret |= resizer_reg_write(resizer, RSZ_VERT_COEFF_STEP,
				 coeff_step_y, COEFF_STEP_MASK);
	ret |= resizer_reg_write(resizer, RSZ_HORI_COEFF_STEP,
				 coeff_step_x, COEFF_STEP_MASK);
	ret |= resizer_reg_write(resizer, RSZ_TAP_ADAPT,
				 resizer->fw_output->tap_adapt_slope,
				 EDGE_PRESERVE_SLOPE);
	ret |= resizer_reg_write(resizer, RSZ_IBSE_PQ,
				 ibse_value, IBSE_PQ_MASK);

	return ret;
}
EXPORT_SYMBOL(mtk_resizer_config);

/** @ingroup IP_group_resizer_external_function
 * @par Description
 *     Config Resizer engine. In this function, Caller need to\n
 *     provides the input and output width/height to this API. \n
 *     About the upscaing case ,Client can decide if enable IBSE\n
 *     and TAB-ADAPT; about enable case, Client can also set IBSE Gain and\n
 *     tap adapt slope value for signal enhancement function.
 * @param[in]
 *     dev: Resizer device node.
 * @param[out]
 *     cmdq_handle: CMDQ command buffer packet.
 * @param[in]
 *     config: Resizer general config information.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from mtk_resizer_size_check().
 * @par Boundary case and Limitation
 *     1. Same as limitation of mtk_resizer_size_check function.\n
 *     2. Needs Resizer firmware.\n
 *     3. dev can not be NULL.
 * @par Error case and Error handling
 *     1. If mtk_resizer_size_check() fails, return its error code.\n
 *     2. If dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_resizer_config_test(const struct device *dev,
		       struct cmdq_pkt **cmdq_handle,
		       struct mtk_resizer_config_test *config)
{
	struct mtk_resizer *resizer;
	int ret = 0;
	u32 coeff_step_x, coeff_step_y;
	u32 control_val, ibse_value;

	if (!dev)
		return -EINVAL;

	resizer = dev_get_drvdata(dev);

	if (!resizer)
		return -ENODEV;

	if (!config || resizer->module_cnt == 0)
		return -EINVAL;

	ret = mtk_resizer_size_check(resizer,
			(config->in_width / resizer->module_cnt),
			config->in_height,
			(config->out_width / resizer->module_cnt),
			config->out_height);
	if (ret != 0)
		return ret;

	resizer->request_res[0] = config->in_width - config->overlap;
	resizer->request_res[1] = config->in_height;
	resizer->request_res[2] = config->out_width;
	resizer->request_res[3] = config->out_height;
	resizer->request_res[4] = config->rsz_in_format;
	resizer->event_request = 1;
	wake_up_interruptible(&resizer->event_wait);
	ret = wait_for_completion_interruptible(&resizer->cmplt);
	if (ret != 0)
		return ret;

	control_val = ((resizer->fw_output->hor_enable << 0) |
		      (resizer->fw_output->ver_enable << 1) |
		      (resizer->fw_output->scaling_up << 4) |
		      (resizer->fw_output->hor_algo << 5) |
		      (resizer->fw_output->ver_algo << 7) |
		      (resizer->fw_output->hor_table << 16) |
		      (resizer->fw_output->ver_table << 21));
	coeff_step_x = resizer->fw_output->coeff_step_x;
	coeff_step_y = resizer->fw_output->coeff_step_y;
	ibse_value = ((resizer->fw_output->pq_g << 2) |
		     (resizer->fw_output->pq_t << 7) |
		     (resizer->fw_output->pq_r << 15));
	resizer->cmdq_handle = cmdq_handle;

	/*force ibse disable in case of down scaling*/
	if ((resizer->request_res[0] >= resizer->request_res[2]) &&
		(resizer->request_res[1] >= resizer->request_res[3])) {
		config->ibse_enable = 0;
		config->tap_adapt_enable = 0;
		dev_warn(dev, "Warning!! CanNot Enable rsz IBSE in down scaling\n");
		dev_warn(dev, "Warning!! Force ibse/edge_preserve disable\n");
	}

	ret |= resizer_reg_write(resizer, RSZ_CONTROL_1,
				 control_val, CONTROL1_MASK);
	ret |= resizer_reg_write(resizer, RSZ_CONTROL_2,
				 config->ibse_enable ? PRESHP_EN : 0,
				 PRESHP_EN);
	ret |= resizer_reg_write(resizer, RSZ_CONTROL_2,
				 config->tap_adapt_enable ?
				 EDGE_PRESERVE_EN : 0, EDGE_PRESERVE_EN);
	ret |= resizer_reg_write(resizer, RSZ_INPUT_IMAGE, (config->in_width |
				 (config->in_height << 16)), 0xffffffff);
	ret |= resizer_reg_write(resizer, RSZ_OUTPUT_IMAGE, (config->out_width |
				 (config->out_height << 16)), 0xffffffff);
	ret |= resizer_reg_write(resizer, RSZ_VERT_COEFF_STEP,
				 coeff_step_y, COEFF_STEP_MASK);
	ret |= resizer_reg_write(resizer, RSZ_HORI_COEFF_STEP,
				 coeff_step_x, COEFF_STEP_MASK);
	ret |= resizer_reg_write(resizer, RSZ_TAP_ADAPT,
				 config->tap_adapt_slope, EDGE_PRESERVE_SLOPE);
	ret |= resizer_reg_write(resizer, RSZ_IBSE_PQ,
				 ibse_value, IBSE_PQ_MASK);
	ret |= resizer_reg_write(resizer, RSZ_IBSE_GAINCONTROL_1,
				config->ibse_gaincontrol_gain, PRE_SHP_GAIN);
	return ret;
}
EXPORT_SYMBOL(mtk_resizer_config_test);

/** @ingroup IP_group_resizer_external_function
 * @par Description
 *     Config the 4 partations of Resizer engine in twin mode.\n
 *     Caller need to calculate the left and right boundery values, Resizer\n
 *     horizontal and vertical integer/subpixel offset for every partations\n
 *     (You can use twin algorithm to calculate it.). If you do not need twin\n
 *     mode function, you do not need to call this function\n
 *     (e.g. computer vision path).
 * @param[in]
 *     dev: Resizer device node.
 * @param[out]
 *     cmdq_handle: CMDQ command buffer packet.
 * @param[in]
 *     config: Resizer quarter config information.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from mtk_resizer_size_check().
 * @par Boundary case and Limitation
 *     1. Same as limitation of mtk_resizer_size_check function.\n
 *     2. luma_x_sub_off just access 21 bits input.\n
 *     3. dev can not be NULL.
 * @par Error case and Error handling
 *     1. If mtk_resizer_size_check() fails, return its error code.\n
 *     2. luma_x_sub_off > 21 bits: return -EINVAL.\n
 *     3. If dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_resizer_config_quarter(const struct device *dev,
			       struct cmdq_pkt **cmdq_handle,
			       const struct mtk_resizer_quarter_config *config)
{
	struct mtk_resizer *resizer;
	int idx, ret = 0;
	u32 input_height[MTK_MM_MODULE_MAX_NUM];
	u32 input_width[MTK_MM_MODULE_MAX_NUM];
	u32 output_height[MTK_MM_MODULE_MAX_NUM];
	u32 output_width[MTK_MM_MODULE_MAX_NUM];

	if (!dev)
		return -EINVAL;

	resizer = dev_get_drvdata(dev);

	if (!resizer)
		return -ENODEV;

	if (resizer->module_cnt > 4)
		return -EINVAL;

	resizer->cmdq_handle = cmdq_handle;
	for (idx = 0; idx < resizer->module_cnt; idx++) {
		input_width[idx] = config->in_x_right[idx] -
				   config->in_x_left[idx] + 1;
		input_height[idx] = config->in_height;
		output_width[idx] = config->out_x_right[idx] -
				    config->out_x_left[idx] + 1;
		output_height[idx] = config->out_height;
		ret = mtk_resizer_size_check(resizer, input_width[idx],
					     input_height[idx],
					     output_width[idx],
					     output_height[idx]);
		if (ret != 0)
			return ret;

		ret |= resizer_single_reg_write(resizer, idx, RSZ_LUMA_HORI_INT,
						config->luma_x_int_off[idx],
						INT_OFFSET_MASK);

		if (config->luma_x_sub_off[idx] & ~SUB_OFFSET_MASK)
			return -EINVAL;

		ret |= resizer_single_reg_write(resizer, idx,
						RSZ_LUMA_HORI_SUBPIXEL,
						config->luma_x_sub_off[idx],
						SUB_OFFSET_MASK);
		ret |= resizer_single_reg_write(resizer, idx,
						RSZ_LUMA_VERT_INT,
						0, INT_OFFSET_MASK);
		ret |= resizer_single_reg_write(resizer, idx,
						RSZ_LUMA_VERT_SUBPIXEL,
						0, SUB_OFFSET_MASK);
		ret |= resizer_single_reg_write(resizer, idx, RSZ_INPUT_IMAGE,
						(input_width[idx] |
						(input_height[idx] << 16)),
						0xfffffff);
		ret |= resizer_single_reg_write(resizer, idx, RSZ_OUTPUT_IMAGE,
						(output_width[idx] |
						(output_height[idx] << 16)),
						0xfffffff);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_resizer_config_quarter);

/** @ingroup IP_group_resizer_external_function
 * @par Description
 *     Enable Resizer output bit clip function and config the threshold of\n
 *     the clip function of valid map or alpha value.
 * @param[in]
 *     dev: Resizer device node.
 * @param[out]
 *     cmdq_handle: CMDQ command buffer packet.
 * @param[in]
 *     enable: enable Resizer output bit clip function.
 * @param[in]
 *     threshold: threshold of the clip function of valid map or alpha value.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     dev can not be NULL.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_resizer_out_bit_clip(const struct device *dev,
			     struct cmdq_pkt **cmdq_handle,
			     bool enable, u8 threshold)
{
	struct mtk_resizer *resizer;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	resizer = dev_get_drvdata(dev);

	if (!resizer)
		return -ENODEV;

	resizer->cmdq_handle = cmdq_handle;
	ret |= resizer_reg_write(resizer, RSZ_OUT_BIT_CONTROL,
				 OUT_BIT_MODE_CLIP, RSZ_OUT_BIT_MODE);
	ret |= resizer_reg_write(resizer, RSZ_OUT_BIT_CONTROL,
				 threshold << CLIP_THRES_8B_SHIFT,
				 RSZ_CLIP_THRES);
	ret |= resizer_reg_write(resizer, RSZ_OUT_BIT_CONTROL,
				 enable == true ? RSZ_OUT_BIT_EN : 0,
				 RSZ_OUT_BIT_EN);

	return ret;
}
EXPORT_SYMBOL(mtk_resizer_out_bit_clip);

/** @ingroup IP_group_resizer_external_function
 * @par Description
 *     Register callback function for resizer interrupt.
 * @param[in]
 *     dev: pointer of resizer device structure.
 * @param[in]
 *     cb: resizer irq callback function pointer assigned by user.
 * @param[in]
 *     status: specify which irq status should be notified via callback.
 * @param[in]
 *     priv_data: resizer irq callback function's priv_data.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If dev is NULL, return -EINVAL.\n
 *     2. If resizer_reg_write() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_resizer_register_cb(struct device *dev, mtk_mmsys_cb cb, u32 status,
			    void *priv_data)
{
	struct mtk_resizer *resizer;
	unsigned long flags;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	resizer = dev_get_drvdata(dev);
	if (!resizer)
		return -ENODEV;
	spin_lock_irqsave(&resizer->spinlock_irq, flags);
	resizer->cb_func = cb;
	resizer->cb_data.status = status;
	resizer->cb_data.priv_data = priv_data;
	resizer->framedone = 0;
	resizer->framestart = 0;
	spin_unlock_irqrestore(&resizer->spinlock_irq, flags);
	resizer->cmdq_handle = NULL;

	if (status & 0x8FFFFFFF) {
		dev_err(dev, "ERROR:Set Resizer status %u\n", status);
		return -EINVAL;
	}

	if (status & RSZ_ERR_INT)
		ret = resizer_reg_write(resizer, RSZ_CONTROL_1,
					RSZ_ERR_INT, RSZ_ERR_INT);
	else
		ret = resizer_reg_write(resizer, RSZ_CONTROL_1,
					0, RSZ_ERR_INT);

	if (ret < 0)
		return ret;

	if (status & RSZ_FRAME_END_INT)
		ret = resizer_reg_write(resizer, RSZ_CONTROL_1,
					RSZ_FRAME_END_INT, RSZ_FRAME_END_INT);
	else
		ret = resizer_reg_write(resizer, RSZ_CONTROL_1,
					0, RSZ_FRAME_END_INT);

	if (ret < 0)
		return ret;

	if (status & RSZ_FRAME_START_INT)
		ret = resizer_reg_write(resizer, RSZ_CONTROL_1,
				RSZ_FRAME_START_INT, RSZ_FRAME_START_INT);
	else
		ret = resizer_reg_write(resizer, RSZ_CONTROL_1,
				0, RSZ_FRAME_START_INT);

	if (ret < 0)
		return ret;

	return ret;
}
EXPORT_SYMBOL(mtk_resizer_register_cb);

static int mtk_resizer_open(struct inode *inode, struct file *filp)
{
	struct mtk_resizer *resizer = container_of(inode->i_cdev,
						   struct mtk_resizer, cdev);

	if (!resizer) {
		pr_err("Cannot find device node\n");
		return -ENOMEM;
	}
	filp->private_data = resizer;

	return 0;
}

ssize_t mtk_resizer_read(struct file *filp, char __user *buf,
			 size_t count, loff_t *f_pos)
{
	struct mtk_resizer *resizer = (struct mtk_resizer *)filp->private_data;
	ssize_t ret = 0;

	if (count > (sizeof(int) * 5))
		return -EFAULT;

	if (resizer->event_request) {
		if (__copy_to_user_inatomic(buf,
					    (char *)(resizer->request_res),
					    count)) {
			ret = -EFAULT;
		} else {
			resizer->event_request = 0;
			ret = count;
		}
	}

	return ret;
}

unsigned int mtk_resizer_poll(struct file *filp, poll_table *wait)
{
	struct mtk_resizer *resizer = (struct mtk_resizer *)filp->private_data;
	unsigned int mask = 0;

	poll_wait(filp, &resizer->event_wait, wait);

	if (resizer->event_request)
		mask = POLLIN | POLLRDNORM;

	return mask;
}

static long mtk_resizer_ioctl(struct file *pFile, unsigned int cmd,
			      unsigned long arg)
{
	long ret = 0;
	struct mtk_resizer *resizer = (struct mtk_resizer *)pFile->private_data;

	switch (cmd) {

	case IOCTL_RESIZER_SCALE_CONFIG:
		ret = copy_from_user(resizer->fw_output, (void *)arg,
				     sizeof(struct rsz_scale_config));
		if (ret == 0)
			complete(&resizer->cmplt);
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

static const struct file_operations mtk_resizer_fops = {
	.owner = THIS_MODULE,
	.open = mtk_resizer_open,
	.read = mtk_resizer_read,
	.poll = mtk_resizer_poll,
	.unlocked_ioctl = mtk_resizer_ioctl,
};

static inline void rsz_unreg_chrdev(struct mtk_resizer *resizer)
{
	device_destroy(resizer->class, resizer->devno);
	class_destroy(resizer->class);
	cdev_del(&resizer->cdev);
	unregister_chrdev_region(resizer->devno, 1);
}

static inline int rsz_reg_chrdev(struct mtk_resizer *resizer,
				 const char *dev_name)
{
	int ret;
	struct class_device *class_dev = NULL;

	ret = alloc_chrdev_region(&resizer->devno, 0, 1, dev_name);
	if (ret) {
		dev_err(resizer->dev, "Failed to alloc chrdev region\n");
		return ret;
	}
	cdev_init(&resizer->cdev, &mtk_resizer_fops);
	resizer->cdev.ops = &mtk_resizer_fops;
	resizer->cdev.owner = THIS_MODULE;
	ret = cdev_add(&resizer->cdev, resizer->devno, 1);
	if ((ret) < 0) {
		unregister_chrdev_region(resizer->devno, 1);
		dev_err(resizer->dev,
			"Attatch file operation failed, %d\n", ret);
		return ret;
	}
	resizer->class = class_create(THIS_MODULE, dev_name);
	if (IS_ERR(resizer->class)) {
		ret = PTR_ERR(resizer->class);
		cdev_del(&resizer->cdev);
		unregister_chrdev_region(resizer->devno, 1);
		dev_err(resizer->dev,
			"Unable to create class, err = %d\n", ret);
		return ret;
	}
	class_dev = (struct class_device *)device_create(resizer->class, NULL,
							 resizer->devno, NULL,
							 dev_name);
	if (IS_ERR(class_dev)) {
		ret = PTR_ERR(class_dev);
		class_destroy(resizer->class);
		cdev_del(&resizer->cdev);
		unregister_chrdev_region(resizer->devno, 1);
		dev_err(resizer->dev,
			"Failed to create device: /dev/%s, err = %d",
			dev_name, ret);
		return ret;
	}

	return 0;
}

#ifdef CONFIG_MTK_DEBUGFS
static int resizer_debug_show(struct seq_file *s, void *unused)
{
	struct mtk_resizer *resizer = s->private;
	u32 reg;

	reg = readl(resizer->regs[0] + RSZ_ENABLE);

	seq_puts(s, "-------------------- RESIZER DEUBG --------------------\n");
	seq_printf(s, "ENABLE:            :%11x\n", CHECK_BIT(reg, 0) ? 1 : 0);

	reg = readl(resizer->regs[0] + RSZ_CONTROL_1);
	seq_printf(s, "HORIZONTAL_EN      :%11x\n", CHECK_BIT(reg, 0) ? 1 : 0);
	seq_printf(s, "VERTICAL_EN        :%11x\n", CHECK_BIT(reg, 1) ? 1 : 0);

	reg = readl(resizer->regs[0] + RSZ_CONTROL_2);

	seq_printf(s, "PRESHP_EN          :%11x\n", CHECK_BIT(reg, 4) ? 1 : 0);
	seq_printf(s, "EDGE_PRESERVE_EN   :%11x\n", CHECK_BIT(reg, 7) ? 1 : 0);

	reg = readl(resizer->regs[0] + RSZ_INPUT_IMAGE);
	seq_printf(s, "INTPUT_W           :%11d\n", reg & 0xFFFF);
	seq_printf(s, "INPUT_H            :%11d\n", (reg & 0xFFFF0000) >> 16);

	reg = readl(resizer->regs[0] + RSZ_OUTPUT_IMAGE);
	seq_printf(s, "OUTPUT_W           :%11d\n", reg & 0xFFFF);
	seq_printf(s, "OUTPUT_H           :%11d\n", (reg & 0xFFFF0000) >> 16);

	resizer_reg_write(resizer, RSZ_DEBUG_SEL, VALID_READY, VALID_READY);
	reg = readl(resizer->regs[0] + RSZ_DEBUG);
	seq_printf(s, "OUT_VALID          :%11x\n", CHECK_BIT(reg, 0) ? 1 : 0);
	seq_printf(s, "OUT_READY          :%11x\n", CHECK_BIT(reg, 1) ? 1 : 0);
	seq_printf(s, "IN_VALID           :%11x\n", CHECK_BIT(reg, 2) ? 1 : 0);
	seq_printf(s, "IN_READY           :%11x\n", CHECK_BIT(reg, 3) ? 1 : 0);

	seq_puts(s, "----------------------------------------------------\n");
	return 0;
}

static int resizer_debug_client_open(struct inode *inode, struct file *file)
{
	return single_open(file, resizer_debug_show, inode->i_private);
}

static const struct file_operations resizer_debug_fops = {
	.open = resizer_debug_client_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

/** @ingroup IP_group_resizer_internal_function
 * @par Description
 *     Get Resizer necessary hardware information from device tree.\n
 *     There are some information Resizer need:\n
 *     1. "hw-number": the total number of Resizer\n
 *     (Generally, the number is 1 or 4).\n
 *     2. "GCE-subsys":the CMDQ subsys base\n
 * @param[in]
 *     pdev: platform device node.
 * @return
 *     0, probe Resizer driver successfully.
 *     Otherwise, Resizer probe failed.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If there is any error during the probing flow,\n
 *     system error value will be returned.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_resizer_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_resizer *resizer;
	struct resource *regs;
	int i, err, irq;
	const char *name;
#ifdef CONFIG_MTK_DEBUGFS
	struct dentry *debug_dentry;
#endif

	resizer = devm_kzalloc(dev, sizeof(*resizer), GFP_KERNEL);
	if (!resizer)
		return -ENOMEM;

	resizer->dev = dev;
	resizer->framedone = 0;
	resizer->framestart = 0;
	err = of_property_read_u32(dev->of_node, "hw-number",
				   &resizer->module_cnt);
	if (err) {
		dev_err(dev,
			"%s: failed to read hw-number property: %d\n",
			dev->of_node->full_name, err);
		return err;
	}

#if defined(CONFIG_COMMON_CLK_MT3612)
	for (i = 0; i < resizer->module_cnt; i++) {
		resizer->clk[i] = of_clk_get(dev->of_node, i);
		if (IS_ERR(resizer->clk[i])) {
			dev_err(dev, "Failed to get Resizer%d clock\n", i);
			return PTR_ERR(resizer->clk[i]);
		}
	}
#endif
	for (i = 0; i < resizer->module_cnt; i++) {
		regs = platform_get_resource(pdev, IORESOURCE_MEM, i);
		resizer->regs[i] = devm_ioremap_resource(dev, regs);
		if (IS_ERR(resizer->regs[i])) {
			dev_err(dev, "Failed to map resizer registers\n");
			return PTR_ERR(resizer->regs);
		}
		resizer->cmdq_offset[i] = (u32) regs->start & 0xffff;

		irq = platform_get_irq(pdev, i);
		if (irq < 0) {
			dev_err(dev, "Failed to get resizer %d irq.\n", i);
			return irq;
		}

		err = devm_request_irq(dev, irq, mtk_resizer_irq_handler,
			       IRQF_TRIGGER_LOW, dev_name(dev), resizer);
		if (err < 0) {
			dev_err(dev, "Failed to request resizer irq %d: %d\n",
				irq, err);
			return err;
		}
		resizer->irq[i] = irq;
	}

	err = of_property_read_u32_array(dev->of_node, "gce-subsys",
					 resizer->cmdq_subsys,
					 resizer->module_cnt);
	if (err) {
		dev_err(dev,
			"%s: failed to read GCE-subsys property: %d\n",
			dev->of_node->full_name, err);
		return err;
	}

	spin_lock_init(&resizer->spinlock_irq);
	init_waitqueue_head(&resizer->event_wait);
	resizer->event_request = 0;
	init_completion(&resizer->cmplt);
	resizer->fw_output = kzalloc(sizeof(*resizer->fw_output), GFP_KERNEL);
	if (!resizer->fw_output)
		return -ENOMEM;

	if (of_property_read_string(dev->of_node, "dev-name", &name) == 0) {
		err = rsz_reg_chrdev(resizer, name);
		if (err)
			return err;
	} else {
		dev_err(dev, "Resizer driver do not support resize function\n"
			"without Resizer firmware, please check Resizer\n"
			"firmware relatived setting\n");
		return -ELIBACC;
	}

#if defined(CONFIG_COMMON_CLK_MT3612)
	pm_runtime_enable(dev);
#endif
	platform_set_drvdata(pdev, resizer);

#ifdef CONFIG_MTK_DEBUGFS
	if (resizer->module_cnt > 1 || !mtk_debugfs_root)
		goto debugfs_done;
	/* debug info create */
	if (!mtk_resizer_debugfs_root)
		mtk_resizer_debugfs_root = debugfs_create_dir("resizer",
							mtk_debugfs_root);

	if (!mtk_resizer_debugfs_root) {
		dev_err(dev, "failed to create resizer debugfs root directory.\n");
		goto debugfs_done;
	}

	debug_dentry = debugfs_create_file(pdev->name, 0664,
					   mtk_resizer_debugfs_root,
					   resizer, &resizer_debug_fops);
debugfs_done:
	if (!debug_dentry)
		dev_err(dev, "Failed to create resizer debugfs %s\n",
			pdev->name);
#endif
	dev_dbg(dev, "resizer probe done!\n");

	return 0;
}

/** @ingroup IP_group_resizer_internal_function
 * @par Description
 *     Do nothing.
 * @param[in]
 *     pdev: platform device node.
 * @return
 *     0, remove Resizer driver successfully.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_resizer_remove(struct platform_device *pdev)
{
	struct mtk_resizer *resizer = platform_get_drvdata(pdev);

	if (resizer->class)
		rsz_unreg_chrdev(resizer);
	kfree(resizer->fw_output);
#if defined(CONFIG_COMMON_CLK_MT3612)
	pm_runtime_disable(&pdev->dev);
#endif

#ifdef CONFIG_MTK_DEBUGFS
	debugfs_remove_recursive(mtk_resizer_debugfs_root);
	mtk_resizer_debugfs_root = NULL;
#endif

	return 0;
}

/** @ingroup IP_group_resizer_internal_struct
 * @brief Resizer open framework device ID.\n
 * This structure is used to attach specific names to\n
 * platform device for use with device tree.
 */
static const struct of_device_id resizer_driver_dt_match[] = {
	{.compatible = "mediatek,mt3612-resizer"},
	{},
};

MODULE_DEVICE_TABLE(of, resizer_driver_dt_match);

/** @ingroup IP_group_resizer_internal_struct
 * @brief Resizer platform driver structure.\n
 * This structure is used to register itself.
 */
struct platform_driver mtk_resizer_driver = {
	.probe = mtk_resizer_probe,
	.remove = mtk_resizer_remove,
	.driver = {
		   .name = "mediatek-resizer",
		   .owner = THIS_MODULE,
		   .of_match_table = resizer_driver_dt_match,
		   },
};

module_platform_driver(mtk_resizer_driver);
MODULE_LICENSE("GPL");
