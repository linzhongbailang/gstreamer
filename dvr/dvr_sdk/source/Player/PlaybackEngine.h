
#ifndef _PLAYBACKENGINE_H_
#define _PLAYBACKENGINE_H_

#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <glib.h>
#include "DVR_PLAYER_DEF.h"
#include "PhotoPlaybackEngine.h"
#include "PhotoAsyncOp.h"
#include "PhotoEngine.h"

enum STREAM_TYPE
{
	STREAM_TYPE_AUDIO,
	STREAM_TYPE_VIDEO,
	STREAM_TYPE_TEXT,
};

enum TAG_TYPE
{
	TAG_TYPE_CONTAINER,
	TAG_TYPE_VIDEO_CODEC,
	TAG_TYPE_AUDIO_CODEC,
	TAG_TYPE_BITRATE,
	TAG_TYPE_LANGUAGE_CODE,
	TAG_TYPE_TITLE,
	TAG_TYPE_ARTIST,
	TAG_TYPE_ALBUM,
	TAG_TYPE_GENRE,
	TAG_TYPE_HOMEPAGE,
};

enum PLAYBACK_ENGINE_STATE
{
	PLAYBACK_ENGINE_STATE_INVALID,
	PLAYBACK_ENGINE_STATE_STOPPED,
	PLAYBACK_ENGINE_STATE_PAUSED,
	PLAYBACK_ENGINE_STATE_RUNNING,
	PLAYBACK_ENGINE_STATE_FASTPLAY,
	PLAYBACK_ENGINE_STATE_FASTSCAN,
	PLAYBACK_ENGINE_STATE_STEPFRAME,
};

enum CUSTOM_GST_ELEMENT
{
	CUSTOM_GST_ELEMENT_VPE,
	CUSTOM_GST_ELEMENT_WAYLAND,
};

typedef enum _tagVIDEO_SINK_MODE
{
	VIDEO_SINK_INVALID,
	VIDEO_SINK_DUCATI
}VIDEO_SINK_MODE;

typedef struct 
{
  uint      writepos;
  uint      elementSize;
  uchar*    cachebuffer;
  void*     (*PbInitRingbufferCb)(uint buffsize);
  void      (*PbFreeRingbufferCb)(void* pbbuffer, uint buffsize);
  int       (*PbmemCacheInvCb)(unsigned int virtAddr, unsigned int length);
  int       (*PbmemCacheWbCb)(unsigned int virtAddrStart, unsigned int virtAddrEnd);
  int       (*PbVideoProcessCb)(void* pbbuffer, unsigned int framelength, unsigned int CanVirAddr);
  int       (*PbPhotoProcessCb)(void* pbbuffer, uint framelength,  uint width, uint height, uint CanVirAddr);
}Playback_Param;

class PLAYBACK_FRAME
{
public:
	PLAYBACK_FRAME()
	{
		m_pImageData = m_pCanData = NULL;
		m_nImageSize = m_nWidth = m_nHeight = m_nCanSize = 0;
	}
	virtual ~PLAYBACK_FRAME()
	{
	}

	void *GetImageData(){ return m_pImageData; }
	gint GetImageSize(){ return m_nImageSize; }
	void *GetCanData(){return m_pCanData; }
	gint GetCanSize(){return m_nCanSize;}
	gint GetImageWidth(){ return m_nWidth; }
	gint GetImageHeight(){ return m_nHeight; }

	void SetFrame(gpointer imagedata, gint width, gint height, gint imagesize, gpointer candata, gint cansize)
	{
		m_pImageData = imagedata;
		m_pCanData = candata;
		m_nWidth = width;
		m_nHeight = height;
		m_nImageSize = imagesize;
		m_nCanSize = cansize;
	}
	
private:
	gpointer m_pImageData;
	gpointer m_pCanData;
	gint m_nImageSize;
	gint m_nCanSize;
	gint m_nWidth;
	gint m_nHeight;
};

class CPlaybackEngine
{
public:
	CPlaybackEngine();
	virtual ~CPlaybackEngine();

public:
	int			Open(gpointer pParam, glong cbParam, gboolean bLockState = TRUE);
	int			Close(gboolean bLockState = TRUE);
	int			SetFuncList(void* pList);
	int			Run(gboolean bLockState = TRUE);
	int			Stop(gboolean bLockState = TRUE);
	int			Pause(gboolean bLockState = TRUE);
	int			SetPosition(glong dwPos, gpointer pvRangePos, gboolean bLockState = TRUE);
	int			GetPosition(glong* pdwPos, gboolean bLockState = FALSE);
	int			Set(glong dwPropID, gpointer pInstanceData, glong cbInstanceData, gpointer pPropData, glong cbPropData);
	int			Get(glong dwPropID, gpointer pInstanceData, glong cbInstanceData, gpointer pPropData, glong cbPropData, glong *pcbReturned);
	int			FastPlay(gboolean bLockState = TRUE);
	int			FastScan(glong lDirection, gboolean bLockState = TRUE);
	int 		FrameUpdate(DVR_IO_FRAME *pInputFrame);

    int         PhotoOpen(const char *fileName);
    int         PhotoClose();
    int         PhotoPlay();
    int         PhotoStop();
	int			PrintScreen(DVR_PHOTO_PARAM *pParam);

public:
	static GstBusSyncReply		MessageHandler(GstBus *bus, GstMessage *msg, gpointer pData); 
	static void  OnQtDemuxPadAdded(GstElement *, GstPad *padSrc, gpointer user_data);

private:
	PLAYER_STATUS			GetStatus();
	int					SetStateWait(GstState nextState);
	void					OnStreamEOF();
	int					StartFastPlay();
	int					StopFastPlay();
	double					CalculateSpeed(int nSpeed);
	int					InitRingbuffer(int len);
	int					FreeRingbuffer(int len);
	int					InternalOpen(gpointer pParam, glong cbParam, gboolean bLockState);

	GstElement*				CreateVideoSinkBin();
	int					ChangeVideoSink(GstElement* pPipeline);

	DVR_PLAYER_CALLBACK_ARRAY* m_ptfnCallBackArray;
    
    int                     OnFrameProcess(DVR_IO_FRAME *pFrame, Playback_Param* pParam, DVR_FILE_TYPE type);
	static int				OnNewSampleFromSinkForVideo(GstElement *pSink, gpointer user_data);
    static int				OnNewSampleFromSinkForPhoto(GstElement *pSink, gpointer user_data);

	static int 			AsyncWaitImgForPhoto(DVR_U32 param1, DVR_U32 param2, void *pContext);
    static int 			AsyncAddPhotoToDB(const char *pLocation, const char *pThumbNailLocation, void *pContext);
	static int 			PhotoReturn(DVR_U32 param1, DVR_U32 param2, void *pContext);	
private:
	GstElement*			m_pPipeline;
	int				m_nSpeed;
	int				m_nDirection;
	PLAYBACK_ENGINE_STATE		m_eEngineState;
	
	DVR_IO_FRAME	m_nPlayBackFrame;
	
	CPhotoAsyncOp	*m_pPhotoAsyncOp;
	CPhotoEngine	*m_pPhotoEngine;
	CThumbNail		*m_pThumbNail;
	
private:
	gboolean m_bStateLocked;
	GMutex	m_mutexStateLock;
	gboolean LockState(gboolean bNeedLockState, unsigned long ulLockPos);
	void UnlockState(gboolean bNeedLockState);
	unsigned long m_ulLockPos;
	gboolean		m_bClosing;

private:
	GstElement*			CreateBin(void* pParam);
	GstElement*			CreatePlayBin(char* pParam);
	VIDEO_SINK_MODE 	m_eSinkMode;
    static void        PlaybackProcessTaskEntry(void *ptr);
	GstElement* 		CreatePipeline(void* pParam);
	gint                CreateProcessTask(void);
	GstElement *m_pSource;
	GstElement *m_pQtDemux;
	GstElement *m_pQueue;
	GstElement *m_pParse;
	GstElement *m_pVideoDec;
	GstElement *m_pSink; 

private:
	char *m_pThreadURI;
	OSA_TskHndl m_hThread;
	/*fast scan thread*/
	static gpointer eof_thread_func(gpointer data);
	int l_eof_thread_func();
	GThread *m_pEOFThread;
	GMutex			m_nFrameMutex;		
	GMutex			m_nOrigDataMutex;		
	GQueue			m_nFrameQueue;
	static gpointer fastscan_thread_func(gpointer data);
	int l_fastscan_thread_func();
	GThread *m_pFastScanThread;
	gboolean m_bStopFastScan;
	glong m_nFastPlayPos;
	
	glong m_nOperationTimeOut;
	GstClockTime m_nOpenTimeOut;

    CPhotoPlaybackEngine *m_pPhotoPlayer;

	gboolean m_bFirstFrame;
	gboolean m_bGetvdFrameInfo;
	gboolean m_bGetpicFrameInfo;
	int m_nVideoPlaybackSkipFrames;
	gboolean m_bSeekIDRFrame;
};

#ifdef __cplusplus
extern "C"
{
#endif
int Dvr_Sdk_Playback_SetCallback(Playback_Param* param);
#ifdef __cplusplus
}
#endif

#endif  // ~_PLAYBACKENGINE_H_

