/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: JB Tsai <jb.tsai@mediatek.com>
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
#ifndef __IVP_CMD_HND_H__
#define __IVP_CMD_HND_H__

#include <linux/interrupt.h>

#include "ivp_driver.h"

irqreturn_t ivp_isr_handler(int irq, void *ivp_core_info);
void ivp_device_power_lock(struct ivp_device *ivp_device);
void ivp_device_power_unlock(struct ivp_device *ivp_device);

void ivp_disable_counter(struct ivp_core *ivp_core);
u32 ivp_read_counter(struct ivp_core *ivp_core);

int ivp_load_firmware(struct ivp_core *ivp_core, const char *fw_name);
int ivp_hw_boot_sequence(struct ivp_core *ivp_core);
void ivp_core_power_lock(struct ivp_core *ivp_core);
void ivp_core_power_unlock(struct ivp_core *ivp_core);
void ivp_core_set_auto_suspend(struct ivp_core *ivp_core, int sync);
int ivp_check_fw_boot_status(struct ivp_core *ivp_core);

#if defined(MTK_IVP_LOAD_FROM_KERNEL)
int ivp_request_algo(struct ivp_request *req, struct ivp_device *ivp_device);
#endif
int ivp_load_algo(struct ivp_request *req, struct ivp_device *ivp_device);
int ivp_reload_algo(struct ivp_device *ivp_device, struct ivp_request *req);
int ivp_exec_algo(struct ivp_request *req, struct ivp_device *ivp_device);
int ivp_unload_algo(struct ivp_request *req, struct ivp_device *ivp_device);
int ivp_free_algo(struct ivp_user *user, struct ivp_algo_context *algo_ctx);
int ivp_get_fwinfo(struct ivp_core *ivp_core);

int ivp_set_dbg_ctl(struct ivp_set_dbg_info *ivp_set_dbg_info,
		    struct ivp_core *ivp_core);
int ivp_get_dbg_msg(struct ivp_get_dbg_info *ivp_get_dbg_info,
		    struct ivp_core *ivp_core);
int ivp_set_ptrace_level(u32 level, struct ivp_core *ivp_core);
int ivp_get_ptrace_msg(u32 *log_buffer_wptr, struct ivp_core *ivp_core);
int ivp_reset_hw(struct ivp_core *ivp_core);

#endif /* __IVP_CMD_HND_H__ */
