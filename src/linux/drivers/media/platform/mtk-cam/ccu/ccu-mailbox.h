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

#ifndef __CCU_MAILBOX_H__
#define __CCU_MAILBOX_H__

#include <soc/mediatek/ccu_ext_interface/ccu_mailbox_extif.h>

struct ccu_device;

enum mb_result {
	MAILBOX_OK = 0,
	MAILBOX_QUEUE_FULL,
	MAILBOX_QUEUE_EMPTY
};

struct ccu_mailbox {
	struct _ccu_mailbox_t *apmcu_mb_addr;
	struct _ccu_mailbox_t *ccu_mb_addr;
};

enum mb_result mtk_ccu_mailbox_init(struct ccu_device *cdev);

enum mb_result mtk_ccu_mailbox_send_cmd(struct ccu_device *cdev,
	struct _ccu_msg_t *task);

enum mb_result mtk_ccu_mailbox_receive_cmd(struct ccu_device *cdev,
	struct _ccu_msg_t *task);

#endif
