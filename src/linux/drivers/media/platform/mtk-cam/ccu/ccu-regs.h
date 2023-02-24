/*
 * Copyright (C) 2016 MediaTek Inc.
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

#ifndef __CCU_REGS_H__
#define __CCU_REGS_H__

#define CCU_RESET 0x00
#define CTL_CCU_INT 0x3C
#define EINTC_CLR 0x54
#define EINTC_ST 0x58
#define CCU_INFO00 0x80
#define CCU_INFO01 0x84
#define CCU_INFO02 0x88
#define CCU_INFO03 0x8C
#define CCU_INFO04 0x90
#define CCU_INFO05 0x94
#define CCU_INFO06 0x98
#define CCU_INFO07 0x9C
#define CCU_INFO08 0xA0
#define CCU_INFO09 0xA4
#define CCU_INFO10 0xA8
#define CCU_INFO11 0xAC
#define CCU_INFO12 0xB0
#define CCU_INFO13 0xB4
#define CCU_INFO14 0xB8
#define CCU_INFO15 0xBC
#define CCU_INFO16 0xC0
#define CCU_INFO17 0xC4
#define CCU_INFO18 0xC8
#define CCU_INFO19 0xCC
#define CCU_INFO20 0xD0
#define CCU_INFO21 0xD4
#define CCU_INFO22 0xD8
#define CCU_INFO23 0xDC
#define CCU_INFO24 0xE0
#define CCU_INFO25 0xE4
#define CCU_INFO26 0xE8
#define CCU_INFO27 0xEC
#define CCU_INFO28 0xF0
#define CCU_INFO29 0xF4
#define CCU_INFO30 0xF8
#define CCU_INFO31 0xFC

/* spare register define */
#define CCU_STA_REG_SW_INIT_DONE CCU_INFO30
#define CCU_STA_REG_3A_INIT_DONE CCU_INFO01
#define CCU_STA_REG_SP_ISR_TASK CCU_INFO24
#define CCU_STA_REG_I2C_TRANSAC_LEN CCU_INFO25
#define CCU_STA_REG_I2C_DO_DMA_EN CCU_INFO26

/*
 * Spare Register	Data Type	Field
 * 0				int32		APMCU mailbox addr.
 * 1				int32		CCU mailbox addr.
 * 2				int32		DRAM log buffer addr.1
 * 3				int32		DRAM log buffer addr.2
 */
#define CCU_DATA_REG_MAILBOX_APMCU CCU_INFO00
#define CCU_DATA_REG_MAILBOX_CCU CCU_INFO01
#define CCU_DATA_REG_LOG_BUF0 CCU_INFO02
#define CCU_DATA_REG_LOG_BUF1 CCU_INFO03

#define MD32_HW_RST 16
#define INT_CTL_CCU 0

#endif /* __CCU_REGS_H__ */
