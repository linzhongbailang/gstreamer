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

#ifndef _DVR_NOTIFIER_H_
#define _DVR_NOTIFIER_H_

#include <windows.h>
#include "osa.h"
#include "osa_tsk.h"
#include "osa_msgq.h"

#ifndef DVR_MAX_NOTIFY
#define DVR_MAX_NOTIFY    16
#endif

#ifndef MAX_MSG_NUMBER
#define MAX_MSG_NUMBER      64
#endif

typedef struct
{
	long	lEventCode;
	void	*pParam1;
	void	*pParam2;
} MsgContent;

class OsaNotifier
{
public:
	OsaNotifier();
	~OsaNotifier();

	int Open();
	int Close();


	static int SendNotification(int lEvent, void *pParam1, long lSize1, void *pParam2, long lSize2, void *pContext);
	static int PostNotification(int lEvent, void *pParam1, long lSize1, void *pParam2, long lSize2, void *pContext);
	int Prepend(PFN_DVR_SDK_NOTIFY pNotify, void *pContext);
	int Register(PFN_DVR_SDK_NOTIFY pNotify, void *pContext);
	int UnRegister(PFN_DVR_SDK_NOTIFY pNotify, void *pContext);

	typedef int(*PFN_ASYNC_NOTIFY) (int enuType, void *pParam1, long lSize1, void *pParam2, long lSize2, void *pContext);

private:
	DISABLE_COPY(OsaNotifier)

	static void RetrieveThread(void *pArg);
	long WaitRetreiveMsg();

	typedef struct NotifyCB_
	{
		PFN_DVR_SDK_NOTIFY    pNotifyCB;
		void                *pContext;
	} NotifyCB;

	NotifyCB m_NotifyArr[DVR_MAX_NOTIFY];
	int      m_nNotifyNum;

	DVR_U8 MsgPool[sizeof(MsgContent)*MAX_MSG_NUMBER];
	OSA_TskHndl m_hThread;
	OSA_MsgqHndl m_msgQueue;
};

#endif
