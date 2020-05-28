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

#ifndef __GST_DUCATIMPEG4ENC_H__
#define __GST_DUCATIMPEG4ENC_H__

#include <ti/sdo/codecs/mpeg4enc/impeg4enc.h>
#include "gstducatividenc.h"

#define GST_TYPE_DUCATIMPEG4ENC \
  (gst_ducati_mpeg4enc_get_type())
#define GST_DUCATIMPEG4ENC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_DUCATIMPEG4ENC,GstDucatiMPEG4Enc))
#define GST_DUCATIMPEG4ENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_DUCATIMPEG4ENC,GstDucatiMPEG4EncClass))
#define GST_IS_DUCATIMPEG4ENC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_DUCATIMPEG4ENC))
#define GST_IS_DUCATIMPEG4ENC_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_DUCATIMPEG4ENC))
#define GST_DUCATIMPEG4ENC_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_DUCATIMPEG4ENC, GstDucatiMPEG4EncClass))

typedef struct _GstDucatiMPEG4Enc GstDucatiMPEG4Enc;
typedef struct _GstDucatiMPEG4EncClass GstDucatiMPEG4EncClass;

struct _GstDucatiMPEG4Enc
{
  GstDucatiVidEnc parent;
  guint profile;
  guint level;
};

struct _GstDucatiMPEG4EncClass
{
  GstDucatiVidEncClass parent_class;
};

GType gst_ducati_mpeg4enc_get_type (void);

#endif
