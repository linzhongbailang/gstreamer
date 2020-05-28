#ifndef _VIDEO_BUFFER_H_
#define _VIDEO_BUFFER_H_
#include <glib.h>
#include <gst/gst.h>
#include "osa_tsk.h"
#include "CycleBuffer.h"

class CCycleBuffer;
class CVideoBuffer
{
public:
    CVideoBuffer(void);
    virtual ~CVideoBuffer(void);
	virtual void Create(void);
	virtual void Destroy(void);
    virtual int	Start(void);
    virtual int	Stop(void);
	virtual int	AddVideoFrame(GstBuffer *buffer);
    virtual int	SetLimitTime(int LimitTime){m_PreRecordLimitTime = LimitTime;};
    virtual int	GetFrame(FrameNode *node, bool isFirstFrame);
    virtual int Reset(void);

protected:
    static void  MonitorTaskEntry(void *ptr);

    CCycleBuffer 	*m_pCycleBuffer;
    OSA_TskHndl 	m_hThread_Monitor;
    gboolean 		m_bEnableMonitorDetect;
    gboolean 		m_bMonitorExit;
    guint64   		m_LastTimeStamp;
	DVR_U32 		m_PreRecordLimitTime;
    int     		m_StartframePos;
    guint64         m_StartTimeStamp;
};

#endif
//_VIDEO_BUFFER_H_
