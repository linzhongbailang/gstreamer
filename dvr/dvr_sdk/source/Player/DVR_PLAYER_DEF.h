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
#ifndef _DVR_PLAYER_DEF_H_
#define _DVR_PLAYER_DEF_H_

#include <DVR_SDK_DEF.h>

#ifndef MAX_PATH
#define MAX_PATH								260
#endif
#define MAX_STREAM_COUNT						32
#define MAX_NAME_LEN							32
#define MILLISEC_TO_SEC							1000
#define MICROSEC_TO_SEC							1000000

#define MAX_OPERATION_MUTEX_WAIT_TIME			2 * MICROSEC_TO_SEC //2second
#define MUTEX_CHECK_WAIT_INTERVAL				50 * MILLISEC_TO_SEC //50ms

/**************************************************************************************************
* Function declare:
*	DVR_PLAYER_CALLBACK_ARRAY
* Description:
*	 This callback function is designed for the host application to inject code between frames during
*  a lengthy decoding operation. The callback function will be called once per video frame by the
*  player.The implementation of the callback should avoid heavy-duty works.
* Parameters:
*	wParam	[in]:
*		Param, usually point out the lParam's type.
*	lParam	[in]:
*		Param, usually point to a struct.
*	bHandled		[in/out]:
*		Does the default callback function need to carry out.
* Return value:
*	none
*************************************************************************************************/
typedef struct
{
	int(*MessageCallBack)(void *pvParam1, void *pvParam2, void *pvParam3, void *pvContext);
	int(*RawVideoNewFrame)(void *pvParam1, void *pvParam2, void *pvParam3, void *pvContext);

	void	*pCallBackInstance;
}DVR_PLAYER_CALLBACK_ARRAY;

typedef enum _tagDVR_PLAYER_CFG_TYPE
{
	//////////////////////////////////////////////////////////////////////////
	// video related config words
	//////////////////////////////////////////////////////////////////////////

	// enable or disable video output, parameter type is "int32_t", 1 enable, 0 disable
	DVR_PLAYER_CFG_VIDEO_RENDER_ENABLE = 0x10000000,
	// set the config of video display effect, refer to the definition of "DISPLAY_EFFECT" 
	DVR_PLAYER_CFG_VIDEO_DISPLAY_EFFECT,
	// set/get the color key value of the destination window
	DVR_PLAYER_CFG_VIDEO_DSTWND_COLOR_KEY,
	// set/get customized color space conversion coefficients
	DVR_PLAYER_CFG_VIDEO_CSC_COEF,
	// get video capability, param is VR_DDCOLORCONTROL
	DVR_PLAYER_CFG_VIDEO_CAPABILITY,
	// set/get the lowest decoding quality for video decoder, param is a "int32_t"
	DVR_PLAYER_CFG_VIDEO_LOWEST_QUALITY,
	// set command to call video flip opengl buffer(in ios or Android platform)
	DVR_PLAYER_CFG_VIDEO_FLIP_BUFFER,
	// set to close the video render, parameter type is "int32_t", set to 1
	DVR_PLAYER_CFG_VIDEO_CLOSE_RENDER,
	// enable or disable video, parameter type is "int32_t", 1 enable, 0 disable. This will apply to both video decoder and video renderer
	DVR_PLAYER_CFG_VIDEO_ENABLE,

	//////////////////////////////////////////////////////////////////////////
	// other controls or status
	//////////////////////////////////////////////////////////////////////////

	// get general media information, refer to the definition of "DVR_MEDIA_INFO"
	DVR_PLAYER_CFG_MEDIA_INFO = 0x10004000,
	// get AV player engine status, refer to the definition of "PLAYER_STATUS"
	DVR_PLAYER_CFG_PLAYER_STATUS,
	// get the duration which the buffered stream extends
	DVR_PLAYER_CFG_BUFFERED_TIME,
	// set/get fast forward speed, parameter type is "int32_t", 1000 represent 1x speed, 2000 represent 2x speed, and so on
	// actually if the speed is lower than 1000, the playback can also be slowed, such as 100 represent 0.1x speed
	DVR_PLAYER_CFG_FAST_FORWARD_SPEED,
	// get the direction of fast scan state, parameter type is "int32_t". 1 is fast forward, -1 is fast backward
	DVR_PLAYER_CFG_FAST_SCAN_DIRECTION,
	// get current byte position of the stream - use in streaming mode to get the accurate byte position for seeking
	// the parameter type is "uint32_t" 
	DVR_PLAYER_CFG_STREAM_BYTE_POSITION,

	// get media tracks information, refer to the definition of "MEDIA_TRACK"
	DVR_PLAYER_CFG_MEDIATRACKS_INFO,

	// get media statistics information
	DVR_PLAYER_CFG_MEDIA_STATISTICS,

	// for streaming mode, pass the file size to sdk. pvCfgData is a pointer to uint32_t
	DVR_PLAYER_CFG_STREAM_FILE_SIZE,

	DVR_PLAYER_CFG_ROOT_DIRECTORY,
}DVR_PLAYER_CFG_TYPE;

typedef enum _tagPLAYER_STATUS
{
	PLAYER_STATUS_INVALID = 0,
	PLAYER_STATUS_STOPPED,
	PLAYER_STATUS_PAUSED,
	PLAYER_STATUS_RUNNING,
	PLAYER_STATUS_FASTPLAY,
	PLAYER_STATUS_FASTSCAN,
	PLAYER_STATUS_STEPFRAME,
}PLAYER_STATUS;

typedef enum _tagPLAYER_EVENT
{
	PLAYER_EVENT_UNKNOWN = 0,
	PLAYER_EVENT_VIDEO_SRCSIZE_CHANGED,
	PLAYER_EVENT_PLAYBACK_BUFFER_INSUFFICIENT,
	PLAYER_EVENT_PLAYBACK_BUFFER_ENOUGH,
	PLAYER_EVENT_PLAYBACK_END,  
	PLAYER_EVENT_AVINFO_COMPLETE,
	PLAYER_EVENT_AUDIO_INFO_CHANGED,
	PLAYER_EVENT_VIDEO_NOT_SUPPORT,
	PLAYER_EVENT_VIDEO_CAP_CHANGED,// video capability(brightness, contrast ...) changed
	PLAYER_EVENT_CREATE_COMPONENT_FAIL,// param 2 is component type(video, audio ,subtitle ,etc.), param3 is the component index
	PLAYER_EVENT_SUBTITLE_INFO_CHANGED, // for subtitle or closed caption info changed
	PLAYER_EVENT_PRINT_SCREEN_ADD2DB,
	PLAYER_EVENT_PRINT_SCREEN_DONE,
}PLAYER_EVENT;

typedef struct _tagPLAYER_END_INFO
{
	PLAYER_STATUS ePlayerStatus;
	int s32PlaySpeed;  // 1000 scales of a second, negative value stands for backward playback.
}PLAYER_END_INFO;

typedef enum _tagPLAYER_COMPONENT_TYPE
{
	PLAYER_COMPONENT_DEMUX,
	PLAYER_COMPONENT_VIDEO_DECODER,
	PLAYER_COMPONENT_VIDEO_RENDERER,
	PLAYER_COMPONENT_AUDIO_DECODER,
	PLAYER_COMPONENT_AUDIO_RENDERER,
	PLAYER_COMPONENT_SUBTITLE,
}PLAYER_COMPONENT_TYPE;

#endif