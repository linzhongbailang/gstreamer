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
#ifndef _DVR_PB_APP_GUI_H_
#define _DVR_PB_APP_GUI_H_

#include "DVR_SDK_DEF.h"

typedef enum
{
	GUI_PB_APP_CMD_FLUSH,
	GUI_PB_APP_CMD_SET_LAYOUT,
	GUI_PB_APP_CMD_MAIN_MENU_TAB_UPDATE,
	GUI_PB_APP_CMD_WARNING_SHOW,
	GUI_PB_APP_CMD_WARNING_HIDE,
	GUI_PB_APP_CMD_WARNING_UPDATE,
	GUI_PB_APP_CMD_VIEW_UPDATE,

	GUI_PB_APP_CMD_PLAY_STATE_UPDATE,
	GUI_PB_APP_CMD_PLAY_TIMER_UPDATE,
	GUI_PB_APP_CMD_PLAY_SPEED_UPDATE,
	GUI_PB_APP_CMD_FILENAME_SHOW,
	GUI_PB_APP_CMD_FILENAME_HIDE,
	GUI_PB_APP_CMD_FILENAME_UPDATE,
    GUI_PB_APP_CMD_SIDEBAR_SHOW,
    GUI_PB_APP_CMD_SIDEBAR_HIDE,
    GUI_PB_APP_CMD_MODE_UPDATE,
    GUI_PB_APP_CMD_VEHICLE_DATA_UPDATE,
}GUI_PB_APP_CMD;

int gui_pb_app_func(DVR_U32 guiCmd, DVR_U32 param1, DVR_U32 param2);

#endif