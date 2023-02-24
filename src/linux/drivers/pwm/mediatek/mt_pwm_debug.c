/*
 * Copyright (C) 2018 MediaTek Inc.
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

/**
 * @file mt_pwm_debug.c
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <generated/autoconf.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <linux/atomic.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/clk.h>

#include "mt_pwm_hal.h"
#include "mt_pwm_reg.h"
#include <linux/err.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include "mt_pwm.h"
#include "mt_pwm_debug.h"

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm node  store API for self test.
 * @param[in]
 *     dev: The basic device structure.
 * @param[in]
 *     attr: Interface for exporting device attributes.
 * @param[in]
 *     buf: Buf point.
 * @param[in]
 *     count: Echo cmd of  test case.
 * @return
 *     If return value is count, no special.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static ssize_t pwm_debug_store(struct device *dev,
			       struct device_attribute *attr, const char *buf,
			       size_t count)
{
	int cmd, sub_cmd, pwm_no, n, i;
	dma_addr_t phys, phys1;
	unsigned int *membuff;
	unsigned int *membuff1;
	struct mtk_pwm *mediatek_pwm;

	PWMDBG("Enter into pwm_debug_store\n");

	mediatek_pwm = dev_get_drvdata(dev);

	n = sscanf(buf, "%d %d %d", &cmd, &sub_cmd, &pwm_no);
	if (!n)
		pr_err("pwm_debug_store nothing\n");

	if (cmd == 0) {
		PWMDBG("********** HELP **********\n");
		PWMDBG("1 -> clk source select: 26M or Others source clock\n");
		PWMDBG("\t1.1 -> sub cmd 1 : 26M\n");
		PWMDBG("\t1.1 -> sub cmd 2 : 66M or Others\n");
		PWMDBG("2 -> FIFO stop bit test: PWM0~PWM4 63, 62, 31\n");
		PWMDBG("\t2.1 -> sub cmd 1 : stop bit is 63\n");
		PWMDBG("\t2.2 -> sub cmd 2 : stop bit is 62\n");
		PWMDBG("\t2.3 -> sub cmd 3 : stop bit is 31\n");
		PWMDBG("3 -> FIFO wavenum test: PWM0~PWM31 num=1/num=0\n");
		PWMDBG("\t3.1 -> sub cmd 1 : PWM0~PWM31 num=0\n");
		PWMDBG("\t3.2 -> sub cmd 2 : PWM0~PWM31 num=1\n");
		PWMDBG("4 -> 32K select or use internal frequency individal\n");
		PWMDBG("\t4.1 -> sub cmd 1 : 32KHz selected\n");
		PWMDBG("\t4.2 -> sub cmd 2 : 26MHz 1625 setting selected\n");
		PWMDBG("\t4.3 -> sub cmd 3 : 26MHz selected\n");
		PWMDBG("5 -> 3D LCM(PWM8~PWM15) test\n");
		PWMDBG("6 -> Sequential mode(PWM0~PWM7) test\n");
		PWMDBG("7 -> MEMORY Mode test\n");
	} else if (cmd == 1) {
		if (sub_cmd == 1) {
			struct pwm_spec_config conf;

			PWMDBG("======clk source select test : 26M========\n");
			conf.pwm_no = pwm_no;
			conf.mode = PWM_MODE_FIFO;
			conf.clk_div = CLK_DIV1;
			conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
			conf.PWM_MODE_FIFO_REGS.idle_value = IDLE_FALSE;
			conf.PWM_MODE_FIFO_REGS.guard_value = GUARD_FALSE;
			conf.PWM_MODE_FIFO_REGS.stop_bitpos_value = 63;
			conf.PWM_MODE_FIFO_REGS.high_duration = 2;
			conf.PWM_MODE_FIFO_REGS.low_duration = 2;
			conf.PWM_MODE_FIFO_REGS.gduration = 0;
			conf.PWM_MODE_FIFO_REGS.send_data0 = 0xffffffff;
			conf.PWM_MODE_FIFO_REGS.send_data1 = 0x00000000;
			conf.PWM_MODE_FIFO_REGS.wave_num = 0;
			mt_pwm_26M_clk_enable_hal(1);
			pwm_set_spec_config(&conf);
		} else if (sub_cmd == 2) {
			struct pwm_spec_config conf;

			PWMDBG("======clk source select test: Others======\n");
			conf.pwm_no = pwm_no;
			conf.mode = PWM_MODE_FIFO;
			conf.clk_div = CLK_DIV1;
			conf.clk_src = PWM_CLK_NEW_MODE_BLOCK_DIV_BY_1625;
			conf.PWM_MODE_FIFO_REGS.idle_value = IDLE_FALSE;
			conf.PWM_MODE_FIFO_REGS.guard_value = GUARD_FALSE;
			conf.PWM_MODE_FIFO_REGS.stop_bitpos_value = 63;
			conf.PWM_MODE_FIFO_REGS.high_duration = 10;
			conf.PWM_MODE_FIFO_REGS.low_duration = 10;
			conf.PWM_MODE_FIFO_REGS.gduration = 0;
			conf.PWM_MODE_FIFO_REGS.send_data0 = 0xf0f0f0f0;
			conf.PWM_MODE_FIFO_REGS.send_data1 = 0xf0f0f0f0;
			conf.PWM_MODE_FIFO_REGS.wave_num = 0;
			mt_pwm_26M_clk_enable(0);
			pwm_set_spec_config(&conf);
		}		/* end sub cmd */
	} else if (cmd == 2) {
		if (sub_cmd == 1) {
			struct pwm_spec_config conf;

			PWMDBG("======Stop bit test : 63========\n");
			conf.pwm_no = pwm_no;
			conf.mode = PWM_MODE_FIFO;
			conf.clk_div = CLK_DIV1;
			conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
			conf.PWM_MODE_FIFO_REGS.idle_value = IDLE_FALSE;
			conf.PWM_MODE_FIFO_REGS.guard_value = GUARD_FALSE;
			conf.PWM_MODE_FIFO_REGS.stop_bitpos_value = 63;
			conf.PWM_MODE_FIFO_REGS.high_duration = 2;
			conf.PWM_MODE_FIFO_REGS.low_duration = 2;
			conf.PWM_MODE_FIFO_REGS.gduration = 0;
			conf.PWM_MODE_FIFO_REGS.send_data0 = 0x0000ff11;
			conf.PWM_MODE_FIFO_REGS.send_data1 = 0xffffffff;
			conf.PWM_MODE_FIFO_REGS.wave_num = 0;
			mt_pwm_26M_clk_enable_hal(1);
			pwm_set_spec_config(&conf);
		} else if (sub_cmd == 2) {
			struct pwm_spec_config conf;

			PWMDBG("======Stop bit test: 62========\n");
			conf.pwm_no = pwm_no;
			conf.mode = PWM_MODE_FIFO;
			conf.clk_div = CLK_DIV1;
			conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
			conf.PWM_MODE_FIFO_REGS.idle_value = IDLE_FALSE;
			conf.PWM_MODE_FIFO_REGS.guard_value = GUARD_FALSE;
			conf.PWM_MODE_FIFO_REGS.stop_bitpos_value = 62;
			conf.PWM_MODE_FIFO_REGS.high_duration = 2;
			conf.PWM_MODE_FIFO_REGS.low_duration = 2;
			conf.PWM_MODE_FIFO_REGS.gduration = 0;
			conf.PWM_MODE_FIFO_REGS.send_data0 = 0x0000ff11;
			conf.PWM_MODE_FIFO_REGS.send_data1 = 0xffffffff;
			conf.PWM_MODE_FIFO_REGS.wave_num = 0;
			mt_pwm_26M_clk_enable_hal(1);
			pwm_set_spec_config(&conf);
		} else if (sub_cmd == 3) {
			struct pwm_spec_config conf;

			PWMDBG("======Stop bit test: 31========\n");
			conf.pwm_no = pwm_no;
			conf.mode = PWM_MODE_FIFO;
			conf.clk_div = CLK_DIV1;
			conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
			conf.PWM_MODE_FIFO_REGS.idle_value = IDLE_FALSE;
			conf.PWM_MODE_FIFO_REGS.guard_value = GUARD_FALSE;
			conf.PWM_MODE_FIFO_REGS.stop_bitpos_value = 31;
			conf.PWM_MODE_FIFO_REGS.high_duration = 2;
			conf.PWM_MODE_FIFO_REGS.low_duration = 2;
			conf.PWM_MODE_FIFO_REGS.gduration = 0;
			conf.PWM_MODE_FIFO_REGS.send_data0 = 0x0000ff11;
			conf.PWM_MODE_FIFO_REGS.send_data1 = 0xffffffff;
			conf.PWM_MODE_FIFO_REGS.wave_num = 0;
			mt_pwm_26M_clk_enable_hal(1);
			pwm_set_spec_config(&conf);
		}		/* end sub cmd */
	} else if (cmd == 3) {
		if (sub_cmd == 1) {
			struct pwm_spec_config conf;

			PWMDBG("======Wave number test : 0========\n");
			conf.pwm_no = pwm_no;
			conf.mode = PWM_MODE_FIFO;
			conf.clk_div = CLK_DIV1;
			conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
			conf.PWM_MODE_FIFO_REGS.idle_value = IDLE_FALSE;
			conf.PWM_MODE_FIFO_REGS.guard_value = GUARD_FALSE;
			conf.PWM_MODE_FIFO_REGS.stop_bitpos_value = 63;
			conf.PWM_MODE_FIFO_REGS.high_duration = 2;
			conf.PWM_MODE_FIFO_REGS.low_duration = 2;
			conf.PWM_MODE_FIFO_REGS.gduration = 0;
			conf.PWM_MODE_FIFO_REGS.send_data0 = 0xf0f0f0f0;
			conf.PWM_MODE_FIFO_REGS.send_data1 = 0xf0f0f0f0;
			conf.PWM_MODE_FIFO_REGS.wave_num = 0;
			mt_pwm_26M_clk_enable_hal(1);
			pwm_set_spec_config(&conf);
		} else if (sub_cmd == 2) {
			struct pwm_spec_config conf;

			PWMDBG("======Wave Number test: 1========\n");
			conf.pwm_no = pwm_no;
			conf.mode = PWM_MODE_FIFO;
			conf.clk_div = CLK_DIV1;
			conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
			conf.PWM_MODE_FIFO_REGS.idle_value = IDLE_FALSE;
			conf.PWM_MODE_FIFO_REGS.guard_value = GUARD_FALSE;
			conf.PWM_MODE_FIFO_REGS.stop_bitpos_value = 63;
			conf.PWM_MODE_FIFO_REGS.high_duration = 119;
			conf.PWM_MODE_FIFO_REGS.low_duration = 119;
			conf.PWM_MODE_FIFO_REGS.gduration = 0;
			conf.PWM_MODE_FIFO_REGS.send_data0 = 0x0000ff11;
			conf.PWM_MODE_FIFO_REGS.send_data1 = 0xffffffff;
			conf.PWM_MODE_FIFO_REGS.wave_num = 2;
			mt_set_intr_enable_hal(pwm_no, PWM_INT_FINISH_EN);
			mt_pwm_26M_clk_enable_hal(1);
			pwm_set_spec_config(&conf);

			mt_set_intr_ack_hal(pwm_no, PWM_INT_FINISH_ACK);

		}		/* end sub cmd */
	} else if (cmd == 4) {
		if (sub_cmd == 1) {
			struct pwm_spec_config conf;

			PWMDBG("======Clk select test : 32KHz========\n");
			conf.pwm_no = pwm_no;
			conf.mode = PWM_MODE_OLD;
			conf.clk_div = CLK_DIV1;
			conf.clk_src = PWM_CLK_OLD_MODE_32K;	/* 16KHz */
			conf.PWM_MODE_OLD_REGS.idle_value = IDLE_FALSE;
			conf.PWM_MODE_OLD_REGS.guard_value = GUARD_FALSE;
			conf.PWM_MODE_OLD_REGS.gduration = 0;
			conf.PWM_MODE_OLD_REGS.wave_num = 0;
			conf.PWM_MODE_OLD_REGS.data_width = 10;
			conf.PWM_MODE_OLD_REGS.thresh = 5;
			pwm_set_spec_config(&conf);
		} else if (sub_cmd == 2) {
			struct pwm_spec_config conf;

			PWMDBG("======Clk select test : 26MHz/1625========\n");
			conf.pwm_no = pwm_no;
			conf.mode = PWM_MODE_OLD;
			conf.clk_div = CLK_DIV1;
			conf.clk_src = PWM_CLK_OLD_MODE_32K;	/* 16KHz */
			conf.PWM_MODE_OLD_REGS.idle_value = IDLE_FALSE;
			conf.PWM_MODE_OLD_REGS.guard_value = GUARD_FALSE;
			conf.PWM_MODE_OLD_REGS.gduration = 0;
			conf.PWM_MODE_OLD_REGS.wave_num = 0;
			conf.PWM_MODE_OLD_REGS.data_width = 10;
			conf.PWM_MODE_OLD_REGS.thresh = 5;
			pwm_set_spec_config(&conf);
		} else if (sub_cmd == 3) {
			struct pwm_spec_config conf;

			PWMDBG("======Clk select test : 26MHz========\n");
			conf.pwm_no = pwm_no;
			conf.mode = PWM_MODE_OLD;
			conf.clk_div = CLK_DIV1;
			conf.clk_src = PWM_CLK_OLD_MODE_BLOCK;	/* 26MHz */
			conf.PWM_MODE_OLD_REGS.idle_value = IDLE_FALSE;
			conf.PWM_MODE_OLD_REGS.guard_value = GUARD_FALSE;
			conf.PWM_MODE_OLD_REGS.gduration = 0;
			conf.PWM_MODE_OLD_REGS.wave_num = 0;
			conf.PWM_MODE_OLD_REGS.data_width = 10;
			conf.PWM_MODE_OLD_REGS.thresh = 5;
			pwm_set_spec_config(&conf);
		}		/* end sub cmd */
	} else if (cmd == 5) {
		struct pwm_spec_config conf;

		PWMDBG("======3DLCM(PWM8~PWM15) test========\n");
		/* Set PWM8 fifo mode parameter */
		PWMDBG("Set PWM8 Base channel PWM_MODE_FIFO First\n");
		conf.mode = PWM_MODE_FIFO;
		conf.pwm_no = PWM8;
		conf.clk_div = CLK_DIV1;
		conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
		conf.PWM_MODE_FIFO_REGS.idle_value = IDLE_FALSE;
		conf.PWM_MODE_FIFO_REGS.guard_value = GUARD_FALSE;
		conf.PWM_MODE_FIFO_REGS.stop_bitpos_value = 63;
		conf.PWM_MODE_FIFO_REGS.high_duration = 2;
		conf.PWM_MODE_FIFO_REGS.low_duration = 2;
		conf.PWM_MODE_FIFO_REGS.gduration = 0;
		conf.PWM_MODE_FIFO_REGS.send_data0 = 0xf0f0f0f0;
		conf.PWM_MODE_FIFO_REGS.send_data1 = 0xf0f0f0f0;
		conf.PWM_MODE_FIFO_REGS.wave_num = 0;
		mt_pwm_26M_clk_enable_hal(1);
		pwm_set_spec_config(&conf);
		/* Set PWM8 as base channel, others as belows */
		conf.mode = PWM_MODE_3DLCM;
		conf.pwm_no = PWM8;
		conf.PWM_MODE_3DLCM_REGS.pwm8_as_base_channel = 1;
		conf.PWM_MODE_3DLCM_REGS.pwm9_as_aux_channel = 1;
		conf.PWM_MODE_3DLCM_REGS.pwm10_as_aux_channel = 1;
		conf.PWM_MODE_3DLCM_REGS.pwm11_as_aux_channel = 1;
		conf.PWM_MODE_3DLCM_REGS.pwm12_as_aux_channel = 1;
		conf.PWM_MODE_3DLCM_REGS.pwm13_as_aux_channel = 1;
		conf.PWM_MODE_3DLCM_REGS.pwm14_as_aux_channel = 1;
		conf.PWM_MODE_3DLCM_REGS.pwm15_as_aux_channel = 1;
		conf.PWM_MODE_3DLCM_REGS.pwm9_inverse_base_channel = 1;
		conf.PWM_MODE_3DLCM_REGS.pwm10_inverse_base_channel = 1;
		conf.PWM_MODE_3DLCM_REGS.pwm11_inverse_base_channel = 0;
		conf.PWM_MODE_3DLCM_REGS.pwm12_inverse_base_channel = 1;
		conf.PWM_MODE_3DLCM_REGS.pwm13_inverse_base_channel = 0;
		conf.PWM_MODE_3DLCM_REGS.pwm14_inverse_base_channel = 1;
		conf.PWM_MODE_3DLCM_REGS.pwm15_inverse_base_channel = 1;
		pwm_set_spec_config(&conf);

	} else if (cmd == 6) {
		struct pwm_spec_config conf;

		PWMDBG("======Sequential mode(PWM0~PWM7) test======\n");
		for (i = PWM0; i <= PWM7; i++) {
			conf.mode = PWM_MODE_FIFO;
			conf.pwm_no = i;
			conf.clk_div = CLK_DIV1;
			conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
			conf.PWM_MODE_FIFO_REGS.idle_value = IDLE_FALSE;
			conf.PWM_MODE_FIFO_REGS.guard_value = GUARD_FALSE;
			conf.PWM_MODE_FIFO_REGS.stop_bitpos_value = 63;
			conf.PWM_MODE_FIFO_REGS.high_duration = 2;
			conf.PWM_MODE_FIFO_REGS.low_duration = 2;
			conf.PWM_MODE_FIFO_REGS.gduration = 0;
			conf.PWM_MODE_FIFO_REGS.send_data0 = 0xf0f0f0f0;
			conf.PWM_MODE_FIFO_REGS.send_data1 = 0xf0f0f0f0;
			conf.PWM_MODE_FIFO_REGS.wave_num = 0;
			mt_pwm_26M_clk_enable_hal(1);
			pwm_set_spec_config(&conf);
		}
		conf.mode = PWM_MODE_DELAY;
		conf.pwm_no = pwm_no;
		conf.PWM_MODE_DELAY_REGS.pwm1_delay_dur = 10;
		conf.PWM_MODE_DELAY_REGS.pwm1_delay_clk = 0;
		conf.PWM_MODE_DELAY_REGS.pwm2_delay_dur = 10;
		conf.PWM_MODE_DELAY_REGS.pwm2_delay_clk = 0;
		conf.PWM_MODE_DELAY_REGS.pwm3_delay_dur = 10;
		conf.PWM_MODE_DELAY_REGS.pwm3_delay_clk = 0;
		conf.PWM_MODE_DELAY_REGS.pwm4_delay_dur = 10;
		conf.PWM_MODE_DELAY_REGS.pwm4_delay_clk = 0;
		conf.PWM_MODE_DELAY_REGS.pwm5_delay_dur = 10;
		conf.PWM_MODE_DELAY_REGS.pwm5_delay_clk = 0;
		conf.PWM_MODE_DELAY_REGS.pwm6_delay_dur = 10;
		conf.PWM_MODE_DELAY_REGS.pwm6_delay_clk = 0;
		conf.PWM_MODE_DELAY_REGS.pwm7_delay_dur = 10;
		conf.PWM_MODE_DELAY_REGS.pwm7_delay_clk = 0;
		pwm_set_spec_config(&conf);

	} else if (cmd == 7) {
		struct pwm_spec_config conf;

		PWMDBG("======MEMORY test========\n");
		conf.mode = PWM_MODE_MEMORY;
		conf.pwm_no = pwm_no;
		conf.clk_div = CLK_DIV1;
		conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
		conf.PWM_MODE_MEMORY_REGS.idle_value = IDLE_FALSE;
		conf.PWM_MODE_MEMORY_REGS.guard_value = GUARD_FALSE;
		conf.PWM_MODE_MEMORY_REGS.high_duration = 119;
		conf.PWM_MODE_MEMORY_REGS.low_duration = 119;
		conf.PWM_MODE_MEMORY_REGS.gduration = 0;
		conf.PWM_MODE_MEMORY_REGS.wave_num = 0;
		conf.PWM_MODE_MEMORY_REGS.stop_bitpos_value = 32;

		mt_pwm_26M_clk_enable_hal(1);

		membuff = dma_alloc_coherent(mediatek_pwm->chip.dev,
			8, &phys, GFP_KERNEL);

		if (unlikely(!membuff)) {
			PWMDBG("func(%s).line(%d), alloct mem fail\n",
				__func__, __LINE__);
			return -ENOMEM;
		}
		membuff[0] = 0xaaaaaaaa;
		membuff[1] = 0xffff0000;
		conf.PWM_MODE_MEMORY_REGS.buf0_size = 8;
		conf.PWM_MODE_MEMORY_REGS.buf0_base_addr = phys;
		pwm_set_spec_config(&conf);
		dma_free_coherent(mediatek_pwm->chip.dev,
			8, (void *)membuff, phys);
	} else if (cmd == 8) {
		struct pwm_spec_config conf;

		PWMDBG("======RANDOM test========\n");
		conf.mode = PWM_MODE_RANDOM;
		conf.pwm_no = pwm_no;
		conf.clk_div = CLK_DIV1;
		conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
		conf.PWM_MODE_RANDOM_REGS.idle_value = IDLE_FALSE;
		conf.PWM_MODE_RANDOM_REGS.guard_value = GUARD_FALSE;
		conf.PWM_MODE_RANDOM_REGS.high_duration = 119;
		conf.PWM_MODE_RANDOM_REGS.low_duration = 119;
		conf.PWM_MODE_RANDOM_REGS.gduration = 0;
		conf.PWM_MODE_RANDOM_REGS.wave_num = 0;
		conf.PWM_MODE_RANDOM_REGS.stop_bitpos_value = 32;

		mt_pwm_26M_clk_enable_hal(1);

		membuff = dma_alloc_coherent(mediatek_pwm->chip.dev,
			8, &phys, GFP_KERNEL);
		if (unlikely(!membuff)) {
			PWMDBG("func(%s).line(%d), alloct membuff fail\n",
				__func__, __LINE__);
			return -ENOMEM;
		}
		membuff[0] = 0xaaaaaaaa;
		membuff[1] = 0xffff0000;

		membuff1 = dma_alloc_coherent(mediatek_pwm->chip.dev,
			8, &phys1, GFP_KERNEL);
		if (unlikely(!membuff1)) {
			PWMDBG("func(%s).line(%d), alloct membuff1 fail\n",
				__func__, __LINE__);
			return -ENOMEM;
		}
		membuff1[0] = 0xaaaaaaaa;
		membuff1[1] = 0xffff0000;

		conf.PWM_MODE_RANDOM_REGS.buf0_size = 8;
		conf.PWM_MODE_RANDOM_REGS.buf0_base_addr = phys;
		conf.PWM_MODE_RANDOM_REGS.buf1_size = 8;
		conf.PWM_MODE_RANDOM_REGS.buf1_base_addr = phys;
		pwm_set_spec_config(&conf);

		dma_free_coherent(mediatek_pwm->chip.dev,
			8, (void *)membuff, phys);
		dma_free_coherent(mediatek_pwm->chip.dev,
			8, (void *)membuff1, phys1);
	} else {
		PWMDBG(" \tInvalid Command!\n");
	}
	return count;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm node  show API for dump regs.
 * @param[in]
 *     dev: The basic device structure.
 * @param[in]
 *     attr: Interface for exporting device attributes.
 * @param[out]
 *     buf: Buf point.
 * @return
 *     If return value is buf, no special.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static ssize_t pwm_debug_show(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	pwm_debug_show_hal();
	PWMDBG("Func:%s,line%d.\n", __func__, __LINE__);
	return 0;
}

static DEVICE_ATTR(pwm_debug, 0644, pwm_debug_show, pwm_debug_store);

void mtk_pwm_debug_init(struct mtk_pwm *mediatek_pwm)
{
	int ret;

	ret = device_create_file(mediatek_pwm->chip.dev, &dev_attr_pwm_debug);
	if (ret)
		PWMDBG("error creating sysfs files: pwm_debug\n");
}

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("MTK");
MODULE_DESCRIPTION(" This module is for mtk chip of mediatek");
