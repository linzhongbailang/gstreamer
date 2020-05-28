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
#include "DVR_APP_INTFS.h"
#include "DVR_GUI_OBJ.h"
#include "gui/Gui_Menu_Setup.h"
#include "graphics/DvrGraphics.h"
#include <log/log.h>

int gui_menu_setup_func(DVR_U32 gui_cmd, DVR_U32 param1, DVR_U32 param2)
{
	DVR_RESULT res = 0;
	DvrGuiResHandler *handler;

	handler = (DvrGuiResHandler *)Dvr_App_GetGuiResHandler();
	if (handler == NULL)
		return DVR_RES_EFAIL;

	switch (gui_cmd) {
	case GUI_MENU_SETUP_CMD_SET_LAYOUT:
		res = handler->SetLayout(param1);
		break;

	case GUI_MENU_SETUP_CMD_MAIN_MENU_TAB_UPDATE:
		res = handler->Update(GUI_OBJ_ID_MAIN_MENU_TAB, &param1, param2);
		break;

	case GUI_MENU_SETUP_CMD_LOOPENC_SPLIT_TIME_UPDATE:
		res = handler->Update(GUI_OBJ_ID_SETUP_SPLIT_TIME, &param1, param2);
		break;

	case GUI_MENU_SETUP_CMD_VIDEO_QUALITY_UPDATE:
		res = handler->Update(GUI_OBJ_ID_SETUP_VIDEO_QUALITY, &param1, param2);
		break;

	case GUI_MENU_SETUP_CMD_PHOTO_QUALITY_UPDATE:
		res = handler->Update(GUI_OBJ_ID_SETUP_PHOTO_QUALITY, &param1, param2);
		break;

    case GUI_MENU_SETUP_CMD_FORMAT_CARD_ENABLE:
        res = handler->Enable(GUI_OBJ_ID_SETUP_FORMAT_CARD);
        break;

    case GUI_MENU_SETUP_CMD_FORMAT_CARD_DISABLE:
        res = handler->Disable(GUI_OBJ_ID_SETUP_FORMAT_CARD);
        break;

	case GUI_MENU_SETUP_CMD_FLUSH:
		res = handler->Flush();
		break;

	default:
		Log_Error("[Gui - Menu] <Func> Undefined GUI command\n");
		res = DVR_RES_EFAIL;
		break;
	}

	return res;
}
