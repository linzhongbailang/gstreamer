#include <stdlib.h>
#include <osa_tsk.h>

static void *OSA_tskThrMain(void *pPrm)
{
	OSA_TskHndl *pPrc;

	pPrc = (OSA_TskHndl *)pPrm;

	while (1)
	{
		if (pPrc->fncMain == NULL)
			break;
		pPrc->fncMain(pPrc->appData);
	}

	return NULL;
}

DVR_S32 DVR::OSA_tskCreate(OSA_TskHndl *pPrc, OSA_TskFncMain fncMain, void *appData)
{
	pPrc->fncMain = fncMain;
	pPrc->appData = appData;

	if (pPrc->fncMain == NULL)
		return DVR_RES_EFAIL;

	DVR::OSA_thrCreate(&pPrc->thrHndl, OSA_tskThrMain, pPrc);

	return DVR_RES_SOK;
}

DVR_S32 DVR::OSA_tskDelete(OSA_TskHndl *pPrc)
{
	pPrc->fncMain = NULL;
	DVR::OSA_thrDelete(&pPrc->thrHndl);

	return DVR_RES_SOK;
}

DVR_S32 DVR::OSA_tskExit(void)
{

	DVR::OSA_thrExit();

	return DVR_RES_SOK;
}

