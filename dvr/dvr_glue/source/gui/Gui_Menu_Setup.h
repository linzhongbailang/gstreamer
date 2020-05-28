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
#ifndef _GUI_MENU_SETUP_H_
#define _GUI_MENU_SETUP_H_

#include "DVR_SDK_DEF.h"

/*************************************************************************
* Menu GUI definitions
************************************************************************/

typedef enum 
{
	GUI_MENU_SETUP_CMD_FLUSH = 0,
	GUI_MENU_SETUP_CMD_SET_LAYOUT,
	GUI_MENU_SETUP_CMD_MAIN_MENU_TAB_UPDATE,
	GUI_MENU_SETUP_CMD_LOOPENC_SPLIT_TIME_UPDATE,
	GUI_MENU_SETUP_CMD_VIDEO_QUALITY_UPDATE,
	GUI_MENU_SETUP_CMD_PHOTO_QUALITY_UPDATE,
    GUI_MENU_SETUP_CMD_FORMAT_CARD_ENABLE,
    GUI_MENU_SETUP_CMD_FORMAT_CARD_DISABLE
} GUI_MENU_CMD_ID;

/*************************************************************************
* Menu Widget GUI functions
************************************************************************/
int gui_menu_setup_func(DVR_U32 guiCmd, DVR_U32 param1, DVR_U32 param2);

#endif
