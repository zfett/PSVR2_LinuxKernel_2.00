/*
 * Flash-based transactional key-value store
 *
 * Copyright (C) 2010 Google, Inc.
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

#ifndef _LINUX_FLASH_TS_H_
#define _LINUX_FLASH_TS_H_

#include <asm/ioctl.h>
#include <asm/types.h>

#include "sce_km_defs.h"

#define FTS_IOC_MAGIC 0x34
#define FTS_DEV_NAME "fts"
#define FTS_MAJOR SCE_KM_CDEV_MAJOR_FTS

#define FLASH_TS_MAX_KEY_SIZE	64
#define FLASH_TS_MAX_VAL_SIZE	2048

struct flash_ts_io_req {
	char key[FLASH_TS_MAX_KEY_SIZE];
	char val[FLASH_TS_MAX_VAL_SIZE];
};

#define FLASH_TS_IO_MAGIC	0xFE
#define FLASH_TS_IO_SET		_IOW(FLASH_TS_IO_MAGIC, 0, struct flash_ts_io_req)
#define FLASH_TS_IO_GET		_IOWR(FLASH_TS_IO_MAGIC, 1, struct flash_ts_io_req)

#endif  /* _LINUX_FLASH_TS_H_ */

