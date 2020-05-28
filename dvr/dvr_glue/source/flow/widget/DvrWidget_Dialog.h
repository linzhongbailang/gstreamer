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
#ifndef _DVR_WIDGET_DIALOG_H_
#define _DVR_WIDGET_DIALOG_H_

#include "DVR_SDK_DEF.h"
#include "flow/widget/DvrWidget_Mgt.h"

/*************************************************************************
* Dialog definitions
************************************************************************/
#if 0
typedef enum {
	DIALOG_TYPE_OK = 0,
	DIALOG_TYPE_Y_N,
	DIALOG_TYPE_NUM
} DIALOG_TYPE_ID;

#define DIALOG_SEL_OK    (0x00)
#define DIALOG_SEL_NO    (0x00)
#define DIALOG_SEL_YES   (0x01)
#endif

typedef int(*DialogSetHandler)(DVR_U32 sel, DVR_U32 param1, DVR_U32 param2);

WIDGET_ITEM* AppDialog_GetWidget(void);
int AppDialog_SetDialog(DVR_U32 type, DVR_U32 subject, DialogSetHandler set);

#endif
