/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: JB Tsai <jb.tsai@mediatek.com>
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

#ifndef __IVP_FW_INTERFACE_H__
#define __IVP_FW_INTERFACE_H__

#include "include/fw/ivp_interface.h"

void ivp_release_fw(void __iomem *ivpctrl_base);
void ivp_halt_fw(void __iomem *ivpctrl_base);

int ivp_fw_get_ivp_status(void __iomem *ivpctrl_base, u32 *state);

void ivp_fw_boot_sequence(void __iomem *ivpctrl_base,
			  struct ivp_core *ivp_core, u32 boot_flags);

void ivp_fw_load_algo(struct apmcu_load_cmd *load_cmd,
		      void __iomem *ivpctrl_base);
void ivp_fw_load_algo_rtn(struct apmcu_load_cmd_rtn *load_cmd_rtn,
			  void __iomem *ivpctrl_base);

void ivp_fw_exec_algo(struct apmcu_exec_cmd *exec_cmd,
		      void __iomem *ivpctrl_base);
void ivp_fw_exec_algo_rtn(struct apmcu_exec_cmd_rtn *exec_cmd_rtn,
			  void __iomem *ivpctrl_base);

void ivp_fw_unload_algo(struct apmcu_unload_cmd *unload_cmd,
			void __iomem *ivpctrl_base);
void ivp_fw_unload_algo_rtn(struct apmcu_unload_cmd_rtn *unload_cmd_rtn,
			    void __iomem *ivpctrl_base);

void ivp_fw_getinfo(struct apmcu_getinfo_cmd *getinfo_cmd,
		    void __iomem *ivpctrl_base);
void ivp_fw_getinfo_rtn(struct apmcu_getinfo_cmd_rtn *getinfo_cmd_rtn,
			void __iomem *ivpctrl_base);

void ivp_fw_set_dbg_ctl(struct dbg_set_ctl_cmd *set_dbg_ctl,
			void __iomem *ivpctrl_base);
void ivp_fw_get_dbg_msg(struct dbg_get_msg_cmd *get_dbg_msg,
			void __iomem *ivpctrl_base);
void ivp_fw_dbg_rtn(int *dbg_rtn, void __iomem *ivpctrl_base);


#endif /* __IVP_FW_INTERFACE_H__ */
