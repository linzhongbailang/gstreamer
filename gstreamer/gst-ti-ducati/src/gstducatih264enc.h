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

#ifndef __GST_DUCATIH264ENC_H__
#define __GST_DUCATIH264ENC_H__

#include <ti/sdo/codecs/h264enc/ih264enc.h>
#include "gstducatividenc.h"

#define GST_TYPE_DUCATIH264ENC \
  (gst_ducati_h264enc_get_type())
#define GST_DUCATIH264ENC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_DUCATIH264ENC,GstDucatiH264Enc))
#define GST_DUCATIH264ENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_DUCATIH264ENC,GstDucatiH264EncClass))
#define GST_IS_DUCATIH264ENC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_DUCATIH264ENC))
#define GST_IS_DUCATIH264ENC_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_DUCATIH264ENC))
#define GST_DUCATIH264ENC_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_DUCATIH264ENC, GstDucatiH264EncClass))

typedef struct _GstDucatiH264Enc GstDucatiH264Enc;
typedef struct _GstDucatiH264EncClass GstDucatiH264EncClass;

struct _GstDucatiH264Enc
{
  GstDucatiVidEnc parent;

  guint profile;
  guint level;

  gint qp_min_i;
  gint qpi;
  gint qp_max_i;
  guint hrd_buffer_size;

  IH264ENC_RateControlParamsPreset rate_control_params_preset;
  IH264ENC_RateControlAlgo rate_control_algo;
  IH264ENC_EntropyCodingMode entropy_coding_mode;
  IH264ENC_SliceMode slice_mode;
  guint inter_interval;
};

struct _GstDucatiH264EncClass
{
  GstDucatiVidEncClass parent_class;
};

GType gst_ducati_h264enc_get_type (void);

#endif
