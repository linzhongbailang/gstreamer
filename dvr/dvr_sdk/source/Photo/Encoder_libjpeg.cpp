#include <math.h>
#include "Encoder_libjpeg.h"

#if _MSC_VER
#define snprintf _snprintf
#pragma warning(disable:4996)	// 4706: assignment within conditional expression
#endif

enum
{
	PROP_0,
	PROP_QUALITY,
	PROP_SMOOTHING,
	PROP_IDCT_METHOD
};

#define JPEG_DEFAULT_QUALITY		85
#define JPEG_DEFAULT_SMOOTHING		0
#define JPEG_DEFAULT_IDCT_METHOD	JDCT_FASTEST

#define ARRAY_SIZE(array) (sizeof((array)) / sizeof((array)[0]))

typedef struct 
{
	unsigned int integer;
	const char* string;
}integer_string_pair;


static integer_string_pair degress_to_exif_lut[] =
{
	// degrees, exif_orientation
	{ 0, "1" },
	{ 90, "6" },
	{ 180, "3" },
	{ 270, "8" },
};

const char* ExifElementsTable::degreesToExifOrientation(unsigned int degrees)
{
	for (unsigned int i = 0; i < ARRAY_SIZE(degress_to_exif_lut); i++)
	{
		if (degrees == degress_to_exif_lut[i].integer)
		{
			return degress_to_exif_lut[i].string;
		}
	}
	return NULL;
}

void ExifElementsTable::stringToRational(const char* str, unsigned int* num, unsigned int* den)
{
	int len;
	char * tempVal = NULL;

	if (str != NULL)
	{
		len = strlen(str);
		tempVal = (char*)malloc(sizeof(char) * (len + 1));
	}

	if (tempVal != NULL)
	{
		// convert the decimal string into a rational
		int den_len;
		char *ctx;
		unsigned int numerator = 0;
		unsigned int denominator = 0;
		char* temp = NULL;

		memset(tempVal, '\0', len + 1);
		strncpy(tempVal, str, len);
#ifdef WIN32
		temp = strtok(tempVal, ".");
#else
		temp = strtok_r(tempVal, ".", &ctx);
#endif

		if (temp != NULL)
			numerator = atoi(temp);

		if (!numerator)
			numerator = 1;

#ifdef WIN32
		temp = strtok(NULL, ".");
#else
		temp = strtok_r(NULL, ".", &ctx);
#endif
		if (temp != NULL)
		{
			den_len = strlen(temp);
			if (HUGE_VAL == den_len)
			{
				den_len = 0;
			}

			denominator = static_cast<unsigned int>(pow(10.0f, den_len));
			numerator = numerator * denominator + atoi(temp);
		}
		else
		{
			denominator = 1;
		}

		free(tempVal);

		*num = numerator;
		*den = denominator;
	}
}

bool ExifElementsTable::isAsciiTag(const char* tag)
{
	// TODO(XXX): Add tags as necessary
	return (strcmp(tag, TAG_GPS_PROCESSING_METHOD) == 0);
}

void ExifElementsTable::insertExifToJpeg(unsigned char* jpeg, size_t jpeg_size)
{
	ReadMode_t read_mode = (ReadMode_t)(READ_METADATA | READ_IMAGE);

	ResetJpgfile();
	if (ReadJpegSectionsFromBuffer(jpeg, jpeg_size, read_mode))
	{
		jpeg_opened = true;
		create_EXIF(table, exif_tag_count, gps_tag_count, has_datetime_tag, false);
	}
}

int ExifElementsTable::insertExifThumbnailImage(const char* thumb, int len)
{
	int ret = 0;

	if ((len > 0) && jpeg_opened)
	{
		ret = ReplaceThumbnailFromBuffer(thumb, len);
		//printf("insertExifThumbnailImage. ReplaceThumbnail(). ret=%d", ret);
	}

	return ret;
}

void ExifElementsTable::saveJpeg(unsigned char* jpeg, size_t jpeg_size)
{
	if (jpeg_opened)
	{
		WriteJpegToBuffer(jpeg, jpeg_size);
		DiscardData();
		jpeg_opened = false;
	}
}

/* public functions */
ExifElementsTable::~ExifElementsTable()
{
	int num_elements = gps_tag_count + exif_tag_count;
	for (int i = 0; i < num_elements; i++)
	{
		if (table[i].Value)
		{
			free(table[i].Value);
		}
	}

	if (jpeg_opened)
	{
		DiscardData();
	}
}

int ExifElementsTable::insertElement(const char* tag, const char* value)
{
	unsigned int value_length = 0;
	int ret = 0;

	if (!value || !tag)
	{
		return -EINVAL;
	}

	if (position >= MAX_EXIF_TAGS_SUPPORTED)
	{
		printf("Max number of EXIF elements already inserted");
		return -1;
	}

	if (isAsciiTag(tag))
	{
		value_length = sizeof(ExifAsciiPrefix) + strlen(value + sizeof(ExifAsciiPrefix));
	}
	else
	{
		value_length = strlen(value);
	}

	if (IsGpsTag(tag))
	{
		table[position].GpsTag = TRUE;
		table[position].Tag = GpsTagNameToValue(tag);
		gps_tag_count++;
	}
	else
	{
		table[position].GpsTag = FALSE;
		table[position].Tag = TagNameToValue(tag);
		exif_tag_count++;

		if (strcmp(tag, TAG_DATETIME) == 0)
		{
			has_datetime_tag = true;
		}
	}

	table[position].DataLength = 0;
	table[position].Value = (char*)malloc(sizeof(char) * (value_length + 1));

	if (table[position].Value)
	{
		memcpy(table[position].Value, value, value_length + 1);
		table[position].DataLength = value_length + 1;
	}

	position++;
	return ret;
}

Encoder_libjpeg::Encoder_libjpeg(CThumbNail *pTNHandle)
{
	mRequestMemory = NULL;
	m_pThumbNail = pTNHandle;

	jpegenc_init();
	jpegenc_start();
}

Encoder_libjpeg::~Encoder_libjpeg()
{
	m_pThumbNail = NULL;

	jpegenc_stop();
	jpegenc_finalize();
}

class libjpeg_destination_mgr : public jpeg_destination_mgr
{
public:
	libjpeg_destination_mgr(gchar* input, int size);

	gchar* buf;
	int bufsize;
	guint jpegsize;
};

static void libjpeg_init_destination(j_compress_ptr cinfo)
{
	libjpeg_destination_mgr* dest = (libjpeg_destination_mgr*)cinfo->dest;

	dest->next_output_byte = (JOCTET *)dest->buf;
	dest->free_in_buffer = dest->bufsize;
	dest->jpegsize = 0;
}

static boolean libjpeg_empty_output_buffer(j_compress_ptr cinfo)
{
	libjpeg_destination_mgr* dest = (libjpeg_destination_mgr*)cinfo->dest;

	dest->next_output_byte = (JOCTET *)dest->buf;
	dest->free_in_buffer = dest->bufsize;
	return TRUE; // ?
}

static void libjpeg_term_destination(j_compress_ptr cinfo)
{
	libjpeg_destination_mgr* dest = (libjpeg_destination_mgr*)cinfo->dest;
	dest->jpegsize = dest->bufsize - dest->free_in_buffer;
}

libjpeg_destination_mgr::libjpeg_destination_mgr(gchar* input, int size)
{
	this->init_destination = libjpeg_init_destination;
	this->empty_output_buffer = libjpeg_empty_output_buffer;
	this->term_destination = libjpeg_term_destination;

	this->buf = input;
	this->bufsize = size;

	this->jpegsize = 0;
}

void Encoder_libjpeg::jpegenc_init(void)
{
	/* setup jpeglib */
	memset(&cinfo, 0, sizeof(cinfo));
	memset(&jerr, 0, sizeof(jerr));
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	/* init properties */
	m_quality = JPEG_DEFAULT_QUALITY;
	m_smoothing = JPEG_DEFAULT_SMOOTHING;
	m_idct_method = JPEG_DEFAULT_IDCT_METHOD;
	gst_video_info_init(&m_videoInfo);

	memset(&m_jpegInst, 0, sizeof(m_jpegInst));
}

void Encoder_libjpeg::jpegenc_finalize(void)
{
	jpeg_destroy_compress(&cinfo);
}

gboolean Encoder_libjpeg::jpegenc_set_format(GstVideoInfo *info)
{
	guint i;

	/* prepare a cached image description  */
	m_jpegInst.channels = GST_VIDEO_INFO_N_COMPONENTS(info);

	/* ... but any alpha is disregarded in encoding */
	if (GST_VIDEO_INFO_IS_GRAY(info))
		m_jpegInst.channels = 1;

	m_jpegInst.h_max_samp = 0;
	m_jpegInst.v_max_samp = 0;
	for (i = 0; i < m_jpegInst.channels; ++i) {
		m_jpegInst.cwidth[i] = GST_VIDEO_INFO_COMP_WIDTH(info, i);
		m_jpegInst.cheight[i] = GST_VIDEO_INFO_COMP_HEIGHT(info, i);
		m_jpegInst.inc[i] = GST_VIDEO_INFO_COMP_PSTRIDE(info, i);
		m_jpegInst.h_samp[i] =
			GST_ROUND_UP_4(GST_VIDEO_INFO_WIDTH(info)) / m_jpegInst.cwidth[i];
		m_jpegInst.h_max_samp = MAX(m_jpegInst.h_max_samp, m_jpegInst.h_samp[i]);
		m_jpegInst.v_samp[i] =
			GST_ROUND_UP_4(GST_VIDEO_INFO_HEIGHT(info)) / m_jpegInst.cheight[i];
		m_jpegInst.v_max_samp = MAX(m_jpegInst.v_max_samp, m_jpegInst.v_samp[i]);
	}
	/* samp should only be 1, 2 or 4 */
	g_assert(m_jpegInst.h_max_samp <= 4);
	g_assert(m_jpegInst.v_max_samp <= 4);

	/* now invert */
	/* maximum is invariant, as one of the components should have samp 1 */
	for (i = 0; i < m_jpegInst.channels; ++i) {
		GST_DEBUG("%d %d", m_jpegInst.h_samp[i], m_jpegInst.h_max_samp);
		m_jpegInst.h_samp[i] = m_jpegInst.h_max_samp / m_jpegInst.h_samp[i];
		m_jpegInst.v_samp[i] = m_jpegInst.v_max_samp / m_jpegInst.v_samp[i];
	}
	m_jpegInst.planar = (m_jpegInst.inc[0] == 1 && m_jpegInst.inc[1] == 1 && m_jpegInst.inc[2] == 1);

	jpegenc_resync();

	return TRUE;
}

void Encoder_libjpeg::jpegenc_resync(void)
{
	GstVideoInfo *info;
	gint width, height;
	guint i, j;

	info = &m_videoInfo;

	cinfo.image_width = width = GST_VIDEO_INFO_WIDTH(info);
	cinfo.image_height = height = GST_VIDEO_INFO_HEIGHT(info);
	cinfo.input_components = m_jpegInst.channels;

	GST_DEBUG("width %d, height %d", width, height);
	GST_DEBUG("format %d", GST_VIDEO_INFO_FORMAT(info));

	if (GST_VIDEO_INFO_IS_RGB(info)) {
		GST_DEBUG("RGB");
		cinfo.in_color_space = JCS_RGB;
	}
	else if (GST_VIDEO_INFO_IS_GRAY(info)) {
		GST_DEBUG("gray");
		cinfo.in_color_space = JCS_GRAYSCALE;
	}
	else {
		GST_DEBUG("YUV");
		cinfo.in_color_space = JCS_YCbCr;
	}

	/* input buffer size as max output */
	m_jpegInst.bufsize = GST_VIDEO_INFO_SIZE(info);
	jpeg_set_defaults(&cinfo);
	cinfo.raw_data_in = TRUE;
	/* duh, libjpeg maps RGB to YUV ... and don't expect some conversion */
	if (cinfo.in_color_space == JCS_RGB)
		jpeg_set_colorspace(&cinfo, JCS_RGB);

	GST_DEBUG("h_max_samp=%d, v_max_samp=%d", m_jpegInst.h_max_samp, m_jpegInst.v_max_samp);
	/* image dimension info */
	for (i = 0; i < m_jpegInst.channels; i++) {
		GST_DEBUG("comp %i: h_samp=%d, v_samp=%d", i, m_jpegInst.h_samp[i], m_jpegInst.v_samp[i]);

		cinfo.comp_info[i].h_samp_factor = m_jpegInst.h_samp[i];
		cinfo.comp_info[i].v_samp_factor = m_jpegInst.v_samp[i];
		g_free(m_jpegInst.line[i]);
		m_jpegInst.line[i] = g_new(guchar *, m_jpegInst.v_max_samp * DCTSIZE);
		if (!m_jpegInst.planar) {
			for (j = 0; j < m_jpegInst.v_max_samp * DCTSIZE; j++) {
				g_free(m_jpegInst.row[i][j]);
				m_jpegInst.row[i][j] = (guchar *)g_malloc(width);
				m_jpegInst.line[i][j] = m_jpegInst.row[i][j];
			}
		}
	}

	/* guard against a potential error in gst_jpegenc_term_destination
	which occurs iff bufsize % 4 < free_space_remaining */
	m_jpegInst.bufsize = GST_ROUND_UP_4(m_jpegInst.bufsize);

	jpeg_suppress_tables(&cinfo, TRUE);

	GST_DEBUG("resync done");
}

GstFlowReturn Encoder_libjpeg::jpegenc_handle_frame(params* input)
{
	guint height;
	guchar *base[3], *end[3];
	guint stride[3];
	guint i, j, k;
	GstFlowReturn res = GST_FLOW_OK;
	GstVideoFrame current_vframe;

	GST_LOG("got new frame");

	GstBuffer *input_buffer = gst_buffer_new_wrapped_full(
		(GstMemoryFlags)0,
		(gpointer)input->src,
		m_videoInfo.size,
		0,
		m_videoInfo.size,
		NULL,
		NULL);

	if (!gst_video_frame_map(&current_vframe, &m_videoInfo, input_buffer, GST_MAP_READ))
		return GST_FLOW_ERROR;

	height = GST_VIDEO_INFO_HEIGHT(&m_videoInfo);

	for (i = 0; i < m_jpegInst.channels; i++) {
		base[i] = GST_VIDEO_FRAME_COMP_DATA(&current_vframe, i);
		stride[i] = GST_VIDEO_FRAME_COMP_STRIDE(&current_vframe, i);
		end[i] =
			base[i] + GST_VIDEO_FRAME_COMP_HEIGHT(&current_vframe,
			i) * stride[i];
	}

	gst_video_frame_unmap(&current_vframe);

	libjpeg_destination_mgr dest_mgr(input->dst, input->dst_size);

	cinfo.dest = &dest_mgr;
	cinfo.client_data = this;

// 	jdest.next_output_byte = output_map.data;
// 	jdest.free_in_buffer = output_map.size;


	/* prepare for raw input */
#if JPEG_LIB_VERSION >= 70
	cinfo.do_fancy_downsampling = FALSE;
#endif
	cinfo.smoothing_factor = m_smoothing;
	cinfo.dct_method = (J_DCT_METHOD)m_idct_method;
	jpeg_set_quality(&cinfo, m_quality, TRUE);
	jpeg_start_compress(&cinfo, TRUE);

	GST_LOG("compressing");

	if (m_jpegInst.planar) {
		for (i = 0; i < height; i += m_jpegInst.v_max_samp * DCTSIZE) {
			for (k = 0; k < m_jpegInst.channels; k++) {
				for (j = 0; j < m_jpegInst.v_samp[k] * DCTSIZE; j++) {
					m_jpegInst.line[k][j] = base[k];
					if (base[k] + stride[k] < end[k])
						base[k] += stride[k];
				}
			}
			jpeg_write_raw_data(&cinfo, m_jpegInst.line, m_jpegInst.v_max_samp * DCTSIZE);
		}
	}
	else {
		for (i = 0; i < height; i += m_jpegInst.v_max_samp * DCTSIZE) {
			for (k = 0; k < m_jpegInst.channels; k++) {
				for (j = 0; j < m_jpegInst.v_samp[k] * DCTSIZE; j++) {
					guchar *src, *dst;
					gint l;

					/* ouch, copy line */
					src = base[k];
					dst = m_jpegInst.line[k][j];
					for (l = m_jpegInst.cwidth[k]; l > 0; l--) {
						*dst = *src;
						src += m_jpegInst.inc[k];
						dst++;
					}
					if (base[k] + stride[k] < end[k])
						base[k] += stride[k];
				}
			}
			jpeg_write_raw_data(&cinfo, m_jpegInst.line, m_jpegInst.v_max_samp * DCTSIZE);
		}
	}

	/* This will ensure that gst_jpegenc_term_destination is called */
	jpeg_finish_compress(&cinfo);
	GST_LOG("compressing done");

	input->jpeg_size = dest_mgr.jpegsize;

	return res;
}

int Encoder_libjpeg::jpegenc_set_property(guint prop_id, int value)
{
	switch (prop_id)
	{
	case PROP_QUALITY:
		m_quality = value;
		break;

#ifdef ENABLE_SMOOTHING
	case PROP_SMOOTHING:
		m_smoothing = value;
		break;
#endif

	case PROP_IDCT_METHOD:
		m_idct_method = value;
		break;

	default:
		break;
	}

	return 0;
}

int Encoder_libjpeg::jpegenc_get_property(guint prop_id, int *value)
{
	if (value == NULL)
		return -1;

	switch (prop_id)
	{
	case PROP_QUALITY:
		*value = m_quality;
		break;

#ifdef ENABLE_SMOOTHING
	case PROP_SMOOTHING:
		*value = m_smoothing;
		break;
#endif

	case PROP_IDCT_METHOD:
		*value = m_idct_method;
		break;

	default:
		break;
	}

	return 0;
}

gboolean Encoder_libjpeg::jpegenc_start(void)
{
	m_jpegInst.line[0] = NULL;
	m_jpegInst.line[1] = NULL;
	m_jpegInst.line[2] = NULL;
	m_jpegInst.sof_marker = -1;

	return TRUE;
}

gboolean Encoder_libjpeg::jpegenc_stop(void)
{
	gint i, j;

	g_free(m_jpegInst.line[0]);
	g_free(m_jpegInst.line[1]);
	g_free(m_jpegInst.line[2]);
	m_jpegInst.line[0] = NULL;
	m_jpegInst.line[1] = NULL;
	m_jpegInst.line[2] = NULL;
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 4 * DCTSIZE; j++) {
			g_free(m_jpegInst.row[i][j]);
			m_jpegInst.row[i][j] = NULL;
		}
	}

	return TRUE;
}

int Encoder_libjpeg::convertEXIF_libjpeg(CAMERA_EXIF* exif, ExifElementsTable* exifTable, PHOTO_INPUT_PARAM *frame)
{
	int ret = 0;

	if ((0 == ret) && strcmp((char *)&(exif->exifmodel), "") != 0)
	{
		ret = exifTable->insertElement(TAG_MODEL, (char *)&(exif->exifmodel));
	}

	if ((0 == ret) && strcmp((char *)&(exif->exifmake), "") != 0)
	{
		ret = exifTable->insertElement(TAG_MAKE, (char *)&(exif->exifmake));
	}

	if ((0 == ret))
	{
		unsigned int numerator = 0, denominator = 0;
		numerator = exif->focalLengthH;
		denominator = exif->focalLengthL;
		if (numerator || denominator)
		{
			char temp_value[256]; // arbitrarily long string
			snprintf(temp_value,
				sizeof(temp_value) / sizeof(char),
				"%u/%u", numerator, denominator);
			ret = exifTable->insertElement(TAG_FOCALLENGTH, temp_value);

		}
	}

	if ((0 == ret)  && strcmp((char *)&(exif->dateTime), "") != 0)
	{
		ret = exifTable->insertElement(TAG_DATETIME, (char *)&exif->dateTime);
	}

	if (0 == ret)
	{
		char temp_value[5];
		snprintf(temp_value, sizeof(temp_value) / sizeof(char), "%u", frame->width);
		ret = exifTable->insertElement(TAG_IMAGE_WIDTH, temp_value);
	}

	if (0 == ret)
	{
		char temp_value[5];
		snprintf(temp_value, sizeof(temp_value) / sizeof(char), "%u", frame->height);
		ret = exifTable->insertElement(TAG_IMAGE_LENGTH, temp_value);
	}
	if (exif->bGPS)
	{
		if ((0 == ret))
		{
			char temp_value[256]; // arbitrarily long string
			snprintf(temp_value,
				sizeof(temp_value) / sizeof(char) - 1,
				"%d/%d,%d/%d,%d/%d",
				exif->gpsLATH[0], exif->gpsLATL[0],
				exif->gpsLATH[1], exif->gpsLATL[1],
				exif->gpsLATH[2], exif->gpsLATL[2]);
			ret = exifTable->insertElement(TAG_GPS_LAT, temp_value);
			if (exif->gpsLATREF == 1)
			{
				ret = exifTable->insertElement(TAG_GPS_LAT_REF, GPS_SOUTH_REF);
			}
			else
			{
				ret = exifTable->insertElement(TAG_GPS_LAT_REF, GPS_NORTH_REF);
			}
		}

		if ((0 == ret))
		{
			char temp_value[256]; // arbitrarily long string
			snprintf(temp_value,
				sizeof(temp_value) / sizeof(char) - 1,
				"%d/%d,%d/%d,%d/%d",
				exif->gpsLONGH[0], exif->gpsLONGL[0],
				exif->gpsLONGH[1], exif->gpsLONGL[1],
				exif->gpsLONGH[2], exif->gpsLONGL[2]);
			ret = exifTable->insertElement(TAG_GPS_LONG, temp_value);
			if (exif->gpsLONGREF == 1)
			{
				ret = exifTable->insertElement(TAG_GPS_LONG_REF, GPS_WEST_REF);
			}
			else
			{
				ret = exifTable->insertElement(TAG_GPS_LONG_REF, GPS_EAST_REF);
			}
		}

		if ((0 == ret))
		{
			char temp_value[256]; // arbitrarily long string
			snprintf(temp_value,
				sizeof(temp_value) / sizeof(char) - 1,
				"%d/%d",
				exif->gpsALTH, exif->gpsALTL);
			ret = exifTable->insertElement(TAG_GPS_ALT, temp_value);
			if (exif->gpsALTREF == 1)
			{
				ret = exifTable->insertElement(TAG_GPS_ALT_REF, "1");
			}
			else
			{
				ret = exifTable->insertElement(TAG_GPS_ALT_REF, "0");
			}
		}

		if ((0 == ret) && (strcmp((char *)&(exif->gpsProcessMethod), "") != 0))
		{
			ret = exifTable->insertElement(TAG_GPS_PROCESSING_METHOD, (char *)&(exif->gpsProcessMethod));
		}

		if (0 == ret)
		{
			char temp_value[256]; // arbitrarily long string
			snprintf(temp_value,
				sizeof(temp_value) / sizeof(char) - 1,
				"%d/%d,%d/%d,%d/%d",
				exif->gpsTimeH[0], exif->gpsTimeL[0],
				exif->gpsTimeH[1], exif->gpsTimeL[1],
				exif->gpsTimeH[2], exif->gpsTimeL[2]);
			ret = exifTable->insertElement(TAG_GPS_TIMESTAMP, temp_value);
		}

		if (0 == ret)
		{
			ret = exifTable->insertElement(TAG_GPS_DATESTAMP, (char *)&(exif->gpsDatestamp));
		}
	}

#if 0
	if (0 == ret)
	{
		char temp_value[5]; // arbitrarily long string
		snprintf(temp_value, sizeof(temp_value) / sizeof(char) - 1, "%d", exif->imageOri);

		ret = exifTable->insertElement(TAG_ORIENTATION, temp_value);
	}
#endif

	return ret;
}

void Encoder_libjpeg::SetFormat(PHOTO_FORMAT_SETTING *pSet)
{
	gst_video_info_set_format(&m_videoInfo, pSet->format, pSet->width, pSet->height);
	jpegenc_set_format(&m_videoInfo);
}

void Encoder_libjpeg::SetQuality(int quality)
{
	jpegenc_set_property(PROP_QUALITY, quality);
}

PHOTO_OUT_MEMORY* Encoder_libjpeg::Encode(params* main_jpeg, params* tn_jpeg, ExifElementsTable* pExifData)
{
	PHOTO_OUT_MEMORY *picture = NULL;

	if (tn_jpeg->valid)
	{
		if (m_pThumbNail)
		{
			TN_INPUT_PARAM tn_frame;
			tn_frame.format = m_videoInfo.finfo->format;
			tn_frame.width = tn_jpeg->in_width;
			tn_frame.height = tn_jpeg->in_height;
			tn_frame.pInputImage = (guchar *)tn_jpeg->src;
			
			m_pThumbNail->Set(THUMBNAIL_PROP_INFRAME, &tn_frame, sizeof(TN_INPUT_PARAM));
			m_pThumbNail->ThumbNail_Process(0, 0, m_pThumbNail);
			
			if (tn_jpeg->dst_size < m_pThumbNail->OutBufSize())
			{
				tn_jpeg->jpeg_size = 0;
			}
			else
			{
				memcpy(tn_jpeg->dst, m_pThumbNail->OutBuf(), m_pThumbNail->OutBufSize());
				tn_jpeg->jpeg_size = m_pThumbNail->OutBufSize();
			}
		}
		else
		{
			//should not got to here
		}
	}

	if (main_jpeg->valid)
	{
		jpegenc_handle_frame(main_jpeg);
		if (pExifData != NULL)
		{
			Section_t* exif_section = NULL;

			pExifData->insertExifToJpeg((unsigned char*)main_jpeg->dst, main_jpeg->jpeg_size);
			if (tn_jpeg->valid && (tn_jpeg->jpeg_size > 0))
			{
				pExifData->insertExifThumbnailImage((const char*)tn_jpeg->dst, (int)tn_jpeg->jpeg_size);
			}

			exif_section = FindSection(M_EXIF);
			if (exif_section)
			{
				if (mRequestMemory)
					picture = mRequestMemory(main_jpeg->jpeg_size + exif_section->Size, 1);
				if (picture && picture->data)
				{
					pExifData->saveJpeg((unsigned char*)picture->data, main_jpeg->jpeg_size + exif_section->Size);
				}
			}
		}
		else
		{
			if (mRequestMemory)
				picture = mRequestMemory(main_jpeg->jpeg_size, 1);
			if (picture && picture->data)
			{
				memcpy(picture->data, main_jpeg->dst, main_jpeg->jpeg_size);
			}
		}
	}
	else
	{
		if (mRequestMemory)
			picture = mRequestMemory(main_jpeg->jpeg_size, 1);
		if (picture && picture->data)
		{
			memcpy(picture->data, main_jpeg->dst, main_jpeg->jpeg_size);
		}
	}

	return picture;
}
