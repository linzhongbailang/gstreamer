
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include "fifobuffer.h"


// Returns the current buffer capacity in terms of samples
uint32_t getCapacity(FIFOBuffer *pBuffer)
{
	return pBuffer->sizeInBytes;
}

// Ensures that the buffer has enough capacity, i.e. space for _at least_
// 'capacityRequirement' number of samples. The buffer is grown in steps of
// 4 kilobytes to eliminate the need for frequently growing up the buffer,
// as well as to round the buffer size up to the virtual memory page size.
int32_t ensureCapacity(FIFOBuffer *pBuffer, uint32_t capacityRequirement)
{
	uint8_t *temp;

	if (capacityRequirement > getCapacity(pBuffer))
	{
		// enlarge the buffer in 4kbyte steps (round up to next 4k boundary)
		pBuffer->sizeInBytes = (capacityRequirement + 4095) & (uint32_t)-4096;
		temp = (uint8_t *)malloc(pBuffer->sizeInBytes);
		if (temp == NULL)
		{
			return -1;
		}

		if (pBuffer->bytesInBuffer)
		{
			memcpy(temp, getPtrBegin(pBuffer), pBuffer->bytesInBuffer);
		}
		if (pBuffer->buffer)
		{
			free(pBuffer->buffer);
			pBuffer->buffer = NULL;
		}
		pBuffer->buffer = temp;
		pBuffer->bufferPos = 0;
	} 

	return 0;
}


// Returns a pointer to the beginning of the currently non-outputted samples. 
// This function is provided for accessing the output samples directly. 
// Please be careful!
//
// When using this function to output samples, also remember to 'remove' the
// outputted samples from the buffer by calling the 
// 'receiveSamples(numSamples)' function
uint8_t *getPtrBegin(FIFOBuffer *pBuffer)
{
	return (pBuffer->buffer + pBuffer->bufferPos);
}


// Returns a pointer to the end of the used part of the sample buffer (i.e. 
// where the new samples are to be inserted). This function may be used for 
// inserting new samples into the sample buffer directly. Please be careful! 
//
// Parameter 'slackCapacity' tells the function how much free capacity (in
// terms of samples) there _at least_ should be, in order to the caller to
// successfully insert all the required samples to the buffer. When necessary, 
// the function grows the buffer size to comply with this requirement.
//
// When using this function as means for inserting new samples, also remember 
// to increase the sample count afterwards, by calling  the 
// 'putFrame(numSamples)' function.
uint8_t *getPtrEnd(FIFOBuffer *pBuffer, uint32_t slackCapacity)
{
	ensureCapacity(pBuffer, pBuffer->bytesInBuffer + slackCapacity);
	return pBuffer->buffer + pBuffer->bytesInBuffer;
}

void putFrame(FIFOBuffer *pBuffer, uint32_t *data, uint32_t bytes)
{
	memcpy(getPtrEnd(pBuffer, bytes), data, bytes);
	pBuffer->bytesInBuffer += bytes;
}


// Returns the number of bytes currently in the buffer
uint32_t getNumBytes(FIFOBuffer *pBuffer)
{
	return pBuffer->bytesInBuffer;
}

// Returns nonzero if the sample buffer is empty
int isEmpty(FIFOBuffer *pBuffer)
{
	return (pBuffer->bytesInBuffer == 0) ? 1 : 0;
}

// Clears the sample buffer
void clear(FIFOBuffer *pBuffer)
{
	pBuffer->bytesInBuffer = 0;
	pBuffer->bufferPos = 0;
	pBuffer->framesInBuffer = 0;
}

int32_t FIFOBuffer_Init(FIFOBuffer *pBuffer)
{
	int32_t nRet;

	pBuffer->sizeInBytes = 0;
	pBuffer->buffer = NULL;
	pBuffer->bytesInBuffer = 0;
	pBuffer->framesInBuffer = 0;
	pBuffer->bufferPos = 0;
	nRet = ensureCapacity(pBuffer, 8192); //allocate initial capacity 8KB

	return nRet;
}

int32_t FIFOBuffer_DeInit(FIFOBuffer *pBuffer)
{
	if (pBuffer->buffer)
	{
		free(pBuffer->buffer);
		pBuffer->buffer = NULL;
	}

	return 0;
}