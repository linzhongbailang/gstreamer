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

#ifndef _DVR_REC_APP_H_
#define _DVR_REC_APP_H_

#include "DVR_APP_INTFS.h"
#include "DVR_SDK_INTFS.h"
#include "framework/DvrAppMgt.h"
#include "framework/DvrAppUtil.h"
#include "framework/DvrAppMain.h"

#define ALARM_REC_LIMIT_TIME_300	5*60*1000
#define ALARM_REC_LIMIT_TIME_30		30*1000
/*************************************************************************
* App Function Definitions
************************************************************************/
enum REC_APP_FUNC_ID
{
	REC_APP_FUNC_INIT,
	REC_APP_FUNC_START,
	REC_APP_FUNC_START_FLAG_ON,
	REC_APP_FUNC_STOP,

	REC_APP_FUNC_SNAPSHOT,
	REC_APP_FUNC_SNAPSHOT_SET,
	

	REC_APP_FUNC_LOOP_RECORD_SWITCH,
	REC_APP_FUNC_LOOP_RECORD_START,
	REC_APP_FUNC_LOOP_RECORD_STOP,

	REC_APP_FUNC_EVENT_RECORD_START,
    REC_APP_FUNC_EVENT_RECORD_COMPLETE,

	REC_APP_FUNC_DAS_RECORD_START,
    REC_APP_FUNC_DAS_RECORD_COMPLETE,

    REC_APP_FUNC_IACC_RECORD_START,
    REC_APP_FUNC_IACC_RECORD_COMPLETE,

	REC_APP_FUNC_ALARM_RECORD_START,
	REC_APP_FUNC_ALARM_RECORD_COMPLETE,

	REC_APP_FUNC_SWITCH_APP_BY_APPID,
    REC_APP_FUNC_SWITCH_APP_BY_TAB,

	REC_APP_FUNC_SET_VIDEO_QUALITY,
	REC_APP_FUNC_SET_PHOTO_QUALITY,

	REC_APP_FUNC_CARD_INSERT,
	REC_APP_FUNC_CARD_REMOVE,
    REC_APP_FUNC_CARD_FORMAT,
    REC_APP_FUNC_CARD_FORMAT_DONE,
	REC_APP_FUNC_CARD_CHECK_STATUS,
	REC_APP_FUNC_CARD_FULL_HANDLE,

	REC_APP_FUNC_CARD_STORAGE_BUSY,
	REC_APP_FUNC_CARD_STORAGE_IDLE,
	REC_APP_FUNC_ERROR_STORAGE_FRAGMENT_ERR,
	REC_APP_FUNC_ERROR_STORAGE_RUNOUT,
	REC_APP_FUNC_ERROR_STORAGE_RUNOUT_HANDLE_DONE,
	REC_APP_FUNC_ERROR_STORAGE_RUNOUT_HANDLE_ERROR,

	REC_APP_FUNC_SET_VIEW_INDEX,
    REC_APP_FUNC_SET_LOOPREC_SPLIT_TIME,

    REC_APP_FUNC_SET_SIDEBAR_STATUS,
};

/*************************************************************************
* App Operation Definitions
************************************************************************/
typedef enum
{
	REC_CAP_STATE_PREVIEW,
	REC_CAP_STATE_RECORD,
}REC_CAP_STATE;

typedef enum
{
	EVENT_REC_STATE_IDLE,
	EVENT_REC_STATE_RUNNING
}EVENT_REC_STATE;

class DvrRecApp
{
private:
	DvrRecApp();
	DvrRecApp(const DvrRecApp&);
	const DvrRecApp& operator = (const DvrRecApp&);

public:
	static DvrRecApp *Get(void);
	APP_MGT_INST *GetApp()
	{
		return &m_recorder;
	}

	int(*Func)(DVR_U32 funcId, DVR_U32 param1, DVR_U32 param2);
	int(*Gui)(DVR_U32 guiCmd, DVR_U32 param1, DVR_U32 param2);

	APP_MGT_INST m_recorder;
	DVR_BOOL m_bSnapShotEnable;

private:
	DVR_BOOL Init();
	DVR_BOOL m_bHasInited;

public:
	REC_CAP_STATE RecCapState;
    GUI_VIEW_INDEX curViewIndx;
	DVR_BOOL bUserEnableRec;
	EVENT_REC_STATE EventRecState;
	EVENT_REC_STATE AlarmRecState;
	EVENT_REC_STATE DasRecState;
    EVENT_REC_STATE IaccRecState;
	DVR_DAS_TYPE	DasRecType;
	DVR_U32 		AlarmRecLastLimitTime;
	DVR_BOOL bPowerMode;
	DVR_BOOL bRecorderSwitchLast;
	
	DVR_BOOL bEncodingHasStarted;
	DVR_BOOL bIacccontinue;
	DVR_BOOL bIaccHasFinshed;
	DVR_BOOL bIaccFailedHasFinshed;
	DVR_BOOL bEventcontinue;
	DVR_BOOL beventRecHasTrigger;
	DVR_BOOL bIaccApaIsInterrupted;
	DVR_BOOL bAlarmRecHasTrigger;
	DVR_BOOL bDasStopCommandHasFired;
	DVR_BOOL bDasStopDelayProcessIsInterrupted;
};

#endif
