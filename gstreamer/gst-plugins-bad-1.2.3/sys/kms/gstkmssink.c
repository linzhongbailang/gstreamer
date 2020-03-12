/* GStreamer
 * Copyright (C) 2012 Texas Instruments
 * Copyright (C) 2012 Collabora Ltd
 *
 * Authors:
 *  Alessandro Decina <alessandro.decina@collabora.co.uk>
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
 *
 * Authors:
 *  Alessandro Decina <alessandro.decina@collabora.co.uk>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstkmssink.h"
#include "gstkmsbufferpriv.h"

#include <libdce.h>
#include <omap_drm.h>
#include <omap_drmif.h>
#include <xf86drmMode.h>

GST_DEBUG_CATEGORY (gst_debug_kms_sink);
#define GST_CAT_DEFAULT gst_debug_kms_sink

G_DEFINE_TYPE (GstKMSSink, gst_kms_sink, GST_TYPE_VIDEO_SINK);

static void gst_kms_sink_reset (GstKMSSink * sink);

static GstStaticPadTemplate gst_kms_sink_template_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE("NV12"))
    );

enum
{
  PROP_0,
  PROP_PIXEL_ASPECT_RATIO,
  PROP_FORCE_ASPECT_RATIO,
  PROP_SCALE,
  PROP_CONNECTOR,
  PROP_CONNECTOR_NAME,
};

static inline void
display_bufs_queue (GstKMSSink * sink, GstBuffer * buf)
{
  int i;
  for (i = 0; i < (NUM_DISPLAY_BUFS - 1); i++)
    gst_buffer_replace (&sink->display_bufs[i], sink->display_bufs[i + 1]);
  gst_buffer_replace (&sink->display_bufs[i], buf);
}

static inline void
display_bufs_free (GstKMSSink * sink)
{
  int i;
  for (i = 0; i < NUM_DISPLAY_BUFS; i++)
    gst_buffer_replace (&sink->display_bufs[i], NULL);
}

static gboolean
gst_kms_sink_calculate_aspect_ratio (GstKMSSink * sink, gint width,
    gint height, gint video_par_n, gint video_par_d)
{
  guint calculated_par_n;
  guint calculated_par_d;

  if (!gst_video_calculate_display_ratio (&calculated_par_n, &calculated_par_d,
          width, height, video_par_n, video_par_d, 1, 1)) {
    GST_ELEMENT_ERROR (sink, CORE, NEGOTIATION, (NULL),
        ("Error calculating the output display ratio of the video."));
    return FALSE;
  }
  GST_DEBUG_OBJECT (sink,
      "video width/height: %dx%d, calculated display ratio: %d/%d",
      width, height, calculated_par_n, calculated_par_d);

  /* now find a width x height that respects this display ratio.
   * prefer those that have one of w/h the same as the incoming video
   * using wd / hd = calculated_pad_n / calculated_par_d */

  /* start with same height, because of interlaced video */
  /* check hd / calculated_par_d is an integer scale factor, and scale wd with the PAR */
  if (height % calculated_par_d == 0) {
    GST_DEBUG_OBJECT (sink, "keeping video height");
    GST_VIDEO_SINK_WIDTH (sink) = (guint)
        gst_util_uint64_scale_int (height, calculated_par_n, calculated_par_d);
    GST_VIDEO_SINK_HEIGHT (sink) = height;
  } else if (width % calculated_par_n == 0) {
    GST_DEBUG_OBJECT (sink, "keeping video width");
    GST_VIDEO_SINK_WIDTH (sink) = width;
    GST_VIDEO_SINK_HEIGHT (sink) = (guint)
        gst_util_uint64_scale_int (width, calculated_par_d, calculated_par_n);
  } else {
    GST_DEBUG_OBJECT (sink, "approximating while keeping video height");
    GST_VIDEO_SINK_WIDTH (sink) = (guint)
        gst_util_uint64_scale_int (height, calculated_par_n, calculated_par_d);
    GST_VIDEO_SINK_HEIGHT (sink) = height;
  }
  GST_DEBUG_OBJECT (sink, "scaling to %dx%d",
      GST_VIDEO_SINK_WIDTH (sink), GST_VIDEO_SINK_HEIGHT (sink));

  return TRUE;
}

static gboolean
gst_kms_sink_setcaps (GstBaseSink * bsink, GstCaps * caps)
{
  GstKMSSink *sink;
  gboolean ret = TRUE;
  gint width, height;
  gint fps_n, fps_d;
  gint par_n, par_d;
  GstVideoFormat format;
  GstVideoInfo info;

  sink = GST_KMS_SINK (bsink);

  ret = gst_video_info_from_caps (&info, caps);
  format = GST_VIDEO_INFO_FORMAT(&info);
  width = GST_VIDEO_INFO_WIDTH(&info);
  height = GST_VIDEO_INFO_HEIGHT(&info);
  fps_n = GST_VIDEO_INFO_FPS_N(&info);
  fps_d = GST_VIDEO_INFO_FPS_D(&info);
  par_n = GST_VIDEO_INFO_PAR_N(&info);
  par_d = GST_VIDEO_INFO_PAR_D(&info);

  if (!ret)
    return FALSE;

  if (width <= 0 || height <= 0) {
    GST_ELEMENT_ERROR (sink, CORE, NEGOTIATION, (NULL),
        ("Invalid image size."));
    return FALSE;
  }

  sink->format = format;
  sink->par_n = par_n;
  sink->par_d = par_d;
  sink->src_rect.x = sink->src_rect.y = 0;
  sink->src_rect.w = width;
  sink->src_rect.h = height;
  sink->input_width = width;
  sink->input_height = height;

  if (!sink->pool || !gst_drm_buffer_pool_check_caps (sink->pool, caps)) {
    int size;

    if (sink->pool) {
      gst_drm_buffer_pool_destroy (sink->pool);
      sink->pool = NULL;
    }

    size = GST_VIDEO_INFO_SIZE(&info);
    sink->pool = gst_drm_buffer_pool_new (GST_ELEMENT (sink),
        sink->fd, caps, size);
  }

  sink->conn.crtc = -1;
  sink->plane = NULL;

  return TRUE;
}

static void
gst_kms_sink_get_times (GstBaseSink * bsink, GstBuffer * buf,
    GstClockTime * start, GstClockTime * end)
{
  GstKMSSink *sink;

  sink = GST_KMS_SINK (bsink);

  if (GST_BUFFER_PTS_IS_VALID (buf)) {
    *start = GST_BUFFER_PTS (buf);
    if (GST_BUFFER_DURATION_IS_VALID (buf)) {
      *end = *start + GST_BUFFER_DURATION (buf);
    } else {
      if (sink->fps_n > 0) {
        *end = *start +
            gst_util_uint64_scale_int (GST_SECOND, sink->fps_d, sink->fps_n);
      }
    }
  }
}

static GstFlowReturn
gst_kms_sink_show_frame (GstVideoSink * vsink, GstBuffer * inbuf)
{
  GstKMSSink *sink = GST_KMS_SINK (vsink);
  GstBuffer *buf = NULL;
  GstKMSBufferPriv *priv;
  GstFlowReturn flow_ret = GST_FLOW_OK;
  int ret;
  gint width, height;
  GstVideoRectangle *c = &sink->src_rect;

 GstVideoCropMeta* crop = gst_buffer_get_video_crop_meta (inbuf);
 if (crop){
  c->y = crop->y;
  c->x = crop->x;

 if (crop->width >= 0) {
     width = crop->width;
 }
 else {
     width = GST_VIDEO_SINK_WIDTH (sink);
  }
 if (crop->height >= 0){
        height = crop->height;
 }
 else {
        height = GST_VIDEO_SINK_HEIGHT (sink);
  }
}


 c->w = width;
 c->h = height;


if (!gst_kms_sink_calculate_aspect_ratio (sink, width, height,
              sink->par_n, sink->par_d))
  GST_DEBUG_OBJECT (sink, "calculate aspect ratio failed");


  GST_INFO_OBJECT (sink, "enter");

  if (sink->conn.crtc == -1) {
    GstVideoRectangle dest = { 0 };

    if (sink->conn_name) {
      if (!gst_drm_connector_find_mode_and_plane_by_name (sink->fd,
              sink->dev, sink->src_rect.w, sink->src_rect.h,
              sink->resources, sink->plane_resources, &sink->conn,
              sink->conn_name, &sink->plane))
        goto connector_not_found;
    } else {
      sink->conn.id = sink->conn_id;
      if (!gst_drm_connector_find_mode_and_plane (sink->fd,
              sink->dev, sink->src_rect.w, sink->src_rect.h,
              sink->resources, sink->plane_resources, &sink->conn,
              &sink->plane))
        goto connector_not_found;
    }

    dest.w = sink->conn.mode->hdisplay;
    dest.h = sink->conn.mode->vdisplay;
    gst_video_sink_center_rect (sink->src_rect, dest, &sink->dst_rect,
        sink->scale);
  }

  priv = gst_kms_buffer_priv (sink, inbuf);
  if (priv) {
    buf = gst_buffer_ref (inbuf);
  } else {
    GST_LOG_OBJECT (sink, "not a KMS buffer, slow-path!");
    buf = gst_drm_buffer_pool_get (sink->pool, FALSE);
    if (buf) {
      GST_BUFFER_PTS (buf) = GST_BUFFER_PTS (inbuf);
      GST_BUFFER_DURATION (buf) = GST_BUFFER_DURATION (inbuf);
      gst_buffer_copy_into (buf, inbuf, GST_BUFFER_COPY_DEEP, 0 ,-1);
      priv = gst_kms_buffer_priv (sink, buf);
    }
    if (!priv)
      goto add_fb2_failed;
  }

  ret = drmModeSetPlane (sink->fd, sink->plane->plane_id,
      sink->conn.crtc, priv->fb_id, 0,
      sink->dst_rect.x, sink->dst_rect.y, sink->dst_rect.w, sink->dst_rect.h,
      sink->src_rect.x << 16, sink->src_rect.y << 16,
      sink->src_rect.w << 16, sink->src_rect.h << 16);
  if (ret)
    goto set_plane_failed;

  display_bufs_queue (sink, buf);

out:
  GST_INFO_OBJECT (sink, "exit");
  if (buf)
    gst_buffer_unref (buf);
  return flow_ret;

add_fb2_failed:
  GST_ELEMENT_ERROR (sink, RESOURCE, FAILED,
      (NULL), ("drmModeAddFB2 failed: %s (%d)", strerror (errno), errno));
  flow_ret = GST_FLOW_ERROR;
  goto out;

set_plane_failed:
  GST_ELEMENT_ERROR (sink, RESOURCE, FAILED,
      (NULL), ("drmModeSetPlane failed: %s (%d)", strerror (errno), errno));
  flow_ret = GST_FLOW_ERROR;
  goto out;

connector_not_found:
  GST_ELEMENT_ERROR (sink, RESOURCE, NOT_FOUND,
      (NULL), ("connector not found", strerror (errno), errno));
  goto out;
}


static gboolean
gst_kms_sink_event (GstBaseSink * bsink, GstEvent * event)
{
  GstKMSSink *sink = GST_KMS_SINK (bsink);

  switch (GST_EVENT_TYPE (event)) {
    default:
      break;
  }
  if (GST_BASE_SINK_CLASS (gst_kms_sink_parent_class)->event)
    return GST_BASE_SINK_CLASS (gst_kms_sink_parent_class)->event (bsink,
        event);
  else
    return TRUE;
}

static void
gst_kms_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstKMSSink *sink;

  g_return_if_fail (GST_IS_KMS_SINK (object));

  sink = GST_KMS_SINK (object);

  switch (prop_id) {
    case PROP_FORCE_ASPECT_RATIO:
      sink->keep_aspect = g_value_get_boolean (value);
      break;
    case PROP_SCALE:
      sink->scale = g_value_get_boolean (value);
      break;
    case PROP_CONNECTOR:
      sink->conn_id = g_value_get_uint (value);
      break;
    case PROP_CONNECTOR_NAME:
      g_free (sink->conn_name);
      sink->conn_name = g_strdup (g_value_get_string (value));
      break;
    case PROP_PIXEL_ASPECT_RATIO:
    {
      GValue *tmp;

      tmp = g_new0 (GValue, 1);
      g_value_init (tmp, GST_TYPE_FRACTION);

      if (!g_value_transform (value, tmp)) {
        GST_WARNING_OBJECT (sink, "Could not transform string to aspect ratio");
      } else {
        sink->par_n = gst_value_get_fraction_numerator (tmp);
        sink->par_d = gst_value_get_fraction_denominator (tmp);
        GST_DEBUG_OBJECT (sink, "set PAR to %d/%d", sink->par_n, sink->par_d);
      }
      g_free (tmp);
    }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_kms_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstKMSSink *sink;

  g_return_if_fail (GST_IS_KMS_SINK (object));

  sink = GST_KMS_SINK (object);

  switch (prop_id) {
    case PROP_FORCE_ASPECT_RATIO:
      g_value_set_boolean (value, sink->keep_aspect);
      break;
    case PROP_SCALE:
      g_value_set_boolean (value, sink->scale);
      break;
    case PROP_CONNECTOR:
      g_value_set_uint (value, sink->conn.id);
      break;
    case PROP_PIXEL_ASPECT_RATIO:
    {
      char *v = g_strdup_printf ("%d/%d", sink->par_n, sink->par_d);
      g_value_take_string (value, v);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_kms_sink_reset (GstKMSSink * sink)
{
  GST_DEBUG_OBJECT (sink, "reset");

  if (sink->fd != -1) {
    gst_drm_connector_cleanup (sink->fd, &sink->conn);
  }
  memset (&sink->conn, 0, sizeof (struct connector));

  if (sink->pool) {
    gst_drm_buffer_pool_destroy (sink->pool);
    sink->pool = NULL;
  }

  if (sink->plane) {
    drmModeFreePlane (sink->plane);
    sink->plane = NULL;
  }

  if (sink->plane_resources) {
    drmModeFreePlaneResources (sink->plane_resources);
    sink->plane_resources = NULL;
  }

  if (sink->resources) {
    drmModeFreeResources (sink->resources);
    sink->resources = NULL;
  }

  display_bufs_free (sink);

  if (sink->dev) {
    dce_deinit (sink->dev);
    sink->dev = NULL;
    sink->fd = -1;
  }

  sink->par_n = sink->par_d = 1;
  sink->src_rect.x = 0;
  sink->src_rect.y = 0;
  sink->src_rect.w = 0;
  sink->src_rect.h = 0;
  sink->input_width = 0;
  sink->input_height = 0;
  sink->format = GST_VIDEO_FORMAT_UNKNOWN;

  memset (&sink->src_rect, 0, sizeof (GstVideoRectangle));
  memset (&sink->dst_rect, 0, sizeof (GstVideoRectangle));
}

static gboolean
gst_kms_sink_start (GstBaseSink * bsink)
{
  GstKMSSink *sink;

  sink = GST_KMS_SINK (bsink);

  sink->dev = dce_init ();
  if (sink->dev == NULL)
    goto device_failed;
  else
    sink->fd = dce_get_fd ();

  sink->resources = drmModeGetResources (sink->fd);
  if (sink->resources == NULL)
    goto resources_failed;

  sink->plane_resources = drmModeGetPlaneResources (sink->fd);
  if (sink->plane_resources == NULL)
    goto plane_resources_failed;

  return TRUE;

fail:
  gst_kms_sink_reset (sink);
  return FALSE;

device_failed:
  GST_ELEMENT_ERROR (sink, RESOURCE, FAILED,
      (NULL), ("omap_device_new failed"));
  goto fail;

resources_failed:
  GST_ELEMENT_ERROR (sink, RESOURCE, FAILED,
      (NULL), ("drmModeGetResources failed: %s (%d)", strerror (errno), errno));
  goto fail;

plane_resources_failed:
  GST_ELEMENT_ERROR (sink, RESOURCE, FAILED,
      (NULL), ("drmModeGetPlaneResources failed: %s (%d)",
          strerror (errno), errno));
  goto fail;
}

static gboolean
gst_kms_sink_stop (GstBaseSink * bsink)
{
  GstKMSSink *sink;

  sink = GST_KMS_SINK (bsink);
  gst_kms_sink_reset (sink);

  return TRUE;
}

static GstFlowReturn
gst_kms_sink_buffer_alloc (GstBaseSink * bsink, guint64 offset, guint size,
    GstCaps * caps, GstBuffer ** buf)
{
  GstKMSSink *sink;
  GstFlowReturn ret = GST_FLOW_OK;

  sink = GST_KMS_SINK (bsink);

  GST_DEBUG_OBJECT (sink, "begin");

  if (G_UNLIKELY (!caps)) {
    GST_WARNING_OBJECT (sink, "have no caps, doing fallback allocation");
    *buf = NULL;
    ret = GST_FLOW_OK;
    goto beach;
  }

  GST_LOG_OBJECT (sink,
      "a buffer of %d bytes was requested with caps %" GST_PTR_FORMAT
      " and offset %" G_GUINT64_FORMAT, size, caps, offset);

  /* initialize the buffer pool if not initialized yet */
  if (G_UNLIKELY (!sink->pool || gst_drm_buffer_pool_size (sink->pool) != size)) {
    GstVideoFormat format;
    gint width, height;
    GstVideoInfo info;

    if (sink->pool) {
      GST_INFO_OBJECT (sink, "in buffer alloc, pool->size != size");
      gst_drm_buffer_pool_destroy (sink->pool);
      sink->pool = NULL;
    }

    gst_video_info_from_caps (&info, caps);
    format = GST_VIDEO_INFO_FORMAT(&info);
    width = GST_VIDEO_INFO_WIDTH(&info);
    height = GST_VIDEO_INFO_HEIGHT(&info);
    size = GST_VIDEO_INFO_SIZE(&info);
    sink->pool = gst_drm_buffer_pool_new (GST_ELEMENT (sink),
        sink->fd, caps, size);
  }
  *buf = GST_BUFFER_CAST (gst_drm_buffer_pool_get (sink->pool, FALSE));

beach:
  return ret;
}

static void
gst_kms_sink_finalize (GObject * object)
{
  GstKMSSink *sink;

  sink = GST_KMS_SINK (object);
  gst_kms_sink_reset (sink);
  g_free (sink->conn_name);
  if (sink->kmsbufferpriv){
    g_hash_table_destroy (sink->kmsbufferpriv);
    sink->kmsbufferpriv = NULL;
}

  G_OBJECT_CLASS (gst_kms_sink_parent_class)->finalize (object);
}

static void
kmsbufferpriv_free_func (GstKMSBufferPriv *priv)
{
  drmModeRmFB (priv->fd, priv->fb_id);
  omap_bo_del (priv->bo);
  g_free(priv);
}


static void
gst_kms_sink_init (GstKMSSink * sink)
{
  sink->fd = -1;
  gst_kms_sink_reset (sink);
  sink->kmsbufferpriv = g_hash_table_new_full (g_direct_hash, g_direct_equal,
      NULL, (GDestroyNotify) kmsbufferpriv_free_func);
}

static void
gst_kms_sink_class_init (GstKMSSinkClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSinkClass *gstbasesink_class;
  GstVideoSinkClass *videosink_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesink_class = (GstBaseSinkClass *) klass;
  videosink_class = (GstVideoSinkClass *) klass;

  gobject_class->finalize = gst_kms_sink_finalize;
  gobject_class->set_property = gst_kms_sink_set_property;
  gobject_class->get_property = gst_kms_sink_get_property;

  g_object_class_install_property (gobject_class, PROP_FORCE_ASPECT_RATIO,
      g_param_spec_boolean ("force-aspect-ratio", "Force aspect ratio",
          "When enabled, reverse caps negotiation (scaling) will respect "
          "original aspect ratio", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIXEL_ASPECT_RATIO,
      g_param_spec_string ("pixel-aspect-ratio", "Pixel Aspect Ratio",
          "The pixel aspect ratio of the device", "1/1",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_SCALE,
      g_param_spec_boolean ("scale", "Scale",
          "When true, scale to render fullscreen", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_CONNECTOR,
      g_param_spec_uint ("connector", "Connector",
          "DRM connector id (0 for automatic selection)", 0, G_MAXUINT32, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT));
  g_object_class_install_property (gobject_class, PROP_CONNECTOR_NAME,
      g_param_spec_string ("connector-name", "Connector name",
          "DRM connector name (alternative to the connector property, "
          "use $type$index, $type-$index, or $type)", "",
          G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_details_simple (gstelement_class,
      "Video sink", "Sink/Video",
      "A video sink using the linux kernel mode setting API",
      "Alessandro Decina <alessandro.d@gmail.com>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_kms_sink_template_factory));

  gstbasesink_class->set_caps = GST_DEBUG_FUNCPTR (gst_kms_sink_setcaps);
  gstbasesink_class->get_times = GST_DEBUG_FUNCPTR (gst_kms_sink_get_times);
  gstbasesink_class->event = GST_DEBUG_FUNCPTR (gst_kms_sink_event);
  gstbasesink_class->start = GST_DEBUG_FUNCPTR (gst_kms_sink_start);
  gstbasesink_class->stop = GST_DEBUG_FUNCPTR (gst_kms_sink_stop);

  /* disable preroll as it's called before GST_CROP_EVENT has been received, so
   * we end up configuring the wrong mode... (based on padded caps)
   */
  gstbasesink_class->preroll = NULL;
  videosink_class->show_frame = GST_DEBUG_FUNCPTR (gst_kms_sink_show_frame);
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "kmssink",
          GST_RANK_PRIMARY + 1, GST_TYPE_KMS_SINK))
    return FALSE;

  GST_DEBUG_CATEGORY_INIT (gst_debug_kms_sink, "kmssink", 0, "kmssink element");

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    kms,
    "KMS video output element",
    plugin_init, VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
