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
#include <unistd.h>
#include "sys/sysinfo.h"
#include "DVR_APP_DEF.h"
#include "DVR_SDK_DEF.h"
#include "flow/entry/DvrEntryApp.h"
#include "framework/DvrAppNotify.h"


DVR_RESULT app_main_start(void)
{
	DvrAppEntry *app = DvrAppEntry::Get();

	if (!APP_CHECKFLAGS(app->m_entry.GFlags, APP_AFLAGS_INIT)) {
		APP_ADDFLAGS(app->m_entry.GFlags, APP_AFLAGS_INIT);
	}
	if (!APP_CHECKFLAGS(app->m_entry.GFlags, APP_AFLAGS_START)) {
		APP_ADDFLAGS(app->m_entry.GFlags, APP_AFLAGS_START);
	}
	if (!APP_CHECKFLAGS(app->m_entry.GFlags, APP_AFLAGS_READY)) {
		APP_ADDFLAGS(app->m_entry.GFlags, APP_AFLAGS_READY);
	}

	return DVR_RES_SOK;
}

DVR_RESULT app_main_stop(void)
{
	return DVR_RES_SOK;
}

DVR_RESULT app_main_on_message(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2)
{
	DVR_RESULT res = DVR_RES_SOK;

	switch (msg)
	{
		case AMSG_STATE_CARD_INSERT_ACTIVE:
		{
			DVR_U32 DVR_SDCardErrorStatus = DVR_SDCARD_ERROR_STATUS_NO_ERROR;
            //printf("app_main_on_message send AMSG_STATE_CARD_INSERT_ACTIVE \n");
			DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_ERROR_STATUS_UPDATE, (void *)&DVR_SDCardErrorStatus, NULL);
		}
		break;

		case AMSG_STATE_CARD_REMOVED:
		{
			DVR_U32 DVR_SDCardErrorStatus = DVR_SDCARD_ERROR_STATUS_NO_SDCARD;
			DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_ERROR_STATUS_UPDATE, (void *)&DVR_SDCardErrorStatus, NULL);
		}
		break;

		default:
			break;
	}

	return res;
}

DvrAppEntry::DvrAppEntry()
{
	m_bHasInited = FALSE;
}

DVR_BOOL DvrAppEntry::Init()
{
	if (m_bHasInited)
	{
		return TRUE;
	}

	m_entry.Id = 0;
	m_entry.Tier = 0;
	m_entry.Parent = 0;
	m_entry.Previous = 0;
	m_entry.Child = 0;
	m_entry.GFlags = APP_AFLAGS_INIT | APP_AFLAGS_START | APP_AFLAGS_READY;
	m_entry.Flags = 0;
	m_entry.Start = app_main_start;
	m_entry.Stop = app_main_stop;
	m_entry.OnMessage = app_main_on_message;

	m_bHasInited = TRUE;

	return m_bHasInited;
}

DvrAppEntry *DvrAppEntry::Get(void)
{
	static DvrAppEntry entry;

	if (entry.Init())
	{
		return &entry;
	}

	return NULL;
}
