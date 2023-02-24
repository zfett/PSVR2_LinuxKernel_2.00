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
 * @file mtk_rbfc.c
 * This rbfc driver is used to control MTK rbfc hardware module.\n
 * It mainly controls the ring buffer between mm modules.\n
 */

/**
 * @defgroup IP_group_RBFC RBFC
 *     RBFC (Ring Buffer Controller) is a line based FIFO controller module.\n
 *     A RBFC is composed of RBFC_WEN and RBFC_REN.
 *     RBFC_WEN is used to maintain the FIFO fullness condition.\n
 *     RBFC_REN is used maintain the emptiness condition.\n
 *     Multiple write modules and read modules are allowed to connect to one\n
 *     RBFC. It supports frame mask function to bypass masked frame of write\n
 *     module. RBFC could detect both write incomplete and read incomplete by\n
 *     using valid row count (accumulated from write FIFO control signal) and\n
 *     current row count (accumulated from read FIFO control signal).\n
 *     @{
 *         @defgroup IP_group_rbfc_external EXTERNAL
 *             The external API document for rbfc.\n
 *             The rbfc driver follows native Linux framework
 *             @{
 *               @defgroup IP_group_rbfc_external_function 1.function
 *                   External function for rbfc.
 *               @defgroup IP_group_rbfc_external_struct 2.structure
 *                   External structure used for rbfc.
 *               @defgroup IP_group_rbfc_external_typedef 3.typedef
 *                   none. Native Linux Driver.
 *               @defgroup IP_group_rbfc_external_enum 4.enumeration
 *                   External enumeration used for rbfc.
 *               @defgroup IP_group_rbfc_external_def 5.define
 *                   External definition used for rbfc.
 *             @}
 *
 *         @defgroup IP_group_rbfc_internal INTERNAL
 *             The internal API document for rbfc.
 *             @{
 *               @defgroup IP_group_rbfc_internal_function 1.function
 *                   Internal function for rbfc and module init.
 *               @defgroup IP_group_rbfc_internal_struct 2.structure
 *                   Internal structure used for rbfc.
 *               @defgroup IP_group_rbfc_internal_typedef 3.typedef
 *                   none. Follow Linux coding style, no typedef used.
 *               @defgroup IP_group_rbfc_internal_enum 4.enumeration
 *                   Internal enumeration used for rbfc.
 *               @defgroup IP_group_rbfc_internal_def 5.define
 *                   Internal definition used for rbfc.
 *             @}
 *     @}
 */

#include <linux/clk.h>
#include <linux/iopoll.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <soc/mediatek/mtk_rbfc.h>
#include "mtk_rbfc_reg.h"

/** @ingroup IP_group_rbfc_internal_def
 * @brief RBFC register definition
 * @{
 */
#define FRAME_WIDTH(n)			(RBFC_FRAME_WIDTH + (0x100 * n))
#define FRAME_HEIGHT(n)			(RBFC_FRAME_HEIGHT + (0x100 * n))
#define ROW_STRIDE(n)			(RBFC_RBF_ROW_STRIDE + (0x100 * n))
#define ROW_NO(n)			(RBFC_RBF_ROW_NO + (0x100 * n))
#define RBFC_READ_EMPTY_THD(n)		(RBFC_EN_RD_TH + (0x100 * n))
#define RBFC_WRITE_FULL_THD(n)		(RBFC_WR_FULL_ROW_TH + (0x100 * n))
#define CON(n)				(RBFC_CON + (0x100 * n))
#define RBFC_BUF_BASE(n)		(RBFC_RING_BUF_BASE + (0x100 * n))
#define RBFC_READ_START_THD(n)		(RBFC_1ST_ROW_EN_RD_TH + (0x100 * n))
#define RBFC_ROW_CNT_INIT(n)		(RBFC_CUR_ROW_CNT_INI_VAL + (0x100 * n))
#define CON1(n)				(RBFC_CON1 + (0x100 * n))
#define CON2(n)				(RBFC_CON2 + (0x100 * n))
#define CFG_R_SOF_AHEAD			CFG_R_SOF_AHEAD_W_SOF_EN
#define CON3(n)				(RBFC_CON3 + (0x100 * n))
#define CON4(n)				(RBFC_CON4 + (0x100 * n))
#define SET_FREERUN_MODE		CFG_BYPASS_MODE_SET
#define CLR_FREERUN_MODE		CFG_BYPASS_MODE_CLR
#define GCE_CON(n)			(RBFC_GCE_CON + (0x100 * n))
#define W_OVER_TH_VAL(n)		(RBFC_W_OVER_TH_VAL + (0x100 * n))
#define GCE_CON1(n)			(RBFC_GCE_CON1 + (0x100 * n))
#define RBFC_READ_UNDER_THD(n)		(RBFC_R_UNDER_TH + (0x100 * n))
#define MUTEX_CON(n)			(RBFC_MUTEX_CON + (0x100 * n))
#define W_OP_CNT(n)			(RBFC_W_OP_CNT + (0x100 * n))
#define R_OP_CNT(n)			(RBFC_R_OP_CNT + (0x100 * n))

#define RBFC_STATUS			0xf0

#define RBFC_MAX_BUFFER_HEIGHT		4096
#define RBFC_MAX_AUTOMASK_VALIDBITS	16

#ifdef CONFIG_MTK_DEBUGFS
struct dentry *mtk_rbfc_debugfs_root;
#endif

#define CHECK_BIT(var, pos) ((var) & (1<<(pos)))

/**
 * @}
 */

/** @ingroup IP_group_rbfc_internal_def
 * @{
 */
/** maximum number of RBFC plane */
#define MAX_RBFC_PLANE			4
/** maximum number of RBFC hardware */
#define RBFC_MAX_HW_NUM			4
/**
 * @}
 */

/** @ingroup IP_group_rbfc_internal_enum
 * @brief RBFC type\n
 * value is from 0 to 2.
 */
enum MTK_RBFC_TYPE {
/** 0: VENC RBFC */
	MTK_VENC_RBFC,
/** 1: VDEC RBFC */
	MTK_VDEC_RBFC,
/** 2: COMMON RBFC */
	MTK_COMM_RBFC,
/** 3:  SRBC-RDMA RBFC */
	MTK_SRBC_RBFC,
};

/** @ingroup IP_group_rbfc_internal_struct
 * @brief Rbfc Driver Data Structure.
 */
struct mtk_rbfc {
	/** rbfc clock node */
	struct clk *clk[RBFC_MAX_HW_NUM];
	/** rbfc register base */
	void __iomem *regs[RBFC_MAX_HW_NUM];
	/** gce subsys */
	u32 subsys[RBFC_MAX_HW_NUM];
	/** register offset used in gce subsys */
	u32 subsys_offset[RBFC_MAX_HW_NUM];
	/** rbfc frame height */
	u32 frame_height[MAX_RBFC_PLANE];
	/** rbfc ring buffer height */
	u32 ring_buf_height[MAX_RBFC_PLANE];
	/** rbfc hardware number */
	u32 hw_nr;
	/** rbfc type */
	u8 rbfc_type;
	/** rbfc plane number */
	u8 plane_num;
	/** rbfc ufo mode */
	u8 ufo_mode;
};

/** @ingroup IP_group_rbfc_internal_function
 * @par Description
 *     Set frame width and height.
 * @param[in] rbfc: rbfc driver data structure pointer.
 * @param[out] handle: CMDQ handle.
 * @param[in] idx: rbfc plane index.
 * @param[in] w: rbfc frame width.
 * @param[in] h: rbfc frame height.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameter.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *     If rbfc is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_rbfc_img_size(struct mtk_rbfc *rbfc, struct cmdq_pkt **handle,
			     u32 idx, u32 w, u32 h)
{
	u32 i;

	if (!rbfc)
		return -EINVAL;

	if ((rbfc->hw_nr < 1) || (rbfc->hw_nr > 4))
		return -EINVAL;

	for (i = 0; i < rbfc->hw_nr; i++) {
		if (handle == NULL) {
			writel(w, rbfc->regs[i] + FRAME_WIDTH(idx));
			writel(h, rbfc->regs[i] + FRAME_HEIGHT(idx));
		} else {
			cmdq_pkt_write_value(handle[i], rbfc->subsys[i],
					     rbfc->subsys_offset[i] +
					     FRAME_WIDTH(idx), w,
					     0xffffffff);
			cmdq_pkt_write_value(handle[i], rbfc->subsys[i],
					     rbfc->subsys_offset[i] +
					     FRAME_HEIGHT(idx), h,
					     0xffffffff);
		}
	}

	return 0;
}

/** @ingroup IP_group_rbfc_internal_function
 * @par Description
 *     Set ring buffer stride.
 * @param[in] rbfc: rbfc driver data structure pointer.
 * @param[out] handle: CMDQ handle.
 * @param[in] hw_offset: rbfc hardware offset.
 * @param[in] idx: rbfc plane index.
 * @param[in] stride: rbfc ring buffer stride.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameter.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *     If rbfc is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_rbfc_ring_buf_stride(struct mtk_rbfc *rbfc,
				    struct cmdq_pkt **handle, u32 hw_offset,
				    u32 idx, u32 stride)
{
	u32 i;

	if (!rbfc)
		return -EINVAL;

	if ((rbfc->hw_nr < 1) || (rbfc->hw_nr > 4))
		return -EINVAL;

	for (i = hw_offset; i < rbfc->hw_nr; i++) {
		if (handle == NULL)
			writel(stride,
			       rbfc->regs[i] + ROW_STRIDE(idx));
		else
			cmdq_pkt_write_value(handle[i], rbfc->subsys[i],
					     rbfc->subsys_offset[i] +
					     ROW_STRIDE(idx), stride,
					     0xffffffff);
	}

	return 0;
}

/** @ingroup IP_group_rbfc_internal_function
 * @par Description
 *     Set ring buffer height.
 * @param[in] rbfc: rbfc driver data structure pointer.
 * @param[out] handle: CMDQ handle.
 * @param[in] hw_offset: rbfc hardware offset.
 * @param[in] idx: rbfc plane index.
 * @param[in] h: rbfc ring buffer height.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameter.
 * @par Boundary case and Limitation
 *	   maximum is 4096.
 * @par Error case and Error handling
 *     If rbfc is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_rbfc_ring_buf_height(struct mtk_rbfc *rbfc,
				    struct cmdq_pkt **handle, u32 hw_offset,
				    u32 idx, u32 h)
{
	u32 i;

	if (!rbfc)
		return -EINVAL;

	if ((rbfc->hw_nr < 1) || (rbfc->hw_nr > 4))
		return -EINVAL;

	for (i = hw_offset; i < rbfc->hw_nr; i++) {
		if (handle == NULL)
			writel(h, rbfc->regs[i] + ROW_NO(idx));
		else
			cmdq_pkt_write_value(handle[i], rbfc->subsys[i],
					     rbfc->subsys_offset[i] +
					     ROW_NO(idx), h,
					     0xffffffff);
	}

	return 0;
}

/** @ingroup IP_group_rbfc_internal_function
 * @par Description
 *     Set ring buffer base address.
 * @param[in] rbfc: rbfc driver data structure pointer.
 * @param[out] handle: CMDQ handle.
 * @param[in] hw_offset: rbfc hardware offset.
 * @param[in] idx: rbfc plane index.
 * @param addr: rbfc ring buffer address.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameter.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *     If rbfc is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_rbfc_ring_buf_addr(struct mtk_rbfc *rbfc,
				  struct cmdq_pkt **handle, u32 hw_offset,
				  u32 idx, u32 addr)
{
	u32 i;

	if (!rbfc)
		return -EINVAL;

	if ((rbfc->hw_nr < 1) || (rbfc->hw_nr > 4))
		return -EINVAL;

	for (i = hw_offset; i < rbfc->hw_nr; i++) {
		if (handle == NULL) {
			writel(addr, rbfc->regs[i] + RBFC_BUF_BASE(idx));
		} else {
			cmdq_pkt_write_value(handle[i], rbfc->subsys[i],
					     rbfc->subsys_offset[i] +
					     RBFC_BUF_BASE(idx), addr,
					     0xffffffff);
		}
	}

	return 0;
}

/** @ingroup IP_group_rbfc_external_function
 * @par Description
 *     Set write target.
 *     0 is DRAM, 1 is SYSRAM.
 * @param[in]
 *     dev: rbfc device node.
 * @param[out]
 *     handle: CMDQ handle.
 * @param[in]
 *     target: DRAM or SYSRAM, #MTK_RBFC_TARGET.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameter.\n
 *     -EPERM, wrong calling sequence.\n
 * @par Boundary case and Limitation
 *	   mtk_rbfc_set_plane_num() should be called in advance.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.\n
 *     If rbfc->plane_num <= 0, return -EPERM.\n
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rbfc_set_target(const struct device *dev, struct cmdq_pkt **handle,
			enum MTK_RBFC_TARGET target)
{
	struct mtk_rbfc *rbfc;
	u32 i, j, reg = 0;

	if (!dev)
		return -EINVAL;

	rbfc = dev_get_drvdata(dev);

	if (!rbfc)
		return -EINVAL;

	if (rbfc->plane_num <= 0) {
		dev_err(dev, "Please set plane number first\n");
		return -EPERM;
	}

	if ((rbfc->hw_nr < 1) || (rbfc->hw_nr > 4))
		return -EINVAL;

	for (i = 0; i < rbfc->hw_nr; i++) {
		for (j = 0; j < rbfc->plane_num; j++) {
			if (handle == NULL) {
				reg = readl(rbfc->regs[i] + CON(j));
				if (target == MTK_RBFC_TO_SYSRAM)
					reg |= CFG_SEL_SYSRAM;
				else
					reg &= ~CFG_SEL_SYSRAM;
				writel(reg, rbfc->regs[i] + CON(j));
			} else {
				if (target == MTK_RBFC_TO_SYSRAM)
					reg = CFG_SEL_SYSRAM;
				cmdq_pkt_write_value(handle[i], rbfc->subsys[i],
						     rbfc->subsys_offset[i] +
						     CON(j), reg,
						     CFG_SEL_SYSRAM);
			}
		}
	}

	return 0;
}
EXPORT_SYMBOL(mtk_rbfc_set_target);

/** @ingroup IP_group_rbfc_external_function
 * @par Description
 *     Set RBFC specific ring buffer size.\n
 *     You could set multiple ones by using index.
 * @param[in]
 *     dev: rbfc device node.
 * @param[out]
 *     handle: CMDQ handle.
 * @param[in]
 *     idx: plane index, range 0 to 3.
 * @param[in]
 *     addr: buffer address.
 * @param[in]
 *     pitch: buffer stride, range 1 to 32767.
 * @param[in]
 *     row_num: ring buffer height, range 1 to 4096.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameter.
 * @par Boundary case and Limitation
 *     row_num maximum value is 4096.\n
 *     It must be called before mtk_rbfc_set_read_thd\n
 *     mtk_rbfc_set_write_thd, and mtk_rbfc_set_w_ov_th.\n
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.\n
 *     If row_num > 4096, return -EINVAL.\n
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rbfc_set_ring_buf_multi(const struct device *dev,
				struct cmdq_pkt **handle,
				u32 idx, dma_addr_t addr, u32 pitch,
				u32 row_num)
{
	struct mtk_rbfc *rbfc;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	if ((pitch < 1) || (pitch > 0x7fff))
		return -EINVAL;

	if ((row_num < 1) || (row_num > RBFC_MAX_BUFFER_HEIGHT))
		return -EINVAL;

	rbfc = dev_get_drvdata(dev);

	if (!rbfc)
		return -EINVAL;

	if (idx >= rbfc->plane_num)
		return -EINVAL;

	rbfc->ring_buf_height[idx] = row_num;
	ret = mtk_rbfc_ring_buf_stride(rbfc, handle, 0, idx, pitch);
	if (ret != 0)
		return ret;
	ret = mtk_rbfc_ring_buf_height(rbfc, handle, 0, idx, row_num);
	if (ret != 0)
		return ret;
	ret = mtk_rbfc_ring_buf_addr(rbfc, handle, 0, idx, addr);

	return ret;
}
EXPORT_SYMBOL(mtk_rbfc_set_ring_buf_multi);

/** @ingroup IP_group_rbfc_external_function
 * @par Description
 *     Set read empty row threshold for multiple planes.\n
 *     When fill row number is larger or equal to this value,\n
 *     read side can start to read .
 * @param[in]
 *     dev: rbfc device node.
 * @param[out]
 *     handle: CMDQ handle.
 * @param[in]
 *     thd: read empty row threshold, range 1 to 4096.
 *     thd should be smaller than ring_buf_height.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameter.\n
 *     -EPERM, wrong calling sequence.\n
 * @par Boundary case and Limitation
 *	   It must be <= ring buffer height.\n
 *	   mtk_rbfc_set_plane_num() should be called in advance.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.\n
 *     If thd is bigger than ring buffer height, return -EINVAL.\n
 *     If rbfc->plane_num <= 0, return -EPERM.\n
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rbfc_set_read_thd(const struct device *dev,
			  struct cmdq_pkt **handle, const u32 *thd)
{
	struct mtk_rbfc *rbfc;
	u32 i, j;

	if (!dev)
		return -EINVAL;

	rbfc = dev_get_drvdata(dev);

	if (!rbfc)
		return -EINVAL;

	if (rbfc->plane_num <= 0) {
		dev_err(dev, "Please set plane number first\n");
		return -EPERM;
	}

	if ((rbfc->hw_nr < 1) || (rbfc->hw_nr > 4))
		return -EINVAL;

	for (i = 0; i < rbfc->hw_nr; i++) {
		for (j = 0; j < rbfc->plane_num; j++) {
			if (rbfc->ring_buf_height[j] != 0 &&
				thd[j] > rbfc->ring_buf_height[j])
				return -EINVAL;

			if (handle == NULL) {
				writel(thd[j],
				       rbfc->regs[i] + RBFC_READ_EMPTY_THD(j));
				writel(thd[j],
				       rbfc->regs[i] + RBFC_READ_START_THD(j));
				writel(thd[j] - 1,
				       rbfc->regs[i] + RBFC_READ_UNDER_THD(j));
			} else {
				cmdq_pkt_write_value(handle[i], rbfc->subsys[i],
						     rbfc->subsys_offset[i] +
						     RBFC_READ_EMPTY_THD(j),
						     thd[j], 0xffffffff);
				cmdq_pkt_write_value(handle[i], rbfc->subsys[i],
						     rbfc->subsys_offset[i] +
						     RBFC_READ_START_THD(j),
						     thd[j], 0xffffffff);
				cmdq_pkt_write_value(handle[i], rbfc->subsys[i],
						     rbfc->subsys_offset[i] +
						     RBFC_READ_UNDER_THD(j),
						     thd[j] - 1,
						     0xffffffff);
			}
		}
	}

	return 0;
}
EXPORT_SYMBOL(mtk_rbfc_set_read_thd);

/** @ingroup IP_group_rbfc_external_function
 * @par Description
 *     Set write full row threshold for multiple planes.\n
 *     high 13 bits : from bit16, plane[i] drop count for read agent is WPE.\n
 *     low 13 bits : plane[i] write full threshold.\n
 *     When the free row number of ring buffer is larger or equal to thd,\n
 *     write side can start to write.
 * @param[in]
 *     dev: rbfc device node.
 * @param[out]
 *     handle: CMDQ handle.
 * @param[in]
 *     thd: write full row threshold, range 1 to 4096.
 *     thd should be smaller than ring_buf_height.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameter.\n
 *     -EPERM, wrong calling sequence.\n
 * @par Boundary case and Limitation
 *	   mtk_rbfc_set_plane_num() should be called in advance.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.\n
 *     If thd low 13 bits is > ring buffer height, return -EINVAL.\n
 *     If rbfc->plane_num <= 0, return -EPERM.\n
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rbfc_set_write_thd(const struct device *dev,
			   struct cmdq_pkt **handle, const u32 *thd)
{
	struct mtk_rbfc *rbfc;
	u32 i, j;

	if (!dev)
		return -EINVAL;

	rbfc = dev_get_drvdata(dev);

	if (!rbfc)
		return -EINVAL;

	if (rbfc->plane_num <= 0) {
		dev_err(dev, "Please set plane number first\n");
		return -EPERM;
	}

	if ((rbfc->hw_nr < 1) || (rbfc->hw_nr > 4))
		return -EINVAL;

	for (i = 0; i < rbfc->hw_nr; i++) {
		for (j = 0; j < rbfc->plane_num; j++) {
			if (rbfc->ring_buf_height[j] != 0 &&
				(thd[j] & 0x1FFF) > rbfc->ring_buf_height[j])
				return -EINVAL;

			if (handle == NULL)
				writel(thd[j],
				       rbfc->regs[i] + RBFC_WRITE_FULL_THD(j));
			else
				cmdq_pkt_write_value(handle[i], rbfc->subsys[i],
						     rbfc->subsys_offset[i] +
						     RBFC_WRITE_FULL_THD(j),
						     thd[j], 0xffffffff);
		}
	}

	return 0;
}
EXPORT_SYMBOL(mtk_rbfc_set_write_thd);

/** @ingroup IP_group_rbfc_external_function
 * @par Description
 *     Set rfbc active agent number.
 *     active[1] : read agent active bits, bit0~3 presents agent 0~3.
 *     active[0]:  write agent active bits, bit0~3 presents agent 0~3.
 * @param[in]
 *     dev: rbfc device node.
 * @param[out]
 *     handle: CMDQ handle.
 * @param[in]
 *     active: active agent number, range 0 to 15.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameter.\n
 *     -EPERM, wrong calling sequence.\n
 * @par Boundary case and Limitation
 *	   mtk_rbfc_set_plane_num() should be called in advance.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.\n
 *     If rbfc->plane_num <= 0, return -EPERM.\n
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rbfc_set_active_num(const struct device *dev,
			    struct cmdq_pkt **handle, const u32 *active)
{
	struct mtk_rbfc *rbfc;
	u32 i, j;

	if (!dev)
		return -EINVAL;

	rbfc = dev_get_drvdata(dev);

	if (!rbfc)
		return -EINVAL;

	if ((active[0] > 0xf) || (active[1] > 0xf))
		return -EINVAL;

	if (rbfc->plane_num <= 0) {
		dev_err(dev, "Please set plane number first\n");
		return -EPERM;
	}

	if ((rbfc->hw_nr < 1) || (rbfc->hw_nr > 4))
		return -EINVAL;

	for (i = 0; i < rbfc->hw_nr; i++) {
		for (j = 0; j < rbfc->plane_num; j++) {
			if (handle == NULL)
				writel(((active[1] & 0xf) << 8) |
				       (active[0] & 0xf),
				       rbfc->regs[i] + CON3(j));
			else
				cmdq_pkt_write_value(handle[i], rbfc->subsys[i],
						     rbfc->subsys_offset[i] +
						     CON3(j),
						     ((active[1] & 0xf) << 8) |
						     (active[0] & 0xf),
						     0xffffffff);
		}

		for (j = rbfc->plane_num; j < MAX_RBFC_PLANE; j++) {
			if (handle == NULL)
				writel(0x0, rbfc->regs[i] + CON3(j));
			else
				cmdq_pkt_write_value(handle[i], rbfc->subsys[i],
						     rbfc->subsys_offset[i] +
						     CON3(j), 0x0,
						     0xffffffff);
		}
	}

	return 0;
}
EXPORT_SYMBOL(mtk_rbfc_set_active_num);

/** @ingroup IP_group_rbfc_external_function
 * @par Description
 *     Set rbfc initial row count value.
 * @param[in]
 *     dev: rbfc device node.
 * @param[out]
 *     handle: CMDQ handle.
 * @param[in]
 *     row_cnt: initial row count value, range 0 to 16383.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameter.\n
 *     -EACCESS, wrong input device type.\n
 *     -EPERM, wrong calling sequence.\n
 * @par Boundary case and Limitation
 *	   It only can be called in VENC RBFC.\n
 *	   The value should be set to non-zero value at read side.\n
 *	   mtk_rbfc_set_plane_num() should be called in advance.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.\n
 *     If dev is not VENC rbfc, return -EACCES.\n
 *     If rbfc->plane_num <= 0, return -EPERM.\n
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rbfc_set_row_cnt_init(const struct device *dev,
			      struct cmdq_pkt **handle, const u32 *row_cnt)
{
	struct mtk_rbfc *rbfc;
	u32 i, j;

	if (!dev)
		return -EINVAL;

	rbfc = dev_get_drvdata(dev);

	if (!rbfc)
		return -EINVAL;

	if (rbfc->rbfc_type != MTK_VENC_RBFC) {
		pr_warn
		    ("The RBFC user shouldn't call %s\n", __func__);
		return -EACCES;
	}
	if (rbfc->plane_num <= 0) {
		dev_err(dev, "Please set plane number first\n");
		return -EPERM;
	}
	for (j = 0; j < rbfc->plane_num; j++) {
		if (row_cnt[j] > 0x3fff)
			return -EINVAL;
	}

	if ((rbfc->hw_nr < 1) || (rbfc->hw_nr > 4))
		return -EINVAL;

	for (i = 0; i < rbfc->hw_nr; i++) {
		for (j = 0; j < rbfc->plane_num; j++)
			if (handle == NULL)
				writel(row_cnt[j],
				       rbfc->regs[i] + RBFC_ROW_CNT_INIT(j));
			else
				cmdq_pkt_write_value(handle[i], rbfc->subsys[i],
						     rbfc->subsys_offset[i] +
						     RBFC_ROW_CNT_INIT(j),
						     row_cnt[j], 0xffffffff);
	}

	return 0;
}
EXPORT_SYMBOL(mtk_rbfc_set_row_cnt_init);

/** @ingroup IP_group_rbfc_external_function
 * @par Description
 *     Set rbfc plane write over threshold.\n
 *     write over-threshold value will be applied to\n
 *     GCE event or Mutex SOF event.
 * @param[in]
 *     dev: rbfc device node.
 * @param[out]
 *     handle: CMDQ handle.
 * @param[in]
 *     w_ov_th: write over threshold, range 0 to 4096.
 *     w_ov_th should be smaller than ring_buf_height.
 * @param[in]
 *     unmask_only: enabled only for unmasked frame.
 *     0: both masked /unmasked frame , 1: masked frame only.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameter.\n
 *     -EPERM, wrong calling sequence.\n
 * @par Boundary case and Limitation
 *	   mtk_rbfc_set_plane_num() should be called in advance.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.\n
 *     If w_ov_th > ring buffer height, return -EINVAL.\n
 *     If rbfc->plane_num <= 0, return -EPERM.\n
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rbfc_set_w_ov_th(const struct device *dev,
			 struct cmdq_pkt **handle,
			 u16 w_ov_th, bool unmask_only)
{
	struct mtk_rbfc *rbfc;
	u32 i, j;
	u32 value = 0;

	if (!dev)
		return -EINVAL;

	rbfc = dev_get_drvdata(dev);

	if (!rbfc)
		return -EINVAL;

	if (rbfc->plane_num <= 0) {
		dev_err(dev, "Please set plane number first\n");
		return -EPERM;
	}

	value = (w_ov_th << 16) | w_ov_th;

	if ((rbfc->hw_nr < 1) || (rbfc->hw_nr > 4))
		return -EINVAL;

	for (i = 0; i < rbfc->hw_nr; i++) {
		for (j = 0; j < rbfc->plane_num; j++) {
			if (rbfc->ring_buf_height[j] != 0 &&
				w_ov_th > rbfc->ring_buf_height[j])
				return -EINVAL;
		}

			if (handle == NULL)
				writel(value,
				       rbfc->regs[i] + W_OVER_TH_VAL(j));
			else
				cmdq_pkt_write_value(handle[i], rbfc->subsys[i],
						     rbfc->subsys_offset[i] +
						     W_OVER_TH_VAL(j),
						     value, 0xffffffff);
	}
	return 0;
}
EXPORT_SYMBOL(mtk_rbfc_set_w_ov_th);

/** @ingroup IP_group_rbfc_internal_function
 * @par Description
 *     Enable rbfc clock.
 * @param[in]
 *     dev: rbfc device node.
 * @param[out]
 *     handle: CMDQ handle.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_rbfc_enable_clk(struct device *dev,
			       struct cmdq_pkt **handle)
{
	struct mtk_rbfc *rbfc;
	u32 i, reg;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	rbfc = dev_get_drvdata(dev);

	if (!rbfc)
		return -EINVAL;

	if ((rbfc->hw_nr < 1) || (rbfc->hw_nr > 4))
		return -EINVAL;

	for (i = 0; i < rbfc->hw_nr; i++) {
#if defined(CONFIG_COMMON_CLK_MT3612)
		if (rbfc->clk[i]) {
			ret = clk_prepare_enable(rbfc->clk[i]);
			if (ret) {
				clk_disable_unprepare(rbfc->clk[i]);
				dev_err(dev,
					"Failed to enable rbfc clock:%d\n",
					ret);
				return ret;
			}
		}
#endif
		if (handle == NULL) {
			reg = readl(rbfc->regs[i] + CON2(0));
			reg |= CFG_RBFC_CKEN;
			writel(reg, rbfc->regs[i] + CON2(0));
		} else
			cmdq_pkt_write_value(handle[i], rbfc->subsys[i],
					     rbfc->subsys_offset[i] +
					     CON2(0), CFG_RBFC_CKEN,
					     CFG_RBFC_CKEN);
	}

	udelay(1);

	for (i = 0; i < rbfc->hw_nr; i++) {
		writel(0x1, rbfc->regs[i] + RBFC_RESET);
		writel(0x0, rbfc->regs[i] + RBFC_RESET);
	}

	return ret;
}

/** @ingroup IP_group_rbfc_internal_function
 * @par Description
 *     Disable rbfc clock.
 * @param[in]
 *     rbfc: rbfc driver data structure pointer.
 * @param[out]
 *     handle: CMDQ handle.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameter.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_rbfc_disable_clk(struct mtk_rbfc *rbfc,
				struct cmdq_pkt **handle)
{
	u32 i, reg;

	if (!rbfc)
		return -EINVAL;

	if ((rbfc->hw_nr < 1) || (rbfc->hw_nr > 4))
		return -EINVAL;

	for (i = 0; i < rbfc->hw_nr; i++) {
		if (handle == NULL) {
			reg = readl(rbfc->regs[i] + CON2(0));
			reg &= ~CFG_RBFC_CKEN;
			writel(reg, rbfc->regs[i] + CON2(0));
		} else
			cmdq_pkt_write_value(handle[i], rbfc->subsys[i],
					     rbfc->subsys_offset[i] +
					     CON2(0), 0, CFG_RBFC_CKEN);

#if defined(CONFIG_COMMON_CLK_MT3612)
		if (rbfc->clk[i])
			clk_disable_unprepare(rbfc->clk[i]);
#endif
	}

	return 0;
}

/** @ingroup IP_group_rbfc_external_function
 * @par Description
 *     Set rfbc operation mode and start rbfc.\n
 *     There are 4 kinds of modes, ex: normal, FID, bypass, disable.\n
 *     It enable address translation while detecting ring buffer height is\n
 *     smaller than frame height.\n
 *     To make user can monitor the write over threshold event by GCE, it is\n
 *     enabled while calling to set mode (bit 16 in RBFC_GCE_CON), user also\n
 *     need to call mtk_rbfc_set_w_ov_th() to set the threshold value.
 * @param[in]
 *     dev: rbfc device node.
 * @param[out]
 *     handle: CMDQ handle.
 * @param[in]
 *     mode: rbfc operation mode, #MTK_RBFC_MODE
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameter.\n
 *     -EPERM, wrong calling sequence.\n
 * @par Boundary case and Limitation
 *     4 kinds of modes, ex: normal, FID, bypass, disable.\n
 *     Since this API would enable RBFC, it should be called after other RBFC\n
 *     configuration are set, ex: target, frame size, ring buffer size,\n
 *     threshold, and etc.\n
 *     If you just want to disable RBFC, before calling this API to set\n
 *     MTK_RBFC_DISABLE_MODE, at least you need call mtk_rbfc_set_plane_num()\n
 *     to set the rbfc plane number and mtk_rbfc_set_active_num() to set\n
 *     the active agent number.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.\n
 *     If dev type is not VDEC and mode is FID, return -EACCES.\n
 *     If rbfc->plane_num <= 0, return -EPERM.\n
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rbfc_start_mode(const struct device *dev, struct cmdq_pkt **handle,
			enum MTK_RBFC_MODE mode)
{
	struct mtk_rbfc *rbfc;
	u32 i, j, reg_con, reg_con2, reg_con4;

	if (!dev)
		return -EINVAL;

	rbfc = dev_get_drvdata(dev);

	if (!rbfc)
		return -EINVAL;

	if (rbfc->plane_num <= 0) {
		dev_err(dev, "Please set plane number first\n");
		return -EPERM;
	}

	if (mode == MTK_RBFC_FID_MODE && rbfc->rbfc_type != MTK_VDEC_RBFC) {
		pr_warn
		    ("The RBFC user shouldn't call %s\n", __func__);
		return -EACCES;
	}

	if ((rbfc->hw_nr < 1) || (rbfc->hw_nr > 4))
		return -EINVAL;

	for (i = 0; i < rbfc->hw_nr; i++) {
		for (j = 0; j < rbfc->plane_num; j++) {
			if (handle == NULL) {
				writel(0x01010301,
				       rbfc->regs[i] + GCE_CON(j));
				writel(0x01010101,
				       rbfc->regs[i] + GCE_CON1(j));
				writel(0x000f0007,
				       rbfc->regs[i] + MUTEX_CON(j));
			} else {
				cmdq_pkt_write_value(handle[i], rbfc->subsys[i],
						     rbfc->subsys_offset[i] +
						     GCE_CON(j),
						     0x01010301, 0xffffffff);
				cmdq_pkt_write_value(handle[i], rbfc->subsys[i],
						     rbfc->subsys_offset[i] +
						     GCE_CON1(j),
						     0x01010101, 0xffffffff);
				cmdq_pkt_write_value(handle[i], rbfc->subsys[i],
						     rbfc->subsys_offset[i] +
						     MUTEX_CON(j),
						     0x000f0007, 0xffffffff);
			}
			if (handle == NULL) {
				reg_con = readl(rbfc->regs[i] + CON(j));
				reg_con2 = readl(rbfc->regs[i] + CON2(j));
				reg_con4 = readl(rbfc->regs[i] + CON4(j));

				if (rbfc->ring_buf_height[j] <
				    rbfc->frame_height[j])
					reg_con |= CFG_ADR_TRANS_EN;
				else
					reg_con &= ~CFG_ADR_TRANS_EN;
				reg_con4 |= CLR_FREERUN_MODE;
				switch (mode) {
				case MTK_RBFC_NORMAL_MODE:
					reg_con2 =
					    (CFG_RBFC_CKEN | CFG_RBFC_EN);
					if (rbfc->rbfc_type == MTK_VENC_RBFC)
						reg_con2 |= CFG_R_SOF_AHEAD;
					break;
				case MTK_RBFC_FID_MODE:
					break;
				case MTK_RBFC_BYPASS_MODE:
					reg_con4 = SET_FREERUN_MODE;
					reg_con2 =
					    (CFG_RBFC_CKEN | CFG_RBFC_EN);
					break;
				case MTK_RBFC_DISABLE_MODE:
					reg_con2 =
					    (CFG_RWEN_CTL_DIS | CFG_RBFC_CKEN);
					reg_con2 |= CFG_RBFC_EN;
					break;
				}
				writel(reg_con4, rbfc->regs[i] + CON4(j));
				writel(reg_con2, rbfc->regs[i] + CON2(j));
				writel(reg_con, rbfc->regs[i] + CON(j));
			} else {
				if (rbfc->ring_buf_height[j] <
				    rbfc->frame_height[j])
					reg_con = CFG_ADR_TRANS_EN;
				else
					reg_con = 0;
				reg_con4 = CLR_FREERUN_MODE;
				reg_con2 = 0;
				switch (mode) {
				case MTK_RBFC_NORMAL_MODE:
					reg_con2 =
					    (CFG_RBFC_CKEN | CFG_RBFC_EN);
					if (rbfc->rbfc_type == MTK_VENC_RBFC)
						reg_con2 |= CFG_R_SOF_AHEAD;
					break;
				case MTK_RBFC_FID_MODE:
					break;
				case MTK_RBFC_BYPASS_MODE:
					reg_con4 = SET_FREERUN_MODE;
					reg_con2 =
					    (CFG_RBFC_CKEN | CFG_RBFC_EN);
					break;
				case MTK_RBFC_DISABLE_MODE:
					reg_con2 =
					    (CFG_RWEN_CTL_DIS | CFG_RBFC_CKEN);
					reg_con2 |= CFG_RBFC_EN;
					break;
				}
				cmdq_pkt_write_value(handle[i], rbfc->subsys[i],
						     rbfc->subsys_offset[i] +
						     CON4(j), reg_con4,
						     SET_FREERUN_MODE |
						     CLR_FREERUN_MODE);
				cmdq_pkt_write_value(handle[i], rbfc->subsys[i],
						     rbfc->subsys_offset[i] +
						     CON2(j), reg_con2,
						     CFG_R_SOF_AHEAD |
						     CFG_RWEN_CTL_DIS |
						     CFG_RBFC_CKEN |
						     CFG_RBFC_EN);
				cmdq_pkt_write_value(handle[i], rbfc->subsys[i],
						     rbfc->subsys_offset[i] +
						     CON(j), reg_con,
						     CFG_ADR_TRANS_EN);
			}
		}
	}
	return 0;
}
EXPORT_SYMBOL(mtk_rbfc_start_mode);

/** @ingroup IP_group_rbfc_external_function
 * @par Description
 *     Set RBFC specific ring buffer size.
 *     You could set multiple ones by using index.
 *     It starts the hardware offset from 2 to support right sight.
 *     It will check the rbfc hw number must be >= 4.
 * @param[in]
 *     dev: rbfc device node.
 * @param[out]
 *     handle: CMDQ handle.
 * @param[in]
 *     idx: plane index, range 0 to 3.
 * @param[in]
 *     addr: buffer address.
 * @param[in]
 *     pitch: buffer stride, range 1 to 32767.
 * @param[in]
 *     row_num: ring buffer height, range 1 to 4096.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameter.\n
 *     -EPERM, wrong calling sequence.\n
 * @par Boundary case and Limitation
 *	   mtk_rbfc_set_plane_num() should be called in advance.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.\n
 *     If this RBFC hardware number is smaller than MAX,\n
 *     it means that this rbfc cannot call this API, return -EINVAL.\n
 *     If rbfc->plane_num <= 0, return -EPERM.\n
 *     If idx >= plane number, return -EINVAL.
 *     If row_num > 4096, return -EINVAL.\n
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rbfc_set_2nd_ring_buf_multi(const struct device *dev,
				    struct cmdq_pkt **handle, u32 idx,
				    dma_addr_t addr, u32 pitch, u32 row_num)
{
	struct mtk_rbfc *rbfc;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	if ((pitch < 1) || (pitch > 0x7fff))
		return -EINVAL;

	if ((row_num < 1) || (row_num > RBFC_MAX_BUFFER_HEIGHT))
		return -EINVAL;

	rbfc = dev_get_drvdata(dev);

	if (!rbfc)
		return -EINVAL;

	if (rbfc->hw_nr < RBFC_MAX_HW_NUM) {
		dev_err(dev,
			"can not config ring buffer R sight with this configuration\n");
		return -EINVAL;
	}

	if (rbfc->plane_num <= 0) {
		dev_err(dev, "Please set plane number first\n");
		return -EPERM;
	}

	if (idx >= rbfc->plane_num)
		return -EINVAL;

	rbfc->ring_buf_height[idx] = row_num;
	ret = mtk_rbfc_ring_buf_stride(rbfc, handle, 2, idx, pitch);
	if (ret != 0)
		return ret;
	ret = mtk_rbfc_ring_buf_height(rbfc, handle, 2, idx, row_num);
	if (ret != 0)
		return ret;
	ret = mtk_rbfc_ring_buf_addr(rbfc, handle, 2, idx, addr);

	return ret;
}
EXPORT_SYMBOL(mtk_rbfc_set_2nd_ring_buf_multi);

/** @ingroup IP_group_rbfc_external_function
 * @par Description
 *     Set RBFC specific frame size.\n
 *     You could set multiple ones by using plane index.\n
 * @param[in]
 *     dev: rbfc device node.
 * @param[out]
 *     handle: CMDQ handle.
 * @param[in]
 *     idx: plane index, range 0 to 3.
 * @param[in]
 *     w: frame width, range 1 to 16383.
 * @param[in]
 *     h: frame height, range 1 to 16383.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameter.\n
 *     -EPERM, wrong calling sequence.\n
 * @par Boundary case and Limitation
 *	   mtk_rbfc_set_plane_num() should be called in advance.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.\n
 *     If idx >= plane number, return -EINVAL.\n
 *     If rbfc->plane_num <= 0, return -EPERM.\n
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rbfc_set_region_multi(const struct device *dev,
			      struct cmdq_pkt **handle, u32 idx, u32 w, u32 h)
{
	struct mtk_rbfc *rbfc;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	rbfc = dev_get_drvdata(dev);

	if (!rbfc)
		return -EINVAL;

	if ((w < 1) || (w > 0x3fff))
		return -EINVAL;

	if ((h < 1) || (h > 0x3fff))
		return -EINVAL;

	if (rbfc->plane_num <= 0) {
		dev_err(dev, "Please set plane number first\n");
		return -EPERM;
	}

	if (idx >= rbfc->plane_num)
		return -EINVAL;

	rbfc->frame_height[idx] = h;
	ret = mtk_rbfc_img_size(rbfc, handle, idx, w, h);
	return ret;
}
EXPORT_SYMBOL(mtk_rbfc_set_region_multi);

/** @ingroup IP_group_rbfc_external_function
 * @par Description
 *     Set rbfc plane number.
 * @param[in]
 *     dev: rbfc device node.
 * @param[in]
 *      data: plane number, range 1 to 4.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameter.
 * @par Boundary case and Limitation
 *     Maximum plane number is 4.\n
 *     After mtk_rbfc_power_on(), it's the first API should be called.\n
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.\n
 *     If data > 4, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rbfc_set_plane_num(const struct device *dev, u8 data)
{
	struct mtk_rbfc *rbfc;

	if (!dev)
		return -EINVAL;

	if (data > 4) {
		dev_err(dev,
			"plane number CANNOT bigger than 4\n");
		return -EINVAL;
	}

	rbfc = dev_get_drvdata(dev);

	if (!rbfc)
		return -EINVAL;

	rbfc->plane_num = data;

	return 0;
}
EXPORT_SYMBOL(mtk_rbfc_set_plane_num);

/** @ingroup IP_group_rbfc_external_function
 * @par Description
 *     Enable rbfc UFO mode or not (only for FID mode).
 * @param[in]
 *     dev: rbfc device node.
 * @param[in]
 *     data: 0 : not ufo_mode, others : ufo_mode.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameter.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rbfc_set_ufo_mode(const struct device *dev, u8 data)
{
	struct mtk_rbfc *rbfc;

	if (!dev)
		return -EINVAL;

	rbfc = dev_get_drvdata(dev);

	if (!rbfc)
		return -EINVAL;

	rbfc->ufo_mode = data;

	return 0;
}
EXPORT_SYMBOL(mtk_rbfc_set_ufo_mode);

/** @ingroup IP_group_rbfc_external_function
 * @par Description
 *     Get rfbc operation count at write side.
 *     The value bit19:16 is valid frame count, and bit14:0 is valid row count.
 * @param[in]
 *     dev: rbfc device node.
 * @param[out]
 *     data: property data buffer pointer.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rbfc_get_w_op_cnt(const struct device *dev, u32 *data)
{
	struct mtk_rbfc *rbfc;

	if (!dev)
		return -EINVAL;

	rbfc = dev_get_drvdata(dev);

	if (!rbfc)
		return -EINVAL;

	*data = readl(rbfc->regs[0] + W_OP_CNT(0));

	return 0;
}
EXPORT_SYMBOL(mtk_rbfc_get_w_op_cnt);

/** @ingroup IP_group_rbfc_external_function
 * @par Description
 *     Get rfbc operation count at read side.
 *     The value bit19:16 is current frame count, and bit14:0 is current row
 *     count.
 * @param[in]
 *     dev: rbfc device node.
 * @param[out]
 *     data: property data buffer pointer.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rbfc_get_r_op_cnt(const struct device *dev, u32 *data)
{
	struct mtk_rbfc *rbfc;

	if (!dev)
		return -EINVAL;

	rbfc = dev_get_drvdata(dev);

	if (!rbfc)
		return -EINVAL;

	*data = readl(rbfc->regs[0] + R_OP_CNT(0));

	return 0;
}
EXPORT_SYMBOL(mtk_rbfc_get_r_op_cnt);

/** @ingroup IP_group_rbfc_external_function
 * @par Description
 *     Finish RBFC.
 * @param[in]
 *     dev: rbfc device node.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameter.
 * @par Boundary case and Limitation
 *     Please call it to finish RBFC while stopping the scenario.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rbfc_finish(const struct device *dev)
{
	struct mtk_rbfc *rbfc;
	u32 i, j, status;

	if (!dev)
		return -EINVAL;

	rbfc = dev_get_drvdata(dev);

	if (!rbfc)
		return -EINVAL;

	if ((rbfc->hw_nr < 1) || (rbfc->hw_nr > 4))
		return -EINVAL;

	/* Reset RBFC GCE and Mutex control registers */
	for (i = 0; i < rbfc->hw_nr; i++) {
		for (j = 0; j < rbfc->plane_num; j++) {
			writel(0, rbfc->regs[i] + GCE_CON(j));
			writel(0, rbfc->regs[i] + GCE_CON1(j));
			writel(0, rbfc->regs[i] + MUTEX_CON(j));
		}
	}

	for (i = 0; i < rbfc->hw_nr; i++) {
		if (readl_poll_timeout_atomic(rbfc->regs[i] + RBFC_STATUS,
					      status, status & 0x8, 0x8, 50)) {
			dev_warn(dev,
				 "mtk_rbfc_finish wait engine idle timeout\n");
		}
		writel(0x1, rbfc->regs[i] + RBFC_RESET);
		writel(0x0, rbfc->regs[i] + RBFC_RESET);
	}
	return 0;
}
EXPORT_SYMBOL(mtk_rbfc_finish);

/** @ingroup IP_group_rbfc_external_function
 * @par Description
 *     Enable rbfc clock to power on.
 * @param[in]
 *     dev: rbfc device node.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rbfc_power_on(struct device *dev)
{
	int ret = 0;

	if (!dev)
		return -EINVAL;

#if defined(CONFIG_COMMON_CLK_MT3612)
	pm_runtime_get_sync(dev);
#endif
	ret = mtk_rbfc_enable_clk(dev, NULL);

	return ret;
}
EXPORT_SYMBOL(mtk_rbfc_power_on);

/** @ingroup IP_group_rbfc_external_function
 * @par Description
 *     Disable rbfc clock to power off.
 * @param[in]
 *     dev: rbfc device node.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_rbfc_power_off(struct device *dev)
{
	struct mtk_rbfc *rbfc;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	rbfc = dev_get_drvdata(dev);

	ret = mtk_rbfc_disable_clk(rbfc, NULL);
#if defined(CONFIG_COMMON_CLK_MT3612)
	pm_runtime_put_sync(dev);
#endif
	return ret;
}
EXPORT_SYMBOL(mtk_rbfc_power_off);

/** @ingroup IP_group_rbfc_internal_struct
 * @brief RBFC Open Framework Device ID.\n
 * This structure is used to attach specific names to\n
 * platform device for use with device tree.
 */
const enum MTK_RBFC_TYPE comm_rbfc = MTK_COMM_RBFC;
const enum MTK_RBFC_TYPE sbrc_rbfc = MTK_SRBC_RBFC;
static const struct of_device_id rbfc_driver_dt_match[] = {
	{.compatible = "mediatek,mt3612-rdma-rbfc",
		.data = (void *)&sbrc_rbfc},
	{.compatible = "mediatek,mt3612-rbfc",
		.data = (void *)&comm_rbfc},
	{},
};

MODULE_DEVICE_TABLE(of, rbfc_driver_dt_match);

#ifdef CONFIG_MTK_DEBUGFS
static int rbfc_debug_show(struct seq_file *s, void *unused)
{
	struct mtk_rbfc *rbfc = s->private;
	u32 reg;

	reg = readl(rbfc->regs[0] + RBFC_IRQ_STATUS0);

	seq_puts(s, "-------------------- RBFC DEUBG --------------------\n");
	seq_printf(s, "ren_w_incomplete      :%11x\n", CHECK_BIT(reg, 7) ? 1:0);
	seq_printf(s, "ren_r_incomplete      :%11x\n", CHECK_BIT(reg, 6) ? 1:0);
	seq_printf(s, "wen_r_incomplete      :%11x\n", CHECK_BIT(reg, 3) ? 1:0);
	seq_printf(s, "wen_w_incomplete      :%11x\n", CHECK_BIT(reg, 2) ? 1:0);
	seq_printf(s, "wr_full               :%11x\n", CHECK_BIT(reg, 1) ? 1:0);

	reg = readl(rbfc->regs[0] + RBFC_FRAME_WIDTH);
	seq_printf(s, "FRAME_WIDTH           :%11d\n", reg & 0x3FFF);

	reg = readl(rbfc->regs[0] + RBFC_FRAME_HEIGHT);
	seq_printf(s, "FRAME_HEIGHT          :%11d\n", reg & 0x3FFF);

	reg = readl(rbfc->regs[0] + RBFC_RBF_ROW_STRIDE);
	seq_printf(s, "ROW_STRIDE            :%11d\n", reg & 0x7FFF);

	reg = readl(rbfc->regs[0] + RBFC_RBF_ROW_NO);
	seq_printf(s, "ROW_NO                :%11d\n", reg & 0x3FFF);

	reg = readl(rbfc->regs[0] + RBFC_CON);
	if (reg & 0x1)
		seq_printf(s, "Active path           :%11s\n", "SRAM");
	else
		seq_printf(s, "Active path           :%11s\n", "DRAM");

	if ((reg >> 8) & 0x1)
		seq_printf(s, "address translate     :%11s\n", "ENABLE");
	else
		seq_printf(s, "address translate     :%11s\n", "DISABLE");

	reg = readl(rbfc->regs[0] + RBFC_RING_BUF_BASE);
	seq_printf(s, "RING_BUF_BASE         : 0x%08x\n", reg);

	reg = readl(rbfc->regs[0] + RBFC_W_OP_CNT);
	seq_printf(s, "W_OP_CNT(0xB0)        : 0x%08x\n", reg);

	reg = readl(rbfc->regs[0] + RBFC_R_OP_CNT);
	seq_printf(s, "R_OP_CNT(0xB4)        : 0x%08x\n", reg);

	reg = readl(rbfc->regs[0] + RBFC_BUF_CNT);
	seq_printf(s, "BUF_CNT(0xB8)         : 0x%08x\n", reg);

	reg = readl(rbfc->regs[0] + RBFC_W_OP_ALL_CNT);
	seq_printf(s, "W_OP_ALL_CNT(0xBC)    : 0x%08x\n", reg);

	reg = readl(rbfc->regs[0] + RBFC_MON0);
	seq_printf(s, "RBFC_MON0(0xF0)       : 0x%08x\n", reg);

	reg = readl(rbfc->regs[0] + RBFC_MON1);
	seq_printf(s, "RBFC_MON1(0xF4)       : 0x%08x\n", reg);
	seq_puts(s, "----------------------------------------------------\n");
	return 0;
}

static int debug_client_open(struct inode *inode, struct file *file)
{
	return single_open(file, rbfc_debug_show, inode->i_private);
}

static const struct file_operations rbfc_debug_fops = {
	.open = debug_client_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

/** @ingroup IP_group_rbfc_internal_function
 * @par Description
 *     Get Necessary Hardware Information from Device Tree.
 * @param[in] pdev: platform device node.
 * @return
 *     If return value is 0, function success.
 *     Otherwise, rbfc probe failed.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If there is any error in the probe flow,\n
 *     system error value will be returned.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_rbfc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_rbfc *rbfc;
	struct resource *regs;
	const struct of_device_id *of_id;
	int i;
#if defined(CONFIG_COMMON_CLK_MT3612)
	char clk[32];
#endif
#ifdef CONFIG_MTK_DEBUGFS
	struct dentry *debug_dentry;
#endif

	rbfc = devm_kzalloc(dev, sizeof(*rbfc), GFP_KERNEL);
	if (!rbfc)
		return -ENOMEM;

	of_property_read_u32(dev->of_node, "hw-number", &rbfc->hw_nr);

	if ((rbfc->hw_nr < 1) || (rbfc->hw_nr > 4))
		return -EINVAL;

	for (i = 0; i < rbfc->hw_nr; i++) {
#if defined(CONFIG_COMMON_CLK_MT3612)
		sprintf(clk, "clk%d", i);
		rbfc->clk[i] = devm_clk_get(dev, clk);
		if (IS_ERR(rbfc->clk[i])) {
			dev_err(dev, "Failed to get rbfc clock\n");
			return PTR_ERR(rbfc->clk[i]);
		}
#endif
		regs = platform_get_resource(pdev, IORESOURCE_MEM, i);
		rbfc->regs[i] = devm_ioremap_resource(dev, regs);
		if (IS_ERR(rbfc->regs[i])) {
			dev_err(dev, "Failed to map mtk_rbfc registers\n");
			return PTR_ERR(rbfc->regs);
		}
		rbfc->subsys_offset[i] = regs->start & 0xFFFF;
	}

	of_property_read_u32_array(dev->of_node, "gce-subsys", rbfc->subsys,
				   rbfc->hw_nr);

	of_id = of_match_device(rbfc_driver_dt_match, &pdev->dev);
	rbfc->rbfc_type = *(enum MTK_RBFC_TYPE *)of_id->data;
#if defined(CONFIG_COMMON_CLK_MT3612)
	pm_runtime_enable(dev);
#endif
	platform_set_drvdata(pdev, rbfc);

#ifdef CONFIG_MTK_DEBUGFS

	/* debug info create */
	if (!mtk_rbfc_debugfs_root)
		mtk_rbfc_debugfs_root = debugfs_create_dir("rbfc",
					mtk_debugfs_root);

	if (!mtk_rbfc_debugfs_root) {
		dev_dbg(dev, "failed to create bfc debugfs root directory.\n");
		goto debugfs_done;
	}

	debug_dentry = debugfs_create_file(pdev->name, 0664,
					   mtk_rbfc_debugfs_root,
					   rbfc, &rbfc_debug_fops);


debugfs_done:
	if (!debug_dentry)
		dev_dbg(dev, "Failed to create wdma debugfs %s\n", pdev->name);

#endif
	dev_dbg(dev, "rbfc probe done, hw type = %d\n", rbfc->rbfc_type);

	return 0;
}

/** @ingroup IP_group_rbfc_internal_function
 * @par Description
 *     none.
 * @param[in] pdev: platform device node.
 * @return
 *     If return value is 0, function success.\n.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_rbfc_remove(struct platform_device *pdev)
{
#if defined(CONFIG_COMMON_CLK_MT3612)
	struct device *dev = &pdev->dev;
#endif

#ifdef CONFIG_MTK_DEBUGFS
	debugfs_remove_recursive(mtk_rbfc_debugfs_root);
	mtk_rbfc_debugfs_root = NULL;
#endif

#if defined(CONFIG_COMMON_CLK_MT3612)
	pm_runtime_disable(dev);
#endif
	return 0;
}

/** @ingroup type_group_rbfc_struct
 * @brief RBFC platform driver structure.\n
 * This structure is used to register itself.
 */
struct platform_driver mtk_rbfc_driver = {
	.probe = mtk_rbfc_probe,
	.remove = mtk_rbfc_remove,
	.driver = {
		   .name = "mediatek-rbfc",
		   .owner = THIS_MODULE,
		   .of_match_table = rbfc_driver_dt_match,
		   },
};

module_platform_driver(mtk_rbfc_driver);
MODULE_LICENSE("GPL");
