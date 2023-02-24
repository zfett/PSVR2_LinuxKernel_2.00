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

#ifndef _SIE_UTIL_H_
#define _SIE_UTIL_H_

struct usb_request *alloc_ep_req(struct usb_ep *ep, size_t buflen, gfp_t gfp_flags);
void free_ep_req(struct usb_ep *ep, struct usb_request *req);
void disable_ep(struct usb_ep *ep);

#endif
