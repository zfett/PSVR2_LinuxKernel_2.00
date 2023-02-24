/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __SCP_HELPER_H__
#define __SCP_HELPER_H__

#include <linux/notifier.h>
#include <linux/interrupt.h>
#include "scp_reg.h"


/* scp config reg. definition*/
#define SCP_A_TCM_SIZE			(scp_reg.total_tcmsize)
#define SCP_A_SHARE_BUFFER \
	(void*)((char*)scp_reg.sram + SCP_A_TCM_SIZE - SHARE_BUF_SIZE*2/*to+from*/)
#define DEBUG_BUF_SIZE sizeof(struct scp_debug_info_t)
#define SCP_A_DEBUG_INFO	((struct scp_debug_info_t*)((char*)SCP_A_SHARE_BUFFER - DEBUG_BUF_SIZE))
#define SCP_LOG_SIZE	(64*1024)
#define SCP_LOG_TOP	((char*)SCP_A_DEBUG_INFO - SCP_LOG_SIZE)
#define SCP_CRASHINFO_SIZE (256)
#define SCP_CRASHINFO_TOP (SCP_LOG_TOP-SCP_CRASHINFO_SIZE)

struct scp_debug_info_t {
	unsigned int log_pos_info;
	unsigned int size_regdump;
	unsigned int pad[6];
};

/* scp dvfs return status flag */
#define SET_PLL_FAIL		(1)
#define SET_PMIC_VOLT_FAIL	(2)

#define mt_reg_sync_writel(v, a) \
	do {    \
		__raw_writel((v), (void __force __iomem *)((a)));   \
		mb();	/*fix for memory barrier without comment */ \
	} while (0)

/* scp notify event */
enum SCP_NOTIFY_EVENT {
	SCP_EVENT_READY = 0,
	SCP_EVENT_STOP,
};

/* reset ID */
#define SCP_ALL_ENABLE	0x00
#define SCP_ALL_REBOOT	0x01
#define SCP_A_ENABLE	0x10
#define SCP_A_REBOOT	0x11

/* scp semaphore definition*/
enum SEMAPHORE_FLAG {
	SEMAPHORE_TEST = 0,
	NR_FLAG,
};

struct scp_regs {
	struct device *dev;
	void __iomem *scpsys;
	void __iomem *sram;
	void __iomem *cfg;
	void __iomem *clkctrl;
	int irq;
	unsigned int total_tcmsize;
	unsigned int cfgregsize;
};

/* scp work struct definition*/
struct scp_work_struct {
	struct work_struct work;
	unsigned int flags;
	unsigned int id;
};

extern struct scp_regs scp_reg;
extern const struct file_operations scp_A_log_file_ops;
extern const struct file_operations scp_A_crashinfo_file_ops;
extern struct bin_attribute bin_attr_scp_dump;

/* scp irq */
irqreturn_t scp_A_irq_handler(int irq, void *dev_id);
void scp_A_irq_init(void);
void scp_A_ipi_init(void);

/* scp helper */
int get_scp_semaphore(int flag);
int release_scp_semaphore(int flag);
int scp_get_semaphore_3way(int flag);
int scp_release_semaphore_3way(int flag);

void memcpy_to_scp(void __iomem *trg, const void *src, int size);
void memcpy_from_scp(void *trg, const void __iomem *src, int size);
int reset_scp(int reset);

extern spinlock_t scp_awake_spinlock;

#endif
