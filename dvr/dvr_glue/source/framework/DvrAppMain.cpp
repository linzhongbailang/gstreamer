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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "framework/DvrAppMain.h"
#include "framework/DvrAppUtil.h"
#include "framework/DvrAppNotify.h"
#include "flow/util/CThreadPool.h"
#include "flow/util/DvrMetaScan.h"
#include <event/ofilm_msg_type.h>
#include <event/AvmEventTypes.h>
#include <event/AvmEvent.h>
#include <log/log.h>
#include <DVR_APP_INTFS_INPUT.h>

int  DvrAppMainControl::m_nLogLevel = 3;
DvrAppMainControl *DvrAppMainControl::m_pDvrMainCtrl = NULL;
DVR_APP_MODE DvrAppMainControl::m_curAppMode = DVR_APP_MODE_NONE;
static unsigned long m_lasttime = 0;

static int InitDvrStorageQuota(void)
{
	/* Set Storage Quota*/
	DVR_U32 nCardTotalSpace = 0;
	DVR_U32 nCardUsedSpace = 0;
	Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_CARD_TOTALSPACE, &nCardTotalSpace, sizeof(DVR_U32), NULL);
	Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_CARD_USEDSPACE, &nCardUsedSpace, sizeof(DVR_U32), NULL);

	DVR_U32 nNormalUsedSpace = 0, nEventUsedSpace = 0, nPhotoUsedSpace = 0, nDasUsedSpace = 0;
	Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_NORMAL_FOLDER_USEDSPACE, &nNormalUsedSpace, sizeof(DVR_U32), NULL);
	Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_EVENT_FOLDER_USEDSPACE, &nEventUsedSpace, sizeof(DVR_U32), NULL);
	Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_PHOTO_FOLDER_USEDSPACE, &nPhotoUsedSpace, sizeof(DVR_U32), NULL);
	Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_DAS_FOLDER_USEDSPACE, &nDasUsedSpace, sizeof(DVR_U32), NULL);

	DVR_U32 nUsedSpaceExceptDVR = nCardUsedSpace - nNormalUsedSpace - nEventUsedSpace - nPhotoUsedSpace - nDasUsedSpace;
	DVR_U32 nCardSpaceForDVR = nCardTotalSpace - nUsedSpaceExceptDVR;

	DVR_STORAGE_QUOTA nStorageQuota;
	nStorageQuota.nEventStorageQuota = (nCardSpaceForDVR - DVR_STORAGE_DAS_FOLDER_QUOTA - DVR_STORAGE_PHOTO_FOLDER_QUOTA - DVR_LOOPREC_STORAGE_FREESPACE_THRESHOLD) * 3 / 10;
	nStorageQuota.nPhotoStorageQuota = DVR_STORAGE_PHOTO_FOLDER_QUOTA;
	nStorageQuota.nDasStorageQuota = DVR_STORAGE_DAS_FOLDER_QUOTA;
	nStorageQuota.nNormalStorageQuota = nCardSpaceForDVR - nStorageQuota.nEventStorageQuota - nStorageQuota.nPhotoStorageQuota - nStorageQuota.nDasStorageQuota;
	
	Log_Error("nCardTotalSpace %d,nUsedSpaceExceptDVR%d, nEventUsedSpace %d, nPhotoUsedSpace %d,nDasUsedSpace %d,nNormalUsedSpace %d\n",
			nCardTotalSpace,nUsedSpaceExceptDVR,nEventUsedSpace,nPhotoUsedSpace,nDasUsedSpace,nNormalUsedSpace);
	Log_Error("nCardSpaceForDVR %d, nEventStorageQuota %d, nPhotoStorageQuota %d,nDasStorageQuota %d,nNormalStorageQuota %d\n",
        nCardSpaceForDVR,nStorageQuota.nEventStorageQuota,nStorageQuota.nPhotoStorageQuota,nStorageQuota.nDasStorageQuota,nStorageQuota.nNormalStorageQuota);

	Dvr_Sdk_StorageCard_Set(DVR_STORAGE_PROP_CARD_QUOTA, &nStorageQuota, sizeof(DVR_STORAGE_QUOTA));
	Dvr_Sdk_MonitorStorage_Set(DVR_MONITOR_STORAGE_PROP_NORMAL_QUOTA, &nStorageQuota.nNormalStorageQuota, sizeof(DVR_U32));/**<set normal storage monitor quota*/
	Dvr_Sdk_MonitorStorage_Set(DVR_MONITOR_STORAGE_PROP_CARD_AVAILABLE_SPACE, &nCardSpaceForDVR, sizeof(DVR_U32));/**<set card available total space*/

    if(nCardTotalSpace < DVR_STORAGE_SDCARD_TOTAL_SPACE_THRESHOLD || 
        nCardSpaceForDVR < DVR_STORAGE_SDCARD_SPACE_FOR_DVR_THRESHOLD)
    {
        DVR_BOOL bFreeSpaceTooSmall = TRUE;
        Dvr_Sdk_StorageCard_Set(DVR_STORAGE_PROP_FREE_SPACE_STATUS, &bFreeSpaceTooSmall, sizeof(DVR_BOOL));
    }
    else
    {
        DVR_BOOL bFreeSpaceTooSmall = FALSE;
        Dvr_Sdk_StorageCard_Set(DVR_STORAGE_PROP_FREE_SPACE_STATUS, &bFreeSpaceTooSmall, sizeof(DVR_BOOL));
    }

    return DVR_RES_SOK;
}

static int SdkNotify(void *pContext, DVR_NOTIFICATION_TYPE enuType, void *pParam1, void *pParam2)
{
	DvrAppMainControl *pDvrMainCtrl = (DvrAppMainControl *)pContext;

	switch (enuType)
	{
	case DVR_NOTIFICATION_TYPE_PLAYERROR:
	{
		DVR_RESULT result = *(DVR_RESULT *)pParam1;
		Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_PLAYER_OPEN_RESULT, result, 0);
	}
	break;

	case DVR_NOTIFICATION_TYPE_PLAYERSTATE:
	{
		DVR_PLAYER_STATE enuState = *(DVR_PLAYER_STATE *)pParam1;
		DVR_U32 AVM_DVRReplayCommandStatus; //0:No Request, 0x1:Play, 0x2:Pause, 0x3:Invalid
		
		if (enuState == DVR_PLAYER_STATE_UNKNOWN) {
			Log_Message("Player State Notified: Unknown State\n");
		}
		if (enuState == DVR_PLAYER_STATE_CLOSE) {
            Log_Message("Player State Notified: Close State\n");
		}
		if (enuState == DVR_PLAYER_STATE_INVALID) {
            Log_Message("Player State Notified: Invalid State\n");
		}
		if (enuState == DVR_PLAYER_STATE_PLAY) {
            Log_Message("Player State Notified: Play State\n");
			
			AVM_DVRReplayCommandStatus = 1;
			DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_PLAYBACK_STATUS_UPDATE, (void *)&AVM_DVRReplayCommandStatus, NULL);
		}
		if (enuState == DVR_PLAYER_STATE_STOP) {
            Log_Message("Player State Notified: Stop State\n");
		}
		if (enuState == DVR_PLAYER_STATE_PAUSE) {
            Log_Message("Player State Notified: Pause State\n");
			
			AVM_DVRReplayCommandStatus = 2;
			DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_PLAYBACK_STATUS_UPDATE, (void *)&AVM_DVRReplayCommandStatus, NULL);
		}
		if (enuState == DVR_PLAYER_STATE_FASTFORWARD) {
            Log_Message("Player State Notified: Fast Forward\n");
		}
		if (enuState == DVR_PLAYER_STATE_FASTBACKWARD) {
            Log_Message("Player State Notified: Fast Backward\n");
		}
	}
	break;

	case DVR_NOTIFICATION_TYPE_DEVICEEVENT:
	{
		DVR_DEVICE_EVENT enuEvent = *(DVR_DEVICE_EVENT *)pParam1;
		DVR_DEVICE       device = *(DVR_DEVICE *)pParam2;

		const char *DEVTYPE[] = {
			"UNKNOWN",
			"SD",
			"USBHD",
			"USBHUB",
		};

		if (enuEvent == DVR_DEVICE_EVENT_PLUGIN)
		{
            Log_Message("Device Plug-in: %s %s %s\n", DEVTYPE[device.eType], device.szMountPoint, device.szDevicePath);

			if (device.eType == DVR_DEVICE_TYPE_USBHD) 
			{
				DVR_BOOL bBlock = 1, bCreate = 0;
				int nPriority = 0;
				
				Dvr_Sdk_SetActiveDrive(&device);
				Dvr_Sdk_DB_Mount(device.szMountPoint, device.szDevicePath, 0);
				//Dvr_Sdk_ComSvcHcMgr_SndMsg(AMSG_STATE_CARD_INSERT_ACTIVE, 0, 0);
				//DVR_DB_SCAN_STATE state = DVR_SCAN_STATE_COMPLETE;
				//OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_SCANSTATE, &state, sizeof(state),
				//	szDrive, (long)(strlen(szDrive) + 1), m_pNotifier);
#if 1
				Log_Message("Filesystem scanning for %s has completed, start to create filemap DB\n", device.szMountPoint);
				
				DVR_RESULT res = Dvr_Sdk_FileMapDB_ScanLibrary(device.szMountPoint);
				if(res != DVR_RES_SOK)
				{
					Log_Error("Dvr_Sdk_FileMapDB_ScanLibrary failed\n");
				}
				else
				{
					Log_Message("Dvr_Sdk_FileMapDB_ScanLibrary done!\n");
					InitDvrStorageQuota();
					//Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_THUMB_UPDATE_FRAME, TRUE, 0);
					Dvr_Sdk_ComSvcHcMgr_SndMsg(AMSG_STATE_CARD_INSERT_ACTIVE, 0, 0);
				}
#endif
			}
		}
		else if (enuEvent == DVR_DEVICE_EVENT_PLUGOUT)
		{
            Log_Message("Device Plug-out: %s %s %s\n", DEVTYPE[device.eType], device.szMountPoint, device.szDevicePath);
			
			DVR_BOOL bFreeSpaceTooSmall = FALSE;
			Dvr_Sdk_StorageCard_Set(DVR_STORAGE_PROP_FREE_SPACE_STATUS, &bFreeSpaceTooSmall, sizeof(DVR_BOOL));
			DVR_BOOL bCardNotReady = TRUE;
			Dvr_Sdk_StorageCard_Set(DVR_STORAGE_PROP_CARD_READY_STATUS, &bCardNotReady, sizeof(DVR_BOOL));

			if (device.eType == DVR_DEVICE_TYPE_USBHD)
			{			
				Dvr_Sdk_FileMapDB_Clear();
				Dvr_Sdk_DB_Unmount(device.szMountPoint);				
				Dvr_Sdk_ClearActiveDrive();
				Dvr_Sdk_ComSvcHcMgr_SndMsg(AMSG_STATE_CARD_REMOVED, 0, 0);			
			}
			else if(device.eType == DVR_DEVICE_TYPE_UNKNOWN)
			{
				Dvr_Sdk_ClearActiveDrive();

				DVR_U32 DVR_SDCardErrorStatus = DVR_SDCARD_ERROR_STATUS_NO_SDCARD; 
				DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_ERROR_STATUS_UPDATE, (void *)&DVR_SDCardErrorStatus, NULL);
			}
			else
			{
				//TODO
			}	
		}
		else if(enuEvent == DVR_DEVICE_EVENT_FORMAT_NOTSUPPORT)
		{
            Log_Message("Device Recognize Error: %s %s %s\n", DEVTYPE[device.eType], device.szMountPoint, device.szDevicePath);

			if(device.eType == DVR_DEVICE_TYPE_UNKNOWN)
			{
				Dvr_Sdk_SetActiveDrive(&device);
			
				DVR_U32 DVR_SDCardErrorStatus = DVR_SDCARD_ERROR_STATUS_FORMAT_ERROR; 
				DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_ERROR_STATUS_UPDATE, (void *)&DVR_SDCardErrorStatus, NULL);
			}
		}
		else if(enuEvent == DVR_DEVICE_EVENT_ERROR)
		{
			DVR_U32 DVR_SDCardErrorStatus = DVR_SDCARD_ERROR_STATUS_NO_SDCARD; 
			DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_ERROR_STATUS_UPDATE, (void *)&DVR_SDCardErrorStatus, NULL);
		}
		else if(enuEvent == DVR_DEVICE_EVENT_FSCHECK)
		{
            Log_Message("Device File System Check: %s %s %s\n", DEVTYPE[device.eType], device.szMountPoint, device.szDevicePath);
			
			DVR_U32 DVR_SDCardErrorStatus = DVR_SDCARD_ERROR_STATUS_RESTORING; 
			DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_ERROR_STATUS_UPDATE, (void *)&DVR_SDCardErrorStatus, NULL);
		}
		else
		{
			//TODO
		}
	}
	break;

	case DVR_NOTIFICATION_TYPE_SCANSTATE:
	{
		DVR_RESULT res = DVR_RES_SOK;
		DVR_DB_SCAN_STATE state = *(DVR_DB_SCAN_STATE *)pParam1;
		const char *szDrive = (const char *)pParam2;
		if (state == DVR_SCAN_STATE_COMPLETE)
		{
            Log_Message("Filesystem scanning for %s has completed, start to create filemap DB\n", szDrive);
			
			res = Dvr_Sdk_FileMapDB_ScanLibrary(szDrive);
			if(res != DVR_RES_SOK)
			{
				Log_Error("Dvr_Sdk_FileMapDB_ScanLibrary failed\n");
			}
			else
			{
				Log_Message("Dvr_Sdk_FileMapDB_ScanLibrary done!\n");
				InitDvrStorageQuota();
				//Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_THUMB_UPDATE_FRAME, TRUE, 0);
				Dvr_Sdk_ComSvcHcMgr_SndMsg(AMSG_STATE_CARD_INSERT_ACTIVE, 0, 0);
			}
		}
		else if (state == DVR_SCAN_STATE_PARTIAL)
		{
            Log_Message("Filesystem scanning partial completed szDrive = %s\n", szDrive);
		}
	}
	break;

	case DVR_NOTIFICATION_TYPE_POSITION:
	{		
        GUI_OBJ_PLAY_TIME_INST play_time;
        memset(&play_time, 0, sizeof(play_time));
		
        play_time.position = *(unsigned int *)pParam1;
        play_time.duration = *(unsigned int *)pParam2;

		Log_Debug("Session Pos = %u s/%u s", play_time.position, play_time.duration);

	    DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_PLAY_TIMER_UPDATE, (void *)&play_time, NULL);		
	}
	break;

    case DVR_NOTIFICATION_TYPE_RECORDER_EMERGENCY_COMPLETE:
    {
    	DVR_BOOL bFailed = *(DVR_BOOL *)pParam1;
		
        Dvr_Sdk_ComSvcHcMgr_SndMsg(AMSG_CMD_EVENT_RECORDER_EMERGENCY_COMPLETE, bFailed, 0);
    }
    break;

	case DVR_NOTIFICATION_TYPE_RECORDER_ALARM_COMPLETE:
	{
		Dvr_Sdk_ComSvcHcMgr_SndMsg(AMSG_CMD_EVENT_RECORDER_ALARM_COMPLETE, 0, 0);
	}
	break;

    case DVR_NOTIFICATION_TYPE_RECORDER_IACC_COMPLETE:
    {
        Dvr_Sdk_ComSvcHcMgr_SndMsg(AMSG_CMD_EVENT_RECORDER_IACC_COMPLETE, 0, 0);
    }
    break;

	case DVR_NOTIFICATION_TYPE_FILEEVENT:
	{
		DVR_RECORDER_FILE_OP *pfileOP = (DVR_RECORDER_FILE_OP *)pParam1;
		if (pParam1 == 0)
			return -1;

		if (pfileOP->eEvent == DVR_FILE_EVENT_CREATE)
		{
	        Log_Message("Recorder a new file finished\n");
			Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_THUMB_UPDATE_FRAME, FALSE, 0);
		}
	}
	break;
	
    case DVR_NOTIFICATION_TYPE_RECORDER_DESTROY_DONE:
    {
        Log_Message("Recorder destroy done\n");
		Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_THUMB_UPDATE_FRAME, TRUE, 0);
    }
    break;

	case DVR_NOTIFICATION_TYPE_PHOTO_DONE:
	{
		Log_Message("photo done\n");
	}
	break;

    case DVR_NOTIFICATION_TYPE_RECORDER_LOOPREC_FATAL_RECOVER:
    {
        Log_Message("Loop Recorder data recovery, start recording!!!");
        Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_RECORDER_ONOFF, 1, 1);
    }
    break;

    case DVR_NOTIFICATION_TYPE_RECORDER_LOOPREC_FATAL_ERROR:
    {
		Log_Message("Loop Recorder fatal error, stop recording!!!");
        if(pParam1 != NULL)
        {
            DVR_BOOL bFailed = *(DVR_BOOL *)pParam1;
            if(bFailed)
            {
                DVR_U32 DVR_SDCardErrorStatus = DVR_SDCARD_ERROR_STATUS_WRITE_ERROR; //write error
                DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_ERROR_STATUS_UPDATE, (void *)&DVR_SDCardErrorStatus, NULL);
            }
        }
		Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_RECORDER_ONOFF, 0, 1);
    }
    break;

	case DVR_NOTIFICATION_TYPE_RECORDER_DASREC_FATAL_ERROR:
	{
		Log_Message("DAS Recorder fatal error, stop recording!!!");
		Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_RECORDER_DAS_RECORD_STOP, 1, 0);
	}
	break;

    case DVR_NOTIFICATION_TYPE_RECORDER_SLOW_WRITING:
    {
        Log_Message("SD slow writing, stop recording!!!");

        DVR_U32 DVR_SDCardErrorStatus = DVR_SDCARD_ERROR_STATUS_LOW_SPEED; //low speed
        DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_ERROR_STATUS_UPDATE, (void *)&DVR_SDCardErrorStatus, NULL);
		
		DVR_BOOL bIsLowSpeed = TRUE;
		Dvr_Sdk_StorageCard_Set(DVR_STORAGE_PROP_CARD_SPEED_STATUS, &bIsLowSpeed, sizeof(DVR_BOOL));
		
        Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_RECORDER_ONOFF, 0, 2);

        DVR_DEVICE CurDrive;
        memset(&CurDrive, 0, sizeof(DVR_DEVICE));
        Dvr_Sdk_GetActiveDrive(&CurDrive);
        Dvr_Sdk_MarkSDSpeed(CurDrive.szDevicePath, CurDrive.szMountPoint, DVR_SD_SPEED_BAD);
    }
    break;

	default:
		break;
	}

	return DVR_RES_SOK;
}

void DvrAppMainControl::Console(void *pContext, int nLevel, const char *szString)
{
	DvrAppMainControl *p = static_cast<DvrAppMainControl *>(pContext);
	if (nLevel <= p->LogLevel())
		Log_Message("%s", szString);
}

int DvrAppMainControl::LogLevel()
{
	return m_nLogLevel;
}

void DvrAppMainControl::SetLogLevel(int nLogLevel)
{
	m_nLogLevel = nLogLevel;
}

DvrAppMainControl::DvrAppMainControl()
{
	m_hcMgrHandler = new HcMgrHandler();
	m_guiResHandler = new DvrGuiResHandler();

	memset(&m_status, 0, sizeof(APP_STATUS));
	memset(m_PhotoPlayBackImageBuffer, 0, sizeof(m_PhotoPlayBackImageBuffer));

#ifdef WIN32
	m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadClient, this, 0, &m_dwThreadID);
#endif

	m_curAppMode = DVR_APP_MODE_NONE;

    if (NULL == CThreadPool::Get())
    {
        Log_Error("CThreadPool Create Failed\n");
    }

    Dvr_AppNotify_Init();
}

DvrAppMainControl::~DvrAppMainControl()
{
#ifdef WIN32
    CloseHandle(m_hThread);
    m_hThread = NULL;
#endif

	if (m_hcMgrHandler)
	{
		delete m_hcMgrHandler;
		m_hcMgrHandler = NULL;
	}

	if (m_guiResHandler)
	{
		delete m_guiResHandler;
		m_guiResHandler = NULL;
	}

    CThreadPool *pool;
    if (NULL != (pool = CThreadPool::Get()))
    {
        pool->Destroy();
    }

    Dvr_AppNotify_DeInit();
}

static int PostRspData(uint32_t data_type, uint32_t param[4])
{
#ifdef __linux__
	AvmEvent *evt = AvmRequestEvent(OUTPUT_EVENT_A15_IPU1_MSG);
	RspData_Msg *pMsg = (RspData_Msg *)evt->GetRawEvent()->payload;
	memset(pMsg, 0, sizeof(RspData_Msg));
	
	pMsg->MsgHead.MsgType = AVM_EVENT_MSG_TYPE_A15_IPU1_DVR_RSP;
	pMsg->MsgHead.MsgSize = sizeof(RspData_Msg);
	pMsg->data_type = data_type;
	pMsg->parameter[0] = param[0];
	pMsg->parameter[1] = param[1];
	pMsg->parameter[2] = param[2];
	pMsg->parameter[3] = param[3];
	
	AvmPostEvent(*evt, NULL);
	AvmEventReleaseAndTrace(*evt);	
#endif	

	return 0;
}

void DvrAppMainControl::DvrPlayerOnNewFrame(void *pInstance, void *pUserData)
{
	uint32_t param[4] = {0};
	DVR_IO_FRAME *pFrame = (DVR_IO_FRAME *)pUserData;
	DvrAppMainControl *pThis = (DvrAppMainControl *)pInstance;
	
    if (pFrame->pMattsImage != NULL) 
    {
        //video playback		
		param[0] = (uint32_t)pFrame->pMattsImage;
		param[1] = (uint32_t)pFrame->pCanBuf;
		param[2] = pFrame->nMattsImageWidth;
		param[3] = pFrame->nFramelength;
        PostRspData(RSP_DATA_TYPE_DVR_VIDEO_PLAYBACK_IMGADDR, param);
    }
    else
    {
        //photo playback
		if(pFrame->crop.width != DVR_PHOTO_PLAYBACK_BUFFER_WIDTH)
		{
			DVR_U8 *pSrc_Y = pFrame->pImageBuf[0];
			DVR_U8 *pSrc_UV = pFrame->pImageBuf[0] + pFrame->nSingleViewWidth * pFrame->nSingleViewHeight;
			DVR_U8 *pDst = pThis->m_PhotoPlayBackImageBuffer;

			for(int h = 0; h < pFrame->crop.height; h++)
			{
				memcpy(pDst, pSrc_Y, pFrame->crop.width);
				pSrc_Y += pFrame->crop.width;
				pDst += DVR_PHOTO_PLAYBACK_BUFFER_WIDTH;
			}

			pDst = pThis->m_PhotoPlayBackImageBuffer + DVR_PHOTO_PLAYBACK_BUFFER_WIDTH * DVR_PHOTO_PLAYBACK_BUFFER_HEIGHT;
			for(int h = 0; h < pFrame->crop.height/2; h++)
			{
				memcpy(pDst, pSrc_UV, pFrame->crop.width);
				pSrc_UV += pFrame->crop.width;
				pDst += DVR_PHOTO_PLAYBACK_BUFFER_WIDTH;
			}
		}
		else
		{
			memcpy(pThis->m_PhotoPlayBackImageBuffer, pFrame->pImageBuf[0], sizeof(pThis->m_PhotoPlayBackImageBuffer));
		}

		param[0] = (uint32_t)pThis->m_PhotoPlayBackImageBuffer;
		param[1] = (uint32_t)pFrame->pCanBuf;
		param[2] = pFrame->crop.width;
		param[3] = pFrame->crop.height;		
        PostRspData(RSP_DATA_TYPE_DVR_IMAGE_PLAYBACK_IMGADDR, param);
    }
}

void DvrAppMainControl::DvrAppMessageCallback(void *pContext, int enuType, void *pParam1, void *pParam2)
{
	DvrAppMainControl *pDvrMainCtrl = (DvrAppMainControl *)pContext;
	uint32_t param[4] = {0};

	switch (enuType)
	{
	case DVR_APP_NOTIFICATION_TYPE_APP_SWTICH:
	{
		if (pParam1 == NULL)
			return;
		int appId = *(int *)pParam1;
		
		DVR_U8 CurDvrAppMode;

		if (Dvr_App_GetAppId(MDL_APP_RECORDER_ID) == appId)
		{
			CurDvrAppMode = DVR_APP_MODE_RECORD;
		}
		else if (Dvr_App_GetAppId(MDL_APP_THUMB_ID) == appId)
		{
			CurDvrAppMode = DVR_APP_MODE_THUMB;
		}
		else if (Dvr_App_GetAppId(MDL_APP_PLAYER_ID) == appId)
		{
			CurDvrAppMode = DVR_APP_MODE_PLAYBACK;
		}
        else
        {
            //TODO
        }

		param[0] = CurDvrAppMode;
		PostRspData(RSP_DATA_TYPE_DVR_APP_MODE, param);
	}
	break;

	case DVR_APP_NOTIFICATION_TYPE_MODE_UPDATE:
	{
		if (pParam1 == NULL)
			return;

		DVR_U8 AVM_DVRModeFeedback = *(DVR_U8 *)pParam1;

		param[0] = AVM_DVRModeFeedback;
		PostRspData(RSP_DATA_TYPE_DVR_MODE_STATUS, param);
	}
	break;

	case DVR_APP_NOTIFICATION_TYPE_RECORDER_SWTICH:
	{
		if (pParam1 == NULL)
			return;

		DVR_U8 DVREnableSetStatus = *(DVR_U8 *)pParam1;
        if(DVREnableSetStatus==0x01)
        {
            AVMInterface_setDvrIsRecoding(0);
            Log_Warning("DVREnableSetStatus %d  close rec icon \n",DVREnableSetStatus);  
        }
        else if(DVREnableSetStatus=0x02)
        {
            
            AVMInterface_setDvrIsRecoding(1);
            Log_Warning("DVREnableSetStatus %d  open rec icon \n",DVREnableSetStatus);  
        }
        
		param[0] = DVREnableSetStatus;
		PostRspData(RSP_DATA_TYPE_DVR_ENABLE_SET_STATUS, param);
	}
	break;

	case DVR_APP_NOTIFICATION_TYPE_EVENT_RECORD_STATUS:
	{
		if (pParam1 == NULL)
			return;

		DVR_U32 EmergencyRecordStatus = *(DVR_U32 *)pParam1;
		
		param[0] = EmergencyRecordStatus;
		PostRspData(RSP_DATA_TYPE_DVR_EMERGENCY_RECORD_STATUS, param);
	}
	break;
    case DVR_APP_NOTIFICATION_TYPE_EVENT_RECORD_SWITCH:
	{
		if (pParam1 == NULL)
			return;

		DVR_U32 EmergencyRecordSwitch = *(DVR_U32 *)pParam1;
		
		param[0] = EmergencyRecordSwitch;
		PostRspData(RSP_DATA_TYPE_DVR_EMERGENCY_RECORD_SWITCH, param);
	}
	break;
    

	case DVR_APP_NOTIFICATION_TYPE_CARD_ERROR_STATUS_UPDATE:
	{
		if (pParam1 == NULL)
			return;

		DVR_U32 DVR_SDCardErrorStatus = *(DVR_U32 *)pParam1;

		param[0] = DVR_SDCardErrorStatus;
		PostRspData(RSP_DATA_TYPE_DVR_SDCARD_ERROR_STATUS, param);
	}
	break;

	case DVR_APP_NOTIFICATION_TYPE_CARD_FULL_STATUS_UPDATE:
	{
		if (pParam1 == NULL)
			return;

		DVR_U32 DVR_SDCardFullStatus = *(DVR_U32 *)pParam1;

		param[0] = DVR_SDCardFullStatus;
		PostRspData(RSP_DATA_TYPE_DVR_SDCARD_FULL_STATUS, param);
	}
	break;
	
	case DVR_APP_NOTIFICATION_TYPE_PLAY_TIMER_UPDATE:
	{
		if (pParam1 == NULL)
			return;

		GUI_OBJ_PLAY_TIME_INST *play_time = (GUI_OBJ_PLAY_TIME_INST *)pParam1;

		param[0] = play_time->position;
		param[1] = play_time->duration;
		PostRspData(RSP_DATA_TYPE_DVR_PLAY_TIME, param);
	}
	break;

	case DVR_APP_NOTIFICATION_TYPE_PAGE_ITEM_NUM_UPDATE:
	{
		if (pParam1 == NULL)
			return;

		DVR_U32 PageItemNum = *(DVR_U32 *)pParam1;

		param[0] = PageItemNum;
		PostRspData(RSP_DATA_TYPE_DVR_CUR_VIDEO_COUNTS, param);
	}
	break;

	case DVR_APP_NOTIFICATION_TYPE_DELETE_STATUS:
	{
		if (pParam1 == NULL)
			return;

		DVR_U32 DeleteStatus = *(DVR_U32 *)pParam1;

		param[0] = DeleteStatus;
		PostRspData(RSP_DATA_TYPE_DVR_DELETE_STATUS, param);
	}
	break;

	case DVR_APP_NOTIFICATION_TYPE_REPLAY_MODE:
	{
		if (pParam1 == NULL)
			return;

		DVR_U32 DVRReplayMode = *(DVR_U32 *)pParam1;

		param[0] = DVRReplayMode;
		PostRspData(RSP_DATA_TYPE_DVR_REPLAY_MODE, param);
	}
	break;

	case DVR_APP_NOTIFICATION_TYPE_STORAGE_PERCENT:
	{
		if (pParam1 == NULL)
			return;

		DVR_U32 DVR_StoragePercent = *(DVR_U32 *)pParam1;

		param[0] = DVR_StoragePercent;
		PostRspData(RSP_DATA_TYPE_DVR_STORAGE_PERCENT, param);
	}
	break;

	case DVR_APP_NOTIFICATION_TYPE_PHOTOGRAPH_RESULT:
	{
		if (pParam1 == NULL)
			return;

		DVR_U32 DVR_PhotographResult = *(DVR_U32 *)pParam1;

		param[0] = DVR_PhotographResult;
		PostRspData(RSP_DATA_TYPE_DVR_PHOTOGRAPH_RESULT, param);
	}
	break;

	case DVR_APP_NOTIFICATION_TYPE_NORMAL_TO_EMERGENCY_STATUS:
	{
		if (pParam1 == NULL)
			return;

		DVR_U32 DVR_NormalToEmergency = *(DVR_U32 *)pParam1;

		param[0] = DVR_NormalToEmergency;
		PostRspData(RSP_DATA_TYPE_DVR_NORMAL_TO_EMERGENCY_STATUS, param);
	}
	break;

	case DVR_APP_NOTIFICATION_TYPE_PLAYBACK_STATUS_UPDATE:
	{
		if (pParam1 == NULL)
			return;

		DVR_U32 AVM_DVRReplayCommandStatus = *(DVR_U32 *)pParam1;

		param[0] = AVM_DVRReplayCommandStatus;
		PostRspData(RSP_DATA_TYPE_DVR_PLAYBACK_STATUS, param);
	}
	break;

	case DVR_APP_NOTIFICATION_TYPE_DISPLAY_VISION_UPDATE:
	{
		if (pParam1 == NULL)
			return;

		DVR_U32 DVR_DisplayVison = *(DVR_U32 *)pParam1;

		param[0] = DVR_DisplayVison;
		PostRspData(RSP_DATA_TYPE_DVR_DISPLAY_VISION_STATUS, param);
	}
	break;

	case DVR_APP_NOTIFICATION_TYPE_RECORDING_CYCLE_UPDATE:
	{
		if (pParam1 == NULL)
			return;

		DVR_U32 DVR_RecordingCycleSetStatus = *(DVR_U32 *)pParam1;

		param[0] = DVR_RecordingCycleSetStatus;
		PostRspData(RSP_DATA_TYPE_DVR_RECORDING_CYCLE_SET_STATUS, param);
	}
	break;

	case DVR_APP_NOTIFICATION_TYPE_PRINTSCREEN_RESULT:
	{
		if (pParam1 == NULL)
			return;

		DVR_U32 DVR_PrintScreenFeedback = *(DVR_U32 *)pParam1;

		param[0] = DVR_PrintScreenFeedback;
		PostRspData(RSP_DATA_TYPE_DVR_PRINTSCREEN_RESULT, param);
	}
	break;

	case DVR_APP_NOTIFICATION_TYPE_CARD_FORMAT_STATUS:
	{
		if (pParam1 == NULL)
			return;

		DVR_U32 DVR_FormatStatus = *(DVR_U32 *)pParam1;

		param[0] = DVR_FormatStatus;
		PostRspData(RSP_DATA_TYPE_DVR_CARD_FORMAT_STATUS, param);
	}
	break;

	case DVR_APP_NOTIFICATION_TYPE_FORMAT_CARD_REQUEST:
	{
		if (pParam1 == NULL)
			return;

		DVR_U32 DVR_FormatRequest = *(DVR_U32 *)pParam1;

		param[0] = DVR_FormatRequest;
		PostRspData(RSP_DATA_TYPE_DVR_FORMAT_CARD_REQUEST, param);
	}
	break;

	case DVR_APP_NOTIFICATION_TYPE_FORCE_IDR_REQUEST:
	{
		if (pParam1 == NULL)
			return;

		DVR_U32 DVR_ForceRequest = *(DVR_U32 *)pParam1;

		param[0] = DVR_ForceRequest;
		PostRspData(RSP_DATA_TYPE_DVR_FORCE_IDR_REQUEST, param);
	}
	break;

    case DVR_APP_NOTIFICATION_TYPE_ASYNC_START_RECORDER:
    {
        extern DVR_RESULT rec_app_start_async(void);
        rec_app_start_async();
    }
    break;	

    case DVR_APP_NOTIFICATION_TYPE_ASYNC_STOP_RECORDER:
    {
        extern DVR_RESULT rec_app_stop_async(void);
        rec_app_stop_async();
    }
    break;

	case DVR_APP_NOTIFICATION_TYPE_RECORDER_START_DONE:
	{
		PostRspData(RSP_DATA_TYPE_DVR_RECORDER_START_DONE, param);
	}
	break;

	case DVR_APP_NOTIFICATION_TYPE_PLAYER_OPEN_RESULT:
	{
		if (pParam1 == NULL)
			return;

		DVR_RESULT result = *(DVR_RESULT *)pParam1;

		param[0] = result;
		PostRspData(RSP_DATA_TYPE_DVR_PLAYER_OPEN_RESULT, param);
	}
	break;
	
	default:
		break;
	}
	return;
}

DvrAppMainControl *DvrAppMainControl::Get()
{
	if (!m_pDvrMainCtrl)
	{
		m_pDvrMainCtrl = new DvrAppMainControl();
	}
	return m_pDvrMainCtrl;
}

void DvrAppMainControl::Destory()
{
	if(m_pDvrMainCtrl)
	{
		delete m_pDvrMainCtrl;
		m_pDvrMainCtrl = NULL;
	}
}

static void playback(void *arg)
{
    sleep(3);
    DvrAppUtil_SwitchApp(Dvr_App_GetAppId(MDL_APP_PLAYER_ID));
}

int Dvr_App_Initialize(void)
{
	DVR_RESULT res = Dvr_Sdk_Initialize();
	if (DVR_FAILED(res))
	{
		return res;
	}
    DvrAppMainControl *pDvrMainCtrl = DvrAppMainControl::Get();
    if (pDvrMainCtrl == NULL)
        return DVR_RES_EFAIL;

    Dvr_Sdk_RegisterConsole(DvrAppMainControl::Console, pDvrMainCtrl);
    Dvr_Sdk_RegisterGSTConsole(DvrAppMainControl::Console, pDvrMainCtrl);
    
	res = Dvr_Sdk_MediaMgr_Open();
    Dvr_Sdk_RegisterNotify(SdkNotify, pDvrMainCtrl);
	if (DVR_SUCCEEDED(res))
	{

		DVR_DEVICE_ARRAY Devices;
		Dvr_Sdk_EnumDevices(DVR_DEVICE_TYPE_USBHD, &Devices);

		if (Devices.nDeviceNum == 0)
		{
			Log_Warning("***WARNING***  No Removable Storage Device Found!!!\n");
		}

		for (int i = 0; i < Devices.nDeviceNum; i++)
		{
			DVR_BOOL bBlock = 0, bCreate = 0;
			int nPriority = 0;
			
			Log_Message("Select Driver [%s], Start to Scan!!!", Devices.pDriveArray[i]->szMountPoint);
			Dvr_Sdk_SetActiveDrive(Devices.pDriveArray[i]);
			
			res = Dvr_Sdk_DB_Mount(Devices.pDriveArray[i]->szMountPoint, Devices.pDriveArray[i]->szDevicePath, i);
			if(res != DVR_RES_SOK)
			{
				Log_Error("Dvr_Sdk_DB_Mount Driver[%s] Fail!!!", Devices.pDriveArray[i]->szMountPoint);
			}
			
#if 1
			Log_Message("Filesystem scanning for %s has completed, start to create filemap DB\n", Devices.pDriveArray[i]->szMountPoint);
			
			res = Dvr_Sdk_FileMapDB_ScanLibrary(Devices.pDriveArray[i]->szMountPoint);
			if(res != DVR_RES_SOK)
			{
				Log_Error("Dvr_Sdk_FileMapDB_ScanLibrary failed\n");
			}
			else
			{
				Log_Message("Dvr_Sdk_FileMapDB_ScanLibrary done!\n");
				InitDvrStorageQuota();
				//Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_THUMB_UPDATE_FRAME, TRUE, 0);
				Dvr_Sdk_ComSvcHcMgr_SndMsg(AMSG_STATE_CARD_INSERT_ACTIVE, 0, 0);
			}
#endif			
			Log_Message("Scan Driver [%s] Complete!!!", Devices.pDriveArray[i]->szMountPoint);
			Dvr_Sdk_Free(Devices.pDriveArray[i]);
		}
	}
	
    APP_STATUS *pStatus = (APP_STATUS *)Dvr_App_GetStatus();
    pStatus->selFileType = DVR_FILE_TYPE_VIDEO;
    pStatus->selFolderType = DVR_FOLDER_TYPE_NORMAL;
    //strcpy(pStatus->selFileName,"/media/data/NOR/NOR_20181113_083204_M_00704.MP4");///media/data/NOR/NOR_20181115_091039_M_00774.MP4
    strcpy(pStatus->selFileName,"/media/data/NOR/NOR_20181121_092648_M_00798.MP4");


    HcMgrHandler *pHcMgrHandler = pDvrMainCtrl->GetHcMgrHandler();
    if (pHcMgrHandler != NULL)
    {
        Dvr_Sdk_ComSvcHcMgr_AttachHandler(pHcMgrHandler->GetHandler(), pHcMgrHandler);
    }
    //CThreadPool::Get()->AddTask(playback, NULL);

    Dvr_Sdk_Player_RegisterCallBack(DvrAppMainControl::DvrPlayerOnNewFrame, pDvrMainCtrl);
    DvrAppNotify::Get()->RegisterNotify(pDvrMainCtrl, DvrAppMainControl::DvrAppMessageCallback);	

#ifdef __ONLINE_DEBUG_ENABLE__
	{
		extern void Debug_Online_Server_Start(void);
		extern void dbgOnline_RegistDvrCmd(void *handle);
		Debug_Online_Server_Start();
        dbgOnline_RegistDvrCmd(pDvrMainCtrl);
	}
#endif

	return res;
}

int Dvr_App_DeInitialize(void)
{
	DVR_RESULT res = DVR_RES_SOK;

	DvrAppUtil_SwitchApp(Dvr_App_GetAppId(MDL_APP_MAIN_ID));
 
	res = Dvr_Sdk_UnRegisterNotify(SdkNotify, DvrAppMainControl::Get());
 	res |= Dvr_Sdk_MediaMgr_Close();
	res |= Dvr_Sdk_RegisterConsole(DvrAppMainControl::Console, DvrAppMainControl::Get());
	res |= Dvr_Sdk_DeInitialize();

	DvrAppMainControl::Destory();

	return res;
}

int Dvr_App_GetAppId(int module)
{
    if (DvrAppMainControl::m_pDvrMainCtrl == NULL)
        return DVR_RES_EFAIL;

    return DvrAppMainControl::m_pDvrMainCtrl->GetHcMgrHandler()->GetAppId(module);
}

const char *Dvr_App_GetAppName(int appId)
{
    if (DvrAppMainControl::m_pDvrMainCtrl == NULL)
        return NULL;

    return DvrAppMainControl::m_pDvrMainCtrl->GetHcMgrHandler()->GetAppName(appId);
}

void* Dvr_App_GetStatus(void)
{
    if (DvrAppMainControl::m_pDvrMainCtrl == NULL)
        return NULL;

    return DvrAppMainControl::m_pDvrMainCtrl->GetAppStatus();
}

void *Dvr_App_GetGuiResHandler(void)
{
    if (DvrAppMainControl::m_pDvrMainCtrl == NULL)
        return NULL;

    return DvrAppMainControl::m_pDvrMainCtrl->GetGuiResHandler();
}

int Dvr_App_Get_GuiLayOut(DVR_GUI_LAYOUT_INST *pInst)
{
    if (DvrAppMainControl::m_pDvrMainCtrl == NULL)
        return DVR_RES_EFAIL;

    return DvrAppMainControl::m_pDvrMainCtrl->GetGuiResHandler()->GetLayout(pInst);
}

int Dvr_App_GetCurMode(void)
{
	return DvrAppMainControl::GetCurMode();
}

int Dvr_App_HuSendCmd(unsigned int msg, unsigned int param1, unsigned int param2)
{
    if(msg == HMSG_USER_CMD_RECORDER_IACC_RECORD ||
       msg == HMSG_USER_CMD_RECORDER_ALARM_RECORD_START)
    {
        struct timespec rt;
        clock_gettime(CLOCK_MONOTONIC, &rt);
        unsigned long curtime = rt.tv_sec * 1000 + rt.tv_nsec/1000000;
        //printf("Dvr_App_HuSendCmd === curtime %ld m_lasttime %ld   diff %ld\n",curtime,m_lasttime,curtime - m_lasttime);
        if(curtime > m_lasttime && curtime - m_lasttime < 333)
        {
            //Log_Error("Dvr_App_HuSendCmd [msg 0x%X] too frequently!!!!!!!!!!!\n",msg);
            return DVR_RES_SOK;
        }
        m_lasttime = curtime;
    }

    return Dvr_Sdk_ComSvcHcMgr_SndMsg(msg, param1, param2);
}

int Dvr_App_SetStartApp(int eDvrMode)
{
	return DvrAppUtil_SetStartApp(eDvrMode);
}

int Dvr_App_SetRecorderSpitTime(int eSplitTime)
{
	return DvrAppUtil_SetRecorderSplitTime(eSplitTime);
}

int Dvr_App_SetLanguageSelected(int eLangSel)
{
	return DvrAppUtil_SetLanguageSel(eLangSel);
}

int Dvr_App_UpdateSystemTime(unsigned int year, 
							unsigned int month, 
							unsigned int day, 
							unsigned int hour, 
							unsigned int minute,
							unsigned int second)
{
	return Dvr_Sdk_UpdateSystemTime(year, month, day, hour, minute, second);
}
