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

#ifndef __IVP_EXP_H__
#define __IVP_EXP_H__

#include "include/fw/ivp_interface.h"
#include "include/fw/ivp_macro.h"
#include "include/fw/ivp_typedef.h"

#include "ivp_driver.h"

#define EXCP_INFO_NUM	16
#define UFRAME_INFO_NUM 23

#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
extern int int_count[3];
extern IVP_UINT32 prev_pc[3];
extern enum ivp_state prev_fw_state[3];
#endif

enum ivp_exccause {
	EXCCAUSE_ILLEGAL = MAKETAG(0xff00, 0),
	EXCCAUSE_SYSCALL = MAKETAG(0xff00, 1),
	EXCCAUSE_INSTR_ERROR = MAKETAG(0xff00, 2),
	EXCCAUSE_LOAD_STORE_ERROR = MAKETAG(0xff00, 3),
	EXCCAUSE_LEVEL1_INTERRUPT = MAKETAG(0xff00, 4),
	EXCCAUSE_ALLOCA = MAKETAG(0xff00, 5),
	EXCCAUSE_DIVIDE_BY_ZERO = MAKETAG(0xff00, 6),
	EXCCAUSE_SPECULATION = MAKETAG(0xff00, 7),
	EXCCAUSE_PRIVILEGED = MAKETAG(0xff00, 8),
	EXCCAUSE_UNALIGNED = MAKETAG(0xff00, 9),

	EXCCAUSE_INSTR_DATA_ERROR = MAKETAG(0xff00, 12),
	EXCCAUSE_LOAD_STORE_DATA_ERROR = MAKETAG(0xff00, 13),
	EXCCAUSE_INSTR_ADDR_ERROR = MAKETAG(0xff00, 14),
	EXCCAUSE_LOAD_STORE_ADDR_ERROR = MAKETAG(0xff00, 15),
	EXCCAUSE_ITLB_MISS = MAKETAG(0xff00, 16),
	EXCCAUSE_ITLB_MULTIHIT = MAKETAG(0xff00, 17),
	EXCCAUSE_INSTR_RING = MAKETAG(0xff00, 18),

	EXCCAUSE_INSTR_PROHIBITED = MAKETAG(0xff00, 20),

	EXCCAUSE_DTLB_MISS = MAKETAG(0xff00, 24),
	EXCCAUSE_DTLB_MULTIHIT = MAKETAG(0xff00, 25),
	EXCCAUSE_LOAD_STORE_RING = MAKETAG(0xff00, 26),

	EXCCAUSE_LOAD_PROHIBITED = MAKETAG(0xff00, 28),
	EXCCAUSE_STORE_PROHIBITED = MAKETAG(0xff00, 29),

	EXCCAUSE_DOUBLE_EXCEPTION = MAKETAG(0xff00, 62),
	EXCCAUSE_DEBUG_EXCEPTION = MAKETAG(0xff00, 63),
};

struct ivp_user_frame {
	IVP_UINT32 pc;
	IVP_UINT32 ps;
	IVP_UINT32 sar;
	IVP_UINT32 vpri;

	/* local register */
	IVP_UINT32 a0;
	IVP_UINT32 a2;
	IVP_UINT32 a3;
	IVP_UINT32 a4;
	IVP_UINT32 a5;
	IVP_UINT32 a6;
	IVP_UINT32 a7;
	IVP_UINT32 a8;
	IVP_UINT32 a9;
	IVP_UINT32 a10;
	IVP_UINT32 a11;
	IVP_UINT32 a12;
	IVP_UINT32 a13;
	IVP_UINT32 a14;
	IVP_UINT32 a15;

	/* exception */
	IVP_UINT32 exccause;

	/* loop */
	IVP_UINT32 lcount;
	IVP_UINT32 lbeg;
	IVP_UINT32 lend;
};

struct ivp_err_inform {
	/* pc */
	IVP_UINT32 pc;

	/* cmd */
	enum ivp_cmd apmcu_cmd;
	IVP_UINT32 arg0;
	IVP_UINT32 arg1;
	IVP_UINT32 arg2;

	/* cmd_rtn */
	enum ivp_state fw_state;
	IVP_INT32 cmd_result;
	IVP_UINT32 algo_hnd;

	/* exception */
	IVP_UINT32 memctl;
	IVP_UINT32 exccause;
	IVP_UINT32 excsave1;
	IVP_UINT32 excvaddr;
	IVP_UINT32 pSP;
	IVP_UINT32 ps;
	IVP_UINT32 epc1;
	IVP_UINT32 epc2;
	IVP_UINT32 epc3;
	IVP_UINT32 epc4;
	IVP_UINT32 eps2;
	IVP_UINT32 eps3;
	IVP_UINT32 eps4;

	IVP_UINT32 windowbase;
	IVP_UINT32 windowstart;

	/* user_frame */
	IVP_UINT32 uf_base;
	bool call0_abi;
	bool loops;
	struct ivp_user_frame *uf;
};

int ivp_get_error_report(struct ivp_core *ivp_core,
			 struct ivp_request *req);
void ivp_excp_handler(struct ivp_core *ivp_core,
		      struct ivp_request *req);


#endif /* __IVP_EXP_H__ */
