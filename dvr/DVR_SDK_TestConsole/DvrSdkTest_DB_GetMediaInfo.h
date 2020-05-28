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

#ifndef _DVRSDKTEST_DB_GETMEDIAINFO_H_
#define _DVRSDKTEST_DB_GETMEDIAINFO_H_

#include <cstdlib>
#include "DvrSdkTestBase.h"

class DvrSdkTest_DB_GetMediaInfo : public DvrSdkTestBase
{
public:
	DvrSdkTest_DB_GetMediaInfo(DVR_HANDLE hDvr)
		: DvrSdkTestBase(hDvr)
    {
    }

    ~DvrSdkTest_DB_GetMediaInfo()
    {
    }

    const char *Command()
    {
        return "media";
    }

    const char *Usage()
    {
        const char *usage = ""
            "media <index>\n"
            "Show the media info for index-th directory of the current selected drive\n";

        return usage;
    }

    int ProcessCmdLine(int argc, char *argv[])
    {
        if (argc != 2)
            return DVR_RES_EFAIL;

        char *pos = NULL;
        m_uDir = strtol(argv[1], &pos, 10);

        return DVR_RES_SOK;
    }

    int Test();

private:
    unsigned m_uDir;
};

#endif
