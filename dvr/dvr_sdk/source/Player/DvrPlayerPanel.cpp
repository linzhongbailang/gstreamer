#include "osa.h"
#include "dprint.h"
#include "DvrMutex.h"
#include "DvrMutexLocker.h"
#include "DvrPlayerPanel.h"
#include "DvrSystemControl.h"
#include "DvrPlayerLoop.h"

CPlayerPanel::CPlayerPanel(void *loop)
{
    m_player    = (DvrPlayerLoop *)loop;
	m_mutex     = new DvrMutex();
    m_curtype   = DVR_FOLDER_TYPE_NORMAL;
    m_playmode  = PLAY_MODE_NORMAL;
    memset(&m_curItem, 0, sizeof(DVR_FILEMAP_META_ITEM));
    strcpy(m_msgQueue.name,"CPlayerPanel");
	DVR_S32 res = DVR::OSA_msgqCreate(&m_msgQueue, MsgPool, sizeof(APP_MESSAGE), PLAY_MGR_MSGQUEUE_SIZE);
	if (DVR_FAILED(res))
		DPrint(DPRINT_ERR, "CPlayerPanel <Init> Create Queue failed = 0x%x\n", res);
	res = DVR::OSA_tskCreate(&m_hThread, ThreadProc, this);
	if (DVR_FAILED(res))
		DPrint(DPRINT_ERR, "CPlayerPanel <Init> Create Task failed = 0x%x\n", res);
}

CPlayerPanel::~CPlayerPanel(void)
{
    FSDel(m_mutex);
	DVR::OSA_msgqDelete(&m_msgQueue);
	DVR::OSA_tskDelete(&m_hThread);
}

int CPlayerPanel::PlayStart(char *szParam)
{
	DvrMutexLocker mutexLocker(m_mutex);
    return OnStart(szParam);
}

int CPlayerPanel::PlayStop(void)
{
	DvrMutexLocker mutexLocker(m_mutex);
    return OnStop();
}

int CPlayerPanel::PlayClean(void)
{
	DvrMutexLocker mutexLocker(m_mutex);
    m_curtype   = DVR_FOLDER_TYPE_NORMAL;
    m_playmode  = PLAY_MODE_NORMAL;
    m_speed     = 0;
    memset(&m_curItem, 0, sizeof(DVR_FILEMAP_META_ITEM));
    return DVR_RES_SOK;
}

int CPlayerPanel::PlayPause(void)
{
	DvrMutexLocker mutexLocker(m_mutex);
    if (m_player == NULL)
        return DVR_RES_EFAIL;
    
    DPrint(DPRINT_INFO, "CPlayerPanel::PlayPause()\n");
    return  m_player->Pause();
}

int CPlayerPanel::PlayResume(void)
{
	DvrMutexLocker mutexLocker(m_mutex);
    if (m_player == NULL)
        return DVR_RES_EFAIL;
    
    DPrint(DPRINT_INFO, "CPlayerPanel::PlayResume()\n");
    return m_player->Resume();
}

int CPlayerPanel::PlaySetType(int type)
{
	DvrMutexLocker mutexLocker(m_mutex);
    if (m_player == NULL)
        return DVR_RES_EFAIL;
    
    DPrint(DPRINT_INFO, "CPlayerPanel::PlaySetType() %d\n",type);
    if (type == DVR_FOLDER_TYPE_NORMAL)
        m_playmode = PLAY_MODE_NORMAL;
    else if (type == DVR_FOLDER_TYPE_EMERGENCY)
        m_playmode = PLAY_MODE_EVENT;
    else if (type == DVR_FOLDER_TYPE_PHOTO)
        m_playmode = PLAY_MODE_PHOTO;
    else if (type == DVR_FOLDER_TYPE_DAS)
        m_playmode = PLAY_MODE_DAS;
    
    m_curtype = (DVR_FOLDER_TYPE)type;
    return DVR_RES_SOK;
}

int CPlayerPanel::PlaySetPosition(unsigned pos)
{
    if (m_player == NULL)
        return DVR_RES_EFAIL;

    DPrint(DPRINT_INFO, "CPlayerPanel::PlaySetPosition() %d\n",pos);
    return m_player->Set(DVR_PLAYER_PROP_POSITION, &pos, sizeof(pos));
}

int CPlayerPanel::PlaySetSpeed(int speed)
{
	DvrMutexLocker mutexLocker(m_mutex);
    if (m_player == NULL)
        return DVR_RES_EFAIL;

    m_speed = speed;
    DPrint(DPRINT_INFO, "CPlayerPanel::PlaySetSpeed() %d\n",speed);
    return m_player->Set(DVR_PLAYER_PROP_SPEED, &speed, sizeof(speed));
}

int CPlayerPanel::PlayGetFile(char *filename, int length)
{  
	DvrMutexLocker mutexLocker(m_mutex);
    if (filename == NULL || m_player == NULL)
        return DVR_RES_EFAIL;

    if(length < sizeof(m_curItem.szMediaFileName))
        return DVR_RES_EFAIL;

    strncpy(filename,m_curItem.szMediaFileName,sizeof(m_curItem.szMediaFileName));
    return DVR_RES_SOK;
}

int CPlayerPanel::PlayNextFile(void)
{
	DvrMutexLocker mutexLocker(m_mutex);
    return OnNextFile();
}

int CPlayerPanel::PlayPrevFile(void)
{
	DvrMutexLocker mutexLocker(m_mutex);
    return OnPrevFile();
}

int CPlayerPanel::PlaySpecFile(unsigned pos)
{
	DvrMutexLocker mutexLocker(m_mutex);
    if (m_player == NULL)
        return DVR_RES_EFAIL;

    DvrSystemControl *sysctrl = (DvrSystemControl*)m_player->GetSysCtrlHandle();
    if(sysctrl == NULL)
        return DVR_RES_EFAIL;

	DVR_FILEMAP_META_ITEM 	item;
    memset(&item, 0, sizeof(DVR_FILEMAP_META_ITEM));
    DVR_RESULT res = sysctrl->FileMapDB_GetNameByRelPos(pos, m_curtype, &item, sizeof(DVR_FILEMAP_META_ITEM));
    DPrint(DPRINT_INFO, "CPlayerPanel::PlaySpecFile(pos %d)  %s\n",pos,item.szMediaFileName);
    if(strlen(item.szMediaFileName) == 0)
        sysctrl->FileMapDB_GetNameByRelPos(0, m_curtype, &item, sizeof(DVR_FILEMAP_META_ITEM));
    if(strcmp(m_curItem.szMediaFileName, item.szMediaFileName) == 0)
        return DVR_RES_SOK;

    if(m_playmode == PLAY_MODE_PHOTO)
        res = OnPicStart(item.szMediaFileName);
    else
        res = OnStart(item.szMediaFileName);
    if(res != DVR_RES_SOK && strlen(item.szMediaFileName) == 0)
        return res;

    memcpy(&m_curItem, &item, sizeof(DVR_FILEMAP_META_ITEM));
    return res;
}

int CPlayerPanel::PlayStepF(void)
{
    DvrMutexLocker mutexLocker(m_mutex);
    if (m_player == NULL)
        return DVR_RES_EFAIL;
    uint pos = 0;
    uint duration = m_player->Duration();
    if(duration == DVR_RES_EPOINTER)
        return DVR_RES_EFAIL;
    m_player->Get(DVR_PLAYER_PROP_POSITION, &pos, sizeof(uint));
    DPrint(DPRINT_INFO, "CPlayerPanel::PlayStepF pos=======%d duration %d\n",pos,duration*1000);
    pos += 9000;
    if(pos <= duration * 1000)
        return PlaySetPosition(pos);
    else
    {
        DvrSystemControl *sysctrl = (DvrSystemControl*)m_player->GetSysCtrlHandle();
        if(sysctrl == NULL)
            return DVR_RES_EFAIL;
        return sysctrl->ComSvcHcmgr_SendMsg(HMSG_AUTO_PLAY_NEXT, 0, 0);
    }

    return DVR_RES_SOK;
}

int CPlayerPanel::PlayStepB(void)
{
    DvrMutexLocker mutexLocker(m_mutex);
    if (m_player == NULL)
        return DVR_RES_EFAIL;
    uint pos = 0;
    uint duration = m_player->Duration();
    if(duration == DVR_RES_EPOINTER)
        return DVR_RES_EFAIL;
    m_player->Get(DVR_PLAYER_PROP_POSITION, &pos, sizeof(uint));
    DPrint(DPRINT_INFO, "CPlayerPanel::PlayStepB pos=======%d duration %d\n",pos,duration*1000);
    int newpos = pos - 9000;
    if(newpos >= 0)
        return PlaySetPosition(newpos);
    else
        return PlaySetPosition(0);

    return DVR_RES_SOK;
}

int CPlayerPanel::PlayPicStart(char *szParam)
{
	DvrMutexLocker mutexLocker(m_mutex);
    return OnPicStart(szParam);
}

int CPlayerPanel::PlayPicStop(void)
{
	DvrMutexLocker mutexLocker(m_mutex);
    if (m_player == NULL)
        return DVR_RES_EFAIL;

    DPrint(DPRINT_INFO, "CPlayerPanel::PlayPicStop()\n");
    return m_player->PhotoStop();
}

int CPlayerPanel::OnPicStart(char *szParam)
{
    if (m_player == NULL || szParam == NULL)
        return DVR_RES_EFAIL;

    HRESULT hr = m_player->PhotoOpen(szParam);
    if (!DVR_SUCCEEDED(hr))
    {
        if(strlen(szParam) != 0)
            strcpy(m_curItem.szMediaFileName, szParam);
        return DVR_RES_EFAIL;
    }

    DPrint(DPRINT_INFO, "CPlayerPanel::PlayPicStart()\n");
    int res = m_player->PhotoPlay();
    if(res != DVR_RES_SOK && strlen(szParam) == 0)
        return res;

    strcpy(m_curItem.szMediaFileName, szParam);
    return res;
}

int CPlayerPanel::OnNextFile(void)
{
    if (m_player == NULL)
        return DVR_RES_EFAIL;

    DvrSystemControl *sysctrl = (DvrSystemControl*)m_player->GetSysCtrlHandle();
    if(sysctrl == NULL)
        return DVR_RES_EFAIL;

	DVR_FILEMAP_META_ITEM 	nextItem;
    memset(&nextItem, 0, sizeof(DVR_FILEMAP_META_ITEM));
    DVR_RESULT res = sysctrl->FileMapDB_GetNextFile(m_curItem.szMediaFileName, &nextItem, m_curtype);
    if(strlen(nextItem.szMediaFileName) == 0)
        sysctrl->FileMapDB_GetNameByRelPos(0, m_curtype, &nextItem, sizeof(DVR_FILEMAP_META_ITEM));

    DPrint(DPRINT_INFO, "CPlayerPanel::OnNextFile() %s\n",nextItem.szMediaFileName);
    if(strcmp(m_curItem.szMediaFileName,nextItem.szMediaFileName) == 0)
        return DVR_RES_SOK;

    if(m_playmode == PLAY_MODE_PHOTO)
        res = OnPicStart(nextItem.szMediaFileName);
    else
        res = OnStart(nextItem.szMediaFileName);
    if(res != DVR_RES_SOK && strlen(nextItem.szMediaFileName) == 0)
        return res;
    memcpy(&m_curItem, &nextItem, sizeof(DVR_FILEMAP_META_ITEM));
    return res;
}

int CPlayerPanel::OnPrevFile(void)
{
    if (m_player == NULL)
        return DVR_RES_EFAIL;

    DvrSystemControl *sysctrl = (DvrSystemControl*)m_player->GetSysCtrlHandle();
    if(sysctrl == NULL)
        return DVR_RES_EFAIL;

	DVR_FILEMAP_META_ITEM 	prevItem;
    memset(&prevItem, 0, sizeof(DVR_FILEMAP_META_ITEM));
    DVR_RESULT res = sysctrl->FileMapDB_GetPrevFile(m_curItem.szMediaFileName, &prevItem, m_curtype);
    if(strlen(prevItem.szMediaFileName) == 0)
        sysctrl->FileMapDB_GetNameByRelPos(0, m_curtype, &prevItem, sizeof(DVR_FILEMAP_META_ITEM));

    DPrint(DPRINT_INFO, "CPlayerPanel::OnPrevFile() %s\n",prevItem.szMediaFileName);
    //if(strcmp(m_curItem.szMediaFileName, prevItem.szMediaFileName) == 0)
    //    return DVR_RES_SOK;

    if(m_playmode == PLAY_MODE_PHOTO)
        res = OnPicStart(prevItem.szMediaFileName);
    else
        res = OnStart(prevItem.szMediaFileName);
    if(res != DVR_RES_SOK && strlen(prevItem.szMediaFileName) == 0)
        return res;

    memcpy(&m_curItem, &prevItem, sizeof(DVR_FILEMAP_META_ITEM));
    return res;
}

int CPlayerPanel::OnStart(char *szParam)
{
    if (m_player == NULL || szParam == NULL)
        return DVR_RES_EFAIL;

    HRESULT hr = m_player->Open(szParam);
    if (!DVR_SUCCEEDED(hr))
    {
        if(strlen(szParam) != 0)
            strcpy(m_curItem.szMediaFileName, szParam);
        return DVR_RES_EFAIL;
    }

    DVR_RESULT res = DVR_RES_EFAIL;
    if (m_speed == 1000 || m_speed == 0)
        res = m_player->Play();
    else
    {
        m_player->Set(DVR_PLAYER_PROP_SPEED, &m_speed, sizeof(m_speed));
        res = m_player->Play();
    }
    if(res != DVR_RES_SOK && strlen(szParam) == 0)
        return res;

    strcpy(m_curItem.szMediaFileName, szParam);
    return res;
}

int CPlayerPanel::OnStop(void)
{
    if (m_player == NULL)
        return DVR_RES_EFAIL;

    DPrint(DPRINT_INFO, "CPlayerPanel::PlayStop()\n");
    return  m_player->Stop();
}

void CPlayerPanel::ThreadProc(void *ptr)
{
    DVR_U32 Param1 = 0, Param2 = 0;
    APP_MESSAGE Msg = { 0 };
    CPlayerPanel *panel = (CPlayerPanel *)ptr;

    while (1)
    {
        panel->PlayRcvMsg(&Msg, DVR_TIMEOUT_WAIT_FOREVER);
        Param1 = Msg.MessageData[0];
        Param2 = Msg.MessageData[1];
        DPrint(DPRINT_LOG, "[Applib - CPlayerPanel] <MgrTask> Received msg: 0x%X (Param1 = 0x%X / Param2 = 0x%X)\n", Msg.MessageID, Param1, Param2);
        switch (Msg.MessageID)
        {
            case DVR_PLAYER_COMMAND_NEXT:
            panel->PlayNextFile();
            break;
            case DVR_PLAYER_COMMAND_PREV:
            panel->PlayPrevFile();
            break;
            case DVR_PLAYER_COMMAND_FSPEC:
            panel->PlaySpecFile(Param1);
            break;
            case DVR_PLAYER_COMMAND_STEPF:
            panel->PlayStepF();
            break;
            case DVR_PLAYER_COMMAND_STEPB:
            panel->PlayStepB();
            break;
            case DVR_PLAYER_COMMAND_THREAD_EXIT:
            printf ("rec DVR_PLAYER_COMMAND_THREAD_EXIT\n");
            //DVR::OSA_tskExit();
            break;
            default:
                break;
        }
    }
    
}

int CPlayerPanel::PlayRcvMsg(APP_MESSAGE *msg, DVR_U32 waitOption)
{
	int res = DVR::OSA_msgqRecv(&m_msgQueue, (void *)msg, waitOption);
	if (DVR_FAILED(res))
	{
		DPrint(DPRINT_ERR, "CPlayerPanel <RcvMsg> OSA_msgqRecv failed = 0x%x\n", res);
	}

	return res;
}

int CPlayerPanel::PlaySndMsg(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2)
{
	APP_MESSAGE TempMessage = { 0 };

	TempMessage.MessageID = msg;
	TempMessage.MessageData[0] = param1;
	TempMessage.MessageData[1] = param2;

	int res = DVR::OSA_msgqSend(&m_msgQueue, &TempMessage, DVR_TIMEOUT_NO_WAIT);
	if (DVR_FAILED(res))
	{
		DPrint(DPRINT_ERR, "CPlayerPanel <SndMsg> OSA_msgqRecv failed = 0x%x\n", res);
	}

	return res;
}

