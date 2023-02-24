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

#include <linux/debugfs.h>
#include <linux/delay.h>
#include "gbuf.h"

#ifdef CONFIG_GBUF_DEBUG
#define GBUF_DEBUG(statement) { statement; }
#else
#define GBUF_DEBUG(statement)
#endif

static struct gbuf_one *gbuf_reserve_one(struct gbuf *gbuf, struct gbuf_one **one)
{
	struct gbuf_one *ret;
	unsigned long flags;
	spin_lock_irqsave(&gbuf->lock, flags);
	ret = *one;
	if (ret)
		atomic_inc(&ret->refcnt);
	spin_unlock_irqrestore(&gbuf->lock, flags);
	return ret;
}
#define gbuf_reserve_in(gbuf) gbuf_reserve_one(gbuf, &gbuf->in)
#define gbuf_reserve_out(gbuf) gbuf_reserve_one(gbuf, &gbuf->out)

static void gbuf_release_one(struct gbuf_one *one)
{
	int i;

	if (one && !atomic_dec_return(&one->refcnt)) {
		// free request and memory
		if (one->dmabufs) {
			// release usb_request
			for (i = 0; i < one->items; i++) {
				if (one->dmabufs[i].req) {
					if (one->dmabufs[i].req->buf) {
						kfree(one->dmabufs[i].req->buf);
					}
					usb_ep_free_request(one->ep,
							    one->dmabufs[i].req);
				}
			}
			// release dmabuffer
			kfree(one->dmabufs);
			one->dmabufs = NULL;
		}
		kfree(one);
	}
}

static struct gbuf_one *gbuf_alloc_one(struct gbuf *gbuf, struct gbuf_one **one)
{
	struct gbuf_one *ret;
	unsigned long flags;
	spin_lock_irqsave(&gbuf->lock, flags);
	ret = *one = kzalloc(sizeof(struct gbuf_one), GFP_KERNEL);
	if (ret)
		atomic_inc(&ret->refcnt);
	spin_unlock_irqrestore(&gbuf->lock, flags);
	return ret;
}
#define gbuf_alloc_in(gbuf) gbuf_alloc_one(gbuf, &gbuf->in)
#define gbuf_alloc_out(gbuf) gbuf_alloc_one(gbuf, &gbuf->out)

static void gbuf_free_one(struct gbuf *gbuf, struct gbuf_one **one)
{
	unsigned long flags;
	spin_lock_irqsave(&gbuf->lock, flags);
	gbuf_release_one(*one);
	*one = NULL;
	spin_unlock_irqrestore(&gbuf->lock, flags);
}
#define gbuf_free_in(gbuf) gbuf_free_one(gbuf, &gbuf->in)
#define gbuf_free_out(gbuf) gbuf_free_one(gbuf, &gbuf->out)

//#include "gbuf_trace.h"
/*
 * TODO:
 * - more descriptive queue status
 */
#if 0
// enable if gbuf_log trace used
static void gbuf_trace(void (*trace)(struct va_format *), const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;

	va_start(args, fmt);
	vaf.fmt = fmt;
	vaf.va = &args;
	trace(&vaf);
	va_end(args);
}
#endif

#ifdef CONFIG_GBUF_DEBUG
static int list_count(struct list_head *l)
{
	int ret = 0;
	struct list_head *p;
	list_for_each(p, l)
		ret++;
	return ret;
}

static int gbuf_queuestat_show(struct seq_file *s, void *unused)
{
	struct gbuf *gbuf = s->private;
	struct gbuf_one *gbufs[2];
	int i;
	unsigned long flags;
	int l1, l2, l3;

	gbufs[0] = gbuf_reserve_out(gbuf);
	gbufs[1] = gbuf_reserve_in(gbuf);

	for (i = 0; i < ARRAY_SIZE(gbufs); i++) {
		struct gbuf_one *gbuf_one = gbufs[i];

		if (!gbuf_one || !gbuf_one->ep)
			continue;
		spin_lock_irqsave(&gbuf_one->lock, flags);
		l1 = list_count(&gbuf_one->filledlist);
		l2 = list_count(&gbuf_one->freelist);
		l3 = list_count(&gbuf_one->busylist);
		spin_unlock_irqrestore(&gbuf_one->lock, flags);
		seq_printf(s,"%02X:comp=%d zlp=%d fil=%d free=%d busy=%d "
			   "wake=%d woken=%d call=%d "
			   "bfill=%d time=%lld\n",
			   gbuf_one->ep->address,
			   gbuf_one->completed,
			   gbuf_one->zlp,
			   l1, l2, l3,
			   gbuf_one->wakeup, gbuf_one->woken, gbuf_one->irw,
			   gbuf_one->busyfill,
			   ktime_to_ns(gbuf_one->lastcb));
	}

	gbuf_release_one(gbufs[0]);
	gbuf_release_one(gbufs[1]);

	return 0;
} // queustat_show

static int gbuf_queuestat_open(struct inode *inode, struct file *file)
{
	return single_open(file, gbuf_queuestat_show, inode->i_private);
}

static const struct file_operations gbuf_queuestat_fops = {
	.open 		= gbuf_queuestat_open,
	.read 		= seq_read,
	.llseek 	= seq_lseek,
	.release 	= single_release,
};
#endif // CONFIG_GBUF_DEBUG

static int gbuf_list_count(struct gbuf_one *gbuf, struct list_head *l)
{
	int ret = 0;
	struct list_head *p;
	unsigned long flags;
	spin_lock_irqsave(&gbuf->lock, flags);
	list_for_each(p, l)
		ret++;
	spin_unlock_irqrestore(&gbuf->lock, flags);
	return ret;
}

/* move a dmabuf to other list */
inline static void _gbuf_move_dmabuf(struct gbuf_dmabuf *dmabuf, struct list_head *tolist)
{
	/* remove it from the list */
	list_move_tail(&dmabuf->list, tolist);
} /* _gbuf_move_dmabuf */

static void gbuf_move_dmabuf(struct gbuf_dmabuf *dmabuf, struct list_head *tolist)
{
	struct gbuf_one *gbuf;
	unsigned long flags;

	gbuf = dmabuf->gbuf;

	spin_lock_irqsave(&gbuf->lock, flags);
	_gbuf_move_dmabuf(dmabuf, tolist);
	spin_unlock_irqrestore(&gbuf->lock, flags);
} /* gbuf_move_dmabuf */

/* put an alone dmabuf to the list */
inline static void _gbuf_put_dmabuf(struct gbuf_dmabuf *dmabuf, struct list_head *tolist)
{
	list_add_tail(&dmabuf->list, tolist);
} /* gbuf_put_dmabuf */

static void gbuf_put_dmabuf(struct gbuf_dmabuf *dmabuf, struct list_head *tolist)
{
	struct gbuf_one *gbuf;
	unsigned long flags;

	gbuf = dmabuf->gbuf;

	spin_lock_irqsave(&gbuf->lock, flags);
	_gbuf_put_dmabuf(dmabuf, tolist);
	spin_unlock_irqrestore(&gbuf->lock, flags);
} /* gbuf_put_dmabuf */

inline static struct gbuf_dmabuf *_gbuf_peek_dmabuf(struct list_head *fromlist)
{
	return list_first_entry_or_null(fromlist,
					struct gbuf_dmabuf, list);

}
static struct gbuf_dmabuf *gbuf_peek_dmabuf(struct gbuf_one *gbuf,
					    struct list_head *fromlist)
{
	struct gbuf_dmabuf *dmabuf;
	unsigned long flags;

	spin_lock_irqsave(&gbuf->lock, flags);
	dmabuf = _gbuf_peek_dmabuf(fromlist);
	spin_unlock_irqrestore(&gbuf->lock, flags);
	return dmabuf;
} /* gbuf_peek_dmabuf */

static struct gbuf_dmabuf *gbuf_get_dmabuf(struct gbuf_one *gbuf,
					   struct list_head *fromlist)
{
	struct gbuf_dmabuf *dmabuf;
	unsigned long flags;

	spin_lock_irqsave(&gbuf->lock, flags);
	dmabuf = _gbuf_peek_dmabuf(fromlist);
	/* remove it from the list */
	if (dmabuf)
		list_del(&dmabuf->list);
	spin_unlock_irqrestore(&gbuf->lock, flags);
	return dmabuf;
} /* gbuf_get_dmabuf */

#define gbuf_ep_queue(dmabuf, gfp_flags) usb_ep_queue(dmabuf->ep, dmabuf->req, gfp_flags)
#define gbuf_ep_dequeue(dmabuf) usb_ep_dequeue(dmabuf->ep, dmabuf->req)

static int gbuf_ep_queue_first(struct gbuf_one *gbuf, struct list_head *fromlist)
{
	struct gbuf_dmabuf *dmabuf;
	int ret;
	unsigned long flags;

	spin_lock_irqsave(&gbuf->lock,flags);
	if (!(dmabuf = _gbuf_peek_dmabuf(fromlist))) {
		spin_unlock_irqrestore(&gbuf->lock,flags);
		return 0;
	}

	if (QUEUE_MAX < atomic_inc_return(&gbuf->busycount)) {
		spin_unlock_irqrestore(&gbuf->lock,flags);
		atomic_dec(&gbuf->busycount);
		return -EBUSY;
	}

	_gbuf_move_dmabuf(dmabuf, &gbuf->busylist);
	ret = gbuf_ep_queue(dmabuf, GFP_KERNEL);
	spin_unlock_irqrestore(&gbuf->lock,flags);

	if (ret < 0) {
		GBUF_DEBUG(gbuf->error++);
		gbuf_move_dmabuf(dmabuf, &gbuf->freelist);
		atomic_dec(&gbuf->busycount);
		dmabuf->req->status = ret;
		wake_up_interruptible(&dmabuf->waitq); /* sync io */
	}

	return ret;
}

/* OUT ep completed */
static void gbuf_ep_complete_out(struct usb_ep *ep, struct usb_request *req)
{
	struct gbuf_dmabuf *dmabuf;
	struct gbuf_one *gbuf;
	bool queue_next = false;

	dmabuf = req->context;
	gbuf = dmabuf->gbuf;

//	trace_gbuf_complete(dmabuf, ep, req);

	atomic_dec(&gbuf->busycount);

	GBUF_DEBUG(gbuf->lastcb = ktime_get());

	dmabuf->status = req->status;
	if (unlikely(dmabuf->status)) {
		GBUF_DEBUG(gbuf->stat_error++);
		dmabuf->validlen = 0;
	} else {
		dmabuf->validlen = req->actual;
	}

	GBUF_DEBUG(gbuf->completed++);

	switch (dmabuf->status) {
	case -ECONNRESET: // request dequeued
	case -ESHUTDOWN: // usb_request queued in UDC gets ESHUTDOWN if EP is disabled
		gbuf_move_dmabuf(dmabuf, &gbuf->freelist);
		break;
	default:
		// intentionally fall through
		// errors except disconnection are passed to the user
	case 0:
		GBUF_DEBUG( if (!dmabuf->validlen) gbuf->zlp++; );

		// For non iso OUT, no need to keep busy the controller,
		// because if no request queued it will NAK/NRDY
		gbuf_move_dmabuf(dmabuf, &gbuf->filledlist);

		queue_next = true;
		break;
	}

	// wake up the reader
	GBUF_DEBUG(gbuf->wakeup++);
	wake_up_interruptible(&gbuf->waitq);
	wake_up_interruptible(&gbuf->gbuf->out_poll_waitq);

	// callback USB function
	if (gbuf->complete)
		gbuf->complete(gbuf->gbuf, ep, req);

	if (queue_next)
		gbuf_ep_queue_first(gbuf, &gbuf->freelist);
} /* gbuf_ep_complete_out */

static void gbuf_ep_complete_in(struct usb_ep *ep, struct usb_request *req)
{
	struct gbuf_dmabuf *dmabuf;
	struct gbuf_one *gbuf;
	bool queue_next = false;

	dmabuf = req->context;
	gbuf = dmabuf->gbuf;

//	trace_gbuf_complete(dmabuf, ep, req);

	atomic_dec(&gbuf->busycount);

	GBUF_DEBUG(gbuf->lastcb = ktime_get());

	GBUF_DEBUG(gbuf->completed++);

	GBUF_DEBUG( if (unlikely(req->status)) gbuf->stat_error++; );

	switch (req->status) {
	case -ECONNRESET: /* request dequeued */
	case -ESHUTDOWN: /* usb_request queued in UDC gets ESHUTDOWN if EP is disabled */
		gbuf_move_dmabuf(dmabuf, &gbuf->freelist);
		break;
	default:
		pr_debug("%s: err=%d\n", __func__, dmabuf->status);
		// intentionally fall through
	case 0:
		/* mode it from busy to freelist */
		gbuf_move_dmabuf(dmabuf, &gbuf->freelist);

		queue_next = true;
		break;
	};

	/* wake up waiter */
	GBUF_DEBUG(gbuf->wakeup++);
	wake_up_interruptible(&gbuf->waitq); /* freelist */
	wake_up_interruptible(&dmabuf->waitq); /* sync io */
	wake_up_interruptible(&gbuf->gbuf->in_poll_waitq); /* poll */

	/* callback USB function */
	if (gbuf->complete)
		gbuf->complete(gbuf->gbuf, ep, req);

	if (queue_next)
		gbuf_ep_queue_first(gbuf, &gbuf->filledlist);
} /* gbuf_complete_in */

// Pre-queue requests to underlaying UDC
// NOTE: dwc3 never queues any requests to hw and
// then transfers never happens after that if
// all the following condition are met:
// - the host has already issued an IN/OUT to the EP
// - EP is isochronous
// - no requests are queued to dwc3
// To avoid this, issue dummy (ZLP) request(s) to dwc3
// before the host starts to issue
// Cf. Synopsys datasheet table6-52 XferNotReady event
int gbuf_pre_queue(struct gbuf *gbuf)
{
	struct gbuf_dmabuf *dmabuf = NULL;
	int i;

	pr_debug("%s:%s start\n", __func__, gbuf->name);

	if (!test_bit(GBUF_FLAG_INIT, &gbuf->flags))
		return -EPERM;

	if (gbuf->out) {
		// start all OUT requests (if we have)
		GBUF_DEBUG(gbuf->out->completed = 0);
		GBUF_DEBUG(gbuf->out->zlp = 0);

		for (i = 0; i < QUEUE_MAX; i++) {
			gbuf_ep_queue_first(gbuf->out, &gbuf->out->freelist);
		}
	}

	if (gbuf->in) {
		// start one IN request (if we have)
		GBUF_DEBUG(gbuf->in->completed = 0);
		GBUF_DEBUG(gbuf->in->zlp = 0);

		// prequeued request issues ZLP
		dmabuf = gbuf_get_dmabuf(gbuf->in, &gbuf->in->freelist);
		dmabuf->req->length = 0;
		gbuf_put_dmabuf(dmabuf, &gbuf->in->filledlist);
		gbuf_ep_queue_first(gbuf->in, &gbuf->in->filledlist);
	}

	return 0;
} /* gbuf_pre_queue */

/*
 * should be called from host's open() callback
 */
int gbuf_open(struct gbuf *gbuf, struct inode *inode, struct file *file)
{
	pr_debug("%s:%s start %lx\n", __func__, gbuf->name, gbuf->flags);

//	trace_gbuf_open(gbuf);

	if (test_and_set_bit(GBUF_FLAG_OPEN, &gbuf->flags))
		return -EBUSY;

	return 0;
} /* gbuf_open */

/*
 * get called when the node is closed
 */
int gbuf_release(struct gbuf *gbuf, struct inode *inode, struct file *file)
{
	struct gbuf_dmabuf *dmabuf;
	int i, j;
	struct gbuf_one *gbufs[2];

	pr_debug("%s:%s start %lx\n", __func__, gbuf->name, gbuf->flags);
//	trace_gbuf_release(gbuf);

	if (!test_and_clear_bit(GBUF_FLAG_OPEN, &gbuf->flags)) {
		pr_info("%s: %s release for unopened\n", __func__, gbuf->name);
		return -EINVAL;
	}

	if (!test_bit(GBUF_FLAG_INIT, &gbuf->flags))
		return 0;

	gbufs[0] = gbuf_reserve_out(gbuf);
	gbufs[1] = gbuf_reserve_in(gbuf);

	// Unblock waiters with GBUF_FLAG_OPEN deleted
	for (j = 0; j < ARRAY_SIZE(gbufs); j++) {
		struct gbuf_one *gbuf_one = gbufs[j];

		if (!gbuf_one || !gbuf_one->ep)
			continue;
		GBUF_DEBUG(gbuf_one->wakeup++);
		wake_up_interruptible(&gbuf_one->waitq);
		// for O_DSYNC waiters
		if (gbuf_one->dmabufs)
			for (i = 0; i < gbuf_one->items; i++)
				wake_up_interruptible(&gbuf_one->dmabufs[i].waitq);
	}
	wake_up_interruptible(&gbuf->out_poll_waitq);
	wake_up_interruptible(&gbuf->in_poll_waitq);

	do {
		struct gbuf_one *gbuf_in = gbufs[1];
		int timeout = 100;

		if (!gbuf_in || !gbuf_in->ep)
			break;

		// wait for O_DSYNC waiters being unblocked
		mutex_lock(&gbuf_in->io_mutex);
		// cancel busy requests
		// they will go to freelist in complete callback
		while ((dmabuf = gbuf_peek_dmabuf(gbuf_in,
						  &gbuf_in->busylist)) &&
		       timeout--) {
			gbuf_ep_dequeue(dmabuf);
			// give UDC chance to give back the request
			msleep(10);
		}

		if (atomic_read(&gbuf_in->busycount)) {
			pr_err("%s:%s can't clean busylist %d\n", __func__,
			       gbuf->name,
			       atomic_read(&gbuf_in->busycount));
		}

		mutex_unlock(&gbuf_in->io_mutex);
	} while(0);

	gbuf_release_one(gbufs[0]);
	gbuf_release_one(gbufs[1]);

	pr_debug("%s:%s end %lx\n", __func__, gbuf->name, gbuf->flags);

	return 0;
} /// gbuf_release

// return true if file IO operation can be done
// i.e. initialized and opened and connected
static bool can_fileio(struct gbuf *gbuf)
{
	if (test_bit(GBUF_FLAG_INIT, &gbuf->flags) &&
	    test_bit(GBUF_FLAG_OPEN, &gbuf->flags))
		return true;
	else
		return false;
}

ssize_t gbuf_read_iter(struct gbuf *gbuf, struct kiocb *kiocb, struct iov_iter *to)
{
	struct gbuf_dmabuf *dmabuf = NULL;
	int ret, qret;
	size_t chunk;
	struct gbuf_one *gbuf_out;
	size_t read;
	unsigned long flags;

//	trace_gbuf_readin(gbuf, count);

	/* no data required */
	if (iov_iter_count(to) == 0)
		return 0;

	if (!can_fileio(gbuf))
		return -EPIPE;

	gbuf_out = gbuf_reserve_out(gbuf);
	if (unlikely(!gbuf_out) || unlikely(!gbuf_out->ep)) {
		ret = -ENODEV;
		goto exit0;
	}

	GBUF_DEBUG(gbuf_out->irw++);

	/* no concurrent readers */
	ret = mutex_lock_interruptible(&gbuf_out->io_mutex);
	if (ret < 0)
		goto exit0;

	/* check fully when read individually mode */
	if (test_bit(GBUF_READ_INDIVIDUALLY, &gbuf->flags)) {
		size_t filled_size = 0;
		int found_shortpacket = 0;

		/* find short packet */
		spin_lock_irqsave(&gbuf_out->lock, flags);
		list_for_each_entry(dmabuf, &gbuf_out->filledlist, list) {
			filled_size += dmabuf->validlen;

			if (dmabuf->validlen < gbuf_out->packetlen) {
				found_shortpacket = 1;
				break;
			}
		}
		spin_unlock_irqrestore(&gbuf_out->lock, flags);

		if (!found_shortpacket) {
			if (gbuf_out->items <= gbuf_list_count(gbuf_out, &gbuf_out->filledlist)) {
				printk(KERN_ERR "gbuf.%s buffer clogged\n", gbuf_out->gbuf->name);
				ret = -EPIPE;
				goto exit1;
			}
			else {
				ret = -EAGAIN;
				goto exit1;
			}
		}
		else if (iov_iter_count(to) < filled_size) {
			ret = -EINVAL;
			goto exit1;
		}
	}

	if (kiocb->ki_filp->f_flags & O_NONBLOCK) {
		if (!(dmabuf = gbuf_peek_dmabuf(gbuf_out, &gbuf_out->filledlist))) {
			ret = -EAGAIN;
			goto exit1;
		}
	}
	else {
		int connection_failed = 0;
		/* wait for buffer being filled */
		if (wait_event_interruptible(gbuf_out->waitq,
					     (dmabuf = gbuf_peek_dmabuf(gbuf_out, &gbuf_out->filledlist)) ||
					     (connection_failed = !can_fileio(gbuf)))) {
			ret = -ERESTARTSYS;
			goto exit1;
		}
		if (connection_failed) {
			ret = -EPIPE;
			goto exit1;
		}
	}

	GBUF_DEBUG(gbuf_out->woken++);

	read = 0;
	for (;;) {
		ret = 0;

		if (!dmabuf->status) {
			chunk = min(dmabuf->validlen, iov_iter_count(to));
			if (copy_to_iter(dmabuf->req->buf, chunk, to) != chunk) {
				ret = -EFAULT;
			}
		} else {
			ret = dmabuf->status;
		}

		/* truncate for next read if copy size less than data size */
		if (!ret && (chunk < dmabuf->validlen)) {
			memmove(dmabuf->req->buf, dmabuf->req->buf + chunk, dmabuf->validlen - chunk);
			dmabuf->validlen -= chunk;
		}
		else {
			/* do it even with error */
			dmabuf->req->length = gbuf_out->packetlen;
			gbuf_move_dmabuf(dmabuf, &gbuf_out->freelist);

			qret = gbuf_ep_queue_first(gbuf_out, &gbuf_out->freelist);
			if ((qret < 0) && (qret != -EBUSY)) {
				pr_err("%s: enqueue failed %d\n", __func__, qret);
				GBUF_DEBUG(gbuf_out->error++);
				ret = qret;
				goto exit1;
			}

			if (ret) goto exit1;
		}
		read += chunk;

		if (test_bit(GBUF_READ_INDIVIDUALLY, &gbuf->flags) &&
		    (chunk < gbuf_out->packetlen))
			break;

		if ((iov_iter_count(to) <= 0) ||
		    !(dmabuf = gbuf_peek_dmabuf(gbuf_out, &gbuf_out->filledlist))) {
			break;
		}
	}

	ret = read;
 exit1:
	mutex_unlock(&gbuf_out->io_mutex);
 exit0:
	gbuf_release_one(gbuf_out);

//	trace_gbuf_readout(gbuf, dmabuf, ret);

	return ret;
} /* gbuf_read */

ssize_t gbuf_write_iter(struct gbuf *gbuf, struct kiocb *kiocb, struct iov_iter *from)
{
	struct gbuf_dmabuf *dmabuf = NULL;
	struct gbuf_dmabuf *comp_dmabuf = NULL;
	struct gbuf_dmabuf *last_dmabuf = NULL;
	int ret;
	size_t chunk;
	struct gbuf_one *gbuf_in;
	size_t written;

//	trace_gbuf_writein(gbuf, count);

	if (!can_fileio(gbuf))
		return -EPIPE;

	gbuf_in = gbuf_reserve_in(gbuf);
	if (unlikely(!gbuf_in) || unlikely(!gbuf_in->ep)) {
		ret = -ENODEV;
		goto exit0;
	}

	GBUF_DEBUG(gbuf_in->irw++);

	/* no concurrent writers */
	ret = mutex_lock_interruptible(&gbuf_in->io_mutex);
	if (ret < 0)
		goto exit0;

	/* check buffer remain when write completely mode */
	if (test_bit(GBUF_WRITE_COMPLETELY, &gbuf->flags)) {
		ssize_t write_size = iov_iter_count(from);
		if (write_size > ((gbuf_list_count(gbuf_in, &gbuf_in->freelist) - 1) * gbuf_in->packetlen)) {
			if (write_size > ((gbuf_in->items - 1) * gbuf_in->packetlen))
				ret = -EINVAL;
			else
				ret = -EAGAIN;
			goto exit1;
		}
	}

	if (kiocb->ki_filp->f_flags & O_NONBLOCK) {
		/* for complete transaction */
		if (!(comp_dmabuf = gbuf_get_dmabuf(gbuf_in, &gbuf_in->freelist))) {
			ret = -EAGAIN;
			goto exit1;
		}

		if (!(dmabuf = gbuf_peek_dmabuf(gbuf_in, &gbuf_in->freelist))) {
			ret = -EAGAIN;
			goto exit1;
		}
	}
	else {
		int connection_failed = 0;
		/* for complete transaction */
		if (wait_event_interruptible(gbuf_in->waitq,
					     (comp_dmabuf = gbuf_get_dmabuf(gbuf_in, &gbuf_in->freelist)) ||
					     (connection_failed = !can_fileio(gbuf)))) {
			ret = -ERESTARTSYS;
			goto exit1;
		}
		if (connection_failed) {
			ret = -EPIPE;
			goto exit1;
		}

		/* wait for free buffer */
		if (wait_event_interruptible(gbuf_in->waitq,
					     (dmabuf = gbuf_peek_dmabuf(gbuf_in, &gbuf_in->freelist)) ||
					     (connection_failed = !can_fileio(gbuf)))) {
			ret = -ERESTARTSYS;
			goto exit1;
		}
		if (connection_failed) {
			ret = -EPIPE;
			goto exit1;
		}
	}

	GBUF_DEBUG(gbuf_in->woken++);

	written = 0;
	for (;;) {
		chunk = min(gbuf_in->packetlen, iov_iter_count(from));
		if (copy_from_iter(dmabuf->req->buf, chunk, from) != chunk) {
			ret = -EFAULT;
			goto exit1;
		}

		/* Enqueue write request */
		dmabuf->req->length = chunk;
		gbuf_move_dmabuf(dmabuf, &gbuf_in->filledlist);
		last_dmabuf = dmabuf;
		dmabuf = NULL;

		ret = gbuf_ep_queue_first(gbuf_in, &gbuf_in->filledlist);
		if ((ret < 0) && (ret != -EBUSY)) {
			pr_err("%s: enqueue failed %d\n", __func__, ret);
			GBUF_DEBUG(gbuf_in->error++);
			goto exit1;
		}

		written += chunk;

		if ((iov_iter_count(from) <= 0) ||
		    !(dmabuf = gbuf_peek_dmabuf(gbuf_in, &gbuf_in->freelist))) {
			break;
		}
	}

	/* for complete transaction.
	   add ZLP if greater than zero and divisible by max packet size */
	if ((written > 0) && ((written % gbuf_in->ep->maxpacket_limit) == 0)) {
		comp_dmabuf->req->length = 0;
		gbuf_put_dmabuf(comp_dmabuf, &gbuf_in->filledlist);
		last_dmabuf = comp_dmabuf;
		comp_dmabuf = NULL;

		ret = gbuf_ep_queue_first(gbuf_in, &gbuf_in->filledlist);
		if ((ret < 0) && (ret != -EBUSY)) {
			pr_err("%s: enqueue failed %d\n", __func__, ret);
			GBUF_DEBUG(gbuf_in->error++);
			goto exit1;
		}
	}

	/* wait for completion if O_DSYNC is specified */
	if (kiocb->ki_filp->f_flags & O_DSYNC) {
		int connection_failed = 0;
		if (wait_event_interruptible(last_dmabuf->waitq,
					     (last_dmabuf->req->status != -EINPROGRESS) ||
					     (connection_failed = !can_fileio(gbuf)))) {
			ret = -ERESTARTSYS;
			goto exit1;
		}

		if (connection_failed) {
			ret = -EPIPE;
			goto exit1;
		}

		if (last_dmabuf->req->status) {
			ret = last_dmabuf->req->status;
			goto exit1;
		}
	}

	ret = written;
 exit1:
	if (dmabuf)
		gbuf_move_dmabuf(dmabuf, &gbuf_in->freelist);
	if (comp_dmabuf)
		gbuf_put_dmabuf(comp_dmabuf, &gbuf_in->freelist);

	mutex_unlock(&gbuf_in->io_mutex);
 exit0:
	gbuf_release_one(gbuf_in);

//	trace_gbuf_writeout(gbuf, dmabuf, ret);
	return ret;
} /* gbuf_write */

/*
 * poll support
 */
unsigned int gbuf_poll(struct gbuf *gbuf, struct file *file, poll_table *wait)
{
	unsigned int mask;
	struct gbuf_one *gbuf_out = gbuf_reserve_out(gbuf);
	struct gbuf_one *gbuf_in = gbuf_reserve_in(gbuf);

	mask = 0;

	if (gbuf_out && gbuf_out->ep) {
		mutex_lock(&gbuf_out->io_mutex);

		poll_wait(file, &gbuf->out_poll_waitq, wait);

		if (!list_empty(&gbuf_out->filledlist))
			mask |= POLLIN | POLLRDNORM;

		mutex_unlock(&gbuf_out->io_mutex);
	}

	if (gbuf_in && gbuf_in->ep) {
		mutex_lock(&gbuf_in->io_mutex);

		poll_wait(file, &gbuf->in_poll_waitq, wait);

		if ((gbuf_list_count(gbuf_in, &gbuf_in->freelist) >= 2) && can_fileio(gbuf))
			mask |= POLLOUT | POLLWRNORM;

		mutex_unlock(&gbuf_in->io_mutex);
	}

	gbuf_release_one(gbuf_out);
	gbuf_release_one(gbuf_in);

//	trace_gbuf_poll(gbuf, mask);
	/* TODO: support POLLHUP */
	return mask;
} /* gbuf_poll */

int gbuf_fsync(struct gbuf *gbuf, struct file *file, loff_t start, loff_t end,
	       int datasync)
{
	int ret = 0;
	struct gbuf_one *gbuf_in;
	const int timeout = 1000;
	int connection_failed = 0;

	if (!can_fileio(gbuf))
		return -EPIPE;

	gbuf_in = gbuf_reserve_in(gbuf);
	if (unlikely(!gbuf_in) || unlikely(!gbuf_in->ep)) {
		ret = -ENODEV;
		goto exit0;
	}

	// can't write concurrent
	ret = mutex_lock_interruptible(&gbuf_in->io_mutex);
	if (ret < 0) {
		goto exit0;
	}

	// wait for completion all with timeout
	ret = wait_event_interruptible_timeout(gbuf_in->waitq,
					       (atomic_read(&gbuf_in->busycount) <= 0) ||
					       (connection_failed = !can_fileio(gbuf)),
					       msecs_to_jiffies(timeout) );
	if (connection_failed)
		ret = -EPIPE;
	else if(0 < ret)
		ret = 0; // success
	else if(ret == 0)
		pr_warn("%s:%s wait busylist completion timed out. remain %d\n",
			__func__, gbuf->name, atomic_read(&gbuf_in->busycount));
	else
		pr_err("%s:%s wait busylist completion failed. %d\n",
		       __func__, gbuf->name, ret);

	mutex_unlock(&gbuf_in->io_mutex);
 exit0:
	gbuf_release_one(gbuf_in);
	return ret;
} /// gbuf_fsync

static void gbuf_deinit_one(struct gbuf_one *one)
{
	struct gbuf_dmabuf *dmabuf = NULL, *n = NULL;
	unsigned long flags;

	// dequeue all
	if (one && one->dmabufs) {
		spin_lock_irqsave(&one->lock, flags);
		list_for_each_entry_safe(dmabuf, n,
					 &one->busylist, list) {
			spin_unlock_irqrestore(&one->lock, flags);
			gbuf_ep_dequeue(dmabuf);
			spin_lock_irqsave(&one->lock, flags);
		}
		spin_unlock_irqrestore(&one->lock, flags);
	}
}

// make usb quiece and free usb_request
// usually call this  in function's unbind()
void gbuf_deinit(struct gbuf *gbuf)
{
	pr_debug("%s:%s start %lx\n", __func__, gbuf->name, gbuf->flags);
//	trace_gbuf_deinit(gbuf);

	if (!test_and_clear_bit(GBUF_FLAG_INIT, &gbuf->flags)) {
		pr_info("%s:%s Not initialized instance\n", __func__, gbuf->name);
		return;
	}

	gbuf_deinit_one(gbuf->out);
	gbuf_deinit_one(gbuf->in);

	GBUF_DEBUG( if (gbuf->dbgroot) debugfs_remove_recursive(gbuf->dbgroot); );

	gbuf_free_out(gbuf);
	gbuf_free_in(gbuf);

	clear_bit(GBUF_FLAG_INITIALIZING, &gbuf->flags);

	pr_debug("%s:%s end\n", __func__, gbuf->name);
	strncpy(gbuf->name, "(DEL)", sizeof(gbuf->name));
} /* gbuf_deinit */

// initialize and allocate usb_request
// usually call this in bind() or set_alt()
static int gbuf_init_one(struct gbuf_one *gbuf, struct usb_ep *ep, size_t packetlen,
			 unsigned num,
			 void (*complete)(struct usb_ep *ep,
					  struct usb_request *req))
{
	int i;
	int ret;

	BUG_ON(!ep);

	if (!packetlen)
		return -EINVAL;

	/* need at least one buffer */
	if (!num)
		return -EINVAL;

	/*
	 * ensure packetlen is a power of 2 so that
	 * any single buffer can not cross 4K boundary
	 * due to the dwc3 restriction
	 */
	if (packetlen & (packetlen-1))
		return -EINVAL;

	gbuf->packetlen = packetlen;
	gbuf->items = num;

	spin_lock_init(&gbuf->lock);
	INIT_LIST_HEAD(&gbuf->freelist);
	INIT_LIST_HEAD(&gbuf->filledlist); // OUT only
	INIT_LIST_HEAD(&gbuf->busylist);
	atomic_set(&gbuf->busycount, 0);
	mutex_init(&gbuf->io_mutex);

	gbuf->dmabufs = kzalloc(sizeof(struct gbuf_dmabuf) * num, GFP_KERNEL);
	if (!gbuf->dmabufs) {
		pr_err("%s: failed allocating memory\n", __func__);
		return -ENOMEM;
	}

	init_waitqueue_head(&gbuf->waitq);

	/* init each dmabuf */
	for (i = 0; i < num; i++) {
		struct usb_request *req;
		struct gbuf_dmabuf *dmabuf;

		dmabuf = &(gbuf->dmabufs[i]);
		dmabuf->gbuf = gbuf;
		INIT_LIST_HEAD(&dmabuf->list);
		init_waitqueue_head(&dmabuf->waitq);

		/* allocate usb_request for this chunk */
		req = usb_ep_alloc_request(ep, GFP_KERNEL);
		if (!req) {
			pr_err("%s: failed allocating usb_req\n", __func__);
			ret = -ENOMEM;
			goto rewind;
		}
		req->buf = kzalloc(gbuf->packetlen, GFP_KERNEL);
		if (!req->buf) {
			pr_err("%s: failed allocating req->buf\n", __func__);
			ret = -ENOMEM;
			goto rewind;
		}

		dmabuf->req = req;
		dmabuf->ep = ep;
		req->context = dmabuf;
		req->length = gbuf->packetlen;// OUT only?
		req->complete = complete;

		pr_debug("%s:%s dmabuf=%p req=%p iobuff=%p\n", __func__, gbuf->gbuf->name,
			 dmabuf, req, req->buf);
		/* OK insert it freelist */
		gbuf_move_dmabuf(dmabuf, &gbuf->freelist);
	} /* for each gbuf_dmabuf */

	/* this direction is initialized */
	gbuf->ep = ep;
	return 0;

 rewind:
	// TODO: rewind in safe way
	// gbuf_deinit(gbuf);
	return ret;
} /* gbuf_init_one */

/*
 * allocate resources and initialize
 * gbuf: struct gbuf to be initialized
 * name: ID name for this instance. Just for debugging
 * iep: EP for IN. Can be NULL if OUT only function
 * oep: EP for OUT. Can be NULL if IN only function
 * ipacketlen: data size of transfer unit in bytes
 * opacketlen: "
 * inum: number of transfer unit to be allocated
 * onum: "
 */
int gbuf_init(struct gbuf *gbuf, const char *name,
	      struct usb_ep *oep, size_t opacketlen, unsigned int onum, gbuf_complete_cb ocomplete,
	      struct usb_ep *iep, size_t ipacketlen, unsigned int inum, gbuf_complete_cb icomplete,
	      int read_individually, int write_completely)
{
	int ret = 0;
#ifdef CONFIG_GBUF_DEBUG
	char dbgname[256];
	struct dentry *file;
#endif

	BUG_ON(!gbuf);

//	trace_gbuf_init(gbuf, name,
//			 oep, opacketlen, onum, iep, ipacketlen, inum);

	pr_debug("%s:%s start %lx\n", __func__, name, gbuf->flags);
	if (iep)
		pr_debug("%s:%s iplen=%zd num=%d\n", __func__, name, ipacketlen, inum);
	if (oep)
		pr_debug("%s:%s oplen=%zd num=%d\n", __func__, name, opacketlen, onum);

	// init twice?
	if (test_and_set_bit(GBUF_FLAG_INITIALIZING, &gbuf->flags)) {
		pr_err("%s:%s already initialized instance\n", __func__, gbuf->name);
		return -EBUSY;
	}

	// only once
	if (!test_and_set_bit(GBUF_FLAG_CONSTRUCTED, &gbuf->flags)) {
		init_waitqueue_head(&gbuf->in_poll_waitq);
		init_waitqueue_head(&gbuf->out_poll_waitq);
	}

	if (name)
		strncpy(gbuf->name, name, sizeof(gbuf->name) - 1);

	if (oep) {
		struct gbuf_one *gbuf_out = gbuf_alloc_out(gbuf);
		if (gbuf_out) {
			if (read_individually)
				set_bit(GBUF_READ_INDIVIDUALLY, &gbuf->flags);

			gbuf_out->gbuf = gbuf;
			ret = gbuf_init_one(gbuf_out, oep, opacketlen, onum, gbuf_ep_complete_out);
			gbuf_out->complete = ocomplete;
		}
	}
	if (iep && !ret) {
		struct gbuf_one *gbuf_in = gbuf_alloc_in(gbuf);
		if (gbuf_in) {
			if (write_completely)
				set_bit(GBUF_WRITE_COMPLETELY, &gbuf->flags);

			gbuf_in->gbuf = gbuf;
			/* num + 1 for complete transaction */
			ret = gbuf_init_one(gbuf_in, iep, ipacketlen, inum + 1, gbuf_ep_complete_in);
			gbuf_in->complete = icomplete;
		}
	}

	if (ret) {
		pr_err("%s:%s gbuf init failed %d\n", __func__, gbuf->name, ret);

		gbuf_free_out(gbuf);
		gbuf_free_in(gbuf);

		clear_bit(GBUF_FLAG_INITIALIZING, &gbuf->flags);
	}
	else {
		set_bit(GBUF_FLAG_INIT, &gbuf->flags);
	}

#ifdef CONFIG_GBUF_DEBUG
	// debugfs
	snprintf(dbgname, sizeof(dbgname), "gbuf.%s", gbuf->name);
	gbuf->dbgroot = debugfs_create_dir(dbgname, NULL);
	if (gbuf->dbgroot) {
		file = debugfs_create_file("queuestat", S_IRUGO | S_IWUSR,
					   gbuf->dbgroot, gbuf,
					   &gbuf_queuestat_fops);
		(void)file;
	}
#endif
	pr_debug("%s:%s ended with %d\n", __func__, gbuf->name, ret);
	return ret;
} /* gbuf_init */

#define CREATE_TRACE_POINTS
//#include "gbuf_trace.h"
