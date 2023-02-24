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

#ifndef _IVP_INTERFACE_H
#define _IVP_INTERFACE_H

#include "ivp_macro.h"
#include "ivp_typedef.h"

/******************** IVP Normal Command Define ********************/
/*
 * IVP_XTENSA_INFO16_REG
 * IVP_XTENSA_INFO17_REG
 * IVP_XTENSA_INFO18_REG
 * IVP_XTENSA_INFO19_REG
 *
 * CMD_IDLE :
 *		description : to idle the ivp firmware
 *		argument :
 *			IVP_XTENSA_INFO16_REG : CMD_IDLE
 *		return :
 *			IVP_XTENSA_INFO00_REG : ivp_state
 *			IVP_XTENSA_INFO01_REG : result at ivp_error.h
 * CMD_LOAD_PROC :
 *		description : to load the algo binary
 *		argument :
 *			IVP_XTENSA_INFO16_REG : CMD_LOAD_PROC
 *			IVP_XTENSA_INFO17_REG : arg0, the flag to load algo
 *				#define FLAG_LOAD_FLO		0x01
 *			IVP_XTENSA_INFO18_REG : arg1, the algo binary size
 *				IVP_UINT32
 *			IVP_XTENSA_INFO19_REG : arg2, the algo binary
 *				IVP_IOVA
 *		return :
 *			IVP_XTENSA_INFO00_REG : ivp_state
 *			IVP_XTENSA_INFO01_REG : result at ivp_error.h
 *			IVP_XTENSA_INFO02_REG : the handle of loaded algo
 * CMD_EXEC_PROC :
 *		description : to execute the algo
 *		argument :
 *			IVP_XTENSA_INFO16_REG : CMD_EXEC_PROC
 *			IVP_XTENSA_INFO17_REG : arg0, the handle of loaded algo
 *			IVP_XTENSA_INFO18_REG : arg1, the size of algo argument
 *			IVP_XTENSA_INFO19_REG : arg2, the algo argument
 *				IVP_IOVA
 *		return :
 *			IVP_XTENSA_INFO00_REG : ivp_state
 *			IVP_XTENSA_INFO01_REG : result at ivp_error.h
 * CMD_UNLOAD_PROC :
 *		description : to execute the algo
 *		argument :
 *			IVP_XTENSA_INFO16_REG : CMD_UNLOAD_PROC
 *			IVP_XTENSA_INFO17_REG : arg0, the handle of loaded algo
 *		return :
 *			IVP_XTENSA_INFO00_REG : ivp_state
 *			IVP_XTENSA_INFO01_REG : result at ivp_error.h
 * CMD_GET_IVPINFO :
 *		description : to execute the algo
 *		argument :
 *			IVP_XTENSA_INFO16_REG : CMD_GET_IVPINFO
 *			IVP_XTENSA_INFO17_REG : arg0, the flag of ivpinfo
 *				#define GET_IVP_FW_VERSION	0x01
 *		return :
 *			IVP_XTENSA_INFO00_REG : ivp_state
 *			IVP_XTENSA_INFO01_REG : result at ivp_error.h
 *			IVP_XTENSA_INFO02_REG : depend by flag
 *				GET_IVP_FW_VERSION : the ivp firmware version
 * CMD_EXIT :
 *		description : to exit the ivp firmware
 *		argument :
 *			IVP_XTENSA_INFO16_REG : CMD_EXIT
 *		return :
 *			IVP_XTENSA_INFO00_REG : ivp_state
 *			IVP_XTENSA_INFO01_REG : result at ivp_error.h
 *
 */

/* the normal command of ivp firmware */
enum ivp_cmd {
	CMD_IDLE = 0,
	CMD_LOAD_PROC = MAKETAG(T_IVP, 0x01),
	CMD_EXEC_PROC = MAKETAG(T_IVP, 0x02),
	CMD_UNLOAD_PROC = MAKETAG(T_IVP, 0x03),
	CMD_GET_IVPINFO = MAKETAG(T_IVP, 0x04),
	CMD_SET_SLEEP = MAKETAG(T_IVP, 0x05),
	CMD_SET_WAKEUP = MAKETAG(T_IVP, 0x06),

	CMD_EXIT = MAKETAG(T_IVP, 0xFFE0),
};

/* the state of ivp firmware */
enum ivp_state {
	STATE_BOOT_START = MAKETAG(T_STATE, 0x01),
	STATE_WAIT_ISR = MAKETAG(T_STATE, 0x02),
	STATE_PROCESS_CMD = MAKETAG(T_STATE, 0x03),
	STATE_WAIT_ISR_BUSY = MAKETAG(T_STATE, 0x04),

	STATE_EXIT = MAKETAG(T_STATE, 0xFFE0),
};

/* CMD_LOAD_PROC INFO17, arg0 flag */
#define FLAG_LOAD_FLO			1	/* fixed overlay memory */

/* CMD_GET_IVPINFO INFO17, arg0 flag */
#define GET_IVP_FW_VERSION		1
#define IVP_FW_MAJOR_VERSION_MASK	0xFFFF0000
#define IVP_FW_MINOR_VERSION_MASK	0xFFFF

/* apmcu command register index and counter */
#define APMCU_CMD_REG_IDX		16
#define APMCU_CMD_REG_COUNT		4

struct apmcu_load_cmd {
	enum ivp_cmd apmcu_cmd;		/* IVP_XTENSA_INFO16_REG */
	IVP_UINT32 flags;		/* IVP_XTENSA_INFO17_REG */
	IVP_UINT32 algo_size;		/* IVP_XTENSA_INFO18_REG */
	IVP_IOVA algo_base;		/* IVP_XTENSA_INFO19_REG */
	IVP_UINT32 timestamp;		/* IVP_XTENSA_INFO15_REG */
};

struct apmcu_exec_cmd {
	enum ivp_cmd apmcu_cmd;		/* IVP_XTENSA_INFO16_REG */
	IVP_UINT32 algo_hnd;		/* IVP_XTENSA_INFO17_REG */
	IVP_UINT32 algo_arg_size;	/* IVP_XTENSA_INFO18_REG */
	IVP_IOVA algo_arg_base;		/* IVP_XTENSA_INFO19_REG */
	IVP_UINT32 timestamp;		/* IVP_XTENSA_INFO15_REG */
};

struct apmcu_unload_cmd {
	enum ivp_cmd apmcu_cmd;		/* IVP_XTENSA_INFO16_REG */
	IVP_UINT32 algo_hnd;		/* IVP_XTENSA_INFO17_REG */
	IVP_UINT32 timestamp;		/* IVP_XTENSA_INFO15_REG */
};

struct apmcu_getinfo_cmd {
	enum ivp_cmd apmcu_cmd;		/* IVP_XTENSA_INFO16_REG */
	IVP_UINT32 flags;		/* IVP_XTENSA_INFO17_REG */
	IVP_UINT32 timestamp;		/* IVP_XTENSA_INFO15_REG */
};

struct apmcu_setsleep_cmd {
	enum ivp_cmd apmcu_cmd;		/* IVP_XTENSA_INFO16_REG */
	IVP_UINT32 timestamp;		/* IVP_XTENSA_INFO15_REG */
};

struct apmcu_setwakeup_cmd {
	enum ivp_cmd apmcu_cmd;		/* IVP_XTENSA_INFO16_REG */
	IVP_UINT32 timestamp;		/* IVP_XTENSA_INFO15_REG */
};

struct apmcu_exit_cmd {
	enum ivp_cmd apmcu_cmd;		/* IVP_XTENSA_INFO16_REG */
	IVP_UINT32 timestamp;		/* IVP_XTENSA_INFO15_REG */
};

/* apmcu return command register index and counter */
#define APMCU_CMD_REG_RTN_IDX		0
#define APMCU_CMD_REG_RTN_COUNT		5

struct apmcu_load_cmd_rtn {
	enum ivp_state fw_state;	/* IVP_XTENSA_INFO00_REG */
	IVP_INT32 cmd_result;		/* IVP_XTENSA_INFO01_REG */
	IVP_UINT32 algo_hnd;		/* IVP_XTENSA_INFO02_REG */
	IVP_UINT32 code_mem;		/* IVP_XTENSA_INFO08_REG */
	IVP_UINT32 data_mem;		/* IVP_XTENSA_INFO09_REG */
};

struct apmcu_exec_cmd_rtn {
	enum ivp_state fw_state;	/* IVP_XTENSA_INFO00_REG */
	IVP_INT32 cmd_result;		/* IVP_XTENSA_INFO01_REG */
};

struct apmcu_unload_cmd_rtn {
	enum ivp_state fw_state;	/* IVP_XTENSA_INFO00_REG */
	IVP_INT32 cmd_result;		/* IVP_XTENSA_INFO01_REG */
};

struct apmcu_getinfo_cmd_rtn {
	enum ivp_state fw_state;	/* IVP_XTENSA_INFO00_REG */
	IVP_INT32 cmd_result;		/* IVP_XTENSA_INFO01_REG */
	IVP_UINT32 fw_info;		/* IVP_XTENSA_INFO02_REG */
};

struct apmcu_setsleep_cmd_rtn {
	enum ivp_state fw_state;	/* IVP_XTENSA_INFO00_REG */
	IVP_INT32 cmd_result;		/* IVP_XTENSA_INFO01_REG */
};

struct apmcu_setwakeup_cmd_rtn {
	enum ivp_state fw_state;	/* IVP_XTENSA_INFO00_REG */
	IVP_INT32 cmd_result;		/* IVP_XTENSA_INFO01_REG */
};

struct apmcu_exit_cmd_rtn {
	enum ivp_state fw_state;	/* IVP_XTENSA_INFO00_REG */
	IVP_INT32 cmd_result;		/* IVP_XTENSA_INFO01_REG */
};

/******************** IVP Debug Command Define ********************/
/*
 * IVP_XTENSA_INFO20_REG
 * IVP_XTENSA_INFO21_REG
 * IVP_XTENSA_INFO22_REG
 *
 * IVP_DBG_SET_CONTROL :
 *		description : to set debug method
 *		argument :
 *			IVP_XTENSA_INFO20_REG : IVP_DBG_SET_CONTROL
 *			IVP_XTENSA_INFO21_REG : flag of IVP_DBG_SET_CONTROL
 *				#define FLAG_SET_CONTAOL		0x01
 *				#define FLAG_SET_LEVEL			0x02
 *			IVP_XTENSA_INFO22_REG : ivp_dbg_info
 *				IVP_IOVA
 *		return :
 *			IVP_XTENSA_INFO03_REG : IVP_DBG_CMD_DONE
 * IVP_DBG_GET_ERROR_MSG :
 *		description : to get the debug buffer point
 *		argument :
 *			IVP_XTENSA_INFO20_REG : IVP_DBG_GET_ERROR_MSG
 *			IVP_XTENSA_INFO21_REG : flag of IVP_DBG_GET_ERROR_MSG
 *				#define FLAG_GET_MSG_BUF_PTR		0x01
 *				#define FLAG_GET_LOG_BUF_WRITE_PTR	0x02
 *			IVP_XTENSA_INFO22_REG : ivp_dbg_info
 *				IVP_IOVA
 *		return :
 *			IVP_XTENSA_INFO03_REG : IVP_DBG_CMD_DONE
 *
 * IVP_PTRACE_SET_LEVEL :
 *		description : to set ptrace level
 *		argument :
 *			IVP_XTENSA_INFO20_REG : IVP_PTRACE_SET_LEVEL
 *			IVP_XTENSA_INFO21_REG : 0
 *			IVP_XTENSA_INFO22_REG : ivp_dbg_info
 *				IVP_IOVA
 *		return :
 *			IVP_XTENSA_INFO03_REG : IVP_DBG_CMD_DONE
 *
 * IVP_PTRACE_GET_MSG :
 *		description : to get ptrace log buffer write point
 *		argument :
 *			IVP_XTENSA_INFO20_REG : IVP_PTRACE_GET_MSG
 *			IVP_XTENSA_INFO21_REG : 0
 *			IVP_XTENSA_INFO22_REG : ivp_dbg_info
 *				IVP_IOVA
 *		return :
 *			IVP_XTENSA_INFO03_REG : IVP_DBG_CMD_DONE
 *
 */

/* the normal command of ivp firmware */
enum ivp_dbg_cmd {
	IVP_DBG_SET_CONTROL = MAKETAG(T_DBG, 0x01),
	IVP_DBG_GET_ERROR_MSG = MAKETAG(T_DBG, 0x02),
	IVP_PTRACE_SET_LEVEL = MAKETAG(T_DBG, 0x03),
	IVP_PTRACE_GET_MSG = MAKETAG(T_DBG, 0x04),
	IVP_DBG_CMD_DONE = MAKETAG(T_DBG, 0xff),
};

struct ivp_dbg_info {
	IVP_UINT32 control;
	IVP_UINT32 level;
	IVP_UINT32 ptrace_level;
	IVP_IOVA msg_buf_ptr;
	IVP_IOVA log_buffer_wptr;
	IVP_IOVA ptrace_log_buffer_wptr;
};

/* IVP_DBG_SET_CONTROL INFO21 */
#define FLAG_SET_CONTAOL		1
#define FLAG_SET_LEVEL			2

/* IVP_DBG_GET_ERROR_MSG INFO21 */
#define FLAG_GET_MSG_BUF_PTR		1
#define FLAG_GET_LOG_BUF_WRITE_PTR	2

/* debug command register index and counter */
#define DEBUG_CMD_REG_IDX		20
#define DEBUG_CMD_REG_COUNT		3

struct dbg_set_ctl_cmd {
	enum ivp_dbg_cmd debug_cmd;	/* IVP_XTENSA_INFO20_REG */
	IVP_UINT32 set_flags;		/* IVP_XTENSA_INFO21_REG */
	IVP_IOVA dbg_info_base;		/* IVP_XTENSA_INFO22_REG */
};

struct dbg_get_msg_cmd {
	enum ivp_dbg_cmd debug_cmd;	/* IVP_XTENSA_INFO20_REG */
	IVP_UINT32 get_flags;		/* IVP_XTENSA_INFO21_REG */
	IVP_IOVA dbg_info_base;		/* IVP_XTENSA_INFO22_REG */
};

/******************** IVP Boot information Define ********************/
/*
 * IVP_XTENSA_INFO28_REG : the flag of boot information
 * IVP_XTENSA_INFO29_REG : the size of ivp_boot_info
 * IVP_XTENSA_INFO30_REG : the address of ivp_boot_info
 *		IVP_IOVA
 * IVP_XTENSA_INFO31_REG : N/A
 *
 */

/* boot command register index and counter */
#define BOOT_CMD_REG_IDX		28
#define BOOT_CMD_REG_COUNT		4

enum ivp_uart_type {
	IVP_UART0,
	IVP_UART1,
	IVP_UART2,
	IVP_UART3,
	IVP_UART_NUM,
};

struct ivp_boot_cmd {		/* IVP_XTENSA_INFO30_REG */
	IVP_UINT32 log_buf_size;
	IVP_IOVA log_buf_base;
	IVP_UINT32 ptrace_log_buf_size;
	IVP_IOVA ptrace_log_buf_base;
	IVP_UINT32 excp_dump_buf_size;
	IVP_IOVA excp_dump_buf_base;
	IVP_UINT32 dbg_control;
	IVP_UINT32 dbg_level;
	IVP_UINT32 ptrace_level;
	IVP_UINT32 uart_base_idx;
};

#define BOOT_NORMAL_BOOT		1

#endif /* _IVP_INTERFACE_H */
