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

#ifndef _SIEUSB_H_
#define _SIEUSB_H_

#include "sce_km_defs.h"

/* Don't change this order. */
/* Updater Gadget uses values of when it was built. */
typedef enum SIEUSB_CONTORL_EVENT {

	// PS4 command with Control Transfer
	HOST_IOCTL_GET_CTRL_COMMAND_REQ = 100,
	HOST_IOCTL_SET_CTRL_COMMAND_RESULT,
	HOST_IOCTL_SET_CTRL_REPORT_DATA,
	HOST_IOCTL_GET_CTRL_REPORT_DATA,

	// Host USB Connection
	HOST_IOCTL_QUIESCENT_EP = 200,
	HOST_IOCTL_RESERVED2,
	HOST_IOCTL_RESERVED3,
	HOST_IOCTL_RESERVED4,

	// Device Authentication
	HOST_IOCTL_GET_AUTH_STATE = 700,
	HOST_IOCTL_GET_H_CHALLENGE_DATA_1,
	HOST_IOCTL_GET_H_CHALLENGE_DATA_2,
	HOST_IOCTL_SET_D_RESPONSE_CHALLENGE_DATA_1,
	HOST_IOCTL_SET_D_RESPONSE_CHALLENGE_DATA_2,
	HOST_IOCTL_GET_H_RESPONSE_DATA,
	HOST_IOCTL_SET_AUTH_OK,
	HOST_IOCTL_SET_AUTH_ERROR,

} _SIEUSB_CONTORL_EVENT;

#define AUTH_SUCCESS                    0
#define AUTH_ERROR_NOT_INITIALIZED      0x80010001
#define AUTH_ERROR_INVALID_ARGS         0x80020001
#define AUTH_ERROR_INVALID_REQUEST      0x80020002
#define AUTH_ERROR_INVALID_BLOCK_NUMBER 0x80030001
#define AUTH_ERROR_NOT_CREATED          0x80030002
#define AUTH_ERROR_NOT_SUPPORTED        0x80030003
#define AUTH_ERROR_NO_DATA              0x80030004

#define AUTH_STATUS_INITIAL                     0x01
#define AUTH_STATUS_WAIT_H_1ST_CHALLENGE        0x10
#define AUTH_STATUS_EXECUTING_H_1ST_CHALLENG    0x11
#define AUTH_STATUS_READY_D_1ST_RESPONSE        0x12
#define AUTH_STATUS_WAIT_H_RESPONSE             0x20
#define AUTH_STATUS_VERIFYING_H_RESPONSE        0x21
#define AUTH_STATUS_AUTHENTICATE_OK             0x40
#define AUTH_STATUS_EXECUTING_H_CHALLENG        0x51
#define AUTH_STATUS_READY_D_RESPONSE            0x52
#define AUTH_STATUS_ERROR_AUTHENTICATE          0x80

#endif
