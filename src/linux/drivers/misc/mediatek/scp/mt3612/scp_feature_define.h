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

#ifndef __SCP_FEATURE_DEFINE_H__
#define __SCP_FEATURE_DEFINE_H__


/* scp platform configs*/
/*#define SCP_BOOT_TIME_OUT_MONITOR       (1) */
#define SCP_LOGGER_ENABLE               (1)

/* scp logger size definition*/
#define LOG_TO_AP_UART_LINE 64
#define SCP_FIFO_SIZE 2048

/* emi mpu define*/
#define ENABLE_SCP_EMI_PROTECTION       (1)
#define MPU_REGION_ID_SCP_SMEM		30

#endif
