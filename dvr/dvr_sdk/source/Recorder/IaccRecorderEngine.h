#ifndef _IACC_RECORDERENGINE_H_
#define _IACC_RECORDERENGINE_H_

#include "EventRecorderEngine.h"
class CIaccRecorderEngine : public CEventRecorderEngine
{
public:
    CIaccRecorderEngine(RecorderAsyncOp *pHandle, CVideoBuffer *pVideoCB);
    virtual ~CIaccRecorderEngine(void);

	virtual int  Open(void);
    virtual int	 Start(void);
    virtual int	 Stop(void);
    virtual int	 Set(EVENT_RECORD_PROP_ID ePropId, void *pPropData, int nPropSize);
    virtual gint CreatePipeline(void);
    static gchar *OnUpdateIaccRecDest(GstElement * element, guint fragment_id, gpointer data);

	static gchar *OnUpdateIaccTagList(GstElement * element, guint fragment_id, gpointer data);
protected:
	static void  OnIaccNeedData(GstElement *appsrc, guint unused_size, gpointer user_data);
    static GstBusSyncReply MessageHandler(GstBus *bus, GstMessage *msg, gpointer pData);
};
#endif
//_IACC_RECORDERENGINE_H_
