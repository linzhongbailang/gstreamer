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
#include "DVR_APP_DEF.h"
#include "DVR_SDK_DEF.h"
#include "flow/rec/DvrRecApp.h"
#include "flow/widget/DvrWidget_Mgt.h"
#include "gui/Gui_DvrRecApp.h"
#include <log/log.h>

int rec_app_func(DVR_U32 funcId, DVR_U32 param1, DVR_U32 param2);

DVR_RESULT app_rec_start(void)
{
	DvrRecApp *app = DvrRecApp::Get();

	app->Func = rec_app_func;
	app->Gui = gui_rec_app_func;

    Log_Message("app_rec_start\n");
    
	if (!APP_CHECKFLAGS(app->m_recorder.GFlags, APP_AFLAGS_INIT)) {
		APP_ADDFLAGS(app->m_recorder.GFlags, APP_AFLAGS_INIT);
		app->Func(REC_APP_FUNC_INIT, 0, 0);
	}

	if (APP_CHECKFLAGS(app->m_recorder.GFlags, APP_AFLAGS_START)) {
		app->Func(REC_APP_FUNC_START_FLAG_ON, 0, 0);
		Log_Message("REC_APP_FUNC_START_FLAG_ON\n");
	}
	else {
		APP_ADDFLAGS(app->m_recorder.GFlags, APP_AFLAGS_START);
		Log_Message("REC_APP_FUNC_START\n");
		app->Func(REC_APP_FUNC_START, 0, 0);
	}

	return DVR_RES_SOK;
}

DVR_RESULT app_rec_stop(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrRecApp *app = DvrRecApp::Get();

	if (!APP_CHECKFLAGS(app->m_recorder.GFlags, APP_AFLAGS_READY)) {
		app->Func(REC_APP_FUNC_STOP, 0, 0);
	}

	return res;
}

DVR_RESULT app_rec_on_message(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrRecApp *app = DvrRecApp::Get();

	res = AppWidget_OnMessage(msg, param1, param2);
	if (res != DVR_RES_SOK)
	{
		return res;
	}

	switch (msg)
	{
	case HMSG_USER_CMD_RECORDER_ONOFF:
		if (!APP_CHECKFLAGS(app->m_recorder.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		if (app->bPowerMode==0) {
			Log_Message("rec HMSG_USER_CMD_RECORDER_ONOFF app->bPowerMode %d\n",app->bPowerMode);
			break;    // disable 
		}

		if(param2 == 0) {
			app->bUserEnableRec = param1;
		}
		
		res = app->Func(REC_APP_FUNC_LOOP_RECORD_SWITCH, param1, param2);
		break;
	case HMSG_USER_CMD_POWER_MODE:
		if (!APP_CHECKFLAGS(app->m_recorder.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}

		if(param1 == 1) {
			app->bPowerMode = 1;
			if(app->bUserEnableRec==1)
				res = app->Func(REC_APP_FUNC_LOOP_RECORD_SWITCH, 1, 0);
		}
		else if(param1 == 0){
			app->bPowerMode = 0;
			res = app->Func(REC_APP_FUNC_LOOP_RECORD_SWITCH, 0, 0);
		}
		Log_Message("rec HMSG_USER_CMD_POWER_MODE app->bPowerMode %d\n",app->bPowerMode);
		break;

	case HMSG_USER_CMD_RECORDER_EVENT_RECORD:
		if (!APP_CHECKFLAGS(app->m_recorder.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		res = app->Func(REC_APP_FUNC_EVENT_RECORD_START, param1, param2);
		break;

    case AMSG_CMD_EVENT_RECORDER_EMERGENCY_COMPLETE:
        res = app->Func(REC_APP_FUNC_EVENT_RECORD_COMPLETE, param1, param2);
        break;

	case AMSG_CMD_EVENT_RECORDER_ALARM_COMPLETE:
		res = app->Func(REC_APP_FUNC_ALARM_RECORD_COMPLETE, param1, param2);
		break;

	case HMSG_USER_CMD_RECORDER_DAS_RECORD_START:
		if (!APP_CHECKFLAGS(app->m_recorder.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		res = app->Func(REC_APP_FUNC_DAS_RECORD_START, param1, param2);
		break;

    case HMSG_USER_CMD_RECORDER_DAS_RECORD_STOP:
        res = app->Func(REC_APP_FUNC_DAS_RECORD_COMPLETE, param1, param2);
        break;

    case HMSG_USER_CMD_RECORDER_IACC_RECORD:
        if (!APP_CHECKFLAGS(app->m_recorder.GFlags, APP_AFLAGS_READY)) {
            break;    // not ready
        }
        res = app->Func(REC_APP_FUNC_IACC_RECORD_START, param1, param2);
        break;

    case AMSG_CMD_EVENT_RECORDER_IACC_COMPLETE:
        if (!APP_CHECKFLAGS(app->m_recorder.GFlags, APP_AFLAGS_READY)) {
            break;    // not ready
        }
        res = app->Func(REC_APP_FUNC_IACC_RECORD_COMPLETE, param1, param2);
        break;

	case HMSG_USER_CMD_RECORDER_VIEW_INDEX:
		res = app->Func(REC_APP_FUNC_SET_VIEW_INDEX, param1, param2);
		break;

    case HMSG_USER_CMD_RECORDER_LOOPREC_SPLIT_TIME:
		res = app->Func(REC_APP_FUNC_SET_LOOPREC_SPLIT_TIME, param1, param2);
		break;

	case HMSG_USER_CMD_RECORDER_SNAPSHOT_SET:
		if (!APP_CHECKFLAGS(app->m_recorder.GFlags, APP_AFLAGS_READY)) {
			break;	  // not ready
		}
		app->Func(REC_APP_FUNC_SNAPSHOT_SET, param1, param2);
		break;

	case HMSG_USER_CMD_RECORDER_SNAPSHOT:
		if (!APP_CHECKFLAGS(app->m_recorder.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		app->Func(REC_APP_FUNC_SNAPSHOT, param1, param2);
		break;

	case AMSG_STATE_CARD_INSERT_ACTIVE:
		res = app->Func(REC_APP_FUNC_CARD_INSERT, param1, param2);
		break;

	case AMSG_CMD_SWITCH_APP:
		res = app->Func(REC_APP_FUNC_SWITCH_APP_BY_APPID, param1, param2);
		break;

	case AMSG_STATE_CARD_REMOVED:
		if (APP_CHECKFLAGS(app->m_recorder.GFlags, APP_AFLAGS_READY)) {
			res = app->Func(REC_APP_FUNC_CARD_REMOVE, param1, param2);
		}
		break;

    case AMSG_CMD_CARD_FORMAT:
        res = app->Func(REC_APP_FUNC_CARD_FORMAT, param1, param2);
        break;

	case HMSG_USER_CMD_RECORDER_SET_VIDEO_QUALITY:
		res = app->Func(REC_APP_FUNC_SET_VIDEO_QUALITY, param1, param2);
		break;

	case HMSG_USER_CMD_RECORDER_SET_PHOTO_QUALITY:
		res = app->Func(REC_APP_FUNC_SET_PHOTO_QUALITY, param1, param2);
		break;

	case HMSG_USER_CMD_MAIN_MENU_TAB:
		res = app->Func(REC_APP_FUNC_SWITCH_APP_BY_TAB, param1, param2);
		break;

	case HMSG_STORAGE_BUSY:
		if (APP_CHECKFLAGS(app->m_recorder.GFlags, APP_AFLAGS_READY)) {
			res = app->Func(REC_APP_FUNC_CARD_STORAGE_BUSY, param1, param2);
		}
		break;

	case HMSG_STORAGE_IDLE:
		if (APP_CHECKFLAGS(app->m_recorder.GFlags, APP_AFLAGS_READY)) {
			res = app->Func(REC_APP_FUNC_CARD_STORAGE_IDLE, param1, param2);
		}
		break;

	case HMSG_STORAGE_TOO_FRAGMENTED:
		if (APP_CHECKFLAGS(app->m_recorder.GFlags, APP_AFLAGS_READY)) {
			res = app->Func(REC_APP_FUNC_ERROR_STORAGE_FRAGMENT_ERR, param1, param2);
		}
		break;

	case HMSG_STORAGE_RUNOUT:
		if (!APP_CHECKFLAGS(app->m_recorder.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		res = app->Func(REC_APP_FUNC_ERROR_STORAGE_RUNOUT, param1, param2);
		break;

	case HMSG_STORAGE_RUNOUT_HANDLE_DONE:
		res = app->Func(REC_APP_FUNC_ERROR_STORAGE_RUNOUT_HANDLE_DONE, param1, param2);
		break;

	case HMSG_STORAGE_RUNOUT_HANDLE_ERROR:
		res = app->Func(REC_APP_FUNC_ERROR_STORAGE_RUNOUT_HANDLE_ERROR, param1, param2);
		break;

    case HMSG_USER_CMD_SIDEBAR_SET:
        res = app->Func(REC_APP_FUNC_SET_SIDEBAR_STATUS, param1, param2);
        break;

	case ASYNC_MGR_MSG_CARD_FORMAT_DONE:
		res = app->Func(REC_APP_FUNC_CARD_FORMAT_DONE, param1, param2);
		break;

	case HMSG_USER_CMD_RECORDER_ALARM_RECORD_START:
		if (!APP_CHECKFLAGS(app->m_recorder.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		res = app->Func(REC_APP_FUNC_ALARM_RECORD_START, param1, param2);		
		break;
				
	default:
		break;
	}

	return res;
}

DvrRecApp::DvrRecApp()
{
	RecCapState = REC_CAP_STATE_PREVIEW;
	m_bHasInited = FALSE;
	m_bSnapShotEnable=TRUE;
}

DVR_BOOL DvrRecApp::Init()
{
	if (m_bHasInited)
	{
		return TRUE;
	}

	m_recorder.Id = 0;
	m_recorder.Tier = 1;
	m_recorder.Parent = 0;
	m_recorder.Previous = 0;
	m_recorder.Child = 0;
	m_recorder.GFlags = 0;
	m_recorder.Flags = 0;
	m_recorder.Start = app_rec_start;
	m_recorder.Stop = app_rec_stop;
	m_recorder.OnMessage = app_rec_on_message;

	m_bHasInited = TRUE;

	return m_bHasInited;
}

DvrRecApp *DvrRecApp::Get(void)
{
	static DvrRecApp recorder;

	if (recorder.Init())
	{
		return &recorder;
	}

	return NULL;
}
