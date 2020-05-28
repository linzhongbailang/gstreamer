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
#ifndef _THREAD_POOL_DVR_H_
#define _THREAD_POOL_DVR_H_

class CThreadPool
{
private:
    CThreadPool();
    CThreadPool(const CThreadPool&);
    const CThreadPool& operator = (const CThreadPool&);

public:
    static CThreadPool *Get(void);
    int AddTask(void (*run)(void *arg), void *arg);
    int Destroy();
    int Flush();

private:
    int Init();

    int m_bHasInited;
    void *m_pThreadPool;
};



#endif