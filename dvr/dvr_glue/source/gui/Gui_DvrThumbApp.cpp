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
#include "gui/Gui_DvrThumbApp.h"
#include "graphics/DvrGraphics.h"

int gui_thumb_app_func(DVR_U32 guiCmd, DVR_U32 param1, DVR_U32 param2)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrGuiResHandler *handler;

	handler = (DvrGuiResHandler *)Dvr_App_GetGuiResHandler();
	if (handler == NULL)
		return DVR_RES_EFAIL;

	switch (guiCmd)
	{
	case GUI_THUMB_APP_CMD_FLUSH:
		res = handler->Flush();
		break;

	case GUI_THUMB_APP_CMD_SET_LAYOUT:
		res = handler->SetLayout(param1);
		break;

	case GUI_THUMB_APP_CMD_HIDE_ALL:
		break;

	case GUI_THUMB_APP_CMD_MAIN_MENU_TAB_UPDATE:
		res = handler->Update(GUI_OBJ_ID_MAIN_MENU_TAB, &param1, param2);
		break;

	case GUI_THUMB_APP_CMD_FRAME_UPDATE:
		res = handler->Update(GUI_OBJ_ID_THUMB_FRAME, (void *)param1, param2);
		break;

	case GUI_THUMB_APP_CMD_STORAGE_CAPACITY_UPDATE:
		res = handler->Update(GUI_OBJ_ID_THUMB_CAPACITY, (void *)param1, param2);
		break;

    case GUI_THUMB_APP_CMD_PAGENUM_UPDATE:
        res = handler->Update(GUI_OBJ_ID_THUMB_PAGE_NUM, (void *)param1, param2);
        break;

	case GUI_THUMB_APP_CMD_TAB_UPDATE:
		res = handler->Update(GUI_OBJ_ID_THUMB_TAB, &param1, param2);
		break;

	case GUI_THUMB_APP_CMD_EDIT_UPDATE:
		res = handler->Update(GUI_OBJ_ID_THUMB_EDIT, &param1, param2);
		break;

	case GUI_THUMB_APP_CMD_EDIT_CHECKBOX_SHOW:
		res = handler->Show(GUI_OBJ_ID_THUMB_EDIT_SEL_CHECKBOX);
		break;

	case GUI_THUMB_APP_CMD_EDIT_CHECKBOX_HIDE:
		res = handler->Hide(GUI_OBJ_ID_THUMB_EDIT_SEL_CHECKBOX);
		break;

	case GUI_THUMB_APP_CMD_EDIT_CHECKBOX_UPDATE:
		res = handler->Update(GUI_OBJ_ID_THUMB_EDIT_SEL_CHECKBOX, (void *)param1, param2);
		break;

	case GUI_THUMB_APP_CMD_WARNING_SHOW:
		res = handler->Show(GUI_OBJ_ID_WARNING);
		break;

	case GUI_THUMB_APP_CMD_WARNING_HIDE:
		res = handler->Hide(GUI_OBJ_ID_WARNING);
		break;

	case GUI_THUMB_APP_CMD_WARNING_UPDATE:
		res = handler->Update(GUI_OBJ_ID_WARNING, &param1, param2);
		break;

	default:
		res = DVR_RES_EFAIL;
		break;
	}

	return res;
}
