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
#ifndef _DVR_COMSVC_HCMGR_H_
#define _DVR_COMSVC_HCMGR_H_

#include "DVR_SDK_DEF.h"
#include "osa.h"
#include "osa_mutex.h"
#include "osa_tsk.h"
#include "osa_msgq.h"

#define HCMGR_MSGQUEUE_SIZE     128

class DvrComSvcHcMgr
{
public:
	DvrComSvcHcMgr(void);
	~DvrComSvcHcMgr(void);

	int DvrComSvcHcmgr_AttachHandler(DVR_HCMGR_HANDLER *handler, void *pContext);
	int DvrComSvcHcmgr_DetachHandler(void);
	int DvrComSvcHcmgr_ResetHandler(void);
	int DvrComSvcHcmgr_SndMsg(APP_MESSAGE *msg, DVR_U32 waitOption);
	int DvrComSvcHcmgr_RcvMsg(APP_MESSAGE *msg, DVR_U32 waitOption);
	int DvrComSvcHcmgr_SendMsg(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2);

private:
	DISABLE_COPY(DvrComSvcHcMgr)

	static void DvrComSvcHcmgr_Task(void *ptr);

	DVR_U8 MsgPool[sizeof(APP_MESSAGE)*HCMGR_MSGQUEUE_SIZE];
	OSA_MutexHndl m_mutexHndl;
	OSA_TskHndl m_hThread;
	OSA_MsgqHndl m_msgQueue;
	DVR_HCMGR_HANDLER *m_handler;
	void *m_pContext;
	DVR_BOOL m_bExit;
};

#endif