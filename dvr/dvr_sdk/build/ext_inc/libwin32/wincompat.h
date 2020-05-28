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

#ifndef _WINCOMPAT_H
#define _WINCOMPAT_H

#ifndef __SYMBIAN32__
// emulate MSVC 6.0 builtins

#define __cdecl
#define __fastcall
#define __stdcall
#define __export
#define __inline      inline   // Microsoft extension for non-C++ inline
#define __forceinline inline   // Microsoft extension for non-C++ inline
#define _inline       __inline        // _inline is deprecated by Microsoft
#define _stdcall

#define _declspec(x)
#define __declspec(x)

#ifndef __int64
#define __int64 long long
#endif
typedef long long IVIInt64;
typedef unsigned long long IVIUInt64;

// actually from MSVC stdio.h

#define _vsnprintf  vsnprintf
#define _vsntprintf vsnprintf
#define _sntprintf  snprintf
#define _snprintf  snprintf
#define _cdecl

// actually from MSVC string.h
#define _stricmp stricmp

// actually from MSVC stdlib.h

#include <stdio.h>
static inline char* _itoa(int d, char *string, int radix)
{
    if(string) 
	{
		if(radix == 10)
			sprintf(string, "%d", d);
		else if(radix == 16)
			sprintf(string, "%x", d);
		else
			return 0;
	}
    return string;
}

static inline char* _i64toa(__int64 d, char *string, int radix)
{
	if(string)
	{
		if(radix == 10)
			sprintf(string, "%llu", d);
		else if(radix == 16)
		{
			unsigned long hd = (unsigned long)(d>>32);
			unsigned long ld = (unsigned long)d;
			sprintf(string,"%lx%08lx",hd,ld);
		}
		else
			return 0;
	}
    return string;
}

#else  
/* SYMBIANOS_BUILD */
#define NO_ASM 1
#ifdef __cplusplus
#include <e32std.h>
#endif

#ifdef __WINS__
/* Symbian Win32 emulation using Microsoft compiler */
#pragma warning(disable:4100) // "unreferenced formal parameter"
#pragma warning(disable:4514) // "unreferenced inline function was removed"
#ifndef __cplusplus
#define inline __inline
#endif

#else
/* ARMI platform */
#define __int64 long long
#define __cdecl
#define __fastcall
#define __stdcall
#define __forceinline inline
#define __export
#define _stdcall
#define _inline inline
#endif
typedef          __int64 IVIInt64;
typedef unsigned __int64 IVIUInt64;

#define valloc malloc
// actually from MSVC string.h
#define _stricmp stricmp

#include <stdarg.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
int portable_snprintf(char *str, size_t str_m, const char *fmt, /*args*/ ...);
int portable_vsnprintf(char *str, size_t str_m, const char *fmt, va_list ap);
void sym_register_file_operations(); // must call before using EMB_FILE_* calls
#ifdef __cplusplus
}
#endif
#define snprintf  portable_snprintf
#define vsnprintf portable_vsnprintf

#define _snprintf snprintf
#define _vsnprintf vsnprintf

#endif /* __SYMBIAN32__ */

#endif /* _WINCOMPAT_H */
