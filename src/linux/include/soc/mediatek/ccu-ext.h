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

#ifndef __CCU_EXT_H
#define __CCU_EXT_H

#include <linux/ioctl.h>
#include "ccu_ext_interface/ccu_mailbox_extif.h"

#define MAX_LOG_BUF_NUM 2

enum ccu_log_status_e {
	CCU_LOG_FLUSH_LOG_BUF_1 = 1,
	CCU_LOG_FLUSH_LOG_BUF_2,
	CCU_LOG_ASSERT,
	CCU_LOG_WARNING,
};

struct ccu_wait_irq {
	enum ccu_log_status_e log_idx;
};

struct ccu_log {
	unsigned int log_addr[MAX_LOG_BUF_NUM];
};

enum ccu_power_control {
	CCU_POWER_ON,
	CCU_POWER_OFF,
	CCU_POWER_RESTART,
	CCU_POWER_PAUSE,
	CCU_POWER_ENABLE_CG_ONLY,
};

struct ccu_power {
	enum ccu_power_control control;
	unsigned int freq;
	unsigned int power;
};

enum ccu_eng_status_e {
	CCU_ENG_STATUS_SUCCESS,
	CCU_ENG_STATUS_BUSY,
	CCU_ENG_STATUS_TIMEOUT,
	CCU_ENG_STATUS_INVALID,
	CCU_ENG_STATUS_FLUSH,
	CCU_ENG_STATUS_FAILURE,
	CCU_ENG_STATUS_ERESTARTSYS,
};

struct ccu_cmd {
	struct _ccu_msg_t task;
	enum ccu_eng_status_e status;
};

struct ccu_hw_info {
	unsigned int ccu_hw_base;
	unsigned int ccu_hw_size;
	unsigned int ccu_pmem_base;
	unsigned int ccu_pmem_size;
	unsigned int ccu_dmem_base;
	unsigned int ccu_dmem_size;
	unsigned int ccu_dmem_offset;
	unsigned int ccu_log_base;
	unsigned int ccu_log_size;
};

#define CCU_MAGIC 'C'
#define CCU_SET_POWER		_IOW(CCU_MAGIC, 0, struct ccu_power)
#define CCU_ENQUE_COMMAND	_IOW(CCU_MAGIC, 1, struct ccu_cmd)
#define CCU_DEQUE_COMMAND	_IOWR(CCU_MAGIC, 2, struct ccu_cmd)
#define CCU_FLUSH_COMMAND	_IO(CCU_MAGIC, 3)
#define CCU_QUERY_HW_INFO	_IOR(CCU_MAGIC, 4, struct ccu_hw_info)
#define CCU_WAIT_IRQ		_IOR(CCU_MAGIC, 5, struct ccu_wait_irq)
#define CCU_SET_RUN		_IO(CCU_MAGIC, 6)
#define CCU_SET_LOG_ADDR	_IOW(CCU_MAGIC, 7, struct ccu_log)
#define CCU_FLUSH_LOG		_IO(CCU_MAGIC, 8)

#endif /* __CCU_EXT_H */
