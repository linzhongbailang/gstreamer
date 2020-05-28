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
#include "flow/thumb/DvrThumbApp.h"
#include "flow/widget/DvrWidget_Dialog.h"
#include "flow/widget/DvrWidget_Mgt.h"
#include "flow/util/CThreadPool.h"
#include "flow/util/DvrMetaScan.h"
#include "flow/util/DvrSDCardSpeedTest.h"
#include "gui/Gui_DvrThumbApp.h"
#include "framework/DvrAppNotify.h"
#include <log/log.h>
#include <string>
#include <sys/stat.h>

#include <sys/time.h>

#include "Gpu_Dvr_Interface.h"

#ifdef __linux__
#include <unistd.h>
#endif

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

static void system_sync(void *arg)
{
#ifdef __linux__
	sync();
#endif	
	return;
}

static DVR_RESULT SendAddFileForPreview(MetaPreviewScanParam *pParam)
{
    DVR_RESULT res = DVR_RES_SOK;
    if (pParam == NULL)
        return DVR_RES_EPOINTER;

    DvrThumbApp *app = DvrThumbApp::Get();
    res = Dvr_Sdk_MetaData_GetFilePreview(&pParam->option, pParam->pPreviewBuf, &pParam->nPreviewBufSize);
    if (res != DVR_RES_SOK)
    {
        Log_Error("Dvr_Sdk_MetaData_GetFilePreview fail for file [%s]\n", pParam->option.filename);
        //Dvr_Sdk_FileMapDB_SetFailedItem(pParam->option.filename, app->pCurTab->eCurFolder);
        Dvr_Sdk_LoadThumbNail(pParam->option.filename,pParam->pPreviewBuf,pParam->nPreviewBufSize);
    }
    else
    {
        Log_Message("GetFilePreview done successfully for file [%s]\n", pParam->option.filename);
    }

    return res;
}

static DVR_RESULT thumb_app_init(void)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

    memset(app->ThumbTab, 0, sizeof(app->ThumbTab));

    app->ThumbTab[GUI_THUMB_TAB_LOOP_REC].eCurFolder = DVR_FOLDER_TYPE_NORMAL;
    app->ThumbTab[GUI_THUMB_TAB_LOOP_REC].eType = DVR_FILE_TYPE_VIDEO;
    Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_NORMAL_FOLDER, app->ThumbTab[GUI_THUMB_TAB_LOOP_REC].FolderName, APP_MAX_FN_SIZE, NULL);

    app->ThumbTab[GUI_THUMB_TAB_EVENT_REC].eCurFolder = DVR_FOLDER_TYPE_EMERGENCY;
    app->ThumbTab[GUI_THUMB_TAB_EVENT_REC].eType = DVR_FILE_TYPE_VIDEO;
    Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_EMERGENCY_FOLDER, app->ThumbTab[GUI_THUMB_TAB_EVENT_REC].FolderName, APP_MAX_FN_SIZE, NULL);

    app->ThumbTab[GUI_THUMB_TAB_DAS].eCurFolder = DVR_FOLDER_TYPE_DAS;
    app->ThumbTab[GUI_THUMB_TAB_DAS].eType = DVR_FILE_TYPE_VIDEO;
    Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_DAS_FOLDER, app->ThumbTab[GUI_THUMB_TAB_DAS].FolderName, APP_MAX_FN_SIZE, NULL);

    app->ThumbTab[GUI_THUMB_TAB_PHOTO].eCurFolder = DVR_FOLDER_TYPE_PHOTO;
    app->ThumbTab[GUI_THUMB_TAB_PHOTO].eType = DVR_FILE_TYPE_IMAGE;
    Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_PHOTO_FOLDER, app->ThumbTab[GUI_THUMB_TAB_PHOTO].FolderName, APP_MAX_FN_SIZE, NULL);

    app->TabCur = GUI_THUMB_TAB_LOOP_REC;
    app->PageCur = 0;
	app->pCurTab = &app->ThumbTab[GUI_THUMB_TAB_LOOP_REC];
    app->pCurPage = &app->pCurTab->ThumbPage[app->PageCur];

    app->bInEditMode = FALSE;
    app->MaxPageItemNum = NUM_THUMBNAIL_PER_PAGE;

    DVR_PREVIEW_OPTION option;
    option.ulPreviewWidth = DVR_THUMBNAIL_WIDTH;
    option.ulPreviewHeight = DVR_THUMBNAIL_HEIGHT;
    option.format = PREVIEW_RGB888;

    for (unsigned int i = 0; i < app->MaxPageItemNum; i++)
    {
        app->stPreview[i].color_format = option.format;
        app->stPreview[i].ulPreviewWidth = option.ulPreviewWidth;
        app->stPreview[i].ulPreviewHeight = option.ulPreviewHeight;
        app->stPreview[i].nPreviewBufSize = DVR_THUMBNAIL_WIDTH*DVR_THUMBNAIL_HEIGHT*3;
    }

    return res;
}

static DVR_RESULT thumb_app_start(void)
{
    DVR_RESULT res = DVR_RES_SOK;

#ifdef __linux__
	DVR_GUI_LAYOUT_INST_EXT RenderData;
	DvrGuiResHandler *handler;
	
	handler = (DvrGuiResHandler *)Dvr_App_GetGuiResHandler();
	if (handler != NULL)
	{
		RenderData.curLayout = GUI_LAYOUT_THUMB_EXT;
		handler->GetLayoutInfo(GUI_LAYOUT_THUMB, (DVR_GRAPHIC_UIOBJ **)&RenderData.pTable, &RenderData.ObjNum);				
		UpdateRenderDvrData((DVR_GUI_LAYOUT_INST_EXT*)&RenderData, sizeof(DVR_GUI_LAYOUT_INST_EXT));
		Log_Message("Set Thumb Layout!!!!!!!!!!!!!!!!!!!!!!!!");
	}
#endif
	//DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_ASYNC_STOP_RECORDER, NULL, NULL);
	//CThreadPool::Get()->AddTask(system_sync, NULL);

	DVR_U32 AVM_DVRModeFeedback = 0x2; //0:Inactive, 0x1:RealTimeMode, 0x2:ReplayMode, 0x3:SettingMode
	Log_Message("[%s:%d] feedback AVM_DVRModeFeedback = %d!!!!", __FUNCTION__, __LINE__, AVM_DVRModeFeedback);
	DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_MODE_UPDATE, (void *)&AVM_DVRModeFeedback, NULL);

    return res;
}

static DVR_RESULT thumb_app_stop(void)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

    for (unsigned int i = 0; i < app->MaxPageItemNum; i++)
    {
        app->pCurPage->item[i].bValid = 0;
        memset(&app->pCurPage->item[i].stFullName, 0, sizeof(app->pCurPage->item[i].stFullName));
        memset(app->pCurPage->item[i].displayName, 0, sizeof(app->pCurPage->item[i].displayName));
    }

    memset(&app->pCurPage->check_box, 0, sizeof(app->pCurPage->check_box));
	app->Func(THUMB_APP_FUNC_CLEAR_CUR_PAGE, 0, 0);

    /* Close the menu or dialog. */
    AppWidget_Off(DVR_WIDGET_ALL, 0);
    APP_REMOVEFLAGS(app->m_thumb.GFlags, APP_AFLAGS_POPUP);

    /*Hide GUI*/
    app->Gui(GUI_THUMB_APP_CMD_HIDE_ALL, 0, 0);
    app->Gui(GUI_THUMB_APP_CMD_FLUSH, 0, 0);

    return res;
}

static DVR_RESULT thumb_app_ready(void)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

    if (!APP_CHECKFLAGS(app->m_thumb.GFlags, APP_AFLAGS_READY)) {
        APP_ADDFLAGS(app->m_thumb.GFlags, APP_AFLAGS_READY);

        DvrAppUtil_ReadyCheck();
        if (!APP_CHECKFLAGS(app->m_thumb.GFlags, APP_AFLAGS_READY)) {
            /* The system could switch the current app to other in the function "AppUtil_ReadyCheck". */
            return res;
        }
    }

    if (app->m_thumb.Previous == Dvr_App_GetAppId(MDL_APP_RECORDER_ID))  
    {
        if (APP_CHECKFLAGS(app->m_thumb.GFlags, APP_AFLAGS_READY)) {
            app->Func(THUMB_APP_FUNC_RESET_DEFAULT_TAB, 0, 0);
            app->Func(THUMB_APP_FUNC_START_DISP_PAGE, TRUE, 0);		
        }		
    }
	else if(app->m_thumb.Previous == Dvr_App_GetAppId(MDL_APP_PLAYER_ID))
	{
        if (APP_CHECKFLAGS(app->m_thumb.GFlags, APP_AFLAGS_READY)) {
            app->Func(THUMB_APP_FUNC_RESET_DEFAULT_TAB, 0, 0);
            app->Func(THUMB_APP_FUNC_START_DISP_PAGE, FALSE, 0);
        }
	}
    else
    {
        if (APP_CHECKFLAGS(app->m_thumb.GFlags, APP_AFLAGS_READY)) {
            app->Func(THUMB_APP_FUNC_RESET_DEFAULT_TAB, 0, 0);
            app->Func(THUMB_APP_FUNC_START_DISP_PAGE, TRUE, 0);
        }
    }

    return res;
}


static DVR_RESULT thumb_app_init_file_info(void)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

    DVR_U32 nTotalFileNum = 0, nTotalPageNum = 0;

    res = Dvr_Sdk_FileMapDB_GetItemCountByType(&nTotalFileNum, app->pCurTab->eCurFolder);
    if (res != DVR_RES_SOK)
    {
        Log_Error("Directory [%s] is not existed in MemDB\n", app->pCurTab->FolderName);
		return res;
    }

    for (unsigned int page = 0; page < MAX_PAGE_NUM_IN_CUR_FOLDER; page++)
    {
    	for (unsigned int index = 0; index < app->MaxPageItemNum; index++)
    	{
            //if(app->pCurPage)
            //    app->pCurPage->item[index].stPreview.Reset();
        	app->pCurTab->ThumbPage[page].item[index].bValid = 0;
        	memset(&app->pCurTab->ThumbPage[page].item[index].stFullName, 0, sizeof(app->pCurTab->ThumbPage[page].item[index].stFullName));
        	memset(app->pCurTab->ThumbPage[page].item[index].displayName, 0, sizeof(app->pCurTab->ThumbPage[page].item[index].displayName));
    	}
    }

    if (nTotalFileNum > 0)
    {
        nTotalPageNum = nTotalFileNum / app->MaxPageItemNum;
        if (nTotalFileNum >= app->MaxPageItemNum)
        {
            for (unsigned int i = 0; i < nTotalPageNum; i++)
            {
                app->pCurTab->ThumbPage[i].PageItemNum = app->MaxPageItemNum;
            }

            if ((nTotalFileNum - (nTotalPageNum * app->MaxPageItemNum)) > 0)
            {
                app->pCurTab->ThumbPage[nTotalPageNum].PageItemNum = nTotalFileNum - (nTotalPageNum * app->MaxPageItemNum);
                nTotalPageNum++;
            }
        }
        else
        {
            nTotalPageNum = 1;
            app->pCurTab->ThumbPage[0].PageItemNum = nTotalFileNum;
        }

        if (nTotalPageNum > MAX_PAGE_NUM_IN_CUR_FOLDER)
        {
            Log_Error("ERROR! Page Number Exceed %d\n", nTotalPageNum);
            return DVR_RES_EFAIL;
        }

        for (unsigned int page = 0; page < nTotalPageNum; page++)
        {
            for (unsigned int index = 0; index < app->pCurTab->ThumbPage[page].PageItemNum; index++)
            {
                res = Dvr_Sdk_FileMapDB_GetNameByRelPos(page * app->MaxPageItemNum + index, app->pCurTab->eCurFolder,
                    &app->pCurTab->ThumbPage[page].item[index].stFullName, APP_MAX_FN_SIZE);
                if (res != DVR_RES_SOK)
                {
                    Log_Error("Dvr_Sdk_FileMapDB_GetNameByRelPos failed for RelPos [%d]\n", page * app->MaxPageItemNum + index);
                    app->pCurTab->ThumbPage[page].item[index].bValid = 0;
                }
                else
                {
                    app->pCurTab->ThumbPage[page].item[index].bValid = 1;
                }
                Dvr_Sdk_FormatDisplayName(app->pCurTab->ThumbPage[page].item[index].stFullName.szMediaFileName, app->pCurTab->ThumbPage[page].item[index].displayName, 0);
            }
        }
        app->Gui(GUI_THUMB_APP_CMD_WARNING_HIDE, 0, 0);
    }
    else
    {
        nTotalFileNum = 0;
        nTotalPageNum = 0;
        if (!APP_CHECKFLAGS(app->m_thumb.GFlags, APP_AFLAGS_POPUP)) {
            app->Func(THUMB_APP_FUNC_WARNING_MSG_SHOW_START, GUI_WARNING_NO_FILES, 1);
        }
        app->Gui(GUI_THUMB_APP_CMD_FLUSH, 0, 0);
    }
	
    app->pCurTab->TotalFileNum = nTotalFileNum;
    app->pCurTab->TotalPageNum = nTotalPageNum;

    return res;
}

static DVR_RESULT thumb_app_init_page_info(void)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

	res = Dvr_Sdk_StorageCard_CheckStatus();
    if (res != DVR_STORAGE_CARD_STATUS_NO_CARD && 
		res != DVR_STORAGE_CARD_STATUS_ERROR_FORMAT )
    {
        thumb_app_init_file_info();
    }
    else 
    {    
	    app->pCurTab->TotalFileNum = 0;
	    app->pCurTab->TotalPageNum = 0;
    }

    return res;
}

static DVR_RESULT thumb_app_start_disp_page(DVR_BOOL bGoToLatestPage)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();
		
	thumb_app_init_page_info();

	if(TRUE == bGoToLatestPage)
	{
		app->PageCur = (app->pCurTab->TotalPageNum == 0) ? 0 : (app->pCurTab->TotalPageNum - 1);
		app->pCurPage = &app->pCurTab->ThumbPage[app->PageCur];
	}
	else
	{
		if(app->pCurTab->TotalPageNum == 0)
		{
			app->PageCur = 0;
		}
		else if(app->PageCur >= app->pCurTab->TotalPageNum)
		{
			app->PageCur = app->pCurTab->TotalPageNum - 1;
		}
		else
		{
			//no change
		}
		
		app->pCurPage = &app->pCurTab->ThumbPage[app->PageCur];
	}
	
	if(app->pCurTab->TotalFileNum == 0)
	{
		memset(app->pCurPage, 0, sizeof(THUMBNAIL_PAGE));
	}
		
    app->Func(THUMB_APP_FUNC_SHOW_PAGE_SIDEINFO, 0, 0);
    app->Func(THUMB_APP_FUNC_SHOW_PAGE_CHECKBOX, 0, 0);

    if (app->pCurTab->TotalFileNum == 0)
    {
        app->Func(THUMB_APP_FUNC_WARNING_MSG_SHOW_START, GUI_WARNING_NO_FILES, 1);
    }

    app->Func(THUMB_APP_FUNC_UPDATE_PAGE_FRAME, 0, 0);

    return res;
}

static DVR_RESULT thumb_app_update_page_frame(void)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

    app->Func(THUMB_APP_FUNC_SHOW_PAGE_NUM, 0, 0);
    app->Func(THUMB_APP_FUNC_SHOW_PAGE_FRAME, 0, 0);

    DVR_U32 CurrentVideoCounts = app->pCurPage->PageItemNum;
    Log_Message("CurrentVideoCounts = %d", CurrentVideoCounts);
    DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_PAGE_ITEM_NUM_UPDATE, (void *)&CurrentVideoCounts, NULL);

    return res;
}

static DVR_RESULT thumb_app_get_prev_page(void)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

    app->PageCur -= 1;
    if (app->PageCur < 0)
    {
        //already in last page, do not need to update preview
        app->PageCur = 0;
    }

    app->pCurPage = &app->pCurTab->ThumbPage[app->PageCur];

    //app->Func(THUMB_APP_FUNC_CLEAR_CUR_PAGE, 0, 0);

    app->Func(THUMB_APP_FUNC_SHOW_PAGE_CHECKBOX, 0, 0);
    app->Func(THUMB_APP_FUNC_SHOW_PAGE_SIDEINFO, 0, 0);
    app->Func(THUMB_APP_FUNC_UPDATE_PAGE_FRAME, 0, 0);

    return res;
}

static DVR_RESULT thumb_app_get_next_page(void)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

    app->PageCur += 1;
    if (app->PageCur >= app->pCurTab->TotalPageNum)
    {
        //already in the first page, do not need to update preview
        app->PageCur = app->pCurTab->TotalPageNum - 1;
    }

    app->pCurPage = &app->pCurTab->ThumbPage[app->PageCur];

    //app->Func(THUMB_APP_FUNC_CLEAR_CUR_PAGE, 0, 0);

    app->Func(THUMB_APP_FUNC_SHOW_PAGE_CHECKBOX, 0, 0);
    app->Func(THUMB_APP_FUNC_SHOW_PAGE_SIDEINFO, 0, 0);
    app->Func(THUMB_APP_FUNC_UPDATE_PAGE_FRAME, 0, 0);

    return res;
}

static DVR_RESULT thumb_app_show_check_box_content()
{
    DVR_RESULT res = DVR_RES_SOK;
    GUI_OBJ_THUMB_EDIT_INST edit_sel;
    DvrThumbApp *app = DvrThumbApp::Get();

    if (app->bInEditMode == TRUE)
    {
        memset(&edit_sel, 0, sizeof(GUI_OBJ_THUMB_EDIT_INST));

        for (int i = 0; i < app->MaxPageItemNum; i++)
        {
            edit_sel.check_box[i] = app->pCurPage->check_box[i];
        }

        app->Gui(GUI_THUMB_APP_CMD_EDIT_CHECKBOX_UPDATE, (DVR_U32)&edit_sel, sizeof(GUI_OBJ_THUMB_EDIT_INST));
        app->Gui(GUI_THUMB_APP_CMD_FLUSH, 0, 0);
    }

    return res;
}

static DVR_RESULT thumb_app_update_tab(void)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();
	//0x0=no request,0x1=cycle mode browse,0x2=emergency mode browse,0x3=photo mode browse,0x4= DAS mode browse
	DVR_U32 ReplayMode;

    switch (app->TabCur)
    {
    case GUI_THUMB_TAB_LOOP_REC:
		ReplayMode = 1;
        break;

    case GUI_THUMB_TAB_EVENT_REC:
		ReplayMode = 2;
        break;

    case GUI_THUMB_TAB_PHOTO:
		ReplayMode = 3;
        break;

	case GUI_THUMB_TAB_DAS:
		ReplayMode = 4;
		break;
    }
    app->Gui(GUI_THUMB_APP_CMD_TAB_UPDATE, app->TabCur, 4);
    app->Gui(GUI_THUMB_APP_CMD_FLUSH, 0, 0);

    Log_Message("[%s:%d] feedback replaymode = %d", __FUNCTION__, __LINE__, ReplayMode);
    DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_REPLAY_MODE, (void *)&ReplayMode, NULL);

    return res;
}

static DVR_RESULT thumb_app_update_capacity(void)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

	DVR_STORAGE_QUOTA nStorageQuota;	
    DVR_S32 status;
	DVR_U32 nUsedSpace_KB = 0;
	DVR_U32 nUsedPercent = 0;	
	Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_CARD_QUOTA, &nStorageQuota, sizeof(DVR_STORAGE_QUOTA), &status);

	switch(app->TabCur)
	{
		case GUI_THUMB_TAB_LOOP_REC:
		{
        	DVR_U32 nEventUsedSpace = 0;
            DVR_U32 nPhotoUsedSpace = 0;
            DVR_U32 nDasUsedSpace = 0;
            DVR_U32 nCardAvailableSpace = 0;
			Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_NORMAL_FOLDER_USEDSPACE, &nUsedSpace_KB, sizeof(DVR_U32), &status);
            Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_EVENT_FOLDER_USEDSPACE, &nEventUsedSpace, sizeof(DVR_U32), NULL);
            Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_PHOTO_FOLDER_USEDSPACE, &nPhotoUsedSpace, sizeof(DVR_U32), NULL);
            Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_DAS_FOLDER_USEDSPACE, &nDasUsedSpace, sizeof(DVR_U32), NULL);
            Dvr_Sdk_MonitorStorage_Get(DVR_MONITOR_STORAGE_PROP_CARD_AVAILABLE_SPACE, &nCardAvailableSpace, sizeof(DVR_U32), NULL);
            DVR_U64 nDividend = 100 * (DVR_U64)nUsedSpace_KB;
            DVR_U32 nNormalAvailSpace = nCardAvailableSpace - nEventUsedSpace - nPhotoUsedSpace - nDasUsedSpace;
			nUsedPercent = (nUsedSpace_KB > 4) ? (1 + (nDividend / nNormalAvailSpace)) : 0;
		}
		break;

		case GUI_THUMB_TAB_EVENT_REC:
		{
			Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_EVENT_FOLDER_USEDSPACE, &nUsedSpace_KB, sizeof(DVR_U32), &status);
			nUsedPercent = (nUsedSpace_KB > 4) ? (1 + (100 * nUsedSpace_KB / nStorageQuota.nEventStorageQuota)) : 0;
		}
		break;

		case GUI_THUMB_TAB_PHOTO:
		{
			Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_PHOTO_FOLDER_USEDSPACE, &nUsedSpace_KB, sizeof(DVR_U32), &status);
			nUsedPercent = (nUsedSpace_KB > 4) ? (1 + (100 * nUsedSpace_KB / nStorageQuota.nPhotoStorageQuota)) : 0;
		}
		break;

		case GUI_THUMB_TAB_DAS:
		{
			Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_DAS_FOLDER_USEDSPACE, &nUsedSpace_KB, sizeof(DVR_U32), &status);
			nUsedPercent = (nUsedSpace_KB > 4) ? (1 + (100 * nUsedSpace_KB / nStorageQuota.nDasStorageQuota)) : 0;
		}
		break;

		default:
			break;
	}

	if(nUsedPercent >= 100)
	{
		nUsedPercent = 100;
	}

    res = Dvr_Sdk_StorageCard_CheckStatus();
    if (res == DVR_STORAGE_CARD_STATUS_NO_CARD)
    {
        nUsedPercent = 100;
    }
	
	DvrAppNotify *pHandle = DvrAppNotify::Get();
	if (pHandle != NULL)
		pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_STORAGE_PERCENT, (void *)&nUsedPercent, NULL);

#if 0
    GUI_OBJ_THUMB_CAPCITY_INST capacity;
    memset(&capacity, 0, sizeof(GUI_OBJ_THUMB_CAPCITY_INST));

    DVR_U32 nTotal_KB, nUsed_KB;
    Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_CARD_TOTALSPACE, &nTotal_KB, sizeof(DVR_U32), NULL);
    Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_CARD_USEDSPACE, &nUsed_KB, sizeof(DVR_U32), NULL);

    capacity.nTotalSpace = (float)nTotal_KB / 1024.0f / 1024.0f;
    capacity.nUsedSpace = (float)nUsed_KB / 1024.0f / 1024.0f;
    app->Gui(GUI_THUMB_APP_CMD_STORAGE_CAPACITY_UPDATE, (DVR_U32)&capacity, sizeof(GUI_OBJ_THUMB_CAPCITY_INST));
    app->Gui(GUI_THUMB_APP_CMD_FLUSH, 0, 0);
#endif

    return res;
}

static DVR_RESULT thumb_app_update_card_status()
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

    int CardParam = 0;
    res = Dvr_Sdk_StorageCard_CheckStatus();
    if (res == DVR_STORAGE_CARD_STATUS_NO_CARD)
    {
        CardParam = GUI_CARD_STATE_NO_CARD;
    }
    else{
        CardParam = GUI_CARD_STATE_READY;
    }

    app->Gui(GUI_THUMB_APP_CMD_CARD_UPDATE, CardParam, 4);
    app->Gui(GUI_THUMB_APP_CMD_CARD_SHOW, 0, 0);

    app->Gui(GUI_THUMB_APP_CMD_FLUSH, 0, 0);

    return res;
}

static DVR_RESULT thumb_app_update_page_num()
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();
    DVR_GUI_LAYOUT_INST_EXT RenderData;

    GUI_OBJ_THUMB_PAGENUM_INST pageNum;

    if (app->pCurTab->TotalPageNum == 0)
    {
        pageNum.nCurPage = 0;
        pageNum.nTotalPage = 0;
    }
    else
    {
        pageNum.nCurPage = app->PageCur + 1;
		//pageNum.nCurPage = app->pCurTab->TotalPageNum - app->PageCur;
        pageNum.nTotalPage = app->pCurTab->TotalPageNum;
    }

	Log_Message("!!!!!pageNum.nCurPage = %d, pageNum.nTotalPage = %d\n", pageNum.nCurPage, pageNum.nTotalPage);

    app->Gui(GUI_THUMB_APP_CMD_PAGENUM_UPDATE, (DVR_U32)&pageNum, sizeof(GUI_OBJ_THUMB_PAGENUM_INST));
    app->Gui(GUI_THUMB_APP_CMD_FLUSH, 0, 0);

    
    RenderData.curLayout = GUI_LAYOUT_THUMB_EXT;
        DvrGui_GetLayoutInfo(GUI_LAYOUT_THUMB, (DVR_GRAPHIC_UIOBJ **)&RenderData.pTable, &RenderData.ObjNum);           
        UpdateRenderDvrData((DVR_GUI_LAYOUT_INST_EXT*)&RenderData, sizeof(DVR_GUI_LAYOUT_INST_EXT));

        
    return res;
}

static DVR_RESULT thumb_app_start_show_sideinfo(void)
{
    DVR_RESULT res = DVR_RES_SOK;

    thumb_app_update_capacity();
    thumb_app_update_tab();
    //thumb_app_update_card_status();
    //thumb_app_update_page_num();

    return res;
}

static DVR_RESULT thumb_app_show_page_frame(void)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

    if (app->pCurTab->TotalFileNum > 0)
    {
        GUI_OBJ_THUMB_FRAME_INST frame;
        memset(&frame, 0, sizeof(GUI_OBJ_THUMB_FRAME_INST));

        for (unsigned int i = 0; i < app->MaxPageItemNum; i++)
        {
            frame.item[i].bValid = app->pCurPage->item[i].bValid;
            memcpy(frame.item[i].filename, app->pCurPage->item[i].displayName, sizeof(app->pCurPage->item[i].displayName));

            frame.item[i].color_format = app->stPreview[i].color_format;
            frame.item[i].ulPreviewWidth = app->stPreview[i].ulPreviewWidth;
            frame.item[i].ulPreviewHeight = app->stPreview[i].ulPreviewHeight;
            frame.item[i].pPreviewBuf = app->stPreview[i].PreviewBuf;
			
            MetaPreviewScanParam stParam;
            memset(&stParam, 0, sizeof(stParam));
            
            stParam.nPreviewBufSize = app->stPreview[i].nPreviewBufSize;
            stParam.pPreviewBuf = app->stPreview[i].PreviewBuf;
            strcpy(stParam.option.filename, app->pCurPage->item[i].stFullName.szMediaFileName);
            stParam.option.format = (DVR_PREVIEW_FORMAT)app->stPreview[i].color_format;
            stParam.option.ulPreviewHeight = app->stPreview[i].ulPreviewHeight;
            stParam.option.ulPreviewWidth = app->stPreview[i].ulPreviewWidth;

			memset(app->stPreview[i].PreviewBuf, 0, sizeof(app->stPreview[i].PreviewBuf));
			if(frame.item[i].bValid)
			{
                SendAddFileForPreview(&stParam);
			    /*if(app->ThumbTab[app->TabCur].eType == DVR_FILE_TYPE_IMAGE)
                {
                    Dvr_Sdk_LoadThumbNail(app->pCurPage->item[i].stFullName.szMediaFileName, 
                        app->stPreview[i].PreviewBuf,
                        sizeof(app->stPreview[i].PreviewBuf));
                }
                else
                {
                    SendAddFileForPreview(&stParam);
                }*/

			}
        }

        app->Gui(GUI_THUMB_APP_CMD_FRAME_UPDATE, (DVR_U32)&frame, sizeof(GUI_OBJ_THUMB_FRAME_INST));
    }
    else
    {
        GUI_OBJ_THUMB_FRAME_INST frame;
        memset(&frame, 0, sizeof(GUI_OBJ_THUMB_FRAME_INST));
        app->Gui(GUI_THUMB_APP_CMD_FRAME_UPDATE, (DVR_U32)&frame, sizeof(GUI_OBJ_THUMB_FRAME_INST));
    }

    app->Gui(GUI_THUMB_APP_CMD_FLUSH, 0, 0);

    return res;
}

static DVR_RESULT thumb_app_shift_tab(int param)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

    if (param >= GUI_THUMB_TAB_NUM)
        return DVR_RES_EFAIL;

    if (app->TabCur == param) //already in current tab
        return res;
    
    app->TabCur = param;
	app->pCurTab = &app->ThumbTab[app->TabCur];
    app->PageCur = (app->pCurTab->TotalPageNum == 0) ? 0 : (app->pCurTab->TotalPageNum - 1);
	app->pCurPage = &app->pCurTab->ThumbPage[app->PageCur];

    app->Func(THUMB_APP_FUNC_START_DISP_PAGE, TRUE, 0);

    // if EditMode on when shift tab, flush EditMode.
    if (app->bInEditMode == TRUE)
        app->Func(THUMB_APP_FUNC_EDIT_ENTER, 0, 0);

    return res;
}

static DVR_RESULT thumb_app_reset_defaut_tab()
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

    thumb_app_update_tab();

    return res;
}

static DVR_RESULT thumb_app_clear_cur_page()
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

    GUI_OBJ_THUMB_FRAME_INST frame;
    memset(&frame, 0, sizeof(GUI_OBJ_THUMB_FRAME_INST));	
    app->Gui(GUI_THUMB_APP_CMD_FRAME_UPDATE, (DVR_U32)&frame, sizeof(GUI_OBJ_THUMB_FRAME_INST));

    GUI_OBJ_THUMB_PAGENUM_INST pageNum;
	memset(&pageNum, 0, sizeof(GUI_OBJ_THUMB_PAGENUM_INST));
    app->Gui(GUI_THUMB_APP_CMD_PAGENUM_UPDATE, (DVR_U32)&pageNum, sizeof(GUI_OBJ_THUMB_PAGENUM_INST));

    app->Gui(GUI_THUMB_APP_CMD_FLUSH, 0, 0);

    return res;
}

static DVR_RESULT thumb_app_select_to_play(DVR_U32 index)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

    if (index >= app->MaxPageItemNum)
        return DVR_RES_EFAIL;

    APP_STATUS *pStatus = (APP_STATUS *)Dvr_App_GetStatus();
    memset(pStatus->selFileName, 0, sizeof(pStatus->selFileName));

	DVR_U32 ReplayMode;

    switch (app->pCurTab->eCurFolder)
    {
    case DVR_FOLDER_TYPE_NORMAL:
    {
        pStatus->selFileType = DVR_FILE_TYPE_VIDEO;
		pStatus->selFolderType = DVR_FOLDER_TYPE_NORMAL;
        memcpy(pStatus->selFileName,
            app->pCurPage->item[index].stFullName.szMediaFileName,
            strlen(app->pCurPage->item[index].stFullName.szMediaFileName));

		ReplayMode = 0x8;
    }
	break;
	
    case DVR_FOLDER_TYPE_EMERGENCY:
    {
        pStatus->selFileType = DVR_FILE_TYPE_VIDEO;
		pStatus->selFolderType = DVR_FOLDER_TYPE_EMERGENCY;
        memcpy(pStatus->selFileName,
            app->pCurPage->item[index].stFullName.szMediaFileName,
            strlen(app->pCurPage->item[index].stFullName.szMediaFileName));

		ReplayMode = 0x9;			
    }
    break;

    case DVR_FOLDER_TYPE_PHOTO:
    {
        pStatus->selFileType = DVR_FILE_TYPE_IMAGE;
		pStatus->selFolderType = DVR_FOLDER_TYPE_PHOTO;
        memcpy(pStatus->selFileName,
            app->pCurPage->item[index].stFullName.szMediaFileName,
            strlen(app->pCurPage->item[index].stFullName.szMediaFileName));

		ReplayMode = 0xA;			
    }
    break;

	case DVR_FOLDER_TYPE_DAS:
	{
        pStatus->selFileType = DVR_FILE_TYPE_VIDEO;
		pStatus->selFolderType = DVR_FOLDER_TYPE_DAS;
        memcpy(pStatus->selFileName,
            app->pCurPage->item[index].stFullName.szMediaFileName,
            strlen(app->pCurPage->item[index].stFullName.szMediaFileName));

		ReplayMode = 0xB;	
	}
	break;

    default:
        return DVR_RES_EFAIL;
    }

	DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_REPLAY_MODE, (void *)&ReplayMode, NULL); 	

	DVR_U32 DVREnableSetStatus = 1;//0x0=invalid,0x1=off,0x2=on,0x3=especially
	DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_RECORDER_SWTICH, (void *)&DVREnableSetStatus, NULL);

    res = DvrAppUtil_SwitchApp(Dvr_App_GetAppId(MDL_APP_PLAYER_ID));

    return res;
}

static DVR_RESULT thumb_app_enter_edit_mode(void)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

    for (int page = 0; page < MAX_PAGE_NUM_IN_CUR_FOLDER; page++)
    {
        for (int item = 0; item < app->MaxPageItemNum; item++)
        {
            app->pCurTab->ThumbPage[page].check_box[item] = 0;
        }
    }
    app->nFilesHasBeenSelected = 0;
    app->bInEditMode = TRUE;

    GUI_OBJ_THUMB_EDIT_INST edit_sel;
    memset(&edit_sel, 0, sizeof(GUI_OBJ_THUMB_EDIT_INST));

    GUI_THUMB_EDIT_STATE edit_state = GUI_THUMB_EDIT_STATE_NORMAL_ON;
    switch (app->pCurTab->eCurFolder)
    {
    case DVR_FOLDER_TYPE_NORMAL:
        edit_state = GUI_THUMB_EDIT_STATE_NORMAL_ON;
        break;
    case DVR_FOLDER_TYPE_EMERGENCY:
        edit_state = GUI_THUMB_EDIT_STATE_EMERGENCY_ON;
        break;
    case DVR_FOLDER_TYPE_PHOTO:
        edit_state = GUI_THUMB_EDIT_STATE_PHOTO_ON;
        break;
    case DVR_FOLDER_TYPE_DAS:
        edit_state = GUI_THUMB_EDIT_STATE_DAS_ON;
        break;
    default:
        break;
    }
    app->Gui(GUI_THUMB_APP_CMD_EDIT_UPDATE, edit_state, 4);

    app->Gui(GUI_THUMB_APP_CMD_EDIT_CHECKBOX_UPDATE, (DVR_U32)&edit_sel, sizeof(GUI_OBJ_THUMB_EDIT_INST));
    app->Gui(GUI_THUMB_APP_CMD_EDIT_CHECKBOX_SHOW, 0, 0);
    app->Gui(GUI_THUMB_APP_CMD_FLUSH, 0, 0);

    return res;
}

static DVR_RESULT thumb_app_enter_exit_mode(void)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

    app->bInEditMode = FALSE;

    GUI_OBJ_THUMB_EDIT_INST edit_sel;
    memset(&edit_sel, 0, sizeof(GUI_OBJ_THUMB_EDIT_INST));

    app->Gui(GUI_THUMB_APP_CMD_EDIT_CHECKBOX_UPDATE, (DVR_U32)&edit_sel, sizeof(GUI_OBJ_THUMB_EDIT_INST));
    app->Gui(GUI_THUMB_APP_CMD_EDIT_UPDATE, GUI_THUMB_EDIT_STATE_OFF, 4);
    app->Gui(GUI_THUMB_APP_CMD_EDIT_CHECKBOX_HIDE, 0, 0);
    app->Gui(GUI_THUMB_APP_CMD_FLUSH, 0, 0);

    return res;
}

static DVR_RESULT thumb_app_format_save_file_fullname(char *srcFn, char *dstFolder, char *dstFn)
{
    char *szSrcFullName = srcFn;
    char *szDstFolder = dstFolder;
    char *slash;

    std::string dst_file_location;
    char *szSrcFileName = (slash = strrchr(szSrcFullName, '/')) ? (slash + 1) : szSrcFullName;

    dst_file_location = szDstFolder;
    dst_file_location = dst_file_location + szSrcFileName;

    strcpy(dstFn, dst_file_location.data());

    return DVR_RES_SOK;
}

static void thumb_app_save_result_feedback_success(void *arg)
{
	DVR_U32 DVR_NormalToEmergency = 3;//save sucess
    DvrAppNotify *pHandle = DvrAppNotify::Get();

    if (pHandle != NULL)
		pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_NORMAL_TO_EMERGENCY_STATUS, (void *)&DVR_NormalToEmergency, NULL);

#ifdef __linux__
		usleep(500*1000); //wait 500ms
#endif

	DVR_NormalToEmergency = 0;
	if (pHandle != NULL)
		pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_NORMAL_TO_EMERGENCY_STATUS, (void *)&DVR_NormalToEmergency, NULL);
}

static void thumb_app_save_result_feedback_failed(void *arg)
{
	DVR_U32 DVR_NormalToEmergency = 4;//save failed
    DvrAppNotify *pHandle = DvrAppNotify::Get();

    if (pHandle != NULL)
		pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_NORMAL_TO_EMERGENCY_STATUS, (void *)&DVR_NormalToEmergency, NULL);

#ifdef __linux__
		usleep(500*1000); //wait 500ms
#endif

	DVR_NormalToEmergency = 0;
	if (pHandle != NULL)
		pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_NORMAL_TO_EMERGENCY_STATUS, (void *)&DVR_NormalToEmergency, NULL);
}

static DVR_RESULT thumb_app_check_event_folder(char pszFullName[NUM_THUMBNAIL_PER_PAGE][APP_MAX_FN_SIZE], DVR_U32 count)
{
    DVR_RESULT res = DVR_RES_SOK;
    struct stat buf;
    DVR_U32 size = 0;

    if(pszFullName == NULL)
        return DVR_RES_EFAIL;

#ifdef __linux__
    for(int i = 0;i < count;i++)
    {
        if(stat(pszFullName[i], &buf) < 0)
        {
            Log_Error("thumb_app_check_event_folder id %d (%s)\n", i, pszFullName[i]);
			CThreadPool::Get()->AddTask(thumb_app_save_result_feedback_failed, NULL);
            return DVR_RES_EFAIL;
        }
        size += (DVR_U32)(buf.st_size >> 10);
    }
#endif

    DVR_U32 nUsedSpace = 0;
    DVR_S32 status;
	DVR_STORAGE_QUOTA nStorageQuota;
    Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_EVENT_FOLDER_USEDSPACE, &nUsedSpace, sizeof(DVR_U32), &status);
	Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_CARD_QUOTA, &nStorageQuota, sizeof(DVR_STORAGE_QUOTA), &status);
	DVR_U32 nFreeSpace = nStorageQuota.nEventStorageQuota - nUsedSpace - size;
    if ((nStorageQuota.nEventStorageQuota < nUsedSpace)
        || (nStorageQuota.nEventStorageQuota - nUsedSpace) < size 
        || nFreeSpace < DVR_STORAGE_EVENT_FOLDER_WARNING_THRESHOLD)
    {
        //storage too small, notify
        DVR_U32 DVR_SDcardStatus = DVR_SDCARD_FULL_STATUS_EVENT_SPACE_FULL; //emergency folder full
        DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_FULL_STATUS_UPDATE, (void *)&DVR_SDcardStatus, NULL);
		CThreadPool::Get()->AddTask(thumb_app_save_result_feedback_failed, NULL);
        res = DVR_RES_EFAIL;
    }

    return res;
}

static DVR_RESULT thumb_app_save_selected_file(void)
{
	DVR_U32 DVR_NormalToEmergency;
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();
    DvrAppNotify *pHandle = DvrAppNotify::Get();
	DVR_DEVICE CurDrive;

	memset(&CurDrive, 0, sizeof(DVR_DEVICE));
	Dvr_Sdk_GetActiveDrive(&CurDrive);
	if (!strcmp(CurDrive.szMountPoint, ""))
		return DVR_RES_EFAIL;

    if (app->pCurTab->eCurFolder != DVR_FOLDER_TYPE_NORMAL) //only active for normal recorded file
        return res;

	DVR_U32 filecount = 0;
    char file_name[NUM_THUMBNAIL_PER_PAGE][APP_MAX_FN_SIZE];

    memset(&file_name, 0, sizeof(file_name));
	for (int idx = 0; idx < app->MaxPageItemNum; idx++)	
    {
        if (app->pCurPage->check_box[idx] == 1)
        {
			strncpy(file_name[filecount], app->pCurPage->item[idx].stFullName.szMediaFileName, APP_MAX_FN_SIZE);
			filecount++;
        }
    }

	if(filecount == 0)
	{
		Log_Error("no file is selected!!!!");
		CThreadPool::Get()->AddTask(thumb_app_save_result_feedback_failed, NULL);
		return DVR_RES_EFAIL;
	}
	
    if(DVR_RES_SOK != thumb_app_check_event_folder(file_name, filecount))
		return DVR_RES_EFAIL;

    char folder_name[APP_MAX_FN_SIZE];
    memset(folder_name, 0, sizeof(folder_name));
    Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_EMERGENCY_FOLDER, folder_name, sizeof(folder_name), NULL);

	DVR_NormalToEmergency = 2;//saving
    if (pHandle != NULL)
        pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_NORMAL_TO_EMERGENCY_STATUS, (void *)&DVR_NormalToEmergency, NULL);

    APP_ADDFLAGS(app->m_thumb.GFlags, APP_AFLAGS_BUSY);
    app->Func(THUMB_APP_FUNC_WARNING_MSG_SHOW_START, GUI_WARNING_PROCESSING, 1);

	for (int idx = 0; idx < app->MaxPageItemNum; idx++)	
    {
        if (app->pCurPage->check_box[idx] == 1)
        {
            DVR_DB_IDXFILE idxfile;
        	if(DVR_RES_SOK == Dvr_Sdk_FileMapDB_GetFileInfo(CurDrive.szMountPoint, app->pCurPage->item[idx].stFullName.szMediaFileName, app->pCurTab->eCurFolder, &idxfile))
            {
				DVR_FILEMAP_META_ITEM stEventItem;
				memset(&stEventItem, 0, sizeof(DVR_FILEMAP_META_ITEM));
				
				thumb_app_format_save_file_fullname(app->pCurPage->item[idx].stFullName.szMediaFileName, folder_name, stEventItem.szMediaFileName);
       	
	        	res |= Dvr_Sdk_ComSvcSyncOp_FileMove(app->pCurPage->item[idx].stFullName.szMediaFileName, stEventItem.szMediaFileName);
				if(DVR_SUCCEEDED(res))
				{
					Dvr_Sdk_FileMapDB_AddItem(&stEventItem, DVR_FOLDER_TYPE_EMERGENCY);
					Dvr_Sdk_FileMapDB_DelItem(app->pCurPage->item[idx].stFullName.szMediaFileName, app->pCurTab->eCurFolder);

					Log_Message("copy file %s to %s complete, res = 0x%x\n", app->pCurPage->item[idx].stFullName.szMediaFileName, folder_name, res);
				}
        	}
			else
			{
				Log_Message("[%d]Dvr_Sdk_FileMapDB_GetFileInfo for file[%s] return failed", __LINE__, app->pCurPage->item[idx].stFullName.szMediaFileName);
				res = DVR_RES_EFAIL;
			}
        }
        app->pCurPage->check_box[idx] = 0;
    }

    APP_REMOVEFLAGS(app->m_thumb.GFlags, APP_AFLAGS_BUSY);
    app->Func(THUMB_APP_FUNC_WARNING_MSG_SHOW_STOP, GUI_WARNING_PROCESSING, 0);

	if(DVR_SUCCEEDED(res))
	{
		DVR_NormalToEmergency = 3;//save complete
	}
	else
	{
		DVR_NormalToEmergency = 4;//save failed
	}

#ifdef __linux__
	usleep(300*1000);
#endif	

    if (pHandle != NULL)
        pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_NORMAL_TO_EMERGENCY_STATUS, (void *)&DVR_NormalToEmergency, NULL);

#ifdef __linux__
    usleep(300*1000);
#endif	

    DVR_NormalToEmergency = 0;
    if (pHandle != NULL)
        pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_NORMAL_TO_EMERGENCY_STATUS, (void *)&DVR_NormalToEmergency, NULL);

    app->Func(THUMB_APP_FUNC_START_DISP_PAGE, FALSE, 0);

	CThreadPool::Get()->AddTask(system_sync, NULL);	

    return res;
}

static DVR_RESULT thumb_app_delete_selected_files(void)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

	app->Func(THUMB_APP_FUNC_DELETE_FILE, 0, 0);

    return res;
}

static DVR_RESULT thumb_app_select_files(DVR_U32 param)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

    if (param > 0xFF)
        return DVR_RES_EFAIL;

	app->nFilesHasBeenSelected = 0;
	for (int idx = 0; idx < app->MaxPageItemNum; idx++)
	{
		app->pCurPage->check_box[idx] = 0;
	}

    for (int idx = 0; idx < app->pCurPage->PageItemNum; idx++)
    {
    	if((param >> idx) & 0x1)
    	{
        	app->pCurPage->check_box[idx] = 1;
        	app->nFilesHasBeenSelected += 1;
			
    		Log_Message("[%s:%d]Select All Files [idx = %d][total = %d]", __FUNCTION__, __LINE__, idx, app->nFilesHasBeenSelected);
    	}
    }

    app->Func(THUMB_APP_FUNC_SHOW_PAGE_CHECKBOX, 0, 0);

    return res;
}

static DVR_RESULT thumb_app_select_all_files()
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

    app->nFilesHasBeenSelected = 0;

    for (unsigned int page = 0; page < app->pCurTab->TotalPageNum; page++)
    {
        for (int idx = 0; idx < app->pCurTab->ThumbPage[page].PageItemNum; idx++)
        {
            app->pCurTab->ThumbPage[page].check_box[idx] = 1;
            app->nFilesHasBeenSelected += 1;
        }
    }
	
	Log_Message("[%s:%d]Select All Files [%d]", __FUNCTION__, __LINE__, app->nFilesHasBeenSelected);

    app->Func(THUMB_APP_FUNC_SHOW_PAGE_CHECKBOX, 0, 0);

    return res;
}

static DVR_RESULT thumb_app_dialog_del_handler(DVR_U32 sel, DVR_U32 param1, DVR_U32 param2)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

    switch (sel)
    {
    case DIALOG_SEL_YES:
        app->Func(THUMB_APP_FUNC_DELETE_FILE, 0, 0);
        break;

    case DIALOG_SEL_NO:
    default:
        break;
    }
    return res;
}

static DVR_RESULT thumb_app_delete_file_dialog_show(void)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

    AppDialog_SetDialog(GUI_DIALOG_TYPE_Y_N, DIALOG_SUBJECT_DEL, thumb_app_dialog_del_handler);
    AppWidget_On(DVR_WIDGET_DIALOG, 0);
    APP_ADDFLAGS(app->m_thumb.GFlags, APP_AFLAGS_POPUP);

    return res;
}

static DVR_RESULT thumb_app_delete_file(void)
{
	DVR_U32 AVM_DVRDeleteStatus;
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();
    DvrAppNotify *pHandle = DvrAppNotify::Get();
	DVR_DEVICE CurDrive;

	memset(&CurDrive, 0, sizeof(DVR_DEVICE));
	Dvr_Sdk_GetActiveDrive(&CurDrive);
	if (!strcmp(CurDrive.szMountPoint, ""))
		return DVR_RES_EFAIL;

    APP_ADDFLAGS(app->m_thumb.GFlags, APP_AFLAGS_BUSY);

    app->Func(THUMB_APP_FUNC_WARNING_MSG_SHOW_START, GUI_WARNING_PROCESSING, 1);

	AVM_DVRDeleteStatus = 1;//deleting
    if (pHandle != NULL)
        pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_DELETE_STATUS, (void *)&AVM_DVRDeleteStatus, NULL);

    for (int idx = 0; idx < app->MaxPageItemNum; idx++)
    {
        //Log_Message("[%s:%d]page:%d, idx:%d, checkbox = %d", __FUNCTION__, __LINE__, page, idx, app->pCurTab->ThumbPage[page].check_box[idx]);
        if (app->pCurPage->check_box[idx] == 1)
        {
            DVR_DB_IDXFILE idxfile;
        	if(DVR_RES_SOK == Dvr_Sdk_FileMapDB_GetFileInfo(CurDrive.szMountPoint, app->pCurPage->item[idx].stFullName.szMediaFileName, app->pCurTab->eCurFolder, &idxfile))
    		{
	            res |= Dvr_Sdk_ComSvcSyncOp_FileDel(app->pCurPage->item[idx].stFullName.szMediaFileName);
				if(DVR_SUCCEEDED(res))
				{
					Dvr_Sdk_FileMapDB_DelItem(app->pCurPage->item[idx].stFullName.szMediaFileName, app->pCurTab->eCurFolder);
	                Log_Message("thumb_app_delete_file: delete file [%s] successfully, res = 0x%x\n", app->pCurPage->item[idx].stFullName.szMediaFileName, res);
				}
				else
				{
	                Log_Message("thumb_app_delete_file: delete file [%s] failed, res = 0x%x\n", app->pCurPage->item[idx].stFullName.szMediaFileName, res);
				}
        	}
			else
			{
				Log_Message("[%d]Dvr_Sdk_FileMapDB_GetFileInfo for file[%s] return failed", __LINE__, app->pCurPage->item[idx].stFullName.szMediaFileName);
				res = DVR_RES_EFAIL;
			}
        }
        app->pCurPage->check_box[idx] = 0;
    }

    APP_REMOVEFLAGS(app->m_thumb.GFlags, APP_AFLAGS_BUSY);
    app->Func(THUMB_APP_FUNC_WARNING_MSG_SHOW_STOP, GUI_WARNING_PROCESSING, 0);

	if(DVR_SUCCEEDED(res))
	{
		AVM_DVRDeleteStatus = 2;//delete complete
	}
	else
	{
		AVM_DVRDeleteStatus = 3;//delete failed
	}

#ifdef __linux__
	usleep(300*1000);
#endif	
	
	if (pHandle != NULL)
		pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_DELETE_STATUS, (void *)&AVM_DVRDeleteStatus, NULL);

#ifdef __linux__
        usleep(300*1000);
#endif
    AVM_DVRDeleteStatus = 0;
    if (pHandle != NULL)
        pHandle->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_DELETE_STATUS, (void *)&AVM_DVRDeleteStatus, NULL);

    app->Func(THUMB_APP_FUNC_START_DISP_PAGE, FALSE, 0);

	CThreadPool::Get()->AddTask(system_sync, NULL);	

    return res;
}

static DVR_RESULT thumb_app_delete_file_complete(DVR_U32 param1, DVR_U32 param2)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();
    static unsigned int count = 0;

    count += 1;
    Log_Message("delete file status: %d/%d\n", count, app->nFilesHasBeenSelected);
    if (count < app->nFilesHasBeenSelected)
        return DVR_RES_SOK;
    count = 0;
    app->nFilesHasBeenSelected = 0;

    APP_REMOVEFLAGS(app->m_thumb.GFlags, APP_AFLAGS_BUSY);
    app->Func(THUMB_APP_FUNC_WARNING_MSG_SHOW_STOP, GUI_WARNING_PROCESSING, 0);

    if (((int)param1) < 0) {
        Log_Error("[App - ThumbMotion] <DeleteFileComplete> Delete files failed: %d", param2);
    }
    else {
        Log_Message("[App - ThumbMotion] <DeleteFileComplete> Received AMSG_MGR_MSG_OP_DONE");
        /** page update */
        app->Func(THUMB_APP_FUNC_START_DISP_PAGE, FALSE, 0);
    }

    return res;
}

static DVR_RESULT thumb_app_copy_file_complete(DVR_U32 param1, DVR_U32 param2)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();
    static unsigned int count = 0;

    count += 1;
    if (count < app->nFilesHasBeenSelected)
        return DVR_RES_SOK;
    count = 0;

    APP_REMOVEFLAGS(app->m_thumb.GFlags, APP_AFLAGS_BUSY);
    app->Gui(GUI_THUMB_APP_CMD_WARNING_HIDE, 0, 0);
    app->Gui(GUI_THUMB_APP_CMD_FLUSH, 0, 0);

    return res;
}

static DVR_RESULT thumb_app_change_display(void)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

    app->Gui(GUI_THUMB_APP_CMD_SET_LAYOUT, GUI_LAYOUT_THUMB, 0);
    app->Gui(GUI_THUMB_APP_CMD_MAIN_MENU_TAB_UPDATE, GUI_MAIN_MENU_TAB_THUMB, 4);
    app->Gui(GUI_THUMB_APP_CMD_EDIT_UPDATE, GUI_THUMB_EDIT_STATE_OFF, 4);
    app->Gui(GUI_THUMB_APP_CMD_EDIT_CHECKBOX_HIDE, 0, 0);
    app->Gui(GUI_THUMB_APP_CMD_FLUSH, 0, 0);

    return res;
}

static void thumb_app_card_insert_async(void *arg)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();
    res = Dvr_Sdk_StorageCard_CheckStatus();
    while(APP_CHECKFLAGS(res, DVR_STORAGE_CARD_STATUS_NOT_READY))
    {
        usleep(20*1000);
        res = Dvr_Sdk_StorageCard_CheckStatus();
    }

    app->Gui(GUI_THUMB_APP_CMD_CARD_UPDATE, GUI_CARD_STATE_READY, 4);
    app->Gui(GUI_THUMB_APP_CMD_FLUSH, 0, 0);
    app->Func(THUMB_APP_FUNC_START_DISP_PAGE, TRUE, 0);
}

static DVR_RESULT thumb_app_card_insert(void)
{
    return CThreadPool::Get()->AddTask(thumb_app_card_insert_async, NULL);
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

#if 0
	DvrSDCardSpeedTest speed_test;
	if(TRUE == speed_test.IsLowSpeedSDCard())
	{
		DVR_U32 DVR_SDCardErrorStatus = DVR_SDCARD_ERROR_STATUS_LOW_SPEED;
		DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_ERROR_STATUS_UPDATE, (void *)&DVR_SDCardErrorStatus, NULL);
	}
	else
	{
		DVR_U32 DVR_SDCardErrorStatus = DVR_SDCARD_ERROR_STATUS_NO_ERROR;
		DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_ERROR_STATUS_UPDATE, (void *)&DVR_SDCardErrorStatus, NULL);
	}	
#endif

    app->Gui(GUI_THUMB_APP_CMD_CARD_UPDATE, GUI_CARD_STATE_READY, 4);
    app->Gui(GUI_THUMB_APP_CMD_FLUSH, 0, 0);
    app->Func(THUMB_APP_FUNC_START_DISP_PAGE, TRUE, 0);
	
    return res;
}

static DVR_RESULT thumb_app_card_remove(void)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

    if (APP_CHECKFLAGS(app->m_thumb.GFlags, APP_AFLAGS_POPUP)) {
        AppWidget_Off(DVR_WIDGET_ALL, 0);
    }

#if 0
	DVR_U32 DVR_SDCardErrorStatus = DVR_SDCARD_ERROR_STATUS_NO_SDCARD; //no card
	DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_ERROR_STATUS_UPDATE, (void *)&DVR_SDCardErrorStatus, NULL);

	DVR_U32 DVR_SDCardFullStatus = DVR_SDCARD_FULL_STATUS_NOT_FULL;
	DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_CARD_FULL_STATUS_UPDATE, (void *)&DVR_SDCardFullStatus, NULL);
#endif

    app->Func(THUMB_APP_FUNC_WARNING_MSG_SHOW_STOP, 0, 0);
    app->Gui(GUI_THUMB_APP_CMD_CARD_UPDATE, GUI_CARD_STATE_NO_CARD, 4);
    app->Gui(GUI_THUMB_APP_CMD_FLUSH, 0, 0);
    app->Func(THUMB_APP_FUNC_START_DISP_PAGE, TRUE, 0);

    return res;
}

static DVR_RESULT thumb_app_warning_msg_show(int enable, int param1, int param2)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

    if (enable)
    {
        if (param2)
        {
            app->Gui(GUI_THUMB_APP_CMD_WARNING_UPDATE, param1, 4);
            app->Gui(GUI_THUMB_APP_CMD_WARNING_SHOW, 0, 0);
        }
    }
    else
    {
        app->Gui(GUI_THUMB_APP_CMD_WARNING_HIDE, 0, 0);
    }

    app->Gui(GUI_THUMB_APP_CMD_FLUSH, 0, 0);

    return res;
}

static DVR_RESULT thumb_app_switch_app(int tab)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

    switch (tab)
    {
    case GUI_MAIN_MENU_TAB_RECORD:	
    {
        res = DvrAppUtil_SwitchApp(Dvr_App_GetAppId(MDL_APP_RECORDER_ID));
        //start recorder async
        //DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_ASYNC_START_RECORDER, NULL, NULL);   
    }
    break;

    case GUI_MAIN_MENU_TAB_THUMB:
    {
        app->Gui(GUI_THUMB_APP_CMD_SET_LAYOUT, GUI_LAYOUT_THUMB, 0);
        app->Gui(GUI_THUMB_APP_CMD_MAIN_MENU_TAB_UPDATE, GUI_MAIN_MENU_TAB_THUMB, 4);
        app->Gui(GUI_THUMB_APP_CMD_FLUSH, 0, 0);
		
    	DVR_U32 AVM_DVRModeFeedback = 0x2; //0:Inactive, 0x1:RealTimeMode, 0x2:ReplayMode, 0x3:SettingMode
        DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_MODE_UPDATE, (void *)&AVM_DVRModeFeedback, NULL);
    }
    break;

    case GUI_MAIN_MENU_TAB_SETUP:
	{	
		AppWidget_On(DVR_WIDGET_MENU_SETUP, 0);
		
    	DVR_U32 AVM_DVRModeFeedback = 0x3; //0:Inactive, 0x1:RealTimeMode, 0x2:ReplayMode, 0x3:SettingMode
        DvrAppNotify::Get()->NotifyEventList(DVR_APP_NOTIFICATION_TYPE_MODE_UPDATE, (void *)&AVM_DVRModeFeedback, NULL);
    }
    break;

    default:
        break;
    }

    return res;
}

static DVR_RESULT thumb_app_format_card(void)
{
    DVR_RESULT res = DVR_RES_SOK;
    DvrThumbApp *app = DvrThumbApp::Get();

	app->Func(THUMB_APP_FUNC_CLEAR_CUR_PAGE, 0, 0);

	return res;
}

int thumb_app_func(DVR_U32 funcId, DVR_U32 param1, DVR_U32 param2)
{
    DVR_RESULT res = DVR_RES_SOK;

    switch (funcId)
    {
    case THUMB_APP_FUNC_INIT:
        res = thumb_app_init();
        break;

    case THUMB_APP_FUNC_START:
        res = thumb_app_start();
        break;

    case THUMB_APP_FUNC_STOP:
        res = thumb_app_stop();
        break;

    case THUMB_APP_FUNC_APP_READY:
        res = thumb_app_ready();
        break;

    case THUMB_APP_FUNC_RESET_DEFAULT_TAB:
        res = thumb_app_reset_defaut_tab();
        break;

    case THUMB_APP_FUNC_START_DISP_PAGE:
        res = thumb_app_start_disp_page(param1);
        break;

    case THUMB_APP_FUNC_SHIFT_TAB:
        res = thumb_app_shift_tab(param1);
        break;

    case THUMB_APP_FUNC_NEXT_PAGE:
        res = thumb_app_get_next_page();
        break;

    case THUMB_APP_FUNC_PREV_PAGE:
        res = thumb_app_get_prev_page();
        break;

    case THUMB_APP_FUNC_SHOW_PAGE_SIDEINFO:
        res = thumb_app_start_show_sideinfo();
        break;

    case THUMB_APP_FUNC_SHOW_PAGE_FRAME:
        res = thumb_app_show_page_frame();
        break;

    case THUMB_APP_FUNC_SHOW_PAGE_CHECKBOX:
        res = thumb_app_show_check_box_content();
        break;

    case THUMB_APP_FUNC_SHOW_PAGE_NUM:
        res = thumb_app_update_page_num();
        break;

    case THUMB_APP_FUNC_CLEAR_CUR_PAGE:
        res = thumb_app_clear_cur_page();
        break;

    case THUMB_APP_FUNC_UPDATE_PAGE_FRAME:
        res = thumb_app_update_page_frame();
        break;

    case THUMB_APP_FUNC_SEL_TO_PLAY:
        res = thumb_app_select_to_play(param1);
        break;

    case THUMB_APP_FUNC_EDIT_ENTER:
        res = thumb_app_enter_edit_mode();
        break;

    case THUMB_APP_FUNC_EDIT_QUIT:
        res = thumb_app_enter_exit_mode();
        break;

    case THUMB_APP_FUNC_EDIT_COPY:
        res = thumb_app_save_selected_file();
        break;

    case THUMB_APP_FUNC_EDIT_DEL:
        res = thumb_app_delete_selected_files();
        break;

    case THUMB_APP_FUNC_EDIT_SEL:
        //res = thumb_app_select_one_file(param1);
		res = thumb_app_select_files(param1);
        break;

    case THUMB_APP_FUNC_EDIT_SEL_ALL:
        res = thumb_app_select_all_files();
        break;

    case THUMB_APP_FUNC_CARD_INSERT:
        res = thumb_app_card_insert();
        break;

    case THUMB_APP_FUNC_CARD_REMOVE:
        res = thumb_app_card_remove();
        break;

    case THUMB_APP_FUNC_DELETE_FILE_DIALOG_SHOW:
        res = thumb_app_delete_file_dialog_show();
        break;

    case THUMB_APP_FUNC_DELETE_FILE:
        res = thumb_app_delete_file();
        break;

    case THUMB_APP_FUNC_DELETE_FILE_COMPLETE:
        res = thumb_app_delete_file_complete(param1, param2);
        break;

    case THUMB_APP_FUNC_COPY_FILE_COMPLETE:
        res = thumb_app_copy_file_complete(param1, param2);
        break;

    case THUMB_APP_FUNC_CHANGE_DISPLAY:
        res = thumb_app_change_display();
        break;

    case THUMB_APP_FUNC_WARNING_MSG_SHOW_START:
        res = thumb_app_warning_msg_show(1, param1, param2);
        break;

    case THUMB_APP_FUNC_WARNING_MSG_SHOW_STOP:
        res = thumb_app_warning_msg_show(0, param1, param2);
        break;

    case THUMB_APP_FUNC_SWITCH_APP:
        res = thumb_app_switch_app(param1);
        break;

    case THUMB_APP_FUNC_FORMAT_CARD:
		res = thumb_app_format_card();
        break;

    default:
        break;
    }

    return res;
}
