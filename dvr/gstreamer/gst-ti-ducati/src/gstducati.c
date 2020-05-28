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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "gstducati.h"
#include "gstducatih264dec.h"
#include "gstducatimpeg4dec.h"
#include "gstducatimpeg2dec.h"
#include "gstducativc1dec.h"
#include "gstducatijpegdec.h"

#include "gstducatih264enc.h"
#include "gstducatimpeg4enc.h"


GST_DEBUG_CATEGORY (gst_ducati_debug);

void
gst_ducati_set_generic_error_strings (const char *strings[])
{
#ifndef GST_DISABLE_GST_DEBUG
  strings[XDM_PARAMSCHANGE] = "sequence parameters change";
  strings[XDM_APPLIEDCONCEALMENT] = "applied concealment";
  strings[XDM_INSUFFICIENTDATA] = "insufficient data";
  strings[XDM_CORRUPTEDDATA] = "corrupted data";
  strings[XDM_CORRUPTEDHEADER] = "corrupted header";
  strings[XDM_UNSUPPORTEDINPUT] = "unsupported input";
  strings[XDM_UNSUPPORTEDPARAM] = "unsupported param";
  strings[XDM_FATALERROR] = "fatal";
#endif
}

#ifndef GST_DISABLE_GST_DEBUG
void
gst_ducati_log_extended_error_info (uint32_t error, const char *strings[])
{
  int bit = 0;
  while (error) {
    if (error & 1) {
      GST_ERROR ("Bit %d (%08x): %s", bit, 1 << bit,
          strings[bit] ? strings[bit] : "unknown");
    }
    error >>= 1;
    ++bit;
  }
}
#endif

static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_ducati_debug, "ducati", 0, "ducati");

  return gst_element_register (plugin, "ducatih264dec", GST_RANK_PRIMARY,
      GST_TYPE_DUCATIH264DEC) &&
      gst_element_register (plugin, "ducatimpeg4dec", GST_RANK_PRIMARY,
      GST_TYPE_DUCATIMPEG4DEC) &&
      gst_element_register (plugin, "ducatimpeg2dec", GST_RANK_PRIMARY,
      GST_TYPE_DUCATIMPEG2DEC) &&
      gst_element_register (plugin, "ducativc1dec", GST_RANK_PRIMARY,
      GST_TYPE_DUCATIVC1DEC) &&
      gst_element_register (plugin, "ducatijpegdec", GST_RANK_SECONDARY,
      GST_TYPE_DUCATIJPEGDEC) &&
      gst_element_register (plugin, "ducatih264enc", GST_RANK_PRIMARY + 1,
      GST_TYPE_DUCATIH264ENC) &&
      gst_element_register (plugin, "ducatimpeg4enc", GST_RANK_PRIMARY + 1,
      GST_TYPE_DUCATIMPEG4ENC);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#  define PACKAGE "ducati"
#endif

#ifndef VERSION
#  define VERSION "1.0.0"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR, GST_VERSION_MINOR, ducati ,
    "Hardware accelerated codecs for OMAP4",
    plugin_init, VERSION, "LGPL", "GStreamer", "http://gstreamer.net/")
