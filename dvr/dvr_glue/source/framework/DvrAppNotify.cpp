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
#include <stdlib.h>
#include <string.h>
#include <DVR_SDK_DEF.h>
#include "DvrAppNotify.h"

static DvrAppNotify *g_hAppNotify = NULL;

DvrAppNotify::DvrAppNotify()
{
    m_NotifyList.clear();
}

DvrAppNotify::~DvrAppNotify()
{

}

int DvrAppNotify::RegisterNotify(void *pNotifyContext, LPFN_AppNotifyCallback pNotifyCallback)
{
    std::list<DvrAppNotifyItem>::iterator i;
    DvrAppNotifyItem item;

    if (pNotifyCallback == 0)
        return DVR_RES_EPOINTER;

    for (i = m_NotifyList.begin(); i != m_NotifyList.end(); ++i)
    {
        if (i->pNotifyCallback == pNotifyCallback && i->pNotifyContext == pNotifyContext)
            return DVR_RES_SOK;	// already exists
    }
    item.pNotifyCallback = pNotifyCallback;
    item.pNotifyContext = pNotifyContext;
    m_NotifyList.push_back(item);

    return DVR_RES_SOK;
}

int DvrAppNotify::UnRegisterNotify(void *pNotifyContext, LPFN_AppNotifyCallback pNotifyCallback)
{
    std::list<DvrAppNotifyItem>::iterator i;

    if (pNotifyCallback == 0)
        return DVR_RES_EPOINTER;

    for (i = m_NotifyList.begin(); i != m_NotifyList.end(); ++i)
    {
        if (i->pNotifyCallback == pNotifyCallback && i->pNotifyContext == pNotifyContext)
        {
            m_NotifyList.erase(i);
            return DVR_RES_SOK;
        }
    }
    return DVR_RES_EFAIL;
}

int DvrAppNotify::NotifyEventList(int lEventCode, void* pulEventParam1, void* pulEventParam2)
{
    std::list<DvrAppNotifyItem>::iterator i;

    for (i = m_NotifyList.begin(); i != m_NotifyList.end(); ++i)
    {
        if (i->pNotifyCallback)
            i->pNotifyCallback(i->pNotifyContext, lEventCode, pulEventParam1, pulEventParam2);
    }

    return DVR_RES_SOK;
}

DvrAppNotify *DvrAppNotify::Get(void)
{
    return g_hAppNotify;
}

int Dvr_AppNotify_Init(void)
{
    if (g_hAppNotify != NULL)
        return DVR_RES_EUNEXPECTED;

    g_hAppNotify = new DvrAppNotify();
    if (g_hAppNotify == NULL)
        return DVR_RES_EOUTOFMEMORY;

    return DVR_RES_SOK;
}

int Dvr_AppNotify_DeInit(void)
{
    if (g_hAppNotify == NULL)
        return DVR_RES_EUNEXPECTED;

    delete g_hAppNotify;
    g_hAppNotify = NULL;

    return DVR_RES_SOK;
}