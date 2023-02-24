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

#include <linux/usb/audio.h>
#include "usb.h"
#include "audio_ctrl.h"

// PS4 MIC  USB-IN  <- OT3 <- IT1 <- HMU

#define AUDIO_AC_INTERFACE				1
#define AUDIO_AS_MIC_INTERFACE			2
#define AUDIO_NUM_INTERFACES			1
#define IN_EP_MAX_PACKET_SIZE			32
#define AUDIO_CONTROL_MUTE				0x01
#define AUDIO_CONTROL_VOLUME			0x02

static struct usb_interface_assoc_descriptor iad_desc = {
	.bLength 			= sizeof(iad_desc),
	.bDescriptorType 	= USB_DT_INTERFACE_ASSOCIATION,
	.bFirstInterface 	= 1,
	.bInterfaceCount 	= 3,
	.bFunctionClass 	= USB_CLASS_AUDIO,
	.bFunctionSubClass 	= 0,
	.bFunctionProtocol 	= UAC_VERSION_1,
};

/* UAC Audio Control(Mic) standard_ac_interface_des */
static struct usb_interface_descriptor ac_interface_desc = {
	.bLength 			= USB_DT_INTERFACE_SIZE,
	.bDescriptorType 	= USB_DT_INTERFACE,
	.bNumEndpoints 		= 0,
	.bInterfaceClass 	= USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOCONTROL,
};

DECLARE_UAC_AC_HEADER_DESCRIPTOR(2);

#define UAC_DT_AC_HEADER_LENGTH UAC_DT_AC_HEADER_SIZE(AUDIO_NUM_INTERFACES)
/* 1 input terminal,  and 1 feature unit */
#define UAC_DT_TOTAL_LENGTH                                       \
	(UAC_DT_AC_HEADER_LENGTH + UAC_DT_INPUT_TERMINAL_SIZE + UAC_DT_OUTPUT_TERMINAL_SIZE +       \
	 UAC_DT_FEATURE_UNIT_SIZE(0))

/* Class-Specific AC Interface Descriptor */
static struct uac1_ac_header_descriptor_2 ac_header_desc = {
	.bLength 			= UAC_DT_AC_HEADER_LENGTH,
	.bDescriptorType 	= USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = UAC_HEADER,
	.bcdADC 			= __constant_cpu_to_le16(0x0100),
	.wTotalLength 		= __constant_cpu_to_le16(UAC_DT_TOTAL_LENGTH),
	.bInCollection 		= AUDIO_NUM_INTERFACES,
	.baInterfaceNr 		= {[0] = AUDIO_AS_MIC_INTERFACE,}
};

#define INPUT_TERMINAL_ID 1
static struct uac_input_terminal_descriptor input_terminal_desc = {
	.bLength 			= UAC_DT_INPUT_TERMINAL_SIZE,
	.bDescriptorType 	= USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = UAC_INPUT_TERMINAL,
	.bTerminalID 		= INPUT_TERMINAL_ID,
	.wTerminalType 		= UAC_INPUT_TERMINAL_MICROPHONE,
	.bAssocTerminal 	= 0,
	.bNrChannels 		= 1,
	.wChannelConfig 	= 0x00, // mono
};

// Mono Feature Unit Descriptor
DECLARE_UAC_FEATURE_UNIT_DESCRIPTOR(0);

#define FEATURE_UNIT_ID 2
static struct uac_feature_unit_descriptor_0 feature_unit_desc = {
	.bLength 			= UAC_DT_FEATURE_UNIT_SIZE(0),
	.bDescriptorType 	= USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = UAC_FEATURE_UNIT,
	.bUnitID 			= FEATURE_UNIT_ID,
	.bSourceID 			= INPUT_TERMINAL_ID,
	.bControlSize 		= 1,
	.bmaControls 		= {[0] = 0x0003, }
};

#define OUTPUT_TERMINAL_ID 3
static struct uac1_output_terminal_descriptor output_terminal_desc = {
	.bLength 			= UAC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType 	= USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = UAC_OUTPUT_TERMINAL,
	.bTerminalID 		= OUTPUT_TERMINAL_ID,
	.wTerminalType 		= UAC_TERMINAL_STREAMING,
	.bAssocTerminal 	= 0,
	.bSourceID 			= FEATURE_UNIT_ID,
};

static struct usb_descriptor_header *fs_micctrl_descs[] = {
	//	(struct usb_descriptor_header *)&iad_desc,
	(struct usb_descriptor_header *)&ac_interface_desc,
	(struct usb_descriptor_header *)&ac_header_desc,
	(struct usb_descriptor_header *)&input_terminal_desc,
	(struct usb_descriptor_header *)&feature_unit_desc,
	(struct usb_descriptor_header *)&output_terminal_desc,
	NULL
};

static struct usb_descriptor_header *ss_micctrl_descs[] = {
	//	(struct usb_descriptor_header *)&iad_desc,
	(struct usb_descriptor_header *)&ac_interface_desc,
	(struct usb_descriptor_header *)&ac_header_desc,
	(struct usb_descriptor_header *)&input_terminal_desc,
	(struct usb_descriptor_header *)&feature_unit_desc,
	(struct usb_descriptor_header *)&output_terminal_desc,
	NULL
};

struct usb_audio_ctrl_interface
{
	struct usb_function interface;
};

static struct usb_audio_ctrl_interface *sMicCtrl;
//----------------------------------------------------------------------------

static int usb_audio_ctrl_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_audio_ctrl_interface *ctrl_interface =
	    (struct usb_audio_ctrl_interface *)f;
	int id;

	ASSERT(c != NULL);
	ASSERT(ctrl_interface != NULL);

	id = usb_string_id(c->cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_AUDIO_CONTROL_IDX].id = id;
	ac_interface_desc.iInterface = id;

	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	ac_interface_desc.bInterfaceNumber = id;

	return 0;
}

static int usb_audio_ctrl_enable(struct usb_function *f)
{
	struct usb_audio_ctrl_interface *ctrl_interface =
	    (struct usb_audio_ctrl_interface *)f;

	ASSERT(ctrl_interface != NULL);

	GADGET_LOG(LOG_DBG, "enabling micctrl interface...\n");
	return 0;
}

static void usb_audio_ctrl_disable(struct usb_function *f)
{
	struct usb_audio_ctrl_interface *ctrl_interface =
	    (struct usb_audio_ctrl_interface *)f;

	ASSERT(ctrl_interface != NULL);

	GADGET_LOG(LOG_DBG, "disabling micctrl interface...\n");
}

static int usb_audio_ctrl_reset(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct usb_audio_ctrl_interface *ctrl_interface =
	    (struct usb_audio_ctrl_interface *)f;

	GADGET_LOG(LOG_DBG, "intf(%d)\n", intf);

	ASSERT(ctrl_interface != NULL);
	ASSERT(alt == 0);

	usb_audio_ctrl_disable(f);
	return usb_audio_ctrl_enable(f);
}

static void audio_control_complete(struct usb_ep *ep, struct usb_request *req)
{
	/* nothing to do here */
}

static int audio_set_endpoint_req(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	int value = -EOPNOTSUPP;
	u16 len = le16_to_cpu(ctrl->wLength);

	switch (ctrl->bRequest) {
	case UAC_SET_CUR:
	case UAC_SET_MIN:
	case UAC_SET_MAX:
	case UAC_SET_RES:
		value = len;
		break;
	default:
		break;
	}

	return value;
}

static int audio_get_endpoint_req(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct usb_composite_dev *cdev = f->config->cdev;
	int value = -EOPNOTSUPP;
	u8 ep = ((le16_to_cpu(ctrl->wIndex) >> 8) & 0xFF);
	u16 w_valueH = ((le16_to_cpu(ctrl->wValue) >> 8) & 0xFF);
	u16 w_valueL = (le16_to_cpu(ctrl->wValue) & 0x00FF);
	u8 *buf = cdev->req->buf;

	switch (ctrl->bRequest) {
	case UAC_GET_CUR:
		if (FEATURE_UNIT_ID == ep) {
			switch (w_valueH) {
			case AUDIO_CONTROL_MUTE:
				buf[0] = 0x01;
				value = 1;
				break;
			case AUDIO_CONTROL_VOLUME:
				if (w_valueL) {
					buf[0] = 0xF0;
					buf[1] = 0x00;
					// L
					value = 2;
				} else {
					buf[0] = 0xF0;
					buf[1] = 0x00;
					// R
					value = 2;
				}
				break;
			default:
				/* STALL control pipe */
				value = 0;
				break;
			}
		} else {
			/* STALL control pipe */
			value = 0;
		}
		break;
	case UAC_GET_MIN:
		if (FEATURE_UNIT_ID == ep) {
			switch (w_valueH) {
			case AUDIO_CONTROL_VOLUME: // L & R
				buf[0] = 0x00;     // 0dB
				buf[1] = 0x80;
				value = 2;
				break;
			default:
				/* STALL control pipe */
				value = 0;
				break;
			}
		} else {
			/* STALL control pipe */
			value = 0;
		}
		break;
	case UAC_GET_MAX:
		if (FEATURE_UNIT_ID == ep) {
			switch (w_valueH) {
			case AUDIO_CONTROL_VOLUME: // L & R
				buf[0] = 0xFF;
				buf[1] = 0x7F;
				value = 2;
				break;
			default:
				/* STALL control pipe */
				value = 0;
				break;
			}
		} else {
			/* STALL control pipe */
			value = 0;
		}
		break;
	case UAC_GET_RES:
		if (FEATURE_UNIT_ID == ep) {
			switch (w_valueH) {
			case AUDIO_CONTROL_VOLUME: // L & R
				buf[0] = 0x00;
				buf[1] = 0x04;
				value = 2;
				break;
			default:
				/* STALL control pipe */
				value = 0;
				break;
			}
		} else {
			/* STALL control pipe */
			value = 0;
		}
		break;
	default:
		GADGET_LOG(LOG_DBG, " default\n");
		break;
	}

	return value;
}

static int usb_audio_ctrl_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request *req = cdev->req;
	int value = -EOPNOTSUPP;

	GADGET_LOG(LOG_DBG, "usb_audio_ctrl_setup\n");

	switch (ctrl->bRequestType) {
	case USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE:
		value = audio_set_endpoint_req(f, ctrl); // Same NXP Bridge
		break;

	case USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE:
		value = audio_get_endpoint_req(f, ctrl); // // Same NXP Bridge
		break;

	case USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_ENDPOINT:
		value = audio_set_endpoint_req(f, ctrl);
		break;

	case USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_ENDPOINT:
		value = audio_get_endpoint_req(f, ctrl);
		break;

	default:
		break;
	}

	/* respond with data transfer or status phase? */
	if (value >= 0) {
//		GADGET_LOG(LOG_DBG, "audio req%02x.%02x v%04x i%04x l%d\n", ctrl->bRequestType,
//			   ctrl->bRequest, w_value, w_index, w_length);
		req->zero = 0;
		req->length = value;
		req->complete = audio_control_complete;
		value = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
		if (value < 0)
			GADGET_LOG(LOG_DBG, "audio response on err %d\n", value);
	}

	/* device either stalls (value < 0) or reports success */
	return value;
}

//----------------------------------------------------------------------------

int usb_audio_ctrl_init(struct usb_function **interface)
{
	struct usb_audio_ctrl_interface *ctrl_interface;

	ASSERT(interface != NULL);

	sMicCtrl = ctrl_interface = kzalloc(sizeof(*ctrl_interface), GFP_KERNEL);
	if (sMicCtrl == NULL)
		return -ENOMEM;

	ctrl_interface->interface.name = "uac_ctrl";
	ctrl_interface->interface.fs_descriptors = fs_micctrl_descs;
	ctrl_interface->interface.hs_descriptors = NULL;
	ctrl_interface->interface.ss_descriptors = ss_micctrl_descs;
	ctrl_interface->interface.bind = usb_audio_ctrl_bind;
	ctrl_interface->interface.set_alt = usb_audio_ctrl_reset;
	ctrl_interface->interface.disable = usb_audio_ctrl_disable;
	ctrl_interface->interface.setup = usb_audio_ctrl_setup;

	*interface = &ctrl_interface->interface;
	return 0;
}

void usb_audio_ctrl_destroy(void)
{
	ASSERT(sMicCtrl != NULL);
	kfree(sMicCtrl);
	sMicCtrl = NULL;
}
