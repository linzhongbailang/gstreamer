/*
 * GStreamer
 *
 * Copyright (C) 2012 Texas Instruments
 * Copyright (C) 2012 Collabora Ltd
 *
 * Authors:
 *  Alessandro Decina <alessandro.decina@collabora.co.uk>
 *  Rob Clark <rob.clark@linaro.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __GSTWLBUFFERPRIV_H__
#define __GSTWLBUFFERPRIV_H__

#include <stdint.h>
#include <gst/gst.h>

#include <omap_drm.h>
#include <omap_drmif.h>

#include <wayland-client.h>

G_BEGIN_DECLS


typedef struct
{
  struct omap_bo *bo;
  struct wl_buffer *buffer;

}GstWLBufferPriv;


GType gst_wl_buffer_priv_get_type (void);

/* Returns a GstWLBufferPriv, if it has a dmabuf fd meatadata */
GstWLBufferPriv * gst_wl_buffer_priv (GstWaylandSink *sink, GstBuffer * buf);

G_END_DECLS


#endif /* __GSTWLBUFFERPRIV_H__ */
