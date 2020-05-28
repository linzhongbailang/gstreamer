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
#ifndef _DVR_APP_INTFS_H_
#define _DVR_APP_INTFS_H_

#include "DVR_APP_DEF.h"

#ifdef __cplusplus
extern "C"
{
#endif

int Dvr_App_Initialize(void);

int Dvr_App_DeInitialize(void);

int Dvr_App_GetAppId(int module);

const char *Dvr_App_GetAppName(int appId);

void* Dvr_App_GetStatus(void);

void *Dvr_App_GetGuiResHandler(void);

int Dvr_App_GetCurMode(void);

int Dvr_App_HuSendCmd(unsigned int msg, unsigned int param1, unsigned int param2);

int Dvr_App_SetStartApp(int eDvrMode);

int Dvr_App_SetRecorderSpitTime(int eSplitTime);

int Dvr_App_SetLanguageSelected(int eLangSel);

int Dvr_App_UpdateSystemTime(unsigned int year, 
										unsigned int month, 
										unsigned int day, 
										unsigned int hour, 
										unsigned int minute,
										unsigned int second);

#ifdef __cplusplus
}
#endif

#endif
