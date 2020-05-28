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
#ifndef _HCMGR_HANDLER_H_
#define _HCMGR_HANDLER_H_

#include "DVR_SDK_DEF.h"
#include "DvrAppMgt.h"

#define APPS_MAX_NUM   (30)

class HcMgrHandler
{
public:
	HcMgrHandler(void);
	~HcMgrHandler(void);

	DVR_HCMGR_HANDLER *GetHandler(){ return &m_handler; }
	int GetAppId(int module);
	const char *GetAppName(int appId);

private:
	static void AppHandler_Entry(void *ptr);
	static int AppHandler_Exit(void);

	DVR_HCMGR_HANDLER m_handler;

	APP_MGT_INST *DvrAppPool[APPS_MAX_NUM];

	DVR_S32 m_app_main_id;
	DVR_S32 m_app_rec_id;
	DVR_S32 m_app_pb_id;
	DVR_S32 m_app_thumb_id;
	DVR_S32 m_app_misc_formatcard_id;
	DVR_S32 m_app_misc_defsetting_id;
};

#endif
