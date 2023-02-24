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

#include <linux/sched.h>

#include <linux/debugfs.h>
#include <soc/mediatek/mtk_ts.h>
#include "../sieimu_spi_if.h"
#include "sie_icm426_reg.h"

/*#define REG_DUMP_ALL*/

static struct sieimu_spi *s_spi_handle;

struct icm426debug_isr_trace {
	u64 ktime;
	u32 vts;
	u32 sample_cnt;
	u32 irq_cnt;
	u16 imu_cnt;
};

struct icm426debug_get_write_pos_trace {
	u64 ktime;
	u32 wrap_cnt;
	u32 write_pos;
};

struct icm426debug_request_callback_trace {
	u64 ktime;
	u32 vts;
};

struct icm426debug_request_callback_done_trace {
	u64 ktime;
	u32 vts;
	s32 diff_vts;
	u32 adjust_isr_phase;
	u32 wc;
	u32 wp;
};

struct icm426debug_data_trace {
	u64 ktime;
	u32 vts;
	u32 thread_cnt;
	u32 imu_vts;
	u16 imu_ts;
	u16 imu_cnt;
	u32 wc;
	u32 wp;
	u16 eod;
	u16 invalid;
};

#define DEBUF_TRACE_BUF_SIZE 10000
#define MAX_HISTOGRAM (IMU_INTERVAL * 8)
static struct icm426debug_isr_trace s_isr_trace[DEBUF_TRACE_BUF_SIZE];
static struct icm426debug_get_write_pos_trace s_get_write_pos_trace[DEBUF_TRACE_BUF_SIZE];
static struct icm426debug_request_callback_trace s_req_callback_trace[DEBUF_TRACE_BUF_SIZE];
static struct icm426debug_request_callback_done_trace s_req_callback_done_trace[DEBUF_TRACE_BUF_SIZE];
static struct icm426debug_data_trace s_data_trace[DEBUF_TRACE_BUF_SIZE];
static u32 s_vts_histogram[MAX_HISTOGRAM];
static u32 s_isr_histogram[MAX_HISTOGRAM];
static u32 s_max_vts_histogram;
static u32 s_max_isr_histogram;

static int s_isr_trace_pos;
static int s_get_write_pos_trace_pos;
static int s_req_callback_trace_pos;
static int s_req_callback_done_trace_pos;
static int s_data_trace_pos;

static DEFINE_MUTEX(s_isr_trace_mutex);
static DEFINE_MUTEX(s_get_write_pos_trace_mutex);
static DEFINE_MUTEX(s_req_callback_trace_mutex);
static DEFINE_MUTEX(s_req_callback_trace_done_mutex);
static DEFINE_MUTEX(s_data_trace_mutex);

void icm426debug_isr_trace(u32 vts, u32 sample_cnt, u32 imu_vts, u16 imu_cnt)
{
	static u32 pre_ktime;
	static u32 pre_imu_vts;
	static u32 irq_cnt;
	u32 ktime;

	irq_cnt++;
	if (mutex_trylock(&s_isr_trace_mutex) != 0) {
		ktime = local_clock() / 1000;
		s_isr_trace[s_isr_trace_pos].ktime = ktime;
		s_isr_trace[s_isr_trace_pos].vts = vts;
		s_isr_trace[s_isr_trace_pos].sample_cnt = sample_cnt;
		s_isr_trace[s_isr_trace_pos].irq_cnt = irq_cnt;
		s_isr_trace[s_isr_trace_pos].imu_cnt = imu_cnt;
		s_isr_trace_pos = (s_isr_trace_pos + 1) % DEBUF_TRACE_BUF_SIZE;
		if (ktime - pre_ktime < MAX_HISTOGRAM) {
			s_isr_histogram[ktime - pre_ktime]++;
		} else if (pre_ktime != 0) {
			s_isr_histogram[MAX_HISTOGRAM - 1]++;
			if (s_max_isr_histogram < ktime - pre_ktime)
				s_max_isr_histogram = ktime - pre_ktime;
		}
		if (imu_vts - pre_imu_vts < MAX_HISTOGRAM) {
			s_vts_histogram[imu_vts - pre_imu_vts]++;
		} else  if (pre_imu_vts != 0) {
			s_vts_histogram[MAX_HISTOGRAM - 1]++;
			if (s_max_vts_histogram < imu_vts - pre_imu_vts)
				s_max_vts_histogram = imu_vts - pre_imu_vts;
		}
		pre_ktime = ktime;
		pre_imu_vts = imu_vts;
		mutex_unlock(&s_isr_trace_mutex);
	}
}

void icm426debug_get_write_pos_trace(u32 wrap_cnt, u32 write_pos)
{
	if (mutex_trylock(&s_get_write_pos_trace_mutex)) {
		s_get_write_pos_trace[s_get_write_pos_trace_pos].ktime = local_clock() / 1000;
		s_get_write_pos_trace[s_get_write_pos_trace_pos].wrap_cnt = wrap_cnt;
		s_get_write_pos_trace[s_get_write_pos_trace_pos].write_pos = write_pos;
		s_get_write_pos_trace_pos = (s_get_write_pos_trace_pos + 1) % DEBUF_TRACE_BUF_SIZE;
		mutex_unlock(&s_get_write_pos_trace_mutex);
	}
}

void icm426debug_req_callback_trace(u32 vts)
{
	if (mutex_trylock(&s_req_callback_trace_mutex)) {
		s_req_callback_trace[s_req_callback_trace_pos].ktime = local_clock() / 1000;
		s_req_callback_trace[s_req_callback_trace_pos].vts = vts;
		s_req_callback_trace_pos = (s_req_callback_trace_pos + 1) % DEBUF_TRACE_BUF_SIZE;
		mutex_unlock(&s_req_callback_trace_mutex);
	}
}

void icm426debug_req_callback_done_trace(u32 vts, s32 diff_vts,
					 u32 adjust_isr_phase, u32 wc, u32 wp)
{
	if (mutex_trylock(&s_req_callback_trace_done_mutex)) {
		s_req_callback_done_trace[s_req_callback_done_trace_pos].ktime = local_clock() / 1000;
		s_req_callback_done_trace[s_req_callback_done_trace_pos].vts = vts;
		s_req_callback_done_trace[s_req_callback_done_trace_pos].diff_vts = diff_vts;
		s_req_callback_done_trace[s_req_callback_done_trace_pos].adjust_isr_phase = adjust_isr_phase;
		s_req_callback_done_trace[s_req_callback_done_trace_pos].wc = wc;
		s_req_callback_done_trace[s_req_callback_done_trace_pos].wp = wp;
		s_req_callback_done_trace_pos = (s_req_callback_done_trace_pos + 1) % DEBUF_TRACE_BUF_SIZE;
		mutex_unlock(&s_req_callback_trace_done_mutex);
	}
}

void icm426debug_data_trace(u32 vts, u32 imu_vts, u16 imu_ts, u16 imu_cnt,
			    u32 wc, u32 wp, u16 eod, u16 invalid)
{
	static u32 thread_cnt;

	thread_cnt++;
	if (mutex_trylock(&s_data_trace_mutex)) {
		s_data_trace[s_data_trace_pos].ktime = local_clock() / 1000;
		s_data_trace[s_data_trace_pos].vts = vts;
		s_data_trace[s_data_trace_pos].thread_cnt = thread_cnt;
		s_data_trace[s_data_trace_pos].imu_vts = imu_vts;
		s_data_trace[s_data_trace_pos].imu_ts = imu_ts;
		s_data_trace[s_data_trace_pos].imu_cnt = imu_cnt;
		s_data_trace[s_data_trace_pos].wp = wp;
		s_data_trace[s_data_trace_pos].wc = wc;
		s_data_trace[s_data_trace_pos].eod = eod;
		s_data_trace[s_data_trace_pos].invalid = invalid;
		s_data_trace_pos = (s_data_trace_pos + 1) % DEBUF_TRACE_BUF_SIZE;
		mutex_unlock(&s_data_trace_mutex);
	}
}

static int icm426debug_log_show(struct seq_file *s, void *unused)
{
	int isr_pos;
	int get_write_pos_pos;
	int req_callback_pos;
	int req_callback_done_pos;
	int data_pos;
	int i;
	u64 start_ktime;

	mutex_lock(&s_isr_trace_mutex);
	mutex_lock(&s_get_write_pos_trace_mutex);
	mutex_lock(&s_req_callback_trace_mutex);
	mutex_lock(&s_req_callback_trace_done_mutex);
	mutex_lock(&s_data_trace_mutex);

	s_isr_trace[s_isr_trace_pos].ktime = ULONG_MAX;
	s_get_write_pos_trace[s_get_write_pos_trace_pos].ktime = ULONG_MAX;
	s_req_callback_trace[s_req_callback_trace_pos].ktime = ULONG_MAX;
	s_req_callback_done_trace[s_req_callback_done_trace_pos].ktime = ULONG_MAX;
	s_data_trace[s_data_trace_pos].ktime = ULONG_MAX;

	data_pos = s_data_trace_pos;
	for (i = 0; i < DEBUF_TRACE_BUF_SIZE; i++) {
		data_pos = (data_pos + 1) % DEBUF_TRACE_BUF_SIZE;
		if (s_data_trace[data_pos].ktime != 0)
			break;
	}
	start_ktime = s_data_trace[data_pos].ktime;

	isr_pos = s_isr_trace_pos;
	for (i = 0; i < DEBUF_TRACE_BUF_SIZE; i++) {
		isr_pos = (isr_pos + 1) % DEBUF_TRACE_BUF_SIZE;
		if (s_isr_trace[isr_pos].ktime > start_ktime)
			break;
	}
	get_write_pos_pos = s_get_write_pos_trace_pos;
	for (i = 0; i < DEBUF_TRACE_BUF_SIZE; i++) {
		get_write_pos_pos = (get_write_pos_pos + 1) % DEBUF_TRACE_BUF_SIZE;
		if (s_get_write_pos_trace[get_write_pos_pos].ktime > start_ktime)
			break;
	}
	req_callback_pos = s_req_callback_trace_pos;
	for (i = 0; i < DEBUF_TRACE_BUF_SIZE; i++) {
		req_callback_pos = (req_callback_pos + 1) % DEBUF_TRACE_BUF_SIZE;
		if (s_req_callback_trace[req_callback_pos].ktime > start_ktime)
			break;
	}
	req_callback_done_pos = s_req_callback_done_trace_pos;
	for (i = 0; i < DEBUF_TRACE_BUF_SIZE; i++) {
		req_callback_done_pos = (req_callback_done_pos + 1) % DEBUF_TRACE_BUF_SIZE;
		if (s_req_callback_done_trace[req_callback_done_pos].ktime > start_ktime)
			break;
	}

	i = 0;
	while (isr_pos != s_isr_trace_pos ||
	       get_write_pos_pos != s_get_write_pos_trace_pos ||
	       req_callback_pos != s_req_callback_trace_pos ||
	       req_callback_done_pos != s_req_callback_done_trace_pos ||
	       data_pos != s_data_trace_pos
	      ) {
		i++;

		if (isr_pos != s_isr_trace_pos &&
		    s_isr_trace[isr_pos].ktime <= s_get_write_pos_trace[get_write_pos_pos].ktime &&
		    s_isr_trace[isr_pos].ktime <= s_req_callback_trace[req_callback_pos].ktime &&
		    s_isr_trace[isr_pos].ktime <= s_req_callback_done_trace[req_callback_done_pos].ktime &&
		    s_isr_trace[isr_pos].ktime <= s_data_trace[data_pos].ktime
		   ) {
			seq_printf(s, "%6llu.%06llu  [ISR ] vts:%u sample_cnt:%u irq_cnt:%u imu_cnt:%u\n",
				   s_isr_trace[isr_pos].ktime / 1000000,
				   s_isr_trace[isr_pos].ktime % 1000000,
				   s_isr_trace[isr_pos].vts,
				   s_isr_trace[isr_pos].sample_cnt,
				   s_isr_trace[isr_pos].irq_cnt,
				   s_isr_trace[isr_pos].imu_cnt);
			isr_pos = (isr_pos + 1) % DEBUF_TRACE_BUF_SIZE;
		}
		if (get_write_pos_pos != s_get_write_pos_trace_pos &&
		    s_get_write_pos_trace[get_write_pos_pos].ktime <= s_isr_trace[isr_pos].ktime &&
		    s_get_write_pos_trace[get_write_pos_pos].ktime <= s_req_callback_trace[req_callback_pos].ktime &&
		    s_get_write_pos_trace[get_write_pos_pos].ktime <= s_req_callback_done_trace[req_callback_done_pos].ktime &&
		    s_get_write_pos_trace[get_write_pos_pos].ktime <= s_data_trace[data_pos].ktime
		   ) {
			seq_printf(s, "%6llu.%06llu  [GET ] wc:%u wp:%u\n",
				   s_get_write_pos_trace[get_write_pos_pos].ktime / 1000000,
				   s_get_write_pos_trace[get_write_pos_pos].ktime % 1000000,
				   s_get_write_pos_trace[get_write_pos_pos].wrap_cnt,
				   s_get_write_pos_trace[get_write_pos_pos].write_pos);
			get_write_pos_pos = (get_write_pos_pos + 1) % DEBUF_TRACE_BUF_SIZE;
		}
		if (req_callback_pos != s_req_callback_trace_pos &&
		    s_req_callback_trace[req_callback_pos].ktime <= s_isr_trace[isr_pos].ktime &&
		    s_req_callback_trace[req_callback_pos].ktime <= s_get_write_pos_trace[get_write_pos_pos].ktime &&
		    s_req_callback_trace[req_callback_pos].ktime <= s_req_callback_done_trace[req_callback_done_pos].ktime &&
		    s_req_callback_trace[req_callback_pos].ktime <= s_data_trace[data_pos].ktime
		   ) {
			seq_printf(s, "%6llu.%06llu *[Req ] vts:%u\n",
				   s_req_callback_trace[req_callback_pos].ktime / 1000000,
				   s_req_callback_trace[req_callback_pos].ktime % 1000000,
				   s_req_callback_trace[req_callback_pos].vts);
			req_callback_pos = (req_callback_pos + 1) % DEBUF_TRACE_BUF_SIZE;
		}
		if (req_callback_done_pos != s_req_callback_done_trace_pos &&
		    s_req_callback_done_trace[req_callback_done_pos].ktime <= s_isr_trace[isr_pos].ktime &&
		    s_req_callback_done_trace[req_callback_done_pos].ktime <= s_get_write_pos_trace[get_write_pos_pos].ktime &&
		    s_req_callback_done_trace[req_callback_done_pos].ktime <= s_req_callback_trace[req_callback_pos].ktime &&
		    s_req_callback_done_trace[req_callback_done_pos].ktime <= s_data_trace[data_pos].ktime
		   ) {
			seq_printf(s, "%6llu.%06llu *[Done] vts:%u diff_vts:%d adjust_isr_phase:%u wc:%u wp:%u\n",
				   s_req_callback_done_trace[req_callback_done_pos].ktime / 1000000,
				   s_req_callback_done_trace[req_callback_done_pos].ktime % 1000000,
				   s_req_callback_done_trace[req_callback_done_pos].vts,
				   s_req_callback_done_trace[req_callback_done_pos].diff_vts,
				   s_req_callback_done_trace[req_callback_done_pos].adjust_isr_phase,
				   s_req_callback_done_trace[req_callback_done_pos].wc,
				   s_req_callback_done_trace[req_callback_done_pos].wp);
			req_callback_done_pos = (req_callback_done_pos + 1) % DEBUF_TRACE_BUF_SIZE;
		}
		if (data_pos != s_data_trace_pos &&
		    s_data_trace[data_pos].ktime <= s_isr_trace[isr_pos].ktime &&
		    s_data_trace[data_pos].ktime <= s_get_write_pos_trace[get_write_pos_pos].ktime &&
		    s_data_trace[data_pos].ktime <= s_req_callback_trace[req_callback_pos].ktime &&
		    s_data_trace[data_pos].ktime <= s_req_callback_done_trace[req_callback_done_pos].ktime
		   ) {
			seq_printf(s, "%6llu.%06llu  [Data] vts:%u thread_cnt:%u imu_vts:%u imu_ts:%u imu_cnt:%u wc:%u wp:%u eod:%u invalid:%u\n",
				   s_data_trace[data_pos].ktime / 1000000,
				   s_data_trace[data_pos].ktime % 1000000,
				   s_data_trace[data_pos].vts,
				   s_data_trace[data_pos].thread_cnt,
				   s_data_trace[data_pos].imu_vts,
				   s_data_trace[data_pos].imu_ts,
				   s_data_trace[data_pos].imu_cnt,
				   s_data_trace[data_pos].wc,
				   s_data_trace[data_pos].wp,
				   s_data_trace[data_pos].eod,
				   s_data_trace[data_pos].invalid);
			data_pos = (data_pos + 1) % DEBUF_TRACE_BUF_SIZE;
		}
	}

	mutex_unlock(&s_data_trace_mutex);
	mutex_unlock(&s_req_callback_trace_done_mutex);
	mutex_unlock(&s_req_callback_trace_mutex);
	mutex_unlock(&s_get_write_pos_trace_mutex);
	mutex_unlock(&s_isr_trace_mutex);

	return 0;
}

static int icm426debug_log_open(struct inode *inode, struct file *file)
{
	return single_open(file, icm426debug_log_show, NULL);
}

static const struct file_operations icm426_debug_log_ops = {
	.open		= icm426debug_log_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int icm426debug_hist_show(struct seq_file *s, void *unused)
{
	int i;
	s64 sum;
	s64 ave;
	u64 sum_dispersion;
	u32 cnt;

	seq_puts(s, "IMU VTS histogram\n");
	sum = 0;
	sum_dispersion = 0;
	cnt = 0;
	for (i = 0; i < MAX_HISTOGRAM - 1; i++) {
		if (s_vts_histogram[i] != 0)
			seq_printf(s, "[%4d] %u\n", i, s_vts_histogram[i]);

		sum += (s64)i * s_vts_histogram[i];
		cnt += s_vts_histogram[i];
	}
	ave = sum / cnt;
	for (i = 0; i < MAX_HISTOGRAM - 1; i++) {
		u64 tmp = (ave - i) * (ave - i) * s_vts_histogram[i];

		sum_dispersion += tmp;
		if (sum_dispersion < tmp) { /* detect overfllow */
			sum_dispersion = 0;
			break;
		}
	}
	seq_printf(s, "[Over %4d] %u (Max : %u)\n", i,
		   s_vts_histogram[i], s_max_vts_histogram);
	seq_printf(s, "Average:%lld dispersion:%lld\n\n",
		   ave, sum_dispersion / cnt);

	seq_puts(s, "ISR timing histogram\n");
	sum = 0;
	sum_dispersion = 0;
	cnt = 0;
	for (i = 0; i < MAX_HISTOGRAM - 1; i++) {
		if (s_isr_histogram[i] != 0)
			seq_printf(s, "[%4d] %u\n", i, s_isr_histogram[i]);

		sum += (s64)i * s_isr_histogram[i];
		cnt += s_isr_histogram[i];
	}
	ave = sum / cnt;
	for (i = 0; i < MAX_HISTOGRAM - 1; i++) {
		u64 tmp = (u64)((ave - i) * (ave - i)) * s_isr_histogram[i];

		sum_dispersion += tmp;
		if (sum_dispersion < tmp) { /* detect overfllow */
			sum_dispersion = 0;
			break;
		}
	}
	seq_printf(s, "[Over %4d] %u (Max : %u)\n", i,
		   s_isr_histogram[i], s_max_isr_histogram);
	seq_printf(s, "Average:%lld dispersion:%lld\n\n",
		   ave, sum_dispersion / cnt);

	return 0;
}

static int icm426debug_hist_open(struct inode *inode, struct file *file)
{
	return single_open(file, icm426debug_hist_show, NULL);
}

static const struct file_operations icm426_debug_hist_ops = {
	.open		= icm426debug_hist_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

struct icm426_reg_list {
	u16 size;
	u16 addr;
	char *name;
};

#define MAX_REG_BANK_NUM 4
static const struct icm426_reg_list bank0_reg_list[] = {
	{ 1, ICM426_REG_DEVICE_CONFIG,			"DEVICE_CONFIG"		},
	{ 1, ICM426_REG_DRIVE_CONFIG,			"DRIVE_CONFIG"		},
	{ 1, ICM426_REG_INT_CONFIG,				"INT_CONFIG"		},
	{ 1, ICM426_REG_FIFO_CONFIG,			"FIFO_CONFIG"		},
	{ 2, ICM426_REG_TEMP_DATA0_UI,			"TEMP_DATA"			},
	{ 2, ICM426_REG_ACCEL_DATA_X0_UI,		"ACCEL_DATA_X"		},
	{ 2, ICM426_REG_ACCEL_DATA_X0_UI + 2,	"ACCEL_DATA_Y"		},
	{ 2, ICM426_REG_ACCEL_DATA_X0_UI + 4,	"ACCEL_DATA_Z"		},
	{ 2, ICM426_REG_GYRO_DATA_X0_UI,		"GYRO_DATA_X"		},
	{ 2, ICM426_REG_GYRO_DATA_X0_UI + 2,	"GYRO_DATA_Y"		},
	{ 2, ICM426_REG_GYRO_DATA_X0_UI + 4,	"GYRO_DATA_Z"		},
	{ 1, ICM426_REG_INT_STATUS,				"INT_STATUS"		},
	{ 2, ICM426_REG_FIFO_BYTE_COUNT1,		"FIFO_BYTE_COUNT"	},
	{ 1, ICM426_REG_INT_STATUS2,			"INT_STATUS2"		},
	{ 1, ICM426_REG_INT_STATUS3,			"INT_STATUS3"		},
	{ 1, ICM426_REG_INTF_CONFIG0,			"INTF_CONFIG0"		},
	{ 1, ICM426_REG_INTF_CONFIG1,			"INTF_CONFIG1"		},
	{ 1, ICM426_REG_PWR_MGMT_0,				"PWR_MGMT"			},
	{ 1, ICM426_REG_GYRO_CONFIG0,			"GYRO_CONFIG0"		},
	{ 1, ICM426_REG_ACCEL_CONFIG0,			"ACCEL_CONFIG0"		},
	{ 1, ICM426_REG_GYRO_CONFIG1,			"GYRO_CONFIG1"		},
	{ 1, ICM426_REG_ACCEL_GYRO_CONFIG0,		"ACCEL_GYRO_CONFIG0"},
	{ 1, ICM426_REG_ACCEL_CONFIG1,			"ACCEL_CONFIG1"		},
	{ 1, ICM426_REG_TMST_CONFIG,			"TMST_CONFIG"		},
	{ 1, ICM426_REG_APEX_CONFIG0,			"APEX_CONFIG0"		},
	{ 1, ICM426_REG_SMD_CONFIG,				"SMD_CONFIG"		},
	{ 1, ICM426_REG_FIFO_CONFIG1,			"FIFO_CONFIG1"		},
	{ 1, ICM426_REG_FIFO_CONFIG2,			"FIFO_CONFIG2"		},
	{ 1, ICM426_REG_INT_CONFIG0,			"INT_CONFIG0"		},
	{ 1, ICM426_REG_INT_CONFIG1,			"INT_CONFIG1"		},
	{ 1, ICM426_REG_INT_SOURCE0,			"INT_SOURCE0"		},
	{ 1, ICM426_REG_INT_SOURCE1,			"INT_SOURCE1"		},
	{ 1, ICM426_REG_INT_SOURCE3,			"INT_SOURCE3"		},
	{ 1, ICM426_REG_INT_SOURCE4,			"INT_SOURCE4"		},
	{ 2, ICM426_REG_FIFO_LOST_PKT0,			"FIFO_LOST_PKT"		},
	{ 1, ICM426_REG_SELF_TEST_CONFIG,		"SELF_TEST_CONFIG"	},
	{ 1, ICM426_REG_WHO_AM_I,				"WHO_AM_I"			},
	{ 1, ICM426_REG_REG_BANK_SEL,			"BANK_SEL"			},
	{ 0, 0, NULL },
};

static const struct icm426_reg_list bank1_reg_list[] = {
	{ 2, ICM426_REG_TMST_VAL0_B1,			"TMST_VAL"			},
	{ 1, ICM426_REG_INTF_CONFIG4_B1,		"INTF_CONFIG4"		},
	{ 1, ICM426_REG_INTF_CONFIG5_B1,		"INTF_CONFIG5"		},
	{ 0, 0, NULL },
};

static const struct icm426_reg_list bank2_reg_list[] = {
	{ 1, ICM426_REG_ACCEL_CONFIG_STATIC2_B2, "ACCEL_CONFIG_STATIC2"},
	{ 1, ICM426_REG_ACCEL_CONFIG_STATIC3_B2, "ACCEL_CONFIG_STATIC3"},
	{ 1, ICM426_REG_ACCEL_CONFIG_STATIC4_B2, "ACCEL_CONFIG_STATIC4"},
	{ 1, ICM426_REG_XA_ST_DATA_B2,			"XA_ST_DATA"		},
	{ 1, ICM426_REG_YA_ST_DATA_B2,			"YA_ST_DATA"		},
	{ 1, ICM426_REG_ZA_ST_DATA_B2,			"ZA_ST_DATA"		},
	{ 0, 0, NULL },
};

static const struct icm426_reg_list bank4_reg_list[] = {
	{ 1, ICM426_REG_APEX_CONFIG1_B4,		"APEX_CONFIG1"		},
	{ 1, ICM426_REG_APEX_CONFIG2_B4,		"APEX_CONFIG2"		},
	{ 1, ICM426_REG_APEX_CONFIG3_B4,		"APEX_CONFIG3"		},
	{ 1, ICM426_REG_APEX_CONFIG4_B4,		"APEX_CONFIG4"		},
	{ 1, ICM426_REG_APEX_CONFIG5_B4,		"APEX_CONFIG5"		},
	{ 1, ICM426_REG_APEX_CONFIG6_B4,		"APEX_CONFIG6"		},
	{ 1, ICM426_REG_APEX_CONFIG7_B4,		"APEX_CONFIG7"		},
	{ 1, ICM426_REG_APEX_CONFIG8_B4,		"APEX_CONFIG8"		},
	{ 1, ICM426_REG_APEX_CONFIG9_B4,		"APEX_CONFIG9"		},
	{ 1, ICM426_REG_ACCEL_WOM_X_THR_B4,		"ACCEL_WOM_X_THR"	},
	{ 1, ICM426_REG_ACCEL_WOM_Y_THR_B4,		"ACCEL_WOM_Y_THR"	},
	{ 1, ICM426_REG_ACCEL_WOM_Z_THR_B4,		"ACCEL_WOM_Z_THR"	},
	{ 1, ICM426_REG_INT_SOURCE6_B4,			"INT_SOURCE6"		},
	{ 1, ICM426_REG_INT_SOURCE7_B4,			"INT_SOURCE7"		},
	{ 1, ICM426_REG_OFFSET_USER_0_B4,		"OFFSET_USER0"		},
	{ 1, ICM426_REG_OFFSET_USER_1_B4,		"OFFSET_USER1"		},
	{ 1, ICM426_REG_OFFSET_USER_2_B4,		"OFFSET_USER2"		},
	{ 1, ICM426_REG_OFFSET_USER_3_B4,		"OFFSET_USER3"		},
	{ 1, ICM426_REG_OFFSET_USER_4_B4,		"OFFSET_USER4"		},
	{ 1, ICM426_REG_OFFSET_USER_5_B4,		"OFFSET_USER5"		},
	{ 1, ICM426_REG_OFFSET_USER_6_B4,		"OFFSET_USER6"		},
	{ 1, ICM426_REG_OFFSET_USER_7_B4,		"OFFSET_USER7"		},
	{ 1, ICM426_REG_OFFSET_USER_8_B4,		"OFFSET_USER8"		},
	{ 0, 0, NULL },
};

static const struct icm426_reg_list *reg_list[MAX_REG_BANK_NUM + 1] = {
	[0] = bank0_reg_list,
#ifdef REG_DUMP_ALL
	[1] = bank1_reg_list,
	[2] = bank2_reg_list,
	[3] = NULL,
	[4] = bank4_reg_list,
#else
	[1] = NULL,
	[2] = NULL,
	[3] = NULL,
	[4] = NULL,
#endif
};

static int icm426debug_reg_dump(struct seq_file *s, void *unused)
{
	int bank;
	int i;
	u8 data[4] = {0, 0, 0, 0};

	for (bank = 0; bank <= MAX_REG_BANK_NUM; bank++) {
		if (!reg_list[bank])
			continue;
		seq_printf(s, "\n################ BANK %d ################\n",
			   bank);
		i = 0;

		/* Select Bank */
		if (bank != 0) {
			data[0] = bank;
			s_spi_handle->write(s_spi_handle,
					 ICM426_REG_REG_BANK_SEL, data, 1);
		}

		while (reg_list[bank][i].size != 0) {
			s_spi_handle->read(s_spi_handle, reg_list[bank][i].addr,
					data, reg_list[bank][i].size);
			switch (reg_list[bank][i].size) {
			case 1:
				seq_printf(s, "%-25s (0x%02x): 0x%02x\n",
					   reg_list[bank][i].name,
					   reg_list[bank][i].addr,
					   data[0]);
				break;
			case 2:
				seq_printf(s, "%-25s (0x%02x): 0x%04x\n",
					   reg_list[bank][i].name,
					   reg_list[bank][i].addr,
					   ((u16)(data[1]) << 8)
					   + data[0]);
				break;
			case 3:
				seq_printf(s, "%-25s (0x%02x): 0x%06x\n",
					   reg_list[bank][i].name,
					   reg_list[bank][i].addr,
					   ((u32)(data[2]) << 16)
					   + ((u32)(data[1]) << 8)
					   + data[0]);
				break;
			case 4:
				seq_printf(s, "%-25s (0x%02x): 0x%06x\n",
					   reg_list[bank][i].name,
					   reg_list[bank][i].addr,
					   ((u32)(data[2]) << 24)
					   + ((u32)(data[2]) << 16)
					   + ((u32)(data[1]) << 8)
					   + data[0]);
				break;
			}
			i++;
		}
	}

	/* Select Bank 0 */
	data[0] = 0;
	s_spi_handle->write(s_spi_handle, ICM426_REG_REG_BANK_SEL, data, 0);

	return 0;
}

static int icm426debug_reg_dump_open(struct inode *inode, struct file *file)
{
	return single_open(file, icm426debug_reg_dump, NULL);
}

static const struct file_operations icm426_debug_reg_dump_ops = {
	.open		= icm426debug_reg_dump_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

int icm426debug_setup(struct sieimu_spi *spi_handle)
{
	struct dentry *root;

	s_spi_handle = spi_handle;

	root = debugfs_create_dir("icm426", NULL);

	if (IS_ERR(root) || !root)
		return -EINVAL;

	if (!debugfs_create_file("log", 0444,
				 root, NULL, &icm426_debug_log_ops)) {
		pr_err("Failed to create icm426 log debugfs file\n");
		return -EINVAL;
	}

	if (!debugfs_create_file("hist", 0444,
				 root, NULL, &icm426_debug_hist_ops)) {
		pr_err("Failed to create icm426 hist debugfs file\n");
		return -EINVAL;
	}

	if (!debugfs_create_file("reg_dump", 0444,
				 root, NULL, &icm426_debug_reg_dump_ops)) {
		pr_err("Failed to create icm426 reg_dump debugfs file\n");
		return -EINVAL;
	}

	return 0;
}

