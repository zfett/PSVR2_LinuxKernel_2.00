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

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "usb.h"

#include "authentication.h"
#include "audio_ctrl.h"
#include "command.h"
#include "mic.h"
#include "data.h"
#include "status.h"
#include "bulkintr.h"

#if IS_ENABLED(CONFIG_USB_MASS_STORAGE)
#include "mass_storage.h"
#endif

struct usb_string strings_dev[] = {
	[STRING_MANUFACTURER_IDX].s 	= MANUFACTURER,
	[STRING_PRODUCT_IDX].s 		= PRODUCT,
	[STRING_AUTHENTICATION_IDX].s 	= AUTHENTICATION_INTERFACE_NAME,
	[STRING_AUDIO_CONTROL_IDX].s 	= AUDIO_CONTROL_INTERFACE_NAME,
	[STRING_MIC_IDX].s 		= MIC_INTERFACE_NAME,
	[STRING_DATA1_IDX].s 		= DATA1_INTERFACE_NAME,
	[STRING_DATA2_IDX].s 		= DATA2_INTERFACE_NAME,
	[STRING_DATA3_IDX].s 		= DATA3_INTERFACE_NAME,
	[STRING_DATA4_IDX].s 		= DATA4_INTERFACE_NAME,
	[STRING_STATUS_IDX].s 		= STATUS_INTERFACE_NAME,
	[STRING_DATA5_IDX].s 		= DATA5_INTERFACE_NAME,
	[STRING_DATA6_IDX].s 		= DATA6_INTERFACE_NAME,
	[STRING_DATA7_IDX].s 		= DATA7_INTERFACE_NAME,
	[STRING_DATA8_IDX].s 		= DATA8_INTERFACE_NAME,
	[STRING_DATA9_IDX].s 		= DATA9_INTERFACE_NAME,
	{ }
};

static struct usb_gadget_strings stringtab_dev = {
	.language    = 0x0409, /* en-us */
	.strings     = strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
	&stringtab_dev,
	NULL,
};

static struct usb_device_descriptor device_desc = {
	.bLength 			= sizeof (device_desc),
	.bDescriptorType 	= USB_DT_DEVICE,
	.bcdUSB 			= cpu_to_le16(0x0320),
	.bDeviceClass 		= USB_CLASS_PER_INTERFACE,
	.idVendor 			= cpu_to_le16(VENDOR_ID),
	.idProduct 			= cpu_to_le16(PRODUCT_ID),
	// bcdDevice is ignored by composite framework.
	// See update_unchanged_dev_desc()
	//.bcdDevice           = cpu_to_le16(BCDDEVICE),
	.bNumConfigurations = 1,
};

static const struct usb_composite_overwrite covr = {
	.bcdDevice = cpu_to_le16(BCDDEVICE),
};


static struct usb_configuration sieusb_config = {
	.label 					= "Default Configuration",
	.strings 				= dev_strings,
	.setup 					= usb_cmd_setup,
	.bConfigurationValue 	= 1,
	.bmAttributes 			= USB_CONFIG_ATT_ONE,
	.MaxPower 				= 896,
};

static struct class *s_usb_class = NULL;

static int interfaces_init(struct usb_configuration *c)
{
	int i, ret;
	struct usb_function *interfaces[MAX_CONFIG_INTERFACES];
	int num_interfaces;

	GADGET_LOG(LOG_DBG, "initializing interfaces...\n");

	/*
		.bDescriptorType =	USB_DT_INTERFACE_ASSOCIATION
		.bInterfaceCount =	4,
		.bFunctionClass =		USB_CLASS_PER_INTERFACE,
		.bFunctionSubClass =	,
		.bFunctionProtocol =	,
	*/

	usb_cmd_init(s_usb_class);

	memset(interfaces, 0, sizeof (interfaces));
	num_interfaces = 0;

	ret = usb_auth_init(s_usb_class, &interfaces[num_interfaces]);
	if (ret) {
		GADGET_LOG(LOG_ERR, "failed to initialize Authentication interface: %d\n", ret);
		goto fail_auth_init;
	}
	num_interfaces++;

	ret = usb_audio_ctrl_init(&interfaces[num_interfaces]);
	if (ret) {
		GADGET_LOG(LOG_ERR, "failed to initialize micctrl interface: %d\n", ret);
		goto fail_audio_ctrl_init;
	}
	num_interfaces++;

	ret = usb_mic_init(&interfaces[num_interfaces]);
	if (ret) {
		GADGET_LOG(LOG_ERR, "failed to initialize mic interface: %d\n", ret);
		goto fail_mic_init;
	}
	num_interfaces++;

	ret = usb_data_init(s_usb_class, 0, &interfaces[num_interfaces]);
	if (ret) {
		GADGET_LOG(LOG_ERR, "failed to initialize data1 interface: %d\n", ret);
		goto fail_data1_init;
	}
	num_interfaces++;

	ret = usb_data_init(s_usb_class, 1, &interfaces[num_interfaces]);
	if (ret) {
		GADGET_LOG(LOG_ERR, "failed to initialize data2 interface: %d\n", ret);
		goto fail_data2_init;
	}
	num_interfaces++;

	ret = usb_bulkintr_init(s_usb_class, &interfaces[num_interfaces]);
	if (ret) {
		GADGET_LOG(LOG_ERR, "failed to initialize data3 interface: %d\n", ret);
		goto fail_data3_init;
	}
	num_interfaces++;

	ret = usb_data_init(s_usb_class, 3, &interfaces[num_interfaces]);
	if (ret) {
		GADGET_LOG(LOG_ERR, "failed to initialize data4 interface: %d\n", ret);
		goto fail_data4_init;
	}
	num_interfaces++;

	ret = usb_status_init(s_usb_class, &interfaces[num_interfaces]);
	if (ret) {
		GADGET_LOG(LOG_ERR, "failed to initialize status interface: %d\n", ret);
		goto fail_status_init;
	}
	num_interfaces++;

	ret = usb_data_init(s_usb_class, 4, &interfaces[num_interfaces]);
	if (ret) {
		GADGET_LOG(LOG_ERR, "failed to initialize data5 interface: %d\n", ret);
		goto fail_data5_init;
	}
	num_interfaces++;

	ret = usb_data_init(s_usb_class, 5, &interfaces[num_interfaces]);
	if (ret) {
		GADGET_LOG(LOG_ERR, "failed to initialize data6 interface: %d\n", ret);
		goto fail_data6_init;
	}
	num_interfaces++;

	ret = usb_data_init(s_usb_class, 6, &interfaces[num_interfaces]);
	if (ret) {
		GADGET_LOG(LOG_ERR, "failed to initialize data7 interface: %d\n", ret);
		goto fail_data7_init;
	}
	num_interfaces++;

	ret = usb_data_init(s_usb_class, 7, &interfaces[num_interfaces]);
	if (ret) {
		GADGET_LOG(LOG_ERR, "failed to initialize data8 interface: %d\n", ret);
		goto fail_data8_init;
	}
	num_interfaces++;

	ret = usb_data_init(s_usb_class, 8, &interfaces[num_interfaces]);
	if (ret) {
		GADGET_LOG(LOG_ERR, "failed to initialize data9 interface: %d\n", ret);
		goto fail_data9_init;
	}
	num_interfaces++;

#if IS_ENABLED(CONFIG_USB_MASS_STORAGE)
	ret = usb_msc_init(c->cdev, &interfaces[num_interfaces]);
	if (ret) {
		GADGET_LOG(LOG_ERR, "failed to initialize mass storage interface: %d\n", ret);
		goto fail_msc_init;
	}
	num_interfaces++;
#endif

	/* Register the interfaces. */
	for (i = 0; i < num_interfaces; i++) {
		ret = usb_add_function(c, interfaces[i]);
		if (ret) {
			GADGET_LOG(LOG_ERR, "unable to add interface: %d\n", ret);

			while (i > 0) {
				i--;
				usb_remove_function(c, interfaces[i]);
			}

			goto fail;
		}
	}

	return 0;

fail:
#if IS_ENABLED(CONFIG_USB_MASS_STORAGE)
fail_msc_init:
	usb_msc_destroy();
#endif
fail_data9_init:
	usb_data_destroy(8);
fail_data8_init:
	usb_data_destroy(7);
fail_data7_init:
	usb_data_destroy(6);
fail_data6_init:
	usb_data_destroy(5);
fail_data5_init:
	usb_data_destroy(4);
fail_status_init:
	usb_status_destroy();
fail_data4_init:
	usb_data_destroy(3);
fail_data3_init:
	usb_bulkintr_destroy();
fail_data2_init:
	usb_data_destroy(1);
fail_data1_init:
	usb_data_destroy(0);
fail_mic_init:
	usb_mic_destroy();
fail_audio_ctrl_init:
	usb_audio_ctrl_destroy();
fail_auth_init:
	usb_auth_destroy();

	return ret;
}

//----------------------------------------------------------------------------
static int usb_bind_config(struct usb_configuration *c)
{
	int ret;

	GADGET_LOG(LOG_DBG, "usb_bind_config\n");

	ret = interfaces_init(c);

	return ret;
}

static void usb_unbind_config(struct usb_configuration *c)
{
	GADGET_LOG(LOG_DBG, "usb_unbind_config\n");

#if IS_ENABLED(CONFIG_USB_MASS_STORAGE)
	usb_msc_destroy();
#endif
	usb_data_destroy(8);
	usb_data_destroy(7);
	usb_data_destroy(6);
	usb_data_destroy(5);
	usb_data_destroy(4);
	usb_status_destroy();
	usb_data_destroy(3);
	usb_bulkintr_destroy();
	usb_data_destroy(1);
	usb_data_destroy(0);
	usb_mic_destroy();
	usb_audio_ctrl_destroy();
	usb_auth_destroy();
	usb_cmd_destroy();

	return;
}

static int usb_bind(struct usb_composite_dev *cdev)
{
	struct usb_gadget *gadget = cdev->gadget;
	int id, ret;

	GADGET_LOG(LOG_DBG, "bind initializing gadget...name=%s\n", gadget->name);

	/* Check for High-Speed support. */
	if (!gadget_is_dualspeed(gadget)) {
		GADGET_LOG(LOG_ERR, "gadget does not support high-speed\n");
		return -ENODEV;
	}

	/* Allocate strings. */
	id = usb_string_id(cdev);
	if (id < 0) {
		return id;
	}

	strings_dev[STRING_MANUFACTURER_IDX].id = id;
	device_desc.iManufacturer = id;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;

	strings_dev[STRING_PRODUCT_IDX].id = id;
	device_desc.iProduct = id;

	sieusb_config.unbind = usb_unbind_config;

	/* With unknown reason, this is required in order to set bcdDevice */
	usb_composite_overwrite_options(cdev,
					(struct usb_composite_overwrite *)&covr);

	/* Add the default configuration. */
	ret = usb_add_config(cdev, &sieusb_config, usb_bind_config);
	if (ret) {
		GADGET_LOG(LOG_ERR, "unable to add configuration: %d\n", ret);
		return ret;
	}

	return 0;
}

static int usb_unbind(struct usb_composite_dev *cdev)
{
	GADGET_LOG(LOG_DBG, "unbind destroying gadget...\n");

	return 0;
}

//----------------------------------------------------------------------------
static struct usb_composite_driver usb_driver = {
	.name        = "sieusb",
	.dev         = &device_desc,
	.strings     = dev_strings,
	.max_speed   = USB_SPEED_SUPER,
	.bind        = usb_bind,
	.unbind      = usb_unbind,
};


//----------------------------------------------------------------------------

static int __init usb_init(void)
{
	int ret;

	/* create parent class of each interfaces */
	s_usb_class = class_create(THIS_MODULE, "sieusb");
	if (IS_ERR(s_usb_class)) {
		printk(KERN_ERR "%s() failed to create class\n", __FUNCTION__);
		return PTR_ERR(s_usb_class);
	}

	// compsite probe
	ret = usb_composite_probe(&usb_driver);
	if (ret) {
		GADGET_LOG(LOG_ERR, "Unable to register composite device, error:%d\n", ret);
		class_destroy(s_usb_class);
		s_usb_class = NULL;
	}

	return ret;
}

static void __exit usb_cleanup(void)
{
	// composite unreg
	usb_composite_unregister(&usb_driver);

	if (s_usb_class) {
		class_destroy(s_usb_class);
		s_usb_class = NULL;
	}
	return;
}

module_init(usb_init);
module_exit(usb_cleanup);

MODULE_AUTHOR("Sony Interactive Entertainment Inc.");
MODULE_DESCRIPTION("sieusb driver");
MODULE_LICENSE("GPL");
