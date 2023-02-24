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

#include <linux/devfreq.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/timekeeper_internal.h>

#include "ivp_cmd_hnd.h"
#include "ivp_dvfs.h"
#include "ivp_opp_ctl.h"
#include "ivp_queue.h"
#include "ivp_utilization.h"

#define IVP_INITIAL_FREQ 604000
#define IVP_POLLING_MS 200

#define IVP_UP_THRESHOLD 90
#define IVP_DOWN_DIFFERENTIAL 10

static int ivp_devfreq_target(struct device *dev,
			      unsigned long *target_freq, u32 flags)
{
	struct ivp_core *ivp_core = dev_get_drvdata(dev);
	struct ivp_device *ivp_device = ivp_core->ivp_device;
	struct ivp_devfreq *ivp_devfreq = ivp_core->ivp_devfreq;
	unsigned long freq = *target_freq;
	unsigned long voltage = 0;
	unsigned long ipu_if_freq = 0;
	int core, i, ret = 0;
	int index, cur_idx, max_idx = 0;
	int *clk_ref;

	/* no clk resource */
	if (!ivp_device->clk_ref)
		return 0;

	index = ivp_find_clk_index(ivp_device, IVP_DSP_SEL, &freq);
	if (index < 0) {
		dev_err(ivp_device->dev,
			"failed to find index of clk%d: %d\n", IVP_DSP_SEL,
			index);
		return -EINVAL;
	}

	cur_idx = ivp_find_clk_index(ivp_device, IVP_DSP_SEL,
				&ivp_devfreq->current_freq);
	mutex_lock(&ivp_device->clk_ref_mutex);
	clk_ref = ivp_device->clk_ref;
	mutex_unlock(&ivp_device->clk_ref_mutex);
	for (i = ivp_device->opp_num - 1; i >= 0; i--) {
		if (cur_idx == i && clk_ref[i] == 1)
			continue;

		if (clk_ref[i]) {
			max_idx = i;
			break;
		}
	}
	if (index > max_idx)
		voltage = ivp_device->opp_clk_table[index].volt;
	else
		voltage = ivp_device->opp_clk_table[max_idx].volt;

	ivp_device_power_lock(ivp_device);

	if (ivp_devfreq->current_freq < freq) {
		ret = ivp_set_voltage(ivp_device, voltage);
		if (ret)
			goto out;
		ret = ivp_set_freq(ivp_core, IVP_DSP_SEL, freq);
		if (ret)  {
			ivp_set_voltage(ivp_device,
					ivp_device->current_voltage);
			goto out;
		}
	} else {
		ret = ivp_set_freq(ivp_core, IVP_DSP_SEL, freq);
		if (ret)
			goto out;
		ret = ivp_set_voltage(ivp_device, voltage);
		if (ret) {
			ivp_set_freq(ivp_core, IVP_DSP_SEL,
				     ivp_devfreq->current_freq);
			goto out;
		}
	}

	*target_freq = freq;
	ivp_devfreq->current_freq = ivp_core->current_freq;

	for (core = 0; core < ivp_device->ivp_core_num; core++) {
		ivp_core = ivp_device->ivp_core[core];
		if (ipu_if_freq < ivp_core->current_freq)
			ipu_if_freq = ivp_core->current_freq;
	}
	ret = ivp_set_freq(ivp_core, IVP_IPU_IF_SEL, ipu_if_freq);
out:
	ivp_device_power_unlock(ivp_device);
	ivp_device_dvfs_report(ivp_device);

	return ret;
}

static int ivp_devfreq_get_dev_status(struct device *dev,
				      struct devfreq_dev_status *status)
{
	struct ivp_core *ivp_core = dev_get_drvdata(dev);
	struct ivp_devfreq *ivp_devfreq = ivp_core->ivp_devfreq;

	int ret = 0;

	status->current_frequency = ivp_devfreq->current_freq;

	ret = ivp_dvfs_get_usage(ivp_core, &status->total_time,
				 &status->busy_time);

	status->private_data = ivp_devfreq->devfreq->data;

	ivp_devfreq->devfreq->last_status = *status;

	return ret;
}

static int ivp_devfreq_get_cur_freq(struct device *dev,
				    unsigned long *freq)
{
	struct ivp_core *ivp_core = dev_get_drvdata(dev);

	*freq = ivp_core->current_freq;

	return 0;
}

static void ivp_devfreq_exit(struct device *dev)
{

}

static struct devfreq_dev_profile ivp_dp = {
	.initial_freq = IVP_INITIAL_FREQ,
	.polling_ms = IVP_POLLING_MS,
	.target = ivp_devfreq_target,
	.get_dev_status = ivp_devfreq_get_dev_status,
	.get_cur_freq = ivp_devfreq_get_cur_freq,
	.exit = ivp_devfreq_exit,
};

int ivp_init_devfreq(struct ivp_core *ivp_core)
{
	struct device *dev = ivp_core->dev;
	struct ivp_device *ivp_device = ivp_core->ivp_device;
	struct ivp_devfreq *ivp_devfreq;
	struct ivp_pll_data *pll_data;
	int clk_num, ret = 0;

	/* no clk resource */
	if (!ivp_device->clk_ref)
		return 0;

	ivp_devfreq = devm_kzalloc(dev, sizeof(*ivp_devfreq), GFP_KERNEL);
	if (!ivp_devfreq)
		return -ENOMEM;

	ivp_devfreq->dev = dev;
	ivp_devfreq->ondemand_data.upthreshold = IVP_UP_THRESHOLD;
	ivp_devfreq->ondemand_data.downdifferential = IVP_DOWN_DIFFERENTIAL;

	ivp_devfreq->devfreq = devm_devfreq_add_device(dev,
					&ivp_dp, "simple_ondemand",
					&ivp_devfreq->ondemand_data);
	if (IS_ERR(ivp_devfreq->devfreq)) {
		ret = PTR_ERR(ivp_devfreq->devfreq);
		dev_err(dev, "Failed to add ivp_devfreq: %d", ret);
		return ret;
	}

	/* set initial frequency to ivp_devfreq */
	ivp_devfreq->current_freq = IVP_INITIAL_FREQ;

	ret = devm_devfreq_register_opp_notifier(dev, ivp_devfreq->devfreq);
	if (ret) {
		dev_err(dev, "Failed to register OPP notifier: %d\n", ret);
		goto err_opp_notifier;
	}

	pll_data = ivp_device->pll_data[IVP_DSP_SEL];
	clk_num = ivp_device->clk_num[IVP_DSP_SEL];

	ivp_devfreq->devfreq->min_freq = pll_data[0].clk_rate;
	ivp_devfreq->devfreq->max_freq = pll_data[clk_num - 1].clk_rate;
	ivp_core->ivp_devfreq = ivp_devfreq;

	return ret;

err_opp_notifier:
	devm_devfreq_remove_device(dev, ivp_devfreq->devfreq);
	ivp_devfreq->devfreq = NULL;

	return ret;
}

int ivp_deinit_devfreq(struct ivp_core *ivp_core)
{
	struct device *dev = ivp_core->dev;
	struct ivp_devfreq *ivp_devfreq = ivp_core->ivp_devfreq;
	int ret = 0;

	if (!IS_ERR_OR_NULL(ivp_devfreq->devfreq)) {
		dev_dbg(dev, "Deinit IVP devfreq\n");

		devm_devfreq_unregister_opp_notifier(dev,
						     ivp_devfreq->devfreq);

		devm_devfreq_remove_device(dev, ivp_devfreq->devfreq);

		ivp_devfreq->devfreq = NULL;
	}

	return ret;
}

