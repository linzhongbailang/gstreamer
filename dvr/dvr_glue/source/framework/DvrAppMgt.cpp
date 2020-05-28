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
#include <string.h>
#include "DVR_APP_DEF.h"
#include "DvrAppMgt.h"
#include <log/log.h>

typedef struct  
{
	APP_MGT_INST **AppsPool;
	int nAppMaxNum;
	int nAppNum;
	int nAppCur;
	int nAppS1Cur;
}APP_MGT_CTRL;

static APP_MGT_CTRL g_appMgtCtrl, *pAppMgt = NULL;

int DvrAppMgt_Init(APP_MGT_INST *sysAppPool[], int appMaxNum)
{
	memset(&g_appMgtCtrl, 0, sizeof(APP_MGT_CTRL));
	pAppMgt = &g_appMgtCtrl;

	pAppMgt->AppsPool = sysAppPool;
	pAppMgt->nAppMaxNum = appMaxNum;
	pAppMgt->nAppCur = 0;

	return DVR_RES_SOK;
}

int DvrAppMgt_Register(APP_MGT_INST *app)
{
	int AppId = -1;

	if (pAppMgt->nAppNum < pAppMgt->nAppMaxNum)
	{
		AppId = pAppMgt->nAppNum;
		app->Id = AppId;
		pAppMgt->AppsPool[AppId] = app;
		pAppMgt->nAppNum++;
	}
	else
	{
		Log_Error("[App - AppMgt] <Register> Fatal Error! No space for more apps\n");
	}

	return AppId;
}

int DvrAppMgt_GetApp(APP_MGT_INST **app, int appId)
{
	if ((appId < 0) || (appId >= pAppMgt->nAppNum)) {
		*app = NULL;
		return -1;
	}
	else {
		*app = pAppMgt->AppsPool[appId];
		return 0;
	}
}

int DvrAppMgt_GetCurApp(APP_MGT_INST **app)
{
	*app = pAppMgt->AppsPool[pAppMgt->nAppCur];
	return pAppMgt->nAppCur;
}

static int DvrAppMgt_Shrink(int appId)
{
	int ReturnValue = 0;
	int AppCurTmp = 0;

	if ((appId < 0) || (appId >= pAppMgt->nAppNum))
	{
		Log_Error("[App - AppMgt] <Shrink> Fatal Error! Invalid app Id: appId = %d / AppNum = %d\n", appId, pAppMgt->nAppNum);
		return -1;
	}

	while (pAppMgt->AppsPool[pAppMgt->nAppCur]->Tier > pAppMgt->AppsPool[appId]->Tier)
	{
		// Remove current app aflags
		APP_REMOVEFLAGS(pAppMgt->AppsPool[pAppMgt->nAppCur]->GFlags, APP_AFLAGS_START);
		APP_REMOVEFLAGS(pAppMgt->AppsPool[pAppMgt->nAppCur]->GFlags, APP_AFLAGS_READY);
		// back up current app Id
		AppCurTmp = pAppMgt->nAppCur;    // AppCurTmp is old app
		// current app Id move up
		pAppMgt->nAppCur = pAppMgt->AppsPool[AppCurTmp]->Parent;
		// reset current app Child app Id
		pAppMgt->AppsPool[pAppMgt->nAppCur]->Child = 0;
		// clear old current status
		pAppMgt->AppsPool[AppCurTmp]->Parent = 0;
		pAppMgt->AppsPool[AppCurTmp]->Previous = 0;
		// Stop old current app
		pAppMgt->AppsPool[AppCurTmp]->Stop();
		if (APP_CHECKFLAGS(pAppMgt->AppsPool[AppCurTmp]->GFlags, APP_AFLAGS_OVERLAP)) {
			ReturnValue = 1;
		}
	}

	return ReturnValue;
}

static int DvrAppMgt_Switch_To(int appId)
{
	int ReturnValue = 0;
	int AppCurTmp = 0;
	int AppParent = 0;

	if ((appId < 0) || (appId >= pAppMgt->nAppNum)) 
	{
		Log_Error("[App - AppMgt] <Shrink> Fatal Error! Invalid app Id: appId = %d / AppNum = %d\n", appId, pAppMgt->nAppNum);
		return -1;
	}

	if (pAppMgt->AppsPool[pAppMgt->nAppCur]->Tier == pAppMgt->AppsPool[appId]->Tier) {
		// Remove current app aflags
		APP_REMOVEFLAGS(pAppMgt->AppsPool[pAppMgt->nAppCur]->GFlags, APP_AFLAGS_START);
		APP_REMOVEFLAGS(pAppMgt->AppsPool[pAppMgt->nAppCur]->GFlags, APP_AFLAGS_READY);
		// Back up current app Id
		AppCurTmp = pAppMgt->nAppCur;  // AppCurTmp is old app
		// Back up Parent app Id
		AppParent = pAppMgt->AppsPool[pAppMgt->nAppCur]->Parent;
		// current app Id move to new app
		pAppMgt->nAppCur = appId;
		// set current Tier-1 app Id if needed
		if (pAppMgt->AppsPool[pAppMgt->nAppCur]->Tier == 1) {
			pAppMgt->nAppS1Cur = pAppMgt->nAppCur;
		}
		// Reset Parent app Child app Id
		pAppMgt->AppsPool[AppParent]->Child = pAppMgt->nAppCur;
		// Set current app status
		pAppMgt->AppsPool[pAppMgt->nAppCur]->Parent = AppParent;
		pAppMgt->AppsPool[pAppMgt->nAppCur]->Previous = AppCurTmp;
		// clear old current app status
		pAppMgt->AppsPool[AppCurTmp]->Parent = 0;
		pAppMgt->AppsPool[AppCurTmp]->Previous = 0;
		// Stop old current app
		pAppMgt->AppsPool[AppCurTmp]->Stop();
		// Stop Parent app if new current app Overlap but old current app doesn't
		if (!APP_CHECKFLAGS(pAppMgt->AppsPool[AppCurTmp]->GFlags, APP_AFLAGS_OVERLAP) &&
			APP_CHECKFLAGS(pAppMgt->AppsPool[pAppMgt->nAppCur]->GFlags, APP_AFLAGS_OVERLAP)) {
			pAppMgt->AppsPool[AppParent]->Stop();
			// Start Parent app if new current app doesn't Overlap but old current app does
		}
		else if (APP_CHECKFLAGS(pAppMgt->AppsPool[AppCurTmp]->GFlags, APP_AFLAGS_OVERLAP) &&
			!APP_CHECKFLAGS(pAppMgt->AppsPool[pAppMgt->nAppCur]->GFlags, APP_AFLAGS_OVERLAP)) {
			pAppMgt->AppsPool[AppParent]->Start();
		}
		// Start new current app
		pAppMgt->AppsPool[pAppMgt->nAppCur]->Start();
	}

	return ReturnValue;
}


int DvrAppMgt_SwitchApp(int appId)
{
	int ReturnValue = 0;
	int Overlap = 0;
	int AppCurTmp = 0;

	if (pAppMgt == NULL)
		return -1;

	if ((appId < 0) || (appId >= pAppMgt->nAppNum))
	{
		Log_Error("[App - AppMgt] <Shrink> Fatal Error! Invalid app Id: appId = %d / AppNum = %d\n", appId, pAppMgt->nAppNum);
		return -1;
	}

	if (appId == pAppMgt->nAppCur) {
		return ReturnValue;
	}

	if (pAppMgt->AppsPool[pAppMgt->nAppCur]->Tier < pAppMgt->AppsPool[appId]->Tier) {
		// back up current app Id
		AppCurTmp = pAppMgt->nAppCur;
		pAppMgt->nAppCur = appId;
		if (pAppMgt->AppsPool[pAppMgt->nAppCur]->Tier == 1) {
			pAppMgt->nAppS1Cur = pAppMgt->nAppCur;
		}
		// set old current Child app Id
		pAppMgt->AppsPool[AppCurTmp]->Child = pAppMgt->nAppCur;
		// set new current Parent app Id
		pAppMgt->AppsPool[pAppMgt->nAppCur]->Parent = AppCurTmp;
		// Stop old current app (Parent) if new current app Overlap
		if (APP_CHECKFLAGS(pAppMgt->AppsPool[pAppMgt->nAppCur]->GFlags, APP_AFLAGS_OVERLAP)) {
			pAppMgt->AppsPool[AppCurTmp]->Stop();
		}
		// Start new current app
		pAppMgt->AppsPool[pAppMgt->nAppCur]->Start();
	}
	else if (APP_CHECKFLAGS(pAppMgt->AppsPool[appId]->GFlags, APP_AFLAGS_START)) {
		/* the new app is the ancestor of current app */
		Overlap = DvrAppMgt_Shrink(appId);
		if (Overlap) {
			// restart new curren app since it was overlapped
			pAppMgt->AppsPool[pAppMgt->nAppCur]->Start();
		}
	}
	else if (!DvrAppMgt_CheckIdle()) {
        Log_Error("[App - AppMgt] <SwitchApp> Stop running application first!\n");
		return -1;
	}
	else if (pAppMgt->AppsPool[pAppMgt->nAppCur]->Tier == pAppMgt->AppsPool[appId]->Tier) {
		DvrAppMgt_Switch_To(appId);
	}
	else {
		Overlap = DvrAppMgt_Shrink(appId);
		DvrAppMgt_Switch_To(appId);
	}

	if (appId != pAppMgt->nAppCur) {
		Log_Error("[App - AppMgt] <SwitchApp> Fatal Error! App switching flow error\n");
		return -1;
	}

	return ReturnValue;
}

int DvrAppMgt_CheckBusy(void)
{
	int Busy = 0;
	APP_MGT_INST *App = pAppMgt->AppsPool[0];    //APP_MAIN

	while ((App->Child != 0) && (!Busy)) {
		App = pAppMgt->AppsPool[App->Child];
		if (APP_CHECKFLAGS(App->GFlags, APP_AFLAGS_BUSY)) {
			Busy = App->Id;
		}
	}

	return Busy;
}

int DvrAppMgt_CheckIo(void)
{
	int Io = 0;
	APP_MGT_INST *App = pAppMgt->AppsPool[0];    //APP_MAIN

	while ((App->Child != 0) && (!Io)) {
		App = pAppMgt->AppsPool[App->Child];
		if (APP_CHECKFLAGS(App->GFlags, APP_AFLAGS_IO)) {
			Io = App->Id;
		}
	}

	return Io;
}

int DvrAppMgt_CheckIdle(void)
{
	APP_MGT_INST *CurApp = pAppMgt->AppsPool[pAppMgt->nAppCur];
	return (APP_CHECKFLAGS(CurApp->GFlags, APP_AFLAGS_READY) &&
		(!DvrAppMgt_CheckBusy()) &&
		(!DvrAppMgt_CheckIo()));
}
