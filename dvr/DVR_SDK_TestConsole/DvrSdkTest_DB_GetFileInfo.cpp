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
#include <ctime>
#include <DVR_SDK_INTFS.h>

#include "DvrSdkTest_DB_GetFileInfo.h"
#include "DvrSdkTestUtils.h"

using namespace std;

int DvrSdkTest_DB_GetFileInfo::Test()
{
    if (DvrSdkTestUtils::CurrentDrive() == NULL) {
        printf("The current drive is not selected\n");
        return DVR_RES_EFAIL;
    }

//! [DB_GetFileInfo Example]
    DVR_RESULT res;

    DVR_FILE_INFO fileInfo;

    // DvrSdkTestUtils::CurrentDrive() returns the current drive path
    // unsigned m_uDir
    res = Dvr_DB_GetFileInfo(m_hDvr, DvrSdkTestUtils::CurrentDrive(), m_enuType, m_uFile, &fileInfo);
    if (DVR_SUCCEEDED(res)) {
        printf("ID: %d\n", fileInfo.uId);
        printf("Parent ID: %d\n", fileInfo.uParId);
        printf("Relative Position: %d\n", fileInfo.uRelPos);
        printf("Size: %lld\n", fileInfo.ullFileSize64);
        printf("Modification Time: %lld\n", fileInfo.ullModifyTime);
        printf("Type: %d\n", fileInfo.eType);
        printf("Name: %s\n", fileInfo.szName);
    }

//! [DB_GetFileInfo Example]

    return res;
}
