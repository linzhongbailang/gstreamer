#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DVR_SDK_DEF.h"
#include "CycleBuffer.h"
#include <dprint.h>



CCycleBuffer::CCycleBuffer(unsigned int buffersize)
{
    memset(&m_CCBufferQue, 0, sizeof(CCycleBufferQue));
    Frame_QueHeader *pHdr;

    pHdr = (Frame_QueHeader *)&m_CCBufferQue.QueHeader;

    /* set values in local handle */
    pHdr->elementSize = MAX_QUE_FRAME_SIZE;
    pHdr->maxElements = MAX_QUE_FRAME_NUM;

    /* reset read and write index */
    pHdr->curRd = 0;
    pHdr->curWr = 0;
    pHdr->frameBuffer = (unsigned char *)malloc(buffersize);
	if(pHdr->frameBuffer == NULL)
		DPrint(DPRINT_ERR, "CCycleBuffer malloc failed buffersize:%d !!!!!!!!!!!!!!!!!!!!!\n",buffersize);
    pHdr->bufferSize  = buffersize;
}

CCycleBuffer::~CCycleBuffer(void)
{
    Frame_QueHeader *pHdr;

    pHdr = (Frame_QueHeader *)&m_CCBufferQue.QueHeader;
    if(pHdr->frameBuffer != NULL)
    {
        free(pHdr->frameBuffer);
        pHdr->frameBuffer = NULL;
    }
    FrameReset();
}

int CCycleBuffer::FrameReset(void)
{
    Frame_QueHeader *pHdr;
   
    pHdr = (Frame_QueHeader *)&m_CCBufferQue.QueHeader;

    if ((pHdr->elementSize == 0) || (pHdr->maxElements == 0))
    {
        return DVR_RES_EFAIL;
    }

    pHdr->curRd = 0;
    pHdr->curWr = 0;
    pHdr->bufferoff = 0;
    return DVR_RES_SOK;
}

unsigned int CCycleBuffer::GetWritePos(void)
{
    Frame_QueHeader *pHdr;
   
    pHdr = (Frame_QueHeader *)&m_CCBufferQue.QueHeader;
        
    return pHdr->curWr;
}

unsigned int CCycleBuffer::GetReadPos(void)
{
    Frame_QueHeader *pHdr;
   
    pHdr = (Frame_QueHeader *)&m_CCBufferQue.QueHeader;
        
    return pHdr->curRd;
}

void CCycleBuffer::SetReadPos(unsigned int pos)
{
    Frame_QueHeader *pHdr;
   
    pHdr = (Frame_QueHeader *)&m_CCBufferQue.QueHeader;
    pHdr->curRd = pos%pHdr->maxElements;
}

int CCycleBuffer::FrameWrite(unsigned char* data, unsigned char* candata, Frame_Info* info)
{
    Frame_QueHeader *pHdr;
    FrameNode *pWrite;
    unsigned int writeIdx;

    if (NULL == data || candata == NULL || info == NULL)
    {
    	
        return -1;
    }

    pHdr = (Frame_QueHeader *)&m_CCBufferQue.QueHeader;
    
    if (info->nFrameLen > pHdr->elementSize)
    {
        return -1;
    }
    
    if (((pHdr->curWr + 1)%pHdr->maxElements) == pHdr->curRd)
    {
        return -1;
    }
    
    writeIdx = pHdr->curWr;

    if(writeIdx < pHdr->maxElements)
    {
        pWrite = (FrameNode*)&m_CCBufferQue.queElements[writeIdx];
        unsigned int writesize = info->nFrameLen + info->nCanLen;
        if(pHdr->bufferoff + writesize > pHdr->bufferSize)
        {
            pHdr->bufferoff = 0;
        }
        pWrite->nDataPos = pHdr->frameBuffer + pHdr->bufferoff;
        pWrite->nCanPos  = pWrite->nDataPos + info->nFrameLen;

        memcpy(pWrite->nDataPos, data, info->nFrameLen);
        memcpy(pWrite->nCanPos,  candata, info->nCanLen);
        memcpy(&pWrite->FrameInfo, info, sizeof(Frame_Info));
        pHdr->bufferoff += writesize;

        pHdr->curWr = (writeIdx + 1)%pHdr->maxElements;

        writeIdx = pHdr->curWr;
    }
    else
    {
        return -1;
    }

    return (writeIdx == 0) ? 0: writeIdx - 1;
}

int CCycleBuffer::FrameRead(FrameNode *node)
{
    Frame_QueHeader *pHdr;
    FrameNode *pRead;
    unsigned int  readIdx;

    if (NULL == node)
    {
        return -1;
    }

    pHdr = (Frame_QueHeader *)&m_CCBufferQue.QueHeader;
    
    if (pHdr->curWr == pHdr->curRd)
    {
        return -1;
    }
    
    readIdx = pHdr->curRd;
    if (readIdx < pHdr->maxElements)
    {
        pRead = (FrameNode*)&m_CCBufferQue.queElements[readIdx];

        memcpy(node, pRead, sizeof(FrameNode));

        pHdr->curRd = (readIdx + 1)%pHdr->maxElements;

        readIdx = pHdr->curRd;
    }
    else
    {
          return -1;
    }
    
    return (readIdx == 0) ? 0: readIdx - 1;
}

int CCycleBuffer::FrameSearch(FrameNode *node, int startpos)
{
    Frame_QueHeader *pHdr;
    FrameNode       *pRead;
    unsigned int    readIdx;

    if (NULL == node)
    {
        return -1;
    }
    
    pHdr = (Frame_QueHeader *)&m_CCBufferQue.QueHeader;

    readIdx = startpos%pHdr->maxElements;
    if (readIdx < pHdr->maxElements)
    {
        pRead = (FrameNode*)&m_CCBufferQue.queElements[readIdx];
        memcpy(node, pRead, sizeof(FrameNode));
    }
    else
    {
        return -1;
    }

    return readIdx;
}

