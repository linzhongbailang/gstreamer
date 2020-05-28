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
#ifndef _DVR_COMSVC_TIMER_H_
#define _DVR_COMSVC_TIMER_H_

#include "DVR_SDK_INTFS.h"
#include "osa.h"
#include "osa_mutex.h"
#include "osa_timer.h"

#define MAX_TIMER_HANDLER   (8) /**<MAX_TIMER_HANDLER   (8)*/

class DvrComSvcTimer
{
public:
	DvrComSvcTimer(void);
	~DvrComSvcTimer(void);

	int DvrComSvcTimer_UnregisterAll(void);
	int DvrComSvcTimer_Register(int tid, PFN_APPTIMER_HANDLER handler);
	int DvrComSvcTimer_Unregister(int tid, PFN_APPTIMER_HANDLER handler);
	int DvrComSvcTimer_Handler(int tid);
	int DvrComSvcTimer_ClearTimer(int tid);
	int DvrComSvcTimer_SetTimer(int tid);
	int DvrComSvcTimer_Start(int tid);

private:
	DISABLE_COPY(DvrComSvcTimer)

	int m_timerId;   /* Cyclic handler ID for the timer */
	DVR_U32 m_valid;  /* 1:Valid timer, 0:free entry */
	int m_handlerNum;
	OSA_TimerHndl m_appTimer;
	OSA_MutexHndl m_mutexHndl;
	PFN_APPTIMER_HANDLER m_handler[MAX_TIMER_HANDLER];
};

#endif /* _DVR_COMSVC_TIMER_H_ */
