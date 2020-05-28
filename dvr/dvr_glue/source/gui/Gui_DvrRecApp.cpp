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
#include "gui/Gui_DvrRecApp.h"
#include "graphics/DvrGraphics.h"

int gui_rec_app_func(DVR_U32 guiCmd, DVR_U32 param1, DVR_U32 param2)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrGuiResHandler *handler;

	handler = (DvrGuiResHandler *)Dvr_App_GetGuiResHandler();
	if (handler == NULL)
		return DVR_RES_EFAIL;

	switch (guiCmd)
	{
	case GUI_REC_APP_CMD_FLUSH:
		res = handler->Flush();
		break;
	case GUI_REC_APP_CMD_SET_LAYOUT:
		res = handler->SetLayout(param1);
		break;
	case GUI_REC_APP_CMD_MAIN_MENU_TAB_UPDATE:
		res = handler->Update(GUI_OBJ_ID_MAIN_MENU_TAB, &param1, param2);
		break;
	case GUI_REC_APP_CMD_CARD_SHOW:
		res = handler->Show(GUI_OBJ_ID_CARD_STATE);
		break;
	case GUI_REC_APP_CMD_CARD_HIDE:
		res = handler->Hide(GUI_OBJ_ID_CARD_STATE);
		break;
	case GUI_REC_APP_CMD_CARD_UPDATE:
		res = handler->Update(GUI_OBJ_ID_CARD_STATE, &param1, param2);
		break;
	case GUI_REC_APP_CMD_WARNING_SHOW:
		res = handler->Show(GUI_OBJ_ID_WARNING);
		break;
	case GUI_REC_APP_CMD_WARNING_HIDE:
		res = handler->Hide(GUI_OBJ_ID_WARNING);
		break;
	case GUI_REC_APP_CMD_WARNING_UPDATE:
		res = handler->Update(GUI_OBJ_ID_WARNING, &param1, param2);
		break;
	case GUI_REC_APP_CMD_REC_SWITCH_SHOW:
		res = handler->Show(GUI_OBJ_ID_REC_SWITCH);
		break;
	case GUI_REC_APP_CMD_REC_SWITCH_HIDE:
		res = handler->Hide(GUI_OBJ_ID_REC_SWITCH);
		break;
	case GUI_REC_APP_CMD_REC_SWITCH_UPDATE:
		res = handler->Update(GUI_OBJ_ID_REC_SWITCH, &param1, param2);
		break;
	case GUI_REC_APP_CMD_REC_STATE_SHOW:
		res = handler->Show(GUI_OBJ_ID_REC_STATE);
		break;
	case GUI_REC_APP_CMD_REC_STATE_HIDE:
		res = handler->Hide(GUI_OBJ_ID_REC_STATE);
		break;
	case GUI_REC_APP_CMD_REC_STATE_UPDATE:
		res = handler->Update(GUI_OBJ_ID_REC_STATE, &param1, param2);
		break;
	case GUI_REC_APP_CMD_EMERGENCY_STATE_SHOW:
		res = handler->Show(GUI_OBJ_ID_REC_EVENT_RECORD_STATE);
		break;
	case GUI_REC_APP_CMD_EMERGENCY_STATE_HIDE:
		res = handler->Hide(GUI_OBJ_ID_REC_EVENT_RECORD_STATE);
		break;
	case GUI_REC_APP_CMD_VIEW_UPDATE:
		res = handler->Update(GUI_OBJ_ID_REC_VIEW_INDEX, &param1, param2);
		break;
    case GUI_REC_APP_CMD_SIDEBAR_SHOW:
        res = handler->Show(GUI_OBJ_ID_SIDEBAR);
        break;
    case GUI_REC_APP_CMD_SIDEBAR_HIDE:
        res = handler->Hide(GUI_OBJ_ID_SIDEBAR);
        break;
	case GUI_REC_APP_CMD_VEHICLE_DATA_UPDATE:
		res = handler->Update(GUI_OBJ_ID_REC_VEHICLE_DATA, (void *)param1, param2);
		break;
	default:
		res = DVR_RES_EFAIL;
		break;
	}

	return res;
}
