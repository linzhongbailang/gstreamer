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
#include "gui/Gui_DvrPbApp.h"
#include "graphics/DvrGraphics.h"

int gui_pb_app_func(DVR_U32 guiCmd, DVR_U32 param1, DVR_U32 param2)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrGuiResHandler *handler;

	handler = (DvrGuiResHandler *)Dvr_App_GetGuiResHandler();
	if (handler == NULL)
		return DVR_RES_EFAIL;

	switch (guiCmd)
	{
	case GUI_PB_APP_CMD_FLUSH:
		res = handler->Flush();
		break;

	case GUI_PB_APP_CMD_SET_LAYOUT:
		res = handler->SetLayout(param1);
		break;

	case GUI_PB_APP_CMD_MAIN_MENU_TAB_UPDATE:
		res = handler->Update(GUI_OBJ_ID_MAIN_MENU_TAB, &param1, param2);
		break;

	case GUI_PB_APP_CMD_WARNING_SHOW:
		res = handler->Show(GUI_OBJ_ID_WARNING);
		break;

	case GUI_PB_APP_CMD_WARNING_HIDE:
		res = handler->Hide(GUI_OBJ_ID_WARNING);
		break;

	case GUI_PB_APP_CMD_WARNING_UPDATE:
		res = handler->Update(GUI_OBJ_ID_WARNING, &param1, param2);
		break;

	case GUI_PB_APP_CMD_VIEW_UPDATE:
		res = handler->Update(GUI_OBJ_ID_PB_VIEW_INDEX, &param1, param2);
		break;

	case GUI_PB_APP_CMD_PLAY_STATE_UPDATE:
		res = handler->Update(GUI_OBJ_ID_PB_PLAY_STATE, &param1, param2);
		break;

	case GUI_PB_APP_CMD_PLAY_TIMER_UPDATE:
		res = handler->Update(GUI_OBJ_ID_PB_PLAY_TIMER, (void *)param1, param2);
		break;

	case GUI_PB_APP_CMD_PLAY_SPEED_UPDATE:
		res = handler->Update(GUI_OBJ_ID_PB_PLAY_SPEED, &param1, param2);
		break;

	case GUI_PB_APP_CMD_FILENAME_SHOW:
		res = handler->Show(GUI_OBJ_ID_PB_FILENAME);
		break;

	case GUI_PB_APP_CMD_FILENAME_HIDE:
		res = handler->Hide(GUI_OBJ_ID_PB_FILENAME);
		break;

	case GUI_PB_APP_CMD_FILENAME_UPDATE:
		res = handler->Update(GUI_OBJ_ID_PB_FILENAME, (void *)param1, param2);
		break;

    case GUI_PB_APP_CMD_SIDEBAR_SHOW:
        res = handler->Show(GUI_OBJ_ID_SIDEBAR);
        break;

    case GUI_PB_APP_CMD_SIDEBAR_HIDE:
        res = handler->Hide(GUI_OBJ_ID_SIDEBAR);
        break;

    case GUI_PB_APP_CMD_MODE_UPDATE:
        res = handler->Update(GUI_OBJ_ID_PB_MODE, &param1, param2);
        break;

	case GUI_PB_APP_CMD_VEHICLE_DATA_UPDATE:
        res = handler->Update(GUI_OBJ_ID_PB_VEHICLE_DATA, (void *)param1, param2);
        break;		

	default:
		res = DVR_RES_EFAIL;
		break;
	}

	return res;
}
