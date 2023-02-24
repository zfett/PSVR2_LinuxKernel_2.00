/*
 * Copyright (c) 2015 MediaTek Inc.
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

#ifndef _MTK_DPRX_CMD_H_
#define _MTK_DPRX_CMD_H_

/**
 * @file mtk_dprx_cmd_int.h
 * Header of Linux debug file system related function
 */

/* ====================== Function Prototype ======================= */
int dprxcmd_debug_init(struct mtk_dprx *dprx);
void dprxcmd_debug_uninit(struct mtk_dprx *dprx);

#endif
