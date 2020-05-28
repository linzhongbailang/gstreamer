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
#include "DvrStorage_AsyncOp.h"

void DvrStorageAsyncOp::DvrStorageAsyncOp_MgrTask(void *ptr)
{
	int i = 0;
	APP_MESSAGE Msg = { 0 };
	DvrStorageAsyncOp *p = (DvrStorageAsyncOp *)ptr;

	while (1)
	{
		p->DvrStorageAsyncOp_RcvMsg(&Msg, DVR_TIMEOUT_WAIT_FOREVER);
		DPrint(DPRINT_LOG, "[Applib - StorageAsyncOp] <MgrTask> Received msg: 0x%X (Param1 = 0x%X / Param2 = 0x%X)\n", Msg.MessageID, Msg.MessageData[0], Msg.MessageData[1]);

		for (i = 0; i < APPLIB_MAX_STORAGE_OP_HANDLER; i++)
		{
			if (p->m_handler[i].Command == Msg.MessageID)
			{
				if (p->m_handler[i].FuncSearch != NULL)
				{
					DPrint(DPRINT_LOG,"[Applib - StorageAsyncOp] <MgrTask> call function FuncSearch\n");
					p->m_handler[i].FuncSearch(Msg.MessageData[0], Msg.MessageData[1], p->m_handler[i].pContext);
				}

				if (p->m_handler[i].FuncHandle != NULL)
				{
					DPrint(DPRINT_LOG, "[Applib - StorageAsyncOp] <MgrTask> call function FuncHandle\n");
					p->m_handler[i].FuncHandle(Msg.MessageData[0], Msg.MessageData[1], p->m_handler[i].pContext);
				}

				if (p->m_handler[i].FuncReturn != NULL)
				{
					DPrint(DPRINT_LOG, "[Applib - StorageAsyncOp] <MgrTask> call function FuncReturn\n");
                    p->m_handler[i].FuncReturn(Msg.MessageData[0], Msg.MessageData[1], p->m_handler[i].pContext);
				}

				break;
			}
		}
	}
}

DvrStorageAsyncOp::DvrStorageAsyncOp()
{
	DVR_S32 res;

	/*Create App Message Queue*/
    strcpy(m_msgQueue.name,"DvrStorageAsyncOp");
	res = DVR::OSA_msgqCreate(&m_msgQueue, MsgPool, sizeof(APP_MESSAGE), ASYNC_OP_MGR_MSGQUEUE_SIZE);
	if (DVR_FAILED(res))
	{
		DPrint(DPRINT_ERR, "DvrStorageAsyncOp <Init> Create Queue failed = 0x%x\n", res);
	}

	res = DVR::OSA_tskCreate(&m_hThread, DvrStorageAsyncOp_MgrTask, this);
	if (DVR_FAILED(res))
	{
		DPrint(DPRINT_ERR, "DvrStorageAsyncOp <Init> Create Task failed = 0x%x\n", res);
	}

	m_asyncOpHandlerCount = 0;
}


DvrStorageAsyncOp::~DvrStorageAsyncOp()
{
	DVR::OSA_msgqDelete(&m_msgQueue);
	DVR::OSA_tskDelete(&m_hThread);
}

int DvrStorageAsyncOp::DvrStorageAsyncOp_RcvMsg(APP_MESSAGE *msg, DVR_U32 waitOption)
{
	int res;

	res = DVR::OSA_msgqRecv(&m_msgQueue, (void *)msg, waitOption);
	if (DVR_FAILED(res))
	{
		DPrint(DPRINT_ERR, "DvrStorageAsyncOp <RcvMsg> OSA_msgqRecv failed = 0x%x\n", res);
	}

	return res;
}

int DvrStorageAsyncOp::DvrStorageAsyncOp_SndMsg(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2)
{
	int res = 0;
	APP_MESSAGE TempMessage = { 0 };

	TempMessage.MessageID = msg;
	TempMessage.MessageData[0] = param1;
	TempMessage.MessageData[1] = param2;

	res = DVR::OSA_msgqSend(&m_msgQueue, &TempMessage, DVR_TIMEOUT_NO_WAIT);
	if (DVR_FAILED(res))
	{
		DPrint(DPRINT_ERR, "DvrStorageAsyncOp <SndMsg> OSA_msgqRecv failed = 0x%x\n", res);
	}

	return res;
}

int DvrStorageAsyncOp::DvrStorageAsyncOp_RegHandler(DVR_STORAGE_HANDLER *handler)
{
	int ReturnValue = 0;
	if (m_asyncOpHandlerCount < APPLIB_MAX_STORAGE_OP_HANDLER) {
		memcpy(&m_handler[m_asyncOpHandlerCount], handler, sizeof(DVR_STORAGE_HANDLER));
		m_asyncOpHandlerCount++;
	}
	else {
		DPrint(DPRINT_ERR, "[%s] <RegHandler> Register Handler Error, Handler runout\n", __FUNCTION__);
		ReturnValue = -1;
	}
	return ReturnValue;
}

int DvrStorageAsyncOp::DvrStorageAsyncOp_ClrHandler(void)
{
    m_asyncOpHandlerCount = 0;
    memset(m_handler, 0, sizeof(m_handler));

    return 0;
}