/*===========================================================================*\
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
\*===========================================================================*/

#include <cstring>

#include "dprint.h"
#include "DvrMutex.h"

#ifdef __linux__
#include <pthread.h>

DvrMutex::DvrMutex()
{
    int res = pthread_mutex_init(&m_mutex, NULL);
    if (res != 0) {
        DPrint(DPRINT_ERR, "DvrMutex: cannot initialize pthread_mutex res = %d: %s\n", res, strerror(res));
    }
}

DvrMutex::~DvrMutex()
{
    int res = pthread_mutex_destroy(&m_mutex);
    if (res != 0) {
        DPrint(DPRINT_ERR, "DvrMutex: cannot destroy pthread_mutex res = %d: %s\n", res, strerror(res));
    }
}

void DvrMutex::Lock()
{
    pthread_mutex_lock(&m_mutex);
}

void DvrMutex::UnLock()
{
    pthread_mutex_unlock(&m_mutex);
}

#else

DvrMutex::DvrMutex()
{
    DWORD res;
    m_mutex = CreateMutex(NULL, FALSE, NULL);
    if (m_mutex == NULL) {
        res = GetLastError();
        if (res == ERROR_ALREADY_EXISTS) {
			DPrint(DPRINT_ERR, "DvrMutex: already exist.\n");
        } else {
			DPrint(DPRINT_ERR, "DvrMutex: cannot initialize mutex, error code = %d.\n", res);

        }        
    }
}

DvrMutex::~DvrMutex()
{
    int res = CloseHandle(m_mutex);
    if (res == 0) {
		DPrint(DPRINT_ERR, "DvrMutex: cannot destroy mutex res = %d: %s\n", res, GetLastError());
    }
}

void DvrMutex::Lock()
{
    WaitForSingleObject(m_mutex, INFINITE);
}

void DvrMutex::UnLock()
{
    ReleaseMutex(m_mutex);
}
#endif
