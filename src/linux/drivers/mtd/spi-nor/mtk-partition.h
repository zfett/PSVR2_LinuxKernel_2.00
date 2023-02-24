/*
 * Copyright (C) 2016  MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifndef __DRIVERS_MTD_NAND_MTK_PARTITION_H__
#define __DRIVERS_MTD_NAND_MTK_PARTITION_H__
#include <linux/mtd/mtd.h>

int mtk_partition_register(struct mtd_info *mtd);

#endif
