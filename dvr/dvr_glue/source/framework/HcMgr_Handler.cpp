/*===========================================================================*\
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
\*===========================================================================*/
#include <string.h>
#include "HcMgr_Handler.h"
#include "DvrAppUtil.h"
#include "flow/widget/DvrWidget_Mgt.h"

#include "flow/entry/DvrEntryApp.h"
#include "flow/rec/DvrRecApp.h"
#include "flow/pb/DvrPbApp.h"
#include "flow/thumb/DvrThumbApp.h"
#include "flow/misc/DvrMiscFormatCard.h"
#include <log/log.h>



HcMgrHandler::HcMgrHandler(void)
{
	m_handler.HandlerMain = AppHandler_Entry;
	m_handler.HandlerExit = AppHandler_Exit;

	memset(DvrAppPool, 0, APPS_MAX_NUM * sizeof(APP_MGT_INST*));

	m_app_main_id = -1;
	m_app_rec_id = -1;
	m_app_pb_id = -1;
	m_app_thumb_id = -1;
	m_app_misc_formatcard_id = -1;
	m_app_misc_defsetting_id = -1;
}

HcMgrHandler::~HcMgrHandler(void)
{
	
}

void HcMgrHandler::AppHandler_Entry(void *ptr)
{
	int AppExit = 0;
	APP_MGT_INST *CurApp = NULL;
	APP_MESSAGE Msg = { 0 };
	APP_MGT_INST *pCurApp = NULL;
	DVR_U32 Param1 = 0, Param2 = 0;

	int res = 0;

	HcMgrHandler *p = (HcMgrHandler *)ptr;

	AppWidget_Init();

	DvrAppMgt_Init(p->DvrAppPool, APPS_MAX_NUM);

	p->m_app_main_id = DvrAppMgt_Register(DvrAppEntry::Get()->GetApp());
	p->m_app_rec_id = DvrAppMgt_Register(DvrRecApp::Get()->GetApp());
	p->m_app_pb_id = DvrAppMgt_Register(DvrPlayBackApp::Get()->GetApp());
	p->m_app_thumb_id = DvrAppMgt_Register(DvrThumbApp::Get()->GetApp());
	p->m_app_misc_formatcard_id = DvrAppMgt_Register(DvrMiscFormatCard::Get()->GetApp());

	DvrAppUtil_SwitchApp(DvrAppUtil_GetStartApp());

	while (AppExit == 0)
	{
		Dvr_Sdk_ComSvcHcMgr_RcvMsg(&Msg, DVR_TIMEOUT_WAIT_FOREVER);
		Param1 = Msg.MessageData[0];
		Param2 = Msg.MessageData[1];
#if 0
		Log_Message("[App - Handler] <Entry> Received msg: 0x%X (Param1 = 0x%X / Param2 = 0x%X)\n", Msg.MessageID, Param1, Param2);

		if (MSG_MDL_ID(Msg.MessageID) == MDL_APP_KEY_ID) {
            Log_Message("[App - Handler] <Entry> Received msg: 0x%X (Param1 = 0x%X / Param2 = 0x%X)\n", Msg.MessageID, Param1, Param2);
		}
#endif

		switch (Msg.MessageID)
		{
		case HMSG_TIMER_CHECK:
		case HMSG_TIMER_1HZ:
		case HMSG_TIMER_2HZ:
		case HMSG_TIMER_4HZ:
		case HMSG_TIMER_10HZ:
		case HMSG_TIMER_20HZ:
		case HMSG_TIMER_5S:
		case HMSG_TIMER_30S:
			Dvr_Sdk_ComSvrTimer_Handler(Param1);
			break;

		case HMSG_STOP_APP_COMPLETE:
            Log_Message("[App - Handler] <Entry> HMSG_STOP_APP_COMPLETE\n");
			Dvr_Sdk_ComSvcHcMgr_DetachHandler();
			break;

		case HMSG_EXIT_HDL:
            Log_Message("[App - Handler] <Entry> Exit handler application!\n");
			AppExit = 1;
			break;

		default:
			DvrAppMgt_GetCurApp(&CurApp);
			CurApp->OnMessage(Msg.MessageID, Param1, Param2);
			break;
		}
	}
}

int HcMgrHandler::AppHandler_Exit(void)
{
	return Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_EXIT_HDL, 0, 0);
}

int HcMgrHandler::GetAppId(int module)
{
	int appId = -1;

	switch (module)
	{
	case MDL_APP_MAIN_ID:
		appId = m_app_main_id;
		break;
	case MDL_APP_RECORDER_ID:
		appId = m_app_rec_id;
		break;
	case MDL_APP_PLAYER_ID:
		appId = m_app_pb_id;
		break;
	case MDL_APP_THUMB_ID:
		appId = m_app_thumb_id;
		break;
	case MDL_APP_FORMATCARD_ID:
		appId = m_app_misc_formatcard_id;
		break;
	default:
		break;
	}

	return appId;
}

const char *HcMgrHandler::GetAppName(int appId)
{
	if (appId == m_app_main_id)
	{
		return "app_main";
	}
	else if (appId == m_app_rec_id)
	{
		return "app_recorder";
	}
	else if (appId == m_app_pb_id)
	{
		return "app_player";
	}
	else if (appId == m_app_thumb_id)
	{
		return "app_thumb";
	}
	else if (appId == m_app_misc_formatcard_id)
	{
		return "app_formatcard";
	}
	else
	{
		return NULL;
	}
}
