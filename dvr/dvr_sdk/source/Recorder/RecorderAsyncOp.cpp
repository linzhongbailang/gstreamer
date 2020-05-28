
#include "RecorderAsyncOp.h"

void RecorderAsyncOp::RecorderAsyncOp_MgrTask(void *ptr)
{
	int i = 0;
	DVR_U32 Param1 = 0, Param2 = 0;
	APP_MESSAGE Msg = { 0 };
	RecorderAsyncOp *p = (RecorderAsyncOp *)ptr;

	while (1)
	{
		p->RecorderAsyncOp_RcvMsg(&Msg, DVR_TIMEOUT_WAIT_FOREVER);
		Param1 = Msg.MessageData[0];
		Param2 = Msg.MessageData[1];

		DVR::OSA_mutexLock(&p->m_mutexHndl);

		for (i = 0; i < RECORDER_MAX_ASYNC_OP_HANDLER; i++)
		{
			if (p->m_handler[i].Command == Msg.MessageID)
			{
				if (p->m_handler[i].FuncPrepare != NULL)
				{
					p->m_handler[i].FuncPrepare(Msg.MessageData[0], Msg.MessageData[1], p->m_handler[i].pContext);
				}
				if (p->m_handler[i].FuncHandle != NULL)
				{
					p->m_handler[i].FuncHandle(Msg.MessageData[0], Msg.MessageData[1], p->m_handler[i].pContext);
				}
				if (p->m_handler[i].FuncReturn != NULL)
				{
                    p->m_handler[i].FuncReturn(Msg.MessageData[0], Msg.MessageData[1], p->m_handler[i].pContext);
				}

				break;
			}
		}

		DVR::OSA_mutexUnlock(&p->m_mutexHndl);
	}
}

RecorderAsyncOp::RecorderAsyncOp()
{
	DVR_S32 res;

	/*Create App Message Queue*/
    strcpy(m_msgQueue.name,"RecorderAsyncOp");
	res = DVR::OSA_msgqCreate(&m_msgQueue, MsgPool, sizeof(APP_MESSAGE), RECORDER_ASYNC_OP_MSGQUEUE_SIZE);
	if (DVR_FAILED(res))
	{
		DPrint(DPRINT_ERR, "RecorderAsyncOp <Init> Create Queue failed = 0x%x\n", res);
	}

	res = DVR::OSA_tskCreate(&m_hThread, RecorderAsyncOp_MgrTask, this);
	if (DVR_FAILED(res))
	{
		DPrint(DPRINT_ERR, "RecorderAsyncOp <Init> Create Task failed = 0x%x\n", res);
	}

	/*Create Mutex*/
	res = DVR::OSA_mutexCreate(&m_mutexHndl);
	if (DVR_FAILED(res))
	{
		DPrint(DPRINT_ERR, "RecorderAsyncOp <Init> Create Mutex failed = 0x%x\n", res);
	}

	m_asyncOpHandlerCount = 0;
	memset(m_handler, 0, sizeof(m_handler));
}

RecorderAsyncOp::~RecorderAsyncOp()
{
	DVR::OSA_msgqDelete(&m_msgQueue);
	DVR::OSA_tskDelete(&m_hThread);
	DVR::OSA_mutexDelete(&m_mutexHndl);
}

int RecorderAsyncOp::RecorderAsyncOp_RcvMsg(APP_MESSAGE *msg, DVR_U32 waitOption)
{
	int res;

	res = DVR::OSA_msgqRecv(&m_msgQueue, (void *)msg, waitOption);
	if (DVR_FAILED(res))
	{
		DPrint(DPRINT_ERR, "RecorderAsyncOp <RcvMsg> OSA_msgqRecv failed = 0x%x\n", res);
	}

	return res;
}


int RecorderAsyncOp::RecorderAsyncOp_SndMsg(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2)
{
	int res = 0;
	APP_MESSAGE TempMessage = { 0 };

	TempMessage.MessageID = msg;
	TempMessage.MessageData[0] = param1;
	TempMessage.MessageData[1] = param2;

	res = DVR::OSA_msgqSend(&m_msgQueue, &TempMessage, DVR_TIMEOUT_NO_WAIT);
	if (DVR_FAILED(res))
	{
		DPrint(DPRINT_ERR, "RecorderAsyncOp <SndMsg> OSA_msgqRecv failed = 0x%x\n", res);
	}

	return res;
}

int RecorderAsyncOp::RecorderAsyncOp_FlushMsg(void)
{
	int res = 0;
	res = DVR::OSA_msgqFlush(&m_msgQueue);
	if (DVR_FAILED(res))
	{
		DPrint(DPRINT_ERR, "RecorderAsyncOp <FlushMsg> OSA_msgqFlush failed = 0x%x\n", res);
	}

	return res;
}
int RecorderAsyncOp::RecorderAsyncOp_RegHandler(RECORDER_ASYNCOP_HANDLER *handler)
{
	int res = 0;
	if (m_asyncOpHandlerCount < RECORDER_MAX_ASYNC_OP_HANDLER) {
		memcpy(&m_handler[m_asyncOpHandlerCount], handler, sizeof(RECORDER_ASYNCOP_HANDLER));
		m_asyncOpHandlerCount++;
	}
	else {
		DPrint(DPRINT_ERR, "[RecorderAsyncOp] <RegHandler> Register Handler Error, Handler runout\n");
		res = -1;
	}
	return res;
}