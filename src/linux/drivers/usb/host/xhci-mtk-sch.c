/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author:
 *  Zhigang.Wei <zhigang.wei@mediatek.com>
 *  Chunfeng.Yun <chunfeng.yun@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/**
 * @file xhci-mtk-sch.c
 * MediaTek xHCI Host Controller Software scheduler driver.
 */

/**
 * @defgroup IP_group_usbhost USB_HOST
 *   The dirver use for MTK USB Host.
 *
 *   @{
 *       @defgroup IP_group_usbhost_external EXTERNAL
 *         The external API document for MTK USB Host. \n
 *
 *         @{
 *           @defgroup IP_group_usbhost_external_function 1.function
 *               none. No extra function in MTK USB Host driver.
 *           @defgroup IP_group_usbhost_external_struct 2.Structure
 *               none. No extra Structure in MTK USB Host driver.
 *           @defgroup IP_group_usbhost_external_typedef 3.Typedef
 *               none. No extra Typedef in MTK USB Host driver.
 *           @defgroup IP_group_usbhost_external_enum 4.Enumeration
 *               none. No extra Enumeration in MTK USB Host driver.
 *           @defgroup IP_group_usbhost_external_def 5.Define
 *               none. No extra Define in MTK USB Host driver.
 *         @}
 *
 *       @defgroup IP_group_usbhost_internal INTERNAL
 *         The internal API document for MTK USB Host. \n
 *
 *         @{
 *           @defgroup IP_group_usbhost_internal_function 1.function
 *               Internal function for MTK USB Host initial.
 *           @defgroup IP_group_usbhost_internal_struct 2.Structure
 *               Internal structure used for MTK USB Host driver.
 *           @defgroup IP_group_usbhost_internal_typedef 3.Typedef
 *               none. No extra Typedef in MTK USB Host driver.
 *           @defgroup IP_group_usbhost_internal_enum 4.Enumeration
 *               Internal enumeration used for MTK USB Host driver.
 *           @defgroup IP_group_usbhost_internal_def 5.Define
 *               Internal define used for MTK USB Host driver.
 *         @}
 *    @}
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "xhci.h"
#include "xhci-mtk.h"

/** @ingroup IP_group_usbhost_internal_def
 * @brief use for mtk HCD Software scheduler driver define.
 * @{
 */
#define SS_BW_BOUNDARY	51000
/** table 5-5. High-speed Isoc Transaction Limits in usb_20 spec */
#define HS_BW_BOUNDARY	6144
/** usb2 spec section11.18.1: at most 188 FS bytes per microframe */
#define FS_PAYLOAD_MAX 188
/** max number of microframes for split transfer,
 * for fs isoc in : 1 ss + 1 idle + 8 cs
 */
#define TT_MICROFRAMES_MAX 10
/** mtk scheduler bitmasks */
#define EP_BPKTS(p)	((p) & 0x7f)
#define EP_BCSCOUNT(p)	(((p) & 0x7) << 8)
#define EP_BBM(p)	((p) << 11)
#define EP_BOFFSET(p)	((p) & 0x3fff)
#define EP_BREPEAT(p)	(((p) & 0x7fff) << 16)
/**
 * @}
 */

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     judge usb device is FULL or LOW speed.
 * @param[in]
 *     speed: enum type usb_device_speed.
 * @return
 *     1, If spped is FULL or LOW.\n
 *     otherwise return value is '0'.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int is_fs_or_ls(enum usb_device_speed speed)
{
	return speed == USB_SPEED_FULL || speed == USB_SPEED_LOW;
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 * get the index of bandwidth domains array which @ep belongs to.\n
 * the bandwidth domain array is saved to @sch_array of struct xhci_hcd_mtk,\n
 * each HS root port is treated as a single bandwidth domain,\n
 * but each SS root port is treated as two bandwidth domains,\n
 * one for IN eps,one for OUT eps.\n
 * @real_port value is defined as follow according to xHCI spec:\n
 * 1 for SSport0, ..., N+1 for SSportN, N+2 for HSport0, N+3 for HSport1,etc\n
 * so the bandwidth domain array is organized as follow for simplification:\n
 * SSport0-OUT, SSport0-IN,..., SSportX-OUT, SSportX-IN, HSport0,..., HSportY.
 * @param[in]
 *     xhci: xhci_hcd struct pointer.\n
 * @param[in]
 *     udev: usb_device struct pointer.\n
 * @param[in]
 *     ep: usb_host_endpoint struct pointer.\n
 * @return
 *     return which bw_index value.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int get_bw_index(struct xhci_hcd *xhci, struct usb_device *udev,
	struct usb_host_endpoint *ep)
{
	struct xhci_virt_device *virt_dev;
	int bw_index;

	virt_dev = xhci->devs[udev->slot_id];

	if (udev->speed == USB_SPEED_SUPER) {
		if (usb_endpoint_dir_out(&ep->desc))
			bw_index = (virt_dev->real_port - 1) * 2;
		else
			bw_index = (virt_dev->real_port - 1) * 2 + 1;
	} else {
		/* add one more for each SS port */
		bw_index = virt_dev->real_port + xhci->num_usb3_ports - 1;
	}

	return bw_index;
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 * setup scheduler parameters for @ep.\n
 * @param[in]
 *     udev: usb_device struct pointer.\n
 * @param[in]
 *     ep_ctx: xhci_ep_ctx struct pointer.\n
 * @param[in]
 *     sch_ep: mu3h_sch_ep_info struct pointer.\n
 * @return
 *     0, function success.\n
 *     -ENOMEM, ep_bw_budget_table alloc fail.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If ep_bw_budget_table alloc fail ,return -ENOMEM.\n
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int setup_sch_info(struct usb_device *udev,
		struct xhci_ep_ctx *ep_ctx, struct mu3h_sch_ep_info *sch_ep)
{
	u32 ep_type;
	u32 ep_interval;
	u32 max_packet_size;
	u32 max_burst;
	u32 max_esit_payload;
	u32 esit_pkts;
	u32 *ep_bw_budget_table = NULL;
	int i;

	ep_type = CTX_TO_EP_TYPE(le32_to_cpu(ep_ctx->ep_info2));
	ep_interval = CTX_TO_EP_INTERVAL(le32_to_cpu(ep_ctx->ep_info));
	max_packet_size = MAX_PACKET_DECODED(le32_to_cpu(ep_ctx->ep_info2));
	max_burst = CTX_TO_MAX_BURST(le32_to_cpu(ep_ctx->ep_info2));
	max_esit_payload =
		CTX_TO_MAX_ESIT_PAYLOAD_HI(le32_to_cpu(ep_ctx->ep_info) << 16) |
		CTX_TO_MAX_ESIT_PAYLOAD(le32_to_cpu(ep_ctx->tx_info));

	sch_ep->esit = 1 << ep_interval;
	sch_ep->offset = 0;
	sch_ep->burst_mode = 0;

	if (udev->speed == USB_SPEED_HIGH) {
		sch_ep->cs_count = 0;

		/*
		 * usb_20 spec section5.9
		 * a single microframe is enough for HS synchromous endpoints
		 * in a interval
		 */
		sch_ep->num_budget_microframes = 1;
		sch_ep->repeat = 0;

		/*
		 * xHCI spec section6.2.3.4
		 * @max_burst is the number of additional transactions
		 * opportunities per microframe
		 */
		sch_ep->pkts = max_burst + 1;
		sch_ep->bw_cost_per_microframe = max_packet_size * sch_ep->pkts;
		ep_bw_budget_table = kzalloc(sizeof(u32), GFP_NOIO);
		if (!ep_bw_budget_table)
			return -ENOMEM;
		ep_bw_budget_table[0] = sch_ep->bw_cost_per_microframe;
	} else if (udev->speed == USB_SPEED_SUPER) {
		/* usb3_r1 spec section4.4.7 & 4.4.8 */
		sch_ep->cs_count = 0;
		sch_ep->burst_mode = 1;
		esit_pkts = max_esit_payload / max_packet_size;
		if (esit_pkts == 0)
			esit_pkts = 1;
		if (ep_type == INT_IN_EP || ep_type == INT_OUT_EP) {
			sch_ep->pkts = esit_pkts;
			sch_ep->num_budget_microframes = 1;
			sch_ep->repeat = 0;
			ep_bw_budget_table = kzalloc(sizeof(u32), GFP_NOIO);
			if (!ep_bw_budget_table)
				return -ENOMEM;
			ep_bw_budget_table[0] = max_packet_size * sch_ep->pkts;
		}

		if (ep_type == ISOC_IN_EP || ep_type == ISOC_OUT_EP) {
			if (esit_pkts <= sch_ep->esit)
				sch_ep->pkts = 1;
			else if (sch_ep->esit == 0)
				sch_ep->pkts = roundup_pow_of_two(esit_pkts);
			else
				sch_ep->pkts = roundup_pow_of_two(esit_pkts)
					/ sch_ep->esit;

			sch_ep->num_budget_microframes =
				DIV_ROUND_UP(esit_pkts, sch_ep->pkts);

			if (sch_ep->num_budget_microframes > 1)
				sch_ep->repeat = 1;
			else
				sch_ep->repeat = 0;

			sch_ep->bw_cost_per_microframe =
				max_packet_size * sch_ep->pkts;

			ep_bw_budget_table =
				kzalloc(sch_ep->num_budget_microframes
				* sizeof(u32), GFP_NOIO);

			if (!ep_bw_budget_table)
				return -ENOMEM;

			for (i = 0; i < sch_ep->num_budget_microframes; i++) {
				if (i != sch_ep->num_budget_microframes - 1)
					ep_bw_budget_table[i] =
					sch_ep->bw_cost_per_microframe;
				else
					ep_bw_budget_table[i] =
					sch_ep->bw_cost_per_microframe
					* sch_ep->num_budget_microframes
					- max_packet_size * esit_pkts;
			}
		}
	} else if (is_fs_or_ls(udev->speed)) {
		sch_ep->repeat = 0;
		sch_ep->pkts = 1;/* at most one packet for each microframe */

		sch_ep->bw_cost_per_microframe =
			(max_packet_size < FS_PAYLOAD_MAX)
			? max_packet_size : FS_PAYLOAD_MAX;

		ep_bw_budget_table = kzalloc(TT_MICROFRAMES_MAX
					* sizeof(u32), GFP_NOIO);

		if (!ep_bw_budget_table)
			return -ENOMEM;
	}

	sch_ep->bw_budget_table = ep_bw_budget_table;

	return 0;
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 * Get maximum bandwidth when we schedule at offset slot.\n
 * @param[in]
 *     sch_bw: mu3h_sch_bw_info struct pointer.\n
 * @param[in]
 *     sch_ep: mu3h_sch_ep_info struct pointer.\n
 * @param[in]
 *     offset: offset value of slot.\n
 * @return
 *     return bandwidth value(u32).
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static u32 get_max_bw(struct mu3h_sch_bw_info *sch_bw,
	struct mu3h_sch_ep_info *sch_ep, u32 offset)
{
	u32 num_esit;
	u32 max_bw = 0;
	u32 uframe_bw;
	int i;
	int j;

	num_esit = XHCI_MTK_MAX_ESIT / sch_ep->esit;
	for (i = 0; i < num_esit; i++) {
		u32 base = offset + i * sch_ep->esit;

		for (j = 0; j < sch_ep->num_budget_microframes; j++) {
			uframe_bw = sch_bw->bus_bw[base + j]
				+ sch_ep->bw_budget_table[j];

			if (uframe_bw > max_bw)
				max_bw = uframe_bw;
		}
	}
	return max_bw;
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 * update SW scheduler bandwidth information.\n
 * @param[in]
 *     sch_bw: mu3h_sch_bw_info struct pointer.\n
 * @param[in]
 *     sch_ep: mu3h_sch_ep_info struct pointer.\n
 * @param[in]
 *     ep_operation: ep operation case.\n
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void update_bus_bw(struct mu3h_sch_bw_info *sch_bw,
	struct mu3h_sch_ep_info *sch_ep,
	enum mu3h_sch_ep_operation ep_operation)
{
	u32 num_esit;
	u32 base;
	int i;
	int j;

	num_esit = XHCI_MTK_MAX_ESIT / sch_ep->esit;
	for (i = 0; i < num_esit; i++) {
		base = sch_ep->offset + i * sch_ep->esit;
		for (j = 0; j < sch_ep->num_budget_microframes; j++) {
			if (ep_operation == ADD_EP)
				sch_bw->bus_bw[base + j] +=
					sch_ep->bw_budget_table[j];
			else
				sch_bw->bus_bw[base + j] -=
					sch_ep->bw_budget_table[j];
		}
	}
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 * Find and create our mu3h_sch_tt data structure.
 * @param[in]
 *     udev: usb_device struct pointer.\n
 * @return
 *     struct pointer, function success return struct mu3h_sch_tt pointer.\n
 *     -ENOMEM, memory alloc fail.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If memory alloc fail ,return -ENOMEM.\n
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static struct mu3h_sch_tt *find_tt(struct usb_device *udev)
{
	struct usb_tt *utt = udev->tt;
	struct mu3h_sch_tt *tt, **tt_index, **ptt;
	unsigned port;
	bool allocated_index = false;

	if (!utt)
		return NULL;/* Not below a TT */

	/*
	 * Find/create our data structure.
	 * For hubs with a single TT, we get it directly.
	 * For hubs with multiple TTs, there's an extra level of pointers.
	 */
	tt_index = NULL;
	if (utt->multi) {
		tt_index = utt->hcpriv;
		if (!tt_index) {		/* Create the index array */
			tt_index = kzalloc(utt->hub->maxchild *
					sizeof(*tt_index), GFP_ATOMIC);
			if (!tt_index)
				return ERR_PTR(-ENOMEM);
			utt->hcpriv = tt_index;
			allocated_index = true;
		}
		port = udev->ttport - 1;
		ptt = &tt_index[port];
	} else {
		port = 0;
		ptt = (struct mu3h_sch_tt **) &utt->hcpriv;
	}

	tt = *ptt;
	if (!tt) {/* Create the mu3h_sch_tt */
		tt = kzalloc(sizeof(*tt), GFP_ATOMIC);
		if (!tt) {
			if (allocated_index) {
				utt->hcpriv = NULL;
				kfree(tt_index);
			}
			return ERR_PTR(-ENOMEM);
		}
		INIT_LIST_HEAD(&tt->ep_list);
		tt->usb_tt = utt;
		tt->tt_port = port;
		*ptt = tt;
	}

	return tt;
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 * Release the TT above udev, if it's not in use.
 * @param[in]
 *     udev: usb_device struct pointer.\n
 * @return
*     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void drop_tt(struct usb_device *udev)
{
	struct usb_tt		*utt = udev->tt;
	struct mu3h_sch_tt		*tt, **tt_index, **ptt;
	int			i, cnt;

	if (!utt || !utt->hcpriv)
		return;		/* Not below a TT, or never allocated */

	cnt = 0;
	if (utt->multi) {
		tt_index = utt->hcpriv;
		ptt = &tt_index[udev->ttport - 1];
		/*  How many entries are left in tt_index? */
		for (i = 0; i < utt->hub->maxchild; ++i)
			cnt += !!tt_index[i];
	} else {
		tt_index = NULL;
		ptt = (struct mu3h_sch_tt **) &utt->hcpriv;
	}

	tt = *ptt;
	if (!tt || !list_empty(&tt->ep_list))
		return;		/* never allocated , or still in use*/

	*ptt = NULL;

	kfree(tt);

	if (cnt == 1) {
		utt->hcpriv = NULL;
		kfree(tt_index);
	}
}


/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 * check TT scheduler parameters and bandwidth enough.
 * @param[in]
 *     udev: usb_device struct pointer.\n
 * @param[in]
 *     sch_ep: mu3h_sch_ep_info struct pointer.\n
 * @param[in]
 *     offset: offset for start ss.\n
 * @return
 *     0, function success.\n
 *     -ERANGE, Over TT Bandwidth limitation.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If Over usb bus Bandwidth limitation, return -ERANGE.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int check_sch_tt(struct usb_device *udev,
	struct mu3h_sch_ep_info *sch_ep, u32 offset)
{
	int i;
	u32 start_ss;
	u32 last_ss;
	u32 start_cs;
	u32 last_cs;
	u32 extra_cs_count;
	u32 fs_budget_start;
	u32 ep_type;
	u32 max_packet_size;
	u32 *ep_bw_budget_table;
	struct xhci_ep_ctx *ep_ctx;
	struct mu3h_sch_tt *tt;

	tt = find_tt(udev);
	if (IS_ERR(tt))
		return PTR_ERR(tt);

	ep_ctx = sch_ep->ep_ctx;
	ep_type = CTX_TO_EP_TYPE(le32_to_cpu(ep_ctx->ep_info2));
	max_packet_size = MAX_PACKET_DECODED(le32_to_cpu(ep_ctx->ep_info2));
	sch_ep->cs_count = DIV_ROUND_UP(max_packet_size, FS_PAYLOAD_MAX);

	ep_bw_budget_table = sch_ep->bw_budget_table;
	memset(ep_bw_budget_table, 0, TT_MICROFRAMES_MAX * sizeof(u32));

	start_ss = offset % 8;
	fs_budget_start = (start_ss + 1) % 8;

	if (ep_type == ISOC_OUT_EP) {
		last_ss = start_ss + sch_ep->cs_count - 1;

		/*
		* usb_20 spec section11.18:
		* must never schedule Start-Split in Y6
		*/
		if (!(start_ss == 7 || last_ss < 6))
			return -ERANGE;

		for (i = 0; i < sch_ep->cs_count; i++) {
			if (test_bit(offset + i, tt->ss_bit_map))
				return -ERANGE;
		}

		sch_ep->num_budget_microframes = sch_ep->cs_count;
		/* update bw_budget_table */
		for (i = 0; i < sch_ep->num_budget_microframes; i++)
			ep_bw_budget_table[i] = sch_ep->bw_cost_per_microframe;
	} else {
		/*
		* usb_20 spec section11.18:
		* must never schedule Start-Split in Y6
		*/

		if (start_ss == 6)
			return -ERANGE;

		/* one uframe for ss + one uframe for idle */
		start_cs = (start_ss + 2) % 8;
		last_cs = start_cs + sch_ep->cs_count - 1;

		if (last_cs > 7)
			return -ERANGE;

		if (ep_type == ISOC_IN_EP) {
			if (last_cs == 7)
				extra_cs_count = 1;
			else
				extra_cs_count = 2;
		} else { /* ep_type : INTR IN / INTR OUT */
			if (fs_budget_start == 6)
				extra_cs_count = 1;
			else
				extra_cs_count = 2;
		}

		sch_ep->cs_count += extra_cs_count;
		if (sch_ep->cs_count > 7)
			sch_ep->cs_count = 7;

		/* one uframe for ss + one uframe for idle */
		sch_ep->num_budget_microframes = sch_ep->cs_count + 2;
		if (sch_ep->num_budget_microframes > sch_ep->esit)
			sch_ep->num_budget_microframes = sch_ep->esit;

		if (test_bit(offset, tt->ss_bit_map))
			return -ERANGE;

		for (i = 0; i < sch_ep->cs_count; i++) {
			if (test_bit(offset + 2 + i, tt->cs_bit_map))
				return -ERANGE;
		}

		/* update bw_budget_table */
		if (ep_type == INT_OUT_EP) {
			ep_bw_budget_table[0] = sch_ep->bw_cost_per_microframe;
		} else { /*  ep_type : INTR IN / ISOC IN */
			ep_bw_budget_table[0] = 0; /* start split */
			ep_bw_budget_table[1] = 0; /* idle */

			for (i = 2; i < sch_ep->num_budget_microframes; i++)
				ep_bw_budget_table[i] =
				sch_ep->bw_cost_per_microframe;
		}

	}

	return 0;

}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 * update TT scheduler bandwidth information.\n
 * @param[in]
 *     udev: usb_device struct pointer.\n
 * @param[in]
 *     sch_ep: mu3h_sch_ep_info struct pointer.\n
 * @return
 *     0, function success.\n
 *     -ENOMEM, find_tt() fail.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If find_tt() fail ,return -ENOMEM.\n
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int update_sch_tt(struct usb_device *udev,
	struct mu3h_sch_ep_info *sch_ep)
{
	int i;
	u32 ep_type;
	u32 offset;
	struct xhci_ep_ctx *ep_ctx;
	struct mu3h_sch_tt *tt;

	tt = find_tt(udev);
	if (IS_ERR(tt))
		return PTR_ERR(tt);

	offset = sch_ep->offset;
	ep_ctx = sch_ep->ep_ctx;
	ep_type = CTX_TO_EP_TYPE(le32_to_cpu(ep_ctx->ep_info2));

	if (ep_type == ISOC_OUT_EP) {
		for (i = 0; i < sch_ep->cs_count; i++)
			set_bit(offset + i, tt->ss_bit_map);
	} else {
		set_bit(offset, tt->ss_bit_map);
		for (i = 0; i < sch_ep->cs_count; i++)
			set_bit(offset + 2 + i, tt->cs_bit_map);
	}

	return 0;
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 * check scheduler parameters and bandwidth enough.
 * @param[in]
 *     udev: usb_device struct pointer.\n
 * @param[in]
 *     sch_bw: mu3h_sch_bw_info struct pointer.\n
 * @param[in]
 *     sch_ep: mu3h_sch_ep_info struct pointer.\n
 * @return
 *     0, function success,Bandwidth enough.\n
 *     -ERANGE, Over usb bus Bandwidth limitation.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If Over usb bus Bandwidth limitation, return -ERANGE.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int check_sch_bw(struct usb_device *udev,
	struct mu3h_sch_bw_info *sch_bw, struct mu3h_sch_ep_info *sch_ep)
{
	int ret;
	u32 offset;
	u32 cs_count;
	u32 esit;
	u32 num_budget_microframes;
	u32 min_bw;
	u32 min_index;
	u32 worst_bw;
	u32 bw_boundary;
	struct usb_host_endpoint *ep;
	bool tt_offset_ok = false;

	ep = sch_ep->ep;

	if (sch_ep->esit > XHCI_MTK_MAX_ESIT)
		sch_ep->esit = XHCI_MTK_MAX_ESIT;

	esit = sch_ep->esit;

	/*
	 * Search through all possible schedule microframes.
	 * and find a microframe where its worst bandwidth is minimum.
	 */
	min_bw = ~0;
	min_index = 0;
	cs_count = 0;
	for (offset = 0; offset < esit; offset++) {
		num_budget_microframes = sch_ep->num_budget_microframes;
		if ((offset + num_budget_microframes) > sch_ep->esit)
			break;

		if (is_fs_or_ls(udev->speed)) {
			ret = check_sch_tt(udev, sch_ep, offset);
			if (ret)
				continue;
			else
				tt_offset_ok = true;
		}

		worst_bw = get_max_bw(sch_bw, sch_ep, offset);
		if (min_bw > worst_bw) {
			min_bw = worst_bw;
			min_index = offset;
			cs_count = sch_ep->cs_count;
		}
		if (min_bw == 0)
			break;
	}

	/* all offset for tt is not ok*/
	if (is_fs_or_ls(udev->speed) && tt_offset_ok == false)
		return -ERANGE;

	sch_ep->offset = min_index;
	sch_ep->cs_count = cs_count;

	bw_boundary = (udev->speed == USB_SPEED_SUPER)
			? SS_BW_BOUNDARY : HS_BW_BOUNDARY;

	/* check bandwidth */
	if (min_bw > bw_boundary)
		return -ERANGE;

	if (is_fs_or_ls(udev->speed)) {
		ret = update_sch_tt(udev, sch_ep);
		if (ret)
			return ret;
	}

	/* update bus bandwidth info */
	update_bus_bw(sch_bw, sch_ep, ADD_EP);

	return 0;
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 * check @ep need scheduler parameters and bandwidth check.\n
 * only for periodic EPs need check.
 * @param[in]
 *      sch_ep: mu3h_sch_ep_info struct pointer.\n
 * @param[in]
 *      speed: enum usb_device_speed speed\n
 * @param[in]
 *      has_tt: has TT Hub connected.
 * @return
 *      true, this is EP need check bandwidth.\n
 *      false, this is EP don't need check bandwidth.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static bool need_bw_sch(struct usb_host_endpoint *ep,
	enum usb_device_speed speed, int has_tt)
{
	/* only for periodic endpoints */
	if (usb_endpoint_xfer_control(&ep->desc)
		|| usb_endpoint_xfer_bulk(&ep->desc))
		return false;

	/*
	 * for LS & FS periodic endpoints which its device is not behind
	 * a TT are also ignored, root-hub will schedule them directly,
	 * but need set @bpkts field of endpoint context to 1.
	 */
	if (is_fs_or_ls(speed) && !has_tt)
		return false;

	return true;
}

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 * MTK Software scheduler mamory alloc and parameters initialization.\n
 * @param[in]
 *     mtk: xhci_hcd_mtk struct pointer.
 * @return
 *     0, function success.\n
 *     -ENOMEM, memory alloc fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If memory alloc fail ,return -ENOMEM.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int xhci_mtk_sch_init(struct xhci_hcd_mtk *mtk)
{
	struct mu3h_sch_bw_info *sch_array;
	int num_usb_bus;
	int i;

	/* ss IN and OUT are separated */
	num_usb_bus = mtk->num_u3_ports * 2 + mtk->num_u2_ports;

	sch_array = kcalloc(num_usb_bus, sizeof(*sch_array), GFP_KERNEL);
	if (sch_array == NULL)
		return -ENOMEM;

	for (i = 0; i < num_usb_bus; i++)
		INIT_LIST_HEAD(&sch_array[i].bw_ep_list);

	mtk->sch_array = sch_array;

	return 0;
}
EXPORT_SYMBOL_GPL(xhci_mtk_sch_init);

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 * MTK Software scheduler mamory release.\n
 * @param[in]
 *     mtk: xhci_hcd_mtk struct pointer.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void xhci_mtk_sch_exit(struct xhci_hcd_mtk *mtk)
{
	kfree(mtk->sch_array);
}
EXPORT_SYMBOL_GPL(xhci_mtk_sch_exit);

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 * add EPs scheduler parameters to endpoint context.
 * @param[in]
 *     hcd: usb_hcd struct pointer.\n
 * @param[in]
 *     udev: usb_device struct pointer.\n
 * @param[in]
 *     ep: usb_host_endpoint struct pointer.\n
 * @return
 *     0, function success.\n
 *     -ENOMEM, memory alloc fail.\n
 *     -ENOMEM, setup_sch_info() fail.\n
 *     -ENOSPC, check_sch_bw() fail.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If memory alloc fail ,return -ENOMEM.\n
 *     2. If setup_sch_info() fail ,return -ENOMEM.\n
 *     3. If check_sch_bw() fail ,return -ENOSPC.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int xhci_mtk_add_ep_quirk(struct usb_hcd *hcd, struct usb_device *udev,
		struct usb_host_endpoint *ep)
{
	struct xhci_hcd_mtk *mtk = hcd_to_mtk(hcd);
	struct xhci_hcd *xhci;
	struct xhci_ep_ctx *ep_ctx;
	struct xhci_slot_ctx *slot_ctx;
	struct xhci_virt_device *virt_dev;
	struct mu3h_sch_bw_info *sch_bw;
	struct mu3h_sch_ep_info *sch_ep;
	struct mu3h_sch_bw_info *sch_array;
	struct mu3h_sch_tt *tt;
	unsigned int ep_index;
	int bw_index;
	int ret = 0;

	xhci = hcd_to_xhci(hcd);
	virt_dev = xhci->devs[udev->slot_id];
	ep_index = xhci_get_endpoint_index(&ep->desc);
	slot_ctx = xhci_get_slot_ctx(xhci, virt_dev->in_ctx);
	ep_ctx = xhci_get_ep_ctx(xhci, virt_dev->in_ctx, ep_index);
	sch_array = mtk->sch_array;

	xhci_dbg(xhci, "%s() type:%d, speed:%d, mpkt:%d, dir:%d, ep:%p\n",
		__func__, usb_endpoint_type(&ep->desc), udev->speed,
		usb_endpoint_maxp(&ep->desc),
		usb_endpoint_dir_in(&ep->desc), ep);

	if (!need_bw_sch(ep, udev->speed, slot_ctx->tt_info & TT_SLOT)) {
		/*
		 * set @bpkts to 1 if it is LS or FS periodic endpoint, and its
		 * device does not connected through an external HS hub
		 */
		if (usb_endpoint_xfer_int(&ep->desc)
			|| usb_endpoint_xfer_isoc(&ep->desc))
			ep_ctx->reserved[0] |= cpu_to_le32(EP_BPKTS(1));

		return 0;
	}

	bw_index = get_bw_index(xhci, udev, ep);
	sch_bw = &sch_array[bw_index];

	sch_ep = kzalloc(sizeof(struct mu3h_sch_ep_info), GFP_NOIO);
	if (!sch_ep)
		return -ENOMEM;
	sch_ep->ep = ep;
	sch_ep->ep_ctx = ep_ctx;

	ret = setup_sch_info(udev, ep_ctx, sch_ep);
	if (ret) {
		xhci_err(xhci, "Setup schduler info fail !\n");
		kfree(sch_ep);
		return ret;
	}

	ret = check_sch_bw(udev, sch_bw, sch_ep);
	if (ret) {
		xhci_err(xhci, "Not enough bandwidth!\n");
		kfree(sch_ep);
		return -ENOSPC;
	}

	list_add_tail(&sch_ep->endpoint, &sch_bw->bw_ep_list);
	if (is_fs_or_ls(udev->speed)) {
		tt = find_tt(udev);
		if (IS_ERR(tt))
			return PTR_ERR(tt);

		list_add_tail(&sch_ep->tt_endpoint, &tt->ep_list);
	}


	ep_ctx->reserved[0] |= cpu_to_le32(EP_BPKTS(sch_ep->pkts)
		| EP_BCSCOUNT(sch_ep->cs_count) | EP_BBM(sch_ep->burst_mode));
	ep_ctx->reserved[1] |= cpu_to_le32(EP_BOFFSET(sch_ep->offset)
		| EP_BREPEAT(sch_ep->repeat));

	xhci_dbg(xhci, " PKTS:%x, CSCOUNT:%x, BM:%x, OFFSET:%x, REPEAT:%x\n",
			sch_ep->pkts, sch_ep->cs_count, sch_ep->burst_mode,
			sch_ep->offset, sch_ep->repeat);

	return 0;
}
EXPORT_SYMBOL_GPL(xhci_mtk_add_ep_quirk);

/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 * drop EPs scheduler parameters.
 * @param[in]
 *     hcd: usb_hcd struct pointer.\n
 * @param[in]
 *     udev: usb_device struct pointer.\n
 * @param[in]
 *     ep: usb_host_endpoint struct pointer.\n
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void xhci_mtk_drop_ep_quirk(struct usb_hcd *hcd, struct usb_device *udev,
		struct usb_host_endpoint *ep)
{
	struct xhci_hcd_mtk *mtk = hcd_to_mtk(hcd);
	struct xhci_hcd *xhci;
	struct xhci_slot_ctx *slot_ctx;
	struct xhci_virt_device *virt_dev;
	struct mu3h_sch_bw_info *sch_array;
	struct mu3h_sch_bw_info *sch_bw;
	struct mu3h_sch_ep_info *sch_ep;
	int bw_index;

	xhci = hcd_to_xhci(hcd);
	virt_dev = xhci->devs[udev->slot_id];
	slot_ctx = xhci_get_slot_ctx(xhci, virt_dev->in_ctx);
	sch_array = mtk->sch_array;

	xhci_dbg(xhci, "%s() type:%d, speed:%d, mpks:%d, dir:%d, ep:%p\n",
		__func__, usb_endpoint_type(&ep->desc), udev->speed,
		usb_endpoint_maxp(&ep->desc),
		usb_endpoint_dir_in(&ep->desc), ep);

	if (!need_bw_sch(ep, udev->speed, slot_ctx->tt_info & TT_SLOT))
		return;

	bw_index = get_bw_index(xhci, udev, ep);
	sch_bw = &sch_array[bw_index];

	list_for_each_entry(sch_ep, &sch_bw->bw_ep_list, endpoint) {
		if (sch_ep->ep == ep) {
			update_bus_bw(sch_bw, sch_ep, DROP_EP);
			list_del(&sch_ep->endpoint);
			if (is_fs_or_ls(udev->speed)) {
				list_del(&sch_ep->tt_endpoint);
				drop_tt(udev);
			}
			kfree(sch_ep->bw_budget_table);
			kfree(sch_ep);
			break;
		}
	}
}
EXPORT_SYMBOL_GPL(xhci_mtk_drop_ep_quirk);


/** @ingroup IP_group_usbhost_internal_function
 * @par Description
 * xhci mtk set link timer.
 * @param[in]
 *     hcd: usb_hcd struct pointer.\n
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void xhci_mtk_retimer(struct usb_hcd *hcd)
{
	struct xhci_hcd_mtk *mtk = hcd_to_mtk(hcd);
	struct mu3c_ippc_regs __iomem *ippc = mtk->ippc_regs;
	void __iomem	*tmp_regs;
	u32 value;

	udelay(1000);

	/* read HW sub ID */
	value = readl(&ippc->ssusb_hw_sub_id);

	if (value != HOST_SSUSB_HW_SUB_ID) {
		tmp_regs = mtk->mac_regs + UX_LFPS_TIMING_PARAMETER;
		value = readl(tmp_regs);
		value &= 0xffffff00;
		value |= UX_EXIT_T12_T11_MINUS_300NS;
		writel(value, tmp_regs);

		tmp_regs = mtk->mac_regs + U1_LFPS_TIMING_PARAMETER_2;
		value = readl(tmp_regs);
		value &= 0xffff0000;
		value |= (U1_EXIT_T13_T11 | U1_EXIT_T12_T10);
		writel(value, tmp_regs);

		tmp_regs = mtk->mac_regs + LINK_HP_TIMER;
		value = readl(tmp_regs);
		value &= 0xffffffe0;
		value |= PHP_TIMEOUT_VALUE;
		writel(value, tmp_regs);

		tmp_regs = mtk->mac_regs + LINK_PM_TIMER;
		value = readl(tmp_regs);
		value &= 0xfffff0e0;
		value |= (PM_LC_TIMEOUT_VALUE | PM_ENTRY_TIMEOUT_VALUE);
		writel(value, tmp_regs);
	}
}
EXPORT_SYMBOL_GPL(xhci_mtk_retimer);
