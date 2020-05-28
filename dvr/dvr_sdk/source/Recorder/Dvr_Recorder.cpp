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

#include <stdlib.h>
#include <string.h>

#include "DVR_RECORDER_INTFS.h"
#include "RecorderEngine.h"

struct RECORDER_SDK
{
public:
	RECORDER_SDK()
	{
		g_mutex_init(&m_MutexLock);
		m_pEngine = new CRecorderEngine();
		memset(&m_ptfnCallBackArray, 0, sizeof(DVR_RECORDER_CALLBACK_ARRAY));
	}

	~RECORDER_SDK()
	{
		g_mutex_clear(&m_MutexLock);
		delete m_pEngine;
		memset(&m_ptfnCallBackArray, 0, sizeof(DVR_RECORDER_CALLBACK_ARRAY));
	}
	
	CRecorderEngine*  m_pEngine;
	DVR_RECORDER_CALLBACK_ARRAY  m_ptfnCallBackArray;
	GMutex  m_MutexLock;
};

#define CHECK_RECORDER_SDK_INITIALIZED \
    do {\
        if (g_pRecorderSDK == NULL)\
            return DVR_RES_EUNEXPECTED;\
    } while(0)

#define CHECK_RECORDER_SDK_HANDLE_VALID(hRecorder) \
    do {\
        if (g_pRecorderSDK != ((hRecorder)))\
            return DVR_RES_EINVALIDARG;\
    } while (0)


static RECORDER_SDK *g_pRecorderSDK = NULL;

int Dvr_Recorder_Initialize(void** phRecorder)
{
	if(g_pRecorderSDK != NULL)
		return DVR_RES_EUNEXPECTED;
	
	if (phRecorder == NULL)
		return DVR_RES_EINVALIDARG;

	g_pRecorderSDK = new RECORDER_SDK;
	if (g_pRecorderSDK == NULL)
	{
		return DVR_RES_EOUTOFMEMORY;
	}

	*phRecorder = (void *)g_pRecorderSDK;

	return DVR_RES_SOK;
}

int Dvr_Recorder_DeInitialize(void* phRecorder)
{
	CHECK_RECORDER_SDK_INITIALIZED;
	CHECK_RECORDER_SDK_HANDLE_VALID(phRecorder);
	
	delete g_pRecorderSDK;
	g_pRecorderSDK = NULL;

	return DVR_RES_SOK;
}

int Dvr_Recorder_AcquireInputBuf(void* phRecorder, void **ppvBuffer)
{
	CHECK_RECORDER_SDK_INITIALIZED;
	CHECK_RECORDER_SDK_HANDLE_VALID(phRecorder);

	int ret = DVR_RES_SOK;
	RECORDER_SDK *pRecorderSDK = (RECORDER_SDK*)phRecorder;
	if (pRecorderSDK->m_pEngine != NULL)
	{
		ret = pRecorderSDK->m_pEngine->AcquireInputBuf(ppvBuffer);
	}

	return ret;
}

int Dvr_Recorder_AddFrame(void* phRecorder, DVR_IO_FRAME *pInputFrame)
{
	CHECK_RECORDER_SDK_INITIALIZED;
	CHECK_RECORDER_SDK_HANDLE_VALID(phRecorder);

	int ret = DVR_RES_SOK;
	RECORDER_SDK *pRecorderSDK = (RECORDER_SDK*)phRecorder;
	if (pRecorderSDK->m_pEngine != NULL)
	{
		ret = pRecorderSDK->m_pEngine->AddFrame(pInputFrame);
	}

	return ret;
}

int Dvr_Recorder_AsyncOpFlush(void* phRecorder)
{
	CHECK_RECORDER_SDK_INITIALIZED;
	CHECK_RECORDER_SDK_HANDLE_VALID(phRecorder);

	int ret = DVR_RES_SOK;
	RECORDER_SDK *pRecorderSDK = (RECORDER_SDK*)phRecorder;
	if (pRecorderSDK->m_pEngine != NULL)
	{
		ret = pRecorderSDK->m_pEngine->AsyncOpFlush();
	}

	return ret;
}

int Dvr_Recorder_Start(void* phRecorder)
{
	CHECK_RECORDER_SDK_INITIALIZED;
	CHECK_RECORDER_SDK_HANDLE_VALID(phRecorder);

	int ret = DVR_RES_SOK;
	RECORDER_SDK *pRecorderSDK = (RECORDER_SDK*)phRecorder;
	if (pRecorderSDK->m_pEngine != NULL)
	{
		ret = pRecorderSDK->m_pEngine->Start();
	}

	return ret;
}

int Dvr_Recorder_LoopRec_Start(void* phRecorder)
{
	CHECK_RECORDER_SDK_INITIALIZED;
	CHECK_RECORDER_SDK_HANDLE_VALID(phRecorder);

	int ret = DVR_RES_SOK;
    RECORDER_SDK *pRecorderSDK = (RECORDER_SDK*)phRecorder;
    if (pRecorderSDK->m_pEngine != NULL)
    {
        ret = pRecorderSDK->m_pEngine->LoopRecStart();
    }

    return ret;
}

int Dvr_Recorder_LoopRec_Stop(void* phRecorder)
{
	CHECK_RECORDER_SDK_INITIALIZED;
	CHECK_RECORDER_SDK_HANDLE_VALID(phRecorder);

	int ret = DVR_RES_SOK;
    RECORDER_SDK *pRecorderSDK = (RECORDER_SDK*)phRecorder;
    if (pRecorderSDK->m_pEngine != NULL)
    {
        ret = pRecorderSDK->m_pEngine->LoopRecStop();
    }

    return ret;
}

int Dvr_Recorder_EventRec_Start(void* phRecorder)
{
	CHECK_RECORDER_SDK_INITIALIZED;
	CHECK_RECORDER_SDK_HANDLE_VALID(phRecorder);

	int ret = DVR_RES_SOK;
    RECORDER_SDK *pRecorderSDK = (RECORDER_SDK*)phRecorder;
    if (pRecorderSDK->m_pEngine != NULL)
    {
        ret = pRecorderSDK->m_pEngine->EventRecStart();
    }

    return ret;
}

int Dvr_Recorder_EventRec_Stop(void* phRecorder)
{
	CHECK_RECORDER_SDK_INITIALIZED;
	CHECK_RECORDER_SDK_HANDLE_VALID(phRecorder);

	int ret = DVR_RES_SOK;
    RECORDER_SDK *pRecorderSDK = (RECORDER_SDK*)phRecorder;
    if (pRecorderSDK->m_pEngine != NULL)
    {
        ret = pRecorderSDK->m_pEngine->EventRecStop();
    }

    return ret;
}

int Dvr_Recorder_DasRec_Start(void* phRecorder)
{
	CHECK_RECORDER_SDK_INITIALIZED;
	CHECK_RECORDER_SDK_HANDLE_VALID(phRecorder);

	int ret = DVR_RES_SOK;
    RECORDER_SDK *pRecorderSDK = (RECORDER_SDK*)phRecorder;
    if (pRecorderSDK->m_pEngine != NULL)
    {
        ret = pRecorderSDK->m_pEngine->DasRecStart();
    }

    return ret;
}

int Dvr_Recorder_DasRec_Stop(void* phRecorder)
{
	CHECK_RECORDER_SDK_INITIALIZED;
	CHECK_RECORDER_SDK_HANDLE_VALID(phRecorder);

	int ret = DVR_RES_SOK;
    RECORDER_SDK *pRecorderSDK = (RECORDER_SDK*)phRecorder;
    if (pRecorderSDK->m_pEngine != NULL)
    {
        ret = pRecorderSDK->m_pEngine->DasRecStop();
    }

    return ret;
}

int Dvr_Recorder_IaccRec_Start(void* phRecorder)
{
    CHECK_RECORDER_SDK_INITIALIZED;
    CHECK_RECORDER_SDK_HANDLE_VALID(phRecorder);

    int ret = DVR_RES_SOK;
    RECORDER_SDK *pRecorderSDK = (RECORDER_SDK*)phRecorder;
    if (pRecorderSDK->m_pEngine != NULL)
    {
        ret = pRecorderSDK->m_pEngine->IaccRecStart();
    }

    return ret;
}

int Dvr_Recorder_IaccRec_Stop(void* phRecorder)
{
    CHECK_RECORDER_SDK_INITIALIZED;
    CHECK_RECORDER_SDK_HANDLE_VALID(phRecorder);

    int ret = DVR_RES_SOK;
    RECORDER_SDK *pRecorderSDK = (RECORDER_SDK*)phRecorder;
    if (pRecorderSDK->m_pEngine != NULL)
    {
        ret = pRecorderSDK->m_pEngine->IaccRecStop();
    }

    return ret;
}

int Dvr_Recorder_Stop(void* phRecorder)
{
	CHECK_RECORDER_SDK_INITIALIZED;
	CHECK_RECORDER_SDK_HANDLE_VALID(phRecorder);

	int ret = DVR_RES_SOK;
	RECORDER_SDK *pRecorderSDK = (RECORDER_SDK*)phRecorder;
	if (pRecorderSDK->m_pEngine != NULL)
	{
		ret = pRecorderSDK->m_pEngine->Stop();
	}

	return ret;
}

int Dvr_Recorder_Reset(void* phRecorder)
{
	CHECK_RECORDER_SDK_INITIALIZED;
	CHECK_RECORDER_SDK_HANDLE_VALID(phRecorder);

	int ret = DVR_RES_SOK;
	RECORDER_SDK *pRecorderSDK = (RECORDER_SDK*)phRecorder;
	if (pRecorderSDK->m_pEngine != NULL)
	{
		ret = pRecorderSDK->m_pEngine->Reset();
	}

	return ret;
}

int Dvr_Recorder_Create(void* phRecorder)
{
	CHECK_RECORDER_SDK_INITIALIZED;
	CHECK_RECORDER_SDK_HANDLE_VALID(phRecorder);

	int ret = DVR_RES_SOK;
	RECORDER_SDK *pRecorderSDK = (RECORDER_SDK*)phRecorder;
	if (pRecorderSDK->m_pEngine != NULL)
	{
		ret = pRecorderSDK->m_pEngine->Create();
	}

	return ret;
}

int Dvr_Recorder_Destroy(void* phRecorder)
{
	CHECK_RECORDER_SDK_INITIALIZED;
	CHECK_RECORDER_SDK_HANDLE_VALID(phRecorder);

	int ret = DVR_RES_SOK;
	RECORDER_SDK *pRecorderSDK = (RECORDER_SDK*)phRecorder;
    if (pRecorderSDK->m_pEngine != NULL)
	{
		ret = pRecorderSDK->m_pEngine->Destroy();
	}

	return ret;
}

/**************************************************************************************************
* Function declare:
*	Dvr_Player_RegisterCallBack
* Description:
*	Register player callback function Return value is the error code.
* Parameters:
*	pPlayerObj	[in]:
*		Player object handle created by Dvr_Player_Initialize interface.
*	pPlayerCallback	[in]:
*		The function point of playback callback.
*	lUserData	[in]:
*		User data which will be passed with callback function
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Recorder_RegisterCallBack(void* phRecorder, DVR_RECORDER_CALLBACK_ARRAY *ptfnCallBackArray)
{
	CHECK_RECORDER_SDK_INITIALIZED;
	CHECK_RECORDER_SDK_HANDLE_VALID(phRecorder);
	
	if (ptfnCallBackArray == NULL)
	{
		return DVR_RES_EINVALIDARG;
	}
	
	int ret = DVR_RES_SOK;
	RECORDER_SDK *pRecorderSDK = (RECORDER_SDK*)phRecorder;
	
	memcpy(&pRecorderSDK->m_ptfnCallBackArray, ptfnCallBackArray, sizeof(DVR_RECORDER_CALLBACK_ARRAY));

	if(pRecorderSDK->m_pEngine != NULL)
	{
		ret = pRecorderSDK->m_pEngine->SetFuncList((void*)&pRecorderSDK->m_ptfnCallBackArray);
	}

	return ret;
}

/**************************************************************************************************
* Function declare:
*	Dvr_Recorder_SetConfig
* Description:
*	Set the specified configuration to recorder.
* Parameters:
*	phRecorder	[in]:
*		Recorder object handle created by Dvr_Recorder_Initialize interface.
*	dwCfgType	[in]:
*		The configuration type.
*	pValue		[in]:
*		The value, and it is correlative to specified configuration type.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Recorder_SetConfig(void* phRecorder, DVR_RECORDER_CFG_TYPE eCfgType, void* pvCfgData, unsigned int u32CfgDataSize)
{
	CHECK_RECORDER_SDK_INITIALIZED;
	CHECK_RECORDER_SDK_HANDLE_VALID(phRecorder);
	
	if (pvCfgData == NULL)
	{
		return DVR_RES_EINVALIDARG;
	}

	int ret = DVR_RES_SOK;
	RECORDER_SDK *pRecorderSDK = (RECORDER_SDK*)phRecorder;
    if (pRecorderSDK->m_pEngine != NULL)
	{
		ret = pRecorderSDK->m_pEngine->Set(eCfgType, 0, 0, pvCfgData, u32CfgDataSize);
	}

    return ret;
}

/**************************************************************************************************
* Function declare:
*	Dvr_Recorder_GetConfig
* Description:
*	Set the specified configuration to recorder.
* Parameters:
*	phRecorder	[in]:
*		Recorder object handle created by Dvr_Recorder_Initialize interface.
*	dwCfgType	[in]:
*		The configuration type.
*	pValue		[in]:
*		The value, and it is correlative to specified configuration type.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Recorder_GetConfig(void* phRecorder, DVR_RECORDER_CFG_TYPE eCfgType, void* pvCfgData, unsigned int u32CfgDataSize)
{
	CHECK_RECORDER_SDK_INITIALIZED;
	CHECK_RECORDER_SDK_HANDLE_VALID(phRecorder);

	if (pvCfgData == NULL)
	{
		return DVR_RES_EINVALIDARG;
	}

	int ret = DVR_RES_SOK;
	RECORDER_SDK *pRecorderSDK = (RECORDER_SDK*)phRecorder;
	if (pRecorderSDK->m_pEngine != NULL)
	{
		ret = pRecorderSDK->m_pEngine->Get(eCfgType, 0, 0, pvCfgData, u32CfgDataSize, 0);
	}

    return ret;
}

/**************************************************************************************************
* Function declare:
*	Dvr_Recorder_GetPosition
* Description:
*	Get the amount of bytes has been recorded.
* Parameters:
*	phRecorder		[in]:
*		Recorder object handle created by Dvr_Recorder_Initialize interface.
*	dwPosition	[out]:
*		The current recorded bytes.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Recorder_GetPosition(void* phRecorder, unsigned int* pu32Position)
{
	CHECK_RECORDER_SDK_INITIALIZED;
	CHECK_RECORDER_SDK_HANDLE_VALID(phRecorder);
	
	if (pu32Position == NULL)
	{
		return DVR_RES_EINVALIDARG;
	}

	int ret = DVR_RES_SOK;
	RECORDER_SDK *pRecorderSDK = (RECORDER_SDK*)phRecorder;
	if (pRecorderSDK->m_pEngine != NULL)
	{
		ret = pRecorderSDK->m_pEngine->GetPosition((glong *)pu32Position);
	}

	return ret;
}

/**************************************************************************************************
* Function declare:
*	Dvr_Recorder_TakePhoto
* Description:
*	Take photo.
* Parameters:
*	phRecorder		[in]:
*		Recorder object handle created by Dvr_Recorder_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Recorder_TakePhoto(void * phRecorder, DVR_PHOTO_PARAM *pParam)
{
	CHECK_RECORDER_SDK_INITIALIZED;
	CHECK_RECORDER_SDK_HANDLE_VALID(phRecorder);

	int ret = DVR_RES_SOK;
	RECORDER_SDK *pRecorderSDK = (RECORDER_SDK*)phRecorder;
	if (pRecorderSDK->m_pEngine != NULL)
	{
		ret = pRecorderSDK->m_pEngine->Photo(pParam);
	}

	return ret;
}


int Dvr_Recorder_TakeOverlay(void * phRecorder, void *canbuffer, void *osdbuffer, int cansize, int osdsize)
{
	CHECK_RECORDER_SDK_INITIALIZED;
	CHECK_RECORDER_SDK_HANDLE_VALID(phRecorder);

	int ret = DVR_RES_SOK;
	RECORDER_SDK *pRecorderSDK = (RECORDER_SDK*)phRecorder;
	if (pRecorderSDK->m_pEngine != NULL)
	{
		ret = pRecorderSDK->m_pEngine->Overlay(canbuffer, osdbuffer, cansize, osdsize);
	}

	return ret;
}

