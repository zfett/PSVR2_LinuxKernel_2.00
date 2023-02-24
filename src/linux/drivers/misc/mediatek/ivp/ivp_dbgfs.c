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

#include <linux/platform_device.h>

#include <linux/ctype.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/syscalls.h>

#include "include/fw/ivp_dbg_ctrl.h"

#include "ivp_cmd_hnd.h"
#include "ivp_dbgfs.h"
#include "ivp_driver.h"
#include "ivp_dvfs.h"
#include "ivp_met_event.h"
#include "ivp_opp_ctl.h"
#include "ivp_profile.h"
#include "ivp_queue.h"
#include "ivp_utilization.h"
#include "ivpbuf-dma-memcpy.h"

#define REG_SIZE 0x1000

struct ivp_state_string {
	int state_no;
	char *string;
};

static ssize_t show_internal_register_para(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct ivp_device *ivp_device = dev_get_drvdata(dev);
	struct ivp_register_dump ivp_reg_dump = ivp_device->register_dump;
	int i, count = 0;

	if (!ivp_device->ivp_init_done) {
		dev_err(dev, "ivp not initialize!\n");
		return 0;
	}

	if (ivp_reg_dump.ivp_base != NULL) {

		count += scnprintf(buf + count, PAGE_SIZE,
			 "reg%d:\n", ivp_reg_dump.sel_base);

		for (i = 0; i < (ivp_reg_dump.dumpsize / 4); i++) {
			count += scnprintf(buf + count, PAGE_SIZE,
				 "0x%03x: 0x%08x\n",
				 ivp_reg_dump.regoff + (i * 4),
				 ioread32(ivp_reg_dump.ivp_base +
				 ivp_reg_dump.regoff + (i * 4)));
		}
		return count;
	}

	return scnprintf(buf, PAGE_SIZE,
	       "echo coreid:0, selreg:0, regoff:0x0, dumpsize:0x200 > %s\n",
	       attr->attr.name);

}

static ssize_t ivp_internal_register_dump(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf,
					  size_t count)
{
	struct ivp_device *ivp_device = dev_get_drvdata(dev);
	struct ivp_core *ivp_core;
	struct ivp_register_dump ivp_reg_dump;
	int coreid;

	if (!ivp_device->ivp_init_done) {
		dev_err(dev, "ivp not initialize!\n");
		return count;
	}

	if (sscanf(buf, "coreid:%d, selreg:%d, regoff:0x%x, dumpsize:0x%x",
		   &coreid, &ivp_reg_dump.sel_base,
		   &ivp_reg_dump.regoff, &ivp_reg_dump.dumpsize) != 4) {
		dev_err(dev, "input parameter is worng\n");
		return count;
	}

	if (coreid >= ivp_device->ivp_core_num || coreid < 0 ||
	    ivp_reg_dump.regoff >= REG_SIZE ||
	    (ivp_reg_dump.regoff + ivp_reg_dump.dumpsize > REG_SIZE)) {
		dev_err(dev, "input parameter is worng (%d, %d, %d)\n",
			coreid, ivp_reg_dump.regoff,
			ivp_reg_dump.dumpsize);
		return count;
	}

	ivp_core = ivp_device->ivp_core[coreid];
	if (!ivp_core) {
		dev_err(dev, "ivpcore%d is not valid\n",
			coreid);
		return count;
	}

	ivp_reg_dump.ivp_base = ivp_core->ivpctrl_base;

	ivp_device->register_dump = ivp_reg_dump;

	return count;
}

static int ischr(int c)
{
	return ((c >= ' ' && c <= '~') ? 1 : 0);
}

static void print_buffer(char *log_buf, u32 start, u32 end)
{
	int i, line_pos = 0;
	char line_buffer[512 + 1] = {0};

	for (i = start; i < end; i++) {
		char in = log_buf[i];

		if (line_pos == 512) {
			pr_err("a %s\n", line_buffer);
			line_pos = 0;
		}
		if (isascii(in) && ischr(in)) {
			line_buffer[line_pos] = in;
			line_pos++;
		} else {
			if (line_pos > 0) {
				line_buffer[line_pos] = '\0';
				pr_err("=> %s\n", line_buffer);
			}
			line_pos = 0;
		}
	}
}

static ssize_t show_ivp_debug_info(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int i, count = 0;
	struct ivp_device *ivp_device = dev_get_drvdata(dev);
	struct ivp_get_dbg_info dbg_info;
	char *log_buf;
	u32 log_buf_base;
	u32 end;

	if (!ivp_device->ivp_init_done) {
		dev_err(dev, "ivp not initialize!\n");
		return count;
	}

	for (i = 0; i < ivp_device->ivp_core_num; i++) {
		if (!ivp_device->ivp_core[i])
			continue;

		log_buf = ivp_device->ivp_core[i]->working_buffer->dbg_log;
		log_buf_base = ivp_device->ivp_core[i]->working_buffer_iova +
				offsetof(struct ivp_working_buf, dbg_log);

		if (ivp_device->ivp_core[i]->state != IVP_UNPREPARE) {
			ivp_get_dbg_msg(&dbg_info, ivp_device->ivp_core[i]);
			count += scnprintf(buf + count, PAGE_SIZE,
					   "core %d mbuf=%x, lbuf=%x\n", i,
					   dbg_info.msg_buf_ptr,
					   dbg_info.log_buffer_wptr);

			if (dbg_info.log_buffer_wptr) {
				end = dbg_info.log_buffer_wptr - log_buf_base;
				pr_err("core%d dbg_log:\n", i);
				print_buffer(log_buf, 0, end);
			}
		} else {
			pr_err("core%d dbg_log:\n", i);
			print_buffer(log_buf, 0,
				     IVP_FW_DBG_MSG_BUFFER_SIZE - 1);
		}
	}
	return count;
}

static ssize_t set_ivp_debug_info(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct ivp_device *ivp_device = dev_get_drvdata(dev);
	struct ivp_set_dbg_info dbg_info;
	int i;

	if (!ivp_device->ivp_init_done) {
		dev_err(dev, "ivp not initialize!\n");
		return count;
	}

	if (sscanf(buf, "%d %d", &dbg_info.control, &dbg_info.level) == 2) {
		if (dbg_info.level & ~DBG_LEVEL_MAX) {
			pr_err("%s %d: invalid level %d\n", __func__, __LINE__,
			       dbg_info.level);
			return count;
		}
		if (dbg_info.control & ~DBG_CTRL_MAX) {
			pr_err("%s %d: invalid control %d\n", __func__,
			       __LINE__, dbg_info.control);
			return count;
		}
	} else {
		pr_err("%s %d: bad parameter\n", __func__, __LINE__);
		return count;
	}

	for (i = 0; i < ivp_device->ivp_core_num; i++) {
		if (!ivp_device->ivp_core[i])
			continue;
		ivp_set_dbg_ctl(&dbg_info, ivp_device->ivp_core[i]);
	}

	return count;
}

static ssize_t show_ivp_ptrace_log(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int i, count = 0;
	struct ivp_device *ivp_device = dev_get_drvdata(dev);
	char *log_buf;
	u32 log_buf_base, log_buffer_wptr;
	u32 start, end;

	if (!ivp_device->ivp_init_done) {
		dev_err(dev, "ivp not initialize!\n");
		return count;
	}

	for (i = 0; i < ivp_device->ivp_core_num; i++) {
		if (!ivp_device->ivp_core[i])
			continue;

		log_buf = ivp_device->ivp_core[i]->working_buffer->ptrace_log;
		log_buf_base = ivp_device->ivp_core[i]->working_buffer_iova +
			   offsetof(struct ivp_working_buf, ptrace_log);
		start = ivp_device->ivp_core[i]->ptrace_log_start;

		if (ivp_device->ivp_core[i]->state != IVP_UNPREPARE) {
			ivp_get_ptrace_msg(&log_buffer_wptr,
					   ivp_device->ivp_core[i]);
			if (log_buffer_wptr) {
				end = log_buffer_wptr - log_buf_base;
				pr_err("core%d ptrace_log:\n", i);
				if (start > end) {
					print_buffer(log_buf, start,
						IVP_FW_PTRACE_MSG_BUFFER_SIZE -
						1);
					start = 0;
				}
				print_buffer(log_buf, start, end);
				ivp_device->ivp_core[i]->ptrace_log_start = end;
			}
		} else {
			pr_err("core%d ptrace_log:\n", i);
			print_buffer(log_buf, start,
				IVP_FW_PTRACE_MSG_BUFFER_SIZE - 1);
			ivp_device->ivp_core[i]->ptrace_log_start = 0;
		}
	}
	return count;
}

static ssize_t set_ivp_ptrace_level(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct ivp_device *ivp_device = dev_get_drvdata(dev);
	u32 level;
	int i;

	if (!ivp_device->ivp_init_done) {
		dev_err(dev, "ivp not initialize!\n");
		return count;
	}

	if (!kstrtou32(buf, 0, &level)) {
		if (level & ~PTRACE_LEVEL_MAX) {
			pr_err("%s %d: invalid level %d\n", __func__, __LINE__,
			       level);
			return count;
		}
	} else {
		pr_err("%s %d: bad parameter\n", __func__, __LINE__);
		return count;
	}

	for (i = 0; i < ivp_device->ivp_core_num; i++) {
		if (!ivp_device->ivp_core[i])
			continue;
		ivp_set_ptrace_level(level, ivp_device->ivp_core[i]);
	}

	return count;
}


static ssize_t show_ivp_fw_ver(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct ivp_device *ivp_device = dev_get_drvdata(dev);
	int i, count = 0;

	if (!ivp_device->ivp_init_done) {
		dev_err(dev, "ivp not initialize!\n");
		return count;
	}

	for (i = 0; i < ivp_device->ivp_core_num; i++) {
		struct ivp_core *ivp_core = ivp_device->ivp_core[i];

		if (!ivp_core)
			continue;
		count += scnprintf(buf + count, PAGE_SIZE,
				   "core %d fw version=%x\n", i,
				   (ivp_core->ver_major << 16) +
				   (ivp_core->ver_minor));
	}
	return count;
}

static ssize_t ivp_set_cmd_timeout(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct ivp_device *ivp_device = dev_get_drvdata(dev);
	u32 timeout2reset, timeout;

	if (sscanf(buf, "%d %d", &timeout2reset, &timeout) == 2) {
		if ((timeout2reset != 0) && (timeout2reset != 1)) {
			pr_err("%s %d: timeout2reset should be 0 or 1\n",
			       __func__, __LINE__);
			return count;
		}
		if (timeout > 1000000) {
			pr_err("%s %d: timeout should be < 1000000 ms\n",
			       __func__, __LINE__);
			return count;
		}
	} else {
		pr_err("%s %d: bad parameter, need 2 parameter\n",
		       __func__, __LINE__);
		return count;
	}
	ivp_device->timeout2reset = timeout2reset;
	ivp_device->timeout = timeout;

	pr_err("%s timeout2reset=%d, timeout=%d ms\n",
	       __func__, timeout2reset, timeout);

	return count;
}

static ssize_t ivp_get_cmd_timeout(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct ivp_device *ivp_device = dev_get_drvdata(dev);
	int count = 0;

	count += scnprintf(buf + count, PAGE_SIZE,
			   "timeout2reset=%d timeout=%dms\n",
			   ivp_device->timeout2reset,
			   ivp_device->timeout);
	return count;
}

static ssize_t set_ivp_autosuspend(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct ivp_device *ivp_device = dev_get_drvdata(dev);
	u32 set_autosuspend;

	if (!kstrtou32(buf, 0, &set_autosuspend)) {
		if ((set_autosuspend != 0) && (set_autosuspend != 1)) {
			pr_err("%s %d: ivp_autosuspend should be 0 or 1\n",
			       __func__, __LINE__);
			return count;
		}
	} else {
		pr_err("%s %d: bad parameter, need 1 parameter!\n",
		       __func__, __LINE__);
		return count;
	}

	if (set_autosuspend && !ivp_device->set_autosuspend) {
		int core;

		for (core = 0; core < ivp_device->ivp_core_num; core++) {
			struct ivp_core *ivp_core = ivp_device->ivp_core[core];

			ivp_core_set_auto_suspend(ivp_core, 0);
		}
	} else if (!set_autosuspend && ivp_device->set_autosuspend) {
		int core;

		for (core = 0; core < ivp_device->ivp_core_num; core++) {
			struct ivp_core *ivp_core = ivp_device->ivp_core[core];

			if (pm_runtime_get_sync(ivp_core->dev) < 0) {
				dev_err(ivp_core->dev,
					"%s: ivp_core_set_resume fail.\n",
					__func__);
				return count;
			}
		}
	}

	ivp_device->set_autosuspend = set_autosuspend;

	return count;
}

static ssize_t show_ivp_autosuspend(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct ivp_device *ivp_device = dev_get_drvdata(dev);
	int count = 0;

	if (ivp_device->set_autosuspend) {
		count += scnprintf(buf + count, PAGE_SIZE,
				   "ivp_autosuspend is on\n");
	} else {
		count += scnprintf(buf + count, PAGE_SIZE,
				   "ivp_autosuspend is off\n");
	}

	return count;
}

static ssize_t set_ivp_initialize(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct ivp_device *ivp_device = dev_get_drvdata(dev);
	u32 set_autosuspend;
	int ret;

	mutex_lock(&ivp_device->init_mutex);
	if (ivp_device->ivp_init_done) {
		dev_err(dev, "ivp initialize done!\n");
		mutex_unlock(&ivp_device->init_mutex);
		return count;
	}
	mutex_unlock(&ivp_device->init_mutex);

	if (!kstrtou32(buf, 0, &set_autosuspend)) {
		if ((set_autosuspend != 0) && (set_autosuspend != 1)) {
			pr_err("%s %d: ivp_autosuspend should be 0 or 1\n",
			       __func__, __LINE__);
			return count;
		}
	} else {
		pr_err("%s %d: bad parameter, need 1 parameter!\n",
		       __func__, __LINE__);
		return count;
	}

	ivp_device->set_autosuspend = set_autosuspend;


	mutex_lock(&ivp_device->init_mutex);
	ret = ivp_initialize(ivp_device);
	if (ret)
		pr_err("fail to initialize ivp");
	mutex_unlock(&ivp_device->init_mutex);

	return count;
}

static ssize_t show_ivp_initialize(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct ivp_device *ivp_device = dev_get_drvdata(dev);
	int count = 0;

	if (!ivp_device->ivp_init_done) {
		dev_err(dev, "ivp not initialize!\n");
		return count;
	}

	if (ivp_device->set_autosuspend) {
		count += scnprintf(buf + count, PAGE_SIZE,
			 "ivp inialize done and ivp_autosuspend is on\n");
	} else {
		count += scnprintf(buf + count, PAGE_SIZE,
			 "ivp inialize done and ivp_autosuspend is off\n");
	}

	return count;
}

static ssize_t set_ivp_opp(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct ivp_device *ivp_device = dev_get_drvdata(dev);
	struct ivp_core *ivp_core;
#ifdef CONFIG_MTK_IVP_DVFS
	struct ivp_devfreq *ivp_devfreq;
#endif
	long freq[IVP_CLK_NUM];
	long voltage = 0;
	int i, ret, coreid;
	int index = 0, cur_idx = 0, max_idx = 0;
	int *clk_ref;

	if (!ivp_device->ivp_init_done) {
		dev_err(dev, "ivp not initialize!\n");
		return count;
	}

	if (sscanf(buf, "%d %ld %ld", &coreid, &freq[0], &freq[1]) == 3) {
		if (coreid >= ivp_device->ivp_core_num || coreid < 0) {
			pr_err("%s %d: core should be in 0-%d\n",
			       __func__, __LINE__, ivp_device->ivp_core_num);
			return count;
		}
	} else {
		pr_err("%s %d: bad parameter, need %d parameter!\n",
		       __func__, __LINE__, IVP_CLK_NUM + 1);
		return count;
	}

	for (i = 0; i < IVP_CLK_NUM; i++) {
		if (freq[i] < 0) {
			pr_warn("%s freq < 0, no changed.\n",
				ivp_device->clk_name[i]);
			freq[i] = 0;
		}
	}

	ivp_core = ivp_device->ivp_core[coreid];
	if (!ivp_core) {
		pr_err("%s: ivp_core%d is not ready.\n", __func__, coreid);
		return count;
	}

#ifdef CONFIG_MTK_IVP_DVFS
	ivp_devfreq = ivp_core->ivp_devfreq;
	if (strcmp(ivp_devfreq->devfreq->governor_name, "userspace")) {
		pr_warn("please change governor to userspace first!\n");
		return count;
	}
#endif

	/* find max index between cores */
	if (freq[0] != 0)
		index = ivp_find_clk_index(ivp_device, 0, &freq[0]);
	if (index < 0) {
		dev_err(ivp_device->dev,
			"failed to find index of clk0: %d\n", index);
		return count;
	}
	cur_idx = ivp_find_clk_index(ivp_device, 0, &ivp_core->current_freq);
	mutex_lock(&ivp_device->clk_ref_mutex);
	clk_ref = ivp_device->clk_ref;
	mutex_unlock(&ivp_device->clk_ref_mutex);

	for (i = ivp_device->opp_num - 1; i >= 0; i--) {
		if (cur_idx == i && clk_ref[i] == 1)
			continue;

		if (clk_ref[i]) {
			max_idx = i;
			break;
		}
	}

	if (index > max_idx)
		max_idx = index;

	/* update max index between cores and ivp_if*/
	if (freq[1])
		index = ivp_find_clk_index(ivp_device, 1, &freq[1]);
	else
		index = ivp_find_clk_index(ivp_device, 1,
						&ivp_device->current_freq[1]);
	if (index > max_idx)
		max_idx = index;

	voltage = ivp_device->opp_clk_table[max_idx].volt;

	ivp_device_power_lock(ivp_device);

	if (ivp_core->current_freq < freq[IVP_DSP_SEL]) {
		ret = ivp_set_voltage(ivp_device, voltage);
		if (ret)
			pr_err("Failed to do ivp_set_voltage: %d\n", ret);
		ret = ivp_set_freq(ivp_core, IVP_DSP_SEL, freq[IVP_DSP_SEL]);
		if (ret)
			pr_err("Failed to do ivp_set_freq: %d\n", ret);
	} else {
		ret = ivp_set_freq(ivp_core, IVP_DSP_SEL, freq[IVP_DSP_SEL]);
		if (ret)
			pr_err("Failed to do ivp_set_freq: %d\n", ret);
		ret = ivp_set_voltage(ivp_device, voltage);
		if (ret)
			pr_err("Failed to do ivp_set_voltage: %d\n", ret);
	}
#ifdef CONFIG_MTK_IVP_DVFS
	ivp_devfreq->current_freq = ivp_core->current_freq;
#endif

	for (i = 1; i < IVP_CLK_NUM; i++) {
		ret = ivp_set_freq(ivp_core, i, freq[i]);
		if (ret) {
			pr_err("Failed to do ivp_set_freq for %s: %d\n",
			       ivp_device->clk_name[i], ret);
		}
	}

	ivp_device_power_unlock(ivp_device);
	ivp_device_dvfs_report(ivp_device);

	return count;
}

static ssize_t show_ivp_opp(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	struct ivp_device *ivp_device = dev_get_drvdata(dev);
	int i, core_id, count = 0;

	if (!ivp_device->ivp_init_done) {
		dev_err(dev, "ivp not initialize!\n");
		return count;
	}

	for (core_id = 0; core_id < ivp_device->ivp_core_num; core_id++) {
		struct ivp_core *ivp_core = ivp_device->ivp_core[core_id];

		count += scnprintf(buf + count, PAGE_SIZE,
			  "core%d: dsp_freq = %ld\n",
			  core_id, ivp_core->current_freq);
	}
	for (i = 0; i < IVP_CLK_NUM; i++) {
		count += scnprintf(buf + count, PAGE_SIZE,
				  "%s = %ld\n", ivp_device->clk_name[i],
				  ivp_device->current_freq[i]);
	}
	count += scnprintf(buf + count, PAGE_SIZE,
			  "voltage = %ld\n",
			  ivp_device->current_voltage);
	count += scnprintf(buf + count, PAGE_SIZE,
			  "sram_voltage = %ld\n",
			  ivp_device->current_sram_voltage);
	return count;
}

static ssize_t show_ivp_opp_table(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	struct ivp_device *ivp_device = dev_get_drvdata(dev);
	struct ivp_opp_clk_table *table = ivp_device->opp_clk_table;
	int i;
	int count = 0;

	if (!ivp_device->ivp_init_done) {
		dev_err(dev, "ivp not initialize!\n");
		return count;
	}

	if (table) {
		count += scnprintf(buf + count, PAGE_SIZE,
					  "frequency, voltage\n");

		for (i = 0; i < ivp_device->opp_num; i++) {
			count += scnprintf(buf + count, PAGE_SIZE,
					  "%9ld, %ld\n", table[i].freq,
					  table[i].volt);
		}
	} else
		count += scnprintf(buf + count, PAGE_SIZE, "no opp table\n");


	return count;
}


static ssize_t show_ivp_core_state(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct ivp_device *ivp_device = dev_get_drvdata(dev);
	int core_id, count = 0;
	struct ivp_state_string state_string[4] = {
		{ IVP_UNPREPARE, "unprepare" },
		{ IVP_IDLE, "idle" },
		{ IVP_ACTIVE, "active" },
		{ IVP_SLEEP, "sleep" },
	};

	if (!ivp_device->ivp_init_done) {
		dev_err(dev, "ivp not initialize!\n");
		return count;
	}

	for (core_id = 0; core_id < ivp_device->ivp_core_num; core_id++) {
		struct ivp_core *ivp_core = ivp_device->ivp_core[core_id];

		if (!ivp_core)
			continue;
		count += scnprintf(buf + count, PAGE_SIZE,
				   "core %d state = %s\n", core_id,
				   state_string[ivp_core->state].string);
	}
	return count;
}

static ssize_t show_ivp_core_utilization(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct ivp_device *ivp_device = dev_get_drvdata(dev);
	struct ivp_util *ivp_util = ivp_device->ivp_util;
	unsigned long busy, util;
	int core_id, count = 0;

	if (!ivp_device->ivp_init_done) {
		dev_err(dev, "ivp not initialize!\n");
		return count;
	}

	count += scnprintf(buf + count, PAGE_SIZE, "utilization in %ld ms:\n",
			   ivp_util->prev_total);

	for (core_id = 0; core_id < ivp_device->ivp_core_num; core_id++) {
		struct ivp_core *ivp_core = ivp_device->ivp_core[core_id];

		if (!ivp_core)
			continue;

		if (ivp_util->prev_total == 0)
			util = 0;
		else {
			busy = ivp_core->prev_busy / USEC_PER_MSEC;
			util = busy * 100 / ivp_util->prev_total;
		}
		count += scnprintf(buf + count, PAGE_SIZE,
				   "core %d utilization = %ld%%\n", core_id,
				   util);
	}
	return count;
}

static ssize_t set_ivp_excp_cmd(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct ivp_device *ivp_device = dev_get_drvdata(dev);
	struct ivp_core *ivp_core;
	u32 set_exp_cmd;
	int core;

	if (sscanf(buf, "%d %d", &core, &set_exp_cmd) == 2) {
		if (core >= ivp_device->ivp_core_num || core < 0) {
			pr_err("%s %d: core should be in 0-%d\n",
			       __func__, __LINE__, ivp_device->ivp_core_num);
			return count;
		}
		if (set_exp_cmd >= IVP_EXP_CMD_NUM) {
			pr_err("%s %d: exp_cmd should be smaller than %d\n",
			       __func__, __LINE__, set_exp_cmd);
			return count;
		}
	} else {
		pr_err("%s %d: bad parameter, need 2 parameter!\n",
		       __func__, __LINE__);
		return count;
	}

	ivp_core = ivp_device->ivp_core[core];

	if (ivp_core)
		ivp_core->exp_cmd = set_exp_cmd;

	return count;
}

static ssize_t show_ivp_excp_cmd(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct ivp_device *ivp_device = dev_get_drvdata(dev);
	int core_id, count = 0;
	struct ivp_state_string state_string[4] = {
		{ IVP_RST_NEXT_CMD, "dsp reset and do next command" },
		{ IVP_RST_RETRY, "dsp reset and retry this command" },
		{ IVP_RST_ONLY, "only dsp reset and do nothing" },
		{ IVP_CONTINUE, "directly continue" },
	};

	if (!ivp_device->ivp_init_done) {
		dev_err(dev, "ivp not initialize!\n");
		return count;
	}

	for (core_id = 0; core_id < ivp_device->ivp_core_num; core_id++) {
		struct ivp_core *ivp_core = ivp_device->ivp_core[core_id];

		if (!ivp_core)
			continue;
		count += scnprintf(buf + count, PAGE_SIZE,
				   "core %d exp_cmd = %s\n", core_id,
				   state_string[ivp_core->exp_cmd].string);
	}

	return count;
}

static ssize_t set_ivp_trace_cmd(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct ivp_device *ivp_device = dev_get_drvdata(dev);
	u32 trace_func;

	if (!ivp_device->ivp_init_done) {
		dev_err(dev, "ivp not initialize!\n");
		return count;
	}

	if (!kstrtou32(buf, 0, &trace_func))
		ivp_device->trace_func = trace_func;
	else
		pr_err("%s %d: bad parameter\n", __func__, __LINE__);

	if (ivp_device->trace_func & IVP_TRACE_EVENT_PMU) {
		if (ivp_device->profiler.enable_timer == 0)
			ivp_init_profiler(ivp_device, 1);
	}

	return count;
}

static ssize_t show_ivp_trace_cmd(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct ivp_device *ivp_device = dev_get_drvdata(dev);
	int count = 0;
	u32 func = ivp_device->trace_func;

	count += scnprintf(buf + count, PAGE_SIZE, "trace_func = 0x%x\n",
			   func);

	count += scnprintf(buf + count, PAGE_SIZE, "%s %s\n",
			   "trace vpu cmd execute : ",
			   (func & IVP_TRACE_EVENT_EXECUTE)
			   ? "enable" : "disable");

	count += scnprintf(buf + count, PAGE_SIZE, "%s %s\n",
			   "trace vpu dvfs : ",
			   (func & IVP_TRACE_EVENT_DVFS)
			   ? "enable" : "disable");

	count += scnprintf(buf + count, PAGE_SIZE, "%s %s\n",
			   "trace vpu busyrate : ",
			   (func & IVP_TRACE_EVENT_BUSYRATE)
			   ? "enable" : "disable");

	count += scnprintf(buf + count, PAGE_SIZE, "%s %s\n",
			   "trace vpu pmu : ",
			   (func & IVP_TRACE_EVENT_PMU)
			   ? "enable" : "disable");

	return count;
}

static ssize_t show_ivp_algo(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	struct ivp_device *ivp_device = dev_get_drvdata(dev);
	int count = 0;

	count = ivp_show_algo(ivp_device, buf, count);

	return count;
}

DEVICE_ATTR(ivp_dbg, 0644, show_ivp_debug_info,
	    set_ivp_debug_info);
DEVICE_ATTR(ivp_ptrace, 0644, show_ivp_ptrace_log,
	    set_ivp_ptrace_level);
DEVICE_ATTR(ivp_fw_ver, 0444, show_ivp_fw_ver, NULL);
DEVICE_ATTR(ivp_register_dump, 0644,
	    show_internal_register_para, ivp_internal_register_dump);
DEVICE_ATTR(ivp_cmd_timeout, 0644,
	    ivp_get_cmd_timeout, ivp_set_cmd_timeout);
DEVICE_ATTR(ivp_core_state, 0444, show_ivp_core_state, NULL);
DEVICE_ATTR(ivp_core_utilization, 0444, show_ivp_core_utilization, NULL);
DEVICE_ATTR(ivp_opp_ctl, 0644, show_ivp_opp, set_ivp_opp);
DEVICE_ATTR(ivp_opp_table, 0444, show_ivp_opp_table, NULL);
DEVICE_ATTR(ivp_autosuspend, 0644, show_ivp_autosuspend, set_ivp_autosuspend);
DEVICE_ATTR(ivp_initialize, 0644, show_ivp_initialize, set_ivp_initialize);
DEVICE_ATTR(ivp_excp_cmd, 0644, show_ivp_excp_cmd, set_ivp_excp_cmd);
DEVICE_ATTR(ivp_trace_cmd, 0644, show_ivp_trace_cmd, set_ivp_trace_cmd);
DEVICE_ATTR(ivp_algo, 0444, show_ivp_algo, NULL);

static struct attribute *ivp_sysfs_entries[] = {
	&dev_attr_ivp_dbg.attr,
	&dev_attr_ivp_ptrace.attr,
	&dev_attr_ivp_fw_ver.attr,
	&dev_attr_ivp_register_dump.attr,
	&dev_attr_ivp_cmd_timeout.attr,
	&dev_attr_ivp_core_state.attr,
	&dev_attr_ivp_core_utilization.attr,
	&dev_attr_ivp_opp_ctl.attr,
	&dev_attr_ivp_opp_table.attr,
	&dev_attr_ivp_autosuspend.attr,
	&dev_attr_ivp_initialize.attr,
	&dev_attr_ivp_excp_cmd.attr,
	&dev_attr_ivp_trace_cmd.attr,
	&dev_attr_ivp_algo.attr,
	NULL,
};

static const struct attribute_group ivp_attr_group = {
	.attrs = ivp_sysfs_entries,
};

int ivp_create_sysfs(struct device *dev)
{
	int ret;

	ret = sysfs_create_group(&dev->kobj, &ivp_attr_group);
	if (ret)
		dev_err(dev, "create sysfs group error, %d\n", ret);

	return ret;
}

void ivp_remove_sysfs(struct device *dev)
{
	sysfs_remove_group(&dev->kobj, &ivp_attr_group);
}

