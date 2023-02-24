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
#include <sieaudio.h>
#include "usb.h"
#include "mic.h"
#include "stat.h"


#define MIC_ENDPOINT_NUM_REQUESTS 16

#define MIC_NORMAL_REQUEST_SIZE 96

#define MIC_REQUEST_NONE		0
#define MIC_REQUEST_QUEUED		1

//----------------------------------------------------------------------------
#define IN_EP_MAX_PACKET_SIZE (MIC_NORMAL_REQUEST_SIZE)
#define OUTPUT_TERMINAL_ID (3)

#define MIC_INTERFACE_RESET_TIMEOUT (200)

/* Standard AS Interface Descriptor */
static struct usb_interface_descriptor as_interface_alt_0_desc = {
	.bLength 			= USB_DT_INTERFACE_SIZE,
	.bDescriptorType 	= USB_DT_INTERFACE,
	.bAlternateSetting 	= 0,
	.bNumEndpoints 		= 0,
	.bInterfaceClass 	= USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOSTREAMING,
};

static struct usb_interface_descriptor as_interface_alt_1_desc = {
	.bLength 			= USB_DT_INTERFACE_SIZE,
	.bDescriptorType 	= USB_DT_INTERFACE,
	.bAlternateSetting 	= 1,
	.bNumEndpoints 		= 1,
	.bInterfaceClass 	= USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOSTREAMING,
};

/* B.4.2  Class-Specific AS Interface Descriptor */
static struct uac1_as_header_descriptor as_header_desc = {
	.bLength 			= UAC_DT_AS_HEADER_SIZE,
	.bDescriptorType 	= USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = UAC_AS_GENERAL,
	.bTerminalLink 		= OUTPUT_TERMINAL_ID, // INPUT_TERMINAL_ID,
	.bDelay 			= 0,
	.wFormatTag 		= UAC_FORMAT_TYPE_I_PCM,
};

DECLARE_UAC_FORMAT_TYPE_I_DISCRETE_DESC(1);

static struct uac_format_type_i_discrete_descriptor_1 as_type_i_desc = {
	.bLength 			= UAC_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(1),
	.bDescriptorType 	= USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = UAC_FORMAT_TYPE,
	.bFormatType 		= UAC_FORMAT_TYPE_I,
	.bSubframeSize 		= 2,
	.bBitResolution 	= 16, // 16bit
	.bSamFreqType 		= 1,
// 48kHz
	.bNrChannels 		= 1, // 1ch
	.tSamFreq[0][0] 	= 0x80,
	.tSamFreq[0][1] 	= 0xBB,
	.tSamFreq[0][2] 	= 0x00
};

/* Standard ISO IN Endpoint Descriptor for superspeed */
static struct usb_endpoint_descriptor ss_as_in_ep_desc = {
	.bLength 			= USB_DT_ENDPOINT_AUDIO_SIZE,
	.bDescriptorType 	= USB_DT_ENDPOINT,
	.bEndpointAddress 	= MIC_ENDPOINT_SOURCE | USB_DIR_IN,
	.bmAttributes 		= USB_ENDPOINT_SYNC_ASYNC | USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize 	= __constant_cpu_to_le16(IN_EP_MAX_PACKET_SIZE),
	.bInterval 			= 4, /* poll 1 per millisecond */
};

static struct usb_ss_ep_comp_descriptor ss_as_in_ep_comp_desc = {
	.bLength 			= sizeof(ss_as_in_ep_comp_desc),
	.bDescriptorType 	= USB_DT_SS_ENDPOINT_COMP,
	.wBytesPerInterval	= __constant_cpu_to_le16(MIC_NORMAL_REQUEST_SIZE)
};

/* Class-specific AS ISO IN Endpoint Descriptor */
static struct uac_iso_endpoint_descriptor as_iso_in_desc = {
	.bLength 			= UAC_ISO_ENDPOINT_DESC_SIZE,
	.bDescriptorType 	= USB_DT_CS_ENDPOINT,
	.bDescriptorSubtype = UAC_EP_GENERAL,
	.bmAttributes 		= 0x80, // 1,
	.bLockDelayUnits 	= 0,
	.wLockDelay 		= 0, //__constant_cpu_to_le16(1),
};

/* Standard ISO IN Endpoint Descriptor for highspeed */
static struct usb_endpoint_descriptor fs_as_in_ep_desc = {
	.bLength 			= USB_DT_ENDPOINT_AUDIO_SIZE,
	.bDescriptorType 	= USB_DT_ENDPOINT,
	.bEndpointAddress 	= MIC_ENDPOINT_SOURCE | USB_DIR_IN,
	.bmAttributes 		= USB_ENDPOINT_SYNC_ASYNC | USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize 	= __constant_cpu_to_le16(IN_EP_MAX_PACKET_SIZE),
	.bInterval 			= 1, /* poll 1 per millisecond */
};

static struct usb_descriptor_header *fs_mic_descs[] = {
	(struct usb_descriptor_header *)&as_interface_alt_0_desc,
	(struct usb_descriptor_header *)&as_interface_alt_1_desc,
	(struct usb_descriptor_header *)&as_header_desc,
	(struct usb_descriptor_header *)&as_type_i_desc,
	(struct usb_descriptor_header *)&fs_as_in_ep_desc,
	(struct usb_descriptor_header *)&as_iso_in_desc,
	NULL,
};

static struct usb_descriptor_header *ss_mic_descs[] = {
	(struct usb_descriptor_header *)&as_interface_alt_0_desc,
	(struct usb_descriptor_header *)&as_interface_alt_1_desc,
	(struct usb_descriptor_header *)&as_header_desc,
	(struct usb_descriptor_header *)&as_type_i_desc,
	(struct usb_descriptor_header *)&ss_as_in_ep_desc,
	(struct usb_descriptor_header *)&ss_as_in_ep_comp_desc,
	(struct usb_descriptor_header *)&as_iso_in_desc,
	NULL,
};

struct usb_mic_interface
{
	struct usb_function interface;

	struct usb_ep *source_ep;
	int alt;

	// members for Tx
	struct usb_request *requests[MIC_ENDPOINT_NUM_REQUESTS];
	atomic_t request_avails;
	int request_flg[MIC_ENDPOINT_NUM_REQUESTS];
	spinlock_t lock;
	int ready;

	void *audio_hundler;
	const char *mic_buffer_start;
	int mic_buffer_total_size;
	const char *req_audio_address;
	int req_audio_size;
	int mic_buffer_previous_offset;
};

static struct usb_mic_interface *sMic = NULL;
static struct usb_mic_stat sStat = {0};
static void usb_mic_disable(struct usb_function *f);
static int usb_mic_enable(struct usb_function *f);
static void usb_mic_callback(void);

void usb_mic_get_stat(struct usb_mic_stat* stat)
{
	*stat = sStat;
}
EXPORT_SYMBOL(usb_mic_get_stat);

static int find_available(void)
{
	int i;
	for (i = 0; i < MIC_ENDPOINT_NUM_REQUESTS; i++) {
		if (sMic->request_flg[i] == MIC_REQUEST_NONE)
			break;
	}
	return i;
}

static int mic_request(const void *buf, size_t buflen)
{
	int index = find_available();
	int ret;
	if (index == MIC_ENDPOINT_NUM_REQUESTS)
		return -ENOENT;

	ASSERT(buf != NULL);
	sMic->requests[index]->buf    = (void *)buf;
	sMic->requests[index]->length = buflen;

	ret = usb_ep_queue(sMic->source_ep, sMic->requests[index], GFP_ATOMIC);
	if (ret) {
		return ret;
	}

	ASSERT(sMic->request_flg[index] == MIC_REQUEST_NONE);
	sMic->request_flg[index] = MIC_REQUEST_QUEUED;
	atomic_dec(&sMic->request_avails);
	sStat.request_avails = atomic_read(&sMic->request_avails);
	sStat.sent++;
	return 0;
}

static int usb_mic_send(const void *buf, size_t buflen)
{
	int ret = 0;
	unsigned long flags;

	if (buflen > MIC_NORMAL_REQUEST_SIZE) {
		sStat.send_error_inval++;
		return -EINVAL;
	}
	if (sMic == NULL) {
		sStat.send_error_nodev++;
		return -ENODEV;
	}

	spin_lock_irqsave(&sMic->lock, flags);
	if (!sMic->ready) {
		spin_unlock_irqrestore(&sMic->lock,flags);
		sStat.send_error_again++;
		return -EAGAIN;
	}
	ret = mic_request(buf, buflen);
	if (ret) {
		spin_unlock(&sMic->lock);
		if (ret == -EINVAL)
			sStat.send_error_inval++;
		else if (ret == -ENODEV)
			sStat.send_error_nodev++;
		else if (ret == -EAGAIN)
			sStat.send_error_again++;
		else if (ret == -ENOENT)
			sStat.send_error_noent++;
		else
			sStat.send_error_others++;
		return ret;
	}
	spin_unlock_irqrestore(&sMic->lock, flags);
	return 0;
}

// called from audio interrupt
static void usb_mic_callback(void)
{
	int offset;
	int ret;

	ASSERT(sMic->audio_hundler != NULL);

	if (!sMic->ready) {
		return;
	}
	if (!sMic->alt) {
		return;
	}
	if (!sMic->mic_buffer_start) {
		sMic->mic_buffer_start = sieAudioGetDmaBufferAddress(sMic->audio_hundler);
		ASSERT(sMic->mic_buffer_start != 0);
		GADGET_LOG(LOG_DBG, "mic_buffer_start=0x%p\n",sMic->mic_buffer_start);
	}
	if (!sMic->mic_buffer_total_size) {
		sMic->mic_buffer_total_size = sieAudioGetDmaBufferSize(sMic->audio_hundler);
		ASSERT(sMic->mic_buffer_total_size != 0);
		GADGET_LOG(LOG_DBG, "mic_buffer_total_size=0x%x\n",sMic->mic_buffer_total_size);
	}

	offset = sieAudioGetCaptureOffset(sMic->audio_hundler);
	if (sMic->mic_buffer_previous_offset == -1) {
		sMic->mic_buffer_previous_offset
			= (offset + MIC_NORMAL_REQUEST_SIZE - 1) / MIC_NORMAL_REQUEST_SIZE * MIC_NORMAL_REQUEST_SIZE;
		return;
	}
	if (sMic->mic_buffer_previous_offset == offset) {
		GADGET_LOG(LOG_ERR, "mic bufffer has no progress.\n");
		return;
	}

	if (offset - sMic->mic_buffer_previous_offset > 0) {
		while (offset - sMic->mic_buffer_previous_offset >= MIC_NORMAL_REQUEST_SIZE) {
			sMic->req_audio_address = sMic->mic_buffer_start + sMic->mic_buffer_previous_offset;
			sMic->req_audio_size = MIC_NORMAL_REQUEST_SIZE;
			ret = usb_mic_send(sMic->req_audio_address, sMic->req_audio_size);
			if (ret) {
				GADGET_LOG(LOG_ERR, "usb_mic_send fail. %d\n",ret);
			}
			sMic->mic_buffer_previous_offset += MIC_NORMAL_REQUEST_SIZE;
		}
	} else {
		while (sMic->mic_buffer_total_size - sMic->mic_buffer_previous_offset >= MIC_NORMAL_REQUEST_SIZE) {
			sMic->req_audio_address = sMic->mic_buffer_start + sMic->mic_buffer_previous_offset;
			sMic->req_audio_size = MIC_NORMAL_REQUEST_SIZE;
			sMic->mic_buffer_previous_offset += MIC_NORMAL_REQUEST_SIZE;
			ret = usb_mic_send(sMic->req_audio_address, sMic->req_audio_size);
			if (ret) {
				GADGET_LOG(LOG_ERR, "usb_mic_send fail. %d\n",ret);
			}
		}
		if (sMic->mic_buffer_total_size - sMic->mic_buffer_previous_offset != 0) {
			GADGET_LOG(LOG_ERR, "mic_buffer_total_size should be multipled by MIC_NORMAL_REQUEST_SIZE.\n");
		}
		sMic->mic_buffer_previous_offset = 0;
	}

	return;
}


//----------------------------------------------------------------------------

static void usb_mic_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct usb_mic_interface *mic_interface = ep->driver_data;

	ASSERT(ep == mic_interface->source_ep);

	if (!mic_interface->ready) {
		return;
	}
	if (!mic_interface->alt) {
		return ;
	}
	switch (req->status) {
	case 0:
		if (req->actual == 0)
			sStat.complete_zero++;
		else
			sStat.complete++;
		break;

	case -ECONNRESET:	/* request dequeued */
	case -ESHUTDOWN:	/* disconnect from host */
	default:
		sStat.complete_error++;
		GADGET_LOG(LOG_ERR, "error request on '%s' %x\n", ep->name, req->status);
		break;
	}

	ASSERT(mic_interface->request_flg[(size_t)req->context] == MIC_REQUEST_QUEUED);
	mic_interface->request_flg[(size_t)req->context] = MIC_REQUEST_NONE;
	atomic_inc(&mic_interface->request_avails);
	sStat.request_avails = atomic_read(&mic_interface->request_avails);

	return;
}



static int usb_mic_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_mic_interface *mic_interface = (struct usb_mic_interface *)f;
	int id;
	int ret;

	ASSERT(c != NULL);
	ASSERT(mic_interface != NULL);

	id = usb_string_id(c->cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_MIC_IDX].id = id;
	as_interface_alt_0_desc.iInterface = id;
	as_interface_alt_1_desc.iInterface = id;

	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	as_interface_alt_0_desc.bInterfaceNumber = id;
	as_interface_alt_1_desc.bInterfaceNumber = id;

	/* Configure endpoint from the descriptor. */
	mic_interface->source_ep = usb_ep_autoconfig_ss(c->cdev->gadget, &ss_as_in_ep_desc, &ss_as_in_ep_comp_desc);
	if (mic_interface->source_ep == NULL) {
		GADGET_LOG(LOG_ERR, "can't autoconfigure mic endpoint\n");
		return -ENODEV;
	}

	ASSERT((mic_interface->source_ep->address & USB_ENDPOINT_NUMBER_MASK) == MIC_ENDPOINT_SOURCE);
	GADGET_LOG(LOG_DBG, "name=%s address=0x%x\n", mic_interface->source_ep->name,
		   mic_interface->source_ep->address);

	/* audio */
	mic_interface->mic_buffer_previous_offset = -1;
	mic_interface->audio_hundler = sieAudioGetHandle();
	ASSERT(mic_interface->audio_hundler != NULL);
	ret = sieAudioRegistUsbCallBack(mic_interface->audio_hundler, usb_mic_callback);
	if (ret) {
		GADGET_LOG(LOG_ERR, "sieAudioSetUsbCallBack Fail 0x%x\n",ret);
		return ret;
	}

	return 0;
}

static int usb_mic_enable(struct usb_function *f)
{
	struct usb_mic_interface *mic_interface = (struct usb_mic_interface *)f;
	int ret, i;

	ASSERT(mic_interface != NULL);

	GADGET_LOG(LOG_DBG, "enabling mic interface...\n");

	/* Enable the endpoint. */
	ret = config_ep_by_speed(f->config->cdev->gadget, f, mic_interface->source_ep);
	if (ret) {
		GADGET_LOG(LOG_ERR, "error configuring mic endpoint\n");
		return ret;
	}
	ret = usb_ep_enable(mic_interface->source_ep);
	if (ret) {
		GADGET_LOG(LOG_ERR, "error starting mic endpoint\n");
		return ret;
	}

	/* Data for the completion callback. */
	mic_interface->source_ep->driver_data = mic_interface;

	/* Prepare descriptors for TX. */
	for (i = 0; i < MIC_ENDPOINT_NUM_REQUESTS; i++) {
		struct usb_request *req;

		req = alloc_ep_req(mic_interface->source_ep, MIC_NORMAL_REQUEST_SIZE,
				   GFP_ATOMIC);
		if (req == NULL) {
			GADGET_LOG(LOG_ERR, "failed allocation usb request\n");
			disable_ep(mic_interface->source_ep);
			return -ENOMEM;
		}

		req->context = (void *)(uintptr_t)i;
		req->complete = usb_mic_complete;

		mic_interface->requests[i] = req;
		mic_interface->request_flg[i] = MIC_REQUEST_NONE;
		atomic_inc(&mic_interface->request_avails);
	}
	sStat.request_avails = atomic_read(&mic_interface->request_avails);

	mic_interface->ready = 1;
	return 0;
}

static void usb_mic_disable(struct usb_function *f)
{
	struct usb_mic_interface *mic_interface = (struct usb_mic_interface *)f;
	unsigned long flags;
	int i;

	ASSERT(mic_interface != NULL);

	GADGET_LOG(LOG_DBG, "disabling mic interface, ready:%d...\n", mic_interface->ready);

	spin_lock_irqsave(&mic_interface->lock, flags);
	if (mic_interface->source_ep->driver_data != NULL) {
		mic_interface->ready = 0;
		disable_ep(mic_interface->source_ep);

		for (i = 0; i < MIC_ENDPOINT_NUM_REQUESTS; i++) {
			mic_interface->request_flg[i] = MIC_REQUEST_QUEUED;
			free_ep_req(mic_interface->source_ep, mic_interface->requests[i]);
		}
		atomic_set(&mic_interface->request_avails,0);
		sStat.request_avails = atomic_read(&mic_interface->request_avails);
	}
	spin_unlock_irqrestore(&mic_interface->lock, flags);
}

static int usb_mic_reset(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct usb_mic_interface *mic_interface = (struct usb_mic_interface *)f;

	GADGET_LOG(LOG_DBG, "intf(%d) alt=%d\n", intf,alt);

	ASSERT(mic_interface != NULL);

	mic_interface->alt = alt;

	usb_mic_disable(f);
	return usb_mic_enable(f);
}

static int usb_mic_get_alt(struct usb_function *f, unsigned intf)
{
	struct usb_mic_interface *mic_interface = (struct usb_mic_interface *)f;
	ASSERT(mic_interface != NULL);

	GADGET_LOG(LOG_DBG, "intf(%d) alt =(%d)\n", intf, mic_interface->alt);
	return mic_interface->alt;
}

//----------------------------------------------------------------------------
int usb_mic_init(struct usb_function **interface)
{
	struct usb_mic_interface *mic_interface;

	ASSERT(interface != NULL);

	GADGET_LOG(LOG_DBG, "\n");

	sMic = mic_interface = kzalloc(sizeof(*mic_interface), GFP_KERNEL);
	if (sMic == NULL) {
		return -ENOMEM;
	}

	spin_lock_init(&mic_interface->lock);

	mic_interface->interface.name = "mic";
	mic_interface->interface.fs_descriptors = fs_mic_descs;
	mic_interface->interface.hs_descriptors = NULL;
	mic_interface->interface.ss_descriptors = ss_mic_descs;
	mic_interface->interface.bind = usb_mic_bind;
	mic_interface->interface.set_alt = usb_mic_reset;
	mic_interface->interface.disable = usb_mic_disable;
	mic_interface->interface.get_alt = usb_mic_get_alt;

	*interface = &mic_interface->interface;
	return 0;
}

void usb_mic_destroy(void)
{
	ASSERT(sMic != NULL);
	kfree(sMic);
	sMic = NULL;
}
