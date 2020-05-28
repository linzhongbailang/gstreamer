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

#ifndef _DVRSDKTEST_PLAYER_CLOSE_H_
#define _DVRSDKTEST_PLAYER_CLOSE_H_

#include <set>
#include <string>
#include <cstring>
#include "DvrSdkTestBase.h"

class DvrSdkTest_Player_Close : public DvrSdkTestBase
{
public:
	DvrSdkTest_Player_Close(DVR_HANDLE hDvr)
		: DvrSdkTestBase(hDvr)
    {
    }

	~DvrSdkTest_Player_Close()
    {
    }

    const char *Command()
    {
        return "close";
    }

    const char *Usage()
    {
        const char *usage = ""
			"close\n"
			"close current playing file\n"
            "Examples\n"
            "close\n";

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

private:
};

#endif
