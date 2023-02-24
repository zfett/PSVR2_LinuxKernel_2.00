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

#include <linux/fs.h>
#include <linux/list.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/wait.h>

#include "sce_km_defs.h"
#include "usb.h"
#include "command.h"

//#define CMD_DEBUG_LOG_ENABLE

#if defined(CMD_DEBUG_LOG_ENABLE)
#define GADGET_CMD_LOG(LEVEL, FORMAT, ...)											\
	if ((LEVEL) <= LOG_LEVEL) {														\
		const char *file = __FILE__;												\
		if (strlen(file) > 20) {													\
			file = file + strlen(file) - 20;										\
		}																			\
		printk("%s:%d (%s) -" FORMAT, file, __LINE__, __FUNCTION__, ##__VA_ARGS__);	\
	}
#else
#define GADGET_CMD_LOG(LEVEL, FORMAT, ...)
#endif

#define REQ_GET_REPORT	0x01
#define REQ_SET_REPORT	0x09

struct usb_cmd_device
{
	struct class *registered_class;
	int major;
	struct device *nodedev;
};

#define CTRL_CMD_DATA_LEN (512)

struct ctrl_cmd_queue
{
	uint8_t data[CTRL_CMD_DATA_LEN];
	struct list_head list;
};

#define CTRL_CMD_REPORT_NUM (256)
static struct ctrl_cmd_queue s_ctrlcmd_report_list[CTRL_CMD_REPORT_NUM];
static spinlock_t s_ctrlcmd_report_list_lock;
static struct list_head s_ctrlcmd_received_list;
static wait_queue_head_t s_ctrlcmd_wait_queue;

static unsigned long atomic_flags;
#define FLAG_OPENED (0)

//----------------------------------------------------------------------------


static void usb_cmd_ctrl_complete(struct usb_ep *ep, struct usb_request *req)
{
	uint8_t *data;
	uint8_t report_id;
	size_t length;
	unsigned long flags;

	GADGET_CMD_LOG(LOG_INFO, "set_report CTRL comp called\n");

	data = req->buf;
	report_id = data[0];
	length = min((unsigned)req->actual, (unsigned)CTRL_CMD_DATA_LEN);

	spin_lock_irqsave(&s_ctrlcmd_report_list_lock, flags);

	memcpy(s_ctrlcmd_report_list[report_id].data, data, length);
	list_move_tail(&(s_ctrlcmd_report_list[report_id].list), &s_ctrlcmd_received_list);

	spin_unlock_irqrestore(&s_ctrlcmd_report_list_lock, flags);

	wake_up_interruptible(&s_ctrlcmd_wait_queue);

	return;
}

static int usb_cmd_get_report(struct usb_configuration *config, const struct usb_ctrlrequest *ctrl)
{
	struct usb_composite_dev *cdev = config->cdev;
	struct usb_request *req = cdev->req;
	uint16_t value = __le16_to_cpu(ctrl->wValue);
	uint16_t length = __le16_to_cpu(ctrl->wLength);
	uint8_t report_id = (uint8_t)(value & 0x00FF);
	unsigned long flags;

	GADGET_CMD_LOG(LOG_INFO, "get_report R-ID: %02X\n", report_id);

	spin_lock_irqsave(&s_ctrlcmd_report_list_lock, flags);

	req->length = min((unsigned)length, (unsigned)CTRL_CMD_DATA_LEN);
	memcpy(req->buf, s_ctrlcmd_report_list[report_id].data, req->length);

	spin_unlock_irqrestore(&s_ctrlcmd_report_list_lock, flags);

	return 0;
}

int usb_cmd_setup(struct usb_configuration *config, const struct usb_ctrlrequest *ctrl)
{
	struct usb_composite_dev *cdev = config->cdev;
	struct usb_request *req = cdev->req;
	int status = 0;
	int is_stall = 1;
	__u16 value, index, length;

	value = __le16_to_cpu(ctrl->wValue);
	index = __le16_to_cpu(ctrl->wIndex);
	length = __le16_to_cpu(ctrl->wLength);

	GADGET_CMD_LOG(LOG_DBG, "usb_setup 0x%x 0x%x 0x%x\n",value,index,length);
	switch ((ctrl->bRequestType << 8) | ctrl->bRequest) {
	// Vendor Request
	case((USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_ENDPOINT ) << 8 | REQ_GET_REPORT) :
		/* handle get report */
		if (index == 0) {		//EP0
			if (usb_cmd_get_report(config, ctrl) >= 0) {
				GADGET_CMD_LOG(LOG_INFO, "get_report succeeded\n");
				is_stall = 0;
			}
		}
		break;

	case((USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_ENDPOINT) << 8 | REQ_SET_REPORT) :
		if (index == 0) {		//EP0
			req->length = min((unsigned)length, (unsigned)CTRL_CMD_DATA_LEN);
			// req->buf is read in usb_cmd_ctrl_complete.
			// So, if it is called after get_report, returned buffer will be processed.
			req->complete = usb_cmd_ctrl_complete;
			is_stall = 0;
		}
		break;

	default:
		GADGET_CMD_LOG(LOG_DBG, "Unknown request 0x%x\n", ctrl->bRequest);
		break;
	}

	if (is_stall) {
		status = -EOPNOTSUPP;
	} else {
		req->zero = 0;
		if (status >= 0) {
			status = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
			if (status < 0) {
				GADGET_CMD_LOG(LOG_DBG, "usb_ep_queue error %d\n", value);
			}
		}
	}

	return status;
}



//----------------------------------------------------------------------------

static int device_cmd_open(struct inode *inode, struct file *filp)
{
	if (!test_and_set_bit(FLAG_OPENED, &atomic_flags)) {
		return 0;
	}
	else {
		return -EBUSY;
	}
}

static int device_cmd_release(struct inode *inode, struct file *filp)
{
	clear_bit(FLAG_OPENED, &atomic_flags);
	return 0;
}

static ssize_t device_cmd_read(struct file *filp, char *buff, size_t count, loff_t *off)
{
	ssize_t ret = 0;
	ssize_t length;
	struct ctrl_cmd_queue *src;
	unsigned long flags;

	if (buff == NULL) {
		return -EINVAL;
	}

	GADGET_CMD_LOG(LOG_DBG, "device_cmd_read count=%zd\n",count);

	if (filp->f_flags & O_NONBLOCK) {
		int empty;
		spin_lock_irqsave(&s_ctrlcmd_report_list_lock, flags);
		empty = list_empty(&s_ctrlcmd_received_list);
		spin_unlock_irqrestore(&s_ctrlcmd_report_list_lock, flags);

		if (empty) {
			return -EAGAIN;
		}
	}
	else {
		ret = wait_event_interruptible(s_ctrlcmd_wait_queue, !list_empty(&s_ctrlcmd_received_list));
		if (ret < 0) {
			return -ERESTARTSYS;
		}
	}
	spin_lock_irqsave(&s_ctrlcmd_report_list_lock, flags);

	src = list_first_entry(&s_ctrlcmd_received_list, struct ctrl_cmd_queue, list);
	list_del_init(&(src->list));
	length = min((unsigned)count, (unsigned)CTRL_CMD_DATA_LEN);
	ret = copy_to_user(buff, src->data, length);

	spin_unlock_irqrestore(&s_ctrlcmd_report_list_lock, flags);
	if (ret != 0) {
		ret = -EFAULT;
	} else {
		ret = length;
	}
	GADGET_CMD_LOG(LOG_DBG, "## read ret=%zd\n",ret);
	return ret;
}

static ssize_t device_cmd_write(struct file *filp, const char *buff, size_t count, loff_t *off)
{
	ssize_t ret = -ENODEV;
	uint8_t data[CTRL_CMD_DATA_LEN];
	uint8_t report_id;
	size_t length;
	unsigned long flags;

	if (buff == NULL) {
		return -EINVAL;
	}

	GADGET_CMD_LOG(LOG_DBG, "device_cmd_write count=%zu\n",count);

	length = min((unsigned)count, (unsigned)CTRL_CMD_DATA_LEN);
	ret = copy_from_user(data, (void *)buff, length);
	if (ret != 0) {
		return -EFAULT;
	} else {
		report_id = data[0];
		spin_lock_irqsave(&s_ctrlcmd_report_list_lock, flags);

		memcpy(s_ctrlcmd_report_list[report_id].data, data, length);

		spin_unlock_irqrestore(&s_ctrlcmd_report_list_lock, flags);
	}
	GADGET_CMD_LOG(LOG_DBG, "## write ret=%zu\n",length);
	return length;
}
static unsigned int device_cmd_poll(struct file *filp, poll_table *wait)
{
	int empty;
	unsigned long flags;

	poll_wait(filp, &s_ctrlcmd_wait_queue, wait);

	spin_lock_irqsave(&s_ctrlcmd_report_list_lock, flags);
	empty = list_empty(&s_ctrlcmd_received_list);
	spin_unlock_irqrestore(&s_ctrlcmd_report_list_lock, flags);

	if (!empty)
		return POLLIN | POLLRDNORM;
	else
		return 0;
}

static const struct file_operations cmd_fops = {
	.read 			= device_cmd_read,
	.write 			= device_cmd_write,
	.poll			= device_cmd_poll,
	.open 			= device_cmd_open,
	.release 		= device_cmd_release,
};

static struct usb_cmd_device s_cmd_device;
#define CMD_DEVNAME "usb_cmd"

int usb_cmd_init(struct class *class)
{
	int i;
	int ret;
	const int major = SCE_KM_CDEV_MAJOR_USB_CMD;
	struct device *nodedev;
	unsigned long flags;

	GADGET_CMD_LOG(LOG_DBG, "usb_cmd_init\n");

	s_cmd_device.registered_class = NULL;
	s_cmd_device.major = 0;
	s_cmd_device.nodedev = NULL;

	spin_lock_init(&s_ctrlcmd_report_list_lock);
	spin_lock_irqsave(&s_ctrlcmd_report_list_lock, flags);

	memset(s_ctrlcmd_report_list, 0, sizeof(s_ctrlcmd_report_list));
	for(i = 0; i < CTRL_CMD_REPORT_NUM; i++) {
		s_ctrlcmd_report_list[i].data[0] = i;
		INIT_LIST_HEAD(&(s_ctrlcmd_report_list[i].list));
	}
	INIT_LIST_HEAD(&s_ctrlcmd_received_list);

	spin_unlock_irqrestore(&s_ctrlcmd_report_list_lock, flags);

	init_waitqueue_head(&s_ctrlcmd_wait_queue);

	/* register devnode */
	ret = register_chrdev(major, CMD_DEVNAME, &cmd_fops);
	if (ret < 0) {
		return ret;
	}

	nodedev = device_create(class, NULL, MKDEV(major, 0), &s_cmd_device, CMD_DEVNAME);
	if (IS_ERR(nodedev)) {
		ret = PTR_ERR(nodedev);
		unregister_chrdev(major, CMD_DEVNAME);
		return ret;
	}

	s_cmd_device.registered_class = class;
	s_cmd_device.major = major;
	s_cmd_device.nodedev = nodedev;

	return 0;
}

void usb_cmd_destroy(void)
{
	GADGET_CMD_LOG(LOG_DBG, "usb_cmd_destroy\n");

	if (s_cmd_device.nodedev) {
		device_destroy(s_cmd_device.registered_class, MKDEV(s_cmd_device.major,0));
		s_cmd_device.registered_class = NULL;
		s_cmd_device.nodedev = NULL;
	}
	if (s_cmd_device.major) {
		unregister_chrdev(s_cmd_device.major, CMD_DEVNAME);
		s_cmd_device.major = 0;
	}
}
