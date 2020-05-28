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

#include "DvrComSvc_Timer.h"

/* Timer period table */
static int TimerPeriodTable[TIMER_NUM] = {
	10000,      /**< TIMER_CHECK 10 seconds */
	1000,       /**< TIMER_1HZ 1 second */
	500,        /**< TIMER_2HZ 500 ms */
	250,        /**< TIMER_4HZ 250 ms*/
	100,        /**< TIMER_10HZ 100 ms*/
	50,         /**< TIMER_20HZ 50ms */
	5000,       /**< TIMER_5S 5 seconds */
	30000       /* *<TIMER_30S 30 seconds*/
};

static DvrComSvcTimer m_timer_pool[TIMER_NUM];

static void DvrComSvcTimer_TimerHandler(DVR_U32 exinf)
{
	DVR_U32 Message = 0;
	Message = HMSG_COMSVC_MODULE_TIMER(exinf); /* exinf = timer id*/
	DPrint(DPRINT_LOG, "<DvrComSvcTimer_TimerHandler> send Message: 0x%X / exinf = %d\n", Message, exinf);
	Dvr_Sdk_ComSvcHcMgr_SndMsg(Message, exinf, 0);
}

int DvrComSvcTimer::DvrComSvcTimer_ClearTimer(int tid)
{
	int res = 0;

	/* Sanity check */
	if ((tid < 0) || (tid >= TIMER_NUM)) {
		return -1;
	}

	DVR::OSA_mutexLock(&m_mutexHndl);

	/* Stop and delete the timer */
	if (m_timer_pool[tid].m_valid) {
		DVR::OSA_timerStop(&m_timer_pool[tid].m_appTimer);
		DVR::OSA_timerDelete(&m_timer_pool[tid].m_appTimer);

		memset(&m_timer_pool[tid], 0, sizeof(DvrComSvcTimer));
	}

	DVR::OSA_mutexUnlock(&m_mutexHndl);

	return res;
}

int DvrComSvcTimer::DvrComSvcTimer_SetTimer(int tid)
{
	int res = 0;

	/* Sanity check */
	if ((tid < 0) || (tid >= TIMER_NUM)) {
		return -1;
	}

	DVR::OSA_mutexLock(&m_mutexHndl);
	/* If this is an old timer set before, delete it first */
	if (m_timer_pool[tid].m_valid) {
		DVR::OSA_timerStop(&m_timer_pool[tid].m_appTimer);
		DVR::OSA_timerDelete(&m_timer_pool[tid].m_appTimer);
	}

	/* Create a timer hander */
	m_timer_pool[tid].m_timerId = DVR::OSA_timerCreate(&m_timer_pool[tid].m_appTimer,
		OSA_TIMER_AUTO_START,
		DvrComSvcTimer_TimerHandler, tid,
		TimerPeriodTable[tid]);

	if (m_timer_pool[tid].m_timerId < 0) 
	{
		res = -1;
	}
	else 
	{
		m_timer_pool[tid].m_valid = 1;
		res = 0;
	}

	DVR::OSA_mutexUnlock(&m_mutexHndl);

	return res;
}


DvrComSvcTimer::DvrComSvcTimer()
{
	DVR_S32 res;

	/*Create Mutex*/
	res = DVR::OSA_mutexCreate(&m_mutexHndl);
	if (DVR_FAILED(res))
	{
		DPrint(DPRINT_ERR, "DvrComSvcHcMgr <Init> Create Mutex failed = 0x%x\n", res);
	}

	memset(m_timer_pool, 0, sizeof(DvrComSvcTimer) * TIMER_NUM);
}

DvrComSvcTimer::~DvrComSvcTimer()
{

}

int DvrComSvcTimer::DvrComSvcTimer_UnregisterAll(void)
{
	int i = 0, j = 0;

	/* TIMER_CHECK = 0 is the auto-poweroff timer */
	for (i = 1; i < TIMER_NUM; i++) {
		for (j = 0; j < MAX_TIMER_HANDLER; j++) {
			m_timer_pool[i].m_handler[j] = NULL;
		}
		m_timer_pool[i].m_handlerNum = 0;
		DvrComSvcTimer_ClearTimer(i);
	}

	return 0;
}

int DvrComSvcTimer::DvrComSvcTimer_Register(int tid, PFN_APPTIMER_HANDLER handler)
{
	int i = 0, Found = 0;
	DvrComSvcTimer *CurrentTimer = NULL;

	if (tid >= TIMER_NUM) {
		// no such timer
		DPrint(DPRINT_ERR, "[DvrComSvcTimer - Timer] <%s> No timer id %d\n", __FUNCTION__, tid);
		return -1;
	}

	if (handler == NULL) {
		// NULL Handler
		DPrint(DPRINT_ERR, "[DvrComSvcTimer - Timer] <%s>Timer handler is NULL\n", __FUNCTION__);
		return -1;
	}

	CurrentTimer = &(m_timer_pool[tid]);

	Found = 0;
	for (i = 0; i < MAX_TIMER_HANDLER; i++) {
		if (handler == CurrentTimer->m_handler[i]) {
			// Handler already exists
			DPrint(DPRINT_LOG, "[DvrComSvcTimer - Timer] <%s>This timer handler has already been registered\n", __FUNCTION__);
			Found = 1;
			break;
		}
	}

	if (!Found) {
		for (i = 0; i < MAX_TIMER_HANDLER; i++) {
			if (CurrentTimer->m_handler[i] == NULL) {
				// Empty slot Found. Insert Handler
				CurrentTimer->m_handler[i] = handler;
				CurrentTimer->m_handlerNum++;
				if (!CurrentTimer->m_valid) {
					DPrint(DPRINT_LOG, "[DvrComSvcTimer - Timer] <%s>Timer id %d is inValid. Create timer id %d\n", __FUNCTION__, tid, tid);
					DvrComSvcTimer_SetTimer(tid);
				}
				break;
			}
		}
	}

	return 0;
}

int DvrComSvcTimer::DvrComSvcTimer_Unregister(int tid, PFN_APPTIMER_HANDLER handler)
{
	int i = 0, Found = 0;
	DvrComSvcTimer *CurrentTimer = NULL;

	if (tid >= TIMER_NUM) {
		// No such timer
		DPrint(DPRINT_ERR, "[DvrComSvcTimer - Timer] <%s>No timer id %d\n", __FUNCTION__, tid);
		return -1;
	}

	if (handler == NULL) {
		// NULL Handler
		DPrint(DPRINT_ERR, "[DvrComSvcTimer - Timer] <%s>Timer handler is NULL\n", __FUNCTION__);
		return -1;
	}

	CurrentTimer = &(m_timer_pool[tid]);

	if (CurrentTimer->m_handlerNum == 0) {
		// No Handlers in timer
		DPrint(DPRINT_ERR, "[DvrComSvcTimer - Timer] <%s>No timer handler is registered\n", __FUNCTION__);
		return -1;
	}

	Found = 0;
	for (i = 0; i < MAX_TIMER_HANDLER; i++) {
		if (handler == CurrentTimer->m_handler[i]) {
			Found = 1;
			CurrentTimer->m_handler[i] = NULL;
			CurrentTimer->m_handlerNum--;
			handler(TIMER_UNREGISTER);
			break;
		}
	}

	if (Found) {
		if (CurrentTimer->m_handlerNum == 0) {
			// all Handlers removed in timer. clear timer
			DPrint(DPRINT_LOG, "[DvrComSvcTimer - Timer] <%s>No timer handler under timer id %d. Delete timer id %d", __FUNCTION__, tid, tid);
			DvrComSvcTimer_ClearTimer(tid);
		}
	}

	return 0;
}

int DvrComSvcTimer::DvrComSvcTimer_Handler(int tid)
{
	int i = 0;
	DvrComSvcTimer *CurrentTimer = NULL;

	if (tid >= TIMER_NUM) {
		// no such timer
		DPrint(DPRINT_ERR, "[DvrComSvcTimer - Timer] <%s>No timer id %d\n", __FUNCTION__, tid);
		return -1;
	}

	CurrentTimer = &(m_timer_pool[tid]);

	if (!CurrentTimer->m_valid) {
		// timer is not Valid
		DPrint(DPRINT_ERR, "[DvrComSvcTimer - Timer] <%s>Timer id %d is invalid\n", __FUNCTION__, tid);
		return -1;
	}

	if (CurrentTimer->m_handlerNum == 0) {
		// no Handlers in timer
		DPrint(DPRINT_ERR, "[DvrComSvcTimer - Timer] <%s>No timer handler is registered\n", __FUNCTION__);
		return -1;
	}

	for (i = 0; i < MAX_TIMER_HANDLER; i++) {
		if (CurrentTimer->m_handler[i] != NULL) {
			DPrint(DPRINT_LOG, "[DvrComSvcTimer - Timer] <%s> Execute handler 0x%X\n", __FUNCTION__, CurrentTimer->m_handler[i]);
			CurrentTimer->m_handler[i](TIMER_TICK);
		}
	}

	return 0;
}

int DvrComSvcTimer::DvrComSvcTimer_Start(int tid)
{
	int i = 0, Found = 0;
	DvrComSvcTimer *CurrentTimer = NULL;

	if (tid >= TIMER_NUM) {
		// No such timer
		DPrint(DPRINT_ERR, "[DvrComSvcTimer - Timer] <%s>No timer id %d\n", __FUNCTION__, tid);
		return -1;
	}

	CurrentTimer = &(m_timer_pool[tid]);

	DVR::OSA_timerStart(&CurrentTimer->m_appTimer);

	return 0;
}