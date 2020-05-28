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
#ifndef _GUI_DIALOG_H_
#define _GUI_DIALOG_H_

#include "DVR_SDK_DEF.h"

typedef enum
{
	GUI_DIALOG_BTN_CANCEL,
	GUI_DIALOG_BTN_SET,
	GUI_DIALOG_BTN_NUM
}GUI_DIALOG_BTN;

typedef enum
{
	GUI_DIALOG_CMD_SHOW,
	GUI_DIALOG_CMD_HIDE,
	GUI_DIALOG_CMD_UPDATE,
	GUI_DIALOG_CMD_FLUSH
}GUI_DIALOG_CMD;

extern int gui_dialog_func(DVR_U32 guiCmd, DVR_U32 param1, DVR_U32 param2);

#endif