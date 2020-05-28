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

#ifndef _DVRSDKTESTEXEC_H_
#define _DVRSDKTESTEXEC_H_

#include <windows.h>

#ifdef max
   #undef max
#endif
#ifdef min
   #undef min
#endif

#include <vector>
#include <DVR_SDK_DEF.h>

class DvrSdkTestBase;

class DvrSdkTestExec
{
public:
	DvrSdkTestExec();
	~DvrSdkTestExec();
    int Init();
    int Uninit();
    int ExecuteCmd(const char *szCmd);

private:
    int ParseStringToArgs(char *szString, char *szBuffer, int *pargc, char **argv);
    int NotifyImp(DVR_NOTIFICATION_TYPE enuType, void *pParam1, void *pParam2);

    static int  Notify(void *pContext, DVR_NOTIFICATION_TYPE enuType, void *pParam1, void *pParam2);
    static void Console(void *pContext, DVR_S32 nLevel, _IN const char *szString);
	static DWORD ThreadClient(LPVOID pParam);

    void PrintTestResult(int nRes);
    void LoadTest();
    void UnloadTest();

    DVR_HANDLE m_hDvr;
	HANDLE m_hThread;
	DWORD m_dwThreadID;
    //DVR_DB_FILESCAN m_scanParam;
	std::vector<DvrSdkTestBase *> m_tests;
};
#endif
