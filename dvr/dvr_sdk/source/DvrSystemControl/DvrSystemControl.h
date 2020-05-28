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
#ifndef _DVRSYSTEMCONTROL_H_
#define _DVRSYSTEMCONTROL_H_

#include <vector>
#include <list>
#include <string>
#include <map>
#include <glib.h>

#include <windows.h>

#include "DVR_SDK_DEF.h"
#include "osa.h"
#include "osa_notifier.h"
#include "FileDBManger.h"

class DvrMutex;
class DvrPlayerLoop;
class DvrRecorderLoop;
class DvrComSvcHcMgr;
class DvrComSvcTimer;
class DvrComSvcAsyncOp;
class DvrComSvcSyncOp;
class DvrStorageAsyncOp;
class DvrStorageCard;
class DvrMonitorStorage;

#ifdef __linux__
class DvrMonitorMount;;
#else
class DvrMonitorMount_Win32;
#endif

typedef struct
{
	char szFileName[APP_MAX_FN_SIZE];
	DVR_U32 uIndex;
    DVR_U64 ullFileSize;
	DVR_U64 ullModifyTime;
}FILEAMP_SORT_ITEM;

class DvrSystemControl
{
public:
	DvrSystemControl(void);
    ~DvrSystemControl(void);

	DVR_RESULT RegisterNotify(PFN_DVR_SDK_NOTIFY pCallback, void *pContext);
	DVR_RESULT UnRegisterNotify(PFN_DVR_SDK_NOTIFY pCallback, void *pContext);
	DVR_RESULT RegisterConsole(PFN_DVR_SDK_CONSOLE pCallback, void *pContext);
    DVR_RESULT RegisterGSTConsole(PFN_DVR_SDK_CONSOLE pCallback, void *pContext);
	DVR_RESULT UnregisterConsole(PFN_DVR_SDK_CONSOLE pCallback, void *pContext);
	DVR_RESULT EnumDevices(DVR_DEVICE_TYPE enuType, DVR_DEVICE_ARRAY *pDevices);
	DVR_RESULT SetActiveDrive(DVR_DEVICE *pszDrive);
	DVR_RESULT ClearActiveDrive(void);
	DVR_RESULT GetActiveDrive(DVR_DEVICE *pszDrive);

	DVR_RESULT MediaMgrOpen();
	DVR_RESULT MediaMgrClose();
	DVR_RESULT MediaMgrReset();

	DVR_RESULT RecorderInit();
	DVR_RESULT RecorderDeInit();
	DVR_RESULT RecorderStart();
	DVR_RESULT RecorderStop();
	DVR_RESULT RecorderReset();
    DVR_RESULT RecorderLoopRecStart();
    DVR_RESULT RecorderLoopRecStop();
	DVR_RESULT RecorderEventRecStart();
    DVR_RESULT RecorderEventRecStop();
	DVR_RESULT RecorderDasRecStart();
    DVR_RESULT RecorderDasRecStop();
    DVR_RESULT RecorderIaccRecStart(int type);
    DVR_RESULT RecorderIaccRecStop(int type);
	DVR_RESULT RecorderAddFrame(DVR_IO_FRAME *pInputFrame);
	DVR_RESULT RecorderAcquireInputBuf(void **ppImgBuf);
	DVR_RESULT RecorderAsyncOpFlush(void);
	DVR_RESULT RecorderSet(DVR_RECORDER_PROP prop, void *propData, int propSize);
	DVR_RESULT RecorderGet(DVR_RECORDER_PROP prop, void *propData, int propSize, int *pnSizeReturned);
	DVR_RESULT RecorderRegisterVehDataUpdateCallBack(PFN_NEW_VEHICLE_DATA callback, void *pContext);
	DVR_RESULT RecorderPhotoTake(DVR_PHOTO_PARAM *pParam);
	DVR_RESULT RecorderTakeOverlay(void *canbuffer, void *osdbuffer,int cansize, int osdsize);

	DVR_RESULT PlayerInit();
	DVR_RESULT PlayerDeInit();
	DVR_RESULT PlayerOpen(const char *fileName, int type);
	DVR_RESULT PlayerClose();
	DVR_RESULT PlayerPlay();
	DVR_RESULT PlayerStop();
	DVR_RESULT PlayerPause();
	DVR_RESULT PlayerResume();
	DVR_RESULT PlayerNext();
	DVR_RESULT PlayerPrev();
	DVR_RESULT PlayerStepF();
	DVR_RESULT PlayerStepB();
	DVR_RESULT PlayerDelta(int nDelta);	
	DVR_RESULT PlayerSet(DVR_PLAYER_PROP prop, void *propData, int propSize);
	DVR_RESULT PlayerGet(DVR_PLAYER_PROP prop, void *propData, int propSize, int *propSizeReturned);
	DVR_RESULT PlayerRegisterCallBack(PFN_PLAYER_NEWFRAME callback, void *pContext);
	DVR_RESULT PlayerFrameUpdate(DVR_IO_FRAME *pInputFrame);
	DVR_RESULT PlayerRegisterVehDataUpdateCallBack(PFN_NEW_VEHICLE_DATA callback, void *pContext);
	DVR_RESULT PlayerPrintScreen(DVR_PHOTO_PARAM *pParam);

	DVR_RESULT PhotoOpen(const char *fileName);
	DVR_RESULT PhotoClose();
	DVR_RESULT PhotoNext();
	DVR_RESULT PhotoPrev();
	DVR_RESULT PhotoDelta(int nDelta);

	DVR_RESULT MetaDataCreate(void **ppMetaDataHandle, void* pvFileName, DVR_U32 *pu32ItemNum);
	DVR_RESULT MetaDataDestroy(void *pvMetaDataHandle);
	DVR_RESULT MetaDataGetItemNum(void *pvMetaDataHandle, DVR_U32 *pu32ItemNum);
	DVR_RESULT MetaDataGetDataByIndex(void *pvMetaDataHandle, DVR_METADATA_ITEM* pItem, DVR_U32 u32Index);
	DVR_RESULT MetaDataGetDataByType(void *pvMetaDataHandle, DVR_METADATA_ITEM* pItem, DVR_METADATA_TYPE eType);
	DVR_RESULT MetaDataGetMediaInfo(void *pvMetaDataHandle, DVR_MEDIA_INFO *pInfo);
	DVR_RESULT MetaDataGetTrackByIndex(void *pvMetaDataHandle, DVR_MEDIA_TRACK* pTrack, DVR_U32 u32Index);
	DVR_RESULT MetaDataGetPreview(DVR_PREVIEW_OPTION *pPreviewOpt, unsigned char *pBuf, unsigned int *pCbSize);
	DVR_RESULT FormatDisplayName(char *srcName, char *dstName, DVR_U32 u32DurationMs);
    DVR_RESULT LoadThumbNail(char *filename, unsigned char *pPreviewBuf, int nSize);

	DVR_RESULT DB_Mount(const char *pszDrive, const char *szDevice, int nDevId);
	DVR_RESULT DB_Unmount(const char *pszDrive);

    DVR_RESULT FileMapDB_ScanLibrary(const char *pszDrive);
	DVR_RESULT FileMapDB_SetFailedItem(const char *szFileName, unsigned eFolderType);
    DVR_RESULT FileMapDB_GetItemCountByType(DVR_U32 *puCount, unsigned eFolderType);
    DVR_RESULT FileMapDB_GetNameByRelPos(unsigned uRelPos, unsigned eFolderType, DVR_FILEMAP_META_ITEM *pszItem, int nSize);
	DVR_RESULT FileMapDB_GetFileInfo(const char *pszDrive, char *pszFullName, DVR_FOLDER_TYPE eFolderType, PDVR_DB_IDXFILE idxfile);
	DVR_RESULT FileMapDB_GetTypeByName(const char *pszDrive, char *szFileName, unsigned& eFolderType);
	DVR_RESULT FileMapDB_GePosByName(unsigned *puRelPos, const char *szFileName, unsigned eFolderType);
	DVR_RESULT FileMapDB_GetNextFile(const char *szFileName, DVR_FILEMAP_META_ITEM *pszItem, unsigned eFolderType);
	DVR_RESULT FileMapDB_GetPrevFile(const char *szFileName, DVR_FILEMAP_META_ITEM *pszItem, unsigned eFolderType);
    DVR_RESULT FileMapDB_DelItem(const char *szFileName, unsigned eFolderType);
    DVR_RESULT FileMapDB_AddItem(DVR_FILEMAP_META_ITEM *pszItem, unsigned eFolderType);
	DVR_RESULT FileMapDB_Clear();
    DVR_RESULT FileMapDB_GetDasOldNameByType(char *szDispName, unsigned eRecType);
    DVR_RESULT FileMapDB_GetDasCount(DVR_U32 *pApaCount, DVR_U32 *pAccCount, DVR_U32 *pIaccCount);


	DVR_RESULT ComSvcHcMgr_AttachHandler(DVR_HCMGR_HANDLER *handler, void *pContext);
	DVR_RESULT ComSvcHcmgr_DetachHandler(void);
	DVR_RESULT ComSvcHcmgr_ResetHandler(void);
	DVR_RESULT ComSvcHcmgr_SndMsg(APP_MESSAGE *msg, DVR_U32 waitOption);
	DVR_RESULT ComSvcHcmgr_SendMsg(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2);
	DVR_RESULT ComSvcHcmgr_RcvMsg(APP_MESSAGE *msg, DVR_U32 waitOption);

	DVR_RESULT ComSvcTimer_Register(int tid, PFN_APPTIMER_HANDLER handler);
	DVR_RESULT ComSvcTimer_UnRegister(int tid, PFN_APPTIMER_HANDLER handler);
	DVR_RESULT ComSvcTimer_Handler(int tid);
	DVR_RESULT ComSvcTimer_Start(int tid);
	DVR_RESULT ComSvcTimer_UnRegisterAll(void);

	DVR_RESULT ComSvcAsyncOp_CardFormat(void);
	DVR_RESULT ComSvcAsyncOp_FileCopy(char *srcFn, char *dstFn);
	DVR_RESULT ComSvcAsyncOp_FileMove(char *srcFn, char *dstFn);
	DVR_RESULT ComSvcAsyncOp_FileDel(char *filename);

    DVR_RESULT ComSvcSyncOp_FileCopy(char *srcFn, char *dstFn);
    DVR_RESULT ComSvcSyncOp_FileMove(char *srcFn, char *dstFn);
    DVR_RESULT ComSvcSyncOp_FileDel(char *filename);

	DVR_RESULT StorageAsyncOp_SndMsg(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2);
	DVR_RESULT StorageAsyncOp_RegHandler(DVR_STORAGE_HANDLER *handler);
    DVR_RESULT StorageAsyncOp_ClrHandler(void);

	DVR_RESULT StorageCard_Set(DVR_STORAGE_PROP prop, void *propData, int propSize);
	DVR_RESULT StorageCard_Get(DVR_STORAGE_PROP prop, void *propData, int propSize, int *pnSizeReturned);
	DVR_RESULT StorageCard_CheckStatus(void);

	DVR_RESULT MonitorStorage_Set(DVR_MONITOR_STORAGE_PROP prop, void *propData, int propSize);
	DVR_RESULT MonitorStorage_Get(DVR_MONITOR_STORAGE_PROP prop, void *propData, int propSize, int *pnSizeReturned);

	DVR_RESULT UpdateSystemTime(DVR_U32 year, DVR_U32 month, DVR_U32 day, DVR_U32 hour, DVR_U32 minute, DVR_U32 second);
	DVR_RESULT MarkSDSpeed(char *szDrive, char *pszpoint, DVR_SDSPEED_TYPE type);
	DVR_RESULT GetSDSpeekType(char *szDrive, char *pszpoint, DVR_U32 *type);

	static OsaNotifier *Notifier();
	
	char *CurrentMountPoint()
	{
		return m_pCurrentMountPoint;
	}

	char *CurrentDevicePath()
	{
		return m_pCurrentDevicePath;
	}

	void SetMountPoint(const char* pszMountPoint)
	{
		if(m_pCurrentDevicePath){
			free(m_pCurrentDevicePath);
			m_pCurrentDevicePath = NULL;
		}

		if(pszMountPoint != NULL){
			m_pCurrentMountPoint = strdup(pszMountPoint);
		}
	}

#ifdef __linux__
	DvrMonitorMount *GetMountMonitor()
#else
    DvrMonitorMount_Win32 *GetMountMonitor()
#endif
	{
		return m_pAutoMount;
	}

 private:
    DISABLE_COPY(DvrSystemControl)

    static void *EventHandler(void *p);
    static int GetMetadata(const char *szName, char **metaData);
    static int Notify(void *pContext, DVR_NOTIFICATION_TYPE enuType, void *pParam1, void *pParam2);
    int        NotifyImp(DVR_NOTIFICATION_TYPE enuType, void *pParam1, void *pParam2);
    void       OnNotifyDeviceEvent(DVR_DEVICE_EVENT devEvent, DVR_DEVICE *dev);

	static void DvrPlayerOnNewFrame(void *pInstance, void *pUserData);
	static void Monitor_TaskEntry(void *ptr);

	static OsaNotifier *m_pNotifier;
    static unsigned int m_photoIndex;
	PFN_PLAYER_NEWFRAME m_ptfnNewFrameCallBack;
	void *m_pNewFrameCallBackContext;

	PFN_NEW_VEHICLE_DATA m_ptfnRecorderUpdateVehicleData;
	void *m_pRecorderUpdateVehicleDataContext;

	PFN_NEW_VEHICLE_DATA m_ptfnPlayerUpdateVehicleData;
	void *m_pPlayerUpdateVehicleDataContext;

	DvrMutex	*m_mediaMutex;
	DvrMutex	*m_recorderMutex;
	DvrMutex	*m_playerMutex;
	DvrMutex	*m_fileMapDBMutex;
	DvrMutex	*m_errorFileMutex;
	
#ifdef __linux__
	DvrMonitorMount *        m_pAutoMount;
#else
	DvrMonitorMount_Win32 *  m_pAutoMount;
#endif

	IFileDBManger	    *m_pFileDBMgr;
	std::list<DVR_DEVICE> m_devices;

	DvrPlayerLoop		*m_player;
	DvrRecorderLoop		*m_recorder;
	DvrComSvcHcMgr		*m_pComSvcHcMgr;
	DvrComSvcTimer		*m_pComSvcTimer;
	DvrComSvcAsyncOp	*m_pComSvcAsyncOp;
    DvrComSvcSyncOp     *m_pComSvcSyncOp;
	DvrStorageAsyncOp	*m_pStorageAsyncOp;
	DvrStorageCard		*m_pStorageCard;
	DvrMonitorStorage	*m_pStorageMonitor;

	char *m_pCurrentMountPoint;
	char *m_pCurrentDevicePath;


    
    char m_GSTconsoleCallbckIsSet;
};

#endif
