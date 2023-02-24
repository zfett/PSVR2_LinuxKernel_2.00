/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: CJ Yang <cj.yang@mediatek.com>
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

#ifndef _IVP_MACRO_H
#define _IVP_MACRO_H

#define MAKETAG(a, b) (((a & 0xffffU) << 16) + (b & 0xffffU))

#define T_IVP		0x3361
#define T_DBG		0xDB23
#define T_STATE		0x1228

#endif /* _IVP_MACRO_H */
