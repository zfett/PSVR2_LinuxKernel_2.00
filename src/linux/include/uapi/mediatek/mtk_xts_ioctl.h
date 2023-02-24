/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _SPI_XTS_H
#define _SPI_XTS_H

struct xts_ioc_param {
	unsigned char *data_src;
	unsigned char *data_dst;
	unsigned int data_len;
	unsigned int addr;
};

#define XTS_IOCTYPE                 'X'
#define XTS_IOCTL_ENC               _IOW(XTS_IOCTYPE, 0, struct xts_ioc_param)
#define XTS_IOCTL_DEC               _IOW(XTS_IOCTYPE, 1, struct xts_ioc_param)
/***********************************************************************/

#endif
