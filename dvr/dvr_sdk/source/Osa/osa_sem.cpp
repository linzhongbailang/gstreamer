
#include <osa_sem.h>

DVR_S32 DVR::OSA_semCreate(OSA_SemHndl *hndl, DVR_U32 maxCount, DVR_U32 initVal)
{
	pthread_mutexattr_t mutex_attr;
	pthread_condattr_t cond_attr;
	int status = DVR_RES_SOK;

	status |= pthread_mutexattr_init(&mutex_attr);
	status |= pthread_condattr_init(&cond_attr);

	status |= pthread_mutex_init(&hndl->lock, &mutex_attr);
	status |= pthread_cond_init(&hndl->cond, &cond_attr);

	hndl->count = initVal;
	hndl->maxCount = maxCount;

	if (hndl->maxCount == 0)
		hndl->maxCount = 1;

	if (hndl->count > hndl->maxCount)
		hndl->count = hndl->maxCount;

	if (status != DVR_RES_SOK)
		DPrint(DPRINT_ERR, "OSA_semCreate() = %d \r\n", status);

	pthread_condattr_destroy(&cond_attr);
	pthread_mutexattr_destroy(&mutex_attr);

	return status;
}

DVR_S32 DVR::OSA_semWait(OSA_SemHndl *hndl, DVR_U32 timeout)
{
	int status = DVR_RES_EFAIL;

	pthread_mutex_lock(&hndl->lock);

	while (1) {
		if (hndl->count > 0) {
			hndl->count--;
			status = DVR_RES_SOK;
			break;
		}
		else {
			if (timeout == DVR_TIMEOUT_NO_WAIT)
				break;

			pthread_cond_wait(&hndl->cond, &hndl->lock);
		}
	}

	pthread_mutex_unlock(&hndl->lock);

	return status;
}

DVR_S32 DVR::OSA_semSignal(OSA_SemHndl *hndl)
{
	int status = DVR_RES_SOK;

	pthread_mutex_lock(&hndl->lock);

	if (hndl->count < hndl->maxCount) {
		hndl->count++;
		status |= pthread_cond_signal(&hndl->cond);
	}

	pthread_mutex_unlock(&hndl->lock);

	return status;
}

DVR_S32 DVR::OSA_semDelete(OSA_SemHndl *hndl)
{
	pthread_cond_destroy(&hndl->cond);
	pthread_mutex_destroy(&hndl->lock);

	return DVR_RES_SOK;
}


