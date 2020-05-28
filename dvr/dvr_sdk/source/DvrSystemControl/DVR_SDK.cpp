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

#include <cstddef>
#include <DVR_SDK_INTFS.h>

#include "DvrSystemControl.h"

#define CHECK_SYSCTRL_INITIALIZED \
    do {\
        if (g_sysCtrl == NULL)\
            return DVR_RES_EUNEXPECTED;\
    } while(0)

static DvrSystemControl *g_sysCtrl = NULL;

DVR_API DVR_RESULT Dvr_Sdk_Initialize(void)
{
    if (g_sysCtrl)
        return DVR_RES_EUNEXPECTED;

    g_sysCtrl = new DvrSystemControl();
    if (g_sysCtrl == NULL)
		return DVR_RES_EOUTOFMEMORY;

	return DVR_RES_SOK;
}

DVR_API DVR_RESULT Dvr_Sdk_DeInitialize()
{
    CHECK_SYSCTRL_INITIALIZED;

    delete g_sysCtrl;
    g_sysCtrl = NULL;

	return DVR_RES_SOK;
}

DVR_API DVR_RESULT Dvr_Sdk_EnumDevices(DVR_DEVICE_TYPE eType, _OUT DVR_DEVICE_ARRAY *pDevices)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->EnumDevices(eType, pDevices);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_SetActiveDrive(_IN DVR_DEVICE *szDrive)
{
	CHECK_SYSCTRL_INITIALIZED;

	if (szDrive == NULL)
		return DVR_RES_EPOINTER;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->SetActiveDrive(szDrive);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_ClearActiveDrive(void)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->ClearActiveDrive();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_GetActiveDrive(_IN DVR_DEVICE *szDrive)
{
	CHECK_SYSCTRL_INITIALIZED;

	if (szDrive == NULL)
		return DVR_RES_EPOINTER;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->GetActiveDrive(szDrive);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Free(_IN void *pAllocData)
{
	CHECK_SYSCTRL_INITIALIZED;

	if (pAllocData == NULL)
		return DVR_RES_EPOINTER;

	free(pAllocData);

	return DVR_RES_SOK;
}

DVR_API DVR_RESULT Dvr_Sdk_MediaMgr_Open()
{
    CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->MediaMgrOpen();

    return res;
}

DVR_API DVR_RESULT Dvr_Sdk_MediaMgr_Close()
{
    CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->MediaMgrClose();

    return res;
}

DVR_API DVR_RESULT Dvr_Sdk_MediaMgr_Reset(void)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->MediaMgrReset();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_RegisterNotify(_IN PFN_DVR_SDK_NOTIFY pCallback, _IN DVR_VOID *pContext)
{
    CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->RegisterNotify(pCallback, pContext);

    return res;
}

DVR_API DVR_RESULT Dvr_Sdk_UnRegisterNotify(_IN PFN_DVR_SDK_NOTIFY pCallback, _IN DVR_VOID *pContext)
{
    CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->UnRegisterNotify(pCallback, pContext);

    return res;
}

DVR_API DVR_RESULT Dvr_Sdk_RegisterConsole(_IN PFN_DVR_SDK_CONSOLE pCallback, _IN void *pContext)
{
    CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->RegisterConsole(pCallback, pContext);

    return res;
}
DVR_API DVR_RESULT Dvr_Sdk_RegisterGSTConsole(_IN PFN_DVR_SDK_CONSOLE pCallback, _IN void *pContext)
{
    CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->RegisterGSTConsole(pCallback, pContext);

    return res;
}


DVR_API DVR_RESULT Dvr_Sdk_UnRegisterConsole(_IN PFN_DVR_SDK_CONSOLE pCallback, _IN void *pContext)
{
    CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->UnregisterConsole(pCallback, pContext);

    return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Recorder_Init()
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->RecorderInit();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Recorder_DeInit()
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->RecorderDeInit();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Recorder_Reset()
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->RecorderReset();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Recorder_Start()
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->RecorderStart();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Recorder_Stop()
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->RecorderStop();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Recorder_AcquireInputBuf(_OUT DVR_VOID **ppvBuffer)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->RecorderAcquireInputBuf(ppvBuffer);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Recorder_AsyncOpFlush()
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->RecorderAsyncOpFlush();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Recorder_AddFrame(_IN DVR_IO_FRAME *pInputFrame)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->RecorderAddFrame(pInputFrame);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Recorder_Set(DVR_RECORDER_PROP eProp, _IN DVR_VOID *pPropData, DVR_S32 nPropSize)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->RecorderSet(eProp, pPropData, nPropSize);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Recorder_Get(DVR_RECORDER_PROP eProp, _OUT DVR_VOID *pPropData, DVR_S32 nPropSize, _OUT DVR_S32 *pnSizeReturned)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->RecorderGet(eProp, pPropData, nPropSize, pnSizeReturned);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Recorder_TakePhoto(DVR_PHOTO_PARAM *pParam)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->RecorderPhotoTake(pParam);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Recorder_TakeOverlay(void *canbuffer, void *osdbuffer,int cansize, int osdsize)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->RecorderTakeOverlay(canbuffer,osdbuffer, cansize, osdsize);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Recorder_LoopRec_Start(void)
{
    CHECK_SYSCTRL_INITIALIZED;

    DVR_RESULT res = DVR_RES_SOK;
    DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->RecorderLoopRecStart();

    return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Recorder_LoopRec_Stop(void)
{
    CHECK_SYSCTRL_INITIALIZED;

    DVR_RESULT res = DVR_RES_SOK;
    DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->RecorderLoopRecStop();

    return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Recorder_EventRec_Start(void)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->RecorderEventRecStart();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Recorder_EventRec_Stop(void)
{
    CHECK_SYSCTRL_INITIALIZED;

    DVR_RESULT res = DVR_RES_SOK;
    DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->RecorderEventRecStop();

    return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Recorder_DasRec_Start()
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->RecorderDasRecStart();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Recorder_DasRec_Stop()
{
    CHECK_SYSCTRL_INITIALIZED;

    DVR_RESULT res = DVR_RES_SOK;
    DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->RecorderDasRecStop();

    return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Recorder_IaccRec_Start(int type)
{
    CHECK_SYSCTRL_INITIALIZED;

    DVR_RESULT res = DVR_RES_SOK;
    DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->RecorderIaccRecStart(type);

    return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Recorder_IaccRec_Stop(int type)
{
    CHECK_SYSCTRL_INITIALIZED;

    DVR_RESULT res = DVR_RES_SOK;
    DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->RecorderIaccRecStop(type);

    return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Recorder_RegisterVehDataUpdateCallBack(PFN_NEW_VEHICLE_DATA ptfnNewVehData, void *pContext)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->RecorderRegisterVehDataUpdateCallBack(ptfnNewVehData, pContext);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Player_Init()
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->PlayerInit();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Player_DeInit()
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->PlayerDeInit();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Player_Open(_IN const char *szFileName, _IN int type)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->PlayerOpen(szFileName,type);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Player_Close()
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->PlayerClose();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Player_Play()
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->PlayerPlay();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Player_Stop()
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->PlayerStop();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Player_Pause()
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->PlayerPause();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Player_Resume()
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->PlayerResume();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Player_Next()
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->PlayerNext();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Player_Prev()
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->PlayerPrev();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Player_StepF()
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->PlayerStepF();

	return res;
}
DVR_API DVR_RESULT Dvr_Sdk_Player_StepB()
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->PlayerStepB();

	return res;
}


DVR_API DVR_RESULT Dvr_Sdk_Player_Advance(int nDelta)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->PlayerDelta(nDelta);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Player_Set(DVR_PLAYER_PROP eProp, _IN DVR_VOID *pPropData, DVR_S32 nPropSize)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->PlayerSet(eProp, pPropData, nPropSize);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Player_Get(DVR_PLAYER_PROP eProp, _OUT DVR_VOID *pPropData, DVR_S32 nPropSize, _OUT DVR_S32 *pnSizeReturned)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->PlayerGet(eProp, pPropData, nPropSize, pnSizeReturned);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Player_PrintScreen(DVR_PHOTO_PARAM *pParam)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->PlayerPrintScreen(pParam);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Player_RegisterCallBack(PFN_PLAYER_NEWFRAME ptfnNewFrame, void *pContext)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->PlayerRegisterCallBack(ptfnNewFrame, pContext);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Player_RegisterVehDataUpdateCallBack(PFN_NEW_VEHICLE_DATA ptfnNewVehData, void *pContext)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->PlayerRegisterVehDataUpdateCallBack(ptfnNewVehData, pContext);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Player_Frame_Update(DVR_IO_FRAME *pInputFrame)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->PlayerFrameUpdate(pInputFrame);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Photo_Open(_IN const char *szFileName)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->PhotoOpen(szFileName);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Photo_Close(void)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->PhotoClose();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Photo_Next(void)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->PhotoNext();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Photo_Prev(void)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->PhotoPrev();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_Photo_Advance(int nDelta)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->PhotoDelta(nDelta);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_DB_Mount(_IN const char *szDrive, _IN const char *szDevice, _IN int nDevId)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->DB_Mount(szDrive, szDevice, nDevId);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_DB_Unmount(_IN const char *szDrive)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->DB_Unmount(szDrive);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_FileMapDB_ScanLibrary(_IN const char *szDrive)
{
    CHECK_SYSCTRL_INITIALIZED;

    DVR_RESULT res = DVR_RES_SOK;
    DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->FileMapDB_ScanLibrary(szDrive);

    return res;
}

DVR_API DVR_RESULT Dvr_Sdk_FileMapDB_SetFailedItem(_IN const char *szFileName, _IN unsigned eFolderType)
{
    CHECK_SYSCTRL_INITIALIZED;

    DVR_RESULT res = DVR_RES_SOK;
    DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->FileMapDB_SetFailedItem(szFileName, eFolderType);

    return res;
}

DVR_API DVR_RESULT Dvr_Sdk_FileMapDB_GetItemCountByType(_OUT DVR_U32 *puCount, _IN unsigned eFolderType)
{
    CHECK_SYSCTRL_INITIALIZED;

    DVR_RESULT res = DVR_RES_SOK;
    DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->FileMapDB_GetItemCountByType(puCount, eFolderType);

    return res;
}

DVR_API DVR_RESULT Dvr_Sdk_FileMapDB_GetNameByRelPos(_IN unsigned uRelPos, _IN unsigned eFolderType, _OUT DVR_FILEMAP_META_ITEM *pszItem, _IN int nSize)
{
    CHECK_SYSCTRL_INITIALIZED;

    DVR_RESULT res = DVR_RES_SOK;
    DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->FileMapDB_GetNameByRelPos(uRelPos, eFolderType, pszItem, nSize);

    return res;
}

DVR_API DVR_RESULT Dvr_Sdk_FileMapDB_GePosByName(_OUT unsigned *puRelPos, _IN const char *szFileName, _IN unsigned eFolderType)
{
    CHECK_SYSCTRL_INITIALIZED;

    DVR_RESULT res = DVR_RES_SOK;
    DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->FileMapDB_GePosByName(puRelPos, szFileName, eFolderType);

    return res;
}

DVR_API DVR_RESULT Dvr_Sdk_FileMapDB_GetNextFile(_IN const char *szFileName, _OUT DVR_FILEMAP_META_ITEM *pszItem, _IN unsigned eFolderType)
{
    CHECK_SYSCTRL_INITIALIZED;

    DVR_RESULT res = DVR_RES_SOK;
    DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->FileMapDB_GetNextFile(szFileName, pszItem, eFolderType);

    return res;
}


DVR_API DVR_RESULT Dvr_Sdk_FileMapDB_GetPrevFile(_IN const char *szFileName, _OUT DVR_FILEMAP_META_ITEM *pszItem, _IN unsigned eFolderType)
{
    CHECK_SYSCTRL_INITIALIZED;

    DVR_RESULT res = DVR_RES_SOK;
    DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->FileMapDB_GetPrevFile(szFileName, pszItem, eFolderType);

    return res;
}


DVR_API DVR_RESULT Dvr_Sdk_FileMapDB_GetFileInfo(_IN const char *pszDrive, _IN char *pszFullName, _IN DVR_FOLDER_TYPE eFolderType, _OUT PDVR_DB_IDXFILE idxfile)
{
    CHECK_SYSCTRL_INITIALIZED;

    DVR_RESULT res = DVR_RES_SOK;
    DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->FileMapDB_GetFileInfo(pszDrive, pszFullName, eFolderType, idxfile);

    return res;
}


DVR_API DVR_RESULT Dvr_Sdk_FileMapDB_DelItem(_IN const char *szFileName, _IN unsigned eFolderType)
{
    CHECK_SYSCTRL_INITIALIZED;

    DVR_RESULT res = DVR_RES_SOK;
    DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->FileMapDB_DelItem(szFileName, eFolderType);

    return res;
}

DVR_API DVR_RESULT Dvr_Sdk_FileMapDB_AddItem(_IN DVR_FILEMAP_META_ITEM *pszItem, _IN unsigned eFolderType)
{
    CHECK_SYSCTRL_INITIALIZED;

    DVR_RESULT res = DVR_RES_SOK;
    DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->FileMapDB_AddItem(pszItem, eFolderType);

    return res;
}

DVR_API DVR_RESULT Dvr_Sdk_FileMapDB_Clear(void)
{
    CHECK_SYSCTRL_INITIALIZED;

    DVR_RESULT res = DVR_RES_SOK;
    DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->FileMapDB_Clear();

    return res;
}

DVR_API DVR_RESULT Dvr_Sdk_ComSvcHcMgr_AttachHandler(_IN DVR_HCMGR_HANDLER *handler, _IN void *pContext)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->ComSvcHcMgr_AttachHandler(handler, pContext);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_ComSvcHcMgr_DetachHandler()
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->ComSvcHcmgr_DetachHandler();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_ComSvcHcMgr_ResetHandler()
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->ComSvcHcmgr_ResetHandler();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_ComSvcHcMgr_SndMsg(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	APP_MESSAGE TempMessage = { 0 };

	TempMessage.MessageID = msg;
	TempMessage.MessageData[0] = param1;
	TempMessage.MessageData[1] = param2;

	res = sysCtrl->ComSvcHcmgr_SndMsg(&TempMessage, DVR_TIMEOUT_NO_WAIT);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_ComSvcHcMgr_RcvMsg(_OUT APP_MESSAGE *msg, _IN DVR_U32 waitOption)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->ComSvcHcmgr_RcvMsg(msg, waitOption);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_ComSvrTimer_Register(int tid, PFN_APPTIMER_HANDLER handler)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->ComSvcTimer_Register(tid, handler);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_ComSvrTimer_UnRegister(int tid, PFN_APPTIMER_HANDLER handler)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->ComSvcTimer_UnRegister(tid, handler);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_ComSvrTimer_Handler(int tid)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->ComSvcTimer_Handler(tid);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_ComSvrTimer_Start(int tid)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->ComSvcTimer_Start(tid);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_ComSvrTimer_UnRegisterAll()
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->ComSvcTimer_UnRegisterAll();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_ComSvcAsyncOp_CardFormat()
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->ComSvcAsyncOp_CardFormat();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_ComSvcAsyncOp_FileCopy(char *srcFn, char *dstFn)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->ComSvcAsyncOp_FileCopy(srcFn, dstFn);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_ComSvcAsyncOp_FileMove(char *srcFn, char *dstFn)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->ComSvcAsyncOp_FileMove(srcFn, dstFn);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_ComSvcAsyncOp_FileDel(char *filename)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->ComSvcAsyncOp_FileDel(filename);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_ComSvcSyncOp_FileCopy(char *srcFn, char *dstFn)
{
    CHECK_SYSCTRL_INITIALIZED;

    DVR_RESULT res = DVR_RES_SOK;
    DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->ComSvcSyncOp_FileCopy(srcFn, dstFn);

    return res;
}

DVR_API DVR_RESULT Dvr_Sdk_ComSvcSyncOp_FileMove(char *srcFn, char *dstFn)
{
    CHECK_SYSCTRL_INITIALIZED;

    DVR_RESULT res = DVR_RES_SOK;
    DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->ComSvcSyncOp_FileMove(srcFn, dstFn);

    return res;
}

DVR_API DVR_RESULT Dvr_Sdk_ComSvcSyncOp_FileDel(char *filename)
{
    CHECK_SYSCTRL_INITIALIZED;

    DVR_RESULT res = DVR_RES_SOK;
    DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->ComSvcSyncOp_FileDel(filename);

    return res;
}

DVR_API DVR_RESULT Dvr_Sdk_StorageAsyncOp_SndMsg(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->StorageAsyncOp_SndMsg(msg, param1, param2);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_StorageAsyncOp_RegHandler(DVR_STORAGE_HANDLER *handler)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->StorageAsyncOp_RegHandler(handler);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_StorageCard_Set(DVR_STORAGE_PROP eProp, _IN DVR_VOID *pPropData, DVR_S32 nPropSize)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->StorageCard_Set(eProp, pPropData, nPropSize);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP eProp, _OUT DVR_VOID *pPropData, DVR_S32 nPropSize, _OUT DVR_S32 *pnSizeReturned)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->StorageCard_Get(eProp, pPropData, nPropSize, pnSizeReturned);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_StorageCard_CheckStatus(void)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->StorageCard_CheckStatus();

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_MonitorStorage_Set(DVR_MONITOR_STORAGE_PROP eProp, _IN DVR_VOID *pPropData, DVR_S32 nPropSize)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->MonitorStorage_Set(eProp, pPropData, nPropSize);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_MonitorStorage_Get(DVR_MONITOR_STORAGE_PROP eProp, _OUT DVR_VOID *pPropData, DVR_S32 nPropSize, _OUT DVR_S32 *pnSizeReturned)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->MonitorStorage_Get(eProp, pPropData, nPropSize, pnSizeReturned);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_MetaData_Create(void **ppMetaDataHandle, void* pvFileName, DVR_U32 *pu32ItemNum)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->MetaDataCreate(ppMetaDataHandle, pvFileName, pu32ItemNum);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_MetaData_Destroy(void *pvMetaDataHandle)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->MetaDataDestroy(pvMetaDataHandle);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_MetaData_GetItemNum(void *pvMetaDataHandle, _OUT DVR_U32 *pu32ItemNum)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->MetaDataGetItemNum(pvMetaDataHandle, pu32ItemNum);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_MetaData_GetDataByIndex(void *pvMetaDataHandle, _OUT DVR_METADATA_ITEM* pItem, _IN DVR_U32 u32Index)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->MetaDataGetDataByIndex(pvMetaDataHandle, pItem, u32Index);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_MetaData_GetDataByType(void *pvMetaDataHandle, _OUT DVR_METADATA_ITEM* pItem, _IN DVR_METADATA_TYPE eType)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->MetaDataGetDataByType(pvMetaDataHandle, pItem, eType);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_MetaData_GetMediaInfo(void *pvMetaDataHandle, _OUT DVR_MEDIA_INFO *pInfo)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->MetaDataGetMediaInfo(pvMetaDataHandle, pInfo);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_MetaData_GetTrackByIndex(void *pvMetaDataHandle, _OUT DVR_MEDIA_TRACK* pTrack, _IN DVR_U32 u32Index)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->MetaDataGetTrackByIndex(pvMetaDataHandle, pTrack, u32Index);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_MetaData_GetFilePreview(DVR_PREVIEW_OPTION *pPreviewOpt, DVR_U8 *pBuf, DVR_U32 *pCbSize)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->MetaDataGetPreview(pPreviewOpt, pBuf, pCbSize);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_FormatDisplayName(char *srcName, char *dstName, DVR_U32 u32DurationMs)
{
	CHECK_SYSCTRL_INITIALIZED;

	DVR_RESULT res = DVR_RES_SOK;
	DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

	res = sysCtrl->FormatDisplayName(srcName, dstName, u32DurationMs);

	return res;
}

DVR_API DVR_RESULT Dvr_Sdk_LoadThumbNail(char *filename, unsigned char *pPreviewBuf, int nSize)
{
    CHECK_SYSCTRL_INITIALIZED;

    DVR_RESULT res = DVR_RES_SOK;
    DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->LoadThumbNail(filename, pPreviewBuf, nSize);

    return res;
}

DVR_API DVR_RESULT Dvr_Sdk_UpdateSystemTime(DVR_U32 year, DVR_U32 month, DVR_U32 day, DVR_U32 hour, DVR_U32 minute, DVR_U32 second)
{
    CHECK_SYSCTRL_INITIALIZED;

    DVR_RESULT res = DVR_RES_SOK;
    DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->UpdateSystemTime(year, month, day, hour, minute, second);

    return res;
}

DVR_API DVR_RESULT Dvr_Sdk_MarkSDSpeed(_IN char *szDrive, _IN char *pszpoint, _IN DVR_SDSPEED_TYPE type)
{
    CHECK_SYSCTRL_INITIALIZED;

    DVR_RESULT res = DVR_RES_SOK;
    DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->MarkSDSpeed(szDrive, pszpoint, type);

    return res;
}

DVR_API DVR_RESULT Dvr_Sdk_GetSDSpeekType(_IN  char *szDrive, _IN char *pszpoint, _OUT DVR_U32 *type)
{
    CHECK_SYSCTRL_INITIALIZED;

    DVR_RESULT res = DVR_RES_SOK;
    DvrSystemControl *sysCtrl = (DvrSystemControl *)g_sysCtrl;

    res = sysCtrl->GetSDSpeekType(szDrive, pszpoint, type);

    return res;
}

#if 1

#ifdef _MANAGED
#pragma managed(push, off)
#endif
#ifndef __linux__
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
    return TRUE;
}
#endif

#ifdef _MANAGED
#pragma managed(pop)
#endif

#endif


