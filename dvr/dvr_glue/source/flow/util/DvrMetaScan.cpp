/*******************************************************************************
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
\*********************************************************************************/
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include "flow/util/DvrMetaScan.h"
#include "log/log.h"

static DvrMetaScan *g_hMetaData = NULL;
int DvrMetaScan::m_photoGroupId = -1;

DvrMetaScan::DvrMetaScan()
{
    int status = DVR_RES_SOK;

    status = pthread_create(&pthreadDispName, NULL, ThreadGetDispName, this);
    if (status != 0) {
        Log_Error("pthread_create() - Could not create thread [%d]\n", status);
    }

    status = pthread_create(&pthreadPreview, NULL, ThreadGetPreview, this);
    if (status != 0) {
        Log_Error("pthread_create() - Could not create thread [%d]\n", status);
    }

    pthread_mutex_init(&m_DispQueueLock, NULL);
    pthread_mutex_init(&m_PreviewQueueLock, NULL);
    pthread_mutex_init(&m_ScanLock, NULL);

    m_DispNameQueue.clear();
    m_PreviewQueue.clear();
}

DvrMetaScan::~DvrMetaScan()
{
    SendExit();
    pthread_join(pthreadDispName, NULL);
    pthread_join(pthreadPreview, NULL);

    if (m_DispNameQueue.size() > 0)
    {
		while(m_DispNameQueue.size() > 0)
        {
            MetaDispNameScanParam stParam;
            stParam = m_DispNameQueue.back();
            m_DispNameQueue.pop_back();
        }
    }

    m_PreviewQueue.clear();

    pthread_mutex_destroy(&m_DispQueueLock);
    pthread_mutex_destroy(&m_PreviewQueueLock);
    pthread_mutex_destroy(&m_ScanLock);
}

void DvrMetaScan::ClearQueue()
{
    Log_Message("metadata clear queue start\n");
    pthread_mutex_lock(&m_DispQueueLock);
    if (m_DispNameQueue.size() > 0)
    {
	    Log_Message("need to clear display name queue: %d\n", m_DispNameQueue.size());

		while(m_DispNameQueue.size() > 0)
        {
            MetaDispNameScanParam stParam;
            stParam = m_DispNameQueue.back();
            m_DispNameQueue.pop_back();

            Log_Message("clear display name queue for %s done\n", stParam.szFileName);
        }
    }
    pthread_mutex_unlock(&m_DispQueueLock);

    pthread_mutex_lock(&m_PreviewQueueLock);
    m_PreviewQueue.clear();
    pthread_mutex_unlock(&m_PreviewQueueLock);
    Log_Message("metadata clear queue done\n");
}

void* DvrMetaScan::ThreadGetPreview(void* pParam)
{
    int ret;
    DvrMetaScan *pThis;
    pThis = reinterpret_cast<DvrMetaScan *>(pParam);

    MetaPreviewScanParam stParam;
    memset(&stParam, 0, sizeof(stParam));

    for (;;)
    {
        while (0 == pThis->m_PreviewQueue.size())
        {
#ifdef WIN32
            Sleep(5);
#else
            usleep(5000);
#endif
        }

        pthread_mutex_lock(&pThis->m_PreviewQueueLock);
        stParam = pThis->m_PreviewQueue.back();
        pThis->m_PreviewQueue.pop_back();
        pthread_mutex_unlock(&pThis->m_PreviewQueueLock);

        if (stParam.Cmd == METASCANCMD_EXIT)
            return 0;

        switch (stParam.Cmd)
        {
        default:
            break;
        case METASCANCMD_EXIT:
            return 0;
        case METASCANCMD_ADDFILE:
            ret = pThis->AddFileForPreview(&stParam);
            break;
        }
    }
    return 0;
}

void* DvrMetaScan::ThreadGetDispName(void* pParam)
{
    int ret;
    DvrMetaScan *pThis;
    pThis = reinterpret_cast<DvrMetaScan *>(pParam);

    MetaDispNameScanParam stParam;
    memset(&stParam, 0, sizeof(stParam));

    for (;;)
    {
        while (0 == pThis->m_DispNameQueue.size())
        {
#ifdef WIN32
            Sleep(5);
#else
            usleep(5000);
#endif
        }

        pthread_mutex_lock(&pThis->m_DispQueueLock);
        stParam = pThis->m_DispNameQueue.back();
        pThis->m_DispNameQueue.pop_back();
        pthread_mutex_unlock(&pThis->m_DispQueueLock);
        
        if (stParam.Cmd == METASCANCMD_EXIT)
            return 0;

        switch (stParam.Cmd)
        {
        default:
            break;
        case METASCANCMD_EXIT:
            return 0;
        case METASCANCMD_ADDFILE:
            ret = pThis->AddFileForDispName(&stParam);
            break;
        }
    }
    return 0;
}

int DvrMetaScan::AddFileForPreview(MetaPreviewScanParam *pParam)
{
    DVR_RESULT res = DVR_RES_SOK;

    pthread_mutex_lock(&m_ScanLock);

    res = Dvr_Sdk_MetaData_GetFilePreview(&pParam->option, pParam->pPreviewBuf, &pParam->nPreviewBufSize);
    if (res != DVR_RES_SOK)
    {
        Log_Error("Dvr_Sdk_MetaData_GetFilePreview fail for file [%s]\n", pParam->option.filename);
    }
    else
    {
        Log_Message("GetFilePreview done successfully for file [%s]\n", pParam->option.filename);
    }

    pthread_mutex_unlock(&m_ScanLock);

    return res;
}

int DvrMetaScan::AddFileForDispName(MetaDispNameScanParam *pParam)
{
    DVR_U32 i = 0;
    DVR_RESULT res = DVR_RES_SOK;
    void *hMetaData = NULL;
    DVR_U32 u32MDItemNum;
	DVR_DEVICE CurDrive;
    DVR_METADATA_ITEM stMetaItem;
    DVR_MEDIA_INFO stMediaInfo;
    Dvr_Sdk_GetActiveDrive(&CurDrive);
	if (!strcmp(CurDrive.szMountPoint, ""))
		return DVR_RES_EFAIL;

    switch (pParam->eMediaType)
    {
    case DVR_FILE_TYPE_VIDEO:
    {
        char displayName[APP_MAX_FN_SIZE];
        DVR_DB_IDXFILE idxfile;
        if (DVR_RES_SOK == Dvr_Sdk_FileMapDB_GetFileInfo(CurDrive.szMountPoint, pParam->szFileName, (DVR_FOLDER_TYPE)pParam->eFolderType, &idxfile))
        {
            //has already scan and store in memory DB
            return DVR_RES_SOK;
        }

        pthread_mutex_lock(&m_ScanLock);

        Dvr_Sdk_MetaData_Create(&hMetaData, (void *)pParam->szFileName, &u32MDItemNum);
        if (hMetaData == NULL)
        {
            Log_Error("[%s]No Meta Data for %s!\n", __FUNCTION__, pParam->szFileName);
            res = DVR_RES_EFAIL;
        }
        else
        {
            //DVR_VIDEO_META_ITEM stDBItem;
            //memset(&stDBItem, 0, sizeof(DVR_VIDEO_META_ITEM));

            //strcpy(stDBItem.szFileName, pParam->szFileName);
            //stDBItem.folderType = pParam->eFolderType;

            if (DVR_RES_SOK == (res = Dvr_Sdk_MetaData_GetDataByType(hMetaData, &stMetaItem, DVR_METADATA_TYPE_DESCRIPTION)))
            {
                if (DVR_RES_SOK == Dvr_Sdk_MetaData_GetMediaInfo(hMetaData, &stMediaInfo))
                {
                    char displayName[64] = { 0 };
                    if (DVR_RES_SOK == Dvr_Sdk_FormatDisplayName(stMetaItem.ps8Data, displayName, stMediaInfo.u32Duration))
                    {
                        //strcpy(stDBItem.displayName, displayName);
                    }
                    else
                    {
                        //memcpy(stDBItem.displayName, stMetaItem.ps8Data, stMetaItem.u32DataSize);
                    }
                }
            }

            //Dvr_Sdk_FileMapDB_AddItem(&stDBItem, pParam->eFolderType);
        }

        Dvr_Sdk_MetaData_Destroy(hMetaData);

        pthread_mutex_unlock(&m_ScanLock);
    }
    break;

    case DVR_FILE_TYPE_IMAGE:
    {
        DVR_DB_IDXFILE idxfile;
        if (DVR_RES_SOK == Dvr_Sdk_FileMapDB_GetFileInfo(CurDrive.szMountPoint, pParam->szFileName, (DVR_FOLDER_TYPE)pParam->eFolderType, &idxfile))
        {
            //has already scan and store in memory DB
            return DVR_RES_SOK;
        }

        pthread_mutex_lock(&m_ScanLock);

        Dvr_Sdk_MetaData_Create(&hMetaData, (void *)pParam->szFileName, &u32MDItemNum);
        if (hMetaData == NULL)
        {
            Log_Error("[%s]No Meta Data for %s!\n", __FUNCTION__, pParam->szFileName);
            res = DVR_RES_EFAIL;
        }
        else
        {
            char display_name[APP_MAX_FN_SIZE];
            int groupId = 0, viewIndex = 0;

            memset(display_name, 0, sizeof(display_name));

            if (DVR_RES_SOK == Dvr_Sdk_MetaData_GetDataByType(hMetaData, &stMetaItem, DVR_METADATA_TYPE_DESCRIPTION))
            {
                memcpy(display_name, stMetaItem.ps8Data, stMetaItem.u32DataSize);
            }

            if (DVR_RES_SOK == Dvr_Sdk_MetaData_GetDataByType(hMetaData, &stMetaItem, DVR_METADATA_TYPE_ARTIST))
            {
                groupId = atoi(stMetaItem.ps8Data);
            }

            if (DVR_RES_SOK == Dvr_Sdk_MetaData_GetDataByType(hMetaData, &stMetaItem, DVR_METADATA_TYPE_APPLICATION))
            {
                if (!strcmp(stMetaItem.ps8Data, "FRONT"))
                {
                    viewIndex = DVR_VIEW_INDEX_FRONT;
                }
                else if (!strcmp(stMetaItem.ps8Data, "REAR"))
                {
                    viewIndex = DVR_VIEW_INDEX_REAR;
                }
                else if (!strcmp(stMetaItem.ps8Data, "LEFT"))
                {
                    viewIndex = DVR_VIEW_INDEX_LEFT;
                }
                else if (!strcmp(stMetaItem.ps8Data, "RIGHT"))
                {
                    viewIndex = DVR_VIEW_INDEX_RIGHT;
                }
                else
                {
                    //should not come here
                }
            }

            if (groupId != m_photoGroupId)
            {
                memset(m_PhotoDispName, 0, sizeof(m_PhotoDispName));
                strcpy(m_PhotoDispName, display_name);
                m_photoGroupId = groupId;
                //Dvr_Sdk_PhotoDB_AddItem(display_name);
            }
        }

        Dvr_Sdk_MetaData_Destroy(hMetaData);

        pthread_mutex_unlock(&m_ScanLock);
        break;
    }

    default:
        break;
    }


    return res;
}

int DvrMetaScan::SendExit()
{
    MetaDispNameScanParam stDispParam;

    memset(&stDispParam, 0, sizeof(stDispParam));
    stDispParam.Cmd = METASCANCMD_EXIT;

    pthread_mutex_lock(&m_DispQueueLock);
    m_DispNameQueue.push_front(stDispParam);
    pthread_mutex_unlock(&m_DispQueueLock);

    MetaPreviewScanParam stPreviewParam;
    memset(&stPreviewParam, 0, sizeof(stPreviewParam));
    stPreviewParam.Cmd = METASCANCMD_EXIT;

    pthread_mutex_lock(&m_PreviewQueueLock);
    m_PreviewQueue.push_front(stPreviewParam);
    pthread_mutex_unlock(&m_PreviewQueueLock);

    return DVR_RES_SOK;
}

DvrMetaScan *DvrMetaScan::Get(void)
{
    return g_hMetaData;
}

int Dvr_MetaScan_Init(void)
{
    if (g_hMetaData != NULL)
        return DVR_RES_EUNEXPECTED;

    g_hMetaData = new DvrMetaScan();
    if (g_hMetaData == NULL)
        return DVR_RES_EOUTOFMEMORY;

    return DVR_RES_SOK;
}

int Dvr_MetaScan_DeInit(void)
{
    if (g_hMetaData == NULL)
        return DVR_RES_EUNEXPECTED;

    delete g_hMetaData;
    g_hMetaData = NULL;

    return DVR_RES_SOK;
}
