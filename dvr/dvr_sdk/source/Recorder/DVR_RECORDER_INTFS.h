/*******************************************************************************
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
\*********************************************************************************/
#ifndef _DVR_RECORDER_INTFS_H_
#define _DVR_RECORDER_INTFS_H_

#include "DVR_RECORDER_DEF.h"

/**************************************************************************************************
* Function declare:
*	Dvr_Recorder_Initialize
* Description: Initialize the handle of recorder object.
* Parameters:
*	phRecorder 	[in/out]:
*		The handle of recorder object. Input a pointer of handle, the returned handle value
*		will be the initialized recorder object handle.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
* Notes:
*	The initial interface must be called once ahead of other recorder interfaces, and the returned
*	recorder object handle should pass to all other implemented recorder interface.
*************************************************************************************************/
int Dvr_Recorder_Initialize(void** phRecorder);

/**************************************************************************************************
* Function declare:
*	Dvr_Recorder_DeInitialize
* Description:
*	Destroy the handle of recorder object.
* Parameters:
*	phRecorder 	[in]:
*		recorder object handle created by Dvr_Recorder_Initialize interface
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
* Notes:
*	The destroy interface must be called while exiting recorder program.
*************************************************************************************************/
int Dvr_Recorder_DeInitialize(void* phRecorder);

/**************************************************************************************************
* Function declare:
*	Dvr_Recorder_RegisterCallBack
* Description:
*	Register recorder callback function Return value is the error code.
* Parameters:
*	pPlayer		   	[in]:
*		Recorder object handle created by Dvr_Player_Initialize interface.
*	pCallbackList	[in]:
*		The Callback function pointer list.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Recorder_RegisterCallBack(void* phRecorder, DVR_RECORDER_CALLBACK_ARRAY *ptfnCallBackArray);

/**************************************************************************************************
* Function declare:
*	Dvr_Recorder_Create
* Description:
*	Allocate resource for recording.
* Parameters:
*	phRecorder		[in]:
*		recorder object handle created by Dvr_Recorder_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Recorder_Create(void* phRecorder);

/**************************************************************************************************
* Function declare:
*	Dvr_Recorder_Destroy
* Description:
*	Free resource for recording.
* Parameters:
*	phRecorder		[in]:
*		recorder object handle created by Dvr_Recorder_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Recorder_Destroy(void* phRecorder);

/**************************************************************************************************
* Function declare:
*	Dvr_Recorder_Start
* Description:
*	Start to record the stream.
* Parameters:
*	phRecorder		[in]:
*		recorder object handle created by Dvr_Recorder_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Recorder_Start(void* phRecorder);

/**************************************************************************************************
* Function declare:
*	Dvr_Recorder_LoopRec_Start
* Description:
*	Start to record the event stream.
* Parameters:
*	phRecorder		[in]:
*		recorder object handle created by Dvr_Recorder_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Recorder_LoopRec_Start(void* phRecorder);

/**************************************************************************************************
* Function declare:
*	Dvr_Recorder_LoopRec_Stop
* Description:
*	Start to record the event stream.
* Parameters:
*	phRecorder		[in]:
*		recorder object handle created by Dvr_Recorder_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Recorder_LoopRec_Stop(void* phRecorder);

/**************************************************************************************************
* Function declare:
*	Dvr_Recorder_EventRec_Start
* Description:
*	Start to record the event stream.
* Parameters:
*	phRecorder		[in]:
*		recorder object handle created by Dvr_Recorder_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Recorder_EventRec_Start(void* phRecorder);

/**************************************************************************************************
* Function declare:
*	Dvr_Recorder_EventRec_Stop
* Description:
*	Start to record the event stream.
* Parameters:
*	phRecorder		[in]:
*		recorder object handle created by Dvr_Recorder_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Recorder_EventRec_Stop(void* phRecorder);

/**************************************************************************************************
* Function declare:
*	Dvr_Recorder_IaccRec_Start
* Description:
*	Start to record the event stream.
* Parameters:
*	phRecorder		[in]:
*		recorder object handle created by Dvr_Recorder_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Recorder_IaccRec_Start(void* phRecorder);

/**************************************************************************************************
* Function declare:
*	Dvr_Recorder_IaccRec_Stop
* Description:
*	Start to record the event stream.
* Parameters:
*	phRecorder		[in]:
*		recorder object handle created by Dvr_Recorder_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Recorder_IaccRec_Stop(void* phRecorder);

/**************************************************************************************************
* Function declare:
*	Dvr_Recorder_DasRec_Start
* Description:
*	Start to record the event stream.
* Parameters:
*	phRecorder		[in]:
*		recorder object handle created by Dvr_Recorder_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Recorder_DasRec_Start(void* phRecorder);

/**************************************************************************************************
* Function declare:
*	Dvr_Recorder_DasRec_Stop
* Description:
*	Start to record the event stream.
* Parameters:
*	phRecorder		[in]:
*		recorder object handle created by Dvr_Recorder_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Recorder_DasRec_Stop(void* phRecorder);

/**************************************************************************************************
* Function declare:
*	Dvr_Recorder_Stop
* Description:
*	Try to stop the recording.
* Parameters:
*	phRecorder		[in]:
*		recorder object handle created by Dvr_Recorder_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Recorder_Stop(void* phRecorder);

/**************************************************************************************************
* Function declare:
*	Dvr_Recorder_AddFrame
* Description:
*	add the camera data to the recorder pipeline
* Parameters:
* 	phRecorder 	[in]:
*		recorder object handle created by Dvr_Recorder_Initialize interface.
* 	pImageBuff	[in]:
* 		Buffer pointer which hold the image data.
* 	u32ImageWidth	[in]:
* 		The width of the image.
* 	u32ImageHeight	[in]:
* 		The height of the image.
*	pCanBuff	[in]
*		Buffer pointer which hold the can data.
*	u32CanSize
*		The size of can data.
* 	u64TimeStamp	[in]:
* 		The timestamp of the image, in millisecond.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Recorder_AddFrame(void* phRecorder, DVR_IO_FRAME *pFrame);

/**************************************************************************************************
* Function declare:
*	Dvr_Recorder_AcquireInputBuf
* Description:
*	Acquire the input buffer to add the camera data
* Parameters:
* 	phRecorder 	[in]:
*		recorder object handle created by Dvr_Recorder_Initialize interface.
* 	ppvBuff	[in]:
* 		Buffer pointer which hold the image data.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Recorder_AcquireInputBuf(void* phRecorder, void **ppvBuffer);

/**************************************************************************************************
* Function declare:
*	Dvr_Recorder_SetConfig
* Description:
*	Set the specified configuration to recorder.
* Parameters:
*	phRecorder		[in]:
*		Recorder object handle created by Dvr_Recorder_Initialize interface.
*	eCfgType	[in]:
*		The configuration type.
*	pvCfgData		[in]:
*		The value, and it is correlative to specified configuration type.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Recorder_SetConfig(void* phRecorder, DVR_RECORDER_CFG_TYPE eCfgType, void* pvCfgData, unsigned int u32CfgDataSize);

/**************************************************************************************************
* Function declare:
*	Dvr_Recorder_GetConfig
* Description:
*	Set the specified configuration to recorder.
* Parameters:
*	phRecorder		[in]:
*		Recorder object handle created by Dvr_Recorder_Initialize interface.
*	eCfgType	[in]:
*		The configuration type.
*	pvCfgData		[in]:
*		The value, and it is correlative to specified configuration type.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Recorder_GetConfig(void* phRecorder, DVR_RECORDER_CFG_TYPE eCfgType, void* pvCfgData, unsigned int u32CfgDataSize);

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
int Dvr_Recorder_GetPosition(void* phRecorder, unsigned int* pu32Position);

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
int Dvr_Recorder_TakePhoto(void * phRecorder, DVR_PHOTO_PARAM *pParam);
int Dvr_Recorder_TakeOverlay(void * phRecorder, void *canbuffer, void *osdbuffer, int cansize, int osdsize);

/**************************************************************************************************
* Function declare:
*	Dvr_Recorder_Reset
* Description:
*	Reset Internal state.
* Parameters:
*	phRecorder		[in]:
*		Recorder object handle created by Dvr_Recorder_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Recorder_Reset(void* phRecorder);
int Dvr_Recorder_AsyncOpFlush(void* phRecorder);


#endif
