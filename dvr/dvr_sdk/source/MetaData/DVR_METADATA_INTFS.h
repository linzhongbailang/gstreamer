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
#ifndef _DVR_METADATA_INTFS_H_
#define _DVR_METADATA_INTFS_H_

#include "DVR_SDK_DEF.h"

void* Dvr_MetaData_Create(void* pvFileName, unsigned int *pu32ItemNum, bool loadall);
void* Dvr_MetaData_Create_Timeout(void* pvFileName, unsigned int* pu32ItemNum, unsigned int u32Timeout);
int Dvr_MetaData_Destroy(void *pvMetaDataHandle);
int Dvr_MetaData_GetItemNum(void *pvMetaDataHandle);
int Dvr_MetaData_GetDuration(void* pvFileName, int* pPrecision);
const DVR_METADATA_ITEM* Dvr_MetaData_GetDataByIndex(void* pvMetaDataHandle, unsigned int u32Index);
const DVR_METADATA_ITEM* Dvr_MetaData_GetDataByType(void* pvMetaDataHandle, DVR_METADATA_TYPE eType);
int Dvr_MetaData_GetMediaInfo(void* pvMetaDataHandle, DVR_MEDIA_INFO* pInfo);
const DVR_MEDIA_TRACK* Dvr_MetaData_GetTrackByIndex(void* pvMetaDataHandle, unsigned int u32Index);
DVR_RESULT Dvr_Get_MediaFilePreview(DVR_PREVIEW_OPTION *pPreviewOpt, unsigned char *pBuf, unsigned int *cbSize);

#endif
