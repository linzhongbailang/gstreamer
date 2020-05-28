#ifndef __j6_arm_app_DvrTask_H__
#define __j6_arm_app_DvrTask_H__

#include <task/BaseTask.h>
#include <event/AvmEventTypes.h>
#include <event/ofilm_msg_type.h>

class DvrTask : public BaseTask
{
public:

    DECLARE_TASKCLASS_CLASSCREATE_INFO(DvrTask);
public:
    DvrTask(const char* name = "DvrTask", ConfigItem* cfg = NULL);
    ~DvrTask();

private:
    DECLARE_TASK_EVENT_TABLE(DvrTask);
    //以下方法由BaseTask得来，参照BaseTask
    virtual bool ExtraInit();
	virtual void ExtraDeInit();
	virtual bool DeferredInit();

    int OnNewImgData(AvmEvent& event);
    int OnNewPlaybackData(AvmEvent& event);
	int OnCommand(AvmEvent_CtrlCmd *pDvrCmd);

private:

};

#endif // __j6_arm_app_DvrTask_H__
