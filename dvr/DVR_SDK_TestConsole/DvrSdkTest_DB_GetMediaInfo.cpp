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

#include <cstdio>
#include <DVR_SDK_INTFS.h>

#include "DvrSdkTest_DB_GetMediaInfo.h"
#include "DvrSdkTestUtils.h"

using namespace std;

int DvrSdkTest_DB_GetMediaInfo::Test()
{
    if (DvrSdkTestUtils::CurrentDrive() == NULL) {
        printf("The current drive is not selected\n");
        return DVR_RES_EFAIL;
    }

//! [DB_GetMediaInfo Example]
    DVR_RESULT res;
    DVR_MEDIA_INFO mediaInfo;

    // DvrSdkTestUtils::CurrentDrive() returns the current drive path
    // unsigned m_uDir
    res = Dvr_DB_GetMediaInfo(m_hDvr, DvrSdkTestUtils::CurrentDrive(), m_uDir, &mediaInfo);
    if (DVR_SUCCEEDED(res)) {
        printf("DRIVE: %s\n", mediaInfo.szName);
		printf("Directory Count: %d\n", mediaInfo.uSum[DVR_FILE_TYPE_DIRECTORY]);
		printf("Audio Count:     %d\n", mediaInfo.uSum[DVR_FILE_TYPE_AUDIO]);
		printf("Video Count:     %d\n", mediaInfo.uSum[DVR_FILE_TYPE_VIDEO]);
		printf("Image Count:     %d\n", mediaInfo.uSum[DVR_FILE_TYPE_IMAGE]);
    }
//! [DB_GetMediaInfo Example]

    return res;
}
