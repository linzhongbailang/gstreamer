#define USE_DTS_PTS_CODE
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
#include <config.h>
#endif

#include "gstducatividdec.h"
#include "gstducatibufferpriv.h"
#include <gst/canbuf/canbuf.h>

#define VERSION_LENGTH 256

static void gst_ducati_viddec_class_init (GstDucatiVidDecClass * klass);
static void gst_ducati_viddec_init (GstDucatiVidDec * self, gpointer klass);
static void gst_ducati_viddec_base_init (gpointer gclass);
static GstElementClass *parent_class = NULL;
void gst_drm_buffer_pool_set_caps (GstDRMBufferPool * self, GstCaps * caps);


void
gst_drm_buffer_pool_set_caps (GstDRMBufferPool * self, GstCaps * caps)
{
  GstStructure *conf;
  conf = gst_buffer_pool_get_config (GST_BUFFER_POOL (self));
  gst_buffer_pool_config_set_params (conf, caps, self->size, 0, 0);
  gst_drm_buffer_pool_set_config (GST_BUFFER_POOL (self), conf);

}

GType
gst_ducati_viddec_get_type (void)
{
  static GType ducati_viddec_type = 0;

  if (!ducati_viddec_type) {
    static const GTypeInfo ducati_viddec_info = {
      sizeof (GstDucatiVidDecClass),
      (GBaseInitFunc) gst_ducati_viddec_base_init,
      NULL,
      (GClassInitFunc) gst_ducati_viddec_class_init,
      NULL,
      NULL,
      sizeof (GstDucatiVidDec),
      0,
      (GInstanceInitFunc) gst_ducati_viddec_init,
    };

    ducati_viddec_type = g_type_register_static (GST_TYPE_ELEMENT,
        "GstDucatiVidDec", &ducati_viddec_info, 0);
  }
  return ducati_viddec_type;
}

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("NV12"))
    );

enum
{
  PROP_0,
  PROP_VERSION,
  PROP_MAX_REORDER_FRAMES,
  PROP_CODEC_DEBUG_INFO
};

/* helper functions */

static void
engine_close (GstDucatiVidDec * self)
{
  if (self->params) {
    dce_free (self->params);
    self->params = NULL;
  }

  if (self->dynParams) {
    dce_free (self->dynParams);
    self->dynParams = NULL;
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

  if (self->inArgs) {
    dce_free (self->inArgs);
    self->inArgs = NULL;
  }

  if (self->outArgs) {
    dce_free (self->outArgs);
    self->outArgs = NULL;
  }

  if (self->engine) {
    Engine_close (self->engine);
    self->engine = NULL;
  }

  if (self->device) {
    dce_deinit (self->device);
    self->device = NULL;
  }
}

static gboolean
engine_open (GstDucatiVidDec * self)
{
  gboolean ret;
  int ec;

  if (G_UNLIKELY (self->engine)) {
    return TRUE;
  }

  if (self->device == NULL) {
    self->device = dce_init ();
    if (self->device == NULL) {
      GST_ERROR_OBJECT (self, "dce_init() failed");
      return FALSE;
    }
  }

  GST_DEBUG_OBJECT (self, "opening engine");

  self->engine = Engine_open ((String) "ivahd_vidsvr", NULL, &ec);
  if (G_UNLIKELY (!self->engine)) {
    GST_ERROR_OBJECT (self, "could not create engine");
    return FALSE;
  }

  ret = GST_DUCATIVIDDEC_GET_CLASS (self)->allocate_params (self,
      sizeof (IVIDDEC3_Params), sizeof (IVIDDEC3_DynamicParams),
      sizeof (IVIDDEC3_Status), sizeof (IVIDDEC3_InArgs),
      sizeof (IVIDDEC3_OutArgs));

  return ret;
}

static void
codec_delete (GstDucatiVidDec * self)
{
  if (self->pool) {
    gst_drm_buffer_pool_destroy (self->pool);
    self->pool = NULL;
  }

  if (self->codec) {
    GST_DEBUG ("Calling VIDDEC3_delete");
    VIDDEC3_delete (self->codec);
    self->codec = NULL;
  }

  if (self->input_bo) {
    close ((int) self->inBufs->descs[0].buf);
    omap_bo_del (self->input_bo);
    self->input_bo = NULL;
  }
}

static gboolean
codec_create (GstDucatiVidDec * self)
{
  gint err, n;
  const gchar *codec_name;
  char *version = NULL;

  codec_delete (self);

  if (G_UNLIKELY (!self->engine)) {
    GST_ERROR_OBJECT (self, "no engine");
    return FALSE;
  }

  /* these need to be set before VIDDEC3_create */
  self->params->maxWidth = self->width;
  self->params->maxHeight = self->height;

  codec_name = GST_DUCATIVIDDEC_GET_CLASS (self)->codec_name;

  /* create codec: */
  GST_DEBUG_OBJECT (self, "creating codec: %s", codec_name);
  self->codec =
      VIDDEC3_create (self->engine, (String) codec_name, self->params);

  if (!self->codec) {
    return FALSE;
  }

  GST_DEBUG ("Calling VIDDEC3_control XDM_SETPARAMS");
  err =
      VIDDEC3_control (self->codec, XDM_SETPARAMS, self->dynParams,
      self->status);
  if (err) {
    GST_ERROR_OBJECT (self, "failed XDM_SETPARAMS");
    return FALSE;
  }

  self->first_in_buffer = TRUE;
  self->first_out_buffer = FALSE;

  version = dce_alloc (VERSION_LENGTH);
  if (version) {
    self->status->data.buf = (XDAS_Int8 *) version;
    self->status->data.bufSize = VERSION_LENGTH;

    GST_DEBUG ("Calling VIDDEC3_control XDM_GETVERSION");
    err = VIDDEC3_control (self->codec, XDM_GETVERSION,
        self->dynParams, self->status);

    if (err) {
      GST_ERROR_OBJECT (self, "failed XDM_GETVERSION");
    } else
      GST_DEBUG ("Codec version %s", self->status->data.buf);

    self->status->data.buf = NULL;
    self->status->data.bufSize = 0;
    dce_free (version);

  }


  /* allocate input buffer and initialize inBufs: */
  /* FIXME:  needed size here has nothing to do with width * height */
  self->input_bo = omap_bo_new (self->device,
      self->width * self->height, OMAP_BO_WC);
  if (!self->input_bo) {
    GST_ERROR_OBJECT (self, "Failed to create bo");
    return FALSE;
  }  
  self->input = omap_bo_map (self->input_bo);
  self->inBufs->numBufs = 1;
  /* IPC requires dmabuf fd in place of bo handle */
  self->inBufs->descs[0].buf = (XDAS_Int8 *) omap_bo_dmabuf (self->input_bo);

  /* Actual buffers will be set later, as they will be different for every
     frame. We allow derived classes to add their own buffers, however, so
     we initialize the number of outBufs here, counting the number of extra
     buffers required, including "holes" in planes, which may not be filled
     if not assigned. */
  self->outBufs->numBufs = 2;   /* luma and chroma planes, always */
  for (n = 0; n < 3; n++)
    if (self->params->metadataType[n] != IVIDEO_METADATAPLANE_NONE)
      self->outBufs->numBufs = 2 + n + 1;

  return TRUE;
}

static inline GstBuffer *
codec_buffer_pool_get (GstDucatiVidDec * self, GstBuffer * buf)
{
  GstBuffer *ret_buf;
  if (G_UNLIKELY (!self->pool)) {
    guint size =
        GST_ROUND_UP_4 (self->padded_width) *
        GST_ROUND_UP_2 (self->padded_height) * 3 / 2;

    GST_DEBUG_OBJECT (self, "creating bufferpool");
    GST_DEBUG_OBJECT (self, "%s\n",
        gst_caps_to_string (gst_pad_get_current_caps (self->srcpad)));
    self->pool =
        gst_drm_buffer_pool_new (GST_ELEMENT (self), dce_get_fd (),
        gst_pad_get_current_caps (self->srcpad), size);
  }
  return GST_BUFFER (gst_drm_buffer_pool_get (self->pool, FALSE));
}

static GstMetaDucatiBufferPriv *
get_buffer_priv (GstDucatiVidDec * self, GstBuffer * buf)
{
  GstMetaDucatiBufferPriv *priv = gst_ducati_buffer_priv_get (buf);
  if (!priv) {
    GstVideoFormat format = GST_VIDEO_FORMAT_NV12;
    struct omap_bo *bo;
    gint uv_offset, size;

    GstMetaDmaBuf *dmabuf = gst_buffer_get_dma_buf_meta (buf);

    /* if it isn't a dmabuf buffer that we can import, then there
     * is nothing we can do with it:
     */
    if (!dmabuf) {
      GST_DEBUG_OBJECT (self, "not importing non dmabuf buffer");
      return NULL;
    }

    bo = omap_bo_from_dmabuf (self->device, gst_dma_buf_meta_get_fd (dmabuf));

    uv_offset =
        GST_ROUND_UP_4 (self->stride) * GST_ROUND_UP_2 (self->padded_height);
    size =
        GST_ROUND_UP_4 (self->stride) * GST_ROUND_UP_2 (self->padded_height) *
        3 / 2;

    priv = gst_ducati_buffer_priv_set (buf, bo, uv_offset, size);
  }
  return priv;
}

static XDAS_Int32
codec_prepare_outbuf (GstDucatiVidDec * self, GstBuffer ** buf,
    gboolean force_internal)
{
  GstMetaDucatiBufferPriv *priv = NULL;
  GstMetaDmaBuf *dmabuf = NULL;

  if (!force_internal)
    priv = get_buffer_priv (self, *buf);

  if (!priv) {
    GstBuffer *orig = *buf;

    GST_DEBUG_OBJECT (self, "internal bufferpool forced");
    *buf = codec_buffer_pool_get (self, NULL);
    GST_BUFFER_PTS (*buf) = GST_BUFFER_PTS (orig);
    GST_BUFFER_DURATION (*buf) = GST_BUFFER_DURATION (orig);
    gst_buffer_unref (orig);
    return codec_prepare_outbuf (self, buf, FALSE);
  }

  /* There are at least two buffers. Derived classes may add codec specific
     buffers (eg, debug info) after these two if they want to. */
  dmabuf = gst_buffer_get_dma_buf_meta (*buf);
  /* XDM_MemoryType required by drm to allcoate buffer */
  self->outBufs->descs[0].memType = XDM_MEMTYPE_RAW;
  /* IPC requires dmabuf fd in place of bo handle */
  self->outBufs->descs[0].buf = (XDAS_Int8 *) gst_dma_buf_meta_get_fd (dmabuf);
  self->outBufs->descs[0].bufSize.bytes = priv->uv_offset;
  self->outBufs->descs[1].memType = XDM_MEMTYPE_RAW;
  /* For singleplaner buffer pass a single dmabuf fd for both the outBufs
     ie: self->outBufs->descs[0].buf and self->outBufs->descs[1].buf
     should point to a single buffer fd and need to update the
     descs[0].bufSize.bytes with the size of luminance(Y) data
     and descs[1].bufSize.bytes with crominance(UV) */
  self->outBufs->descs[1].buf = (XDAS_Int8 *) self->outBufs->descs[0].buf;
  self->outBufs->descs[1].bufSize.bytes = priv->size - priv->uv_offset;

  return (XDAS_Int32) * buf;
}

static GstBuffer *
codec_get_outbuf (GstDucatiVidDec * self, XDAS_Int32 id)
{
  GstBuffer *buf = (GstBuffer *) id;

  if (buf) {
    g_hash_table_insert (self->passed_in_bufs, buf, buf);

    gst_buffer_ref (buf);
  }
  return buf;
}

static void
do_dce_buf_unlock (GstBuffer * buf)
{
  SizeT fd;
  /* Get dmabuf fd of the buffer to unlock */
  fd = gst_dma_buf_meta_get_fd (gst_buffer_get_dma_buf_meta (buf));

  dce_buf_unlock (1, &fd);
}

static void
dce_buf_free_all (GstBuffer * buf, gpointer key, GstDucatiVidDec * self)
{
  if (FALSE == g_hash_table_remove (self->passed_in_bufs, buf)) {
    /* Buffer was not found in the hash table, remove it anyway */
    gst_buffer_unref (buf);
  }
}

static void
codec_unlock_outbuf (GstDucatiVidDec * self, XDAS_Int32 id)
{
  GstBuffer *buf = (GstBuffer *) id;

  if (buf) {
    GST_DEBUG_OBJECT (self, "free buffer: %d %p", id, buf);
    /* Must unlock the buffer before free */
    g_hash_table_remove (self->dce_locked_bufs, buf);
    if (FALSE == g_hash_table_remove (self->passed_in_bufs, buf)) {
      /* Buffer was not found in the hash table, remove it anyway */
      gst_buffer_unref (buf);
    }
  }
}

/* Called when playing in reverse */
static GstFlowReturn
gst_ducati_viddec_push_latest (GstDucatiVidDec * self)
{
  GstBuffer *buf;

  if (self->backlog_nframes == 0)
    return GST_FLOW_OK;

  /* send it, giving away the ref */
  buf = self->backlog_frames[--self->backlog_nframes];
  GST_DEBUG_OBJECT (self, "Actually pushing backlog buffer %" GST_PTR_FORMAT,
      buf);
  return gst_pad_push (self->srcpad, buf);
}

static GstFlowReturn
gst_ducati_viddec_push_earliest (GstDucatiVidDec * self)
{
  guint64 earliest_order = G_MAXUINT64;
  guint earliest_index = 0, i;
  GstBuffer *buf;

  if (self->backlog_nframes == 0)
    return GST_FLOW_OK;

  /* work out which frame has the earliest poc */
  for (i = 0; i < self->backlog_nframes; i++) {
    guint64 order = GST_BUFFER_OFFSET_END (self->backlog_frames[i]);
    if (earliest_order == G_MAXUINT64 || order < earliest_order) {
      earliest_order = order;
      earliest_index = i;
    }
  }

  /* send it, giving away the ref */
  buf = self->backlog_frames[earliest_index];
  self->backlog_frames[earliest_index] =
      self->backlog_frames[--self->backlog_nframes];
  GST_DEBUG_OBJECT (self, "Actually pushing backlog buffer %" GST_PTR_FORMAT,
      buf);
  return gst_pad_push (self->srcpad, buf);
}

static void
gst_ducati_viddec_on_flush (GstDucatiVidDec * self, gboolean eos)
{
  if (self->segment.format == GST_FORMAT_TIME &&
      self->segment.rate < (gdouble) 0.0) {
    /* negative rate */
    /* push everything on the backlog, ignoring errors */
    while (self->backlog_nframes > 0) {
      gst_ducati_viddec_push_latest (self);
    }
  } else {
    /* push everything on the backlog, ignoring errors */
    while (self->backlog_nframes > 0) {
      gst_ducati_viddec_push_earliest (self);
    }
  }
}

static gint
codec_process (GstDucatiVidDec * self, gboolean send, gboolean flush,
    GstFlowReturn * flow_ret)
{
  gint err, getstatus_err;
  GstClockTime t;
  GstBuffer *outbuf = NULL;
  gint i;
  SizeT fd;
  GstDucatiVidDecClass *klass = GST_DUCATIVIDDEC_GET_CLASS (self);
  GstFlowReturn ret = GST_FLOW_OK;
  if (flow_ret)
    /* never leave flow_ret uninitialized */
    *flow_ret = GST_FLOW_OK;

  memset (&self->outArgs->outputID, 0, sizeof (self->outArgs->outputID));
  memset (&self->outArgs->freeBufID, 0, sizeof (self->outArgs->freeBufID));

  if (self->inArgs->inputID != 0) {
    /* Check if this inputID was already sent to the codec */
    if (g_hash_table_contains (self->dce_locked_bufs,
            (gpointer) self->inArgs->inputID)) {
      GstMetaDucatiBufferPriv *priv =
          gst_ducati_buffer_priv_get ((GstBuffer *) self->inArgs->inputID);

      GST_DEBUG_OBJECT (self, "Resending (inputID: %08x)",
          self->inArgs->inputID);
      /* Input ID needs to be resent to the codec for cases like H.264 field coded pictures.
         The decoder indicates this by setting outArgs->outBufsInUseFlag */
      self->outBufs->descs[0].memType = XDM_MEMTYPE_RAW;
      self->outBufs->descs[0].buf = (XDAS_Int8 *)
          gst_dma_buf_meta_get_fd (gst_buffer_get_dma_buf_meta ((GstBuffer *)
              self->inArgs->inputID));
      self->outBufs->descs[0].bufSize.bytes = priv->uv_offset;
      self->outBufs->descs[1].memType = XDM_MEMTYPE_RAW;
      self->outBufs->descs[1].buf = (XDAS_Int8 *) self->outBufs->descs[0].buf;
      self->outBufs->descs[1].bufSize.bytes = priv->size - priv->uv_offset;
    } else {
      /* Get dmabuf fd of the buffer to lock it */
      fd = gst_dma_buf_meta_get_fd (gst_buffer_get_dma_buf_meta ((GstBuffer *)
              self->inArgs->inputID));
      /* Must lock all the buffer passed to ducati */
      GST_DEBUG_OBJECT (self, "dce_buf_lock(inputID: %08x, fd: %d)",
          self->inArgs->inputID, fd);
      dce_buf_lock (1, &fd);
      g_hash_table_insert (self->dce_locked_bufs,
          (gpointer) self->inArgs->inputID, (gpointer) self->inArgs->inputID);
    }
  }
  t = gst_util_get_timestamp ();
  err = VIDDEC3_process (self->codec,
      self->inBufs, self->outBufs, self->inArgs, self->outArgs);
  t = gst_util_get_timestamp () - t;
  GST_DEBUG_OBJECT (self, "VIDDEC3_process took %10dns (%d ms)", (gint) t,
      (gint) (t / 1000000));

  if (err) {
    GST_WARNING_OBJECT (self, "err=%d, extendedError=%08x",
        err, self->outArgs->extendedError);
    gst_ducati_log_extended_error_info (self->outArgs->extendedError,
        self->error_strings);
  }

  if (err || self->first_in_buffer) {
    GST_DEBUG ("Calling VIDDEC3_control XDM_GETSTATUS");
    getstatus_err = VIDDEC3_control (self->codec, XDM_GETSTATUS,
        self->dynParams, self->status);
    if (getstatus_err) {
      GST_WARNING_OBJECT (self, "XDM_GETSTATUS: err=%d, extendedError=%08x",
          getstatus_err, self->status->extendedError);
      gst_ducati_log_extended_error_info (self->status->extendedError,
          self->error_strings);
    }

    if (!getstatus_err && self->first_in_buffer) {
      if (send && self->status->maxNumDisplayBufs != 0) {
        GstCaps *caps;
        GST_WARNING_OBJECT (self, "changing max-ref-frames in caps to %d",
            self->status->maxNumDisplayBufs);

        caps = gst_caps_make_writable (gst_pad_get_current_caps (self->srcpad));

        gst_caps_set_simple (caps, "max-ref-frames", G_TYPE_INT,
            self->status->maxNumDisplayBufs, NULL);
        if (self->pool)
          gst_drm_buffer_pool_set_caps (self->pool, caps);
        if (!gst_pad_set_caps (self->srcpad, caps)) {
          GST_ERROR_OBJECT (self, "downstream didn't accept new caps");
          err = XDM_EFAIL;
        }
        gst_caps_unref (caps);
      }
    }
  }

  if (err) {
    if (flush)
      err = XDM_EFAIL;
    else
      err = klass->handle_error (self, err,
          self->outArgs->extendedError, self->status->extendedError);
  }

  /* we now let the codec decide */
  self->dynParams->newFrameFlag = XDAS_FALSE;

  if (err == XDM_EFAIL)
    goto skip_outbuf_processing;

  for (i = 0; i < IVIDEO2_MAX_IO_BUFFERS && self->outArgs->outputID[i]; i++) {
    gboolean interlaced;

    GST_DEBUG_OBJECT (self, "VIDDEC3_process outputID[%d]: %08x",
        i, self->outArgs->outputID[i]);
    interlaced =
        self->outArgs->displayBufs.bufDesc[0].contentType ==
        IVIDEO_PROGRESSIVE ? FALSE : TRUE;

    if (interlaced) {
      GstBuffer *buf = GST_BUFFER (self->outArgs->outputID[i]);
      if (!buf || !gst_buffer_is_writable (buf)) {
        GST_ERROR_OBJECT (self, "Cannot change buffer flags!!");
      } else {
        GST_BUFFER_FLAG_UNSET (buf, GST_VIDEO_BUFFER_FLAG_TFF);
        GST_BUFFER_FLAG_UNSET (buf, GST_VIDEO_BUFFER_FLAG_RFF);
        if (self->outArgs->displayBufs.bufDesc[0].topFieldFirstFlag)
          GST_BUFFER_FLAG_SET (buf, GST_VIDEO_BUFFER_FLAG_TFF);
        if (self->outArgs->displayBufs.bufDesc[0].repeatFirstFieldFlag)
          GST_BUFFER_FLAG_SET (buf, GST_VIDEO_BUFFER_FLAG_RFF);
      }
    }

	if(1)
	{
		Ofilm_Can_Data_T *pCan;
		GstMetaCanBuf *pCanMeta = NULL;

		pCan = (Ofilm_Can_Data_T *)g_queue_pop_tail(&self->can_queue);
		if(pCan != NULL)
		{
			GstBuffer *buf = (GstBuffer *)self->outArgs->outputID[i];
		  
			pCanMeta = gst_buffer_add_can_buf_meta(GST_BUFFER(buf), pCan, sizeof(Ofilm_Can_Data_T));
			if (!pCanMeta){
				GST_ERROR("Failed to add can meta to buffer");
			}
			g_free(pCan);
		}		
	}	

    /* Getting an extra reference for the decoder */
    outbuf = codec_get_outbuf (self, self->outArgs->outputID[i]);

    /* if send is FALSE, don't try to renegotiate as we could be flushing during
     * a PAUSED->READY state change
     */
    if (send && interlaced != self->interlaced) {
      GstCaps *caps;

      GST_WARNING_OBJECT (self, "upstream set interlaced=%d but codec "
          "thinks interlaced=%d... trusting codec", self->interlaced,
          interlaced);

      self->interlaced = interlaced;

      caps = gst_caps_make_writable (gst_pad_get_current_caps (self->srcpad));
      GST_INFO_OBJECT (self, "changing interlace field in caps");
      gst_caps_set_simple (caps, "interlaced", G_TYPE_BOOLEAN, interlaced,
          NULL);
      if (self->pool)
        gst_drm_buffer_pool_set_caps (self->pool, caps);
      if (!gst_pad_set_caps (self->srcpad, caps)) {
        GST_ERROR_OBJECT (self,
            "downstream didn't want to change interlace mode");
        err = XDM_EFAIL;
      }
      gst_caps_unref (caps);
    }

    if (send) {
      GstVideoCropMeta *crop = gst_buffer_get_video_crop_meta (outbuf);
      if (crop) {
        gint crop_width, crop_height;
        /* send region of interest to sink on first buffer: */
        XDM_Rect *r =
            &(self->outArgs->displayBufs.bufDesc[0].activeFrameRegion);

        crop_width = r->bottomRight.x - r->topLeft.x;
        crop_height = r->bottomRight.y - r->topLeft.y;

        if (crop_width > self->input_width)
          crop_width = self->input_width;
        if (crop_height > self->input_height)
          crop_height = self->input_height;

        GST_INFO_OBJECT (self, "active frame region %d, %d, %d, %d, crop %dx%d",
            r->topLeft.x, r->topLeft.y, r->bottomRight.x, r->bottomRight.y,
            crop_width, crop_height);

        crop->x = r->topLeft.x;
        crop->y = r->topLeft.y;
        crop->width = crop_width;
        crop->height = crop_height;
      } else {
        GST_INFO_OBJECT (self, "Crop metadata not present in buffer");
      }
    }

    if (G_UNLIKELY (self->first_out_buffer) && send) {
      GstDRMBufferPool *pool;
      self->first_out_buffer = FALSE;

      /* Destroy the pool so the buffers we used so far are eventually released.
       * The pool will be recreated if needed.
       */
      pool = self->pool;
      self->pool = NULL;
      if (pool)
        gst_drm_buffer_pool_destroy (pool);
    }

    if (send) {
      GstClockTime ts;

      ts = GST_BUFFER_PTS (outbuf);

      GST_DEBUG_OBJECT (self, "got buffer: %d %p (%" GST_TIME_FORMAT ")",
          i, outbuf, GST_TIME_ARGS (ts));

#ifdef USE_DTS_PTS_CODE
      if (self->ts_may_be_pts) {
        if ((self->last_pts != GST_CLOCK_TIME_NONE) && (self->last_pts > ts)) {
          GST_DEBUG_OBJECT (self, "detected PTS going backwards, "
              "enabling ts_is_pts");
          self->ts_is_pts = TRUE;
        }
      }
#endif

      self->last_pts = ts;

      if (self->dts_ridx != self->dts_widx) {
        ts = self->dts_queue[self->dts_ridx++ % NDTS];
      }

      if (self->ts_is_pts) {
        /* if we have a queued DTS from demuxer, use that instead: */
        GST_BUFFER_PTS (outbuf) = ts;
        GST_DEBUG_OBJECT (self, "fixed ts: %d %p (%" GST_TIME_FORMAT ")",
            i, outbuf, GST_TIME_ARGS (ts));
      }

      ret = klass->push_output (self, outbuf);
      if (flow_ret)
        *flow_ret = ret;
      if (ret != GST_FLOW_OK) {
        GST_WARNING_OBJECT (self, "push failed %s", gst_flow_get_name (ret));
        /* just unref the remaining buffers (if any) */
        send = FALSE;
      }
    } else {
      GST_DEBUG_OBJECT (self, "Buffer not pushed, dropping 'chain' ref: %d %p",
          i, outbuf);

      gst_buffer_unref (outbuf);
    }
  }

skip_outbuf_processing:
  for (i = 0; i < IVIDEO2_MAX_IO_BUFFERS && self->outArgs->freeBufID[i]; i++) {
    GST_DEBUG_OBJECT (self, "VIDDEC3_process freeBufID[%d]: %08x",
        i, self->outArgs->freeBufID[i]);
    codec_unlock_outbuf (self, self->outArgs->freeBufID[i]);
  }

  return err;
}

/** call control(FLUSH), and then process() to pop out all buffers */
gboolean
gst_ducati_viddec_codec_flush (GstDucatiVidDec * self, gboolean eos)
{
  gint err = FALSE;
  int prev_num_in_bufs, prev_num_out_bufs;

  GST_DEBUG_OBJECT (self, "flush: eos=%d", eos);

  GST_DUCATIVIDDEC_GET_CLASS (self)->on_flush (self, eos);

  /* note: flush is synchronized against _chain() to avoid calling
   * the codec from multiple threads
   */
  GST_PAD_STREAM_LOCK (self->sinkpad);

#ifdef USE_DTS_PTS_CODE
  self->dts_ridx = self->dts_widx = 0;
  self->last_dts = self->last_pts = GST_CLOCK_TIME_NONE;
  self->ts_may_be_pts = TRUE;
  self->ts_is_pts = FALSE;
#endif
  self->wait_keyframe = TRUE;
  self->in_size = 0;
  self->needs_flushing = FALSE;
  self->need_out_buf = TRUE;

  if (G_UNLIKELY (self->first_in_buffer)) {
    goto out;
  }

  if (G_UNLIKELY (!self->codec)) {
    GST_WARNING_OBJECT (self, "no codec");
    goto out;
  }


  GST_DEBUG ("Calling VIDDEC3_control XDM_FLUSH");
  err = VIDDEC3_control (self->codec, XDM_FLUSH, self->dynParams, self->status);
  if (err) {
    GST_ERROR_OBJECT (self, "failed XDM_FLUSH");
    goto out;
  }

  prev_num_in_bufs = self->inBufs->numBufs;
  prev_num_out_bufs = self->outBufs->numBufs;

  self->inBufs->descs[0].bufSize.bytes = 0;
  self->inBufs->numBufs = 0;
  self->inArgs->numBytes = 0;
  self->inArgs->inputID = 0;
  self->outBufs->numBufs = 0;

  do {
    err = codec_process (self, eos, TRUE, NULL);
  } while (err != XDM_EFAIL);

  /* We flushed the decoder, we can now remove the buffer that have never been
   * unrefed in it */
  g_hash_table_foreach (self->dce_locked_bufs, (GHFunc) dce_buf_free_all, self);
  g_hash_table_remove_all (self->dce_locked_bufs);
  g_hash_table_remove_all (self->passed_in_bufs);

  /* reset outArgs in case we're flushing in codec_process trying to do error
   * recovery */
  memset (&self->outArgs->outputID, 0, sizeof (self->outArgs->outputID));
  memset (&self->outArgs->freeBufID, 0, sizeof (self->outArgs->freeBufID));

  self->dynParams->newFrameFlag = XDAS_TRUE;

  /* Reset the push buffer and YUV buffers, plus any codec specific buffers */
  self->inBufs->numBufs = prev_num_in_bufs;
  self->outBufs->numBufs = prev_num_out_bufs;

  /* on a flush, it is normal (and not an error) for the last _process() call
   * to return an error..
   */
  err = XDM_EOK;

out:
  GST_PAD_STREAM_UNLOCK (self->sinkpad);
  GST_DEBUG_OBJECT (self, "done");

  return !err;
}

/* GstDucatiVidDec vmethod default implementations */

static gboolean
gst_ducati_viddec_parse_caps (GstDucatiVidDec * self, GstStructure * s)
{
  const GValue *codec_data;
  gint w, h;

  if (gst_structure_get_int (s, "width", &self->input_width) &&
      gst_structure_get_int (s, "height", &self->input_height)) {

    h = ALIGN2 (self->input_height, 4); /* round up to MB */
    w = ALIGN2 (self->input_width, 4);  /* round up to MB */

    /* if we've already created codec, but the resolution has changed, we
     * need to re-create the codec:
     */
    if (G_UNLIKELY ((self->codec) && ((h != self->height) || (w != self->width)
                || self->codec_create_params_changed))) {
      GST_DEBUG_OBJECT (self, "%dx%d => %dx%d, %d", self->width,
          self->height, w, h, self->codec_create_params_changed);
      codec_delete (self);
    }

    self->codec_create_params_changed = FALSE;
    self->width = w;
    self->height = h;

    codec_data = gst_structure_get_value (s, "codec_data");

    if (codec_data) {
      int i;
      GstMapInfo info;
      gboolean mapped;
      GstBuffer *buffer = gst_value_get_buffer (codec_data);

      GST_DEBUG_OBJECT (self, "codec_data: %" GST_PTR_FORMAT, buffer);

      mapped = gst_buffer_map (buffer, &info, GST_MAP_READ);
      GST_DEBUG_OBJECT (self, "codec_data dump, size = %d ", info.size);
      for (i = 0; i < info.size; i++) {
        GST_DEBUG_OBJECT (self, "%02x ", info.data[i]);
      }
      if (info.size) {
        self->codecdata = g_slice_alloc (info.size);
        if (self->codecdata) {
          memcpy (self->codecdata, info.data, info.size);
        } else {
          GST_DEBUG_OBJECT (self, "g_slice_alloc failed");
        }
        self->codecdatasize = info.size;
      }
      if (mapped) {
        gst_buffer_unmap (buffer, &info);
      }
    }

    return TRUE;
  }

  return FALSE;
}

static gboolean
gst_ducati_viddec_allocate_params (GstDucatiVidDec * self, gint params_sz,
    gint dynparams_sz, gint status_sz, gint inargs_sz, gint outargs_sz)
{

  /* allocate params: */
  self->params = dce_alloc (params_sz);
  if (G_UNLIKELY (!self->params)) {
    return FALSE;
  }
  self->params->size = params_sz;
  self->params->maxFrameRate = 30000;
  self->params->maxBitRate = 10000000;

  self->params->dataEndianness = XDM_BYTE;
  self->params->forceChromaFormat = XDM_YUV_420SP;
  self->params->operatingMode = IVIDEO_DECODE_ONLY;

  self->params->displayBufsMode = IVIDDEC3_DISPLAYBUFS_EMBEDDED;
  self->params->inputDataMode = IVIDEO_ENTIREFRAME;
  self->params->outputDataMode = IVIDEO_ENTIREFRAME;
  self->params->numInputDataUnits = 0;
  self->params->numOutputDataUnits = 0;

  self->params->metadataType[0] = IVIDEO_METADATAPLANE_NONE;
  self->params->metadataType[1] = IVIDEO_METADATAPLANE_NONE;
  self->params->metadataType[2] = IVIDEO_METADATAPLANE_NONE;
  self->params->errorInfoMode = IVIDEO_ERRORINFO_OFF;

  /* allocate dynParams: */
  self->dynParams = dce_alloc (dynparams_sz);
  if (G_UNLIKELY (!self->dynParams)) {
    return FALSE;
  }
  self->dynParams->size = dynparams_sz;
  self->dynParams->decodeHeader = XDM_DECODE_AU;
  self->dynParams->displayWidth = 0;
  self->dynParams->frameSkipMode = IVIDEO_NO_SKIP;
  self->dynParams->newFrameFlag = XDAS_TRUE;

  /* allocate status: */
  self->status = dce_alloc (status_sz);
  if (G_UNLIKELY (!self->status)) {
    return FALSE;
  }
  memset (self->status, 0, status_sz);
  self->status->size = status_sz;

  /* allocate inBufs/outBufs: */
  self->inBufs = dce_alloc (sizeof (XDM2_BufDesc));
  self->outBufs = dce_alloc (sizeof (XDM2_BufDesc));
  if (G_UNLIKELY (!self->inBufs) || G_UNLIKELY (!self->outBufs)) {
    return FALSE;
  }

  /* allocate inArgs/outArgs: */
  self->inArgs = dce_alloc (inargs_sz);
  self->outArgs = dce_alloc (outargs_sz);
  if (G_UNLIKELY (!self->inArgs) || G_UNLIKELY (!self->outArgs)) {
    return FALSE;
  }
  self->inArgs->size = inargs_sz;
  self->outArgs->size = outargs_sz;

  return TRUE;
}

static GstBuffer *
gst_ducati_viddec_push_input (GstDucatiVidDec * self, GstBuffer * buf)
{
  GstMapInfo info;
  gboolean mapped;
  if (G_UNLIKELY (self->first_in_buffer) && self->codecdata) {
    push_input (self, self->codecdata, self->codecdatasize);
  }
  /* just copy entire buffer */

  mapped = gst_buffer_map (buf, &info, GST_MAP_READ);
  if (mapped) {
    push_input (self, info.data, info.size);
    gst_buffer_unmap (buf, &info);
  }
  gst_buffer_unref (buf);

  return NULL;
}

static GstFlowReturn
gst_ducati_viddec_push_output (GstDucatiVidDec * self, GstBuffer * buf)
{
  GstFlowReturn ret = GST_FLOW_OK;

  if (self->segment.format == GST_FORMAT_TIME &&
      self->segment.rate < (gdouble) 0.0) {
    /* negative rate: reverse playback */

    if (self->backlog_nframes > 0 &&
        (GST_BUFFER_PTS (self->backlog_frames[0]) > GST_BUFFER_PTS (buf))) {
      /* push out all backlog frames, since we have a buffer that is
         earlier than any other in the list */
      while (self->backlog_nframes > 0) {
        ret = gst_ducati_viddec_push_latest (self);
        if (ret != GST_FLOW_OK)
          break;
      }
    }
    /* add the frame to the list, the array will own the ref */
    GST_DEBUG_OBJECT (self, "Adding buffer %" GST_PTR_FORMAT " to backlog",
        buf);
    if (self->backlog_nframes < MAX_BACKLOG_ARRAY_SIZE) {
      self->backlog_frames[self->backlog_nframes++] = buf;
    } else {
      /* No space in the re-order buffer, drop the frame */
      GST_WARNING_OBJECT (self, "Dropping buffer %" GST_PTR_FORMAT, buf);
      gst_buffer_unref (buf);
    }

  } else {
    /* if no reordering info was set, just send the buffer */
    if (GST_BUFFER_OFFSET_END (buf) == GST_BUFFER_OFFSET_NONE) {
      GST_DEBUG_OBJECT (self, "No reordering info on that buffer, sending now");
      return gst_pad_push (self->srcpad, buf);
    }

    /* add the frame to the list, the array will own the ref */
    GST_DEBUG_OBJECT (self, "Adding buffer %" GST_PTR_FORMAT " to backlog",
        buf);
    self->backlog_frames[self->backlog_nframes++] = buf;

    /* push till we have no more than the max needed, or error */
    while (self->backlog_nframes > self->backlog_maxframes) {
      ret = gst_ducati_viddec_push_earliest (self);
      if (ret != GST_FLOW_OK)
        break;
    }
  }
  return ret;
}

static gint
gst_ducati_viddec_handle_error (GstDucatiVidDec * self, gint ret,
    gint extended_error, gint status_extended_error)
{
  if (XDM_ISFATALERROR (extended_error))
    ret = XDM_EFAIL;
  else
    ret = XDM_EOK;

  return ret;
}

/* GstElement vmethod implementations */

static gboolean
gst_ducati_viddec_set_sink_caps (GstDucatiVidDec * self, GstCaps * caps)
{
  gboolean ret = TRUE;
  GstDucatiVidDecClass *klass = GST_DUCATIVIDDEC_GET_CLASS (self);
  GstStructure *s;
  GstCaps *outcaps = NULL;
  GstStructure *out_s;
  gint par_width, par_height;
  gboolean par_present;

  GST_INFO_OBJECT (self, "set_caps (sink): %" GST_PTR_FORMAT, caps);

  s = gst_caps_get_structure (caps, 0);
  if (!klass->parse_caps (self, s)) {
    GST_WARNING_OBJECT (self, "missing required fields");
    ret = FALSE;
    goto out;
  }

  /* update output/padded sizes */
  klass->update_buffer_size (self);

  if (!gst_structure_get_fraction (s, "framerate", &self->fps_n, &self->fps_d)) {
    self->fps_n = 0;
    self->fps_d = 1;
  }
  gst_structure_get_boolean (s, "interlaced", &self->interlaced);
  par_present = gst_structure_get_fraction (s, "pixel-aspect-ratio",
      &par_width, &par_height);

  outcaps = gst_pad_get_allowed_caps (self->srcpad);
  GST_DEBUG_OBJECT (self, "%s",
      gst_caps_to_string (gst_pad_get_current_caps (self->srcpad)));
  if (outcaps) {
    outcaps = gst_caps_make_writable (outcaps);
    outcaps = gst_caps_truncate (outcaps);
    if (gst_caps_is_empty (outcaps)) {
      gst_caps_unref (outcaps);
      outcaps = NULL;
    }
  }

  if (!outcaps) {
    outcaps = gst_caps_new_simple ("video/x-raw",
        "format", G_TYPE_STRING, "NV12", NULL);
  }

  out_s = gst_caps_get_structure (outcaps, 0);
  gst_structure_set (out_s,
      "width", G_TYPE_INT, self->padded_width,
      "height", G_TYPE_INT, self->padded_height,
      "framerate", GST_TYPE_FRACTION, self->fps_n, self->fps_d, NULL);
  if (par_present)
    gst_structure_set (out_s, "pixel-aspect-ratio", GST_TYPE_FRACTION,
        par_width, par_height, NULL);

  if (self->interlaced)
    gst_structure_set (out_s, "interlaced", G_TYPE_BOOLEAN, TRUE, NULL);

  self->stride = GST_ROUND_UP_4 (self->padded_width);

  self->outsize =
      GST_ROUND_UP_4 (self->stride) * GST_ROUND_UP_2 (self->padded_height) * 3 /
      2;

  GST_INFO_OBJECT (self, "outsize %d stride %d outcaps: %" GST_PTR_FORMAT,
      self->outsize, self->stride, outcaps);

  if (!self->first_in_buffer) {
    /* Caps changed mid stream. We flush the codec to unlock all the potentially
     * locked buffers. This is needed for downstream sinks that provide a
     * buffer pool and need to destroy all the outstanding buffers before they
     * can negotiate new caps (hello v4l2sink).
     */
    gst_ducati_viddec_codec_flush (self, FALSE);
  }


  ret = gst_pad_set_caps (self->srcpad, outcaps);

  GST_INFO_OBJECT (self, "set caps done %d, %" GST_PTR_FORMAT, ret, outcaps);

  /* default to no reordering */
  self->backlog_maxframes = 0;

  if (ret == TRUE) {
    if (self->sinkcaps)
      gst_caps_unref (self->sinkcaps);
    self->sinkcaps = gst_caps_copy (caps);
  }

out:
  if (outcaps)
    gst_caps_unref (outcaps);

  return ret;
}

static gboolean
gst_ducati_viddec_sink_setcaps (GstPad * pad, GstCaps * caps)
{
  gboolean ret = TRUE;
  GstDucatiVidDec *self = GST_DUCATIVIDDEC (gst_pad_get_parent (pad));
  GstDucatiVidDecClass *klass = GST_DUCATIVIDDEC_GET_CLASS (self);

  GST_INFO_OBJECT (self, "setcaps (sink): %" GST_PTR_FORMAT, caps);

  if (!self->sinkcaps || !gst_caps_is_strictly_equal (self->sinkcaps, caps))
    ret = klass->set_sink_caps (self, caps);
  else
    ret = TRUE;

  gst_object_unref (self);

  return ret;
}

static GstCaps *
gst_ducati_viddec_src_getcaps (GstPad * pad)
{
  GstCaps *caps = NULL;

  caps = gst_pad_get_current_caps (pad);
  if (caps == NULL) {
    GstCaps *fil = gst_pad_get_pad_template_caps (pad);
    GST_DEBUG ("filter caps = %s \n", gst_caps_to_string (fil));
    return gst_caps_copy (fil);
  } else {
    return gst_caps_copy (caps);
  }
}

static gboolean
gst_ducati_viddec_query (GstDucatiVidDec * self, GstPad * pad,
    GstQuery * query, gboolean * forward)
{
  gboolean res = TRUE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CAPS:
    {
      GstCaps *filter;
      filter = gst_ducati_viddec_src_getcaps (pad);
      gst_query_parse_caps (query, &filter);
      break;
    }
    case GST_QUERY_LATENCY:
    {
      gboolean live;
      GstClockTime min, max, latency;

      if (self->fps_d == 0) {
        GST_INFO_OBJECT (self, "not ready to report latency");
        res = FALSE;
        break;
      }

      gst_query_parse_latency (query, &live, &min, &max);
      if (self->fps_n != 0)
        latency = gst_util_uint64_scale (GST_SECOND, self->fps_d, self->fps_n);
      else
        latency = 0;

      /* Take into account the backlog frames for reordering */
      latency *= (self->backlog_maxframes + 1);

      if (min == GST_CLOCK_TIME_NONE)
        min = latency;
      else
        min += latency;

      if (max != GST_CLOCK_TIME_NONE)
        max += latency;

      GST_INFO_OBJECT (self,
          "latency %" GST_TIME_FORMAT " ours %" GST_TIME_FORMAT,
          GST_TIME_ARGS (min), GST_TIME_ARGS (latency));
      gst_query_set_latency (query, live, min, max);
      break;
    }
    default:
      break;
  }


  return res;
}

static gboolean
gst_ducati_viddec_src_query (GstPad * pad, GstObject * parent, GstQuery * query)
{
  gboolean res = TRUE, forward = TRUE;
  GstDucatiVidDec *self = GST_DUCATIVIDDEC (parent);
  GstDucatiVidDecClass *klass = GST_DUCATIVIDDEC_GET_CLASS (self);

  GST_DEBUG_OBJECT (self, "query: %" GST_PTR_FORMAT, query);
  res = klass->query (self, pad, query, &forward);
  if (res && forward)
    res = gst_pad_query_default (pad, parent, query);

  return res;
}

static gboolean
gst_ducati_viddec_do_qos (GstDucatiVidDec * self, GstBuffer * buf)
{
  GstClockTime timestamp, qostime;
  GstDucatiVidDecClass *klass = GST_DUCATIVIDDEC_GET_CLASS (self);
  gint64 diff;

  if (self->wait_keyframe) {
    if (GST_BUFFER_FLAG_IS_SET (buf, GST_BUFFER_FLAG_DELTA_UNIT)) {
      GST_INFO_OBJECT (self, "skipping until the next keyframe");
      return FALSE;
    }

    self->wait_keyframe = FALSE;
  }

  timestamp = GST_BUFFER_PTS (buf);
  if (self->segment.format != GST_FORMAT_TIME ||
      self->qos_earliest_time == GST_CLOCK_TIME_NONE)
    goto no_qos;

  if (G_UNLIKELY (!GST_CLOCK_TIME_IS_VALID (timestamp)))
    goto no_qos;

  qostime = gst_segment_to_running_time (&self->segment,
      GST_FORMAT_TIME, timestamp);
  if (G_UNLIKELY (!GST_CLOCK_TIME_IS_VALID (qostime)))
    /* out of segment */
    goto no_qos;

  /* see how our next timestamp relates to the latest qos timestamp. negative
   * values mean we are early, positive values mean we are too late. */
  diff = GST_CLOCK_DIFF (qostime, self->qos_earliest_time);

  GST_DEBUG_OBJECT (self, "QOS: qostime %" GST_TIME_FORMAT
      ", earliest %" GST_TIME_FORMAT " diff %" G_GINT64_FORMAT " proportion %f",
      GST_TIME_ARGS (qostime), GST_TIME_ARGS (self->qos_earliest_time), diff,
      self->qos_proportion);

  if (klass->can_drop_frame (self, buf, diff)) {
    GST_INFO_OBJECT (self, "dropping frame");
    return FALSE;
  }

no_qos:
  return TRUE;
}

static gboolean
gst_ducati_viddec_can_drop_frame (GstDucatiVidDec * self, GstBuffer * buf,
    gint64 diff)
{
  gboolean is_keyframe = !GST_BUFFER_FLAG_IS_SET (buf,
      GST_BUFFER_FLAG_DELTA_UNIT);

  if (diff >= 0 && !is_keyframe)
    return TRUE;

  return FALSE;
}

static GstFlowReturn
gst_ducati_viddec_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  SizeT fd;

  GstDucatiVidDec *self = GST_DUCATIVIDDEC (parent);
  GstClockTime ts = GST_BUFFER_PTS (buf);
  GstFlowReturn ret = GST_FLOW_OK;
  Int32 err;
  GstBuffer *outbuf = NULL;
  GstCaps *outcaps = NULL;
  gboolean decode;

  GstQuery *query = NULL;
  guint min = 0;
  guint max = 0;
  guint size = 0;
  
  Ofilm_Can_Data_T *pCan = NULL;
  GstMetaCanBuf *canbuf = NULL;

  canbuf = gst_buffer_get_can_buf_meta(buf);
  if(canbuf != NULL)
  {
	  pCan = (Ofilm_Can_Data_T *)g_malloc(sizeof(Ofilm_Can_Data_T));
	  memcpy(pCan, gst_can_buf_meta_get_can_data(canbuf), sizeof(Ofilm_Can_Data_T));
	  g_queue_push_head(&self->can_queue, pCan);
  }

normal:
  if (G_UNLIKELY (!self->engine)) {
    GST_ERROR_OBJECT (self, "no engine");
    gst_buffer_unref (buf);
    return GST_FLOW_ERROR;
  }

  GST_DEBUG_OBJECT (self, "chain: %" GST_TIME_FORMAT " (%d bytes, flags %d)",
      GST_TIME_ARGS (ts), gst_buffer_get_size (buf), GST_BUFFER_FLAGS (buf));

  decode = gst_ducati_viddec_do_qos (self, buf);
  if (!decode) {
    gst_buffer_unref (buf);
    return GST_FLOW_OK;
  }

  if (!self->need_out_buf)
    goto have_out_buf;

  /* do this before creating codec to ensure reverse caps negotiation
   * happens first:
   */
allocate_buffer:
  /* For plugins like VPE that allocate buffers to peers */
  if (!self->queried_external_pool) {
    query =
        gst_query_new_allocation (gst_pad_get_current_caps (self->srcpad),
        TRUE);
    if (gst_pad_peer_query (self->srcpad, query)) {
      gst_query_parse_nth_allocation_pool (query, 0, &self->externalpool, &size,
          &min, &max);
    }
    gst_query_unref (query);
    self->queried_external_pool = TRUE;
  }

  if (self->externalpool) {
    GstFlowReturn ret_acq_buf =
        gst_buffer_pool_acquire_buffer (GST_BUFFER_POOL (self->externalpool),
        &outbuf,
        NULL);
    if (ret_acq_buf == GST_FLOW_OK) {
      GstMetaDmaBuf *meta = gst_buffer_get_dma_buf_meta (outbuf);
      if (meta) {
        goto common;
      } else {
        gst_buffer_unref (outbuf);
      }
    }
    GST_WARNING_OBJECT (self, "acquire buffer from externalpool failed %s",
        gst_flow_get_name (ret_acq_buf));
  }

aqcuire_from_own_pool:
  if (self->externalpool) {
    gst_object_unref (self->externalpool);
    self->externalpool = NULL;
  }
  outbuf = codec_buffer_pool_get (self, NULL);

common:
  if (outbuf == NULL) {
    GST_WARNING_OBJECT (self, "alloc_buffer failed");
    gst_buffer_unref (buf);
    return GST_FLOW_ERROR;
  }

  if (G_UNLIKELY (!self->codec)) {
    if (!codec_create (self)) {
      GST_ERROR_OBJECT (self, "could not create codec");
      gst_buffer_unref (buf);
      gst_buffer_unref (outbuf);
      return GST_FLOW_ERROR;
    }
  }

  GST_BUFFER_PTS (outbuf) = GST_BUFFER_PTS (buf);
  GST_BUFFER_DURATION (outbuf) = GST_BUFFER_DURATION (buf);

  /* Pass new output buffer to the decoder to decode into. Use buffers from the
   * internal pool while self->first_out_buffer == TRUE in order to simplify
   * things in case we need to renegotiate */
  self->inArgs->inputID =
      codec_prepare_outbuf (self, &outbuf, self->first_out_buffer);
  if (!self->inArgs->inputID) {
    GST_ERROR_OBJECT (self, "could not prepare output buffer");
    gst_buffer_unref (buf);
    return GST_FLOW_ERROR;
  }
  GST_BUFFER_OFFSET_END (outbuf) = GST_BUFFER_OFFSET_END (buf);

have_out_buf:
  buf = GST_DUCATIVIDDEC_GET_CLASS (self)->push_input (self, buf);

#ifdef USE_DTS_PTS_CODE
  if (ts != GST_CLOCK_TIME_NONE) {
    self->dts_queue[self->dts_widx++ % NDTS] = ts;
    /* if next buffer has earlier ts than previous, then the ts
     * we are getting are definitely decode order (DTS):
     */
    if ((self->last_dts != GST_CLOCK_TIME_NONE) && (self->last_dts > ts)) {
      GST_DEBUG_OBJECT (self, "input timestamp definitely DTS");
      self->ts_may_be_pts = FALSE;
    }
    self->last_dts = ts;
  }
#endif

  if (self->in_size == 0 && outbuf) {
    GST_DEBUG_OBJECT (self, "no input, skipping process");

    gst_buffer_unref (outbuf);
    return GST_FLOW_OK;
  }

  self->inArgs->numBytes = self->in_size;
  self->inBufs->descs[0].bufSize.bytes = self->in_size;
  /* XDM_MemoryType required by drm to allcoate buffer */
  self->inBufs->descs[0].memType = XDM_MEMTYPE_RAW;

  err = codec_process (self, TRUE, FALSE, &ret);
  if (err) {
    GST_ELEMENT_ERROR (self, STREAM, DECODE, (NULL),
        ("process returned error: %d %08x", err, self->outArgs->extendedError));
    gst_ducati_log_extended_error_info (self->outArgs->extendedError,
        self->error_strings);

    return GST_FLOW_ERROR;
  }

  if (ret != GST_FLOW_OK) {
    GST_WARNING_OBJECT (self, "push from codec_process failed %s",
        gst_flow_get_name (ret));

    return ret;
  }

  self->first_in_buffer = FALSE;

  if (self->params->inputDataMode != IVIDEO_ENTIREFRAME) {
    /* The copy could be avoided by playing with the buffer pointer,
       but it seems to be rare and for not many bytes */
    GST_DEBUG_OBJECT (self, "Consumed %d/%d (%d) bytes, %d left",
        self->outArgs->bytesConsumed, self->in_size,
        self->inArgs->numBytes, self->in_size - self->outArgs->bytesConsumed);
    if (self->outArgs->bytesConsumed > 0) {
      if (self->outArgs->bytesConsumed > self->in_size) {
        GST_WARNING_OBJECT (self,
            "Codec claims to have used more bytes than supplied");
        self->in_size = 0;
      } else {
        if (self->outArgs->bytesConsumed < self->in_size) {
          memmove (self->input, self->input + self->outArgs->bytesConsumed,
              self->in_size - self->outArgs->bytesConsumed);
        }
        self->in_size -= self->outArgs->bytesConsumed;
      }
    }
  } else {
    self->in_size = 0;
  }

  if (self->outArgs->outBufsInUseFlag) {
    GST_DEBUG_OBJECT (self, "outBufsInUseFlag set");
    self->need_out_buf = FALSE;
  } else {
    self->need_out_buf = TRUE;
  }

  if (buf) {
    GST_DEBUG_OBJECT (self, "found remaining data: %d bytes",
        gst_buffer_get_size (buf));
    ts = GST_BUFFER_PTS (buf);
    goto allocate_buffer;
  }

  if (self->needs_flushing)
    gst_ducati_viddec_codec_flush (self, FALSE);

  return GST_FLOW_OK;
}

static gboolean
gst_ducati_viddec_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstDucatiVidDec *self = GST_DUCATIVIDDEC (parent);
  gboolean ret = TRUE;

  GST_DEBUG_OBJECT (self, "begin: event=%s", GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;
      gst_event_parse_caps (event, &caps);
      return gst_ducati_viddec_sink_setcaps (pad, caps);
      break;
    }
    case GST_EVENT_SEGMENT:
    {

      gst_event_copy_segment (event, &self->segment);

      break;
    }
    case GST_EVENT_EOS:
      if (!gst_ducati_viddec_codec_flush (self, TRUE)) {
        GST_ERROR_OBJECT (self, "could not flush on eos");
        ret = FALSE;
      }
      break;
    case GST_EVENT_FLUSH_STOP:
      if (!gst_ducati_viddec_codec_flush (self, FALSE)) {
        GST_ERROR_OBJECT (self, "could not flush");
        gst_event_unref (event);
        ret = FALSE;
      }
      gst_segment_init (&self->segment, GST_FORMAT_UNDEFINED);
      self->qos_earliest_time = GST_CLOCK_TIME_NONE;
      self->qos_proportion = 1;
      self->need_out_buf = TRUE;
      break;
    default:
      break;
  }

  if (ret)
    ret = gst_pad_push_event (self->srcpad, event);
  GST_LOG_OBJECT (self, "end");

  return ret;
}

static gboolean
gst_ducati_viddec_src_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstDucatiVidDec *self = GST_DUCATIVIDDEC (parent);
  gboolean ret = TRUE;

  GST_LOG_OBJECT (self, "begin: event=%s", GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_QOS:
    {
      gdouble proportion;
      GstClockTimeDiff diff;
      GstClockTime timestamp;
      GstQOSType type;

      gst_event_parse_qos (event, &type, &proportion, &diff, &timestamp);

      GST_OBJECT_LOCK (self);
      self->qos_proportion = proportion;
      self->qos_earliest_time = timestamp + 2 * diff;
      GST_OBJECT_UNLOCK (self);

      GST_DEBUG_OBJECT (self,
          "got QoS proportion %f %" GST_TIME_FORMAT ", %" G_GINT64_FORMAT,
          proportion, GST_TIME_ARGS (timestamp), diff);

      ret = gst_pad_push_event (self->sinkpad, event);
      break;
    }
    default:
      ret = gst_pad_push_event (self->sinkpad, event);
      break;
  }

  GST_LOG_OBJECT (self, "end");

  return ret;
}

static GstStateChangeReturn
gst_ducati_viddec_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstDucatiVidDec *self = GST_DUCATIVIDDEC (element);
  gboolean supported;

  GST_DEBUG_OBJECT (self, "begin: changing state %s -> %s",
      gst_element_state_get_name (GST_STATE_TRANSITION_CURRENT (transition)),
      gst_element_state_get_name (GST_STATE_TRANSITION_NEXT (transition)));

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      if (!engine_open (self)) {
        GST_ERROR_OBJECT (self, "could not open");
        return GST_STATE_CHANGE_FAILURE;
      }
      /* try to create/destroy the codec here, it may not be supported */
      supported = codec_create (self);
      codec_delete (self);
      self->codec = NULL;
      if (!supported) {
        GST_ERROR_OBJECT (element, "Failed to create codec %s, not supported",
            GST_DUCATIVIDDEC_GET_CLASS (self)->codec_name);
        engine_close (self);
        return GST_STATE_CHANGE_FAILURE;
      }
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  if (ret == GST_STATE_CHANGE_FAILURE)
    goto leave;

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      self->interlaced = FALSE;
      gst_ducati_viddec_codec_flush (self, FALSE);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      codec_delete (self);
      engine_close (self);
      break;
    default:
      break;
  }

leave:
  GST_LOG_OBJECT (self, "end");

  return ret;
}

/* GObject vmethod implementations */


static void
gst_ducati_viddec_get_property (GObject * obj,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstDucatiVidDec *self = GST_DUCATIVIDDEC (obj);


  switch (prop_id) {
    case PROP_VERSION:{
      int err;
      char *version = NULL;

      if (!self->engine)
        engine_open (self);

      if (!self->codec)
        codec_create (self);

      if (self->codec) {
        version = dce_alloc (VERSION_LENGTH);
        if (version) {
          self->status->data.buf = (XDAS_Int8 *) version;
          self->status->data.bufSize = VERSION_LENGTH;

          GST_DEBUG ("Calling VIDDEC3_control XDM_GETVERSION");
          err = VIDDEC3_control (self->codec, XDM_GETVERSION,
              self->dynParams, self->status);

          if (err) {
            GST_ERROR_OBJECT (self, "failed XDM_GETVERSION");
          } else
            GST_DEBUG ("Codec version %s", self->status->data.buf);

          self->status->data.buf = NULL;
          self->status->data.bufSize = 0;
          dce_free (version);
        }
      }
      break;
    }
    case PROP_MAX_REORDER_FRAMES:
      g_value_set_int (value, self->backlog_max_maxframes);
      break;
    case PROP_CODEC_DEBUG_INFO:
      g_value_set_boolean (value, self->codec_debug_info);
      break;
    default:{
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
      break;
    }
  }
}

static void
gst_ducati_viddec_set_property (GObject * obj,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstDucatiVidDec *self = GST_DUCATIVIDDEC (obj);

  switch (prop_id) {
    case PROP_MAX_REORDER_FRAMES:
      self->backlog_max_maxframes = g_value_get_int (value);
      break;
    case PROP_CODEC_DEBUG_INFO:
      self->codec_debug_info = g_value_get_boolean (value);
      break;
    default:{
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
      break;
    }
  }
}

static void
gst_ducati_viddec_finalize (GObject * obj)
{
  GstDucatiVidDec *self = GST_DUCATIVIDDEC (obj);

  codec_delete (self);

  if (self->sinkcaps)
    gst_caps_unref (self->sinkcaps);

  if (self->externalpool) {
    gst_object_unref (self->externalpool);
    self->externalpool = NULL;
  }

  engine_close (self);

  /* Will unref the remaining buffers if needed */
  g_hash_table_unref (self->dce_locked_bufs);
  g_hash_table_unref (self->passed_in_bufs);

  if (self->codecdata) {
    g_slice_free1 (self->codecdatasize, self->codecdata);
    self->codecdata = NULL;
  }
  
  g_queue_clear(&self->can_queue);

  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
gst_ducati_viddec_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_factory));
}

static void
gst_ducati_viddec_class_init (GstDucatiVidDecClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);
  parent_class = g_type_class_peek_parent (klass);

  gobject_class->get_property =
      GST_DEBUG_FUNCPTR (gst_ducati_viddec_get_property);
  gobject_class->set_property =
      GST_DEBUG_FUNCPTR (gst_ducati_viddec_set_property);
  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_ducati_viddec_finalize);
  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_ducati_viddec_change_state);

  klass->parse_caps = GST_DEBUG_FUNCPTR (gst_ducati_viddec_parse_caps);
  klass->allocate_params =
      GST_DEBUG_FUNCPTR (gst_ducati_viddec_allocate_params);
  klass->push_input = GST_DEBUG_FUNCPTR (gst_ducati_viddec_push_input);
  klass->handle_error = GST_DEBUG_FUNCPTR (gst_ducati_viddec_handle_error);
  klass->can_drop_frame = GST_DEBUG_FUNCPTR (gst_ducati_viddec_can_drop_frame);
  klass->query = GST_DEBUG_FUNCPTR (gst_ducati_viddec_query);
  klass->push_output = GST_DEBUG_FUNCPTR (gst_ducati_viddec_push_output);
  klass->on_flush = GST_DEBUG_FUNCPTR (gst_ducati_viddec_on_flush);
  klass->set_sink_caps = GST_DEBUG_FUNCPTR (gst_ducati_viddec_set_sink_caps);

  g_object_class_install_property (gobject_class, PROP_VERSION,
      g_param_spec_string ("version", "Version",
          "The codec version string", "",
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MAX_REORDER_FRAMES,
      g_param_spec_int ("max-reorder-frames",
          "Maximum number of frames needed for reordering",
          "The maximum number of frames needed for reordering output frames. "
          "Only meaningful for codecs with B frames. 0 means no reordering. "
          "This value will be used if the correct value cannot be inferred "
          "from the stream. Too low a value may cause misordering, too high "
          "will cause extra latency.",
          0, MAX_BACKLOG_FRAMES, MAX_BACKLOG_FRAMES,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CODEC_DEBUG_INFO,
      g_param_spec_boolean ("codec-debug-info",
          "Gather debug info from the codec",
          "Gather and log relevant debug information from the codec. "
          "What is gathered is typically codec specific", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gst_ducati_viddec_init (GstDucatiVidDec * self, gpointer klass)
{
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);

  gst_ducati_set_generic_error_strings (self->error_strings);

  self->sinkpad =
      gst_pad_new_from_template (gst_element_class_get_pad_template
      (gstelement_class, "sink"), "sink");
  gst_pad_set_chain_function (self->sinkpad, gst_ducati_viddec_chain);
  gst_pad_set_event_function (self->sinkpad, gst_ducati_viddec_event);

  self->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  gst_pad_set_event_function (self->srcpad, gst_ducati_viddec_src_event);
  gst_pad_set_query_function (self->srcpad, gst_ducati_viddec_src_query);

  gst_element_add_pad (GST_ELEMENT (self), self->sinkpad);
  gst_element_add_pad (GST_ELEMENT (self), self->srcpad);

  self->input_width = 0;
  self->input_height = 0;
  /* sane defaults in case we need to create codec without caps negotiation
   * (for example, to get 'version' property)
   */
  self->width = 128;
  self->height = 128;
  self->fps_n = -1;
  self->fps_d = -1;

  self->first_in_buffer = TRUE;
  self->first_out_buffer = FALSE;
  self->interlaced = FALSE;

#ifdef USE_DTS_PTS_CODE
  self->dts_ridx = self->dts_widx = 0;
  self->last_dts = self->last_pts = GST_CLOCK_TIME_NONE;
  self->ts_may_be_pts = TRUE;
  self->ts_is_pts = FALSE;
#endif

  self->codec_create_params_changed = FALSE;

  self->pageMemType = XDM_MEMTYPE_TILEDPAGE;

  self->queried_external_pool = FALSE;
  self->externalpool = NULL;

  self->codecdata = NULL;
  self->codecdatasize = 0;

  gst_segment_init (&self->segment, GST_FORMAT_UNDEFINED);

  self->qos_proportion = 1;
  self->qos_earliest_time = GST_CLOCK_TIME_NONE;
  self->wait_keyframe = TRUE;

  self->need_out_buf = TRUE;
  self->device = NULL;
  self->input_bo = NULL;

  self->sinkcaps = NULL;

  self->backlog_maxframes = 0;
  self->backlog_nframes = 0;
  self->backlog_max_maxframes = MAX_BACKLOG_FRAMES;

  self->codec_debug_info = FALSE;

  self->dce_locked_bufs = g_hash_table_new_full (g_direct_hash, g_direct_equal,
      NULL, (GDestroyNotify) do_dce_buf_unlock);
  self->passed_in_bufs = g_hash_table_new_full (g_direct_hash, g_direct_equal,
      NULL, (GDestroyNotify) gst_buffer_unref);

  g_queue_init(&self->can_queue);
}
