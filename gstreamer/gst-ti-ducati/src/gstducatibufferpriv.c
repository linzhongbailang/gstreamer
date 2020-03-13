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

#include <omap_drm.h>
#include <omap_drmif.h>
#include <xf86drmMode.h>

#include "gstducatibufferpriv.h"

static GType gst_meta_ducati_buffer_priv_api_get_type (void);
#define GST_META_DUCATI_BUFFER_PRIV_API_TYPE (gst_meta_ducati_buffer_priv_api_get_type())

static const GstMetaInfo *gst_meta_ducati_buffer_priv_get_info (void);
#define GST_META_DUCATI_BUFFER_PRIV_INFO (gst_meta_ducati_buffer_priv_get_info())

#define GST_META_DUCATI_BUFFER_PRIV_GET(buf) ((GstMetaDucatiBufferPriv *)gst_buffer_get_meta(buf,GST_META_DUCATI_BUFFER_PRIV_API_TYPE))
#define GST_META_DUCATI_BUFFER_PRIV_ADD(buf) ((GstMetaDucatiBufferPriv *)gst_buffer_add_meta(buf,GST_META_DUCATI_BUFFER_PRIV_INFO,NULL))


static gboolean
ducatibufferpriv_init_func (GstMeta * meta, gpointer params, GstBuffer * buffer)
{
  GST_DEBUG ("init called on buffer %p, meta %p", buffer, meta);
  /* nothing to init really, the init function is mostly for allocating
   * additional memory or doing special setup as part of adding the metadata to
   * the buffer*/
  return TRUE;
}

static void
ducatibufferpriv_free_func (GstMeta * meta, GstBuffer * buffer)
{
  GstMetaDucatiBufferPriv *priv = (GstMetaDucatiBufferPriv *) meta;
  /* Release the memory associated with priv */
  omap_bo_del (priv->bo);
}

static gboolean
ducatibufferpriv_transform_func (GstBuffer * transbuf, GstMeta * meta,
    GstBuffer * buffer, GQuark type, gpointer data)
{
  /* Nothing to be done. Returning FALSE signifies that it is not an allowed operation */
  return FALSE;
}

static GType
gst_meta_ducati_buffer_priv_api_get_type (void)
{
  static volatile GType type;
  static const gchar *tags[] = { "ducatibufferpriv", NULL };

  if (g_once_init_enter (&type)) {
    GType _type =
        gst_meta_api_type_register ("GstMetaDucatiBufferPrivAPI", tags);
    g_once_init_leave (&type, _type);
  }
  return type;
}

static const GstMetaInfo *
gst_meta_ducati_buffer_priv_get_info (void)
{
  static const GstMetaInfo *meta_ducati_buffer_priv_info = NULL;

  if (g_once_init_enter (&meta_ducati_buffer_priv_info)) {
    const GstMetaInfo *mi =
        gst_meta_register (GST_META_DUCATI_BUFFER_PRIV_API_TYPE,
        "GstMetaDucatiBufferPriv",
        sizeof (GstMetaDucatiBufferPriv),
        ducatibufferpriv_init_func, ducatibufferpriv_free_func,
        ducatibufferpriv_transform_func);
    g_once_init_leave (&meta_ducati_buffer_priv_info, mi);
  }
  return meta_ducati_buffer_priv_info;
}

/**
 * gst_ducati_buffer_priv_set:
 * @buf: a #GstBuffer
 * @bo: a omap_bo structure
 * @uv_offset: the uv offset
 * @size: the size
 *
 * Adds a GstMetaDucatiBufferPriv metadata to the buffer. The @bo, @uv_offset and @size are also set in the metadata added 
 * 
 * Returns: the #GstMetaDucatiBufferPriv set on the @buf
 *
 * Since: 1.2.?
 */

GstMetaDucatiBufferPriv *
gst_ducati_buffer_priv_set (GstBuffer * buf, struct omap_bo * bo,
    gint uv_offset, gint size)
{
  GstMetaDucatiBufferPriv *priv;
  priv = GST_META_DUCATI_BUFFER_PRIV_ADD (buf);
  if (priv) {
    priv->bo = bo;
    priv->uv_offset = uv_offset;
    priv->size = size;
  }
  return priv;
}

/**
 * gst_ducati_buffer_priv_get:
 * @buf: a #GstBuffer
 *
 * Get the GstMetaDucatiBufferPriv metadata that has previously been attached to a buffer
 * with gst_ducati_buffer_priv_set(), usually by another element
 * upstream.
 * 
 * Returns: the #GstMetaDucatiBufferPriv previously set on @buf
 *
 * Since: 1.2.?
 */
GstMetaDucatiBufferPriv *
gst_ducati_buffer_priv_get (GstBuffer * buf)
{
  GstMetaDucatiBufferPriv *priv;
  priv = GST_META_DUCATI_BUFFER_PRIV_GET (buf);
  return priv;
}
