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

#ifndef _DVRSDKTEST_OPEN_H_
#define _DVRSDKTEST_OPEN_H_

#include <set>
#include <string>
#include <cstring>
#include "DvrSdkTestBase.h"

class DvrSdkTest_MediaMgrOpen : public DvrSdkTestBase
{
public:
	DvrSdkTest_MediaMgrOpen(DVR_HANDLE hDvr)
        : DvrSdkTestBase(hDvr)
    {
    }

	~DvrSdkTest_MediaMgrOpen()
    {
    }

    const char *Command()
    {
        return "mmopen";
    }

    const char *Usage()
    {
        const char *usage = ""
            "mmopen\n"
            "Open the SDK and initialize all the media features\n"
            "Examples\n"
            "mmopen\n";

        return usage;
    }

    int ProcessCmdLine(int argc, char *argv[])
    {
        if (argc != 1) {
            return DVR_RES_EFAIL;
        }

        return DVR_RES_SOK;
    }

    int Test();

    static int Notify(void *pContext, DVR_NOTIFICATION_TYPE enuType, void *pParam1, void *pParam2);

private:
    int NotifyImp(DVR_NOTIFICATION_TYPE enuType, void *pParam1, void *pParam2);

    DVR_DB_FILESCAN m_scanParam;
    std::set<std::string> m_partialSet;
};

#endif
