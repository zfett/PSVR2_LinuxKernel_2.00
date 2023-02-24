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

#include "mtk_mfg_utils.h"

/** @ingroup IP_group_mfg_internal_function
 * @par Description
 *     mfg register mask write common function.
 * @param[in]
 *     regs: register base.
 * @param[in]
 *     offset: register offset.
 * @param[in]
 *     value: write data value.
 * @param[in]
 *     mask: write mask value.
 * @return
 *     0, write successfully.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int reg_write_mask(void __iomem *regs, u32 offset, u32 value, u32 mask)
{
	u32 reg;

	reg = readl(regs + offset);
	reg &= ~mask;
	reg |= value;
	writel(reg, regs + offset);

	return 0;
}

/** @ingroup IP_group_mfg_internal_function
 * @par Description
 *     mfg register mask read common function.
 * @param[in]
 *     regs: register base.
 * @param[in]
 *     offset: register offset.
 * @param[in]
 *     mask: read mask value.
 * @return
 *     reg value.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int reg_read_mask(void __iomem *regs, u32 offset, u32 mask)
{
	u32 reg;

	reg = readl(regs + offset);
	reg &= mask;

	return reg;
}

