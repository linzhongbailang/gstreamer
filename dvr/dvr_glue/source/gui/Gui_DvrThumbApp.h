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
#ifndef _DVR_THUMB_APP_GUI_H_
#define _DVR_THUMB_APP_GUI_H_

#include "DVR_SDK_DEF.h"

typedef enum
{
	GUI_THUMB_APP_CMD_FLUSH,
	GUI_THUMB_APP_CMD_SET_LAYOUT,
	GUI_THUMB_APP_CMD_HIDE_ALL,
	GUI_THUMB_APP_CMD_MAIN_MENU_TAB_UPDATE,
	GUI_THUMB_APP_CMD_FRAME_UPDATE,
	GUI_THUMB_APP_CMD_STORAGE_CAPACITY_SHOW,
	GUI_THUMB_APP_CMD_STORAGE_CAPACITY_HIDE,
	GUI_THUMB_APP_CMD_STORAGE_CAPACITY_UPDATE,
    GUI_THUMB_APP_CMD_PAGENUM_UPDATE,
	GUI_THUMB_APP_CMD_TAB_UPDATE,
	GUI_THUMB_APP_CMD_EDIT_UPDATE,
	GUI_THUMB_APP_CMD_EDIT_CHECKBOX_SHOW,
	GUI_THUMB_APP_CMD_EDIT_CHECKBOX_HIDE,
	GUI_THUMB_APP_CMD_EDIT_CHECKBOX_UPDATE,
	GUI_THUMB_APP_CMD_WARNING_SHOW,
	GUI_THUMB_APP_CMD_WARNING_HIDE,
	GUI_THUMB_APP_CMD_WARNING_UPDATE,
	GUI_THUMB_APP_CMD_CARD_SHOW,
	GUI_THUMB_APP_CMD_CARD_HIDE,
	GUI_THUMB_APP_CMD_CARD_UPDATE
}GUI_THUMB_APP_CMD;

int gui_thumb_app_func(DVR_U32 guiCmd, DVR_U32 param1, DVR_U32 param2);

#endif