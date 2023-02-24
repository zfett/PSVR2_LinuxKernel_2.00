/*
 * Mediatek SoCs General-Purpose Timer handling.
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
 */

/**
 * @file mtk_timer.c
 * The gpt driver is a software module that responsible for generating
 * a system alarm and getting system time.
 * The gpt driver provides the interfaces which will be used in linux
 * clockevent and clocksource framework and register the gpt clockevent
 * and clocksource devices in linux system.
 */

/**
 * @defgroup IP_group_gpt GPT
 *   There are total 5 32-bit GPT (GPT1~GPT5) and 1 64-bit GPT (GPT6) in SOC.
 *   Currently only GPT1(GPT_CLK_EVT) is used as a clock event trigger, and
 *   GPT2(GPT_CLK_SRC) is used as a clock source for system by default.
 *
 *   @{
 *       @defgroup IP_group_gpt_external EXTERNAL
 *         The external API document for gpt.
 *
 *         @{
 *            @defgroup IP_group_gpt_external_function 1.function
 *              none. External function reference for gpt.
 *            @defgroup IP_group_gpt_external_struct 2.structure
 *              none. External structure for gpt.
 *            @defgroup IP_group_gpt_external_typedef 3.typedef
 *              none. External typedef for gpt.
 *            @defgroup IP_group_gpt_external_enum 4.enumeration
 *              none. External enumeration for gpt.
 *            @defgroup IP_group_gpt_external_def 5.define
 *              none. External define for gpt.
 *         @}
 *
 *       @defgroup IP_group_gpt_internal INTERNAL
 *         The internal API document for gpt.
 *
 *         @{
 *            @defgroup IP_group_gpt_internal_function 1.function
 *              Internal function reference for gpt.
 *            @defgroup IP_group_gpt_internal_struct 2.structure
 *              Internal structure for gpt.
 *            @defgroup IP_group_gpt_internal_typedef 3.typedef
 *              none. Internal typedef for gpt.
 *            @defgroup IP_group_gpt_internal_enum 4.enumeration
 *              none. Internal enumeration for gpt.
 *            @defgroup IP_group_gpt_internal_def 5.define
 *              Internal define for gpt.
 *         @}
 *    @}
 */

#define pr_fmt(fmt) KBUILD_MODNAME ":%s: " fmt, __func__
/** @brief Some of the include files of the declaration
 * @{
 */
#include <linux/clk.h>
#include <linux/clockchips.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqreturn.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/sched_clock.h>
#include <linux/slab.h>
/**
 * @}
 */

/** @ingroup IP_group_gpt_internal_def
 * @brief brief for below all define.
 * @{
 */
#define GPT_IRQ_EN_REG		0x00
#define GPT_IRQ_ENABLE(val)	BIT((val) - 1)
#define GPT_IRQ_ACK_REG		0x08
#define GPT_IRQ_ACK(val)	BIT((val) - 1)

#define TIMER_CTRL_REG(val)	(0x10 * (val))
#define TIMER_CTRL_OP(val)	(((val) & 0x3) << 4)
#define TIMER_CTRL_OP_ONESHOT	(0)
#define TIMER_CTRL_OP_REPEAT	(1)
#define TIMER_CTRL_OP_KEEPGO	(2)
#define TIMER_CTRL_OP_FREERUN	(3)
#define TIMER_CTRL_CLEAR	(2)
#define TIMER_CTRL_ENABLE	(1)
#define TIMER_CTRL_DISABLE	(0)

#define TIMER_CLK_REG(val)	(0x04 + (0x10 * (val)))
#define TIMER_CLK_SRC(val)	(((val) & 0x1) << 4)
#define TIMER_CLK_SRC_SYS13M	(0)
#define TIMER_CLK_SRC_RTC32K	(1)
#define TIMER_CLK_DIV1		(0x0)
#define TIMER_CLK_DIV2		(0x1)

#define TIMER_CNT_REG(val)	(0x08 + (0x10 * (val)))
#define TIMER_CMP_REG(val)	(0x0C + (0x10 * (val)))

#define GPT_CLK_EVT	1
#define GPT_CLK_SRC	2
/**
 * @}
 */

/** @ingroup IP_group_gpt_internal_struct
 * @brief Used to define GPT device structure.
 */
struct mtk_clock_event_device {
	/** mapped GPT register base address */
	void __iomem *gpt_base;
	/** ticks per jiffy for GPT counter */
	u32 ticks_per_jiffy;
	/** clock_event_device structure for event clock (GPT_CLK_EVT)*/
	struct clock_event_device dev;
};

static void __iomem *gpt_sched_reg __read_mostly;

/** @ingroup IP_group_gpt_internal_function
 * @par Description
 *     Get clock source counter value.
 *     This function is used as read() of the sched clock in system, and it is
 *     registered by mtk_timer_init().
 * @param[in]
 *     none.
 * @return
 *     return clock source counter value.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static u64 notrace mtk_read_sched_clock(void)
{
	return readl_relaxed(gpt_sched_reg);
}

/** @ingroup IP_group_gpt_internal_function
 * @par Description
 *     Get mtk_clock_event_device pointer by clock_event_device pointer.
 * @param[in]
 *     c: a clock_event_device structure pointer.
 * @return
 *     mtk_clock_event_device structure pointer.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static inline struct mtk_clock_event_device *to_mtk_clk(
				struct clock_event_device *c)
{
	return container_of(c, struct mtk_clock_event_device, dev);
}

/** @ingroup IP_group_gpt_internal_function
 * @par Description
 *     Stop GPT.
 * @param[in]
 *     evt: mtk_clock_event_device structure pointer.
 * @param[in]
 *     timer: the serial number of the stopped GPT.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_clkevt_time_stop(struct mtk_clock_event_device *evt, u8 timer)
{
	u32 val;

	val = readl(evt->gpt_base + TIMER_CTRL_REG(timer));
	writel(val & ~TIMER_CTRL_ENABLE, evt->gpt_base +
			TIMER_CTRL_REG(timer));
}

/** @ingroup IP_group_gpt_internal_function
 * @par Description
 *     Set GPT compare value.
 * @param[in]
 *     evt: mtk_clock_event_device structure pointer.
 * @param[in]
 *     delay: the compare value for the set GPT.
 * @param[in]
 *     timer: the serial number of the set GPT.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_clkevt_time_setup(struct mtk_clock_event_device *evt,
				unsigned long delay, u8 timer)
{
	writel(delay, evt->gpt_base + TIMER_CMP_REG(timer));
}

/** @ingroup IP_group_gpt_internal_function
 * @par Description
 *     Start GPT.
 * @param[in]
 *     evt: mtk_clock_event_device structure pointer.
 * @param[in]
 *     periodic: 0, set GPT as one-shot mode.
 *               1, set GPT as repeat mode.
 * @param[in]
 *     timer: the serial number of the started GPT.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_clkevt_time_start(struct mtk_clock_event_device *evt,
		bool periodic, u8 timer)
{
	u32 val;

	/* Acknowledge interrupt */
	writel(GPT_IRQ_ACK(timer), evt->gpt_base + GPT_IRQ_ACK_REG);

	val = readl(evt->gpt_base + TIMER_CTRL_REG(timer));

	/* Clear 2 bit timer operation mode field */
	val &= ~TIMER_CTRL_OP(0x3);

	if (periodic)
		val |= TIMER_CTRL_OP(TIMER_CTRL_OP_REPEAT);
	else
		val |= TIMER_CTRL_OP(TIMER_CTRL_OP_ONESHOT);

	writel(val | TIMER_CTRL_ENABLE | TIMER_CTRL_CLEAR,
	       evt->gpt_base + TIMER_CTRL_REG(timer));
}

/** @ingroup IP_group_gpt_internal_function
 * @par Description
 *     Shutdown clock event GPT (GPT_CLK_EVT).
 * @param[in]
 *     clk: clock_event_device structure pointer.
 * @return
 *     always return 0 (success).
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_clkevt_shutdown(struct clock_event_device *clk)
{
	mtk_clkevt_time_stop(to_mtk_clk(clk), GPT_CLK_EVT);
	return 0;
}

/** @ingroup IP_group_gpt_internal_function
 * @par Description
 *     Set and start periodic trigger task for clock event GPT (GPT_CLK_EVT).
 * @param[in]
 *     clk: clock_event_device structure pointer.
 * @return
 *     always return 0 (success).
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_clkevt_set_periodic(struct clock_event_device *clk)
{
	struct mtk_clock_event_device *evt = to_mtk_clk(clk);

	mtk_clkevt_time_stop(evt, GPT_CLK_EVT);
	mtk_clkevt_time_setup(evt, evt->ticks_per_jiffy, GPT_CLK_EVT);
	mtk_clkevt_time_start(evt, true, GPT_CLK_EVT);
	return 0;
}

/** @ingroup IP_group_gpt_internal_function
 * @par Description
 *     Set and start one-shot trigger task for clock event GPT (GPT_CLK_EVT).
 * @param[in]
 *     event: the compare value for GPT_CLK_EVT.
 * @param[in]
 *     clk: clock_event_device structure pointer.
 * @return
 *     always return 0 (success).
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_clkevt_next_event(unsigned long event,
				   struct clock_event_device *clk)
{
	struct mtk_clock_event_device *evt = to_mtk_clk(clk);

	mtk_clkevt_time_stop(evt, GPT_CLK_EVT);
	mtk_clkevt_time_setup(evt, event, GPT_CLK_EVT);
	mtk_clkevt_time_start(evt, false, GPT_CLK_EVT);

	return 0;
}

/** @ingroup IP_group_gpt_internal_function
 * @par Description
 *     Interrupt handler for clock event by GPT_CLK_EVT.
 * @param[in]
 *     irq: virq number.
 * @param[in]
 *     dev_id: device ID.
 * @return
 *     always return IRQ_HANDLED.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static irqreturn_t mtk_timer_interrupt(int irq, void *dev_id)
{
	struct mtk_clock_event_device *evt = dev_id;

	/* Acknowledge timer0 irq */
	writel(GPT_IRQ_ACK(GPT_CLK_EVT), evt->gpt_base + GPT_IRQ_ACK_REG);
	evt->dev.event_handler(&evt->dev);

	return IRQ_HANDLED;
}

/** @ingroup IP_group_gpt_internal_function
 * @par Description
 *     Set and start GPT.
 * @param[in]
 *     evt: mtk_clock_event_device structure pointer.
 * @param[in]
 *     timer: the serial number of the set GPT.
 * @param[in]
 *     option: the count mode of the set GPT.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void
mtk_timer_setup(struct mtk_clock_event_device *evt, u8 timer, u8 option)
{
	writel(TIMER_CTRL_CLEAR | TIMER_CTRL_DISABLE,
		evt->gpt_base + TIMER_CTRL_REG(timer));

	writel(TIMER_CLK_SRC(TIMER_CLK_SRC_SYS13M) | TIMER_CLK_DIV1,
			evt->gpt_base + TIMER_CLK_REG(timer));

	writel(0x0, evt->gpt_base + TIMER_CMP_REG(timer));

	writel(TIMER_CTRL_OP(option) | TIMER_CTRL_ENABLE,
			evt->gpt_base + TIMER_CTRL_REG(timer));
}

/** @ingroup IP_group_gpt_internal_function
 * @par Description
 *     Enable irq for the GPT.
 * @param[in]
 *     evt: mtk_clock_event_device pionter.
 * @param[in]
 *     timer: the serial number of the set GPT.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_timer_enable_irq(struct mtk_clock_event_device *evt, u8 timer)
{
	u32 val;

	/* Disable all interrupts */
	writel(0x0, evt->gpt_base + GPT_IRQ_EN_REG);

	/* Acknowledge all spurious pending interrupts */
	writel(0x3f, evt->gpt_base + GPT_IRQ_ACK_REG);

	val = readl(evt->gpt_base + GPT_IRQ_EN_REG);
	writel(val | GPT_IRQ_ENABLE(timer),
			evt->gpt_base + GPT_IRQ_EN_REG);
}

/** @ingroup IP_group_gpt_internal_function
 * @par Description
 *     Mediatek GPT driver init function.
 * @param[in]
 *     node: GPT device node from device tree.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void __init mtk_timer_init(struct device_node *node)
{
	struct mtk_clock_event_device *evt;
	struct resource res;
	unsigned long rate = 0;
	struct clk *clk;

	evt = kzalloc(sizeof(*evt), GFP_KERNEL);
	if (!evt) {
		pr_warn("Can't allocate mtk clock event driver struct");
		return;
	}

	evt->dev.name = "mtk_tick";
	evt->dev.rating = 300;
	evt->dev.features = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT;
	evt->dev.set_state_shutdown = mtk_clkevt_shutdown;
	evt->dev.set_state_periodic = mtk_clkevt_set_periodic;
	evt->dev.set_state_oneshot = mtk_clkevt_shutdown;
	evt->dev.tick_resume = mtk_clkevt_shutdown;
	evt->dev.set_next_event = mtk_clkevt_next_event;
	evt->dev.cpumask = cpu_possible_mask;

	evt->gpt_base = of_io_request_and_map(node, 0, "mtk-timer");
	if (IS_ERR(evt->gpt_base)) {
		pr_warn("Can't get resource\n");
		goto err_evt;
	}

	evt->dev.irq = irq_of_parse_and_map(node, 0);
	if (evt->dev.irq <= 0) {
		pr_warn("Can't parse IRQ");
		goto err_mem;
	}

	clk = of_clk_get(node, 0);
	if (IS_ERR(clk)) {
		pr_warn("Can't get timer clock");
		goto err_irq;
	}

	if (clk_prepare_enable(clk)) {
		pr_warn("Can't prepare clock");
		goto err_clk_put;
	}
	rate = clk_get_rate(clk);

	if (request_irq(evt->dev.irq, mtk_timer_interrupt,
			IRQF_TIMER | IRQF_IRQPOLL, "mtk_timer", evt)) {
		pr_warn("failed to setup irq %d\n", evt->dev.irq);
		goto err_clk_disable;
	}

	evt->ticks_per_jiffy = DIV_ROUND_UP(rate, HZ);

	/* Configure clock source */
	mtk_timer_setup(evt, GPT_CLK_SRC, TIMER_CTRL_OP_FREERUN);
	clocksource_mmio_init(evt->gpt_base + TIMER_CNT_REG(GPT_CLK_SRC),
			node->name, rate, 300, 32, clocksource_mmio_readl_up);
	gpt_sched_reg = evt->gpt_base + TIMER_CNT_REG(GPT_CLK_SRC);
	sched_clock_register(mtk_read_sched_clock, 32, rate);

	/* Configure clock event */
	mtk_timer_setup(evt, GPT_CLK_EVT, TIMER_CTRL_OP_REPEAT);
	clockevents_config_and_register(&evt->dev, rate, 0x3,
					0xffffffff);

	mtk_timer_enable_irq(evt, GPT_CLK_EVT);
	pr_info("GPT init finish\n");
	return;

err_clk_disable:
	clk_disable_unprepare(clk);
err_clk_put:
	clk_put(clk);
err_irq:
	irq_dispose_mapping(evt->dev.irq);
err_mem:
	iounmap(evt->gpt_base);
	if (!of_address_to_resource(node, 0, &res))
		release_mem_region(res.start, resource_size(&res));
err_evt:
	kfree(evt);
}
CLOCKSOURCE_OF_DECLARE(mtk_mt6577, "mediatek,mt6577-timer", mtk_timer_init);
