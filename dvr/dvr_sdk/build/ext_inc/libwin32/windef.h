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
// windef.h
//
// basic windows type definitions

#ifndef _WINDEF_
#define _WINDEF_ 1 /* set to 1 so that sismcif.h does not redefine DWORD etc */

#include "wincompat.h"

// BASETYPES
typedef unsigned long       ULONG;
typedef unsigned short      USHORT;
typedef unsigned char       UCHAR, *PUCHAR;

#if   defined(_WIN32_MAX_PATH)
#define MAX_PATH	_WIN32_MAX_PATH
#elif defined(MAXPATHLEN)   /* SymbianOS */
#define MAX_PATH    MAXPATHLEN
#else
#include <limits.h>  /* has value of PATH_MAX */
#if !defined(PATH_MAX)
#include <linux/limits.h>  /* has value of PATH_MAX */
#endif
#define MAX_PATH	PATH_MAX
#endif

#define _MAX_PATH MAX_PATH

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#ifndef FALSE
#define FALSE   0
#endif

#ifndef TRUE
#define TRUE    1
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif

#define far
#define near
#define pascal __stdcall
#define cdecl  _cdecl

#define CALLBACK   __stdcall
#define WINAPI     __stdcall
#define WINAPIV    __cdecl
#define APIENTRY   WINAPI
#define APIPRIVATE __stdcall
#define PASCAL     __stdcall

#define FAR   far
#define NEAR  near
#define CONST const

typedef IVIUInt64           DWORDLONG;
typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef float               FLOAT;
typedef FLOAT               *PFLOAT;
typedef BOOL near           *PBOOL;
typedef BOOL far            *LPBOOL;
typedef BYTE                *PBYTE;
typedef BYTE far            *LPBYTE;
typedef int near            *PINT;
typedef int far             *LPINT;
typedef WORD near           *PWORD;
typedef WORD far            *LPWORD;
typedef long                *PLONG;
typedef long far            *LPLONG;
typedef DWORD				*PDWORD;
typedef DWORD far           *LPDWORD;
typedef void far            *LPVOID;
typedef CONST void far      *LPCVOID;

typedef long                LONG;

typedef int                 INT;
typedef unsigned int        UINT;
typedef unsigned int        *PUINT;
typedef unsigned long       *PULONG;

#include "winnt.h"

/* Types use for passing & returning polymorphic values */
typedef UINT WPARAM;
typedef LONG LPARAM;
typedef LONG LRESULT;

#ifndef NOMINMAX

#define __min(a,b)          (((a) < (b)) ? (a) : (b))
#define __max(a,b)          (((a) > (b)) ? (a) : (b))

#ifndef MIN
#define MIN   __min
#endif
#ifndef MAX
#define MAX   __max
#endif
#ifndef min
#define min   __min
#endif
#ifndef max
#define max   __max
#endif

#endif  /* NOMINMAX */

#define LOWORD(l)           ((WORD)(l))
#define HIWORD(l)           ((WORD)((l>>16)&0xff))
#define LOBYTE(w)			((BYTE)(w))
#define HIBYTE(w)			((BYTE)((((WORD)(W) >> 8) & 0Xff)))

DECLARE_HANDLE            (HWND);

typedef BOOL (* FARPROC)(void);

DECLARE_HANDLE(HLOCAL);
DECLARE_HANDLE(HBITMAP);
DECLARE_HANDLE(HBRUSH);
DECLARE_HANDLE(HDC);
DECLARE_HANDLE(HINSTANCE);
typedef HINSTANCE HMODULE;  /* HMODULEs can be used in place of HINSTANCEs */

typedef DWORD   COLORREF;

typedef struct tagRECT {
  LONG    left;
  LONG    top;
  LONG    right;
  LONG    bottom;
#if 0
  tagRECT& operator=(const struct tagRECT &obj) {
  	left = obj.left;
	top = obj.top;
	right = obj.right;
	bottom = obj.bottom;
  	return *this;
  }
  BOOL operator==(const struct tagRECT &other) const {
	return (left   == other.left  &&
			top    == other.top   &&
			right  == other.right &&
			bottom == other.bottom);
  }
  BOOL operator!=(const struct tagRECT &other) const {
	return !(*this == other);
  }
  void print(char *caption="") { 
	printf("%s: left=%ld, top=%ld, right=%ld, bottom=%ld\n", 
		   caption, left, top, right, bottom); 
  }
#endif
} RECT, *PRECT, *LPRECT;

typedef struct tagSIZE { 
	  LONG cx; 
	    LONG cy; 
} SIZE, *PSIZE; 

typedef struct tagPOINT {
  LONG  x;
  LONG  y;
} POINT, *PPOINT, *LPPOINT, POINTL;

#endif
