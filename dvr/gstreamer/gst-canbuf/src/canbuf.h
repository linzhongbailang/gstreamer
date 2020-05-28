/* GStreamer canbuf
 */

#ifndef __GST_CAN_BUF_H__
#define __GST_CAN_BUF_H__

#include <gst/gst.h>
#include "can.h"

G_BEGIN_DECLS

/**
 * GstMetaCanBuf:
 *
 * A structure that enables setting file descriptor as a metadata on a buffer.
 *
 */


typedef struct
{
  GstMeta meta;

  Ofilm_Can_Data_T	can_data;
} GstMetaCanBuf;



/* attach GstMetaCanBuf metadata to buffers */
GstMetaCanBuf * gst_buffer_add_can_buf_meta (GstBuffer * buf, void *can, gint can_size);

/* retrieve GstMetaCanBuf metadata from buffers */
GstMetaCanBuf * gst_buffer_get_can_buf_meta (GstBuffer * buf);

/* retrieve can value from a GstMetaCanBuf metadata */
Ofilm_Can_Data_T* gst_can_buf_meta_get_can_data (GstMetaCanBuf * canbuf);

/* retrieve urgent flag from a GstMetaCanBuf metadata */
gboolean gst_can_buf_meta_get_urgent_flag (GstMetaCanBuf * canbuf);

G_END_DECLS

#endif /* __GST_CAN_BUF_H__ */
