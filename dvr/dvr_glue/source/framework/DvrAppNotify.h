/*******************************************************************************
* Copyright 2003 O-Film Technologies, Inc., All Rights Reserved.
* O-Film Confidential
*
* DESCRIPTION:
*
* ABBREVIATIONS:
*   TODO: List of abbreviations used, or reference(s) to external document(s)
*
* TRACEABILITY INFO:
*   Design Document(s):
*     TODO: Update list of design document(s)
*
*   Requirements Document(s):
*     TODO: Update list of requirements document(s)
*
*   Applicable Standards (in order of precedence: highest first):
*
* DEVIATIONS FROM STANDARDS:
*   TODO: List of deviations from standards in this file, or
*   None.
*
\*********************************************************************************/
#ifndef _DVR_APP_NOTIFY_H_
#define _DVR_APP_NOTIFY_H_

#include <DVR_APP_DEF.h>
#include <list>

typedef void(*LPFN_AppNotifyCallback)(void* pNotifyContext, int lEventCode, void* pulEventParam1, void* pulEventParam2);

typedef struct 
{
    LPFN_AppNotifyCallback pNotifyCallback;
    void *pNotifyContext;
}DvrAppNotifyItem;

class DvrAppNotify
{
public:
    DvrAppNotify();
    ~DvrAppNotify();
    static DvrAppNotify *Get(void);

    int RegisterNotify(void *pNotifyContext, LPFN_AppNotifyCallback pNotifyCallback);
    int UnRegisterNotify(void *pNotifyContext, LPFN_AppNotifyCallback pNotifyCallback);
    int NotifyEventList(int lEventCode, void* pulEventParam1, void* pulEventParam2);

private:
    std::list<DvrAppNotifyItem> m_NotifyList;
};

int Dvr_AppNotify_Init(void);
int Dvr_AppNotify_DeInit(void);

#endif