#ifndef _CCYCLE_BUFFER_CFG_
#define _CCYCLE_BUFFER_CFG_
#include <stdio.h>
#include <gst/canbuf/canbuf.h>

#define FRAME_TYPE_P           (0)
#define FRAME_TYPE_I           (1)
#define FRAME_TYPE_IDR         (2)
#define MAX_QUE_FRAME_NUM      (500)
#define MAX_QUE_FRAME_SIZE     (500*1024)
 
typedef struct 
{
  unsigned int      curRd;
  unsigned int      curWr;
  unsigned int      elementSize;
  unsigned int      maxElements;
  unsigned int      bufferSize;
  unsigned int      bufferoff;
  unsigned char*    frameBuffer;
}Frame_QueHeader;

typedef struct 
{
    unsigned long long  nTimeStamp;
    unsigned int        nFrameLen;
    unsigned short      nCanLen;
    unsigned char       nFrameType;
}Frame_Info;

typedef struct 
{
    Frame_Info          FrameInfo;
    unsigned char*      nDataPos;
    unsigned char*      nCanPos;
}FrameNode;

typedef struct 
{
    Frame_QueHeader QueHeader;
    FrameNode       queElements[MAX_QUE_FRAME_NUM];
}CCycleBufferQue;

class CCycleBuffer
{
    public:
        CCycleBuffer(unsigned int buffersize);
        virtual ~CCycleBuffer(void);
        int     FrameReset(void);
        int     FrameWrite(unsigned char* data, unsigned char* candata, Frame_Info* info);
        int     FrameRead(FrameNode *node);
        int     FrameSearch(FrameNode *node, int startpos);
    public:
        void        SetReadPos(unsigned int pos);
        unsigned int GetWritePos(void);
        unsigned int GetReadPos(void);

   private:
        CCycleBufferQue m_CCBufferQue;
};
#endif//_CCYCLE_BUFFER_CFG_

