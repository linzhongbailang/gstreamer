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

#ifndef _DVRSDKTESTCMD_POS_H_
#define _DVRSDKTESTCMD_POS_H_

#include <cstring>
#include <DVR_SDK_DEF.h>
#include "DvrSdkTestCmd.h"

class DvrSdkTestCmd_Pos : public DvrSdkTestCmd
{
public:
	DvrSdkTestCmd_Pos(DVR_HANDLE hDvr)
		: DvrSdkTestCmd(hDvr)
    {
    }
	~DvrSdkTestCmd_Pos()
    {
    }

    const char *Command()
    {
        return "pos";
    }
    const char *Usage()
    {
        const char *usage = ""
            "pos <on/off>\n"
            "show/hide the position information\n"
            "Examples\n"
            "pos on\n";
            
        return usage;
    }

    int ProcessCmdLine(int argc, char *argv[])
    {

        if (argc != 2) {
            return DVR_RES_EFAIL;
        }

        if (strcmp(argv[1], "on") == 0) {
            m_bOn = true;
        } else if (strcmp(argv[1], "off") == 0) {
            m_bOn = false;
        } else {
            return DVR_RES_EFAIL;
        }

        return DVR_RES_SOK;
    }

private:
    virtual int ExecCmd();

    bool m_bOn;
};

#endif
