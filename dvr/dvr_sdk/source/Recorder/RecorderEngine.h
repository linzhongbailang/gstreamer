
#ifndef _RECORDERENGINE_H_
#define _RECORDERENGINE_H_

#include <gst/gst.h>
#include <glib.h>
#if (!defined WIN32) && (!defined LINUX86)
#include <gst/drm/gstdrmbufferpool.h>
#endif
#include "DvrMutexLocker.h"
#include "DVR_RECORDER_DEF.h"
#include "RecorderAsyncOp.h"
#include "PhotoAsyncOp.h"
#include "PhotoEngine.h"

enum RECORDER_MODE
{
	RECORDER_MODE_NORMAL = 0,
	RECORDER_MODE_EMERGENCY,
	RECORDER_MODE_APA,
	RECORDER_MODE_IACC,
	RECORDER_MODE_COUNT,
};

typedef struct _tagRecordConfig
{
	guint intra_interval;

	RECORDER_VIDEO_QUALITY_SETTING m_ActiveVQSetting;
	RECORDER_VIDEO_QUALITY_SETTING m_PendVQSetting;
	gboolean m_bVQPend;
}RecordConfig;

enum RECORDER_ENGINE_STATE
{
	RECORDER_ENGINE_STATE_INVALID,
	RECORDER_ENGINE_STATE_RUNNING,
	RECORDER_ENGINE_STATE_START_TO_STOP,
	RECORDER_ENGINE_STATE_STOP_DONE,
};

class CLoopRecorderEngine;
class CVideoBuffer;
class CEventRecorderEngine;
class CPhotoEngine;
class CThumbNail;
class COSDTitleEx;

class CRecorderEngine
{
public:
	CRecorderEngine();
	virtual ~CRecorderEngine();

public:
	int			Create(gboolean bLockState = TRUE);
	int			Destroy(gboolean bLockState = TRUE);
	int			Start(gboolean bLockState = TRUE);
	int			Stop(gboolean bLockState = TRUE);
	int			Reset(gboolean bLockState = TRUE);
	int			Set(glong dwPropID, gpointer pInstanceData, gint cbInstanceData, gpointer pPropData, gint cbPropData);
	int			Get(glong dwPropID, gpointer pInstanceData, gint cbInstanceData, gpointer pPropData, gint cbPropData, gint *pcbReturned);
	int			AddFrame(DVR_IO_FRAME *pFrame);
    int         RecvBitStreamer(DVR_IO_FRAME *pInputFrame);
	int			AcquireInputBuf(void **ppvBuff);
	int			SetFuncList(void* pList);
	void		SetFatalEnd(void);
	void		SetLoopRecovery(void);
	int			GetPosition(glong* pdwPos, gboolean bLockState = FALSE);
    int			Photo(DVR_PHOTO_PARAM *pParam);
    int         Overlay(void *canbuffer, void *osdbuffer, int cansize, int osdsize);

    int         LoopRecStart();
    int         LoopRecStop();

    int			EventRecStart();
    int			EventRecStop();

	static int AsyncStartLoopRec(DVR_U32 param1, DVR_U32 param2, void *pContext);
	static int AsyncStopLoopRec(DVR_U32 param1, DVR_U32 param2, void *pContext);
	
    static int AsyncStartEventRec(DVR_U32 param1, DVR_U32 param2, void *pContext);
	static int AsyncStopEventRec(DVR_U32 param1, DVR_U32 param2, void *pContext);

    static int AsyncStartDasRec(DVR_U32 param1, DVR_U32 param2, void *pContext);
    static int AsyncStopDasRec(DVR_U32 param1, DVR_U32 param2, void *pContext);

    static int AsyncStartIaccRec(DVR_U32 param1, DVR_U32 param2, void *pContext);
    static int AsyncStopIaccRec(DVR_U32 param1, DVR_U32 param2, void *pContext);

    int         IaccRecStart(int type = 0);
    int         IaccRecStop(int type = 0);

    int         DasRecStart();
    int         DasRecStop();
	int         AsyncOpFlush(void);

	static int AsyncWaitImgForPhoto(DVR_U32 param1, DVR_U32 param2, void *pContext);
    static int AsyncAddPhotoToDB(const char *pLocation, const char *pThumbNailLocation, void *pContext);
	static int PhotoReturn(DVR_U32 param1, DVR_U32 param2, void *pContext);

	static int AsyncWaitImgForVideo(DVR_U32 param1, DVR_U32 param2, void *pContext);
    static int VideoAddThumbNail(DVR_U32 param1, DVR_U32 param2, void *pContext);
	static void* Async(void* pParam);
private:
	RECORDER_ENGINE_STATE		m_eEngineState;
	RecordConfig	m_nConfig;
	gdouble  		m_last_time;
	GQueue			m_nFrameQueue;
	DVR_IO_FRAME	m_nInFrameForPhoto;

private:
	void SetOsdTitle(GstBuffer* buffer, DVR_IO_FRAME* pFrame);

	gint CreateMonitorTask(void);
	gint DestroyRecorder(void);

    static void  RecMonitorTaskEntry(void *ptr);

private:
	GMutex			m_nFrameMutex;		
    DvrMutex        m_nConfigMutex;
    gint64          m_basetime;
gint64          m_inputCount;	
DVR_RECORDER_CALLBACK_ARRAY* m_ptfnCallBackArray;
	RecorderAsyncOp *m_pRecorderAsyncOp;
	CPhotoAsyncOp	*m_pPhotoAsyncOp;
	CPhotoEngine	*m_pPhotoEngine;
    CVideoBuffer 	*m_pVideoBuffer;
    CEventRecorderEngine *m_pEventRecordEngine;
    CEventRecorderEngine *m_pIaccRecordEngine;
    CLoopRecorderEngine  *m_pApaRecordEngine;
    CLoopRecorderEngine  *m_pLoopRecordEngine;
	CThumbNail		*m_pThumbNail;
	COSDTitleEx		*m_pOsdTitie;
	
	OSA_TskHndl m_hThread;
};

#endif  // ~_RECORDERENGINE_H_

