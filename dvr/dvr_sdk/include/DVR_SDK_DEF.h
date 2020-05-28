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
#ifndef _DVR_SDK_DEF_H_
#define _DVR_SDK_DEF_H_

#ifndef PARAM_DIRECT
#define PARAM_DIRECT
#define _IN
#define _OUT
#define _IN_OUT
#define CONST const
#endif

#ifdef __linux__
#include <linux/limits.h>
#include <stdlib.h>
#else
#ifndef PATH_MAX
#define PATH_MAX 260
#endif
#endif

#ifndef MAX_DEVICE_NUM
#define MAX_DEVICE_NUM	26			/* Only support 26 devices	 */
#endif

#define APP_MAX_DIR_SIZE     (128)
#define APP_MAX_FN_SIZE      (128)

#define DVR_THUMBNAIL_WIDTH		192
#define DVR_THUMBNAIL_HEIGHT	112

#define APPLIB_ADDFLAGS(x, y)          ((x) |= (y))
#define APPLIB_REMOVEFLAGS(x, y)    ((x) &= (~(y)))
#define APPLIB_CHECKFLAGS(x, y)       ((x) & (y))

#define DVR_SUCCEEDED(x)		(((signed)(x)) >= 0)
#define DVR_FAILED(x)			(((signed)(x)) < 0)
#define DVR_ARRAYSIZE(x)  		(sizeof((x))/sizeof(((x)[0])))

#define DVR_TIMEOUT_NO_WAIT                ((unsigned) 0)  // no timeout
#define DVR_TIMEOUT_WAIT_FOREVER           ((unsigned)-1)  // wait forever

#ifdef __linux__
#define DVR_API
#else
#define DVR_API __declspec(dllexport)
#endif

#define DVR_VIDEO_SPLIT_TIME_60_SECONDS			61		/**<VIDEO SPLIT TIME 60_SECONDS  */
#define DVR_VIDEO_SPLIT_TIME_180_SECONDS		181		/**<VIDEO SPLIT TIME 180_SECONDS  */
#define DVR_VIDEO_SPLIT_TIME_300_SECONDS		301		/**<VIDEO SPLIT TIME 300_SECONDS  */

/**threshold value for free space check*/
#define DVR_LOOPREC_STORAGE_FREESPACE_THRESHOLD		1024*1024 /**< default 1G*/
#define DVR_STORAGE_PHOTO_FOLDER_QUOTA			200*1024 /**< default 200MB*/
#define DVR_STORAGE_DAS_FOLDER_QUOTA			1.5*1024*1024 /**< default 1.5GB*/

#define DVR_STORAGE_EVENT_FOLDER_WARNING_THRESHOLD	300*1024	/**< default 300MB*/
#define DVR_STORAGE_PHOTO_FOLDER_WARNING_THRESHOLD	2*1024	/**< default 2MB*/

#define DVR_STORAGE_SDCARD_TOTAL_SPACE_THRESHOLD	6*1024*1024 /**< 6GB*/
#define DVR_STORAGE_SDCARD_SPACE_FOR_DVR_THRESHOLD	5*1024*1024 /**< 5GB*/

typedef unsigned char		DVR_U8;
typedef unsigned short		DVR_U16;
typedef unsigned int		DVR_U32;
typedef unsigned long long	DVR_U64;
typedef signed char			DVR_S8;
typedef signed short		DVR_S16;
typedef signed int			DVR_S32;
typedef signed long long	DVR_S64;
typedef float				DVR_F32;
typedef double				DVR_F64;
typedef int					DVR_BOOL;
typedef void				DVR_VOID;
typedef int					DVR_RESULT;
typedef void *				DVR_HANDLE;

#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif

enum
{
	DVR_RES_SOK									= 0x00000000,
	DVR_RES_EFAIL								= 0x80000001,
	DVR_RES_EPOINTER							= 0x80000002,
	DVR_RES_EOUTOFMEMORY						= 0x80000003,
	DVR_RES_EINVALIDARG							= 0x80000004,
	DVR_RES_EUNEXPECTED							= 0x80000005,
	DVR_RES_EINPBUFTOOSMALL						= 0x80000006,
	DVR_RES_EOUTBUFTOOSMALL						= 0x80000007,
	DVR_RES_EMODENOTSUPPORTED					= 0x80000008,
	DVR_RES_NOTINITED							= 0x80000009,
	DVR_RES_DEMUX_FAIL							= 0x8000000A,
	DVR_RES_EDEMUX								= 0x8000000B,
	DVR_RES_EAUDIO_DECODER						= 0x8000000C,
	DVR_RES_EVIDEO_DECODER						= 0x8000000D,
	DVR_RES_EAUDIO_RENDER						= 0x8000000E,
	DVR_RES_EVIDEO_RENDER						= 0x8000000F,	
	DVR_RES_ERESOLUTIONNOTSUPPORTED				= 0x80000010,
	DVR_RES_ENOTIMPL							= 0x80000011,
	DVR_RES_EABORT								= 0x80000012,
	DVR_RES_EPLAYERCMDNOTSUPPORT				= 0x80000013,
};

typedef enum _tagDVR_RECORDER_PROP
{
	DVR_RECORDER_PROP_VIDEO_SPLIT_TIME,		/*!< RW: pPropData: unsigned int *, length in seconds.*/
	DVR_RECORDER_PROP_ROOT_DIRECTORY,
	DVR_RECORDER_PROP_VIDEO_QUALITY,			/*!< RW: pPropData: unsigned int *, sfine, fine, normal*/
	DVR_RECORDER_PROP_PHOTO_QUALITY,			/*!< RW: pPropData: unsigned int *, sfine, fine, normal*/
    DVR_RECORDER_PROP_EMERGENCY_SETTING,
	DVR_RECORDER_PROP_DAS_RECTYPE,
    DVR_RECORDER_PROP_FATAL_ERROR,
	DVR_RECORDER_PROP_DAS_MOVE,
}DVR_RECORDER_PROP;

typedef struct
{
	DVR_U32 nSplitTime;
	DVR_BOOL bImmediatelyEffect;
}DVR_RECORDER_PERIOD_OPTION;

typedef enum _tagDVR_PLAYER_PROP
{
	DVR_PLAYER_PROP_POSITION,  			/*!< RW: pPropData: unsigned int *, position in milliseconds. For example: @snippet DvrTest_Set_Player_POSITION.cpp Set_Player_POSITION Example */
	DVR_PLAYER_PROP_DURATION,			/*!< R:  pPropData:  unsigned int *, duration in milliseconds. */
	DVR_PLAYER_PROP_SPEED,				/*!< RW: pPropData: int *, speed based on 1000 = 1x.  Can be positive or negative. For example: @snippet DvrTest_Set_Player_SPEED.cpp Set_Player_SPEED Example */
	DVR_PLAYER_PROP_DB_FILE,			/*!< RW: pPropData: char * The location for the database file, this can be fully qualified or relative (all other filesystem databases are suffixes of this in the same directory). */
	DVR_PLAYER_PROP_CURRENT_FILE,
	DVR_PLAYER_PROP_ROOT_DIRECTORY
}DVR_PLAYER_PROP;

typedef enum _tagDVR_STORAGE_PROP
{
    DVR_STORAGE_PROP_THRESHOLD,
    DVR_STORAGE_PROP_CARD_FREESPACE,
    DVR_STORAGE_PROP_CARD_TOTALSPACE,
    DVR_STORAGE_PROP_CARD_USEDSPACE,
	DVR_STORAGE_PROP_CARD_QUOTA,
	DVR_STORAGE_PROP_CARD_SPEED_STATUS,
	DVR_STORAGE_PROP_CARD_READY_STATUS,
	DVR_STORAGE_PROP_FREE_SPACE_STATUS,
    DVR_STORAGE_PROP_NORMAL_FOLDER_USEDSPACE,
    DVR_STORAGE_PROP_EVENT_FOLDER_USEDSPACE,
    DVR_STORAGE_PROP_PHOTO_FOLDER_USEDSPACE,
	DVR_STORAGE_PROP_DAS_FOLDER_USEDSPACE,
	DVR_STORAGE_PROP_DIRECTORY_STRUCTURE,
	DVR_STORAGE_PROP_ROOT_FOLDER,
	DVR_STORAGE_PROP_NORMAL_FOLDER,
	DVR_STORAGE_PROP_EMERGENCY_FOLDER,
	DVR_STORAGE_PROP_PHOTO_FOLDER,
	DVR_STORAGE_PROP_DAS_FOLDER
}DVR_STORAGE_PROP;

typedef enum _tagDVR_MONITOR_STORAGE_PROP
{
	DVR_MONITOR_STORAGE_PROP_LOOPREC_THRESHOLD,
	DVR_MONITOR_STORAGE_PROP_ENABLE_NORMAL_DETECT,
	DVR_MONITOR_STORAGE_PROP_ENABLE_NORMAL_MSG,
    DVR_MONITOR_STORAGE_PROP_NORMAL_QUOTA,
	DVR_MONITOR_STORAGE_PROP_CARD_AVAILABLE_SPACE,
    DVR_MONITOR_STORAGE_PROP_ENABLE_DAS_DETECT,
    DVR_MONITOR_STORAGE_PROP_ENABLE_DAS_MSG
}DVR_MONITOR_STORAGE_PROP;

typedef struct
{
    DVR_U32 nNormalStorageQuota;
    DVR_U32 nEventStorageQuota;
	DVR_U32 nPhotoStorageQuota;
	DVR_U32 nDasStorageQuota;
}DVR_STORAGE_QUOTA;

typedef struct  
{
	char szRootDir[APP_MAX_FN_SIZE];
    char szLoopRecDir[APP_MAX_FN_SIZE];
    char szEventRecDir[APP_MAX_FN_SIZE];
	char szPhotoDir[APP_MAX_FN_SIZE];
	char szDasDir[APP_MAX_FN_SIZE];
}DVR_DISK_DIRECTORY;

typedef struct
{
	unsigned  int x;
	unsigned  int y;
	unsigned  int width;
	unsigned  int height;
}VideoCropMeta;

typedef struct
{
	void *pPhyAddr;
	void *pVirtAddr;
	void *pGstBuffer;
}DVR_DRM_BUFFER;

typedef struct
{
    DVR_U8 *pImageBuf[4];
    DVR_U32 nSingleViewWidth;
    DVR_U32 nSingleViewHeight;
    DVR_U32 nFramelength;

    void *pMattsImage;
    DVR_U32 nMattsImageWidth;
    DVR_U32 nMattsImageHeight;
	VideoCropMeta crop;

	DVR_U8 *pCanBuf;
	DVR_U8 IsKeyFrame;
	DVR_U32 nCanSize;
	
	DVR_U64 nTimeStamp;
}DVR_IO_FRAME;

typedef enum
{
	DVR_EVENTREC_SOURCE_TYPE_SWITCH,
	DVR_EVENTREC_SOURCE_TYPE_CRASH,
	DVR_EVENTREC_SOURCE_TYPE_ALARM,
	DVR_EVENTREC_SOURCE_TYPE_IACC_IAC,
	DVR_EVENTREC_SOURCE_TYPE_IACC_ACC,
	DVR_EVENTREC_SOURCE_TYPE_IACC_AEB,
	DVR_EVENTREC_SOURCE_TYPE_MAX
}DVR_EVENTREC_SOURCE_TYPE;

typedef struct 
{
    DVR_U32 EventPreRecordLimitTime;//Event record pre record limit time in millisecond
    DVR_U32 EventRecordLimitTime;//Event record clip limit time in millisecond
	DVR_U32 EventRecordDelayTime;
    DVR_EVENTREC_SOURCE_TYPE eType;
	char szEventDir[APP_MAX_FN_SIZE];
} DVR_EVENT_RECORD_SETTING;

typedef enum _tagDVR_MEDIA_TYPE
{
	DVR_MEDIATYPE_DEMUX,
	DVR_MEDIATYPE_AUDIO,
	DVR_MEDIATYPE_VIDEO,
	DVR_MEDIATYPE_SUBTITLE,
	DVR_MEDIATYPE_TELETEXT
}DVR_MEDIA_TYPE;

typedef enum _tagDVR_SDSPEED_TYPE
{
	DVR_SD_SPEED_NONE,
	DVR_SD_SPEED_NORMAL,
	DVR_SD_SPEED_BAD,
	DVR_SD_SPEED_TYPE
}DVR_SDSPEED_TYPE;

typedef enum _tagDVR_CODEC_TYPE
{
	DVR_CODEC_NONE = 0,

	// audio decoder
	DVR_AUDIO_DECODER = 0x00010000,
	DVR_AUDDEC_MPEGA,             // MPEG Audio
	DVR_AUDDEC_AMRNB,             // AMR narrow band
	DVR_AUDDEC_AMRWB,             // AMR wide band
	DVR_AUDDEC_AAC,               // AAC, by MPEG
	DVR_AUDDEC_WMA,               // WMA, by MicroSoft
	DVR_AUDDEC_PCM,               // Raw PCM
	DVR_AUDDEC_AC3,
	DVR_AUDDEC_APE,
	DVR_AUDDEC_BSAC,
	DVR_AUDDEC_DRA,
	DVR_AUDDEC_G711,
	DVR_AUDDEC_G723,
	DVR_AUDDEC_G726,
	DVR_AUDDEC_G729,
	DVR_AUDDEC_FLAC,
	DVR_AUDDEC_OGG,
	DVR_AUDDEC_IMAPCM,
	DVR_AUDDEC_LATM,
	DVR_AUDDEC_HVXC,
	DVR_AUDDEC_CELP,
	DVR_AUDDEC_REAL,
	DVR_AUDDEC_VORBIS,
	DVR_AUDDEC_ALAC,
	DVR_AUDDEC_MPC,
	DVR_AUDDEC_MSADPCM,

	// video decoder
	DVR_VIDEO_DECODER = 0x00020000,
	DVR_VIDDEC_MP1,
	DVR_VIDDEC_MP2,               // MPEG2
	DVR_VIDDEC_MP4,               // MPEG4
	DVR_VIDDEC_H263,              // H.263
	DVR_VIDDEC_H264,              // H.264
	DVR_VIDDEC_WMV,               // WMV, by MicroSoft
	DVR_VIDDEC_REAL,
	DVR_VIDDEC_DIVX,
	DVR_VIDDEC_FLV,
	DVR_VIDDEC_MJPEG,
	DVR_VIDDEC_MP4V3,
	DVR_VIDDEC_VP6,
	DVR_VIDDEC_VP8,
	DVR_VIDDEC_AVS,

	// subtitle
	DVR_SUBTITLE_NONE = 0,
	DVR_SUBTITLE_DECODER = 0x00030000,
	DVR_SUBDEC_ISDBTCC,
	DVR_SUBDEC_DVBTSUB,
	DVR_SUBDEC_TXTSUB,
	DVR_SUBDEC_ATSCCC,

	DVR_CODEC_UNSUPPORTED = -1,
}DVR_CODEC_TYPE;

typedef enum _tagCI_LANGUAGE_TYPE
{
	DVR_LAN_UNKNOWN = 0,
	DVR_LAN_EN,    // English
	DVR_LAN_FR,    // French
	DVR_LAN_ZH,    // Chinese
	DVR_LAN_JA,    // Japanese
	DVR_LAN_PT,    // Portuguese
	DVR_LAN_KO,    // Korean 
}DVR_LANGUAGE_TYPE;

typedef struct _tagCI_MEDIA_TRACK
{
	DVR_MEDIA_TYPE	eMediaType;
	DVR_CODEC_TYPE	eCodecType;
	DVR_LANGUAGE_TYPE eLanguageType;
	unsigned int	u32TrackID;
	unsigned int	u32Bitrate;
	unsigned char	au8CdcNameStr[32];
	unsigned char	au8DescriptionStr[32];
	unsigned int	u32ExtDataLength;
	unsigned char*	pu8ExtData;
	union {
		struct {
			unsigned int u32AspectRatioX;
			unsigned int u32AspectRatioY;
			unsigned int u32DisplayWidth;
			unsigned int u32DisplayHeight;
			unsigned int u32FrameRate100;
		} Video;
		struct {
			unsigned int u32AudioSamplingFreq;
			unsigned int u32AudioOutputSamplingFreq;
			unsigned int u32AudioChannel;
			unsigned int u32AudBitsPerSample;
		} Audio;
	} AV;
}DVR_MEDIA_TRACK;

typedef struct _tagDVR_MEDIA_INFO
{
	unsigned int u32Seekable;
	unsigned int u32Duration;  // millisecond
	unsigned int u32BitRate;   // bits per second
	unsigned char  au8DemuxNameStr[32];
	unsigned int u32AudioTrackCount;
	unsigned int u32VideoTrackCount;
	unsigned int u32SubtitleTrackCount;
}DVR_MEDIA_INFO;

/*
*/
typedef enum _tagDVR_NOTIFICATION_TYPE
{
	DVR_NOTIFICATION_TYPE_EXCEPTION = 0,		/*!< pParam1: @ref DVR_RESULT *, exception error code <br> */
												/*!< pParam2: None */
	DVR_NOTIFICATION_TYPE_PLAYERROR,			/*!< pParam1: Error code to describe the errors. @ref DVR_RESULT *<br> */
												/*!< pParam2: None */
	DVR_NOTIFICATION_TYPE_PLAYERSTATE,			/*!< pParam1: @ref DVR_PLAYER_STATE *, state of current playback <br> */
												/*!< pParam2: None */
	DVR_NOTIFICATION_TYPE_POSITION,				/*!< pParam1: unsigned int *, position of current playback, number of seconds <br> */
												/*!< pParam2: unsigned int *, duration of current playback, number of seconds */
	DVR_NOTIFICATION_TYPE_SCANSTATE,			/*!< pParam1: @ref DVR_DB_SCAN_STATE *, state of mounting device <br> */
												/*!< pParam2: const char * specify the drive string*/
	DVR_NOTIFICATION_TYPE_RANGEEVENT,			/*!< pParam1: @ref DVR_RANGE_EVENT *, <br> */
												/*!< pParam2: if *(DVR_RANGE_EVENT *pParam1) == DVR_RGE_EOF, the pParam2 is @ref DVR_OPERATION_PLAY* */
	DVR_NOTIFICATION_TYPE_DEVICEEVENT,			/*!< pParam1: @ref DVR_DEVICE_EVENT *,<br>  */
												/*!< pParam2: @ref DVR_DEVICE * */
	DVR_NOTIFICATION_TYPE_MEDIALIBRARYFINISH,	/*!< pParam1: None*/
												/*!< pParam2: None*/
    /*... */
	DVR_NOTIFICATION_TYPE_INTERNAL = 0x8000,	/*!< Reserved after this event for internal usage */
	DVR_NOTIFICATION_TYPE_FILEEVENT,
	DVR_NOTIFICATION_TYPE_RECORDER_STOP_DONE,
    DVR_NOTIFICATION_TYPE_RECORDER_DESTROY_DONE,
    DVR_NOTIFICATION_TYPE_RECORDER_EMERGENCY_COMPLETE,
    DVR_NOTIFICATION_TYPE_RECORDER_ALARM_COMPLETE,
	DVR_NOTIFICATION_TYPE_RECORDER_IACC_COMPLETE,
	DVR_NOTIFICATION_TYPE_RECORDER_LOOPREC_FATAL_RECOVER,
    DVR_NOTIFICATION_TYPE_RECORDER_LOOPREC_FATAL_ERROR,
    DVR_NOTIFICATION_TYPE_RECORDER_DASREC_FATAL_ERROR,
	DVR_NOTIFICATION_TYPE_RECORDER_SLOW_WRITING,
    DVR_NOTIFICATION_TYPE_PHOTO_DONE,
	DVR_EXCEPTION_TYPE_ENGINE_FILE_ERROR = 0x80220000, /*!< Bad file from the engine */
} DVR_NOTIFICATION_TYPE;

typedef enum _tagDVR_PLAYER_STATE
{
	DVR_PLAYER_STATE_UNKNOWN = 0,
	DVR_PLAYER_STATE_INVALID,
	DVR_PLAYER_STATE_CLOSE,
	DVR_PLAYER_STATE_STOP,
	DVR_PLAYER_STATE_PLAY,
	DVR_PLAYER_STATE_PAUSE,
	DVR_PLAYER_STATE_FASTFORWARD,
	DVR_PLAYER_STATE_FASTBACKWARD,
} DVR_PLAYER_STATE;

typedef enum _tagDVR_RANGE_EVENT
{
	DVR_RANGE_EVENT_START = 0,		/*!< Start to play in range */
	DVR_RANGE_EVENT_EOF,				/*!< Finished current file playback */
	DVR_RANGE_EVENT_SWITCHFILE,		/*!< Switch to next file in range and start to play */
	DVR_RANGE_EVENT_END,				/*!< End of playing in range */
} DVR_RANGE_EVENT;

typedef enum _tagDVR_DEVICE_EVENT
{
	DVR_DEVICE_EVENT_PLUGIN,		/*!< Device Plug-in */
	DVR_DEVICE_EVENT_PLUGOUT,		/*!< Device plug-out */
	DVR_DEVICE_EVENT_FSCHECK,		/*!< Device fs-check */
    DVR_DEVICE_EVENT_ERROR,			/*!< Device Error */
    DVR_DEVICE_EVENT_FORMAT_NOTSUPPORT,
} DVR_DEVICE_EVENT;

typedef enum _tagDVR_FILE_EVENT
{
	DVR_FILE_EVENT_CREATE,
	DVR_FILE_EVENT_DELETE,
}DVR_FILE_EVENT;

typedef enum _tagDVR_DEVICE_TYPE
{
	DVR_DEVICE_TYPE_UNKNOWN = 0,		/*!< unknown device */
	DVR_DEVICE_TYPE_SD,				/*!< SD card */
	DVR_DEVICE_TYPE_USBHD,			/*!< USB hard drive */
	DVR_DEVICE_TYPE_USBHUB,

	/* ... */
	DVR_DEVICE_TYPE_ALL = -1,			/*!< all support device */
} DVR_DEVICE_TYPE;

typedef struct _tagDVR_MOUNT_STATE
{
	unsigned uFirstScanStatus;		/*!< DVR_FIRSTSCAN_STATUS bitfield */
} DVR_MOUNT_STATE;

static const char * const DVR_AUDIO_EXT[] =
{
	"mp3",
	"flac",
	"aac",
	"wav",
	"pcm",
};

static const char * const DVR_IMAGE_EXT[] =
{
	"jpg",
};

static const char * const DVR_VIDEO_EXT[] =
{
	"mp4",
};

static const char * const DVR_THUMBNAIL_EXT[] =
{
    "bmp",
};

static const char *const DVR_DEFAULT_METANAME = "MediaLibrary";
static const char *const DVR_DEFAULT_PHOTO_METANAME = "PhotoMetaLibrary";
static const char *const DVR_DEFAULT_VIDEO_METANAME = "VideoMetaLibrary";
static const char *const DVR_DEFAULT_FILEMAP_METANAME = "FileMapLibrary";

typedef struct
{
    char szMediaFileName[APP_MAX_FN_SIZE];
    char szThumbNailFileName[APP_MAX_FN_SIZE];
    DVR_U64 ullMediaFileSize;
    DVR_U64 ullThumbNailFileSize;
	DVR_U64 ullMediaFileModifyTime;
	DVR_U32 uDirId;
}DVR_FILEMAP_ITEM;

typedef enum _tagDVR_FILE_TYPE
{
	DVR_FILE_TYPE_DIRECTORY = 0,
	DVR_FILE_TYPE_AUDIO = 1,
	DVR_FILE_TYPE_VIDEO = 2,
	DVR_FILE_TYPE_IMAGE = 3,
    DVR_FILE_TYPE_THUMBNAIL = 4,
    DVR_FILE_TYPE_NUM
} DVR_FILE_TYPE;

typedef enum _tagDVR_VIDEO_QUALITY
{
	DVR_VIDEO_QUALITY_SFINE,
	DVR_VIDEO_QUALITY_FINE,
	DVR_VIDEO_QUALITY_NORMAL,
	DVR_VIDEO_QUALITY_NUM
}DVR_VIDEO_QUALITY;

typedef enum _tagDVR_PHOTO_TYPE
{
	DVR_PHOTO_TYPE_RAW,
	DVR_PHOTO_TYPE_BMP,
	DVR_PHOTO_TYPE_JPG,
	DVR_PHOTO_TYPE_NUM
}DVR_PHOTO_TYPE;

typedef enum _tagDVR_PHOTO_QUALITY
{
	DVR_PHOTO_QUALITY_SFINE,
	DVR_PHOTO_QUALITY_FINE,
	DVR_PHOTO_QUALITY_NORMAL,
	DVR_PHOTO_QUALITY_NUM
}DVR_PHOTO_QUALITY;

typedef enum
{
    DVR_VIEW_INDEX_FRONT = 0,
    DVR_VIEW_INDEX_REAR,
    DVR_VIEW_INDEX_LEFT,
    DVR_VIEW_INDEX_RIGHT,
    DVR_VIEW_INDEX_MATTS,
    DVR_VIEW_INDEX_NUM
}DVR_VIEW_INDEX;

typedef struct
{
    DVR_PHOTO_TYPE eType;
    DVR_PHOTO_QUALITY eQuality;
    DVR_VIEW_INDEX eIndex;
}DVR_PHOTO_PARAM;

typedef enum
{
	DVR_FOLDER_TYPE_ROOT = 0,
	DVR_FOLDER_TYPE_NORMAL,
	DVR_FOLDER_TYPE_EMERGENCY,
	DVR_FOLDER_TYPE_PHOTO,
	DVR_FOLDER_TYPE_DAS,
	DVR_FOLDER_TYPE_NUM
}DVR_FOLDER_TYPE;

typedef enum _tagDVR_DAS_TYPE
{
    DVR_DAS_TYPE_NONE = 0,
    DVR_DAS_TYPE_APA = 0x20,
    DVR_DAS_TYPE_ACC,
	DVR_DAS_TYPE_IACC,
	DVR_DAS_TYPE_NUM,
}DVR_DAS_TYPE;

typedef struct _tagDVR_DAS_MONITOR
{
	DVR_DAS_TYPE eType;
	DVR_BOOL	 bEnable;
}DVR_DAS_MONITOR;

typedef struct
{
	unsigned long long time_stamp;
	const char *file_location;
    const char *thumbnail_location;
    DVR_FOLDER_TYPE eType;
}DVR_ADDTODB_FILE_INFO;

typedef enum _tagDVR_METADATA_TYPE
{
	DVR_METADATA_TYPE_UNKNOWN = 0,
	DVR_METADATA_TYPE_TITLE,			/*!< Stored: media title string */
	DVR_METADATA_TYPE_ARTIST,			/*!< Stored: media artist string */
	DVR_METADATA_TYPE_ALBUM,			/*!< Stored: media album string */
	DVR_METADATA_TYPE_GENRE,			/*!< Stored: media genre type string */
	DVR_METADATA_TYPE_AUTHOR,			/*!< Stored: media author name string, composer */
	DVR_METADATA_TYPE_TRACK,			/*!< Stored: media track number in album (metadata)  */
	DVR_METADATA_TYPE_YEAR,				/*!< media published year number */
	DVR_METADATA_TYPE_DATE,				/*!< media published date time */
	DVR_METADATA_TYPE_COMMENT,			/*!< media comment string */
	DVR_METADATA_TYPE_PICTURE,			/*!< media cover picture buffer */
	DVR_METADATA_TYPE_COPYRIGHT,		/*!< media Copyright information string */
	DVR_METADATA_TYPE_DESCRIPTION,		/*!< media description string */
	DVR_METADATA_TYPE_LOCATION,			/*!< media location string */
	DVR_METADATA_TYPE_ORGANIZATION,		/*!< media organization string */
	DVR_METADATA_TYPE_APPLICATION		/*!< media application string */
} DVR_METADATA_TYPE;

typedef enum _tagDVR_METADATA_CODETYPE
{
	DVR_METADATA_CODETYPE_UNKNOWN = 0,
	DVR_METADATA_CODETYPE_INT,	//32 bits
	DVR_METADATA_CODETYPE_ANSI,
	DVR_METADATA_CODETYPE_UTF8,
	DVR_METADATA_CODETYPE_UTF16,
	DVR_METADATA_CODETYPE_MAC,
	DVR_METADATA_CODETYPE_JPG,
	DVR_METADATA_CODETYPE_PNG,
	DVR_METADATA_CODETYPE_QWORD, //64 bits
	DVR_METADATA_CODETYPE_WORD,  //16 bits
	DVR_METADATA_CODETYPE_BYTES  //BYTE array
}DVR_METADATA_CODETYPE;

typedef struct _tagDVR_METADATA_ITEM
{
	DVR_METADATA_TYPE     eType;
	DVR_METADATA_CODETYPE eCodeType;
	char *                ps8Data;
	unsigned int          u32DataSize;
}DVR_METADATA_ITEM;

typedef enum
{
	PREVIEW_RGB565,
	PREVIEW_RGB888,
	PREVIEW_RGB32,
	PREVIEW_YUV420,
	PREVIEW_YUV422
}DVR_PREVIEW_FORMAT;

typedef struct
{
	DVR_PREVIEW_FORMAT format;
	DVR_U32 ulPreviewHeight;
	DVR_U32 ulPreviewWidth;
	char filename[APP_MAX_FN_SIZE];
}DVR_PREVIEW_OPTION;

typedef DVR_RESULT(*PFN_DVR_SDK_NOTIFY)(_IN DVR_VOID *pContext, DVR_NOTIFICATION_TYPE eType, _IN DVR_VOID *pParam1, _IN DVR_VOID *pParam2);
typedef void(*PFN_DVR_SDK_CONSOLE)(_IN DVR_VOID *pContext, DVR_S32 nLevel, _IN const char *szString);
typedef void(*PFN_PLAYER_NEWFRAME)(_IN DVR_VOID *pContext, _IN DVR_VOID *pUserData);
typedef void(*PFN_APPTIMER_HANDLER)(DVR_U32 eid); /**< App Timer Handler*/
typedef void(*PFN_NEW_VEHICLE_DATA)(_IN DVR_VOID *pContext, _IN DVR_VOID *pUserData);

typedef enum _tagDVR_PLAYER_LOOP_NEXTFILE_MODE
{
	DVR_PLAYER_NEXTFILE_MODE_REPEAT,					/*!< Do not advance */
	DVR_PLAYER_NEXTFILE_MODE_SEQUENTIAL,				/*!< Advance sequentially */
	DVR_PLAYER_NEXTFILE_MODE_REVERSESEQUENTIAL,		/*!< Advance reverse sequentially */
	DVR_PLAYER_NEXTFILE_MODE_SHUFFLE,				/*!< Advance in shuffle mode */
	DVR_PLAYER_NEXTFILE_MODE_RANDOM,					/*!< Advance random mode */
} DVR_PLAYER_LOOP_NEXTFILE_MODE;

typedef enum _tagDVR_PLAYER_LOOP_SOURCE
{
	DVR_PLAYER_LOOP_SOURCE_FILE,						/*!< Single file */
	DVR_PLAYER_LOOP_SOURCE_DIRECTORY_VIDEO,				/*!< Directory */
    DVR_PLAYER_LOOP_SOURCE_DIRECTORY_IMAGE,
	DVR_PLAYER_LOOP_SOURCE_META,						/*!< Metadata */
} DVR_PLAYER_LOOP_SOURCE;

typedef enum _tagDVR_PLAYER_LOOP_RECURSIVE
{
	DVR_PLAYER_LOOP_RECURSIVE_ALLFILE,				/*!< all files in the leaf are played */
	DVR_PLAYER_LOOP_RECURSIVE_FIRSTFILE,				/*!< first file in the leaf is played */
} DVR_PLAYER_LOOP_RECURSIVE;

typedef enum _tagDVR_PLAYER_LOOP_SCANACTION
{
	DVR_PLAYER_LOOP_SCANACTION_CONTINUE,					/*!< Continue at current speed */
	DVR_PLAYER_LOOP_SCANACTION_FORWARDPLAY,				/*!< Change to forward 1x play */
} DVR_PLAYER_LOOP_SCANACTION;

typedef enum _tagDVR_PLAYER_LOOP_COMMAND
{
	DVR_PLAYER_LOOP_COMMAND_NULL = 0,				/*!< Ignored */
	DVR_PLAYER_LOOP_COMMAND_START,					/*!< Start loop playback.  Depending on bWaitForItem, can be used to update state. (&Action) (SizeofAction) (&State) (SizeofState) */
	DVR_PLAYER_LOOP_COMMAND_STOP,					/*!< Stop loop playback and file playback. (NULL) (0) (NULL) (0) */
	DVR_PLAYER_LOOP_COMMAND_PAUSE,					/*!< Pause loop playback - does not pause current playing file. (NULL) (0) (NULL) (0) */
	DVR_PLAYER_LOOP_COMMAND_RESUME,					/*!< Resume loop playback - does not resume file, but the handler when the file finishes. (NULL) (0) (NULL) (0)*/
	DVR_PLAYER_LOOP_COMMAND_ADVANCE,					/*!< Advance a certain number of items (can be forward pos or backward, neg). bPend means wait until current file is finished. (NULL) (nCount) (NULL) (bPend) */
	DVR_PLAYER_LOOP_COMMAND_SETCOUNT,				/*!< SetCount sets the internal counter (from which the index is derived) - nIndex is translated from nCount  by the eNextFileMode to determine the index in collection. bPend means wait until current file is finished.(NULL) (nCount) (NULL) (bPend) */
	DVR_PLAYER_LOOP_COMMAND_SETNEXTFILEMODE,			/*!< SetNextFileMode sets the next file mode - pParam is a pointer to int, the index in the collection to set. (NULL) (nValue) (NULL) (0) */
} DVR_PLAYER_LOOP_COMMAND;

typedef enum _tagDVR_DB_SCAN_STATE
{
	DVR_SCAN_STATE_PARTIAL,						/*!< Partial scan state available */
	DVR_SCAN_STATE_CACHED,						/*!< Cached index available */
	DVR_SCAN_STATE_COMPLETE,						/*!< Completed scan */
} DVR_DB_SCAN_STATE;

typedef struct _tagDVR_DB_IDXFILE
{
    DVR_FILE_TYPE   	eType;
    DVR_FOLDER_TYPE 	efdtype;
    unsigned short		idf;
    time_t       		mktime;
    DVR_U64 			length;
    char            	filename[APP_MAX_FN_SIZE];
}DVR_DB_IDXFILE,*PDVR_DB_IDXFILE;

typedef struct _tagDVR_FILEMAP_META_ITEM
{
    char szMediaFileName[APP_MAX_FN_SIZE];
    char szThumbNailName[APP_MAX_FN_SIZE];
}DVR_FILEMAP_META_ITEM;

typedef struct _tagDVR_DEVICE
{
	DVR_DEVICE_TYPE eType;		/*!< Type of device */
	int intfClass;
	int nPartitionIndex;			/*!< 0 is primary (or first partition).   */
	char szMountPoint[PATH_MAX];
	char szDevicePath[PATH_MAX];
} DVR_DEVICE;

typedef struct _tagDVR_DEVICE_ARRAY
{
	int nDeviceNum;
	DVR_DEVICE *pDriveArray[MAX_DEVICE_NUM];
} DVR_DEVICE_ARRAY;

typedef struct _tagDVR_RECORDER_FILE_OP
{
	DVR_FILE_EVENT eEvent;			
	char szLocation[PATH_MAX];
    char szTNLocation[PATH_MAX];
    DVR_FOLDER_TYPE eType;
	unsigned long long nTimeStamp;
} DVR_RECORDER_FILE_OP;

typedef struct _tagDVR_HCMGR_HANDLER
{
	void(*HandlerMain)(void *ptr);  /**< Main handler. */
	int(*HandlerExit)(void);   /**< Exit handler. */
}DVR_HCMGR_HANDLER;

typedef struct _tagAPP_MESSAGE
{
	DVR_U32 MessageID;      /**< Message Id.*/
	DVR_U32 MessageData[2]; /**< Message data.*/
} APP_MESSAGE;

typedef struct
{
	int(*FuncSearch) (DVR_U32 param1, DVR_U32 param2, void *pContext);/**< Search CB Function */
	int(*FuncHandle) (DVR_U32 param1, DVR_U32 param2, void *pContext);/**< Handle CB function */
    int(*FuncReturn) (DVR_U32 param1, DVR_U32 param2, void *pContext);/**< Return CB Function */
	DVR_U32 Command;/**< Function Set corresponding Command */
	void *pContext;
}DVR_STORAGE_HANDLER;

/**info[1:0] represent step finish*/
#define DVR_LOOP_ENC_SEARCH_DONE			0x01       /**<LOOP ENCODE STATUS MSG*/
#define DVR_LOOP_ENC_HANDLE_DONE			0x02       /**<LOOP ENCODE STATUS MSG*/
#define DVR_LOOP_ENC_SEARCH_ERROR			0x04        /**<LOOP ENCODE STATUS MSG*/
#define DVR_LOOP_ENC_HANDLE_ERROR			0x08        /**<LOOP ENCODE STATUS MSG*/
#define DVR_LOOP_ENC_CHECK_SEARCH			0x10        /**<LOOP ENCODE STATUS MSG*/
#define DVR_LOOP_ENC_CHECK_HANDLE			0x20        /**<LOOP ENCODE STATUS MSG*/
#define DVR_LOOP_ENC_TYPE_IMAGE				0x40
#define DVR_LOOP_ENC_TYPE_VIDEO				0x80

/* Card status check flags */
#define DVR_STORAGE_CARD_CHECK_WRITE        (0x00000001)
#define DVR_STORAGE_CARD_CHECK_DELETE       (0x00000002)
#define DVR_STORAGE_CARD_CHECK_ID_CHANGE    (0x00000004)
#define DVR_STORAGE_CARD_CHECK_MODIFY       (0x00000008)
#define DVR_STORAGE_CARD_CHECK_CONT_WRITE   (0x0000000A)
#define DVR_STORAGE_CARD_CHECK_PRESENT      (0x40000000)
#define DVR_STORAGE_CARD_CHECK_RESET        (0x80000000)
/* Card status check return values */
#define DVR_STORAGE_CARD_STATUS_CHECK_PASS          (0x00000000)
#define DVR_STORAGE_CARD_STATUS_NO_CARD             (0x00000001)
#define DVR_STORAGE_CARD_STATUS_UNFORMAT_CARD       (0x00000002)
#define DVR_STORAGE_CARD_STATUS_NOT_ENOUGH_SPACE    (0x00000004)
#define DVR_STORAGE_CARD_STATUS_INVALID_CARD        (0x00000008)
#define DVR_STORAGE_CARD_STATUS_WP_CARD             (0x00000010)
#define DVR_STORAGE_CARD_STATUS_REFRESHING          (0x00000020)
#define DVR_STORAGE_CARD_STATUS_ERROR_FORMAT        (0x00000040)
#define DVR_STORAGE_CARD_STATUS_CHK_FRGMT_FAIL		(0x00000080)
#define DVR_STORAGE_CARD_STATUS_EVENT_FOLDER_NOT_ENOUGH_SPACE	(0x00000100)
#define DVR_STORAGE_CARD_STATUS_PHOTO_FOLDER_NOT_ENOUGH_SPACE	(0x00000200)
#define DVR_STORAGE_CARD_STATUS_LOW_SPEED	(0x00000400)
#define DVR_STORAGE_CARD_STATUS_NOT_READY	(0x00008000)

/**
*  Defines for Module ID
*/
#define MDL_APPLIB_SYSTEM_ID		0x01
#define MDL_APPLIB_RECORDER_ID		0x02
#define MDL_APPLIB_PLAYER_ID		0x03
#define MDL_APPLIB_MONITOR_ID		0x04
#define MDL_APPLIB_STORAGE_ID		0x05
#define MDL_APPLIB_COMSVC_ID		0x06

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

#define MSG_NULL_ID             0x00000000

/**********************************************************************/
/* MDL_APPLIB_RECORDER_ID messages                                        */
/**********************************************************************/
/**
* Partition: |31 - 27|26 - 24|23 - 16|15 -  8| 7 -  0|
*   |31 - 27|: MDL_APPLIB_RECORDER_ID
*   |26 - 24|: MSG_TYPE_HMI
*   |23 - 16|: module or interface type ID
*   |15 -  8|: Self-defined
*   | 7 -  0|: Self-defined
* Note:
*   bit 0-15 could be defined in the module itself (individual
*   header files). However, module ID should be defined here for arrangement
**/
#define HMSG_RECORDER_MODULE(x)  MSG_ID(MDL_APPLIB_RECORDER_ID, MSG_TYPE_HMI, (x))
/** Sub-group:type of interface events */
#define HMSG_RECORDER_MODULE_ID_COMMON  (0x01)
#define HMSG_RECORDER_MODULE_ID_VIDEO   (0x02)
#define HMSG_RECORDER_MODULE_ID_STILL   (0x03)
#define HMSG_RECORDER_MODULE_ID_ERROR   (0x04)

/** The recorder status.*/
#define HMSG_RECORDER_MODULE_COMMON(x)  HMSG_RECORDER_MODULE(((DVR_U32)HMSG_RECORDER_MODULE_ID_COMMON << 16) | (x))
#define HMSG_RECORDER_STATE_IDLE                HMSG_RECORDER_MODULE_COMMON(0x0000)
#define HMSG_RECORDER_STATE_LIVEVIEW            HMSG_RECORDER_MODULE_COMMON(0x0001)
#define HMSG_RECORDER_STATE_ILLEGAL_SIGNAL      HMSG_RECORDER_MODULE_COMMON(0x0002)

/** The recorder status about video recording.*/
#define HMSG_RECORDER_MODULE_VIDEO(x)   HMSG_RECORDER_MODULE(((DVR_U32)HMSG_RECORDER_MODULE_ID_VIDEO << 16) | (x))
#define HMSG_RECORDER_STATE_RECORDING           HMSG_RECORDER_MODULE_VIDEO(0x0001)
#define HMSG_RECORDER_STATE_RECORDING_PAUSE     HMSG_RECORDER_MODULE_VIDEO(0x0002)
#define HMSG_RECORDER_STATE_PRERECORD           HMSG_RECORDER_MODULE_VIDEO(0x0003)

/**********************************************************************/
/* MDL_APPLIB_STORAGE_ID messages                                        */
/**********************************************************************/
/**
* Partition: |31 - 27|26 - 24|23 - 16|15 -  8| 7 -  0|
*   |31 - 27|: MDL_APPLIB_STORAGE_ID
*   |26 - 24|: MSG_TYPE_HMI
*   |23 - 16|: module or interface type ID
*   |15 -  8|: Self-defined
*   | 7 -  0|: Self-defined
* Note:
*   bit 0-15 could be defined in the module itself (individual
*   header files). However, module ID should be defined here for arrangement
**/
#define HMSG_STORAGE_MODULE(x)			MSG_ID(MDL_APPLIB_STORAGE_ID, MSG_TYPE_HMI, (x))
/** Sub-group:type of app library & interface events */
#define HMSG_STORAGE_MODULE_ID_COMMON   (0x01)
#define HMSG_STORAGE_MODULE_ID_ASYNCOP  (0x02)

#define HMSG_STORAGE_MODULE_COMMON(x)			HMSG_STORAGE_MODULE(((DVR_U32)HMSG_STORAGE_MODULE_ID_COMMON << 16) | (x))
#define HMSG_STORAGE_BUSY						HMSG_STORAGE_MODULE_COMMON(0x0001)
#define HMSG_STORAGE_IDLE						HMSG_STORAGE_MODULE_COMMON(0x0002)
#define HMSG_STORAGE_TOO_FRAGMENTED				HMSG_STORAGE_MODULE_COMMON(0x0003)
#define HMSG_STORAGE_RUNOUT						HMSG_STORAGE_MODULE_COMMON(0x0004)
#define HMSG_STORAGE_IO_TOO_SLOW				HMSG_STORAGE_MODULE_COMMON(0x0005)
#define HMSG_STORAGE_REACH_FILE_LIMIT			HMSG_STORAGE_MODULE_COMMON(0x0006)
#define HMSG_STORAGE_REACH_FILE_NUMBER			HMSG_STORAGE_MODULE_COMMON(0x0007)

/**async op msg*/
#define HMSG_STORAGE_MODULE_ASYNC_OP(x)			HMSG_STORAGE_MODULE(((DVR_U32)HMSG_STORAGE_MODULE_ID_ASYNCOP << 16) | (x))
/**The status about loop encode*/
#define HMSG_STORAGE_RUNOUT_HANDLE_DONE			HMSG_STORAGE_MODULE_ASYNC_OP(0x0001)
#define HMSG_STORAGE_RUNOUT_HANDLE_ERROR		HMSG_STORAGE_MODULE_ASYNC_OP(0x0002)
#define HMSG_STORAGE_RUNOUT_HANDLE_START		HMSG_STORAGE_MODULE_ASYNC_OP(0x0003)
/**auto playback */
#define HMSG_AUTO_PLAY_PREV						HMSG_STORAGE_MODULE_ASYNC_OP(0x0004)
#define HMSG_AUTO_PLAY_NEXT						HMSG_STORAGE_MODULE_ASYNC_OP(0x0005)

#define HMSG_STORAGE_DAS_RUNOUT_HANDLE_START		HMSG_STORAGE_MODULE_ASYNC_OP(0x0008)
#define HMSG_STORAGE_DAS_NUM_RUNOUT_HANDLE_START	HMSG_STORAGE_MODULE_ASYNC_OP(0x0009)

/**********************************************************************/
/* MDL_APPLIB_COMSVC_ID messages                                        */
/**********************************************************************/
/**
* Partition: |31 - 27|26 - 24|23 - 16|15 -  8| 7 -  0|
*   |31 - 27|: MDL_APPLIB_COMSVC_ID
*   |26 - 24|: MSG_TYPE_HMI
*   |23 - 16|: module or interface type ID
*   |15 -  8|: Self-defined
*   | 7 -  0|: Self-defined
* Note:
*   bit 0-15 could be defined in the module itself (individual
*   header files). However, module ID should be defined here
*   for arrangement
**/
#define HMSG_COMSVC_MODULE(x)					MSG_ID(MDL_APPLIB_COMSVC_ID, MSG_TYPE_HMI, (x))

#define HMSG_COMSVC_MODULE_ID_ASYNC            (0x01) /**< Sub-group:type of app library & interface events */
#define HMSG_COMSVC_MODULE_ID_TIMER            (0x02) /**< Sub-group:type of app library & interface events */

/** Async op module events */
#define HMSG_COMSVC_MODULE_ASYNC(x)				HMSG_COMSVC_MODULE(((DVR_U32)HMSG_COMSVC_MODULE_ID_ASYNC << 16) | (x))
#define ASYNC_MGR_CMD_SHUTDOWN					HMSG_COMSVC_MODULE_ASYNC(0x0001)           
#define ASYNC_MGR_CMD_CARD_FORMAT				HMSG_COMSVC_MODULE_ASYNC(0x0002)           
#define ASYNC_MGR_CMD_CARD_INSERT				HMSG_COMSVC_MODULE_ASYNC(0x0003)           
#define ASYNC_MGR_CMD_FILE_COPY					HMSG_COMSVC_MODULE_ASYNC(0x0004)           
#define ASYNC_MGR_CMD_FILE_MOVE					HMSG_COMSVC_MODULE_ASYNC(0x0005)           
#define ASYNC_MGR_CMD_FILE_DEL					HMSG_COMSVC_MODULE_ASYNC(0x0006)           
#define ASYNC_MGR_CMD_DMF_FCOPY					HMSG_COMSVC_MODULE_ASYNC(0x0007)           
#define ASYNC_MGR_CMD_DMF_FMOVE					HMSG_COMSVC_MODULE_ASYNC(0x0008)           
#define ASYNC_MGR_CMD_DMF_FDEL					HMSG_COMSVC_MODULE_ASYNC(0x0009)

#define ASYNC_MGR_MSG_OP_DONE(x)    ((x) | 0x8000)        /**<ASYNC_MGR_MSG_OP_DONE(x)*/
#define ASYNC_MGR_MSG_SHUTDOWN_DONE				ASYNC_MGR_MSG_OP_DONE(ASYNC_MGR_CMD_SHUTDOWN)                  
#define ASYNC_MGR_MSG_CARD_FORMAT_DONE			ASYNC_MGR_MSG_OP_DONE(ASYNC_MGR_CMD_CARD_FORMAT)               
#define ASYNC_MGR_MSG_CARD_INSERT_DONE			ASYNC_MGR_MSG_OP_DONE(ASYNC_MGR_CMD_CARD_INSERT)               
#define ASYNC_MGR_MSG_FILE_COPY_DONE			ASYNC_MGR_MSG_OP_DONE(ASYNC_MGR_CMD_FILE_COPY)                 
#define ASYNC_MGR_MSG_FILE_MOVE_DONE			ASYNC_MGR_MSG_OP_DONE(ASYNC_MGR_CMD_FILE_MOVE)                 
#define ASYNC_MGR_MSG_FILE_DEL_DONE				ASYNC_MGR_MSG_OP_DONE(ASYNC_MGR_CMD_FILE_DEL)                  
#define ASYNC_MGR_MSG_DMF_FCOPY_DONE			ASYNC_MGR_MSG_OP_DONE(ASYNC_MGR_CMD_DMF_FCOPY)                 
#define ASYNC_MGR_MSG_DMF_FMOVE_DONE			ASYNC_MGR_MSG_OP_DONE(ASYNC_MGR_CMD_DMF_FMOVE)                 
#define ASYNC_MGR_MSG_DMF_FDEL_DONE				ASYNC_MGR_MSG_OP_DONE(ASYNC_MGR_CMD_DMF_FDEL)                  

/** Timer module events */
enum
{
	TIMER_CHECK = 0,
	TIMER_1HZ,
	TIMER_2HZ,
	TIMER_4HZ,
	TIMER_10HZ,
	TIMER_20HZ,
	TIMER_5S,
	TIMER_30S,
	TIMER_NUM
};
#define HMSG_COMSVC_MODULE_TIMER(x)				HMSG_COMSVC_MODULE(((DVR_U32)HMSG_COMSVC_MODULE_ID_TIMER << 16) | (x))
#define HMSG_TIMER_CHECK						HMSG_COMSVC_MODULE_TIMER(TIMER_CHECK)  /**<HMSG_TIMER_CHECK*/
#define HMSG_TIMER_1HZ							HMSG_COMSVC_MODULE_TIMER(TIMER_1HZ)    /**<HMSG_TIMER_1HZ  */
#define HMSG_TIMER_2HZ							HMSG_COMSVC_MODULE_TIMER(TIMER_2HZ)    /**<HMSG_TIMER_2HZ  */
#define HMSG_TIMER_4HZ							HMSG_COMSVC_MODULE_TIMER(TIMER_4HZ)    /**<HMSG_TIMER_4HZ  */
#define HMSG_TIMER_10HZ							HMSG_COMSVC_MODULE_TIMER(TIMER_10HZ)   /**<HMSG_TIMER_10HZ */
#define HMSG_TIMER_20HZ							HMSG_COMSVC_MODULE_TIMER(TIMER_20HZ)   /**<HMSG_TIMER_20HZ */
#define HMSG_TIMER_5S							HMSG_COMSVC_MODULE_TIMER(TIMER_5S)     /**<HMSG_TIMER_5S   */
#define HMSG_TIMER_30S							HMSG_COMSVC_MODULE_TIMER(TIMER_30S)    /**<HMSG_TIMER_30S  */

#define TIMER_TICK              (1)  /**<TIMER_TICK              (1) */
#define TIMER_UNREGISTER        (2)  /**<TIMER_UNREGISTER        (2) */




#define SD_MOUNT_POINT   "/media/data"



#endif  // ~_DVR_SDK_DEF_H_

