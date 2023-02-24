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

#ifndef __CCU_INTERFACE__
#define __CCU_INTERFACE__

/******************************************************************************
* Task definition
******************************************************************************/
enum ccu_msg_id {
	MSG_TO_CCU_IDLE = 0x00000000,
	MSG_TO_APMCU_FLUSH_LOG,
	MSG_TO_APMCU_CCU_ASSERT,
	MSG_TO_APMCU_CCU_WARNING
};

struct _ccu_msg_t { /*12bytes*/
	enum ccu_msg_id msg_id;
	unsigned int in_data_ptr;
	unsigned int out_data_ptr;
	unsigned int rsv;
};

/******************************************************************************
* Status definition
******************************************************************************/
#define CCU_STATUS_INIT_DONE              0xffff0000
#define CCU_STATUS_INIT_DONE_2            0xffff00a5
#endif
