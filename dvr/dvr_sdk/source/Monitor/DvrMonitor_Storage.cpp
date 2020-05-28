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
#include "DvrMonitor_Storage.h"

DvrMonitorStorage::DvrMonitorStorage(void *sysCtrl)
{
    m_bEnableLoopRecDetect = 0;
    m_bEnableLoopRecMsg = 0;
    m_LoopRecMsgFlag = 0;
    m_CardAvailableSpace = 0;
    m_LoopRecStorageQuota = 0;

    m_bEnableDasRecDetect = 0;
    m_DasRecMsgFlag = 0;

    m_bExit = 0;
    memset(m_bEnableDasRecMsg, 0, (DVR_DAS_TYPE_NUM - DVR_DAS_TYPE_APA) * sizeof(DVR_BOOL));
    m_LoopRecStorageMonSize = LOOPREC_STORAGE_MONITOR_SIZE_LIMITATION;
    m_DasRecStorageMonSize = DASREC_STORAGE_MONITOR_SIZE_LIMITATION;
    m_sysCtrl = (DvrSystemControl *)sysCtrl;

    DVR::OSA_tskCreate(&m_hThread, Monitor_StorageTaskEntry, this);
}

DvrMonitorStorage::~DvrMonitorStorage()
{
	m_bExit = 1;
	DVR::OSA_tskDelete(&m_hThread);
}

void DvrMonitorStorage::Monitor_StorageTaskEntry(void *ptr)
{
    DvrMonitorStorage* p = (DvrMonitorStorage*)ptr;
    DVR_U32 nCardTotalSpace = 0;
    DVR_U32 nCardUsedSpace = 0;
    DVR_U32 freeSpaceSize = 0;
    DVR_U32 nNormalUsedSpace = 0;
	DVR_U32 nEventUsedSpace = 0;
	DVR_U32 nPhotoUsedSpace = 0;
	DVR_U32 nDasUsedSpace = 0;
    DVR_U32 nApaCount = 0;
    DVR_U32 nAccCount = 0;
    DVR_U32 nIaccCount = 0;
    int res = DVR_RES_SOK;

    while (p->m_bExit == 0)
    {
        OSA_Sleep(1000);

        DVR_STORAGE_QUOTA nStorageQuota;
        memset(&nStorageQuota, 0, sizeof(DVR_STORAGE_QUOTA));

        p->m_sysCtrl->StorageCard_Get(DVR_STORAGE_PROP_CARD_QUOTA, &nStorageQuota, sizeof(DVR_STORAGE_QUOTA), NULL);

        if (p->m_bEnableLoopRecDetect)
        {
            res |= p->m_sysCtrl->StorageCard_Get(DVR_STORAGE_PROP_CARD_TOTALSPACE, &nCardTotalSpace, sizeof(DVR_U32), NULL);
            res |= p->m_sysCtrl->StorageCard_Get(DVR_STORAGE_PROP_CARD_USEDSPACE, &nCardUsedSpace, sizeof(DVR_U32), NULL);
            res |= p->m_sysCtrl->StorageCard_Get(DVR_STORAGE_PROP_NORMAL_FOLDER_USEDSPACE, &nNormalUsedSpace, sizeof(DVR_U32), NULL);
            res |= p->m_sysCtrl->StorageCard_Get(DVR_STORAGE_PROP_EVENT_FOLDER_USEDSPACE, &nEventUsedSpace, sizeof(DVR_U32), NULL);
            res |= p->m_sysCtrl->StorageCard_Get(DVR_STORAGE_PROP_PHOTO_FOLDER_USEDSPACE, &nPhotoUsedSpace, sizeof(DVR_U32), NULL);
            res |= p->m_sysCtrl->StorageCard_Get(DVR_STORAGE_PROP_DAS_FOLDER_USEDSPACE, &nDasUsedSpace, sizeof(DVR_U32), NULL);
            DVR_U32 nUsedSpaceExceptDVR = nCardUsedSpace - nNormalUsedSpace - nEventUsedSpace - nPhotoUsedSpace - nDasUsedSpace;
            DVR_U32 nCardSpaceForDVR = nCardTotalSpace - nUsedSpaceExceptDVR;
            DVR_U32 nUsedSpaceDVR = nNormalUsedSpace + nEventUsedSpace + nPhotoUsedSpace + nDasUsedSpace;
            if(nCardSpaceForDVR > nUsedSpaceDVR)
                freeSpaceSize = nCardSpaceForDVR - nUsedSpaceDVR;
             else
                freeSpaceSize = 0;

            if ((res == DVR_RES_SOK) && (freeSpaceSize <= p->m_LoopRecStorageMonSize))
            {
                if (p->m_bEnableLoopRecMsg)
                {
                    if (p->m_LoopRecMsgFlag == 0)
                    {
                        DPrint(DPRINT_ERR, "m_CardAvailableSpace = %d nCardSpaceForDVR %d nUsedSpaceDVR = %d freeSpaceSize %d\n",p->m_CardAvailableSpace,nCardSpaceForDVR,nUsedSpaceDVR,freeSpaceSize);
                        p->m_sysCtrl->ComSvcHcmgr_SendMsg(HMSG_STORAGE_RUNOUT, DVR_FOLDER_TYPE_NORMAL, 0);
                        p->m_LoopRecMsgFlag = 1;
                    }
                }
            }
        }

        if (p->m_bEnableDasRecDetect)
        {
            res |= p->m_sysCtrl->StorageCard_Get(DVR_STORAGE_PROP_DAS_FOLDER_USEDSPACE, &nDasUsedSpace, sizeof(DVR_U32), NULL);
            if (nStorageQuota.nDasStorageQuota > nDasUsedSpace)
                freeSpaceSize = nStorageQuota.nDasStorageQuota - nDasUsedSpace;
            else
                freeSpaceSize = 0;

            res |= p->m_sysCtrl->FileMapDB_GetDasCount(&nApaCount, &nAccCount, &nIaccCount);
            //DVR_BOOL gloaldel   = freeSpaceSize <= p->m_DasRecStorageMonSize ? TRUE : FALSE; 
            DVR_BOOL apadel     = nApaCount >= DASREC_APA_STORAGE_MONITOR_NUM_LIMITATION ? TRUE : FALSE; 
            DVR_BOOL accdel     = nAccCount >= DASREC_IACC_STORAGE_MONITOR_SIZE_LIMITATION ? TRUE : FALSE;
            DVR_BOOL iaccdel    = nIaccCount >= DASREC_IACC_STORAGE_MONITOR_SIZE_LIMITATION ? TRUE : FALSE;
            if(res != DVR_RES_SOK)
                continue;

            DVR_BOOL needdel[3] = {apadel, accdel, iaccdel};
            for(int type = DVR_DAS_TYPE_APA; type < DVR_DAS_TYPE_NUM;type++)
            {
                if (!p->m_bEnableDasRecMsg[type - DVR_DAS_TYPE_APA])
                    continue;

                if(!needdel[type - DVR_DAS_TYPE_APA])
                    continue;

                if (p->m_DasRecMsgFlag == 0)
                {
                    DPrint(DPRINT_ERR, "+++++++++++++++apadel = %d accdel %d iaccdel %d++++++++++++++++\n",apadel, accdel, iaccdel);
                    p->m_sysCtrl->ComSvcHcmgr_SendMsg(HMSG_STORAGE_RUNOUT, DVR_FOLDER_TYPE_DAS, type);
                    p->m_DasRecMsgFlag = 1;
                }
            }

            /*if(gloaldel)
            {
                if (p->m_bEnableDasRecMsg)
                {
                    if (p->m_DasRecMsgFlag == 0)
                    {
                        p->m_sysCtrl->ComSvcHcmgr_SendMsg(HMSG_STORAGE_RUNOUT, DVR_FOLDER_TYPE_DAS, 0);
                        p->m_DasRecMsgFlag = 1;
                    }
                }
            }*/
        }
    }
}

void DvrMonitorStorage::SetEnableLoopRecDetect(DVR_BOOL enable)
{
    m_bEnableLoopRecDetect = enable;
}

void DvrMonitorStorage::SetLoopRecThreshold(DVR_U32 threshold)
{
    m_LoopRecStorageMonSize = threshold;
}

DVR_BOOL DvrMonitorStorage::GetEnableLoopRecDetect(void)
{
    return m_bEnableLoopRecDetect;
}

DVR_U32 DvrMonitorStorage::GetLoopRecThreshold(void)
{
    return m_LoopRecStorageMonSize;
}

void DvrMonitorStorage::SetEnableLoopRecMsg(DVR_BOOL enable)
{
    m_bEnableLoopRecMsg = enable;
    if (enable == 1) {
        m_LoopRecMsgFlag = 0;
    }
}

void DvrMonitorStorage::SetLoopRecQuota(DVR_U32 quota)
{
    m_LoopRecStorageQuota = quota;
}

DVR_U32 DvrMonitorStorage::GetLoopRecQuota(void)
{
    return m_LoopRecStorageQuota;
}

void DvrMonitorStorage::SetAvailableSpace(DVR_U32 space)
{
    m_CardAvailableSpace = space;
}

DVR_U32 DvrMonitorStorage::GetAvailableSpace(void)
{
    return m_CardAvailableSpace;
}


void DvrMonitorStorage::SetEnableDasRecDetect(DVR_BOOL enable)
{
    m_bEnableDasRecDetect = enable;
}

DVR_BOOL DvrMonitorStorage::GetEnableDasRecDetect(void)
{
    return m_bEnableDasRecDetect;
}

void DvrMonitorStorage::SetEnableDasRecMsg(DVR_DAS_MONITOR* monitor)
{
    if(monitor == NULL)
        return;

    if(monitor->eType == DVR_DAS_TYPE_NONE || monitor->eType >= DVR_DAS_TYPE_NUM)
        return;

    int type = monitor->eType - DVR_DAS_TYPE_APA;
    m_bEnableDasRecMsg[type] = monitor->bEnable;
    if (monitor->bEnable == 1) {
        m_DasRecMsgFlag = 0;
    }
}
