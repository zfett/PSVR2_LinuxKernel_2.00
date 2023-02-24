/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: Andrew-sh.Cheng, Mediatek
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

#ifndef __LINUX_REGULATOR_MT3615_H
#define __LINUX_REGULATOR_MT3615_H

enum {
	MT3615_ID_VPROC = 0,
	MT3615_ID_VGPU11,
	MT3615_ID_VGPU12,
	MT3615_ID_VPU,
	MT3615_ID_VCORE1,
	MT3615_ID_VCORE2,
	MT3615_ID_VCORE4,
	MT3615_ID_VS1,
	MT3615_ID_VS2,
	MT3615_ID_VS3,
	MT3615_ID_VDRAM1,
	MT3615_ID_VDRAM2,
	MT3615_ID_VIO18,
	MT3615_ID_VIO31,
	MT3615_ID_VAUX18,
	MT3615_ID_VXO,
	MT3615_ID_VEFUSE,
	MT3615_ID_VM18,
	MT3615_ID_VUSB,
	MT3615_ID_VA18,
	MT3615_ID_VA12,
	MT3615_ID_VA09,
	MT3615_ID_VSRAM_PROC,
	MT3615_ID_VSRAM_COREX,
	MT3615_ID_VSRAM_GPU,
	MT3615_ID_VSRAM_VPU,
	MT3615_ID_VPIO31,
	MT3615_ID_VBBCK,
	MT3615_ID_VPIO18,
	/* RO LDO
	 * MT3615_ID_VDIG18,
	 */
	MT3615_ID_RG_MAX,
};

#define MT3615_MAX_REGULATOR	MT3615_ID_RG_MAX
#ifdef ANDREW
#define MT6397_REGULATOR_ID97	0x97
#define MT6397_REGULATOR_ID91	0x91

#endif
#endif /* __LINUX_REGULATOR_MT3615_H */
