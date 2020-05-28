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
#ifndef _DVRSDKTEST_PLAYER_SET_POSITION_H_
#define _DVRSDKTEST_PLAYER_SET_POSITION_H_

#include <cstring>
#include "DvrSdkTestBase.h"

class DvrSdkTest_Player_Set_Position : public DvrSdkTestBase
{
public:
	DvrSdkTest_Player_Set_Position(DVR_HANDLE hDvr)
		: DvrSdkTestBase(hDvr)
    {
    }

	~DvrSdkTest_Player_Set_Position()
    {
    }

    const char *Command()
    {
        return "seek";
    }

    const char *Usage()
    {
        const char *usage = ""
            "seek <position>\n"
            "Seek to the position (in Milisecondes) for current playing file\n"
            "Examples\n"
            "seek 60\n";
            
        return usage;
    }

    int ProcessCmdLine(int argc, char *argv[])
    {
        if (argc != 2) {
            return DVR_RES_EFAIL;
        }

        char *pos;
        int v = strtol(argv[1], &pos, 10);
        if (v < 0) {
            return DVR_RES_EFAIL;
        }

        m_uPos = v;
        return DVR_RES_SOK;
    }

    int Test();

private:
    unsigned m_uPos;
};

#endif
