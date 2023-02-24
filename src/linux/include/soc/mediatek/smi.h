/*
 * Copyright (c) 2015-2016 MediaTek Inc.
 * Author: Yong Wu <yong.wu@mediatek.com>
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
 * @file smi.h
 * Header of mtk-smi.c
 */
#ifndef MTK_IOMMU_SMI_H
#define MTK_IOMMU_SMI_H

#include <linux/bitops.h>
#include <linux/device.h>

#ifdef CONFIG_MTK_SMI

/** @ingroup IP_group_smi_external_def
 * @brief SMI-IOMMU register definition
 */
/** the max larb number */
#define MTK_LARB_NR_MAX		64

/** the mmu enable macro */
#define MTK_SMI_MMU_EN(port)	BIT(port)

/** @ingroup IP_group_smi_external_struct
 * @brief the Larb MMU Data Structure.
 */
struct mtk_smi_larb_iommu {
	/** the larb device structure */
	struct device *dev;
	/** the ports that enabled iommu in this larb */
	unsigned int   mmu;
};

/** @ingroup IP_group_smi_external_struct
 * @brief SMI Larbs MMU Data Structure
 */
struct mtk_smi_iommu {
	/** the larb number in this soc */
	unsigned int larb_nr;
	/** the mmu info for each larb */
	struct mtk_smi_larb_iommu larb_imu[MTK_LARB_NR_MAX];
};

/*
 * mtk_smi_larb_get: Enable the power domain and clocks for this local arbiter.
 *                   It also initialize some basic setting(like iommu).
 * mtk_smi_larb_put: Disable the power domain and clocks for this local arbiter.
 * Both should be called in non-atomic context.
 *
 * Returns 0 if successful, negative on failure.
 */
int mtk_smi_larb_get(struct device *larbdev);
void mtk_smi_larb_put(struct device *larbdev);

#else

static inline int mtk_smi_larb_get(struct device *larbdev)
{
	return 0;
}

static inline void mtk_smi_larb_put(struct device *larbdev) { }

#endif

#endif
