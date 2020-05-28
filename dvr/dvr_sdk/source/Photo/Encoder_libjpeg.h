#ifndef _ENCODER_LIBJPEG_H_
#define _ENCODER_LIBJPEG_H_

#include "jhead/jhead.h"
#include <jpeglib.h>
#include "osa.h"

#include "DVR_PHOTO_DEF.h"
#include "ThumbNail.h"

/**
* libjpeg encoder class - uses libjpeg to encode yuv
*/

#define MAX_EXIF_TAGS_SUPPORTED 30

static const char TAG_MODEL[] = "Model";
static const char TAG_MAKE[] = "Make";
static const char TAG_FOCALLENGTH[] = "FocalLength";
static const char TAG_DATETIME[] = "DateTime";
static const char TAG_IMAGE_WIDTH[] = "ImageWidth";
static const char TAG_IMAGE_LENGTH[] = "ImageLength";
static const char TAG_GPS_LAT[] = "GPSLatitude";
static const char TAG_GPS_LAT_REF[] = "GPSLatitudeRef";
static const char TAG_GPS_LONG[] = "GPSLongitude";
static const char TAG_GPS_LONG_REF[] = "GPSLongitudeRef";
static const char TAG_GPS_ALT[] = "GPSAltitude";
static const char TAG_GPS_ALT_REF[] = "GPSAltitudeRef";
static const char TAG_GPS_MAP_DATUM[] = "GPSMapDatum";
static const char TAG_GPS_PROCESSING_METHOD[] = "GPSProcessingMethod";
static const char TAG_GPS_VERSION_ID[] = "GPSVersionID";
static const char TAG_GPS_TIMESTAMP[] = "GPSTimeStamp";
static const char TAG_GPS_DATESTAMP[] = "GPSDateStamp";
static const char TAG_ORIENTATION[] = "Orientation";

class ExifElementsTable
{
public:
	ExifElementsTable() :
		gps_tag_count(0), exif_tag_count(0), position(0),
		jpeg_opened(false), has_datetime_tag(false) 
	{
		memset(table, 0, sizeof(table));
	}
	~ExifElementsTable();

	int insertElement(const char* tag, const char* value);
	void insertExifToJpeg(unsigned char* jpeg, size_t jpeg_size);
	int insertExifThumbnailImage(const char*, int);
	void saveJpeg(unsigned char* picture, size_t jpeg_size);
	static const char* degreesToExifOrientation(unsigned int);
	static void stringToRational(const char*, unsigned int*, unsigned int*);
	static bool isAsciiTag(const char* tag);
private:
	ExifElement_t table[MAX_EXIF_TAGS_SUPPORTED];
	unsigned int gps_tag_count;
	unsigned int exif_tag_count;
	unsigned int position;
	bool jpeg_opened;
	bool has_datetime_tag;
};

typedef struct
{
	guint channels;

	gint inc[GST_VIDEO_MAX_COMPONENTS];
	guint cwidth[GST_VIDEO_MAX_COMPONENTS];
	guint cheight[GST_VIDEO_MAX_COMPONENTS];
	guint h_samp[GST_VIDEO_MAX_COMPONENTS];
	guint v_samp[GST_VIDEO_MAX_COMPONENTS];
	guint h_max_samp;
	guint v_max_samp;
	gboolean planar;
	gint sof_marker;
	/* the video buffer */
	gint bufsize;
	/* the jpeg line buffer */
	guchar **line[3];
	/* indirect encoding line buffers */
	guchar *row[3][4 * DCTSIZE];
}JPEGEncoderInstance;

class Encoder_libjpeg
{
	/* public member types and variables */
public:
	typedef struct 
	{
		gchar* src;
		int src_size;
		gchar* dst;
		int dst_size;
		int in_width;
		int in_height;
		int out_width;
		int out_height;
		int out_format;
		guint jpeg_size;
		gboolean valid;
	}params;

	/* public member functions */
public:
	Encoder_libjpeg(CThumbNail *pTNHandle);
	~Encoder_libjpeg();

	PHOTO_OUT_MEMORY* Encode(params* main_jpeg, params* tn_jpeg, ExifElementsTable* pExifData);

	void SetFormat(PHOTO_FORMAT_SETTING *pSet);
	void SetQuality(int quality);
	int OutBufSize()
	{
		return m_jpegInst.bufsize;
	}

	int InBufSize()
	{
		return m_videoInfo.size;
	}

	void SetCallback(PFN_PHOTO_REQUEST_MEMORY callback)
	{
		mRequestMemory = callback;
	}

	int convertEXIF_libjpeg(CAMERA_EXIF* exif, ExifElementsTable* exifTable, PHOTO_INPUT_PARAM *frame);

private:
	PFN_PHOTO_REQUEST_MEMORY mRequestMemory;

private:
	void jpegenc_init(void);
	void jpegenc_finalize(void);
	gboolean jpegenc_set_format(GstVideoInfo *info);
	void jpegenc_resync(void);
	GstFlowReturn jpegenc_handle_frame(params* input);
	gboolean jpegenc_start(void);
	gboolean jpegenc_stop(void);
	int jpegenc_set_property(guint prop_id, int value);
	int jpegenc_get_property(guint prop_id, int *value);

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
//	struct jpeg_destination_mgr jdest;

	/* properties */
	gint m_quality;
	gint m_smoothing;
	gint m_idct_method;

	JPEGEncoderInstance m_jpegInst;
	GstVideoInfo m_videoInfo;

private:
	CThumbNail *m_pThumbNail;
};

#endif