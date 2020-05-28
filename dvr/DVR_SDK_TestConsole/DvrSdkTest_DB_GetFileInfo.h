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

#ifndef _DVRSDKTEST_DB_GETFILEINFO_H_
#define _DVRSDKTEST_DB_GETFILEINFO_H_

#include <cstring>
#include <cstdlib>
#include "DvrSdkTestBase.h"

class DvrSdkTest_DB_GetFileInfo : public DvrSdkTestBase
{
public:
	DvrSdkTest_DB_GetFileInfo(DVR_HANDLE hDvr)
		: DvrSdkTestBase(hDvr)
    {
    }

	~DvrSdkTest_DB_GetFileInfo()
    {
    }

    const char *Command()
    {
        return "file";
    }

    const char *Usage()
    {
        const char *usage = ""
            "file <audio|video|image|playlist> <index>\n"
            "Show the file information about the index-th file of type <audio|video|image|playlist> in the current selected drive\n";

        return usage;
    }

    int ProcessCmdLine(int argc, char *argv[])
    {
        if (argc != 3)
            return DVR_RES_EFAIL;

        static struct {
            const char *name;
            DVR_FILE_TYPE type;
        } type[] = {
			{ "video", DVR_FILE_TYPE_VIDEO },
			{ "audio", DVR_FILE_TYPE_AUDIO },
			{ "image", DVR_FILE_TYPE_IMAGE },
        };

        for (int i = 0; i < DVR_ARRAYSIZE(type); i++) {
            if (strcmp(argv[1], type[i].name) == 0) {
                m_enuType = type[i].type;
                break;
            }
        }

        char *pos = NULL;
        m_uFile = strtol(argv[2], &pos, 10);

        return DVR_RES_SOK;
    }

    int Test();

private:
    DVR_FILE_TYPE m_enuType;
    unsigned m_uFile;
};

#endif
