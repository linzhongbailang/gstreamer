/* GStreamer Wayland video sink
 *
 * Copyright (C) 2011 Intel Corporation
 * Copyright (C) 2011 Sreerenj Balachandran <sreerenj.balachandran@intel.com>
 * Copyright (C) 2012 Wim Taymans <wim.taymans@gmail.com>
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 */

/**
 * SECTION:element-waylandsink
 *
 *  The waylandsink is creating its own window and render the decoded video frames to that.
 *  Setup the Wayland environment as described in
 *  <ulink url="http://wayland.freedesktop.org/building.html">Wayland</ulink> home page.
 *  The current implementaion is based on weston compositor.
 *
 * <refsect2>
 * <title>Example pipelines</title>
 * |[
 * gst-launch -v videotestsrc ! waylandsink
 * ]| test the video rendering in wayland
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gstwaylandsink.h"
#include "gstwlbufferpriv.h"

#include <wayland-client-protocol.h>
#include "wayland-drm-client-protocol.h"

#include <linux/input.h>


/* signals */
enum
{
  SIGNAL_0,
  LAST_SIGNAL
};

/* Properties */
enum
{
  PROP_0,
  PROP_WAYLAND_DISPLAY,
  PROP_WAYLAND_TITLE
};

GST_DEBUG_CATEGORY (gstwayland_debug);
#define GST_CAT_DEFAULT gstwayland_debug

#if G_BYTE_ORDER == G_BIG_ENDIAN
#define CAPS "{xRGB, ARGB, NV21}"
#else
#define CAPS "{BGRx, BGRA, NV12, I420, YUY2, UYVY}"
#endif

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE (CAPS))
    );

/*Fixme: Add more interfaces */
#define gst_wayland_sink_parent_class parent_class
G_DEFINE_TYPE (GstWaylandSink, gst_wayland_sink, GST_TYPE_VIDEO_SINK);

static void gst_wayland_sink_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);
static void gst_wayland_sink_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_wayland_sink_finalize (GObject * object);
static GstCaps *gst_wayland_sink_get_caps (GstBaseSink * bsink,
    GstCaps * filter);
static gboolean gst_wayland_sink_set_caps (GstBaseSink * bsink, GstCaps * caps);
static gboolean gst_wayland_sink_start (GstBaseSink * bsink);
static gboolean gst_wayland_sink_preroll (GstBaseSink * bsink,
    GstBuffer * buffer);
static gboolean
gst_wayland_sink_propose_allocation (GstBaseSink * bsink, GstQuery * query);
static gboolean gst_wayland_sink_render (GstBaseSink * bsink,
    GstBuffer * buffer);
static gboolean gst_wayland_sink_stop (GstBaseSink * bsink);

static struct display *create_display (void);
static void registry_handle_global (void *data, struct wl_registry *registry,
    uint32_t id, const char *interface, uint32_t version);
static void frame_redraw_callback (void *data,
    struct wl_callback *callback, uint32_t time);
static void create_window (GstWaylandSink * sink, struct display *display,
    int width, int height);
static void shm_pool_destroy (struct shm_pool *pool);

static void input_grab (struct input *input, struct window *window);
static void input_ungrab (struct input *input);

typedef struct
{
  uint32_t wl_format;
  GstVideoFormat gst_format;
} wl_VideoFormat;

static const wl_VideoFormat formats[] = {
#if G_BYTE_ORDER == G_BIG_ENDIAN
  {WL_SHM_FORMAT_XRGB8888, GST_VIDEO_FORMAT_xRGB},
  {WL_SHM_FORMAT_ARGB8888, GST_VIDEO_FORMAT_ARGB},
#else
  {WL_SHM_FORMAT_XRGB8888, GST_VIDEO_FORMAT_BGRx},
  {WL_SHM_FORMAT_ARGB8888, GST_VIDEO_FORMAT_BGRA},
#endif
};

static uint32_t
gst_wayland_format_to_wl_format (GstVideoFormat format)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (formats); i++)
    if (formats[i].gst_format == format)
      return formats[i].wl_format;

  GST_WARNING ("wayland video format not found");
  return -1;
}

#ifndef GST_DISABLE_GST_DEBUG
static const gchar *
gst_wayland_format_to_string (uint32_t wl_format)
{
  guint i;
  GstVideoFormat format = GST_VIDEO_FORMAT_UNKNOWN;

  for (i = 0; i < G_N_ELEMENTS (formats); i++)
    if (formats[i].wl_format == wl_format)
      format = formats[i].gst_format;

  return gst_video_format_to_string (format);
}
#endif

static void
gst_wayland_sink_class_init (GstWaylandSinkClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSinkClass *gstbasesink_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesink_class = (GstBaseSinkClass *) klass;

  gobject_class->set_property = gst_wayland_sink_set_property;
  gobject_class->get_property = gst_wayland_sink_get_property;
  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_wayland_sink_finalize);

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_template));

  gst_element_class_set_static_metadata (gstelement_class,
      "wayland video sink", "Sink/Video",
      "Output to wayland surface",
      "Sreerenj Balachandran <sreerenj.balachandran@intel.com>");

  gstbasesink_class->get_caps = GST_DEBUG_FUNCPTR (gst_wayland_sink_get_caps);
  gstbasesink_class->set_caps = GST_DEBUG_FUNCPTR (gst_wayland_sink_set_caps);
  gstbasesink_class->start = GST_DEBUG_FUNCPTR (gst_wayland_sink_start);
  gstbasesink_class->preroll = GST_DEBUG_FUNCPTR (gst_wayland_sink_preroll);
  gstbasesink_class->propose_allocation =
      GST_DEBUG_FUNCPTR (gst_wayland_sink_propose_allocation);
  gstbasesink_class->render = GST_DEBUG_FUNCPTR (gst_wayland_sink_render);
  gstbasesink_class->stop = GST_DEBUG_FUNCPTR (gst_wayland_sink_stop);

  g_object_class_install_property (gobject_class, PROP_WAYLAND_DISPLAY,
      g_param_spec_pointer ("wayland-display", "Wayland Display",
          "Wayland  Display handle created by the application ",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_WAYLAND_TITLE,
      g_param_spec_string ("title", "Surface title",
          "Title of the wayland surface associated ",
          "wayland-surface",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

/* Free function for key destruction for the hashtable we are using*/
static void
wlbufferpriv_free_func (GstWLBufferPriv * priv)
{
  wl_buffer_destroy (priv->buffer);
  omap_bo_del (priv->bo);
  g_free (priv);
}

static void
gst_wayland_sink_init (GstWaylandSink * sink)
{
  sink->display = NULL;
  sink->window = NULL;
  sink->shm_pool = NULL;
  sink->pool = NULL;
  sink->wlbufferpriv = NULL;
  sink->title = "";
  /* Initialising the hastable for storing map between dmabuf fd and GstWLBufferPriv */

  g_mutex_init (&sink->wayland_lock);
}

static void
gst_wayland_sink_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstWaylandSink *sink = GST_WAYLAND_SINK (object);

  switch (prop_id) {
    case PROP_WAYLAND_DISPLAY:
      g_value_set_pointer (value, sink->display);
      break;
    case PROP_WAYLAND_TITLE:
      g_value_set_string(value, sink->title);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_wayland_sink_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstWaylandSink *sink = GST_WAYLAND_SINK (object);

  switch (prop_id) {
    case PROP_WAYLAND_DISPLAY:
      sink->display = g_value_get_pointer (value);
      break;
    case PROP_WAYLAND_TITLE:
      sink->title = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
input_grab (struct input *input, struct window *window)
{
  input->grab = window;
}

static void
input_ungrab (struct input *input)
{
  input->grab = NULL;
}

static void
input_remove_pointer_focus (struct input *input)
{
  struct window *window = input->pointer_focus;

  if (!window)
    return;

  input->pointer_focus = NULL;
}

static void
input_destroy (struct input *input)
{
  input_remove_pointer_focus (input);

  if (input->display->seat_version >= 3) {
    if (input->pointer)
      wl_pointer_release (input->pointer);
  }

  wl_list_remove (&input->link);
  wl_seat_destroy (input->seat);
  free (input);
}

static void
display_destroy_inputs (struct display *display)
{
  struct input *tmp;
  struct input *input;

  wl_list_for_each_safe (input, tmp, &display->input_list, link)
      input_destroy (input);
}

static void
destroy_display (struct display *display)
{
  if (display->shm)
    wl_shm_destroy (display->shm);

  if (display->drm)
    wl_drm_destroy (display->drm);

  if (display->shell)
    wl_shell_destroy (display->shell);

  if (display->compositor)
    wl_compositor_destroy (display->compositor);

  display_destroy_inputs (display);
  wl_display_flush (display->display);
  wl_display_disconnect (display->display);

  if (display->dev) {
    omap_device_del (display->dev);
    display->dev = NULL;
  }

  if (display->fd !=-1)
    close (display->fd);

  free (display);
}

static void
destroy_window (struct window *window)
{
  if (window->callback) {
    wl_callback_destroy (window->callback);
    window->callback = NULL;
  }

  if (window->buffer)
    wl_buffer_destroy (window->buffer);

  if (window->shell_surface)
    wl_shell_surface_destroy (window->shell_surface);

  if (window->surface)
    wl_surface_destroy (window->surface);

  free (window);
}

static void
shm_pool_destroy (struct shm_pool *pool)
{
  munmap (pool->data, pool->size);
  wl_shm_pool_destroy (pool->pool);
  free (pool);
}

static void
gst_wayland_sink_finalize (GObject * object)
{
  GstWaylandSink *sink = GST_WAYLAND_SINK (object);

  GST_DEBUG_OBJECT (sink, "Finalizing the sink..");

  gst_buffer_replace (&sink->last_buf, NULL);
  gst_buffer_replace (&sink->display_buf, NULL);

  if (sink->window)
    destroy_window (sink->window);
  if (sink->display)
    destroy_display (sink->display);
  if (sink->shm_pool)
    shm_pool_destroy (sink->shm_pool);

  if (sink->wlbufferpriv) {
    g_hash_table_destroy (sink->wlbufferpriv);
    sink->wlbufferpriv = NULL;
  }

  g_mutex_clear (&sink->wayland_lock);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GstCaps *
gst_wayland_sink_get_caps (GstBaseSink * bsink, GstCaps * filter)
{
  GstWaylandSink *sink;
  GstCaps *caps;

  sink = GST_WAYLAND_SINK (bsink);

  caps = gst_pad_get_pad_template_caps (GST_VIDEO_SINK_PAD (sink));
  if (filter) {
    GstCaps *intersection;

    intersection =
        gst_caps_intersect_full (filter, caps, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (caps);
    caps = intersection;
  }
  return caps;
}

static void
shm_format (void *data, struct wl_shm *wl_shm, uint32_t format)
{
  struct display *d = data;

  d->formats |= (1 << format);
}

struct wl_shm_listener shm_listenter = {
  shm_format
};

/* For wl_drm_listener */
static void
drm_handle_device (void *data, struct wl_drm *drm, const char *device)
{
  struct display *d = data;
  drm_magic_t magic;

  d->fd = open (device, O_RDWR | O_CLOEXEC);
  if (d->fd == -1) {
    GST_ERROR ("could not open %s: %m", device);
    // XXX hmm, probably need to throw up some error now??
    return;
  }

  drmGetMagic (d->fd, &magic);
  wl_drm_authenticate (d->drm, magic);
}


static void
drm_handle_format (void *data, struct wl_drm *drm, uint32_t format)
{
  GST_DEBUG ("got format: %" GST_FOURCC_FORMAT, GST_FOURCC_ARGS (format));
}

static void
drm_handle_authenticated (void *data, struct wl_drm *drm)
{
  struct display *d = data;
  GST_DEBUG ("authenticated");

  d->dev = omap_device_new (d->fd);
  d->authenticated = 1;
  GST_DEBUG ("drm_handle_authenticated: dev: %p, d->authenticated: %d\n",
      d->dev, d->authenticated);
}

static const struct wl_drm_listener drm_listener = {
  drm_handle_device,
  drm_handle_format,
  drm_handle_authenticated
};

static void
pointer_handle_enter (void *data, struct wl_pointer *pointer,
    uint32_t serial, struct wl_surface *surface,
    wl_fixed_t sx_w, wl_fixed_t sy_w)
{
  struct input *input = data;

  if (!surface) {
    /* enter event for a window we've just destroyed */
    return;
  }

  input->display->serial = serial;
  input->pointer_focus = wl_surface_get_user_data (surface);
}

static void
pointer_handle_leave (void *data, struct wl_pointer *pointer,
    uint32_t serial, struct wl_surface *surface)
{
  struct input *input = data;

  input_remove_pointer_focus (input);
}

static void
pointer_handle_motion (void *data, struct wl_pointer *pointer,
    uint32_t time, wl_fixed_t sx_w, wl_fixed_t sy_w)
{
  struct input *input = data;
  struct window *window = input->pointer_focus;

  if (!window)
    return;

  if (input->grab)
    wl_shell_surface_move (input->grab->shell_surface, input->seat,
        input->display->serial);

}

static void
pointer_handle_button (void *data, struct wl_pointer *pointer, uint32_t serial,
    uint32_t time, uint32_t button, uint32_t state_w)
{
  struct input *input = data;
  enum wl_pointer_button_state state = state_w;
  input->display->serial = serial;

  if (button == BTN_LEFT) {
    if (state == WL_POINTER_BUTTON_STATE_PRESSED)
      input_grab (input, input->pointer_focus);

    if (input->grab && state == WL_POINTER_BUTTON_STATE_RELEASED)
      input_ungrab (input);
  }

  if (input->grab)
    wl_shell_surface_move (input->grab->shell_surface, input->seat,
        input->display->serial);
}

static void
pointer_handle_axis (void *data, struct wl_pointer *pointer,
    uint32_t time, uint32_t axis, wl_fixed_t value)
{
}

static const struct wl_pointer_listener pointer_listener = {
  pointer_handle_enter,
  pointer_handle_leave,
  pointer_handle_motion,
  pointer_handle_button,
  pointer_handle_axis,
};

static void
touch_handle_down (void *data, struct wl_touch *wl_touch,
    uint32_t serial, uint32_t time, struct wl_surface *surface,
    int32_t id, wl_fixed_t x_w, wl_fixed_t y_w)
{
  struct input *input = data;
  struct touch_point *tp;

  input->display->serial = serial;
  input->touch_focus = wl_surface_get_user_data (surface);
  if (!input->touch_focus) {
    return;
  }

  tp = malloc (sizeof *tp);
  if (tp) {
    tp->id = id;
    wl_list_insert (&input->touch_point_list, &tp->link);
    wl_shell_surface_move (input->touch_focus->shell_surface, input->seat,
        serial);
  }
}

static void
touch_handle_motion (void *data, struct wl_touch *wl_touch,
    uint32_t time, int32_t id, wl_fixed_t x_w, wl_fixed_t y_w)
{
  struct input *input = data;
  struct touch_point *tp;


  if (!input->touch_focus) {
    return;
  }
  wl_list_for_each (tp, &input->touch_point_list, link) {
    if (tp->id != id)
      continue;

    wl_shell_surface_move (input->touch_focus->shell_surface, input->seat,
        input->display->serial);

    return;
  }
}

static void
touch_handle_frame (void *data, struct wl_touch *wl_touch)
{
}

static void
touch_handle_cancel (void *data, struct wl_touch *wl_touch)
{
}

static void
touch_handle_up (void *data, struct wl_touch *wl_touch,
    uint32_t serial, uint32_t time, int32_t id)
{
  struct input *input = data;
  struct touch_point *tp, *tmp;

  if (!input->touch_focus) {
    return;
  }

  wl_list_for_each_safe (tp, tmp, &input->touch_point_list, link) {
    if (tp->id != id)
      continue;

    wl_list_remove (&tp->link);
    free (tp);

    return;
  }
}

static const struct wl_touch_listener touch_listener = {
  touch_handle_down,
  touch_handle_up,
  touch_handle_motion,
  touch_handle_frame,
  touch_handle_cancel,
};



static void
seat_handle_capabilities (void *data, struct wl_seat *seat,
    enum wl_seat_capability caps)
{
  struct input *input = data;

  if ((caps & WL_SEAT_CAPABILITY_POINTER) && !input->pointer) {
    input->pointer = wl_seat_get_pointer (seat);
    wl_pointer_set_user_data (input->pointer, input);
    wl_pointer_add_listener (input->pointer, &pointer_listener, input);
  } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && input->pointer) {
    wl_pointer_destroy (input->pointer);
    input->pointer = NULL;
  }

  if ((caps & WL_SEAT_CAPABILITY_TOUCH) && !input->touch) {
    input->touch = wl_seat_get_touch (seat);
    wl_touch_set_user_data (input->touch, input);
    wl_touch_add_listener (input->touch, &touch_listener, input);
  } else if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && input->touch) {
    wl_touch_destroy (input->touch);
    input->touch = NULL;
  }
}

static void
seat_handle_name (void *data, struct wl_seat *seat, const char *name)
{

}

static const struct wl_seat_listener seat_listener = {
  seat_handle_capabilities,
  seat_handle_name
};

static void
display_add_input (struct display *d, uint32_t id)
{
  struct input *input;

  input = calloc (1, sizeof (*input));
  if (input == NULL) {
    fprintf (stderr, "%s: out of memory\n", "gst-wayland-sink");
    exit (EXIT_FAILURE);
  }
  input->display = d;
  input->seat = wl_registry_bind (d->registry, id, &wl_seat_interface,
      MAX (d->seat_version, 3));
  input->touch_focus = NULL;
  input->pointer_focus = NULL;
  wl_list_init (&input->touch_point_list);
  wl_list_insert (d->input_list.prev, &input->link);

  wl_seat_add_listener (input->seat, &seat_listener, input);
  wl_seat_set_user_data (input->seat, input);

}

static void
registry_handle_global (void *data, struct wl_registry *registry,
    uint32_t id, const char *interface, uint32_t version)
{
  struct display *d = data;

  if (strcmp (interface, "wl_compositor") == 0) {
    d->compositor =
        wl_registry_bind (registry, id, &wl_compositor_interface, 1);
  } else if (strcmp (interface, "wl_shell") == 0) {
    d->shell = wl_registry_bind (registry, id, &wl_shell_interface, 1);
  } else if (strcmp (interface, "wl_shm") == 0) {
    d->shm = wl_registry_bind (registry, id, &wl_shm_interface, 1);
    wl_shm_add_listener (d->shm, &shm_listenter, d);
  } else if (strcmp (interface, "wl_drm") == 0) {
    d->drm = wl_registry_bind (registry, id, &wl_drm_interface, 1);
    wl_drm_add_listener (d->drm, &drm_listener, d);
  } else if (strcmp (interface, "wl_seat") == 0) {
    d->seat_version = version;
    display_add_input (d, id);
  }
}

static const struct wl_registry_listener registry_listener = {
  registry_handle_global
};

static struct display *
create_display (void)
{
  struct display *display;

  display = malloc (sizeof *display);
  display->display = wl_display_connect (NULL);

  if (display->display == NULL) {
    free (display);
    return NULL;
  }
  display->authenticated = 0;
  display->shm = NULL;
  display->drm = NULL;
  display->shell = NULL;
  display->compositor = NULL;
  display->dev = NULL;
  display->fd = -1;

  wl_list_init (&display->input_list);

  display->registry = wl_display_get_registry (display->display);
  wl_registry_add_listener (display->registry, &registry_listener, display);

  wl_display_roundtrip (display->display);
  if (display->shm == NULL && display->drm == NULL) {
    GST_ERROR ("No wl_shm global and wl_drm global..");
    return NULL;
  }

  wl_display_roundtrip (display->display);

  wl_display_get_fd (display->display);

  return display;
}

static gboolean
gst_wayland_sink_format_from_caps (uint32_t * wl_format, GstCaps * caps)
{
  GstStructure *structure;
  const gchar *format;
  GstVideoFormat fmt;

  structure = gst_caps_get_structure (caps, 0);
  format = gst_structure_get_string (structure, "format");
  fmt = gst_video_format_from_string (format);

  *wl_format = gst_wayland_format_to_wl_format (fmt);

  return (*wl_format != -1);
}

static void
wait_authentication (GstWaylandSink * sink)
{
  GST_DEBUG_OBJECT (sink, "Before wait aunthenticated value is %d : \n",
      sink->display->authenticated);
  while (!sink->display->authenticated) {
    GST_DEBUG_OBJECT (sink, "waiting for authentication");
    wl_display_roundtrip (sink->display->display);
  }
  GST_DEBUG_OBJECT (sink, "After wait aunthenticated value is %d : \n",
      sink->display->authenticated);
}

/* create a drm buffer pool if the video format is NV12 */
static gboolean
create_pool (GstWaylandSink * sink, GstCaps * caps)
{

  GstVideoInfo info;

  wait_authentication (sink);

  while (!sink->display->authenticated) {
    GST_DEBUG_OBJECT (sink, "not authenticated yet");
  }

  if (!gst_video_info_from_caps (&info, caps))
    goto invalid_format;

  sink->video_width = info.width;
  sink->video_height = info.height;

  /* Code to be added if different bufferpool support for NV12 is needed.
     The current test cases use ducati + vpe plugin. Buffers will be allocated
     by vpe plugin. If vpe is not used, ducati plugin allocates buffers by itself from
     it's own bufferpool. Therefore waylandsink doesnot need a bufferpool implementation
     for allocating buffers.
  */
    return TRUE;

invalid_format:
  {
    GST_DEBUG_OBJECT (sink,
        "Could not locate image format from caps %" GST_PTR_FORMAT, caps);
    return FALSE;
  }
}

static gboolean
gst_wayland_sink_set_caps (GstBaseSink * bsink, GstCaps * caps)
{
  GstWaylandSink *sink = GST_WAYLAND_SINK (bsink);
  GstBufferPool *newpool, *oldpool;
  GstVideoInfo info;
  GstStructure *structure;
  static GstAllocationParams params = { 0, 0, 0, 15, };
  guint size;
  GstVideoFormat fmt;

  sink = GST_WAYLAND_SINK (bsink);

  GST_LOG_OBJECT (sink, "set caps %" GST_PTR_FORMAT, caps);

  if (!gst_video_info_from_caps (&info, caps))
    goto invalid_format;

  fmt = GST_VIDEO_INFO_FORMAT (&info);
  if (fmt == GST_VIDEO_FORMAT_NV12 || fmt == GST_VIDEO_FORMAT_I420
      || fmt == GST_VIDEO_FORMAT_YUY2 || fmt == GST_VIDEO_FORMAT_UYVY) {
    create_pool (sink, caps);
    return TRUE;
  }

  if (!gst_wayland_sink_format_from_caps (&sink->format, caps))
    goto invalid_format;


  if (!(sink->display->formats & (1 << sink->format))) {
    GST_DEBUG_OBJECT (sink, "%s not available",
        gst_wayland_format_to_string (sink->format));
    return FALSE;
  }

  sink->video_width = info.width;
  sink->video_height = info.height;
  size = info.size;

  /* create a new pool for the new configuration */
  newpool = gst_wayland_buffer_pool_new (sink);


  if (!newpool) {
    GST_DEBUG_OBJECT (sink, "Failed to create new pool");
    return FALSE;
  }

  structure = gst_buffer_pool_get_config (newpool);
  gst_buffer_pool_config_set_params (structure, caps, size, 2, 0);
  gst_buffer_pool_config_set_allocator (structure, NULL, &params);
  if (!gst_buffer_pool_set_config (newpool, structure))
    goto config_failed;

  oldpool = sink->pool;
  sink->pool = newpool;
  if (oldpool)
    gst_object_unref (oldpool);

  return TRUE;

invalid_format:
  {
    GST_DEBUG_OBJECT (sink,
        "Could not locate image format from caps %" GST_PTR_FORMAT, caps);
    return FALSE;
  }
config_failed:
  {
    GST_DEBUG_OBJECT (bsink, "failed setting config");
    return FALSE;
  }
}

static void
handle_ping (void *data, struct wl_shell_surface *shell_surface,
    uint32_t serial)
{
  wl_shell_surface_pong (shell_surface, serial);
}

static void
handle_configure (void *data, struct wl_shell_surface *shell_surface,
    uint32_t edges, int32_t width, int32_t height)
{
}

static void
handle_popup_done (void *data, struct wl_shell_surface *shell_surface)
{
}

static const struct wl_shell_surface_listener shell_surface_listener = {
  handle_ping,
  handle_configure,
  handle_popup_done
};

static void
create_window (GstWaylandSink * sink, struct display *display, int width,
    int height)
{
  struct window *window;

  if (sink->window)
    return;

  g_mutex_lock (&sink->wayland_lock);

  window = malloc (sizeof *window);
  window->buffer = NULL;
  window->display = display;
  window->width = width;
  window->height = height;
  window->redraw_pending = FALSE;

  window->surface = wl_compositor_create_surface (display->compositor);

  wl_surface_set_user_data (window->surface, window);

  window->shell_surface = wl_shell_get_shell_surface (display->shell,
      window->surface);

  g_return_if_fail (window->shell_surface);

  wl_shell_surface_add_listener (window->shell_surface,
      &shell_surface_listener, window);

  wl_shell_surface_set_toplevel (window->shell_surface);
  wl_shell_surface_set_title(window->shell_surface, sink->title);

  sink->window = window;

  g_mutex_unlock (&sink->wayland_lock);
}

static gboolean
gst_wayland_sink_start (GstBaseSink * bsink)
{
  GstWaylandSink *sink = (GstWaylandSink *) bsink;
  gboolean result = TRUE;

  GST_DEBUG_OBJECT (sink, "start");

  if (!sink->display)
    sink->display = create_display ();

  if (sink->display == NULL) {
    GST_ELEMENT_ERROR (bsink, RESOURCE, OPEN_READ_WRITE,
        ("Could not initialise Wayland output"),
        ("Could not create Wayland display"));
    return FALSE;
  }

  /* Initialising the hastable for storing map between dmabuf fd and GstWLBufferPriv */
  if (!sink->wlbufferpriv)  {
     sink->wlbufferpriv = g_hash_table_new_full (g_direct_hash, g_direct_equal,
         NULL, (GDestroyNotify) wlbufferpriv_free_func);
  }

  return result;
}

static gboolean
gst_wayland_sink_stop (GstBaseSink * bsink)
{
  GstWaylandSink *sink = (GstWaylandSink *) bsink;
  GST_DEBUG_OBJECT (sink, "stop");

  gst_buffer_replace (&sink->last_buf, NULL);
  gst_buffer_replace (&sink->display_buf, NULL);

  if (sink->wlbufferpriv) {
    g_hash_table_destroy (sink->wlbufferpriv);
    sink->wlbufferpriv = NULL;
  }

  if (sink->window) {
    destroy_window (sink->window);
    sink->window=NULL;
  }

  if (sink->display) {
    destroy_display (sink->display);
    sink->display=NULL;
  }

   return TRUE;
}

static gboolean
gst_wayland_sink_propose_allocation (GstBaseSink * bsink, GstQuery * query)
{
  GstWaylandSink *sink = GST_WAYLAND_SINK (bsink);
  GstBufferPool *pool;
  GstStructure *config;
  GstCaps *caps;
  guint size;
  gboolean need_pool;

  gst_query_parse_allocation (query, &caps, &need_pool);

  if (caps == NULL)
    goto no_caps;

  g_mutex_lock (&sink->wayland_lock);
  if ((pool = sink->pool))
    gst_object_ref (pool);
  g_mutex_unlock (&sink->wayland_lock);

  if (pool != NULL) {
    GstCaps *pcaps;

    /* we had a pool, check caps */
    config = gst_buffer_pool_get_config (pool);
    gst_buffer_pool_config_get_params (config, &pcaps, &size, NULL, NULL);

    if (!gst_caps_is_equal (caps, pcaps)) {
      /* different caps, we can't use this pool */
      gst_object_unref (pool);
      pool = NULL;
    }
    gst_structure_free (config);
  }


  if (pool == NULL && need_pool) {
    GstVideoInfo info;

    if (!gst_video_info_from_caps (&info, caps))
      goto invalid_caps;

    GST_DEBUG_OBJECT (sink, "create new pool");
    pool = gst_wayland_buffer_pool_new (sink);

    /* the normal size of a frame */
    size = info.size;

    config = gst_buffer_pool_get_config (pool);
    gst_buffer_pool_config_set_params (config, caps, size, 2, 0);
    if (!gst_buffer_pool_set_config (pool, config))
      goto config_failed;
  }
  if (pool) {
    gst_query_add_allocation_pool (query, pool, size, 2, 0);
    gst_object_unref (pool);
  }
  return TRUE;

  /* ERRORS */
no_caps:
  {
    GST_DEBUG_OBJECT (bsink, "no caps specified");
    return FALSE;
  }
invalid_caps:
  {
    GST_DEBUG_OBJECT (bsink, "invalid caps specified");
    return FALSE;
  }
config_failed:
  {
    GST_DEBUG_OBJECT (bsink, "failed setting config");
    gst_object_unref (pool);
    return FALSE;
  }
}

static GstFlowReturn
gst_wayland_sink_preroll (GstBaseSink * bsink, GstBuffer * buffer)
{
  GST_DEBUG_OBJECT (bsink, "preroll buffer %p", buffer);
  return gst_wayland_sink_render (bsink, buffer);
}

static void
frame_redraw_callback (void *data, struct wl_callback *callback, uint32_t time)
{
  struct window *window = (struct window *) data;

  if(callback != window->callback) {
    GST_DEBUG ("wl_callback received not equal to window callback");
  }
  window->redraw_pending = FALSE;
  wl_callback_destroy (callback);
  window->callback = NULL;
}

static const struct wl_callback_listener frame_callback_listener = {
  frame_redraw_callback
};


static GstFlowReturn
gst_wayland_sink_render (GstBaseSink * bsink, GstBuffer * buffer)
{
  GstWaylandSink *sink = GST_WAYLAND_SINK (bsink);
  GstVideoRectangle src, dst, res;
  GstBuffer *to_render = NULL;
  GstWlMeta *meta;
  GstFlowReturn ret;
  struct window *window;
  struct display *display;
  GstWLBufferPriv *priv;
  GstMapInfo mapsrc;

  GST_LOG_OBJECT (sink, "render buffer %p", buffer);
  if (!sink->window) {
    gint video_width = sink->video_width;
    gint video_height = sink->video_height;
    GstVideoCropMeta *crop = gst_buffer_get_video_crop_meta (buffer);
    if (crop) {
      if (crop->width) {
        video_width = crop->width;
      }
      if (crop->height) {
        video_height = crop->height;
      }
    }
    create_window (sink, sink->display, video_width, video_height);
  }

  window = sink->window;
  display = sink->display;

  meta = gst_buffer_get_wl_meta (buffer);
  priv = gst_wl_buffer_priv (sink, buffer);

  if (window->redraw_pending) {
    wl_display_dispatch (display->display);
  }


  if (meta && meta->sink == sink) {
    GST_LOG_OBJECT (sink, "buffer %p from our pool, writing directly", buffer);
    to_render = buffer;
  } else if (priv) {
    to_render = buffer;
    GST_LOG_OBJECT (sink, " priv buffer %p from drm pool, writing directly",
        buffer);
  } else {
    GST_LOG_OBJECT (sink, "buffer %p not from our pool, copying", buffer);

    if (!sink->pool)
      goto no_pool;

    if (!gst_buffer_pool_set_active (sink->pool, TRUE))
      goto activate_failed;

    ret = gst_buffer_pool_acquire_buffer (sink->pool, &to_render, NULL);
    if (ret != GST_FLOW_OK)
      goto no_buffer;

    gst_buffer_map (buffer, &mapsrc, GST_MAP_READ);
    gst_buffer_fill (to_render, 0, mapsrc.data, mapsrc.size);
    gst_buffer_unmap (buffer, &mapsrc);

    meta = gst_buffer_get_wl_meta (to_render);
  }

  src.w = sink->video_width;
  src.h = sink->video_height;
  dst.w = sink->window->width;
  dst.h = sink->window->height;

  gst_video_sink_center_rect (src, dst, &res, FALSE);

  /* display the buffer stored in priv, if the buffer obtained returns a priv */
  if (priv) {
    wl_surface_attach (sink->window->surface, priv->buffer, res.x, res.y);
  } else {
    wl_surface_attach (sink->window->surface, meta->wbuffer, 0, 0);
  }

  wl_surface_damage (sink->window->surface, 0, 0, res.w, res.h);
  window->redraw_pending = TRUE;
  window->callback = wl_surface_frame (window->surface);
  wl_callback_add_listener (window->callback, &frame_callback_listener, window);
  wl_surface_commit (window->surface);
  wl_display_dispatch (display->display);

  gst_buffer_replace (&sink->last_buf, sink->display_buf);
  gst_buffer_replace (&sink->display_buf, to_render);

  if (buffer != to_render)
    gst_buffer_unref (to_render);
  return GST_FLOW_OK;

no_buffer:
  {
    GST_WARNING_OBJECT (sink, "could not create image");
    return ret;
  }
no_pool:
  {
    GST_ELEMENT_ERROR (sink, RESOURCE, WRITE,
        ("Internal error: can't allocate images"),
        ("We don't have a bufferpool negotiated"));
    return GST_FLOW_ERROR;
  }
activate_failed:
  {
    GST_ERROR_OBJECT (sink, "failed to activate bufferpool.");
    ret = GST_FLOW_ERROR;
    return ret;
  }
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gstwayland_debug, "waylandsink", 0,
      " wayland video sink");

  return gst_element_register (plugin, "waylandsink", GST_RANK_MARGINAL,
      GST_TYPE_WAYLAND_SINK);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    waylandsink,
    "Wayland Video Sink", plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME,
    GST_PACKAGE_ORIGIN)
