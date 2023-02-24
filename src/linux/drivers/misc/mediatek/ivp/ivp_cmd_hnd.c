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

#include <linux/clk.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/regulator/consumer.h>

/* ivp fw interface header files*/
#include "include/fw/ivp_dbg_ctrl.h"
#include "include/fw/ivp_error.h"
#include "include/fw/ivp_interface.h"
#include "include/fw/ivp_macro.h"
#include "include/fw/ivp_typedef.h"

/* ivp driver header files */
#include "ivp_cmd_hnd.h"
#include "ivp_err_hnd.h"
#include "ivp_fw_interface.h"
#include "ivp_power_ctl.h"
#include "ivp_profile.h"
#include "ivp_queue.h"
#include "ivp_reg.h"
#include "ivp_systrace.h"
#include "ivpbuf-core.h"

#define IVP_TEMPLOG_SIZE 512
#define IVP_IN_PWAIT	 0x00000080

#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
int int_count[3] = {0};
int resend_int_count[3] = {0};
int received_int_count[3] = {0};
IVP_UINT32 curr_timestamp[3] = {0};
IVP_UINT32 prev_timestamp[3] = {0};
IVP_UINT32 prev_pc[3];
enum ivp_state prev_fw_state[3];
#endif

static inline void lock_command(struct ivp_core *ivp_core)
{
	mutex_lock(&ivp_core->cmd_mutex);
	ivp_core->is_cmd_done = false;
}

static inline int check_command(IVP_UINT32 timestamp,
				void __iomem *ivpctrl_base)
{
	IVP_UINT32 tmp;

	tmp = (IVP_UINT32)ivp_read_reg32(ivpctrl_base,
					 IVP_CTRL_REG_XTENSA_INFO24);

	return (tmp == timestamp) ? 0 : -EINVAL;
}

static inline int wait_command(struct ivp_core *ivp_core, u32 timeout)
{
	return (wait_event_interruptible_timeout(ivp_core->cmd_wait,
						 ivp_core->is_cmd_done,
						 msecs_to_jiffies
						 (timeout)) > 0)
	    ? 0 : -ETIMEDOUT;
}

static inline void unlock_command(struct ivp_core *ivp_core)
{
	mutex_unlock(&ivp_core->cmd_mutex);
}

static void ivp_clr_interrupt(void __iomem *ivpctrl_base)
{
	ivp_set_reg32(ivpctrl_base, IVP_CTRL_REG_XTENSA_INT, IVP_INT_XTENSA);
}

#ifdef CONFIG_SIE_PRINTK_CUSTOM_LOG_DRIVER
static void ivp_log_dump(struct ivp_core *ivp_core)
{
	char temp[IVP_TEMPLOG_SIZE] = { 0 };
	const char *src;
	size_t read_sz;
	uintptr_t rptr = (uintptr_t)ivp_core->working_buffer->dbg_log;

	if (ivp_core->core >= IVP_CORE_NUM)
		return;

	src	= (char *)rptr;
	read_sz	= sizeof(temp) - 1;
	memcpy(temp, src, read_sz);

	if (strnlen(temp, read_sz) > 0)
		pr_warn("IVP[%u] : %s", ivp_core->core, temp);

	memset(ivp_core->working_buffer->dbg_log, 0,
	       IVP_FW_DBG_MSG_BUFFER_SIZE);
}
#endif

#if defined(CONFIG_MTK_IVP_DEBUG_LOG)

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

void ivp_debug_log(struct ivp_core *ivp_core)
{
	void __iomem *ivpctrl_base = ivp_core->ivpctrl_base;

	dev_err(ivp_core->dev, "===================================\n");
	dev_err(ivp_core->dev, "info before send command:\n");
	dev_err(ivp_core->dev, "pc = 0x%08x\n", prev_pc[ivp_core->core]);
	dev_err(ivp_core->dev, "fw_state = 0x%08x\n",
		prev_fw_state[ivp_core->core]);

	dev_err(ivp_core->dev, "current info:\n");
	dev_err(ivp_core->dev, "pc = 0x%08x\n", (IVP_UINT32)ivp_read_reg32(
				ivpctrl_base, IVP_CTRL_REG_DEBUG_INFO05));
	dev_err(ivp_core->dev, "fw_state = 0x%08x\n",
		(IVP_UINT32)ivp_read_reg32(ivpctrl_base,
				IVP_CTRL_REG_XTENSA_INFO00));
	dev_err(ivp_core->dev, "fw done status = 0x%08x\n",
		(IVP_UINT32)ivp_read_reg32(ivpctrl_base,
					   IVP_CTRL_REG_DONE_ST));
	dev_err(ivp_core->dev, "clr int bit = 0x%08x\n",
		(IVP_UINT32)ivp_read_reg32(ivp_core->ivpctrl_base,
					   IVP_CTRL_REG_XTENSA_INT));
	dev_err(ivp_core->dev, "send interrupt counts = %d\n",
		int_count[ivp_core->core]);
	dev_err(ivp_core->dev, "resend interrupt counts = %d\n",
		resend_int_count[ivp_core->core]);
	dev_err(ivp_core->dev, "received interrupt counts = %d\n",
		received_int_count[ivp_core->core]);

	dev_err(ivp_core->dev, "about timestamp:\n");
	dev_err(ivp_core->dev, "previous timestamp = 0x%08x\n",
		prev_timestamp[ivp_core->core]);
	dev_err(ivp_core->dev, "curren timestamp = 0x%08x\n",
		curr_timestamp[ivp_core->core]);
	dev_err(ivp_core->dev, "send to fw timestamp = 0x%08x\n",
		(IVP_UINT32)ivp_read_reg32(ivpctrl_base,
					   IVP_CTRL_REG_XTENSA_INFO15));
	dev_err(ivp_core->dev, "fw irq handler received timestamp = 0x%08x\n",
		(IVP_UINT32)ivp_read_reg32(ivpctrl_base,
					   IVP_CTRL_REG_XTENSA_INFO23));
	dev_err(ivp_core->dev, "fw process start timestamp = 0x%08x\n",
		(IVP_UINT32)ivp_read_reg32(ivpctrl_base,
					   IVP_CTRL_REG_XTENSA_INFO24));
	dev_err(ivp_core->dev, "fw process done timestamp = 0x%08x\n",
		(IVP_UINT32)ivp_read_reg32(ivpctrl_base,
					   IVP_CTRL_REG_XTENSA_INFO25));
	dev_err(ivp_core->dev, "===================================\n");

	dev_err(ivp_core->dev, "ivp fw debug message buffer:\n");

	print_buffer(ivp_core->working_buffer->dbg_log,
		     0, IVP_FW_DBG_MSG_BUFFER_SIZE - 1);
}
#endif

irqreturn_t ivp_isr_handler(int irq, void *ivp_core_info)
{
	struct ivp_core *ivp_core = (struct ivp_core *)ivp_core_info;

	if (ivp_core->kservice_state == IVP_KSERVICE_BUSY_WITH_GCE)
		return IRQ_HANDLED;

	ivp_clr_interrupt(ivp_core->ivpctrl_base);

#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
	received_int_count[ivp_core->core]++;
#endif

	ivp_core->is_cmd_done = true;
	wake_up_interruptible(&ivp_core->cmd_wait);

	return IRQ_HANDLED;
}

void ivp_device_power_lock(struct ivp_device *ivp_device)
{
	int core_id;

	for (core_id = 0; core_id < ivp_device->ivp_core_num; core_id++)
		ivp_core_power_lock(ivp_device->ivp_core[core_id]);
}

void ivp_device_power_unlock(struct ivp_device *ivp_device)
{
	int core_id;

	for (core_id = 0; core_id < ivp_device->ivp_core_num; core_id++)
		ivp_core_power_unlock(ivp_device->ivp_core[core_id]);
}

void ivp_enable_counter(struct ivp_core *ivp_core)
{
	void __iomem *debug_base = ivp_core->ivpdebug_base;

	if (ivp_core->enable_profiler) {
		ivp_profiler_core_enable(ivp_core);
		return;
	}

	if (!debug_base)
		return;

	/* ctrl_bit : XTPERF_CNT_CYCLES */
	ivp_write_reg32(debug_base, 0x1100, 0x000100F0);
	/* status : reset to zero */
	ivp_write_reg32(debug_base, 0x1180, 0);
	/* counter : reset to zero */
	ivp_write_reg32(debug_base, 0x1080, 0);
	/* enable PMG_ENABLE */
	ivp_write_reg32(debug_base, 0x1000, 0x1);
}

void ivp_disable_counter(struct ivp_core *ivp_core)
{
	void __iomem *debug_base = ivp_core->ivpdebug_base;

	if (ivp_core->enable_profiler) {
		ivp_profiler_core_disable(ivp_core);
		return;
	}

	if (!debug_base)
		return;

	ivp_write_reg32(debug_base, 0x1000, 0);
	ivp_write_reg32(debug_base, 0x1100, 0);
}

u32 ivp_read_counter(struct ivp_core *ivp_core)
{
	void __iomem *debug_base = ivp_core->ivpdebug_base;

	if (!debug_base)
		return 0;

	return ivp_read_reg32(debug_base, 0x1080);
}

void ivp_core_set_auto_suspend(struct ivp_core *ivp_core, int sync)
{
	pm_runtime_mark_last_busy(ivp_core->dev);
	if (sync)
		pm_runtime_put_sync(ivp_core->dev);
	else
		pm_runtime_put_sync_autosuspend(ivp_core->dev);
}

void ivp_core_power_lock(struct ivp_core *ivp_core)
{
	mutex_lock(&ivp_core->power_mutex);
}

void ivp_core_power_unlock(struct ivp_core *ivp_core)
{
	mutex_unlock(&ivp_core->power_mutex);
}

int ivp_hw_boot_sequence(struct ivp_core *ivp_core)
{
	struct ivp_boot_cmd *boot_cmd;
	struct ivp_dbg_info *dbg_info;
	void __iomem *ivpctrl_base = ivp_core->ivpctrl_base;

	dbg_info = (struct ivp_dbg_info *)ivp_core->working_buffer->dbg_cmd;

	boot_cmd = (struct ivp_boot_cmd *)ivp_core->working_buffer->boot_cmd;
	boot_cmd->log_buf_base = ivp_core->working_buffer_iova +
		offsetof(struct ivp_working_buf, dbg_log);
	boot_cmd->log_buf_size = IVP_FW_DBG_MSG_BUFFER_SIZE;
	boot_cmd->ptrace_log_buf_base = ivp_core->working_buffer_iova +
		offsetof(struct ivp_working_buf, ptrace_log);
	boot_cmd->ptrace_log_buf_size = IVP_FW_PTRACE_MSG_BUFFER_SIZE;
	boot_cmd->excp_dump_buf_base = ivp_core->working_buffer_iova +
		offsetof(struct ivp_working_buf, excp_dump);
	boot_cmd->excp_dump_buf_size = IVP_FW_EXCEPTION_DUMP_BUFFER_SIZE;
	if (dbg_info) {
		boot_cmd->dbg_control = dbg_info->control;
		boot_cmd->dbg_level = dbg_info->level;
		boot_cmd->ptrace_level = dbg_info->ptrace_level;
	} else {
		boot_cmd->dbg_control = 0;
		boot_cmd->dbg_level = 0;
		boot_cmd->ptrace_level = 0;
	}
	boot_cmd->uart_base_idx = IVP_UART1;
	ivp_fw_boot_sequence(ivpctrl_base, ivp_core, BOOT_NORMAL_BOOT);

	ivp_enable_counter(ivp_core);

#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
	int_count[ivp_core->core] = 0;
	resend_int_count[ivp_core->core] = 0;
	received_int_count[ivp_core->core] = 0;
	curr_timestamp[ivp_core->core] = 0;
	prev_timestamp[ivp_core->core] = 0;
#endif

	return 0;
}

int ivp_load_firmware(struct ivp_core *ivp_core, const char *fw_name)
{
	/* fw info */
	const struct firmware *ivp_fw;
	const u8 *fw_base = NULL;
	/* image program info */
	struct ivp_imagefile_header *image_header = NULL;
	struct ivp_image_program *program_header = NULL;
	u32 program_offset;
	u32 program_size;
	u32 program_num;
	/* device info */
	struct ivp_device *ivp_device = ivp_core->ivp_device;
	struct device *dev = ivp_device->dev;
	struct ivp_core_mem *core_m;
	/* local variable */
	size_t size;
	dma_addr_t busaddr;
	void *start_vaddr, *vaddr;
	int ret = 0, i;

	/* request fw */
	ret = request_firmware(&ivp_fw, fw_name, dev);
	if (ret < 0) {
		pr_err("Failed to load %s, %d\n", fw_name, ret);
		return ret;
	}

	/* get the fw information */
	fw_base = ivp_fw->data;
	image_header = (struct ivp_imagefile_header *)fw_base;
	ivp_core->ivp_entry = image_header->ih_entry;
	program_offset = image_header->ih_phoff;
	program_size = image_header->ih_phensize;
	program_num = image_header->ih_phnum;

	core_m = &ivp_device->ivp_mng->core_mem[ivp_core->core];
	/* init debug command memory */
	ivp_core->working_buffer = (struct ivp_working_buf *)core_m->sdbg.vaddr;
	/* clear buffer */
	memset(ivp_core->working_buffer, 0, sizeof(struct ivp_working_buf));
	ivp_core->working_buffer_iova = core_m->sdbg.dma_addr;
	if (sizeof(struct ivp_working_buf) > core_m->sdbg.size) {
		dev_err(ivp_core->dev,
			"size of ivp_working_buf(%lx) > allocated size(%x)\n",
			sizeof(struct ivp_working_buf), core_m->sdbg.size);
		ret = -ENOMEM;
		goto err_release_fw;
	}

	/* mmap ivp_entry[core] memory */
	busaddr = core_m->fw.dma_addr;
	start_vaddr = core_m->fw.vaddr;
#ifndef IVP_PERFORMANCE
	pr_debug
	    ("ivp_load_fw, fw memory info core%d, busaddr=%pad, vaddr=%p\n",
	     ivp_core->core, &busaddr, start_vaddr);
#endif

	/* copy each program into ivp_entry[core] memory */
	for (i = 0; i < program_num; i++) {
		program_header =
		    (struct ivp_image_program *)(fw_base + program_offset +
						 i *
						 sizeof(struct
							ivp_image_program));
		busaddr = program_header->ip_paddr;
		size = program_header->ip_filesz;

		vaddr =
		    start_vaddr + program_header->ip_paddr -
		    image_header->ih_entry;

		if ((program_header->ip_offset + size) <= ivp_fw->size)
			memcpy(vaddr, fw_base + program_header->ip_offset,
			       size);
	}

err_release_fw:
	/* release fw */
	release_firmware(ivp_fw);
	return ret;
}

int ivp_check_fw_boot_status(struct ivp_core *ivp_core)
{
	int ret;
	u32 state;

	lock_command(ivp_core);
	ivp_release_fw(ivp_core->ivpctrl_base);

	ret = wait_command(ivp_core, ivp_core->ivp_device->timeout);
	if (ret) {
		pr_err
		    ("%s:IVP timeout to external boot\n", __func__);
		goto out;
	}
	ret = ivp_fw_get_ivp_status(ivp_core->ivpctrl_base, &state);

out:
	unlock_command(ivp_core);
	return ret;
}

static int ivp_check_cmd_send(struct ivp_core *ivp_core, ktime_t time,
			       u32 timeout)
{
	void __iomem *ivpctrl_base = ivp_core->ivpctrl_base;
	IVP_UINT32 timestamp = time.tv64;
	bool is_timeout = false;

#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
	prev_timestamp[ivp_core->core] = curr_timestamp[ivp_core->core];
	curr_timestamp[ivp_core->core] = timestamp;
#endif

	while (check_command(timestamp, ivpctrl_base)) {
		if (ktime_ms_delta(ktime_get(), time) > timeout) {
			enum ivp_state fw_state =
				(enum ivp_state)ivp_read_reg32(ivpctrl_base,
						IVP_CTRL_REG_XTENSA_INFO00);

			if (is_timeout)
				return -ETIMEDOUT;
			if (fw_state == STATE_WAIT_ISR) {
				time = ktime_get();
				timestamp++;
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
				prev_timestamp[ivp_core->core] =
					curr_timestamp[ivp_core->core];
				curr_timestamp[ivp_core->core] = timestamp;
#endif
				ivp_write_reg32(ivpctrl_base,
						IVP_CTRL_REG_XTENSA_INFO15,
						timestamp);
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
				prev_pc[ivp_core->core] =
					(IVP_UINT32)ivp_read_reg32(
						ivp_core->ivpctrl_base,
						IVP_CTRL_REG_DEBUG_INFO05);
				prev_fw_state[ivp_core->core] = fw_state;
#endif
				ivp_set_reg32(ivpctrl_base,
					      IVP_CTRL_REG_CTL_XTENSA_INT,
					      (0x1 << INT00_LEVEL_L1));
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
				resend_int_count[ivp_core->core]++;
#endif
			}
			is_timeout = true;
		}
	}

	return 0;
}

#if defined(MTK_IVP_LOAD_FROM_KERNEL)
static int ivp_check_elf_header(const struct firmware *ivp_algo,
				 uint32_t *flag)
{
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)ivp_algo->data;

	if (ivp_algo->size < sizeof(Elf32_Ehdr)) {
		pr_err("%s: not an ELF\n", __func__);
		return -EINVAL;
	}

	if (ehdr->e_ident[EI_MAG0] != 0x7f)
		return -EINVAL;

	if (ehdr->e_ident[EI_MAG1] != 'E')
		return -EINVAL;

	if (ehdr->e_ident[EI_MAG2] != 'L')
		return -EINVAL;

	if (ehdr->e_ident[EI_MAG3] != 'F')
		return -EINVAL;

	if (ehdr->e_ident[EI_CLASS] != ELFCLASS32)
		return -EINVAL;

	if (ehdr->e_type != ET_DYN)
		*flag = FLAG_LOAD_FLO;
	else
		*flag = 0;

	return 0;
}

int ivp_request_algo(struct ivp_request *req, struct ivp_device *ivp_device)
{
	int ret;
	const struct firmware *ivp_algo;
	struct ivp_alloc_buf alloc_buf;
	struct ivp_buffer *ivpb;
	struct ivp_user *ivp_user = req->user;
	struct ivp_algo_context *algo_ctx;
	struct ivp_manage *mng = ivp_device->ivp_mng;

	if (req->algo_name[0] == '\0') {
		pr_err("%s: No algo to be requested!\n", __func__);
		req->cmd_result = IVP_BAD_PARAMETER;
		return -EINVAL;
	}

	algo_ctx = idr_find(&ivp_user->algo_idr[req->core], req->drv_algo_hnd);
	if (!algo_ctx) {
		pr_err("%s: fail to get algo_ctx\n", __func__);
		return -EINVAL;
	}

	ret = request_firmware(&ivp_algo, req->algo_name,
			       ivp_device->dev);
	if (ret < 0) {
		pr_err("Failed to load %s, %d\n", req->algo_name, ret);
		req->cmd_result = IVP_BAD_PARAMETER;
		return ret;
	}

	ret = ivp_check_elf_header(ivp_algo, &req->argv);
	if (ret) {
		pr_err("%s: %s not an elf\n", __func__, req->algo_name);
		req->cmd_result = IVP_BAD_PARAMETER;
		goto out;
	}

	alloc_buf.req_size = ivp_algo->size;
	alloc_buf.is_cached = 1;
	ret = ivp_manage_alloc(ivp_device, &alloc_buf);
	if (ret) {
		pr_err("%s: alloc buf failed, ret=%d\n", __func__, ret);
		req->cmd_result = IVP_OUT_OF_MEMORY;
		goto out;
	}
	ivpb = idr_find(&mng->ivp_idr, alloc_buf.handle);
	if (!ivpb) {
		struct ivp_free_buf free_buf;

		pr_err("%s: fail to get ivp_buffer\n", __func__);
		free_buf.handle = alloc_buf.handle;
		ivp_manage_free(ivp_device->ivp_mng, &free_buf);
		ret = -EINVAL;
		goto out;
	}

	memcpy(ivpb->vaddr, ivp_algo->data, ivp_algo->size);

	req->addr_handle = ivpb->dma_addr;
	req->size = ivpb->length;
	algo_ctx->algo_buf_hnd = alloc_buf.handle;

out:
	release_firmware(ivp_algo);
	return ret;
}
#endif

static bool ivp_in_wait_mode(void __iomem *ivpctrl_base)
{
	int cnt = 0;
	enum ivp_state state;

	while ((IVP_UINT32)ivp_read_reg32(ivpctrl_base, IVP_CTRL_REG_DONE_ST) !=
	       IVP_IN_PWAIT && cnt++ < 3)
		usleep_range(200, 300);
	if ((IVP_UINT32)ivp_read_reg32(ivpctrl_base, IVP_CTRL_REG_DONE_ST) ==
	    IVP_IN_PWAIT)
		return true;

	state = (enum ivp_state)ivp_read_reg32(ivpctrl_base,
					       IVP_CTRL_REG_XTENSA_INFO00);
	if (state == STATE_WAIT_ISR)
		return true;
	else if (state == STATE_WAIT_ISR_BUSY)
		return true;

	return false;
}

int ivp_load_algo(struct ivp_request *req, struct ivp_device *ivp_device)
{
	int ret;
	u32 core = req->core;
	void __iomem *ivpctrl_base;
	struct apmcu_load_cmd load_cmd;
	struct apmcu_load_cmd_rtn load_cmd_rtn;
	struct ivp_user *ivp_user = req->user;
	struct ivp_core *ivp_core = ivp_device->ivp_core[core];
	struct ivp_algo_context *algo_ctx;
	ktime_t timestamp = ktime_get();

	algo_ctx = idr_find(&ivp_user->algo_idr[core], req->drv_algo_hnd);
	if (!algo_ctx) {
		pr_err("%s: fail to get algo_ctx\n", __func__);
		req->cmd_result = IVP_KERNEL_LINVALID;
		return -EINVAL;
	}
	ivpctrl_base = ivp_core->ivpctrl_base;

	load_cmd.apmcu_cmd = CMD_LOAD_PROC;
	load_cmd.flags = req->argv;
	load_cmd.algo_size = req->size;
	load_cmd.algo_base = req->addr_handle;
	load_cmd.timestamp = timestamp.tv64;

	lock_command(ivp_core);
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
	prev_pc[ivp_core->core] = (IVP_UINT32)ivp_read_reg32(
		ivp_core->ivpctrl_base, IVP_CTRL_REG_DEBUG_INFO05);
	prev_fw_state[ivp_core->core] = (enum ivp_state)ivp_read_reg32(
		ivp_core->ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO00);
#endif
	if (!ivp_in_wait_mode(ivpctrl_base))
		pr_warn("%s: fw is not in wait mode [%u]\n", __func__, core);
	ivp_fw_load_algo(&load_cmd, ivpctrl_base);
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
	int_count[ivp_core->core]++;
#endif

	ret = ivp_check_cmd_send(ivp_core, timestamp, req->duration);
	if (ret) {
		pr_err("%s: fail to send command\n", __func__);
		req->cmd_result = IVP_KERNEL_LTIMEOUT;
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
		ivp_debug_log(ivp_core);
#endif
		goto out;
	}

	ret = wait_command(ivp_core, req->duration);
	if (ret) {
		pr_err("%s: timeout to do loader\n", __func__);
		algo_ctx = NULL;
		req->cmd_result = IVP_KERNEL_LTIMEOUT;
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
		ivp_debug_log(ivp_core);
#endif
		goto out;
	}
	ivp_fw_load_algo_rtn(&load_cmd_rtn, ivpctrl_base);
	algo_ctx->fw_algo_hnd = load_cmd_rtn.algo_hnd;
	algo_ctx->algo_code_mem = load_cmd_rtn.code_mem;
	algo_ctx->algo_data_mem = load_cmd_rtn.data_mem;
	req->cmd_result = load_cmd_rtn.cmd_result;
	if (req->cmd_result != 0) {
		pr_err("load algo failed: 0x%x.\n", req->cmd_result);
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
		ivp_debug_log(ivp_core);
#endif
		goto out;
	}

	algo_ctx->algo_flags = load_cmd.flags;
	algo_ctx->algo_size = load_cmd.algo_size;
	algo_ctx->algo_iova = req->addr_handle;
	algo_ctx->state = IVP_ALGO_STATE_READY;

out:
	unlock_command(ivp_core);
	if (req->cmd_result == IVP_EXCEPTION_FAULT) {
		ivp_get_error_report(ivp_core, req);
		ivp_excp_handler (ivp_core, req);
	}
	return ret;
}

int ivp_reload_algo(struct ivp_device *ivp_device, struct ivp_request *req)
{
	int ret;
	u32 core = req->core;
	void __iomem *ivpctrl_base;
	struct apmcu_load_cmd load_cmd;
	struct apmcu_load_cmd_rtn load_cmd_rtn = {0};
	struct ivp_user *ivp_user = req->user;
	struct ivp_core *ivp_core = ivp_device->ivp_core[core];
	struct ivp_algo_context *algo_ctx;
	ktime_t timestamp = ktime_get();

	algo_ctx = idr_find(&ivp_user->algo_idr[core], req->drv_algo_hnd);
	if (!algo_ctx) {
		pr_err("%s: fail to get algo_ctx\n", __func__);
		/* store fail result of reload on first 16 bits  */
		req->cmd_result = (IVP_KERNEL_RINVALID * 65536) +
			(req->cmd_result & 0x0000ffff);
		return -EINVAL;
	}
	ivpctrl_base = ivp_core->ivpctrl_base;

	load_cmd.apmcu_cmd = CMD_LOAD_PROC;
	load_cmd.flags = algo_ctx->algo_flags;
	load_cmd.algo_size = algo_ctx->algo_size;
	load_cmd.algo_base = algo_ctx->algo_iova;
	load_cmd.timestamp = timestamp.tv64;

	lock_command(ivp_core);
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
	prev_pc[ivp_core->core] = (IVP_UINT32)ivp_read_reg32(
		ivp_core->ivpctrl_base, IVP_CTRL_REG_DEBUG_INFO05);
	prev_fw_state[ivp_core->core] = (enum ivp_state)ivp_read_reg32(
		ivp_core->ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO00);
#endif
	if (!ivp_in_wait_mode(ivpctrl_base))
		pr_warn("%s: fw is not in wait mode [%u]\n", __func__, core);
	ivp_fw_load_algo(&load_cmd, ivpctrl_base);
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
	int_count[ivp_core->core]++;
#endif

	ret = ivp_check_cmd_send(ivp_core, timestamp,
				 algo_ctx->algo_load_duration);
	if (ret) {
		pr_err("%s: fail to send command\n", __func__);
		/* store fail result of reload on first 16 bits  */
		req->cmd_result = (IVP_KERNEL_RTIMEOUT * 65536) +
			(req->cmd_result & 0x0000ffff);
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
		ivp_debug_log(ivp_core);
#endif
		goto out;
	}

	ret = wait_command(ivp_core, algo_ctx->algo_load_duration);
	if (ret) {
		pr_err("%s: timeout to re-load algo\n", __func__);
		/* store fail result of reload on first 16 bits  */
		req->cmd_result = (IVP_KERNEL_RTIMEOUT * 65536) +
			(req->cmd_result & 0x0000ffff);
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
		ivp_debug_log(ivp_core);
#endif
		goto out;
	}
	ivp_fw_load_algo_rtn(&load_cmd_rtn, ivpctrl_base);
	algo_ctx->fw_algo_hnd = load_cmd_rtn.algo_hnd;
	if (load_cmd_rtn.cmd_result != 0) {
		pr_err("%s: cmd_result = 0x%08x [%u]\n", __func__,
		       load_cmd_rtn.cmd_result, core);
		/* store fail result of reload on first 16 bits  */
		req->cmd_result = (load_cmd_rtn.cmd_result * 65536) +
			(req->cmd_result & 0x0000ffff);
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
		ivp_debug_log(ivp_core);
#endif
	}

out:
	unlock_command(ivp_core);
	if (load_cmd_rtn.cmd_result == IVP_EXCEPTION_FAULT) {
		ivp_get_error_report(ivp_core, req);
		ivp_excp_handler (ivp_core, req);
	}
	return ret;
}

int ivp_exec_algo(struct ivp_request *req, struct ivp_device *ivp_device)
{
	u32 core = req->core;
	int ret = 0;
	void __iomem *ivpctrl_base;
	struct apmcu_exec_cmd exec_cmd;
	struct apmcu_exec_cmd_rtn exec_cmd_rtn;
	struct ivp_user *ivp_user = req->user;
	struct ivp_core *ivp_core = ivp_device->ivp_core[core];
	struct ivp_algo_context *algo_ctx;
	ktime_t timestamp = ktime_get();

	algo_ctx = idr_find(&ivp_user->algo_idr[core], req->drv_algo_hnd);
	if (!algo_ctx) {
		dev_err(ivp_core->dev, "%s: no algo can excute. [%u]\n",
			__func__, core);
		req->cmd_result = IVP_KERNEL_EINVALID;
		return ret;
	}

	if (algo_ctx->state == IVP_ALGO_STATE_RESET) {
		pr_warn("%s, to reload algo, algo_ctx->state=%d [%u]\n",
			__func__, algo_ctx->state, core);
		ivp_reload_algo(ivp_device, req);
		algo_ctx->state = IVP_ALGO_STATE_READY;
	} else if (algo_ctx->state == IVP_ALGO_STATE_UNSET) {
		pr_warn("%s, algo has been unloaded [%u]\n", __func__, core);
		req->cmd_result = IVP_KERNEL_EINVALID;
		return -EINVAL;
	}

	ivpctrl_base = ivp_core->ivpctrl_base;

	exec_cmd.apmcu_cmd = CMD_EXEC_PROC;
	exec_cmd.algo_hnd = algo_ctx->fw_algo_hnd;
	exec_cmd.algo_arg_size = req->size;
	exec_cmd.algo_arg_base = req->addr_handle;
	exec_cmd.timestamp = timestamp.tv64;

	lock_command(ivp_core);
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
	prev_pc[ivp_core->core] = (IVP_UINT32)ivp_read_reg32(
		ivp_core->ivpctrl_base, IVP_CTRL_REG_DEBUG_INFO05);
	prev_fw_state[ivp_core->core] = (enum ivp_state)ivp_read_reg32(
		ivp_core->ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO00);
#endif
	if (!ivp_in_wait_mode(ivpctrl_base))
		pr_warn("%s: fw is not in wait mode [%u]\n", __func__, core);
	ivp_fw_exec_algo(&exec_cmd, ivpctrl_base);
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
	int_count[ivp_core->core]++;
#endif

	ret = ivp_check_cmd_send(ivp_core, timestamp, req->duration);
	if (ret) {
		pr_err("%s: fail to send command [%u]\n", __func__, core);
		req->cmd_result = IVP_KERNEL_ETIMEOUT;
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
		ivp_debug_log(ivp_core);
#endif
		goto out;
	}

	ret = wait_command(ivp_core, req->duration);
	if (ret) {
		pr_err("%s: timeout to do execution [%u]\n", __func__, core);
		req->cmd_result = IVP_KERNEL_ETIMEOUT;
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
		ivp_debug_log(ivp_core);
#endif
		goto out;
	}
	ivp_fw_exec_algo_rtn(&exec_cmd_rtn, ivpctrl_base);
	req->cmd_result = exec_cmd_rtn.cmd_result;
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
	if (req->cmd_result != 0) {
		pr_err("%s: cmd_result = 0x%08x [%u]\n", __func__,
		       req->cmd_result, core);
		ivp_debug_log(ivp_core);
	}
#endif

out:
#ifdef CONFIG_SIE_PRINTK_CUSTOM_LOG_DRIVER
	ivp_log_dump(ivp_core);
#endif
	unlock_command(ivp_core);
	if (req->cmd_result == IVP_EXCEPTION_FAULT) {
		ivp_get_error_report(ivp_core, req);
		ivp_excp_handler (ivp_core, req);
	}

	return ret;
}

int ivp_unload_algo(struct ivp_request *req, struct ivp_device *ivp_device)
{
	int ret = 0;
	u32 core = req->core;
	void __iomem *ivpctrl_base;
	struct apmcu_unload_cmd unload_cmd;
	struct apmcu_unload_cmd_rtn unload_cmd_rtn;
	struct ivp_user *ivp_user = req->user;
	struct ivp_core *ivp_core = ivp_device->ivp_core[core];
	struct ivp_algo_context *algo_ctx;
	ktime_t timestamp = ktime_get();

	algo_ctx = idr_find(&ivp_user->algo_idr[core], req->drv_algo_hnd);
	ivpctrl_base = ivp_core->ivpctrl_base;

	if (!algo_ctx) {
		dev_err(ivp_core->dev, "%s: no algo to unload\n", __func__);
		req->cmd_result = IVP_KERNEL_UINVALID;
		return ret;
	}

	if (algo_ctx->state != IVP_ALGO_STATE_READY)
		return ret;

	unload_cmd.apmcu_cmd = CMD_UNLOAD_PROC;
	unload_cmd.algo_hnd = algo_ctx->fw_algo_hnd;
	unload_cmd.timestamp = timestamp.tv64;

	lock_command(ivp_core);
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
	prev_pc[ivp_core->core] = (IVP_UINT32)ivp_read_reg32(
		ivp_core->ivpctrl_base, IVP_CTRL_REG_DEBUG_INFO05);
	prev_fw_state[ivp_core->core] = (enum ivp_state)ivp_read_reg32(
		ivp_core->ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO00);
#endif
	if (!ivp_in_wait_mode(ivpctrl_base))
		pr_warn("%s: fw is not in wait mode [%u]\n", __func__, core);
	ivp_fw_unload_algo(&unload_cmd, ivpctrl_base);
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
	int_count[ivp_core->core]++;
#endif

	ret = ivp_check_cmd_send(ivp_core, timestamp, req->duration);
	if (ret) {
		pr_err("%s: fail to send command\n", __func__);
		req->cmd_result = IVP_KERNEL_UTIMEOUT;
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
		ivp_debug_log(ivp_core);
#endif
		goto out;
	}

	ret = wait_command(ivp_core, req->duration);
	if (ret) {
		pr_err("%s: timeout to unload algo\n", __func__);
		req->cmd_result = IVP_KERNEL_UTIMEOUT;
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
		ivp_debug_log(ivp_core);
#endif
		goto out;
	}
	ivp_fw_unload_algo_rtn(&unload_cmd_rtn, ivpctrl_base);
	req->cmd_result = unload_cmd_rtn.cmd_result;
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
	if (req->cmd_result != 0) {
		pr_err("%s: cmd_result = 0x%08x [%u]\n", __func__,
		       req->cmd_result, core);
		ivp_debug_log(ivp_core);
	}
#endif
out:
	unlock_command(ivp_core);
	if (algo_ctx->algo_buf_hnd) {
		struct ivp_free_buf free_buf;

		free_buf.handle = algo_ctx->algo_buf_hnd;
		ivp_manage_free(ivp_device->ivp_mng, &free_buf);
	}
	if (req->cmd_result == IVP_EXCEPTION_FAULT) {
		ivp_get_error_report(ivp_core, req);
		ivp_excp_handler (ivp_core, req);
	}
	algo_ctx->state = IVP_ALGO_STATE_UNSET;
	return ret;
}

int ivp_free_algo(struct ivp_user *user, struct ivp_algo_context *algo_ctx)
{
	int ret = 0;
	u32 core = algo_ctx->core;
	void __iomem *ivpctrl_base;
	struct apmcu_unload_cmd unload_cmd;
	struct apmcu_unload_cmd_rtn unload_cmd_rtn = {0};
	struct ivp_device *ivp_device = dev_get_drvdata(user->dev);
	struct ivp_core *ivp_core = ivp_device->ivp_core[core];
	ktime_t timestamp = ktime_get();
	unsigned int loop_cnt = 0;

	if (ivp_core)
		ivpctrl_base = ivp_core->ivpctrl_base;
	else
		return -EINVAL;

	if (algo_ctx->state != IVP_ALGO_STATE_READY)
		return ret;

	unload_cmd.apmcu_cmd = CMD_UNLOAD_PROC;
	unload_cmd.algo_hnd = algo_ctx->fw_algo_hnd;
	unload_cmd.timestamp = timestamp.tv64;

	lock_command(ivp_core);
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
	prev_pc[ivp_core->core] = (IVP_UINT32)ivp_read_reg32(
		ivp_core->ivpctrl_base, IVP_CTRL_REG_DEBUG_INFO05);
	prev_fw_state[ivp_core->core] = (enum ivp_state)ivp_read_reg32(
		ivp_core->ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO00);
#endif
	ivp_fw_unload_algo(&unload_cmd, ivpctrl_base);
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
	int_count[ivp_core->core]++;
#endif

	ret = ivp_check_cmd_send(ivp_core, timestamp, ivp_device->timeout);
	if (ret) {
		pr_err("%s: fail to send command\n", __func__);
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
		ivp_debug_log(ivp_core);
#endif
		goto out;
	}

	ivp_fw_unload_algo_rtn(&unload_cmd_rtn, ivpctrl_base);
	while (unload_cmd_rtn.fw_state != STATE_WAIT_ISR) {
		loop_cnt++;
		if (loop_cnt > 0x1000000) {
			pr_err("%s: timeout to unload algo\n", __func__);
			ret = -ETIMEDOUT;
			goto out;
		}
		ivp_fw_unload_algo_rtn(&unload_cmd_rtn, ivpctrl_base);
	}

out:
	unlock_command(ivp_core);
	if (unload_cmd_rtn.cmd_result == IVP_EXCEPTION_FAULT) {
		ivp_reset_hw(ivp_core);
		ivp_core->state = IVP_IDLE;
	}
	return ret;
}

int ivp_get_fwinfo(struct ivp_core *ivp_core)
{
	int ret;
	void __iomem *ivpctrl_base = ivp_core->ivpctrl_base;
	struct apmcu_getinfo_cmd getinfo_cmd;
	struct apmcu_getinfo_cmd_rtn getinfo_cmd_rtn;
	ktime_t timestamp = ktime_get();

	getinfo_cmd.apmcu_cmd = CMD_GET_IVPINFO;
	getinfo_cmd.flags = GET_IVP_FW_VERSION;
	getinfo_cmd.timestamp = timestamp.tv64;

	lock_command(ivp_core);
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
	prev_pc[ivp_core->core] = (IVP_UINT32)ivp_read_reg32(
		ivp_core->ivpctrl_base, IVP_CTRL_REG_DEBUG_INFO05);
	prev_fw_state[ivp_core->core] = (enum ivp_state)ivp_read_reg32(
		ivp_core->ivpctrl_base, IVP_CTRL_REG_XTENSA_INFO00);
#endif
	ivp_fw_getinfo(&getinfo_cmd, ivpctrl_base);
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
	int_count[ivp_core->core]++;
#endif

	ret = ivp_check_cmd_send(ivp_core, timestamp,
				 ivp_core->ivp_device->timeout);
	if (ret) {
		pr_err("%s: fail to send command\n", __func__);
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
		ivp_debug_log(ivp_core);
#endif
		goto out;
	}

	ret = wait_command(ivp_core, ivp_core->ivp_device->timeout);
	if (ret) {
		pr_err("%s: timeout to get fw info\n", __func__);
#if defined(CONFIG_MTK_IVP_DEBUG_LOG)
		ivp_debug_log(ivp_core);
#endif
		goto out;
	}
	ivp_fw_getinfo_rtn(&getinfo_cmd_rtn, ivpctrl_base);

	ivp_core->ver_major =
	    (getinfo_cmd_rtn.fw_info & IVP_FW_MAJOR_VERSION_MASK) >> 16;
	ivp_core->ver_minor =
	    (getinfo_cmd_rtn.fw_info & IVP_FW_MINOR_VERSION_MASK);

out:
	unlock_command(ivp_core);
	return ret;
}

int ivp_set_dbg_ctl(struct ivp_set_dbg_info *ivp_set_dbg_info,
		    struct ivp_core *ivp_core)
{
	struct dbg_set_ctl_cmd set_dbg_ctl;
	struct ivp_dbg_info *dbg_info;
	int dbg_rtn = 0;
	ktime_t timestamp;

	set_dbg_ctl.debug_cmd = IVP_DBG_SET_CONTROL;

	dbg_info = (struct ivp_dbg_info *)ivp_core->working_buffer->dbg_cmd;
	if (!dbg_info) {
		pr_err("invalid virtual memory of debug info\n");
		return -EINVAL;
	}

	set_dbg_ctl.dbg_info_base = ivp_core->working_buffer_iova +
		offsetof(struct ivp_working_buf, dbg_cmd);
	set_dbg_ctl.set_flags = 0;
	if (ivp_set_dbg_info->level != 0) {
		set_dbg_ctl.set_flags |= FLAG_SET_LEVEL;
		dbg_info->level = ivp_set_dbg_info->level;
	}
	if (ivp_set_dbg_info->control != 0) {
		set_dbg_ctl.set_flags |= FLAG_SET_CONTAOL;
		dbg_info->control = ivp_set_dbg_info->control;
	}
	if (ivp_core->state != IVP_UNPREPARE) {
		mutex_lock(&ivp_core->dbg_mutex);
		timestamp = ktime_get();
		ivp_fw_set_dbg_ctl(&set_dbg_ctl, ivp_core->ivpctrl_base);
		while (dbg_rtn != IVP_DBG_CMD_DONE) {
			if (ktime_ms_delta(ktime_get(), timestamp) >
			    ivp_core->ivp_device->timeout) {
				dev_err(ivp_core->dev, "timeout to set dbg_ctl\n");
				break;
			}
			ivp_fw_dbg_rtn(&dbg_rtn, ivp_core->ivpctrl_base);
		}
		mutex_unlock(&ivp_core->dbg_mutex);
	}

	return 0;
}

int ivp_get_dbg_msg(struct ivp_get_dbg_info *ivp_get_dbg_info,
		    struct ivp_core *ivp_core)
{
	struct dbg_get_msg_cmd get_dbg_msg;
	struct ivp_dbg_info *dbg_info;
	int dbg_rtn = 0;
	ktime_t timestamp;

	dbg_info = (struct ivp_dbg_info *)ivp_core->working_buffer->dbg_cmd;
	if (!dbg_info) {
		pr_err("invalid virtual memory of debug info\n");
		return -EINVAL;
	}

	get_dbg_msg.debug_cmd = IVP_DBG_GET_ERROR_MSG;
	get_dbg_msg.get_flags = FLAG_GET_MSG_BUF_PTR |
				FLAG_GET_LOG_BUF_WRITE_PTR;
	get_dbg_msg.dbg_info_base = ivp_core->working_buffer_iova +
		offsetof(struct ivp_working_buf, dbg_cmd);
	mutex_lock(&ivp_core->dbg_mutex);
	timestamp = ktime_get();
	ivp_fw_get_dbg_msg(&get_dbg_msg, ivp_core->ivpctrl_base);
	while (dbg_rtn != IVP_DBG_CMD_DONE) {
		if (ktime_ms_delta(ktime_get(), timestamp) >
		    ivp_core->ivp_device->timeout) {
			dev_err(ivp_core->dev, "timeout to get dbg_msg\n");
			break;
		}
		ivp_fw_dbg_rtn(&dbg_rtn, ivp_core->ivpctrl_base);
	}
	mutex_unlock(&ivp_core->dbg_mutex);

	ivp_get_dbg_info->msg_buf_ptr = dbg_info->msg_buf_ptr;
	ivp_get_dbg_info->log_buffer_wptr = dbg_info->log_buffer_wptr;

	return 0;
}

int ivp_set_ptrace_level(u32 level, struct ivp_core *ivp_core)
{
	struct dbg_set_ctl_cmd set_ptrace_ctl;
	struct ivp_dbg_info *dbg_info;
	int dbg_rtn = 0;
	ktime_t timestamp = ktime_get();

	set_ptrace_ctl.debug_cmd = IVP_PTRACE_SET_LEVEL;

	dbg_info = (struct ivp_dbg_info *)ivp_core->working_buffer->dbg_cmd;
	if (!dbg_info) {
		pr_err("invalid virtual memory of debug info\n");
		return -EINVAL;
	}

	set_ptrace_ctl.dbg_info_base = ivp_core->working_buffer_iova +
		offsetof(struct ivp_working_buf, dbg_cmd);
	set_ptrace_ctl.set_flags = 0;
	if (level)
		dbg_info->ptrace_level = level;
	if (ivp_core->state != IVP_UNPREPARE) {
		mutex_lock(&ivp_core->dbg_mutex);
		timestamp = ktime_get();
		ivp_fw_set_dbg_ctl(&set_ptrace_ctl, ivp_core->ivpctrl_base);
		while (dbg_rtn != IVP_DBG_CMD_DONE) {
			if (ktime_ms_delta(ktime_get(), timestamp) >
			    ivp_core->ivp_device->timeout) {
				dev_err(ivp_core->dev,
					"timeout to set ptrace_level\n");
				break;
			}
			ivp_fw_dbg_rtn(&dbg_rtn, ivp_core->ivpctrl_base);
		}
		mutex_unlock(&ivp_core->dbg_mutex);
	}

	return 0;
}

int ivp_get_ptrace_msg(u32 *log_buffer_wptr, struct ivp_core *ivp_core)
{
	struct dbg_get_msg_cmd get_ptrace_msg;
	struct ivp_dbg_info *dbg_info;
	int dbg_rtn = 0;
	ktime_t timestamp;

	dbg_info = (struct ivp_dbg_info *)ivp_core->working_buffer->dbg_cmd;
	if (!dbg_info) {
		pr_err("invalid virtual memory of debug info\n");
		return -EINVAL;
	}

	get_ptrace_msg.debug_cmd = IVP_PTRACE_GET_MSG;
	get_ptrace_msg.get_flags = 0;
	get_ptrace_msg.dbg_info_base = ivp_core->working_buffer_iova +
		offsetof(struct ivp_working_buf, dbg_cmd);
	mutex_lock(&ivp_core->dbg_mutex);
	timestamp = ktime_get();
	ivp_fw_get_dbg_msg(&get_ptrace_msg, ivp_core->ivpctrl_base);
	while (dbg_rtn != IVP_DBG_CMD_DONE) {
		if (ktime_ms_delta(ktime_get(), timestamp) >
		    ivp_core->ivp_device->timeout) {
			dev_err(ivp_core->dev, "timeout to get ptrace_msg\n");
			break;
		}
		ivp_fw_dbg_rtn(&dbg_rtn, ivp_core->ivpctrl_base);
	}
	mutex_unlock(&ivp_core->dbg_mutex);

	*log_buffer_wptr = dbg_info->ptrace_log_buffer_wptr;

	return 0;
}

int ivp_reset_hw(struct ivp_core *ivp_core)
{
	void __iomem *ivpctrl_base = ivp_core->ivpctrl_base;

	ivp_halt_fw(ivpctrl_base);

	ivp_core_reset(ivp_core);
	ivp_hw_boot_sequence(ivp_core);

	if (ivp_check_fw_boot_status(ivp_core)) {
		pr_err("%s, ivp firmware boot fail.\n", __func__);
		return -1;
	}

	return 0;
}

