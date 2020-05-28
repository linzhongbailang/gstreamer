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

#ifndef __GST_DUCATIVIDENC_H__
#define __GST_DUCATIVIDENC_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideoencoder.h>
#include <gst/video/gstvideoutils.h>
#include <gst/drm/gstdrmbufferpool.h>

#include <ti/sdo/ce/video2/videnc2.h>

#include <gst/canbuf/canbuf.h>

#define GST_TYPE_DUCATIVIDENC \
  (gst_ducati_videnc_get_type())
#define GST_DUCATIVIDENC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_DUCATIVIDENC,GstDucatiVidEnc))
#define GST_DUCATIVIDENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_DUCATIVIDENC,GstDucatiVidEncClass))
#define GST_IS_DUCATIVIDENC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_DUCATIVIDENC))
#define GST_IS_DUCATIVIDENC_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_DUCATIVIDENC))
#define GST_DUCATIVIDENC_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_DUCATIVIDENC, GstDucatiVidEncClass))

typedef struct _GstDucatiVidEnc GstDucatiVidEnc;
typedef struct _GstDucatiVidEncClass GstDucatiVidEncClass;

typedef struct {
  gint x;
  gint y;
  gint w;
  gint h;
} GstDucatiVideoRectangle;

typedef struct
{
  Ofilm_Can_Data_T can_data;
} QueueData;

struct _GstDucatiVidEnc
{
  GstVideoEncoder base_encoder;

  GstPad *sinkpad;
  GstPad *srcpad;

  struct omap_device *device;
  struct omap_device *external_dce_device;
  Engine_Handle engine;
  VIDENC2_Handle codec;
  IVIDENC2_Params *params;
  IVIDENC2_DynamicParams *dynParams;
  IVIDENC2_Status *status;
  IVIDEO2_BufDesc *inBufs;
  XDM2_BufDesc *outBufs;
  IVIDENC2_InArgs *inArgs;
  IVIDENC2_OutArgs *outArgs;

  GstDRMBufferPool *input_pool;
  GstDRMBufferPool *output_pool;
  GstDRMBufferPool *external_inpool;
  gboolean configure;

  GstDucatiVideoRectangle rect;
  GstVideoCodecState * input_state;

  gint bitrate;
  guint rate_preset;
  guint intra_interval;

  const char *error_strings[32];
  GQueue can_queue;
};

struct _GstDucatiVidEncClass
{
  GstVideoEncoderClass parent_class;

  const gchar *codec_name;
  gboolean (*allocate_params) (GstDucatiVidEnc * self, gint params_sz,
      gint dynparams_sz, gint status_sz, gint inargs_sz, gint outargs_sz);
  gboolean (*configure) (GstDucatiVidEnc * self);
  gboolean (*is_sync_point) (GstDucatiVidEnc * self, int type);
};

GType gst_ducati_videnc_get_type (void);

#endif
