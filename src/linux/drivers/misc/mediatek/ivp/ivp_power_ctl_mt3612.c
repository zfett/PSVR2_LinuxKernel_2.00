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

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/regulator/consumer.h>

#if defined(CONFIG_MTK_PMICW_P2P)
#include <soc/mediatek/mtk_pmic_p2p.h>
#endif

/* ivp fw interface header files*/
#include "include/fw/ivp_dbg_ctrl.h"
#include "include/fw/ivp_error.h"
#include "include/fw/ivp_interface.h"
#include "include/fw/ivp_macro.h"
#include "include/fw/ivp_typedef.h"

/* ivp driver header files */
#include "ivp_cmd_hnd.h"
#include "ivp_fw_interface.h"
#include "ivp_opp_ctl.h"
#include "ivp_power_ctl.h"
#include "ivp_reg.h"

#define IVP_SRAM_MIN_VOLT	850000
#define IVP_VOLT_DELTA		100000

#define IVP_DEFAULT_VOLT	650000
#define IVP_DEFAULT_VOLT_INDEX	5	/* 0.65V */
#define IVP_DSP_SEL_INDEX	5	/* 312MHz */
#define IVP_IPU_IF_SEL_INDEX	5	/* 312MHz */
#define IVP_REG_DELTA		5000
#define IVP_MV_TO_UV		1000

struct voltage_control {
	int index;
	unsigned long vivp;
	unsigned long vivp_adjust;
	unsigned long vsram_adjust;
};

#define NUM_VOLTAGE 5
struct voltage_control volt_ctrl[NUM_VOLTAGE] = {
	{0, 600000, 607500,  850000},
	{1, 650000, 658125,  850000},
	{2, 700000, 708750,  850000},
	{3, 800000, 810000,  910000},
	{4, 850000, 860625,  960625},
};

static const char * const core_cg_names[] = {
	"ivp_core_axi_cg",
	"ivp_core_vpu_cg",
	"ivp_core_jtag_cg",
};

static const char * const clk_names[] = {
	"ivp_dsp_sel",
	"ivp_ipu_if_sel",
};

static const char * const pll_names_dsp[] = {
	"ivp_clk26m",
	"ivp_mmpll1_d8",
	"ivp_univpll1_d4",
	"ivp_mmpll1_d4",
	"ivp_univpll_d5",
	"ivp_univpll1_d2",
	"ivp_syspll_d3",
	"ivp_univpll_d3",
	"ivp_mmpll_d7",
	"ivp_vpupll1_d2",
	"ivp_syspll_d2",
	"ivp_vpupll_d5",
	"ivp_mmpll_d5",
	"ivp_vpupll_d4",
};

static const char * const * const pll_names[] = {
	pll_names_dsp,
	pll_names_dsp,
};

static const int pll_nums = ARRAY_SIZE(pll_names_dsp);

#define IVP_CORE_CG_NUM		ARRAY_SIZE(core_cg_names)

int ivp_setup_clk_resource(struct device *dev, struct ivp_device *ivp_device)
{
#if defined(CONFIG_COMMON_CLK_MT3612)
	int i, j;
	int *clk_ref;

	for (i = 0; i < IVP_CLK_NUM; i++) {
		ivp_device->clk_sel[i] = devm_clk_get(dev,
						clk_names[i]);
		if (IS_ERR(ivp_device->clk_sel)) {
			dev_err(dev, "cannot get %s\n", clk_names[i]);
			return -ENOENT;
		}
		ivp_device->clk_name[i] = clk_names[i];
		ivp_device->clk_num[i] = pll_nums;

		ivp_device->pll_data[i] = devm_kcalloc(dev,
					    ivp_device->clk_num[i],
					    sizeof(*ivp_device->pll_data[i]),
					    GFP_KERNEL);
		if (IS_ERR(ivp_device->pll_data[i]))
			return -ENOMEM;

		for (j = 0; j < ivp_device->clk_num[i]; j++) {
			ivp_device->pll_data[i][j].clk = devm_clk_get(dev,
							 pll_names[i][j]);
			if (IS_ERR(ivp_device->pll_data[i][j].clk)) {
				dev_err(dev, "cannot get clk %s\n",
					pll_names[i][j]);
				return -ENOENT;
			}
			ivp_device->pll_data[i][j].clk_rate =
				clk_get_rate(ivp_device->pll_data[i][j].clk);
			ivp_device->pll_data[i][j].name = pll_names[i][j];
		}
	}

	mutex_init(&ivp_device->clk_ref_mutex);
	clk_ref = devm_kcalloc(dev, pll_nums, sizeof(*clk_ref), GFP_KERNEL);
	if (!clk_ref)
		return -ENOMEM;
	ivp_device->clk_ref = clk_ref;
#endif

	return 0;

}

int ivp_device_power_on(struct ivp_device *ivp_device)
{
	int ret = 0;
#if defined(CONFIG_COMMON_CLK_MT3612)
	int i;

	for (i = 0; i < IVP_CLK_NUM; i++) {
		ret = clk_prepare_enable(ivp_device->clk_sel[i]);
		if (ret) {
			dev_err(ivp_device->dev, "clk%d enable failed\n", i);
			return ret;
		}
	}
#endif
	return ret;
}

void ivp_device_power_off(struct ivp_device *ivp_device)
{
#if defined(CONFIG_COMMON_CLK_MT3612)
	int i;

	for (i = 0; i < IVP_CLK_NUM; i++)
		clk_disable_unprepare(ivp_device->clk_sel[i]);
#endif
}

int ivp_device_set_init_frequency(struct ivp_device *ivp_device)
{
	int ret = 0;
	struct ivp_pll_data *pll_data;
	int i, index;

	/* no clk resource */
	if (!ivp_device->clk_ref)
		return 0;

	for (i = 0; i < IVP_CLK_NUM; i++) {
		if (i == IVP_DSP_SEL)
			index = IVP_DSP_SEL_INDEX;
		else
			index = IVP_IPU_IF_SEL_INDEX;
		pll_data = &ivp_device->pll_data[i][index];

		if (!pll_data || !pll_data->clk) {
			dev_err(ivp_device->dev,
				"no pll_data(%p) or no pll_data->clk\n",
				pll_data);
			return -ENOENT;
		}

		dev_dbg(ivp_device->dev,
			"idx = %d, select %s pll_data(%p)\n",
			i, pll_data->name, pll_data);

		ret = ivp_set_clk(ivp_device, ivp_device->clk_sel[i],
				  pll_data);
		if (ret)
			return ret;

		dev_dbg(ivp_device->dev,
			"set dsp source %s, clk %lu, dsp clk rate %lu\n",
			pll_data->name, pll_data->clk_rate,
			clk_get_rate(ivp_device->clk_sel[i]));

		ivp_device->current_freq[i] = pll_data->clk_rate;
	}
	return ret;
}

int ivp_device_set_init_voltage(struct ivp_device *ivp_device)
{
	int ret = 0;
	struct ivp_opp_clk_table *table = ivp_device->opp_clk_table;
	unsigned long volt = 0;

	if (table && IVP_DEFAULT_VOLT_INDEX < ivp_device->opp_num)
		volt = table[IVP_DEFAULT_VOLT_INDEX].volt;
	else
		volt = IVP_DEFAULT_VOLT;

	ret = ivp_set_voltage(ivp_device, volt);
	if (ret) {
		dev_err(ivp_device->dev, "failed to do ivp_set_voltage: %d\n",
			ret);
	}

	return ret;
}

int ivp_core_setup_clk_resource(struct device *dev, struct ivp_core *ivp_core)
{
#if defined(CONFIG_COMMON_CLK_MT3612)
	int i;

	ivp_core->clk_sel = devm_clk_get(dev, clk_names[0]);
	if (IS_ERR(ivp_core->clk_sel)) {
		dev_err(dev, "cannot get %s\n", clk_names[0]);
		return -ENOENT;
	}

	ivp_core->clk_cg = devm_kcalloc(dev, IVP_CORE_CG_NUM,
				    sizeof(*ivp_core->clk_cg), GFP_KERNEL);
	if (!ivp_core->clk_cg)
		return -ENOMEM;
	for (i = 0; i < IVP_CORE_CG_NUM ; i++) {
		ivp_core->clk_cg[i] = devm_clk_get(dev, core_cg_names[i]);
		if (IS_ERR(ivp_core->clk_cg)) {
			dev_err(dev, "cannot get %s\n", core_cg_names[i]);
			return -ENOENT;
		}
	}
#endif
	return 0;
}

int ivp_core_power_on(struct ivp_core *ivp_core)
{
	int ret = 0;
#if defined(CONFIG_COMMON_CLK_MT3612)
	int i;

	ret = clk_prepare_enable(ivp_core->clk_sel);
	if (ret) {
		dev_err(ivp_core->dev, "clk enable failed\n");
		return ret;
	}

	for (i = 0; i < IVP_CORE_CG_NUM; i++) {
		ret = clk_prepare_enable(ivp_core->clk_cg[i]);
		if (ret) {
			dev_err(ivp_core->dev, "clk_cg%d enable failed\n", i);
			return ret;
		}
	}
#endif
	return ret;
}

void ivp_core_power_off(struct ivp_core *ivp_core)
{
#if defined(CONFIG_COMMON_CLK_MT3612)
	int i;

	for (i = 0; i < IVP_CORE_CG_NUM; i++)
		clk_disable_unprepare(ivp_core->clk_cg[i]);
	clk_disable_unprepare(ivp_core->clk_sel);
#endif
}

void ivp_core_reset(struct ivp_core *ivp_core)
{

}

int ivp_core_set_init_frequency(struct ivp_core *ivp_core)
{
	int ret = 0;
	struct ivp_device *ivp_device = ivp_core->ivp_device;
	struct ivp_pll_data *pll_data;

	/* no clk resource */
	if (!ivp_device->clk_ref)
		return 0;

	pll_data = &ivp_device->pll_data[IVP_DSP_SEL][IVP_DSP_SEL_INDEX];
	if (!pll_data || !pll_data->clk) {
		dev_err(ivp_device->dev,
			"no pll_data(%p) or no pll_data->clk\n",
			pll_data);
		return -ENOENT;
	}
	dev_dbg(ivp_core->dev,
		"select %s pll_data(%p)\n", pll_data->name, pll_data);

	ret = ivp_set_clk(ivp_device, ivp_core->clk_sel, pll_data);
	if (ret)
		return ret;

	dev_dbg(ivp_device->dev,
		"set dsp source %s, clk %lu, dsp clk rate %lu\n",
		pll_data->name, pll_data->clk_rate,
		clk_get_rate(ivp_core->clk_sel));

	ivp_core->current_freq = pll_data->clk_rate;
	mutex_lock(&ivp_device->clk_ref_mutex);
	ivp_device->clk_ref[IVP_DSP_SEL_INDEX] =
		ivp_device->clk_ref[IVP_DSP_SEL_INDEX] + 1;
	mutex_unlock(&ivp_device->clk_ref_mutex);

	return ret;
}

unsigned long ivp_get_sram_voltage(unsigned long volt)
{
	int i;
	unsigned long sram_volt = volt + IVP_VOLT_DELTA;

	if (sram_volt < IVP_SRAM_MIN_VOLT)
		sram_volt = IVP_SRAM_MIN_VOLT;

	for (i = 0; i < NUM_VOLTAGE; i++) {
		if (volt == volt_ctrl[i].vivp ||
		    volt == volt_ctrl[i].vivp_adjust) {
			sram_volt = volt_ctrl[i].vsram_adjust;
			break;
		}
	}

	return sram_volt;
}

#if defined(CONFIG_REGULATOR)
static int ivp_avs_setting(struct ivp_device *ivp_device)
{
#if defined(CONFIG_MTK_PMICW_P2P)
	int i;
	struct device_node *node;
	struct platform_device *pdev;
	struct regulator *vivp = ivp_device->regulator;
	unsigned long volt;

	node = of_find_compatible_node(NULL, NULL,
				       "mediatek,mt3612-pmicw-p2p");
	if (!node) {
		dev_warn(ivp_device->dev,
			 "Waiting for getting mt3612-pmicw-p2p node\n");
		of_node_put(node);
		return -EPROBE_DEFER;
	}

	pdev = of_find_device_by_node(node);
	if (!pdev) {
		dev_warn(ivp_device->dev, "Device dose not enable %s\n",
			 node->full_name);
		of_node_put(node);
		return -EPROBE_DEFER;
	}
	of_node_put(node);

	for (i = 0; i < NUM_VOLTAGE; i++) {
		volt = IVP_MV_TO_UV * mtk_get_vpu_avs_dac(pdev, i);
		if (volt == 0)
			continue;
		if (regulator_is_supported_voltage(vivp,
				   volt,
				   volt + IVP_REG_DELTA) == 1) {
			volt = volt * 10125 / 10000;
			volt_ctrl[i].vivp_adjust = volt;
			if (volt + IVP_VOLT_DELTA > IVP_SRAM_MIN_VOLT)
				volt_ctrl[i].vsram_adjust =
							volt + IVP_VOLT_DELTA;
		}
	}
#endif
	return 0;
}
#endif

int ivp_adjust_voltage(struct ivp_device *ivp_device)
{
	int ret = 0;
#if defined(CONFIG_REGULATOR)
	int i, j;
	struct ivp_opp_clk_table *table = ivp_device->opp_clk_table;

	ret = ivp_avs_setting(ivp_device);
	if (ret)
		return ret;

	for (i = 0; i < ivp_device->opp_num; i++) {
		for (j = 0; j < NUM_VOLTAGE; j++) {
			if (table[i].volt == volt_ctrl[j].vivp)
				break;
		}

		if (j == NUM_VOLTAGE)
			continue;

		table[i].volt_idx = j;
		table[i].volt = volt_ctrl[j].vivp_adjust;
	}
#endif
	return ret;
}

