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
#include <windows.h>
#include <tchar.h>
#include <sys/stat.h>
#include "osa_fs.h"

#include "dprint.h"
#include "DvrMutexLocker.h"
#include "DvrTimer.h"
#include "DvrPlayerLoop.h"
#include "DvrPlayerPanel.h"

#ifdef _MSC_VER
#pragma warning(disable:4996)	// 4706: assignment within conditional expression
#endif

using std::string;

DvrPlayerLoop::DvrPlayerLoop(void *sysCtrl)
{
	int res = Dvr_Player_Initialize(&m_pEngine);
    if (DVR_FAILED(res)) {
        DPrint(DPRINT_ERR, "DvrPlayerLoop::DvrPlayerLoop() Dvr_Player_Initialize() failed: res = 0x%08x m_pEngine = 0x%08x\n", res, m_pEngine);
		m_pEngine = NULL;
    }

	memset(&m_ptfnCallBackArray, 0, sizeof(m_ptfnCallBackArray));
    memset(m_filename, 0, sizeof(m_filename));

	m_sysCtrl           = (DvrSystemControl *)sysCtrl;
	m_pNewFrameCallBack = NULL;
	m_pNewFrameContext  = NULL;
    m_playPanel         = new CPlayerPanel(this);
	m_timer             = new DvrTimer(DvrPlayerLoop::TimerCallback, this);
    m_speed             = 1000; // Normal playback speed is 1000
    m_playerState       = DVR_PLAYER_STATE_UNKNOWN;
    DvrSystemControl::Notifier()->Register(Notify, this);

    m_bAutoLoop     = TRUE;
    m_cachePostion  = -1;
    m_cacheDuration = -1;
}

DvrPlayerLoop::~DvrPlayerLoop()
{
    //Close_Impl();


	if(m_timer != NULL)
	{
		delete m_timer;
		m_timer = NULL;
	}
    
    if (m_pEngine) {
		Dvr_Player_DeInitialize(m_pEngine);
		m_pEngine = NULL;
    }


	DvrSystemControl::Notifier()->UnRegister(Notify, this);
    
    //if(m_playPanel != NULL)
        //m_playPanel->PlaySndMsg(DVR_PLAYER_COMMAND_THREAD_EXIT, 0, 0);


    FSDel(m_playPanel);
}

DVR_RESULT DvrPlayerLoop::Open(const char *fileName)
{
	int res;

    if (m_pEngine == NULL)
		return DVR_RES_EPOINTER;

    DPrint(DPRINT_INFO, "DvrPlayerLoop::Open() fileName = %s\n", fileName);

    Close_Impl();

	memset(&m_ptfnCallBackArray, 0, sizeof(m_ptfnCallBackArray));
	m_ptfnCallBackArray.MessageCallBack = PlayerMessageCallback;
	m_ptfnCallBackArray.RawVideoNewFrame = OnNewFrame;
	m_ptfnCallBackArray.pCallBackInstance = (void *)this;
	Dvr_Player_RegisterCallBack(m_pEngine, &m_ptfnCallBackArray);

	res = Dvr_Player_Open(m_pEngine, (void *)fileName);

    DPrint(DPRINT_INFO, "DvrPlayerLoop::Open() Dvr_Player_Open() returns %d for %s\n", res, fileName);
    
    if (DVR_FAILED(res)) {
        DPrint(DPRINT_ERR, "DvrPlayerLoop::Open() Dvr_Player_Open() failed! res = 0x%08x\n", res);

        DVR_RESULT r = DVR_RES_EFAIL;
		OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_PLAYERROR, &r, sizeof(r), NULL, 0, DvrSystemControl::Notifier());
    }
	else
	{
		DVR_RESULT r = DVR_RES_SOK;
		OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_PLAYERROR, &r, sizeof(r), NULL, 0, DvrSystemControl::Notifier());
	}

    return res;
}

DVR_RESULT DvrPlayerLoop::Close()
{
    DvrMutexLocker locker(&m_curFileLock);
    return Close_Impl();
}

DVR_RESULT DvrPlayerLoop::Close_Impl()
{
	if (m_pEngine == NULL)
		return DVR_RES_EPOINTER;
	
    Stop();

    m_cachePostion  = -1;
    m_cacheDuration = -1;

	int res = Dvr_Player_Close(m_pEngine);
    if (DVR_FAILED(res)) {
        DPrint(DPRINT_ERR, "DvrPlayerLoop::Close() Dvr_Player_Close() failed res = 0x%08x\n", res);
        return DVR_RES_EFAIL;
    }

    return DVR_RES_SOK;
}

DVR_RESULT DvrPlayerLoop::Seek(unsigned miliSec)
{
	if (m_pEngine == NULL)
		return DVR_RES_EPOINTER;

    DPrint(DPRINT_LOG, "DvrPlayerLoop::Seek() miliSec = %d\n", miliSec);

	int res = Dvr_Player_SetPosition(m_pEngine, miliSec);
    if (DVR_FAILED(res)) {
        DPrint(DPRINT_ERR, "DvrPlayerLoop::Set() Dvr_Player_SetPosition() failed! res = 0x%08x\n", res);
        return DVR_RES_EFAIL;
    }

    return DVR_RES_SOK;
}

DVR_RESULT DvrPlayerLoop::FastForward(int speed)
{
	if (m_pEngine == NULL)
		return DVR_RES_EPOINTER;

    if (speed == 1000) {        // 1x speed 
		Stop();
		DVR_RESULT res = Dvr_Player_SetConfig(m_pEngine, DVR_PLAYER_CFG_FAST_FORWARD_SPEED, &speed, sizeof(speed));
		if (DVR_FAILED(res)) {
			DPrint(DPRINT_ERR, "DvrPlayerLoop::FastForward() Dvr_Player_SetConfig() with DVR_PLAYER_CFG_FAST_FORWARD_SPEED failed! res = 0x%08x\n", res);
			return DVR_RES_EFAIL;
		}
		
		res = Play();
        if (DVR_FAILED(res)) {
            DPrint(DPRINT_ERR, "DvrPlayerLoop::FastForward() DvrPlayerLoop::Play() failed! res = 0x%08x\n", res);
            return DVR_RES_EFAIL;
        }
    } else {
        int s = abs(speed);
		int res = Dvr_Player_SetConfig(m_pEngine, DVR_PLAYER_CFG_FAST_FORWARD_SPEED, &s, sizeof(s));
        if (DVR_FAILED(res)) {
            DPrint(DPRINT_ERR, "DvrPlayerLoop::FastForward() Dvr_Player_SetConfig() with DVR_PLAYER_CFG_FAST_FORWARD_SPEED failed! res = 0x%08x\n", res);
            return DVR_RES_EFAIL;
        }

        int direction = 1;
        if (speed < 0) {
            direction = -1;
        }

		res = Dvr_Player_FastScan(m_pEngine, direction);
        if (DVR_FAILED(res)) {
            DPrint(DPRINT_ERR, "DvrPlayerLoop::FastForward() Dvr_Player_FastScan() failed! res = 0x%08x\n", res);
            return DVR_RES_EFAIL;
        }

// 		struct timeval tv = { 0, 500000 };
// 		m_timer->Start(tv);
    }

    NotifyPlayerState();

    return DVR_RES_SOK;
}

unsigned DvrPlayerLoop::Position()
{
	if (m_pEngine == NULL)
        return 0;

    if (PlayerState() != DVR_PLAYER_STATE_PLAY && m_cachePostion != -1) {
        return m_cachePostion;
    }

    unsigned int pos = 0;
	Dvr_Player_GetPosition(m_pEngine, &pos);
    pos /= 1000;

    unsigned duration = Duration();
    if (pos > duration)
        pos = duration;

	if(duration == 1)
		pos = 1;

    return pos;
}

unsigned DvrPlayerLoop::Duration()
{
	if (m_pEngine == NULL)
		return DVR_RES_EPOINTER;

    if (PlayerState() != DVR_PLAYER_STATE_PLAY && m_cacheDuration != -1) {
        return m_cacheDuration;
    }

	DVR_MEDIA_INFO mediaInfo;
    memset(&mediaInfo, 0, sizeof(mediaInfo));
	Dvr_Player_GetConfig(m_pEngine, DVR_PLAYER_CFG_MEDIA_INFO, &mediaInfo, sizeof(mediaInfo));

    unsigned duration = mediaInfo.u32Duration / 1000;

	if(duration == 0)
		duration = 1;

    return duration;
}

DVR_RESULT DvrPlayerLoop::Play()
{
	if (m_pEngine == NULL)
		return DVR_RES_EPOINTER;

    DPrint(DPRINT_LOG, "DvrPlayerLoop::Play() About to call Dvr_Player_Play()\n");

	int res = Dvr_Player_Play(m_pEngine);
    if (DVR_FAILED(res)) {
        DPrint(DPRINT_ERR, "DvrPlayerLoop::Play() Dvr_Player_Play() failed res = 0x%08x\n", res);
        return DVR_RES_EFAIL;
    }

    struct timeval tv = {0, 500000};
	if(m_timer != NULL)
        m_timer->Start(tv);

    NotifyPlayerState();

    return DVR_RES_SOK;
}

DVR_RESULT DvrPlayerLoop::Stop()
{
	if (m_pEngine == NULL)
		return DVR_RES_EPOINTER;

    if(m_timer != NULL)
        m_timer->Stop();

//     m_cachePostion = Position();
//     m_cacheDuration = Duration();

    DPrint(DPRINT_ERR, "DvrPlayerLoop::Stop() calls Dvr_Player_Stop()\n");
	int res = Dvr_Player_Stop(m_pEngine);
    DPrint(DPRINT_ERR, "DvrPlayerLoop::Stop() Dvr_Player_Stop() returns %d\n", res);
    if (DVR_FAILED(res)) {
        DPrint(DPRINT_ERR, "DvrPlayerLoop::Stop() Dvr_Player_Stop() failed res = 0x%08x\n", res);
        return DVR_RES_EFAIL;
    }

    NotifyPlayerState();

    return DVR_RES_SOK;
}

DVR_RESULT DvrPlayerLoop::Pause()
{
	if (m_pEngine == NULL)
		return DVR_RES_EPOINTER;

	int res = Dvr_Player_Pause(m_pEngine);
    if (DVR_FAILED(res)) {
        DPrint(DPRINT_ERR, "DvrPlayerLoop::Pause() Dvr_Player_Pause() failed res = 0x%08x\n", res);
        return DVR_RES_EFAIL;
    }

    NotifyPlayerState();

    return DVR_RES_SOK;
}

DVR_RESULT DvrPlayerLoop::Resume()
{
	if (m_pEngine == NULL)
		return DVR_RES_EPOINTER;

    DPrint(DPRINT_LOG, "DvrPlayerLoop::Resume() about to call Dvr_Player_Resume()\n");
    int res = Dvr_Player_Resume(m_pEngine);
    if (DVR_FAILED(res)) {
        DPrint(DPRINT_ERR, "DvrPlayerLoop::Resume() Dvr_Player_Resume() failed res = 0x%08x\n", res);
        return DVR_RES_EFAIL;
    }

    NotifyPlayerState();

    return DVR_RES_SOK;
}

DVR_RESULT DvrPlayerLoop::Set(DVR_PLAYER_PROP type, void *configData, int size)
{
    switch (type) {
	case DVR_PLAYER_PROP_POSITION:
        {
            if (size != sizeof(unsigned))
                return DVR_RES_EINVALIDARG;

            unsigned pos = *(unsigned *)configData;
            DVR_RESULT res = Seek(pos);
            if (DVR_FAILED(res)) {
                DPrint(DPRINT_ERR, "DvrPlayerLoop::Set() with DVR_PLAYER_PROP_POSITION DvrPlayerLoop::Seek() failed! res = 0x%08x\n", res);
                return DVR_RES_EFAIL;
            }
        }
        break;
	case DVR_PLAYER_PROP_SPEED:
        {
            if (size != sizeof(int))
				return DVR_RES_EINVALIDARG;
            m_speed = *(int *)configData;
			DVR_RESULT res = FastForward(m_speed);
			if (DVR_FAILED(res)) {
                DPrint(DPRINT_ERR, "DvrPlayerLoop::Set() with DVR_PLAYER_PROP_SPEED DvrPlayerLoop::FastForward() failed! res = 0x%08x\n", res);
                return DVR_RES_EFAIL;
            }
        }
        break;
	case DVR_PLAYER_PROP_ROOT_DIRECTORY:
		{
			DVR_RESULT res = Dvr_Player_SetConfig(m_pEngine, DVR_PLAYER_CFG_ROOT_DIRECTORY, configData, size);
			if (DVR_FAILED(res)) {
				DPrint(DPRINT_ERR, "DvrPlayerLoop::Set() Dvr_Player_SetConfig() with DVR_PLAYER_CFG_ROOT_DIRECTORY failed! res = 0x%08x\n", res);
				return DVR_RES_EFAIL;
			}
		}
		break;
    default:
        break;
    }

    return DVR_RES_SOK;
}

DVR_RESULT DvrPlayerLoop::Frame_Update(DVR_IO_FRAME *pInputFrame)
{
	if (m_pEngine == NULL)
		return DVR_RES_EPOINTER;

	DVR_RESULT res = Dvr_Player_Frame_Update(m_pEngine, pInputFrame);

	return res;
}

DVR_RESULT DvrPlayerLoop::Get(DVR_PLAYER_PROP type, void *configData, int size)
{
    if (m_pEngine == NULL)
        return DVR_RES_EPOINTER;

    switch (type) {
	case DVR_PLAYER_PROP_POSITION:
	{
		if (size != sizeof(unsigned))
			return DVR_RES_EINVALIDARG;

		if (PlayerState() != DVR_PLAYER_STATE_PLAY && m_cachePostion != -1) {
			*(unsigned *)configData = m_cachePostion;
			DPrint(DPRINT_LOG, "DvrPlayerLoop::Get() retrieve the cached player position m_cachePostion = %d!\n", m_cachePostion);
		}
		else {
			int res = Dvr_Player_GetPosition(m_pEngine, (unsigned int *)configData);
			if (DVR_FAILED(res)) {
				DPrint(DPRINT_ERR, "DvrPlayerLoop::Get() Dvr_Player_GetPosition() failed! res = 0x%08x\n", res);
				return DVR_RES_EFAIL;
			}
		}
	}
	break;

	case DVR_PLAYER_PROP_SPEED:
	{
		if (size != sizeof(int))
			return DVR_RES_EINVALIDARG;

		if (PlayerState() == DVR_PLAYER_STATE_PLAY) {
			*(int *)configData = 1000;
			return DVR_RES_SOK;
		}

		int res = Dvr_Player_GetConfig(m_pEngine, DVR_PLAYER_CFG_FAST_FORWARD_SPEED, configData, size);
		if (DVR_FAILED(res)) {
			DPrint(DPRINT_ERR, "DvrPlayerLoop::Get() Dvr_Player_GetConfig() with DVR_PLAYER_CFG_FAST_FORWARD_SPEED failed! res = 0x%08x\n", res);
			return DVR_RES_EFAIL;
		}

		int direct = 1;
		Dvr_Player_GetConfig(m_pEngine, DVR_PLAYER_CFG_FAST_SCAN_DIRECTION, &direct, sizeof(direct));

		if (direct < 0)
			*(int *)configData = -*(int *)configData;
	}
	break;

	case DVR_PLAYER_PROP_DURATION:
	{
		if (size != sizeof(int))
			return DVR_RES_EINVALIDARG;

		if (PlayerState() != DVR_PLAYER_STATE_PLAY && m_cacheDuration != -1) {
			*(unsigned *)configData = m_cacheDuration;
			DPrint(DPRINT_LOG, "DvrPlayerLoop::Get() retrieve the cached duration. m_cacheDuration = %d\n", m_cacheDuration);
		}
		else {
			DVR_MEDIA_INFO mediaInfo;
			memset(&mediaInfo, 0, sizeof(mediaInfo));
			int res = Dvr_Player_GetConfig(m_pEngine, DVR_PLAYER_CFG_MEDIA_INFO, &mediaInfo, sizeof(mediaInfo));
			if (DVR_FAILED(res)) {
				DPrint(DPRINT_ERR, "DvrPlayerLoop::Get() Dvr_Player_GetConfig() with DVR_PLAYER_CFG_MEDIA_INFO failed! res = 0x%08x\n", res);
				return DVR_RES_EFAIL;
			}
			*(unsigned *)configData = mediaInfo.u32Duration;
		}
	}
	break;

	case DVR_PLAYER_PROP_CURRENT_FILE:
	{
		if (configData == NULL)
			return DVR_RES_EINVALIDARG;

		memset(configData, 0, size);

		char szPath[PATH_MAX];
		memset(szPath, 0, PATH_MAX);
		int nIndex = 0;

		DVR_RESULT res = Loop_GetCurrentIndex(&nIndex, szPath, PATH_MAX);
		if (DVR_SUCCEEDED(res))
		{
			if (size < (strlen(szPath) + 1))
			{
				return DVR_RES_EINVALIDARG;
			}
			memcpy(configData, szPath, strlen(szPath));
		}
	}
	break;

    default:
        break;
    }

    return DVR_RES_SOK;
}

DVR_RESULT DvrPlayerLoop::Loop_Setup(const char *pszFullName, DVR_FILE_TYPE eFileType, DVR_FOLDER_TYPE eFolderType)
{
    DvrMutexLocker locker(&m_curFileLock);
    if(m_pEngine == NULL || m_playPanel == NULL)
        return DVR_RES_EPOINTER;

    DVR_RESULT res = DVR_RES_EFAIL;
    if(eFileType == DVR_FILE_TYPE_IMAGE)
    {
        m_playPanel->PlaySetType(eFolderType);
        res = m_playPanel->PlayPicStart((char*)pszFullName);
    }
    else if(eFileType == DVR_FILE_TYPE_VIDEO)
    {
        m_playPanel->PlaySetSpeed(m_speed);
        m_playPanel->PlaySetType(eFolderType);
        res = m_playPanel->PlayStart((char*)pszFullName);
    }
    return res;
}

DVR_RESULT DvrPlayerLoop::Loop_GetCurrentIndex(int *index, char *name, int length)
{
    if(m_playPanel == NULL)
        return DVR_RES_EPOINTER;
    DPrint(DPRINT_LOG, "DvrPlayerLoop::Loop_GetCurrentIndex() index = 0x%08x name = 0x%08x length = %d\n", index, name, length);
    return m_playPanel->PlayGetFile(name, length);
}

DVR_RESULT DvrPlayerLoop::Loop_Prev()
{
    if(m_playPanel == NULL)
		return DVR_RES_EPOINTER;
    return m_playPanel->PlaySndMsg(DVR_PLAYER_COMMAND_PREV, 0, 0);
}

DVR_RESULT DvrPlayerLoop::Loop_Next()
{
    if(m_playPanel == NULL)
        return DVR_RES_EPOINTER;
    return m_playPanel->PlaySndMsg(DVR_PLAYER_COMMAND_NEXT, 0, 0);
}

DVR_RESULT DvrPlayerLoop::Loop_StepF()
{
    if(m_playPanel == NULL)
        return DVR_RES_EPOINTER;
    return m_playPanel->PlaySndMsg(DVR_PLAYER_COMMAND_STEPF, 0, 0);
}

DVR_RESULT DvrPlayerLoop::Loop_StepB()
{
    if(m_playPanel == NULL)
        return DVR_RES_EPOINTER;
    return m_playPanel->PlaySndMsg(DVR_PLAYER_COMMAND_STEPB, 0, 0);
}

DVR_RESULT DvrPlayerLoop::Loop_Delta(int nDelta)
{
    if(m_playPanel == NULL)
        return DVR_RES_EPOINTER;
    return m_playPanel->PlaySndMsg(DVR_PLAYER_COMMAND_FSPEC, nDelta, 0);
}

DVR_PLAYER_STATE DvrPlayerLoop::PlayerState()
{
    if (m_pEngine == NULL)
        return DVR_PLAYER_STATE_UNKNOWN;

    PLAYER_STATUS status;
	int res = Dvr_Player_GetConfig(m_pEngine, DVR_PLAYER_CFG_PLAYER_STATUS, &status, sizeof(status));
    if (DVR_FAILED(res)) {
        DPrint(DPRINT_ERR, "DvrPlayerLoop::PlayerState() Dvr_Player_GetConfig() with DVR_PLAYER_CFG_PLAYER_STATUS failed! res = 0x%08x\n", res);
        return DVR_PLAYER_STATE_UNKNOWN;
    }

    DVR_PLAYER_STATE state = DVR_PLAYER_STATE_UNKNOWN;
    switch (status)
    {
	case PLAYER_STATUS_INVALID:
        state = DVR_PLAYER_STATE_CLOSE;
        break;
	case PLAYER_STATUS_STOPPED:
		state = DVR_PLAYER_STATE_STOP;
        break;
	case PLAYER_STATUS_PAUSED:
		state = DVR_PLAYER_STATE_PAUSE;
        break;
	case PLAYER_STATUS_RUNNING:
        state = DVR_PLAYER_STATE_PLAY;
        break;
	case PLAYER_STATUS_FASTPLAY:
		state = DVR_PLAYER_STATE_FASTFORWARD;
        break;
	case PLAYER_STATUS_FASTSCAN:
        int direct;
		Dvr_Player_GetConfig(m_pEngine, DVR_PLAYER_CFG_FAST_SCAN_DIRECTION, &direct, sizeof(direct));
        if (direct > 0)
            state = DVR_PLAYER_STATE_FASTFORWARD;
        else
            state = DVR_PLAYER_STATE_FASTBACKWARD;
        break;
    default:
        break;
    }

    return state;
}

void DvrPlayerLoop::NotifyPlayerState()
{
	DVR_PLAYER_STATE state = PlayerState();
	if (state != m_playerState) {
		OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_PLAYERSTATE, &state, sizeof(state), NULL, 0, DvrSystemControl::Notifier());
		m_playerState = state;
	}
}

void *DvrPlayerLoop::TimerCallback(void *arg)
{
    DvrPlayerLoop *p = (DvrPlayerLoop *)arg;

    unsigned pos      = p->Position();
    unsigned duration = p->Duration();

	OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_POSITION, &pos, sizeof(pos), &duration, sizeof(duration), DvrSystemControl::Notifier());

    return (void *)0;
}

DVR_RESULT DvrPlayerLoop::RegisterCallBack(PFN_PLAYER_NEWFRAME callback, void *pContext)
{
	m_pNewFrameCallBack = callback;
	m_pNewFrameContext = pContext;

	return DVR_RES_SOK;
}

int DvrPlayerLoop::OnNewFrame(void *pvParam1, void *pvParam2, void *pvParam3, void *pvContext)
{
	DvrPlayerLoop *p = (DvrPlayerLoop *)pvContext;
	DVR_IO_FRAME *frame = (DVR_IO_FRAME *)pvParam1;

	if (p->m_pNewFrameCallBack != NULL)
		p->m_pNewFrameCallBack(p->m_pNewFrameContext, frame);

	return DVR_RES_SOK;
}

int DvrPlayerLoop::PlayerMessageCallback(void *pvParam1, void *pvParam2, void *pvParam3, void *pvContext)
{
    DvrPlayerLoop *p      = (DvrPlayerLoop *)pvContext;
	PLAYER_EVENT enuEventCode = *(PLAYER_EVENT *)pvParam1;

    switch (enuEventCode)
    {
    case PLAYER_EVENT_PLAYBACK_END:
        {
            if (!p->m_bAutoLoop) 
            {
                DVR_RANGE_EVENT enuEvent = DVR_RANGE_EVENT_EOF;
                DPrint(DPRINT_LOG, "DvrPlayerLoop::PlayerMessageCallback() About to post the EOF message speed = %d\n", p->m_speed);
				OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_RANGEEVENT, &enuEvent, sizeof(enuEvent), &p->m_speed, sizeof(p->m_speed), DvrSystemControl::Notifier());
                DPrint(DPRINT_LOG, "DvrPlayerLoop::PlayerMessageCallback() The EOF message posted\n");

				unsigned duration = p->Duration();
				OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_POSITION, &duration, sizeof(duration), &duration, sizeof(duration), DvrSystemControl::Notifier());

				#if 0
				//special work for some file which end abnormally
				unsigned pos	  = p->Position();
				unsigned duration = p->Duration();
                if (p->m_playPanel && ((pos < duration) || duration == 0)) 
                {
                    DPrint(DPRINT_ERR, "Abnormal file[pos:%d, duration:%d], switch to next file", pos, duration);
                    p->m_playPanel->PlaySndMsg(DVR_PLAYER_COMMAND_NEXT, 0, 0);
                }
				#endif				
            }
            else 
            {
                if (p->m_playPanel)
                    p->m_playPanel->PlaySndMsg(DVR_PLAYER_COMMAND_NEXT, 0, 0);
            }
        }
        break;
    case PLAYER_EVENT_CREATE_COMPONENT_FAIL:
        {
            DVR_RESULT res = DVR_RES_SOK;
			PLAYER_COMPONENT_TYPE comType = *(PLAYER_COMPONENT_TYPE *)pvParam2;
            switch (comType) {
            case PLAYER_COMPONENT_DEMUX:
                res = DVR_RES_EDEMUX;
                break;
			case PLAYER_COMPONENT_VIDEO_DECODER:
                res = DVR_RES_EVIDEO_DECODER;
                break;
			case PLAYER_COMPONENT_VIDEO_RENDERER:
                res = DVR_RES_EVIDEO_RENDER;
                break;
			case PLAYER_COMPONENT_AUDIO_DECODER:
                res = DVR_RES_EAUDIO_DECODER;
                break;
			case PLAYER_COMPONENT_AUDIO_RENDERER:
                res = DVR_RES_EAUDIO_RENDER;
                break;
            }

            if (DVR_FAILED(res)) {
				OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_PLAYERROR, &res, sizeof(res), NULL, 0, DvrSystemControl::Notifier());
            }
        }
        break;

	case PLAYER_EVENT_PRINT_SCREEN_ADD2DB:
	{
		DVR_ADDTODB_FILE_INFO *ptFileInfo = (DVR_ADDTODB_FILE_INFO *)pvParam2;
		
		DPrint(DPRINT_LOG, "DvrPlayerLoop::PlayerMessageCallback() Finished print screen file [%s], thumbnail file[%s] at %ld\n", 
            ptFileInfo->file_location, ptFileInfo->thumbnail_location, ptFileInfo->time_stamp);
		DVR_RECORDER_FILE_OP file_op;

		file_op.eEvent = DVR_FILE_EVENT_CREATE;
        if (ptFileInfo->file_location != NULL && ptFileInfo->thumbnail_location != NULL)
        {
            strcpy(file_op.szLocation, ptFileInfo->file_location);
            strcpy(file_op.szTNLocation, ptFileInfo->thumbnail_location);
            file_op.nTimeStamp = ptFileInfo->time_stamp;
            file_op.eType = ptFileInfo->eType;

            OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_FILEEVENT, &file_op, sizeof(file_op), NULL, 0, DvrSystemControl::Notifier());
        }
	}
	break;

	case PLAYER_EVENT_PRINT_SCREEN_DONE:
	{
		OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_PHOTO_DONE, NULL, 0, NULL, 0, DvrSystemControl::Notifier());
	}
	break;
    }

    return DVR_RES_SOK;
}

int DvrPlayerLoop::Notify(void *pContext, DVR_NOTIFICATION_TYPE enuType, void *pParam1, void *pParam2)
{
    DvrPlayerLoop *p = (DvrPlayerLoop *)pContext;

    switch (enuType) {
		default:
			break;
    }

    return 0;
}

DVR_RESULT DvrPlayerLoop::PhotoOpen(const char *fileName)
{
    int res;

    if (m_pEngine == NULL)
        return DVR_RES_EPOINTER;

    DPrint(DPRINT_LOG, "DvrPlayerLoop::PhotoOpen() %s\n", fileName);

    PhotoClose();

    memset(&m_ptfnCallBackArray, 0, sizeof(m_ptfnCallBackArray));
    m_ptfnCallBackArray.MessageCallBack = PlayerMessageCallback;
    m_ptfnCallBackArray.RawVideoNewFrame = OnNewFrame;
    m_ptfnCallBackArray.pCallBackInstance = (void *)this;
    Dvr_Player_RegisterCallBack(m_pEngine, &m_ptfnCallBackArray);

    res = Dvr_Photo_Player_Open(m_pEngine, fileName);
	
    DPrint(DPRINT_INFO, "DvrPlayerLoop::Open() Dvr_Photo_Player_Open() returns %d for %s\n", res, fileName);
	
    if (DVR_FAILED(res)) {
        DPrint(DPRINT_ERR, "DvrPlayerLoop::PhotoOpen() Dvr_Photo_Player_Open() failed! res = 0x%08x\n", res);

        DVR_RESULT r = DVR_RES_EFAIL;
        OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_PLAYERROR, &r, sizeof(r), NULL, 0, DvrSystemControl::Notifier());
    }
	else
	{
		DVR_RESULT r = DVR_RES_SOK;
		OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_PLAYERROR, &r, sizeof(r), NULL, 0, DvrSystemControl::Notifier());
	}

    return res;
}

DVR_RESULT DvrPlayerLoop::PhotoClose()
{
    if (m_pEngine == NULL)
        return DVR_RES_EPOINTER;

    int res = Dvr_Photo_Player_Close(m_pEngine);
    if (DVR_FAILED(res)) {
        DPrint(DPRINT_ERR, "DvrPlayerLoop::PhotoClose() Dvr_Photo_Player_Close() failed res = 0x%08x\n", res);
    }

    return res;
}

DVR_RESULT DvrPlayerLoop::PhotoPlay()
{
    if (m_pEngine == NULL)
        return DVR_RES_EPOINTER;

    int res = Dvr_Photo_Player_Play(m_pEngine);
    if (DVR_FAILED(res)) {
        DPrint(DPRINT_ERR, "DvrPlayerLoop::PhotoPlay() Dvr_Photo_Player_Play() failed res = 0x%08x\n", res);
    }

    return res;
}

DVR_RESULT DvrPlayerLoop::PhotoStop()
{
    if (m_pEngine == NULL)
        return DVR_RES_EPOINTER;

    int res = Dvr_Photo_Player_Stop(m_pEngine);
    if (DVR_FAILED(res)) {
        DPrint(DPRINT_ERR, "DvrPlayerLoop::PhotoStop() Dvr_Photo_Player_Stop() failed res = 0x%08x\n", res);
    }

    return res;
}

DVR_RESULT DvrPlayerLoop::PrintScreen(DVR_PHOTO_PARAM *pParam)
{
	int res;

	if (m_pEngine == NULL)
		return DVR_RES_EPOINTER;

	DPrint(DPRINT_INFO, "Trigger Print Screen\n");
	
	res = Dvr_Player_PrintScreen(m_pEngine, pParam);
	if (DVR_FAILED(res)) {
		DPrint(DPRINT_ERR, "DvrPlayerLoop::Dvr_Player_PrintScreen() failed res = 0x%08x\n", res);
		return DVR_RES_EFAIL;
	}

	return DVR_RES_SOK;
}

