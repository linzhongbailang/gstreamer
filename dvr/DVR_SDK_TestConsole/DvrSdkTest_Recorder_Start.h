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
#ifndef _DVRSDKTEST_RECORDER_START_H_
#define _DVRSDKTEST_RECORDER_START_H_

#include <cstring>
#include "DvrSdkTestBase.h"

class DvrSdkTest_Recorder_Start : public DvrSdkTestBase
{
public:
	DvrSdkTest_Recorder_Start(DVR_HANDLE hDvr)
		: DvrSdkTestBase(hDvr)
	{
		
	}

	~DvrSdkTest_Recorder_Start()
	{
	}

	const char *Command()
	{
		return "recstart";
	}

	const char *Usage()
	{
		const char *usage = ""
			"recstart\n"
			"recstart recording\n"
			"Examples\n"
			"recstart\n";

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
