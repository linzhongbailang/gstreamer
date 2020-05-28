#ifndef __GST_DRMUTILS_H__
#define __GST_DRMUTILS_H__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <xf86drmMode.h>
#include <omap_drm.h>
#include <omap_drmif.h>
#include <drm_fourcc.h>
#include <gst/gst.h>

struct connector {
	uint32_t id;
	char mode_str[64];
	drmModeConnector *connector;
	drmModeModeInfo *mode;
	drmModeEncoder *encoder;
	uint32_t fb_id;
	struct omap_bo *fb_bo;
	int crtc;
	int pipe;
};

void gst_drm_connector_cleanup (int fd, struct connector * c);
gboolean gst_drm_connector_find_mode_and_plane (int fd,
    struct omap_device * dev, int width, int height,
    drmModeRes * resources, drmModePlaneRes * plane_resources,
    struct connector *c, drmModePlane ** out_plane);
gboolean gst_drm_connector_find_mode_and_plane_by_name (int fd,
    struct omap_device *dev, int width, int height,
    drmModeRes * resources, drmModePlaneRes * plane_resources,
    struct connector *c, const char *name,
    drmModePlane ** out_plane);

#endif /* __GST_DRMUTILS_H__ */
