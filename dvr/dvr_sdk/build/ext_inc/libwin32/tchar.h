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

#ifndef _TCHAR_H
#define _TCHAR_H 1

#if defined(TCHAR)
#undef TCHAR
#endif

#ifndef _TCHAR_DEFINED
#define _TCHAR_DEFINED
typedef char TCHAR;
#endif

#define _tcsdup    _strdup
#define _tcslen    strlen
#define _tcscpy    strcpy
#define _tcsncpy   strncpy
#define _tcscat    strcat
#define _tcsncat   strncat
#define _tcsncicmp strncasecmp
#define _tcsnicmp  strncasecmp
#define _tcsicmp   stricmp
#define _tcschr    strchr
#define _tcsrchr   strrchr
#define _stprintf  sprintf
#define wsprintf   sprintf
#define _topen     open
#define _write     write
#define _tfopen	   fopen
#define _stscanf   sscanf


#define _T(x) x
#define _ttol(x) atol(x)

#endif /* _TCHAR_H */
