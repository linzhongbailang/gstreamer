#include <glib/gstdio.h>
#include <osa_msgq.h>

DVR_S32 DVR::OSA_msgqCreate(OSA_MsgqHndl *pMsgQueue, void *pMsgQueueBase, DVR_U32 MsgSize, DVR_U32 MaxNumMsg)
{
	pthread_mutexattr_t mutex_attr;
	pthread_condattr_t cond_attr;
	int status = DVR_RES_SOK;

	status |= pthread_mutexattr_init(&mutex_attr);
	status |= pthread_condattr_init(&cond_attr);

	status |= pthread_mutex_init(&pMsgQueue->lock, &mutex_attr);
	status |= pthread_cond_init(&pMsgQueue->condRd, &cond_attr);
	status |= pthread_cond_init(&pMsgQueue->condWr, &cond_attr);

	pMsgQueue->curRd = pMsgQueue->curWr = 0;
	pMsgQueue->count = 0;
	pMsgQueue->msgSize = MsgSize;
	pMsgQueue->len = MaxNumMsg;
	pMsgQueue->queue = (DVR_U8*)pMsgQueueBase;

	if (status != DVR_RES_SOK)
		DPrint(DPRINT_ERR, "OSA_msgqCreate() = %d \r\n", status);

	pthread_condattr_destroy(&cond_attr);
	pthread_mutexattr_destroy(&mutex_attr);

	return status;
}

DVR_S32 DVR::OSA_msgqDelete(OSA_MsgqHndl *pMsgQueue)
{
	pthread_cond_destroy(&pMsgQueue->condRd);
	pthread_cond_destroy(&pMsgQueue->condWr);
	pthread_mutex_destroy(&pMsgQueue->lock);

	return DVR_RES_SOK;
}

DVR_S32 DVR::OSA_msgqSend(OSA_MsgqHndl *pMsgQueue, void *pMsgSource, DVR_U32 Timeout)
{
	int status = DVR_RES_EFAIL;

	pthread_mutex_lock(&pMsgQueue->lock);

	while (1) {
		if (pMsgQueue->count < pMsgQueue->len) {
			memcpy(pMsgQueue->queue + pMsgQueue->curWr * pMsgQueue->msgSize, pMsgSource, pMsgQueue->msgSize);
			pMsgQueue->curWr = (pMsgQueue->curWr + 1) % pMsgQueue->len;
			pMsgQueue->count++;
			status = DVR_RES_SOK;
			pthread_cond_signal(&pMsgQueue->condRd);
			break;
		}
		else {
			if (Timeout == DVR_TIMEOUT_NO_WAIT)
            {
                static int flag = 0;
                g_print("OSA_msgqSend----name %s timeout %d\n",pMsgQueue->name, pMsgQueue->count);
                if(flag == 0)
                {
                    for(int i = 0;i < pMsgQueue->count;i++)
                    {
                        APP_MESSAGE* msg = (APP_MESSAGE*)(pMsgQueue->queue + i * pMsgQueue->msgSize);
                        g_print("OSA_msgqSend----name %s (%d) 0x%x\n",pMsgQueue->name, i, msg->MessageID);
                    }
                    flag = 1;
                }
                break;
            }

			status = pthread_cond_wait(&pMsgQueue->condWr, &pMsgQueue->lock);
		}
	}

	pthread_mutex_unlock(&pMsgQueue->lock);

	return status;
}

DVR_S32 DVR::OSA_msgqRecv(OSA_MsgqHndl *pMsgQueue, void *pMsgDest, DVR_U32 Timeout)
{
	int status = DVR_RES_EFAIL;

	pthread_mutex_lock(&pMsgQueue->lock);

	while (1) {
		if (pMsgQueue->count > 0) {

			if (pMsgDest != NULL) {
				memcpy(pMsgDest, pMsgQueue->queue + pMsgQueue->curRd * pMsgQueue->msgSize, pMsgQueue->msgSize);
			}

			pMsgQueue->curRd = (pMsgQueue->curRd + 1) % pMsgQueue->len;
			pMsgQueue->count--;
			status = DVR_RES_SOK;
			pthread_cond_signal(&pMsgQueue->condWr);
			break;
		}
		else {
			if (Timeout == DVR_TIMEOUT_NO_WAIT)
				break;
			status = pthread_cond_wait(&pMsgQueue->condRd, &pMsgQueue->lock);
		}
	}

	pthread_mutex_unlock(&pMsgQueue->lock);

	return status;
}

DVR_S32 DVR::OSA_msgqFlush(OSA_MsgqHndl *pMsgQueue)
{
	int status = DVR_RES_SOK;

	pthread_mutex_lock(&pMsgQueue->lock);

	pMsgQueue->curRd = 0;
	pMsgQueue->curWr = 0;
	pMsgQueue->count = 0;

	pthread_mutex_unlock(&pMsgQueue->lock);

	return status;
}

DVR_S32 DVR::OSA_msgqQuery(OSA_MsgqHndl *pMsgQueue, DVR_U32 *pCurCount, DVR_U32 *pCurRemainedSpace)
{
	int status = DVR_RES_SOK;

	pthread_mutex_lock(&pMsgQueue->lock);

	if (pCurCount)
		*pCurCount = pMsgQueue->count;

	if (pCurRemainedSpace)
		*pCurRemainedSpace = pMsgQueue->len - pMsgQueue->count;

	pthread_mutex_unlock(&pMsgQueue->lock);

	return status;
}


