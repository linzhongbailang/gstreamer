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
#ifndef _DVRSDKTEST_PLAYER_PREVIOUS_H_
#define _DVRSDKTEST_PLAYER_PREVIOUS_H_

#include <cstring>
#include "DvrSdkTestBase.h"

class DvrSdkTest_Player_Prev : public DvrSdkTestBase
{
public:
	DvrSdkTest_Player_Prev(DVR_HANDLE hDvr)
		: DvrSdkTestBase(hDvr)
    {
    }

	~DvrSdkTest_Player_Prev()
    {
    }

    const char *Command()
    {
        return "prev";
    }

    const char *Usage()
    {
        const char *usage = ""
			"prev\n"
			"play previous file in current playlist\n"
			"Examples\n"
			"prev\n";

        return usage;
    }

    int ProcessCmdLine(int argc, char *argv[])
    {
        if (argc != 1)
            return DVR_RES_EFAIL;

        return DVR_RES_SOK;
    }

    int Test();

private:

};

#endif
