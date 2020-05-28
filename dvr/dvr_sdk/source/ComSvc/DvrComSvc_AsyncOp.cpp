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
#ifdef _MSC_VER
#pragma warning(disable:4996)	// 4706: assignment within conditional expression
#endif

#include "DvrComSvc_AsyncOp.h"

#ifdef __linux__
#include "DvrMonitor_Mount.h"
#endif

void DvrComSvcAsyncOp::DvrComSvcAsyncOp_MgrTask(void *ptr)
{
	int res = 0;
	DVR_U32 Param1 = 0, Param2 = 0;
	APP_MESSAGE Msg = { 0 };
	int ReturnInfo = 0;
	DvrComSvcAsyncOp *p = (DvrComSvcAsyncOp *)ptr;

	while (1)
	{
		p->DvrComSvcAsyncOp_RcvMsg(&Msg, DVR_TIMEOUT_WAIT_FOREVER);
		Param1 = Msg.MessageData[0];
		Param2 = Msg.MessageData[1];
		DPrint(DPRINT_LOG, "[Applib - ComSvcAsyncOp] <MgrTask> Received msg: 0x%X (Param1 = 0x%X / Param2 = 0x%X)\n", Msg.MessageID, Param1, Param2);

		switch (Msg.MessageID)
		{
		case ASYNC_MGR_CMD_CARD_FORMAT:
		{
		    
            p->m_sysCtrl->FileMapDB_Clear();
            p->m_sysCtrl->DB_Unmount(p->m_sysCtrl->CurrentMountPoint());
            ReturnInfo = 0;
			res = DVR::OSA_CardFormat(p->m_sysCtrl->CurrentDevicePath(), p->m_sysCtrl->CurrentMountPoint());
			if(res == DVR_RES_SOK && p->m_sysCtrl->CurrentMountPoint() == NULL)
			{
			    ReturnInfo = 1;
				//for error format card
				DVR_DEVICE_EVENT deviceEvent = DVR_DEVICE_EVENT_PLUGIN;
				DVR_DEVICE device;
				memset(&device, 0, sizeof(device));
                DVR_BOOL isExit = TRUE;
#ifdef __linux__
                if(access(p->m_sysCtrl->CurrentDevicePath(), F_OK) == 0)
                {
                    isExit = TRUE;
                    p->m_sysCtrl->GetMountMonitor()->SetMountStatus(TRUE);
                }
                else
                    isExit = FALSE;
                
#endif
                printf("rec ASYNC_MGR_CMD_CARD_FORMAT  isexit %d\n",isExit);
                if(isExit)
                {
                    device.eType = DVR_DEVICE_TYPE_USBHD;
                    strcpy(device.szMountPoint, SD_MOUNT_POINT);
                    strncpy(device.szDevicePath, p->m_sysCtrl->CurrentDevicePath(), PATH_MAX);
                    OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_DEVICEEVENT, (void *)&deviceEvent, (long)sizeof(deviceEvent),
                        (void *)&device, (long)sizeof(device), DvrSystemControl::Notifier());
                }
			}
		}
		break;

		case ASYNC_MGR_CMD_FILE_COPY:
		{
			unsigned id = 0;
			char *szSrcFullName = p->m_pAsyncFileOpItems[Param1].SrcFn;
			char *szDstFolder = p->m_pAsyncFileOpItems[Param1].DstFn;
			char *slash;

			std::string dst_file_location;
			char *szSrcFileName = (slash = strrchr(szSrcFullName, '/')) ? (slash + 1) : szSrcFullName;

			dst_file_location = szDstFolder;
			dst_file_location = dst_file_location + szSrcFileName;

			res = DVR::OSA_FileCopy(szSrcFullName, (char *)dst_file_location.data());
			memset(&p->m_pAsyncFileOpItems[Param1], 0, sizeof(ASYNC_MGR_FILE_OP_ITEM));
		}
		break;

		case ASYNC_MGR_CMD_FILE_MOVE:
			break;

		case ASYNC_MGR_CMD_FILE_DEL:
		{
			DVR_FILE_TYPE fileType;
			unsigned id = 0;
            unsigned folderType;
			char *szFullName = p->m_pAsyncFileOpItems[Param1].SrcFn;

            res = p->m_sysCtrl->FileMapDB_GetTypeByName(p->m_sysCtrl->CurrentMountPoint(), szFullName, folderType);
			if (DVR_FAILED(res))
			{
				DPrint(DPRINT_ERR, "DvrComSvcAsyncOp_MgrTask::ASYNC_MGR_CMD_FILE_DEL(): Can not find %s\n", szFullName);
				break;
			}

			res = DVR::OSA_FileDel(szFullName);
			DPrint(DPRINT_INFO, "DvrComSvcAsyncOp_MgrTask::ASYNC_MGR_CMD_FILE_DEL(): delete %s, res=0x%x\n", szFullName,res);
			memset(&p->m_pAsyncFileOpItems[Param1], 0, sizeof(ASYNC_MGR_FILE_OP_ITEM));
		}
		break;

		default:
			break;
		}

		DPrint(DPRINT_LOG, "[Applib - ComSvcAsyncOp] <MgrTask> msg 0x%X is done (res = %d / ReturnInfo = %d)\n", Msg.MessageID, res, ReturnInfo);

		p->m_sysCtrl->ComSvcHcmgr_SendMsg(ASYNC_MGR_MSG_OP_DONE(Msg.MessageID), res, ReturnInfo);
	}
}


DvrComSvcAsyncOp::DvrComSvcAsyncOp(void *sysCtrl)
{
	DVR_S32 res;

	/*Create App Message Queue*/
    strcpy(m_msgQueue.name,"DvrComSvcAsyncOp");
	res = DVR::OSA_msgqCreate(&m_msgQueue, MsgPool, sizeof(APP_MESSAGE), ASYNC_MGR_MSGQUEUE_SIZE);
	if (DVR_FAILED(res))
	{
		DPrint(DPRINT_ERR, "DvrComSvcAsyncOp <Init> Create Queue failed = 0x%x\n", res);
	}

	res = DVR::OSA_tskCreate(&m_hThread, DvrComSvcAsyncOp_MgrTask, this);
	if (DVR_FAILED(res))
	{
		DPrint(DPRINT_ERR, "DvrComSvcAsyncOp <Init> Create Task failed = 0x%x\n", res);
	}

	m_pAsyncFileOpItems = (ASYNC_MGR_FILE_OP_ITEM *)malloc(ASYNC_FILE_OP_MAX*sizeof(ASYNC_MGR_FILE_OP_ITEM));
	if (NULL == m_pAsyncFileOpItems)
	{
		DPrint(DPRINT_ERR, "DvrComSvcAsyncOp <Init> Create ASYNC_MGR_FILE_OP_ITEM failed\n");
	}
	else
	{
		memset(m_pAsyncFileOpItems, 0, ASYNC_FILE_OP_MAX*sizeof(ASYNC_MGR_FILE_OP_ITEM));
	}

	m_sysCtrl = (DvrSystemControl *)sysCtrl;
}

DvrComSvcAsyncOp::~DvrComSvcAsyncOp()
{
	DVR::OSA_msgqDelete(&m_msgQueue);
	DVR::OSA_tskDelete(&m_hThread);

	if (m_pAsyncFileOpItems)
	{
		free(m_pAsyncFileOpItems);
		m_pAsyncFileOpItems = NULL;
	}
}

int DvrComSvcAsyncOp::DvrComSvcAsyncOp_CardFormat(void)
{
	return DvrComSvcAsyncOp_SndMsg(ASYNC_MGR_CMD_CARD_FORMAT, 0, 0);
}

int DvrComSvcAsyncOp::DvrComSvcAsyncOp_FileCopy(char *srcFn, char *dstFn)
{
	int ReturnValue = -1;
	ReturnValue = DvrComSvcAsyncOp_FileOpGetEmptyItem();
	if (ReturnValue >= 0) {
		strcpy(m_pAsyncFileOpItems[ReturnValue].SrcFn, srcFn);
		strcpy(m_pAsyncFileOpItems[ReturnValue].DstFn, dstFn);
		DvrComSvcAsyncOp_SndMsg(ASYNC_MGR_CMD_FILE_COPY, ReturnValue, 0);
		ReturnValue = DVR_RES_SOK;
	}
	return ReturnValue;
}

int DvrComSvcAsyncOp::DvrComSvcAsyncOp_FileMove(char *srcFn, char *dstFn)
{
	int ReturnValue = -1;
	ReturnValue = DvrComSvcAsyncOp_FileOpGetEmptyItem();
	if (ReturnValue >= 0) {
		strcpy(m_pAsyncFileOpItems[ReturnValue].SrcFn, srcFn);
		strcpy(m_pAsyncFileOpItems[ReturnValue].DstFn, dstFn);
		DvrComSvcAsyncOp_SndMsg(ASYNC_MGR_CMD_FILE_MOVE, ReturnValue, 0);
		ReturnValue = DVR_RES_SOK;
	}
	return ReturnValue;
}

int DvrComSvcAsyncOp::DvrComSvcAsyncOp_FileDel(char *filename)
{
	int ReturnValue = -1;
	ReturnValue = DvrComSvcAsyncOp_FileOpGetEmptyItem();
	if (ReturnValue >= 0) {
		strcpy(m_pAsyncFileOpItems[ReturnValue].SrcFn, filename);
		DvrComSvcAsyncOp_SndMsg(ASYNC_MGR_CMD_FILE_DEL, ReturnValue, 0);
		ReturnValue = DVR_RES_SOK;
	}
	return ReturnValue;
}

int DvrComSvcAsyncOp::DvrComSvcAsyncOp_FileOpGetEmptyItem(void)
{
	int i = 0;
	for (i = 0; i < ASYNC_FILE_OP_MAX; i++) {
		if (m_pAsyncFileOpItems[i].SrcFn[0] == '\0') {
			return i;
		}
	}
	DPrint(DPRINT_ERR, "[Applib - ComSvcAsyncOp] <FileOpGetEmptyItem> No empty file op item found\n");
	return -1;
}

int DvrComSvcAsyncOp::DvrComSvcAsyncOp_RcvMsg(APP_MESSAGE *msg, DVR_U32 waitOption)
{
	int res;

	res = DVR::OSA_msgqRecv(&m_msgQueue, (void *)msg, waitOption);
	if (DVR_FAILED(res))
	{
		DPrint(DPRINT_ERR, "DvrComSvcAsyncOp <RcvMsg> OSA_msgqRecv failed = 0x%x\n", res);
	}

	return res;
}

int DvrComSvcAsyncOp::DvrComSvcAsyncOp_SndMsg(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2)
{
	int res = 0;
	APP_MESSAGE TempMessage = { 0 };

	TempMessage.MessageID = msg;
	TempMessage.MessageData[0] = param1;
	TempMessage.MessageData[1] = param2;

	res = DVR::OSA_msgqSend(&m_msgQueue, &TempMessage, DVR_TIMEOUT_NO_WAIT);
	if (DVR_FAILED(res))
	{
		DPrint(DPRINT_ERR, "DvrComSvcAsyncOp <SndMsg> OSA_msgqRecv failed = 0x%x\n", res);
	}

	return res;
}
