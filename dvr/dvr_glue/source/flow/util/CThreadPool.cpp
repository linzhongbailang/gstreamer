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
#include "flow/util/CThreadPool.h"
#include "thpool.h"

#define NUM_THREADS_INPOOL  4

CThreadPool::CThreadPool()
{
    m_bHasInited = 0;
}

int CThreadPool::Init()
{
    if (m_bHasInited)
    {
        return 1;
    }

    m_pThreadPool = thpool_init(NUM_THREADS_INPOOL);
    if (m_pThreadPool != NULL)
    {
        m_bHasInited = 1;
    }

    return m_bHasInited;
}

int CThreadPool::Destroy()
{
    if (m_pThreadPool != NULL)
    {
        thpool_destroy((threadpool)m_pThreadPool);
        m_pThreadPool = NULL;
    }

    m_bHasInited = 0;

    return 0;
}

int CThreadPool::Flush()
{
    if (m_pThreadPool != NULL)
    {
        thpool_Flush((threadpool)m_pThreadPool);
    }

    return 0;
}
CThreadPool *CThreadPool::Get(void)
{
    static CThreadPool pool;

    if (pool.Init())
    {
        return &pool;
    }

    return NULL;
}

int CThreadPool::AddTask(void (*run)(void *arg), void *arg)
{
    return thpool_add_work((threadpool)m_pThreadPool, run, arg);
}