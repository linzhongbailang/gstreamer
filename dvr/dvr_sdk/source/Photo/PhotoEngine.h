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
#ifndef _PHOTO_ENGINE_H_
#define _PHOTO_ENGINE_H_

#include "DVR_PHOTO_DEF.h"
#include "Encoder_libjpeg.h"

class CPhotoEngine 
{
public:
	CPhotoEngine(void *pParentEngine, CThumbNail *pTNHandle);
	~CPhotoEngine();

	static int Photo_Process(DVR_U32 param1, DVR_U32 param2, void *pContext);
	static int Photo_Return(void *pContext);

	void InitParam(int index, PHOTO_INPUT_PARAM *pParam, DVR_PHOTO_TYPE type);
	int Encode(void);
	int Set(PHOTO_PROP_ID ePropId, void *pPropData, int nPropSize);
	int Get(PHOTO_PROP_ID ePropId, void *pPropData, int nPropSize);
	int SetOsdTitle(DVR_U8* pImgBuf, PHOTO_INPUT_PARAM *pParam);
	int CreatePhotoFile(gchar *time, int direction, guint file_index, DVR_PHOTO_TYPE type);

	void *ParentEngine()
	{
		return m_pParentEngine;
	}

	void SetDBFunction(PFN_DBADDFILE pFunc)
	{
		m_pFnDB_AddFile = pFunc;
	}

	gchar *GetMainOutBuf();
	gchar *GetTNOutBuf();

private:
	gboolean m_bHasExifData;
	CAMERA_EXIF m_ExifData;

private:
	CPhotoEngine(const CPhotoEngine&);
	const CPhotoEngine& operator = (const CPhotoEngine&);

	int ClosePhotoFile();

	gchar *m_pPhotoDir;
	guint photo_folder_max_file_index;

	gchar m_MediaFileName[128];
	FILE *m_pFileDescriptor;
	char *m_pImgBuf;

    gchar m_TNFileName[128];

	PFN_DBADDFILE m_pFnDB_AddFile;

	Encoder_libjpeg *m_pEncoder;
	Encoder_libjpeg::params m_MainJpeg;
	Encoder_libjpeg::params m_TNJpeg;

	gchar *m_pMainOutBuf;
	gchar *m_pTNOutBuf;
	ExifElementsTable *m_pExifData;

	PHOTO_OUT_MEMORY* m_pOut;

private:
	void *m_pParentEngine;
	CThumbNail *m_pThumbNail;
};

#endif
