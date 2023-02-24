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

#include <linux/usb/gadget.h>

struct usb_request *alloc_ep_req(struct usb_ep *ep, size_t buflen, gfp_t gfp_flags)
{
	struct usb_request *req;

	req = usb_ep_alloc_request(ep, gfp_flags);
	if (req != NULL) {
		req->length = buflen;
	}

	return req;
}

void free_ep_req(struct usb_ep *ep, struct usb_request *req)
{
	usb_ep_free_request(ep, req);
	return;
}

void disable_ep(struct usb_ep *ep)
{
	int value;

	if (ep->driver_data != NULL) {
		value = usb_ep_disable(ep);
		if (value < 0)
			pr_err("error disabling endpoint '%s'\n", ep->name);

		ep->driver_data = NULL;
	}
	return;
}

