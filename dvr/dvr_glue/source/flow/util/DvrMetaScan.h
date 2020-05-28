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
#ifndef _DVR_META_SCAN_H_
#define _DVR_META_SCAN_H_

#include <pthread.h>
#include <list>
#include "DVR_SDK_INTFS.h"
#include "DVR_APP_INTFS.h"

#ifndef MAX_PATH
#define MAX_PATH		260
#endif

enum METASCANCMD
{
    METASCANCMD_ADDFILE,
    METASCANCMD_STOP,
    METASCANCMD_EXIT,
};

typedef struct
{
    METASCANCMD Cmd;
    char szFileName[MAX_PATH];
    int eMediaType;
    int eFolderType;
}MetaDispNameScanParam;

typedef struct
{
    METASCANCMD Cmd;
    DVR_PREVIEW_OPTION option;
    unsigned char *pPreviewBuf;
    DVR_U32 nPreviewBufSize;
}MetaPreviewScanParam;

class DvrMetaScan
{
private:
    DvrMetaScan(const DvrMetaScan&);
    const DvrMetaScan& operator = (const DvrMetaScan&);

public:
    DvrMetaScan();
    ~DvrMetaScan();

    static DvrMetaScan *Get(void);
    int AddFileForDispName(MetaDispNameScanParam *pParam);
    int AddFileForPreview(MetaPreviewScanParam *pParam);
    static void ResetPhotoGroupId()
    {
        m_photoGroupId = -1;
    }

    pthread_mutex_t* GetDispLock()
    {
        return &m_DispQueueLock;
    }

    pthread_mutex_t* GetPreviewLock()
    {
        return &m_PreviewQueueLock;
    }

    std::list<MetaDispNameScanParam>* GetDispNameQueue()
    {
        return &m_DispNameQueue;
    }

    std::list<MetaPreviewScanParam>* GetPreviewQueue()
    {
        return &m_PreviewQueue;
    }

    void ClearQueue();

private:
    pthread_t pthreadDispName;
    pthread_t pthreadPreview;

    std::list<MetaDispNameScanParam> m_DispNameQueue;
    std::list<MetaPreviewScanParam> m_PreviewQueue;

    pthread_mutex_t m_DispQueueLock;
    pthread_mutex_t m_PreviewQueueLock;
    pthread_mutex_t m_ScanLock;

    static int m_photoGroupId;
    char m_PhotoDispName[APP_MAX_FN_SIZE];

    static void* ThreadGetDispName(void* pParam);
    static void* ThreadGetPreview(void* pParam);
    int SendExit();
};

class CAutoScanLock
{
public:
    CAutoScanLock(pthread_mutex_t *lock)
    {
        m_pLock = lock;
        if (m_pLock)
            pthread_mutex_lock(m_pLock);
    }
    ~CAutoScanLock()
    {
        if (m_pLock)
        {
            pthread_mutex_unlock(m_pLock);
            m_pLock = 0;
        }
    }
    pthread_mutex_t *m_pLock;
};

int Dvr_MetaScan_Init(void);
int Dvr_MetaScan_DeInit(void);

#endif