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

#ifndef _MTK_DVFS_H_
#define _MTK_DVFS_H_

/* for DVFS used */
int mtk_cpufreq_set_target_index(unsigned int dvfs_index);
int mtk_devfreq_set_target_index(unsigned int dvfs_index);

#endif	/* _MTK_DVFS_H_ */
