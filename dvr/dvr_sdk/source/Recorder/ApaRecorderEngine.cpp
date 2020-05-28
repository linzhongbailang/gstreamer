#include "DVR_RECORDER_DEF.h"
#include "ApaRecorderEngine.h"
#include "DvrSystemControl.h"
#include <gst/canbuf/canbuf.h>
#include "RecorderUtils.h"

GST_DEBUG_CATEGORY_STATIC(apa_recorder_engine_category);  // define category (statically)
#define GST_CAT_DEFAULT apa_recorder_engine_category       // set as default

CApaRecorderEngine::CApaRecorderEngine(RecorderAsyncOp *pHandle) : CLoopRecorderEngine(pHandle)
{

}

CApaRecorderEngine::~CApaRecorderEngine(void)
{
    
}

int CApaRecorderEngine::Open(void)
{
    GST_DEBUG_CATEGORY_INIT(apa_recorder_engine_category, "APA_RECORDER_ENGINE", GST_DEBUG_FG_GREEN, "apa recorder engine category");
    g_queue_init(&m_nFrameQueue);
    g_mutex_init(&m_mutexStateLock);
    g_mutex_init(&m_nFrameMutex);

    memset(&m_LoopRecSetting, 0, sizeof(LoopRecPrivateSetting));
    m_LoopRecSetting.video_dst_format       = g_strdup("APA_%Y%m%d_%H%M%S_M");
    m_LoopRecSetting.thumbnail_dst_format   = g_strdup("THM_%Y%m%d_%H%M%S_F");

    m_LoopRecSetting.m_ActivePeriod         = DVR_VIDEO_SPLIT_TIME_300_SECONDS; //5min
    m_LoopRecSetting.m_PendPeriod           = 0;
    m_LoopRecSetting.m_bPendPeriod          = FALSE;
    m_eLoopRecState                         = LOOP_RECORD_STATE_INVALID;

    return DVR_RES_SOK;
}

int	CApaRecorderEngine::Start(gboolean bLockState)
{
    GST_INFO("*************ApaRecStart***********\n");

    if (m_eLoopRecState == LOOP_RECORD_STATE_RUNNING)
    {
        GST_WARNING("start apa recorder failed, it has already started\n");
        return DVR_RES_EFAIL;
    }

    if (m_eLoopRecState == LOOP_RECORD_STATE_START_TO_STOP)
    {
        GST_WARNING("stop apa process is running, wait stop complete\n");
        return DVR_RES_EFAIL;
    }

    GST_INFO("start apa recorder\n");

    if (!LockState(bLockState, __LINE__))
    {
        GST_ERROR("[Engine][Run] LockState() FAILED!\n");
        return DVR_RES_EFAIL;
    }

    g_mutex_lock(&m_nFrameMutex);
    g_queue_foreach(&m_nFrameQueue, (GFunc)gst_buffer_unref, NULL);
    g_queue_clear(&m_nFrameQueue);
    g_mutex_unlock(&m_nFrameMutex);

    m_eLoopRecState = LOOP_RECORD_STATE_RUNNING;
	m_bSlowWriting = FALSE;

    gst_element_set_state(m_pPipeline, GST_STATE_PLAYING);

    UnlockState(bLockState);

    return DVR_RES_SOK;
}

int	CApaRecorderEngine::Stop(gboolean bLockState)
{
    int Ret = DVR_RES_SOK;

    GST_INFO("*************ApaRecStop***********\n");

    if (m_eLoopRecState == LOOP_RECORD_STATE_INVALID ||
        m_eLoopRecState == LOOP_RECORD_STATE_START_TO_STOP ||
        m_eLoopRecState == LOOP_RECORD_STATE_STOP_DONE)
    {
        GST_INFO("stop apa video recorder failed, it has already stopped\n");
        return DVR_RES_SOK;
    }

    GST_INFO("stop apa recorder\n");

    m_eLoopRecState = LOOP_RECORD_STATE_START_TO_STOP;

    if (!LockState(bLockState, __LINE__))
    {
        GST_ERROR("[Engine][Stop] LockState() FAILED!\n");
        return DVR_RES_EFAIL;
    }

    if (m_pPipeline == NULL)
    {
        UnlockState(bLockState);
        GST_ERROR("pipeline is NULL, maybe recorder is not start or has already stop!\n");
        return DVR_RES_EUNEXPECTED;
    }

    g_mutex_lock(&m_nFrameMutex);
    g_queue_foreach(&m_nFrameQueue, (GFunc)gst_buffer_unref, NULL);
    g_queue_clear(&m_nFrameQueue);
    g_mutex_unlock(&m_nFrameMutex);

    SendEOSAndWait();

    SetStateWait(GST_STATE_NULL);

    if(m_brebuilding == TRUE)
    {
        RebuildPipeline();
        m_brebuilding = FALSE;
    }

    UnlockState(bLockState);

    m_eLoopRecState = LOOP_RECORD_STATE_STOP_DONE;
	m_bSlowWriting = FALSE;

    if (m_ptfnCallBackArray != NULL && m_ptfnCallBackArray->MessageCallBack != NULL)
    {
        RECORDER_EVENT  eEvent = RECORDER_EVENT_STOP_DONE;
        m_ptfnCallBackArray->MessageCallBack((void*)&eEvent, NULL, NULL, m_ptfnCallBackArray->pCallBackInstance);
    }

    GST_INFO("stop apa recorder done\n");

    return Ret;
}

void CApaRecorderEngine::OnNeedApaData(GstElement *appsrc, guint unused_size, gpointer user_data)
{
    CApaRecorderEngine *pThis = (CApaRecorderEngine*)user_data;
    GstBuffer *buffer;
    GstFlowReturn ret;
    GstMetaCanBuf *pCanMeta = NULL;
   
    while (0 == g_queue_get_length(&pThis->m_nFrameQueue) && (pThis->m_eLoopRecState == LOOP_RECORD_STATE_RUNNING))
    {
        GST_LOG("not enough data in image queue or it is not start, waiting...\n");
        g_usleep(40 * MILLISEC_TO_SEC);
    }

    g_mutex_lock(&pThis->m_nFrameMutex);
    if (pThis->m_eLoopRecState == LOOP_RECORD_STATE_RUNNING)
    {
        GST_LOG("frames available in queue:%d", g_queue_get_length(&pThis->m_nFrameQueue));
        buffer = (GstBuffer *)g_queue_pop_tail(&pThis->m_nFrameQueue);
        if (buffer != NULL)
        {
            g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
            gst_buffer_unref(buffer);
        }
    }
    g_mutex_unlock(&pThis->m_nFrameMutex);
}

gchar *CApaRecorderEngine::OnUpdateApaRecDest(GstElement * element, guint fragment_id, gpointer data)
{
    CApaRecorderEngine *pThis = (CApaRecorderEngine*)data;
    LoopRecPrivateSetting *pSetting = (LoopRecPrivateSetting *)&pThis->m_LoopRecSetting;

    gchar *dst = create_video_filename(pSetting->video_dst_format, pSetting->loop_record_folder_max_file_index, pSetting->loop_record_location);
	memset(pThis->m_CurLoopRecVideoFileName, 0, sizeof(pThis->m_CurLoopRecVideoFileName));
	if(dst != NULL){
		strncpy(pThis->m_CurLoopRecVideoFileName, dst, APP_MAX_FN_SIZE);
	}

	gchar *thumb_name = create_thumbnail_filename(pSetting->thumbnail_dst_format, pSetting->loop_record_folder_max_file_index, pSetting->loop_record_location);
	memset(pThis->m_CurLoopRecThumbNailFileName, 0, sizeof(pThis->m_CurLoopRecThumbNailFileName));
	if(thumb_name != NULL){
		strncpy(pThis->m_CurLoopRecThumbNailFileName, thumb_name, APP_MAX_FN_SIZE);
	}

	if (pThis->m_pRecorderAsyncOp != NULL && thumb_name != NULL){
        pThis->m_pRecorderAsyncOp->RecorderAsyncOp_SndMsg(RECORDER_ASYNCOP_CMD_GENERATE_THUMBNAIL, (DVR_U32)thumb_name, 0);
    }

    if (pSetting->m_bPendPeriod)
    {
        g_object_set(G_OBJECT(pThis->m_pSink), "max-size-time", (guint64)((guint64)pSetting->m_PendPeriod *  NANOSEC_TO_SEC), NULL);
        g_object_set(G_OBJECT(pThis->m_pQtMuxer), "reserved-max-duration", (guint64)((guint64)(pSetting->m_PendPeriod + 3) * NANOSEC_TO_SEC), NULL);

        pSetting->m_ActivePeriod = pSetting->m_PendPeriod;
        pSetting->m_bPendPeriod = FALSE;
    }

    pSetting->loop_record_folder_max_file_index += 1;

    return dst;
}

gint CApaRecorderEngine::CreatePipeline(void)
{
    gboolean link_ok;
    GstBus* bus = NULL;
    GstCaps *prop_caps = NULL;
    GstPad *srcpad = NULL, *sinkpad = NULL;
    LoopRecPrivateSetting *pSetting = &m_LoopRecSetting;

    m_pPipeline = gst_pipeline_new("ApaRecord");
    if (!m_pPipeline)
    {
        GST_ERROR("Create ApaRecorder Pipeline Failed\n\n");
        goto exit;
    }

    m_pVideoAppSrc  = gst_element_factory_make("appsrc", "apa-source");
    m_pVideoParse   = gst_element_factory_make("h264parse", "apa-parser");
    m_pQtMuxer      = gst_element_factory_make("qtmux", "apa-muxer");
    m_pSink         = gst_element_factory_make("splitmuxsink", "apa-sink");
    if (!m_pVideoAppSrc || !m_pVideoParse || !m_pQtMuxer || !m_pSink)
    {
        GST_ERROR("[Engine]Create One of Plugins failed!\n");
        return DVR_RES_EFAIL;
    }

    prop_caps = gst_caps_new_simple("video/x-h264",
        "stream-format", G_TYPE_STRING, "byte-stream",
        "align", G_TYPE_STRING, "au",
        "framerate", GST_TYPE_FRACTION, 25, 1,
        "width", G_TYPE_INT, DVR_IMG_REAL_WIDTH,
        "height", G_TYPE_INT, DVR_IMG_REAL_HEIGHT,
        NULL);
    g_object_set(G_OBJECT(m_pVideoAppSrc), "caps", prop_caps, NULL);
    gst_caps_unref (prop_caps);

    g_object_set(G_OBJECT(m_pVideoAppSrc), "do-timestamp", TRUE, NULL);
    g_signal_connect(m_pVideoAppSrc, "need-data", G_CALLBACK(OnNeedApaData), this);

    g_object_set(G_OBJECT(m_pQtMuxer), "reserved-max-duration", (guint64)((guint64)(pSetting->m_ActivePeriod + 3) * NANOSEC_TO_SEC), NULL);
    g_object_set(G_OBJECT(m_pQtMuxer),"reserved-bytes-per-sec", (guint32)(RECORD_FPS*(30 + sizeof(Ofilm_Can_Data_T)) + RESERVED_PER_SEC_PER_TRAK_SIZE), NULL);  // per second per trak reserve bytes
    g_object_set(G_OBJECT(m_pQtMuxer), "reserved-moov-update-period", (guint64)RESERVED_MOOV_UPDATE_INTERVAL, NULL); // update moov per 1 second
    g_object_set(G_OBJECT(m_pQtMuxer), "reserved-extra-bytes", (guint32)RESERVED_MOOV_EXTRA_BYTES, NULL);

    g_object_set(G_OBJECT(m_pSink), "muxer", m_pQtMuxer, NULL);
	g_object_set(G_OBJECT(m_pSink), "use-robust-muxing", TRUE, NULL);	
    /* Never forget type conversion here, or it won't work */
    g_object_set(G_OBJECT(m_pSink), "max-size-time", (guint64)((guint64)pSetting->m_ActivePeriod * NANOSEC_TO_SEC), NULL);

    /* offer the new dest location */
    g_signal_connect(m_pSink, "format-location", G_CALLBACK(OnUpdateApaRecDest), this);

    /* add tag */
    g_signal_connect(m_pQtMuxer, "tag-add", G_CALLBACK(OnUpdateApaTagList), this);

    /* we add all elements into the pipeline */
    gst_bin_add_many(GST_BIN(m_pPipeline), m_pVideoAppSrc, m_pVideoParse, m_pSink, NULL);

    // link pipeline's element that has static pad
    link_ok = gst_element_link_many(m_pVideoAppSrc, m_pVideoParse, m_pSink, NULL);
    if (!link_ok) {
        goto exit;
    }

    bus = gst_pipeline_get_bus(GST_PIPELINE(m_pPipeline));
    gst_bus_set_sync_handler(bus, (GstBusSyncHandler)MessageHandler, this, NULL);
    gst_object_unref(bus);

    g_object_set(G_OBJECT(m_pPipeline), "message-forward", TRUE, NULL);

    return DVR_RES_SOK;

exit:
    if (m_pPipeline)
        gst_object_unref(GST_OBJECT(m_pPipeline));

    return DVR_RES_EFAIL;
}

int CApaRecorderEngine::SetFatalEnd(void)
{
    if (m_eLoopRecState == LOOP_RECORD_STATE_RUNNING)
    {
        if (m_ptfnCallBackArray != NULL && m_ptfnCallBackArray->MessageCallBack != NULL)
        {
            RECORDER_EVENT  eEvent = RECORDER_EVENT_DASREC_FATAL_ERROR;
            m_ptfnCallBackArray->MessageCallBack((void*)&eEvent, NULL, NULL, m_ptfnCallBackArray->pCallBackInstance);
        }
    }
    return DVR_RES_SOK;
}

gchar *CApaRecorderEngine::OnUpdateApaTagList(GstElement * element, guint fragment_id, gpointer data)
{
    CApaRecorderEngine *pThis = (CApaRecorderEngine*)data;

    GDateTime *date = g_date_time_new_now_local();
    gchar *description = g_date_time_format(date, "%Y-%m-%d-%H-%M-%S");
    g_date_time_unref (date);

    GST_OBJECT_LOCK(pThis->m_pQtMuxer);
    GstTagList *tag_list = gst_tag_list_new_empty();
    gst_tag_list_add(tag_list, GST_TAG_MERGE_APPEND, GST_TAG_DESCRIPTION, description, NULL);
    gst_tag_setter_merge_tags(GST_TAG_SETTER(pThis->m_pQtMuxer), tag_list, GST_TAG_MERGE_APPEND);
    GST_OBJECT_UNLOCK(pThis->m_pQtMuxer);

    gst_tag_list_unref(tag_list);
    g_free(description);

    if (pThis->m_pRecorderAsyncOp != NULL){
        pThis->m_pRecorderAsyncOp->RecorderAsyncOp_SndMsg(RECORDER_ASYNCOP_CMD_GENERATE_THUMBNAIL, (DVR_U32)pThis->m_CurLoopRecThumbNailFileName, 2);
    }

    //if (pThis->m_pRecorderAsyncOp != NULL){
    //    pThis->m_pRecorderAsyncOp->RecorderAsyncOp_SndMsg(RECORDER_ASYNCOP_CMD_ADD_TAG_LIST, (DVR_U32)pThis->m_CurLoopRecThumbNailFileName, 2);
    //}

    return NULL;
}

GstBusSyncReply CApaRecorderEngine::MessageHandler(GstBus *bus, GstMessage *msg, gpointer pData)
{
    if (NULL == pData)
    {
        return GST_BUS_PASS;
    }

    CApaRecorderEngine *pThis = (CApaRecorderEngine *)pData;
    
    GError *err;
    gchar *debug_info;
    const gchar *detailed_message;

    GST_LOG("Element: %s; Msg: %s\n", GST_OBJECT_NAME(msg->src), GST_MESSAGE_TYPE_NAME(msg));

    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_ELEMENT:
    {
        const GstStructure *st;
        const gchar *location;
        GstClockTime time;
        DVR_ADDTODB_FILE_INFO tFileInfo;

        st = gst_message_get_structure(msg);
        if (st)
        {
            if (gst_structure_has_name(st, "splitmuxsink-fragment-opened"))
            {
                location = gst_structure_get_string(st, "location");
                gst_structure_get_clock_time(st, "running-time", &time);

                tFileInfo.file_location         = location;
                tFileInfo.thumbnail_location    = pThis->m_CurLoopRecThumbNailFileName;
                tFileInfo.time_stamp            = time;
                if (pThis->m_ptfnCallBackArray != NULL && pThis->m_ptfnCallBackArray->MessageCallBack != NULL)
                {
                    RECORDER_EVENT  eEvent      = RECORDER_EVENT_NEW_FILE_START;
                    pThis->m_ptfnCallBackArray->MessageCallBack((void*)&eEvent, &tFileInfo, NULL, pThis->m_ptfnCallBackArray->pCallBackInstance);
                }

                pThis->m_bQtMuxStopFileDone = FALSE;
            }
            else if (gst_structure_has_name(st, "splitmuxsink-fragment-closed"))
            {
                location = gst_structure_get_string(st, "location");
                gst_structure_get_clock_time(st, "running-time", &time);
                tFileInfo.file_location         = location;
                tFileInfo.thumbnail_location    = pThis->m_CurLoopRecThumbNailFileName;
                tFileInfo.time_stamp            = time;
                tFileInfo.eType                 = DVR_FOLDER_TYPE_DAS;
                if (pThis->m_ptfnCallBackArray != NULL && pThis->m_ptfnCallBackArray->MessageCallBack != NULL)
                {
                    RECORDER_EVENT  eEvent      = RECORDER_EVENT_NEW_FILE_FINISHED;
                    pThis->m_ptfnCallBackArray->MessageCallBack((void*)&eEvent, &tFileInfo, NULL, pThis->m_ptfnCallBackArray->pCallBackInstance);
                }

                pThis->m_bQtMuxStopFileDone = TRUE;
            }
            else
            {

            }
        }
    }
    break;

    case GST_MESSAGE_INFO:
    {
        gst_message_parse_info(msg, &err, &debug_info);
        g_clear_error(&err);
        g_free(debug_info);
    }
    break;

    case GST_MESSAGE_ERROR:
    {
        gst_message_parse_error(msg, &err, &debug_info);
        g_free(debug_info);

        GST_ERROR("Error: %s\n", err->message);
        g_error_free(err);

		if(pThis->m_bFatalErrorNotifyHasFired == FALSE && pThis->m_eLoopRecState == LOOP_RECORD_STATE_RUNNING)
		{
            pThis->m_brebuilding = TRUE;
			pThis->m_bFatalError = TRUE;
			pThis->m_bFatalErrorNotifyHasFired = TRUE;
			if (pThis->m_ptfnCallBackArray != NULL && pThis->m_ptfnCallBackArray->MessageCallBack != NULL)
			{
				RECORDER_EVENT	eEvent = RECORDER_EVENT_DASREC_FATAL_ERROR;
				pThis->m_ptfnCallBackArray->MessageCallBack((void*)&eEvent, NULL, NULL, pThis->m_ptfnCallBackArray->pCallBackInstance);
			}
		}
    }
    break;

    case GST_MESSAGE_STREAM_STATUS:
    {
        GstStreamStatusType type;
        GstElement *pElement;
        gst_message_parse_stream_status(msg, &type, &pElement);
    }
    break;

    default:
    {
        detailed_message = GST_MESSAGE_TYPE_NAME(msg);
    }
    break;

    }

    /* We want to keep receiving messages */
    return GST_BUS_PASS;
}

