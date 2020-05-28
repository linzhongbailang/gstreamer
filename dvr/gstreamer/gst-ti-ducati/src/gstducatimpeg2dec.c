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
 * SECTION:element-ducatimpeg2dec
 *
 * FIXME:Describe ducatimpeg2dec here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! ducatimpeg2dec ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gstducatimpeg2dec.h"

static void gst_ducati_mpeg2dec_class_init (GstDucatiMpeg2DecClass * klass);
static void gst_ducati_mpeg2dec_init (GstDucatiMpeg2Dec * self, gpointer klass);
static void gst_ducati_mpeg2dec_base_init (gpointer gclass);
static GstDucatiVidDecClass *parent_class = NULL;

GType
gst_ducati_mpeg2dec_get_type (void)
{
  static GType ducati_mpeg2dec_type = 0;

  if (!ducati_mpeg2dec_type) {
    static const GTypeInfo ducati_mpeg2dec_info = {
      sizeof (GstDucatiMpeg2DecClass),
      (GBaseInitFunc) gst_ducati_mpeg2dec_base_init,
      NULL,
      (GClassInitFunc) gst_ducati_mpeg2dec_class_init,
      NULL,
      NULL,
      sizeof (GstDucatiMpeg2Dec),
      0,
      (GInstanceInitFunc) gst_ducati_mpeg2dec_init,
    };

    ducati_mpeg2dec_type = g_type_register_static (GST_TYPE_DUCATIVIDDEC,
        "GstDucatiMpeg2Dec", &ducati_mpeg2dec_info, 0);
  }
  return ducati_mpeg2dec_type;
}

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/mpeg, " "mpegversion = (int)[ 1, 2 ], "     // XXX check on MPEG-1..
        "systemstream = (boolean)false, "
        "parsed = (boolean)true, "
        "width = (int)[ 64, 2048 ], "
        "height = (int)[ 64, 2048 ], " "framerate = (fraction)[ 0, max ];")
    );

/* GstDucatiVidDec vmethod implementations */

static void
gst_ducati_mpeg2dec_update_buffer_size (GstDucatiVidDec * self)
{
  gint w = self->width;
  gint h = self->height;

  /* calculate output buffer parameters: */
  self->padded_width = w;
  self->padded_height = h;
  self->min_buffers = 8;
}

static gboolean
gst_ducati_mpeg2dec_allocate_params (GstDucatiVidDec * self, gint params_sz,
    gint dynparams_sz, gint status_sz, gint inargs_sz, gint outargs_sz)
{
  gboolean ret = GST_DUCATIVIDDEC_CLASS (parent_class)->allocate_params (self,
      sizeof (IVIDDEC3_Params), sizeof (IVIDDEC3_DynamicParams),
      sizeof (IVIDDEC3_Status), sizeof (IVIDDEC3_InArgs),
      sizeof (IVIDDEC3_OutArgs));

  if (ret)
    self->params->displayDelay = IVIDDEC3_DISPLAY_DELAY_AUTO;

  return ret;
}

static GstBuffer *
gst_ducati_mpeg2dec_push_input (GstDucatiVidDec * vdec, GstBuffer * buf)
{
  GstDucatiMpeg2Dec *self = GST_DUCATIMPEG2DEC (vdec);

  GstMapInfo info;
  gboolean mapped;
  mapped = gst_buffer_map (buf, &info, GST_MAP_READ);

  /* skip codec_data, which is same as first buffer from mpegvideoparse (and
   * appears to be periodically resent) and instead prepend to next frame..
   */
  if (vdec->codecdata && (info.size == vdec->codecdatasize) &&
      !memcmp (info.data, vdec->codecdata, info.size)) {
    GST_DEBUG_OBJECT (self, "skipping codec_data buffer");
    self->prepend_codec_data = TRUE;
  } else {
    if (self->prepend_codec_data) {
      GST_DEBUG_OBJECT (self, "prepending codec_data buffer");
      push_input (vdec, vdec->codecdata, vdec->codecdatasize);
      self->prepend_codec_data = FALSE;
    }
    if (mapped) {
      push_input (vdec, info.data, info.size);
    }
  }
  if (mapped) {
    gst_buffer_unmap (buf, &info);
  }
  gst_buffer_unref (buf);

  return NULL;
}

static gboolean
gst_ducati_mpeg2dec_set_sink_caps (GstDucatiVidDec * self, GstCaps * caps)
{
  GstStructure *structure;
  const gchar *profile;

  GST_DEBUG_OBJECT (self, "set_sink_caps: %" GST_PTR_FORMAT, caps);

  if (!GST_CALL_PARENT_WITH_DEFAULT (GST_DUCATIVIDDEC_CLASS, set_sink_caps,
          (self, caps), TRUE))
    return FALSE;

  /* I have at least one case where incoming timestamps are ordered,
     but frames are not, so we'd need to find out how to get correct
     ordering from the internals of the frames. For now, let the hw
     codec do the reordering to avoid more aggravation (and the 1.0
     port will let the codec do it anyway). */
#if 0
  /* Simple profile does not have B frames */
  structure = gst_caps_get_structure (caps, 0);
  profile = gst_structure_get_string (structure, "profile");
  if (!profile || strcmp (profile, "simple")) {
    /* TODO: can a better bound be found from stream headers ? */
    self->backlog_maxframes = self->backlog_max_maxframes;
  }
#endif

  return TRUE;
}

static GstFlowReturn
gst_ducati_mpeg2dec_push_output (GstDucatiVidDec * self, GstBuffer * buf)
{
  GST_BUFFER_OFFSET_END (buf) = GST_BUFFER_PTS (buf);
  return GST_DUCATIVIDDEC_CLASS (parent_class)->push_output (self, buf);
}

/* GObject vmethod implementations */

static void
gst_ducati_mpeg2dec_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_set_static_metadata (element_class,
      "DucatiMpeg2Dec",
      "Codec/Decoder/Video",
      "Decodes video in MPEG-2 format with ducati", "Rob Clark <rob@ti.com>");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_factory));
}

static void
gst_ducati_mpeg2dec_class_init (GstDucatiMpeg2DecClass * klass)
{
  GstDucatiVidDecClass *bclass = GST_DUCATIVIDDEC_CLASS (klass);
  parent_class = g_type_class_peek_parent (klass);
  bclass->codec_name = "ivahd_mpeg2vdec";
  bclass->update_buffer_size =
      GST_DEBUG_FUNCPTR (gst_ducati_mpeg2dec_update_buffer_size);
  bclass->allocate_params =
      GST_DEBUG_FUNCPTR (gst_ducati_mpeg2dec_allocate_params);
  bclass->push_input = GST_DEBUG_FUNCPTR (gst_ducati_mpeg2dec_push_input);
  bclass->set_sink_caps = GST_DEBUG_FUNCPTR (gst_ducati_mpeg2dec_set_sink_caps);
  bclass->push_output = GST_DEBUG_FUNCPTR (gst_ducati_mpeg2dec_push_output);
}

static void
gst_ducati_mpeg2dec_init (GstDucatiMpeg2Dec * self, gpointer gclass)
{
  GstDucatiVidDec *vdec = GST_DUCATIVIDDEC (self);
  vdec->pageMemType = XDM_MEMTYPE_RAW;
}
