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
#ifndef _DVR_APP_UTIL_H_
#define _DVR_APP_UTIL_H_

#include "DVR_APP_DEF.h"
#include <DVR_SDK_INTFS.h>

DVR_RESULT DvrAppUtil_ReadyCheck(void);
DVR_RESULT DvrAppUtil_SwitchApp(int appId);
int DvrAppUtil_GetStartApp(void);
int DvrAppUtil_SetStartApp(int eDvrMode);
int DvrAppUtil_SetRecorderSplitTime(int eSplitTime);
int DvrAppUtil_GetRecorderSplitTime(int *pSplitTime);
int DvrAppUtil_SetLanguageSel(int eLangSel);
int DvrAppUtil_GetLanguageSel(int *pLangSel);

#endif
