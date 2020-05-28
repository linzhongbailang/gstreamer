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

#ifndef _DVRSDKTEST_ENUMDEVICES_H_
#define _DVRSDKTEST_ENUMDEVICES_H_

#include "DvrSdkTestBase.h"

class DvrSdkTest_EnumDevices : public DvrSdkTestBase
{
public:
	DvrSdkTest_EnumDevices(DVR_HANDLE hDvr)
		: DvrSdkTestBase(hDvr)
    {
    }

	~DvrSdkTest_EnumDevices()
    {
    }

    const char *Command()
    {
        return "showdevice";
    }

    const char *Usage()
    {
        const char *usage = ""
            "showdevice\n"
            "List all the devices connected to the system\n";

        return usage;
    }

    int Test();

};

#endif
