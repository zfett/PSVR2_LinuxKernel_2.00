/*
 * Copyright (c) 2015-2016 MediaTek Inc.
 * Author: Yong Wu <yong.wu@mediatek.com>
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
 * @file mtk-smi.c
 * This smi driver is used to control MTK smi hardware module.\n
 */

/**
 * @defgroup IP_group_SMI SMI
 *     SMI (Smart Multimedia Interface) is a MEDIATEK proprietary bus\n
 *     interface used in multimedia systems.
 *     The total number of SMI local arbiters (acronym: LARBs) depends\n
 *     on the requirement (such as power plan) of each project.\n
 *     @{
 *         @defgroup IP_group_smi_external EXTERNAL
 *             The external API document for smi.\n
 *             The smi driver follows native Linux framework
 *             @{
 *               @defgroup IP_group_smi_external_function 1.function
 *                   External function for smi.
 *               @defgroup IP_group_smi_external_struct 2.structure
 *                   External structure used for smi.
 *               @defgroup IP_group_smi_external_typedef 3.typedef
 *                   none. Native Linux Driver.
 *               @defgroup IP_group_smi_external_enum 4.enumeration
 *                   External enumeration used for smi.
 *               @defgroup IP_group_smi_external_def 5.define
 *                   External definition used for smi.
 *             @}
 *
 *         @defgroup IP_group_smi_internal INTERNAL
 *             The internal API document for smi.
 *             @{
 *               @defgroup IP_group_smi_internal_function 1.function
 *                   Internal function for smi and module init.
 *               @defgroup IP_group_smi_internal_struct 2.structure
 *                   Internal structure used for smi.
 *               @defgroup IP_group_smi_internal_typedef 3.typedef
 *                   none. Follow Linux coding style, no typedef used.
 *               @defgroup IP_group_smi_internal_enum 4.enumeration
 *                   Internal enumeration used for smi.
 *               @defgroup IP_group_smi_internal_def 5.define
 *                   Internal definition used for smi.
 *             @}
 *     @}
 */

#include <linux/clk.h>
#include <linux/component.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <soc/mediatek/smi.h>

/** @ingroup IP_group_smi_internal_def
 * @brief SMI register definition
 */
/* mt3612 */
#define SMI_LARB_CMD_THRT_CON	0x024
#define F_WR_EN		BIT(18)
#define F_RD_NU_EN	BIT(17)
#define F_RD_EN		BIT(16)

/* mt8173 */
#define SMI_LARB_MMU_EN		0xf00
#define REG_SMI_SECUR_CON_BASE		0x5c0

/* every register control 8 port, register offset 0x4 */
#define REG_SMI_SECUR_CON_OFFSET(id)	(((id) >> 3) << 2)
#define REG_SMI_SECUR_CON_ADDR(id)	\
	(REG_SMI_SECUR_CON_BASE + REG_SMI_SECUR_CON_OFFSET(id))

/*
 * every port have 4 bit to control, bit[port + 3] control virtual or physical,
 * bit[port + 2 : port + 1] control the domain, bit[port] control the security
 * or non-security.
 */
#define SMI_SECUR_CON_VAL_MSK(id)	(~(0xf << (((id) & 0x7) << 2)))
#define SMI_SECUR_CON_VAL_VIRT(id)	BIT((((id) & 0x7) << 2) + 3)
/* mt2701 domain should be set to 3 */
#define SMI_SECUR_CON_VAL_DOMAIN(id)	(0x3 << ((((id) & 0x7) << 2) + 1))

/* mt3612 */
#define SMI_LARB_NONSEC_CON(id)	(0x380 + (id * 4))
#define SMI_LARB_SEC_CON(id)	(0xf80 + (id * 4))
#define F_MMU_EN		BIT(0)

#define SMI_BUS_SEL		0x220
#define SMI_BUS_SEL_MT3612	0x5554 /* default is 0x5144 */

struct mtk_smi_larb_gen {
	bool need_larbid;
	int port_in_larb[MTK_LARB_NR_MAX + 1];
	void (*config_port)(struct device *);
};

/** @ingroup IP_group_smi_internal_struct
 * @brief SMI Driver Data Structure.
 */
struct mtk_smi {
	/** smi device node */
	struct device *dev;
	/** smi clock node */
	struct clk *clk_apb, *clk_smi, *clk_gals;
	struct clk *clk_async;	/*only needed by mt2701 */
	/** smi ao base */
	void __iomem *smi_ao_base;
	/** smi common base */
	void __iomem		*base;
};

/** @ingroup IP_group_smi_internal_struct
 * @brief SMI Larb Data Structure.
 */
struct mtk_smi_larb {		/* larb: local arbiter */
	/** smi driver data structure */
	struct mtk_smi smi;
	/** smi larb register base */
	void __iomem *base;
	/** smi_common device node */
	struct device *smi_common_dev;
	const struct mtk_smi_larb_gen *larb_gen;
	/** smi larb id */
	int larbid;
	/** smi is mm_sysram? */
	bool is_mm_sysram;
	/** smi iommu port information */
	u32 *mmu;
};

/** @ingroup IP_group_smi_internal_enum
 * @brief smi generation\n
 * value is from 0 to 1.
 */
enum mtk_smi_gen {
/** 0: SMI generation 1 */
	MTK_SMI_GEN1,
/** 1: SMI generation 2 */
	MTK_SMI_GEN2
};

/** @ingroup IP_group_smi_internal_function
 * @par Description
 *     Enable smi larb power and clock.
 * @param[in] smi: mtk smi driver data structure pointer.
 * @return
 *     0, configuration success.\n
 *     Non zero, power or clock fails to enable.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If pm_runtime_get_sync() fails, return its error code.
 *     2. If clk_prepare_enable() fails, return its error code..
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_smi_enable(const struct mtk_smi *smi)
{
	int ret;

	ret = pm_runtime_get_sync(smi->dev);
	if (ret < 0)
		return ret;

	if (!IS_ERR(smi->clk_gals)) {
		ret = clk_prepare_enable(smi->clk_gals);
		if (ret) {
			dev_err(smi->dev, "Failed to enable GALS clock\n");
			goto err_put_pm;
		}
	}

	ret = clk_prepare_enable(smi->clk_apb);
	if (ret)
		goto err_disable_gals;

	ret = clk_prepare_enable(smi->clk_smi);
	if (ret)
		goto err_disable_apb;

	return 0;

err_disable_apb:
	clk_disable_unprepare(smi->clk_apb);
err_disable_gals:
	if (!IS_ERR(smi->clk_gals))
		clk_disable_unprepare(smi->clk_gals);
err_put_pm:
	pm_runtime_put_sync(smi->dev);
	return ret;
}

/** @ingroup IP_group_smi_internal_function
 * @par Description
 *     Disable smi larb power and clock.
 * @param[in] smi: mtk smi driver data structure pointer.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_smi_disable(const struct mtk_smi *smi)
{
	clk_disable_unprepare(smi->clk_smi);
	clk_disable_unprepare(smi->clk_apb);
	if (!IS_ERR(smi->clk_gals))
		clk_disable_unprepare(smi->clk_gals);

	pm_runtime_put_sync(smi->dev);
}

/** @ingroup IP_group_smi_external_function
 * @par Description
 *     Enable smi larb power and clock, and configure its iommu.
 * @param[in] larbdev: smi larb device node.
 * @return
 *     0, configuration success.\n
 *     -ENODEV, wrong input parameter.
 *     error code from pm_runtime_get_sync().
 *     error code from clk_prepare_enable().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If larbdev's smi_common is NULL, return -ENODEV.
 *     2. If pm_runtime_get_sync() fails, return its error code.
 *     3. If clk_prepare_enable() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_smi_larb_get(struct device *larbdev)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(larbdev);
	const struct mtk_smi_larb_gen *larb_gen = larb->larb_gen;
	int ret;

	if (larb->is_mm_sysram) {
		if (!larb->smi_common_dev) {
			dev_err(larbdev, "Failed to get the sysram device\n");
			return -ENODEV;
		}
		/* Enable the larb's power and clocks */
		ret = mtk_smi_enable(&larb->smi);
		if (ret)
			return ret;
	} else {
		struct mtk_smi *common = dev_get_drvdata(larb->smi_common_dev);

		if (!common) {
			dev_err(larbdev, "Failed to get the smi_common device\n");
			return -ENODEV;
		}

		/* Enable the smi-common's power and clocks */
		ret = mtk_smi_enable(common);
		if (ret)
			return ret;

		/* Enable the larb's power and clocks */
		ret = mtk_smi_enable(&larb->smi);
		if (ret) {
			mtk_smi_disable(common);
			return ret;
		}
	}

	/* Configure the iommu info for this larb */
	larb_gen->config_port(larbdev);

	return 0;
}
EXPORT_SYMBOL_GPL(mtk_smi_larb_get);

/** @ingroup IP_group_smi_external_function
 * @par Description
 *     Disable smi larb power and clock.
 * @param[in] larbdev: smi larb device node.
 * @return
 *     void
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mtk_smi_larb_put(struct device *larbdev)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(larbdev);

	/*
	 * Don't de-configure the iommu info for this larb since there may be
	 * several modules in this larb.
	 * The iommu info will be reset after power off.
	 */

	mtk_smi_disable(&larb->smi);

	if (larb->is_mm_sysram) {
		if (!larb->smi_common_dev) {
			dev_err(larbdev, "Failed to get the sysram smi_common device\n");
			return;
		}
	} else {
		struct mtk_smi *common = dev_get_drvdata(larb->smi_common_dev);

		if (!common) {
			dev_err(larbdev, "Failed to get the smi_common device\n");
			return;
		}

		mtk_smi_disable(common);
	}
}
EXPORT_SYMBOL_GPL(mtk_smi_larb_put);

/** @ingroup IP_group_smi_internal_function
 * @par Description
 *     Binding smi larb with related IOMMU ports.
 * @param[in] dev: smi larb device node.
 * @param[in] master: unused device node.
 * @param[in] data: smi larb iommu ports information pointer.
 * @return
 *     0, bind success.\n
 *     -ENODEV, wrong input parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int
mtk_smi_larb_bind(struct device *dev, struct device *master, void *data)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(dev);
	struct mtk_smi_iommu *smi_iommu = data;
	unsigned int         i;

	if (larb->larb_gen->need_larbid) {
		larb->mmu = &smi_iommu->larb_imu[larb->larbid].mmu;
		return 0;
	}

	/*
	 * If there is no larbid property, Loop to find the corresponding
	 * iommu information.
	 */
	for (i = 0; i < smi_iommu->larb_nr; i++) {
		if (dev == smi_iommu->larb_imu[i].dev) {
			/* The 'mmu' may be updated in iommu-attach/detach. */
			larb->mmu = &smi_iommu->larb_imu[i].mmu;
			return 0;
		}
	}
	return -ENODEV;
}

static void mtk_smi_larb_config_port_mt8173(struct device *dev)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(dev);

	writel(*larb->mmu, larb->base + SMI_LARB_MMU_EN);
}

/** @ingroup IP_group_smi_internal_function
 * @par Description
 *     Configure smi larb's iommu.
 * @param[in] dev: smi larb device node.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_smi_larb_config_port_mt3612(struct device *dev)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(dev);
	struct mtk_smi *common = dev_get_drvdata(larb->smi_common_dev);
	u32 reg;
	int i;

	/* Enable SMI Throttling */
	reg = readl_relaxed(larb->base + SMI_LARB_CMD_THRT_CON);
	reg |= (F_WR_EN | F_RD_NU_EN | F_RD_EN);
	writel_relaxed(reg, larb->base + SMI_LARB_CMD_THRT_CON);

	if (!larb->mmu)
		return;

	for_each_set_bit(i, (unsigned long *)larb->mmu, 32) {
		reg = readl_relaxed(larb->base + SMI_LARB_NONSEC_CON(i));
		reg |= F_MMU_EN;
		writel(reg, larb->base + SMI_LARB_NONSEC_CON(i));
		/*
		 * Confirm the register is updated successfully or not.
		 * if the power or clk is not ready, it will fail.
		 */
		reg = readl_relaxed(larb->base + SMI_LARB_NONSEC_CON(i));
		if (!(reg & F_MMU_EN) &&
		    !(larb->larbid == 0 && (i == 8 || i == 9))) /* VPU dummy */
			dev_warn_ratelimited(dev, "mmu_en-%d confirm fail\n",
					     i);
	}
	/* Update bus_sel for smi-common. */
	writel(SMI_BUS_SEL_MT3612, common->base + SMI_BUS_SEL);
}

/** @ingroup IP_group_smi_internal_function
 * @par Description
 *     Do nothing.
 * @param[in] dev: smi larb device node.
 * @param[in] master: unused device node.
 * @param[in] data: smi larb iommu ports information pointer.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void
mtk_smi_larb_unbind(struct device *dev, struct device *master, void *data)
{
	/* Do nothing as the iommu is always enabled. */
}

static const struct component_ops mtk_smi_larb_component_ops = {
	.bind = mtk_smi_larb_bind,
	.unbind = mtk_smi_larb_unbind,
};

static const struct mtk_smi_larb_gen mtk_smi_larb_mt8173 = {
	/* mt8173 do not need the port in larb */
	.config_port = mtk_smi_larb_config_port_mt8173,
};

static const struct mtk_smi_larb_gen mtk_smi_larb_mt3612 = {
	.need_larbid = true,
	.config_port = mtk_smi_larb_config_port_mt3612,
};

static const struct of_device_id mtk_smi_larb_of_ids[] = {
	{
	 .compatible = "mediatek,mt8173-smi-larb",
	 .data = &mtk_smi_larb_mt8173},
	{
	 .compatible = "mediatek,mt3612-smi-larb",
	 .data = &mtk_smi_larb_mt3612},
	{}
};

/** @ingroup IP_group_smi_internal_function
 * @par Description
 *     Get Necessary Hardware Information from Device Tree.
 * @param[in] pdev: platform device node.
 * @return
 *     If return value is 0, function success.
 *     Otherwise, smi larb probe failed.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If there is any error in the probe flow,\n
 *     system error value will be returned.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_smi_larb_probe(struct platform_device *pdev)
{
	struct mtk_smi_larb *larb;
	struct resource *res;
	struct device *dev = &pdev->dev;
	struct device_node *smi_node;
	struct device_node *sysram_node;
	struct platform_device *smi_pdev;
	struct platform_device *sysram_pdev;
	int err;

	larb = devm_kzalloc(dev, sizeof(*larb), GFP_KERNEL);
	if (!larb)
		return -ENOMEM;

	larb->larb_gen = of_device_get_match_data(dev);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	larb->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(larb->base))
		return PTR_ERR(larb->base);

	larb->smi.clk_apb = devm_clk_get(dev, "apb");
	if (IS_ERR(larb->smi.clk_apb))
		return PTR_ERR(larb->smi.clk_apb);

	larb->smi.clk_smi = devm_clk_get(dev, "smi");
	if (IS_ERR(larb->smi.clk_smi))
		return PTR_ERR(larb->smi.clk_smi);

	/* only some smi larbs need to enable gals clock */
	larb->smi.clk_gals = devm_clk_get(dev, "gals");

	larb->smi.dev = dev;

	if (larb->larb_gen->need_larbid) {
		err = of_property_read_u32(dev->of_node, "mediatek,larb-id",
					   &larb->larbid);
		if (err) {
			dev_err(dev, "missing larbid property\n");
			return err;
		}
	}

	sysram_node = of_parse_phandle(dev->of_node, "mediatek,sysram", 0);
	if (sysram_node) {
		sysram_pdev = of_find_device_by_node(sysram_node);
		of_node_put(sysram_node);
		if (sysram_pdev) {
			larb->smi_common_dev = &sysram_pdev->dev;
		} else {
			dev_err(dev, "Failed to get the sysram device\n");
			return -EINVAL;
		}
		larb->is_mm_sysram = true;
	} else {
		smi_node = of_parse_phandle(dev->of_node, "mediatek,smi", 0);
		if (!smi_node)
			return -EINVAL;

		smi_pdev = of_find_device_by_node(smi_node);
		of_node_put(smi_node);
		if (smi_pdev) {
			if (!platform_get_drvdata(smi_pdev))
				return -EPROBE_DEFER;
			larb->smi_common_dev = &smi_pdev->dev;
		} else {
			dev_err(dev, "Failed to get the smi_common device\n");
			return -EINVAL;
		}
	}

	pm_runtime_enable(dev);
	platform_set_drvdata(pdev, larb);
	return component_add(dev, &mtk_smi_larb_component_ops);
}

/** @ingroup IP_group_smi_internal_function
 * @par Description
 *     Remove SMI Larb Hardware Information.
 * @param[in] pdev: platform device node.
 * @return
 *     If return value is 0, function success.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_smi_larb_remove(struct platform_device *pdev)
{
	pm_runtime_disable(&pdev->dev);
	component_del(&pdev->dev, &mtk_smi_larb_component_ops);
	return 0;
}

static struct platform_driver mtk_smi_larb_driver = {
	.probe = mtk_smi_larb_probe,
	.remove = mtk_smi_larb_remove,
	.driver = {
		   .name = "mtk-smi-larb",
		   .of_match_table = mtk_smi_larb_of_ids,
		   }
};

static const struct of_device_id mtk_smi_common_of_ids[] = {
	{
	 .compatible = "mediatek,mt8173-smi-common",
	 .data = (void *)MTK_SMI_GEN2},
	{
	 .compatible = "mediatek,mt3612-smi-common",
	 .data = (void *)MTK_SMI_GEN2},
	{
	 .compatible = "mediatek,mt2701-smi-common",
	 .data = (void *)MTK_SMI_GEN1},
	{}
};

/** @ingroup IP_group_smi_internal_function
 * @par Description
 *     Get Necessary Hardware Information from Device Tree.
 * @param[in] pdev: platform device node.
 * @return
 *     If return value is 0, function success.
 *     Otherwise, smi common probe failed.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If there is any error in the probe flow,\n
 *     system error value will be returned.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_smi_common_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_smi *common;
	struct resource *res;
	enum mtk_smi_gen smi_gen;
	int ret;

	common = devm_kzalloc(dev, sizeof(*common), GFP_KERNEL);
	if (!common)
		return -ENOMEM;
	common->dev = dev;

	common->clk_apb = devm_clk_get(dev, "apb");
	if (IS_ERR(common->clk_apb))
		return PTR_ERR(common->clk_apb);

	common->clk_smi = devm_clk_get(dev, "smi");
	if (IS_ERR(common->clk_smi))
		return PTR_ERR(common->clk_smi);


	/*
	 * for mtk smi gen 1, we need to get the ao(always on) base to config
	 * m4u port, and we need to enable the aync clock for transform the smi
	 * clock into emi clock domain, but for mtk smi gen2, there's no smi ao
	 * base.
	 */
	smi_gen = (enum mtk_smi_gen)of_device_get_match_data(dev);
	if (smi_gen == MTK_SMI_GEN1) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		common->smi_ao_base = devm_ioremap_resource(dev, res);
		if (IS_ERR(common->smi_ao_base))
			return PTR_ERR(common->smi_ao_base);

		common->clk_async = devm_clk_get(dev, "async");
		if (IS_ERR(common->clk_async))
			return PTR_ERR(common->clk_async);

		ret = clk_prepare_enable(common->clk_async);
		if (ret)
			return ret;
	} else {
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		common->base = devm_ioremap_resource(dev, res);
		if (IS_ERR(common->base))
			return PTR_ERR(common->base);
	}
	pm_runtime_enable(dev);
	platform_set_drvdata(pdev, common);
	return 0;
}

/** @ingroup IP_group_smi_internal_function
 * @par Description
 *     Remove SMI Coomon Hardware Information.
 * @param[in] pdev: platform device node.
 * @return
 *     If return value is 0, function success.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_smi_common_remove(struct platform_device *pdev)
{
	pm_runtime_disable(&pdev->dev);
	return 0;
}

static struct platform_driver mtk_smi_common_driver = {
	.probe = mtk_smi_common_probe,
	.remove = mtk_smi_common_remove,
	.driver = {
		   .name = "mtk-smi-common",
		   .of_match_table = mtk_smi_common_of_ids,
		   }
};

static int __init mtk_smi_init(void)
{
	int ret;

	ret = platform_driver_register(&mtk_smi_common_driver);
	if (ret != 0) {
		pr_err("Failed to register SMI driver\n");
		return ret;
	}

	ret = platform_driver_register(&mtk_smi_larb_driver);
	if (ret != 0) {
		pr_err("Failed to register SMI-LARB driver\n");
		goto err_unreg_smi;
	}
	return ret;

err_unreg_smi:
	platform_driver_unregister(&mtk_smi_common_driver);
	return ret;
}

module_init(mtk_smi_init);
