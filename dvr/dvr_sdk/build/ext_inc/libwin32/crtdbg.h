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

#include <assert.h>
#ifdef ANDROID
#define assert(x) 
#endif
#define _ASSERT(x) assert(x)
#define _ASSERTE(x) assert(x)
#if !defined(ASSERT)
#define ASSERT(x) assert(x)
#endif
#define _CrtSetBreakAlloc(a)                ((long)0)
