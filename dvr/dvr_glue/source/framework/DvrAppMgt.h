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
#ifndef _DVR_APP_MGT_H_
#define _DVR_APP_MGT_H_
/*Header of Application Management*/

#include "DVR_SDK_DEF.h"

typedef struct
{
	DVR_S32 Id;
	DVR_S32 Tier;
	DVR_S32 Parent;
	DVR_S32 Previous;
	DVR_S32 Child;
	DVR_U32 GFlags;
	DVR_U32 Flags;

	DVR_S32(*Start)(void);
	DVR_S32(*Stop)(void);
	DVR_S32(*OnMessage)(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2);
}APP_MGT_INST;

int DvrAppMgt_Init(APP_MGT_INST *sysAppPool[], int appMaxNum);
int DvrAppMgt_Register(APP_MGT_INST *app);
int DvrAppMgt_GetApp(APP_MGT_INST **app, int appId);
int DvrAppMgt_GetCurApp(APP_MGT_INST **app);
int DvrAppMgt_SwitchApp(int appId);

int DvrAppMgt_CheckBusy(void);
int DvrAppMgt_CheckIo(void);
int DvrAppMgt_CheckIdle(void);

#endif
