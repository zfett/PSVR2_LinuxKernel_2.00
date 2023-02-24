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
#ifndef __IVP_QUEUE_H__
#define __IVP_QUEUE_H__

#include <uapi/mediatek/ivp_ioctl.h>

int ivp_create_user(struct ivp_user **user, struct ivp_device *ivp_device);
int ivp_delete_user(struct ivp_user *user, struct ivp_device *ivp_device);
int ivp_enque_routine_loop(void *arg);
int ivp_alloc_request(struct ivp_request **rreq);
int ivp_free_request(struct ivp_request *req);
void ivp_setup_enqueue_request(struct ivp_request *req,
			       struct ivp_cmd_enque *cmd_enque);
int ivp_enque_pre_process(struct ivp_device *ivp_device,
			  struct ivp_request *req);
int ivp_enque_post_process(struct ivp_device *ivp_device,
			   struct ivp_request *req,
			   struct ivp_cmd_enque *cmd_enque);
void ivp_deque_post_process(struct ivp_device *ivp_device,
			    struct ivp_request *req);
int ivp_create_algo_ctx(struct ivp_device *ivp_device,
			struct ivp_request *req);
void ivp_free_algo_ctx(struct ivp_request *req);
int ivp_show_algo(struct ivp_device *ivp_device, char *buf, int count);
void ivp_reset_algo_ctx(struct ivp_core *ivp_core);
int ivp_push_request_to_queue(struct ivp_user *user,
			      struct ivp_request *req);
int ivp_pop_request_from_queue(s32 handle, u32 core,
			       struct ivp_user *user,
			       struct ivp_request **rreq);
int ivp_flush_requests_from_queue(u32 core, struct ivp_user *user);

#endif /* __IVP_QUEUE_H__ */
