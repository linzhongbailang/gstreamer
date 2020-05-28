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
// DirectX SDK - Windows C++ header for DirectShow COM streams

#ifndef _STREAMS_H
#define _STREAMS_H

#include "strmif.h"
#include "vfwmsgs.h"  // (called from DirectX's <errors.h>)
#include "amfilter.h"

// uuids.h

#ifndef OUR_GUID_ENTRY
    #define OUR_GUID_ENTRY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8);
#endif

// ksuuids.h

// 3577EB09-9582-477f-B29C-B0C452A4FF9A
OUR_GUID_ENTRY(AM_KSPROPSETID_DVD_RateChange,
0x3577eb09, 0x9582, 0x477f, 0xb2, 0x9c, 0xb0, 0xc4, 0x52, 0xa4, 0xff, 0x9a)

#ifdef USE_TAKARA
#include "linuxworld.h"
#endif

#endif // _STREAMS_H
