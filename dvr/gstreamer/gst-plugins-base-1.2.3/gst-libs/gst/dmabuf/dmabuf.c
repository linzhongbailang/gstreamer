/* GStreamer dmabuf
 *
 * Copyright (c) 2012, Texas Instruments Incorporated
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:gstdmabuf
 * @short_description: GStreamer dmabuf metadata support
 *
 * Since: 1.2.?
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>

#include "dmabuf.h"


static GType gst_meta_dma_buf_api_get_type (void);
#define GST_META_DMA_BUF_API_TYPE (gst_meta_dma_buf_api_get_type())

static const GstMetaInfo *gst_meta_dma_buf_get_info (void);
#define GST_META_DMA_BUF_INFO (gst_meta_dma_buf_get_info())

#define GST_META_DMA_BUF_GET(buf) ((GstMetaDmaBuf *)gst_buffer_get_meta(buf,GST_META_DMA_BUF_API_TYPE))
#define GST_META_DMA_BUF_ADD(buf) ((GstMetaDmaBuf *)gst_buffer_add_meta(buf,GST_META_DMA_BUF_INFO,NULL))


static gboolean
dmabuf_init_func (GstMeta * meta, gpointer params, GstBuffer * buffer)
{
   GST_DEBUG ("init called on buffer %p, meta %p", buffer, meta);
  /* nothing to init really, the init function is mostly for allocating
   * additional memory or doing special setup as part of adding the metadata to
   * the buffer*/
  return TRUE;
}

static void
dmabuf_free_func (GstMeta * meta, GstBuffer * buffer)
{
   /* close the file descriptor associated with the buffer, 
    * that is stored as metadata*/
   GstMetaDmaBuf *dmabuf = (GstMetaDmaBuf *) meta;
   GST_DEBUG ("free called on buffer %p, meta %p", buffer, meta);
   close (dmabuf->fd);
}

static gboolean
dmabuf_transform_func (GstBuffer * transbuf, GstMeta * meta,
    GstBuffer * buffer, GQuark type, gpointer data)
{
  GstMetaDmaBuf *transmeta, *tmeta = (GstMetaDmaBuf *) meta;

  GST_DEBUG ("transform %s called from buffer %p to %p, meta %p",
     g_quark_to_string (type), buffer, transbuf, meta);

  if (GST_META_TRANSFORM_IS_COPY (type)) {
    GstMetaTransformCopy *copy_data = data;
    
    if (!copy_data->region){
    /* only copy if the complete data is copied as well */
       transmeta = GST_META_DMA_BUF_ADD (transbuf);
       if(!transmeta) {
          return FALSE;
       }
    /* create a copy of the file descriptor*/
       transmeta->fd = dup(tmeta->fd);
    }
  }
  return TRUE;
}

static GType
gst_meta_dma_buf_api_get_type (void)
{
  static volatile GType type;
  static const gchar *tags[] = { "dmabuf", NULL };

  if (g_once_init_enter (&type)) {
    GType _type = gst_meta_api_type_register ("GstMetaDmaBufAPI", tags);
    g_once_init_leave (&type, _type);
  }
  return type;
}

static const GstMetaInfo *
gst_meta_dma_buf_get_info (void)
{
  static const GstMetaInfo *meta_dma_buf_info = NULL;

  if (g_once_init_enter (&meta_dma_buf_info)) {
    const GstMetaInfo *mi = gst_meta_register (GST_META_DMA_BUF_API_TYPE,
        "GstMetaDmaBuf",
        sizeof (GstMetaDmaBuf),
        dmabuf_init_func, dmabuf_free_func, dmabuf_transform_func);
    g_once_init_leave (&meta_dma_buf_info, mi);
  }
  return meta_dma_buf_info;
}



/**
 * gst_buffer_add_dma_buf_meta:
 * @buf: a #GstBuffer
 * @fd: the associated file descriptor to be added as metadata
 *
 * Adds a GstMetaDmaBuf metadata to the buffer. The @fd is also set in the metadata added 
 * 
 * Returns: the #GstMetaDmaBuf on @buf
 *
 * Since: 1.2.?
 */

GstMetaDmaBuf *
gst_buffer_add_dma_buf_meta (GstBuffer * buf, int fd, void *virt_addr, void *phy_addr)
{
  GstMetaDmaBuf *dmabuf;
  dmabuf = GST_META_DMA_BUF_ADD (buf);
  if (dmabuf)
  {
     dmabuf->fd = fd;
	 dmabuf->virt_addr = virt_addr;
	 dmabuf->phy_addr = phy_addr;
  }
  return dmabuf;
}

/**
 * gst_buffer_get_dma_buf_meta:
 * @buf: a #GstBuffer
 *
 * Get the GstMetaDmaBuf metadata that has previously been attached to a buffer
 * with gst_buffer_add_dma_buf_meta(), usually by another element
 * upstream.
 *
 * Returns: the #GstMetaDmaBuf attached to @buf
 *
 * Since: 1.2.?
 */
GstMetaDmaBuf *
gst_buffer_get_dma_buf_meta (GstBuffer * buf)
{
  GstMetaDmaBuf * dmabuf;
  dmabuf = GST_META_DMA_BUF_GET (buf);
  return dmabuf;
}


/**
 * gst_dma_buf_meta_get_fd:
 * @dmabuf: a #GstMetaDmaBuf
 *
 * Returns: the file descriptor set as GstMetaDmaBuf metadata 
 * that has previously been attached to a buffer
 * with gst_buffer_add_dma_buf_meta()
 *
 * Since: 1.2.?
 */
int
gst_dma_buf_meta_get_fd (GstMetaDmaBuf * dmabuf)
{
  if(dmabuf) {
    return dmabuf->fd;
  } else {
    GST_DEBUG("Received Null parameter for GstMetaDmaBuf ");
    return 0;
  }
}

void *gst_dma_buf_meta_get_phy_addr(GstMetaDmaBuf * dmabuf)
{
  if(dmabuf) {
    return dmabuf->phy_addr;
  } else {
    GST_DEBUG("Received Null parameter for GstMetaDmaBuf ");
    return NULL;
  }
}

void *gst_dma_buf_meta_get_virt_addr(GstMetaDmaBuf * dmabuf)
{
  if(dmabuf) {
    return dmabuf->virt_addr;
  } else {
    GST_DEBUG("Received Null parameter for GstMetaDmaBuf ");
    return NULL;
  }
}