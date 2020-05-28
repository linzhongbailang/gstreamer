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

#include <stdlib.h>
#include <string.h>
#include "osa.h"
#include "osa_notifier.h"

#define OSA_NOTIFY_EXIT		0xFF

OsaNotifier::OsaNotifier(void)
{
	m_nNotifyNum = 0;
	memset(m_NotifyArr, 0, sizeof(m_NotifyArr));
}

OsaNotifier::~OsaNotifier(void)
{
}

int OsaNotifier::Open()
{
	DVR_S32 res;

    strcpy(m_msgQueue.name,"OsaNotifier");
	res = DVR::OSA_msgqCreate(&m_msgQueue, MsgPool, sizeof(MsgContent), MAX_MSG_NUMBER);
	if (DVR_FAILED(res))
	{
		DPrint(DPRINT_ERR, "OsaNotifier <Open> Create Queue failed = 0x%x\n", res);
	}

	res = DVR::OSA_tskCreate(&m_hThread, RetrieveThread, this);
	if (DVR_FAILED(res))
	{
		DPrint(DPRINT_ERR, "OsaNotifier <Open> Create Task failed = 0x%x\n", res);
	}

	return DVR_RES_SOK;
}

int OsaNotifier::Close()
{
	PostNotification(OSA_NOTIFY_EXIT, NULL, 0, NULL, 0, this);

	DVR::OSA_msgqDelete(&m_msgQueue);
	DVR::OSA_tskDelete(&m_hThread);

	return DVR_RES_SOK;
}

int OsaNotifier::PostNotification(int lEvent, void *pParam1, long lSize1, void *pParam2, long lSize2, void *pContext)
{
	OsaNotifier *pThis = (OsaNotifier *)pContext;
	if (pThis == NULL)
		return DVR_RES_EFAIL;
	
	MsgContent Msg;
	memset(&Msg, 0, sizeof(Msg));
	Msg.lEventCode = lEvent;

	if (lSize1 > 0 && pParam1)
	{
		Msg.pParam1 = malloc(lSize1);
		memcpy(Msg.pParam1, pParam1, lSize1);
	}
	if (lSize2 > 0 && pParam2)
	{
		Msg.pParam2 = malloc(lSize2);
		memcpy(Msg.pParam2, pParam2, lSize2);
	}

	DVR::OSA_msgqSend(&pThis->m_msgQueue, &Msg, DVR_TIMEOUT_NO_WAIT);
	
	return DVR_RES_SOK;
}

int OsaNotifier::SendNotification(int lEvent, void *pParam1, long lSize1, void *pParam2, long lSize2, void *pContext)
{
	OsaNotifier *pThis = (OsaNotifier *)pContext;
	for (int i = 0; i < pThis->m_nNotifyNum; i++)
	{
		if (pThis->m_NotifyArr[i].pNotifyCB)
			pThis->m_NotifyArr[i].pNotifyCB(pThis->m_NotifyArr[i].pContext, (DVR_NOTIFICATION_TYPE)lEvent, pParam1, pParam2);
	}
	return DVR_RES_SOK;
}

int OsaNotifier::Prepend(PFN_DVR_SDK_NOTIFY pNotify, void *pContext)
{
	OsaNotifier *pThis = (OsaNotifier *)pContext;

	if (pThis->m_nNotifyNum == DVR_MAX_NOTIFY)
		return DVR_RES_EFAIL;

	for (int i = 0; i < pThis->m_nNotifyNum; i++)
		m_NotifyArr[i + 1] = m_NotifyArr[i];

	pThis->m_nNotifyNum++;

	m_NotifyArr[0].pNotifyCB = pNotify;
	m_NotifyArr[0].pContext = pContext;

	return DVR_RES_SOK;
}

int OsaNotifier::Register(PFN_DVR_SDK_NOTIFY pNotify, void *pContext)
{
	int i = 0;	
	int bHasRegister = FALSE;
	
	for (i = 0; i < DVR_MAX_NOTIFY; i++)
	{
		if (m_NotifyArr[i].pNotifyCB == pNotify && m_NotifyArr[i].pContext == pContext)
		{
			bHasRegister = TRUE;
			break;
		}
	}

	if(FALSE == bHasRegister)
	{
		for (i = 0; i < DVR_MAX_NOTIFY; i++)
		{
			if (m_NotifyArr[i].pNotifyCB == NULL)
			{
				m_NotifyArr[i].pNotifyCB = pNotify;
				m_NotifyArr[i].pContext = pContext;
				m_nNotifyNum++;
				
				break;
			}
		}
		
		if (i == DVR_MAX_NOTIFY)
			return DVR_RES_EFAIL;
	}

	return DVR_RES_SOK;
}

int OsaNotifier::UnRegister(PFN_DVR_SDK_NOTIFY pNotify, void *pContext)
{
	int i = 0;
	for (i = 0; i < DVR_MAX_NOTIFY; i++)
	{
		if (m_NotifyArr[i].pNotifyCB == pNotify && m_NotifyArr[i].pContext == pContext)
		{
			m_NotifyArr[i].pNotifyCB = NULL;
			m_NotifyArr[i].pContext = NULL;
		}
	}

	if(m_nNotifyNum >= 1)
	{
		for (i = 0; i < m_nNotifyNum - 1; i++)
		{
			if(m_NotifyArr[i].pNotifyCB == NULL)
			{
				m_NotifyArr[i] = m_NotifyArr[i + 1];
			}
		}
		
		m_nNotifyNum--;	
	}
		
	return DVR_RES_SOK;
}

void OsaNotifier::RetrieveThread(void *pArg)
{
	OsaNotifier *pThis = (OsaNotifier *)pArg;
	if (pThis != NULL)
	{
		pThis->WaitRetreiveMsg();
	}	
}

long OsaNotifier::WaitRetreiveMsg()
{
	MsgContent Msg;
	while(1)	
	{
		DVR::OSA_msgqRecv(&m_msgQueue, (void *)&Msg, DVR_TIMEOUT_WAIT_FOREVER);
		
		switch (Msg.lEventCode)
		{
		case OSA_NOTIFY_EXIT:
			return 0;
		default:
			for (int i = 0; i < m_nNotifyNum; i++)
			{
				if (m_NotifyArr[i].pNotifyCB)
					m_NotifyArr[i].pNotifyCB(m_NotifyArr[i].pContext, (DVR_NOTIFICATION_TYPE)Msg.lEventCode, Msg.pParam1, Msg.pParam2);
			}
			break;
		}
		if (Msg.pParam1)
			free(Msg.pParam1);
		if (Msg.pParam2)
			free(Msg.pParam2);
	}
	return 0;
}
