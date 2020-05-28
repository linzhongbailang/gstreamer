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
#ifndef _DVRSDKTEST_PLAYER_GET_SPEED_H_
#define _DVRSDKTEST_PLAYER_GET_SPEED_H_
#include <cstring>
#include "DvrSdkTestBase.h"

class DvrSdkTest_Player_Get_Speed : public DvrSdkTestBase
{
public:
	DvrSdkTest_Player_Get_Speed(DVR_HANDLE hDvr)
		: DvrSdkTestBase(hDvr)
	{
	}

	~DvrSdkTest_Player_Get_Speed()
	{
	}

	const char *Command()
	{
		return "getspeed";
	}

	const char *Usage()
	{
		const char *usage = ""
			"getspeed\n"
			"Get the play speed. Positive speed value means fast forward and negative speed value means rewind.\n"
			"The playing can be speeded up or slowed down. value = 1000 means the normal speed (1X). value = 100 means ten times slower (0.1X).\n"
			"And value = 2000 means two times faster (2X)\n"
			"Examples\n"
			"getspeed\n";

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
	int  m_nSpeed;
};

#endif
