//=============================================================================
//	THIS SOURCE CODE IS PROPRIETARY INFORMATION BELONGING TO INTERVIDEO, INC.
// 	ANY USE INCLUDING BUT NOT LIMITED TO COPYING OF CODE, CONCEPTS, AND/OR
//	ALGORITHMS IS PROHIBITED EXCEPT WITH EXPRESS WRITTEN PERMISSION BY THE 
//	COMPANY.
//
// 	THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
// 	KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// 	IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
// 	PURPOSE.
//
// 	Copyright (c) 2004 - 2005  InterVideo Corporation.  All Rights Reserved.
//
//-----------------------------------------------------------------------------
// DirectX SDK

#ifndef AMFILTER_H_
#define AMFILTER_H_
class IMediaSample
{};
class IMemAllocator
{};

#define ALLOCATOR_PROPERTIES DWORD
#define UNITS 1000000

class IAsyncReader
{
public:
HRESULT Length(
  LONGLONG *pTotal,
LONGLONG *pAvailable
){ return S_OK; }
HRESULT BeginFlush(void)
{ return S_OK; }
HRESULT EndFlush(void)
{ return S_OK; }
HRESULT SyncReadAligned(
  IMediaSample *pSample
){ return S_OK; }
HRESULT WaitForNext(
DWORD dwTimeout,
IMediaSample **ppSample,
DWORD *pdwUser
){ return S_OK; }
HRESULT Request(
IMediaSample *pSample,
DWORD dwUser
){ return S_OK; }
HRESULT RequestAllocator(
IMemAllocator *pPreferred,
ALLOCATOR_PROPERTIES *pProps,
IMemAllocator **ppActual
){ return S_OK; }
HRESULT SyncRead(
LONGLONG llPosition,
LONG lLength,
BYTE *pBuffer
){ return S_OK; }

};

#define TDMX_SYNC 1

#endif

