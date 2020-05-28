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
#ifndef _DVR_APP_DEF_H_
#define _DVR_APP_DEF_H_

typedef enum 
{
	DVR_APP_MODE_NONE = 0,
	DVR_APP_MODE_RECORD,
	DVR_APP_MODE_THUMB,
	DVR_APP_MODE_PLAYBACK
}DVR_APP_MODE;

typedef enum
{
	DVR_APP_NOTIFICATION_TYPE_APP_SWTICH = 0,
	DVR_APP_NOTIFICATION_TYPE_MODE_UPDATE,
	DVR_APP_NOTIFICATION_TYPE_RECORDER_SWTICH,
	DVR_APP_NOTIFICATION_TYPE_EVENT_RECORD_STATUS,
	DVR_APP_NOTIFICATION_TYPE_EVENT_RECORD_SWITCH,
	DVR_APP_NOTIFICATION_TYPE_CARD_ERROR_STATUS_UPDATE,
	DVR_APP_NOTIFICATION_TYPE_CARD_FULL_STATUS_UPDATE,
    DVR_APP_NOTIFICATION_TYPE_PLAY_TIMER_UPDATE,
    DVR_APP_NOTIFICATION_TYPE_PAGE_ITEM_NUM_UPDATE,
	DVR_APP_NOTIFICATION_TYPE_DELETE_STATUS,
	DVR_APP_NOTIFICATION_TYPE_REPLAY_MODE,
	DVR_APP_NOTIFICATION_TYPE_STORAGE_PERCENT,
	DVR_APP_NOTIFICATION_TYPE_PHOTOGRAPH_RESULT,
	DVR_APP_NOTIFICATION_TYPE_NORMAL_TO_EMERGENCY_STATUS,
	DVR_APP_NOTIFICATION_TYPE_PLAYBACK_STATUS_UPDATE,
	DVR_APP_NOTIFICATION_TYPE_DISPLAY_VISION_UPDATE,
	DVR_APP_NOTIFICATION_TYPE_RECORDING_CYCLE_UPDATE,
	DVR_APP_NOTIFICATION_TYPE_PRINTSCREEN_RESULT,
	DVR_APP_NOTIFICATION_TYPE_CARD_FORMAT_STATUS,
	DVR_APP_NOTIFICATION_TYPE_FORMAT_CARD_REQUEST,
    DVR_APP_NOTIFICATION_TYPE_FORCE_IDR_REQUEST,
	
    //internal use
    DVR_APP_NOTIFICATION_TYPE_ASYNC_START_RECORDER,
    DVR_APP_NOTIFICATION_TYPE_ASYNC_STOP_RECORDER,
    DVR_APP_NOTIFICATION_TYPE_RECORDER_START_DONE,
    DVR_APP_NOTIFICATION_TYPE_PLAYER_OPEN_RESULT,
}DVR_APP_NOTIFICATION_TYPE;

//private use for S302
typedef enum
{
	DVR_SDCARD_ERROR_STATUS_NO_ERROR = 0,
	DVR_SDCARD_ERROR_STATUS_NO_SDCARD,
	DVR_SDCARD_ERROR_STATUS_WRITE_ERROR,
	DVR_SDCARD_ERROR_STATUS_FORMAT_ERROR,
	DVR_SDCARD_ERROR_STATUS_INSUFFICIENT_SPACE,
	DVR_SDCARD_ERROR_STATUS_LOW_SPEED,
	DVR_SDCARD_ERROR_STATUS_RESTORING,
	DVR_SDCARD_ERROR_STATUS_RESERVED,
}DVR_SDCARD_ERROR_STATUS;

typedef enum
{
	DVR_SDCARD_FULL_STATUS_NOT_FULL = 0,
	DVR_SDCARD_FULL_STATUS_EVENT_SPACE_FULL,
	DVR_SDCARD_FULL_STATUS_PHOTO_SPACE_FULL,
	DVR_SDCARD_FULL_STATUS_EVENT_PHOTO_SPACE_FULL,
}DVR_SDCARD_FULL_STATUS;

#define MDL_APP_MAIN_ID			0x01
#define MDL_APP_RECORDER_ID		0x02
#define MDL_APP_PLAYER_ID		0x03
#define MDL_APP_THUMB_ID		0x04
#define MDL_APP_FORMATCARD_ID	0x05

/*************************************************************************
* Application Global Flags
************************************************************************/
#define APP_AFLAGS_INIT         (0x00000001)	/*The application is initialized*/
#define APP_AFLAGS_START		(0x00000002)	/*The application is started*/
#define APP_AFLAGS_READY		(0x00000004)	/*The application is ready to do something*/
#define APP_AFLAGS_OVERLAP		(0x00000008)	/*The application will fully overlap the GUI of its parent*/
#define APP_AFLAGS_BUSY			(0x00000010)	/*The application is encoding/decoding video/photo*/
#define APP_AFLAGS_IO           (0x00000020)	/*The application is doing storage/file IO*/
#define APP_AFLAGS_ERROR		(0x00000040)	/*The application enters the error state*/
#define APP_AFLAGS_POPUP		(0x00000080)	/*The application has pop up bar/dialog/menu*/

#define APP_ADDFLAGS(x, y)          ((x) |= (y))
#define APP_REMOVEFLAGS(x, y)		((x) &= (~(y)))
#define APP_CHECKFLAGS(x, y)		((x) & (y))

/**
*  Defines for Message Type
*/
#define MSG_TYPE_HMI            0x00
#define MSG_TYPE_CMD            0x01
#define MSG_TYPE_PARAM          0x02
#define MSG_TYPE_ERROR          0x03

#define MSG_ID(mdl_id, msg_type, msg_par)       (((mdl_id & 0x0000001F) << 27) | ((msg_type & 0x00000007) << 24) | (msg_par & 0x00FFFFFF))
#define MSG_MDL_ID(id)          ((id & 0xF8000000)>>27)
#define MSG_TYPE(id)            ((id & 0x07000000)>>24)

/**
* MDL_APP_FLOW_ID 0x1C is defined for the message group of
* application flows and test flows, such as app state messages,
* app command messages, and app test messages
**/
#define MDL_APP_FLOW_ID				0x1C

/**********************************************************************/
/* MDL_APP_FLOW_ID messages                                          */
/**********************************************************************/
/**
* Partition: |31 - 27|26 - 24|23 - 16|15 -  8| 7 -  0|
*   |31 - 27|: MDL_APP_FLOW_ID
*   |26 - 24|: MSG_TYPE_HMI
*   |23 - 16|: app flow type ID
*   |15 -  8|: Self-defined
*   | 7 -  0|: Self-defined
* Note:
*   bit 0-15 could be defined in the app itself (individual
*   header files). However, module ID should be defined here
*   for arrangement
**/
#define HMSG_APPFLOW(x)         MSG_ID(MDL_APP_FLOW_ID, MSG_TYPE_HMI, (x))
/** Sub-group:type of app library & interface events */
#define HMSG_APPFLOW_ID_FLOW    (0x01)

/* App flow events */
#define HMSG_APPFLOW_FLOW(x)    HMSG_APPFLOW(((unsigned int)HMSG_APPFLOW_ID_FLOW << 16) | (x))
#define HMSG_EXIT_HDL           HMSG_APPFLOW_FLOW(0x0001)
#define HMSG_STOP_APP_COMPLETE  HMSG_APPFLOW_FLOW(0x0002)
/*************************************************************************
* Application messages ID (|15 -  8| in message)
************************************************************************/
#define HMSG_APPFLOW_FLOW_ID_ERROR      (0x01)
#define HMSG_APPFLOW_FLOW_ID_STATE      (0x02)
#define HMSG_APPFLOW_FLOW_ID_CMD        (0x03)
/*************************************************************************
* Application Error messages
************************************************************************/
#define HMSG_APPFLOW_FLOW_ERROR(x)      HMSG_APPFLOW_FLOW(((unsigned int)HMSG_APPFLOW_FLOW_ID_ERROR << 8) | (x))
#define AMSG_ERROR_CARD_REMOVED         HMSG_APPFLOW_FLOW_ERROR(0x01)
/*************************************************************************
* Application state messages
************************************************************************/
#define HMSG_APPFLOW_FLOW_STATE(x)      HMSG_APPFLOW_FLOW(((unsigned int)HMSG_APPFLOW_FLOW_ID_STATE << 8) | (x))
#define AMSG_STATE_CARD_REMOVED         HMSG_APPFLOW_FLOW_STATE(0x01)
#define AMSG_STATE_CARD_INSERT_ACTIVE   HMSG_APPFLOW_FLOW_STATE(0x02)

/*************************************************************************
* Application flow command messages
************************************************************************/
#define HMSG_APPFLOW_FLOW_CMD(x)		HMSG_APPFLOW_FLOW(((unsigned int)HMSG_APPFLOW_FLOW_ID_CMD << 8) | (x))

/**
* Note: For easy extension in the future, use enumerator to
* define app flow commands. Please be aware that, since only 8
* bits are arranged for defining flow commands, the definitions
* should be rearranged when the number of flow commands exceeds
* 255 (command ID 0 is not used)
**/
enum {
	HMSG_USER_FLOW_CMD_DUMMY = HMSG_APPFLOW_FLOW_CMD(0x00),
	AMSG_CMD_APP_READY,
	AMSG_CMD_SWITCH_APP,
	AMSG_CMD_CARD_FORMAT,
    AMSG_CMD_EVENT_RECORDER_EMERGENCY_COMPLETE,
    AMSG_CMD_EVENT_RECORDER_ALARM_COMPLETE,
    AMSG_CMD_EVENT_RECORDER_IACC_COMPLETE,

	HMSG_USER_CMD_RECORDER_ONOFF,
	HMSG_USER_CMD_RECORDER_EVENT_RECORD,
	HMSG_USER_CMD_RECORDER_DAS_RECORD_START,
    HMSG_USER_CMD_RECORDER_DAS_RECORD_STOP,
    HMSG_USER_CMD_RECORDER_IACC_RECORD,
    HMSG_USER_CMD_RECORDER_ALARM_RECORD_START,
	HMSG_USER_CMD_RECORDER_VIEW_INDEX,
	HMSG_USER_CMD_RECORDER_SET_VIDEO_QUALITY,
	HMSG_USER_CMD_RECORDER_SET_PHOTO_QUALITY,
	HMSG_USER_CMD_RECORDER_SNAPSHOT_SET,
	HMSG_USER_CMD_RECORDER_SNAPSHOT,
	HMSG_USER_CMD_RECORDER_LOOPREC_SPLIT_TIME,

	HMSG_USER_CMD_THUMB_TAB,
	HMSG_USER_CMD_THUMB_NEXT_PAGE,
	HMSG_USER_CMD_THUMB_PREV_PAGE,
	HMSG_USER_CMD_THUMB_SEL_TO_PLAY,
	HMSG_USER_CMD_THUMB_EDIT_ENTER,
	HMSG_USER_CMD_THUMB_EDIT_QUIT,
	HMSG_USER_CMD_THUMB_EDIT_COPY,
	HMSG_USER_CMD_THUMB_EDIT_DEL,
	HMSG_USER_CMD_THUMB_EDIT_SEL,
	HMSG_USER_CMD_THUMB_EDIT_SEL_ALL,

	HMSG_USER_CMD_PLAYER_PLAY,
	HMSG_USER_CMD_PLAYER_PAUSE,
	HMSG_USER_CMD_PLAYER_STOP,
	HMSG_USER_CMD_PLAYER_NEXT,
	HMSG_USER_CMD_PLAYER_PREV,
	HMSG_USER_CMD_PLAYER_SPEED,
	HMSG_USER_CMD_PLAYER_SEEK,
	HMSG_USER_CMD_PLAYER_DEL,
	HMSG_USER_CMD_PLAYER_COPY,
	HMSG_USER_CMD_PLAYER_SCREEN_SHOT,
	HMSG_USER_CMD_PLAYER_QUIT,
	HMSG_USER_CMD_PLAYER_DC_ONOFF,
	HMSG_USER_CMD_PLAYER_VIEW_INDEX,
	HMSG_USER_CMD_PLAYER_MODE_CHANGE,

	HMSG_USER_CMD_MAIN_MENU_TAB,
	HMSG_USER_CMD_DIALOG_SET,
	HMSG_USER_CMD_FORMAT_CARD,
    HMSG_USER_CMD_SIDEBAR_SET,
    HMSG_USER_CMD_THUMB_UPDATE_FRAME,
    HMSG_USER_CMD_PLAYER_OPEN_RESULT,
	HMSG_USER_CMD_PLAYER_FORWORD,
	HMSG_USER_CMD_PLAYER_REWIND,

	HMSG_USER_CMD_POWER_MODE,

	HMSG_USER_FLOW_CMD_LAST = HMSG_APPFLOW_FLOW_CMD(0xFF)
} ;

#endif //_DVR_APP_DEF_H_
