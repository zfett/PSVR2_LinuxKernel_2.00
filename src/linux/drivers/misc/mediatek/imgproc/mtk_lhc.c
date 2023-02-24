/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	Houlong Wei <houlong.wei@mediatek.com>
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
 * @file mtk_lhc.c
 * This LHC(Local Histogram Counter) driver is used to control
 * MTK LHC hardware module.\n
 *
 */
/**
 * @defgroup IP_group_lhc LHC
 *   There are total 4 LHC HW in MT3612.\n
 *
 *   @{
 *       @defgroup IP_group_lhc_external EXTERNAL
 *         The external API document for LHC. \n
 *
 *         @{
 *            @defgroup IP_group_lhc_external_function 1.function
 *              none.
 *            @defgroup IP_group_lhc_external_struct 2.structure
 *              none.
 *            @defgroup IP_group_lhc_external_typedef 3.typedef
 *              none.
 *            @defgroup IP_group_lhc_external_enum 4.enumeration
 *              none.
 *            @defgroup IP_group_lhc_external_def 5.define
 *              none.
 *         @}
 *
 *       @defgroup IP_group_lhc_internal INTERNAL
 *         The internal API document for sysirq. \n
 *
 *         @{
 *            @defgroup IP_group_lhc_internal_function 1.function
 *              Internal function for Linux interrupt framework and module init.
 *            @defgroup IP_group_lhc_internal_struct 2.structure
 *              Internal structure used for sysirq.
 *            @defgroup IP_group_lhc_internal_typedef 3.typedef
 *              none. Follow Linux coding style, no typedef used.
 *            @defgroup IP_group_lhc_internal_enum 4.enumeration
 *              none. No enumeration in sysirq driver.
 *            @defgroup IP_group_lhc_internal_def 5.define
 *              none. No extra define in sysirq driver.
 *         @}
 *   @}
 */
#include <asm-generic/io.h>
#include <linux/cdev.h>
#include <linux/clk.h>
#include <linux/compiler.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#ifdef CONFIG_MTK_DEBUGFS
#include <linux/bitops.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#endif
#include <soc/mediatek/mtk_lhc.h>
#include "mtk_lhc_reg.h"

/** @ingroup IP_group_lhc_internal_def
 * @brief LHC driver internal definitions
 * @{
 */
#define LHC_INT_ALL	(FRAME_DONE_INT_EN | READ_FINISH_INT_EN | \
			IF_UNFINISH_INT_EN | OF_UNFINISH_INT_EN | \
			SRAM_RW_ERR_INT_EN | APB_RW_ERR_INT_EN)
/**
 * @}
 */

/** @ingroup IP_group_lhc_internal_struct
 * @brief Struct keeping LHC driver data
 */
struct mtk_lhc {
	struct device			*dev;
	/** LHC clock node */
	struct clk			*clk[MTK_MM_MODULE_MAX_NUM];
	/** LHCregister base */
	void __iomem			*regs[MTK_MM_MODULE_MAX_NUM];
	/** LHC irq no */
	int				irq[MTK_MM_MODULE_MAX_NUM];
	/** spinlock for irq handler */
	spinlock_t			lock_irq;
	/** gce subsys prefix of LHC*/
	u32				subsys[MTK_MM_MODULE_MAX_NUM];
	/** LHC register offset */
	u32				subsys_offset[MTK_MM_MODULE_MAX_NUM];
	/** LHC HW amount in running */
	int				module_cnt;
	/** LHC read HIST flag */
	bool				read_hist;
	mtk_mmsys_cb			cb_func;
	struct mtk_mmsys_cb_data	cb_data;
	u32				cb_status;
	u32				frame_done;
	u32				read_finish;
#ifdef CONFIG_MTK_DEBUGFS
	struct dentry			*debugfs;
	const char			*file_name;
#endif
};

/** @ingroup IP_group_lhc_internal_struct
 * @brief Basically used for read/write register.\n
 * To keep register base address, offset & cmdq subsys prefix info, etc.
 */
struct reg_info {
	/** register base */
	void __iomem *regs;
	/** gce subsys prefix */
	u32 subsys;
	/** register offset */
	u32 subsys_offset;
	/** cmdq packet handle of given memory address to contain cmds */
	struct cmdq_pkt *pkt;
};

/** @ingroup IP_group_lhc_internal_function
 * @par Description
 *     Write partial of register use CPU write registers
 * @param[in]
 *     addr: register address
 * @param[in]
 *     value: register value
 * @param[in]
 *     mask: mask bit that need to modify register
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_lhc_cpu_write_mask(void __iomem *addr, u32 value, u32 mask)
{
	u32 temp;

	temp = readl(addr) & ~mask;
	temp |= value & mask;
	writel(temp, addr);
}

/**
 * @ingroup IP_group_lhc_internal_function
 * @par Description
 *     Write partial of register with  cmdq or not.
 * @param[out]
 *     rif: pointer of struct reg_info.\n
 *     rif->pkt is cmdq handle; If rif.pkt is null, use cpu to write register.\n
 *     If rif.pkt exists, use cmdq to write register.
 * @param[in]
 *     value: data value for writing to register.
 * @param[in]
 *     offset: offset position from base address of register.
 * @param[in]
 *     mask: specified bits to be modified only.
 * @return
 *     0, write successfully.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_lhc_reg_write_mask(struct reg_info *rif, u32 value,
				  u32 offset, u32 mask)
{
	int ret = 0;

	if (rif->pkt) {
		ret = cmdq_pkt_write_value(rif->pkt, rif->subsys,
					   rif->subsys_offset | offset,
					   value, mask);
	} else
		mtk_lhc_cpu_write_mask(rif->regs + offset, value, mask);

	return ret;
}

/**
 * @ingroup IP_group_lhc_internal_function
 * @par Description
 *     Write partial of register with cmdq or not.
 * @param[out]
 *     rif: pointer of struct reg_info.\n
 *     rif->pkt is cmdq handle; If rif.pkt is null, use cpu to write register.\n
 *     If rif.pkt exists, use cmdq to write register.
 * @param[in]
 *     value: data value for writing to register.
 * @param[in]
 *     offset: offset position from base address of register.
 * @return
 *     0, write successfully.\n
 *     error code from mtk_lhc_reg_write_mask().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If mtk_lhc_reg_write_mask() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_lhc_reg_write(struct reg_info *rif, u32 value, u32 offset)
{
	int ret = 0;

	if (rif->pkt) {
		ret = cmdq_pkt_write_value(rif->pkt, rif->subsys,
				   rif->subsys_offset | offset,
				   value, 0xffffffff);
	} else
		writel(value, rif->regs + offset);

	return ret;
}

/**
 * @ingroup IP_group_lhc_internal_function
 * @par Description
 *     YUV To RGB conversion use programable table.
 * @param[out]
 *     rif: pointer of struct reg_info.
 * @param[in]
 *     coff: pointer to Y2R matrix 3x3 table.
 * @param[in]
 *     pre_add: pointer to Y2R pre-adding table.
 * @param[in]
 *     post_add: pointer to Y2R post-adding table.
 * @return
 *     0, successfully.\n
 *     -EINVAL, null coff/pre_add/post_add pointer.\n
 *     error code from mtk_lhc_reg_write().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If coff or pre_ad or post_add is null, return -EINVAL.
 *     2. If mtk_lhc_reg_write()/_mask() fails, return their error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_lhc_ext_color_transform_matrix(struct reg_info *rif,
		u32 (*coff)[3][3], u32 (*pre_add)[3], u32 (*post_add)[3])
{
	int ret = 0;
	u32 v;

	if (!coff || !pre_add || !post_add)
		return -EINVAL;

	v = (*pre_add)[0] & Y2R_PRE_ADD_0_S;
	v |= ((*pre_add)[1] << 16) & Y2R_PRE_ADD_1_S;
	ret |= mtk_lhc_reg_write(rif, v,  LHC_Y2R_00);

	v = (*pre_add)[2] & Y2R_PRE_ADD_2_S;
	v |= ((*coff)[0][0] << 16) & Y2R_C00_S;
	ret |= mtk_lhc_reg_write(rif, v, LHC_Y2R_01);

	v = (*coff)[0][1] & Y2R_C01_S;
	v |= ((*coff)[0][2] << 16) & Y2R_C02_S;
	ret |= mtk_lhc_reg_write(rif, v, LHC_Y2R_02);

	v = (*coff)[1][0] & Y2R_C10_S;
	v |= ((*coff)[1][1] << 16) & Y2R_C11_S;
	ret |= mtk_lhc_reg_write(rif, v, LHC_Y2R_03);

	v = (*coff)[1][2] & Y2R_C12_S;
	v |= ((*coff)[2][0] << 16) & Y2R_C20_S;
	ret |= mtk_lhc_reg_write(rif, v, LHC_Y2R_04);

	v = (*coff)[2][1] & Y2R_C21_S;
	v |= ((*coff)[2][2] << 16) & Y2R_C22_S;
	ret |= mtk_lhc_reg_write(rif, v, LHC_Y2R_05);

	v = (*post_add)[0] & Y2R_POST_ADD_0_S;
	v |= ((*post_add)[1] << 16) & Y2R_POST_ADD_1_S;
	ret |= mtk_lhc_reg_write(rif, v, LHC_Y2R_06);

	v = ((*post_add)[2] << 16) & Y2R_POST_ADD_2_S;
	ret |= mtk_lhc_reg_write_mask(rif, v, LHC_Y2R_07, Y2R_POST_ADD_2_S);
	ret |= mtk_lhc_reg_write_mask(rif, Y2R_EXT_TABLE_EN, LHC_Y2R_07,
		Y2R_EXT_TABLE_EN);

	return ret;
}

/**
 * @ingroup IP_group_lhc_internal_function
 * @par Description
 *     YUV To RGB conversion use internal table.
 * @param[out]
 *     rif: pointer of struct reg_info.
 * @param[in]
 *     int_matrix: internal table select by yuv color space & range.
 * @return
 *    0, write successfully.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If int_matrix is wrong number, return -EINVAL
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_lhc_int_color_transform_matrix(struct reg_info *rif,
		enum mtk_lhc_int_table int_matrix)
{
	if (int_matrix < LHC_INT_TBL_BT2020L || int_matrix > LHC_INT_TBL_MAX)
		return -EINVAL;

	mtk_lhc_reg_write_mask(rif, 0, LHC_Y2R_07, Y2R_EXT_TABLE_EN);
	mtk_lhc_reg_write_mask(rif, (u32)int_matrix << 1, LHC_Y2R_07,
			       Y2R_INT_TABLE_SEL);

	return 0;
}

/** @ingroup IP_group_lhc_external_function
 * @par Description
 *     Register callback function for LHC interrupt.
 * @param[in]
 *     dev: device node pointer of LHC.
 * @param[in]
 *     cb: LHC irq callback function pointer assigned by user.
 * @param[in]
 *     status: Specify which irq status should be notified via callback.\n
 *             It's combination of LHC_CB_STA_xxx.
 * @param[in]
 *     priv_data: LHC irq callback function's priv_data.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     dev can not be NULL.
 * @par Error case and Error handling
 *     1. If dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_lhc_register_cb(const struct device *dev,
			mtk_mmsys_cb cb, u32 status, void *priv_data)
{
	struct mtk_lhc *lhc;
	unsigned long flags;
	int i, cnt;

	if (!dev)
		return -EINVAL;
	if (!cb)
		status = 0;
	lhc = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(lhc))
		return -EINVAL;

	spin_lock_irqsave(&lhc->lock_irq, flags);
	lhc->cb_func = cb;
	lhc->cb_data.priv_data = priv_data;
	lhc->frame_done = 0;
	lhc->read_finish = 0;
	lhc->cb_status = status;
	/* Force to enable frame done interrupt because the driver relies on
	 * this interrupt signal to determine whether the histogram data can
	 * be read.
	 */
	status |= LHC_CB_STA_FRAME_DONE;

	cnt = min_t(int, lhc->module_cnt, ARRAY_SIZE(lhc->regs));
	for (i = 0; i < cnt; i++) {
		if (status & LHC_CB_STA_READ_FINISH) {
			mtk_lhc_cpu_write_mask(lhc->regs[i] + LHC_SRAM_READ_CNT,
						LHC_BLK_Y_NUM * LHC_X_BIN_NUM,
						HIST_SRAM_TOTAL_CNT);
		}
		mtk_lhc_cpu_write_mask(lhc->regs[i] + LHC_INTEN, status,
					LHC_INT_ALL);
	}
	spin_unlock_irqrestore(&lhc->lock_irq, flags);

	return 0;
}
EXPORT_SYMBOL(mtk_lhc_register_cb);

/**
 * @ingroup IP_group_lhc_external_function
 * @par Description
 *     Config each partition of LHC, such as input frame size, active window,
 *     block number, block width/height and so on.
 * @param[in]
 *     dev: device node pointer of LHC.
 * @param[in]
 *     config: array porinter of struct mtk_lhc_slice, which contains the\n
 *     frame information of each partition.
 * @param[out]
 *     handle: cmdq packet handle of given memory address to contain cmds.\n
 *     If null, use cpu to write register.\n
 *     If exists, use cmdq to write register.
 * @return
 *     0, config successfully.\n
 *     -EINVAL, null dev pointer.\n
 *     -EINVAL, null config pointer.\n
 *     -EINVAL, wrong frame size in config[].\n
 *     error code from mtk_lhc_reg_write().\n
 *     error code from mtk_lhc_reg_write_mask().\n
 *     error code from mtk_lhc_ctrl().
 * @par Boundary case and Limitation
 *     w in config[] should align to display pipe's width and in range 1~4320.\n
 *     h in config[] should align to display pipe's height and in range 1~2205.
 * @par Error case and Error handling
 *     1. If dev is null, return -EINVAL.\n
 *     2. If config is null, return -EINVAL.\n
 *     3. Wrong frame size in config[], return -EINVAL.\n
 *     4. If mtk_lhc_reg_write() fails, return its error code.\n
 *     5. If mtk_lhc_reg_write_mask() fails, return its error code.\n
 *     6. If mtk_lhc_ctrl() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_lhc_config_slices(const struct device *dev,
			  struct mtk_lhc_slice (*config)[4],
			  struct cmdq_pkt **handle)
{
	struct mtk_lhc_slice *s;
	struct mtk_lhc *lhc;
	struct reg_info rif;
	int i, cnt;
	int ret = 0;
	u32 blk_w, blk_h;

	if (!dev || !config)
		return -EINVAL;

	lhc = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(lhc))
		return -EINVAL;

	cnt = min_t(int, lhc->module_cnt, 4);
	for (i = 0; i < cnt; i++) {
		s = &(*config)[i];
		if ((s->w == 0 || s->w > 4320) || (s->h == 0 || s->h > 2205)) {
			dev_err(dev, "lhc[%d] invalid frame size %ux%u\n",
				i, s->w, s->h);
			return -EINVAL;
		}
		if ((s->end - s->start + 1) % LHC_BLK_X_NUM != 0) {
			dev_err(dev,
				"lhc[%d] frame width may not be divisible by 16.\n"
				"Active window start = %u, end = %u\n",
				i, s->start, s->end);
			return -EINVAL;
		}
	}

	for (i = 0; i < cnt; i++) {
		if (!handle)
			rif.pkt = NULL;
		else
			rif.pkt = handle[i];
		rif.regs = lhc->regs[i];
		rif.subsys_offset = lhc->subsys_offset[i];
		rif.subsys = lhc->subsys[i];

		s = &(*config)[i];
		blk_w = (s->end - s->start + 1) / LHC_BLK_X_NUM;
		/*
		 * Block height will be rounded up to integer by HW
		 * automatically when frame height is indivisible by 16.
		 *
		 * Our SW just set blk_h the integer quotient of height and 16,
		 * which drops the remainder.
		 */
		blk_h = s->h / LHC_BLK_Y_NUM;
		dev_dbg(dev, "lhc[%d] wxh=%ux%u,act_win:%u~%u,blk:%ux%u\n",
			i, s->w, s->h, s->start, s->end, blk_w, blk_h);

		/* config frame size */
		ret |= mtk_lhc_reg_write(&rif, (s->w << 16) | s->h, LHC_SIZE);
		/* config active window position & blk*/
		ret |= mtk_lhc_reg_write(&rif, (s->end << 16) | s->start,
					LHC_DRE_BLOCK_INFO_00);
		ret |= mtk_lhc_reg_write(&rif,
					(LHC_BLK_Y_NUM << 16) | LHC_BLK_X_NUM,
					LHC_DRE_BLOCK_INFO_01);
		ret |= mtk_lhc_reg_write(&rif, (blk_h << 16) | blk_w,
					LHC_DRE_BLOCK_INFO_02);

		/* default set input swap */
		ret |= mtk_lhc_reg_write_mask(&rif, LHC_INPUT_SWAP,  LHC_CFG,
					      LHC_INPUT_SWAP);
		/* default set frame done interrupt enable */
		ret |= mtk_lhc_reg_write_mask(&rif, FRAME_DONE_INT_EN,
					      LHC_INTEN, FRAME_DONE_INT_EN);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_lhc_config_slices);

/**
 * @ingroup IP_group_lhc_external_function
 * @par Description
 *     Config input frame size, active window, block number, block width/height
 *     and so on. It's used in the case that all slices are non-overlapped.
 * @param[in]
 *     dev: device node pointer of LHC.
 * @param[in]
 *     w: LCM's total width of all partitions.
 * @param[in]
 *     h: LCM's height.
 * @param[out]
 *     handle: cmdq packet handle of given memory address to contain cmds.\n
 *     If null, use cpu to write register.\n
 *     If exists, use cmdq to write register.
 * @return
 *     0, config successfully.\n
 *     -EINVAL, null dev pointer.\n
 *     -EINVAL, wrong w/h parameter value.\n
 *     error code from mtk_lhc_config_slices().
 * @par Boundary case and Limitation
 *     Parameter, w, should align to display pipe's width and in range 1~4320.\n
 *     Parameter, h, should align to display pipe's height and in range 1~2205.
 * @par Error case and Error handling
 *     1. If dev is null, return -EINVAL.\n
 *     2. Wrong width/height, return -EINVAL.\n
 *     3. If mtk_lhc_config_slices() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_lhc_config(const struct device *dev, unsigned int w, unsigned int h,
		   struct cmdq_pkt **handle)
{
	struct mtk_lhc_slice s[4];
	struct mtk_lhc *lhc;
	int i, ret, cnt;

	if (!dev)
		return -EINVAL;
	if ((w == 0 || w > 4320) || (h == 0 || h > 2205)) {
		dev_err(dev, "lhc invalid frame size %ux%u\n", w, h);
		return -EINVAL;
	}

	lhc = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(lhc))
		return -EINVAL;

	cnt = min_t(int, lhc->module_cnt, 4);

	w = w / cnt;
	for (i = 0; i < lhc->module_cnt; i++) {
		s[i].w = w;
		s[i].h = h;
		s[i].start = 0;
		s[i].end = w - 1;
	}

	ret = mtk_lhc_config_slices(dev, &s, handle);

	return ret;
}
EXPORT_SYMBOL(mtk_lhc_config);

/**
 * @ingroup IP_group_lhc_external_function
 * @par Description
 *     Start LHC operation.
 * @param[in]
 *     dev: device node pointer of LHC.
 * @param[out]
 *     handle: cmdq packet handle of given memory address to contain cmds.\n
 *     If null, use cpu to write register.\n
 *     If exists, use cmdq to write register.
 * @return
 *     0, set start done.\n
 *     -EINVAL, null dev pointer.\n
 *     error code from mtk_lhc_reg_write().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If dev is null, return -EINVAL.\n
 *     2. If mtk_lhc_reg_write() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_lhc_start(const struct device *dev, struct cmdq_pkt **handle)
{
	struct mtk_lhc *lhc;
	struct reg_info rif;
	int i, cnt;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	lhc = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(lhc))
		return -EINVAL;

	cnt = min_t(int, lhc->module_cnt, ARRAY_SIZE(lhc->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle)
			rif.pkt = NULL;
		else
			rif.pkt = handle[i];

		rif.regs = lhc->regs[i];
		rif.subsys_offset = lhc->subsys_offset[i];
		rif.subsys = lhc->subsys[i];

		ret |= mtk_lhc_reg_write(&rif, REG_LHC_EN, LHC_EN);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_lhc_start);

/**
 * @ingroup IP_group_lhc_external_function
 * @par Description
 *     Stop LHC operation.
 * @param[in]
 *     dev: device node pointer of LHC.
 * @param[out]
 *     handle: cmdq packet handle of given memory address to contain cmds.\n
 *     If null, use cpu to write register.\n
 *     If exists, use cmdq to write register.
 * @return
 *     0, set stop done.\n
 *     -EINVAL, null dev pointer.\n
 *     error code from mtk_lhc_reg_write().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If dev is null, return -EINVAL.\n
 *     2. If mtk_lhc_reg_write() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_lhc_stop(const struct device *dev, struct cmdq_pkt **handle)
{
	struct mtk_lhc *lhc;
	struct reg_info rif;
	int i, cnt;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	lhc = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(lhc))
		return -EINVAL;

	cnt = min_t(int, lhc->module_cnt, ARRAY_SIZE(lhc->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle)
			rif.pkt = NULL;
		else
			rif.pkt = handle[i];

		rif.regs = lhc->regs[i];
		rif.subsys_offset = lhc->subsys_offset[i];
		rif.subsys = lhc->subsys[i];
		ret |= mtk_lhc_reg_write(&rif, 0x0, LHC_EN);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_lhc_stop);
/**
 * @ingroup IP_group_lhc_external_function
 * @par Description
 *     Trigger reset of LHC.
 * @param[in]
 *     dev: device node pointer of LHC.
 * @param[out]
 *     handle: cmdq packet handle of given memory address to contain cmds.\n
 *     If null, use cpu to write register.\n
 *     If exists, use cmdq to write register.
 * @return
 *     0, set reset done.\n
 *     -EINVAL, null dev pointer.\n
 *     error code from mtk_lhc_reg_write().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If dev is null, return -EINVAL.\n
 *     2. If mtk_lhc_reg_write() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_lhc_reset(const struct device *dev, struct cmdq_pkt **handle)
{
	struct mtk_lhc *lhc;
	struct reg_info rif;
	int i, cnt;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	lhc = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(lhc))
		return -EINVAL;

	cnt = min_t(int, lhc->module_cnt, ARRAY_SIZE(lhc->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle)
			rif.pkt = NULL;
		else
			rif.pkt = handle[i];

		rif.regs = lhc->regs[i];
		rif.subsys_offset = lhc->subsys_offset[i];
		rif.subsys = lhc->subsys[i];
		ret |= mtk_lhc_reg_write(&rif, REG_LHC_RESET, LHC_RESET);
		ret |= mtk_lhc_reg_write(&rif, 0x0, LHC_RESET);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_lhc_reset);

/** @ingroup IP_group_lhc_external_function
 * @par Description
 *     Turn On LHC Clock.
 * @param[out]
 *     dev: device node pointer of lhc.
 * @return
 *     0, successful to turn on clock.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from pm_runtime_get_sync().\n
 *     error code from clk_prepare_enable().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If dev is null, return -EINVAL.\n
 *     2. If pm_runtime_get_sync() fails, return its error code.\n
 *     3. If clk_prepare_enable() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_lhc_power_on(struct device *dev)
{
	struct mtk_lhc *lhc;
#ifdef CONFIG_COMMON_CLK_MT3612
	int i, cnt;
#endif
	int ret = 0;

	if (!dev) {
		pr_err("%s:null dev!\n", __func__);
		return -EINVAL;
	}
	lhc = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(lhc))
		return -EINVAL;

#ifdef CONFIG_COMMON_CLK_MT3612
	ret = pm_runtime_get_sync(dev);
	if (ret < 0) {
		dev_err(dev, "Failed to get power domain, ret=%d\n", ret);
		return ret;
	}

	cnt = min_t(int, lhc->module_cnt, ARRAY_SIZE(lhc->clk));
	for (i = 0; i < cnt; i++) {
		if (lhc->clk[i]) {
			ret = clk_prepare_enable(lhc->clk[i]);
			if (ret) {
				dev_err(dev, "Failed to enable clock[%d]:%d\n",
					i, ret);
				break;
			}
		}
	}

	if (ret) {
		while (--i >= 0)
			clk_disable_unprepare(lhc->clk[i]);
		pm_runtime_put_sync(dev);
	}
#endif

	return ret;
}
EXPORT_SYMBOL(mtk_lhc_power_on);

/** @ingroup IP_group_lhc_external_function
 * @par Description
 *     Turn Off LHC Clock.
 * @param[out]
 *     dev: device node pointer of lhc.
 * @return
 *     0, successful to turn on clock.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from clk_prepare_enable().\n
 *     error code from pm_runtime_put_sync().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If dev is null, return -EINVAL.\n
 *     2. If clk_prepare_enable() fails, return its error code.\n
 *     3. If pm_runtime_put_sync() fails, return its error code.\n
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_lhc_power_off(struct device *dev)
{
	struct mtk_lhc *lhc;
#ifdef CONFIG_COMMON_CLK_MT3612
	int i, cnt;
#endif
	int ret = 0;

	if (!dev)
		return -EINVAL;

	lhc = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(lhc))
		return -EINVAL;

#ifdef CONFIG_COMMON_CLK_MT3612
	cnt = min_t(int, lhc->module_cnt, ARRAY_SIZE(lhc->clk));
	for (i = 0; i < cnt; i++) {
		if (lhc->clk[i])
			clk_disable_unprepare(lhc->clk[i]);
	}

	ret = pm_runtime_put_sync(dev);
	if (ret < 0) {
		dev_err(dev, "Failed to put power domain, ret=%d\n", ret);
		return ret;
	}
#endif

	return ret;
}
EXPORT_SYMBOL(mtk_lhc_power_off);

/** @ingroup IP_group_lhc_external_function
 * @par Description
 *     Set LHC color transform
 * @param[in]
 *     dev: pointer of lhc device structure.
 * @param[in]
 *     color_trans: pointer of color transfer structure.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *     to register directly. Otherwise, write the configuration to\n
 *     command queue packet.
 * @return
 *     0, successful to set color transform.\n
 *     -EINVAL, null dev pointer or null color_trans pointer.
 *     error code from mtk_lhc_int_color_transform_matrix().\n
 *     error code from mtk_lhc_reg_write_mask().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If dev is null or color_trans is null, return -EINVAL.
 *     2. If mtk_lhc_int/ext_color_transform_matrix() fails, return its\n
 *        error code.\n
 *     3. If mtk_lhc_reg_write_mask() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_lhc_set_color_transform(const struct device *dev,
				struct mtk_lhc_color_trans *color_trans,
				struct cmdq_pkt **handle)
{
	struct mtk_lhc *lhc;
	struct reg_info rif;
	int i, cnt, ret = 0;

	if (!dev || !color_trans)
		return -EINVAL;

	lhc = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(lhc))
		return -EINVAL;

	cnt = min_t(int, lhc->module_cnt, ARRAY_SIZE(lhc->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle)
			rif.pkt = NULL;
		else
			rif.pkt = handle[i];

		rif.regs = lhc->regs[i];
		rif.subsys_offset = lhc->subsys_offset[i];
		rif.subsys = lhc->subsys[i];


		if (!color_trans->enable) {
			mtk_lhc_reg_write_mask(&rif, 0, LHC_Y2R_07, Y2R_EN);
			continue;
		}

		if (color_trans->external_matrix) {
			ret |= mtk_lhc_ext_color_transform_matrix(&rif,
					&color_trans->coff,
					&color_trans->pre_add,
					&color_trans->post_add);
		} else {
			ret |= mtk_lhc_int_color_transform_matrix(&rif,
					color_trans->int_matrix);
		}

		ret |= mtk_lhc_reg_write_mask(&rif, 1, LHC_Y2R_07, Y2R_EN);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_lhc_set_color_transform);

/** @ingroup IP_group_lhc_internal_function
 * @par Description
 *     lhc interrupt handler.
 * @param[in]
 *     irq: irq number.
 * @param[out]
 *     dev_id: pointer used to save device node pointer of lhc.
 * @return
 *     IRQ_HANDLED, irq handler execute done\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static irqreturn_t mtk_lhc_irq_handler(int irq, void *dev_id)
{
	struct mtk_lhc *lhc = dev_id;
	u32 i, cb_status;
	int cnt;
	u32 int_sta = 0;
	unsigned long flags;
	mtk_mmsys_cb cb;
	struct mtk_mmsys_cb_data cb_data;
	const u32 frame_done_val = (1 << lhc->module_cnt) - 1;

	cnt = min_t(int, lhc->module_cnt, ARRAY_SIZE(lhc->regs));
	for (i = 0; i < cnt; i++) {
		if  (irq == lhc->irq[i]) {
			int_sta = readl(lhc->regs[i] + LHC_INTSTA);
			writel(~int_sta, lhc->regs[i] + LHC_INTSTA);
			break;
		}
	}

	if (i == lhc->module_cnt)
		return IRQ_NONE;

	spin_lock_irqsave(&lhc->lock_irq, flags);
	cb_data = lhc->cb_data;
	cb = lhc->cb_func;
	cb_status = lhc->cb_status;
	spin_unlock_irqrestore(&lhc->lock_irq, flags);

	if ((cb_status & int_sta) && cb) {
		bool frame_done_cb = false, read_finish_cb = false;

		spin_lock_irqsave(&lhc->lock_irq, flags);
		if (cb_status & int_sta & LHC_CB_STA_FRAME_DONE) {
			lhc->frame_done |= BIT(i);
			if (lhc->frame_done == frame_done_val) {
				lhc->frame_done = 0;
				lhc->read_hist = true;
				frame_done_cb = true;
			}
			int_sta &= ~LHC_CB_STA_FRAME_DONE;
		}

		if (cb_status & int_sta & LHC_CB_STA_READ_FINISH) {
			lhc->read_finish |= BIT(i);
			if (lhc->read_finish == frame_done_val) {
				lhc->read_finish = 0;
				read_finish_cb = true;
			}
			int_sta &= ~LHC_CB_STA_READ_FINISH;
		}
		spin_unlock_irqrestore(&lhc->lock_irq, flags);

		if (int_sta > 0) {
			cb_data.part_id = i;
			cb_data.status = int_sta;
			cb(&cb_data);
		}

		if (read_finish_cb) {
			cb_data.part_id = frame_done_val;
			cb_data.status = LHC_CB_STA_READ_FINISH;
			cb(&cb_data);
		}

		if (frame_done_cb) {
			cb_data.part_id = frame_done_val;
			cb_data.status = LHC_CB_STA_FRAME_DONE;
			cb(&cb_data);
		}
	} else if (int_sta & LHC_CB_STA_FRAME_DONE) {
		spin_lock_irqsave(&lhc->lock_irq, flags);
		lhc->frame_done |= BIT(i);
		if (lhc->frame_done == frame_done_val)
			lhc->read_hist = true;
		spin_unlock_irqrestore(&lhc->lock_irq, flags);
	}

	return IRQ_HANDLED;
}

/** @ingroup IP_group_lhc_external_function
 * @par Description
 *     Read LHC frame done status.
 * @param[in]
 *     dev: pointer of lhc device structure.
 * @param[out]
 *     frame_done: frame done status.
 * @return
 *     0, successful read hist.\n
 *     -EINVAL, null dev pointer or null frame_done pointer.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If dev is null or frame_done is null, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_lhc_read_frame_done_status(const struct device *dev, bool *frame_done)
{
	struct mtk_lhc *lhc;
	unsigned long flags;

	if (!dev || !frame_done)
		return -EINVAL;

	lhc = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(lhc))
		return -EINVAL;

	spin_lock_irqsave(&lhc->lock_irq, flags);
	*frame_done = lhc->read_hist;
	spin_unlock_irqrestore(&lhc->lock_irq, flags);

	return 0;
}
EXPORT_SYMBOL(mtk_lhc_read_frame_done_status);

/** @ingroup IP_group_lhc_internal_function
 * @par Description
 *     Read histogram data of one of RGB components.
 * @param[in]
 *     addr0: register address to read from.
 * @param[in]
 *     addr1: register address to read from.
 * @param[in]
 *     addr2: register address to read from.
 * @param[in]
 *     m: LHC module (slice) index.
 * @param[out]
 *     d0: pointer to histogram data array.
 * @param[out]
 *     d1: pointer to histogram data array.
 * @param[out]
 *     d2: pointer to histogram data array.
 * @return
 *     0, success.\n
 *     -EINVAL, null addr pointer or null d pointer.
 *     -EINVAL, m is equal to or larger than MTK_MM_MODULE_MAX_NUM.
 * @par Boundary case and Limitation
 *     m ranges from 0 to MTK_MM_MODULE_MAX_NUM - 1.
 * @par Error case and Error handling
 *     If addr is null or m is null, return -EINVAL.
 *     If m is equal to or larger than MTK_MM_MODULE_MAX_NUM, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_lhc_read_3data(
	const void __iomem *addr0,
	const void __iomem *addr1,
	const void __iomem *addr2,
	const u32 m,
	u8 (*d0)[LHC_BLK_Y_NUM][LHC_BLK_X_NUMS][LHC_BLK_BIN_NUM],
	u8 (*d1)[LHC_BLK_Y_NUM][LHC_BLK_X_NUMS][LHC_BLK_BIN_NUM],
	u8 (*d2)[LHC_BLK_Y_NUM][LHC_BLK_X_NUMS][LHC_BLK_BIN_NUM])
{
	u32 x, y;

	if (!addr0 || !d0 || !addr1 || !d1 || !addr2 || !d2)
		return -EINVAL;

	if (m >= MTK_MM_MODULE_MAX_NUM)
		return -EINVAL;

	for (y = 0; y < LHC_BLK_Y_NUM; y++) {
		u8 *p0 = (*d0)[y][m * LHC_BLK_X_NUM];
		u8 *p1 = (*d1)[y][m * LHC_BLK_X_NUM];
		u8 *p2 = (*d2)[y][m * LHC_BLK_X_NUM];

		for (x = 0; x < LHC_BLK_BIN_NUM; x++) {
			u32 v0 = readl(addr0);
			u32 v1 = readl(addr1);
			u32 v2 = readl(addr2);

			memcpy(p0, &v0, sizeof(v0));
			memcpy(p1, &v1, sizeof(v1));
			memcpy(p2, &v2, sizeof(v2));
			p0 += sizeof(v0);
			p1 += sizeof(v1);
			p2 += sizeof(v2);
		}
		for (; x < LHC_X_BIN_NUM; x++) {
			readl(addr0);
			readl(addr1);
			readl(addr2);
		}
	}

	return 0;
}

/** @ingroup IP_group_lhc_external_function
 * @par Description
 *     Get histogram data of 4 partition.
 * @param[in]
 *     dev: pointer of lhc device structure.
 * @param[out]
 *     data: hist structure pointer of counter result.
 * @return
 *     0, successful read hist.\n
 *     -EINVAL, null dev pointer or null data pointer.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If dev is null or data is null, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_lhc_read_histogram(const struct device *dev, struct mtk_lhc_hist *data)
{
	struct mtk_lhc *lhc;
	unsigned long flags;
	int m, cnt;
	int ret = 0;

	if (!dev || !data)
		return -EINVAL;

	lhc = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(lhc))
		return -EINVAL;

	spin_lock_irqsave(&lhc->lock_irq, flags);
	if (!lhc->read_hist) {
		spin_unlock_irqrestore(&lhc->lock_irq, flags);
		return -EBUSY;
	}
	lhc->read_hist = false;
	spin_unlock_irqrestore(&lhc->lock_irq, flags);

	data->module_num = (u8)lhc->module_cnt;
	cnt = min_t(int, lhc->module_cnt, ARRAY_SIZE(lhc->regs));
	for (m = 0; m < cnt; m++) {
		void *addr0, *addr1, *addr2;

		mtk_lhc_cpu_write_mask(lhc->regs[m] + LHC_SRAM_RW_IF_0_2,
					0x0, HIST_SRAM_RADDR_R);
		mtk_lhc_cpu_write_mask(lhc->regs[m] + LHC_SRAM_RW_IF_1_2,
					0x0, HIST_SRAM_RADDR_G);
		mtk_lhc_cpu_write_mask(lhc->regs[m] + LHC_SRAM_RW_IF_2_2,
					0x0, HIST_SRAM_RADDR_B);

		addr0 = lhc->regs[m] + LHC_SRAM_RW_IF_0_3;
		addr1 = lhc->regs[m] + LHC_SRAM_RW_IF_1_3;
		addr2 = lhc->regs[m] + LHC_SRAM_RW_IF_2_3;

		ret |= mtk_lhc_read_3data(addr0, addr1, addr2, m,
			&data->r, &data->g, &data->b);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_lhc_read_histogram);

/** @ingroup IP_group_lhc_internal_function
 * @par Description
 *     Write histogram data of one of RGB components.
 * @param[in]
 *     addr: register address to write to.
 * @param[in]
 *     m: LHC module (slice) index.
 * @param[in]
 *     d: pointer to histogram data array.
 * @return
 *     0, success.\n
 *     -EINVAL, null addr pointer or null d pointer.
 *     -EINVAL, m is equal to or larger than MTK_MM_MODULE_MAX_NUM.
 * @par Boundary case and Limitation
 *     m ranges from 0 to MTK_MM_MODULE_MAX_NUM - 1.
 * @par Error case and Error handling
 *     If addr is null or m is null, return -EINVAL.
 *     If m is equal to or larger than MTK_MM_MODULE_MAX_NUM, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_lhc_write_data(void __iomem *addr, const u32 m,
		u8 (*d)[LHC_BLK_Y_NUM][LHC_BLK_X_NUMS][LHC_BLK_BIN_NUM])
{
	u32 y;

	if (!addr || !d)
		return -EINVAL;

	if (m >= MTK_MM_MODULE_MAX_NUM)
		return -EINVAL;

	for (y = 0; y < LHC_BLK_Y_NUM; y++) {
		u32 x, v;
		u32 j = m * LHC_BLK_X_NUM, k = 0, t = 0;

		for (x = 0; x < LHC_X_BIN_NUM; x++) {
			v = 0;
			if (x * 4 < LHC_X_BIN_BYTES) {
				v = (*d)[y][j][k++];
				if (k == LHC_BLK_BIN_NUM) {
					k = 0;
					t++;
					t %= 4;
					j = t + m * LHC_BLK_X_NUM;
				}
			}

			if (x * 4 + 1 < LHC_X_BIN_BYTES) {
				v |= (u32)((*d)[y][j][k++]) << 8;
				if (k == LHC_BLK_BIN_NUM) {
					k = 0;
					t++;
					t %= 4;
					j = t + m * LHC_BLK_X_NUM;
				}
			}

			if (x * 4 + 2 < LHC_X_BIN_BYTES) {
				v |= (u32)((*d)[y][j][k++]) << 16;
				if (k == LHC_BLK_BIN_NUM) {
					k = 0;
					t++;
					t %= 4;
					j = t + m * LHC_BLK_X_NUM;
				}
			}

			if (x * 4 + 3 < LHC_X_BIN_BYTES) {
				v |= (u32)((*d)[y][j][k++]) << 24;
				if (k == LHC_BLK_BIN_NUM) {
					k = 0;
					t++;
					t %= 4;
					j = t + m * LHC_BLK_X_NUM;
				}
			}

			writel(v, addr);
		}
	}

	return 0;
}

/** @ingroup IP_group_lhc_internal_function
 * @par Description
 *     Write histogram data of 4 partition.
 * @param[in]
 *     dev: pointer of lhc device structure.
 * @param[in]
 *     data: hist structure pointer of counter data.
 * @return
 *     0, successful read hist.\n
 *     -EINVAL, null dev pointer or null data pointer.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If dev is null or data is null, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_lhc_write_histogram(const struct device *dev, struct mtk_lhc_hist *data)
{
	struct mtk_lhc *lhc;
	unsigned long flags;
	int m, cnt;
	int ret = 0;

	if (!dev || !data)
		return -EINVAL;

	lhc = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(lhc))
		return -EINVAL;

	data->module_num = (u8)lhc->module_cnt;
	cnt = min_t(int, lhc->module_cnt, ARRAY_SIZE(lhc->regs));
	for (m = 0; m < cnt; m++) {
		void *addr;

		mtk_lhc_cpu_write_mask(lhc->regs[m] + LHC_SRAM_RW_IF_0_0,
					0x0, HIST_SRAM_WADDR_R);
		addr = lhc->regs[m] + LHC_SRAM_RW_IF_0_1;
		ret |= mtk_lhc_write_data(addr, m, &data->r);

		mtk_lhc_cpu_write_mask(lhc->regs[m] + LHC_SRAM_RW_IF_1_0,
					0x0, HIST_SRAM_WADDR_G);
		addr = lhc->regs[m] + LHC_SRAM_RW_IF_1_1;
		ret |= mtk_lhc_write_data(addr, m, &data->g);

		mtk_lhc_cpu_write_mask(lhc->regs[m] + LHC_SRAM_RW_IF_2_0,
					0x0, HIST_SRAM_RADDR_B);
		addr = lhc->regs[m] + LHC_SRAM_RW_IF_2_1;
		ret |= mtk_lhc_write_data(addr, m, &data->b);
	}

	spin_lock_irqsave(&lhc->lock_irq, flags);
	lhc->read_hist = true;
	spin_unlock_irqrestore(&lhc->lock_irq, flags);

	return ret;
}
EXPORT_SYMBOL(mtk_lhc_write_histogram);

#ifdef CONFIG_MTK_DEBUGFS
static ssize_t mtk_lhc_debug_read(struct file *file, char __user *ubuf,
				size_t count, loff_t *ppos)
{
	struct mtk_lhc *lhc = file->private_data;
	int len = 0, size = 200;
	ssize_t ret = 0;
	char *usage;

	usage = kzalloc(size, GFP_KERNEL);
	if (usage == NULL)
		return -ENOMEM;

	len += scnprintf(usage + len, size - len, "[USAGE]\n");
	len += scnprintf(usage + len, size - len,
		"   echo [ACTION]... > /sys/kernel/debug/mediatek/lhc/%s\n",
		lhc->file_name);
	len += scnprintf(usage + len, size - len, "[ACTION]\n"
		"   dump_reg\n");

	if (len > size)
		len = size;
	ret = simple_read_from_buffer(ubuf, count, ppos, usage, len);
	kfree(usage);

	return ret;
}

static void mtk_lhc_dump_reg(struct mtk_lhc *lhc)
{
	int i, cnt;
	u32 u;
	unsigned long v;
	cnt = min_t(int, lhc->module_cnt, ARRAY_SIZE(lhc->regs));

	pr_err("-------------------- LHC DEBUG --------------------\n");
	for (i = 0; i < cnt; i++) {
		pr_err("-------- LHC[%d] register\n", i);
		v = readl(lhc->regs[i] + LHC_EN);
		pr_err("%-30s:%8d\n", "LHC_EN", test_bit(0, &v));
		v = readl(lhc->regs[i] + LHC_INTEN);
		pr_err("%-30s:%8d\n", "FRAME_DONE_INT_EN", test_bit(0, &v));
		pr_err("%-30s:%8d\n", "READ_FINISH_INT_EN", test_bit(1, &v));
		pr_err("%-30s:%8d\n", "IF_UNFINISH_INT_EN", test_bit(2, &v));
		pr_err("%-30s:%8d\n", "OF_UNFINISH_INT_EN", test_bit(3, &v));
		pr_err("%-30s:%8d\n", "SRAM_RW_ERR_INT_EN", test_bit(4, &v));
		pr_err("%-30s:%8d\n", "APB_RW_ERR_INT_EN", test_bit(5, &v));
		v = readl(lhc->regs[i] + LHC_INTSTA);
		pr_err("%-30s:%8d\n", "FRAME_DONE_INT_STA", test_bit(0, &v));
		pr_err("%-30s:%8d\n", "READ_FINISH_INT_STA", test_bit(1, &v));
		pr_err("%-30s:%8d\n", "IF_UNFINISH_INT_STA", test_bit(2, &v));
		pr_err("%-30s:%8d\n", "OF_UNFINISH_INT_EN", test_bit(3, &v));
		pr_err("%-30s:%8d\n", "SRAM_RW_ERR_INT_STA", test_bit(4, &v));
		pr_err("%-30s:%8d\n", "APB_RW_ERR_INT_STA", test_bit(5, &v));
		v = readl(lhc->regs[i] + LHC_CFG);
		pr_err("%-30s:%8d\n", "RELAY_MODE", test_bit(0, &v));
		pr_err("%-30s:%8d\n", "LHC_INPUT_SWAP", test_bit(3, &v));
		u = readl(lhc->regs[i] + LHC_SIZE);
		pr_err("%-30s:%8lu\n", "VSIZE", u & VSIZE);
		pr_err("%-30s:%8lu\n", "HSIZE", (u & HSIZE) >> 16);
		u = readl(lhc->regs[i] + LHC_DRE_BLOCK_INFO_00);
		pr_err("%-30s:%8lu\n", "ACT_WIN_X_START", u & ACT_WIN_X_START);
		pr_err("%-30s:%8lu\n", "ACT_WIN_X_END",
			(u & ACT_WIN_X_END) >> 16);
		u = readl(lhc->regs[i] + LHC_DRE_BLOCK_INFO_01);
		pr_err("%-30s:%8lu\n", "DRE_BLK_X_NUM", u & DRE_BLK_X_NUM);
		pr_err("%-30s:%8lu\n", "DRE_BLK_Y_NUM",
			(u & DRE_BLK_Y_NUM) >> 16);
		u = readl(lhc->regs[i] + LHC_DRE_BLOCK_INFO_02);
		pr_err("%-30s:%8lu\n", "DRE_BLK_WIDTH", u & DRE_BLK_WIDTH);
		pr_err("%-30s:%8lu\n", "DRE_BLK_HEIGHT",
			(u & DRE_BLK_HEIGHT) >> 16);
		u = readl(lhc->regs[i] + LHC_Y2R_00);
		pr_err("%-30s:%8lu\n", "Y2R_PRE_ADD_0_S", u & Y2R_PRE_ADD_0_S);
		pr_err("%-30s:%8lu\n", "Y2R_PRE_ADD_1_S",
			(u & Y2R_PRE_ADD_1_S) >> 16);
		u = readl(lhc->regs[i] + LHC_Y2R_01);
		pr_err("%-30s:%8lu\n", "Y2R_PRE_ADD_2_S", u & Y2R_PRE_ADD_2_S);
		pr_err("%-30s:%8lu\n", "Y2R_C00_S", (u & Y2R_C00_S) >> 16);
		u = readl(lhc->regs[i] + LHC_Y2R_02);
		pr_err("%-30s:%8lu\n", "Y2R_C01_S", u & Y2R_C01_S);
		pr_err("%-30s:%8lu\n", "Y2R_C02_S", (u & Y2R_C02_S) >> 16);
		u = readl(lhc->regs[i] + LHC_Y2R_03);
		pr_err("%-30s:%8lu\n", "Y2R_C10_S", u & Y2R_C10_S);
		pr_err("%-30s:%8lu\n", "Y2R_C11_S", (u & Y2R_C11_S) >> 16);
		u = readl(lhc->regs[i] + LHC_Y2R_04);
		pr_err("%-30s:%8lu\n", "Y2R_C12_S", u & Y2R_C12_S);
		pr_err("%-30s:%8lu\n", "Y2R_C20_S", (u & Y2R_C20_S) >> 16);
		u = readl(lhc->regs[i] + LHC_Y2R_05);
		pr_err("%-30s:%8lu\n", "Y2R_C21_S", u & Y2R_C21_S);
		pr_err("%-30s:%8lu\n", "Y2R_C22_S", (u & Y2R_C22_S) >> 16);
		u = readl(lhc->regs[i] + LHC_Y2R_06);
		pr_err("%-30s:%8lu\n", "Y2R_POST_ADD_0_S",
			u & Y2R_POST_ADD_0_S);
		pr_err("%-30s:%8lu\n", "Y2R_POST_ADD_1_S",
			(u & Y2R_POST_ADD_1_S) >> 16);
		v = readl(lhc->regs[i] + LHC_Y2R_07);
		pr_err("%-30s:%8u\n", "Y2R_POST_ADD_2_S",
			(u32)(v & Y2R_POST_ADD_2_S) >> 16);
		pr_err("%-30s:%8d\n", "Y2R_EN", test_bit(0, &v));
		pr_err("%-30s:%8u\n", "Y2R_INT_TABLE_SEL",
					(u32)(v & Y2R_INT_TABLE_SEL) >> 1);
		pr_err("%-30s:%8d\n", "Y2R_EXT_TABLE_EN", test_bit(4, &v));
	}
	pr_err("---------------------------------------------------\n");
}

static ssize_t mtk_lhc_debug_write(struct file *file, const char __user *ubuf,
				   size_t count, loff_t *ppos)
{
	struct mtk_lhc *lhc = file->private_data;
	char buf[64];
	size_t buf_size = min(count, sizeof(buf) - 1);

	if (copy_from_user(buf, ubuf, buf_size))
		return -EFAULT;
	buf[buf_size] = '\0';

	dev_err(lhc->dev, "debug cmd: %s\n", buf);
	if (strncmp(buf, "dump_reg", 8) == 0)
		mtk_lhc_dump_reg(lhc);
	else
		dev_err(lhc->dev, "unknown command %s\n", buf);

	return count;
}

static const struct file_operations mtk_lhc_debug_fops = {
	.owner = THIS_MODULE,
	.open		= simple_open,
	.read		= mtk_lhc_debug_read,
	.write		= mtk_lhc_debug_write,
	.llseek		= default_llseek,
};

static void mtk_lhc_debugfs_init(struct mtk_lhc *lhc, const char *name)
{
	struct dentry *file;

	if (!mtk_debugfs_root) {
		dev_err(lhc->dev, "No mtk debugfs root!\n");
		return;
	}

	lhc->debugfs = debugfs_create_dir("lhc", mtk_debugfs_root);
	if (IS_ERR(lhc->debugfs)) {
		dev_err(lhc->dev, "failed to create debug dir (%p)\n",
			lhc->debugfs);
		lhc->debugfs = NULL;
		return;
	}
	file = debugfs_create_file(name, 0664, lhc->debugfs,
				   (void *)lhc, &mtk_lhc_debug_fops);
	if (IS_ERR(file)) {
		dev_err(lhc->dev, "failed to create debug file (%p)\n", file);
		debugfs_remove(lhc->debugfs);
		lhc->debugfs = NULL;
		return;
	}
	lhc->file_name = name;
}
#endif

/** @ingroup IP_group_lhc_internal_function
 * @par Description
 *     LHC probe function
 * @param[out]
 *     pdev: platform_device setting.
 * @return
 *     0, LHC probe successed.\n
 *     -EINVAL, null pdev pointer.\n
 *     error code from of_property_read_u32().\n
 *     error code from devm_ioremap_resource().\n
 *     error code from devm_clk_get().\n
 *     error code from of_property_read_u32_array().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If pdev is null, return -EINVAL.\n
 *     2. If of_property_read_u32() fails, return its error code.\n
 *     3. If devm_ioremap_resource() fails, return its error code.\n
 *     4. If devm_clk_get() fails, return its error code.\n
 *     5. If of_property_read_u32_array() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_lhc_probe(struct platform_device *pdev)
{
	struct device *dev;
	struct mtk_lhc *lhc;
	struct device_node *node;
	struct resource *regs;
	int i, irq, cnt;
	int ret = 0;

	if (!pdev)
		return -EINVAL;

	dev = &pdev->dev;
	node = dev->of_node;

	dev_dbg(dev, "LHC probe starts\n");

	lhc = devm_kzalloc(dev, sizeof(*lhc), GFP_KERNEL);
	if (!lhc)
		return -ENOMEM;

	lhc->dev = dev;
	ret = of_property_read_u32(node, "hw-number", &lhc->module_cnt);
	if (ret != 0) {
		dev_err(dev, "can't find hw-number\n");
		return ret;
	}

	ret = of_property_read_u32_array(node, "gce-subsys", lhc->subsys,
					 lhc->module_cnt);
	if (ret != 0) {
		dev_err(dev, "can't find gce-subsys\n");
		return ret;
	}

	cnt = min_t(int, lhc->module_cnt, ARRAY_SIZE(lhc->regs));
	for (i = 0; i < cnt; i++) {
		regs = platform_get_resource(pdev, IORESOURCE_MEM, i);
		lhc->regs[i] = devm_ioremap_resource(dev, regs);
		if (IS_ERR(lhc->regs[i])) {
			dev_err(dev, "Failed to map lhc registers\n");
			return PTR_ERR(lhc->regs[i]);
		}
		lhc->subsys_offset[i] = regs->start & 0xffff;

		irq = platform_get_irq(pdev, i);
		if (irq < 0) {
			dev_err(dev, "Failed to get irq %x\n", irq);
			return irq;
		}
		lhc->irq[i] = irq;
		ret = devm_request_irq(dev, irq, mtk_lhc_irq_handler,
				       IRQF_TRIGGER_LOW, dev_name(dev), lhc);

		if (ret < 0) {
			dev_err(dev, "Failed to request irq %d:%d\n", irq, ret);
			return ret;
		}
#ifdef CONFIG_COMMON_CLK_MT3612
		lhc->clk[i] = of_clk_get(dev->of_node, i);
		if (IS_ERR(lhc->clk[i])) {
			dev_err(dev,
				"Failed to get LHC%d clock\n", i);
			return PTR_ERR(lhc->clk[i]);
		}
#endif
	}

#ifdef CONFIG_COMMON_CLK_MT3612
	pm_runtime_enable(dev);
#endif

	dev_dbg(dev, "%s probe m_cnt=%d\n", node->full_name, lhc->module_cnt);

	cnt = min_t(int, lhc->module_cnt, ARRAY_SIZE(lhc->regs));
	for (i = 0; i < cnt; i++) {
		dev_dbg(dev,
			"part=%d regs=%p subsys=0x%x sub_offset=0x%x irq=%d\n",
			 i, lhc->regs[i], lhc->subsys[i],
			 lhc->subsys_offset[i], lhc->irq[i]);
	}

	lhc->read_hist = false;
	spin_lock_init(&lhc->lock_irq);
	platform_set_drvdata(pdev, lhc);
#ifdef CONFIG_MTK_DEBUGFS
	mtk_lhc_debugfs_init(lhc, pdev->name);
#endif
	dev_dbg(dev, "LHC probe is done\n");

	return 0;
}

/** @ingroup IP_group_lhc_internal_function
 * @par Description
 *     LHC remove function used to close LHC driver
 * @param[out]
 *     pdev: platform_device setting.
 * @return
 *     0, remove successed.\n
 *     -EINVAL, null pdev pointer.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If pdev is null, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_lhc_remove(struct platform_device *pdev)
{
	struct mtk_lhc *lhc;

	if (!pdev)
		return -EINVAL;
	lhc = platform_get_drvdata(pdev);

#ifdef CONFIG_MTK_DEBUGFS
	debugfs_remove_recursive(lhc->debugfs);
	lhc->debugfs = NULL;
#endif
#ifdef CONFIG_COMMON_CLK_MT3612
	pm_runtime_disable(&pdev->dev);
#endif

	return 0;
}

/** @ingroup IP_group_lhc_internal_struct
 * @brief LHC Open Framework Device ID.\n
 * This structure is used to attach specific names to\n
 * platform device for use with device tree.
 */
static const struct of_device_id lhc_driver_dt_match[] = {
	{.compatible = "mediatek,mt3612-lhc"},
	{},
};
MODULE_DEVICE_TABLE(of, lhc_driver_dt_match);

/** @ingroup IP_group_lhc_internal_struct
 * @brief LHC platform driver structure.\n
 * This structure is used to register itself.
 */
struct platform_driver mtk_lhc_driver = {
	.probe = mtk_lhc_probe,
	.remove = mtk_lhc_remove,
	.driver = {
		   .name = "mediatek-lhc",
		   .owner = THIS_MODULE,
		   .of_match_table = lhc_driver_dt_match,
	},
};

module_platform_driver(mtk_lhc_driver);
MODULE_LICENSE("GPL");
