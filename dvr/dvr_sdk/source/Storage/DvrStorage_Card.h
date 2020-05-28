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
#ifndef _DVR_STORAGE_CARD_H_
#define _DVR_STORAGE_CARD_H_

#include "DVR_SDK_INTFS.h"
#include "osa.h"
#include "osa_mutex.h"
#include "osa_tsk.h"
#include "osa_msgq.h"
#include "osa_fs.h"
#include "DvrSystemControl.h"

#include <map>

enum 
{
	CARD_FLAGS_ERROR_FORMAT,
	CARD_FLAGS_ERROR_FRGMT,
};

class DvrStorageCard
{
public:
	DvrStorageCard(void *sysCtrl);
	~DvrStorageCard(void);
    
    DVR_U32 DvrStorageCard_GetLoopRecUsedSpace(void);
    DVR_U32 DvrStorageCard_GetEventRecUsedSpace(void);
    DVR_U32 DvrStorageCard_GetPhotoUsedSpace(void);
	DVR_U32 DvrStorageCard_GetDasUsedSpace(void);
	DVR_U32 DvrStorageCard_GetFreeSpace(const char *szDrive);
	DVR_U32 DvrStorageCard_GetTotalSpace(const char *szDrive);
	DVR_U32 DvrStorageCard_GetUsedSpace(const char *szDrive);
	DVR_RESULT DvrStorageCard_Format(const char *szDrive);
	DVR_RESULT DvrStorageCard_CheckFrgmt(const char *szDrive);
	DVR_RESULT DvrStorageCard_SetThreshold(DVR_U32 threshold);
	DVR_U32 DvrStorageCard_GetThreshold(void);
	DVR_S32 DvrStorageCard_CheckStatus(void);
	DVR_RESULT DvrStorageCard_SetRoot(char *szDrive);
	DVR_RESULT DvrStorageCard_GetDirectory(DVR_DISK_DIRECTORY *pDir);
	DVR_RESULT DvrStorageCard_GetFolder(DVR_FOLDER_TYPE type, char *pFolder);
	DVR_RESULT DvrStorageCard_SetQuota(DVR_STORAGE_QUOTA *pQuota);
	DVR_RESULT DvrStorageCard_GetQuota(DVR_STORAGE_QUOTA *pQuota);
	DVR_RESULT DvrStorageCard_SetCardStatus(DVR_BOOL bCardPlugOut);
	DVR_RESULT DvrStorageCard_SetSpeedStatus(DVR_BOOL bLowSpeedCard);
	DVR_RESULT DvrStorageCard_SetFreeSpaceStatus(DVR_BOOL bFreeSpaceTooSmall);
	DVR_RESULT DvrStorageCard_SetReadyStatus(DVR_BOOL bCardReady);

private:
	DISABLE_COPY(DvrStorageCard)

	DVR_BOOL	m_bWriteProtect;
	DVR_BOOL	m_bCardPlugOut;
	DVR_BOOL	m_bCardLowSpeed;
	DVR_BOOL	m_bCardNotReady;
	DVR_BOOL	m_bFreeSpaceTooSmall;
	DVR_U8		m_flagsError;
	DVR_U32		m_diskTotalSpace; //in MB
	DVR_U32		m_diskFreeSpace; //in MB
	DVR_U32		m_diskUsedSpace; //in MB
	DVR_U32		m_diskThreshold; //in MB
	DvrSystemControl *m_sysCtrl;

	std::string	m_szRootDir;
	std::string	m_szLoopRecDir;
    std::string	m_szEventRecDir;
	std::string	m_szPhotoDir;
	std::string m_szDasDir;

	std::map<DVR_FOLDER_TYPE, std::string> m_FolderMap;
	DVR_STORAGE_QUOTA m_StorageQuota;
};

#endif /* _DVR_STORAGE_CARD_H_ */
