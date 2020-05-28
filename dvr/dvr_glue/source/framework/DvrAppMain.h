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
#ifndef _DVR_APP_MAIN_H_
#define _DVR_APP_MAIN_H_

#include "DVR_APP_INTFS.h"
#include "DVR_SDK_INTFS.h"
#include "HcMgr_Handler.h"
#include "graphics/DvrGraphics.h"
#include <queue>

#ifdef WIN32
#include <windows.h>
#include "AvmEvent.h"
#else
#include <event/AvmEventTypes.h>
#endif

#define DVR_PHOTO_PLAYBACK_BUFFER_WIDTH		1280
#define DVR_PHOTO_PLAYBACK_BUFFER_HEIGHT	720

class HcMgrHandler;
class DvrGuiResHandler;

typedef struct
{
	DVR_U32 AppSwitchBlocked;
	char selFileName[APP_MAX_FN_SIZE];
	DVR_FILE_TYPE selFileType;
	DVR_FOLDER_TYPE selFolderType;
}APP_STATUS;

class DvrAppMainControl
{
public:
	DvrAppMainControl();
	~DvrAppMainControl();

	static DvrAppMainControl *Get();
	static void Destory();
    static DvrAppMainControl *m_pDvrMainCtrl;

#ifdef WIN32
    static int OnEvent(void *p_event);
    int OnCommand(void *pCmd);
#endif
	HcMgrHandler *GetHcMgrHandler()
	{
		return m_hcMgrHandler;
	}

	DvrGuiResHandler *GetGuiResHandler()
	{
		return m_guiResHandler;
	}

	APP_STATUS *GetAppStatus()
	{
		return &m_status;
	}

	static DVR_APP_MODE GetCurMode()
	{
		return m_curAppMode;
	}

	static void Console(void *pContext, int nLevel, const char *szString);
	static void DvrPlayerOnNewFrame(void *pInstance, void *pUserData);
	static void DvrAppMessageCallback(void *pContext, int enuType, void *pParam1, void *pParam2);

private:
	DvrAppMainControl(const DvrAppMainControl&);
	const DvrAppMainControl& operator = (const DvrAppMainControl&);

	static int  LogLevel();
	static void SetLogLevel(int nLogLevel);

	HcMgrHandler 		*m_hcMgrHandler;
	DvrGuiResHandler 	*m_guiResHandler;
	APP_STATUS 			 m_status;
	static DVR_APP_MODE  m_curAppMode;

	static int  		 m_nLogLevel;

private:	
	DVR_U8 m_PhotoPlayBackImageBuffer[DVR_PHOTO_PLAYBACK_BUFFER_WIDTH * DVR_PHOTO_PLAYBACK_BUFFER_HEIGHT*3/2];

#ifdef WIN32
	HANDLE m_hThread;
	DWORD m_dwThreadID;
	static DWORD ThreadClient(LPVOID pParam);
#endif
};

#endif
