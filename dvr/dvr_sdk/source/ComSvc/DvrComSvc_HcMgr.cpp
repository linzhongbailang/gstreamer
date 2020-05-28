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
#include "DvrComSvc_HcMgr.h"

void DvrComSvcHcMgr::DvrComSvcHcmgr_Task(void *ptr)
{
	DvrComSvcHcMgr *p = (DvrComSvcHcMgr *)ptr;

	while (p->m_handler == NULL)
	{
		//DPrint(DPRINT_ERR, "DvrComSvcHcMgr <Task> waiting..........");
        OSA_Sleep(50);
		if (p->m_bExit == TRUE)
			break;
	}
	if (p->m_handler != NULL) {
		p->m_handler->HandlerMain(p->m_pContext);
	}
	else {
		DPrint(DPRINT_ERR, "DvrComSvcHcMgr <Task> The handler is not registered.");
	}
}

DvrComSvcHcMgr::DvrComSvcHcMgr(void)
{
	DVR_S32 res;

	m_handler = NULL;
	m_pContext = NULL;
	m_bExit = FALSE;

	/*Create App Message Queue*/
    strcpy(m_msgQueue.name,"DvrComSvcHcMgr");
	res = DVR::OSA_msgqCreate(&m_msgQueue, MsgPool, sizeof(APP_MESSAGE), HCMGR_MSGQUEUE_SIZE);
	if (DVR_FAILED(res))
	{
		DPrint(DPRINT_ERR, "DvrComSvcHcMgr <Init> Create Queue failed = 0x%x", res);
	}

	/*Create Mutex*/
	res = DVR::OSA_mutexCreate(&m_mutexHndl);
	if (DVR_FAILED(res))
	{
		DPrint(DPRINT_ERR, "DvrComSvcHcMgr <Init> Create Mutex failed = 0x%x", res);
	}

	res = DVR::OSA_tskCreate(&m_hThread, DvrComSvcHcmgr_Task, this);
	if (DVR_FAILED(res))
	{
		DPrint(DPRINT_ERR, "DvrComSvcHcMgr <Init> Create Task failed = 0x%x", res);
	}
}

DvrComSvcHcMgr::~DvrComSvcHcMgr(void)
{
	m_bExit = TRUE;

	DVR::OSA_msgqDelete(&m_msgQueue);
	DVR::OSA_mutexDelete(&m_mutexHndl);
	DVR::OSA_tskDelete(&m_hThread);
}

int DvrComSvcHcMgr::DvrComSvcHcmgr_AttachHandler(DVR_HCMGR_HANDLER *handler, void *pContext)
{
	DVR::OSA_mutexLock(&m_mutexHndl);

	if (handler == NULL)
	{
		DVR::OSA_mutexUnlock(&m_mutexHndl);
		return DVR_RES_EINVALIDARG;
	}

    DPrint(DPRINT_LOG, "[%s]handler = %p, pContext = %p\n", __FUNCTION__, handler, pContext);

	m_handler = handler;
	m_pContext = pContext;

	DVR::OSA_mutexUnlock(&m_mutexHndl);

	return DVR_RES_SOK;
}

int DvrComSvcHcMgr::DvrComSvcHcmgr_DetachHandler(void)
{
	DVR::OSA_mutexLock(&m_mutexHndl);

	if (m_handler == NULL)
	{
		DVR::OSA_mutexUnlock(&m_mutexHndl);
		return DVR_RES_EUNEXPECTED;
	}

	m_handler->HandlerExit();

	DVR::OSA_mutexUnlock(&m_mutexHndl);

	return DVR_RES_SOK;
}

int DvrComSvcHcMgr::DvrComSvcHcmgr_ResetHandler(void)
{
	DVR::OSA_mutexLock(&m_mutexHndl);

	m_handler = NULL;

	DVR::OSA_mutexUnlock(&m_mutexHndl);

	return DVR_RES_SOK;
}

int DvrComSvcHcMgr::DvrComSvcHcmgr_SndMsg(APP_MESSAGE *msg, DVR_U32 waitOption)
{
	int res;

	res = DVR::OSA_msgqSend(&m_msgQueue, (void *)msg, waitOption);
	if (DVR_FAILED(res))
	{
		DPrint(DPRINT_ERR, "DvrComSvcHcMgr <SndMsg> OSA_msgqSend failed = 0x%x\n", res);
	}

	return res;
}

int DvrComSvcHcMgr::DvrComSvcHcmgr_RcvMsg(APP_MESSAGE *msg, DVR_U32 waitOption)
{
	int res;

	res = DVR::OSA_msgqRecv(&m_msgQueue, (void *)msg, waitOption);
	if (DVR_FAILED(res))
	{
		DPrint(DPRINT_ERR, "DvrComSvcHcMgr <SndMsg> OSA_msgqRecv failed = 0x%x\n", res);
	}

	return res;
}

int DvrComSvcHcMgr::DvrComSvcHcmgr_SendMsg(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2)
{
	APP_MESSAGE TempMessage = { 0 };

	TempMessage.MessageID = msg;
	TempMessage.MessageData[0] = param1;
	TempMessage.MessageData[1] = param2;

	return DvrComSvcHcmgr_SndMsg(&TempMessage, DVR_TIMEOUT_WAIT_FOREVER);
}
