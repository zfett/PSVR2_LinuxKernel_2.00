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

#ifndef _SIE_STAT_H_
#define _SIE_STAT_H_

struct usb_mic_stat
{
	uint32_t sent;
	uint32_t send_error_inval;
	uint32_t send_error_nodev;
	uint32_t send_error_again;
	uint32_t send_error_noent;
	uint32_t send_error_others;
	uint32_t large;
	uint32_t merged;
	uint32_t complete;
	uint32_t complete_zero;
	uint32_t complete_error;
	uint32_t request_avails;
};

void usb_mic_get_stat(struct usb_mic_stat* stat);

#endif
