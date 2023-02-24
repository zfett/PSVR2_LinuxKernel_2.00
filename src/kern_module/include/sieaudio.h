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

#pragma once

#include <linux/ioctl.h>
#include "sce_km_defs.h"

typedef enum {
	SIE_AUDIO_INPUT_PORT_DP = 1,
	/* SIE_AUDIO_INPUT_PORT_HDMI, */
} SIE_AUDIO_INPUT_PORT_TYPE;

struct audio_io_transfer_start_req {
	SIE_AUDIO_INPUT_PORT_TYPE port_type;
	unsigned int rate;
};

typedef enum {
	SIE_AUDIO_CAPTURE_PORT_NONE = 0,
	SIE_AUDIO_CAPTURE_PORT_MIC_TO_USB = 1,
#if defined(CONFIG_SIE_DEVELOP_BUILD) || defined(ENABLE_DEVELOP_BUILD)
	SIE_AUDIO_CAPTURE_PORT_MIC = 2,
	SIE_AUDIO_CAPTURE_PORT_DP = 3,
#endif
	SIE_AUDIO_CAPTURE_PORT_MAX,
} SIE_AUDIO_CAPTURE_PORT_TYPE;

struct audio_io_capture_read_req {
	SIE_AUDIO_CAPTURE_PORT_TYPE port;
	void *buf;
	unsigned int size;
};

struct audio_io_playback_write_req {
	void *buf;
	unsigned int size;
};

#define SIE_AUDIO_DEVICE_IOC_TYPE 'A'
#define SIE_AUDIO_DEVCE_NAME "audio"

#define SIE_AUDIO_IO_TRANSFER_START		_IOW(SIE_AUDIO_DEVICE_IOC_TYPE, 1, struct audio_io_transfer_start_req)
#define SIE_AUDIO_IO_TRANSFER_STOP		_IO(SIE_AUDIO_DEVICE_IOC_TYPE, 2)
#define SIE_AUDIO_IO_PREPARE			_IO(SIE_AUDIO_DEVICE_IOC_TYPE, 3)
#define SIE_AUDIO_IO_CAPTURE_START		_IOW(SIE_AUDIO_DEVICE_IOC_TYPE, 4, SIE_AUDIO_CAPTURE_PORT_TYPE)
#define SIE_AUDIO_IO_CAPTURE_STOP		_IO(SIE_AUDIO_DEVICE_IOC_TYPE, 5)
#if defined(CONFIG_SIE_DEVELOP_BUILD) || defined(ENABLE_DEVELOP_BUILD)
#define SIE_AUDIO_IO_CAPTURE_READ		_IOWR(SIE_AUDIO_DEVICE_IOC_TYPE, 6, struct audio_io_capture_read_req)
#define SIE_AUDIO_IO_PLAYBACK_START		_IO(SIE_AUDIO_DEVICE_IOC_TYPE, 7)
#define SIE_AUDIO_IO_PLAYBACK_STOP		_IO(SIE_AUDIO_DEVICE_IOC_TYPE, 8)
#define SIE_AUDIO_IO_PLAYBACK_WRITE		_IOW(SIE_AUDIO_DEVICE_IOC_TYPE, 9, struct audio_io_playback_write_req)
#define SIE_AUDIO_IO_START_TONEGEN		_IO(SIE_AUDIO_DEVICE_IOC_TYPE, 10)
#define SIE_AUDIO_IO_STOP_TONEGEN		_IO(SIE_AUDIO_DEVICE_IOC_TYPE, 11)
#endif

#ifdef __KERNEL__
void *sieAudioGetHandle(void);
void *sieAudioGetDmaBufferAddress(void *handle);
int sieAudioGetDmaBufferSize(void *handle);
unsigned int sieAudioGetCaptureOffset(void *handle);
int sieAudioRegistUsbCallBack(void *handle, void (*callback)(void));
#endif
