/*
 * Copyright (c) 2015 MediaTek Inc.
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
* @file mtk_dprx_os.c
*
*  OS(Linux) related function
 *
 */

/* include file  */
#include "mtk_dprx_os_int.h"
#include <soc/mediatek/mtk_dprx_info.h>
#include "mtk_dprx_drv_int.h"
#include "mtk_dprx_avc_int.h"
#include "mtk_dprx_isr_int.h"
#include "mtk_dprx_reg_int.h"
#include <soc/mediatek/mtk_dprx_if.h>
#include "mtk_dprx_cmd_int.h"

/* ====================== Global Structure ========================= */
struct mtk_dprx *pdprx;
struct platform_device *my_pdev;
bool f_pwr_on_initial;
bool f_phy_gce;

/* ====================== Function ================================= */

#ifdef CONFIG_MTK_DPRX_PHY_GCE
/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX find compatible device
 * @param[in]
 *     dev: struct device
 * @param[in]
 *     name: const char
 * @return
 *     device(struct device)\n
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static struct device *find_compatible_dev(struct device *dev, const char *name)
{
	struct device_node *node;
	struct platform_device *pdev;
	struct device *find_dev = NULL;

	node = of_find_compatible_node(NULL, NULL, name);
	if (!node) {
		dev_err(dev, "Failed to get %s node\n", name);
		return ERR_PTR(-ENOENT);
	}
	pdev = of_find_device_by_node(node);
	if (!pdev) {
		dev_err(dev, "Device dose not enable %s\n",
			node->full_name);
		find_dev = ERR_PTR(-ENXIO);
		goto find_dev_err;
	}
	if (!dev_get_drvdata(&pdev->dev)) {
		dev_err(dev, "Waiting for %s device %s\n",
			name, node->full_name);
		find_dev = ERR_PTR(-EPROBE_DEFER);
		goto find_dev_err;
	}
	find_dev = &pdev->dev;
find_dev_err:
	of_node_put(node);
	return find_dev;
}
#endif

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX driver probe function
 * @param[in]
 *     pdev: platform_device
 * @return
 *     0, probe is done.\n
 *     non zero, probe is fail.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_dprx_probe(struct platform_device *pdev)
{
	struct mtk_dprx *dprx;
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct resource *regs;
	unsigned long dprx_adr;
	int ret, i;
	struct phy *phy;
	int phy_num;

	dev_info(dev, "dprx probe start...\n");

	if (pdev == NULL) {
		dev_err(&pdev->dev, "pdev is NULL");
		return -ENXIO;
	}

	dprx = devm_kzalloc(dev, sizeof(struct mtk_dprx), GFP_KERNEL);
	if (dprx) {
		dev_dbg(dev, "Allocate dprx dev\n");
	} else {
		dev_err(dev, "Unable to allocate dprx dev\n");
		return -ENOMEM;
	}
	pdprx = dprx;

	dprx->dev = dev;

#ifdef CONFIG_MTK_DPRX_PHY_GCE
	dprx->mmsys_dev = find_compatible_dev(dev, "mediatek,mt3612-mmsyscfg");
	if (IS_ERR(dprx->mmsys_dev))
		return PTR_ERR(dprx->mmsys_dev);
	dprx->mutex_dev = find_compatible_dev(dev,
					      "mediatek,mt3612-mutex_gaze");
	if (IS_ERR(dprx->mutex_dev))
		return PTR_ERR(dprx->mutex_dev);
	f_phy_gce = 0;
#endif

	dprx->num_phys = of_count_phandle_with_args(node,
			"phys", "#phy-cells");
	if (dprx->num_phys > 0) {
		dprx->phys = devm_kcalloc(dev, dprx->num_phys,
					  sizeof(*dprx->phys), GFP_KERNEL);
		dev_dbg(dev, "dprx->num_phys = %d\n", dprx->num_phys);
		if (!dprx->phys)
			return -ENOMEM;
	} else {
		dprx->num_phys = 0;
	}

	ret = of_property_read_u32(dev->of_node, "sub_hw_nr", &dprx->sub_hw_nr);
	if (!ret) {
		dev_dbg(dev, "parsing sub_hw_nr: %d", dprx->sub_hw_nr);
	} else {
		dev_err(dev, "parsing sub_hw_nr error: %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < dprx->sub_hw_nr; i++) {
		regs = platform_get_resource(pdev, IORESOURCE_MEM, i);

		dev_dbg(dev,
			"sub hw %d adr start: 0x%lx, end: 0x%lx, name: %s\n",
			i,
			(unsigned long int)(regs->start),
			(unsigned long int)(regs->end),
			regs->name);

		dprx->regs[i] = devm_ioremap_resource(dev, regs);

		if (IS_ERR(dprx->regs[i])) {
			ret = PTR_ERR(dprx->regs[i]);
			dev_err(dev,
				"Failed to ioremap dprx %d registers: %d\n", i,
				ret);
			return PTR_ERR(dprx->regs[i]);
		}
		dprx->gce_subsys_offset[i] = regs->start & 0xffffffff;
		dev_dbg(dev, "sub adr 0x%lx --> 0x%lx , offset: 0x%x\n",
			(unsigned long int)(regs->start),
			(unsigned long int)(dprx->regs[i]),
			dprx->gce_subsys_offset[i]);
		dprx_adr = (unsigned long) dprx->regs[i];
		dev_dbg(dev, "dprx_adr: 0x%lx\n", dprx_adr);
	}

#ifdef CONFIG_COMMON_CLK_MT3612
	pm_runtime_enable(&pdev->dev);
	my_pdev = pdev;
	#ifdef CONFIG_MTK_DPRX_HDCP_KEY
	f_pwr_on_initial = 1;
	#else
	f_pwr_on_initial = 0;
	#endif

	dprx->rstc = devm_reset_control_get(dprx->dev, "dprx_rst");
	if (IS_ERR(dprx->rstc)) {
		dev_err(dev, "fail to get dprx_rst\n");
		return PTR_ERR(dprx->rstc);
	}

	dprx->ref_clk = devm_clk_get(dev, "ref_clk");
	if (IS_ERR(dprx->ref_clk)) {
		dev_err(dev, "fail to get ref_clk\n");
		return PTR_ERR(dprx->ref_clk);
	}
#endif

#ifdef CONFIG_COMMON_CLK_MT3612
	dprx->apb_rstc = devm_reset_control_get(dprx->dev, "dprx_apb_rst");
	if (IS_ERR(dprx->apb_rstc)) {
		dev_err(dev, "fail to get dprx_apb_rst\n");
		return PTR_ERR(dprx->apb_rstc);
	}
#endif

	ret = of_property_read_u32(dev->of_node, "fpga-mode", &dprx->fpga_mode);
	if (!ret) {
		dev_dbg(dev, "parsing fpga-mode: %d", dprx->fpga_mode);
	} else {
		dev_err(dev, "parsing fpga-mode error: %d\n", ret);
		return -EINVAL;
	}

	ret = of_property_read_u32(dev->of_node, "early-porting",
				   &dprx->early_porting);
	if (!ret) {
		dev_dbg(dev, "parsing early-porting: %d", dprx->early_porting);
	} else {
		dev_err(dev, "parsing early-porting error: %d\n", ret);
		return -EINVAL;
	}

	ret = of_property_read_u32(dev->of_node, "dp1p3", &dprx->dp1p3);
	if (!ret) {
		dev_dbg(dev, "parsing dp1p3: %d", dprx->dp1p3);
	} else {
		dev_err(dev, "parsing dp1p3 error: %d\n", ret);
		return -EINVAL;
	}

	ret = of_property_read_u32(dev->of_node, "dsc", &dprx->dsc);
	if (!ret) {
		dev_dbg(dev, "parsing dsc: %d", dprx->dsc);
	} else {
		dev_err(dev, "parsing dsc error: %d\n", ret);
		return -EINVAL;
	}

	ret = of_property_read_u32(dev->of_node, "fec", &dprx->fec);
	if (!ret) {
		dev_dbg(dev, "parsing fec: %d", dprx->fec);
	} else {
		dev_err(dev, "parsing fec error: %d\n", ret);
		return -EINVAL;
	}

	ret = of_property_read_u32(dev->of_node, "hdcp2p2", &dprx->hdcp2p2);
	if (!ret) {
		dev_dbg(dev, "parsing hdcp2p2: %d", dprx->hdcp2p2);
	} else {
		dev_err(dev, "parsing hdcp2p2 error: %d\n", ret);
		return -EINVAL;
	}

	ret = of_property_read_u32(dev->of_node, "yuv420", &dprx->yuv420);
	if (!ret) {
		dev_dbg(dev, "parsing yuv420: %d", dprx->yuv420);
	} else {
		dev_err(dev, "parsing yuv420 error: %d\n", ret);
		return -EINVAL;
	}

	ret = of_property_read_u32(dev->of_node, "audio", &dprx->audio);
	if (!ret) {
		dev_dbg(dev, "parsing audio: %d", dprx->audio);
	} else {
		dev_err(dev, "parsing audio error: %d\n", ret);
		return -EINVAL;
	}

	ret = of_property_read_u32(dev->of_node, "hbr3", &dprx->hbr3);
	if (!ret) {
		dev_dbg(dev, "parsing hbr3: %d", dprx->hbr3);
	} else {
		dev_err(dev, "parsing hbr3 error: %d\n", ret);
		return -EINVAL;
	}

	ret = of_property_read_u32(dev->of_node, "dprx-lanes", &dprx->lanes);
	if (ret) {
		dev_err(dev, "parsing dprx-lanes error: %d\n", ret);
		return -EINVAL;
	}
	if ((dprx->lanes == 0) || (dprx->lanes == 3) || (dprx->lanes > 4)) {
		dev_err(dev, "lane number is wrong: %d\n", dprx->lanes);
		return -EINVAL;
	}

	/* IRQ init */
	dprx->irq_num = platform_get_irq(pdev, 0);
	dev_dbg(dev, "dprx irq_num: %d\n", dprx->irq_num);

	if (dprx->irq_num < 0) {
		dev_err(dev, "failed to request dprx irq resource\n");
		ret = dprx->irq_num;
		return -EPROBE_DEFER;
	}
	dev_dbg(dev, "dprx irq num is %d\n", dprx->irq_num);

	if (&pdev->dev == NULL)
		dev_err(dev, "err!!! &pdev is NULL\n");

	dev_dbg(dev, "dprx dev name: %s", dev_name(&pdev->dev));

	ret = devm_request_irq(&pdev->dev, dprx->irq_num, mtk_dprx_irq,
			       IRQF_TRIGGER_HIGH,
			       dev_name(&pdev->dev), dprx);

	if (!ret) {
		dev_dbg(dev, "request irq okay, irq num %d\n", dprx->irq_num);
	} else {
		dev_err(dev, "failed to request mediatek dprx irq %d\n", ret);
		return -EPROBE_DEFER;
	}

	dprx->irq_data = 0;

	dprx_drv_task_init(dprx);

	#if !IS_ENABLED(CONFIG_PHY_MTK_DPRX_PHY)
	dprx->num_phys = 0;
	#endif

	for (phy_num = 0; phy_num < dprx->num_phys; phy_num++) {
		phy = devm_of_phy_get_by_index(dev, node, phy_num);
		if (IS_ERR(phy)) {
			ret = PTR_ERR(phy);
			goto exit_phys;
		}
		dprx->phys[phy_num] = phy;
	}

	platform_set_drvdata(pdev, dprx);

	/* debugfs init */
#ifdef CONFIG_DEBUG_FS
	ret = dprxcmd_debug_init(dprx);
	if (ret) {
		dev_err(dev, "[DPRX]Failed debugfs\n");
		return ret;
	}
#endif

	return 0;

exit_phys:
#ifdef CONFIG_COMMON_CLK_MT3612
	pm_runtime_disable(&pdev->dev);
#endif
	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX driver remove function
 * @param[in]
 *     pdev: platform_device
 * @return
 *     0, function is done
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_dprx_remove(struct platform_device *pdev)
{
#ifdef CONFIG_DEBUG_FS
	struct mtk_dprx *dprx;
#endif
	struct device *dev = &pdev->dev;
#ifdef CONFIG_DEBUG_FS
	dprx = dev_get_drvdata(dev);
#endif
	pr_info("dprx remove start...\n");

#ifdef CONFIG_COMMON_CLK_MT3612
	pm_runtime_disable(&pdev->dev);
#endif
	/* debugfs exit */
#ifdef CONFIG_DEBUG_FS
	dprxcmd_debug_uninit(dprx);
#endif
	mtk_dprx_phy_gce_deinit(dev);
	pr_info("dprx remove complete...\n");
	return 0;
}

/**
 * @brief a global structure to register dprx
 */
static const struct of_device_id mtk_dprx_of_match[] = {
	{.compatible = "mediatek,mt3611-dprx"},
	{.compatible = "mediatek,mt3612-dprx"},
	{},
};

MODULE_DEVICE_TABLE(of, mtk_dprx_of_match);

/**
 * @brief a global structure to register dprx
 */
struct platform_driver mtk_dprx_driver = {
	.probe = mtk_dprx_probe,
	.remove = mtk_dprx_remove,
	.driver = {
			.name = "mtk-dprx",
			.owner = THIS_MODULE,
			.of_match_table = mtk_dprx_of_match,
		},
};

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX driver module init function
 * @par Parameters
 *     none
 * @return
 *     0, function is done.\n
 *     0, function is not done.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_dprx_init(void)
{
	int ret = 0;

	pr_debug("mtk dprx driver init!\n");

	ret = platform_driver_register(&mtk_dprx_driver);
	if (ret) {
		pr_alert(" platform_driver_register failed!\n");
		return ret;
	}
	pr_debug("mtk dprx driver init ok!\n");
	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX driver module exit function
 * @par Parameters
 *     none
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dprx_exit(void)
{
	pr_debug("mtk dprx driver exit!\n");
	platform_driver_unregister(&mtk_dprx_driver);
}

module_init(mtk_dprx_init);
module_exit(mtk_dprx_exit);

MODULE_AUTHOR("jjs");
MODULE_DESCRIPTION("DisplayPort Receiver Driver");
MODULE_LICENSE("GPL v2");
