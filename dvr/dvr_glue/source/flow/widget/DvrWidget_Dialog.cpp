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
#include "DVR_APP_DEF.h"
#include "flow/widget/DvrWidget_Dialog.h"
#include "gui/Gui_Dialog.h"
#include "DVR_GUI_OBJ.h"
#include <log/log.h>

typedef enum 
{
	DIALOG_CUR_SET
} DIALOG_FUNC_ID_e;

static int dialog_func(DVR_U32 funcId, DVR_U32 param1, DVR_U32 param2);

typedef struct _tagWIDGET_DIALOG
{
	DVR_U8 Type;
	DVR_S8 Select;
	DVR_U32 Subject;
	DialogSetHandler Set;
	int(*Func)(DVR_U32 funcId, DVR_U32 param1, DVR_U32 param2);
	int(*Gui)(DVR_U32 guiCmd, DVR_U32 param1, DVR_U32 param2);
} WIDGET_DIALOG;

static WIDGET_DIALOG g_widgetDialog;

static int AppDialog_On(DVR_U32 param);
static int AppDialog_Off(DVR_U32 param);
static int AppDialog_OnMessage(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2);

WIDGET_ITEM WidgetDialog = {
	AppDialog_On,
	AppDialog_Off,
	AppDialog_OnMessage
};

/*************************************************************************
* Dialog APIs
************************************************************************/
static int dialog_func(DVR_U32 funcId, DVR_U32 param1, DVR_U32 param2)
{
	int ReturnValue = 0;

	switch (funcId) {
	case DIALOG_CUR_SET:
		if (g_widgetDialog.Type == GUI_DIALOG_TYPE_OK) {
			g_widgetDialog.Select = DIALOG_SEL_OK;
		}
		else
		{
			g_widgetDialog.Select = param1;
		}
		g_widgetDialog.Set(g_widgetDialog.Select, 0, 0);
		break;
	default:
		Log_Error("[App - Dialog] <Func> The function is undefined\n");
		break;
	}

	return ReturnValue;
}

static int AppDialog_On(DVR_U32 param)
{
	DVR_RESULT res = DVR_RES_SOK;

	g_widgetDialog.Func = dialog_func;
	g_widgetDialog.Gui = gui_dialog_func;

	/** Show Dialog frame */
	g_widgetDialog.Gui(GUI_DIALOG_CMD_SHOW, 0, 0);

	if (g_widgetDialog.Type == GUI_DIALOG_TYPE_OK) {
		g_widgetDialog.Select = DIALOG_SEL_OK;
	}
	else {
		g_widgetDialog.Select = DIALOG_SEL_NO;
	}

	GUI_OBJ_DIALOG_INST dialogInst;
	dialogInst.type = (GUI_DIALOG_TYPE)g_widgetDialog.Type;
	dialogInst.subjectId = (DIALOG_SUBJECT_ID)g_widgetDialog.Subject;

	g_widgetDialog.Gui(GUI_DIALOG_CMD_UPDATE, (DVR_U32)&dialogInst, sizeof(GUI_OBJ_DIALOG_INST));
	g_widgetDialog.Gui(GUI_DIALOG_CMD_FLUSH, 0, 0);

	return res;
}

static int AppDialog_Off(DVR_U32 param)
{
	DVR_RESULT res = DVR_RES_SOK;

	g_widgetDialog.Gui(GUI_DIALOG_CMD_HIDE, 0, 0);
	g_widgetDialog.Gui(GUI_DIALOG_CMD_FLUSH, 0, 0);

	return res;
}

static int AppDialog_OnMessage(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2)
{
	DVR_RESULT res = DVR_RES_SOK;

	switch (msg) {
	case HMSG_USER_CMD_DIALOG_SET:
		AppWidget_Off(DVR_WIDGET_DIALOG, 0);
		g_widgetDialog.Func(DIALOG_CUR_SET, param1, param2);
		break;
	default:
		res = DVR_RES_SOK;
		break;
	}

	return res;
}

WIDGET_ITEM* AppDialog_GetWidget(void)
{
	return &WidgetDialog;
}

int AppDialog_SetDialog(DVR_U32 type, DVR_U32 subject, DialogSetHandler set)
{
	g_widgetDialog.Type = type;
	g_widgetDialog.Subject = subject;
	g_widgetDialog.Set = set;

	return DVR_RES_SOK;
}

