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

#include <vector>
#include <cstring>
#include <cstdio>
#include <DVR_SDK_INTFS.h>
#include "DvrSdkTestExec.h"
#include "DvrSdkTestUtils.h"
#include <gst/canbuf/can.h>

using namespace std;

#define FRAME_SIZE 2560*1440*3/2

#ifdef WIN32
char phy_buffer[FRAME_SIZE];
#else
void *phy_buffer;
#endif

DWORD DvrSdkTestExec::ThreadClient(LPVOID pParam)
{
	int ret = DVR_RES_SOK;

	//void *phy_buffer;

	DvrSdkTestExec *pThis = (DvrSdkTestExec *)pParam;
	long long frame_cnt = 0;

	for (;;)
	{
		while(NULL == DvrSdkTestUtils::CurrentDrive())
		{
#ifdef WIN32
			Sleep(10);
#else
			usleep(10*1000);
#endif
		}
	
#ifdef __linux__
		ret = Dvr_Recorder_AcquireInputBuf(pThis->m_hDvr, &phy_buffer);
		if (ret != DVR_RES_SOK)
		{
			//printf("Dvr_Recorder_AcquireInputBuf failed, ret=0x%x\n", ret);
			usleep(10*1000);
			continue;
		}
#endif
		Ofilm_Can_Data_T can;
		memset(&can, 0, sizeof(Ofilm_Can_Data_T));
		can.vehicle_data.time_stamp = frame_cnt;
		while (DVR_RES_SOK != Dvr_Recorder_AddFrame(pThis->m_hDvr, phy_buffer, 352, 288, &can, sizeof(Ofilm_Can_Data_T)))
		{
#ifdef WIN32
			Sleep(5);
#else
			usleep(5*1000);
#endif
		}

		frame_cnt++;

#ifdef WIN32
		Sleep(40);
#else
		usleep(40*1000);
#endif
	}
}

int DvrSdkTestExec::Init()
{
    DVR_RESULT res;
//! [Initialize Example]
	res = Dvr_Initialize(&m_hDvr);
//! [Initialize Example]
    if (DVR_FAILED(res))
        return res;

 	if (m_hThread == NULL)
 	{
 		m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadClient, this, 0, &m_dwThreadID);
 		if (m_hThread == NULL)
 			return DVR_RES_EFAIL;
 	}

//! [RegisterConsole Example]
	res = Dvr_RegisterConsole(m_hDvr, Console, this);
//! [RegisterConsole Example]
    if (DVR_FAILED(res)) {
		Dvr_Uninitialize(m_hDvr);
        return res;
    }

    LoadTest();

    return res;
}

int DvrSdkTestExec::Uninit()
{
    DVR_RESULT res;

//! [UnregisterConsole Example]
	Dvr_UnregisterConsole(m_hDvr, Console, this);
//! [UnregisterConsole Example]

//! [Uninitialize Example]
	res = Dvr_Uninitialize(m_hDvr);
//! [Uninitialize Example]

    UnloadTest();

    return res;
}

void DvrSdkTestExec::Console(void *pContext, DVR_S32 nLevel, _IN const char *szString)
{
	DvrSdkTestExec *p = (DvrSdkTestExec *)pContext;
    if (nLevel <= DvrSdkTestUtils::LogLevel())
        printf("%s", szString);
}
