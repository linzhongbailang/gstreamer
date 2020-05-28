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


#include "DvrSdkTest_MediaMgrOpen.h"
#include "DvrSdkTest_MediaMgrClose.h"

#include "DvrSdkTestCmd_Log.h"
#include "DvrSdkTestCmd_Select.h"
#include "DvrSdkTest_EnumDevices.h"
#include "DvrSdkTestCmd_Pos.h"

#include "DvrSdkTest_DB_GetMediaInfo.h"
#include "DvrSdkTest_DB_GetFileInfo.h"
#include "DvrSdkTest_DB_GetDirInfo.h"

#include "DvrSdkTest_Player_Open.h"
#include "DvrSdkTest_Player_Close.h"
#include "DvrSdkTest_Player_Play.h"
#include "DvrSdkTest_Player_Pause.h"
#include "DvrSdkTest_Player_Resume.h"
#include "DvrSdkTest_Player_Stop.h"
#include "DvrSdkTest_Player_Next.h"
#include "DvrSdkTest_Player_Prev.h"
#include "DvrSdkTest_Player_Set_Position.h"
#include "DvrSdkTest_Player_Set_Speed.h"
#include "DvrSdkTest_Player_Get_Position.h"
#include "DvrSdkTest_Player_Get_Speed.h"

#include "DvrSdkTest_Recorder_Start.h"
#include "DvrSdkTest_Recorder_Stop.h"

#include "DvrSdkTestExec.h"
#include "DvrSdkTestUtils.h"


#include <cstdio>
#include <cstring>
//#include <vector>

using namespace std;

DvrSdkTestExec::DvrSdkTestExec()
{
    m_hDvr = NULL;
	m_hThread = NULL;
    m_tests.clear();
}

DvrSdkTestExec::~DvrSdkTestExec()
{
	delete DvrSdkTestUtils::CurrentDrive();
}

void DvrSdkTestExec::LoadTest()
{
   m_tests.push_back(new DvrSdkTest_MediaMgrOpen(m_hDvr));
   m_tests.push_back(new DvrSdkTest_MediaMgrClose(m_hDvr));
   m_tests.push_back(new DvrSdkTestCmd_Log(m_hDvr));
   m_tests.push_back(new DvrSdkTestCmd_Select(m_hDvr));
   m_tests.push_back(new DvrSdkTest_EnumDevices(m_hDvr));
   m_tests.push_back(new DvrSdkTestCmd_Pos(m_hDvr));

   m_tests.push_back(new DvrSdkTest_DB_GetMediaInfo(m_hDvr));
   m_tests.push_back(new DvrSdkTest_DB_GetDirInfo(m_hDvr));
   m_tests.push_back(new DvrSdkTest_DB_GetFileInfo(m_hDvr));

   m_tests.push_back(new DvrSdkTest_Player_Open(m_hDvr));
   m_tests.push_back(new DvrSdkTest_Player_Close(m_hDvr));
   m_tests.push_back(new DvrSdkTest_Player_Play(m_hDvr));
   m_tests.push_back(new DvrSdkTest_Player_Stop(m_hDvr));
   m_tests.push_back(new DvrSdkTest_Player_Pause(m_hDvr));
   m_tests.push_back(new DvrSdkTest_Player_Resume(m_hDvr));
   m_tests.push_back(new DvrSdkTest_Player_Next(m_hDvr));
   m_tests.push_back(new DvrSdkTest_Player_Prev(m_hDvr));
   m_tests.push_back(new DvrSdkTest_Player_Set_Position(m_hDvr));
   m_tests.push_back(new DvrSdkTest_Player_Set_Speed(m_hDvr));
   m_tests.push_back(new DvrSdkTest_Player_Get_Position(m_hDvr));
   m_tests.push_back(new DvrSdkTest_Player_Get_Speed(m_hDvr));

   m_tests.push_back(new DvrSdkTest_Recorder_Start(m_hDvr));
   m_tests.push_back(new DvrSdkTest_Recorder_Stop(m_hDvr));
}

void DvrSdkTestExec::UnloadTest()
{
    vector<DvrSdkTestBase *>::iterator it;
    for (it = m_tests.begin(); it != m_tests.end(); it++) {
        delete *it;
    }

    m_tests.clear();
}

int DvrSdkTestExec::ExecuteCmd(const char *szCmd)
{
    int   nRes;
    char *pBuf, *pCmd;
    int   argc = 256;
    char *argv[256];

    pBuf = strdup(szCmd);
    if (pBuf == NULL)
        return DVR_RES_EOUTOFMEMORY;
    pCmd = strdup(szCmd);
    if (pCmd == NULL) {
        free(pBuf);
        return DVR_RES_EOUTOFMEMORY;
    }

    ParseStringToArgs(pCmd,pBuf,&argc,argv);

    vector<DvrSdkTestBase *>::iterator it;
    for (it = m_tests.begin(); it != m_tests.end(); it++) {
        if (strcmp((*it)->Command(), argv[0]) == 0) {
            nRes = (*it)->ProcessCmdLine(argc, argv);
            if (DVR_FAILED(nRes)) {
                printf("%s\n", (*it)->Usage());
            } 
			else 
			{
                nRes = (*it)->Test();
				PrintTestResult(nRes);
            }
            break;
        }
    }
    if (it == m_tests.end()) {
        if (strcmp(argv[0], "help") == 0) {
            for (it = m_tests.begin(); it != m_tests.end(); it++) {
                printf("%s\n", (*it)->Command());
                printf("%s\n", (*it)->Usage());
            }
        } else {
            printf("Unknown command\n");
        }
    }

    free(pCmd);
    free(pBuf);
    return nRes;
}

int DvrSdkTestExec::ParseStringToArgs(char *szString, char *szBuffer, int *pargc, char **argv)
{
    char *sptr, *dptr;
    int argc, quote, semicolon;
    static char semicolon_form[] = (";");

    if(pargc==0 || argv==0)
        return 0;
    if(szString==0)
    {
        *pargc = 0;
        return 0;
    }
    for(sptr=szString,dptr=szBuffer,semicolon=0,argc=0;argc<*pargc;argc++)
    {
        if(semicolon)
        {
            if(argc && argv[argc-1]!=semicolon_form)
                argv[argc++] = semicolon_form;
            semicolon = 0;
        }
        for(;*sptr==' '||*sptr=='\t'||*sptr=='\n';sptr++);
        if(*sptr==0)
            break;
        argv[argc] = dptr;
        quote = 0;
        for(;*sptr;sptr++)
        {
            if(*sptr==('\\'))
            {
                sptr++;
                if(*sptr==0)
                    break;
                *dptr++ = *sptr;
            }
            else if(quote)
            {
                if(*sptr==('"'))
                    quote = 0;
                else
                    *dptr++ = *sptr;
            }
            else
            {
                if(*sptr==(' ')||*sptr==('\t')||*sptr==('\n'))
                    break;
                if(*sptr==(';'))
                {    
                    sptr++;
                    if(argv[argc]==dptr)
                        argc--;
                    semicolon = 1;
                    break;
                }
                if(*sptr==('"'))
                    quote = 1;
                else
                    *dptr++ = *sptr;
            }
        }
        *dptr++ = 0;
    }
    *pargc = argc;
    return argc;
}

void DvrSdkTestExec::PrintTestResult(int nRes)
{
	switch(nRes) {
	case DVR_RES_SOK:
		printf("DVR_RES_SOK\n");
		break;
	case DVR_RES_ENOTIMPL:
		printf("DVR_RES_ENOTIMPL\n");
		break;
	case DVR_RES_EPOINTER:
		printf("DVR_RES_EPOINTER\n");
		break;
	case DVR_RES_EFAIL:
		printf("DVR_RES_EFAIL\n");
		break;
	case DVR_RES_EUNEXPECTED:
		printf("DVR_RES_EUNEXPECTED\n");
		break;
	case DVR_RES_EOUTOFMEMORY:
		printf("DVR_RES_EOUTOFMEMORY\n");
		break;
	case DVR_RES_EINVALIDARG:
		printf("DVR_RES_EINVALIDARG\n");
		break;
	case DVR_RES_EINPBUFTOOSMALL:
		printf("DVR_RES_EINPBUFTOOSMALL\n");
		break;
	case DVR_RES_EOUTBUFTOOSMALL:
		printf("DVR_RES_EOUTBUFTOOSMALL\n");
		break;
	case DVR_RES_EMODENOTSUPPORTED:
		printf("DVR_RES_EMODENOTSUPPORTED\n");
		break;
    case DVR_RES_EDEMUX:
        printf("DVR_RES_EDEMUX\n");
        break;
    case DVR_RES_EAUDIO_DECODER:
        printf("DVR_RES_EAUDIO_DECODER\n");
        break;
    case DVR_RES_EAUDIO_RENDER:
        printf("DVR_RES_EAUDIO_RENDER\n");
        break;
    case DVR_RES_EVIDEO_DECODER:
        printf("DVR_RES_EVIDEO_DECODER\n");
        break;
    case DVR_RES_EVIDEO_RENDER:
        printf("DVR_RES_EVIDEO_RENDER\n");
        break;
	default:
		printf("Unknown Result\n");
		break;
	}
}
