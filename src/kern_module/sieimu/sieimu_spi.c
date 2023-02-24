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

#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include "sieimu_spi_if.h"

//#define VERBOSE

static int sieimu_spi_read(struct sieimu_spi *handle,
		    u8 reg, u8 *buf, u32 len)
{
	int ret = 0;
	struct spi_message msg;
	struct spi_transfer xfer;
	struct spi_device *spi = (struct spi_device*)handle->spi_info;

	if (len > SPI_BUF_SIZE)
		return -EPERM;


#ifdef VERBOSE
	printk("BEGIN: SPI Read(len:%d)\n", len);
#endif

	spi_message_init(&msg);

	mutex_lock(&handle->buf_lock);
	memset(&xfer, 0, sizeof(xfer));
	*handle->tx_buf = 0x80 | reg;
	memset(handle->tx_buf + 1, 0xFF, len);
	/* memset(rx_buf, 0, len + 1); */

	xfer.len = len + 1;
	xfer.tx_buf = handle->tx_buf;
	xfer.rx_buf = handle->rx_buf;

	spi_message_add_tail(&xfer, &msg);

	ret = spi_sync(spi, &msg);

	memcpy(buf, handle->rx_buf + 1, len);
	mutex_unlock(&handle->buf_lock);

#ifdef VERBOSE
	{
		unsigned int i;

		printk("END: SPI Read(len:%d/ret:%d): 0x%02x:", len, ret, reg);
		for (i = 0; i < len; i++)
			printk(" %02x", *(buf + i));
		printk("\n");
	}
#endif

	return ret;
}

static int sieimu_spi_write(struct sieimu_spi *handle,
		     u8 reg, const u8 *buf, u32 len)
{
	struct spi_device *spi = (struct spi_device*)handle->spi_info;
	int ret;

	if (!handle)
		return -EPERM;

#ifdef VERBOSE
	printk("BEGIN: SPI Write(len:%d): 0x%02x:", len, reg);
#endif

	if (len > 1) {
		pr_err("%s: Not support burst write.\n", __func__);
#ifdef CONFIG_SIE_DEVELOP_BUILD
		WARN_ON(1);
#endif
		return -EPERM;
	}

	*handle->tx_buf = reg;

	mutex_lock(&handle->buf_lock);
	memcpy(handle->tx_buf + 1, buf, len);

	ret = spi_write(spi, handle->tx_buf, len + 1);

#ifdef VERBOSE
	{
		int i;

		printk("END: SPI Write(len:%d): 0x%02x:", len, reg);
		for (i = 1; i <= len; i++)
			printk(" %02x", *(handle->tx_buf + i));
		printk("\n");
	}
#endif

	mutex_unlock(&handle->buf_lock);

	return ret;
}

static int sieimu_spi_read_fifo(struct sieimu_spi *handle,
			 u8 fifo_reg, u32 len,
			 void (*completion)(u8 *buf, u32 len, void *arg),
			 void *arg)
{
	int ret = 0;
	struct spi_device *spi = (struct spi_device*)handle->spi_info;
	struct spi_message msg;
	struct spi_transfer xfer;

	if (len > SPI_BUF_SIZE)
		return -EPERM;
#ifdef VERBOSE
	printk("BEGIN: SPI Read Fifo(len:%d): ", len);
#endif

	spi_message_init(&msg);

	mutex_lock(&handle->buf_lock);
	memset(&xfer, 0, sizeof(xfer));
	*handle->tx_buf = 0x80 | fifo_reg;
	memset(handle->tx_buf + 1, 0xFF, len);

	xfer.len = len + 1;
	xfer.tx_buf = handle->tx_buf;
	xfer.rx_buf = handle->rx_buf;

	spi_message_add_tail(&xfer, &msg);

	ret = spi_sync(spi, &msg);
	if (ret < 0)
		goto err;

#ifdef VERBOSE
	{
		unsigned int i;

		printk("END: SPI Read Fifo(len:%d): ", len);
		for (i = 1; i <= len; i++)
			printk(" %02x", *(handle->rx_buf + i));
		printk("\n");
	}
#endif

	(*completion)(handle->rx_buf + 1, len, arg);

err:
	mutex_unlock(&handle->buf_lock);

	return ret;
}

static void sieimu_spi_term(struct sieimu_spi *handle)
{
	struct spi_device *spi = (struct spi_device*)handle->spi_info;
	struct device *dev = &spi->dev;

	mutex_destroy(&handle->buf_lock);
	if (handle->tx_buf)
		devm_kfree(dev, handle->tx_buf);
	if (handle->rx_buf)
		devm_kfree(dev, handle->rx_buf);
}

static struct sieimu_spi *sieimu_spi_init(struct spi_device *spi)
{
	int ret = 0;
	struct device *dev = &spi->dev;
	struct sieimu_spi *handle;

	handle = devm_kzalloc(dev, sizeof(struct sieimu_spi), GFP_KERNEL);
	if (!handle) {
		ret = -ENOMEM;
		goto err;
	}

	handle->spi_info = (void*)spi;

	handle->tx_buf =
		devm_kzalloc(dev, SPI_BUF_SIZE + 1, GFP_KERNEL | GFP_DMA);
	if (!handle->tx_buf) {
		ret = -ENOMEM;
		goto err;
	}

	handle->rx_buf =
		devm_kzalloc(dev, SPI_BUF_SIZE + 1, GFP_KERNEL | GFP_DMA);
	if (!handle->rx_buf) {
		ret = -ENOMEM;
		goto err;
	}

	spi->mode = SPI_MODE_0;
	ret = spi_setup(spi);
	if (ret != 0) {
		dev_err(dev, "spi_setup error :%x\n", ret);
		goto err;
	}

	mutex_init(&handle->buf_lock);

	handle->read = sieimu_spi_read;
	handle->write = sieimu_spi_write;
	handle->read_fifo = sieimu_spi_read_fifo;
	handle->dev = dev;

	return handle;

err:
	if (handle) {
		if (handle->tx_buf)
			devm_kfree(dev, handle->tx_buf);
		if (handle->rx_buf)
			devm_kfree(dev, handle->rx_buf);
		devm_kfree(dev, handle);
	}
	return NULL;
}

static int sieimu_spi_probe(struct spi_device *spi)
{
	const struct spi_device_id *id = spi_get_device_id(spi);
	struct sieimu_spi *handle;
	int ret = 0;

	handle = sieimu_spi_init(spi);
	if (!handle) {
		dev_err(&spi->dev, "%s: get sieimu_spi_init fail.\n", __func__);
		return -EINVAL;
	}

	ret = sieimu_probe(handle, id->driver_data);
	if (ret != 0) {
		dev_err(&spi->dev, "%s: sieimu_probe() fail.\n", __func__);
		return -EINVAL;
	}

	spi_set_drvdata(spi, handle);

	return 0;
}

static int sieimu_spi_remove(struct spi_device *spi)
{
	struct sieimu_spi *handle;

	handle = (struct sieimu_spi*) spi_get_drvdata(spi);
	sieimu_remove(handle);
	sieimu_spi_term(handle);

	spi_set_drvdata(spi, NULL);

	return 0;
}

static const struct spi_device_id sieimu_id[] = {
	{"icm426xx", 0},
	{}
};

MODULE_DEVICE_TABLE(spi, sieimu_id);

#ifdef CONFIG_OF
static const struct of_device_id sieimu_of_match[] = {
	{ .compatible = "sie,icm426xx" },
	{ },
};
MODULE_DEVICE_TABLE(of, sieimu_of_match);
#else
#define sieimu_of_match NULL
#endif

static struct spi_driver sieimu_driver = {
	.probe		= sieimu_spi_probe,
	.remove		= sieimu_spi_remove,
	.id_table	= sieimu_id,
	.driver = {
		.of_match_table		= of_match_ptr(sieimu_of_match),
		.name			= "sieimu",
	},
};
module_spi_driver(sieimu_driver);

MODULE_AUTHOR("Sony Interactive Entertainment Inc.");
MODULE_DESCRIPTION("SIE imu spi driver");
MODULE_LICENSE("GPL");
