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

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include "mt/mt-afe-common.h"
#include "mt/mt-afe-pcm.h"

#include <sce_km_defs.h>
#include <sieaudio.h>

#define DEFAULT_SAMPLING_RATE 48000

struct sieaudio_private_data {
	struct platform_device *pdev;
	int transfer_started;
#ifdef CONFIG_SIE_DEVELOP_BUILD
	int mic_capture_req_cnt;
	int playback_started;
#endif
	unsigned int rate;
};

static struct sieaudio_private_data *pdata;

static int audio_open(struct inode *inode, struct file *filp)
{
	pr_debug("%s\n", __func__);
	if (!pdata) {
		pr_err("device is not probed.\n");
		return -1;
	}
	return mt_afe_open(&pdata->pdev->dev);
}

static int audio_io_transfer_start(void *user)
{
	int ret;
	struct audio_io_transfer_start_req args;

	pr_debug("%s\n", __func__);

	ret = copy_from_user(&args, user, sizeof(struct audio_io_transfer_start_req));
	if (ret != 0) {
		return -EFAULT;
	}

	if (args.port_type != SIE_AUDIO_INPUT_PORT_DP) {
		return -EPERM;
	}

	pdata->rate = args.rate;
	pdata->transfer_started = 1;
	return mt_afe_transfer_start(&pdata->pdev->dev, AFE_INPUT_PORT_DP, pdata->rate);
}

static int audio_io_transfer_stop(void *user)
{
	pr_debug("%s\n", __func__);
	pdata->transfer_started = 0;
	return mt_afe_transfer_stop(&pdata->pdev->dev);
}

static int audio_io_prepare(void *user)
{
	pr_debug("%s\n", __func__);
	if (!pdata) {
		pr_err("device is not probed.\n");
		return -1;
	}
	pdata->rate = DEFAULT_SAMPLING_RATE;

	return mt_afe_prepare(&pdata->pdev->dev);
}

static int audio_io_capture_start(void *user)
{
	int ret;
	unsigned int capture_port;

	pr_debug("%s\n", __func__);
	if (!pdata) {
		pr_err("device is not probed.\n");
		return -1;
	}

	ret = copy_from_user(&capture_port, user, sizeof(unsigned int));
	if (ret != 0) {
		pr_err("%s: err copy_from_user:%d.\n", __func__, ret);
		return -EFAULT;
	}

	switch (capture_port) {
	case SIE_AUDIO_CAPTURE_PORT_MIC_TO_USB:
		pr_debug("%s: SIE_AUDIO_CAPTURE_PORT_MIC_TO_USB\n", __func__);
		mt_afe_enable_usb_callback(MT_AFE_MEMIF_VUL12);
		ret = mt_afe_capture_start(&pdata->pdev->dev, MT_AFE_MEMIF_VUL12);
		break;
#if defined(CONFIG_SIE_DEVELOP_BUILD)
	case SIE_AUDIO_CAPTURE_PORT_MIC:
		ret = mt_afe_capture_start(&pdata->pdev->dev, MT_AFE_MEMIF_VUL12);
		break;
	case SIE_AUDIO_CAPTURE_PORT_DP:
		ret = mt_afe_capture_start(&pdata->pdev->dev, MT_AFE_MEMIF_AWB);
		break;
#endif
	default:
		pr_err("%s: capture port error:%d\n.", __func__, capture_port);
		return -EFAULT;
	}

	return ret;
}

static int audio_io_capture_stop(void *user)
{
	int ret;
	unsigned int capture_port;

	pr_debug("%s\n", __func__);
	if (!pdata) {
		pr_err("device is not probed.\n");
		return -1;
	}

	ret = copy_from_user(&capture_port, user, sizeof(unsigned int));
	if (ret != 0) {
		pr_err("%s: err copy_from_user:%d.\n", __func__, ret);
		return -EFAULT;
	}

	switch (capture_port) {
	case SIE_AUDIO_CAPTURE_PORT_MIC_TO_USB:
		mt_afe_disable_usb_callback();
		ret = mt_afe_capture_stop(&pdata->pdev->dev, MT_AFE_MEMIF_VUL12);
		break;
#if defined(CONFIG_SIE_DEVELOP_BUILD)
	case SIE_AUDIO_CAPTURE_PORT_MIC:
		ret = mt_afe_capture_stop(&pdata->pdev->dev, MT_AFE_MEMIF_VUL12);
		break;
	case SIE_AUDIO_CAPTURE_PORT_DP:
		ret = mt_afe_capture_stop(&pdata->pdev->dev, MT_AFE_MEMIF_AWB);
		break;
#endif
	default:
		pr_err("%s: capture port error:%d\n.", __func__, capture_port);
		return -EFAULT;
	}

	return ret;
}

#ifdef CONFIG_SIE_DEVELOP_BUILD
static int audio_io_capture_read(void *user)
{
	struct audio_io_capture_read_req args;
	int dai_id;
	int ret;

	pr_debug("%s\n", __func__);
	if (!pdata) {
		pr_err("device is not probed.\n");
		return -1;
	}

	ret = copy_from_user(&args, user, sizeof(struct audio_io_capture_read_req));
	if (ret != 0) {
		return -EFAULT;
	}

	switch (args.port) {
	case SIE_AUDIO_CAPTURE_PORT_MIC:
		dai_id = MT_AFE_MEMIF_VUL12;
		break;
	case SIE_AUDIO_CAPTURE_PORT_DP:
		dai_id = MT_AFE_MEMIF_AWB;
		break;
	default:
		pr_err("%s: Not supported port:%d\n", __func__, args.port);
		return -EINVAL;
	}

	return mt_afe_capture_read(&pdata->pdev->dev, args.buf, args.size, dai_id);
}

static int audio_io_playback_start(void *user)
{
	pr_debug("%s\n", __func__);
	if (!pdata) {
		pr_err("device is not probed.\n");
		return -1;
	}
	pdata->playback_started = 1;
	return mt_afe_playback_start(&pdata->pdev->dev);
}

static int audio_io_playback_stop(void *user)
{
	pr_debug("%s\n", __func__);
	if (!pdata) {
		pr_err("device is not probed.\n");
		return -1;
	}
	pdata->playback_started = 0;
	return mt_afe_playback_stop(&pdata->pdev->dev);
}

static int audio_io_playback_write(void *user)
{
	struct audio_io_playback_write_req args;
	int ret;

	pr_debug("%s\n", __func__);
	if (!pdata) {
		pr_err("device is not probed.\n");
		return -1;
	}

	ret = copy_from_user(&args, user, sizeof(struct audio_io_playback_write_req));
	if (ret != 0) {
		return -EFAULT;
	}

	args.size = mt_afe_playback_write(&pdata->pdev->dev, args.buf, args.size);

	ret = copy_to_user(user, &args, sizeof(struct audio_io_playback_write_req));
	if (ret != 0) {
		return -EFAULT;
	}

	return 0;
}

static int audio_io_start_tonegen(void *user)
{
	if (!pdata) {
		pr_err("device is not probed.\n");
		return -1;
	}

	mt_afe_start_tonegen(&pdata->pdev->dev);

	return 0;
}

static int audio_io_stop_tonegen(void *user)
{
	if (!pdata) {
		pr_err("device is not probed.\n");
		return -1;
	}

	mt_afe_stop_tonegen(&pdata->pdev->dev);

	return 0;
}
#endif

static int audio_release(struct inode *inode, struct file *filp)
{
	pr_debug("%s\n", __func__);

	if (pdata->transfer_started == 1) {
		audio_io_transfer_stop(&pdata->pdev->dev);
	}
#ifdef CONFIG_SIE_DEVELOP_BUILD
	if (pdata->playback_started == 1) {
		audio_io_playback_stop(&pdata->pdev->dev);
	}
#endif
	return mt_afe_close(&pdata->pdev->dev);
}

static long audio_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case SIE_AUDIO_IO_TRANSFER_START:
		ret = audio_io_transfer_start((void *)arg);
		break;
	case SIE_AUDIO_IO_TRANSFER_STOP:
		ret = audio_io_transfer_stop((void *)arg);
		break;
	case SIE_AUDIO_IO_PREPARE:
		ret = audio_io_prepare((void *)arg);
		break;
	case SIE_AUDIO_IO_CAPTURE_START:
		ret = audio_io_capture_start((void *)arg);
		break;
	case SIE_AUDIO_IO_CAPTURE_STOP:
		ret = audio_io_capture_stop((void *)arg);
		break;
#ifdef CONFIG_SIE_DEVELOP_BUILD
	case SIE_AUDIO_IO_CAPTURE_READ:
		ret = audio_io_capture_read((void *)arg);
		break;
	case SIE_AUDIO_IO_PLAYBACK_START:
		ret = audio_io_playback_start((void *)arg);
		break;
	case SIE_AUDIO_IO_PLAYBACK_STOP:
		ret = audio_io_playback_stop((void *)arg);
		break;
	case SIE_AUDIO_IO_PLAYBACK_WRITE:
		ret = audio_io_playback_write((void *)arg);
		break;
	case SIE_AUDIO_IO_START_TONEGEN:
		ret = audio_io_start_tonegen((void *)arg);
		break;
	case SIE_AUDIO_IO_STOP_TONEGEN:
		ret = audio_io_stop_tonegen((void *)arg);
		break;
#endif
	default:
		WARN_ON(1);
	}

	return ret;
}

void *sieAudioGetHandle(void)
{
	pr_debug("%s: called.\n", __func__);
	if (!pdata) {
		pr_err("%s: sieaudio is not probed.\n", __func__);
		return NULL;
	}
	return pdata;
}
EXPORT_SYMBOL(sieAudioGetHandle);

void *sieAudioGetDmaBufferAddress(void *handle)
{
	struct sieaudio_private_data *pdata = (struct sieaudio_private_data *)handle;

	return mt_afe_get_capture_dma_address(&pdata->pdev->dev, MT_AFE_MEMIF_VUL12);
}
EXPORT_SYMBOL(sieAudioGetDmaBufferAddress);

int sieAudioGetDmaBufferSize(void *handle)
{
	struct sieaudio_private_data *pdata = (struct sieaudio_private_data *)handle;

	return mt_afe_get_capture_buffer_size(&pdata->pdev->dev, MT_AFE_MEMIF_VUL12);

}
EXPORT_SYMBOL(sieAudioGetDmaBufferSize);

unsigned int sieAudioGetCaptureOffset(void *handle)
{
	struct sieaudio_private_data *pdata = (struct sieaudio_private_data *)handle;

	return mt_afe_capture_pointer(&pdata->pdev->dev, MT_AFE_MEMIF_VUL12);
}
EXPORT_SYMBOL(sieAudioGetCaptureOffset);

int sieAudioRegistUsbCallBack(void *handle, void (*callback)(void))
{
	struct sieaudio_private_data *pdata = (struct sieaudio_private_data *)handle;

	pr_debug("%s: called.\n", __func__);
	return mt_afe_regist_usb_callback(&pdata->pdev->dev, callback);
}
EXPORT_SYMBOL(sieAudioRegistUsbCallBack);

static const struct file_operations fops = {
	.unlocked_ioctl = audio_ioctl,
	.open			= audio_open,
	.release		= audio_release,
};

static int audio_dev_probe(struct platform_device *pdev)
{
	int ret;

	pr_debug("%s\n", __func__);

	ret = mt_afe_pcm_dev_probe(pdev);
	if (ret < 0) {
		pr_err("Failed to _mt_afe_pcm_dev_probe\n");
		return ret;
	}
	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	pdata->pdev = pdev;

	ret = register_chrdev(SCE_KM_CDEV_MAJOR_AUDIO, SCE_KM_AUDIO_DEVICE_NAME, &fops);
	if (ret < 0) {
		pr_err("Failed to register_chrdev\n");
		return ret;
	}
	return 0;
}

static int audio_dev_remove(struct platform_device *pdev)
{
	unregister_chrdev(SCE_KM_CDEV_MAJOR_AUDIO, SCE_KM_AUDIO_DEVICE_NAME);
	mt_afe_pcm_dev_remove(pdata->pdev);
	pdata->pdev = NULL;
	kfree(pdata);
	pdata = NULL;

	return 0;
}

static const struct of_device_id mt_afe_pcm_dt_match[] = {
	{ .compatible = "sie,mt-afe-pcm", },
	{ }
};
MODULE_DEVICE_TABLE(of, mt_afe_pcm_dt_match);

static struct platform_driver mt_afe_pcm_driver = {
	.driver = {
		   .name = "mtk-afe-pcm",
		   .of_match_table = mt_afe_pcm_dt_match,
	},
	.probe = audio_dev_probe,
	.remove = audio_dev_remove,
};

module_platform_driver(mt_afe_pcm_driver);

MODULE_AUTHOR("Sony Interactive Entertainment Inc.");
MODULE_DESCRIPTION("audio driver");
MODULE_LICENSE("GPL v2");
