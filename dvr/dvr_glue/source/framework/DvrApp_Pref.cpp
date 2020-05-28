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
#include "DvrApp_Pref.h"

// Unit: Bytes
#define SYSTEM_PREF_SIZE    (512)
static DVR_U8 DefaultTable[SYSTEM_PREF_SIZE] = {
	0
};

static DVR_U8 SettingTable[SYSTEM_PREF_SIZE] = { 0 };
APP_PREF_USER *UserSetting = NULL;

int AppPref_InitPref(void)
{
	UserSetting = (APP_PREF_USER *)&SettingTable;
	return 0;
}

int AppPref_Save(void)
{
	return 0;
}