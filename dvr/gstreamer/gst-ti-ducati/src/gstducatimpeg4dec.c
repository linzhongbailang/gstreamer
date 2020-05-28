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

/**
 * SECTION:element-ducatimpeg4dec
 *
 * FIXME:Describe ducatimpeg4dec here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! ducatimpeg4dec ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gstducatimpeg4dec.h"

#include <math.h>


#define PADX  32
#define PADY  32


static void gst_ducati_mpeg4dec_class_init (GstDucatiMpeg4DecClass * klass);
static void gst_ducati_mpeg4dec_init (GstDucatiMpeg4Dec * self, gpointer klass);
static void gst_ducati_mpeg4dec_base_init (gpointer gclass);
static GstDucatiVidDecClass *parent_class = NULL;

GType
gst_ducati_mpeg4dec_get_type (void)
{
  static GType ducati_mpeg4dec_type = 0;

  if (!ducati_mpeg4dec_type) {
    static const GTypeInfo ducati_mpeg4dec_info = {
      sizeof (GstDucatiMpeg4DecClass),
      (GBaseInitFunc) gst_ducati_mpeg4dec_base_init,
      NULL,
      (GClassInitFunc) gst_ducati_mpeg4dec_class_init,
      NULL,
      NULL,
      sizeof (GstDucatiMpeg4Dec),
      0,
      (GInstanceInitFunc) gst_ducati_mpeg4dec_init,
    };

    ducati_mpeg4dec_type = g_type_register_static (GST_TYPE_DUCATIVIDDEC,
        "GstDucatiMpeg4Dec", &ducati_mpeg4dec_info, 0);
  }
  return ducati_mpeg4dec_type;
}

#define MPEG4DEC_SINKCAPS_COMMON \
    "width = (int)[ 16, 2048 ], " \
    "height = (int)[ 16, 2048 ], " \
    "framerate = (fraction)[ 0, max ]"

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/mpeg, " "mpegversion = (int)4, " "systemstream = (boolean)false, " MPEG4DEC_SINKCAPS_COMMON ";" "video/x-divx, " "divxversion = (int)[4, 5], "      /* TODO check this */
        MPEG4DEC_SINKCAPS_COMMON ";"
        "video/x-xvid, "
        MPEG4DEC_SINKCAPS_COMMON ";"
        "video/x-3ivx, " MPEG4DEC_SINKCAPS_COMMON ";")
    );

/* GstDucatiVidDec vmethod implementations */

static gboolean
gst_ducati_mpeg4dec_can_drop_frame (GstDucatiVidDec * self, GstBuffer * buf,
    gint64 diff)
{
  /* The mpeg4 video parser element (mpeg4videoparse) does not do a correct job
     in identifying key-frames for some interlaced streams.
     This produces undesirable artifacts when frames are dropped due to QOS.
     This workaround asks the base class to not drop (avoid decoding) any mpeg4 
     frames based on buffer flags.
     TODO: Implement a parser to check if a frame is skippable or not
   */
  return FALSE;
}

static void
gst_ducati_mpeg4dec_update_buffer_size (GstDucatiVidDec * self)
{
  gint w = self->width;
  gint h = self->height;

  /* calculate output buffer parameters: */
  self->padded_width = ALIGN2 (w + PADX, 7);
  self->padded_height = h + PADY;
  self->min_buffers = 8;
}

static gboolean
gst_ducati_mpeg4dec_allocate_params (GstDucatiVidDec * self, gint params_sz,
    gint dynparams_sz, gint status_sz, gint inargs_sz, gint outargs_sz)
{
  gboolean ret = GST_DUCATIVIDDEC_CLASS (parent_class)->allocate_params (self,
      sizeof (IMPEG4VDEC_Params), sizeof (IMPEG4VDEC_DynamicParams),
      sizeof (IMPEG4VDEC_Status), sizeof (IMPEG4VDEC_InArgs),
      sizeof (IMPEG4VDEC_OutArgs));

  if (ret) {
    /* NB: custom params are needed as base params seem to break xvid */
    IMPEG4VDEC_Params *params = (IMPEG4VDEC_Params *) self->params;
    self->params->displayDelay = IVIDDEC3_DISPLAY_DELAY_AUTO;
    self->dynParams->lateAcquireArg = -1;
    params->outloopDeBlocking = FALSE;
    params->sorensonSparkStream = FALSE;
    params->errorConcealmentEnable = FALSE;
  }

  return ret;
}

#define VOS_START_CODE    0xb0  /* + visual object sequence */
#define VOS_END_CODE      0xb1
#define UD_START_CODE     0xb2  /* user data */
#define GVOP_START_CODE   0xb3  /* + group of VOP */
#define VS_ERROR_CODE     0xb4
#define VO_START_CODE     0xb5  /* visual object */
#define VOP_START_CODE    0xb6  /* + */

static const guint8 sc[] = { 0x00, 0x00, 0x01 };        /* start code */

#define SC_SZ                G_N_ELEMENTS (sc)  /* start code size */

static GstBitReader *
get_bit_reader (GstDucatiMpeg4Dec * self, const guint8 * data, guint size)
{
  if (self->br) {
    gst_bit_reader_init (self->br, data, size);
  } else {
    self->br = gst_bit_reader_new (data, size);
  }
  return self->br;
}

static void
decode_vol_header (GstDucatiMpeg4Dec * self, const guint8 * data, guint size)
{
  GstBitReader *br = get_bit_reader (self, data, size);
  guint32 is_oli = 0, vc_param = 0, vbv_param = 0;
  guint32 ar_info = 0, vop_tir = 0;

  gst_bit_reader_skip (br, 1);  /* random_accessible_vol */
  gst_bit_reader_skip (br, 8);  /* video_object_type_indication */

  gst_bit_reader_get_bits_uint32 (br,   /* is_object_layer_identifier */
      &is_oli, 1);
  if (is_oli) {
    gst_bit_reader_skip (br, 4);        /* video_object_layer_verid */
    gst_bit_reader_skip (br, 3);        /* video_object_layer_priority */
  }

  gst_bit_reader_get_bits_uint32 (br,   /* aspect_ratio_info */
      &ar_info, 4);
  if (ar_info == 0xf) {
    gst_bit_reader_skip (br, 8);        /* par_width */
    gst_bit_reader_skip (br, 8);        /* par_height */
  }

  gst_bit_reader_get_bits_uint32 (br,   /* vol_control_parameters */
      &vc_param, 1);
  if (vc_param) {
    gst_bit_reader_skip (br, 2);        /* chroma_format */
    gst_bit_reader_skip (br, 1);        /* low_delay */
    gst_bit_reader_get_bits_uint32 (    /* vbv_parameters */
        br, &vbv_param, 1);
    if (vbv_param) {
      gst_bit_reader_skip (br, 79);     /* don't care */
    }
  }

  gst_bit_reader_skip (br, 2);  /* video_object_layer_shape */
  gst_bit_reader_skip (br, 1);  /* marker_bit */
  gst_bit_reader_get_bits_uint32 (br,   /* vop_time_increment_resolution */
      &vop_tir, 16);
  gst_bit_reader_skip (br, 1);  /* marker_bit */

  self->time_increment_bits = (guint32) log2 ((double) (vop_tir - 1)) + 1;

  GST_DEBUG_OBJECT (self, "vop_tir=%d, time_increment_bits=%d",
      vop_tir, self->time_increment_bits);

  if (self->time_increment_bits < 1)
    self->time_increment_bits = 1;

  /* we don't care about anything beyond here */
}

static void
decode_user_data (GstDucatiMpeg4Dec * self, const guint8 * data, guint size)
{
  GstDucatiVidDec *vdec = GST_DUCATIVIDDEC (self);
  const char *buf = (const char *) data;
  int n, ver, build;
  char c;

  /* divx detection: */
  n = sscanf (buf, "DivX%dBuild%d%c", &ver, &build, &c);
  if (n < 2)
    n = sscanf (buf, "DivX%db%d%c", &ver, &build, &c);
  if (n >= 2) {
    GST_INFO_OBJECT (self, "DivX: version %d, build %d", ver, build);
    if ((n == 3) && (c == 'p')) {
      GST_INFO_OBJECT (self, "detected packed B frames");
      /* enable workarounds: */
      vdec->ts_is_pts = TRUE;
    }
  }

  /* xvid detection: */
  n = sscanf (buf, "XviD%d", &build);
  if (n == 1) {
    GST_INFO_OBJECT (self, "XviD: build %d", build);
    /* I believe we only get this in avi container, which means
     * we also need to enable the workarounds:
     */
    vdec->ts_is_pts = TRUE;
  }
}

static gboolean
is_vop_coded (GstDucatiMpeg4Dec * self, const guint8 * data, guint size)
{
  GstBitReader *br = get_bit_reader (self, data, size);
  guint32 b = 0;

  gst_bit_reader_skip (br, 2);  /* vop_coding_type */

  do {                          /* modulo_time_base */
    gst_bit_reader_get_bits_uint32 (br, &b, 1);
  } while (b != 0);

  gst_bit_reader_skip (br, 1);  /* marker_bit */
  gst_bit_reader_skip (br,      /* vop_time_increment */
      self->time_increment_bits);
  gst_bit_reader_skip (br, 1);  /* marker_bit */
  gst_bit_reader_get_bits_uint32 (br,   /* vop_coded */
      &b, 1);

  return b;
}

static GstBuffer *
gst_ducati_mpeg4dec_push_input (GstDucatiVidDec * vdec, GstBuffer * buf)
{
  GstDucatiMpeg4Dec *self = GST_DUCATIMPEG4DEC (vdec);
  GstBuffer *remaining = NULL;
  gint insize = 0;
  guint8 *in = NULL;
  gint size = 0;
  guint8 last_start_code = 0xff;
  GstMapInfo info;
  gboolean mapped;
  mapped = gst_buffer_map (buf, &info, GST_MAP_READ);
  if (mapped) {
    in = info.data;
    insize = info.size;
  }
  if (G_UNLIKELY (vdec->first_in_buffer) && vdec->codecdata) {
    push_input (vdec, vdec->codecdata, vdec->codecdatasize);
  }

  while (insize > (SC_SZ + 1)) {
    gint nal_size;
    guint8 start_code = in[SC_SZ];
    gboolean skip = FALSE;

    GST_DEBUG_OBJECT (self, "start_code: %02x", start_code);

    if (size > 0) {
      /* check if we've found a potential start of frame: */
      if ((start_code == VOS_START_CODE) || (start_code == GVOP_START_CODE) || (start_code == VOP_START_CODE) || (start_code <= 0x1f)) {        /* 00->0f is video_object_start_code */
        /* if last was a VOP, or if this is first VOP, then what follows
         * must be the next frame:
         */
        if (((last_start_code == 0xff) && (start_code == VOP_START_CODE)) ||
            (last_start_code == VOP_START_CODE)) {
          GST_DEBUG_OBJECT (self, "found end");
          break;
        }
      } else if ((0x20 <= start_code) && (start_code <= 0x2f)) {
        decode_vol_header (self, in + SC_SZ + 1, insize - SC_SZ - 1);
      }
    }

    last_start_code = start_code;

    nal_size = SC_SZ + find_start_code (sc, SC_SZ, in + SC_SZ, insize - SC_SZ);

    if ((start_code == VOP_START_CODE) && (nal_size < 20)) {
      /* suspiciously small nal..  check for !vop_coded and filter
       * that out to avoid upsetting the decoder:
       *
       * XXX 20 is arbitrary value, but I want to avoid having
       * to parse every VOP.. the non-coded VOP's I'm seeing
       * are all 7 bytes but need to come up with some sane
       * threshold
       */
      skip = !is_vop_coded (self, in + SC_SZ + 1, insize - SC_SZ - 1);
      if (skip)
        GST_DEBUG_OBJECT (self, "skipping non-coded VOP");
    } else if (start_code == UD_START_CODE) {
      decode_user_data (self, in + SC_SZ + 1, nal_size - SC_SZ - 1);
    }

    if (!skip)
      push_input (vdec, in, nal_size);

    in += nal_size;
    insize -= nal_size;
    size += nal_size;
  }

  /* if there are remaining bytes, wrap those back as a buffer
   * for the next go around:
   */
  if (insize > 0) {
    remaining =
        gst_buffer_copy_region (buf, GST_BUFFER_COPY_DEEP, size, insize);

    GST_BUFFER_DURATION (remaining) = GST_BUFFER_DURATION (buf);
    if (vdec->ts_is_pts) {
      GST_BUFFER_PTS (remaining) = GST_CLOCK_TIME_NONE;
    } else {
      GST_BUFFER_PTS (remaining) = GST_BUFFER_PTS (buf) +
          GST_BUFFER_DURATION (buf);
    }
  }
  if (mapped) {
    gst_buffer_unmap (buf, &info);
  }
  gst_buffer_unref (buf);

  return remaining;
}

/* GObject vmethod implementations */

static void
gst_ducati_mpeg4dec_finalize (GObject * obj)
{
  GstDucatiMpeg4Dec *self = GST_DUCATIMPEG4DEC (obj);
  if (self->br)
    gst_bit_reader_free (self->br);
  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
gst_ducati_mpeg4dec_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_set_static_metadata (element_class,
      "DucatiMpeg4Dec",
      "Codec/Decoder/Video",
      "Decodes video in MPEG-4 format with ducati", "Rob Clark <rob@ti.com>");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_factory));
}

static void
gst_ducati_mpeg4dec_class_init (GstDucatiMpeg4DecClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstDucatiVidDecClass *bclass = GST_DUCATIVIDDEC_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_ducati_mpeg4dec_finalize);

  bclass->codec_name = "ivahd_mpeg4dec";
  bclass->update_buffer_size =
      GST_DEBUG_FUNCPTR (gst_ducati_mpeg4dec_update_buffer_size);
  bclass->allocate_params =
      GST_DEBUG_FUNCPTR (gst_ducati_mpeg4dec_allocate_params);
  bclass->push_input = GST_DEBUG_FUNCPTR (gst_ducati_mpeg4dec_push_input);
  bclass->can_drop_frame =
      GST_DEBUG_FUNCPTR (gst_ducati_mpeg4dec_can_drop_frame);
}

static void
gst_ducati_mpeg4dec_init (GstDucatiMpeg4Dec * self, gpointer gclass)
{
#ifndef GST_DISABLE_GST_DEBUG
  GstDucatiVidDec *dec = GST_DUCATIVIDDEC (self);

  dec->error_strings[0] = "no video object sequence found";
  dec->error_strings[1] = "incorrect video object type";
  dec->error_strings[2] = "error in video object layer";
  dec->error_strings[3] = "error parsing group of video";
  dec->error_strings[4] = "error parsing video object plane";
  dec->error_strings[5] = "error in short header parsing";
  dec->error_strings[6] = "error in GOB parsing";
  dec->error_strings[7] = "error in video packet parsing";
  dec->error_strings[16] = "error in MB data parsing";
  dec->error_strings[17] = "invalid parameter";
  dec->error_strings[18] = "unsupported feature";
  dec->error_strings[19] = "stream end";
  dec->error_strings[20] = "valid header not found";
  dec->error_strings[21] = "unsupported resolution";
  dec->error_strings[22] = "stream buffer underflow";
  dec->error_strings[23] = "invalid mbox message";
  dec->error_strings[24] = "no frame to flush";
  dec->error_strings[25] = "given vop is not codec";
  dec->error_strings[26] = "start code not present";
  dec->error_strings[27] = "unsupported time increment resolution";
  dec->error_strings[28] = "resolution change";
  dec->error_strings[29] = "unsupported H263 annex";
  dec->error_strings[30] = "bad HDVICP2 state";
  dec->error_strings[31] = "frame dropped";
#endif
}
