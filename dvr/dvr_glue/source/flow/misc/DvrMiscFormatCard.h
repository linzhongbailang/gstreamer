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
#ifndef _DVR_MISC_FORMAT_CARD_H_
#define _DVR_MISC_FORMAT_CARD_H_

#include "DVR_SDK_DEF.h"
#include "framework/DvrAppMgt.h"

/*************************************************************************
* App Flag Definitions
************************************************************************/
#define MISC_FORMATCARD_CARD_PROTECTED      (0x0001)
#define MISC_FORMATCARD_DO_FORMAT_CARD      (0x0002)
#define MISC_FORMATCARD_WARNING_MSG_RUN		(0x0004)

typedef enum 
{
	MISC_APP_FORMATCARD_INIT = 0,
	MISC_APP_FORMATCARD_START,
	MISC_APP_FORMATCARD_START_FLG_ON,
	MISC_APP_FORMATCARD_STOP,
	MISC_APP_FORMATCARD_OP_DONE,
	MISC_APP_FORMATCARD_OP_SUCCESS,
	MISC_APP_FORMATCARD_OP_FAILED,
	MISC_APP_FORMATCARD_SWITCH_APP,
	MISC_APP_FORMATCARD_CARD_REMOVED,
	MISC_APP_FORMATCARD_DIALOG_SHOW_FORMATCARD,
	MISC_APP_FORMATCARD_STATE_WIDGET_CLOSED,
	MISC_APP_FORMATCARD_WARNING_MSG_SHOW_START,
	MISC_APP_FORMATCARD_WARNING_MSG_SHOW_STOP
} MISC_APP_FORMATCARD_FUNC_ID;

class DvrMiscFormatCard
{
private:
	DvrMiscFormatCard();
	DvrMiscFormatCard(const DvrMiscFormatCard&);
	const DvrMiscFormatCard& operator = (const DvrMiscFormatCard&);

public:
	static DvrMiscFormatCard *Get(void);
	APP_MGT_INST *GetApp()
	{
		return &m_formatcard;
	}

	int(*Func)(DVR_U32 funcId, DVR_U32 param1, DVR_U32 param2);
	int(*Gui)(DVR_U32 guiCmd, DVR_U32 param1, DVR_U32 param2);

	APP_MGT_INST m_formatcard;

private:
	DVR_BOOL Init();
	DVR_BOOL m_bHasInited;
};


#endif
