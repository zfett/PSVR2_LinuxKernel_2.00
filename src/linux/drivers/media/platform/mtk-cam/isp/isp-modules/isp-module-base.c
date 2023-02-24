/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/
#define pr_fmt(fmt) "%s:%d:" fmt, __func__, __LINE__

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <linux/of.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>

#include "../isp.h"

static int isp_module_of_parent_id(const struct device_node *np)
{
	struct device_node *pnode = NULL;
	int id;

	pnode = of_get_parent(np);
	if (!pnode) {
		pr_err("Does not find parent node\n");
		return -EINVAL;
	}

	if (of_property_read_u32(pnode, "id", &id)) {
		pr_err("Property 'parent id' missing or invalid\n");
		return -EINVAL;
	}

	return id;
}

int isp_module_power_on(const struct isp_module *module)
{
	struct isp_clk *modclk = NULL;
	int i = 0, ret = 0;

	if (!module)
		return -EINVAL;

	if (!module->module_dev)
		return -EINVAL;

	if (module->pdm_cnt) {
		pr_debug("pm_runtime_get_sync(%s) ===>\n", module->long_name);
		ret = pm_runtime_get_sync(module->module_dev);
		if (ret < 0) {
			dev_err(module->module_dev, "Failed to enable power domain: %d\n",
					ret);
			return ret;
		}
	}

	if (!module->clklist)
		return 0;

	list_for_each_entry(modclk, &module->clklist->head, list) {
		ret = clk_prepare_enable(modclk->clk);

		if (ret) {
			dev_err(module->module_dev, "Failed to enable clock(%d), ret:%d\n",
					i, ret);
			modclk->en = false;
			goto err;
		}
		modclk->en = true;
		i++;
	}
	return 0;
err:
	pr_err("clk fail, close PM (%s) <==\n", module->long_name);

	return isp_module_power_off(module);
}
EXPORT_SYMBOL(isp_module_power_on);

int isp_module_power_off(const struct isp_module *module)
{
	struct isp_clk *modclk = NULL;

	if (!module)
		return -EINVAL;

	if (!module->module_dev)
		return -EINVAL;

	if (module->clklist) {
		list_for_each_entry(modclk, &module->clklist->head, list) {
			if (modclk->en) {
				clk_disable_unprepare(modclk->clk);
				modclk->en = false;
			}
		}
	}

	if (module->pdm_cnt) {
		pr_info("pm_runtime_put(%s) <===\n", module->long_name);
		pm_runtime_put(module->module_dev);
	}

	return 0;
}
EXPORT_SYMBOL(isp_module_power_off);

struct isp_module *_isp_module_probe_index(const struct platform_device *pdev,
		int index, const char *module_name)
{
	struct device_node *np = pdev->dev.of_node;
	struct isp_module *module;
	struct resource res;
	int ret = 0;

	if (!np) {
		pr_err("No device node for mtk isp module driver\n");
		return ERR_PTR(-ENODEV);
	}

	module = kzalloc(sizeof(struct isp_module), GFP_KERNEL);
	if (!module)
		return ERR_PTR(-ENOMEM);

	ret = of_address_to_resource(np, index, &res);
	if (ret < 0) {
		pr_err("Unable to get resource\n");
		goto err;
	}

	module->hw_info.base_addr = of_iomap(np, index);
	if (!module->hw_info.base_addr) {
		pr_err("Unable to ioremap registers, of_iomap fail\n");
		ret = -ENOMEM;
		goto err;
	}

	module->hw_info.irq = irq_of_parse_and_map(np, index);
	if (module->hw_info.irq < 0) {
		pr_err("irq is incorrect\n");
		ret = -EINVAL;
		goto err;
	}

	if (of_property_read_u32_index(np,
			"module_id", index, &module->module_id)) {
		pr_err("Property 'module_id' missing or invalid\n");
		ret = -EINVAL;
		goto err;
	}

	module->hw_info.start = res.start;
	module->hw_info.end = res.end;
	module->hw_info.range = res.end - res.start + 1;
	module->parent_id = isp_module_of_parent_id(np);
	snprintf(module->name, sizeof(module->name), "%s%d", module_name, index);
	snprintf(module->long_name, sizeof(module->name), "ISP%d-%s%d",
		module->parent_id, module_name, index);

	return module;
err:
	kfree(module);

	return ERR_PTR(ret);
}
EXPORT_SYMBOL(_isp_module_probe_index);

int isp_module_grp_clk_free(struct isp_module *module)
{
	struct isp_clk_list *cl = module->clklist;
	struct isp_clk *gpclk, *tmp;

	if (!cl)
		return 0;

	list_for_each_entry_safe(gpclk, tmp, &cl->head, list) {
		list_del(&gpclk->list);
		kfree(gpclk);
	}
	kfree(cl);

	return 0;
}
EXPORT_SYMBOL(isp_module_grp_clk_free);

int isp_module_grp_clk_get(struct device *dev, struct isp_clk_list **cl)
{
	struct device_node *np = dev->of_node;
	int ret = 0;
	int i = 0;
	int clkcnt = 0;
	struct isp_clk *gpclk;

	if (!np) {
		pr_err("No device node for getting isp clock\n");
		return PTR_ERR(np);
	}

	clkcnt = of_property_count_elems_of_size(np, "clocks", 2*sizeof(u32));
	if (clkcnt < 0) {
		pr_debug("NO clocks are found in this\n");
		return -ENODATA;
	}

	(*cl) = kzalloc(sizeof(struct isp_clk_list), GFP_KERNEL);
	if (!(*cl))
		return -ENOMEM;

	INIT_LIST_HEAD(&(*cl)->head);
	(*cl)->clkcnt = clkcnt;

	for (i = 0; i < (*cl)->clkcnt; i++) {
		gpclk = kzalloc(sizeof(struct isp_clk), GFP_KERNEL);
		gpclk->clk = of_clk_get(np, i);
		if (IS_ERR(gpclk->clk)) {
			kfree(gpclk);
			dev_err(dev, "Failed to get isp clock(%d)\n", i);
			ret = PTR_ERR(gpclk->clk);
			goto of_clk_get_err;
		}
		list_add_tail(&gpclk->list, &(*cl)->head);
	}

	return 0;

of_clk_get_err:
	list_for_each_entry(gpclk, &(*cl)->head, list) {
		list_del(&gpclk->list);
		kfree(gpclk);
	}
	kfree(*cl);

	return ret;
}
EXPORT_SYMBOL(isp_module_grp_clk_get);

int isp_module_pdm_enable(struct device *dev, struct isp_module *module)
{
	int ret = 0;
	struct device_node *np = dev->of_node;
	int pmcnt;

	if (!np)
		return PTR_ERR(np);

	spin_lock(&module->pdm_lock);
	pmcnt = of_property_count_elems_of_size(np,
			"power-domains", 2 * sizeof(u32));
	if (pmcnt < 0) {
		module->pdm_cnt = 0;
		spin_unlock(&module->pdm_lock);
		return 0;
	}

	module->pdm_cnt = pmcnt;
	pr_debug("pm_runtime_enable(%s) - cnt(%d) ===>\n",
		module->long_name, module->pdm_cnt);
	spin_unlock(&module->pdm_lock);

	pm_runtime_enable(dev);

	return ret;
}
EXPORT_SYMBOL(isp_module_pdm_enable);

int isp_module_pdm_disable(struct device *dev, struct isp_module *module)
{
	int ret = 0;
	struct device_node *np = dev->of_node;

	if (!np)
		return PTR_ERR(np);

	if (!module->pdm_cnt)
		return 0;

	pr_debug("pm_runtime_disable(%s) - cnt(%d) <===\n",
		module->long_name, module->pdm_cnt);
	pm_runtime_disable(dev);

	return ret;
}
EXPORT_SYMBOL(isp_module_pdm_disable);
