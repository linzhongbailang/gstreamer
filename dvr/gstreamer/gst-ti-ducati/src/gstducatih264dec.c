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
 * SECTION:element-ducatih264dec
 *
 * FIXME:Describe ducatih264dec here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! ducatih264dec ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include "gstducatih264dec.h"


#define GST_BUFFER_FLAG_B_FRAME (GST_BUFFER_FLAG_LAST << 0)
#define PADX  32
#define PADY  24

/* This structure is not public, this should be replaced by
   sizeof(sErrConcealLayerStr) when it is made so. */
#define SIZE_OF_CONCEALMENT_DATA 65536


static void gst_ducati_h264dec_base_init (gpointer gclass);
static void gst_ducati_h264dec_class_init (GstDucatiH264DecClass * klass);
static void gst_ducati_h264dec_init (GstDucatiH264Dec * self, gpointer klass);

static GstDucatiVidDecClass *parent_class = NULL;

GType
gst_ducati_h264dec_get_type (void)
{
  static GType ducati_h264dec_type = 0;

  if (!ducati_h264dec_type) {
    static const GTypeInfo ducati_h264dec_info = {
      sizeof (GstDucatiH264DecClass),
      (GBaseInitFunc) gst_ducati_h264dec_base_init,
      NULL,
      (GClassInitFunc) gst_ducati_h264dec_class_init,
      NULL,
      NULL,
      sizeof (GstDucatiH264Dec),
      0,
      (GInstanceInitFunc) gst_ducati_h264dec_init,
    };

    ducati_h264dec_type = g_type_register_static (GST_TYPE_DUCATIVIDDEC,
        "GstDucatiH264Dec", &ducati_h264dec_info, 0);
  }
  return ducati_h264dec_type;
}

/* *INDENT-OFF* */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-h264, "
        "stream-format = byte-stream, "   /* only byte-stream */
        "alignment = au, "          /* only entire frames */
        "width = (int)[ 16, 4320 ], "
        "height = (int)[ 16, 4096 ]; "
        "video/x-h264, "
        "stream-format = byte-stream, "   /* only byte-stream */
        "alignment = au, "          /* only entire frames */
        "width = (int)[ 16, 2048 ], "
        "height = (int)[ 16, 2048 ], "
        "profile = (string) {high, high-10-intra, high-10, high-4:2:2-intra, "
        "high-4:2:2, high-4:4:4-intra, high-4:4:4, cavlc-4:4:4-intra}, "
        "level = (string) {1, 1b, 1.1, 1.2, 1.3, 2, 2.1, 2.2, 3, 3.1, 3.2, 4, 4.1, 4.2, 5, 5.1};")
    );

static const struct
{
  const char *level;
  gint kb;
} max_dpb_by_level[] = {
  { "1", 149 },
  { "1b", 149 }, /* That one's not in the spec ?? */
  { "1.1", 338 },
  { "1.2", 891 },
  { "1.3", 891 },
  { "2", 891 },
  { "2.1", 1782 },
  { "2.2", 3038 },
  { "3", 3038 },
  { "3.1", 6750 },
  { "3.2", 7680 },
  { "4", 12288 },
  { "4.1", 12288 },
  { "4.2", 12288 },
  { "5", 41400 },
  { "5.1", 69120 },
};
/* *INDENT-ON* */

/* GstDucatiVidDec vmethod implementations */

static void
gst_ducati_h264dec_update_buffer_size (GstDucatiVidDec * self)
{
  gint w = self->width;
  gint h = self->height;

  /* calculate output buffer parameters: */
  self->padded_width = ALIGN2 (w + (2 * PADX), 7);
  self->padded_height = h + 4 * PADY;
  self->min_buffers = MIN (16, 32768 / ((w / 16) * (h / 16))) + 3;
}

static gboolean
gst_ducati_h264dec_allocate_params (GstDucatiVidDec * self, gint params_sz,
    gint dynparams_sz, gint status_sz, gint inargs_sz, gint outargs_sz)
{
  gboolean ret = GST_DUCATIVIDDEC_CLASS (parent_class)->allocate_params (self,
      sizeof (IH264VDEC_Params), sizeof (IH264VDEC_DynamicParams),
      sizeof (IH264VDEC_Status), sizeof (IH264VDEC_InArgs),
      sizeof (IH264VDEC_OutArgs));

  if (ret) {
    IH264VDEC_Params *params = (IH264VDEC_Params *) self->params;

    self->params->displayDelay = 6/*IVIDDEC3_DISPLAY_DELAY_AUTO*/;
    params->dpbSizeInFrames = 7/*IH264VDEC_DPB_NUMFRAMES_AUTO*/;
    params->pConstantMemory = 0;
    params->presetLevelIdc = IH264VDEC_LEVEL41;
    params->errConcealmentMode = IH264VDEC_APPLY_CONCEALMENT;
    params->temporalDirModePred = TRUE;

    if (self->codec_debug_info) {
      /* We must allocate a byte per MB, plus the size of some struture which
         is not public. Place this in the first metadata buffer slot, and ask
         for MBINFO metadata for it. */
      GstDucatiH264Dec *h264dec = GST_DUCATIH264DEC (self);
      unsigned mbw = (self->width + 15) / 16;
      unsigned mbh = (self->height + 15) / 16;
      unsigned nmb = mbw * mbh;

      h264dec->bo_mberror =
          omap_bo_new (self->device, nmb + SIZE_OF_CONCEALMENT_DATA,
          OMAP_BO_WC);
	  if (!h264dec->bo_mberror) {
	    GST_ERROR_OBJECT (self, "Failed to create bo");
	    return FALSE;
	  }	  
      /* XDM_MemoryType required by drm to allcoate buffer */
      self->outBufs->descs[2].memType = XDM_MEMTYPE_RAW;
      self->outBufs->descs[2].buf =
          (XDAS_Int8 *) omap_bo_dmabuf (h264dec->bo_mberror);
      self->outBufs->descs[2].bufSize.bytes = nmb + SIZE_OF_CONCEALMENT_DATA;
      self->params->metadataType[0] = IVIDEO_METADATAPLANE_MBINFO;
      /* need to lock the buffer to get the page pinned */
      dce_buf_lock (1, (size_t *) & self->outBufs->descs[2].buf);
    }
  }

  return ret;
}

static gint
gst_ducati_h264dec_handle_error (GstDucatiVidDec * self, gint ret,
    gint extended_error, gint status_extended_error)
{
  GstDucatiH264Dec *h264dec = GST_DUCATIH264DEC (self);
  const unsigned char *mberror, *mbcon;
  unsigned mbw, mbh, nmb;
  uint16_t mbwr, mbhr;
  size_t n, nerr = 0;
  char *line;
  unsigned x, y;

  if (h264dec->bo_mberror) {
    mberror = omap_bo_map (h264dec->bo_mberror);
    mbw = (self->width + 15) / 16;
    mbh = (self->height + 15) / 16;
    nmb = mbw * mbh;
    mbcon = mberror + nmb;
    mbwr = ((const uint16_t *) mbcon)[21];      /* not a public struct */
    mbhr = ((const uint16_t *) mbcon)[22];      /* not a public struct */
    if (nmb != mbwr * mbhr) {
      GST_WARNING_OBJECT (self, "Failed to find MB size - "
          "corruption might have happened");
    } else {
      for (n = 0; n < nmb; ++n) {
        if (mberror[n])
          ++nerr;
      }
      GST_INFO_OBJECT (self, "Frame has %zu MB errors over %zu (%u x %u) MBs",
          nerr, nmb, mbwr, mbhr);
      line = g_malloc (mbw + 1);
      for (y = 0; y < mbh; y++) {
        line[mbw] = 0;
        for (x = 0; x < mbw; x++) {
          line[x] = mberror[x + y * mbw] ? '!' : '.';
        }
        GST_INFO_OBJECT (self, "MB: %4u: %s", y, line);
      }
      g_free (line);
    }
  }

  if (extended_error & 0x00000001) {
    /* No valid slice. This seems to be bad enough that it's better to flush and
     * skip to the next keyframe.
     */

    if (extended_error == 0x00000201) {
      /* the codec doesn't unlock the input buffer in this case... */
      gst_buffer_unref ((GstBuffer *) self->inArgs->inputID);
      self->inArgs->inputID = 0;
    }

    self->needs_flushing = TRUE;
  }

  ret =
      GST_DUCATIVIDDEC_CLASS (parent_class)->handle_error (self, ret,
      extended_error, status_extended_error);

  return ret;
}

static gboolean
gst_ducati_h264dec_can_drop_frame (GstDucatiVidDec * self, GstBuffer * buf,
    gint64 diff)
{
  gboolean is_bframe = GST_BUFFER_FLAG_IS_SET (buf,
      GST_BUFFER_FLAG_B_FRAME);

  if (diff >= 0 && is_bframe)
    return TRUE;

  return FALSE;
}

static gint
gst_ducati_h264dec_find_max_dpb_from_level (GstDucatiVidDec * self,
    const char *level)
{
  guint n;

  for (n = 0; n < G_N_ELEMENTS (max_dpb_by_level); ++n)
    if (!strcmp (level, max_dpb_by_level[n].level))
      return max_dpb_by_level[n].kb;

  GST_WARNING_OBJECT (self, "Max DBP not found for level %s", level);
  return -1;
}

static gint
gst_ducati_h264dec_get_max_dpb_size (GstDucatiVidDec * self, GstCaps * caps)
{
  gint wmb = (self->width + 15) / 16;
  gint hmb = (self->height + 15) / 16;
  gint max_dpb, max_dpb_size;
  GstStructure *structure;
  const char *level;
  float chroma_factor = 1.5;    /* We only support NV12, which is 4:2:0 */

  /* Min( 1024 * MaxDPB / ( PicWidthInMbs * FrameHeightInMbs * 256 * ChromaFormatFactor ), 16 ) */

  structure = gst_caps_get_structure (caps, 0);
  if (!structure)
    return -1;

  level = gst_structure_get_string (structure, "level");
  if (!level)
    return -1;

  max_dpb = gst_ducati_h264dec_find_max_dpb_from_level (self, level);
  if (max_dpb < 0)
    return -1;

  max_dpb_size =
      lrint (ceil (1024 * max_dpb / (wmb * hmb * 256 * chroma_factor)));
  if (max_dpb_size > MAX_BACKLOG_FRAMES)
    max_dpb_size = MAX_BACKLOG_FRAMES;

  return max_dpb_size;
}

static gboolean
gst_ducati_h264dec_set_sink_caps (GstDucatiVidDec * self, GstCaps * caps)
{
  GstDucatiH264Dec *h264dec = GST_DUCATIH264DEC (self);
  IH264VDEC_Params *params = (IH264VDEC_Params *) self->params;
  GstStructure *structure;
  const char *level;
  structure = gst_caps_get_structure (caps, 0);
  level = gst_structure_get_string (structure, "level");

  if (level) {
    GST_DEBUG_OBJECT (self, "Level obtained from stream caps = %s", level);
    if (!strcmp (level, "5") || !strcmp (level, "5.1")) {
      if (params->presetLevelIdc != IH264VDEC_LEVEL51) {
        params->presetLevelIdc = IH264VDEC_LEVEL51;
        self->codec_create_params_changed = TRUE;
        GST_DEBUG_OBJECT (self,
            "Level was earlier set to 4.2, changing it to 5.1");
      }
    }
  }

  GST_DEBUG_OBJECT (self, "set_sink_caps: %" GST_PTR_FORMAT, caps);

  if (!GST_CALL_PARENT_WITH_DEFAULT (GST_DUCATIVIDDEC_CLASS, set_sink_caps,
          (self, caps), TRUE))
    return FALSE;

  /* HW decoder fails in GETSTATUS */
#if 0
  /* When we have the first decoded buffer, we ask the decoder for the
     max number of frames needed to reorder */
  int err = VIDDEC3_control (self->codec, XDM_GETSTATUS,
      self->dynParams, self->status);
  if (!err) {
    IH264VDEC_Status *s = (IH264VDEC_Status *) self->status;
    if (s->spsMaxRefFrames > MAX_BACKLOG_FRAMES) {
      h264dec->backlog_maxframes = MAX_BACKLOG_FRAMES;
      GST_WARNING_OBJECT (self,
          "Stream needs %d frames for reordering, we can only accomodate %d",
          s->spsMaxRefFrames, MAX_BACKLOG_FRAMES);
    } else {
      h264dec->backlog_maxframes = s->spsMaxRefFrames;
      GST_INFO_OBJECT (self, "Num frames for reordering: %d",
          h264dec->backlog_maxframes);
    }
  } else {
    h264dec->backlog_maxframes = MAX_BACKLOG_FRAMES;
    GST_WARNING_OBJECT (self,
        "Failed to request num frames for reordering, defaulting to %d",
        h264dec->backlog_maxframes);
  }
#endif

  self->backlog_maxframes = -1;

  structure = gst_caps_get_structure (caps, 0);
  if (structure) {
    gint num_ref_frames = -1, num_reorder_frames = -1;
    const char *profile;

    /* baseline profile does not use B frames (and I'll say constrained-baseline
       is unlikely either from the name, it's not present in my H264 spec... */
    profile = gst_structure_get_string (structure, "profile");
    if (profile && (!strcmp (profile, "baseline")
            || !strcmp (profile, "constrained-baseline"))) {
      GST_DEBUG_OBJECT (self, "No need for reordering for %s profile", profile);
      self->backlog_maxframes = 0;
      goto no_b_frames;
    }


    if (gst_structure_get_int (structure, "num-ref-frames", &num_ref_frames)
        && num_ref_frames >= 0) {
      ((IH264VDEC_Params *) self->params)->dpbSizeInFrames = num_ref_frames;
    }
    if (gst_structure_get_int (structure, "num-reorder-frames",
            &num_reorder_frames)
        && num_reorder_frames >= 0) {
      if (num_reorder_frames > MAX_BACKLOG_FRAMES) {
        self->backlog_maxframes = MAX_BACKLOG_FRAMES;
        GST_WARNING_OBJECT (self,
            "Stream needs %d frames for reordering, we can only accomodate %d",
            num_reorder_frames, MAX_BACKLOG_FRAMES);
      } else {
        self->backlog_maxframes = num_reorder_frames;
        GST_INFO_OBJECT (self, "Num frames for reordering: %d",
            self->backlog_maxframes);
      }
    }
  }

  /* If not present, use the spec forumula for a bound */
  if (self->backlog_maxframes < 0) {
    self->backlog_maxframes = gst_ducati_h264dec_get_max_dpb_size (self, caps);
    if (self->backlog_maxframes >= 0) {
      GST_WARNING_OBJECT (self,
          "num-reorder-frames not found on caps, calculation from stream parameters gives %d",
          self->backlog_maxframes);
    } else {
      self->backlog_maxframes = MAX_H264_BACKLOG_FRAMES;
    }
  }
  if (self->backlog_maxframes > self->backlog_max_maxframes)
    self->backlog_maxframes = self->backlog_max_maxframes;
  GST_WARNING_OBJECT (self,
      "Using %d frames for reordering", self->backlog_maxframes);

no_b_frames:

  return TRUE;
}

/* GObject vmethod implementations */

static void
gst_ducati_h264dec_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_set_static_metadata (element_class,
      "DucatiH264Dec",
      "Codec/Decoder/Video",
      "Decodes video in H.264/bytestream format with ducati",
      "Rob Clark <rob@ti.com>");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_factory));
}

static void
gst_ducati_h264dec_finalize (GObject * obj)
{
  GstDucatiH264Dec *self = GST_DUCATIH264DEC (obj);
  if (self->bo_mberror)
    omap_bo_del (self->bo_mberror);
  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
gst_ducati_h264dec_class_init (GstDucatiH264DecClass * klass)
{
  GstDucatiVidDecClass *bclass = GST_DUCATIVIDDEC_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  bclass->codec_name = "ivahd_h264dec";
  bclass->update_buffer_size =
      GST_DEBUG_FUNCPTR (gst_ducati_h264dec_update_buffer_size);
  bclass->allocate_params =
      GST_DEBUG_FUNCPTR (gst_ducati_h264dec_allocate_params);
  bclass->handle_error = GST_DEBUG_FUNCPTR (gst_ducati_h264dec_handle_error);
  bclass->can_drop_frame =
      GST_DEBUG_FUNCPTR (gst_ducati_h264dec_can_drop_frame);
  bclass->set_sink_caps = GST_DEBUG_FUNCPTR (gst_ducati_h264dec_set_sink_caps);
  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_ducati_h264dec_finalize);
}

static void
gst_ducati_h264dec_init (GstDucatiH264Dec * self, gpointer gclass)
{
#ifndef GST_DISABLE_GST_DEBUG
  GstDucatiVidDec *dec = GST_DUCATIVIDDEC (self);

  dec->error_strings[0] = "no error-free slice";
  dec->error_strings[1] = "error parsing SPS";
  dec->error_strings[2] = "error parsing PPS";
  dec->error_strings[3] = "error parsing slice header";
  dec->error_strings[4] = "error parsing MB data";
  dec->error_strings[5] = "unknown SPS";
  dec->error_strings[6] = "unknown PPS";
  dec->error_strings[7] = "invalid parameter";
  dec->error_strings[16] = "unsupported feature";
  dec->error_strings[17] = "SEI buffer overflow";
  dec->error_strings[18] = "stream end";
  dec->error_strings[19] = "no free buffers";
  dec->error_strings[20] = "resolution change";
  dec->error_strings[21] = "unsupported resolution";
  dec->error_strings[22] = "invalid maxNumRefFrames";
  dec->error_strings[23] = "invalid mbox message";
  dec->error_strings[24] = "bad datasync input";
  dec->error_strings[25] = "missing slice";
  dec->error_strings[26] = "bad datasync param";
  dec->error_strings[27] = "bad hw state";
  dec->error_strings[28] = "temporal direct mode";
  dec->error_strings[29] = "display width too small";
  dec->error_strings[30] = "no SPS/PPS header";
  dec->error_strings[31] = "gap in frame num";
#endif
}
