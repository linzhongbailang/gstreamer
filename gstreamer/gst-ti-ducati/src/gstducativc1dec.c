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
 * SECTION:element-ducativc1dec
 *
 * FIXME:Describe ducativc1dec here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! ducativc1dec ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gstducativc1dec.h"


#define PADX  32
#define PADY  40
#define GST_BUFFER_FLAG_B_FRAME (GST_BUFFER_FLAG_LAST << 0)


static void gst_ducati_vc1dec_base_init (gpointer gclass);
static void gst_ducati_vc1dec_class_init (GstDucatiVC1DecClass * klass);
static void gst_ducati_vc1dec_init (GstDucatiVC1Dec * self, gpointer klass);

static GstDucatiVidDecClass *parent_class = NULL;

GType
gst_ducati_vc1dec_get_type (void)
{
  static GType ducati_vc1dec_type = 0;

  if (!ducati_vc1dec_type) {
    static const GTypeInfo ducati_vc1dec_info = {
      sizeof (GstDucatiVC1DecClass),
      (GBaseInitFunc) gst_ducati_vc1dec_base_init,
      NULL,
      (GClassInitFunc) gst_ducati_vc1dec_class_init,
      NULL,
      NULL,
      sizeof (GstDucatiVC1Dec),
      0,
      (GInstanceInitFunc) gst_ducati_vc1dec_init,
    };

    ducati_vc1dec_type = g_type_register_static (GST_TYPE_DUCATIVIDDEC,
        "GstDucatiVC1Dec", &ducati_vc1dec_info, 0);
  }
  return ducati_vc1dec_type;
}

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-wmv, "
        "wmvversion = (int) 3, "
        "format = (string){ WVC1, WMV3 }, "
        "width = (int)[ 16, 2048 ], "
        "height = (int)[ 16, 2048 ], " "framerate = (fraction)[ 0, max ];")
    );

/* GstDucatiVidDec vmethod implementations */

static gboolean
gst_ducati_vc1dec_parse_caps (GstDucatiVidDec * vdec, GstStructure * s)
{
  GstDucatiVC1Dec *self = GST_DUCATIVC1DEC (vdec);

  if (GST_DUCATIVIDDEC_CLASS (parent_class)->parse_caps (vdec, s)) {
    const gchar *format;
    gboolean ret = FALSE;
    format = gst_structure_get_string (s, "format");
    if (format) {
      if (strcmp (format, "WVC1") == 0) {

        ret = TRUE;
        self->level = 4;
        return ret;
      }

      if (strcmp (format, "WMV3") == 0) {
        ret = TRUE;
        self->level = 3;
        return ret;

      }
    }
    GST_INFO_OBJECT (vdec, "level %d", self->level);
  }

  return FALSE;
}

static void
gst_ducati_vc1dec_update_buffer_size (GstDucatiVidDec * self)
{
  gint w = self->width;
  gint h = self->height;

  /* calculate output buffer parameters: */
  self->padded_width = ALIGN2 (w + (2 * PADX), 7);
  self->padded_height = (ALIGN2 (h / 2, 4) * 2) + 4 * PADY;
  self->min_buffers = 8;
}

static gboolean
gst_ducati_vc1dec_allocate_params (GstDucatiVidDec * self, gint params_sz,
    gint dynparams_sz, gint status_sz, gint inargs_sz, gint outargs_sz)
{
  gboolean ret = GST_DUCATIVIDDEC_CLASS (parent_class)->allocate_params (self,
      sizeof (IVC1VDEC_Params), sizeof (IVC1VDEC_DynamicParams),
      sizeof (IVC1VDEC_Status), sizeof (IVC1VDEC_InArgs),
      sizeof (IVC1VDEC_OutArgs));

  if (ret) {
    IVC1VDEC_Params *params = (IVC1VDEC_Params *) self->params;
    self->params->maxBitRate = 45000000;
    self->params->displayDelay = IVIDDEC3_DISPLAY_DELAY_AUTO;

    /* this indicates whether buffers are prefixed with the frame layer struct
     * or not.  See Table 266: Frame Layer Data Structure from the spec */
    params->frameLayerDataPresentFlag = FALSE;

    /* enable concealment */
    params->errorConcealmentON = 1;

    /* codec wants lateAcquireArg = -1 */
    self->dynParams->lateAcquireArg = -1;
  }

  return ret;
}

static GstBuffer *
gst_ducati_vc1dec_push_input (GstDucatiVidDec * vdec, GstBuffer * buf)
{
  GstDucatiVC1Dec *self = GST_DUCATIVC1DEC (vdec);
  IVC1VDEC_Params *params = (IVC1VDEC_Params *) vdec->params;
  guint32 val;
  GstMapInfo info;
  gboolean mapped;

  /* need a base ts for frame layer timestamps */
  if (self->first_ts == GST_CLOCK_TIME_NONE)
    self->first_ts = GST_BUFFER_PTS (buf);

  if (G_UNLIKELY (vdec->first_in_buffer) && vdec->codecdata) {
    if (vdec->codecdatasize > 0) {
      /* There is at least one VC1 stream that claims it is simple profile,
         but goes on to have frames that use some feature that is unavailable
         in simple profile(intensity compensation). Since ducati supports
         both, we frob the header to claim all simple profile videos are
         main profile. This is a lie, but it should not cause any trouble
         (I'm sure all liars must say that). */

      if (!(vdec->codecdata[0] & 192))
        vdec->codecdata[0] |= 64;
    }

    if (self->level == 4) {
      /* for VC-1 Advanced Profile, strip off first byte, and
       * send rest of codec_data unmodified;
       */
      push_input (vdec, vdec->codecdata + 1, vdec->codecdatasize - 1);
    } else {
      /* for VC-1 Simple and Main Profile, build the Table 265 Sequence
       * Layer Data Structure header (refer to VC-1 spec, Annex L):
       */
      val = 0xc5ffffff;         /* we don't know the number of frames */
      push_input (vdec, (const guint8 *) &val, 4);

      /* STRUCT_C (preceded by length).. see Table 263, 264 */
      val = 0x00000004;
      push_input (vdec, (const guint8 *) &val, 4);

      val = GST_READ_UINT32_LE (vdec->codecdata);
      /* FIXME: i have NO idea why asfdemux gives me something I need to patch... */
      val |= 0x01 << 24;
      push_input (vdec, (const guint8 *) &val, 4);
      /* STRUCT_A.. see Table 260 and Annex J.2 */
      val = vdec->height;
      push_input (vdec, (const guint8 *) &val, 4);
      val = vdec->width;
      push_input (vdec, (const guint8 *) &val, 4);
      GST_INFO_OBJECT (vdec, "seq hdr resolution: %dx%d", vdec->width,
          vdec->height);

      val = 0x0000000c;
      push_input (vdec, (const guint8 *) &val, 4);

      /* STRUCT_B.. see Table 261, 262 */
      val = 0x00000000;         /* not sure how to populate, but codec ignores anyways */
      push_input (vdec, (const guint8 *) &val, 4);
      push_input (vdec, (const guint8 *) &val, 4);
      push_input (vdec, (const guint8 *) &val, 4);
    }
  }

  /* VC-1 Advanced profile needs start-code prepended: */
  if (self->level == 4) {
    static const guint8 sc[] = { 0x00, 0x00, 0x01, 0x0d };      /* start code */
    push_input (vdec, sc, sizeof (sc));
  }

  if (params->frameLayerDataPresentFlag) {
    val = gst_buffer_get_sizes (buf, NULL, NULL);
    if (!GST_BUFFER_FLAG_IS_SET (buf, GST_BUFFER_FLAG_DELTA_UNIT))
      val |= 0x80 << 24;
    else
      val |= 0x00 << 24;
    push_input (vdec, (const guint8 *) &val, 4);
    val = GST_TIME_AS_MSECONDS (GST_BUFFER_PTS (buf) - self->first_ts);
    push_input (vdec, (const guint8 *) &val, 4);
  }

  mapped = gst_buffer_map (buf, &info, GST_MAP_READ);
  if (mapped) {
    push_input (vdec, info.data, info.size);
    gst_buffer_unmap (buf, &info);
  }
  gst_buffer_unref (buf);

  return NULL;
}

static gint
gst_ducati_vc1dec_handle_error (GstDucatiVidDec * self, gint ret,
    gint extended_error, gint status_extended_error)
{
  if (extended_error == 0x00409000)
    /* the codec sets some IVC1DEC_ERR_PICHDR (corrupted picture headers) errors
     * as fatal even though it's able to recover 
     */
    ret = XDM_EOK;
  else
    ret =
        GST_DUCATIVIDDEC_CLASS (parent_class)->handle_error (self, ret,
        extended_error, status_extended_error);

  return ret;
}

static gboolean
gst_ducati_vc1dec_can_drop_frame (GstDucatiVidDec * self, GstBuffer * buf,
    gint64 diff)
{
  gboolean is_bframe = GST_BUFFER_FLAG_IS_SET (buf,
      GST_BUFFER_FLAG_B_FRAME);

  if (diff >= 0 && is_bframe)
    return TRUE;

  return FALSE;
}

/* GstElement vmethod implementations */

static GstStateChangeReturn
gst_ducati_vc1dec_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstDucatiVC1Dec *self = GST_DUCATIVC1DEC (element);

  GST_INFO_OBJECT (self, "begin: changing state %s -> %s",
      gst_element_state_get_name (GST_STATE_TRANSITION_CURRENT (transition)),
      gst_element_state_get_name (GST_STATE_TRANSITION_NEXT (transition)));

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  if (ret == GST_STATE_CHANGE_FAILURE)
    goto leave;

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      self->level = -1;
      self->first_ts = GST_CLOCK_TIME_NONE;
      break;
    default:
      break;
  }

leave:
  GST_LOG_OBJECT (self, "end");

  return ret;
}

/* GObject vmethod implementations */

static void
gst_ducati_vc1dec_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_set_static_metadata (element_class,
      "DucatiVC1Dec",
      "Codec/Decoder/Video",
      "Decodes video in WMV3/VC-1 format with ducati",
      "Rob Clark <rob@ti.com>");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_factory));
}

static void
gst_ducati_vc1dec_class_init (GstDucatiVC1DecClass * klass)
{
  GstDucatiVidDecClass *bclass = GST_DUCATIVIDDEC_CLASS (klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);
  parent_class = g_type_class_peek_parent (klass);

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_ducati_vc1dec_change_state);

  bclass->codec_name = "ivahd_vc1vdec";
  bclass->parse_caps = GST_DEBUG_FUNCPTR (gst_ducati_vc1dec_parse_caps);
  bclass->update_buffer_size =
      GST_DEBUG_FUNCPTR (gst_ducati_vc1dec_update_buffer_size);
  bclass->allocate_params =
      GST_DEBUG_FUNCPTR (gst_ducati_vc1dec_allocate_params);
  bclass->push_input = GST_DEBUG_FUNCPTR (gst_ducati_vc1dec_push_input);
  bclass->handle_error = GST_DEBUG_FUNCPTR (gst_ducati_vc1dec_handle_error);
  bclass->can_drop_frame = GST_DEBUG_FUNCPTR (gst_ducati_vc1dec_can_drop_frame);
}

static void
gst_ducati_vc1dec_init (GstDucatiVC1Dec * self, gpointer gclass)
{
  GstDucatiVidDec *vdec = GST_DUCATIVIDDEC (self);

#ifndef GST_DISABLE_GST_DEBUG
  vdec->error_strings[0] = "unsupported VIDDEC3 params";
  vdec->error_strings[1] = "unsupported dynamic VIDDEC3 params";
  vdec->error_strings[2] = "unsupported VC1 VIDDEC3 params";
  vdec->error_strings[3] = "bad datasync settings";
  vdec->error_strings[4] = "no slice";
  vdec->error_strings[5] = "corrupted slice header";
  vdec->error_strings[6] = "corrupted MB data";
  vdec->error_strings[7] = "unsupported VC1 feature";
  vdec->error_strings[16] = "stream end";
  vdec->error_strings[17] = "unsupported resolution";
  vdec->error_strings[18] = "IVA standby";
  vdec->error_strings[19] = "invalid mbox message";
  vdec->error_strings[20] = "corrupted sequence header";
  vdec->error_strings[21] = "corrupted entry point header";
  vdec->error_strings[22] = "corrupted picture header";
  vdec->error_strings[23] = "ref picture buffer error";
  vdec->error_strings[24] = "no sequence header";
  vdec->error_strings[30] = "invalid buffer descriptor";
  vdec->error_strings[31] = "pic size change";
#endif

  self->level = -1;
  self->first_ts = GST_CLOCK_TIME_NONE;
  vdec->pageMemType = XDM_MEMTYPE_RAW;
}
