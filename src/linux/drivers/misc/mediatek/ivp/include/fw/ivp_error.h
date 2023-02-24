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

#ifndef _IVP_ERROR_H
#define _IVP_ERROR_H

#define IVP_NO_ERROR		(0)
#define IVP_BAD_PARAMETER	(-1)
#define IVP_BAD_ATTRIBUTE	(-2)
#define IVP_OUT_OF_MEMORY	(-3)
#define IVP_NULL_POINTER	(-4)

#define IVP_BUSY_RETRY		(-5)
#define IVP_UNSUPPORT_CMD	(-6)
#define IVP_FLO_UNAVAILABLE	(-7)

#define IVP_EXCEPTION_FAULT		(-40)

/*
 * L for load
 * R for reload
 * E for execute
 * U for unload
 */
#define IVP_KERNEL_LTIMEOUT	(-50)
#define IVP_KERNEL_RTIMEOUT	(-51)
#define IVP_KERNEL_ETIMEOUT	(-52)
#define IVP_KERNEL_UTIMEOUT	(-53)
#define IVP_KERNEL_LINVALID	(-54)
#define IVP_KERNEL_RINVALID	(-55)
#define IVP_KERNEL_EINVALID	(-56)
#define IVP_KERNEL_UINVALID	(-57)

/* exception inform */
#define IVP_EXCP_pSP		0
#define IVP_EXCP_MEMCTL		1
#define IVP_EXCP_PS		2
#define IVP_EXCP_WBASE		3
#define IVP_EXCP_WSTART		4
#define IVP_EXCP_EXCCAUSE	5
#define IVP_EXCP_EXCSAVE1	6
#define IVP_EXCP_EXCVADDR	7
#define IVP_EXCP_EPC1		8
#define IVP_EXCP_EPC2		9
#define IVP_EXCP_EPC3		10
#define IVP_EXCP_EPC4		11
#define IVP_EXCP_EPS2		12
#define IVP_EXCP_EPS3		13
#define IVP_EXCP_EPS4		14

#endif /* _IVP_ERROR_H */

