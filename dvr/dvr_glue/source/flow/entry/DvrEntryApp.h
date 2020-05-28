/*******************************************************************************
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
\*********************************************************************************/

#ifndef _DVR_ENTRY_APP_H_
#define _DVR_ENTRY_APP_H_

#include "DVR_SDK_DEF.h"
#include "framework/DvrAppMgt.h"

class DvrAppEntry
{
private:
	DvrAppEntry();
	DvrAppEntry(const DvrAppEntry&);
	const DvrAppEntry& operator = (const DvrAppEntry&);

public:
	static DvrAppEntry *Get(void);
	APP_MGT_INST *GetApp()
	{
		return &m_entry;
	}

	APP_MGT_INST m_entry;

private:
	DVR_BOOL Init();
	DVR_BOOL m_bHasInited;
};

#endif