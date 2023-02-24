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
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <soc/mediatek/mtk_ts.h>

#include <sieimu.h>
#include "sie_icm426.h"
#include "sie_icm426_reg.h"
#include "../sieimu_spi_if.h"

#ifdef CONFIG_DEBUG_FS
#include "sie_icm426_debug.h"
#endif

#define FATAL_ERROR_TIMEOUT (1000) /* 1000ms */

#define IMU_THREAD_PRIORITY (96)
#define ICM426_RINGBUFFER_SIZE (SIE_IMU_RINGBUFFER_SIZE)

struct icm426 {
	struct mutex wp_lock; /* lock for write_pos */

	struct device *dev;
	struct device *dev_ts;
	struct sieimu_spi *spi_handle;
	struct task_struct *task;
	void (*req_callback)(struct sieimu_data *imu_data);
	struct sieimu_data *ring_buffer;
	wait_queue_head_t wait_queue;

	int gpio;
	unsigned int irq;
	int init;
	int flag;
	u32 read_pos;
	u32 write_pos;
	u32 wrap_cnt;
	u32 req_vts;
	int adjust_isr_phase;

	atomic_t isr_imu_cnt;
	s8 temperature;
};

static struct icm426 *s_icm426;

struct icm426_isr_info {
	u16 imu_cnt;
};

struct sieimu_data *sie_icm426_get_fifo_address(void)
{
	if (!s_icm426->ring_buffer) {
		WARN_ON(1);
		return NULL;
	}

	return s_icm426->ring_buffer;
}

int sie_icm426_get_fifo_size(void)
{
	return ICM426_RINGBUFFER_SIZE;
}

static int icm426_get_temperature(s8 *temperature)
{
	*temperature = s_icm426->temperature;

	return 0;
}

static int read_reg(struct icm426 *icm426, u8 reg, u8 *buf)
{
	WARN_ON(!icm426->spi_handle && !icm426->spi_handle->read);
	return icm426->spi_handle->read(icm426->spi_handle, reg, buf, 1);
}

static int write_reg(struct icm426 *icm426, u8 reg, const u8 data)
{
	WARN_ON(!icm426->spi_handle && !icm426->spi_handle->write);
	return icm426->spi_handle->write(icm426->spi_handle, reg, &data, 1);
}

static u8 get_int_status(struct icm426 *icm426, u8 *status)
{
	*status = 0;
	return read_reg(icm426, ICM426_REG_INT_STATUS, status); /* 0x2D */
}

static int reset_device(struct icm426 *icm426)
{
	int timeout = FATAL_ERROR_TIMEOUT;
	u8 status;
	/* Soft Reset */
	write_reg(icm426, ICM426_REG_DEVICE_CONFIG, BIT_SOFT_RESET_ENABLE); /* 0x11 */

	do {
		timeout--;
		if (timeout < 0) {
			return -1;
		}
		usleep_range(1000, 2000);
		get_int_status(icm426, &status);
	} while (!(status & BIT_INT_RESET_DONE_INT1_EN));

	return 0;
}

static int setup_device(struct icm426 *icm426)
{
	int ret = 0;
	u8 who_am_i;

	/* Initialize device */
	dev_info(icm426->dev, "%s: Initialize icm426", __func__);

	ret = reset_device(icm426);
	if (ret < 0) {
		dev_err(icm426->dev, "%s: reset_device fail. error:%d", __func__, ret);
		return -EIO;
	}

	/* Check WHOAMI */
	ret = read_reg(icm426, ICM426_REG_WHO_AM_I, &who_am_i); /* 0x75 */
	if (ret) {
		dev_err(icm426->dev, "%s: read icm426 whoami value fail.", __func__);
		return -EIO;
	}

	if (who_am_i == 0 || who_am_i == 0xFF) {
		dev_err(icm426->dev, "%s: Bad WHOAMI value. Got 0x%02x", __func__, who_am_i);
		return -ENXIO;
	}

	dev_info(icm426->dev, "%s: who_am_i: %x\n", __func__, who_am_i);

	/* Enable push pull on INT1 to avoid moving in Test Mode after a soft reset */
	/* Configure the INT1 interrupt pulse as active low */
	ret |= write_reg(icm426, ICM426_REG_INT_CONFIG,
		  BIT_INT1_POLARITY_ACTIVE_LOW | BIT_INT1_DRIVE_CIRCUIT_PUSH_PULL | BIT_INT1_PULSED_MODE); /* 0x14 */

	/* Setup FIFO mode */
	ret |= write_reg(icm426, ICM426_REG_FIFO_CONFIG, BIT_FIFO_MODE_STOP_FULL); /* 0x16 */
	ret |= write_reg(icm426, ICM426_REG_INTF_CONFIG0, BIT_FIFO_COUNT_REC | BIT_UI_SIFS_DISABLE_I2C); /* 0x4C */

	/* set_gyro_fsr & set_gyro_frequency (+-2000dps, 2000Hz) */
	ret |= write_reg(icm426, ICM426_REG_GYRO_CONFIG0, BIT_GYRO_FSR_2000 | BIT_GYRO_ODR_2000); /* 0x4F */
	/* set_accel_fsr & set_accel_frequency (+-4G, 2000Hz) */
	ret |= write_reg(icm426, ICM426_REG_ACCEL_CONFIG0, BIT_ACCEL_FSR_4G | BIT_ACCEL_ODR_2000); /* 0x50 */

	/* Set the UI filter order to 2 */
	ret |= write_reg(icm426, ICM426_REG_GYRO_CONFIG1,
		  BIT_TEMP_FILT_BW_BYPASS | BIT_GYR_AVG_FLT_RATE_8KHZ | BIT_GYR_UI_FILT_ORD_IND_2 | BIT_GYR_DEC2_M2_ORD_3); /* 0x51 */
	ret |= write_reg(icm426, ICM426_REG_ACCEL_CONFIG1,
		  BIT_ACC_UI_FILT_ODR_IND_2 | BIT_ACC_DEC2_M2_ORD_3 | BIT_ACC_AVG_FLT_RATE_8KHZ); /* 0x53 */

	/* Enable timestamp and set timestamp resolution 1us */
	ret |= write_reg(icm426, ICM426_REG_TMST_CONFIG, BIT_EN_DREG_FIFO_D2A | BIT_TMST_RESOL_1US | BIT_TMST_EN); /* 0x54 */

	/* Enable DATA_READY Interrupt */
	ret |= write_reg(icm426, ICM426_REG_INT_SOURCE0, BIT_INT_UI_DRDY_INT1_EN); /* 0x65 */

	/* enable accel/gyro/temp FIFO */
	ret |= write_reg(icm426, ICM426_REG_FIFO_CONFIG1, BIT_FIFO_ACCEL_EN | BIT_FIFO_GYRO_EN | BIT_FIFO_TEMP_EN); /* 0x5F */

	/* wr_gyro_accel_config0_accel_filt_bw */
	ret |= write_reg(icm426, ICM426_REG_ACCEL_GYRO_CONFIG0, BIT_ACCEL_UI_LNM_BW_4_IIR | BIT_GYRO_UI_LNM_BW_4_IIR); /* 0x52 */

	/* Enable accel and gyro in low noise mode */
	ret |= write_reg(icm426, ICM426_REG_PWR_MGMT_0, BIT_GYRO_MODE_LNM | BIT_ACCEL_MODE_LNM); /* 0x4E */
	{
		unsigned long start_time = (INV_ICM42600_ACCEL_START_TIME > INV_ICM42600_GYRO_START_TIME) ?
			INV_ICM42600_ACCEL_START_TIME : INV_ICM42600_GYRO_START_TIME;
		usleep_range(start_time * 1000, start_time * 1000 + 100);
	}

	/* reset_fifo */
	ret |= write_reg(icm426, ICM426_REG_SIGNAL_PATH_RESET, BIT_FIFO_FLUSH); /* 0x4B */

	if (ret) {
		dev_err(icm426->dev, "%s: Initialize IMU registers fail. error:%d", __func__, ret);
		return -ENXIO;
	}

	return 0;
}

static int request_data(struct icm426 *icm426, void(*completion)(u8 *buf, u32 len, void *arg), void *arg)
{
	int ret;
	u8 int_status;
	u16 packet_count = 0;

	ret = get_int_status(icm426, &int_status);
	if (ret < 0) {
		return -1;
	}

	if ((int_status & BIT_INT_UI_DRDY_INT1_EN) || (int_status & BIT_INT_FIFO_FULL_INT1_EN)) {
		u8 data[2];

		/* FIFO record mode configured at driver init, so we read packet number, not byte count */
		if (icm426->spi_handle->read(icm426->spi_handle, ICM426_REG_FIFO_BYTE_COUNT1, data, 2))
			return -1;

		packet_count = (u16)(data[0] | (data[1] << 8));
		if (packet_count > 0) {
			if (icm426->spi_handle->read_fifo(icm426->spi_handle, ICM426_REG_FIFO_DATA,
							  FIFO_PACKET_BYTE_6X * packet_count, completion, arg)) {
				/* sensor data is in FIFO according to FIFO_COUNT but failed to read FIFO, */
				/* reset FIFO and try next chance */
				write_reg(icm426, ICM426_REG_SIGNAL_PATH_RESET, BIT_FIFO_FLUSH); /* 0x4B */
				return -1;
			}
		}
	}

	return 0;
}

static int parse_data(u8 *buf, struct sieimu_data *imu_data, u16 *imu_ts, s8 *temperature)
{
	int fifo_idx = 0;
	union icm426_fifo_header_t header;

	header = *(union icm426_fifo_header_t *)&buf[fifo_idx];
	fifo_idx += sizeof(union icm426_fifo_header_t);

#ifdef CONFIG_SIE_DEVELOP_BUILD
	WARN_ON(header.bits.twentybits_bit);
	WARN_ON(header.bits.fsync_bit);
	WARN_ON(header.bits.msg_bit);
#endif

	if (header.bits.accel_bit && header.bits.gyro_bit) {
		memcpy(imu_data, &buf[fifo_idx], 12);
		fifo_idx += 12;
	} else if (header.bits.accel_bit) {
		memcpy(&imu_data->accel, &buf[fifo_idx], 6);
		fifo_idx += 6;
		imu_data->gyro[0] = INVALID_VALUE_FIFO;
		imu_data->gyro[1] = INVALID_VALUE_FIFO;
		imu_data->gyro[2] = INVALID_VALUE_FIFO;
	} else if (header.bits.gyro_bit) {
		imu_data->accel[0] = INVALID_VALUE_FIFO;
		imu_data->accel[1] = INVALID_VALUE_FIFO;
		imu_data->accel[2] = INVALID_VALUE_FIFO;
		memcpy(&imu_data->gyro, &buf[fifo_idx], 6);
		fifo_idx += 6;
	} else {
		imu_data->accel[0] = INVALID_VALUE_FIFO;
		imu_data->accel[1] = INVALID_VALUE_FIFO;
		imu_data->accel[2] = INVALID_VALUE_FIFO;
		imu_data->gyro[0] = INVALID_VALUE_FIFO;
		imu_data->gyro[1] = INVALID_VALUE_FIFO;
		imu_data->gyro[2] = INVALID_VALUE_FIFO;
	}

	if ((header.bits.accel_bit) || (header.bits.gyro_bit)) {
		*temperature = (s8)buf[fifo_idx];
		fifo_idx++;
	} else {
		*temperature = INVALID_VALUE_FIFO_1B;
	}

	if (header.bits.timestamp_bit) {
		*imu_ts = (buf[1 + fifo_idx] << 8) | buf[0 + fifo_idx];
		fifo_idx += 2;
	} else {
		*imu_ts = INVALID_VALUE_FIFO;
	}

	return fifo_idx;
}

static int icm426_get_write_pos(struct sieimu_get_write_pos *get_write_pos)
{
	mutex_lock(&s_icm426->wp_lock);
	get_write_pos->wrap_cnt = s_icm426->wrap_cnt;
	get_write_pos->write_pos = s_icm426->write_pos;
	mutex_unlock(&s_icm426->wp_lock);

#ifdef CONFIG_DEBUG_FS
	icm426debug_get_write_pos_trace(get_write_pos->wrap_cnt, get_write_pos->write_pos);
#endif

	return 0;
}

int sie_icm426_request_callback(void (*callback)(struct sieimu_data *ring_buffer))
{
	u32 vts;
	u64 stc;
	u32 dp_cnt;

	mtk_ts_get_current_time(s_icm426->dev_ts, &vts, &stc, &dp_cnt);

	s_icm426->req_callback = callback;
	s_icm426->req_vts = vts;

#ifdef CONFIG_DEBUG_FS
	icm426debug_req_callback_trace(vts);
#endif

	return 0;
}

static void fifo_receive_callback(u8 *buf, u32 len, void *arg)
{
	struct icm426_isr_info *isr_info = (struct icm426_isr_info *)arg;
	u32 imu_vts;
	u32 dp_cnt;
	u16 imu_cnt;
	u32 tmp_write_pos;
	u16 imu_ts;
	s8 temperature = INVALID_VALUE_FIFO_1B;
	u32 i;

	mtk_ts_get_imu_0_time(s_icm426->dev_ts, &imu_vts, &dp_cnt, &imu_cnt);

	i = 0;
	while (i < len) {
		u16 eod = 0;
		u16 invalid_vts = 1;

		tmp_write_pos = s_icm426->write_pos;

		i += parse_data(buf + i, &s_icm426->ring_buffer[tmp_write_pos], &imu_ts, &temperature);

#ifdef CONFIG_SIE_DEVELOP_BUILD
		WARN_ON(i > len); /* Check Read Position does not over len */
#endif
		if (i >= len) {
			if (isr_info->imu_cnt == imu_cnt) {
				/* Latest packet(imu_cnt is not updated) only become valid. */
				invalid_vts = 0;
			}
			eod = 1;
		}

		s_icm426->ring_buffer[tmp_write_pos].imu_ts = imu_ts;
		s_icm426->ring_buffer[tmp_write_pos].vts = imu_vts;
		s_icm426->ring_buffer[tmp_write_pos].dp_frame_cnt = dp_cnt & 0xFFFF;
		s_icm426->ring_buffer[tmp_write_pos].dp_line_cnt = dp_cnt >> 16;
		s_icm426->ring_buffer[tmp_write_pos].status = 0;
		if (invalid_vts)
			s_icm426->ring_buffer[tmp_write_pos].status |= SIE_IMU_STATUS_INVALID;

		/* smp_mb(); */
		mutex_lock(&s_icm426->wp_lock);
		s_icm426->write_pos = (s_icm426->write_pos + 1) % ICM426_RINGBUFFER_SIZE;
		if (s_icm426->write_pos == 0)
			s_icm426->wrap_cnt++;
		mutex_unlock(&s_icm426->wp_lock);

#ifdef CONFIG_DEBUG_FS
		{
			u32 vts;
			u64 stc;

			mtk_ts_get_current_time(s_icm426->dev_ts, &vts, &stc, &dp_cnt);
			icm426debug_data_trace(vts, imu_vts, imu_ts, imu_cnt,
					       s_icm426->write_pos, s_icm426->wrap_cnt, eod, invalid_vts);
		}
#endif

		if (eod == 1 && s_icm426->req_callback) {
			void (*_callback)(struct sieimu_data *imu_data);
			u32 vts;
			u64 stc;

			mtk_ts_get_current_time(s_icm426->dev_ts, &vts, &stc, &dp_cnt);
			_callback = s_icm426->req_callback;
			if (vts - s_icm426->req_vts > IMU_INTERVAL)
				s_icm426->adjust_isr_phase = 1;
			else
				s_icm426->adjust_isr_phase = 0;
			s_icm426->req_callback = NULL;
			(*_callback)(&s_icm426->ring_buffer[s_icm426->write_pos]);
#ifdef CONFIG_DEBUG_FS
			icm426debug_req_callback_done_trace(vts, vts - s_icm426->req_vts, s_icm426->adjust_isr_phase,
							    s_icm426->wrap_cnt,  s_icm426->write_pos);
#endif
		}
	} /* end of FIFO read for loop */

	/* new temperature data */
	if (temperature != INVALID_VALUE_FIFO_1B)
		s_icm426->temperature = temperature;
}

static int icm426_thread(void *arg)
{
	struct icm426 *icm426 = (struct icm426 *)arg;
	struct icm426_isr_info isr_info;
	int err_cnt = 0;
	int ret;

	while (!kthread_should_stop()) {
		icm426->flag = 0;
		ret = wait_event_interruptible_timeout(icm426->wait_queue, icm426->flag != 0,
						       msecs_to_jiffies(FATAL_ERROR_TIMEOUT));
		if (ret < 0) {
			continue;
		} else if (ret == 0) {
			panic("IMU: Interrupt timeout");
		}
		isr_info.imu_cnt = atomic_read(&icm426->isr_imu_cnt);
		if (request_data(icm426, fifo_receive_callback, (void *)&isr_info) < 0) {
			err_cnt++;
			if (err_cnt > FATAL_ERROR_TIMEOUT) {
				panic("IMU: Exceed read error count");
			}
		} else {
			err_cnt = 0;
		}
	}

	return 0;
}

static irqreturn_t icm426_irq_isr(int irq, void *arg)
{
	struct icm426 *icm426 = (struct icm426 *)arg;
	static s32 sample_cnt = IMU_SAMPLE_RATIO;
	u32 imu_vts;
	u32 dp_cnt;
	u16 imu_cnt;

	sample_cnt--;
	if (sample_cnt > 0 && icm426->adjust_isr_phase == 0)
		return 0;
	icm426->adjust_isr_phase = 0;
	sample_cnt = IMU_SAMPLE_RATIO;

	mtk_ts_get_imu_0_time(s_icm426->dev_ts, &imu_vts, &dp_cnt, &imu_cnt);

	atomic_set(&icm426->isr_imu_cnt, imu_cnt);

#ifdef CONFIG_DEBUG_FS
	{
		u64 stc;
		u32 vts;

		mtk_ts_get_current_time(icm426->dev_ts, &vts, &stc, &dp_cnt);
		icm426debug_isr_trace(vts, sample_cnt, imu_vts, imu_cnt);
	}
#endif

	icm426->flag = 1;
	wake_up_interruptible(&icm426->wait_queue);

	return IRQ_HANDLED;
}

static int icm426_init(void)
{
	struct icm426 *icm426 = s_icm426;
	int ret;

	/* Initialize icm426 */
	ret = setup_device(icm426);
	if (ret) {
		dev_err(icm426->dev, "%s: setup device fail. error:%d", __func__, ret);
		return -EIO;
	}

	icm426->task = kthread_run(icm426_thread, icm426, "icm426_thread");
	if (IS_ERR(icm426->task)) {
		dev_err(icm426->dev, "%s: kthread_run fail. error:%ld\n", __func__,
			PTR_ERR(icm426->task));
		return -EPERM;
	}
#ifdef IMU_THREAD_PRIORITY
	{
		struct sched_param param;

		param.sched_priority = IMU_THREAD_PRIORITY;
		sched_setscheduler(icm426->task, SCHED_RR, &param);
	}
#endif

	return 0;
}

static void icm426_term(void)
{
	struct icm426 *icm426 = s_icm426;

	kthread_stop(icm426->task);
	devm_free_irq(icm426->dev, icm426->irq, icm426);
}

static int icm426_get_device(struct device *dev,
			     char *compatible, int idx, struct device **child_dev)
{
	struct platform_device *pdev;
	struct device_node *node;

	node = of_parse_phandle(dev->of_node, compatible, idx);
	if (!node) {
		dev_err(dev, "%s: icm426: get_device: could not find %s %d\n",
			__func__, compatible, idx);
		return -ENODEV;
	}

	pdev = of_find_device_by_node(node);
	if (!pdev || !pdev->dev.driver) {
		dev_warn(dev,
			 "%s: get_device: waiting for device %s\n",
			 __func__, node->full_name);
		return -EPROBE_DEFER;
	}

	*child_dev = &pdev->dev;

	return 0;
}

static int icm426_open(struct inode *inode, struct file *filp)
{
	if (s_icm426->init != 0)
		return -EINVAL;

	s_icm426->init = 1;
	return icm426_init();
}

static int icm426_release(struct inode *inode, struct file *filp)
{
	if (s_icm426->init == 0)
		return -EINVAL;

	s_icm426->init = 0;
	icm426_term();
	return 0;
}

static int icm426_mmap(struct file *filp, struct vm_area_struct *vma)
{
	size_t size = vma->vm_end - vma->vm_start;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;

	if (offset != 0) {
		pr_err("%s: offset must be zeeo.\n", __func__);
		return -EPERM;
	}

	if ((size >> PAGE_SHIFT) != (sizeof(struct sieimu_data) * ICM426_RINGBUFFER_SIZE + (1 << PAGE_SHIFT) - 1) >> PAGE_SHIFT) {
		pr_err("%s: size error. %lu.\n", __func__, size);
		pr_err("%s: size error. %lu vs %lu.\n", __func__, (size >> PAGE_SHIFT), (sizeof(struct sieimu_data) * ICM426_RINGBUFFER_SIZE) >> PAGE_SHIFT);
		return -EPERM;
	}

	if (remap_pfn_range(vma, vma->vm_start, __pa(s_icm426->ring_buffer) >> PAGE_SHIFT,
			    size, vma->vm_page_prot)) {
		pr_err("%s: remap_pfn_range error.\n", __func__);
		return -EAGAIN;
	}
	return 0;
}

static long icm426_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case SIE_IMU_IO_GET_TEMPERATURE:
		{
			s8 temperature = INVALID_VALUE_FIFO_1B;

			if (icm426_get_temperature(&temperature)) {
				pr_err("icm426_get_temperature failed.\n");
				return -EINVAL;
			}
			if (copy_to_user((void *)arg, &temperature, sizeof(s8))) {
				WARN_ON(1);
				return -EINVAL;
			}
		}
		break;
	case SIE_IMU_IO_GET_WRITE_POS:
		{
			struct sieimu_get_write_pos get_write_pos;

			if (icm426_get_write_pos(&get_write_pos)) {
				pr_err("icm426_get_write_pos failed.\n");
				return -EINVAL;
			}
			if (copy_to_user((void *)arg, &get_write_pos, sizeof(get_write_pos))) {
				WARN_ON(1);
				return -EINVAL;
			}
		}
		break;
	default:
		pr_err("Illegal ioctl\n");
		return -EINVAL;
	}

	return 0;
}

static const struct file_operations fops = {
	.unlocked_ioctl	= icm426_ioctl,
	.mmap			= icm426_mmap,
	.open			= icm426_open,
	.release		= icm426_release,
};

static void *icm426_allocate_free_page(struct device *dev, u32 size)
{
	int allocorder = 0;
	int allocsize = PAGE_SIZE;

	while (allocsize < size) {
		allocsize *= 2;
		allocorder++;
	}

	return (void *)devm_get_free_pages(dev, GFP_KERNEL, allocorder);
}

int sie_icm426_probe(struct sieimu_spi *handle)
{
	struct device *dev = handle->dev;
	int ret = 0;

	dev_dbg(dev, "%s\n", __func__);
	if (s_icm426) {
		dev_err(dev, "%s: icm426 has already been probed.", __func__);
		return -EBUSY;
	}

	s_icm426 = devm_kzalloc(dev, sizeof(struct icm426), GFP_KERNEL);
	if (!s_icm426) {
		ret = -ENOMEM;
		goto err;
	}

	s_icm426->dev = dev;
	s_icm426->spi_handle = handle;

	s_icm426->ring_buffer = icm426_allocate_free_page(dev, sizeof(struct sieimu_data) * ICM426_RINGBUFFER_SIZE);
	if (!s_icm426->ring_buffer) {
		ret = -ENOMEM;
		goto err;
	}

	ret = icm426_get_device(dev, "mediatek,timestamp", 0, &s_icm426->dev_ts);
	if (ret) {
		dev_err(dev, "%s: get timestamp fail. error:%d\n", __func__, ret);
		goto err;
	}

	init_waitqueue_head(&s_icm426->wait_queue);

	s_icm426->gpio = of_get_gpio(dev->of_node, 0);
	pr_info("%s gpio:%d\n", __func__, s_icm426->gpio);

	ret = devm_gpio_request_one(dev, s_icm426->gpio, GPIOF_IN, "IMU_INT_PIN");
	if (ret < 0) {
		dev_err(dev, "%s: request GPIO %d fail, error:%d\n", __func__, s_icm426->gpio, ret);
		goto err;
	}

	ret = gpio_to_irq(s_icm426->gpio);
	if (ret < 0) {
		dev_err(dev, "%s: Unable to get irq number for GPIO %d, error:%d\n", __func__, s_icm426->gpio, ret);
		goto err;
	}
	pr_info("%s irq:%d\n", __func__, ret);
	s_icm426->irq = ret;

	mutex_init(&s_icm426->wp_lock);

	s_icm426->temperature = INVALID_VALUE_FIFO_1B;

	mtk_ts_select_imu_0_trigger_edge(s_icm426->dev_ts, MTK_TS_IMU_FALL_EDGE);

	ret = devm_request_any_context_irq(s_icm426->dev, s_icm426->irq,
					   icm426_irq_isr,
					   IRQF_TRIGGER_FALLING,
					   "IMU_INT", s_icm426);
	if (ret < 0) {
		dev_err(s_icm426->dev, "%s: Unable to claim irq %d. error:%d\n", __func__, s_icm426->irq, ret);
		goto err;
	}

	ret = register_chrdev(SCE_KM_CDEV_MAJOR_IMU, SCE_KM_IMU_DEVICE_NAME, &fops);
	if (ret < 0) {
		dev_err(dev, "%s: register_chrdev fail.\n", __func__);
		goto err;
	}

#ifdef CONFIG_DEBUG_FS
	icm426debug_setup(s_icm426->spi_handle);
#endif

	return 0;

err:
	if (s_icm426) {
		if (s_icm426->ring_buffer)
			devm_free_pages(dev, (unsigned long)s_icm426->ring_buffer);
		if (s_icm426->irq != 0) {
			if (s_icm426->gpio != 0) {
				devm_gpio_free(dev, s_icm426->gpio);
				s_icm426->gpio = 0;
			}
		}
		devm_kfree(dev, s_icm426);
		s_icm426 = NULL;
	}
	return ret;
}

int sie_icm426_remove(struct sieimu_spi *handle)
{
	struct device *dev = handle->dev;

	unregister_chrdev(SCE_KM_CDEV_MAJOR_IMU, SCE_KM_IMU_DEVICE_NAME);
	if (s_icm426) {
		mutex_destroy(&s_icm426->wp_lock);
		if (s_icm426->ring_buffer)
			devm_free_pages(dev, (unsigned long)s_icm426->ring_buffer);
		if (s_icm426->irq != 0) {
			if (s_icm426->gpio != 0) {
				devm_gpio_free(dev, s_icm426->gpio);
				s_icm426->gpio = 0;
			}
		}
		devm_kfree(s_icm426->dev, s_icm426);
		s_icm426 = NULL;
	}
	return 0;
}

