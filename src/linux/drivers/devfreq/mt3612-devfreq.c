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
 * @file mt3612-devfreq.c
 *   VCORE DVFS Linux Driver.
 *   SoC apply optimal DVFS level for power saving by scenarios requirement.
 */

/**
 * @defgroup IP_group_vcore_dvfs DVFS
 *   VCORE DVFS Linux Driver. \n
 *   SoC apply optimal DVFS level for power saving by scenarios requirement. \n
 *   VCORE DVFS has 3 levels listed as below: \n
 *      LEVEL 3: VCORE2=0.8V; DRAM Frequency=467MHz \n
 *      LEVEL 2: VCORE2=0.7V; DRAM Frequency=400MHz \n
 *      LEVEL 1: VCORE2=0.6V; DRAM Frequency=200MHz \n
 *   @{
 *       @defgroup IP_group_vcore_dvfs_external EXTERNAL
 *         The external API document for vcore dvfs. \n
 *
 *         @{
 *            @defgroup IP_group_vcore_dvfs_external_function 1.function
 *              External function for vcore dvfs driver.
 *            @defgroup IP_group_vcore_dvfs_external_struct 2.structure
 *              none. No structure used.
 *            @defgroup IP_group_vcore_dvfs_external_typedef 3.typedef
 *              none. Follow Linux coding style, no typedef used.
 *            @defgroup IP_group_vcore_dvfs_external_enum 4.enumeration
 *              External enumeration for vcore dvfs driver.
 *            @defgroup IP_group_vcore_dvfs_external_def 5.define
 *              External define for vcore dvfs driver.
 *         @}
 *
 *       @defgroup IP_group_vcore_dvfs_internal INTERNAL
 *         The internal API document for vcore dvfs. \n
 *
 *         @{
 *            @defgroup IP_group_vcore_dvfs_internal_function 1.function
 *              Internal function for vcore dvfs driver.
 *            @defgroup IP_group_vcore_dvfs_internal_struct 2.structure
 *              none. No structure used.
 *            @defgroup IP_group_vcore_dvfs_internal_typedef 3.typedef
 *              none. Follow Linux coding style, no typedef used.
 *            @defgroup IP_group_vcore_dvfs_internal_enum 4.enumeration
 *              Internal enumeration for vcore dvfs driver.
 *            @defgroup IP_group_vcore_dvfs_internal_def 5.define
 *              Internal define for vcore dvfs driver.
 *         @}
 *     @}
 */

#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/devfreq.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/regulator/consumer.h>
#include <linux/rwsem.h>
#include <linux/suspend.h>
#include <linux/delay.h>

#include <soc/mediatek/mtk_scp_ipi.h>
#include <soc/mediatek/mtk_dvfs.h>

struct mt3612_devfreq {
	struct devfreq		*devfreq;
	void __iomem	*tar_index;
};

/** @ingroup IP_group_vcore_dvfs_external_function
 * @par Description
 *     Set VCORE voltage & frequency by DVFS index.
 * @param[in] dvfs_index: VCORE DVFS index.
 * @return
 *     status for VCORE DVFS.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_devfreq_set_target_index(unsigned int dvfs_index)
{
	int ret = 0, try_cnt = 4;

	do {
		ret = scp_ipi_send(SCP_IPI_DVFS_SET_VCORE,
				&dvfs_index, sizeof(dvfs_index), 1, SCP_A_ID);
		pr_info("[DRAM FREQ]core_dvfs_index = %d\n", dvfs_index);

		if (ret) {
			usleep_range(1000, 2000);
			try_cnt -= 1;
			if (try_cnt == 0)
				pr_err("[DRAM FREQ]core dvfs Fail!\n");
		}
	} while (ret && try_cnt);

	return ret;
}
EXPORT_SYMBOL_GPL(mtk_devfreq_set_target_index);

/*
* static int mtk_devfreq_set_target(struct device *dev,
*			       unsigned long *dvfs_index, u32 flags)
* {
*	int ret = 0;
*
*	ret = mtk_devfreq_set_target_index(*dvfs_index);
*
*	return ret;
* }
*
* static struct devfreq_dev_profile mt3612_devfreq_profile = {
*	.polling_ms	= 0,
*	.target		= mtk_devfreq_set_target,
* };
*
* static int mt3612_devfreq_probe(struct platform_device *pdev)
* {
*
*	struct mt3612_devfreq *mt3612;
*
*	mt3612->devfreq = devm_devfreq_add_device(&pdev->dev,
*						 &mt3612_devfreq_profile,
*						 "mt3612",
*						 NULL);
*
*	return 0;
* }
*/

static const u32 dram_freq_count = 3;
static const u32 dram_freq_table[] = {3733, 3200, 1600};
u32 g_dram_freq = 3733;

static ssize_t store_devfreq_access(struct device *dev
	, struct device_attribute *attr, const char *buf, size_t size)
{
	int i = 0, ret = 0;
	char temp_buf[32], *pvalue;
	unsigned int input_freq = 0;
	unsigned int dram_freq = dram_freq_table[dram_freq_count-1];
	unsigned int dvfs_index = 1;

	strncpy(temp_buf, buf, sizeof(temp_buf));
	temp_buf[sizeof(temp_buf) - 1] = 0;
	pvalue = temp_buf;
	ret = kstrtouint(pvalue, 10, &input_freq);
	for (i = 0; i < dram_freq_count; i++) {
		if (input_freq >= dram_freq_table[i]) {
			dvfs_index = dram_freq_count - i;
			dram_freq = dram_freq_table[i];
			break;
		}
	}
	ret = mtk_devfreq_set_target_index(dvfs_index);
	if (ret) {
		dev_err(dev, "[CORE DVFS] Index %d, DRAM %d Mbps fail\n",
			dvfs_index, dram_freq);
		return ret;
	} else {
		g_dram_freq = dram_freq;
		dev_err(dev, "[CORE DVFS] Index %d, DRAM %d Mbps done\n",
			dvfs_index, dram_freq);
		return size;
	}
}

static ssize_t show_devfreq_access(struct device *dev
	, struct device_attribute *attr, char *buf)
{
	dev_err(dev, "[CORE DVFS]dram freq = %d Mbps\n", g_dram_freq);

	return 0;
}

static DEVICE_ATTR(dram, 0664, show_devfreq_access, store_devfreq_access);

static int mt3612_devfreq_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mt3612_devfreq *mtk_devfreq;

	mtk_devfreq = devm_kzalloc(dev, sizeof(*mtk_devfreq), GFP_KERNEL);
	if (!mtk_devfreq)
		return -ENOMEM;

	mtk_devfreq->devfreq = NULL;
	if (device_create_file(&(pdev->dev), &dev_attr_dram))
		return -EINVAL;

	dev_info(dev, "mt3612-devfreq probe done\n");

	return 0;
}

static int mt3612_devfreq_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id mt3612_devfreq_of_match[] = {
	{ .compatible = "mediatek,mt3612-devfreq" },
	{ },
};
MODULE_DEVICE_TABLE(of, mt3612_devfreq_of_match);

static struct platform_driver mt3612_devfreq_driver = {
	.probe	= mt3612_devfreq_probe,
	.remove	= mt3612_devfreq_remove,
	.driver = {
		.name	= "mt3612-devfreq",
		.of_match_table = mt3612_devfreq_of_match,
	},
};
module_platform_driver(mt3612_devfreq_driver);

MODULE_DESCRIPTION("MT3612 devfreq driver");
MODULE_AUTHOR("Zhaokai Wu <zhaokai.wu@mediatek.com>");
MODULE_LICENSE("GPL v2");
