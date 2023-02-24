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
#include <linux/kthread.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/console.h>
#include <linux/serial_core.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <crypto/hash.h>
#include <crypto/sha.h>
#include <linux/mmc/core.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/host.h>
#include <linux/genhd.h>
#include <linux/scatterlist.h>
#include <linux/sched.h>

MODULE_AUTHOR("Sony Interactive Entertainment Inc.");
MODULE_DESCRIPTION("log driver");
MODULE_LICENSE("GPL");

#define LOGDRV_SCP_ALL_BUFFER_SIZE (256 * 1024)
#define LOGDRV_SCP_LOG_LEN_ON_CRASH 512
#define LOGDRV_THREAD_INTERVAL_MS 500
#define LOGDRV_KMSGDEV_PATH "/dev/kmsg"
#define LOGDRV_SCPDEV_PATH "/dev/scp"
#define LOGDRV_SCPCRASHDEV_PATH "/dev/scpcrash"
#define LOGDRV_THREAD_TTY_NAME "logdrv tty"

#define LOGDRV_CRASHREPORT_PATH "/dev/mmcblk0"
#define LOGDRV_CRASHREPORT_DIGEST_SIZE 32
#define LOGDRV_CRASHREPORT_LENGTH_SIZE 4
#define LOGDRV_CRASHREPORT_BLOCK_HEAD 0x120800 /* /dev/mmcblk0p10 */
#define LOGDRV_EMMC_BLOCK_SIZE 512
#define LOGDRV_EMMC_PART_SIZE (64 * 1024 * 1024)

static struct task_struct *logdrv_thread_tty;
static bool log_output_console = false;

extern struct console *console_drivers;
extern void (*em_dump_here)(void);
extern int use_custom_log;


static ssize_t logdrv_console_output(const char *str, size_t str_len)
{
	struct console *con;
	size_t i;
	int line;
	struct tty_driver *ttydrv;
	struct tty_struct *tty;

	if (!console_drivers)
		return -ENODEV;

	for_each_console(con)
	{
		if (!(con->flags & CON_ENABLED)) {
			continue;
		}
		if (!con->write) {
			continue;
		}
		if (!cpu_online(smp_processor_id())
		    && !(con->flags & CON_ANYTIME)) {
			continue;
		}
		if (con->flags & CON_EXTENDED) {
			continue;
		}

		if (log_output_console) {
			// USE CONSOLE DRIVER
			for (i = str_len; i > 0; i--) {
				con->write(con, str, 1);
				str++;
			}
		} else {
			// USE TTY DRIVER
			ttydrv = con->device(con, &line);
			if (!ttydrv) {
				continue;
			}
			tty = *(ttydrv->ttys);
			if (!tty) {
				continue;
			}
			for (i = str_len; i > 0; i--) {
				if (*str == '\n') {
					do {
						if (ttydrv->ops->put_char(tty, '\r')) break;
						udelay(1);
					} while (1);
				}
				do {
					if (ttydrv->ops->put_char(tty, *str)) break;
					udelay(1);
				} while (1);
				str++;
			}
			ttydrv->ops->flush_chars(tty);
		}

	}

	return str_len;
}

static int logdrv_parse_msg(char *buf, size_t buf_sz, char *dst, size_t dst_sz,
			    bool *is_print)
{
	ssize_t ret;
	char *p;
	u64 seq, ts_usec, ts_msec;
	uint32_t type;
	char continue_flag;
	ssize_t msg_sz;
	bool user;

	/* refer: msg_print_ext_header() */
	ret = sscanf(buf, "%u,%llu,%llu,%c;", &type, &seq, &ts_usec,
		     &continue_flag);
	if (ret < 0) {
		return -EINVAL;
	}

	p = memchr(buf, ';', strnlen(buf, buf_sz));
	if (p == NULL) {
		return -EINVAL;
	}
	p++;

	BUG_ON(p < buf || p >= buf + buf_sz);

	/* facility / log level checks */
	user = !!(type & 0x8);
	if (user || ((type & 0x7) <= console_loglevel)) {
		*is_print = true;
	} else {
		*is_print = false;
	}

	ts_msec = ts_usec / 1000;
	if (ts_msec / 1000 > 99999) {
		if (user) {
			msg_sz = snprintf(dst, dst_sz, "%9d %s",
					  (int)(ts_msec / 1000), p);
		} else {
			msg_sz = snprintf(dst, dst_sz, "%9d [  KERNEL  ] %s",
					  (int)(ts_msec / 1000), p);
		}
	} else {
		if (user) {
			msg_sz = snprintf(
				dst, dst_sz, "%5d.%03d %s",
				(int)((ts_msec - (ts_msec % 1000)) / 1000),
				(int)(ts_msec % 1000), p);
		} else {
			msg_sz = snprintf(
				dst, dst_sz, "%5d.%03d [  KERNEL  ] %s",
				(int)((ts_msec - (ts_msec % 1000)) / 1000),
				(int)(ts_msec % 1000), p);
		}
	}

	BUG_ON(msg_sz > dst_sz);

	return msg_sz;
}

static ssize_t logdrv_read_kmsg(struct file **pfilp, char *buf, size_t bufsize)
{
	mm_segment_t old_fs;
	ssize_t ret;

	BUG_ON(!pfilp);
	BUG_ON(!buf);

	if (*pfilp == NULL) {
		*pfilp = filp_open(LOGDRV_KMSGDEV_PATH, O_RDWR, 0);
		if (IS_ERR(*pfilp)) {
			ret = PTR_ERR(*pfilp);
			*pfilp = NULL;
			return ret;
		}
		/* When panic occurs, we do not have the triger of wake-up (log_wait) at devkmsg_read().
		 * So we add O_NONBLOCK to flags. */
		(*pfilp)->f_flags |= O_NONBLOCK;
	}

	memset(buf, 0, bufsize);

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	ret = __vfs_read(*pfilp, (void *)buf, bufsize - 1, &(*pfilp)->f_pos);
	if (ret <= 0) {
		static const char *errmsg = "debug print buffer OVERFLOWed!\n";
		if (ret == -EPIPE) {
			filp_close(*pfilp, 0);
			*pfilp = NULL;
			logdrv_console_output(errmsg, strlen(errmsg));
		}
	}
	set_fs(old_fs);

	return ret;
}

static int logdrv_flush(void)
{
	static struct file *filp = NULL;
	static char readbuf[CONSOLE_EXT_LOG_MAX];
	static char parsebuf[CONSOLE_EXT_LOG_MAX+512];

	bool is_print;
	ssize_t ret;
	static unsigned long flushlock = 0;
#define FLAG_FLUSHLOCK 0

	if (test_and_set_bit(FLAG_FLUSHLOCK, &flushlock)) return -EAGAIN;
	while (true) {

		ret = logdrv_read_kmsg(&filp, readbuf, sizeof(readbuf));
		if (ret <= 0) {
			break;
		}

		ret = logdrv_parse_msg(readbuf, ret, parsebuf, sizeof(parsebuf) - 1,
							   &is_print);
		if (ret < 0 || !is_print) {
			continue;
		}
		logdrv_console_output(parsebuf, ret);
	}
	clear_bit(FLAG_FLUSHLOCK ,&flushlock);

	return 0;
}

static void logdrv_show_maps(void)
{
	char str[sizeof("0x0123456789abcdef-0x0123456789abcdef RWX 0x0123456789abcdef 0123456789abcdef0123456789abcdef\n")];
	const char *name;
	int r;
	struct vm_area_struct *vm;

	if (!current || !current->mm || !current->mm->mmap) {
		printk(KERN_ERR "map is null\n");
		return;
	}

	printk(KERN_ERR "show maps\n");
	printk(KERN_ERR "start              end                flg offset             name\n");

	for (vm = current->mm->mmap; vm; vm = vm->vm_next) {
		if (vm->vm_file && vm->vm_file->f_path.dentry &&
			vm->vm_file->f_path.dentry->d_name.name) {
			name = vm->vm_file->f_path.dentry->d_name.name;
		} else {
			name = " ";
		}
		r = snprintf(str, sizeof(str),
		"0x%016lx-0x%016lx %c%c%c 0x%016lx %s\n",
			vm->vm_start, vm->vm_end,
			(vm->vm_flags & VM_READ) ? 'R' : '-',
			(vm->vm_flags & VM_WRITE) ? 'W' : '-',
			(vm->vm_flags & VM_EXEC) ? 'X' : '-',
			vm->vm_pgoff * PAGE_SIZE,
			name);
		if (r <= 0)
		   return;
		printk(KERN_ERR "%s", str);
   }

}

static void logdrv_stop_threads(void)
{
	struct task_struct *t;
	static const int signal = SIGSTOP;
	static const int priv = 1;
	for_each_process(t) {
		if (current == t)
			continue;
		send_sig(signal, t, priv);
	}
}

static void logdrv_dump_user_mem(unsigned long bottom, unsigned long top)
{
	unsigned long first;
	mm_segment_t old_fs = get_fs();
	set_fs(KERNEL_DS);

#define DUMP_BYTES 8
#define DUMP_LINE_BYTES (DUMP_BYTES * 4)
	for (first = (bottom & ~(DUMP_LINE_BYTES-1)); first < top; first += DUMP_LINE_BYTES) {
		unsigned long p;
		char str[(DUMP_LINE_BYTES * 2) + (DUMP_LINE_BYTES / DUMP_BYTES) + 1]; /* 1char2byte 4space \0 */
		int istr = 0;

		for (p = first; p < (first + DUMP_LINE_BYTES); p += DUMP_BYTES) {
			if (p >= (bottom-(DUMP_BYTES-1)) && p < top) {
				unsigned long val;

				if (get_user(val, (unsigned long *)p) == 0)
					istr += snprintf(str+istr, sizeof(str)-istr, " %016lx", val);
				else
					istr += snprintf(str+istr, sizeof(str)-istr, " ????????????????");
			}
			else {
				istr += snprintf(str+istr, sizeof(str)-istr, "                 ");
			}
		}

		str[sizeof(str) - 1] = '\0';
		printk(KERN_ERR "%04lx:%s\n", first & 0xffff, str);
	}

	set_fs(old_fs);
}
static void logdrv_dump_user_sp(void)
{
	unsigned long bottom, top;
	mm_segment_t old_fs;
	struct pt_regs *current_regs;
	unsigned long p;

	current_regs = task_pt_regs(current);

	if (!user_mode(current_regs))
		return;

	bottom = user_stack_pointer(current_regs);
	top = PAGE_ALIGN(bottom) + PAGE_SIZE;

	printk(KERN_ERR "[%s] around user stack (0x%016lx to 0x%016lx)\n",
	       (current->comm)?:"",
	       bottom, top);

	logdrv_dump_user_mem(bottom, top);

	if (!current->mm || !current->mm->mmap) {
		return;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	for (p = bottom; p < top; p += sizeof(void *)) {
		unsigned long val;

		if (get_user(val, (unsigned long *)p) == 0) {
			struct vm_area_struct *vm;

			if ((val >= bottom) && (val < top)) {
				continue; /* skip if address on stack */
			}

			for (vm = current->mm->mmap; vm; vm = vm->vm_next) {
				if ((val >= vm->vm_start) && (val < vm->vm_end)) {
					if ((vm->vm_flags & VM_READ) &&
					    (vm->vm_flags & VM_WRITE) ){
						printk(KERN_ERR "[<%016lx>]\n", val);
						logdrv_dump_user_mem(val, val + 256);
					}
					break;
				}
			}
		}
	}

	set_fs(old_fs);
}

static int logdrv_dump_mem_appendix(char *dst, int dst_sz, u64 p, bool ishead)
{
	u64 val;
	mm_segment_t old_fs = get_fs();
	set_fs(KERNEL_DS);

	if (get_user(val, (u64*)p) != 0) {
		val = 0;
	}

	set_fs(old_fs);

	return snprintf(dst, dst_sz,
			"%s%llu"
			, (ishead) ? "" : ","
			, val);
}
static int logdrv_read_appendix(char *dst, int dst_sz)
{
	int ret = 0;
	enum STATE {
		APPEND_DUMP_STACK_PREPARE,
		APPEND_DUMP_STACK,
		APPEND_DUMP_MEM_PREPARE,
		APPEND_DUMP_MEM,
		APPEND_DUMP_MAP_PREPARE,
		APPEND_DUMP_MAP,
		APPEND_LOG_TAG,
	};
	static enum STATE state = APPEND_DUMP_STACK_PREPARE;
	static u64 bottom = 0, top = 0, memstart = 0, memend = 0, pdump = 0, pmem = 0;
	static bool ismemhead = true;
	static struct vm_area_struct *pvm = NULL;

	switch(state) {
	case APPEND_DUMP_STACK_PREPARE:
	{
		int i;
		struct pt_regs *current_regs;

		if (!user_mode(task_pt_regs(current))) {
			state = APPEND_LOG_TAG;
			break;
		}

		// reg
		current_regs = task_pt_regs(current);

		ret += snprintf(dst+ret, dst_sz-ret,
				"\n[dump]\n"
				"{"
				"\"time\":%llu,"
				"\"task\":\"%s\","
				"\"reg\":{"
				"\"pc\":%llu,"
				"\"lr\":%llu,"
				"\"sp\":%llu,"
				, local_clock() / 1000000
				, (current->comm)?:""
				, current_regs->pc, current_regs->regs[30], current_regs->sp);

		for (i = 29; i >= 0; i--) {
			ret += snprintf(dst+ret, dst_sz-ret,
					"\"x%d\":%llu,", i, current_regs->regs[i]);
		}

		ret += snprintf(dst+ret, dst_sz-ret,
				"\"pstate\":%llu"
				"},"
				, current_regs->pstate);

		bottom = user_stack_pointer(current_regs);
		top = PAGE_ALIGN(bottom) + PAGE_SIZE;
		pdump = pmem = bottom;

		ret += snprintf(dst+ret, dst_sz-ret,
				"\"stack\":{"
				"\"addr\":%llu,"
				"\"size\":%llu,"
				"\"data\":["
				, bottom, top - bottom);

		state++;
	}
	/* fall through */
	case APPEND_DUMP_STACK:
		ret += logdrv_dump_mem_appendix(dst+ret, dst_sz-ret, pdump, (pdump == bottom));
		pdump += sizeof(void*);
		if (pdump >= top) {
			ret += snprintf(dst+ret, dst_sz-ret, "]}");
			state++;
		}

		break;
	case APPEND_DUMP_MEM_PREPARE:
		if (pmem < top) {
			u64 val;
			bool valid_val;

			mm_segment_t old_fs = get_fs();
			set_fs(KERNEL_DS);
			valid_val = (get_user(val, (u64*)pmem) == 0);
			set_fs(old_fs);

			memstart = 0;
			if ((valid_val) && ((val < bottom) || (val >= top))) {
				struct vm_area_struct *vm;

				for (vm = current->mm->mmap; vm; vm = vm->vm_next) {
					if ((val >= vm->vm_start) && (val < vm->vm_end)) {
						if ((vm->vm_flags & VM_READ) &&
						    (vm->vm_flags & VM_WRITE) ){
							memstart = val;
						}
						break;
					}
				}
			}


			if (memstart != 0) {
				memend = memstart + 256;
				pdump = memstart;

				if (ismemhead) {
					ret += snprintf(dst+ret, dst_sz-ret,
							",\"mem\":[");
				}
				ret += snprintf(dst+ret, dst_sz-ret,
						"%s{"
						"\"addr\":%llu,"
						"\"size\":%llu,"
						"\"data\":["
						, (ismemhead) ? "" : ","
						, memstart, memend - memstart);
				ismemhead = false;

				state++;
			}
			pmem += sizeof(void*);
		}
		else {
			if (!ismemhead) {
				ret += snprintf(dst+ret, dst_sz-ret, "]");
			}
			state = APPEND_DUMP_MAP_PREPARE;
		}

		break;
	case APPEND_DUMP_MEM:
		ret += logdrv_dump_mem_appendix(dst+ret, dst_sz-ret, pdump, (pdump == memstart));
		pdump += sizeof(void*);
		if (pdump >= memend) {
			ret += snprintf(dst+ret, dst_sz-ret, "]}");
			state--;
		}

		break;
	case APPEND_DUMP_MAP_PREPARE:
		ret += snprintf(dst+ret, dst_sz-ret,
				",\"map\":[");
		if (current && current->mm) {
			pvm = current->mm->mmap;
		}

		state++;
		/* fall through */
	case APPEND_DUMP_MAP:
		if (pvm) {
			const char *name;
			if (pvm->vm_file && pvm->vm_file->f_path.dentry &&
			    pvm->vm_file->f_path.dentry->d_name.name) {
				name = pvm->vm_file->f_path.dentry->d_name.name;
			} else {
				name = "";
			}
			ret += snprintf(dst+ret, dst_sz-ret,
					"%s{"
					"\"start\":%lu,"
					"\"end\":%lu,"
					"\"R\":%s,"
					"\"W\":%s,"
					"\"X\":%s,"
					"\"offset\":%lu,"
					"\"name\":\"%s\""
					"}",
					(pvm == current->mm->mmap) ? "" : ",",
					pvm->vm_start, pvm->vm_end,
					(pvm->vm_flags & VM_READ) ? "true" : "false",
					(pvm->vm_flags & VM_WRITE) ? "true" : "false",
					(pvm->vm_flags & VM_EXEC) ? "true" : "false",
					pvm->vm_pgoff * PAGE_SIZE,
					name);
			pvm = pvm->vm_next;
		}
		else {
			ret += snprintf(dst+ret, dst_sz-ret, "]}");
			state++;
		}

		break;
	case APPEND_LOG_TAG:
		ret += snprintf(dst+ret, dst_sz-ret, "\n[log]\n");

		state++;
		break;
	default:
		ret = -ENODATA;
		break;
	}

	BUG_ON(ret > dst_sz);
	return ret;
}

static void logdrv_emmc_resume(struct mmc_host *host)
{
	struct device *dev;

	dev = mmc_dev(host);
	if (dev && dev->driver && dev->driver->pm && dev->driver->pm->runtime_resume) {
		dev->driver->pm->runtime_resume(dev);
	}
}

static struct mmc_host *logdrv_emmc_get_host(void)
{
	static struct mmc_host *host = NULL;

	if (host == NULL) {
		struct block_device *bdev;
		struct device *diskdev = NULL;
		struct mmc_card *card = NULL;
		fmode_t mode = FMODE_WRITE|FMODE_NDELAY;

		bdev = blkdev_get_by_path(LOGDRV_CRASHREPORT_PATH, mode, NULL);
		if (IS_ERR(bdev)) {
			return NULL;
		}

		if (!(bdev->bd_disk)) {
			blkdev_put(bdev, mode);
			return NULL;
		}

		diskdev = disk_to_dev(bdev->bd_disk);
		if (!(diskdev->parent)) {
			blkdev_put(bdev, mode);
			return NULL;
		}

		card = mmc_dev_to_card(diskdev->parent);
		if (!(card->host)) {
			blkdev_put(bdev, mode);
			return NULL;
		}

		host = card->host;

		logdrv_emmc_resume(host); // enable MSDC clk
	}

	return host;
}

static void logdrv_emmc_write_done(struct mmc_request *mrq)
{
	complete(&mrq->completion);
}
static int logdrv_emmc_req_wait(struct mmc_host *host, struct mmc_request *mrq)
{
	int retry;

	mrq->host = host;
	mrq->done = logdrv_emmc_write_done;
	mrq->cmd->mrq = mrq;
	if (mrq->data) {
		mrq->cmd->data = mrq->data;
		mrq->data->mrq = mrq;
	}

	init_completion(&mrq->completion);

	host->ops->request(host, mrq);

	for (retry = 500; retry > 0; retry--) {
		if (try_wait_for_completion(&mrq->completion)) break;
		mdelay(1);
	}

	return (retry <= 0)? -ETIME: mrq->cmd->error;
}

static int logdrv_emmc_write_single_block(const void *buf, off_t offset)
{
	int ret;
	struct mmc_host *host;
	struct mmc_request mrq;
	struct mmc_command cmd;
	struct mmc_data data;
	struct scatterlist sg;
	int retry;

	if (((offset % LOGDRV_EMMC_BLOCK_SIZE) != 0) ||
	    (offset >= LOGDRV_EMMC_PART_SIZE) ){
		return -EINVAL;
	}

	host = logdrv_emmc_get_host();
	if (host == NULL) {
		return -ENXIO;
	}

	memset(&mrq, 0, sizeof(mrq));
	memset(&cmd, 0, sizeof(cmd));
	memset(&data, 0, sizeof(data));

	mrq.cmd = &cmd;
	mrq.data = &data;

	mrq.cmd->opcode = MMC_WRITE_BLOCK;
	mrq.cmd->arg = LOGDRV_CRASHREPORT_BLOCK_HEAD + (offset / LOGDRV_EMMC_BLOCK_SIZE);
	mrq.cmd->flags = MMC_RSP_R1 | MMC_CMD_ADTC;
	mrq.cmd->error = 0;

	mrq.data->blksz = LOGDRV_EMMC_BLOCK_SIZE;
	mrq.data->blocks = 1;
	mrq.data->flags = MMC_DATA_WRITE;
	mrq.data->error = 0;
	sg_init_one(&sg, buf, LOGDRV_EMMC_BLOCK_SIZE);
	mrq.data->sg = &sg;
	mrq.data->sg_len = 1;

	mmc_set_data_timeout(mrq.data, host->card);

	if (host->ops->pre_req)
		host->ops->pre_req(host, &mrq, 1);

	ret = logdrv_emmc_req_wait(host, &mrq);
	if (ret) {
		return ret;
	}

	if (host->ops->post_req)
		host->ops->post_req(host, &mrq, 0);

	ret = -ETIME;
	for (retry = 500; retry > 0; retry--) {
		memset(&mrq, 0, sizeof(mrq));
		memset(&cmd, 0, sizeof(cmd));

		mrq.cmd = &cmd;

		mrq.cmd->opcode = MMC_SEND_STATUS;
		mrq.cmd->arg = host->card->rca << 16;
		mrq.cmd->flags = MMC_RSP_R1 | MMC_CMD_AC;
		mrq.cmd->error = 0;

		ret = logdrv_emmc_req_wait(host, &mrq);
		if (ret) {
			continue;
		}

		if ((mrq.cmd->resp[0] & R1_READY_FOR_DATA) &&
		    (R1_CURRENT_STATE(mrq.cmd->resp[0]) != R1_STATE_PRG)) {
			ret = 0;
			break;
		}
	}

	return ret;
}

static void logdrv_crash_report(void)
{
	struct file *filp = NULL;
	bool is_print;
	static char tmp[CONSOLE_EXT_LOG_MAX];
	size_t f_pos;
	size_t sizeof_header = LOGDRV_CRASHREPORT_DIGEST_SIZE + LOGDRV_CRASHREPORT_LENGTH_SIZE;
	uint8_t *buffer;
	size_t b_pos;
	ssize_t msg_len;
	struct crypto_shash *sha256;
	uint8_t __sha256desc[sizeof(struct crypto_shash) + sizeof(struct sha256_state)];
	struct shash_desc *sha256desc = (struct shash_desc *)__sha256desc;
	ssize_t ret;
	bool log_appendix = true;

	buffer = (uint8_t*)__get_free_page(GFP_KERNEL);
	if (!buffer) {
		goto page_alloc_failed;
	}
	memset(buffer, 0, PAGE_SIZE);

	sha256 = crypto_alloc_shash("sha256", 0, 0);
	if (IS_ERR(sha256)) {
		goto alloc_shash_failed;
	}
	sha256desc->tfm = sha256;
	sha256desc->flags = 0;
	ret = crypto_shash_init(sha256desc);
	if (ret < 0) {
		goto shash_failed;
	}

	f_pos = 0;
	b_pos = sizeof_header;
	while (true) {
		int buffer_free = PAGE_SIZE - b_pos;
		if (log_appendix) {
			msg_len = logdrv_read_appendix(&(buffer[b_pos]), buffer_free);
			if (msg_len < 0) {
				log_appendix = false;
			}
		}
		else {
			ret = logdrv_read_kmsg(&filp, tmp, sizeof(tmp));
			if (ret <= 0) {
				break;
			}

			msg_len = logdrv_parse_msg(tmp, strnlen(tmp, sizeof(tmp) - 1),
						   &(buffer[b_pos]), buffer_free - 1,
						   &is_print);
		}
		if (msg_len < 0) {
			continue;
		}

		ret = crypto_shash_update(sha256desc, &(buffer[b_pos]), msg_len);
		if (ret < 0) {
			goto shash_failed;
		}

		b_pos += msg_len;
		while (b_pos >= (LOGDRV_EMMC_BLOCK_SIZE*2)) {
			f_pos += LOGDRV_EMMC_BLOCK_SIZE;
			ret = logdrv_emmc_write_single_block(&(buffer[LOGDRV_EMMC_BLOCK_SIZE]), f_pos);
			if (ret < 0) {
				goto write_failed;
			}
			memmove(&(buffer[LOGDRV_EMMC_BLOCK_SIZE]),
				&(buffer[LOGDRV_EMMC_BLOCK_SIZE*2]),
				PAGE_SIZE - (LOGDRV_EMMC_BLOCK_SIZE*2));

			b_pos -= LOGDRV_EMMC_BLOCK_SIZE;
			memset(&(buffer[b_pos]), 0, PAGE_SIZE - b_pos);
		}
	}

	/* write remain */
	if (b_pos > LOGDRV_EMMC_BLOCK_SIZE) {
		f_pos += LOGDRV_EMMC_BLOCK_SIZE;
		ret = logdrv_emmc_write_single_block(&(buffer[LOGDRV_EMMC_BLOCK_SIZE]), f_pos);
		if (ret < 0) {
			goto write_failed;
		}

		b_pos -= LOGDRV_EMMC_BLOCK_SIZE;
	}

	ret = crypto_shash_final(sha256desc, buffer);
	if (ret < 0) {
		goto shash_failed;
	}
	*(uint32_t*)&(buffer[LOGDRV_CRASHREPORT_DIGEST_SIZE]) = f_pos + b_pos - sizeof_header;

	/* write first block */
	ret = logdrv_emmc_write_single_block(buffer, 0);
	if (ret < 0) {
		goto write_failed;
	}

shash_failed:
write_failed:
	crypto_free_shash(sha256);
alloc_shash_failed:
	free_page((unsigned long)buffer);
page_alloc_failed:
	return;
}

static char scpbuf[LOGDRV_SCP_ALL_BUFFER_SIZE];
static void log_dump_here(void)
{
	mm_segment_t old_fs = get_fs();
	struct file *filp;
	ssize_t ret;
	char *p;
	static const char *msg = "\n\n ========== LOG DUMP ==========\n\n";

	logdrv_stop_threads();

	log_output_console = true;
	logdrv_console_output(msg, strlen(msg));

	filp = filp_open(LOGDRV_SCPDEV_PATH, O_RDWR, 0);
	if (!IS_ERR(filp)) {
		set_fs(KERNEL_DS);
		ret = __vfs_read(filp, (void *)scpbuf, sizeof(scpbuf) - 1,
				 &filp->f_pos);
		set_fs(old_fs);

		if (ret > 0) {
			if (ret > LOGDRV_SCP_LOG_LEN_ON_CRASH) {
				p = &scpbuf[ret - LOGDRV_SCP_LOG_LEN_ON_CRASH];
			} else {
				p = &scpbuf[0];
			}
			logdrv_console_output(p, LOGDRV_SCP_LOG_LEN_ON_CRASH);
		}
	}

	logdrv_dump_user_sp();

	logdrv_show_maps();

	while (logdrv_flush()) cpu_relax();

	logdrv_crash_report();

	while (logdrv_flush()) cpu_relax();

	/* change to the default behavior of console out for supporting sysreq. */
	use_custom_log = 0;
}

static int logdrv_thread_func(void *arg)
{
	(void)arg;

	while (!kthread_should_stop()) {
		logdrv_flush();
		msleep_interruptible(LOGDRV_THREAD_INTERVAL_MS);
	}
	return 0;
}

static int __init logdrv_module_init(void)
{
	logdrv_thread_tty =
		kthread_create(logdrv_thread_func, NULL,
			       LOGDRV_THREAD_TTY_NAME);
	if (IS_ERR(logdrv_thread_tty)) {
		BUG_ON(1);
	}

	if (!wake_up_process(logdrv_thread_tty)) {
		BUG_ON(1);
	}

	em_dump_here = log_dump_here;

	use_custom_log = 1;

	return 0;
}

static void __exit logdrv_module_cleanup(void)
{
	kthread_stop(logdrv_thread_tty);
	em_dump_here = NULL;
	use_custom_log = 0;
}

module_init(logdrv_module_init);
module_exit(logdrv_module_cleanup);
