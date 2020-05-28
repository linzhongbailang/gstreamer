#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gst/video/gstvideosink.h>
#include <gst/pbutils/pbutils.h>
#include <gst/video/gstvideometa.h>
#include <gst/canbuf/canbuf.h>
#include <gst/dmabuf/dmabuf.h>
#include "PlaybackEngine.h"
#include "RecorderUtils.h"
#include "dprint.h"


#ifdef _MSC_VER
#pragma warning(disable:4996)	// 4706: assignment within conditional expression
#endif

#define VIDEO_PLAYBACK_NUM_FIRST_FRAMES_TO_SKIP		10
#define VIDEO_PLAYBACK_NUM_FRAMES_SKIP_AFTER_SEEK	5
#define VIDEO_PLAYBACK_RING_BUFFER_LEN              (3*1024*1024)
#define PHOTO_PLAYBACK_RING_BUFFER_LEN              (3*1024*1024)

static uchar startcode[H264_STARTCODE_SIZE] = {0x00, 0x00, 0x00, 0x01};
/*static char spspps[H264_COPY_HEAD_SIZE] = 
{0x00 ,0x00 ,0x00 ,0x01 ,0x27 ,0x64 ,0x00 ,0x33 ,0xAD ,0x84 ,0x05 ,0x45 ,0x62 ,0xB8 ,0xAC ,0x54 ,0x71 ,0x08 ,0x0A ,0x8A ,0xC5 ,0x71 ,0x58 ,0xA8 ,\
0xE2 ,0x10 ,0x24 ,0x85 ,0x21 ,0x39 ,0x3C ,0x9F ,0x27 ,0xE4 ,0xFE ,0x4F ,0xC9 ,0xF2 ,0x79 ,0xB9 ,0xB3 ,0x4D ,0x08 ,0x12 ,0x42 ,0x90 ,0x9C ,0x9E ,\
0x4F ,0x93 ,0xF2 ,0x7F ,0x27 ,0xE4 ,0xF9 ,0x3C ,0xDC ,0xD9 ,0xA6 ,0x17 ,0x2A ,0x00 ,0xA0 ,0x02 ,0xD6 ,0x40 ,0x00 ,0x00 ,0x00 ,0x01 ,0x28 ,0xFE ,\
0x01 ,0xAE ,0x2C, 0x00, 0x00, 0x00, 0x01};*/

uchar spspps[H264_COPY_HEAD_SIZE] = 
{0x00 ,0x00 ,0x00 ,0x01 ,0x27 ,0x64 ,0x00 ,0x33 ,0xAD ,0x84 ,0x05 ,0x45 ,0x62 ,0xB8 ,0xAC ,0x54 ,0x71 ,0x08 ,0x0A ,0x8A ,0xC5 ,0x71 ,0x58 ,0xA8 ,\
0xE2 ,0x10 ,0x24 ,0x85 ,0x21 ,0x39 ,0x3C ,0x9F ,0x27 ,0xE4 ,0xFE ,0x4F ,0xC9 ,0xF2 ,0x79 ,0xB9 ,0xB3 ,0x4D ,0x08 ,0x12 ,0x42 ,0x90 ,0x9C ,0x9E ,\
0x4F ,0x93 ,0xF2 ,0x7F ,0x27 ,0xE4 ,0xF9 ,0x3C ,0xDC ,0xD9 ,0xA6 ,0x17 ,0x2A ,0x00 ,0x80 ,0x02 ,0x86 ,0x40 ,0x00 ,0x00 ,0x00 ,0x01 ,0x28 ,0xFE ,\
0x01 ,0xAE ,0x2C, 0x00, 0x00, 0x00, 0x01};


gboolean FindElement(GstElement *pEle, gchar *s, gboolean silent, GstElement **ppFound = NULL);
char* MakeURI(char* pFile);

GST_DEBUG_CATEGORY_STATIC(playback_engine_category);  // define category (statically)
#define GST_CAT_DEFAULT playback_engine_category       // set as default

static Playback_Param  m_nPlayBackparam;

int Dvr_Sdk_Playback_SetCallback(Playback_Param* param)
{
    memcpy(&m_nPlayBackparam, param, sizeof(Playback_Param));
}

CPlaybackEngine::CPlaybackEngine()
{
	DVR_S32 res;
	
	GST_DEBUG_CATEGORY_INIT(playback_engine_category, "PLAYBACK_ENGINE", GST_DEBUG_FG_CYAN, "playback engine category");
	
	if (!gst_is_initialized())
	{
		GST_LOG("Gstreamer not initialized ... initializing");
		gst_init(NULL, NULL);
	}

	m_pPipeline = NULL;
	m_ptfnCallBackArray = NULL;
	m_pThreadURI = NULL;

	m_nSpeed = 1000; //1x
	m_nDirection = 1;
	m_pEOFThread = 0;
	m_bStopFastScan = TRUE;
	m_pFastScanThread = 0;

	m_bClosing = FALSE;

	g_mutex_init(&m_mutexStateLock);

	m_eEngineState = PLAYBACK_ENGINE_STATE_INVALID;
	m_bStateLocked = FALSE;
	m_eSinkMode = VIDEO_SINK_INVALID;

	m_nOperationTimeOut = 0;
	m_nOpenTimeOut = 0;

    m_bFirstFrame       = TRUE;
	m_bGetvdFrameInfo   = TRUE;
	m_bGetpicFrameInfo  = TRUE;
	
    m_pPhotoPlayer = new CPhotoPlaybackEngine(this);

	m_pThumbNail = new CThumbNail(this);
	m_pPhotoAsyncOp = new CPhotoAsyncOp;
	m_pPhotoEngine = new CPhotoEngine(this, m_pThumbNail);
	PHOTO_ASYNCOP_HANDLER PhotoAsyncOpHandler = { 0 };
	m_pPhotoEngine->SetDBFunction(CPlaybackEngine::AsyncAddPhotoToDB);
	PhotoAsyncOpHandler.FuncPrepare = CPlaybackEngine::AsyncWaitImgForPhoto;
	PhotoAsyncOpHandler.FuncHandle = m_pPhotoEngine->Photo_Process;
	PhotoAsyncOpHandler.FuncReturn = CPlaybackEngine::PhotoReturn;
	PhotoAsyncOpHandler.Command = PHOTO_ASYNCOP_CMD_TAKE_PHOTO;
	PhotoAsyncOpHandler.pContext = m_pPhotoEngine;
	m_pPhotoAsyncOp->AsyncOp_RegHandler(&PhotoAsyncOpHandler);		
	g_queue_init(&m_nFrameQueue);
	g_mutex_init(&m_nFrameMutex);
    g_mutex_init(&m_nOrigDataMutex);
	//CreateProcessTask();

	FreeRingbuffer(VIDEO_PLAYBACK_RING_BUFFER_LEN);
	res = DVR::OSA_tskCreate(&m_hThread, PlaybackProcessTaskEntry, this);
	if (DVR_FAILED(res))
		DPrint(DPRINT_ERR, "CPlaybackEngine <Init> Create Task failed = 0x%x\n", res);
    GST_INFO("CPlaybackEngine  initialized ");
}

CPlaybackEngine::~CPlaybackEngine()
{
    if (m_pPhotoPlayer)
    {
        delete m_pPhotoPlayer;
        m_pPhotoPlayer = NULL;
    }

	m_bClosing = TRUE;

	if (m_pEOFThread)
	{
		g_thread_join(m_pEOFThread);
		m_pEOFThread = 0;
	}

	DVR::OSA_tskDelete(&m_hThread);

	g_mutex_clear(&m_mutexStateLock);
	g_queue_clear(&m_nFrameQueue);
	g_mutex_clear(&m_nFrameMutex);
    g_mutex_clear(&m_nOrigDataMutex);
	delete m_pThumbNail;
	delete m_pPhotoAsyncOp;
	delete m_pPhotoEngine;

	
	
}

gboolean CPlaybackEngine::LockState(gboolean bNeedLockState, unsigned long ulLockPos)
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
				if (m_bClosing)
					return FALSE;
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

void CPlaybackEngine::UnlockState(gboolean bNeedLockState)
{
	if (bNeedLockState)
		m_bStateLocked = FALSE;
}

void CPlaybackEngine::OnQtDemuxPadAdded(GstElement *src, GstPad *new_pad, gpointer user_data)
{
	CPlaybackEngine* pThis = (CPlaybackEngine*)user_data;

	GstPad *sink_pad_video = gst_element_get_static_pad (pThis->m_pQueue, "sink");
	
    GstPadLinkReturn ret;
    GstCaps *new_pad_caps = NULL;
    GstStructure *new_pad_struct = NULL;
    const gchar *new_pad_type = NULL;
    GST_INFO("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));

    new_pad_caps = gst_pad_query_caps (new_pad, NULL);
    new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
    new_pad_type = gst_structure_get_name (new_pad_struct);

	if (g_str_has_prefix (new_pad_type, "video/x-h264")) 
	{
		ret = gst_pad_link (new_pad, sink_pad_video);

		if (GST_PAD_LINK_FAILED (ret)) 
		{
			GST_INFO (" Type is '%s' but link failed.\n", new_pad_type);
		} 
		else 
		{
			GST_INFO (" Link succeeded (type '%s').\n", new_pad_type);
		}
	} 
	else
	{
		GST_ERROR (" It has type '%s' which is not raw video. Ignoring.\n", new_pad_type);
	}

	 if (new_pad_caps != NULL)
		 gst_caps_unref (new_pad_caps);
	 gst_object_unref (sink_pad_video);

	return;
}

GstElement* CPlaybackEngine::CreatePipeline(void* pParam)
{
	gboolean bRes;
	
	if (!pParam)
	{
		return NULL;
	}

	m_pPipeline = gst_pipeline_new("DVR_Playback");
	if (!m_pPipeline)
	{
		GST_ERROR("[Engine] m_pPipeline is not initialized!\n");
		return NULL;
	}

	m_pSource = gst_element_factory_make("filesrc", "filesrc_name");
	m_pQtDemux = gst_element_factory_make("qtdemux", "qtdemux_name");
	m_pQueue = gst_element_factory_make("queue", "videoqueue");
	m_pParse = gst_element_factory_make("h264parse", "h264parse_name"); 
	//m_pVideoDec = gst_element_factory_make("ducatih264dec", "ti_ducatih264dec");
	m_pSink = gst_element_factory_make("appsink", "AppSink");
	
	if (!m_pSource || !m_pQtDemux || !m_pQueue || !m_pSink || /*!m_pVideoDec ||*/ ! m_pParse)
	{
		GST_ERROR("[Engine]Create One of Plugins failed!\n");
		return NULL;
	}

	g_object_set(G_OBJECT(m_pSource), "location", pParam, NULL);
	
	g_object_set(m_pSink, "emit-signals", TRUE, "sync", TRUE, NULL);
	g_signal_connect(m_pSink, "new-sample", G_CALLBACK(OnNewSampleFromSinkForVideo), this);
	
	gst_bin_add_many(GST_BIN(m_pPipeline), m_pSource, m_pQtDemux, m_pQueue, m_pParse,/* m_pVideoDec,*/ m_pSink, NULL);
	bRes = gst_element_link_many(m_pSource, m_pQtDemux, NULL);
	bRes |= gst_element_link_many(m_pQueue, m_pParse, /*m_pVideoDec,*/ m_pSink, NULL);
	if (!bRes)
	{
		GST_ERROR("[Engine] Link plugins Failed!!!\n");
		return NULL;
	}

	g_signal_connect(m_pQtDemux, "pad-added", (GCallback)OnQtDemuxPadAdded, this);

	return m_pPipeline;
}

GstElement* CPlaybackEngine::CreateBin(void* pParam)
{
	if (!pParam)
	{
		return NULL;
	}

	GST_LOG("%s", (char*)pParam);

	return CreatePlayBin((char*)pParam);
}

GstElement* CPlaybackEngine::CreatePlayBin(char* pParam)
{
	if (NULL == pParam)
	{
		return NULL;
	}

	GstElement* pBin = NULL;
	char* pUri = NULL;

	pBin = gst_element_factory_make("playbin", "playbin");
	if (NULL == pBin)
	{
		return NULL;
	}

	pUri = MakeURI((char*)pParam);
	if (pUri == NULL)
	{
		g_object_unref(pBin);
		return NULL;
	}
	GST_INFO("[Engine] Open URI: %s\n", pUri);
	g_object_set(pBin, "uri", pUri, NULL);

	if (pUri)
	{
		free(pUri);
		pUri = NULL;
	}

	return pBin;
}


int CPlaybackEngine::Open(gpointer pParam, glong cbParam, gboolean bLockState)
{
	int ret;
	GTimeVal timeval;
	GstClockTime t_0, t_1;
	GstClockTimeDiff t_diff;

    m_bFirstFrame       = TRUE;
	m_bGetvdFrameInfo   = TRUE;
	m_nVideoPlaybackSkipFrames = 0; //VIDEO_PLAYBACK_NUM_FIRST_FRAMES_TO_SKIP;
	m_bSeekIDRFrame =0;

	m_nOpenTimeOut = (m_nOperationTimeOut == 0) ? 9 * GST_SECOND : m_nOperationTimeOut*GST_MSECOND;
	GST_INFO("[Engine] m_nOpenTimeOut = %lld", m_nOpenTimeOut);

	g_get_current_time(&timeval);
	t_0 = GST_TIMEVAL_TO_TIME(timeval);

	if (m_pThreadURI)
		free(m_pThreadURI);
	{
		int uri_len = strlen((char *)pParam) + 16;
		m_pThreadURI = (char *)calloc(uri_len, sizeof(char));
		if (m_pThreadURI == NULL)
			return DVR_RES_EOUTOFMEMORY;
		strcpy(m_pThreadURI, (char *)pParam);
	}

	m_eSinkMode = VIDEO_SINK_DUCATI;

//build_pipeline:
	g_get_current_time(&timeval);
	t_1 = GST_TIMEVAL_TO_TIME(timeval);
	t_diff = t_1 - t_0;
	if (m_nOpenTimeOut <= t_diff)
		return DVR_RES_EFAIL;
	m_nOpenTimeOut -= t_diff;
	t_0 = t_1;
	GST_INFO("[Engine] m_nOpenTimeOut = %lld", m_nOpenTimeOut);

	ret = InternalOpen(pParam, cbParam, bLockState);

	g_get_current_time(&timeval);
	t_1 = GST_TIMEVAL_TO_TIME(timeval);
	t_diff = t_1 - t_0;
	if (m_nOpenTimeOut <= t_diff)
		return DVR_RES_EFAIL;
	m_nOpenTimeOut -= t_diff;
	t_0 = t_1;
	GST_INFO("[Engine] m_nOpenTimeOut = %lld", m_nOpenTimeOut);

    InitRingbuffer(VIDEO_PLAYBACK_RING_BUFFER_LEN);

    g_mutex_lock(&m_nFrameMutex);
    g_queue_foreach(&m_nFrameQueue, (GFunc)free, NULL);
    g_queue_clear(&m_nFrameQueue);
    g_mutex_unlock(&m_nFrameMutex);
	return ret;
}

int CPlaybackEngine::InternalOpen(gpointer pParam, glong cbParam, gboolean bLockState)
{
	if (!LockState(bLockState, __LINE__))
	{
		GST_ERROR("[Engine][Open][%4d] LockState() FAILED!\n", __LINE__);
		return DVR_RES_EFAIL;
	}

	GST_LOG("[Engine] ===> Open");

	GstStateChangeReturn ret;
	int Ret = DVR_RES_SOK;

	GstBus* bus = NULL;
	GstMessage *msg = NULL;

	if (pParam == NULL)
	{
		UnlockState(bLockState);
		return DVR_RES_EINVALIDARG;
	}

	if (!gst_is_initialized())
	{
		UnlockState(bLockState);
		GST_ERROR("[Engine] Gstreamer is not initialized!\n");
		return DVR_RES_EFAIL;
	}

#if 0
	m_pPipeline = CreateBin(pParam);
	if (m_pPipeline == NULL)
	{
		UnlockState(bLockState);
		GST_ERROR("[Engine] Create playbin2 failed!\n");
		return DVR_RES_EFAIL;
	}
	ChangeVideoSink(m_pPipeline);
#else
	m_pPipeline = CreatePipeline(pParam);
#endif

	bus = gst_pipeline_get_bus(GST_PIPELINE(m_pPipeline));

	gst_bus_set_sync_handler(bus, (GstBusSyncHandler)MessageHandler, this, NULL);

	ret = gst_element_set_state(m_pPipeline, GST_STATE_PAUSED);

	switch (ret)
	{
	case GST_STATE_CHANGE_FAILURE:
		Ret = DVR_RES_EFAIL;
		goto open_end;
		break;
	case GST_STATE_CHANGE_NO_PREROLL:
		GST_DEBUG("Livein Source!!");
		break;
	case GST_STATE_CHANGE_ASYNC:
		break;
	case GST_STATE_CHANGE_SUCCESS:
		break;
	default:
		break;
	}


	GST_LOG("[Engine]Waiting for ready to play ...");
	//wait for GST_MESSAGE_ASYNC_DONE or GST_MESSAGE_ERROR
	msg = gst_bus_timed_pop_filtered(bus, m_nOpenTimeOut, GstMessageType(GST_MESSAGE_ASYNC_DONE | GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
	if (msg != NULL)
	{
		GError *err;
		gchar *debug_info;

		switch (GST_MESSAGE_TYPE(msg))
		{
		case GST_MESSAGE_ERROR:
			gst_message_parse_error(msg, &err, &debug_info);
			GST_ERROR("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
			GST_ERROR("Debugging information: %s\n", debug_info ? debug_info : "none");
			g_clear_error(&err);
			g_free(debug_info);
			Ret = DVR_RES_EFAIL;
			goto open_end;
			break;
		case GST_MESSAGE_ASYNC_DONE:
			GST_INFO("[Engine] Receive ASync Done Message ...");
			if (GST_STATE(m_pPipeline) != GST_STATE_PAUSED)
			{
				Ret = DVR_RES_EFAIL;
				goto open_end;
			}
			break;
		default:
			/* We should not reach here because we only asked for ERRORs and EOS */
			GST_WARNING("Unexpected message received.\n");
			break;
		}
	}
	else
	{
		Ret = DVR_RES_EFAIL;
		goto open_end;
	}

	//FindElement(GST_ELEMENT(m_pPipeline), (gchar *)"ABABAB", FALSE);

	GST_LOG("[Engine]Waiting for ready to play ...done!!");
	//generate graph for debug, the file will be in GST_DEBUG_DUMP_DOT_DIR
	//GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(m_pPipeline), GST_DEBUG_GRAPH_SHOW_ALL, "player");

open_end:

	//free resource
	if (bus)
	{
		gst_object_unref(bus);
		bus = NULL;
	}
	if (msg)
	{
		gst_message_unref(msg);
		msg = NULL;
	}

	if (Ret != DVR_RES_SOK)
	{
		GST_ERROR("[Engine]Open Error. Free Resource.");
		if (m_pPipeline)
		{
			gst_element_set_state(m_pPipeline, GST_STATE_NULL);

			gst_object_unref(m_pPipeline);
			m_pPipeline = NULL;
		}

		m_eEngineState = PLAYBACK_ENGINE_STATE_INVALID;
	}

	UnlockState(bLockState);

	return Ret;
}

int CPlaybackEngine::InitRingbuffer(int len)
{
    if(m_nPlayBackparam.cachebuffer == NULL)
    {
        if(m_nPlayBackparam.PbInitRingbufferCb != NULL)
        {
            m_nPlayBackparam.cachebuffer = (uchar*)m_nPlayBackparam.PbInitRingbufferCb(len);
            m_nPlayBackparam.writepos    = 0;
            m_nPlayBackparam.elementSize = len;
            DPrint(DPRINT_ERR,"CPlaybackEngine InitRingbuffer  %p\n",m_nPlayBackparam.cachebuffer);
        }
    }
    return m_nPlayBackparam.cachebuffer == NULL ? DVR_RES_EFAIL : DVR_RES_SOK;
}

int CPlaybackEngine::FreeRingbuffer(int len)
{
	return DVR_RES_SOK;
	
    if(m_nPlayBackparam.cachebuffer != NULL)
    {
        if(m_nPlayBackparam.PbFreeRingbufferCb != NULL)
        {
            m_nPlayBackparam.PbFreeRingbufferCb((void*)m_nPlayBackparam.cachebuffer, len);
			DPrint(DPRINT_ERR,"CPlaybackEngine FreeRingbuffer  %p\n",m_nPlayBackparam.cachebuffer);
            m_nPlayBackparam.cachebuffer    = NULL;
            m_nPlayBackparam.elementSize    = 0;
            m_nPlayBackparam.writepos       = 0;
			
           
        }
    }
    return DVR_RES_SOK;
}

int CPlaybackEngine::ChangeVideoSink(GstElement* pPipeline)
{
	if (pPipeline == NULL)
	{
		return DVR_RES_EINVALIDARG;
	}

	GstElement* pBin = CreateVideoSinkBin();
	if (pBin != NULL)
	{
		g_object_set(GST_OBJECT(pPipeline), "video-sink", pBin, NULL);
	}
	else
	{
		GST_WARNING("[Engine]Create Video Sink bin failed! Video sink not changed!\n");
		return DVR_RES_EFAIL;
	}

	return DVR_RES_SOK;
}

#if 0
gint CPlaybackEngine::CreateProcessTask(void)
{
    pthread_t m_hThread;
    int res = pthread_create(&m_hThread, NULL, PlaybackProcessTaskEntry, this);
    if (DVR_FAILED(res))
    {
        GST_ERROR("CPlaybackEngine Create Task failed = 0x%x\n", res);
    }
    pthread_detach(m_hThread);
    return DVR_RES_SOK;
}
#endif
GstElement* CPlaybackEngine::CreateVideoSinkBin()
{
	GstElement *pBin = NULL, *pSink, *pVPE, *pVideoDec = NULL;
	GstPad *pPad, *pGhostPad;
	gboolean bRes;

	GstElement *pParse = NULL;
	const gchar *szParse = NULL;
	const gchar *pParseName = NULL;
	GstElement *pCapsFilter = NULL;
	const gchar *szCapsFilter = NULL;
	const gchar *pCapsFilterName = NULL;

	if (!gst_is_initialized())
	{
		GST_ERROR("Gstreamer is not initialized!\n");
		return NULL;
	}

    if (m_pPipeline == NULL)
    {
        GST_ERROR("Pipeline is NULL!\n");
        return NULL;
    }

	GST_LOG("[Engine] Create Video Sink Bin ...");

	if (m_eSinkMode == VIDEO_SINK_INVALID)
	{
		pSink = gst_element_factory_make("fakesink", "FakeSink");
		if (NULL == pSink)
		{
			GST_ERROR("[Engine]Create Plugin failed (fakesink)!\n");
			return NULL;
		}
		return pSink;
	}

	pBin = gst_bin_new("Video Sink Bin");
#ifdef WIN32
	//pSink = gst_element_factory_make("fakesink", "Windows Sink");
	pSink = gst_element_factory_make("appsink", "AppSink");
	pVideoDec = gst_element_factory_make("avdec_h264", "avdec_h264");
#else
	//pSink = gst_element_factory_make("waylandsink", "TI Wayland Sink");
	pSink = gst_element_factory_make("appsink", "AppSink");
	pVideoDec = gst_element_factory_make("ducatih264dec", "ti_ducatih264dec");
	//pVPE = gst_element_factory_make("vpe", "TI VPE");
#endif

	pParse = gst_element_factory_make("h264parse", "h264parse_name");
	pCapsFilter = gst_element_factory_make("capsfilter", "capsfilter_name");

	if (!pSink || !pVideoDec || ! pParse || !pCapsFilter)
	{
		GST_ERROR("[Engine]Create One of Plugins failed!\n");
		return NULL;
	}

	g_object_set(pSink, "emit-signals", TRUE, "sync", TRUE, NULL);
	g_signal_connect(pSink, "new-sample", G_CALLBACK(OnNewSampleFromSinkForVideo), this);

#ifdef WIN32
	gst_bin_add_many(GST_BIN(pBin), pParse, pCapsFilter, pVideoDec, pSink, NULL);
	bRes = gst_element_link_many(pParse, pCapsFilter, pVideoDec, pSink, NULL);
#else
	gst_bin_add_many(GST_BIN(pBin), pParse, pCapsFilter, pVideoDec, pSink, NULL);
	bRes = gst_element_link_many(pParse, pCapsFilter, pVideoDec, pSink, NULL);
#endif

	if (!bRes)
	{
		GST_ERROR("[Engine] Link plugins Failed!!!\n");
		return NULL;
	}

	pPad = gst_element_get_static_pad(pParse, "sink");

	pGhostPad = gst_ghost_pad_new("sink", pPad);
	gst_pad_set_active(pGhostPad, TRUE);
	bRes = gst_element_add_pad(pBin, pGhostPad);
	gst_object_unref(pPad);

	return pBin;
}

void CPlaybackEngine::PlaybackProcessTaskEntry(void *ptr)
{
    DVR_IO_FRAME *pFrame = NULL;
    CPlaybackEngine* p = (CPlaybackEngine*)ptr;

    while(1)
    {
        if(p->m_eEngineState != PLAYBACK_ENGINE_STATE_RUNNING)
        {
            g_usleep(40 * MILLISEC_TO_SEC);
            continue;
        }

        if (0 == g_queue_get_length(&p->m_nFrameQueue))
        {
            g_usleep(35 * MILLISEC_TO_SEC);
            continue;
        }


        g_mutex_lock(&p->m_nFrameMutex);
        pFrame = (DVR_IO_FRAME *)g_queue_pop_tail(&p->m_nFrameQueue);
        m_nPlayBackparam.PbVideoProcessCb(pFrame->pMattsImage, pFrame->nFramelength, (uint32_t)pFrame->pCanBuf);
        if(pFrame)
            free(pFrame);
        g_mutex_unlock(&p->m_nFrameMutex);
        if (1)
        {
            extern gdouble ms_time(void);
            static gdouble last_cap_time = ms_time();
            static gdouble last_frameRate_count_time = 0;
            static int last_frameCount = 0;
            static int frameCount = 0;
            gdouble cap_time = ms_time();
        
            if (cap_time - last_frameRate_count_time > 1000.0)
            {
                int framesInOneSecond = frameCount - last_frameCount;
                DPrint(DPRINT_ERR,"PB Frame Rate: %d\n", framesInOneSecond);
                last_frameRate_count_time = cap_time;
                last_frameCount = frameCount;
            }
        
            last_cap_time = cap_time;
            frameCount++;
        }        
        g_usleep(30 * MILLISEC_TO_SEC);
    }
}

int CPlaybackEngine::OnFrameProcess(DVR_IO_FRAME *pFrame, Playback_Param* pParam, DVR_FILE_TYPE type)
{
    if(pFrame == NULL || pParam == NULL)
        return DVR_RES_EFAIL;

    uint pos = pParam->writepos;
    if(pos > pParam->elementSize || pParam->cachebuffer == NULL)
    {
        printf("[ERROR]OnFrameProcess %d elementSize %d cachebuffer %p!!!!!!!!!!!!!!\n",pos,pParam->elementSize, pParam->cachebuffer);
        if(pParam->cachebuffer == NULL)
        {
            printf("pParam->cachebuffer ERROR---NULL!!!!!!!!!!!!!!\n");
            return DVR_RES_EFAIL;
        }
    }

    if(pFrame->pMattsImage == NULL)
    {
        printf("pParam->pMattsImage ERROR---NULL!!!!!!!!!!!!!!\n");
        return DVR_RES_EFAIL;
    }

    if(pFrame->nFramelength + pos > pParam->elementSize)
    {
        pos = 0;
        pParam->writepos = 0;
    }

    uchar* writebuffer = pParam->cachebuffer + pos;
    if(type == DVR_FILE_TYPE_VIDEO)
    {
    
        pParam->PbmemCacheInvCb((unsigned int) writebuffer, pFrame->nFramelength);
        if(pFrame->IsKeyFrame)
        {
            memcpy(writebuffer, spspps, H264_COPY_HEAD_SIZE);
            memcpy(writebuffer + H264_COPY_HEAD_SIZE, (void *)((uchar *)pFrame->pMattsImage + H264_COPY_HEAD_SIZE), pFrame->nFramelength - H264_COPY_HEAD_SIZE);
            pParam->writepos += pFrame->nFramelength;
        }
        else
        {
            memcpy(writebuffer, startcode, H264_STARTCODE_SIZE);
            memcpy(writebuffer + H264_STARTCODE_SIZE, (void *)((uchar *)pFrame->pMattsImage + H264_STARTCODE_SIZE), pFrame->nFramelength - H264_STARTCODE_SIZE);
            pParam->writepos +=  pFrame->nFramelength;
        }
    }
    else if(type == DVR_FILE_TYPE_IMAGE)
    {
        pParam->PbmemCacheInvCb((unsigned int) writebuffer, pFrame->nFramelength);
        memcpy(writebuffer, pFrame->pMattsImage, pFrame->nFramelength);
        pParam->writepos +=  pFrame->nFramelength;
    }

    pParam->PbmemCacheWbCb((unsigned int) writebuffer, (uint)((uchar *)writebuffer + pFrame->nFramelength));
    pFrame->pMattsImage = (void *)writebuffer;
    return DVR_RES_SOK;
}

int CPlaybackEngine::OnNewSampleFromSinkForVideo(GstElement *pSink,  gpointer user_data)
{
	CPlaybackEngine *pThis = (CPlaybackEngine*)user_data;
	GstSample *pSample;
	GstBuffer *buffer;
	void *phy_addr = NULL;
	GstCaps *caps;
    GstStructure *s;
	static gint width = 0, height = 0;
	gboolean res;
	gboolean bDoSkip = FALSE;
	int ret;

	if (pThis->m_eEngineState == PLAYBACK_ENGINE_STATE_RUNNING)
	{
	 	g_signal_emit_by_name(pSink, "pull-sample", &pSample);

		if (pThis->m_bGetvdFrameInfo == TRUE)
		{
			caps = gst_sample_get_caps(pSample);
			if (NULL != caps)
			{
				s = gst_caps_get_structure(caps, 0);
				if (s != NULL)
				{
					/* we need to get the final caps on the buffer to get the size */
					res = gst_structure_get_int(s, "width", &width);
					res |= gst_structure_get_int(s, "height", &height);
					if (!res) {
						g_print("could not get snapshot dimension\n");
					}
				}
			}

			pThis->m_bGetvdFrameInfo = FALSE;
		}
#if 0
		if(pThis->m_nVideoPlaybackSkipFrames > 0){
			pThis->m_nVideoPlaybackSkipFrames--;
		}

		bDoSkip = (pThis->m_nVideoPlaybackSkipFrames == 0) ? FALSE : TRUE;

#endif
		if ( pThis->m_ptfnCallBackArray != NULL && pThis->m_ptfnCallBackArray->RawVideoNewFrame != NULL)
		{
			buffer = gst_sample_get_buffer(pSample);

			GstVideoCropMeta *crop = gst_buffer_get_video_crop_meta(buffer);

            if (1)
			{
#if __linux__
                GstMapInfo map;
                gst_buffer_map(buffer, &map, GST_MAP_READ);
                phy_addr = map.data;
                gst_buffer_unmap(buffer, &map);
				if(pThis->m_bSeekIDRFrame==TRUE){
					
					if(!(GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT))){
						pThis->m_bSeekIDRFrame=FALSE;
						bDoSkip=FALSE;
					}
					else{
						bDoSkip=TRUE;
					}					
				}					
#endif
                if( (phy_addr != NULL) && (!bDoSkip) )
                {            
                    DVR_IO_FRAME *frame = (DVR_IO_FRAME *)malloc(sizeof(DVR_IO_FRAME));
                    memset(frame, 0, sizeof(DVR_IO_FRAME));
                    if (frame == NULL)
                    {
                        printf("malloc failed---------------\n");
                        return DVR_RES_EOUTOFMEMORY;
                    }
                    frame->pMattsImage = phy_addr;
                    frame->nMattsImageWidth = width;
                    frame->nMattsImageHeight = height;
                    frame->nFramelength = gst_buffer_get_size( buffer );
                    if (crop != NULL)
                    {
                        frame->crop.x = crop->x;
                        frame->crop.y = crop->y;
                        frame->crop.width = crop->width;
                        frame->crop.height = crop->height;
                    }
                    Ofilm_Can_Data_T *pCan = NULL;
                    GstMetaCanBuf *canbuf = NULL;
#ifdef __linux__ 				
                    canbuf = gst_buffer_get_can_buf_meta(buffer);
                    if(canbuf != NULL)
                    {
                        pCan = gst_can_buf_meta_get_can_data(canbuf);
                    }
#endif
                    frame->pCanBuf = (DVR_U8 *)pCan;
                    frame->nCanSize = sizeof(Ofilm_Can_Data_T);

                    if (GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT))
                        frame->IsKeyFrame = false;
                    else
                        frame->IsKeyFrame = true;

                    if (GST_BUFFER_PTS_IS_VALID (buffer))
                        frame->nTimeStamp = GST_BUFFER_PTS (buffer);
                    else
                        frame->nTimeStamp = GST_BUFFER_DTS (buffer);
                    pThis->OnFrameProcess(frame, &m_nPlayBackparam, DVR_FILE_TYPE_VIDEO);

                    /*DPrint(DPRINT_ERR,"image[0x%x, %d x %d], crop[x:%d, y:%d, w:%d, h:%d], can[0x%p, %d] nFramelength %d queue num %d\n",
                        frame->pMattsImage, frame->nMattsImageWidth, frame->nMattsImageHeight,
                        frame->crop.x, frame->crop.y, frame->crop.width, frame->crop.height,
                        frame->pCanBuf, frame->nCanSize,frame->nFramelength,g_queue_get_length(&pThis->m_nFrameQueue));*/
                    g_mutex_lock(&pThis->m_nFrameMutex);
                    g_queue_push_head(&pThis->m_nFrameQueue, frame);
                    g_mutex_unlock(&pThis->m_nFrameMutex);
                }
            }
            else
            {
                GST_ERROR("Invalid physical address");
            }
        }
        gst_sample_unref(pSample);
    }
    return DVR_RES_SOK;
}

int CPlaybackEngine::OnNewSampleFromSinkForPhoto(GstElement *pSink, gpointer user_data)
{
    CPlaybackEngine *pThis = (CPlaybackEngine*)user_data;
    GstSample *pSample;
    GstBuffer *buffer;
    GstCaps *caps;
    GstStructure *s;
    static gint width = 0, height = 0;
    gboolean res;
    int ret;
    GstMapInfo map;

    if (pThis->m_eEngineState == PLAYBACK_ENGINE_STATE_RUNNING)
    {
        g_signal_emit_by_name(pSink, "pull-sample", &pSample);

        if (pThis->m_bGetpicFrameInfo == TRUE)
        {
            caps = gst_sample_get_caps(pSample);
            if (NULL != caps)
            {
                s = gst_caps_get_structure(caps, 0);
                if (s != NULL)
                {
                    /* we need to get the final caps on the buffer to get the size */
                    res = gst_structure_get_int(s, "width", &width);
                    res |= gst_structure_get_int(s, "height", &height);
                    if (!res) {
                        g_print("could not get snapshot dimension\n");
                    }
                }
            }

            pThis->m_bGetpicFrameInfo = FALSE;
        }

        if (pThis->m_ptfnCallBackArray != NULL && pThis->m_ptfnCallBackArray->RawVideoNewFrame != NULL)
        {
            buffer = gst_sample_get_buffer(pSample);

            GstVideoCropMeta *crop = gst_buffer_get_video_crop_meta(buffer);

            if (gst_buffer_map(buffer, &map, GST_MAP_READ))
            {
                DVR_IO_FRAME frame;
                memset(&frame, 0, sizeof(DVR_IO_FRAME));

                frame.pMattsImage       = map.data;
                frame.nSingleViewWidth  = width;
                frame.nSingleViewHeight = height;
                frame.nFramelength = gst_buffer_get_size( buffer );
                if (crop != NULL)
                {
                    frame.crop.x = crop->x;
                    frame.crop.y = crop->y;
                    frame.crop.width = crop->width;
                    frame.crop.height = crop->height;
                }

                frame.pCanBuf = (DVR_U8 *)pThis->m_pPhotoPlayer->GetCanData();
                frame.nCanSize = sizeof(Ofilm_Can_Data_T);
                //printf("------------------------ ----frame.nFramelength %d frame.pCanBuf %p\n",frame.nFramelength,frame.pCanBuf);

                if(frame.nFramelength > 65536)
                {
                    pThis->OnFrameProcess(&frame, &m_nPlayBackparam, DVR_FILE_TYPE_IMAGE);
                    m_nPlayBackparam.PbPhotoProcessCb(frame.pMattsImage, frame.nFramelength, width, height, (uint32_t)frame.pCanBuf);
                }
                gst_buffer_unmap(buffer, &map);
            }
        }

        gst_sample_unref(pSample);
    }

    if (1)
    {
        extern gdouble ms_time(void);
        static gdouble last_cap_time = ms_time();
        static gdouble last_frameRate_count_time = 0;
        static int last_frameCount = 0;
        static int frameCount = 0;
        gdouble cap_time = ms_time();

        if (cap_time - last_frameRate_count_time > 1000.0)
        {
            int framesInOneSecond = frameCount - last_frameCount;
            GST_INFO("Frame Rate: %d\n", framesInOneSecond);
            last_frameRate_count_time = cap_time;
            last_frameCount = frameCount;
        }

        last_cap_time = cap_time;
        frameCount++;
    }

    return DVR_RES_SOK;
}

int CPlaybackEngine::Close(gboolean bLockState)
{
	int Ret;

	if (!LockState(bLockState, __LINE__))
	{
		GST_ERROR("[Engine][Close] LockState() FAILED!\n");
		return DVR_RES_EFAIL;
	}

	m_eSinkMode = VIDEO_SINK_INVALID;

	if (!gst_is_initialized() || m_pPipeline == NULL)
	{
		UnlockState(bLockState);
		return DVR_RES_EFAIL;
	}

	GST_LOG("[Engine] Close");

	Ret = SetStateWait(GST_STATE_NULL);

	m_bClosing = FALSE;

	if (m_pThreadURI != NULL)
	{
		free(m_pThreadURI);
		m_pThreadURI = NULL;
	}

	gst_object_unref(m_pPipeline);
	m_pPipeline = NULL;
	m_eEngineState = PLAYBACK_ENGINE_STATE_INVALID;

	UnlockState(bLockState);
	
	return Ret;
}

int CPlaybackEngine::SetFuncList(void* pList)
{
	m_ptfnCallBackArray = (DVR_PLAYER_CALLBACK_ARRAY*)pList;
	return DVR_RES_SOK;
}

int CPlaybackEngine::Run(gboolean bLockState)
{
	GstStateChangeReturn ret;

	if (!LockState(bLockState, __LINE__))
	{
		GST_ERROR("[Engine][Run] LockState() FAILED!\n");
		return DVR_RES_EFAIL;
	}

	if (!gst_is_initialized())
	{
		UnlockState(bLockState);
		GST_ERROR("Gstreamer is not initialized!\n");
		return DVR_RES_EFAIL;
	}

    if (m_pPipeline == NULL)
    {
        UnlockState(bLockState);
        GST_ERROR("Pipeline is NULL, maybe the file is not open!\n");
        return DVR_RES_EUNEXPECTED;
}

#if 0
	StopFastPlay();

	m_nDirection = 1;
	m_nSpeed = 1000;
#endif

	ret = gst_element_set_state(m_pPipeline, GST_STATE_PLAYING);

	if (m_nSpeed == 1000)
	{
		StopFastPlay();
		m_eEngineState = PLAYBACK_ENGINE_STATE_RUNNING;
	}
	else
	{
		FastScan(m_nDirection);
	}

	UnlockState(bLockState);
	return DVR_RES_SOK;
}

int CPlaybackEngine::Pause(gboolean bLockState /* = TRUE */)
{
	int Ret = DVR_RES_SOK;
	gboolean doPause = true;

	if (!LockState(bLockState, __LINE__))
	{
		GST_ERROR("[Engine][Pause] LockState() FAILED!\n");
		return DVR_RES_EFAIL;
	}

	if (!gst_is_initialized())
	{
		UnlockState(bLockState);
		GST_ERROR("Gstreamer is not initialized!\n");
		return DVR_RES_EFAIL;
	}

    if (m_pPipeline == NULL)
    {
        UnlockState(bLockState);
        GST_ERROR("Pipeline is NULL!\n");
        return DVR_RES_EUNEXPECTED;
    }

	StopFastPlay();

	{
		GstFormat fm = GST_FORMAT_TIME;
		gint64 len;
		gboolean b;
		gint64 pos = 0, pos_1 = 0;;

		b = gst_element_query_duration(m_pPipeline, fm, &len);
		b = gst_element_query_position(m_pPipeline, fm, &pos);

		if (len > 0)
		{
			if ((pos + 500 * GST_MSECOND) >= len)
				doPause = false;
		}
		else
		{
			g_usleep(100*MILLISEC_TO_SEC);

			b = gst_element_query_position(m_pPipeline, fm, &pos_1);
			if (pos_1 <= pos)
				doPause = false;
		}
	}

	if (doPause)
	{
		Ret = SetStateWait(GST_STATE_PAUSED);
		m_eEngineState = PLAYBACK_ENGINE_STATE_PAUSED;
	}
	else
	{
		Ret = DVR_RES_EFAIL;
	}

	UnlockState(bLockState);
	return Ret;
}

int CPlaybackEngine::Stop(gboolean bLockState)
{
    int Ret;

    if (!LockState(bLockState, __LINE__))
    {
        GST_ERROR("[Engine][Stop] LockState() FAILED!\n");
        return DVR_RES_EFAIL;
    }

    if (!gst_is_initialized())
    {
        UnlockState(bLockState);
        GST_ERROR("Gstreamer is not initialized!\n");
        return DVR_RES_EFAIL;
    }

    if (m_pPipeline == NULL)
    {
        UnlockState(bLockState);
        GST_ERROR("pipeline is NULL, maybe file has not open or has already close!\n");
        return DVR_RES_EUNEXPECTED;
    }

    StopFastPlay();

    Ret = SetStateWait(GST_STATE_PAUSED);

    m_nDirection = 1;
    m_nSpeed = 1000;

    g_mutex_lock(&m_nFrameMutex);
    g_queue_foreach(&m_nFrameQueue, (GFunc)free, NULL);
    g_queue_clear(&m_nFrameQueue);
    g_mutex_unlock(&m_nFrameMutex);
    //SetPosition(0, NULL, FALSE);

    m_eEngineState = PLAYBACK_ENGINE_STATE_STOPPED;

    FreeRingbuffer(VIDEO_PLAYBACK_RING_BUFFER_LEN);

    UnlockState(bLockState);
    return Ret;
}

int CPlaybackEngine::SetPosition(glong dwPos, gpointer pvRangePos, gboolean bLockState /* = TRUE */)
{
	if (!LockState(bLockState, __LINE__))
	{
		GST_ERROR("[Engine][Set Position] LockState() FAILED!\n");
		return DVR_RES_EFAIL;
	}

	if (!gst_is_initialized())
	{
		UnlockState(bLockState);
		GST_ERROR("Gstreamer is not initialized!\n");
		return DVR_RES_EFAIL;
	}	

    if (m_pPipeline == NULL)
    {
        UnlockState(bLockState);
        GST_ERROR("Pipeline is NULL!\n");
        return DVR_RES_EUNEXPECTED;
    }

	gint64 newPos = dwPos;
	GST_INFO("[Engine]new position: %lld ms\n", newPos);
	newPos *= 1000000;
	gboolean bl = gst_element_seek(m_pPipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, newPos, GST_SEEK_TYPE_SET, GST_CLOCK_TIME_NONE);
	if (!bl)
	{
		GST_ERROR("[Engine] -- Seek Failed --\n");
	}
	else
	{
		GST_INFO("[Engine] -- Seek Success --\n");
	}

	m_nVideoPlaybackSkipFrames = VIDEO_PLAYBACK_NUM_FRAMES_SKIP_AFTER_SEEK;
	m_bSeekIDRFrame = TRUE;
	
	UnlockState(bLockState);
	return bl ? DVR_RES_SOK : DVR_RES_EFAIL;
}

int CPlaybackEngine::GetPosition(glong* pdwPos, gboolean bLockState /* = FALSE */)
{
	if (!LockState(bLockState, __LINE__))
	{
		GST_ERROR("[Engine][Set Position] LockState() FAILED!\n");
		return DVR_RES_EFAIL;
	}

	if (!gst_is_initialized())
	{
		UnlockState(bLockState);
		GST_ERROR("Gstreamer is not initialized!\n");
		return DVR_RES_EFAIL;
	}

    if (m_pPipeline == NULL)
    {
        UnlockState(bLockState);
        GST_ERROR("Pipeline is NULL!\n");
        return DVR_RES_EUNEXPECTED;
    }

	if (m_pFastScanThread && m_eEngineState == PLAYBACK_ENGINE_STATE_FASTPLAY)
	{
		*pdwPos = m_nFastPlayPos;
		UnlockState(bLockState);
		return DVR_RES_SOK;
	}

	GstFormat fm = GST_FORMAT_TIME;
	gint64 pos = 0;

	gboolean bl = gst_element_query_position(m_pPipeline, fm, &pos);

	*pdwPos = (glong)(pos / 1000000);

	UnlockState(bLockState);
	return bl ? DVR_RES_SOK : DVR_RES_EFAIL;
}

int CPlaybackEngine::SetStateWait(GstState nextState)
{
	GstStateChangeReturn ret;

	if (!gst_is_initialized())
	{
		GST_ERROR("[Engine][%d]Gstreamer is not initialized!\n", __LINE__);
		return DVR_RES_EFAIL;
	}

    if (m_pPipeline == NULL)
    {
        GST_ERROR("[Engine][%d]Pipeline is NULL!\n", __LINE__);
        return DVR_RES_EUNEXPECTED;
    }

	ret = gst_element_set_state(m_pPipeline, nextState);
	if (ret == GST_STATE_CHANGE_ASYNC)
	{
		GstState state;
		GstState pending;
		GstClockTime timeout = 5 * GST_SECOND;
		if (nextState == GST_STATE_NULL && m_nOperationTimeOut)
			timeout = m_nOperationTimeOut * GST_MSECOND;
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

PLAYER_STATUS CPlaybackEngine::GetStatus()
{
	if (!gst_is_initialized())
	{
		GST_ERROR("[Engine][%d]Gstreamer is not initialized!\n", __LINE__);
		return PLAYER_STATUS_INVALID;
	}

    if (m_pPipeline == NULL)
    {
        GST_ERROR("[Engine][%d]Pipeline is NULL!\n", __LINE__);
        return PLAYER_STATUS_INVALID;
    }

	PLAYER_STATUS status;
	GstState state = GST_STATE_NULL;
	gst_element_get_state(m_pPipeline, &state, NULL, 4 * GST_SECOND);
	switch (state)
	{
	case GST_STATE_NULL:
	case GST_STATE_VOID_PENDING:
		status = PLAYER_STATUS_INVALID;
		break;
	case GST_STATE_PAUSED:
		status = PLAYER_STATUS_PAUSED;
		break;
	case GST_STATE_PLAYING:
		status = PLAYER_STATUS_RUNNING;
		break;
	case GST_STATE_READY:
		status = PLAYER_STATUS_STOPPED;
		break;
	}

	return status;
}

int CPlaybackEngine::Set(glong dwPropID, gpointer pInstanceData, glong cbInstanceData, void* pPropData, glong cbPropData)
{
	DVR_RESULT res = DVR_RES_SOK;
	
	switch (dwPropID)
	{
	case DVR_PLAYER_CFG_FAST_FORWARD_SPEED:
	{
		if (pPropData == NULL || cbPropData != sizeof(int))
		{
			GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		int nSpeed = *((int*)pPropData);
		if (nSpeed <= 0)
		{
			GST_ERROR("[Engine] (%d) Invalid Speed(0)!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}
		m_nSpeed = nSpeed;
	}
	break;

	case DVR_PLAYER_CFG_FAST_SCAN_DIRECTION:
	{
		if (pPropData == NULL || cbPropData != sizeof(int))
		{
			GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		int nDirection = *((int*)pPropData);
		if (nDirection == 0)
		{
			GST_ERROR("[Engine] (%d) Invalid Speed(0)!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		m_nDirection = nDirection;
	}
	break;

	case DVR_PLAYER_CFG_ROOT_DIRECTORY:
	{
		if (pPropData == NULL || cbPropData != sizeof(DVR_DISK_DIRECTORY))
		{
			GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		DVR_DISK_DIRECTORY *pDir = (DVR_DISK_DIRECTORY *)pPropData;

		if (m_pPhotoEngine)
			m_pPhotoEngine->Set(PHOTO_PROP_DIR, pDir->szPhotoDir, sizeof(pDir->szPhotoDir));
	}
	break;

	default:
		res = DVR_RES_EFAIL;
		break;
	}

	return res;
}

int CPlaybackEngine::Get(glong dwPropID, gpointer pInstanceData, glong cbInstanceData, gpointer pPropData, glong cbPropData, glong *pcbReturned)
{
	switch (dwPropID)
	{	
	case DVR_PLAYER_CFG_FAST_FORWARD_SPEED:
	{
		if (pPropData == NULL || cbPropData != sizeof(int))
		{
			GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}
		*((int*)pPropData) = (int)m_nSpeed;
		return DVR_RES_SOK;
	}
	case DVR_PLAYER_CFG_FAST_SCAN_DIRECTION:
	{
		if (pPropData == NULL || cbPropData != sizeof(int))
		{
			GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}
		*((int*)pPropData) = m_nDirection;
		return DVR_RES_SOK;
	}

	case DVR_PLAYER_CFG_PLAYER_STATUS:
	{
		if (pPropData == NULL || cbPropData != sizeof(PLAYER_STATUS))
		{
			GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		*(PLAYER_STATUS*)pPropData = GetStatus();
		return DVR_RES_SOK;
	}
	break;
	
	case DVR_PLAYER_CFG_MEDIA_INFO:
	{
		if (cbPropData != sizeof(DVR_MEDIA_INFO) || pPropData == NULL)
		{
			return DVR_RES_EINVALIDARG;
		}

		if (!gst_is_initialized())
		{
			GST_ERROR("Gstreamer is not initialized!\n");
			return DVR_RES_EFAIL;
		}

        if (m_pPipeline == NULL)
        {
            GST_ERROR("Pipeline is NULL!\n");
            return DVR_RES_EUNEXPECTED;
        }

		glong dwNumOfAudio, dwNumOfVideo, dwNumOfSubtitle;

		dwNumOfAudio = dwNumOfVideo = dwNumOfSubtitle = 0;

		memset(pPropData, 0, sizeof(DVR_MEDIA_INFO));
		DVR_MEDIA_INFO* pMediaInfo = (DVR_MEDIA_INFO*)pPropData;

		GstFormat fm = GST_FORMAT_TIME;
		gint64 len;
		gboolean b;

		b = gst_element_query_duration(m_pPipeline, fm, &len);

		glong dwDuration = (glong)(len / 1000000);
		pMediaInfo->u32Duration = dwDuration;

		if (dwDuration > 0)
		{
			pMediaInfo->u32Seekable = 1;
		}
	}
	break;

	default:
		return DVR_RES_EFAIL;
	}

	return DVR_RES_SOK;
}

static void print_one_tag(const GstTagList * list, const gchar * tag, gpointer user_data)
{
	int i, num;

	num = gst_tag_list_get_tag_size(list, tag);
	for (i = 0; i < num; ++i) {
		const GValue *val;

		/* Note: when looking for specific tags, use the gst_tag_list_get_xyz() API,
		 * we only use the GValue approach here because it is more generic */
		val = gst_tag_list_get_value_index(list, tag, i);
		if (G_VALUE_HOLDS_STRING(val)) {
			GST_INFO("\t%20s : %s\n", tag, g_value_get_string(val));
		}
		else if (G_VALUE_HOLDS_UINT(val)) {
			GST_INFO("\t%20s : %u\n", tag, g_value_get_uint(val));
		}
		else if (G_VALUE_HOLDS_DOUBLE(val)) {
			GST_INFO("\t%20s : %g\n", tag, g_value_get_double(val));
		}
		else if (G_VALUE_HOLDS_BOOLEAN(val)) {
			GST_INFO("\t%20s : %s\n", tag,
				(g_value_get_boolean(val)) ? "true" : "false");
		}
		else if (GST_VALUE_HOLDS_BUFFER(val)) {
			GstBuffer *buf = gst_value_get_buffer(val);
			//      guint buffer_size = gst_buffer_get_size (buf);

			//    g_print ("\t%20s : buffer of size %u\n", tag, buffer_size);
		}
		else if (GST_VALUE_HOLDS_DATE_TIME(val)) {
			GstDateTime *dt = (GstDateTime *)g_value_get_boxed(val);
			//      gchar *dt_str = gst_date_time_to_iso8601_string (dt);

			//      g_print ("\t%20s : %s\n", tag, dt_str);
			//     g_free (dt_str);
		}
		else {
			GST_INFO("\t%20s : tag of type '%s'\n", tag, G_VALUE_TYPE_NAME(val));
		}
	}
}

void CPlaybackEngine::OnStreamEOF()
{
	PLAYER_END_INFO tPlaybackEndInfo;
	switch (m_eEngineState)
	{
	case PLAYBACK_ENGINE_STATE_STOPPED:
		tPlaybackEndInfo.ePlayerStatus = PLAYER_STATUS_STOPPED;
		break;
	case PLAYBACK_ENGINE_STATE_PAUSED:
		tPlaybackEndInfo.ePlayerStatus = PLAYER_STATUS_PAUSED;
		break;
	case PLAYBACK_ENGINE_STATE_FASTPLAY:
		tPlaybackEndInfo.ePlayerStatus = PLAYER_STATUS_FASTPLAY;
		break;
	default:
		tPlaybackEndInfo.ePlayerStatus = PLAYER_STATUS_RUNNING;
		break;
	}
	tPlaybackEndInfo.s32PlaySpeed = m_nSpeed * m_nDirection;

#if 0
	m_nDirection = 1;
	m_nSpeed = 1000;
	m_eEngineState = PLAYBACK_ENGINE_STATE_RUNNING;
#endif

	if (m_ptfnCallBackArray != NULL && m_ptfnCallBackArray->MessageCallBack != NULL)
	{
		PLAYER_EVENT  eEvent = PLAYER_EVENT_PLAYBACK_END;
		m_ptfnCallBackArray->MessageCallBack((void*)&eEvent, &tPlaybackEndInfo, m_pThreadURI, m_ptfnCallBackArray->pCallBackInstance);
	}
}

//max speed: 32X
double CPlaybackEngine::CalculateSpeed(int nSpeed)
{
	double speed = 1.0;
	if (nSpeed > 32000)
	{
		nSpeed = 32000;
	}
	else if (nSpeed < -32000)
	{
		nSpeed = -32000;
	}

	speed = (double)nSpeed / 1000.0;

	return speed;
}

gpointer CPlaybackEngine::eof_thread_func(gpointer data)
{
	CPlaybackEngine *pThis = reinterpret_cast<CPlaybackEngine*>(data);
	pThis->l_eof_thread_func();
	return 0;
}

int CPlaybackEngine::l_eof_thread_func()
{
	OnStreamEOF();
	return DVR_RES_SOK;
}

gpointer CPlaybackEngine::fastscan_thread_func(gpointer data)
{
	CPlaybackEngine *pThis = reinterpret_cast<CPlaybackEngine*>(data);
	pThis->l_fastscan_thread_func();
	return 0;
}

int CPlaybackEngine::l_fastscan_thread_func()
{
	double speed = CalculateSpeed(m_nSpeed);

	GstFormat fm = GST_FORMAT_TIME;
	gint64 pos = 0, len = 0, curr_pos = 0;;
	gboolean bl;
	gint64 pos_step = (gint64)speed*m_nDirection*GST_SECOND / 2;
	GstSeekFlags flags = GST_SEEK_FLAG_NONE;

	bl = gst_element_query_duration(m_pPipeline, fm, &len);
	bl = gst_element_query_position(m_pPipeline, fm, &pos);

	if (!bl)
	{
		GST_ERROR("[Engine] (%d) Query Position Failed!.\n", __LINE__);
		return DVR_RES_EFAIL;
	}

	if (m_nDirection >= 0 && len > 0 && (pos + 2 * GST_SECOND) > len)
		return DVR_RES_SOK;

	GST_WARNING("[Engine] (%d) pos = %lld, len = %lld, pos_step = %lld\n", __LINE__, pos, len, pos_step);

	if (m_nDirection >= 0)
		flags = (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SNAP_AFTER);
	else
		flags = (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SNAP_BEFORE);

	speed = 1.0;

	m_nFastPlayPos = 0;

	while (m_bStopFastScan == FALSE)
	{
		pos += pos_step;

		if (!GST_IS_ELEMENT(m_pPipeline))
		{
			GST_ERROR("[%s][%d] m_pPipeline is invalid\n", __FUNCTION__, __LINE__);
			break;
		}
		if (m_eEngineState != PLAYBACK_ENGINE_STATE_FASTPLAY)
		{
			GST_ERROR("[%s][%d] m_eEngineState is invalid\n", __FUNCTION__, __LINE__);
			break;
		}

		if (pos < 0)
			pos = 0;

		bl = gst_element_query_position(m_pPipeline, fm, &curr_pos);
		if (bl && m_nDirection > 0 && len > 0)
		{
			if ((curr_pos + 500 * GST_MSECOND) > len)
			{
				g_usleep(G_USEC_PER_SEC / 2); //0.5 seconds
				continue;
			}
		}

		GST_WARNING("[Engine] (%d) pos = %lld, curr_pos = %lld\n", __LINE__, pos, curr_pos);

		gboolean res = gst_element_seek(m_pPipeline, speed, GST_FORMAT_TIME, flags, GST_SEEK_TYPE_SET, pos, GST_SEEK_TYPE_SET, GST_CLOCK_TIME_NONE);
		if (!res)
		{
			GST_ERROR("[Engine] -- Seek Failed -- pos: %lld\n", pos);
		}
		else
		{
			GST_INFO("[Engine] -- Seek Success -- pos: %lld\n", pos);
			m_nFastPlayPos = (pos / 1000000);
			if (m_nFastPlayPos <= 0 && m_nDirection == -1)
			{
				m_nFastPlayPos = 0 - 0xbaad;
				break;
			}
		}
		g_usleep(G_USEC_PER_SEC / 2); //0.5 seconds
	}

	if (m_nFastPlayPos == (0 - 0xbaad))
	{
		if (m_pEOFThread)
		{
			g_thread_join(m_pEOFThread);
			m_pEOFThread = 0;
		}
		m_pEOFThread = g_thread_new("fastbackward eof", (GThreadFunc)eof_thread_func, this);
	}

	return DVR_RES_SOK;
}

int CPlaybackEngine::StopFastPlay()
{
	if (!gst_is_initialized() || m_pPipeline == NULL)
	{
		return DVR_RES_EFAIL;
	}

	if (m_eEngineState != PLAYBACK_ENGINE_STATE_FASTPLAY)
		return DVR_RES_EFAIL;

	//stop fastscan thread
	m_bStopFastScan = TRUE;
	if (m_pFastScanThread)
	{
		GThread *pFastScanThread = m_pFastScanThread;
		m_pFastScanThread = 0;
		g_thread_join(pFastScanThread);
		return DVR_RES_SOK;
	}
	
	return DVR_RES_SOK;
}

int CPlaybackEngine::StartFastPlay()
{
	if (!gst_is_initialized())
	{
		GST_ERROR("[Engine] Gstreamer is not initialized!\n");
		return DVR_RES_EFAIL;
	}

    if (m_pPipeline == NULL)
    {
        GST_ERROR("[Engine] Pipeline is NULL!\n");
        return DVR_RES_EUNEXPECTED;
    }

#if 1
	m_bStopFastScan = FALSE;
	m_pFastScanThread = g_thread_new("fastscan", (GThreadFunc)fastscan_thread_func, this);
	return DVR_RES_SOK;
#else
	double speed = CalculateSpeed(m_nSpeed*m_nDirection);

	GstFormat fm = GST_FORMAT_TIME;
	gint64 pos = 0;
	gboolean bl;

	bl = gst_element_query_position (m_pPipeline, fm, &pos);
	if(!bl)
	{
		GST_ERROR("[Engine] (%d) Query Position Failed!.\n", __LINE__);
		return DVR_RES_EFAIL;
	}

	GstSeekFlags flags = (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE);

	gboolean res = 0;

	if(speed > 0)
	{
		res = gst_element_seek (m_pPipeline, speed, GST_FORMAT_TIME, flags, GST_SEEK_TYPE_SET, pos, GST_SEEK_TYPE_SET, GST_CLOCK_TIME_NONE);
	}
	else if(speed < 0)
	{
		res = gst_element_seek (m_pPipeline, speed, GST_FORMAT_TIME, flags, GST_SEEK_TYPE_SET, GST_CLOCK_TIME_NONE, GST_SEEK_TYPE_SET, pos);
	}

	return (!res ? DVR_RES_EFAIL : DVR_RES_SOK);
#endif
}

int CPlaybackEngine::FastPlay(gboolean bLockState)
{
	if (!LockState(bLockState, __LINE__))
	{
		GST_ERROR("[ERR][FastPlay] LockState() FAILED!\n");
		return DVR_RES_EFAIL;
	}

	if (GetStatus() == PLAYER_STATUS_INVALID)
	{
		UnlockState(bLockState);
		GST_ERROR("[Engine][%d] engine has NOT been INITIALIZED yet!\n", __LINE__);
		return DVR_RES_EFAIL;
	}
	
#if 0
	if(m_eEngineState == PLACKBACK_ENGINE_STATE_FASTPLAY)
	{
		GST_ERROR("[Engine] Already in fast play mode!");
		return DVR_RES_SOK;
	}
#endif

	GST_INFO("[Engine][%4d] ------------FastPlay-----------\n", __LINE__);

	int res = StartFastPlay();
	if (res == DVR_RES_SOK)
	{
		if (m_nSpeed == 1000)
		{
			m_eEngineState = PLAYBACK_ENGINE_STATE_RUNNING;
		}
		else
		{
			m_eEngineState = PLAYBACK_ENGINE_STATE_FASTPLAY;
		}
	}
	else
	{
		m_nSpeed = 1000;
	}

	UnlockState(bLockState);
	return res;
}

int CPlaybackEngine::FastScan(glong lDirection, gboolean bLockState)
{
	if (GetStatus() == PLAYER_STATUS_INVALID)
	{
		GST_ERROR("[Engine][%d] engine has NOT been INITIALIZED yet!\n", __LINE__);
		return DVR_RES_EFAIL;
	}

#if 0
	if(m_eEngineState == PLACKBACK_ENGINE_STATE_FASTPLAY)
	{
		GST_ERROR("[Engine] Already in fast play mode!");
		return DVR_RES_SOK;
	}
#endif
	if (lDirection == 0)
	{
		GST_ERROR("[Engine][%d] Invalid Direction(0).\n", __LINE__);
		return DVR_RES_EFAIL;
	}

	if (lDirection == 1 && m_nSpeed == 1000)
	{
		GST_ERROR("[Engine][%d] Should Call Dvr_Player_Play() Interface.\n", __LINE__);
		return DVR_RES_EFAIL;
	}

	StopFastPlay();

	GST_ERROR("[log][PBE][%4d] ------------FastPlay-----------\n", __LINE__);

	m_nDirection = lDirection;
	int res = StartFastPlay();
	if (res == DVR_RES_SOK)
	{
		m_eEngineState = PLAYBACK_ENGINE_STATE_FASTPLAY;
	}
	else
	{
		m_nSpeed = 1000;
	}

	return res;
}

GstBusSyncReply CPlaybackEngine::MessageHandler(GstBus *bus, GstMessage *msg, gpointer pData)
{
	if (NULL == pData)
	{
		return GST_BUS_PASS;
	}

	GError *err;
	gchar *debug_info;
	const gchar *detailed_message;

	CPlaybackEngine* pEngine = (CPlaybackEngine*)pData;

	GST_LOG("Element: %s; Msg: %s\n", GST_OBJECT_NAME(msg->src), GST_MESSAGE_TYPE_NAME(msg));

	switch (GST_MESSAGE_TYPE(msg))
	{
	case GST_MESSAGE_ELEMENT:
	{
		//TODO
	}
	break;
	case GST_MESSAGE_INFO:
		gst_message_parse_info(msg, &err, &debug_info);
		g_clear_error(&err);
		g_free(debug_info);
		break;
	case GST_MESSAGE_ERROR:
		gst_message_parse_error(msg, &err, &debug_info);
		GST_WARNING("[Engine] Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
		GST_WARNING("[Engine] Debugging information: %s\n", debug_info ? debug_info : "none");
		g_clear_error(&err);
		g_free(debug_info);
		break;
	case GST_MESSAGE_EOS:
		GST_INFO("[Engine] End-Of-Stream reached.\n");
		{
			pEngine->StopFastPlay();
			pEngine->OnStreamEOF();
		}
		break;

	case GST_MESSAGE_STREAM_STATUS:
	{
		GstStreamStatusType type;
		GstElement *pElement;
		gst_message_parse_stream_status(msg, &type, &pElement);
		break;
	}
	case GST_MESSAGE_TAG:
	{
		GstTagList *tags;
		gst_message_parse_tag(msg, &tags);
		{
			GST_LOG("tag: %s", gst_tag_list_to_string(tags));
		}
		gst_tag_list_free(tags);
		break;
	}
	case GST_MESSAGE_DURATION_CHANGED:
	{
		GstFormat fmt = GST_FORMAT_TIME;
		gint64 duration = 0;
		gst_element_query_duration(pEngine->m_pPipeline, fmt, &duration);
		break;
	}
	default:
	{
		detailed_message = GST_MESSAGE_TYPE_NAME(msg);
		break;
	}
	}

	/* We want to keep receiving messages */
	return GST_BUS_PASS;
}

int CPlaybackEngine::PhotoOpen(const char *fileName)
{
    int ret;

    if (m_pPhotoPlayer == NULL)
        return DVR_RES_EPOINTER;

	m_bGetpicFrameInfo = TRUE;

    ret = m_pPhotoPlayer->Open(fileName, OnNewSampleFromSinkForPhoto);

    InitRingbuffer(PHOTO_PLAYBACK_RING_BUFFER_LEN);

    return ret;
}

int CPlaybackEngine::PhotoClose()
{
    int ret;

    if (m_pPhotoPlayer == NULL)
        return DVR_RES_EPOINTER;

    ret = m_pPhotoPlayer->Close();
	
	m_eEngineState = PLAYBACK_ENGINE_STATE_INVALID;

    return ret;
}

int CPlaybackEngine::PhotoPlay()
{
    int ret;

    if (m_pPhotoPlayer == NULL)
        return DVR_RES_EPOINTER;

    ret = m_pPhotoPlayer->Start();
    if (ret == DVR_RES_SOK)
    {
        m_eEngineState = PLAYBACK_ENGINE_STATE_RUNNING;
    }

    return ret;
}

int CPlaybackEngine::PhotoStop()
{
    int ret;

    if (m_pPhotoPlayer == NULL)
        return DVR_RES_EPOINTER;

    ret = m_pPhotoPlayer->Stop();
    if (ret == DVR_RES_SOK)
    {
        m_eEngineState = PLAYBACK_ENGINE_STATE_STOPPED;
    }

    FreeRingbuffer(PHOTO_PLAYBACK_RING_BUFFER_LEN);

    return ret;
}

int CPlaybackEngine::FrameUpdate(DVR_IO_FRAME *pInputFrame)
{
    g_mutex_lock(&m_nOrigDataMutex);
    memcpy(&m_nPlayBackFrame, pInputFrame, sizeof(DVR_IO_FRAME));
    g_mutex_unlock(&m_nOrigDataMutex);

	
   	//DPrint(DPRINT_ERR, "FrameUpdate  pCanBuf:0x%p\n", m_nPlayBackFrame.pCanBuf);
    
    return DVR_RES_SOK;
}

int CPlaybackEngine::AsyncAddPhotoToDB(const char *pLocation, const char *pThumbNailLocation, void *pContext)
{
	CPlaybackEngine *pThis = (CPlaybackEngine *)pContext;
	DVR_ADDTODB_FILE_INFO tFileInfo;

	tFileInfo.file_location = pLocation;
    tFileInfo.thumbnail_location = pThumbNailLocation;
	tFileInfo.time_stamp = 0;
    tFileInfo.eType = DVR_FOLDER_TYPE_PHOTO;
	if (pThis->m_ptfnCallBackArray != NULL && pThis->m_ptfnCallBackArray->MessageCallBack != NULL)
	{
		PLAYER_EVENT  eEvent = PLAYER_EVENT_PRINT_SCREEN_ADD2DB;
		pThis->m_ptfnCallBackArray->MessageCallBack((void*)&eEvent, &tFileInfo, NULL, pThis->m_ptfnCallBackArray->pCallBackInstance);
	}

	return DVR_RES_SOK;
}

int CPlaybackEngine::PhotoReturn(DVR_U32 param1, DVR_U32 param2, void *pContext)
{
	CPlaybackEngine *pThis = (CPlaybackEngine *)((CPhotoEngine *)pContext)->ParentEngine();
	PHOTO_INPUT_PARAM *pPhotoParam = (PHOTO_INPUT_PARAM *)param1;
	
	if(pThis == NULL || pPhotoParam == NULL)
		return DVR_RES_EPOINTER;

	free(pPhotoParam);

	if (pThis->m_ptfnCallBackArray != NULL && pThis->m_ptfnCallBackArray->MessageCallBack != NULL)
	{
		PLAYER_EVENT	eEvent = PLAYER_EVENT_PRINT_SCREEN_DONE;
		pThis->m_ptfnCallBackArray->MessageCallBack((void*)&eEvent, NULL, NULL, pThis->m_ptfnCallBackArray->pCallBackInstance);
	}

	return DVR_RES_SOK;
}
int CPlaybackEngine::AsyncWaitImgForPhoto(DVR_U32 param1, DVR_U32 param2, void *pContext)
{
	DVR_IO_FRAME pIoFrame;
	DVR_IO_FRAME *pFrame = NULL;
	PHOTO_FORMAT_SETTING setting;
	CPlaybackEngine *pThis = (CPlaybackEngine *)((CPhotoEngine *)pContext)->ParentEngine();
	PHOTO_INPUT_PARAM *pPhotoParam = (PHOTO_INPUT_PARAM *)param1;
	//PHOTO_INPUT_PARAM PhotoParamTemp;
	if(pThis == NULL || pPhotoParam == NULL)
		return DVR_RES_EPOINTER;


	
	
    memset(&pIoFrame, 0, sizeof(DVR_IO_FRAME));
    g_mutex_lock(&pThis->m_nOrigDataMutex);
    pFrame = &pIoFrame;
    memcpy(pFrame, &pThis->m_nPlayBackFrame, sizeof(DVR_IO_FRAME));
    g_mutex_unlock(&pThis->m_nOrigDataMutex);
	setting.format = GST_VIDEO_FORMAT_NV12;
	setting.width = pFrame->crop.width;
	setting.height = pFrame->crop.height;
	for (int i = 0; i < 4; i++)
	{
		pPhotoParam->pImageBuf[i] = pFrame->pImageBuf[i];
	}

	pPhotoParam->width = pFrame->crop.width;
	pPhotoParam->height = pFrame->crop.height;

    Ofilm_Can_Data_T *pCanData = (Ofilm_Can_Data_T *)pFrame->pCanBuf;

	//DPrint(DPRINT_ERR, "CPlaybackEngine::AsyncWaitImgForPhoto 0x%p CPlaybackEngine:0x%p index:%d  pCanData:0x%p\n", param1,pThis,pPhotoParam->eIndex,pCanData);
	
	//OSA_Sleep(1000);
    //if (pCanData != NULL)
	if (0)
	{
		pPhotoParam->stVehInfo.TimeYear = pCanData->vehicle_data.year;
		pPhotoParam->stVehInfo.TimeMon = pCanData->vehicle_data.month;
		pPhotoParam->stVehInfo.TimeDay = pCanData->vehicle_data.day;
		pPhotoParam->stVehInfo.TimeHour = pCanData->vehicle_data.hour;
		pPhotoParam->stVehInfo.TimeMin = pCanData->vehicle_data.minute;
		pPhotoParam->stVehInfo.TimeSec = pCanData->vehicle_data.second;
		pPhotoParam->stVehInfo.VehicleSpeed = pCanData->vehicle_data.vehicle_speed;
		pPhotoParam->stVehInfo.GearShiftPositon = pCanData->vehicle_data.vehicle_movement_state;
		pPhotoParam->stVehInfo.BrakePedalStatus = pCanData->vehicle_data.Brake_Pedal_Position;
		pPhotoParam->stVehInfo.DriverBuckleSwitchStatus = pCanData->vehicle_data.DriverBuckleSwitchStatus;
		pPhotoParam->stVehInfo.AccePedalPosition = pCanData->vehicle_data.Accelerator_Actual_Position;
		pPhotoParam->stVehInfo.TurnSignal = pCanData->vehicle_data.turn_signal;
		pPhotoParam->stVehInfo.GpsLongitude = pCanData->vehicle_data.positioning_system_longitude;
		pPhotoParam->stVehInfo.GpsLatitude = pCanData->vehicle_data.positioning_system_latitude;
	}

	pThis->m_pPhotoEngine->Set(PHOTO_PROP_FORMAT, &setting, sizeof(PHOTO_FORMAT_SETTING));

	return DVR_RES_SOK;
}


int	CPlaybackEngine::PrintScreen(DVR_PHOTO_PARAM *pParam)
{
	PHOTO_INPUT_PARAM *pPhotoParam = (PHOTO_INPUT_PARAM *)malloc(sizeof(PHOTO_INPUT_PARAM));
	if(pPhotoParam == NULL)
		return DVR_RES_EOUTOFMEMORY;

	memset(pPhotoParam, 0, sizeof(PHOTO_INPUT_PARAM));
    pPhotoParam->eType = pParam->eType;
    pPhotoParam->eQuality = pParam->eQuality;
    pPhotoParam->eIndex = pParam->eIndex;
	
	//DPrint(DPRINT_ERR, "CPlaybackEngine::PrintScreen pPhotoParam:0x%p  m_pPhotoEngine:0X%p this:0x%p\n", pPhotoParam,m_pPhotoEngine,this);
	//OSA_Sleep(1000);
	
	return m_pPhotoAsyncOp->AsyncOp_SndMsg(PHOTO_ASYNCOP_CMD_TAKE_PHOTO, (DVR_U32)pPhotoParam, 0);	
}

