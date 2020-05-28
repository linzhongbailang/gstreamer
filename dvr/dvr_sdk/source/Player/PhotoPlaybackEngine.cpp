#include <stdlib.h>
#include <string.h>

#include "osa_fs.h"
#include "PhotoPlaybackEngine.h"

#define DEFAULT_NONEXISTENT_PHOTO "/opt/svres/HMI/DVR/default_nonexistent_photo.jpg"

GST_DEBUG_CATEGORY_STATIC(photo_pb_engine_category);  // define category (statically)
#define GST_CAT_DEFAULT photo_pb_engine_category       // set as default

CPhotoPlaybackEngine::CPhotoPlaybackEngine(void *playbackEngine)
{
    GST_DEBUG_CATEGORY_INIT(photo_pb_engine_category, "PHOTO_PLAYBACK_ENGINE", GST_DEBUG_FG_BLUE, "photo playback engine category");

	if (!gst_is_initialized())
	{
		GST_LOG("Gstreamer not initialized ... initializing");
		gst_init(NULL, NULL);
	}

	g_mutex_init(&m_lock);
	m_pipeline = NULL;

    m_source = NULL;
    m_parse = NULL;
    m_videosink = NULL;
    m_playbackEngine = playbackEngine;

    m_default_nonexistent_photo = 
        g_file_test(DEFAULT_NONEXISTENT_PHOTO, G_FILE_TEST_EXISTS);

    memset(&m_photoCanInfo, 0, sizeof(m_photoCanInfo));
	m_eEngineState = PHOTO_PLAYBACK_ENGINE_STATE_INVALID;
}

CPhotoPlaybackEngine::~CPhotoPlaybackEngine()
{
	g_mutex_clear(&m_lock);
    m_playbackEngine = NULL;
}

int CPhotoPlaybackEngine::ParseExif(const char *fileName)
{
    FILE *fp = fopen(fileName, "rb");
    if (!fp) {
        GST_ERROR("Can't open file.");
        return DVR_RES_EFAIL;
    }
    fseek(fp, 0, SEEK_END);
    unsigned long fsize = ftell(fp);
    rewind(fp);
    unsigned char *buf = new unsigned char[fsize];
    if (buf == NULL){
        GST_ERROR("out of memory\n");
        fclose(fp);
        return DVR_RES_EFAIL;
    }

    if (fread(buf, 1, fsize, fp) != fsize) {
        GST_ERROR("Can't read file.\n");
        delete[] buf;
        fclose(fp);
        return DVR_RES_EFAIL;
    }
    fclose(fp);

    // Parse EXIF
    int code = m_ExifInfo.parseFrom(buf, fsize);
    delete[] buf;

    return DVR_RES_SOK;
}

int CPhotoPlaybackEngine::Open(const char *fileName, int(*OnNewSampleFromSink)(GstElement *pSink, gpointer user_data))
{
	if(m_eEngineState != PHOTO_PLAYBACK_ENGINE_STATE_INVALID)
	{
		GST_ERROR("Invalid Photo Playback State[%d] while open", m_eEngineState);
		return DVR_RES_EFAIL;
	}

	DVR_U64 ullPhotoFileSize = 0;
	DVR_RESULT res = DVR::OSA_GetFileStatInfo(fileName, &ullPhotoFileSize, NULL);
	if((res == DVR_RES_SOK && ullPhotoFileSize == 0) || (res != DVR_RES_SOK))
	{
		return DVR_RES_EFAIL;
	}
	
	m_eEngineState = PHOTO_PLAYBACK_ENGINE_STATE_OPEN_START;

    if (DVR_RES_SOK == ParseExif(fileName))
    {
        gchar **tokens, **walk;
        tokens = g_strsplit(m_ExifInfo.DateTime.c_str(), "-", 0);
        if (g_strv_length(tokens) != 6)
        {
            g_strfreev(tokens);
            goto photo_open;
        }

        walk = tokens;
        m_photoCanInfo.vehicle_data.year = strtol(*walk++, NULL, 10) - 2013;
        m_photoCanInfo.vehicle_data.month = strtol(*walk++, NULL, 10) - 1;
        m_photoCanInfo.vehicle_data.day = strtol(*walk++, NULL, 10) - 1;
        m_photoCanInfo.vehicle_data.hour = strtol(*walk++, NULL, 10);
        m_photoCanInfo.vehicle_data.minute = strtol(*walk++, NULL, 10);
        m_photoCanInfo.vehicle_data.second = strtol(*walk++, NULL, 10);
        g_strfreev(tokens);

        m_photoCanInfo.vehicle_data.vehicle_speed = m_ExifInfo.CustomRendered;
        m_photoCanInfo.vehicle_data.vehicle_movement_state = m_ExifInfo.ExposureMode;
        m_photoCanInfo.vehicle_data.Brake_Pedal_Position = m_ExifInfo.WhiteBalance;
        m_photoCanInfo.vehicle_data.DriverBuckleSwitchStatus = m_ExifInfo.Contrast;
        m_photoCanInfo.vehicle_data.Accelerator_Actual_Position = m_ExifInfo.Saturation;
        m_photoCanInfo.vehicle_data.turn_signal = m_ExifInfo.Sharpness;
        m_photoCanInfo.vehicle_data.positioning_system_longitude = strtol(m_ExifInfo.Make.c_str(), NULL, 10);
        m_photoCanInfo.vehicle_data.positioning_system_latitude = strtol(m_ExifInfo.Model.c_str(), NULL, 10);
    }
	else
	{
		GST_ERROR("ParseExif for file[%s] fail", fileName);
	}

photo_open:
	g_mutex_lock(&m_lock);

    InitPipeline(fileName, OnNewSampleFromSink);

	g_mutex_unlock(&m_lock);
	
	m_eEngineState = PHOTO_PLAYBACK_ENGINE_STATE_OPEN_DONE;

	return DVR_RES_SOK;
}

int CPhotoPlaybackEngine::Close()
{
	g_mutex_lock(&m_lock);

    if (!gst_is_initialized() || m_pipeline == NULL)
    {
        g_mutex_unlock(&m_lock);
        return DVR_RES_EFAIL;
    }

    GstStateChangeReturn status = gst_element_set_state(m_pipeline, GST_STATE_NULL);
    if (status == GST_STATE_CHANGE_ASYNC)
    {
        // wait for status update
        GstState state;
        GstState pending;
        status = gst_element_get_state(m_pipeline, &state, &pending, GST_CLOCK_TIME_NONE);
    }
    if (status == GST_STATE_CHANGE_FAILURE)
    {
        gst_object_unref(GST_OBJECT(m_pipeline));
        m_pipeline = NULL;
        GST_ERROR("Unable to stop gstreamer pipeline");
        return DVR_RES_EFAIL;
    }

    gst_object_unref(GST_OBJECT(m_pipeline));
    m_pipeline = NULL;

	m_eEngineState = PHOTO_PLAYBACK_ENGINE_STATE_INVALID;
	
	g_mutex_unlock(&m_lock);

	return DVR_RES_SOK;
}

int CPhotoPlaybackEngine::Start()
{
    g_mutex_lock(&m_lock);

    if (!gst_is_initialized() || m_pipeline == NULL)
    {
        g_mutex_unlock(&m_lock);
        return DVR_RES_EFAIL;
    }

    gst_element_set_state(GST_ELEMENT(m_pipeline), GST_STATE_PLAYING);

	m_eEngineState = PHOTO_PLAYBACK_ENGINE_STATE_RUNNING;

    g_mutex_unlock(&m_lock);

    return DVR_RES_SOK;
}

int CPhotoPlaybackEngine::Stop()
{
    GstStateChangeReturn ret;

    g_mutex_lock(&m_lock);

    if (!gst_is_initialized() || m_pipeline == NULL)
    {
        g_mutex_unlock(&m_lock);
        return DVR_RES_SOK;
    }

    ret = gst_element_set_state(GST_ELEMENT(m_pipeline), GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_ASYNC)
    {
        GstState state;
        GstState pending;
        ret = gst_element_get_state(GST_ELEMENT(m_pipeline), &state, &pending, GST_CLOCK_TIME_NONE);
        if (ret == GST_STATE_CHANGE_FAILURE || ret == GST_STATE_CHANGE_ASYNC)
		{
			g_mutex_unlock(&m_lock);
            return DVR_RES_EFAIL;
		}
    }
    else if (ret == GST_STATE_CHANGE_FAILURE)
    {
        g_mutex_unlock(&m_lock);
        return DVR_RES_EFAIL;
    }

    g_mutex_unlock(&m_lock);

    return DVR_RES_SOK;
}

gboolean CPhotoPlaybackEngine::InitPipeline(const char *fileName, int(*OnNewSampleFromSink)(GstElement *pSink, gpointer user_data))
{
    GstStateChangeReturn stateret;
    m_pipeline  = gst_pipeline_new("photo_playback");
    m_source    = gst_element_factory_make("filesrc", NULL);
    m_parse     = gst_element_factory_make("jpegparse", NULL);
    m_videosink = gst_element_factory_make("appsink", NULL);
    if (m_pipeline == NULL || m_source == NULL || m_parse == NULL || m_videosink == NULL)
        return false;

    g_object_set(m_videosink, "emit-signals", TRUE, "sync", TRUE, NULL);
    g_signal_connect(m_videosink, "new-sample", G_CALLBACK(OnNewSampleFromSink), m_playbackEngine);

    g_object_set(G_OBJECT(m_source), "location", fileName, NULL);

    gst_bin_add_many(GST_BIN(m_pipeline), m_source, m_parse, m_videosink, NULL);
    if (!gst_element_link_many(m_source, m_parse, m_videosink)) {
        GST_ERROR("cannot link elements");
    }
    
    stateret = gst_element_set_state(GST_ELEMENT(m_pipeline), GST_STATE_PAUSED);
    if (stateret == GST_STATE_CHANGE_FAILURE) {
        GST_ERROR("cannot put pipeline to play\n");
    }

    return true;
}

