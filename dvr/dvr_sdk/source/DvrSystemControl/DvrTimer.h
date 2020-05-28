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

#ifndef _DVRTIMER_H_
#define _DVRTIMER_H_

#include <DVR_SDK_DEF.h>

#include "osa.h"

class DvrTimerLinuxPrivate;
class DvrTimerWindowsPrivate;

class DvrTimer
{
public:

    typedef void *(*Callback)(void *arg);

	DvrTimer(Callback callback, void *context);
	~DvrTimer();

    DVR_RESULT Start(const struct timeval &tv);
	DVR_RESULT Stop();
private:
	DISABLE_COPY(DvrTimer)

#ifdef __linux__
    DvrTimerLinuxPrivate* m_p;
#else
    DvrTimerWindowsPrivate* m_p;
#endif
};

#endif
