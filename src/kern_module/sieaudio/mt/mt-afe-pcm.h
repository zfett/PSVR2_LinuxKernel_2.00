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

#ifndef _MT_AFE_PCM_H_
#define _MT_AFE_PCM_H_
#include <sound/asound.h>

enum afe_base_clock_setting {
	AFE_BASE_CLOCK_INTERNAL, /* Use 26M internal clock */
	AFE_BASE_CLOCK_EXTERNAL, /* Sync to DP-RX clock */
};

enum afe_input_port_type {
	AFE_INPUT_PORT_NONE = 0,
	AFE_INPUT_PORT_DP,
};

/* mt/mt-afe-pcm.c */
int mt_afe_pcm_dev_probe(struct platform_device *pdev);
int mt_afe_pcm_dev_remove(struct platform_device *pdev);

int mt_afe_open(struct device *dev);
int mt_afe_prepare(struct device *dev);
int mt_afe_close(struct device *dev);

int mt_afe_capture_start(struct device *dev, const int dai_id);
int mt_afe_capture_stop(struct device *dev, const int dai_id);
#ifdef CONFIG_SIE_DEVELOP_BUILD
int mt_afe_capture_read(struct device *dev, void *buf, unsigned int size, const int dai_id);
#endif

int mt_afe_transfer_start(struct device *dev, enum afe_input_port_type port_type, unsigned int sample_rate);
int mt_afe_transfer_stop(struct device *dev);

void *mt_afe_get_capture_dma_address(struct device *dev, const int dai_id);
unsigned int mt_afe_get_capture_buffer_size(struct device *dev, const int dai_id);
unsigned int mt_afe_capture_pointer(struct device *dev, const int dai_id);
int mt_afe_regist_usb_callback(struct device *dev, void (*callback)(void));
void mt_afe_enable_usb_callback(int dai_id);
void mt_afe_disable_usb_callback(void);

#ifdef CONFIG_SIE_DEVELOP_BUILD
int mt_afe_playback_start(struct device *dev);
int mt_afe_playback_stop(struct device *dev);
int mt_afe_playback_write(struct device *dev, void *buf, unsigned int size);
int mt_afe_start_tonegen(struct device *dev);
int mt_afe_stop_tonegen(struct device *dev);
#endif

#endif
