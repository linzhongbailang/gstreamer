
#ifndef _RECORD_ASYNCOP_H_
#define _RECORD_ASYNCOP_H_

#include "osa.h"
#include "osa_mutex.h"
#include "osa_tsk.h"
#include "osa_msgq.h"
#include "DVR_SDK_DEF.h"

#define RECORDER_ASYNC_OP_MSGQUEUE_SIZE		(32)
#define RECORDER_MAX_ASYNC_OP_HANDLER		(16)

enum
{
	RECORDER_ASYNCOP_CMD_GENERATE_THUMBNAIL = 0,
	RECORDER_ASYNCOP_CMD_ADD_TAG_LIST,
	RECORDER_ASYNCOP_CMD_START_LOOP_RECORDER,
	RECORDER_ASYNCOP_CMD_STOP_LOOP_RECORDER,
	RECORDER_ASYNCOP_CMD_START_EVENT_RECORDER,
	RECORDER_ASYNCOP_CMD_STOP_EVENT_RECORDER,
    RECORDER_ASYNCOP_CMD_START_DAS_RECORDER,
    RECORDER_ASYNCOP_CMD_STOP_DAS_RECORDER,
    RECORDER_ASYNCOP_CMD_START_IACC_RECORDER,
	RECORDER_ASYNCOP_CMD_STOP_IACC_RECORDER,
	RECORDER_ASYNCOP_CMD_NUM
};

typedef struct
{
	int(*FuncPrepare) (DVR_U32 param1, DVR_U32 param2, void *pContext);/**< Prepare CB function */
	int(*FuncHandle) (DVR_U32 param1, DVR_U32 param2, void *pContext);/**< Handle CB function */
    int(*FuncReturn) (DVR_U32 param1, DVR_U32 param2, void *pContext);/**< Return CB Function */
	DVR_U32 Command;/**< Function Set corresponding Command */
	void *pContext;
}RECORDER_ASYNCOP_HANDLER;

class RecorderAsyncOp
{
public:
	RecorderAsyncOp(void);
	~RecorderAsyncOp(void);

	int RecorderAsyncOp_SndMsg(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2);
	int RecorderAsyncOp_FlushMsg(void);
	int RecorderAsyncOp_RegHandler(RECORDER_ASYNCOP_HANDLER *handler);

private:

	static void RecorderAsyncOp_MgrTask(void *ptr);

	int RecorderAsyncOp_RcvMsg(APP_MESSAGE *msg, DVR_U32 waitOption);

	DVR_U8 MsgPool[sizeof(APP_MESSAGE)*RECORDER_ASYNC_OP_MSGQUEUE_SIZE];
	OSA_MutexHndl m_mutexHndl;
	OSA_TskHndl m_hThread;
	OSA_MsgqHndl m_msgQueue;
	RECORDER_ASYNCOP_HANDLER m_handler[RECORDER_MAX_ASYNC_OP_HANDLER];
	int m_asyncOpHandlerCount;
};


#endif
