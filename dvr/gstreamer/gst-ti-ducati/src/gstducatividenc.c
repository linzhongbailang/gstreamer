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
#include "gstducatividenc.h"
#include "gstducatibufferpriv.h"

#include <string.h>

#include <math.h>
#include <gst/canbuf/canbuf.h>

#define GST_CAT_DEFAULT gst_ducati_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_PERFORMANCE);

#define DEFAULT_BITRATE 12288//16384
#define DEFAULT_RATE_PRESET GST_DUCATI_VIDENC_RATE_PRESET_STORAGE
#define DEFAULT_INTRA_INTERVAL 8
#define DEFAULT_OUTPUT_BUFFER_SIZE 1024*1024 //1M

#define GST_TYPE_DUCATI_VIDENC_RATE_PRESET (gst_ducati_videnc_rate_preset_get_type ())

enum
{
  PROP_0,
  PROP_BITRATE,
  PROP_RATE_PRESET,
  PROP_INTRA_INTERVAL,
  PROP_EXTERNAL_DCE_DEV,
  PROP_EXTERNAL_INPOOL,
};

static void gst_ducati_videnc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_ducati_videnc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_ducati_videnc_set_format (GstVideoEncoder *
    base_video_encoder, GstVideoCodecState * state);
static gboolean gst_ducati_videnc_start (GstVideoEncoder * base_video_encoder);
static gboolean gst_ducati_videnc_stop (GstVideoEncoder * base_video_encoder);
static GstFlowReturn gst_ducati_videnc_finish (GstVideoEncoder *
    base_video_encoder);
static GstFlowReturn gst_ducati_videnc_handle_frame (GstVideoEncoder *
    base_video_encoder, GstVideoCodecFrame * frame);
static gboolean gst_ducati_videnc_allocate_params_default (GstDucatiVidEnc *
    self, gint params_sz, gint dynparams_sz, gint status_sz, gint inargs_sz,
    gint outargs_sz);
static gboolean gst_ducati_videnc_is_sync_point_default (GstDucatiVidEnc * enc,
    int type);
static gboolean gst_ducati_videnc_configure_default (GstDucatiVidEnc * self);
static gboolean gst_ducati_videnc_event (GstVideoEncoder * enc,
    GstEvent * event);


#define gst_ducati_videnc_parent_class parent_class
G_DEFINE_TYPE (GstDucatiVidEnc, gst_ducati_videnc, GST_TYPE_VIDEO_ENCODER);


/* the values for the following enums are taken from the codec */

enum
{
  GST_DUCATI_VIDENC_RATE_PRESET_LOW_DELAY = IVIDEO_LOW_DELAY,   /**< CBR rate control for video conferencing. */
  GST_DUCATI_VIDENC_RATE_PRESET_STORAGE = IVIDEO_STORAGE,  /**< VBR rate control for local storage (DVD)
                           *   recording.
                           */
  GST_DUCATI_VIDENC_RATE_PRESET_TWOPASS = IVIDEO_TWOPASS,  /**< Two pass rate control for non real time
                           *   applications.
                           */
  GST_DUCATI_VIDENC_RATE_PRESET_NONE = IVIDEO_NONE,        /**< No configurable video rate control
                            *  mechanism.
                            */
  GST_DUCATI_VIDENC_RATE_PRESET_USER_DEFINED = IVIDEO_USER_DEFINED,/**< User defined configuration using extended
                           *   parameters.
                           */
};

static GType
gst_ducati_videnc_rate_preset_get_type (void)
{
  static GType type = 0;

  if (!type) {
    static const GEnumValue vals[] = {
      {GST_DUCATI_VIDENC_RATE_PRESET_LOW_DELAY, "Low Delay", "low-delay"},
      {GST_DUCATI_VIDENC_RATE_PRESET_STORAGE, "Storage", "storage"},
      {GST_DUCATI_VIDENC_RATE_PRESET_TWOPASS, "Two-Pass", "two-pass"},
      {GST_DUCATI_VIDENC_RATE_PRESET_NONE, "None", "none"},
      {GST_DUCATI_VIDENC_RATE_PRESET_USER_DEFINED, "User defined",
          "user-defined"},
      {0, NULL, NULL},
    };

    type = g_enum_register_static ("GstDucatiVidEncRatePreset", vals);
  }

  return type;
}


static void
gst_ducati_videnc_class_init (GstDucatiVidEncClass * klass)
{
  GObjectClass *gobject_class;
  GstVideoEncoderClass *basevideoencoder_class;

  gobject_class = G_OBJECT_CLASS (klass);
  basevideoencoder_class = GST_VIDEO_ENCODER_CLASS (klass);
  parent_class = g_type_class_peek_parent (klass);

  gobject_class->set_property = gst_ducati_videnc_set_property;
  gobject_class->get_property = gst_ducati_videnc_get_property;

  basevideoencoder_class->set_format =
      GST_DEBUG_FUNCPTR (gst_ducati_videnc_set_format);
  basevideoencoder_class->start = GST_DEBUG_FUNCPTR (gst_ducati_videnc_start);
  basevideoencoder_class->stop = GST_DEBUG_FUNCPTR (gst_ducati_videnc_stop);
  basevideoencoder_class->finish = GST_DEBUG_FUNCPTR (gst_ducati_videnc_finish);
  basevideoencoder_class->handle_frame =
      GST_DEBUG_FUNCPTR (gst_ducati_videnc_handle_frame);

  basevideoencoder_class->src_event =
      GST_DEBUG_FUNCPTR (gst_ducati_videnc_event);

  klass->allocate_params = gst_ducati_videnc_allocate_params_default;
  klass->configure = gst_ducati_videnc_configure_default;
  klass->is_sync_point = gst_ducati_videnc_is_sync_point_default;

  g_object_class_install_property (gobject_class, PROP_BITRATE,
      g_param_spec_int ("bitrate", "Bitrate", "Bitrate in kbit/sec", -1,
          100 * 1024, DEFAULT_BITRATE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_RATE_PRESET,
      g_param_spec_enum ("rate-preset", "H.264 Rate Control",
          "H.264 Rate Control",
          GST_TYPE_DUCATI_VIDENC_RATE_PRESET, DEFAULT_RATE_PRESET,
          G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_INTRA_INTERVAL,
      g_param_spec_int ("intra-interval", "Intra-frame interval",
          "Interval between intra frames (keyframes)", 0, INT_MAX,
          DEFAULT_INTRA_INTERVAL, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_EXTERNAL_DCE_DEV,
		g_param_spec_pointer ("external-dcedev", "External libdce device",
			"External Libdce device", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  
  g_object_class_install_property (gobject_class, PROP_EXTERNAL_INPOOL,
		g_param_spec_pointer ("external-inpool", "External inpool",
			"External Input Pool for Ducati Encoder", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  GST_DEBUG_CATEGORY_GET (GST_CAT_PERFORMANCE, "GST_PERFORMANCE");
}

static void
gst_ducati_videnc_init (GstDucatiVidEnc * self)
{
  GST_DEBUG ("gst_ducati_videnc_init");

  gst_ducati_set_generic_error_strings (self->error_strings);

  self->device = NULL;
  self->external_dce_device = NULL;
  self->engine = NULL;
  self->codec = NULL;
  self->params = NULL;
  self->status = NULL;
  self->inBufs = NULL;
  self->outBufs = NULL;
  self->inArgs = NULL;
  self->outArgs = NULL;
  self->input_pool = NULL;
  self->output_pool = NULL;
  self->external_inpool = NULL;

  self->bitrate = DEFAULT_BITRATE * 1000;
  self->rate_preset = DEFAULT_RATE_PRESET;
  self->intra_interval = DEFAULT_INTRA_INTERVAL;
}

static gboolean
gst_ducati_videnc_set_format (GstVideoEncoder * base_video_encoder,
    GstVideoCodecState * state)
{
  GstDucatiVidEnc *self = GST_DUCATIVIDENC (base_video_encoder);
  GstVideoCodecState *output_state;
  GstCaps *allowed_caps = NULL;


  GST_DEBUG_OBJECT (self, "picking an output format ...");
  allowed_caps =
      gst_pad_get_allowed_caps (GST_VIDEO_ENCODER_SRC_PAD (base_video_encoder));
  if (!allowed_caps) {
    GST_DEBUG_OBJECT (self, "... but no peer, using template caps");
    allowed_caps =
        gst_pad_get_pad_template_caps (GST_VIDEO_ENCODER_SRC_PAD
        (base_video_encoder));
  }
  GST_DEBUG_OBJECT (self, "chose caps %" GST_PTR_FORMAT, allowed_caps);
  allowed_caps = gst_caps_truncate (allowed_caps);
  GST_DEBUG_OBJECT (self, "allowed caps %" GST_PTR_FORMAT, allowed_caps);
  output_state = gst_video_encoder_set_output_state (GST_VIDEO_ENCODER (self),
      allowed_caps, state);
  gst_video_codec_state_unref (output_state);


  if (!gst_video_encoder_negotiate (GST_VIDEO_ENCODER (self))) {
    GST_DEBUG_OBJECT (self, "negotiate failed");
    return FALSE;
  }

  if (self->input_state)
    gst_video_codec_state_unref (self->input_state);
  self->input_state = gst_video_codec_state_ref (state);
  self->configure = TRUE;

  return TRUE;
}

static void
gst_ducati_videnc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstDucatiVidEnc *self;

  g_return_if_fail (GST_IS_DUCATIVIDENC (object));
  self = GST_DUCATIVIDENC (object);

  switch (prop_id) {
    case PROP_BITRATE:
      self->bitrate = g_value_get_int (value) * 1000;
      break;
    case PROP_RATE_PRESET:
      self->rate_preset = g_value_get_enum (value);
      break;
    case PROP_INTRA_INTERVAL:
      self->intra_interval = g_value_get_int (value);
      break;
    case PROP_EXTERNAL_DCE_DEV:
      self->external_dce_device = g_value_get_pointer (value);
      break;
	case PROP_EXTERNAL_INPOOL:
	  self->external_inpool = g_value_get_pointer (value);
	  break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
gst_ducati_videnc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstDucatiVidEnc *self;

  g_return_if_fail (GST_IS_DUCATIVIDENC (object));
  self = GST_DUCATIVIDENC (object);

  switch (prop_id) {
    case PROP_BITRATE:
      g_value_set_int (value, self->bitrate / 1000);
      break;
    case PROP_RATE_PRESET:
      g_value_set_enum (value, self->rate_preset);
      break;
    case PROP_INTRA_INTERVAL:
      g_value_set_int (value, self->intra_interval);
      break;
    case PROP_EXTERNAL_DCE_DEV:
	  g_value_set_pointer (value, self->external_dce_device);
      break;
	case PROP_EXTERNAL_INPOOL:
	  g_value_set_pointer (value, self->external_inpool);
	  break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static gboolean
gst_ducati_videnc_configure (GstDucatiVidEnc * self)
{
  int err;
  int i;
  int max_out_size = 0;
  const GstVideoCodecState *state;
  GstCaps *allowed_sink_caps = NULL;
  GstCaps *allowed_src_caps = NULL;
  state = self->input_state;

  if (!GST_DUCATIVIDENC_GET_CLASS (self)->configure (self))
    return FALSE;

  if (self->codec == NULL) {
    const gchar *codec_name;

    codec_name = GST_DUCATIVIDENC_GET_CLASS (self)->codec_name;
    self->codec = VIDENC2_create (self->engine,
        (String) codec_name, self->params);
    if (self->codec == NULL) {
      GST_ERROR_OBJECT (self, "couldn't create codec");
      return FALSE;
    }
  }

  err = VIDENC2_control (self->codec,
      XDM_SETPARAMS, self->dynParams, self->status);
  if (err) {
    GST_ERROR_OBJECT (self, "XDM_SETPARAMS err=%d, extendedError=%08x",
        err, self->status->extendedError);
    gst_ducati_log_extended_error_info (self->status->extendedError,
        self->error_strings);

    return FALSE;
  }

  err = VIDENC2_control (self->codec,
      XDM_GETBUFINFO, self->dynParams, self->status);
  if (err) {
    GST_ERROR_OBJECT (self, "XDM_GETBUFINFO err=%d, extendedError=%08x",
        err, self->status->extendedError);

    return FALSE;
  }

  self->outBufs->numBufs = self->status->bufInfo.minNumOutBufs;
  for (i = 0; i < self->outBufs->numBufs; i++) {
    int size = self->status->bufInfo.minOutBufSize[i].bytes;
    if (size > max_out_size)
      max_out_size = size;
  }

  g_assert (self->input_pool == NULL);
  if(self->external_inpool == NULL)
  {
	  allowed_sink_caps =
	      gst_pad_get_allowed_caps (GST_VIDEO_ENCODER_SINK_PAD (self));
	  if (!allowed_sink_caps) {
	    GST_DEBUG_OBJECT (self, "... but no peer, using template caps");
	    allowed_sink_caps =
	        gst_pad_get_pad_template_caps (GST_VIDEO_ENCODER_SINK_PAD (self));
	  }
	  GST_DEBUG_OBJECT (self, "chose caps %" GST_PTR_FORMAT, allowed_sink_caps);
	  allowed_sink_caps = gst_caps_truncate (allowed_sink_caps);
	  
		  self->input_pool = gst_drm_buffer_pool_new (GST_ELEMENT (self),
		      dce_get_fd (), gst_caps_fixate (allowed_sink_caps),
		      GST_VIDEO_INFO_SIZE (&state->info));
  }

  g_assert (self->output_pool == NULL);
  allowed_src_caps =
      gst_pad_get_allowed_caps (GST_VIDEO_ENCODER_SRC_PAD (self));
  if (!allowed_src_caps) {
    GST_DEBUG_OBJECT (self, "... but no peer, using template caps");
    allowed_src_caps =
        gst_pad_get_pad_template_caps (GST_VIDEO_ENCODER_SRC_PAD (self));
  }
  GST_DEBUG_OBJECT (self, "chose caps %" GST_PTR_FORMAT, allowed_src_caps);
  allowed_src_caps = gst_caps_truncate (allowed_src_caps);
  self->output_pool = gst_drm_buffer_pool_new (GST_ELEMENT (self),
      dce_get_fd (), gst_caps_fixate (allowed_src_caps), DEFAULT_OUTPUT_BUFFER_SIZE/*max_out_size*/);

  GST_INFO_OBJECT (self, "configured");

  self->configure = FALSE;

  return TRUE;
}

static gboolean
gst_ducati_videnc_configure_default (GstDucatiVidEnc * self)
{
  VIDENC2_DynamicParams *dynParams;
  VIDENC2_Params *params;
  const GstVideoCodecState *state;
  int i;

  state = self->input_state;

  if (self->rect.w == 0)
    self->rect.w = GST_VIDEO_INFO_WIDTH (&state->info);

  if (self->rect.h == 0)
    self->rect.h = GST_VIDEO_INFO_HEIGHT (&state->info);

  params = (VIDENC2_Params *) self->params;
  params->encodingPreset = 0x03;
  params->rateControlPreset = self->rate_preset;
  params->maxHeight = self->rect.h;
  params->maxWidth = self->rect.w;
  params->dataEndianness = XDM_BYTE;
  params->maxInterFrameInterval = 1;
  params->maxBitRate = -1;
  params->minBitRate = 0;
  params->inputChromaFormat = XDM_YUV_420SP;
  params->inputContentType = IVIDEO_PROGRESSIVE;
  params->operatingMode = IVIDEO_ENCODE_ONLY;
  params->inputDataMode = IVIDEO_ENTIREFRAME;
  params->outputDataMode = IVIDEO_ENTIREFRAME;
  params->numInputDataUnits = 1;
  params->numOutputDataUnits = 1;
  for (i = 0; i < IVIDEO_MAX_NUM_METADATA_PLANES; i++) {
    params->metadataType[i] = IVIDEO_METADATAPLANE_NONE;
  }

  dynParams = (VIDENC2_DynamicParams *) self->dynParams;

  dynParams->refFrameRate =
      gst_util_uint64_scale (1000, GST_VIDEO_INFO_FPS_N (&state->info),
      GST_VIDEO_INFO_FPS_D (&state->info));
  dynParams->targetFrameRate = dynParams->refFrameRate;
  dynParams->inputWidth = self->rect.w;
  dynParams->inputHeight = self->rect.h;
  dynParams->targetBitRate = self->bitrate;
  dynParams->intraFrameInterval = self->intra_interval;
  dynParams->captureWidth = dynParams->inputWidth;

  dynParams->forceFrame = IVIDEO_NA_FRAME;
  dynParams->interFrameInterval = 1;
  dynParams->mvAccuracy = IVIDENC2_MOTIONVECTOR_QUARTERPEL;
  dynParams->sampleAspectRatioHeight = 1;
  dynParams->sampleAspectRatioWidth = 1;
  dynParams->generateHeader = XDM_ENCODE_AU;
  dynParams->ignoreOutbufSizeFlag = 1;
  dynParams->lateAcquireArg = -1;

  self->inBufs->chromaFormat = XDM_YUV_420SP;
  self->inBufs->numPlanes = 2;

  return TRUE;
}

static gboolean
gst_ducati_videnc_open_engine (GstDucatiVidEnc * self)
{
  int error_code;

  if (self->device == NULL && self->external_dce_device == NULL) {
    self->device = dce_init ();
    if (self->device == NULL)
      return FALSE;
  }

  self->engine = Engine_open ((String) "ivahd_vidsvr", NULL, &error_code);
  if (self->engine == NULL) {
    GST_ERROR_OBJECT (self, "couldn't open engine");
    return FALSE;
  }

  return TRUE;
}

static gboolean
gst_ducati_videnc_allocate_params (GstDucatiVidEnc * self)
{
  return GST_DUCATIVIDENC_GET_CLASS (self)->allocate_params (self,
      sizeof (IVIDENC2_Params), sizeof (IVIDENC2_DynamicParams),
      sizeof (IVIDENC2_Status), sizeof (IVIDENC2_InArgs),
      sizeof (IVIDENC2_OutArgs));
}

static gboolean
gst_ducati_videnc_allocate_params_default (GstDucatiVidEnc * self,
    gint params_sz, gint dynparams_sz, gint status_sz, gint inargs_sz,
    gint outargs_sz)
{
  self->params = dce_alloc (params_sz);
  memset (self->params, 0, params_sz);
  self->params->size = params_sz;

  self->dynParams = dce_alloc (dynparams_sz);
  memset (self->dynParams, 0, dynparams_sz);
  self->dynParams->size = dynparams_sz;

  self->status = dce_alloc (status_sz);
  memset (self->status, 0, status_sz);
  self->status->size = status_sz;

  self->inBufs = dce_alloc (sizeof (IVIDEO2_BufDesc));
  memset (self->inBufs, 0, sizeof (IVIDEO2_BufDesc));

  self->outBufs = dce_alloc (sizeof (XDM2_BufDesc));
  memset (self->outBufs, 0, sizeof (XDM2_BufDesc));

  self->inArgs = dce_alloc (inargs_sz);
  memset (self->inArgs, 0, inargs_sz);
  self->inArgs->size = inargs_sz;

  self->outArgs = dce_alloc (outargs_sz);
  memset (self->outArgs, 0, outargs_sz);
  self->outArgs->size = outargs_sz;

  GST_INFO_OBJECT (self, "started");

  return TRUE;
}

static gboolean
gst_ducati_videnc_free_params (GstDucatiVidEnc * self)
{
  if (self->params) {
    dce_free (self->params);
    self->params = NULL;
  }

  if (self->dynParams) {
    dce_free (self->dynParams);
    self->dynParams = NULL;
  }

  if (self->inArgs) {
    dce_free (self->inArgs);
    self->inArgs = NULL;
  }

  if (self->outArgs) {
    dce_free (self->outArgs);
    self->outArgs = NULL;
  }

  if (self->status) {
    dce_free (self->status);
    self->status = NULL;
  }

  if (self->inBufs) {
    dce_free (self->inBufs);
    self->inBufs = NULL;
  }

  if (self->outBufs) {
    dce_free (self->outBufs);
    self->outBufs = NULL;
  }

  if (self->codec) {
    VIDENC2_delete (self->codec);
    self->codec = NULL;
  }

  return TRUE;
}

static void
gst_ducati_videnc_close_engine (GstDucatiVidEnc * self)
{
  if (self->engine) {
    Engine_close (self->engine);
    self->engine = NULL;
  }

  if (self->device && self->external_dce_device == NULL) {
    dce_deinit (self->device);
    self->device = NULL;
  }
}


static gboolean
gst_ducati_videnc_start (GstVideoEncoder * base_video_encoder)
{
  GstDucatiVidEnc *self = GST_DUCATIVIDENC (base_video_encoder);

  self->configure = TRUE;
  memset (&self->rect, 0, sizeof (GstDucatiVideoRectangle));

  if (!gst_ducati_videnc_open_engine (self))
    goto fail;

  if (!gst_ducati_videnc_allocate_params (self))
    goto fail;
  
  g_queue_init(&self->can_queue);

  return TRUE;

fail:
  gst_ducati_videnc_free_params (self);
  gst_ducati_videnc_close_engine (self);
  return FALSE;
}

static gboolean
gst_ducati_videnc_stop (GstVideoEncoder * base_video_encoder)
{
  GstDucatiVidEnc *self = GST_DUCATIVIDENC (base_video_encoder);

  gst_ducati_videnc_free_params (self);
  gst_ducati_videnc_close_engine (self);

  if (self->input_pool && self->external_inpool == NULL) {
    gst_drm_buffer_pool_destroy (self->input_pool);
    self->input_pool = NULL;
  }

  if (self->output_pool) {
    gst_drm_buffer_pool_destroy (self->output_pool);
    self->output_pool = NULL;
  }

  /* reset cropping rect */
  memset (&self->rect, 0, sizeof (GstDucatiVideoRectangle));

  g_queue_clear(&self->can_queue);

  return TRUE;
}

static GstFlowReturn
gst_ducati_videnc_finish (GstVideoEncoder * base_video_encoder)
{
  GstDucatiVidEnc *self = GST_DUCATIVIDENC (base_video_encoder);

  GST_DEBUG_OBJECT (self, "finish");

  return GST_FLOW_OK;
}

static int
gst_ducati_videnc_buffer_lock (GstDucatiVidEnc * self, GstBuffer * buf)
{
  int fd;
  GstMetaDmaBuf *dmabuf = gst_buffer_get_dma_buf_meta (buf);
  if (!dmabuf) {
    //GST_ERROR_OBJECT (self, "invalid dmabuf for buf = %p", buf);
    return -1;
  }
  fd = gst_dma_buf_meta_get_fd (dmabuf);
  if (fd < 0) {
    GST_ERROR_OBJECT (self, "Invalid dma buf fd %d", fd);
    return -1;
  }
  dce_buf_lock (1, (size_t *) & fd);
  return fd;
}

static void
gst_ducati_videnc_buffer_unlock (GstDucatiVidEnc * self, GstBuffer * buf)
{
  int fd;
  GstMetaDmaBuf *dmabuf = gst_buffer_get_dma_buf_meta (buf);
  if (!dmabuf) {
    GST_ERROR_OBJECT (self, "invalid dmabuf for buf = %p", buf);
    return;
  }
  fd = gst_dma_buf_meta_get_fd (dmabuf);
  if (fd < 0) {
    GST_ERROR_OBJECT (self, "Invalid dma buf fd %d", fd);
    return;
  }
  dce_buf_unlock (1, (size_t *) & fd);
}

static GstFlowReturn
gst_ducati_videnc_handle_frame (GstVideoEncoder * base_video_encoder,
    GstVideoCodecFrame * frame)
{
  GstDucatiVidEnc *self = GST_DUCATIVIDENC (base_video_encoder);
  GstBuffer *inbuf, *outbuf;
  GstBuffer *output_buffer;
  int dmabuf_fd_in, dmabuf_fd_out;
  XDAS_Int32 err;
  const GstVideoCodecState *state;
  int i;
  GstClockTime ts;
  GstClockTime t;
  GstVideoCropMeta *crop;
  QueueData *pCan;
  GstMetaCanBuf *canbuf = NULL;

  state = self->input_state;

  if (G_UNLIKELY (self->configure)) {
    if (!gst_ducati_videnc_configure (self)) {
      GST_DEBUG_OBJECT (self, "configure failed");
      GST_ELEMENT_ERROR (self, STREAM, ENCODE, (NULL), (NULL));

      return GST_FLOW_ERROR;
    }
  }

  inbuf = gst_buffer_ref (frame->input_buffer);
  
  canbuf = gst_buffer_get_can_buf_meta(frame->input_buffer);
  if(canbuf != NULL)
  {
  	pCan = (QueueData *)g_malloc(sizeof(QueueData));
  	memcpy(&pCan->can_data, gst_can_buf_meta_get_can_data(canbuf), sizeof(Ofilm_Can_Data_T));
  	g_queue_push_head(&self->can_queue, pCan);
  }
  
  ts = GST_BUFFER_PTS (inbuf);
have_inbuf:
  dmabuf_fd_in = gst_ducati_videnc_buffer_lock (self, inbuf);
  if (dmabuf_fd_in < 0) {
    GstMapInfo info;
    gboolean mapped;

    GST_DEBUG_OBJECT (self, "memcpying input");
    gst_buffer_unref (inbuf);
    inbuf = GST_BUFFER (gst_drm_buffer_pool_get (self->input_pool, FALSE));
	if(inbuf == NULL)
	{
	    GST_ERROR_OBJECT (self, "Failed to get drm buffer");
		return GST_FLOW_ERROR;
	}
	
    gst_buffer_pool_set_active (GST_BUFFER_POOL (self->input_pool), TRUE);

    mapped = gst_buffer_map (frame->input_buffer, &info, GST_MAP_READ);
    if (mapped) {
      gst_buffer_fill (inbuf, 0, info.data, info.size);
      gst_buffer_unmap (frame->input_buffer, &info);
    }

    GST_BUFFER_PTS (inbuf) = ts;		  
    goto have_inbuf;
  }

  outbuf = GST_BUFFER (gst_drm_buffer_pool_get (self->output_pool, FALSE));
  if(outbuf == NULL)
  {
	  GST_ERROR_OBJECT (self, "Failed to get drm buffer");
	  return GST_FLOW_ERROR;
  }
  gst_buffer_pool_set_active (GST_BUFFER_POOL (self->output_pool), TRUE);
  crop = gst_buffer_get_video_crop_meta (outbuf);
  crop->width = GST_VIDEO_INFO_WIDTH (&state->info);
  crop->height = GST_VIDEO_INFO_HEIGHT (&state->info);
  dmabuf_fd_out = gst_ducati_videnc_buffer_lock (self, outbuf);

  self->inBufs->planeDesc[0].buf = (XDAS_Int8 *) dmabuf_fd_in;
  self->inBufs->planeDesc[0].memType = XDM_MEMTYPE_RAW;
  self->inBufs->planeDesc[0].bufSize.tileMem.width =
      GST_VIDEO_INFO_WIDTH (&state->info);
  self->inBufs->planeDesc[0].bufSize.tileMem.height =
      GST_VIDEO_INFO_HEIGHT (&state->info);
  self->inBufs->planeDesc[0].bufSize.bytes =
      GST_VIDEO_INFO_WIDTH (&state->info) *
      GST_VIDEO_INFO_HEIGHT (&state->info);
  self->inBufs->planeDesc[1].buf = (XDAS_Int8 *) dmabuf_fd_in;
  self->inBufs->planeDesc[1].memType = XDM_MEMTYPE_RAW;
  self->inBufs->planeDesc[1].bufSize.tileMem.width =
      GST_VIDEO_INFO_WIDTH (&state->info);
  self->inBufs->planeDesc[1].bufSize.tileMem.height =
      GST_VIDEO_INFO_HEIGHT (&state->info) / 2;
  self->inBufs->planeDesc[1].bufSize.bytes =
      GST_VIDEO_INFO_WIDTH (&state->info) *
      GST_VIDEO_INFO_HEIGHT (&state->info) / 2;
  /* setting imageRegion doesn't seem to be strictly needed if activeFrameRegion
   * is set but we set it anyway...
   */
  self->inBufs->imageRegion.topLeft.x = self->rect.x;
  self->inBufs->imageRegion.topLeft.y = self->rect.y;
  self->inBufs->imageRegion.bottomRight.x = self->rect.x + self->rect.w;
  self->inBufs->imageRegion.bottomRight.y = self->rect.y + self->rect.h;
  self->inBufs->activeFrameRegion.topLeft.x = self->rect.x;
  self->inBufs->activeFrameRegion.topLeft.y = self->rect.y;
  self->inBufs->activeFrameRegion.bottomRight.x = self->rect.x + self->rect.w;
  self->inBufs->activeFrameRegion.bottomRight.y = self->rect.y + self->rect.h;
  self->inBufs->imagePitch[0] = GST_VIDEO_INFO_WIDTH (&state->info);
  self->inBufs->imagePitch[1] = GST_VIDEO_INFO_WIDTH (&state->info);
  self->inBufs->topFieldFirstFlag = TRUE;

  self->outBufs->numBufs = 1;
  self->outBufs->descs[0].buf = (XDAS_Int8 *) dmabuf_fd_out;
  self->outBufs->descs[0].bufSize.bytes = self->output_pool->size;
  self->outBufs->descs[0].memType = XDM_MEMTYPE_RAW;

  self->inArgs->inputID = GPOINTER_TO_INT (inbuf);

  GST_DEBUG ("Calling VIDENC2_process");
  t = gst_util_get_timestamp ();
  err = VIDENC2_process (self->codec, self->inBufs, self->outBufs,
      self->inArgs, self->outArgs);
  t = gst_util_get_timestamp () - t;
  GST_DEBUG_OBJECT (self, "VIDENC2_process took %10dns (%d ms)", (gint) t,
      (gint) (t / 1000000));
  gst_ducati_videnc_buffer_unlock (self, outbuf);
  if (err) {
    GST_WARNING_OBJECT (self, "process failed: err=%d, extendedError=%08x",
        err, self->outArgs->extendedError);
    gst_ducati_log_extended_error_info (self->outArgs->extendedError,
        self->error_strings);

    err = VIDENC2_control (self->codec,
        XDM_GETSTATUS, (IVIDENC2_DynamicParams *) self->dynParams,
        self->status);

    GST_WARNING_OBJECT (self, "XDM_GETSTATUS: err=%d, extendedError=%08x",
        err, self->status->extendedError);

	gst_buffer_unref (outbuf);

    return GST_FLOW_OK;
  }

  if (self->outArgs->bytesGenerated > 0) {
    GstMapInfo info;
    gboolean mapped;
    GstMetaCanBuf *pCanMeta;	
    if (GST_DUCATIVIDENC_GET_CLASS (self)->is_sync_point (self,
            self->outArgs->encodedFrameType)) {
      GST_VIDEO_CODEC_FRAME_SET_SYNC_POINT (frame);
    }
    if (frame->output_buffer) {
      gst_buffer_unref (frame->output_buffer);
    }

    frame->output_buffer =
        gst_video_encoder_allocate_output_buffer (GST_VIDEO_ENCODER (self),
        self->outArgs->bytesGenerated);
    mapped = gst_buffer_map (outbuf, &info, GST_MAP_READ);
    if (mapped) {
      gst_buffer_fill (frame->output_buffer, 0, info.data, info.size);
      gst_buffer_unmap (outbuf, &info);
    }

	pCan = (QueueData *)g_queue_pop_tail(&self->can_queue);
	if(pCan != NULL)
	{
		pCanMeta = gst_buffer_add_can_buf_meta(GST_BUFFER(frame->output_buffer), &pCan->can_data, sizeof(Ofilm_Can_Data_T));
		if (!pCanMeta){
			GST_ERROR("Failed to add can meta to buffer");
		}
		g_free(pCan);
	}

    GST_CAT_DEBUG_OBJECT (GST_CAT_PERFORMANCE, self,
        "Encoded frame in %u bytes", self->outArgs->bytesGenerated);

    /* As we can get frames in a different order we sent them (if the codec
       supports B frames and we set it up for generating those), we need to
       work out what input frame corresponds to the frame we just got, to
       keep presentation times correct.
       It seems that the codec will free buffers in the right order for this,
       but I can not find anything saying this in the docs, so:
       - it might be subject to change
       - it might not be true in all setups
       - it might not be true for all codecs
       However, that's the only way I can see to do it. So there's a nice
       assert below that will blow up if the codec does not free exactly one
       input frame when it outputs a frame. That doesn't catch all cases,
       such as when it frees them in the wrong order, but that seems less
       likely to happen.
       The timestamp and duration are given to the base class, which will
       in turn set them onto the encoded buffer. */
    g_assert (self->outArgs->freeBufID[0] && !self->outArgs->freeBufID[1]);
    inbuf = GST_BUFFER (self->outArgs->freeBufID[0]);
    frame->pts = GST_BUFFER_PTS (inbuf);
    frame->duration = GST_BUFFER_DURATION (inbuf);
    GST_BUFFER_OFFSET_END (frame->output_buffer) = GST_BUFFER_PTS (inbuf);
  }

  gst_buffer_unref (outbuf);

  for (i = 0; self->outArgs->freeBufID[i]; i++) {
    GstBuffer *buf = (GstBuffer *) self->outArgs->freeBufID[i];

    GST_LOG_OBJECT (self, "free buffer: %p", buf);
    gst_ducati_videnc_buffer_unlock (self, buf);
    gst_buffer_unref (buf);
  }

  return gst_video_encoder_finish_frame (base_video_encoder, frame);
}

static gboolean
gst_ducati_videnc_is_sync_point_default (GstDucatiVidEnc * enc, int type)
{
  return type == IVIDEO_I_FRAME;
}

static gboolean
gst_ducati_videnc_event (GstVideoEncoder * enc, GstEvent * event)
{
  gboolean handled = FALSE;
  GstDucatiVidEnc *self = GST_DUCATIVIDENC (enc);

  switch (GST_EVENT_TYPE (event)) {
    default:
      break;
  }

  return handled;
}
