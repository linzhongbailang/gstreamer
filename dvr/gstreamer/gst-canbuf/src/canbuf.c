/* GStreamer canbuf
 */

/**
 * SECTION:gstcanbuf
 * @short_description: GStreamer canbuf metadata support
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <string.h>
#include <gst/canbuf/canbuf.h>


static GType gst_meta_can_buf_api_get_type (void);
#define GST_META_CAN_BUF_API_TYPE (gst_meta_can_buf_api_get_type())

static const GstMetaInfo *gst_meta_can_buf_get_info (void);
#define GST_META_CAN_BUF_INFO (gst_meta_can_buf_get_info())

#define GST_META_CAN_BUF_GET(buf) ((GstMetaCanBuf *)gst_buffer_get_meta(buf,GST_META_CAN_BUF_API_TYPE))
#define GST_META_CAN_BUF_ADD(buf) ((GstMetaCanBuf *)gst_buffer_add_meta(buf,GST_META_CAN_BUF_INFO,NULL))


static gboolean
canbuf_init_func (GstMeta * meta, gpointer params, GstBuffer * buffer)
{
   GST_DEBUG ("init called on buffer %p, meta %p", buffer, meta);
  /* nothing to init really, the init function is mostly for allocating
   * additional memory or doing special setup as part of adding the metadata to
   * the buffer*/
  return TRUE;
}

static void
canbuf_free_func (GstMeta * meta, GstBuffer * buffer)
{
   /* close the file descriptor associated with the buffer, 
    * that is stored as metadata*/
   GstMetaCanBuf *canbuf = (GstMetaCanBuf *) meta;
   GST_DEBUG ("free called on buffer %p, meta %p", buffer, meta);
}

static gboolean
canbuf_transform_func (GstBuffer * transbuf, GstMeta * meta,
    GstBuffer * buffer, GQuark type, gpointer data)
{
  GstMetaCanBuf *transmeta, *tmeta = (GstMetaCanBuf *) meta;

  GST_DEBUG ("transform %s called from buffer %p to %p, meta %p",
     g_quark_to_string (type), buffer, transbuf, meta);

  if (GST_META_TRANSFORM_IS_COPY (type)) {
    GstMetaTransformCopy *copy_data = data;
    
    if (!copy_data->region){
    /* only copy if the complete data is copied as well */
       transmeta = GST_META_CAN_BUF_ADD (transbuf);
       if(!transmeta) {
          return FALSE;
       }
    /* create a copy of the can data*/
	   memcpy(&transmeta->can_data, &tmeta->can_data, sizeof(Ofilm_Can_Data_T));
    }
  }
  return TRUE;
}

static GType
gst_meta_can_buf_api_get_type (void)
{
  static volatile GType type;
  static const gchar *tags[] = { "canbuf", NULL };

  if (g_once_init_enter (&type)) {
    GType _type = gst_meta_api_type_register ("GstMetaCanBufAPI", tags);
    g_once_init_leave (&type, _type);
  }
  return type;
}

static const GstMetaInfo *
gst_meta_can_buf_get_info (void)
{
  static const GstMetaInfo *meta_can_buf_info = NULL;

  if (g_once_init_enter (&meta_can_buf_info)) {
    const GstMetaInfo *mi = gst_meta_register (GST_META_CAN_BUF_API_TYPE,
        "GstMetaCanBuf",
        sizeof (GstMetaCanBuf),
        canbuf_init_func, canbuf_free_func, canbuf_transform_func);
    g_once_init_leave (&meta_can_buf_info, mi);
  }
  return meta_can_buf_info;
}



/**
 * gst_buffer_add_can_buf_meta:
 * @buf: a #GstBuffer
 * @fd: the associated file descriptor to be added as metadata
 *
 * Adds a GstMetaCanBuf metadata to the buffer. 
 * 
 * Returns: the #GstMetaCanBuf on @buf
 *
 */

GstMetaCanBuf *
gst_buffer_add_can_buf_meta (GstBuffer * buf, void *can, gint can_size)
{
  GstMetaCanBuf *canbuf;
  canbuf = GST_META_CAN_BUF_ADD (buf);
  if (canbuf)
  {
	  if ((can != NULL) && (can_size != sizeof(Ofilm_Can_Data_T)))
	  {
		  g_print("can size mismatch, expect:%d, actual:%d\n", sizeof(Ofilm_Can_Data_T), can_size);
		  return NULL;
	  }
	  memcpy(&canbuf->can_data, can, can_size);
  }
  return canbuf;
}

/**
 * gst_buffer_get_can_buf_meta:
 * @buf: a #GstBuffer
 *
 * Get the GstMetaCanBuf metadata that has previously been attached to a buffer
 * with gst_buffer_add_can_buf_meta(), usually by another element
 * upstream.
 *
 * Returns: the #GstMetaCanBuf attached to @buf
 *
 */
GstMetaCanBuf *
gst_buffer_get_can_buf_meta (GstBuffer * buf)
{
  GstMetaCanBuf * canbuf;
  canbuf = GST_META_CAN_BUF_GET (buf);
  return canbuf;
}


/**
 * gst_can_buf_meta_get_fd:
 * @canbuf: a #GstMetaCanBuf
 *
 * Returns: the can data set as GstMetaCanBuf metadata 
 * that has previously been attached to a buffer
 * with gst_buffer_add_can_buf_meta()
 *
 */
Ofilm_Can_Data_T* gst_can_buf_meta_get_can_data (GstMetaCanBuf * canbuf)
{
  if(canbuf) {
    return &canbuf->can_data;
  } else {
    GST_DEBUG("Received Null parameter for GstMetaCanBuf ");
    return 0;
  }
}
