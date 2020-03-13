
#ifndef _BITSTREAM_H_
#define _BITSTREAM_H_

#include <stdint.h>

typedef struct
{
	uint8_t*  pu8DataBegin;
	uint32_t  u32DataLength;
	uint8_t*  pu8CurrReadPtr;
	int32_t   s32BytesLeft;
}BitStream;

int32_t BitStreamAttach(BitStream* pBitStream, int8_t* ps8DataBuffer, uint32_t u32DataBufLength);
int32_t BitStreamSeek(BitStream* pBitStream, uint32_t u32SeekBufLength);
uint32_t GetBuffer(BitStream* pBitStream, uint8_t *buf, uint32_t u32Bytes);
uint32_t SkipBuffer(BitStream* pBitStream, uint32_t u32Bytes);

#endif  // ~_BITSTREAM_H_
