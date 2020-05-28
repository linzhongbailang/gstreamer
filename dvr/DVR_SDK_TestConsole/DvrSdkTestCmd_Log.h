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

#ifndef _DVRSDKTESTCMD_LOG_H_
#define _DVRSDKTESTCMD_LOG_H_

#include <DVR_SDK_DEF.h>
#include "DvrSdkTestCmd.h"

class DvrSdkTestCmd_Log : public DvrSdkTestCmd
{
public:
	DvrSdkTestCmd_Log(DVR_HANDLE hDvr)
		: DvrSdkTestCmd(hDvr)
    {
    }
	~DvrSdkTestCmd_Log()
    {
    }

    const char *Command()
    {
        return "log";
    }
    const char *Usage()
    {
        const char *usage = ""
            "log <level>\n"
            "Print all the log informations whose level is smaller the specified one\n"
            "Examples\n"
            "log 5\n";
            
        return usage;
    }

    int ProcessCmdLine(int argc, char *argv[])
    {
        if (argc != 2)
            return DVR_RES_EFAIL;

        char *pos = NULL;
        m_nLogLevel = strtol(argv[1], &pos, 10);

        return DVR_RES_SOK;
    }

private:
    virtual int ExecCmd();

    int m_nLogLevel;
};

#endif
