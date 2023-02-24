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

#ifndef MTK_DRM_CRTC_H
#define MTK_DRM_CRTC_H

#include <drm/drm_crtc.h>
#include "mtk_drm_ddp_comp.h"
#include "mtk_drm_plane.h"

#define OVL_LAYER_NR	4
#define MTK_LUT_SIZE	512
#define MTK_MAX_BPC	10
#define MTK_MIN_BPC	3

#ifdef MTK_DRM_VIRTUAL_SUPPORT
struct drm_fake_connector {
	struct drm_connector base;

	/**
	 * @encoder: Internal encoder used by the connector to fulfill
	 * the DRM framework requirements. The users of the
	 * @drm_writeback_connector control the behaviour of the @encoder
	 * by passing the @enc_funcs parameter to drm_writeback_connector_init()
	 * function.
	 */
	struct drm_encoder encoder;

	/**
	 * @pixel_formats_blob_ptr:
	 *
	 * DRM blob property data for the pixel formats list on writeback
	 * connectors
	 * See also drm_writeback_connector_init()
	 */
	struct drm_property_blob *pixel_formats_blob_ptr;
};
#endif
/**
 * struct mtk_drm_crtc - MediaTek specific crtc structure.
 * @base: crtc object.
 * @enabled: records whether crtc_enable succeeded
 * @planes: array of 4 drm_plane structures, one for each overlay plane
 * @config_regs: memory mapped mmsys configuration register space
 * @mutex: handle to one of the ten disp_mutex streams
 * @ddp_comp_nr: number of components in ddp_comp
 * @ddp_comp: array of pointers the mtk_ddp_comp structures used by this crtc
 */
struct mtk_drm_crtc {
	struct drm_crtc			base;
	bool				enabled;

	bool				pending_needs_vblank;
	struct drm_pending_vblank_event	*event;

	struct cmdq_client		*cmdq_client;
	int				cmdq_event;

	struct drm_plane		planes[OVL_LAYER_NR];

	void __iomem			*config_regs;
	struct mtk_disp_mutex		*mutex;
	unsigned int			ddp_comp_nr;
	struct mtk_ddp_comp		**ddp_comp;

#ifdef MTK_DRM_VIRTUAL_SUPPORT
	struct drm_fake_connector	fake_connector;
#endif
};

int mtk_drm_crtc_enable_vblank(struct drm_device *drm, unsigned int pipe);
void mtk_drm_crtc_disable_vblank(struct drm_device *drm, unsigned int pipe);
void mtk_crtc_vblank_irq(struct drm_crtc *crtc);
int mtk_drm_crtc_create(struct drm_device *drm_dev,
			const enum mtk_ddp_comp_id *path,
			unsigned int path_len);

void mtk_drm_crtc_plane_update(struct drm_crtc *crtc, struct drm_plane *plane,
			       struct mtk_plane_pending_state *pending);

#endif /* MTK_DRM_CRTC_H */
