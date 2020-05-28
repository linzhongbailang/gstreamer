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

#include "DvrSdkTest_DB_GetDirInfo.h"
#include "DvrSdkTestUtils.h"

using namespace std;

int DvrSdkTest_DB_GetDirInfo::Test()
{
    if (DvrSdkTestUtils::CurrentDrive() == NULL) {
        printf("The current drive is not selected\n");
        return DVR_RES_EFAIL;
    }

//! [DB_GetDirInfo Example]
    DVR_RESULT res;
    DVR_DIR_INFO dirInfo;

    // DvrSdkTestUtils::CurrentDrive() returns the current drive path
    // unsigned m_uDir
    res = Dvr_DB_GetDirInfo(m_hDvr, DvrSdkTestUtils::CurrentDrive(), m_uDir, &dirInfo);
    if (DVR_SUCCEEDED(res)) {
        printf("ID: %d Parent ID: %d Relative Position: %d Size: %lld Modification Time: %lld\n", dirInfo.uId, dirInfo.uParId, dirInfo.uRelPos, dirInfo.ullFileSize64, dirInfo.ullModifyTime);
        const static char *fileType[] = {
            "dir",
            "audio",
            "video",
            "image",
            "playlist"
        };
        for (int i = 0; i < 5; i++) {
            unsigned count = 0;
			Dvr_DB_GetIdFromParRelPos(m_hDvr, DvrSdkTestUtils::CurrentDrive(), (DVR_FILE_TYPE)i, &count, dirInfo.uId, (unsigned)-1);
            for (unsigned j = 1; j <= count; j++) {
                unsigned id = 0;
				Dvr_DB_GetIdFromParRelPos(m_hDvr, DvrSdkTestUtils::CurrentDrive(), (DVR_FILE_TYPE)i, &id, dirInfo.uId, j);
                DVR_FILE_INFO fileInfo = {0};
				Dvr_DB_GetFileInfo(m_hDvr, DvrSdkTestUtils::CurrentDrive(), (DVR_FILE_TYPE)i, id, &fileInfo);
                printf("%8s %5u %s\n", fileType[i], fileInfo.uId, fileInfo.szName);

            }
            
			/*
            for (unsigned j = dirInfo.Range[i].uMinId; j < dirInfo.Range[i].uMaxId; j++) {
                DVR_FILE_INFO fileInfo;
                Dvr_DB_GetFileInfo(m_hDvr, DvrSdkTestUtils::CurrentDrive(), (DVR_FILE_TYPE)i, j, &fileInfo);
                printf("%8s %5u %s\n", fileType[i], fileInfo.uId, fileInfo.szName);
            } 
			*/
        }
    }
//! [DB_GetDirInfo Example]

    return res;

    return res;
}
