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

#ifndef _DVRSDKTESTCMD_H_
#define _DVRSDKTESTCMD_H_

#include <DVR_SDK_DEF.h>
#include "DvrSdkTestBase.h"

class DvrSdkTestCmd : public DvrSdkTestBase
{
public:
	DvrSdkTestCmd(DVR_HANDLE hDvr);
	virtual ~DvrSdkTestCmd();

    virtual int Test();

private:
    virtual int ExecCmd() = 0;
};

#endif
