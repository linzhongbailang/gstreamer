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
#include "DVR_APP_INTFS.h"
#include "framework/DvrAppMain.h"
#include "framework/DvrAppUtil.h"
#include "framework/DvrAppMgt.h"
#include "framework/DvrAppNotify.h"
#include <log/log.h>

#include "Gpu_Dvr_Interface.h"

static int startDvrMode = 1; //defalut in record mode
static int eDvrLoopRecordingPeriod = GUI_SETUP_SPLIT_TIME_3MIN;
static int eDvrLangeuageSel = 0;

DVR_RESULT DvrAppUtil_ReadyCheck(void)
{
	int res = 0;

	/*Check unsaved video status*/
	//TODO

	/*Check card format parameter*/
	//TODO

	/**Check app switch*/
	APP_STATUS *pStatus = (APP_STATUS *)Dvr_App_GetStatus();
	if (pStatus->AppSwitchBlocked > 0) {
		DvrAppUtil_SwitchApp(pStatus->AppSwitchBlocked);
		pStatus->AppSwitchBlocked = 0;
		return DVR_RES_SOK;
	}
	
	/*Check card insert block*/
// 	char *pDrive = NULL;
// 	Dvr_Sdk_GetActiveDrive(&pDrive);
// 	if (pDrive != NULL)
// 	{
// 		APP_MGT_INST *CurApp = NULL;
// 
// 		DvrAppMgt_GetCurApp(&CurApp);
// 		CurApp->OnMessage(AMSG_CMD_CARD_UPDATE_ACTIVE_CARD, 0, 0);
// 	}
// 	else
// 	{
// 
// 	}

	return DVR_RES_SOK;
}

DVR_RESULT DvrAppUtil_SwitchApp(int appId)
{
	int res = 0;
	APP_MGT_INST *CurApp = NULL;
	APP_MGT_INST *NewApp = NULL;

	if (appId < 0) {
		Log_Error("<SwitchApp> This app is not registered");
		return -1;
	}

	{
        DvrAppNotify *pHandle = DvrAppNotify::Get();
        if (pHandle != NULL)
            pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_APP_SWTICH, &appId, NULL);
    }

	DvrAppMgt_GetCurApp(&CurApp);
	DvrAppMgt_GetApp(&NewApp, appId);

	if (DvrAppMgt_CheckIdle() || // app is idle
		(CurApp->Tier < NewApp->Tier) || // new app is descendant
		APP_CHECKFLAGS(NewApp->GFlags, APP_AFLAGS_READY)) // new app is ancestor
	{ 
	    Log_Message("DvrAppMgt_SwitchApp %s ",Dvr_App_GetAppName(appId));
		DvrAppMgt_SwitchApp(appId);
	}
	else {
		Log_Message("<SwitchApp> App switch target appId = %d is blocked\n", appId);

		APP_STATUS *pStatus = (APP_STATUS *)Dvr_App_GetStatus();
		pStatus->AppSwitchBlocked = appId;

		res = DvrAppMgt_CheckBusy();
		if (res) {
            Log_Message("<SwitchApp> App switch is blocked by busy appId = %d\n", res);
			CurApp->OnMessage(AMSG_CMD_SWITCH_APP, appId, 0);
		}
	}

	return res;
}

int DvrAppUtil_GetStartApp(void)
{
	int appId;

	Log_Message("Get Start DVR Mode = %d!!!!!", startDvrMode);
	
	switch(startDvrMode)
	{
		case 0:
		{
			appId = Dvr_App_GetAppId(MDL_APP_MAIN_ID);
		}
		break;
		
		case 1:
		{
			appId = Dvr_App_GetAppId(MDL_APP_RECORDER_ID);
		}
		break;

		case 2:
		{
		#if 0
			appId = Dvr_App_GetAppId(MDL_APP_THUMB_ID);
		#else
			appId = Dvr_App_GetAppId(MDL_APP_RECORDER_ID);
			Dvr_App_HuSendCmd(HMSG_USER_CMD_MAIN_MENU_TAB, GUI_MAIN_MENU_TAB_THUMB, sizeof(int));
		#endif
		}
		break;

		case 3:
		{
			appId = Dvr_App_GetAppId(MDL_APP_RECORDER_ID);
			Dvr_App_HuSendCmd(HMSG_USER_CMD_MAIN_MENU_TAB, GUI_MAIN_MENU_TAB_SETUP, sizeof(int));
		}
		break;

		default:
		{
			appId = Dvr_App_GetAppId(MDL_APP_RECORDER_ID);
		}
		break;
	}

	return appId;
}

int DvrAppUtil_SetStartApp(int eDvrMode)
{
	startDvrMode = eDvrMode;

	Log_Message("Set Start DVR Mode = %d!!!!!", startDvrMode);

#if __linux__
	DVR_GUI_LAYOUT_INST_EXT RenderData;

	switch(startDvrMode)
	{
		case 0:
		case 1:
		{
			RenderData.curLayout = GUI_LAYOUT_RECORD_EXT;
			DvrGui_GetLayoutInfo(GUI_LAYOUT_RECORD, (DVR_GRAPHIC_UIOBJ **)&RenderData.pTable, &RenderData.ObjNum);			
			UpdateRenderDvrData((DVR_GUI_LAYOUT_INST_EXT*)&RenderData, sizeof(DVR_GUI_LAYOUT_INST_EXT));
		}
		break;

		case 2:
		{
			RenderData.curLayout = GUI_LAYOUT_THUMB_EXT;
			DvrGui_GetLayoutInfo(GUI_LAYOUT_THUMB, (DVR_GRAPHIC_UIOBJ **)&RenderData.pTable, &RenderData.ObjNum); 			
			UpdateRenderDvrData((DVR_GUI_LAYOUT_INST_EXT*)&RenderData, sizeof(DVR_GUI_LAYOUT_INST_EXT));
		}
		break;

		default:
			break;
	}
#endif
	
	return 0;
}

int DvrAppUtil_SetLanguageSel(int eLangSel)
{
	eDvrLangeuageSel = eLangSel;
	return 0;
}

int DvrAppUtil_GetLanguageSel(int *pLangSel)
{
	if(pLangSel != NULL)
	{
		*pLangSel = eDvrLangeuageSel;
	}

	return 0;
}

int DvrAppUtil_SetRecorderSplitTime(int eSplitTime)
{
	eDvrLoopRecordingPeriod = eSplitTime;
	return 0;
}

int DvrAppUtil_GetRecorderSplitTime(int *pSplitTime)
{
	if(pSplitTime != NULL)
	{
		*pSplitTime = eDvrLoopRecordingPeriod;
	}

	return 0;
}

