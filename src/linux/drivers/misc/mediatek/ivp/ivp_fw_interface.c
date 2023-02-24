/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: PoChun Lin <pochun.lin@mediatek.com>
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

/**
 * @file ivp_fw_interface.c
 * Handle the information of IVP firmware.
 */

#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/ktime.h>
#include <linux/wait.h>

#include <uapi/mediatek/ivp_ioctl.h>

#include "ivp_driver.h"
#include "ivp_reg.h"

#include "include/fw/ivp_dbg_ctrl.h"
#include "include/fw/ivp_error.h"
#include "include/fw/ivp_interface.h"
#include "include/fw/ivp_macro.h"
#include "include/fw/ivp_typedef.h"

void ivp_release_fw(void __iomem *ivpctrl_base)
{
	ivp_clear_reg32(ivpctrl_base, IVP_CTRL_REG_CTRL, RUN_STALL);
}

void ivp_halt_fw(void __iomem *ivpctrl_base)
{
	ivp_set_reg32(ivpctrl_base, IVP_CTRL_REG_CTRL, RUN_STALL);
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     Use this function to get ivp status.
 * @param[in]
 *     ivpctrl_base: pointer to the base address of IVP contral.
 * @param[out]
 *     state: denote the ivp state.
 * @return
 *     0, successful to get ivp status.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int ivp_fw_get_ivp_status(void __iomem *ivpctrl_base, u32 *state)
{
	*state =
	    (u32)ivp_read_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO00);
	pr_debug("%s: state=%x\n", __func__, *state);

	return 0;
}

void ivp_fw_boot_sequence(void __iomem *ivpctrl_base,
			  struct ivp_core *ivp_core, u32 boot_flags)
{
	u32 fw_entry = ivp_core->ivp_entry;
	u32 boot_cmd_entry = ivp_core->working_buffer_iova +
		offsetof(struct ivp_working_buf, boot_cmd);
	int delay_usec = 10;

	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INT, IVP_INT_XTENSA);

	/* need before reset */
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_ALTRESETVEC,
			fw_entry);	/* set SRAM_BASE 0x5000000 */

	/* IVP boot sequence */
	ivp_set_reg32(ivpctrl_base, IVP_CTRL_REG_CTRL,
		      P_DEBUG_ENABLE | STAT_VECTOR_SEL | PBCLK_EN);
		      /* set boot from IVP_XTENSA_ALTRESETVEC */

	ivp_set_reg32(ivpctrl_base, IVP_CTRL_REG_RESET, VPU_OCD_HALT_ON_RST);
	udelay(delay_usec);
	ivp_clear_reg32(ivpctrl_base, IVP_CTRL_REG_RESET, VPU_OCD_HALT_ON_RST);

	ivp_set_reg32(ivpctrl_base, IVP_CTRL_REG_RESET, VPU_B_RST);
	ivp_set_reg32(ivpctrl_base, IVP_CTRL_REG_RESET, VPU_D_RST);
	udelay(delay_usec);
	ivp_clear_reg32(ivpctrl_base, IVP_CTRL_REG_RESET, VPU_B_RST);
	ivp_clear_reg32(ivpctrl_base, IVP_CTRL_REG_RESET, VPU_D_RST);
	udelay(delay_usec);

	ivp_clear_reg32(ivpctrl_base, IVP_CTRL_REG_CTRL, BUS_PIF_GATED);
		/* pif gated disable, To prevent unknown propagate to BUS */

	udelay(delay_usec);

	/* write the boot command info */
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO28, boot_flags);
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO29,
			sizeof(struct ivp_boot_cmd));
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO30,
			boot_cmd_entry);
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO31, 0);

	/* iommu enable (default0 & default1) */
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_AXI_DEFAULT0,
			(0x10 << 18) | (0x10 << 23));
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_AXI_DEFAULT0 + 0x4,
			(0x10 << 0) | (0x10 << 5));

	/* jtag enable */
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_AXI_DEFAULT3,
		DBG_EN | NIDEN | SPIDEN | SPNIDEN);
}

void ivp_fw_load_algo(struct apmcu_load_cmd *load_cmd,
		      void __iomem *ivpctrl_base)
{
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO16,
			load_cmd->apmcu_cmd);
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO17,
			load_cmd->flags);
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO18,
			load_cmd->algo_size);
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO19,
			load_cmd->algo_base);
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO15,
			load_cmd->timestamp);

	ivp_set_reg32(ivpctrl_base, IVP_CTRL_REG_CTL_XTENSA_INT,
		      (0x1 << INT00_LEVEL_L1));
}

void ivp_fw_load_algo_rtn(struct apmcu_load_cmd_rtn *load_cmd_rtn,
			  void __iomem *ivpctrl_base)
{
	load_cmd_rtn->fw_state =
	    (enum ivp_state)ivp_read_reg32(ivpctrl_base,
					   IVP_CTRL_REG_XTENSA_INFO00);
	load_cmd_rtn->cmd_result =
	    (IVP_INT32)ivp_read_reg32(ivpctrl_base,
				       IVP_CTRL_REG_XTENSA_INFO01);
	load_cmd_rtn->algo_hnd =
	    (IVP_UINT32)ivp_read_reg32(ivpctrl_base,
					IVP_CTRL_REG_XTENSA_INFO02);
	load_cmd_rtn->code_mem =
	    (IVP_UINT32)ivp_read_reg32(ivpctrl_base,
					IVP_CTRL_REG_XTENSA_INFO08);
	load_cmd_rtn->data_mem =
	    (IVP_UINT32)ivp_read_reg32(ivpctrl_base,
					IVP_CTRL_REG_XTENSA_INFO09);
}

void ivp_fw_exec_algo(struct apmcu_exec_cmd *exec_cmd,
		      void __iomem *ivpctrl_base)
{
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO16,
			exec_cmd->apmcu_cmd);
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO17,
			exec_cmd->algo_hnd);
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO18,
			exec_cmd->algo_arg_size);
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO19,
			exec_cmd->algo_arg_base);
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO15,
			exec_cmd->timestamp);

	ivp_set_reg32(ivpctrl_base, IVP_CTRL_REG_CTL_XTENSA_INT,
		      (0x1 << INT00_LEVEL_L1));
}

void ivp_fw_exec_algo_rtn(struct apmcu_exec_cmd_rtn *exec_cmd_rtn,
			  void __iomem *ivpctrl_base)
{
	exec_cmd_rtn->fw_state =
	    (enum ivp_state)ivp_read_reg32(ivpctrl_base,
					   IVP_CTRL_REG_XTENSA_INFO00);
	exec_cmd_rtn->cmd_result =
	    (IVP_INT32)ivp_read_reg32(ivpctrl_base,
				       IVP_CTRL_REG_XTENSA_INFO01);
}

void ivp_fw_unload_algo(struct apmcu_unload_cmd *unload_cmd,
			void __iomem *ivpctrl_base)
{
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO16,
			unload_cmd->apmcu_cmd);
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO17,
			unload_cmd->algo_hnd);
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO15,
			unload_cmd->timestamp);

	ivp_set_reg32(ivpctrl_base, IVP_CTRL_REG_CTL_XTENSA_INT,
		      (0x1 << INT00_LEVEL_L1));
}

void ivp_fw_unload_algo_rtn(struct apmcu_unload_cmd_rtn *unload_cmd_rtn,
			    void __iomem *ivpctrl_base)
{
	unload_cmd_rtn->fw_state =
	    (enum ivp_state)ivp_read_reg32(ivpctrl_base,
					   IVP_CTRL_REG_XTENSA_INFO00);
	unload_cmd_rtn->cmd_result =
	    (IVP_INT32)ivp_read_reg32(ivpctrl_base,
				       IVP_CTRL_REG_XTENSA_INFO01);
}

void ivp_fw_getinfo(struct apmcu_getinfo_cmd *getinfo_cmd,
		    void __iomem *ivpctrl_base)
{
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO16,
			getinfo_cmd->apmcu_cmd);
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO17,
			getinfo_cmd->flags);
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO15,
			getinfo_cmd->timestamp);

	ivp_set_reg32(ivpctrl_base, IVP_CTRL_REG_CTL_XTENSA_INT,
		      (0x1 << INT00_LEVEL_L1));
}

void ivp_fw_getinfo_rtn(struct apmcu_getinfo_cmd_rtn *getinfo_cmd_rtn,
			void __iomem *ivpctrl_base)
{
	getinfo_cmd_rtn->fw_state =
	    (enum ivp_state)ivp_read_reg32(ivpctrl_base,
					   IVP_CTRL_REG_XTENSA_INFO00);
	getinfo_cmd_rtn->cmd_result =
	    (IVP_INT32)ivp_read_reg32(ivpctrl_base,
				       IVP_CTRL_REG_XTENSA_INFO01);
	getinfo_cmd_rtn->fw_info =
	    (IVP_INT32)ivp_read_reg32(ivpctrl_base,
				       IVP_CTRL_REG_XTENSA_INFO02);
}

void ivp_fw_set_dbg_ctl(struct dbg_set_ctl_cmd *set_dbg_ctl,
			void __iomem *ivpctrl_base)
{
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO20,
			set_dbg_ctl->debug_cmd);
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO21,
			set_dbg_ctl->set_flags);
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO22,
			set_dbg_ctl->dbg_info_base);

	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO03, 0);

	ivp_set_reg32(ivpctrl_base, IVP_CTRL_REG_CTL_XTENSA_INT,
		      (0x1 << INT02_LEVEL_L1));
}

void ivp_fw_get_dbg_msg(struct dbg_get_msg_cmd *get_dbg_msg,
			void __iomem *ivpctrl_base)
{
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO20,
			get_dbg_msg->debug_cmd);
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO21,
			get_dbg_msg->get_flags);
	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO22,
			get_dbg_msg->dbg_info_base);

	ivp_write_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO03, 0);

	ivp_set_reg32(ivpctrl_base, IVP_CTRL_REG_CTL_XTENSA_INT,
		      (0x1 << INT02_LEVEL_L1));
}

void ivp_fw_dbg_rtn(int *dbg_rtn, void __iomem *ivpctrl_base)
{
	*dbg_rtn = (enum ivp_dbg_cmd)ivp_read_reg32(ivpctrl_base,
					   IVP_CTRL_REG_XTENSA_INFO03);
}
