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
#ifndef __IVP_SYSTRACE_H__
#define __IVP_SYSTRACE_H__

#ifdef IVP_PERFORMANCE

#define ATRACE_KERNEL_DEBUG
#include <linux/atrace_kernel.h>

#define ivp_atrace_init() atrace_init()
#define ivp_atrace_begin(name) atrace_begin(name)
#define ivp_atrace_end(name) atrace_end(name)

#else

#define ivp_atrace_init()
#define ivp_atrace_begin(name)
#define ivp_atrace_end(name)

#endif

#endif /* __IVP_SYSTRACE_H__ */
