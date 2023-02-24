/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

/**
 * @file Msdc_fpga_io.c
 * The Msdc_fpga_io.c file include function for eMMC host controller clock \n
 * control in fpga platform.
 */

#ifdef pr_fmt
#undef pr_fmt
#endif

#define pr_fmt(fmt) "["KBUILD_MODNAME"]" fmt
#include <linux/io.h>
#include "mtk_sd.h"

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Select eMMC host clock frequency.
 * @param *host: msdc host structure.
 * @param clksrc: target frequency of host clock.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
#ifdef CONFIG_MACH_FPGA
void msdc_select_clksrc(struct msdc_host *host, int clksrc)
{
	host->hclk = msdc_get_hclk(host->id, clksrc);
	host->hw->clk_src = clksrc;

	pr_err("[%s]: msdc%d select clk_src as %d(%dKHz)\n", __func__,
		host->id, clksrc, host->hclk/1000);
}
#endif

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Check eMMC host clock status.
 * @param *status: eMMC host clock status.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
#ifdef CONFIG_MACH_FPGA
void msdc_clk_status(int *status)
{

}
#endif
