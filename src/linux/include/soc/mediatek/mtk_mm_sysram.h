/*
 * Copyright (C) 2017 MediaTek Inc.
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

#ifndef _MTK_MM_SYSRAM_H_
#define _MTK_MM_SYSRAM_H_

#include <linux/types.h>
#include <linux/device.h>

int mtk_mm_sysram_power_on(struct device *dev, const int addr, const int size);
int mtk_mm_sysram_power_off(struct device *dev, const int addr, const int size);
ssize_t mtk_mm_sysram_fill(const struct device *dev, const u32 addr,
				const size_t len, const u8 data);
ssize_t mtk_mm_sysram_fill_128b(const struct device *dev, const u32 addr,
				const size_t len, const u32 *buf);
int mtk_mm_sysram_read(const struct device *dev, const u32 addr, u32 *buf);

#endif /* #ifndef _MTK_MM_SYSRAM_H_ */
