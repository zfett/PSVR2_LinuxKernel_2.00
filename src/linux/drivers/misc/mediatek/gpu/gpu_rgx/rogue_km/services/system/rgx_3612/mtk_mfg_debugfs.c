/*
 * Copyright (c) 2019 MediaTek Inc.
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

#include <asm/io.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/hardirq.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <stdarg.h>

#include "img_types.h"
#include "pvr_debug.h"
#include "pvr_debugfs.h"
#include "pvr_uaccess.h"
#include "pvrsrv.h"

#include "mtk_mfg_debugfs.h"
#include "mtk_mfg_axictrl.h"
#include "mtk_mfg_sys.h"

#ifdef MTK_GPU_AXI_CTRL
static void *AxiCtrlSeqStart(struct seq_file *psSeqFile, loff_t *puiPosition)
{
	if (*puiPosition == 0)
		return psSeqFile->private;

	return NULL;
}

static void AxiCtrlSeqStop(struct seq_file *psSeqFile, void *pvData)
{
	PVR_UNREFERENCED_PARAMETER(psSeqFile);
	PVR_UNREFERENCED_PARAMETER(pvData);
}

static void *AxiCtrlSeqNext(struct seq_file *psSeqFile,
							   void *pvData,
							   loff_t *puiPosition)
{
	PVR_UNREFERENCED_PARAMETER(psSeqFile);
	PVR_UNREFERENCED_PARAMETER(pvData);
	PVR_UNREFERENCED_PARAMETER(puiPosition);

	return NULL;
}

static int AxiCtrlSeqShow(struct seq_file *psSeqFile, void *pvData)
{
	if (pvData != NULL) {
		IMG_UINT32 uiDebugLevel = *((IMG_UINT32 *)pvData);

		seq_printf(psSeqFile, "%u\n", uiDebugLevel);

		return 0;
	}

	return -EINVAL;
}

static const struct seq_operations gsAxiCtrlReadOps = {
	.start = AxiCtrlSeqStart,
	.stop = AxiCtrlSeqStop,
	.next = AxiCtrlSeqNext,
	.show = AxiCtrlSeqShow,
};

long gMTKAxiCtrlMode = 0x2;

static IMG_INT AxiCtrlSet(const char __user *pcBuffer,
							 size_t uiCount,
							 loff_t *puiPosition,
							 void *pvData)
{
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
	PVRSRV_DEVICE_CONFIG *psDevConfig =
		psPVRSRVData->psDeviceNodeList->psDevConfig;
	long *uiAxiMode = (long *)pvData;
	IMG_CHAR acDataBuffer[6];
	int ret = 0;

	if (puiPosition == NULL || *puiPosition != 0)
		return -EIO;

	if (uiCount > ARRAY_SIZE(acDataBuffer))
		return -EINVAL;

	if (pvr_copy_from_user(acDataBuffer, pcBuffer, uiCount)) {
		PVR_LOG(("pvr_copy_from_user failed!"));
		return -EINVAL;
	}

	if (uiCount == 0) {
		PVR_LOG(("uiCount is 0!"));
		return -EINVAL;
	}

	if (acDataBuffer[uiCount - 1] != '\n') {
		PVR_LOG(("no end of line!"));
		return -EINVAL;
	}

	acDataBuffer[uiCount - 1] = '\0';
	ret = kstrtol(acDataBuffer, 10, &gMTKAxiCtrlMode);
	if (ret != 0) {
		PVR_LOG(("kstrtol falied!(ret = %d)", ret));
		return -EINVAL;
	}

	mtk_mfg_set_aximode(psDevConfig, (int)gMTKAxiCtrlMode);
	(*uiAxiMode) = gMTKAxiCtrlMode;

	*puiPosition += uiCount;
	return uiCount;
}

static PPVR_DEBUGFS_ENTRY_DATA gpsAxiCtrlDebugFSEntry;

#endif /* ifdef MTK_GPU_AXI_CTRL */

#ifdef MTK_DVFS_CTRL
static void *DVFSCtrlSeqStart(struct seq_file *psSeqFile, loff_t *puiPosition)
{
	if (*puiPosition == 0)
		return psSeqFile->private;

	return NULL;
}

static void DVFSCtrlSeqStop(struct seq_file *psSeqFile, void *pvData)
{
	PVR_UNREFERENCED_PARAMETER(psSeqFile);
	PVR_UNREFERENCED_PARAMETER(pvData);
}

static void *DVFSCtrlSeqNext(struct seq_file *psSeqFile,
							   void *pvData,
							   loff_t *puiPosition)
{
	PVR_UNREFERENCED_PARAMETER(psSeqFile);
	PVR_UNREFERENCED_PARAMETER(pvData);
	PVR_UNREFERENCED_PARAMETER(puiPosition);

	return NULL;
}

static int DVFSCtrlSeqShow(struct seq_file *psSeqFile, void *pvData)
{
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
	PVRSRV_DEVICE_CONFIG *psDevConfig =
		psPVRSRVData->psDeviceNodeList->psDevConfig;
	IMG_DVFS_DEVICE *psDVFSDevice = &psDevConfig->sDVFS.sDVFSDevice;
	IMG_UINT32 enable = (IMG_UINT32)psDVFSDevice->bEnabled;

	PVR_UNREFERENCED_PARAMETER(pvData);

	seq_printf(psSeqFile, "%d\n", enable);

	return 0;
}

static const struct seq_operations gsDVFSCtrlReadOps = {
	.start = DVFSCtrlSeqStart,
	.stop = DVFSCtrlSeqStop,
	.next = DVFSCtrlSeqNext,
	.show = DVFSCtrlSeqShow,
};

static ssize_t DVFSCtrlSet(const char __user *pcBuffer,
							 size_t uiCount,
							 loff_t *puiPosition,
							 void *pvData)
{
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
	PVRSRV_DEVICE_CONFIG *psDevConfig =
		psPVRSRVData->psDeviceNodeList->psDevConfig;
	IMG_DVFS_DEVICE *psDVFSDevice = &psDevConfig->sDVFS.sDVFSDevice;
	long enable;
	IMG_CHAR acDataBuffer[6];
	int ret = 0;

	PVR_UNREFERENCED_PARAMETER(pvData);

	if (puiPosition == NULL || *puiPosition != 0)
		return -EIO;

	if (uiCount > ARRAY_SIZE(acDataBuffer))
		return -EINVAL;

	if (pvr_copy_from_user(acDataBuffer, pcBuffer, uiCount)) {
		PVR_LOG(("pvr_copy_from_user failed!"));
		return -EINVAL;
	}

	if (uiCount == 0) {
		PVR_LOG(("uiCount is 0!"));
		return -EINVAL;
	}

	if (acDataBuffer[uiCount - 1] != '\n') {
		PVR_LOG(("no end of line!"));
		return -EINVAL;
	}

	acDataBuffer[uiCount - 1] = '\0';
	ret = kstrtol(acDataBuffer, 10, &enable);
	if (ret != 0) {
		PVR_LOG(("kstrtol falied!(ret = %d)", ret));
		return -EINVAL;
	}

	if (enable == 0 || enable == 1) {
		psDVFSDevice->bEnabled = enable;
		*puiPosition += uiCount;

		PVR_LOG(("set DVFS enable:%ld", enable));
	}

	return uiCount;
}

static PPVR_DEBUGFS_ENTRY_DATA gpsDVFSCtrlDebugFSEntry;

static int DVFSOppTblSeqShow(struct seq_file *psSeqFile, void *pvData)
{
	PVR_UNREFERENCED_PARAMETER(pvData);

	mtk_mfg_show_opp_tbl(psSeqFile);

	return 0;
}

static const struct seq_operations gsDVFSOppTblReadOps = {
	.start = DVFSCtrlSeqStart,
	.stop = DVFSCtrlSeqStop,
	.next = DVFSCtrlSeqNext,
	.show = DVFSOppTblSeqShow,
};

static PPVR_DEBUGFS_ENTRY_DATA gpsDVFSOppTblDebugFSEntry;

static int DVFSCurOppSeqShow(struct seq_file *psSeqFile, void *pvData)
{
	PVR_UNREFERENCED_PARAMETER(pvData);

	mtk_mfg_show_cur_opp(psSeqFile);

	return 0;
}

static const struct seq_operations gsDVFSCurOppReadOps = {
	.start = DVFSCtrlSeqStart,
	.stop = DVFSCtrlSeqStop,
	.next = DVFSCtrlSeqNext,
	.show = DVFSCurOppSeqShow,
};

static PPVR_DEBUGFS_ENTRY_DATA gpsDVFSCurOppDebugFSEntry;

static int DVFSLoadingShow(struct seq_file *psSeqFile, void *pvData)
{
	int loading;

	PVR_UNREFERENCED_PARAMETER(pvData);

	loading = mtk_mfg_get_loading();
	seq_printf(psSeqFile, "%d\n", loading);

	return 0;
}

static const struct seq_operations gsDVFSLoadingReadOps = {
	.start = DVFSCtrlSeqStart,
	.stop = DVFSCtrlSeqStop,
	.next = DVFSCtrlSeqNext,
	.show = DVFSLoadingShow,
};

static PPVR_DEBUGFS_ENTRY_DATA gpsDVFSLoadingDebugFSEntry;

#endif /* ifdef MTK_DVFS_CTRL */

int mtk_create_debugfs_entry(void)
{
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
#if defined(MTK_GPU_AXI_CTRL) || defined(MTK_DVFS_CTRL)
	int iResult;
#endif

	PVR_ASSERT(psPVRSRVData != NULL);

#ifdef MTK_GPU_AXI_CTRL
	iResult = PVRDebugFSCreateEntry("axi_ctrl",
					NULL,
					&gsAxiCtrlReadOps,
					(PVRSRV_ENTRY_WRITE_FUNC *)AxiCtrlSet,
					NULL,
					NULL,
					&gMTKAxiCtrlMode,
					&gpsAxiCtrlDebugFSEntry);
	if (iResult != 0)
		goto ErrorRemoveAxiCtrlEntry;
#endif /* ifdef MTK_GPU_AXI_CTRL */

#ifdef MTK_DVFS_CTRL
	iResult = PVRDebugFSCreateEntry("dvfs_en",
					NULL,
					&gsDVFSCtrlReadOps,
					(PVRSRV_ENTRY_WRITE_FUNC *)DVFSCtrlSet,
					NULL,
					NULL,
					psPVRSRVData,
					&gpsDVFSCtrlDebugFSEntry);
	if (iResult != 0)
		goto ErrorRemoveDVFSCtrlEntry;

	iResult = PVRDebugFSCreateEntry("show_opp_tbl",
					NULL,
					&gsDVFSOppTblReadOps,
					NULL,
					NULL,
					NULL,
					psPVRSRVData,
					&gpsDVFSOppTblDebugFSEntry);
	if (iResult != 0)
		goto ErrorRemoveDVFSOppTblEntry;

	iResult = PVRDebugFSCreateEntry("show_cur_opp",
					NULL,
					&gsDVFSCurOppReadOps,
					NULL,
					NULL,
					NULL,
					psPVRSRVData,
					&gpsDVFSCurOppDebugFSEntry);
	if (iResult != 0)
		goto ErrorRemoveDVFSCurOppEntry;

	iResult = PVRDebugFSCreateEntry("show_loading",
					NULL,
					&gsDVFSLoadingReadOps,
					NULL,
					NULL,
					NULL,
					psPVRSRVData,
					&gpsDVFSLoadingDebugFSEntry);
	if (iResult != 0)
		goto ErrorRemoveDVFSLoadingEntry;

#endif /* ifdef MTK_DVFS_CTRL */

	return 0;

#ifdef MTK_GPU_AXI_CTRL
ErrorRemoveAxiCtrlEntry:
	PVRDebugFSRemoveEntry(&gpsAxiCtrlDebugFSEntry);
#endif

#ifdef MTK_DVFS_CTRL
ErrorRemoveDVFSCtrlEntry:
	PVRDebugFSRemoveEntry(&gpsDVFSCtrlDebugFSEntry);

ErrorRemoveDVFSOppTblEntry:
	PVRDebugFSRemoveEntry(&gpsDVFSCurOppDebugFSEntry);

ErrorRemoveDVFSCurOppEntry:
	PVRDebugFSRemoveEntry(&gpsDVFSOppTblDebugFSEntry);

ErrorRemoveDVFSLoadingEntry:
	PVRDebugFSRemoveEntry(&gpsDVFSLoadingDebugFSEntry);
#endif

#if defined(MTK_GPU_AXI_CTRL) || defined(MTK_DVFS_CTRL)
	return iResult;
#endif
}

void mtk_remove_debugfs_entry(void)
{
#ifdef MTK_GPU_AXI_CTRL
	if (gpsAxiCtrlDebugFSEntry != NULL)
		PVRDebugFSRemoveEntry(&gpsAxiCtrlDebugFSEntry);
#endif
#ifdef MTK_DVFS_CTRL
	if (gpsDVFSCtrlDebugFSEntry != NULL)
		PVRDebugFSRemoveEntry(&gpsDVFSCtrlDebugFSEntry);

	if (gpsDVFSOppTblDebugFSEntry != NULL)
		PVRDebugFSRemoveEntry(&gpsDVFSOppTblDebugFSEntry);

	if (gpsDVFSCurOppDebugFSEntry != NULL)
		PVRDebugFSRemoveEntry(&gpsDVFSCurOppDebugFSEntry);

	if (gpsDVFSLoadingDebugFSEntry != NULL)
		PVRDebugFSRemoveEntry(&gpsDVFSLoadingDebugFSEntry);
#endif
}

