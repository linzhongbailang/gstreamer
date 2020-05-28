#include "DVR_RECORDER_DEF.h"
#include "IaccRecorderEngine.h"
#include "DvrSystemControl.h"
#include <gst/canbuf/canbuf.h>
#include "VideoBuffer.h"
#include "RecorderUtils.h"

GST_DEBUG_CATEGORY_STATIC(iacc_recorder_engine_category);  // define category (statically)
#define GST_CAT_DEFAULT iacc_recorder_engine_category       // set as default
#define DEFAULT_DELAY_TIME   15*1000

CIaccRecorderEngine::CIaccRecorderEngine(RecorderAsyncOp *pHandle, CVideoBuffer *pVideoCB) : CEventRecorderEngine(pHandle, pVideoCB)
{
    m_pPipeline         = NULL;
    m_pVideoAppSrc      = NULL;
    m_pVideoParse       = NULL;
    m_pQtMuxer          = NULL;
    m_pSink             = NULL;
    m_pVideoBuffer      = pVideoCB;
    m_pRecorderAsyncOp  = pHandle;
}


CIaccRecorderEngine::~CIaccRecorderEngine(void)
{

}

int CIaccRecorderEngine::Open(void)
{
    DVR_S32 res;

    GST_DEBUG_CATEGORY_INIT(iacc_recorder_engine_category, "IACC_RECORDER_ENGINE", GST_DEBUG_FG_YELLOW, "iacc recorder engine category");

    memset(&m_Setting, 0, sizeof(DVR_EVENT_RECORD_SETTING));

    memset(&m_EventRecSetting, 0, sizeof(EventRecPrivateSetting));
    m_EventRecSetting.video_dst_format = g_strdup("IAC_%Y%m%d_%H%M%S_M");
    m_EventRecSetting.thumbnail_dst_format = g_strdup("THM_%Y%m%d_%H%M%S_F");
    Reset();

    g_mutex_init(&m_nMutex);
    return DVR_RES_SOK;
}

int	CIaccRecorderEngine::Start(void)
{
    GST_INFO("*************IaccRecStart***********\n");
    DVR_S32 res = CEventRecorderEngine::Start();
    m_postRecordTimeLimit = (m_Setting.EventRecordLimitTime - m_Setting.EventPreRecordLimitTime) * MILLISEC_TO_SEC;
    return res;
}

int	CIaccRecorderEngine::Stop(void)
{
    DVR_S32 res;
    GST_INFO("*************IaccRecStop*************");
    res =  CEventRecorderEngine::Stop();
    GST_INFO("*************IaccRecStopdone*************");
    return res;
}

gint CIaccRecorderEngine::CreatePipeline(void)
{
    gboolean link_ok;
    GstPad *srcpad = NULL;
    GstPad *sinkpad = NULL;
    GstBus* bus = NULL;
    GstCaps * prop_caps = NULL;

    m_pPipeline = gst_pipeline_new("IaccRecord");
    if (!m_pPipeline)
    {
        GST_ERROR("Create IaccRecorder Pipeline Failed\n\n");
        goto exit;
    }

    m_pVideoAppSrc  = gst_element_factory_make("appsrc", "iacc-source");
    m_pVideoParse   = gst_element_factory_make("h264parse", "iacc-parser");
    m_pQtMuxer      = gst_element_factory_make("qtmux", "iacc-muxer");
    m_pSink         = gst_element_factory_make("splitmuxsink", "iacc-sink");
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

    g_signal_connect(m_pVideoAppSrc, "need-data", G_CALLBACK(OnIaccNeedData), this);

    g_object_set(G_OBJECT(m_pQtMuxer), "reserved-max-duration", (guint64)((guint64)30 * NANOSEC_TO_SEC), NULL);
    g_object_set(G_OBJECT(m_pQtMuxer), "reserved-bytes-per-sec", (guint32)(RECORD_FPS*(30 + sizeof(Ofilm_Can_Data_T)) + RESERVED_PER_SEC_PER_TRAK_SIZE), NULL);  // per second per trak reserve bytes
    g_object_set(G_OBJECT(m_pQtMuxer), "reserved-moov-update-period", (guint64)RESERVED_MOOV_UPDATE_INTERVAL, NULL); // update moov per 1 second
    g_object_set(G_OBJECT(m_pQtMuxer), "reserved-extra-bytes", (guint32)RESERVED_MOOV_EXTRA_BYTES, NULL);

    g_object_set(G_OBJECT(m_pSink), "muxer", m_pQtMuxer, NULL);
	g_object_set(G_OBJECT(m_pSink), "use-robust-muxing", TRUE, NULL);
    g_object_set(G_OBJECT(m_pSink), "max-size-time", (guint64)((guint64)60 * NANOSEC_TO_SEC), NULL);
    /* offer the new dest location */
    g_signal_connect(m_pSink, "format-location", G_CALLBACK(OnUpdateIaccRecDest), this);
    /* add tag */
    g_signal_connect(m_pQtMuxer, "tag-add", G_CALLBACK(OnUpdateIaccTagList), this);

    /* we add all elements into the pipeline */
    gst_bin_add_many(GST_BIN(m_pPipeline), m_pVideoAppSrc, m_pVideoParse, m_pSink, NULL);

    // link pipeline's element that has static pad
    link_ok = gst_element_link_many(m_pVideoAppSrc, m_pVideoParse, m_pSink, NULL);
    if (!link_ok) {
        goto exit;
    }

    bus = gst_pipeline_get_bus(GST_PIPELINE(m_pPipeline));
    gst_bus_set_sync_handler(bus, (GstBusSyncHandler)MessageHandler, this, NULL);
    g_object_set(G_OBJECT(m_pPipeline), "message-forward", TRUE, NULL);
    return DVR_RES_SOK;

exit:
    if (m_pPipeline)
        gst_object_unref(GST_OBJECT(m_pPipeline));
    return DVR_RES_EFAIL;
}

void CIaccRecorderEngine::OnIaccNeedData(GstElement *appsrc, guint unused_size, gpointer user_data)
{
    CIaccRecorderEngine *pThis = (CIaccRecorderEngine*)user_data;
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

gchar *CIaccRecorderEngine::OnUpdateIaccRecDest(GstElement * element, guint fragment_id, gpointer data)
{
    CIaccRecorderEngine *pThis = (CIaccRecorderEngine*)data;
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

	//if (pThis->m_pRecorderAsyncOp != NULL && thumb_name != NULL){
    //    pThis->m_pRecorderAsyncOp->RecorderAsyncOp_SndMsg(RECORDER_ASYNCOP_CMD_GENERATE_THUMBNAIL, (DVR_U32)thumb_name, 0);
    //}	

    pSetting->event_record_folder_max_file_index += 1;

    return dst;
}

gchar *CIaccRecorderEngine::OnUpdateIaccTagList(GstElement * element, guint fragment_id, gpointer data)
{
    CIaccRecorderEngine *pThis = (CIaccRecorderEngine*)data;

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
        pThis->m_pRecorderAsyncOp->RecorderAsyncOp_SndMsg(RECORDER_ASYNCOP_CMD_GENERATE_THUMBNAIL, (DVR_U32)pThis->m_CurEventRecThumbNailFileName, 3);
    }
    return NULL;
}


GstBusSyncReply CIaccRecorderEngine::MessageHandler(GstBus *bus, GstMessage *msg, gpointer pData)
{
    if (NULL == pData)
        return GST_BUS_PASS;

    CIaccRecorderEngine *pThis = (CIaccRecorderEngine *)pData;
    
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
                tFileInfo.thumbnail_location    = pThis->m_CurEventRecThumbNailFileName;
                tFileInfo.time_stamp            = time;
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
                tFileInfo.file_location         = location;
                tFileInfo.thumbnail_location    = pThis->m_CurEventRecThumbNailFileName;
                tFileInfo.time_stamp            = time;
                tFileInfo.eType                 = DVR_FOLDER_TYPE_DAS;
                GST_INFO("iacc file splitmuxsink-fragment-closed!!!!\n");
                if(pThis->m_Setting.eType == DVR_EVENTREC_SOURCE_TYPE_IACC_AEB)
                    tFileInfo.eType = DVR_FOLDER_TYPE_EMERGENCY;

                if (pThis->m_ptfnCallBackArray != NULL && pThis->m_ptfnCallBackArray->MessageCallBack != NULL)
                {
                    RECORDER_EVENT  eEvent = RECORDER_EVENT_NEW_FILE_FINISHED;
                    pThis->m_ptfnCallBackArray->MessageCallBack((void*)&eEvent, &tFileInfo, NULL, pThis->m_ptfnCallBackArray->pCallBackInstance);
               
                    eEvent = RECORDER_EVENT_IACC_COMPLETE;
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
                RECORDER_EVENT  eEvent = RECORDER_EVENT_EVENTREC_FATAL_ERROR;
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

int	CIaccRecorderEngine::Set(EVENT_RECORD_PROP_ID ePropId, void *pPropData, int nPropSize)
{
    switch (ePropId)
    {
        case EVENT_RECORD_PROP_DIR:
        CEventRecorderEngine::Set(ePropId, pPropData, nPropSize);
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

            DVR_U32 nMaxTime = pSetting->EventRecordLimitTime + 3*1000 + DEFAULT_DELAY_TIME;
            g_object_set(G_OBJECT(m_pSink), "max-size-time", (guint64)((guint64)nMaxTime * MICROSEC_TO_SEC), NULL);
            g_object_set(G_OBJECT(m_pQtMuxer), "reserved-max-duration", (guint64)((guint64)nMaxTime * MICROSEC_TO_SEC), NULL);
            g_object_set(G_OBJECT(m_pQtMuxer), "reserved-bytes-per-sec", (guint32)(RECORD_FPS*(30 + sizeof(Ofilm_Can_Data_T)) + RESERVED_PER_SEC_PER_TRAK_SIZE), NULL);  // per second per trak reserve bytes
        }
        break;

	    case EVENT_RECORD_PROP_FATAL_ERROR:
        CEventRecorderEngine::Set(ePropId, pPropData, nPropSize);
	    break;

	    case EVENT_RECORD_PROP_LIMITM_DELAY:
    	{
            if (pPropData == NULL || nPropSize != sizeof(DVR_U32))
    		{
    			GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
    			return DVR_RES_EINVALIDARG;
    		}

            g_mutex_lock(&m_nMutex);
            GST_INFO("iacc TimeLimit old (%lld) m_bUsedRecordTime %lld\n", m_postRecordTimeLimit, m_bUsedRecordTime);
    		m_postRecordTimeLimit = m_bUsedRecordTime + (guint64)((*(DVR_U32 *)pPropData) * GST_MSECOND);
            GST_INFO("iacc TimeLimit new (%lld)\n", m_postRecordTimeLimit);
            g_mutex_unlock(&m_nMutex);
    	}
	    break;

        default:
        break;
    }

    return DVR_RES_SOK;
}

