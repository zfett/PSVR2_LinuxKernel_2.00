/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: JB Tsai <jb.tsai@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __IVP_DBGFS_H__
#define __IVP_DBGFS_H__

int ivp_create_sysfs(struct device *dev);
void ivp_remove_sysfs(struct device *dev);

#endif /* __IVP_DBGFS_H__ */