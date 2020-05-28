#ifndef _LOOP_RECORDERENGINE_H_
#define _LOOP_RECORDERENGINE_H_

#include <glib.h>
#include <gst/gst.h>

#include "DVR_SDK_DEF.h"
#include "RecorderAsyncOp.h"
#include "osa_tsk.h"

#define RECORD_FPS                      25
#define RESERVED_PER_SEC_PER_TRAK_SIZE  30
#define RESERVED_MOOV_UPDATE_INTERVAL	1*NANOSEC_TO_SEC
#define THUMBNAIL_SIZE_MAX_SIZE         70000
#define RESERVED_MOOV_EXTRA_BYTES       THUMBNAIL_SIZE_MAX_SIZE
#define MAX_THREAD_NUM 					5
#define MAX_LOOP_QUEUE_FRAMES			2500

enum LOOP_RECORD_STATE
{
    LOOP_RECORD_STATE_INVALID,
    LOOP_RECORD_STATE_RUNNING,
    LOOP_RECORD_STATE_START_TO_STOP,
    LOOP_RECORD_STATE_STOP_DONE,
};

typedef enum
{
    LOOP_RECORD_PROP_DIR,
    LOOP_RECORD_PROP_FILE_SPLIT_TIME,
    LOOP_RECORD_PROP_FATAL_ERROR,
}LOOP_RECORD_PROP_ID;

typedef struct
{
    gchar *loop_record_location;
    guint loop_record_folder_max_file_index;
    gchar *video_dst_format;
    gchar *thumbnail_dst_format;

    guint m_ActivePeriod;
    guint m_PendPeriod;
    gboolean m_bPendPeriod;
}LoopRecPrivateSetting;

class CLoopRecorderEngine
{
public:
    CLoopRecorderEngine(RecorderAsyncOp *pHandle);
    virtual ~CLoopRecorderEngine(void);

    virtual int	Create(void);
    virtual int	Destroy(void);
	virtual int Open(void);
    virtual int	Close(void);
    virtual int	Set(LOOP_RECORD_PROP_ID ePropId, void *pPropData, int nPropSize);
    virtual int	Get(LOOP_RECORD_PROP_ID ePropId, void *pPropData, int nPropSize);
	virtual int GetRecState(void){return m_eLoopRecState;};
    virtual int AddVideoFrame(GstBuffer *buffer);
    virtual int	Start(gboolean bLockState = TRUE);
    virtual int	Stop(gboolean bLockState = TRUE);
    virtual int	SetFuncList(void* pList);
    virtual GstElement *GetMuxer(){ return m_pQtMuxer; }
	virtual int	SetFatalEnd(void);
	virtual int	SetRecovery(void);

    static gchar *OnUpdateLoopRecDest(GstElement * element, guint fragment_id, gpointer data);
    static gchar *OnUpdateLoopTagList(GstElement * element, guint fragment_id, gpointer data);

protected:
    GstElement		*m_pPipeline;
    GstElement		*m_pVideoAppSrc;
    GstElement		*m_pVideoParse;
    GstElement		*m_pQtMuxer;
    GstElement		*m_pSink;

    gboolean  		m_bQtMuxStopFileDone;
    gboolean		m_bFatalError;
    gboolean		m_bNeedRecover;
	gboolean		m_bSlowWriting;
	gboolean		m_brebuilding;
	gboolean		m_bFatalErrorNotifyHasFired;
    GQueue			m_nFrameQueue;

    LoopRecPrivateSetting 	m_LoopRecSetting;
    LOOP_RECORD_STATE 		m_eLoopRecState;

    RecorderAsyncOp 		*m_pRecorderAsyncOp;

    virtual gint CreatePipeline(void);
   	virtual gint DestroyPipeline(void);
   	virtual gint RebuildPipeline(void);

    static void  OnNeedData(GstElement *appsrc, guint unused_size, gpointer user_data);
    static GstBusSyncReply MessageHandler(GstBus *bus, GstMessage *msg, gpointer pData);

protected:
    gboolean m_bStateLocked;
    GMutex	 m_mutexStateLock;
    gboolean LockState(gboolean bNeedLockState, unsigned long ulLockPos);
    void UnlockState(gboolean bNeedLockState);
    unsigned long m_ulLockPos;

    GMutex			m_nFrameMutex;
    DVR_RECORDER_CALLBACK_ARRAY* m_ptfnCallBackArray;

protected:
	gchar m_CurLoopRecVideoFileName[APP_MAX_FN_SIZE];
    gchar m_CurLoopRecThumbNailFileName[APP_MAX_FN_SIZE];

protected:
    gint SendEOSAndWait(void);
    int SetStateWait(GstState nextState);
};

#endif
