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
#include "flow/thumb/DvrThumbApp.h"
#include "flow/widget/DvrWidget_Mgt.h"
#include "gui/Gui_DvrThumbApp.h"
#include <log/log.h>

#include "flow/rec/DvrRecApp.h"

int thumb_app_func(DVR_U32 funcId, DVR_U32 param1, DVR_U32 param2);

DVR_RESULT app_thumb_start(void)
{
	DvrThumbApp *app = DvrThumbApp::Get();

	app->Func = thumb_app_func;
	app->Gui = gui_thumb_app_func;
    
    Log_Message("app_thumb_start\n");
    
	if (!APP_CHECKFLAGS(app->m_thumb.GFlags, APP_AFLAGS_INIT)) {
		APP_ADDFLAGS(app->m_thumb.GFlags, APP_AFLAGS_INIT);
		app->Func(THUMB_APP_FUNC_INIT, 0, 0);
	}

	if (APP_CHECKFLAGS(app->m_thumb.GFlags, APP_AFLAGS_START)) {
		app->Func(THUMB_APP_FUNC_START_FLAG_ON, 0, 0);
		app->Func(THUMB_APP_FUNC_CHANGE_DISPLAY, 0, 0);
        app->Func(THUMB_APP_FUNC_START_DISP_PAGE, TRUE, 0);
	}
	else {
		APP_ADDFLAGS(app->m_thumb.GFlags, APP_AFLAGS_START);
		app->Func(THUMB_APP_FUNC_START, 0, 0);
		app->Func(THUMB_APP_FUNC_CHANGE_DISPLAY, 0, 0);
		app->Func(THUMB_APP_FUNC_APP_READY, 0, 0);
	}

	return DVR_RES_SOK;
}

DVR_RESULT app_thumb_stop(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrThumbApp *app = DvrThumbApp::Get();

	if (!APP_CHECKFLAGS(app->m_thumb.GFlags, APP_AFLAGS_READY)) {
		app->Func(THUMB_APP_FUNC_STOP, 0, 0);
	}

	return res;
}

DVR_RESULT app_thumb_on_message(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrThumbApp *app = DvrThumbApp::Get();

    if (APP_CHECKFLAGS(app->m_thumb.GFlags, APP_AFLAGS_BUSY))
    {
        return DVR_RES_SOK;
    }

	res = AppWidget_OnMessage(msg, param1, param2);
	if (res != DVR_RES_SOK)
	{
		return res;
	}

	switch (msg)
	{
	case AMSG_CMD_APP_READY:
		app->Func(THUMB_APP_FUNC_APP_READY, 0, 0);
		break;

	case HMSG_USER_CMD_THUMB_TAB:
		if (!APP_CHECKFLAGS(app->m_thumb.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		res = app->Func(THUMB_APP_FUNC_SHIFT_TAB, param1, param2);
		break;

	case HMSG_USER_CMD_THUMB_NEXT_PAGE:
		if (!APP_CHECKFLAGS(app->m_thumb.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		res = app->Func(THUMB_APP_FUNC_NEXT_PAGE, param1, param2);
		break;

	case HMSG_USER_CMD_THUMB_PREV_PAGE:
		if (!APP_CHECKFLAGS(app->m_thumb.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		res = app->Func(THUMB_APP_FUNC_PREV_PAGE, param1, param2);
		break;

	case HMSG_USER_CMD_THUMB_SEL_TO_PLAY:
		if (!APP_CHECKFLAGS(app->m_thumb.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		res = app->Func(THUMB_APP_FUNC_SEL_TO_PLAY, param1, param2);
		break;

	case HMSG_USER_CMD_THUMB_EDIT_ENTER:
		if (!APP_CHECKFLAGS(app->m_thumb.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		res = app->Func(THUMB_APP_FUNC_EDIT_ENTER, param1, param2);
		break;

	case HMSG_USER_CMD_THUMB_EDIT_QUIT:
		if (!APP_CHECKFLAGS(app->m_thumb.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		res = app->Func(THUMB_APP_FUNC_EDIT_QUIT, param1, param2);
		break;

	case HMSG_USER_CMD_THUMB_EDIT_COPY:
		if (!APP_CHECKFLAGS(app->m_thumb.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		res = app->Func(THUMB_APP_FUNC_EDIT_COPY, param1, param2);
		break;

	case HMSG_USER_CMD_THUMB_EDIT_DEL:
		if (!APP_CHECKFLAGS(app->m_thumb.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		res = app->Func(THUMB_APP_FUNC_EDIT_DEL, param1, param2);
		break;

	case HMSG_USER_CMD_THUMB_EDIT_SEL:
		if (!APP_CHECKFLAGS(app->m_thumb.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		res = app->Func(THUMB_APP_FUNC_EDIT_SEL, param1, param2);
		break;

	case HMSG_USER_CMD_THUMB_EDIT_SEL_ALL:
		if (!APP_CHECKFLAGS(app->m_thumb.GFlags, APP_AFLAGS_READY)) {
			break;    // not ready
		}
		res = app->Func(THUMB_APP_FUNC_EDIT_SEL_ALL, param1, param2);
		break;

	case HMSG_USER_CMD_MAIN_MENU_TAB:
		res = app->Func(THUMB_APP_FUNC_SWITCH_APP, param1, param2);
		break;

	case ASYNC_MGR_MSG_FILE_DEL_DONE:
		res = app->Func(THUMB_APP_FUNC_DELETE_FILE_COMPLETE, param1, param2);
		break;

	case ASYNC_MGR_MSG_FILE_COPY_DONE:
		res = app->Func(THUMB_APP_FUNC_COPY_FILE_COMPLETE, param1, param2);
		break;

	case AMSG_STATE_CARD_INSERT_ACTIVE:
		{
			DvrRecApp *rec_app = DvrRecApp::Get();
			if(rec_app != NULL && rec_app->Func != NULL){
				rec_app->Func(REC_APP_FUNC_CARD_INSERT, param1, param2);
			}
			
			if (APP_CHECKFLAGS(app->m_thumb.GFlags, APP_AFLAGS_READY)) {
				res = app->Func(THUMB_APP_FUNC_CARD_INSERT, param1, param2);
			}
		}
		break;

	case AMSG_STATE_CARD_REMOVED:
		{
			if (APP_CHECKFLAGS(app->m_thumb.GFlags, APP_AFLAGS_READY)) {
				res = app->Func(THUMB_APP_FUNC_CARD_REMOVE, param1, param2);
			}

			DvrRecApp *rec_app = DvrRecApp::Get();
			if(rec_app != NULL && rec_app->Func != NULL){
				rec_app->Func(REC_APP_FUNC_CARD_REMOVE, param1, param2);
			}
		}
		break;

    case AMSG_CMD_CARD_FORMAT:
    	{
        	res = app->Func(THUMB_APP_FUNC_FORMAT_CARD, param1, param2);

			DvrRecApp *rec_app = DvrRecApp::Get();
			if(rec_app != NULL && rec_app->Func != NULL){
				rec_app->Func(REC_APP_FUNC_CARD_FORMAT, param1, param2);
			}
    	}
        break;

	case ASYNC_MGR_MSG_CARD_FORMAT_DONE:
		{
			DvrRecApp *rec_app = DvrRecApp::Get();
			if(rec_app != NULL && rec_app->Func != NULL){
				rec_app->Func(REC_APP_FUNC_CARD_FORMAT_DONE, param1, param2);
			}
		}
		break;

	case HMSG_USER_CMD_THUMB_UPDATE_FRAME:
		if(FALSE == app->bInEditMode)
		{
			res = app->Func(THUMB_APP_FUNC_START_DISP_PAGE, param1, param2);
		}
		break;

	case HMSG_USER_CMD_RECORDER_ONOFF:
		{
			DvrRecApp *rec_app = DvrRecApp::Get();
			if(rec_app != NULL && rec_app->Func != NULL){
				res = rec_app->Func(REC_APP_FUNC_LOOP_RECORD_SWITCH, param1, param2);
			}
		}
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
			Log_Warning("thumb app m_bSnapShotEnable %d\n",rec_app->m_bSnapShotEnable);		}
		}
		break;

    case HMSG_USER_CMD_RECORDER_LOOPREC_SPLIT_TIME:
		{
			DvrRecApp *rec_app = DvrRecApp::Get();
			if(rec_app != NULL && rec_app->Func != NULL){
                res = rec_app->Func(REC_APP_FUNC_SET_LOOPREC_SPLIT_TIME, param1, param2);
			}
		}
		break;

	case HMSG_USER_CMD_RECORDER_SNAPSHOT:
		{
			DvrRecApp *rec_app = DvrRecApp::Get();
			if(rec_app != NULL && rec_app->Func != NULL){
                res = rec_app->Func(REC_APP_FUNC_SNAPSHOT, param1, param2);
			}
		}
		break;

	case HMSG_USER_CMD_RECORDER_EVENT_RECORD:
		{
			DvrRecApp *rec_app = DvrRecApp::Get();
			if(rec_app != NULL && rec_app->Func != NULL){
				res = rec_app->Func(REC_APP_FUNC_EVENT_RECORD_START, param1, param2);
			}
		}
		break;

	case AMSG_CMD_EVENT_RECORDER_EMERGENCY_COMPLETE:
		{
			DvrRecApp *rec_app = DvrRecApp::Get();
			if(rec_app != NULL && rec_app->Func != NULL){
				res = rec_app->Func(REC_APP_FUNC_EVENT_RECORD_COMPLETE, param1, param2);
			}
		}
		break;

	case AMSG_CMD_EVENT_RECORDER_ALARM_COMPLETE:
		{
			DvrRecApp *rec_app = DvrRecApp::Get();
			if(rec_app != NULL && rec_app->Func != NULL){
				res = rec_app->Func(REC_APP_FUNC_ALARM_RECORD_COMPLETE, param1, param2);
			}
		}
		break;

	case HMSG_USER_CMD_RECORDER_ALARM_RECORD_START:
		{
			DvrRecApp *rec_app = DvrRecApp::Get();
			if(rec_app != NULL && rec_app->Func != NULL){
				res = rec_app->Func(REC_APP_FUNC_ALARM_RECORD_START, param1, param2);
			}
		}
		break;

	case HMSG_USER_CMD_RECORDER_DAS_RECORD_START:
        {
            DvrRecApp *rec_app = DvrRecApp::Get();
            if(rec_app != NULL && rec_app->Func != NULL){
                res = rec_app->Func(REC_APP_FUNC_DAS_RECORD_START, param1, param2);
            }
        }
		break;

    case HMSG_USER_CMD_RECORDER_DAS_RECORD_STOP:
        {
            {
                DvrRecApp *rec_app = DvrRecApp::Get();
                if(rec_app != NULL && rec_app->Func != NULL){
                    res = rec_app->Func(REC_APP_FUNC_DAS_RECORD_COMPLETE, param1, param2);
                }
            }
        }
        break;

    case HMSG_USER_CMD_RECORDER_IACC_RECORD:
        {
            DvrRecApp *rec_app = DvrRecApp::Get();
            if(rec_app != NULL && rec_app->Func != NULL){
                res = rec_app->Func(REC_APP_FUNC_IACC_RECORD_START, param1, param2);
            }
        }
        break;

    case AMSG_CMD_EVENT_RECORDER_IACC_COMPLETE:
        {
            DvrRecApp *rec_app = DvrRecApp::Get();
            if(rec_app != NULL && rec_app->Func != NULL){
                res = rec_app->Func(REC_APP_FUNC_IACC_RECORD_COMPLETE, param1, param2);
            }
        }
        break;

	case HMSG_STORAGE_RUNOUT:
		{
			DvrRecApp *rec_app = DvrRecApp::Get();
			if(rec_app != NULL && rec_app->Func != NULL){
				res = rec_app->Func(REC_APP_FUNC_ERROR_STORAGE_RUNOUT, param1, param2);
			}
		}
		break;

	case HMSG_STORAGE_RUNOUT_HANDLE_DONE:
		{
			DvrRecApp *rec_app = DvrRecApp::Get();
			if(rec_app != NULL && rec_app->Func != NULL){
				res = rec_app->Func(REC_APP_FUNC_ERROR_STORAGE_RUNOUT_HANDLE_DONE, param1, param2);
			}
			
			if(FALSE == app->bInEditMode)
			{
				res = app->Func(THUMB_APP_FUNC_START_DISP_PAGE, FALSE, 0);
			}
		}
		break;

	case HMSG_STORAGE_RUNOUT_HANDLE_ERROR:
		{
			DvrRecApp *rec_app = DvrRecApp::Get();
			if(rec_app != NULL && rec_app->Func != NULL){
				res = rec_app->Func(REC_APP_FUNC_ERROR_STORAGE_RUNOUT_HANDLE_ERROR, param1, param2);
			}
		}
		break;

	default:
		break;
	}

	return res;
}

DvrThumbApp::DvrThumbApp()
{
	m_bHasInited = FALSE;
}

DVR_BOOL DvrThumbApp::Init()
{
	if (m_bHasInited)
	{
		return TRUE;
	}

	m_thumb.Id = 0;
	m_thumb.Tier = 1;
	m_thumb.Parent = 0;
	m_thumb.Previous = 0;
	m_thumb.Child = 0;
	m_thumb.GFlags = 0;
	m_thumb.Flags = 0;
	m_thumb.Start = app_thumb_start;
	m_thumb.Stop = app_thumb_stop;
	m_thumb.OnMessage = app_thumb_on_message;

	m_bHasInited = TRUE;

	return m_bHasInited;
}

DvrThumbApp *DvrThumbApp::Get(void)
{
	static DvrThumbApp thumb;

	if (thumb.Init())
	{
		return &thumb;
	}

	return NULL;
}
