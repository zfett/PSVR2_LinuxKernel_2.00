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

#ifndef _MTK_SCP_IPI_H_
#define _MTK_SCP_IPI_H_

/* scp Core ID definition*/
enum scp_core_id {
	SCP_A_ID = 0,
	SCP_CORE_TOTAL = 1,
};

/* scp ipi ID definition
 * need to sync with SCP-side
 */
enum scp_ipi_id {
	SCP_IPI_DVFS_SET_CPU = 0,
	SCP_IPI_DVFS_SET_VCORE = 1,
	SCP_IPI_WDT = 5,
	SCP_IPI_LOGGER_INIT = 6,
	SCP_IPI_LOGGER_ENABLE = 7,
	SCP_IPI_SCP_READY = 8,
	SCP_IPI_RESERVED_START = 100,
	SCP_IPI_RESERVED_END = 400,
	SCP_NR_IPI,
};

enum scp_ipi_status {
	SCP_ERROR = -1,
	SCP_DONE,
	SCP_BUSY,
};

enum scp_ipi_status scp_ipi_registration(unsigned int id,
					 void (*handler)(int id, void *data,
					 unsigned int len, void *udata),
					 const char *name, void *pdata);
enum scp_ipi_status scp_ipi_unregistration(unsigned int id);
enum scp_ipi_status scp_ipi_send(unsigned int id, void *buf,
				 unsigned int len, unsigned int wait,
				 enum scp_core_id scp_id);
#endif
