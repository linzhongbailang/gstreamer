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
#include <cstring>
#include <cstdlib>
#ifdef __linux__
#include <unistd.h>
#else
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif
#include "DvrSdkTestExec.h"

int main(int argc, const char* argv[])
{
#ifdef WIN32
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF);
#endif
    int nRes = 0;
    char szCommand[2048];
    static const char exitstr[] = "exit";

	DvrSdkTestExec *pTestExec = new DvrSdkTestExec();
    if (pTestExec == NULL)
        goto cleanup;

    // Test Initialize
    nRes = pTestExec->Init();
    if (DVR_FAILED(nRes))
        goto cleanup;

    for (int i = 0; ; i++)
    {
        printf("%d> ",i);
        fflush(stdout);
        if(fgets(szCommand,DVR_ARRAYSIZE(szCommand),stdin)==0)
        {
//            usleep(200*1000);
            continue;
        }
        if(strncmp(szCommand,exitstr,strlen(exitstr))==0) {
            pTestExec->ExecuteCmd("mmclose");
            break;
        }
        if(strcmp(szCommand, "\n") == 0)
            continue;
        pTestExec->ExecuteCmd(szCommand);
    }

    pTestExec->Uninit();

cleanup:
    if (pTestExec)
    {
        delete pTestExec;
        pTestExec = NULL;
    }

#ifdef WIN32
 	_CrtDumpMemoryLeaks();
#endif
   return !nRes;
}
