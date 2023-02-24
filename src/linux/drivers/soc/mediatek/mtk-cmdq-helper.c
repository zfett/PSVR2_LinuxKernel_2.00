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
  * @file mtk-cmdq-helper.c
  * This helper library has two parts. The first part has support to\n
  * assemble gce instructions into the command buffer. The second part\n
  * implements basic controlling interface, client can create\n
  * command buffer and mailbox client by these functions.
  */

#include <linux/completion.h>
#include <linux/errno.h>
#include <linux/mailbox/mtk-cmdq-mailbox.h>
#include <linux/of_address.h>

/** @ingroup IP_group_gce_internal_def
 * @{
 */
/** get the argument b from 32bits value */
#define CMDQ_GET_ARG_B(arg)			(((arg) & GENMASK(31, 16)) \
						>> 16)
/** get the argument c from 32bits value */
#define CMDQ_GET_ARG_C(arg)			((arg) & GENMASK(15, 0))
/** conbine the argument b and c to a 32bits balue */
#define CMDQ_GET_32B_VALUE(arg_b, arg_c)	((u32)((arg_b) << 16) | (arg_c))
/** get the register index prefix from type */
#define CMDQ_REG_IDX_PREFIX(type)		((type) ? "" : "Reg Index ")
/** get operand index or value */
#define CMDQ_OPERAND_GET_IDX_VALUE(operand)	((operand)->reg ? \
						(operand)->idx : \
						(operand)->value)
#define CMDQ_IMMEDIATE_VALUE			0
#define CMDQ_REG_TYPE				1
#define CMDQ_SYNC_TOKEN_UPDATE			0x68
#define CMDQ_SUBSYS_SHIFT			16
#define CMDQ_OP_CODE_SHIFT			24
#define CMDQ_EOC_IRQ_EN				BIT(0)
#define CMDQ_EOC_CMD				((u64)((CMDQ_CODE_EOC \
						<< CMDQ_OP_CODE_SHIFT)) << 32 \
						| CMDQ_EOC_IRQ_EN)
#define CMDQ_WFE_UPDATE				BIT(31)
#define CMDQ_WFE_WAIT				BIT(15)
#define CMDQ_WFE_WAIT_VALUE			0x1
#define CMDQ_WFE_OPTION				(CMDQ_WFE_UPDATE | \
						CMDQ_WFE_WAIT | \
						CMDQ_WFE_WAIT_VALUE)
/**
 * @}
 */

/** @ingroup IP_group_gce_internal_struct
 * @brief cmdq 64bits instruction structure.
 */
struct cmdq_instruction {
	s16 arg_c:16;
	s16 arg_b:16;
	s16 arg_a:16;
	u8 s_op:5;
	u8 arg_c_type:1;
	u8 arg_b_type:1;
	u8 arg_a_type:1;
	u8 op:8;
};

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     Encode the gce instuction.
 * @param[out]
 *     pkt: command buffer pointer.
 * @param[in]
 *     arg_c: argument c of gce instructions.
 * @param[in]
 *     arg_b: argument b of gce instructions.
 * @param[in]
 *     arg_a: argument a of gce instructions.
 * @param[in]
 *     s_op: sub operation code of gce instructions.
 * @param[in]
 *     arg_c_type: argument c type of gce instructions.
 * @param[in]
 *     arg_b_type: argument b type of gce instructions.
 * @param[in]
 *     arg_a_type: argument a type of gce instructions.
 * @param[in]
 *     op: operation code of gce instructions.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void cmdq_pkt_instr_encoder(struct cmdq_pkt *pkt, s16 arg_c, s16 arg_b,
				   s16 arg_a, u8 s_op, u8 arg_c_type,
				   u8 arg_b_type, u8 arg_a_type, u8 op)
{
	struct cmdq_instruction *cmdq_inst;

	cmdq_inst = pkt->va_base + pkt->cmd_buf_size;
	cmdq_inst->op = op;
	cmdq_inst->arg_a_type = arg_a_type;
	cmdq_inst->arg_b_type = arg_b_type;
	cmdq_inst->arg_c_type = arg_c_type;
	cmdq_inst->s_op = s_op;
	cmdq_inst->arg_a = arg_a;
	cmdq_inst->arg_b = arg_b;
	cmdq_inst->arg_c = arg_c;
	pkt->cmd_buf_size += CMDQ_INST_SIZE;
}

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     Realloc the command buffer, store the virtual address and buffer size.
 * @param[out]
 *     pkt: command buffer pointer.
 * @param[in]
 *     size: new command buffer size.
 * @return
 *     0, successfully re-allocate the command buffer.\n
 *     -ENOMEM, re-allocate the command buffer fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int cmdq_pkt_realloc_cmd_buffer(struct cmdq_pkt *pkt, size_t size)
{
	void *new_buf;

	new_buf = krealloc(pkt->va_base, size, GFP_KERNEL | __GFP_ZERO);
	if (!new_buf)
		return -ENOMEM;
	pkt->va_base = new_buf;
	pkt->buf_size = size;

	return 0;
}

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Create cmdq mailbox client and channel.
 * @param[in]
 *     dev: device of cmdq mailbox client.
 * @param[in]
 *     index: index of cmdq mailbox channel.
 * @return
 *     cmdq mailbox client pointer, successfully create mailbox client
 *     and channel.\n
 *     ERR_PTR(-EINVAL), wrong input parameters.\n
 *     ERR_PTR(-ENOMEM), allocate cmdq client fail.\n
 *     ERR_PTR from mbox_request_channel().
 * @par Boundary case and Limitation
 *     dev can not be NULL.
 * @par Error case and Error handling
 *     1. If dev is NULL, return ERR_PTR(-EINVAL).\n
 *     2. If kzalloc() fails, return ERR_PTR(-ENOMEM).\n
 *     3. If mbox_request_channel() fails, return its ERR_PTR.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
struct cmdq_client *cmdq_mbox_create(struct device *dev, int index)
{
	struct cmdq_client *client;
	long err = 0;

	if (!dev)
		return (struct cmdq_client *)ERR_PTR(-EINVAL);

	client = kzalloc(sizeof(*client), GFP_KERNEL);
	if (!client)
		return (struct cmdq_client *)ERR_PTR(-ENOMEM);

	client->client.dev = dev;
	client->client.tx_block = false;
	client->chan = mbox_request_channel(&client->client, index);

	if (IS_ERR(client->chan)) {
		dev_err(dev, "failed to request channel\n");
		err = PTR_ERR(client->chan);
		kfree(client);

		return (struct cmdq_client *)err;
	}

	return client;
}
EXPORT_SYMBOL(cmdq_mbox_create);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Destroy cmdq mailbox client and channel.
 * @param[out]
 *     client: cmdq mailbox client pointer.
 * @return
 *     0, successfully destroy the client\n
 *     -EINVAL, client is a NULL pointer.
 * @par Boundary case and Limitation
 *     client can not be invalid.
 * @par Error case and Error handling
 *     If client is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_mbox_destroy(struct cmdq_client *client)
{
	if (IS_ERR_OR_NULL(client))
		return -EINVAL;

	mbox_free_channel(client->chan);
	kfree(client);

	return 0;
}
EXPORT_SYMBOL(cmdq_mbox_destroy);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Create the cmdq mailbox client array for gce multiple control.
 * @param[in]
 *     dev: device of cmdq mailbox client.
 * @param[in]
 *     base: beginning index of cmdq mailbox channel.
 * @param[in]
 *     length: a total of cmdq mailbox channel.
 * @return
 *     cmdq mailbox client array, successfully multiple create mailbox client
 *     and channel.\n
 *     ERR_PTR from cmdq_mbox_create().
 * @par Boundary case and Limitation
 *     dev can not be NULL.
 * @par Error case and Error handling
 *     1. If dev is NULL, return ERR_PTR(-EINVAL).\n
 *     2. If base or length is invalid, return ERR_PTR(-EINVAL).\n
 *     3. If kcalloc() fails, return ERR_PTR(-ENOMEM).\n
 *     4. If cmdq_mbox_create() fails, return its ERR_PTR.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
struct cmdq_client **cmdq_mbox_multiple_create(struct device *dev,
					       int base, int length)
{
	struct cmdq_client **client;
	int i;

	if (!dev)
		return (struct cmdq_client **)ERR_PTR(-EINVAL);
	if (base < 0 || base >= CMDQ_THR_MAX_COUNT ||
	    length <= 0 || base + length > CMDQ_THR_MAX_COUNT)
		return (struct cmdq_client **)ERR_PTR(-EINVAL);

	client = kcalloc(length, sizeof(*client), GFP_KERNEL);
	if (!client)
		return (struct cmdq_client **)ERR_PTR(-ENOMEM);

	for (i = base; i < base + length; i++)
		client[i - base] = cmdq_mbox_create(dev, i);

	return client;
}
EXPORT_SYMBOL(cmdq_mbox_multiple_create);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Destroy the cmdq mailbox client array for gce multiple control.
 * @param[out]
 *     client: the cmdq mailbox client array.
 * @param[in]
 *     num: a total of cmdq mailbox channel.
 * @return
 *     -EINVAL, num is not between 1 to CMDQ_THR_MAX_COUNT.\n
 *     error code from cmdq_mbox_destroy().
 * @par Boundary case and Limitation
 *     client can not be NULL.
 * @par Error case and Error handling
 *     If client is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_mbox_multiple_destroy(struct cmdq_client **client, int num)
{
	int i, err = 0;

	if (IS_ERR_OR_NULL(client))
		return -EINVAL;
	if (num < 1 || num > CMDQ_THR_MAX_COUNT)
		return -EINVAL;
	for (i = 0; i < num; i++)
		err |= cmdq_mbox_destroy(client[i]);

	kfree(client);
	return err;
}
EXPORT_SYMBOL(cmdq_mbox_multiple_destroy);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Create a cmdq packet.
 * @param[out]
 *     pkt_ptr: cmdq packet pointer to retrieve cmdq_pkt.
 * @return
 *     cmdq packet pointer, successfully create cmdq command buffer.\n
 *     -EINVAL, pkt_ptr is invalid.\n
 *     -ENOMEM, allocate cmdq packet fail.\n
 *     error code from cmdq_pkt_realloc_cmd_buffer().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If pkt_ptr is invalid, return -EINVAL.\n
 *     2. If pkt is NULL, return -ENOMEM.\n
 *     3. If cmdq_pkt_realloc_cmd_buffer() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_create(struct cmdq_pkt **pkt_ptr)
{
	struct cmdq_pkt *pkt;
	int err;

	if (IS_ERR_OR_NULL(pkt_ptr))
		return -EINVAL;
	pkt = kzalloc(sizeof(*pkt), GFP_KERNEL);
	if (!pkt) {
		*pkt_ptr = NULL;
		return -ENOMEM;
	}
	err = cmdq_pkt_realloc_cmd_buffer(pkt, PAGE_SIZE);
	if (err < 0) {
		kfree(pkt);
		return err;
	}
	*pkt_ptr = pkt;

	return 0;
}
EXPORT_SYMBOL(cmdq_pkt_create);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Destroy a cmdq packet.
 * @param[out]
 *     pkt: the cmdq packet.
 * @return
 *     0, successfully destroy the pkt\n
 *     -EINVAL, pkt is a NULL pointer.
 * @par Boundary case and Limitation
 *     pkt can not be NULL.
 * @par Error case and Error handling
 *     If pkt is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_destroy(struct cmdq_pkt *pkt)
{
	if (IS_ERR_OR_NULL(pkt))
		return -EINVAL;

	kfree(pkt->va_base);
	kfree(pkt);

	return 0;
}
EXPORT_SYMBOL(cmdq_pkt_destroy);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Create the cmdq packet array for gce multiple control.
 * @param[in]
 *     num: a total of cmdq packet.
 * @return
 *     cmdq packet array, successfully multiple create cmdq command buffer.\n
 *     ERR_PTR(-EINVAL), num is not between 1 to CMDQ_THR_MAX_COUNT.\n
 *     ERR_PTR(-ENOMEM), allocate cmdq packet fail.\n
 *     ERR_PTR from cmdq_pkt_create().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If num is invalid, return ERR_PTR(-EINVAL).\n
 *     2. If kcalloc() fails, return ERR_PTR(-ENOMEM).\n
 *     3. If cmdq_pkt_create() fails, WARN_ON.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
struct cmdq_pkt **cmdq_pkt_multiple_create(int num)
{
	struct cmdq_pkt **pkt_ptr;
	int i;

	if (num < 1 || num > CMDQ_THR_MAX_COUNT)
		return (struct cmdq_pkt **)ERR_PTR(-EINVAL);
	pkt_ptr = kcalloc(num, sizeof(*pkt_ptr), GFP_KERNEL);
	if (!pkt_ptr)
		return (struct cmdq_pkt **)ERR_PTR(-ENOMEM);

	for (i = 0; i < num; i++)
		WARN_ON(cmdq_pkt_create(&pkt_ptr[i]));

	return pkt_ptr;
}
EXPORT_SYMBOL(cmdq_pkt_multiple_create);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Destroy the cmdq packet array for gce multiple control.
 * @param[out]
 *     pkt: the cmdq packet array.
 * @param[in]
 *     num: a total of cmdq packet.
 * @return
 *     -EINVAL, pkt is invalid.\n
 *     -EINVAL, num is not between 1 to CMDQ_THR_MAX_COUNT.\n
 *     error code from cmdq_mbox_destroy().
 * @par Boundary case and Limitation
 *     pkt can not be NULL.
 * @par Error case and Error handling
 *     If pkt is invalid, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_multiple_destroy(struct cmdq_pkt **pkt, int num)
{
	int i, err = 0;

	if (IS_ERR_OR_NULL(pkt))
		return -EINVAL;
	if (num < 1 || num > CMDQ_THR_MAX_COUNT)
		return -EINVAL;
	for (i = 0; i < num; i++)
		err |= cmdq_pkt_destroy(pkt[i]);

	kfree(pkt);
	return err;
}
EXPORT_SYMBOL(cmdq_pkt_multiple_destroy);

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     Check if the command is finalized.
 * @param[in]
 *     pkt: the cmdq packet.
 * @return
 *     true, the packet has been flushed, client cannot append the cmd.\n
 *     false, the packet has not been flushed, client can append cmd.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static bool cmdq_pkt_is_finalized(struct cmdq_pkt *pkt)
{
	u64 *expect_eoc;

	if (pkt->cmd_buf_size < CMDQ_INST_SIZE << 1)
		return false;

	expect_eoc = pkt->va_base + pkt->cmd_buf_size - (CMDQ_INST_SIZE << 1);
	if (*expect_eoc == CMDQ_EOC_CMD)
		return true;

	return false;
}

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     Append the command into packet.
 * @param[out]
 *     pkt: the cmdq packet.
 * @param[in]
 *     arg_c: argument c of gce instructions.
 * @param[in]
 *     arg_b: argument b of gce instructions.
 * @param[in]
 *     arg_a: argument a of gce instructions.
 * @param[in]
 *     s_op: sub operation code of gce instructions.
 * @param[in]
 *     arg_c_type: argument c type of gce instructions.
 * @param[in]
 *     arg_b_type: argument b type of gce instructions.
 * @param[in]
 *     arg_a_type: argument a type of gce instructions.
 * @param[in]
 *     code: operation code of gce instructions.
 * @return
 *     0, successfully append command into the cmdq command buffer.\n
 *     -EBUSY, the packet has been flushed, client cannot append the cmd.\n
 *     -EINVAL, the packet is an invalid pointer.\n
 *     error code from cmdq_pkt_realloc_cmd_buffer().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int cmdq_pkt_append_command(struct cmdq_pkt *pkt, s16 arg_c, s16 arg_b,
				   s16 arg_a, u8 s_op, u8 arg_c_type,
				   u8 arg_b_type, u8 arg_a_type,
				   enum cmdq_code code)
{
	int err;

	if (IS_ERR_OR_NULL(pkt))
		return -EINVAL;
	if (WARN_ON(cmdq_pkt_is_finalized(pkt)))
		return -EBUSY;
	if (unlikely(pkt->cmd_buf_size + CMDQ_INST_SIZE > pkt->buf_size)) {
		err = cmdq_pkt_realloc_cmd_buffer(pkt, pkt->buf_size << 1);
		if (err < 0)
			return err;
	}
	cmdq_pkt_instr_encoder(pkt, arg_c, arg_b, arg_a, s_op, arg_c_type,
			       arg_b_type, arg_a_type, code);

	return 0;
}

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Append read command to the cmdq packet,
 *     ask gce to execute an instruction that
 *     read a subsys hardware register and store
 *     the value into SPR. gce can touch the register
 *     if the register is existential and un-secure.
 * @param[out]
 *     pkt: the cmdq packet.
 * @param[in]
 *     subsys: the cmdq subsys base.
 * @param[in]
 *     offset: hardware register offset.
 * @param[in]
 *     dst_reg_idx: the destination index of SPR.
 * @return
 *     0, successfully append command into the cmdq command buffer.\n
 *     error code from cmdq_pkt_append_command().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_read(struct cmdq_pkt *pkt, u8 subsys, u16 offset, u16 dst_reg_idx)
{
	return cmdq_pkt_append_command(pkt, 0, offset, dst_reg_idx, subsys,
				       CMDQ_IMMEDIATE_VALUE,
				       CMDQ_IMMEDIATE_VALUE, CMDQ_REG_TYPE,
				       CMDQ_CODE_READ_S);
}
EXPORT_SYMBOL(cmdq_pkt_read);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Append write reg command to the cmdq packet,
 *     ask gce to execute an instruction that write a
 *     value which store in SPR into subsys hardware register.
 *     gce can touch the register if the register is existential and un-secure.
 * @param[out]
 *     pkt: the cmdq packet.
 * @param[in]
 *     subsys: the cmdq subsys base.
 * @param[in]
 *     offset: hardware register offset.
 * @param[in]
 *     src_reg_idx: the source index of SPR.
 * @param[in]
 *     mask:  the specified target register mask.
 * @return
 *     0, successfully append command into the cmdq command buffer.\n
 *     error code from cmdq_pkt_append_command().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_write_reg(struct cmdq_pkt *pkt, u8 subsys,
		       u16 offset, u16 src_reg_idx, u32 mask)
{
	int err = 0;
	enum cmdq_code op = CMDQ_CODE_WRITE_S;

	if (mask != 0xffffffff) {
		err = cmdq_pkt_append_command(pkt, CMDQ_GET_ARG_C(~mask),
					      CMDQ_GET_ARG_B(~mask), 0, 0, 0, 0,
					      0, CMDQ_CODE_MASK);
		if (err != 0)
			return err;

		op = CMDQ_CODE_WRITE_S_W_MASK;
	}

	return cmdq_pkt_append_command(pkt, 0, src_reg_idx, offset, subsys,
				       CMDQ_IMMEDIATE_VALUE, CMDQ_REG_TYPE,
				       CMDQ_IMMEDIATE_VALUE, op);
}
EXPORT_SYMBOL(cmdq_pkt_write_reg);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Append write reg command to the cmdq packet,
 *     ask gce to execute an instruction that write an
 *     immediate 32bit value into subsys hardware register.
 *     gce can touch the register if the register is existential and un-secure.
 * @param[out]
 *     pkt: the cmdq packet.
 * @param[in]
 *     subsys: the cmdq subsys base.
 * @param[in]
 *     offset: hardware register offset.
 * @param[in]
 *     value: the specified target register 32bits value.
 * @param[in]
 *     mask: the specified target register mask.
 * @return
 *     0, successfully append command into the cmdq command buffer.\n
 *     error code from cmdq_pkt_append_command().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_write_value(struct cmdq_pkt *pkt, u8 subsys,
			 u16 offset, u32 value, u32 mask)
{
	int err = 0;
	enum cmdq_code op = CMDQ_CODE_WRITE_S;

	if (mask != 0xffffffff) {
		err = cmdq_pkt_append_command(pkt, CMDQ_GET_ARG_C(~mask),
					      CMDQ_GET_ARG_B(~mask), 0, 0, 0, 0,
					      0, CMDQ_CODE_MASK);
		if (err != 0)
			return err;

		op = CMDQ_CODE_WRITE_S_W_MASK;
	}

	return cmdq_pkt_append_command(pkt, CMDQ_GET_ARG_C(value),
				       CMDQ_GET_ARG_B(value), offset, subsys,
				       CMDQ_IMMEDIATE_VALUE,
				       CMDQ_IMMEDIATE_VALUE,
				       CMDQ_IMMEDIATE_VALUE, op);
}
EXPORT_SYMBOL(cmdq_pkt_write_value);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Append read command to the cmdq packet,
 *     ask gce to execute an instruction
 *     that read a value from DRAM address stored in SPR and store
 *     the value into another SPR.
 * @param[out]
 *     pkt: the cmdq packet.
 * @param[in]
 *     dst_reg_idx: the destination index of SPR.
 * @param[in]
 *     indirect_src_reg_idx: the index of SPR containing the
 *     address of source memory.
 * @return
 *     0, successfully append command into the cmdq command buffer.\n
 *     error code from cmdq_pkt_append_command().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_load(struct cmdq_pkt *pkt, u16 dst_reg_idx,
		  u16 indirect_src_reg_idx)
{
	return cmdq_pkt_append_command(pkt, 0, indirect_src_reg_idx,
				       dst_reg_idx, 0, CMDQ_IMMEDIATE_VALUE,
				       CMDQ_REG_TYPE, CMDQ_REG_TYPE,
				       CMDQ_CODE_READ_S);
}
EXPORT_SYMBOL(cmdq_pkt_load);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Append write reg command to the cmdq packet,
 *     ask gce to execute an instruction that store
 *     a value stored in SPR and store the value into
 *     the DRAM address stored in another SPR.
 * @param[out]
 *     pkt: the cmdq packet.
 * @param[in]
 *     indirect_dst_reg_idx: the index of SPR containing
 *     the address of destination memory.
 * @param[in]
 *     src_reg_idx: the source index of SPR.
 * @param[in]
 *     mask: the specified target register mask.
 * @return
 *     0, successfully append command into the cmdq command buffer.\n
 *     error code from cmdq_pkt_append_command().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_store_reg(struct cmdq_pkt *pkt, u16 indirect_dst_reg_idx,
		       u16 src_reg_idx, u32 mask)
{
	int err = 0;
	enum cmdq_code op = CMDQ_CODE_WRITE_S;

	if (mask != 0xffffffff) {
		err = cmdq_pkt_append_command(pkt, CMDQ_GET_ARG_C(~mask),
					      CMDQ_GET_ARG_B(~mask), 0, 0, 0, 0,
					      0, CMDQ_CODE_MASK);
		if (err != 0)
			return err;

		op = CMDQ_CODE_WRITE_S_W_MASK;
	}

	return cmdq_pkt_append_command(pkt, 0, src_reg_idx,
				       indirect_dst_reg_idx, 0,
				       CMDQ_IMMEDIATE_VALUE, CMDQ_REG_TYPE,
				       CMDQ_REG_TYPE, op);
}
EXPORT_SYMBOL(cmdq_pkt_store_reg);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Append write reg command to the cmdq packet,
 *     ask gce to execute an instruction that store
 *     a value stored in SPR and store the value into
 *     the DRAM address stored in another SPR.
 * @param[out]
 *     pkt: the cmdq packet.
 * @param[in]
 *     indirect_dst_reg_idx: the index of SPR containing
 *     the address of destination memory.
 * @param[in]
 *     value: the specified target register 32bits value.
 * @param[in]
 *     mask: the specified target register mask.
 * @return
 *     0, successfully append command into the cmdq command buffer.\n
 *     error code from cmdq_pkt_append_command().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_store_value(struct cmdq_pkt *pkt, u16 indirect_dst_reg_idx,
			 u32 value, u32 mask)
{
	int err = 0;
	enum cmdq_code op = CMDQ_CODE_WRITE_S;

	if (mask != 0xffffffff) {
		err = cmdq_pkt_append_command(pkt, CMDQ_GET_ARG_C(~mask),
					      CMDQ_GET_ARG_B(~mask), 0, 0, 0, 0,
					      0, CMDQ_CODE_MASK);
		if (err != 0)
			return err;

		op = CMDQ_CODE_WRITE_S_W_MASK;
	}

	return cmdq_pkt_append_command(pkt, CMDQ_GET_ARG_C(value),
				       CMDQ_GET_ARG_B(value),
				       indirect_dst_reg_idx, 0,
				       CMDQ_IMMEDIATE_VALUE,
				       CMDQ_IMMEDIATE_VALUE, CMDQ_REG_TYPE, op);
}
EXPORT_SYMBOL(cmdq_pkt_store_value);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Append logic assign command to the cmdq packet,
 *     ask gce to execute an instruction that assign an
 *     immediate 32bit value into the SPR.
 * @param[out]
 *     pkt: the cmdq packet.
 * @param[in]
 *     reg_idx: SPR index that store the assigned value.
 * @param[in]
 *     value: the value you want to assign.
 * @return
 *     0, successfully append command into the cmdq command buffer.\n
 *     error code from cmdq_pkt_append_command().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_assign_command(struct cmdq_pkt *pkt, u16 reg_idx, s32 value)
{
	return cmdq_pkt_append_command(pkt, CMDQ_GET_ARG_C(value),
				       CMDQ_GET_ARG_B(value), reg_idx,
				       CMDQ_LOGIC_ASSIGN, CMDQ_IMMEDIATE_VALUE,
				       CMDQ_IMMEDIATE_VALUE, CMDQ_REG_TYPE,
				       CMDQ_CODE_LOGIC);
}
EXPORT_SYMBOL(cmdq_pkt_assign_command);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Append logic command to the cmdq packet, ask gce
 *     to execute a arithematic calculate instruction,
 *     such as add, subtract, multiply, AND, OR and NOT,
 *     etc. Note that logic instruction just accept unsigned
 *     calculate. If there are any overflows, gce will sent
 *     the invalid IRQ to notify cmdq driver. cmdq driver
 *     will align the naming of hardware design, even the logic
 *     command only provide arithematic calculate instruction.
 * @param[out]
 *     pkt: the cmdq packet.
 * @param[in]
 *     s_op: the logic operator enum, please refer to CMDQ_LOGIC_ENUM enum.
 * @param[in]
 *     result_reg_idx: SPR index that store operation
 *     result of left_operand and right_operand.
 * @param[in]
 *     left_operand: left operand (need call cmdq_operand_immediate() or
 *     cmdq_operand_reg() to assign the value or SPR index).
 * @param[in]
 *     right_operand: right operand (need call cmdq_operand_immediate() or
 *     cmdq_operand_reg() to assign the value or SPR index).
 * @return
 *     0, successfully append command into the cmdq command buffer.\n
 *     -EINVAL, left_operand or right_operand is NULL pointer.\n
 *     error code from cmdq_pkt_append_command().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_logic_command(struct cmdq_pkt *pkt, enum CMDQ_LOGIC_ENUM s_op,
			   u16 result_reg_idx,
			   struct cmdq_operand *left_operand,
			   struct cmdq_operand *right_operand)
{
	u32 left_idx_value;
	u32 right_idx_value;

	if (!left_operand || !right_operand)
		return -EINVAL;

	left_idx_value = CMDQ_OPERAND_GET_IDX_VALUE(left_operand);
	right_idx_value = CMDQ_OPERAND_GET_IDX_VALUE(right_operand);

	return cmdq_pkt_append_command(pkt, right_idx_value, left_idx_value,
				       result_reg_idx, s_op, right_operand->reg,
				       left_operand->reg, CMDQ_REG_TYPE,
				       CMDQ_CODE_LOGIC);
}
EXPORT_SYMBOL(cmdq_pkt_logic_command);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Add writing register value with mask commands to the cmdq packet.\n
 *     It's used for registers that do not have corresponding GCE subsys code.
 *     User should specify two different working registers of GCE.
 * @param[out]
 *     pkt: the cmdq packet.
 * @param[in]
 *     reg_addr: the register to be written.
 * @param[in]
 *     val: the value to be written.
 * @param[in]
 *     mask: it specifies whith bits of register are going to be written.
 * @param[in]
 *     work_reg_idx_0: it specifies one of the working registers of GCE.
 * @param[in]
 *     work_reg_idx_1: it specifies one of the working registers of GCE.
 * @return
 *     0, successfully append command into the cmdq command buffer.\n
 *     -EINVAL, if work_reg_idx_0 is equal to work_reg_idx_1.\n
 *     error code from cmdq_pkt_assign_command()/cmdq_pkt_store_reg().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_writel_mask(struct cmdq_pkt *pkt, u32 reg_addr, u32 val,
			u32 mask, u16 work_reg_idx_0, u16 work_reg_idx_1)
{
	int err;

	if (work_reg_idx_0 == work_reg_idx_1)
		return -EINVAL;

	/* GCE_REG[work_reg_idx_0] = value */
	err = cmdq_pkt_assign_command(pkt, work_reg_idx_0, val);
	/* GCE_REG[work_reg_idx_1] = reg_addr */
	err |= cmdq_pkt_assign_command(pkt, work_reg_idx_1, reg_addr);
	/* *GCE_REG[work_reg_idx_1] = GCE_REG[work_reg_idx_0] */
	err |= cmdq_pkt_store_reg(pkt, work_reg_idx_1, work_reg_idx_0, mask);

	return err;
}
EXPORT_SYMBOL(cmdq_pkt_writel_mask);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Append jump command to the cmdq packet, ask gce
 *     to execute an instruction that can make the gce's
 *     program counter jump an offset from current position.
 * @param[out]
 *     pkt: the cmdq packet.
 * @param[in]
 *     addr_offset: the offset address.
 * @return
 *     0, successfully append command into the cmdq command buffer.\n
 *     error code from cmdq_pkt_append_command().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_jump(struct cmdq_pkt *pkt, s32 addr_offset)
{
	return cmdq_pkt_append_command(pkt, CMDQ_GET_ARG_C(addr_offset),
				       CMDQ_GET_ARG_B(addr_offset), 0, 0, 0, 0,
				       0, CMDQ_CODE_JUMP);
}
EXPORT_SYMBOL(cmdq_pkt_jump);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Append conditional jump command to the cmdq packet,
 *     ask gce to execute an instruction that can make the gce's
 *     program counter jump an offset from current position under
 *     specified condition.
 * @param[out]
 *     pkt: the cmdq packet.
 * @param[in]
 *     offset_reg_idx: SPR index that store the offset address
 *     (the number of instruction x INST_SIZE).
 * @param[in]
 *     left_operand: left operand (need call cmdq_operand_immediate()
 *     or cmdq_operand_reg() to assign the value or SPR index).
 * @param[in]
 *     right_operand: right operand (need call cmdq_operand_immediate()
 *     or cmdq_operand_reg() to assign the value or SPR index).
 * @param[in]
 *     condition_operator: the conditional operator enum, please
 *     refer to CMDQ_CONDITION_ENUM enum.
 * @return
 *     0, successfully append command into the cmdq command buffer.\n
 *     -EINVAL, left_operand or right_operand is NULL pointer.\n
 *     -EINVAL, offset_reg_idx is GPR.\n
 *     error code from cmdq_pkt_append_command().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_cond_jump(struct cmdq_pkt *pkt,
		       u16 offset_reg_idx,
		       struct cmdq_operand *left_operand,
		       struct cmdq_operand *right_operand,
		       enum CMDQ_CONDITION_ENUM condition_operator)
{
	u32 left_idx_value;
	u32 right_idx_value;

	if (!left_operand || !right_operand)
		return -EINVAL;
	if (offset_reg_idx >= GCE_GPRR2 && offset_reg_idx <= GCE_GPRR15)
		return -EINVAL;

	left_idx_value = CMDQ_OPERAND_GET_IDX_VALUE(left_operand);
	right_idx_value = CMDQ_OPERAND_GET_IDX_VALUE(right_operand);

	return cmdq_pkt_append_command(pkt, right_idx_value, left_idx_value,
				       offset_reg_idx, condition_operator,
				       right_operand->reg, left_operand->reg,
				       CMDQ_REG_TYPE,
				       CMDQ_CODE_JUMP_C_RELATIVE);
}
EXPORT_SYMBOL(cmdq_pkt_cond_jump);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Append wait for event command to the cmdq packet,
 *     ask gce to execute an instruction that wait for a specified
 *     hardware event. A gce hardware thread which called this
 *     instruction will be blocked by this instruction; therefore,
 *     a thread which is the highest priority in other runnable
 *     threads is choice and re-started to run.
 * @param[out]
 *     pkt: the cmdq packet.
 * @param[in]
 *     event: the desired event type to "wait and CLEAR".
 * @return
 *     0, successfully append command into the cmdq command buffer.\n
 *     -EINVAL, the event number > CMDQ_MAX_EVENT.\n
 *     error code from cmdq_pkt_append_command().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_wfe(struct cmdq_pkt *pkt, u16 event)
{
	if (event > CMDQ_MAX_EVENT)
		return -EINVAL;

	return cmdq_pkt_append_command(pkt, CMDQ_GET_ARG_C(CMDQ_WFE_OPTION),
				       CMDQ_GET_ARG_B(CMDQ_WFE_OPTION), event,
				       0, 0, 0, 0, CMDQ_CODE_WFE);
}
EXPORT_SYMBOL(cmdq_pkt_wfe);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Append clear event command to the cmdq packet,
 *     ask gce to execute an instruction that clear a specified hardware event.
 * @param[out]
 *     pkt: the cmdq packet.
 * @param[in]
 *     event: the desired event to be cleared.
 * @return
 *     0, successfully append command into the cmdq command buffer.\n
 *     -EINVAL, the event number > CMDQ_MAX_EVENT.\n
 *     error code from cmdq_pkt_append_command().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_clear_event(struct cmdq_pkt *pkt, u16 event)
{
	if (event > CMDQ_MAX_EVENT)
		return -EINVAL;

	return cmdq_pkt_append_command(pkt, CMDQ_GET_ARG_C(CMDQ_WFE_UPDATE),
				       CMDQ_GET_ARG_B(CMDQ_WFE_UPDATE), event,
				       0, 0, 0, 0, CMDQ_CODE_WFE);
}
EXPORT_SYMBOL(cmdq_pkt_clear_event);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Append poll command to the cmdq packet, ask gce
 *     to execute an instruction that poll a specified hardware register.
 * @param[out]
 *     pkt: the cmdq packet.
 * @param[in]
 *     subsys: the cmdq subsys base.
 * @param[in]
 *     offset: hardware register offset.
 * @param[in]
 *     value: the specified target register 32bits value.
 * @param[in]
 *     mask: the specified target register mask.
 * @return
 *     0, successfully append command into the cmdq command buffer.\n
 *     error code from cmdq_pkt_append_command().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_poll(struct cmdq_pkt *pkt, u8 subsys, u16 offset,
		  u32 value, u32 mask)
{
	int err;

	if (mask != 0xffffffff) {
		err = cmdq_pkt_append_command(pkt, CMDQ_GET_ARG_C(~mask),
					      CMDQ_GET_ARG_B(~mask), 0, 0, 0, 0,
					      0, CMDQ_CODE_MASK);
		if (err != 0)
			return err;

		offset |= 0x1;
	}

	return cmdq_pkt_append_command(pkt, CMDQ_GET_ARG_C(value),
				       CMDQ_GET_ARG_B(value), offset,
				       subsys, CMDQ_IMMEDIATE_VALUE,
				       CMDQ_IMMEDIATE_VALUE,
				       CMDQ_IMMEDIATE_VALUE, CMDQ_CODE_POLL);
}
EXPORT_SYMBOL(cmdq_pkt_poll);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Append set software token command to the cmdq packet.
 * @param[out]
 *     pkt: the cmdq packet.
 * @param[in]
 *     token: the desired token to be set.
 * @param[in]
 *     gce_subsys: the token from the desired gce.
 * @return
 *     0, successfully append command into the cmdq command buffer.\n
 *     -EINVAL, the token number > CMDQ_MAX_EVENT.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_set_token(struct cmdq_pkt *pkt, u16 token, u16 gce_subsys)
{
	if (token > CMDQ_MAX_EVENT)
		return -EINVAL;

	return cmdq_pkt_write_value(pkt, gce_subsys, CMDQ_SYNC_TOKEN_UPDATE,
				    BIT(16) | token, 0xffffffff);
}
EXPORT_SYMBOL(cmdq_pkt_set_token);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Append clear software token command to the cmdq packet.
 * @param[out]
 *     pkt: the cmdq packet.
 * @param[in]
 *     token: the desired token to be cleared.
 * @param[in]
 *     gce_subsys: the token from the desired gce.
 * @return
 *     0, successfully append command into the cmdq command buffer.\n
 *     -EINVAL, the token number > CMDQ_MAX_EVENT.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_clear_token(struct cmdq_pkt *pkt, u16 token, u16 gce_subsys)
{
	if (token > CMDQ_MAX_EVENT)
		return -EINVAL;

	return cmdq_pkt_write_value(pkt, gce_subsys, CMDQ_SYNC_TOKEN_UPDATE,
				    token, 0xffffffff);
}
EXPORT_SYMBOL(cmdq_pkt_clear_token);

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     Append the finalize instruction into packet.
 * @param[out]
 *     pkt: the cmdq packet.
 * @return
 *     0, successfully append command into the cmdq command buffer.\n
 *     -EINVAL, the packet is a NULL pointer.\n
 *     error code from cmdq_pkt_append_command().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int cmdq_pkt_finalize(struct cmdq_pkt *pkt)
{
	int err;

	if (IS_ERR_OR_NULL(pkt))
		return -EINVAL;
	if (cmdq_pkt_is_finalized(pkt))
		return 0;

	/* insert EOC and generate IRQ for each command iteration */
	err = cmdq_pkt_append_command(pkt, CMDQ_GET_ARG_C(CMDQ_EOC_IRQ_EN),
				      CMDQ_GET_ARG_B(CMDQ_EOC_IRQ_EN), 0, 0, 0,
				      0, 0, CMDQ_CODE_EOC);
	if (err < 0)
		return err;

	return 0;
}

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Trigger gce to asynchronously execute the cmdq packet
 *     and call back at the end of done packet.
 * @param[in]
 *     client: the cmdq mailbox client.
 * @param[out]
 *     pkt: the cmdq packet.
 * @param[in]
 *     cb: called at the end of done packet.
 * @param[in]
 *     data: this data will pass back to cb.
 * @return
 *     0, successfully ask gce begin to work.\n
 *     -EINVAL, the client is an invalid pointer.\n
 *     error code from cmdq_pkt_finalize().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_flush_async(struct cmdq_client *client, struct cmdq_pkt *pkt,
			 cmdq_async_flush_cb cb, void *data)
{
	int err;

	if (IS_ERR_OR_NULL(client))
		return -EINVAL;

	err = cmdq_pkt_finalize(pkt);
	if (err < 0)
		return err;

	pkt->cb.cb = cb;
	pkt->cb.data = data;

	mbox_send_message(client->chan, pkt);

	return 0;
}
EXPORT_SYMBOL(cmdq_pkt_flush_async);

/** @ingroup IP_group_gce_internal_def
 * @brief cmdq flush completion structure.
 */
struct cmdq_flush_completion {
	/** completion */
	struct completion cmplt;
	/** error status */
	bool err;
};

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     Cmdq internal callback function.
 * @param[in]
 *     data: callback data.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void cmdq_pkt_flush_cb(struct cmdq_cb_data data)
{
	struct cmdq_flush_completion *cmplt = data.data;

	cmplt->err = data.err;
	complete(&cmplt->cmplt);
}

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Trigger gce to execute the cmdq packet.
 * @param[in]
 *     client: the cmdq mailbox client.
 * @param[out]
 *     pkt: the cmdq packet.
 * @return
 *     0, successfully ask gce begin to work.\n
 *     error code from cmdq_pkt_flush_async().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_flush(struct cmdq_client *client, struct cmdq_pkt *pkt)
{
	struct cmdq_flush_completion cmplt;
	int err;

	init_completion(&cmplt.cmplt);
	err = cmdq_pkt_flush_async(client, pkt, cmdq_pkt_flush_cb, &cmplt);
	if (err < 0)
		return err;
	wait_for_completion(&cmplt.cmplt);

	return cmplt.err ? -EFAULT : 0;
}
EXPORT_SYMBOL(cmdq_pkt_flush);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Trigger multiple cmdq to asynchronously execute the
 *     cmdq packet and call back at the end of done all the packets.
 * @param[in]
 *     client: the cmdq mailbox client array.
 * @param[out]
 *     pkt: the cmdq packet array.
 * @param[in]
 *     final_cb: called at the end of done all the packets.
 * @param[in]
 *     data: this data will pass back to cb.
 * @param[in]
 *     num: a total of cmdq packet.
 * @param[in]
 *     thread_token: the token used for sync.
 * @return
 *     0, successfully ask multiple gce begin to work.\n
 *     -EINVAL, client or pkt is invalid pointer.\n
 *     -EINVAL, num is not between 1 to CMDQ_THR_MAX_COUNT.\n
 *     error code from cmdq_pkt_flush_async().\n
 *     error code from cmdq_pkt_set_token().\n
 *     error code from cmdq_pkt_wfe().\n
 *     error code from cmdq_pkt_clear_event().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_multiple_flush_async(struct cmdq_client **client,
				  struct cmdq_pkt **pkt,
				  cmdq_async_flush_cb final_cb,
				  void *data, int num, u16 thread_token)
{
	int i, err = 0;

	if (IS_ERR_OR_NULL(client) || IS_ERR_OR_NULL(pkt))
		return -EINVAL;
	if (num < 1 || num > CMDQ_THR_MAX_COUNT)
		return -EINVAL;

	for (i = 1; i < num ; i++) {
		err |= cmdq_pkt_set_token(pkt[i], (thread_token + 5 + i), 0);
		err |= cmdq_pkt_wfe(pkt[0], (thread_token + 5 + i));
		err |= cmdq_pkt_clear_event(pkt[0], (thread_token + 5 + i));
	}
	if (err < 0)
		return err;

	err = cmdq_pkt_flush_async(client[0], pkt[0], final_cb, data);
	if (err < 0)
		return err;

	for (i = 1; i < num; i++) {
		err = cmdq_pkt_flush_async(client[i], pkt[i], NULL, data);
		if (err < 0)
			return err;
	}

	return 0;
}
EXPORT_SYMBOL(cmdq_pkt_multiple_flush_async);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Trigger multiple cmdq to execute the multiple cmdq packet.
 * @param[in]
 *     client: the cmdq mailbox client array.
 * @param[out]
 *     pkt: the cmdq packet array.
 * @param[in]
 *     num: a total of cmdq packet.
 * @param[in]
 *     thread_token: the token used for sync.
 * @return
 *     0, successfully ask multiple gce begin to work.\n
 *     -EINVAL, client or pkt is invalid pointer.\n
 *     -EINVAL, num is not between 1 to CMDQ_THR_MAX_COUNT.\n
 *     error code from cmdq_pkt_multiple_flush_async().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_multiple_flush(struct cmdq_client **client,
			    struct cmdq_pkt **pkt, int num, u16 thread_token)
{
	struct cmdq_flush_completion cmplt;
	int err;

	if (IS_ERR_OR_NULL(client) || IS_ERR_OR_NULL(pkt))
		return -EINVAL;
	if (num < 1 || num > CMDQ_THR_MAX_COUNT)
		return -EINVAL;

	init_completion(&cmplt.cmplt);
	err = cmdq_pkt_multiple_flush_async(client, pkt, cmdq_pkt_flush_cb,
					    &cmplt, num, thread_token);
	if (err < 0)
		return err;
	wait_for_completion(&cmplt.cmplt);

	return cmplt.err ? -EFAULT : 0;
}
EXPORT_SYMBOL(cmdq_pkt_multiple_flush);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Reset multiple cmdq command size to 0
 * @param[in]
 *     pkt: the cmdq packet array.
 * @param[in]
 *     num: a total of cmdq packet.
 * @return
 *     0, successfully ask multiple gce begin to work.\n
 *     -EINVAL, if pkt is invalid pointer.\n
 *     -EINVAL, num is not between 1 to CMDQ_THR_MAX_COUNT.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_multiple_reset(struct cmdq_pkt **pkt, int num)
{
	int i;

	if (IS_ERR_OR_NULL(pkt))
		return -EINVAL;
	if (num < 1 || num > CMDQ_THR_MAX_COUNT)
		return -EINVAL;

	for (i = 0; i < num; i++)
		if (pkt[i])
			pkt[i]->cmd_buf_size = 0;

	return 0;
}
EXPORT_SYMBOL(cmdq_pkt_multiple_reset);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Append wait for event command to the multiple cmdq packet.
 * @param[out]
 *     pkt: the cmdq packet.
 * @param[in]
 *     event: the desired event type to "wait and CLEAR".
 * @param[in]
 *     num: a total of cmdq packet
 * @return
 *     0, successfully append command into the cmdq command buffer.\n
 *     -EINVAL, the event number > CMDQ_MAX_EVENT.\n
 *     -EINVAL, if the pkt is invalid pointer.\n
 *     -EINVAL, num is not between 1 to CMDQ_THR_MAX_COUNT.\n
 *     error code from cmdq_pkt_append_command().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_multiple_wfe(struct cmdq_pkt **pkt, u16 event, int num)
{
	int i, err;

	if (event > CMDQ_MAX_EVENT)
		return -EINVAL;
	if (IS_ERR_OR_NULL(pkt))
		return -EINVAL;
	if (num < 1 || num > CMDQ_THR_MAX_COUNT)
		return -EINVAL;

	for (i = 0; i < num; i++) {
		err = cmdq_pkt_append_command(pkt[i],
					      CMDQ_GET_ARG_C(CMDQ_WFE_OPTION),
					      CMDQ_GET_ARG_B(CMDQ_WFE_OPTION),
					      event, 0, 0, 0, 0, CMDQ_CODE_WFE);
		if (err < 0)
			return err;
	}

	return 0;
}
EXPORT_SYMBOL(cmdq_pkt_multiple_wfe);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Append clear event command to the multiple cmdq packet.
 * @param[out]
 *     pkt: the cmdq packet.
 * @param[in]
 *     event: the desired event to be cleared.
 * @param[in]
 *     num: a total of cmdq packet
 * @return
 *     0, successfully append command into the cmdq command buffer.\n
 *     -EINVAL, the event number > CMDQ_MAX_EVENT.\n
 *     -EINVAL, the pkt is invalid pointer.\n
 *     -EINVAL, num is not between 1 to CMDQ_THR_MAX_COUNT.\n
 *     error code from cmdq_pkt_append_command().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_multiple_clear_event(struct cmdq_pkt **pkt, u16 event, int num)
{
	int i, err;

	if (event > CMDQ_MAX_EVENT)
		return -EINVAL;
	if (IS_ERR_OR_NULL(pkt))
		return -EINVAL;
	if (num < 1 || num > CMDQ_THR_MAX_COUNT)
		return -EINVAL;

	for (i = 0; i < num; i++) {
		err = cmdq_pkt_append_command(pkt[i],
					      CMDQ_GET_ARG_C(CMDQ_WFE_UPDATE),
					      CMDQ_GET_ARG_B(CMDQ_WFE_UPDATE),
					      event, 0, 0, 0, 0, CMDQ_CODE_WFE);
		if (err < 0)
			return err;
	}

	return 0;
}
EXPORT_SYMBOL(cmdq_pkt_multiple_clear_event);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Append sync 4 gce command at the beginning of command buffer.
 * @param[out]
 *     cmdq_handle: the cmdq packet.
 * @param[in]
 *     num: a total of cmdq packet.
 * @param[in]
 *     sync_event: the event used for sync.
 * @param[in]
 *     thread_token: the token used for sync.
 * @return
 *     0, successfully append presync command into command buffer.\n
 *     -EINVAL, the cmdq_handle is invalid pointer.\n
 *     -EINVAL, num is not equal to 4.\n
 *     error code from cmdq_pkt_set_token().\n
 *     error code from cmdq_pkt_wfe().\n
 *     error code from cmdq_pkt_clear_event().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_pkt_presync_gce(struct cmdq_pkt **cmdq_handle, int num,
			  u16 sync_event, u16 thread_token)
{
	int i, j, err = 0;

	if (IS_ERR_OR_NULL(cmdq_handle))
		return -EINVAL;
	if (num != 4)
		return -EINVAL;

	for (i = 0; i < num; i++) {
		for (j = 0; j < num; j++)
			err |= cmdq_pkt_set_token(cmdq_handle[i],
						  thread_token + i, j);
		for (j = 0; j < num; j++)
			err |= cmdq_pkt_wfe(cmdq_handle[i], thread_token + j);
		for (j = 0; j < num; j++)
			err |= cmdq_pkt_clear_event(cmdq_handle[i],
						    thread_token + j);
	}

	err |= cmdq_pkt_clear_event(cmdq_handle[0], sync_event);
	err |= cmdq_pkt_wfe(cmdq_handle[0], sync_event);

	for (i = 0; i < num ; i++) {
		err |= cmdq_pkt_set_token(cmdq_handle[0], thread_token + 4, i);
		err |= cmdq_pkt_wfe(cmdq_handle[i], thread_token + 4);
		err |= cmdq_pkt_clear_event(cmdq_handle[i], thread_token + 4);
	}

	return err;
}
EXPORT_SYMBOL(cmdq_pkt_presync_gce);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Assign the value into operand variable.
 * @param[out]
 *     operand: the cmdq operand.
 * @param[in]
 *     value: the value client want to assign.
 * @return
 *     cmdq_operand structure pointer.\n
 *     ERR_PTR(-EINVAL), operand is a NULL pointer.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If operand is NULL, return ERR_PTR(-EINVAL).
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
struct cmdq_operand *cmdq_operand_immediate(struct cmdq_operand *operand,
					    u16 value)
{
	if (!operand)
		return (struct cmdq_operand *)ERR_PTR(-EINVAL);

	operand->reg = false;
	operand->value = value;

	return operand;
}
EXPORT_SYMBOL(cmdq_operand_immediate);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Assign the register into operand variable.
 * @param[out]
 *     operand: the cmdq operand.
 * @param[in]
 *     idx: the register index client want to assign.
 * @return
 *     cmdq_operand structure pointer.\n
 *     ERR_PTR(-EINVAL), operand is a NULL pointer.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If operand is NULL, return ERR_PTR(-EINVAL).
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
struct cmdq_operand *cmdq_operand_reg(struct cmdq_operand *operand, u16 idx)
{
	if (!operand)
		return (struct cmdq_operand *)ERR_PTR(-EINVAL);

	operand->reg = true;
	operand->idx = idx;

	return operand;
}
EXPORT_SYMBOL(cmdq_operand_reg);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Get the command buffer size.
 * @param[in]
 *     pkt: the cmdq packet.
 * @return
 *     command buffer size.\n
 *     -EINVAL, pkt is an invalid pointer.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_get_cmd_buf_size(struct cmdq_pkt *pkt)
{
	if (IS_ERR_OR_NULL(pkt))
		return -EINVAL;

	return (int)(pkt->cmd_buf_size);
}
EXPORT_SYMBOL(cmdq_get_cmd_buf_size);

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     Make the read instruction readable.
 * @param[in]
 *     dev: caller's device.
 * @param[in]
 *     offset: command offset address.
 * @param[in]
 *     cmdq_inst: gce 64bits instruction.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void cmdq_buf_print_read(const struct device *dev, u32 offset,
				struct cmdq_instruction *cmdq_inst)
{
	u32 addr = ((u32)(cmdq_inst->arg_b |
		    (cmdq_inst->s_op << CMDQ_SUBSYS_SHIFT)));

	dev_err(dev, "0x%08x [Read | Load] Reg Index 0x%08x = %s0x%08x\n",
		offset, cmdq_inst->arg_a,
		cmdq_inst->arg_b_type ? "*Reg Index " : "SubSys Reg ", addr);
}

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     Make the write instruction readable.
 * @param[in]
 *     dev: caller's device.
 * @param[in]
 *     offset: command offset address.
 * @param[in]
 *     cmdq_inst: gce 64bits instruction.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void cmdq_buf_print_write(const struct device *dev, u32 offset,
				 struct cmdq_instruction *cmdq_inst)
{
	u32 addr = ((u32)(cmdq_inst->arg_a |
		    (cmdq_inst->s_op << CMDQ_SUBSYS_SHIFT)));

	dev_err(dev, "0x%08x [Write | Store] %s0x%08x = %s0x%08x\n",
		offset, cmdq_inst->arg_a_type ? "*Reg Index " : "SubSys Reg ",
		addr, CMDQ_REG_IDX_PREFIX(!cmdq_inst->arg_b_type),
		cmdq_inst->arg_b_type ? cmdq_inst->arg_b :
		CMDQ_GET_32B_VALUE(cmdq_inst->arg_b, cmdq_inst->arg_c));
}

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     Make the wfe instruction readable.
 * @param[in]
 *     dev: caller's device.
 * @param[in]
 *     offset: command offset address.
 * @param[in]
 *     cmdq_inst: gce 64bits instruction.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void cmdq_buf_print_wfe(const struct device *dev, u32 offset,
			       struct cmdq_instruction *cmdq_inst)
{
	u32 cmd = CMDQ_GET_32B_VALUE(cmdq_inst->arg_b, cmdq_inst->arg_c);

	dev_err(dev, "0x%08x %s event %u\n", offset,
		(cmd & CMDQ_WFE_OPTION) ? "wait for" : "clear",
		cmdq_inst->arg_a);
}

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     Convert the s_op to words.
 * @param[in]
 *     s_op: CMDQ_LOGIC_ENUM.
 * @return
 *     words.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static const char *cmdq_parse_logic_sop(enum CMDQ_LOGIC_ENUM s_op)
{
	switch (s_op) {
	case CMDQ_LOGIC_ASSIGN:
		return "= ";
	case CMDQ_LOGIC_ADD:
		return "+ ";
	case CMDQ_LOGIC_SUBTRACT:
		return "- ";
	case CMDQ_LOGIC_MULTIPLY:
		return "* ";
	case CMDQ_LOGIC_XOR:
		return "^";
	case CMDQ_LOGIC_NOT:
		return "= ~";
	case CMDQ_LOGIC_OR:
		return "| ";
	case CMDQ_LOGIC_AND:
		return "& ";
	case CMDQ_LOGIC_LEFT_SHIFT:
		return "<< ";
	case CMDQ_LOGIC_RIGHT_SHIFT:
		return ">> ";
	default:
		return "<error: unsupported logic sop>";
	}
}

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     Convert the s_op to words.
 * @param[in]
 *     s_op: CMDQ_CONDITION_ENUM.
 * @return
 *     words.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static const char *cmdq_parse_jump_c_sop(enum CMDQ_CONDITION_ENUM s_op)
{
	switch (s_op) {
	case CMDQ_EQUAL:
		return "==";
	case CMDQ_NOT_EQUAL:
		return "!=";
	case CMDQ_GREATER_THAN_AND_EQUAL:
		return ">=";
	case CMDQ_LESS_THAN_AND_EQUAL:
		return "<=";
	case CMDQ_GREATER_THAN:
		return ">";
	case CMDQ_LESS_THAN:
		return "<";
	default:
		return "<error: unsupported jump conditional sop>";
	}
}

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     Make the mask instruction readable.
 * @param[in]
 *     dev: caller's device.
 * @param[in]
 *     offset: command offset address.
 * @param[in]
 *     cmdq_inst: gce 64bits instruction.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void cmdq_buf_print_mask(const struct device *dev, u32 offset,
				struct cmdq_instruction *cmdq_inst)
{
	u32 mask = CMDQ_GET_32B_VALUE(cmdq_inst->arg_b, cmdq_inst->arg_c);

	dev_err(dev, "0x%08x mask 0x%08x\n", offset, ~mask);
}

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     Make the logic instruction readable.
 * @param[in]
 *     dev: caller's device.
 * @param[in]
 *     offset: command offset address.
 * @param[in]
 *     cmdq_inst: gce 64bits instruction.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void cmdq_buf_print_logic(const struct device *dev, u32 offset,
				 struct cmdq_instruction *cmdq_inst)
{
	switch (cmdq_inst->s_op) {
	case CMDQ_LOGIC_ASSIGN:
		dev_err(dev,
			"0x%08x [Logic] Reg Index 0x%08x %s%s0x%08x\n",
			offset, cmdq_inst->arg_a,
			cmdq_parse_logic_sop(cmdq_inst->s_op),
			CMDQ_REG_IDX_PREFIX(cmdq_inst->arg_b_type),
			CMDQ_GET_32B_VALUE(cmdq_inst->arg_b, cmdq_inst->arg_c));
		break;
	case CMDQ_LOGIC_NOT:
		dev_err(dev,
			"0x%08x [Logic] Reg Index 0x%08x %s%s0x%08x\n",
			offset, cmdq_inst->arg_a,
			cmdq_parse_logic_sop(cmdq_inst->s_op),
			CMDQ_REG_IDX_PREFIX(cmdq_inst->arg_b_type),
			cmdq_inst->arg_b);
		break;
	default:
		dev_err(dev,
			"0x%08x [Logic] %s0x%08x = %s0x%08x %s%s0x%08x\n",
			offset, CMDQ_REG_IDX_PREFIX(cmdq_inst->arg_a_type),
			cmdq_inst->arg_a,
			CMDQ_REG_IDX_PREFIX(cmdq_inst->arg_b_type),
			cmdq_inst->arg_b, cmdq_parse_logic_sop(cmdq_inst->s_op),
			CMDQ_REG_IDX_PREFIX(cmdq_inst->arg_c_type),
			cmdq_inst->arg_c);
		break;
	}
}

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     Make the jump_c instruction readable.
 * @param[in]
 *     dev: caller's device.
 * @param[in]
 *     offset: command offset address.
 * @param[in]
 *     cmdq_inst: gce 64bits instruction.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void cmdq_buf_print_write_jump_c(const struct device *dev, u32 offset,
					struct cmdq_instruction *cmdq_inst)
{
	dev_err(dev,
		"0x%08x [Relative Jump] if (%s0x%08x %s %s0x%08x) jump %s0x%08x\n",
		offset, CMDQ_REG_IDX_PREFIX(cmdq_inst->arg_b_type),
		cmdq_inst->arg_b, cmdq_parse_jump_c_sop(cmdq_inst->s_op),
		CMDQ_REG_IDX_PREFIX(cmdq_inst->arg_c_type), cmdq_inst->arg_c,
		CMDQ_REG_IDX_PREFIX(cmdq_inst->arg_a_type), cmdq_inst->arg_a);
}

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     Make the polling instruction readable.
 * @param[in]
 *     dev: caller's device.
 * @param[in]
 *     offset: command offset address.
 * @param[in]
 *     cmdq_inst: gce 64bits instruction.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void cmdq_buf_print_poll(const struct device *dev, u32 offset,
				struct cmdq_instruction *cmdq_inst)
{
	u32 addr = ((u32)(cmdq_inst->arg_a |
		   (cmdq_inst->s_op << CMDQ_SUBSYS_SHIFT)));

	dev_err(dev, "0x%08x 0x%016llx[Polling] 0x%08x = 0x%08x", offset,
		*((u64 *)cmdq_inst), addr,
		CMDQ_GET_32B_VALUE(cmdq_inst->arg_b, cmdq_inst->arg_c));
}

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     Make the misc instruction readable.
 * @param[in]
 *     dev: caller's device.
 * @param[in]
 *     offset: command offset address.
 * @param[in]
 *     cmdq_inst: gce 64bits instruction.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void cmdq_buf_print_misc(const struct device *dev, u32 offset,
				struct cmdq_instruction *cmdq_inst)
{
	char *cmd_str;

	switch (cmdq_inst->op) {
	case CMDQ_CODE_JUMP:
		cmd_str = "jump";
		break;
	case CMDQ_CODE_EOC:
		cmd_str = "eoc";
		break;
	default:
		cmd_str = "unknown";
		break;
	}

	dev_err(dev, "0x%08x %s\n", offset, cmd_str);
}

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     Parse the command information.
 * @param[in]
 *     dev: caller's device.
 * @param[in]
 *     buf: buffer stored the printing information.
 * @param[in]
 *     cmd_nr: command counter.
 * @param[in]
 *     pa_offset: error phycial address offset.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void cmdq_buf_cmd_parse(const struct device *dev, u64 *buf,
			       int cmd_nr, u32 pa_offset)
{
	struct cmdq_instruction *cmdq_inst = (struct cmdq_instruction *)buf;
	u32 i, offset = 0;

	for (i = 0; i < cmd_nr; i++) {
		if (offset == pa_offset)
			dev_err(dev,
				"\e[1;31;40m==========ERROR==========\e[0m\n");
		switch (cmdq_inst[i].op) {
		case CMDQ_CODE_WRITE_S:
		case CMDQ_CODE_WRITE_S_W_MASK:
			cmdq_buf_print_write(dev, offset, &cmdq_inst[i]);
			break;
		case CMDQ_CODE_WFE:
			cmdq_buf_print_wfe(dev, offset, &cmdq_inst[i]);
			break;
		case CMDQ_CODE_MASK:
			cmdq_buf_print_mask(dev, offset, &cmdq_inst[i]);
			break;
		case CMDQ_CODE_READ_S:
			cmdq_buf_print_read(dev, offset, &cmdq_inst[i]);
			break;
		case CMDQ_CODE_LOGIC:
			cmdq_buf_print_logic(dev, offset, &cmdq_inst[i]);
			break;
		case CMDQ_CODE_JUMP_C_RELATIVE:
			cmdq_buf_print_write_jump_c(dev, offset, &cmdq_inst[i]);
			break;
		case CMDQ_CODE_POLL:
			cmdq_buf_print_poll(dev, offset, &cmdq_inst[i]);
			break;
		default:
			cmdq_buf_print_misc(dev, offset, &cmdq_inst[i]);
			break;
		}
		if (offset == pa_offset)
			dev_err(dev,
				"\e[1;31;40m==========ERROR==========\e[0m\n");
		offset += CMDQ_INST_SIZE;
	}
}

/** @ingroup IP_group_gce_internal_function
 * @par Description
 *     Dump the command information.
 * @param[in]
 *     thread: cmdq thread.
 * @param[in]
 *     timeout: timeout information.
 * @param[in]
 *     curr_pa: phsical address counter.
 * @return
 *     0, successfully dump the information from the command buffer.\n
 *     -EINVAL, thread is a NULL pointer.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_buf_dump(struct cmdq_thread *thread, bool timeout, u32 curr_pa)
{
	struct device *dev;
	u32 pa_offset;

	if (!thread)
		return -EINVAL;

	dev = thread->cmdq->mbox.dev;
	pa_offset = curr_pa - thread->pa_base;
	dev_err(dev, "dump %s pkt 0x%p start ----------\n",
		timeout ? "timeout" : "error", thread->pkt);
	dev_err(dev, "pa_curr - pa_base = 0x%x\n", pa_offset);
	dev_err(dev, "dump %s pkt 0x%p end   ----------\n",
		timeout ? "timeout" : "error", thread->pkt);
	cmdq_buf_cmd_parse(dev, thread->pkt->va_base,
			   CMDQ_NUM_CMD(thread->pkt->cmd_buf_size), pa_offset);

	return 0;
}
EXPORT_SYMBOL(cmdq_buf_dump);

/** @ingroup IP_group_gce_external_function
 * @par Description
 *     Dump command buffer.
 * @param[in]
 *     dev: Caller's device.
 * @param[in]
 *     pkt: command buffer pointer.
 * @return
 *     0, successfully dump the command buffer.\n
 *     -EINVAL, pkt is an invalid pointer.\n
 *     -EINVAL, dev is a NULL pointer.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int cmdq_dump_pkt(const struct device *dev, struct cmdq_pkt *pkt)
{
	u64 *buf;

	if (IS_ERR_OR_NULL(pkt) || !dev)
		return -EINVAL;

	buf = (u64 *)pkt->va_base;
	dev_err(dev, "Normal dump cmdq pkt start --------\n");
	dev_err(dev, "pkt_va = 0x%p\n", buf);
	cmdq_buf_cmd_parse(dev, buf, pkt->cmd_buf_size / CMDQ_INST_SIZE, -1);
	dev_err(dev, "Normal dump cmdq pkt end   --------\n");

	return 0;
}
EXPORT_SYMBOL(cmdq_dump_pkt);
