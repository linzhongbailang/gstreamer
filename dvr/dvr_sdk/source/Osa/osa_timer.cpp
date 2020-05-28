#include <windows.h>
#include <osa_timer.h>

#ifdef WIN32

#pragma comment (lib, "winmm.lib")

void CALLBACK timerCallBack(UINT uTimerID, UINT uMsg,
	DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	OSA_TimerHndl *p = (OSA_TimerHndl *)dwUser;

	if (p->m_running) {
		p->m_callback(p->m_arg);
	}
}

DVR_S32 DVR::OSA_timerCreate(OSA_TimerHndl *hndl, DVR_BOOL bAutoStart, OSA_TimerCallBack callback, DVR_U32 ExpirationArg, DVR_U32 reload_msec)
{
	hndl->m_callback = callback;
	hndl->m_arg = ExpirationArg;
	hndl->m_running = FALSE;
	hndl->m_timeid = 0;
	
	hndl->m_time.tv_sec = reload_msec / 1000;
	hndl->m_time.tv_usec = reload_msec % 1000 * 1000;

	if (bAutoStart == OSA_TIMER_AUTO_START)
		OSA_timerStart(hndl);

	return DVR_RES_SOK;
}

DVR_S32 DVR::OSA_timerDelete(OSA_TimerHndl *hndl)
{
	return DVR_RES_SOK;
}

DVR_S32 DVR::OSA_timerStart(OSA_TimerHndl *hndl)
{
	unsigned long ms = 0;

	if (hndl->m_running == TRUE)
		return DVR_RES_SOK;

	hndl->m_running = TRUE;
	ms = hndl->m_time.tv_sec * 1000 + hndl->m_time.tv_usec / 1000;
	hndl->m_timeid = timeSetEvent(ms, 1, timerCallBack, (DWORD_PTR)hndl, TIME_PERIODIC);

	return DVR_RES_SOK;
}

DVR_S32 DVR::OSA_timerStop(OSA_TimerHndl *hndl)
{
	if (hndl->m_running) {
		hndl->m_running = FALSE;
		timeKillEvent(hndl->m_timeid);
		hndl->m_timeid = 0;
	}

	return DVR_RES_SOK;
}

#else

void *ThreadProc(void *arg)
{
	OSA_TimerHndl *p = (OSA_TimerHndl *)arg;

	while (p->m_running) {
		struct timeval tv;
		tv.tv_sec = p->m_time.tv_sec;
		tv.tv_usec = p->m_time.tv_usec;
		if (select(0, 0, 0, 0, &tv) == 0) {
			p->m_callback(p->m_arg);
		}
	}

	return NULL;
}

DVR_S32 DVR::OSA_timerCreate(OSA_TimerHndl *hndl, DVR_BOOL bAutoStart, OSA_TimerCallBack callback, DVR_U32 ExpirationArg, DVR_U32 reload_msec)
{
	hndl->m_callback = callback;
	hndl->m_arg = ExpirationArg;
	hndl->m_running = FALSE;
	memset(&hndl->m_thread, 0, sizeof(hndl->m_thread));

	hndl->m_time.tv_sec = reload_msec / 1000;
	hndl->m_time.tv_usec = reload_msec % 1000 * 1000;

	if (bAutoStart == OSA_TIMER_AUTO_START)
		OSA_timerStart(hndl);

	return DVR_RES_SOK;
}

DVR_S32 DVR::OSA_timerDelete(OSA_TimerHndl *hndl)
{
    if(hndl->m_thread != 0)
    {
        pthread_join(hndl->m_thread, NULL);
        memset(&hndl->m_thread, 0, sizeof(hndl->m_thread));
    }
    return DVR_RES_SOK;
}

DVR_S32 DVR::OSA_timerStart(OSA_TimerHndl *hndl)
{
	if (!hndl->m_running) {
		hndl->m_running = TRUE;
		pthread_create(&hndl->m_thread, NULL, ThreadProc, hndl);
	}

	return DVR_RES_SOK;
}

DVR_S32 DVR::OSA_timerStop(OSA_TimerHndl *hndl)
{
	if (hndl->m_running) {
		hndl->m_running = FALSE;
	}

	return DVR_RES_SOK;
}


#endif
