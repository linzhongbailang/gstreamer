#include <osa_thr.h>

DVR_S32 DVR::OSA_thrCreate(OSA_ThrHndl *hndl, OSA_ThrEntryFunc entryFunc, void *prm)
{
	int status = DVR_RES_SOK;

	if (entryFunc == NULL)
		return DVR_RES_EFAIL;

	status = pthread_create(&hndl->hndl, NULL, entryFunc, prm);
	if (status != 0) {
		DPrint(DPRINT_ERR, "OSA_thrCreate() - Could not create thread [%d]\n", status);
	}

	return status;
}

DVR_S32 DVR::OSA_thrJoin(OSA_ThrHndl *hndl)
{
	int status = DVR_RES_SOK;
	void *returnVal;

	status = pthread_join(hndl->hndl, &returnVal);

	return status;
}

DVR_S32 DVR::OSA_thrDelete(OSA_ThrHndl *hndl)
{
	int status = DVR_RES_SOK;

	if (&hndl->hndl != NULL)
	{
		status = pthread_cancel(hndl->hndl);
		status |= DVR::OSA_thrJoin(hndl);
	}

	return status;
}

DVR_S32 DVR::OSA_thrExit(void)
{
	pthread_exit(0);
	return DVR_RES_SOK;
}

