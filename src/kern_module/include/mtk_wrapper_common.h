/*
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

#ifndef _MTK_WRAPPER_DISPLAY_COMPONENT_DEF_H_
#define _MTK_WRAPPER_DISPLAY_COMPONENT_DEF_H_

typedef enum {
	DISPLAY_COMPONENT_INVALID     = 0x00000000,
	DISPLAY_COMPONENT_DP          = 0x00010000,
	DISPLAY_COMPONENT_SLICER_VID  = 0x00020000,
	DISPLAY_COMPONENT_SLICER_DSC  = 0x00020001,
	DISPLAY_COMPONENT_WDMA        = 0x00030000,
	DISPLAY_COMPONENT_MDP_RDMA    = 0x00040000,
	DISPLAY_COMPONENT_PVRIC_RDMA  = 0x00040001,
	DISPLAY_COMPONENT_DISP_RDMA   = 0x00040002,
	DISPLAY_COMPONENT_MUTEX       = 0x00050000,
	DISPLAY_COMPONENT_DSI         = 0x00060000,
	DISPLAY_COMPONENT_DSC         = 0x00070000,
	DISPLAY_COMPONENT_RESIZER     = 0x00080000,
	DISPLAY_COMPONENT_CROP        = 0x00090000,
	DISPLAY_COMPONENT_P2S         = 0x000a0000,
	DISPLAY_COMPONENT_LHC         = 0x000b0000,
} DISPLAY_COMPONENT;

typedef enum {
	MTK_WRAPPER_FRAME_FORMAT_Y_ONLY,
	MTK_WRAPPER_FRAME_FORMAT_YUV422,
	MTK_WRAPPER_FRAME_FORMAT_YUV444,
	MTK_WRAPPER_FRAME_FORMAT_RGB,
	MTK_WRAPPER_FRAME_FORMAT_UNKNOWN,
} MTK_WRAPPER_FRAME_FORMAT;

#endif /* _MTK_WRAPPER_DISPLAY_COMPONENT_DEF_H_*/
