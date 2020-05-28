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
#ifndef _DVR_STORAGE_ASYNC_OP_H_
#define _DVR_STORAGE_ASYNC_OP_H_

#include "DVR_SDK_INTFS.h"
#include "osa.h"
#include "osa_mutex.h"
#include "osa_tsk.h"
#include "osa_msgq.h"

#define ASYNC_OP_MGR_MSGQUEUE_SIZE		(128)
#define APPLIB_MAX_STORAGE_OP_HANDLER   (4)

class DvrStorageAsyncOp
{
public:
	DvrStorageAsyncOp(void);
	~DvrStorageAsyncOp(void);

	int DvrStorageAsyncOp_RegHandler(DVR_STORAGE_HANDLER *handler);
    int DvrStorageAsyncOp_ClrHandler(void);
	int DvrStorageAsyncOp_SndMsg(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2);

private:
	DISABLE_COPY(DvrStorageAsyncOp)

	static void DvrStorageAsyncOp_MgrTask(void *ptr);
	int DvrStorageAsyncOp_RcvMsg(APP_MESSAGE *msg, DVR_U32 waitOption);

	DVR_U8 MsgPool[sizeof(APP_MESSAGE)*ASYNC_OP_MGR_MSGQUEUE_SIZE];
	OSA_TskHndl m_hThread;
	OSA_MsgqHndl m_msgQueue;
	DVR_STORAGE_HANDLER m_handler[APPLIB_MAX_STORAGE_OP_HANDLER];
	int m_asyncOpHandlerCount;
};

#endif /* _DVR_STORAGE_ASYNC_OP_H_ */
