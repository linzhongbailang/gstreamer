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

#ifndef _OBJBASE_H
#define _OBJBASE_H

#ifdef __cplusplus
__inline BOOL  IsEqualGUID(REFGUID rguid1, REFGUID rguid2)
{
   return (
      ((PLONG) &rguid1)[0] == ((PLONG) &rguid2)[0] &&
      ((PLONG) &rguid1)[1] == ((PLONG) &rguid2)[1] &&
      ((PLONG) &rguid1)[2] == ((PLONG) &rguid2)[2] &&
      ((PLONG) &rguid1)[3] == ((PLONG) &rguid2)[3]);
}

__inline BOOL operator==(const GUID& guidOne, const GUID& guidOther)
{
    return IsEqualGUID(guidOne,guidOther);
}

__inline BOOL operator!=(const GUID& guidOne, const GUID& guidOther)
{
    return !(guidOne == guidOther);
}
#endif

/****** STD Object API Prototypes *****************************************/
static inline HRESULT CoInitializeEx(LPVOID pvReserved, DWORD dwCoInit)
                                                           { return S_OK; }
static inline HRESULT CoInitialize(LPVOID pvReserved)      { return S_OK; }
static inline HRESULT CoUninitialize()                     { return S_OK; }

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv);

/****** Default Memory Allocation ******************************************/
static inline void *CoTaskMemAlloc(ULONG cb) { return malloc(cb); }
static inline void  CoTaskMemFree(void* pv)  { (void)free(pv);   }

#endif /* _OBJBASE_H */
