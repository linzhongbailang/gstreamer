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

#ifndef _DVRSDKTEST_PLAYER_OPEN_H_
#define _DVRSDKTEST_PLAYER_OPEN_H_

#include <set>
#include <string>
#include <cstring>
#include "DvrSdkTestBase.h"

class DvrSdkTest_Player_Open : public DvrSdkTestBase
{
public:
	DvrSdkTest_Player_Open(DVR_HANDLE hDvr)
		: DvrSdkTestBase(hDvr)
    {
    }

	~DvrSdkTest_Player_Open()
    {
    }

    const char *Command()
    {
        return "open";
    }

    const char *Usage()
    {
        const char *usage = ""
			"open <filename>\n"
			"Open the file with <filename> to play\n"
            "Examples\n"
            "open 1.MP4\n";

        return usage;
    }

    int ProcessCmdLine(int argc, char *argv[])
    {
        if (argc != 2) {
            return DVR_RES_EFAIL;
        }

		strcpy(m_szFileName, argv[1]);

        return DVR_RES_SOK;
    }

    int Test();

private:
    char m_szFileName[PATH_MAX];
};

#endif
