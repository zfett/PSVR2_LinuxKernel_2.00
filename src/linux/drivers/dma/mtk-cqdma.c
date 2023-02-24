/*
 * MTK DMAengine support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/**
 * @file mtk-cqdma.c
 * CQDMA Driver. Total 3 channels.
 * Support burst size : 8 Bytes
 * Support burst length : 1~7 Beats
 * 2 out-standings for 3 channels
 */

/**
 * @defgroup IP_group_cqdma CQDMA
 *   The cqdma dirver follow Linux dma framework.
 *
 *   @{
 *       @defgroup IP_group_cqdma_external EXTERNAL
 *         The external API document for cqdma. \n
 *         The cqdma driver follow Linux dma framework.
 *
 *         @{
 *            @defgroup IP_group_cqdma_external_function 1.function
 *              This is cqdma external function.
 *            @defgroup IP_group_cqdma_external_struct 2.structure
 *              This is cqdma external structure.
 *            @defgroup IP_group_cqdma_external_typedef 3.typedef
 *              This is cqdma external typedef.
 *            @defgroup IP_group_cqdma_external_enum 4.enumeration
 *              This is cqdma external enumeration.
 *            @defgroup IP_group_cqdma_external_def 5.define
 *              This is cqdma external define.
 *         @}
 *
 *       @defgroup IP_group_cqdma_internal INTERNAL
 *         The internal API document for cqdma. \n
 *
 *         @{
 *            @defgroup IP_group_cqdma_internal_function 1.function
 *              This is cqdma internal function.
 *            @defgroup IP_group_cqdma_internal_struct 2.structure
 *              This is cqdma internal structure.
 *            @defgroup IP_group_cqdma_internal_typedef 3.typedef
 *              This is cqdma internal typedef.
 *            @defgroup IP_group_cqdma_internal_enum 4.enumeration
 *              This is cqdma internal enumeration.
 *            @defgroup IP_group_cqdma_internal_def 5.define
 *              This is cqdma internal define.
 *         @}
 *     @}
 */


#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/types.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/of_dma.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/sizes.h>
#include <linux/jiffies.h>
#include "virt-dma.h"
#include "mtk-dma.h"
#include "mtk-cqdma_reg.h"


/** @ingroup IP_group_cqdma_internal_def
 * @brief use for some control bit.
 * @{
 */
#define DRV_NAME	"mtk-cqdma"
/*---------------------------------------------------------------------------*/
#define INT_FLAG_B		BIT(0)
#define INT_FLAG_CLR_B		(0)
#define INT_EN_B		BIT(0)
#define EN_B			BIT(0)
#define HARD_RST_B		BIT(1)
#define WARM_RST_B		BIT(0)
#define WARM_RST_CLR_B		(0)
#define STOP_B			BIT(0)
#define STOP_CLR_B		(0)
#define CON_BURST_LEN(x)	((x) << 16)
#define CON_FIX_EN		BIT(1)
/*---------------------------------------------------------------------------*/
/**
 * @}
 */

/** @ingroup IP_group_cqdma_internal_struct
 * @brief Internal device CQDMA struct.
 */
struct mtk_cqdmadev {
	struct dma_device ddev;
	spinlock_t lock;
	void __iomem *base;
	struct clk *clk;
	unsigned int m2m_chs;
};

/** @ingroup IP_group_cqdma_internal_enum
 * @brief CQDMA control register offset
 *
 */
enum {
	/** CQ_DMA_G_DMA_INT_FLAG */
	DMA_INT_FLAG = CQ_DMA_G_DMA_INT_FLAG,
	/** CQ_DMA_G_DMA_INT_EN */
	DMA_INT_EN = CQ_DMA_G_DMA_INT_EN,
	/** CQ_DMA_G_DMA_EN */
	DMA_EN = CQ_DMA_G_DMA_EN,
	/** CQ_DMA_G_DMA_RST */
	DMA_RST = CQ_DMA_G_DMA_RST,
	/** CQ_DMA_G_DMA_STOP */
	DMA_STOP = CQ_DMA_G_DMA_STOP,
	/** CQ_DMA_G_DMA_CON */
	DMA_CON = CQ_DMA_G_DMA_CON,
	/** CQ_DMA_G_DMA_SRC_ADDR */
	DMA_SRC_ADDR = CQ_DMA_G_DMA_SRC_ADDR,
	/** CQ_DMA_G_DMA_SRC_ADDR2 */
	DMA_SRC_ADDR2 = CQ_DMA_G_DMA_SRC_ADDR2,
	/** CQ_DMA_G_DMA_DST_ADDR */
	DMA_DST_ADDR = CQ_DMA_G_DMA_DST_ADDR,
	/** CQ_DMA_G_DMA_DST_ADDR2 */
	DMA_DST_ADDR2 = CQ_DMA_G_DMA_DST_ADDR2,
	/** CQ_DMA_G_DMA_LEN1 */
	DMA_LEN1 = CQ_DMA_G_DMA_LEN1,
	/** CQ_DMA_G_DMA_LEN2 */
	DMA_LEN2 = CQ_DMA_G_DMA_LEN2,
	/** CQ_DMA_G_DMA_JUMP_ADDR */
	DMA_JUMP_ADDR = CQ_DMA_G_DMA_JUMP_ADDR,
	/** CQ_DMA_G_DMA_INT_BUF_SIZE */
	DMA_INT_BUF_SIZE = CQ_DMA_G_DMA_INT_BUF_SIZE,
	/** CQ_DMA_G_DMA_SEC_EN */
	DMA_SEC_EN = CQ_DMA_G_DMA_SEC_EN,
};

/** @ingroup IP_group_cqdma_internal_def
 * @brief use for some process of CQDMA.
 * @{
 */
/** get CQDMA channel base address */
#define	MTK_CQDMA_CHANIO(base, n)	((base) + 0x80 * (n))
/** max dma length */
#define MAX_DMA_LEN	(SZ_1M - 1)
/** max timeout (sec) */
#define MAX_TIMEOUT_SEC	10
/** mask for LSB 32 bit */
#define MASK_LSB_32BIT 0xffffffff
/**
 * @}
 */


/** @ingroup IP_group_cqdma_internal_function
 * @par Description
 *     CQDMA convert pointer to dma device.
 * @param[in]
 *     d: dma_device pointer.
 * @return
 *     mtk_cqdmadev pointer.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static inline struct mtk_cqdmadev *to_mtk_dma_dev(struct dma_device *d)
{
	return container_of(d, struct mtk_cqdmadev, ddev);
}

/** @ingroup IP_group_cqdma_internal_function
 * @par Description
 *     CQDMA convert pointer to mtk channel.
 * @param[in]
 *     c: dma_chan pointer.
 * @return
 *     mtk_chan pointer.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static inline struct mtk_chan *to_mtk_dma_chan(struct dma_chan *c)
{
	return container_of(c, struct mtk_chan, vc.chan);
}

/** @ingroup IP_group_cqdma_internal_function
 * @par Description
 *     CQDMA convert pointer to mtk_desc.
 * @param[in]
 *     t: dma_async_tx_descriptor pointer.
 * @return
 *     mtk_desc pointer.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static inline struct mtk_desc *to_mtk_dma_desc(struct dma_async_tx_descriptor
					       *t)
{
	return container_of(t, struct mtk_desc, vd.tx);
}

/** @ingroup type_group_cqdma_InFn
 * @par Description
 *     CQDMA channel write.
 * @param[out]
 *     c: mtk_chan pointer.
 * @param[in]
 *     reg: CR offset.
 * @param[in]
 *     val: write value
 * @return
 *     void.
 * @par Boundary case and Limitation
 *     parameter "reg" must be use the enums which are define in this file.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dma_chan_write(struct mtk_chan *c, unsigned int reg,
			       unsigned int val)
{
	writel(val, c->channel_base + reg);
}

/** @ingroup IP_group_cqdma_internal_function
 * @par Description
 *     CQDMA channel read.
 * @param[in]
 *     c: mtk_chan pointer.
 * @param[in]
 *     reg: CR offset.
 * @return
 *     the CR value.
 * @par Boundary case and Limitation
 *     parameter "reg" must be use the enums which are define in this file.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static unsigned int mtk_dma_chan_read(struct mtk_chan *c, unsigned int reg)
{
	return readl(c->channel_base + reg);
}

/** @ingroup IP_group_cqdma_internal_function
 * @par Description
 *     Destroy a mtk_desc which is not attached on any tx_list
 *     free desc and all its fields.
 * @param[in]
 *     desc: the mtk_desc pointer to the descriptor to be destroyed.
 * @return
 *     None.
 * @par Boundary case and Limitation
 *     An valid descptor pointer.
 * @par Error case and Error handling
 *     Null pointer.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dma_desc_destroy(struct mtk_desc *desc)
{
	unsigned int count;
	struct mtk_dma_cb *cb;

	if (!desc)
		return;

	if (!desc->sg)
		goto free_desc;

	for (count = 0; count < desc->sg_num; count++) {
		cb = desc->sg[count].cb;
		kfree(cb);
		desc->sg[count].cb = NULL;
	}

	kfree(desc->sg);
	desc->sg = NULL;

free_desc:
	kfree(desc);
}

/** @ingroup IP_group_cqdma_internal_function
 * @par Description
 *     free descriptor.
 * @param[out]
 *     vd: virt_dma_desc pointer.
 * @return
 *     void.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dma_desc_free(struct virt_dma_desc *vd)
{
	struct mtk_desc *desc;

	if (!vd) {
		pr_err("[%s] Invalid dma_desc\n", __func__);
		return;
	}

	desc = to_mtk_dma_desc(&vd->tx);
	mtk_dma_desc_destroy(desc);
}

/** @ingroup IP_group_cqdma_internal_function
 * @par Description
 *     disable CQDMA clock
 * @param[in]
 *     mtkd: mtk_cqdmadev pointer.
 * @return
 *     void.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dma_clk_disable(struct mtk_cqdmadev *mtkd)
{
	if (!mtkd) {
		pr_err("[%s] Invalid mtk_cqdmadev\n", __func__);
		return;
	}

	/* for SoC feature */
	if (mtkd->clk)
		clk_disable_unprepare(mtkd->clk);
}

/** @ingroup IP_group_cqdma_internal_function
 * @par Description
 *     set memory to memory parameter.
 * @param[out]
 *     c: mtk_chan pointer.
 * @return
 *     void.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dma_set_m2m_param(struct mtk_chan *c)
{
	struct mtk_desc *desc_list;
	struct mtk_dma_cb *cb;
	dma_addr_t *cb_src;
	dma_addr_t *cb_dst;
	unsigned int *cb_len;
	unsigned int *cb_info;

	if (!c || !c->desc || !c->desc->sg)
		return;

	desc_list = c->desc;
	cb = desc_list->sg[desc_list->sg_idx].cb;
	cb_src = &cb->src;
	cb_dst = &cb->dst;
	cb_len = &cb->length;
	cb_info = &cb->info;

	dev_info(c->vc.chan.device->dev,
		 "[%s]ch:0x%x, src:0x%llx, dst:0x%llx, len:0x%x\n", __func__,
		 c->dma_ch, *cb_src, *cb_dst, *cb_len);

	/* src */
	mtk_dma_chan_write(c, DMA_SRC_ADDR, (*cb_src) & MASK_LSB_32BIT);
	mtk_dma_chan_write(c, DMA_SRC_ADDR2, (*cb_src) >> 32);
	/* dst */
	mtk_dma_chan_write(c, DMA_DST_ADDR, (*cb_dst) & MASK_LSB_32BIT);
	mtk_dma_chan_write(c, DMA_DST_ADDR2, (*cb_dst) >> 32);
	/* len */
	mtk_dma_chan_write(c, DMA_LEN1, *cb_len);
	/* con */
	mtk_dma_chan_write(c, DMA_CON, *cb_info);
	/* int_en */
	mtk_dma_chan_write(c, DMA_INT_EN, INT_EN_B);
	/* kick dma */
	mtk_dma_chan_write(c, DMA_EN, EN_B);
}

/** @ingroup IP_group_cqdma_internal_function
 * @par Description
 *     set dma reset by channel.
 * @param[in]
 *     chan: dma_chan pointer.
 * @return
 *     0, reset timeout.\n
 *     1, reset done.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If return value is false, try again.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static bool mtk_dma_reset(struct mtk_chan *c)
{
	unsigned long timeout;

	if (!c) {
		pr_err("[%s] Invalid argument mtk_chan\n", __func__);
		return false;
	}

	dev_info(c->vc.chan.device->dev, "[%s]DMA ch%d\n", __func__, c->dma_ch);

	mtk_dma_chan_write(c, DMA_RST, WARM_RST_B);
	timeout = jiffies + HZ * MAX_TIMEOUT_SEC;
	while (time_before(jiffies, timeout)) {
		if (mtk_dma_chan_read(c, DMA_EN) == 0)
			return true;
	};

	dev_err(c->vc.chan.device->dev, "CQDMA warm resets error(ch = %d)!!\n",
		c->dma_ch);

	return false;
}

/** @ingroup IP_group_cqdma_internal_function
 * @par Description
 *     stop dma by channel.
 * @param[in]
 *     chan: dma_chan pointer.
 * @return
 *     0, stop timeout.\n
 *     1, stop done.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If return value is false, try again.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static bool mtk_dma_stop(struct mtk_chan *c)
{
	unsigned long timeout;

	if (!c) {
		pr_err("[%s] Invalid argument mtk_chan\n", __func__);
		return false;
	}

	mtk_dma_chan_write(c, DMA_STOP, STOP_B);
	timeout = jiffies + HZ * MAX_TIMEOUT_SEC;
	while (time_before(jiffies, timeout)) {
		if (mtk_dma_chan_read(c, DMA_EN) == 0)
			return true;
	};

	dev_err(c->vc.chan.device->dev, "CQDMA stops error(ch = %d)!!\n",
		c->dma_ch);

	return false;

}

/** @ingroup IP_group_cqdma_internal_function
 * @par Description
 *     start CQDMA descriptor.
 * @param[out]
 *     c: mtk_chan pointer.
 * @return
 *     void.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dma_start_desc(struct mtk_chan *c)
{
	struct virt_dma_desc *vd;

	if (!c) {
		pr_err("[%s] Invalid argument mtk_chan\n", __func__);
		return;
	}

	vd = vchan_next_desc(&c->vc);
	if (!vd) {
		dev_info(c->vc.chan.device->dev,
			"[%s] desc list empty, ch%d\n", __func__, c->dma_ch);
		return;
	}

	c->desc = to_mtk_dma_desc(&vd->tx);
	mtk_dma_set_m2m_param(c);
}

/** @ingroup IP_group_cqdma_internal_function
 * @par Description
 *     CQDMA callback function.
 * @param[in]
 *     irq: irq number.
 * @param[out]
 *     data: void pointer.
 * @return
 *     irqreturn_t, IRQ_HANDLED
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static irqreturn_t mtk_dma_callback(int irq, void *data)
{
	struct mtk_cqdmadev *mtkd = (struct mtk_cqdmadev *)data;
	struct mtk_chan *c, *next;
	unsigned long flags;

	dev_info(mtkd->ddev.dev, "[%s]\n", __func__);

	list_for_each_entry_safe(c, next, &mtkd->ddev.channels,
				 vc.chan.device_node) {
		if (c->channel_base == NULL)
			continue;

		if (!(mtk_dma_chan_read(c, DMA_INT_FLAG) & INT_FLAG_B))
			continue;

		mtk_dma_chan_write(c, DMA_INT_FLAG, INT_FLAG_CLR_B);

		spin_lock_irqsave(&c->vc.lock, flags);
		if (!c->desc) {
			spin_unlock_irqrestore(&c->vc.lock, flags);
			continue;
		}
		c->desc->sg_idx++;
		if (c->desc->sg_idx == c->desc->sg_num) {
			list_del(&c->desc->vd.node);
			vchan_cookie_complete(&c->desc->vd);
			dev_info(mtkd->ddev.dev,
				 "\t\tDMA ch%d completes.\n",
				 c->dma_ch);
			c->desc = NULL;
			mtk_dma_start_desc(c);
		} else {
			dev_info(mtkd->ddev.dev,
				 "\t\tDMA ch%d re-fills.\n", c->dma_ch);
			mtk_dma_set_m2m_param(c);
		}
		spin_unlock_irqrestore(&c->vc.lock, flags);
	}

	return IRQ_HANDLED;
}

/** @ingroup IP_group_cqdma_internal_function
 * @par Description
 *     allocate CQDMA channel resources.
 * @param[out]
 *     chan: dma_chan pointer.
 * @return
 *     0, OK.\n
 *     1, fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_dma_alloc_chan_resources(struct dma_chan *chan)
{
	struct mtk_cqdmadev *mtkd;
	struct mtk_chan *c;
	int ret = 0;
	unsigned long flags;

	if (!chan || !chan->device) {
		pr_err("[%s] Invalid argument dma_chan\n", __func__);
		return -EINVAL;
	}

	c = to_mtk_dma_chan(chan);
	mtkd = to_mtk_dma_dev(chan->device);

	dev_dbg(mtkd->ddev.dev, "[%s]\n", __func__);

	c->dma_ch = chan->chan_id;
	c->channel_base = MTK_CQDMA_CHANIO(mtkd->base, c->dma_ch);
	ret =
	    request_irq(c->irq_number, mtk_dma_callback, 0,
			(const char *)c->irq_name, (void *)mtkd);
	if (ret)
		dev_err(mtkd->ddev.dev, "Fail to request IRQ\n");

	spin_lock_irqsave(&c->vc.lock, flags);
	dma_cookie_init(chan);
	spin_unlock_irqrestore(&c->vc.lock, flags);

	mtk_dma_reset(c);

	dev_dbg(mtkd->ddev.dev, "\t\tch = %d, ch_base = 0x%lx, irq = %d\n",
		c->dma_ch, (unsigned long int)c->channel_base, c->irq_number);

	return ret;
}

/** @ingroup IP_group_cqdma_internal_function
 * @par Description
 *     free CQDMA channel resources.
 * @param[out]
 *     chan: dma_chan pointer.
 * @return
 *     void
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dma_free_chan_resources(struct dma_chan *chan)
{
	struct mtk_cqdmadev *mtkd;
	struct mtk_chan *c;

	if (!chan || !chan->device) {
		pr_err("[%s] Invalid argument dma_chan\n", __func__);
		return;
	}

	c = to_mtk_dma_chan(chan);
	mtkd = to_mtk_dma_dev(chan->device);

	free_irq(c->irq_number, mtkd);
	c->channel_base = NULL;
	vchan_free_chan_resources(&c->vc);

	dev_dbg(mtkd->ddev.dev, "[%s]DMA ch%d.\n", __func__, c->dma_ch);
}

/** @ingroup IP_group_cqdma_internal_function
 * @par Description
 *     calculate CQDMA descriptor size.
 * @param[in]
 *     c: mtk_chan pointer.
 * @return
 *     size_t
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static size_t mtk_dma_desc_size(struct mtk_chan *c)
{
	struct mtk_desc *d;
	unsigned int i;
	size_t size = 0;

	if (!c || !c->desc) {
		pr_err("[%s] Invalid descriptor\n", __func__);
		return 0;
	}

	d = c->desc;
	for (i = d->sg_idx; i < d->sg_num; i++)
		size += d->sg[i].cb->length;

	return size;
}

/** @ingroup type_group_cqdma_InFn
 * @par Description
 *     check tx status
 * @param[out]
 *     chan: dma_chan pointer.
 * @param[in]
 *     cookie: dma_cookie_t.
 * @param[out]
 *     txstate: dma_tx_state pointer
 * @return
 *     enum dma_status
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static enum dma_status mtk_dma_tx_status(struct dma_chan *chan,
					 dma_cookie_t cookie,
					 struct dma_tx_state *txstate)
{
	struct mtk_chan *c;
	struct virt_dma_desc *vd;
	enum dma_status ret;
	unsigned long flags;

	if (!chan)
		return DMA_ERROR;

	c = to_mtk_dma_chan(chan);

	ret = dma_cookie_status(chan, cookie, txstate);

	dev_dbg(chan->device->dev, "\t\tDMA ch%d is %s\n", c->dma_ch,
		((ret == 0) ? "completion" : "on process"));

	if (ret == DMA_COMPLETE || !txstate)
		return ret;

	spin_lock_irqsave(&c->vc.lock, flags);
	vd = vchan_find_desc(&c->vc, cookie);
	if (vd && c->desc == to_mtk_dma_desc(&vd->tx))
		txstate->residue = mtk_dma_desc_size(c);
	else
		txstate->residue = 0;
	spin_unlock_irqrestore(&c->vc.lock, flags);

	return ret;
}

/** @ingroup IP_group_cqdma_internal_function
 * @par Description
 *     count CQDMA frame length.
 * @param[in]
 *     len: transmission length.
 * @param[in]
 *     max_len: CQDMA max transmission length.
 * @return
 *     size_t.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static inline size_t mtk_dma_frames_for_length(size_t len, size_t max_len)
{
	return DIV_ROUND_UP(len, max_len);
}

/** @ingroup IP_group_cqdma_internal_function
 * @par Description
 *     prepare CQDMA memory copy.
 * @param[in]
 *     chan: dma_chan pointer.
 * @param[in]
 *     dst: destination.
 * @param[in]
 *     src: source.
 * @param[in]
 *     len: transmission length.
 * @param[in]
 *     flags: flags.
 * @return
 *     dma_async_tx_descriptor, OK.
 *     NULL, process NG.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static struct dma_async_tx_descriptor *mtk_dma_prep_dma_memcpy(struct dma_chan
							       *chan,
							       dma_addr_t dst,
							       dma_addr_t src,
							       size_t len,
							       unsigned long
							       flags)
{
	struct mtk_chan *c;
	struct mtk_desc *d;
	struct mtk_dma_cb *control_block;
	unsigned int count;
	size_t max_len = MAX_DMA_LEN, frames;

	if (!chan)
		return NULL;

	c = to_mtk_dma_chan(chan);

	dev_info(chan->device->dev, "[%s]\n", __func__);

	/* src, dst or len is not null */
	if (!src || !dst || !len)
		return NULL;

	frames = mtk_dma_frames_for_length(len, max_len);
	if (!frames)
		return NULL;

	/* allocate and setup the descriptor */
	d = kmalloc(sizeof(*d), GFP_KERNEL);
	if (!d)
		return NULL;

	/* c->desc = d; */
	d->c = c;
	d->sg_idx = 0;
	d->sg_num = frames;
	d->dir = DMA_MEM_TO_MEM;

	d->sg = kcalloc(d->sg_num, sizeof(*d->sg), GFP_KERNEL);
	if (!d->sg) {
		kfree(d);
		return NULL;
	}

	for (count = 0; count < frames; count++) {
		control_block = kzalloc(sizeof(struct mtk_dma_cb), GFP_KERNEL);
		if (!control_block) {
			dev_err(chan->device->dev,
				"\t\tFail to locate mtk_dma_cb!!\n");
			d->sg_num = count;
			goto error_sg;
		}

		control_block->info = CON_BURST_LEN(7);
		control_block->src = src + (count * max_len);
		control_block->dst = dst + (count * max_len);
		control_block->length =
		    min_t(u32, (len - (count * max_len)), max_len);
		control_block->next = 0;
		d->sg[count].cb = control_block;
	}

	dev_info(chan->device->dev, "\t\tch: %d, dma_len: 0x%zx, sg_num: %d\n",
		 c->dma_ch, len, d->sg_num);

	return vchan_tx_prep(&c->vc, &d->vd, flags);

error_sg:
	mtk_dma_desc_destroy(d);
	return NULL;
}

/** @ingroup IP_group_cqdma_internal_function
 * @par Description
 *     prepare CQDMA memory set.
 * @param[in]
 *     chan: dma_chan pointer.
 * @param[in]
 *     dst: destination.
 * @param[in]
 *     value: this value will be fill to destination.
 * @param[in]
 *     len: transmission length.
 * @param[in]
 *     flags: flags.
 * @return
 *     dma_async_tx_descriptor, OK.
 *     NULL, process NG.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static struct dma_async_tx_descriptor *mtk_dma_prep_dma_memset(struct dma_chan
							       *chan,
							       dma_addr_t dst,
							       int value,
							       size_t len,
							       unsigned long
							       flags)
{
	struct mtk_chan *c;
	struct mtk_desc *d;
	struct mtk_dma_cb *control_block;
	unsigned int count, frames;
	size_t max_len = MAX_DMA_LEN;

	if (!chan)
		return NULL;

	c = to_mtk_dma_chan(chan);

	dev_info(chan->device->dev, "[%s]\n", __func__);

	/* src, dst or len is not null */
	if (!dst || !len)
		return NULL;

	frames = mtk_dma_frames_for_length(len, max_len);
	if (!frames)
		return NULL;

	/* allocate and setup the descriptor */
	d = kmalloc(sizeof(*d), GFP_KERNEL);
	if (!d)
		return NULL;

	d->c = c;
	d->sg_idx = 0;
	d->sg_num = frames;
	d->dir = DMA_MEM_TO_MEM;

	d->sg = kcalloc(d->sg_num, sizeof(*d->sg), GFP_KERNEL);
	if (!d->sg) {
		kfree(d);
		return NULL;
	}

	for (count = 0; count < frames; count++) {
		control_block = kzalloc(sizeof(struct mtk_dma_cb), GFP_KERNEL);

		if (!control_block) {
			d->sg_num = count;
			goto error_sg;
		}

		control_block->info = CON_BURST_LEN(7) | CON_FIX_EN;
		control_block->src = value;
		control_block->dst = dst + (count * max_len);
		control_block->length =
		    min_t(u32, (len - (count * max_len)), max_len);
		control_block->next = 0;
		d->sg[count].cb = control_block;
	}

	dev_info(chan->device->dev, "\t\tch: %d, dma_len: 0x%zx, sg_num: %u\n",
		 c->dma_ch, len, d->sg_num);

	return vchan_tx_prep(&c->vc, &d->vd, flags);

error_sg:
	dev_err(chan->device->dev, "\t\tFail to locate mtk_dma_cb!!\n");
	mtk_dma_desc_destroy(d);
	return NULL;
}

/** @ingroup IP_group_cqdma_internal_function
 * @par Description
 *     issue CQDMA all transmission to pending list.
 * @param[in]
 *     chan: dma_chan pointer.
 * @return
 *     void.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dma_issue_pending(struct dma_chan *chan)
{
	struct mtk_chan *c;
	unsigned long flags;

	if (!chan)
		return;

	c = to_mtk_dma_chan(chan);

	dev_info(chan->device->dev, "[%s]DMA ch%d issues.\n", __func__,
		 c->dma_ch);

	spin_lock_irqsave(&c->vc.lock, flags);
	if (vchan_issue_pending(&c->vc) && !c->desc)
		mtk_dma_start_desc(c);
	else {
		dev_info(chan->device->dev, "\t\tDMA ch%d is not issdued!!\n",
			 c->dma_ch);
	}

	spin_unlock_irqrestore(&c->vc.lock, flags);
}

/** @ingroup IP_group_cqdma_internal_function
 * @par Description
 *     terminate all transmission.
 * @param[in]
 *     chan: dma_chan pointer.
 * @return
 *     0, always return 0.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_dma_terminate_all(struct dma_chan *chan)
{
	struct mtk_chan *c;
	struct mtk_cqdmadev *mtkd;
	unsigned long flags;
	LIST_HEAD(head);

	if (!chan)
		return -1;

	c = to_mtk_dma_chan(chan);
	mtkd = to_mtk_dma_dev(c->vc.chan.device);

	dev_info(chan->device->dev, "[%s]DMA ch%d terminates.\n", __func__,
		 c->dma_ch);

	spin_lock_irqsave(&c->vc.lock, flags);

	/* Prevent this channel being scheduled */
	spin_lock(&mtkd->lock);
	list_del_init(&c->node);
	spin_unlock(&mtkd->lock);

	if (c->desc) {
		mtk_dma_stop(c);
		c->desc = NULL;
	}
	vchan_get_all_descriptors(&c->vc, &head);
	spin_unlock_irqrestore(&c->vc.lock, flags);
	vchan_dma_desc_free_list(&c->vc, &head);

	return 0;
}

/** @ingroup IP_group_cqdma_internal_function
 * @par Description
 *     initial CQDMA channel.
 * @param[out]
 *     mtkd: mtk_cqdmadev pointer.
 * @return
 *     c, mtk_chan pointer.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static struct mtk_chan *mtk_dma_chan_init(struct mtk_cqdmadev *mtkd)
{
	struct mtk_chan *c;

	c = devm_kzalloc(mtkd->ddev.dev, sizeof(*c), GFP_KERNEL);
	if (!c)
		return NULL;

	c->vc.desc_free = mtk_dma_desc_free;
	vchan_init(&c->vc, &mtkd->ddev);
	spin_lock_init(&c->lock);
	INIT_LIST_HEAD(&c->node);

	return c;
}

/** @ingroup IP_group_cqdma_internal_function
 * @par Description
 *     release CQDMA.
 * @param[out]
 *     mtkd: mtk_cqdmadev pointer.
 * @return
 *     void.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dma_free(struct mtk_cqdmadev *mtkd)
{
	while (!list_empty(&mtkd->ddev.channels)) {
		struct mtk_chan *c = list_first_entry(&mtkd->ddev.channels,
						      struct mtk_chan,
						      vc.chan.device_node);

		list_del(&c->vc.chan.device_node);
		tasklet_kill(&c->vc.task);
		devm_kfree(mtkd->ddev.dev, c);
	}
}

static const struct of_device_id mtk_cqdma_of_match[] = {
	{.compatible = "mediatek,mt3612-cqdma",},
	{ /* sentinel */ },
};

MODULE_DEVICE_TABLE(of, mtk_cqdma_of_match);

/** @ingroup IP_group_cqdma_internal_function
 * @par Description
 *     probe device CQDMA.
 * @param[out]
 *     pdev: platform_device pointer.
 * @return
 *     0, OK.\n
 *     1, fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_dma_probe(struct platform_device *pdev)
{
	struct mtk_cqdmadev *mtkd;
	struct resource *res;
	unsigned int m2m_chs;
	int rc, i;
	struct mtk_chan *c;
	const char *s;
	struct clk *clk;

	/* Set the dma mask bits */
	rc = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
	if (rc)
		return rc;

	/* Check DMA Probe */
	dev_info(&pdev->dev, "[%s]Starting mtk CQDMA driver.", __func__);

	/* DMA channel num from DTS */
	if (of_property_read_u32
	    (pdev->dev.of_node, "mediatek,cqdma-ch-num", &m2m_chs)) {
		dev_err(&pdev->dev, "\t\tFailed to get cqdma channel num!!\n");
		return -EINVAL;
	}

	/* Allocate memory and initialize the DMA engine */
	mtkd = devm_kzalloc(&pdev->dev, sizeof(*mtkd), GFP_KERNEL);
	if (!mtkd) {
		/* dev_err(&pdev->dev,
		 *      "\t\tFailed to memory locate mtk_cqdmadev!!\n");
		 */
		return -ENOMEM;
	}

	clk = devm_clk_get(&pdev->dev, "cqdmaclk");
	if (IS_ERR(clk)) {
		if (of_find_property(pdev->dev.of_node, "clocks", NULL)) {
			dev_err(&pdev->dev, "fail to get clock\n");
			return -ENODEV;
		}
		dev_info(&pdev->dev, "No available clocks in DTS\n");
	} else {
		mtkd->clk = clk;
		rc = clk_prepare_enable(clk);
		if (rc) {
			dev_err(&pdev->dev, "failed to enable clock\n");
			return rc;
		}
	}

	/* Request and map I/O memory */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev,
			"\t\tFailed to get cqdma base address resource!!\n");
		return -ENODEV;
	}

	/* Get DMA base address from DTS */
	mtkd->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mtkd->base)) {
		dev_err(&pdev->dev,
			"\t\tFailed to remap cqdma base address!!\n");
		return PTR_ERR(mtkd->base);
	}

	/* memory transfer */
	dma_cap_set(DMA_MEMCPY, mtkd->ddev.cap_mask);
	dma_cap_set(DMA_MEMSET, mtkd->ddev.cap_mask);

	/* Must be probed for DMA */
	mtkd->ddev.device_alloc_chan_resources = mtk_dma_alloc_chan_resources;
	mtkd->ddev.device_free_chan_resources = mtk_dma_free_chan_resources;
	mtkd->ddev.device_tx_status = mtk_dma_tx_status;
	mtkd->ddev.device_issue_pending = mtk_dma_issue_pending;

	/* For memory to memory copy */
	if (dma_has_cap(DMA_MEMCPY, mtkd->ddev.cap_mask))
		mtkd->ddev.device_prep_dma_memcpy = mtk_dma_prep_dma_memcpy;

	/* For memory to memory copy */
	if (dma_has_cap(DMA_MEMSET, mtkd->ddev.cap_mask))
		mtkd->ddev.device_prep_dma_memset = mtk_dma_prep_dma_memset;

	/* Additional flag. */
	/* abort all transfers on a given channel */
	mtkd->ddev.device_terminate_all = mtk_dma_terminate_all;

	mtkd->ddev.src_addr_widths = BIT(DMA_SLAVE_BUSWIDTH_8_BYTES);
	mtkd->ddev.dst_addr_widths = BIT(DMA_SLAVE_BUSWIDTH_8_BYTES);
	mtkd->ddev.directions = BIT(DMA_MEM_TO_MEM);
	mtkd->ddev.residue_granularity = DMA_RESIDUE_GRANULARITY_DESCRIPTOR;
	mtkd->ddev.dev = &pdev->dev;

	INIT_LIST_HEAD(&mtkd->ddev.channels);
	spin_lock_init(&mtkd->lock);
	platform_set_drvdata(pdev, mtkd);
	mtkd->m2m_chs = m2m_chs;

	for (i = 0; i < (mtkd->m2m_chs); i++) {
		c = mtk_dma_chan_init(mtkd);
		if (!c) {
			dev_err(&pdev->dev,
				"\t\tmtk_dma_chan_init function fails!!\n");
			goto err_no_dma;
		}

		c->irq_number = platform_get_irq(pdev, i);

		if (c->irq_number < 0) {
			dev_err(&pdev->dev, "\t\tCannot claim IRQ\n");
			rc = -EINVAL;
			goto err_no_dma;
		}
		if (of_property_read_string_index
		    (pdev->dev.of_node, "interrupt-names", i, &s))
			continue;
		else {
			strcpy(c->irq_name, s);
		}
	}

	dev_info(&pdev->dev, "\t\tInitialized %d CQDMA channels\n",
		 (mtkd->m2m_chs));

	/* Register the DMA engine with the core */
	rc = dma_async_device_register(&mtkd->ddev);
	if (rc) {
		dev_err(&pdev->dev,
			"\t\tFailed to register slave DMA engine device: %d\n",
			rc);
		goto err_no_dma;
	}
	dev_info(&pdev->dev,
		 "\t\tLoad cqdma engine driver(phys: 0x%lx, mmio: 0x%p, size: 0x%lx).\n",
		 (unsigned long int)res->start,
		  mtkd->base,
		  (unsigned long int)(res->end - res->start + 1));
	return 0;

err_no_dma:
	dev_err(&pdev->dev, "\t\tFail to load cqdma engine driver!!\n");
	mtk_dma_free(mtkd);
	devm_kfree(&pdev->dev, mtkd);
	return rc;
}

/** @ingroup IP_group_cqdma_internal_function
 * @par Description
 *     remove device CQDMA.
 * @param[out]
 *     pdev: platform_device pointer.
 * @return
 *     0, always return 0
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_dma_remove(struct platform_device *pdev)
{
	struct mtk_cqdmadev *mtkd = platform_get_drvdata(pdev);

	if (pdev->dev.of_node)
		of_dma_controller_free(pdev->dev.of_node);

	mtk_dma_clk_disable(mtkd);

	dma_async_device_unregister(&mtkd->ddev);

	mtk_dma_free(mtkd);

	return 0;
}

static struct platform_driver mtk_dma_driver = {
	.probe = mtk_dma_probe,
	.remove = mtk_dma_remove,
	.driver = {
		   .name = DRV_NAME,
		   .of_match_table = of_match_ptr(mtk_cqdma_of_match),
		   },
};

/** @ingroup IP_group_cqdma_internal_function
 * @par Description
 *     register device CQDMA.
 * @param Parameters
 *     none.
 * @return
 *     return platform_driver_register.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_dma_init(void)
{
	return platform_driver_register(&mtk_dma_driver);
}

subsys_initcall(mtk_dma_init);

/** @ingroup IP_group_cqdma_internal_function
 * @par Description
 *     unregister device CQDMA.
 * @param Parameters
 *     none.
 * @return
 *     void.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void __exit mtk_dma_exit(void)
{
	platform_driver_unregister(&mtk_dma_driver);
}

module_exit(mtk_dma_exit);

MODULE_DESCRIPTION("MediaTek MTK CQDMA Controller Driver");
MODULE_LICENSE("GPL v2");
