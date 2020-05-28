#ifndef _DVR_PHOTO_H_
#define _DVR_PHOTO_H_

#include <gst/gst.h>
#include <gst/video/video.h>

//exif
#define EXIF_MODEL_SIZE             100
#define EXIF_MAKE_SIZE              100
#define EXIF_DATE_TIME_SIZE         20

#define GPS_MIN_DIV                 60
#define GPS_SEC_DIV                 60
#define GPS_SEC_ACCURACY            1000
#define GPS_TIMESTAMP_SIZE          6
#define GPS_DATESTAMP_SIZE          11
#define GPS_REF_SIZE                2
#define GPS_MAPDATUM_SIZE           100
#define GPS_PROCESSING_SIZE         100
#define GPS_VERSION_SIZE            4
#define GPS_NORTH_REF               "N"
#define GPS_SOUTH_REF               "S"
#define GPS_EAST_REF                "E"
#define GPS_WEST_REF                "W"

typedef struct
{
	guint		width;
	guint		height;

	gboolean    bExifEnable;
	guint		imageOri;

	gchar       dateTime[EXIF_DATE_TIME_SIZE];
	gchar       exifmake[EXIF_MAKE_SIZE];
	gchar       exifmodel[EXIF_MODEL_SIZE];
	guint		focalLengthL;
	guint		focalLengthH;
	gboolean    bGPS;
	guint		gpsLATL[3];//latitude
	guint		gpsLATH[3];
	guint		gpsLATREF; //N:0,S:1

	guint		gpsLONGL[3];//longitude
	guint		gpsLONGH[3];
	guint		gpsLONGREF; //E:0,W:1

	guint		gpsALTL;
	guint		gpsALTH;
	guint		gpsALTREF; //Sea level:0, under sea level:1

	gchar       gpsProcessMethod[GPS_PROCESSING_SIZE];
	guint		gpsTimeL[3];
	guint		gpsTimeH[3];
	gchar       gpsDatestamp[GPS_DATESTAMP_SIZE];
}CAMERA_EXIF;

typedef enum
{
	PHOTO_PROP_DIR,
	PHOTO_PROP_QUALITY,
	PHOTO_PROP_FORMAT,
	PHOTO_PROP_CAMERA_EXIF
}PHOTO_PROP_ID;

typedef struct  
{
    DVR_U16 TimeYear;
    DVR_U16 TimeMon;
    DVR_U16 TimeDay;
    DVR_U16 TimeHour;
    DVR_U16 TimeMin;
    DVR_U16 TimeSec;
    DVR_U16 VehicleSpeed;
	DVR_U8 VehicleSpeedValidity;
    DVR_U8 GearShiftPositon;
    DVR_U8 BrakePedalStatus;
    DVR_U8 DriverBuckleSwitchStatus;
    DVR_U8 AccePedalPosition;
    DVR_U8 TurnSignal;
    DVR_U32 GpsLongitude;
    DVR_U32 GpsLatitude;
	DVR_F32 LateralAcceleration;
	DVR_F32 LongitAcceleration;
	DVR_U8 EmergencyLightstatus;
	DVR_U8 APALSCAction;
	DVR_U8 IACCTakeoverReq;
	DVR_U8 ACCTakeOverReq;
	DVR_U8 AEBDecCtrlAvail;
}PHOTO_VEHICLE_INFO;

typedef struct
{
	void *pImageBuf[4];
	int width;
	int height;
    DVR_PHOTO_TYPE eType;
    DVR_PHOTO_QUALITY eQuality;
    DVR_VIEW_INDEX eIndex;
    PHOTO_VEHICLE_INFO stVehInfo;
}PHOTO_INPUT_PARAM;

typedef struct
{
	GstVideoFormat format;
	gint width;
	gint height;
}PHOTO_FORMAT_SETTING;

typedef struct  
{
	void *data;
	int size;
}PHOTO_OUT_MEMORY;

typedef int(*PFN_DBADDFILE)(const char *pLocation, const char *pThumbNailLocation, void *pContext);
typedef PHOTO_OUT_MEMORY *(*PFN_PHOTO_REQUEST_MEMORY)(int buf_size, unsigned int num_bufs);

#endif
