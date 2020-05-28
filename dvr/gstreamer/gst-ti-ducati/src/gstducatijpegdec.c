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
 * SECTION:element-ducatijpegdec
 *
 * FIXME:Describe ducatijpegdec here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! ducatijpegdec ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gstducatijpegdec.h"

static void gst_ducati_jpegdec_base_init (gpointer gclass);
static void gst_ducati_jpegdec_class_init (GstDucatiJpegDecClass * klass);
static void gst_ducati_jpegdec_init (GstDucatiJpegDec * self, gpointer klass);

static GstDucatiVidDecClass *parent_class = NULL;

GType
gst_ducati_jpegdec_get_type (void)
{
  static GType ducati_jpegdec_type = 0;

  if (!ducati_jpegdec_type) {
    static const GTypeInfo ducati_jpegdec_info = {
      sizeof (GstDucatiJpegDecClass),
      (GBaseInitFunc) gst_ducati_jpegdec_base_init,
      NULL,
      (GClassInitFunc) gst_ducati_jpegdec_class_init,
      NULL,
      NULL,
      sizeof (GstDucatiJpegDec),
      0,
      (GInstanceInitFunc) gst_ducati_jpegdec_init,
    };

    ducati_jpegdec_type = g_type_register_static (GST_TYPE_DUCATIVIDDEC,
        "GstDucatiJpegDec", &ducati_jpegdec_info, 0);
  }
  return ducati_jpegdec_type;
}

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("image/jpeg, "
        "parsed = (boolean)true, "
        "width = (int)[ 32, 4096 ], "
        "height = (int)[ 32, 4096 ], " "framerate = (fraction)[ 0, max ];")
    );

/* GstDucatiVidDec vmethod implementations */

static void
gst_ducati_jpegdec_update_buffer_size (GstDucatiVidDec * self)
{
  gint w = self->width;
  gint h = self->height;

  /* calculate output buffer parameters: */
  self->padded_width = w;
  self->padded_height = h;
  self->min_buffers = 1;
}

static gboolean
gst_ducati_jpegdec_allocate_params (GstDucatiVidDec * self, gint params_sz,
    gint dynparams_sz, gint status_sz, gint inargs_sz, gint outargs_sz)
{
  IJPEGVDEC_DynamicParams *dynParams;

  gboolean ret = GST_DUCATIVIDDEC_CLASS (parent_class)->allocate_params (self,
      sizeof (IJPEGVDEC_Params), sizeof (IJPEGVDEC_DynamicParams),
      sizeof (IJPEGVDEC_Status), sizeof (IJPEGVDEC_InArgs),
      sizeof (IJPEGVDEC_OutArgs));

  if (!ret)
    return ret;

  /* We're doing ENTIREFRAME decoding so in theory 0 should be a valid value
   * for this. The codec seems to check that it's non-zero though...
   */
  self->params->numOutputDataUnits = 1;

  dynParams = (IJPEGVDEC_DynamicParams *) self->dynParams;
  dynParams->decodeThumbnail = 0;
  dynParams->thumbnailMode = 3;
  dynParams->downsamplingFactor = 1;
  dynParams->streamingCompliant = 0;

  return ret;
}

/* GObject vmethod implementations */

static void
gst_ducati_jpegdec_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_set_static_metadata (element_class,
      "DucatiJpegDec",
      "Codec/Decoder/Video",
      "Decodes video in MJPEG format with ducati",
      "Alessandro Decina <alessandro.decina@collabora.co.uk>");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_factory));
}

static void
gst_ducati_jpegdec_class_init (GstDucatiJpegDecClass * klass)
{
  GstDucatiVidDecClass *bclass = GST_DUCATIVIDDEC_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  bclass->codec_name = "ivahd_jpegvdec";
  bclass->update_buffer_size =
      GST_DEBUG_FUNCPTR (gst_ducati_jpegdec_update_buffer_size);
  bclass->allocate_params =
      GST_DEBUG_FUNCPTR (gst_ducati_jpegdec_allocate_params);
}

static void
gst_ducati_jpegdec_init (GstDucatiJpegDec * self,
    gpointer gclass)
{
  GstDucatiVidDec *vdec = GST_DUCATIVIDDEC (self);
  vdec->pageMemType = XDM_MEMTYPE_RAW;
#ifndef GST_DISABLE_GST_DEBUG
  vdec->error_strings[0] = "unsupported VIDDEC3 parameters";
  vdec->error_strings[1] = "unsupported VIDDEC3 dynamic parameters";
  vdec->error_strings[2] = "unsupported JPEGDEC dynamic parameters";
  vdec->error_strings[3] = "no slice";
  vdec->error_strings[4] = "MB data error";
  vdec->error_strings[5] = "standby";
  vdec->error_strings[6] = "invalid mbox message";
  vdec->error_strings[7] = "HDVICP reset";
  vdec->error_strings[16] = "HDVICP wait not clean exit";
  vdec->error_strings[17] = "Frame header error";
  vdec->error_strings[18] = "Scan header error";
  vdec->error_strings[19] = "Huffman table header error";
  vdec->error_strings[20] = "Quantization table header error";
  vdec->error_strings[21] = "Bad chroma format";
  vdec->error_strings[22] = "Unsupported marker";
  vdec->error_strings[23] = "Thumbnail error";
  vdec->error_strings[24] = "IRES handle error";
  vdec->error_strings[25] = "Dynamic params handle error";
  vdec->error_strings[26] = "Data sync error";
  vdec->error_strings[27] = "Downsample input format error";
  vdec->error_strings[28] = "Unsupported feature";
  vdec->error_strings[29] = "Unsupported resolution";
#endif
}
