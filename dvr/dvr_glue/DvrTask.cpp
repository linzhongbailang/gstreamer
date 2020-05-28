#include "DvrTask.h"
#include <log/log.h>
#include <memory.h>
#include <errno.h>
#include <unistd.h>
#include <glib/gstdio.h>
#include <gio/gio.h>

#include "database/Database.h"
#include <event/AvmEventTypes.h>
#include <event/AvmEvent.h>
#include "DVR_APP_INTFS.h"
#include "DVR_SDK_INTFS.h"
#include "DVR_GUI_OBJ.h"

#include <event/AvmEvent.h>
#include <event/AvmEventsManager.h>

#include <event/AvmEventTypes.h>

IMPLEMENT_CLASS_CREATE_INFO(DvrTask);

BEGIN_TASK_EVENT_TABLE(DvrTask, BaseTask)
    TASK_EVENT_ENTRY(CTRL_EVENT_A15_LOCAL_MSG_REALTIME_IMAGE, &DvrTask::OnNewImgData)
    TASK_EVENT_ENTRY(CTRL_EVENT_A15_LOCAL_MSG_PLAYBACK_IMAGE, &DvrTask::OnNewPlaybackData)
    
END_TASK_EVENT_TABLE()


DvrTask::DvrTask(const char* name, ConfigItem* cfg) : BaseTask(name, cfg)
{
    //AvmRegisterEventHandler(CTRL_EVENT_A15_LOCAL_MSG_REALTIME_IMAGE, this);
	//AvmRegisterEventHandler(CTRL_EVENT_A15_LOCAL_MSG_PLAYBACK_IMAGE, this);
    //AvmRegisterEventHandler(CTRL_EVENT_A15_LOCAL_MSG_DVR_CMD, this);

    //Bind(AVM_EVENT_ANY, (AvmEventHandleEntry)&DvrTask::OnEvent2);
}

DvrTask::~DvrTask()
{
}

bool DvrTask::ExtraInit()
{
	int res = 0;

	//sleep(3);

    res = Dvr_App_Initialize();
	if(res != 0)
	{
		return false;
	}

	return true;
}

bool DvrTask::DeferredInit()
{
    SetMaxPendingEventCount(AvmGetEventTypeByName("CTRL_EVENT_A15_LOCAL_MSG_REALTIME_IMAGE"), 8);    
    SetMaxPendingEventCount(AvmGetEventTypeByName("CTRL_EVENT_A15_LOCAL_MSG_PLAYBACK_IMAGE"), 8);    
    SetMaxPendingEventCount(AvmGetEventTypeByName("OUTPUT_EVENT_A15_IPU1_MSG"), 8);    
    
	return true;
}

void DvrTask::ExtraDeInit()
{
	Dvr_App_DeInitialize();
}

int DvrTask::OnNewPlaybackData(AvmEvent& event)
{
	DVR_RESULT res = DVR_RES_SOK;

	AvmEventType type = event.GetEventType();
	if(type == CTRL_EVENT_A15_LOCAL_MSG_PLAYBACK_IMAGE)
	{
		Msg_CtrlImageData* pInput = (Msg_CtrlImageData*)event.GetRawEvent()->payload;
			
		DVR_IO_FRAME input_frame;
		memset(&input_frame, 0, sizeof(DVR_IO_FRAME));
		
		memcpy(input_frame.pImageBuf, pInput->Image, 4 * sizeof(void *));
		input_frame.pMattsImage = NULL;
		input_frame.pCanBuf = (DVR_U8*)pInput->VehicleData;
		input_frame.nCanSize = sizeof(Ofilm_Can_Data_T);
		input_frame.nSingleViewWidth = pInput->ImageWidth;
		input_frame.nSingleViewHeight = pInput->ImageHeight;
		input_frame.crop.x = pInput->crop.x;
		input_frame.crop.y = pInput->crop.y;
		input_frame.crop.width = pInput->crop.width;
		input_frame.crop.height = pInput->crop.height;
        //Log_Message("OnNewPlaybackData  pCanBuf:0x%p\n", input_frame.pCanBuf);
		res = Dvr_Sdk_Player_Frame_Update(&input_frame);
	}

	return res;    
}

int DvrTask::OnNewImgData(AvmEvent& event)
{
	DVR_RESULT res = DVR_RES_SOK;

	AvmEventType type = event.GetEventType();

	if (type == CTRL_EVENT_A15_LOCAL_MSG_REALTIME_IMAGE)
	{
		Msg_CtrlImageData* pInput = (Msg_CtrlImageData*)event.GetRawEvent()->payload;
		if (pInput->MergeImageValid)
		{
			DVR_IO_FRAME input_frame;
			memset(&input_frame, 0, sizeof(DVR_IO_FRAME));
		
			memcpy(input_frame.pImageBuf, pInput->Image, 4 * sizeof(void *));
			input_frame.pMattsImage = /*(DVR_U8 *)*/pInput->RecordMergeImage;
			input_frame.pCanBuf = (DVR_U8*)pInput->VehicleData;
			input_frame.nCanSize = sizeof(Ofilm_Can_Data_T);
			input_frame.nSingleViewWidth = pInput->ImageWidth;
			input_frame.nSingleViewHeight = pInput->ImageHeight;
            input_frame.nFramelength = pInput->framelength;
            input_frame.IsKeyFrame = pInput->IsKeyFrame;
            input_frame.nTimeStamp = pInput->TimeStamp;
			
			input_frame.crop.x = pInput->crop.x;
			input_frame.crop.y = pInput->crop.y;
			input_frame.crop.width = pInput->crop.width;
			input_frame.crop.height = pInput->crop.height;				
		    input_frame.nTimeStamp=pInput->TimeStamp;
            //printf(" DvrTask::OnNewImgData rec Msg_CtrlImageData.TimeStamp=%x \n",pInput->TimeStamp);
       
			res = Dvr_Sdk_Recorder_AddFrame(&input_frame);
		}
	}
	else if (type == CTRL_EVENT_A15_LOCAL_MSG_DVR_CMD)
	{
		AvmEvent_CtrlCmd* pCmd = (AvmEvent_CtrlCmd*)event.GetRawEvent()->payload;

		if(pCmd->MsgHead.MsgType == AVM_EVENT_MSG_TYPE_A15_LOCAL_DVR_CMD)
		{
			OnCommand(pCmd);
		}
	}

	return res;    
}

int DvrTask::OnCommand(AvmEvent_CtrlCmd *pDvrCmd)
{
	DVR_RESULT res = DVR_RES_SOK;
	int eCmdType = pDvrCmd->parameter[0];
	int param = pDvrCmd->parameter[1];

	switch (pDvrCmd->parameter[0])
	{
	case DVR_USER_CLICK_MAIN_MENU:
		Log_Message("click main menu, change to menu %d\n", param);
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_MAIN_MENU_TAB, param, 4);
		break;

	case DVR_USER_CLICK_RECORD_SWITCH:
		Log_Message("click recorder switch\n");
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_RECORDER_ONOFF, 0, 0);
		break;

	case DVR_USER_CLICK_PHOTO_SHUTTER:
		Log_Message("click photo shutter\n");
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_RECORDER_SNAPSHOT, 0, 0);
		break;

	case DVR_USER_CLICK_EVENT_RECORD:
		Log_Message("click emergency record\n");
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_RECORDER_EVENT_RECORD, 0, 0);
		break;

	case DVR_USER_CLICK_LIVE_VIEW:
		Log_Message("slect live view:%d\n", param);
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_RECORDER_VIEW_INDEX, param, 4);
		break;

	case DVR_USER_CLICK_THUMB_TAB:
		Log_Message("select thumb tab %d\n", param);
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_THUMB_TAB, param, 4);
		break;

	case DVR_USER_CLICK_THUMB_NEXT_PAGE:
		Log_Message("thumb next page\n");
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_THUMB_NEXT_PAGE, 0, 0);
		break;

	case DVR_USER_CLICK_THUMB_PREV_PAGE:
		Log_Message("thumb previous page\n");
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_THUMB_PREV_PAGE, 0, 0);
		break;

	case DVR_USER_CLICK_THUMB_SEL_TO_PLAY:
		Log_Message("select file[%d] to play\n", param);
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_THUMB_SEL_TO_PLAY, param, 4);
		break;

	case DVR_USER_CLICK_THUMB_EDIT:
		Log_Message("enter thumb edit mode\n");
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_THUMB_EDIT_ENTER, 0, 0);
		break;

	case DVR_USER_CLICK_THUMB_EDIT_CANCEL:
		Log_Message("exit thumb edit mode\n");
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_THUMB_EDIT_QUIT, 0, 0);
		break;

	case DVR_USER_CLICK_THUMB_EDIT_SAVE:
		Log_Message("copy selected files to emergency folder\n");
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_THUMB_EDIT_COPY, 0, 0);
		break;

	case DVR_USER_CLICK_THUMB_EDIT_DELETE:
		Log_Message("delete selected files\n");
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_THUMB_EDIT_DEL, 0, 0);
		break;

	case DVR_USER_CLICK_THUMB_EDIT_SELECT_ALL:
		Log_Message("select all\n");
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_THUMB_EDIT_SEL_ALL, 0, 0);
		break;

	case DVR_USER_CLICK_THUMB_EDIT_SEL:
		Log_Message("select file[%d]\n", param);
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_THUMB_EDIT_SEL, param, 4);
		break;

	case DVR_USER_CLICK_PLAYER_MODE:
		Log_Message("change playback mode\n");
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_PLAYER_MODE_CHANGE, 0, 0);
		break;

	case DVR_USER_CLICK_PLAYER_SAVE:
		Log_Message("copy current playing file to emergency folder\n");
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_PLAYER_COPY, 0, 0);
		break;

	case DVR_USER_CLICK_PLAYER_DELETE:
		Log_Message("delete current playing file\n");
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_PLAYER_DEL, 0, 0);
		break;

	case DVR_USER_CLICK_PLAYER_DC_SWITCH:
		Log_Message("click distortion correction switch\n");
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_PLAYER_DC_ONOFF, 0, 0);
		break;

	case DVR_USER_CLICK_PLAYER_VIEW:
		Log_Message("select play view %d\n", param);
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_PLAYER_VIEW_INDEX, param, 4);
		break;

	case DVR_USER_CLICK_PLAYER_PLAY:
		Log_Message("click play/pause\n");
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_PLAYER_PLAY, 0, 0);
		break;

	case DVR_USER_CLICK_PLAYER_PREV:
		Log_Message("play previous file\n");
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_PLAYER_PREV, 0, 0);
		break;

	case DVR_USER_CLICK_PLAYER_NEXT:
		Log_Message("play next file\n");
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_PLAYER_NEXT, 0, 0);
		break;

	case DVR_USER_CLICK_PLAYER_SPEED:
		Log_Message("change play speed to %d\n", param);
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_PLAYER_SPEED, param, 0);
		break;

	case DVR_USER_CLICK_PLAYER_SEEK:
		Log_Message("change play speed to %d\n", param);
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_PLAYER_SEEK, param, 0);
		break;

	case DVR_USER_CLICK_PLAYER_SCREEN_SHOT:
		Log_Message("click player screen shot");
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_PLAYER_SCREEN_SHOT, 0, 0);
		break;

	case DVR_USER_CLICK_PLAYER_QUIT:
		Log_Message("quit player\n");
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_PLAYER_QUIT, 0, 0);
		break;

	case DVR_USER_CLICK_SETUP_LOOPENC_SPLIT_TIME:
		Log_Message("click loopenc split time setting\n");
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_RECORDER_LOOPREC_SPLIT_TIME, param, 4);
		break;

	case DVR_USER_CLICK_SETUP_VIDEO_QUALITY:
		Log_Message("click video quality setting\n");
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_RECORDER_SET_VIDEO_QUALITY, param, 4);
		break;

	case DVR_USER_CLICK_SETUP_PHOTO_QUALITY:
		Log_Message("click photo quality setting\n");
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_RECORDER_SET_PHOTO_QUALITY, param, 4);
		break;

	case DVR_USER_CLICK_SETUP_FORMAT_CARD:
		Log_Message("click format card\n");
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_FORMAT_CARD, 0, 0);
		break;

	case DVR_USER_CLICK_DIALOG:
		Log_Message("click dialog %d\n", param);
		res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_DIALOG_SET, param, 4);
		break;

    case DVR_USER_CLICK_SIDEBAR:
        Log_Message("click sidebar");
        res = Dvr_Sdk_ComSvcHcMgr_SndMsg(HMSG_USER_CMD_SIDEBAR_SET, param, 4);
        break;

	case DVR_SHUTDOWN:
		//TODO
		break;

	default:
		break;

	}

	return res;
}


