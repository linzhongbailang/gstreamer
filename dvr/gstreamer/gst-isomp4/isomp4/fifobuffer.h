
#ifndef _FIFO_BUFFER_H_
#define _FIFO_BUFFER_H_

#include <stdint.h>

typedef struct  
{
	uint8_t		*buffer;
	uint32_t	sizeInBytes;
	uint32_t	bytesInBuffer;
	uint32_t	framesInBuffer;
	uint32_t	bufferPos;
}FIFOBuffer;

uint8_t *getPtrBegin(FIFOBuffer *pBuffer);
uint8_t *getPtrEnd(FIFOBuffer *pBuffer, uint32_t slackCapacity);
void putFrame(FIFOBuffer *pBuffer, uint32_t *data, uint32_t bytes);
void clear(FIFOBuffer *pBuffer);
int isEmpty(FIFOBuffer *pBuffer);
uint32_t getNumBytes(FIFOBuffer *pBuffer);
int32_t FIFOBuffer_Init(FIFOBuffer *pBuffer);
int32_t FIFOBuffer_DeInit(FIFOBuffer *pBuffer);

#endif