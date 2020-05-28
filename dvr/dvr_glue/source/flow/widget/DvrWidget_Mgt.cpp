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
#include <stdlib.h>
#include <string.h>
#include "DVR_APP_DEF.h"
#include "flow/widget/DvrWidget_Mgt.h"
#include "flow/widget/DvrWidget_Menu_Setup.h"
#include "flow/widget/DvrWidget_Dialog.h"
#include "DVR_GUI_OBJ.h"
#include <log/log.h>

/*************************************************************************
* Widget Definitions
************************************************************************/
typedef struct _WIDGET_s_ {
	DVR_U32 Flags;
	DVR_U32 Cur;
	WIDGET_ITEM *Menu_Setup;
	WIDGET_ITEM *Dialog;
} DVR_WIDGET;

static DVR_WIDGET g_widgetInst;
/*************************************************************************
* Widget APIs
************************************************************************/

DVR_RESULT AppWidget_Init(void)
{
	DVR_RESULT res = DVR_RES_SOK;

	memset(&g_widgetInst, 0, sizeof(DVR_WIDGET));
	g_widgetInst.Flags = DVR_WIDGET_NONE;
	g_widgetInst.Menu_Setup = (WIDGET_ITEM *)AppMenuSetup_GetWidget();
	g_widgetInst.Dialog = (WIDGET_ITEM *)AppDialog_GetWidget();

	return 0;
}

DVR_U32 AppWidget_GetCur(void)
{
	return g_widgetInst.Cur;
}

DVR_RESULT AppWidget_On(DVR_U32 widgetId, DVR_U32 param)
{
	DVR_RESULT res = DVR_RES_SOK;

	if (g_widgetInst.Cur) {
		switch (g_widgetInst.Cur) {
		case DVR_WIDGET_MENU_SETUP:
			g_widgetInst.Menu_Setup->Off(0);
			g_widgetInst.Cur = DVR_WIDGET_NONE;
			Log_Message("[App - Widget] <On> Menu is hidden for widget %d\n", widgetId);
			break;
		case DVR_WIDGET_DIALOG:
            Log_Message("[App - Widget] <On> Dialog should be decided!!!\n");
			res = DVR_RES_EFAIL;
			break;
		default:
            Log_Message("[App - Widget] <On> Unknown current widget!!!\n");
			res = DVR_RES_EFAIL;
			break;
		}
	}

	if (res != DVR_RES_SOK) {
		return res;
	}

	switch (widgetId) {
	case DVR_WIDGET_MENU_SETUP:
		APP_ADDFLAGS(g_widgetInst.Flags, DVR_WIDGET_MENU_SETUP);
		g_widgetInst.Menu_Setup->On(0);
		g_widgetInst.Cur = widgetId;
		break;
	case DVR_WIDGET_DIALOG:
		APP_ADDFLAGS(g_widgetInst.Flags, DVR_WIDGET_DIALOG);
		g_widgetInst.Dialog->On(0);
		g_widgetInst.Cur = widgetId;
		break;
	default:
        Log_Error("[App - Widget] <On> Unknown widget!!!\n");
		res = DVR_RES_EFAIL;
		break;
	}

	return res;
}

DVR_RESULT AppWidget_Off(DVR_U32 widgetId, DVR_U32 param)
{
	DVR_RESULT res = DVR_RES_SOK;

	if (widgetId == DVR_WIDGET_ALL) {
		if (APP_CHECKFLAGS(g_widgetInst.Flags, DVR_WIDGET_MENU_SETUP)) {
			APP_REMOVEFLAGS(g_widgetInst.Flags, DVR_WIDGET_MENU_SETUP);
			if (g_widgetInst.Cur == DVR_WIDGET_MENU_SETUP) {
				g_widgetInst.Menu_Setup->Off(0);
				g_widgetInst.Cur = DVR_WIDGET_NONE;
			}
		}
		
		if (APP_CHECKFLAGS(g_widgetInst.Flags, DVR_WIDGET_DIALOG)) {
			APP_REMOVEFLAGS(g_widgetInst.Flags, DVR_WIDGET_DIALOG);
			if (g_widgetInst.Cur == DVR_WIDGET_DIALOG) {
				g_widgetInst.Dialog->OnMessage(DIALOG_SEL_NO, 0, 0);
				g_widgetInst.Dialog->Off(0);
				g_widgetInst.Cur = DVR_WIDGET_NONE;
			}
		}
	}
	else
	{
		if (widgetId == g_widgetInst.Cur) 
		{
			switch (widgetId) {
			case DVR_WIDGET_MENU_SETUP:
				APP_REMOVEFLAGS(g_widgetInst.Flags, DVR_WIDGET_MENU_SETUP);
				g_widgetInst.Menu_Setup->Off(0);
				g_widgetInst.Cur = DVR_WIDGET_NONE;
				break;
			case DVR_WIDGET_DIALOG:
				APP_REMOVEFLAGS(g_widgetInst.Flags, DVR_WIDGET_DIALOG);
				g_widgetInst.Dialog->Off(0);
				g_widgetInst.Cur = DVR_WIDGET_NONE;
				break;
			default:
				Log_Error("[App - Widget] <Off> Unknown widget!!!\n");
				res = -1;
				break;
			}
			if (g_widgetInst.Flags) {
				Log_Message("[App - Widget] <Off> Some other widgets are still ON\n");
				if (APP_CHECKFLAGS(g_widgetInst.Flags, DVR_WIDGET_DIALOG)) {
					AppWidget_On(DVR_WIDGET_DIALOG, 0);
				}
				else if (APP_CHECKFLAGS(g_widgetInst.Flags, DVR_WIDGET_MENU_SETUP)) {
					AppWidget_On(DVR_WIDGET_MENU_SETUP, 0);
				}
			}
		}
		else {
			Log_Error("[App - Widget] <Off> Should not close non-current widget\n");
		}
	}

	return res;
}

DVR_RESULT AppWidget_OnMessage(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2)
{
	DVR_RESULT res = DVR_RES_SOK;

	if (g_widgetInst.Cur) {
		switch (g_widgetInst.Cur) {
		case DVR_WIDGET_MENU_SETUP:
			res = g_widgetInst.Menu_Setup->OnMessage(msg, param1, param2);
			break;
		case DVR_WIDGET_DIALOG:
			res = g_widgetInst.Dialog->OnMessage(msg, param1, param2);
			break;
		default:
			Log_Error("[App - Widget] <OnMessage> Unknown current widget\n");
			res = -1;
			break;
		}
	}

	return res;
}

DVR_U32 AppWidget_GetFlags(void)
{
	return g_widgetInst.Flags;
}
