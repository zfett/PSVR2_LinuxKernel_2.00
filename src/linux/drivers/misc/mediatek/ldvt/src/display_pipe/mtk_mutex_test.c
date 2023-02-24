/*
 * Copyright (c) 2018 MediaTek Inc.
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

#include <linux/string.h>
#include <linux/slab.h>
#include <soc/mediatek/mtk_mutex.h>
#include "mtk_mutex_test.h"

struct mtk_mutex_cb_data {
	enum mtk_mutex_irq_sta_index idx;
	void *dispsys;
	u32 cb_cnt;
};

struct mtk_mutex_test_ctx {
	struct mtk_mutex_cb_data priv[MUTEX_IRQ_STA_NR];
	struct mtk_mutex_res *dly;
};

static void mtk_mutex_test_init(struct mtk_dispsys *dispsys)
{
	struct mtk_mutex_test_ctx *ctx;
	enum mtk_mutex_irq_sta_index i;

	if (!dispsys) {
		pr_err("Invalid parameter @%s, %d\n", __func__, __LINE__);
		return;
	}
	if (dispsys->mutex_test) {
		pr_err("mutex test context has been initialized!\n");
		return;
	}

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	for (i = MUTEX_IRQ_STA_0; i < MUTEX_IRQ_STA_NR; i++) {
		ctx->priv[i].idx = i;
		ctx->priv[i].dispsys = dispsys;
	}
	dispsys->mutex_test = ctx;

	pr_err("mmsys core test context is initialized!\n");
}

static void print_fps(u32 mm_clock, u32 period)
{
	u32 mhz = mm_clock, khz = mhz * 1000, hz = khz * 1000;
	u32 r, t, ms, us, ns, fps, kfps;

	ms = period / khz;
	r = period % khz;
	us = r / mhz;
	r = r % mhz;
	ns = r * 1000 * 1000 / mhz;

	fps = hz / period;
	r = hz % period;
	if (r > 0xffffffff / 1000) {
		/* avoid 32-bit overflow */
		t = r / (0xffffffff / 1000) + 1;
		kfps = r / t * 1000 / (period / t);
	} else {
		kfps = r * 1000 / period;
	}

	pr_err("period time about: %u.%03u%03u ms, fps about: %u.%03u\n",
		ms, us, ns, fps, kfps);
}

static void mtk_mutex_test_irq_status_cb(struct mtk_mmsys_cb_data *data)
{
	struct mtk_mutex_cb_data *priv = data->priv_data;
	struct mtk_dispsys *dispsys;

	if (!priv) {
		pr_err("Invalid parameter @%s, %d\n", __func__, __LINE__);
		return;
	}

	if (priv->cb_cnt > 0)
		priv->cb_cnt--;

	pr_err("~mutex_cb: idx=%u, status=0x%08x, remain_cnt=%u\n",
		priv->idx, data->status, priv->cb_cnt);

	dispsys = priv->dispsys;
	if (priv->idx == MUTEX_IRQ_STA_8 || priv->idx == MUTEX_IRQ_STA_10) {
		struct mtk_mutex_res *m;
		struct mtk_mutex_timer_status s;

		m = dispsys->mutex[MUTEX_DISP];
		mtk_mutex_timer_get_status(m, &s);
		pr_err("timer(%d) %08x,%08x(%u),%08x(%u),%08x(%u),%08x(%u)\n",
			mtk_mutex_res2id(m), *(u32 *)&s,
			s.src_timeout, s.src_timeout,
			s.ref_timeout, s.ref_timeout,
			s.src_time, s.src_time,
			s.ref_time, s.ref_time);

		if (s.src_sel == s.ref_sel &&
		    s.src_edge == s.ref_edge &&
		    s.event_sel == MUTEX_TO_EVENT_REF_EDGE &&
		    s.ref_time > 0)
			print_fps(s.mm_clock, s.ref_time);

	}

	if (priv->cb_cnt == 0) {
		struct device *dev;

		dev = dispsys->disp_dev[MTK_DISP_MUTEX];
		mtk_mutex_register_cb(dev, priv->idx, NULL, 0, NULL);
	}
}

static void mtk_mutex_test_enable_irq(struct mtk_dispsys *dispsys,
				      u32 idx, u32 status, u32 count)
{
	struct mtk_mutex_test_ctx *ctx;
	struct device *dev;

	if (!dispsys) {
		pr_err("Invalid parameter @%s, %d\n", __func__, __LINE__);
		return;
	}
	if (!dispsys->mutex_test) {
		pr_err("mutex test context has not been initialized!\n");
		return;
	}

	ctx = dispsys->mutex_test;
	ctx->priv[idx].cb_cnt = count;
	dev = dispsys->disp_dev[MTK_DISP_MUTEX];
	mtk_mutex_register_cb(dev, idx, mtk_mutex_test_irq_status_cb,
				status,	&ctx->priv[idx]);
}

static void mtk_mutex_test_remove_comp_for_fpga(struct mtk_dispsys *dispsys)
{
	int i, j;
	struct mtk_mutex_res *m;
	const u8 comp[] = {
		MUTEX_COMPONENT_MDP_RDMA1,
		MUTEX_COMPONENT_MDP_RDMA2,
		MUTEX_COMPONENT_MDP_RDMA3,
		MUTEX_COMPONENT_MDP_RDMA_PVRIC1,
		MUTEX_COMPONENT_MDP_RDMA_PVRIC2,
		MUTEX_COMPONENT_MDP_RDMA_PVRIC3,
		MUTEX_COMPONENT_DISP_RDMA1,
		MUTEX_COMPONENT_DISP_RDMA2,
		MUTEX_COMPONENT_DISP_RDMA3,
		MUTEX_COMPONENT_LHC1,
		MUTEX_COMPONENT_LHC2,
		MUTEX_COMPONENT_LHC3,
		MUTEX_COMPONENT_DSC1,
		MUTEX_COMPONENT_DSC2,
		MUTEX_COMPONENT_DSC3,
		MUTEX_COMPONENT_DSI1,
		MUTEX_COMPONENT_DSI2,
		MUTEX_COMPONENT_DSI3
	};

	if (!dispsys) {
		pr_err("Invalid parameter @%s, %d\n", __func__, __LINE__);
		return;
	}
	if (!dispsys->mutex_test) {
		pr_err("mutex test context has been initialized!\n");
		return;
	}

	for (i = 0; i < MUTEX_NR; i++) {
		m = dispsys->mutex[i];
		if (IS_ERR(m))
			continue;
		pr_err("Remove components from mutex[%d] on FPGA\n",
			*(int *)m);
		for (j = 0; j < ARRAY_SIZE(comp); j++)
			mtk_mutex_remove_comp(m, comp[j], NULL);

		mtk_mutex_enable(m, NULL);
	}
}

static void mtk_mutex_test_enable(struct mtk_dispsys *dispsys, u32 en)
{
	int i;
	struct mtk_mutex_res *m;

	if (!dispsys) {
		pr_err("Invalid parameter @%s, %d\n", __func__, __LINE__);
		return;
	}
	if (!dispsys->mutex_test) {
		pr_err("mutex test context has been initialized!\n");
		return;
	}

	for (i = 0; i < MUTEX_NR; i++) {
		m = dispsys->mutex[i];
		if (IS_ERR(m))
			continue;
		pr_err("mutex[%d] enable(%u)\n", *(int *)m, en);
		if (en == 1)
			mtk_mutex_enable(m, NULL);
		else
			mtk_mutex_disable(m, NULL);
	}
}

static void mtk_mutex_test_add_err_monitor(struct mtk_dispsys *dispsys)
{
	int i;
	int ret = 0;
	struct mtk_mutex_res *m;

	if (!dispsys) {
		pr_err("Invalid parameter @%s, %d\n", __func__, __LINE__);
		return;
	}

	for (i = 0; i < MUTEX_NR; i++) {
		m = dispsys->mutex[i];
		if (IS_ERR(m))
			continue;
		ret |= mtk_mutex_add_monitor(m,
			MUTEX_MMSYS_DSI0_UNDERFLOW, NULL);
		ret |= mtk_mutex_add_monitor(m,
			MUTEX_MMSYS_DSI1_UNDERFLOW, NULL);
		ret |= mtk_mutex_add_monitor(m,
			MUTEX_MMSYS_DSI2_UNDERFLOW, NULL);
		ret |= mtk_mutex_add_monitor(m,
			MUTEX_MMSYS_DSI3_UNDERFLOW, NULL);
		ret |= mtk_mutex_error_monitor_enable(m, NULL, false);
	}

	if (ret != 0)
		pr_err("mutex add error monitor failed %d!", ret);
}

static void mtk_mutex_test_remove_err_monitor(struct mtk_dispsys *dispsys)
{
	int i;
	int ret = 0;
	struct mtk_mutex_res *m;

	if (!dispsys) {
		pr_err("Invalid parameter @%s, %d\n", __func__, __LINE__);
		return;
	}

	for (i = 0; i < MUTEX_NR; i++) {
		m = dispsys->mutex[i];
		if (IS_ERR(m))
			continue;
		ret |= mtk_mutex_remove_monitor(m,
			MUTEX_MMSYS_DSI0_UNDERFLOW, NULL);
		ret |= mtk_mutex_remove_monitor(m,
			MUTEX_MMSYS_DSI1_UNDERFLOW, NULL);
		ret |= mtk_mutex_remove_monitor(m,
			MUTEX_MMSYS_DSI2_UNDERFLOW, NULL);
		ret |= mtk_mutex_remove_monitor(m,
			MUTEX_MMSYS_DSI3_UNDERFLOW, NULL);
		ret |= mtk_mutex_error_monitor_disable(m, NULL);
	}

	if (ret != 0)
		pr_err("mutex remove error monitor failed %d!", ret);
}

static void mtk_mutex_test_add_to_ext_signal(struct mtk_dispsys *dispsys)
{
	int i;
	int ret = 0;
	struct mtk_mutex_res *m;

	if (!dispsys) {
		pr_err("Invalid parameter @%s, %d\n", __func__, __LINE__);
		return;
	}

	for (i = 0; i < MUTEX_NR; i++) {
		m = dispsys->mutex[i];
		if (IS_ERR(m))
			continue;
		ret |= mtk_mutex_add_to_ext_signal(m,
			MMCORE_DSI_0_MUTE_EXT_SIGNAL, NULL);
		ret |= mtk_mutex_add_to_ext_signal(m,
			MMCORE_DSI_1_MUTE_EXT_SIGNAL, NULL);
		ret |= mtk_mutex_add_to_ext_signal(m,
			MMCORE_DSI_2_MUTE_EXT_SIGNAL, NULL);
		ret |= mtk_mutex_add_to_ext_signal(m,
			MMCORE_DSI_3_MUTE_EXT_SIGNAL, NULL);
	}

	if (ret != 0)
		pr_err("mutex add to ext signal failed %d!", ret);
}

static void mtk_mutex_test_remove_from_ext_signal(struct mtk_dispsys *dispsys)
{
	int i;
	int ret = 0;
	struct mtk_mutex_res *m;

	if (!dispsys) {
		pr_err("Invalid parameter @%s, %d\n", __func__, __LINE__);
		return;
	}

	for (i = 0; i < MUTEX_NR; i++) {
		m = dispsys->mutex[i];
		if (IS_ERR(m))
			continue;
		ret |= mtk_mutex_remove_from_ext_signal(m,
			MMCORE_DSI_0_MUTE_EXT_SIGNAL, NULL);
		ret |= mtk_mutex_remove_from_ext_signal(m,
			MMCORE_DSI_1_MUTE_EXT_SIGNAL, NULL);
		ret |= mtk_mutex_remove_from_ext_signal(m,
			MMCORE_DSI_2_MUTE_EXT_SIGNAL, NULL);
		ret |= mtk_mutex_remove_from_ext_signal(m,
			MMCORE_DSI_3_MUTE_EXT_SIGNAL, NULL);
	}

	if (ret != 0)
		pr_err("mutex remove from ext signal failed %d!", ret);
}

static void mtk_mutex_test_sync_delay(struct mtk_dispsys *dispsys, u32 dly_idx,
				      u32 dly_time)
{
	struct mtk_mutex_test_ctx *ctx;
	struct mtk_mutex_res *dly, *m;
	struct device *dev;
	int i, ret;

	if (!dispsys) {
		pr_err("Invalid parameter @%s, %d\n", __func__, __LINE__);
		return;
	}
	if (!dispsys->mutex_test) {
		pr_err("mutex test context has not been initialized!\n");
		return;
	}
	ctx = dispsys->mutex_test;
	if (ctx->dly) {
		pr_err("please disable/put sync delay(%d) firstly!\n",
			*(int *)ctx->dly);
		return;
	}

	dev = dispsys->disp_dev[MTK_DISP_MUTEX];
	dly = mtk_mutex_delay_get(dev, dly_idx);
	if (IS_ERR(dly)) {
		pr_err("mutex_delay_get failed! index=%u, %p\n", dly_idx, dly);
		return;
	}

	ret = mtk_mutex_select_delay_sof(dly, MUTEX_MMSYS_SOF_DSI0, NULL);
	if (ret)
		pr_err("mutex_select_delay_sof failed! %d\n", ret);

	ret |= mtk_mutex_set_delay_us(dly, dly_time, NULL);
	if (ret)
		pr_err("mutex_set_delay_us failed! %d\n", ret);

	for (i = 0; i < MUTEX_NR; i++) {
		m = dispsys->mutex[i];
		if (IS_ERR(m))
			continue;
		ret |= mtk_mutex_select_sof(m, mtk_mutex_get_delay_sof(dly),
			NULL, true);
		if (ret)
			pr_err("mutex_select_sof failed! %d\n",	ret);
	}

	ret |= mtk_mutex_delay_enable(dly, NULL);
	for (i = 0; i < MUTEX_NR; i++) {
		m = dispsys->mutex[i];
		if (IS_ERR(m))
			continue;
		ret |= mtk_mutex_enable(m, NULL);
	}

	if (ret != 0)
		pr_err("mutex sync delay test failed %d!", ret);

	ctx->dly = dly;
}

static void mtk_mutex_test_sync_delay_put(struct mtk_dispsys *dispsys)
{
	struct mtk_mutex_test_ctx *ctx;
	int ret;

	if (!dispsys) {
		pr_err("Invalid parameter @%s, %d\n", __func__, __LINE__);
		return;
	}
	if (!dispsys->mutex_test) {
		pr_err("mutex test context has not been initialized!\n");
		return;
	}
	ctx = dispsys->mutex_test;
	if (!ctx->dly) {
		pr_err("No sync delay is used!\n");
		return;
	}

	ret = mtk_mutex_delay_disable(ctx->dly, NULL);
	ret |= mtk_mutex_delay_put(ctx->dly);
	if (ret != 0)
		pr_err("mutex sync delay release failed %d!", ret);
	else
		pr_err("mutex sync delay release success!");

	ctx->dly = NULL;
}

static void mtk_mutex_test_timer_debugger(struct mtk_dispsys *dispsys,
			u32 src_sof, u32 ref_sof,
			bool src_edge, u32 ref_edge,
			u32 src_time, u32 ref_time,
			enum mtk_mutex_timeout_event event)
{
	struct mtk_mutex_res *m;
	int i, ret = 0;

	if (!dispsys) {
		pr_err("Invalid parameter @%s, %d\n", __func__, __LINE__);
		return;
	}
	if (!dispsys->mutex_test) {
		pr_err("mutex test context has not been initialized!\n");
		return;
	}

	for (i = 0; i < MUTEX_NR; i++) {
		m = dispsys->mutex[i];
		if (IS_ERR(m) || i != MUTEX_DISP)
			continue;
		pr_err("enable mutex timer(%d)...\n", mtk_mutex_res2id(m));
		ret |= mtk_mutex_select_timer_sof(m, src_sof, ref_sof, NULL);
		ret |= mtk_mutex_select_timer_sof_timing(m, src_edge, ref_edge,
							 NULL);
		ret |= mtk_mutex_set_timer_us(m, src_time, ref_time, NULL);
		ret |= mtk_mutex_timer_enable_ex(m, true, event, NULL);
	}

	if (ret != 0)
		pr_err("mutex timer debugger config failed %d!", ret);
}

static void mtk_mutex_test_timer_debugger_disable(struct mtk_dispsys *dispsys)
{
	struct mtk_mutex_res *m;
	int i, ret = 0;

	if (!dispsys) {
		pr_err("Invalid parameter @%s, %d\n", __func__, __LINE__);
		return;
	}
	if (!dispsys->mutex_test) {
		pr_err("mutex test context has not been initialized!\n");
		return;
	}

	for (i = 0; i < MUTEX_NR; i++) {
		m = dispsys->mutex[i];
		if (IS_ERR(m) || i != MUTEX_DISP)
			continue;
		pr_err("disable mutex timer(%d)...\n", mtk_mutex_res2id(m));
		ret |= mtk_mutex_timer_disable(m, NULL);
	}

	if (ret != 0)
		pr_err("mutex timer debugger disable failed %d!", ret);
}

void mtk_mutex_test(struct mtk_dispsys *dispsys, char *buf)
{
	int ret;

	if (!dispsys || !buf) {
		pr_err("Invalid parameter @%s, %d\n", __func__, __LINE__);
		return;
	}

	if (strncmp(buf, "init", 4) == 0) {
		mtk_mutex_test_init(dispsys);
	} else if (strncmp(buf, "irqe:", 5) == 0) {
		u32 idx = 0, irq_sta = 0, cnt = 0;

		ret = sscanf(buf, "irqe: %u %x %u", &idx, &irq_sta, &cnt);
		if (idx >= MUTEX_IRQ_STA_NR)
			idx = 0;
		pr_err("parse IRQ enable parameter(%d), %u 0x%x %u\n",
			ret, idx, irq_sta, cnt);
		mtk_mutex_test_enable_irq(dispsys, idx, irq_sta, cnt);
	} else if (strncmp(buf, "irqse:", 6) == 0) {
		u32 i = 0, idx = 0, irq_sta = 0, cnt = 0;

		ret = sscanf(buf, "irqse: %x %x %u", &idx, &irq_sta, &cnt);
		pr_err("parse IRQs enable parameter(%d), %u 0x%x %u\n",
			ret, idx, irq_sta, cnt);

		for (i = 0; i < MUTEX_IRQ_STA_NR; i++) {
			if ((BIT(i) & idx) == 0)
				continue;
			mtk_mutex_test_enable_irq(dispsys, i, irq_sta, cnt);
		}
	} else if (strncmp(buf, "fpga", 4) == 0) {
		mtk_mutex_test_remove_comp_for_fpga(dispsys);
	} else if (strncmp(buf, "e:", 2) == 0) {
		u32 en = 1;

		ret = sscanf(buf, "e: %u", &en);
		pr_err("parse mutex enable parameter(%d), %u\n", ret, en);
		if (ret >= 1)
			mtk_mutex_test_enable(dispsys, en);
	} else if (strncmp(buf, "ah:", 3) == 0) {
		u32 add = 1;

		ret = sscanf(buf, "ah: %u", &add);
		pr_err("parse auto hold parameter(%d), %u\n", ret, add);
		if (add == 1)
			mtk_mutex_test_add_err_monitor(dispsys);
		else
			mtk_mutex_test_remove_err_monitor(dispsys);
	} else if (strncmp(buf, "ext:", 4) == 0) {
		u32 add = 1;

		ret = sscanf(buf, "ext: %u", &add);
		pr_err("parse ext signal parameter(%d), %u\n", ret, add);
		if (add == 1)
			mtk_mutex_test_add_to_ext_signal(dispsys);
		else
			mtk_mutex_test_remove_from_ext_signal(dispsys);
	} else if (strncmp(buf, "dly:", 4) == 0) {
		u32 dly_idx = 0, dly_time = 10000;

		ret = sscanf(buf, "dly: %u %u", &dly_idx, &dly_time);
		pr_err("parse sync delay parameter(%d), %u %u\n",
			ret, dly_idx, dly_time);
		mtk_mutex_test_sync_delay(dispsys, dly_idx, dly_time);
	} else if (strncmp(buf, "dly_put", 7) == 0) {
		mtk_mutex_test_sync_delay_put(dispsys);
	} else if (strncmp(buf, "timer:", 6) == 0) {
		u32 src_sof = MUTEX_MMSYS_SOF_DSI0;
		u32 ref_sof = MUTEX_MMSYS_SOF_SINGLE;
		u32 src_time = 1000000, ref_time = 200000;
		u32 e = MUTEX_TO_EVENT_REF_EDGE;
		u32 src_edge = 0, ref_edge = 0;

		ret = sscanf(buf, "timer: %u %u %u %u %u %u %u",
				&src_sof, &ref_sof, &src_time, &ref_time,
				&e, &src_edge, &ref_edge);
		pr_err("parse timer parameter(%d), %u %u %u %u %u %u %u\n",
			ret, src_sof, ref_sof, src_time, ref_time, e,
			src_edge, ref_edge);
		mtk_mutex_test_timer_debugger(dispsys, src_sof, ref_sof,
						(bool)src_edge, (bool)ref_edge,
						src_time, ref_time, e);
	} else if (strncmp(buf, "timer_dis", 9) == 0) {
		mtk_mutex_test_timer_debugger_disable(dispsys);
	} else if (strncmp(buf, "dbg_en", 6) == 0) {
		mtk_mutex_debug_enable(dispsys->disp_dev[MTK_DISP_MUTEX],
					NULL);
	} else if (strncmp(buf, "dbg_dis", 7) == 0) {
		mtk_mutex_debug_disable(dispsys->disp_dev[MTK_DISP_MUTEX],
							NULL);
	} else if (strncmp(buf, "sof_cnt:", 8) == 0) {
		int id = 0;

		ret = sscanf(buf, "sof_cnt: %d", &id);
		ret = mtk_mutex_debug_get_sent_sof_count(
					dispsys->disp_dev[MTK_DISP_MUTEX], id);
		pr_info("mutex send sof[%d] %d times\n", id, ret);
	} else {
		pr_err("not supported command: %s", buf);
	}
}
