/*
 * MTK DMAengine support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

 /**
 * @file mtk-apdma.c
 * APDMA Driver.
 * UART *10 (TX/RX channel are separated, Total 20 channels)
 * I2C * 19 channels
 * Support burst size : 8 Bytes
 * Support burst length : 1 Beat
 */

 /**
 * @defgroup IP_group_apdma APDMA
 *   The apdma dirver follow Linux dma framework.
 *
 *   @{
 *       @defgroup IP_group_apdma_external EXTERNAL
 *         The external API document for apdma. \n
 *         The apdma driver follow Linux dma framework.
 *
 *         @{
 *            @defgroup IP_group_apdma_external_function 1.function
 *              This is apdma external function.
 *            @defgroup IP_group_apdma_external_struct 2.structure
 *              This is apdma external structure.
 *            @defgroup IP_group_apdma_external_typedef 3.typedef
 *              This is apdma external typedef.
 *            @defgroup IP_group_apdma_external_enum 4.enumeration
 *              This is apdma external enumeration.
 *            @defgroup IP_group_apdma_external_def 5.define
 *              This is apdma external define.
 *         @}
 *
 *       @defgroup IP_group_apdma_internal INTERNAL
 *         The internal API document for apdma. \n
 *
 *         @{
 *            @defgroup IP_group_apdma_internal_function 1.function
 *              This is apdma internal function.
 *            @defgroup IP_group_apdma_internal_struct 2.structure
 *              This is apdma internal structure.
 *            @defgroup IP_group_apdma_internal_typedef 3.typedef
 *              This is apdma internal typedef.
 *            @defgroup IP_group_apdma_internal_enum 4.enumeration
 *              This is apdma internal enumeration.
 *            @defgroup IP_group_apdma_internal_def 5.define
 *              This is apdma internal define.
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
#include <linux/serial_core.h>
#include "virt-dma.h"
#include "mtk-dma.h"
#include "mtk-apdma_reg.h"

/** @ingroup IP_group_apdma_internal_def
 * @brief use for some control bit.
 * @{
 */
#define DRV_NAME			"mtk-apdma"
#define DMA0_NAME			"apdma0"
#define DMA1_NAME			"apdma1"
/*---------------------------------------------------------------------------*/
#define INT_FLAG_B			BIT(0)

#define EN_B				BIT(0)
#define HARD_RST_B			BIT(1)
#define WARM_RST_B			BIT(0)
#define WARM_RST_CLR_B			(0)
#define STOP_B				BIT(0)
#define STOP_CLR_B			(0)
#define FLUSH_B				BIT(0)
#define FLUSH_CLR_B			(0)

#define I2C_DIR_RX_B			BIT(0)
#define I2C_DIR_TX_B			(0)

#define UART_TX_INT_EN0_B		BIT(0)
#define UART_RX_INT_EN0_B		BIT(0)
#define UART_RX_INT_EN1_B		BIT(1)

#define I2C_TX_INT_EN0_B		BIT(0)
#define I2C_RX_INT_EN1_B		BIT(1)
#define I2C_TX2RX_INT_EN2_B		BIT(2)

#define UART_INT_FLAG_B			BIT(0)
#define I2C_TX_INT_FLAG_B		BIT(0)
#define I2C_RX_INT_FLAG_B		BIT(1)
#define I2C_TX2RX_INT_FLAG_B		BIT(2)

#define UART_TX_VFF_WPT_WRAP_B		BIT(16)
#define UART_RX_VFF_RPT_WRAP_B		BIT(16)

#define AP_DMA_I2C_0_BASE		0x80
#define AP_DMA_UART_0_RX_BASE		AP_DMA_UART_0_RX_INT_FLAG

/* If the last remianed data in apdma is less than 8 bytes
 * the data may be kept in VFF forever, it will need to flush the channel.
 */
#define TX_VFF_FLUSH_THRESHOLD		(7)
/*---------------------------------------------------------------------------*/
/**
 * @}
 */

/** @ingroup IP_group_apdma_internal_struct
 * @brief Internal device APDMA struct.
 */
struct mtk_apdmadev {
	struct dma_device ddev;
	spinlock_t lock;
	void __iomem **base;
	struct clk *clk;
	unsigned int dma0_chs;
	unsigned int dma1_chs;
};

/** @ingroup IP_group_apdma_internal_enum
 * @brief APDMA common control register offset
 *
 */
enum apdma_common_reg_offset {
	APDMA_INT_FLAG = AP_DMA_I2C_0_INT_FLAG - AP_DMA_I2C_0_BASE,
	APDMA_INT_EN = AP_DMA_I2C_0_INT_EN - AP_DMA_I2C_0_BASE,
	APDMA_DMA_EN = AP_DMA_I2C_0_EN - AP_DMA_I2C_0_BASE,
	APDMA_RST = AP_DMA_I2C_0_RST - AP_DMA_I2C_0_BASE,
	APDMA_STOP = AP_DMA_I2C_0_STOP - AP_DMA_I2C_0_BASE,
	APDMA_FLUSH = AP_DMA_I2C_0_FLUSH - AP_DMA_I2C_0_BASE,
	APDMA_INT_BUF_SIZE = AP_DMA_I2C_0_INT_BUF_SIZE - AP_DMA_I2C_0_BASE,
};

/** @ingroup IP_group_apdma_internal_enum
 * @brief APDMA I2C control register offset
 *
 */
enum apdma_i2c_reg_offset {
	APDMA_I2C_CON = AP_DMA_I2C_0_CON - AP_DMA_I2C_0_BASE,
	APDMA_I2C_TX_MEM_ADDR = AP_DMA_I2C_0_TX_MEM_ADDR - AP_DMA_I2C_0_BASE,
	APDMA_I2C_TX_MEM_ADDR2 = AP_DMA_I2C_0_TX_MEM_ADDR2 - AP_DMA_I2C_0_BASE,
	APDMA_I2C_RX_MEM_ADDR = AP_DMA_I2C_0_RX_MEM_ADDR - AP_DMA_I2C_0_BASE,
	APDMA_I2C_RX_MEM_ADDR2 = AP_DMA_I2C_0_RX_MEM_ADDR2 - AP_DMA_I2C_0_BASE,
	APDMA_I2C_TX_LEN = AP_DMA_I2C_0_TX_LEN - AP_DMA_I2C_0_BASE,
	APDMA_I2C_RX_LEN = AP_DMA_I2C_0_RX_LEN - AP_DMA_I2C_0_BASE,
};

/** @ingroup IP_group_apdma_internal_enum
 * @brief APDMA UART control register offset
 *
 */
enum apdma_uart_reg_offset {
	APDMA_UART_VFF_ADDR = AP_DMA_UART_0_RX_VFF_ADDR - AP_DMA_UART_0_RX_BASE,
	APDMA_UART_VFF_ADDR2 = AP_DMA_UART_0_RX_VFF_ADDR2
				- AP_DMA_UART_0_RX_BASE,
	APDMA_UART_VFF_LEN = AP_DMA_UART_0_RX_VFF_LEN - AP_DMA_UART_0_RX_BASE,
	APDMA_UART_VFF_THRE = AP_DMA_UART_0_RX_VFF_THRE - AP_DMA_UART_0_RX_BASE,
	APDMA_UART_VFF_WPT = AP_DMA_UART_0_RX_VFF_WPT - AP_DMA_UART_0_RX_BASE,
	APDMA_UART_VFF_RPT = AP_DMA_UART_0_RX_VFF_RPT - AP_DMA_UART_0_RX_BASE,
	APDMA_UART_RX_FLOW_CTRL_THRE = AP_DMA_UART_0_RX_FLOW_CTRL_THRE
					- AP_DMA_UART_0_RX_BASE,
	APDMA_UART_VFF_VALID_SIZE = AP_DMA_UART_0_RX_VFF_VALID_SIZE
					- AP_DMA_UART_0_RX_BASE,
	APDMA_UART_VFF_LEFT_SIZE = AP_DMA_UART_0_RX_VFF_LEFT_SIZE
					- AP_DMA_UART_0_RX_BASE,

};

/** @ingroup IP_group_apdma_internal_enum
 * @brief APDMA slave ip
 *
 */
enum apdma_slave_ip {
	SLAVE_ID_I2C = 0,
	SLAVE_ID_UART = 1,
	SLAVE_ID_NONE
};

/** @ingroup IP_group_apdma_internal_def
 * @brief use for some process of APDMA.
 * @{
 */
/** max APDMA I2C length */
#define MAX_DMA_I2C_LEN		(SZ_64K - 1)
/** max APDMA UART length */
#define MAX_DMA_UART_LEN	(SZ_64K - 1)
/** max timeout (sec) */
#define MAX_TIMEOUT_SEC		10
/** mask MSB 16 bits */
#define MASK_MSB_16_BIT		0x0000ffff
/** mask MSB 29 bits */
#define MASK_MSB_29_BIT		0x00000007
/** mask MSB for 8 bytes alignment */
#define MASK_FOR_8_BYTE_ALIGN	0xfff8

#define APDMA_UART_RX_THRESHOLD 0x80
/** mask for LSB 32 bit */
#define MASK_LSB_32BIT 0xffffffff


/**
 * @}
 */

static bool mtk_dma_filter_fn(struct dma_chan *chan, void *param);
static struct of_dma_filter_info mtk_dma_info = {
	.filter_fn = mtk_dma_filter_fn,
};

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     APDMA convert pointer to dma device.
 * @param[in]
 *     d: dma_device pointer.
 * @return
 *     mtk_apdmadev pointer, mtk_apdmadev pointer.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static inline struct mtk_apdmadev *to_mtk_dma_dev(struct dma_device *d)
{
	return container_of(d, struct mtk_apdmadev, ddev);
}

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     APDMA convert pointer to mtk channel.
 * @param[in]
 *     c: dma_chan pointer.
 * @return
 *     mtk_chan pointer, mtk_chan pointer.
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

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     APDMA convert pointer to mtk_desc.
 * @param[in]
 *     t: dma_async_tx_descriptor pointer.
 * @return
 *     mtk_desc pointer, mtk_desc pointer.
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

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     APDMA channel write.
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

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     APDMA channel read.
 * @param[in]
 *     c: mtk_chan pointer.
 * @param[in]
 *     reg: CR offset.
 * @return
 *     value, the CR value.
 * @par Boundary case and Limitation
 *     parameter "reg" must be use the enums which are define in this file.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static unsigned int mtk_dma_chan_read(struct mtk_chan *c, unsigned int reg)
{
	return readl(c->channel_base + reg);
}

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     disable APDMA clock
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
static void mtk_dma_clk_disable(struct mtk_apdmadev *mtkd)
{
	if (mtkd && mtkd->clk)
		clk_disable_unprepare(mtkd->clk);
}

/** @ingroup IP_group_apdma_internal_function
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
	struct dma_chan *chan = vd->tx.chan;
	struct mtk_chan *c = to_mtk_dma_chan(chan);
	unsigned int count;

	/* dev_info(chan->device->dev,
	*	"[%s]DMA ch%d\n", __func__, c->dma_ch);
	*/

	spin_lock(&c->lock);
	for (count = 0; count < c->desc->sg_num; count++) {
		kfree(c->desc->sg[count].cb);
		c->desc->sg[count].cb = NULL;
	}
	kfree(c->desc->sg);
	c->desc->sg = NULL;

	kfree(c->desc);
	c->desc = NULL;
	spin_unlock(&c->lock);
}

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     flush APDMA by channel.
 * @param[in]
 *     c: mtk_chan pointer.
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
static bool mtk_dma_flush(struct mtk_chan *c)
{
	unsigned long timeout;

	if (!c) {
		pr_err("[%s]MTK channel NULL\n", __func__);
		return false;
	}

	dev_dbg(c->vc.chan.device->dev, "[%s]DMA ch%d\n", __func__, c->dma_ch);

	mtk_dma_chan_write(c, APDMA_FLUSH, FLUSH_B);
	timeout = jiffies + HZ * MAX_TIMEOUT_SEC;
	while (time_before(jiffies, timeout)) {
		if (mtk_dma_chan_read(c, APDMA_FLUSH) == 0)
			return true;
	};

	dev_err(c->vc.chan.device->dev, "APDMA (%s) flushes error(ch = %d)!!\n",
		(c->cfg.direction == DMA_MEM_TO_DEV ? "TX" : "RX"), c->dma_ch);

	return false;
}

/** @ingroup IP_group_apdma_internal_function
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

	/* dev_dbg(c->vc.chan.device->dev,
	*	"[%s]DMA ch%d\n", __func__, c->dma_ch);
	*/

	if (!c) {
		pr_err("[%s]MTK channel NULL\n", __func__);
		return false;
	}

	mtk_dma_chan_write(c, APDMA_RST, WARM_RST_B);
	timeout = jiffies + HZ * MAX_TIMEOUT_SEC;
	while (time_before(jiffies, timeout)) {
		if (mtk_dma_chan_read(c, APDMA_DMA_EN) == 0)
			return true;
	};

	dev_err(c->vc.chan.device->dev, "APDMA warm resets error(ch = %d)!!\n",
		c->dma_ch);

	return false;
}

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     stop dma by channel.
 * @param[in]
 *     c: dma_chan pointer.
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
		pr_err("[%s]MTK channel NULL\n", __func__);
		return false;
	}

	dev_dbg(c->vc.chan.device->dev, "[%s]DMA ch%d\n", __func__, c->dma_ch);

	mtk_dma_chan_write(c, APDMA_STOP, STOP_B);
	timeout = jiffies + HZ * MAX_TIMEOUT_SEC;
	while (time_before(jiffies, timeout)) {
		if (mtk_dma_chan_read(c, APDMA_DMA_EN) == 0)
			return true;
	};

	dev_err(c->vc.chan.device->dev, "APDMA stops error(ch = %d)!!\n",
		c->dma_ch);

	return false;
}

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     set virtual channel completion.
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
static void mtk_set_vchan_completion(struct mtk_chan *c)
{
	if (!c) {
		pr_err("[%s]MTK channel NULL\n", __func__);
		return;
	}

	dev_dbg(c->vc.chan.device->dev, "[%s]DMA ch%d\n", __func__, c->dma_ch);

	if (!list_empty(&c->vc.desc_issued)) {
		dev_dbg(c->vc.chan.device->dev,
			"\t\tRemove virt descriptor on issue list.\n");
		list_del(&c->desc->vd.node);
		vchan_cookie_complete(&c->desc->vd);
	} else {
		dev_dbg(c->vc.chan.device->dev,
			"\t\tVchan's desc_issued is empty!!\n");
	}
}

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     set virtual fifo dma parameter (NOT the first time).
 * @param[out]
 *     c: mtk_chan pointer.
 * @return
 *     0, the first time to set virtual fifo dma parameter.\n
 *     1, NOT the first time to set virtual fifo dma parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static bool mtk_set_vff_dma_param_check(struct mtk_chan *c)
{
	struct mtk_desc *desc_list;
	struct mtk_dma_cb *cb;
	unsigned int *cb_len;
	unsigned int size, ptr, tmp;

	if (!c || !c->desc) {
		pr_err("[%s]MTK channel empty\n", __func__);
		return false;
	}

	desc_list = c->desc;
	cb = desc_list->sg[desc_list->sg_idx].cb;
	cb_len = &cb->length;

	if (c->vc.chan.completed_cookie <= DMA_MIN_COOKIE)
		return false;

	dev_info(c->vc.chan.device->dev,
		 "[%s]Skip initialize UART VFF setting!!\n",
		 __func__);

	size = *cb_len;
	size &= MASK_MSB_16_BIT;

	if (c->cfg.direction == DMA_DEV_TO_MEM) {
		if (mtk_dma_chan_read(c, APDMA_UART_VFF_VALID_SIZE))
			mtk_set_vchan_completion(c);

	} else if (size) {/* c->cfg.direction != DMA_DEV_TO_MEM  &&  size != 0*/
		ptr = mtk_dma_chan_read(c, APDMA_UART_VFF_WPT);
		tmp = (ptr & (UART_TX_VFF_WPT_WRAP_B - 1)) + size;

		if (tmp >= mtk_dma_chan_read(c,	APDMA_UART_VFF_LEN)) {
			tmp -= mtk_dma_chan_read(c, APDMA_UART_VFF_LEN);
			ptr = tmp |
			      ((ptr & UART_TX_VFF_WPT_WRAP_B) ^
			      (UART_TX_VFF_WPT_WRAP_B));
		} else {
			ptr += size;
		}

		mtk_dma_chan_write(c, APDMA_UART_VFF_WPT, ptr);

		/* int_en */
		mtk_dma_chan_write(c, APDMA_INT_EN, UART_TX_INT_EN0_B);

		if (size < 8)
			mtk_dma_flush(c);

	} else {
		dev_info(c->vc.chan.device->dev, "\t\tError happens on TX side!!\n");
	}

	dev_info(c->vc.chan.device->dev,
		 "\t\tvff_addr: 0x%x, vff_len: 0x%x, cb_len: 0x%x, dir: %s, thres: 0x%x, wptr: 0x%x, rptr: 0x%x\n",
		 mtk_dma_chan_read(c, APDMA_UART_VFF_ADDR),
		 mtk_dma_chan_read(c, APDMA_UART_VFF_LEN),
		 (*cb_len),
		 (c->cfg.direction == DMA_MEM_TO_DEV ? "TX" : "RX"),
		 mtk_dma_chan_read(c, APDMA_UART_VFF_THRE),
		 mtk_dma_chan_read(c, APDMA_UART_VFF_WPT),
		 mtk_dma_chan_read(c, APDMA_UART_VFF_RPT));

	return true;
}

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     set virtual fifo dma parameter (UART).
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
static void mtk_set_vff_dma_param(struct mtk_chan *c)
{
	struct mtk_desc *desc_list;
	struct mtk_dma_cb *cb;
	dma_addr_t *cb_src;
	dma_addr_t *cb_dst;
	unsigned int *cb_len;

	if (!c || !c->desc || !c->desc->sg) {
		pr_err("[%s]MTK channel or descriptors NULL\n", __func__);
		return;
	}

	dev_info(c->vc.chan.device->dev, "[%s]DMA ch%d\n", __func__, c->dma_ch);

	if (mtk_set_vff_dma_param_check(c))
		return;

	mtk_dma_reset(c);

	desc_list = c->desc;
	cb = desc_list->sg[desc_list->sg_idx].cb;
	cb_src = &cb->src;
	cb_dst = &cb->dst;
	cb_len = &cb->length;

	switch (c->cfg.direction) {
	case DMA_MEM_TO_DEV:
		/* TX */
		/* src */
		mtk_dma_chan_write(c, APDMA_UART_VFF_ADDR,
					(*cb_src) & MASK_LSB_32BIT);
		mtk_dma_chan_write(c, APDMA_UART_VFF_ADDR2, (*cb_src) >> 32);
		mtk_dma_chan_write(c, APDMA_UART_VFF_LEN, UART_XMIT_SIZE);
		/* write pointer */
		mtk_dma_chan_write(c, APDMA_UART_VFF_WPT,
				   (*cb_len & MASK_MSB_16_BIT));
		/* int_en */
		mtk_dma_chan_write(c, APDMA_INT_EN, UART_TX_INT_EN0_B);
		/* thres */
		mtk_dma_chan_write(c, APDMA_UART_VFF_THRE,
				UART_XMIT_SIZE - TX_VFF_FLUSH_THRESHOLD);
		break;
	case DMA_DEV_TO_MEM:
		mtk_dma_chan_write(c, APDMA_UART_VFF_ADDR,
					(*cb_dst) & MASK_LSB_32BIT);
		mtk_dma_chan_write(c, APDMA_UART_VFF_ADDR2, (*cb_dst) >> 32);
		mtk_dma_chan_write(c, APDMA_UART_VFF_LEN, *cb_len);
		/* read pointer */
		mtk_dma_chan_write(c, APDMA_UART_VFF_RPT, 0);
		/* int_en */
		mtk_dma_chan_write(c, APDMA_INT_EN,
				   UART_RX_INT_EN0_B | UART_RX_INT_EN1_B);
		/* thres */
		/* rx_vff_valid_size >= rx_vff_thre */
		mtk_dma_chan_write(c, APDMA_UART_VFF_THRE,
				   (*cb_len & MASK_FOR_8_BYTE_ALIGN));
		mtk_dma_chan_write(c, APDMA_UART_RX_FLOW_CTRL_THRE,
				   APDMA_UART_RX_THRESHOLD);
		break;
	default:
		dev_err(c->vc.chan.device->dev,
			"\t\tNot support transfer direction(ch: %d, dir: %d)!!\n",
			c->dma_ch, c->cfg.direction);
		return;
	}

	dev_info(c->vc.chan.device->dev,
		 "\t\tvff_addr: 0x%llx, vff_len: 0x%x, xfer_len: 0x%x, dir: %s, thres: 0x%x\n",
		 (dma_addr_t)((dma_addr_t)mtk_dma_chan_read(c,
						APDMA_UART_VFF_ADDR)
				+ ((dma_addr_t)mtk_dma_chan_read(c,
						APDMA_UART_VFF_ADDR2)
					<< 32)),
		 mtk_dma_chan_read(c, APDMA_UART_VFF_LEN),
		 *cb_len,
		 (c->cfg.direction == DMA_MEM_TO_DEV ? "TX" : "RX"),
		 mtk_dma_chan_read(c, APDMA_UART_VFF_THRE));

	/* kick dma */
	mtk_dma_chan_write(c, APDMA_DMA_EN, EN_B);
}

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     set peripheral dma parameter (I2C).
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
static void mtk_set_peri_dma_param(struct mtk_chan *c)
{
	struct mtk_desc *desc_list;
	struct mtk_dma_cb *cb;
	dma_addr_t *cb_src;
	dma_addr_t *cb_dst;
	unsigned int *cb_len;
	unsigned int size;

	if (!c || !c->desc || !c->desc->sg) {
		pr_err("[%s]MTK channel or descriptors NULL\n", __func__);
		return;
	}

	/* dev_info(c->vc.chan.device->dev, "[%s]\n", __func__); */

	mtk_dma_reset(c);

	desc_list = c->desc;
	cb = desc_list->sg[desc_list->sg_idx].cb;
	cb_src = &cb->src;
	cb_dst = &cb->dst;
	cb_len = &cb->length;

	switch (c->cfg.direction) {
	case DMA_MEM_TO_DEV:
		/* TX*/
		/* src */
		mtk_dma_chan_write(c, APDMA_I2C_TX_MEM_ADDR,
					(*cb_src) & MASK_LSB_32BIT);
		mtk_dma_chan_write(c, APDMA_I2C_TX_MEM_ADDR2, (*cb_src) >> 32);
		mtk_dma_chan_write(c, APDMA_I2C_TX_LEN, *cb_len);
		/* int_en */
		mtk_dma_chan_write(c, APDMA_INT_EN, I2C_TX_INT_EN0_B);
		/* dev_info(c->vc.chan.device->dev,
		*	 "\t\ttx_addr: 0x%llx, tx_len: 0x%x,",
		*	 (dma_addr_t)
		*	 ((dma_addr_t)mtk_dma_chan_read(c,
		*				APDMA_I2C_TX_MEM_ADDR)
		*		+ ((dma_addr_t)mtk_dma_chan_read(c,
		*				APDMA_I2C_TX_MEM_ADDR2)
		*			<< 32)),
		*	 mtk_dma_chan_read(c, APDMA_I2C_TX_LEN));
		*/

		/* con */
		mtk_dma_chan_write(c, APDMA_I2C_CON, I2C_DIR_TX_B);
		break;
	case DMA_DEV_TO_MEM:
		/* RX */
		size = *cb_len >> 16;
		mtk_dma_chan_write(c, APDMA_I2C_TX_MEM_ADDR,
					(*cb_src) & MASK_LSB_32BIT);
		mtk_dma_chan_write(c, APDMA_I2C_TX_MEM_ADDR2, (*cb_src) >> 32);
		if (size) {
			mtk_dma_chan_write(c, APDMA_I2C_CON, I2C_DIR_TX_B);
			mtk_dma_chan_write(c, APDMA_I2C_TX_LEN, size);
		} else {
			mtk_dma_chan_write(c, APDMA_I2C_CON, I2C_DIR_RX_B);
		}

		mtk_dma_chan_write(c, APDMA_I2C_RX_MEM_ADDR,
					(*cb_dst) & MASK_LSB_32BIT);
		mtk_dma_chan_write(c, APDMA_I2C_RX_MEM_ADDR2, (*cb_dst) >> 32);
		mtk_dma_chan_write(c, APDMA_I2C_RX_LEN, (*cb_len & 0xFFFFl));
		/* int_en */
		mtk_dma_chan_write(c, APDMA_INT_EN,
				   I2C_RX_INT_EN1_B | I2C_TX2RX_INT_EN2_B);
		dev_info(c->vc.chan.device->dev,
			 " rx_addr: 0x%llx, rx_len: 0x%x, xfer_len: 0x%x, dir: %s\n",
			 (dma_addr_t)
			 ((dma_addr_t)mtk_dma_chan_read(c,
							APDMA_I2C_RX_MEM_ADDR)
				+ ((dma_addr_t)mtk_dma_chan_read(c,
							APDMA_I2C_RX_MEM_ADDR)
						<< 32)),
			 mtk_dma_chan_read(c, APDMA_I2C_RX_LEN),
			 *cb_len,
			 (c->cfg.direction == DMA_MEM_TO_DEV ? "TX" : "RX"));
		break;
	default:
		dev_err(c->vc.chan.device->dev,
			"\t\tNot support transfer direction(ch: %d, dir: %d)!!\n",
			c->dma_ch, c->cfg.direction);
		return;
	}
	/* kick dma */
	mtk_dma_chan_write(c, APDMA_DMA_EN, EN_B);
}

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     mtk dma tasklet.
 * @param[out]
 *     data: mtk_chan pointer.
 * @return
 *     void
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dma_tasklet(unsigned long data)
{
	struct mtk_chan *c = (struct mtk_chan *)data;

	if (!c || !c->channel_base) {
		pr_err("[%s]Invalid MTK channel\n", __func__);
		return;
	}

	mtk_dma_flush(c);

	if ((mtk_dma_chan_read(c, APDMA_UART_VFF_RPT) ==
	     mtk_dma_chan_read(c, APDMA_UART_VFF_WPT)) &&
	     (mtk_dma_chan_read(c, APDMA_INT_BUF_SIZE) == 0)) {
		mtk_set_vchan_completion(c);
		dev_info(c->vc.chan.device->dev,
			 "\t\tVFF DMA ch%d TX completes.\n", c->dma_ch);
	} else {
		dev_err(c->vc.chan.device->dev,
			"\t\tVFF DMA ch%d TX error happens!!\n", c->dma_ch);
	}
}

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     APDMA peripheral irq function.
 * @param[out]
 *     c: mtk_chan pointer.
 * @return
 *     void.
 * @par Boundary case and Limitation
 *     The irq flag must not be cleared before the function.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_peri_dma_irq(struct mtk_chan *c)
{
	unsigned int irq_flag;

	if (!c) {
		pr_err("[%s]MTK channel NULL\n", __func__);
		return;
	}

	irq_flag = mtk_dma_chan_read(c, APDMA_INT_FLAG);
	if (irq_flag == 0)
		return;

	/* dev_info(c->vc.chan.device->dev, "[%s]\n", __func__); */

	if (irq_flag & I2C_TX_INT_FLAG_B) {	/* W0C */
		irq_flag &= (~I2C_TX_INT_FLAG_B);
		mtk_dma_chan_write(c, APDMA_INT_FLAG, irq_flag);
		if (c->cfg.direction == DMA_MEM_TO_DEV)
			mtk_set_vchan_completion(c);

		/*dev_info(c->vc.chan.device->dev,
		*	 "\t\tPERI DMA ch%d TX completes.\n", c->dma_ch);
		*/
	}

	if (irq_flag & I2C_RX_INT_FLAG_B) {	/* W0C */
		irq_flag &= (~I2C_RX_INT_FLAG_B);
		mtk_dma_chan_write(c, APDMA_INT_FLAG, irq_flag);

		mtk_set_vchan_completion(c);
		/* dev_info(c->vc.chan.device->dev,
		*	 "\t\tPERI DMA ch%d RX completes.\n", c->dma_ch);
		*/
	}

	if (irq_flag & I2C_TX2RX_INT_FLAG_B) {	/* W0C */
		irq_flag &= (~I2C_TX2RX_INT_FLAG_B);
		mtk_dma_chan_write(c, APDMA_INT_FLAG, irq_flag);
		/* Error recovery ? */
		dev_err(c->vc.chan.device->dev,
			"\t\tIncorrect dma module name!!\n");
	}
}

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     APDMA virtual fifo irq function.
 * @param[out]
 *     c: mtk_chan pointer.
 * @return
 *     void.
 * @par Boundary case and Limitation
 *     The irq flag must not be cleared before the function.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_vff_dma_irq(struct mtk_chan *c)
{
	unsigned int irq_flag;

	if (!c) {
		pr_err("[%s]MTK channel NULL\n", __func__);
		return;
	}

	irq_flag = mtk_dma_chan_read(c, APDMA_INT_FLAG);
	if (irq_flag == 0)
		return;

	/* dev_dbg(c->vc.chan.device->dev,
	*	"[%s]DMA ch%d\n", __func__, c->dma_ch);
	*/

	switch (c->cfg.direction) {
	case DMA_MEM_TO_DEV:
		mtk_dma_chan_write(c, APDMA_INT_FLAG, 0);	/* W0C */
		tasklet_schedule(&c->task);
		break;
	case DMA_DEV_TO_MEM:
		mtk_dma_chan_write(c, APDMA_INT_FLAG, irq_flag); /* W1C */

		mtk_set_vchan_completion(c);
		/* dev_dbg(c->vc.chan.device->dev,
		*	"\t\tVFF DMA ch%d RX completes.\n", c->dma_ch);
		*/

		break;
	default:
		dev_err(c->vc.chan.device->dev,
			"\t\tNot support transfer direction(ch: %d, dir: %d)!!\n",
			c->dma_ch, c->cfg.direction);
		return;
	}
}

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     APDMA callback function.
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
	struct mtk_apdmadev *mtkd = (struct mtk_apdmadev *)data;
	struct mtk_chan *c, *next;
	unsigned long flags;

	/* dev_dbg(mtkd->ddev.dev, "[%s]\n", __func__); */

	list_for_each_entry_safe(c, next, &mtkd->ddev.channels,
				 vc.chan.device_node) {
		if (c->channel_base == NULL)
			continue;

		spin_lock_irqsave(&c->vc.lock, flags);
		switch (c->cfg.slave_id) {
		case SLAVE_ID_I2C:
			mtk_peri_dma_irq(c);
			break;
		case SLAVE_ID_UART:
			mtk_vff_dma_irq(c);
			break;
		}
		spin_unlock_irqrestore(&c->vc.lock, flags);
	}

	return IRQ_HANDLED;
}

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     allocate APDMA channel resources.
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
	struct mtk_apdmadev *mtkd;
	struct mtk_chan *c;
	int ret = 0;
	unsigned long flags;

	/* dev_dbg(mtkd->ddev.dev, "[%s]\n", __func__); */
	if (!chan) {
		pr_err("[%s]DMA channel NULL\n", __func__);
		return -EINVAL;
	}

	mtkd = to_mtk_dma_dev(chan->device);
	c = to_mtk_dma_chan(chan);

	c->dma_ch = chan->chan_id;
	if (strncmp(c->irq_name, DMA0_NAME, (strlen(DMA0_NAME))) == 0) {
		c->channel_base =
		    MTK_APDMA0_CHANIO(mtkd->base[0], c->dma_ch);
	} else if (strncmp(c->irq_name, DMA1_NAME, (strlen(DMA1_NAME))) == 0) {
		c->channel_base =
		    MTK_APDMA1_CHANIO(mtkd->base[1],
					 (c->dma_ch - mtkd->dma0_chs));
	} else {
		dev_err(mtkd->ddev.dev, "\t\tIncorrect dma module name!!\n");
		return -ENXIO;
	}
	ret =
	    request_irq(c->irq_number, mtk_dma_callback, 0,
			(const char *)c->irq_name, (void *)mtkd);
	if (ret)
		dev_err(mtkd->ddev.dev, "Fail to request IRQ\n");

	tasklet_init(&c->task, mtk_dma_tasklet, (unsigned long)c);

	spin_lock_irqsave(&c->vc.lock, flags);
	dma_cookie_init(chan);
	spin_unlock_irqrestore(&c->vc.lock, flags);

	mtk_dma_reset(c);

	/* dev_dbg(mtkd->ddev.dev, "\t\tch = %d, ch_base = 0x%lx, irq = %d\n",
	*	c->dma_ch, (unsigned long int)c->channel_base, c->irq_number);
	*/
	return ret;
}

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     free APDMA channel resources.
 * @param[out]
 *     chan: dma_chan pointer.
 * @return
 *     void
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dma_free_chan_resources(struct dma_chan *chan)
{
	struct mtk_apdmadev *mtkd;
	struct mtk_chan *c;

	if (!chan) {
		pr_err("[%s]DMA channel NULL\n", __func__);
		return;
	}

	mtkd = to_mtk_dma_dev(chan->device);
	c = to_mtk_dma_chan(chan);

	free_irq(c->irq_number, mtkd);
	tasklet_kill(&c->task);

	c->channel_base = NULL;

	vchan_free_chan_resources(&c->vc);

	/* dev_dbg(mtkd->ddev.dev, "[%s]DMA ch%d.\n", __func__, c->dma_ch); */
}

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     calculate APDMA descriptor size.
 * @param[in]
 *     c: mtk_chan pointer.
 * @return
 *     size_t
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static size_t mtk_dma_desc_size(struct mtk_chan *c)
{
	size_t size = 0;

	if (!c) {
		pr_err("[%s]MTK channel NULL\n", __func__);
		return 0;
	}

	if (c->cfg.direction == DMA_MEM_TO_DEV)
		size = mtk_dma_chan_read(c, APDMA_I2C_TX_LEN);
	else
		size = mtk_dma_chan_read(c, APDMA_I2C_RX_LEN);

	return size;
}

/** @ingroup IP_group_apdma_internal_function
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
	unsigned int size, ptr, tmp;

	if (!chan) {
		pr_err("[%s]DMA channel NULL\n", __func__);
		return DMA_ERROR;
	}

	c = to_mtk_dma_chan(chan);

	ret = dma_cookie_status(chan, cookie, txstate);

	dev_dbg(chan->device->dev, "[%s]DMA ch%d is %s, cookie = %d\n",
		__func__, c->dma_ch, ((ret == 0) ? "completion" : "on process"),
		cookie);

	spin_lock_irqsave(&c->vc.lock, flags);
	if (c->cfg.slave_id == SLAVE_ID_UART) {
		size = mtk_dma_chan_read(c, APDMA_UART_VFF_VALID_SIZE);
		if (size == 0) {
			spin_unlock_irqrestore(&c->vc.lock, flags);
			return ret;
		}

		if (c->cfg.direction == DMA_DEV_TO_MEM) {
			txstate->residue =
			mtk_dma_chan_read(c, APDMA_UART_VFF_LEN) - size;

			ptr = mtk_dma_chan_read(c, APDMA_UART_VFF_RPT);
			tmp = (ptr & (UART_RX_VFF_RPT_WRAP_B - 1)) +
					size;
			if (tmp >= mtk_dma_chan_read(c, APDMA_UART_VFF_LEN)) {
				tmp -= mtk_dma_chan_read(c, APDMA_UART_VFF_LEN);
				ptr = tmp |
					((ptr & UART_RX_VFF_RPT_WRAP_B) ^
					 (UART_RX_VFF_RPT_WRAP_B));
			} else {
				ptr += size;
			}
			mtk_dma_chan_write(c, APDMA_UART_VFF_RPT, ptr);
		} else {
			txstate->residue =
			mtk_dma_chan_read(c, APDMA_UART_VFF_LEFT_SIZE);
		}
	} else {
		if ((ret != DMA_COMPLETE) && (txstate)) {
			vd = vchan_find_desc(&c->vc, cookie);
			if (vd)
				txstate->residue = mtk_dma_desc_size(c);
			else
				txstate->residue = 0;
		}
	}
	spin_unlock_irqrestore(&c->vc.lock, flags);

	dev_dbg(chan->device->dev, "\t\tResidue size = 0x%x\n",
		txstate->residue);
	return ret;
}

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     start APDMA descriptor.
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
	struct mtk_desc *d;

	if (!c) {
		pr_err("[%s]MTK channel NULL\n", __func__);
		return;
	}

	vd = vchan_next_desc(&c->vc);
	if (!vd) {
		c->desc = NULL;
		return;
	}
	/* list_del(&vd->node); */
	c->desc = d = to_mtk_dma_desc(&vd->tx);
	switch (c->cfg.slave_id) {
	case SLAVE_ID_I2C:
		mtk_set_peri_dma_param(c);
		break;
	case SLAVE_ID_UART:
		mtk_set_vff_dma_param(c);
		break;
	}
}

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     count APDMA frame length.
 * @param[in]
 *     len: transmission length.
 * @param[in]
 *     max_len: APDMA max transmission length.
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

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     count frame for scatter gather.
 * @param[in]
 *     c: mtk_chan pointer.
 * @param[in]
 *     sgl: scatterlist pointer.
 * @param[in]
 *     sg_len: scatter gather length.
 * @return
 *     size_t, the size of frame.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static inline size_t mtk_dma_count_frames_for_sg(struct mtk_chan *c,
						 struct scatterlist *sgl,
						 unsigned int sg_len)
{
	size_t frames = 0;
	struct scatterlist *sgent;
	unsigned int i, max_len, len;

	if (!c) {
		pr_err("[%s]MTK channel NULL\n", __func__);
		return 0;
	}

	sgent = sgl;
	if (c->cfg.slave_id == SLAVE_ID_UART) { /* UART VFF size */
		max_len = (c->cfg.direction == DMA_MEM_TO_DEV) ?
			UART_XMIT_SIZE : sg_dma_len(sgent);
		if (max_len & MASK_MSB_29_BIT) /* VFF size is not 8B alignment*/
			goto err_sg_info;
	} else {
		max_len = MAX_DMA_I2C_LEN;
	}

	len = sg_dma_len(sgent) & 0xffffl;

	for_each_sg(sgl, sgent, sg_len, i) {
		frames += mtk_dma_frames_for_length(len, max_len);
	}
err_sg_info:
	return frames;
}

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     prepare APDMA slave scatter gather.
 * @param[in]
 *     chan: dma_chan pointer.
 * @param[in]
 *     sgl: scatterlist.
 * @param[in]
 *     sglen: scatter gather length.
 * @param[in]
 *     dir: dma_transfer_direction.
 * @param[in]
 *     tx_flags: tx_flags.
 * @param[in]
 *     context: void pointer
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
static struct dma_async_tx_descriptor *mtk_dma_prep_slave_sg(
					struct dma_chan *chan,
					struct scatterlist *sgl,
					unsigned int sglen,
					enum dma_transfer_direction dir,
					unsigned long tx_flags,
					void *context)
{
	struct mtk_chan *c;
	struct scatterlist *sg;
	struct mtk_desc *d;
	struct mtk_dma_cb *control_block;
	unsigned int count, frames;

	/* dev_info(chan->device->dev, "[%s]DMA ch%d", __func__, c->dma_ch); */
	if (!chan) {
		pr_err("[%s]DMA channel NULL\n", __func__);
		return NULL;
	}

	c = to_mtk_dma_chan(chan);

	frames = mtk_dma_count_frames_for_sg(c, sgl, sglen);
	if ((frames == 0) || (frames > 1)) {	/* Only support frames = 1. */
		dev_err(chan->device->dev,
			"\t\tSG length(0x%x) > max dma support length(0x%x).\n",
			sg_dma_len(sgl),
			(c->cfg.slave_id == SLAVE_ID_UART) ?
			MAX_DMA_UART_LEN : MAX_DMA_I2C_LEN);
		return NULL;
	}

	/* allocate and setup the descriptor */
	d = kmalloc(sizeof(*d), GFP_KERNEL);
	if (!d)
		return NULL;

	d->c = c;
	d->sg_idx = 0;
	d->sg_num = frames;
	d->sg = kcalloc(d->sg_num, sizeof(*d->sg), GFP_KERNEL);

	if (!d->sg) {
		kfree(d);
		return NULL;
	}
	sg = sgl;
	for (count = 0; count < frames; count++) {
		control_block = kzalloc(sizeof(struct mtk_dma_cb), GFP_KERNEL);

		if (!control_block) {
			d->sg_num = count;
			c->desc = d;
			goto error_sg;
		}

		/* DMA_DEV_TO_MEM, DMA_RX */
		if (c->cfg.direction == DMA_DEV_TO_MEM) {
			control_block->src =
			    (c->cfg.slave_id == SLAVE_ID_I2C) ?
			    sg_dma_address(sg) : 0;
			control_block->dst = sg_dma_address(sg);
		} else {	/* DMA_MEM_TO_DEV, DMA_TX */
			control_block->src = sg_dma_address(sg);
			control_block->dst = 0;
		}
		control_block->next = 0;
		d->sg[count].cb = control_block;
		control_block->length = sg_dma_len(sg);
		sg = sg_next(sg);
	}
	return vchan_tx_prep(&c->vc, &d->vd, tx_flags);

error_sg:
	mtk_dma_desc_free(&d->vd);
	return NULL;
}

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     issue APDMA all transmission to pending list.
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

	if (!chan) {
		pr_err("[%s]DMA channel NULL\n", __func__);
		return;
	}

	c = to_mtk_dma_chan(chan);
	/*dev_info(chan->device->dev, "[%s]DMA ch%d issues.\n",
	*	 __func__, c->dma_ch);
	*/

	spin_lock_irqsave(&c->vc.lock, flags);
	if (vchan_issue_pending(&c->vc) && !c->desc)
		mtk_dma_start_desc(c);

	spin_unlock_irqrestore(&c->vc.lock, flags);
}

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     config APDMA slave.
 * @param[in]
 *     chan: dma_chan pointer.
 * @param[out]
 *     cfg: dma_slave_config.
 * @return
 *     0, OK.\n
 *     -1, fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_dma_slave_config(struct dma_chan *chan,
				struct dma_slave_config *cfg)
{
	struct mtk_chan *c;
	int ret = 0;

	if (!chan) {
		pr_err("[%s]DMA channel NULL\n", __func__);
		return -1;
	}

	c = to_mtk_dma_chan(chan);

	/* dev_info(chan->device->dev, "[%s]DMA ch%d", __func__, c->dma_ch); */

	if (cfg->slave_id >= SLAVE_ID_NONE) {
		dev_err(chan->device->dev,
			"\t\tAPDMA not support this slave(slave id = %d)!!\n",
			c->cfg.slave_id);
		ret = -EPERM;
		goto exit;
	}

	memcpy(&c->cfg, cfg, sizeof(c->cfg));

	/* dev_info(chan->device->dev,
	*	 "\t\tSlave of APDMA is %s(%s)!!\n",
	*	 c->cfg.slave_id == 0 ? "I2C" : "UART",
	*	 c->cfg.direction == DMA_DEV_TO_MEM ? "TX" : "RX");
	*/
exit:
	return ret;
}

/** @ingroup IP_group_apdma_internal_function
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
	struct mtk_apdmadev *d;
	unsigned long flags;
	LIST_HEAD(head);

	if (!chan) {
		pr_err("[%s]DMA channel NULL\n", __func__);
		return -1;
	}
	c = to_mtk_dma_chan(chan);
	d = to_mtk_dma_dev(c->vc.chan.device);

	dev_info(chan->device->dev, "[%s]DMA ch%d", __func__, c->dma_ch);
	spin_lock_irqsave(&c->vc.lock, flags);

	/* Prevent this channel being scheduled */
	spin_lock(&d->lock);
	list_del_init(&c->node);
	spin_unlock(&d->lock);

	if (c->desc)
		mtk_dma_stop(c);
	vchan_get_all_descriptors(&c->vc, &head);
	spin_unlock_irqrestore(&c->vc.lock, flags);
	vchan_dma_desc_free_list(&c->vc, &head);

	return 0;
}

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     initial APDMA channel.
 * @param[out]
 *     mtkd: mtk_apdmadev pointer.
 * @return
 *     c, mtk_chan pointer.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static struct mtk_chan *mtk_dma_chan_init(struct mtk_apdmadev *mtkd)
{
	struct mtk_chan *c;

	if (!mtkd) {
		pr_err("[%s]MTK APDMA device NULL\n", __func__);
		return NULL;
	}

	c = devm_kzalloc(mtkd->ddev.dev, sizeof(*c), GFP_KERNEL);
	if (!c)
		return NULL;

	c->vc.desc_free = mtk_dma_desc_free;
	vchan_init(&c->vc, &mtkd->ddev);
	spin_lock_init(&c->lock);
	INIT_LIST_HEAD(&c->node);
	return c;
}

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     release APDMA.
 * @param[out]
 *     mtkd: mtk_apdmadev pointer.
 * @return
 *     void.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dma_free(struct mtk_apdmadev *mtkd)
{
	if (!mtkd) {
		pr_err("[%s]MTK APDMA device NULL\n", __func__);
		return;
	}

	while (!list_empty(&mtkd->ddev.channels)) {
		struct mtk_chan *c = list_first_entry(&mtkd->ddev.channels,
						      struct mtk_chan,
						      vc.chan.device_node);

		list_del(&c->vc.chan.device_node);
		tasklet_kill(&c->vc.task);
		devm_kfree(mtkd->ddev.dev, c);
	}
}

static const struct of_device_id mtk_apdma_of_match[] = {
	{.compatible = "mediatek,mt3612-apdma",},
	{ /* sentinel */ },
};

MODULE_DEVICE_TABLE(of, mtk_apdma_of_match);

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     probe device APDMA.
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
	struct mtk_apdmadev *mtkd;
	struct resource *res;
	unsigned int dma0_chs, dma1_chs, dma_num;
	int rc, i;
	struct mtk_chan *c;
	const char *s;
	struct clk *clk;

	rc = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
	if (rc)
		return rc;

	dev_info(&pdev->dev, "[%s]Starting mtk APDMA driver.", __func__);

	if (of_property_read_u32(pdev->dev.of_node,
				 "mediatek,apdma0-ch-num", &dma0_chs)) {
		dev_err(&pdev->dev, "\t\tFailed to get i2c channel num\n");
		return -EINVAL;
	}
	if (of_property_read_u32(pdev->dev.of_node,
				 "mediatek,apdma1-ch-num", &dma1_chs)) {
		dev_err(&pdev->dev, "\t\tFailed to get uart channel num\n");
		return -EINVAL;
	}

	if (of_property_read_u32(pdev->dev.of_node,
				 "mediatek,apdma-num", &dma_num)) {
		dev_err(&pdev->dev, "\t\tFailed to get dma_num!!\n");
		return -EINVAL;
	}

	mtkd = devm_kzalloc(&pdev->dev, sizeof(*mtkd), GFP_KERNEL);
	if (!mtkd)
		return -ENOMEM;

	clk = devm_clk_get(&pdev->dev, "apdmaclk");
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


	mtkd->base = devm_kcalloc(&pdev->dev,
				dma_num, sizeof(*mtkd->base), GFP_KERNEL);

	for (i = 0; i < dma_num; i++) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res) {
			dev_err(&pdev->dev,
				"\t\tFailed to get apdma base address resource[%d]!!\n",
				i);
			return -ENODEV;
		}

		mtkd->base[i] = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(mtkd->base[i])) {
			dev_err(&pdev->dev,
				"\t\tFailed to remap apdma base address[%d]!!\n",
				i);
			return PTR_ERR(mtkd->base[i]);
		}
	}

	dma_cap_set(DMA_SLAVE, mtkd->ddev.cap_mask);
	dma_cap_set(DMA_SLAVE, mtk_dma_info.dma_cap);

	mtkd->ddev.device_alloc_chan_resources = mtk_dma_alloc_chan_resources;
	mtkd->ddev.device_free_chan_resources = mtk_dma_free_chan_resources;
	mtkd->ddev.device_tx_status = mtk_dma_tx_status;
	mtkd->ddev.device_issue_pending = mtk_dma_issue_pending;
	mtkd->ddev.device_prep_slave_sg = mtk_dma_prep_slave_sg;
	mtkd->ddev.device_config = mtk_dma_slave_config;
	mtkd->ddev.device_terminate_all = mtk_dma_terminate_all;
	mtkd->ddev.src_addr_widths = BIT(DMA_SLAVE_BUSWIDTH_1_BYTE);
	mtkd->ddev.dst_addr_widths = BIT(DMA_SLAVE_BUSWIDTH_1_BYTE);
	mtkd->ddev.directions = BIT(DMA_DEV_TO_MEM) | BIT(DMA_MEM_TO_DEV);
	mtkd->ddev.residue_granularity = DMA_RESIDUE_GRANULARITY_DESCRIPTOR;
	mtkd->ddev.dev = &pdev->dev;
	INIT_LIST_HEAD(&mtkd->ddev.channels);
	spin_lock_init(&mtkd->lock);

	platform_set_drvdata(pdev, mtkd);
	mtkd->dma0_chs = dma0_chs;
	mtkd->dma1_chs = dma1_chs;
	for (i = 0; i < (mtkd->dma0_chs + mtkd->dma1_chs); i++) {
		c = mtk_dma_chan_init(mtkd);
		if (!c) {
			dev_err(&pdev->dev,
				"\t\tmtk_dma_chan_init function fails(i = %d)!!\n",
				i);
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
		else
			strcpy(c->irq_name, s);
	}
	dev_info(&pdev->dev, "\t\tInitialized %d APDMA channels.\n",
		 (mtkd->dma0_chs + mtkd->dma1_chs));

	/* Device-tree DMA controller registration */
	rc = of_dma_controller_register(pdev->dev.of_node, of_dma_simple_xlate,
					&mtk_dma_info);

	if (rc) {
		dev_err(&pdev->dev,
			"\t\tFailed to register dma controller!!\n");
		goto err_no_dma;
	}

	rc = dma_async_device_register(&mtkd->ddev);
	if (rc) {
		dev_err(&pdev->dev,
			"\t\tFailed to register slave dma engine device: %d\n",
			rc);
		goto err_no_dma;
	}

	dev_info(&pdev->dev, "\t\tLoad APDMA engine driver.\n");
	return 0;

err_no_dma:
	dev_err(&pdev->dev, "\t\tFail to load apdma engine driver !!\n");
	mtk_dma_free(mtkd);
	devm_kfree(&pdev->dev, mtkd);
	return rc;
}

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     remove device APDMA.
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
	struct mtk_apdmadev *mtkd = platform_get_drvdata(pdev);

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
		   .of_match_table = of_match_ptr(mtk_apdma_of_match),
		   },
};

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     filter dma channel id.
 * @param[in]
 *     chan: dma_chan pointer.
 * @param[in]
 *     param: void pointer.
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

static bool mtk_dma_filter_fn(struct dma_chan *chan, void *param)
{
	int req = *(int *)param;

	if (chan->chan_id == req)
		return true;
	else
		return false;
}

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     register device APDMA.
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

/** @ingroup IP_group_apdma_internal_function
 * @par Description
 *     unregister device APDMA.
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

MODULE_DESCRIPTION("MediaTek MTK APDMA Controller Driver");
MODULE_LICENSE("GPL v2");
