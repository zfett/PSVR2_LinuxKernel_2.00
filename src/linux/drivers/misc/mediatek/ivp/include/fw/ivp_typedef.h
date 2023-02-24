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

#ifndef __IVP_TYPEDEF_H__
#define __IVP_TYPEDEF_H__

typedef signed long long	IVP_INT64;
typedef unsigned long long	IVP_UINT64;
typedef signed int	IVP_INT32;
typedef unsigned int	IVP_UINT32;
typedef signed short	IVP_INT16;
typedef unsigned short	IVP_UINT16;
typedef signed char	IVP_INT8;
typedef unsigned char	IVP_UINT8;

typedef unsigned int	IVP_IOVA;

struct ivp_buffer_info {
	IVP_IOVA		buf_base;
	IVP_UINT32		buf_size;
};

#endif /* __IVP_TYPEDEF_H__ */

