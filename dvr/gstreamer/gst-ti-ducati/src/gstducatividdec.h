/*
 * GStreamer
 * Copyright (c) 2010, Texas Instruments Incorporated
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

#ifndef __GST_DUCATIVIDDEC_H__
#define __GST_DUCATIVIDDEC_H__

#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <omap_drm.h>
#include <omap_drmif.h>

#include "gstducati.h"
#include "gstducatibufferpriv.h"

#include <gst/drm/gstdrmbufferpool.h>
#include <gst/video/video.h>
#include <gst/video/gstvideometa.h>

G_BEGIN_DECLS
#define GST_TYPE_DUCATIVIDDEC               (gst_ducati_viddec_get_type())
#define GST_DUCATIVIDDEC(obj)               (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_DUCATIVIDDEC, GstDucatiVidDec))
#define GST_DUCATIVIDDEC_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_DUCATIVIDDEC, GstDucatiVidDecClass))
#define GST_IS_DUCATIVIDDEC(obj)            (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_DUCATIVIDDEC))
#define GST_IS_DUCATIVIDDEC_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_DUCATIVIDDEC))
#define GST_DUCATIVIDDEC_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_DUCATIVIDDEC, GstDucatiVidDecClass))
typedef struct _GstDucatiVidDec GstDucatiVidDec;
typedef struct _GstDucatiVidDecClass GstDucatiVidDecClass;

/* For re-ordering in normal playback */
#define MAX_BACKLOG_FRAMES 16
/* For re-ordering in reverse playback */
#define MAX_BACKLOG_ARRAY_SIZE 120

struct _GstDucatiVidDec
{
  GstElement parent;

  GstPad *sinkpad, *srcpad;

  GstDRMBufferPool *pool;

  /* minimum output size required by the codec: */
  gint outsize;

  /* minimum number of buffers required by the codec: */
  gint min_buffers;

  /* input (unpadded, unaligned) size of video: */
  gint input_width, input_height;

  /* input (unpadded, aligned to MB) size of video: */
  gint width, height;

  gint fps_n, fps_d;

  /* output (padded) size including any codec padding: */
  gint padded_width, padded_height;

  /* output stride (>= padded_width) */
  gint stride;

  gboolean interlaced;

  struct omap_bo *input_bo;
  /* input buffer, allocated when codec is created: */
  guint8 *input;

  /* number of bytes pushed to input on current frame: */
  gint in_size;

  /* on first output buffer, we need to send crop info to sink.. and some
   * operations like flushing should be avoided if we haven't sent any
   * input buffers:
   */
  gboolean first_out_buffer, first_in_buffer;

  GstSegment segment;
  gdouble qos_proportion;
  GstClockTime qos_earliest_time;

  gboolean need_out_buf;

  /* For storing the external pool query history */
  gboolean queried_external_pool;
  GstBufferPool *externalpool;

  /* by default, codec_data from sinkpad is prepended to first buffer: */

  guint8 *codecdata;
  gsize codecdatasize;

  /* workaround enabled to indicate that timestamp from demuxer is PTS,
   * not DTS (cough, cough.. avi):
   */
  gboolean ts_is_pts;

  /* auto-detection for ts_is_pts workaround.. if we detect out of order
   * timestamps from demuxer/parser, then the ts is definitely DTS,
   * otherwise it may be PTS and out of order timestamps out of decoder
   * will trigger the ts_is_pts workaround.
   */
  gboolean ts_may_be_pts;

  gboolean wait_keyframe;

  gboolean needs_flushing;

  gboolean codec_create_params_changed;

  GHashTable *passed_in_bufs;

  GHashTable *dce_locked_bufs;

#define NDTS 32
  GstClockTime dts_queue[NDTS];
  gint dts_ridx, dts_widx;
  GstClockTime last_dts, last_pts;

  Engine_Handle engine;
  VIDDEC3_Handle codec;
  VIDDEC3_Params *params;
  VIDDEC3_DynamicParams *dynParams;
  VIDDEC3_Status *status;
  XDM2_BufDesc *inBufs;
  XDM2_BufDesc *outBufs;
  VIDDEC3_InArgs *inArgs;
  VIDDEC3_OutArgs *outArgs;

  XDAS_Int16 pageMemType;
  struct omap_device *device;

  GstCaps *sinkcaps;

  /* Frames waiting to be reordered */
  GstBuffer *backlog_frames[MAX_BACKLOG_ARRAY_SIZE + 1];
  gint backlog_maxframes;
  gint backlog_nframes;
  gint backlog_max_maxframes;

  gboolean codec_debug_info;

  const char *error_strings[32];
  
  GQueue can_queue;
};

struct _GstDucatiVidDecClass
{
  GstElementClass parent_class;

  const gchar *codec_name;

  /**
   * Parse codec specific fields the given caps structure.  The base-
   * class implementation of this method handles standard stuff like
   * width/height/framerate/codec_data.
   */
    gboolean (*parse_caps) (GstDucatiVidDec * self, GstStructure * s);

  /**
   * Called when the input buffer size changes, to recalculate codec required
   * output buffer size and minimum count
   */
  void (*update_buffer_size) (GstDucatiVidDec * self);

  /**
   * Called to allocate/initialize  params/dynParams/status/inArgs/outArgs
   */
    gboolean (*allocate_params) (GstDucatiVidDec * self, gint params_sz,
      gint dynparams_sz, gint status_sz, gint inargs_sz, gint outargs_sz);

  /**
   * Push input data into codec's input buffer, returning a sub-buffer of
   * any remaining data, or NULL if none.  Consumes reference to 'buf'
   */
  GstBuffer *(*push_input) (GstDucatiVidDec * self, GstBuffer * buf);

  /**
   * Called to handle errors returned by VIDDEC3_process.
   */
    gint (*handle_error) (GstDucatiVidDec * self, gint ret, gint extended_error,
      gint status_extended_error);

  /**
   * Called to check whether it's a good idea to drop buf or not.
   */
    gboolean (*can_drop_frame) (GstDucatiVidDec * self, GstBuffer * buf,
      gint64 diff);

    gboolean (*query) (GstDucatiVidDec * self, GstPad * pad, GstQuery * query,
      gboolean * forward);

  /**
   * Called to push a decoder buffer. Consumes reference to 'buf'.
   */
    GstFlowReturn (*push_output) (GstDucatiVidDec * self, GstBuffer * buf);

  /**
   * Called before a flush happens.
   */
  void (*on_flush) (GstDucatiVidDec * self, gboolean eos);

  /**
   * Called to set new caps on the sink pad.
   */
    gboolean (*set_sink_caps) (GstDucatiVidDec * self, GstCaps * caps);
};

GType gst_ducati_viddec_get_type (void);

/* helper methods for derived classes: */

static inline void
push_input (GstDucatiVidDec * self, const guint8 * in, gint sz)
{
  GST_DEBUG_OBJECT (self, "push: %d bytes)", sz);
  memcpy (self->input + self->in_size, in, sz);
  self->in_size += sz;
}

static inline int
check_start_code (const guint8 * sc, gint scsize,
    const guint8 * inbuf, gint insize)
{
  if (insize < scsize)
    return FALSE;

  while (scsize) {
    if (*sc != *inbuf)
      return FALSE;
    scsize--;
    sc++;
    inbuf++;
  }

  return TRUE;
}

static inline int
find_start_code (const guint8 * sc, gint scsize,
    const guint8 * inbuf, gint insize)
{
  gint size = 0;
  while (insize) {
    if (check_start_code (sc, scsize, inbuf, insize))
      break;
    insize--;
    size++;
    inbuf++;
  }
  return size;
}

gboolean gst_ducati_viddec_codec_flush (GstDucatiVidDec * self, gboolean eos);

G_END_DECLS
#endif /* __GST_DUCATIVIDDEC_H__ */
