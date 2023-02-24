/*
 * eMMC-based transactional key-value store
 *
 * Copyright (C) 2010 Google, Inc.
 * Author: Eugene Surovegin <es@google.com>
 *
 * Copyright (C) 2011, Marvell International Ltd.
 * Author: Qingwei Huang <huangqw@marvell.com>
 *
 * Copyright (C) 2020 Sony Interactive Entertainment Inc.
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

#include <linux/crc32.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/slab.h>

#include <linux/scatterlist.h>
#include <linux/syscalls.h>
#include <linux/delay.h>

#include <flash_ts.h>

#define ENABLE_MISC_DEVICE 0
#define DRV_NAME        	"fts"
#define DRV_VERSION     	"0.999"
#define DRV_DESC        	"eMMC-based transactional key-value storage"
#define RETRY_OPEN_MAX		200

MODULE_DESCRIPTION(DRV_DESC);
MODULE_VERSION(DRV_VERSION);
MODULE_AUTHOR("Qingwei Huang <huangqw@marvell.com>");
MODULE_LICENSE("GPL");

static unsigned int fts_major = FTS_MAJOR;
static int dev_id = 0;
module_param(dev_id, int, 0444);
MODULE_PARM_DESC(dev_id, "device node of fts partition");

static int erasesize = 0;
module_param(erasesize, int, 0444);
MODULE_PARM_DESC(erasesize, "erase size of the eMMC device");

/* Keep in sync with 'struct flash_ts' */
#define FLASH_TS_HDR_SIZE	(4 * sizeof(u32))
#define FLASH_TS_MAX_SIZE	(16 * 1024)
#define FLASH_TS_MAX_DATA_SIZE	(FLASH_TS_MAX_SIZE - FLASH_TS_HDR_SIZE)
#define FLASH_TS_CHNUNK_NUM		16

#define FLASH_TS_MAGIC		0x53542a46
static char fts_dev_node[sizeof("/dev/mmcblk0pxx")];

/* Physical flash layout */
struct flash_ts {
	u32 magic;		/* "F*TS" */
	u32 crc;		/* doesn't include magic and crc fields */
	u32 len;		/* real size of data */
	u32 version;		/* generation counter, must be positive */

	/* data format is very similar to Unix environment:
	 *   key1=value1\0key2=value2\0\0
	 */
	char data[FLASH_TS_MAX_DATA_SIZE];
};

struct emmc_info {
	u32 erasesize;
	u32 writesize;
	u32 size;
};
static struct emmc_info default_emmc = {512*1024, 512, 0};

/* Internal state */
struct flash_ts_priv {
	struct mutex lock;
	struct emmc_info *emmc;

	/* chunk size, >= sizeof(struct flash_ts) */
	size_t chunk;

	/* current record offset within eMMC device */
	loff_t offset;

	/* in-memory copy of flash content */
	struct flash_ts cache;

	/* temporary buffers
	 *  - one backup for failure rollback
	 *  - another for read-after-write verification
	 */
	struct flash_ts cache_tmp_backup;
	struct flash_ts cache_tmp_verify;
};
static struct flash_ts_priv *__ts;

static int is_blank(const void *buf, size_t size)
{
	int i;
	const unsigned int *data = (const unsigned int *)buf;

	size >>= 2;
	if (data[0] != 0 && data[0] != 0xffffffff)
		return 0;
	for (i = 1; i < size; i++)
		if (data[i] != data[0])
			return 0;
	return 1;
}

static int erase_block(struct emmc_info *emmc, loff_t off)
{
	struct file *filp;
	mm_segment_t old_fs = get_fs();
	int ret, i, cnt;
	char buf[512] = {0};

	set_fs(KERNEL_DS);

	printk(KERN_DEBUG DRV_NAME ": erase_block off=0x%08llx size=0x%x\n", off, emmc->erasesize);

	filp = filp_open(fts_dev_node, O_WRONLY, 0);
	if (IS_ERR(filp)) {
		printk(KERN_ERR DRV_NAME ": failed to open %s\n", fts_dev_node);
		ret = -1;
		goto err1;
	}

	if (filp->f_op->llseek(filp, off, SEEK_SET) < 0) {
		printk(KERN_ERR DRV_NAME ": failed to llseek %s\n", fts_dev_node);
		ret = -2;
		goto err2;
	}

	cnt = emmc->erasesize/sizeof(buf);
	for (i = 0; i < cnt; i++)
		if (__vfs_write(filp, buf, sizeof(buf), &filp->f_pos) < sizeof(buf)) {
			printk(KERN_ERR DRV_NAME ": failed to write %s\n", fts_dev_node);
			ret = -3;
			goto err2;
		}

	ret = 0;
err2:
	filp_close(filp, NULL);
err1:
	set_fs(old_fs);
	return ret;
}

static int flash_write(struct emmc_info *emmc, loff_t off,
		       const void *buf, size_t size)
{
	struct file *filp;
	mm_segment_t old_fs = get_fs();
	int ret;

	set_fs(KERNEL_DS);

	printk(KERN_DEBUG DRV_NAME ": write off=0x%llx size=0x%lx\n", off, size);

	filp = filp_open(fts_dev_node, O_WRONLY, 0);
	if (IS_ERR(filp)) {
		printk(KERN_ERR DRV_NAME ": failed to open %s\n", fts_dev_node);
		ret = -1;
		goto err1;
	}

	if (filp->f_op->llseek(filp, off, SEEK_SET) < 0) {
		printk(KERN_ERR DRV_NAME ": failed to llseek %s\n", fts_dev_node);
		ret = -2;
		goto err2;
	}

	if (__vfs_write(filp, buf, size, &filp->f_pos) < size) {
		printk(KERN_ERR DRV_NAME ": failed to write %s\n", fts_dev_node);
		ret = -3;
		goto err2;
	}

	ret = 0;
err2:
	filp_close(filp, NULL);
err1:
	set_fs(old_fs);
	return ret;
}

static int flash_read(struct emmc_info *emmc, loff_t off, void *buf, size_t size)
{
	struct file *filp;
	mm_segment_t old_fs = get_fs();
	int ret;

	set_fs(KERNEL_DS);

	filp = filp_open(fts_dev_node, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		printk(KERN_ERR DRV_NAME ": failed to open %s\n", fts_dev_node);
		ret = -1;
		goto err1;
	}

	if (filp->f_op->llseek(filp, off, SEEK_SET) < 0) {
		printk(KERN_ERR DRV_NAME ": failed to llseek %s\n", fts_dev_node);
		ret = -2;
		goto err2;
	}

	if (__vfs_read(filp, buf, size, &filp->f_pos) < size) {
		printk(KERN_ERR DRV_NAME ": failed to read %s\n", fts_dev_node);
		ret = -3;
		goto err2;
	}

	ret = 0;
err2:
	filp_close(filp, NULL);
err1:
	set_fs(old_fs);
	return ret;
}

static int get_fts_dev_node(void)
{
	struct file *filp;
	mm_segment_t old_fs = get_fs();
	int ret;
	int retry_cnt = 0;

	set_fs(KERNEL_DS);

open_retry:
	filp = filp_open(fts_dev_node, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		ret = -1;
		if (retry_cnt++ < RETRY_OPEN_MAX) {
			printk(KERN_ERR DRV_NAME
		       ": retry to open %s\n", fts_dev_node);
			msleep(10);
			goto open_retry;
		}
		printk(KERN_ERR DRV_NAME
		       ": failed to open %s\n", fts_dev_node);
		goto err1;
	}

	if (filp->f_op->llseek(filp, 0, SEEK_END) < 0) {
		printk(KERN_ERR DRV_NAME ": failed to llseek %s\n", fts_dev_node);
		ret = -2;
		goto err2;
	}
	ret = filp->f_pos;
	printk(KERN_DEBUG DRV_NAME ": fts partition is on %s, 0x%x bytes\n",
		fts_dev_node, ret);

err2:
	filp_close(filp, NULL);
err1:
	set_fs(old_fs);
	return ret;
}

static char *flash_ts_find(struct flash_ts_priv *ts, const char *key,
			   size_t key_len)
{
	char *s = ts->cache.data;
	while (*s) {
		if (!strncmp(s, key, key_len)) {
			if (s[key_len] == '=')
				return s;
		}

		s += strlen(s) + 1;
	}
	return NULL;
}

static inline u32 flash_ts_crc(const struct flash_ts *cache)
{
	/* skip magic and crc fields */
	return crc32(0, &cache->len, cache->len + 2 * sizeof(u32)) ^ ~0;
}

/* Verifies cache consistency and locks it */
static struct flash_ts_priv *__flash_ts_get(void)
{
	struct flash_ts_priv *ts = __ts;

	if (likely(ts)) {
		mutex_lock(&ts->lock);
	} else {
		printk(KERN_ERR DRV_NAME ": not initialized yet\n");
	}

	return ts;
}

static inline void __flash_ts_put(struct flash_ts_priv *ts)
{
	mutex_unlock(&ts->lock);
}

static int flash_ts_commit(struct flash_ts_priv *ts)
{
	struct emmc_info *emmc = ts->emmc;
	loff_t off = ts->offset + ts->chunk;
	/* we try to make two passes to handle non-erased blocks
	 * this should only matter for the inital pass over the whole device.
	 */
	int max_iterations = 10;
	size_t size = ALIGN(FLASH_TS_HDR_SIZE + ts->cache.len, emmc->writesize);

	/* fill unused part of data */
	memset(ts->cache.data + ts->cache.len, 0xff,
	       sizeof(ts->cache.data) - ts->cache.len);

	while (max_iterations--) {
		/* wrap around */
		if (off >= emmc->size)
			off = 0;
		/* write and read back to veryfy */
		if (flash_write(emmc, off, &ts->cache, size) ||
		    flash_read(emmc, off, &ts->cache_tmp_verify, size)) {
			/* next chunk */
			off += ts->chunk;
			continue;
		}

		/* compare */
		if (memcmp(&ts->cache, &ts->cache_tmp_verify, size)) {
			printk(KERN_WARNING DRV_NAME
			       ": record v%u read mismatch @ 0x%08llx\n",
				ts->cache.version, off);
			/* next chunk */
			off += ts->chunk;
			continue;
		}

		/* for new block, erase the previous block after write done,
		 * it's to speed up flash_ts_scan
		 */
		if (!(off & (emmc->erasesize - 1))) {
			loff_t pre_block_base = ts->offset & ~(emmc->erasesize - 1);
			loff_t cur_block_base = off & ~(emmc->erasesize - 1);
			if (cur_block_base != pre_block_base)
				erase_block(emmc, pre_block_base);
		}

		ts->offset = off;
		printk(KERN_DEBUG DRV_NAME ": record v%u commited @ 0x%08llx\n",
		       ts->cache.version, off);
		return 0;
	}

	printk(KERN_ERR DRV_NAME ": commit failure\n");
	return -EIO;
}

static int flash_ts_set(const char *key, const char *value)
{
	struct flash_ts_priv *ts;
	size_t klen = strlen(key);
	size_t vlen = strlen(value);
	int res;
	char *p;

	ts = __flash_ts_get();
	if (unlikely(!ts))
		return -EINVAL;

	/* save current cache contents so we can restore it on failure */
	memcpy(&ts->cache_tmp_backup, &ts->cache, sizeof(ts->cache_tmp_backup));

	p = flash_ts_find(ts, key, klen);
	if (p) {
		/* we are replacing existing entry,
		 * empty value (vlen == 0) removes entry completely.
		 */
		size_t cur_len = strlen(p) + 1;
		size_t new_len = vlen ? klen + 1 + vlen + 1 : 0;

		if (cur_len != new_len) {
			/* we need to move stuff around */

			if ((ts->cache.len - cur_len) + new_len >
			     sizeof(ts->cache.data))
				goto no_space;

			memmove(p + new_len, p + cur_len,
				ts->cache.len - (p - ts->cache.data + cur_len));

			ts->cache.len = (ts->cache.len - cur_len) + new_len;
		} else if (!strcmp(p + klen + 1, value)) {
			/* skip update if new value is the same as the old one */
			res = 0;
			goto out;
		}

		if (vlen) {
			p += klen + 1;
			memcpy(p, value, vlen);
			p[vlen] = '\0';
		}
	} else {
		size_t len = klen + 1 + vlen + 1;

		/* don't do anything if value is empty */
		if (!vlen) {
			res = 0;
			goto out;
		}

		if (ts->cache.len + len > sizeof(ts->cache.data))
			goto no_space;

		/* add new entry at the end */
		p = ts->cache.data + ts->cache.len - 1;
		memcpy(p, key, klen);
		p += klen;
		*p++ = '=';
		memcpy(p, value, vlen);
		p += vlen;
		*p++ = '\0';
		*p = '\0';
		ts->cache.len += len;
	}

	++ts->cache.version;
	ts->cache.crc = flash_ts_crc(&ts->cache);
	res = flash_ts_commit(ts);
	if (unlikely(res))
		memcpy(&ts->cache, &ts->cache_tmp_backup, sizeof(ts->cache));
	goto out;

no_space:
	printk(KERN_WARNING DRV_NAME ": no space left for '%s=%s'\n",
	       key, value);
	res = -ENOSPC;
out:
	__flash_ts_put(ts);

	return res;
}

static void flash_ts_get(const char *key, char *value, unsigned int size)
{
	size_t klen = strlen(key);
	struct flash_ts_priv *ts;
	const char *p;

	BUG_ON(!size);

	*value = '\0';

	ts = __flash_ts_get();
	if (unlikely(!ts))
		return;

	p = flash_ts_find(ts, key, klen);
	if (p)
		strlcpy(value, p + klen + 1, size);

	__flash_ts_put(ts);
}

static inline u32 flash_ts_check_header(const struct flash_ts *cache)
{
	if (cache->magic == FLASH_TS_MAGIC &&
	    cache->version &&
	    cache->len && cache->len <= sizeof(cache->data) &&
	    cache->crc == flash_ts_crc(cache) &&
	    /* check correct null-termination */
	    !cache->data[cache->len - 1] &&
	    (cache->len == 1 || !cache->data[cache->len - 2])) {
		/* all is good */
		return cache->version;
	}

	return 0;
}

static int flash_ts_scan(struct flash_ts_priv *ts)
{
	struct emmc_info *emmc = ts->emmc;
	int res, skiped_blocks = 0;
	loff_t off = 0;

	do {
		res = flash_read(emmc, off, &ts->cache_tmp_verify,
				 sizeof(ts->cache_tmp_verify));
		if (!res) {
			u32 version =
			    flash_ts_check_header(&ts->cache_tmp_verify);
			if (version > ts->cache.version) {
				memcpy(&ts->cache, &ts->cache_tmp_verify,
				       sizeof(ts->cache));
				ts->offset = off;
			}
			if (0 == version &&
			    is_blank(&ts->cache_tmp_verify,
				     sizeof(ts->cache_tmp_verify))) {
				/* skip the whole block if chunk is blank */
				off = (off + emmc->erasesize) & ~(emmc->erasesize - 1);
				skiped_blocks++;
			} else {
				off += ts->chunk;
			}
		} else {
			off += ts->chunk;
		}
	} while (off < emmc->size);

	printk(KERN_DEBUG DRV_NAME ": flash_ts_scan skiped %d blocks\n", skiped_blocks);
	return 0;
}

/* User-space access */
struct flash_ts_dev {
	struct mutex lock;
	struct flash_ts_io_req req;
};

/* Round-up to the next power-of-2,
 * from "Hacker's Delight" by Henry S. Warren.
 */
static inline u32 clp2(u32 x)
{
	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

static int flash_ts_open(struct inode *inode, struct file *file)
{
	struct flash_ts_priv *ts;
	int res, size;
	struct flash_ts_dev *dev;

	size = get_fts_dev_node();
	if (size <= 0)
		return -ENOMEM;

	dev = kmalloc(sizeof(*dev), GFP_KERNEL);
	if (unlikely(!dev))
		return -ENOENT;

	mutex_init(&dev->lock);
	file->private_data = dev;

	ts = __flash_ts_get();
	if (unlikely(!ts)) {
		res = -EINVAL;
		goto err1;
	}

	if (ts->chunk != 0) {
		/* ts already initialized */
		__flash_ts_put(ts);
		return 0;
	}

	ts->chunk = clp2((sizeof(struct flash_ts) + ts->emmc->writesize - 1) &
			  ~(ts->emmc->writesize - 1));
	if (unlikely(ts->chunk > ts->emmc->erasesize)) {
		res = -ENODEV;
		printk(KERN_ERR DRV_NAME ": eMMC block size is too small\n");
		goto err2;
	}

	/* determine chunk size so it doesn't cross block boundary,
	 * is multiple of page size and there is no wasted space in a block.
	 * We assume page and block sizes are power-of-2.
	 */
	ts->emmc->size = ts->chunk * FLASH_TS_CHNUNK_NUM > size ? size : ts->chunk * FLASH_TS_CHNUNK_NUM;

	printk(KERN_DEBUG DRV_NAME ": chunk: 0x%lx\n", ts->chunk);

	/* default empty state */
	ts->offset = ts->emmc->size - ts->chunk;
	ts->cache.magic = FLASH_TS_MAGIC;
	ts->cache.version = 0;
	ts->cache.len = 1;
	ts->cache.data[0] = '\0';
	ts->cache.crc = flash_ts_crc(&ts->cache);

	/* scan flash partition for the most recent record */
	res = flash_ts_scan(ts);
	if (ts->cache.version)
		printk(KERN_DEBUG DRV_NAME ": v%u loaded from 0x%08llx\n",
		       ts->cache.version, ts->offset);
	__flash_ts_put(ts);
	return 0;

err2:
	__flash_ts_put(ts);
err1:
	kfree(dev);
	return res;
}

static int flash_ts_release(struct inode *inode, struct file *file)
{
	kfree(file->private_data);
	return 0;
}

static long flash_ts_ioctl(struct file *file, unsigned int cmd,
			  unsigned long arg)
{
	struct flash_ts_dev *dev = file->private_data;
	struct flash_ts_io_req *req = &dev->req;
	int res;

	if (unlikely(mutex_lock_interruptible(&dev->lock)))
		return -ERESTARTSYS;

	if (unlikely(copy_from_user(req, (const void* __user)arg,
				    sizeof(*req)))) {
		res = -EFAULT;
		goto out;
	}

	req->key[sizeof(req->key) - 1] = '\0';

	switch (cmd) {
	case FLASH_TS_IO_SET:
		req->val[sizeof(req->val) - 1] = '\0';
		res = flash_ts_set(req->key, req->val);
		break;

	case FLASH_TS_IO_GET:
		flash_ts_get(req->key, req->val, sizeof(req->val));
		res = copy_to_user((void* __user)arg, req,
				   sizeof(*req)) ? -EFAULT : 0;
		break;

	default:
		res = -ENOTTY;
	}

out:
	mutex_unlock(&dev->lock);
	return res;
}

static struct file_operations flash_ts_fops = {
	.owner = THIS_MODULE,
	.open = flash_ts_open,
	.unlocked_ioctl = flash_ts_ioctl,
	.release = flash_ts_release,
};

#if ENABLE_MISC_DEVICE
static struct miscdevice flash_ts_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DRV_NAME,
	.fops = &flash_ts_fops,
};
#endif

/* Debugging (procfs) */
static void *flash_ts_proc_start(struct seq_file *m, loff_t *pos)
{
	if (*pos == 0) {
		struct flash_ts_priv *ts = __flash_ts_get();
		if (ts) {
			BUG_ON(m->private);
			m->private = ts;
			return ts->cache.data;
		}
	}

	*pos = 0;
	return NULL;
}

static void *flash_ts_proc_next(struct seq_file *m, void *v, loff_t *pos)
{
	char *s = (char *)v;
	s += strlen(s) + 1;
	++(*pos);
	return *s ? s : NULL;
}

static void flash_ts_proc_stop(struct seq_file *m, void *v)
{
	struct flash_ts_priv *ts = m->private;
	if (ts) {
		m->private = NULL;
		__flash_ts_put(ts);
	}
}

static int flash_ts_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", (char*)v);
	return 0;
}

static struct seq_operations flash_ts_seq_ops = {
	.start	= flash_ts_proc_start,
	.next	= flash_ts_proc_next,
	.stop	= flash_ts_proc_stop,
	.show	= flash_ts_proc_show,
};

static int flash_ts_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &flash_ts_seq_ops);
}

static const struct file_operations flash_ts_proc_fops = {
	.owner = THIS_MODULE,
	.open = flash_ts_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int __init flash_ts_init(void)
{
	struct flash_ts_priv *ts;
	struct emmc_info *emmc = &default_emmc;
	int res;

	if (dev_id <= 0 || dev_id >= 100) {
		printk(KERN_ERR DRV_NAME
		       ": invalid emmc_ts.dev_id %d\n", dev_id);
		return -ENODEV;
	}
	sprintf(fts_dev_node, "/dev/mmcblk0p%d", dev_id);

	emmc->erasesize = erasesize;
	/* make sure both page and block sizes are power-of-2
	 * (this will make chunk size determination simpler).
	 */

	if (unlikely(!is_power_of_2(emmc->writesize) ||
		     !is_power_of_2(emmc->erasesize) ||
		     emmc->erasesize < sizeof(struct flash_ts))) {
		res = -ENODEV;
		printk(KERN_ERR DRV_NAME ": unsupported eMMC geometry\n");
		return res;
	}

	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (unlikely(!ts)) {
		res = -ENOMEM;
		printk(KERN_ERR DRV_NAME ": failed to allocate memory\n");
		return res;
	}

	mutex_init(&ts->lock);
	ts->emmc = emmc;

#if ENABLE_MISC_DEVICE
	res = misc_register(&flash_ts_miscdev);
	if (unlikely(res))
		goto out_free;
#else
	res = register_chrdev(fts_major, FTS_DEV_NAME, &flash_ts_fops);
	if (unlikely(res))
		goto out_free;
#endif

	smp_mb();
	__ts = ts;

	proc_create(DRV_NAME, 0, NULL, &flash_ts_proc_fops);

	return 0;

out_free:
	kfree(ts);

	return res;
}

#if ENABLE_MISC_DEVICE
/* Make sure eMMC subsystem is already initialized */
late_initcall(flash_ts_init);
#else
module_init(flash_ts_init);
#endif
