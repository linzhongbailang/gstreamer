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
#ifndef _DVR_GRAPHICS_H_
#define _DVR_GRAPHICS_H_

#include <pthread.h>
#include "DVR_GUI_OBJ.h"
#include "DVR_SDK_DEF.h"

class DvrGuiResHandler
{
public:
	DvrGuiResHandler(void);
	~DvrGuiResHandler(void);

	DVR_RESULT SetDefault(DVR_GUI_LAYOUT_TYPE id);
	DVR_RESULT Show(DVR_U32 guiId);
	DVR_RESULT Hide(DVR_U32 guiId);
	DVR_RESULT Enable(DVR_U32 guiId);
	DVR_RESULT Disable(DVR_U32 guiId);
	DVR_RESULT Update(DVR_U32 guiId, void *pPropData, int nPropSize);
	DVR_RESULT Flush(void);
	DVR_RESULT SetLayout(DVR_U32 id);

	DVR_RESULT GetLayout(DVR_GUI_LAYOUT_INST *pInst);
	DVR_RESULT GetLayoutInfo(DVR_U32 id, DVR_GRAPHIC_UIOBJ **ppTable, int *pObjNum);

private:
	pthread_mutex_t m_lock;
	DVR_GUI_LAYOUT_INST m_LayoutTbl[GUI_LAYOUT_NUM];
	DVR_GUI_LAYOUT_TYPE m_curLayout;
};

DVR_RESULT DvrGui_GetLayoutInfo(DVR_U32 id, DVR_GRAPHIC_UIOBJ **ppTable, int *pObjNum);

#endif
