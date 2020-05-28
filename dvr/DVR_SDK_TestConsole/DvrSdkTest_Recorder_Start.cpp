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
#include <cstddef>
#include <windows.h>

#include <DVR_SDK_INTFS.h>
#include "DvrSdkTest_Recorder_Start.h"

int DvrSdkTest_Recorder_Start::Test()
{
	//! [Start Example]
	DVR_RESULT res;
	res = Dvr_Recorder_Start(m_hDvr);
	//! [Start Example]

	return res;
}