

#include "osa_mutex.h"

int DVR::OSA_mutexCreate(OSA_MutexHndl *hndl)
{
	pthread_mutexattr_t mutex_attr;
	int status = DVR_RES_SOK;

	status |= pthread_mutexattr_init(&mutex_attr);
	status |= pthread_mutex_init(&hndl->lock, &mutex_attr);

	if (status != DVR_RES_SOK)
		DPrint(DPRINT_ERR, "OSA_mutexCreate() = %d \r\n", status);

	pthread_mutexattr_destroy(&mutex_attr);

	return status;
}

int DVR::OSA_mutexDelete(OSA_MutexHndl *hndl)
{
	pthread_mutex_destroy(&hndl->lock);

	return DVR_RES_SOK;
}

int DVR::OSA_mutexLock(OSA_MutexHndl *hndl)
{
	return pthread_mutex_lock(&hndl->lock);
}

int DVR::OSA_mutexUnlock(OSA_MutexHndl *hndl)
{
	return pthread_mutex_unlock(&hndl->lock);
}


