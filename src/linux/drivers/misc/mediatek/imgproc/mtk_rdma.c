/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	Qing Li <qing.li@mediatek.com>
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
 * @file mtk_rdma.c
 */
/**
  * @defgroup IP_group_rdma RDMA
  *     There is a set of 4 RDMAs in MT3612 accompanied by RBFC modules.\n
  *     RDMA is a hardware to read data from RAM.\n
  *     RDMA accepts input destinaion of either SRAM or DRAM.\n
  *     RDMA driver supports input in RGB/ARGB/YUYV/Y-only formats.\n
  *
  *     @{
  *         @defgroup IP_group_rdma_external EXTERNAL
  *              The external API document for RDMA.\n
  *
  *	    @{
  *             @defgroup IP_group_rdma_external_function 1.function
  *                 This is RDMA external functions.
  *             @defgroup IP_group_rdma_external_struct 3.structure
  *                 This is RDMA external structure.
  *             @defgroup IP_group_rdma_external_typedef 4.typedef
  *                 None.
  *             @defgroup IP_group_rdma_external_enum 2.enumeration
  *                 This is RDMA external enumeration.
  *             @defgroup IP_group_rdma_external_def 5.define
  *                 None.
  *	    @}
  *
  *	  @defgroup IP_group_rdma_internal INTERNAL
  *	       The internal API document for RDMA.\n
  *
  *	    @{
  *             @defgroup IP_group_rdma_internal_function 1.function
  *                 Functions for module init, remove and configuration.
  *             @defgroup IP_group_rdma_internal_struct 3.structure
  *                 This is RDMA internal structure.
  *             @defgroup IP_group_rdma_internal_typedef 4.typedef
  *                 None.
  *             @defgroup IP_group_rdma_internal_enum 2.enumeration
  *                 This is RDMA internal enumeration.
  *             @defgroup IP_group_rdma_internal_def 5.define
  *                 RDMA register definition and constant definition
  *	    @}
  *     @}
  */

#include <linux/clk.h>
#include <linux/compiler.h>
#include <linux/iopoll.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <soc/mediatek/smi.h>
#ifdef CONFIG_MTK_DEBUGFS
#include <linux/bitops.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#endif
#include <soc/mediatek/mtk_rdma.h>
#include <uapi/drm/drm_fourcc.h>

#include "mtk_rdma_reg.h"

/** @ingroup IP_group_rdma_internal_struct
 * @brief RDMA resource structure
 */
struct mtk_rdma {
	struct device			*dev;
	struct clk			*clk[MTK_MM_MODULE_MAX_NUM];
	void __iomem			*regs[MTK_MM_MODULE_MAX_NUM];
	int				irq[MTK_MM_MODULE_MAX_NUM];
	u32				source_format;
	u32				module_cnt;
	u32				cmdq_subsys[MTK_MM_MODULE_MAX_NUM];
	u32				cmdq_offset[MTK_MM_MODULE_MAX_NUM];
	struct device			*larb_dev[MTK_MM_MODULE_MAX_NUM];
	struct device			*s_larb_dev[MTK_MM_MODULE_MAX_NUM];
	spinlock_t			spinlock_irq;
	u32				frame_done;
	u32				reg_update;
	u32				cb_status;
	mtk_mmsys_cb			cb_func;
	struct mtk_mmsys_cb_data	cb_data;
	/* 0:mdp_rdma, 1:pvric_rdma, 2:disp_rdma */
	u32				rdma_type;
	u32                             eye_nr;
#ifdef CONFIG_MTK_DEBUGFS
	u32				reg_base[MTK_MM_MODULE_MAX_NUM];
	struct dentry			**debugfs;
	const char			*file_name;
#endif
};

#define mtk_rdma_type_check(rdma) \
({ \
	int local_check = 0; \
	if (!rdma) { \
		local_check = 1; \
	} else if (rdma->rdma_type > RDMA_TYPE_DISP) { \
		dev_err(rdma->dev, "invalid rdma type[%d] in func[%s]\n", \
			rdma->rdma_type, __func__); \
		local_check = 1; \
	} \
	local_check; \
})

/** @ingroup IP_group_rdma_external_function
 * @par Description
 *     RDMA get larb.
 * @param[in] dev: RDMA device node.
 * @param[in] is_sram: access memory from sram or not.
 * @return
 *     0, get larb success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from mtk_smi_larb_get.
 * @par Boundary case and Limitation
 *     dev can not be NULL.
 * @par Error case and Error handling
 *     1. If dev is NULL, return -EINVAL.\n
 *     2. Return error code if mtk_smi_larb_get fail.
 *     3. Return -EINVAL if it is called by mdp and is_sram is true.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rdma_larb_get(const struct device *dev, bool is_sram)
{
	u32 i, ret = 0;
	struct mtk_rdma *rdma;

	if (!dev)
		return -EINVAL;

	rdma = dev_get_drvdata(dev);
	if (!rdma)
		return -EINVAL;

	if (rdma->rdma_type >= RDMA_TYPE_DISP)
		return -EINVAL;

	if (!is_sram) {
		for (i = 0; i < rdma->module_cnt; i++)
			ret |= mtk_smi_larb_get(rdma->larb_dev[i]);
	} else {
		for (i = 0; i < rdma->module_cnt; i++) {
			if (rdma->s_larb_dev[i])
				ret |= mtk_smi_larb_get(rdma->s_larb_dev[i]);
			else {
				ret = -EINVAL;
				break;
			}
		}
	}
	return ret;
}
EXPORT_SYMBOL(mtk_rdma_larb_get);

/** @ingroup IP_group_rdma_external_function
 * @par Description
 *     RDMA put larb.
 * @param[in] dev: RDMA device node.
 * @param[in] is_sram: access memory from sram or not.
 * @return
 *     0, put larb success.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     dev can not be NULL.
 * @par Error case and Error handling
 *     1. If dev is NULL, return -EINVAL.
 *     2. Return -EINVAL if it is called by mdp and is_sram is true.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rdma_larb_put(const struct device *dev, bool is_sram)
{
	u32 i, ret = 0;
	struct mtk_rdma *rdma;

	if (!dev)
		return -EINVAL;

	rdma = dev_get_drvdata(dev);
	if (!rdma)
		return -EINVAL;

	if (rdma->rdma_type >= RDMA_TYPE_DISP)
		return -EINVAL;

	if (!is_sram) {
		for (i = 0; i < rdma->module_cnt; i++)
			mtk_smi_larb_put(rdma->larb_dev[i]);
	} else {
		for (i = 0; i < rdma->module_cnt; i++) {
			if (rdma->s_larb_dev[i])
				mtk_smi_larb_put(rdma->s_larb_dev[i]);
			else {
				ret = -EINVAL;
				break;
			}
		}
	}
	return ret;
}
EXPORT_SYMBOL(mtk_rdma_larb_put);

/** @ingroup IP_group_rdma_external_function
 * @par Description
 *     Enable the power and clock of RDMA engine.
 * @param[in]
 *     dev: RDMA device node.
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
int mtk_rdma_power_on(struct device *dev)
{
#ifdef CONFIG_COMMON_CLK_MT3612
	struct mtk_rdma *rdma;
	int i, ret = 0;

	if (!dev)
		return -EINVAL;

	rdma = dev_get_drvdata(dev);
	if (!rdma)
		return -EINVAL;

	ret = pm_runtime_get_sync(dev);
	if (ret < 0) {
		dev_err(dev, "Failed to enable rdma power domain: %d\n", ret);
		return ret;
	}

	for (i = 0; i < rdma->module_cnt; i++) {
		ret = clk_prepare_enable(rdma->clk[i]);
		if (ret) {
			dev_err(dev, "Failed to enable rdma clock %d: %d\n",
				i, ret);
			goto err;
		}
	}
	return 0;
err:
	while (--i >= 0)
		clk_disable_unprepare(rdma->clk[i]);
	pm_runtime_put(dev);
	return ret;
#else
	return 0;
#endif
}
EXPORT_SYMBOL(mtk_rdma_power_on);

/** @ingroup IP_group_rdma_external_function
 * @par Description
 *     Disable the power and clock of RDMA engine.
 * @param[in]
 *     dev: RDMA device node.
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
int mtk_rdma_power_off(struct device *dev)
{
#ifdef CONFIG_COMMON_CLK_MT3612
	struct mtk_rdma *rdma;
	int i;

	if (!dev)
		return -EINVAL;

	rdma = dev_get_drvdata(dev);
	if (!rdma)
		return -EINVAL;

	for (i = 0; i < rdma->module_cnt; i++)
		clk_disable_unprepare(rdma->clk[i]);
	pm_runtime_put(dev);
#endif
	return 0;
}
EXPORT_SYMBOL(mtk_rdma_power_off);

/** @ingroup IP_group_rdma_internal_function
 * @par Description
 *     This function judges if the given format is 10 bit.\n
 * @param[in]
 *     format: given format
 * @return
 *     1 for 10-bit given format
 *     -EINVAL, if mutex_res is not valid.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If the given format is unrecognizable, the function returns -EINVAL.\n
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int rdma_is_10_bit_fmt(u32 format)
{
	switch (format) {
	case DRM_FORMAT_ARGB2101010:
	case DRM_FORMAT_ABGR2101010:
	case DRM_FORMAT_RGBA1010102:
	case DRM_FORMAT_BGRA1010102:
	case DRM_FORMAT_R10:
	case DRM_FORMAT_AYUV2101010:
		return 1;
	case DRM_FORMAT_RGB888:
	case DRM_FORMAT_BGR888:
	case DRM_FORMAT_R8:
	case DRM_FORMAT_YUYV:
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_ABGR8888:
		return 0;
	default:
		return -EINVAL;
	}
}

/** @ingroup IP_group_rdma_internal_function
 * @par Description
 *     Setting module registers by CPU or CMDQ.\n
 * @param[in]
 *     rdma: driver data of RDMA
 * @param[in]
 *     mod_idx: RDMA of which partition id to be set
 * @param[in]
 *     offset: register offset base to be set
 * @param[in]
 *     value: value to be set
 * @param[out]
 *     cmdq_handle: multiple CMDQ handles, set NULL for CPU write directly.
 * @return
 *     0, all parameters are valid.\n
 *     -EINVAL, if rdma, offset or mod_idx is not valid.
 * @par Boundary case and Limitation
 *     Offset should be within module valid range.
 * @par Error case and Error handling
 *      If rdma, offset or mod_idx is not valid, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int rdma_writel_s(struct mtk_rdma *rdma, u32 mod_idx, u32 offset,
		   u32 value, struct cmdq_pkt **cmdq_handle)
{
	int ret = 0;

	if (cmdq_handle) {
		ret |= cmdq_pkt_write_value(cmdq_handle[mod_idx],
				     rdma->cmdq_subsys[mod_idx],
				     rdma->cmdq_offset[mod_idx] + offset,
				     value, 0xffffffff);
	} else {
		void __iomem *reg = rdma->regs[mod_idx];

		writel(value, reg + offset);
	}

	return ret;
}

/** @ingroup IP_group_rdma_internal_function
 * @par Description
 *     Setting module registers with mask by CPU or CMDQ.\n
 * @param[in]
 *     rdma: driver data of RDMA
 * @param[in]
 *     mod_idx: RDMA of which partition id to be set
 * @param[in]
 *     offset: register offset base to be set
 * @param[in]
 *     value: value to be set
 * @param[in]
 *     mask: mask for register set
 * @param[out]
 *     cmdq_handle: multiple CMDQ handles, set NULL for CPU write directly.
 * @return
 *     0, all parameters are valid.\n
 *     -EINVAL, if rdma, offset or mod_idx is not valid.
 * @par Boundary case and Limitation
 *     Offset should be within module valid range.
 * @par Error case and Error handling
 *     If rdma, offset or mod_idx is not valid, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int rdma_writel_mask_s(struct mtk_rdma *rdma, u32 mod_idx,
			u32 offset, u32 value, u32 mask,
			struct cmdq_pkt **cmdq_handle)
{
	int ret = 0;

	if (cmdq_handle) {
		ret |= cmdq_pkt_write_value(cmdq_handle[mod_idx],
				     rdma->cmdq_subsys[mod_idx],
				     rdma->cmdq_offset[mod_idx] + offset,
				     value, mask);
	} else {
		void __iomem *reg = rdma->regs[mod_idx];
		u32 regvalue;

		regvalue = readl(reg + offset);
		regvalue = (regvalue & ~mask) | (value & mask);
		writel(regvalue, reg + offset);
	}

	return ret;
}

/** @ingroup IP_group_rdma_internal_function
 * @par Description
 *     Setting module registers of 4 partitions by CPU or CMDQ.\n
 * @param[in]
 *     rdma: driver data of RDMA
 * @param[in]
 *     offset: register offset base to be set
 * @param[in]
 *     value: value to be set
 * @param[out]
 *     cmdq_handle: multiple CMDQ handles, set NULL for CPU write directly.
 * @return
 *     0, all parameters are valid.\n
 *     -EINVAL, if rdma or offset is not valid.
 * @par Boundary case and Limitation
 *     Offset should be within module valid range.
 * @par Error case and Error handling
 *     If rdma or offset is not valid, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int rdma_writel(struct mtk_rdma *rdma, u32 offset, u32 value,
		 struct cmdq_pkt **cmdq_handle)
{
	u32 m_idx;
	int ret = 0;

	for (m_idx = 0; m_idx < rdma->module_cnt; m_idx++)
		ret |= rdma_writel_s(rdma, m_idx, offset, value, cmdq_handle);
	return ret;
}

/** @ingroup IP_group_rdma_internal_function
 * @par Description
 *     Setting module registers of 4 partitions with mask by CPU or CMDQ.
 * @param[in]
 *     rdma: driver data of RDMA
 * @param[in]
 *     offset: register offset base to be set
 * @param[in]
 *     value: value to be set
 * @param[in]
 *     mask: mask for register set
 * @param[out]
 *     cmdq_handle: multiple CMDQ handles, set NULL for CPU write directly.
 * @return
 *     0, all parameters are valid.\n
 *     -EINVAL, if rdma or offset is not valid.
 * @par Boundary case and Limitation
 *     Offset should be within module valid range.
 * @par Error case and Error handling
 *     If rdma or offset is not valid, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int rdma_writel_mask(struct mtk_rdma *rdma, u32 offset, u32 value,
		      u32 mask, struct cmdq_pkt **cmdq_handle)
{
	u32 m_idx;
	int ret = 0;

	for (m_idx = 0; m_idx < rdma->module_cnt; m_idx++)
		ret |= rdma_writel_mask_s(rdma, m_idx, offset, value,
				mask, cmdq_handle);
	return ret;
}

/** @ingroup IP_group_rdma_internal_function
 * @par Description
 *     Setting module registers by CPU or CMDQ.\n
 * @param[in]
 *     rdma: driver data of RDMA
 * @param[in]
 *     mod_idx: RDMA of which partition id to be set
 * @param[in]
 *     offset: register offset base to be set
 * @param[in]
 *     value: value to be set
 * @param[out]
 *     cmdq_handle: single CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, all parameters are valid.\n
 *     -EINVAL, if rdma, offset or mod_idx is not valid.
 * @par Boundary case and Limitation
 *     Offset should be within module valid range.
 * @par Error case and Error handling
 *      If rdma, offset or mod_idx is not valid, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int rdma_writel_s_with_single_cmdq(
		struct mtk_rdma *rdma, u32 mod_idx, u32 offset,
		u32 value, struct cmdq_pkt *cmdq_handle)
{
	int ret = 0;

	if (cmdq_handle) {
		ret |= cmdq_pkt_write_value(cmdq_handle,
				     rdma->cmdq_subsys[mod_idx],
				     rdma->cmdq_offset[mod_idx] + offset,
				     value, 0xffffffff);
	} else {
		void __iomem *reg = rdma->regs[mod_idx];

		writel(value, reg + offset);
	}

	return ret;
}

/** @ingroup IP_group_rdma_internal_function
 * @par Description
 *     Setting module registers with mask by CPU or CMDQ.\n
 * @param[in]
 *     rdma: driver data of RDMA
 * @param[in]
 *     mod_idx: RDMA of which partition id to be set
 * @param[in]
 *     offset: register offset base to be set
 * @param[in]
 *     value: value to be set
 * @param[in]
 *     mask: mask for register set
 * @param[out]
 *     cmdq_handle: single CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, all parameters are valid.\n
 *     -EINVAL, if rdma, offset or mod_idx is not valid.
 * @par Boundary case and Limitation
 *     Offset should be within module valid range.
 * @par Error case and Error handling
 *     If rdma, offset or mod_idx is not valid, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int rdma_writel_mask_s_with_single_cmdq(
		struct mtk_rdma *rdma, u32 mod_idx,
		u32 offset, u32 value, u32 mask,
		struct cmdq_pkt *cmdq_handle)
{
	int ret = 0;

	if (cmdq_handle) {
		ret |= cmdq_pkt_write_value(cmdq_handle,
				     rdma->cmdq_subsys[mod_idx],
				     rdma->cmdq_offset[mod_idx] + offset,
				     value, mask);
	} else {
		void __iomem *reg = rdma->regs[mod_idx];
		u32 regvalue;

		regvalue = readl(reg + offset);
		regvalue = (regvalue & ~mask) | (value & mask);
		writel(regvalue, reg + offset);
	}

	return ret;
}

/** @ingroup IP_group_rdma_internal_function
 * @par Description
 *     Setting module registers of 4 partitions by CPU or CMDQ.\n
 * @param[in]
 *     rdma: driver data of RDMA
 * @param[in]
 *     offset: register offset base to be set
 * @param[in]
 *     value: value to be set
 * @param[out]
 *     cmdq_handle: single CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, all parameters are valid.\n
 *     -EINVAL, if rdma or offset is not valid.
 * @par Boundary case and Limitation
 *     Offset should be within module valid range.
 * @par Error case and Error handling
 *     If rdma or offset is not valid, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int rdma_writel_with_single_cmdq(
		struct mtk_rdma *rdma, u32 offset, u32 value,
		struct cmdq_pkt *cmdq_handle)
{
	u32 m_idx;
	int ret = 0;

	for (m_idx = 0; m_idx < rdma->module_cnt; m_idx++)
		ret |= rdma_writel_s_with_single_cmdq(
			rdma, m_idx, offset, value, cmdq_handle);
	return ret;
}

/** @ingroup IP_group_rdma_internal_function
 * @par Description
 *     Setting module registers of 4 partitions with mask by CPU or CMDQ.
 * @param[in]
 *     rdma: driver data of RDMA
 * @param[in]
 *     offset: register offset base to be set
 * @param[in]
 *     value: value to be set
 * @param[in]
 *     mask: mask for register set
 * @param[out]
 *     cmdq_handle: single CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, all parameters are valid.\n
 *     -EINVAL, if rdma or offset is not valid.
 * @par Boundary case and Limitation
 *     Offset should be within module valid range.
 * @par Error case and Error handling
 *     If rdma or offset is not valid, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int rdma_writel_mask_with_single_cmdq(
		struct mtk_rdma *rdma, u32 offset, u32 value,
		u32 mask, struct cmdq_pkt *cmdq_handle)
{
	u32 m_idx;
	int ret = 0;

	for (m_idx = 0; m_idx < rdma->module_cnt; m_idx++)
		ret |= rdma_writel_mask_s_with_single_cmdq(
			rdma, m_idx, offset, value, mask, cmdq_handle);
	return ret;
}

/** @ingroup IP_group_rdma_internal_function
 * @par Description
 *     polling module registers with mask by CMDQ.\n
 * @param[in]
 *     rdma: driver data of RDMA
 * @param[in]
 *     mod_idx: RDMA of which partition id to be set
 * @param[in]
 *     offset: register offset base to be set
 * @param[in]
 *     value: value to be set
 * @param[in]
 *     mask: mask for register set
 * @param[out]
 *     cmdq_handle: single CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, all parameters are valid.\n
 *     -EINVAL, if cmdq_handle, rdma, offset or mod_idx is not valid.
 * @par Boundary case and Limitation
 *     Offset should be within module valid range.
 * @par Error case and Error handling
 *     If cmdq_handle, rdma, offset or mod_idx is not valid, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int rdma_poll_mask_s_with_single_cmdq(
		struct mtk_rdma *rdma, u32 mod_idx,
		u32 offset, u32 value, u32 mask,
		struct cmdq_pkt *cmdq_handle)
{
	int ret = -1;

	if (cmdq_handle) {
		ret = cmdq_pkt_poll(cmdq_handle,
				     rdma->cmdq_subsys[mod_idx],
				     rdma->cmdq_offset[mod_idx] + offset,
				     value, mask);
	}

	return ret;
}

/** @ingroup IP_group_rdma_internal_function
 * @par Description
 *     polling module registers of 4 partitions with mask by CMDQ.
 * @param[in]
 *     rdma: driver data of RDMA
 * @param[in]
 *     offset: register offset base to be set
 * @param[in]
 *     value: value to be set
 * @param[in]
 *     mask: mask for register set
 * @param[out]
 *     cmdq_handle: single CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, all parameters are valid.\n
 *     -EINVAL, if rdma or offset is not valid.
 * @par Boundary case and Limitation
 *     Offset should be within module valid range.
 * @par Error case and Error handling
 *     If rdma or offset is not valid, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int rdma_poll_mask_with_single_cmdq(
		struct mtk_rdma *rdma, u32 offset, u32 value,
		u32 mask, struct cmdq_pkt *cmdq_handle)
{
	u32 m_idx;
	int ret = 0;

	for (m_idx = 0; m_idx < rdma->module_cnt; m_idx++)
		ret |= rdma_poll_mask_s_with_single_cmdq(
			rdma, m_idx, offset, value, mask, cmdq_handle);

	return ret;
}

/** @ingroup IP_group_rdma_internal_function
 * @par Description
 *    RDMA driver irq handler. RDMA will send error interrupt\n
 *    when error occurs. We then check the error status in this handler.
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
static irqreturn_t mtk_rdma_irq_handler(int irq, void *dev_id)
{
	mtk_mmsys_cb cb;
	struct mtk_mmsys_cb_data cb_data;
	struct mtk_rdma *rdma = dev_id;
	u32 val = 0;
	u32 m_idx, cb_status;
	unsigned long flags;

	if (mtk_rdma_type_check(rdma))
		return IRQ_NONE;

	for (m_idx = 0; m_idx < rdma->module_cnt; m_idx++) {
		if (irq == rdma->irq[m_idx]) {
			if (rdma->rdma_type == RDMA_TYPE_MDP) {
				val = readl(rdma->regs[m_idx] +
					MDP_RDMA_INTERRUPT_STATUS);
				writel(0, rdma->regs[m_idx] +
					MDP_RDMA_INTERRUPT_STATUS);
			} else if (rdma->rdma_type == RDMA_TYPE_PVRIC) {
				val = readl(rdma->regs[m_idx] +
					PVRIC_INTERRUPT_STATUS);
				writel(0, rdma->regs[m_idx] +
					PVRIC_INTERRUPT_STATUS);
			} else if (rdma->rdma_type == RDMA_TYPE_DISP) {
				val = readl(rdma->regs[m_idx] +
					DISP_RDMA_INT_STATUS);
				writel(0, rdma->regs[m_idx] +
					DISP_RDMA_INT_STATUS);
			}

			break;
		}
	}
	if (m_idx == rdma->module_cnt)
		return IRQ_NONE;

	spin_lock_irqsave(&rdma->spinlock_irq, flags);
	cb_data = rdma->cb_data;
	cb = rdma->cb_func;
	cb_status = rdma->cb_status;
	spin_unlock_irqrestore(&rdma->spinlock_irq, flags);

	if ((cb_status & val) && cb) {
		bool frame_done_cb = false, reg_update_cb = false;
		const u32 frame_done_val = (1 << rdma->module_cnt) - 1;

		if (cb_status & val & MTK_RDMA_FRAME_COMPLETE) {
			spin_lock_irqsave(&rdma->spinlock_irq, flags);
			rdma->frame_done |= (1 << m_idx);
			if (rdma->frame_done == frame_done_val) {
				rdma->frame_done = 0;
				frame_done_cb = true;
			}
			spin_unlock_irqrestore(&rdma->spinlock_irq, flags);
			val &= ~MTK_RDMA_FRAME_COMPLETE;
		}

		if (cb_status & val & MTK_RDMA_REG_UPDATE) {
			spin_lock_irqsave(&rdma->spinlock_irq, flags);
			rdma->reg_update |= (1 << m_idx);
			if (rdma->reg_update == frame_done_val) {
				rdma->reg_update = 0;
				reg_update_cb = true;
			}
			spin_unlock_irqrestore(&rdma->spinlock_irq, flags);
			val &= ~MTK_RDMA_REG_UPDATE;
		}

		if (val > 0) {
			cb_data.part_id = m_idx;
			cb_data.status = val;
			cb(&cb_data);
		}

		if (reg_update_cb) {
			cb_data.part_id = frame_done_val;
			cb_data.status = MTK_RDMA_REG_UPDATE;
			cb(&cb_data);
		}

		if (frame_done_cb) {
			cb_data.part_id = frame_done_val;
			cb_data.status = MTK_RDMA_FRAME_COMPLETE;
			cb(&cb_data);
		}
	}

	return IRQ_HANDLED;
}

/** @ingroup IP_group_rdma_external_function
 * @par Description
 *     Register callback function for RDMA interrupt.
 * @param[in]
 *     dev: RDMA device node.
 * @param[in]
 *     cb: RDMA irq callback function pointer assigned by user.
 * @param[in]
 *     status: Specify which irq status should be notified via callback.
 *     If mdp_rdma, bit[2:0] is valid.
 *     If pvric_rdma, bit[2:0] is valid.
 *     If disp_rdma, bit[9:0] is valid.
 * @param[in]
 *     priv_data: RDMA irq callback function's priv_data.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from rdma_writel_mask().
 * @par Boundary case and Limitation
 *     dev can not be NULL.
 * @par Error case and Error handling
 *     1. If dev is NULL, return -EINVAL.\n
 *     2. If rdma_writel_mask() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rdma_register_cb(struct device *dev,
			mtk_mmsys_cb cb, u32 status, void *priv_data)
{
	struct mtk_rdma *rdma;
	int ret = 0;
	unsigned long flags;

	if (!dev)
		return -EINVAL;

	rdma = dev_get_drvdata(dev);

	if (mtk_rdma_type_check(rdma))
		return -EINVAL;

	if (rdma->rdma_type == RDMA_TYPE_MDP) {
		if (status & 0xfffffff8)
			return -EINVAL;
	} else if (rdma->rdma_type == RDMA_TYPE_PVRIC) {
		if (status & 0xfffffff8)
			return -EINVAL;
	} else if (rdma->rdma_type == RDMA_TYPE_DISP) {
		if (status & 0xfffffc00)
			return -EINVAL;
	}

	spin_lock_irqsave(&rdma->spinlock_irq, flags);
	rdma->cb_func = cb;
	rdma->cb_data.priv_data = priv_data;
	rdma->frame_done = 0;
	rdma->reg_update = 0;
	rdma->cb_status = status;

	/* clear/set ignore flag to enable/disable cb */

	if (rdma->cb_func) {
		if (rdma->rdma_type == RDMA_TYPE_MDP)
			ret = rdma_writel_mask(rdma, MDP_RDMA_INTERRUPT_ENABLE,
						status, 0xFFFFFFFF, NULL);
		else if (rdma->rdma_type == RDMA_TYPE_PVRIC)
			ret = rdma_writel_mask(rdma, PVRIC_INTERRUPT_ENABLE,
						status, 0xFFFFFFFF, NULL);
		else if (rdma->rdma_type == RDMA_TYPE_DISP)
			ret = rdma_writel_mask(rdma, DISP_RDMA_INT_ENABLE,
						status, 0xFFFFFFFF, NULL);
	} else {
		if (rdma->rdma_type == RDMA_TYPE_MDP)
			ret = rdma_writel_mask(rdma, MDP_RDMA_INTERRUPT_ENABLE,
						0, 0xFFFFFFFF, NULL);
		else if (rdma->rdma_type == RDMA_TYPE_PVRIC)
			ret = rdma_writel_mask(rdma, PVRIC_INTERRUPT_ENABLE,
						0, 0xFFFFFFFF, NULL);
		else if (rdma->rdma_type == RDMA_TYPE_DISP)
			ret = rdma_writel_mask(rdma, DISP_RDMA_INT_ENABLE,
						0, 0xFFFFFFFF, NULL);
	}

	spin_unlock_irqrestore(&rdma->spinlock_irq, flags);
	return ret;
}
EXPORT_SYMBOL(mtk_rdma_register_cb);

/** @ingroup IP_group_rdma_external_function
 * @par Description
 *     This function triggers SW reset of RDMA.\n
 * @param[in]
 *     dev: device node pointer of RDMA.
 * @param[out]
 *     cmdq_handle: single CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, all parameters are valid.\n
 *     -EINVAL, if dev is NULL.
 * @par Boundary case and Limitation
 *     None.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rdma_reset_split_start(
	const struct device *dev, struct cmdq_pkt *cmdq_handle)
{
	struct mtk_rdma *rdma;

	if (!dev)
		return -EINVAL;

	rdma = dev_get_drvdata(dev);

	if (mtk_rdma_type_check(rdma))
		return -EINVAL;

	if (rdma->rdma_type == RDMA_TYPE_MDP) {
		rdma_writel_with_single_cmdq(
			rdma, MDP_RDMA_RESET, 1, cmdq_handle);
	} else if (rdma->rdma_type == RDMA_TYPE_PVRIC) {
		rdma_writel_with_single_cmdq(rdma, PVRIC_RESET, 1, cmdq_handle);
	} else if (rdma->rdma_type == RDMA_TYPE_DISP) {
		rdma_writel_mask_with_single_cmdq(rdma, DISP_RDMA_GLOBAL_CON,
			0x1 << 4, DISP_RDMA_SOFT_RESET, cmdq_handle);
	}

	return 0;
}
EXPORT_SYMBOL(mtk_rdma_reset_split_start);

/** @ingroup IP_group_rdma_external_function
 * @par Description
 *     This function polling hw status to confirm complete SW reset of RDMA.\n
 * @param[in]
 *     dev: device node pointer of RDMA.
 * @return
 *     0, all parameters are valid.\n
 *     -EINVAL, if dev is NULL.
 * @par Boundary case and Limitation
 *     None.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rdma_reset_split_end(const struct device *dev)
{
	struct mtk_rdma *rdma;
	u32 idx;

	if (!dev)
		return -EINVAL;

	rdma = dev_get_drvdata(dev);

	if (mtk_rdma_type_check(rdma))
		return -EINVAL;

	if (rdma->rdma_type == RDMA_TYPE_MDP) {
		for (idx = 0; idx < rdma->module_cnt; idx++) {
			if (((readl(rdma->regs[idx] + MDP_RDMA_RESET))
					& WARM_RESET) != 0) {
				dev_err(dev, "mtk_rdma_reset reset fail\n");
				return -EBUSY;
			}
		}
	} else if (rdma->rdma_type == RDMA_TYPE_PVRIC) {
		for (idx = 0; idx < rdma->module_cnt; idx++) {
			if (((readl(rdma->regs[idx] + PVRIC_RESET))
					& WARM_RESET) != 0) {
				dev_err(dev, "mtk_rdma_reset reset fail\n");
				return -EBUSY;
			}
		}
	} else if (rdma->rdma_type == RDMA_TYPE_DISP) {
		for (idx = 0; idx < rdma->module_cnt; idx++) {
			if (((readl(rdma->regs[idx] + DISP_RDMA_GLOBAL_CON))
					& DISP_RDMA_SOFT_RESET) != 0) {
				dev_err(dev, "mtk_rdma_reset reset fail\n");
				return -EBUSY;
			}
		}
	}

	return 0;
}
EXPORT_SYMBOL(mtk_rdma_reset_split_end);

/** @ingroup IP_group_rdma_external_function
 * @par Description
 *     This function polling hw status to confirm complete SW reset of RDMA.\n
 * @param[in]
 *     dev: device node pointer of RDMA.
 * @param[out]
 *     cmdq_handle: single CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, all parameters are valid.\n
 *     -EINVAL, if dev is NULL.
 * @par Boundary case and Limitation
 *     None.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rdma_reset_split_polling(
	const struct device *dev, struct cmdq_pkt *cmdq_handle)
{
	int ret = 0;
	struct mtk_rdma *rdma;

	if ((!dev) || (!cmdq_handle))
		return -EINVAL;

	rdma = dev_get_drvdata(dev);

	if (mtk_rdma_type_check(rdma))
		return -EINVAL;

	if (rdma->rdma_type == RDMA_TYPE_MDP) {
		ret = rdma_poll_mask_with_single_cmdq(
			rdma, MDP_RDMA_RESET, 0, 0x1, cmdq_handle);
	} else if (rdma->rdma_type == RDMA_TYPE_PVRIC) {
		ret = rdma_poll_mask_with_single_cmdq(
			rdma, PVRIC_RESET, 0, 0x1, cmdq_handle);
	} else if (rdma->rdma_type == RDMA_TYPE_DISP) {
		ret = rdma_poll_mask_with_single_cmdq(
			rdma, DISP_RDMA_GLOBAL_CON, 0,
			DISP_RDMA_SOFT_RESET, cmdq_handle);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_rdma_reset_split_polling);

/** @ingroup IP_group_rdma_external_function
 * @par Description
 *     This function triggers SW reset of RDMA and polling hw status.\n
 * @param[in]
 *     dev: device node pointer of RDMA.
 * @return
 *     0, all parameters are valid.\n
 *     -EINVAL, if dev is NULL.
 * @par Boundary case and Limitation
 *     None.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rdma_reset(const struct device *dev)
{
	struct mtk_rdma *rdma;
	u32 idx, rst_status;

	if (!dev)
		return -EINVAL;

	rdma = dev_get_drvdata(dev);

	if (mtk_rdma_type_check(rdma))
		return -EINVAL;

	if (rdma->rdma_type == RDMA_TYPE_MDP) {
		rdma_writel(rdma, MDP_RDMA_RESET, 1, NULL);

		/* polling reset status */
		for (idx = 0; idx < rdma->module_cnt; idx++) {
			if (readl_poll_timeout_atomic(
				rdma->regs[idx] + MDP_RDMA_RESET,
				rst_status, ((rst_status & WARM_RESET) == 0),
				2, 10)) {
				dev_err(dev, "mtk_rdma_reset reset timeout\n");
				return -EBUSY;
			}
		}
	} else if (rdma->rdma_type == RDMA_TYPE_PVRIC) {
		rdma_writel(rdma, PVRIC_RESET, 1, NULL);

		/* polling reset status */
		for (idx = 0; idx < rdma->module_cnt; idx++) {
			if (readl_poll_timeout_atomic(
				rdma->regs[idx] + PVRIC_RESET,
				rst_status, ((rst_status & WARM_RESET) == 0),
				2, 10)) {
				dev_err(dev, "mtk_rdma_reset reset timeout\n");
				return -EBUSY;
			}
		}
	} else if (rdma->rdma_type == RDMA_TYPE_DISP) {

		rdma_writel_mask(rdma, DISP_RDMA_GLOBAL_CON,
			0x1 << 4, 0x1 << 4, NULL);

		for (idx = 0; idx < rdma->module_cnt; idx++) {
			if (readl_poll_timeout_atomic(
				rdma->regs[idx] + DISP_RDMA_GLOBAL_CON,
				rst_status, ((rst_status & 0x700) == 0x400),
				2, 10)) {
				dev_err(dev, "mtk_rdma_reset wait for reset start timeout\n");
				return -EBUSY;
			}
		}

		rdma_writel_mask(rdma, DISP_RDMA_GLOBAL_CON,
			0x0 << 4, 0x1 << 4, NULL);
	}

	return 0;
}
EXPORT_SYMBOL(mtk_rdma_reset);

/** @ingroup IP_group_rdma_external_function
 * @par Description
 *     This function starts RDMA.\n
 * @param[in]
 *     dev: device node pointer of RDMA.
 * @param[out]
 *     cmdq_handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, all parameters are valid.\n
 *     -EINVAL, if dev is NULL.
 * @par Boundary case and Limitation
 *     None.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rdma_start(const struct device *dev, struct cmdq_pkt **cmdq_handle)
{
	struct mtk_rdma *rdma;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	rdma = dev_get_drvdata(dev);

	if (mtk_rdma_type_check(rdma))
		return -EINVAL;

	if (rdma->rdma_type == RDMA_TYPE_MDP) {
		rdma_writel(rdma, MDP_RDMA_GMCIF_CON,
			((0x7 << 4) + (0x1 << 12) + (0x1 << 16)),
			cmdq_handle);
		rdma_writel(rdma, MDP_RDMA_DMAULTRA_CON_0,
			0xC0A0A080,
			cmdq_handle);
	} else if (rdma->rdma_type == RDMA_TYPE_PVRIC) {
		rdma_writel(rdma, PVRIC_SMI_ULTRA_CON,
			0x00001001, cmdq_handle);
		rdma_writel(rdma, PVRIC_SMI_ULTRA_CON2,
			0x00002001, cmdq_handle);
	}

	if (rdma->rdma_type == RDMA_TYPE_MDP)
		rdma_writel(rdma, MDP_RDMA_EN, 0x1, cmdq_handle);
	else if (rdma->rdma_type == RDMA_TYPE_PVRIC)
		rdma_writel(rdma, PVRIC_EN, 0x1, cmdq_handle);
	else if (rdma->rdma_type == RDMA_TYPE_DISP)
		rdma_writel_mask(rdma, DISP_RDMA_GLOBAL_CON,
			0x1, 0x1, cmdq_handle);

	return ret;
}
EXPORT_SYMBOL(mtk_rdma_start);

/** @ingroup IP_group_rdma_external_function
 * @par Description
 *     This function stops RDMA.\n
 * @param[in]
 *     dev: device node pointer of RDMA.
 * @param[out]
 *     cmdq_handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, all parameters are valid.\n
 *     -EINVAL, if dev is NULL.
 * @par Boundary case and Limitation
 *     None.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rdma_stop(const struct device *dev, struct cmdq_pkt **cmdq_handle)
{
	struct mtk_rdma *rdma;

	if (!dev)
		return -EINVAL;

	rdma = dev_get_drvdata(dev);

	if (mtk_rdma_type_check(rdma))
		return -EINVAL;

	if (rdma->rdma_type == RDMA_TYPE_MDP)
		rdma_writel(rdma, MDP_RDMA_EN, 0x0, cmdq_handle);
	else if (rdma->rdma_type == RDMA_TYPE_PVRIC)
		rdma_writel(rdma, PVRIC_EN, 0x0, cmdq_handle);
	else if (rdma->rdma_type == RDMA_TYPE_DISP)
		rdma_writel_mask(rdma, DISP_RDMA_GLOBAL_CON,
			0x0, 0x1, cmdq_handle);

	return 0;
}
EXPORT_SYMBOL(mtk_rdma_stop);

/** @ingroup IP_group_rdma_external_function
 * @par Description
 *     This function select sram share type for RDMA.\n
 * @param[in]
 *     dev: device node pointer of RDMA.
 * @param[out]
 *     cmdq_handle: CMDQ handle, set NULL for CPU write directly.
 * @param[in]
 *     sel_rdma_type: select the sram type which rdma hardware will be use.
 *     The valid value is RDMA_TYPE_MDP(0) ~ RDMA_TYPE_DISP(2).
 * @return
 *     0, all parameters are valid.\n
 *     -EINVAL, if dev is NULL.
 * @par Boundary case and Limitation
 *     None.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.\n
 *     If rdma_type in device is not RDMA_TYPE_PVRIC, return -EINVAL.\n
 *     If sel_rdma_type value is more than RDMA_TYPE_DISP, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rdma_sel_sram(const struct device *dev,
		struct cmdq_pkt **cmdq_handle,
		u8 sel_rdma_type)
{
	struct mtk_rdma *rdma;
	int ret = 0;
	u8 sram_share_en = 0;
	u8 sram_share_sel = 0;

	if (!dev)
		return -EINVAL;

	rdma = dev_get_drvdata(dev);
	if (!rdma)
		return -EINVAL;

	if (rdma->rdma_type != RDMA_TYPE_PVRIC) {
		dev_err(rdma->dev, "invalid device-rdma-type[%d] to select sram\n",
			rdma->rdma_type);
		return -EINVAL;
	}

	if (sel_rdma_type > RDMA_TYPE_DISP) {
		dev_err(rdma->dev, "invalid select-rdma-type[%d] to select sram\n",
			rdma->rdma_type);
		return -EINVAL;
	}

	if (sel_rdma_type == RDMA_TYPE_MDP) {
		sram_share_en = 1;
		sram_share_sel = 1;
	} else if (sel_rdma_type == RDMA_TYPE_PVRIC) {
		sram_share_en = 0;
		sram_share_sel = 0;
	} else if (sel_rdma_type == RDMA_TYPE_DISP) {
		sram_share_en = 1;
		sram_share_sel = 0;
	}

	rdma_writel_mask(rdma, PVRIC_SRC_CON,
			(sram_share_sel << 1) | sram_share_en,
			SRAM_SHARE_SEL | SRAM_SHARE_EN,
			cmdq_handle);

	return ret;
}
EXPORT_SYMBOL(mtk_rdma_sel_sram);

/** @ingroup IP_group_rdma_external_function
 * @par Description
 *     Config RDMA DMA ultra high for real time tasks
 * @param[in]
 *     dev: device node pointer of RDMA
 * @param[in]
 *     set_rt: 1 set ultra threshold, 0 unset
 * @param[out]
 *     cmdq_handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, all parameters are valid.\n
 *     -EINVAL, if dev is NULL.\n
 * @par Boundary case and Limitation
 *     None.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.\n
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rdma_gmc_rt(const struct device *dev,
				bool set_rt, struct cmdq_pkt **cmdq_handle)
{
	struct mtk_rdma *rdma;

	if (!dev)
		return -EINVAL;

	rdma = dev_get_drvdata(dev);
	if (!rdma)
		return -EINVAL;

	if (rdma->rdma_type != RDMA_TYPE_MDP)
		return 0;

	if (set_rt) {
		/* Enable GMC ultra */
		rdma_writel(rdma, MDP_RDMA_DMAULTRA_CON_0, 0xc0a0a080,
				cmdq_handle);
	} else {
		/* reset settings to init value */
		rdma_writel(rdma, MDP_RDMA_DMAULTRA_CON_0, 0, cmdq_handle);
	}

	return 0;
}
EXPORT_SYMBOL(mtk_rdma_gmc_rt);

/** @ingroup IP_group_rdma_external_function
 * @par Description
 *     Config RDMA ROI
 * @param[in]
 *     dev: device node pointer of RDMA
 * @param[in]
 *     eye_nr: 0, left/right 2 buf control; 1, otherwise
 * @param[out]
 *     cmdq_handle: CMDQ handle, set NULL for CPU write directly.
 * @param[in]
 *     configs: config parameters for each RDMA
 * @return
 *     0, all parameters are valid.\n
 *     -EINVAL, if dev is NULL.\n
 *     -EINVAL, if necessary config is NULL.
 * @par Boundary case and Limitation
 *     None.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.\n
 *     If config_l is NULL, return -EINVAL.\n
 *     If it's not single_buf but config_r is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rdma_config_frame(const struct device *dev, u32 eye_nr,
					struct cmdq_pkt **cmdq_handle,
					struct mtk_rdma_config **configs)
{
	struct mtk_rdma *rdma;
	int idx;
	struct mtk_rdma_config *config;
	u32 addr_offset_y = 0;
	u32 addr_offset_uv = 0;
	u32 idx_addr_offset_y = 0;
	u32 idx_addr_offset_uv = 0;
	u32 bitsPerPixelY = 0;
	u32 bitsPerPixelUV = 0;
	u32 pitch_in_pxl = 0;
	u32 src_addr[2] = { 0, 0};
	u32 src_ufo_addr[2] = { 0, 0};
	u32 src_con, src_con2, cnt_per_eye;
	u32 single_width;	/* width for each RDMA */

	if (!dev)
		return -EINVAL;
	if (!configs[0] || ((eye_nr > 1) && !configs[1]))
		return -EINVAL;

	rdma = dev_get_drvdata(dev);

	if (mtk_rdma_type_check(rdma))
		return -EINVAL;

	rdma->eye_nr = eye_nr;
	cnt_per_eye = rdma->module_cnt / eye_nr;

	if (rdma->rdma_type == RDMA_TYPE_MDP) {
		for (idx = 0; idx < rdma->module_cnt; idx++) {
			config = configs[idx / cnt_per_eye];
			single_width = config->width / cnt_per_eye;

			src_addr[0] = config->mem_addr[0];
			src_addr[1] = config->mem_addr[1];
			src_ufo_addr[0] = config->mem_ufo_len_addr[0];
			src_ufo_addr[1] = config->mem_ufo_len_addr[1];
			rdma->source_format = config->format;
			src_con2 = 0x0;	/* Disable RGB 10bit */

			switch (config->format) {
			case DRM_FORMAT_ARGB2101010:
				src_con = RDMA_FMT_BGRA8888;
				/* Enable RGB 10bit */
				src_con2 = ARGB_10BIT;
				pitch_in_pxl = (config->y_pitch << 2) / 5;
				bitsPerPixelY = 32;
				break;

			case DRM_FORMAT_ABGR2101010:
				src_con = RDMA_FMT_BGRA8888 | SWAP;
				/* Enable RGB 10bit */
				src_con2 = ARGB_10BIT;
				pitch_in_pxl = (config->y_pitch << 2) / 5;
				bitsPerPixelY = 32;
				break;

			case DRM_FORMAT_AYUV2101010:
				src_con = RDMA_FMT_BGRA8888;
				/* Enable RGB 10bit */
				src_con2 = ARGB_10BIT;
				pitch_in_pxl = (config->y_pitch << 2) / 5;
				bitsPerPixelY = 32;
				break;

			case DRM_FORMAT_RGBA1010102:
				src_con = RDMA_FMT_ARGB8888;
				/* Enable RGB 10bit */
				src_con2 = ARGB_10BIT;
				pitch_in_pxl = (config->y_pitch << 2) / 5;
				bitsPerPixelY = 32;
				break;

			case DRM_FORMAT_BGRA1010102:
				src_con = RDMA_FMT_ARGB8888 | SWAP;
				/* Enable RGB 10bit */
				src_con2 = ARGB_10BIT;
				pitch_in_pxl = (config->y_pitch << 2) / 5;
				bitsPerPixelY = 32;
				break;

			case DRM_FORMAT_ARGB8888:
				src_con = RDMA_FMT_BGRA8888;
				pitch_in_pxl = config->y_pitch;
				bitsPerPixelY = 32;
				break;

			case DRM_FORMAT_ABGR8888:
				src_con = RDMA_FMT_BGRA8888 | SWAP;
				pitch_in_pxl = config->y_pitch;
				bitsPerPixelY = 32;
				break;

			case DRM_FORMAT_RGB888:
				src_con = RDMA_FMT_RGB888;
				pitch_in_pxl = config->y_pitch;
				bitsPerPixelY = 24;
				break;

			case DRM_FORMAT_BGR888:
				src_con = RDMA_FMT_RGB888 | SWAP;
				pitch_in_pxl = config->y_pitch;
				bitsPerPixelY = 24;
				break;

			case DRM_FORMAT_R8:
				src_con = RDMA_FMT_Y8;
				pitch_in_pxl = config->y_pitch;
				bitsPerPixelY = 8;
				break;

			case DRM_FORMAT_R10:
				src_con = RDMA_FMT_Y8;
				/* Enable RGB 10bit */
				src_con2 = Y_ONLY_10BIT;
				pitch_in_pxl = (config->y_pitch << 2) / 5;
				bitsPerPixelY = 10;
				break;

			case DRM_FORMAT_YUYV:
				src_con = RDMA_FMT_YUY2;
				pitch_in_pxl = config->y_pitch;
				bitsPerPixelY = 16;
				break;

			default:
				dev_err(dev, "Unrecognizable color format: %d\n",
					config->format);
				return -EINVAL;
			}
			addr_offset_y = (config->width * bitsPerPixelY) /
					(cnt_per_eye * 8);
			addr_offset_uv = (config->width * bitsPerPixelUV) /
					(cnt_per_eye * 8);
			idx_addr_offset_y =
				(idx % cnt_per_eye) * addr_offset_y;
			idx_addr_offset_uv =
				(idx % cnt_per_eye) * addr_offset_uv;
			/* Setup source buffer base */
			dev_info(rdma->dev, "%s idx = %d offset_y = %d offset_uv = %d\n",
				__func__, idx,
				idx_addr_offset_y, idx_addr_offset_uv);
			rdma_writel_s(rdma, idx, MDP_RDMA_SRC_BASE_0,
				src_addr[0], cmdq_handle);
			/* Setup source buffer offset */
			rdma_writel_s(rdma, idx, MDP_RDMA_SRC_OFFSET_0,
				idx_addr_offset_y, cmdq_handle);
			/* Setup source frame pitch */
			rdma_writel_s(rdma, idx, MDP_RDMA_MF_BKGD_SIZE_IN_BYTE,
				config->y_pitch, cmdq_handle);
			rdma_writel_s(rdma, idx, MDP_RDMA_MF_SRC_SIZE,
				(config->height << 16 | single_width),
				cmdq_handle);
			rdma_writel_s(rdma, idx, MDP_RDMA_MF_CLIP_SIZE,
				(config->height << 16 | single_width),
				cmdq_handle);
			rdma_writel_s(rdma, idx, MDP_RDMA_MF_OFFSET_1,
				0x0, cmdq_handle);
			rdma_writel_s(rdma, idx, MDP_RDMA_MF_BKGD_SIZE_IN_PXL,
				pitch_in_pxl, cmdq_handle);

			rdma_writel_mask_s(rdma, idx, MDP_RDMA_SRC_CON, src_con,
				SWAP | SRC_FORMAT,
				cmdq_handle);

			rdma_writel_s(rdma, idx, MDP_RDMA_SRC_CON2, src_con2,
				cmdq_handle);
		}
	} else if (rdma->rdma_type == RDMA_TYPE_PVRIC) {
		unsigned int pvric_fmt = 0;
		unsigned int pvric_argb_output = 0;
		unsigned int pvric_10Bit = 0;
		unsigned int pvric_swap_8Bit = 0;
		unsigned int pvric_swap_10Bit = 0;

		for (idx = 0; idx < rdma->module_cnt; idx++) {
			config = configs[idx / cnt_per_eye];
			single_width = config->width / cnt_per_eye;

			switch (config->format) {
			/* here RGB888 just like XRGB8888 */
			case DRM_FORMAT_RGB888:
			case DRM_FORMAT_ARGB8888:
				pvric_fmt = 0xc;
				pvric_argb_output = 1;
				pvric_10Bit = 0;
				break;
			/* here BGR888 just like XBGR8888 */
			case DRM_FORMAT_BGR888:
			case DRM_FORMAT_ABGR8888:
				pvric_fmt = 0xc;
				pvric_argb_output = 1;
				pvric_10Bit = 0;
				pvric_swap_8Bit = 5;
				break;
			case DRM_FORMAT_ARGB2101010:
				pvric_fmt = 0xe;
				pvric_argb_output = 0;
				pvric_10Bit = 1;
				break;
			case DRM_FORMAT_ABGR2101010:
				pvric_fmt = 0xe;
				pvric_argb_output = 0;
				pvric_10Bit = 1;
				pvric_swap_10Bit = 1;
				break;
			case DRM_FORMAT_RGBA1010102:
				pvric_fmt = 0x55;
				pvric_argb_output = 0;
				pvric_10Bit = 1;
				break;
			case DRM_FORMAT_BGRA1010102:
				pvric_fmt = 0x55;
				pvric_argb_output = 0;
				pvric_10Bit = 1;
				pvric_swap_10Bit = 1;
				break;
			default:
				pvric_fmt = 0xc;
				pvric_argb_output = 1;
				pvric_10Bit = 0;
				break;
			}

			rdma_writel_mask_s(rdma, idx, PVRIC_SRC_CON,
				((pvric_swap_10Bit << 4) |
					(pvric_fmt << 8) |
					(pvric_argb_output << 25)),
				(SRC_SWAP | PVRIC_FORMAT | OUTPUT_ARGB),
				cmdq_handle);
			rdma_writel_mask_s(rdma, idx, PVRIC_POST_PROC_SRC_CON,
				((pvric_10Bit << 27) | (pvric_swap_8Bit << 28)),
				(OUT_10B | ARGB_SWAP),
				cmdq_handle);
			rdma_writel_s(rdma, idx, PVRIC_BASE_ADDR_MSB,
				0, cmdq_handle);
			rdma_writel_s(rdma, idx, PVRIC_BASE_ADDR_LSB,
				config->mem_addr[0] >> 6, cmdq_handle);
			rdma_writel_s(rdma, idx, PVRIC_START_OFFSET,
				single_width * (idx % cnt_per_eye) / 8,
				cmdq_handle);
			rdma_writel_s(rdma, idx, PVRIC_TILE_SRC_WIDTH,
				single_width / 8, cmdq_handle);
			rdma_writel_s(rdma, idx, PVRIC_TILE_SRC_HEIGHT,
				config->height / 8, cmdq_handle);
			rdma_writel_s(rdma, idx, PVRIC_OFFSET_PITCH,
				config->y_pitch / 8, cmdq_handle);
			rdma_writel_mask_s(rdma, idx, PVRIC_POST_PROC_SRC_CON3,
				(0 | (single_width << 13) | (single_width)),
				(CLIP_OFFSET_W | CLIP_W | SRC_W),
				cmdq_handle);
			rdma_writel_mask_s(rdma, idx, PVRIC_POST_PROC_SRC_CON4,
				(0 | (config->height << 13) | (config->height)),
				(CLIP_OFFSET_H | CLIP_H | SRC_H),
				cmdq_handle);
		}
	} else if (rdma->rdma_type == RDMA_TYPE_DISP) {
		for (idx = 0; idx < rdma->module_cnt; idx++) {
			config = configs[idx / cnt_per_eye];
			single_width = config->width / cnt_per_eye;

			rdma_writel_mask_s(rdma, idx, DISP_RDMA_GLOBAL_CON,
				DISP_RDMA_PIXEL_10_BIT, DISP_RDMA_PIXEL_10_BIT,
				cmdq_handle);
			rdma_writel_mask_s(rdma, idx, DISP_RDMA_SIZE_CON_0,
				single_width, DISP_RDMA_OUTPUT_FRAME_WIDTH,
				cmdq_handle);
			rdma_writel_mask_s(rdma, idx, DISP_RDMA_SIZE_CON_1,
				config->height, DISP_RDMA_OUTPUT_FRAME_HEIGHT,
				cmdq_handle);
			rdma_writel_mask_s(rdma, idx, DISP_RDMA_FIFO_CON,
				(0xc00 << 16), DISP_RDMA_FIFO_PSEUDO_SIZE,
				cmdq_handle);
			rdma_writel_mask_s(rdma, idx, DISP_RDMA_SRAM_SEL,
				3, DISP_RDMA_BIT_SRAM_SEL,
				cmdq_handle);
			rdma_writel_mask_s(rdma, idx, DISP_RDMA_SRAM_CASCADE,
				(0x400 | (0x800 << 16)),
				DISP_RDMA_SRAM_SIZE_0 | DISP_RDMA_SRAM_SIZE_1,
				cmdq_handle);
			rdma_writel_mask_s(rdma, idx, DISP_RDMA_FIFO_CON,
				0, DISP_RDMA_FIFO_UNDERFLOW_EN,
				cmdq_handle);
		}
	}

	return 0;
}
EXPORT_SYMBOL(mtk_rdma_config_frame);

 /** @ingroup IP_group_rdma_external_function
 * @par Description
 *     Config RDMA ROI
 * @param[in]
 *     dev: device node pointer of RDMA
 * @param[out]
 *     cmdq_handle: CMDQ handle, set NULL for CPU write directly.
 * @param[in]
 *     config: ROI information for each RDMA
 * @return
 *     0, all parameters are valid.\n
 *     -EINVAL, if dev is NULL.\n
 *     -EINVAL, if necessary config is NULL.
 * @par Boundary case and Limitation
 *     None.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.\n
 *     If config is NULL, return -EINVAL.\n
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rdma_config_quarter(const struct device *dev,
			       struct cmdq_pkt **cmdq_handle,
			       struct mtk_rdma_frame_adv_config *config)
{
	struct mtk_rdma *rdma;
	u32 src_w[4];
	u32 src_h[4];
	u32 clip_w[4];
	u32 clip_h[4];
	u32 clip_offset[4];
	u32 src_offset_y[4];
	u32 src_offset_uv[4];
	u32 src_offset_y_p[4];
	u32 bitsPerPixelY;
	u32 bitsPerPixelUV = 0;
	u32 horizontalShiftUV = 0;
	u32 in_x_left;
	u32 resv_size_2, resv_size_3;
	int idx;

	if (!dev)
		return -EINVAL;
	if (!config)
		return -EINVAL;

	rdma = dev_get_drvdata(dev);

	if (mtk_rdma_type_check(rdma))
		return -EINVAL;

	if (rdma->rdma_type == RDMA_TYPE_MDP) {
		switch (rdma->source_format) {
		case DRM_FORMAT_ARGB2101010:
		case DRM_FORMAT_ABGR2101010:
		case DRM_FORMAT_RGBA1010102:
		case DRM_FORMAT_BGRA1010102:
		case DRM_FORMAT_AYUV2101010:
		case DRM_FORMAT_ARGB8888:
		case DRM_FORMAT_ABGR8888:
			bitsPerPixelY = 32;
			break;
		case DRM_FORMAT_RGB888:
		case DRM_FORMAT_BGR888:
			bitsPerPixelY = 24;
			break;
		case DRM_FORMAT_R8:
			bitsPerPixelY = 8;
			break;
		case DRM_FORMAT_R10:
			bitsPerPixelY = 10;
			break;
		default:
			dev_err(dev, "Unrecognizable color format: %d\n",
				rdma->source_format);
			return -EINVAL;
		}

		for (idx = 0; idx < rdma->module_cnt; idx++) {
			in_x_left = config->in_x_left[idx];
			src_offset_y[idx] =
				(in_x_left * bitsPerPixelY) >> 3;
			src_offset_uv[idx] =
				(((in_x_left >> horizontalShiftUV) *
				bitsPerPixelUV) >> 3);
			src_offset_y_p[idx] =
				(rdma_is_10_bit_fmt(
					rdma->source_format)) ?
					((src_offset_y[idx] << 2) / 5) :
					src_offset_y[idx];

			/* source size */
			src_w[idx] =
			    config->in_x_right[idx] - in_x_left + 1;
			src_h[idx] = config->in_height;
			/* target size */
			clip_w[idx] =
			    config->out_x_right[idx] -
			    config->out_x_left[idx] + 1;
			clip_h[idx] = config->out_height;

			clip_offset[idx] =
			    config->out_x_left[idx] - in_x_left;
		}

		/* Notice: LL(0)-LR(1), RL(2)-RR(3) */
		for (idx = 0; idx < rdma->module_cnt; idx++) {
			rdma_writel_s(rdma, idx, MDP_RDMA_SRC_OFFSET_0,
				      src_offset_y[idx], cmdq_handle);
			rdma_writel_s(rdma, idx, MDP_RDMA_MF_SRC_SIZE,
				      (src_h[idx] << 16) + (src_w[idx] << 0),
				      cmdq_handle);
			rdma_writel_s(rdma, idx, MDP_RDMA_MF_CLIP_SIZE,
				      (clip_h[idx] << 16) + (clip_w[idx] << 0),
				      cmdq_handle);
			rdma_writel_s(rdma, idx, MDP_RDMA_MF_OFFSET_1,
				      clip_offset[idx], cmdq_handle);
			resv_size_2 = 64 - (8 * (src_w[idx] / 128));
			resv_size_3 = 64 - (8 * (src_w[idx] / 256));
		}
	} else if (rdma->rdma_type == RDMA_TYPE_PVRIC) {
		for (idx = 0; idx < rdma->module_cnt; idx++) {
			rdma_writel_s(rdma, idx, PVRIC_START_OFFSET,
				config->in_x_left[idx] / 8, cmdq_handle);
			rdma_writel_s(rdma, idx, PVRIC_TILE_SRC_WIDTH,
				config->in_width / 8, cmdq_handle);
			rdma_writel_s(rdma, idx, PVRIC_TILE_SRC_HEIGHT,
				config->in_height / 8, cmdq_handle);
			rdma_writel_mask_s(rdma, idx, PVRIC_POST_PROC_SRC_CON3,
				0 | (config->in_width << 13) | config->in_width,
				CLIP_OFFSET_W | CLIP_W | SRC_W,
				cmdq_handle);
			rdma_writel_mask_s(rdma, idx, PVRIC_POST_PROC_SRC_CON4,
				0 | (config->in_height << 13),
				CLIP_OFFSET_H | CLIP_H,
				cmdq_handle);
		}
	} else if (rdma->rdma_type == RDMA_TYPE_DISP) {
		/* will add code here to support disp-rdma */
	}

	return 0;
}
EXPORT_SYMBOL(mtk_rdma_config_quarter);

/** @ingroup IP_group_rdma_external_function
 * @par Description
 *     Config RDMA input address
 * @param[in]
 *     dev: device node pointer of RDMA
 * @param[out]
 *     cmdq_handle: CMDQ handle, set NULL for CPU write directly.
 * @param[in]
 *     param: buffer address and size for RDMA to read
 * @return
 *     0, all parameters are valid.\n
 *     -EINVAL, if dev is NULL.\n
 *     -EINVAL, if necessary param is NULL.
 * @par Boundary case and Limitation
 *     None.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.\n
 *     If param is NULL, return -EINVAL.\n
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rdma_config_srcaddr(const struct device *dev,
				      struct cmdq_pkt **cmdq_handle,
				      struct mtk_rdma_src *param)
{
	int idx;
	u32 cnt_per_eye;
	struct mtk_rdma *rdma;

	if (!dev)
		return -EINVAL;
	if (!param)
		return -EINVAL;

	rdma = dev_get_drvdata(dev);
	if (!rdma)
		return -EINVAL;

	cnt_per_eye = rdma->module_cnt / rdma->eye_nr;

	if (rdma->rdma_type == RDMA_TYPE_MDP) {
		/* Setup source buffer base */
		rdma_writel(rdma, MDP_RDMA_SRC_BASE_0, param->mem_addr[0],
			    cmdq_handle);

		/* Setup source buffer end */
		rdma_writel(rdma, MDP_RDMA_SRC_END_0,
			    param->mem_addr[0] + param->mem_size[0],
			    cmdq_handle);
	} else if (rdma->rdma_type == RDMA_TYPE_PVRIC) {
		for (idx = 0; idx < rdma->module_cnt; idx++) {
			rdma_writel_s(rdma, idx, PVRIC_BASE_ADDR_MSB, 0,
				cmdq_handle);
			rdma_writel_s(rdma, idx,
				PVRIC_BASE_ADDR_LSB,
				param[idx / cnt_per_eye].mem_addr[0] >> 6,
				cmdq_handle);
		}
	} else
		dev_err(rdma->dev, "invalid rdma type[%d] in func[%s]\n",
			rdma->rdma_type, __func__);

	return 0;
}
EXPORT_SYMBOL(mtk_rdma_config_srcaddr);

/** @ingroup IP_group_rdma_external_function
 * @par Description
 *     Config RDMA target line
 * @param[in]
 *     dev: device node pointer of RDMA
 * @param[out]
 *     cmdq_handle: CMDQ handle, set NULL for CPU write directly.
 * @param[in]
 *     target_line_count: target line count to set
 * @return
 *     0, all parameters are valid.\n
 *     -EINVAL, if dev is NULL.
 * @par Boundary case and Limitation
 *     None.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rdma_set_target_line(
		const struct device *dev,
		struct cmdq_pkt **cmdq_handle,
		unsigned int target_line_count)
{
	struct mtk_rdma *rdma;

	if (!dev)
		return -EINVAL;

	rdma = dev_get_drvdata(dev);
	if (!rdma)
		return -EINVAL;

	if (rdma->rdma_type == RDMA_TYPE_MDP)
		rdma_writel_mask(rdma, MDP_RDMA_TARGET_LINE,
			target_line_count, TARGET_LINE,
			cmdq_handle);
	else if (rdma->rdma_type == RDMA_TYPE_PVRIC)
		rdma_writel_mask(rdma, PVRIC_TARGET_LINE,
			target_line_count, TARGET_LINE,
			cmdq_handle);
	else {
		dev_err(rdma->dev, "invalid rdma type[%d] in func[%s]\n",
			rdma->rdma_type, __func__);
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(mtk_rdma_set_target_line);

#ifdef CONFIG_MTK_DEBUGFS
static char *mtk_rdma_debug_mdp_fmt_to_str(u32 fmt)
{
	switch (fmt) {
	case 0:
		return "RGB565";
	case 1:
		return "RGB888";
	case 2:
		return "BGRA8888";
	case 3:
		return "ARGB8888";
	case 4:
		return "UYVY";
	case 5:
		return "YUY2";
	case 7:
		return "Y8";
	case 8:
		return "YV12";
	case 9:
		return "YV16";
	case 10:
		return "YV24";
	case 12:
		return "NV12";
	case 13:
		return "NV16";
	case 14:
		return "NV24";
	default:
		return "unknown";
	}
}

static void mtk_rdma_debug_print_reg_for_mdp_rdma(struct mtk_rdma *rdma)
{
	u32 i;
	u32 reg;

	for (i = 0; i < MTK_MM_MODULE_MAX_NUM; i++) {
		if (rdma->regs[i] == NULL) {
			dev_err(rdma->dev, "mdp_rdma[%d] reg base is NULL\n",
				i);
			continue;
		}

		dev_err(rdma->dev, "******** start to print mdp_rdma[%d] reg value ********\n",
			i);

		reg = readl(rdma->regs[i] + MDP_RDMA_EN);
		dev_err(rdma->dev, "rdma enable                    : %10u\n",
			(reg & ROT_ENABLE) ? 1 : 0);

		reg = readl(rdma->regs[i] + MDP_RDMA_INTERRUPT_ENABLE);
		dev_err(rdma->dev, "frame_done irq enable          : %10u\n",
			(reg & FRAME_COMPLETE_INT_EN) ? 1 : 0);
		dev_err(rdma->dev, "reg_update irq enable          : %10u\n",
			(reg & REG_UPDATE_INT_EN) ? 1 : 0);
		dev_err(rdma->dev, "under_run irq enable           : %10u\n",
			(reg & UNDERRUN_INT_EN) ? 1 : 0);

		reg = readl(rdma->regs[i] + MDP_RDMA_INTERRUPT_STATUS);
		dev_err(rdma->dev, "frame_done irq raise           : %10u\n",
			(reg & FRAME_COMPLETE_INT) ? 1 : 0);
		dev_err(rdma->dev, "reg_update irq raise           : %10u\n",
			(reg & REG_UPDATE_INT) ? 1 : 0);
		dev_err(rdma->dev, "under_run irq raise            : %10u\n",
			(reg & UNDERRUN_INT) ? 1 : 0);

		reg = readl(rdma->regs[i] + MDP_RDMA_SRC_CON);
		dev_err(rdma->dev, "output_argb                    : %10u\n",
			(reg & OUTPUT_ARGB) ? 1 : 0);
		dev_err(rdma->dev, "swap                           : %10u\n",
			(reg & SWAP) ? 1 : 0);

		dev_err(rdma->dev, "src_format                     : %s\n",
			mtk_rdma_debug_mdp_fmt_to_str(reg & SRC_FORMAT));

		reg = readl(rdma->regs[i] + MDP_RDMA_SRC_CON2);
		dev_err(rdma->dev, "argb_10bit                     : %10u\n",
			(reg & ARGB_10BIT) ? 1 : 0);

		reg = readl(rdma->regs[i] + MDP_RDMA_MF_SRC_SIZE);
		dev_err(rdma->dev, "src_width                      : %10lu\n",
			reg & MF_SRC_W);
		dev_err(rdma->dev, "src_height                     : %10lu\n",
			(reg & MF_SRC_H) >> 16);

		reg = readl(rdma->regs[i] + MDP_RDMA_MF_CLIP_SIZE);
		dev_err(rdma->dev, "clip_width                     : %10lu\n",
			reg & MF_CLIP_W);
		dev_err(rdma->dev, "clip_height                    : %10lu\n",
			(reg & MF_CLIP_H) >> 16);

		reg = readl(rdma->regs[i] + MDP_RDMA_MF_OFFSET_1);
		dev_err(rdma->dev, "offset_w                       : %10lu\n",
			reg & MF_OFFSET_W_1);
		dev_err(rdma->dev, "offset_h                       : %10lu\n",
			(reg & MF_OFFSET_H_1) >> 16);

		reg = readl(rdma->regs[i] + MDP_RDMA_SRC_END_0);
		dev_err(rdma->dev, "src_end                        : 0x%08x\n",
			reg);

		reg = readl(rdma->regs[i] + MDP_RDMA_SRC_OFFSET_0);
		dev_err(rdma->dev, "src_offset                     : 0x%08x\n",
			reg);

		reg = readl(rdma->regs[i] + MDP_RDMA_SRC_BASE_0);
		dev_err(rdma->dev, "src_base                       : 0x%08x\n",
			reg);

		reg = readl(rdma->regs[i] + MDP_RDMA_SMI_CRC_CON);
		dev_err(rdma->dev, "crc_en                         : %10u\n",
			(reg & CRC_EN) ? 1 : 0);

		reg = readl(rdma->regs[i] + MDP_RDMA_SMI_CRC_PRE);
		dev_err(rdma->dev, "crc_pre                        : 0x%08lx\n",
			reg & CRC_PRE);

		reg = readl(rdma->regs[i] + MDP_RDMA_SMI_CRC_CUR);
		dev_err(rdma->dev, "crc_cur                        : 0x%08lx\n",
			reg & CRC_CUR);

		dev_err(rdma->dev, "******** end to print mdp_rdma[%d] reg value ********\n",
			i);
	}
}

static char *mtk_rdma_debug_pvric_fmt_to_str(u32 fmt)
{
	switch (fmt) {
	case 0xc:
		return "ARGB8888";
	case 0xe:
		return "ARGB2101010";
	case 0x55:
		return "RGBA1010102";
	default:
		return "unknown";
	}
}

static char *mtk_rdma_debug_pvric_swap_to_str(u32 swap)
{
	static char str[20];

	switch (swap) {
	case 0:
		strcpy(str, "YUV/RGB");
		break;
	case 1:
		strcpy(str, "VUY/BGR, R/B swap");
		break;
	case 2:
		strcpy(str, "YVU/RBG, U/V swap");
		break;
	case 3:
		strcpy(str, "UVY/GBR");
		break;
	case 4:
		strcpy(str, "UYV/GRB");
		break;
	case 5:
		strcpy(str, "VYU/BRG");
		break;
	default:
		strcpy(str, "unknown");
		break;
	}

	return str;
}

static char *mtk_rdma_debug_pvric_argb_swap_to_str(u32 swap)
{
	static char str[20];

	switch (swap) {
	case 0:
		strcpy(str, "ARGB");
		break;
	case 1:
		strcpy(str, "ARBG");
		break;
	case 2:
		strcpy(str, "AGRB");
		break;
	case 3:
		strcpy(str, "ABRG");
		break;
	case 4:
		strcpy(str, "AGBR");
		break;
	case 5:
		strcpy(str, "ABGR");
		break;
	case 8:
		strcpy(str, "RGBA");
		break;
	case 9:
		strcpy(str, "RBGA");
		break;
	case 10:
		strcpy(str, "GRBA");
		break;
	case 11:
		strcpy(str, "BRGA");
		break;
	case 12:
		strcpy(str, "GBRA");
		break;
	case 13:
		strcpy(str, "BGRA");
		break;
	default:
		strcpy(str, "ARGB");
		break;
	}

	return str;
}

static void mtk_rdma_debug_print_reg_for_pvric_rdma(struct mtk_rdma *rdma)
{
	u32 i;
	u32 reg;

	for (i = 0; i < MTK_MM_MODULE_MAX_NUM; i++) {
		if (rdma->regs[i] == NULL) {
			dev_err(rdma->dev, "pvric_rdma[%d] reg base is NULL\n",
				i);
			continue;
		}

		dev_err(rdma->dev, "******** start to print pvric_rdma[%d] reg value ********\n",
			i);

		reg = readl(rdma->regs[i] + PVRIC_EN);
		dev_err(rdma->dev, "rdma enable                    : %10u\n",
			(reg & ROT_ENABLE) ? 1 : 0);

		reg = readl(rdma->regs[i] + PVRIC_INTERRUPT_ENABLE);
		dev_err(rdma->dev, "frame_done irq enable          : %10u\n",
			(reg & FRAME_COMPLETE_INT_EN) ? 1 : 0);
		dev_err(rdma->dev, "reg_update irq enable          : %10u\n",
			(reg & REG_UPDATE_INT_EN) ? 1 : 0);
		dev_err(rdma->dev, "under_run irq enable           : %10u\n",
			(reg & UNDERRUN_INT_EN) ? 1 : 0);

		reg = readl(rdma->regs[i] + PVRIC_INTERRUPT_STATUS);
		dev_err(rdma->dev, "frame_done irq raise           : %10u\n",
			(reg & FRAME_COMPLETE_INT) ? 1 : 0);
		dev_err(rdma->dev, "reg_update irq raise           : %10u\n",
			(reg & REG_UPDATE_INT) ? 1 : 0);
		dev_err(rdma->dev, "under_run irq raise            : %10u\n",
			(reg & UNDERRUN_INT) ? 1 : 0);

		reg = readl(rdma->regs[i] + PVRIC_SRC_CON);
		dev_err(rdma->dev, "output_argb                    : %10u\n",
			(reg & OUTPUT_ARGB) ? 1 : 0);

		dev_err(rdma->dev, "src_format                     : %s\n",
			mtk_rdma_debug_pvric_fmt_to_str(
				(reg & PVRIC_FORMAT) >> 8));

		dev_err(rdma->dev, "src_swap                       : %s\n",
			mtk_rdma_debug_pvric_swap_to_str(
				(reg & SRC_SWAP) >> 4));

		dev_err(rdma->dev, "sram_share_en                  : %10u\n",
			(reg & SRAM_SHARE_EN) ? 1 : 0);

		dev_err(rdma->dev, "sram_share_sel                 : %s\n",
			(reg & SRAM_SHARE_SEL) ?
				"mdp_rdma" : "disp_rdma");

		reg = readl(rdma->regs[i] + PVRIC_BASE_ADDR_LSB);
		dev_err(rdma->dev, "base_addr_lsb                  : 0x%08lx\n",
			reg & BASE_ADDR_LSB);

		reg = readl(rdma->regs[i] + PVRIC_START_OFFSET);
		dev_err(rdma->dev, "start_offset                   : 0x%08lx\n",
			reg & START_OFFSET);

		reg = readl(rdma->regs[i] + PVRIC_TILE_SRC_WIDTH);
		dev_err(rdma->dev, "tile_src_width                 : %10lu\n",
			reg & TILE_SRC_WIDTH);

		reg = readl(rdma->regs[i] + PVRIC_TILE_SRC_HEIGHT);
		dev_err(rdma->dev, "tile_src_height                : %10lu\n",
			reg & TILE_SRC_HEIGHT);

		reg = readl(rdma->regs[i] + PVRIC_OFFSET_PITCH);
		dev_err(rdma->dev, "offset_pitch                   : %10lu\n",
			reg & OFFSET_PITCH);

		reg = readl(rdma->regs[i] + PVRIC_POST_PROC_SRC_CON);
		dev_err(rdma->dev, "argb_swap                      : %s\n",
			mtk_rdma_debug_pvric_argb_swap_to_str(
				(reg & ARGB_SWAP) >> 28));
		dev_err(rdma->dev, "out_10B                        : %10u\n",
			(reg & OUT_10B) ? 1 : 0);

		reg = readl(rdma->regs[i] + PVRIC_POST_PROC_SRC_CON3);
		dev_err(rdma->dev, "src_w                          : %10lu\n",
			reg & SRC_W);
		dev_err(rdma->dev, "clip_w                         : %10lu\n",
			(reg & CLIP_W) >> 13);
		dev_err(rdma->dev, "clip_offset_w                  : %10lu\n",
			(reg & CLIP_OFFSET_W) >> 26);

		reg = readl(rdma->regs[i] + PVRIC_POST_PROC_SRC_CON4);
		dev_err(rdma->dev, "src_h                          : %10lu\n",
			reg & SRC_H);
		dev_err(rdma->dev, "clip_h                         : %10lu\n",
			(reg & CLIP_H) >> 13);
		dev_err(rdma->dev, "clip_offset_h                  : %10lu\n",
			(reg & CLIP_OFFSET_H) >> 26);

		reg = readl(rdma->regs[i] + PVRIC_SMI_CRC_CON);
		dev_err(rdma->dev, "crc_en                         : %10u\n",
			(reg & CRC_EN) ? 1 : 0);

		reg = readl(rdma->regs[i] + PVRIC_SMI_CRC_PRE);
		dev_err(rdma->dev, "crc_pre                        : 0x%08lx\n",
			reg & CRC_PRE);

		reg = readl(rdma->regs[i] + PVRIC_SMI_CRC_CUR);
		dev_err(rdma->dev, "crc_cur                        : 0x%08lx\n",
			reg & CRC_CUR);

		dev_err(rdma->dev, "******** end to print pvric_rdma[%d] reg value ********\n",
			i);
	}
}

static void mtk_rdma_debug_print_reg_for_disp_rdma(struct mtk_rdma *rdma)
{
	u32 i;
	u32 reg;

	for (i = 0; i < MTK_MM_MODULE_MAX_NUM; i++) {
		if (rdma->regs[i] == NULL) {
			dev_err(rdma->dev, "disp_rdma[%d] reg base is NULL\n",
				i);
			continue;
		}

		dev_err(rdma->dev, "******** start to print disp_rdma[%d] reg value ********\n",
			i);

		reg = readl(rdma->regs[i] + DISP_RDMA_INT_ENABLE);
		dev_err(rdma->dev, "reg_update irq enable          : %10u\n",
			(reg & DISP_RDMA_REG_UPDATE_INT_EN) ? 1 : 0);
		dev_err(rdma->dev, "frame_end irq enable           : %10u\n",
			(reg & DISP_RDMA_FRAME_END_INT_EN) ? 1 : 0);

		reg = readl(rdma->regs[i] + DISP_RDMA_INT_STATUS);
		dev_err(rdma->dev, "reg_update irq raise           : %10u\n",
			(reg & DISP_RDMA_REG_UPDATE_INT_FLAG) ? 1 : 0);
		dev_err(rdma->dev, "frame_end irq raise            : %10u\n",
			(reg & DISP_RDMA_FRAME_END_INT_FLAG) ? 1 : 0);

		reg = readl(rdma->regs[i] + DISP_RDMA_GLOBAL_CON);
		dev_err(rdma->dev, "engine_en                      : %10u\n",
			(reg & DISP_RDMA_ENGINE_EN) ? 1 : 0);
		dev_err(rdma->dev, "pixel_10_bit                   : %10u\n",
			(reg & DISP_RDMA_PIXEL_10_BIT) ? 1 : 0);

		reg = readl(rdma->regs[i] + DISP_RDMA_SIZE_CON_0);
		dev_err(rdma->dev, "output_frame_width             : %10lu\n",
			reg & DISP_RDMA_OUTPUT_FRAME_WIDTH);

		reg = readl(rdma->regs[i] + DISP_RDMA_SIZE_CON_1);
		dev_err(rdma->dev, "output_frame_height            : %10lu\n",
			reg & DISP_RDMA_OUTPUT_FRAME_HEIGHT);

		reg = readl(rdma->regs[i] + DISP_RDMA_FIFO_CON);
		dev_err(rdma->dev, "fifo_undeflow_en               : %10u\n",
			(reg & DISP_RDMA_FIFO_UNDERFLOW_EN) ? 1 : 0);
		dev_err(rdma->dev, "fifo_pseudo_size               : 0x%08lx\n",
			(reg & DISP_RDMA_FIFO_PSEUDO_SIZE) >> 16);

		reg = readl(rdma->regs[i] + DISP_RDMA_SRAM_SEL);
		dev_err(rdma->dev, "sram_sel                       : %s\n",
			((reg & DISP_RDMA_BIT_SRAM_SEL) == 3) ?
				"sram share with other rdma" : "unknown");

		reg = readl(rdma->regs[i] + DISP_RDMA_SRAM_CASCADE);
		dev_err(rdma->dev, "sram_size_0                    : 0x%08lx\n",
			reg & DISP_RDMA_SRAM_SIZE_0);
		dev_err(rdma->dev, "sram_size_1                    : 0x%08lx\n",
			(reg & DISP_RDMA_SRAM_SIZE_1) >> 16);

		dev_err(rdma->dev, "******** end to print disp_rdma[%d] reg value ********\n",
			i);
	}
}

static void mtk_rdma_debug_print_reg(struct mtk_rdma *rdma)
{
	if (rdma == NULL) {
		pr_err("NULL rdma\n");
		return;
	}

	if (rdma->rdma_type > RDMA_TYPE_DISP) {
		dev_err(rdma->dev, "invalid rdma type[%d] in func[%s]\n",
			rdma->rdma_type, __func__);
		return;
	}

	if (rdma->rdma_type == RDMA_TYPE_MDP)
		mtk_rdma_debug_print_reg_for_mdp_rdma(rdma);
	else if (rdma->rdma_type == RDMA_TYPE_PVRIC)
		mtk_rdma_debug_print_reg_for_pvric_rdma(rdma);
	else if (rdma->rdma_type == RDMA_TYPE_DISP)
		mtk_rdma_debug_print_reg_for_disp_rdma(rdma);
}

static void mtk_rdma_debug_print_info(
	struct mtk_rdma *rdma, char __user *ubuf, size_t count, loff_t *ppos)
{
	int len = 0;
	int size = 200;
	char *usage;

	usage = kzalloc(size, GFP_KERNEL);
	if (usage == NULL)
		return;

	len += scnprintf(usage + len, size - len, "[USAGE]\n");
	len += scnprintf(usage + len, size - len,
		"   echo [ACTION]... > /sys/kernel/debug/mediatek/rdma/%s\n",
		rdma->file_name);
	len += scnprintf(usage + len, size - len, "[ACTION]\n");
	len += scnprintf(usage + len, size - len, "   reg_r <offset> <size>\n");

	if (len > size)
		len = size;
	simple_read_from_buffer(ubuf, count, ppos, usage, len);
	kfree(usage);
}

static ssize_t mtk_rdma_debug_read(struct file *file, char __user *ubuf,
				size_t count, loff_t *ppos)
{
	struct mtk_rdma *rdma = file->private_data;

	ssize_t ret = 0;

	mtk_rdma_debug_print_reg(rdma);
	mtk_rdma_debug_print_info(rdma, ubuf, count, ppos);

	return ret;
}

static void mtk_rdma_debug_reg_read(struct mtk_rdma *rdma, u32 offset,
				     u32 size)
{
	void *r;
	u32 i, j, t1, t2;

	if (offset >= 0x1000 || (offset + size) > 0x1000) {
		dev_err(rdma->dev, "Specified register range overflow!\n");
		return;
	}

	t1 = rounddown(offset, 16);
	t2 = roundup(offset + size, 16);

	dev_err(rdma->dev, "---------------- %s reg_r 0x%x 0x%x ----------------\n",
		rdma->file_name, offset, size);
	for (i = 0; i < rdma->module_cnt; i++) {
		if (rdma->module_cnt > 1)
			dev_err(rdma->dev, "-------- %s[%d]\n",
				rdma->file_name, i);
		for (j = t1; j < t2; j += 16) {
			r = rdma->regs[i] + j;
			dev_err(rdma->dev, "%X | %08X %08X %08X %08X\n",
				rdma->reg_base[i] + j,
				readl(r), readl(r + 4),
				readl(r + 8), readl(r + 12));
		}
	}
	dev_err(rdma->dev, "---------------------------------------------------\n");
}

static ssize_t mtk_rdma_debug_write(struct file *file, const char __user *ubuf,
				   size_t count, loff_t *ppos)
{
	struct mtk_rdma *rdma = file->private_data;
	char buf[64];
	size_t buf_size = min(count, sizeof(buf) - 1);

	if (copy_from_user(buf, ubuf, buf_size))
		return -EFAULT;
	buf[buf_size] = '\0';

	dev_info(rdma->dev, "debug cmd: %s\n", buf);
	if (strncmp(buf, "reg_r", 5) == 0) {
		int ret;
		u32 offset = 0, size = 16;

		ret = sscanf(buf, "reg_r %x %x", &offset, &size);
		if (ret == 2)
			mtk_rdma_debug_reg_read(rdma, offset, size);
		else
			dev_err(rdma->dev, "Correct your input! (reg_r <offset> <size>)\n");

	} else {
		dev_err(rdma->dev, "unknown command %s\n", buf);
	}

	return count;
}

static const struct file_operations mtk_rdma_debug_fops = {
	.owner = THIS_MODULE,
	.open		= simple_open,
	.read		= mtk_rdma_debug_read,
	.write		= mtk_rdma_debug_write,
	.llseek		= default_llseek,
};

static void mtk_rdma_debugfs_init(struct mtk_rdma *rdma, const char *name)
{
	static struct dentry *debugfs_dir;
	struct dentry *file;

	if (!mtk_debugfs_root) {
		dev_err(rdma->dev, "No mtk debugfs root!\n");
		return;
	}

	rdma->debugfs = &debugfs_dir;
	if (debugfs_dir == NULL) {
		debugfs_dir = debugfs_create_dir("rdma", mtk_debugfs_root);
		if (IS_ERR(debugfs_dir)) {
			dev_err(rdma->dev, "failed to create debug dir (%p)\n",
				debugfs_dir);
			debugfs_dir = NULL;
			return;
		}
	}
	file = debugfs_create_file(name, 0664, debugfs_dir,
				   (void *)rdma, &mtk_rdma_debug_fops);
	if (IS_ERR(file)) {
		dev_err(rdma->dev, "failed to create debug file (%p)\n", file);
		debugfs_remove_recursive(debugfs_dir);
		debugfs_dir = NULL;
		return;
	}
	rdma->file_name = name;
}
#endif

/** @ingroup IP_group_rdma_internal_function
 * @par Description
 *     Get Necessary Hardware Information from Device Tree.\n
 *     There are HW number, delay generator number, and gce related\n
 *     information which will be parsed from device tree.
 * @param[in]
 *     pdev: platform device node.
 * @return
 *     If return value is 0, function success.
 *     Otherwise, mutex probe failed.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If there is any error during the probing flow,\n
 *     system error value will be returned.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_rdma_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_rdma *rdma;
	struct resource *regs;
	u32 i;
	int irq, err;

	rdma = devm_kzalloc(dev, sizeof(*rdma), GFP_KERNEL);
	if (!rdma)
		return -ENOMEM;

	rdma->dev = dev;

	err = of_property_read_u32(dev->of_node, "rdma-type", &rdma->rdma_type);
	if (err)
		rdma->rdma_type = RDMA_TYPE_MDP;

	of_property_read_u32(dev->of_node, "hw-number", &rdma->module_cnt);
#ifdef CONFIG_COMMON_CLK_MT3612
	for (i = 0; i < rdma->module_cnt; i++) {
		rdma->clk[i] = of_clk_get(dev->of_node, i);
		if (IS_ERR(rdma->clk[i])) {
			dev_err(dev, "Failed to get rdma%d clock\n", i);
			return PTR_ERR(rdma->clk);
		}
	}
#endif
	for (i = 0; i < rdma->module_cnt; i++) {
		irq = platform_get_irq(pdev, i);
		if (irq < 0) {
			dev_err(dev, "Failed to get irq %x\n", irq);
			return irq;
		}
		rdma->irq[i] = irq;
		err = devm_request_irq(dev, rdma->irq[i],
			     mtk_rdma_irq_handler, IRQF_TRIGGER_LOW,
			     dev_name(dev), rdma);
		if (err < 0) {
			dev_err(dev, "Failed to request irq %d: %d\n",
				rdma->irq[i], err);
			return err;
		}

		regs = platform_get_resource(pdev, IORESOURCE_MEM, i);
		rdma->regs[i] = devm_ioremap_resource(dev, regs);
		if (IS_ERR(rdma->regs[i])) {
			dev_err(dev, "Failed to map RDMA registers\n");
			return PTR_ERR(rdma->regs);
		}
		#ifdef CONFIG_MTK_DEBUGFS
		rdma->reg_base[i] = regs->start;
		#endif
		rdma->cmdq_offset[i] = (u32) regs->start & 0x0000F000;
		dev_info(dev, "Get RDMA GCE addr offset[%d] = 0x%03X\n",
			 i, rdma->cmdq_offset[i]);
	}
	rdma->frame_done = 0;
	rdma->reg_update = 0;
	rdma->cb_status = 0;
	rdma->cb_func = (mtk_mmsys_cb)NULL;
	spin_lock_init(&rdma->spinlock_irq);

	err = of_property_read_u32_array(dev->of_node, "gce-subsys",
					 rdma->cmdq_subsys,
					 rdma->module_cnt);
	if (err) {
		dev_err(dev,
			"%s: failed to read gce-subsys property: %d\n",
			dev->of_node->full_name, err);
		return err;
	}

	if (rdma->rdma_type != RDMA_TYPE_DISP) {
		for (i = 0; i < rdma->module_cnt; i++) {
			struct device_node *larb_node;
			struct platform_device *larb_pdev;

			larb_node = of_parse_phandle(
				dev->of_node, "mediatek,larb", i / 2);
			if (!larb_node) {
				dev_err(rdma->dev, "get RDMA DRAM LARB %d failed\n",
					i);
				return -EINVAL;
			}

			if (!of_device_is_available(larb_node))
				continue;

			larb_pdev = of_find_device_by_node(larb_node);
			if (!dev_get_drvdata(&larb_pdev->dev)) {
				dev_info(dev, "rdma is waiting for larb device %s\n",
					 larb_node->full_name);
				of_node_put(larb_node);
				return -EPROBE_DEFER;
			}
			rdma->larb_dev[i] = &larb_pdev->dev;
			of_node_put(larb_node);

			larb_node = of_parse_phandle(dev->of_node,
					"mediatek,s_larb", i / 2);
			if (!larb_node)
				continue;

			if (!of_device_is_available(larb_node))
				continue;

			larb_pdev = of_find_device_by_node(larb_node);
			if (!dev_get_drvdata(&larb_pdev->dev)) {
				dev_info(dev, "rdma is waiting for s_larb device %s\n",
					 larb_node->full_name);
				of_node_put(larb_node);
				return -EPROBE_DEFER;
			}
			rdma->s_larb_dev[i] = &larb_pdev->dev;
			of_node_put(larb_node);
		}
	}

#ifdef CONFIG_COMMON_CLK_MT3612
	pm_runtime_enable(dev);
#endif

	platform_set_drvdata(pdev, rdma);

#ifdef CONFIG_MTK_DEBUGFS
	mtk_rdma_debugfs_init(rdma, pdev->name);
	dev_info(dev, "RDMA debugfs init complete\n");
#endif

	dev_info(dev, "RDMA dev_type[%d] probe done\n", rdma->rdma_type);

	return 0;
}

/** @ingroup IP_group_rdma_internal_function
 * @par Description
 *     Do Nothing.
 * @param[in]
 *     pdev: platform device node.
 * @return
 *     return value is 0.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_rdma_remove(struct platform_device *pdev)
{
#ifdef CONFIG_MTK_DEBUGFS
	struct mtk_rdma *rdma;

	rdma = platform_get_drvdata(pdev);

	if (rdma->debugfs) {
		debugfs_remove_recursive(*rdma->debugfs);
		*rdma->debugfs = NULL;
	}
#endif

#ifdef CONFIG_COMMON_CLK_MT3612
	pm_runtime_disable(&pdev->dev);
#endif
	return 0;
}

/** @ingroup IP_group_rdma_internal_struct
 * @brief RDMA Open Framework Device ID.\n
 * This structure is used to attach specific names to\n
 * platform device for use with device tree.
 */
static const struct of_device_id rdma_driver_dt_match[] = {
	{.compatible = "mediatek,mt3612-rdma"},
	{.compatible = "mediatek,mt3612-mdp-rdma"},
	{.compatible = "mediatek,mt3612-pvric-rdma"},
	{.compatible = "mediatek,mt3612-disp-rdma"},
	{},
};

MODULE_DEVICE_TABLE(of, rdma_driver_dt_match);

/** @ingroup IP_group_rdma_internal_struct
 * @brief RDMA platform driver structure.\n
 * This structure is used to register itself.
 */
struct platform_driver mtk_rdma_driver = {
	.probe = mtk_rdma_probe,
	.remove = mtk_rdma_remove,
	.driver = {
		   .name = "mediatek-rdma",
		   .owner = THIS_MODULE,
		   .of_match_table = rdma_driver_dt_match,
		   },
};

module_platform_driver(mtk_rdma_driver);
MODULE_LICENSE("GPL");
