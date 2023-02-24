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

int icm426debug_setup(struct sieimu_spi *spi_handle);
void icm426debug_isr_trace(u32 vts, u32 sample_cnt, u32 imu_vts, u16 imu_cnt);
void icm426debug_get_write_pos_trace(u32 wrap_cnt, u32 write_pos);
void icm426debug_req_callback_trace(u32 vts);
void icm426debug_req_callback_done_trace(u32 vts, s32 diff_vts,
					 u32 adjust_isr_phase, u32 wc, u32 wp);
void icm426debug_data_trace(u32 vts, u32 imu_vts, u16 imu_ts,
			    u16 imu_cnt, u32 wc, u32 wp, u16 eod, u16 invalid);
