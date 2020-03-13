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

#ifndef __GSTDUCATIBUFFERPRIV_H__
#define __GSTDUCATIBUFFERPRIV_H__

#include <stdint.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#include <gst/dmabuf/dmabuf.h>

/**
 * GstMetaDucatiBufferPriv:
 *
 * A structure that enables setting required metadata on a buffer.
 *
 * Since: 1.2.?
 */
typedef struct
{
  GstMeta meta;

  struct omap_bo *bo;
  gint uv_offset, size;

} GstMetaDucatiBufferPriv;


/* add the GstMetaDucatiBufferPriv metadata */
GstMetaDucatiBufferPriv * gst_ducati_buffer_priv_set (GstBuffer * buf,  struct omap_bo *bo, gint uv_offset, gint size);
/* retrieve the GstMetaDucatiBufferPriv metadata */
GstMetaDucatiBufferPriv * gst_ducati_buffer_priv_get (GstBuffer * buf);


G_END_DECLS


#endif /* __GSTDUCATIBUFFERPRIV_H__ */
