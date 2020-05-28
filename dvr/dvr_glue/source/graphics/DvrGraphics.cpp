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
#include "graphics/DvrGraphics.h"

static GUI_OBJ_PLAY_TIME_INST m_play_time;
static GUI_OBJ_PLAY_FILENAME_INST m_play_filename;
static GUI_OBJ_THUMB_FRAME_INST m_thumb_frame;
static GUI_OBJ_THUMB_EDIT_INST m_thumb_edit;
static GUI_OBJ_THUMB_CAPCITY_INST m_capacity;
static GUI_OBJ_THUMB_PAGENUM_INST m_page_num;
static GUI_OBJ_DIALOG_INST m_dialog;
static GUI_OBJ_VEHICLE_DATA_INST m_vehicle_data;

DVR_GRAPHIC_UIOBJ rec_gui_table[] =
{
	{ GUI_OBJ_ID_MAIN_MENU_TAB, "main_menu", 1, 1, GUI_OBJ_STATUS_TYPE_U32, GUI_MAIN_MENU_TAB_RECORD },
    { GUI_OBJ_ID_CARD_STATE, "card_state", 1, 1, GUI_OBJ_STATUS_TYPE_U32, GUI_CARD_STATE_NO_CARD },
    { GUI_OBJ_ID_REC_SWITCH, "record_switch", 1, 1, GUI_OBJ_STATUS_TYPE_U32, GUI_SWITCH_STATE_OFF },
    { GUI_OBJ_ID_REC_STATE, "record_state", 1, 1, GUI_OBJ_STATUS_TYPE_U32, GUI_REC_STATE_STOP },
    { GUI_OBJ_ID_REC_EVENT_RECORD_STATE, "event_record_state", 1, 0, GUI_OBJ_STATUS_TYPE_U32, GUI_EMERGENCY_REC_STOP },
    { GUI_OBJ_ID_REC_VIEW_INDEX, "record_view", 1, 1, GUI_OBJ_STATUS_TYPE_U32, GUI_VIEW_INDEX_FRONT },
    { GUI_OBJ_ID_SIDEBAR, "sidebar", 1, 1, GUI_OBJ_STATUS_TYPE_U32, 0 },
    { GUI_OBJ_ID_REC_VEHICLE_DATA, "vehicle_data", 1, 1, GUI_OBJ_STATUS_TYPE_POINTER, (unsigned int)&m_vehicle_data },
};

DVR_GRAPHIC_UIOBJ thumb_gui_table[] =
{
    { GUI_OBJ_ID_MAIN_MENU_TAB, "main_menu", 1, 1, GUI_OBJ_STATUS_TYPE_U32, GUI_MAIN_MENU_TAB_THUMB },
    { GUI_OBJ_ID_THUMB_TAB, "thumb_tab", 1, 1, GUI_OBJ_STATUS_TYPE_U32, GUI_THUMB_TAB_LOOP_REC },
    { GUI_OBJ_ID_THUMB_EDIT, "thumb_edit_state", 1, 1, GUI_OBJ_STATUS_TYPE_U32, GUI_THUMB_EDIT_STATE_OFF },
	{ GUI_OBJ_ID_THUMB_EDIT_SEL_CHECKBOX, "thumb_edit_sel_chkbox", 1, 0, GUI_OBJ_STATUS_TYPE_POINTER, (unsigned int)&m_thumb_edit },
    { GUI_OBJ_ID_THUMB_FRAME, "thumb_content", 1, 1, GUI_OBJ_STATUS_TYPE_POINTER, (unsigned int)&m_thumb_frame },
    { GUI_OBJ_ID_THUMB_CAPACITY, "thumb_capacity", 1, 1, GUI_OBJ_STATUS_TYPE_POINTER, (unsigned int)&m_capacity },
    { GUI_OBJ_ID_THUMB_PAGE_NUM, "thumb_page", 1, 1, GUI_OBJ_STATUS_TYPE_POINTER, (unsigned int)&m_page_num },
    { GUI_OBJ_ID_DIALOG, "dialog", 1, 0, GUI_OBJ_STATUS_TYPE_POINTER, (unsigned int)&m_dialog },
    { GUI_OBJ_ID_WARNING, "warning", 1, 0, GUI_OBJ_STATUS_TYPE_U32, GUI_WARNING_NONE },
};

DVR_GRAPHIC_UIOBJ pb_video_gui_table[] =
{
    { GUI_OBJ_ID_MAIN_MENU_TAB, "main_menu", 1, 1, GUI_OBJ_STATUS_TYPE_U32, GUI_MAIN_MENU_TAB_THUMB },
    { GUI_OBJ_ID_PB_PLAY_STATE, "play_state", 1, 1, GUI_OBJ_STATUS_TYPE_U32, GUI_PLAY_STATE_INVALID },
    { GUI_OBJ_ID_PB_PLAY_TIMER, "play_timer", 1, 1, GUI_OBJ_STATUS_TYPE_POINTER, (unsigned int)&m_play_time },
    { GUI_OBJ_ID_PB_PLAY_SPEED, "play_speed", 1, 1, GUI_OBJ_STATUS_TYPE_U32, GUI_PLAY_SPEED_X1 },
    { GUI_OBJ_ID_PB_FILENAME, "play_file_name", 1, 1, GUI_OBJ_STATUS_TYPE_POINTER, (unsigned int)&m_play_filename },
    { GUI_OBJ_ID_PB_VIEW_INDEX, "play_view", 1, 1, GUI_OBJ_STATUS_TYPE_U32, GUI_VIEW_INDEX_FRONT },
    { GUI_OBJ_ID_PB_DC_SWITCH, "play_dc_switch", 1, 1, GUI_OBJ_STATUS_TYPE_U32, GUI_SWITCH_STATE_OFF },
    { GUI_OBJ_ID_DIALOG, "dialog", 1, 0, GUI_OBJ_STATUS_TYPE_POINTER, (unsigned int)&m_dialog },
    { GUI_OBJ_ID_SIDEBAR, "sidebar", 1, 1, GUI_OBJ_STATUS_TYPE_U32, 0 },
    { GUI_OBJ_ID_PB_MODE, "play_mode", 1, 1, GUI_OBJ_STATUS_TYPE_U32, GUI_PLAY_MODE_DVR },
	{ GUI_OBJ_ID_PB_VEHICLE_DATA, "vehicle_data", 1, 0, GUI_OBJ_STATUS_TYPE_POINTER, (unsigned int)&m_vehicle_data }, 
	{ GUI_OBJ_ID_WARNING, "warning", 1, 0, GUI_OBJ_STATUS_TYPE_U32, GUI_WARNING_NONE }
};

DVR_GRAPHIC_UIOBJ pb_image_gui_table[] =
{
    { GUI_OBJ_ID_MAIN_MENU_TAB, "main_menu", 1, 1, GUI_OBJ_STATUS_TYPE_U32, GUI_MAIN_MENU_TAB_THUMB },
    { GUI_OBJ_ID_PB_FILENAME, "play_file_name", 1, 1, GUI_OBJ_STATUS_TYPE_POINTER, (unsigned int)&m_play_filename },
    { GUI_OBJ_ID_PB_VIEW_INDEX, "play_view", 1, 1, GUI_OBJ_STATUS_TYPE_U32, GUI_VIEW_INDEX_FRONT },
    { GUI_OBJ_ID_PB_DC_SWITCH, "play_dc_switch", 1, 1, GUI_OBJ_STATUS_TYPE_U32, GUI_SWITCH_STATE_OFF },
    { GUI_OBJ_ID_DIALOG, "dialog", 1, 0, GUI_OBJ_STATUS_TYPE_POINTER, (unsigned int)&m_dialog },
    { GUI_OBJ_ID_SIDEBAR, "sidebar", 1, 1, GUI_OBJ_STATUS_TYPE_U32, 0 },
    { GUI_OBJ_ID_PB_MODE, "play_mode", 1, 1, GUI_OBJ_STATUS_TYPE_U32, GUI_PLAY_MODE_DVR },
	{ GUI_OBJ_ID_PB_VEHICLE_DATA, "vehicle_data", 1, 0, GUI_OBJ_STATUS_TYPE_POINTER, (unsigned int)&m_vehicle_data }, 
	{ GUI_OBJ_ID_WARNING, "warning", 1, 0, GUI_OBJ_STATUS_TYPE_U32, GUI_WARNING_NONE }
};

DVR_GRAPHIC_UIOBJ setup_gui_table[] =
{
    { GUI_OBJ_ID_MAIN_MENU_TAB, "main_menu", 1, 1, GUI_OBJ_STATUS_TYPE_U32, GUI_MAIN_MENU_TAB_SETUP },
    { GUI_OBJ_ID_SETUP_SPLIT_TIME, "split_time", 1, 1, GUI_OBJ_STATUS_TYPE_U32, GUI_SETUP_SPLIT_TIME_1MIN },
    { GUI_OBJ_ID_SETUP_VIDEO_QUALITY, "video_quality", 1, 1, GUI_OBJ_STATUS_TYPE_U32, GUI_SETUP_VIDEO_QUALITY_SFINE },
    { GUI_OBJ_ID_SETUP_PHOTO_QUALITY, "photo_quality", 1, 1, GUI_OBJ_STATUS_TYPE_U32, GUI_SETUP_PHOTO_QUALITY_SFINE },
    { GUI_OBJ_ID_SETUP_FORMAT_CARD, "format_card", 1, 1, GUI_OBJ_STATUS_TYPE_U32, 0 },
    { GUI_OBJ_ID_DIALOG, "dialog", 1, 0, GUI_OBJ_STATUS_TYPE_POINTER, (unsigned int)&m_dialog },
    { GUI_OBJ_ID_WARNING, "warning", 1, 0, GUI_OBJ_STATUS_TYPE_U32, GUI_WARNING_NONE }
};

DvrGuiResHandler::DvrGuiResHandler(void)
{	
	m_LayoutTbl[GUI_LAYOUT_RECORD].curLayout = GUI_LAYOUT_RECORD;
	m_LayoutTbl[GUI_LAYOUT_RECORD].pTable = (DVR_GRAPHIC_UIOBJ *)malloc(sizeof(rec_gui_table));
	if (m_LayoutTbl[GUI_LAYOUT_RECORD].pTable == NULL)
		return;
	m_LayoutTbl[GUI_LAYOUT_RECORD].ObjNum = sizeof(rec_gui_table) / sizeof(rec_gui_table[0]);
	SetDefault(GUI_LAYOUT_RECORD);

	m_LayoutTbl[GUI_LAYOUT_THUMB].curLayout = GUI_LAYOUT_THUMB;
	m_LayoutTbl[GUI_LAYOUT_THUMB].pTable = (DVR_GRAPHIC_UIOBJ *)malloc(sizeof(thumb_gui_table));
	if (m_LayoutTbl[GUI_LAYOUT_THUMB].pTable == NULL)
		return;
	m_LayoutTbl[GUI_LAYOUT_THUMB].ObjNum = sizeof(thumb_gui_table) / sizeof(thumb_gui_table[0]);
	SetDefault(GUI_LAYOUT_THUMB);

	m_LayoutTbl[GUI_LAYOUT_PLAYBACK_VIDEO].curLayout = GUI_LAYOUT_PLAYBACK_VIDEO;
	m_LayoutTbl[GUI_LAYOUT_PLAYBACK_VIDEO].pTable = (DVR_GRAPHIC_UIOBJ *)malloc(sizeof(pb_video_gui_table));
	if (m_LayoutTbl[GUI_LAYOUT_PLAYBACK_VIDEO].pTable == NULL)
		return;
	m_LayoutTbl[GUI_LAYOUT_PLAYBACK_VIDEO].ObjNum = sizeof(pb_video_gui_table) / sizeof(pb_video_gui_table[0]);
	SetDefault(GUI_LAYOUT_PLAYBACK_VIDEO);

	m_LayoutTbl[GUI_LAYOUT_PLAYBACK_IMAGE].curLayout = GUI_LAYOUT_PLAYBACK_IMAGE;
	m_LayoutTbl[GUI_LAYOUT_PLAYBACK_IMAGE].pTable = (DVR_GRAPHIC_UIOBJ *)malloc(sizeof(pb_image_gui_table));
	if (m_LayoutTbl[GUI_LAYOUT_PLAYBACK_IMAGE].pTable == NULL)
		return;
	m_LayoutTbl[GUI_LAYOUT_PLAYBACK_IMAGE].ObjNum = sizeof(pb_image_gui_table) / sizeof(pb_image_gui_table[0]);
	SetDefault(GUI_LAYOUT_PLAYBACK_IMAGE);

	m_LayoutTbl[GUI_LAYOUT_SETUP].curLayout = GUI_LAYOUT_SETUP;
	m_LayoutTbl[GUI_LAYOUT_SETUP].pTable = (DVR_GRAPHIC_UIOBJ *)malloc(sizeof(setup_gui_table));
	if (m_LayoutTbl[GUI_LAYOUT_SETUP].pTable == NULL)
		return;
	m_LayoutTbl[GUI_LAYOUT_SETUP].ObjNum = sizeof(setup_gui_table) / sizeof(setup_gui_table[0]);
	SetDefault(GUI_LAYOUT_SETUP);

	m_curLayout = GUI_LAYOUT_RECORD;

	memset(&m_play_time, 0, sizeof(m_play_time));
	memset(&m_play_filename, 0, sizeof(m_play_filename));
	memset(&m_thumb_frame, 0, sizeof(m_thumb_frame));
	memset(&m_thumb_edit, 0, sizeof(m_thumb_edit));
	memset(&m_capacity, 0, sizeof(m_capacity));
	memset(&m_dialog, 0, sizeof(m_dialog));
	memset(&m_vehicle_data, 0, sizeof(m_vehicle_data));

	pthread_mutex_init(&m_lock, NULL);
}

DvrGuiResHandler::~DvrGuiResHandler(void)
{
	for (int i = 0; i < GUI_LAYOUT_NUM; i++)
	{
		if (m_LayoutTbl[i].pTable != NULL)
			free(m_LayoutTbl[i].pTable);
	}
	pthread_mutex_destroy(&m_lock);
}

DVR_RESULT DvrGuiResHandler::SetDefault(DVR_GUI_LAYOUT_TYPE id)
{
	DVR_RESULT res = DVR_RES_SOK;

	if (id >= GUI_LAYOUT_NUM)
		return DVR_RES_EINVALIDARG;

	switch (id)
	{
	case GUI_LAYOUT_RECORD:
		memcpy(m_LayoutTbl[GUI_LAYOUT_RECORD].pTable, rec_gui_table, sizeof(rec_gui_table));
		break;

	case GUI_LAYOUT_THUMB:
		memcpy(m_LayoutTbl[GUI_LAYOUT_THUMB].pTable, thumb_gui_table, sizeof(thumb_gui_table));
		break;

	case GUI_LAYOUT_PLAYBACK_VIDEO:
		memcpy(m_LayoutTbl[GUI_LAYOUT_PLAYBACK_VIDEO].pTable, pb_video_gui_table, sizeof(pb_video_gui_table));
		break;

	case GUI_LAYOUT_PLAYBACK_IMAGE:
		memcpy(m_LayoutTbl[GUI_LAYOUT_PLAYBACK_IMAGE].pTable, pb_image_gui_table, sizeof(pb_image_gui_table));
		break;

	case GUI_LAYOUT_SETUP:
		memcpy(m_LayoutTbl[GUI_LAYOUT_SETUP].pTable, setup_gui_table, sizeof(setup_gui_table));
		break;

	default:
		break;
	}

	return res;
}

DVR_RESULT DvrGuiResHandler::SetLayout(DVR_U32 id)
{
	if (id >= GUI_LAYOUT_NUM)
		return DVR_RES_EINVALIDARG;

	m_curLayout = (DVR_GUI_LAYOUT_TYPE)id;

	return DVR_RES_SOK;
}


DVR_RESULT DvrGuiResHandler::Show(DVR_U32 guiId)
{
	DVR_RESULT res = DVR_RES_SOK;

	if (guiId >= GUI_OBJ_ID_NUM)
		return DVR_RES_EINVALIDARG;

	pthread_mutex_lock(&m_lock);

	for (int i = 0; i < m_LayoutTbl[m_curLayout].ObjNum; i++)
	{
		if (m_LayoutTbl[m_curLayout].pTable[i].Id == guiId)
			m_LayoutTbl[m_curLayout].pTable[i].bShow = 1;
	}

	pthread_mutex_unlock(&m_lock);

	return res;
}

DVR_RESULT DvrGuiResHandler::Hide(DVR_U32 guiId)
{
	DVR_RESULT res = DVR_RES_SOK;

	if (guiId >= GUI_OBJ_ID_NUM)
		return DVR_RES_EINVALIDARG;

	pthread_mutex_lock(&m_lock);

	for (int i = 0; i < m_LayoutTbl[m_curLayout].ObjNum; i++)
	{
		if (m_LayoutTbl[m_curLayout].pTable[i].Id == guiId)
			m_LayoutTbl[m_curLayout].pTable[i].bShow = 0;
	}

	pthread_mutex_unlock(&m_lock);

	return res;
}

DVR_RESULT DvrGuiResHandler::Enable(DVR_U32 guiId)
{
	DVR_RESULT res = DVR_RES_SOK;

	if (guiId >= GUI_OBJ_ID_NUM)
		return DVR_RES_EINVALIDARG;

	pthread_mutex_lock(&m_lock);

	for (int i = 0; i < m_LayoutTbl[m_curLayout].ObjNum; i++)
	{
		if (m_LayoutTbl[m_curLayout].pTable[i].Id == guiId)
			m_LayoutTbl[m_curLayout].pTable[i].bEnable = 1;
	}

	pthread_mutex_unlock(&m_lock);

	return res;
}

DVR_RESULT DvrGuiResHandler::Disable(DVR_U32 guiId)
{
	DVR_RESULT res = DVR_RES_SOK;

	if (guiId >= GUI_OBJ_ID_NUM)
		return DVR_RES_EINVALIDARG;

	pthread_mutex_lock(&m_lock);

	for (int i = 0; i < m_LayoutTbl[m_curLayout].ObjNum; i++)
	{
		if (m_LayoutTbl[m_curLayout].pTable[i].Id == guiId)
			m_LayoutTbl[m_curLayout].pTable[i].bEnable = 0;
	}

	pthread_mutex_unlock(&m_lock);

	return res;
}

DVR_RESULT DvrGuiResHandler::Flush(void)
{
	DVR_RESULT res = DVR_RES_SOK;
	int i;

	pthread_mutex_lock(&m_lock);

	switch (m_curLayout)
	{
	case GUI_LAYOUT_RECORD:
		for (i = 0; i < m_LayoutTbl[GUI_LAYOUT_RECORD].ObjNum; i++)
		{
			memcpy(&rec_gui_table[i], &m_LayoutTbl[GUI_LAYOUT_RECORD].pTable[i], sizeof(DVR_GRAPHIC_UIOBJ));
		}
		break;

	case GUI_LAYOUT_THUMB:
		for (i = 0; i < m_LayoutTbl[GUI_LAYOUT_THUMB].ObjNum; i++)
		{
			memcpy(&thumb_gui_table[i], &m_LayoutTbl[GUI_LAYOUT_THUMB].pTable[i], sizeof(DVR_GRAPHIC_UIOBJ));
		}
		break;

	case GUI_LAYOUT_PLAYBACK_VIDEO:
		for (i = 0; i < m_LayoutTbl[GUI_LAYOUT_PLAYBACK_VIDEO].ObjNum; i++)
		{
			memcpy(&pb_video_gui_table[i], &m_LayoutTbl[GUI_LAYOUT_PLAYBACK_VIDEO].pTable[i], sizeof(DVR_GRAPHIC_UIOBJ));
		}
		break;

	case GUI_LAYOUT_PLAYBACK_IMAGE:
		for (i = 0; i < m_LayoutTbl[GUI_LAYOUT_PLAYBACK_IMAGE].ObjNum; i++)
		{
			memcpy(&pb_image_gui_table[i], &m_LayoutTbl[GUI_LAYOUT_PLAYBACK_IMAGE].pTable[i], sizeof(DVR_GRAPHIC_UIOBJ));
		}
		break;

	case GUI_LAYOUT_SETUP:
		for (i = 0; i < m_LayoutTbl[GUI_LAYOUT_SETUP].ObjNum; i++)
		{
			memcpy(&setup_gui_table[i], &m_LayoutTbl[GUI_LAYOUT_SETUP].pTable[i], sizeof(DVR_GRAPHIC_UIOBJ));
		}
		break;

	default:
		break;

	}

	pthread_mutex_unlock(&m_lock);

	return res;
}

DVR_RESULT DvrGuiResHandler::Update(DVR_U32 guiId, void *pPropData, int nPropSize)
{
	DVR_RESULT res = DVR_RES_SOK;
	int i;

	if (guiId >= GUI_OBJ_ID_NUM)
		return DVR_RES_EINVALIDARG;

	if (pPropData == NULL)
		return DVR_RES_EPOINTER;

	pthread_mutex_lock(&m_lock);

	switch (m_curLayout)
	{
	case GUI_LAYOUT_RECORD:
	{
		switch (guiId)
		{
		case GUI_OBJ_ID_MAIN_MENU_TAB:
		case GUI_OBJ_ID_CARD_STATE:
		case GUI_OBJ_ID_REC_SWITCH:
		case GUI_OBJ_ID_REC_STATE:
		case GUI_OBJ_ID_REC_VIEW_INDEX:
		{
			if (nPropSize != 4)
			{
				pthread_mutex_unlock(&m_lock);
				return DVR_RES_EINVALIDARG;
			}
			for (i = 0; i < m_LayoutTbl[GUI_LAYOUT_RECORD].ObjNum; i++)
			{
				if (m_LayoutTbl[GUI_LAYOUT_RECORD].pTable[i].Id == guiId)
				{
					m_LayoutTbl[GUI_LAYOUT_RECORD].pTable[i].uStatus.ObjVal = *(DVR_U32 *)pPropData;
					break;
				}
			}
		}
		break;

		case GUI_OBJ_ID_REC_VEHICLE_DATA:
		{
			if (nPropSize != sizeof(GUI_OBJ_VEHICLE_DATA_INST))
			{
				pthread_mutex_unlock(&m_lock);
				return DVR_RES_EINVALIDARG;
			}
			for (i = 0; i < m_LayoutTbl[GUI_LAYOUT_RECORD].ObjNum; i++)
			{
				if (m_LayoutTbl[GUI_LAYOUT_RECORD].pTable[i].Id == guiId)
				{
					memcpy(&m_vehicle_data, pPropData, nPropSize);
					m_LayoutTbl[GUI_LAYOUT_RECORD].pTable[i].uStatus.ptr = &m_vehicle_data;
					break;
				}
			}
		}
		break;		

		default:
			break;
		}
	}
	break;

	case GUI_LAYOUT_THUMB:
	{
		switch (guiId)
		{
		case GUI_OBJ_ID_MAIN_MENU_TAB:
		case GUI_OBJ_ID_THUMB_TAB:
		case GUI_OBJ_ID_THUMB_EDIT:
		case GUI_OBJ_ID_WARNING:
		{
			if (nPropSize != 4)
			{
				pthread_mutex_unlock(&m_lock);
				return DVR_RES_EINVALIDARG;
			}
			for (i = 0; i < m_LayoutTbl[GUI_LAYOUT_THUMB].ObjNum; i++)
			{
				if (m_LayoutTbl[GUI_LAYOUT_THUMB].pTable[i].Id == guiId)
				{
					m_LayoutTbl[GUI_LAYOUT_THUMB].pTable[i].uStatus.ObjVal = *(DVR_U32 *)pPropData;
					break;
				}
			}
		}
		break;

		case GUI_OBJ_ID_THUMB_FRAME:
		{
			if (nPropSize != sizeof(GUI_OBJ_THUMB_FRAME_INST))
			{
				pthread_mutex_unlock(&m_lock);
				return DVR_RES_EINVALIDARG;
			}
			for (i = 0; i < m_LayoutTbl[GUI_LAYOUT_THUMB].ObjNum; i++)
			{
				if (m_LayoutTbl[GUI_LAYOUT_THUMB].pTable[i].Id == guiId)
				{
					memcpy(&m_thumb_frame, pPropData, nPropSize);
					m_LayoutTbl[GUI_LAYOUT_THUMB].pTable[i].uStatus.ptr = &m_thumb_frame;
					break;
				}
			}
		}
		break;

		case GUI_OBJ_ID_THUMB_EDIT_SEL_CHECKBOX:
		{
			if (nPropSize != sizeof(GUI_OBJ_THUMB_EDIT_INST))
			{
				pthread_mutex_unlock(&m_lock);
				return DVR_RES_EINVALIDARG;
			}
			for (i = 0; i < m_LayoutTbl[GUI_LAYOUT_THUMB].ObjNum; i++)
			{
				if (m_LayoutTbl[GUI_LAYOUT_THUMB].pTable[i].Id == guiId)
				{
					memcpy(&m_thumb_edit, pPropData, nPropSize);
					m_LayoutTbl[GUI_LAYOUT_THUMB].pTable[i].uStatus.ptr = &m_thumb_edit;
					break;
				}
			}
		}
		break;

		case GUI_OBJ_ID_THUMB_CAPACITY:
		{
			if (nPropSize != sizeof(GUI_OBJ_THUMB_CAPCITY_INST))
			{
				pthread_mutex_unlock(&m_lock);
				return DVR_RES_EINVALIDARG;
			}
			for (i = 0; i < m_LayoutTbl[GUI_LAYOUT_THUMB].ObjNum; i++)
			{
				if (m_LayoutTbl[GUI_LAYOUT_THUMB].pTable[i].Id == guiId)
				{
					memcpy(&m_capacity, pPropData, nPropSize);
					m_LayoutTbl[GUI_LAYOUT_THUMB].pTable[i].uStatus.ptr = &m_capacity;
					break;
				}
			}
		}
		break;

        case GUI_OBJ_ID_THUMB_PAGE_NUM:
        {
            if (nPropSize != sizeof(GUI_OBJ_THUMB_PAGENUM_INST))
            {
                pthread_mutex_unlock(&m_lock);
                return DVR_RES_EINVALIDARG;
            }
            for (i = 0; i < m_LayoutTbl[GUI_LAYOUT_THUMB].ObjNum; i++)
            {
                if (m_LayoutTbl[GUI_LAYOUT_THUMB].pTable[i].Id == guiId)
                {
                    memcpy(&m_page_num, pPropData, nPropSize);
                    m_LayoutTbl[GUI_LAYOUT_THUMB].pTable[i].uStatus.ptr = &m_page_num;
                    break;
                }
            }
        }
        break;

		case GUI_OBJ_ID_DIALOG:
		{
			if (nPropSize != sizeof(GUI_OBJ_DIALOG_INST))
			{
				pthread_mutex_unlock(&m_lock);
				return DVR_RES_EINVALIDARG;
			}
			for (i = 0; i < m_LayoutTbl[GUI_LAYOUT_THUMB].ObjNum; i++)
			{
				if (m_LayoutTbl[GUI_LAYOUT_THUMB].pTable[i].Id == guiId)
				{
					memcpy(&m_dialog, pPropData, nPropSize);
					m_LayoutTbl[GUI_LAYOUT_THUMB].pTable[i].uStatus.ptr = &m_dialog;
					break;
				}
			}
		}
		break;

		default:
			break;
		}
	}
	break;

	case GUI_LAYOUT_PLAYBACK_VIDEO:
	{
		switch (guiId)
		{
		case GUI_OBJ_ID_MAIN_MENU_TAB:
		case GUI_OBJ_ID_PB_PLAY_STATE:
		case GUI_OBJ_ID_PB_PLAY_SPEED:
		case GUI_OBJ_ID_PB_VIEW_INDEX:
		case GUI_OBJ_ID_PB_DC_SWITCH:
        case GUI_OBJ_ID_PB_MODE:
		{
			if (nPropSize != 4)
			{
				pthread_mutex_unlock(&m_lock);
				return DVR_RES_EINVALIDARG;
			}
			for (i = 0; i < m_LayoutTbl[GUI_LAYOUT_PLAYBACK_VIDEO].ObjNum; i++)
			{
				if (m_LayoutTbl[GUI_LAYOUT_PLAYBACK_VIDEO].pTable[i].Id == guiId)
				{
					m_LayoutTbl[GUI_LAYOUT_PLAYBACK_VIDEO].pTable[i].uStatus.ObjVal = *(DVR_U32 *)pPropData;
					break;
				}
			}
		}
		break;

		case GUI_OBJ_ID_PB_PLAY_TIMER:
		{
			if (nPropSize != sizeof(GUI_OBJ_PLAY_TIME_INST))
			{
				pthread_mutex_unlock(&m_lock);
				return DVR_RES_EINVALIDARG;
			}
			for (i = 0; i < m_LayoutTbl[GUI_LAYOUT_PLAYBACK_VIDEO].ObjNum; i++)
			{
				if (m_LayoutTbl[GUI_LAYOUT_PLAYBACK_VIDEO].pTable[i].Id == guiId)
				{
					memcpy(&m_play_time, pPropData, nPropSize);
					m_LayoutTbl[GUI_LAYOUT_PLAYBACK_VIDEO].pTable[i].uStatus.ptr = &m_play_time;
					break;
				}
			}
		}
		break;

		case GUI_OBJ_ID_PB_FILENAME:
		{
			if (nPropSize != sizeof(GUI_OBJ_PLAY_FILENAME_INST))
			{
				pthread_mutex_unlock(&m_lock);
				return DVR_RES_EINVALIDARG;
			}
			for (i = 0; i < m_LayoutTbl[GUI_LAYOUT_PLAYBACK_VIDEO].ObjNum; i++)
			{
				if (m_LayoutTbl[GUI_LAYOUT_PLAYBACK_VIDEO].pTable[i].Id == guiId)
				{
					memcpy(&m_play_filename, pPropData, nPropSize);
					m_LayoutTbl[GUI_LAYOUT_PLAYBACK_VIDEO].pTable[i].uStatus.ptr = &m_play_filename;
					break;
				}
			}
		}
		break;

		case GUI_OBJ_ID_DIALOG:
		{
			if (nPropSize != sizeof(GUI_OBJ_DIALOG_INST))
			{
				pthread_mutex_unlock(&m_lock);
				return DVR_RES_EINVALIDARG;
			}
			for (i = 0; i < m_LayoutTbl[GUI_LAYOUT_PLAYBACK_VIDEO].ObjNum; i++)
			{
				if (m_LayoutTbl[GUI_LAYOUT_PLAYBACK_VIDEO].pTable[i].Id == guiId)
				{
					memcpy(&m_dialog, pPropData, nPropSize);
					m_LayoutTbl[GUI_LAYOUT_PLAYBACK_VIDEO].pTable[i].uStatus.ptr = &m_dialog;
					break;
				}
			}
		}
		break;
		
		case GUI_OBJ_ID_PB_VEHICLE_DATA:
		{
			if (nPropSize != sizeof(GUI_OBJ_VEHICLE_DATA_INST))
			{
				pthread_mutex_unlock(&m_lock);
				return DVR_RES_EINVALIDARG;
			}
			for (i = 0; i < m_LayoutTbl[GUI_LAYOUT_PLAYBACK_VIDEO].ObjNum; i++)
			{
				if (m_LayoutTbl[GUI_LAYOUT_PLAYBACK_VIDEO].pTable[i].Id == guiId)
				{
					memcpy(&m_vehicle_data, pPropData, nPropSize);
					m_LayoutTbl[GUI_LAYOUT_PLAYBACK_VIDEO].pTable[i].uStatus.ptr = &m_vehicle_data;
					break;
				}
			}
		}
		break;	

		default:
			break;
		}
	}
	break;

	case GUI_LAYOUT_PLAYBACK_IMAGE:
	{
		switch (guiId)
		{
		case GUI_OBJ_ID_MAIN_MENU_TAB:
		case GUI_OBJ_ID_PB_VIEW_INDEX:
        case GUI_OBJ_ID_PB_DC_SWITCH:
        case GUI_OBJ_ID_PB_MODE:
		{
			if (nPropSize != 4)
			{
				pthread_mutex_unlock(&m_lock);
				return DVR_RES_EINVALIDARG;
			}
			for (i = 0; i < m_LayoutTbl[GUI_LAYOUT_PLAYBACK_IMAGE].ObjNum; i++)
			{
				if (m_LayoutTbl[GUI_LAYOUT_PLAYBACK_IMAGE].pTable[i].Id == guiId)
				{
					m_LayoutTbl[GUI_LAYOUT_PLAYBACK_IMAGE].pTable[i].uStatus.ObjVal = *(DVR_U32 *)pPropData;
					break;
				}
			}
		}
		break;

		case GUI_OBJ_ID_PB_FILENAME:
		{
			if (nPropSize != sizeof(GUI_OBJ_PLAY_FILENAME_INST))
			{
				pthread_mutex_unlock(&m_lock);
				return DVR_RES_EINVALIDARG;
			}
			for (i = 0; i < m_LayoutTbl[GUI_LAYOUT_PLAYBACK_IMAGE].ObjNum; i++)
			{
				if (m_LayoutTbl[GUI_LAYOUT_PLAYBACK_IMAGE].pTable[i].Id == guiId)
				{
					memcpy(&m_play_filename, pPropData, nPropSize);
					m_LayoutTbl[GUI_LAYOUT_PLAYBACK_IMAGE].pTable[i].uStatus.ptr = &m_play_filename;
					break;
				}
			}
		}
		break;

		case GUI_OBJ_ID_DIALOG:
		{
			if (nPropSize != sizeof(GUI_OBJ_DIALOG_INST))
			{
				pthread_mutex_unlock(&m_lock);
				return DVR_RES_EINVALIDARG;
			}
			for (i = 0; i < m_LayoutTbl[GUI_LAYOUT_PLAYBACK_IMAGE].ObjNum; i++)
			{
				if (m_LayoutTbl[GUI_LAYOUT_PLAYBACK_IMAGE].pTable[i].Id == guiId)
				{
					memcpy(&m_dialog, pPropData, nPropSize);
					m_LayoutTbl[GUI_LAYOUT_PLAYBACK_IMAGE].pTable[i].uStatus.ptr = &m_dialog;
					break;
				}
			}
		}
		break;

		case GUI_OBJ_ID_PB_VEHICLE_DATA:
		{
			if (nPropSize != sizeof(GUI_OBJ_VEHICLE_DATA_INST))
			{
				pthread_mutex_unlock(&m_lock);
				return DVR_RES_EINVALIDARG;
			}
			for (i = 0; i < m_LayoutTbl[GUI_LAYOUT_PLAYBACK_IMAGE].ObjNum; i++)
			{
				if (m_LayoutTbl[GUI_LAYOUT_PLAYBACK_IMAGE].pTable[i].Id == guiId)
				{
					memcpy(&m_vehicle_data, pPropData, nPropSize);
					m_LayoutTbl[GUI_LAYOUT_PLAYBACK_IMAGE].pTable[i].uStatus.ptr = &m_vehicle_data;
					break;
				}
			}
		}
		break;			

		default:
			break;
		}
	}
	break;

	case GUI_LAYOUT_SETUP:
	{
		switch (guiId)
		{
		case GUI_OBJ_ID_MAIN_MENU_TAB:
		case GUI_OBJ_ID_SETUP_SPLIT_TIME:
		case GUI_OBJ_ID_SETUP_VIDEO_QUALITY:
		case GUI_OBJ_ID_SETUP_PHOTO_QUALITY:
		case GUI_OBJ_ID_WARNING:
		{
			if (nPropSize != 4)
			{
				pthread_mutex_unlock(&m_lock);
				return DVR_RES_EINVALIDARG;
			}
			for (i = 0; i < m_LayoutTbl[GUI_LAYOUT_SETUP].ObjNum; i++)
			{
				if (m_LayoutTbl[GUI_LAYOUT_SETUP].pTable[i].Id == guiId)
				{
					m_LayoutTbl[GUI_LAYOUT_SETUP].pTable[i].uStatus.ObjVal = *(DVR_U32 *)pPropData;
					break;
				}
			}
		}
		break;

		case GUI_OBJ_ID_DIALOG:
		{
			if (nPropSize != sizeof(GUI_OBJ_DIALOG_INST))
			{
				pthread_mutex_unlock(&m_lock);
				return DVR_RES_EINVALIDARG;
			}
			for (i = 0; i < m_LayoutTbl[GUI_LAYOUT_SETUP].ObjNum; i++)
			{
				if (m_LayoutTbl[GUI_LAYOUT_SETUP].pTable[i].Id == guiId)
				{
					memcpy(&m_dialog, pPropData, nPropSize);
					m_LayoutTbl[GUI_LAYOUT_SETUP].pTable[i].uStatus.ptr = &m_dialog;
					break;
				}
			}
		}
		break;

		default:
			break;
		}
	}
	break;

	default:
		break;
	}
	
	pthread_mutex_unlock(&m_lock);

	return res;
}

DVR_RESULT DvrGuiResHandler::GetLayout(DVR_GUI_LAYOUT_INST *pInst)
{
	DVR_RESULT res = DVR_RES_SOK;

	if (pInst == NULL)
		return DVR_RES_EPOINTER;

	pthread_mutex_lock(&m_lock);

	switch (m_curLayout)
	{
        case GUI_LAYOUT_RECORD:
        {
            pInst->curLayout = GUI_LAYOUT_RECORD;
            pInst->ObjNum = sizeof(rec_gui_table) / sizeof(rec_gui_table[0]);
            pInst->pTable = rec_gui_table;
        }
		break;

        case GUI_LAYOUT_THUMB:
        {
			pInst->curLayout = GUI_LAYOUT_THUMB;
			pInst->ObjNum = sizeof(thumb_gui_table) / sizeof(thumb_gui_table[0]);
			pInst->pTable = thumb_gui_table;
        }
		break;

        case GUI_LAYOUT_PLAYBACK_VIDEO:
        {
			pInst->curLayout = GUI_LAYOUT_PLAYBACK_VIDEO;
			pInst->ObjNum = sizeof(pb_video_gui_table) / sizeof(pb_video_gui_table[0]);
			pInst->pTable = pb_video_gui_table;
        }
        break;

        case GUI_LAYOUT_PLAYBACK_IMAGE:
        {
            pInst->curLayout = GUI_LAYOUT_PLAYBACK_IMAGE;
            pInst->ObjNum = sizeof(pb_image_gui_table) / sizeof(pb_image_gui_table[0]);
            pInst->pTable = pb_image_gui_table;
        }
        break;

        case GUI_LAYOUT_SETUP:
        {
			pInst->curLayout = GUI_LAYOUT_SETUP;
			pInst->ObjNum = sizeof(setup_gui_table) / sizeof(setup_gui_table[0]);
			pInst->pTable = setup_gui_table;
        }
        break;

		default:
			break;
	}

	pthread_mutex_unlock(&m_lock);

	return res;
}

DVR_RESULT DvrGuiResHandler::GetLayoutInfo(DVR_U32 id, DVR_GRAPHIC_UIOBJ **ppTable, int *pObjNum)
{
	if (id >= GUI_LAYOUT_NUM || ppTable == NULL || pObjNum == NULL)
		return DVR_RES_EINVALIDARG;

	switch(id)
	{
		case GUI_LAYOUT_RECORD:
		{
			*ppTable = rec_gui_table;
			*pObjNum = sizeof(rec_gui_table) / sizeof(rec_gui_table[0]);
		}
		break;

		case GUI_LAYOUT_THUMB:
		{
			*ppTable = thumb_gui_table;
			*pObjNum = sizeof(thumb_gui_table) / sizeof(thumb_gui_table[0]);
		}
		break;

		case GUI_LAYOUT_PLAYBACK_VIDEO:
		{
			*ppTable = pb_video_gui_table;
			*pObjNum = sizeof(pb_video_gui_table) / sizeof(pb_video_gui_table[0]);
		}
		break;

		case GUI_LAYOUT_PLAYBACK_IMAGE:
		{
			*ppTable = pb_image_gui_table;
			*pObjNum = sizeof(pb_image_gui_table) / sizeof(pb_image_gui_table[0]);
		}
		break;

		case GUI_LAYOUT_SETUP:
		{
			*ppTable = setup_gui_table;
			*pObjNum = sizeof(setup_gui_table) / sizeof(setup_gui_table[0]);
		}

		default:
			break;
	}

	return DVR_RES_SOK;
}

DVR_RESULT DvrGui_GetLayoutInfo(DVR_U32 id, DVR_GRAPHIC_UIOBJ **ppTable, int *pObjNum)
{
	if (id >= GUI_LAYOUT_NUM || ppTable == NULL || pObjNum == NULL)
		return DVR_RES_EINVALIDARG;

	switch(id)
	{
		case GUI_LAYOUT_RECORD:
		{
			*ppTable = rec_gui_table;
			*pObjNum = sizeof(rec_gui_table) / sizeof(rec_gui_table[0]);
		}
		break;

		case GUI_LAYOUT_THUMB:
		{
			*ppTable = thumb_gui_table;
			*pObjNum = sizeof(thumb_gui_table) / sizeof(thumb_gui_table[0]);
		}
		break;

		case GUI_LAYOUT_PLAYBACK_VIDEO:
		{
			*ppTable = pb_video_gui_table;
			*pObjNum = sizeof(pb_video_gui_table) / sizeof(pb_video_gui_table[0]);
		}
		break;

		case GUI_LAYOUT_PLAYBACK_IMAGE:
		{
			*ppTable = pb_image_gui_table;
			*pObjNum = sizeof(pb_image_gui_table) / sizeof(pb_image_gui_table[0]);
		}
		break;

		case GUI_LAYOUT_SETUP:
		{
			*ppTable = setup_gui_table;
			*pObjNum = sizeof(setup_gui_table) / sizeof(setup_gui_table[0]);
		}

		default:
			break;
	}

	return DVR_RES_SOK;
}

