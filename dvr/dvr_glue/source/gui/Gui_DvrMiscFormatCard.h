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
#ifndef _GUI_DVR_MISC_FORMATCARD_H_
#define _GUI_DVR_MISC_FORMATCARD_H_

#include "DVR_SDK_DEF.h"

typedef enum
{
	GUI_MISC_FORMATCARD_APP_CMD_FLUSH,
	GUI_MISC_FORMATCARD_APP_CMD_WARNING_UPDATE,
	GUI_MISC_FORMATCARD_APP_CMD_WARNING_SHOW,
	GUI_MISC_FORMATCARD_APP_CMD_WARNING_HIDE
}GUI_MISC_FORMATCARD_APP_CMD;

int gui_misc_formatcard_func(DVR_U32 guiCmd, DVR_U32 param1, DVR_U32 param2);

#endif
