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
#include "DVR_SDK_INTFS.h"
#include "DVR_GUI_OBJ.h"
#include "flow/widget/DvrWidget_Dialog.h"
#include "flow/widget/DvrWidget_Mgt.h"
#include "flow/misc/DvrMiscFormatCard.h"
#include "flow/util/CThreadPool.h"
#include "flow/rec/DvrRecApp.h"
#include "gui/Gui_DvrMiscFormatCard.h"
#include "framework/DvrAppUtil.h"
#include "framework/DvrAppNotify.h"
#include <log/log.h>

#ifdef __linux__
#include <unistd.h>
#endif

static DVR_RESULT misc_formatcard_start(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrMiscFormatCard *app = DvrMiscFormatCard::Get();

	app->Func(MISC_APP_FORMATCARD_DIALOG_SHOW_FORMATCARD, 0, 0);

	return res;
}

static DVR_RESULT misc_formatcard_stop(void)
{
	DVR_RESULT res = 0;

	return res;
}

static int formatcard_dialog_format_caution_handler(DVR_U32 sel, DVR_U32 param1, DVR_U32 param2)
{
	int res = 0;
	DvrMiscFormatCard *app = DvrMiscFormatCard::Get();

	APP_MGT_INST *ParentApp = NULL;

	switch (sel) {
	case DIALOG_SEL_YES:
	{
		APP_ADDFLAGS(app->m_formatcard.GFlags, APP_AFLAGS_BUSY);
		APP_ADDFLAGS(app->m_formatcard.Flags, MISC_FORMATCARD_DO_FORMAT_CARD);
		/** Unregister all timers: slide show, self timer...     */
		//Dvr_Sdk_ComSvrTimer_UnRegisterAll();


        DVR_U32 DVR_FormatStatus = 3; //formatting

		DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_FORMAT_STATUS, (void *)&DVR_FormatStatus, NULL);

		if (DvrAppMgt_GetApp(&ParentApp, app->m_formatcard.Parent) == 0)
		{
			res = ParentApp->OnMessage(AMSG_CMD_CARD_FORMAT, param1, param2);
		}

		Log_Message("Start Formatting !!!!!!!!!!!!");
		res = Dvr_Sdk_ComSvcAsyncOp_CardFormat();

		app->Func(MISC_APP_FORMATCARD_WARNING_MSG_SHOW_START, GUI_WARNING_PROCESSING, 1);
	}
	break;

	default:
	case DIALOG_SEL_NO:
		/** To give up formatting card.*/
		app->Func(MISC_APP_FORMATCARD_SWITCH_APP, 0, 0);
		break;
	}

	return res;
}

static DVR_RESULT formatcard_dialog_format_handler(DVR_U32 sel, DVR_U32 param1, DVR_U32 param2)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrMiscFormatCard *app = DvrMiscFormatCard::Get();

	switch (sel)
	{
	case DIALOG_SEL_YES:
		res = AppDialog_SetDialog(GUI_DIALOG_TYPE_Y_N, DIALOG_SUBJECT_FORMATCARD_CAUTION, formatcard_dialog_format_caution_handler);
		res = AppWidget_On(DVR_WIDGET_DIALOG, 0);
		APP_ADDFLAGS(app->m_formatcard.GFlags, APP_AFLAGS_POPUP);
		break;

	case DIALOG_SEL_NO:
	default:
		/** To give up formatting card.*/
		app->Func(MISC_APP_FORMATCARD_SWITCH_APP, 0, 0);
		break;
	}
	return res;
}

static int misc_formatcard_dialog_show_formatcard(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrMiscFormatCard *app = DvrMiscFormatCard::Get();

	res = Dvr_Sdk_StorageCard_CheckStatus();
	if (res != DVR_STORAGE_CARD_STATUS_NO_CARD)
	{
	#if 0
		AppDialog_SetDialog(GUI_DIALOG_TYPE_Y_N, DIALOG_SUBJECT_FORMATCARD, formatcard_dialog_format_handler);
		AppWidget_On(DVR_WIDGET_DIALOG, 0);
		APP_ADDFLAGS(app->m_formatcard.GFlags, APP_AFLAGS_POPUP);
	#else
		formatcard_dialog_format_caution_handler(DIALOG_SEL_YES, 0, 0);
	#endif
	}
	else 
	{
		if (res == DVR_STORAGE_CARD_STATUS_NO_CARD){
			Log_Error("[App - FormatCard] <DialogShowFormatCard> WARNING_NO_CARD\n");
			DVR_U32 DVR_SDCardErrorStatus = DVR_SDCARD_ERROR_STATUS_NO_SDCARD; //no card
            DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_ERROR_STATUS_UPDATE, (void *)&DVR_SDCardErrorStatus, NULL);
		}
		else{
			Log_Error("[App - FormatCard] <DialogShowFormatCard> WARNING_CARD_Error %d\n", res);
		}

		app->Func(MISC_APP_FORMATCARD_SWITCH_APP, 0, 0);
	}

	return res;
}

static DVR_RESULT misc_formatcard_widget_closed(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrMiscFormatCard *app = DvrMiscFormatCard::Get();

	if (APP_CHECKFLAGS(app->m_formatcard.GFlags, APP_AFLAGS_POPUP)) {
		APP_REMOVEFLAGS(app->m_formatcard.GFlags, APP_AFLAGS_POPUP);
	}

	return res;
}

static DVR_RESULT misc_formatcard_switch_app(int param)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrMiscFormatCard *app = DvrMiscFormatCard::Get();

	if (param)
	{
		/*The process of formatting card has done*/
		DvrAppUtil_SwitchApp(Dvr_App_GetAppId(MDL_APP_RECORDER_ID));
	}
	else
	{
		/* Doesn't format card.*/
		if (app->m_formatcard.Parent != Dvr_App_GetAppId(MDL_APP_MAIN_ID))
		{
			DvrAppUtil_SwitchApp(app->m_formatcard.Parent);
		}
		else
		{
			DvrAppUtil_SwitchApp(Dvr_App_GetAppId(MDL_APP_RECORDER_ID));
		}
	}

	
	
	DvrRecApp *Recapp = DvrRecApp::Get();
	if( (TRUE == Recapp->bUserEnableRec) && (TRUE == Recapp->bPowerMode))
	{
		Log_Error("misc_formatcard_switch_app  REC_APP_FUNC_LOOP_RECORD_SWITCH\n");
		res = Recapp->Func(REC_APP_FUNC_LOOP_RECORD_SWITCH, TRUE, 0);
	}
	
	
	Dvr_App_HuSendCmd(HMSG_USER_CMD_MAIN_MENU_TAB, GUI_MAIN_MENU_TAB_SETUP, sizeof(int));	

	return res;
}

static DVR_RESULT misc_formatcard_op_done(int param1, int param2)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrMiscFormatCard *app = DvrMiscFormatCard::Get();

	if (param1 < 0) {
		app->Func(MISC_APP_FORMATCARD_OP_FAILED, 0, 0);
	}
	else {
		app->Func(MISC_APP_FORMATCARD_OP_SUCCESS, 0, param2);
	}

	return res;
}


static int formatcard_dialog_ok_handler(DVR_U32 sel, DVR_U32 param1, DVR_U32 param2)
{
	int ReturnValue = 0;
	DvrMiscFormatCard *app = DvrMiscFormatCard::Get();

	APP_REMOVEFLAGS(app->m_formatcard.Flags, MISC_FORMATCARD_DO_FORMAT_CARD);
	switch (sel) {
	default:
	case DIALOG_SEL_OK:
		/*The process of formatting card has done.*/
		if (APP_CHECKFLAGS(app->m_formatcard.GFlags, APP_AFLAGS_READY)) {
			/* The system could switch the current app to other in the function "app_util_check_busy_blocked". */
			app->Func(MISC_APP_FORMATCARD_SWITCH_APP, 0, 0);
		}
		break;
	}

	return ReturnValue;
}

static void misc_formatcard_feedback_success(void *arg)
{
    DvrAppNotify *pHandle = DvrAppNotify::Get();

    DVR_U32 DVR_FormatStatus = 4;//format complete

    if (pHandle != NULL)
		pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_FORMAT_STATUS, (void *)&DVR_FormatStatus, NULL);

#ifdef __linux__
    usleep(3*1000*1000); //wait 1000ms
#endif

	DVR_FormatStatus = 1;
	if (pHandle != NULL)
		pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_FORMAT_STATUS, (void *)&DVR_FormatStatus, NULL);
}

static void misc_formatcard_feedback_failed(void *arg)
{
    DvrAppNotify *pHandle = DvrAppNotify::Get();

    DVR_U32 DVR_FormatStatus = 5;//format failed


    if (pHandle != NULL)
		pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_FORMAT_STATUS, (void *)&DVR_FormatStatus, NULL);

#ifdef __linux__
    usleep(3*1000*1000); //wait 1000ms
#endif

	DVR_FormatStatus = 1;
	if (pHandle != NULL)
		pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_FORMAT_STATUS, (void *)&DVR_FormatStatus, NULL);
}

static DVR_RESULT misc_formatcard_op_success(int param1, int param2)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrMiscFormatCard *app = DvrMiscFormatCard::Get();
	APP_MGT_INST *CurApp = NULL;

	Dvr_Sdk_FileMapDB_Clear();

	if(param2 == 0)
	{
	    res = Dvr_Sdk_MediaMgr_Reset();
		if (res < 0) {
			Log_Error("[App - FormatCard] <OpSuccess> refresh media manager error!\n");
		}
	}

	/** update card_status.format*/
	app->Func(MISC_APP_FORMATCARD_WARNING_MSG_SHOW_STOP, 0, 0);

#if 0
	APP_REMOVEFLAGS(app->m_formatcard.GFlags, APP_AFLAGS_BUSY);
	res = AppDialog_SetDialog(GUI_DIALOG_TYPE_OK, DIALOG_SUBJECT_FORMATCARD_OK, formatcard_dialog_ok_handler);
	res = AppWidget_On(DVR_WIDGET_DIALOG, 0);
	APP_ADDFLAGS(app->m_formatcard.GFlags, APP_AFLAGS_POPUP);
	APP_REMOVEFLAGS(app->m_formatcard.Flags, MISC_FORMATCARD_DO_FORMAT_CARD);
#endif
	Log_Message("misc_formatcard_op_success!!!!!!!!");

	formatcard_dialog_ok_handler(DIALOG_SEL_OK, 0, 0);

#if 0
	DvrAppMgt_GetCurApp(&CurApp);
	if(CurApp != NULL)
	{
		if(app->m_bErrorFormatCard == TRUE)
			res = CurApp->OnMessage(ASYNC_MGR_MSG_CARD_FORMAT_DONE, 1, 0);
		else
			res = CurApp->OnMessage(ASYNC_MGR_MSG_CARD_FORMAT_DONE, 0, 0);
	}
#endif	

	CThreadPool::Get()->AddTask(misc_formatcard_feedback_success, NULL);

	return res;
}


static DVR_RESULT misc_formatcard_op_failed(int param1, int param2)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrMiscFormatCard *app = DvrMiscFormatCard::Get();

	app->Func(MISC_APP_FORMATCARD_WARNING_MSG_SHOW_STOP, 0, 0);

#if 0
	APP_REMOVEFLAGS(app->m_formatcard.GFlags, APP_AFLAGS_BUSY);
	res = AppDialog_SetDialog(GUI_DIALOG_TYPE_OK, DIALOG_SUBJECT_FORMATCARD_FAILED, formatcard_dialog_ok_handler);
	res = AppWidget_On(DVR_WIDGET_DIALOG, 0);
	APP_ADDFLAGS(app->m_formatcard.GFlags, APP_AFLAGS_POPUP);
#endif
	Log_Message("misc_formatcard_op_failed!!!!!!!!");

	formatcard_dialog_ok_handler(DIALOG_SEL_OK, 0, 0);

	CThreadPool::Get()->AddTask(misc_formatcard_feedback_failed, NULL);
	
	return res;
}

static DVR_RESULT misc_formatcard_warning_msg_show(int enable, int param1, int param2)
{
	DVR_RESULT res = 0;
	DvrMiscFormatCard *app = DvrMiscFormatCard::Get();

	if (enable) {
		if (param1 == MISC_FORMATCARD_CARD_PROTECTED) {
			param1 = GUI_WARNING_CARD_PROTECTED;
		}
		if (param2) {
			app->Gui(GUI_MISC_FORMATCARD_APP_CMD_WARNING_UPDATE, param1, sizeof(DVR_U32));
			app->Gui(GUI_MISC_FORMATCARD_APP_CMD_WARNING_SHOW, 0, 0);
		}
		else {
			if (!APP_CHECKFLAGS(app->m_formatcard.Flags, MISC_FORMATCARD_WARNING_MSG_RUN)) {
				APP_ADDFLAGS(app->m_formatcard.Flags, MISC_FORMATCARD_WARNING_MSG_RUN);
				app->Gui(GUI_MISC_FORMATCARD_APP_CMD_WARNING_UPDATE, param1, 0);
				app->Gui(GUI_MISC_FORMATCARD_APP_CMD_WARNING_SHOW, 0, 0);
			}
		}
	}
	else {
		if (APP_CHECKFLAGS(app->m_formatcard.Flags, MISC_FORMATCARD_WARNING_MSG_RUN)) {
			APP_REMOVEFLAGS(app->m_formatcard.Flags, MISC_FORMATCARD_WARNING_MSG_RUN);
		}
		app->Gui(GUI_MISC_FORMATCARD_APP_CMD_WARNING_HIDE, 0, 0);
	}
	app->Gui(GUI_MISC_FORMATCARD_APP_CMD_FLUSH, 0, 0);

	return res;
}

static int misc_formatcard_card_removed(void)
{
	int ReturnValue = 0;
	DvrMiscFormatCard *app = DvrMiscFormatCard::Get();

	app->Func(MISC_APP_FORMATCARD_WARNING_MSG_SHOW_STOP, 0, 0);
	if (APP_CHECKFLAGS(app->m_formatcard.GFlags, APP_AFLAGS_READY)) {
		APP_REMOVEFLAGS(app->m_formatcard.GFlags, APP_AFLAGS_BUSY);
		if (APP_CHECKFLAGS(app->m_formatcard.Flags, MISC_FORMATCARD_DO_FORMAT_CARD) == 0) {
			DvrAppUtil_SwitchApp(Dvr_App_GetAppId(MDL_APP_RECORDER_ID));
		}
	}

	return ReturnValue;
}

int misc_formatcard_func(DVR_U32 funcId, DVR_U32 param1, DVR_U32 param2)
{
	DVR_RESULT res = DVR_RES_SOK;

	switch (funcId)
	{
	case MISC_APP_FORMATCARD_INIT:
		break;

	case MISC_APP_FORMATCARD_START:
		res = misc_formatcard_start();
		break;

	case MISC_APP_FORMATCARD_STOP:
		res = misc_formatcard_stop();
		break;

	case MISC_APP_FORMATCARD_OP_DONE:
        printf("rec MISC_APP_FORMATCARD_OP_DONE \n");
		res = misc_formatcard_op_done(param1, param2);
		break;

	case MISC_APP_FORMATCARD_OP_SUCCESS:
		res = misc_formatcard_op_success(param1, param2);
		break;

	case MISC_APP_FORMATCARD_OP_FAILED:
		res = misc_formatcard_op_failed(param1, param2);
		break;

	case MISC_APP_FORMATCARD_SWITCH_APP:
		res = misc_formatcard_switch_app(param1);
		break;

	case MISC_APP_FORMATCARD_CARD_REMOVED:
		res = misc_formatcard_card_removed();
		break;

	case MISC_APP_FORMATCARD_DIALOG_SHOW_FORMATCARD:
		res = misc_formatcard_dialog_show_formatcard();
		break;

	case MISC_APP_FORMATCARD_STATE_WIDGET_CLOSED:
		res = misc_formatcard_widget_closed();
		break;

	case MISC_APP_FORMATCARD_WARNING_MSG_SHOW_START:
		res = misc_formatcard_warning_msg_show(1, param1, param2);
		break;

	case MISC_APP_FORMATCARD_WARNING_MSG_SHOW_STOP:
		res = misc_formatcard_warning_msg_show(0, param1, param2);
		break;

	default:
		break;
	}

	return res;
}
