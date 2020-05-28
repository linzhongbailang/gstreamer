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

#ifndef _DVRSDKTESTBASE_H_
#define _DVRSDKTESTBASE_H_

#ifndef __linux__
#include <windows.h>
#endif

#include <cstdio>
#include <DVR_SDK_DEF.h>

class DvrSdkTestBase
{
public:
	DvrSdkTestBase(DVR_HANDLE hDvr);
	virtual ~DvrSdkTestBase();

public:
    virtual const char *Command() = 0;
    virtual const char *Usage()   = 0;
    virtual int ProcessCmdLine(int argc, char *argv[]);
    virtual int Test() = 0;

protected:
    DVR_HANDLE m_hDvr;
};
#endif
