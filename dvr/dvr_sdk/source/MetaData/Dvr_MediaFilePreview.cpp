#include <windows.h>
#include <stdio.h>
#include <gst/gst.h>
//#include "DVR_PLAYER_DEF.h"
#include "DVR_METADATA_INTFS.h"

static DWORD CalculateBufferSize(DVR_PREVIEW_OPTION *pPreviewOpt)
{
	if (pPreviewOpt == NULL)
	{
		return 0;
	}
	DWORD dwSize = 0;

	if (pPreviewOpt->format == PREVIEW_RGB565)
	{
		dwSize = pPreviewOpt->ulPreviewHeight * pPreviewOpt->ulPreviewWidth * 2;
	}
	else if (pPreviewOpt->format == PREVIEW_RGB888)
	{
		dwSize = pPreviewOpt->ulPreviewHeight * pPreviewOpt->ulPreviewWidth * 3;
	}
	else if (pPreviewOpt->format == PREVIEW_RGB32)
	{
		dwSize = pPreviewOpt->ulPreviewHeight * pPreviewOpt->ulPreviewWidth * 4;
	}
	else if (pPreviewOpt->format == PREVIEW_YUV420)
	{
		dwSize = pPreviewOpt->ulPreviewHeight * pPreviewOpt->ulPreviewWidth * 3 / 2;
	}
	else if (pPreviewOpt->format == PREVIEW_YUV422)
	{
		dwSize = pPreviewOpt->ulPreviewHeight * pPreviewOpt->ulPreviewWidth * 2;
	}
	return dwSize;
}


static BOOL CheckSupportedFormat(DVR_PREVIEW_FORMAT format)
{
	switch (format)
	{
	case PREVIEW_RGB565:
	case PREVIEW_RGB888:
	case PREVIEW_RGB32:
	case PREVIEW_YUV420:
	case PREVIEW_YUV422:
		return TRUE;
	default:
		break;
	}

	return FALSE;
}

DVR_RESULT Dvr_Get_MediaFilePreview(DVR_PREVIEW_OPTION *pPreviewOpt, unsigned char *pBuf, unsigned int *cbSize)
{
	DVR_RESULT res = DVR_RES_SOK;

	if (cbSize == NULL || pPreviewOpt == NULL)
	{
		return DVR_RES_EINVALIDARG;
	}

	if (!CheckSupportedFormat(pPreviewOpt->format))
	{
		return DVR_RES_EMODENOTSUPPORTED;
	}
	// When pBuf is NULL, buffer size will be returned.
	if (pBuf == NULL)
	{
		*cbSize = CalculateBufferSize(pPreviewOpt);
		return DVR_RES_SOK;
	}

	if (*cbSize < CalculateBufferSize(pPreviewOpt))
	{
		return DVR_RES_EINVALIDARG;
	}

	if (strcmp(pPreviewOpt->filename, "") == 0)
	{
		GST_ERROR("NULL filename:%s", pPreviewOpt->filename);
		return DVR_RES_EFAIL;
	}

	void *hMetaData = NULL;
	DVR_U32 u32MDItemNum;
	const DVR_METADATA_ITEM *pItem;

	hMetaData = Dvr_MetaData_Create(pPreviewOpt->filename, &u32MDItemNum, false);
	if (hMetaData == NULL)
	{
		GST_ERROR("No Meta Data!");
		return DVR_RES_EFAIL;
	}

	if (g_str_has_suffix(pPreviewOpt->filename, "MP4") || g_str_has_suffix(pPreviewOpt->filename, "mp4"))
	{
		pItem = Dvr_MetaData_GetDataByType(hMetaData, DVR_METADATA_TYPE_PICTURE);
		if (pItem != NULL)
		{
			int pngStartFlagLength = 8;
			int pngEndFlagLength = 12;
			unsigned int thumbnail_length = pItem->u32DataSize - pngStartFlagLength - pngEndFlagLength;
			if (thumbnail_length > *cbSize)
			{
				GST_ERROR("thumbnail length too large[%d > %d]", thumbnail_length, *cbSize);
				res = DVR_RES_EFAIL;
				goto exit;
			}
			memcpy(pBuf, pItem->ps8Data + pngStartFlagLength, thumbnail_length);
			*cbSize = thumbnail_length;
		}
	}
	else if (g_str_has_suffix(pPreviewOpt->filename, "JPG") || g_str_has_suffix(pPreviewOpt->filename, "jpg"))
	{	
		pItem = Dvr_MetaData_GetDataByType(hMetaData, DVR_METADATA_TYPE_COMMENT);
		if (pItem != NULL)
		{
			guint thumbnail_offset = 0, thumbnail_length = 0;

			gchar **tokens, **walk;
			tokens = g_strsplit(pItem->ps8Data, "/", 0);
			if (g_strv_length(tokens) != 2)
			{
				GST_ERROR("Invalid metadata[%s] for thumbnail", pItem->ps8Data);
				g_strfreev(tokens);
				res = DVR_RES_EFAIL;
				goto exit;
			}

			walk = tokens;
			thumbnail_offset = strtol(*walk++, NULL, 0);
			thumbnail_length = strtol(*walk, NULL, 0);
			g_strfreev(tokens);
			GST_INFO("Get thumbnail offset:%d, length:%d", thumbnail_offset, thumbnail_length);

			if (thumbnail_length > *cbSize || thumbnail_length == 0)
			{
				GST_ERROR("thumbnail length too large or too small[%d]", thumbnail_length);
				res = DVR_RES_EFAIL;
				goto exit;
			}

			FILE *fp = fopen(pPreviewOpt->filename, "rb");
			if (fp == NULL)
			{
				GST_ERROR("Failed to open preview file %s", pPreviewOpt->filename);
				res = DVR_RES_EFAIL;
				goto exit;
			}

			fseek(fp, 0L, SEEK_END);
			guint file_size = ftell(fp);
			fseek(fp, 0L, SEEK_SET);
			if (thumbnail_offset > file_size || thumbnail_length > file_size || (thumbnail_offset + thumbnail_length) > file_size)
			{
				GST_ERROR("Invalid thumbnail offset and length");
				res = DVR_RES_EFAIL;
				goto exit;
			}
			fseek(fp, thumbnail_offset, SEEK_SET);
			fread(pBuf, 1, thumbnail_length, fp);
			fclose(fp);
			*cbSize = thumbnail_length;
		}	
	}
	else
	{
		//TODO
	}

exit:
	Dvr_MetaData_Destroy(hMetaData);

	return res;
}