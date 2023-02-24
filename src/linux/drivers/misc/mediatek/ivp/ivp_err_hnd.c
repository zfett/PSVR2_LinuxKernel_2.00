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

#include "include/fw/ivp_error.h"
#include "include/fw/ivp_interface.h"
#include "include/fw/ivp_typedef.h"

#include "ivp_cmd_hnd.h"
#include "ivp_err_hnd.h"
#include "ivp_queue.h"
#include "ivp_reg.h"
#include "ivpbuf-dma-memcpy.h"

#define PS_SIZE 20

struct ivp_string {
	int no;
	char *string;
};

static int ivp_get_userframe(struct ivp_core *ivp_core)
{
	struct ivp_err_inform *ivp_err = ivp_core->ivp_err;
	int *uf_buf = (int *)ivp_core->working_buffer->excp_dump +
			EXCP_INFO_NUM;
	struct ivp_user_frame *uf = ivp_err->uf;
	int i = 0;

	uf->pc = uf_buf[i++];
	uf->ps = uf_buf[i++];
	uf->sar = uf_buf[i++];
	uf->vpri = uf_buf[i++];
	if (ivp_err->call0_abi)
		uf->a0 = uf_buf[i++];
	uf->a2 = uf_buf[i++];
	uf->a3 = uf_buf[i++];
	uf->a4 = uf_buf[i++];
	uf->a5 = uf_buf[i++];
	if (ivp_err->call0_abi) {
		uf->a6 = uf_buf[i++];
		uf->a7 = uf_buf[i++];
		uf->a8 = uf_buf[i++];
		uf->a9 = uf_buf[i++];
		uf->a10 = uf_buf[i++];
		uf->a11 = uf_buf[i++];
		uf->a12 = uf_buf[i++];
		uf->a13 = uf_buf[i++];
		uf->a14 = uf_buf[i++];
		uf->a15 = uf_buf[i++];
	}
	uf->exccause = uf_buf[i++];
	if (ivp_err->loops) {
		uf->lcount = uf_buf[i++];
		uf->lbeg = uf_buf[i++];
		uf->lend = uf_buf[i++];
	}

	return 0;
}

static int ivp_print_userframe(struct ivp_core *ivp_core)
{
	struct ivp_err_inform *ivp_err = ivp_core->ivp_err;
	struct ivp_user_frame *uf = ivp_err->uf;
	int ps[PS_SIZE];
	int i;

	/* decimal to binary */
	for (i = 0; i < PS_SIZE; i++)
		ps[i] = ((0x1 << i)&uf->ps)>>i;

	dev_err(ivp_core->dev, "user frame(uf) information\n");
	dev_err(ivp_core->dev, "uf->pc = 0x%08x\n", uf->pc);

	/* processor state parsing */
	dev_err(ivp_core->dev, "uf->ps = 0x%08x\n", uf->ps);
	dev_err(ivp_core->dev, "uf->ps.INTLEVEL = %d\n",
		ps[0] + (ps[1] << 1) + (ps[2] << 2) + (ps[3] << 3));
	dev_err(ivp_core->dev, "uf->ps.EXCM = %d\n", ps[4]);
	dev_err(ivp_core->dev, "uf->ps.UM = %d\n", ps[5]);
	dev_err(ivp_core->dev, "uf->ps.RING = %d\n", ps[6] + (ps[7] << 1));
	dev_err(ivp_core->dev, "uf->ps.OWB = %d\n",
		ps[8] + (ps[9] << 1) + (ps[10] << 2) + (ps[11] << 3));
	dev_err(ivp_core->dev, "uf->ps.CALLINC = %d\n", ps[16] +
		(ps[17] << 1));
	dev_err(ivp_core->dev, "uf->ps.WOE = %d\n", ps[18]);

	dev_err(ivp_core->dev, "uf->sar = 0x%08x\n", uf->sar);
	dev_err(ivp_core->dev, "uf->vpri = 0x%08x\n", uf->vpri);
	if (ivp_err->call0_abi)
		dev_err(ivp_core->dev, "uf->a0 = 0x%08x\n", uf->a0);
	dev_err(ivp_core->dev, "uf->a2 = 0x%08x\n", uf->a2);
	dev_err(ivp_core->dev, "uf->a3 = 0x%08x\n", uf->a3);
	dev_err(ivp_core->dev, "uf->a4 = 0x%08x\n", uf->a4);
	dev_err(ivp_core->dev, "uf->a5 = 0x%08x\n", uf->a5);
	if (ivp_err->call0_abi) {
		dev_err(ivp_core->dev, "uf->a6 = 0x%08x\n", uf->a6);
		dev_err(ivp_core->dev, "uf->a7 = 0x%08x\n", uf->a7);
		dev_err(ivp_core->dev, "uf->a8 = 0x%08x\n", uf->a8);
		dev_err(ivp_core->dev, "uf->a9 = 0x%08x\n", uf->a9);
		dev_err(ivp_core->dev, "uf->a10 = 0x%08x\n", uf->a10);
		dev_err(ivp_core->dev, "uf->a11 = 0x%08x\n", uf->a11);
		dev_err(ivp_core->dev, "uf->a12 = 0x%08x\n", uf->a12);
		dev_err(ivp_core->dev, "uf->a13 = 0x%08x\n", uf->a13);
		dev_err(ivp_core->dev, "uf->a14 = 0x%08x\n", uf->a14);
		dev_err(ivp_core->dev, "uf->a15 = 0x%08x\n", uf->a15);
	}
	dev_err(ivp_core->dev, "uf->exccause = 0x%08x\n", uf->exccause);
	if (ivp_err->loops) {
		dev_err(ivp_core->dev, "uf->lcount = 0x%08x\n", uf->lcount);
		dev_err(ivp_core->dev, "uf->lbeg = 0x%08x\n", uf->lbeg);
		dev_err(ivp_core->dev, "uf->lend = 0x%08x\n\n", uf->lend);
	}

	return 0;
}

static int ivp_print_stack(struct ivp_core *ivp_core)
{
	int *stack_buf = (int *)ivp_core->working_buffer->excp_dump +
			EXCP_INFO_NUM + UFRAME_INFO_NUM;
	int *excp_dump_buf_end = (int *)(ivp_core->working_buffer->excp_dump +
			IVP_FW_EXCEPTION_DUMP_BUFFER_SIZE);
	int i;

	dev_err(ivp_core->dev, "Stack:\n");

	for (i = 0;; i += 5) {
		if ((stack_buf[i] & 0xffff0000) == 0 ||
		    &stack_buf[i] > excp_dump_buf_end - 5)
			break;
		pr_err("0x%08x| 0x%08x 0x%08x 0x%08x 0x%08x\n",
			stack_buf[i], stack_buf[i + 1],
			stack_buf[i + 2], stack_buf[i + 3],
			stack_buf[i + 4]);
	}

	return 0;
}


static void ivp_get_error_inform(struct ivp_core *ivp_core)
{
	void __iomem *ivpctrl_base = ivp_core->ivpctrl_base;
	struct ivp_err_inform *ivp_err = ivp_core->ivp_err;
	int *excp_dump_buf = (int *)ivp_core->working_buffer->excp_dump;

	ivp_err->pc =
		(IVP_UINT32)ivp_read_reg32(ivpctrl_base,
					   IVP_CTRL_REG_DEBUG_INFO05);
	ivp_err->apmcu_cmd =
		(enum ivp_cmd)ivp_read_reg32(ivpctrl_base,
					     IVP_CTRL_REG_XTENSA_INFO16);
	ivp_err->arg0 =
		(enum ivp_cmd)ivp_read_reg32(ivpctrl_base,
					     IVP_CTRL_REG_XTENSA_INFO17);
	ivp_err->arg1 =
		(enum ivp_cmd)ivp_read_reg32(ivpctrl_base,
					     IVP_CTRL_REG_XTENSA_INFO18);
	ivp_err->arg2 =
		(enum ivp_cmd)ivp_read_reg32(ivpctrl_base,
					     IVP_CTRL_REG_XTENSA_INFO19);
	ivp_err->fw_state =
		(enum ivp_state)ivp_read_reg32(ivpctrl_base,
					       IVP_CTRL_REG_XTENSA_INFO00);
	ivp_err->cmd_result =
		(IVP_INT32)ivp_read_reg32(ivpctrl_base,
					  IVP_CTRL_REG_XTENSA_INFO01);
	ivp_err->algo_hnd =
		(IVP_UINT32)ivp_read_reg32(ivpctrl_base,
					   IVP_CTRL_REG_XTENSA_INFO02);
	ivp_err->pSP =
		(IVP_UINT32)ivp_read_reg32(ivpctrl_base,
					   IVP_CTRL_REG_XTENSA_INFO04);
	ivp_err->exccause =
		(IVP_UINT32)ivp_read_reg32(ivpctrl_base,
					   IVP_CTRL_REG_XTENSA_INFO05);
	ivp_err->uf_base =
		(IVP_UINT32)ivp_read_reg32(ivpctrl_base,
					   IVP_CTRL_REG_XTENSA_INFO06);
	ivp_err->call0_abi =
		(IVP_UINT32)ivp_read_reg32(ivpctrl_base,
					   IVP_CTRL_REG_XTENSA_INFO12);
	ivp_err->loops =
		(IVP_UINT32)ivp_read_reg32(ivpctrl_base,
					   IVP_CTRL_REG_XTENSA_INFO13);

	ivp_err->pSP = excp_dump_buf[IVP_EXCP_pSP];
	ivp_err->memctl = excp_dump_buf[IVP_EXCP_MEMCTL];
	ivp_err->ps = excp_dump_buf[IVP_EXCP_PS];
	ivp_err->windowbase = excp_dump_buf[IVP_EXCP_WBASE];
	ivp_err->windowstart = excp_dump_buf[IVP_EXCP_WSTART];
/*	ivp_err->exccause = excp_dump_buf[IVP_EXCP_EXCCAUSE];
 */
	ivp_err->excsave1 = excp_dump_buf[IVP_EXCP_EXCSAVE1];
	ivp_err->excvaddr = excp_dump_buf[IVP_EXCP_EXCVADDR];
	ivp_err->epc1 = excp_dump_buf[IVP_EXCP_EPC1];
	ivp_err->epc2 = excp_dump_buf[IVP_EXCP_EPC2];
	ivp_err->epc3 = excp_dump_buf[IVP_EXCP_EPC3];
	ivp_err->epc4 = excp_dump_buf[IVP_EXCP_EPC4];
	ivp_err->eps2 = excp_dump_buf[IVP_EXCP_EPS2];
	ivp_err->eps3 = excp_dump_buf[IVP_EXCP_EPS3];
	ivp_err->eps4 = excp_dump_buf[IVP_EXCP_EPS4];

	ivp_get_userframe(ivp_core);
}

static int ivp_get_str_idx(struct ivp_string *ivp_string, int num)
{
	int idx;

	for (idx = 0; ; idx++) {
		if (ivp_string[idx].no == num)
			return idx;
		if (ivp_string[idx].no == -1)
			return -EINVAL;
	}
}

int ivp_get_error_report(struct ivp_core *ivp_core,
			 struct ivp_request *req) {
	struct ivp_err_inform *ivp_err;
	struct ivp_user *ivp_user = req->user;
	struct ivp_algo_context *algo_ctx;
	struct ivp_string exc_err_string[] = {
		{ EXCCAUSE_ILLEGAL,
			"Illegal Instruction" },
		{ EXCCAUSE_SYSCALL,
			"System Call" },
		{ EXCCAUSE_INSTR_ERROR,
			"Instruction Fetch Error" },
		{ EXCCAUSE_LOAD_STORE_ERROR,
			"Load Store Error" },
		{ EXCCAUSE_LEVEL1_INTERRUPT,
			"Level 1 Interrupt" },
		{ EXCCAUSE_ALLOCA,
			"Stack Extension Assist for alloca" },
		{ EXCCAUSE_DIVIDE_BY_ZERO,
			"Integer Divided by Zero" },
		{ EXCCAUSE_SPECULATION,
			"Use of Failed Speculative Access" },
		{ EXCCAUSE_PRIVILEGED,
			"Privileged Instruction" },
		{ EXCCAUSE_UNALIGNED,
			"Unaligned Load or Store" },
		{ EXCCAUSE_INSTR_DATA_ERROR,
			"PIF Data Error on Instruction Fetch" },
		{ EXCCAUSE_LOAD_STORE_DATA_ERROR,
			"PIF Data Error on Load or Store" },
		{ EXCCAUSE_INSTR_ADDR_ERROR,
			"PIF Address Error on Instruction Fetch" },
		{ EXCCAUSE_LOAD_STORE_ADDR_ERROR,
			"PIF Address Error on Load or Store" },
		{ EXCCAUSE_ITLB_MISS,
			"ITLB Miss" },
		{ EXCCAUSE_ITLB_MULTIHIT,
			"ITLB Multihit" },
		{ EXCCAUSE_INSTR_RING,
			"Ring Privilege Violation on Instruction Fetch" },
		{ EXCCAUSE_INSTR_PROHIBITED,
			"Cache Attribute does not allow Instruction Fetch" },
		{ EXCCAUSE_DTLB_MISS,
			"DTLB Miss" },
		{ EXCCAUSE_DTLB_MULTIHIT,
			"DTLB Multihit" },
		{ EXCCAUSE_LOAD_STORE_RING,
			"Ring Privilege Violation on Load or Store" },
		{ EXCCAUSE_LOAD_PROHIBITED,
			"Cache Attribute does not allow Load" },
		{ EXCCAUSE_STORE_PROHIBITED,
			"Cache Attribute does not allow Store" },
		{ EXCCAUSE_DOUBLE_EXCEPTION,
			"Double Exception" },
		{ EXCCAUSE_DEBUG_EXCEPTION,
			"Debug Exception" },
		{ -1, ""},
	};
	struct ivp_string cmd_string[] = {
		{ CMD_IDLE, "Idle" },
		{ CMD_LOAD_PROC, "Load process" },
		{ CMD_EXEC_PROC, "Execute process" },
		{ CMD_UNLOAD_PROC, "Unload process" },
		{ CMD_GET_IVPINFO, "Get IVP information" },
		{ CMD_SET_WAKEUP, "Set wakeup" },
		{ CMD_EXIT, "Exit" },
		{ -1, ""},
	};
	int idx;

	algo_ctx = idr_find(&ivp_user->algo_idr[ivp_core->core],
			    req->drv_algo_hnd);
	if (!algo_ctx) {
		pr_err("%s: fail to get algo_ctx\n", __func__);
		return -EINVAL;
	}

	ivp_get_error_inform(ivp_core);
	ivp_err = ivp_core->ivp_err;

	dev_err(ivp_core->dev, "ivp_error_report_conclusion:\n");
	idx = ivp_get_str_idx(exc_err_string, ivp_err->exccause);
	if (idx >= 0) {
		dev_err(ivp_core->dev, "Exception cause: %s\n",
			exc_err_string[idx].string);
	} else
		dev_err(ivp_core->dev, "Exception cause: Unknown error\n");
	idx = ivp_get_str_idx(cmd_string, ivp_err->apmcu_cmd);
	if (idx >= 0) {
		dev_err(ivp_core->dev, "Exception cmd: %s(0x%08x)\n",
			cmd_string[idx].string, ivp_err->apmcu_cmd);
	} else {
		dev_err(ivp_core->dev, "Exception cmd: Unknown cmd(0x%08x)\n",
			ivp_err->apmcu_cmd);
	}
	dev_err(ivp_core->dev, "Exception pc: 0x%08x\n\n", ivp_err->uf->pc);

	dev_err(ivp_core->dev, "ivp_error_report_detail:\n");

	dev_err(ivp_core->dev, "============ Kernel space ============\n");
	dev_err(ivp_core->dev, "apmcu_cmd = 0x%08x\n", ivp_err->apmcu_cmd);
	dev_err(ivp_core->dev, "cmd_result = 0x%08x\n", ivp_err->cmd_result);
	switch (ivp_err->apmcu_cmd) {
	case CMD_LOAD_PROC:
		dev_err(ivp_core->dev, "load_cmd.flags = 0x%08x(%s)\n",
			req->argv, req->argv ? "fix" : "dyn");
		dev_err(ivp_core->dev, "load_cmd.algo_size = 0x%08x\n",
			req->size);
		dev_err(ivp_core->dev, "load_cmd.algo_base = 0x%08x\n",
			req->addr_handle);
		dev_err(ivp_core->dev, "load_cmd.code_mem = 0x%08x\n",
			algo_ctx->algo_code_mem);
		dev_err(ivp_core->dev, "load_cmd.data_mem = 0x%08x\n",
			algo_ctx->algo_data_mem);
		break;
	case CMD_EXEC_PROC:
		dev_err(ivp_core->dev, "exec_cmd.algo_hnd = 0x%08x\n",
			algo_ctx->fw_algo_hnd);
		dev_err(ivp_core->dev, "exec_cmd.flags = 0x%08x(%s)\n",
			algo_ctx->algo_flags,
			algo_ctx->algo_flags ? "fix" : "dyn");
		dev_err(ivp_core->dev, "exec_cmd.algo_arg_size = 0x%08x\n",
			req->size);
		dev_err(ivp_core->dev, "exec_cmd.algo_arg_base = 0x%08x\n",
			req->addr_handle);
		dev_err(ivp_core->dev, "exec_cmd.code_mem = 0x%08x\n",
			algo_ctx->algo_code_mem);
		dev_err(ivp_core->dev, "exec_cmd.data_mem = 0x%08x\n",
			algo_ctx->algo_data_mem);
		break;
	case CMD_UNLOAD_PROC:
		dev_err(ivp_core->dev, "unload_cmd.algo_hnd = 0x%08x\n",
			algo_ctx->fw_algo_hnd);
		dev_err(ivp_core->dev, "unload_cmd.flags = 0x%08x(%s)\n",
			algo_ctx->algo_flags,
			algo_ctx->algo_flags ? "fix" : "dyn");
		dev_err(ivp_core->dev, "unload_cmd.code_mem = 0x%08x\n",
			algo_ctx->algo_code_mem);
		dev_err(ivp_core->dev, "unload_cmd.data_mem = 0x%08x\n",
			algo_ctx->algo_data_mem);
		break;
	default:
		break;
	}

	dev_err(ivp_core->dev, "============ DSP space ============\n");
	dev_err(ivp_core->dev, "apmcu_cmd = 0x%08x\n", ivp_err->apmcu_cmd);
	switch (ivp_err->apmcu_cmd) {
	case CMD_LOAD_PROC:
		dev_err(ivp_core->dev, "load_cmd.flags = 0x%08x\n",
			ivp_err->arg0);
		dev_err(ivp_core->dev, "load_cmd.algo_size = 0x%08x\n",
			ivp_err->arg1);
		dev_err(ivp_core->dev, "load_cmd.algo_base = 0x%08x\n",
			ivp_err->arg2);
		break;
	case CMD_EXEC_PROC:
		dev_err(ivp_core->dev, "exec_cmd.algo_hnd = 0x%08x\n",
			ivp_err->arg0);
		dev_err(ivp_core->dev, "exec_cmd.algo_arg_size = 0x%08x\n",
			ivp_err->arg1);
		dev_err(ivp_core->dev, "exec_cmd.algo_arg_base = 0x%08x\n",
			ivp_err->arg2);
		break;
	case CMD_UNLOAD_PROC:
		dev_err(ivp_core->dev, "unload_cmd.algo_hnd = 0x%08x\n",
			ivp_err->arg0);
		break;
	default:
		break;
	}
	dev_err(ivp_core->dev, "cmd_rtn.fw_state = 0x%08x\n",
		ivp_err->fw_state);
	dev_err(ivp_core->dev, "cmd_rtn.cmd_result = 0x%08x\n",
		ivp_err->cmd_result);
	dev_err(ivp_core->dev, "cmd_rtn.algo_hnd = 0x%08x\n",
		ivp_err->algo_hnd);
	dev_err(ivp_core->dev, "pc = 0x%08x\n", ivp_err->pc);
	dev_err(ivp_core->dev, "pSP = 0x%08x\n", ivp_err->pSP);
	dev_err(ivp_core->dev, "exccause = 0x%08x\n", ivp_err->exccause);
	dev_err(ivp_core->dev, "memctl = 0x%08x\n", ivp_err->memctl);
	dev_err(ivp_core->dev, "ps = 0x%08x\n", ivp_err->ps);
	dev_err(ivp_core->dev, "windowbase = 0x%08x\n", ivp_err->windowbase);
	dev_err(ivp_core->dev, "windowstart = 0x%08x\n", ivp_err->windowstart);
	dev_err(ivp_core->dev, "excsave1 = 0x%08x\n", ivp_err->excsave1);
	dev_err(ivp_core->dev, "excvaddr = 0x%08x\n", ivp_err->excvaddr);
	dev_err(ivp_core->dev, "epc1 = 0x%08x\n", ivp_err->epc1);
	dev_err(ivp_core->dev, "epc2 = 0x%08x\n", ivp_err->epc2);
	dev_err(ivp_core->dev, "epc3 = 0x%08x\n", ivp_err->epc3);
	dev_err(ivp_core->dev, "epc4 = 0x%08x\n", ivp_err->epc4);
	dev_err(ivp_core->dev, "eps2 = 0x%08x\n", ivp_err->eps2);
	dev_err(ivp_core->dev, "eps3 = 0x%08x\n", ivp_err->eps3);
	dev_err(ivp_core->dev, "eps4 = 0x%08x\n", ivp_err->eps4);
	dev_err(ivp_core->dev, "uf_base = 0x%08x\n\n", ivp_err->uf_base);

	ivp_print_userframe(ivp_core);

	ivp_print_stack(ivp_core);

	return 0;
}

void ivp_excp_handler(struct ivp_core *ivp_core,
		     struct ivp_request *req) {
	void __iomem *ivpctrl_base = ivp_core->ivpctrl_base;
	struct ivp_user *ivp_user = req->user;
	struct ivp_device *ivp_device = ivp_core->ivp_device;
	struct ivp_err_inform *ivp_err = ivp_core->ivp_err;

	switch (ivp_core->exp_cmd) {
	case IVP_RST_NEXT_CMD:
		ivp_reset_algo_ctx(ivp_core);
		ivp_reset_hw(ivp_core);
		ivp_core->state = IVP_IDLE;
		if (ivp_err->apmcu_cmd == CMD_LOAD_PROC)
			ivp_load_algo(req, ivp_device);
		else if (ivp_err->apmcu_cmd == CMD_EXEC_PROC) {
			struct ivp_algo_context *algo_ctx =
				idr_find(&ivp_user->algo_idr[ivp_core->core],
					 req->drv_algo_hnd);

			if (!algo_ctx) {
				pr_err("%s: fail to get algo_ctx\n", __func__);
				return;
			}
			ivp_reload_algo(ivp_device, req);
			algo_ctx->state = IVP_ALGO_STATE_READY;
		}
		break;
	case IVP_RST_RETRY:
		ivp_reset_hw(ivp_core);

		if (ivp_err->apmcu_cmd == CMD_LOAD_PROC)
			ivp_load_algo(req, ivp_device);
		else if (ivp_err->apmcu_cmd == CMD_EXEC_PROC) {
			mutex_lock(&ivp_device->user_mutex);
			ivp_reset_algo_ctx(ivp_core);
			mutex_unlock(&ivp_device->user_mutex);
			ivp_exec_algo(req, ivp_device);
		} else if (ivp_err->apmcu_cmd == CMD_UNLOAD_PROC)
			ivp_unload_algo(req, ivp_device);
		break;
	case IVP_RST_ONLY:
		ivp_reset_hw(ivp_core);
		ivp_core->state = IVP_IDLE;

		if (ivp_err->apmcu_cmd == CMD_EXEC_PROC ||
		    ivp_err->apmcu_cmd == CMD_UNLOAD_PROC) {
			ivp_reset_algo_ctx(ivp_core);
		}
		break;
	case IVP_CONTINUE:
		ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO14,
				0xffff0001);
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
		prev_pc[ivp_core->core] = (IVP_UINT32)ivp_read_reg32(
			ivp_core->ivpctrl_base, IVP_CTRL_REG_DEBUG_INFO05);
		prev_fw_state[ivp_core->core] = (IVP_UINT32)ivp_read_reg32(
			ivp_core->ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO00);
#endif
		ivp_set_reg32(ivpctrl_base, IVP_CTRL_REG_CTL_XTENSA_INT,
			      (0x1 << INT00_LEVEL_L1));
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
		int_count[ivp_core->core]++;
#endif
		break;
	default:
		pr_warn("%s: bad command!\n", __func__);
		break;
	}
}

