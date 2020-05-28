#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib/gstdio.h>
#include <gio/gio.h>

#include <gst/video/gstvideosink.h>
#include <gst/pbutils/pbutils.h>
#include <gst/canbuf/canbuf.h>
#include <gst/dmabuf/dmabuf.h>
#include "RecorderEngine.h"
#include "ApaRecorderEngine.h"
#include "IaccRecorderEngine.h"
#include "EventRecorderEngine.h"
#include "LoopRecorderEngine.h"
#include "RecorderUtils.h"
#include "PhotoEngine.h"
#include "ThumbNail.h"
#include "VideoBuffer.h"
#include "Overlay.h"
#include "OsdTitle.h"
#include "OsdString.h"
#include "BMP.h"

#ifndef WIN32
#include <sys/sysinfo.h>
#endif


//#define FEATURE_RECORD_ASYNCOP_ENABLE

#ifdef _MSC_VER
#pragma warning(disable:4996)	// 4706: assignment within conditional expression
#endif

GST_DEBUG_CATEGORY_STATIC(recorder_engine_category);  // define category (statically)
#define GST_CAT_DEFAULT recorder_engine_category       // set as default

gdouble ms_time(void);

#define SAFE_DELETE(x) { if (x) delete x; x = 0; }
CRecorderEngine::CRecorderEngine()
{
	GST_DEBUG_CATEGORY_INIT(recorder_engine_category, "RECORDER_ENGINE", GST_DEBUG_FG_BLUE, "recorder engine category");

	if (!gst_is_initialized())
	{
		GST_LOG("Gstreamer not initialized ... initializing");
		gst_init(NULL, NULL);
	}

    memset(&m_nConfig, 0, sizeof(RecordConfig));

	g_queue_init(&m_nFrameQueue);
	g_mutex_init(&m_nFrameMutex);
	m_eEngineState = RECORDER_ENGINE_STATE_INVALID;
    m_last_time    = 0;
    m_basetime     = 0;
	m_inputCount=0;

	m_ptfnCallBackArray = NULL;

	m_pRecorderAsyncOp = new RecorderAsyncOp;

	m_pThumbNail = new CThumbNail(this);
	RECORDER_ASYNCOP_HANDLER RecAsyncOpHandler = { 0 };
    RecAsyncOpHandler.FuncPrepare = CRecorderEngine::AsyncWaitImgForVideo;
	RecAsyncOpHandler.FuncHandle = m_pThumbNail->ThumbNail_Process;
	RecAsyncOpHandler.FuncReturn = CRecorderEngine::VideoAddThumbNail;
	RecAsyncOpHandler.Command = RECORDER_ASYNCOP_CMD_GENERATE_THUMBNAIL;
	RecAsyncOpHandler.pContext = m_pThumbNail;
	m_pRecorderAsyncOp->RecorderAsyncOp_RegHandler(&RecAsyncOpHandler);

    RecAsyncOpHandler.FuncPrepare   = NULL;
    RecAsyncOpHandler.FuncHandle    = CRecorderEngine::VideoAddThumbNail;
    RecAsyncOpHandler.FuncReturn    = NULL;
    RecAsyncOpHandler.Command       = RECORDER_ASYNCOP_CMD_ADD_TAG_LIST;
    RecAsyncOpHandler.pContext      = m_pThumbNail;
    m_pRecorderAsyncOp->RecorderAsyncOp_RegHandler(&RecAsyncOpHandler);

    RecAsyncOpHandler.FuncPrepare = NULL;
	RecAsyncOpHandler.FuncHandle = CRecorderEngine::AsyncStartLoopRec;
	RecAsyncOpHandler.FuncReturn = NULL;
	RecAsyncOpHandler.Command = RECORDER_ASYNCOP_CMD_START_LOOP_RECORDER;
	RecAsyncOpHandler.pContext = this;
	m_pRecorderAsyncOp->RecorderAsyncOp_RegHandler(&RecAsyncOpHandler);	

    RecAsyncOpHandler.FuncPrepare = NULL;
	RecAsyncOpHandler.FuncHandle = CRecorderEngine::AsyncStopLoopRec;
	RecAsyncOpHandler.FuncReturn = NULL;
	RecAsyncOpHandler.Command = RECORDER_ASYNCOP_CMD_STOP_LOOP_RECORDER;
	RecAsyncOpHandler.pContext = this;
	m_pRecorderAsyncOp->RecorderAsyncOp_RegHandler(&RecAsyncOpHandler);	

    RecAsyncOpHandler.FuncPrepare = NULL;
	RecAsyncOpHandler.FuncHandle = CRecorderEngine::AsyncStartEventRec;
	RecAsyncOpHandler.FuncReturn = NULL;
	RecAsyncOpHandler.Command = RECORDER_ASYNCOP_CMD_START_EVENT_RECORDER;
	RecAsyncOpHandler.pContext = this;
	m_pRecorderAsyncOp->RecorderAsyncOp_RegHandler(&RecAsyncOpHandler);	

    RecAsyncOpHandler.FuncPrepare = NULL;
	RecAsyncOpHandler.FuncHandle = CRecorderEngine::AsyncStopEventRec;
	RecAsyncOpHandler.FuncReturn = NULL;
	RecAsyncOpHandler.Command = RECORDER_ASYNCOP_CMD_STOP_EVENT_RECORDER;
	RecAsyncOpHandler.pContext = this;
	m_pRecorderAsyncOp->RecorderAsyncOp_RegHandler(&RecAsyncOpHandler);	

    RecAsyncOpHandler.FuncPrepare = NULL;
    RecAsyncOpHandler.FuncHandle = CRecorderEngine::AsyncStartIaccRec;
    RecAsyncOpHandler.FuncReturn = NULL;
    RecAsyncOpHandler.Command = RECORDER_ASYNCOP_CMD_START_IACC_RECORDER;
    RecAsyncOpHandler.pContext = this;
    m_pRecorderAsyncOp->RecorderAsyncOp_RegHandler(&RecAsyncOpHandler);

    RecAsyncOpHandler.FuncPrepare = NULL;
	RecAsyncOpHandler.FuncHandle = CRecorderEngine::AsyncStopIaccRec;
	RecAsyncOpHandler.FuncReturn = NULL;
	RecAsyncOpHandler.Command = RECORDER_ASYNCOP_CMD_STOP_IACC_RECORDER;
	RecAsyncOpHandler.pContext = this;
	m_pRecorderAsyncOp->RecorderAsyncOp_RegHandler(&RecAsyncOpHandler);	

    RecAsyncOpHandler.FuncPrepare = NULL;
    RecAsyncOpHandler.FuncHandle = CRecorderEngine::AsyncStartDasRec;
    RecAsyncOpHandler.FuncReturn = NULL;
    RecAsyncOpHandler.Command = RECORDER_ASYNCOP_CMD_START_DAS_RECORDER;
    RecAsyncOpHandler.pContext = this;
    m_pRecorderAsyncOp->RecorderAsyncOp_RegHandler(&RecAsyncOpHandler);

    RecAsyncOpHandler.FuncPrepare = NULL;
    RecAsyncOpHandler.FuncHandle = CRecorderEngine::AsyncStopDasRec;
    RecAsyncOpHandler.FuncReturn = NULL;
    RecAsyncOpHandler.Command = RECORDER_ASYNCOP_CMD_STOP_DAS_RECORDER;
    RecAsyncOpHandler.pContext = this;
    m_pRecorderAsyncOp->RecorderAsyncOp_RegHandler(&RecAsyncOpHandler);

	m_pPhotoAsyncOp = new CPhotoAsyncOp;
	m_pPhotoEngine = new CPhotoEngine(this, m_pThumbNail);
	PHOTO_ASYNCOP_HANDLER PhotoAsyncOpHandler = { 0 };
	m_pPhotoEngine->SetDBFunction(CRecorderEngine::AsyncAddPhotoToDB);
	PhotoAsyncOpHandler.FuncPrepare = CRecorderEngine::AsyncWaitImgForPhoto;
	PhotoAsyncOpHandler.FuncHandle = m_pPhotoEngine->Photo_Process;
	PhotoAsyncOpHandler.FuncReturn = CRecorderEngine::PhotoReturn;
	PhotoAsyncOpHandler.Command = PHOTO_ASYNCOP_CMD_TAKE_PHOTO;
	PhotoAsyncOpHandler.pContext = m_pPhotoEngine;
	m_pPhotoAsyncOp->AsyncOp_RegHandler(&PhotoAsyncOpHandler);

    m_pLoopRecordEngine = new CLoopRecorderEngine(m_pRecorderAsyncOp);
    m_pLoopRecordEngine->Open();

    m_pEventRecordEngine = NULL;
    m_pApaRecordEngine  = NULL;
    m_pIaccRecordEngine = NULL;
    m_pVideoBuffer=NULL;
    m_pVideoBuffer = new CVideoBuffer();
    m_pVideoBuffer->Create();
    gboolean bEventRecordEnable = TRUE;
    if (bEventRecordEnable){
        m_pEventRecordEngine = new CEventRecorderEngine(m_pRecorderAsyncOp, m_pVideoBuffer);
        m_pEventRecordEngine->Open();
    }else{
        m_pEventRecordEngine = NULL;
    }

#ifdef DVR_FEATURE_V302
    gboolean bDasRecordEnable = TRUE;
#else
    gboolean bDasRecordEnable = FALSE;
#endif
    if (bDasRecordEnable){
        m_pApaRecordEngine   = new CApaRecorderEngine(m_pRecorderAsyncOp);
        m_pApaRecordEngine->Open();
    }else{
        m_pApaRecordEngine  = NULL;
    }

    gboolean bIaccRecordEnable = TRUE;
    if (bIaccRecordEnable){
        m_pIaccRecordEngine = new CIaccRecorderEngine(m_pRecorderAsyncOp, m_pVideoBuffer);
        m_pIaccRecordEngine->Open();
    }else{
        m_pIaccRecordEngine = NULL;
    }

	
    m_pOsdTitie = new COSDTitleEx;
    //CreateMonitorTask();

	DVR_S32	res = DVR::OSA_tskCreate(&m_hThread, RecMonitorTaskEntry, this);
	if (DVR_FAILED(res))
		DPrint(DPRINT_ERR, "CPlaybackEngine <Init> Create Task failed = 0x%x\n", res);
}

CRecorderEngine::~CRecorderEngine()
{
	DVR::OSA_tskDelete(&m_hThread);

	g_queue_clear(&m_nFrameQueue);
	g_mutex_clear(&m_nFrameMutex);

	SAFE_DELETE(m_pThumbNail);
	SAFE_DELETE(m_pRecorderAsyncOp);
	SAFE_DELETE(m_pPhotoAsyncOp);
	SAFE_DELETE(m_pPhotoEngine);
    SAFE_DELETE(m_pEventRecordEngine);
    SAFE_DELETE(m_pLoopRecordEngine);
    SAFE_DELETE(m_pApaRecordEngine);
    SAFE_DELETE(m_pIaccRecordEngine);
    SAFE_DELETE(m_pOsdTitie);
    SAFE_DELETE(m_pVideoBuffer);

	
	
}

int CRecorderEngine::RecvBitStreamer(DVR_IO_FRAME *pInputFrame)
{
    GstBuffer *buffer = NULL;
    GstMetaCanBuf *pCanMeta = NULL;
	

    if(pInputFrame == NULL || pInputFrame->nFramelength == 0)
    {
        GST_ERROR("pInputFrame %p nFramelength %d\n",pInputFrame, pInputFrame->nFramelength);
        return DVR_RES_EFAIL;
    }

    buffer = gst_buffer_new_allocate(NULL, pInputFrame->nFramelength, NULL);
    gst_buffer_fill(buffer, 0, (guchar *)pInputFrame->pMattsImage, pInputFrame->nFramelength );
    if(m_basetime == 0)
    {
        m_basetime = pInputFrame->nTimeStamp;
		m_inputCount =0;
    }
	
	//GST_ERROR("pInputFrame->nTimeStamp %lld",pInputFrame->nTimeStamp);
    //GST_BUFFER_PTS(buffer) = (pInputFrame->nTimeStamp - m_basetime)*MILLISEC_TO_SEC;
    GST_BUFFER_PTS(buffer) = (41666*m_inputCount)*MILLISEC_TO_SEC;
	m_inputCount++;
    GST_BUFFER_DTS(buffer) = GST_BUFFER_PTS(buffer);
    GST_BUFFER_DURATION(buffer) = GST_CLOCK_TIME_NONE;
    GST_BUFFER_OFFSET(buffer) = GST_BUFFER_OFFSET_NONE;
    GST_BUFFER_OFFSET_END(buffer) = GST_BUFFER_OFFSET_NONE;

    if(pInputFrame->IsKeyFrame)
        GST_BUFFER_FLAG_UNSET(buffer, GST_BUFFER_FLAG_DELTA_UNIT);
    else
        GST_BUFFER_FLAG_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT);
    pCanMeta = gst_buffer_add_can_buf_meta(GST_BUFFER(buffer), pInputFrame->pCanBuf, pInputFrame->nCanSize);
    if (!pCanMeta){
        GST_ERROR("Failed to add can meta to buffer");
        //TODO:Free the meta
    }

    if (buffer != NULL)
    {
        if (m_pLoopRecordEngine != NULL)
        {
            m_pLoopRecordEngine->AddVideoFrame(gst_buffer_ref(buffer));
        }

        if (m_pApaRecordEngine != NULL)
        {
            m_pApaRecordEngine->AddVideoFrame(gst_buffer_ref(buffer));
        }

        if (m_pEventRecordEngine != NULL)
        {
            m_pEventRecordEngine->AddVideoFrame(gst_buffer_ref(buffer));
        }

        if (m_pIaccRecordEngine != NULL)
        {
            m_pIaccRecordEngine->AddVideoFrame(gst_buffer_ref(buffer));
        }
        
        if (m_pVideoBuffer != NULL)
        {
            m_pVideoBuffer->AddVideoFrame(gst_buffer_ref(buffer));
        }
    }
    gst_buffer_unref(buffer);
    return DVR_RES_SOK;
}

int CRecorderEngine::AddFrame(DVR_IO_FRAME *pInputFrame)
{
	DVR_IO_FRAME *pFrame;

	memcpy(&m_nInFrameForPhoto, pInputFrame, sizeof(DVR_IO_FRAME));
 	if (m_eEngineState != RECORDER_ENGINE_STATE_RUNNING)
 		return DVR_RES_SOK;

	pFrame = (DVR_IO_FRAME *)malloc(sizeof(DVR_IO_FRAME));
	if (pFrame == NULL)
	{
		GST_ERROR("malloc failed\n");
		return DVR_RES_EOUTOFMEMORY;
	}

	memcpy(pFrame, pInputFrame, sizeof(DVR_IO_FRAME));
	g_mutex_lock(&m_nFrameMutex);
	g_queue_push_head(&m_nFrameQueue, pFrame);
    g_mutex_unlock(&m_nFrameMutex);

    return RecvBitStreamer(pInputFrame);
}

int CRecorderEngine::SetFuncList(void* pList)
{
	m_ptfnCallBackArray = (DVR_RECORDER_CALLBACK_ARRAY*)pList;

    if (m_pLoopRecordEngine != NULL)
    {
        m_pLoopRecordEngine->SetFuncList(pList);
    }

    if (m_pEventRecordEngine != NULL)
    {
        m_pEventRecordEngine->SetFuncList(pList);
    }

    if (m_pApaRecordEngine != NULL)
    {
        m_pApaRecordEngine->SetFuncList(pList);
    }

    if (m_pIaccRecordEngine != NULL)
    {
        m_pIaccRecordEngine->SetFuncList(pList);
    }

	return DVR_RES_SOK;
}



void CRecorderEngine::RecMonitorTaskEntry(void *ptr)
{
    DVR_IO_FRAME *pFrame = NULL;
    static gboolean need_recovery = false;
    CRecorderEngine* p = (CRecorderEngine*)ptr;

	while(1)
    {
        if(p->m_eEngineState != RECORDER_ENGINE_STATE_RUNNING)
        {
            g_usleep(40 * MILLISEC_TO_SEC);
            continue;
        }

        if(0 == g_queue_get_length(&p->m_nFrameQueue))
        {
            gdouble cap_time = ms_time();
            if (cap_time - p->m_last_time > 3000.0)
            {
                p->m_last_time = cap_time;
                g_print("Not receiving data for more than 3 seconds, turning off recording!!!\n");
                p->SetFatalEnd();
                need_recovery = true;
            }
            g_usleep(40 * MILLISEC_TO_SEC);
            continue;
        }

        if(need_recovery)
        {
            p->SetLoopRecovery();
            need_recovery = false;
        }
        p->m_last_time = ms_time();
		g_mutex_lock(&p->m_nFrameMutex);
		pFrame = (DVR_IO_FRAME *)g_queue_pop_tail(&p->m_nFrameQueue);
        if(pFrame)
            free(pFrame);
		g_mutex_unlock(&p->m_nFrameMutex);
        if (1)
        {
            static gdouble last_cap_time = ms_time();
            static double frame_rate_avg = 30.0;
            static gint64 frame_dt_ms = 33;
            static gdouble last_frameRate_count_time = 0;
            static int last_frameCount = 0;
            static int frameCount = 0;
            gdouble cap_time = ms_time();

            frame_dt_ms = cap_time - last_cap_time;
            if (cap_time - last_frameRate_count_time > 1000.0)
            {
                int framesInOneSecond = frameCount - last_frameCount;
				
				DPrint(DPRINT_ERR, "RE Frame Rate: %d\n", framesInOneSecond);
#ifdef __linux__
                get_memoccupy();
#endif
                last_frameRate_count_time = cap_time;
                last_frameCount = frameCount;
            }

            last_cap_time = cap_time;
            frameCount++;
        }
        g_usleep(40 * MILLISEC_TO_SEC);
    }
}

#if 0
gint CRecorderEngine::CreateMonitorTask(void)
{
    pthread_t m_hThread;
    int res = pthread_create(&m_hThread, NULL, RecMonitorTaskEntry, this);
    if (DVR_FAILED(res))
    {
        GST_ERROR("CEventRecorderEngine Create Task failed = 0x%x\n", res);
    }
    pthread_detach(m_hThread);
    return DVR_RES_SOK;
}
#endif
gint CRecorderEngine::DestroyRecorder(void)
{
    if (m_eEngineState == RECORDER_ENGINE_STATE_START_TO_STOP)
    {
        while (m_eEngineState != RECORDER_ENGINE_STATE_STOP_DONE)
        {
            GST_INFO("Recorder has not finish stop, waiting...");
            g_usleep(5 * MILLISEC_TO_SEC);
        }
    }

    g_mutex_lock(&m_nFrameMutex);
	g_queue_foreach(&m_nFrameQueue, (GFunc)free, NULL);
	g_queue_clear(&m_nFrameQueue);
    g_mutex_unlock(&m_nFrameMutex);
	return DVR_RES_SOK;
}

void CRecorderEngine::SetOsdTitle(GstBuffer* buffer, DVR_IO_FRAME* pFrame)
{
#if 0
    GstMapInfo map;
    OSD_SRC_PARAM srcparam;
    if(buffer == NULL || pFrame == NULL)
        return;
    
    srcparam.type = OSD_SRC_VIDEO;
    srcparam.imagewidth  = DVR_IMG_REAL_WIDTH;
    srcparam.imageheight = DVR_IMG_REAL_HEIGHT;
    srcparam.rectype     = OSD_SRC_REC_TYPE_NONE;
    Get(DVR_RECORDER_CFG_DAS_RECTYPE, NULL, 0, &srcparam.rectype, sizeof(int), NULL);

    Ofilm_Can_Data_T *pCanData = (Ofilm_Can_Data_T *)pFrame->pCanBuf;
#ifdef __linux__
	GstMetaDmaBuf *dmabuf = gst_buffer_get_dma_buf_meta(buffer);
	if(dmabuf != NULL)
	{
		srcparam.data = (unsigned char*)gst_dma_buf_meta_get_virt_addr(dmabuf);
		if(m_pOsdTitie != NULL && srcparam.data != NULL)
			m_pOsdTitie->SetOverlay(pCanData, &srcparam);
        if(0)
        {
            struct tm *t;
            time_t tt;
            time(&tt);
            static int m_time = 0;
            t = localtime(&tt);
            if(t->tm_sec != m_time)
            {
                static int first = 1;
                static FILE *fd = 0;
                if(first == 1)
                {
                    char buffer[30];
                    sprintf(buffer,"/data/11.YUV");
                    fd = fopen(buffer,"wb");
                    first = 0;
                }
        
                fwrite(srcparam.data, 1, DVR_IMG_REAL_WIDTH*DVR_IMG_REAL_HEIGHT*3>>1, fd);
                fflush(fd);
                m_time = t->tm_sec;
            }
        }
	}	
#else
	gst_buffer_map(buffer, &map, GST_MAP_WRITE);
	srcparam.data = map.data;
    if(m_pOsdTitie != NULL && srcparam.data != NULL)
        m_pOsdTitie->SetOverlay(pCanData, &srcparam);
    gst_buffer_unmap(buffer,&map);
#endif	
#endif
}

void CRecorderEngine::SetLoopRecovery(void)
{
    if (m_eEngineState == RECORDER_ENGINE_STATE_RUNNING)
    {
        if (m_pLoopRecordEngine != NULL)
        {
            m_pLoopRecordEngine->SetRecovery();
        }
    }
}

void CRecorderEngine::SetFatalEnd(void)
{
    if (m_eEngineState == RECORDER_ENGINE_STATE_RUNNING)
    {
        if (m_pLoopRecordEngine != NULL)
        {
            m_pLoopRecordEngine->SetFatalEnd();
        }

        if (m_pEventRecordEngine != NULL)
        {
            m_pEventRecordEngine->SetFatalEnd();
        }

        if (m_pIaccRecordEngine != NULL)
        {
            m_pIaccRecordEngine->SetFatalEnd();
        }

        if (m_pApaRecordEngine != NULL)
        {
            m_pApaRecordEngine->SetFatalEnd();
        }
    }
}

int CRecorderEngine::Create(gboolean bLockState)
{
    if (m_pLoopRecordEngine != NULL)
    {
        m_pLoopRecordEngine->Create();
    }

    if (m_pEventRecordEngine != NULL)
    {
        m_pEventRecordEngine->Create();
    }

    if (m_pApaRecordEngine != NULL)
    {
        m_pApaRecordEngine->Create();
    }

    if (m_pIaccRecordEngine != NULL)
    {
        m_pIaccRecordEngine->Create();
    }

	return DVR_RES_SOK;
}

int CRecorderEngine::Destroy(gboolean bLockState)
{
    if (m_pLoopRecordEngine != NULL)
    {
        m_pLoopRecordEngine->Destroy();
    }

    if (m_pEventRecordEngine != NULL)
    {
        m_pEventRecordEngine->Destroy();
    }

    if (m_pApaRecordEngine != NULL)
    {
        m_pApaRecordEngine->Destroy();
    }

    if (m_pIaccRecordEngine != NULL)
    {
        m_pIaccRecordEngine->Destroy();
    }

	return DestroyRecorder();
}

int CRecorderEngine::Start(gboolean bLockState)
{
	if (m_eEngineState == RECORDER_ENGINE_STATE_RUNNING)
	{
		GST_WARNING("start video recorder failed, it has already started\n");
		return DVR_RES_EFAIL;
	}

    if (m_eEngineState == RECORDER_ENGINE_STATE_START_TO_STOP)
    {
        GST_WARNING("stop process is running, wait stop complete\n");
        return DVR_RES_EFAIL;
    }

    if (!gst_is_initialized())
    {
        GST_ERROR("Gstreamer is not initialized!\n");
        return DVR_RES_EFAIL;
    }

	m_eEngineState = RECORDER_ENGINE_STATE_RUNNING;

    m_last_time = ms_time();
    m_basetime  = 0;
	m_inputCount =0;
	return DVR_RES_SOK;
}

int CRecorderEngine::Stop(gboolean bLockState)
{
	int Ret = DVR_RES_SOK;

	if (m_eEngineState == RECORDER_ENGINE_STATE_INVALID ||
		m_eEngineState == RECORDER_ENGINE_STATE_START_TO_STOP ||
		m_eEngineState == RECORDER_ENGINE_STATE_STOP_DONE)
	{
		GST_INFO("stop video recorder failed, it has already stopped\n");
		return DVR_RES_SOK;
	}


	GST_INFO("stop video recorder\n");

	m_eEngineState = RECORDER_ENGINE_STATE_START_TO_STOP;

	g_queue_clear(&m_nFrameQueue);
	m_eEngineState = RECORDER_ENGINE_STATE_STOP_DONE;

	GST_INFO("stop video recorder done\n");

	return Ret;
}

int CRecorderEngine::Reset(gboolean bLockState)
{
	int Ret = DVR_RES_SOK;

	GST_INFO("reset event recorder\n");

    if (m_pEventRecordEngine != NULL)
    {
        m_pEventRecordEngine->Reset();
    }	

	if(m_pIaccRecordEngine != NULL)
	{
		m_pIaccRecordEngine->Reset();
	}

	GST_INFO("reset event recorder done\n");

	return Ret;

}

int CRecorderEngine::LoopRecStart()
{
    int ret = DVR_RES_EFAIL;

	if (m_eEngineState != RECORDER_ENGINE_STATE_RUNNING)
		return DVR_RES_EFAIL;

    if (m_pLoopRecordEngine != NULL)
    {
        ret = m_pLoopRecordEngine->Start();
    }

    return ret;
}

int CRecorderEngine::LoopRecStop()
{
    int ret = DVR_RES_EFAIL;

    if (m_pLoopRecordEngine != NULL)
    {
        ret = m_pLoopRecordEngine->Stop();
    }

    return ret;
}

int CRecorderEngine::DasRecStart()
{
    int ret = DVR_RES_SOK;

    if (m_pApaRecordEngine != NULL)
    {
#ifdef FEATURE_RECORD_ASYNCOP_ENABLE
        if (m_pRecorderAsyncOp != NULL)
            m_pRecorderAsyncOp->RecorderAsyncOp_SndMsg(RECORDER_ASYNCOP_CMD_START_DAS_RECORDER, 0, 0);
#else
        m_pApaRecordEngine->Start();
#endif
    }

    return ret;
}

int CRecorderEngine::DasRecStop()
{
    int ret = DVR_RES_SOK;

    if (m_pApaRecordEngine != NULL)
    {
#ifdef FEATURE_RECORD_ASYNCOP_ENABLE  
        if (m_pRecorderAsyncOp != NULL)
            m_pRecorderAsyncOp->RecorderAsyncOp_SndMsg(RECORDER_ASYNCOP_CMD_STOP_DAS_RECORDER, 0, 0);
#else
		m_pApaRecordEngine->Stop();
#endif
    }

    return ret;
}

int CRecorderEngine::IaccRecStart(int type)
{
    int ret = DVR_RES_SOK;

    if (m_pIaccRecordEngine != NULL)
    {
#ifdef FEATURE_RECORD_ASYNCOP_ENABLE      
        if (m_pRecorderAsyncOp != NULL)
            m_pRecorderAsyncOp->RecorderAsyncOp_SndMsg(RECORDER_ASYNCOP_CMD_START_IACC_RECORDER, 0, 0);
#else
		m_pIaccRecordEngine->Start();
#endif
    }

    return ret;
}

int CRecorderEngine::IaccRecStop(int type)
{
    int ret = DVR_RES_SOK;

    if (m_pIaccRecordEngine != NULL)
    {
#ifdef FEATURE_RECORD_ASYNCOP_ENABLE      
	    if (m_pRecorderAsyncOp != NULL)
	        m_pRecorderAsyncOp->RecorderAsyncOp_SndMsg(RECORDER_ASYNCOP_CMD_STOP_IACC_RECORDER, 0, 0);
#else
		m_pIaccRecordEngine->Stop();
#endif
	}
	
    return ret;
}

int CRecorderEngine::Set(glong dwPropID, gpointer pInstanceData, gint cbInstanceData, gpointer pPropData, gint cbPropData)
{
	int ret = DVR_RES_SOK;
	
    DvrMutexLocker locker(&m_nConfigMutex);
	switch (dwPropID)
	{
	case DVR_RECORDER_CFG_FILE_SPLIT_TIME:
	{
		if (pPropData == NULL || cbPropData != sizeof(DVR_RECORDER_PERIOD_OPTION))
		{
			GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

        if (m_pLoopRecordEngine != NULL)
        {
            m_pLoopRecordEngine->Set(LOOP_RECORD_PROP_FILE_SPLIT_TIME, pPropData, cbPropData);
        }
	}
	break;

	case DVR_RECORDER_CFG_VIDEO_QUALITY:
	{
		if (pPropData == NULL || cbPropData != sizeof(RECORDER_VIDEO_QUALITY_SETTING))
		{
			GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}
		RECORDER_VIDEO_QUALITY_SETTING *pSetting = (RECORDER_VIDEO_QUALITY_SETTING *)pPropData;

		memcpy(&m_nConfig.m_PendVQSetting, pSetting, sizeof(RECORDER_VIDEO_QUALITY_SETTING));
		m_nConfig.m_bVQPend = TRUE;
	}
	break;

	case DVR_RECORDER_CFG_INTRA_INTERVAL:
	{
		if (pPropData == NULL || cbPropData != sizeof(guint))
		{
			GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		m_nConfig.intra_interval = *(guint*)pPropData;
	}
	break;

	case DVR_RECORDER_CFG_ROOT_DIRECTORY:
	{
		if (pPropData == NULL || cbPropData != sizeof(DVR_DISK_DIRECTORY))
		{
			GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		DVR_DISK_DIRECTORY *pDir = (DVR_DISK_DIRECTORY *)pPropData;

        if (m_pLoopRecordEngine != NULL)
            m_pLoopRecordEngine->Set(LOOP_RECORD_PROP_DIR, pDir->szLoopRecDir, sizeof(pDir->szLoopRecDir));

        if (m_pEventRecordEngine != NULL)
            m_pEventRecordEngine->Set(EVENT_RECORD_PROP_DIR, pDir->szEventRecDir, sizeof(pDir->szEventRecDir));

        if (m_pApaRecordEngine != NULL)
            m_pApaRecordEngine->Set(LOOP_RECORD_PROP_DIR, pDir->szDasDir, sizeof(pDir->szDasDir));

        if (m_pIaccRecordEngine != NULL)
            m_pIaccRecordEngine->Set(EVENT_RECORD_PROP_DIR, pDir->szDasDir, sizeof(pDir->szDasDir));

		if (m_pPhotoEngine != NULL)
			m_pPhotoEngine->Set(PHOTO_PROP_DIR, pDir->szPhotoDir, sizeof(pDir->szPhotoDir));
	}
	break;

    case DVR_RECORDER_CFG_EMERGENCY_SETTING:
    {
        if (pPropData == NULL || cbPropData != sizeof(DVR_EVENT_RECORD_SETTING))
        {
            GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
            return DVR_RES_EINVALIDARG;
        }
        
		DVR_EVENT_RECORD_SETTING *pSetting = (DVR_EVENT_RECORD_SETTING *)pPropData;

        if(pSetting->EventRecordDelayTime != 0)
        {
            if (m_pIaccRecordEngine != NULL)
                m_pIaccRecordEngine->Set(EVENT_RECORD_PROP_LIMITM_DELAY, &pSetting->EventRecordDelayTime, sizeof(DVR_U32));
        }
        else
        {
            if (m_pEventRecordEngine != NULL)
                m_pEventRecordEngine->Set(EVENT_RECORD_PROP_PERIOD_SETTING, pPropData, cbPropData);

            if (m_pIaccRecordEngine != NULL)
                m_pIaccRecordEngine->Set(EVENT_RECORD_PROP_PERIOD_SETTING, pPropData, cbPropData);
        }
    }
	break;

	case DVR_RECORDER_CFG_FATAL_ERROR:
	{
        if (pPropData == NULL || cbPropData != sizeof(gboolean))
        {
            GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
            return DVR_RES_EINVALIDARG;
        }
		
		if(m_pLoopRecordEngine != NULL)
			m_pLoopRecordEngine->Set(LOOP_RECORD_PROP_FATAL_ERROR, pPropData, cbPropData);

        if (m_pEventRecordEngine != NULL)
            m_pEventRecordEngine->Set(EVENT_RECORD_PROP_FATAL_ERROR, pPropData, cbPropData);

        if (m_pApaRecordEngine != NULL)
            m_pApaRecordEngine->Set(LOOP_RECORD_PROP_FATAL_ERROR, pPropData, cbPropData);

        if (m_pIaccRecordEngine != NULL)
            m_pIaccRecordEngine->Set(EVENT_RECORD_PROP_FATAL_ERROR, pPropData, cbPropData);		
	}
	break;

	default:
		return DVR_RES_EFAIL;
	}

	return ret;
}

int CRecorderEngine::Get(glong dwPropID, gpointer pInstanceData, gint cbInstanceData, gpointer pPropData, gint cbPropData, gint *pcbReturned)
{
    DvrMutexLocker locker(&m_nConfigMutex);
	switch (dwPropID)
	{
	case DVR_RECORDER_CFG_FILE_SPLIT_TIME:
	{
		if (pPropData == NULL || cbPropData != sizeof(guint))
		{
			GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

        if (m_pLoopRecordEngine != NULL)
            m_pLoopRecordEngine->Get(LOOP_RECORD_PROP_FILE_SPLIT_TIME, pPropData, cbPropData);

		break;
	}

	case DVR_RECORDER_CFG_RECORDER_STATUS:
	{
		if (pPropData == NULL || cbPropData != sizeof(gint))
		{
			GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		gint state = RECORDER_STATUS_INVALID;
		if (m_eEngineState == RECORDER_ENGINE_STATE_RUNNING)
		{
			state = RECORDER_STATUS_RUNNING;
		}
		else if (m_eEngineState == RECORDER_ENGINE_STATE_STOP_DONE)
		{
			state = RECORDER_STATUS_STOPPED;
		}

		*(gint *)pPropData = state;

		break;
	}

    case DVR_RECORDER_CFG_DAS_RECTYPE:
    {
		if (pPropData == NULL || cbPropData != sizeof(gint))
		{
			GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

        gint type = OSD_SRC_REC_TYPE_NONE;
        if (m_pIaccRecordEngine != NULL && m_pIaccRecordEngine->GetRecState() == EVENT_RECORD_STATE_RUNNING)
        {
            DVR_EVENT_RECORD_SETTING pPropData;
            memset(&pPropData, 0, sizeof(DVR_EVENT_RECORD_SETTING));
            m_pEventRecordEngine->Get(EVENT_RECORD_PROP_PERIOD_SETTING, &pPropData, sizeof(DVR_EVENT_RECORD_SETTING));
            if(pPropData.eType == DVR_EVENTREC_SOURCE_TYPE_IACC_IAC)
                type = OSD_SRC_REC_TYPE_IACC;
            else if(pPropData.eType == DVR_EVENTREC_SOURCE_TYPE_IACC_ACC)
                type = OSD_SRC_REC_TYPE_ACC;
            else if(pPropData.eType == DVR_EVENTREC_SOURCE_TYPE_IACC_AEB)
                type = OSD_SRC_REC_TYPE_AEB;
        }
        else if (m_pApaRecordEngine != NULL && m_pApaRecordEngine->GetRecState() == LOOP_RECORD_STATE_RUNNING)
        {
            type = OSD_SRC_REC_TYPE_APA;
        }

		*(gint *)pPropData = type;

        break;
    }
    case DVR_RECORDER_CFG_EMERGENCY_SETTING:
    {
        if (pPropData == NULL || cbPropData != sizeof(DVR_EVENT_RECORD_SETTING))
        {
            GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
            return DVR_RES_EINVALIDARG;
        }

        if (m_pEventRecordEngine != NULL)
            m_pEventRecordEngine->Get(EVENT_RECORD_PROP_PERIOD_SETTING, pPropData, cbPropData);

        break;
    }

	default:
		return DVR_RES_EFAIL;
	}

	return DVR_RES_SOK;
}

int CRecorderEngine::AcquireInputBuf(void **ppvBuffer)
{
	*ppvBuffer = NULL;
    GST_INFO("Recorder Engine AcquireInputBuf!!!\n");
	return DVR_RES_EFAIL;
}

int	CRecorderEngine::AsyncOpFlush(void)
{   
    if(m_pPhotoAsyncOp != NULL)
        m_pPhotoAsyncOp->AsyncOp_FlushMsg();
    if(m_pRecorderAsyncOp != NULL)
        m_pRecorderAsyncOp->RecorderAsyncOp_FlushMsg();
    
    GST_INFO("Recorder Engine AsyncOpFlush!!!\n");
    return DVR_RES_SOK;
}

int	CRecorderEngine::Photo(DVR_PHOTO_PARAM *pParam)
{
    if (m_eEngineState != RECORDER_ENGINE_STATE_RUNNING)
    {
        GST_ERROR ("Recorder Engine is closed!!!\n");
        return DVR_RES_EFAIL;
    }

	GST_INFO("take a photo\n");
	PHOTO_INPUT_PARAM *pPhotoParam = (PHOTO_INPUT_PARAM *)malloc(sizeof(PHOTO_INPUT_PARAM));
	if(pPhotoParam == NULL)
		return DVR_RES_EOUTOFMEMORY;

	memset(pPhotoParam, 0, sizeof(PHOTO_INPUT_PARAM));
    pPhotoParam->eType = pParam->eType;
    pPhotoParam->eQuality = pParam->eQuality;
    pPhotoParam->eIndex = pParam->eIndex;

	return m_pPhotoAsyncOp->AsyncOp_SndMsg(PHOTO_ASYNCOP_CMD_TAKE_PHOTO, (DVR_U32)pPhotoParam, 0);
}

int	CRecorderEngine::Overlay(void *canbuffer, void *osdbuffer, int cansize, int osdsize)
{
    OSD_SRC_PARAM srcparam;
    if (m_eEngineState != RECORDER_ENGINE_STATE_RUNNING)
    {
        //GST_ERROR ("Recorder Engine is closed!!!\n");
        return DVR_RES_EFAIL;
    }
    
    if(canbuffer == NULL || osdbuffer == NULL)
    {
        GST_ERROR ("canbuffer is %p ,osdbuffer is %p!!!\n",canbuffer,osdbuffer);
        return DVR_RES_EFAIL;
    }
    
    if(cansize != sizeof(Ofilm_Can_Data_T))
    {
        GST_ERROR ("cansize is %d ,sizeof(Ofilm_Can_Data_T) is %d!!!\n",cansize,sizeof(Ofilm_Can_Data_T));
        return DVR_RES_EFAIL;
    }

    if(m_pOsdTitie == NULL)
        return DVR_RES_EFAIL;

	//GST_INFO("take a Overlay\n");
    srcparam.type = OSD_SRC_VIDEO;
    srcparam.imagewidth  = 1024;
    srcparam.imageheight = 52;
    srcparam.rectype     = OSD_SRC_REC_TYPE_NONE;
    Get(DVR_RECORDER_CFG_DAS_RECTYPE, NULL, 0, &srcparam.rectype, sizeof(int), NULL);
    return m_pOsdTitie->SetOverlay((Ofilm_Can_Data_T *)canbuffer, &srcparam, osdbuffer,osdsize );
}

int CRecorderEngine::GetPosition(glong* pdwPos, gboolean bLockState /* = FALSE */)
{
	GST_ERROR("[Engine][Set Position] LockState() FAILED!\n");
	return DVR_RES_EFAIL;
}

int CRecorderEngine::AsyncAddPhotoToDB(const char *pLocation, const char *pThumbNailLocation, void *pContext)
{
	CRecorderEngine *pThis = (CRecorderEngine *)pContext;
	DVR_ADDTODB_FILE_INFO tFileInfo;

	tFileInfo.file_location = pLocation;
    tFileInfo.thumbnail_location = pThumbNailLocation;
	tFileInfo.time_stamp = 0;
    tFileInfo.eType = DVR_FOLDER_TYPE_PHOTO;
	if (pThis->m_ptfnCallBackArray != NULL && pThis->m_ptfnCallBackArray->MessageCallBack != NULL)
	{
		RECORDER_EVENT  eEvent = RECORDER_EVENT_NEW_FILE_FINISHED;
		pThis->m_ptfnCallBackArray->MessageCallBack((void*)&eEvent, &tFileInfo, NULL, pThis->m_ptfnCallBackArray->pCallBackInstance);
	}

	return DVR_RES_SOK;
}

int CRecorderEngine::AsyncWaitImgForPhoto(DVR_U32 param1, DVR_U32 param2, void *pContext)
{
	DVR_IO_FRAME *pFrame;
	PHOTO_FORMAT_SETTING setting;
	CRecorderEngine *pThis = (CRecorderEngine *)((CPhotoEngine *)pContext)->ParentEngine();
	PHOTO_INPUT_PARAM *pPhotoParam = (PHOTO_INPUT_PARAM *)param1;
	if(pThis == NULL || pPhotoParam == NULL)
		return DVR_RES_EPOINTER;

	pFrame = &pThis->m_nInFrameForPhoto;
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
    if (pCanData != NULL)
    {
        pPhotoParam->stVehInfo.TimeYear = pCanData->vehicle_data.year;
        pPhotoParam->stVehInfo.TimeMon = pCanData->vehicle_data.month;
        pPhotoParam->stVehInfo.TimeDay = pCanData->vehicle_data.day;
        pPhotoParam->stVehInfo.TimeHour = pCanData->vehicle_data.hour;
        pPhotoParam->stVehInfo.TimeMin = pCanData->vehicle_data.minute;
        pPhotoParam->stVehInfo.TimeSec = pCanData->vehicle_data.second;
        pPhotoParam->stVehInfo.VehicleSpeed = pCanData->vehicle_data.vehicle_speed;
		pPhotoParam->stVehInfo.VehicleSpeedValidity = pCanData->vehicle_data.Vehicle_Speed_Validity;
        pPhotoParam->stVehInfo.GearShiftPositon = pCanData->vehicle_data.vehicle_movement_state;
        pPhotoParam->stVehInfo.BrakePedalStatus = pCanData->vehicle_data.Brake_Pedal_Position;
        pPhotoParam->stVehInfo.DriverBuckleSwitchStatus = pCanData->vehicle_data.DriverBuckleSwitchStatus;
        pPhotoParam->stVehInfo.AccePedalPosition = pCanData->vehicle_data.Accelerator_Actual_Position;
        pPhotoParam->stVehInfo.TurnSignal = pCanData->vehicle_data.turn_signal;
        pPhotoParam->stVehInfo.GpsLongitude = pCanData->vehicle_data.positioning_system_longitude;
        pPhotoParam->stVehInfo.GpsLatitude = pCanData->vehicle_data.positioning_system_latitude;
        pPhotoParam->stVehInfo.LateralAcceleration = pCanData->vehicle_data.Lateral_Acceleration;
        pPhotoParam->stVehInfo.LongitAcceleration = pCanData->vehicle_data.Longitudinal_Acceleration;
        pPhotoParam->stVehInfo.EmergencyLightstatus = pCanData->vehicle_data.EmergencyLightstatus;
        pPhotoParam->stVehInfo.APALSCAction = pCanData->vehicle_data.APA_LSCAction;
        pPhotoParam->stVehInfo.IACCTakeoverReq = pCanData->vehicle_data.LAS_IACCTakeoverReq;
        pPhotoParam->stVehInfo.ACCTakeOverReq = pCanData->vehicle_data.ACC_TakeOverReq;
        pPhotoParam->stVehInfo.AEBDecCtrlAvail = pCanData->vehicle_data.ACC_AEBDecCtrlAvail;
    }

	pThis->m_pPhotoEngine->Set(PHOTO_PROP_FORMAT, &setting, sizeof(PHOTO_FORMAT_SETTING));

	return DVR_RES_SOK;
}

int CRecorderEngine::PhotoReturn(DVR_U32 param1, DVR_U32 param2, void *pContext)
{
	CRecorderEngine *pThis = (CRecorderEngine *)((CPhotoEngine *)pContext)->ParentEngine();
	PHOTO_INPUT_PARAM *pPhotoParam = (PHOTO_INPUT_PARAM *)param1;

	if(pThis == NULL || pPhotoParam == NULL)
		return DVR_RES_EPOINTER;

	free(pPhotoParam);

	if (pThis->m_ptfnCallBackArray != NULL && pThis->m_ptfnCallBackArray->MessageCallBack != NULL)
	{
		RECORDER_EVENT	eEvent = RECORDER_EVENT_PHOTO_DONE;
		pThis->m_ptfnCallBackArray->MessageCallBack((void*)&eEvent, NULL, NULL, pThis->m_ptfnCallBackArray->pCallBackInstance);
	}

	return DVR_RES_SOK;
}

int	CRecorderEngine::EventRecStart()
{
    if (m_eEngineState != RECORDER_ENGINE_STATE_RUNNING)
    {
        GST_INFO("Recorder Engine has not run!");
        return DVR_RES_EFAIL;
    }

    if (m_pEventRecordEngine != NULL)
    {
#ifdef FEATURE_RECORD_ASYNCOP_ENABLE
        if (m_pRecorderAsyncOp != NULL)
            m_pRecorderAsyncOp->RecorderAsyncOp_SndMsg(RECORDER_ASYNCOP_CMD_START_EVENT_RECORDER, 0, 0);
#else
        m_pEventRecordEngine->Start();
#endif
    }

    return DVR_RES_SOK;
}

int CRecorderEngine::EventRecStop()
{
    if (m_pEventRecordEngine != NULL)
    {
//#ifdef FEATURE_RECORD_ASYNCOP_ENABLE
        if (m_pRecorderAsyncOp != NULL)
            m_pRecorderAsyncOp->RecorderAsyncOp_SndMsg(RECORDER_ASYNCOP_CMD_STOP_EVENT_RECORDER, 0, 0);
//#else
//        m_pEventRecordEngine->Stop();
//#endif
    }	

    return DVR_RES_SOK;
}

int CRecorderEngine::AsyncStartLoopRec(DVR_U32 param1, DVR_U32 param2, void *pContext)
{
	CRecorderEngine *pThis = (CRecorderEngine *)pContext;
    if (pThis->m_pLoopRecordEngine != NULL)
    {
        pThis->m_pLoopRecordEngine->Start();
    }

	return DVR_RES_SOK;
}

int CRecorderEngine::AsyncStopLoopRec(DVR_U32 param1, DVR_U32 param2, void *pContext)
{
	CRecorderEngine *pThis = (CRecorderEngine *)pContext;
	
    if (pThis->m_pLoopRecordEngine != NULL)
    {
        pThis->m_pLoopRecordEngine->Stop();
    }

	return DVR_RES_SOK;
}

int CRecorderEngine::AsyncStartDasRec(DVR_U32 param1, DVR_U32 param2, void *pContext)
{
    CRecorderEngine *pThis = (CRecorderEngine *)pContext;

    if (pThis->m_pApaRecordEngine != NULL)
    {
        pThis->m_pApaRecordEngine->Start();
    }

    return DVR_RES_SOK;
}

int CRecorderEngine::AsyncStopDasRec(DVR_U32 param1, DVR_U32 param2, void *pContext)
{
    CRecorderEngine *pThis = (CRecorderEngine *)pContext;

    if (pThis->m_pApaRecordEngine != NULL)
    {
        pThis->m_pApaRecordEngine->Stop();
    }

    return DVR_RES_SOK;
}

int CRecorderEngine::AsyncStartIaccRec(DVR_U32 param1, DVR_U32 param2, void *pContext)
{
    CRecorderEngine *pThis = (CRecorderEngine *)pContext;

    if (pThis->m_pIaccRecordEngine != NULL)
    {
        pThis->m_pIaccRecordEngine->Start();
    }

    return DVR_RES_SOK;
}

int CRecorderEngine::AsyncStopIaccRec(DVR_U32 param1, DVR_U32 param2, void *pContext)
{
	CRecorderEngine *pThis = (CRecorderEngine *)pContext;
	
    if (pThis->m_pIaccRecordEngine != NULL)
    {
        pThis->m_pIaccRecordEngine->Stop();
    }

	return DVR_RES_SOK;
}

int CRecorderEngine::AsyncStartEventRec(DVR_U32 param1, DVR_U32 param2, void *pContext)
{
	CRecorderEngine *pThis = (CRecorderEngine *)pContext;

    if (pThis->m_pEventRecordEngine != NULL)
    {
        pThis->m_pEventRecordEngine->Start();
    }

	return DVR_RES_SOK;
}

int CRecorderEngine::AsyncStopEventRec(DVR_U32 param1, DVR_U32 param2, void *pContext)
{
	CRecorderEngine *pThis = (CRecorderEngine *)pContext;
	
    if (pThis->m_pEventRecordEngine != NULL)
    {
        pThis->m_pEventRecordEngine->Stop();
    }

	return DVR_RES_SOK;
}

int CRecorderEngine::AsyncWaitImgForVideo(DVR_U32 param1, DVR_U32 param2, void *pContext)
{
	DVR_IO_FRAME *pFrame;
	CThumbNail *pHandle = (CThumbNail *)pContext;
	
	if (!pHandle)
	{
		GST_ERROR("pHandle is null");
		return DVR_RES_EPOINTER;
	}

	CRecorderEngine *pRecorderEngine = (CRecorderEngine *)(pHandle->RecorderEngine());
	if (NULL == pRecorderEngine)
	{
		GST_ERROR("recorderEngine is NULL");
		return DVR_RES_EPOINTER;
	}

	while (0 == g_queue_get_length(&pRecorderEngine->m_nFrameQueue))
	{
		GST_LOG("not enough data in image queue, waiting...\n");
		g_usleep(1 * MILLISEC_TO_SEC);
	}

	g_mutex_lock(&pRecorderEngine->m_nFrameMutex);

    pFrame = (DVR_IO_FRAME *)g_queue_peek_head(&pRecorderEngine->m_nFrameQueue);
	TN_INPUT_PARAM tn_frame;
	tn_frame.format = GST_VIDEO_FORMAT_NV12;
	tn_frame.width = pFrame->crop.width;
	tn_frame.height = pFrame->crop.height;
	tn_frame.pInputImage = pFrame->pImageBuf[0];
	pHandle->Set(THUMBNAIL_PROP_INFRAME, &tn_frame, sizeof(TN_INPUT_PARAM));

	g_mutex_unlock(&pRecorderEngine->m_nFrameMutex);

    OSD_SRC_PARAM srcparam;
    srcparam.type = OSD_SRC_PIC;
    srcparam.imagewidth  = tn_frame.width;
    srcparam.imageheight = tn_frame.height;
    srcparam.rectype     = OSD_SRC_REC_TYPE_NONE;
    srcparam.data        = pHandle->InBuf();
    pRecorderEngine->Get(DVR_RECORDER_CFG_DAS_RECTYPE, NULL, 0, &srcparam.rectype, sizeof(int), NULL);

    if(srcparam.rectype != OSD_SRC_REC_TYPE_NONE)
    {
        COsdString *pOsdString = new COsdString();
        pOsdString->SetString(&srcparam, NULL);

        delete pOsdString;
        pOsdString = NULL;
    }
	return DVR_RES_SOK;
}

static unsigned char * createPngBuffer(unsigned char *srcBuf, int srcBufSize, int *destBufSize)
{
	guint8 pngFileStartFlags[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
	guint8 pngFileEndFlags[] = { 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82 };

	if (!srcBuf || (srcBufSize <= 0))
	{
		GST_ERROR("srcBuf is NULL");
		return NULL;
	}

	unsigned char *destBuffer = (unsigned char *)g_malloc(srcBufSize + sizeof(pngFileStartFlags) + sizeof(pngFileEndFlags));
	if (destBuffer == NULL)
	{
		return NULL;
	}
	memcpy(destBuffer, pngFileStartFlags, sizeof(pngFileStartFlags));
	memcpy(destBuffer + sizeof(pngFileStartFlags), srcBuf, srcBufSize);
	memcpy(destBuffer + srcBufSize + sizeof(pngFileStartFlags), pngFileEndFlags, sizeof(pngFileEndFlags));

	*destBufSize = srcBufSize + sizeof(pngFileStartFlags) + sizeof(pngFileEndFlags);
	return destBuffer;
}
void* CRecorderEngine::Async(void* pParam)
{
#ifdef __linux__
    sync();
#endif
    return NULL;
}

int CRecorderEngine::VideoAddThumbNail(DVR_U32 param1, DVR_U32 param2, void *pContext)
{
    GstElement *pQtMuxer = NULL;
    CThumbNail *pHandle = (CThumbNail *)pContext;
    gchar *thumb_name = (gchar *)param1;
    RECORDER_MODE mode = (RECORDER_MODE)param2;
    GST_INFO("record mode (%d)\n",mode);
    if (!pHandle)
    {
        GST_ERROR("pHandle is null");
        return DVR_RES_EPOINTER;
    }

    CRecorderEngine *pRecorderEngine = (CRecorderEngine *)(pHandle->RecorderEngine());
    if (NULL == pRecorderEngine)
    {
        GST_ERROR("recorderEngine is NULL");
        return DVR_RES_EPOINTER;
    }
    
    if(mode == RECORDER_MODE_NORMAL && pRecorderEngine->m_pLoopRecordEngine != NULL)
        pQtMuxer = pRecorderEngine->m_pLoopRecordEngine->GetMuxer();
    else if(mode == RECORDER_MODE_EMERGENCY && pRecorderEngine->m_pEventRecordEngine != NULL)
        pQtMuxer = pRecorderEngine->m_pEventRecordEngine->GetMuxer();
    else if(mode == RECORDER_MODE_APA && pRecorderEngine->m_pApaRecordEngine != NULL)
        pQtMuxer = pRecorderEngine->m_pApaRecordEngine->GetMuxer();
    else if(mode == RECORDER_MODE_IACC && pRecorderEngine->m_pIaccRecordEngine != NULL)
        pQtMuxer = pRecorderEngine->m_pIaccRecordEngine->GetMuxer();

    BMPIMAGE bmpimg;
	int err = 0;

    bmpimg.channels = 3;
    bmpimg.data = pHandle->OutBuf();
    bmpimg.width = pHandle->ThumbNailWidth();
    bmpimg.height = pHandle->ThumbNailHeight();

    OSD_SRC_PARAM srcparam;
    srcparam.type = OSD_SRC_PIC;
    srcparam.rectype  = OSD_SRC_REC_TYPE_NONE;

    pRecorderEngine->Get(DVR_RECORDER_CFG_DAS_RECTYPE, NULL, 0, &srcparam.rectype, sizeof(int), NULL);

    if(srcparam.rectype != OSD_SRC_REC_TYPE_NONE)
    {
        COsdString *pOsdString = new COsdString();
        pOsdString->SetString(&srcparam, &bmpimg);

        delete pOsdString;
        pOsdString = NULL;
    }

    
    if(!pQtMuxer)
    {
        GST_ERROR("pQtMuxer is null");
        return DVR_RES_EPOINTER;
    }
    
    int pngDataSize = 0;
    unsigned char *pPngData = createPngBuffer(pHandle->OutBuf(), pHandle->OutBufSize(), &pngDataSize);
    if (!pPngData)
    {
        GST_ERROR("createPngBuffer error");
        return DVR_RES_EFAIL;
    }

    GstBuffer *buffer;
    buffer = gst_buffer_new_wrapped(pPngData, pngDataSize);

    GstCaps *caps = gst_caps_from_string("image/png");
    GstStructure  *structure = gst_structure_new_empty("GstTagImageInfo");
    GstSample *sample = gst_sample_new(buffer, caps, NULL, structure);
    gst_caps_unref(caps);


    GstTagList *tagList = gst_tag_list_new_empty();
    if (tagList != NULL)

    {
        gst_tag_list_add(tagList, GST_TAG_MERGE_APPEND, GST_TAG_IMAGE, sample, NULL);
    }
    else
    {
        GST_ERROR("new a tag list failed");
        gst_buffer_unref(buffer);
        gst_sample_unref(sample);
        return DVR_RES_EPOINTER;
    }
    if (mode == RECORDER_MODE_NORMAL || mode == RECORDER_MODE_APA)
    {
        GST_OBJECT_LOCK(pQtMuxer);
        gst_tag_setter_merge_tags(GST_TAG_SETTER(pQtMuxer), tagList, GST_TAG_MERGE_APPEND);
        GST_OBJECT_UNLOCK(pQtMuxer);
    }
    else if (mode == RECORDER_MODE_EMERGENCY || mode == RECORDER_MODE_IACC)
    {
        if (pRecorderEngine->m_pEventRecordEngine != NULL)
        {
            gst_tag_setter_merge_tags(GST_TAG_SETTER(pQtMuxer), tagList, GST_TAG_MERGE_APPEND);
            
            GST_OBJECT_LOCK(pQtMuxer);
            const GstTagList *tmpList = gst_tag_setter_get_tag_list(GST_TAG_SETTER(pQtMuxer));
            GST_OBJECT_UNLOCK(pQtMuxer);
            
            gst_element_send_event(pQtMuxer, gst_event_new_tag(gst_tag_list_copy(tmpList)));
            
            GST_INFO("Add thumbnail for emergency recording!!!!!");
        }
    }

    gst_buffer_unref(buffer);
    gst_sample_unref(sample);
    gst_tag_list_unref(tagList);


    return DVR_RES_SOK;
}

