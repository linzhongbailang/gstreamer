#ifndef _EVENT_RECORDERENGINE_H_
#define _EVENT_RECORDERENGINE_H_

#include <glib.h>
#include <gst/gst.h>

#include "DVR_SDK_DEF.h"
#include "RecorderAsyncOp.h"
#include "osa_tsk.h"

#define EVENT_RECORD_MSGQUEUE_SIZE (16)

enum EVENT_RECORD_STATE
{
    EVENT_RECORD_STATE_IDLE,
    EVENT_RECORD_STATE_REDAY_TO_STOP,
	EVENT_RECORD_STATE_START_TO_STOP,
    EVENT_RECORD_STATE_RUNNING,
};

typedef enum
{
    EVENT_RECORD_PROP_DIR,
    EVENT_RECORD_PROP_PERIOD_SETTING,
	EVENT_RECORD_PROP_LIMITM_DELAY,
    EVENT_RECORD_PROP_FATAL_ERROR,
}EVENT_RECORD_PROP_ID;

typedef struct
{
    gchar *event_record_location;
    guint event_record_folder_max_file_index;
    gchar *video_dst_format;
    gchar *thumbnail_dst_format;
}EventRecPrivateSetting;

#define VIDEOCB_FLAGS_EVENT_OCCUR           (0x00000001)
#define VIDEOCB_FLAGS_EVENT_WRITE_COMPLETE  (0x00000002)

#define VIDEOCB_ADDFLAGS(x, y)          	((x) |= (y))
#define VIDEOCB_REMOVEFLAGS(x, y)	    	((x) &= (~(y)))
#define VIDEOCB_CHECKFLAGS(x, y)			((x) & (y))

#define CHAINBUFFER_UNDERFLOW_THRESHOLD     500 * GST_MSECOND
#define CHAINBUFFER_OVERFLOW_THRESHOLD      15 * GST_SECOND
#define CHAINBUFFER_MAX_DURATION            18 * GST_SECOND

#define RECORD_FPS                      	25
#define RESERVED_PER_SEC_PER_TRAK_SIZE  	30
#define RESERVED_MOOV_UPDATE_INTERVAL		1*NANOSEC_TO_SEC
#define THUMBNAIL_SIZE_MAX_SIZE         	70000
#define RESERVED_MOOV_EXTRA_BYTES       	THUMBNAIL_SIZE_MAX_SIZE

typedef int(*PFN_NOTIFY_WHEN_EMERGENCY_COMPLETE)(void *pContext);

class CVideoBuffer;
class CEventRecorderEngine
{
public:
    CEventRecorderEngine(RecorderAsyncOp *pHandle, CVideoBuffer *pVideoCB);
    virtual ~CEventRecorderEngine();

	virtual int Open(void);
	virtual int Close(void);
    virtual int	Create(void);
    virtual int	Destroy(void);
    virtual int	Set(EVENT_RECORD_PROP_ID ePropId, void *pPropData, int nPropSize);
    virtual int	Get(EVENT_RECORD_PROP_ID ePropId, void *pPropData, int nPropSize);

    virtual int	Start(void);
    virtual int	Stop(void);
    virtual int Reset(void);
    virtual int GetRecState(void){return m_eEventRecState;};
    virtual int	SetFuncList(void* pList);
	virtual int SetFatalEnd(void);
    virtual int AddVideoFrame(GstBuffer *buffer);
    virtual GstElement *GetMuxer(){ return m_pQtMuxer; }

	static gchar *OnUpdateEventTagList(GstElement * element, guint fragment_id, gpointer data);
    static gchar *OnUpdateEventRecDest(GstElement * element, guint fragment_id, gpointer data);

protected:
    GstElement		*m_pPipeline;
    GstElement		*m_pVideoAppSrc;
    GstElement		*m_pVideoParse;
    GstElement		*m_pQtMuxer;
    GstElement		*m_pSink;

    CVideoBuffer 	*m_pVideoBuffer;
    RecorderAsyncOp *m_pRecorderAsyncOp;

    guint    m_frameCnt;
    gboolean m_bEventRecWriteComplete;
    gboolean m_bEOSHasTriggered;
    guint    m_GFlags;
    GMutex	 m_nMutex;

    OSA_TskHndl m_hThread_Monitor;
    guint64   	m_postRecordTimeLimit;
    guint64   	m_postRecordStartTimeStamp;
    gboolean  	m_bPostRecordStartTimeStampInitDone;
    guint64   	m_fistNodeTimeStamp;
    guint64   	m_bUsedRecordTime;
    gboolean  	m_bQtMuxStopFileDone;
	gboolean  	m_bFatalError;
	gboolean	m_brebuilding;
	gboolean	m_bForceFlush;
	gboolean	m_bFatalErrorNotifyHasFired;

    DVR_EVENT_RECORD_SETTING 		m_Setting;
    EVENT_RECORD_STATE 				m_eEventRecState;

    EventRecPrivateSetting 			m_EventRecSetting;
    DVR_RECORDER_CALLBACK_ARRAY* 	m_ptfnCallBackArray;


    virtual gint CreatePipeline(void);
    virtual gint DestroyPipeline(void);
	virtual gint RebuildPipeline(void);

    virtual gint SendEOSAndWait(void);

 	static void* Async(void* pParam);
	static int   ForceFlush(void);
	static void  OnNeedData(GstElement *appsrc, guint unused_size, gpointer user_data);
    static GstBusSyncReply MessageHandler(GstBus *bus, GstMessage *msg, gpointer pData);

protected:
	gchar m_CurEventRecVideoFileName[APP_MAX_FN_SIZE];
    gchar m_CurEventRecThumbNailFileName[APP_MAX_FN_SIZE];

};

#endif
