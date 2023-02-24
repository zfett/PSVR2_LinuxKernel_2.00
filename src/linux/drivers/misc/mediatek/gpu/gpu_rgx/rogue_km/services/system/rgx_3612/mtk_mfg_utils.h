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

#ifndef MTK_MFG_UTILS_H
#define MTK_MFG_UTILS_H

#include "mtk_mfg_reg.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>

#define reg_write(base, offset, value) writel(value, base + offset)

int reg_write_mask(void __iomem *regs, u32 offset, u32 value, u32 mask);
int reg_read_mask(void __iomem *regs, u32 offset, u32 mask);

#endif /* MTK_MFG_UTILS_H */
