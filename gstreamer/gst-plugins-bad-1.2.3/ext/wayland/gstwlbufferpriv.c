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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdint.h>
#include <gst/gst.h>
#include <gst/dmabuf/dmabuf.h>
#include <gst/video/gstvideometa.h>

#include <omap_drm.h>
#include <omap_drmif.h>

#include "gstwaylandsink.h"
#include "gstwlbufferpriv.h"
#include "wayland-drm-client-protocol.h"


/* Create planar wl_buffer that can be given to waylandsink.
 * Crop info is also used */
static int
create_wl_buffer (GstWLBufferPriv * priv, GstWaylandSink * sink,
    GstBuffer * buf)
{
  GstVideoCropMeta *crop;
  gint video_width = sink->video_width;
  gint video_height = sink->video_height;

  /* TODO get format, etc from caps.. and query device for
   * supported formats, and make this all more flexible to
   * cope with various formats:
   */
  uint32_t fourcc = GST_MAKE_FOURCC ('N', 'V', '1', '2');
  uint32_t name;

  /* note: wayland and mesa use the terminology:
   *    stride - rowstride in bytes
   *    pitch  - rowstride in pixels
   */
  uint32_t strides[3] = {
    GST_ROUND_UP_4 (sink->video_width), GST_ROUND_UP_4 (sink->video_width), 0,
  };
  uint32_t offsets[3] = {
    0, strides[0] * sink->video_height, 0
  };

  crop = gst_buffer_get_video_crop_meta (buf);
  if (crop) {
    guint left, top;
    left = crop->y;
    top = crop->x;

    offsets[0] = left;
    offsets[1] += (video_width * top / 2) + left;
    if(crop->width)
     video_width = crop->width;
  }

  if (omap_bo_get_name (priv->bo, &name)) {
    GST_WARNING_OBJECT (sink, "could not get name");
    return -1;
  }

 GST_LOG_OBJECT (sink,"width = %d , height = %d , fourcc = %d ",  video_width, video_height, fourcc );

  priv->buffer = wl_drm_create_planar_buffer (sink->display->drm, name,
      video_width, video_height, fourcc,
      offsets[0], strides[0],
      offsets[1], strides[1],
      offsets[2], strides[2]);

  GST_DEBUG_OBJECT (sink, "create planar buffer: %p (name=%d)",
      priv->buffer, name);

  return priv->buffer ? 0 : -1;
}


/**
 * gst_wl_buffer_priv:
 * @sink: a #GstWaylandSink
 * @buf: a pointer to #GstBuffer
 *
 * Checks if the @buf has a GstMetaDmaBuf metadata set. If it doesn't we return a NULL
 * indicating its not a dmabuf buffer. We maintain a hashtable with dmabuf fd as key and 
 * the GstWLBufferPriv structure as value
 *
 * Returns: the #GstWLBufferPriv
 *
 * Since: 1.2.?
 */
GstWLBufferPriv *
gst_wl_buffer_priv (GstWaylandSink * sink, GstBuffer * buf)
{
  
    GstMetaDmaBuf *dmabuf = gst_buffer_get_dma_buf_meta (buf);
    GstWLBufferPriv *priv;
    int fd,fd_copy;

    /* if it isn't a dmabuf buffer that we can import, then there
     * is nothing we can do with it:
     */
    if (!dmabuf) {
      GST_DEBUG_OBJECT (sink, "not importing non dmabuf buffer");
      return NULL;
    }
    fd = gst_dma_buf_meta_get_fd (dmabuf);
    fd_copy =fd;

     /* lookup the hashtable with fd as key. If present return bo & buffer structure */
    priv = g_hash_table_lookup (sink->wlbufferpriv, (gpointer)fd_copy);
    if(priv) {
       return priv;
     }

    priv = g_malloc0 (sizeof (GstWLBufferPriv));
    priv->bo = omap_bo_from_dmabuf (sink->display->dev, fd);

    if (create_wl_buffer (priv, sink, buf)) {
      GST_WARNING_OBJECT (sink, "could not create framebuffer: %s",
          strerror (errno));
      g_free(priv);
      return NULL;
    }

    /* if fd not present, write to hash table fd and the corresponding priv. */
    g_hash_table_insert(sink->wlbufferpriv, (gpointer)fd_copy, priv);    
    
   
  return priv;
}

