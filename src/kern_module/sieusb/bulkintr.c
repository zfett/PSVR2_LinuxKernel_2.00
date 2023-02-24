/*
 * Copyright (C) 2021 Sony Interactive Entertainment Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <linux/cdev.h>

#include "sce_km_defs.h"
#include "usb.h"
#include "gbuf.h"
#include "bulkintr.h"

#define BULK_ENDPOINT_MAX_PACKET_SIZE 1024
#define BULK_ENDPOINT_MAX_BURST       0xF
#define BULK_ENDPOINT_NUM_REQUESTS    4
#define BULK_ENDPOINT_REQUEST_SIZE    (BULK_ENDPOINT_MAX_PACKET_SIZE * (BULK_ENDPOINT_MAX_BURST + 1))
#define INTR_ENDPOINT_MAX_PACKET_SIZE 512

static struct usb_interface_descriptor bulkintr_intf_alt_0_desc = {
	.bLength            = sizeof(struct usb_interface_descriptor),
	.bDescriptorType    = USB_DT_INTERFACE,
	.bAlternateSetting  = 0,
	.bNumEndpoints      = 2,
	.bInterfaceClass    = USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass = USB_SUBCLASS_DATA3,
	.bInterfaceProtocol = 0,
};

static struct usb_interface_descriptor bulkintr_intf_alt_1_desc = {
	.bLength            = sizeof(struct usb_interface_descriptor),
	.bDescriptorType    = USB_DT_INTERFACE,
	.bAlternateSetting  = 1,
	.bNumEndpoints      = 2,
	.bInterfaceClass    = USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass = USB_SUBCLASS_DATA3,
	.bInterfaceProtocol = 0,
};

static struct usb_interface_descriptor bulkintr_intf_alt_2_desc = {
	.bLength            = sizeof(struct usb_interface_descriptor),
	.bDescriptorType    = USB_DT_INTERFACE,
	.bAlternateSetting  = 2,
	.bNumEndpoints      = 2,
	.bInterfaceClass    = USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass = USB_SUBCLASS_DATA3,
	.bInterfaceProtocol = 0,
};

static struct usb_endpoint_descriptor fs_bulkintr_source_alt_0_desc = {
	.bLength          = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType  = USB_DT_ENDPOINT,
	.bEndpointAddress = DATA3_ENDPOINT_SOURCE | USB_DIR_IN,
	.bmAttributes     = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize   = cpu_to_le16(FS_BULK_MAX_PACKET_SIZE),
};

static struct usb_endpoint_descriptor fs_bulkintr_source_alt_1_desc = {
	.bLength          = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType  = USB_DT_ENDPOINT,
	.bEndpointAddress = INTR_ENDPOINT_SOURCE | USB_DIR_IN,
	.bmAttributes     = USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize   = cpu_to_le16(FS_INTR_MAX_PACKET_SIZE),
	.bInterval        = 1, /* 1ms */
};

static struct usb_endpoint_descriptor fs_bulkintr_sink_desc = {
	.bLength          = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType  = USB_DT_ENDPOINT,
	.bEndpointAddress = DATA3_ENDPOINT_SINK | USB_DIR_OUT,
	.bmAttributes     = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize   = cpu_to_le16(FS_BULK_MAX_PACKET_SIZE),
};

static struct usb_descriptor_header *fs_bulkintr_descs[] = {
	(struct usb_descriptor_header *)&bulkintr_intf_alt_0_desc,
	(struct usb_descriptor_header *)&fs_bulkintr_source_alt_0_desc,
	(struct usb_descriptor_header *)&fs_bulkintr_sink_desc,
	(struct usb_descriptor_header *)&bulkintr_intf_alt_1_desc,
	(struct usb_descriptor_header *)&fs_bulkintr_source_alt_1_desc,
	(struct usb_descriptor_header *)&fs_bulkintr_sink_desc,
	(struct usb_descriptor_header *)&bulkintr_intf_alt_2_desc,
	(struct usb_descriptor_header *)&fs_bulkintr_source_alt_0_desc,
	(struct usb_descriptor_header *)&fs_bulkintr_sink_desc,
	NULL,
};

static struct usb_endpoint_descriptor ss_bulkintr_source_alt_0_desc = {
	.bLength          = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType  = USB_DT_ENDPOINT,
	.bEndpointAddress = DATA3_ENDPOINT_SOURCE | USB_DIR_IN,
	.bmAttributes     = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize   = cpu_to_le16(BULK_ENDPOINT_MAX_PACKET_SIZE),
};

static struct usb_endpoint_descriptor ss_bulkintr_source_alt_1_desc = {
	.bLength          = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType  = USB_DT_ENDPOINT,
	.bEndpointAddress = INTR_ENDPOINT_SOURCE | USB_DIR_IN,
	.bmAttributes     = USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize   = cpu_to_le16(INTR_ENDPOINT_MAX_PACKET_SIZE),
	.bInterval        = 1, /* 125us 2^(bInterval-1)x125us */
};

static struct usb_ss_ep_comp_descriptor ss_bulkintr_source_comp_alt_0_desc = {
	.bLength         = sizeof(struct usb_ss_ep_comp_descriptor),
	.bDescriptorType = USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst       = BULK_ENDPOINT_MAX_BURST,
};

static struct usb_ss_ep_comp_descriptor ss_bulkintr_source_comp_alt_1_desc = {
	.bLength           = sizeof(struct usb_ss_ep_comp_descriptor),
	.bDescriptorType   = USB_DT_SS_ENDPOINT_COMP,
	.wBytesPerInterval = cpu_to_le16(INTR_ENDPOINT_MAX_PACKET_SIZE)
};

static struct usb_endpoint_descriptor ss_bulkintr_sink_desc = {
	.bLength          = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType  = USB_DT_ENDPOINT,
	.bEndpointAddress = DATA3_ENDPOINT_SINK | USB_DIR_OUT,
	.bmAttributes     = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize   = cpu_to_le16(BULK_ENDPOINT_MAX_PACKET_SIZE),
};

static struct usb_ss_ep_comp_descriptor ss_bulkintr_sink_comp_desc = {
	.bLength         = sizeof(struct usb_ss_ep_comp_descriptor),
	.bDescriptorType = USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst       = BULK_ENDPOINT_MAX_BURST,
};

static struct usb_descriptor_header *ss_bulkintr_descs[] = {
	(struct usb_descriptor_header *)&bulkintr_intf_alt_0_desc,
	(struct usb_descriptor_header *)&ss_bulkintr_source_alt_0_desc,
	(struct usb_descriptor_header *)&ss_bulkintr_source_comp_alt_0_desc,
	(struct usb_descriptor_header *)&ss_bulkintr_sink_desc,
	(struct usb_descriptor_header *)&ss_bulkintr_sink_comp_desc,
	(struct usb_descriptor_header *)&bulkintr_intf_alt_1_desc,
	(struct usb_descriptor_header *)&ss_bulkintr_source_alt_1_desc,
	(struct usb_descriptor_header *)&ss_bulkintr_source_comp_alt_1_desc,
	(struct usb_descriptor_header *)&ss_bulkintr_sink_desc,
	(struct usb_descriptor_header *)&ss_bulkintr_sink_comp_desc,
	(struct usb_descriptor_header *)&bulkintr_intf_alt_2_desc,
	(struct usb_descriptor_header *)&ss_bulkintr_source_alt_0_desc,
	(struct usb_descriptor_header *)&ss_bulkintr_source_comp_alt_0_desc,
	(struct usb_descriptor_header *)&ss_bulkintr_sink_desc,
	(struct usb_descriptor_header *)&ss_bulkintr_sink_comp_desc,
	NULL,
};

struct usb_bulkintr_interface {
	struct usb_function interface;
	struct usb_ep *source_alt_0_ep;
	struct usb_ep *source_alt_1_ep;
	struct usb_ep *sink_ep;
	int alt;

	struct gbuf gbuf;

	struct usb_request *req;

	unsigned long flags;
#define FLAG_BULKINTR_INTERFACE_ENABLED (1<<0)
#define FLAG_BULKINTR_INTERFACE_ENQUEUE (1<<1)

	dev_t devid;
	struct cdev chrdev;
	struct class *registered_class;
	struct device *nodedev;
};

static struct usb_bulkintr_interface *s_bulkintr_interface_instance = NULL;

//----------------------------------------------------------------------------
static int bulkintr_open(struct inode *inode, struct file *filp)
{
	struct usb_bulkintr_interface *bulkintr_intf =
		container_of(inode->i_cdev, struct usb_bulkintr_interface, chrdev);

	struct gbuf *gbuf = &bulkintr_intf->gbuf;
	filp->private_data = bulkintr_intf;

	return gbuf_open(gbuf, inode, filp);
}

static int bulkintr_release(struct inode *inode, struct file *filp)
{
	struct usb_bulkintr_interface *bulkintr_intf =
		filp->private_data;
	struct gbuf *gbuf = &bulkintr_intf->gbuf;

	return gbuf_release(gbuf, inode, filp);
}

static ssize_t bulkintr_read_iter(struct kiocb *kiocb, struct iov_iter *to)
{
	struct usb_bulkintr_interface *bulkintr_intf =
		kiocb->ki_filp->private_data;
	struct gbuf *gbuf = &bulkintr_intf->gbuf;

	return gbuf_read_iter(gbuf, kiocb, to);
}

static ssize_t bulkintr_write_iter(struct kiocb *kiocb, struct iov_iter *from)
{
	struct usb_bulkintr_interface *bulkintr_intf =
		kiocb->ki_filp->private_data;

	if (bulkintr_intf->alt != 1) {
		/* Alt 0, Bulk Transfer */
		struct gbuf *gbuf = &bulkintr_intf->gbuf;

		return gbuf_write_iter(gbuf, kiocb, from);
	}
	else {
		/* Alt 1, Interrupt Transfer */
		struct usb_request *req = bulkintr_intf->req;
		int ret;

		if (!test_bit(FLAG_BULKINTR_INTERFACE_ENABLED, &bulkintr_intf->flags)) {
			return -EPIPE;
		}
		if (test_and_set_bit(FLAG_BULKINTR_INTERFACE_ENQUEUE, &bulkintr_intf->flags)) {
			return -EAGAIN;
		}

		req->length = min((unsigned)iov_iter_count(from), (unsigned)INTR_ENDPOINT_MAX_PACKET_SIZE);
		ret = copy_from_iter(req->buf, req->length, from);
		if (ret != req->length) {
			clear_bit(FLAG_BULKINTR_INTERFACE_ENQUEUE, &bulkintr_intf->flags);
			return -EFAULT;
		}

		ret = usb_ep_queue(bulkintr_intf->source_alt_1_ep, req, GFP_KERNEL);
		if (ret < 0) {
			clear_bit(FLAG_BULKINTR_INTERFACE_ENQUEUE, &bulkintr_intf->flags);
			return ret;
		}

		return req->length;
	}

	return -EINVAL;
}

static unsigned int bulkintr_poll(struct file *filp, poll_table *wait)
{
	struct usb_bulkintr_interface *bulkintr_intf =
		filp->private_data;
	struct gbuf *gbuf = &bulkintr_intf->gbuf;

	return gbuf_poll(gbuf, filp, wait);
}

static int bulkintr_fsync(struct file *filp, loff_t start, loff_t end, int datasync)
{
	struct usb_bulkintr_interface *bulkintr_intf =
		filp->private_data;
	struct gbuf *gbuf = &bulkintr_intf->gbuf;

	return gbuf_fsync(gbuf, filp, start, end, datasync);
}

static const struct file_operations bulkintr_fops;
#define BULKINTR_DEVNAME "bulkintr"
#define USB_BULKINTR_DEVNAME "usb_data3"
#define MINORS 1

static void bulkintr_complete_in(struct usb_ep *ep, struct usb_request *req)
{
	struct usb_bulkintr_interface *bulkintr_intf = req->context;

	clear_bit(FLAG_BULKINTR_INTERFACE_ENQUEUE, &bulkintr_intf->flags);
}

static int bulkintr_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_bulkintr_interface *bulkintr_intf =
		container_of(f, struct usb_bulkintr_interface, interface);
	int ret;

	/* register devnode */
	ret = register_chrdev_region(bulkintr_intf->devid, MINORS, USB_BULKINTR_DEVNAME);
	if (ret < 0) {
		dev_err(&c->cdev->gadget->dev, "failed to register_chrdev_region\n");
		goto fail_alloc_chrdev_region;
	}
	cdev_init(&bulkintr_intf->chrdev, &bulkintr_fops);
	ret = cdev_add(&bulkintr_intf->chrdev, bulkintr_intf->devid, MINORS);
	if (ret < 0) {
		dev_err(&c->cdev->gadget->dev, "failed to cdev_add\n");
		goto fail_cdev_add;
	}

	bulkintr_intf->nodedev = device_create(bulkintr_intf->registered_class, NULL,
				       bulkintr_intf->devid, bulkintr_intf,
				       USB_BULKINTR_DEVNAME);
	if (IS_ERR(bulkintr_intf->nodedev)) {
		dev_err(&c->cdev->gadget->dev, "failed to device_create\n");
		ret = PTR_ERR(bulkintr_intf->nodedev);
		goto fail_device_create;
	}

	/* register usb interface */
	ret = usb_string_id(c->cdev);
	if (ret < 0)
		goto fail_usb_id;
	strings_dev[STRING_DATA3_IDX].id = ret;
	bulkintr_intf_alt_0_desc.iInterface = ret;
	bulkintr_intf_alt_1_desc.iInterface = ret;
	bulkintr_intf_alt_2_desc.iInterface = ret;

	ret = usb_interface_id(c, f);
	if (ret < 0)
		goto fail_usb_id;
	bulkintr_intf_alt_0_desc.bInterfaceNumber = ret;
	bulkintr_intf_alt_1_desc.bInterfaceNumber = ret;
	bulkintr_intf_alt_2_desc.bInterfaceNumber = ret;

	/* Configure endpoint from the descriptor. */
	bulkintr_intf->source_alt_0_ep = usb_ep_autoconfig_ss(c->cdev->gadget, &ss_bulkintr_source_alt_0_desc, &ss_bulkintr_source_comp_alt_0_desc);
	if (!bulkintr_intf->source_alt_0_ep) {
		dev_err(bulkintr_intf->nodedev, "can't autoconfigure %s source endpoint\n", BULKINTR_DEVNAME);
		ret = -ENODEV;
		goto fail_usb_ep_autoconfig;
	}
	GADGET_LOG(LOG_DBG, "epname=%s address=0x%x\n",
		   bulkintr_intf->source_alt_0_ep->name,
		   bulkintr_intf->source_alt_0_ep->address);

	/* Configure endpoint from the descriptor. */
	bulkintr_intf->source_alt_1_ep = usb_ep_autoconfig_ss(c->cdev->gadget, &ss_bulkintr_source_alt_1_desc, &ss_bulkintr_source_comp_alt_1_desc);
	if (!bulkintr_intf->source_alt_1_ep) {
		dev_err(bulkintr_intf->nodedev, "can't autoconfigure %s source endpoint\n", BULKINTR_DEVNAME);
		ret = -ENODEV;
		goto fail_usb_ep_autoconfig;
	}
	GADGET_LOG(LOG_DBG, "epname=%s address=0x%x\n",
		   bulkintr_intf->source_alt_1_ep->name,
		   bulkintr_intf->source_alt_1_ep->address);

	/* Configure endpoint from the descriptor. */
	bulkintr_intf->sink_ep = usb_ep_autoconfig_ss(c->cdev->gadget, &ss_bulkintr_sink_desc, &ss_bulkintr_sink_comp_desc);
	if (!bulkintr_intf->sink_ep) {
		dev_err(bulkintr_intf->nodedev, "can't autoconfigure %s sink endpoint\n", BULKINTR_DEVNAME);
		ret = -ENODEV;
		goto fail_usb_ep_autoconfig;
	}
	GADGET_LOG(LOG_DBG, "epname=%s address=0x%x\n",
		   bulkintr_intf->sink_ep->name,
		   bulkintr_intf->sink_ep->address);

	/* allocate usb_request for intr */
	bulkintr_intf->req = usb_ep_alloc_request(bulkintr_intf->source_alt_1_ep, GFP_KERNEL);
	if (!bulkintr_intf->req) {
		dev_err(bulkintr_intf->nodedev, "failed allocating usb_req\n");
		ret = -ENOMEM;
		goto fail_intr_alloc;
	}
	bulkintr_intf->req->buf = kmalloc(INTR_ENDPOINT_MAX_PACKET_SIZE, GFP_KERNEL);
	if (!bulkintr_intf->req->buf) {
		dev_err(bulkintr_intf->nodedev, "failed allocating usb_req->buf\n");
		ret = -ENOMEM;
		goto fail_intr_alloc;
	}

	bulkintr_intf->alt = 0;

	return 0;

fail_intr_alloc:
	if (bulkintr_intf->req) {
		if (bulkintr_intf->req->buf)
			kfree(bulkintr_intf->req->buf);
		usb_ep_free_request(bulkintr_intf->source_alt_1_ep, bulkintr_intf->req);
		bulkintr_intf->req = NULL;
	}
fail_usb_ep_autoconfig:
	if (bulkintr_intf->source_alt_0_ep) {
		usb_ep_autoconfig_release(bulkintr_intf->source_alt_0_ep);
		bulkintr_intf->source_alt_0_ep = NULL;
	}
	if (bulkintr_intf->source_alt_1_ep) {
		usb_ep_autoconfig_release(bulkintr_intf->source_alt_1_ep);
		bulkintr_intf->source_alt_1_ep = NULL;
	}
	if (bulkintr_intf->sink_ep) {
		usb_ep_autoconfig_release(bulkintr_intf->sink_ep);
		bulkintr_intf->sink_ep = NULL;
	}
fail_usb_id:
	device_destroy(bulkintr_intf->registered_class, bulkintr_intf->devid);
fail_device_create:
	cdev_del(&bulkintr_intf->chrdev);
fail_cdev_add:
	unregister_chrdev_region(bulkintr_intf->devid, MINORS);
fail_alloc_chrdev_region:

	return ret;
}

static void bulkintr_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_bulkintr_interface *bulkintr_intf =
		container_of(f, struct usb_bulkintr_interface, interface);

	if (bulkintr_intf->req) {
		if (bulkintr_intf->req->buf)
			kfree(bulkintr_intf->req->buf);
		usb_ep_free_request(bulkintr_intf->source_alt_1_ep, bulkintr_intf->req);
		bulkintr_intf->req = NULL;
	}

	usb_ep_autoconfig_release(bulkintr_intf->source_alt_0_ep);
	bulkintr_intf->source_alt_0_ep = NULL;
	usb_ep_autoconfig_release(bulkintr_intf->source_alt_1_ep);
	bulkintr_intf->source_alt_1_ep = NULL;
	usb_ep_autoconfig_release(bulkintr_intf->sink_ep);
	bulkintr_intf->sink_ep = NULL;

	device_destroy(bulkintr_intf->registered_class, bulkintr_intf->devid);
	cdev_del(&bulkintr_intf->chrdev);
	unregister_chrdev_region(bulkintr_intf->devid, MINORS);
}

static int bulkintr_enable(struct usb_function *f)
{
	struct usb_bulkintr_interface *bulkintr_intf =
		container_of(f, struct usb_bulkintr_interface, interface);
	struct gbuf *gbuf = &bulkintr_intf->gbuf;
	struct usb_ep *source_ep = NULL;
	int ret;

	if (test_and_set_bit(FLAG_BULKINTR_INTERFACE_ENABLED, &bulkintr_intf->flags)) {
		return 0;
	}

	if (bulkintr_intf->alt != 1) {
		source_ep = bulkintr_intf->source_alt_0_ep;
	}
	else {
		source_ep = bulkintr_intf->source_alt_1_ep;
	}
	GADGET_LOG(LOG_DBG, "enabling data/intr interface...\n");

	if (gbuf_init(gbuf, BULKINTR_DEVNAME,
		      bulkintr_intf->sink_ep, BULK_ENDPOINT_REQUEST_SIZE, BULK_ENDPOINT_NUM_REQUESTS, NULL,
		      ((bulkintr_intf->alt != 1)? bulkintr_intf->source_alt_0_ep: NULL),
		      BULK_ENDPOINT_REQUEST_SIZE, BULK_ENDPOINT_NUM_REQUESTS, NULL,
		      0, 0)) {
		dev_err(bulkintr_intf->nodedev, "can't initialize gbuf\n");
		ret = -ENODEV;
		goto fail_gbuf_init;
	}

	if (bulkintr_intf->alt == 1) {
		bulkintr_intf->req->length = 0;
		bulkintr_intf->req->complete = bulkintr_complete_in;
		bulkintr_intf->req->context = bulkintr_intf;
	}

	/* Enable the endpoint. */
	ret = config_ep_by_speed(f->config->cdev->gadget, f, source_ep);
	if (ret) {
		dev_err(bulkintr_intf->nodedev, "error configuring data/intr source endpoint\n");
		goto fail;
	}
	ret = config_ep_by_speed(f->config->cdev->gadget, f, bulkintr_intf->sink_ep);
	if (ret) {
		dev_err(bulkintr_intf->nodedev, "error configuring data/intr sink endpoint\n");
		goto fail;
	}

	ret = usb_ep_enable(source_ep);
	if (ret) {
		dev_err(bulkintr_intf->nodedev, "error starting data/intr source endpoint\n");
		goto fail;
	}
	ret = usb_ep_enable(bulkintr_intf->sink_ep);
	if (ret) {
		dev_err(bulkintr_intf->nodedev, "error starting data/intr sink endpoint\n");
		goto fail;
	}

	ret = gbuf_pre_queue(gbuf);
	if (ret) {
		dev_err(bulkintr_intf->nodedev, "error preparing gbuf\n");
		goto fail;
	}

	return 0;

fail:
	usb_ep_disable(bulkintr_intf->source_alt_0_ep);
	usb_ep_disable(bulkintr_intf->source_alt_1_ep);
	usb_ep_disable(bulkintr_intf->sink_ep);

	gbuf_deinit(gbuf);
fail_gbuf_init:
	clear_bit(FLAG_BULKINTR_INTERFACE_ENABLED, &bulkintr_intf->flags);
	return ret;
}

static void bulkintr_disable(struct usb_function *f)
{
	struct usb_bulkintr_interface *bulkintr_intf =
		container_of(f, struct usb_bulkintr_interface, interface);
	struct gbuf *gbuf = &bulkintr_intf->gbuf;

	GADGET_LOG(LOG_DBG, "disabling data/intr interface...\n");

	if (test_and_clear_bit(FLAG_BULKINTR_INTERFACE_ENABLED, &bulkintr_intf->flags)) {
		usb_ep_dequeue(bulkintr_intf->source_alt_1_ep,
			       bulkintr_intf->req);

		usb_ep_disable(bulkintr_intf->source_alt_0_ep);
		usb_ep_disable(bulkintr_intf->source_alt_1_ep);
		usb_ep_disable(bulkintr_intf->sink_ep);
		gbuf_deinit(gbuf);
	}
}

static int bulkintr_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct usb_bulkintr_interface *bulkintr_intf =
		container_of(f, struct usb_bulkintr_interface, interface);

	GADGET_LOG(LOG_DBG, "intf(%d) alt=%d\n", intf,alt);

	bulkintr_disable(f);

	bulkintr_intf->alt = alt;

	return bulkintr_enable(f);
}

static int bulkintr_get_alt(struct usb_function *f, unsigned intf)
{
	struct usb_bulkintr_interface *bulkintr_intf =
		container_of(f, struct usb_bulkintr_interface, interface);

	GADGET_LOG(LOG_DBG, "intf(%d) alt=%d\n", intf, bulkintr_intf->alt);

	return bulkintr_intf->alt;
}

//----------------------------------------------------------------------------

static const struct file_operations bulkintr_fops = {
	.read_iter  = bulkintr_read_iter,
	.write_iter = bulkintr_write_iter,
	.poll       = bulkintr_poll,
	.open       = bulkintr_open,
	.release    = bulkintr_release,
	.fsync      = bulkintr_fsync,
};

int usb_bulkintr_init(struct class *class, struct usb_function **interface)
{
	struct usb_bulkintr_interface *bulkintr_intf;

	bulkintr_intf = kzalloc(sizeof(struct usb_bulkintr_interface), GFP_KERNEL);
	if (!bulkintr_intf) {
		return -ENOMEM;
	}

	bulkintr_intf->registered_class = class;
	bulkintr_intf->devid = MKDEV(SCE_KM_CDEV_MAJOR_USB_DATA3, 0);

	bulkintr_intf->interface.name = BULKINTR_DEVNAME;
	bulkintr_intf->interface.fs_descriptors = fs_bulkintr_descs;
	bulkintr_intf->interface.hs_descriptors = NULL;
	bulkintr_intf->interface.ss_descriptors = ss_bulkintr_descs;
	bulkintr_intf->interface.bind = bulkintr_bind;
	bulkintr_intf->interface.unbind = bulkintr_unbind;
	bulkintr_intf->interface.set_alt = bulkintr_set_alt;
	bulkintr_intf->interface.disable = bulkintr_disable;
	bulkintr_intf->interface.get_alt = bulkintr_get_alt;

	*interface = &bulkintr_intf->interface;

	s_bulkintr_interface_instance = bulkintr_intf;

	return 0;
}

void usb_bulkintr_destroy(void)
{
	GADGET_LOG(LOG_DBG, "\n");
	if (s_bulkintr_interface_instance) {
		kfree(s_bulkintr_interface_instance);
		s_bulkintr_interface_instance = NULL;
	}
}
