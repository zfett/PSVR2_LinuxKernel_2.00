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

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/of_platform.h>
#include <linux/uaccess.h>
#include <soc/mediatek/mtk_mm_sysram.h>
#include "mtk_wrapper_util.h"

// SYSRAM2 Definition
#define SYSRAM2_START_ADDRESS (CONFIG_MTK_GPU_FW_MEMORY_START + CONFIG_MTK_GPU_FW_MEMORY_SIZE)
#define SYSRAM2_SIZE (0x600000 - SYSRAM2_START_ADDRESS)

int mtk_fm_dev_probe(struct platform_device *pdev);
int mtk_fm_dev_remove(struct platform_device *pdev);
int mtk_warpa_fe_probe(struct platform_device *pdev);
int mtk_warpa_fe_remove(struct platform_device *pdev);

static struct platform_device* fm_pdev;
static struct platform_device* warpa_fe_pdev;
static struct device* s_dev_sysram = NULL;

int init_module_mtk_wrapper_cv(void)
{
	struct device_node* node=NULL;
	struct platform_device *pdev;

	for_each_compatible_node(node, NULL, "mediatek,mt3612-mm_sysram"){
		pdev = of_find_device_by_node(node);
		if (strstr(pdev->name, "mm_sysram2")) {
			s_dev_sysram = &pdev->dev;
		}
	}
	if (s_dev_sysram == NULL) {
		pr_err("[%s] Can not find system2\n", __FUNCTION__);
		return -1;
	}

	warpa_fe_pdev = pdev_find_dt("sie,warpa-fe");
	if (warpa_fe_pdev == NULL) {
		pr_err("[%s] can't find warpa-fe.\n", __FUNCTION__);
		return -1;
	}
	fm_pdev = pdev_find_dt("sie,fm-dev");
	if (fm_pdev == NULL) {
		pr_err("[%s] can't find fm-dev.\n", __FUNCTION__);
		return -1;
	}


	/* sysram power on */
	if (mtk_mm_sysram_power_on(s_dev_sysram, SYSRAM2_START_ADDRESS, SYSRAM2_SIZE) != 0) {
		dev_err(s_dev_sysram, "Can't Power On SYSRAM2: 0x%x - 0x%x\n" ,
				SYSRAM2_START_ADDRESS, SYSRAM2_START_ADDRESS + SYSRAM2_SIZE);
		return -1;
	}
	mtk_warpa_fe_probe(warpa_fe_pdev);
	mtk_fm_dev_probe(fm_pdev);

	pr_info("%s: success.\n", __FUNCTION__);

	return 0;
}

void cleanup_module_mtk_wrapper_cv(void)
{
	pr_info("%s start\n", __FUNCTION__);

	mtk_fm_dev_remove(fm_pdev);
	mtk_warpa_fe_remove(warpa_fe_pdev);

	/* sysram power off */
	if (mtk_mm_sysram_power_off(s_dev_sysram, SYSRAM2_START_ADDRESS, SYSRAM2_SIZE) != 0) {
		dev_err(s_dev_sysram, "Can't Power Off SYSRAM2: 0x%x - 0x%x\n" ,
				SYSRAM2_START_ADDRESS, SYSRAM2_START_ADDRESS + SYSRAM2_SIZE);
	}
}
