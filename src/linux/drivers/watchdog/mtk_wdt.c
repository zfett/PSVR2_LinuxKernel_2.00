/*
 * Mediatek Watchdog Driver
 *
 * Copyright (C) 2014 Matthias Brugger
 *
 * Matthias Brugger <matthias.bgg@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Based on sunxi_wdt.c
 */

/**
 * @file mtk_wdt.c
 * The wdt driver provides the interfaces which will be used in linux
 * watchdog framework and register the watchdog device in linux system.
 * WDT is the system's watchdog device.
 */

/**
 * @defgroup IP_group_wdt WDT
 *   There is a Watch Dog Timers in SOC.
 *
 *   @{
 *       @defgroup IP_group_wdt_external EXTERNAL
 *         The external API document for wdt.
 *
 *         @{
 *            @defgroup IP_group_wdt_external_function 1.function
 *              none. External function reference for wdt.
 *            @defgroup IP_group_wdt_external_struct 2.structure
 *              none. External structure for wdt.
 *            @defgroup IP_group_wdt_external_typedef 3.typedef
 *              none. External typedef for wdt.
 *            @defgroup IP_group_wdt_external_enum 4.enumeration
 *              none. External enumeration for wdt.
 *            @defgroup IP_group_wdt_external_def 5.define
 *              none. External define for wdt.
 *         @}
 *
 *       @defgroup IP_group_wdt_internal INTERNAL
 *         The internal API document for wdt.
 *
 *         @{
 *            @defgroup IP_group_wdt_internal_function 1.function
 *              Internal function reference for wdt.
 *            @defgroup IP_group_wdt_internal_struct 2.structure
 *              Internal structure for wdt.
 *            @defgroup IP_group_wdt_internal_typedef 3.typedef
 *              none. Internal typedef for wdt.
 *            @defgroup IP_group_wdt_internal_enum 4.enumeration
 *              none. Internal enumeration for wdt.
 *            @defgroup IP_group_wdt_internal_def 5.define
 *              Internal define for wdt.
 *         @}
 *    @}
 */

/** @brief Some of the include files of the declaration
 * @{
 */
#include <asm/barrier.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/reboot.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/watchdog.h>
#include <linux/kmsg_dump.h>
/**
 * @}
 */

/** @ingroup IP_group_wdt_internal_def
 * @brief brief for below all define.
 * @{
 */
#define WDT_CLOCK_FREQ		32768
#define WDT_MAX_TIMEOUT		31
#define WDT_MIN_TIMEOUT		1

#define WDT_LENGTH		0x04
#define WDT_LENGTH_OFFSET	5
/** 1 tick of WDT LENGTH is 512 cycles for clock source */
#define WDT_TIMEOUT_SEC(sec, clkfreq)	((((sec) * (clkfreq)) >> 9) - 1)
#define WDT_LENGTH_KEY		0x8

#define WDT_RST			0x08
#define WDT_RST_RELOAD		0x1971

#define WDT_MODE		0x00
#define WDT_MODE_EN		(1 << 0)
#define WDT_MODE_EXT_POL_HIGH	(1 << 1)
#define WDT_MODE_EXRST_EN	(1 << 2)
#define WDT_MODE_IRQ_EN		(1 << 3)
#define WDT_MODE_AUTO_START	(1 << 4)
#define WDT_MODE_DUAL_EN	(1 << 6)
#define WDT_MODE_KEY		0x22000000

#define WDT_STATUS		0x0c
#define WDT_NONRST_REG		0x20

#define WDT_SWRST		0x14
#define WDT_SWRST_KEY		0x1209

#define DRV_NAME		"mtk-wdt"
#define DRV_VERSION		"1.0"
/**
 * @}
 */

static unsigned int wdt_clk_freq;
static bool nowayout = WATCHDOG_NOWAYOUT;
static unsigned int timeout = WDT_MAX_TIMEOUT;

#ifdef CONFIG_SIE_PRINTK_CUSTOM_LOG_DRIVER
extern int use_custom_log;
#endif
/** @ingroup IP_group_wdt_internal_struct
 * @brief This structure used to define watchdog device information
 *        used in mediatek watchdog driver.
 */
struct mtk_wdt_dev {
	/* watchdog_device register in linux watchdog framework */
	struct watchdog_device wdt_dev;
	/* mapped watchdog register base */
	void __iomem *wdt_base;
	/* watchdog virq number */
	int wdt_irq_id;
	/* restart notify handler provided by watchdog driver */
	struct notifier_block restart_handler;
};

/** @ingroup IP_group_wdt_internal_function
 * @par Description
 *     Watchdog reset handler, after this function called,
 *     system reset.
 * @param[in]
 *     this: pionter to the notifier_block.
 * @param[in]
 *     mode: not used.
 * @param[in]
 *     cmd: not used.
 * @return
 *     never return. system will reset.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_reset_handler(struct notifier_block *this, unsigned long mode,
				void *cmd)
{
	struct mtk_wdt_dev *mtk_wdt;
	void __iomem *wdt_base;
	u32 reg;

	mtk_wdt = container_of(this, struct mtk_wdt_dev, restart_handler);
	wdt_base = mtk_wdt->wdt_base;

	/* Set max delay ensure that there is no timeout during reset phase */
	reg = WDT_TIMEOUT_SEC(WDT_MAX_TIMEOUT, wdt_clk_freq);
	reg = (reg << WDT_LENGTH_OFFSET) | WDT_LENGTH_KEY;
	writel_relaxed(reg, wdt_base + WDT_LENGTH);
	writel(WDT_RST_RELOAD, wdt_base + WDT_RST);

	/* Set MODE reg to default value then SWRST*/
	reg = (WDT_MODE_EXRST_EN | WDT_MODE_EN | WDT_MODE_KEY);
	writel(reg, wdt_base + WDT_MODE);

	while (1) {
		writel(WDT_SWRST_KEY, wdt_base + WDT_SWRST);
		mdelay(5);
	}

	return NOTIFY_DONE;
}

/** @ingroup IP_group_wdt_internal_function
 * @par Description
 *     The "ping" function implemented in watchdog driver.
 *     Watchdog timer will restart after this function called.
 * @param[in]
 *     wdt_dev: pionter to the watchdog_device.
 * @return
 *     always return 0 (success).
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_wdt_ping(struct watchdog_device *wdt_dev)
{
	struct mtk_wdt_dev *mtk_wdt = watchdog_get_drvdata(wdt_dev);
	void __iomem *wdt_base = mtk_wdt->wdt_base;

	writel(WDT_RST_RELOAD, wdt_base + WDT_RST);

	return 0;
}

/** @ingroup IP_group_wdt_internal_function
 * @par Description
 *     The "set_timeout" function implemented in watchdog driver.
 *     This function will set the timeout value to watchdog hardware.
 * @param[in]
 *     wdt_dev: pionter to the watchdog_device.
 * @param[in]
 *     timeout: timeout value.
 * @return
 *     always return 0 (success).
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_wdt_set_timeout(struct watchdog_device *wdt_dev,
				unsigned int time_out)
{
	struct mtk_wdt_dev *mtk_wdt = watchdog_get_drvdata(wdt_dev);
	void __iomem *wdt_base = mtk_wdt->wdt_base;
	u32 reg;

	if (time_out > WDT_MAX_TIMEOUT)
		time_out = WDT_MAX_TIMEOUT;

	wdt_dev->timeout = time_out;

	reg = WDT_TIMEOUT_SEC(wdt_dev->timeout, wdt_clk_freq);
	reg = (reg << WDT_LENGTH_OFFSET) | WDT_LENGTH_KEY;
	writel(reg, wdt_base + WDT_LENGTH);

	mtk_wdt_ping(wdt_dev);

	return 0;
}

/** @ingroup IP_group_wdt_internal_function
 * @par Description
 *     The "stop" function implemented in watchdog driver.\n
 *     This function will stop the watchdog.
 * @param[in]
 *     wdt_dev: pionter to the watchdog_device.
 * @return
 *     always return 0 (success).
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_wdt_stop(struct watchdog_device *wdt_dev)
{
	struct mtk_wdt_dev *mtk_wdt = watchdog_get_drvdata(wdt_dev);
	void __iomem *wdt_base = mtk_wdt->wdt_base;
	u32 reg;

	reg = readl(wdt_base + WDT_MODE);
	reg &= ~WDT_MODE_EN;
	reg |= WDT_MODE_KEY;
	writel(reg, wdt_base + WDT_MODE);

	return 0;
}

/** @ingroup IP_group_wdt_internal_function
 * @par Description
 *     The "start" function implemented in watchdog driver.\n
 *     This function will start the watchdog.
 * @param[in]
 *     wdt_dev: pionter to the watchdog_device.
 * @return
 *     always return 0 (success).
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_wdt_start(struct watchdog_device *wdt_dev)
{
	u32 reg;
	struct mtk_wdt_dev *mtk_wdt = watchdog_get_drvdata(wdt_dev);
	void __iomem *wdt_base = mtk_wdt->wdt_base;
	int ret;

	ret = mtk_wdt_set_timeout(wdt_dev, wdt_dev->timeout);
	if (ret < 0)
		return ret;

	reg = readl(wdt_base + WDT_MODE);
	reg |= (WDT_MODE_DUAL_EN | WDT_MODE_IRQ_EN | WDT_MODE_EXRST_EN);
	reg &= ~WDT_MODE_EXT_POL_HIGH;
	reg |= (WDT_MODE_EN | WDT_MODE_KEY);
	writel(reg, wdt_base + WDT_MODE);

	return 0;
}

static const struct watchdog_info mtk_wdt_info = {
	.identity	= DRV_NAME,
	.options	= WDIOF_SETTIMEOUT |
			WDIOF_KEEPALIVEPING |
			WDIOF_MAGICCLOSE,
};

static const struct watchdog_ops mtk_wdt_ops = {
	.owner		= THIS_MODULE,
	.start		= mtk_wdt_start,
	.stop		= mtk_wdt_stop,
	.ping		= mtk_wdt_ping,
	.set_timeout	= mtk_wdt_set_timeout,
};

/** @ingroup IP_group_wdt_internal_function
 * @par Description
 *     The isr callback function for watchdog.
 *     Occurred a WDT timeout interrupt means that system is stucking,
 *     and need to wait system reset caused by WDT timeout again.
 *     PLEASE Ensure that the WDT interrupt is caused by WDT timeout with dual
 *     mode, then WDT status register will be retained successfully as WDT
 *     timeout after system reset.
 * @param[in]
 *     irq: irq number.
 * @param[in]
 *     dev_id: irq data, a pointer of structure mtk_wdt_dev.
 * @return
 *     always return IRQ_HANDLED (success).
 *     But in fact this function will not return because it will wait for the
 *     WDT timeout again.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static irqreturn_t mtk_wdt_isr(int irq, void *dev_id)
{
	unsigned int wdt_mode_val;
	struct task_struct *task;
	void __iomem *wdt_base = ((struct mtk_wdt_dev *)dev_id)->wdt_base;
	u32 reg;

#ifdef CONFIG_SIE_PRINTK_CUSTOM_LOG_DRIVER
	console_verbose();
	/* disable custom log driver */
	use_custom_log = 0;
#endif

	/* Save interrupt status to NONRST reg 1 */
	wdt_mode_val = readl_relaxed(wdt_base + WDT_STATUS);
	writel_relaxed(wdt_mode_val, wdt_base + WDT_NONRST_REG);

	/* Set MODE reg to default value then SWRST*/
	reg = (WDT_MODE_EXRST_EN | WDT_MODE_EN | WDT_MODE_KEY);
	writel(reg, wdt_base + WDT_MODE);

	writel(WDT_RST_RELOAD, wdt_base + WDT_RST);

	/* print tasks information */
	pr_debug("watchdog timeout isr\n");
	task = &init_task;
	for_each_process(task) {
		if (task->state == 0) {
			pr_err("PID: %d, name: %s\n backtrace:\n",
				 task->pid, task->comm);
			show_stack(task, NULL);
			pr_debug("\n");
		}
	}
	show_state();
	show_workqueue_state();
	/* TODO new kmsg_dump_reason is needed? */
	kmsg_dump(KMSG_DUMP_EMERG);

	/* wait for system reset */
	while (1)
		;

	return IRQ_HANDLED;
}

/** @ingroup IP_group_wdt_internal_function
 * @par Description
 *     Mediatek watchdog driver probe function. WDT is closed by default.
 * @param[in]
 *     pdev: pionter to platform_device.
 * @return
 *     return 0 means success.
 *     return < 0 means fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     ENOMEM: failed to allocate mtk_wdt_dev.
 *     PTR_ERR(mtk_wdt->wdt_base): failed to get WDT address.
 *     ENODEV: failed to get WDT interrupt ID.
 *     err: failed to register the interrupt vector or device for WDT
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_wdt_probe(struct platform_device *pdev)
{
	struct mtk_wdt_dev *mtk_wdt;
	struct resource *res;
	int err;
	struct clk *clk;

	mtk_wdt = devm_kzalloc(&pdev->dev, sizeof(*mtk_wdt), GFP_KERNEL);
	if (!mtk_wdt)
		return -ENOMEM;

	platform_set_drvdata(pdev, mtk_wdt);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mtk_wdt->wdt_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mtk_wdt->wdt_base))
		return PTR_ERR(mtk_wdt->wdt_base);

	mtk_wdt->wdt_irq_id = irq_of_parse_and_map(pdev->dev.of_node, 0);
	if (!mtk_wdt->wdt_irq_id) {
		pr_err("RGU get IRQ ID failed\n");
		return -ENODEV;
	}
	err = request_irq(mtk_wdt->wdt_irq_id, mtk_wdt_isr,
			 IRQF_TRIGGER_NONE, DRV_NAME, mtk_wdt);
	if (err != 0) {
		pr_err("mtk_wdt_probe: failed to request irq (%d)\n", err);
		return err;
	}

	mtk_wdt->wdt_dev.info = &mtk_wdt_info;
	mtk_wdt->wdt_dev.ops = &mtk_wdt_ops;
	mtk_wdt->wdt_dev.timeout = WDT_MAX_TIMEOUT;
	mtk_wdt->wdt_dev.max_timeout = WDT_MAX_TIMEOUT;
	mtk_wdt->wdt_dev.min_timeout = WDT_MIN_TIMEOUT;
	mtk_wdt->wdt_dev.parent = &pdev->dev;

	watchdog_init_timeout(&mtk_wdt->wdt_dev, timeout, &pdev->dev);
	watchdog_set_nowayout(&mtk_wdt->wdt_dev, nowayout);

	watchdog_set_drvdata(&mtk_wdt->wdt_dev, mtk_wdt);

	mtk_wdt_start(&mtk_wdt->wdt_dev);

	clk = devm_clk_get(&pdev->dev, "wdtclksrc");
	if (IS_ERR(clk))
		pr_err("Can't get wdt clocksource(err:%ld), default freq:%u\n",
			PTR_ERR(clk), WDT_CLOCK_FREQ);
	wdt_clk_freq = (IS_ERR(clk)) ? WDT_CLOCK_FREQ : clk_get_rate(clk);

	err = watchdog_register_device(&mtk_wdt->wdt_dev);
	if (unlikely(err))
		return err;

	mtk_wdt->restart_handler.notifier_call = mtk_reset_handler;
	mtk_wdt->restart_handler.priority = 128;
	err = register_restart_handler(&mtk_wdt->restart_handler);
	if (err)
		dev_warn(&pdev->dev,
			"cannot register restart handler (err=%d)\n", err);

	dev_info(&pdev->dev, "Watchdog enabled (timeout=%d sec, nowayout=%d)\n",
			mtk_wdt->wdt_dev.timeout, nowayout);

	return 0;
}

/** @ingroup IP_group_wdt_internal_function
 * @par Description
 *     Mediatek watchdog driver shutdown function.
 * @param[in]
 *     pdev: pionter to platform_device.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_wdt_shutdown(struct platform_device *pdev)
{
	struct mtk_wdt_dev *mtk_wdt = platform_get_drvdata(pdev);

	if (watchdog_active(&mtk_wdt->wdt_dev))
		mtk_wdt_stop(&mtk_wdt->wdt_dev);
}

/** @ingroup IP_group_wdt_internal_function
 * @par Description
 *     "remove" function for platform_driver.
 *     This function will unregister the restart handler and watchdog_device.
 * @param[in]
 *     pdev: pionter to platform_device.
 * @return
 *     always return 0 means success.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_wdt_remove(struct platform_device *pdev)
{
	struct mtk_wdt_dev *mtk_wdt = platform_get_drvdata(pdev);

	mtk_wdt_stop(&mtk_wdt->wdt_dev);

	unregister_restart_handler(&mtk_wdt->restart_handler);
	watchdog_unregister_device(&mtk_wdt->wdt_dev);

	return 0;
}

#ifdef CONFIG_PM_SLEEP

/** @ingroup IP_group_wdt_internal_function
 * @par Description
 *     Mediatek watchdog driver suspend function.
 * @param[in]
 *     pdev: pionter to platform_device.
 * @return
 *     always return 0 (success).
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_wdt_suspend(struct device *dev)
{
	struct mtk_wdt_dev *mtk_wdt = dev_get_drvdata(dev);

	if (watchdog_active(&mtk_wdt->wdt_dev))
		mtk_wdt_stop(&mtk_wdt->wdt_dev);

	return 0;
}

/** @ingroup IP_group_wdt_internal_function
 * @par Description
 *     Mediatek watchdog driver resume function.
 * @param[in]
 *     pdev: pionter to platform_device.
 * @return
 *     always return 0 (success).
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_wdt_resume(struct device *dev)
{
	struct mtk_wdt_dev *mtk_wdt = dev_get_drvdata(dev);

	if (watchdog_active(&mtk_wdt->wdt_dev)) {
		mtk_wdt_start(&mtk_wdt->wdt_dev);
		mtk_wdt_ping(&mtk_wdt->wdt_dev);
	}

	return 0;
}
#endif /* CONFIG_PM_SLEEP */

static const struct of_device_id mtk_wdt_dt_ids[] = {
	{ .compatible = "mediatek,mt3611-wdt" },
	{ .compatible = "mediatek,mt6589-wdt" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, mtk_wdt_dt_ids);

static const struct dev_pm_ops mtk_wdt_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mtk_wdt_suspend,
				mtk_wdt_resume)
};

static struct platform_driver mtk_wdt_driver = {
	.probe		= mtk_wdt_probe,
	.remove		= mtk_wdt_remove,
	.shutdown	= mtk_wdt_shutdown,
	.driver		= {
		.name		= DRV_NAME,
		.pm		= &mtk_wdt_pm_ops,
		.of_match_table	= mtk_wdt_dt_ids,
	},
};

module_platform_driver(mtk_wdt_driver);

module_param(timeout, uint, 0);
MODULE_PARM_DESC(timeout, "Watchdog heartbeat in seconds");

module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started (default="
			__MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Matthias Brugger <matthias.bgg@gmail.com>");
MODULE_DESCRIPTION("Mediatek WatchDog Timer Driver");
MODULE_VERSION(DRV_VERSION);
