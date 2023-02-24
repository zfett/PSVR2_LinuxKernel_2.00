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

#ifndef _SIE_GBUF_H
#define _SIE_GBUF_H

#include <linux/types.h>

//#define CONFIG_GBUF_DEBUG

#ifdef __KERNEL__
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/poll.h>
#include <linux/usb/gadget.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/uio.h>

#define GBUF_NAME_LEN (16)

struct gbuf;
struct gbuf_one;

struct gbuf_dmabuf {
	struct list_head list;
	struct usb_request *req; // usb_request mapped with this
	struct usb_ep *ep;       // associated EP
	size_t validlen;         // valid data size returned by UDC
	int status;              // copy of usb_request.status
	wait_queue_head_t waitq; // for sync write (IN)
	struct gbuf_one *gbuf;   // parent
};

#define QUEUE_MAX (16)  // maximum allowable queue count in dwc3

typedef void (*gbuf_complete_cb)(struct gbuf *gbuf, struct usb_ep *ep, struct usb_request *req);

struct gbuf_one {
	// io lock
	struct mutex io_mutex;
	// lists for an mbuf_dmabuf
	spinlock_t lock;                // lock for following lists
	struct list_head freelist;      // unused. ready for usb_ep_request()
	struct list_head filledlist;    // data buffer holding
	struct list_head busylist;      // enqueued to udc

	atomic_t busycount;             // how many requests in UDC queue

	wait_queue_head_t waitq;
	size_t packetlen;               // buffer size of each item
	unsigned int items;             // number of gbuf_dmabuf

	struct gbuf_dmabuf *dmabufs;

	// callbacks (usb_request)
	gbuf_complete_cb complete;

	// EPs associated
	struct usb_ep *ep;

	struct gbuf *gbuf;              // parent
	atomic_t refcnt;                // for kmalloc memory and usb_request

#ifdef CONFIG_GBUF_DEBUG
	// stat/debug
	unsigned int stat_error;        // error reported in usb_request.stat
	unsigned int error;             // misc error on underlaying layer

	int completed;
	int zlp;
	int wakeup;
	int irw;
	int woken;
	int busyfill;
	ktime_t lastcb;
#endif
};

struct gbuf {
	struct gbuf_one *in;
	struct gbuf_one *out;
	wait_queue_head_t in_poll_waitq;
	wait_queue_head_t out_poll_waitq;
	spinlock_t lock;                // lock for refer the gbuf_one instance

	unsigned long flags;
#define GBUF_FLAG_INITIALIZING (0)  // gbuf_init() already called
#define GBUF_FLAG_INIT      (1)     // gbuf_init() done
#define GBUF_FLAG_OPEN      (2)     // open(2) called
#define GBUF_READ_INDIVIDUALLY (3)
#define GBUF_WRITE_COMPLETELY  (4)
#define GBUF_FLAG_CONSTRUCTED  (5)  // gbuf_init() first call

	// misc
	char name[GBUF_NAME_LEN];       // identifier (just for debug)
	void *user;                     // user scratch pad

#ifdef CONFIG_GBUF_DEBUG
	struct dentry *dbgroot;
#endif
};

// filesystem_ops
extern ssize_t gbuf_read_iter(struct gbuf *gbuf, struct kiocb *kiocb, struct iov_iter *to);
extern ssize_t gbuf_write_iter(struct gbuf *gbuf, struct kiocb *kiocb, struct iov_iter *from);
extern unsigned int gbuf_poll(struct gbuf *gbuf, struct file *file, poll_table *wait);

extern int gbuf_open(struct gbuf *gbuf, struct inode *inode, struct file *file);
extern int gbuf_release(struct gbuf *gbuf, struct inode *inode, struct file *file);
extern int gbuf_fsync(struct gbuf *gbuf, struct file *file, loff_t start, loff_t end,
		      int datasync);
// gbuf if
extern int gbuf_init(struct gbuf *gbuf, const char *name,
		     struct usb_ep *oep, size_t opacketlen, unsigned int orequests, gbuf_complete_cb ocomplete,
		     struct usb_ep *iep, size_t ipacketlen, unsigned int irequests, gbuf_complete_cb icomplete,
		     int read_individually, int write_completely);
extern void gbuf_deinit(struct gbuf *gbuf);

// Pre-queue some requests
extern int gbuf_pre_queue(struct gbuf *gbuf);

#endif // _KERNEL_
#endif // _SIE_GBUF_H
