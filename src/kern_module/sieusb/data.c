/*
 * Copyright (C) 2020 Sony Interactive Entertainment Inc.
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
#include "data.h"

#define USB_IN_DESCRIPTORS(NAME, SUBC, EP, MAXPACKET, MAXBURST)		\
	static struct usb_interface_descriptor NAME##_intf_desc = {	\
		.bLength            = sizeof(struct usb_interface_descriptor), \
		.bDescriptorType    = USB_DT_INTERFACE,			\
		.bAlternateSetting  = 0,				\
		.bNumEndpoints      = 1,				\
		.bInterfaceClass    = USB_CLASS_VENDOR_SPEC,		\
		.bInterfaceSubClass = (SUBC),				\
		.bInterfaceProtocol = 0,				\
	};								\
	static struct usb_endpoint_descriptor fs_##NAME##_source_desc = { \
		.bLength          = USB_DT_ENDPOINT_SIZE,		\
		.bDescriptorType  = USB_DT_ENDPOINT,			\
		.bEndpointAddress = (EP) | USB_DIR_IN,			\
		.bmAttributes     = USB_ENDPOINT_XFER_BULK,		\
		.wMaxPacketSize   = cpu_to_le16(FS_BULK_MAX_PACKET_SIZE), \
	};								\
	static struct usb_descriptor_header *fs_##NAME##_descs[] = {	\
		(struct usb_descriptor_header *)&NAME##_intf_desc,	\
		(struct usb_descriptor_header *)&fs_##NAME##_source_desc, \
		NULL,							\
	};								\
	static struct usb_endpoint_descriptor ss_##NAME##_source_desc = { \
		.bLength          = USB_DT_ENDPOINT_SIZE,		\
		.bDescriptorType  = USB_DT_ENDPOINT,			\
		.bEndpointAddress = (EP) | USB_DIR_IN,			\
		.bmAttributes     = USB_ENDPOINT_XFER_BULK,		\
		.wMaxPacketSize   = cpu_to_le16(MAXPACKET),		\
	};								\
	static struct usb_ss_ep_comp_descriptor ss_##NAME##_source_comp_desc = { \
		.bLength         = sizeof(struct usb_ss_ep_comp_descriptor), \
		.bDescriptorType = USB_DT_SS_ENDPOINT_COMP,		\
		.bMaxBurst       = (MAXBURST),				\
	};								\
	static struct usb_descriptor_header *ss_##NAME##_descs[] = {	\
		(struct usb_descriptor_header *)&NAME##_intf_desc,	\
		(struct usb_descriptor_header *)&ss_##NAME##_source_desc, \
		(struct usb_descriptor_header *)&ss_##NAME##_source_comp_desc, \
		NULL,							\
	};

#define USB_INOUT_DESCRIPTORS(NAME, SUBC, EP_SRC, EP_SINK, MAXPACKET, MAXBURST) \
	static struct usb_interface_descriptor NAME##_intf_desc = {	\
		.bLength            = sizeof(struct usb_interface_descriptor), \
		.bDescriptorType    = USB_DT_INTERFACE,			\
		.bAlternateSetting  = 0,				\
		.bNumEndpoints      = 2,				\
		.bInterfaceClass    = USB_CLASS_VENDOR_SPEC,		\
		.bInterfaceSubClass = (SUBC),				\
		.bInterfaceProtocol = 0,				\
	};								\
	static struct usb_endpoint_descriptor fs_##NAME##_source_desc = { \
		.bLength          = USB_DT_ENDPOINT_SIZE,		\
		.bDescriptorType  = USB_DT_ENDPOINT,			\
		.bEndpointAddress = (EP_SRC) | USB_DIR_IN,		\
		.bmAttributes     = USB_ENDPOINT_XFER_BULK,		\
		.wMaxPacketSize   = cpu_to_le16(FS_BULK_MAX_PACKET_SIZE), \
	};								\
	static struct usb_endpoint_descriptor fs_##NAME##_sink_desc = {	\
		.bLength          = USB_DT_ENDPOINT_SIZE,		\
		.bDescriptorType  = USB_DT_ENDPOINT,			\
		.bEndpointAddress = (EP_SINK) | USB_DIR_OUT,		\
		.bmAttributes     = USB_ENDPOINT_XFER_BULK,		\
		.wMaxPacketSize   = cpu_to_le16(FS_BULK_MAX_PACKET_SIZE), \
	};								\
	static struct usb_descriptor_header *fs_##NAME##_descs[] = {	\
		(struct usb_descriptor_header *)&NAME##_intf_desc,	\
		(struct usb_descriptor_header *)&fs_##NAME##_source_desc, \
		(struct usb_descriptor_header *)&fs_##NAME##_sink_desc,	\
		NULL,							\
	};								\
	static struct usb_endpoint_descriptor ss_##NAME##_source_desc = { \
		.bLength          = USB_DT_ENDPOINT_SIZE,		\
		.bDescriptorType  = USB_DT_ENDPOINT,			\
		.bEndpointAddress = (EP_SRC) | USB_DIR_IN,		\
		.bmAttributes     = USB_ENDPOINT_XFER_BULK,		\
		.wMaxPacketSize   = cpu_to_le16(MAXPACKET),		\
	};								\
	static struct usb_ss_ep_comp_descriptor ss_##NAME##_source_comp_desc = { \
		.bLength         = sizeof(struct usb_ss_ep_comp_descriptor), \
		.bDescriptorType = USB_DT_SS_ENDPOINT_COMP,		\
		.bMaxBurst       = (MAXBURST),				\
	};								\
	static struct usb_endpoint_descriptor ss_##NAME##_sink_desc = {	\
		.bLength          = USB_DT_ENDPOINT_SIZE,		\
		.bDescriptorType  = USB_DT_ENDPOINT,			\
		.bEndpointAddress = (EP_SINK) | USB_DIR_OUT,		\
		.bmAttributes     = USB_ENDPOINT_XFER_BULK,		\
		.wMaxPacketSize   = cpu_to_le16(MAXPACKET),		\
	};								\
	static struct usb_ss_ep_comp_descriptor ss_##NAME##_sink_comp_desc = { \
		.bLength         = sizeof(struct usb_ss_ep_comp_descriptor), \
		.bDescriptorType = USB_DT_SS_ENDPOINT_COMP,		\
		.bMaxBurst       = MAXBURST,				\
	};								\
	static struct usb_descriptor_header *ss_##NAME##_descs[] = {	\
		(struct usb_descriptor_header *)&NAME##_intf_desc,	\
		(struct usb_descriptor_header *)&ss_##NAME##_source_desc, \
		(struct usb_descriptor_header *)&ss_##NAME##_source_comp_desc, \
		(struct usb_descriptor_header *)&ss_##NAME##_sink_desc,	\
		(struct usb_descriptor_header *)&ss_##NAME##_sink_comp_desc, \
		NULL,							\
	};

/* data1 : IN 1024 */
USB_IN_DESCRIPTORS(data1, USB_SUBCLASS_DATA1, DATA1_ENDPOINT_SOURCE, 1024, 0);
/* data2 : IN 1024x16, OUT 1024x16 */
USB_INOUT_DESCRIPTORS(data2, USB_SUBCLASS_DATA2, DATA2_ENDPOINT_SOURCE, DATA2_ENDPOINT_SINK, 1024, 0xF);
/* data3 : IN 1024x16, OUT 1024x16 */
USB_INOUT_DESCRIPTORS(data3, USB_SUBCLASS_DATA3, DATA3_ENDPOINT_SOURCE, DATA3_ENDPOINT_SINK, 1024, 0xF);
/* data4 : IN 1024x16 */
USB_IN_DESCRIPTORS(data4, USB_SUBCLASS_DATA4, DATA4_ENDPOINT_SOURCE, 1024, 0xF);
/* data5 : IN 1024x16 */
USB_IN_DESCRIPTORS(data5, USB_SUBCLASS_DATA5, DATA5_ENDPOINT_SOURCE, 1024, 0xF);
/* data6 : IN 1024x16 */
USB_IN_DESCRIPTORS(data6, USB_SUBCLASS_DATA6, DATA6_ENDPOINT_SOURCE, 1024, 0xF);
/* data7 : IN 1024x16, OUT 1024x16 */
USB_INOUT_DESCRIPTORS(data7, USB_SUBCLASS_DATA7, DATA7_ENDPOINT_SOURCE, DATA7_ENDPOINT_SINK, 1024, 0xF);
/* data8 : IN 1024x16 */
USB_IN_DESCRIPTORS(data8, USB_SUBCLASS_DATA8, DATA8_ENDPOINT_SOURCE, 1024, 0xF);
/* data9 : IN 1024 */
USB_IN_DESCRIPTORS(data9, USB_SUBCLASS_DATA9, DATA9_ENDPOINT_SOURCE, 1024, 0);

struct usb_data_setting {
	struct usb_descriptor_header **fs_descs;
	struct usb_descriptor_header **ss_descs;

	struct usb_interface_descriptor *intf_desc;
	struct usb_endpoint_descriptor *ss_source_ep_desc;
	struct usb_ss_ep_comp_descriptor *ss_source_ep_comp_desc;
	struct usb_endpoint_descriptor *ss_sink_ep_desc;
	struct usb_ss_ep_comp_descriptor *ss_sink_ep_comp_desc;

	int major;
	int string_desc_index;
	int request_size;
	int request_num;
	int read_individually;
	int write_completely;
};
#define USB_IN_SETTING(NAME, MAJOR, STRIDX, REQ_SIZE, REQ_NUM, WRITE_COMP) \
	{								\
		fs_##NAME##_descs,					\
		ss_##NAME##_descs,					\
		&NAME##_intf_desc,					\
		&ss_##NAME##_source_desc, &ss_##NAME##_source_comp_desc, \
		NULL, NULL,						\
		(MAJOR), (STRIDX),					\
		(REQ_SIZE), (REQ_NUM),					\
		0, (WRITE_COMP)						\
	}
#define USB_INOUT_SETTING(NAME, MAJOR, STRIDX, REQ_SIZE, REQ_NUM, READ_INDIV, WRITE_COMP) \
	{								\
		fs_##NAME##_descs,					\
		ss_##NAME##_descs,					\
		&NAME##_intf_desc,					\
		&ss_##NAME##_source_desc, &ss_##NAME##_source_comp_desc, \
		&ss_##NAME##_sink_desc, &ss_##NAME##_sink_comp_desc, 	\
		(MAJOR), (STRIDX),					\
		(REQ_SIZE), (REQ_NUM),					\
		(READ_INDIV), (WRITE_COMP)				\
	}
static struct usb_data_setting usb_data_settings[] = {
	USB_IN_SETTING(data1, SCE_KM_CDEV_MAJOR_USB_DATA1, STRING_DATA1_IDX, 1024, 16, 0),
	USB_INOUT_SETTING(data2, SCE_KM_CDEV_MAJOR_USB_DATA2, STRING_DATA2_IDX, 1024*16, 4, 0, 0),
	USB_INOUT_SETTING(data3, SCE_KM_CDEV_MAJOR_USB_DATA3, STRING_DATA3_IDX, 1024*16, 4, 0, 0),
	USB_IN_SETTING(data4, SCE_KM_CDEV_MAJOR_USB_DATA4, STRING_DATA4_IDX, 1024*16, 300, 1),
	USB_IN_SETTING(data5, SCE_KM_CDEV_MAJOR_USB_DATA5, STRING_DATA5_IDX, 1024*16, 4, 1),
	USB_IN_SETTING(data6, SCE_KM_CDEV_MAJOR_USB_DATA6, STRING_DATA6_IDX, 1024*16, 51, 1),
	USB_INOUT_SETTING(data7, SCE_KM_CDEV_MAJOR_USB_DATA7, STRING_DATA7_IDX, 1024*16, 300, 1, 1),
	USB_IN_SETTING(data8, SCE_KM_CDEV_MAJOR_USB_DATA8, STRING_DATA8_IDX, 1024*16, 32, 1),
	USB_IN_SETTING(data9, SCE_KM_CDEV_MAJOR_USB_DATA9, STRING_DATA9_IDX, 1024, 16, 0),
};

struct usb_data {
	struct usb_data_setting *setting;

	struct usb_function interface;
	struct usb_ep *source_ep;
	struct usb_ep *sink_ep;
	struct gbuf gbuf;

	unsigned long flags;
#define FLAG_DATA_INTERFACE_ENABLED (1<<0)

	dev_t devid;
	struct cdev chrdev;
	struct class *registered_class;
	struct device *nodedev;
	const char *name;
	char *devname;
};
static struct usb_data *usb_data_insts[ARRAY_SIZE(usb_data_settings)] = {};

static const struct file_operations data_fops;

//----------------------------------------------------------------------------
static int data_open(struct inode *inode, struct file *filp)
{
	struct usb_data *udata =
		container_of(inode->i_cdev, struct usb_data, chrdev);

	struct gbuf *gbuf = &udata->gbuf;
	filp->private_data = gbuf;

	return gbuf_open(gbuf, inode, filp);
}

static int data_release(struct inode *inode, struct file *filp)
{
	struct gbuf *gbuf = filp->private_data;

	return gbuf_release(gbuf, inode, filp);
}

static ssize_t data_read_iter(struct kiocb *kiocb, struct iov_iter *to)
{
	struct gbuf *gbuf = kiocb->ki_filp->private_data;

	return gbuf_read_iter(gbuf, kiocb, to);
}

static ssize_t data_write_iter(struct kiocb *kiocb, struct iov_iter *from)
{
	struct gbuf *gbuf = kiocb->ki_filp->private_data;

	return gbuf_write_iter(gbuf, kiocb, from);
}

static unsigned int data_poll(struct file *filp, poll_table *wait)
{
	struct gbuf *gbuf = filp->private_data;

	return gbuf_poll(gbuf, filp, wait);
}

static int data_fsync(struct file *filp, loff_t start, loff_t end, int datasync)
{
	struct gbuf *gbuf = filp->private_data;

	return gbuf_fsync(gbuf, filp, start, end, datasync);
}

#define MINORS 1

static int data_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_data *udata =
		container_of(f, struct usb_data, interface);
	int ret;
	struct usb_interface_descriptor *interface_desc;
	struct usb_endpoint_descriptor *source_ep_desc;
	struct usb_ss_ep_comp_descriptor *source_ep_comp_desc;
	struct usb_endpoint_descriptor *sink_ep_desc;
	struct usb_ss_ep_comp_descriptor *sink_ep_comp_desc;

	/* register devnode */
	ret = register_chrdev_region(udata->devid, MINORS, udata->devname);
	if (ret < 0) {
		dev_err(&c->cdev->gadget->dev, "failed to register_chrdev_region\n");
		goto fail_alloc_chrdev_region;
	}
	cdev_init(&udata->chrdev, &data_fops);
	ret = cdev_add(&udata->chrdev, udata->devid, MINORS);
	if (ret < 0) {
		dev_err(&c->cdev->gadget->dev, "failed to cdev_add\n");
		goto fail_cdev_add;
	}

	udata->nodedev = device_create(udata->registered_class, NULL,
				       udata->devid, udata,
				       udata->devname);
	if (IS_ERR(udata->nodedev)) {
		dev_err(&c->cdev->gadget->dev, "failed to device_create\n");
		ret = PTR_ERR(udata->nodedev);
		goto fail_device_create;
	}

	interface_desc = udata->setting->intf_desc;
	source_ep_desc = udata->setting->ss_source_ep_desc;
	source_ep_comp_desc = udata->setting->ss_source_ep_comp_desc;
	sink_ep_desc = udata->setting->ss_sink_ep_desc;
	sink_ep_comp_desc = udata->setting->ss_sink_ep_comp_desc;

	/* register usb interface */
	ret = usb_string_id(c->cdev);
	if (ret < 0)
		goto fail_usb_id;
	strings_dev[udata->setting->string_desc_index].id = ret;
	interface_desc->iInterface = ret;

	ret = usb_interface_id(c, f);
	if (ret < 0)
		goto fail_usb_id;
	interface_desc->bInterfaceNumber = ret;

	udata->source_ep = NULL;
	if (source_ep_desc) {
		/* Configure endpoint from the descriptor. */
		udata->source_ep = usb_ep_autoconfig_ss(c->cdev->gadget, source_ep_desc, source_ep_comp_desc);
		if (!udata->source_ep) {
			dev_err(udata->nodedev, "can't autoconfigure %s source endpoint\n", udata->name);
			ret = -ENODEV;
			goto fail_usb_ep_autoconfig;
		}
		GADGET_DATA_LOG(LOG_DBG, "epname=%s address=0x%x\n",
				udata->source_ep->name,
				udata->source_ep->address);
	}

	udata->sink_ep = NULL;
	if (sink_ep_desc) {
		/* Configure endpoint from the descriptor. */
		udata->sink_ep = usb_ep_autoconfig_ss(c->cdev->gadget, sink_ep_desc, sink_ep_comp_desc);
		if (!udata->sink_ep) {
			dev_err(udata->nodedev, "can't autoconfigure %s sink endpoint\n", udata->name);
			ret = -ENODEV;
			goto fail_usb_ep_autoconfig;
		}
		GADGET_DATA_LOG(LOG_DBG, "epname=%s address=0x%x\n",
				udata->sink_ep->name,
				udata->sink_ep->address);
	}

	return 0;

fail_usb_ep_autoconfig:
fail_usb_id:
	device_destroy(udata->registered_class, udata->devid);
fail_device_create:
	cdev_del(&udata->chrdev);
fail_cdev_add:
	unregister_chrdev_region(udata->devid, MINORS);
fail_alloc_chrdev_region:

	return ret;
}

static void data_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_data *udata =
		container_of(f, struct usb_data, interface);

	if (udata->source_ep) {
		usb_ep_autoconfig_release(udata->source_ep);
	}
	if (udata->sink_ep) {
		usb_ep_autoconfig_release(udata->sink_ep);
	}
	device_destroy(udata->registered_class, udata->devid);
	cdev_del(&udata->chrdev);
	unregister_chrdev_region(udata->devid, MINORS);
}

static int data_enable(struct usb_function *f)
{
	struct usb_data *udata =
		container_of(f, struct usb_data, interface);
	struct gbuf *gbuf = &udata->gbuf;
	int ret;

	if (test_and_set_bit(FLAG_DATA_INTERFACE_ENABLED, &udata->flags)) {
		return 0;
	}

	GADGET_DATA_LOG(LOG_DBG, "enabling data interface...\n");

	if (gbuf_init(gbuf, udata->name,
		      udata->sink_ep, udata->setting->request_size, udata->setting->request_num, NULL,
		      udata->source_ep, udata->setting->request_size, udata->setting->request_num, NULL,
		      udata->setting->read_individually, udata->setting->write_completely)) {
		dev_err(udata->nodedev, "can't initialize gbuf\n");
		ret = -ENODEV;
		goto fail_gbuf_init;
	}

	/* Enable the endpoint. */
	if (udata->source_ep) {
		ret = config_ep_by_speed(f->config->cdev->gadget, f, udata->source_ep);
		if (ret) {
			dev_err(udata->nodedev, "error configuring data source endpoint\n");
			goto fail;
		}
	}
	if (udata->sink_ep) {
		ret = config_ep_by_speed(f->config->cdev->gadget, f, udata->sink_ep);
		if (ret) {
			dev_err(udata->nodedev, "error configuring data sink endpoint\n");
			goto fail;
		}
	}

	if (udata->source_ep) {
		ret = usb_ep_enable(udata->source_ep);
		if (ret) {
			dev_err(udata->nodedev, "error starting data source endpoint\n");
			goto fail;
		}
	}
	if (udata->sink_ep) {
		ret = usb_ep_enable(udata->sink_ep);
		if (ret) {
			dev_err(udata->nodedev, "error starting data sink endpoint\n");
			goto fail;
		}
	}

	ret = gbuf_pre_queue(gbuf);
	if (ret) {
		if (udata->source_ep) {
			usb_ep_disable(udata->source_ep);
		}
		if (udata->sink_ep) {
			usb_ep_disable(udata->sink_ep);
		}
		goto fail;
	}

	return 0;

fail:
	gbuf_deinit(gbuf);
fail_gbuf_init:
	clear_bit(FLAG_DATA_INTERFACE_ENABLED, &udata->flags);
	return ret;
}

static void data_disable(struct usb_function *f)
{
	struct usb_data *udata =
		container_of(f, struct usb_data, interface);
	struct gbuf *gbuf = &udata->gbuf;

	GADGET_DATA_LOG(LOG_DBG, "disabling data interface...\n");

	if (test_and_clear_bit(FLAG_DATA_INTERFACE_ENABLED, &udata->flags)) {
		if (udata->source_ep) {
			usb_ep_disable(udata->source_ep);
		}
		if (udata->sink_ep) {
			usb_ep_disable(udata->sink_ep);
		}
		gbuf_deinit(gbuf);
	}
}

static int data_reset(struct usb_function *f, unsigned intf, unsigned alt)
{
	// we have only one interface
	if (alt)
		return -EINVAL;

	GADGET_DATA_LOG(LOG_DBG, "intf(%d)\n", intf);

	data_disable(f);
	return data_enable(f);
}

//----------------------------------------------------------------------------

static const struct file_operations data_fops = {
	.read_iter  = data_read_iter,
	.write_iter = data_write_iter,
	.poll       = data_poll,
	.open       = data_open,
	.release    = data_release,
	.fsync      = data_fsync,
};

int usb_data_init(struct class *class, int index, struct usb_function **interface)
{
	struct usb_data *udata;
	int devname_size;

	if (index >= ARRAY_SIZE(usb_data_settings)) {
		return -EINVAL;
	}

	udata = kzalloc(sizeof(struct usb_data), GFP_KERNEL);
	if (!udata) {
		return -ENOMEM;
	}

	udata->setting = &usb_data_settings[index];

	udata->registered_class = class;
	udata->devid = MKDEV(udata->setting->major, 0);

	devname_size = sizeof("usb_data") + 1;
	udata->devname = kmalloc(devname_size, GFP_KERNEL);
	if (!udata->devname) {
		kfree(udata);
		return -ENOMEM;
	}

	snprintf(udata->devname, devname_size, "usb_data%1d", index+1);
	udata->name = udata->devname + (sizeof("usb_") - sizeof(""));

	udata->interface.name = udata->name;
	udata->interface.fs_descriptors = usb_data_settings[index].fs_descs;
	udata->interface.hs_descriptors = NULL;
	udata->interface.ss_descriptors = usb_data_settings[index].ss_descs;
	udata->interface.bind = data_bind;
	udata->interface.unbind = data_unbind;
	udata->interface.set_alt = data_reset;
	udata->interface.disable = data_disable;

	*interface = &udata->interface;

	usb_data_insts[index] = udata;

	return 0;
}

void usb_data_destroy(int index)
{
	if (index < ARRAY_SIZE(usb_data_insts)) {
		if (usb_data_insts[index]) {
			kfree(usb_data_insts[index]->devname);
			kfree(usb_data_insts[index]);
			usb_data_insts[index] = NULL;
		}
	}
}
