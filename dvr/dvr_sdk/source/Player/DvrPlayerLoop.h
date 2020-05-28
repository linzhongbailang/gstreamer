/*===========================================================================*\
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
\*===========================================================================*/

#ifndef _DVRPLAYERLOOP_H_
#define _DVRPLAYERLOOP_H_

#include <windows.h>
#include "DVR_PLAYER_INTFS.h"
#include "DVR_METADATA_INTFS.h"
#include "DvrSystemControl.h"
#include "DvrMutex.h"

class DvrTimer;
class CPlayerPanel;
class DvrPlayerLoop
{
public:
	DvrPlayerLoop(void *sysCtrl);
	~DvrPlayerLoop();

    DVR_RESULT Open(const char *fileName);
    DVR_RESULT Close();
    DVR_RESULT Seek(unsigned miliSec);
    DVR_RESULT FastForward(int speed);
    unsigned Position();
    unsigned Duration();

    DVR_RESULT PhotoOpen(const char *fileName);
    DVR_RESULT PhotoClose();
    DVR_RESULT PhotoPlay();
    DVR_RESULT PhotoStop();
	DVR_RESULT PrintScreen(DVR_PHOTO_PARAM *pParam);

	DVR_RESULT Play();
	DVR_RESULT Stop();
	DVR_RESULT Pause();
	DVR_RESULT Resume();
	DVR_RESULT Set(DVR_PLAYER_PROP type, void *configData, int size);
	DVR_RESULT Get(DVR_PLAYER_PROP type, void *configData, int size);
	DVR_RESULT RegisterCallBack(PFN_PLAYER_NEWFRAME callback, void *pContext);
	DVR_RESULT Frame_Update(DVR_IO_FRAME *pInputFrame);

	DVR_RESULT Loop_Setup(const char *pszFullName, DVR_FILE_TYPE eFileType, DVR_FOLDER_TYPE eFolderType);
	DVR_RESULT Loop_GetCurrentIndex(int *index, char *name, int length);
	DVR_RESULT Loop_Prev();
	DVR_RESULT Loop_Next();
	DVR_RESULT Loop_StepF();
	DVR_RESULT Loop_StepB();
	DVR_RESULT Loop_Delta(int nDelta);	

    void SetAutoLoop(bool bAutoLoop)
    {
        m_bAutoLoop = bAutoLoop;
    }

	void *GetSysCtrlHandle(void)
	{
		return m_sysCtrl;
	}

private:
    DVR_RESULT 	Close_Impl();
    static void *TimerCallback(void *arg);
    static int  PlayerMessageCallback(void *pvParam1, void *pvParam2, void *pvParam3, void *pvContext);
	static int  OnNewFrame(void *pvParam1, void *pvParam2, void *pvParam3, void *pvContext);
    static int  Notify(void *pContext, DVR_NOTIFICATION_TYPE enuType, void *pParam1, void *pParam2);

    DVR_PLAYER_STATE 	PlayerState();
    void               	NotifyPlayerState();

    HANDLE                     m_pEngine;
	DVR_PLAYER_CALLBACK_ARRAY  m_ptfnCallBackArray;
    DvrTimer *                 m_timer;
    int                        m_speed;
    DVR_PLAYER_STATE           m_playerState;
    char                       m_filename[PATH_MAX];
    DvrMutex                   m_curFileLock;
	DvrSystemControl *		   m_sysCtrl;
	CPlayerPanel*			   m_playPanel;
    int                        m_cachePostion;
    int                        m_cacheDuration;
	PFN_PLAYER_NEWFRAME		   m_pNewFrameCallBack;
	void					  *m_pNewFrameContext;
    bool                       m_bAutoLoop;// auto loop related
};

#endif
