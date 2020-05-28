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

#ifndef _DVR_THUMB_APP_H_
#define _DVR_THUMB_APP_H_

#include <string.h>
#include "DVR_APP_INTFS.h"
#include "DVR_SDK_INTFS.h"
#include "framework/DvrAppMgt.h"
#include "framework/DvrAppUtil.h"
#include "framework/DvrAppMain.h"
#include "DVR_GUI_OBJ.h"

/*************************************************************************
* App Function Definitions
************************************************************************/
enum THUMB_APP_FUNC_ID
{
	THUMB_APP_FUNC_INIT,
	THUMB_APP_FUNC_START,
	THUMB_APP_FUNC_START_FLAG_ON,
	THUMB_APP_FUNC_STOP,

	THUMB_APP_FUNC_APP_READY,
    THUMB_APP_FUNC_RESET_DEFAULT_TAB,
	THUMB_APP_FUNC_START_DISP_PAGE,
	THUMB_APP_FUNC_SHIFT_TAB,
	THUMB_APP_FUNC_NEXT_PAGE,
	THUMB_APP_FUNC_PREV_PAGE,
    THUMB_APP_FUNC_SHOW_PAGE_SIDEINFO,
    THUMB_APP_FUNC_SHOW_PAGE_FRAME,
    THUMB_APP_FUNC_SHOW_PAGE_CHECKBOX,
    THUMB_APP_FUNC_SHOW_PAGE_NUM,
    THUMB_APP_FUNC_CLEAR_CUR_PAGE,
    THUMB_APP_FUNC_UPDATE_PAGE_FRAME,

	THUMB_APP_FUNC_CARD_INSERT,
	THUMB_APP_FUNC_CARD_REMOVE,

	THUMB_APP_FUNC_SELECT_DELETE_FILE_MODE,
	THUMB_APP_FUNC_SET_DELETE_FILE_MODE,
	THUMB_APP_FUNC_DELETE_FILE_DIALOG_SHOW,
	THUMB_APP_FUNC_DELETE_FILE,
	THUMB_APP_FUNC_DELETE_FILE_COMPLETE,
	THUMB_APP_FUNC_COPY_FILE_COMPLETE,

	THUMB_APP_FUNC_CHANGE_DISPLAY,

	THUMB_APP_FUNC_WARNING_MSG_SHOW_START,
	THUMB_APP_FUNC_WARNING_MSG_SHOW_STOP,

	THUMB_APP_FUNC_SEL_TO_PLAY,
	THUMB_APP_FUNC_EDIT_ENTER,
	THUMB_APP_FUNC_EDIT_QUIT,
	THUMB_APP_FUNC_EDIT_COPY,
	THUMB_APP_FUNC_EDIT_DEL,
	THUMB_APP_FUNC_EDIT_SEL,
	THUMB_APP_FUNC_EDIT_SEL_ALL,

	THUMB_APP_FUNC_SWITCH_APP,
    THUMB_APP_FUNC_FORMAT_CARD,
};

#define MAX_FILE_NUM_IN_CUR_FOLDER             1024
#ifdef DVR_FEATURE_V302
#define MAX_PAGE_NUM_IN_CUR_FOLDER             265
#else
#define MAX_PAGE_NUM_IN_CUR_FOLDER             200
#endif

/*************************************************************************
* App Status Definitions
************************************************************************/

typedef struct  
{
    unsigned char PreviewBuf[DVR_THUMBNAIL_WIDTH*DVR_THUMBNAIL_HEIGHT*3];
    unsigned int nPreviewBufSize;
    unsigned int ulPreviewHeight;
    unsigned int ulPreviewWidth;
    int color_format;
    unsigned char bPreviewReady;

    void Reset()
    {
        bPreviewReady = FALSE;
        memset(PreviewBuf, 0, sizeof(PreviewBuf));
    }
}THUMBNAIL_PREVIEW;

typedef struct
{
	unsigned char bValid;
    char displayName[APP_MAX_FN_SIZE];

    DVR_FILEMAP_META_ITEM stFullName;
}THUMBNAIL_ELEMENT;

typedef struct
{
	THUMBNAIL_ELEMENT item[NUM_THUMBNAIL_PER_PAGE];
	DVR_U8 check_box[NUM_THUMBNAIL_PER_PAGE];
	DVR_U8 PageItemNum; /** item number in current page */
}THUMBNAIL_PAGE;

typedef struct
{
	THUMBNAIL_PAGE ThumbPage[MAX_PAGE_NUM_IN_CUR_FOLDER];

	DVR_FOLDER_TYPE eCurFolder;
	DVR_FILE_TYPE eType;
	char FolderName[APP_MAX_FN_SIZE];
	DVR_U32 TotalFileNum;
	DVR_U32 TotalPageNum;
}THUMBNAIL_TAB;

typedef struct{
	DVR_FOLDER_TYPE eCurFolder;
	DVR_FILE_TYPE eType;
	char folder_name[APP_MAX_FN_SIZE];
	DVR_U32 TotalFileNum;
	DVR_U32 TotalPageNum;
	DVR_S32 PageCur;
	DVR_U8 chkbox[MAX_FILE_NUM_IN_CUR_FOLDER];
} THUMB_MOTION_PAGE_INFO;

class DvrThumbApp
{
private:
	DvrThumbApp();
	DvrThumbApp(const DvrThumbApp&);
	const DvrThumbApp& operator = (const DvrThumbApp&);

public:
	static DvrThumbApp *Get(void);
	APP_MGT_INST *GetApp()
	{
		return &m_thumb;
	}

	int(*Func)(DVR_U32 funcId, DVR_U32 param1, DVR_U32 param2);
	int(*Gui)(DVR_U32 guiCmd, DVR_U32 param1, DVR_U32 param2);

	APP_MGT_INST m_thumb;

private:
	DVR_BOOL Init();
	DVR_BOOL m_bHasInited;

public:
	DVR_U8 TabCur;
	DVR_U32 MaxPageItemNum;
	DVR_U32 nFilesHasBeenSelected;
    //THUMB_MOTION_PAGE_INFO PageInfo;
	//THUMBNAIL_PAGE thumb_page[MAX_PAGE_NUM_IN_CUR_FOLDER];
	//THUMBNAIL_PAGE cur_page;

	THUMBNAIL_TAB ThumbTab[GUI_THUMB_TAB_NUM];
	THUMBNAIL_TAB *pCurTab;
    THUMBNAIL_PAGE *pCurPage;
    DVR_S32 PageCur;
	
    THUMBNAIL_PREVIEW stPreview[NUM_THUMBNAIL_PER_PAGE];

    DVR_U8 bInEditMode;
};

#endif
