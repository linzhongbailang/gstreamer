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
#include "flow/pb/DvrPbApp.h"
#include "flow/widget/DvrWidget_Mgt.h"
#include "gui/Gui_DvrPbApp.h"
#include "flow/rec/DvrRecApp.h"
#include <log/log.h>

int pb_app_func(DVR_U32 funcId, DVR_U32 param1, DVR_U32 param2);

DVR_RESULT app_pb_start(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();

	app->Func = pb_app_func;
	app->Gui = gui_pb_app_func;

	if (!APP_CHECKFLAGS(app->m_playback.GFlags, APP_AFLAGS_INIT)) {
		APP_ADDFLAGS(app->m_playback.GFlags, APP_AFLAGS_INIT);
		app->Func(PB_APP_FUNC_INIT, 0, 0);
	}

	if (APP_CHECKFLAGS(app->m_playback.GFlags, APP_AFLAGS_START)) {
		app->Func(PB_APP_FUNC_START_FLAG_ON, 0, 0);
	}
	else {
		APP_ADDFLAGS(app->m_playback.GFlags, APP_AFLAGS_START);

		app->Func(PB_APP_FUNC_START, 0, 0);
		app->Func(PB_APP_FUNC_CHANGE_DISPLAY, 0, 0);
		app->Func(PB_APP_FUNC_APP_READY, 0, 0);
	}

	return res;
}

DVR_RESULT app_pb_stop(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();

	if (!APP_CHECKFLAGS(app->m_playback.GFlags, APP_AFLAGS_READY)) 
	{
		app->Func(PB_APP_FUNC_STOP, 0, 0);
	}

	return res;
}

DVR_RESULT app_pb_on_message(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();

	res = AppWidget_OnMessage(msg, param1, param2);
	if (res != DVR_RES_SOK)
	{
		return res;
	}

	switch (msg)
	{
	case HMSG_USER_CMD_MAIN_MENU_TAB:		
		res = app->Func(PB_APP_FUNC_SWITCH_APP_BY_TAB, param1, param2);
		break;

	case AMSG_CMD_APP_READY:
		res = app->Func(PB_APP_FUNC_APP_READY, 0, 0);
		break;
	
	case HMSG_USER_CMD_PLAYER_SCREEN_SHOT:
		if (!APP_CHECKFLAGS(app->m_playback.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		res = app->Func(PB_APP_FUNC_SCREEN_SHOT, param1, param2);
		break;
	case HMSG_USER_CMD_POWER_MODE:
		{
			DvrRecApp *rec_app = DvrRecApp::Get();
			if(rec_app != NULL && rec_app->Func != NULL){
				if(param1 == 1) {
					rec_app->bPowerMode = 1;
					if(rec_app->bUserEnableRec==1)
						res = rec_app->Func(REC_APP_FUNC_LOOP_RECORD_SWITCH, 1, 0);
				}
				else if(param1 == 0){
					rec_app->bPowerMode = 0;
					res = rec_app->Func(REC_APP_FUNC_LOOP_RECORD_SWITCH, 0, 0);
				}
				Log_Message("thumb app rec HMSG_USER_CMD_POWER_MODE app->bPowerMode %d\n",rec_app->bPowerMode); 		
			}
		}
		break;
	case HMSG_USER_CMD_RECORDER_SNAPSHOT_SET:
		{
			DvrRecApp *rec_app = DvrRecApp::Get();
			if(rec_app != NULL && rec_app->Func != NULL){
				rec_app->m_bSnapShotEnable=param1;
			Log_Warning("thumb app m_bSnapShotEnable %d\n",rec_app->m_bSnapShotEnable); 	}
		}
		break;
	

	case HMSG_USER_CMD_PLAYER_QUIT:
		if (!APP_CHECKFLAGS(app->m_playback.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		res = app->Func(PB_APP_FUNC_QUIT, param1, param2);
		break;	

	case HMSG_USER_CMD_PLAYER_DEL:
		if (!APP_CHECKFLAGS(app->m_playback.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
#if 0	
		res = app->Func(PB_APP_FUNC_DELETE_FILE_DIALOG_SHOW, 0, 0);
#else
		res = app->Func(PB_APP_FUNC_DELETE_FILE, 0, 0);
#endif
		break;

	case HMSG_USER_CMD_PLAYER_COPY:
		if (!APP_CHECKFLAGS(app->m_playback.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		res = app->Func(PB_APP_FUNC_SAVE_FILE, param1, param2);
		break;

	case HMSG_USER_CMD_PLAYER_MODE_CHANGE:
		if (!APP_CHECKFLAGS(app->m_playback.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		res = app->Func(PB_APP_FUNC_CHANGE_MODE, 0, 0);
		break;

	case HMSG_USER_CMD_PLAYER_PLAY:
		if (!APP_CHECKFLAGS(app->m_playback.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		if (app->playState == PB_APP_STATE_PLAYER_PAUSE)
		{
			res = app->Func(PB_APP_FUNC_RESUME_PLAY, 0, 0);
		}
		else
		{
			//error state
		}
		break;

	case HMSG_USER_CMD_PLAYER_PAUSE:
		if (!APP_CHECKFLAGS(app->m_playback.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		if (app->playState == PB_APP_STATE_PLAYER_RUNNING)
		{
			res = app->Func(PB_APP_FUNC_PAUSE_PLAY, 0, 0);
		}
		else
		{
			//error state
		}
		break;

	case HMSG_USER_CMD_PLAYER_STOP:
		if (!APP_CHECKFLAGS(app->m_playback.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		res = app->Func(PB_APP_FUNC_STOP_PLAY, 0, 0);
		break;

    case HMSG_AUTO_PLAY_NEXT:
	case HMSG_USER_CMD_PLAYER_NEXT:
		if (!APP_CHECKFLAGS(app->m_playback.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		res = app->Func(PB_APP_FUNC_NEXT_FILE, 0, 0);
		break;

    case HMSG_AUTO_PLAY_PREV:
	case HMSG_USER_CMD_PLAYER_PREV:
		if (!APP_CHECKFLAGS(app->m_playback.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		res = app->Func(PB_APP_FUNC_PREV_FILE, 0, 0);
		break;
	
	case HMSG_USER_CMD_PLAYER_SPEED:
		if (!APP_CHECKFLAGS(app->m_playback.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		res = app->Func(PB_APP_FUNC_SET_SPEED, param1, 0);
		break;

	case HMSG_USER_CMD_PLAYER_SEEK:
		if (!APP_CHECKFLAGS(app->m_playback.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		res = app->Func(PB_APP_FUNC_SET_POSITION, param1, 0);
		break;

	case HMSG_USER_CMD_PLAYER_VIEW_INDEX:
		if (!APP_CHECKFLAGS(app->m_playback.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		res = app->Func(PB_APP_FUNC_SET_VIEW_INDEX, param1, 0);
		break;

	case AMSG_STATE_CARD_INSERT_ACTIVE:
		if (APP_CHECKFLAGS(app->m_playback.GFlags, APP_AFLAGS_READY)) {
			res = app->Func(PB_APP_FUNC_CARD_INSERT, param1, param2);
		}
		break;

	case AMSG_STATE_CARD_REMOVED:
		if (APP_CHECKFLAGS(app->m_playback.GFlags, APP_AFLAGS_READY)) {
			res = app->Func(PB_APP_FUNC_CARD_REMOVE, param1, param2);
		}
		break;

    case AMSG_CMD_CARD_FORMAT:
        res = app->Func(PB_APP_FUNC_CARD_FORMAT, param1, param2);
        break;	

    case HMSG_USER_CMD_SIDEBAR_SET:
        res = app->Func(PB_APP_FUNC_SET_SIDEBAR_STATUS, param1, param2);
        break;

	case HMSG_USER_CMD_PLAYER_OPEN_RESULT:
		res = app->Func(PB_APP_FUNC_DECODE_ERROR, param1, param2);
		break;

	case HMSG_USER_CMD_PLAYER_FORWORD:
		res = app->Func(PB_APP_FUNC_FORWARD_PLAY, param1, param2);
		break;

    case HMSG_USER_CMD_PLAYER_REWIND:
        res = app->Func(PB_APP_FUNC_REWIND_PLAY, param1, param2);
        break;

	default:
		break;
	}

	return res;
}

DvrPlayBackApp::DvrPlayBackApp()
{
	playState = PB_APP_STATE_PLAYER_IDLE;
	m_bHasInited = FALSE;
}

DVR_BOOL DvrPlayBackApp::Init()
{
	if (m_bHasInited)
	{
		return TRUE;
	}

	m_playback.Id = 0;
	m_playback.Tier = 1;
	m_playback.Parent = 0;
	m_playback.Previous = 0;
	m_playback.Child = 0;
	m_playback.GFlags = 0;
	m_playback.Flags = 0;
	m_playback.Start = app_pb_start;
	m_playback.Stop = app_pb_stop;
	m_playback.OnMessage = app_pb_on_message;

	m_bHasInited = TRUE;

	return m_bHasInited;
}

DvrPlayBackApp *DvrPlayBackApp::Get(void)
{
	static DvrPlayBackApp playback;

	if (playback.Init())
	{
		return &playback;
	}

	return NULL;
}
