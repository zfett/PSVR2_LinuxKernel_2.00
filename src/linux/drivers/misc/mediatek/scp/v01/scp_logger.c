/*
* Copyright (C) 2011-2015 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or
* modify it under the terms of the
* GNU General Public License version 2 as published by the Free
* Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/


#include <linux/fs.h>           /* needed by file_operations* */
#include <linux/miscdevice.h>   /* needed by miscdevice* */
#include <linux/device.h>       /* needed by device_* */
#include <linux/uaccess.h>      /* needed by copy_to_user */
#include <linux/fs.h>           /* needed by file_operations* */
#include <linux/slab.h>         /* needed by kmalloc */
#include <linux/poll.h>         /* needed by poll */
#include <linux/vmalloc.h>      /* needed by vmalloc */
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/io.h>

#include "scp_ipi.h"
#include "scp_helper.h"
#include "scp_feature_define.h"

static ssize_t scp_A_log_if_read(
	struct file *file, char __user *data, size_t len, loff_t *ppos)
{
	ssize_t rpos = (ssize_t)file->private_data;
	int wrapped_start = rpos & ~(SCP_LOG_SIZE-1);
	char *src = SCP_LOG_TOP;
	ssize_t ret;
	ssize_t leftsize = SCP_LOG_SIZE;
	loff_t pos = *ppos;

	if (pos >= SCP_LOG_SIZE) {
		return 0;
	}

	rpos -= wrapped_start;
	if (!wrapped_start) {
		leftsize = rpos;
		rpos = 0;
	}
	leftsize -= pos;
	rpos += pos;

	if (len >= leftsize) {
		len = leftsize;
	}
	ret = len;

	if (rpos + len > SCP_LOG_SIZE) {
		// wrap
		int wrap_length = SCP_LOG_SIZE - rpos;
		if (copy_to_user(data, src + rpos, wrap_length) != 0) {
			return -EFAULT;
		}
		len -= wrap_length;
		data += wrap_length;
		rpos = 0;
	}
	if (copy_to_user(data, src + rpos, len) != 0) {
		return -EFAULT;
	}
	*ppos += ret;

	return ret;
}

static int scp_A_log_if_open(struct inode *inode, struct file *file)
{
	/*pr_debug("[SCP A] scp_A_log_if_open\n");*/
	file->private_data = (void*)(uintptr_t)SCP_A_DEBUG_INFO->log_pos_info;
	return 0;
}

static ssize_t scp_A_crashinfo_if_read(
	struct file *file, char __user *data, size_t len, loff_t *ppos)
{
	size_t dumpsize = SCP_A_DEBUG_INFO->size_regdump;
	char *src = SCP_CRASHINFO_TOP;
	loff_t pos = *ppos;

	if (dumpsize > SCP_CRASHINFO_SIZE) {
		dumpsize = SCP_CRASHINFO_SIZE;
	}

	if (pos >= dumpsize) {
		return 0;
	}

	dumpsize -= pos;
	if (len > dumpsize) {
		len = dumpsize;
	}

	if (copy_to_user(data, src + pos, len) != 0) {
		return -EFAULT;
	}
	*ppos += len;

	return len;
}

static int scp_A_crashinfo_if_open(struct inode *inode, struct file *file)
{
	return 0;
}

const struct file_operations scp_A_log_file_ops = {
	.owner = THIS_MODULE,
	.read = scp_A_log_if_read,
	.open = scp_A_log_if_open,
};

const struct file_operations scp_A_crashinfo_file_ops = {
	.owner = THIS_MODULE,
	.read = scp_A_crashinfo_if_read,
	.open = scp_A_crashinfo_if_open,
};
