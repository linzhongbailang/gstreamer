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
#ifndef _DVR_WIDGET_MGT_H_
#define _DVR_WIDGET_MGT_H_

#include "DVR_SDK_DEF.h"

#define DVR_WIDGET_NONE			(0x00000000)
#define DVR_WIDGET_MENU_SETUP	(0x00000001)
#define DVR_WIDGET_DIALOG		(0x00000002)
#define DVR_WIDGET_ALL			(0xFFFFFFFF)

typedef struct  
{
	DVR_RESULT(*On)(DVR_U32 param);
	DVR_RESULT(*Off)(DVR_U32 param);
	DVR_RESULT(*OnMessage)(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2);
}WIDGET_ITEM;


DVR_RESULT AppWidget_Init(void);
DVR_U32 AppWidget_GetCur(void);
DVR_RESULT AppWidget_On(DVR_U32 widgetId, DVR_U32 param);
DVR_RESULT AppWidget_Off(DVR_U32 widgetId, DVR_U32 param);
DVR_RESULT AppWidget_OnMessage(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2);
DVR_U32 AppWidget_GetFlags(void);

#endif