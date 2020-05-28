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

#ifdef __linux__
#include "pthread.h"
#else 
#include <Windows.h>
#include <MMSystem.h> 
#ifdef WINCE
#pragma comment (lib, "mmtimer.lib")
#else
#pragma comment (lib, "winmm.lib")
#endif
#endif

#include <cstring>

#include "DvrTimer.h"
#include "dprint.h"

#ifdef __linux__
class DvrTimerLinuxPrivate
{
public:
	typedef void *(*Callback)(void *arg);

	DvrTimerLinuxPrivate(Callback callback, void *context);
	~DvrTimerLinuxPrivate();
	DVR_RESULT Start(const struct timeval &tv);
	DVR_RESULT Stop();
private:
	DISABLE_COPY(DvrTimerLinuxPrivate)

	static void *ThreadProc(void *arg);

	Callback       m_callback;
	pthread_t      m_thread;
	void *         m_context;
	bool           m_running;
	struct timeval m_time;
};

#else

class DvrTimerWindowsPrivate
{
public:
	typedef void *(*Callback)(void *arg);


	DvrTimerWindowsPrivate(Callback callback, void *context);
	~DvrTimerWindowsPrivate();

	DVR_RESULT Start(const struct timeval &tv);
	DVR_RESULT Stop();
private:
	DISABLE_COPY(DvrTimerWindowsPrivate)

	static void CALLBACK timerCallBack(UINT uTimerID, UINT uMsg, 
		DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

	Callback       m_callback;	
	void *         m_context;
	bool           m_running;
	UINT	       m_timeid;
	struct timeval m_time;
};
#endif

DvrTimer::DvrTimer(Callback callback, void *context)
{
#ifdef __linux__
	m_p = new DvrTimerLinuxPrivate(callback, context);
#else
	m_p = new DvrTimerWindowsPrivate(callback, context);
#endif
}

DvrTimer::~DvrTimer()
{
	if(m_p != NULL)
	{
		m_p->Stop();
		delete m_p;
		m_p = NULL;
	}
}

DVR_RESULT DvrTimer::Start(const struct timeval &tv)
{
	if(m_p != NULL)
		m_p->Start(tv);

	return DVR_RES_SOK;
}

DVR_RESULT DvrTimer::Stop()
{
	if(m_p != NULL)
		m_p->Stop();

	return DVR_RES_SOK;
}

#ifdef __linux__
DvrTimerLinuxPrivate::DvrTimerLinuxPrivate(Callback callback, void *context)
{
	m_callback = callback;
	m_context  = context;
	m_running  = false;
	memset(&m_thread, 0, sizeof(m_thread));

	memset(&m_time, 0, sizeof(m_time));
}

DvrTimerLinuxPrivate::~DvrTimerLinuxPrivate()
{
	Stop();
}

DVR_RESULT DvrTimerLinuxPrivate::Start(const struct timeval &tv)
{
	m_time = tv;

    if (!m_running) {
        DPrint(DPRINT_LOG, "DvrTimerLinuxPrivate::Start() interval is %d.%06d\n", tv.tv_sec, tv.tv_usec);
        m_running = true;
		pthread_create(&m_thread, NULL, DvrTimerLinuxPrivate::ThreadProc, this);
    }

	return DVR_RES_SOK;
}

DVR_RESULT DvrTimerLinuxPrivate::Stop()
{
	if (m_running) {
		m_running = false;
		pthread_join(m_thread, NULL);
        DPrint(DPRINT_LOG, "DvrTimerLinuxPrivate::Stop() thread is joined\n");
		memset(&m_thread, 0, sizeof(m_thread));
	}

	return DVR_RES_SOK;
}

void *DvrTimerLinuxPrivate::ThreadProc(void *arg)
{
	DvrTimerLinuxPrivate *p = (DvrTimerLinuxPrivate *)arg;

	while (p->m_running) {
		struct timeval tv;
		tv.tv_sec  = p->m_time.tv_sec;
		tv.tv_usec = p->m_time.tv_usec;
		if (select(0, 0, 0, 0, &tv) == 0) {
			if(!p->m_running)
				break;
			if(p->m_callback)
			    p->m_callback(p->m_context);
		}
	}

	return (void *)0;
}

#else

DvrTimerWindowsPrivate::DvrTimerWindowsPrivate(Callback callback, void *context)
{
	m_callback = callback;
	m_context  = context;
	m_running  = false;
	m_timeid   = 0;

	memset(&m_time, 0, sizeof(m_time));
}

DvrTimerWindowsPrivate::~DvrTimerWindowsPrivate()
{
	Stop();
}

DVR_RESULT DvrTimerWindowsPrivate::Start(const struct timeval &tv)
{
	unsigned long ms = 0;
	m_time = tv;

	m_running = true;
	ms = m_time.tv_sec * 1000 + m_time.tv_usec / 1000;
	m_timeid = timeSetEvent(ms, 1, 
		DvrTimerWindowsPrivate::timerCallBack, (DWORD_PTR)this, TIME_PERIODIC /* TIME_ONESHOT */);

	return DVR_RES_SOK;
}

DVR_RESULT DvrTimerWindowsPrivate::Stop()
{
	if (m_running) {
		m_running = false;
		timeKillEvent(m_timeid);
		m_timeid = 0;
	}

	return DVR_RES_SOK;
}

void DvrTimerWindowsPrivate::timerCallBack(UINT uTimerID, UINT uMsg,
					   DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	DvrTimerWindowsPrivate *p = (DvrTimerWindowsPrivate *)dwUser;

	if (p->m_running) {
		p->m_callback(p->m_context);
	}
}

#endif
