
#ifndef _OSA_THR_H_
#define _OSA_THR_H_

#include <osa.h>

typedef void * (*OSA_ThrEntryFunc)(void *);

typedef struct
{
	pthread_t      hndl;  
} OSA_ThrHndl;

namespace DVR
{
	DVR_S32 OSA_thrCreate(OSA_ThrHndl *hndl, OSA_ThrEntryFunc entryFunc, void *prm);
	DVR_S32 OSA_thrDelete(OSA_ThrHndl *hndl);
	DVR_S32 OSA_thrJoin(OSA_ThrHndl *hndl);
	DVR_S32 OSA_thrExit(void );
}

#endif /* _OSA_THR_H_ */



