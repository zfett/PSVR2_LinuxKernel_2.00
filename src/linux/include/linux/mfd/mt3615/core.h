/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: zm.Chen, MediaTek
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

#ifndef __MFD_MT3615_CORE_H__
#define __MFD_MT3615_CORE_H__

#include <linux/hrtimer.h>

enum mt3615_irq_numbers {
	MT3615_INT_STATUS_PWRKEY = 0,
	MT3615_INT_STATUS_PWRKEY_R,
	MT3615_INT_STATUS_NI_LBAT_INT,
	MT3615_INT_STATUS_CABDET,
	MT3615_INT_STATUS_CABDET_EDGE,
	MT3615_INT_STATUS_RTC,
	MT3615_INT_STATUS_RTC_NSEC,
	MT3615_INT_STATUS_VPROC_OC = 16,
	MT3615_INT_STATUS_VGPU11_OC,
	MT3615_INT_STATUS_VGPU12_OC,
	MT3615_INT_STATUS_VPU_OC,
	MT3615_INT_STATUS_VCORE1_OC,
	MT3615_INT_STATUS_VCORE2_OC,
	MT3615_INT_STATUS_VCORE4_OC,
	MT3615_INT_STATUS_VS1_OC,
	MT3615_INT_STATUS_VS2_OC,
	MT3615_INT_STATUS_VS3_OC,
	MT3615_INT_STATUS_VDRAM1_OC,
	MT3615_INT_STATUS_VDRAM2_OC,
	MT3615_INT_STATUS_VIO18_OC,
	MT3615_INT_STATUS_VIO31_OC,
	MT3615_INT_STATUS_VAUX18_OC,
	MT3615_INT_STATUS_VXO_OC,
	MT3615_INT_STATUS_VEFUSE_OC,
	MT3615_INT_STATUS_VM18_OC,
	MT3615_INT_STATUS_VUSB_OC,
	MT3615_INT_STATUS_VA18_OC,
	MT3615_INT_STATUS_VA12_OC,
	MT3615_INT_STATUS_VA09_OC,
	MT3615_INT_STATUS_VSRAM_PROC_OC,
	MT3615_INT_STATUS_VSRAM_COREX_OC,
	MT3615_INT_STATUS_VSRAM_GPU_OC,
	MT3615_INT_STATUS_VSRAM_VPU_OC,
	MT3615_INT_STATUS_VPIO31_OC,
	MT3615_INT_STATUS_VBBCK_OC,
	MT3615_INT_STATUS_VPIO18_OC,

	MT3615_IRQ_NR,
};

enum SHUTDOWN_IDX {
	SHUTDOWN1 = 0,
	SHUTDOWN2,
	SHUTDOWN3,
};

struct mt3615_chip {
	struct device *dev;
	struct regmap *regmap;
	int key_irq;
	int cable_irq;
	struct irq_domain *irq_domain;
	struct mutex irqlock;
	struct hrtimer	ptimer;
	wait_queue_head_t wait_queue;
	unsigned int pmic_thread_flag;
	u16 wake_mask[7];
	u16 irq_masks_cur[7];
	u16 irq_masks_cache[7];
	u16 int_con[7];
	u16 int_status[7];
};
struct cmd_t {
	char name[256];
	int (*cb_func)(struct mt3615_chip *ut_dev,
		int argc, char **argv);
};

#endif /* __MFD_MT3615_CORE_H__ */
