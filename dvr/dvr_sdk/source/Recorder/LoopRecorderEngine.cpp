#include "DVR_RECORDER_DEF.h"
#include "LoopRecorderEngine.h"
#include "DvrSystemControl.h"
#include <gst/canbuf/canbuf.h>
#include "RecorderUtils.h"

GST_DEBUG_CATEGORY_STATIC(loop_recorder_engine_category);  // define category (statically)
#define GST_CAT_DEFAULT loop_recorder_engine_category       // set as default

CLoopRecorderEngine::CLoopRecorderEngine(RecorderAsyncOp *pHandle)
{
    m_pPipeline             = NULL;
    m_pVideoAppSrc          = NULL;
    m_pVideoParse           = NULL;
    m_pQtMuxer              = NULL;
    m_pSink                 = NULL;
    m_ptfnCallBackArray     = NULL;
    m_pRecorderAsyncOp      = pHandle;
    m_bStateLocked          = FALSE;
    m_bQtMuxStopFileDone    = FALSE;
    m_bFatalError           = FALSE;
    m_bNeedRecover          = FALSE;
	m_bSlowWriting			= FALSE;
    m_brebuilding           = FALSE;
	m_bFatalErrorNotifyHasFired = FALSE;
}

CLoopRecorderEngine::~CLoopRecorderEngine(void)
{
    Close();
}

int CLoopRecorderEngine::Open(void)
{
    GST_DEBUG_CATEGORY_INIT(loop_recorder_engine_category, "LOOP_RECORDER_ENGINE", GST_DEBUG_FG_GREEN, "loop recorder engine category");
    g_queue_init(&m_nFrameQueue);
    g_mutex_init(&m_mutexStateLock);
    g_mutex_init(&m_nFrameMutex);
    memset(&m_LoopRecSetting, 0, sizeof(LoopRecPrivateSetting));
    m_LoopRecSetting.video_dst_format       = g_strdup("NOR_%Y%m%d_%H%M%S_M");
    m_LoopRecSetting.thumbnail_dst_format   = g_strdup("THM_%Y%m%d_%H%M%S_F");

    m_LoopRecSetting.m_ActivePeriod         = DVR_VIDEO_SPLIT_TIME_60_SECONDS; //60sec
    m_LoopRecSetting.m_PendPeriod           = 0;
    m_LoopRecSetting.m_bPendPeriod          = FALSE;
    m_eLoopRecState                         = LOOP_RECORD_STATE_INVALID;
    return DVR_RES_SOK;
}

int CLoopRecorderEngine::Close(void)
{
    if (m_LoopRecSetting.video_dst_format)
    {
        g_free(m_LoopRecSetting.video_dst_format);
        m_LoopRecSetting.video_dst_format = NULL;
    }

    if (m_LoopRecSetting.thumbnail_dst_format)
    {
        g_free(m_LoopRecSetting.thumbnail_dst_format);
        m_LoopRecSetting.thumbnail_dst_format = NULL;
    }

	if(m_LoopRecSetting.loop_record_location != NULL)
	{
		g_free(m_LoopRecSetting.loop_record_location);
		m_LoopRecSetting.loop_record_location = NULL;
	}

	g_mutex_clear(&m_nFrameMutex);
	g_mutex_clear(&m_mutexStateLock);
	g_queue_clear(&m_nFrameQueue);
    return DVR_RES_SOK;
}

gboolean CLoopRecorderEngine::LockState(gboolean bNeedLockState, unsigned long ulLockPos)
{
    if (!bNeedLockState)
        return TRUE;

    gint64 dwBeginTime = g_get_real_time();
    while (1)
    {
        g_mutex_lock(&m_mutexStateLock);
        if (m_bStateLocked)
        {
            g_mutex_unlock(&m_mutexStateLock);
            if (g_get_real_time() - dwBeginTime > MAX_OPERATION_MUTEX_WAIT_TIME)
            {
                GST_ERROR("[Engine]LockState failed, previous lock position: %u.\n", m_ulLockPos);
                return FALSE;
            }
            else
            {
                g_usleep(MUTEX_CHECK_WAIT_INTERVAL);
            }
        }
        else
            break;
    }
    m_bStateLocked = TRUE;
    m_ulLockPos = ulLockPos;
    g_mutex_unlock(&m_mutexStateLock);
    return TRUE;
}

void CLoopRecorderEngine::UnlockState(gboolean bNeedLockState)
{
    if (bNeedLockState)
        m_bStateLocked = FALSE;
}

int CLoopRecorderEngine::SetStateWait(GstState nextState)
{
    GstStateChangeReturn ret;

    if (!gst_is_initialized() || m_pPipeline == NULL)
    {
        GST_ERROR("[Engine][%d]Gstreamer is not initialized!\n", __LINE__);
        return DVR_RES_EFAIL;
    }

    ret = gst_element_set_state(m_pPipeline, nextState);
    if (ret == GST_STATE_CHANGE_ASYNC)
    {
        GstState state;
        GstState pending;
        GstClockTime timeout = 5 * GST_SECOND;
        ret = gst_element_get_state(m_pPipeline, &state, &pending, timeout);
        if (ret == GST_STATE_CHANGE_FAILURE || ret == GST_STATE_CHANGE_ASYNC)
            return DVR_RES_EFAIL;
    }
    else if (ret == GST_STATE_CHANGE_FAILURE)
    {
        return DVR_RES_EFAIL;
    }

    return DVR_RES_SOK;
}

int	CLoopRecorderEngine::Start(gboolean bLockState)
{
    GST_INFO("*************LoopRecStart***********\n");

    if (m_eLoopRecState == LOOP_RECORD_STATE_RUNNING)
    {
        GST_WARNING("start loop recorder failed, it has already started\n");
        return DVR_RES_EFAIL;
    }

    if (m_eLoopRecState == LOOP_RECORD_STATE_START_TO_STOP)
    {
        GST_WARNING("stop process is running, wait stop complete\n");
        return DVR_RES_EFAIL;
    }

    GST_INFO("start loop recorder\n");

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

gint CLoopRecorderEngine::SendEOSAndWait(void)
{
    gboolean it_done = FALSE;
    GValue it_value = G_VALUE_INIT;
    GstPad *sinkpad;
    GstIterator *it;

    /* end record file first */
    it = gst_element_iterate_sink_pads(m_pSink);
    while (!it_done)
    {
        switch (gst_iterator_next(it, &it_value))
        {
        case GST_ITERATOR_OK:
            sinkpad = (GstPad *)g_value_get_object(&it_value);
            gst_pad_send_event(sinkpad, gst_event_new_eos());
            gst_object_unref(GST_OBJECT(sinkpad));
            break;
        case GST_ITERATOR_RESYNC:
            gst_iterator_resync(it);
            break;
        case GST_ITERATOR_ERROR:
        case GST_ITERATOR_DONE:
            it_done = TRUE;
            break;
        }
    }
    gst_object_unref(m_pSink);
    //gst_iterator_free(it);

	if(m_bFatalError == TRUE)
	{
		DVR_ADDTODB_FILE_INFO tFileInfo;
		
		tFileInfo.file_location = m_CurLoopRecVideoFileName;
		tFileInfo.thumbnail_location = m_CurLoopRecThumbNailFileName;
		tFileInfo.time_stamp = 0;
		tFileInfo.eType = DVR_FOLDER_TYPE_NORMAL;
		if (m_ptfnCallBackArray != NULL && m_ptfnCallBackArray->MessageCallBack != NULL)
		{
			RECORDER_EVENT	eEvent = RECORDER_EVENT_NEW_FILE_FINISHED;
			m_ptfnCallBackArray->MessageCallBack((void*)&eEvent, &tFileInfo, NULL, m_ptfnCallBackArray->pCallBackInstance);
		}
	}
	else
	{
		int delayCnt = 0;
		while (m_bQtMuxStopFileDone == FALSE)//wait for file closing
		{
			g_usleep(10 * MILLISEC_TO_SEC);
			if((delayCnt++) > 100)
				break;
		}
	}

    m_bQtMuxStopFileDone = TRUE;
	m_bFatalError = FALSE;
	m_bFatalErrorNotifyHasFired = FALSE;

    return DVR_RES_SOK;
}

int	CLoopRecorderEngine::Stop(gboolean bLockState)
{
    int Ret = DVR_RES_SOK;

    GST_INFO("*************LoopRecStop***********\n");

    if (m_eLoopRecState == LOOP_RECORD_STATE_INVALID ||
        m_eLoopRecState == LOOP_RECORD_STATE_START_TO_STOP ||
        m_eLoopRecState == LOOP_RECORD_STATE_STOP_DONE)
    {
        GST_INFO("[%d]stop video recorder failed, it has already stopped\n", m_eLoopRecState);
        return DVR_RES_SOK;
    }

    GST_INFO("stop loop recorder\n");

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

	GST_INFO("*************LoopRecStop Done***********\n");

    return Ret;
}

int	CLoopRecorderEngine::SetFuncList(void* pList)
{
    m_ptfnCallBackArray = (DVR_RECORDER_CALLBACK_ARRAY*)pList;
    return DVR_RES_SOK;
}

int	CLoopRecorderEngine::SetRecovery(void)
{
    if (m_bNeedRecover && m_ptfnCallBackArray != NULL && m_ptfnCallBackArray->MessageCallBack != NULL)
    {
        g_print("LoopRecord--------SetRecovery !!!!\n");
        m_bNeedRecover  = FALSE;
        RECORDER_EVENT  eEvent = RECORDER_EVENT_LOOPREC_FATAL_RECOVER;
        m_ptfnCallBackArray->MessageCallBack((void*)&eEvent, NULL, NULL, m_ptfnCallBackArray->pCallBackInstance);
    }
    return DVR_RES_SOK;
}

int CLoopRecorderEngine::SetFatalEnd(void)
{
    if (m_eLoopRecState == LOOP_RECORD_STATE_RUNNING)
    {
        if (m_ptfnCallBackArray != NULL && m_ptfnCallBackArray->MessageCallBack != NULL)
        {
            g_print("LoopRecord--------SetFatalEnd !!!!\n");
            m_bFatalError   = TRUE;
            m_bNeedRecover  = TRUE;
            RECORDER_EVENT  eEvent = RECORDER_EVENT_LOOPREC_FATAL_ERROR;
            m_ptfnCallBackArray->MessageCallBack((void*)&eEvent, NULL, NULL, m_ptfnCallBackArray->pCallBackInstance);
        }
    }
    return DVR_RES_SOK;
}

gint CLoopRecorderEngine::CreatePipeline(void)
{
    gboolean link_ok;
    GstPad *srcpad = NULL, *sinkpad = NULL;
    GstBus* bus = NULL;
    GstCaps * prop_caps = NULL;
    LoopRecPrivateSetting *pSetting = &m_LoopRecSetting;

    m_pPipeline = gst_pipeline_new("LoopRecord");
    if (!m_pPipeline)
    {
        GST_ERROR("Create Recorder Pipeline Failed\n\n");
        goto exit;
    }

    m_pVideoAppSrc = gst_element_factory_make("appsrc", "source");
    m_pVideoParse = gst_element_factory_make("h264parse", "parser");
    m_pQtMuxer = gst_element_factory_make("qtmux", "muxer");
    m_pSink = gst_element_factory_make("splitmuxsink", "sink");
    if (!m_pVideoAppSrc || !m_pVideoParse || !m_pQtMuxer || !m_pSink)
    {
        GST_ERROR("[Engine]Create One of Plugins failed!\n");
        goto exit;
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

    g_signal_connect(m_pVideoAppSrc, "need-data", G_CALLBACK(OnNeedData), this);

    g_object_set(G_OBJECT(m_pQtMuxer), "reserved-max-duration", (guint64)((guint64)30 * NANOSEC_TO_SEC), NULL);
    g_object_set(G_OBJECT(m_pQtMuxer), "reserved-bytes-per-sec", (guint32)(RECORD_FPS*(30 + sizeof(Ofilm_Can_Data_T)) + RESERVED_PER_SEC_PER_TRAK_SIZE), NULL);  // per second per trak reserve bytes
    g_object_set(G_OBJECT(m_pQtMuxer), "reserved-moov-update-period", (guint64)RESERVED_MOOV_UPDATE_INTERVAL, NULL); // update moov per 1 second
    g_object_set(G_OBJECT(m_pQtMuxer), "reserved-extra-bytes", (guint32)RESERVED_MOOV_EXTRA_BYTES, NULL);

    g_object_set(G_OBJECT(m_pSink), "muxer", m_pQtMuxer, NULL);
	g_object_set(G_OBJECT(m_pSink), "use-robust-muxing", TRUE, NULL);
    /* Never forget type conversion here, or it won't work */
    g_object_set(G_OBJECT(m_pSink), "max-size-time", (guint64)((guint64)pSetting->m_ActivePeriod * NANOSEC_TO_SEC), NULL);

    /* offer the new dest location */
    g_signal_connect(m_pSink, "format-location", G_CALLBACK(OnUpdateLoopRecDest), this);
    
    /* add tag */
    g_signal_connect(m_pQtMuxer, "tag-add", G_CALLBACK(OnUpdateLoopTagList), this);

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

gint CLoopRecorderEngine::DestroyPipeline(void)
{
    GstStateChangeReturn ret;

    if (!gst_is_initialized() || m_pPipeline == NULL)
    {
        GST_ERROR("[Engine][%d]Gstreamer is not initialized!\n", __LINE__);
        return DVR_RES_EFAIL;
    }

    SetStateWait(GST_STATE_NULL);

    while (GST_OBJECT_REFCOUNT(m_pPipeline))
    {
        gst_object_unref(GST_OBJECT(m_pPipeline));
    }

    m_pPipeline = NULL;
    m_pVideoAppSrc = NULL;
    m_pVideoParse = NULL;

    m_pQtMuxer = NULL;
    m_pSink = NULL;

    return DVR_RES_SOK;
}

gint CLoopRecorderEngine::RebuildPipeline(void)
{
    GST_INFO("*************Loop RebuildPipeline start***********\n");
    DestroyPipeline();
    CreatePipeline();
    GST_INFO("*************Loop RebuildPipeline  stop***********\n");
    return DVR_RES_SOK;
}

void CLoopRecorderEngine::OnNeedData(GstElement *appsrc, guint unused_size, gpointer user_data)
{
    CLoopRecorderEngine *pThis = (CLoopRecorderEngine*)user_data;
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

gchar *CLoopRecorderEngine::OnUpdateLoopTagList(GstElement * element, guint fragment_id, gpointer data)
{
    CLoopRecorderEngine *pThis = (CLoopRecorderEngine*)data;

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
        pThis->m_pRecorderAsyncOp->RecorderAsyncOp_SndMsg(RECORDER_ASYNCOP_CMD_GENERATE_THUMBNAIL, (DVR_U32)pThis->m_CurLoopRecThumbNailFileName, 0);
    }

    //if (pThis->m_pRecorderAsyncOp != NULL){
    //    pThis->m_pRecorderAsyncOp->RecorderAsyncOp_SndMsg(RECORDER_ASYNCOP_CMD_ADD_TAG_LIST, (DVR_U32)pThis->m_CurLoopRecThumbNailFileName, 0);
    //}
    return NULL;
}

gchar *CLoopRecorderEngine::OnUpdateLoopRecDest(GstElement * element, guint fragment_id, gpointer data)
{
    CLoopRecorderEngine *pThis = (CLoopRecorderEngine*)data;
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
    g_free(thumb_name);

	//if (pThis->m_pRecorderAsyncOp != NULL){
    //    pThis->m_pRecorderAsyncOp->RecorderAsyncOp_SndMsg(RECORDER_ASYNCOP_CMD_GENERATE_THUMBNAIL, (DVR_U32)pThis->m_CurLoopRecThumbNailFileName, 0);
    //}

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

GstBusSyncReply CLoopRecorderEngine::MessageHandler(GstBus *bus, GstMessage *msg, gpointer pData)
{
    if (NULL == pData)
    {
        return GST_BUS_PASS;
    }

    CLoopRecorderEngine *pThis = (CLoopRecorderEngine *)pData;
    
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

                tFileInfo.file_location = location;
                tFileInfo.thumbnail_location = pThis->m_CurLoopRecThumbNailFileName;
                tFileInfo.time_stamp = time;
                if (pThis->m_ptfnCallBackArray != NULL && pThis->m_ptfnCallBackArray->MessageCallBack != NULL)
                {
                    RECORDER_EVENT  eEvent = RECORDER_EVENT_NEW_FILE_START;
                    pThis->m_ptfnCallBackArray->MessageCallBack((void*)&eEvent, &tFileInfo, NULL, pThis->m_ptfnCallBackArray->pCallBackInstance);
                }

                pThis->m_bQtMuxStopFileDone = FALSE;
            }
            else if (gst_structure_has_name(st, "splitmuxsink-fragment-closed"))
            {
                location = gst_structure_get_string(st, "location");
                gst_structure_get_clock_time(st, "running-time", &time);
                tFileInfo.file_location = location;
                tFileInfo.thumbnail_location = pThis->m_CurLoopRecThumbNailFileName;
                tFileInfo.time_stamp = time;
                tFileInfo.eType = DVR_FOLDER_TYPE_NORMAL;
                if (pThis->m_ptfnCallBackArray != NULL && pThis->m_ptfnCallBackArray->MessageCallBack != NULL)
                {
                    RECORDER_EVENT  eEvent = RECORDER_EVENT_NEW_FILE_FINISHED;
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
                DVR_BOOL error = TRUE;
				RECORDER_EVENT	eEvent = RECORDER_EVENT_LOOPREC_FATAL_ERROR;
				pThis->m_ptfnCallBackArray->MessageCallBack((void*)&eEvent, NULL, &error, pThis->m_ptfnCallBackArray->pCallBackInstance);
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

int	CLoopRecorderEngine::Create(void)
{
    return CreatePipeline();
}

int	CLoopRecorderEngine::Destroy(void)
{
    return DestroyPipeline();
}

int CLoopRecorderEngine::AddVideoFrame(GstBuffer *buffer)
{
    if (m_eLoopRecState != LOOP_RECORD_STATE_RUNNING || m_bSlowWriting == TRUE)
    {
        gst_buffer_unref(buffer);
        return TRUE;
    }

    g_mutex_lock(&m_nFrameMutex);

    if (g_queue_get_length(&m_nFrameQueue) >= MAX_LOOP_QUEUE_FRAMES)
    {
        g_print("Record Error(list full) framenum %d !!!!\n",g_queue_get_length(&m_nFrameQueue));

		m_bSlowWriting = TRUE;

		if (m_ptfnCallBackArray != NULL && m_ptfnCallBackArray->MessageCallBack != NULL)
		{
			RECORDER_EVENT eEvent = RECORDER_EVENT_SLOW_WRITING;
			m_ptfnCallBackArray->MessageCallBack((void*)&eEvent, NULL, NULL, m_ptfnCallBackArray->pCallBackInstance);
		}
    }
	else
	{
		g_queue_push_head(&m_nFrameQueue, buffer);
	}

    g_mutex_unlock(&m_nFrameMutex);

    return TRUE;
}

int	CLoopRecorderEngine::Set(LOOP_RECORD_PROP_ID ePropId, void *pPropData, int nPropSize)
{
    switch (ePropId)
    {
    case LOOP_RECORD_PROP_DIR:
    {
        if (pPropData == NULL)
        {
            GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
            return DVR_RES_EINVALIDARG;
        }
		
		if(m_LoopRecSetting.loop_record_location != NULL)
		{
			g_free(m_LoopRecSetting.loop_record_location);
			m_LoopRecSetting.loop_record_location = NULL;
		}
		
        m_LoopRecSetting.loop_record_location = g_strdup((const char*)pPropData);
        m_LoopRecSetting.loop_record_folder_max_file_index = get_max_file_index(m_LoopRecSetting.loop_record_location) + 1;
    }
    break;

    case LOOP_RECORD_PROP_FILE_SPLIT_TIME:
    {
        if (pPropData == NULL || nPropSize != sizeof(DVR_RECORDER_PERIOD_OPTION))
        {
            GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
            return DVR_RES_EINVALIDARG;
        }

        DVR_RECORDER_PERIOD_OPTION *pOption = (DVR_RECORDER_PERIOD_OPTION *)pPropData;
        if (TRUE == pOption->bImmediatelyEffect)
        {
            g_object_set(G_OBJECT(m_pSink), "max-size-time", (guint64)((guint64)pOption->nSplitTime * NANOSEC_TO_SEC), NULL);
            g_object_set(G_OBJECT(m_pQtMuxer), "reserved-max-duration", (guint64)((guint64)(pOption->nSplitTime + 3) * NANOSEC_TO_SEC), NULL);
            m_LoopRecSetting.m_ActivePeriod = pOption->nSplitTime;
            GST_INFO("set loopenc file split time to %d second, will take effect immediately\n", m_LoopRecSetting.m_ActivePeriod - 1);
        }
        else
        {
            m_LoopRecSetting.m_PendPeriod = *(int*)pPropData;
            m_LoopRecSetting.m_bPendPeriod = TRUE;
            GST_INFO("set loopenc file split time to %d second, will take effect at next file\n", m_LoopRecSetting.m_PendPeriod - 1);
        }
    }
	break;

	case LOOP_RECORD_PROP_FATAL_ERROR:
	{
        if (pPropData == NULL || nPropSize != sizeof(gboolean))
		{
			GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		m_bFatalError = *(gboolean *)pPropData;
	}
	break;

    default:
        break;
    }

    return DVR_RES_SOK;
}

int	CLoopRecorderEngine::Get(LOOP_RECORD_PROP_ID ePropId, void *pPropData, int nPropSize)
{
    switch (ePropId)
    {
    case LOOP_RECORD_PROP_FILE_SPLIT_TIME:
    {
        if (pPropData == NULL || nPropSize != sizeof(DVR_RECORDER_PERIOD_OPTION))
        {
            GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
            return DVR_RES_EINVALIDARG;
        }

		DVR_RECORDER_PERIOD_OPTION *pOption = (DVR_RECORDER_PERIOD_OPTION *)pPropData;
		pOption->nSplitTime = m_LoopRecSetting.m_ActivePeriod;
    }
    break;

    default:
        break;
    }

    return DVR_RES_SOK;
}
