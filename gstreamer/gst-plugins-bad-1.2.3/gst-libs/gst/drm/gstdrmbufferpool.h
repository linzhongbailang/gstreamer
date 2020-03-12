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

#ifndef __GSTDRMBUFFERPOOL_H__
#define __GSTDRMBUFFERPOOL_H__

#include <gst/gst.h>
#include <gst/video/gstvideometa.h>
G_BEGIN_DECLS

/* TODO replace dependency on libdrm_omap w/ libdrm.. the only thing
 * missing is way to allocate buffers, but this should probably be
 * done via libdrm?
 *
 * NOTE: this dependency is only for those who want to subclass us,
 * so we could perhaps move the struct definitions into a separate
 * header or split out private ptr and move that into the .c file..
 */
#include <stdint.h>
#include <omap_drm.h>
#include <omap_drmif.h>

#define GST_TYPE_DRM_BUFFER_POOL (gst_drm_buffer_pool_get_type())
#define GST_IS_DRM_BUFFER_POOL(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_DRM_BUFFER_POOL))
#define GST_DRM_BUFFER_POOL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_DRM_BUFFER_POOL, GstDRMBufferPool))
#define GST_DRM_BUFFER_POOL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_DRM_BUFFER_POOL, GstDRMBufferPoolClass))
#define GST_DRM_BUFFER_POOL_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_DRM_BUFFER_POOL, GstDRMBufferPoolClass))


typedef struct _GstDRMBufferPool GstDRMBufferPool;
typedef struct _GstDRMBufferPoolClass GstDRMBufferPoolClass;


typedef struct
{
  GstMeta meta;
  struct omap_bo *bo;
} GstMetaDRMBuffer;


/*
 * GstDRMBufferPool:
 */

struct _GstDRMBufferPool {
  GstBufferPool parent;

  int fd;
  struct omap_device *dev;

  /* output (padded) size including any codec padding: */
  guint size;
  gint width, height;

  /* Video info obtained from caps */
  GstVideoInfo info;
  
  GstCaps         *caps;
  GstElement      *element;  /* the element that owns us.. */

#ifndef GST_DISABLE_GST_DEBUG
  guint            nbbufs;
#endif /* DEBUG */

  /* TODO add reserved */
};

struct _GstDRMBufferPoolClass {
  GstBufferPoolClass klass;

  /* allow the subclass to allocate it's own buffers that extend
   * GstDRMBuffer:
   */
  GstFlowReturn  (*alloc_buffer)(GstBufferPool * pool, GstBuffer ** buffer,
    GstBufferPoolAcquireParams * params);

  /* The a buffer subclass should not override finalize, as that
   * would interfere with reviving the buffer and returning to the
   * pool.  Instead you can implement this vmethod to cleanup a
   * buffer.
   */
  void (*buffer_cleanup)(GstDRMBufferPool * pool, GstBuffer *buf);

  /* Called when a buffer is added back to the pool after its last
   * ref has been removed.
   */
  void (*buffer_pooled)(GstDRMBufferPool * pool, GstBuffer *buf);

  /* TODO add reserved */
};

GType gst_drm_buffer_pool_get_type (void);

void gst_drm_buffer_pool_initialize (GstDRMBufferPool * self,
    GstElement * element, int fd, GstCaps * caps, guint size);

/* to set/change the config of pool */
gboolean gst_drm_buffer_pool_set_config (GstBufferPool * pool, GstStructure * config);

/* create a new drm buffer pool */
GstDRMBufferPool * gst_drm_buffer_pool_new (GstElement * element,
    int fd, GstCaps * caps, guint size);

/* unref the drm buffer pool */
void gst_drm_buffer_pool_destroy (GstDRMBufferPool * self);

/* size of buffers in the pool */
guint gst_drm_buffer_pool_size (GstDRMBufferPool * self);

/* check the present caps of the pool */
gboolean gst_drm_buffer_pool_check_caps (GstDRMBufferPool * self,
    GstCaps * caps);

/* get a buffer from the pool */
GstBuffer * gst_drm_buffer_pool_get (GstDRMBufferPool * self,
    gboolean force_alloc);

/* release a buffer to the pool */
gboolean gst_drm_buffer_pool_put (GstDRMBufferPool * self, GstBuffer * buf);

G_END_DECLS

#endif /* __GSTDRMBUFFERPOOL_H__ */
