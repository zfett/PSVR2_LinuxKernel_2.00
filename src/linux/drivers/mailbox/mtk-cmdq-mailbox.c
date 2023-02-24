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
 * @file mtk-cmdq-mailbox.c
 * cmdq driver.
 * This cmdq driver use native Linux mailbox framework to config\n
 * Mediatek Global Command Engine hardware.\n
 */

/**
 * @defgroup IP_group_gce GCE
 *   Global Command Engine, which is instruction based, multiple hardware\n
 *   threaded command dispatcher for multi-media hardwares.\n
 *   The pre-defined instructions include READ, WRITE, SIMPLE LOGIC,\n
 *   CONDITIONAL JUMP, WAIT FOR EVENT and CLEAR EVENT.
 *
 *   @{
 *       @defgroup IP_group_gce_external EXTERNAL
 *         The external API document for gce.\n
 *         The gce driver follow native Linux mailbox framework
 *
 *         @{
 *            @defgroup IP_group_gce_external_function 1.function
 *              none. Native Linux Driver.
 *            @defgroup IP_group_gce_external_struct 2.structure
 *              External structure used for gce helper function.
 *            @defgroup IP_group_gce_external_typedef 3.typedef
 *              none. Native Linux Driver.
 *            @defgroup IP_group_gce_external_enum 4.enumeration
 *              none. Native Linux Driver.
 *            @defgroup IP_group_gce_external_def 5.define
 *              External definition used for gce helper function.
 *         @}
 *
 *       @defgroup IP_group_gce_internal INTERNAL
 *         The internal API document for gce.\n
 *
 *         @{
 *            @defgroup IP_group_gce_internal_function 1.function
 *              Internal function for Linux mailbox framework and module init.
 *            @defgroup IP_group_gce_internal_struct 2.structure
 *              Internal structure used for gce.
 *            @defgroup IP_group_gce_internal_typedef 3.typedef
 *              none. Follow Linux coding style, no typedef used.
 *            @defgroup IP_group_gce_internal_enum 4.enumeration
 *              none. No enumeration in gce driver.
 *            @defgroup IP_group_gce_internal_def 5.define
 *              Internal definition used for gce.
 *         @}
 *     @}
 */

#include <linux/bitops.h>
#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/mailbox_controller.h>
#include <linux/mailbox/mtk-cmdq-mailbox.h>
#include <linux/soc/mediatek/mtk-cmdq.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

/** @ingroup IP_group_gce_internal_def
 * @brief gce hardware register offset definition
 * @{
 */
#define CMDQ_CURR_IRQ_STATUS		0x10
#define CMDQ_THR_SLOT_CYCLES		0x30
#define CMDQ_THR_BASE			0x100
#define CMDQ_THR_SIZE			0x80
#define CMDQ_THR_WARM_RESET		0x00
#define CMDQ_THR_ENABLE_TASK		0x04
#define CMDQ_THR_IRQ_STATUS		0x10
#define CMDQ_THR_IRQ_ENABLE		0x14
#define CMDQ_THR_CURR_ADDR		0x20
#define CMDQ_THR_END_ADDR		0x24
#define CMDQ_THR_PRIORITY		0x40
#define CMDQ_SYNC_TOKEN_UPDATE		0x68
/**
 * @}
 */

/** @ingroup IP_group_gce_internal_def
 * @brief constant definition used for gce
 * @{
 */
#define CMDQ_NO_TIMEOUT			0xffffffff
#define CMDQ_THR_ACTIVE_SLOT_CYCLES	0x3200
#define CMDQ_THR_ENABLED		0x1
#define CMDQ_THR_DISABLED		0x0
#define CMDQ_THR_SUSPEND		0x1
#define CMDQ_THR_RESUME			0x0
#define CMDQ_THR_STATUS_SUSPENDED	BIT(1)
#define CMDQ_THR_DO_WARM_RESET		BIT(0)
#define CMDQ_THR_IRQ_DONE		0x1
#define CMDQ_THR_IRQ_ERROR		0x12
#define CMDQ_THR_IRQ_EN			(CMDQ_THR_IRQ_ERROR | CMDQ_THR_IRQ_DONE)
/**
 * @}
 */

/** @ingroup IP_group_gce_internal_struct
 * @brief cmdq clock release structure.
 */
struct cmdq_clk_release {
	/** cmdq structure */
	struct cmdq		*cmdq;
	/** the work structure for release colck */
	struct work_struct	release_work;
};

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     Warm reset gce hardware thread.
 * @param[in]
 *     thread: cmdq thread structure.
 * @return
 *     0, finish resetting the gce hardware thread.\n
 *     -EFAULT, reset gce hardware thread fail.\n
 * @par Boundary case and Limitation
 *     There is software timeout protection mechanism in this function,\n
 *     timeout is 10us.
 * @par Error case and Error handling
 *     Reset gce hardware thread fail, return -EFAULT.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int cmdq_thread_reset(const struct cmdq_thread *thread)
{
	u32 warm_reset;

	writel(CMDQ_THR_DO_WARM_RESET, thread->base + CMDQ_THR_WARM_RESET);
	if (readl_poll_timeout_atomic(thread->base + CMDQ_THR_WARM_RESET,
			warm_reset, !(warm_reset & CMDQ_THR_DO_WARM_RESET),
			0, 10)) {
		dev_err(thread->cmdq->mbox.dev,
			"reset GCE thread 0x%x failed\n",
			(u32)(thread->base - thread->cmdq->base));

		return -EFAULT;
	}

	return 0;
}

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     After gce hardware finished all the commands flushed from client,\n
 *     gce hardware will sent interrupt to the CPU and trigger the\n
 *     cmdq_irq_handler. This function will be called from the irq handler.\n
 *     One flush will trigger one interrupt, this function check gce hardware\n
 *     status, dump the command buffer if there is any errors, call the\n
 *     callback function to notify client, disable the gce hardware thread\n
 *     and clock and notify the mailbox framework that gce finished all\n
 *     the works.
 * @param[in]
 *     thread: cmdq thread.
 * @param[in]
 *     shutdown: gce status.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void cmdq_pkt_exec_done(struct cmdq_thread *thread, bool shutdown)
{
	struct cmdq_cb_data cmdq_cb_data;
	bool err = false, timeout = false;
	u32 irq_flag;
	unsigned long flags, curr_pa, end_pa;

	spin_lock_irqsave(&thread->chan->lock, flags);
	if (!thread->pkt) {
		spin_unlock_irqrestore(&thread->chan->lock, flags);
		return;
	}

	if (!shutdown) {
		irq_flag = readl(thread->base + CMDQ_THR_IRQ_STATUS);
		writel(0, thread->base + CMDQ_THR_IRQ_STATUS);
		curr_pa = readl(thread->base + CMDQ_THR_CURR_ADDR);
		end_pa = readl(thread->base + CMDQ_THR_END_ADDR);

		if (curr_pa != end_pa) {
			err = true;
			timeout = true;
		} else if (irq_flag & CMDQ_THR_IRQ_ERROR) {
			err = true;
		}

		if (err)
			cmdq_buf_dump(thread, timeout,
				      readl(thread->base + CMDQ_THR_CURR_ADDR));
	}

	dma_unmap_single(thread->cmdq->mbox.dev, thread->pa_base,
			 thread->pkt->cmd_buf_size, DMA_TO_DEVICE);

	if (thread->pkt->cb.cb) {
		cmdq_cb_data.err = err;
		cmdq_cb_data.data = thread->pkt->cb.data;
		thread->pkt->cb.cb(cmdq_cb_data);
	}

	thread->pkt = NULL;
	writel(CMDQ_THR_DISABLED, thread->base + CMDQ_THR_ENABLE_TASK);
	spin_unlock_irqrestore(&thread->chan->lock, flags);
	mbox_chan_txdone(thread->chan, 0);
}

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     Cmdq driver timeout handler. Every gce hardware thread has own\n
 *     software time out protection mechanism. This function will be fired\n
 *     in timeout_ms after gce start to work. We parse timeout_ms value\n
 *     from device node in device tree, if you set the timeout_ms as\n
 *     CMDQ_NO_TIMEOUT in your device node, we won't set the time out\n
 *     mechanism, this is a special application, we can ask a gce hardware\n
 *     thread like while loop.\n
 * @param[in]
 *     data: time out data.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void cmdq_thread_handle_timeout(unsigned long data)
{
	cmdq_pkt_exec_done((struct cmdq_thread *)data, false);
}

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     Cmdq driver irq handler. After gce hardware finished all the command\n
 *     flushed from client, gce hardware will sent gic interrupt to the CPU\n
 *     and trigger this function.
 * @param[in]
 *     irq: irq information.
 * @param[in]
 *     dev: cmdq driver data.
 * @return
 *     irqreturn_t.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static irqreturn_t cmdq_irq_handler(int irq, void *dev)
{
	struct cmdq *cmdq = dev;
	unsigned long irq_status;
	int bit;

	irq_status = readl(cmdq->base + CMDQ_CURR_IRQ_STATUS) & CMDQ_IRQ_MASK;
	if (!(irq_status ^ CMDQ_IRQ_MASK))
		return IRQ_NONE;

	for_each_clear_bit(bit, &irq_status, fls(CMDQ_IRQ_MASK))
		cmdq_pkt_exec_done(&cmdq->thread[bit], false);

	return IRQ_HANDLED;
}

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     Suspend gce hardware and flush the clock release work queue.\n
 *     If we suspend gce hardware when gce has not finished all the commands\n
 *     yet, it will fire a warning message.
 * @param[in]
 *     dev: cmdq driver data.
 * @return
 *     0 for success; else the error code is returned
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int cmdq_suspend(struct device *dev)
{
	struct cmdq *cmdq = dev_get_drvdata(dev);
	int i;

	cmdq->suspended = true;
	for (i = 0; i < ARRAY_SIZE(cmdq->thread); i++)
		if (cmdq->thread[i].pkt)
			dev_warn(dev,
				 "thread %d exist running pkt in suspend\n", i);

	return 0;
}

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     Resume gce hardware.
 * @param[in]
 *     dev: cmdq driver data.
 * @return
 *     0 for success; else the error code is returned
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int cmdq_resume(struct device *dev)
{
	struct cmdq *cmdq = dev_get_drvdata(dev);

	cmdq->suspended = false;

	return 0;
}

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     This function inherited the function structure mbox_chan_ops.\n
 *     The API asks the MBOX controller driver, in atomic context try\n
 *     to transmit a message on the bus. Returns 0 if data is accepted\n
 *     for transmission, -EBUSY while rejecting if the remote hasn't yet\n
 *     read the last data sent. Actual transmission of data is reported by\n
 *     the controller via mbox_chan_txdone (if it has some TX ACK irq).\n
 *     It must not sleep. We do some setting in this function as below:\n
 *     1. Reset the gce hardware thread that the client is using now.\n
 *        Reset the gce hardware thread will clear all the setting and status.\n
 *     2. Write the CMDQ_THR_CURR_ADDR register to set the physical address of\n
 *        command buffer.\n
 *     3. Write the CMDQ_THR_END_ADDR register to set the end address of\n
 *        command buffer (command buffer beginning address +\n
 *        command buffer size).\n
 *     4. Write the CMDQ_THR_PRIORITY register to set the priority\n
 *        of gce hardware thread that the client is using now.\n
 *     5. Enable the IRQ and enable bit of the gce hardware thread\n
 *        that the client is using now.\n
 *     6. If client assigned the timeout value, cmdq will do mod_timer() here.
 * @param[in]
 *     chan: mailbox channel information.
 * @param[in]
 *     data: mailbox private data.
 * @return
 *     0 for success; else the error code is returned
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int cmdq_mbox_send_data(struct mbox_chan *chan, void *data)
{
	struct cmdq_pkt *pkt = data;
	struct cmdq_thread *thread = chan->con_priv;
	struct cmdq *cmdq;

	cmdq = dev_get_drvdata(thread->chan->mbox->dev);

	/* Client should not flush new pkts if suspended. */
	WARN_ON(cmdq->suspended);

	thread->cmdq = cmdq;
	thread->pa_base = dma_map_single(cmdq->mbox.dev, pkt->va_base,
					 pkt->cmd_buf_size, DMA_TO_DEVICE);
	thread->pkt = pkt;

	WARN_ON(cmdq_thread_reset(thread) < 0);

	writel(thread->pa_base, thread->base + CMDQ_THR_CURR_ADDR);
	writel(thread->pa_base + thread->pkt->cmd_buf_size,
	       thread->base + CMDQ_THR_END_ADDR);
	writel(thread->priority, thread->base + CMDQ_THR_PRIORITY);
	writel(CMDQ_THR_IRQ_EN, thread->base + CMDQ_THR_IRQ_ENABLE);
	writel(CMDQ_THR_ENABLED, thread->base + CMDQ_THR_ENABLE_TASK);

	if (thread->timeout_ms != CMDQ_NO_TIMEOUT)
		mod_timer(&thread->timeout,
			  jiffies + msecs_to_jiffies(thread->timeout_ms));
	return 0;
}

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     This function inherited the function structure mbox_chan_ops.\n
 *     Called when a client requests the chan. The controller could ask\n
 *     clients for additional parameters of communication to be provided\n
 *     via client's chan_data. This call may block. After this call the\n
 *     Controller must forward any data received on the chan by calling\n
 *     mbox_chan_received_data. The controller may do stuff that need to\n
 *     sleep. In this function, we just write one gce hardware register to set\n
 *     the cycles value for each active gce hardware thread time slot, if\n
 *     allocated time slot cycles are reached, the gce hardware thread will\n
 *     be preempted and yield to other threads.
 * @param[in]
 *     chan: mailbox channel information.
 * @return
 *     0 for success; else the error code is returned
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int cmdq_mbox_startup(struct mbox_chan *chan)
{
	struct cmdq *cmdq = dev_get_drvdata(chan->mbox->dev);

	writel(CMDQ_THR_ACTIVE_SLOT_CYCLES, cmdq->base + CMDQ_THR_SLOT_CYCLES);

	return 0;
}

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     This function inherited the function structure mbox_chan_ops.\n
 *     Called when a client relinquishes control of a chan. This call may\n
 *     block too. The controller must not forward any received data anymore.
 * @param[in]
 *     chan: mailbox channel information.
 * @return
 *     0 for success; else the error code is returned
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void cmdq_mbox_shutdown(struct mbox_chan *chan)
{
	cmdq_pkt_exec_done(chan->con_priv, true);
}

/** @ingroup type_group_gce_struct
 * @brief cmdq mailbox channel function structure
 */
static const struct mbox_chan_ops cmdq_mbox_chan_ops = {
	.send_data = cmdq_mbox_send_data,
	.startup = cmdq_mbox_startup,
	.shutdown = cmdq_mbox_shutdown,
};

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     Controller driver specific mapping of channel via DT.
 * @param[in]
 *     mbox: mailbox controller information.
 * @param[in]
 *     sp: p_handle arguments.
 * @return
 *     0 for success; else the error code is returned
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static struct mbox_chan *cmdq_xlate(struct mbox_controller *mbox,
		const struct of_phandle_args *sp)
{
	int ind = sp->args[0];
	struct cmdq_thread *thread;

	if (ind >= mbox->num_chans)
		return ERR_PTR(-EINVAL);

	thread = mbox->chans[ind].con_priv;
	thread->timeout_ms = sp->args[1];
	thread->priority = sp->args[2];
	thread->chan = &mbox->chans[ind];

	return &mbox->chans[ind];
}

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     Release workqueue and mailbox controller.
 * @param[in]
 *     pdev: platform device node.
 * @return
 *     return value is 0.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int cmdq_remove(struct platform_device *pdev)
{
	struct cmdq *cmdq = platform_get_drvdata(pdev);
	u32 i;

	mbox_controller_unregister(&cmdq->mbox);
	for (i = 0; i < ARRAY_SIZE(cmdq->thread); i++)
		del_timer_sync(&cmdq->thread[i].timeout);

	return 0;
}

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     Get becessary hardware information from device tree. Set the mailbox\n
 *     controller information into the structure and register this driver\n
 *     as mailbox controller driver.
 * @param[in]
 *     pdev: platform device node.
 * @return
 *     If return value is 0, function success.
 *     Otherwise, resizer probe failed.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If there is any error during the probing flow,\n
 *     system error value will be returned.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int cmdq_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	struct cmdq *cmdq;
	int err, i;

	cmdq = devm_kzalloc(dev, sizeof(*cmdq), GFP_KERNEL);
	if (!cmdq)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	cmdq->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(cmdq->base)) {
		dev_err(dev, "failed to ioremap gce\n");
		return PTR_ERR(cmdq->base);
	}

	cmdq->irq = platform_get_irq(pdev, 0);
	if (!cmdq->irq) {
		dev_err(dev, "failed to get irq\n");
		return -EINVAL;
	}
	err = devm_request_irq(dev, cmdq->irq, cmdq_irq_handler, IRQF_SHARED,
			       "mtk_cmdq", cmdq);
	if (err < 0) {
		dev_err(dev, "failed to register ISR (%d)\n", err);
		return err;
	}

	dev_dbg(dev, "cmdq device: addr:0x%p, va:0x%p, irq:%d\n",
		dev, cmdq->base, cmdq->irq);

	cmdq->mbox.dev = dev;
	cmdq->mbox.chans = devm_kcalloc(dev, CMDQ_THR_MAX_COUNT,
					sizeof(*cmdq->mbox.chans), GFP_KERNEL);
	if (!cmdq->mbox.chans)
		return -ENOMEM;

	cmdq->mbox.num_chans = CMDQ_THR_MAX_COUNT;
	cmdq->mbox.ops = &cmdq_mbox_chan_ops;
	cmdq->mbox.of_xlate = cmdq_xlate;

	/* make use of TXDONE_BY_ACK */
	cmdq->mbox.txdone_irq = true;
	cmdq->mbox.txdone_poll = false;

	for (i = 0; i < ARRAY_SIZE(cmdq->thread); i++) {
		cmdq->thread[i].base = cmdq->base + CMDQ_THR_BASE +
				CMDQ_THR_SIZE * i;
		init_timer(&cmdq->thread[i].timeout);
		cmdq->thread[i].timeout.function = cmdq_thread_handle_timeout;
		cmdq->thread[i].timeout.data = (unsigned long)&cmdq->thread[i];
		cmdq->thread[i].pkt = NULL;
		cmdq->mbox.chans[i].con_priv = &cmdq->thread[i];
	}

	err = mbox_controller_register(&cmdq->mbox);
	if (err < 0) {
		dev_err(dev, "failed to register mailbox: %d\n", err);
		return err;
	}

	platform_set_drvdata(pdev, cmdq);

	for (i = 0; i <= CMDQ_MAX_EVENT; i++)
		writel(i, cmdq->base + CMDQ_SYNC_TOKEN_UPDATE);

	return 0;
}

/** @ingroup IP_group_gce_internal_struct
 * @brief gce power management function structure.\n
 * This structure is used to register itself.
 */
static const struct dev_pm_ops cmdq_pm_ops = {
	.suspend = cmdq_suspend,
	.resume = cmdq_resume,
};

/** @ingroup IP_group_gce_internal_struct
 * @brief gce open framework device ID.\n
 * This structure is used to attach specific names to\n
 * platform device for use with device tree.
 */
static const struct of_device_id cmdq_of_ids[] = {
	{.compatible = "mediatek,mt3612-gce",},
	{}
};

/** @ingroup IP_group_gce_internal_struct
 * @brief gce platform driver structure.\n
 * This structure is used to register itself.
 */
static struct platform_driver cmdq_drv = {
	.probe = cmdq_probe,
	.remove = cmdq_remove,
	.driver = {
		.name = "mtk_cmdq",
		.pm = &cmdq_pm_ops,
		.of_match_table = cmdq_of_ids,
	}
};

builtin_platform_driver(cmdq_drv);
