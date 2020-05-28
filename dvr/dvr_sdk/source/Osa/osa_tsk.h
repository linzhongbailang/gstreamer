
#ifndef _OSA_TSK_H_
#define _OSA_TSK_H_

#include "osa_thr.h"


typedef void(*OSA_TskFncMain)(void *ptr);

/**
  \brief Task Handle
*/
typedef struct OSA_TskHndl {
	OSA_ThrHndl thrHndl;      ///< OS thread handle

	OSA_TskFncMain fncMain;   ///< Task Main, this function is entered when a message is received by the process
  
	void *appData;
    
} OSA_TskHndl;

namespace DVR
{
	DVR_S32 OSA_tskCreate(OSA_TskHndl *pPrc, OSA_TskFncMain fncMain, void *appData);
	DVR_S32 OSA_tskDelete(OSA_TskHndl *pTsk);	
    DVR_S32 OSA_tskExit(void);
}

#endif /* _OSA_TSK_H_ */


