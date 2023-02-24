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
#include <drm/drmP.h>
#include <linux/clk.h>
#include <linux/component.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/kthread.h>
#include "mtk_drm_crtc.h"
#include "mtk_drm_ddp_comp.h"
#include <linux/time.h>

struct mtk_disp_rdma_data {
	unsigned int fifo_size;
};
/**
 * struct mtk_disp_fake - DISP_FAKE driver structure
 * @ddp_comp - structure containing type enum and hardware resources
 * @crtc - associated crtc to report irq events to
 */
struct mtk_disp_fake {
	struct mtk_ddp_comp		ddp_comp;
	struct drm_crtc			*crtc;
	const struct mtk_disp_rdma_data	*data;
};
static inline struct mtk_disp_fake *comp_to_fake(struct mtk_ddp_comp *comp)
{
	return container_of(comp, struct mtk_disp_fake, ddp_comp);
}
static atomic_t fake_vblank_enable = ATOMIC_INIT(0);
struct drm_crtc *fake_crtc;
static struct timer_list timer;
DECLARE_WAIT_QUEUE_HEAD(trigger_fake_irq);

void my_timer_callback(unsigned long data)
{
	mtk_crtc_vblank_irq(fake_crtc);
	mod_timer(&timer, jiffies + msecs_to_jiffies(20));

}

static int mtk_disp_fake_irq_handler(void *arg)
{
	static bool thread_init;
	int ret;

	if (thread_init)
		return 0;
	thread_init = true;

	init_timer(&timer);
	setup_timer(&timer, my_timer_callback, 0);
	wait_event_interruptible(trigger_fake_irq,
					atomic_read(&fake_vblank_enable) != 0);

	mtk_crtc_vblank_irq(fake_crtc);
	ret = mod_timer(&timer, jiffies + msecs_to_jiffies(20));
	if (ret)
		DRM_ERROR("mtk_disp_fake_irq_handler: Error in mod_timer\n");

	return 0;
}


static void mtk_fake_enable_vblank(struct mtk_ddp_comp *comp,
				  struct drm_crtc *crtc,
				  struct cmdq_pkt *handle)
{
	fake_crtc = crtc;
	atomic_inc(&fake_vblank_enable);
	wake_up_interruptible(&trigger_fake_irq);
}

static const struct mtk_ddp_comp_funcs mtk_disp_fake_funcs = {
	.enable_vblank = mtk_fake_enable_vblank,
};
static int mtk_disp_fake_bind(struct device *dev, struct device *master,
			      void *data)
{
	struct mtk_disp_fake *priv = dev_get_drvdata(dev);
	struct drm_device *drm_dev = data;
	int ret;

	ret = mtk_ddp_comp_register(drm_dev, &priv->ddp_comp);
	if (ret < 0) {
		dev_err(dev, "Failed to register component %pOF: %d\n",
			dev->of_node, ret);
		return ret;
	}
	return 0;
}
static void mtk_disp_fake_unbind(struct device *dev, struct device *master,
				 void *data)
{
	struct mtk_disp_fake *priv = dev_get_drvdata(dev);
	struct drm_device *drm_dev = data;

	mtk_ddp_comp_unregister(drm_dev, &priv->ddp_comp);
}

static const struct component_ops mtk_disp_fake_component_ops = {
	.bind	= mtk_disp_fake_bind,
	.unbind = mtk_disp_fake_unbind,
};

static int mtk_disp_fake_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_disp_fake *priv;
	int comp_id;
	int ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	comp_id = mtk_ddp_comp_get_id(dev->of_node, MTK_DISP_FAKE);
	if (comp_id < 0) {
		dev_err(dev, "Failed to identify by alias: %d\n", comp_id);
		return comp_id;
	}
	ret = mtk_ddp_comp_init(dev, dev->of_node, &priv->ddp_comp, comp_id,
				&mtk_disp_fake_funcs);
	if (ret) {
		dev_err(dev, "Failed to initialize component: %d\n", ret);
		return ret;
	}
	priv->data = of_device_get_match_data(dev);
	platform_set_drvdata(pdev, priv);
	ret = component_add(dev, &mtk_disp_fake_component_ops);
	if (ret)
		dev_err(dev, "Failed to add component: %d\n", ret);
	kthread_run(mtk_disp_fake_irq_handler, NULL, "mtk_fake_irq_handler");
	return ret;
}
static int mtk_disp_fake_remove(struct platform_device *pdev)
{
	int ret;
	component_del(&pdev->dev, &mtk_disp_fake_component_ops);
	ret = del_timer(&timer);
	if (ret)
		dev_err(&pdev->dev, "The timer is still in use ...\n");

	return 0;
}
static const struct of_device_id mtk_disp_fake_driver_dt_match[] = {
	{ .compatible = "mediatek,mtxxxx-disp-fake", },
	{},
};
MODULE_DEVICE_TABLE(of, mtk_disp_fake_driver_dt_match);
struct platform_driver mtk_disp_fake_driver = {
	.probe		= mtk_disp_fake_probe,
	.remove		= mtk_disp_fake_remove,
	.driver		= {
	.name	= "mediatek-disp-fake",
	.owner	= THIS_MODULE,
	.of_match_table = mtk_disp_fake_driver_dt_match,
	},
};
