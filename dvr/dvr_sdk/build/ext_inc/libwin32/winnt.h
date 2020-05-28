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

#ifndef _WINNT_
#define _WINNT_

#include "basetsd.h"
#include <sys/stat.h>  // for S_IFDIR

typedef void *PVOID;

typedef void                VOID;
typedef char                CHAR;
typedef short               SHORT;
typedef INT64               LONGLONG;
typedef UINT64              ULONGLONG;
typedef unsigned char       BOOLEAN;

typedef union _ULARGE_INTEGER {
    //(older gcc compiler does not support anonymous unions)
    //struct {
    //    DWORD LowPart;
    //    DWORD HighPart;
    //};
    struct {
        DWORD LowPart;
        DWORD HighPart;
    } u;
    ULONGLONG QuadPart;
} ULARGE_INTEGER;

//
// ANSI (Multi-byte Character) types
//
typedef char * BSTR;
typedef CHAR *PCHAR;
typedef CHAR *LPCH, *PCH;

typedef CONST CHAR *LPCCH, *PCCH;
typedef CHAR *NPSTR;
typedef CHAR *LPSTR, *PSTR;
typedef CONST CHAR *LPCSTR, *PCSTR;

// Definitions when Unicode is not used
#ifndef _TCHAR_DEFINED
typedef char TCHAR, *PTCHAR;
typedef unsigned char TBYTE , *PTBYTE ;
#define _TCHAR_DEFINED
#endif /* !_TCHAR_DEFINED */

typedef LPSTR LPTCH, PTCH;
typedef LPSTR PTSTR, LPTSTR;
typedef LPCSTR LPCTSTR;
#define __TEXT(quote) quote         // r_winnt
#define TEXT(quote) quote         // r_winnt

typedef const wchar_t * LPCWSTR;
typedef const wchar_t * LPWSTR;
typedef const wchar_t * LPCOLESTR;
typedef wchar_t *       LPOLESTR;
typedef wchar_t         WCHAR;

typedef PVOID HANDLE;
#define DECLARE_HANDLE(h) typedef HANDLE h;


#ifndef SHFOLDERAPI
#define SHFOLDERAPI void
#endif
#define OLESTR(x) L##x

#define MAKEINTRESOURCEA(i) (LPSTR)((DWORD)((WORD)(i)))
#define MAKEINTRESOURCEW(i) (LPWSTR)((DWORD)((WORD)(i)))
#ifdef UNICODE
#define MAKEINTRESOURCE  MAKEINTRESOURCEW
#else
#define MAKEINTRESOURCE  MAKEINTRESOURCEA
#endif // !UNICODE

typedef union _LARGE_INTEGER { 
    struct {
        DWORD LowPart; 
        LONG  HighPart; 
    } u;
    LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER; 

#define GENERIC_READ		O_RDONLY
#define GENERIC_WRITE		O_WRONLY

#define KEY_CREATE_LINK 	0x1
#define KEY_CREATE_SUB_KEY 	0x2
#define KEY_ENUMERATE_SUB_KEYS 	0x4
#define KEY_EXECUTE 		0x8
#define KEY_NOTIFY 		0x10
#define KEY_QUERY_VALUE 	0x20
#define KEY_SET_VALUE 		0x40
#define KEY_ALL_ACCESS 		0x80
#define KEY_READ 		0x100
#define KEY_WRITE 		0x200

typedef DWORD EXECUTION_STATE;

// COFF file header stuff from winnt.h

typedef struct _IMAGE_DATA_DIRECTORY {
    DWORD   VirtualAddress;
    DWORD   Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES    16
#define IMAGE_DIRECTORY_ENTRY_BASERELOC       5   // Base Relocation Table

#define DLL_PROCESS_ATTACH 1
#define DLL_THREAD_ATTACH  2
#define DLL_THREAD_DETACH  3
#define DLL_PROCESS_DETACH 0

#define STATUS_WAIT_0                    ((DWORD   )0x00000000L)    
#define STATUS_ABANDONED_WAIT_0          ((DWORD   )0x00000080L)    
#define STATUS_TIMEOUT                   ((DWORD   )0x00000102L)    
#define STATUS_PENDING                   ((DWORD   )0x00000103L)    
#define MAXIMUM_WAIT_OBJECTS 64    // Maximum number of wait objects

// Used for SetThreadPriority
#define THREAD_BASE_PRIORITY_LOWRT  15  // value that gets a thread to LowRealtime-1
#define THREAD_BASE_PRIORITY_MAX    2   // maximum thread base priority boost
#define THREAD_BASE_PRIORITY_MIN    -2  // minimum thread base priority boost
#define THREAD_BASE_PRIORITY_IDLE   -15 // value that gets a thread to idle

#define FILE_SHARE_READ   0x00000001  // not used
#define FILE_SHARE_WRITE  0x00000002  // not used

#define FILE_ATTRIBUTE_NORMAL 0644
#define FILE_ATTRIBUTE_READONLY 0444
#define FILE_ATTRIBUTE_DIRECTORY  S_IFDIR

#ifdef __SYMBIAN32__
#define PAGE_READONLY  0x02 
#define PAGE_READWRITE 0x04
#else  /* linux, maybe other POSIX */
#include <sys/mman.h>  // for MAP_* defines
#define PAGE_READONLY  MAP_SHARED
#define PAGE_READWRITE MAP_SHARED
#endif

#endif /* _WINNT_ */
