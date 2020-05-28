#include "DVR_RECORDER_DEF.h"
#include "EventRecorderEngine.h"
#include "DvrSystemControl.h"
#include "VideoBuffer.h"
#include "RecorderUtils.h"
#include <gst/canbuf/canbuf.h>

extern gdouble ms_time(void);
GST_DEBUG_CATEGORY_STATIC(event_recorder_engine_category);  // define category (statically)
#define GST_CAT_DEFAULT event_recorder_engine_category       // set as default

CEventRecorderEngine::CEventRecorderEngine(RecorderAsyncOp *pHandle, CVideoBuffer *pVideoCB)
{
    m_pPipeline         = NULL;
    m_pVideoAppSrc      = NULL;
    m_pVideoParse       = NULL;
    m_pQtMuxer          = NULL;
    m_pSink             = NULL;
    m_pVideoBuffer      = pVideoCB;
    m_pRecorderAsyncOp  = pHandle;
    memset(&m_hThread_Monitor, 0, sizeof(OSA_TskHndl));
}

CEventRecorderEngine::~CEventRecorderEngine()
{
    Close();
}

int CEventRecorderEngine::Open(void)
{
    DVR_S32 res;

    GST_DEBUG_CATEGORY_INIT(event_recorder_engine_category, "EVENT_RECORDER_ENGINE", GST_DEBUG_FG_YELLOW, "event recorder engine category");

    memset(&m_Setting, 0, sizeof(DVR_EVENT_RECORD_SETTING));

    memset(&m_EventRecSetting, 0, sizeof(EventRecPrivateSetting));
    m_EventRecSetting.video_dst_format = g_strdup("EVT_%Y%m%d_%H%M%S_M");
    m_EventRecSetting.thumbnail_dst_format = g_strdup("THM_%Y%m%d_%H%M%S_F");
    Reset();

    g_mutex_init(&m_nMutex);
    return DVR_RES_SOK;
}

int CEventRecorderEngine::Close(void)
{
  	if(m_EventRecSetting.video_dst_format)
	{
		g_free(m_EventRecSetting.video_dst_format);
		m_EventRecSetting.video_dst_format = NULL;
	}

	if(m_EventRecSetting.thumbnail_dst_format)
	{
		g_free(m_EventRecSetting.thumbnail_dst_format);
		m_EventRecSetting.thumbnail_dst_format = NULL;
	}

	if(m_EventRecSetting.event_record_location != NULL)
	{
		g_free(m_EventRecSetting.event_record_location);
		m_EventRecSetting.event_record_location = NULL;
	}	

	g_mutex_clear(&m_nMutex);
    return DVR_RES_SOK;
}

int	CEventRecorderEngine::Start(void)
{
    GST_INFO("*************EventRecStart***********\n");

    if (m_eEventRecState == EVENT_RECORD_STATE_RUNNING)
    {
        GST_INFO("event recording has already triggered");
        return DVR_RES_EFAIL;
    }

    uint count = 0;
    while (m_eEventRecState == EVENT_RECORD_STATE_START_TO_STOP)//wait for file closing
    {
        if(count >= 50)
            break;
        GST_WARNING("stop process is running, wait stop complete\n");
        usleep(200*1000);
        count++;
    }

	//fixed 15s
    m_postRecordTimeLimit = (m_Setting.EventRecordLimitTime - m_Setting.EventPreRecordLimitTime)*MILLISEC_TO_SEC;

    m_eEventRecState = EVENT_RECORD_STATE_RUNNING;
    m_pVideoBuffer->Start();

    gst_element_set_state(m_pPipeline, GST_STATE_PLAYING);

    return DVR_RES_SOK;
}

gint CEventRecorderEngine::SendEOSAndWait(void)
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

    return DVR_RES_SOK;
}

int	CEventRecorderEngine::Stop(void)
{
    GST_INFO("*************EventRecStop*************");

    if (m_eEventRecState == EVENT_RECORD_STATE_IDLE ||
        m_eEventRecState == EVENT_RECORD_STATE_START_TO_STOP)
	{
		GST_INFO("Event Recording Already Stopped\n");
		return 0;
	}
    m_eEventRecState = EVENT_RECORD_STATE_START_TO_STOP;

    SendEOSAndWait();

	if(m_bFatalError == TRUE)
	{
		DVR_ADDTODB_FILE_INFO tFileInfo;
		
		tFileInfo.file_location = m_CurEventRecVideoFileName;
		tFileInfo.thumbnail_location = m_CurEventRecThumbNailFileName;
		tFileInfo.time_stamp = 0;
		tFileInfo.eType = DVR_FOLDER_TYPE_EMERGENCY;
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

    gst_element_set_state(m_pPipeline, GST_STATE_NULL);
    if(m_brebuilding == TRUE)
    {
        RebuildPipeline();
        m_brebuilding = FALSE;
    }

    Reset();

	GST_INFO("*************EventRecStop Done***********");

    return 0;
}

int CEventRecorderEngine::AddVideoFrame(GstBuffer *buffer)
{
    GstMapInfo map;
    guint64 pts, dts;
    gboolean bIsKeyFrame;

    Ofilm_Can_Data_T *pCan = NULL;
#ifdef __linux__ 				
    GstMetaCanBuf *canbuf = gst_buffer_get_can_buf_meta(buffer);
    if (canbuf != NULL)
    {
        pCan = gst_can_buf_meta_get_can_data(canbuf);
    }
#endif

    pts = GST_BUFFER_PTS(buffer)/MILLISEC_TO_SEC;
    dts = GST_BUFFER_DTS(buffer)/MILLISEC_TO_SEC;
    bIsKeyFrame = !GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT);

    if (m_eEventRecState == EVENT_RECORD_STATE_RUNNING)
    {
        if (bIsKeyFrame && !m_bPostRecordStartTimeStampInitDone)
        {
            m_postRecordStartTimeStamp = pts;
            m_bPostRecordStartTimeStampInitDone = TRUE;
        }

        g_mutex_lock(&m_nMutex);
        m_bUsedRecordTime = pts - m_postRecordStartTimeStamp;
        gboolean oversize = m_bUsedRecordTime >= m_postRecordTimeLimit ? TRUE : FALSE;
        g_mutex_unlock(&m_nMutex);
        if (bIsKeyFrame && m_bPostRecordStartTimeStampInitDone && oversize)
        {
            GST_INFO("m_bUsedRecordTime %lld m_postRecordTimeLimit %lld m_postRecordStartTimeStamp %lld\n",m_bUsedRecordTime, m_postRecordTimeLimit, m_postRecordStartTimeStamp );
            m_bEventRecWriteComplete = TRUE;
        }

        if(m_bUsedRecordTime/MICROSEC_TO_SEC == 8 && m_bForceFlush == FALSE)
        {
            m_bForceFlush = TRUE;
            ForceFlush();
        }
    }
    gst_buffer_unref(buffer);

    return DVR_RES_SOK;
}

gint CEventRecorderEngine::CreatePipeline(void)
{
    gboolean link_ok;
    GstPad *srcpad = NULL;
    GstPad *sinkpad = NULL;
    GstBus* bus = NULL;
    GstCaps * prop_caps = NULL;

    m_pPipeline = gst_pipeline_new("EventRecord");
    if (!m_pPipeline)
    {
        GST_ERROR("Create Recorder Pipeline Failed\n\n");
        goto exit;
    }

    m_pVideoAppSrc = gst_element_factory_make("appsrc", "event-source");
    m_pVideoParse = gst_element_factory_make("h264parse", "event-parser");
    m_pQtMuxer = gst_element_factory_make("qtmux", "event-muxer");
    m_pSink = gst_element_factory_make("splitmuxsink", "event-sink");
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

    g_signal_connect(m_pVideoAppSrc, "need-data", G_CALLBACK(OnNeedData), this);

    g_object_set(G_OBJECT(m_pQtMuxer), "reserved-max-duration", (guint64)((guint64)30 * NANOSEC_TO_SEC), NULL);
    g_object_set(G_OBJECT(m_pQtMuxer), "reserved-bytes-per-sec", (guint32)(RECORD_FPS*(30 + sizeof(Ofilm_Can_Data_T)) + RESERVED_PER_SEC_PER_TRAK_SIZE), NULL);  // per second per trak reserve bytes
    g_object_set(G_OBJECT(m_pQtMuxer), "reserved-moov-update-period", (guint64)RESERVED_MOOV_UPDATE_INTERVAL, NULL); // update moov per 1 second
    g_object_set(G_OBJECT(m_pQtMuxer), "reserved-extra-bytes", (guint32)RESERVED_MOOV_EXTRA_BYTES, NULL);

	g_object_set(G_OBJECT(m_pSink), "muxer", m_pQtMuxer, NULL);
	g_object_set(G_OBJECT(m_pSink), "use-robust-muxing", TRUE, NULL);
    g_object_set(G_OBJECT(m_pSink), "max-size-time", (guint64)((guint64)60 * NANOSEC_TO_SEC), NULL);
    /* offer the new dest location */
    g_signal_connect(m_pSink, "format-location", G_CALLBACK(OnUpdateEventRecDest), this);
    /* add tag */
    g_signal_connect(m_pQtMuxer, "tag-add", G_CALLBACK(OnUpdateEventTagList), this);
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

gint CEventRecorderEngine::DestroyPipeline(void)
{
    GstStateChangeReturn ret;

    if (!gst_is_initialized() || m_pPipeline == NULL)
    {
        GST_ERROR("[Engine][%d]Gstreamer is not initialized!\n", __LINE__);
        return DVR_RES_EFAIL;
    }

    ret = gst_element_set_state(m_pPipeline, GST_STATE_NULL);
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

gint CEventRecorderEngine::RebuildPipeline(void)
{
    GST_INFO("*************Event RebuildPipeline start***********\n");
    DestroyPipeline();
    CreatePipeline();
    GST_INFO("*************Event RebuildPipeline  stop***********\n");
    return DVR_RES_SOK;
}

void CEventRecorderEngine::OnNeedData(GstElement *appsrc, guint unused_size, gpointer user_data)
{
    CEventRecorderEngine *pThis = (CEventRecorderEngine*)user_data;
    GstBuffer *buffer;
    GstFlowReturn ret;
    GstMetaCanBuf *pCanMeta = NULL;
    int lRet;
    GstMapInfo map;
    FrameNode node;


    if(pThis->m_eEventRecState == EVENT_RECORD_STATE_RUNNING)
    {
        if(pThis->m_eEventRecState == EVENT_RECORD_STATE_REDAY_TO_STOP)
        {
            g_usleep(40 * MILLISEC_TO_SEC);
            return;
        }
        //first frame
        if (pThis->m_fistNodeTimeStamp == 0)
        {
            pThis->m_pVideoBuffer->GetFrame(&node, true);
            while(node.FrameInfo.nFrameType != FRAME_TYPE_I && 
                (pThis->m_eEventRecState == EVENT_RECORD_STATE_RUNNING))
            {
                memset(&node, 0, sizeof(FrameNode));
                pThis->m_pVideoBuffer->GetFrame(&node, false);
            }
	        pThis->m_fistNodeTimeStamp = node.FrameInfo.nTimeStamp;
        }
        else
        {
            while (pThis->m_pVideoBuffer->GetFrame(&node, false) < 0 && 
                (pThis->m_eEventRecState == EVENT_RECORD_STATE_RUNNING))
            {
                //GST_INFO("not enough data in image queue or it is not start, waiting...\n");
                g_usleep(40 * MILLISEC_TO_SEC);
            }
        }


        if (pThis->m_bEventRecWriteComplete && !pThis->m_bEOSHasTriggered)
        {
            /*GST_INFO("nFrameType %d  pts0 %lld ulBuffLength %d---pts %lld\n",
            node.FrameInfo.nFrameType,
            node.FrameInfo.nTimeStamp,
            node.FrameInfo.nFrameLen,node.FrameInfo.nTimeStamp);*/

            GST_INFO("Send EOS from AppSrc\n");
            g_signal_emit_by_name(appsrc, "end-of-stream", &ret);

            GST_INFO("Total Push Buffer Duration : %d ms, Post Record Duration : %d ms\n",
                (guint)((node.FrameInfo.nTimeStamp - pThis->m_fistNodeTimeStamp) / GST_MSECOND), (guint)(pThis->m_postRecordTimeLimit / GST_MSECOND));
            pThis->m_bEOSHasTriggered = TRUE;
            return;
        }
        buffer = gst_buffer_new_allocate(NULL, node.FrameInfo.nFrameLen, NULL);
        gst_buffer_fill(buffer, 0, (unsigned char*)node.nDataPos, node.FrameInfo.nFrameLen);
        
        GST_BUFFER_PTS(buffer) = node.FrameInfo.nTimeStamp*MILLISEC_TO_SEC;
        GST_BUFFER_DTS(buffer) = GST_BUFFER_PTS(buffer);
        GST_BUFFER_DURATION(buffer) = GST_CLOCK_TIME_NONE;
        GST_BUFFER_OFFSET(buffer) = GST_BUFFER_OFFSET_NONE;
        GST_BUFFER_OFFSET_END(buffer) = GST_BUFFER_OFFSET_NONE;

        pCanMeta = gst_buffer_add_can_buf_meta(GST_BUFFER(buffer), node.nCanPos, node.FrameInfo.nCanLen);
        if (!pCanMeta){
            GST_ERROR("Failed to add can meta to buffer");
            //TODO:Free the meta
        }

        g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
        gst_buffer_unref(buffer);
    }
}

gchar *CEventRecorderEngine::OnUpdateEventTagList(GstElement * element, guint fragment_id, gpointer data)
{
    CEventRecorderEngine *pThis = (CEventRecorderEngine*)data;

    GDateTime *date = g_date_time_new_now_local();
    gchar *description = g_date_time_format(date, "%Y-%m-%d-%H-%M-%S");
    g_date_time_unref (date);

    GstTagList *tag_list = gst_tag_list_new_empty();
    gst_tag_list_add(tag_list, GST_TAG_MERGE_APPEND, GST_TAG_DESCRIPTION, description, NULL);
    GST_OBJECT_LOCK(pThis->m_pQtMuxer);
    gst_tag_setter_merge_tags(GST_TAG_SETTER(pThis->m_pQtMuxer), tag_list, GST_TAG_MERGE_APPEND);
    GST_OBJECT_UNLOCK(pThis->m_pQtMuxer);

    gst_tag_list_unref(tag_list);
    g_free(description);

    if (pThis->m_pRecorderAsyncOp != NULL){
        pThis->m_pRecorderAsyncOp->RecorderAsyncOp_SndMsg(RECORDER_ASYNCOP_CMD_GENERATE_THUMBNAIL, (DVR_U32)pThis->m_CurEventRecThumbNailFileName, 1);
    }

    //if (pThis->m_pRecorderAsyncOp != NULL){
    //    pThis->m_pRecorderAsyncOp->RecorderAsyncOp_SndMsg(RECORDER_ASYNCOP_CMD_ADD_TAG_LIST, (DVR_U32)pThis->m_CurEventRecThumbNailFileName, 1);
    //}

    return NULL;
}

gchar *CEventRecorderEngine::OnUpdateEventRecDest(GstElement * element, guint fragment_id, gpointer data)
{
    CEventRecorderEngine *pThis = (CEventRecorderEngine*)data;
    EventRecPrivateSetting *pSetting = (EventRecPrivateSetting *)&pThis->m_EventRecSetting;

    gchar *dst = create_video_filename(pSetting->video_dst_format, pSetting->event_record_folder_max_file_index, pSetting->event_record_location);
	memset(pThis->m_CurEventRecVideoFileName, 0, sizeof(pThis->m_CurEventRecVideoFileName));
	if(dst != NULL){
		strncpy(pThis->m_CurEventRecVideoFileName, dst, APP_MAX_FN_SIZE);
	}

	gchar *thumb_name = create_thumbnail_filename(pSetting->thumbnail_dst_format, pSetting->event_record_folder_max_file_index, pSetting->event_record_location);
	memset(pThis->m_CurEventRecThumbNailFileName, 0, sizeof(pThis->m_CurEventRecThumbNailFileName));
	if(thumb_name != NULL){
		strncpy(pThis->m_CurEventRecThumbNailFileName, thumb_name, APP_MAX_FN_SIZE);
	}
    g_free(thumb_name);

    //if (pThis->m_pRecorderAsyncOp != NULL){
    //    pThis->m_pRecorderAsyncOp->RecorderAsyncOp_SndMsg(RECORDER_ASYNCOP_CMD_GENERATE_THUMBNAIL, (DVR_U32)pThis->m_CurEventRecThumbNailFileName, 1);
    //}

    pSetting->event_record_folder_max_file_index += 1;

    return dst;
}

GstBusSyncReply CEventRecorderEngine::MessageHandler(GstBus *bus, GstMessage *msg, gpointer pData)
{
    if (NULL == pData)
    {
        return GST_BUS_PASS;
    }

    CEventRecorderEngine *pThis = (CEventRecorderEngine *)pData;
    
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
                if(pThis->m_eEventRecState == EVENT_RECORD_STATE_REDAY_TO_STOP)
                    return GST_BUS_PASS;
                location = gst_structure_get_string(st, "location");
                gst_structure_get_clock_time(st, "running-time", &time);

                tFileInfo.file_location = location;
                tFileInfo.thumbnail_location = pThis->m_CurEventRecThumbNailFileName;
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
                pThis->m_eEventRecState = EVENT_RECORD_STATE_REDAY_TO_STOP;
                location = gst_structure_get_string(st, "location");
                gst_structure_get_clock_time(st, "running-time", &time);
                tFileInfo.file_location = location;
                tFileInfo.thumbnail_location = pThis->m_CurEventRecThumbNailFileName;
                tFileInfo.time_stamp = time;
                tFileInfo.eType = DVR_FOLDER_TYPE_EMERGENCY;

                if( pThis->m_Setting.eType == DVR_EVENTREC_SOURCE_TYPE_IACC_IAC ||
                    pThis->m_Setting.eType == DVR_EVENTREC_SOURCE_TYPE_IACC_ACC)
                    tFileInfo.eType = DVR_FOLDER_TYPE_DAS;

                if (pThis->m_ptfnCallBackArray != NULL && pThis->m_ptfnCallBackArray->MessageCallBack != NULL)
                {
                    RECORDER_EVENT  eEvent = RECORDER_EVENT_NEW_FILE_FINISHED;
                    pThis->m_ptfnCallBackArray->MessageCallBack((void*)&eEvent, &tFileInfo, NULL, pThis->m_ptfnCallBackArray->pCallBackInstance);

					if(pThis->m_Setting.eType == DVR_EVENTREC_SOURCE_TYPE_SWITCH ||
                       pThis->m_Setting.eType == DVR_EVENTREC_SOURCE_TYPE_CRASH)
					{
						eEvent = RECORDER_EVENT_EMERGENCY_COMPLETE;
					}
					else if(pThis->m_Setting.eType == DVR_EVENTREC_SOURCE_TYPE_ALARM)
					{
						eEvent = RECORDER_EVENT_ALARM_COMPLETE;
					}
					else if(pThis->m_Setting.eType == DVR_EVENTREC_SOURCE_TYPE_IACC_IAC ||
                            pThis->m_Setting.eType == DVR_EVENTREC_SOURCE_TYPE_IACC_ACC ||
                            pThis->m_Setting.eType == DVR_EVENTREC_SOURCE_TYPE_IACC_AEB)
					{
						eEvent = RECORDER_EVENT_IACC_COMPLETE;
					}
					else
					{
						//TODO
					}
                        
                    pThis->m_ptfnCallBackArray->MessageCallBack((void*)&eEvent, NULL, NULL, pThis->m_ptfnCallBackArray->pCallBackInstance);
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

		if(pThis->m_bFatalErrorNotifyHasFired == FALSE && pThis->m_eEventRecState == EVENT_RECORD_STATE_RUNNING)
		{
            pThis->m_brebuilding = TRUE;
			pThis->m_bFatalError = TRUE;
			pThis->m_bFatalErrorNotifyHasFired = TRUE;

			if (pThis->m_ptfnCallBackArray != NULL && pThis->m_ptfnCallBackArray->MessageCallBack != NULL)
			{
				RECORDER_EVENT	eEvent = RECORDER_EVENT_EVENTREC_FATAL_ERROR;
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

int	CEventRecorderEngine::Create(void)
{
    return CreatePipeline();
}

int	CEventRecorderEngine::Destroy(void)
{
    return DestroyPipeline();
}

int	CEventRecorderEngine::Set(EVENT_RECORD_PROP_ID ePropId, void *pPropData, int nPropSize)
{
    switch (ePropId)
    {
    case EVENT_RECORD_PROP_DIR:
    {
        if (pPropData == NULL)
        {
            GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
            return DVR_RES_EINVALIDARG;
        }
		
		if(m_EventRecSetting.event_record_location != NULL)
		{
			g_free(m_EventRecSetting.event_record_location);
			m_EventRecSetting.event_record_location = NULL;
		}

        m_EventRecSetting.event_record_location = g_strdup((const char*)pPropData);
        m_EventRecSetting.event_record_folder_max_file_index = get_max_file_index(m_EventRecSetting.event_record_location) + 1;
    }
    break;

    case EVENT_RECORD_PROP_PERIOD_SETTING:
    {
        if (pPropData == NULL || nPropSize != sizeof(DVR_EVENT_RECORD_SETTING))
        {
            GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
            return DVR_RES_EINVALIDARG;
        }

        DVR_EVENT_RECORD_SETTING *pSetting = (DVR_EVENT_RECORD_SETTING *)pPropData;
        memcpy(&m_Setting, pSetting, sizeof(DVR_EVENT_RECORD_SETTING));
        if(m_EventRecSetting.video_dst_format != NULL)
        {
            g_free(m_EventRecSetting.video_dst_format);
            m_EventRecSetting.video_dst_format = NULL;
        }
        if(m_Setting.eType == DVR_EVENTREC_SOURCE_TYPE_SWITCH ||
            m_Setting.eType == DVR_EVENTREC_SOURCE_TYPE_ALARM)
            m_EventRecSetting.video_dst_format = g_strdup("EVT_%Y%m%d_%H%M%S_M");
        else if(m_Setting.eType == DVR_EVENTREC_SOURCE_TYPE_CRASH)
            m_EventRecSetting.video_dst_format = g_strdup("CRS_%Y%m%d_%H%M%S_M");
        else if(m_Setting.eType == DVR_EVENTREC_SOURCE_TYPE_IACC_IAC)
            m_EventRecSetting.video_dst_format = g_strdup("IAC_%Y%m%d_%H%M%S_M");
        else if(m_Setting.eType == DVR_EVENTREC_SOURCE_TYPE_IACC_ACC)
            m_EventRecSetting.video_dst_format = g_strdup("ACC_%Y%m%d_%H%M%S_M");
        else if(m_Setting.eType == DVR_EVENTREC_SOURCE_TYPE_IACC_AEB)
            m_EventRecSetting.video_dst_format = g_strdup("AEB_%Y%m%d_%H%M%S_M");

        if(m_EventRecSetting.event_record_location != NULL)
        {
            g_free(m_EventRecSetting.event_record_location);
            m_EventRecSetting.event_record_location = NULL;
        }
        m_EventRecSetting.event_record_location = g_strdup((const char*)m_Setting.szEventDir);

		if(pSetting->EventPreRecordLimitTime == 0)
		{
			if (m_pVideoBuffer != NULL)
				m_pVideoBuffer->Reset();
		}
        m_pVideoBuffer->SetLimitTime(pSetting->EventPreRecordLimitTime);

        DVR_U32 nMaxTime = pSetting->EventRecordLimitTime + 3*1000;
        g_object_set(G_OBJECT(m_pSink), "max-size-time", (guint64)((guint64)nMaxTime * MICROSEC_TO_SEC), NULL);
        g_object_set(G_OBJECT(m_pQtMuxer), "reserved-max-duration", (guint64)((guint64)nMaxTime * MICROSEC_TO_SEC), NULL);
        g_object_set(G_OBJECT(m_pQtMuxer), "reserved-bytes-per-sec", (guint32)(RECORD_FPS*(30 + sizeof(Ofilm_Can_Data_T)) + RESERVED_PER_SEC_PER_TRAK_SIZE), NULL);  // per second per trak reserve bytes
    }
    break;

	case EVENT_RECORD_PROP_FATAL_ERROR:
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

int	CEventRecorderEngine::Get(EVENT_RECORD_PROP_ID ePropId, void *pPropData, int nPropSize)
{
    switch (ePropId)
    {
    case EVENT_RECORD_PROP_PERIOD_SETTING:
    {
        if (pPropData == NULL || nPropSize != sizeof(DVR_EVENT_RECORD_SETTING))
        {
            GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
            return DVR_RES_EINVALIDARG;
        }

        memcpy(pPropData, &m_Setting, sizeof(DVR_EVENT_RECORD_SETTING));
        break;
    }

    default:
        break;
    }

    return DVR_RES_SOK;
}

int	CEventRecorderEngine::SetFuncList(void* pList)
{
    m_ptfnCallBackArray = (DVR_RECORDER_CALLBACK_ARRAY*)pList;

    return DVR_RES_SOK;
}

int CEventRecorderEngine::SetFatalEnd(void)
{
    if (m_eEventRecState == EVENT_RECORD_STATE_RUNNING)
    {
        if (m_ptfnCallBackArray != NULL && m_ptfnCallBackArray->MessageCallBack != NULL)
        {
            RECORDER_EVENT  eEvent = RECORDER_EVENT_EVENTREC_FATAL_ERROR;
            m_ptfnCallBackArray->MessageCallBack((void*)&eEvent, NULL, NULL, m_ptfnCallBackArray->pCallBackInstance);
        }
    }
    return DVR_RES_SOK;
}

int CEventRecorderEngine::Reset(void)
{
    m_bEventRecWriteComplete = FALSE;

    m_postRecordTimeLimit = 0;
    m_bUsedRecordTime = 0;
    m_postRecordStartTimeStamp = 0;
    m_bPostRecordStartTimeStampInitDone = FALSE;

    m_fistNodeTimeStamp = 0;
    m_frameCnt = 0;

    m_bEOSHasTriggered = FALSE;
    m_bQtMuxStopFileDone = FALSE;
	m_bFatalError = FALSE;
    m_brebuilding = FALSE;
    m_bForceFlush = FALSE;
	m_bFatalErrorNotifyHasFired = FALSE;


    m_eEventRecState = EVENT_RECORD_STATE_IDLE;
    if (m_pVideoBuffer != NULL)
    {
        m_pVideoBuffer->Reset();
    }

    return DVR_RES_SOK;
}

void* CEventRecorderEngine::Async(void* pParam)
{
#ifdef __linux__
    sync();
#endif
    return NULL;
}

int CEventRecorderEngine::ForceFlush(void)
{
    GST_INFO("[%d]ForceFlush------\n", __LINE__);
    pthread_t   thread;
    DVR_RESULT status = pthread_create(&thread, NULL, Async, NULL);
    if(status != 0)
        DPrint(DPRINT_ERR,"pthread_create() - Could not create thread [%d]\n", status);
    pthread_detach(thread);
}
