

#ifndef _OSA_MSGQ_H_
#define _OSA_MSGQ_H_

#include <osa.h>

typedef struct {

	DVR_U32 curRd;
	DVR_U32 curWr;
	DVR_U32 len;
	DVR_U32 count;
	DVR_U32 msgSize;

	DVR_U8 *queue;
	char 	name[128];

	pthread_mutex_t lock;
	pthread_cond_t  condRd;
	pthread_cond_t  condWr;
} OSA_MsgqHndl;

namespace DVR
{
	DVR_S32 OSA_msgqCreate(OSA_MsgqHndl *pMsgQueue, void *pMsgQueueBase, DVR_U32 MsgSize, DVR_U32 MaxNumMsg);
	DVR_S32 OSA_msgqDelete(OSA_MsgqHndl *pMsgQueue);
	DVR_S32 OSA_msgqSend(OSA_MsgqHndl *pMsgQueue, void *pMsgSource, DVR_U32 Timeout);
	DVR_S32 OSA_msgqRecv(OSA_MsgqHndl *pMsgQueue, void *pMsgDest, DVR_U32 Timeout);
	DVR_S32 OSA_msgqFlush(OSA_MsgqHndl *pMsgQueue);
	DVR_S32 OSA_msgqQuery(OSA_MsgqHndl *pMsgQueue, DVR_U32 *pCurCount, DVR_U32 *pCurRemainedSpace);
}

#endif /* _OSA_FLG_H_ */



