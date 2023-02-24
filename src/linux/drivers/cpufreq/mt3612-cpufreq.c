/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: Zhaokai Wu <zhaokai.wu@mediatek.com>
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
 * @file mt3612-cpufreq.c
 *   CPU DVFS Linux Driver.
 *   SoC apply optimal DVFS level for power saving by scenarios requirement.
 */

/**
 * @defgroup IP_group_cpu_dvfs DVFS
 *   VCORE DVFS Linux Driver. \n
 *   SoC apply optimal DVFS level for power saving by scenarios requirement. \n
 *   CPU DVFS has 5 levels listed as below: \n
 *      LEVEL 4: Voltage=1.023V; Frequecy=1.8GHz \n
 *      LEVEL 3: Voltage=0.9V; Frequecy=1.44GHz \n
 *      LEVEL 2: Voltage=0.8V; Frequecy=1.15GHz \n
 *      LEVEL 1: Voltage=0.7V; Frequecy=750MHz \n
 *      LEVEL 0: Voltage=0.6V; Frequecy=400MHz \n
 *   @{
 *       @defgroup IP_group_cpu_dvfs_external EXTERNAL
 *         The external API document for cpu dvfs. \n
 *
 *         @{
 *            @defgroup IP_group_cpu_dvfs_external_function 1.function
 *              External function for cpu dvfs driver.
 *            @defgroup IP_group_cpu_dvfs_external_struct 2.structure
 *              none. No structure used.
 *            @defgroup IP_group_cpu_dvfs_external_typedef 3.typedef
 *              none. Follow Linux coding style, no typedef used.
 *            @defgroup IP_group_cpu_dvfs_external_enum 4.enumeration
 *              External enumeration for cpu dvfs driver.
 *            @defgroup IP_group_cpu_dvfs_external_def 5.define
 *              External define for cpu dvfs driver.
 *         @}
 *
 *       @defgroup IP_group_cpu_dvfs_internal INTERNAL
 *         The internal API document for cpu dvfs. \n
 *
 *         @{
 *            @defgroup IP_group_cpu_dvfs_internal_function 1.function
 *              Internal function for cpu dvfs driver.
 *            @defgroup IP_group_cpu_dvfs_internal_struct 2.structure
 *              none. No structure used.
 *            @defgroup IP_group_cpu_dvfs_internal_typedef 3.typedef
 *              none. Follow Linux coding style, no typedef used.
 *            @defgroup IP_group_cpu_dvfs_internal_enum 4.enumeration
 *              Internal enumeration for cpu dvfs driver.
 *            @defgroup IP_group_cpu_dvfs_internal_def 5.define
 *              Internal define for cpu dvfs driver.
 *         @}
 *     @}
 */

#include <linux/clk.h>
#include <linux/cpu.h>
#include <linux/cpu_cooling.h>
#include <linux/cpufreq.h>
#include <linux/cpumask.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/thermal.h>
#include <linux/delay.h>

#include <soc/mediatek/mtk_scp_ipi.h>
#include <soc/mediatek/mtk_dvfs.h>

#define VOLT_TOL		(10000)

/*
 * The struct mtk_cpu_dvfs_info holds necessary information for doing CPU DVFS
 * on each CPU power/clock domain of Mediatek SoCs. Each CPU cluster in
 * Mediatek SoCs has two voltage inputs, Vproc and Vsram. In some cases the two
 * voltage inputs need to be controlled under a hardware limitation:
 * 100mV < Vsram - Vproc < 200mV
 *
 * When scaling the clock frequency of a CPU clock domain, the clock source
 * needs to be switched to another stable PLL clock temporarily until
 * the original PLL becomes stable at target frequency.
 */
struct mtk_cpu_dvfs_info {
	struct device *cpu_dev;
	struct regulator *proc_reg;
	struct regulator *sram_reg;
	struct clk *cpu_clk;
	struct clk *inter_clk;
	struct thermal_cooling_device *cdev;
	int intermediate_voltage;
	bool need_voltage_tracking;
};

/** @ingroup IP_group_cpu_dvfs_external_function
 * @par Description
 *     Set CPU voltage & frequency by DVFS index.
 * @param[in] dvfs_index: CPU DVFS index.
 * @return
 *     status for CPU DVFS.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_cpufreq_set_target_index(unsigned int dvfs_index)
{
	int ret = 0, try_cnt = 4;

	do {
		ret = scp_ipi_send(SCP_IPI_DVFS_SET_CPU,
				&dvfs_index, sizeof(dvfs_index), 1, SCP_A_ID);
		pr_debug("[CPU FREQ] cpu_dvfs_index = %d\n", dvfs_index);

		if (ret) {
			usleep_range(1000, 2000);
			try_cnt -= 1;
			if (try_cnt == 0)
				pr_err("[CPU FREQ] CPU DVFS Fail!\n");
		}
	} while (ret && try_cnt);
	return ret;
}
EXPORT_SYMBOL_GPL(mtk_cpufreq_set_target_index);

static int mtk_cpufreq_set_target(struct cpufreq_policy *policy,
				      unsigned int dvfs_index)
{
	int ret = 0;

	ret = mtk_cpufreq_set_target_index(dvfs_index);

	return ret;
}

/* static int mtk_cpufreq_set_voltage(struct mtk_cpu_dvfs_info *info, int vproc)
* {
*	return regulator_set_voltage(info->proc_reg, vproc, vproc + VOLT_TOL);
* }
*/

static void mtk_cpufreq_ready(struct cpufreq_policy *policy)
{
	struct mtk_cpu_dvfs_info *info = policy->driver_data;
	struct device_node *np = of_node_get(info->cpu_dev->of_node);

	if (WARN_ON(!np))
		return;

	if (of_find_property(np, "#cooling-cells", NULL)) {
		info->cdev = of_cpufreq_cooling_register(np,
							 policy->related_cpus);

		if (IS_ERR(info->cdev)) {
			dev_err(info->cpu_dev,
				"running cpufreq without cooling device: %ld\n",
				PTR_ERR(info->cdev));

			info->cdev = NULL;
		}
	}

	of_node_put(np);
}

static int mtk_cpu_dvfs_info_init(struct mtk_cpu_dvfs_info *info, int cpu)
{
	struct device *cpu_dev;
	struct regulator *sram_reg = ERR_PTR(-ENODEV);
	struct clk *cpu_clk = ERR_PTR(-ENODEV);
	int ret;

	cpu_dev = get_cpu_device(cpu);
	if (!cpu_dev) {
		pr_err("failed to get cpu%d device\n", cpu);
		return -ENODEV;
	}

	cpu_clk = clk_get(cpu_dev, "cpu");
	if (IS_ERR(cpu_clk)) {
		if (PTR_ERR(cpu_clk) == -EPROBE_DEFER)
			pr_warn("cpu clk for cpu%d not ready, retry.\n", cpu);
		else
			pr_err("failed to get cpu clk for cpu%d\n", cpu);

		ret = PTR_ERR(cpu_clk);
		return ret;
	}

	/* Both presence and absence of sram regulator are valid cases. */
	sram_reg = regulator_get_exclusive(cpu_dev, "sram");

	ret = dev_pm_opp_of_add_table(cpu_dev);
	if (ret) {
		pr_warn("no OPP table for cpu%d\n", cpu);
		goto out_free_resources;
	}

	info->cpu_dev = cpu_dev;
	info->sram_reg = IS_ERR(sram_reg) ? NULL : sram_reg;
	info->cpu_clk = cpu_clk;

	/*
	 * If SRAM regulator is present, software "voltage tracking" is needed
	 * for this CPU power domain.
	 */
	info->need_voltage_tracking = !IS_ERR(sram_reg);

	return 0;

out_free_resources:
	if (!IS_ERR(sram_reg))
		regulator_put(sram_reg);
	if (!IS_ERR(cpu_clk))
		clk_put(cpu_clk);

	return ret;
}

static void mtk_cpu_dvfs_info_release(struct mtk_cpu_dvfs_info *info)
{
	if (!IS_ERR(info->proc_reg))
		regulator_put(info->proc_reg);
	if (!IS_ERR(info->sram_reg))
		regulator_put(info->sram_reg);
	if (!IS_ERR(info->cpu_clk))
		clk_put(info->cpu_clk);
	if (!IS_ERR(info->inter_clk))
		clk_put(info->inter_clk);

	dev_pm_opp_of_remove_table(info->cpu_dev);
}

static int mtk_cpufreq_init(struct cpufreq_policy *policy)
{
	struct mtk_cpu_dvfs_info *info;
	struct cpufreq_frequency_table *freq_table;
	int ret;

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	ret = mtk_cpu_dvfs_info_init(info, policy->cpu);
	if (ret) {
		pr_err("%s failed to initialize dvfs info for cpu%d\n",
		       __func__, policy->cpu);
		goto out_free_dvfs_info;
	}

	ret = dev_pm_opp_init_cpufreq_table(info->cpu_dev, &freq_table);
	if (ret) {
		pr_err("failed to init cpufreq table for cpu%d: %d\n",
		       policy->cpu, ret);
		goto out_release_dvfs_info;
	}

	ret = cpufreq_table_validate_and_show(policy, freq_table);
	if (ret) {
		pr_err("%s: invalid frequency table: %d\n", __func__, ret);
		goto out_free_cpufreq_table;
	}

	/* CPUs in the same cluster share a clock and power domain. */
	cpumask_copy(policy->cpus, &cpu_topology[policy->cpu].core_sibling);
	policy->driver_data = info;
	policy->clk = info->cpu_clk;

	return 0;

out_free_cpufreq_table:
	dev_pm_opp_free_cpufreq_table(info->cpu_dev, &freq_table);

out_release_dvfs_info:
	mtk_cpu_dvfs_info_release(info);

out_free_dvfs_info:
	kfree(info);

	return ret;
}

static int mtk_cpufreq_exit(struct cpufreq_policy *policy)
{
	struct mtk_cpu_dvfs_info *info = policy->driver_data;

	cpufreq_cooling_unregister(info->cdev);
	dev_pm_opp_free_cpufreq_table(info->cpu_dev, &policy->freq_table);
	mtk_cpu_dvfs_info_release(info);
	kfree(info);

	return 0;
}

static struct cpufreq_driver mt3612_cpufreq_driver = {
	.flags = CPUFREQ_STICKY | CPUFREQ_NEED_INITIAL_FREQ_CHECK,
	.verify = cpufreq_generic_frequency_table_verify,
	.target_index = mtk_cpufreq_set_target,
	.get = cpufreq_generic_get,
	.init = mtk_cpufreq_init,
	.exit = mtk_cpufreq_exit,
	.ready = mtk_cpufreq_ready,
	.name = "mtk-cpufreq",
	.attr = cpufreq_generic_attr,
};

static int mt3612_cpufreq_probe(struct platform_device *pdev)
{
	int ret;

	ret = cpufreq_register_driver(&mt3612_cpufreq_driver);
	if (ret)
		pr_err("failed to register mtk cpufreq driver\n");

	pr_info("mt3612-cpufreq probe done\n");

	return ret;
}

static struct platform_driver mt3612_cpufreq_platdrv = {
	.driver = {
		.name	= "mt3612-cpufreq",
	},
	.probe		= mt3612_cpufreq_probe,
};

static int mt3612_cpufreq_driver_init(void)
{
	struct platform_device *pdev;
	int err;

	if (!of_machine_is_compatible("mediatek,mt3612"))
		return -ENODEV;

	err = platform_driver_register(&mt3612_cpufreq_platdrv);
	if (err)
		return err;

	/*
	 * Since there's no place to hold device registration code and no
	 * device tree based way to match cpufreq driver yet, both the driver
	 * and the device registration codes are put here to handle defer
	 * probing.
	 */
	pdev = platform_device_register_simple("mt3612-cpufreq", -1, NULL, 0);
	if (IS_ERR(pdev)) {
		pr_err("failed to register mtk-cpufreq platform device\n");
		return PTR_ERR(pdev);
	}

	return 0;
}
device_initcall(mt3612_cpufreq_driver_init);

MODULE_DESCRIPTION("MT3612 cpufreq driver");
MODULE_AUTHOR("Zhaokai Wu <zhaokai.wu@mediatek.com>");
MODULE_LICENSE("GPL v2");
