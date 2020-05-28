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
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "flow/rec/DvrRecApp.h"
#include "flow/widget/DvrWidget_Mgt.h"
#include "flow/util/CThreadPool.h"
#include "flow/util/DvrSDCardSpeedTest.h"
#include "gui/Gui_DvrRecApp.h"
#include "framework/DvrAppNotify.h"
#include <log/log.h>
#include <event/ofilm_msg_type.h>

#include "Gpu_Dvr_Interface.h"

static DVR_RESULT rec_app_init(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrRecApp *app = DvrRecApp::Get();

	app->bUserEnableRec = TRUE;
	app->RecCapState    = REC_CAP_STATE_PREVIEW;
	app->EventRecState  = EVENT_REC_STATE_IDLE;
	app->bEncodingHasStarted = FALSE;
	app->bPowerMode = 1;

	

	app->AlarmRecState = EVENT_REC_STATE_IDLE;
    app->IaccRecState = EVENT_REC_STATE_IDLE;
    app->DasRecState = EVENT_REC_STATE_IDLE;
    app->DasRecType = DVR_DAS_TYPE_NONE;
    app->AlarmRecLastLimitTime = 0;
    app->bIacccontinue = FALSE;
    app->bIaccHasFinshed = FALSE;
    app->bEventcontinue = FALSE;
    app->beventRecHasTrigger = FALSE;
    app->bIaccFailedHasFinshed = FALSE;
	app->bDasStopCommandHasFired = FALSE;
    app->bAlarmRecHasTrigger = FALSE;
    app->bIaccApaIsInterrupted = FALSE;
	app->bDasStopDelayProcessIsInterrupted = FALSE;

	DVR_U32 setting;
	/** set free space threshold*/
	setting = DVR_LOOPREC_STORAGE_FREESPACE_THRESHOLD;
	res = Dvr_Sdk_StorageCard_Set(DVR_STORAGE_PROP_THRESHOLD, &setting, sizeof(DVR_U32));/**<set card check threshold*/
	res |= Dvr_Sdk_MonitorStorage_Set(DVR_MONITOR_STORAGE_PROP_LOOPREC_THRESHOLD, &setting, sizeof(DVR_U32));/**<set storage monitor threshold*/	

	res |= Dvr_Sdk_Recorder_Init();
	
	return res;
}

static void system_sync(void *arg)
{
#ifdef __linux__
	sync();
#endif
	return;
}

void DvrRecorderOnNewVehicleData(void *pInstance, void *pUserData)
{
	Ofilm_Can_Data_T *pVehicleData = (Ofilm_Can_Data_T *)pUserData;

    if (pVehicleData == NULL)
    {
        return;
    }

	DvrRecApp *app = DvrRecApp::Get();
	
	GUI_OBJ_VEHICLE_DATA_INST vehicle_data;
	memset(&vehicle_data, 0, sizeof(vehicle_data));

	vehicle_data.TimeYear = pVehicleData->vehicle_data.year + 2013;
	vehicle_data.TimeMon = pVehicleData->vehicle_data.month;
	vehicle_data.TimeDay = pVehicleData->vehicle_data.day;
	vehicle_data.TimeHour = pVehicleData->vehicle_data.hour;
	vehicle_data.TimeMin = pVehicleData->vehicle_data.minute;
	vehicle_data.TimeSec = pVehicleData->vehicle_data.second;

	if(pVehicleData->vehicle_data.vehicle_speed > 254.0f)
	{
		vehicle_data.VehicleSpeed =254.0;
	}
	else if(pVehicleData->vehicle_data.vehicle_speed > 0.0f)
	{
		vehicle_data.VehicleSpeed =pVehicleData->vehicle_data.vehicle_speed;	// (unsigned short)(2.0f + 1.02f * pVehicleData->vehicle_data.vehicle_speed);
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
	else if(pVehicleData->vehicle_data.EmergencyLightstatus == 1)
	{
		vehicle_data.LeftTurnLampStatus = 1;
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

	absGpsLongitude = (absGpsLongitude > 180.0f) ? 180.0f : absGpsLongitude;
	absGpsLatitude = (absGpsLatitude > 90.0f) ? 90.0f : absGpsLatitude;

	vehicle_data.GpsLongitude_IsEast = (GpsLongitude >= 0) ? 1 : 0;
	vehicle_data.GpsLongitude_Deg = (uint16_t)absGpsLongitude;
	vehicle_data.GpsLongitude_Min = (uint16_t)((float)((float)absGpsLongitude - vehicle_data.GpsLongitude_Deg) * 60);
	vehicle_data.GpsLongitude_Sec = (uint16_t)((((float)((float)absGpsLongitude - vehicle_data.GpsLongitude_Deg) * 60) - vehicle_data.GpsLongitude_Min) * 60)  ;
	
	vehicle_data.GpsLatitude_IsNorth = (GpsLatitude >= 0) ? 1 : 0;
	vehicle_data.GpsLatitude_Deg = (uint16_t)absGpsLatitude;
	vehicle_data.GpsLatitude_Min = (uint16_t)((float)((float)absGpsLatitude - vehicle_data.GpsLatitude_Deg) * 60);
	vehicle_data.GpsLatitude_Sec = (uint16_t)((((float)((float)absGpsLatitude - vehicle_data.GpsLatitude_Deg) * 60) - vehicle_data.GpsLatitude_Min) * 60)  ;

    vehicle_data.LateralAcceleration = pVehicleData->vehicle_data.Lateral_Acceleration;
    vehicle_data.LongitudinalAcceleration = pVehicleData->vehicle_data.Longitudinal_Acceleration;
    vehicle_data.ApaRecstatus = (app->DasRecState == EVENT_REC_STATE_RUNNING) ? 1 : 0;
    vehicle_data.ApaRecstatus = (pVehicleData->vehicle_data.APA_LSCAction == 0x1) ? 1 : 0;

    int iacctoReq = pVehicleData->vehicle_data.LAS_IACCTakeoverReq;
    if(iacctoReq == 0x1 || iacctoReq == 0x2 || 
        iacctoReq == 0x3 || iacctoReq == 0x4 || iacctoReq == 0x6)
        vehicle_data.IaccTakeOverRecstatus = 1;

    vehicle_data.AccTakeOverRecstatus   = (pVehicleData->vehicle_data.ACC_TakeOverReq == 0x1) ? 1 : 0;
    vehicle_data.AEbCtrlAvailRecstatus  = (pVehicleData->vehicle_data.ACC_AEBDecCtrlAvail == 0x1) ? 1 : 0;
  	

    if(app->IaccRecState == EVENT_REC_STATE_RUNNING)
    {
        DVR_EVENT_RECORD_SETTING EventRecordSetting;
        memset(&EventRecordSetting, 0, sizeof(DVR_EVENT_RECORD_SETTING));
        Dvr_Sdk_Recorder_Get(DVR_RECORDER_PROP_EMERGENCY_SETTING, &EventRecordSetting, sizeof(DVR_EVENT_RECORD_SETTING), NULL);
        int iacctoReq = pVehicleData->vehicle_data.LAS_IACCTakeoverReq;
        if(iacctoReq == 0x1 || iacctoReq == 0x2 || 
            iacctoReq == 0x3 || iacctoReq == 0x4 || iacctoReq == 0x6)
            vehicle_data.IaccTakeOverRecstatus = 1;

        if(pVehicleData->vehicle_data.APA_LSCAction == 0x1)
            vehicle_data.ApaRecstatus = 1;

        if(pVehicleData->vehicle_data.ACC_TakeOverReq == 0x1)
            vehicle_data.AccTakeOverRecstatus = 1;
        
        if(pVehicleData->vehicle_data.ACC_AEBDecCtrlAvail == 0x1)
            vehicle_data.AEbCtrlAvailRecstatus = 1;

        if(EventRecordSetting.eType == DVR_EVENTREC_SOURCE_TYPE_IACC_IAC)
        {
            vehicle_data.IaccTakeOverRecstatus = 1;
        }
        else if(EventRecordSetting.eType == DVR_EVENTREC_SOURCE_TYPE_IACC_ACC)
        {
            vehicle_data.AccTakeOverRecstatus = 1;
        }
        else if(EventRecordSetting.eType == DVR_EVENTREC_SOURCE_TYPE_IACC_AEB)
        {
            vehicle_data.AEbCtrlAvailRecstatus = 1;
        }
    }

	app->Gui(GUI_REC_APP_CMD_VEHICLE_DATA_UPDATE, (DVR_U32)&vehicle_data, sizeof(GUI_OBJ_VEHICLE_DATA_INST));
	app->Gui(GUI_REC_APP_CMD_FLUSH, 0, 0);
}

static DVR_BOOL rec_app_feature_record_is_busy(void)
{
	DvrRecApp *app = DvrRecApp::Get();
	DVR_BOOL isBusy = 
		(app->AlarmRecState == EVENT_REC_STATE_RUNNING ||
		app->EventRecState == EVENT_REC_STATE_RUNNING || 
		app->DasRecState == EVENT_REC_STATE_RUNNING ||
		app->IaccRecState == EVENT_REC_STATE_RUNNING);

	return isBusy;
}

static DVR_RESULT rec_app_rec_force_KeyFrame(void)
{
	DvrRecApp *app = DvrRecApp::Get();
	DVR_U32 ForcKeyeFrame = 0x1; 
	DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_FORCE_IDR_REQUEST, (void *)&ForcKeyeFrame, NULL);
}

static DVR_RESULT rec_app_change_display(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrRecApp *app = DvrRecApp::Get();

#if __linux__
	DVR_GUI_LAYOUT_INST_EXT RenderData;
	DvrGuiResHandler *handler;
	
	handler = (DvrGuiResHandler *)Dvr_App_GetGuiResHandler();
	if (handler != NULL)
	{
		RenderData.curLayout = GUI_LAYOUT_RECORD_EXT;
		handler->GetLayoutInfo(GUI_LAYOUT_RECORD, (DVR_GRAPHIC_UIOBJ **)&RenderData.pTable, &RenderData.ObjNum);			
		UpdateRenderDvrData((DVR_GUI_LAYOUT_INST_EXT*)&RenderData, sizeof(DVR_GUI_LAYOUT_INST_EXT));
	}	
#endif
	
	DVR_U32 AVM_DVRModeFeedback = 0x1; //0:Inactive, 0x1:RealTimeMode, 0x2:ReplayMode, 0x3:SettingMode
	DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_MODE_UPDATE, (void *)&AVM_DVRModeFeedback, NULL);

	DVR_U32 DVR_DisplayVison = 0x1; //0:NoRequest, 0x1:Front, 0x2:Rear, 0x3:Left, 0x4:Right, 0x5:MATTS
	DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_DISPLAY_VISION_UPDATE, (void *)&DVR_DisplayVison, NULL);
	
	DVR_U32 ReplayMode = 0;
	DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_REPLAY_MODE, (void *)&ReplayMode, NULL);

	app->curViewIndx = GUI_VIEW_INDEX_FRONT;
	app->Gui(GUI_REC_APP_CMD_SET_LAYOUT, GUI_LAYOUT_RECORD, 0);
	app->Gui(GUI_REC_APP_CMD_VIEW_UPDATE, app->curViewIndx, 4);
	app->Gui(GUI_REC_APP_CMD_MAIN_MENU_TAB_UPDATE, GUI_MAIN_MENU_TAB_RECORD, 4);
	app->Gui(GUI_REC_APP_CMD_REC_STATE_HIDE, 0, 0);
	app->Gui(GUI_REC_APP_CMD_FLUSH, 0, 0);

	DVR_U32 setting;
	DVR_U32 DVR_RecordingCycleSetStatus = 1; //1:1min, 2: 3min 3:5min
	DvrAppUtil_GetRecorderSplitTime((int *)&setting);
	switch (setting)
	{
	case GUI_SETUP_SPLIT_TIME_1MIN:
        DVR_RecordingCycleSetStatus = 1;
		break;
	case GUI_SETUP_SPLIT_TIME_3MIN:
	default:
        DVR_RecordingCycleSetStatus = 2;
		break;
	case GUI_SETUP_SPLIT_TIME_5MIN:
        DVR_RecordingCycleSetStatus = 3;
		break;
	}

    DvrAppNotify *pHandle = DvrAppNotify::Get();
    pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_RECORDING_CYCLE_UPDATE, (void *)&DVR_RecordingCycleSetStatus, NULL);
	return res;
}
static DVR_RESULT rec_app_set_loop_monitor(DVR_BOOL enable)
{
    Dvr_Sdk_MonitorStorage_Set(DVR_MONITOR_STORAGE_PROP_ENABLE_NORMAL_DETECT, &enable, sizeof(DVR_BOOL));   
    Dvr_Sdk_MonitorStorage_Set(DVR_MONITOR_STORAGE_PROP_ENABLE_NORMAL_MSG, &enable, sizeof(DVR_BOOL));
	return DVR_RES_SOK;
}

static DVR_RESULT rec_app_set_das_monitor(DVR_BOOL enable, int type)
{
	DvrRecApp *app = DvrRecApp::Get();
    DVR_DAS_MONITOR monitor;
    monitor.bEnable = enable;
    monitor.eType   = (DVR_DAS_TYPE)type;
    Dvr_Sdk_MonitorStorage_Set(DVR_MONITOR_STORAGE_PROP_ENABLE_DAS_DETECT, &enable, sizeof(DVR_BOOL));
    Dvr_Sdk_MonitorStorage_Set(DVR_MONITOR_STORAGE_PROP_ENABLE_DAS_MSG, &monitor, sizeof(DVR_DAS_MONITOR));
    if(!enable)
        app->DasRecType = DVR_DAS_TYPE_NONE;
	return DVR_RES_SOK;
}

static DVR_RESULT rec_app_start(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrRecApp *app = DvrRecApp::Get();

	rec_app_change_display();

	if (APP_CHECKFLAGS(app->m_recorder.GFlags, APP_AFLAGS_READY)){
		//TODO
	}
	else{
		APP_ADDFLAGS(app->m_recorder.GFlags, APP_AFLAGS_READY);
	}

	Dvr_Sdk_Recorder_RegisterVehDataUpdateCallBack(DvrRecorderOnNewVehicleData, NULL);

	if(app->RecCapState == REC_CAP_STATE_RECORD)
	{
		app->Func(REC_APP_FUNC_CARD_CHECK_STATUS, 0, 0);
		Log_Message("Already in Record State, Return!!!!!!");
		return DVR_RES_SOK;
	}

	if(app->bEncodingHasStarted == FALSE)
	{
		res = Dvr_Sdk_Recorder_Reset();
		if (res == DVR_RES_SOK)
		{	
			DVR_EVENT_RECORD_SETTING EventRecordSetting;
			EventRecordSetting.EventPreRecordLimitTime = 15*1000;
			EventRecordSetting.EventRecordLimitTime = 30*1000;
			EventRecordSetting.EventRecordDelayTime = 0;
			Dvr_Sdk_Recorder_Set(DVR_RECORDER_PROP_EMERGENCY_SETTING, &EventRecordSetting, sizeof(DVR_EVENT_RECORD_SETTING));

			Dvr_Sdk_Recorder_Start();
			app->bEncodingHasStarted = TRUE;
		}
		else
		{
			Log_Error("Dvr_Sdk_Recorder_Init Failed!!!!");
		}

		DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_RECORDER_START_DONE, NULL, NULL);		
	}

	res = app->Func(REC_APP_FUNC_CARD_CHECK_STATUS, 0, 0); 
	if ( !APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NO_CARD) &&
		!APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_ERROR_FORMAT) &&
		!APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NOT_ENOUGH_SPACE))	
    {
		DVR_S32 status;
		DVR_DISK_DIRECTORY Directory;
		memset(&Directory, 0, sizeof(DVR_DISK_DIRECTORY));
		if (DVR_RES_SOK == Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_DIRECTORY_STRUCTURE, &Directory, sizeof(DVR_DISK_DIRECTORY), &status))
		{
			Dvr_Sdk_Recorder_Set(DVR_RECORDER_PROP_ROOT_DIRECTORY, &Directory, sizeof(DVR_DISK_DIRECTORY));
		}	
	
        if( (TRUE == app->bUserEnableRec) && (TRUE == app->bPowerMode))
		{
			res = app->Func(REC_APP_FUNC_LOOP_RECORD_SWITCH, TRUE, 0);
		}
	}

	return res;
}

DVR_RESULT rec_app_start_async(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrRecApp *app = DvrRecApp::Get();

	if(app->bEncodingHasStarted == FALSE)
	{
		res = Dvr_Sdk_Recorder_Init();
		if (res == DVR_RES_SOK)
		{		
			DVR_EVENT_RECORD_SETTING EventRecordSetting;
			EventRecordSetting.EventPreRecordLimitTime = 15000;
			EventRecordSetting.EventRecordLimitTime = 30000;
			EventRecordSetting.EventRecordDelayTime = 0;
			EventRecordSetting.eType = DVR_EVENTREC_SOURCE_TYPE_SWITCH;
			Dvr_Sdk_Recorder_Set(DVR_RECORDER_PROP_EMERGENCY_SETTING, &EventRecordSetting, sizeof(DVR_EVENT_RECORD_SETTING));

			Dvr_Sdk_Recorder_Start();
			app->bEncodingHasStarted = TRUE;
		}
		else
		{
			Log_Error("Dvr_Sdk_Recorder_Init Failed!!!!");
		}
	}

	return res;
}

DVR_RESULT rec_app_stop_async(void)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrRecApp *app = DvrRecApp::Get();

	if(app->Func != NULL)
	{
    	app->Func(REC_APP_FUNC_LOOP_RECORD_STOP, 0, 0);

		DVR_U32 DVREnableSetStatus = 1;//0x0=invalid,0x1=off,0x2=on,0x3=especially
		DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_RECORDER_SWTICH, (void *)&DVREnableSetStatus, NULL);		

		if(app->DasRecState == EVENT_REC_STATE_RUNNING)
		{
            rec_app_set_das_monitor(FALSE, app->DasRecType);

            rec_app_set_loop_monitor(FALSE);

	        Dvr_Sdk_Recorder_DasRec_Stop();
	        app->DasRecState = EVENT_REC_STATE_IDLE;
		}

		if (app->EventRecState == EVENT_REC_STATE_RUNNING)
		{
		    Dvr_Sdk_Recorder_EventRec_Stop();
		    app->EventRecState = EVENT_REC_STATE_IDLE;
		}

		if (app->AlarmRecState == EVENT_REC_STATE_RUNNING)
		{
		    Dvr_Sdk_Recorder_EventRec_Stop();
		    app->AlarmRecState = EVENT_REC_STATE_IDLE;
		}

		if(app->IaccRecState == EVENT_REC_STATE_RUNNING)
		{
		    Dvr_Sdk_Recorder_IaccRec_Stop(0);
		    app->IaccRecState = EVENT_REC_STATE_IDLE;
		}
	}

    Dvr_Sdk_Recorder_Stop();

#if 0 //disable to speed up the playback loading
    Dvr_Sdk_Recorder_DeInit();
#endif

	app->bEncodingHasStarted = FALSE;

    return res;
}

static DVR_RESULT rec_app_stop(void)
{
    return DVR_RES_SOK;
}

static void rec_app_switch_looprec_async(void *arg)
{
    DvrRecApp *app = DvrRecApp::Get();
	if(app != NULL)
        app->Func(REC_APP_FUNC_LOOP_RECORD_SWITCH, TRUE, 0); //turn on loop recording
	return;
}

static void rec_app_stop_looprec_async(void *arg)
{
	Dvr_Sdk_Recorder_LoopRec_Stop();
	
	return;
}

static void rec_app_das_record_stop_asyc(void *arg)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrRecApp *app = DvrRecApp::Get();

    if (app->DasRecState != EVENT_REC_STATE_RUNNING)
    {
        app->bIaccApaIsInterrupted = FALSE;
        app->bDasStopCommandHasFired = FALSE;
        app->bDasStopDelayProcessIsInterrupted = FALSE;
		Log_Message("DAS Recording is not running!!!!!");
        return;
    }

    int waittimecnt = 5000/20;
    if(arg != NULL)
        waittimecnt = (*(int *)arg)/20;
    //wait 5s
    for(int i = 0;i < waittimecnt;i++)
    {
        if(app->bDasStopDelayProcessIsInterrupted)
            break;
#ifdef __linux__
        usleep(20 * 1000);
#else
        Sleep(20);
#endif
    }

    if (app->DasRecState == EVENT_REC_STATE_RUNNING && 
		FALSE == app->bDasStopDelayProcessIsInterrupted)
    {
        rec_app_set_das_monitor(FALSE, app->DasRecType);
        
        rec_app_set_loop_monitor(FALSE);

        Dvr_Sdk_Recorder_DasRec_Stop();
		
        app->DasRecState = EVENT_REC_STATE_IDLE;
		
		if( (TRUE == app->bUserEnableRec) && (TRUE == app->bPowerMode) ) 
		{
			res = app->Func(REC_APP_FUNC_LOOP_RECORD_SWITCH, TRUE, 0); //turn on loop recording
		}

		CThreadPool::Get()->AddTask(system_sync, NULL);
    }

	app->bDasStopCommandHasFired = FALSE;
	app->bDasStopDelayProcessIsInterrupted = FALSE;

    if(arg && app->bIaccApaIsInterrupted)
        app->bIaccApaIsInterrupted = FALSE;

    return;
}

static DVR_BOOL rec_app_iaccapa_record_Interrupt(void)
{
	DvrRecApp *app = DvrRecApp::Get();

    if(app->IaccRecState == EVENT_REC_STATE_RUNNING)
    {
	    DVR_EVENT_RECORD_SETTING EventRecordSetting;
	    Dvr_Sdk_Recorder_Get(DVR_RECORDER_PROP_EMERGENCY_SETTING, &EventRecordSetting, sizeof(DVR_EVENT_RECORD_SETTING), NULL);
	    if(app->bIaccApaIsInterrupted == TRUE || EventRecordSetting.eType == DVR_EVENTREC_SOURCE_TYPE_IACC_AEB)
            return DVR_RES_EFAIL;

        app->bIaccApaIsInterrupted = TRUE;
		Log_Message("[%s:%d]IACC recording task is already running, delay 15s to stop!!!", __FUNCTION__, __LINE__);
        EventRecordSetting.EventRecordDelayTime = 15;
	    Dvr_Sdk_Recorder_Set(DVR_RECORDER_PROP_EMERGENCY_SETTING, &EventRecordSetting, sizeof(DVR_EVENT_RECORD_SETTING));
        DVR_BOOL bMove = TRUE;
        Dvr_Sdk_Recorder_Set(DVR_RECORDER_PROP_DAS_MOVE, &bMove, sizeof(DVR_BOOL));
        return DVR_RES_EFAIL;
    }

    if(app->DasRecState == EVENT_REC_STATE_RUNNING) 
	{
	    if(app->bIaccApaIsInterrupted == TRUE)
            return DVR_RES_EFAIL;
        app->bIaccApaIsInterrupted = TRUE;
		Log_Message("[%s:%d]APA recording task is already running,  delay 15s to stop!!!", __FUNCTION__, __LINE__);
        static int delaytime = 15000;
        //Trigger emergency recording within 5 seconds after APA stops,first end last stop
        if(app->bDasStopCommandHasFired)
        {
            app->bDasStopDelayProcessIsInterrupted = TRUE;
            usleep(25 * 1000);
        }
        if(!app->bDasStopCommandHasFired)
            app->bDasStopCommandHasFired = TRUE;
        CThreadPool::Get()->AddTask(rec_app_das_record_stop_asyc, &delaytime);
        DVR_BOOL bMove = TRUE;
        Dvr_Sdk_Recorder_Set(DVR_RECORDER_PROP_DAS_MOVE, &bMove, sizeof(DVR_BOOL));
        return DVR_RES_EFAIL;
	}

    return DVR_RES_SOK;
}

static void shoot_result_feedback_success(void *arg)
{
	DVR_U32 DVR_PhotographResult = 1;//0x0=invalid,0x1=success,0x2=fail,0x3=reserved
    DvrAppNotify *pHandle = DvrAppNotify::Get();
    DvrRecApp *app = (DvrRecApp *)arg;	
    if (pHandle != NULL)
        pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_PHOTOGRAPH_RESULT, (void *)&DVR_PhotographResult, NULL);

#ifdef __linux__
		usleep(500*1000); //wait 500ms
#endif

	DVR_PhotographResult = 0;
	if (pHandle != NULL)
		pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_PHOTOGRAPH_RESULT, (void *)&DVR_PhotographResult, NULL);

#ifdef __linux__
	sync();
#endif

	if(app != NULL && app->RecCapState != REC_CAP_STATE_RECORD &&
	   app->AlarmRecState != EVENT_REC_STATE_RUNNING &&
	   app->EventRecState != EVENT_REC_STATE_RUNNING && 
	   app->IaccRecState != EVENT_REC_STATE_RUNNING)
    {
        rec_app_set_loop_monitor(FALSE);
    }   
}

static void shoot_result_feedback_failed(void *arg)
{
	DVR_U32 DVR_PhotographResult = 2;//0x0=invalid,0x1=success,0x2=fail,0x3=reserved
    DvrAppNotify *pHandle = DvrAppNotify::Get();
		
    if (pHandle != NULL)
        pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_PHOTOGRAPH_RESULT, (void *)&DVR_PhotographResult, NULL);

#ifdef __linux__
		usleep(500*1000); //wait 500ms
#endif

	DVR_PhotographResult = 0;
	if (pHandle != NULL)
		pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_PHOTOGRAPH_RESULT, (void *)&DVR_PhotographResult, NULL);
}
static DVR_RESULT rec_app_snaphot_set(int EnSanphot)
{
	DVR_RESULT res = DVR_RES_SOK;
    DvrRecApp *app = DvrRecApp::Get();	

	app->m_bSnapShotEnable=EnSanphot;
	Log_Warning("m_bSnapShotEnable %d\n",app->m_bSnapShotEnable);
	
	return res;

}

static DVR_RESULT rec_app_snapshot(int matts,int shot_force_flag)
{
	DVR_RESULT res = DVR_RES_SOK;
    DvrRecApp *app = DvrRecApp::Get();	

	if((app->m_bSnapShotEnable==FALSE) && (shot_force_flag==0) ){
		Log_Error("m_bSnapShotEnable %d shot_force_flag %d\n",app->m_bSnapShotEnable,shot_force_flag);
		return res;
	}
		
	res = app->Func(REC_APP_FUNC_CARD_CHECK_STATUS, 0, 0);
	if(APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NO_CARD) ||
		APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_ERROR_FORMAT) ||
		APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_PHOTO_FOLDER_NOT_ENOUGH_SPACE) ||
		APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_LOW_SPEED) ||
		APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NOT_READY) ||
		APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NOT_ENOUGH_SPACE))
	{
		CThreadPool::Get()->AddTask(shoot_result_feedback_failed, NULL);	
		return res;
	}

    rec_app_set_loop_monitor(TRUE);
    DVR_PHOTO_PARAM param;
    param.eType = DVR_PHOTO_TYPE_JPG;
    param.eQuality = DVR_PHOTO_QUALITY_SFINE;

    int viewIndx = matts ? GUI_VIEW_INDEX_MATTS : app->curViewIndx;
    switch (viewIndx)
    {
	    case GUI_VIEW_INDEX_FRONT:
	    {
			param.eIndex = DVR_VIEW_INDEX_FRONT;
			res = Dvr_Sdk_Recorder_TakePhoto(&param);			
	    }
	    break;

	    case GUI_VIEW_INDEX_REAR:
	    {
	        param.eIndex = DVR_VIEW_INDEX_REAR;
			res = Dvr_Sdk_Recorder_TakePhoto(&param);			
	    }	
	    break;

	    case GUI_VIEW_INDEX_LEFT:
	    {
	        param.eIndex = DVR_VIEW_INDEX_LEFT;
			res = Dvr_Sdk_Recorder_TakePhoto(&param);
	    }
	    break;

	    case GUI_VIEW_INDEX_RIGHT:
	    {
	        param.eIndex = DVR_VIEW_INDEX_RIGHT;
			res = Dvr_Sdk_Recorder_TakePhoto(&param);
	    }
	    break;

		case GUI_VIEW_INDEX_MATTS:
		{
			param.eIndex = DVR_VIEW_INDEX_FRONT;
			res = Dvr_Sdk_Recorder_TakePhoto(&param);

#if 1
	        param.eIndex = DVR_VIEW_INDEX_REAR;
			Dvr_Sdk_Recorder_TakePhoto(&param);

	        param.eIndex = DVR_VIEW_INDEX_LEFT;
			Dvr_Sdk_Recorder_TakePhoto(&param);

	        param.eIndex = DVR_VIEW_INDEX_RIGHT;
			Dvr_Sdk_Recorder_TakePhoto(&param);
#endif
		}
		break;

	    default:
	    {
	        Log_Error("Not Support take photo in view [%d]", viewIndx);
	        return DVR_RES_EFAIL;
	    }
    }

    if(res != DVR_RES_SOK)
	{
	    printf("take photo failed         0x%x\n",res);
		CThreadPool::Get()->AddTask(shoot_result_feedback_failed, NULL);	
		return res;
	}

	//usleep(30*1000);
	CThreadPool::Get()->AddTask(shoot_result_feedback_success, app);
	return res;
}

static void rec_app_timer_handler(DVR_U32 eid)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrRecApp *app = DvrRecApp::Get();
    static int bStatus = 0;

    if (app->RecCapState == REC_CAP_STATE_RECORD)
    {
        bStatus ^= 1;
        if (bStatus == 1)
        {
            app->Gui(GUI_REC_APP_CMD_REC_STATE_SHOW, 0, 0);
        }
        else
        {
            app->Gui(GUI_REC_APP_CMD_REC_STATE_HIDE, 0, 0);
        }

		app->Gui(GUI_REC_APP_CMD_REC_STATE_UPDATE, GUI_REC_STATE_START, 4);
        app->Gui(GUI_REC_APP_CMD_FLUSH, 0, 0);
    }
}

static DVR_RESULT rec_app_loop_record_switch(int bEnable, int param2)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrRecApp *app = DvrRecApp::Get();

	if(bEnable)
	{
		if(FALSE == rec_app_feature_record_is_busy())
		{
			if( app->RecCapState == REC_CAP_STATE_PREVIEW )
			{
				/* Check the card's status. */
				res = app->Func(REC_APP_FUNC_CARD_CHECK_STATUS, 0, 0);
				if (!APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NO_CARD) &&
					!APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_ERROR_FORMAT) &&
					!APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_LOW_SPEED) &&
                    !APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NOT_READY) &&
					!APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NOT_ENOUGH_SPACE))
				{
					/* To record the clip if the card is ready. */
					if(DVR_RES_SOK == app->Func(REC_APP_FUNC_LOOP_RECORD_START, 0, 0))
					{
						DVR_U32 DVREnableSetStatus = 2;//0x0=invalid,0x1=off,0x2=on,0x3=especially
						DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_RECORDER_SWTICH, (void *)&DVREnableSetStatus, NULL);
					}
				}
			}
		}
		else
		{
			Log_Message("another recording task is already running, can not start loop record");
		}
	}
	else
	{
		if(app->RecCapState == REC_CAP_STATE_RECORD)
		{
			DVR_U32 DVREnableSetStatus = 1;//0x0=invalid,0x1=off,0x2=on,0x3=especially
			DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_RECORDER_SWTICH, (void *)&DVREnableSetStatus, NULL);
		
			/* Stop recording. */
			app->Func(REC_APP_FUNC_LOOP_RECORD_STOP, param2, 0);
		}
	}

	return res;
}

static DVR_RESULT rec_app_loop_record_start(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrRecApp *app = DvrRecApp::Get();

    rec_app_set_loop_monitor(TRUE);
	DVR_RECORDER_PERIOD_OPTION option;
	DVR_U32 setting;
	DvrAppUtil_GetRecorderSplitTime((int *)&setting);
	switch (setting)
	{
	case GUI_SETUP_SPLIT_TIME_1MIN:
		option.nSplitTime = DVR_VIDEO_SPLIT_TIME_60_SECONDS;
		break;
	case GUI_SETUP_SPLIT_TIME_3MIN:
	default:
		option.nSplitTime = DVR_VIDEO_SPLIT_TIME_180_SECONDS;
		break;
	case GUI_SETUP_SPLIT_TIME_5MIN:
		option.nSplitTime = DVR_VIDEO_SPLIT_TIME_300_SECONDS;
		break;
	}
	option.bImmediatelyEffect = TRUE;
	res |= Dvr_Sdk_Recorder_Set(DVR_RECORDER_PROP_VIDEO_SPLIT_TIME, &option, sizeof(DVR_RECORDER_PERIOD_OPTION));

	res |= Dvr_Sdk_Recorder_LoopRec_Start();
	if(res == DVR_RES_SOK)
	{
	    //rec_app_rec_force_KeyFrame();
		Dvr_Sdk_ComSvrTimer_Register(TIMER_2HZ, rec_app_timer_handler);
		
		app->RecCapState = REC_CAP_STATE_RECORD;
		APP_ADDFLAGS(app->m_recorder.GFlags, APP_AFLAGS_BUSY);
	}

	/*Update the gui*/
	app->Gui(GUI_REC_APP_CMD_REC_SWITCH_SHOW, 0, 0);
	app->Gui(GUI_REC_APP_CMD_REC_SWITCH_UPDATE, GUI_SWITCH_STATE_ON, 4);
	app->Gui(GUI_REC_APP_CMD_REC_STATE_SHOW, 0, 0);
	app->Gui(GUI_REC_APP_CMD_REC_STATE_UPDATE, GUI_REC_STATE_START, 4);
	app->Gui(GUI_REC_APP_CMD_FLUSH, 0, 0);

	return res;
}

static DVR_RESULT rec_app_loop_record_stop(int param)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrRecApp *app = DvrRecApp::Get();

    if (app->RecCapState == REC_CAP_STATE_RECORD)
    {
    	app->RecCapState = REC_CAP_STATE_PREVIEW;
        if (APP_CHECKFLAGS(app->m_recorder.GFlags, APP_AFLAGS_POPUP)) {
            AppWidget_Off(DVR_WIDGET_ALL, 0);
        }

        rec_app_set_loop_monitor(FALSE);

	    APP_REMOVEFLAGS(app->m_recorder.GFlags, APP_AFLAGS_BUSY);

        Dvr_Sdk_ComSvrTimer_UnRegister(TIMER_2HZ, rec_app_timer_handler);

		if(param == 2)
		{
			CThreadPool::Get()->AddTask(rec_app_stop_looprec_async, NULL);
		}
		else
		{
			res = Dvr_Sdk_Recorder_LoopRec_Stop();
		}

	    /*Update the gui*/
	    app->Gui(GUI_REC_APP_CMD_REC_SWITCH_SHOW, 0, 0);
	    app->Gui(GUI_REC_APP_CMD_REC_SWITCH_UPDATE, GUI_SWITCH_STATE_OFF, 4);
	    app->Gui(GUI_REC_APP_CMD_REC_STATE_HIDE, 0, 0);
	    app->Gui(GUI_REC_APP_CMD_REC_STATE_UPDATE, GUI_REC_STATE_STOP, 4);
	    app->Gui(GUI_REC_APP_CMD_FLUSH, 0, 0);

		CThreadPool::Get()->AddTask(system_sync, NULL);
    }

    return res;
}

static void event_record_result_feedback_success(void *arg)
{
	DVR_U32 EmergencyRecordStatus = 2;//0x0=invalid,0x1=Unsuccessful,0x2=Success,0x3=Saving
	DVR_U32 EmergencyRecordSwitch = 0;  //0x0= off    0x01=1  on
    DvrAppNotify *pHandle = DvrAppNotify::Get();
		
    DvrRecApp *app = (DvrRecApp *)arg;
    if (pHandle != NULL)
    {
		pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_EVENT_RECORD_STATUS, (void *)&EmergencyRecordStatus, NULL);
	    pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_EVENT_RECORD_SWITCH, (void *)&EmergencyRecordSwitch, NULL);
    }    
#ifdef __linux__
    if(app != NULL && app->bEventcontinue == TRUE)
	    usleep(200*1000); //wait 200ms
    else
	    usleep(500*1000); //wait 500ms
#endif

	EmergencyRecordStatus = 0;
	if (pHandle != NULL)
		pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_EVENT_RECORD_STATUS, (void *)&EmergencyRecordStatus, NULL);
    if(app != NULL)
    {
        if(app->bIacccontinue == TRUE)
        {
            usleep(500*1000); //wait 500ms
            app->bIaccHasFinshed = TRUE;
            app->bIacccontinue = FALSE;
        }
        else if(app->bEventcontinue == TRUE)
        {
            usleep(200*1000); //wait 200ms
            app->bEventcontinue = FALSE;
        }
    }

#ifdef __linux__
	sync();
#endif
}

static void event_record_result_feedback_failed(void *arg)
{
	DVR_U32 EmergencyRecordStatus = 1;//0x0=invalid,0x1=Unsuccessful,0x2=Success,0x3=Saving
	DVR_U32 EmergencyRecordSwitch = 0;  //0x0= off    0x01=1  on
    DvrAppNotify *pHandle = DvrAppNotify::Get();

	
    if (pHandle != NULL)
    {
        pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_EVENT_RECORD_STATUS, (void *)&EmergencyRecordStatus, NULL);
	    pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_EVENT_RECORD_SWITCH, (void *)&EmergencyRecordSwitch, NULL);
    }      
#ifdef __linux__
	usleep(500*1000); //wait 500ms
#endif

	EmergencyRecordStatus = 0;
	if (pHandle != NULL)
		pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_EVENT_RECORD_STATUS, (void *)&EmergencyRecordStatus, NULL);
    DvrRecApp *app = (DvrRecApp *)arg;

    if(app != NULL)
    {
        if(app->bIacccontinue == TRUE)
        {
            usleep(500*1000); //wait 500ms
            app->bIaccHasFinshed = TRUE;
            app->bIacccontinue = FALSE;
        }
        else if(app->bEventcontinue == TRUE)
        {
            usleep(200*1000); //wait 200ms
            app->bEventcontinue = FALSE;
        }
    }
}

static void rec_app_event_record_async(void *arg)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrRecApp *app = DvrRecApp::Get();
    DVR_U32 EmergencyRecordStatus;//0x0=invalid,0x1=Unsuccessful,0x2=Success,0x3=Saving
	DVR_U32 EmergencyRecordSwitch = 1;  //0x0= off    0x01=1  on

    int type = 0;
    if(arg != NULL)
        type = *(int *)arg;

    uint count = 0;
    while(app->bEventcontinue)
    {
        if(count >= 40)
            break;
        Log_Message("waiting event over count %d!!!!!!!!!!!!!!!!!!!!\n",count);
        usleep(50*1000);
        count++;
    }
    app->bEventcontinue = TRUE;
    rec_app_set_loop_monitor(TRUE);
    EmergencyRecordStatus = 3;//0x0=invalid,0x1=Unsuccessful,0x2=Success,0x3=Saving
    DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_EVENT_RECORD_STATUS, (void *)&EmergencyRecordStatus, NULL);
    DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_EVENT_RECORD_SWITCH, (void *)&EmergencyRecordSwitch, NULL);
    DVR_S32 status;
    DVR_DISK_DIRECTORY Directory;
    memset(&Directory, 0, sizeof(DVR_DISK_DIRECTORY));
    
    DVR_EVENT_RECORD_SETTING EventRecordSetting;
    EventRecordSetting.EventPreRecordLimitTime = 15*1000;
    EventRecordSetting.EventRecordLimitTime = 30*1000;
    EventRecordSetting.eType = (DVR_EVENTREC_SOURCE_TYPE)type;
    EventRecordSetting.EventRecordDelayTime = 0;
    if (DVR_RES_SOK == Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_DIRECTORY_STRUCTURE, &Directory, sizeof(DVR_DISK_DIRECTORY), &status))
        strcpy(EventRecordSetting.szEventDir, Directory.szEventRecDir);

    Dvr_Sdk_Recorder_Set(DVR_RECORDER_PROP_EMERGENCY_SETTING, &EventRecordSetting, sizeof(DVR_EVENT_RECORD_SETTING));

    app->Func(REC_APP_FUNC_LOOP_RECORD_SWITCH, FALSE, 2); //turn off loop recording

    Dvr_Sdk_Recorder_EventRec_Start();

    app->EventRecState = EVENT_REC_STATE_RUNNING;
    
    app->Gui(GUI_REC_APP_CMD_EMERGENCY_STATE_SHOW, 0, 0);
    app->Gui(GUI_REC_APP_CMD_FLUSH, 0, 0);
    app->beventRecHasTrigger = FALSE;
}

static DVR_RESULT rec_app_event_record_start(int type)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrRecApp *app = DvrRecApp::Get();
    if(app->bEncodingHasStarted == FALSE)
    {
        Log_Message("IVA encoding process has not started, can not trigger event recording!!!");
        CThreadPool::Get()->AddTask(event_record_result_feedback_failed, app); 
        return DVR_RES_SOK;
    }

    if(app->AlarmRecState == EVENT_REC_STATE_RUNNING ||
    app->EventRecState == EVENT_REC_STATE_RUNNING) 
    {
        Log_Message("[%s:%d]another recording task is already running", __FUNCTION__, __LINE__);
        return DVR_RES_SOK;
    }

    if(rec_app_iaccapa_record_Interrupt() != DVR_RES_SOK)
        return DVR_RES_SOK;

    res = app->Func(REC_APP_FUNC_CARD_CHECK_STATUS, 0, 0);
    if(APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NO_CARD) ||
        APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_ERROR_FORMAT) ||
        APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_EVENT_FOLDER_NOT_ENOUGH_SPACE) ||
        APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_LOW_SPEED) ||
        APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NOT_READY) ||
        APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NOT_ENOUGH_SPACE))
    {
        CThreadPool::Get()->AddTask(event_record_result_feedback_failed, app); 
        return res;
    }

    if(app->EventRecState != EVENT_REC_STATE_IDLE)
        return DVR_RES_SOK;

    if(app->beventRecHasTrigger == TRUE)
    {
        Log_Message("Event Rec have Trigger %d!!!\n",type);
    }
    else
    {
        static int rectype = type;
        app->beventRecHasTrigger = TRUE;
        CThreadPool::Get()->AddTask(rec_app_event_record_async, &rectype);
    }
    return res;
}

static DVR_RESULT rec_app_event_record_finish(int bIsFailed)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrRecApp *app = DvrRecApp::Get();

	
    if (app->EventRecState == EVENT_REC_STATE_RUNNING)
    {
        Dvr_Sdk_Recorder_EventRec_Stop();
		
	    app->EventRecState = EVENT_REC_STATE_IDLE;
        app->beventRecHasTrigger = FALSE;

        rec_app_set_loop_monitor(FALSE);
        
		if( (TRUE == app->bUserEnableRec) && (TRUE == app->bPowerMode) )
		{
            CThreadPool::Get()->AddTask(rec_app_switch_looprec_async, NULL);
		}

		app->Gui(GUI_REC_APP_CMD_EMERGENCY_STATE_HIDE, 0, 0);
		app->Gui(GUI_REC_APP_CMD_FLUSH, 0, 0);

		if(bIsFailed)
		{
			CThreadPool::Get()->AddTask(event_record_result_feedback_failed, app);	
		}
		else
		{
			CThreadPool::Get()->AddTask(event_record_result_feedback_success, app);	
		}	
    }

	return res;
}

static DVR_RESULT rec_app_alarm_record_start(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrRecApp *app = DvrRecApp::Get();
	DVR_U32 EmergencyRecordStatus;//0x0=invalid,0x1=Unsuccessful,0x2=Success,0x3=Saving

	if(app->bEncodingHasStarted == FALSE)
	{
		Log_Message("IVA encoding process has not started, can not trigger event recording!!!");
		CThreadPool::Get()->AddTask(event_record_result_feedback_failed, NULL); 
		return DVR_RES_SOK;
	}

	if( TRUE == rec_app_feature_record_is_busy())
	{
		Log_Message("[%s:%d]another recording task is already running", __FUNCTION__, __LINE__);
		return DVR_RES_SOK;
	}	

    if(app->AlarmRecLastLimitTime == ALARM_REC_LIMIT_TIME_300)
    {
		Log_Message("[%s:%d], AlarmRec has already finished,can not trigger Alarm recording!!!", __FUNCTION__, __LINE__);
		return DVR_RES_SOK;
	}

	res = app->Func(REC_APP_FUNC_CARD_CHECK_STATUS, 0, 0);
	if(APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NO_CARD) ||
		APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_ERROR_FORMAT) ||
		APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_EVENT_FOLDER_NOT_ENOUGH_SPACE) ||
		APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_LOW_SPEED) ||
		APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NOT_READY) ||
		APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NOT_ENOUGH_SPACE))
	{
		CThreadPool::Get()->AddTask(event_record_result_feedback_failed, NULL); 
		return res;
	}

	if(app->AlarmRecState == EVENT_REC_STATE_IDLE)
	{
        rec_app_set_loop_monitor(TRUE);

		DVR_S32 status;
        DVR_DISK_DIRECTORY Directory;
        memset(&Directory, 0, sizeof(DVR_DISK_DIRECTORY));
		DVR_EVENT_RECORD_SETTING EventRecordSetting;
		EventRecordSetting.EventPreRecordLimitTime = 0;
        EventRecordSetting.EventRecordDelayTime = 0;
		EventRecordSetting.eType = DVR_EVENTREC_SOURCE_TYPE_ALARM;

        if(app->bAlarmRecHasTrigger)
        {
            app->AlarmRecLastLimitTime = ALARM_REC_LIMIT_TIME_300;
            EventRecordSetting.EventRecordLimitTime = ALARM_REC_LIMIT_TIME_300;
        }
        else
        {
            app->AlarmRecLastLimitTime = ALARM_REC_LIMIT_TIME_30;
		    EventRecordSetting.EventRecordLimitTime = ALARM_REC_LIMIT_TIME_30;
        }
        if (DVR_RES_SOK == Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_DIRECTORY_STRUCTURE, &Directory, sizeof(DVR_DISK_DIRECTORY), &status))
            strcpy(EventRecordSetting.szEventDir, Directory.szEventRecDir);

		Dvr_Sdk_Recorder_Set(DVR_RECORDER_PROP_EMERGENCY_SETTING, &EventRecordSetting, sizeof(DVR_EVENT_RECORD_SETTING));

		app->Func(REC_APP_FUNC_LOOP_RECORD_SWITCH, FALSE, 0); //turn off loop recording
		
		Dvr_Sdk_Recorder_EventRec_Start();
		
		app->AlarmRecState = EVENT_REC_STATE_RUNNING;
		app->Gui(GUI_REC_APP_CMD_EMERGENCY_STATE_SHOW, 0, 0);
		app->Gui(GUI_REC_APP_CMD_FLUSH, 0, 0);

		EmergencyRecordStatus = 3;//0x0=invalid,0x1=Unsuccessful,0x2=Success,0x3=Saving
		//DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_EVENT_RECORD_STATUS, (void *)&EmergencyRecordStatus, NULL);		
	}

	return res;
}

static DVR_RESULT rec_app_alarm_record_finish(DVR_U32 param1, DVR_U32 param2)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrRecApp *app = DvrRecApp::Get();

    if(param2)
    {
        app->bAlarmRecHasTrigger = FALSE;
        if(app->AlarmRecLastLimitTime == ALARM_REC_LIMIT_TIME_30)
        {
            app->AlarmRecLastLimitTime = 0;
            return DVR_RES_SOK;
        }
        app->AlarmRecLastLimitTime = 0;
    }

    if (app->AlarmRecState == EVENT_REC_STATE_RUNNING)
    {
        Dvr_Sdk_Recorder_EventRec_Stop();

        if(app->AlarmRecLastLimitTime == ALARM_REC_LIMIT_TIME_30)
        {
            if(app->bAlarmRecHasTrigger == FALSE)
                app->bAlarmRecHasTrigger = TRUE;
        }
        else
            app->bAlarmRecHasTrigger = FALSE;

	    app->AlarmRecState = EVENT_REC_STATE_IDLE;

        rec_app_set_loop_monitor(FALSE);
		if( (TRUE == app->bUserEnableRec) && (TRUE == app->bPowerMode) )
		{
			res = app->Func(REC_APP_FUNC_LOOP_RECORD_SWITCH, TRUE, 0); //turn on loop recording
		}

		app->Gui(GUI_REC_APP_CMD_EMERGENCY_STATE_HIDE, 0, 0);
		app->Gui(GUI_REC_APP_CMD_FLUSH, 0, 0);

		CThreadPool::Get()->AddTask(system_sync, NULL);
    }

	return res;
}

static DVR_RESULT rec_app_aeb_record_start(int type)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrRecApp *app = DvrRecApp::Get();
	DVR_U32 EmergencyRecordStatus;//0x0=invalid,0x1=Unsuccessful,0x2=Success,0x3=Saving
	DVR_U32 EmergencyRecordSwitch = 1;  //0x0= off    0x01=1  on

    if(app->bEncodingHasStarted == FALSE)
    {
        Log_Message("IVA encoding process has not started, can not trigger event recording!!!");
        if(app->bIacccontinue == TRUE && app->bIaccFailedHasFinshed == FALSE)
            return DVR_RES_SOK;
        app->bIacccontinue = TRUE;
        app->bIaccFailedHasFinshed = FALSE;
        CThreadPool::Get()->AddTask(event_record_result_feedback_failed, app); 
        return DVR_RES_SOK;
    }

    if(app->AlarmRecState == EVENT_REC_STATE_RUNNING ||
    app->EventRecState == EVENT_REC_STATE_RUNNING) 
	{
		Log_Message("[%s:%d]another recording task is already running", __FUNCTION__, __LINE__);
		return DVR_RES_SOK;
	}

    if(app->IaccRecState == EVENT_REC_STATE_RUNNING)
    {
        app->bIacccontinue = TRUE;
    }

    if(rec_app_iaccapa_record_Interrupt() != DVR_RES_SOK)
		return DVR_RES_SOK;

    if(app->bIacccontinue == TRUE && app->bIaccHasFinshed == FALSE)
    {
        Log_Message("[%s:%d]Iacc recording has not Stoped", __FUNCTION__, __LINE__);
        return DVR_RES_SOK;
    }

    res = app->Func(REC_APP_FUNC_CARD_CHECK_STATUS, 0, 0);
    if(APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NO_CARD) ||
        APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_ERROR_FORMAT) ||
		APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_EVENT_FOLDER_NOT_ENOUGH_SPACE) ||
        APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_LOW_SPEED) ||
		APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NOT_READY) ||
        APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NOT_ENOUGH_SPACE))
    {
        if(app->bIacccontinue == TRUE && app->bIaccFailedHasFinshed == FALSE)
            return DVR_RES_SOK;
        app->bIacccontinue = TRUE;
        app->bIaccFailedHasFinshed = FALSE;
        CThreadPool::Get()->AddTask(event_record_result_feedback_failed, app); 
        return res;
    }

    if(app->IaccRecState == EVENT_REC_STATE_IDLE)
    {
        app->bIacccontinue = FALSE;
        app->bIaccHasFinshed = FALSE;
        app->bIaccFailedHasFinshed = FALSE;
		EmergencyRecordStatus = 3;//0x0=invalid,0x1=Unsuccessful,0x2=Success,0x3=Saving
		DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_EVENT_RECORD_STATUS, (void *)&EmergencyRecordStatus, NULL);
		DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_EVENT_RECORD_SWITCH, (void *)&EmergencyRecordSwitch, NULL);
        DVR_S32 status;
        DVR_DISK_DIRECTORY Directory;
        memset(&Directory, 0, sizeof(DVR_DISK_DIRECTORY));

        DVR_EVENT_RECORD_SETTING EventRecordSetting;
        memset(&EventRecordSetting, 0, sizeof(DVR_EVENT_RECORD_SETTING));
	    EventRecordSetting.EventPreRecordLimitTime = 15*1000;
	    EventRecordSetting.EventRecordLimitTime = 30*1000;
        EventRecordSetting.eType = (DVR_EVENTREC_SOURCE_TYPE)type;
        Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_DIRECTORY_STRUCTURE, &Directory, sizeof(DVR_DISK_DIRECTORY), &status);
        strcpy(EventRecordSetting.szEventDir, Directory.szEventRecDir);
        Dvr_Sdk_Recorder_Set(DVR_RECORDER_PROP_EMERGENCY_SETTING, &EventRecordSetting, sizeof(DVR_EVENT_RECORD_SETTING));

        app->Func(REC_APP_FUNC_LOOP_RECORD_SWITCH, FALSE, 0); //turn off loop recording

        rec_app_set_loop_monitor(TRUE);

		Dvr_Sdk_Recorder_IaccRec_Start(type);

        app->IaccRecState = EVENT_REC_STATE_RUNNING;
        app->Gui(GUI_REC_APP_CMD_EMERGENCY_STATE_SHOW, 0, 0);
        app->Gui(GUI_REC_APP_CMD_FLUSH, 0, 0);
    }

    return res;
}

static DVR_RESULT rec_app_iacacc_record_start(int type)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrRecApp *app = DvrRecApp::Get();
	DVR_U32 EmergencyRecordStatus;//0x0=invalid,0x1=Unsuccessful,0x2=Success,0x3=Saving
	DVR_U32 EmergencyRecordSwitch = 1;  //0x0= off    0x01=1  on

    if(app->bEncodingHasStarted == FALSE)
    {
        Log_Message("IVA encoding process has not started, can not trigger event recording!!!");
        if(app->bIacccontinue == TRUE && app->bIaccFailedHasFinshed == FALSE)
            return DVR_RES_SOK;
        app->bIacccontinue = TRUE;
        app->bIaccFailedHasFinshed = FALSE;
        CThreadPool::Get()->AddTask(event_record_result_feedback_failed, app); 
        return DVR_RES_SOK;
    }

    if( TRUE == rec_app_feature_record_is_busy())
    {
        if(app->IaccRecState == EVENT_REC_STATE_RUNNING)
        {
            app->bIacccontinue = TRUE;
        }
        Log_Message("[%s:%d]another recording task is already running", __FUNCTION__, __LINE__);
        return DVR_RES_SOK;
    }

    if(app->bIacccontinue == TRUE && app->bIaccHasFinshed == FALSE)
    {
        Log_Message("[%s:%d]Iacc recording has not Stoped", __FUNCTION__, __LINE__);
        return DVR_RES_SOK;
    }

    res = app->Func(REC_APP_FUNC_CARD_CHECK_STATUS, 0, 0);
    if(APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NO_CARD) ||
        APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_ERROR_FORMAT) ||
        APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_LOW_SPEED) ||
		APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NOT_READY) ||
        APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NOT_ENOUGH_SPACE))
    {
        if(app->bIacccontinue == TRUE && app->bIaccFailedHasFinshed == FALSE)
            return DVR_RES_SOK;
        app->bIacccontinue = TRUE;
        app->bIaccFailedHasFinshed = FALSE;
        CThreadPool::Get()->AddTask(event_record_result_feedback_failed, app); 
        return res;
    }

    if(app->IaccRecState == EVENT_REC_STATE_IDLE)
    {
        app->bIacccontinue = FALSE;
        app->bIaccHasFinshed = FALSE;
        app->bIaccFailedHasFinshed = FALSE;
		EmergencyRecordStatus = 3;//0x0=invalid,0x1=Unsuccessful,0x2=Success,0x3=Saving
		DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_EVENT_RECORD_STATUS, (void *)&EmergencyRecordStatus, NULL);
        DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_EVENT_RECORD_SWITCH, (void *)&EmergencyRecordSwitch, NULL);

        DVR_S32 status;
        DVR_DISK_DIRECTORY Directory;
        memset(&Directory, 0, sizeof(DVR_DISK_DIRECTORY));

        DVR_EVENT_RECORD_SETTING EventRecordSetting;
        memset(&EventRecordSetting, 0, sizeof(DVR_EVENT_RECORD_SETTING));
	    EventRecordSetting.EventPreRecordLimitTime = 15*1000;
	    EventRecordSetting.EventRecordLimitTime = 30*1000;
        EventRecordSetting.eType = (DVR_EVENTREC_SOURCE_TYPE)type;
        Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_DIRECTORY_STRUCTURE, &Directory, sizeof(DVR_DISK_DIRECTORY), &status);

        if(type == DVR_EVENTREC_SOURCE_TYPE_IACC_ACC)
            app->DasRecType = DVR_DAS_TYPE_ACC;
        else if(type == DVR_EVENTREC_SOURCE_TYPE_IACC_IAC)
            app->DasRecType = DVR_DAS_TYPE_IACC;

        strcpy(EventRecordSetting.szEventDir, Directory.szDasDir);

        Dvr_Sdk_Recorder_Set(DVR_RECORDER_PROP_EMERGENCY_SETTING, &EventRecordSetting, sizeof(DVR_EVENT_RECORD_SETTING));

        app->Func(REC_APP_FUNC_LOOP_RECORD_SWITCH, FALSE, 0); //turn off loop recording

        rec_app_set_das_monitor(TRUE, app->DasRecType);

        rec_app_set_loop_monitor(TRUE);

		Dvr_Sdk_Recorder_IaccRec_Start(type);

        app->IaccRecState = EVENT_REC_STATE_RUNNING;
        app->Gui(GUI_REC_APP_CMD_EMERGENCY_STATE_SHOW, 0, 0);
        app->Gui(GUI_REC_APP_CMD_FLUSH, 0, 0);
    }

    return res;
}

static DVR_RESULT rec_app_iacc_record_start(int type)
{
    if(type == DVR_EVENTREC_SOURCE_TYPE_IACC_AEB)
        rec_app_aeb_record_start(type);
    else
        rec_app_iacacc_record_start(type);
    return DVR_RES_SOK;
}

static DVR_RESULT rec_app_iacc_record_finish(int bIsFailed, int type)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrRecApp *app = DvrRecApp::Get();

    if (app->IaccRecState == EVENT_REC_STATE_RUNNING)
    {
        rec_app_set_das_monitor(FALSE, app->DasRecType);

        rec_app_set_loop_monitor(FALSE);

        Dvr_Sdk_Recorder_IaccRec_Stop(0);

        app->IaccRecState = EVENT_REC_STATE_IDLE;

        rec_app_set_loop_monitor(FALSE);

        if( (TRUE == app->bUserEnableRec) && (TRUE == app->bPowerMode))
        {
            res = app->Func(REC_APP_FUNC_LOOP_RECORD_SWITCH, TRUE, 0); //turn on loop recording
        }
        
        app->Gui(GUI_REC_APP_CMD_EMERGENCY_STATE_HIDE, 0, 0);
        app->Gui(GUI_REC_APP_CMD_FLUSH, 0, 0);
        CThreadPool::Get()->AddTask(system_sync, NULL);
    }

    if (bIsFailed)
    {
        CThreadPool::Get()->AddTask(event_record_result_feedback_failed, app);
    }
    else
    {
        CThreadPool::Get()->AddTask(event_record_result_feedback_success, app);
    }

    app->bIaccApaIsInterrupted = FALSE;
    return res;
}

static DVR_RESULT rec_app_das_record_start()
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrRecApp *app = DvrRecApp::Get();

    if( app->AlarmRecState == EVENT_REC_STATE_RUNNING ||
		app->EventRecState == EVENT_REC_STATE_RUNNING || 
		app->IaccRecState == EVENT_REC_STATE_RUNNING)
    {
        Log_Message("[%s:%d]another recording task is already running", __FUNCTION__, __LINE__);
        return DVR_RES_SOK;
    }

    /* Check the card's status. */
	res = app->Func(REC_APP_FUNC_CARD_CHECK_STATUS, FALSE, 0);
	if(APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NO_CARD) ||
		APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_ERROR_FORMAT) ||
		APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_LOW_SPEED) ||
		APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NOT_READY) ||
		APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NOT_ENOUGH_SPACE))
	{
		return res;
	}

    if (app->DasRecState == EVENT_REC_STATE_IDLE)
    {
        app->DasRecType = DVR_DAS_TYPE_APA;

		app->Func(REC_APP_FUNC_LOOP_RECORD_SWITCH, FALSE, 0); //turn off loop recording

        rec_app_set_das_monitor(TRUE, app->DasRecType);

        rec_app_set_loop_monitor(TRUE);

        Dvr_Sdk_Recorder_DasRec_Start();

        app->DasRecState = EVENT_REC_STATE_RUNNING;
    }
	else
	{
		Log_Message("DAS Recording is still running!!!!!");
        if(app->bDasStopCommandHasFired && app->bIaccApaIsInterrupted == FALSE)
		    app->bDasStopDelayProcessIsInterrupted = TRUE;
	}

	return res;
}

static DVR_RESULT rec_app_das_record_finish(DVR_BOOL bIsFailed)
{
	int res = 0;
	DvrRecApp *app = DvrRecApp::Get();

	
	if(bIsFailed == TRUE)
	{
		if (app->DasRecState == EVENT_REC_STATE_RUNNING)
		{
            rec_app_set_das_monitor(FALSE, app->DasRecType);

            rec_app_set_loop_monitor(FALSE);

			Dvr_Sdk_Recorder_DasRec_Stop();
						
			app->DasRecState = EVENT_REC_STATE_IDLE;
		}
	}
	else
	{
		if(app->bDasStopCommandHasFired == FALSE)
		{
			CThreadPool::Get()->AddTask(rec_app_das_record_stop_asyc, NULL);
			app->bDasStopCommandHasFired = TRUE;
		}
		else
		{
			Log_Message("das stop command has fired");
		}
	}
    return DVR_RES_SOK;
}

static DVR_RESULT rec_app_switch_app_by_tab(int tab)
{
	int res = 0;
	DvrRecApp *app = DvrRecApp::Get();

	switch (tab)
	{
	case GUI_MAIN_MENU_TAB_RECORD:
	{
		app->Gui(GUI_REC_APP_CMD_SET_LAYOUT, GUI_LAYOUT_RECORD, 0);
		app->Gui(GUI_REC_APP_CMD_MAIN_MENU_TAB_UPDATE, GUI_MAIN_MENU_TAB_RECORD, 4);

		if(app->RecCapState == REC_CAP_STATE_PREVIEW)
		{
			app->Gui(GUI_REC_APP_CMD_REC_STATE_HIDE, 0, 0);
			app->Gui(GUI_REC_APP_CMD_REC_STATE_UPDATE, GUI_REC_STATE_STOP, 4);
		}
		
		app->Gui(GUI_REC_APP_CMD_FLUSH, 0, 0);
		
		//app->Func(REC_APP_FUNC_SET_VIEW_INDEX, GUI_VIEW_INDEX_FRONT, 0);
		
    	DVR_U32 AVM_DVRModeFeedback = 0x1; //0:Inactive, 0x1:RealTimeMode, 0x2:ReplayMode, 0x3:SettingMode
        DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_MODE_UPDATE, (void *)&AVM_DVRModeFeedback, NULL);
	}
	break;

	case GUI_MAIN_MENU_TAB_THUMB:
	{	
		//res = app->Func(REC_APP_FUNC_RECORD_STOP, 0, 0);
        APP_REMOVEFLAGS(app->m_recorder.GFlags, APP_AFLAGS_BUSY);
		res |= DvrAppUtil_SwitchApp(Dvr_App_GetAppId(MDL_APP_THUMB_ID));
	}
	break;

	case GUI_MAIN_MENU_TAB_SETUP:
	{
		AppWidget_On(DVR_WIDGET_MENU_SETUP, 0);

    	DVR_U32 AVM_DVRModeFeedback = 0x3; //0:Inactive, 0x1:RealTimeMode, 0x2:ReplayMode, 0x3:SettingMode
        DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_MODE_UPDATE, (void *)&AVM_DVRModeFeedback, NULL);
	}	
	break;

	default:
		break;
	}

	return res;
}

static DVR_RESULT rec_app_switch_app_by_appId(int appId)
{
    int res = 0;
    DvrRecApp *app = DvrRecApp::Get();

    res = DvrAppUtil_SwitchApp(appId);

    return res;
}

static DVR_RESULT rec_app_set_video_quality(DVR_U32 eType)
{
	DVR_RESULT res = DVR_RES_SOK;
	DVR_U32 quality = DVR_VIDEO_QUALITY_SFINE;

	switch (eType)
	{
	case GUI_SETUP_VIDEO_QUALITY_SFINE:
		quality = DVR_VIDEO_QUALITY_SFINE;
		break;

	case GUI_SETUP_VIDEO_QUALITY_FINE:
		quality = DVR_VIDEO_QUALITY_FINE;
		break;

	case GUI_SETUP_VIDEO_QUALITY_NORMAL:
		quality = DVR_VIDEO_QUALITY_NORMAL;
		break;

	default:
		break;
	}

	res = Dvr_Sdk_Recorder_Set(DVR_RECORDER_PROP_VIDEO_QUALITY, &quality, sizeof(quality));

	return res;
}

static void rec_app_card_insert_async(void *arg)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrRecApp *app = DvrRecApp::Get();
	
	DVR_BOOL bFatalError = FALSE;
	Dvr_Sdk_Recorder_Set(DVR_RECORDER_PROP_FATAL_ERROR, &bFatalError, sizeof(DVR_BOOL));

	DvrSDCardSpeedTest speed_test;
	if(TRUE == speed_test.IsLowSpeedSDCard())
	{
		DVR_U32 DVR_SDCardErrorStatus = DVR_SDCARD_ERROR_STATUS_LOW_SPEED;
		DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_ERROR_STATUS_UPDATE, (void *)&DVR_SDCardErrorStatus, NULL);

		DVR_BOOL bIsLowSpeed = TRUE;
		Dvr_Sdk_StorageCard_Set(DVR_STORAGE_PROP_CARD_SPEED_STATUS, &bIsLowSpeed, sizeof(DVR_BOOL));
	}
	else
	{
		DVR_U32 DVR_SDCardErrorStatus = DVR_SDCARD_ERROR_STATUS_NO_ERROR;

        printf(" rec_app_card_insert_async send DVR_APP_NOTIFICATION_TYPE_CARD_ERROR_STATUS_UPDATE \n");
		DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_ERROR_STATUS_UPDATE, (void *)&DVR_SDCardErrorStatus, NULL);

		DVR_BOOL bIsLowSpeed = FALSE;
		Dvr_Sdk_StorageCard_Set(DVR_STORAGE_PROP_CARD_SPEED_STATUS, &bIsLowSpeed, sizeof(DVR_BOOL));		
	}	

	app->Gui(GUI_REC_APP_CMD_CARD_UPDATE, GUI_CARD_STATE_READY, 4);
	app->Gui(GUI_REC_APP_CMD_FLUSH, 0, 0);

	DVR_S32 status;
	DVR_DISK_DIRECTORY Directory;
	memset(&Directory, 0, sizeof(DVR_DISK_DIRECTORY));
	if (DVR_RES_SOK == Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_DIRECTORY_STRUCTURE, &Directory, sizeof(DVR_DISK_DIRECTORY), &status))
	{
		Dvr_Sdk_Recorder_Set(DVR_RECORDER_PROP_ROOT_DIRECTORY, &Directory, sizeof(DVR_DISK_DIRECTORY));
	}

	app->Func(REC_APP_FUNC_CARD_CHECK_STATUS, 0, 0); //check space, update card status to HU


    DVR_BOOL bCardNotReady = FALSE;
    Dvr_Sdk_StorageCard_Set(DVR_STORAGE_PROP_CARD_READY_STATUS, &bCardNotReady, sizeof(DVR_BOOL));
	if( (TRUE == app->bUserEnableRec) && (TRUE == app->bPowerMode) )
	{
		res = app->Func(REC_APP_FUNC_LOOP_RECORD_SWITCH, TRUE, 0); //turn on loop recording
	}
}

static DVR_RESULT rec_app_card_insert(void)
{
    return CThreadPool::Get()->AddTask(rec_app_card_insert_async, NULL);
}

static DVR_RESULT rec_app_card_remove(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrRecApp *app = DvrRecApp::Get();

#if 1
	DVR_BOOL bFatalError = TRUE;
	
	Dvr_Sdk_Recorder_Set(DVR_RECORDER_PROP_FATAL_ERROR, &bFatalError, sizeof(DVR_BOOL));

	
	app->Func(REC_APP_FUNC_LOOP_RECORD_SWITCH, 0, 0);	

	if(app->EventRecState == EVENT_REC_STATE_RUNNING)
	{
		app->Func(REC_APP_FUNC_EVENT_RECORD_COMPLETE, TRUE, 0);
	}

	if(app->AlarmRecState == EVENT_REC_STATE_RUNNING)
	{
		app->Func(REC_APP_FUNC_ALARM_RECORD_COMPLETE, 0, 0);
	}

	if(app->DasRecState == EVENT_REC_STATE_RUNNING)
	{
		app->Func(REC_APP_FUNC_DAS_RECORD_COMPLETE, TRUE, 0);
	}
	
    if (app->IaccRecState == EVENT_REC_STATE_RUNNING)
	{
		app->Func(REC_APP_FUNC_IACC_RECORD_COMPLETE, TRUE, 0);
	}
#endif	

	DVR_U32 DVR_SDCardErrorStatus = DVR_SDCARD_ERROR_STATUS_NO_SDCARD; //no card
	DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_ERROR_STATUS_UPDATE, (void *)&DVR_SDCardErrorStatus, NULL);

	DVR_U32 DVR_SDCardFullStatus = DVR_SDCARD_FULL_STATUS_NOT_FULL;
	DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_FULL_STATUS_UPDATE, (void *)&DVR_SDCardFullStatus, NULL);

	app->Gui(GUI_REC_APP_CMD_CARD_UPDATE, GUI_CARD_STATE_NO_CARD, 4);
	app->Gui(GUI_REC_APP_CMD_FLUSH, 0, 0);

    Dvr_Sdk_Recorder_AsyncOpFlush();
    CThreadPool::Get()->Flush();
	return res;
}

static DVR_RESULT rec_app_card_format(void)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrRecApp *app = DvrRecApp::Get();

    if(app->IaccRecState == EVENT_REC_STATE_RUNNING)
    {
        app->Func(REC_APP_FUNC_IACC_RECORD_COMPLETE, TRUE, 0);
    }

	if(app->RecCapState == REC_CAP_STATE_RECORD)
	{
		/* Stop recording. */
		app->Func(REC_APP_FUNC_LOOP_RECORD_STOP, 0, 0);
		
		DVR_U32 DVREnableSetStatus = 1;//0x0=invalid,0x1=off,0x2=on,0x3=especially
		DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_RECORDER_SWTICH, (void *)&DVREnableSetStatus, NULL);
	}	

    Dvr_Sdk_Recorder_AsyncOpFlush();
    CThreadPool::Get()->Flush();
    return res;
}

static DVR_RESULT rec_app_card_format_done(DVR_U32 param1)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrRecApp *app = DvrRecApp::Get();

	if(param1 == 0)
	{		
		DVR_S32 status;
		DVR_DISK_DIRECTORY Directory;
		memset(&Directory, 0, sizeof(DVR_DISK_DIRECTORY));
		if (DVR_RES_SOK == Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_DIRECTORY_STRUCTURE, &Directory, sizeof(DVR_DISK_DIRECTORY), &status))
		{
			Dvr_Sdk_Recorder_Set(DVR_RECORDER_PROP_ROOT_DIRECTORY, &Directory, sizeof(DVR_DISK_DIRECTORY));
		}	

		if( (TRUE == app->bUserEnableRec) && (TRUE == app->bPowerMode) )
		{
			res = app->Func(REC_APP_FUNC_LOOP_RECORD_SWITCH, TRUE, 0); //turn on loop recording
		}
	}

	return res;
}

static DVR_RESULT rec_app_card_check_status(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrRecApp *app = DvrRecApp::Get();

	res = Dvr_Sdk_StorageCard_CheckStatus();	
	if (res == DVR_STORAGE_CARD_STATUS_CHECK_PASS)
	{
		//DVR_U32 DVR_SDCardErrorStatus = DVR_SDCARD_ERROR_STATUS_NO_ERROR;
        //DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_ERROR_STATUS_UPDATE, (void *)&DVR_SDCardErrorStatus, NULL);

		DVR_U32 DVR_SDCardFullStatus = DVR_SDCARD_FULL_STATUS_NOT_FULL;
        DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_FULL_STATUS_UPDATE, (void *)&DVR_SDCardFullStatus, NULL);
	}
	else
	{
		if (APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NO_CARD))
		{
			Log_Error("!!!!!!!!!No SD Card!!!!!!!!\n");
			app->Gui(GUI_REC_APP_CMD_CARD_UPDATE, GUI_CARD_STATE_NO_CARD, 4);		
			app->Gui(GUI_REC_APP_CMD_REC_STATE_UPDATE, GUI_REC_STATE_STOP, 4);
	
			DVR_U32 DVR_SDCardErrorStatus = DVR_SDCARD_ERROR_STATUS_NO_SDCARD; //no card
			DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_ERROR_STATUS_UPDATE, (void *)&DVR_SDCardErrorStatus, NULL);
		}
		else if (APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_ERROR_FORMAT))
		{
			Log_Error("!!!!!!!!!SD Card Error Format!!!!!!!!\n");
	
			DVR_U32 DVR_SDCardErrorStatus = DVR_SDCARD_ERROR_STATUS_FORMAT_ERROR; //format error
			DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_ERROR_STATUS_UPDATE, (void *)&DVR_SDCardErrorStatus, NULL);
		}
		else if (APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_LOW_SPEED))
		{
			Log_Error("!!!!!!!!!Low Speed SD Card!!!!!!!!\n");
	
			DVR_U32 DVR_SDCardErrorStatus = DVR_SDCARD_ERROR_STATUS_LOW_SPEED; //low speed
			DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_ERROR_STATUS_UPDATE, (void *)&DVR_SDCardErrorStatus, NULL);
		}
		else if (APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NOT_ENOUGH_SPACE))
		{
			Log_Error("!!!!!!!!!Insufficient Space!!!!!!!!\n");
	
			DVR_U32 DVR_SDCardErrorStatus = DVR_SDCARD_ERROR_STATUS_INSUFFICIENT_SPACE; //insufficient space
			DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_ERROR_STATUS_UPDATE, (void *)&DVR_SDCardErrorStatus, NULL);
		}		
		else
		{
			//TODO
		}

		if(APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_EVENT_FOLDER_NOT_ENOUGH_SPACE) && 
			APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_PHOTO_FOLDER_NOT_ENOUGH_SPACE))
		{	
			Log_Error("SD Card both event and photo folder space is under threshold\n");
	
			DVR_U32 DVR_SDCardFullStatus = DVR_SDCARD_FULL_STATUS_EVENT_PHOTO_SPACE_FULL; //emergency and photo space full
			DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_FULL_STATUS_UPDATE, (void *)&DVR_SDCardFullStatus, NULL);
		}
		else if (APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_EVENT_FOLDER_NOT_ENOUGH_SPACE))
		{
			Log_Error("SD Card event folder space is under threshold\n");
			app->Gui(GUI_REC_APP_CMD_CARD_UPDATE, GUI_CARD_STATE_READY, 4);
	
			DVR_U32 DVR_SDCardFullStatus = DVR_SDCARD_FULL_STATUS_EVENT_SPACE_FULL; //emergency folder full
			DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_FULL_STATUS_UPDATE, (void *)&DVR_SDCardFullStatus, NULL);
		}
		else if (APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_PHOTO_FOLDER_NOT_ENOUGH_SPACE))
		{
			Log_Error("SD Card photo folder space is under threshold\n");
			app->Gui(GUI_REC_APP_CMD_CARD_UPDATE, GUI_CARD_STATE_READY, 4);
	
			DVR_U32 DVR_SDCardFullStatus = DVR_SDCARD_FULL_STATUS_PHOTO_SPACE_FULL; //photo space full
			DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_FULL_STATUS_UPDATE, (void *)&DVR_SDCardFullStatus, NULL);
		}
		else
		{
			//TODO
		}
	}

	return res;
}

static DVR_RESULT rec_app_card_full_handle(DVR_U32 maintype, DVR_U32 subtype)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrRecApp *app = DvrRecApp::Get();

    if (maintype == DVR_FOLDER_TYPE_NORMAL)
    {
        DVR_BOOL enable = 0;
        Dvr_Sdk_MonitorStorage_Set(DVR_MONITOR_STORAGE_PROP_ENABLE_NORMAL_MSG, &enable, sizeof(DVR_BOOL));

        Dvr_Sdk_StorageAsyncOp_SndMsg(HMSG_STORAGE_RUNOUT_HANDLE_START, DVR_FOLDER_TYPE_NORMAL, DVR_FILE_TYPE_VIDEO);
    }
    else if (maintype == DVR_FOLDER_TYPE_DAS)
    {
        if(subtype == 0)
        {
            DVR_DAS_MONITOR monitor;
            monitor.bEnable = 0;
            monitor.eType   = app->DasRecType;
            Dvr_Sdk_MonitorStorage_Set(DVR_MONITOR_STORAGE_PROP_ENABLE_DAS_MSG, &monitor, sizeof(DVR_DAS_MONITOR));   
            Dvr_Sdk_StorageAsyncOp_SndMsg(HMSG_STORAGE_DAS_RUNOUT_HANDLE_START, DVR_FOLDER_TYPE_DAS, DVR_FILE_TYPE_VIDEO);
        }
        else
        {
            DVR_DAS_MONITOR monitor;
            monitor.bEnable = 0;
            monitor.eType   = app->DasRecType;
            Dvr_Sdk_MonitorStorage_Set(DVR_MONITOR_STORAGE_PROP_ENABLE_DAS_MSG, &monitor, sizeof(DVR_DAS_MONITOR));   
            
            Dvr_Sdk_StorageAsyncOp_SndMsg(HMSG_STORAGE_DAS_NUM_RUNOUT_HANDLE_START, DVR_FOLDER_TYPE_DAS, subtype);
        }
    }
    else
    {
        //TODO
    }

	return res;
}

static DVR_RESULT rec_app_card_storage_busy(void)
{
	DVR_RESULT res = DVR_RES_SOK;

	return res;
}

static DVR_RESULT rec_app_card_storage_idle(void)
{
	DVR_RESULT res = DVR_RES_SOK;

	return res;
}

static DVR_RESULT rec_app_error_storage_fragment_err(void)
{
	DVR_RESULT res = DVR_RES_SOK;

	return res;
}

static DVR_RESULT rec_app_error_storage_runout(int eFolderType, int eSubType)
{
	DVR_RESULT res  = DVR_RES_SOK;
	DvrRecApp *app  = DvrRecApp::Get();

	/**call card full handle to do loop enc*/
    res = app->Func(REC_APP_FUNC_CARD_FULL_HANDLE, eFolderType, eSubType);
	
	return res;
}

static DVR_RESULT rec_app_error_storage_runout_handle_done(DVR_U32 eFolderType, DVR_U32 type)
{
    DVR_RESULT res = DVR_RES_SOK;

    if (eFolderType == DVR_FOLDER_TYPE_NORMAL)
    {
        DVR_BOOL enable = 1;
        Dvr_Sdk_MonitorStorage_Set(DVR_MONITOR_STORAGE_PROP_ENABLE_NORMAL_MSG, &enable, sizeof(DVR_BOOL));
    }
    else if (eFolderType == DVR_FOLDER_TYPE_DAS)
    {
        DvrRecApp *app = DvrRecApp::Get();
        DVR_DAS_MONITOR monitor;
        monitor.bEnable = 1;
        monitor.eType   = app->DasRecType;
        Dvr_Sdk_MonitorStorage_Set(DVR_MONITOR_STORAGE_PROP_ENABLE_DAS_MSG, &monitor, sizeof(DVR_DAS_MONITOR));   
    }
    else
    {
        //TODO
    }

    return res;
}

static DVR_RESULT rec_app_error_storage_runout_handle_error(int error_type, DVR_U8 param)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrRecApp *app = DvrRecApp::Get();

    DVR_BOOL enable = 1;
    Dvr_Sdk_MonitorStorage_Set(DVR_MONITOR_STORAGE_PROP_ENABLE_NORMAL_MSG, &enable, sizeof(DVR_BOOL));

    if (error_type != 0)
    {
        /*delete file fail, keep recording*/
    }
    else
    {
        /*search file fail, stop recording*/
		app->Func(REC_APP_FUNC_LOOP_RECORD_SWITCH, 0, 0);
    }

    return res;
}

int rec_app_set_view_index(int view)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrRecApp *app = DvrRecApp::Get();

	if (view >= GUI_VIEW_INDEX_NUM)
	{
        Log_Error("view index out of range\n");
		return DVR_RES_EFAIL;
	}

    app->curViewIndx = (GUI_VIEW_INDEX)view;
	app->Gui(GUI_REC_APP_CMD_VIEW_UPDATE, view, 4);
	app->Gui(GUI_REC_APP_CMD_FLUSH, 0, 0);

	
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

int rec_app_set_loop_record_split_time(int type)
{
	DVR_RESULT res = DVR_RES_SOK;
	DVR_RECORDER_PERIOD_OPTION option;
	DVR_U32 DVR_RecordingCycleSetStatus = 1; //1:1min, 2: 3min 3:5min

	switch (type)
	{
	case GUI_SETUP_SPLIT_TIME_1MIN:
    {
        DVR_RecordingCycleSetStatus = 1;
		option.nSplitTime = DVR_VIDEO_SPLIT_TIME_60_SECONDS;
    }
	break;
	case GUI_SETUP_SPLIT_TIME_3MIN:
    {
        DVR_RecordingCycleSetStatus = 2;
		option.nSplitTime = DVR_VIDEO_SPLIT_TIME_180_SECONDS;
    }
    break;

	case GUI_SETUP_SPLIT_TIME_5MIN:
    {
        DVR_RecordingCycleSetStatus = 3;
		option.nSplitTime = DVR_VIDEO_SPLIT_TIME_300_SECONDS;
    }
    break;
	default:
		break;
	}

	option.bImmediatelyEffect = FALSE;
	res = Dvr_Sdk_Recorder_Set(DVR_RECORDER_PROP_VIDEO_SPLIT_TIME, &option, sizeof(DVR_RECORDER_PERIOD_OPTION));

    DvrAppNotify *pHandle = DvrAppNotify::Get();
    pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_RECORDING_CYCLE_UPDATE, (void *)&DVR_RecordingCycleSetStatus, NULL);

	return res;
}

int rec_app_set_sidebar_status(DVR_U32 param1)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrRecApp *app = DvrRecApp::Get();

    if (param1 >= GUI_SIDEBAR_STATUS_NUM)
    {
        Log_Error("side bar status out of range\n");
        return DVR_RES_EFAIL;
    }

    if (param1 == GUI_SIDEBAR_STATUS_SHOW)
    {
        app->Gui(GUI_REC_APP_CMD_SIDEBAR_SHOW, 0, 0);
    }
    else
    {
        app->Gui(GUI_REC_APP_CMD_SIDEBAR_HIDE, 0, 0);
    }
   
    app->Gui(GUI_REC_APP_CMD_FLUSH, 0, 0);

    return res;
}

int rec_app_func(DVR_U32 funcId, DVR_U32 param1, DVR_U32 param2)
{
	DVR_RESULT res = DVR_RES_SOK;

	switch (funcId)
	{
	case REC_APP_FUNC_INIT:
		res = rec_app_init();
		break;

	case REC_APP_FUNC_START:
		res = rec_app_start();
		break;

	case REC_APP_FUNC_STOP:
		res = rec_app_stop();
		break;
	case REC_APP_FUNC_SNAPSHOT_SET:
		res = rec_app_snaphot_set(param1);
		break;
		
	case REC_APP_FUNC_SNAPSHOT:
		res = rec_app_snapshot(param1, param2);
		break;
	
	case REC_APP_FUNC_LOOP_RECORD_SWITCH:
		res = rec_app_loop_record_switch(param1, param2);
		break;

	case REC_APP_FUNC_LOOP_RECORD_START:
        res = rec_app_loop_record_start();
		break;

	case REC_APP_FUNC_LOOP_RECORD_STOP:
		res = rec_app_loop_record_stop(param1);
		break;

	case REC_APP_FUNC_EVENT_RECORD_START:
		res = rec_app_event_record_start(param1);
		break;

    case REC_APP_FUNC_EVENT_RECORD_COMPLETE:
        res = rec_app_event_record_finish(param1);
        break;

	case REC_APP_FUNC_DAS_RECORD_START:
		res = rec_app_das_record_start();
		break;

    case REC_APP_FUNC_DAS_RECORD_COMPLETE:
        res = rec_app_das_record_finish(param1);
        break;

    case REC_APP_FUNC_IACC_RECORD_START:
        res = rec_app_iacc_record_start(param1);
        break;

    case REC_APP_FUNC_IACC_RECORD_COMPLETE:
        res = rec_app_iacc_record_finish(param1, param2);
        break;

	case REC_APP_FUNC_SWITCH_APP_BY_APPID:
		res = rec_app_switch_app_by_appId(param1);
		break;

    case REC_APP_FUNC_SWITCH_APP_BY_TAB:
        res = rec_app_switch_app_by_tab(param1);
        break;

	case REC_APP_FUNC_SET_VIDEO_QUALITY:
		res = rec_app_set_video_quality(param1);
		break;

	case REC_APP_FUNC_CARD_INSERT:
		res = rec_app_card_insert();
		break;

	case REC_APP_FUNC_CARD_REMOVE:
		res = rec_app_card_remove();
		break;

    case REC_APP_FUNC_CARD_FORMAT:
        res = rec_app_card_format();
        break;

	case REC_APP_FUNC_CARD_FORMAT_DONE:
		res = rec_app_card_format_done(param1);
		break;

	case REC_APP_FUNC_CARD_CHECK_STATUS:
		res = rec_app_card_check_status();
		break;

	case REC_APP_FUNC_CARD_FULL_HANDLE:
		res = rec_app_card_full_handle(param1, param2);
		break;

	case REC_APP_FUNC_CARD_STORAGE_BUSY:
		res = rec_app_card_storage_busy();
		break;

	case REC_APP_FUNC_CARD_STORAGE_IDLE:
		res = rec_app_card_storage_idle();
		break;

	case REC_APP_FUNC_ERROR_STORAGE_FRAGMENT_ERR:
		res = rec_app_error_storage_fragment_err();
		break;

	case REC_APP_FUNC_ERROR_STORAGE_RUNOUT:
		res = rec_app_error_storage_runout(param1, param2);
		break;

	case REC_APP_FUNC_ERROR_STORAGE_RUNOUT_HANDLE_DONE:
		res = rec_app_error_storage_runout_handle_done(param1, param2);
		break;

	case REC_APP_FUNC_ERROR_STORAGE_RUNOUT_HANDLE_ERROR:
		res = rec_app_error_storage_runout_handle_error(param1, param2);
		break;

	case REC_APP_FUNC_SET_VIEW_INDEX:
		res = rec_app_set_view_index(param1);
		break;

    case REC_APP_FUNC_SET_LOOPREC_SPLIT_TIME:
        res = rec_app_set_loop_record_split_time(param1);
		break;

    case REC_APP_FUNC_SET_SIDEBAR_STATUS:
        res = rec_app_set_sidebar_status(param1);
        break;

	case REC_APP_FUNC_ALARM_RECORD_START:
		res = rec_app_alarm_record_start();
		break;

	case REC_APP_FUNC_ALARM_RECORD_COMPLETE:
		res = rec_app_alarm_record_finish(param1, param2);
		break;

	default:
		break;
	}

	return res;
}
