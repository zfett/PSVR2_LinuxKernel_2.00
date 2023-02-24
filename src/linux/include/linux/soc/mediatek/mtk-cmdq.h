/*
 * Copyright (c) 2015 MediaTek Inc.
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
 * @file mtk-cmdq.h
 * Header of mtk-cmdq-helper.c
 */

#ifndef __MTK_CMDQ_H__
#define __MTK_CMDQ_H__

#include <linux/mailbox_client.h>
#include <linux/platform_device.h>

/** @ingroup IP_group_gce_external_def
 * @brief gce instruction size definition.
 * @{
 */
#define CMDQ_INST_SIZE		8
/**
 * @}
 */

/** @ingroup IP_group_gce_external_def
 * @brief SPRs are thread-independent and hardware secure protect.\n
 * The value stored in SPR will be cleared after reset gce hardware thread.
 * @{
 */
#define GCE_SPR0		0
#define GCE_SPR1		1
#define GCE_SPR2		2
#define GCE_SPR3		3
/**
 * @}
 */

/** @ingroup IP_group_gce_external_def
 * @brief GPRs are shared by all of the 32 gce hardware threads.\n
 * Please manage the usage of GPRs by yourself.
 * @{
 */
#define GCE_GPRR2		34
#define GCE_GPRR3		35
#define GCE_GPRR4		36
#define GCE_GPRR5		37
#define GCE_GPRR6		38
#define GCE_GPRR7		39
#define GCE_GPRR8		40
#define GCE_GPRR9		41
#define GCE_GPRR10		42
#define GCE_GPRR11		43
#define GCE_GPRR12		44
#define GCE_GPRR13		45
#define GCE_GPRR14		46
#define GCE_GPRR15		47
/**
 * @}
 */

/** @ingroup IP_group_gce_external_def
 * @brief CPRs are shared by all of the 32 gce hardware threads.\n
 * Please manage the usage of CPRs by yourself.
 * @{
 */
#define GCE_CPR_NR		736
#define GCE_CPR_BASE		0x8000
#define GCE_CPR(idx)		(GCE_CPR_BASE + (idx))
#define GCE_CPR_MAX		GCE_CPR(GCE_CPR_NR)
/**
 * @}
 */

/** @ingroup IP_group_gce_external_struct
 * @brief cmdq callback data.
 */
struct cmdq_cb_data {
	/** error information */
	bool	err;
	/** callback data */
	void	*data;
};

typedef void (*cmdq_async_flush_cb)(struct cmdq_cb_data data);
struct cmdq_pkt;
struct cmdq_thread;

/** @ingroup IP_group_gce_external_struct
 * @brief cmdq client information.
 */
struct cmdq_client {
	/** user of a mailbox */
	struct mbox_client client;
	/** s/w representation of a communication chan */
	struct mbox_chan *chan;
};

/** @ingroup IP_group_gce_external_struct
 * @brief cmdq operand.
 */
struct cmdq_operand {
	/** register type */
	bool reg;
	union {
		/** index */
		u16 idx;
		/** value */
		u16 value;
	};
};

/** @ingroup IP_group_gce_external_enum
 * @brief gce operation code enum.
 */
enum cmdq_code {
	CMDQ_CODE_MASK = 0x02,
	CMDQ_CODE_POLL = 0x08,
	CMDQ_CODE_JUMP = 0x10,
	CMDQ_CODE_WFE = 0x20,
	CMDQ_CODE_READ_S = 0x80,
	CMDQ_CODE_WRITE_S = 0x90,
	CMDQ_CODE_WRITE_S_W_MASK = 0x91,
	CMDQ_CODE_LOGIC = 0xa0,
	CMDQ_CODE_JUMP_C_RELATIVE = 0xb1,
	CMDQ_CODE_EOC = 0x40,
};

/** @ingroup IP_group_gce_external_enum
 * @brief gce logic enum.
 */
enum CMDQ_LOGIC_ENUM {
	CMDQ_LOGIC_ASSIGN = 0,
	CMDQ_LOGIC_ADD = 1,
	CMDQ_LOGIC_SUBTRACT = 2,
	CMDQ_LOGIC_MULTIPLY = 3,
	CMDQ_LOGIC_XOR = 8,
	CMDQ_LOGIC_NOT = 9,
	CMDQ_LOGIC_OR = 10,
	CMDQ_LOGIC_AND = 11,
	CMDQ_LOGIC_LEFT_SHIFT = 12,
	CMDQ_LOGIC_RIGHT_SHIFT = 13
};

/** @ingroup IP_group_gce_external_enum
 * @brief gce condition enum.
 */
enum CMDQ_CONDITION_ENUM {
	CMDQ_CONDITION_ERROR = -1,
	CMDQ_EQUAL = 0,
	CMDQ_NOT_EQUAL = 1,
	CMDQ_GREATER_THAN_AND_EQUAL = 2,
	CMDQ_LESS_THAN_AND_EQUAL = 3,
	CMDQ_GREATER_THAN = 4,
	CMDQ_LESS_THAN = 5,
	CMDQ_CONDITION_MAX,
};

struct cmdq_client *cmdq_mbox_create(struct device *dev, int index);
int cmdq_mbox_destroy(struct cmdq_client *client);
int cmdq_pkt_create(struct cmdq_pkt **pkt_ptr);
int cmdq_pkt_destroy(struct cmdq_pkt *pkt);
int cmdq_pkt_read(struct cmdq_pkt *pkt, u8 subsys,
		  u16 offset, u16 dst_reg_idx);
int cmdq_pkt_write_reg(struct cmdq_pkt *pkt, u8 subsys,
		       u16 offset, u16 src_reg_idx, u32 mask);
int cmdq_pkt_write_value(struct cmdq_pkt *pkt, u8 subsys,
			 u16 offset, u32 value, u32 mask);
int cmdq_pkt_load(struct cmdq_pkt *pkt, u16 dst_reg_idx,
		  u16 indirect_src_reg_idx);
int cmdq_pkt_store_reg(struct cmdq_pkt *pkt, u16 indirect_dst_reg_idx,
		       u16 src_reg_idx, u32 mask);
int cmdq_pkt_store_value(struct cmdq_pkt *pkt, u16 indirect_dst_reg_idx,
			 u32 value, u32 mask);
int cmdq_pkt_assign_command(struct cmdq_pkt *pkt, u16 reg_idx, s32 value);
int cmdq_pkt_logic_command(struct cmdq_pkt *pkt, enum CMDQ_LOGIC_ENUM s_op,
			   u16 result_reg_idx,
			   struct cmdq_operand *left_operand,
			   struct cmdq_operand *right_operand);
int cmdq_pkt_writel_mask(struct cmdq_pkt *pkt, u32 reg_addr, u32 val,
			u32 mask, u16 work_reg_idx_0, u16 work_reg_idx_1);
int cmdq_pkt_jump(struct cmdq_pkt *pkt, s32 addr_offset);
int cmdq_pkt_cond_jump(struct cmdq_pkt *pkt,
		       u16 offset_reg_idx,
		       struct cmdq_operand *left_operand,
		       struct cmdq_operand *right_operand,
		       enum CMDQ_CONDITION_ENUM condition_operator);
int cmdq_pkt_wfe(struct cmdq_pkt *pkt, u16 event);
int cmdq_pkt_clear_event(struct cmdq_pkt *pkt, u16 event);
int cmdq_pkt_poll(struct cmdq_pkt *pkt, u8 subsys, u16 offset,
		  u32 value, u32 mask);
int cmdq_pkt_flush(struct cmdq_client *client, struct cmdq_pkt *pkt);
int cmdq_pkt_flush_async(struct cmdq_client *client, struct cmdq_pkt *pkt,
			 cmdq_async_flush_cb cb, void *data);
struct cmdq_client **cmdq_mbox_multiple_create(struct device *dev,
					       int base, int length);
int cmdq_mbox_multiple_destroy(struct cmdq_client **client, int num);
struct cmdq_pkt **cmdq_pkt_multiple_create(int num);
int cmdq_pkt_multiple_destroy(struct cmdq_pkt **pkt, int num);
int cmdq_pkt_multiple_flush_async(struct cmdq_client **client,
				  struct cmdq_pkt **pkt,
				  cmdq_async_flush_cb final_cb,
				  void *data, int num, u16 thread_token);
int cmdq_pkt_multiple_flush(struct cmdq_client **client,
			    struct cmdq_pkt **pkt, int num, u16 thread_token);
int cmdq_pkt_multiple_reset(struct cmdq_pkt **pkt, int num);
int cmdq_pkt_multiple_wfe(struct cmdq_pkt **pkt, u16 event, int num);
int cmdq_pkt_multiple_clear_event(struct cmdq_pkt **pkt, u16 event, int num);
int cmdq_pkt_presync_gce(struct cmdq_pkt **cmdq_handle, int num,
			 u16 sync_event, u16 thread_token);
int cmdq_pkt_set_token(struct cmdq_pkt *pkt, u16 token, u16 gce_subsys);
int cmdq_pkt_clear_token(struct cmdq_pkt *pkt, u16 token, u16 gce_subsys);
struct cmdq_operand *cmdq_operand_immediate(struct cmdq_operand *operand,
					    u16 value);
struct cmdq_operand *cmdq_operand_reg(struct cmdq_operand *operand, u16 idx);
int cmdq_get_cmd_buf_size(struct cmdq_pkt *pkt);
int cmdq_buf_dump(struct cmdq_thread *thread, bool timeout, u32 curr_pa);
int cmdq_dump_pkt(const struct device *dev, struct cmdq_pkt *pkt);

#endif	/* __MTK_CMDQ_H__ */
