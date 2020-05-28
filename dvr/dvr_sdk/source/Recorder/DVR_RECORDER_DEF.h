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
#ifndef _DVR_RECORDER_DEF_H_
#define _DVR_RECORDER_DEF_H_

#include <DVR_SDK_DEF.h>

#define MILLISEC_TO_SEC							1000
#define MICROSEC_TO_SEC							1000000
#define NANOSEC_TO_SEC							1000000000ULL

#define DVR_IMG_BUF_WIDTH						2176//2560 the same as playback image buffer
#define DVR_IMG_BUF_HEIGHT						1376//1440

#define DVR_IMG_REAL_WIDTH						2048  //(128+1024+128)
#define DVR_IMG_REAL_HEIGHT						1280  //(640+80)

#define MAX_OPERATION_MUTEX_WAIT_TIME			2 * MICROSEC_TO_SEC //2second
#define MUTEX_CHECK_WAIT_INTERVAL				50 * MILLISEC_TO_SEC //50ms

typedef enum _tagDVR_RECORDER_CFG_TYPE
{
	DVR_RECORDER_CFG_FILE_SPLIT_TIME,				//time in second
	DVR_RECORDER_CFG_VIDEO_QUALITY,
	DVR_RECORDER_CFG_INTRA_INTERVAL,				//GOP Length
	DVR_RECORDER_CFG_ROOT_DIRECTORY,
	DVR_RECORDER_CFG_RECORDER_STATUS,
    DVR_RECORDER_CFG_EMERGENCY_SETTING,
	DVR_RECORDER_CFG_DAS_RECTYPE,
    DVR_RECORDER_CFG_FATAL_ERROR
}DVR_RECORDER_CFG_TYPE;

typedef enum _tagRECORDER_STATUS
{
	RECORDER_STATUS_INVALID = 0,
	RECORDER_STATUS_STOPPED,
	RECORDER_STATUS_RUNNING,
}RECORDER_STATUS;

typedef enum _tagRECORDER_MODE
{
	DVR_RECORDER_MODE_UNKNOWN = 0,
	DVR_RECORDER_MODE_NORMAL,
	DVR_RECORDER_MODE_EMERGENCY,
}DVR_RECORDER_MODE;

typedef enum _tagRECORDER_EVENT
{
	RECORDER_EVENT_UNKNOWN = 0,
	RECORDER_EVENT_NEW_FILE_START,
	RECORDER_EVENT_NEW_FILE_FINISHED,
	RECORDER_EVENT_STOP_DONE,
    RECORDER_EVENT_EMERGENCY_COMPLETE,
    RECORDER_EVENT_ALARM_COMPLETE,
	RECORDER_EVENT_IACC_COMPLETE,
	RECORDER_EVENT_SLOW_WRITING,
	RECORDER_EVENT_LOOPREC_FATAL_RECOVER,
    RECORDER_EVENT_LOOPREC_FATAL_ERROR,
    RECORDER_EVENT_EVENTREC_FATAL_ERROR,
    RECORDER_EVENT_IACCREC_FATAL_ERROR,
    RECORDER_EVENT_DASREC_FATAL_ERROR,
    RECORDER_EVENT_PHOTO_DONE,
}RECORDER_EVENT;

typedef struct
{
	int(*MessageCallBack)(void *pvParam1, void *pvParam2, void *pvParam3, void *pvContext);

	void	*pCallBackInstance;
}DVR_RECORDER_CALLBACK_ARRAY;

typedef struct
{
	int qp_i;
	unsigned int target_bitrate;
}RECORDER_VIDEO_QUALITY_SETTING;

#endif
