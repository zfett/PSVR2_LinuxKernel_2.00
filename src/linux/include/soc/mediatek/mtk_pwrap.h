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
#ifndef MTK_PWRAP_H
#define MTK_PWRAP_H
int32_t pwrap_read_reg(uint32_t reg_addr, uint32_t *val,
			uint32_t mask, uint32_t shift);
int32_t pwrap_write_reg(uint32_t reg_addr, uint32_t val,
			uint32_t mask, uint32_t shift);
#endif
