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

/**
 * SECTION:GstDRMBufferPool
 * @short_description: GStreamer DRM buffer pool support
 *
 * Since: 1.2.?
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include <gst/dmabuf/dmabuf.h>

#include "gstdrmbufferpool.h"

GST_DEBUG_CATEGORY (drmbufferpool_debug);
#define GST_CAT_DEFAULT drmbufferpool_debug

static GstFlowReturn gst_drm_alloc_new_buffer (GstBufferPool * pool, GstBuffer ** buffer,
    GstBufferPoolAcquireParams * params);


#define gst_drm_buffer_pool_parent_class parent_class
G_DEFINE_TYPE (GstDRMBufferPool, gst_drm_buffer_pool, GST_TYPE_BUFFER_POOL);


static GType gst_meta_DRM_buffer_api_get_type (void);
#define GST_META_DRM_BUFFER_API_TYPE (gst_meta_DRM_buffer_api_get_type())

static const GstMetaInfo *gst_meta_DRM_buffer_get_info (void);
#define GST_META_DRM_BUFFER_INFO (gst_meta_DRM_buffer_get_info())

#define GST_META_DRM_BUFFER_GET(buf) ((GstMetaDRMBuffer *)gst_buffer_get_meta(buf,GST_META_DRM_BUFFER_API_TYPE))
#define GST_META_DRM_BUFFER_ADD(buf) ((GstMetaDRMBuffer *)gst_buffer_add_meta(buf,GST_META_DRM_BUFFER_INFO,NULL))


static gboolean
drmbuffer_init_func (GstMeta * meta, gpointer params, GstBuffer * buffer)
{
  GST_DEBUG ("init called on buffer %p, meta %p", buffer, meta);
  /* nothing to init really, the init function is mostly for allocating
   * additional memory or doing special setup as part of adding the metadata to
   * the buffer*/
  return TRUE;
}

static void
drmbuffer_free_func (GstMeta * drmmeta, GstBuffer * buffer)
{
  GstMetaDRMBuffer *meta = (GstMetaDRMBuffer *) drmmeta;

    /* Free the DRM buffer */
    omap_bo_del (meta->bo);
}

static gboolean
drmbuffer_transform_func (GstBuffer * transbuf, GstMeta * meta,
    GstBuffer * buffer, GQuark type, gpointer data)
{
  return FALSE;
}

static GType
gst_meta_DRM_buffer_api_get_type (void)
{
  static volatile GType type;
  static const gchar *tags[] = { "drmbuffer", NULL };

  if (g_once_init_enter (&type)) {
    GType _type = gst_meta_api_type_register ("GstMetaDRMBufferAPI", tags);
    g_once_init_leave (&type, _type);
  }
  return type;
}

static const GstMetaInfo *
gst_meta_DRM_buffer_get_info (void)
{
  static const GstMetaInfo *meta_drm_buffer_info = NULL;

  if (g_once_init_enter (&meta_drm_buffer_info)) {
    const GstMetaInfo *mi = gst_meta_register (GST_META_DRM_BUFFER_API_TYPE,
        "GstMetaDRMBuffer",
        sizeof (GstMetaDRMBuffer),
        drmbuffer_init_func, drmbuffer_free_func, drmbuffer_transform_func);
    g_once_init_leave (&meta_drm_buffer_info, mi);
  }
  return meta_drm_buffer_info;
}

/**
 * gst_drm_buffer_pool_set_config:
 * @pool: a #GstBufferPool
 * @config: a #GstStructure
 *
 * Parses the @config to retrieve the caps that is set by gst_buffer_pool_config_set_params().
 * This caps is then parsed to retrieve the video info. This function can be called to
 * change the caps of the buffer pool. 
 * 
 * Returns: the boolean value, TRUE for success and FALSE in case of an errors
 *
 * Since: 1.2.?
 */

gboolean
gst_drm_buffer_pool_set_config (GstBufferPool * pool, GstStructure * config)
{
  GstDRMBufferPool *drmpool = GST_DRM_BUFFER_POOL (pool);
  GstCaps *caps;

  /* get the caps param already set in the config */
  if (!gst_buffer_pool_config_get_params (config, &caps, NULL, NULL, NULL))
    goto wrong_config;

  if (caps == NULL)
    goto no_caps;

  /* now parse the caps from the config to get the video info */
  if (!gst_video_info_from_caps (&drmpool->info, caps))
    goto wrong_caps;

  GST_LOG_OBJECT (pool, "%dx%d, caps %" GST_PTR_FORMAT, drmpool->info.width, drmpool->info.height,
      caps);

  /* Set caps related variables of the pool */
  drmpool->caps = gst_caps_ref (caps);
  drmpool->width = drmpool->info.width;
  drmpool->height = drmpool->info.height;

  return GST_BUFFER_POOL_CLASS (parent_class)->set_config (pool, config);

  /* ERRORS */
wrong_config:
  {
    GST_WARNING_OBJECT (pool, "invalid config");
    return FALSE;
  }
no_caps:
  {
    GST_WARNING_OBJECT (pool, "no caps in config");
    return FALSE;
  }
wrong_caps:
  {
    GST_WARNING_OBJECT (pool,
        "failed getting geometry from caps %" GST_PTR_FORMAT, caps);
    return FALSE;
  }
}


/**
 * gst_drm_buffer_pool_initialize:
 * @pool: a #GstBufferPool
 * @element: a #GstElement
 * @fd: a file descriptor
 * @caps: a #GstCaps
 * @size: the padded size of buffers
 *
 * Initializes the device related info. Sets the params for buffer pool config.
 * Sets the config of the pool by calling gst_buffer_pool_set_config(). 
 * 
 *
 * Since: 1.2.?
 */


void
gst_drm_buffer_pool_initialize (GstDRMBufferPool * self,
    GstElement * element, int fd, GstCaps * caps, guint size)
{
  GstStructure *conf;

  /* store the element that requested for the pool */
  if(element)
   self->element = gst_object_ref (element);

  /* initialize device info */
  self->fd = fd;
  self->dev = omap_device_new (fd);

  /* Padded size of buffers. Can be used for testing requested-buffer-size vs obtained-buffer-size */
  self->size = size;
 

  /* get the present config of the buffer pool */
  conf = gst_buffer_pool_get_config (GST_BUFFER_POOL(self));
  if(conf == NULL) {
   GST_WARNING_OBJECT(self, "NULL config obtained after get_config on the pool");
   }

  /* set the config params : caps, size of the buffers, min number of buffers,
     max number of buffers (0 for unlimited) */
  gst_buffer_pool_config_set_params (conf, caps, size, 0, 0);
  if(conf == NULL){
    GST_WARNING_OBJECT(self, "NULL config after set_params");
   }

  /* set config of the pool */
  gst_buffer_pool_set_config (GST_BUFFER_POOL(self), conf);

}


/**
 * gst_drm_buffer_pool_new:
 * @element: a #GstElement
 * @fd: a file descriptor
 * @caps: a #GstCaps
 * @size: the padded size of buffers
 *
 * Creates a GstDRMBufferPool and initializes it through gst_drm_buffer_pool_initialize()
 *
 * Returns: the #GstDRMBufferPool created and initialized.
 *
 * Since: 1.2.?
 */

GstDRMBufferPool *
gst_drm_buffer_pool_new (GstElement * element,
    int fd, GstCaps * caps, guint size)
{
  GstDRMBufferPool *self = g_object_new (GST_TYPE_DRM_BUFFER_POOL, NULL);

  GST_DEBUG_OBJECT (element,
      "Creating DRM buffer pool with caps %" GST_PTR_FORMAT, caps);

  gst_drm_buffer_pool_initialize (self, element, fd, caps, size);
  gst_buffer_pool_set_active (GST_BUFFER_POOL(self), TRUE);
  return self;
}


/**
 * gst_drm_buffer_pool_size:
 * @self: a #GstDRMBufferPool
 *
 * Obtain the padded size of buffers set during bufferpool creation
 *
 * Returns: the size of individual buffers within the bufferpool.
 *
 * Since: 1.2.?
 */

guint
gst_drm_buffer_pool_size (GstDRMBufferPool * self)
{
  return self->size;
}


/**
 * gst_drm_buffer_pool_check_caps:
 * @self: a #GstDRMBufferPool
 * @caps: a #GstCaps
 *
 * Check if the @caps and the caps of the @self is strictly equal
 *
 * Returns: the boolean value obtained from gst_caps_is_strictly_equal()
 *
 * Since: 1.2.?
 */

gboolean
gst_drm_buffer_pool_check_caps (GstDRMBufferPool * self, GstCaps * caps)
{
  return gst_caps_is_strictly_equal (self->caps, caps);
}


/**
 * gst_drm_buffer_pool_destroy:
 * @self: a #GstDRMBufferPool
 *
 * destroy existing bufferpool by gst_object_unref()
 *
 * Since: 1.2.?
 */

void
gst_drm_buffer_pool_destroy (GstDRMBufferPool * self)
{
  g_return_if_fail (self);

  GST_DEBUG_OBJECT (self->element, "destroy pool (contains: %d buffers)",
      self->nbbufs);

 /* Sets the buffer pool active to FALSE. Unrefs the buffer pool.
    If the the ref_count becomes zero, all buffers are freed and the bufferpool is destroyed */  
 if(GST_OBJECT_REFCOUNT(self)) {
  gst_object_unref (self);
  }
}



/**
 * gst_drm_buffer_pool_get:
 * @self: a #GstDRMBufferPool
 * @force_alloc: a boolean indicating if a buffer should be acquired from already queued buffers of pool.
 *
 * Get a buffer from the #GstDRMBufferPool
 *
 * Returns: the #GstBuffer
 *
 * Since: 1.2.?
 */

GstBuffer *
gst_drm_buffer_pool_get (GstDRMBufferPool * self, gboolean force_alloc)
{
  GstBuffer *buf = NULL; 
  g_return_val_if_fail (self, NULL);

  /* re-use a buffer off the queued buffers of pool if any are available */
  if (!force_alloc) {
     gst_buffer_pool_acquire_buffer (GST_BUFFER_POOL (self), &buf, NULL);
  } else {
     GST_BUFFER_POOL_CLASS(GST_DRM_BUFFER_POOL_GET_CLASS (self))->alloc_buffer(GST_BUFFER_POOL (self), &buf, NULL);

  }
       
  GST_LOG_OBJECT (self->element, "returning buf %p", buf);

  return GST_BUFFER (buf);
}


/**
 * gst_drm_buffer_pool_put:
 * @self: a #GstDRMBufferPool
 * @force_alloc: a boolean indicating if a buffer should be acquired from already queued buffers of pool.
 *
 * Get a buffer from the #GstDRMBufferPool
 *
 * Returns: the boolean value corresponding to success (TRUE)
 *
 * Since: 1.2.?
 */
gboolean
gst_drm_buffer_pool_put (GstDRMBufferPool * self, GstBuffer * buf)
{
  gboolean reuse = gst_buffer_pool_is_active (GST_BUFFER_POOL (self));
   if(reuse){
       gst_buffer_pool_release_buffer(GST_BUFFER_POOL(self) , buf);
   }
  return reuse;
}

static void
gst_drm_buffer_pool_finalize (GObject * pool)
{
  GstDRMBufferPool *self = GST_DRM_BUFFER_POOL (pool);
  GST_DEBUG_OBJECT (self->element, "finalize");

  if (self->caps)
    gst_caps_unref (self->caps);
  if (self->element)
    gst_object_unref (self->element);
  if (self->dev)
   omap_device_del (self->dev);

  G_OBJECT_CLASS (gst_drm_buffer_pool_parent_class)->finalize(pool);
}

static void
gst_drm_buffer_pool_class_init (GstDRMBufferPoolClass * klass)
{
  GObjectClass *object_class;
  GstBufferPoolClass *bclass = GST_BUFFER_POOL_CLASS (klass);
  GST_DEBUG_CATEGORY_INIT (drmbufferpool_debug, "drmbufferpool", 0,
      "DRM buffer pool");
  parent_class = g_type_class_peek_parent (klass);
  object_class = G_OBJECT_CLASS (klass);
  bclass->set_config = gst_drm_buffer_pool_set_config;
  bclass->alloc_buffer = gst_drm_alloc_new_buffer;
  object_class->finalize = gst_drm_buffer_pool_finalize;
}

static void
gst_drm_buffer_pool_init (GstDRMBufferPool * self)
{
#ifndef GST_DISABLE_GST_DEBUG
  self->nbbufs = 0;
#endif /* DEBUG */
}

/**
 * gst_drm_alloc_new_buffer:
 * @bufpool: a #GstBufferPool
 * @buffer: a pointer to #GstBuffer
 * @params: a #GstBufferPoolAcquireParams
 *
 * Allocate a new buffer to the #GstDRMBufferPool
 *
 * Returns: the #GstFlowReturn
 *
 * Since: 1.2.?
 */
static GstFlowReturn
gst_drm_alloc_new_buffer (GstBufferPool * bufpool, GstBuffer ** buffer,
    GstBufferPoolAcquireParams * params)
{

  /* create a buffer with ref_count = 1 */
  GstBuffer *buf = gst_buffer_new ();
  GstDRMBufferPool *pool = GST_DRM_BUFFER_POOL(bufpool);
  GstVideoCropMeta *crop;
  GstMetaDmaBuf *dmabuf;
  GstVideoMeta *videometa;
  GstMetaDRMBuffer *drmbuf;

  drmbuf = GST_META_DRM_BUFFER_ADD (buf);

  /* TODO: if allocation could be handled via libkms then this
   * bufferpool implementation could be completely generic..
   * otherwise we might want some support for various different
   * drm drivers here:
   */

  struct omap_bo *bo = omap_bo_new (pool->dev, pool->size, OMAP_BO_WC);
  if (!bo) {
    GST_WARNING_OBJECT (pool->element, "Failed to create bo");
    return GST_FLOW_ERROR;;
  }

  drmbuf->bo = bo;

  /* allocating a memory to the buffer we created */
  gst_buffer_append_memory (buf,
      gst_memory_new_wrapped (GST_MEMORY_FLAG_NO_SHARE, omap_bo_map (bo),
          pool->size, 0, pool->size, NULL, NULL));
  
  /* Adding the necessary metadatas with initialization*/

  dmabuf = gst_buffer_add_dma_buf_meta (GST_BUFFER (buf), omap_bo_dmabuf (bo), omap_bo_map (bo), (void *)omap_bo_get_paddr (bo));
  if(!dmabuf){
    GST_DEBUG_OBJECT (pool, "Failed to add dmabuf meta to buffer");
  }

  videometa = gst_buffer_add_video_meta(buf,GST_VIDEO_FRAME_FLAG_NONE, GST_VIDEO_INFO_FORMAT(&pool->info), pool->width, pool->height);
  if(!videometa){
    GST_DEBUG_OBJECT (pool, "Failed to add video meta to buffer");
  }

  crop = gst_buffer_add_video_crop_meta(buf);
  if(!crop){
    GST_DEBUG_OBJECT (pool, "Failed to add crop meta to buffer");
  } else {
  crop->x = 0;
  crop->y = 0;
  crop->height = pool->height;
  crop->width = pool->width;
  }

  /* Pointer to the buffer (passed as argument) should now point to the buffer we created */
  *buffer = buf;

#ifndef GST_DISABLE_GST_DEBUG
      {
        GST_DEBUG_OBJECT (pool, "Creating new buffer (living buffer: %i)",
            ++pool->nbbufs);
      }
#endif

  return GST_FLOW_OK;

}

