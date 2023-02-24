/* SIE CONFIDENTIAL
 *
 *  Copyright (C) 2019 Sony Interactive Entertainment Inc.
 *                     All Rights Reserved.
 *
 **/
#pragma once

#define MAX_DEBUGFS_BUFFER_SIZE (1024)
#define MAX_HW_NUM (4)

struct devfsinfo {
	const char* compatible;
	int idx;
	const char* dev_name;
	struct file_operations* fops;
	u8    hw_num;
	void __iomem *regs[MAX_HW_NUM];
	u32 phys[MAX_HW_NUM];
};
