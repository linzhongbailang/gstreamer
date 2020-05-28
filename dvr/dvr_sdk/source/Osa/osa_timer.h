#ifndef _OSA_TIMER_H_
#define _OSA_TIMER_H_

#include <osa.h>

typedef void(*OSA_TimerCallBack)(DVR_U32 arg);

#define OSA_TIMER_AUTO_START	1
#define OSA_TIMER_DONT_START	0

#ifdef WIN32

#include <Windows.h>
#include <MMSystem.h> 

typedef struct {
	OSA_TimerCallBack       m_callback;
	DVR_U32         m_arg;
	DVR_BOOL        m_running;
	DVR_U32	       m_timeid;
	struct timeval m_time;
} OSA_TimerHndl;

#else

#include <pthread.h>

typedef struct {
	OSA_TimerCallBack       m_callback;
	pthread_t	   m_thread;
	DVR_U32         m_arg;
	DVR_BOOL        m_running;
	struct timeval m_time;
} OSA_TimerHndl;

#endif

namespace DVR
{
	DVR_S32 OSA_timerCreate(OSA_TimerHndl *hndl, DVR_BOOL bAutoStart, OSA_TimerCallBack callback, DVR_U32 ExpirationArg, DVR_U32 reload_msec);
	DVR_S32 OSA_timerDelete(OSA_TimerHndl *hndl);
	DVR_S32 OSA_timerStart(OSA_TimerHndl *hndl);
	DVR_S32 OSA_timerStop(OSA_TimerHndl *hndl);
}

#endif