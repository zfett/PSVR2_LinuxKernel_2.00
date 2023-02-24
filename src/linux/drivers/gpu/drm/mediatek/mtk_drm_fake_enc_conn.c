/*
 * Copyright (c) 2017 MediaTek Inc.
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

#include <drm/drmP.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>

#include "mtk_drm_ddp.h"
#include "mtk_drm_ddp_comp.h"
#include "mtk_drm_drv.h"
#include "mtk_drm_gem.h"
#include "mtk_drm_fb.h"
#include "mtk_drm_debugfs.h"
#include "mtk_drm_fake_enc_conn.h"

static const u32 mtk_fake_output_formats[] = {
	DRM_FORMAT_RGB888,
};

static enum drm_connector_status mtk_fake_connector_detect(
	struct drm_connector *connector, bool force)
{
	/*return connector_status_disconnected;*/
	return connector_status_connected;
}

static int mtk_fake_connector_get_modes(struct drm_connector *connector)
{
	struct drm_device *dev = connector->dev;

	return drm_add_modes_noedid(connector, dev->mode_config.max_width,
				    dev->mode_config.max_height);
}

static enum drm_mode_status mtk_fake_connector_mode_valid(
	struct drm_connector *connector, struct drm_display_mode *mode)
{
	struct drm_device *dev = connector->dev;
	struct drm_mode_config *mode_config = &dev->mode_config;
	int w = mode->hdisplay, h = mode->vdisplay;

	if ((w < mode_config->min_width) || (w > mode_config->max_width)) {
		drm_err("w %d (%d %d) is invalid\n",
		w,
		mode_config->min_width,
		mode_config->max_width);
		return MODE_BAD_HVALUE;
	}

	if ((h < mode_config->min_height) || (h > mode_config->max_height)) {
		drm_err("h %d (%d %d) is invalid\n",
		w,
		mode_config->min_height,
		mode_config->max_height);
		return MODE_BAD_VVALUE;
	}

	return MODE_OK;
}

static int mtk_fake_atomic_check(struct drm_encoder *encoder,
				struct drm_crtc_state *crtc_state,
				struct drm_connector_state *conn_state)
{
	return 0;
}

static void mtk_fake_encoder_disable(struct drm_encoder *encoder)
{

}

static const struct drm_encoder_helper_funcs mtk_fake_encoder_helper_funcs = {
	.atomic_check = mtk_fake_atomic_check,
	.disable = mtk_fake_encoder_disable,
};

static const struct drm_connector_funcs mtk_fake_connector_funcs = {
	.dpms = drm_atomic_helper_connector_dpms,
	.detect = mtk_fake_connector_detect,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = drm_connector_cleanup,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

static const struct drm_connector_helper_funcs
	mtk_fake_connector_helper_funcs = {
	.get_modes = mtk_fake_connector_get_modes,
	.mode_valid = mtk_fake_connector_mode_valid,
};

int mtk_fake_set_possible_crtcs(struct drm_device *drm_dev,
			      struct mtk_drm_crtc *mtk_crtc,
			      struct mtk_ddp_comp *comp)
{
	struct drm_fake_connector *fake_connector = &mtk_crtc->fake_connector;
	unsigned int *possible_crtcs = &fake_connector->encoder.possible_crtcs;

	*possible_crtcs = 1;

	if (*possible_crtcs == 0) {
		drm_err("Failed to set fake possible_crtcs\n");
		return -1;
	}

	return 0;
}


static const struct drm_encoder_funcs drm_fake_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};
/**
 * drm_fake_connector_init - Initialize a fake connector and its properties
 * @dev: DRM device
 * @wb_connector: fake connector to initialize
 * @con_funcs: Connector funcs vtable
 * @enc_helper_funcs: Encoder helper funcs vtable to
 *   be used by the internal encoder
 * @formats: Array of supported pixel formats for the fake engine
 * @n_formats: Length of the formats array
 *
 * This function creates the fake-connector-specific properties if they
 * have not been already created, initializes the connector as
 * type DRM_MODE_CONNECTOR_FAKE, and correctly initializes the property
 * values. It will also create an internal encoder associated with the
 * drm_fake_connector and set it to use the @enc_helper_funcs vtable for
 * the encoder helper.
 *
 * Drivers should always use this function instead of drm_connector_init() to
 * set up fake connectors.
 *
 * Returns: 0 on success, or a negative error code
 */
int drm_fake_connector_init(struct drm_device *dev,
		 struct drm_fake_connector *fake_connector,
		 const struct drm_connector_funcs *con_funcs,
		 const struct drm_encoder_helper_funcs *enc_helper_funcs,
		 const u32 *formats, int n_formats)
{
	int ret;
	struct drm_property_blob *blob;
	struct drm_connector *connector = &fake_connector->base;
	/*struct drm_mode_config *config = &dev->mode_config;*/
	blob = drm_property_create_blob(dev, n_formats * sizeof(*formats),
					formats);
	if (IS_ERR(blob)) {
		drm_err("create blob fail!\n");
		return PTR_ERR(blob);
	}

	drm_encoder_helper_add(&fake_connector->encoder, enc_helper_funcs);
	ret = drm_encoder_init(dev, &fake_connector->encoder,
			       &drm_fake_encoder_funcs,
			       DRM_MODE_ENCODER_VIRTUAL);
	if (ret) {
		drm_err("encoder helper add fail!\n");
		goto fail;
	}

	connector->interlace_allowed = 0;
	fake_connector->encoder.possible_crtcs = 1;
	ret = drm_connector_init(dev, connector, con_funcs,
				 DRM_MODE_CONNECTOR_VIRTUAL);
	if (ret) {
		drm_err("fake connector init fail!\n");
		goto connector_fail;
	}
	ret = drm_mode_connector_attach_encoder(connector,
						&fake_connector->encoder);
	if (ret) {
		drm_err("fake connector attach encoder fail!\n");
		goto attach_fail;
	}
	/*
	*drm_object_attach_property(&connector->base,
	*			   config->writeback_pixel_formats_property,
	*			   blob->base.id);
	*fake_connector->pixel_formats_blob_ptr = blob;
	*/

	return 0;

attach_fail:
	drm_connector_cleanup(connector);
connector_fail:
	drm_encoder_cleanup(&fake_connector->encoder);
fail:
	drm_property_unreference_blob(blob);
	return ret;
}


struct drm_fake_connector _fake_connector;
int mtk_fake_connector_init(struct drm_device *drm_dev)
{
	int ret;
	struct drm_fake_connector *fake_connector = &_fake_connector;

	ret = drm_fake_connector_init(drm_dev, fake_connector,
				   &mtk_fake_connector_funcs,
				   &mtk_fake_encoder_helper_funcs,
				   mtk_fake_output_formats,
				   (int)ARRAY_SIZE(mtk_fake_output_formats));
	if (ret != 0) {
		drm_err("fake connector init fail\n");
		return ret;
	}

	drm_connector_helper_add(&fake_connector->base,
				 &mtk_fake_connector_helper_funcs);

	return 0;
}


