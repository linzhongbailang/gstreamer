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

#ifndef __GST_DUCATIJPEGDEC_H__
#define __GST_DUCATIJPEGDEC_H__

#include "gstducatividdec.h"

#include <ti/sdo/codecs/jpegvdec/ijpegvdec.h>


G_BEGIN_DECLS

#define GST_TYPE_DUCATIJPEGDEC              (gst_ducati_jpegdec_get_type())
#define GST_DUCATIJPEGDEC(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_DUCATIJPEGDEC, GstDucatiJpegDec))
#define GST_DUCATIJPEGDEC_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_DUCATIJPEGDEC, GstDucatiJpegDecClass))
#define GST_IS_DUCATIJPEGDEC(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_DUCATIJPEGDEC))
#define GST_IS_DUCATIJPEGDEC_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_DUCATIJPEGDEC))

typedef struct _GstDucatiJpegDec      GstDucatiJpegDec;
typedef struct _GstDucatiJpegDecClass GstDucatiJpegDecClass;

struct _GstDucatiJpegDec
{
  GstDucatiVidDec parent;

  gboolean prepend_codec_data;
};

struct _GstDucatiJpegDecClass
{
  GstDucatiVidDecClass parent_class;
};

GType gst_ducati_jpegdec_get_type (void);

G_END_DECLS

#endif /* __GST_DUCATIJPEGDEC_H__ */
