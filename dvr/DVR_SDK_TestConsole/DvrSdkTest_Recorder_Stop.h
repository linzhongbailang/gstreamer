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
#ifndef _DVRSDKTEST_RECORDER_STOP_H_
#define _DVRSDKTEST_RECORDER_STOP_H_

#include <cstring>
#include "DvrSdkTestBase.h"

class DvrSdkTest_Recorder_Stop : public DvrSdkTestBase
{
public:
	DvrSdkTest_Recorder_Stop(DVR_HANDLE hDvr)
		: DvrSdkTestBase(hDvr)
	{
	}

	~DvrSdkTest_Recorder_Stop()
	{
	}

	const char *Command()
	{
		return "recstop";
	}

	const char *Usage()
	{
		const char *usage = ""
			"recstop\n"
			"recstop current recording file\n"
			"Examples\n"
			"recstop\n";

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
