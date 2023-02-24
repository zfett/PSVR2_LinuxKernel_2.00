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

#ifndef __SCP_REG_H
#define __SCP_REG_H

#define SCP_BASE			(scp_reg.cfg)
#define SCP_CM4_RESET_ASSERT		(0x1ffffffe)
#define SCP_CM4_RESET_DEASSERT		(0x1fffffff)

#define SCP_A_TO_HOST_REG		(scp_reg.cfg + 0x001c)
#define SCP_IRQ_SCP2HOST		(1UL << 0)
#define SCP_IRQ_WDT			(1UL << 8)

#define SCP_TO_DVFSCTL_REG		(scp_reg.cfg + 0x0020)
#define SCP_TO_DVFSCTL_EVENT_CLR_ALL	(0x000000ff)
#define SCP_TO_DVFSCTL_EVENT_CLR	(0x00000001)

#define GIPC_TO_SCP_REG			(scp_reg.cfg + 0x0028)
#define HOST_TO_SCP_A			(1UL << 0)

/* scp awake lock definition */
#define SCP_A_IPI_AWAKE_NUM		(2)

#define SCP_A_WDT_REG			(scp_reg.cfg + 0x0084)

#define SCP_A_GENERAL_REG0		(scp_reg.cfg + 0x0050)
#define SCP_A_GENERAL_REG1		(scp_reg.cfg + 0x0054)
#define SCP_A_GENERAL_REG2		(scp_reg.cfg + 0x0058)
#define SCP_A_GENERAL_REG3		(scp_reg.cfg + 0x005C)
#define SCP_A_GENERAL_REG4		(scp_reg.cfg + 0x0060)
#define SCP_A_GENERAL_REG5		(scp_reg.cfg + 0x0064)
#define SCP_A_GENERAL_REG6		(scp_reg.cfg + 0x0068)
#define SCP_A_GENERAL_REG7		(scp_reg.cfg + 0x006c)
#define SCP_SEMAPHORE			(scp_reg.cfg + 0x0090)
#define SCP_SEMAPHORE_3WAY		(scp_reg.cfg + 0x0094)
#define SCP_SLP_PROTECT_CFG		(scp_reg.cfg + 0x00c8)

#define SCP_SLEEP_STATUS_REG		(scp_reg.cfg + 0x0114)
#define SCP_A_IS_SLEEP			(1<<0)
#define SCP_A_IS_DEEPSLEEP		(1<<1)

#endif
