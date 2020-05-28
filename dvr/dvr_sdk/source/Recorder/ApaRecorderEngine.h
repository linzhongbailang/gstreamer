#ifndef _DAS_RECORDERENGINE_H_
#define _DAS_RECORDERENGINE_H_

#include "osa.h"
#include "LoopRecorderEngine.h"

class CApaRecorderEngine : public CLoopRecorderEngine
{
public:
    CApaRecorderEngine(RecorderAsyncOp *pHandle);
    virtual ~CApaRecorderEngine(void);

	virtual int Open(void);
    virtual int	Start(gboolean bLockState = TRUE);
    virtual int	Stop(gboolean bLockState = TRUE);
	virtual int	SetFatalEnd(void);

	static gchar *OnUpdateApaTagList(GstElement * element, guint fragment_id, gpointer data);
protected:
    virtual gint 	CreatePipeline(void);
    static gchar 	*OnUpdateApaRecDest(GstElement * element, guint fragment_id, gpointer data);
    static void  	OnNeedApaData(GstElement *appsrc, guint unused_size, gpointer user_data);
    static GstBusSyncReply MessageHandler(GstBus *bus, GstMessage *msg, gpointer pData);
};
#endif
//_DAS_RECORDERENGINE_H_
