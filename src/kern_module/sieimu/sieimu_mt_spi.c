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
#include "mt_spi.h"

//#define VERBOSE

static int sieimu_mt_spi_read(struct sieimu_spi *handle,
		    u8 reg, u8 *buf, u32 len)
{
	int ret = 0;
	struct mt_spi *mdata = handle->spi_info;
	struct mt_spi_transfer xfer;

	if (len > SPI_BUF_SIZE)
		return -EPERM;

#ifdef VERBOSE
	printk("BEGIN: SPI Read(len:%d)\n", len);
#endif

	mutex_lock(&handle->buf_lock);
	memset(&xfer, 0, sizeof(xfer));
	*handle->tx_buf = 0x80 | reg;
	memset(handle->tx_buf + 1, 0xFF, len);
	/* memset(rx_buf, 0, len + 1); */

	xfer.len = len + 1;
	xfer.tx_buf = handle->tx_buf;
	xfer.rx_buf = handle->rx_buf;

	ret = mt_spi_sync_send(mdata, &xfer);

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

static int sieimu_mt_spi_write(struct sieimu_spi *handle,
		     u8 reg, const u8 *buf, u32 len)
{
	struct mt_spi *mdata = handle->spi_info;
	struct mt_spi_transfer xfer;
	int ret;

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
	memset(&xfer, 0, sizeof(xfer));
	memcpy(handle->tx_buf + 1, buf, len);

	xfer.len = len + 1;
	xfer.tx_buf = handle->tx_buf;
	xfer.rx_buf = NULL;

	ret = mt_spi_sync_send(mdata, &xfer);

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

static int sieimu_mt_spi_read_fifo(struct sieimu_spi *handle,
			 u8 fifo_reg, u32 len,
			 void (*completion)(u8 *buf, u32 len, void *arg),
			 void *arg)
{
	int ret = 0;
	struct mt_spi_transfer xfer;
	struct mt_spi *mdata= handle->spi_info;

	if (len > SPI_BUF_SIZE)
		return -EPERM;
#ifdef VERBOSE
	printk("BEGIN: SPI Read Fifo(len:%d): ", len);
#endif

	mutex_lock(&handle->buf_lock);
	memset(&xfer, 0, sizeof(xfer));
	*handle->tx_buf = 0x80 | fifo_reg;
	memset(handle->tx_buf + 1, 0xFF, len);

	xfer.len = len + 1;
	xfer.tx_buf = handle->tx_buf;
	xfer.rx_buf = handle->rx_buf;

	ret = mt_spi_sync_send(mdata, &xfer);
	if (ret != 0)
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

static void sieimu_mt_spi_term(struct sieimu_spi *handle)
{
	struct mt_spi* mdata = handle->spi_info;
	struct device *dev = mdata->dev;

	mutex_destroy(&handle->buf_lock);
	if (handle->tx_buf)
		devm_kfree(dev, handle->tx_buf);
	if (handle->rx_buf)
		devm_kfree(dev, handle->rx_buf);
}

static struct sieimu_spi *sieimu_mt_spi_init(struct mt_spi *mdata)
{
	int ret = 0;
	struct device *dev = mdata->dev;
	struct sieimu_spi *handle;

	handle = devm_kzalloc(dev, sizeof(struct sieimu_spi), GFP_KERNEL);
	if (!handle) {
		ret = -ENOMEM;
		goto err;
	}

	handle->spi_info = mdata;

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

	mutex_init(&handle->buf_lock);

	handle->read = sieimu_mt_spi_read;
	handle->write = sieimu_mt_spi_write;
	handle->read_fifo = sieimu_mt_spi_read_fifo;
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

static const struct of_device_id sieimu_mt_of_match[] = {
	{.compatible = "sie,sieimu_spi" },
	{}
};

MODULE_DEVICE_TABLE(of, sieimu_mt_of_match);

static int sieimu_mt_spi_probe(struct platform_device *pdev)
{
	const struct of_device_id *of_id;
	struct mt_spi *mdata;
	struct sieimu_spi *handle;
	int ret;

	of_id = of_match_node(sieimu_mt_of_match, pdev->dev.of_node);
	if (!of_id) {
		dev_err(&pdev->dev, "failed to probe of_node\n");
		return -EINVAL;
	}

	mdata = mt_spi_probe(pdev);
	if (!mdata) {
		dev_err(&pdev->dev, "%s: get mt_spi_probe fail.\n", __func__);
		return -EINVAL;
	}

	handle = sieimu_mt_spi_init(mdata);
	if (!handle) {
		dev_err(&pdev->dev, "%s: get spi_handle fail.\n", __func__);
		return -EINVAL;
	}

	ret = sieimu_probe(handle, 0);
	if (ret != 0) {
		dev_err(&pdev->dev, "%s: sieimu_probe() fail.\n", __func__);
		return -EINVAL;
	}

	platform_set_drvdata(pdev, handle);

	return 0;
}

static int sieimu_mt_spi_remove(struct platform_device *pdev)
{
	struct sieimu_spi *handle;
	int ret;

	handle = (struct sieimu_spi*) platform_get_drvdata(pdev);
	sieimu_remove(handle);
	sieimu_mt_spi_term(handle);

	ret = mt_spi_remove(pdev);
	return ret;
}

static struct platform_driver sieimu_mt_driver = {
	.probe = sieimu_mt_spi_probe,
	.remove = sieimu_mt_spi_remove,
	.driver = {
		   .name = "sieimu_mt_spi",
		   .of_match_table = sieimu_mt_of_match,
		   },
};
module_platform_driver(sieimu_mt_driver);

MODULE_AUTHOR("Sony Interactive Entertainment Inc.");
MODULE_DESCRIPTION("SIE imu mt spi driver");
MODULE_LICENSE("GPL");
