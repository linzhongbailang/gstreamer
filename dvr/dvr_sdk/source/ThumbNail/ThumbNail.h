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
#ifndef _THUMBNAIL_H_
#define _THUMBNAIL_H_

#include <gst/gst.h>
#include <gst/video/video.h>
#include "DVR_SDK_DEF.h"

typedef struct  
{
	guchar *pInputImage;
	GstVideoFormat format;
	gint width;
	gint height;
}TN_INPUT_PARAM;

typedef enum
{
	THUMBNAIL_PROP_INFRAME,
}THUMBNAIL_PROP_ID;

class CThumbNail
{
public:
	CThumbNail(void *handle);
	~CThumbNail();

	static int ThumbNail_Process(DVR_U32 param1, DVR_U32 param2, void *pContext);

	void *RecorderEngine()
	{
		return m_pRecorderEngine;
	}

	guchar *OutBuf()
	{
		return m_pOutBuf;
	}

	guchar *InBuf()
	{
		return m_ImgBuf;
	}
	int OutBufSize()
	{
		return m_OutBufSize;
	}

    int ThumbNailWidth()
    {
        return m_TnWidth;
    }

    int ThumbNailHeight()
    {
        return m_TnHeight;
    }

	int Set(THUMBNAIL_PROP_ID ePropId, void *pPropData, int nPropSize);
	int Get(THUMBNAIL_PROP_ID ePropId, void *pPropData, int nPropSize);

private:
	int		m_TnWidth;
	int		m_TnHeight;
	char	m_formatName[8];
	guchar*	m_pOutBuf;
	int		m_OutBufSize;
#ifdef VPE_HW_ACCEL
	void*	m_pVpe;
#else
    void*   m_pSwScale;
#endif
	GMutex  m_Mutex;
    guchar  m_ImgBuf[1280*720*3/2];

private:
	void*	m_pRecorderEngine;
};

#endif