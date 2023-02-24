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
#include <linux/kernel.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/of_regulator.h>

#include "ivp_driver.h"
#include "ivp_dvfs.h"
#include "ivp_met_event.h"
#include "ivp_opp_ctl.h"
#include "ivp_power_ctl.h"

#define IVP_SRAM_MIN_VOLT	850000
#define IVP_VOLT_DELTA		100000
#define IVP_VOLT_MID_DELTA	175000
#define IVP_VOLT_MAX_DELTA	250000
#define IVP_REG_DELTA		5000
#define IVP_SRAM_REG_DELTA	6250

int ivp_find_clk_index(struct ivp_device *ivp_device,
		       int clk_type, unsigned long *target_freq)
{
	struct ivp_opp_clk_table *table = ivp_device->opp_clk_table;
	struct ivp_pll_data *pll_data;
	int opp_num = ivp_device->opp_num;
	int clk_num;
	int i, ret = -EINVAL;

	/* no clk resource */
	if (!ivp_device->clk_ref)
		return 0;

	if (table && clk_type == IVP_DSP_SEL) {
		/* if target_freq >= max_freq */
		if (*target_freq >= table[opp_num - 1].freq) {
			*target_freq = table[opp_num - 1].freq;
			return opp_num - 1;
		}

		for (i = 0; i < opp_num; i++) {
			if (*target_freq <= table[i].freq) {
				*target_freq = table[i].freq;
				return i;
			}
		}
	} else {
		clk_num = ivp_device->clk_num[clk_type];
		pll_data = ivp_device->pll_data[clk_type];

		/* if target_freq >= max_freq */
		if (*target_freq >= pll_data[clk_num - 1].clk_rate) {
			*target_freq = pll_data[clk_num - 1].clk_rate;
			return clk_num - 1;
		}

		for (i = 0; i < clk_num; i++) {
			if (*target_freq <= pll_data[i].clk_rate) {
				*target_freq = pll_data[i].clk_rate;
				return i;
			}
		}
	}

	return ret;
}

static void ivp_check_opp_ref(struct ivp_device *ivp_device)
{
	int *clk_ref = ivp_device->clk_ref;
	int i, sum = 0;

	/* no clk resource */
	if (!clk_ref)
		return;

	for (i = 0; i < ivp_device->opp_num; i++)
		sum = sum + clk_ref[i];

	if (sum != ivp_device->ivp_core_num)
		dev_err(ivp_device->dev, "clk_ref err: %d\n", sum);
}

int ivp_set_clk(struct ivp_device *ivp_device, struct clk *clk,
		struct ivp_pll_data *pll_data)
{
	int ret = 0;

	if (!pll_data || !pll_data->clk || !clk) {
		dev_err(ivp_device->dev,
			"no pll_data(%p), pll_data->clk or clk\n", pll_data);
		return -EINVAL;
	}

	ret = clk_prepare_enable(pll_data->clk);
	if (ret) {
		dev_err(ivp_device->dev, "clk %s enable failed, %d\n",
			pll_data->name, ret);
		return ret;
	}

	ret = clk_set_parent(clk, pll_data->clk);
	clk_disable_unprepare(pll_data->clk);
	if (ret) {
		dev_err(ivp_device->dev, "clk %s set parent failed\n",
			pll_data->name);
		return ret;
	}

	return ret;
}

int ivp_set_freq(struct ivp_core *ivp_core, int clk_type,
		 unsigned long freq)
{
	struct ivp_device *ivp_device = ivp_core->ivp_device;
	struct ivp_opp_clk_table *table = ivp_device->opp_clk_table;
	struct ivp_pll_data *pll_data = NULL;
	struct clk *clk = NULL;
	int i, max_idx = 0, index, cur_idx, ret = 0;
	int *clk_ref;

	/* no clk resource */
	if (!ivp_device->clk_ref)
		return 0;

	if (clk_type >= IVP_CLK_NUM) {
		dev_err(ivp_device->dev, "%s: no such clk!\n", __func__);
		return -EINVAL;
	}

	/* If there is no change on frequency, no update */
	if (freq == 0)
		return 0;

	index = ivp_find_clk_index(ivp_device, clk_type, &freq);
	if (index < 0) {
		dev_err(ivp_device->dev,
			"failed to find index of clk%d: %d\n", clk_type,
			index);
		return -EINVAL;
	}

	if (clk_type == IVP_DSP_SEL) {
		if (freq == ivp_core->current_freq)
			return 0;
		if (table)
			pll_data = table[index].pll_data;
		else
			pll_data = &ivp_device->pll_data[clk_type][index];
		clk = ivp_core->clk_sel;
	} else {
		if (freq == ivp_device->current_freq[clk_type])
			return 0;
		pll_data = &ivp_device->pll_data[clk_type][index];
		clk = ivp_device->clk_sel[clk_type];
	}

	ret = ivp_set_clk(ivp_device, clk, pll_data);
	if (ret)
		return ret;

	if (clk_type == IVP_DSP_SEL) {
		cur_idx = ivp_find_clk_index(ivp_device, clk_type,
					     &ivp_core->current_freq);
		ivp_core->current_freq = clk_get_rate(pll_data->clk);

		mutex_lock(&ivp_device->clk_ref_mutex);
		ivp_device->clk_ref[index] =
			ivp_device->clk_ref[index] + 1;
		ivp_device->clk_ref[cur_idx] =
			ivp_device->clk_ref[cur_idx] - 1;
		ivp_check_opp_ref(ivp_device);
		clk_ref = ivp_device->clk_ref;
		mutex_unlock(&ivp_device->clk_ref_mutex);
		for (i = ivp_device->opp_num - 1; i >= 0; i--) {
			if (clk_ref[i]) {
				max_idx = i;
				break;
			}
		}
		if (table[max_idx].freq != ivp_device->current_freq[clk_type]) {
			pll_data = &ivp_device->pll_data[clk_type][max_idx];
			clk = ivp_device->clk_sel[clk_type];
			ret = ivp_set_clk(ivp_device, clk, pll_data);
			if (ret)
				return ret;

			ivp_device->current_freq[clk_type] =
						clk_get_rate(pll_data->clk);
		}
		dev_dbg(ivp_device->dev, "set core%d %s target frequency = %lu\n",
			 ivp_core->core, ivp_device->clk_name[clk_type], freq);

		dev_dbg(ivp_device->dev, "get core%d %s current frequency = %lu\n",
			 ivp_core->core, ivp_device->clk_name[clk_type],
			 ivp_core->current_freq);
	} else {
		ivp_device->current_freq[clk_type] =
						clk_get_rate(pll_data->clk);
		dev_dbg(ivp_device->dev, "set %s target frequency = %lu\n",
			 ivp_device->clk_name[clk_type], freq);
		dev_dbg(ivp_device->dev, "get %s current frequency = %lu\n",
			 ivp_device->clk_name[clk_type],
			 ivp_device->current_freq[clk_type]);
	}

	return 0;
}

int ivp_set_voltage(struct ivp_device *ivp_device, unsigned long voltage)
{
	unsigned long sram_voltage = ivp_get_sram_voltage(voltage);
	unsigned long tmp_vsram, tmp_voltage;
	int ret = 0;

	if ((ivp_device->current_voltage <= voltage + IVP_REG_DELTA &&
	    ivp_device->current_voltage >= voltage) ||
	    voltage == 0)
		return 0;

	tmp_vsram = regulator_get_voltage(ivp_device->sram_regulator);
	tmp_voltage = regulator_get_voltage(ivp_device->regulator);

#if defined(CONFIG_REGULATOR)
	if (voltage < ivp_device->current_voltage) {
		while (tmp_voltage != voltage) {
			if (tmp_vsram - voltage > IVP_VOLT_MAX_DELTA) {
				tmp_voltage = tmp_vsram - IVP_VOLT_MID_DELTA;
				tmp_vsram = ivp_get_sram_voltage(tmp_voltage);
			} else {
				tmp_voltage = voltage;
				tmp_vsram = sram_voltage;
			}
			ret = regulator_set_voltage(ivp_device->regulator,
				    tmp_voltage, tmp_voltage + IVP_REG_DELTA);
			if (ret) {
				dev_err(ivp_device->dev,
					"failed to set regulator voltage:%d\n",
					ret);
			} else {
				ret = regulator_set_voltage(
					ivp_device->sram_regulator,
					tmp_vsram,
					tmp_vsram + IVP_SRAM_REG_DELTA);
				if (ret) {
					dev_err(ivp_device->dev,
						"failed to set sram_regulator voltage:%d\n",
						ret);
				}
			}
		}
	} else {
		while (tmp_voltage != voltage) {
			if (sram_voltage - tmp_voltage > IVP_VOLT_MAX_DELTA) {
				tmp_voltage = tmp_voltage + IVP_VOLT_MID_DELTA;
				tmp_vsram = ivp_get_sram_voltage(tmp_voltage);
			} else {
				tmp_voltage = voltage;
				tmp_vsram = sram_voltage;
			}

			ret = regulator_set_voltage(ivp_device->sram_regulator,
				    tmp_vsram, tmp_vsram + IVP_SRAM_REG_DELTA);
			if (ret) {
				dev_err(ivp_device->dev,
					"failed to set regulator voltage:%d\n",
					ret);
			} else {
				ret = regulator_set_voltage(
					ivp_device->regulator,
					tmp_voltage,
					tmp_voltage + IVP_REG_DELTA);
				if (ret) {
					dev_err(ivp_device->dev,
						"failed to set sram_regulator voltage:%d\n",
						ret);
				}
			}
		}
	}

	dev_dbg(ivp_device->dev, "ivp set target voltage = %lu\n",
		 voltage);

	voltage = regulator_get_voltage(ivp_device->regulator);
	sram_voltage = regulator_get_voltage(ivp_device->sram_regulator);
	dev_dbg(ivp_device->dev, "ivp get current voltage = %lu, vsram voltage = %lu\n",
		 voltage, sram_voltage);
#endif

	ivp_device->current_voltage = voltage;
	ivp_device->current_sram_voltage = sram_voltage;

	return ret;
}

void ivp_device_dvfs_report(struct ivp_device *ivp_device)
{
	int core, freq[3] = {0}, core_num = 3;
	int voltage = 0;

	if (!(ivp_device->trace_func & IVP_TRACE_EVENT_DVFS))
		return;

	if (core_num > ivp_device->ivp_core_num)
		core_num = ivp_device->ivp_core_num;

	for (core = 0; core < core_num; core++) {
		if ((ivp_device->ivp_core[core]) &&
		    (ivp_device->ivp_core[core]->state != IVP_UNPREPARE))
			freq[core] = ivp_device->ivp_core[core]->current_freq;
	}

	if (freq[0] || freq[1] || freq[2])
		voltage = ivp_device->current_voltage;
	ivp_met_event_dvfs_trace(voltage, freq[0], freq[1], freq[2]);
}

int ivp_init_opp_clk_table(struct ivp_device *ivp_device)
{
	int ret = 0;
	struct device *dev = ivp_device->dev;
	struct ivp_opp_clk_table *table;
	struct dev_pm_opp *opp;
	unsigned long freq = 0;
	int opp_num, i;

	/* no clk resource */
	if (!ivp_device->clk_ref)
		return 0;

	rcu_read_lock();
	opp_num = dev_pm_opp_get_opp_count(dev);
	if (opp_num < 0) {
		rcu_read_unlock();
		dev_err(dev, "num of opp is negative: %d\n", opp_num);
		ret =  opp_num;
		goto out;
	}
	rcu_read_unlock();

	table = devm_kcalloc(dev, opp_num, sizeof(*table), GFP_KERNEL);
	if (!table) {
		ret = -ENOMEM;
		goto out;
	}

	rcu_read_lock();
	for (i = 0; i < opp_num; i++, freq++) {
		opp = dev_pm_opp_find_freq_ceil(dev, &freq);
		if (IS_ERR_OR_NULL(opp)) {
			rcu_read_unlock();
			dev_err(dev, "Failed to get opp: %ld\n", PTR_ERR(opp));
			ret = PTR_ERR(opp);
			goto out;
		}
		table[i].volt = dev_pm_opp_get_voltage(opp);
		table[i].pll_data = &ivp_device->pll_data[IVP_DSP_SEL][i];
		table[i].freq = clk_get_rate(table[i].pll_data->clk);
	}
	rcu_read_unlock();

	ivp_device->opp_num = opp_num;
	ivp_device->opp_clk_table = table;

	return 0;
out:
	ivp_device->opp_clk_table = NULL;

	return ret;

}

