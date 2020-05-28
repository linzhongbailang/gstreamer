#include <stdlib.h>
#include <string.h>
#include "ThumbNail.h"

#ifdef VPE_HW_ACCEL
#include "Vpe.h"
#else
#include "SwScale.h" 
#endif

CThumbNail::CThumbNail(void *handle)
{
	m_TnWidth = DVR_THUMBNAIL_WIDTH;
	m_TnHeight = DVR_THUMBNAIL_HEIGHT;

	memset(m_formatName, 0, sizeof(m_formatName));
	strcpy(m_formatName, "rgb24");

	m_OutBufSize = DVR_THUMBNAIL_WIDTH * DVR_THUMBNAIL_HEIGHT * 3;
	m_pOutBuf = (guchar *)malloc(m_OutBufSize);
	if (m_pOutBuf)
		memset(m_pOutBuf, 0, m_OutBufSize);

	m_pRecorderEngine = handle;

#ifdef VPE_HW_ACCEL
	m_pVpe = new Vpe;
#else
    m_pSwScale = new SwScale;
#endif
	g_mutex_init(&m_Mutex);
}
	
CThumbNail::~CThumbNail()
{
	if (m_pOutBuf)
	{
		free(m_pOutBuf);
		m_pOutBuf = NULL;
	}
	
#ifdef VPE_HW_ACCEL
	delete (Vpe*)m_pVpe;
#else
    delete (SwScale *)m_pSwScale;
#endif

	g_mutex_clear(&m_Mutex);
}

int CThumbNail::ThumbNail_Process(DVR_U32 param1, DVR_U32 param2, void *pContext)
{
	CThumbNail *pHandle = (CThumbNail*)pContext;
	if (pHandle == NULL)
	{
		return DVR_RES_EPOINTER;
	}

	g_mutex_lock(&pHandle->m_Mutex);

#ifdef VPE_HW_ACCEL
	Vpe *vpeHandle = (Vpe*)(pHandle->m_pVpe);
	int ret = vpeHandle->imgProcess();
#else 
    SwScale *pSwScaleHandle = (SwScale*)(pHandle->m_pSwScale);
    int ret = pSwScaleHandle->imgProcess();
#endif
	g_mutex_unlock(&pHandle->m_Mutex);

	return ret;
}

int CThumbNail::Set(THUMBNAIL_PROP_ID ePropId, void *pPropData, int nPropSize)
{
	int ret = 0;

	switch (ePropId)
	{
	case THUMBNAIL_PROP_INFRAME:
	{
		if (pPropData == NULL || nPropSize != sizeof(TN_INPUT_PARAM))
		{
			GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		TN_INPUT_PARAM *pInFrame = (TN_INPUT_PARAM *)pPropData;
		
		if(pInFrame->pInputImage != NULL)
		{
			memcpy(m_ImgBuf, pInFrame->pInputImage, pInFrame->width*pInFrame->height*3/2);
		}
		
#ifdef VPE_HW_ACCEL
		Vpe *vpeHandle = (Vpe*)m_pVpe;

		vpeHandle->m_InputData.width = pInFrame->width;
		vpeHandle->m_InputData.height = pInFrame->height;
		if (pInFrame->format == GST_VIDEO_FORMAT_NV12)
		{
			vpeHandle->m_InputData.dataSize = pInFrame->width*pInFrame->height * 3 / 2;
			memset(vpeHandle->m_InputData.formatName, 0, sizeof(vpeHandle->m_InputData.formatName));
			strcpy(vpeHandle->m_InputData.formatName, "nv12");
		}
		else
		{
			//currently the input always is NV12
		}
				
		vpeHandle->m_InputData.pDataBuffer = m_ImgBuf;
		
		vpeHandle->m_OutputData.width = m_TnWidth;
		vpeHandle->m_OutputData.height = m_TnHeight;
		memset(vpeHandle->m_OutputData.formatName, 0, sizeof(vpeHandle->m_OutputData.formatName));
		strcpy(vpeHandle->m_OutputData.formatName, m_formatName);
		vpeHandle->m_OutputData.dataSize = m_OutBufSize;

		vpeHandle->m_OutputData.pDataBuffer = m_pOutBuf;
#else
        SwScale *pSwScaleHandle = (SwScale*)m_pSwScale;

        pSwScaleHandle->m_InputData.width = pInFrame->width;
        pSwScaleHandle->m_InputData.height = pInFrame->height;
        if (pInFrame->format == GST_VIDEO_FORMAT_NV12)
        {
            pSwScaleHandle->m_InputData.dataSize = pInFrame->width*pInFrame->height * 3 / 2;
            pSwScaleHandle->m_InputData.pixfmt = AV_PIX_FMT_NV12;
        }
        else
        {
            //currently the input always is NV12
        }

        pSwScaleHandle->m_InputData.pDataBuffer = m_ImgBuf;

        pSwScaleHandle->m_OutputData.width = m_TnWidth;
        pSwScaleHandle->m_OutputData.height = m_TnHeight;
        pSwScaleHandle->m_OutputData.dataSize = m_OutBufSize;
        pSwScaleHandle->m_OutputData.pDataBuffer = m_pOutBuf;
        pSwScaleHandle->m_OutputData.pixfmt = AV_PIX_FMT_RGB24;
#endif
	}
	break;

	default:
		break;
	}

	return ret;
}

int CThumbNail::Get(THUMBNAIL_PROP_ID ePropId, void *pPropData, int nPropSize)
{
	int ret = 0;

	switch (ePropId)
	{
	default:
		break;
	}

	return ret;
}
