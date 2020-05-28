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
#ifndef _DVR_REC_APP_GUI_H_
#define _DVR_REC_APP_GUI_H_

#include "DVR_SDK_DEF.h"

typedef enum
{
	GUI_REC_APP_CMD_FLUSH,
	GUI_REC_APP_CMD_SET_LAYOUT,
	GUI_REC_APP_CMD_MAIN_MENU_TAB_UPDATE,
	GUI_REC_APP_CMD_CARD_SHOW,
	GUI_REC_APP_CMD_CARD_HIDE,
	GUI_REC_APP_CMD_CARD_UPDATE,
	GUI_REC_APP_CMD_WARNING_SHOW,
	GUI_REC_APP_CMD_WARNING_HIDE,
	GUI_REC_APP_CMD_WARNING_UPDATE,
	GUI_REC_APP_CMD_REC_SWITCH_SHOW,
	GUI_REC_APP_CMD_REC_SWITCH_HIDE,
	GUI_REC_APP_CMD_REC_SWITCH_UPDATE,
	GUI_REC_APP_CMD_REC_STATE_SHOW,
	GUI_REC_APP_CMD_REC_STATE_HIDE,
	GUI_REC_APP_CMD_REC_STATE_UPDATE,
	GUI_REC_APP_CMD_EMERGENCY_STATE_SHOW,
	GUI_REC_APP_CMD_EMERGENCY_STATE_HIDE,
	GUI_REC_APP_CMD_VIEW_UPDATE,
    GUI_REC_APP_CMD_SIDEBAR_SHOW,
    GUI_REC_APP_CMD_SIDEBAR_HIDE,
    GUI_REC_APP_CMD_VEHICLE_DATA_UPDATE
}GUI_REC_APP_CMD;

int gui_rec_app_func(DVR_U32 guiCmd, DVR_U32 param1, DVR_U32 param2);

#endif