/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/
#ifndef MTK_CAMMEM_H_
#define MTK_CAMMEM_H_

#include <linux/ioctl.h>

#define CAMMEM_MAGIC 'y'

enum IRQTEST_CMD_ENUM {
	ISP_CMD_RSVD_ALLOC,
	ISP_CMD_RSVD_DEALLOC,
	ISP_CMD_ION_MAPPHY,
	ISP_CMD_ION_UNMAPPHY,
	ISP_CMD_ION_MAP_KERVRT,
	ISP_CMD_ION_UNMAP_KERVRT,
	ISP_CMD_ION_CACHE_FLUSH,
	ISP_CMD_ION_CACHE_INVALID,
	ISP_CMD_ION_SRAM_PHY_ADDR,
};

#define NAME_MAX_LENGTH (64)

struct ISP_MEMORY_STRUCT {
	char name[NAME_MAX_LENGTH];
	unsigned int size;
	int *UserVirtAddr;
	int *KerVirtAddr;
	int *phyAddr;
	int memID;
	int key;
};

#define ISP_ION_MAPPHY                                                         \
	_IOWR(CAMMEM_MAGIC, ISP_CMD_ION_MAPPHY, struct ISP_MEMORY_STRUCT)
#define ISP_ION_UNMAPPHY                                                       \
	_IOWR(CAMMEM_MAGIC, ISP_CMD_ION_UNMAPPHY, struct ISP_MEMORY_STRUCT)
#define ISP_RSVD_ALLOC                                                         \
	_IOWR(CAMMEM_MAGIC, ISP_CMD_RSVD_ALLOC, struct ISP_MEMORY_STRUCT)
#define ISP_RSVD_DEALLOC                                                       \
	_IOWR(CAMMEM_MAGIC, ISP_CMD_RSVD_DEALLOC, struct ISP_MEMORY_STRUCT)
#define ISP_ION_MAP_KERVRT                                                    \
	_IOWR(CAMMEM_MAGIC, ISP_CMD_ION_MAP_KERVRT, struct ISP_MEMORY_STRUCT)
#define ISP_ION_UNMAP_KERVRT                                               \
	_IOW(CAMMEM_MAGIC, ISP_CMD_ION_UNMAP_KERVRT, struct ISP_MEMORY_STRUCT)
#define ISP_ION_CACHE_FLUSH		\
	_IOW(CAMMEM_MAGIC, ISP_CMD_ION_CACHE_FLUSH, struct ISP_MEMORY_STRUCT)
#define ISP_ION_CACHE_INVALID	\
	_IOW(CAMMEM_MAGIC, ISP_CMD_ION_CACHE_INVALID, struct ISP_MEMORY_STRUCT)
#define ISP_ION_SRAM_PHY_ADDR \
	_IOWR(CAMMEM_MAGIC, ISP_CMD_ION_SRAM_PHY_ADDR, struct ISP_MEMORY_STRUCT)

#endif
