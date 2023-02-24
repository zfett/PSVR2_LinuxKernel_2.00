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

#include <linux/uaccess.h>

#include "usb.h"
#include "status.h"

#include "sieimu.h"

#define STATUS_USB_DATA_SIZE 32
#define IMU_USB_DATA_NUM 40
struct imu_usb_data_t {
	uint32_t vts;
	int16_t accel[3];
	int16_t gyro[3];
	uint16_t dp_frame_cnt;
	uint16_t dp_line_cnt;
	uint16_t imu_ts;
	uint16_t status;
} __attribute__((packed));
#define IMU_USB_DATA_MAX (sizeof(struct imu_usb_data_t) * IMU_USB_DATA_NUM)

/* USB Descriptors */
#define STATUS_ENDPOINT_MAX_PACKET_SIZE 1024

_Static_assert((STATUS_USB_DATA_SIZE + IMU_USB_DATA_MAX) <= STATUS_ENDPOINT_MAX_PACKET_SIZE, "size error");

static struct usb_interface_descriptor status_intf_alt_0 = {
	.bLength            = sizeof (status_intf_alt_0),
	.bDescriptorType    = USB_DT_INTERFACE,
	.bAlternateSetting  = 0,
	.bNumEndpoints      = 0,
	.bInterfaceClass    = USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass = USB_SUBCLASS_STATUS,
	.bInterfaceProtocol = 0,
};

static struct usb_interface_descriptor status_intf_alt_1 = {
	.bLength            = sizeof (status_intf_alt_1),
	.bDescriptorType    = USB_DT_INTERFACE,
	.bAlternateSetting  = 1,
	.bNumEndpoints      = 1,
	.bInterfaceClass    = USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass = USB_SUBCLASS_STATUS,
	.bInterfaceProtocol = 0,
};

static struct usb_descriptor_header *fs_status_descs[] = {
	(struct usb_descriptor_header *)&status_intf_alt_0,
	NULL
};

static struct usb_endpoint_descriptor ss_status_source_desc = {
	.bLength          = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType  = USB_DT_ENDPOINT,
	.bEndpointAddress = STATUS_ENDPOINT_SOURCE | USB_DIR_IN,
	.bmAttributes     = USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize   = cpu_to_le16(STATUS_ENDPOINT_MAX_PACKET_SIZE),
	.bInterval        = USB_MS_TO_HS_INTERVAL(1), /* 1ms */
};

static struct usb_ss_ep_comp_descriptor ss_status_source_comp_desc = {
	.bLength           = sizeof(ss_status_source_comp_desc),
	.bDescriptorType   = USB_DT_SS_ENDPOINT_COMP,
	.wBytesPerInterval = __constant_cpu_to_le16(STATUS_ENDPOINT_MAX_PACKET_SIZE)
};

static struct usb_descriptor_header *ss_status_descs[] = {
	(struct usb_descriptor_header *)&status_intf_alt_0,
	(struct usb_descriptor_header *)&status_intf_alt_1,
	(struct usb_descriptor_header *)&ss_status_source_desc,
	(struct usb_descriptor_header *)&ss_status_source_comp_desc,
	NULL
};

struct usb_status_interface {
	struct usb_function interface;
	struct usb_ep *source_ep;
	int alt;

	struct usb_request *req;

	uint8_t status[STATUS_USB_DATA_SIZE];
	spinlock_t status_lock;

	struct sieimu_data *imu_buffer_start;
	struct sieimu_data *imu_buffer_end;
	struct sieimu_data *imu_data_last;

	unsigned long flags;
#define FLAG_STATUS_INTERFACE_ENABLED (1<<0)
#define FLAG_STATUS_INTERFACE_ENQUEUE (1<<1)

	struct class *registered_class;
	int major;
	struct device *nodedev;
};

static struct usb_status_interface s_status_interface;

static void usb_status_complete_in(struct usb_ep *ep, struct usb_request *req);

static void usb_imu_callback(struct sieimu_data* imu_data_end)
{
	struct usb_status_interface *status_interface = &s_status_interface;
	struct usb_request *req = status_interface->req;
	struct imu_usb_data_t *imu_usb_data;
	int imu_data_count;
	int ret;
	unsigned long flags;

	if (!test_bit(FLAG_STATUS_INTERFACE_ENABLED, &status_interface->flags)) {
		return;
	}

	BUG_ON(imu_data_end < status_interface->imu_buffer_start);
	BUG_ON(imu_data_end >= status_interface->imu_buffer_end);
	BUG_ON((((uintptr_t)imu_data_end - (uintptr_t)status_interface->imu_buffer_start) % sizeof(struct sieimu_data)) != 0);

	spin_lock_irqsave(&status_interface->status_lock, flags);

	memcpy(req->buf, status_interface->status, STATUS_USB_DATA_SIZE);

	spin_unlock_irqrestore(&status_interface->status_lock, flags);

	imu_usb_data = (struct imu_usb_data_t *)(req->buf + STATUS_USB_DATA_SIZE);

	for (imu_data_count = 0; imu_data_count < IMU_USB_DATA_NUM; imu_data_count++) {
		if (status_interface->imu_data_last == imu_data_end)
			break;

		imu_usb_data->accel[0] = status_interface->imu_data_last->accel[0];
		imu_usb_data->accel[1] = status_interface->imu_data_last->accel[1];
		imu_usb_data->accel[2] = status_interface->imu_data_last->accel[2];
		imu_usb_data->gyro[0] = status_interface->imu_data_last->gyro[0];
		imu_usb_data->gyro[1] = status_interface->imu_data_last->gyro[1];
		imu_usb_data->gyro[2] = status_interface->imu_data_last->gyro[2];
		imu_usb_data->vts = status_interface->imu_data_last->vts;
		imu_usb_data->dp_frame_cnt = status_interface->imu_data_last->dp_frame_cnt;
		imu_usb_data->dp_line_cnt = status_interface->imu_data_last->dp_line_cnt;
		imu_usb_data->imu_ts = status_interface->imu_data_last->imu_ts;
		imu_usb_data->status = status_interface->imu_data_last->status;
		imu_usb_data++;

		status_interface->imu_data_last++;
		if (status_interface->imu_data_last >= status_interface->imu_buffer_end)
			status_interface->imu_data_last = status_interface->imu_buffer_start;
	}

	req->length = STATUS_USB_DATA_SIZE + (sizeof(struct imu_usb_data_t) * imu_data_count);

	if (test_and_set_bit(FLAG_STATUS_INTERFACE_ENQUEUE, &status_interface->flags)) {
		printk(KERN_WARNING "imu unexpected update data.\n");
	}
	ret = usb_ep_queue(status_interface->source_ep, req, GFP_KERNEL);
	if (ret < 0) {
		req->status = ret;
		usb_status_complete_in(status_interface->source_ep, req);
	}
}

static void usb_status_complete_in(struct usb_ep *ep, struct usb_request *req)
{
	struct usb_status_interface *status_interface = req->context;

	clear_bit(FLAG_STATUS_INTERFACE_ENQUEUE, &status_interface->flags);

	sieimuRequestCallBack(NULL, usb_imu_callback);
}

static const struct file_operations status_fops;
#define STATUS_DEVNAME "usb_status"

static int usb_status_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_status_interface *status_interface =
		container_of(f, struct usb_status_interface, interface);
	int ret;

	GADGET_LOG(LOG_DBG, "binding status interface...\n");

	/* register devnode */
	ret = register_chrdev(status_interface->major, STATUS_DEVNAME, &status_fops);
	if (ret < 0) {
		dev_err(&c->cdev->gadget->dev, "failed to register_chrdev\n");
		goto fail_register_chrdev;
	}

	status_interface->nodedev = device_create(status_interface->registered_class, NULL,
						  MKDEV(status_interface->major, 0), status_interface,
						  STATUS_DEVNAME);
	if (IS_ERR(status_interface->nodedev)) {
		dev_err(&c->cdev->gadget->dev, "failed to device_create\n");
		ret = PTR_ERR(status_interface->nodedev);
		goto fail_device_create;
	}

	ret = usb_string_id(c->cdev);
	if (ret < 0)
		goto fail_usb_id;
	strings_dev[STRING_STATUS_IDX].id = ret;
	status_intf_alt_0.iInterface = ret;
	status_intf_alt_1.iInterface = ret;

	ret = usb_interface_id(c, f);
	if (ret < 0)
		goto fail_usb_id;
	status_intf_alt_0.bInterfaceNumber = ret;
	status_intf_alt_1.bInterfaceNumber = ret;

	status_interface->source_ep = usb_ep_autoconfig_ss(c->cdev->gadget, &ss_status_source_desc, &ss_status_source_comp_desc);
	if (status_interface->source_ep == NULL) {
		GADGET_LOG(LOG_ERR, "can't autoconfigure status endpoint\n");
		ret = -ENODEV;
		goto fail_usb_ep_autoconfig;
	}

	spin_lock_init(&status_interface->status_lock);

	/* allocate usb_request */
	status_interface->req = usb_ep_alloc_request(status_interface->source_ep, GFP_KERNEL);
	if (!status_interface->req) {
		GADGET_LOG(LOG_ERR, "%s: failed allocating usb_req\n", __func__);
		ret = -ENOMEM;
		goto fail_usb_ep_alloc;
	}
	status_interface->req->buf = kmalloc(STATUS_USB_DATA_SIZE + IMU_USB_DATA_MAX, GFP_KERNEL);
	if (!status_interface->req->buf) {
		GADGET_LOG(LOG_ERR, "%s: failed allocating usb_req->buf\n", __func__);
		ret = -ENOMEM;
		goto fail_usb_ep_alloc;
	}
	status_interface->req->length = 0;
	status_interface->req->complete = usb_status_complete_in;
	status_interface->req->context = status_interface;

	return 0;

 fail_usb_ep_alloc:
	if (status_interface->req) {
		if (status_interface->req->buf)
			kfree(status_interface->req->buf);

		usb_ep_free_request(status_interface->source_ep, status_interface->req);
	}
	status_interface->req = NULL;

	usb_ep_autoconfig_release(status_interface->source_ep);
	status_interface->source_ep = NULL;
 fail_usb_ep_autoconfig:
 fail_usb_id:
	device_destroy(status_interface->registered_class, MKDEV(status_interface->major, 0));
 fail_device_create:
	unregister_chrdev(status_interface->major, STATUS_DEVNAME);
 fail_register_chrdev:

	return ret;
} // usb_status_bind

static void usb_status_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_status_interface *status_interface =
		container_of(f, struct usb_status_interface, interface);

	GADGET_LOG(LOG_DBG, "unbinding status interface...\n");

	if (status_interface->source_ep) {
		if (status_interface->req) {
			if (status_interface->req->buf)
				kfree(status_interface->req->buf);

			usb_ep_free_request(status_interface->source_ep, status_interface->req);
		}
		status_interface->req = NULL;

		usb_ep_autoconfig_release(status_interface->source_ep);
		status_interface->source_ep = NULL;
	}

	device_destroy(status_interface->registered_class, MKDEV(status_interface->major, 0));
	unregister_chrdev(status_interface->major, STATUS_DEVNAME);
}

static int usb_status_enable(struct usb_function *f)
{
	struct usb_status_interface *status_interface =
		container_of(f, struct usb_status_interface, interface);
	int ret = 0;

	if (test_and_set_bit(FLAG_STATUS_INTERFACE_ENABLED, &status_interface->flags)) {
		return 0;
	}

	GADGET_LOG(LOG_DBG, "enabling status interface...\n");

	/* Enable the endpoint. */
	ret = config_ep_by_speed(f->config->cdev->gadget, f, status_interface->source_ep);
	if (ret) {
		GADGET_LOG(LOG_ERR, "error configuring status endpoint. ret=[%d]\n", ret);
		goto fail;
	}

	ret = usb_ep_enable(status_interface->source_ep);
	if (ret) {
		GADGET_LOG(LOG_ERR, "error starting status endpoint. ret=[%d]\n", ret);
		goto fail;
	}

	status_interface->req->length = 0;

	status_interface->imu_buffer_start = sieimuGetFifoAddress(NULL);
	status_interface->imu_buffer_end = status_interface->imu_buffer_start + sieimuGetFifoSize(NULL);
	status_interface->imu_data_last = status_interface->imu_buffer_start;
	// for first trans
	sieimuRequestCallBack(NULL, usb_imu_callback);

	return 0;

fail:
	clear_bit(FLAG_STATUS_INTERFACE_ENABLED, &status_interface->flags);
	return ret;
} //  usb_status_enable

static void usb_status_disable(struct usb_function *f)
{
	struct usb_status_interface *status_interface =
		container_of(f, struct usb_status_interface, interface);

	GADGET_LOG(LOG_DBG, "disabling status interface...\n");

	if (test_and_clear_bit(FLAG_STATUS_INTERFACE_ENABLED, &status_interface->flags)) {
		usb_ep_dequeue(status_interface->source_ep,
			       status_interface->req);
		usb_ep_disable(status_interface->source_ep);
	}

} // usb_status_disable

static int usb_status_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct usb_status_interface *status_interface = (struct usb_status_interface *)f;
	int ret = 0;

	GADGET_LOG(LOG_DBG, "intf(%d) alt=%d\n", intf,alt);

	status_interface->alt = alt;
	usb_status_disable(f);

	if (alt)
		ret = usb_status_enable(f);

	return ret;
} // usb_status_reset

static int usb_status_get_alt(struct usb_function *f, unsigned intf)
{
	struct usb_status_interface *status_interface = (struct usb_status_interface *)f;
	ASSERT(status_interface != NULL);

	GADGET_LOG(LOG_DBG, "intf(%d) alt =(%d)\n", intf, status_interface->alt);
	return status_interface->alt;
}

//----------------------------------------------------------------------------

static ssize_t status_write(struct file *filp, const char *buff, size_t count, loff_t *off)
{
	struct usb_status_interface *status_interface = &s_status_interface;
	ssize_t ret;
	size_t length;
	unsigned long flags;

	if (buff == NULL) {
		return -EINVAL;
	}

	GADGET_LOG(LOG_DBG, "%s() count=%zd\n", __func__, count);

	length = min((unsigned)count, (unsigned)STATUS_USB_DATA_SIZE);

	spin_lock_irqsave(&status_interface->status_lock, flags);

	ret = copy_from_user(status_interface->status, (void *)buff, length);

	spin_unlock_irqrestore(&status_interface->status_lock, flags);

	return (ret == 0)? length: -EFAULT;
}

static const struct file_operations status_fops = {
	.write   = status_write,
};

int usb_status_init(struct class *class, struct usb_function **interface)
{
	GADGET_LOG(LOG_DBG, "\n");

	memset(&s_status_interface, 0, sizeof(s_status_interface));

	s_status_interface.registered_class = class;
	s_status_interface.major = SCE_KM_CDEV_MAJOR_USB_STATUS;

	s_status_interface.interface.name = "status";
	s_status_interface.interface.fs_descriptors = fs_status_descs;
	s_status_interface.interface.hs_descriptors = NULL;
	s_status_interface.interface.ss_descriptors = ss_status_descs;
	s_status_interface.interface.bind = usb_status_bind;
	s_status_interface.interface.unbind = usb_status_unbind;
	s_status_interface.interface.set_alt = usb_status_set_alt;
	s_status_interface.interface.disable = usb_status_disable;
	s_status_interface.interface.get_alt = usb_status_get_alt;

	*interface = &s_status_interface.interface;

	return 0;
} // usb_status_init


void usb_status_destroy(void)
{
	GADGET_LOG(LOG_DBG, "\n");
} // usb_status_destroy
