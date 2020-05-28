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
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "flow/pb/DvrPbApp.h"
#include "flow/widget/DvrWidget_Dialog.h"
#include "flow/widget/DvrWidget_Mgt.h"
#include "flow/util/CThreadPool.h"
#include "framework/DvrAppNotify.h"
#include "gui/Gui_DvrPbApp.h"
#include <log/log.h>
#include <event/ofilm_msg_type.h>
#include <string>
#include <sys/stat.h>

#include "Gpu_Dvr_Interface.h"
#include <DVR_APP_INTFS_INPUT.h>

#define MIN_CLICK_DELAY_TIME 1000

static DVR_RESULT pb_app_init(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();

	app->lastClickTime = 0;
	app->PBSpeedlastClickTime = 0;
	app->OpenFileResult = DVR_RES_SOK;
	app->bAutoSwitchIsInterrupted = FALSE;

	return res;
}

static void system_sync(void *arg)
{
#ifdef __linux__
	sync();
#endif
	return;
}

static unsigned long pb_app_get_sys_time()
{
#ifdef __linux__
	static struct timespec rt;
	clock_gettime(CLOCK_MONOTONIC, &rt);
	return (rt.tv_sec * 1000 + rt.tv_nsec/1000000);
#else
	return (GetTickCount()); 
#endif
}

void DvrPlayerOnNewVehicleData(void *pInstance, void *pUserData)
{
	Ofilm_Can_Data_T *pVehicleData = (Ofilm_Can_Data_T *)pUserData;

    if (pVehicleData == NULL)
    {
        return;
    }

	DvrPlayBackApp *app = DvrPlayBackApp::Get();
	
	GUI_OBJ_VEHICLE_DATA_INST vehicle_data;
	memset(&vehicle_data, 0, sizeof(vehicle_data));

	vehicle_data.TimeYear = pVehicleData->vehicle_data.year + 2013;
	vehicle_data.TimeMon = pVehicleData->vehicle_data.month;
	vehicle_data.TimeDay = pVehicleData->vehicle_data.day;
	vehicle_data.TimeHour = pVehicleData->vehicle_data.hour;
	vehicle_data.TimeMin = pVehicleData->vehicle_data.minute;
	vehicle_data.TimeSec = pVehicleData->vehicle_data.second;	
	if(pVehicleData->vehicle_data.vehicle_speed > 0.0f)
	{
		vehicle_data.VehicleSpeed = (unsigned short)(2.0f + 1.02f * pVehicleData->vehicle_data.vehicle_speed);
	}
	else
	{
		vehicle_data.VehicleSpeed = 0;
	}
	vehicle_data.VehicleSpeedValidity = pVehicleData->vehicle_data.Vehicle_Speed_Validity;
	vehicle_data.GearShiftPositon = pVehicleData->vehicle_data.vehicle_movement_state;
	vehicle_data.BrakePedalStatus = pVehicleData->vehicle_data.Brake_Pedal_Position;
	vehicle_data.DriverBuckleSwitchStatus = pVehicleData->vehicle_data.DriverBuckleSwitchStatus;
	vehicle_data.AccePedalPosition = pVehicleData->vehicle_data.Accelerator_Actual_Position;		
	
	if(pVehicleData->vehicle_data.turn_signal == 1)
	{
		vehicle_data.LeftTurnLampStatus = 1;
		vehicle_data.RightTurnLampStatus = 0;
	}
	else if(pVehicleData->vehicle_data.turn_signal == 2)
	{
		vehicle_data.LeftTurnLampStatus = 0;
		vehicle_data.RightTurnLampStatus = 1;
	}	
	else
	{
		vehicle_data.LeftTurnLampStatus = 0;
		vehicle_data.RightTurnLampStatus = 0;
	}

	float GpsLongitude = (float)pVehicleData->vehicle_data.positioning_system_longitude * 0.000001f - 268.435455f;
	float GpsLatitude = (float)pVehicleData->vehicle_data.positioning_system_latitude * 0.000001f - 134.217727f;
	float absGpsLongitude = fabs(GpsLongitude);
	float absGpsLatitude = fabs(GpsLatitude); 

	absGpsLongitude = (absGpsLongitude > 180) ? 180 : absGpsLongitude;
	absGpsLatitude = (absGpsLatitude > 90) ? 90 : absGpsLatitude;

	vehicle_data.GpsLongitude_IsEast = (GpsLongitude >= 0) ? 1 : 0;
	vehicle_data.GpsLongitude_Deg = (uint16_t)absGpsLongitude;
	vehicle_data.GpsLongitude_Min = (uint16_t)((float)((float)absGpsLongitude - vehicle_data.GpsLongitude_Deg) * 60);
	vehicle_data.GpsLongitude_Sec = (uint16_t)((((float)((float)absGpsLongitude - vehicle_data.GpsLongitude_Deg) * 60) - vehicle_data.GpsLongitude_Min) * 60)  ;
	
	vehicle_data.GpsLatitude_IsNorth = (GpsLatitude >= 0) ? 1 : 0;
	vehicle_data.GpsLatitude_Deg = (uint16_t)absGpsLatitude;
	vehicle_data.GpsLatitude_Min = (uint16_t)((float)((float)absGpsLatitude - vehicle_data.GpsLatitude_Deg) * 60);
	vehicle_data.GpsLatitude_Sec = (uint16_t)((((float)((float)absGpsLatitude - vehicle_data.GpsLatitude_Deg) * 60) - vehicle_data.GpsLatitude_Min) * 60)  ;

	app->Gui(GUI_PB_APP_CMD_VEHICLE_DATA_UPDATE, (DVR_U32)&vehicle_data, sizeof(GUI_OBJ_VEHICLE_DATA_INST));
	app->Gui(GUI_PB_APP_CMD_FLUSH, 0, 0);
}

static void pb_app_timer_handler(DVR_U32 eid)
{
	DvrPlayBackApp *app = DvrPlayBackApp::Get();
    APP_STATUS *pStatus = (APP_STATUS *)Dvr_App_GetStatus();

    if (pStatus->selFileType == DVR_FILE_TYPE_VIDEO)
    {
        GUI_OBJ_PLAY_TIME_INST play_time;
        memset(&play_time, 0, sizeof(play_time));

        unsigned int position_ms = 0, duration_ms = 0;

        Dvr_Sdk_Player_Get(DVR_PLAYER_PROP_POSITION, &position_ms, sizeof(play_time.position), NULL);
        Dvr_Sdk_Player_Get(DVR_PLAYER_PROP_DURATION, &duration_ms, sizeof(play_time.duration), NULL);

        play_time.position = position_ms;
        play_time.duration = duration_ms;

        app->Gui(GUI_PB_APP_CMD_PLAY_TIMER_UPDATE, (DVR_U32)&play_time, sizeof(GUI_OBJ_PLAY_TIME_INST));
	    DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_PLAY_TIMER_UPDATE, (void *)&play_time, NULL);

        GUI_OBJ_PLAY_FILENAME_INST play_filename;
        char CurFile[APP_MAX_FN_SIZE] = { 0 };
        Dvr_Sdk_Player_Get(DVR_PLAYER_PROP_CURRENT_FILE, CurFile, APP_MAX_FN_SIZE, NULL);

		memset(&play_filename, 0, sizeof(play_filename));
		memcpy(play_filename.filename, CurFile, APP_MAX_FN_SIZE);

        app->Gui(GUI_PB_APP_CMD_FILENAME_UPDATE, (DVR_U32)&play_filename, sizeof(GUI_OBJ_PLAY_FILENAME_INST));
    }
    else
    {
        GUI_OBJ_PLAY_FILENAME_INST play_filename;
        memset(&play_filename, 0, sizeof(play_filename));

        char CurFile[APP_MAX_FN_SIZE] = { 0 };
        Dvr_Sdk_Player_Get(DVR_PLAYER_PROP_CURRENT_FILE, CurFile, APP_MAX_FN_SIZE, NULL);

        strcpy(play_filename.filename, CurFile);

        app->Gui(GUI_PB_APP_CMD_FILENAME_UPDATE, (DVR_U32)&play_filename, sizeof(GUI_OBJ_PLAY_FILENAME_INST));
    }

	app->Gui(GUI_PB_APP_CMD_FLUSH, 0, 0);
}

static DVR_RESULT pb_app_start(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();

	res = Dvr_Sdk_Player_Init();
	if (res != DVR_RES_SOK)
		return res;

	//Dvr_Sdk_ComSvrTimer_Register(TIMER_2HZ, pb_app_timer_handler);

	Dvr_Sdk_Player_RegisterVehDataUpdateCallBack(DvrPlayerOnNewVehicleData, NULL);

	DVR_S32 status;
	DVR_DISK_DIRECTORY Directory;
	memset(&Directory, 0, sizeof(DVR_DISK_DIRECTORY));

	if (DVR_RES_SOK == Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_DIRECTORY_STRUCTURE, &Directory, sizeof(DVR_DISK_DIRECTORY), &status))
	{
		Dvr_Sdk_Player_Set(DVR_PLAYER_PROP_ROOT_DIRECTORY, &Directory, sizeof(DVR_DISK_DIRECTORY));
	}

	app->playState = PB_APP_STATE_PLAYER_IDLE;

	return res;
}

static DVR_RESULT pb_app_stop(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();
	
	app->Gui(GUI_PB_APP_CMD_WARNING_HIDE, GUI_WARNING_ERROR_FILE, 0);
	app->Gui(GUI_PB_APP_CMD_FLUSH, 0, 0);

	res = Dvr_Sdk_Player_DeInit();

	//Dvr_Sdk_ComSvrTimer_UnRegister(TIMER_2HZ, pb_app_timer_handler);

	APP_REMOVEFLAGS(app->m_playback.GFlags, APP_AFLAGS_POPUP);

	app->playState = PB_APP_STATE_PLAYER_IDLE;

	return res;
}



static DVR_RESULT pb_app_player_quit(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();

	app->bAutoSwitchIsInterrupted = TRUE;
	APP_REMOVEFLAGS(app->m_playback.GFlags, APP_AFLAGS_BUSY);
	
	res = app->Func(PB_APP_FUNC_SWITCH_APP_BY_APPID, app->m_playback.Previous, 0);

	AVMInterface_setDvrUIMode(GUI_LAYOUT_THUMB_EXT, GUI_VIEW_INDEX_FRONT_EXT);

#if 0
	//start recorder async
	DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_ASYNC_START_RECORDER, NULL, NULL);	
#endif

	return res;
}

static DVR_RESULT pb_app_ready(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();
	APP_STATUS *pStatus = (APP_STATUS *)Dvr_App_GetStatus();

	if (!APP_CHECKFLAGS(app->m_playback.GFlags, APP_AFLAGS_READY))
	{
		APP_ADDFLAGS(app->m_playback.GFlags, APP_AFLAGS_READY);
		DvrAppUtil_ReadyCheck();

		if (!APP_CHECKFLAGS(app->m_playback.GFlags, APP_AFLAGS_READY)) {
			/* The system could switch the current app to other in the function "DvrAppUtil_ReadyCheck". */
			return res;
		}
	}

	if (APP_CHECKFLAGS(app->m_playback.GFlags, APP_AFLAGS_READY)) {
		app->Func(PB_APP_FUNC_OPEN_FILE, 0, 0);
	}

	return res;
}

static DVR_RESULT pb_app_start_disp_page(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	
	return res;
}

static DVR_RESULT pb_app_player_open(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();
	APP_STATUS *pStatus = (APP_STATUS *)Dvr_App_GetStatus();

	//stop recorder async
	DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_ASYNC_STOP_RECORDER, NULL, NULL);
	CThreadPool::Get()->AddTask(system_sync, NULL);

	switch (pStatus->selFileType)
	{
	case DVR_FILE_TYPE_VIDEO:
		res = Dvr_Sdk_Player_Open((const char *)pStatus->selFileName,pStatus->selFolderType);
		break;

	case DVR_FILE_TYPE_IMAGE:
		res = Dvr_Sdk_Photo_Open((const char *)pStatus->selFileName);
		break;

	default:
		return DVR_RES_EINVALIDARG;
	}

	app->playState = PB_APP_STATE_PLAYER_RUNNING;
	app->Gui(GUI_PB_APP_CMD_PLAY_STATE_UPDATE, GUI_PLAY_STATE_RUNNING, 4);	
	APP_ADDFLAGS(app->m_playback.GFlags, APP_AFLAGS_BUSY);

	return res;
}

static DVR_RESULT pb_app_player_close(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();

	APP_STATUS *pStatus = (APP_STATUS *)Dvr_App_GetStatus();
	switch (pStatus->selFileType)
	{
	case DVR_FILE_TYPE_VIDEO:
		res = Dvr_Sdk_Player_Close();
		break;

	case DVR_FILE_TYPE_IMAGE:
		res = Dvr_Sdk_Photo_Close();
		break;

	default:
		return DVR_RES_EINVALIDARG;
	}

	app->playState = PB_APP_STATE_PLAYER_IDLE;
	APP_REMOVEFLAGS(app->m_playback.GFlags, APP_AFLAGS_BUSY);

	return res;
}

static DVR_RESULT pb_app_switch_app_by_appId(int appId)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();
		
	pb_app_player_close();
    

	DVR_U32 DVR_DisplayVison = 0x0; //0:NoRequest, 0x1:Front, 0x2:Rear, 0x3:Left, 0x4:Right, 0x5:MATTS
	DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_DISPLAY_VISION_UPDATE, (void *)&DVR_DisplayVison, NULL);		

    
    Log_Message("pb_app_switch_app_by_appId");
	res = DvrAppUtil_SwitchApp(appId);

	DVR_U32 AVM_DVRReplayCommandStatus = 0x0; //0:No Request, 0x1:Play, 0x2:Pause, 0x3:Invalid
	DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_PLAYBACK_STATUS_UPDATE, (void *)&AVM_DVRReplayCommandStatus, NULL);	

	GUI_OBJ_PLAY_TIME_INST play_time;
	play_time.position = 0;
	play_time.duration = 0;	
	DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_PLAY_TIMER_UPDATE, (void *)&play_time, NULL);			

	return res;
}

static DVR_RESULT pb_app_switch_app_by_tab(int tab)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();
	
	switch (tab)
	{
	case GUI_MAIN_MENU_TAB_RECORD:
	{
		res = app->Func(PB_APP_FUNC_SWITCH_APP_BY_APPID, Dvr_App_GetAppId(MDL_APP_RECORDER_ID), 0);
	}
	break;

	case GUI_MAIN_MENU_TAB_THUMB:
	{
		res = app->Func(PB_APP_FUNC_SWITCH_APP_BY_APPID, Dvr_App_GetAppId(MDL_APP_THUMB_ID), 0);
	}
	break;

	case GUI_MAIN_MENU_TAB_SETUP:
	{
		AppWidget_On(DVR_WIDGET_MENU_SETUP, 0);
		res = app->Func(PB_APP_FUNC_SWITCH_APP_BY_APPID, Dvr_App_GetAppId(MDL_APP_RECORDER_ID), 0);

    	DVR_U32 AVM_DVRModeFeedback = 0x3; //0:Inactive, 0x1:RealTimeMode, 0x2:ReplayMode, 0x3:SettingMode
        DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_MODE_UPDATE, (void *)&AVM_DVRModeFeedback, NULL);
	}	
	break;

	default:
		break;
	}

	return res;
}

static DVR_RESULT pb_app_player_play(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();

	res = Dvr_Sdk_Player_Play();

	app->playState = PB_APP_STATE_PLAYER_RUNNING;

	app->Gui(GUI_PB_APP_CMD_PLAY_STATE_UPDATE, GUI_PLAY_STATE_RUNNING, 4);
	app->Gui(GUI_PB_APP_CMD_FLUSH, 0, 0);

	return res;
}

static DVR_RESULT pb_app_player_stop(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();

	res = Dvr_Sdk_Player_Stop();

	app->playState = PB_APP_STATE_PLAYER_STOP;

	app->Gui(GUI_PB_APP_CMD_PLAY_STATE_UPDATE, GUI_PLAY_STATE_STOP, 4);
	app->Gui(GUI_PB_APP_CMD_FLUSH, 0, 0);

	return res;
}

static DVR_RESULT pb_app_player_pause(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();

	res = Dvr_Sdk_Player_Pause();
	if(res == DVR_RES_SOK)
	{
		app->playState = PB_APP_STATE_PLAYER_PAUSE;
	}

	app->Gui(GUI_PB_APP_CMD_PLAY_STATE_UPDATE, GUI_PLAY_STATE_PAUSE, 4);
	app->Gui(GUI_PB_APP_CMD_FLUSH, 0, 0);

	return res;
}

static DVR_RESULT pb_app_player_resume(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();

	res = Dvr_Sdk_Player_Resume();
	if(res == DVR_RES_SOK)
	{
		app->playState = PB_APP_STATE_PLAYER_RUNNING;
	}	

	app->Gui(GUI_PB_APP_CMD_PLAY_STATE_UPDATE, GUI_PLAY_STATE_RUNNING, 4);
	app->Gui(GUI_PB_APP_CMD_FLUSH, 0, 0);

	return res;
}

static DVR_RESULT pb_app_player_prev(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();

	long currentTime = pb_app_get_sys_time();
	if((currentTime - app->lastClickTime) < MIN_CLICK_DELAY_TIME)
	{
		Log_Message("pb_app_player_prev click gap[%d], click too frequently!!!!!!!!!!!",currentTime - app->lastClickTime);
		//app->lastClickTime = currentTime;
		return DVR_RES_SOK;
	}

	app->bAutoSwitchIsInterrupted = TRUE;
	app->lastClickTime = currentTime;
	app->playState = PB_APP_STATE_PLAYER_RUNNING;
	
	APP_STATUS *pStatus = (APP_STATUS *)Dvr_App_GetStatus();
	switch (pStatus->selFileType)
	{
	case DVR_FILE_TYPE_VIDEO:
		res = Dvr_Sdk_Player_Prev();
		break;

	case DVR_FILE_TYPE_IMAGE:
		res = Dvr_Sdk_Photo_Prev();
		break;

	default:
		return DVR_RES_EINVALIDARG;
	}

	return res;
}

static DVR_RESULT pb_app_player_next(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();

	long currentTime = pb_app_get_sys_time();
	if((currentTime - app->lastClickTime) < MIN_CLICK_DELAY_TIME)
	{
		Log_Message("pb_app_player_next click gap[%d], click too frequently!!!!!!!!!!!",currentTime - app->lastClickTime);
		//app->lastClickTime = currentTime;
		return DVR_RES_SOK;
	}

	app->bAutoSwitchIsInterrupted = TRUE;
	app->lastClickTime = currentTime;
	app->playState = PB_APP_STATE_PLAYER_RUNNING;

	APP_STATUS *pStatus = (APP_STATUS *)Dvr_App_GetStatus();
	switch (pStatus->selFileType)
	{
	case DVR_FILE_TYPE_VIDEO:
		res = Dvr_Sdk_Player_Next();
		break;

	case DVR_FILE_TYPE_IMAGE:
		res = Dvr_Sdk_Photo_Next();
		break;

	default:
		return DVR_RES_EINVALIDARG;
	}

	return res;
}

static DVR_RESULT pb_app_player_forward(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();

	APP_STATUS *pStatus = (APP_STATUS *)Dvr_App_GetStatus();

	long currentTime = pb_app_get_sys_time();
	if((currentTime - app->PBSpeedlastClickTime) < MIN_CLICK_DELAY_TIME)
	{
		Log_Message("pb_app_player_forward click gap[%d], click too frequently!!!!!!!!!!!",currentTime - app->PBSpeedlastClickTime);
		//app->PBSpeedlastClickTime = currentTime;
		return DVR_RES_SOK;
	}
	app->PBSpeedlastClickTime = currentTime;
	//Log_Message("pb_app_player_forward click gap[%d], click too frequently!!!!!!!!!!!",currentTime - app->PBSpeedlastClickTime);

	switch (pStatus->selFileType)
	{
	case DVR_FILE_TYPE_VIDEO:
		res = Dvr_Sdk_Player_StepF();
		break;

	default:
		return DVR_RES_EINVALIDARG;
	}

	return res;
}

static DVR_RESULT pb_app_player_rewind(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();

	APP_STATUS *pStatus = (APP_STATUS *)Dvr_App_GetStatus();

	long currentTime = pb_app_get_sys_time();
	if((currentTime - app->PBSpeedlastClickTime) < MIN_CLICK_DELAY_TIME)
	{
		Log_Message("pb_app_player_rewind click gap[%d], click too frequently!!!!!!!!!!!",currentTime - app->PBSpeedlastClickTime);
		//app->PBSpeedlastClickTime = currentTime;
		return DVR_RES_SOK;
	}
	app->PBSpeedlastClickTime = currentTime;
	
	switch (pStatus->selFileType)
	{
	case DVR_FILE_TYPE_VIDEO:
		res = Dvr_Sdk_Player_StepB();
		break;

	default:
		return DVR_RES_EINVALIDARG;
	}

	return res;
}

static DVR_RESULT pb_app_player_set_speed(DVR_U32 option)
{
	DVR_RESULT res = DVR_RES_SOK;
	DVR_U32 speed;

	switch (option)
	{
	case GUI_PLAY_SPEED_X1:
		speed = 1000;
		break;
	case GUI_PLAY_SPEED_X2:
		speed = 2000;
		break;
	case GUI_PLAY_SPEED_X4:
		speed = 4000;
		break;
	case GUI_PLAY_SPEED_X8:
		speed = 8000;
		break;
	default:
		return DVR_RES_EINVALIDARG;
	}

	res = Dvr_Sdk_Player_Set(DVR_PLAYER_PROP_SPEED, (DVR_VOID *)&speed, sizeof(DVR_U32));

	return res;
}

static DVR_RESULT pb_app_player_set_position(DVR_U32 position)
{
	DVR_RESULT res = DVR_RES_SOK;

	Log_Message("pb_app_player_set_position set position to %d", position);

	res = Dvr_Sdk_Player_Set(DVR_PLAYER_PROP_POSITION, (DVR_VOID *)&position, sizeof(DVR_U32));

	return res;
}

static void print_screen_result_feedback_success(void *arg)
{
	DVR_U32 DVR_PrintScreenFeedback = 1;//0x0=invalid,0x1=success,0x2=fail,0x3=reserved
    DvrAppNotify *pHandle = DvrAppNotify::Get();
		
    if (pHandle != NULL)
        pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_PRINTSCREEN_RESULT, (void *)&DVR_PrintScreenFeedback, NULL);

#ifdef __linux__
		usleep(500*1000); //wait 500ms
#endif

	DVR_PrintScreenFeedback = 0;
	if (pHandle != NULL)
		pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_PRINTSCREEN_RESULT, (void *)&DVR_PrintScreenFeedback, NULL);
}

static void print_screen_result_feedback_failed(void *arg)
{
	DVR_U32 DVR_PrintScreenFeedback = 2;//0x0=invalid,0x1=success,0x2=fail,0x3=reserved
    DvrAppNotify *pHandle = DvrAppNotify::Get();
		
    if (pHandle != NULL)
        pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_PRINTSCREEN_RESULT, (void *)&DVR_PrintScreenFeedback, NULL);

#ifdef __linux__
		usleep(500*1000); //wait 500ms
#endif

	DVR_PrintScreenFeedback = 0;
	if (pHandle != NULL)
		pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_PRINTSCREEN_RESULT, (void *)&DVR_PrintScreenFeedback, NULL);
}

static DVR_RESULT pb_app_player_print_screen(void)
{
	DVR_RESULT res = DVR_RES_SOK;
    DvrPlayBackApp *app = DvrPlayBackApp::Get();

	res = Dvr_Sdk_StorageCard_CheckStatus();
	if(APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_PHOTO_FOLDER_NOT_ENOUGH_SPACE) ||
		APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_LOW_SPEED) ||
		APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NOT_ENOUGH_SPACE))
	{
		CThreadPool::Get()->AddTask(print_screen_result_feedback_failed, NULL);	
		return res;
	}

	if(app->OpenFileResult != DVR_RES_SOK)
	{
		CThreadPool::Get()->AddTask(print_screen_result_feedback_failed, NULL);	
		return DVR_RES_EFAIL;
	}
	
    DVR_PHOTO_PARAM param;
    param.eType = DVR_PHOTO_TYPE_JPG;
    param.eQuality = DVR_PHOTO_QUALITY_SFINE;

    switch (app->curViewIndx)
    {
	    case GUI_VIEW_INDEX_FRONT:
	    {
			param.eIndex = DVR_VIEW_INDEX_FRONT;
			Dvr_Sdk_Player_PrintScreen(&param);
	    }
	    break;

	    case GUI_VIEW_INDEX_REAR:
	    {
	        param.eIndex = DVR_VIEW_INDEX_REAR;
			Dvr_Sdk_Player_PrintScreen(&param);		
	    }	
	    break;

	    case GUI_VIEW_INDEX_LEFT:
	    {
	        param.eIndex = DVR_VIEW_INDEX_LEFT;
			Dvr_Sdk_Player_PrintScreen(&param);			
	    }
	    break;

	    case GUI_VIEW_INDEX_RIGHT:
	    {
	        param.eIndex = DVR_VIEW_INDEX_RIGHT;
			Dvr_Sdk_Player_PrintScreen(&param);		
	    }
	    break;

		case GUI_VIEW_INDEX_MATTS:
		{
			param.eIndex = DVR_VIEW_INDEX_FRONT;
			Dvr_Sdk_Player_PrintScreen(&param);

#ifdef DVR_FEATURE_V302
	        param.eIndex = DVR_VIEW_INDEX_REAR;
			Dvr_Sdk_Player_PrintScreen(&param);

	        param.eIndex = DVR_VIEW_INDEX_LEFT;
			Dvr_Sdk_Player_PrintScreen(&param);

	        param.eIndex = DVR_VIEW_INDEX_RIGHT;
			Dvr_Sdk_Player_PrintScreen(&param);
#endif
		}
		break;

	    default:
	    {
	        Log_Error("Not Support take photo in view [%d]", app->curViewIndx);
	        return DVR_RES_EFAIL;
	    }
    }

	CThreadPool::Get()->AddTask(print_screen_result_feedback_success, NULL);

	return res;
}

static DVR_RESULT pb_app_card_insert(void)
{
	DVR_RESULT res = DVR_RES_SOK;

	return res;
}

static DVR_RESULT pb_app_card_remove(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();
	
	res = app->Func(PB_APP_FUNC_QUIT, 0, 0);
    
    DVR_U32 DVR_SDCardErrorStatus = DVR_SDCARD_ERROR_STATUS_NO_SDCARD;
    DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_ERROR_STATUS_UPDATE, (void *)&DVR_SDCardErrorStatus, NULL);
	return res;
}

static DVR_RESULT pb_app_card_format(void)
{
	DVR_RESULT res = DVR_RES_SOK;

	pb_app_player_close();

	Dvr_Sdk_ComSvrTimer_UnRegister(TIMER_2HZ, pb_app_timer_handler);

	return res;
}

static int pb_app_dialog_del_handler(DVR_U32 sel, DVR_U32 param1, DVR_U32 param2)
{
	int ReturnValue = 0;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();

	switch (sel) {
	case DIALOG_SEL_YES:
		app->Func(PB_APP_FUNC_DELETE_FILE, 0, 0);
		break;
	case DIALOG_SEL_NO:
	default:
		break;
	}

	return ReturnValue;
}

static DVR_RESULT pb_app_delete_file_dialog_show(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();

	AppDialog_SetDialog(GUI_DIALOG_TYPE_Y_N, DIALOG_SUBJECT_DEL, pb_app_dialog_del_handler);
	AppWidget_On(DVR_WIDGET_DIALOG, 0);
	APP_ADDFLAGS(app->m_playback.GFlags, APP_AFLAGS_POPUP);

	return res;
}

static DVR_RESULT pb_app_delete_current_file(void)
{
	DVR_U32 AVM_DVRDeleteStatus;
	DVR_RESULT res = DVR_RES_SOK;	
	DvrPlayBackApp *app = DvrPlayBackApp::Get();
    DvrAppNotify *pHandle = DvrAppNotify::Get();
	APP_STATUS *pStatus = (APP_STATUS *)Dvr_App_GetStatus();
	char CurFile[APP_MAX_FN_SIZE];
	DVR_DEVICE CurDrive;
    unsigned uRelPos = 0;
    const char* szFileName = CurFile;

	memset(&CurDrive, 0, sizeof(DVR_DEVICE));
	Dvr_Sdk_GetActiveDrive(&CurDrive);
	if (!strcmp(CurDrive.szMountPoint, ""))
		return DVR_RES_EFAIL;
	
	AVM_DVRDeleteStatus = 1;//deleting
    if (pHandle != NULL)
        pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_DELETE_STATUS, (void *)&AVM_DVRDeleteStatus, NULL);

	if(pStatus->selFileType == DVR_FILE_TYPE_VIDEO)
	{
		res = Dvr_Sdk_Player_Get(DVR_PLAYER_PROP_CURRENT_FILE, CurFile, APP_MAX_FN_SIZE, NULL);
		if (DVR_SUCCEEDED(res))
		{
			Dvr_Sdk_Player_Close();

			Log_Message("Dvr_Sdk_ComSvcSyncOp_FileDel file: %s", CurFile);

            DVR_DB_IDXFILE idxfile;
            Dvr_Sdk_FileMapDB_GePosByName(&uRelPos, szFileName, pStatus->selFolderType);
        	if(DVR_RES_SOK == Dvr_Sdk_FileMapDB_GetFileInfo(CurDrive.szMountPoint, CurFile, pStatus->selFolderType, &idxfile))
        	{
				res |= Dvr_Sdk_ComSvcSyncOp_FileDel(CurFile);
				if(DVR_SUCCEEDED(res))
				{
					Dvr_Sdk_FileMapDB_DelItem(CurFile, pStatus->selFolderType);
					Log_Message("pb_app_delete_current_file: delete file [%s] successfully, res = 0x%x\n", CurFile, res);					
				}
				else
				{
					Log_Message("pb_app_delete_current_file: delete file [%s] failed, res = 0x%x\n", CurFile, res);
				}
			}
			else
			{
				Log_Message("[%d]Dvr_Sdk_FileMapDB_GetFileInfo for file[%s] return failed", __LINE__, CurFile);
				res = DVR_RES_EFAIL;
			}

			if(DVR_SUCCEEDED(res))
			{
				AVM_DVRDeleteStatus = 2;//delete complete
			}
			else
			{
				AVM_DVRDeleteStatus = 3;//delete failed
			}			
		}

#ifdef __linux__
		usleep(300*1000);
#endif
		
		if (pHandle != NULL)
			pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_DELETE_STATUS, (void *)&AVM_DVRDeleteStatus, NULL);

#ifdef __linux__
        usleep(300*1000);
#endif
        AVM_DVRDeleteStatus = 0;
        if (pHandle != NULL)
            pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_DELETE_STATUS, (void *)&AVM_DVRDeleteStatus, NULL);

		DVR_U32 nTotalFileNum = 0;
		Dvr_Sdk_FileMapDB_GetItemCountByType(&nTotalFileNum, pStatus->selFolderType);
		if(nTotalFileNum != 0)
		{
			Dvr_Sdk_Player_Advance(uRelPos);
		}
		else
		{
			Log_Message("No file left, switch to thumb app!!!");
	        /*No file left, switch to thumb app*/
			res = app->Func(PB_APP_FUNC_QUIT, 0, 0);
		}
	}
	else
	{
        res = Dvr_Sdk_Player_Get(DVR_PLAYER_PROP_CURRENT_FILE, CurFile, APP_MAX_FN_SIZE, NULL);
		if (DVR_SUCCEEDED(res))
		{
			Dvr_Sdk_Photo_Close();

            DVR_DB_IDXFILE idxfile;
            Dvr_Sdk_FileMapDB_GePosByName(&uRelPos, szFileName, pStatus->selFolderType);
        	if(DVR_RES_SOK == Dvr_Sdk_FileMapDB_GetFileInfo(CurDrive.szMountPoint, CurFile, pStatus->selFolderType, &idxfile))
			{
				res = Dvr_Sdk_ComSvcSyncOp_FileDel(CurFile);
				if(DVR_SUCCEEDED(res))
				{
					Dvr_Sdk_FileMapDB_DelItem(CurFile, pStatus->selFolderType);
					Log_Message("pb_app_delete_current_file: delete file [%s] successfully, res = 0x%x\n", CurFile, res);					
				}
				else
				{
					Log_Message("pb_app_delete_current_file: delete file [%s] failed, res = 0x%x\n", CurFile, res);
				}
			}
			else
			{
				Log_Message("[%d]Dvr_Sdk_FileMapDB_GetFileInfo for file[%s] return failed", __LINE__, CurFile);
				res = DVR_RES_EFAIL;
			}

			if(DVR_SUCCEEDED(res))
			{
				AVM_DVRDeleteStatus = 2;//delete complete
			}
			else
			{
				AVM_DVRDeleteStatus = 3;//delete failed
			}
		}

#ifdef __linux__
		usleep(300*1000);
#endif		
		
		if (pHandle != NULL)
			pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_DELETE_STATUS, (void *)&AVM_DVRDeleteStatus, NULL);

#ifdef __linux__
            usleep(300*1000);
#endif
        AVM_DVRDeleteStatus = 0;
        if (pHandle != NULL)
            pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_DELETE_STATUS, (void *)&AVM_DVRDeleteStatus, NULL);

		DVR_U32 nTotalFileNum = 0;
		Dvr_Sdk_FileMapDB_GetItemCountByType(&nTotalFileNum, pStatus->selFolderType);
		if(nTotalFileNum != 0)
		{
			Dvr_Sdk_Photo_Advance(uRelPos);
		}
		else
		{
			Log_Message("No file left, switch to thumb app!!!");
	        /*No file left, switch to thumb app*/
			res = app->Func(PB_APP_FUNC_QUIT, 0, 0);
		}
	}
	
	CThreadPool::Get()->AddTask(system_sync, NULL);	

	return res;
}

static DVR_RESULT pb_app_format_save_file_fullname(char *srcFn, char *dstFolder, char *dstFn)
{
    char *szSrcFullName = srcFn;
    char *szDstFolder = dstFolder;
    char *slash;

    std::string dst_file_location;
    char *szSrcFileName = (slash = strrchr(szSrcFullName, '/')) ? (slash + 1) : szSrcFullName;

    dst_file_location = szDstFolder;
    dst_file_location = dst_file_location + szSrcFileName;

    strcpy(dstFn, dst_file_location.data());

    return DVR_RES_SOK;
}

static void pb_app_save_result_feedback_success(void *arg)
{
	DVR_U32 DVR_NormalToEmergency = 3;//save faild
    DvrAppNotify *pHandle = DvrAppNotify::Get();

    if (pHandle != NULL)
		pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_NORMAL_TO_EMERGENCY_STATUS, (void *)&DVR_NormalToEmergency, NULL);

#ifdef __linux__
		usleep(500*1000); //wait 500ms
#endif

	DVR_NormalToEmergency = 0;
	if (pHandle != NULL)
		pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_NORMAL_TO_EMERGENCY_STATUS, (void *)&DVR_NormalToEmergency, NULL);
}

static void pb_app_save_result_feedback_failed(void *arg)
{
	DVR_U32 DVR_NormalToEmergency = 4;//save faild
    DvrAppNotify *pHandle = DvrAppNotify::Get();

    if (pHandle != NULL)
		pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_NORMAL_TO_EMERGENCY_STATUS, (void *)&DVR_NormalToEmergency, NULL);

#ifdef __linux__
		usleep(500*1000); //wait 500ms
#endif

	DVR_NormalToEmergency = 0;
	if (pHandle != NULL)
		pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_NORMAL_TO_EMERGENCY_STATUS, (void *)&DVR_NormalToEmergency, NULL);
}

static DVR_RESULT pb_app_save_current_file(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DVR_U32 DVR_NormalToEmergency;
    DVR_DB_IDXFILE idxfile;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();
    DvrAppNotify *pHandle = DvrAppNotify::Get();
    APP_STATUS *pStatus = (APP_STATUS *)Dvr_App_GetStatus();
	DVR_DEVICE CurDrive;

	memset(&CurDrive, 0, sizeof(DVR_DEVICE));
	Dvr_Sdk_GetActiveDrive(&CurDrive);
	if (!strcmp(CurDrive.szMountPoint, ""))
		return DVR_RES_EFAIL;

    DVR_RESULT state = DVR_RES_SOK;
	char CurFile[APP_MAX_FN_SIZE];
    char folder_name[APP_MAX_FN_SIZE];
    unsigned uRelPos = 0;
    const char* szFileName = CurFile;

	res = Dvr_Sdk_StorageCard_CheckStatus();
	if(APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_EVENT_FOLDER_NOT_ENOUGH_SPACE) ||
		APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_LOW_SPEED) ||
		APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NOT_ENOUGH_SPACE))
	{
		CThreadPool::Get()->AddTask(pb_app_save_result_feedback_failed, NULL);	
		return DVR_RES_EFAIL;
	}

    memset(CurFile, 0, sizeof(CurFile));
	res     = Dvr_Sdk_Player_Get(DVR_PLAYER_PROP_CURRENT_FILE, CurFile, APP_MAX_FN_SIZE, NULL);
    state   = Dvr_Sdk_FileMapDB_GetFileInfo(CurDrive.szMountPoint, CurFile, pStatus->selFolderType, &idxfile);

    memset(folder_name, 0, sizeof(folder_name));
    Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_EMERGENCY_FOLDER, folder_name, sizeof(folder_name), NULL);

	DVR_NormalToEmergency = 2;//saving
    if (pHandle != NULL)
        pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_NORMAL_TO_EMERGENCY_STATUS, (void *)&DVR_NormalToEmergency, NULL);
	if (DVR_SUCCEEDED(res))
	{
		Dvr_Sdk_Player_Close();
		
		Log_Message("Dvr_Sdk_ComSvcSyncOp_FileMove file: %s", CurFile);
	
        if(state == DVR_RES_SOK)
		{
			DVR_FILEMAP_META_ITEM stEventItem;
			memset(&stEventItem, 0, sizeof(DVR_FILEMAP_META_ITEM));
            Dvr_Sdk_FileMapDB_GePosByName(&uRelPos, szFileName, pStatus->selFolderType);
			pb_app_format_save_file_fullname(CurFile, folder_name, stEventItem.szMediaFileName);

			Dvr_Sdk_ComSvcSyncOp_FileMove(CurFile, stEventItem.szMediaFileName);

			if(DVR_SUCCEEDED(res))
			{
				Dvr_Sdk_FileMapDB_AddItem(&stEventItem, DVR_FOLDER_TYPE_EMERGENCY);
				Dvr_Sdk_FileMapDB_DelItem(CurFile, pStatus->selFolderType);

				Log_Message("copy file %s to %s complete, res = 0x%x\n", CurFile, folder_name, res);
			}
		}
		else
		{
			Log_Message("[%d]Dvr_Sdk_FileMapDB_GetFileInfo for file[%s] return failed", __LINE__, CurFile);
			res = DVR_RES_EFAIL;
		}
	}

#ifdef __linux__
	usleep(300*1000);
#endif
	
	CThreadPool::Get()->AddTask(pb_app_save_result_feedback_success, NULL);

	DVR_U32 nTotalFileNum = 0;
	Dvr_Sdk_FileMapDB_GetItemCountByType(&nTotalFileNum, pStatus->selFolderType);
	if(nTotalFileNum != 0)
	{
		Dvr_Sdk_Player_Advance(uRelPos);
	}
	else
	{
		Log_Message("No file left, switch to thumb app!!!");
		/*No file left, switch to thumb app*/
		res = app->Func(PB_APP_FUNC_QUIT, 0, 0);		
	}
	
	CThreadPool::Get()->AddTask(system_sync, NULL);	

	return DVR_RES_SOK;
}

static DVR_RESULT pb_app_change_display(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();

#ifdef __linux__
	DVR_GUI_LAYOUT_INST_EXT RenderData;
	memset(&RenderData, 0, sizeof(DVR_GUI_LAYOUT_INST_EXT));
	DvrGuiResHandler *handler = (DvrGuiResHandler *)Dvr_App_GetGuiResHandler();
#endif

	APP_STATUS *pStatus = (APP_STATUS *)Dvr_App_GetStatus();
	switch (pStatus->selFileType)
	{
	case DVR_FILE_TYPE_VIDEO:
	{
		DVR_U32 DVR_DisplayVison = 0x5; //0:NoRequest, 0x1:Front, 0x2:Rear, 0x3:Left, 0x4:Right, 0x5:MATTS
		DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_DISPLAY_VISION_UPDATE, (void *)&DVR_DisplayVison, NULL);

#ifdef __linux__
		usleep(300*1000); //make sure HU receive the display vision signal
#endif
		app->Gui(GUI_PB_APP_CMD_SET_LAYOUT, GUI_LAYOUT_PLAYBACK_VIDEO, 0);
		app->Gui(GUI_PB_APP_CMD_VIEW_UPDATE, GUI_VIEW_INDEX_MATTS, 4);
		app->Gui(GUI_PB_APP_CMD_SIDEBAR_HIDE, 0, 0);
		app->Gui(GUI_PB_APP_CMD_FLUSH, 0, 0);

		app->curViewIndx = GUI_VIEW_INDEX_MATTS;

#ifdef __linux__
		Log_Message("Set GUI_LAYOUT_PLAYBACK_VIDEO Layout!!!!!!");
		RenderData.curLayout = GUI_LAYOUT_PLAYBACK_VIDEO_EXT;
		handler->GetLayoutInfo(GUI_LAYOUT_PLAYBACK_VIDEO, (DVR_GRAPHIC_UIOBJ **)&RenderData.pTable, &RenderData.ObjNum);	
		UpdateRenderDvrData((DVR_GUI_LAYOUT_INST_EXT*)&RenderData, sizeof(DVR_GUI_LAYOUT_INST_EXT));
#endif
	}
	break;

	case DVR_FILE_TYPE_IMAGE:
	{
		DVR_U32 DVR_DisplayVison = 0x1; //0:NoRequest, 0x1:Front, 0x2:Rear, 0x3:Left, 0x4:Right, 0x5:MATTS
		DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_DISPLAY_VISION_UPDATE, (void *)&DVR_DisplayVison, NULL);
		
#ifdef __linux__
		usleep(300*1000); //make sure HU receive the display vision signal
#endif	
		app->Gui(GUI_PB_APP_CMD_SET_LAYOUT, GUI_LAYOUT_PLAYBACK_IMAGE, 0);
		app->Gui(GUI_PB_APP_CMD_VIEW_UPDATE, GUI_VIEW_INDEX_FRONT, 4);
		app->Gui(GUI_PB_APP_CMD_SIDEBAR_HIDE, 0, 0);
		app->Gui(GUI_PB_APP_CMD_FLUSH, 0, 0);

		app->curViewIndx = GUI_VIEW_INDEX_FRONT;

#ifdef __linux__
		Log_Message("Set GUI_LAYOUT_PLAYBACK_IMAGE Layout!!!!!!");
		RenderData.curLayout = GUI_LAYOUT_PLAYBACK_IMAGE_EXT;
		handler->GetLayoutInfo(GUI_LAYOUT_PLAYBACK_IMAGE, (DVR_GRAPHIC_UIOBJ **)&RenderData.pTable, &RenderData.ObjNum);	
		UpdateRenderDvrData((DVR_GUI_LAYOUT_INST_EXT*)&RenderData, sizeof(DVR_GUI_LAYOUT_INST_EXT));
#endif
	}
	break;

	default:
		return DVR_RES_EINVALIDARG;
	}

	return res;
}

static DVR_RESULT pb_app_change_mode(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();

    app->playMode = (PB_APP_MODE)((app->playMode + 1) % PB_APP_MODE_NUM);

    app->Gui(GUI_PB_APP_CMD_MODE_UPDATE, app->playMode, 4);
    app->Gui(GUI_PB_APP_CMD_FLUSH, 0, 0);

	return res;
}

static void pb_app_async_switch_to_next_file(void *arg)
{
	DvrPlayBackApp *app = DvrPlayBackApp::Get();

	app->bAutoSwitchIsInterrupted = FALSE;

#ifdef __linux__
	usleep(2000*1000); //wait 2000ms
#endif

	if(TRUE == app->bAutoSwitchIsInterrupted)
	{
		Log_Message("Auto Switch is Interrupted, return!!!!");
		return;
	}

	APP_STATUS *pStatus = (APP_STATUS *)Dvr_App_GetStatus();
	switch (pStatus->selFileType)
	{
	case DVR_FILE_TYPE_VIDEO:
		Dvr_Sdk_Player_Next();
		break;

	case DVR_FILE_TYPE_IMAGE:
		Dvr_Sdk_Photo_Next();
		break;

	default:
		break;
	}	
}

static DVR_RESULT pb_app_decode_error(int result)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();

	if(result == DVR_RES_SOK)
	{
		app->Gui(GUI_PB_APP_CMD_WARNING_HIDE, GUI_WARNING_ERROR_FILE, 0);
		
		GUI_OBJ_PLAY_TIME_INST play_time;
		play_time.position = 0;
		play_time.duration = 0;	
		DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_PLAY_TIMER_UPDATE, (void *)&play_time, NULL);
	}
	else
	{
		app->Gui(GUI_PB_APP_CMD_WARNING_SHOW, GUI_WARNING_ERROR_FILE, 0);
		
		GUI_OBJ_PLAY_TIME_INST play_time;
		play_time.position = 0;
		play_time.duration = 0;	
		DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_PLAY_TIMER_UPDATE, (void *)&play_time, NULL);

		CThreadPool::Get()->AddTask(pb_app_async_switch_to_next_file, NULL);	
	}

	app->OpenFileResult = result;
	app->Gui(GUI_PB_APP_CMD_FLUSH, 0, 0);
	
	DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_PLAYER_OPEN_RESULT, (void *)&result, NULL); 	

	return res;
}

static int pb_app_set_view_index(int view)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrPlayBackApp *app = DvrPlayBackApp::Get();
	APP_STATUS *pStatus = (APP_STATUS *)Dvr_App_GetStatus();

	if (view >= GUI_VIEW_INDEX_NUM)
	{
		Log_Error("view index out of range\n");
		return DVR_RES_EFAIL;
	}

	if(pStatus->selFileType == DVR_FILE_TYPE_IMAGE)
	{
		Log_Error("image playback do not support view change\n");
		return DVR_RES_EFAIL;
	}

    app->curViewIndx = (GUI_VIEW_INDEX)view;
	app->Gui(GUI_PB_APP_CMD_VIEW_UPDATE, view, 4);
	app->Gui(GUI_PB_APP_CMD_FLUSH, 0, 0);

    {    
    	DVR_U32 DVR_DisplayVison = 0; //0:NoRequest, 0x1:Front, 0x2:Rear, 0x3:Left, 0x4:Right, 0x5:MATTS
        DvrAppNotify *pHandle = DvrAppNotify::Get();
        if (pHandle != NULL)
        {
        	if(view == GUI_VIEW_INDEX_FRONT){
        		DVR_DisplayVison = 1;}
			else if(view == GUI_VIEW_INDEX_REAR){
				DVR_DisplayVison = 2;}
			else if(view == GUI_VIEW_INDEX_LEFT){
				DVR_DisplayVison = 3;}
			else if(view == GUI_VIEW_INDEX_RIGHT){
				DVR_DisplayVison = 4;}
			else if(view == GUI_VIEW_INDEX_MATTS){
				DVR_DisplayVison = 5;}
			else{
			}
			
            pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_DISPLAY_VISION_UPDATE, (void *)&DVR_DisplayVison, NULL);
        }
	}

	return res;
}

int pb_app_set_sidebar_status(DVR_U32 param1)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrPlayBackApp *app = DvrPlayBackApp::Get();

    if (param1 >= GUI_SIDEBAR_STATUS_NUM)
    {
        Log_Error("side bar status out of range\n");
        return DVR_RES_EFAIL;
    }

    if (param1 == GUI_SIDEBAR_STATUS_SHOW)
    {
        app->Gui(GUI_PB_APP_CMD_SIDEBAR_SHOW, 0, 0);
    }
    else
    {
        app->Gui(GUI_PB_APP_CMD_SIDEBAR_HIDE, 0, 0);
    }

    app->Gui(GUI_PB_APP_CMD_FLUSH, 0, 0);

    return res;
}

DVR_RESULT pb_app_func(DVR_U32 funcId, DVR_U32 param1, DVR_U32 param2)
{
	DVR_RESULT res = DVR_RES_SOK;

	switch (funcId)
	{
	case PB_APP_FUNC_INIT:
		res = pb_app_init();
		break;

	case PB_APP_FUNC_START:
		res = pb_app_start();
		break;

	case PB_APP_FUNC_STOP:
		res = pb_app_stop();
		break;

	case PB_APP_FUNC_QUIT:
		res = pb_app_player_quit();
		break;		

	case PB_APP_FUNC_APP_READY:
		res = pb_app_ready();
		break;

	case PB_APP_FUNC_START_DISP_PAGE:
		res = pb_app_start_disp_page();
		break;

	case PB_APP_FUNC_SWITCH_APP_BY_APPID:
		res = pb_app_switch_app_by_appId(param1);
		break;

    case PB_APP_FUNC_SWITCH_APP_BY_TAB:
        res = pb_app_switch_app_by_tab(param1);
        break;		

	case PB_APP_FUNC_OPEN_FILE:
		res = pb_app_player_open();
		break;

	case PB_APP_FUNC_CLOSE_FILE:
		res = pb_app_player_close();
		break;

	case PB_APP_FUNC_START_PLAY:
		res = pb_app_player_play();
		break;

	case PB_APP_FUNC_STOP_PLAY:
		res = pb_app_player_stop();
		break;

	case PB_APP_FUNC_PAUSE_PLAY:
		res = pb_app_player_pause();
		break;

	case PB_APP_FUNC_RESUME_PLAY:
		res = pb_app_player_resume();
		break;

	case PB_APP_FUNC_FORWARD_PLAY:
		res = pb_app_player_forward();
		break;

	case PB_APP_FUNC_REWIND_PLAY:
		res = pb_app_player_rewind();
		break;

	case PB_APP_FUNC_PREV_FILE:
		res = pb_app_player_prev();
		break;

	case PB_APP_FUNC_NEXT_FILE:
		res = pb_app_player_next();
		break;

	case PB_APP_FUNC_SET_SPEED:
		res = pb_app_player_set_speed(param1);
		break;

	case PB_APP_FUNC_SET_POSITION:
		res = pb_app_player_set_position(param1);
		break;

	case PB_APP_FUNC_SCREEN_SHOT:
		res = pb_app_player_print_screen();
		break;

	case PB_APP_FUNC_CARD_INSERT:
		res = pb_app_card_insert();
		break;

	case PB_APP_FUNC_CARD_REMOVE:
		res = pb_app_card_remove();
		break;

	case PB_APP_FUNC_CARD_FORMAT:
		res = pb_app_card_format();
		break;

	case PB_APP_FUNC_DELETE_FILE:
		res = pb_app_delete_current_file();
		break;

	case PB_APP_FUNC_DELETE_FILE_DIALOG_SHOW:
		res = pb_app_player_pause();
		res = pb_app_delete_file_dialog_show();
		break;

	case PB_APP_FUNC_SAVE_FILE:
		res = pb_app_save_current_file();
		break;

	case PB_APP_FUNC_CHANGE_DISPLAY:
		res = pb_app_change_display();
		break;

	case PB_APP_FUNC_CHANGE_MODE:
		res = pb_app_change_mode();
		break;

	case PB_APP_FUNC_DECODE_ERROR:
		res = pb_app_decode_error(param1);
		break;

	case PB_APP_FUNC_SET_VIEW_INDEX:
		res = pb_app_set_view_index(param1);
		break;

    case PB_APP_FUNC_SET_SIDEBAR_STATUS:
        res = pb_app_set_sidebar_status(param1);
        break;

	default:
		break;
	}

	return res;
}
