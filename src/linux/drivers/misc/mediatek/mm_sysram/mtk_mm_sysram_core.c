/*
 * Copyright (C) 2018 MediaTek Inc.
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
 * @file mtk_mm_sysram_core.c
 *  This mm_sysram driver is used to control MTK mm_sysram hardware module.
 */

/**
 * @defgroup IP_group_MM_SYSRAM MM_SYSRAM
 *     MM_SYSRAM module
 *     @{
 *         @defgroup IP_group_mm_sysram_external EXTERNAL
 *             The external API document for mm_sysram.\n
 *             The mm_sysram driver follows native Linux framework
 *             @{
 *               @defgroup IP_group_mm_sysram_external_function 1.function
 *                   External function for mm_sysram.
 *               @defgroup IP_group_mm_sysram_external_struct 2.structure
 *                   External structure used for mm_sysram.
 *               @defgroup IP_group_mm_sysram_external_typedef 3.typedef
 *                   none. Native Linux Driver.
 *               @defgroup IP_group_mm_sysram_external_enum 4.enumeration
 *                   External enumeration used for mm_sysram.
 *               @defgroup IP_group_mm_sysram_external_def 5.define
 *                   External definition used for mm_sysram.
 *             @}
 *
 *         @defgroup IP_group_mm_sysram_internal INTERNAL
 *             The internal API document for mm_sysram.
 *             @{
 *               @defgroup IP_group_mm_sysram_internal_function 1.function
 *                   Internal function for mm_sysram and module init.
 *               @defgroup IP_group_mm_sysram_internal_struct 2.structure
 *                   Internal structure used for mm_sysram.
 *               @defgroup IP_group_mm_sysram_internal_typedef 3.typedef
 *                   none. Follow Linux coding style, no typedef used.
 *               @defgroup IP_group_mm_sysram_internal_enum 4.enumeration
 *                   Internal enumeration used for mm_sysram.
 *               @defgroup IP_group_mm_sysram_internal_def 5.define
 *                   Internal definition used for mm_sysram.
 *             @}
 *     @}
 */

#include <linux/clk.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/seq_file.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/uaccess.h>
#include <asm/unaligned.h>

#include <soc/mediatek/mtk_mm_sysram.h>

#include "mtk_mm_sysram_reg.h"
#include "mtk_sysram_smi_common_reg.h"
#include "power_controller.h"

/**
 * @ingroup IP_group_mm_sysram_internal_def
 * @{
 */
#define MM_SYSRAM_ALIGNMENT		(sizeof(u32) * 4)
#define MM_SYSRAM_CMD_MAX_TRY		10
#define MM_SYSRAM_CMD_TRY_INTERVAL	4

#define SYSRAM2_WEIGHT(w) ((w) | ((w) << 5) | ((w) << 10) | ((w) << 15))
/**
 * @}
 */

/**
 * @ingroup IP_group_mm_sysram_internal_struct
 * @brief MM_SYSRAM Driver Data Structure.
 */
struct mm_sysram_s {
	/** mm_sysram device ID */
	int dev_id;
	/** mm_sysram size */
	int size;
	/** mm_sysram register base */
	void __iomem *base;

	/** number of SMI_COMMON */
	u32 smi_num;
	/** base of SMI_COMMON */
	void **smi_bases;

#ifdef CONFIG_COMMON_CLK_MT3612
	/** mm_sysram clock gate */
	struct clk *clk_gate;
	/** mm_sysram clock mux */
	struct clk *clk_sel;
	/** mm_sysram clock source */
	struct clk *clk_src;
	/** mm_sysram clock source name */
	char *clk_src_name;
	/** mm_sysram clock rate */
	u32 clk_rate;
#endif

	/** mm_sysram local power control feature */
	struct power_controller pwr_ctrl;
	/** mm_sysram enable state */
	int enable;

	/** mm_sysram mutex */
	struct mutex lock;
};

/**
 * @ingroup IP_group_mm_sysram_internal_function
 * @par Description
 *      Wait specific bit become 1
 * @param[in] reg: register
 * @param[in] bit: bit in the register
 * @return
 *      0, successful\n
 *      -1, timeout
 */
static int wait_bit_up(const u32 *reg, const u32 bit)
{
	u32 tmp, tried;

	for (tried = 0; tried < MM_SYSRAM_CMD_MAX_TRY; tried++) {
		tmp = readl(reg);

		if (tmp & bit)
			return 0;

		msleep(MM_SYSRAM_CMD_TRY_INTERVAL);
	}

	return -1;
}

/**
 * @ingroup IP_group_mm_sysram_internal_function
 * @par Description
 *      Wait specific bit become 0
 * @param[in] reg: register
 * @param[in] bit: bit in the register
 * @return
 *      0, successful\n
 *      -1, timeout
 */
static int wait_bit_down(const u32 *reg, const u32 bit)
{
	u32 tmp, tried;

	for (tried = 0; tried < MM_SYSRAM_CMD_MAX_TRY; tried++) {
		tmp = readl(reg);

		if (!(tmp & bit))
			return 0;

		msleep(MM_SYSRAM_CMD_TRY_INTERVAL);
	}

	return -1;
}

#ifdef CONFIG_COMMON_CLK_MT3612
/**
 * @ingroup IP_group_mm_sysram_internal_function
 * @par Description
 *	Enable mm_sysram clock.
 * @param[in] dev: mm_sysram device node.
 * @return
 *	0, successful.\n
 *	-EINVAL, invalid argument.\n
 *	-ENODEV, no such device.
 * @par Boundary case and Limitation
 *	the device node should be exist.
 * @par Error case and Error handling
 *	If clk_prepare_enable() failed, then call clk_disable_unprepare().
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_mm_sysram_enable_clk(struct device *dev)
{
	struct mm_sysram_s *mm_sysram;
	int ret;

	if (!dev)
		return -EINVAL;

	mm_sysram = dev_get_drvdata(dev);

	if (!mm_sysram)
		return -ENODEV;

	if (!mm_sysram->clk_gate)
		return 0;

	ret = clk_prepare_enable(mm_sysram->clk_gate);

	if (ret) {
		clk_disable_unprepare(mm_sysram->clk_gate);
		dev_err(dev, "Failed to enable mm_sysram clock: %d\n", ret);
		return ret;
	}

	return 0;
}

/**
 * @ingroup IP_group_mm_sysram_internal_function
 * @par Description
 *	Disable mm_sysram clock.
 * @param[in] dev: mm_sysram device node.
 * @return
 *	0, successful.\n
 * @par Boundary case and Limitation
 *	the device node should be exist.
 * @par Error case and Error handling
 *	none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_mm_sysram_disable_clk(struct device *dev)
{
	struct mm_sysram_s *mm_sysram;

	if (!dev)
		return -EINVAL;

	mm_sysram = dev_get_drvdata(dev);

	if (!mm_sysram)
		return -ENODEV;

	if (mm_sysram->clk_gate)
		clk_disable_unprepare(mm_sysram->clk_gate);

	return 0;
}
#endif /* #ifdef CONFIG_COMMON_CLK_MT3612 */

/**
 * @ingroup IP_group_mm_sysram_internal_function
 * @par Description
 *	Initialize mm_sysram inidividual part.
 * @param[in] dev: mm_sysram device node.
 * @return
 *	0, if successful.\n
 *	Otherwise, mm_sysram initial failed.
 * @par Boundary case and Limitation
 *	none.
 * @par Error case and Error handling
 *	If there is any error during initial flow,\n
 *	system error number will be returned.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_mm_sysram_init_individual(struct device *dev)
{
	struct mm_sysram_s *mm_sysram;

	if (!dev)
		return -EINVAL;

	mm_sysram = dev_get_drvdata(dev);

	if (!mm_sysram)
		return -ENODEV;

	switch (mm_sysram->dev_id) {
	case 0:
	case 1:
		/** Initialize local power control feature */
		pwr_ctrl_init(&mm_sysram->pwr_ctrl, dev, mm_sysram->size,
						mm_sysram->base,
						PWR_CTRL_TYPE_NORMAL);
		break;

	case 2:
		pwr_ctrl_init(&mm_sysram->pwr_ctrl, dev, mm_sysram->size,
						mm_sysram->base,
						PWR_CTRL_TYPE_3612_SRAM2);
		break;

	default:
		return -ENODEV;
	}

	return 0;
}

/**
 * @ingroup IP_group_mm_sysram_external_function
 * @par Description
 *	Enable mm_sysram clock and power on.
 * @param[in] dev: mm_sysram device node.
 * @param[in] addr: start address (0 ~ (size described in the device tree))
 * @param[in] size: requested size (1 ~ (size described in the device tree))
 * @return
 *	0, configuration successful.\n
 *	-EINVAL, invalid argument.\n
 *	-ENODEV, no such device.\n
 * @par Boundary case and Limitation
 *	(addr + size) must not greater than sysram size
 *      {addr, size} pair must not overlap with regions already requested
 * @par Error case and Error handling
 *	If dev is NULL, return -EINVAL.
 *      If {addr, size} pair is invalid, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mm_sysram_power_on(struct device *dev, const int addr, const int size)
{
	struct mm_sysram_s *mm_sysram;
	u32 tmp;
	int i;
	int ret;

	/** Sanity check - Pass 1 */
	if (!dev)
		return -EINVAL;

	/** Get driver data and lock */
	mm_sysram = dev_get_drvdata(dev);

	if (!mm_sysram)
		return -ENODEV;

	mutex_lock(&mm_sysram->lock);

	/** Sanity check - Pass 2 */
	if (addr < 0 || addr > mm_sysram->size) {
		dev_err(dev, "Start address is out of range: 0x%x\n", addr);
		ret = -EINVAL;
		goto err_after_lock;
	}

	if (size <= 0 || size > mm_sysram->size) {
		dev_err(dev, "Invalid size: 0x%x\n", size);
		ret = -EINVAL;
		goto err_after_lock;
	}

	if (addr + size > mm_sysram->size) {
		dev_err(dev, "Region out of range: 0x%x - 0x%x\n", addr,
							addr + size - 1);
		ret = -EINVAL;
		goto err_after_lock;
	}

#ifdef CONFIG_COMMON_CLK_MT3612
	/** Power on SYSRAM */
	pm_runtime_get_sync(dev);

	ret = mtk_mm_sysram_enable_clk(dev);

	if (ret)
		goto err_after_pm_get;
#endif

	/** Local power control */
	ret = pwr_ctrl_require(&mm_sysram->pwr_ctrl, addr, size);
	if (ret)
		goto err_after_pm_get;

	/* Misc setting */
	if (mm_sysram->dev_id == 2) {
		/** Setup CMDFF R/W weight */
		/** -- Read command first mode */
		writel(0x3, mm_sysram->base + SYS2_CMDFF_CTRL);
		/** -- M1(RDMA0/1), M2(RDMA2/3) read:write -> 1:1 */
		writel(0x01010101, mm_sysram->base + SYS2_CMDFF_PARAM);
		/** -- M3(GPU0), M4(GPU1) read:write -> 3:1 */
		writel(0x03010301, mm_sysram->base + SYS2_CMDFF_PARAM_2);
		/** -- M5(WDMA/Infra), M6(VPU0/1, FM) read:write -> 1:1 */
		writel(0x01010101, mm_sysram->base + SYS2_CMDFF_PARAM_3);

		/** Setup Master weight */
		writel(SYSRAM2_WEIGHT(0),
				mm_sysram->base + SYS2_SRAM_WEIGHTED_M1);
		writel(SYSRAM2_WEIGHT(0),
				mm_sysram->base + SYS2_SRAM_WEIGHTED_M2);
		writel(SYSRAM2_WEIGHT(8),
				mm_sysram->base + SYS2_SRAM_WEIGHTED_M3);
		writel(SYSRAM2_WEIGHT(8),
				mm_sysram->base + SYS2_SRAM_WEIGHTED_M4);
		writel(SYSRAM2_WEIGHT(0),
				mm_sysram->base + SYS2_SRAM_WEIGHTED_M5);
		writel(SYSRAM2_WEIGHT(0),
				mm_sysram->base + SYS2_SRAM_WEIGHTED_M6);
		writel(SYSRAM2_WEIGHT(0),
				mm_sysram->base + SYS2_SRAM_WEIGHTED_M7);
		writel(SYSRAM2_WEIGHT(0),
				mm_sysram->base + SYS2_SRAM_WEIGHTED_M8);

		/** Setup SMI_COMMON */
		for (i = 0; i < mm_sysram->smi_num; i++) {
			if (mm_sysram->smi_bases[i]) {
				tmp = readl(mm_sysram->smi_bases[i] +
							SMI_READ_FIFO_TH);
				tmp |= STD_AXI_MODE;
				writel(tmp, mm_sysram->smi_bases[i] +
							SMI_READ_FIFO_TH);
			}
		}
	}

	/** Unlock */
	mutex_unlock(&mm_sysram->lock);

	return 0;

err_after_pm_get:
#ifdef CONFIG_COMMON_CLK_MT3612
	pm_runtime_put_sync(dev);
#endif

err_after_lock:
	mutex_unlock(&mm_sysram->lock);

	return ret;
}
EXPORT_SYMBOL(mtk_mm_sysram_power_on);

/**
 * @ingroup IP_group_mm_sysram_external_function
 * @par Description
 *	Disable mm_sysram clock and power off SYSRAM.
 * @param[in] dev: mm_sysram device node.
 * @param[in] addr: start address (0 ~ (size described in the device tree))
 * @param[in] size: requested size (1 ~ (size described in the device tree))
 * @return
 *	0, configuration successful.\n
 *	-EINVAL, invalid argument.\n
 *	-ENODEV, no such device.
 * @par Boundary case and Limitation
 *	(addr + size) must not greater than sysram size
 *      {addr, size} pair must be powered on using mtk_mm_sysram_power_on()
 * @par Error case and Error handling
 *	If dev is NULL, return -EINVAL.
 *      If {addr, size} pair is invalid, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mm_sysram_power_off(struct device *dev, const int addr, const int size)
{
	struct mm_sysram_s *mm_sysram;
	int ret;

	/** Sanity check - Pass 1 */
	if (!dev)
		return -EINVAL;

	/** Get driver data and lock */
	mm_sysram = dev_get_drvdata(dev);

	if (!mm_sysram)
		return -ENODEV;

	mutex_lock(&mm_sysram->lock);

	/** Sanity check - Pass 2 */
	if (addr < 0 || addr > mm_sysram->size) {
		dev_err(dev, "Start address is out of range: 0x%x\n", addr);
		ret = -EINVAL;
		goto err_after_lock;
	}

	if (size <= 0 || size > mm_sysram->size) {
		dev_err(dev, "Invalid size: 0x%x\n", size);
		ret = -EINVAL;
		goto err_after_lock;
	}

	if (addr + size > mm_sysram->size) {
		dev_err(dev, "Region out of range: 0x%x\n", addr + size);
		ret = -EINVAL;
		goto err_after_lock;
	}

	/** TODO: Local power control */
	ret = pwr_ctrl_release(&mm_sysram->pwr_ctrl, addr, size);
	if (ret)
		goto err_after_lock;

#ifdef CONFIG_COMMON_CLK_MT3612
	/* Power off SYSRAM */
	mtk_mm_sysram_disable_clk(dev);
	pm_runtime_put_sync(dev);
#endif

	/* Unlock */
	mutex_unlock(&mm_sysram->lock);

	return 0;

err_after_lock:
	mutex_unlock(&mm_sysram->lock);

	return ret;
}
EXPORT_SYMBOL(mtk_mm_sysram_power_off);

/**
 * @ingroup IP_group_mm_sysram_external_function
 * @par Description
 *	Fill a range of mm_sysram with a specific byte value.
 * @param[in] dev: mm_sysram device node.
 * @param[in] addr: the mm_sysram base address to fill to. (0 ~ (size described
 *                  in the device tree))
 * @param[in] len: the length for fill. (0x10 ~ ((size described in the device
 *                 tree) - addr))
 * @param[in] data: the data for fill. (0 ~ 255)
 * @return
 *	If successful, return the number of data filled.\n
 *	-EINVAL, invalid argument.\n
 *	-ENODEV, no such device.\n
 *	-EBUSY, device is busy.
 * @par Boundary case and Limitation
 *	The base address and length must be not out of the mm_sysram range.
 *	The base address and length must align to 16 bytes.
 * @par Error case and Error handling
 *	If dev is NULL or does not exist, then return -ENODEV.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
ssize_t mtk_mm_sysram_fill(const struct device *dev, const u32 addr,
				const size_t len, const u8 data)
{
	struct mm_sysram_s *mm_sysram;
	u32 len_fixed, tmp;
	int ret;

	/** Sanity check - Pass 1 */
	if (!dev)
		return -EINVAL;

	if (!IS_ALIGNED(addr, MM_SYSRAM_ALIGNMENT)) {
		dev_err(dev, "Address is not align: 0x%x\n", addr);
		return -EINVAL;
	}

	if (!IS_ALIGNED(len, MM_SYSRAM_ALIGNMENT)) {
		dev_err(dev, "Length is not align: 0x%x\n", (u32)len);
		return -EINVAL;
	}

	if (len == 0) {
		dev_err(dev, "Length can't be zero\n");
		return -EINVAL;
	}

	/** Get driver data and lock */
	mm_sysram = dev_get_drvdata(dev);

	if (!mm_sysram) {
		dev_err(dev, "Get driver data failed\n");
		return -ENODEV;
	}

	mutex_lock(&mm_sysram->lock);

	/** Sanity check - Pass 2 */
	if (addr + len > mm_sysram->size) {
		dev_err(dev, "Exceed the size of sram\n");
		ret = -EINVAL;
		goto err_after_lock;
	}

	len_fixed = len / MM_SYSRAM_ALIGNMENT - 1;

	/** Write 1 to clear the status of fw_fill_sram_done */
	writel(FW_FILL_SRAM_DONE_CLR,
		mm_sysram->base + SYS0_FW_ACTION_TRIG);

	/** Wait for fw_fill_sram_done clear */
	if (wait_bit_down(mm_sysram->base + SYS0_FW_ACCESS_SRAM_DONE,
			FW_FILL_SRAM_DONE)) {
		ret = -EBUSY;
		goto err_after_lock;
	}

	/** Set fw_fill_sram_mode to 0 */
	tmp = readl(mm_sysram->base + SYS0_COMMON_CTRL_SET);
	tmp &= ~FW_FILL_SRAM_MODE;
	writel(tmp, mm_sysram->base + SYS0_COMMON_CTRL_SET);

	/** Set up the parameters */
	writel(addr, mm_sysram->base + SYS0_FW_FILL_SRAM_ADDR);
	writel(len_fixed, mm_sysram->base + SYS0_FW_FILL_SRAM_LEN);
	writel(data, mm_sysram->base + SYS0_FW_FILL_SRAM_DAT);

	/** Write 1 to trigger this function */
	writel(FW_FILL_SRAM_TRIGGER,
		mm_sysram->base + SYS0_FW_ACTION_TRIG);

	/** Wait for fw_fill_sram_done set */
	if (wait_bit_up(mm_sysram->base + SYS0_FW_ACCESS_SRAM_DONE,
			FW_FILL_SRAM_DONE)) {
		ret = -EBUSY;
		goto err_after_lock;
	}

	/** Write 1 to clear the status of fw_fill_sram_done */
	writel(FW_FILL_SRAM_DONE_CLR,
		mm_sysram->base + SYS0_FW_ACTION_TRIG);

	/** Unlock */
	mutex_unlock(&mm_sysram->lock);

	return len;

err_after_lock:
	mutex_unlock(&mm_sysram->lock);

	return ret;

}
EXPORT_SYMBOL(mtk_mm_sysram_fill);

/**
 * @ingroup IP_group_mm_sysram_external_function
 * @par Description
 *      Fill a range of mm_sysram with specific 16-bytes value.
 * @param[in] dev: mm_sysram device node.
 * @param[in] addr: the mm_sysram base address to fill to. (0 ~((size described
 *                  in the device tree) - 0x10))
 * @param[in] len: the length for fill. (0x10 ~ ((size described in the device
 *                 tree) - addr))
 * @param[in] buf: the pointer of the input data buffer.
 * @return
 *      If successful, return the number of data filled.\n
 *      -EINVAL, invalid argument.\n
 *      -ENODEV, no such device.\n
 *      -EBUSY, device is busy.
 * @par Boundary case and Limitation
 *      The base address and length must be not out of the mm_sysram range.
 *      The base address and length must align to 16 bytes.
 *      The data buffer must not be NULL
 *      The size of data buffer must greater than 16 bytes.
 * @par Error case and Error handling
 *      If dev is NULL or does not exist, then return -ENODEV.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
ssize_t mtk_mm_sysram_fill_128b(const struct device *dev, const u32 addr,
				const size_t len, const u32 *buf)
{
	struct mm_sysram_s *mm_sysram;
	u32 len_fixed, tmp;
	int ret;

	/** Sanity check - Pass 1 */
	if (!dev)
		return -EINVAL;

	if (!IS_ALIGNED(addr, MM_SYSRAM_ALIGNMENT)) {
		dev_err(dev, "Address is not align: 0x%x\n", addr);
		return -EINVAL;
	}

	if (!IS_ALIGNED(len, MM_SYSRAM_ALIGNMENT)) {
		dev_err(dev, "Length is not align: 0x%x\n", (u32)len);
		return -EINVAL;
	}

	if (buf == NULL) {
		dev_err(dev, "The buffer pointer cannot be NULL\n");
		return -EINVAL;
	}

	if (len == 0) {
		dev_err(dev, "Length can't be zero\n");
		return -EINVAL;
	}

	/** Get driver data and lock */
	mm_sysram = dev_get_drvdata(dev);

	if (!mm_sysram) {
		dev_err(dev, "Get driver data failed\n");
		return -ENODEV;
	}

	mutex_lock(&mm_sysram->lock);

	/** Sanity check - Pass 2 */
	if (addr + len > mm_sysram->size) {
		dev_err(dev, "Exceed the size of sram\n");
		ret = -EINVAL;
		goto err_after_lock;
	}

	/** Get length */
	len_fixed = len / MM_SYSRAM_ALIGNMENT - 1;

	/** Write 1 to clear the status of fw_fill_sram_done */
	writel(FW_FILL_SRAM_DONE_CLR,
		mm_sysram->base + SYS0_FW_ACTION_TRIG);

	/** Wait for fw_fill_sram_done clear */
	if (wait_bit_down(mm_sysram->base + SYS0_FW_ACCESS_SRAM_DONE,
			FW_FILL_SRAM_DONE)) {
		ret = -EBUSY;
		goto err_after_lock;
	}

	/** Set fw_fill_sram_mode to 1 */
	tmp = readl(mm_sysram->base + SYS0_COMMON_CTRL_SET);
	tmp |= FW_FILL_SRAM_MODE;
	writel(tmp, mm_sysram->base + SYS0_COMMON_CTRL_SET);

	/** Set up the parameters */
	writel(addr, mm_sysram->base + SYS0_FW_FILL_SRAM_ADDR);
	writel(len_fixed, mm_sysram->base + SYS0_FW_FILL_SRAM_LEN);
	writel(buf[0], mm_sysram->base + SYS0_FW_FILL_DATA_128B_0);
	writel(buf[1], mm_sysram->base + SYS0_FW_FILL_DATA_128B_1);
	writel(buf[2], mm_sysram->base + SYS0_FW_FILL_DATA_128B_2);
	writel(buf[3], mm_sysram->base + SYS0_FW_FILL_DATA_128B_3);

	/** Write 1 to trigger this function */
	writel(FW_FILL_SRAM_TRIGGER,
		mm_sysram->base + SYS0_FW_ACTION_TRIG);

	/** Wait for fw_fill_sram_done set */
	if (wait_bit_up(mm_sysram->base + SYS0_FW_ACCESS_SRAM_DONE,
			FW_FILL_SRAM_DONE)) {
		ret = -EBUSY;
		goto err_after_lock;
	}

	/** Write 1 to clear the status of fw_fill_sram_done */
	writel(FW_FILL_SRAM_DONE_CLR,
		mm_sysram->base + SYS0_FW_ACTION_TRIG);

	/** Unlock */
	mutex_unlock(&mm_sysram->lock);

	return len;

err_after_lock:
	mutex_unlock(&mm_sysram->lock);

	return ret;
}
EXPORT_SYMBOL(mtk_mm_sysram_fill_128b);

/**
 * @ingroup IP_group_mm_sysram_external_function
 * @par Description
 *      Read 16-bytes value from sysram.
 * @param[in] dev: mm_sysram device node.
 * @param[in] addr: the mm_sysram base address to read from. (0 ~ ((size
 *                  described in the device tree) - 0x10))
 * @param[out] buf: the pointer of the output data buffer.
 * @return
 *      If successful, return 0.\n
 *      -EINVAL, invalid argument.\n
 *      -ENODEV, no such device.\n
 *      -EBUSY, device is busy.
 * @par Boundary case and Limitation
 *      The base address must be not out of the mm_sysram range.
 *      The base address must align to 16-bytes.
 *      The data buffer must not be NULL
 *      The size of data buffer must greater than 16 bytes.
 * @par Error case and Error handling
 *      If dev is NULL or does not exist, then return -ENODEV.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mm_sysram_read(const struct device *dev, const u32 addr, u32 *buf)
{
	struct mm_sysram_s *mm_sysram;
	int ret;

	/** Sanity check - Pass 1 */
	if (!dev)
		return -EINVAL;

	if (!IS_ALIGNED(addr, MM_SYSRAM_ALIGNMENT)) {
		dev_err(dev, "Address is not align: 0x%x\n", addr);
		return -EINVAL;
	}

	if (buf == NULL) {
		dev_err(dev, "The buffer pointer cannot be NULL\n");
		return -EINVAL;
	}

	/** Get driver data and lock */
	mm_sysram = dev_get_drvdata(dev);

	if (!mm_sysram) {
		dev_err(dev, "Get driver data failed\n");
		return -ENODEV;
	}

	mutex_lock(&mm_sysram->lock);

	/** Sanity check - Pass 2 */
	if (addr > (mm_sysram->size - 0x10)) {
		dev_err(dev, "Address must less than sysram size - 0x10\n");
		ret = -EINVAL;
		goto err_after_lock;
	}

	/** Write 1 to clear the status of fw_read_sram_done */
	writel(FW_RD_SRAM_DONE_CLR,
		mm_sysram->base + SYS0_FW_ACTION_TRIG);

	/** Wait for fw_rd_sram_done clear */
	if (wait_bit_down(mm_sysram->base + SYS0_FW_ACCESS_SRAM_DONE,
			FW_RD_SRAM_DONE)) {
		ret = -EBUSY;
		goto err_after_lock;
	}

	/** Set up the parameters */
	writel(addr, mm_sysram->base + SYS0_FW_RD_SRAM_ADDR);

	/** Write 1 to trigger this function */
	writel(FW_RD_SRAM_TRIGGER,
		mm_sysram->base + SYS0_FW_ACTION_TRIG);

	/** Wait for fw_rd_sram_done set */
	if (wait_bit_up(mm_sysram->base + SYS0_FW_ACCESS_SRAM_DONE,
			FW_RD_SRAM_DONE)) {
		ret = -EBUSY;
		goto err_after_lock;
	}

	/** Write 1 to clear the status of fw_rd_sram_done */
	writel(FW_RD_SRAM_DONE_CLR,
		mm_sysram->base + SYS0_FW_ACTION_TRIG);

	/** Copy the data to the buffer */
	buf[0] = readl(mm_sysram->base + SYS0_FW_RD_SRAM_DAT_1);
	buf[1] = readl(mm_sysram->base + SYS0_FW_RD_SRAM_DAT_2);
	buf[2] = readl(mm_sysram->base + SYS0_FW_RD_SRAM_DAT_3);
	buf[3] = readl(mm_sysram->base + SYS0_FW_RD_SRAM_DAT_4);

	/** Unlock */
	mutex_unlock(&mm_sysram->lock);

	return 0;

err_after_lock:
	mutex_unlock(&mm_sysram->lock);

	return ret;
}
EXPORT_SYMBOL(mtk_mm_sysram_read);

/**
 * @ingroup IP_group_mm_sysram_internal_function
 * @par Description
 *	Probe mm_sysram according to device tree.
 * @param[in] pdev: platform device node.
 * @return
 *	0, if successful.\n
 *	Otherwise, mm_sysram probe failed.
 * @par Boundary case and Limitation
 *	none.
 * @par Error case and Error handling
 *	If there is any error during probe flow,\n
 *	system error number will be returned.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_mm_sysram_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mm_sysram_s *mm_sysram;
	struct resource *regs;
	struct device_node *dev_node;
#ifdef CONFIG_COMMON_CLK_MT3612
	char buf[16];
#endif
	int ret;
	int i;

	mm_sysram = devm_kzalloc(dev, sizeof(*mm_sysram), GFP_KERNEL);
	if (!mm_sysram)
		return -ENOMEM;

	platform_set_drvdata(pdev, mm_sysram);

	/** Get sysram ID & Size */
	of_property_read_u32(dev->of_node, "dev-id", &mm_sysram->dev_id);
	of_property_read_u32(dev->of_node, "size", &mm_sysram->size);

	dev_info(dev, "Init mm_sysram%d, size = %d (0x0 ~ 0x%x)",
						mm_sysram->dev_id,
						mm_sysram->size,
						mm_sysram->size - 1);

	/** Mapping register */
	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!regs) {
		dev_err(dev, "Failed to get mm_sysram IORESOURCE_MEM\n");
		ret = -ENODEV;
		goto err_after_alloc_mm_sysram;
	}

	mm_sysram->base = devm_ioremap_resource(dev, regs);
	if (!mm_sysram->base) {
		dev_err(dev, "Failed to get mm_sysram register mapping\n");
		ret = PTR_ERR(mm_sysram->base);
		goto err_after_alloc_mm_sysram;
	}

#ifdef CONFIG_COMMON_CLK_MT3612
	/** Enable power-doman */
	pm_runtime_enable(dev);

	/** Get clock information */
	scnprintf(buf, sizeof(buf), "mm_sysram%d_cg", mm_sysram->dev_id);
	if (of_property_match_string(dev->of_node, "clock-names", buf) >= 0) {
		mm_sysram->clk_gate = devm_clk_get(dev, buf);
		if (IS_ERR(mm_sysram->clk_gate)) {
			dev_err(dev, "Failed to get mm_sysram clk_gate\n");
			ret = PTR_ERR(mm_sysram->clk_gate);
			goto err_after_alloc_mm_sysram;
		}
	} else {
		dev_warn(dev, "Cannot find mm_sysram clk_gate\n");
		mm_sysram->clk_gate = NULL;
	}

	scnprintf(buf, sizeof(buf), "mm_sysram%d_sel", mm_sysram->dev_id);
	if (of_property_match_string(dev->of_node, "clock-names", buf) >= 0) {
		mm_sysram->clk_sel = devm_clk_get(dev, buf);
		if (IS_ERR(mm_sysram->clk_sel)) {
			dev_err(dev, "Failed to get mm_sysram clk_sel\n");
			ret = PTR_ERR(mm_sysram->clk_sel);
			goto err_after_alloc_mm_sysram;
		}
	} else {
		dev_err(dev, "Cannot find mm_sysram clk_sel\n");
		ret = -EINVAL;
		goto err_after_alloc_mm_sysram;
	}

	if (!of_property_read_string(dev->of_node, "clock-source-name",
				(const char **)&mm_sysram->clk_src_name)) {
		mm_sysram->clk_src = devm_clk_get(dev,
				mm_sysram->clk_src_name);
	} else {
		mm_sysram->clk_src = NULL;
	}

	if (!mm_sysram->clk_src || IS_ERR(mm_sysram->clk_src)) {
		dev_warn(dev, "Cannot find mm_sysram clock source.\n");
		mm_sysram->clk_src = devm_clk_get(dev, "clk26m");
		if (IS_ERR(mm_sysram->clk_src)) {
			dev_err(dev, "Failed to get default clock source.\n");
			ret = -EINVAL;
			goto err_after_alloc_mm_sysram;
		} else {
			dev_warn(dev, "Using Xtal as default clock source.\n");
			mm_sysram->clk_src_name = "clk26m";
		}
	}

	ret = clk_set_parent(mm_sysram->clk_sel, mm_sysram->clk_src);
	if (ret) {
		dev_err(dev, "Failed to set clock source\n");
		goto err_after_alloc_mm_sysram;
	}

	mm_sysram->clk_rate = clk_get_rate(mm_sysram->clk_src);

#endif /* #ifdef CONFIG_COMMON_CLK_MT3612 */

	/** Get SMI_COMMON information */
	ret = of_property_count_elems_of_size(dev->of_node, "mediatek,smi",
							sizeof(phandle));

	if (ret > 0) {
		dev_info(dev, "Find SMI_COMMON information, num = %d\n", ret);

		mm_sysram->smi_num = ret;
		mm_sysram->smi_bases = devm_kzalloc(dev,
					sizeof(*mm_sysram->smi_bases) * ret,
					GFP_KERNEL);

		if (!mm_sysram->smi_bases) {
			dev_err(dev, "Fail to allocate memory for SMI\n");
			ret = -ENOMEM;
			goto err_after_alloc_mm_sysram;
		}

		for (i = 0; i < mm_sysram->smi_num; i++) {
			/** Get SMI_COMMON node */
			dev_node = of_parse_phandle(dev->of_node,
						"mediatek,smi", i);

			if (!dev_node) {
				dev_err(dev, "Failed to get SMI node #%d\n",
									i);
				ret = -ENODEV;
				goto err_after_alloc_smi;
			}

			/** Check the status of the SMI_COMMON */
			if (!of_device_is_available(dev_node)) {
				dev_info(dev, "SMI #%d is disabled\n", i);
				mm_sysram->smi_bases[i] = NULL;
				continue;
			}

			/** Map SMI_COMMON register */
			mm_sysram->smi_bases[i] = of_iomap(dev_node, 0);
			of_node_put(dev_node);

			if (!mm_sysram->smi_bases[i]) {
				dev_err(dev, "Failed to map SMI #%d\n", i);
				ret = -ENODEV;
				goto err_after_alloc_smi;
			}
		}

	} else {
		mm_sysram->smi_num = 0;
		mm_sysram->smi_bases = NULL;
	}

	/** MISC */
	mm_sysram->enable = 0;
	mutex_init(&mm_sysram->lock);

	/** Initialize inidividual part of mm_sysram */
	ret = mtk_mm_sysram_init_individual(dev);
	if (ret)
		goto err_after_alloc_smi;

	return 0;

err_after_alloc_smi:
	if (mm_sysram->smi_bases)
		devm_kfree(dev, mm_sysram->smi_bases);

err_after_alloc_mm_sysram:
	devm_kfree(dev, mm_sysram);

	return ret;
}

/**
 * @ingroup IP_group_mm_sysram_internal_function
 * @par Description
 *	Release memory.
 * @param[in] pdev: platform device node.
 * @return
 *	0, if successful.\n.
 *	-ENODEV, no such device.\n
 *	-EBUSY, if this mm_sysram needed by another device.
 * @par Boundary case and Limitation
 *	none.
 * @par Error case and Error handling
 *	none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_mm_sysram_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mm_sysram_s *mm_sysram;
	int ret;

	if (!dev)
		return -ENODEV;

	mm_sysram = dev_get_drvdata(dev);

	if (!mm_sysram)
		return 0;

	if (mm_sysram->enable)
		return -EBUSY;

	ret = pwr_ctrl_exit(&mm_sysram->pwr_ctrl);
	if (ret)
		return ret;

	if (mm_sysram->smi_bases)
		devm_kfree(dev, mm_sysram->smi_bases);

#ifdef CONFIG_COMMON_CLK_MT3612
	pm_runtime_disable(dev);
#endif

	devm_kfree(dev, mm_sysram);

	return 0;
}

/**
 * @ingroup IP_group_mm_sysram_internal_struct
 * @brief Open Framework Device ID for MM_SYSRAM.\n
 *	This structure is used to attach specific names to\n
 *	platform device for use with device tree.
 */
static const struct of_device_id of_match_table_mm_sysram[] = {
	{ .compatible = "mediatek,mt3612-mm_sysram" },
	{},
};
MODULE_DEVICE_TABLE(of, of_match_table_mm_sysram);

/**
 * @ingroup type_group_mm_sysram_struct
 * @brief MM_SYSRAM platform driver structure.\n
 *	This structure is used to register itself.
 */
struct platform_driver platform_driver_mm_sysram = {
	.probe = mtk_mm_sysram_probe,
	.remove = mtk_mm_sysram_remove,
	.driver = {
		.name = "mediatek-mm-sysram",
		.owner = THIS_MODULE,
		.of_match_table = of_match_table_mm_sysram,
	},
};

module_platform_driver(platform_driver_mm_sysram);

MODULE_AUTHOR("Chun-Chi Lan");
MODULE_DESCRIPTION("MediaTek MM_SYSRAM Driver");
MODULE_LICENSE("GPL");
