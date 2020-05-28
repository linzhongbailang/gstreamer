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
#ifndef _DVRSDKTEST_SET_NAV_SPEED_H_
#define _DVRSDKTEST_SET_NAV_SPEED_H_

#include <cstring>
#include "DvrSdkTestBase.h"

class DvrSdkTest_Player_Set_Speed : public DvrSdkTestBase
{
public:
	DvrSdkTest_Player_Set_Speed(DVR_HANDLE hDvr)
		: DvrSdkTestBase(hDvr)
    {
    }

	~DvrSdkTest_Player_Set_Speed()
    {
    }

    const char *Command()
    {
        return "speed";
    }

    const char *Usage()
    {
        const char *usage = ""
            "speed <value>\n"
            "Set the play speed to value. Positive speed value means fast forward and negative speed value means rewind.\n"
            "The playing can be speeded up or slowed down. value = 1000 means the normal speed (1X). value = 100 means ten times slower (0.1X).\n"
            "And value = 2000 means two times faster (2X)\n"
            "Examples\n"
            "speed 2000\n";
            
        return usage;
    }

    int ProcessCmdLine(int argc, char *argv[])
    {
        if (argc != 2) {
            return DVR_RES_EFAIL;
        }

        char *pos;
        m_nSpeed = strtol(argv[1], &pos, 10);

        return DVR_RES_SOK;
    }

    int Test();

private:
    int  m_nSpeed;
};

#endif
