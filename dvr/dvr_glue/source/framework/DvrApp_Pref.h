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
#ifndef _DVR_APP_PREF_H_
#define _DVR_APP_PREF_H_

#include "DVR_SDK_DEF.h"

typedef struct 
{
	DVR_U8 LoopEncSplitTime;
	DVR_U8 VideoQuality;
	DVR_U8 PhotoQuality;
} APP_PREF_USER_SETUP;

typedef struct 
{
	APP_PREF_USER_SETUP SetupPref;
} APP_PREF_USER;

extern APP_PREF_USER *UserSetting;
/*************************************************************************
* User preference API
************************************************************************/
int AppPref_InitPref(void);
int AppPref_Save(void);

#endif