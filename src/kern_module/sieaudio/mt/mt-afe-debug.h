/**
 * @file mt-afe-debug.h
 * Mediatek audio debug function
 *
 * Copyright (c) 2017 MediaTek Inc.
 *
 * Copyright (C) 2020 Sony Interactive Entertainment Inc.
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

#ifndef __MT_AFE_DEBUG_H__
#define __MT_AFE_DEBUG_H__

struct mtk_afe;

void mt_afe_init_debugfs(struct mtk_afe *afe);

void mt_afe_cleanup_debugfs(struct mtk_afe *afe);

#endif
