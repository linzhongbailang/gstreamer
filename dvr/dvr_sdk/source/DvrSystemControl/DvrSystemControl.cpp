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
#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#pragma warning(disable:4996)	// 4706: assignment within conditional expression
#endif

#ifdef __linux__
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mount.h>
#include "DvrMonitor_Mount.h"
#else
#include <stdlib.h>
#include <string.h>
#ifndef strcasecmp
#define strcasecmp _stricmp
#endif
#include "DvrMonitor_Mount_Win32.h"
#endif
#include <gst/gst.h>
#include "DVR_SDK_DEF.h"

#include "dprint.h"
#include "DvrSystemControl.h"
#include "DvrMutex.h"
#include "DvrMutexLocker.h"
#include "DvrPlayerLoop.h"
#include "DvrRecorderLoop.h"
#include "DvrComSvc_HcMgr.h"
#include "DvrComSvc_Timer.h"
#include "DvrComSvc_AsyncOp.h"
#include "DvrComSvc_SyncOp.h"
#include "DvrStorage_AsyncOp.h"
#include "DvrStorage_Card.h"
#include "DvrMonitor_Storage.h"
#include "osa_mark.h"

#define FILESUM(pdi, type) ((pdi)->m_pExtArray[(type)].m_uMax - (pdi)->m_pExtArray[(type)].m_uMin)
#define SETRANGE(range, pdi, type) \
    do\
		    { (range)[(type)].uMinId = (pdi)->m_pExtArray[(type)].m_uMin;\
      (range)[(type)].uMaxId = (pdi)->m_pExtArray[(type)].m_uMax;} while (0)

const char *m_szMeta[] =
{
	"Name",
	"Description",
	"Duration",
	"Application",
};

const char *g_szPhotoMeta[] =
{
    "Name",
    "ID",
    "FrontName",
    "RearName",
    "LeftName",
    "RightName"
};

const char *g_szVideoMeta[] =
{
    "Name",
    "DisplayName",
    "FolderType"
};

const char *g_szFileMapMeta[] =
{
    "Name",
    "ThumbNailFileName",
    "DirId"
};

OsaNotifier *DvrSystemControl::m_pNotifier = NULL;
unsigned int DvrSystemControl::m_photoIndex = 0;

static DVR_RESULT Dvr_Pre_Initialize(void)
{
	gst_init(NULL, NULL);

	#if 0
    GstElement *pVideoAppSrc = gst_element_factory_make("appsrc", "source");
    GstElement *pVideoParse = gst_element_factory_make("h264parse", "parser");
    GstElement *pQtMuxer = gst_element_factory_make("qtmux", "muxer");
    GstElement *pSink = gst_element_factory_make("splitmuxsink", "sink");	
    GstElement *pVideoEnc = gst_element_factory_make("ducatih264enc", "video_encoder");
    GstElement *pVideoSink = gst_element_factory_make("appsink", "video_sink");
	#endif

	return DVR_RES_SOK;
}

DvrSystemControl::DvrSystemControl()
{
	m_pAutoMount                = NULL;
    m_pFileDBMgr                = NULL;
	m_pCurrentMountPoint        = NULL;
	m_pCurrentDevicePath        = NULL;
	m_player                    = NULL;
	m_recorder                  = NULL;
	m_pComSvcHcMgr              = new DvrComSvcHcMgr();
	m_pComSvcTimer              = new DvrComSvcTimer();
	m_pComSvcAsyncOp            = new DvrComSvcAsyncOp(this);
    m_pComSvcSyncOp             = new DvrComSvcSyncOp(this);
	m_pStorageAsyncOp           = new DvrStorageAsyncOp();
	m_pStorageCard              = new DvrStorageCard(this);
	m_pStorageMonitor           = new DvrMonitorStorage(this);
	m_ptfnNewFrameCallBack      = NULL;
	m_pNewFrameCallBackContext  = NULL;
	m_recorderMutex             = new DvrMutex();
	m_playerMutex               = new DvrMutex();
	m_mediaMutex                = new DvrMutex();
	m_fileMapDBMutex            = new DvrMutex();
	m_pNotifier                 = new OsaNotifier();
	m_ptfnRecorderUpdateVehicleData     = NULL;
	m_pRecorderUpdateVehicleDataContext = NULL;
	m_ptfnPlayerUpdateVehicleData       = NULL;
	m_pPlayerUpdateVehicleDataContext   = NULL;
    m_GSTconsoleCallbckIsSet = false; 
	DVR_RESULT res = m_pNotifier->Open();
	if (DVR_FAILED(res)) {
		DPrint(DPRINT_ERR, "DvrSystemControl::Open() OsaNotifier::Open() failed! res = 0x%08x\n", res);
	}

	m_pNotifier->Register(Notify, this);
	Dvr_Pre_Initialize();
    
}

DvrSystemControl::~DvrSystemControl(void)
{
	if (m_pNotifier) {
		m_pNotifier->Close();
		delete m_pNotifier;
		m_pNotifier = NULL;
	}

	if(m_pCurrentMountPoint){
		free(m_pCurrentMountPoint);
		m_pCurrentMountPoint = NULL;
	}

	if(m_pCurrentDevicePath){
		free(m_pCurrentDevicePath);
		m_pCurrentDevicePath = NULL;
	}

	

	PlayerDeInit();
	RecorderDeInit();

	delete m_mediaMutex;
	delete m_recorderMutex;
	delete m_playerMutex;
	delete m_fileMapDBMutex;
    delete m_errorFileMutex;
	delete m_pComSvcAsyncOp;
    delete m_pComSvcSyncOp;
	delete m_pComSvcTimer;
	delete m_pStorageAsyncOp;
	delete m_pStorageCard;
	delete m_pStorageMonitor;
	delete m_pComSvcHcMgr;
    gst_deinit();
}

DVR_RESULT DvrSystemControl::RegisterNotify(PFN_DVR_SDK_NOTIFY pCallback, void *pContext)
{
	DVR_RESULT res = DVR_RES_SOK;

	res = m_pNotifier->Register(pCallback, pContext);

	return res;
}

DVR_RESULT DvrSystemControl::UnRegisterNotify(PFN_DVR_SDK_NOTIFY pCallback, void *pContext)
{
	DVR_RESULT res = DVR_RES_SOK;

	if (m_pNotifier == NULL)
		return DVR_RES_EUNEXPECTED;

	res = m_pNotifier->UnRegister(pCallback, pContext);

	return res;
}


DVR_RESULT DvrSystemControl::RegisterConsole(PFN_DVR_SDK_CONSOLE pCallback, void *pContext)
{
	if (g_consoleCallback != NULL)
		return DVR_RES_EFAIL;

	g_consoleCallback = pCallback;
	g_consoleCallbackContext = pContext;

	return DVR_RES_SOK;
}

extern "C" 
{
void gst_debug_log_config(PFN_DVR_SDK_CONSOLE pCallback, void *pContext );
}
DVR_RESULT DvrSystemControl::RegisterGSTConsole(PFN_DVR_SDK_CONSOLE pCallback, void *pContext)
{
	if (m_GSTconsoleCallbckIsSet == true)
		return DVR_RES_EFAIL;

    m_GSTconsoleCallbckIsSet=true;
    
    /**************call gst api*/
    gst_debug_log_config(pCallback,pContext);

	return DVR_RES_SOK;
}






DVR_RESULT DvrSystemControl::UnregisterConsole(PFN_DVR_SDK_CONSOLE pCallback, void *pContext)
{
	if (g_consoleCallback != pCallback || g_consoleCallbackContext != pContext)
		return DVR_RES_EFAIL;

	g_consoleCallbackContext = NULL;
	g_consoleCallback = NULL;

	return DVR_RES_SOK;
}

DVR_RESULT DvrSystemControl::EnumDevices(DVR_DEVICE_TYPE enuType, DVR_DEVICE_ARRAY *pDevices)
{
	DvrMutexLocker mutexLocker(m_mediaMutex);

	if (pDevices == NULL)
		return DVR_RES_EPOINTER;
	memset(pDevices, 0, sizeof(DVR_DEVICE_ARRAY));

	DPrint(DPRINT_LOG, "DvrSystemControl::EnumDevices() enuType = 0x%08x pDevices = %p\n", enuType, pDevices);

	pDevices->nDeviceNum = 0;

	// FIXME: thread protection
	std::list<DVR_DEVICE>::iterator it;
	for (it = m_devices.begin(); it != m_devices.end(); it++) {
		if (it->eType == enuType || enuType == DVR_DEVICE_TYPE_ALL) {
			void *p = malloc(sizeof(DVR_DEVICE));
			if (p == NULL) {
				for (int i = 0; i < pDevices->nDeviceNum; i++)
					free(pDevices->pDriveArray[i]);
				memset(pDevices, 0, sizeof(DVR_DEVICE_ARRAY));
				return DVR_RES_EOUTOFMEMORY;
			}

			pDevices->pDriveArray[pDevices->nDeviceNum] = (DVR_DEVICE *)p;
			*(pDevices->pDriveArray[pDevices->nDeviceNum]) = *it;
			pDevices->nDeviceNum++;
		}
	}

	DPrint(DPRINT_LOG, "DvrSystemControl::EnumDevices() %d devices connected\n", m_devices.size());
	return DVR_RES_SOK;
}

DVR_RESULT DvrSystemControl::SetActiveDrive(DVR_DEVICE *pszDrive)
{
	DvrMutexLocker mutexLocker(m_mediaMutex);

	if (pszDrive == NULL) {
		return DVR_RES_EPOINTER;
	}

	if(m_pCurrentMountPoint){
		free(m_pCurrentMountPoint);
		m_pCurrentMountPoint = NULL;
	}

	if(m_pCurrentDevicePath){
		free(m_pCurrentDevicePath);
		m_pCurrentDevicePath = NULL;
	}

	if(strcmp(pszDrive->szMountPoint, ""))
		m_pCurrentMountPoint = strdup(pszDrive->szMountPoint);

	if(strcmp(pszDrive->szDevicePath, ""))
		m_pCurrentDevicePath = strdup(pszDrive->szDevicePath);

	if(m_pCurrentMountPoint != NULL)
	{
		m_pStorageCard->DvrStorageCard_SetRoot(m_pCurrentMountPoint);
		m_pStorageCard->DvrStorageCard_SetCardStatus(FALSE);
	}

	return DVR_RES_SOK;
}

DVR_RESULT DvrSystemControl::ClearActiveDrive()
{
	DvrMutexLocker mutexLocker(m_mediaMutex);

	if(m_pCurrentMountPoint){
		free(m_pCurrentMountPoint);
		m_pCurrentMountPoint = NULL;
	}

	if(m_pCurrentDevicePath){
		free(m_pCurrentDevicePath);
		m_pCurrentDevicePath = NULL;
	}

	m_pStorageCard->DvrStorageCard_SetCardStatus(TRUE);

	return DVR_RES_SOK;
}

DVR_RESULT DvrSystemControl::GetActiveDrive(DVR_DEVICE *pszDrive)
{
	DvrMutexLocker mutexLocker(m_mediaMutex);

	if(pszDrive == NULL){
		return DVR_RES_EPOINTER;
	}

	if(m_pCurrentMountPoint != NULL)
	{
		strncpy(pszDrive->szMountPoint, m_pCurrentMountPoint, PATH_MAX);
	}

	if(m_pCurrentDevicePath != NULL)
	{
		strncpy(pszDrive->szDevicePath, m_pCurrentDevicePath, PATH_MAX);
	}

	return DVR_RES_SOK;
}

DVR_RESULT DvrSystemControl::MediaMgrOpen()
{
	DvrMutexLocker mutexLocker(m_mediaMutex);

	DPrint(DPRINT_LOG, "DvrSystemControl::MediaMgrOpen() Initializing the SDK\n");

    m_pFileDBMgr  = new IFileDBManger(m_pNotifier);
	if (m_pFileDBMgr == 0) {
		DPrint(DPRINT_ERR, "DvrSystemControl::Open() Failed to create IFileDBManager m_pFileDBMgr = 0x%08x\n",m_pFileDBMgr);
		return DVR_RES_EFAIL;
	}
#ifdef __linux__
	m_pAutoMount = new DvrMonitorMount(m_pNotifier, this);
	m_pAutoMount->StartMonitor(&m_devices);
#else
	m_pAutoMount = new DvrMonitorMount_Win32(m_pNotifier);
	m_pAutoMount->StartMonitor(&m_devices);
#endif

	return DVR_RES_SOK;
}

DVR_RESULT DvrSystemControl::MediaMgrClose()
{
	DvrMutexLocker mutexLocker(m_mediaMutex);

	DPrint(DPRINT_LOG, "DvrSystemControl::MediaMgrClose() Uninitializing the SDK\n");

	if (m_pAutoMount) 
    {
		m_pAutoMount->StopMonitor();
		delete m_pAutoMount;
		m_pAutoMount = NULL;
	}
    FSDel(m_pFileDBMgr);
	return DVR_RES_SOK;
}

DVR_RESULT DvrSystemControl::MediaMgrReset()
{
	DVR_RESULT res = DVR_RES_SOK;

	if (m_pCurrentMountPoint == NULL){
		return DVR_RES_EFAIL;
	}

	if (m_pStorageCard != NULL)
	{
		m_pStorageCard->DvrStorageCard_SetRoot(m_pCurrentMountPoint);
	}

	DB_Unmount(m_pCurrentMountPoint);
	return DB_Mount(m_pCurrentMountPoint,m_pCurrentDevicePath, 0);
}

DVR_RESULT DvrSystemControl::RecorderInit()
{
	if(m_recorder != NULL)
		return DVR_RES_EFAIL;
	
	m_recorder = new DvrRecorderLoop(this);
	if (m_recorder == NULL)
		return DVR_RES_EOUTOFMEMORY;

	return DVR_RES_SOK;
}

DVR_RESULT DvrSystemControl::RecorderDeInit()
{
	if(m_recorder != NULL)
	{
	    DvrRecorderLoop *tmp = m_recorder;

	    m_recorder = NULL;

		m_ptfnRecorderUpdateVehicleData = NULL;
		m_pRecorderUpdateVehicleDataContext = NULL;

	    delete tmp;
	}

	return DVR_RES_SOK;
}

DVR_RESULT DvrSystemControl::RecorderReset()
{
	DvrMutexLocker mutexLocker(m_recorderMutex);

	if (m_recorder == NULL) {
		return DVR_RES_EPOINTER;
	}

	return m_recorder->Reset();
}

DVR_RESULT DvrSystemControl::RecorderStart()
{
	DvrMutexLocker mutexLocker(m_recorderMutex);

	if (m_recorder == NULL) {
		return DVR_RES_EPOINTER;
	}

	return m_recorder->Start();
}

DVR_RESULT DvrSystemControl::RecorderStop()
{
	DvrMutexLocker mutexLocker(m_recorderMutex);

	if (m_recorder == NULL) {
		return DVR_RES_EPOINTER;
	}

	return m_recorder->Stop();
}

DVR_RESULT DvrSystemControl::RecorderPhotoTake(DVR_PHOTO_PARAM *pParam)
{
	DvrMutexLocker mutexLocker(m_recorderMutex);
	
	if (m_recorder == NULL) {
		return DVR_RES_EPOINTER;
	}

	return m_recorder->Photo(pParam);
}

DVR_RESULT DvrSystemControl::RecorderTakeOverlay(void *canbuffer, void *osdbuffer,int cansize, int osdsize)
{
	DvrMutexLocker mutexLocker(m_recorderMutex);
	
	if (m_recorder == NULL) {
		return DVR_RES_EPOINTER;
	}

	return m_recorder->Overlay(canbuffer,osdbuffer, cansize, osdsize);
}

DVR_RESULT DvrSystemControl::RecorderLoopRecStart()
{
    DvrMutexLocker mutexLocker(m_recorderMutex);

    if (m_recorder == NULL) {
        return DVR_RES_EPOINTER;
    }

    return m_recorder->LoopRecStart();
}

DVR_RESULT DvrSystemControl::RecorderLoopRecStop()
{
    DvrMutexLocker mutexLocker(m_recorderMutex);

    if (m_recorder == NULL) {
        return DVR_RES_EPOINTER;
    }

    return m_recorder->LoopRecStop();
}

DVR_RESULT DvrSystemControl::RecorderEventRecStart(void)
{
	DvrMutexLocker mutexLocker(m_recorderMutex);

	if (m_recorder == NULL) {
		return DVR_RES_EPOINTER;
	}

	return m_recorder->EventRecStart();
}

DVR_RESULT DvrSystemControl::RecorderEventRecStop(void)
{
    DvrMutexLocker mutexLocker(m_recorderMutex);

    if (m_recorder == NULL) {
        return DVR_RES_EPOINTER;
    }

    return m_recorder->EventRecStop();
}

DVR_RESULT DvrSystemControl::RecorderDasRecStart()
{
	DvrMutexLocker mutexLocker(m_recorderMutex);

	if (m_recorder == NULL) {
		return DVR_RES_EPOINTER;
	}

	return m_recorder->DasRecStart();
}

DVR_RESULT DvrSystemControl::RecorderDasRecStop()
{
    DvrMutexLocker mutexLocker(m_recorderMutex);

    if (m_recorder == NULL) {
        return DVR_RES_EPOINTER;
    }

    return m_recorder->DasRecStop();
}

DVR_RESULT DvrSystemControl::RecorderIaccRecStart(int type)
{
    DvrMutexLocker mutexLocker(m_recorderMutex);

    if (m_recorder == NULL) {
        return DVR_RES_EPOINTER;
    }

    return m_recorder->IaccRecStart(type);
}

DVR_RESULT DvrSystemControl::RecorderIaccRecStop(int type)
{
    DvrMutexLocker mutexLocker(m_recorderMutex);

    if (m_recorder == NULL) {
        return DVR_RES_EPOINTER;
    }

    return m_recorder->IaccRecStop(type);
}

DVR_RESULT DvrSystemControl::RecorderAddFrame(DVR_IO_FRAME *pInputFrame)
{
	//DvrMutexLocker mutexLocker(m_recorderMutex);

	if(m_ptfnRecorderUpdateVehicleData != NULL)
	{	
		m_ptfnRecorderUpdateVehicleData(m_pRecorderUpdateVehicleDataContext, pInputFrame->pCanBuf);
	}

	if (m_recorder == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_recorder->AddFrame(pInputFrame);
}

DVR_RESULT DvrSystemControl::RecorderAcquireInputBuf(void **ppImgBuf)
{
	//DvrMutexLocker mutexLocker(m_recorderMutex);

	if (m_recorder == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_recorder->AcauireInputBuf(ppImgBuf);
}

DVR_RESULT DvrSystemControl::RecorderAsyncOpFlush(void)
{
	DvrMutexLocker mutexLocker(m_recorderMutex);

	if (m_recorder == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_recorder->AsyncOpFlush();
}

DVR_RESULT DvrSystemControl::RecorderSet(DVR_RECORDER_PROP prop, void *propData, int propSize)
{
	//DvrMutexLocker mutexLocker(m_recorderMutex);

	if (m_recorder == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_recorder->Set(prop, propData, propSize);
}

DVR_RESULT DvrSystemControl::RecorderGet(DVR_RECORDER_PROP prop, void *propData, int propSize, int *pnSizeReturned)
{
	//DvrMutexLocker mutexLocker(m_recorderMutex);

	if (m_recorder == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_recorder->Get(prop, propData, propSize);
}

DVR_RESULT DvrSystemControl::RecorderRegisterVehDataUpdateCallBack(PFN_NEW_VEHICLE_DATA callback, void *pContext)
{
	DvrMutexLocker mutexLocker(m_recorderMutex);

	m_ptfnRecorderUpdateVehicleData = callback;
	m_pRecorderUpdateVehicleDataContext = pContext;
	
	return DVR_RES_SOK;
}

DVR_RESULT DvrSystemControl::PlayerInit()
{
    if(m_player)
        delete m_player;
	m_player = new DvrPlayerLoop(this);
	if (m_player == NULL)
		return DVR_RES_EOUTOFMEMORY;

	return DVR_RES_SOK;
}

DVR_RESULT DvrSystemControl::PlayerDeInit()
{
	DvrPlayerLoop *tmp = m_player;

	m_player = NULL;

	m_ptfnPlayerUpdateVehicleData = NULL;
	m_pPlayerUpdateVehicleDataContext = NULL;

    delete tmp;

	return DVR_RES_SOK;
}

DVR_RESULT DvrSystemControl::PlayerOpen(const char *fileName, int type)
{
	DvrMutexLocker mutexLocker(m_playerMutex);
	if (fileName == NULL || m_player == NULL)
		return DVR_RES_EPOINTER;

	if (m_pCurrentMountPoint == NULL)
	{
		DPrint(DPRINT_ERR, "DvrSystemControl::PlayerOpen(): The current drive is not selected\n");
		return DVR_RES_EFAIL;
	}

    if (m_pFileDBMgr == NULL)
    {
        DPrint(DPRINT_ERR, "%s: IFileDBManager is not created! Cannot do operation.\n", __func__);
        return DVR_RES_EABORT;
    }

	m_player->RegisterCallBack(DvrPlayerOnNewFrame, this);
	return m_player->Loop_Setup(fileName, DVR_FILE_TYPE_VIDEO, (DVR_FOLDER_TYPE)type);
}

DVR_RESULT DvrSystemControl::PlayerClose()
{
	DvrMutexLocker mutexLocker(m_playerMutex);

	if (m_player == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_player->Close();
}

DVR_RESULT DvrSystemControl::PlayerPlay()
{
	DvrMutexLocker mutexLocker(m_playerMutex);

	if (m_player == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_player->Play();
}

DVR_RESULT DvrSystemControl::PlayerStop()
{
	DvrMutexLocker mutexLocker(m_playerMutex);

	if (m_player == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_player->Stop();
}

DVR_RESULT DvrSystemControl::PlayerPause()
{
	DvrMutexLocker mutexLocker(m_playerMutex);

	if (m_player == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_player->Pause();
}

DVR_RESULT DvrSystemControl::PlayerResume()
{
	DvrMutexLocker mutexLocker(m_playerMutex);

	if (m_player == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_player->Resume();
}

DVR_RESULT DvrSystemControl::PlayerNext()
{
	DvrMutexLocker mutexLocker(m_playerMutex);

	if (m_player == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_player->Loop_Next();
}

DVR_RESULT DvrSystemControl::PlayerPrev()
{
	DvrMutexLocker mutexLocker(m_playerMutex);

	if (m_player == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_player->Loop_Prev();
}

DVR_RESULT DvrSystemControl::PlayerStepF()
{
	DvrMutexLocker mutexLocker(m_playerMutex);

	if (m_player == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_player->Loop_StepF();
}

DVR_RESULT DvrSystemControl::PlayerStepB()
{
	DvrMutexLocker mutexLocker(m_playerMutex);

	if (m_player == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_player->Loop_StepB();
}


DVR_RESULT DvrSystemControl::PlayerDelta(int nDelta)
{
    DvrMutexLocker mutexLocker(m_playerMutex);

    if (m_player == NULL) {
        return DVR_RES_EPOINTER;
    }
    return m_player->Loop_Delta(nDelta);
}

DVR_RESULT DvrSystemControl::PlayerSet(DVR_PLAYER_PROP prop, void *propData, int propSize)
{
	DvrMutexLocker mutexLocker(m_playerMutex);

	if (m_player == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_player->Set(prop, propData, propSize);
}

DVR_RESULT DvrSystemControl::PlayerGet(DVR_PLAYER_PROP prop, void *propData, int propSize, int *propSizeReturned)
{
	DvrMutexLocker mutexLocker(m_playerMutex);

	if (m_player == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_player->Get(prop, propData, propSize);
}


DVR_RESULT DvrSystemControl::PlayerPrintScreen(DVR_PHOTO_PARAM *pParam)
{
	DvrMutexLocker mutexLocker(m_playerMutex);

	if (m_player == NULL) {
		return DVR_RES_EPOINTER;
	}
	
	return m_player->PrintScreen(pParam);
}

DVR_RESULT DvrSystemControl::PlayerFrameUpdate(DVR_IO_FRAME *pInputFrame)
{
	DvrMutexLocker mutexLocker(m_playerMutex);

	if (m_player == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_player->Frame_Update(pInputFrame);
}

DVR_RESULT DvrSystemControl::PlayerRegisterCallBack(PFN_PLAYER_NEWFRAME callback, void *pContext)
{
	DvrMutexLocker mutexLocker(m_playerMutex);

	m_ptfnNewFrameCallBack = callback;
	m_pNewFrameCallBackContext = pContext;
	return DVR_RES_SOK;
}

DVR_RESULT DvrSystemControl::PlayerRegisterVehDataUpdateCallBack(PFN_NEW_VEHICLE_DATA callback, void *pContext)
{
	DvrMutexLocker mutexLocker(m_playerMutex);

	m_ptfnPlayerUpdateVehicleData = callback;
	m_pPlayerUpdateVehicleDataContext = pContext;
	
	return DVR_RES_SOK;
}

DVR_RESULT DvrSystemControl::PhotoOpen(const char *fileName)
{
    DvrMutexLocker mutexLocker(m_playerMutex);
    if (fileName == NULL || m_player == NULL)
        return DVR_RES_EPOINTER;

    if (m_pCurrentMountPoint == NULL)
    {
        DPrint(DPRINT_ERR, "DvrSystemControl::PhotoOpen(): The current drive is not selected\n");
        return DVR_RES_EFAIL;
    }

    if (m_pFileDBMgr == NULL)
    {
        DPrint(DPRINT_ERR, "%s: IFileDBManager is not created! Cannot do operation.\n", __func__);
        return DVR_RES_EABORT;
    }

 	m_player->RegisterCallBack(DvrPlayerOnNewFrame, this);
	return m_player->Loop_Setup(fileName, DVR_FILE_TYPE_IMAGE, DVR_FOLDER_TYPE_PHOTO);
}

DVR_RESULT DvrSystemControl::PhotoClose()
{
    DvrMutexLocker mutexLocker(m_playerMutex);

    if (m_player == NULL) {
        return DVR_RES_EPOINTER;
    }
    return m_player->PhotoClose();
}

DVR_RESULT DvrSystemControl::PhotoNext()
{
    DvrMutexLocker mutexLocker(m_playerMutex);

    if (m_player == NULL) {
        return DVR_RES_EPOINTER;
    }
    return m_player->Loop_Next();
}

DVR_RESULT DvrSystemControl::PhotoPrev()
{
    DvrMutexLocker mutexLocker(m_playerMutex);

    if (m_player == NULL) {
        return DVR_RES_EPOINTER;
    }
    return m_player->Loop_Prev();
}

DVR_RESULT DvrSystemControl::PhotoDelta(int nDelta)
{
    DvrMutexLocker mutexLocker(m_playerMutex);

    if (m_player == NULL) {
        return DVR_RES_EPOINTER;
    }
    return m_player->Loop_Delta(nDelta);
}

int DvrSystemControl::Notify(void *pContext, DVR_NOTIFICATION_TYPE enuType, void *pParam1, void *pParam2)
{
	DvrSystemControl *pThis = (DvrSystemControl *)pContext;

	if (pThis == 0)
		return -1;

	return pThis->NotifyImp(enuType, pParam1, pParam2);
}

#include <sys/stat.h>
int DvrSystemControl::NotifyImp(DVR_NOTIFICATION_TYPE enuType, void *pParam1, void *pParam2)
{
	DVR_RESULT res = DVR_RES_SOK;

	switch (enuType)
	{
	case DVR_NOTIFICATION_TYPE_RANGEEVENT:
	{
		DVR_RANGE_EVENT enuEvent = *(DVR_RANGE_EVENT *)pParam1;
		if (pParam1 == 0)
			return -1;

		if (enuEvent == DVR_RANGE_EVENT_EOF) {
			int nSpeed = *(int *)pParam2;
			DPrint(DPRINT_LOG, "DvrSystemControl::NotifyImp() DVR_RANGE_EVENT_EOF received nSpeed = %d\n", nSpeed);
		}
	}
	break;
	case DVR_NOTIFICATION_TYPE_DEVICEEVENT:
	{
		DVR_DEVICE_EVENT devEvent = *(DVR_DEVICE_EVENT *)pParam1;
		DVR_DEVICE           *dev = (DVR_DEVICE *)pParam2;
		if (pParam1 == 0 || pParam2 == NULL)
			return -1;

		OnNotifyDeviceEvent(devEvent, dev);
	}
	break;
	case DVR_NOTIFICATION_TYPE_FILEEVENT:
	{
		DVR_RECORDER_FILE_OP *pfileOP = (DVR_RECORDER_FILE_OP *)pParam1;
		if (pParam1 == 0)
			return -1;

		if(m_pCurrentMountPoint == NULL)
			return -1;

		if (pfileOP->eEvent == DVR_FILE_EVENT_CREATE)
		{
            DVR_U64 MediaFileSize = 0;
            DVR::OSA_GetFileStatInfo(pfileOP->szLocation, &MediaFileSize, NULL);
		    if(pfileOP->eType != DVR_FOLDER_TYPE_PHOTO && MediaFileSize < DVR_THUMBNAIL_WIDTH*DVR_THUMBNAIL_HEIGHT*3>>10)
            {
                DPrint(DPRINT_ERR, "DvrSystemControl::NotifyImp(%s) DVR_NOTIFICATION_TYPE_FILEEVENT (%lld)kb error!!!\n", pfileOP->szLocation,MediaFileSize);
                ComSvcSyncOp_FileDel(pfileOP->szLocation);
				return res;
            }
            DVR_FILEMAP_META_ITEM stItem;
            memset(&stItem, 0, sizeof(DVR_FILEMAP_META_ITEM));
            strcpy(stItem.szMediaFileName, pfileOP->szLocation);
            strcpy(stItem.szThumbNailName, pfileOP->szTNLocation);
            FileMapDB_AddItem(&stItem, pfileOP->eType);
			DPrint(DPRINT_INFO, "DvrSystemControl::NotifyImp(%s) DVR_NOTIFICATION_TYPE_FILEEVENT done!!!\n", stItem.szMediaFileName);
		}
	}
	break;

	case DVR_NOTIFICATION_TYPE_RECORDER_STOP_DONE:
	{
        DPrint(DPRINT_ERR, "Recorder stop done\n");
	}
	break;

	default:
		break;
	}

	return 0;
}

void DvrSystemControl::OnNotifyDeviceEvent(DVR_DEVICE_EVENT devEvent, DVR_DEVICE *dev)
{
	if (devEvent == DVR_DEVICE_EVENT_PLUGIN)
	{
		m_devices.push_back(*dev);
	}

	if (devEvent == DVR_DEVICE_EVENT_PLUGOUT)
	{
		std::list<DVR_DEVICE>::iterator it;
		for (it = m_devices.begin(); it != m_devices.end(); it++)
		{
			if (it->eType == dev->eType &&
				strcmp(it->szMountPoint, dev->szMountPoint) == 0 &&
				strcmp(it->szDevicePath, dev->szDevicePath) == 0)
			{
				m_devices.erase(it);
				break;
			}
		}
	}
}

DVR_RESULT DvrSystemControl::DB_Mount(const char *pszDrive, const char *szDevice, int nDevId)
{
	DvrMutexLocker mutexLocker(m_mediaMutex);
	if (pszDrive == NULL)
		return DVR_RES_EPOINTER;

    if (m_pFileDBMgr == NULL)
    {
        DPrint(DPRINT_ERR, "%s: IFileDBManager is not created! Cannot do mount operation.\n", __func__);
        return DVR_RES_EABORT;
    }

    return m_pFileDBMgr->Mount(pszDrive, szDevice);
}

DVR_RESULT DvrSystemControl::DB_Unmount(const char *pszDrive)
{
	DvrMutexLocker mutexLocker(m_mediaMutex);

	if (m_pFileDBMgr == 0) {
		DPrint(DPRINT_ERR, "%s: FileManager is not created!\n", __func__);
		return DVR_RES_EABORT;
	}

	DPrint(DPRINT_INFO, "%s: Unmount drive %s\n", __func__, pszDrive);
    return m_pFileDBMgr->UnMount(pszDrive);
}

DVR_RESULT DvrSystemControl::FileMapDB_ScanLibrary(const char *pszDrive)
{
	DvrMutexLocker mutexLocker(m_fileMapDBMutex);
    if (pszDrive == NULL)
        return DVR_RES_EPOINTER;

    if (m_pFileDBMgr == 0)
    {
        DPrint(DPRINT_ERR, "%s: IFileDBManager is not created!\n", __func__);
        return DVR_RES_EABORT;
    }

    DPrint(DPRINT_LOG, "%s: szDrive = %s\n", __func__, pszDrive);
   return m_pFileDBMgr->ScanLibrary(pszDrive);
}

DVR_RESULT DvrSystemControl::FileMapDB_SetFailedItem(const char *szFileName, unsigned eFolderType)
{
	DvrMutexLocker mutexLocker(m_fileMapDBMutex);
    if (szFileName == NULL || m_pFileDBMgr == NULL)
        return DVR_RES_EFAIL;

    return m_pFileDBMgr->SetFaildedItem(szFileName, eFolderType);
}

DVR_RESULT DvrSystemControl::FileMapDB_GetItemCountByType(DVR_U32 *puCount, unsigned eFolderType)
{
	DvrMutexLocker mutexLocker(m_fileMapDBMutex);
    if (puCount == NULL || m_pFileDBMgr == NULL)
        return DVR_RES_EPOINTER;

    return m_pFileDBMgr->GetCountByType(*puCount, eFolderType);
}

DVR_RESULT DvrSystemControl::FileMapDB_GetNameByRelPos(unsigned uRelPos, unsigned eFolderType, DVR_FILEMAP_META_ITEM *pszItem, int nSize)
{	
	DvrMutexLocker mutexLocker(m_fileMapDBMutex);
    if (pszItem == NULL || m_pFileDBMgr == NULL)
        return DVR_RES_EPOINTER;

    memset(pszItem, 0, sizeof(DVR_FILEMAP_META_ITEM));
    return m_pFileDBMgr->GetNameByPos(uRelPos, eFolderType, pszItem);
}

DVR_RESULT DvrSystemControl::FileMapDB_GePosByName(unsigned *puRelPos, const char *szFileName, unsigned eFolderType)
{	
	DvrMutexLocker mutexLocker(m_fileMapDBMutex);
    if (puRelPos == NULL || m_pFileDBMgr == NULL)
        return DVR_RES_EPOINTER;

    *puRelPos = 0;
    return m_pFileDBMgr->GetPosByName(puRelPos, szFileName, eFolderType);
}

DVR_RESULT DvrSystemControl::FileMapDB_GetTypeByName(const char *pszDrive, char *szFileName, unsigned& eFolderType)
{	
	DvrMutexLocker mutexLocker(m_fileMapDBMutex);
    if (szFileName == NULL || m_pFileDBMgr == NULL)
        return DVR_RES_EPOINTER;

    return m_pFileDBMgr->GetTypeByName(pszDrive, szFileName, eFolderType);
}

DVR_RESULT DvrSystemControl::FileMapDB_GetNextFile(const char *szFileName, DVR_FILEMAP_META_ITEM *pszItem, unsigned eFolderType)
{
	DvrMutexLocker mutexLocker(m_mediaMutex);

	if (szFileName == NULL || pszItem == NULL)
		return DVR_RES_EPOINTER;

	if (m_pFileDBMgr == 0) {
		DPrint(DPRINT_ERR, "%s: IFileDBManager is not created!\n", __func__);
		return DVR_RES_EABORT;
	}

	DPrint(DPRINT_LOG, "%s: szFileName = %s\n", __func__, szFileName);
    return m_pFileDBMgr->GetNextName(szFileName, pszItem, eFolderType);
}

DVR_RESULT DvrSystemControl::FileMapDB_GetPrevFile(const char *szFileName, DVR_FILEMAP_META_ITEM *pszItem, unsigned eFolderType)
{
	DvrMutexLocker mutexLocker(m_mediaMutex);

	if (szFileName == NULL || pszItem == NULL)
		return DVR_RES_EPOINTER;

	if (m_pFileDBMgr == NULL) {
		DPrint(DPRINT_ERR, "%s: IFileDBManager is not created!\n", __func__);
		return DVR_RES_EABORT;
	}

	DPrint(DPRINT_LOG, "%s: szFileName = %s\n", __func__, szFileName);
    return m_pFileDBMgr->GetPrevName(szFileName, pszItem, eFolderType);
}

DVR_RESULT DvrSystemControl::FileMapDB_GetFileInfo(const char *pszDrive, char *pszFullName, DVR_FOLDER_TYPE eFolderType, PDVR_DB_IDXFILE idxfile)
{
	DvrMutexLocker mutexLocker(m_mediaMutex);

	if (pszDrive == NULL || idxfile == NULL)
		return DVR_RES_EPOINTER;

	if (m_pFileDBMgr == NULL) {
		DPrint(DPRINT_ERR, "%s: IFileDBManager is not created!\n", __func__);
		return DVR_RES_EABORT;
	}

	DPrint(DPRINT_LOG, "%s: pszDrive = %s, pszFullName = %s\n", __func__, pszDrive, pszFullName);
    return m_pFileDBMgr->GetFileInfo(pszFullName, eFolderType, idxfile);
}

DVR_RESULT DvrSystemControl::FileMapDB_AddItem(DVR_FILEMAP_META_ITEM *pszItem, unsigned eFolderType)
{
	DvrMutexLocker mutexLocker(m_fileMapDBMutex);
    if (pszItem == NULL || m_pFileDBMgr == NULL)
        return DVR_RES_EFAIL;

    return m_pFileDBMgr->AddItem(pszItem, eFolderType);
}

DVR_RESULT DvrSystemControl::FileMapDB_DelItem(const char *szFileName, unsigned eFolderType)
{
    DvrMutexLocker mutexLocker(m_fileMapDBMutex);
    if(szFileName == NULL || m_pFileDBMgr == NULL)
        return DVR_RES_EFAIL;

    return m_pFileDBMgr->DelItem(szFileName, eFolderType);
}

DVR_RESULT DvrSystemControl::FileMapDB_Clear()
{


    if(m_pFileDBMgr == NULL)
        return DVR_RES_EFAIL;

    return m_pFileDBMgr->Clear();
}


DVR_RESULT DvrSystemControl::FileMapDB_GetDasOldNameByType(char *szDispName, unsigned eRecType)
{
    DvrMutexLocker mutexLocker(m_fileMapDBMutex);
    DVR_FILEMAP_META_ITEM 	FileItem;
    DVR_RESULT res = DVR_RES_EPOINTER;

    if (m_pFileDBMgr == NULL) {
		DPrint(DPRINT_ERR, "%s: IFileDBManager is not created!\n", __func__);
		return DVR_RES_EABORT;
	}
    
    if (szDispName == NULL )
        return DVR_RES_EPOINTER;
    if(eRecType >= DVR_DAS_TYPE_NUM)
        return  DVR_RES_EPOINTER;

    DVR_U32 totalnum ;
    res=m_pFileDBMgr->GetCountByType(totalnum,DVR_FOLDER_TYPE_DAS);
    if(res!=DVR_RES_SOK)
        return res;

    if(totalnum <= 2)
        return DVR_RES_EPOINTER;

    const char* name[] = {"APA","ACC","IAC"};
    for(int i = 0; i < totalnum;i++)
    {        
        memset(&FileItem, 0, sizeof(DVR_FILEMAP_META_ITEM));

        res=m_pFileDBMgr->GetNameByPos(i, DVR_FOLDER_TYPE_DAS, &FileItem);
        if(res!=DVR_RES_SOK){
            DPrint(DPRINT_ERR, "%s GetNameByPos error !\n", __func__);
    		return res;
    	}
        
        char *filename = strrchr(FileItem.szMediaFileName, '/');
        if(filename == NULL)
            continue;

        if(strncmp(filename + 1, name[eRecType - DVR_DAS_TYPE_APA], 3) == 0)
        {
            strncpy(szDispName, FileItem.szMediaFileName, APP_MAX_FN_SIZE);
            break;
        }
    }

    return DVR_RES_SOK;
}


DVR_RESULT DvrSystemControl::FileMapDB_GetDasCount(DVR_U32 *pApaCount, DVR_U32 *pAccCount, DVR_U32 *pIaccCount)
{
    DvrMutexLocker mutexLocker(m_fileMapDBMutex);
    DVR_RESULT res = DVR_RES_EPOINTER;
    DVR_FILEMAP_META_ITEM 	prevItem;
    DVR_U32 totalnum;

    if (m_pFileDBMgr == NULL) {
		DPrint(DPRINT_ERR, "%s: IFileDBManager is not created!\n", __func__);
		return DVR_RES_EABORT;
	}
    
    if (pApaCount == NULL || pIaccCount == NULL || pIaccCount==NULL)
        return DVR_RES_EPOINTER;

    *pApaCount = 0;
    *pAccCount = 0;
    *pIaccCount = 0;
    res= m_pFileDBMgr->GetCountByType(totalnum,DVR_FOLDER_TYPE_DAS);
    if(res!=DVR_RES_SOK)
    {
        return res;
    }

    
    if(totalnum <= 2)
    {
        *pApaCount = 0;
        *pAccCount = 0;
        *pIaccCount = 0;
        return DVR_RES_SOK;
    }
    
    for(int i = 0; i < totalnum;i++)
    {
        memset(&prevItem, 0, sizeof(DVR_FILEMAP_META_ITEM)); 
        res=m_pFileDBMgr->GetNameByPos(i,  DVR_FOLDER_TYPE_DAS, &prevItem);

        
        if(res== DVR_RES_SOK)
        {
            char *filename = strrchr(prevItem.szMediaFileName, '/');
            if(filename == NULL)
                continue;

            if(strncmp(filename + 1, "APA", 3) == 0)
                (*pApaCount)++;
            else if(strncmp(filename + 1, "ACC", 3) == 0)
                (*pAccCount)++;
            else if(strncmp(filename + 1, "IAC", 3) == 0)
                (*pIaccCount)++;
        }
    }
    DPrint(DPRINT_INFO, "+++++++++++++++totalnum %d ApaCount %d AccCount %d IaccCount %d+++++++++++++++\n",totalnum, *pApaCount, *pAccCount, *pIaccCount);
    return DVR_RES_SOK;
}

void DvrSystemControl::DvrPlayerOnNewFrame(void *pInstance, void *pUserData)
{
	DVR_IO_FRAME *pFrame = (DVR_IO_FRAME *)pUserData;
	DvrSystemControl *pThis = (DvrSystemControl *)pInstance;

	if(pThis->m_ptfnPlayerUpdateVehicleData != NULL)
	{
		pThis->m_ptfnPlayerUpdateVehicleData(pThis->m_pPlayerUpdateVehicleDataContext, pFrame->pCanBuf);
	}

	if(pThis->m_ptfnNewFrameCallBack != NULL)
	{
		pThis->m_ptfnNewFrameCallBack(pThis->m_pNewFrameCallBackContext, pFrame);
	}
}

DVR_RESULT DvrSystemControl::MetaDataCreate(void **ppMetaDataHandle, void* pvFileName, DVR_U32 *pu32ItemNum)
{
	if (ppMetaDataHandle == NULL)
		return DVR_RES_EPOINTER;

	*ppMetaDataHandle = Dvr_MetaData_Create(pvFileName, pu32ItemNum, true);

	return DVR_RES_SOK;
}

DVR_RESULT DvrSystemControl::MetaDataDestroy(void *pvMetaDataHandle)
{
	if (pvMetaDataHandle == NULL)
		return DVR_RES_EPOINTER;

	return Dvr_MetaData_Destroy(pvMetaDataHandle);
}

DVR_RESULT DvrSystemControl::MetaDataGetItemNum(void *pvMetaDataHandle, DVR_U32 *pu32ItemNum)
{
	if (pvMetaDataHandle == NULL || pu32ItemNum == NULL)
		return DVR_RES_EPOINTER;

	*pu32ItemNum = Dvr_MetaData_GetItemNum(pvMetaDataHandle);

	return DVR_RES_SOK;
}

DVR_RESULT DvrSystemControl::MetaDataGetDataByIndex(void *pvMetaDataHandle, DVR_METADATA_ITEM* pItem, DVR_U32 u32Index)
{
	if (pvMetaDataHandle == NULL || pItem == NULL)
		return DVR_RES_EPOINTER;

	const DVR_METADATA_ITEM *ptr = Dvr_MetaData_GetDataByIndex(pvMetaDataHandle, u32Index);
	if (ptr == NULL)
		return DVR_RES_EFAIL;

	memcpy(pItem, ptr, sizeof(DVR_METADATA_ITEM));

	return DVR_RES_SOK;
}

DVR_RESULT DvrSystemControl::MetaDataGetDataByType(void *pvMetaDataHandle, DVR_METADATA_ITEM* pItem, DVR_METADATA_TYPE eType)
{
	if (pvMetaDataHandle == NULL || pItem == NULL)
		return DVR_RES_EPOINTER;

	const DVR_METADATA_ITEM *ptr = Dvr_MetaData_GetDataByType(pvMetaDataHandle, eType);
	if (ptr == NULL)
		return DVR_RES_EFAIL;

	memcpy(pItem, ptr, sizeof(DVR_METADATA_ITEM));

	return DVR_RES_SOK;
}

DVR_RESULT DvrSystemControl::MetaDataGetMediaInfo(void *pvMetaDataHandle, DVR_MEDIA_INFO *pInfo)
{
	if (pvMetaDataHandle == NULL || pInfo == NULL)
		return DVR_RES_EPOINTER;

	return Dvr_MetaData_GetMediaInfo(pvMetaDataHandle, pInfo);
}

DVR_RESULT DvrSystemControl::MetaDataGetTrackByIndex(void *pvMetaDataHandle, DVR_MEDIA_TRACK* pTrack, DVR_U32 u32Index)
{
	if (pvMetaDataHandle == NULL || pTrack == NULL)
		return DVR_RES_EPOINTER;

	const DVR_MEDIA_TRACK *ptr = Dvr_MetaData_GetTrackByIndex(pvMetaDataHandle, u32Index);
	memcpy(pTrack, ptr, sizeof(DVR_MEDIA_TRACK));

	return DVR_RES_SOK;
}

DVR_RESULT DvrSystemControl::MetaDataGetPreview(DVR_PREVIEW_OPTION *pPreviewOpt, unsigned char *pBuf, unsigned int *pCbSize)
{
	return Dvr_Get_MediaFilePreview(pPreviewOpt, pBuf, pCbSize);
}

DVR_RESULT DvrSystemControl::FormatDisplayName(char *srcName, char *dstName, DVR_U32 u32DurationMs)
{
	return OSA_FormatDisplayName(srcName, dstName, u32DurationMs);
}

DVR_RESULT DvrSystemControl::LoadThumbNail(char *filename, unsigned char *pPreviewBuf, int nSize)
{
    return DvrRecorderLoop::LoadThumbNail(filename, pPreviewBuf, nSize);
}

OsaNotifier *DvrSystemControl::Notifier()
{
	return m_pNotifier;
}

DVR_RESULT DvrSystemControl::ComSvcHcMgr_AttachHandler(DVR_HCMGR_HANDLER *handler, void *pContext)
{
	if (m_pComSvcHcMgr == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_pComSvcHcMgr->DvrComSvcHcmgr_AttachHandler(handler, pContext);
}

DVR_RESULT DvrSystemControl::ComSvcHcmgr_DetachHandler(void)
{
	if (m_pComSvcHcMgr == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_pComSvcHcMgr->DvrComSvcHcmgr_DetachHandler();
}

DVR_RESULT DvrSystemControl::ComSvcHcmgr_ResetHandler(void)
{
	if (m_pComSvcHcMgr == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_pComSvcHcMgr->DvrComSvcHcmgr_ResetHandler();
}

DVR_RESULT DvrSystemControl::ComSvcHcmgr_SndMsg(APP_MESSAGE *msg, DVR_U32 waitOption)
{
	if (m_pComSvcHcMgr == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_pComSvcHcMgr->DvrComSvcHcmgr_SndMsg(msg, waitOption);
}

DVR_RESULT DvrSystemControl::ComSvcHcmgr_SendMsg(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2)
{
	if (m_pComSvcHcMgr == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_pComSvcHcMgr->DvrComSvcHcmgr_SendMsg(msg, param1, param2);
}

DVR_RESULT DvrSystemControl::ComSvcHcmgr_RcvMsg(APP_MESSAGE *msg, DVR_U32 waitOption)
{
	if (m_pComSvcHcMgr == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_pComSvcHcMgr->DvrComSvcHcmgr_RcvMsg(msg, waitOption);
}

DVR_RESULT DvrSystemControl::ComSvcTimer_Register(int tid, PFN_APPTIMER_HANDLER handler)
{
	if (m_pComSvcTimer == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_pComSvcTimer->DvrComSvcTimer_Register(tid, handler);
}

DVR_RESULT DvrSystemControl::ComSvcTimer_UnRegister(int tid, PFN_APPTIMER_HANDLER handler)
{
	if (m_pComSvcTimer == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_pComSvcTimer->DvrComSvcTimer_Unregister(tid, handler);
}

DVR_RESULT DvrSystemControl::ComSvcTimer_Handler(int tid)
{
	if (m_pComSvcTimer == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_pComSvcTimer->DvrComSvcTimer_Handler(tid);
}

DVR_RESULT DvrSystemControl::ComSvcTimer_Start(int tid)
{
	if (m_pComSvcTimer == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_pComSvcTimer->DvrComSvcTimer_Start(tid);
}

DVR_RESULT DvrSystemControl::ComSvcTimer_UnRegisterAll(void)
{
	if (m_pComSvcTimer == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_pComSvcTimer->DvrComSvcTimer_UnregisterAll();
}

DVR_RESULT DvrSystemControl::ComSvcAsyncOp_CardFormat(void)
{
	if (m_pComSvcAsyncOp == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_pComSvcAsyncOp->DvrComSvcAsyncOp_CardFormat();
}

DVR_RESULT DvrSystemControl::ComSvcAsyncOp_FileCopy(char *srcFn, char *dstFn)
{
	if (m_pComSvcAsyncOp == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_pComSvcAsyncOp->DvrComSvcAsyncOp_FileCopy(srcFn, dstFn);
}

DVR_RESULT DvrSystemControl::ComSvcAsyncOp_FileMove(char *srcFn, char *dstFn)
{
	if (m_pComSvcAsyncOp == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_pComSvcAsyncOp->DvrComSvcAsyncOp_FileMove(srcFn, dstFn);
}

DVR_RESULT DvrSystemControl::ComSvcAsyncOp_FileDel(char *filename)
{
	if (m_pComSvcAsyncOp == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_pComSvcAsyncOp->DvrComSvcAsyncOp_FileDel(filename);
}

DVR_RESULT DvrSystemControl::ComSvcSyncOp_FileCopy(char *srcFn, char *dstFn)
{
    if (m_pComSvcSyncOp == NULL) {
        return DVR_RES_EPOINTER;
    }
    return m_pComSvcSyncOp->DvrComSvcSyncOp_FileCopy(srcFn, dstFn);
}

DVR_RESULT DvrSystemControl::ComSvcSyncOp_FileMove(char *srcFn, char *dstFn)
{
    if (m_pComSvcSyncOp == NULL) {
        return DVR_RES_EPOINTER;
    }
    return m_pComSvcSyncOp->DvrComSvcSyncOp_FileMove(srcFn, dstFn);
}

DVR_RESULT DvrSystemControl::ComSvcSyncOp_FileDel(char *filename)
{
    if (m_pComSvcSyncOp == NULL) {
        return DVR_RES_EPOINTER;
    }
    return m_pComSvcSyncOp->DvrComSvcSyncOp_FileDel(filename);
}

DVR_RESULT DvrSystemControl::StorageAsyncOp_SndMsg(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2)
{
	if (m_pStorageAsyncOp == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_pStorageAsyncOp->DvrStorageAsyncOp_SndMsg(msg, param1, param2);
}

DVR_RESULT DvrSystemControl::StorageAsyncOp_RegHandler(DVR_STORAGE_HANDLER *handler)
{
	if (m_pStorageAsyncOp == NULL) {
		return DVR_RES_EPOINTER;
	}
	return m_pStorageAsyncOp->DvrStorageAsyncOp_RegHandler(handler);
}

DVR_RESULT DvrSystemControl::StorageAsyncOp_ClrHandler(void)
{
    if (m_pStorageAsyncOp == NULL) {
        return DVR_RES_EPOINTER;
    }
    return m_pStorageAsyncOp->DvrStorageAsyncOp_ClrHandler();
}

DVR_RESULT DvrSystemControl::StorageCard_Set(DVR_STORAGE_PROP prop, void *propData, int propSize)
{
	if (m_pStorageCard == NULL) {
		return DVR_RES_EPOINTER;
	}

	switch (prop)
	{
	case DVR_STORAGE_PROP_THRESHOLD:
	{
		if (propData == NULL || propSize != sizeof(int))
		{
			DPrint(DPRINT_ERR, "StorageCard_Set Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		int threshold = *(int *)propData;
		m_pStorageCard->DvrStorageCard_SetThreshold(threshold);
	}
	break;

	case DVR_STORAGE_PROP_CARD_QUOTA:
	{
		if (propData == NULL || propSize != sizeof(DVR_STORAGE_QUOTA))
		{
			DPrint(DPRINT_ERR, "StorageCard_Set Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		DVR_STORAGE_QUOTA *pQuota = (DVR_STORAGE_QUOTA *)propData;
		m_pStorageCard->DvrStorageCard_SetQuota(pQuota);
	}
	break;

	case DVR_STORAGE_PROP_CARD_SPEED_STATUS:
	{
		if (propData == NULL || propSize != sizeof(DVR_BOOL))
		{
			DPrint(DPRINT_ERR, "StorageCard_Set Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		DVR_BOOL bIsLowSpeed = *(DVR_BOOL *)propData;
		m_pStorageCard->DvrStorageCard_SetSpeedStatus(bIsLowSpeed);
	}
	break;

	case DVR_STORAGE_PROP_FREE_SPACE_STATUS:
	{
		if (propData == NULL || propSize != sizeof(DVR_BOOL))
		{
			DPrint(DPRINT_ERR, "StorageCard_Set Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		DVR_BOOL bFreeSpaceTooSmall = *(DVR_BOOL *)propData;
		m_pStorageCard->DvrStorageCard_SetFreeSpaceStatus(bFreeSpaceTooSmall);
	}
	break;
    
	case DVR_STORAGE_PROP_CARD_READY_STATUS:
	{
		if (propData == NULL || propSize != sizeof(DVR_BOOL))
		{
			DPrint(DPRINT_ERR, "StorageCard_Set Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		DVR_BOOL bIsReady = *(DVR_BOOL *)propData;
		m_pStorageCard->DvrStorageCard_SetReadyStatus(bIsReady);
	}
	break;

	default:
		break;
	}

	return DVR_RES_SOK;
}

DVR_RESULT DvrSystemControl::StorageCard_Get(DVR_STORAGE_PROP prop, void *propData, int propSize, int *pnSizeReturned)
{
	DVR_RESULT res = DVR_RES_SOK;

	if (m_pStorageCard == NULL) {
		return DVR_RES_EPOINTER;
	}

	switch (prop)
	{
	case DVR_STORAGE_PROP_THRESHOLD:
	{
		if (propData == NULL || propSize != sizeof(DVR_U32))
		{
			DPrint(DPRINT_ERR, "StorageCard_Set Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		DVR_U32 threshold = m_pStorageCard->DvrStorageCard_GetThreshold();

		*(DVR_U32 *)propData = threshold;
	}
	break;

    case DVR_STORAGE_PROP_CARD_FREESPACE:
	{
		if (propData == NULL || propSize != sizeof(DVR_U32))
		{
			DPrint(DPRINT_ERR, "StorageCard_Set Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		if (m_pCurrentMountPoint == NULL)
		{
			return DVR_RES_EFAIL;
		}

		DVR_U32 freespace = m_pStorageCard->DvrStorageCard_GetFreeSpace(m_pCurrentMountPoint);

		*(DVR_U32 *)propData = freespace;
	}
	break;

    case DVR_STORAGE_PROP_CARD_TOTALSPACE:
	{
		if (propData == NULL || propSize != sizeof(DVR_U32))
		{
			DPrint(DPRINT_ERR, "StorageCard_Set Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		if (m_pCurrentMountPoint == NULL)
		{
			return DVR_RES_EFAIL;
		}

		DVR_U32 totalspace = m_pStorageCard->DvrStorageCard_GetTotalSpace(m_pCurrentMountPoint);

		*(DVR_U32 *)propData = totalspace;
	}
	break;

	case DVR_STORAGE_PROP_CARD_USEDSPACE:
	{
		if (propData == NULL || propSize != sizeof(DVR_U32))
		{
			DPrint(DPRINT_ERR, "StorageCard_Set Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		if (m_pCurrentMountPoint == NULL)
		{
			return DVR_RES_EFAIL;
		}

		DVR_U32 usedspace = m_pStorageCard->DvrStorageCard_GetUsedSpace(m_pCurrentMountPoint);

		*(DVR_U32 *)propData = usedspace;
	}
	break;

	case DVR_STORAGE_PROP_CARD_QUOTA:
	{
		if (propData == NULL || propSize != sizeof(DVR_STORAGE_QUOTA))
		{
			DPrint(DPRINT_ERR, "StorageCard_Set Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		if (m_pCurrentMountPoint == NULL)
		{
			return DVR_RES_EFAIL;
		}
		
		DVR_STORAGE_QUOTA *pQuota = (DVR_STORAGE_QUOTA *)propData;
		m_pStorageCard->DvrStorageCard_GetQuota(pQuota);
	}
	break;

    case DVR_STORAGE_PROP_NORMAL_FOLDER_USEDSPACE:
    {
        if (propData == NULL || propSize != sizeof(DVR_U32))
        {
            DPrint(DPRINT_ERR, "StorageCard_Set Invalid Parameters!\n", __LINE__);
            return DVR_RES_EINVALIDARG;
        }

        if (m_pCurrentMountPoint == NULL)
        {
            return DVR_RES_EFAIL;
        }

        if(m_pFileDBMgr != NULL)
            *(DVR_U32 *)propData = m_pFileDBMgr->GetUsedSpace(DVR_FOLDER_TYPE_NORMAL);
    }
    break;

    case DVR_STORAGE_PROP_EVENT_FOLDER_USEDSPACE:
    {
        if (propData == NULL || propSize != sizeof(DVR_U32))
        {
            DPrint(DPRINT_ERR, "StorageCard_Set Invalid Parameters!\n", __LINE__);
            return DVR_RES_EINVALIDARG;
        }

        if (m_pCurrentMountPoint == NULL)
        {
            return DVR_RES_EFAIL;
        }

        if(m_pFileDBMgr != NULL)
            *(DVR_U32 *)propData = m_pFileDBMgr->GetUsedSpace(DVR_FOLDER_TYPE_EMERGENCY);
    }
    break;

    case DVR_STORAGE_PROP_PHOTO_FOLDER_USEDSPACE:
    {
        if (propData == NULL || propSize != sizeof(DVR_U32))
        {
            DPrint(DPRINT_ERR, "StorageCard_Set Invalid Parameters!\n", __LINE__);
            return DVR_RES_EINVALIDARG;
        }

        if (m_pCurrentMountPoint == NULL)
        {
            return DVR_RES_EFAIL;
        }

        if(m_pFileDBMgr != NULL)
            *(DVR_U32 *)propData = m_pFileDBMgr->GetUsedSpace(DVR_FOLDER_TYPE_PHOTO);
    }
    break;

    case DVR_STORAGE_PROP_DAS_FOLDER_USEDSPACE:
    {
        if (propData == NULL || propSize != sizeof(DVR_U32))
        {
            DPrint(DPRINT_ERR, "StorageCard_Set Invalid Parameters!\n", __LINE__);
            return DVR_RES_EINVALIDARG;
        }

        if (m_pCurrentMountPoint == NULL)
        {
            return DVR_RES_EFAIL;
        }

        if(m_pFileDBMgr != NULL)
            *(DVR_U32 *)propData = m_pFileDBMgr->GetUsedSpace(DVR_FOLDER_TYPE_DAS);
    }
    break;

	case DVR_STORAGE_PROP_DIRECTORY_STRUCTURE:
	{
		if (propData == NULL || propSize != sizeof(DVR_DISK_DIRECTORY))
		{
			DPrint(DPRINT_ERR, "StorageCard_Set Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		if (m_pCurrentMountPoint == NULL)
		{
			return DVR_RES_EFAIL;
		}

		DVR_DISK_DIRECTORY *pDir = (DVR_DISK_DIRECTORY*)propData;

		m_pStorageCard->DvrStorageCard_GetDirectory(pDir);
	}
	break;

	case DVR_STORAGE_PROP_ROOT_FOLDER:
	{
		if (propData == NULL)
		{
			DPrint(DPRINT_ERR, "StorageCard_Set Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		memset(propData, 0, propSize);

		m_pStorageCard->DvrStorageCard_GetFolder(DVR_FOLDER_TYPE_ROOT, (char *)propData);
	}
	break;

	case DVR_STORAGE_PROP_NORMAL_FOLDER:
	{
		if (propData == NULL)
		{
			DPrint(DPRINT_ERR, "StorageCard_Set Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		m_pStorageCard->DvrStorageCard_GetFolder(DVR_FOLDER_TYPE_NORMAL, (char *)propData);
	}
	break;

	case DVR_STORAGE_PROP_EMERGENCY_FOLDER:
	{
		if (propData == NULL)
		{
			DPrint(DPRINT_ERR, "StorageCard_Set Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		m_pStorageCard->DvrStorageCard_GetFolder(DVR_FOLDER_TYPE_EMERGENCY, (char *)propData);
	}
	break;

	case DVR_STORAGE_PROP_PHOTO_FOLDER:
	{
		if (propData == NULL)
		{
			DPrint(DPRINT_ERR, "StorageCard_Set Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		m_pStorageCard->DvrStorageCard_GetFolder(DVR_FOLDER_TYPE_PHOTO, (char *)propData);
	}
	break;

	case DVR_STORAGE_PROP_DAS_FOLDER:
	{
		if (propData == NULL)
		{
			DPrint(DPRINT_ERR, "StorageCard_Set Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		m_pStorageCard->DvrStorageCard_GetFolder(DVR_FOLDER_TYPE_DAS, (char *)propData);
	}
	break;

	default:
		break;
	}

	return DVR_RES_SOK;
}

DVR_RESULT DvrSystemControl::StorageCard_CheckStatus(void)
{
	if (m_pStorageCard == NULL) {
		return DVR_RES_EPOINTER;
	}

	return m_pStorageCard->DvrStorageCard_CheckStatus();
}

DVR_RESULT DvrSystemControl::MonitorStorage_Set(DVR_MONITOR_STORAGE_PROP prop, void *propData, int propSize)
{
	if (m_pStorageMonitor == NULL) {
		return DVR_RES_EPOINTER;
	}

	switch (prop)
	{
	case DVR_MONITOR_STORAGE_PROP_LOOPREC_THRESHOLD:
	{
		if (propData == NULL || propSize != sizeof(DVR_U32))
		{
			DPrint(DPRINT_ERR, "%s MonitorStorage_Set Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		DVR_U32 threshold = *(DVR_U32 *)propData;
		m_pStorageMonitor->SetLoopRecThreshold(threshold);
	}
	break;

	case DVR_MONITOR_STORAGE_PROP_ENABLE_NORMAL_DETECT:
	{
		if (propData == NULL || propSize != sizeof(DVR_BOOL))
		{
			DPrint(DPRINT_ERR, "%s MonitorStorage_Set Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		DVR_BOOL enable = *(DVR_BOOL *)propData;
		m_pStorageMonitor->SetEnableLoopRecDetect(enable);
	}
	break;

	case DVR_MONITOR_STORAGE_PROP_ENABLE_NORMAL_MSG:
	{
		if (propData == NULL || propSize != sizeof(DVR_BOOL))
		{
			DPrint(DPRINT_ERR, "%s MonitorStorage_Set Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		DVR_BOOL enable = *(DVR_BOOL *)propData;
		m_pStorageMonitor->SetEnableLoopRecMsg(enable);
	}
    break;

    case DVR_MONITOR_STORAGE_PROP_NORMAL_QUOTA:
    {
        if (propData == NULL || propSize != sizeof(DVR_U32))
        {
            DPrint(DPRINT_ERR, "%s MonitorStorage_Set Invalid Parameters!\n", __LINE__);
            return DVR_RES_EINVALIDARG;
        }

        DVR_U32 quota = *(DVR_U32 *)propData;
        m_pStorageMonitor->SetLoopRecQuota(quota);
    }
    break;

    case DVR_MONITOR_STORAGE_PROP_CARD_AVAILABLE_SPACE:
    {
        if (propData == NULL || propSize != sizeof(DVR_U32))
        {
            DPrint(DPRINT_ERR, "%s MonitorStorage_Set Invalid Parameters!\n", __LINE__);
            return DVR_RES_EINVALIDARG;
        }
    
        DVR_U32 space = *(DVR_U32 *)propData;
        m_pStorageMonitor->SetAvailableSpace(space);
    }
    break;

    case DVR_MONITOR_STORAGE_PROP_ENABLE_DAS_DETECT:
    {
        if (propData == NULL || propSize != sizeof(DVR_BOOL))
        {
            DPrint(DPRINT_ERR, "%s MonitorStorage_Set Invalid Parameters!\n", __LINE__);
            return DVR_RES_EINVALIDARG;
        }

        DVR_BOOL enable = *(DVR_BOOL *)propData;
        m_pStorageMonitor->SetEnableDasRecDetect(enable);
    }
    break;

    case DVR_MONITOR_STORAGE_PROP_ENABLE_DAS_MSG:
    {
        if (propData == NULL || propSize != sizeof(DVR_DAS_MONITOR))
        {
            DPrint(DPRINT_ERR, "%s MonitorStorage_Set Invalid Parameters!\n", __LINE__);
            return DVR_RES_EINVALIDARG;
        }

        DVR_DAS_MONITOR monitor;
        memcpy(&monitor, propData, sizeof(DVR_DAS_MONITOR));
        m_pStorageMonitor->SetEnableDasRecMsg(&monitor);
    }
    break;

	default:
		break;
	}

	return DVR_RES_SOK;
}

DVR_RESULT DvrSystemControl::MonitorStorage_Get(DVR_MONITOR_STORAGE_PROP prop, void *propData, int propSize, int *pnSizeReturned)
{
	if (m_pStorageMonitor == NULL) {
		return DVR_RES_EPOINTER;
	}

	switch (prop)
	{
	case DVR_MONITOR_STORAGE_PROP_LOOPREC_THRESHOLD:
	{
		if (propData == NULL || propSize != sizeof(DVR_U32))
		{
			DPrint(DPRINT_ERR, "MonitorStorage_Get Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		DVR_U32 threshold = m_pStorageMonitor->GetLoopRecThreshold();

		*(DVR_U32 *)propData = threshold;
	}
	break;

	case DVR_MONITOR_STORAGE_PROP_NORMAL_QUOTA:
	{
		if (propData == NULL || propSize != sizeof(DVR_U32))
		{
			DPrint(DPRINT_ERR, "MonitorStorage_Get Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}
		DVR_U32 quota = m_pStorageMonitor->GetLoopRecQuota();
		*(DVR_U32 *)propData = quota;
	}
	break;

    case DVR_MONITOR_STORAGE_PROP_CARD_AVAILABLE_SPACE:
    {
        if (propData == NULL || propSize != sizeof(DVR_U32))
        {
            DPrint(DPRINT_ERR, "MonitorStorage_Get Invalid Parameters!\n", __LINE__);
            return DVR_RES_EINVALIDARG;
        }
    
        DVR_U32 space = m_pStorageMonitor->GetAvailableSpace();
		*(DVR_U32 *)propData = space;
    }
    break;

	case DVR_MONITOR_STORAGE_PROP_ENABLE_NORMAL_DETECT:
	{
		if (propData == NULL || propSize != sizeof(DVR_BOOL))
		{
			DPrint(DPRINT_ERR, "MonitorStorage_Get Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		DVR_BOOL enable = m_pStorageMonitor->GetEnableLoopRecDetect();

		*(DVR_BOOL *)propData = enable;
	}
	break;

	default:
		break;
	}

	return DVR_RES_SOK;
}

DVR_RESULT DvrSystemControl::UpdateSystemTime(DVR_U32 year, DVR_U32 month, DVR_U32 day, DVR_U32 hour, DVR_U32 minute, DVR_U32 second)
{
	return OSA_UpdateSystemTime(year, month, day, hour, minute, second);
}

DVR_RESULT DvrSystemControl::MarkSDSpeed(char *szDrive, char *pszpoint, DVR_SDSPEED_TYPE type)
{
	return DVR::OSA_MarkSDSpeed(szDrive, pszpoint, type);
}

DVR_RESULT DvrSystemControl::GetSDSpeekType(char *szDrive, char *pszpoint, DVR_U32 *type)
{
	return DVR::OSA_GetSDSpeekType(szDrive, pszpoint, type);
}



