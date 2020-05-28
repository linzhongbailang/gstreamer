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

#include <omap_drm.h>
#include <omap_drmif.h>
#include <xf86drmMode.h>

#include "gstkmssink.h"
#include "gstkmsbufferpriv.h"

static int
create_fb (GstKMSBufferPriv * priv, GstKMSSink * sink)
{
  /* TODO get format, etc from caps.. and query device for
   * supported formats, and make this all more flexible to
   * cope with various formats:
   */
  uint32_t fourcc = GST_MAKE_FOURCC ('N', 'V', '1', '2');

  uint32_t handles[4] = {
    omap_bo_handle (priv->bo), omap_bo_handle (priv->bo),
  };
  uint32_t pitches[4] = {
    GST_ROUND_UP_4 (sink->input_width), GST_ROUND_UP_4 (sink->input_width),
  };
  uint32_t offsets[4] = {
    0, pitches[0] * sink->input_height
  };

  return drmModeAddFB2 (priv->fd, sink->input_width, sink->input_height,
      fourcc, handles, pitches, offsets, &priv->fb_id, 0);
}

/**
 * gst_kms_buffer_priv:
 * @sink: a #GstKMSSink
 * @buf: a pointer to #GstBuffer
 *
 * Checks if the @buf has a GstMetaDmaBuf metadata set. If it doesn't we return a NULL
 * indicating its not a dmabuf buffer. We maintain a hashtable with dmabuf fd as key and 
 * the GstKMSBufferPriv structure as value
 *
 * Returns: the #GstKMSBufferPriv
 *
 * Since: 1.2.?
 */
GstKMSBufferPriv *
gst_kms_buffer_priv (GstKMSSink * sink, GstBuffer * buf)
{
    GstMetaDmaBuf *dmabuf = gst_buffer_get_dma_buf_meta (buf);


    struct omap_bo *bo;
    int fd;
    int fd_copy;
    GstKMSBufferPriv * priv;

    /* if it isn't a dmabuf buffer that we can import, then there
     * is nothing we can do with it:
     */
   
    if (!dmabuf) {
      GST_DEBUG_OBJECT (sink, "not importing non dmabuf buffer");
      return NULL;
    }

    fd_copy = gst_dma_buf_meta_get_fd (dmabuf);

    /* lookup the hashtable with fd as key. If present return bo & buffer structure */
    priv = g_hash_table_lookup (sink->kmsbufferpriv, (gpointer)fd_copy);
    if(priv) {
       return priv;
     }

    priv = g_malloc0 (sizeof (GstKMSBufferPriv));
    bo = omap_bo_from_dmabuf (sink->dev, fd_copy);
    fd = sink->fd;

      priv->bo = bo;
      priv->fd = fd;

    if (create_fb (priv, sink)) {
      GST_WARNING_OBJECT (sink, "could not create framebuffer: %s",
          strerror (errno));
      g_free(priv);
      return NULL;
    }

    /* if fd not present, write to hash table fd and the corresponding priv. */
    g_hash_table_insert(sink->kmsbufferpriv, (gpointer)fd_copy, priv); 
   
  
  return priv;
}
