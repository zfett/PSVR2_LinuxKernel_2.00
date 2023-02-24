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
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/time.h>
#include <linux/spinlock.h>
#include <linux/capability.h>

#include <sce_km_defs.h>

#define MTK_WRAPPER_DEV_NAME	"mktwrapperdrv"
static unsigned int mtk_wrapper_major = SCE_KM_CDEV_MAJOR_MTK_WRAPPER;

static const struct file_operations mtk_wrapper_fops = {
	.write			= NULL,
	.unlocked_ioctl = NULL,
	.mmap			= NULL,
	.open			= NULL,
	.release		= NULL,
};

#define CONFIG_MTK_WRAPPER_CV
#define MTK_WRAPPER_DPRX_ENABLE

#if (defined(CONFIG_DEBUG_FS) && defined(CONFIG_MTK_WRAPPER_DEBUG))
#define MTK_WRAPPER_DEBUG
#endif

#ifdef MTK_WRAPPER_DPRX_ENABLE
int init_module_mtk_wrapper_dprx(void);
#endif
int init_module_mtk_wrapper_mutex(void);
int init_module_mtk_wrapper_slicer(void);
int init_module_mtk_wrapper_mmsyscfg(void);
int init_module_mtk_wrapper_wdma(void);
int init_module_mtk_wrapper_dsi(void);
int init_module_mtk_wrapper_dscenc(void);
int init_module_mtk_wrapper_rdma(void);
int init_module_mtk_wrapper_resizer(void);
int init_module_mtk_wrapper_crop(void);
int init_module_mtk_wrapper_p2s(void);
int init_module_mtk_wrapper_lhc(void);
int init_module_mtk_wrapper_ts(void);
int init_module_mtk_wrapper_gce(void);
#ifdef CONFIG_MTK_WRAPPER_CV
int init_module_mtk_wrapper_cv(void);
#endif
#ifdef MTK_WRAPPER_DEBUG
int init_module_mtk_wrapper_debugfs(void);
#endif
int init_module_mtk_wrapper_thermal(void);

#ifdef MTK_WRAPPER_DPRX_ENABLE
void cleanup_module_mtk_wrapper_dprx(void);
#endif
void cleanup_module_mtk_wrapper_mutex(void);
void cleanup_module_mtk_wrapper_slicer(void);
void cleanup_module_mtk_wrapper_mmsyscfg(void);
void cleanup_module_mtk_wrapper_wdma(void);
void cleanup_module_mtk_wrapper_dsi(void);
void cleanup_module_mtk_wrapper_dscenc(void);
void cleanup_module_mtk_wrapper_rdma(void);
void cleanup_module_mtk_wrapper_resizer(void);
void cleanup_module_mtk_wrapper_crop(void);
void cleanup_module_mtk_wrapper_p2s(void);
void cleanup_module_mtk_wrapper_lhc(void);
void cleanup_module_mtk_wrapper_ts(void);
void cleanup_module_mtk_wrapper_gce(void);
#ifdef CONFIG_MTK_WRAPPER_CV
void cleanup_module_mtk_wrapper_cv(void);
#endif
#ifdef MTK_WRAPPER_DEBUG
void cleanup_module_mtk_wrapper_debugfs(void);
#endif
int cleanup_module_mtk_wrapper_thermal(void);

static int __init init_module_mtk_wrapper(void)
{
	int ret;

	pr_info("init_module_mtk_wrapper()\n");
	ret = register_chrdev(mtk_wrapper_major, MTK_WRAPPER_DEV_NAME, &mtk_wrapper_fops);
	if (ret < 0) {
		pr_err("log: alloc_chrdev failed(%d).\n", ret);
		return ret;
	}
	if (mtk_wrapper_major == 0) {
		mtk_wrapper_major = ret;
	}

	ret = init_module_mtk_wrapper_dsi();
	if (ret < 0) {
		pr_err("dsi initialize failed.\n");
		return ret;
	}
	ret = init_module_mtk_wrapper_dscenc();
	if (ret < 0) {
		pr_err("dscenc initialize failed.\n");
		return ret;
	}
#ifdef MTK_WRAPPER_DPRX_ENABLE
	ret = init_module_mtk_wrapper_dprx();
	if (ret < 0) {
		pr_err("dprx initialize failed.\n");
		return ret;
	}
#endif
	ret = init_module_mtk_wrapper_mutex();
	if (ret < 0) {
		pr_err("mutex initialize failed.\n");
		return ret;
	}
	ret = init_module_mtk_wrapper_slicer();
	if (ret < 0) {
		pr_err("slicer initialize failed.\n");
		return ret;
	}
	ret = init_module_mtk_wrapper_mmsyscfg();
	if (ret < 0) {
		pr_err("mmsyscfg initialize failed.\n");
		return ret;
	}
	ret = init_module_mtk_wrapper_wdma();
	if (ret < 0) {
		pr_err("wdma initialize failed.\n");
		return ret;
	}
	ret = init_module_mtk_wrapper_rdma();
	if (ret < 0) {
		pr_err("rdma initialize failed.\n");
		return ret;
	}
	ret = init_module_mtk_wrapper_resizer();
	if (ret < 0) {
		pr_err("resizer initialize failed.\n");
		return ret;
	}
	ret = init_module_mtk_wrapper_crop();
	if (ret < 0) {
		pr_err("crop initialize failed.\n");
		return ret;
	}
	ret = init_module_mtk_wrapper_p2s();
	if (ret < 0) {
		pr_err("p2s initialize failed.\n");
		return ret;
	}
	ret = init_module_mtk_wrapper_lhc();
	if (ret < 0) {
		pr_err("lhc initialize failed.\n");
		return ret;
	}
	ret = init_module_mtk_wrapper_ts();
	if (ret < 0) {
		pr_err("ts initialize failed.\n");
		return ret;
	}
	ret = init_module_mtk_wrapper_gce();
	if (ret < 0) {
		pr_err("gce initialize failed.\n");
		return ret;
	}
#ifdef CONFIG_MTK_WRAPPER_CV
	ret = init_module_mtk_wrapper_cv();
	if (ret < 0) {
		pr_err("cv initialize failed.\n");
		return ret;
	}
#endif

#ifdef MTK_WRAPPER_DEBUG
	ret = init_module_mtk_wrapper_debugfs();
	if (ret < 0) {
		pr_err("mtkwrapper debugfs initialize failed.\n");
	}
#endif
	ret = init_module_mtk_wrapper_thermal();
	if (ret < 0) {
		pr_err("mtkwrapper thermal initialize failed.\n");
		return ret;
	}
	return 0;
}

static void __exit cleanup_module_mtk_wrapper(void)
{
	cleanup_module_mtk_wrapper_crop();
	cleanup_module_mtk_wrapper_p2s();
	cleanup_module_mtk_wrapper_lhc();
	cleanup_module_mtk_wrapper_ts();
	cleanup_module_mtk_wrapper_gce();
	cleanup_module_mtk_wrapper_resizer();
	cleanup_module_mtk_wrapper_rdma();
	cleanup_module_mtk_wrapper_wdma();
	cleanup_module_mtk_wrapper_mmsyscfg();
	cleanup_module_mtk_wrapper_slicer();
	cleanup_module_mtk_wrapper_mutex();
#ifdef MTK_WRAPPER_DPRX_ENABLE
	cleanup_module_mtk_wrapper_dprx();
#endif
	cleanup_module_mtk_wrapper_dscenc();
	cleanup_module_mtk_wrapper_dsi();
#ifdef CONFIG_MTK_WRAPPER_CV
	cleanup_module_mtk_wrapper_cv();
#endif

#ifdef MTK_WRAPPER_DEBUG
	cleanup_module_mtk_wrapper_debugfs();
#endif

	cleanup_module_mtk_wrapper_thermal();

	pr_info("cleanup_module_mtk_wrapper()\n");
	unregister_chrdev(mtk_wrapper_major, MTK_WRAPPER_DEV_NAME);

}

module_init(init_module_mtk_wrapper);
module_exit(cleanup_module_mtk_wrapper);

MODULE_AUTHOR("Sony Interactive Entertainment Inc.");
MODULE_DESCRIPTION("epcot wrapper driver");
MODULE_LICENSE("GPL");

