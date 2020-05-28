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
// placeholder for Microsoft header file commdlg.h

#ifndef _COMMDLG_H
#define _COMMDLG_H

#define PACKED __attribute__ ((__packed__))

typedef struct tagOFNA {
   DWORD        lStructSize;
   HWND         hwndOwner;
   LPCSTR       lpstrFilter;
   LPSTR        lpstrFile;
   DWORD        nMaxFile;
   LPCSTR       lpstrTitle;
   DWORD        Flags;
} PACKED  OPENFILENAMEA, *LPOPENFILENAMEA;

typedef OPENFILENAMEA OPENFILENAME;
typedef LPOPENFILENAMEA LPOPENFILENAME;

#undef PACKED

#endif // _COMMDLG_H
