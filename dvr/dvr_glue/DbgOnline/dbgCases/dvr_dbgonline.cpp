#ifdef __ONLINE_DEBUG_ENABLE__

/*****************************************************************************
*                    Includes
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "reuse.h"
#include "DVR_GUI_OBJ.h"
#include "event/ofilm_msg_type.h"
#include <event/AvmEventTypes.h>
#include <event/AvmEvent.h>

#include "../inc/debug_online_common.h"   

#define LOG_ERROR printf("[%s:%d]==>",__func__, __LINE__);printf
#define LOG_INFO  printf("[%s:%d]==>",__func__, __LINE__);printf

static int _DvrDbg_Component_Exec(int argc,char *argv[]);
static void _DvrDbg_Component_Help(void);

static const Debug_Online_CbCmdParam_t dvr_dbg = {
	Name : "dvr",
	CmdCbFunc : _DvrDbg_Component_Exec,
	help : _DvrDbg_Component_Help,
};

/*****************************************************************************
*                    Static Function Prototypes
*****************************************************************************/


/*****************************************************************************
*                    Extern Function Definitions
*****************************************************************************/

#define DVRDBG_COMPONENT_HELP                  "help"

#define DVRDBG_COMPONENT_MAIN_MENU				"main_menu"
#define DVRDBG_COMPONENT_RECORDER				"recorder"
#define DVRDBG_COMPONENT_THUMB					"thumb"
#define DVRDBG_COMPONENT_PLAYER					"player"
#define DVRDBG_COMPONENT_SETUP					"setup"
#define DVRDBG_COMPONENT_DIALOG					"dialog"


static void _DvrDbg_Component_Help(void)
{
	LOG_INFO("COMPONENT %s: dvr online debug component\n",dvr_dbg.Name);
	LOG_INFO("Usage:\n");

	LOG_INFO("%s %s tab_id : switch the main menu, 0:record, 1:thumb, 2:setup\n",\
		dvr_dbg.Name,\
		DVRDBG_COMPONENT_MAIN_MENU );	
	LOG_INFO("=========================================================\n");

	LOG_INFO("%s %s switch : recording on/off\n",\
		dvr_dbg.Name,\
		DVRDBG_COMPONENT_RECORDER );
	LOG_INFO("%s %s photo : take a photo\n",\
		dvr_dbg.Name,\
		DVRDBG_COMPONENT_RECORDER );
	LOG_INFO("%s %s event: trigger event record\n",\
		dvr_dbg.Name,\
		DVRDBG_COMPONENT_RECORDER );
	LOG_INFO("%s %s view index: 0:front, 1:rear, 2:left, 3:right, 4:matts\n",\
		dvr_dbg.Name,\
		DVRDBG_COMPONENT_RECORDER );
	LOG_INFO("=========================================================\n");

	LOG_INFO("%s %s tab id: 0:normal, 1:emergency, 2:photo\n", \
		dvr_dbg.Name, \
		DVRDBG_COMPONENT_THUMB);
	LOG_INFO("%s %s nextpage\n", \
		dvr_dbg.Name, \
		DVRDBG_COMPONENT_THUMB);
	LOG_INFO("%s %s prevpage\n", \
		dvr_dbg.Name, \
		DVRDBG_COMPONENT_THUMB);
	LOG_INFO("%s %s sel_to_play index:0~5\n", \
		dvr_dbg.Name, \
		DVRDBG_COMPONENT_THUMB);
	LOG_INFO("%s %s edit_enter\n", \
		dvr_dbg.Name, \
		DVRDBG_COMPONENT_THUMB);
	LOG_INFO("%s %s edit_exit\n", \
		dvr_dbg.Name, \
		DVRDBG_COMPONENT_THUMB);
	LOG_INFO("%s %s edit_save\n", \
		dvr_dbg.Name, \
		DVRDBG_COMPONENT_THUMB);
	LOG_INFO("%s %s edit_sel index: 0~5\n", \
		dvr_dbg.Name, \
		DVRDBG_COMPONENT_THUMB);
	LOG_INFO("%s %s edit_sel_all\n", \
		dvr_dbg.Name, \
		DVRDBG_COMPONENT_THUMB);
	LOG_INFO("%s %s edit_delete\n", \
		dvr_dbg.Name, \
		DVRDBG_COMPONENT_THUMB);
	LOG_INFO("=========================================================\n");

	LOG_INFO("%s %s split_time option: 0:1min, 1:3min, 2:5min\n", \
		dvr_dbg.Name, \
		DVRDBG_COMPONENT_SETUP);
	LOG_INFO("%s %s video_quality option: 0:sfine, 1:fine, 2:normal\n", \
		dvr_dbg.Name, \
		DVRDBG_COMPONENT_SETUP);
	LOG_INFO("%s %s photo_quality option: 0:sfine, 1:fine, 2:normal\n", \
		dvr_dbg.Name, \
		DVRDBG_COMPONENT_SETUP);
	LOG_INFO("%s %s formatcard\n", \
		dvr_dbg.Name, \
		DVRDBG_COMPONENT_SETUP);

	LOG_INFO("%s %s option: 0:ok/no, 1:yes\n", \
		dvr_dbg.Name, \
		DVRDBG_COMPONENT_DIALOG);
	LOG_INFO("=========================================================\n");
	return;
}

static void _DvrDbg_Component_MainMenu(int argc,char *argv[])
{
	unsigned int param;
		
	if(argc<3)
	{
		LOG_INFO("ERROR: invalid argument number(%d)\n\n", argc);
		_DvrDbg_Component_Help();
		return;
	}

	param = atoi(argv[2]);
	if(param >= GUI_MAIN_MENU_TAB_NUM)
		return;

	AvmEvent *evt = AvmRequestEvent(CTRL_EVENT_A15_LOCAL_MSG_DVR_CMD);
	if (evt == NULL)
	{
		LOG_INFO("Request %u cmd event error!", AVM_EVENT_MSG_TYPE_A15_LOCAL_DVR_CMD);
		return ;
	}

	AvmEvent_CtrlCmd* pCmd = (AvmEvent_CtrlCmd*)evt->GetRawEvent()->payload;
	pCmd->MsgHead.MsgType = AVM_EVENT_MSG_TYPE_A15_LOCAL_DVR_CMD;
	pCmd->MsgHead.MsgSize = sizeof(AvmEvent_CtrlCmd);
	pCmd->parameter[0] = DVR_USER_CLICK_MAIN_MENU;
	pCmd->parameter[1] = param;
	pCmd->ctrl_status = 0;
	
	bool status = AvmPostEvent(*evt,NULL);
	
	AvmEventReleaseAndTrace(*evt);

	//_DvrDbg_Component_Help();

}

static void _DvrDbg_Component_Recorder(int argc,char *argv[])
{		
	if(argc<3)
	{
		LOG_INFO("ERROR: invalid argument number(%d)\n\n", argc);
		_DvrDbg_Component_Help();
		return;
	}

	AvmEvent *evt = AvmRequestEvent(CTRL_EVENT_A15_LOCAL_MSG_DVR_CMD);
	if (evt == NULL)
	{
		LOG_INFO("Request %u cmd event error!", AVM_EVENT_MSG_TYPE_A15_LOCAL_DVR_CMD);
		return ;
	}

	AvmEvent_CtrlCmd* pCmd = (AvmEvent_CtrlCmd*)evt->GetRawEvent()->payload;
	pCmd->MsgHead.MsgType = AVM_EVENT_MSG_TYPE_A15_LOCAL_DVR_CMD;
	pCmd->MsgHead.MsgSize = sizeof(AvmEvent_CtrlCmd);	
	pCmd->ctrl_status = 0;

	if(!strcasecmp("switch",argv[2]))
	{
		pCmd->parameter[0] = DVR_USER_CLICK_RECORD_SWITCH;
	
}
	else if(!strcasecmp("photo",argv[2]))
	{
		pCmd->parameter[0] = DVR_USER_CLICK_PHOTO_SHUTTER;
	}
	else if(!strcasecmp("event",argv[2]))
	{
		pCmd->parameter[0] = DVR_USER_CLICK_EVENT_RECORD;
	}
	else if(!strcasecmp("view",argv[2]))
	{
		unsigned int view_index = 0;
		if(argc > 3)
		{
			view_index = atoi(argv[3]);
		}
		if(view_index >= GUI_VIEW_INDEX_NUM)
			return;
		
		pCmd->parameter[0] = DVR_USER_CLICK_LIVE_VIEW;
		pCmd->parameter[1] = view_index;
	}
	else
	{
		goto exit;
	}
	
	AvmPostEvent(*evt,NULL);
	
exit:
	AvmEventReleaseAndTrace(*evt);
	//_DvrDbg_Component_Help();
	return;
}

static void _DvrDbg_Component_Thumb(int argc,char *argv[])
{
	if (argc < 3)
	{
		LOG_INFO("ERROR: invalid argument number(%d)\n\n", argc);
		_DvrDbg_Component_Help();
		return;
	}

	AvmEvent *evt = AvmRequestEvent(CTRL_EVENT_A15_LOCAL_MSG_DVR_CMD);
	if (evt == NULL)
	{
		LOG_INFO("Request %u cmd event error!", AVM_EVENT_MSG_TYPE_A15_LOCAL_DVR_CMD);
		return ;
	}

	AvmEvent_CtrlCmd* pCmd = (AvmEvent_CtrlCmd*)evt->GetRawEvent()->payload;
	pCmd->MsgHead.MsgType = AVM_EVENT_MSG_TYPE_A15_LOCAL_DVR_CMD;
	pCmd->MsgHead.MsgSize = sizeof(AvmEvent_CtrlCmd);	
	pCmd->ctrl_status = 0;	
	if (!strcasecmp("tab", argv[2]))
	{
		unsigned int tab_id = 0;
		if (argc > 3)
		{
			tab_id = atoi(argv[3]);
		}
		if (tab_id >= GUI_THUMB_TAB_NUM)
			return;

		pCmd->parameter[0] = DVR_USER_CLICK_THUMB_TAB;
		pCmd->parameter[1] = tab_id;
	}
	else if (!strcasecmp("nextpage", argv[2]))
	{
		pCmd->parameter[0] = DVR_USER_CLICK_THUMB_NEXT_PAGE;
	}
	else if (!strcasecmp("prevpage", argv[2]))
	{
		pCmd->parameter[0] = DVR_USER_CLICK_THUMB_PREV_PAGE;
	}
	else if (!strcasecmp("sel_to_play", argv[2]))
	{
		unsigned int sel_id = 0;
		if (argc > 3)
		{
			sel_id = atoi(argv[3]);
		}
		if (sel_id >= NUM_THUMBNAIL_PER_PAGE)
			return;
		pCmd->parameter[0] = DVR_USER_CLICK_THUMB_SEL_TO_PLAY;
		pCmd->parameter[1] = sel_id;
	}
	else if (!strcasecmp("edit_enter", argv[2]))
	{
		pCmd->parameter[0] = DVR_USER_CLICK_THUMB_EDIT;
	}
	else if (!strcasecmp("edit_exit", argv[2]))
	{
		pCmd->parameter[0] = DVR_USER_CLICK_THUMB_EDIT_CANCEL;
	}
	else if (!strcasecmp("edit_save", argv[2]))
	{
		pCmd->parameter[0] = DVR_USER_CLICK_THUMB_EDIT_SAVE;
	}
	else if (!strcasecmp("edit_sel", argv[2]))
	{
		unsigned int sel_id = 0;
		if (argc > 3)
		{
			sel_id = atoi(argv[3]);
		}
		if (sel_id >= NUM_THUMBNAIL_PER_PAGE)
			return;
		pCmd->parameter[0] = DVR_USER_CLICK_THUMB_EDIT_SEL;
		pCmd->parameter[1] = sel_id;
	}
	else if (!strcasecmp("edit_sel_all", argv[2]))
	{
		pCmd->parameter[0] = DVR_USER_CLICK_THUMB_EDIT_SELECT_ALL;
	}
	else if (!strcasecmp("edit_del", argv[2]))
	{
		pCmd->parameter[0] = DVR_USER_CLICK_THUMB_EDIT_DELETE;
	}
	else
	{
		goto exit;
	}

	AvmPostEvent(*evt,NULL);
	
exit:
	AvmEventReleaseAndTrace(*evt);
	//_DvrDbg_Component_Help();	

	return;

}

static void _DvrDbg_Component_Setup(int argc,char *argv[])
{
	if (argc < 3)
	{
		LOG_INFO("ERROR: invalid argument number(%d)\n\n", argc);
		_DvrDbg_Component_Help();
		return;
	}

	AvmEvent *evt = AvmRequestEvent(CTRL_EVENT_A15_LOCAL_MSG_DVR_CMD);
	if (evt == NULL)
	{
		LOG_INFO("Request %u cmd event error!", AVM_EVENT_MSG_TYPE_A15_LOCAL_DVR_CMD);
		return ;
	}

	AvmEvent_CtrlCmd* pCmd = (AvmEvent_CtrlCmd*)evt->GetRawEvent()->payload;
	pCmd->MsgHead.MsgType = AVM_EVENT_MSG_TYPE_A15_LOCAL_DVR_CMD;
	pCmd->MsgHead.MsgSize = sizeof(AvmEvent_CtrlCmd);	
	pCmd->ctrl_status = 0;		

	if (!strcasecmp("split_time", argv[2]))
	{
		unsigned int option = 0;
		if (argc > 3)
		{
			option = atoi(argv[3]);
		}
		if (option >= GUI_SETUP_SPLIT_TIME_NUM)
			return;

		pCmd->parameter[0] = DVR_USER_CLICK_SETUP_LOOPENC_SPLIT_TIME;
		pCmd->parameter[1] = option;
	}
	else if (!strcasecmp("video_quality", argv[2]))
	{
		unsigned int option = 0;
		if (argc > 3)
		{
			option = atoi(argv[3]);
		}
		if (option >= GUI_SETUP_VIDEO_QUALITY_NUM)
			return;
		pCmd->parameter[0] = DVR_USER_CLICK_SETUP_VIDEO_QUALITY;
		pCmd->parameter[1] = option;
	}
	else if (!strcasecmp("photo_quality", argv[2]))
	{
		unsigned int option = 0;
		if (argc > 3)
		{
			option = atoi(argv[3]);
		}
		if (option >= GUI_SETUP_PHOTO_QUALITY_NUM)
			return;
		pCmd->parameter[0] = DVR_USER_CLICK_SETUP_PHOTO_QUALITY;
		pCmd->parameter[1] = option;
	}
	else if (!strcasecmp("formatcard", argv[2]))
	{
		pCmd->parameter[0] = DVR_USER_CLICK_SETUP_FORMAT_CARD;
	}
	else
	{
		goto exit;
	}

	AvmPostEvent(*evt,NULL);
		
exit:
	AvmEventReleaseAndTrace(*evt);
	//_DvrDbg_Component_Help();	

	return;

}

static void _DvrDbg_Component_Player(int argc,char *argv[])
{
	if (argc < 3)
	{
		LOG_INFO("ERROR: invalid argument number(%d)\n\n", argc);
		_DvrDbg_Component_Help();
		return;
	}

	AvmEvent *evt = AvmRequestEvent(CTRL_EVENT_A15_LOCAL_MSG_DVR_CMD);
	if (evt == NULL)
	{
		LOG_INFO("Request %u cmd event error!", AVM_EVENT_MSG_TYPE_A15_LOCAL_DVR_CMD);
		return ;
	}

	AvmEvent_CtrlCmd* pCmd = (AvmEvent_CtrlCmd*)evt->GetRawEvent()->payload;
	pCmd->MsgHead.MsgType = AVM_EVENT_MSG_TYPE_A15_LOCAL_DVR_CMD;
	pCmd->MsgHead.MsgSize = sizeof(AvmEvent_CtrlCmd);	
	pCmd->ctrl_status = 0;	

	if (!strcasecmp("delete", argv[2]))
	{
		pCmd->parameter[0] = DVR_USER_CLICK_PLAYER_DELETE;
	}
	else if (!strcasecmp("save", argv[2]))
	{
		pCmd->parameter[0] = DVR_USER_CLICK_PLAYER_SAVE;
	}
	else if (!strcasecmp("play_pause", argv[2]))
	{
		pCmd->parameter[0] = DVR_USER_CLICK_PLAYER_PLAY;
	}
	else if (!strcasecmp("dc_switch", argv[2]))
	{
		pCmd->parameter[0] = DVR_USER_CLICK_PLAYER_DC_SWITCH;
	}
	else if (!strcasecmp("prev", argv[2]))
	{
		pCmd->parameter[0] = DVR_USER_CLICK_PLAYER_PREV;
	}
	else if (!strcasecmp("next", argv[2]))
	{
		pCmd->parameter[0] = DVR_USER_CLICK_PLAYER_NEXT;
	}
	else if (!strcasecmp("speed", argv[2]))
	{
		unsigned int option = 0;
		if (argc > 3)
		{
			option = atoi(argv[3]);
		}
		if (option >= GUI_PLAY_SPEED_NUM)
			return;
		pCmd->parameter[0] = DVR_USER_CLICK_PLAYER_SPEED;
		pCmd->parameter[1] = option;
	}
	else if (!strcasecmp("screenshot", argv[2]))
	{
		pCmd->parameter[0] = DVR_USER_CLICK_PLAYER_SCREEN_SHOT;
	}
	else if (!strcasecmp("view", argv[2]))
	{
		unsigned int view_index = 0;
		if (argc > 3)
		{
			view_index = atoi(argv[3]);
		}
		if (view_index >= GUI_VIEW_INDEX_NUM)
			return;

		pCmd->parameter[0] = DVR_USER_CLICK_PLAYER_VIEW;
		pCmd->parameter[1] = view_index;
	}
	else if (!strcasecmp("quit", argv[2]))
	{
		pCmd->parameter[0] = DVR_USER_CLICK_PLAYER_QUIT;
	}
	else
	{
		goto exit;
	}
	
	AvmPostEvent(*evt,NULL);
		
exit:
	AvmEventReleaseAndTrace(*evt);
	//_DvrDbg_Component_Help();

	return;

}

static void _DvrDbg_Component_Dialog(int argc, char *argv[])
{
	if (argc < 3)
	{
		LOG_INFO("ERROR: invalid argument number(%d)\n\n", argc);
		_DvrDbg_Component_Help();
		return;
	}

	AvmEvent *evt = AvmRequestEvent(CTRL_EVENT_A15_LOCAL_MSG_DVR_CMD);
	if (evt == NULL)
	{
		LOG_INFO("Request %u cmd event error!", AVM_EVENT_MSG_TYPE_A15_LOCAL_DVR_CMD);
		return ;
	}

	AvmEvent_CtrlCmd* pCmd = (AvmEvent_CtrlCmd*)evt->GetRawEvent()->payload;
	pCmd->MsgHead.MsgType = AVM_EVENT_MSG_TYPE_A15_LOCAL_DVR_CMD;
	pCmd->MsgHead.MsgSize = sizeof(AvmEvent_CtrlCmd);	
	pCmd->ctrl_status = 0;	

	unsigned int option = 0;
	if (argc > 2)
	{
		option = atoi(argv[2]);
	}
	if (option >= DIALOG_SEL_NUM)
		return;
	
	pCmd->parameter[0] = DVR_USER_CLICK_DIALOG;
	pCmd->parameter[1] = option;
	
	AvmPostEvent(*evt,NULL);
			
	AvmEventReleaseAndTrace(*evt);
	//_DvrDbg_Component_Help();

	return;
}

static int _DvrDbg_Component_Exec(int argc,char *argv[])
{
	if(argc>1)
	{
		if(!strcasecmp(DVRDBG_COMPONENT_MAIN_MENU,argv[1]))
		{
			_DvrDbg_Component_MainMenu(argc, argv);
			return 0;
		}
		else if(!strcasecmp(DVRDBG_COMPONENT_RECORDER,argv[1]))
		{
			_DvrDbg_Component_Recorder(argc, argv);
			return 0;
		}
		else if(!strcasecmp(DVRDBG_COMPONENT_THUMB,argv[1]))
		{
			_DvrDbg_Component_Thumb(argc, argv);
			return 0;
		}
		else if(!strcasecmp(DVRDBG_COMPONENT_PLAYER,argv[1]))
		{
			_DvrDbg_Component_Player(argc, argv);
			return 0;
		}
		else if(!strcasecmp(DVRDBG_COMPONENT_SETUP,argv[1]))
		{
			_DvrDbg_Component_Setup(argc, argv);
			return 0;
		}
		else if (!strcasecmp(DVRDBG_COMPONENT_DIALOG, argv[1]))
		{
			_DvrDbg_Component_Dialog(argc, argv);
			return 0;
		}
		else
		{
			LOG_ERROR("ERROR: invalid dvr component(%s)\n\n", argv[1]);
		}
	}

	_DvrDbg_Component_Help();
	
	return -1;
}

void dbgOnline_RegistDvrCmd(void *handle)
{
    if(Debug_Online_Server_RegistCmd(&dvr_dbg))
    {
        LOG_INFO("ERROR: dvr debug register FAILED\n");
        return;
    }

    LOG_INFO("Dvr debug register SUCCESS\n");

    return;
}

#endif //#ifdef __ONLINE_DEBUG_ENABLE__

/*****************************************************************************
*                    End Of File
*****************************************************************************/

