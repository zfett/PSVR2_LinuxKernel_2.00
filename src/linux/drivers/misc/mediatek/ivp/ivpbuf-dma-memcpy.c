/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: chiawen Lee <chiawen.lee@mediatek.com>
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
 * @file ivpbuf_dma_memcpy.c
 * Handle IVP buffer DMA memory copy.
 */


#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/wait.h>

#include "ivpbuf-core.h"
#include "ivpbuf-dma-memcpy.h"

struct ivp_dma_request {
	s32 handle;
	dma_addr_t src;
	dma_addr_t dst;
	u32 length;
};

DECLARE_VLIST(ivp_dma_request);
DEFINE_IDR(ivp_dma_idr);

static void _ivp_dma_complete_func(void *completion)
{
	complete(completion);
}

static int _ivp_dma_memcpy(struct ivpbuf_dma_chan *ivp_chan,
			   dma_addr_t dst, dma_addr_t src, int len)
{
	dma_cookie_t cookie;
	struct dma_device *dma_dev = ivp_chan->dma_chan->device;
	enum dma_ctrl_flags flags = DMA_CTRL_ACK | DMA_PREP_INTERRUPT;
	struct dma_async_tx_descriptor *tx = NULL;

	tx = dma_dev->device_prep_dma_memcpy(ivp_chan->dma_chan, dst, src, len,
					     flags);
	if (!tx) {
		dev_err(ivp_chan->dev,
			"_ivp_dma_memcpy get dma_async_tx_descriptor failed\n");
		return -EINVAL;
	}

	init_completion(&ivp_chan->tx_completion);
	tx->callback = _ivp_dma_complete_func;
	tx->callback_param = &ivp_chan->tx_completion;

	cookie = tx->tx_submit(tx);
	if (dma_submit_error(cookie)) {
		dev_err(ivp_chan->dev, "Failed to do DMA tx_submit\n");
		return -EINVAL;
	}

	dma_async_issue_pending(ivp_chan->dma_chan);

	if (!wait_for_completion_timeout(&ivp_chan->tx_completion,
					 msecs_to_jiffies(1000))) {
		dev_err(ivp_chan->dev, "Failed to wait DMA completion\n");
		return -EBUSY;
	}

	return 0;
}

static int _ivp_dma_routine_loop(void *data)
{
	struct ivp_dma_request *req;
	struct ivpbuf_dma_chan *ivp_chan = (struct ivpbuf_dma_chan *)data;

	DEFINE_WAIT_FUNC(wait, woken_wake_function);

	for (; !kthread_should_stop();) {

		add_wait_queue(&ivp_chan->dma_req_wait, &wait);
		while (1) {
			if (!list_empty(&ivp_chan->dma_enque_list) ||
			    kthread_should_stop())
				break;
			wait_woken(&wait, TASK_INTERRUPTIBLE,
				   MAX_SCHEDULE_TIMEOUT);
		}
		remove_wait_queue(&ivp_chan->dma_req_wait, &wait);
		mutex_lock(&ivp_chan->dma_mutex);

		if (!list_empty(&ivp_chan->dma_enque_list)) {
			req = vlist_node_of(ivp_chan->dma_enque_list.next,
					    struct ivp_dma_request);
			list_del_init(vlist_link(req, struct ivp_dma_request));
			mutex_unlock(&ivp_chan->dma_mutex);
			_ivp_dma_memcpy(ivp_chan,
					req->dst, req->src, req->length);

			mutex_lock(&ivp_chan->dma_mutex);
			list_add_tail(vlist_link(req, struct ivp_dma_request),
				      &ivp_chan->dma_deque_list);
		}

		mutex_unlock(&ivp_chan->dma_mutex);

		/* release cpu for another operations */
		usleep_range(1, 10);
	}

	return 0;
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     Copy data through dma from source to destination.
 * @param[in]
 *     pdev: pointer to IVP device.
 * @param[out]
 *     handle: command handle.
 * @return
 *     return the status of request.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int ivpdma_check_request_status(struct ivp_device *pdev, s32 handle)
{
	struct list_head *head, *temp;
	struct ivp_dma_request *req;
	int status = IVP_REQ_STATUS_RUN;
	struct ivpbuf_dma_chan *ivp_chan = pdev->ivp_chan;

	if (!ivp_chan)
		return -EPERM;

	mutex_lock(&ivp_chan->dma_mutex);

	if (list_empty(&ivp_chan->dma_deque_list)) {
		mutex_unlock(&ivp_chan->dma_mutex);
		return status;
	}

	status = IVP_REQ_STATUS_INVALID;
	list_for_each_safe(head, temp, &ivp_chan->dma_deque_list) {
		req = vlist_node_of(head, struct ivp_dma_request);

		if (req && req->handle == handle) {
			list_del_init(vlist_link(req, struct ivp_dma_request));
			idr_remove(&ivp_dma_idr, handle);
			kfree(req);
			status = IVP_REQ_STATUS_DEQUEUE;
			break;
		}
	}
	mutex_unlock(&ivp_chan->dma_mutex);

	return status;
}

int ivpdma_request_dma_channel(struct ivp_device *pdev)
{
	dma_cap_mask_t mask;
	struct ivpbuf_dma_chan *ivp_chan;

	ivp_chan = devm_kzalloc(pdev->dev, sizeof(*ivp_chan), GFP_KERNEL);
	if (!ivp_chan)
		return -ENOMEM;

	dma_cap_zero(mask);
	dma_cap_set(DMA_MEMCPY, mask);
	ivp_chan->dma_chan = dma_request_channel(mask, NULL, pdev->dev);
	if (!ivp_chan->dma_chan) {
		pr_err("%s: Fail dma_request_channel.\n", __FUNCTION__);
		pdev->ivp_chan = NULL;
		return -EPROBE_DEFER;
	}

	mutex_init(&ivp_chan->dma_mutex);
	INIT_LIST_HEAD(&ivp_chan->dma_enque_list);
	INIT_LIST_HEAD(&ivp_chan->dma_deque_list);

	init_waitqueue_head(&ivp_chan->dma_req_wait);
	ivp_chan->dma_task =
	    kthread_run(_ivp_dma_routine_loop, ivp_chan, "ivp cqdma task");
	if (IS_ERR(ivp_chan->dma_task)) {
		ivp_chan->dma_task = NULL;
		dev_err(pdev->dev, "create kthread(ivp cqdma task) error.\n");
		return -EINVAL;
	}

	ivp_chan->dev = pdev->dev;
	pdev->ivp_chan = ivp_chan;

	return 0;
}

void ivpdma_release_dma_channel(struct ivp_device *pdev)
{
	struct ivpbuf_dma_chan *ivp_chan = pdev->ivp_chan;

	if (ivp_chan && ivp_chan->dma_chan) {
		kthread_stop(ivp_chan->dma_task);
		dma_release_channel(ivp_chan->dma_chan);
	}
}

static dma_addr_t _ivp_calc_ivp_buffer_phys(struct device *dev,
					    struct ivp_buffer *ivpb)
{
	struct sg_table *sgt;
	struct scatterlist *s;
	int i, ret;
	int num = 0;
	dma_addr_t phys = 0;

	sgt = kmalloc(sizeof(*sgt), GFP_KERNEL);
	if (!sgt)
		return num;

	ret =
	    dma_get_sgtable(dev, sgt, ivpb->vaddr, ivpb->dma_addr,
			    ivpb->length);
	if (ret) {
		kfree(sgt);
		return num;
	}

	for_each_sg(sgt->sgl, s, sgt->nents, i) {
		phys = page_to_phys(sg_page(s));
		break;
	}

	kfree(sgt);
	return phys;
}

static dma_addr_t _ivp_transfer_dma_addr(struct device *dev,
					 struct ivp_mem_para *para, int len)
{
	dma_addr_t phys_addr = 0;

	if (!para)
		return 0;

	switch (para->mem_type) {
	case IVP_MEMTYPE_SYSTEM:
		{
			struct ivp_buffer *ivpb;
			struct ivp_device *ivp_device;
			ivp_device = dev_get_drvdata(dev);
			if (!ivp_device || !ivp_device->ivp_mng) {
				return -ENODEV;
			}
			ivpb = idr_find(&ivp_device->ivp_mng->ivp_idr,
					para->buffer_handle);
			if (!ivpb) {
				pr_err("%s: fail to get ivp buffer\n",
				       __func__);
				return -EINVAL;
			}

			if ((para->start_byte_offset < ivpb->length) &&
			    ((para->start_byte_offset + len) < ivpb->length))
				phys_addr =
				    _ivp_calc_ivp_buffer_phys(dev, ivpb);
			break;
		}
	case IVP_MEMTYPE_DATARAM0:
	case IVP_MEMTYPE_DATARAM1:
		{
			if ((para->start_byte_offset < IVP_SIZE_DATARAM) &&
			    (para->start_byte_offset + len < IVP_SIZE_DATARAM))
				phys_addr = IVP_MCU_DATARAM(para->coreid) +
				    IVP_SIZE_DATARAM * (para->mem_type -
							IVP_MEMTYPE_DATARAM0);
			break;
		}
	case IVP_MEMTYPE_INSTRAM:
		{
			if ((para->start_byte_offset < IVP_SIZE_INSTRAM) &&
			    (para->start_byte_offset + len < IVP_SIZE_INSTRAM))
				phys_addr = IVP_MCU_INSTRAM(para->coreid);
			break;
		}
	default:
		break;
	}
	return (phys_addr + para->start_byte_offset);
}

static u64 _ivp_dma_get_request(struct ivpbuf_dma_chan *ivp_chan,
				struct ivp_mem_para *dst_para,
				struct ivp_mem_para *src_para, int len)
{
	dma_addr_t dst = _ivp_transfer_dma_addr(ivp_chan->dev, dst_para, len);
	dma_addr_t src = _ivp_transfer_dma_addr(ivp_chan->dev, src_para, len);
	struct ivp_dma_request *req = 0;

	if (!dst || !src)
		return 0;

	req = kzalloc(sizeof(vlist_type(struct ivp_dma_request)), GFP_KERNEL);
	if (!req)
		return 0;

	mutex_lock(&ivp_chan->dma_mutex);
	req->handle = idr_alloc(&ivp_dma_idr, req, 1, 0, GFP_KERNEL);
	req->src    = src;
	req->dst    = dst;
	req->length = len;

	list_add_tail(vlist_link(req, struct ivp_dma_request),
		      &ivp_chan->dma_enque_list);
	mutex_unlock(&ivp_chan->dma_mutex);

	wake_up(&ivp_chan->dma_req_wait);
	return req->handle;
}

static int _ivp_setup_memcpy_para(struct ivp_mem_para *para,
				  u32 buf_memtype, u64 buf_handle)
{
	if (buf_memtype == MEM_SYSTEM) {
		para->mem_type = IVP_MEMTYPE_SYSTEM;
		return 0;
	}

	if ((buf_handle >= IVP_IOVA_DATARAM0) &&
	    (buf_handle < IVP_IOVA_DATARAM1)) {
		para->mem_type = IVP_MEMTYPE_DATARAM0;
		para->start_byte_offset = buf_handle & (IVP_SIZE_DATARAM - 1);
	} else if ((buf_handle >= IVP_IOVA_DATARAM1) &&
		   (buf_handle < IVP_IOVA_INSTRAM)) {
		para->mem_type = IVP_MEMTYPE_DATARAM1;
		para->start_byte_offset = buf_handle & (IVP_SIZE_DATARAM - 1);
	} else if ((buf_handle >= IVP_IOVA_INSTRAM) &&
		   (buf_handle < (IVP_IOVA_INSTRAM + IVP_SIZE_INSTRAM))) {
		para->mem_type = IVP_MEMTYPE_INSTRAM;
		para->start_byte_offset = buf_handle & (IVP_SIZE_INSTRAM - 1);
	} else {
		pr_err("invalid buf_handle = 0x%llx\n", buf_handle);
		return -EINVAL;
	}

	return 0;
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     Copy data through dma from source to destination.
 * @param[in]
 *     pdev: pointer to IVP device.
 * @param[out]
 *     buf: pointer to the structure that denotes buffer of dma memory copy.
 * @return
 *     0, successful to copy data. \n
 *     error code from _ivp_setup_memcpy_para().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If _ivp_setup_memcpy_para() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int ivpdma_submit_request(struct ivp_device *pdev,
			  struct ivp_dma_memcpy_buf *buf)
{
	int ret = 0;
	struct ivpbuf_dma_chan *ivp_chan = pdev->ivp_chan;
	struct ivp_mem_para dst_para, src_para;

	if (!ivp_chan)
		return -EPERM;

	dst_para.coreid = buf->core;
	dst_para.start_byte_offset = buf->dst_buf_offset;
	dst_para.buffer_handle = buf->dst_buf_handle;
	ret = _ivp_setup_memcpy_para(&dst_para,
				     buf->dst_memtype, buf->dst_buf_handle);
	if (ret)
		return ret;

	src_para.coreid = buf->core;
	src_para.start_byte_offset = buf->src_buf_offset;
	src_para.buffer_handle = buf->src_buf_handle;
	ret = _ivp_setup_memcpy_para(&src_para,
				     buf->src_memtype, buf->src_buf_handle);
	if (ret)
		return ret;

	buf->handle = _ivp_dma_get_request(ivp_chan,
					   &dst_para, &src_para, buf->size);

	return 0;
}
