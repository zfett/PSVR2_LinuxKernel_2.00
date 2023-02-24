/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: Mao Lin <Zih-Ling.Lin@mediatek.com>
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
#ifndef __IVP_KSERVICE_H__
#define __IVP_KSERVICE_H__

#include "ivp_driver.h"
#include <soc/mediatek/ivp_kservice_api.h>

struct ivp_kservice_request {
	struct ivp_request base;
	unsigned int issue_id;
	unsigned int submit_stamp;
};

int ivp_lock_kservice(struct ivp_core *ivp_core);
int ivp_unlock_kservice(struct ivp_core *ivp_core);

int ivp_alloc_kservice_request(struct ivp_kservice_request **rreq,
			       struct ivp_device *ivp_device);
int ivp_free_kservice_request(struct ivp_kservice_request *req);
int ivp_push_request_to_kservice_queue(struct ivp_kservice_request *kreq,
				       struct ivp_device *ivp_device);
int ivp_pop_request_from_kservice_queue(u64 handle, u32 core,
					struct ivp_device *ivp_device,
					struct ivp_kservice_request **rreq);

int ivp_test_auto_run(u32 core, struct ivp_device *ivp_device);


#endif  /* __IVP_KSERVICE_H__ */
