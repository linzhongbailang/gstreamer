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
#ifndef _DVR_COMSVC_ASYNC_OP_H_
#define _DVR_COMSVC_ASYNC_OP_H_

#include "DVR_SDK_INTFS.h"
#include "osa.h"
#include "osa_mutex.h"
#include "osa_tsk.h"
#include "osa_msgq.h"
#include "osa_fs.h"
#include "DvrSystemControl.h"

#define ASYNC_MGR_MSGQUEUE_SIZE (16)
#define ASYNC_FILE_OP_MAX    (ASYNC_MGR_MSGQUEUE_SIZE)
#define ASYNC_FILE_LEN_MAX    (64)

typedef struct
{
	char SrcFn[ASYNC_FILE_LEN_MAX];
	char DstFn[ASYNC_FILE_LEN_MAX];
	int(*GetPartNum)(int, void *);
	int(*ProgramStatusReport)(int, void *);
}ASYNC_MGR_FILE_OP_ITEM;

class DvrComSvcAsyncOp
{
public:
	DvrComSvcAsyncOp(void *sysCtrl);
	~DvrComSvcAsyncOp(void);

	int DvrComSvcAsyncOp_CardFormat(void);
	int DvrComSvcAsyncOp_FileCopy(char *srcFn, char *dstFn);
	int DvrComSvcAsyncOp_FileMove(char *srcFn, char *dstFn);
	int DvrComSvcAsyncOp_FileDel(char *filename);

private:
	DISABLE_COPY(DvrComSvcAsyncOp)

	static void DvrComSvcAsyncOp_MgrTask(void *ptr);
	int DvrComSvcAsyncOp_FileOpGetEmptyItem(void);
	int DvrComSvcAsyncOp_RcvMsg(APP_MESSAGE *msg, DVR_U32 waitOption);
	int DvrComSvcAsyncOp_SndMsg(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2);

	DVR_U8 MsgPool[sizeof(APP_MESSAGE)*ASYNC_MGR_MSGQUEUE_SIZE];
	OSA_TskHndl m_hThread;
	OSA_MsgqHndl m_msgQueue;

	ASYNC_MGR_FILE_OP_ITEM *m_pAsyncFileOpItems;
	DvrSystemControl *m_sysCtrl;
};

#endif /* _DVR_COMSVC_ASYNC_OP_H_ */
