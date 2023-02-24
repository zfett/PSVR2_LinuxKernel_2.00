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

 /**
 * @file mtk_fm.c
 * This FM driver is used to control MTK FM hardware module.\n
 * It supports both temporal and spatial matching for 60FPS and 120FPS.\n
 */

/**
 * @defgroup IP_group_fm FM
 *     FM (Feature Matching) engine is a hardware engine that responsible\n
 *     for finding the matches of 2 feature sets; the FM driver is aim to set\n
 *     up the features and provide the interface for user to modify\n
 *     corresponding properties. \n
 *     @{
 *         @defgroup IP_group_fm_external EXTERNAL
 *             The external API document for fm.\n
 *             The fm driver follows native Linux framework
 *             @{
 *               @defgroup IP_group_fm_external_function 1.function
 *                   External function for fm.
 *               @defgroup IP_group_fm_external_struct 2.structure
 *                   External structure used for fm.
 *               @defgroup IP_group_fm_external_typedef 3.typedef
 *                   none. Native Linux Driver.
 *               @defgroup IP_group_fm_external_enum 4.enumeration
 *                   External enumeration used for fm.
 *               @defgroup IP_group_fm_external_def 5.define
 *                   none.
 *             @}
 *
 *         @defgroup IP_group_fm_internal INTERNAL
 *             The internal API document for fm.
 *             @{
 *               @defgroup IP_group_fm_internal_function 1.function
 *                   Internal function for fm and module init.
 *               @defgroup IP_group_fm_internal_struct 2.structure
 *                   Internal structure used for fm.
 *               @defgroup IP_group_fm_internal_typedef 3.typedef
 *                   none. Follow Linux coding style, no typedef used.
 *               @defgroup IP_group_fm_internal_enum 4.enumeration
 *                   none.
 *               @defgroup IP_group_fm_internal_def 5.define
 *                   Internal definition used for fm.
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
#include <soc/mediatek/mtk_fm.h>
#include <soc/mediatek/smi.h>
#include "mtk_fm_reg.h"

#ifdef CONFIG_MTK_DEBUGFS
struct dentry *mtk_fm_debugfs_root;
#endif


/** @ingroup IP_group_fm_internal_def
 * @brief check bit value definition
 * @{
 */
#define CHECK_BIT(var, pos) ((var) & (1 << (pos)))


/** @ingroup IP_group_fm_internal_def
 * @brief FM driver use definition
 * @{
 */
#define FM_SR_TYPE_21x21			1
#define FM_SR_TYPE_141x11			2
/**
 * @}
 */

/** @ingroup IP_group_fm_internal_enum
 * @brief define for valid functions
 */
#define LARB_IOMMU (1 << 0)
#define CLK (1 << 1)

#define FM_REG_MAX_OFFSET	0x2B0
#define FM_REG_MIN_OFFSET	0x200

/** @ingroup IP_group_fm_internal_enum
 * @brief Distinguish between FPGA and A0 code
 */
enum dev_stage {
	FPGA = 0,
	A0 = 1,
};

/** @ingroup IP_group_fm_internal_struct
 * @brief FM Register Target Data Structure.
 */
struct reg_trg {
	/** target register base */
	void __iomem *regs;
	/** target register start address */
	u32 reg_base;
	/** cmdq packet */
	struct cmdq_pkt *pkt;
	/** gce subsys */
	int subsys;
};

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     FM reigster mask write common function.
 * @param[in] trg: register target.
 * @param[in] offset: register offset.
 * @param[in] value: write data value.
 * @param[in] mask: write mask value.
 * @return
 *     0, write to register success.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
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

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     FM reigster write common function.
 * @param[in] trg: register target.
 * @param[in] offset: register offset.
 * @param[in] value: write data value.
 * @return
 *     0, write to register success.\n
 *     error code from reg_write_mask().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If reg_write_mask() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int reg_write(struct reg_trg *trg, u32 offset, u32 value)
{
	int ret = 0;

	if (trg->pkt)
		ret = reg_write_mask(trg, offset, value, 0xffffffff);
	else
		writel(value, trg->regs + offset);

	return ret;
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Set ZNCC(Zero-mean Normalized Cross Correlation) value.
 * @param[in] trg: register target.
 * @param[in] thrshld: ZNCC value.
 * @return
 *     0, write to register success.\n
 *     error code from reg_write().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If reg_write() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_zncc_thrshld(struct reg_trg *trg, u32 thrshld)
{
	return reg_write(trg, FM_THR, thrshld);
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Set block size type.
 * @param[in] trg: register target.
 * @param[in] type: block size type;\n
 *     0: block size 32x32,\n
 *     1: block size 16x16,\n
 *     2: block size 8x8
 * @return
 *     0, write to register success.\n
 *     error code from reg_write_mask().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If reg_write_mask() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_blk_type(struct reg_trg *trg, u32 type)
{
	return reg_write_mask(trg, FM_TYPE, type << 3, FM_FM_BLK_TYPE);
}


/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Set search range type.
 * @param[in] trg: register target.
 * @param[in] type: search range type;\n
 *     0: temporal-initial,\n
 *     1: temporal-tracking,\n
 *     2: spatial.
 * @return
 *     0, write to register success.\n
 *     error code from reg_write_mask().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If reg_write_mask() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_sr_type_set(struct reg_trg *trg, u32 type)
{
	return reg_write_mask(trg, FM_TYPE, type << 1, FM_FM_SR_TYPE);
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Set input block number and image size.
 * @param[in] trg: register target.
 * @param[in] w: width of input image.
 * @param[in] h: height of input image.
 * @param[in] blk_nr: total block number from fe.
 * @return
 *     0, write to register success.\n
 *     error code from reg_write/_mask().
 * @par Boundary case and Limitation
 *     width and height should be limited to 160x160~640x640.
 *     block number should be limited to 25~400.
 * @par Error case and Error handling
 *     If reg_write/_mask() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_in_img_size(struct reg_trg *trg, u32 w, u32 h, u32 blk_nr)
{
	int ret = 0;

	ret = reg_write_mask(trg, FM_INPUT_INFO, blk_nr << 16,
			     FM_FM_BLK_NUM);
	if (ret)
		return ret;
	ret = reg_write(trg, FM_INPUT_SIZE, w | (h << 16));
	if (ret)
		return ret;

	return 0;
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Set fm read/write engine request interval.
 * @param[in] trg: register target.
 * @param[in] img_req_interval:
 *	rdma(img) request interval, range 1 to 65535(cycle)
 * @param[in] desc_req_interval:
 *	rdma(descriptor) request interval, range 1 to 65535(cycle)
 * @param[in] point_req_interval:
 *	rdma(point) request interval, range 1 to 65535(cycle)
 * @param[in] fmo_req_interval:
 *	wdma(fmo) request interval, range 1 to 65535(cycle)
 * @param[in] zncc_req_interval:
 *	wdma(zncc_subpix) request interval, range 1 to 65535(cycle)
 * @return
 *     0, write to register success.\n
 *     error code from reg_write/_mask().
 * @par Boundary case and Limitation
 *     each request interval should be limited to 1~65535(16bits)
 * @par Error case and Error handling
 *     If reg_write/_mask() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_req_interval(struct reg_trg *trg, u32 img_req_interval,
	u32 desc_req_interval, u32 point_req_interval,
	u32 fmo_req_interval, u32 zncc_req_interval)
{
	int ret = 0;

	ret = reg_write_mask(trg, FM_SMI_CONF0, img_req_interval,
		FM_REQ0_INTERVAL);
	if (ret)
		return ret;
	ret = reg_write_mask(trg, FM_SMI_CONF0, desc_req_interval << 16,
		FM_REQ1_INTERVAL);
	if (ret)
		return ret;
	ret = reg_write_mask(trg, FM_SMI_CONF1, point_req_interval,
		FM_REQ2_INTERVAL);
	if (ret)
		return ret;
	ret = reg_write_mask(trg, FM_SMI_CONF1, fmo_req_interval << 16,
		FM_REQ3_INTERVAL);
	if (ret)
		return ret;
	ret = reg_write_mask(trg, FM_SMI_CONF4, zncc_req_interval,
		FM_REQ4_INTERVAL);
	if (ret)
		return ret;

	return 0;
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Set base address of input image.
 * @param[in] trg: register target
 * @param[in] addr: base address of input image.
 * @return
 *     0, write to register success.\n
 *     error code from reg_write_mask().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If reg_write_mask() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_in_img_addr(struct reg_trg *trg, dma_addr_t addr)
{
	return reg_write_mask(trg, FM_RADDR0, addr, FM_FM_IN_ADDR0);
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Set stride of input image.
 * @param[in] trg: register target.
 * @param[in] stride: stride of input image.
 * @return
 *     0, write to register success.\n
 *     error code from reg_write_mask().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If reg_write_mask() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_in_img_stride(struct reg_trg *trg, u32 stride)
{
	return reg_write_mask(trg, FM_INPUT_INFO, stride,
			      FM_FM_IMG_STRIDE);
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Set base address of input descriptor.
 * @param[in] trg: register target.
 * @param[in] addr: base address of input descriptor.
 * @return
 *     0, write to register success.\n
 *     error code from reg_write_mask().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If reg_write_mask() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_in_descriptor_addr(struct reg_trg *trg, dma_addr_t addr)
{
	return reg_write_mask(trg, FM_RADDR1, addr, FM_FM_IN_ADDR1);
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Set base address of feature point.
 * @param[in] trg: register target.
 * @param[in] addr: base address of feature point.
 * @return
 *     0, write to register success.\n
 *     error code from reg_write_mask().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If reg_write_mask() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_in_point_addr(struct reg_trg *trg, dma_addr_t addr)
{
	return reg_write_mask(trg, FM_RADDR2, addr, FM_FM_IN_ADDR2);
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Set base address of output buffer.
 * @param[in] trg: register target.
 * @param[in] addr: base address of output buffer.
 * @return
 *     0, write to register success.\n
 *     error code from reg_write_mask().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If reg_write_mask() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_out_addr(struct reg_trg *trg, dma_addr_t addr)
{
	return reg_write_mask(trg, FM_WADDR0, addr, FM_FM_OUT_ADDR0);
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Set base address of fm zncc subpixels buffer.
 * @param[in] trg: register target.
 * @param[in] addr: base address of output buffer.
 * @return
 *     0, write to register success.\n
 *     error code from reg_write_mask().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If reg_write_mask() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_zncc_sub_addr(struct reg_trg *trg, dma_addr_t addr)
{
	return reg_write_mask(trg, FM_WADDR1, addr, FM_FM_OUT_ADDR1);
}


/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Switch to software mode and stop swapping CPU and FM internal circuit\n
 *     for sram access.\n
 * @param[in] trg: register target.
 * @param[in] sw_mode: software mode; true for sram ping-pong halt and enable\n
 *     sram access, else set false.
 * @return
 *     0, write to register success.\n
 *     error code from reg_write_mask().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If reg_write_mask() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_mask_tbl_sw_mode(struct reg_trg *trg, bool sw_mode)
{
	if (sw_mode)
		return reg_write_mask(trg, FM_MASK_SRAM_CFG,
				      FM_MASK_SRAM_PP_HALT |
				      FM_FORCE_MASK_SRAM_EN,
				      FM_MASK_SRAM_PP_HALT |
				      FM_FORCE_MASK_SRAM_EN);
	else
		return reg_write_mask(trg, FM_MASK_SRAM_CFG, 0,
				      FM_MASK_SRAM_PP_HALT |
				      FM_FORCE_MASK_SRAM_EN);
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Set software mask table index.
 * @param[in] trg: register target.
 * @param[in] idx: mask table index.
 * @return
 *     0, write to register success.\n
 *     error code from reg_write_mask().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If reg_write_mask() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_mask_tbl_sw_idx(struct reg_trg *trg, u32 idx)
{
	if (idx)
		return reg_write_mask(trg, FM_MASK_SRAM_CFG,
				      FM_FORCE_MASK_SRAM_APB,
				      FM_FORCE_MASK_SRAM_APB);
	else
		return reg_write_mask(trg, FM_MASK_SRAM_CFG, 0,
				      FM_FORCE_MASK_SRAM_APB);
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Set hardware mask table index.
 * @param[in] trg: register target.
 * @param[in] idx: mask table index.
 * @return
 *     0, write to register success.\n
 *     error code from reg_write_mask().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If reg_write_mask() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_mask_tbl_hw_idx(struct reg_trg *trg, u32 idx)
{
	if (idx)
		return reg_write_mask(trg, FM_MASK_SRAM_CFG,
				      FM_FORCE_MASK_SRAM_INT,
				      FM_FORCE_MASK_SRAM_INT);
	else
		return reg_write_mask(trg, FM_MASK_SRAM_CFG, 0,
				      FM_FORCE_MASK_SRAM_INT);
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Set the write index of mask table.
 * @param[in] trg: register target.
 * @param[in] idx: mask table index.
 * @return
 *     0, write to register success.\n
 *     error code from reg_write().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If reg_write() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_mask_tbl_write_idx(struct reg_trg *trg, u32 idx)
{
	return reg_write(trg, FM_MASK_SRAM_RW_IF_0, idx);
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Write the value of mask table.
 * @param[in] trg: register target.
 * @param[in] val: mask table value.
 * @return
 *     0, write to register success.\n
 *     error code from reg_write().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If reg_write() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_mask_tbl_write_val(struct reg_trg *trg, u32 val)
{
	return reg_write(trg, FM_MASK_SRAM_RW_IF_1, val);
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Set the write index of search center.
 * @param[in] trg: register target.
 * @param[in] idx: search center index.
 * @return
 *     0, write to register success.\n
 *     error code from reg_write().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If reg_write() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */

static int mtk_fm_search_center_write_idx(struct reg_trg *trg, u32 idx)
{
	return reg_write(trg, FM_SC_SRAM_RW_IF_0, idx);
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Write the value of search center.
 * @param[in] trg: register target.
 * @param[in] val: search center value.
 * @return
 *     0, write to register success.\n
 *     error code from reg_write().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If reg_write() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_search_center_write_val(struct reg_trg *trg, u32 val)
{
	return reg_write(trg, FM_SC_SRAM_RW_IF_1, val);
}


/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Set read mask table index.
 * @param[in] regs: fm register base address.
 * @param[in] idx: mask table index.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_fm_mask_tbl_read_idx(void __iomem *regs, u32 idx)
{
	writel(idx, regs + FM_MASK_SRAM_RW_IF_2);
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Check if sram read ready.
 * @param[in] regs: register base.
 * @return
 *     0, if sram is ready to read.\n
 *     -EBUSY, if sram is not ready to read.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     -EBUSY, if sram is not ready to read.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_mask_tbl_read_ready(void __iomem *regs)
{
	u32 status;

	if (readl_poll_timeout_atomic(regs + FM_MASK_SRAM_STATUS, status,
				      status & FM_MASK_SRAM_RRDY, 0x2, 10)) {
		pr_err("mtk_fm_mask_tbl_read_ready timeout\n");
		return -EBUSY;
	}

	return 0;
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Check if sram write ready.
 * @param[in] regs: register base.
 * @return
 *     0, if sram is ready to write.\n
 *     -EBUSY, if sram is not ready to write.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     -EBUSY, if sram is not ready to write.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_mask_tbl_write_ready(void __iomem *regs)
{
	u32 status;

	if (readl_poll_timeout_atomic(regs + FM_MASK_SRAM_STATUS, status,
				      status & FM_MASK_SRAM_WRDY, 0x2, 10)) {
		pr_err("mtk_fm_mask_tbl_write_ready timeout\n");
		return -EBUSY;
	}

	return 0;
}


/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Read mask table value.
 * @param[in] regs: fm register base address.
 * @return
 *     Retrun mask table value.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static u32 mtk_fm_mask_tbl_read_val(void __iomem *regs)
{
	return readl(regs + FM_MASK_SRAM_RW_IF_3);
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Set read search center index.
 * @param[in] regs: fm register base address.
 * @param[in] idx: search center index.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_fm_search_center_read_idx(void __iomem *regs, u32 idx)
{
	writel(idx, regs + FM_SC_SRAM_RW_IF_2);
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Check if sram read ready.
 * @param[in] regs: register base.
 * @return
 *     0, if sram is ready to read.\n
 *     -EBUSY, if sram is not ready to read.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     -EBUSY, if sram is not ready to read.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_search_center_read_ready(void __iomem *regs)
{
	u32 status;

	if (readl_poll_timeout_atomic(regs + FM_SC_SRAM_STATUS, status,
				      status & FM_SC_SRAM_RRDY, 0x2, 10)) {
		pr_err("mtk_fm_search_center_read_ready timeout\n");
		return -EBUSY;
	}

	return 0;
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Check if sram write ready.
 * @param[in] regs: register base.
 * @return
 *     0, if sram is ready to write.\n
 *     -EBUSY, if sram is not ready to write.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     -EBUSY, if sram is not ready to write.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_search_center_write_ready(void __iomem *regs)
{
	u32 status;

	if (readl_poll_timeout_atomic(regs + FM_SC_SRAM_STATUS, status,
				      status & FM_SC_SRAM_WRDY, 0x2, 10)) {
		pr_err("mtk_fm_search_center_write_ready timeout\n");
		return -EBUSY;
	}

	return 0;
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Read search center value.
 * @param[in] regs: fm register base address.
 * @return
 *     Retrun search center value.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static u32 mtk_fm_search_center_read_val(void __iomem *regs)
{
	return readl(regs + FM_SC_SRAM_RW_IF_3);
}


/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Enable DRAM access.
 * @param[in] trg: register target.
 * @return
 *     0, write to register success.\n
 *     error code from reg_write_mask().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If reg_write_mask() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_dram_enable(struct reg_trg *trg)
{
	return reg_write_mask(trg, FM_CTRL0, FM_DRAM_EN, FM_DRAM_EN);
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Disable DRAM access.
 * @param[in] trg: register target.
 * @return
 *     0, write to register success.\n
 *     error code from reg_write_mask().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If reg_write_mask() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_dram_disable(struct reg_trg *trg)
{
	return reg_write_mask(trg, FM_CTRL0, 0x0, FM_DRAM_EN);
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Reset FM. Assert a software reset.
 * @param[in] regs: fm register base address.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_fm_rst(void __iomem *regs)
{
	u32 reg = readl(regs + FM_CTRL0);

	reg |= FM_FM_RESET;
	writel(reg, regs + FM_CTRL0);

	reg &= ~FM_FM_RESET;
	writel(reg, regs + FM_CTRL0);
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Start FM.
 * @param[in] trg: register target.
 * @return
 *     0, write to register success.\n
 *     error code from reg_write().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If reg_write() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_enable(struct reg_trg *trg)
{
	return reg_write(trg, FM_CTRL1, 0x1);
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Stop FM.
 * @param[in] trg: .
 * @return
 *     0, write to register success.\n
 *     error code from reg_write().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If reg_write() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_disable(struct reg_trg *trg)
{
	return reg_write(trg, FM_CTRL1, 0x0);
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Get FM engine's status.
 * @param[in] regs: fm register base address.
 * @return
 *     True for idle, else for active.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static u32 mtk_fm_is_done(void __iomem *regs)
{
	u32 idle_mask = FM_WDMA_IDLE | FM_RDMA0_IDLE |
			FM_RDMA1_IDLE | FM_RDMA2_IDLE |
			FM_WDMA_SUBPIX_IDLE;

	return ((readl(regs + FM_STATUS) & FM_FM_ACTIVE) == 0) &&
		((readl(regs + FM_STATUS) & idle_mask) == idle_mask);
}

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Get dma idle status.
 * @param[in] trg: register target.
 * @return
 *     0 for idle, -EBUSY for idle timeout.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     -EBUSY for polling idle status timeout.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_wait_dma_idle(struct reg_trg *trg)
{
	u32 idle_mask = FM_WDMA_IDLE | FM_RDMA0_IDLE |
			FM_RDMA1_IDLE | FM_RDMA2_IDLE;
	u32 status;
	int ret = 0;

	if (trg->pkt) {
		ret = cmdq_pkt_poll(trg->pkt, trg->subsys,
				    (trg->reg_base & 0xffff) | FM_STATUS,
				    idle_mask, idle_mask);
	} else {
		if (readl_poll_timeout_atomic(trg->regs + FM_STATUS, status,
					      (status & idle_mask) == idle_mask,
					      0x2, 10)) {
			pr_err("mtk_fm_wait_dma_idle timeout, status=0x%x\n",
			       status);
			return -EBUSY;
		}
	}
	return ret;
}

/** @ingroup IP_group_fm_internal_struct
 * @brief FM Driver Data Structure.
 */
struct mtk_fm {
	/** fm device node */
	struct device *dev;
	/** fm mdp clock node */
	struct clk *clk_mdp;
	/** fm register base */
	void __iomem *regs;
	/** fm register start address */
	u32 reg_base;
	/** gce subsys */
	u32 subsys;
	/** larb device */
	struct device *larbdev;
	/** spinlock for irq control */
	spinlock_t spinlock_irq;
	/** irq number */
	int irq;
	/** callback function */
	mtk_mmsys_cb cb_func;
	/** callback data */
	void *cb_priv_data;
	/** development stage */
	u32 valid_func;
	u32 blk_size;
};

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *    fm driver irq handler.
 * @param[in] irq: irq information.
 * @param[in] dev_id: fm driver data.
 * @return
 *     irqreturn_t.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static irqreturn_t mtk_fm_irq_handler(int irq, void *dev_id)
{
	struct mtk_fm *fm = dev_id;
	u32 status;
	struct mtk_mmsys_cb_data cb_data;
	mtk_mmsys_cb func;
	unsigned long flags;

	status = readl(fm->regs + FM_INTSTA);
	writel(0x0, fm->regs + FM_INTSTA);

	spin_lock_irqsave(&fm->spinlock_irq, flags);
	func = fm->cb_func;
	cb_data.status = status;
	cb_data.priv_data = fm->cb_priv_data;
	spin_unlock_irqrestore(&fm->spinlock_irq, flags);

	cb_data.part_id = 0;

	if (func && cb_data.status)
		func(&cb_data);

	return IRQ_HANDLED;
}

/** @ingroup IP_group_fm_external_function
 * @par Description
 *     Set base address of feature point, feature descriptor, and input mage\n
 *     respectively.
 * @param[in] dev: FM device node.
 * @param[out] handle: CMDQ handle, set NULL for CPU write directly.
 * @param[in] idx: input type. #mtk_fm_in_buf
 * @param[in] addr: input buffer address.
 * @param[in] pitch: stride of input image. Set if the input is an image.
 *         range 160 to 2560 and 16-aligned.
 * @return
 *     0, config fm input buffer address success.
 *     -EINVAL, invalid input buffer address, stride, idx, or NULL dev.
 *      error code from reg_write/_mask().
 * @par Boundary case and Limitation
 *     idx should be in #mtk_fm_in_buf.
 *     pitch should be limit to 160 to 2560 and 16-aligned.
 * @par Error case and Error handling
 *     1. If fm dev is NULL, return -EINVAL.\n
 *     2. If set following parameters with invalid values, return error code\n
 *         from reg_write/_mask().\n
 *         addr, pitch
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fm_set_multi_in_buf(const struct device *dev, struct cmdq_pkt *handle,
			    enum mtk_fm_in_buf idx, dma_addr_t addr, u32 pitch)
{
	struct mtk_fm *fm;
	struct reg_trg trg;
	int ret = 0;


	if (!dev)
		return -EINVAL;

	fm = dev_get_drvdata(dev);
	if (!fm)
		return -EINVAL;

	if (addr & 0x7f) {
		dev_err(dev, "mtk_fm_set_multi_in_buf: invalid addr %pad\n",
			&addr);
		return -EINVAL;
	}

	trg.regs = fm->regs;
	trg.reg_base = fm->reg_base;
	trg.pkt = handle;
	trg.subsys = fm->subsys;

	switch (idx) {
	case MTK_FM_IN_BUF_IMG:
		if (pitch & 0xf || pitch > 2560 || pitch < 160) {
			dev_err(dev, "mtk_fm_set_multi_in_buf: invalid pitch %d\n",
				pitch);
			return -EINVAL;
		}
		ret = mtk_fm_in_img_addr(&trg, addr);
		if (ret) {
			dev_err(dev, "mtk_fm_set_multi_in_buf: error IMG addr\n");
			break;
		}
		ret = mtk_fm_in_img_stride(&trg, pitch);
		if (ret)
			dev_err(dev, "mtk_fm_set_multi_in_buf: error IMG pitch\n");
		break;
	case MTK_FM_IN_BUF_DESCRIPTOR:
		ret = mtk_fm_in_descriptor_addr(&trg, addr);
		if (ret)
			dev_err(dev, "mtk_fm_set_multi_in_buf: error DES addr\n");
		break;
	case MTK_FM_IN_BUF_POINT:
		ret = mtk_fm_in_point_addr(&trg, addr);
		if (ret)
			dev_err(dev, "mtk_fm_set_multi_in_buf: error PT addr\n");
		break;
	default:
		dev_err(dev, "mtk_fm_set_multi_in_buf: error invalid idx %d\n",
			idx);
		return -EINVAL;
	}

	return ret;
}
EXPORT_SYMBOL(mtk_fm_set_multi_in_buf);

/** @ingroup IP_group_fm_external_function
 * @par Description
 *     Set the base address of FM output result. The output buffer\n
 *         could be fmo buffer or zncc subpixel buffer.
 * @param[in] dev: FM device node.
 * @param[out] handle: CMDQ handle, set NULL for CPU write directly.
 * @param[in] idx: the enum index of multiple output buffer. #mtk_fm_out_buf
 * @param[in] addr: out buffer's address.
 * @return
 *     0, config fm output buffer address success.
 *     -EINVAL, invalid output buffer address or NULL dev.
 *      error code from reg_write/_mask().
 * @par Boundary case and Limitation
 *     idx should be in #mtk_fm_out_buf.
 *     Address of output buffer should align to 128 bytes.
 * @par Error case and Error handling
 *     1. If fm dev is NULL, return -EINVAL.\n
 *     2. If buffer address is unaligned,  return -EINVAL.\n
 *     3. If set following parameters with invalid values, return error code\n
 *         from reg_write/_mask().\n
 *         addr
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fm_set_multi_out_buf(const struct device *dev, struct cmdq_pkt *handle,
			     enum mtk_fm_out_buf idx, dma_addr_t addr)
{
	struct mtk_fm *fm;
	struct reg_trg trg;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	fm = dev_get_drvdata(dev);
	if (!fm)
		return -EINVAL;

	if (addr & 0x7f) {
		dev_err(dev, "mtk_fm_set_out_buf: invalid addr %pad\n",
			&addr);
		return -EINVAL;
	}
	trg.regs = fm->regs;
	trg.reg_base = fm->reg_base;
	trg.pkt = handle;
	trg.subsys = fm->subsys;

	switch (idx) {
	case MTK_FM_OUT_BUF_FMO:
		ret = mtk_fm_out_addr(&trg, addr);
		if (ret)
			dev_err(dev,
				"mtk_fm_set_multi_out_buf: error fmo addr\n");
		break;
	case MTK_FM_OUT_BUF_ZNCC_SUBPIXEL:
		ret = mtk_fm_zncc_sub_addr(&trg, addr);
		if (ret)
			dev_err(dev,
				"mtk_fm_set_multi_out_buf: error ZNCC_SUBPIXEL addr\n");
		break;
	default:
		dev_err(dev,
			"mtk_fm_set_multi_out_buf: error invalid idx %d\n",
			idx);
		return -EINVAL;
	}
	return ret;
}
EXPORT_SYMBOL(mtk_fm_set_multi_out_buf);

/** @ingroup IP_group_fm_external_function
 * @par Description
 *     Check if FM and all DMA idle.
 * @param[in] dev: FM device node.
 * @param[out] ret: return value of register FM_STATUS check; true: FM is idle,
 *     false: FM is active.
 * @return
 *     0, get fm done status.\n
 *     -EINVAL, invalid dev or return value.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Return -EINVAL if invalid device or ret.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fm_get_fm_done(const struct device *dev, u32 *ret)
{
	struct mtk_fm *fm;

	if (!dev)
		return -EINVAL;

	fm = dev_get_drvdata(dev);
	if (!fm)
		return -EINVAL;

	if (!ret)
		return -EINVAL;

	*ret = mtk_fm_is_done(fm->regs);

	return 0;
}
EXPORT_SYMBOL(mtk_fm_get_fm_done);

/** @ingroup IP_group_fm_external_function
 * @par Description
 *     Check if dma access is done. This will poll and wait all DMA idle
 * @param[in] dev: FM device node.
 * @param[out]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @return
 *     0, dma access is in idle state\n
 *     -EINVAL, invalid dev or mask_tbl.\n
 *     error code from mtk_fm_wait_dma_idle.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Return error code if invalid device or dma idle timeout.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fm_get_dma_idle(const struct device *dev, struct cmdq_pkt *handle)
{
	struct mtk_fm *fm;
	struct reg_trg trg;
	int dma_idle = -1;

	if (!dev)
		return -EINVAL;

	fm = dev_get_drvdata(dev);
	if (!fm)
		return -EINVAL;

	trg.regs = fm->regs;
	trg.reg_base = fm->reg_base;
	trg.pkt = handle;
	trg.subsys = fm->subsys;
	dma_idle = mtk_fm_wait_dma_idle(&trg);

	return dma_idle;
}
EXPORT_SYMBOL(mtk_fm_get_dma_idle);

/** @ingroup IP_group_fm_external_function
 * @par Description
 *     Read current active mask table.
 * @param[in] dev: FM device node.
 * @param[out] mask_tbl: active mask table.
 * @return
 *     0, get fm mask table done.\n
 *     -EINVAL, invalid dev or mask_tbl.
 * @par Boundary case and Limitation
 *     The max search range is sized 141X11, and each search block is sized
 *     4X8; therefore, the table size should be limited to
 *     ceil(141/4) X ceil(11/8) = 72
 * @par Error case and Error handling
 *     Return -EINVAL if invalid device or mask_tbl.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fm_get_mask_tbl(const struct device *dev,
			struct mtk_fm_mask_tbl *mask_tbl)
{
	struct mtk_fm *fm;
	int i;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	fm = dev_get_drvdata(dev);
	if (!fm)
		return -EINVAL;

	if (!mask_tbl)
		return -EINVAL;

	if (mask_tbl->size > 72) {
		dev_err(dev, "mtk_fm_get_mask_tbl: mask_tbl size %d overflow\n",
			mask_tbl->size);
		return -EINVAL;
	}

	mtk_fm_mask_tbl_read_idx(fm->regs, 0);
	ret = mtk_fm_mask_tbl_read_ready(fm->regs);
	if (ret) {
		dev_err(dev, "mtk_fm_get_mask_tbl: poll read_ready fail\n");
		return ret;
	}

	for (i = 0; i < mask_tbl->size; i++)
		mask_tbl->mask_tbl[i] = mtk_fm_mask_tbl_read_val(fm->regs);

	return 0;
}
EXPORT_SYMBOL(mtk_fm_get_mask_tbl);

/** @ingroup IP_group_fm_external_function
 * @par Description
 *     Read current search center.
 * @param[in] dev: FM device node.
 * @param[out] sc: active search center.
 * @return
 *     0, get fm search center done.\n
 *     -EINVAL, invalid dev or sc.
 * @par Boundary case and Limitation
 *     each block can set one search center, so total search center number
 *         must less than max block number(400).
 * @par Error case and Error handling
 *     Return -EINVAL if invalid device or sc.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fm_get_search_center(const struct device *dev,
			     struct mtk_fm_search_center *sc)
{
	struct mtk_fm *fm;
	int i;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	fm = dev_get_drvdata(dev);
	if (!fm)
		return -EINVAL;

	if (!sc)
		return -EINVAL;

	if (sc->size > 400) {
		dev_err(dev, "mtk_fm_get_search center: sc size %d overflow\n",
			sc->size);
		return -EINVAL;
	}

	mtk_fm_search_center_read_idx(fm->regs, 0);
	ret = mtk_fm_search_center_read_ready(fm->regs);
	if (ret) {
		dev_err(dev,
			"mtk_fm_get_search_center: poll read_ready fail\n");
		return ret;
	}

	for (i = 0; i < sc->size; i++) {
		sc->sc[i] =
			mtk_fm_search_center_read_val(fm->regs);
	}

	return 0;
}
EXPORT_SYMBOL(mtk_fm_get_search_center);

/** @ingroup IP_group_fm_external_function
 * @par Description
 *     Set block size of input fe.
 * @param[in] dev: FM device node.
 * @param[out] handle: CMDQ handle, set NULL for CPU write directly.
 * @param[in] blk_type: block size type, #mtk_fm_blk_type.
 * @return
 *     0, set blk_type success.
 *     -EINVAL, invalid sr_type or NULL dev.
 *      error code from reg_write/_mask().
 * @par Boundary case and Limitation
 *     blk_type should be in #mtk_fm_blk_type.
 * @par Error case and Error handling
 *     1. If fm dev is NULL, return -EINVAL.\n
 *     2. If use invalid sr_type, return -EINVAL.\n
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fm_set_blk_type(const struct device *dev, struct cmdq_pkt *handle,
			enum mtk_fm_blk_size_type blk_type)
{
	struct mtk_fm *fm;
	struct reg_trg trg;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	fm = dev_get_drvdata(dev);
	if (!fm)
		return -EINVAL;

	trg.regs = fm->regs;
	trg.reg_base = fm->reg_base;
	trg.pkt = handle;
	trg.subsys = fm->subsys;

	switch (blk_type) {
	case MTK_FM_BLK_SIZE_TYPE_32x32:
		ret = mtk_fm_blk_type(&trg, MTK_FM_BLK_SIZE_TYPE_32x32);
		fm->blk_size = 32;
		break;
	case MTK_FM_BLK_SIZE_TYPE_16x16:
		ret = mtk_fm_blk_type(&trg, MTK_FM_BLK_SIZE_TYPE_16x16);
		fm->blk_size = 16;
		break;
	case MTK_FM_BLK_SIZE_TYPE_8x8:
		ret = mtk_fm_blk_type(&trg, MTK_FM_BLK_SIZE_TYPE_8x8);
		fm->blk_size = 8;
		break;
	default:
		dev_err(dev, "mtk_fm_set_blk_type: error invalid blk_type %d\n",
			blk_type);
		return -EINVAL;
	}

	if (ret)
		dev_err(dev, "mtk_fm_set_blk_type: error set blk_type %d\n",
			blk_type);

	return ret;
}
EXPORT_SYMBOL(mtk_fm_set_blk_type);


/** @ingroup IP_group_fm_external_function
 * @par Description
 *     Set search range type.
 * @param[in] dev: FM device node.
 * @param[out] handle: CMDQ handle, set NULL for CPU write directly.
 * @param[in] sr_type: search range type. #mtk_fm_sr_type
 * @return
 *     0, set sr_type success.
 *     -EINVAL, invalid sr_type or NULL dev.
 *      error code from reg_write/_mask().
 * @par Boundary case and Limitation
 *     sr_type should be in #mtk_fm_sr_type.
 * @par Error case and Error handling
 *     1. If fm dev is NULL, return -EINVAL.\n
 *     2. If use invalid sr_type, return -EINVAL.\n
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fm_set_sr_type(const struct device *dev, struct cmdq_pkt *handle,
		       enum mtk_fm_sr_type sr_type)
{
	struct mtk_fm *fm;
	struct reg_trg trg;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	fm = dev_get_drvdata(dev);
	if (!fm)
		return -EINVAL;

	trg.regs = fm->regs;
	trg.reg_base = fm->reg_base;
	trg.pkt = handle;
	trg.subsys = fm->subsys;

	switch (sr_type) {
	case MTK_FM_SR_TYPE_TEMPORAL_PREDICTION:
		ret = mtk_fm_sr_type_set(&trg, FM_SR_TYPE_21x21);
		break;
	case MTK_FM_SR_TYPE_SPATIAL:
		ret = mtk_fm_sr_type_set(&trg, FM_SR_TYPE_141x11);
		break;
	default:
		dev_err(dev, "mtk_fm_set_sr_type: error invalid sr_type %d\n",
			sr_type);
		return -EINVAL;
	}

	if (ret)
		dev_err(dev, "mtk_fm_set_sr_type: error set sr_type %d\n",
			sr_type);

	return ret;
}
EXPORT_SYMBOL(mtk_fm_set_sr_type);

/** @ingroup IP_group_fm_external_function
 * @par Description
 *     Set software mask table.
 * @param[in] dev: FM device node.
 * @param[out] handle: CMDQ handle, set NULL for CPU write directly.
 * @param[in] mask_tbl: user-defined mask table.
 * @return
 *     0, set sw mask table success.
 *     -EINVAL, invalid mask_tbl or NULL dev.
 *     error code from reg_write/_mask().
 * @par Boundary case and Limitation
 *     The max search range is sized 141X11, and each search block is sized
 *     4X8; therefore, the table size should be limited to
 *     ceil(141/4) X ceil(11/8) = 72
 * @par Error case and Error handling
 *     1. If fm dev or mask_tbl is NULL, return -EINVAL.\n
 *     2. If set following parameters with invalid values, return error code\n
 *         from reg_write/_mask().\n
 *         mask_tbl, enable sw mask
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fm_set_mask_tbl(const struct device *dev, struct cmdq_pkt *handle,
			struct mtk_fm_mask_tbl *mask_tbl)
{
	struct mtk_fm *fm;
	struct reg_trg trg;
	int i;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	fm = dev_get_drvdata(dev);
	if (!fm)
		return -EINVAL;

	if (!mask_tbl)
		return -EINVAL;

	if (mask_tbl->size > 72) {
		dev_err(dev, "mtk_fm_set_mask_tbl: mask_tbl size %d overflow\n",
			mask_tbl->size);
		return -EINVAL;
	}

	trg.regs = fm->regs;
	trg.reg_base = fm->reg_base;
	trg.pkt = handle;
	trg.subsys = fm->subsys;

	ret = mtk_fm_mask_tbl_sw_mode(&trg, true);
	if (ret) {
		dev_err(dev, "mtk_fm_set_mask_tbl: sw_mode fail\n");
		return ret;
	}
	ret = mtk_fm_mask_tbl_write_idx(&trg, 0);
	if (ret) {
		dev_err(dev, "mtk_fm_set_mask_tbl: write_idx fail\n");
		return ret;
	}
	/* need to add gce poll ? */
	ret = mtk_fm_mask_tbl_write_ready(fm->regs);
	if (ret) {
		dev_err(dev, "mtk_fm_set_mask_tbl: write_ready fail\n");
		return ret;
	}

	for (i = 0; i < mask_tbl->size; i++) {
		ret = mtk_fm_mask_tbl_write_val(&trg,
				mask_tbl->mask_tbl[i]);
		if (ret) {
			dev_err(dev, "mtk_fm_set_mask_tbl: write_val fail\n");
			return ret;
		}
	}

	ret = mtk_fm_mask_tbl_write_ready(fm->regs);
	if (ret) {
		dev_err(dev, "mtk_fm_set_mask_tbl: write_ready fail\n");
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mtk_fm_set_mask_tbl);

/** @ingroup IP_group_fm_external_function
 * @par Description
 *     Set software mask table index.
 * @param[in] dev: FM device node.
 * @param[out] handle: CMDQ handle, set NULL for CPU write directly.
 * @param[in] idx: mask table index, 0 or 1.
 * @return
 *     0, set sw mask idx success.
 *     -EINVAL, invalid idx or NULL dev.
 *     error code from reg_write/_mask().
 * @par Boundary case and Limitation
 *     idx value should be limited to 0 or 1.
 * @par Error case and Error handling
 *     1. If fm dev or idx is invalid, return -EINVAL.\n
 *     2. If set following parameters with invalid values, return error code\n
 *         from reg_write/_mask().\n
 *         idx
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fm_set_mask_tbl_sw_idx(const struct device *dev,
			       struct cmdq_pkt *handle, u32 idx)
{
	struct mtk_fm *fm;
	struct reg_trg trg;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	fm = dev_get_drvdata(dev);
	if (!fm)
		return -EINVAL;

	if (idx > 1)
		return -EINVAL;

	trg.regs = fm->regs;
	trg.reg_base = fm->reg_base;
	trg.pkt = handle;
	trg.subsys = fm->subsys;

	ret = mtk_fm_mask_tbl_sw_idx(&trg, idx);
	if (ret)
		dev_err(dev, "mtk_fm_set_mask_tbl_sw_idx fail\n");

	return ret;
}
EXPORT_SYMBOL(mtk_fm_set_mask_tbl_sw_idx);

/** @ingroup IP_group_fm_external_function
 * @par Description
 *     Set hardware mask table index.
 * @param[in] dev: FM device node.
 * @param[out] handle: CMDQ handle, set NULL for CPU write directly.
 * @param[in] idx: mask table index, 0 or 1.
 * @return
 *     0, set hw mask idx success.
 *     -EINVAL, invalid idx or NULL dev.
 *     error code from reg_write/_mask().
 * @par Boundary case and Limitation
 *     idx value should be limited to 0 or 1.
 * @par Error case and Error handling
 *     1. If fm dev or idx is invalid, return -EINVAL.\n
 *     2. If set following parameters with invalid values, return error code\n
 *         from reg_write/_mask().\n
 *         idx
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fm_set_mask_tbl_hw_idx(const struct device *dev,
			       struct cmdq_pkt *handle, u32 idx)
{
	struct mtk_fm *fm;
	struct reg_trg trg;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	fm = dev_get_drvdata(dev);
	if (!fm)
		return -EINVAL;

	if (idx > 1)
		return -EINVAL;

	trg.regs = fm->regs;
	trg.reg_base = fm->reg_base;
	trg.pkt = handle;
	trg.subsys = fm->subsys;

	ret = mtk_fm_mask_tbl_hw_idx(&trg, idx);
	if (ret)
		dev_err(dev, "mtk_fm_set_mask_tbl_hw_idx fail\n");

	return ret;
}
EXPORT_SYMBOL(mtk_fm_set_mask_tbl_hw_idx);

/** @ingroup IP_group_fm_external_function
 * @par Description
 *     Set fm zncc threshold
 * @param[in] dev: FM device node.
 * @param[out] handle: CMDQ handle, set NULL for CPU write directly.
 * @param[in] zncc_threshold: zncc threshold of zncc engine, range 0 to 32767.
 * @return
 *     0, set zncc threshold success.
 *     -EINVAL, invalid parameters or NULL dev.
 *     error code from reg_write/_mask().
 * @par Boundary case and Limitation
 *     zncc_threshold sholud be limit to 0 to 32767
 * @par Error case and Error handling
 *     1. If fm dev or parameters are invalid, return -EINVAL.\n
 *     2. If set following parameters with invalid values, return error code\n
 *         from reg_write/_mask().\n
 *         zncc_threshold
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fm_set_zncc_threshold(const struct device *dev, struct cmdq_pkt *handle,
		u32 zncc_threshold)
{
	struct mtk_fm *fm;
	struct reg_trg trg;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	fm = dev_get_drvdata(dev);
	if (!fm)
		return -EINVAL;

	if (zncc_threshold > 0x7FFF)
		return -EINVAL;

	trg.regs = fm->regs;
	trg.reg_base = fm->reg_base;
	trg.pkt = handle;
	trg.subsys = fm->subsys;

	ret = mtk_fm_zncc_thrshld(&trg, zncc_threshold);
	if (ret) {
		dev_err(dev, "mtk_fm_start: mtk_fm_zncc_thrshld fail\n");
		return ret;
	}
	return 0;
}
EXPORT_SYMBOL(mtk_fm_set_zncc_threshold);


/** @ingroup IP_group_fm_external_function
 * @par Description
 *     Set input image's size.
 * @param[in] dev: FM device node.
 * @param[out] handle: CMDQ handle, set NULL for CPU write directly.
 * @param[in] w: width of input image, range 160 to 640
 * @param[in] h: height of input image, range 120 to 640
 * @param[in] blk_nr: total block number from fe, range 25 to 400.
 * @return
 *     0, set region success.
 *     -EINVAL, invalid parameters or NULL dev.
 *     error code from reg_write/_mask().
 * @par Boundary case and Limitation
 *     width and height should be limited to 160x120~640x640.
 *     block number should be limited to 25~400.
 * @par Error case and Error handling
 *     1. If fm dev or parameters are invalid, return -EINVAL.\n
 *     2. If set following parameters with invalid values, return error code\n
 *         from reg_write/_mask().\n
 *         w, h
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fm_set_region(const struct device *dev, struct cmdq_pkt *handle,
		      u32 w, u32 h, u32 blk_nr)
{
	struct mtk_fm *fm;
	struct reg_trg trg;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	fm = dev_get_drvdata(dev);
	if (!fm)
		return -EINVAL;

	if (w > 640 || w < 160) {
		dev_err(dev, "mtk_fm_set_region: invalid w=%d\n", w);
		return -EINVAL;
	}

	if (h > 640 || h < 120) {
		dev_err(dev, "mtk_fm_set_region: invalid h=%d\n", h);
		return -EINVAL;
	}

	if (blk_nr > 400 || blk_nr < 25) {
		dev_err(dev, "mtk_fm_set_region: invalid blk_nr=%d\n", blk_nr);
		return -EINVAL;
	}

	trg.regs = fm->regs;
	trg.reg_base = fm->reg_base;
	trg.pkt = handle;
	trg.subsys = fm->subsys;

	ret = mtk_fm_in_img_size(&trg, w, h, blk_nr);
	if (ret)
		dev_err(dev, "mtk_fm_set_region: mtk_fm_in_img_size fail\n");

	return ret;
}
EXPORT_SYMBOL(mtk_fm_set_region);

/** @ingroup IP_group_fm_external_function
 * @par Description
 *     Set fm read/write request interval.
 * @param[in] dev: FM device node.
 * @param[out] handle: CMDQ handle, set NULL for CPU write directly.
 * @param[in] img_req_interval:
 *	rdma(img) request interval, range 1 to 65535(cycle)
 * @param[in] desc_req_interval:
 *	rdma(descriptor) request interval, range 1 to 65535(cycle)
 * @param[in] point_req_interval:
 *	rdma(point) request interval, range 1 to 65535(cycle)
 * @param[in] fmo_req_interval:
 *	wdma(fmo) request interval, range 1 to 65535(cycle)
 * @param[in] zncc_req_interval:
 *	wdma(zncc_subpix) request interval, range 1 to 65535(cycle)
 * @return
 *     0, set request interval success.
 *     -EINVAL, invalid parameters or NULL dev.
 *     error code from reg_write/_mask().
 * @par Boundary case and Limitation
 *     each interval should be limited to 1~65535(16bits).
 * @par Error case and Error handling
 *     1. If fm dev or parameters are invalid, return -EINVAL.\n
 *     2. If set following parameters with invalid values, return error code\n
 *         from reg_write/_mask().\n
 *         req0, req1, req2, req3, req4.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fm_set_req_interval(const struct device *dev, struct cmdq_pkt *handle,
	u32 img_req_interval, u32 desc_req_interval, u32 point_req_interval,
	u32 fmo_req_interval, u32 zncc_req_interval)
{
	struct mtk_fm *fm;
	struct reg_trg trg;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	fm = dev_get_drvdata(dev);
	if (!fm)
		return -EINVAL;

	if (img_req_interval > 65535 || img_req_interval < 1) {
		dev_err(dev, "mtk_fm_set_req_interval: invalid req=%d\n",
			img_req_interval);
		return -EINVAL;
	}

	if (desc_req_interval > 65535 || desc_req_interval < 1) {
		dev_err(dev, "mtk_fm_set_req_interval: invalid req=%d\n",
			desc_req_interval);
		return -EINVAL;
	}

	if (point_req_interval > 65535 || point_req_interval < 1) {
		dev_err(dev, "mtk_fm_set_req_interval: invalid req=%d\n",
			point_req_interval);
		return -EINVAL;
	}

	if (fmo_req_interval > 65535 || fmo_req_interval < 1) {
		dev_err(dev, "mtk_fm_set_req_interval: invalid req=%d\n",
			fmo_req_interval);
		return -EINVAL;
	}

	if (zncc_req_interval > 65535 || zncc_req_interval < 1) {
		dev_err(dev, "mtk_fm_set_req_interval: invalid req=%d\n",
			zncc_req_interval);
		return -EINVAL;
	}

	trg.regs = fm->regs;
	trg.reg_base = fm->reg_base;
	trg.pkt = handle;
	trg.subsys = fm->subsys;

	ret = mtk_fm_req_interval(&trg, img_req_interval, desc_req_interval,
		point_req_interval, fmo_req_interval, zncc_req_interval);
	if (ret)
		dev_err(dev,
			"mtk_fm_set_req_interval: mtk_fm_req_interval fail\n");

	return ret;
}
EXPORT_SYMBOL(mtk_fm_set_req_interval);

/** @ingroup IP_group_fm_external_function
 * @par Description
 *     Set fm search center.
 * @param[in] dev: FM device node.
 * @param[out] handle: CMDQ handle, set NULL for CPU write directly.
 * @param[in] sc: user-defined search center.
 * @return
 *     0, set search center success.
 *     -EINVAL, invalid mask_tbl or NULL dev.
 *     error code from reg_write/_mask().
 * @par Boundary case and Limitation
 *     each block can set one search center, so total search center number
 *         must less than max block number(400).
 * @par Error case and Error handling
 *     1. If fm dev or search_center is NULL, return -EINVAL.\n
 *     2. If set following parameters with invalid values, return error code\n
 *         from reg_write/_mask().\n
 *         sc, search center setting
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */

int mtk_fm_set_search_center(const struct device *dev, struct cmdq_pkt *handle,
			     struct mtk_fm_search_center *sc)
{
	struct mtk_fm *fm;
	struct reg_trg trg;
	int ret = 0, i = 0;

	if (!dev)
		return -EINVAL;

	fm = dev_get_drvdata(dev);
	if (!fm)
		return -EINVAL;

	if (!sc)
		return -EINVAL;

	if (sc->size > 400) {
		dev_err(dev, "mtk_fm_set_search center: search center size %d overflow\n",
			sc->size);
		return -EINVAL;
	}

	trg.regs = fm->regs;
	trg.reg_base = fm->reg_base;
	trg.pkt = handle;
	trg.subsys = fm->subsys;

	ret = mtk_fm_search_center_write_idx(&trg, 0);
	if (ret) {
		dev_err(dev, "mtk_fm_set_mask_tbl: set write_idx fail\n");
		return ret;
	}

	/* need to add gce poll ? */
	ret = mtk_fm_search_center_write_ready(fm->regs);
	if (ret) {
		dev_err(dev,
			"mtk_fm_set_search_center: poll write_ready fail\n");
		return ret;
	}

	for (i = 0; i < sc->size; i++) {
		ret = mtk_fm_search_center_write_val(&trg, sc->sc[i]);
		if (ret) {
			dev_err(dev,
				"mtk_fm_set_search_center: write_val fail\n");
			return ret;
		}
	}
	return ret;
}
EXPORT_SYMBOL(mtk_fm_set_search_center);


/** @ingroup IP_group_fm_external_function
 * @par Description
 *     Enable FM engine, dram, and switch to software control mode; set\n
 *     default ZNCC threshold.
 * @param[in] dev: FM device node.
 * @param[out] handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, trigger fm start success.
 *     -EINVAL, NULL dev.
 *     error code from reg_write/_mask().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If fm dev is NULL, return -EINVAL.\n
 *     2. If write register fail, return error code from reg_write/_mask().
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fm_start(const struct device *dev, struct cmdq_pkt *handle)
{
	struct mtk_fm *fm;
	struct reg_trg trg;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	fm = dev_get_drvdata(dev);
	if (!fm)
		return -EINVAL;

	trg.regs = fm->regs;
	trg.reg_base = fm->reg_base;
	trg.pkt = handle;
	trg.subsys = fm->subsys;

	ret = mtk_fm_dram_enable(&trg);
	if (ret) {
		dev_err(dev, "mtk_fm_start: mtk_fm_dram_enable fail\n");
		return ret;
	}

	ret = mtk_fm_enable(&trg);
	if (ret) {
		dev_err(dev, "mtk_fm_start: mtk_fm_enable fail\n");
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mtk_fm_start);

/** @ingroup IP_group_fm_external_function
 * @par Description
 *     Disable FM engine and dram
 * @param[in] dev: FM device node.
 * @param[out] handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, trigger fm stop success.
 *     -EINVAL, NULL dev.
 *     error code from reg_write/_mask().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If fm dev is NULL, return -EINVAL.\n
 *     2. If write register fail, return error code from reg_write/_mask().
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fm_stop(const struct device *dev, struct cmdq_pkt *handle)
{
	struct mtk_fm *fm;
	struct reg_trg trg;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	fm = dev_get_drvdata(dev);
	if (!fm)
		return -EINVAL;

	trg.regs = fm->regs;
	trg.reg_base = fm->reg_base;
	trg.pkt = handle;
	trg.subsys = fm->subsys;

	ret = mtk_fm_disable(&trg);
	if (ret) {
		dev_err(dev, "mtk_fm_stop: mtk_fm_disable fail\n");
		return ret;
	}

	ret = mtk_fm_dram_disable(&trg);
	if (ret) {
		dev_err(dev, "mtk_fm_stop: mtk_fm_dram_disable fail\n");
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mtk_fm_stop);

/** @ingroup IP_group_fm_external_function
 * @par Description
 *     Soft reset FM
 * @param[in] dev: FM device node.
 * @return
 *     0, trigger fm reset success.
 *     -EINVAL, NULL dev.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If fm dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fm_reset(const struct device *dev)
{
	struct mtk_fm *fm;

	if (!dev)
		return -EINVAL;

	fm = dev_get_drvdata(dev);
	if (!fm)
		return -EINVAL;

	mtk_fm_rst(fm->regs);

	return 0;
}
EXPORT_SYMBOL(mtk_fm_reset);

/** @ingroup IP_group_fm_external_function
 * @par Description
 * write value to specific fm registers.
 * @param[in]
 *     dev: pointer of fm device structure.
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
 *     If fm dev is NULL, return -EINVAL.
 *     If offset > 0x1000, return -EINVAL
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fm_write_register(const struct device *dev,
	u32 offset, u32 value)
{
	struct mtk_fm *fm;
	struct reg_trg trg;

	if (!dev)
		return -EINVAL;

	fm = dev_get_drvdata(dev);
	if (!fm)
		return -EINVAL;

	if (offset > FM_REG_MAX_OFFSET || offset < FM_REG_MIN_OFFSET)
		return -EINVAL;

	trg.regs = fm->regs;
	trg.reg_base = fm->reg_base;
	trg.pkt = NULL;
	trg.subsys = fm->subsys;

	return reg_write(&trg, offset, value);
}
EXPORT_SYMBOL(mtk_fm_write_register);

/** @ingroup IP_group_fm_external_function
 * @par Description
 *     read value from specific fm registers.
 * @param[in]
 *     dev: pointer of fm device structure.
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
 *     If fm dev is NULL, return -EINVAL.
 *     If offset > 0x1000, return -EINVAL
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fm_read_register(const struct device *dev, u32 offset, u32 *value)
{
	struct mtk_fm *fm;

	if (!dev)
		return -EINVAL;

	fm = dev_get_drvdata(dev);
	if (!fm || !value)
		return -EINVAL;

	if (offset > FM_REG_MAX_OFFSET || offset < FM_REG_MIN_OFFSET)
		return -EINVAL;

	*value = readl(fm->regs + offset);

	return 0;
}
EXPORT_SYMBOL(mtk_fm_read_register);


/** @ingroup IP_group_fm_external_function
 * @par Description
 *     Enable FM clock to power on.
 * @param[in] dev: FM device node.
 * @return
 *     0, fm power on success.\n
 *     -EINVAL, invalid dev.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If fm dev is NULL, return -EINVAL
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fm_power_on(struct device *dev)
{
	struct mtk_fm *fm;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	fm = dev_get_drvdata(dev);
	if (!fm)
		return -EINVAL;

	if (fm->valid_func & CLK) {
#ifdef CONFIG_COMMON_CLK_MT3612
		ret = pm_runtime_get_sync(dev);
		if (ret < 0) {
			dev_err(fm->dev,
				"Failed to enable power domain: %d\n", ret);
			return ret;
		}
		if (fm->clk_mdp) {
			ret = clk_prepare_enable(fm->clk_mdp);
			if (ret) {
				dev_err(fm->dev,
					"Failed to enable clock:%d\n", ret);
				return ret;
			}
		}
#endif
	}
	if (fm->larbdev != NULL) {
		if (fm->valid_func & LARB_IOMMU)
			mtk_smi_larb_get(fm->larbdev);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_fm_power_on);

/** @ingroup IP_group_fm_external_function
 * @par Description
 *     Disable FM clock to power off.
 * @param[in] dev: FM device node.
 * @return
 *     0, power off success.\n
 *     -EINVAL, invalid dev.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If fm dev is NULL, return -EINVAL
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fm_power_off(struct device *dev)
{
	struct mtk_fm *fm;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	fm = dev_get_drvdata(dev);
	if (!fm)
		return -EINVAL;
	if (fm->valid_func & CLK) {
#ifdef CONFIG_COMMON_CLK_MT3612
		if (fm->clk_mdp)
			clk_disable_unprepare(fm->clk_mdp);
		pm_runtime_put(dev);
#endif
	}
	if (fm->larbdev != NULL) {
		if (fm->valid_func & LARB_IOMMU)
			mtk_smi_larb_put(fm->larbdev);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_fm_power_off);

/** @ingroup IP_group_fm_external_function
 * @par Description
 *     Register callback function for fm interrupts.
 * @param[in] dev: pointer of fm device structure.
 * @param[in] func: callback function which is called by fm interrupt handler.
 * @param[in] priv_data: caller may have multiple instance, so it could use\n
 *         this private data to distinguish which instance is callbacked.
 * @return
 *     0, successfully register callback function.\n
 *     -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_fm_register_cb(const struct device *dev, mtk_mmsys_cb func,
		       void *priv_data)
{
	struct mtk_fm *fm;
	unsigned long flags;
	u32 reg;

	if (!dev)
		return -EINVAL;

	fm = dev_get_drvdata(dev);
	if (!fm)
		return -EINVAL;

	spin_lock_irqsave(&fm->spinlock_irq, flags);
	fm->cb_func = func;
	fm->cb_priv_data = priv_data;
	spin_unlock_irqrestore(&fm->spinlock_irq, flags);

	/* enable interrupts: clear mask*/
	if (func) {
		reg = readl(fm->regs + FM_INTEN);
		reg |= FM_OF_END_INT_EN;
		writel(reg, fm->regs + FM_INTEN);
	} else {
		reg = readl(fm->regs + FM_INTEN);
		reg &= ~FM_OF_END_INT_EN;
		writel(reg, fm->regs + FM_INTEN);
	}

	return 0;
}
EXPORT_SYMBOL(mtk_fm_register_cb);

static const enum dev_stage fpga = FPGA;
static const enum dev_stage a0 = A0;

/** @ingroup IP_group_fm_internal_struct
 * @brief FM Open Framework Device ID.\n
 * This structure is used to attach specific names to\n
 * platform device for use with device tree.
 */
static const struct of_device_id fm_driver_dt_match[] = {
	{ .compatible = "mediatek,mt3612-fm-fpga", .data = (void *)&fpga },
	{ .compatible = "mediatek,mt3612-fm", .data = (void *)&a0 },
	{},
};
MODULE_DEVICE_TABLE(of, fm_driver_dt_match);

#ifdef CONFIG_MTK_DEBUGFS
static int fm_debug_show(struct seq_file *s, void *unused)
{
	struct mtk_fm *fm = s->private;
	u32 reg;

	reg = readl(fm->regs + FM_THR);

	seq_puts(s, "--------------------- FM DEUBG ---------------------\n");
	seq_printf(s, "FM ZNCC_TH:              :0x%.8x\n", reg);

	reg = readl(fm->regs + FM_TYPE);
	seq_printf(s, "FM SR_TYPE               :0x%.8x\n",
		   (u32)(reg & (FM_FM_SR_TYPE)) >> 1);
	seq_printf(s, "FM BLK_TYPE              :0x%.8x\n",
		   (u32)(reg & (FM_FM_BLK_TYPE)) >> 3);

	reg = readl(fm->regs + FM_INPUT_SIZE);
	seq_printf(s, "FM WIDTH                 :0x%.8x\n",
		   (u32)(reg & (FM_FM_IN_WIDTH)));
	seq_printf(s, "FM HEIGHT                :0x%.8x\n",
		   (u32)(reg & (FM_FM_IN_HEIGHT)) >> 16);

	reg = readl(fm->regs + FM_INPUT_INFO);
	seq_printf(s, "FM IMG_STRIDE            :0x%.8x\n",
		   (u32)(reg & (FM_FM_IMG_STRIDE)) >> 4);
	seq_printf(s, "FM BLK_NUM               :0x%.8x\n",
		   (u32)(reg & (FM_FM_BLK_NUM)) >> 16);

	reg = readl(fm->regs + FM_RADDR0);
	seq_printf(s, "FM IMG_ADDR:             :0x%.8x\n", reg);

	reg = readl(fm->regs + FM_RADDR1);
	seq_printf(s, "FM FEO_DESC_ADDR:        :0x%.8x\n", reg);

	reg = readl(fm->regs + FM_RADDR2);
	seq_printf(s, "FM FEO_POINT_ADDR:       :0x%.8x\n", reg);

	reg = readl(fm->regs + FM_WADDR0);
	seq_printf(s, "FM FMO_ADDR:             :0x%.8x\n", reg);

	reg = readl(fm->regs + FM_WADDR1);
	seq_printf(s, "FM ZNCC_SUB_ADDR:        :0x%.8x\n", reg);

	reg = readl(fm->regs + FM_STATUS);
	seq_printf(s, "FM FMO_IDLE:             :%10x\n",
		   (u32)(reg & (FM_WDMA_IDLE)));
	seq_printf(s, "FM ZNCC_SUB_IDLE:        :%10x\n",
		   (u32)(reg & (FM_WDMA_SUBPIX_IDLE)) >> 5);
	seq_printf(s, "FM IMG_RDMA_IDLE:        :%10x\n",
		   (u32)(reg & (FM_RDMA0_IDLE)) >> 1);
	seq_printf(s, "FM DESC_RDMA_IDLE:       :%10x\n",
		   (u32)(reg & (FM_RDMA1_IDLE)) >> 2);
	seq_printf(s, "FM POINT_RDMA_IDLE:      :%10x\n",
		   (u32)(reg & (FM_RDMA2_IDLE)) >> 3);

	seq_printf(s, "FM ACTIVE:               :%10x\n",
		   (u32)(reg & (FM_FM_ACTIVE)) >> 4);

	seq_puts(s, "----------------------------------------------------\n");
	return 0;
}


static int debug_client_open(struct inode *inode, struct file *file)
{
	return single_open(file, fm_debug_show, inode->i_private);
}

static const struct file_operations fm_debug_fops = {
	.open = debug_client_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

int dump_fm_reg(struct device *dev)
{
	struct mtk_fm *fm = dev_get_drvdata(dev);
	u32 reg;

	reg = readl(fm->regs + FM_THR);

	dev_err(dev, "-------------------- FM REG DUMP --------------------\n");
	dev_err(dev, "FM ZNCC_TH:              :0x%.8x\n", reg);

	reg = readl(fm->regs + FM_TYPE);
	dev_err(dev, "FM SR_TYPE               :0x%.8x\n",
		   (u32)(reg & (FM_FM_SR_TYPE)) >> 1);
	dev_err(dev, "FM BLK_TYPE              :0x%.8x\n",
		   (u32)(reg & (FM_FM_BLK_TYPE)) >> 3);

	reg = readl(fm->regs + FM_INPUT_SIZE);
	dev_err(dev, "FM WIDTH                 :0x%.8x\n",
		   (u32)(reg & (FM_FM_IN_WIDTH)));
	dev_err(dev, "FM HEIGHT                :0x%.8x\n",
		   (u32)(reg & (FM_FM_IN_HEIGHT)) >> 16);

	reg = readl(fm->regs + FM_INPUT_INFO);
	dev_err(dev, "FM IMG_STRIDE            :0x%.8x\n",
		   (u32)(reg & (FM_FM_IMG_STRIDE)) >> 4);
	dev_err(dev, "FM BLK_NUM               :0x%.8x\n",
		   (u32)(reg & (FM_FM_BLK_NUM)) >> 16);

	reg = readl(fm->regs + FM_RADDR0);
	dev_err(dev, "FM IMG_ADDR:             :0x%.8x\n", reg);

	reg = readl(fm->regs + FM_RADDR1);
	dev_err(dev, "FM FEO_DESC_ADDR:        :0x%.8x\n", reg);

	reg = readl(fm->regs + FM_RADDR2);
	dev_err(dev, "FM FEO_POINT_ADDR:       :0x%.8x\n", reg);

	reg = readl(fm->regs + FM_WADDR0);
	dev_err(dev, "FM FMO_ADDR:             :0x%.8x\n", reg);

	reg = readl(fm->regs + FM_WADDR1);
	dev_err(dev, "FM ZNCC_SUB_ADDR:        :0x%.8x\n", reg);

	reg = readl(fm->regs + FM_STATUS);
	dev_err(dev, "FM FMO_IDLE:             :%10x\n",
		   (u32)(reg & (FM_WDMA_IDLE)));
	dev_err(dev, "FM ZNCC_SUB_IDLE:        :%10x\n",
		   (u32)(reg & (FM_WDMA_SUBPIX_IDLE)) >> 5);
	dev_err(dev, "FM IMG_RDMA_IDLE:        :%10x\n",
		   (u32)(reg & (FM_RDMA0_IDLE)) >> 1);
	dev_err(dev, "FM DESC_RDMA_IDLE:       :%10x\n",
		   (u32)(reg & (FM_RDMA1_IDLE)) >> 2);
	dev_err(dev, "FM POINT_RDMA_IDLE:      :%10x\n",
		   (u32)(reg & (FM_RDMA2_IDLE)) >> 3);

	dev_err(dev, "FM ACTIVE:               :%10x\n",
		   (u32)(reg & (FM_FM_ACTIVE)) >> 4);

	dev_err(dev, "-----------------------------------------------------\n");
	return 0;
}
EXPORT_SYMBOL(dump_fm_reg);
#endif

/** @ingroup IP_group_fm_internal_function
 * @par Description
 *     Get Necessary Hardware Information from Device Tree.
 * @param[in] pdev: platform device node.
 * @return
 *     If return value is 0, function success.
 *     Otherwise, FM probe failed.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If there is any error in the probe flow,\n
 *     system error value will be returned.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_fm_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_fm *fm;
	struct resource *regs;
	const void *ptr;
	struct platform_device *larb_pdev;
	struct device_node *larb_node;
	int err;
	const struct of_device_id *of_id;
	u32 stage;
#ifdef CONFIG_MTK_DEBUGFS
	struct dentry *debug_dentry = NULL;
#endif
	int ret;

	dev_dbg(dev, "start fm probe!!\n");

	fm = devm_kzalloc(dev, sizeof(*fm), GFP_KERNEL);
	if (!fm)
		return -ENOMEM;

	of_id = of_match_device(fm_driver_dt_match, &pdev->dev);
	if (!of_id)
		return -ENODEV;

	stage = *(enum dev_stage *)of_id->data;
	dev_dbg(dev, "dt match fm dev_stage: %d\n", stage);

	if (stage != FPGA) {
#ifdef CONFIG_COMMON_CLK_MT3612
		fm->clk_mdp = devm_clk_get(dev, "mmsys_cmm_mdp_fm");
		if (IS_ERR(fm->clk_mdp)) {
			dev_err(dev, "Failed to get fm clock mdp\n");
			return PTR_ERR(fm->clk_mdp);
		}
		fm->valid_func |= CLK;
#endif
		fm->valid_func |= LARB_IOMMU;
	} else {
		fm->valid_func = LARB_IOMMU;
	}
	dev_dbg(dev, "platform_get_resource\n");
	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	fm->regs = devm_ioremap_resource(dev, regs);
	if (IS_ERR(fm->regs)) {
		dev_err(dev, "Failed to map mtk_fm registers\n");
		return PTR_ERR(fm->regs);
	}
	fm->reg_base = (u32)regs->start;

	dev_dbg(dev, "spin_lock_init\n");
	spin_lock_init(&fm->spinlock_irq);
	dev_dbg(dev, "platform_get_irq\n");
	fm->irq = platform_get_irq(pdev, 0);
	if (fm->irq < 0) {
		dev_err(dev, "Failed to get fm irq: %d\n", fm->irq);
		return fm->irq;
	}

	dev_dbg(dev, "devm_request_irq\n");
	err = devm_request_irq(dev, fm->irq, mtk_fm_irq_handler,
			       IRQF_TRIGGER_LOW, dev_name(dev), fm);
	if (err < 0) {
		dev_err(dev, "Failed to request fm irq: %d\n", err);
		return err;
	}
	dev_dbg(dev, "of_get_property\n");
	ptr = of_get_property(dev->of_node, "gce-subsys", NULL);
	if (ptr)
		fm->subsys = be32_to_cpup(ptr);
	else
		fm->subsys = 0;
	if (fm->valid_func & LARB_IOMMU) {
		larb_node = of_parse_phandle(dev->of_node, "mediatek,larbs", 0);
		if (!larb_node) {
			dev_err(dev,
				"Missing mediadek,larb phandle in larbs\n");
			return -EINVAL;
		}
		larb_pdev = of_find_device_by_node(larb_node);
		if (!dev_get_drvdata(&larb_pdev->dev)) {
			dev_warn(dev, "FM is waiting for larb device %s\n",
				 larb_node->full_name);
			of_node_put(larb_node);
			return -EPROBE_DEFER;
		}

		fm->larbdev = &larb_pdev->dev;
		ret = mtk_smi_larb_get(fm->larbdev);
		if (ret < 0)
			dev_err(dev, "Failed to get larb: %d\n", ret);

		pr_debug("fm smi larb get\n");
		of_node_put(larb_node);

		of_node_put(dev->of_node);
	}
	if (fm->valid_func & CLK) {
#ifdef CONFIG_COMMON_CLK_MT3612
		pm_runtime_enable(dev);
#endif
	}

	fm->blk_size = 32;

	dev_dbg(dev, "platform_set_drvdata\n");
	platform_set_drvdata(pdev, fm);

#ifdef CONFIG_MTK_DEBUGFS
	if (!mtk_debugfs_root)
		goto debugfs_done;

	/* debug info create */
	if (!mtk_fm_debugfs_root)
		mtk_fm_debugfs_root = debugfs_create_dir("fm",
							 mtk_debugfs_root);

	if (!mtk_fm_debugfs_root) {
		dev_dbg(dev, "failed to create fm debugfs root directory.\n");
		goto debugfs_done;
	}

	debug_dentry = debugfs_create_file(pdev->name, 0664,
					   mtk_fm_debugfs_root,
					   fm, &fm_debug_fops);

debugfs_done:
	if (!debug_dentry)
		dev_warn(dev, "Failed to create fm debugfs %s\n", pdev->name);

#endif

	dev_dbg(dev, "fm probe done!!\n");
	return 0;
}

/** @ingroup IP_group_fm_internal_function
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
static int mtk_fm_remove(struct platform_device *pdev)
{
	struct mtk_fm *fm = platform_get_drvdata(pdev);

	devm_free_irq(&pdev->dev, fm->irq, fm);
	if (fm->valid_func & CLK) {
#ifdef CONFIG_COMMON_CLK_MT3612
		pm_runtime_disable(&pdev->dev);
#endif
	}
	if (fm->valid_func & LARB_IOMMU)
		mtk_smi_larb_put(fm->larbdev);

#ifdef CONFIG_MTK_DEBUGFS
	debugfs_remove_recursive(mtk_fm_debugfs_root);
	mtk_fm_debugfs_root = NULL;
#endif

	return 0;
}

/** @ingroup IP_group_fm_internal_struct
 * @brief FM platform driver structure.\n
 * This structure is used to register itself.
 */
struct platform_driver mtk_fm_driver = {
	.probe		= mtk_fm_probe,
	.remove		= mtk_fm_remove,
	.driver		= {
		.name	= "mediatek-fm",
		.owner	= THIS_MODULE,
		.of_match_table = fm_driver_dt_match,
	},
};

module_platform_driver(mtk_fm_driver);
MODULE_LICENSE("GPL");
