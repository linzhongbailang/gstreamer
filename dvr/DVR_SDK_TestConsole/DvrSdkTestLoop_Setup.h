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

#ifndef _DVRSDKTEST_LOOP_SETUP_H_
#define _DVRSDKTEST_LOOP_SETUP_H_

#include <cstring>
#include "DvrSdkTestBase.h"

class DvrSdkTest_Loop_Setup : public DvrSdkTestBase
{
public:
    DvrSdkTest_Loop_Setup(DVR_HANDLE hDvr)
		: DvrSdkTestBase(hDvr)
    {
    }

    ~DvrSdkTest_Loop_Setup()
    {
    }

    const char *Command()
    {
        return "loop";
    }

    const char *Usage()
    {
        const char *usage = ""
            "loop <session> <dir|video|audio|image|playlist> <index[:internal_index]> [random|shuffle|firstfile] [scan_msec]"
            "Loop files specified by <dir|video|audio|image|playlist> <index[:internal_index]> with the session specified by <session>\n"
            "If [random|shuffle|firstfile] is set, loop in the specified mode.\n"
            "if [scan_seconds] is set, each file is played for only [scan_msec] milli-seconds\n";

        return usage;
    }

    int ProcessCmdLine(int argc, char *argv[])
    {
        int i;
        char *pos;

        m_nInternalIndex = 0;

        if (argc < 4 || argc > 6) {
            return DVR_RES_EFAIL;
        }

        strcpy(m_szSession, argv[1]);

        static struct {
            const char *name;
           DVR_FILE_TYPE type;
        } type[] = {
            {"dir", DVR_FILE_TYPE_DIRECTORY},
            {"video", DVR_FILE_TYPE_VIDEO},
            {"audio", DVR_FILE_TYPE_AUDIO},
            {"image", DVR_FILE_TYPE_IMAGE},
        };

        for (i = 0; i < DVR_ARRAYSIZE(type); i++) {
            if (strcmp(argv[2], type[i].name) == 0) {
                m_enuType = type[i].type;
                break;
            }
        }

        if (i == DVR_ARRAYSIZE(type)) {
            return DVR_RES_EFAIL;
        }

        pos = NULL;
        char* pInternalIndex = strstr(argv[3], ":");
        if(pInternalIndex)
        {
            pInternalIndex += 1;
            m_nInternalIndex = strtol(pInternalIndex, &pos, 10);
        }
        m_nFileNum = strtol(argv[3], &pos, 10);

        m_eNextFileMode = DVR_NEXTFILE_MODE_SEQUENTIAL;
        m_eRecursive    = DVR_LOOP_RECURSIVE_ALLFILE;
        m_nScanMsec     = 0;

        if (argc > 4) {
            if (strcmp(argv[4], "random") == 0) {
                m_eNextFileMode = DVR_NEXTFILE_MODE_RANDOM;
            } else if (strcmp(argv[4], "shuffle") == 0) {
                m_eNextFileMode = DVR_NEXTFILE_MODE_SHUFFLE;
            } else if (strcmp(argv[4], "firstfile") == 0) {
                m_eRecursive = DVR_LOOP_RECURSIVE_FIRSTFILE;
            } else {
                m_nScanMsec = strtol(argv[4], &pos, 10);
            }
        }

        if (argc > 5) {
            m_nScanMsec = strtol(argv[5], &pos, 10);
        }
        return DVR_RES_SOK;
    }

    int Test();

private:
    char m_szSession[PATH_MAX];

    DVR_FILE_TYPE m_enuType;
    int             m_nFileNum;
    int             m_nScanMsec;
    int             m_nInternalIndex;

    DVR_LOOP_NEXTFILE_MODE m_eNextFileMode;
    DVR_LOOP_RECURSIVE     m_eRecursive;
};

#endif
