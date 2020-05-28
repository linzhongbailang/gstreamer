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
#include <stdlib.h>
#include "DVR_APP_DEF.h"
#include "flow/misc/DvrMiscFormatCard.h"
#include "flow/widget/DvrWidget_Mgt.h"
#include "gui/Gui_DvrMiscFormatCard.h"

int misc_formatcard_func(DVR_U32 funcId, DVR_U32 param1, DVR_U32 param2);

DVR_RESULT app_misc_formatcard_start(void)
{
	DvrMiscFormatCard *app = DvrMiscFormatCard::Get();

	/* Set app function and operate sets */
	app->Func = misc_formatcard_func;
	app->Gui = gui_misc_formatcard_func;

	if (!APP_CHECKFLAGS(app->m_formatcard.GFlags, APP_AFLAGS_INIT)) {
		APP_ADDFLAGS(app->m_formatcard.GFlags, APP_AFLAGS_INIT);
		app->Func(MISC_APP_FORMATCARD_INIT, 0, 0);
	}

	if (APP_CHECKFLAGS(app->m_formatcard.GFlags, APP_AFLAGS_START)) {
		app->Func(MISC_APP_FORMATCARD_START_FLG_ON, 0, 0);
	}
	else {
		APP_ADDFLAGS(app->m_formatcard.GFlags, APP_AFLAGS_START);
		if (!APP_CHECKFLAGS(app->m_formatcard.GFlags, APP_AFLAGS_READY)) {
			APP_ADDFLAGS(app->m_formatcard.GFlags, APP_AFLAGS_READY);
		}

		app->Func(MISC_APP_FORMATCARD_START, 0, 0);
	}

	return DVR_RES_SOK;
}

DVR_RESULT app_misc_formatcard_stop(void)
{
	return DVR_RES_SOK;
}

DVR_RESULT app_misc_formatcard_on_message(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2)
{
	DVR_RESULT res = DVR_RES_SOK;
	DvrMiscFormatCard *app = DvrMiscFormatCard::Get();
	APP_MGT_INST *ParentApp = NULL;

	res = AppWidget_OnMessage(msg, param1, param2);
	if (res != DVR_RES_SOK)
	{
		return res;
	}

	switch (msg)
	{
		case ASYNC_MGR_MSG_CARD_FORMAT_DONE:
		{
			res = app->Func(MISC_APP_FORMATCARD_OP_DONE, param1, param2);
		}
		break;

		case AMSG_STATE_CARD_REMOVED:
		{
			if (APP_CHECKFLAGS(app->m_formatcard.GFlags, APP_AFLAGS_READY))
			{
				res = DvrAppMgt_GetApp(&ParentApp, app->m_formatcard.Parent);
				res = ParentApp->OnMessage(msg, param1, param2);
			}
			res = app->Func(MISC_APP_FORMATCARD_CARD_REMOVED, param1, param2);
		}
		break;

		default:
		{
#if 0
			if (DvrAppMgt_GetApp(&ParentApp, app->m_formatcard.Parent) == 0)
			{
				res = ParentApp->OnMessage(msg, param1, param2);
			}
#endif
		}
		break;
	}

	return res;
}

DvrMiscFormatCard::DvrMiscFormatCard()
{
	m_bHasInited = FALSE;
}

DVR_BOOL DvrMiscFormatCard::Init()
{
	if (m_bHasInited)
	{
		return TRUE;
	}

	m_formatcard.Id = 0;
	m_formatcard.Tier = 2;
	m_formatcard.Parent = 0;
	m_formatcard.Previous = 0;
	m_formatcard.Child = 0;
	m_formatcard.GFlags = APP_AFLAGS_OVERLAP;
	m_formatcard.Flags = 0;
	m_formatcard.Start = app_misc_formatcard_start;
	m_formatcard.Stop = app_misc_formatcard_stop;
	m_formatcard.OnMessage = app_misc_formatcard_on_message;

	m_bHasInited = TRUE;

	return m_bHasInited;
}

DvrMiscFormatCard *DvrMiscFormatCard::Get(void)
{
	static DvrMiscFormatCard formatcard;

	if (formatcard.Init())
	{
		return &formatcard;
	}

	return NULL;
}