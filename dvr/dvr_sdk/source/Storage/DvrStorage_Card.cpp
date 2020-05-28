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
#include "DvrStorage_Card.h"

using std::make_pair;

DvrStorageCard::DvrStorageCard(void *sysCtrl)
{
	m_bWriteProtect = 0;
	m_bCardPlugOut = FALSE;
	m_bCardLowSpeed = FALSE;

    m_bCardNotReady    = TRUE;
	m_bFreeSpaceTooSmall = FALSE;
	m_flagsError = 0;
	m_diskTotalSpace = 0; //in MB
	m_diskFreeSpace = 0; //in MB
	m_diskUsedSpace = 0; //in MB
	m_diskThreshold = 0; // in MB

	m_szRootDir.clear();
    m_szLoopRecDir.clear();
    m_szEventRecDir.clear();
	m_szPhotoDir.clear();
	m_szDasDir.clear();
	m_FolderMap.clear();

	memset(&m_StorageQuota, 0, sizeof(DVR_STORAGE_QUOTA));

	m_sysCtrl = (DvrSystemControl *)sysCtrl;
}

DvrStorageCard::~DvrStorageCard()
{
	m_szRootDir.clear();
    m_szLoopRecDir.clear();
    m_szEventRecDir.clear();
	m_szPhotoDir.clear();
	m_szDasDir.clear();
	m_FolderMap.clear();
}

DVR_U32 DvrStorageCard::DvrStorageCard_GetLoopRecUsedSpace(void)
{
    return DVR::OSA_FS_CheckUsedSpace(m_szLoopRecDir.data(), &m_bCardPlugOut);
}

DVR_U32 DvrStorageCard::DvrStorageCard_GetEventRecUsedSpace(void)
{
    return DVR::OSA_FS_CheckUsedSpace(m_szEventRecDir.data(), &m_bCardPlugOut);
}

DVR_U32 DvrStorageCard::DvrStorageCard_GetPhotoUsedSpace(void)
{
    return DVR::OSA_FS_CheckUsedSpace(m_szPhotoDir.data(), &m_bCardPlugOut);
}

DVR_U32 DvrStorageCard::DvrStorageCard_GetDasUsedSpace(void)
{
    return DVR::OSA_FS_CheckUsedSpace(m_szDasDir.data(), &m_bCardPlugOut);
}

DVR_U32 DvrStorageCard::DvrStorageCard_GetFreeSpace(const char *szDrive)
{
	return DVR::OSA_FS_GetFreeSpace(szDrive);
}

DVR_U32 DvrStorageCard::DvrStorageCard_GetTotalSpace(const char *szDrive)
{
	return DVR::OSA_FS_GetTotalSpace(szDrive);
}

DVR_U32 DvrStorageCard::DvrStorageCard_GetUsedSpace(const char *szDrive)
{
	return DVR::OSA_FS_GetUsedSpace(szDrive);
}

DVR_RESULT DvrStorageCard::DvrStorageCard_Format(const char *szDrive)
{
	return DVR_RES_SOK;
}

DVR_RESULT DvrStorageCard::DvrStorageCard_CheckFrgmt(const char *szDrive)
{
	return DVR_RES_SOK;
}

DVR_RESULT DvrStorageCard::DvrStorageCard_SetThreshold(DVR_U32 threshold)
{
	m_diskThreshold = threshold;

	return DVR_RES_SOK;
}

DVR_U32 DvrStorageCard::DvrStorageCard_GetThreshold(void)
{
	return m_diskThreshold;
}

DVR_S32 DvrStorageCard::DvrStorageCard_CheckStatus(void)
{
	int res = DVR_STORAGE_CARD_STATUS_CHECK_PASS;
	DVR_STORAGE_QUOTA quota;
	DVR_U32 nEventUsedSpace;
	DVR_U32 nPhotoUsedSpace;

	if (m_sysCtrl->CurrentMountPoint() == NULL)
	{
		if(m_sysCtrl->CurrentDevicePath() != NULL)
		{
			res |= DVR_STORAGE_CARD_STATUS_ERROR_FORMAT;
		}
		else
		{
			res |= DVR_STORAGE_CARD_STATUS_NO_CARD;
		}
	}
	else
	{
		DvrStorageCard_GetQuota(&quota);
		
        m_sysCtrl->StorageCard_Get(DVR_STORAGE_PROP_EVENT_FOLDER_USEDSPACE, &nEventUsedSpace, sizeof(DVR_U32), NULL);
        m_sysCtrl->StorageCard_Get(DVR_STORAGE_PROP_PHOTO_FOLDER_USEDSPACE, &nPhotoUsedSpace, sizeof(DVR_U32), NULL);

		DVR_BOOL isEventFolderFull;
		DVR_BOOL isPhotoFolderFull;

		if(quota.nEventStorageQuota >= nEventUsedSpace){
			isEventFolderFull = ((quota.nEventStorageQuota - nEventUsedSpace) < DVR_STORAGE_EVENT_FOLDER_WARNING_THRESHOLD);
		}
		else{
			isEventFolderFull = TRUE;
		}

		if(quota.nPhotoStorageQuota >= nPhotoUsedSpace){
			isPhotoFolderFull = ((quota.nPhotoStorageQuota - nPhotoUsedSpace) < DVR_STORAGE_PHOTO_FOLDER_WARNING_THRESHOLD);
		}
		else{
			isPhotoFolderFull = TRUE;
		}
		
		if(isEventFolderFull)
		{
			res |= DVR_STORAGE_CARD_STATUS_EVENT_FOLDER_NOT_ENOUGH_SPACE;
		}

		if(isPhotoFolderFull)
		{
			res |= DVR_STORAGE_CARD_STATUS_PHOTO_FOLDER_NOT_ENOUGH_SPACE;
		}
	}

	if(m_bCardLowSpeed)
	{
		res |= DVR_STORAGE_CARD_STATUS_LOW_SPEED;
	}

    if(m_bCardNotReady)
    {
        res |= DVR_STORAGE_CARD_STATUS_NOT_READY;
    }

	if(m_bFreeSpaceTooSmall)
	{
		res = DVR_STORAGE_CARD_STATUS_NOT_ENOUGH_SPACE;
	}
	
	return res;
}

DVR_RESULT DvrStorageCard::DvrStorageCard_SetRoot(char *pRoot)
{
	DVR_RESULT res = DVR_RES_SOK;

	m_szRootDir.clear();
    m_szLoopRecDir.clear();
    m_szEventRecDir.clear();
	m_szPhotoDir.clear();
	m_FolderMap.clear();
    m_szDasDir.clear();

	m_szRootDir = pRoot;
#ifdef WIN32
    m_szLoopRecDir = m_szRootDir + "NOR\\";
    m_szEventRecDir = m_szRootDir + "EVT\\";
	m_szPhotoDir = m_szRootDir + "PHO\\";
	m_szDasDir = m_szRootDir + "DAS\\";
#else
    m_szLoopRecDir = m_szRootDir + "/NOR/";
    m_szEventRecDir = m_szRootDir + "/EVT/";
	m_szPhotoDir = m_szRootDir + "/PHO/";
//#ifndef DVR_FEATURE_V302
	m_szDasDir = m_szRootDir + "/DAS/";
//#endif
#endif

	DVR::OSA_MkDir(m_szRootDir.data());
    DVR::OSA_MkDir(m_szLoopRecDir.data());
    DVR::OSA_MkDir(m_szEventRecDir.data());
	DVR::OSA_MkDir(m_szPhotoDir.data());
//#ifndef DVR_FEATURE_V302
	DVR::OSA_MkDir(m_szDasDir.data());
//#endif

	m_FolderMap.insert(make_pair(DVR_FOLDER_TYPE_ROOT, m_szRootDir));
    m_FolderMap.insert(make_pair(DVR_FOLDER_TYPE_NORMAL, m_szLoopRecDir));
    m_FolderMap.insert(make_pair(DVR_FOLDER_TYPE_EMERGENCY, m_szEventRecDir));
	m_FolderMap.insert(make_pair(DVR_FOLDER_TYPE_PHOTO, m_szPhotoDir));
//#ifndef DVR_FEATURE_V302
	m_FolderMap.insert(make_pair(DVR_FOLDER_TYPE_DAS, m_szDasDir));
//#endif

	return res;
}

DVR_RESULT DvrStorageCard::DvrStorageCard_GetDirectory(DVR_DISK_DIRECTORY *pDir)
{
	DVR_RESULT res = DVR_RES_SOK;

    if (m_szRootDir.empty() || m_szLoopRecDir.empty() || m_szEventRecDir.empty() || m_szPhotoDir.empty())
		return DVR_RES_EFAIL;

	memcpy(pDir->szRootDir, m_szRootDir.data(), m_szRootDir.length());
    memcpy(pDir->szLoopRecDir, m_szLoopRecDir.data(), m_szLoopRecDir.length());
    memcpy(pDir->szEventRecDir, m_szEventRecDir.data(), m_szEventRecDir.length());
	memcpy(pDir->szPhotoDir, m_szPhotoDir.data(), m_szPhotoDir.length());
	memcpy(pDir->szDasDir, m_szDasDir.data(), m_szDasDir.length());

	return res;
}

DVR_RESULT DvrStorageCard::DvrStorageCard_GetFolder(DVR_FOLDER_TYPE type, char *pFolder)
{
	DVR_RESULT res = DVR_RES_SOK;

	std::map<DVR_FOLDER_TYPE, std::string>::iterator it;

	if (pFolder == NULL)
		return DVR_RES_EFAIL;

	it = m_FolderMap.find(type);
	if (it == m_FolderMap.end())
		return DVR_RES_EFAIL;

	memcpy(pFolder, it->second.data(), it->second.length());

	return res;
}

DVR_RESULT DvrStorageCard::DvrStorageCard_SetQuota(DVR_STORAGE_QUOTA *pQuota)
{
	if(pQuota != NULL)
	{
		memcpy(&m_StorageQuota, pQuota, sizeof(DVR_STORAGE_QUOTA));
	}

	return DVR_RES_SOK;
}

DVR_RESULT DvrStorageCard::DvrStorageCard_GetQuota(DVR_STORAGE_QUOTA *pQuota)
{
	if(pQuota != NULL)
	{
		memcpy(pQuota, &m_StorageQuota, sizeof(DVR_STORAGE_QUOTA));
	}
	
	return DVR_RES_SOK;
}

DVR_RESULT DvrStorageCard::DvrStorageCard_SetCardStatus(DVR_BOOL bCardPlugOut)
{
	DPrint(DPRINT_INFO, "bCardPlugOut = %d!!!!!!!", bCardPlugOut);
	
	m_bCardPlugOut = bCardPlugOut;
	
	return DVR_RES_SOK;
}

DVR_RESULT DvrStorageCard::DvrStorageCard_SetSpeedStatus(DVR_BOOL bLowSpeedCard)
{
	DPrint(DPRINT_INFO, "bLowSpeedCard = %d!!!!!!!", bLowSpeedCard);
	
	m_bCardLowSpeed = bLowSpeedCard;
	
	return DVR_RES_SOK;	
}

DVR_RESULT DvrStorageCard::DvrStorageCard_SetFreeSpaceStatus(DVR_BOOL bFreeSpaceTooSmall)
{
	DPrint(DPRINT_INFO, "bFreeSpaceTooSmall = %d!!!!!!!", bFreeSpaceTooSmall);
	
	m_bFreeSpaceTooSmall = bFreeSpaceTooSmall;
	
	return DVR_RES_SOK;	
}

DVR_RESULT DvrStorageCard::DvrStorageCard_SetReadyStatus(DVR_BOOL bCardNotReady)
{
	DPrint(DPRINT_INFO, "bCardNotReady = %d!!!!!!!", bCardNotReady);
	
	m_bCardNotReady = bCardNotReady;
	
	return DVR_RES_SOK;	
}


