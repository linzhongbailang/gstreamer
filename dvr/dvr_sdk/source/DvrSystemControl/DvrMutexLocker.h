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

#ifndef _DVRMUTEXLOCKER_H_
#define _DVRMUTEXLOCKER_H_

#include "osa.h"
#include "DvrMutex.h"

class DvrMutexLocker
{
public:
	DvrMutexLocker(DvrMutex *mutex);
	~DvrMutexLocker();

private:
    DvrMutex *m_mutex;
};

#endif
