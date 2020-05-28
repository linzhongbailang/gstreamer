/*===========================================================================*\
* Copyright 2003 O-Film Technologies, Inc., All Rights Reserved.
* O-Film Confidential
*
* DESCRIPTION:
*
* ABBREVIATIONS:
*   TODO: List of abbreviations used, or reference(s) to external document(s)
*
* TRACEABILITY INFO:
*   Design Document(s):
*     TODO: Update list of design document(s)
*
*   Requirements Document(s):
*     TODO: Update list of requirements document(s)
*
*   Applicable Standards (in order of precedence: highest first):
*
* DEVIATIONS FROM STANDARDS:
*   TODO: List of deviations from standards in this file, or
*   None.
*
\*===========================================================================*/

#include "dprint.h"
#include "DvrMutexLocker.h"
#include "DvrTimer.h"
//#include "JpegThumb.h"
#include "DvrRecorderLoop.h"
#include "osa_fs.h"
#include "BMP.h"

#ifdef _MSC_VER
#pragma warning(disable:4996)	// 4706: assignment within conditional expression
#endif

static RECORDER_VIDEO_QUALITY_SETTING VideoQualityPresets[DVR_VIDEO_QUALITY_NUM] =
{
	{ 10, 16384 },
	{ 20, 12288 },
	{ 32, 10240 }
};

static DVR_RESULT format_move_file_fullname(char *srcFn, char *dstFolder, char *dstFn)
{
    char *szSrcFullName = srcFn;
    char *szDstFolder = dstFolder;
    char *slash;

    std::string dst_file_location;
    char *szSrcFileName = (slash = strrchr(szSrcFullName, '/')) ? (slash + 1) : szSrcFullName;

    dst_file_location = szDstFolder;
    dst_file_location = dst_file_location + szSrcFileName;

    strcpy(dstFn, dst_file_location.data());

    return DVR_RES_SOK;
}

DvrRecorderLoop::DvrRecorderLoop(void *sysCtrl)
{
	int res = Dvr_Recorder_Initialize(&m_pEngine);
	if (DVR_FAILED(res)) {
		DPrint(DPRINT_ERR, "DvrRecorderLoop::DvrRecorderLoop() Dvr_Recorder_Initialize() failed: res = 0x%08x m_pEngine = 0x%08x\n", res, m_pEngine);
		m_pEngine = NULL;
	}

	res = Dvr_Recorder_Create(m_pEngine);
	if (DVR_FAILED(res)) {
		DPrint(DPRINT_ERR, "DvrRecorderLoop::DvrRecorderLoop() Dvr_Recorder_Create() failed: res = 0x%08x m_pEngine = 0x%08x\n", res, m_pEngine);
		Dvr_Recorder_DeInitialize(m_pEngine);
		m_pEngine = NULL;
	}

	if (m_pEngine != NULL)
	{
		memset(&m_ptfnCallBackArray, 0, sizeof(m_ptfnCallBackArray));
		m_ptfnCallBackArray.MessageCallBack = RecorderMessageCallback;
		m_ptfnCallBackArray.pCallBackInstance = (void *)this;
		Dvr_Recorder_RegisterCallBack(m_pEngine, &m_ptfnCallBackArray);
	}

	DvrSystemControl::Notifier()->Register(Notify, this);

#ifdef WIN32
	m_timer = new DvrTimer(DvrRecorderLoop::TimerCallback, this);
#endif

	m_sysCtrl = (DvrSystemControl *)sysCtrl;
	m_curRecFileName = NULL;
    memset(&m_fileToDelete, 0, sizeof(m_fileToDelete));
    memset(m_DasfileToDelete, 0, (DVR_DAS_TYPE_NUM - DVR_DAS_TYPE_APA) * sizeof(DVR_FILEMAP_META_ITEM));
    

	m_StorageHandleFlag = 0;
	m_bExit = FALSE;
    m_bMove = FALSE;

	DVR_STORAGE_HANDLER LoopEncStorageHandler = { 0 };
	LoopEncStorageHandler.FuncSearch = LoopEnc_Storage_SearchOldestFile;
	LoopEncStorageHandler.FuncHandle = LoopEnc_Storage_DeleteFile;
	LoopEncStorageHandler.FuncReturn = LoopEnc_Storage_Return;
	LoopEncStorageHandler.Command = HMSG_STORAGE_RUNOUT_HANDLE_START;
	LoopEncStorageHandler.pContext = this;
	m_sysCtrl->StorageAsyncOp_RegHandler(&LoopEncStorageHandler);

    /*DVR_STORAGE_HANDLER DasStorageHandler = { 0 };
    DasStorageHandler.FuncSearch = LoopEnc_Storage_SearchOldestFile;
    DasStorageHandler.FuncHandle = LoopEnc_Storage_DeleteFile;
    DasStorageHandler.FuncReturn = LoopEnc_Storage_Return;
    DasStorageHandler.Command = HMSG_STORAGE_DAS_RUNOUT_HANDLE_START;
    DasStorageHandler.pContext = this;
    m_sysCtrl->StorageAsyncOp_RegHandler(&DasStorageHandler);*/

    DVR_STORAGE_HANDLER DasFileNumStorageHandler = { 0 };
    DasFileNumStorageHandler.FuncSearch = DAS_Storage_DeleteFile;
    DasFileNumStorageHandler.FuncHandle = NULL;
    DasFileNumStorageHandler.FuncReturn = NULL;
    DasFileNumStorageHandler.Command = HMSG_STORAGE_DAS_NUM_RUNOUT_HANDLE_START;
    DasFileNumStorageHandler.pContext = this;
    m_sysCtrl->StorageAsyncOp_RegHandler(&DasFileNumStorageHandler);
}

DvrRecorderLoop::~DvrRecorderLoop()
{
	m_bExit = TRUE;
	
	Stop();

	if (m_pEngine) {
		Dvr_Recorder_Destroy(m_pEngine);
		Dvr_Recorder_DeInitialize(m_pEngine);
		m_pEngine = NULL;
	}

	if (m_curRecFileName != NULL)
	{
		free(m_curRecFileName);
		m_curRecFileName = NULL;
	}

    OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_RECORDER_DESTROY_DONE, NULL, 0, NULL, 0, DvrSystemControl::Notifier());

	DvrSystemControl::Notifier()->UnRegister(Notify, this);

    m_sysCtrl->StorageAsyncOp_ClrHandler();

#ifdef WIN32
	delete m_timer;
    m_timer = NULL;
#endif
}

DVR_RESULT DvrRecorderLoop::Start()
{
	int res;

	if (m_pEngine == NULL)
		return DVR_RES_EPOINTER;

	res = Dvr_Recorder_Start(m_pEngine);
	if (DVR_FAILED(res)) {
		DPrint(DPRINT_ERR, "DvrRecorderLoop::Start() failed res = 0x%08x\n", res);
		return DVR_RES_EFAIL;
	}
	
#ifdef WIN32
	struct timeval tv = { 5, 0 };
    if (m_timer != NULL)
	    m_timer->Start(tv);
#endif

	return DVR_RES_SOK;
}

DVR_RESULT DvrRecorderLoop::Stop()
{
	int res;

	if (m_pEngine == NULL)
		return DVR_RES_EPOINTER;

	DPrint(DPRINT_INFO, "calling Dvr_Recorder_Stop\n");
	res = Dvr_Recorder_Stop(m_pEngine);
	if (DVR_FAILED(res)) {
		DPrint(DPRINT_ERR, "DvrRecorderLoop::Stop() failed res = 0x%08x\n", res);
		return DVR_RES_EFAIL;
	}
	DPrint(DPRINT_INFO, "Finish Dvr_Recorder_Stop\n");

#ifdef WIN32
    if (m_timer != NULL)
	    m_timer->Stop();
#endif

	return DVR_RES_SOK;
}

DVR_RESULT DvrRecorderLoop::Reset()
{
	int res;

	if (m_pEngine == NULL)
		return DVR_RES_EPOINTER;

	DPrint(DPRINT_INFO, "calling Dvr_Recorder_Reset\n");
	res = Dvr_Recorder_Reset(m_pEngine);
	if (DVR_FAILED(res)) {
		DPrint(DPRINT_ERR, "DvrRecorderLoop::Reset() failed res = 0x%08x\n", res);
		return DVR_RES_EFAIL;
	}
	DPrint(DPRINT_INFO, "Finish Dvr_Recorder_Reset\n");
	
	return DVR_RES_SOK;

}

DVR_RESULT DvrRecorderLoop::Set(DVR_RECORDER_PROP type, void *configData, unsigned int size)
{
	int res;

	if (m_pEngine == NULL || configData == NULL)
		return DVR_RES_EPOINTER;

	switch (type)
	{
	case DVR_RECORDER_PROP_VIDEO_SPLIT_TIME:
	{
		DPrint(DPRINT_INFO, "Dvr_Recorder_SetConfig() with DVR_RECORDER_CFG_FILE_SPLIT_TIME %d\n", *(int *)configData);
		res = Dvr_Recorder_SetConfig(m_pEngine, DVR_RECORDER_CFG_FILE_SPLIT_TIME, configData, size);
		if (DVR_FAILED(res)) {
			DPrint(DPRINT_ERR, "DvrRecorderLoop::Set() Dvr_Recorder_SetConfig() with DVR_RECORDER_CFG_FILE_SPLIT_TIME failed! res = 0x%08x\n", res);
			return DVR_RES_EFAIL;
		}
	}
	break;

	case DVR_RECORDER_PROP_VIDEO_QUALITY:
	{
		DVR_U32 option = *(DVR_U32 *)configData;
		if (option >= DVR_VIDEO_QUALITY_NUM)
			return DVR_RES_EINVALIDARG;

		DPrint(DPRINT_INFO, "Dvr_Recorder_SetConfig() with DVR_RECORDER_PROP_VIDEO_QUALITY %d\n", *(int *)configData);
		res = Dvr_Recorder_SetConfig(m_pEngine, DVR_RECORDER_CFG_VIDEO_QUALITY, &VideoQualityPresets[option], sizeof(RECORDER_VIDEO_QUALITY_SETTING));

		if (DVR_FAILED(res)) {
			DPrint(DPRINT_ERR, "DvrRecorderLoop::Set() DVR_RECORDER_PROP_VIDEO_QUALITY failed! res = 0x%08x\n", res);
			return DVR_RES_EFAIL;
		}
	}
	break;

	case DVR_RECORDER_PROP_PHOTO_QUALITY:
	{

	}
	break;

	case DVR_RECORDER_PROP_ROOT_DIRECTORY:
	{
		res = Dvr_Recorder_SetConfig(m_pEngine, DVR_RECORDER_CFG_ROOT_DIRECTORY, configData, size);
		if (DVR_FAILED(res)) {
			DPrint(DPRINT_ERR, "DvrRecorderLoop::Set() Dvr_Recorder_SetConfig() with DVR_RECORDER_CFG_ROOT_DIRECTORY failed! res = 0x%08x\n", res);
			return DVR_RES_EFAIL;
		}
	}
	break;

    case DVR_RECORDER_PROP_EMERGENCY_SETTING:
    {
        res = Dvr_Recorder_SetConfig(m_pEngine, DVR_RECORDER_CFG_EMERGENCY_SETTING, configData, size);
        if (DVR_FAILED(res)) {
            DPrint(DPRINT_ERR, "DvrRecorderLoop::Set() Dvr_Recorder_SetConfig() with DVR_RECORDER_CFG_EMERGENCY_SETTING failed! res = 0x%08x\n", res);
            return DVR_RES_EFAIL;
        }
    }
    break;

	case DVR_RECORDER_PROP_FATAL_ERROR:
	{
        res = Dvr_Recorder_SetConfig(m_pEngine, DVR_RECORDER_CFG_FATAL_ERROR, configData, size);
        if (DVR_FAILED(res)) {
            DPrint(DPRINT_ERR, "DvrRecorderLoop::Set() Dvr_Recorder_SetConfig() with DVR_RECORDER_CFG_FATAL_ERROR failed! res = 0x%08x\n", res);
            return DVR_RES_EFAIL;
        }
	}
	break;

    case DVR_RECORDER_PROP_DAS_MOVE:
    {
		m_bMove = *(DVR_BOOL *)configData;
    }
    break;

	default:
		break;
	}

	return DVR_RES_SOK;
}

DVR_RESULT DvrRecorderLoop::Get(DVR_RECORDER_PROP type, void *configData, unsigned int size)
{
	int res;

	if (m_pEngine == NULL || configData == NULL)
		return DVR_RES_EPOINTER;

	switch (type)
	{
	case DVR_RECORDER_PROP_VIDEO_SPLIT_TIME:
	{
		res = Dvr_Recorder_GetConfig(m_pEngine, DVR_RECORDER_CFG_FILE_SPLIT_TIME, configData, size);
		if (DVR_FAILED(res)) {
			DPrint(DPRINT_ERR, "DvrRecorderLoop::Get() Dvr_Recorder_GetConfig() with DVR_RECORDER_CFG_FILE_SPLIT_TIME failed! res = 0x%08x\n", res);
			return DVR_RES_EFAIL;
		}
	}
	break;

	case DVR_RECORDER_PROP_VIDEO_QUALITY:
	{

	}
	break;

	case DVR_RECORDER_PROP_PHOTO_QUALITY:
	{

	}
	break;

    case DVR_RECORDER_PROP_EMERGENCY_SETTING:
    {
        res = Dvr_Recorder_GetConfig(m_pEngine, DVR_RECORDER_CFG_EMERGENCY_SETTING, configData, size);
        if (DVR_FAILED(res)) {
            DPrint(DPRINT_ERR, "DvrRecorderLoop::Get() Dvr_Recorder_GetConfig() with DVR_RECORDER_CFG_EMERGENCY_SETTING failed! res = 0x%08x\n", res);
            return DVR_RES_EFAIL;
        }
    }
    break;

	default:
		return DVR_RES_EINVALIDARG;
	}

	return DVR_RES_SOK;
}

DVR_RESULT DvrRecorderLoop::AddFrame(DVR_IO_FRAME *pInputFrame)
{
	int res;

	if (m_pEngine == NULL)
		return DVR_RES_EPOINTER;

	if (m_bExit)
		return DVR_RES_SOK;

	res = Dvr_Recorder_AddFrame(m_pEngine, pInputFrame);
	if (DVR_FAILED(res)) {
		DPrint(DPRINT_ERR, "DvrRecorderLoop::AddFrame() failed res = 0x%08x\n", res);
		return DVR_RES_EFAIL;
	}

	return DVR_RES_SOK;
}

DVR_RESULT	DvrRecorderLoop::AcauireInputBuf(void **ppvBuffer)
{
	int res;

	if (m_pEngine == NULL)
		return DVR_RES_EPOINTER;

	if (m_bExit)
	{
		*ppvBuffer = NULL;
		return DVR_RES_EFAIL;
	}

	res = Dvr_Recorder_AcquireInputBuf(m_pEngine, ppvBuffer);
	if (DVR_FAILED(res)) {
		DPrint(DPRINT_LOG, "DvrRecorderLoop::AcauireInputBuf() failed res = 0x%08x\n", res);
		return DVR_RES_EFAIL;
	}

	return DVR_RES_SOK;
}

DVR_RESULT	DvrRecorderLoop::AsyncOpFlush(void)
{
	int res;

	if (m_pEngine == NULL)
		return DVR_RES_EPOINTER;

	res = Dvr_Recorder_AsyncOpFlush(m_pEngine);
	if (DVR_FAILED(res)) {
		DPrint(DPRINT_LOG, "DvrRecorderLoop::AsyncOpFlush() failed res = 0x%08x\n", res);
		return DVR_RES_EFAIL;
	}

	return DVR_RES_SOK;
}

DVR_RESULT DvrRecorderLoop::EventRecStart()
{
    int res;

	if (m_pEngine == NULL)
		return DVR_RES_EPOINTER;

	DPrint(DPRINT_INFO, "Trigger Event Record\n");
    res = Dvr_Recorder_EventRec_Start(m_pEngine);
    if (DVR_FAILED(res)) {
        DPrint(DPRINT_ERR, "DvrRecorderLoop::EventRecStart() failed res = 0x%08x\n", res);
        return DVR_RES_EFAIL;
    }
	return DVR_RES_SOK;
}

DVR_RESULT DvrRecorderLoop::EventRecStop()
{
    int res;

    if (m_pEngine == NULL)
        return DVR_RES_EPOINTER;

    DPrint(DPRINT_INFO, "Stop Event Record\n");
    res = Dvr_Recorder_EventRec_Stop(m_pEngine);
    if (DVR_FAILED(res)) {
        DPrint(DPRINT_ERR, "DvrRecorderLoop::EventRecStop() failed res = 0x%08x\n", res);
        return DVR_RES_EFAIL;
    }
    return DVR_RES_SOK;
}

DVR_RESULT DvrRecorderLoop::LoopRecStart()
{
    int res;

    if (m_pEngine == NULL)
        return DVR_RES_EPOINTER;

    DPrint(DPRINT_INFO, "Start Loop Record\n");
    res = Dvr_Recorder_LoopRec_Start(m_pEngine);
    if (DVR_FAILED(res)) {
        DPrint(DPRINT_ERR, "DvrRecorderLoop::LoopRecStart() failed res = 0x%08x\n", res);
        return DVR_RES_EFAIL;
    }
    return DVR_RES_SOK;
}

DVR_RESULT DvrRecorderLoop::LoopRecStop()
{
    int res;

    if (m_pEngine == NULL)
        return DVR_RES_EPOINTER;

    DPrint(DPRINT_INFO, "Stop Loop Record\n");
    res = Dvr_Recorder_LoopRec_Stop(m_pEngine);
    if (DVR_FAILED(res)) {
        DPrint(DPRINT_ERR, "DvrRecorderLoop::LoopRecStop() failed res = 0x%08x\n", res);
        return DVR_RES_EFAIL;
    }
    return DVR_RES_SOK;
}

DVR_RESULT DvrRecorderLoop::DasRecStart()
{
    int res;

    if (m_pEngine == NULL)
        return DVR_RES_EPOINTER;

    DPrint(DPRINT_INFO, "Start Das Record\n");
    res = Dvr_Recorder_DasRec_Start(m_pEngine);
    if (DVR_FAILED(res)) {
        DPrint(DPRINT_ERR, "DvrRecorderDas::DasRecStart() failed res = 0x%08x\n", res);
        return DVR_RES_EFAIL;
    }
    return DVR_RES_SOK;
}

DVR_RESULT DvrRecorderLoop::DasRecStop()
{
    int res;

    if (m_pEngine == NULL)
        return DVR_RES_EPOINTER;

    DPrint(DPRINT_INFO, "Stop Das Record\n");
    res = Dvr_Recorder_DasRec_Stop(m_pEngine);
    if (DVR_FAILED(res)) {
        DPrint(DPRINT_ERR, "DvrRecorderDas::DasRecStop() failed res = 0x%08x\n", res);
        return DVR_RES_EFAIL;
    }
    return DVR_RES_SOK;
}

DVR_RESULT DvrRecorderLoop::IaccRecStart(int type)
{
    int res;

    if (m_pEngine == NULL)
        return DVR_RES_EPOINTER;

    DPrint(DPRINT_INFO, "Start IACC Record type %d\n",type);
    res = Dvr_Recorder_IaccRec_Start(m_pEngine);
    if (DVR_FAILED(res)) {
        DPrint(DPRINT_ERR, "DvrRecorderLoop::IaccRecStart() failed res = 0x%08x\n", res);
        return DVR_RES_EFAIL;
    }
    return DVR_RES_SOK;
}

DVR_RESULT DvrRecorderLoop::IaccRecStop(int type)
{
    int res;

    if (m_pEngine == NULL)
        return DVR_RES_EPOINTER;

    DPrint(DPRINT_INFO, "Stop IACC Record type %d\n",type);
    res = Dvr_Recorder_IaccRec_Stop(m_pEngine);
    if (DVR_FAILED(res)) {
        DPrint(DPRINT_ERR, "DvrRecorderLoop::IaccRecStop() failed res = 0x%08x\n", res);
        return DVR_RES_EFAIL;
    }
    return DVR_RES_SOK;
}

DVR_RESULT DvrRecorderLoop::Photo(DVR_PHOTO_PARAM *pParam)
{
	int res;

	if (m_pEngine == NULL)
		return DVR_RES_EPOINTER;

	DPrint(DPRINT_INFO, "Trigger Photo Shutter\n");
	
	res = Dvr_Recorder_TakePhoto(m_pEngine, pParam);
	if (DVR_FAILED(res)) {
		DPrint(DPRINT_ERR, "DvrRecorderLoop::Dvr_Recorder_TakePhoto() failed res = 0x%08x\n", res);
		return DVR_RES_EFAIL;
	}

	return DVR_RES_SOK;
}

DVR_RESULT DvrRecorderLoop::Overlay(void *canbuffer, void *osdbuffer,int cansize, int osdsize)
{
	int res;

	if (m_pEngine == NULL)
		return DVR_RES_EPOINTER;

	res = Dvr_Recorder_TakeOverlay(m_pEngine, canbuffer, osdbuffer, cansize, osdsize);
	if (DVR_FAILED(res)) {
		//DPrint(DPRINT_ERR, "DvrRecorderLoop::Dvr_Recorder_TakeOverlay() failed res = 0x%08x\n", res);
		return DVR_RES_EFAIL;
	}

	return DVR_RES_SOK;
}

int DvrRecorderLoop::Notify(void *pContext, DVR_NOTIFICATION_TYPE enuType, void *pParam1, void *pParam2)
{
	DvrRecorderLoop *p = (DvrRecorderLoop *)pContext;

	switch (enuType) {
	default:
		break;
	}

	return 0;
}

int DvrRecorderLoop::RecorderMessageCallback(void *pvParam1, void *pvParam2, void *pvParam3, void *pvContext)
{
	DvrRecorderLoop *p = (DvrRecorderLoop *)pvContext;
	RECORDER_EVENT enuEventCode = *(RECORDER_EVENT *)pvParam1;
	DVR_ADDTODB_FILE_INFO *ptFileInfo = (DVR_ADDTODB_FILE_INFO *)pvParam2;

	switch (enuEventCode)
	{
	case RECORDER_EVENT_NEW_FILE_START:
	{
		DPrint(DPRINT_LOG, "DvrRecorderLoop::RecorderMessageCallback() Start recording file [%s] at %ld\n", ptFileInfo->file_location, ptFileInfo->time_stamp);
		if (p->m_curRecFileName) {
			free(p->m_curRecFileName);
			p->m_curRecFileName = NULL;
		}

        if (ptFileInfo->file_location != NULL)
        {
		    p->m_curRecFileName = (char *)malloc(strlen(ptFileInfo->file_location) + 1);
		    strcpy(p->m_curRecFileName, ptFileInfo->file_location);
        }
	}
	break;

	case RECORDER_EVENT_NEW_FILE_FINISHED:
	{
		DPrint(DPRINT_LOG, "DvrRecorderLoop::RecorderMessageCallback() Finished recording file [%s], thumbnail file[%s] at %ld\n", 
            ptFileInfo->file_location, ptFileInfo->thumbnail_location, ptFileInfo->time_stamp);
		DVR_RECORDER_FILE_OP file_op;

		file_op.eEvent = DVR_FILE_EVENT_CREATE;
        if (ptFileInfo->file_location != NULL && ptFileInfo->thumbnail_location != NULL)
        {
            strcpy(file_op.szLocation, ptFileInfo->file_location);
            strcpy(file_op.szTNLocation, ptFileInfo->thumbnail_location);
            file_op.nTimeStamp = ptFileInfo->time_stamp;
            file_op.eType = ptFileInfo->eType;

            if(file_op.eType == DVR_FOLDER_TYPE_PHOTO && CheckPhotoStorage(pvContext, &file_op) != DVR_RES_SOK)
                return DVR_RES_EFAIL;

            if(p->m_bMove && file_op.eType == DVR_FOLDER_TYPE_DAS)
                MoveToEmergencuyFolder(&file_op);
            p->m_bMove = FALSE;
            OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_FILEEVENT, &file_op, sizeof(file_op), NULL, 0, DvrSystemControl::Notifier());
        }
	}
	break;

	case RECORDER_EVENT_STOP_DONE:
	{
		OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_RECORDER_STOP_DONE, NULL, 0, NULL, 0, DvrSystemControl::Notifier());
	}
	break;

    case RECORDER_EVENT_EMERGENCY_COMPLETE:
    {
    	DVR_BOOL bFailed = FALSE;
        OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_RECORDER_EMERGENCY_COMPLETE, &bFailed, sizeof(DVR_BOOL), NULL, 0, DvrSystemControl::Notifier());
    }
    break;

	case RECORDER_EVENT_ALARM_COMPLETE:
	{
		OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_RECORDER_ALARM_COMPLETE, NULL, 0, NULL, 0, DvrSystemControl::Notifier());
	}
	break;
	
	case RECORDER_EVENT_EVENTREC_FATAL_ERROR:
	{
		DPrint(DPRINT_ERR, "RECORDER_EVENT_EVENTREC_FATAL_ERROR\n");
		
		DVR_BOOL bFailed = TRUE;
        OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_RECORDER_EMERGENCY_COMPLETE, &bFailed, sizeof(DVR_BOOL), NULL, 0, DvrSystemControl::Notifier());
	}
	break;

    case RECORDER_EVENT_IACC_COMPLETE:
    {
        OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_RECORDER_IACC_COMPLETE, NULL, 0, NULL, 0, DvrSystemControl::Notifier());
    }
    break;
	
	case RECORDER_EVENT_IACCREC_FATAL_ERROR:
	{
		DPrint(DPRINT_ERR, "RECORDER_EVENT_IACCREC_FATAL_ERROR\n");
        //OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_RECORDER_IACC_COMPLETE, NULL, 0, NULL, 0, DvrSystemControl::Notifier());
	}
	break;

	case RECORDER_EVENT_LOOPREC_FATAL_ERROR:
	{
		DPrint(DPRINT_ERR, "RECORDER_EVENT_LOOPREC_FATAL_ERROR\n");
        OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_RECORDER_LOOPREC_FATAL_ERROR, NULL, 0, NULL, 0, DvrSystemControl::Notifier());
	}
	break;

	case RECORDER_EVENT_DASREC_FATAL_ERROR:
	{
		DPrint(DPRINT_ERR, "RECORDER_EVENT_DASREC_FATAL_ERROR\n");
        OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_RECORDER_DASREC_FATAL_ERROR, NULL, 0, NULL, 0, DvrSystemControl::Notifier());
	}
	break;

	case RECORDER_EVENT_LOOPREC_FATAL_RECOVER:
	{
		DPrint(DPRINT_ERR, "RECORDER_EVENT_LOOPREC_FATAL_RECOVER\n");
        OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_RECORDER_LOOPREC_FATAL_RECOVER, NULL, 0, NULL, 0, DvrSystemControl::Notifier());
	}
	break;

	case RECORDER_EVENT_SLOW_WRITING:
	{
		DPrint(DPRINT_ERR, "RECORDER_EVENT_SLOW_WRITING\n");
        OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_RECORDER_SLOW_WRITING, NULL, 0, NULL, 0, DvrSystemControl::Notifier());
	}
	break;

	case RECORDER_EVENT_PHOTO_DONE:
	{
		OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_PHOTO_DONE, NULL, 0, NULL, 0, DvrSystemControl::Notifier());
	}
	break;

	default:
		break;
	}

	return DVR_RES_SOK;
}

unsigned DvrRecorderLoop::ClipPeriod()
{
	if (m_pEngine == NULL)
		return 0;

	unsigned int period;
	int res = Dvr_Recorder_GetConfig(m_pEngine, DVR_RECORDER_CFG_FILE_SPLIT_TIME, &period, sizeof(period));
	if (DVR_FAILED(res)) {
		DPrint(DPRINT_ERR, "DvrRecorderLoop::DiskUsed() Dvr_Recorder_GetConfig() with DVR_RECORDER_CFG_FILE_SPLIT_TIME failed! res = 0x%08x\n", res);
		return 0;
	}

	return period;
}

DVR_RESULT DvrRecorderLoop::CheckPhotoStorage(void *pvContext, DVR_RECORDER_FILE_OP* file_op)
{
    if(pvContext == NULL || file_op == NULL)
        return DVR_RES_EFAIL;

	DvrRecorderLoop *p = (DvrRecorderLoop *)pvContext;
    if(p->m_sysCtrl == NULL)
        return DVR_RES_EFAIL;

    DVR_U32 nPhotoUsedSpace = 0;
    DVR_BOOL isPhotoFolderFull = FALSE;
	DVR_STORAGE_QUOTA nStorageQuota;

	Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_CARD_QUOTA, &nStorageQuota, sizeof(DVR_STORAGE_QUOTA), NULL);
    p->m_sysCtrl->StorageCard_Get(DVR_STORAGE_PROP_PHOTO_FOLDER_USEDSPACE, &nPhotoUsedSpace, sizeof(DVR_U32), NULL);
    if(nStorageQuota.nPhotoStorageQuota >= nPhotoUsedSpace)
    {
        isPhotoFolderFull = ((nStorageQuota.nPhotoStorageQuota - nPhotoUsedSpace) < DVR_STORAGE_PHOTO_FOLDER_WARNING_THRESHOLD);
    }
    else
    {
        isPhotoFolderFull = TRUE;
    }

    if (isPhotoFolderFull && file_op->szLocation != NULL)
    {
        printf("CheckPhotoStorage::OSA_FileDel %s\n", file_op->szLocation);
        DVR::OSA_FileDel(file_op->szLocation);
        return DVR_RES_EFAIL;
    }
    return DVR_RES_SOK;
}

float DvrRecorderLoop::Position()
{
	if (m_pEngine == NULL)
		return 0;

	unsigned int pos = 0;
	int res = Dvr_Recorder_GetPosition(m_pEngine, &pos);
	if (DVR_FAILED(res)) {
		DPrint(DPRINT_ERR, "DvrRecorderLoop::Position() Dvr_Recorder_GetPosition() failed! res = 0x%08x\n", res);
		return 0;
	}

	static unsigned int pre_period_in_ms = 0;
	static unsigned int pos_elapsed = 0;
	unsigned int cur_period_in_ms = 1000 * ClipPeriod();
	if (pre_period_in_ms != cur_period_in_ms)
	{
		pre_period_in_ms = cur_period_in_ms;
		pos_elapsed = pos;
	}
	
	pos = (pos - pos_elapsed) % cur_period_in_ms;

	return (pos/1000.0f);
}

DVR_RESULT DvrRecorderLoop::MoveToEmergencuyFolder(DVR_RECORDER_FILE_OP *file_op)
{
    if(file_op == NULL)
		return DVR_RES_EFAIL;

    DVR_RESULT res = DVR_RES_SOK;
    DVR_FILEMAP_META_ITEM stEventItem;
    char folder_name[APP_MAX_FN_SIZE];
    
    file_op->eType = DVR_FOLDER_TYPE_EMERGENCY;
    memset(folder_name, 0, sizeof(folder_name));
    memset(&stEventItem, 0, sizeof(DVR_FILEMAP_META_ITEM));
    Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_EMERGENCY_FOLDER, folder_name, sizeof(folder_name), NULL);
    
	format_move_file_fullname(file_op->szLocation, folder_name, stEventItem.szMediaFileName);
	format_move_file_fullname(file_op->szTNLocation, folder_name, stEventItem.szThumbNailName);

    res |= DVR::OSA_FileMove(file_op->szLocation, stEventItem.szMediaFileName);
    //res |= DVR::OSA_FileMove(file_op->szTNLocation, stEventItem.szThumbNailName);

    strncpy(file_op->szLocation, stEventItem.szMediaFileName, APP_MAX_FN_SIZE);
    strncpy(file_op->szTNLocation, stEventItem.szThumbNailName, APP_MAX_FN_SIZE);
    DPrint(DPRINT_INFO, "copy file %s to %s complete, res = 0x%x\n", file_op->szLocation, folder_name, res);
    return res;
}

void *DvrRecorderLoop::TimerCallback(void *arg)
{
	DvrRecorderLoop *p = (DvrRecorderLoop *)arg;

#if 0
	float position = p->Position();
	printf("current file: %s, position = %.3fs\n", p->m_curRecFileName, position);
#endif

	return (void *)0;
}


DVR_RESULT DvrRecorderLoop::LoopEnc_Storage_SearchOldestFile(DVR_U32 param1, DVR_U32 param2, void *pContext)
{
    DVR_RESULT res = DVR_RES_SOK;

    if (pContext == NULL)
        return DVR_RES_EFAIL;

    DVR_FOLDER_TYPE eFolderType = (DVR_FOLDER_TYPE)param1;
    DVR_FILE_TYPE eType = (DVR_FILE_TYPE)param2;
    DvrRecorderLoop *pThis = (DvrRecorderLoop *)pContext;
    DvrSystemControl *pSysCtrl = (DvrSystemControl *)pThis->m_sysCtrl;

    if (pSysCtrl == NULL)
        return DVR_RES_EFAIL;

    if (eType == DVR_FILE_TYPE_VIDEO)
    {
        APPLIB_ADDFLAGS(pThis->m_StorageHandleFlag, DVR_LOOP_ENC_TYPE_VIDEO);
    }
    else
    {
        APPLIB_ADDFLAGS(pThis->m_StorageHandleFlag, DVR_LOOP_ENC_TYPE_IMAGE);
    }

    if (DVR_RES_SOK == pSysCtrl->FileMapDB_GetNameByRelPos(0, eFolderType, &pThis->m_fileToDelete, sizeof(pThis->m_fileToDelete)))
    {
        DVR_DB_IDXFILE idxfile;
        if(DVR_RES_SOK == pSysCtrl->FileMapDB_GetFileInfo(pSysCtrl->CurrentMountPoint(), pThis->m_fileToDelete.szMediaFileName, eFolderType, &idxfile))
    	{
			APPLIB_ADDFLAGS(pThis->m_StorageHandleFlag, DVR_LOOP_ENC_SEARCH_DONE);
		}
		else
		{
			APPLIB_ADDFLAGS(pThis->m_StorageHandleFlag, DVR_LOOP_ENC_SEARCH_ERROR);
		}
    }
    else
    {
        APPLIB_ADDFLAGS(pThis->m_StorageHandleFlag, DVR_LOOP_ENC_SEARCH_ERROR);
        return DVR_RES_EFAIL;
    }

    return DVR_RES_SOK;
}

DVR_RESULT DvrRecorderLoop::LoopEnc_Storage_DeleteFile(DVR_U32 param1, DVR_U32 param2, void *pContext)
{
    DVR_RESULT res = DVR_RES_SOK;

    DVR_FOLDER_TYPE eFolderType = (DVR_FOLDER_TYPE)param1;
    DVR_FILE_TYPE eType = (DVR_FILE_TYPE)param2;
    DvrRecorderLoop *pThis = (DvrRecorderLoop *)pContext;
    DvrSystemControl *pSysCtrl = (DvrSystemControl *)pThis->m_sysCtrl;

    /**if search fail skip delete file*/
    if (APPLIB_CHECKFLAGS(pThis->m_StorageHandleFlag, DVR_LOOP_ENC_SEARCH_ERROR)) {
        return res;
    }

    DVR_DB_IDXFILE idxfile;
    if (DVR_RES_SOK == pSysCtrl->FileMapDB_GetFileInfo(pSysCtrl->CurrentMountPoint(), pThis->m_fileToDelete.szMediaFileName, eFolderType, &idxfile))
    {
        res = DVR::OSA_FileDel(pThis->m_fileToDelete.szMediaFileName);
        if (res == DVR_RES_SOK)
        {
            APPLIB_ADDFLAGS(pThis->m_StorageHandleFlag, DVR_LOOP_ENC_HANDLE_DONE);
        }
        else
        {
            APPLIB_ADDFLAGS(pThis->m_StorageHandleFlag, DVR_LOOP_ENC_HANDLE_ERROR);
        }

        if (res == DVR_RES_SOK)
        {
            pSysCtrl->FileMapDB_DelItem(pThis->m_fileToDelete.szMediaFileName, eFolderType);
        }
    }
	else
	{		
		DPrint(DPRINT_ERR, "LoopEnc_Storage_DeleteFile FileMapDB_GetFileInfo[%s] return error!!!", pThis->m_fileToDelete.szMediaFileName);
	}

    return res;
}

DVR_RESULT DvrRecorderLoop::LoopEnc_Storage_Return(DVR_U32 param1, DVR_U32 param2, void *pContext)
{
    DvrRecorderLoop *pThis = (DvrRecorderLoop *)pContext;
    DvrSystemControl *pSysCtrl = (DvrSystemControl *)pThis->m_sysCtrl;

    if (APPLIB_CHECKFLAGS(pThis->m_StorageHandleFlag, DVR_LOOP_ENC_SEARCH_ERROR))
    {
        pSysCtrl->ComSvcHcmgr_SendMsg(HMSG_STORAGE_RUNOUT_HANDLE_ERROR, 0, 0);
    }
    else if (APPLIB_CHECKFLAGS(pThis->m_StorageHandleFlag, DVR_LOOP_ENC_HANDLE_ERROR))
    {
        pSysCtrl->ComSvcHcmgr_SendMsg(HMSG_STORAGE_RUNOUT_HANDLE_ERROR, 1, 0);
    }
    else if (APPLIB_CHECKFLAGS(pThis->m_StorageHandleFlag, DVR_LOOP_ENC_SEARCH_DONE) &&
        APPLIB_CHECKFLAGS(pThis->m_StorageHandleFlag, DVR_LOOP_ENC_HANDLE_DONE))
    {
        if (APPLIB_CHECKFLAGS(pThis->m_StorageHandleFlag, DVR_LOOP_ENC_TYPE_IMAGE))
        {
            pSysCtrl->ComSvcHcmgr_SendMsg(HMSG_STORAGE_RUNOUT_HANDLE_DONE, param1, DVR_FILE_TYPE_IMAGE);
        }
        else
        {
            pSysCtrl->ComSvcHcmgr_SendMsg(HMSG_STORAGE_RUNOUT_HANDLE_DONE, param1, DVR_FILE_TYPE_VIDEO);
        }
    }

    pThis->m_StorageHandleFlag = 0;
    memset(&pThis->m_fileToDelete, 0, sizeof(pThis->m_fileToDelete));

    return DVR_RES_SOK;
}


DVR_RESULT DvrRecorderLoop::DAS_Storage_DeleteFile(DVR_U32 param1, DVR_U32 param2, void *pContext)
{
    DVR_RESULT res = DVR_RES_SOK;

    DVR_FILE_TYPE fileType;
    unsigned id = 0;

    if (pContext == NULL)
        return DVR_RES_EFAIL;

    DVR_DAS_TYPE eType = (DVR_DAS_TYPE)param2;
    DvrRecorderLoop *pThis = (DvrRecorderLoop *)pContext;
    DvrSystemControl *pSysCtrl = (DvrSystemControl *)pThis->m_sysCtrl;

    if (pSysCtrl == NULL || eType >= DVR_DAS_TYPE_NUM)
        return DVR_RES_EFAIL;

    uint type = eType - DVR_DAS_TYPE_APA;
    if (DVR_RES_SOK == pSysCtrl->FileMapDB_GetDasOldNameByType(pThis->m_DasfileToDelete[type].szMediaFileName, eType))
    {
        res = DVR::OSA_FileDel(pThis->m_DasfileToDelete[type].szMediaFileName);
        if (res == DVR_RES_SOK)
        {
            APPLIB_ADDFLAGS(pThis->m_StorageHandleFlag, DVR_LOOP_ENC_HANDLE_DONE);
        }
        else
        {
            APPLIB_ADDFLAGS(pThis->m_StorageHandleFlag, DVR_LOOP_ENC_HANDLE_ERROR);
        }

        if (res == DVR_RES_SOK)
        {
            pSysCtrl->FileMapDB_DelItem(pThis->m_fileToDelete.szMediaFileName, DVR_FOLDER_TYPE_DAS);
        }
        else
        {
            DPrint(DPRINT_ERR, "DAS_Storage_DeleteFile type %d [%s] return error!!!", type, pThis->m_DasfileToDelete[type].szMediaFileName);
            return DVR_RES_EFAIL;
        }
    }
    else
    {
        DPrint(DPRINT_ERR, "DAS_Storage_DeleteFile VideoDB_GetDasOldNameByType type %d [%s] return error!!!",type, pThis->m_DasfileToDelete[type].szMediaFileName);
        return DVR_RES_EFAIL;
    }

    memset(pThis->m_DasfileToDelete, 0, (DVR_DAS_TYPE_NUM - DVR_DAS_TYPE_APA) * sizeof(DVR_FILEMAP_META_ITEM));
    pSysCtrl->ComSvcHcmgr_SendMsg(HMSG_STORAGE_RUNOUT_HANDLE_DONE, param1, DVR_FILE_TYPE_VIDEO);
    return DVR_RES_SOK;
}

//#define DEFAULT_BADFILE_THUMBNAIL "/opt/avm/svres/badFile.bmp"

DVR_RESULT DvrRecorderLoop::LoadThumbNail(char *filename, unsigned char *pPreviewBuf, int nSize)
{
    BMPIMAGE out_img;
	int result = 0;
	char tmpFileName[PATH_MAX] = {0};
	
    //CJpegThumb* jpegthumb = new CJpegThumb();
    //result = jpegthumb->MakeThumb(filename, pPreviewBuf, nSize);
    //delete jpegthumb;
    //return result;

	memset(&out_img, 0, sizeof(BMPIMAGE));

	DVR_U64 ullFileSize = 0;
	int ret = DVR::OSA_GetFileStatInfo(filename, &ullFileSize, NULL);
	if(ret == DVR_RES_SOK && ullFileSize != 0)
	{
		strncpy(tmpFileName, filename, PATH_MAX);
	}
	else
	{
		strncpy(tmpFileName, DEFAULT_BADFILE_THUMBNAIL, PATH_MAX);
	}

	result = LoadBMP(tmpFileName, &out_img, BIT24);
	if(result == 0)
	{
		DVR_U32 outSize = out_img.height * out_img.width * out_img.channels;
		if (nSize < outSize)
			return DVR_RES_EFAIL;
		
		memcpy(pPreviewBuf, out_img.data, outSize);
		
		freeImage(&out_img);
	}

    return DVR_RES_SOK;
}

