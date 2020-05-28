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

#ifndef _STRMIF_H_
#define _STRMIF_H_

#include "rpc.h"
#include "rpcndr.h"

#include "windows.h"
#include "ole2.h"

#ifndef USE_ICC
//#include "unknwn.h"
#include "iviUnknwn.h"
#include "ocidl.h"

#define CHARS_IN_GUID     39
typedef struct _AMMediaType
    {
    GUID majortype;
    GUID subtype;
    BOOL bFixedSizeSamples;
    BOOL bTemporalCompression;
    ULONG lSampleSize;
    GUID formattype;
    IUnknown *pUnk;
    ULONG cbFormat;
    /* [size_is] */ BYTE *pbFormat;
    } 	AM_MEDIA_TYPE;

typedef enum _PinDirection
    {	
		PINDIR_INPUT	= 0,
		PINDIR_OUTPUT	= PINDIR_INPUT + 1
    } PIN_DIRECTION;

#define MAX_PIN_NAME     128
#define MAX_FILTER_NAME  128
typedef LONGLONG REFERENCE_TIME;

typedef double REFTIME;

#endif  // !USE_ICC
#endif  // _STRMIF_H_
