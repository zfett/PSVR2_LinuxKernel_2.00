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

#include <linux/mtd/partitions.h>
#include "mtk-partition.h"

static struct mtd_partition g_exist_Partition_64M[] = {
	{"BOOTLOADER",	0x00040000, 0x0, 0x0, NULL},
	{"LOADER",		0x00040000, 0x40000, 0x0, NULL},
	{"RECOVERY_LOADER",	0x00040000, 0x80000, 0x0, NULL},
	{"FACTORY_0",		0x00040000, 0xa0000, 0x0, NULL},
	{"FACTORY_1",		0x00040000, 0x100000, 0x0, NULL},
	{"FACTORY_2",		0x00100000, 0x140000, 0x0, NULL},
	{"FACTORY_3",		0x00040000, 0x240000, 0x0, NULL},
	{"RECOVERY_SCP",	0x00040000, 0x280000, 0x0, NULL},
	{"RECOVERY_ATF",	0x00040000, 0x2a0000, 0x0, NULL},
	{"RECOVERY_TSP",	0x00040000, 0x300000, 0x0, NULL},
	{"RECOVERY_LINUX",	0x01C00000, 0x340000, 0x0, NULL},
	{"SCP",			0x00040000, 0x1F40000, 0x0, NULL},
	{"ATF",			0x00040000, 0x1F80000, 0x0, NULL},
	{"TSP",			0x00040000, 0x1FA0000, 0x0, NULL},
	{"LINUX",		0x01F00000, 0x2000000, 0x0, NULL},
	{"FTS",			0x00100000, 0x3F00000, 0x0, NULL},
};

static struct mtd_partition g_exist_Partition_32M[] = {
	{"BOOTLOADER",		0x00030000, 0x0, 0x0, NULL},
	{"LOADER",		0x00050000, 0x30000, 0x0, NULL},
	{"RECOVERY_LOADER",	0x00050000, 0x80000, 0x0, NULL},
	{"FACTORY_0",		0x00010000, 0xD0000, 0x0, NULL},
	{"FACTORY_1",		0x00010000, 0xE0000, 0x0, NULL},
	{"FACTORY_2",		0x00100000, 0xF0000, 0x0, NULL},
	{"FACTORY_3",		0x00010000, 0x1F0000, 0x0, NULL},
	{"RECOVERY_SCP",	0x00030000, 0x200000, 0x0, NULL},
	{"RECOVERY_ATF",	0x00020000, 0x230000, 0x0, NULL},
	{"RECOVERY_TSP",	0x00020000, 0x250000, 0x0, NULL},
	{"RECOVERY_LINUX",	0x00E00000, 0x270000, 0x0, NULL},
	{"SCP",			0x00030000, 0x1070000, 0x0, NULL},
	{"ATF",			0x00020000, 0x10a0000, 0x0, NULL},
	{"TSP",			0x00020000, 0x10c0000, 0x0, NULL},
	{"LINUX",		0x00E00000, 0x10e0000, 0x0, NULL},
	{"FTS",			0x00100000, 0x1ee0000, 0x0, NULL},
};

int mtk_partition_register(struct mtd_info *mtd)
{
	int part_num, ret;

	if (mtd->size == 0x4000000) {
		part_num = ARRAY_SIZE(g_exist_Partition_64M);
		ret = mtd_device_register(mtd, g_exist_Partition_64M, part_num);
	} else if (mtd->size == 0x2000000) {
		part_num = ARRAY_SIZE(g_exist_Partition_32M);
		ret = mtd_device_register(mtd, g_exist_Partition_32M, part_num);
	} else {
		ret = mtd_device_register(mtd, NULL, 0);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_partition_register);

