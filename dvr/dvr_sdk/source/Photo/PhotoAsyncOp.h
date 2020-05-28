
#ifndef _PHOTO_ASYNCOP_H_
#define _PHOTO_ASYNCOP_H_

#include "osa.h"
#include "osa_mutex.h"
#include "osa_tsk.h"
#include "osa_msgq.h"
#include "DVR_SDK_DEF.h"

#define PHOTO_ASYNC_OP_MSGQUEUE_SIZE		(128)
#define PHOTO_MAX_ASYNC_OP_HANDLER			(4)

enum
{
	PHOTO_ASYNCOP_CMD_TAKE_PHOTO,
	PHOTO_ASYNCOP_CMD_NUM
};

typedef struct
{
	int(*FuncPrepare) (DVR_U32 param1, DVR_U32 param2, void *pContext);/**< Prepare CB function */
	int(*FuncHandle) (DVR_U32 param1, DVR_U32 param2, void *pContext);/**< Handle CB function */
	int(*FuncReturn) (DVR_U32 param1, DVR_U32 param2, void *pContext);/**< Return CB Function */
	DVR_U32 Command;/**< Function Set corresponding Command */
	void *pContext;
}PHOTO_ASYNCOP_HANDLER;

class CPhotoAsyncOp
{
public:
	CPhotoAsyncOp(void);
	~CPhotoAsyncOp(void);

	int AsyncOp_SndMsg(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2);
	int AsyncOp_FlushMsg(void);
	int AsyncOp_RegHandler(PHOTO_ASYNCOP_HANDLER *handler);

private:

	static void PhotoAsyncOp_MgrTask(void *ptr);

	int AsyncOp_RcvMsg(APP_MESSAGE *msg, DVR_U32 waitOption);

	DVR_U8 MsgPool[sizeof(APP_MESSAGE)*PHOTO_ASYNC_OP_MSGQUEUE_SIZE];
	OSA_MutexHndl m_mutexHndl;
	OSA_TskHndl m_hThread;
	OSA_MsgqHndl m_msgQueue;
	PHOTO_ASYNCOP_HANDLER m_handler[PHOTO_MAX_ASYNC_OP_HANDLER];
	int m_asyncOpHandlerCount;
};


#endif
