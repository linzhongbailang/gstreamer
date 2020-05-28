

#ifndef _OSA_SEM_H_
#define _OSA_SEM_H_

#include <osa.h>

typedef struct {

  DVR_U32 count;
  DVR_U32 maxCount;
  pthread_mutex_t lock;
  pthread_cond_t  cond;

} OSA_SemHndl;

namespace DVR
{
	// sem
	DVR_S32 OSA_semCreate(OSA_SemHndl *hndl, DVR_U32 maxCount, DVR_U32 initVal);
	DVR_S32 OSA_semWait(OSA_SemHndl *hndl, DVR_U32 timeout);
	DVR_S32 OSA_semSignal(OSA_SemHndl *hndl);
	DVR_S32 OSA_semDelete(OSA_SemHndl *hndl);
}

#endif /* _OSA_FLG_H_ */



