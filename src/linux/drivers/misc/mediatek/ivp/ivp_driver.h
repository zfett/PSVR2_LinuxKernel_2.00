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
#ifndef __IVP_DRIVER_H__
#define __IVP_DRIVER_H__

#include <linux/cdev.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/hrtimer.h>

#include <uapi/mediatek/ivp_ioctl.h>

#define IVP_CORE_NUM 3
#define IVP_CORE_NAME_SIZE 20
#define MAX_PMU_COUNTER 8

struct ivpbuf_dma_chan;
struct ivp_device;

enum ivp_iomem_type {
	IVP_IOMEM_CTRL,
	IVP_IOMEM_MAX,
};

#define IVP_INTERNAL_CMD_MASK	0x49565000	/* IVPx */
#define IVP_INTERNAL_FW_SLEEP	(IVP_INTERNAL_CMD_MASK + 0x1)
#define IVP_INTERNAL_FW_WAKEUP	(IVP_INTERNAL_CMD_MASK + 0x2)

#define IVP_FW_BOOT_CMD_INFO_SIZE		0x1000
#define IVP_FW_DBG_CMD_INFO_SIZE		0x1000
#define IVP_FW_DBG_MSG_BUFFER_SIZE		0x5000
#define IVP_FW_PTRACE_MSG_BUFFER_SIZE		0x5000
#define IVP_FW_EXCEPTION_DUMP_BUFFER_SIZE	0x1000

enum ivp_algo_state {
	IVP_ALGO_STATE_INIT	= 0,
	IVP_ALGO_STATE_READY	= 1,
	IVP_ALGO_STATE_RESET	= 2,
	IVP_ALGO_STATE_UNSET	= 3,
	IVP_ALGO_STATE_MAX,
};

enum ivp_core_state {
	IVP_UNPREPARE,
	IVP_IDLE,
	IVP_ACTIVE,
	IVP_SLEEP,
};

enum ivp_clk_type {
	IVP_DSP_SEL,
	IVP_IPU_IF_SEL,
	IVP_CLK_NUM,
};

enum ivp_core_exp_cmd {
	IVP_RST_NEXT_CMD,
	IVP_RST_RETRY,
	IVP_RST_ONLY,
	IVP_CONTINUE,
	IVP_EXP_CMD_NUM,
};

enum ivp_kservice_state {
	IVP_KSERVICE_STOP = 0,
	IVP_KSERVICE_INIT = 1,
	IVP_KSERVICE_BUSY = 2,
	IVP_KSERVICE_BUSY_WITH_GCE = 3,
};

struct ivp_working_buf {
	char dbg_cmd[IVP_FW_DBG_CMD_INFO_SIZE];
	char boot_cmd[IVP_FW_BOOT_CMD_INFO_SIZE];
	char dbg_log[IVP_FW_DBG_MSG_BUFFER_SIZE];
	char ptrace_log[IVP_FW_PTRACE_MSG_BUFFER_SIZE];
	char excp_dump[IVP_FW_EXCEPTION_DUMP_BUFFER_SIZE];
};

struct gce_trg {
	u32 reg_base;
	int subsys;
};

struct ivp_pll_data {
	const char *name;
	struct clk *clk;
	unsigned long clk_rate;
};

struct ivp_core {
	struct device *dev;
	u32 core;
	struct ivp_device *ivp_device;

	void __iomem *ivpctrl_base;

	struct resource *ivpdebug_mem;
	void __iomem	*ivpdebug_base;

	struct clk **clk_cg;
	struct clk *clk_sel;
	unsigned long current_freq;

	/* ivp fw base address */
	u32 ivp_entry;
	/* ivp working buffer info */
	struct ivp_working_buf *working_buffer;
	u32 working_buffer_iova;

	u32 ptrace_log_start;

	s32 irq;

	/* ivp fw version */
	u32 ver_major;
	u32 ver_minor;
	/* notify enque thread */
	wait_queue_head_t req_wait;
	/* ivp irq operation info */
	struct mutex cmd_mutex;
	struct mutex dbg_mutex;
	wait_queue_head_t cmd_wait;
	bool is_cmd_done;

	/* ivp utilization */
	ktime_t working_start;
	enum ivp_core_state state;
	unsigned long acc_busy;
	unsigned long prev_busy;

#ifdef CONFIG_MTK_IVP_DVFS
	struct ivp_devfreq *ivp_devfreq;
#endif /* CONFIG_MTK_IVP_DVFS */

	/* ivp core power control */
	struct mutex power_mutex;
	bool is_suspended;

	/* exception */
	struct ivp_err_inform *ivp_err;
	enum ivp_core_exp_cmd exp_cmd;

	struct task_struct *enque_task;
	u8 core_name[IVP_CORE_NAME_SIZE];

	bool prepared;

	/* ivp kernel service */
	enum ivp_kservice_state kservice_state;
	struct list_head kservice_request_list;
	struct mutex kservice_mutex;
	struct gce_trg trg;

	int enable_profiler;
	int pmu_counter[MAX_PMU_COUNTER];
};

struct ivp_register_dump {
	unsigned int sel_base;
	unsigned int regoff;
	unsigned int dumpsize;
	void __iomem *ivp_base;
};

struct ivp_ram_dump {
	unsigned int coreid;
	unsigned int ram_idx;
	unsigned int size;
	void __iomem *ivp_ram;
};

struct ivp_profiler {
	int enabled;

	int enable_timer;
	struct mutex mutex;
	struct hrtimer timer;
	atomic_t refcount;
};

struct ivp_device {
	struct device *dev;
	void __iomem *vcore_base;
	void __iomem *conn_base;

	struct clk **clk_cg;
	struct clk *clk_sel[IVP_CLK_NUM];
	struct ivp_pll_data *pll_data[IVP_CLK_NUM];
	int clk_num[IVP_CLK_NUM];
	const char *clk_name[IVP_CLK_NUM];
	int *clk_ref;
	struct mutex clk_ref_mutex;
	unsigned long current_freq[IVP_CLK_NUM];

	int opp_num;
	struct ivp_opp_clk_table *opp_clk_table;

#if defined(CONFIG_REGULATOR)
	struct regulator *regulator;
	struct regulator *sram_regulator;
#endif
	unsigned long current_voltage;
	unsigned long current_sram_voltage;

	const char *board_name;

	unsigned int ivp_core_num;
	struct ivp_core *ivp_core[IVP_CORE_NUM];

	struct mutex init_mutex;
	bool ivp_init_done;

	/* to check user list must have mutex protection */
	struct mutex user_mutex;
	struct list_head user_list;
	int ivp_num_users;

	/* mutex protection when power setting*/
	struct mutex power_mutex;

	struct ivp_manage *ivp_mng;

	struct ivpbuf_dma_chan *ivp_chan;
	dev_t ivp_devt;
	struct cdev ivp_chardev;

	struct ivp_register_dump register_dump;
	struct ivp_ram_dump ram_dump;

	struct ivp_util *ivp_util;
	bool set_autosuspend;

	/* timeout setting */
	bool timeout2reset;
	u32 timeout;

	/* ivp profiler */
	struct ivp_profiler profiler;
	int trace_func;

};

struct ivp_user {
	pid_t open_pid;
	pid_t open_tgid;
	u32 id;
	struct device *dev;
	/* to enque/deque must have mutex protection */
	struct mutex data_mutex[IVP_CORE_NUM];
	bool running[IVP_CORE_NUM];
	bool flush;
	bool locked;
	u32 power_mode;
	/* list of vlist_type(struct ivp_request) */
	struct list_head enque_list[IVP_CORE_NUM];
	struct list_head deque_list[IVP_CORE_NUM];
	wait_queue_head_t deque_wait[IVP_CORE_NUM];
	wait_queue_head_t poll_wait_queue;

	/* to check buffer list must have mutex protection */
	struct mutex dbgbuf_mutex;
	struct list_head dbgbuf_list;

	/* to check algo list must have mutex protection */
	struct mutex algo_mutex[IVP_CORE_NUM];

	/* ivp idr */
	struct idr algo_idr[IVP_CORE_NUM];
	struct idr req_idr[IVP_CORE_NUM];
};

struct ivp_dbg_buf {
	s32 handle;
};

struct ivp_request {
	s32 handle;
	u32 cmd;
	u32 core;
	s32 drv_algo_hnd;
	u32 argv;
	u32 addr_handle;
	u32 size;
	u32 duration;
	s32 cmd_result;
	u32 cmd_status;
	u32 submit_cyc;
	u32 execute_start_cyc;
	u32 execute_end_cyc;
	s64 submit_nsec;
	s64 execute_start_nsec;
	s64 execute_end_nsec;
#if defined(MTK_IVP_LOAD_FROM_KERNEL)
	char algo_name[MAX_ALGO_NAME_SIZE];
#endif
	struct ivp_user *user;
};

struct ivp_algo_context {
	s32 drv_algo_hnd;
	u32 core;
	u32 fw_algo_hnd;
	u32 algo_size;
	u32 algo_flags;
	u32 algo_iova;
	u32 algo_code_mem;
	u32 algo_data_mem;
	s32 algo_buf_hnd;
	u32 algo_load_duration;
	enum ivp_algo_state state;
};

struct ivp_imagefile_header {
	u32 ih_entry;	/* entry pointer                */
	u32 ih_phoff;	/* program offset               */
	u32 ih_ehsize;	/* image header size            */
	u32 ih_phensize;	/* program size                 */
	u32 ih_phnum;	/* program code number          */
};

struct ivp_image_program {
	u32 ip_offset;	/* offset                       */
	u32 ip_paddr;	/* destination                  */
	u32 ip_filesz;	/* file size byte to copy       */
	u32 ip_memsz;	/* mem. size byte to occupied   */
};

struct ivp_mem_block {
	u32 iaddr;
	u32 size;
};

struct ivp_mem_block2 {
	u32 iaddr1;
	u32 size1;
	u32 iaddr2;
	u32 size2;
};

struct ivp_mem_layout {
	struct ivp_mem_block algo;
	struct ivp_mem_block sbuf;
	struct ivp_mem_block2 fw;
};

struct ivp_set_dbg_info {
	u32 control;
	u32 level;
};

struct ivp_get_dbg_info {
	u32 msg_buf_ptr;
	u32 log_buffer_wptr;
};

long ivp_ioctl(struct file *flip, unsigned int cmd, unsigned long arg);
int ivp_initialize(struct ivp_device *ivp_device);

#endif /* __IVP_DRIVER_H__ */
