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

#ifndef _MSDC_IO_H_
#define _MSDC_IO_H_

/**************************************************************/
/* Section 1: Device Tree                                     */
/**************************************************************/
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>


extern const struct of_device_id msdc_of_ids[];
extern unsigned int cd_gpio;

int msdc_dt_init(struct platform_device *pdev, struct mmc_host *mmc);

/**************************************************************/
/* Section 2: Power                                           */
/**************************************************************/
void msdc_set_host_power_control(struct msdc_host *host);
#if !defined(CONFIG_MACH_FPGA)
void msdc_dump_ldo_sts(struct msdc_host *host);
void msdc_HQA_set_vcore(struct msdc_host *host);
#else
#define msdc_dump_ldo_sts(host)
#endif

/**************************************************************/
/* Section 3: Clock                                           */
/**************************************************************/
#include <linux/clk.h>

#if defined(CONFIG_MACH_FPGA)
extern  u32 hclks_msdc[];
#define msdc_dump_clock_sts(host)
#define msdc_get_hclk(host, src)        MSDC_SRC_FPGA
static inline int msdc_clk_enable(struct msdc_host *host) { return 0; }
#define msdc_clk_disable(host)
#define msdc_get_ccf_clk_pointer(pdev, host) (0)
#endif

#if !defined(CONFIG_MACH_FPGA)
extern u32 *hclks_msdc_all[];
void msdc_dump_clock_sts(struct msdc_host *host);
#define msdc_get_hclk(id, src)		hclks_msdc_all[id][src]
#define msdc_clk_enable(host)		clk_enable(host->clock_control)
#define msdc_clk_disable(host)		clk_disable(host->clock_control)
int msdc_get_ccf_clk_pointer(struct platform_device *pdev,
	struct msdc_host *host);
#endif

/**************************************************************/
/* Section 4: GPIO and Pad                                    */
/**************************************************************/
/*******************************************************************************
 *PINMUX and GPIO definition
 ******************************************************************************/
#define MSDC_PIN_PULL_NONE      (0)
#define MSDC_PIN_PULL_DOWN      (1)
#define MSDC_PIN_PULL_UP        (2)
#define MSDC_PIN_KEEP           (3)

#define MSDC_TDRDSEL_SLEEP      (0)
#define MSDC_TDRDSEL_3V         (1)
#define MSDC_TDRDSEL_1V8        (2)
#define MSDC_TDRDSEL_CUST       (3)

#ifndef CONFIG_MACH_FPGA
void msdc_set_io_driving(struct msdc_hw_driving *driving);
void msdc_get_io_driving(struct msdc_hw_driving *driving);
void msdc_set_io_ies(int set_ies);
void msdc_set_io_smt(int set_smt);
void msdc_set_io_tdsel(u32 flag, u32 value);
void msdc_set_io_rdsel(u32 flag, u32 value);
void msdc_get_io_tdsel(u32 *value);
void msdc_get_io_rdsel(u32 *value);
void msdc_dump_io_padctl(void);
void msdc_pin_io_config(u32 mode);
void msdc_set_pin_mode(struct msdc_host *host);
#else
#define msdc_set_io_driving(driving)
#define msdc_get_io_driving(driving)
#define msdc_set_io_ies(set_ies)
#define msdc_set_io_smt(set_smt)
#define msdc_set_io_tdsel(flag, value)
#define msdc_set_io_rdsel(flag, value)
#define msdc_get_io_tdsel(value)
#define msdc_get_io_rdsel(value)
#define msdc_dump_io_padctl()
#define msdc_pin_io_config(mode)
#define msdc_set_pin_mode(host)
#endif

#define msdc_get_driving(host, driving) \
	msdc_get_io_driving(driving)

#define msdc_set_driving(host, driving) \
	msdc_set_io_driving(driving)

#define msdc_set_ies(host, set_ies) \
	msdc_set_io_ies(set_ies)

#define msdc_set_smt(host, set_smt) \
	msdc_set_io_smt(set_smt)

#define msdc_set_tdsel(host, flag, value) \
	msdc_set_io_tdsel(flag, value)

#define msdc_set_rdsel(host, flag, value) \
	msdc_set_io_rdsel(flag, value)

#define msdc_get_tdsel(host, value) \
	msdc_get_io_tdsel(value)

#define msdc_get_rdsel(host, value) \
	msdc_get_io_rdsel(value)

#define msdc_dump_padctl(host) \
	msdc_dump_io_padctl()

#define msdc_pin_config(host, mode) \
	msdc_pin_io_config(mode)

#endif /* end of _MSDC_IO_H_ */
