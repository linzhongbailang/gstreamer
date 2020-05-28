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
// Emacs -*- mode: C++ -*-

#ifndef _WINDOWS_H
#define _WINDOWS_H

#include "wincompat.h" // defines Microsoft compiler builtins

#ifdef __cplusplus
extern "C" {
#endif

#ifdef RH_70
#define __restrict
#define _G_fpos64_t _G_off64_t
#endif // RH_70

#include <unistd.h>       // for open, close, read, lseek
#include <stdio.h>
#include <string.h>
#include <stdlib.h>   // for getenv, setenv
#include <errno.h>    // for error enumerations
#include <fcntl.h>    // for O_RDONLY
#include <time.h>     // for nanosleep
#include <sys/time.h> // for struct timeval
#include <sys/stat.h> // for stat
#include <sys/types.h>
#include <dirent.h>

#ifndef __SYMBIAN32__
#include <pthread.h>
#ifdef __linux__
#include <linux/unistd.h> // for llseek
#include <malloc.h>       // for memalign, valloc
#endif
#ifdef __APPLE__
#include <unistd.h>
#include <malloc/malloc.h>
#endif
#endif

#include "windef.h"    // windows header for basic Windows type definitions
#include "winbase.h"   // windows header for Win32 Base APIs
#include "winuser.h"   // windows header for USER proc declarations and macros
#include "winreg.h"    // windows 32-bit registry API
#include "winerror.h"  // windows header for COM and Windows errors
#include "basetyps.h"  // windows header for GUID stuff
#include "tchar.h"     // windows header for UNICODE
#include "wtypes.h"    // windows header for basic Windows data types
#include "crtdbg.h"    // windows header for assert macros
#include "unknwn.h"    // windows header for IUnknown interfaces
#include "mmsystem.h"  // windows header for multimedia APIs
#include "ole2.h"      // windows header for OLE2 (IsEqualGUID, CoInitialize)
#include "winnls.h"    // windows header for i18n/wide characters
#include "wingdi.h"    // windows header for graphics device/bitmap definitions

// Misc

#ifndef stricmp
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif
#define _strdup strdup

#define _TEXT

#if 0
#if !defined(USE_TAKARA) && !defined(USE_DVR) && !defined(__SYMBIAN32__)
static inline size_t wcslen(const wchar_t* string)
{
	return strlen((const char*)string);
}
#endif
#endif
static inline char* _strupr(char* string)
{
	char* s = string;

	while (*s)
	{
		if (*s >= 'a' && *s <= 'z')
			*s -= ('a' - 'A');
		s++;
	}

	return string;
}

static inline char* _strlwr(char* string)
{
	char* s = string;

	while (*s)
	{
		if (*s >= 'A' && *s <= 'Z')
			*s += ('a' - 'A');
		s++;
	}

	return string;
}

// From winbase.h

UINT GetSystemDirectory(LPSTR lpBuffer, UINT uSize);

static inline DWORD GetEnvironmentVariable(
  LPCTSTR lpName,  // address of environment variable name
  LPTSTR lpBuffer, // address of buffer for variable value
  DWORD nSize      // size of buffer, in characters
)
{
  const char *data = getenv(lpName);
  size_t datalen;

  if (data==0)
	return 0;

  strncpy(lpBuffer, data, nSize);
  datalen = strlen(data);
  if (datalen+1 <= nSize)
	return datalen;    // number of chars copied, does not include null char

  return datalen+1;  // space needed, includes null char
}

static inline BOOL SetEnvironmentVariable(
  LPCTSTR lpName,  // address of environment variable name
  LPCTSTR lpValue  // address of new value for variable
)
{
  if (lpValue)
	return !setenv(lpName, lpValue, 1);

  unsetenv(lpName);
  return 0;
}

static inline DWORD GetFileAttributes(LPCTSTR lpFileName) {
  struct stat s;
  if (stat(lpFileName, &s) < 0)
	return 0xffffffff;
  return s.st_mode;
}

// CRT library

char* _strdate(char* datestr);
char* _strtime(char* timestr);

//
// the following five functions should be defined by the user application.
// they should not be defined in LIBWIN32.
//

int     MessageBox (HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType);
BOOL    PostMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT SendMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL    GetClientRect (HWND hWnd, LPRECT lpRect);
BOOL    ClientToScreen(HWND hWnd, LPPOINT lpPoint);


#ifdef __GNUG__
void OutputDebugString(char *szBuffer)  __attribute__((weak));  /* overridable by app */
#else
void OutputDebugString(char *szBuffer);
#endif //__GNUG__


static inline void DBP(char *szFormat, ...)
{
	OutputDebugString(szFormat);
}

#if  defined(__x86_64__) || defined(ANDROID) || defined(__linux__)
#define _lseeki64 lseek64
#else
#if !defined(MSASM) && !defined(__APPLE__) // we can remove this checking after porting _lseeki64 compiled by ICC with msasm option
#ifndef __SYMBIAN32__
static inline _syscall5(int,  _llseek,  uint,  fd, ulong, hi, ulong, lo,
						loff_t *, res, uint, wh);
static inline long long _lseeki64(int handle, unsigned long long offset,
								  int origin)
{
  loff_t n_bytes = 0;
  int retval = _llseek(handle, offset>>32, (unsigned long)offset,
					   &n_bytes, origin);
  if (retval<0 && errno==0)
	errno = EINVAL;
  return (retval < 0) ? retval : n_bytes;
}
#endif
#endif
#endif
#if 0
// comment for "gcc-linaro-5.3-2016.02-x86_64_arm-linux-gnueabihf"
#if __GNUC__ < 5
#define _mm_malloc(size, alignment)      _aligned_malloc(size,alignment)
#define _mm_free(ptr)                    _aligned_free(ptr)
#define _aligned_malloc(size, alignment) memalign(alignment, size)
#define _aligned_free(ptr)               free(ptr)
#else
#include <mm_malloc.h>
#endif
#endif
// Critical sections, implemented using mutex

#ifndef __SYMBIAN32__
typedef struct tagCRITICAL_SECTION {
  pthread_mutex_t     _mutex;
  pthread_mutexattr_t _mutexattr;
} CRITICAL_SECTION, *LPCRITICAL_SECTION;


static inline
VOID InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection) {
  //  printf("[init]  %0x\n", (UINT)lpCriticalSection);
  int ret;
  ret = pthread_mutexattr_init(&(lpCriticalSection->_mutexattr));
  //assert(ret==0);
#if defined (__APPLE__) || defined(__linux__)
	//lpCriticalSection->_mutex.__sig = _PTHREAD_MUTEX_SIG_init;
	pthread_mutexattr_settype(&lpCriticalSection->_mutexattr, PTHREAD_MUTEX_RECURSIVE);
#elif ANDROID
    // Do nothing
#else
  lpCriticalSection->_mutexattr.__mutexkind = PTHREAD_MUTEX_RECURSIVE_NP;
#endif
  ret = pthread_mutex_init(&(lpCriticalSection->_mutex),
						   &(lpCriticalSection->_mutexattr));
  //assert(ret==0);
}

static inline
VOID DeleteCriticalSection    (LPCRITICAL_SECTION lpCriticalSection) {
  //  printf("[dele]  %0x\n", (UINT)lpCriticalSection);

  int ret;
  ret = pthread_mutex_destroy    (&(lpCriticalSection->_mutex));
  //assert(ret==0);
  ret = pthread_mutexattr_destroy(&(lpCriticalSection->_mutexattr));
  //assert(ret==0);
}

static inline
VOID EnterCriticalSection     (LPCRITICAL_SECTION lpCriticalSection) {
  //  printf("[enter] %0x\n", (UINT)lpCriticalSection);
  int ret;
  ret = pthread_mutex_lock(&(lpCriticalSection)->_mutex);
  //assert(ret==0);
}

static inline
VOID LeaveCriticalSection     (LPCRITICAL_SECTION lpCriticalSection) {
  //  printf("[leave] %0x\n", (UINT)lpCriticalSection);
  int ret;
  ret = pthread_mutex_unlock(&(lpCriticalSection->_mutex));
  // ret==1 means mutex is owned by another thread!
}
#else
/* SYMBIANOS */
#ifdef __cplusplus
typedef struct tagCRITICAL_SECTION {
  RCriticalSection _cs;
  RCriticalSection _cslck;
  TThreadId  _id;
  int _lockcount;
} CRITICAL_SECTION, *LPCRITICAL_SECTION;

VOID InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
VOID DeleteCriticalSection    (LPCRITICAL_SECTION lpCriticalSection);
VOID EnterCriticalSection     (LPCRITICAL_SECTION lpCriticalSection);
VOID LeaveCriticalSection     (LPCRITICAL_SECTION lpCriticalSection);

#endif  /* __cplusplus */
#endif  /* __SYMBIAN32__ */

// VirtualAlloc/VirtualFree - used to allocate page-aligned buffers
// to reduce unnecessary copying

#define MEM_COMMIT           0x1000
#define MEM_RESERVE          0x2000
#define MEM_RELEASE          0x8000

static inline
void CopyMemory(PVOID Destination, CONST VOID *Source, SIZE_T Length)
{
	memcpy(Destination,Source,Length);
}

static inline LPVOID VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize,
								  DWORD flAllocationType, DWORD flProtect) {
  const void *addr;
  assert(lpAddress==NULL);
  addr = valloc(dwSize);
#if 0
  if (flAllocationType&MEM_RESERVE)
	mlock(addr, dwSize);   // only locks memory if process running as root!
#endif
  return (LPVOID)addr;
}

static inline LPVOID VirtualFree (LPVOID lpAddress, SIZE_T dwSize,
								  DWORD dwFreeType) {
  assert((dwSize==0) && (dwFreeType==MEM_RELEASE));
  assert(lpAddress!=0);
  free(lpAddress);
  return (void *)TRUE;
}

#define _sleep sleep

#ifdef __SYMBIAN32__
#ifdef __cplusplus
static inline VOID Sleep(DWORD dwMilliseconds)
	{ User::After(dwMilliseconds*1000); }
#endif
#else
VOID Sleep(DWORD dwMilliseconds);
#endif

static inline VOID ZeroMemory(LPVOID Destination, SIZE_T Length)
    { memset(Destination,0,Length); }

static inline BOOL SetRect(
  LPRECT lprc, // rectangle
  int xLeft,   // left side
  int yTop,    // top side
  int xRight,  // right side
  int yBottom  // bottom side
)
{
  if (lprc)
	{
	  lprc->left = xLeft;
	  lprc->top = yTop;
	  lprc->right = xRight;
	  lprc->bottom = yBottom;
	  return 1;
	}
  return 0;
}

static inline BOOL IntersectRect(
  LPRECT lprcDst,        // intersection buffer
  CONST RECT *lprcSrc1,  // first rectangle
  CONST RECT *lprcSrc2   // second rectangle
)
{
  lprcDst->top    = MAX(lprcSrc1->top,    lprcSrc2->top);
  lprcDst->bottom = MIN(lprcSrc1->bottom, lprcSrc2->bottom);
  lprcDst->left   = MAX(lprcSrc1->left,   lprcSrc2->left);
  lprcDst->right  = MIN(lprcSrc1->right,  lprcSrc2->right);
  if (lprcDst->top>=lprcDst->bottom || lprcDst->left>=lprcDst->right)
	{
	  SetRect(lprcDst,0,0,0,0);
	  return 0;
	}
  return 1;
}

static inline BOOL UnionRect(
  LPRECT lprcDst,        // destination rectangle
  CONST RECT *lprcSrc1,  // first rectangle
  CONST RECT *lprcSrc2   // second rectangle
)
{
  lprcDst->top    = MIN(lprcSrc1->top,    lprcSrc2->top);
  lprcDst->bottom = MAX(lprcSrc1->bottom, lprcSrc2->bottom);
  lprcDst->left   = MIN(lprcSrc1->left,   lprcSrc2->left);
  lprcDst->right  = MAX(lprcSrc1->right,  lprcSrc2->right);
  if (lprcDst->top>=lprcDst->bottom || lprcDst->left>=lprcDst->right)
	{
	  SetRect(lprcDst,0,0,0,0);
	  return 0;
	}
  return 1;
}

static inline BOOL EqualRect(
  CONST RECT *lprc1,  // first rectangle
  CONST RECT *lprc2   // second rectangle
)
{
  return (lprc1->left   == lprc2->left  &&
		  lprc1->right  == lprc2->right &&
		  lprc1->top    == lprc2->top   &&
		  lprc1->bottom == lprc2->bottom);
}

static inline BOOL OffsetRect(
  LPRECT lprc,  // rectangle
  int dx,       // horizontal offset
  int dy        // vertical offset
)
{
  if (lprc)	{
	lprc->left   += dx;
	lprc->right  += dx;
	lprc->top    += dy;
	lprc->bottom += dy;
	return 1;
  }
  return 0;
}

static inline BOOL IsRectEmpty(CONST RECT *lprc)
	{
	return !lprc || lprc->right<=lprc->left || lprc->bottom<=lprc->top ?
		TRUE : FALSE;
	}

BOOL PtInRect(CONST RECT *lprc, POINT pt);


#define CLR_INVALID -1


// these ones are really lame!
#define _open   open
#define _read   read
#define _close  close
#define _lseek  lseek
#define _stat   stat
#define _stati64  stat
#define _fstati64 fstat
#define _O_RDONLY O_RDONLY
#define _O_BINARY O_BINARY
#define _O_RDWR   O_RDWR
#define _O_CREAT  O_CREAT
#define _O_TRUNC  O_TRUNC
#define _S_IREAD  S_IRUSR|S_IRGRP|S_IROTH
#define _S_IWRITE S_IWUSR|S_IWGRP|S_IWOTH

static inline int _fileno(FILE *f) { return fileno(f); }

static inline DWORD GetTickCount(void) { return timeGetTime(); }

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID);

#define ERROR_INVALID -1
#define _strnicmp strncasecmp
#define _tcsncmp strncmp
#if !defined(_tcscmp)
#define _tcscmp strcmp //_UNICODE && _MBCS not defined.
#endif

#ifdef __cplusplus
}  /* end extern "C" */
#endif

#endif /* _WINDOWS_H */
