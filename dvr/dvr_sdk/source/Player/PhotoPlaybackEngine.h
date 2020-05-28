#ifndef _PHOTO_PLAYBACK_ENGINE_H_
#define _PHOTO_PLAYBACK_ENGINE_H_

#include <gst/gst.h>
#include <glib.h>
#include <gst/video/gstvideometa.h>
#include <gst/canbuf/canbuf.h>
#include "DVR_PLAYER_DEF.h"
#include "EasyExif.h"

enum PHOTO_PLAYBACK_ENGINE_STATE
{
	PHOTO_PLAYBACK_ENGINE_STATE_INVALID,
	PHOTO_PLAYBACK_ENGINE_STATE_OPEN_START,
	PHOTO_PLAYBACK_ENGINE_STATE_OPEN_DONE,
	PHOTO_PLAYBACK_ENGINE_STATE_RUNNING,
	PHOTO_PLAYBACK_ENGINE_STATE_STOPPED,
};

class CPhotoPlaybackEngine
{
public:
    CPhotoPlaybackEngine(void *playbackEngine);
	~CPhotoPlaybackEngine();

    int Open(const char *fileName, int (*OnNewSampleFromSink)(GstElement *pSink, gpointer user_data));
    int Close();
    int Start();
    int Stop();

    Ofilm_Can_Data_T *GetCanData()
    {
        return &m_photoCanInfo;
    }

private:
    gboolean InitPipeline(const char *fileName, int(*OnNewSampleFromSink)(GstElement *pSink, gpointer user_data));
    int ParseExif(const char *fileName);
    int FileFilter(const char *fileName);

private:
	GMutex	m_lock;

    void *m_playbackEngine;

    GstElement* m_pipeline;
    GstElement* m_source;
    GstElement* m_parse;
    GstElement* m_decoder;
    GstElement* m_convert;
    GstElement* m_capfilter;
    GstElement* m_imagefreeze;
    GstElement* m_videosink;

    gboolean m_default_nonexistent_photo;

    easyexif::EXIFInfo m_ExifInfo;
    Ofilm_Can_Data_T m_photoCanInfo;

	PHOTO_PLAYBACK_ENGINE_STATE m_eEngineState;
};

#endif
