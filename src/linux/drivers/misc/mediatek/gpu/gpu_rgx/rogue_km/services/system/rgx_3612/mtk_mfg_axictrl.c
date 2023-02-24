/*
 * Copyright (c) 2019 MediaTek Inc.
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

#include <linux/of_irq.h>
#include "mtk_mfg_axictrl.h"
#include "mtk_mfg_utils.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>

#define CH0_ONLY 0
#define CH1_ONLY 1

/** @ingroup IP_group_MFG_internal_function
 * @par Description
 *     set axi control to EMI port.
 * @param[in]
 *     dev_config: PVR device configs.
 * @param[in]
 *     mode: AXI control mode.
 * @return
 *     0, write successfully.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mtk_mfg_set_aximode(PVRSRV_DEVICE_CONFIG *dev_config, int mode)
{
	struct platform_device *pdev;
	void __iomem *mfg_base;

	pdev = to_platform_device((struct device *)dev_config->pvOSDevice);
	mfg_base = of_iomap(pdev->dev.of_node, 2);

	if (mfg_base == NULL) {
		PVR_LOG(("Map mfg_base fail!"));
		return;
	}

	if (mode == CH0_ONLY) {
		reg_write_mask(mfg_base, MFG_1TO2AXI_CON_00,
			0x200, 0xFFFFFFFF);
		reg_write_mask(mfg_base, MFG_1TO2AXI_CON_01,
			0x0, 0xFF);
		reg_write_mask(mfg_base, MFG_1TO2AXI_CON_02,
			0x200, 0xFFFFFFFF);
		reg_write_mask(mfg_base, MFG_1TO2AXI_CON_03,
			0x0, 0xFF);
		PVR_LOG(("AXI use ch 0 mode"));
	} else if (mode == CH1_ONLY) {
		reg_write_mask(mfg_base, MFG_1TO2AXI_CON_00,
			0x200, 0xFFFFFFFF);
		reg_write_mask(mfg_base, MFG_1TO2AXI_CON_01,
			0xFF, 0xFF);
		reg_write_mask(mfg_base, MFG_1TO2AXI_CON_02,
			0x200, 0xFFFFFFFF);
		reg_write_mask(mfg_base, MFG_1TO2AXI_CON_03,
			0xFF, 0xFF);
		PVR_LOG(("AXI use ch 1 mode"));
	} else {
		reg_write_mask(mfg_base, MFG_1TO2AXI_CON_00,
			0x0, 0xFFFFFFFF);
		reg_write_mask(mfg_base, MFG_1TO2AXI_CON_01,
			0xAA, 0xFF);
		reg_write_mask(mfg_base, MFG_1TO2AXI_CON_02,
			0x0, 0xFFFFFFFF);
		reg_write_mask(mfg_base, MFG_1TO2AXI_CON_03,
			0xAA, 0xFF);
		PVR_LOG(("AXI use 2 ch mode"));
	}

	iounmap(mfg_base);
}

