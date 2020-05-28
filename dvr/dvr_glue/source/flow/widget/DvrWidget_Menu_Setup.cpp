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
#include "DVR_APP_INTFS.h"
#include "DVR_GUI_OBJ.h"
#include "DVR_SDK_INTFS.h"
#include "flow/widget/DvrWidget_Menu_Setup.h"
#include "gui/Gui_Menu_Setup.h"
#include "framework/DvrAppUtil.h"
#include "framework/DvrAppMgt.h"
#include "framework/DvrAppNotify.h"
#include <log/log.h>

/** Menu status */
typedef struct
{
	int(*Func)(DVR_U32 funcId, DVR_U32 param1, DVR_U32 param2);
	int(*Gui)(DVR_U32 guiCmd, DVR_U32 param1, DVR_U32 param2);
} WIDGET_MENU_SETUP;

static WIDGET_MENU_SETUP g_widgetMenu_Setup;

/** Menu interface */
static int app_menu_setup_on(DVR_U32 param);
static int app_menu_setup_off(DVR_U32 param);
static int app_menu_setup_on_message(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2);

WIDGET_ITEM widget_menu_setup = {
	app_menu_setup_on,
	app_menu_setup_off,
	app_menu_setup_on_message
};

/*************************************************************************
* Menu APIs
************************************************************************/
/** Menu functions */

static int menu_setup_func(DVR_U32 funcId, DVR_U32 param1, DVR_U32 param2)
{
	int res = 0;

	return res;
}

static int app_menu_setup_on(DVR_U32 param)
{
	int res = 0;
	int i = 0;

	g_widgetMenu_Setup.Func = menu_setup_func;
	g_widgetMenu_Setup.Gui = gui_menu_setup_func;

	g_widgetMenu_Setup.Gui(GUI_MENU_SETUP_CMD_SET_LAYOUT, GUI_LAYOUT_SETUP, 0);
	g_widgetMenu_Setup.Gui(GUI_MENU_SETUP_CMD_MAIN_MENU_TAB_UPDATE, GUI_MAIN_MENU_TAB_SETUP, 4);
	g_widgetMenu_Setup.Gui(GUI_MENU_SETUP_CMD_FLUSH, 0, 0);

	res = Dvr_Sdk_StorageCard_CheckStatus();
	if(res == DVR_STORAGE_CARD_STATUS_NO_CARD)
	{
		DVR_U32 DVR_FormatStatus = 0x6; //can't format
		DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_FORMAT_STATUS, (void *)&DVR_FormatStatus, NULL);
	}
	
	return res;
}

static int app_menu_setup_off(DVR_U32 param)
{
	return 0;
}

static int app_menu_setup_on_message(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2)
{
	int res = DVR_RES_SOK;

	switch (msg) {
    case HMSG_USER_CMD_RECORDER_LOOPREC_SPLIT_TIME:
		res = g_widgetMenu_Setup.Gui(GUI_MENU_SETUP_CMD_LOOPENC_SPLIT_TIME_UPDATE, param1, param2);
		res |= g_widgetMenu_Setup.Gui(GUI_MENU_SETUP_CMD_FLUSH, 0, 0);
		break;

	case HMSG_USER_CMD_RECORDER_SET_VIDEO_QUALITY:
		res = g_widgetMenu_Setup.Gui(GUI_MENU_SETUP_CMD_VIDEO_QUALITY_UPDATE, param1, param2);
		res |= g_widgetMenu_Setup.Gui(GUI_MENU_SETUP_CMD_FLUSH, 0, 0);
		break;

	case HMSG_USER_CMD_RECORDER_SET_PHOTO_QUALITY:
		res = g_widgetMenu_Setup.Gui(GUI_MENU_SETUP_CMD_PHOTO_QUALITY_UPDATE, param1, param2);
		res |= g_widgetMenu_Setup.Gui(GUI_MENU_SETUP_CMD_FLUSH, 0, 0);
		break;

	case HMSG_USER_CMD_FORMAT_CARD:
	{
		res = Dvr_Sdk_StorageCard_CheckStatus();
		if (res == DVR_STORAGE_CARD_STATUS_NO_CARD) {
			Log_Error("[App - MenuSetup] <FormatSet> WARNING_NO_CARD\n");
		}
		else {
			res = DvrAppUtil_SwitchApp(Dvr_App_GetAppId(MDL_APP_FORMATCARD_ID));
		}
	}
	break;

	default:
		//res = DVR_RES_EFAIL;
		break;
	}

	return res;
}

WIDGET_ITEM* AppMenuSetup_GetWidget(void)
{
	return &widget_menu_setup;
}
