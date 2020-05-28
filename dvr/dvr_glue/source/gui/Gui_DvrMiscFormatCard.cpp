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
#include "gui/Gui_DvrMiscFormatCard.h"
#include "graphics/DvrGraphics.h"
#include <log/log.h>

int gui_misc_formatcard_func(DVR_U32 guiCmd, DVR_U32 param1, DVR_U32 param2)
{
	DVR_RESULT res = 0;
	DvrGuiResHandler *handler;

	handler = (DvrGuiResHandler *)Dvr_App_GetGuiResHandler();
	if (handler == NULL)
		return DVR_RES_EFAIL;

	switch (guiCmd) {
	case GUI_MISC_FORMATCARD_APP_CMD_FLUSH:
		res = handler->Flush();
		break;

	case GUI_MISC_FORMATCARD_APP_CMD_WARNING_UPDATE:
		res = handler->Update(GUI_OBJ_ID_WARNING, &param1, param2);
		break;

	case GUI_MISC_FORMATCARD_APP_CMD_WARNING_SHOW:
		res = handler->Show(GUI_OBJ_ID_WARNING);
		break;

	case GUI_MISC_FORMATCARD_APP_CMD_WARNING_HIDE:
		res = handler->Hide(GUI_OBJ_ID_WARNING);
		break;

	default:
		Log_Error("[Gui - DvrMiscFormatCard] <Func> Undefined GUI command\n");
		res = DVR_RES_EFAIL;
		break;
	}

	return res;
}
