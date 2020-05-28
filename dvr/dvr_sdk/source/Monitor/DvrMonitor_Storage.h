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

#ifndef _DVRMONITOR_STORAGE_H_
#define _DVRMONITOR_STORAGE_H_

#include <pthread.h>
#ifdef __linux__
#include <sys/poll.h>
#endif

#include <list>
#include <map>
#include <string>
#include <DVR_SDK_DEF.h>
#include "DvrSystemControl.h"

#include "osa_tsk.h"
#include "osa.h"

#define LOOPREC_STORAGE_MONITOR_SIZE_LIMITATION   (400 * 1024)/**< Default value 400MB */
#define DASREC_STORAGE_MONITOR_SIZE_LIMITATION   (500 * 1024)/**< Default value 500MB */
#define DASREC_APA_STORAGE_MONITOR_NUM_LIMITATION   2
#define DASREC_IACC_STORAGE_MONITOR_SIZE_LIMITATION   4

class DvrMonitorStorage
{
public:
	DvrMonitorStorage(void *sysCtrl);
	~DvrMonitorStorage();

    void 	 SetEnableLoopRecDetect(DVR_BOOL enable);
    DVR_BOOL GetEnableLoopRecDetect(void);
	void 	 SetLoopRecThreshold(DVR_U32 threshold);
	DVR_U32  GetLoopRecThreshold(void);
	void 	 SetEnableLoopRecMsg(DVR_BOOL enable);
    void 	 SetLoopRecQuota(DVR_U32 quota);
	void	 SetAvailableSpace(DVR_U32 space);
	DVR_U32  GetAvailableSpace(void);
    DVR_U32  GetLoopRecQuota(void);

    void 	 SetEnableDasRecDetect(DVR_BOOL enable);
    DVR_BOOL GetEnableDasRecDetect(void);
    void 	 SetEnableDasRecMsg(DVR_DAS_MONITOR* monitor);
    
private:
	DISABLE_COPY(DvrMonitorStorage)

	static void 	Monitor_StorageTaskEntry(void *ptr);

    OSA_TskHndl m_hThread;

    DVR_U32 m_LoopRecStorageMonSize;
    DVR_U32 m_LoopRecStorageQuota;
    DVR_U32 m_CardAvailableSpace;
    
    DVR_BOOL m_bEnableLoopRecDetect;
    DVR_BOOL m_bEnableLoopRecMsg;
    DVR_U8	m_LoopRecMsgFlag;
    
    DVR_U32 m_DasRecStorageMonSize;
    DVR_BOOL m_bEnableDasRecDetect;
    DVR_BOOL m_bEnableDasRecMsg[DVR_DAS_TYPE_NUM - DVR_DAS_TYPE_APA];
    DVR_U8	m_DasRecMsgFlag;

    DVR_BOOL m_bExit;
    DvrSystemControl *m_sysCtrl;
};

#endif
