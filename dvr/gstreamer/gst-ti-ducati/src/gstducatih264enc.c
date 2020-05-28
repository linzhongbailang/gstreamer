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
#include "gstducatih264enc.h"

#include <string.h>

#include <math.h>

#define GST_CAT_DEFAULT gst_ducati_debug

#define DEFAULT_PROFILE IH264_HIGH_PROFILE
#define DEFAULT_LEVEL IH264_LEVEL_51
#define DEFAULT_QPI 26
#define DEFAULT_QP_MAX_I 36
#define DEFAULT_QP_MIN_I 24
/* 2 x targetBitRate for VBR Rate Control */
#define DEFAULT_HRD_BUFFER_SIZE 40960000
#define DEFAULT_INTER_INTERVAL 1

#define GST_TYPE_DUCATI_H264ENC_PROFILE (gst_ducati_h264enc_profile_get_type ())
#define GST_TYPE_DUCATI_H264ENC_LEVEL (gst_ducati_h264enc_level_get_type ())
#define GST_TYPE_DUCATI_H264ENC_RCPP (gst_ducati_h264enc_get_rate_control_params_preset_type ())
#define GST_TYPE_DUCATI_H264ENC_RATE_CONTROL_ALGO (gst_ducati_h264enc_get_rate_control_algo_type ())
#define GST_TYPE_DUCATI_H264ENC_ENTROPY_CODING_MODE (gst_ducati_h264enc_get_entropy_coding_mode_type ())
#define GST_TYPE_DUCATI_H264ENC_SLICE_MODE (gst_ducati_h264enc_get_slice_mode_type ())


enum
{
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_PROFILE,
  PROP_LEVEL,
  PROP_RATE_CONTROL_PARAMS_PRESET,
  PROP_RATE_CONTROL_ALGO,
  PROP_QPI,
  PROP_QP_MAX_I,
  PROP_QP_MIN_I,
  PROP_HRD_BUFFER_SIZE,
  PROP_ENTROPY_CODING_MODE,
  PROP_INTER_INTERVAL,
  PROP_SLICE_MODE,
};

static void gst_ducati_h264enc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_ducati_h264enc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_ducati_h264enc_allocate_params (GstDucatiVidEnc *
    self, gint params_sz, gint dynparams_sz, gint status_sz, gint inargs_sz,
    gint outargs_sz);
static gboolean gst_ducati_h264enc_configure (GstDucatiVidEnc * self);
static gboolean gst_ducati_h264enc_is_sync_point (GstDucatiVidEnc * enc,
    int type);


static GstStaticPadTemplate gst_ducati_h264enc_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("NV12"))
    );

static GstStaticPadTemplate gst_ducati_h264enc_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS
    ("video/x-h264, alignment=(string)au, stream-format=(string)byte-stream")
    );

#define gst_ducati_h264enc_parent_class parent_class
G_DEFINE_TYPE (GstDucatiH264Enc, gst_ducati_h264enc, GST_TYPE_DUCATIVIDENC);


static GType
gst_ducati_h264enc_profile_get_type (void)
{
  static GType type = 0;

  if (!type) {
    static const GEnumValue vals[] = {
      {IH264_BASELINE_PROFILE, "Base Profile", "baseline"},
      {IH264_MAIN_PROFILE, "Main Profile", "main"},
      {IH264_EXTENDED_PROFILE, "Extended Profile", "extended"},
      {IH264_HIGH_PROFILE, "High Profile", "high"},
      {IH264_HIGH10_PROFILE, "High 10 Profile", "high-10"},
      {IH264_HIGH422_PROFILE, "High 4:2:2 Profile", "high-422"},
      {0, NULL, NULL},
    };

    type = g_enum_register_static ("GstDucatiH264EncProfile", vals);
  }

  return type;
}

static GType
gst_ducati_h264enc_get_rate_control_params_preset_type (void)
{
  static GType type = 0;

  if (!type) {
    static const GEnumValue vals[] = {
      {IH264_RATECONTROLPARAMS_DEFAULT, "Rate Control params preset default",
          "rate-control-params-preset-default"},
      {IH264_RATECONTROLPARAMS_USERDEFINED, "User defined rate control",
          "rate-control-params-preset-user-defined"},
      {IH264_RATECONTROLPARAMS_EXISTING, "Existing rate control params",
          "rate-control-params-preset-existing"},
      {0, NULL, NULL},
    };

    type = g_enum_register_static ("GstDucatiH264EncRateControlParams", vals);
  }

  return type;
}

static GType
gst_ducati_h264enc_get_rate_control_algo_type (void)
{
  static GType type = 0;

  if (!type) {
    static const GEnumValue vals[] = {
      {IH264_RATECONTROL_PRC, "Perceptual rate control",
          "perceptual-rate-control"},
      {IH264_RATECONTROL_PRC_LOW_DELAY, "Low delay rate control",
          "low-delay-rate-control"},
      {0, NULL, NULL},
    };

    type = g_enum_register_static ("GstDucatiH264EncRateControlAlgo", vals);
  }

  return type;
}

static GType
gst_ducati_h264enc_level_get_type (void)
{
  static GType type = 0;

  if (!type) {
    static const GEnumValue vals[] = {
      {IH264_LEVEL_10, "Level 1", "level-1"},
      {IH264_LEVEL_1b, "Level 1b", "level-1b"},
      {IH264_LEVEL_11, "Level 11", "level-11"},
      {IH264_LEVEL_12, "Level 12", "level-12"},
      {IH264_LEVEL_13, "Level 13", "level-13"},
      {IH264_LEVEL_20, "Level 2", "level-2"},
      {IH264_LEVEL_21, "Level 21", "level-21"},
      {IH264_LEVEL_22, "Level 22", "level-22"},
      {IH264_LEVEL_30, "Level 3", "level-3"},
      {IH264_LEVEL_31, "Level 31", "level-31"},
      {IH264_LEVEL_32, "Level 32", "level-32"},
      {IH264_LEVEL_40, "Level 4", "level-4"},
      {IH264_LEVEL_41, "Level 41", "level-41"},
      {IH264_LEVEL_42, "Level 42", "level-42"},
      {IH264_LEVEL_50, "Level 5", "level-5"},
      {IH264_LEVEL_51, "Level 51", "level-51"},
      {0, NULL, NULL},
    };

    type = g_enum_register_static ("GstDucatiH264EncLevel", vals);
  }

  return type;
}

static GType
gst_ducati_h264enc_get_entropy_coding_mode_type (void)
{
  static GType type = 0;

  if (!type) {
    static const GEnumValue vals[] = {
      {IH264_ENTROPYCODING_CAVLC, "CAVLC coding type", "cavlc"},
      {IH264_ENTROPYCODING_CABAC, "Cabac coding mode", "cabac"},
      {0, NULL, NULL},
    };

    type = g_enum_register_static ("GstDucatiEntropyCodingMode", vals);
  }

  return type;
}

static GType
gst_ducati_h264enc_get_slice_mode_type (void)
{
  static GType type = 0;

  if (!type) {
    static const GEnumValue vals[] = {
      {IH264_SLICEMODE_NONE, "No slice mode", "none"},
      {IH264_SLICEMODE_MBUNIT,
          "Slices are controlled based upon number of Macroblocks", "mbunit"},
      {IH264_SLICEMODE_BYTES,
          "Slices are controlled based upon number of bytes", "bytes"},
      {IH264_SLICEMODE_OFFSET,
          "Slices are controlled based upon user defined offset unit of Row",
          "offset"},
      {0, NULL, NULL},
    };

    type = g_enum_register_static ("GstDucatiSliceMode", vals);
  }

  return type;
}

static void
gst_ducati_h264enc_class_init (GstDucatiH264EncClass * klass)
{
  GObjectClass *gobject_class;
  GstDucatiVidEncClass *videnc_class;
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  videnc_class = GST_DUCATIVIDENC_CLASS (klass);
  parent_class = g_type_class_peek_parent (klass);


  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_ducati_h264enc_src_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_ducati_h264enc_sink_template));

  gst_element_class_set_metadata (element_class, "H264 Encoder",
      "Codec/Encoder/Video",
      "Encode raw video into H264 stream",
      "Alessandro Decina <alessandro.decina@collabora.com>");

  GST_DUCATIVIDENC_CLASS (element_class)->codec_name = "ivahd_h264enc";

  gobject_class->set_property = gst_ducati_h264enc_set_property;
  gobject_class->get_property = gst_ducati_h264enc_get_property;

  videnc_class->allocate_params = gst_ducati_h264enc_allocate_params;
  videnc_class->configure = gst_ducati_h264enc_configure;
  videnc_class->is_sync_point = gst_ducati_h264enc_is_sync_point;

  g_object_class_install_property (gobject_class, PROP_PROFILE,
      g_param_spec_enum ("profile", "H.264 Profile", "H.264 Profile",
          GST_TYPE_DUCATI_H264ENC_PROFILE, DEFAULT_PROFILE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_LEVEL,
      g_param_spec_enum ("level", "H.264 Level", "H.264 Level",
          GST_TYPE_DUCATI_H264ENC_LEVEL, DEFAULT_LEVEL, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_INTER_INTERVAL,
      g_param_spec_uint ("inter-interval", "Inter-frame interval",
          "Max inter frame interval (B frames are allowed between them if > 1)",
          1, 31, DEFAULT_INTER_INTERVAL, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
      PROP_RATE_CONTROL_PARAMS_PRESET,
      g_param_spec_enum ("rate-control-params-preset",
          "H.264 rate control params preset",
          "This preset controls the USER_DEFINED versus "
          "DEFAULT mode. If you are not aware about the "
          "fields, it should be set as 'rate-control-params-preset-default'",
          GST_TYPE_DUCATI_H264ENC_RCPP, IH264_RATECONTROLPARAMS_DEFAULT,
          G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_RATE_CONTROL_ALGO,
      g_param_spec_enum ("rate-control-algo", "H.264 rate control algorithm",
          "This defines the rate control algorithm to be used. Only useful if "
          " 'rate-control-params-preset' is set as "
          "'rate-control-params-preset-user-defined'",
          GST_TYPE_DUCATI_H264ENC_RATE_CONTROL_ALGO, IH264_RATECONTROL_DEFAULT,
          G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_QPI,
      g_param_spec_int ("qpi", "Initial quantization parameter",
          "Initial quantization parameter for I/IDR frames.", -1, 51,
          DEFAULT_QPI, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_QP_MIN_I,
      g_param_spec_int ("qp-min-i", "Minimum quantization parameter",
          "Minimum quantization parameter for I/IDR frames.", 0, 51,
          DEFAULT_QP_MIN_I, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_QP_MAX_I,
      g_param_spec_int ("qp-max-i", "Maximum quantization parameter",
          "Maximum quantization parameter for I/IDR frames.", 0, 51,
          DEFAULT_QP_MAX_I, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_HRD_BUFFER_SIZE,
      g_param_spec_uint ("hrd-buffer-size",
          "Hypothetical reference decoder buffer size",
          "Hypothetical reference decoder buffer size. This "
          "size controls the frame skip logic of the encoder. "
          "For low delay applications this size should be "
          "small. This size is in bits. Maximum Value is level "
          "dependant and min value is 4096",
          4096, G_MAXUINT, DEFAULT_HRD_BUFFER_SIZE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_ENTROPY_CODING_MODE,
      g_param_spec_enum ("entropy-coding-mode", "H.264 entropy coding mode",
          "Controls the entropy coding type.",
          GST_TYPE_DUCATI_H264ENC_ENTROPY_CODING_MODE,
          IH264_ENTROPYCODING_DEFAULT, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SLICE_MODE,
      g_param_spec_enum ("slice-mode", "H.264 slice mode",
          "This defines the control mechanism to split a picture in slices."
          " It can be either MB based or bytes based",
          GST_TYPE_DUCATI_H264ENC_SLICE_MODE,
          IH264_SLICEMODE_DEFAULT, G_PARAM_READWRITE));

}

static void
gst_ducati_h264enc_init (GstDucatiH264Enc * self)
{
  GST_DEBUG ("gst_ducati_h264enc_init");

  self->profile = DEFAULT_PROFILE;
  self->level = DEFAULT_LEVEL;
  self->rate_control_params_preset = IH264_RATECONTROLPARAMS_DEFAULT;
  self->rate_control_algo = IH264_RATECONTROL_DEFAULT;
  self->qpi = DEFAULT_QPI;
  self->qp_min_i = DEFAULT_QP_MIN_I;
  self->qp_max_i = DEFAULT_QP_MAX_I;
  self->hrd_buffer_size = DEFAULT_HRD_BUFFER_SIZE;
  self->inter_interval = DEFAULT_INTER_INTERVAL;
}

static void
gst_ducati_h264enc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstDucatiH264Enc *self = GST_DUCATIH264ENC (object);
  GstDucatiVidEnc *venc = GST_DUCATIVIDENC (object);
  IH264ENC_DynamicParams *dynParams =
      (IH264ENC_DynamicParams *) venc->dynParams;

  self = GST_DUCATIH264ENC (object);

  switch (prop_id) {
    case PROP_PROFILE:
      self->profile = g_value_get_enum (value);
      break;
    case PROP_LEVEL:
      self->level = g_value_get_enum (value);
      break;
    case PROP_RATE_CONTROL_PARAMS_PRESET:
      self->rate_control_params_preset = g_value_get_enum (value);
      if (dynParams)
        dynParams->rateControlParams.rateControlParamsPreset =
            self->rate_control_params_preset;
      break;
    case PROP_RATE_CONTROL_ALGO:
      self->rate_control_algo = g_value_get_enum (value);

      if (self->rate_control_params_preset !=
          IH264_RATECONTROLPARAMS_USERDEFINED)
        GST_INFO_OBJECT (self,
            "Setting rcAlgo but rateControlParamsPreset not "
            "'rate-control-params-preset-user-defined' config won't be taken "
            "into account");

      if (dynParams)
        dynParams->rateControlParams.rcAlgo = self->rate_control_algo;
      break;
    case PROP_QPI:
      self->qpi = g_value_get_int (value);
      if (dynParams)
        dynParams->rateControlParams.qpI = self->qpi;

      break;
    case PROP_QP_MIN_I:
      self->qp_min_i = g_value_get_int (value);
      if (dynParams)
        dynParams->rateControlParams.qpMinI = self->qp_min_i;
      break;
    case PROP_QP_MAX_I:
      self->qp_max_i = g_value_get_int (value);
      if (dynParams)
        dynParams->rateControlParams.qpMaxI = self->qp_max_i;
      break;
    case PROP_HRD_BUFFER_SIZE:
      self->hrd_buffer_size = g_value_get_uint (value);
      break;
    case PROP_ENTROPY_CODING_MODE:
      self->entropy_coding_mode = g_value_get_enum (value);
      break;
    case PROP_SLICE_MODE:
      self->slice_mode = g_value_get_enum (value);
      break;
    case PROP_INTER_INTERVAL:
      self->inter_interval = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
gst_ducati_h264enc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstDucatiH264Enc *self = GST_DUCATIH264ENC (object);

  g_return_if_fail (GST_IS_DUCATIH264ENC (object));
  self = GST_DUCATIH264ENC (object);

  switch (prop_id) {
    case PROP_PROFILE:
      g_value_set_enum (value, self->profile);
      break;
    case PROP_LEVEL:
      g_value_set_enum (value, self->level);
      break;
    case PROP_RATE_CONTROL_PARAMS_PRESET:
      g_value_set_enum (value, self->rate_control_params_preset);
      break;
    case PROP_RATE_CONTROL_ALGO:
      g_value_set_enum (value, self->rate_control_algo);
      break;
    case PROP_QPI:
      g_value_set_int (value, self->qpi);
      break;
    case PROP_QP_MIN_I:
      g_value_set_int (value, self->qp_min_i);
      break;
    case PROP_QP_MAX_I:
      g_value_set_int (value, self->qp_max_i);
      break;
    case PROP_HRD_BUFFER_SIZE:
      g_value_set_uint (value, self->hrd_buffer_size);
      break;
    case PROP_ENTROPY_CODING_MODE:
      g_value_set_enum (value, self->entropy_coding_mode);
      break;
    case PROP_SLICE_MODE:
      g_value_set_enum (value, self->slice_mode);
      break;
    case PROP_INTER_INTERVAL:
      g_value_set_uint (value, self->inter_interval);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static const char *
get_profile_name (guint profile)
{
  switch (profile) {
    case IH264_BASELINE_PROFILE:
      return "baseline";
    case IH264_MAIN_PROFILE:
      return "main";
    case IH264_EXTENDED_PROFILE:
      return "extended";
    case IH264_HIGH_PROFILE:
      return "high";
    case IH264_HIGH10_PROFILE:
      return "high-10";
    case IH264_HIGH422_PROFILE:
      return "high-422";
    default:
      return NULL;
  }
}

static const char *
get_level_name (guint level)
{
  switch (level) {
    case IH264_LEVEL_10:
      return "1";
    case IH264_LEVEL_1b:
      return "1b";
    case IH264_LEVEL_11:
      return "1.1";
    case IH264_LEVEL_12:
      return "1.2";
    case IH264_LEVEL_13:
      return "1.3";
    case IH264_LEVEL_20:
      return "2";
    case IH264_LEVEL_21:
      return "2.1";
    case IH264_LEVEL_22:
      return "2.2";
    case IH264_LEVEL_30:
      return "3";
    case IH264_LEVEL_31:
      return "3.1";
    case IH264_LEVEL_32:
      return "3.2";
    case IH264_LEVEL_40:
      return "4";
    case IH264_LEVEL_41:
      return "4.1";
    case IH264_LEVEL_42:
      return "4.2";
    case IH264_LEVEL_50:
      return "5";
    case IH264_LEVEL_51:
      return "5.1";
    default:
      return NULL;
  }
}


static int print_dynamic_params(IVIDENC2_DynamicParams *videnc2DynamicParams)
{
    g_print("videnc2DynamicParams -> inputHeight             : %d\n", videnc2DynamicParams->inputHeight);
    g_print("videnc2DynamicParams -> inputWidth              : %d\n", videnc2DynamicParams->inputWidth);
    g_print("videnc2DynamicParams -> refFrameRate            : %d\n", videnc2DynamicParams->refFrameRate);
    g_print("videnc2DynamicParams -> targetFrameRate         : %d\n", videnc2DynamicParams->targetFrameRate);
    g_print("videnc2DynamicParams -> targetBitRate           : %d\n", videnc2DynamicParams->targetBitRate);
    g_print("videnc2DynamicParams -> intraFrameInterval      : %d\n", videnc2DynamicParams->intraFrameInterval);
    g_print("videnc2DynamicParams -> generateHeader          : %d\n", videnc2DynamicParams->generateHeader);
    g_print("videnc2DynamicParams -> captureWidth            : %d\n", videnc2DynamicParams->captureWidth);
    g_print("videnc2DynamicParams -> forceFrame              : %d\n", videnc2DynamicParams->forceFrame);
    g_print("videnc2DynamicParams -> interFrameInterval      : %d\n", videnc2DynamicParams->interFrameInterval);
    g_print("videnc2DynamicParams -> mvAccuracy              : %d\n", videnc2DynamicParams->mvAccuracy);
    g_print("videnc2DynamicParams -> sampleAspectRatioHeight : %d\n", videnc2DynamicParams->sampleAspectRatioHeight);
    g_print("videnc2DynamicParams -> sampleAspectRatioWidth  : %d\n", videnc2DynamicParams->sampleAspectRatioWidth);
    g_print("videnc2DynamicParams -> ignoreOutbufSizeFlag    : %d\n", videnc2DynamicParams->ignoreOutbufSizeFlag);
    g_print("videnc2DynamicParams -> lateAcquireArg          : %d\n", videnc2DynamicParams->lateAcquireArg);

    return 0;
}

static int h264_print_dynamic_params(IH264ENC_DynamicParams *dynamicParams)
{
    print_dynamic_params(&dynamicParams->videnc2DynamicParams);
    g_print(" \n");
    g_print("rateControlParams -> rateControlParamsPreset        : %d\n", dynamicParams->rateControlParams.rateControlParamsPreset);
    g_print("rateControlParams -> scalingMatrixPreset            : %d\n", dynamicParams->rateControlParams.scalingMatrixPreset);
    g_print("rateControlParams -> rcAlgo                         : %d\n", dynamicParams->rateControlParams.rcAlgo);
    g_print("rateControlParams -> qpI                            : %d\n", dynamicParams->rateControlParams.qpI);
    g_print("rateControlParams -> qpMaxI                         : %d\n", dynamicParams->rateControlParams.qpMaxI);
    g_print("rateControlParams -> qpMinI                         : %d\n", dynamicParams->rateControlParams.qpMinI);
    g_print("rateControlParams -> qpP                            : %d\n", dynamicParams->rateControlParams.qpP);
    g_print("rateControlParams -> qpMaxP                         : %d\n", dynamicParams->rateControlParams.qpMaxP);
    g_print("rateControlParams -> qpMinP                         : %d\n", dynamicParams->rateControlParams.qpMinP);
    g_print("rateControlParams -> qpOffsetB                      : %d\n", dynamicParams->rateControlParams.qpOffsetB);
    g_print("rateControlParams -> qpMaxB                         : %d\n", dynamicParams->rateControlParams.qpMaxB);
    g_print("rateControlParams -> qpMinB                         : %d\n", dynamicParams->rateControlParams.qpMinB);
    g_print("rateControlParams -> allowFrameSkip                 : %d\n", dynamicParams->rateControlParams.allowFrameSkip);
    g_print("rateControlParams -> removeExpensiveCoeff           : %d\n", dynamicParams->rateControlParams.removeExpensiveCoeff);
    g_print("rateControlParams -> chromaQPIndexOffset            : %d\n", dynamicParams->rateControlParams.chromaQPIndexOffset);
    g_print("rateControlParams -> IPQualityFactor                : %d\n", dynamicParams->rateControlParams.IPQualityFactor);
    g_print("rateControlParams -> initialBufferLevel             : %d\n", dynamicParams->rateControlParams.initialBufferLevel);
    g_print("rateControlParams -> HRDBufferSize                  : %d\n", dynamicParams->rateControlParams.HRDBufferSize);
    g_print("rateControlParams -> minPicSizeRatioI               : %d\n", dynamicParams->rateControlParams.minPicSizeRatioI);
    g_print("rateControlParams -> maxPicSizeRatioI               : %d\n", dynamicParams->rateControlParams.maxPicSizeRatioI);
    g_print("rateControlParams -> minPicSizeRatioP               : %d\n", dynamicParams->rateControlParams.minPicSizeRatioP);
    g_print("rateControlParams -> maxPicSizeRatioP               : %d\n", dynamicParams->rateControlParams.maxPicSizeRatioP);
    g_print("rateControlParams -> minPicSizeRatioB               : %d\n", dynamicParams->rateControlParams.minPicSizeRatioB);
    g_print("rateControlParams -> maxPicSizeRatioB               : %d\n", dynamicParams->rateControlParams.maxPicSizeRatioB);
    g_print("rateControlParams -> enablePRC                      : %d\n", dynamicParams->rateControlParams.enablePRC);
    g_print("rateControlParams -> enablePartialFrameSkip         : %d\n", dynamicParams->rateControlParams.enablePartialFrameSkip);
    g_print("rateControlParams -> discardSavedBits               : %d\n", dynamicParams->rateControlParams.discardSavedBits);
    g_print("rateControlParams -> VBRDuration                    : %d\n", dynamicParams->rateControlParams.VBRDuration);
    g_print("rateControlParams -> VBRsensitivity                 : %d\n", dynamicParams->rateControlParams.VBRsensitivity);
    g_print("rateControlParams -> skipDistributionWindowLength   : %d\n", dynamicParams->rateControlParams.skipDistributionWindowLength);
    g_print("rateControlParams -> numSkipInDistributionWindow    : %d\n", dynamicParams->rateControlParams.numSkipInDistributionWindow);
    g_print("rateControlParams -> enableHRDComplianceMode        : %d\n", dynamicParams->rateControlParams.enableHRDComplianceMode);
    g_print("rateControlParams -> frameSkipThMulQ5               : %d\n", dynamicParams->rateControlParams.frameSkipThMulQ5);
    g_print("rateControlParams -> vbvUseLevelThQ5                : %d\n", dynamicParams->rateControlParams.vbvUseLevelThQ5);
    g_print(" \n");
    g_print("interCodingParams -> interCodingPreset  : %d\n", dynamicParams->interCodingParams.interCodingPreset);
    g_print("interCodingParams -> searchRangeHorP    : %d\n", dynamicParams->interCodingParams.searchRangeHorP);
    g_print("interCodingParams -> searchRangeVerP    : %d\n", dynamicParams->interCodingParams.searchRangeVerP);
    g_print("interCodingParams -> searchRangeHorB    : %d\n", dynamicParams->interCodingParams.searchRangeHorB);
    g_print("interCodingParams -> searchRangeVerB    : %d\n", dynamicParams->interCodingParams.searchRangeVerB);
    g_print("interCodingParams -> interCodingBias    : %d\n", dynamicParams->interCodingParams.interCodingBias);
    g_print("interCodingParams -> skipMVCodingBias   : %d\n", dynamicParams->interCodingParams.skipMVCodingBias);
    g_print("interCodingParams -> minBlockSizeP      : %d\n", dynamicParams->interCodingParams.minBlockSizeP);
    g_print("interCodingParams -> minBlockSizeB      : %d\n", dynamicParams->interCodingParams.minBlockSizeB);
    g_print("interCodingParams -> meAlgoMode         : %d\n", dynamicParams->interCodingParams.meAlgoMode);
    g_print(" \n");
    g_print("intraCodingParams -> intraCodingPreset          : %d\n", dynamicParams->intraCodingParams.intraCodingPreset);
    g_print("intraCodingParams -> lumaIntra4x4Enable         : %d\n", dynamicParams->intraCodingParams.lumaIntra4x4Enable);
    g_print("intraCodingParams -> lumaIntra8x8Enable         : %d\n", dynamicParams->intraCodingParams.lumaIntra8x8Enable);
    g_print("intraCodingParams -> lumaIntra16x16Enable       : %d\n", dynamicParams->intraCodingParams.lumaIntra16x16Enable);
    g_print("intraCodingParams -> chromaIntra8x8Enable       : %d\n", dynamicParams->intraCodingParams.chromaIntra8x8Enable);
    g_print("intraCodingParams -> chromaComponentEnable      : %d\n", dynamicParams->intraCodingParams.chromaComponentEnable);
    g_print("intraCodingParams -> intraRefreshMethod         : %d\n", dynamicParams->intraCodingParams.intraRefreshMethod);
    g_print("intraCodingParams -> intraRefreshRate           : %d\n", dynamicParams->intraCodingParams.intraRefreshRate);
    g_print("intraCodingParams -> gdrOverlapRowsBtwFrames    : %d\n", dynamicParams->intraCodingParams.gdrOverlapRowsBtwFrames);
    g_print("intraCodingParams -> constrainedIntraPredEnable : %d\n", dynamicParams->intraCodingParams.constrainedIntraPredEnable);
    g_print("intraCodingParams -> intraCodingBias            : %d\n", dynamicParams->intraCodingParams.intraCodingBias);
    g_print(" \n");
    g_print("sliceCodingParams -> sliceCodingPreset  : %d\n", dynamicParams->sliceCodingParams.sliceCodingPreset);
    g_print("sliceCodingParams -> sliceMode          : %d\n", dynamicParams->sliceCodingParams.sliceMode);
    g_print("sliceCodingParams -> sliceUnitSize      : %d\n", dynamicParams->sliceCodingParams.sliceUnitSize);
    g_print("sliceCodingParams -> sliceStartOffset   : [%d %d %d]\n",
            dynamicParams->sliceCodingParams.sliceStartOffset[0],
            dynamicParams->sliceCodingParams.sliceStartOffset[1],
            dynamicParams->sliceCodingParams.sliceStartOffset[2]
        );
    g_print("sliceCodingParams -> streamFormat       : %d\n", dynamicParams->sliceCodingParams.streamFormat);
    g_print(" \n");
    g_print("sliceGroupChangeCycle           : %d\n", dynamicParams->sliceGroupChangeCycle);
    g_print("searchCenter                    : %d\n", dynamicParams->searchCenter);
    g_print("enableStaticMBCount             : %d\n", dynamicParams->enableStaticMBCount);
    g_print("enableROI                       : %d\n", dynamicParams->enableROI);
    g_print(" \n");
    g_print(" \n");

    return 0;
}


static gboolean
gst_ducati_h264enc_configure (GstDucatiVidEnc * videnc)
{
  GstDucatiH264Enc *self = GST_DUCATIH264ENC (videnc);
  IH264ENC_Params *params;
  IH264ENC_DynamicParams *dynParams;
  gboolean ret;
  const char *s;
  const GstVideoCodecState *state;
  GstCaps *caps;
  int inter_interval;

  ret = GST_DUCATIVIDENC_CLASS (parent_class)->configure (videnc);
  if (!ret)
    return FALSE;

  videnc->params->profile = self->profile;
  videnc->params->level = self->level;

  inter_interval = self->inter_interval;
  if (self->profile == IH264_BASELINE_PROFILE)
    inter_interval = 1;
  else if (videnc->rate_preset == IVIDEO_LOW_DELAY)
    inter_interval = 1;

  params = (IH264ENC_Params *) videnc->params;
  /* this is the only non-base field strictly required */
  params->maxIntraFrameInterval = 0x7fffffff;
  params->IDRFrameInterval = 1;
  params->numTemporalLayer = 1;
  params->entropyCodingMode = self->entropy_coding_mode;
  videnc->params->maxInterFrameInterval = inter_interval;

  /* Dynamic params */
  dynParams = (IH264ENC_DynamicParams *) videnc->dynParams;
  dynParams->rateControlParams.rateControlParamsPreset =
      self->rate_control_params_preset;
  dynParams->rateControlParams.rcAlgo = self->rate_control_algo;
  dynParams->rateControlParams.qpI = self->qpi;
  dynParams->rateControlParams.qpMaxI = self->qp_max_i;
  dynParams->rateControlParams.qpMinI = self->qp_min_i;
  dynParams->rateControlParams.HRDBufferSize = self->hrd_buffer_size;
  dynParams->sliceCodingParams.sliceMode = self->slice_mode;
  videnc->dynParams->interFrameInterval = inter_interval;

  state = videnc->input_state;

  caps = gst_caps_new_simple ("video/x-h264",
      "width", G_TYPE_INT, videnc->rect.w,
      "height", G_TYPE_INT, videnc->rect.h,
      "framerate", GST_TYPE_FRACTION, GST_VIDEO_INFO_FPS_N (&state->info), GST_VIDEO_INFO_FPS_D (&state->info),
      "pixel-aspect-ratio", GST_TYPE_FRACTION, GST_VIDEO_INFO_PAR_N (&state->info), GST_VIDEO_INFO_PAR_D (&state->info),
      "stream-format", G_TYPE_STRING, "byte-stream",
      "align", G_TYPE_STRING, "au",
      "num-reorder-frames", G_TYPE_INT, inter_interval - 1, NULL);
  s = get_profile_name (self->profile);
  if (s)
    gst_caps_set_simple (caps, "profile", G_TYPE_STRING, s, NULL);
  s = get_level_name (self->level);
  if (s)
    gst_caps_set_simple (caps, "level", G_TYPE_STRING, s, NULL);

  ret = gst_pad_set_caps (GST_VIDEO_ENCODER_SRC_PAD (self), caps);

  //h264_print_dynamic_params(dynParams);
 
  return ret;
}

static gboolean
gst_ducati_h264enc_allocate_params (GstDucatiVidEnc *
    videnc, gint params_sz, gint dynparams_sz, gint status_sz, gint inargs_sz,
    gint outargs_sz)
{
  gboolean ret = GST_DUCATIVIDENC_CLASS (parent_class)->allocate_params (videnc,
      sizeof (IH264ENC_Params), sizeof (IH264ENC_DynamicParams),
      sizeof (IVIDENC2_Status), sizeof (IVIDENC2_InArgs),
      sizeof (IVIDENC2_OutArgs));

  GstDucatiH264Enc *self = GST_DUCATIH264ENC (videnc);

  if (ret == TRUE) {
    IH264ENC_DynamicParams *dynParams =
        (IH264ENC_DynamicParams *) videnc->dynParams;

    dynParams->rateControlParams.rateControlParamsPreset =
        self->rate_control_params_preset;
    dynParams->rateControlParams.rcAlgo = self->rate_control_algo;
    dynParams->rateControlParams.qpI = self->qpi;
    dynParams->rateControlParams.qpMaxI = self->qp_max_i;
    dynParams->rateControlParams.qpMinI = self->qp_min_i;
    dynParams->rateControlParams.HRDBufferSize = self->hrd_buffer_size;
    dynParams->sliceCodingParams.sliceMode = self->slice_mode;
  }

  return ret;
}

static gboolean
gst_ducati_h264enc_is_sync_point (GstDucatiVidEnc * enc, int type)
{
  return type == IVIDEO_IDR_FRAME;
}
