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
#include "DvrSdkTest_MediaMgrOpen.h"
#include "DvrSdkTestUtils.h"

int DvrSdkTest_MediaMgrOpen::Test()
{
//! [Open Example]
    DVR_RESULT res;

    res = Dvr_MediaMgr_Open(m_hDvr);

    if (DVR_SUCCEEDED(res)) {
		m_scanParam.eSortMode = DVR_SORT_MODE_FILENAME_LH;
		m_scanParam.eSortModifier = DVR_SORT_MODIFIER_TAIL;
		m_scanParam.eRecursiveMode = DVR_RECURSIVE_MODE_DFS;
        m_scanParam.szSortConfig                  = "Lzh-Hans-CN_DO";
        m_scanParam.bSuppressEmptyDirectories     = 1;
        m_scanParam.bSuppressSingleDirectoryChain = 1;
		m_scanParam.uRejectPartialSaveDB = DVR_FIRSTSCAN_STATUS_STOP_USERREQUEST | DVR_FIRSTSCAN_STATUS_STOP_TIMEOUT |
			DVR_FIRSTSCAN_STATUS_GENERALERROR | DVR_FIRSTSCAN_STATUS_ACCESSDENIED |
			DVR_FIRSTSCAN_STATUS_PATHNOTFOUND | DVR_FIRSTSCAN_STATUS_GENERALREADERROR |
			DVR_FIRSTSCAN_STATUS_DEVICEUNPLUGGED;
		m_scanParam.uUseAndRescanPartialSave = DVR_FIRSTSCAN_STATUS_PARTIAL;
        m_scanParam.uDFSFirstCompleteDirectories  = 0xFFFFFFFF;
        m_scanParam.uBFSFirstDirectories          = 20;
        m_scanParam.uBFSFirstDepth                = 3;
        m_scanParam.bGeneratePartial              = 0;
        m_scanParam.bCheckCache                   = 1;
        m_scanParam.bSaveCache                    = 1;
        m_scanParam.nSkipDepth                    = 0;
        m_scanParam.m_eElideMode                  = DVR_ELIDE_MODE_FILE;
        m_scanParam.nElideDepth                   = 8;
        m_scanParam.nBypassCheckTime              = 10000;
        m_scanParam.nMaxTime                      = 0;
        m_scanParam.nMaxFileCount                 = 0;
        memset(m_scanParam.nMaxCount, 0, sizeof(m_scanParam.nMaxCount));
		m_scanParam.nExtArray[DVR_FILE_TYPE_AUDIO] = DVR_ARRAYSIZE(DVR_AUDIO_EXT);
		m_scanParam.szExtArray[DVR_FILE_TYPE_AUDIO] = DVR_AUDIO_EXT;
		m_scanParam.nExtArray[DVR_FILE_TYPE_VIDEO] = DVR_ARRAYSIZE(DVR_VIDEO_EXT);
		m_scanParam.szExtArray[DVR_FILE_TYPE_VIDEO] = DVR_VIDEO_EXT;
		m_scanParam.nExtArray[DVR_FILE_TYPE_IMAGE] = DVR_ARRAYSIZE(DVR_IMAGE_EXT);
		m_scanParam.szExtArray[DVR_FILE_TYPE_IMAGE] = DVR_IMAGE_EXT;
        m_scanParam.nPartialFileCount = 20;
        m_scanParam.nPartialDirCount  = 0;
        m_scanParam.nPartialMsec      = 0;

        DVR_DEVICE_ARRAY Devices;
        Dvr_EnumDevices(m_hDvr, DVR_DEVICE_TYPE_ALL, &Devices);

        for (int i = 0; i < Devices.nDeviceNum; i ++)
        {
            DVR_BOOL bBlock = 1, bCreate = 0;
            int nPriority = 0;
			if (Devices.pDriveArray[i]->eType == DVR_DEVICE_TYPE_USBHD ||
				Devices.pDriveArray[i]->eType == DVR_DEVICE_TYPE_SD) {
                //![Dvr_DB_Mount Example]
#if 1
				Dvr_DB_Mount(m_hDvr, Devices.pDriveArray[i]->szDrivePath, Devices.pDriveArray[i]->szDrivePath,
                        &m_scanParam, bBlock, nPriority, &bCreate);
#else
				Dvr_DB_Mount(m_hDvr, Devices.pDriveArray[i]->szDrivePath, "./VideoRecord",
					&m_scanParam, bBlock, nPriority, &bCreate);
#endif
                //![Dvr_DB_Mount Example]
            }
            //![Dvr_Free Example]
			Dvr_Free(m_hDvr, Devices.pDriveArray[i]);
            //![Dvr_Free Example]
        }

        //! [RegisterNotify Example]
		res = Dvr_RegisterNotify(m_hDvr, Notify, this);
        //! [RegisterNotify Example]
        DvrSdkTestUtils::SetNotifyContext(this);
    }

    return res;
//! [Open Example]
}

int DvrSdkTest_MediaMgrOpen::Notify(void *pContext, DVR_NOTIFICATION_TYPE enuType, void *pParam1, void *pParam2)
{
	DvrSdkTest_MediaMgrOpen *pThis = (DvrSdkTest_MediaMgrOpen*)pContext;

    if (pThis == 0)
        return -1;

    return pThis->NotifyImp(enuType, pParam1, pParam2);
}

int DvrSdkTest_MediaMgrOpen::NotifyImp(DVR_NOTIFICATION_TYPE enuType, void *pParam1, void *pParam2)
{
    switch(enuType)
    {
    case DVR_NOTIFICATION_TYPE_PLAYERROR:
        {
            DVR_RESULT res = *(DVR_RESULT *)pParam1;
            switch (res) {
                case DVR_RES_EDEMUX:
                    printf("File playback failed because demux error\n");
                    break;
				case DVR_RES_EAUDIO_DECODER:
                    printf("File playback failed because audio decoder error\n");
                    break;
				case DVR_RES_EAUDIO_RENDER:
                    printf("File playback failed because audio render error\n");
                    break;
				case DVR_RES_EVIDEO_DECODER:
                    printf("File playback failed because video decoder error\n");
                    break;
				case DVR_RES_EVIDEO_RENDER:
                    printf("File playback failed because video render error\n");
                    break;
            }
        }
        break;
    case DVR_NOTIFICATION_TYPE_PLAYERSTATE:
        {
            //printf("Player State for DVR\n");
            DVR_PLAYER_STATE enuState = *(DVR_PLAYER_STATE *)pParam1;
            if (enuState == DVR_PLAYER_STATE_UNKNOWN) {
                printf("Player State Notified: Unkown State\n");
            }
            if (enuState == DVR_PLAYER_STATE_CLOSE) {
                printf("Player State Notified: Close State\n");
            }
            if (enuState == DVR_PLAYER_STATE_INVALID) {
                printf("Player State Notified: Invalid State\n");
            }
            if (enuState == DVR_PLAYER_STATE_PLAY) {
                printf("Player State Notified: Play State\n");
            }
            if (enuState ==DVR_PLAYER_STATE_STOP) {
                printf("Player State Notified: Stop State\n");
            }
            if (enuState == DVR_PLAYER_STATE_PAUSE) {
                printf("Player State Notified: Pause State\n");
            }
            if (enuState == DVR_PLAYER_STATE_FASTFORWARD) {
                printf("Player State Notified: Fast Forward\n");
            }
            if (enuState == DVR_PLAYER_STATE_FASTBACKWARD) {
                printf("Player State Notified: Fast Backward\n");
            }
        }
        break;
    case DVR_NOTIFICATION_TYPE_DEVICEEVENT:
        {
            DVR_DEVICE_EVENT enuEvent = *(DVR_DEVICE_EVENT *)pParam1;
            DVR_DEVICE       device   = *(DVR_DEVICE *)pParam2;

            const char *DEVTYPE[] = {
                "UNKNOWN",
                "SD",
                "USBHD",
                "USBHUB",
            };

            if (enuEvent == DVR_DEVICE_EVENT_PLUGIN)
            {
                printf("Device Plug-in: %s %s %s\n", DEVTYPE[device.eType], device.szDrivePath, device.szDevice);

				if (device.eType == DVR_DEVICE_TYPE_USBHD ||
					device.eType == DVR_DEVICE_TYPE_SD) {
                    DVR_BOOL bBlock = 1, bCreate = 0;
                    int nPriority = 0;
                    Dvr_DB_Mount(m_hDvr, device.szDrivePath, device.szDrivePath, &m_scanParam, bBlock, nPriority, &bCreate);
					Dvr_SetCurrentDrive(m_hDvr, device.szDrivePath);
					Dvr_Recorder_Start(m_hDvr);
                }
            }
            if (enuEvent == DVR_DEVICE_EVENT_PLUGOUT)
            {
                printf("Device Plug-out: %s %s %s\n", DEVTYPE[device.eType], device.szDrivePath, device.szDevice);

				if (device.eType == DVR_DEVICE_TYPE_USBHD ||
					device.eType == DVR_DEVICE_TYPE_SD) {

					Dvr_Recorder_Stop(m_hDvr);

                    //![Dvr_DB_Unmount Example]
					Dvr_DB_Unmount(m_hDvr, device.szDrivePath);
                    //![Dvr_DB_Unmount Example]
                }
            }
        }
        break;
    case DVR_NOTIFICATION_TYPE_POSITION:
        {
            if (DvrSdkTestUtils::ShowPosition())
            {
                unsigned int uPos = *(unsigned int *)pParam1;
                unsigned int uDuration = *(unsigned int *)pParam2;
                printf("Session Pos = %u s/%u s\n", uPos, uDuration);
            }
        }
        break;
    case DVR_NOTIFICATION_TYPE_RANGEEVENT:
        {
            DVR_RANGE_EVENT enuEvent = *(DVR_RANGE_EVENT *)pParam1;

            if (enuEvent == DVR_RANGE_EVENT_EOF)
            {
                printf("DVR_RANGE_EVENT_EOF\n");
            }
        }
        break;

    case DVR_NOTIFICATION_TYPE_SCANSTATE:
        {
            DVR_SCAN_STATE state = *(DVR_SCAN_STATE *)pParam1;
            const char *szDrive    = (const char *)pParam2;

            if (state == DVR_SCAN_STATE_COMPLETE) {
                printf("Filesystem scanning for %s has completed\n", szDrive);
#if 0
                CIAVN_MOUNT_STATE status;
				CIAVN_RESULT res = CIAVN_DB_GetScanState(m_hDvr, szDrive, &status, sizeof(status));
                if (CIAVN_SUCCEEDED(res)) {
                    if (status.uFirstScanStatus == CIAVN_FIRSTSCAN_STATUS_SUCCESS) {
                        printf("Filesystem scanning for %s is successful!\n", szDrive);
                        std::set<std::string>::iterator it;
                        for (it = m_partialSet.begin(); it != m_partialSet.end(); it++) {
                            if (strncmp((*it).c_str(), szDrive, strlen(szDrive)) == 0) {
                                m_partialSet.erase(it);
                                break;
                            }
                        }
                    } else {
                        printf("Filesystem scanning for %s is unsuccessful\n", szDrive);
                    }
                }
#endif
            } else if (state == DVR_SCAN_STATE_PARTIAL) {
                printf("Filesystem scanning partial completed szDrive = %s\n", szDrive);
#if 0
                const char *szPartialType = szDrive + strlen(szDrive) - 3;

                if (strcmp(szPartialType, "<0>") == 0) {
                    m_partialSet.insert(szDrive);

                    char *url = (char *)malloc(PATH_MAX);
					CIAVN_RESULT res = CIAVN_DB_GetMetaTableTextByName(m_hDvr, szDrive, "_Env", "Restore", "Value", url, PATH_MAX);
                    free(url);
                    if (CIAVN_FAILED(res)) {
						CIAVN_OpenSession(m_hDvr, "usb", CIAVN_ADSP_SOURCEID_PRIMARY);

                        CIAVN_LOOP_STATE  state  = {0};
                        CIAVN_LOOP_ACTION action = {0};

                        state.nLoopMax = 1;
                        state.eNextfileMode      = CIAVN_NEXTFILE_MODE_SEQUENTIAL;
                        state.eRecursive         = CIAVN_LOOP_RECURSIVE_ALLFILE;
                        state.nNeverRepeatLength = 4;
                        state.nCount             = 0;

                        state.eSource = CIAVN_LOOP_SOURCE_FILE;
                        CIAVN_FILE_INFO fileInfo = {0};
						CIAVN_RESULT res = CIAVN_DB_GetFileInfo(m_hDvr, szDrive, CIAVN_FILE_TYPE_AUDIO, 13, &fileInfo);
                        if (CIAVN_FAILED(res)) {
                            printf("FAILED to retrieve the first file from %s\n", szDrive);
                            break;
                        }

                        strcpy(state.szSource, fileInfo.szName);

                        action.bEnable                = 1;
                        action.nPrevToReplayMsec      = 7000;
                        action.eRWItemBeginAction     = CIAVN_LOOP_SCANACTION_CONTINUE;
                        action.eRWLoopBeginAction     = CIAVN_LOOP_SCANACTION_CONTINUE;
                        action.eRWSequenceBeginAction = CIAVN_LOOP_SCANACTION_FORWARDPLAY;
                        action.eFWItemEndAction       = CIAVN_LOOP_SCANACTION_CONTINUE;
                        action.eFWLoopEndAction       = CIAVN_LOOP_SCANACTION_CONTINUE;
                        action.nScanMsec              = 0;

						res = CIAVN_Loop_Setup(m_hDvr, "usb", szDrive, &action, &state);
                        if (CIAVN_FAILED(res)) {
                            printf("Failed to play the first file %s of %s, res = 0x%08x\n", fileInfo.szName, szDrive, res);
                        }
                    }
                }
#endif
            }
        }
        break;
    default:
        break;
    }

    return 0;
}

