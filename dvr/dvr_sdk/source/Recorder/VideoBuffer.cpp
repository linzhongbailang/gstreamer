#include "VideoBuffer.h"
#include "CycleBuffer.h"


#if !defined(DEFAULT_MAX_BUFFER_SIZE_LIMIT)
#define DEFAULT_MAX_BUFFER_SIZE_LIMIT   20*1024*1024  // 20MB
#endif

CVideoBuffer::CVideoBuffer(void)
{
    m_pCycleBuffer         = NULL;
    m_bMonitorExit         = TRUE;
    m_PreRecordLimitTime   = 0;
    m_LastTimeStamp        = 0;
    m_StartframePos        = 0;
    m_StartTimeStamp       = 0;
    m_bEnableMonitorDetect = FALSE;
}

CVideoBuffer::~CVideoBuffer(void)
{
    Destroy();
}

void CVideoBuffer::Create(void)
{
    if(m_pCycleBuffer != NULL)
        return;

    m_bMonitorExit = FALSE;
    m_pCycleBuffer = new CCycleBuffer(DEFAULT_MAX_BUFFER_SIZE_LIMIT);
    int res = DVR::OSA_tskCreate(&m_hThread_Monitor, MonitorTaskEntry, this);
    if (DVR_FAILED(res))
    {
        GST_ERROR("CVideoBuffer Create Task failed = 0x%x\n", res);
    }
}

void CVideoBuffer::Destroy(void)
{
    if(m_pCycleBuffer != NULL)
    {
        delete m_pCycleBuffer;
        m_pCycleBuffer = NULL;
    }
    m_bMonitorExit = TRUE;
    DVR::OSA_tskDelete(&m_hThread_Monitor);
}

int CVideoBuffer::Start(void)
{
    m_bEnableMonitorDetect = FALSE;
    return DVR_RES_SOK;
}

int CVideoBuffer::Stop(void)
{
    return DVR_RES_SOK;
}

int CVideoBuffer::Reset(void)
{
    if(m_pCycleBuffer == NULL)
        return DVR_RES_EFAIL;
    
    m_LastTimeStamp         = 0;
    m_StartframePos         = 0;
    m_StartTimeStamp        = 0;
    m_bEnableMonitorDetect  = TRUE;
    m_pCycleBuffer->FrameReset();
    return DVR_RES_SOK;
}

int CVideoBuffer::GetFrame(FrameNode *node, bool isFirstFrame)
{
    if(node == NULL)
        return DVR_RES_EFAIL;

    if(isFirstFrame && m_StartTimeStamp != 0)
    {
        m_pCycleBuffer->SetReadPos(m_StartframePos);
    }
    return (m_pCycleBuffer->FrameRead(node) < 0) ? DVR_RES_EFAIL : DVR_RES_SOK;
}

int CVideoBuffer::AddVideoFrame(GstBuffer *buffer)
{
    GstMapInfo map;
    guint64 pts, dts;
    gboolean bIsKeyFrame;

    Ofilm_Can_Data_T *pCan = NULL;
#ifdef __linux__ 
    GstMetaCanBuf *canbuf = gst_buffer_get_can_buf_meta(buffer);
    if (canbuf != NULL)
    {
        pCan = gst_can_buf_meta_get_can_data(canbuf);
    }
#endif

    pts = GST_BUFFER_PTS(buffer)/1000;
    dts = GST_BUFFER_DTS(buffer)/1000;
    if(m_LastTimeStamp == pts)
    {
        gst_buffer_unref(buffer);
        return DVR_RES_SOK;
    }

    bIsKeyFrame = !GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT);

    if (gst_buffer_map(buffer, &map, GST_MAP_READ))
    {
        if (m_pCycleBuffer != NULL)
        {
            Frame_Info info;
            info.nTimeStamp = pts;
            info.nFrameLen  = map.size;
            info.nCanLen    = sizeof(Ofilm_Can_Data_T);
            info.nFrameType = bIsKeyFrame ? FRAME_TYPE_I : FRAME_TYPE_P;
            m_pCycleBuffer->FrameWrite(map.data, (guchar *)pCan, &info);
        }

        gst_buffer_unmap(buffer, &map);
    }

    gst_buffer_unref(buffer);
    m_LastTimeStamp = pts;

    return DVR_RES_SOK;
}


void CVideoBuffer::MonitorTaskEntry(void *ptr)
{
    FrameNode node;
    int curRdpos = 0;
    CVideoBuffer* p = (CVideoBuffer*)ptr;
    while (p->m_bMonitorExit == FALSE)
    {
        OSA_Sleep(40);
       
        if (p->m_bEnableMonitorDetect)
        {
            curRdpos = p->m_pCycleBuffer->FrameRead(&node);
            if(curRdpos < 0)
            {
                continue;
            }

            if(node.FrameInfo.nFrameType == FRAME_TYPE_P)
                continue;

            if(p->m_StartTimeStamp == 0)
            {
                p->m_StartframePos  = curRdpos;
                p->m_StartTimeStamp = node.FrameInfo.nTimeStamp;
                continue;
            }

            if(node.FrameInfo.nTimeStamp - p->m_StartTimeStamp > p->m_PreRecordLimitTime*1000)
            {
                curRdpos = p->m_pCycleBuffer->FrameSearch(&node, p->m_StartframePos + 1);
                while(node.FrameInfo.nFrameType != FRAME_TYPE_I)
                {
                    curRdpos = p->m_pCycleBuffer->FrameSearch(&node, curRdpos + 1);
                    if(curRdpos == -1)
                        break;
                }
                if(curRdpos != -1)
                {
                    p->m_StartframePos  = curRdpos;
                    p->m_StartTimeStamp = node.FrameInfo.nTimeStamp;
                    //GST_ERROR("MonitorTaskEntry m_StartframePos %d curWdpos %d m_StartTimeStamp %lld!!!!! \n",curRdpos,p->m_pCycleBuffer->GetWritePos(),p->m_StartTimeStamp);
                }
                else
                {
                    GST_ERROR("MonitorTaskEntry------------ curRdpos %d\n",curRdpos);
                }
            }
        }
    }
}

