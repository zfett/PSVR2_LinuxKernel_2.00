/*
 * Copyright (c) 2019 MediaTek Inc.
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/consumer.h>
#include <linux/pm_opp.h>
#include <linux/delay.h>
#include "pvrsrv.h"
#include "rgxdevice.h"
#include "mtk_mfg_sys.h"
#if defined(MTK_ENABLE_MERGE_SETTING)
#include "mtk_mfg_utils.h"
#endif
#if defined(SUPPORT_AVS)
#include <soc/mediatek/mtk_mt3612_pmic_p2p.h>
#endif
#ifdef CONFIG_MTK_GPU_DEDICATED_FW_MEMORY
#include <soc/mediatek/mtk_mm_sysram.h>
#endif

struct mtk_mfg_base {
	struct platform_device *pdev;
	struct platform_device *pdev_mfg_pm0;
	struct platform_device *pdev_mfg_pm1;
	struct platform_device *pdev_mfg_pm2;
	struct platform_device *pdev_mfg_pm3;
#ifdef CONFIG_MTK_GPU_DEDICATED_FW_MEMORY
	struct device *dev_sysram;
#endif
	void __iomem *reg_base;
	struct clk *gpu_clk; /* GPUPLL_CK : gpupll */
	struct clk *univd3_clk; /* UNIVPLL_D3 : 416 MHz*/
	struct clk *sysd3_clk;  /* SYSPLL_D3  : 364 MHz*/
	struct clk *mfg_clk_sel;
	struct clk *mfg_cg;
	struct clk *gpupll;
	unsigned long cur_freq;
	struct mutex set_power_state;
	int power_state;
#if defined(CONFIG_REGULATOR)
	struct regulator *regulator11;
	struct regulator *sram_regulator;
	int vmax11;
	int sram_vmax;
#endif
};

static struct platform_device *pvr_pdev;
static struct platform_device *mfg0_pdev;
static struct platform_device *mfg1_pdev;
static struct platform_device *mfg2_pdev;
static struct platform_device *mfg3_pdev;

#define GET_MTK_MFG_BASE(x) (struct mtk_mfg_base *)(x->dev.platform_data)
#define GET_MTK_MFG_BASE_BY_DEV(x) (struct mtk_mfg_base *)(dev->platform_data)
#define GPU_SRAM_MIN_VOLT	850000
#define GPU_MAX_VOLT		900000
#define GPU_VOLT_DELTA		100000 /* delta of vgpu and vgpu_sram*/
#define GPU_VOLT_MIN_SHIFT	100000 /* 100mV < Vsram - Vcore < 250mV */
#define GPU_VOLT_MAX_SHIFT	250000 /* 100mV < Vsram - Vcore < 250mV */
#define GPU_REG_DELTA		6250
#define GPU_SRAM_REG_DELTA	6250
#define GPU_MV_TO_UV		1000
/*
 * 375MHz - 900MHz use divider 4,
 * frequency which is lower than 375MHz use divider 8.
 * Switch to other pll for parking when crossing 375 MHz.
 */
#define DIVIDER_LOW_BOUND	375000000
#define SYSPLL_D3_FREQ		364000000
#define UNIVPLL_D3_FREQ         416000000

#define OPP_FREQ_0		300000000 /* Hz */
#define OPP_FREQ_1		435000000
#define OPP_FREQ_2		570000000
#define OPP_FREQ_3		665000000
#define OPP_FREQ_4		760000000
#define OPP_FREQ_5		815000000
#define OPP_FREQ_6		870000000
#define CUST_FREQ_0		400000000
#define CUST_FREQ_1		750000000

#define POWER_ON	0
#define POWER_OFF	1

#define VSRAM(x)	((x)+GPU_VOLT_DELTA)
#if defined(MTK_ADD_VOLTAGE_SHIFT)
#define VSHIFT(x)	((x)*81/80)
#else
#define VSHIFT(x)	(x)
#endif

#ifdef CONFIG_MTK_GPU_DEDICATED_FW_MEMORY
#define GPU_FW_MEM_START (CONFIG_MTK_GPU_FW_MEMORY_START)
#define GPU_FW_MEM_SIZE (CONFIG_MTK_GPU_FW_MEMORY_SIZE)
#endif

#ifdef CONFIG_COMMON_CLK_MT3612
static PVRSRV_ERROR mtk_mfg_merge_setting(void)
{
#if defined(MTK_ENABLE_MERGE_SETTING)
	struct mtk_mfg_base *mfg_base = GET_MTK_MFG_BASE(pvr_pdev);

	reg_write(mfg_base->reg_base, MFG_MERGE_R_CON_00, 0x1180721);
	reg_write(mfg_base->reg_base, MFG_MERGE_R_CON_10, 0x1180721);
	reg_write(mfg_base->reg_base, MFG_MERGE_W_CON_00, 0x1180721);
	reg_write(mfg_base->reg_base, MFG_MERGE_W_CON_10, 0x1180721);
#endif
	return PVRSRV_OK;
}

static void mtk_mfg_enable_clock(void)
{
	int ret;
	struct mtk_mfg_base *mfg_base = GET_MTK_MFG_BASE(pvr_pdev);
	struct device *dev = &mfg_base->pdev->dev;

	dev_dbg(dev, "mtk_mfg_enable_clock\n");

#if defined(CONFIG_REGULATOR)
	/* enable regulator for voltage control*/
	ret = regulator_enable(mfg_base->regulator11);
	if (ret) {
		dev_err(dev, "regulator11 enable failed\n");
		return;
	}
	ret = regulator_enable(mfg_base->sram_regulator);
	if (ret) {
		dev_err(dev, "sram_regulator enable failed\n");
		return;
	}
#endif

	if (mfg_base->power_state == POWER_OFF) {
		/* power on mfg */
		pm_runtime_enable(&mfg_base->pdev_mfg_pm0->dev);
		pm_runtime_enable(&mfg_base->pdev_mfg_pm1->dev);
		pm_runtime_enable(&mfg_base->pdev_mfg_pm2->dev);
		pm_runtime_enable(&mfg_base->pdev_mfg_pm3->dev);
		pm_runtime_get_sync(&mfg_base->pdev_mfg_pm0->dev);
		pm_runtime_get_sync(&mfg_base->pdev_mfg_pm1->dev);
		pm_runtime_get_sync(&mfg_base->pdev_mfg_pm2->dev);
		pm_runtime_get_sync(&mfg_base->pdev_mfg_pm3->dev);
	}

	/* power on clock */
	ret = clk_prepare_enable(mfg_base->mfg_cg);
	if (ret) {
		dev_err(dev, "Unable to power on gpu_clk\n");
		return;
	}

	ret = clk_set_parent(mfg_base->mfg_clk_sel, mfg_base->gpu_clk);
	if (ret) {
		dev_err(dev, "Unable to set gpu_clk\n");
		clk_disable_unprepare(mfg_base->mfg_cg);
	}

	mtk_mfg_merge_setting();
	mfg_base->power_state = POWER_ON;
}

static void mtk_mfg_disable_clock(void)
{
	struct mtk_mfg_base *mfg_base = GET_MTK_MFG_BASE(pvr_pdev);
	struct device *dev = &mfg_base->pdev->dev;

	dev_dbg(dev, "mtk_mfg_disable_clock\n");

	/* power off clock */
	clk_disable_unprepare(mfg_base->mfg_cg);

	if (mfg_base->power_state == POWER_ON) {
		/* power off mfg */
		pm_runtime_put_sync(&mfg_base->pdev_mfg_pm3->dev);
		pm_runtime_put_sync(&mfg_base->pdev_mfg_pm2->dev);
		pm_runtime_put_sync(&mfg_base->pdev_mfg_pm1->dev);
		pm_runtime_put_sync(&mfg_base->pdev_mfg_pm0->dev);
		pm_runtime_disable(&mfg_base->pdev_mfg_pm3->dev);
		pm_runtime_disable(&mfg_base->pdev_mfg_pm2->dev);
		pm_runtime_disable(&mfg_base->pdev_mfg_pm1->dev);
		pm_runtime_disable(&mfg_base->pdev_mfg_pm0->dev);
	}

#if defined(CONFIG_REGULATOR)
	/* disable regulator for voltage control*/
	regulator_disable(mfg_base->regulator11);
	regulator_disable(mfg_base->sram_regulator);
#endif

	mfg_base->power_state = POWER_OFF;
}
#endif

static int mtk_mfg_bind_device_resource(struct platform_device *pdev,
					struct mtk_mfg_base *mfg_base)
{
	int err = 0;

	mfg_base->reg_base = of_iomap(pdev->dev.of_node, 2);
	if (!mfg_base->reg_base) {
		PVR_LOG(("Unable to ioremap registers pdev %p\n", pdev));
		err = -ENOMEM;
	}

#if defined(CONFIG_REGULATOR)
	mfg_base->regulator11 = devm_regulator_get(&pdev->dev, "vgpu11");
	if (IS_ERR(mfg_base->regulator11)) {
		dev_err(&pdev->dev, "cannot get regulator vgpu11\n");
		err = PTR_ERR(mfg_base->regulator11);
	}
	mfg_base->sram_regulator = devm_regulator_get(&pdev->dev, "vsram_gpu");
	if (IS_ERR(mfg_base->sram_regulator)) {
		dev_err(&pdev->dev, "cannot get regulator vsram\n");
		err = PTR_ERR(mfg_base->sram_regulator);
	}

	mfg_base->vmax11 = VSHIFT(GPU_MAX_VOLT);
	mfg_base->sram_vmax = mfg_base->vmax11 + GPU_VOLT_DELTA;
#endif

#ifdef CONFIG_COMMON_CLK_MT3612
	mfg_base->gpu_clk = devm_clk_get(&pdev->dev, "gpupll_ck");
	if (IS_ERR(mfg_base->gpu_clk)) {
		PVR_LOG(("Unable to get gpu_clk"));
		err = -ENOMEM;
	}

	mfg_base->univd3_clk = devm_clk_get(&pdev->dev, "univpll_d3");
	if (IS_ERR(mfg_base->univd3_clk)) {
		PVR_LOG(("Unable to get univd3_clk"));
		err = -ENOMEM;
	}

	mfg_base->sysd3_clk = devm_clk_get(&pdev->dev, "syspll_d3");
	if (IS_ERR(mfg_base->sysd3_clk)) {
		PVR_LOG(("Unable to get sysd3_clk"));
		err = -ENOMEM;
	}

	mfg_base->mfg_clk_sel = devm_clk_get(&pdev->dev, "mfg_clk_sel");
	if (IS_ERR(mfg_base->mfg_clk_sel)) {
		PVR_LOG(("Unable to get mfg_clk_sel"));
		err = -ENOMEM;
	}

	mfg_base->mfg_cg = devm_clk_get(&pdev->dev, "mfg_cg");
	if (IS_ERR(mfg_base->mfg_cg)) {
		PVR_LOG(("Unable to get mfg_cg"));
		err = -ENOMEM;
	}

	mfg_base->gpupll = devm_clk_get(&pdev->dev, "gpupll");
	if (IS_ERR(mfg_base->gpupll)) {
		PVR_LOG(("Unable to get gpupll"));
		err = -ENOMEM;
	}

	mfg_base->cur_freq = OPP_FREQ_6;

	mfg_base->pdev_mfg_pm0 = mfg0_pdev;
	mfg_base->pdev_mfg_pm1 = mfg1_pdev;
	mfg_base->pdev_mfg_pm2 = mfg2_pdev;
	mfg_base->pdev_mfg_pm3 = mfg3_pdev;

	/* pm_runtime_enable(&pdev->dev); */
#endif
	mfg_base->pdev = pdev;

	return err;
}

void mtk_mfg_set_frequency(IMG_UINT32 ui32Frequency)
{
	struct mtk_mfg_base *mfg_base = GET_MTK_MFG_BASE(pvr_pdev);
	struct device *dev = &mfg_base->pdev->dev;
#ifdef CONFIG_COMMON_CLK_MT3612
	int ret = 0;
#endif

	dev_dbg(dev, "set_frequency %u, cur_freq %lu\n",
		     ui32Frequency, mfg_base->cur_freq);

#ifdef CONFIG_COMMON_CLK_MT3612
	if (ui32Frequency < DIVIDER_LOW_BOUND
				&& mfg_base->cur_freq > DIVIDER_LOW_BOUND) {
		/*
		 * cur_freq (higher than 375 MHz) -> target (lower than 375 MHz)
		 */
		dev_dbg(dev, "switch to sysd3_pll\n");

		ret = clk_set_parent(mfg_base->mfg_clk_sel,
				     mfg_base->sysd3_clk);
		if (ret) {
			dev_err(dev, "Unable to set sysd3_clk\n");
			return;
		}
		ret = clk_set_rate(mfg_base->gpupll, ui32Frequency);
		if (ret)
			dev_err(dev, "failed to set gpupll %d\n",
				     ui32Frequency);

		udelay(20);

		ret = clk_set_parent(mfg_base->mfg_clk_sel, mfg_base->gpu_clk);
		if (ret) {
			dev_err(dev, "Unable to set gpu_clk\n");
			return;
		}
	} else if (ui32Frequency > DIVIDER_LOW_BOUND
				&& mfg_base->cur_freq < DIVIDER_LOW_BOUND) {
		/*
		 * cur_freq (lower than 375 MHz) -> target (higher than 375 MHz)
		 */
		if (ui32Frequency < UNIVPLL_D3_FREQ) {
			dev_dbg(dev, "switch to sysd3_clk for parking\n");
			ret = clk_set_parent(mfg_base->mfg_clk_sel,
					     mfg_base->sysd3_clk);
			if (ret) {
				dev_err(dev, "Unable to set sysd3_clk\n");
				return;
			}
		} else {
			dev_dbg(dev, "switch to univd3_clk for parking\n");
			ret = clk_set_parent(mfg_base->mfg_clk_sel,
					     mfg_base->univd3_clk);
			if (ret) {
				dev_err(dev, "Unable to set univd3_clk\n");
				return;
			}
		}

		ret = clk_set_rate(mfg_base->gpupll, ui32Frequency);
		if (ret)
			dev_err(dev, "failed to set gpupll %d\n",
				     ui32Frequency);

		/* wait pll stable */
		udelay(20);

		dev_dbg(dev, "switch to gpu_clk\n");
		ret = clk_set_parent(mfg_base->mfg_clk_sel, mfg_base->gpu_clk);
		if (ret) {
			dev_err(dev, "Unable to set gpu_clk\n");
			return;
		}

	} else {
		ret = clk_set_rate(mfg_base->gpupll, ui32Frequency);
		if (ret) {
			dev_err(dev, "failed to set gpupll %d\n",
				     ui32Frequency);
			return;
		}
	}

	mfg_base->cur_freq = ui32Frequency;
#endif
}

static int mtk_mfg_set_vgpu(int volt11)
{
	int regu_vmax11;
	int err = 0;
	struct mtk_mfg_base *mfg_base = GET_MTK_MFG_BASE(pvr_pdev);
	struct device *dev = &mfg_base->pdev->dev;

	dev_dbg(dev, "mtk_mfg_set_vgpu %u\n", volt11);

	regu_vmax11 = mfg_base->vmax11 + GPU_REG_DELTA;

	err = regulator_set_voltage(mfg_base->regulator11,
				    volt11,
				    regu_vmax11);
	if (err) {
		dev_err(dev,
			"failed to set regu11 voltage:%d\n", err);
	}

	return err;
}

static int mtk_mfg_set_vsram(int volt)
{
	int err = 0;
	struct mtk_mfg_base *mfg_base = GET_MTK_MFG_BASE(pvr_pdev);
	struct device *dev = &mfg_base->pdev->dev;
	int sregu_vmax = mfg_base->sram_vmax + GPU_SRAM_REG_DELTA;

	dev_dbg(dev, "mtk_mfg_set_vsram %u\n", volt);

	if (volt < GPU_SRAM_MIN_VOLT)
		volt = GPU_SRAM_MIN_VOLT;

	err = regulator_set_voltage(mfg_base->sram_regulator,
				    volt,
				    sregu_vmax);
	if (err)
		dev_err(dev, "failed to set sram_regu voltage:%d\n", err);

	return err;
}

void mtk_mfg_set_voltage(IMG_UINT32 ui32Voltage)
{
#if defined(CONFIG_REGULATOR)
	int err = 0;
#endif
	int cur_volt11, cur_svolt;
	int new_volt11 = VSHIFT(ui32Voltage);
	int new_svolt;
	struct mtk_mfg_base *mfg_base = GET_MTK_MFG_BASE(pvr_pdev);
	struct device *dev = &mfg_base->pdev->dev;

	dev_dbg(dev, "set_voltage %u %u\n", ui32Voltage, new_volt11);

#if defined(CONFIG_REGULATOR)
	new_svolt = VSRAM(new_volt11);

	cur_volt11 = regulator_get_voltage(mfg_base->regulator11);
	cur_svolt = regulator_get_voltage(mfg_base->sram_regulator);

	if (new_volt11 > cur_volt11) { /* level up : vsram -> vgpu*/
		do {
			cur_svolt = min(new_svolt,
					cur_volt11 + GPU_VOLT_MAX_SHIFT);
			err = mtk_mfg_set_vsram(cur_svolt);
			if (err)
				return;

			if (cur_svolt == new_svolt)
				cur_volt11 = new_volt11;
			else
				cur_volt11 = cur_svolt - GPU_VOLT_DELTA;

			err = mtk_mfg_set_vgpu(cur_volt11);
			if (err)
				return;
		} while (cur_volt11 < new_volt11 || cur_svolt < new_svolt);
	} else { /* level down : vgpu -> vsram */
		do {

			cur_volt11 = max(new_volt11,
					 cur_svolt - GPU_VOLT_MAX_SHIFT);
			err = mtk_mfg_set_vgpu(cur_volt11);

			if (err)
				return;

			if (cur_volt11 == new_volt11)
				cur_svolt = new_svolt;
			else
				cur_svolt = max(new_svolt,
					       cur_volt11 + GPU_VOLT_MIN_SHIFT);

			err = mtk_mfg_set_vsram(cur_svolt);
			if (err)
				return;
		} while (cur_volt11 > new_volt11 || cur_svolt > new_svolt);
	}

	cur_volt11 = regulator_get_voltage(mfg_base->regulator11);
	cur_svolt = regulator_get_voltage(mfg_base->sram_regulator);
#endif
	dev_dbg(dev, "vgpu11 = %u, sram_voltage = %u\n",
		cur_volt11, cur_svolt);
}

#ifdef CONFIG_MTK_GPU_DEDICATED_FW_MEMORY
static int mtk_mfg_get_device(struct device *dev,
	char *compatible, int idx, struct device **child_dev)
{
	struct platform_device *pdev;
	struct device_node *node;

	node = of_parse_phandle(dev->of_node, compatible, idx);
	if (!node) {
		dev_err(dev, "mfg_get_device: could not find %s %d\n",
			compatible, idx);
		return -EPROBE_DEFER;
	}

	pdev = of_find_device_by_node(node);
	if (!pdev || !pdev->dev.driver) {
		dev_warn(dev,
			 "mfg_get_device: waiting for device %s\n",
			 node->full_name);
		return -EPROBE_DEFER;
	}
	*child_dev = &pdev->dev;

	return 0;
}
#endif

#if defined(SUPPORT_AVS)
static int get_avs_idx(unsigned long freq)
{
	int idx;

	switch (freq) {
	case OPP_FREQ_0:
		idx = 0;
		break;
	case OPP_FREQ_1:
	case CUST_FREQ_0:
		idx = 1;
		break;
	case OPP_FREQ_2:
		idx = 2;
		break;
	case OPP_FREQ_3:
		idx = 3;
		break;
	case OPP_FREQ_4:
	case CUST_FREQ_1:
		idx = 4;
		break;
	case OPP_FREQ_5:
		idx = 5;
		break;
	case OPP_FREQ_6:
		idx = 6;
		break;
	default:
		idx = -1;
		break;
	}

	return idx;
}
#endif

void mtk_mfg_avs_config(struct device *dev)
{
#if defined(SUPPORT_AVS)
	struct device_node *node;
	struct platform_device *pdev;
	int i, idx;
	struct dev_pm_opp *opp;
	int opp_num;
	unsigned long freq;
#if defined(CONFIG_REGULATOR)
	struct mtk_mfg_base *mfg_base = GET_MTK_MFG_BASE_BY_DEV(dev);
	int volt;
#endif

	node = of_find_compatible_node(NULL, NULL,
				       "mediatek,mt3612-pmicw-p2p");
	if (!node) {
		dev_warn(dev,
			 "Waiting for getting mt3612-pmicw-p2p node\n");
		of_node_put(node);
		return;
	}
	pdev = of_find_device_by_node(node);
	if (!pdev) {
		dev_warn(dev, "Device dose not enable %s\n", node->full_name);
		of_node_put(node);
		return;
	}
	of_node_put(node);

	rcu_read_lock();
	opp_num = dev_pm_opp_get_opp_count(dev);
	if (opp_num < 0) {
		rcu_read_unlock();
		dev_err(dev, "num of opp is negative: %d\n", opp_num);
		rcu_read_unlock();
		return;
	}
	rcu_read_unlock();

	rcu_read_lock();
	freq = 0;
	for (i = 0; i < opp_num; i++) {
		freq++;
		opp = dev_pm_opp_find_freq_ceil(dev, &freq);
		if (IS_ERR_OR_NULL(opp)) {
			dev_err(dev, "Failed to get opp: %ld\n", PTR_ERR(opp));
			continue;
		}

		idx = get_avs_idx(freq);
		if (idx < 0)
			continue;
#if defined(CONFIG_REGULATOR)
		volt = GPU_MV_TO_UV * mtk_get_gpu_avs_dac(pdev, idx);
		if (regulator_is_supported_voltage(mfg_base->regulator11, volt,
						   volt + GPU_REG_DELTA) == 1) {
			dev_pm_opp_remove(dev, freq);
			dev_pm_opp_add(dev, freq, volt);
			dev_dbg(dev, "Update OPP: freq:%lu, volt:%u\n",
				freq, volt);
		}
#endif
	}
	rcu_read_unlock();

#endif /* SUPPORT_AVS */
}

void mtk_mfg_show_opp_tbl(struct seq_file *sf)
{
	struct mtk_mfg_base *mfg_base = GET_MTK_MFG_BASE(pvr_pdev);
	struct device *dev = &mfg_base->pdev->dev;
	int i;
	struct dev_pm_opp *opp;
	int opp_num;
	unsigned long freq, volt;

	rcu_read_lock();
	opp_num = dev_pm_opp_get_opp_count(dev);
	if (opp_num < 0) {
		rcu_read_unlock();
		dev_err(dev, "num of opp is negative: %d\n", opp_num);
		rcu_read_unlock();
		return;
	}
	rcu_read_unlock();

	rcu_read_lock();
	freq = 0;
	for (i = 0; i < opp_num; i++) {
		freq++;
		opp = dev_pm_opp_find_freq_ceil(dev, &freq);
		if (IS_ERR_OR_NULL(opp)) {
			dev_err(dev, "Failed to get opp: %ld\n", PTR_ERR(opp));
			continue;
		}
		volt = dev_pm_opp_get_voltage(opp);
		seq_printf(sf, "opp[%d]: (%lu Hz, %lu uV)\n", i, freq, volt);

	}
	rcu_read_unlock();

}
void mtk_mfg_show_cur_opp(struct seq_file *sf)
{
	struct mtk_mfg_base *mfg_base = GET_MTK_MFG_BASE(pvr_pdev);
#if defined(CONFIG_REGULATOR)
	unsigned long volt11, svolt;

	volt11 = regulator_get_voltage(mfg_base->regulator11);
	svolt = regulator_get_voltage(mfg_base->sram_regulator);
#endif

	seq_printf(sf, "frequency: %lu Hz\n", mfg_base->cur_freq);
#if defined(CONFIG_REGULATOR)
	seq_printf(sf, "vgpu11: %lu uV\n", volt11);
	seq_printf(sf, "vgpu_sram: %lu uV\n", svolt);
#endif
}

int mtk_mfg_get_loading(void)
{
#ifdef MTK_DVFS_CTRL
	PVRSRV_DATA *pvr_data = PVRSRVGetPVRSRVData();
	PVRSRV_DEVICE_NODE *pvr_dev_node = pvr_data->psDeviceNodeList;
	PVRSRV_RGXDEV_INFO *pvr_dev_info = pvr_dev_node->pvDevice;
	IMG_DVFS_DEVICE *dvfs_dev =
				&pvr_dev_node->psDevConfig->sDVFS.sDVFSDevice;
	RGXFWIF_GPU_UTIL_STATS stats;
	struct mtk_mfg_base *mfg_base = GET_MTK_MFG_BASE(pvr_pdev);
	struct device *dev = &mfg_base->pdev->dev;
	int ret;
	unsigned long long btime, ttime;

	if (pvr_dev_info->pfnGetGpuUtilStats == NULL)
		/* Not yet ready */
		return 0;

	ret = pvr_dev_info->pfnGetGpuUtilStats(pvr_dev_info->psDeviceNode,
						dvfs_dev->hGpuUtilUserDVFS,
						&stats);

	if (ret != PVRSRV_OK) {
		dev_err(dev, "get gpu loading failed\n");
		return 0;
	}

	btime = stats.ui64GpuStatActiveHigh + stats.ui64GpuStatActiveLow;
	ttime = stats.ui64GpuStatCumulative;

	return (int)((btime * 100) / ttime);
#else
	return 0;
#endif /* MTK_DVFS_CTRL */
}
EXPORT_SYMBOL(mtk_mfg_get_loading);

unsigned long mtk_mfg_get_freq(void)
{
	struct mtk_mfg_base *mfg_base = GET_MTK_MFG_BASE(pvr_pdev);

	return mfg_base->cur_freq;
}
EXPORT_SYMBOL(mtk_mfg_get_freq);

PVRSRV_ERROR mtk_mfg_device_init(void *pvOSDevice)
{
	struct platform_device *pdev;
	struct mtk_mfg_base *mfg_base;
	int err = 0;

	pdev = to_platform_device((struct device *)pvOSDevice);
	pvr_pdev = pdev;
	mfg_base = devm_kzalloc(&pdev->dev, sizeof(*mfg_base), GFP_KERNEL);
	if (!mfg_base)
		return -ENOMEM;

	err = mtk_mfg_bind_device_resource(pdev, mfg_base);
	if (err != 0)
		return err;

	mutex_init(&mfg_base->set_power_state);
	pdev->dev.platform_data = mfg_base;

	mfg_base->power_state = POWER_ON;
/* sysram power on */
#ifdef CONFIG_MTK_GPU_DEDICATED_FW_MEMORY
	err = mtk_mfg_get_device(&pdev->dev, "mediatek,sysram", 0,
				&mfg_base->dev_sysram);
	if (err != 0)
		return err;

	err = mtk_mm_sysram_power_on(mfg_base->dev_sysram,
				     GPU_FW_MEM_START,
				     GPU_FW_MEM_SIZE);
	if (err) {
		dev_err(&pdev->dev, "Failed to power on sysram: %d\n", err);
		return err;
	}
#endif

	return err;
}

void mtk_mfg_device_deinit(void *pvOSDevice)
{
	struct mtk_mfg_base *mfg_base = GET_MTK_MFG_BASE(pvr_pdev);
#ifdef CONFIG_MTK_GPU_DEDICATED_FW_MEMORY
	struct device *dev = &mfg_base->pdev->dev;
	int ret;
#endif

/* sysram power off */
#ifdef CONFIG_MTK_GPU_DEDICATED_FW_MEMORY
	ret = mtk_mm_sysram_power_off(mfg_base->dev_sysram,
				      GPU_FW_MEM_START,
				      GPU_FW_MEM_SIZE);
	if (ret)
		dev_err(dev, "Failed to power off sbrc: %d\n", ret);
#endif

	iounmap(mfg_base->reg_base);
	mfg_base->power_state = POWER_OFF;
}

PVRSRV_ERROR mtk_mfg_prepower_state(IMG_HANDLE hSysData,
			      PVRSRV_DEV_POWER_STATE eNewPowerState,
			      PVRSRV_DEV_POWER_STATE eCurrentPowerState,
			      IMG_BOOL bForced)
{
#ifdef CONFIG_COMMON_CLK_MT3612
	struct mtk_mfg_base *mfg_base = GET_MTK_MFG_BASE(pvr_pdev);
	struct device *dev = &mfg_base->pdev->dev;

	dev_dbg(dev, "mtk_mfg_prepower_state\n");

	mutex_lock(&mfg_base->set_power_state);
	if ((eNewPowerState == PVRSRV_DEV_POWER_STATE_OFF) &&
	    (eCurrentPowerState == PVRSRV_DEV_POWER_STATE_ON))
		mtk_mfg_disable_clock();

	mutex_unlock(&mfg_base->set_power_state);
#endif

	return PVRSRV_OK;
}

PVRSRV_ERROR mtk_mfg_postpower_state(IMG_HANDLE hSysData,
			      PVRSRV_DEV_POWER_STATE eNewPowerState,
			      PVRSRV_DEV_POWER_STATE eCurrentPowerState,
			      IMG_BOOL bForced)
{
#ifdef CONFIG_COMMON_CLK_MT3612
	struct mtk_mfg_base *mfg_base = GET_MTK_MFG_BASE(pvr_pdev);
	struct device *dev = &mfg_base->pdev->dev;

	dev_dbg(dev, "mtk_mfg_postpower_state\n");

	mutex_lock(&mfg_base->set_power_state);
	if ((eNewPowerState == PVRSRV_DEV_POWER_STATE_ON) &&
	    (eCurrentPowerState == PVRSRV_DEV_POWER_STATE_OFF))
		mtk_mfg_enable_clock();

	mutex_unlock(&mfg_base->set_power_state);
#endif

	return PVRSRV_OK;
}

static int mtk_mfg_pm0_probe(struct platform_device *pdev)
{
	if (!pdev->dev.pm_domain) {
		dev_err(&pdev->dev, "Failed to get dev->pm_domain\n");
		return -EPROBE_DEFER;
	}

	pm_runtime_enable(&pdev->dev);
	mfg0_pdev = pdev;

	if (mfg0_pdev)
		pm_runtime_get_sync(&mfg0_pdev->dev);
	else
		PVR_LOG(("mtk_mfg_pm0_init enable power domain failed"));

	return 0;
}

static int mtk_mfg_pm1_probe(struct platform_device *pdev)
{
	if (!pdev->dev.pm_domain) {
		dev_err(&pdev->dev, "Failed to get dev->pm_domain\n");
		return -EPROBE_DEFER;
	}

	pm_runtime_enable(&pdev->dev);
	mfg1_pdev = pdev;

	if (mfg1_pdev)
		pm_runtime_get_sync(&mfg1_pdev->dev);
	else
		PVR_LOG(("mtk_mfg_pm1_init enable power domain failed"));

	return 0;
}

static int mtk_mfg_pm2_probe(struct platform_device *pdev)
{
	if (!pdev->dev.pm_domain) {
		dev_err(&pdev->dev, "Failed to get dev->pm_domain\n");
		return -EPROBE_DEFER;
	}

	pm_runtime_enable(&pdev->dev);
	mfg2_pdev = pdev;

	if (mfg2_pdev)
		pm_runtime_get_sync(&mfg2_pdev->dev);
	else
		PVR_LOG(("mtk_mfg_pm2_init enable power domain failed"));

	return 0;
}

static int mtk_mfg_pm3_probe(struct platform_device *pdev)
{
	if (!pdev->dev.pm_domain) {
		dev_err(&pdev->dev, "Failed to get dev->pm_domain\n");
		return -EPROBE_DEFER;
	}

	pm_runtime_enable(&pdev->dev);
	mfg3_pdev = pdev;

	if (mfg3_pdev)
		pm_runtime_get_sync(&mfg3_pdev->dev);
	else
		PVR_LOG(("mtk_mfg_pm3_init enable power domain failed"));

	return 0;
}

static int mtk_mfg_pm_remove(struct platform_device *pdev)
{
	struct mtk_mfg_base *mfg_base = GET_MTK_MFG_BASE(pvr_pdev);

	if (mfg_base->power_state == POWER_ON) {
		pm_runtime_put_sync(&pdev->dev);
		pm_runtime_disable(&pdev->dev);
	}

	return 0;
}

static const struct of_device_id mtk_mfg_pm0_of_ids[] = {
	{ .compatible = "mediatek,mt3612-mfg-pm0",},
	{}
};

static struct platform_driver mtk_mfg_pm0_driver = {
	.probe  = mtk_mfg_pm0_probe,
	.remove = mtk_mfg_pm_remove,
	.driver = {
		.name = "mfg_pm0",
		.of_match_table = mtk_mfg_pm0_of_ids,
	}
};

int __init mtk_mfg_pm0_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&mtk_mfg_pm0_driver);
	if (ret != 0) {
		PVR_LOG(("Failed to register mfg pm0 driver"));
		return ret;
	}

	return ret;
}

void mtk_mfg_pm0_deinit(void)
{
	platform_driver_unregister(&mtk_mfg_pm0_driver);
}

static const struct of_device_id mtk_mfg_pm1_of_ids[] = {
	{ .compatible = "mediatek,mt3612-mfg-pm1",},
	{}
};

static struct platform_driver mtk_mfg_pm1_driver = {
	.probe  = mtk_mfg_pm1_probe,
	.remove = mtk_mfg_pm_remove,
	.driver = {
		.name = "mfg_pm1",
		.of_match_table = mtk_mfg_pm1_of_ids,
	}
};

int __init mtk_mfg_pm1_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&mtk_mfg_pm1_driver);
	if (ret != 0) {
		PVR_LOG(("Failed to register mfg pm1 driver"));
		return ret;
	}

	return ret;
}

void mtk_mfg_pm1_deinit(void)
{
	platform_driver_unregister(&mtk_mfg_pm1_driver);
}

static const struct of_device_id mtk_mfg_pm2_of_ids[] = {
	{ .compatible = "mediatek,mt3612-mfg-pm2",},
	{}
};

static struct platform_driver mtk_mfg_pm2_driver = {
	.probe  = mtk_mfg_pm2_probe,
	.remove = mtk_mfg_pm_remove,
	.driver = {
		.name = "mfg_pm2",
		.of_match_table = mtk_mfg_pm2_of_ids,
	}
};

int __init mtk_mfg_pm2_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&mtk_mfg_pm2_driver);
	if (ret != 0) {
		PVR_LOG(("Failed to register mfg pm2 driver"));
		return ret;
	}

	return ret;
}

void mtk_mfg_pm2_deinit(void)
{
	platform_driver_unregister(&mtk_mfg_pm2_driver);
}

static const struct of_device_id mtk_mfg_pm3_of_ids[] = {
	{ .compatible = "mediatek,mt3612-mfg-pm3",},
	{}
};

static struct platform_driver mtk_mfg_pm3_driver = {
	.probe  = mtk_mfg_pm3_probe,
	.remove = mtk_mfg_pm_remove,
	.driver = {
		.name = "mfg_pm3",
		.of_match_table = mtk_mfg_pm3_of_ids,
	}
};

int __init mtk_mfg_pm3_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&mtk_mfg_pm3_driver);
	if (ret != 0) {
		PVR_LOG(("Failed to register mfg pm3 driver"));
		return ret;
	}

	return ret;
}

void mtk_mfg_pm3_deinit(void)
{
	platform_driver_unregister(&mtk_mfg_pm3_driver);
}
