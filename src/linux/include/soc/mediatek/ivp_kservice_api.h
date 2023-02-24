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
#ifndef __IVP_KSERVICE_API_H__
#define __IVP_KSERVICE_API_H__

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/soc/mediatek/mtk-cmdq.h>

int mtk_send_ivp_request(struct platform_device *pdev,
			 u64 handle, u32 core, struct cmdq_pkt *pkt);

int mtk_clr_ivp_request(struct platform_device *pdev,
			u64 handle, u32 core, struct cmdq_pkt *pkt);

int mtk_reset_ivp_kservice(struct platform_device *pdev,
			   u64 handle, u32 core);

int mtk_lock_ivp_command(struct platform_device *pdev,
			 u32 core, u32 enable);

#endif  /* __IVP_KSERVICE_API_H__ */
