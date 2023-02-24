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

#ifndef __SCP_IPI_H
#define __SCP_IPI_H

#include "scp_reg.h"
#include <soc/mediatek/mtk_scp_ipi.h>

#define SHARE_BUF_SIZE 288
/* scp awake timout count definition*/
#define SCP_AWAKE_TIMEOUT 5000

struct scp_ipi_desc {
	unsigned int recv_count;
	unsigned int success_count;
	unsigned int busy_count;
	unsigned int error_count;
	void (*handler)(int id, void *data, unsigned int len, void *udata);
	const char *name;
	void *pdata;
};

struct share_obj {
	enum scp_ipi_id id;
	unsigned int len;
	unsigned char reserve[8];
	unsigned char share_buf[SHARE_BUF_SIZE - 16];
};

extern unsigned char *scp_send_buff[SCP_CORE_TOTAL];
extern unsigned char *scp_recv_buff[SCP_CORE_TOTAL];
extern const char *core_ids[SCP_CORE_TOTAL];

unsigned int is_scp_ready(enum scp_core_id scp_id);
void scp_A_ipi_handler(void);
void scp_ipi_status_dump(void);
void scp_ipi_status_dump_id(enum scp_ipi_id id);
#endif
