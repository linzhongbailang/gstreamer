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

#ifndef _DVR_PB_APP_H_
#define _DVR_PB_APP_H_

#include "DVR_APP_INTFS.h"
#include "DVR_SDK_INTFS.h"
#include "framework/DvrAppMgt.h"
#include "framework/DvrAppUtil.h"
#include "framework/DvrAppMain.h"
#include "pthread.h"

/*************************************************************************
* App Flag Definitions
************************************************************************/
#define PB_APP_FLAG_OPEN_FAILED		(0x00000001)


/*************************************************************************
* App Function Definitions
************************************************************************/
enum PB_APP_FUNC_ID
{
	PB_APP_FUNC_INIT,
	PB_APP_FUNC_START,
	PB_APP_FUNC_START_FLAG_ON,
	PB_APP_FUNC_STOP,
	PB_APP_FUNC_QUIT,

	PB_APP_FUNC_APP_READY,
	PB_APP_FUNC_START_DISP_PAGE,
	PB_APP_FUNC_SWITCH_APP_BY_APPID,
	PB_APP_FUNC_SWITCH_APP_BY_TAB,

	PB_APP_FUNC_OPEN_FILE,
	PB_APP_FUNC_CLOSE_FILE,
	PB_APP_FUNC_START_PLAY,
	PB_APP_FUNC_STOP_PLAY,
	PB_APP_FUNC_PAUSE_PLAY,
	PB_APP_FUNC_RESUME_PLAY,
	PB_APP_FUNC_FORWARD_PLAY,
	PB_APP_FUNC_REWIND_PLAY,
	PB_APP_FUNC_PREV_FILE,
	PB_APP_FUNC_NEXT_FILE,
	PB_APP_FUNC_SET_SPEED,
	PB_APP_FUNC_SET_POSITION,
	PB_APP_FUNC_SCREEN_SHOT,

	PB_APP_FUNC_CARD_INSERT,
	PB_APP_FUNC_CARD_REMOVE,
	PB_APP_FUNC_CARD_FORMAT,

	PB_APP_FUNC_DELETE_FILE_DIALOG_SHOW,
	PB_APP_FUNC_DELETE_FILE,
	PB_APP_FUNC_DELETE_FILE_COMPLETE,

	PB_APP_FUNC_SAVE_FILE_DIALOG_SHOW,
	PB_APP_FUNC_SAVE_FILE,
	PB_APP_FUNC_SAVE_FILE_COMPLETE,	

	PB_APP_FUNC_CHANGE_DISPLAY,

	PB_APP_FUNC_CHANGE_MODE,

	PB_APP_FUNC_WARNING_MSG_SHOW_START,
	PB_APP_FUNC_WARNING_MSG_SHOW_STOP,
	PB_APP_FUNC_DECODE_ERROR,

	PB_APP_FUNC_SET_VIEW_INDEX,
    PB_APP_FUNC_SET_SIDEBAR_STATUS,
};

/*************************************************************************
* App Operation Definitions
************************************************************************/
typedef enum
{
	PB_APP_STATE_PLAYER_IDLE,
	PB_APP_STATE_PLAYER_RUNNING,
	PB_APP_STATE_PLAYER_PAUSE,
	PB_APP_STATE_PLAYER_STOP
}PB_APP_STATE;

typedef enum
{
    PB_APP_MODE_DVR,
    PB_APP_MODE_ALGO,
    PB_APP_MODE_NUM
}PB_APP_MODE;

class DvrPlayBackApp
{
private:
	DvrPlayBackApp();
	DvrPlayBackApp(const DvrPlayBackApp&);
	const DvrPlayBackApp& operator = (const DvrPlayBackApp&);

public:
	static DvrPlayBackApp *Get(void);
	APP_MGT_INST *GetApp()
	{
		return &m_playback;
	}

	int(*Func)(DVR_U32 funcId, DVR_U32 param1, DVR_U32 param2);
	int(*Gui)(DVR_U32 guiCmd, DVR_U32 param1, DVR_U32 param2);

	APP_MGT_INST m_playback;

private:
	DVR_BOOL Init();
	DVR_BOOL m_bHasInited;

public:
	char CurFileName[APP_MAX_FN_SIZE];
	PB_APP_STATE playState;
    PB_APP_MODE playMode;
	GUI_VIEW_INDEX curViewIndx;
	unsigned long lastClickTime;
	unsigned long PBSpeedlastClickTime;
	
	DVR_RESULT OpenFileResult;
	DVR_BOOL bAutoSwitchIsInterrupted;
};


#endif //_DVR_PB_APP_H_