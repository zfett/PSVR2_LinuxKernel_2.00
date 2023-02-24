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

#ifndef _SIE_USB_H_
#define _SIE_USB_H_

#include <linux/usb/gadget.h>

#include "util.h"
//----------------------------------------------------------------------------
/* Compiler switch flags */
//#define ENABLE_SIEUSB_DEBUG

//----------------------------------------------------------------------------

/* Definitions for the device descriptors. */
#define VENDOR_ID		0x054c
#define PRODUCT_ID		0x0cde

#define BCDDEVICE		0x0110

// string descriptor
#define MANUFACTURER		"Sony"
// (R) mark U+00AE in UTF-8 is 0xc2 0xae
#define PRODUCT			"PlayStation\xc2\xaeVR2"

// AUTHENTICATION_INTERFACE #0
#define AUTHENTICATION_INTERFACE_CLASS	0x03
#define AUTHENTICATION_INTERFACE_NAME	"PS VR2 Control"
#define AUTHENTICATION_ENDPOINT_SINK	0x01
#define AUTHENTICATION_ENDPOINT_SOURCE	0x01

// MICCONTROL_INTERFACE #1
#define AUDIO_CONTROL_INTERFACE_CLASS	0x01
#define AUDIO_CONTROL_INTERFACE_NAME	"PS VR2 Audio"
#define AUDIO_CONTROL_INTERFACE_SOURCE	0

// MIC_INTERFACE #2
#define MIC_INTERFACE_CLASS			0x01
#define MIC_INTERFACE_NAME			"PS VR2 Audio Mic"
#define MIC_ENDPOINT_SOURCE			0x02

// DATA1_INTERFACE #3
#define DATA1_INTERFACE_NAME		"PS VR2 Data 1"
#define DATA1_ENDPOINT_SOURCE		0x03

// DATA2_INTERFACE #4
#define DATA2_INTERFACE_NAME		"PS VR2 Data 2"
#define DATA2_ENDPOINT_SINK			0x04
#define DATA2_ENDPOINT_SOURCE		0x04

// DATA3_INTERFACE #5
#define DATA3_INTERFACE_NAME		"PS VR2 Data 3"
#define DATA3_ENDPOINT_SINK			0x05
#define DATA3_ENDPOINT_SOURCE		0x05
#define INTR_ENDPOINT_SOURCE		0x06

// DATA4_INTERFACE #6
#define DATA4_INTERFACE_NAME		"PS VR2 Data 4"
#define DATA4_ENDPOINT_SOURCE		0x07

// STATUS_INTERFACE #7
#define STATUS_INTERFACE_NAME		"PS VR2 Status"
#define STATUS_ENDPOINT_SOURCE		0x08

// DATA5_INTERFACE #8
#define DATA5_INTERFACE_NAME		"PS VR2 Data 5"
#define DATA5_ENDPOINT_SOURCE		0x09

// DATA6_INTERFACE #9
#define DATA6_INTERFACE_NAME		"PS VR2 Data 6"
#define DATA6_ENDPOINT_SOURCE		0x0A

// DATA7_INTERFACE #10
#define DATA7_INTERFACE_NAME		"PS VR2 Data 7"
#define DATA7_ENDPOINT_SINK			0x02
#define DATA7_ENDPOINT_SOURCE		0x0B

// DATA8_INTERFACE #11
#define DATA8_INTERFACE_NAME		"PS VR2 Data 8"
#define DATA8_ENDPOINT_SOURCE		0x0C

// DATA9_INTERFACE #12
#define DATA9_INTERFACE_NAME		"PS VR2 Data 9"
#define DATA9_ENDPOINT_SOURCE		0x0D

// Vendor Specific subclass
#define USB_SUBCLASS_DATA1		0x01
#define USB_SUBCLASS_DATA2		0x02
#define USB_SUBCLASS_DATA3		0x03
#define USB_SUBCLASS_DATA4		0x04
#define USB_SUBCLASS_STATUS		0x05
#define USB_SUBCLASS_DATA5		0x06
#define USB_SUBCLASS_DATA6		0x07
#define USB_SUBCLASS_DATA7		0x08
#define USB_SUBCLASS_DATA8		0x09
#define USB_SUBCLASS_DATA9		0xDB

//----------------------------------------------------------------------------

/* String indexes. */
#define STRING_MANUFACTURER_IDX		0
#define STRING_PRODUCT_IDX			1
#define STRING_AUTHENTICATION_IDX	2
#define STRING_AUDIO_CONTROL_IDX	3
#define STRING_MIC_IDX				4
#define STRING_DATA1_IDX			5
#define STRING_DATA2_IDX			6
#define STRING_DATA3_IDX			7
#define STRING_DATA4_IDX			8
#define STRING_STATUS_IDX			9
#define STRING_DATA5_IDX			10
#define STRING_DATA6_IDX			11
#define STRING_DATA7_IDX			12
#define STRING_DATA8_IDX			13
#define STRING_DATA9_IDX			14

//----------------------------------------------------------------------------

/* Utility macros. */

#define ASSERT_ON_COMPILE(condition) switch(0){case 0:case condition:;}
#define ASSERT(expression)						\
	if (unlikely(!(expression))) {					\
		pr_err("Assertion failed! %s,%s,%s:%d\n", #expression,	\
		       __FILE__, __FUNCTION__, __LINE__);		\
	}

/* Logging. */

#define LOG_LEVEL		LOG_DBG
#define LOG_ERR			0
#define LOG_WARN		1
#define LOG_INFO		2
#define LOG_DBG			3

#if defined(ENABLE_SIEUSB_DEBUG)
#define GADGET_LOG(LEVEL, FORMAT, ...)					\
	if ((LEVEL) <= LOG_LEVEL)					\
	{								\
		const char* file=__FILE__;				\
		if (strlen(file) > 20)					\
			file = file+strlen(file) - 20;			\
		printk("%s:%d (%s) -"FORMAT,  file, __LINE__, __FUNCTION__,##__VA_ARGS__); \
	}
#else
#define GADGET_LOG(LEVEL, FORMAT, ...) do {} while(0)
#endif

/* Exported. */

extern struct usb_string strings_dev[];

#endif
