
#include <string.h>
#include <stdint.h>
#include "bitstream.h"


int32_t BitStreamAttach(BitStream* pBitStream, int8_t* ps8DataBuffer, uint32_t u32DataBufLength)
{
	register uint32_t  mask = 0x00ff00ff;
	register uint32_t  temp;
	
	if( (pBitStream==0) || (ps8DataBuffer==0) || (u32DataBufLength==0) )
		return -1;
	
	pBitStream->pu8DataBegin = (uint8_t*)ps8DataBuffer;
	pBitStream->u32DataLength = u32DataBufLength;
	pBitStream->pu8CurrReadPtr = (uint8_t*)ps8DataBuffer;
	pBitStream->s32BytesLeft = u32DataBufLength;

	return 0;
}

int32_t BitStreamSeek(BitStream* pBitStream, uint32_t u32SeekBufLength)
{
	pBitStream->pu8CurrReadPtr = (uint8_t*)pBitStream->pu8DataBegin + u32SeekBufLength;
	pBitStream->s32BytesLeft = pBitStream->u32DataLength - u32SeekBufLength;

	return 0;
}

uint32_t GetBuffer(BitStream* pBitStream, uint8_t *buf,uint32_t u32Bytes)
{
	if (pBitStream->s32BytesLeft < (int32_t)u32Bytes)
		return 0;

	memcpy(buf, pBitStream->pu8CurrReadPtr, u32Bytes);
	pBitStream->pu8CurrReadPtr += u32Bytes;
	pBitStream->s32BytesLeft -= u32Bytes;

	return u32Bytes;
}

uint32_t SkipBuffer(BitStream* pBitStream, uint32_t u32Bytes)
{
	if (pBitStream->s32BytesLeft < (int32_t)u32Bytes)
		return 0;
	
	pBitStream->pu8CurrReadPtr += u32Bytes;
	pBitStream->s32BytesLeft -= u32Bytes;

	return u32Bytes;
}

