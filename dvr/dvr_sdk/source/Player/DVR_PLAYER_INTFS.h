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
#ifndef _DVR_PLAYER_INTFS_H_
#define _DVR_PLAYER_INTFS_H_

#include "DVR_PLAYER_DEF.h"

/**************************************************************************************************
* Function declare:
*	Dvr_Player_Initialize
* Description: Initialize the handle of player object.
* Parameters:
*	ppPlayer 	[in/out]:
*		The handle of player object. Input a pointer of handle, the returned handle value
*		will be the initialized player object handle.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
* Notes:
*	The initial interface must be called once ahead of other player interfaces, and the returned
*	player object handle should pass to all other implemented player interface.
*************************************************************************************************/
int Dvr_Player_Initialize(void** phPlayer);

/**************************************************************************************************
* Function declare:
*	Dvr_Player_DeInitialize
* Description:
*	Destroy the handle of player object.
* Parameters:
*	ppPlayer 	[in]:
*		Player object handle created by Dvr_Player_Initialize interface
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
* Notes:
*	The destroy interface must be called while exiting player program.
*************************************************************************************************/
int Dvr_Player_DeInitialize(void* phPlayer);

/**************************************************************************************************
* Function declare:
*	Dvr_Player_Open
* Description:
*	Open the clip via a specified URL.
* Parameters:
* 	pPlayer 	[in]:
*		Player object handle created by Dvr_Player_Initialize interface.
* 	pFileName	[in]:
* 		The pFileName is the specified file including whole path. In the future, it will support
*		standard URL format.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Player_Open(void* hPlayer, void* pvFileName);

/**************************************************************************************************
* Function declare:
*	Dvr_Player_Frame_Update
* Description:
*	Update the new frame buffer from player. Used under stream playback mode.
* Parameters:
* 	pPlayer 	[in]:
*		Player object handle created by Dvr_Player_Initialize interface.
* 	pImageBuf[in]:
* 		Buffer pointer which point to the address of real image data.
*
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/

int Dvr_Player_Frame_Update(void* hPlayer, DVR_IO_FRAME *pInputFrame);

/**************************************************************************************************
* Function declare:
*	Dvr_Player_Close
* Description:
*	Close the clip resource, the state will switch to initial status.
* Parameters:
* 	pPlayer 	[in]:
*		Player object handle created by Dvr_Player_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
* Notes:
*	Call this function to close the media stream and release related resources.
*************************************************************************************************/
int Dvr_Player_Close(void* hPlayer);

/**************************************************************************************************
* Function declare:
*	Dvr_Player_RegisterCallBack
* Description:
*	Register player callback function Return value is the error code.
* Parameters:
*	pPlayer		   	[in]:
*		Player object handle created by Dvr_Player_Initialize interface.
*	pCallbackList	[in]:
*		The Callback function pointer list.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Player_RegisterCallBack(void* hPlayer, DVR_PLAYER_CALLBACK_ARRAY *ptfnCallBackArray);
/**************************************************************************************************
* Function declare:
*	Dvr_Player_Play
* Description:
*	Start to playback the opened clip or paused clip.
* Parameters:
*	pPlayer		[in]:
*		Player object handle created by Dvr_Player_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Player_Play(void* hPlayer);

/**************************************************************************************************
* Function declare:
*	Dvr_Player_FastForward
* Description:
*	Turn current playback into fast forward mode.
* Parameters:
*	pPlayer		[in]:
*		Player object handle created by Dvr_Player_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Player_FastForward(void* hPlayer);
int Dvr_Player_FastScan(void* hPlayer, int s32Direction);

/**************************************************************************************************
* Function declare:
*	Dvr_Player_Pause
* Description:
*	Try to pause the playing clip.
* Parameters:
*	pPlayer		[in]:
*		Player object handle created by Dvr_Player_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Player_Pause(void* hPlayer);

/**************************************************************************************************
* Function declare:
*	Dvr_Player_Resume
* Description:
*	Try to pause the playing clip.
* Parameters:
*	pPlayer		[in]:
*		Player object handle created by Dvr_Player_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Player_Resume(void* hPlayer);

/**************************************************************************************************
* Function declare:
*	Dvr_Player_Stop
* Description:
*	Try to stop the playing clip.
* Parameters:
*	pPlayer		[in]:
*		Player object handle created by Dvr_Player_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Player_Stop(void* hPlayer);

/**************************************************************************************************
* Function declare:
*	Dvr_Player_ShowNextFrame
* Description:
*	Enter step-frame mode that the video frame will forward one by one by calling this interface.
* Parameters:
*	pPlayer		[in]:
*		Player object handle created by Dvr_Player_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Player_ShowNextFrame(void* hPlayer);

/**************************************************************************************************
* Function declare:
*	Dvr_Player_SetPosition
* Description:
*	Seek the clip to the specified position.
* Parameters:
*	pPlayer		[in]:
*		Player object handle created by Dvr_Player_Initialize interface.
*	dwPosition	[in]:
*		The position in ms which wants to seek to.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Player_SetPosition(void* hPlayer, unsigned int u32Position);

/**************************************************************************************************
* Function declare:
*	Dvr_Player_GetPosition
* Description:
*	Get the current position of clip.
* Parameters:
*	pPlayer		[in]:
*		Player object handle created by Dvr_Player_Initialize interface.
*	dwPosition	[out]:
*		The current playback position.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Player_GetPosition(void* hPlayer, unsigned int* pu32Position);

/**************************************************************************************************
* Function declare:
*	Dvr_Player_SetConfig
* Description:
*	Set the specified configuration to player.
* Parameters:
*	pPlayer		[in]:
*		Player object handle created by Dvr_Player_Initialize interface.
*	dwCfgType	[in]:
*		The configuration type.
*	pValue		[in]:
*		The value, and it is correlative to specified configuration type.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Player_SetConfig(void* hPlayer, DVR_PLAYER_CFG_TYPE eCfgType, void* pvCfgData, unsigned int u32CfgDataSize);

/**************************************************************************************************
* Function declare:
*	Dvr_Player_GetConfig
* Description:
*	Get the specified configuration of player.
* Parameters:
*	pPlayer		[in]:
*		Player object handle created by Dvr_Player_Initialize interface.
*	dwCfgType	[in]:
*		The configuration type.
*	pValue		[out]:
*		The value, and it is correlative to specified configuration type.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Player_GetConfig(void* hPlayer, DVR_PLAYER_CFG_TYPE eCfgType, void* pvCfgData, unsigned int u32CfgDataSize);

/**************************************************************************************************
* Function declare:
*	Dvr_Photo_Player_Open
* Description:
*	Open the clip via a specified URL.
* Parameters:
* 	pPlayer 	[in]:
*		Player object handle created by Dvr_Player_Initialize interface.
* 	pFileName	[in]:
* 		The pFileName is the specified file including whole path. In the future, it will support
*		standard URL format.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Photo_Player_Open(void *hPlayer, const char *fileName);

/**************************************************************************************************
* Function declare:
*	Dvr_Photo_Player_Close
* Description:
*	Close the clip resource, the state will switch to initial status.
* Parameters:
* 	pPlayer 	[in]:
*		Player object handle created by Dvr_Player_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
* Notes:
*	Call this function to close the media stream and release related resources.
*************************************************************************************************/
int Dvr_Photo_Player_Close(void *hPlayer);

/**************************************************************************************************
* Function declare:
*	Dvr_Photo_Player_Play
* Description:
*	Play the clip resource, the state will switch to initial status.
* Parameters:
* 	pPlayer 	[in]:
*		Player object handle created by Dvr_Player_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
* Notes:
*	Call this function to play the media stream and release related resources.
*************************************************************************************************/
int Dvr_Photo_Player_Play(void *hPlayer);

/**************************************************************************************************
* Function declare:
*	Dvr_Photo_Player_Stop
* Description:
*	Stop the clip resource, the state will switch to initial status.
* Parameters:
* 	pPlayer 	[in]:
*		Player object handle created by Dvr_Player_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
* Notes:
*	Call this function to stop the media stream and release related resources.
*************************************************************************************************/
int Dvr_Photo_Player_Stop(void *hPlayer);

/**************************************************************************************************
* Function declare:
*	Dvr_Player_PrintScreen
* Description:
*	Screen Print
* Parameters:
* 	pPlayer 	[in]:
*		Player object handle created by Dvr_Player_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
* Notes:
*	Call this function to stop the media stream and release related resources.
*************************************************************************************************/
int Dvr_Player_PrintScreen(void * hPlayer, DVR_PHOTO_PARAM *pParam);

#endif
