#ifndef __PLAYPANEL_H__
#define __PLAYPANEL_H__
#include "Object.h"
#include "osa_tsk.h"
#include "osa_msgq.h"
#include "DVR_SDK_INTFS.h"

#define PLAY_MODE_NORMAL 		(0)
#define	PLAY_MODE_EVENT 		(1)
#define	PLAY_MODE_DAS 			(2)
#define	PLAY_MODE_PHOTO 		(3)
#define PLAY_MGR_MSGQUEUE_SIZE  (16)

typedef enum _tagDVR_PLAYER_COMMAND
{
	DVR_PLAYER_COMMAND_NULL = 0,
	DVR_PLAYER_COMMAND_START,
	DVR_PLAYER_COMMAND_STOP,
	DVR_PLAYER_COMMAND_PAUSE,
	DVR_PLAYER_COMMAND_RESUME,
	DVR_PLAYER_COMMAND_NEXT,
	DVR_PLAYER_COMMAND_PREV,
	DVR_PLAYER_COMMAND_FSPEC,
	DVR_PLAYER_COMMAND_STEPF,
	DVR_PLAYER_COMMAND_STEPB,
	DVR_PLAYER_COMMAND_THREAD_EXIT,
	DVR_PLAYER_COMMAND_NUM
} DVR_PLAYER_COMMAND;

class DvrMutex;
class DvrPlayerLoop;
class CPlayerPanel : public CObject
{
public:
	CPlayerPanel(void *loop);
	virtual ~CPlayerPanel(void);

public:
	int 	PlayStart(char *szParam);
	int		PlayStop(void);
	int		PlayClean(void);
	int		PlayPause(void);
	int 	PlayResume(void);
	int		PlaySetPosition(unsigned pos);
	int		PlaySetSpeed(int speed);
	int		PlaySetType(int type);
	int		PlayGetFile(char *filename, int length);
	int		PlayNextFile(void);
	int		PlayPrevFile(void);
	int		PlaySpecFile(unsigned pos);
	int		PlayPicStart(char *szParam);
	int		PlayPicStop(void);
	int		PlayStepB(void);
	int		PlayStepF(void);
	int 	PlayRcvMsg(APP_MESSAGE *msg, DVR_U32 waitOption);
	int 	PlaySndMsg(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2);

protected:
	int		OnStart(char *szParam);
	int		OnPicStart(char *szParam);
	int		OnStop(void);
	int		OnNextFile(void);
	int		OnPrevFile(void);

protected:
    
	static void 	ThreadProc(void *ptr);
	DVR_U8 			MsgPool[sizeof(APP_MESSAGE)*PLAY_MGR_MSGQUEUE_SIZE];
	OSA_TskHndl 	m_hThread;
	OSA_MsgqHndl 	m_msgQueue;

private:
	short					m_speed;
	unsigned short			m_playmode;
	DvrMutex				*m_mutex;
	DvrPlayerLoop 			*m_player;
	DVR_FOLDER_TYPE 		m_curtype;
	DVR_FILEMAP_META_ITEM 	m_curItem;
};
#endif//__PLAYPANEL_H__

