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

#include <asm/uaccess.h>
#include <linux/hid.h>
#include <linux/crc32.h>

#include "usb.h"
#include "authentication.h"
#include "sieusb.h"

//#define ENABLE_SIEUSB_AUTH_DEBUG
//#define ENABLE_DUMP_LOG

//----------------------------------------------------------------------------
#if defined(ENABLE_SIEUSB_AUTH_DEBUG)
#define LOG_LEVEL_AUTH (LOG_DBG)
#else
#define LOG_LEVEL_AUTH (LOG_INFO)
#endif
#define GADGET_AUTH_LOG(LEVEL, FORMAT, ...)				\
	if ((LEVEL) <= LOG_LEVEL_AUTH)					\
	{								\
		const char* file=__FILE__;				\
		if( strlen(file)>20)					\
		{							\
			file = file+strlen(file)-20;			\
		}							\
		printk("%s:%d (%s) -"FORMAT,  file, __LINE__, __FUNCTION__, ##__VA_ARGS__); \
	}
//----------------------------------------------------------------------------

#define H_DATA1_LENGTH					180
#define H_DATA2_LENGTH					20
#define D_DATA1_LENGTH					192
#define D_DATA2_LENGTH					32
#define H_RESPONSE_LENGTH				60
#define H_RESPONSE_CRC_LENGTH			12
#define CRC_TARGET_LENGTH				60

#define REPORT_ID_SET_AUTH1_DATA		0xF0
#define REPORT_ID_GET_AUTH2_DATA		0xF1
#define REPORT_ID_GET_AUTH_STATUS		0xF2

#define SUB_ID_H_CHALLENGE_1			0x01
#define SUB_ID_H_RESPONSE				0x02
#define SUB_ID_H_CHALLENGE_2			0x03
#define SUB_ID_D_RESPONSE_CHALLENGE_1	0x01
#define SUB_ID_D_RESPONSE_CHALLENGE_2	0x03

#define SET_AUTH1_DATA_BLOCK_SIZE		56
#define GET_AUTH2_DATA_BLOCK_SIZE		56
#define MAX_AUTH1_BLOCK_NUMBER			4
#define MAX_AUTH2_BLOCK_NUMBER			4


// for descriptor
#define AUTHENTICATION_ENDPOINT_SOURCE_REQUEST_SIZE	64
#define AUTHENTICATION_ENDPOINT_SINK_REQUEST_SIZE	64
#define AUTHENTICATION_REPORT_DESC_LENGTH			48

/* USB Descriptors */
static struct usb_interface_descriptor auth_intf = {
	.bLength 			= sizeof (auth_intf),
	.bDescriptorType	= USB_DT_INTERFACE,
	.bAlternateSetting 	= 0,
	.bNumEndpoints 		= 2,
	.bInterfaceClass 	= USB_CLASS_HID,
};

static struct hid_descriptor auth_hid_desc = {
	.bLength 			= sizeof (auth_hid_desc),
	.bDescriptorType 	= HID_DT_HID,
	.bcdHID 			= 0x0111,
	.bCountryCode 		= 0x00,
	.bNumDescriptors 	= 0x1,
};

static struct usb_endpoint_descriptor fs_auth_source_desc = {
	.bLength 			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType 	= USB_DT_ENDPOINT,
	.bEndpointAddress 	= AUTHENTICATION_ENDPOINT_SOURCE | USB_DIR_IN,
	.bmAttributes 		= USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize 	= cpu_to_le16(AUTHENTICATION_ENDPOINT_SOURCE_REQUEST_SIZE),
	.bInterval 			= 1, /* 1ms */
};

static struct usb_endpoint_descriptor fs_auth_sink_desc = {
	.bLength 			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType 	= USB_DT_ENDPOINT,
	.bEndpointAddress 	= AUTHENTICATION_ENDPOINT_SINK | USB_DIR_OUT,
	.bmAttributes 		= USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize 	= cpu_to_le16(AUTHENTICATION_ENDPOINT_SINK_REQUEST_SIZE),
	.bInterval 			= 1, /* 1ms */
};

static struct usb_descriptor_header *fs_auth_descs[] = {
	(struct usb_descriptor_header *)&auth_intf,
	(struct usb_descriptor_header *)&auth_hid_desc,
	(struct usb_descriptor_header *)&fs_auth_source_desc,
	(struct usb_descriptor_header *)&fs_auth_sink_desc,
	NULL
};

static struct usb_endpoint_descriptor ss_auth_source_desc = {
	.bLength 			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType 	= USB_DT_ENDPOINT,
	.bEndpointAddress 	= AUTHENTICATION_ENDPOINT_SOURCE | USB_DIR_IN,
	.bmAttributes 		= USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize 	= cpu_to_le16(AUTHENTICATION_ENDPOINT_SOURCE_REQUEST_SIZE),
	.bInterval 			= 4, /* 1ms */
};

static struct usb_ss_ep_comp_descriptor ss_auth_source_comp_desc = {
	.bLength 			= sizeof(ss_auth_source_comp_desc),
	.bDescriptorType 	= USB_DT_SS_ENDPOINT_COMP,
	.wBytesPerInterval	= __constant_cpu_to_le16(AUTHENTICATION_ENDPOINT_SOURCE_REQUEST_SIZE)
};

static struct usb_endpoint_descriptor ss_auth_sink_desc = {
	.bLength 			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType 	= USB_DT_ENDPOINT,
	.bEndpointAddress 	= AUTHENTICATION_ENDPOINT_SINK | USB_DIR_OUT,
	.bmAttributes 		= USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize 	= cpu_to_le16(AUTHENTICATION_ENDPOINT_SINK_REQUEST_SIZE),
	.bInterval 			= 4, /* 1ms */
};

static struct usb_ss_ep_comp_descriptor ss_auth_sink_comp_desc = {
	.bLength 			= sizeof(ss_auth_sink_comp_desc),
	.bDescriptorType 	= USB_DT_SS_ENDPOINT_COMP,
	.wBytesPerInterval	= __constant_cpu_to_le16(AUTHENTICATION_ENDPOINT_SINK_REQUEST_SIZE)
};

static struct usb_descriptor_header *ss_auth_descs[] = {
	(struct usb_descriptor_header *)&auth_intf,
	(struct usb_descriptor_header *)&auth_hid_desc,
	(struct usb_descriptor_header *)&ss_auth_source_desc,
	(struct usb_descriptor_header *)&ss_auth_source_comp_desc,
	(struct usb_descriptor_header *)&ss_auth_sink_desc,
	(struct usb_descriptor_header *)&ss_auth_sink_comp_desc,
	NULL
};

static const char AuthenticationReportDescriptor[AUTHENTICATION_REPORT_DESC_LENGTH] =
{
	0x06, 0x00, 0xff,	   //@@SIE@@ USAGE_PAGE (Authentication)
	0x15, 0x00,		       //@@SIE@@ LOGICAL_MINIMUM (0)
	0x26, 0xff, 0x00,	   //@@SIE@@ LOGICAL_MAXIMUM (255)
	0x75, 0x08,		       //@@SIE@@ REPORT_SIZE (8)
	0x95, 0x3f,		       //@@SIE@@ REPORT_COUNT (63)

	0x09, 0x40,		       //@@SIE@@ USAGE (Device Authentication)
	0xa1, 0x01,		       //@@SIE@@ COLLECTION (Application)
	0x85, 0xf0,		       //@@SIE@@   REPORT_ID (240)
	0x09, 0x47,		       //@@SIE@@   USAGE (Get Auth2 Data)
	0x15, 0x00,		       //@@SIE@@   LOGICAL_MINIMUM (0)
	0x26, 0xff, 0x00,	   //@@SIE@@   LOGICAL_MAXIMUM (255)
	0x75, 0x08,		       //@@SIE@@   REPORT_SIZE (8bit)
	0x95, 0x3f,		       //@@SIE@@   REPORT_COUNT (63)
	0xb1, 0x02,		       //@@SIE@@   FEATURE (Data,Var,Abs)

	0x85, 0xf1,		       //@@SIE@@   REPORT_ID (241)
	0x09, 0x48,		       //@@SIE@@   USAGE (Get Auth Status)
	0x95, 0x3f,		       //@@SIE@@   REPORT_COUNT (63)
	0xb1, 0x02,		       //@@SIE@@   FEATURE (Data,Var,Abs)

	0x85, 0xf2,		       //@@SIE@@   REPORT_ID (242)
	0x09, 0x49,		       //@@SIE@@   USAGE (Get Auth Config)
	0x95, 0x0f,		       //@@SIE@@   REPORT_COUNT (15)
	0xb1, 0x02,		       //@@SIE@@   FEATURE (Data,Var,Abs)
	0xc0			       //@@SIE@@ END_COLLECTION
};
/* USB Descriptors */

#pragma pack(1)
struct usb_auth_status {
	uint8_t report_id;
	uint8_t reserved1;
	uint8_t sequence_number;
	uint8_t status;
	uint8_t reserved2[8];
	uint32_t crc32;
};

struct usb_auth1_data {
	uint8_t report_id;
	uint8_t sub_id;
	uint8_t sequence_number;
	uint8_t block_number;
	uint8_t data[SET_AUTH1_DATA_BLOCK_SIZE];
	uint32_t crc32;
};

struct usb_auth2_data {
	uint8_t report_id;
	uint8_t sub_id;
	uint8_t sequence_number;
	uint8_t block_number;
	uint8_t data[GET_AUTH2_DATA_BLOCK_SIZE];
	uint32_t crc32;
};

#pragma pack()

struct usb_auth_interface {
	struct usb_function	interface;

	struct usb_ep	*source_ep;
	struct usb_ep	*sink_ep;

	spinlock_t		lock;

	uint8_t		sequence_number;
	uint8_t		auth1_block_number;
	uint8_t		auth2_block_number;
	uint8_t		status;
	uint8_t		auth1_data[SET_AUTH1_DATA_BLOCK_SIZE * MAX_AUTH1_BLOCK_NUMBER];
	uint8_t		auth2_data[GET_AUTH2_DATA_BLOCK_SIZE * MAX_AUTH2_BLOCK_NUMBER];
	uint8_t		response_data[H_RESPONSE_LENGTH];
	int			num_of_received;
	int			num_of_sent;
	int			error;
	int			cancel;

	int				major; /* userspace device node */
	struct class	*registered_class;
	struct device	*nodedev;
};

static struct usb_auth_interface *sAuth = NULL;
#define AUTH_DEVNAME "usb_auth"
static const struct file_operations auth_fops;
static void usb_auth_unbind(struct usb_configuration *c, struct usb_function *f);

static void usb_auth_init_internal(struct usb_auth_interface *auth)
{
	// Device Authentication
	auth->status = AUTH_STATUS_INITIAL;
	auth->sequence_number = 0;
	auth->auth1_block_number = 0;
	auth->auth2_block_number = 0;
	auth->num_of_received = 0;
	auth->num_of_sent = 0;
} // usb_auth_init_internal

static long device_auth_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = AUTH_SUCCESS;
	unsigned long flags;

	if (sAuth == NULL) {
		GADGET_AUTH_LOG(LOG_ERR, "Device is not Initialized.\n");
		ret = AUTH_ERROR_NOT_INITIALIZED;
		return ret;
	}

	spin_lock_irqsave(&sAuth->lock, flags);

	if (sAuth->status == AUTH_STATUS_INITIAL) {
		sAuth->status = AUTH_STATUS_WAIT_H_1ST_CHALLENGE;
	}

	switch (cmd) {
	case HOST_IOCTL_GET_AUTH_STATE:
		if (arg == 0) {
			GADGET_AUTH_LOG(LOG_ERR, "arg is NULL.\n");
			ret = -EINVAL;
			break;
		}
		ret = copy_to_user((uint8_t*)arg, &sAuth->status, sizeof(sAuth->status));
		if (ret) {
			GADGET_AUTH_LOG(LOG_ERR, "copy_to_user error. ret=%d\n", ret);
			ret = -EFAULT;
		}
		break;

	case HOST_IOCTL_GET_H_CHALLENGE_DATA_1:
		GADGET_AUTH_LOG(LOG_DBG, "get H challenge data.\n");
		if (arg == 0) {
			GADGET_AUTH_LOG(LOG_ERR, "arg is NULL.\n");
			ret = -EINVAL;
			break;
		}
		if (sAuth->status == AUTH_STATUS_EXECUTING_H_1ST_CHALLENG) {
			ret = copy_to_user((uint8_t*)arg, &sAuth->auth1_data, H_DATA1_LENGTH);
			if (ret) {
				GADGET_AUTH_LOG(LOG_ERR, "copy_to_user error. ret=%d\n", ret);
				ret = -EFAULT;
			} else {
				sAuth->auth2_block_number = 0;
			}
		} else {
			GADGET_AUTH_LOG(LOG_ERR, "invalid status.\n");
			ret = -EINVAL;
		}
		break;

	case HOST_IOCTL_SET_D_RESPONSE_CHALLENGE_DATA_1:
		GADGET_AUTH_LOG(LOG_DBG, "d auth data copy.\n");
		if (arg == 0) {
			GADGET_AUTH_LOG(LOG_ERR, "arg is NULL.\n");
			ret = -EINVAL;
			break;
		}
		if (sAuth->status == AUTH_STATUS_EXECUTING_H_1ST_CHALLENG) {
			ret = copy_from_user(&sAuth->auth2_data, (uint8_t*)arg, D_DATA1_LENGTH);
			if (ret) {
				GADGET_AUTH_LOG(LOG_ERR, "copy_from_user error. ret=%d\n", ret);
				ret = -EFAULT;
			} else {
				sAuth->status = AUTH_STATUS_READY_D_1ST_RESPONSE;
			}
		} else {
			GADGET_AUTH_LOG(LOG_ERR, "invalid status.\n");
			ret = -EINVAL;
		}
		break;

	case HOST_IOCTL_GET_H_RESPONSE_DATA:
		GADGET_AUTH_LOG(LOG_DBG, "get H auth response data.\n");
		if (arg == 0) {
			GADGET_AUTH_LOG(LOG_ERR, "arg is NULL.\n");
			ret = -EINVAL;
			break;
		}
		ret = copy_to_user((uint8_t*)arg, &sAuth->response_data, H_RESPONSE_LENGTH);
		if (ret) {
			GADGET_AUTH_LOG(LOG_ERR, "copy_from_user error. ret=%d\n", ret);
			ret = -EFAULT;
		}
		break;

	case HOST_IOCTL_GET_H_CHALLENGE_DATA_2:
		GADGET_AUTH_LOG(LOG_DBG, "get H challenge data.\n");
		if (arg == 0) {
			GADGET_AUTH_LOG(LOG_ERR, "arg is NULL.\n");
			ret = -EINVAL;
			break;
		}
		if (sAuth->status == AUTH_STATUS_EXECUTING_H_CHALLENG) {
			ret = copy_to_user((uint8_t*)arg, &sAuth->auth1_data, H_DATA2_LENGTH);
			if (ret) {
				GADGET_AUTH_LOG(LOG_ERR, "copy_from_user error. ret=%d\n", ret);
				ret = -EFAULT;
			} else {
				sAuth->auth2_block_number = 0;
			}
		} else {
			GADGET_AUTH_LOG(LOG_ERR, "invalid status.\n");
			ret = -EINVAL;
		}
		break;

	case HOST_IOCTL_SET_D_RESPONSE_CHALLENGE_DATA_2:
		GADGET_AUTH_LOG(LOG_DBG, "d auth data copy.\n");
		if (arg == 0) {
			GADGET_AUTH_LOG(LOG_ERR, "arg is NULL.\n");
			ret = -EINVAL;
			break;
		}
		if (sAuth->status == AUTH_STATUS_EXECUTING_H_CHALLENG) {
			ret = copy_from_user(&sAuth->auth2_data, (uint8_t*)arg, D_DATA2_LENGTH);
			if (ret) {
				GADGET_AUTH_LOG(LOG_ERR, "copy_from_user error. ret=%d\n", ret);
				ret = -EFAULT;
			} else {
				sAuth->status = AUTH_STATUS_READY_D_RESPONSE;
			}
		} else {
			GADGET_AUTH_LOG(LOG_ERR, "invalid status.\n");
			ret = -EINVAL;
		}
		break;

	case HOST_IOCTL_SET_AUTH_OK:
		GADGET_AUTH_LOG(LOG_DBG, "set auth ok.\n");
		sAuth->status = AUTH_STATUS_AUTHENTICATE_OK;
		break;

	case HOST_IOCTL_SET_AUTH_ERROR:
		GADGET_AUTH_LOG(LOG_DBG, "Set Auth error.\n");
		sAuth->status = AUTH_STATUS_ERROR_AUTHENTICATE;
		break;

	default:
		break;
	}
	spin_unlock_irqrestore(&sAuth->lock, flags);
	return ret;
} // device_auth_ioctl

//----------------------------------------------------------------------------
static void dump_auth1(struct usb_auth1_data *auth1_data)
{
	GADGET_AUTH_LOG(LOG_DBG, "report_id      =0x%02x\n", auth1_data->report_id);
	GADGET_AUTH_LOG(LOG_DBG, "sub_id         =0x%02x\n", auth1_data->sub_id);
	GADGET_AUTH_LOG(LOG_DBG, "sequence_number=0x%02x\n", auth1_data->sequence_number);
	GADGET_AUTH_LOG(LOG_DBG, "block_number   =0x%02x\n", auth1_data->block_number);
#if defined(ENABLE_DUMP_LOG)
	{
	int i = 0;
	printk("auth1 data begin------------------------------\n");
	for (i = 0; i < sizeof(auth1_data->data); i++) {
		printk("0x%02x", auth1_data->data[i]);
		if ((i + 1) % 16 != 0)
			printk(",");
		if ((i + 1) % 16 == 0)
			printk("\n");
	}
	printk("auth1 data end------------------------------\n");
	}
#endif
	GADGET_AUTH_LOG(LOG_DBG, "crc32=%08x\n",auth1_data->crc32);
}

static void dump_auth2(struct usb_auth2_data *auth2_data)
{
	GADGET_AUTH_LOG(LOG_DBG, "report_id      =0x%02x\n", auth2_data->report_id);
	GADGET_AUTH_LOG(LOG_DBG, "sub_id         =0x%02x\n", auth2_data->sub_id);
	GADGET_AUTH_LOG(LOG_DBG, "sequence_number=0x%02x\n", auth2_data->sequence_number);
	GADGET_AUTH_LOG(LOG_DBG, "block_number   =0x%02x\n", auth2_data->block_number);
#if defined(ENABLE_DUMP_LOG)
	{
	int i = 0;
	printk("auth2 data begin------------------------------\n");
	for (i = 0; i < sizeof(auth2_data->data); i++) {
		printk("0x%02x", auth2_data->data[i]);
		if ((i + 1) % 16 != 0)
			printk(",");
		if ((i + 1) % 16 == 0)
			printk("\n");
	}
	printk("auth2 data end------------------------------\n");
	}
#endif
	GADGET_AUTH_LOG(LOG_DBG, "crc32=%08x\n",auth2_data->crc32);
}

static int usb_auth_set_auth1_data(struct usb_request *req, struct usb_auth_interface *auth)
{
	struct usb_auth1_data auth1_data;
	int ret = AUTH_SUCCESS;
	int length = 0;
	unsigned long flags;

	GADGET_AUTH_LOG(LOG_DBG, "start set auth1 data.\n");

	length = req->length;
	memset(&auth1_data, 0, sizeof(auth1_data));
	memcpy(&auth1_data, req->buf, length);
	dump_auth1(&auth1_data);

	spin_lock_irqsave(&auth->lock, flags);

	if ((auth->status != AUTH_STATUS_WAIT_H_1ST_CHALLENGE) && (auth->status != AUTH_STATUS_WAIT_H_RESPONSE) &&
		(auth->status != AUTH_STATUS_READY_D_RESPONSE) &&
		(auth->status != AUTH_STATUS_AUTHENTICATE_OK) && (auth->status != AUTH_STATUS_ERROR_AUTHENTICATE)) {
		GADGET_AUTH_LOG(LOG_ERR, "Invalid request. State error. current=0x%02x.\n", auth->status);
		ret = AUTH_ERROR_INVALID_REQUEST;
		goto cleanup;
	}
	if (auth1_data.block_number >= MAX_AUTH1_BLOCK_NUMBER) {
		GADGET_AUTH_LOG(LOG_ERR, "Invalid block number. limit over. value=%d\n", auth1_data.block_number);
		ret = AUTH_ERROR_INVALID_BLOCK_NUMBER;
		goto cleanup;
	}
	if (auth->sequence_number != auth1_data.sequence_number) {
		GADGET_AUTH_LOG(LOG_DBG, "Sequence number chenge. perv=%d, current=0x%02x\n", auth->sequence_number, auth1_data.sequence_number);

		auth->sequence_number = auth1_data.sequence_number;
		auth->num_of_received = 0;
		auth->auth1_block_number = 0;
	}
	if (auth->auth1_block_number != auth1_data.block_number) {
		GADGET_AUTH_LOG(LOG_ERR, "Invalid block number. sequence error. prev=%d, value=%d\n", auth->auth1_block_number, auth1_data.block_number);
		ret = AUTH_ERROR_INVALID_BLOCK_NUMBER;
		goto cleanup;
	}

	memcpy(&auth->auth1_data[auth1_data.block_number*SET_AUTH1_DATA_BLOCK_SIZE], &auth1_data.data[0], SET_AUTH1_DATA_BLOCK_SIZE);

	auth->auth1_block_number++;
	auth->num_of_received += SET_AUTH1_DATA_BLOCK_SIZE;
	GADGET_AUTH_LOG(LOG_DBG, "%d byte received. Total %d byte received.\n", length, auth->num_of_received);

	if (auth1_data.sub_id == SUB_ID_H_CHALLENGE_1) {
		if (auth->num_of_received >= H_DATA1_LENGTH) {
			GADGET_AUTH_LOG(LOG_DBG, "Received all data. Start creation of h auth data.\n");
			auth->status = AUTH_STATUS_EXECUTING_H_1ST_CHALLENG;
		}
	} else if (auth1_data.sub_id == SUB_ID_H_CHALLENGE_2) {
		if (auth->num_of_received >= H_DATA2_LENGTH) {
			GADGET_AUTH_LOG(LOG_DBG, "Received all data. Start creation of h auth data.\n");
			auth->status = AUTH_STATUS_EXECUTING_H_CHALLENG;
		}
	}

cleanup:
	spin_unlock_irqrestore(&auth->lock, flags);
	return ret;
} // usb_auth_set_auth1_data

static int usb_auth_set_response_data(struct usb_request *req, struct usb_auth_interface *auth)
{
	struct usb_auth1_data auth1_data;
	int ret = AUTH_SUCCESS;
	int length = 0;
	unsigned long flags;

	GADGET_AUTH_LOG(LOG_DBG, "start set response data.\n");

	length = req->length;
	memset(&auth1_data, 0, sizeof(auth1_data));
	memcpy(&auth1_data, req->buf, length);
	dump_auth1(&auth1_data);

	spin_lock_irqsave(&auth->lock, flags);

	if (auth->status != AUTH_STATUS_WAIT_H_RESPONSE) {
		GADGET_AUTH_LOG(LOG_ERR, "Invalid request. State error. current=0x%02x.\n", auth->status);
		ret = AUTH_ERROR_INVALID_REQUEST;
		goto cleanup;
	}
	if (auth1_data.block_number >= MAX_AUTH1_BLOCK_NUMBER) {
		GADGET_AUTH_LOG(LOG_ERR, "Invalid block number. limit over. value=%d\n", auth1_data.block_number);
		ret = AUTH_ERROR_INVALID_BLOCK_NUMBER;
		goto cleanup;
	}
	auth->num_of_received = 0;
	auth->auth1_block_number = 0;
	memcpy(&auth->response_data[0], &auth1_data.report_id, H_RESPONSE_LENGTH);

	auth->num_of_received += SET_AUTH1_DATA_BLOCK_SIZE;
	GADGET_AUTH_LOG(LOG_DBG, "%d byte received. Total %d byte received.\n", length, auth->num_of_received);

	if (auth->num_of_received >= H_DATA2_LENGTH) {
		GADGET_AUTH_LOG(LOG_DBG, "Received all data. Start creation of h auth data.\n");
		auth->status = AUTH_STATUS_VERIFYING_H_RESPONSE;
	}

cleanup:
	spin_unlock_irqrestore(&auth->lock, flags);
	return ret;
} // usb_auth_set_response_data

static void usb_auth_ctrl_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct usb_auth_interface *auth = NULL;
	unsigned char report_id;
	unsigned char sub_id;

	GADGET_AUTH_LOG(LOG_DBG, "ctrl_complete authentication interface...\n");
	auth = ep->driver_data;

	memcpy(&report_id, req->buf, 1);
	memcpy(&sub_id, req->buf + 1, 1);
	GADGET_AUTH_LOG(LOG_DBG, "report_id=0x%02x\n", report_id);

	switch (report_id) {
	case REPORT_ID_SET_AUTH1_DATA:		// Set auth1 data
		if (sub_id == SUB_ID_H_CHALLENGE_1) {
			usb_auth_set_auth1_data(req, auth);
		} else if (sub_id == SUB_ID_H_CHALLENGE_2) {
			usb_auth_set_auth1_data(req, auth);
		} else if (sub_id == SUB_ID_H_RESPONSE) {
			usb_auth_set_response_data(req, auth);
		}
		break;
	case REPORT_ID_GET_AUTH2_DATA:		// Get auth2 data
	case REPORT_ID_GET_AUTH_STATUS:		// Get auth status
	default:
		GADGET_AUTH_LOG(LOG_DBG, "do nothing.\n");
		break;
	}
} // usb_auth_ctrl_complete

static int usb_auth_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_auth_interface *auth = (struct usb_auth_interface *)f;
	int ret;

	GADGET_AUTH_LOG(LOG_DBG, "binding authentication interface...\n");

	ret = usb_string_id(c->cdev);
	if (ret < 0)
		return ret;
	strings_dev[STRING_AUTHENTICATION_IDX].id = ret;
	auth_intf.iInterface = ret;

	ret = usb_interface_id(c, f);
	if (ret < 0)
		return ret;
	auth_intf.bInterfaceNumber = ret;

	/* register devnode */
	ret = register_chrdev(auth->major, AUTH_DEVNAME, &auth_fops);
	if (ret < 0) {
		dev_info(&c->cdev->gadget->dev, "failed to register chrdev\n");
		goto fail;
	}

	auth->nodedev = device_create(auth->registered_class, NULL, MKDEV(auth->major, 0), auth, AUTH_DEVNAME);
	if (IS_ERR(auth->nodedev)) {
		dev_err(&c->cdev->gadget->dev, "failed to create nodedev\n");
		ret = PTR_ERR(auth->nodedev);
		goto fail;
	}

	/* Configure endpoint from the descriptor. */
	auth->source_ep = usb_ep_autoconfig_ss(c->cdev->gadget, &ss_auth_source_desc, &ss_auth_source_comp_desc);
	if (auth->source_ep == NULL) {
		GADGET_AUTH_LOG(LOG_ERR, "can't autoconfigure auth endpoint\n");
		ret = -ENODEV;
		goto fail;
	}

	auth->sink_ep = usb_ep_autoconfig_ss(c->cdev->gadget, &ss_auth_sink_desc, &ss_auth_sink_comp_desc);
	if (auth->sink_ep == NULL) {
		GADGET_AUTH_LOG(LOG_ERR, "can't autoconfigure auth endpoint\n");
		ret = -ENODEV;
		goto fail;
	}

	auth_hid_desc.desc[0].bDescriptorType = HID_DT_REPORT;
	auth_hid_desc.desc[0].wDescriptorLength = cpu_to_le16(AUTHENTICATION_REPORT_DESC_LENGTH);

	return AUTH_SUCCESS;

fail:
	usb_auth_unbind(c, f);
	return ret;
} // usb_auth_bind

static void usb_auth_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_auth_interface *auth = (struct usb_auth_interface *)f;

	GADGET_AUTH_LOG(LOG_DBG, "unbinding authentication interface...\n");

	if (auth->nodedev) {
		device_destroy(auth->registered_class, MKDEV(auth->major,0));
		auth->nodedev = NULL;
	}
	if (auth->major) {
		unregister_chrdev(auth->major, AUTH_DEVNAME);
		auth->major = 0;
	}
}

static int usb_auth_enable(struct usb_function *f)
{
	struct usb_auth_interface *auth = (struct usb_auth_interface *)f;
	int ret = 0;

	GADGET_AUTH_LOG(LOG_DBG, "enabling authentication interface...\n");

	/* Enable the endpoint. */
	ret = config_ep_by_speed(f->config->cdev->gadget, f, auth->source_ep);
	if (ret) {
		GADGET_AUTH_LOG(LOG_ERR, "error configuring auth endpoint. ret=%d\n", ret);
		return ret;
	}

	ret = usb_ep_enable(auth->source_ep);
	if (ret) {
		GADGET_AUTH_LOG(LOG_ERR, "error starting auth endpoint. ret=%d\n", ret);
		return ret;
	}

	/* Data for the completion callback. */
	auth->source_ep->driver_data = auth;

	ret = config_ep_by_speed(f->config->cdev->gadget, f, auth->sink_ep);
	if (ret) {
		GADGET_AUTH_LOG(LOG_ERR, "error configuring auth endpoint. ret=%d\n", ret);
		return ret;
	}

	ret = usb_ep_enable(auth->sink_ep);
	if (ret) {
		GADGET_AUTH_LOG(LOG_ERR, "error starting auth endpoint. ret=%d\n", ret);
		return ret;
	}

	auth->sink_ep->driver_data = auth;
	return AUTH_SUCCESS;
} //  usb_auth_enable

static void usb_auth_disable(struct usb_function *f)
{
	struct usb_auth_interface *auth = (struct usb_auth_interface *)f;

	GADGET_AUTH_LOG(LOG_DBG, "disabling authentication interface...\n");

	disable_ep(auth->source_ep);
	disable_ep(auth->sink_ep);
	usb_auth_init_internal(auth);

	return;
} // usb_auth_disable

static int usb_auth_reset(struct usb_function *f, unsigned intf, unsigned alt)
{
	if (alt)
		return -EINVAL;

	GADGET_AUTH_LOG(LOG_DBG, "intf(%d)\n", intf);
	usb_auth_disable(f);
	return usb_auth_enable(f);
} // usb_auth_reset

static int usb_auth_get_auth_status(struct usb_request *req, struct usb_auth_interface *auth, uint16_t length)
{
	int ret = AUTH_SUCCESS;
	struct usb_auth_status auth_status;

	GADGET_AUTH_LOG(LOG_DBG, "Get Auth status. status=0x%02x, sequence=%d\n", auth->status, auth->sequence_number);

	memset(&auth_status, 0, sizeof(auth_status));
	auth_status.report_id = REPORT_ID_GET_AUTH_STATUS;
	auth_status.sequence_number = auth->sequence_number;
	auth_status.status = auth->status;
	memcpy(req->buf, &auth_status, sizeof(auth_status));
	auth_status.crc32 = crc32_be(~0, (unsigned const char *)&auth_status, H_RESPONSE_CRC_LENGTH);
	GADGET_AUTH_LOG(LOG_DBG, "crc32=%08x\n",auth_status.crc32);

	return ret;
} // mbrige_auth_get_auth_status

static int usb_auth_get_auth2_data(struct usb_request *req, struct usb_auth_interface *auth, uint16_t length)
{
	int ret = AUTH_SUCCESS;
	int send_size;
	struct usb_auth2_data auth2_data;

	GADGET_AUTH_LOG(LOG_DBG, "Get Auth2 data. status=0x%02x, sequence=%d, block=%d\n",
			auth->status, auth->sequence_number, auth->auth2_block_number);

	memset(&auth2_data, 0, sizeof(auth2_data));
	auth2_data.report_id = REPORT_ID_GET_AUTH2_DATA;
	if (auth->status == AUTH_STATUS_READY_D_1ST_RESPONSE) {
		auth2_data.sub_id = SUB_ID_D_RESPONSE_CHALLENGE_1;
	} else if (auth->status == AUTH_STATUS_READY_D_RESPONSE) {
		auth2_data.sub_id = SUB_ID_D_RESPONSE_CHALLENGE_2;
	} else {
		GADGET_AUTH_LOG(LOG_WARN, "auth2 not created.\n");
		memcpy(req->buf, &auth2_data, sizeof(auth2_data));
		return AUTH_ERROR_NOT_CREATED;
	}

	if (auth->auth2_block_number >= MAX_AUTH2_BLOCK_NUMBER) {
		GADGET_AUTH_LOG(LOG_WARN, "auth2 no data.\n");
		memcpy(req->buf, &auth2_data, sizeof(auth2_data));
		return AUTH_ERROR_NO_DATA;
	}

	auth2_data.sequence_number = auth->sequence_number;
	auth2_data.block_number = auth->auth2_block_number;

	// data copy.
	if (auth2_data.sub_id == SUB_ID_D_RESPONSE_CHALLENGE_1) {
		if (auth->num_of_sent >= (D_DATA1_LENGTH - GET_AUTH2_DATA_BLOCK_SIZE)) {
			send_size = (D_DATA1_LENGTH - auth->num_of_sent);
		} else {
			send_size = GET_AUTH2_DATA_BLOCK_SIZE;
		}
	} else if (auth2_data.sub_id == SUB_ID_D_RESPONSE_CHALLENGE_2) {
		send_size = GET_AUTH2_DATA_BLOCK_SIZE;
	}

	GADGET_AUTH_LOG(LOG_DBG, "Get Auth2 data. send size=%d\n", send_size);
	memcpy(&auth2_data.data[0], &auth->auth2_data[auth->auth2_block_number*GET_AUTH2_DATA_BLOCK_SIZE], send_size);
	auth2_data.crc32 = crc32_be(~0, (unsigned const char *)&auth2_data, CRC_TARGET_LENGTH);
	auth->auth2_block_number++;
	dump_auth2(&auth2_data);
	memcpy(req->buf, &auth2_data, sizeof(auth2_data));
	auth->num_of_sent += send_size;

	if (auth->status == AUTH_STATUS_READY_D_1ST_RESPONSE) {
		if (auth->num_of_sent >= D_DATA1_LENGTH) {
			GADGET_AUTH_LOG(LOG_DBG, "Send all data.\n");
			auth->status = AUTH_STATUS_WAIT_H_RESPONSE;
		}
	} else if (auth->status == AUTH_STATUS_READY_D_RESPONSE) {
		if (auth->num_of_sent >= D_DATA2_LENGTH) {
			GADGET_AUTH_LOG(LOG_DBG, "Send all data.\n");
			auth->status = AUTH_STATUS_WAIT_H_RESPONSE;
		}
	}

	return ret;
} // usb_auth_get_auth2_data

static int usb_auth_get_report(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct usb_auth_interface *auth = (struct usb_auth_interface *)f;
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request *req = cdev->req;
	uint8_t report_id;
	__u16 value, length;
	int ret = AUTH_SUCCESS;

	value  = __le16_to_cpu(ctrl->wValue);
	length = __le16_to_cpu(ctrl->wLength);

	report_id = (value & 0x00FF);
	GADGET_AUTH_LOG(LOG_DBG, "report_id=0x%02x\n", report_id);

	/* report id */
	switch (report_id) {
	case REPORT_ID_GET_AUTH2_DATA: // Get auth2 data
		ret = usb_auth_get_auth2_data(req, auth, length);
		break;
	case REPORT_ID_GET_AUTH_STATUS: // Get auth status
		ret = usb_auth_get_auth_status(req, auth, length);
		break;
	default:
		GADGET_AUTH_LOG(LOG_DBG, "do nothing.\n");
		ret = AUTH_ERROR_NOT_SUPPORTED;
		break;
	}

	return ret;
} // usb_auth_get_report

static int usb_auth_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct usb_auth_interface *auth = (struct usb_auth_interface *)f;
	int ret = AUTH_SUCCESS;

	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request *req = cdev->req;
	uint16_t value, length;

	value  = __le16_to_cpu(ctrl->wValue);
	length = __le16_to_cpu(ctrl->wLength);

	GADGET_AUTH_LOG(LOG_DBG, "usb_auth_setup\n");
	GADGET_AUTH_LOG(LOG_DBG, "control req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest, value, le16_to_cpu(ctrl->wIndex), length);

	switch ((ctrl->bRequestType << 8) | ctrl->bRequest) {

	case ((USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8 | HID_REQ_GET_REPORT):
		GADGET_AUTH_LOG(LOG_DBG, "get_report\n");
		ret = usb_auth_get_report(f, ctrl);
		break;

	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8 | HID_REQ_SET_REPORT):
		GADGET_AUTH_LOG(LOG_DBG, "set_report | wLength=%d\n", ctrl->wLength);
		req->complete = usb_auth_ctrl_complete;
		break;

	case ((USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_INTERFACE) << 8 | USB_REQ_GET_DESCRIPTOR):
		GADGET_AUTH_LOG(LOG_DBG, "USB_REQ_GET_DESCRIPTOR\n");
		switch (value >> 8) {
		case HID_DT_HID:
			GADGET_AUTH_LOG(LOG_DBG, "USB_REQ_GET_DESCRIPTOR: HID\n");
			length = min_t(unsigned short, length, auth_hid_desc.bLength);
			memcpy(req->buf, &auth_hid_desc, length);
			break;

		case HID_DT_REPORT:
			GADGET_AUTH_LOG(LOG_DBG, "USB_REQ_GET_DESCRIPTOR: REPORT\n");
			length = min_t(unsigned short, length, AUTHENTICATION_REPORT_DESC_LENGTH);
			memcpy(req->buf, AuthenticationReportDescriptor, length);
			break;
		default:
			GADGET_AUTH_LOG(LOG_DBG, "Unknown descriptor request 0x%x\n", value >> 8);
			ret = AUTH_ERROR_NOT_SUPPORTED;
			break;
		}
		break;
	default:
		GADGET_AUTH_LOG(LOG_DBG, "Unknown request 0x%x\n", ctrl->bRequest);
		ret = AUTH_ERROR_NOT_SUPPORTED;
		break;
	}

	if (ret == AUTH_ERROR_NOT_SUPPORTED)
		return -EOPNOTSUPP;

	req->zero = 0;
	req->length = length;
	cdev->gadget->ep0->driver_data = auth;
	ret = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
	if (ret < 0)
		GADGET_AUTH_LOG(LOG_DBG, "usb_ep_queue error on ep0 %d, ret=%d\n", value, ret);
	return ret;
} // usb_auth_setup


//----------------------------------------------------------------------------

static const struct file_operations auth_fops = {
	.unlocked_ioctl = device_auth_ioctl,
};

int usb_auth_init(struct class *class, struct usb_function **interface)
{
	struct usb_auth_interface *auth;

	GADGET_AUTH_LOG(LOG_DBG, "\n");

	sAuth = auth = kzalloc(sizeof (*auth), GFP_KERNEL);
	if (auth == NULL)
		return -ENOMEM;

	spin_lock_init(&auth->lock);

	auth->registered_class = class;
	auth->major = SCE_KM_CDEV_MAJOR_USB_AUTH;

	auth->interface.name = "auth";
	auth->interface.fs_descriptors = fs_auth_descs;
	auth->interface.hs_descriptors = NULL;
	auth->interface.ss_descriptors = ss_auth_descs;
	auth->interface.bind = usb_auth_bind;
	auth->interface.unbind = usb_auth_unbind;
	auth->interface.set_alt = usb_auth_reset;
	auth->interface.disable = usb_auth_disable;
	auth->interface.setup = usb_auth_setup;

	usb_auth_init_internal(auth);

	*interface = &auth->interface;
	return 0;
} // usb_auth_init


void usb_auth_destroy(void)
{
	kfree(sAuth);
	sAuth = NULL;
} // usb_auth_destroy
