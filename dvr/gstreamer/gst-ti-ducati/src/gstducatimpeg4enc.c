/* GStreamer
 * Copyright (c) 2011, Texas Instruments Incorporated
 * Copyright (c) 2011, Collabora Ltd.
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
 * Author: Alessandro Decina <alessandro.decina@collabora.com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstducati.h"
#include "gstducatimpeg4enc.h"

#include <string.h>

#include <math.h>

#define GST_CAT_DEFAULT gst_ducati_debug

#define DEFAULT_PROFILE MPEG4_SIMPLE_PROFILE_IDC
#define DEFAULT_LEVEL IMPEG4ENC_SP_LEVEL_5

#define GST_TYPE_DUCATI_MPEG4ENC_PROFILE (gst_ducati_mpeg4enc_profile_get_type ())
#define GST_TYPE_DUCATI_MPEG4ENC_LEVEL (gst_ducati_mpeg4enc_level_get_type ())


enum
{
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_PROFILE,
  PROP_LEVEL,
};

static void gst_ducati_mpeg4enc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_ducati_mpeg4enc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_ducati_mpeg4enc_allocate_params (GstDucatiVidEnc *
    self, gint params_sz, gint dynparams_sz, gint status_sz, gint inargs_sz,
    gint outargs_sz);
static gboolean gst_ducati_mpeg4enc_configure (GstDucatiVidEnc * self);


static GstStaticPadTemplate gst_ducati_mpeg4enc_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("NV12"))
    );

static GstStaticPadTemplate gst_ducati_mpeg4enc_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/mpeg, mpegversion=4, systemstream=false")
    );

#define parent_class gst_ducati_mpeg4enc_parent_class
G_DEFINE_TYPE (GstDucatiMPEG4Enc, gst_ducati_mpeg4enc, GST_TYPE_DUCATIVIDENC);


static GType
gst_ducati_mpeg4enc_profile_get_type (void)
{
  static GType type = 0;

  if (!type) {
    static const GEnumValue vals[] = {
      {IMPEG4ENC_SP_LEVEL_3, "Simple", "simple"},
      {0, NULL, NULL},
    };

    type = g_enum_register_static ("GstDucatiMPEG4EncProfile", vals);
  }

  return type;
}

static GType
gst_ducati_mpeg4enc_level_get_type (void)
{
  static GType type = 0;

  if (!type) {
    static const GEnumValue vals[] = {
      {IMPEG4ENC_SP_LEVEL_0, "Level 0", "level-0"},
      {IMPEG4ENC_SP_LEVEL_0B, "Level 0B", "level-0b"},
      {IMPEG4ENC_SP_LEVEL_1, "Level 1", "level-1"},
      {IMPEG4ENC_SP_LEVEL_2, "Level 2", "level-2"},
      {IMPEG4ENC_SP_LEVEL_3, "Level 3", "level-3"},
      {IMPEG4ENC_SP_LEVEL_4A, "Level 4", "level-4"},
      {IMPEG4ENC_SP_LEVEL_5, "Level 5", "level-5"},
      {IMPEG4ENC_SP_LEVEL_6, "Level 6", "level-6"},
      {0, NULL, NULL},
    };

    type = g_enum_register_static ("GstDucatiMPEG4EncLevel", vals);
  }

  return type;
}


static void
gst_ducati_mpeg4enc_class_init (GstDucatiMPEG4EncClass * klass)
{
  GObjectClass *gobject_class;
  GstDucatiVidEncClass *videnc_class;
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  videnc_class = GST_DUCATIVIDENC_CLASS (klass);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_ducati_mpeg4enc_src_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_ducati_mpeg4enc_sink_template));

  gst_element_class_set_metadata (element_class, "MPEG4 Encoder",
      "Codec/Encoder/Video",
      "Encode raw video into MPEG4 stream",
      "Alessandro Decina <alessandro.decina@collabora.com>");

  GST_DUCATIVIDENC_CLASS (element_class)->codec_name = "ivahd_mpeg4enc";

  gobject_class->set_property = gst_ducati_mpeg4enc_set_property;
  gobject_class->get_property = gst_ducati_mpeg4enc_get_property;

  videnc_class->allocate_params = gst_ducati_mpeg4enc_allocate_params;
  videnc_class->configure = gst_ducati_mpeg4enc_configure;

  g_object_class_install_property (gobject_class, PROP_PROFILE,
      g_param_spec_enum ("profile", "MPEG4 Profile", "MPEG4 Profile",
          GST_TYPE_DUCATI_MPEG4ENC_PROFILE, DEFAULT_PROFILE,
          G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_LEVEL,
      g_param_spec_enum ("level", "MPEG4 Level", "MPEG4 Level",
          GST_TYPE_DUCATI_MPEG4ENC_LEVEL, DEFAULT_LEVEL, G_PARAM_READWRITE));
}

static void
gst_ducati_mpeg4enc_init (GstDucatiMPEG4Enc * self)
{
  GST_DEBUG ("gst_ducati_mpeg4enc_init");

  self->profile = DEFAULT_PROFILE;
  self->level = DEFAULT_LEVEL;
}

static void
gst_ducati_mpeg4enc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstDucatiMPEG4Enc *self;

  g_return_if_fail (GST_IS_DUCATIMPEG4ENC (object));
  self = GST_DUCATIMPEG4ENC (object);

  switch (prop_id) {
    case PROP_PROFILE:
      self->profile = g_value_get_enum (value);
      break;
    case PROP_LEVEL:
      self->level = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
gst_ducati_mpeg4enc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstDucatiMPEG4Enc *self;

  g_return_if_fail (GST_IS_DUCATIMPEG4ENC (object));
  self = GST_DUCATIMPEG4ENC (object);

  switch (prop_id) {
    case PROP_PROFILE:
      g_value_set_enum (value, self->profile);
      break;
    case PROP_LEVEL:
      g_value_set_enum (value, self->level);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static const char *
get_profile_name (guint profile)
{
  switch (profile) {
    case IMPEG4ENC_SP_LEVEL_3:
      return "simple";
    default:
      return NULL;
  }
}

static gboolean
gst_ducati_mpeg4enc_configure (GstDucatiVidEnc * videnc)
{
  GstDucatiMPEG4Enc *self = GST_DUCATIMPEG4ENC (videnc);
  const GstVideoCodecState *state;
  GstCaps *caps;
  const char *s;
  gboolean ret = TRUE;

  if (!GST_DUCATIVIDENC_CLASS (parent_class)->configure (videnc))
    return FALSE;

  videnc->params->profile = self->profile;
  videnc->params->level = self->level;
  videnc->params->maxInterFrameInterval = 0;
  videnc->dynParams->mvAccuracy = IVIDENC2_MOTIONVECTOR_HALFPEL;
  videnc->dynParams->interFrameInterval = 0;

  state = videnc->input_state;
  caps = gst_caps_new_simple ("video/mpeg",
      "mpegversion", G_TYPE_INT, 4,
      "systemstream", G_TYPE_BOOLEAN, FALSE,
      "width", G_TYPE_INT, videnc->rect.w,
      "height", G_TYPE_INT, videnc->rect.h,
      "framerate", GST_TYPE_FRACTION, GST_VIDEO_INFO_FPS_N (&state->info), GST_VIDEO_INFO_FPS_D (&state->info),
      "pixel-aspect-ratio", GST_TYPE_FRACTION, GST_VIDEO_INFO_PAR_N (&state->info), GST_VIDEO_INFO_PAR_D (&state->info),
      NULL);
  s = get_profile_name (self->profile);
  if (s)
    gst_caps_set_simple (caps, "profile", G_TYPE_STRING, s, NULL);
  ret = gst_pad_set_caps (GST_VIDEO_ENCODER_SRC_PAD (self), caps);

  return ret;
}

static gboolean
gst_ducati_mpeg4enc_allocate_params (GstDucatiVidEnc *
    videnc, gint params_sz, gint dynparams_sz, gint status_sz, gint inargs_sz,
    gint outargs_sz)
{
  return GST_DUCATIVIDENC_CLASS (parent_class)->allocate_params (videnc,
      sizeof (IVIDENC2_Params), sizeof (IVIDENC2_DynamicParams),
      sizeof (IVIDENC2_Status), sizeof (IVIDENC2_InArgs),
      sizeof (IVIDENC2_OutArgs));
}
