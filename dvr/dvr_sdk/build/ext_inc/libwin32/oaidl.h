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

#ifndef _OAIDL_H
#define _OAIDL_H

#include "rpc.h"
#include "rpcndr.h"
#include "windows.h"
#include "windef.h"

#ifndef __IDispatch_FWD_DEFINED__
#define __IDispatch_FWD_DEFINED__
typedef interface IDispatch IDispatch;
#endif 	/* __IDispatch_FWD_DEFINED__ */

/* VARIANT STRUCTURE
 *
 *  VARTYPE vt;
 *  WORD wReserved1;
 *  WORD wReserved2;
 *  WORD wReserved3;
 *  union {
 *    LONG           VT_I4
 *    BYTE           VT_UI1
 *    SHORT          VT_I2
 *    FLOAT          VT_R4
 *    DOUBLE         VT_R8
 *    VARIANT_BOOL   VT_BOOL
 *    SCODE          VT_ERROR
 *    CY             VT_CY
 *    DATE           VT_DATE
 *    BSTR           VT_BSTR
 *    IUnknown *     VT_UNKNOWN
 *    IDispatch *    VT_DISPATCH
 *    SAFEARRAY *    VT_ARRAY
 *    BYTE *         VT_BYREF|VT_UI1
 *    SHORT *        VT_BYREF|VT_I2
 *    LONG *         VT_BYREF|VT_I4
 *    FLOAT *        VT_BYREF|VT_R4
 *    DOUBLE *       VT_BYREF|VT_R8
 *    VARIANT_BOOL * VT_BYREF|VT_BOOL
 *    SCODE *        VT_BYREF|VT_ERROR
 *    CY *           VT_BYREF|VT_CY
 *    DATE *         VT_BYREF|VT_DATE
 *    BSTR *         VT_BYREF|VT_BSTR
 *    IUnknown **    VT_BYREF|VT_UNKNOWN
 *    IDispatch **   VT_BYREF|VT_DISPATCH
 *    SAFEARRAY **   VT_BYREF|VT_ARRAY
 *    VARIANT *      VT_BYREF|VT_VARIANT
 *    PVOID          VT_BYREF (Generic ByRef)
 *    CHAR           VT_I1
 *    USHORT         VT_UI2
 *    ULONG          VT_UI4
 *    INT            VT_INT
 *    UINT           VT_UINT
 *    DECIMAL *      VT_BYREF|VT_DECIMAL
 *    CHAR *         VT_BYREF|VT_I1
 *    USHORT *       VT_BYREF|VT_UI2
 *    ULONG *        VT_BYREF|VT_UI4
 *    INT *          VT_BYREF|VT_INT
 *    UINT *         VT_BYREF|VT_UINT
 *  }
 */
#if (__STDC__ && !defined(_FORCENAMELESSUNION)) || defined(NONAMELESSUNION)
#define __VARIANT_NAME_1 n1
#define __VARIANT_NAME_2 n2
#define __VARIANT_NAME_3 n3
#define __VARIANT_NAME_4 brecVal
#else
#define __tagVARIANT
#define __VARIANT_NAME_1
#define __VARIANT_NAME_2
#define __VARIANT_NAME_3
#define __tagBRECORD
#define __VARIANT_NAME_4
#endif
typedef /* [wire_marshal] */ struct tagVARIANT VARIANT;

struct  tagVARIANT
    {
    union 
        {
        struct  __tagVARIANT
            {
#ifdef NOTNEEDED
            VARTYPE vt;
#endif
            WORD wReserved1;
            WORD wReserved2;
            WORD wReserved3;
            union 
                {
                LONG lVal;
                BYTE bVal;
                SHORT iVal;
                FLOAT fltVal;
#ifdef NOTNEEDED
                DOUBLE dblVal;
                VARIANT_BOOL boolVal;
                _VARIANT_BOOL bool;
                SCODE scode;
                CY cyVal;
                DATE date;
#endif
                BSTR bstrVal;
                IUnknown __RPC_FAR *punkVal;
                IDispatch __RPC_FAR *pdispVal;
#if NOTNEEDED
                SAFEARRAY __RPC_FAR *parray;
#endif
                BYTE __RPC_FAR *pbVal;
                SHORT __RPC_FAR *piVal;
                LONG __RPC_FAR *plVal;
                FLOAT __RPC_FAR *pfltVal;
#ifdef NOTNEEDED
                DOUBLE __RPC_FAR *pdblVal;
                VARIANT_BOOL __RPC_FAR *pboolVal;
                _VARIANT_BOOL __RPC_FAR *pbool;
                SCODE __RPC_FAR *pscode;
                CY __RPC_FAR *pcyVal;
                DATE __RPC_FAR *pdate;
#endif
                BSTR __RPC_FAR *pbstrVal;
                IUnknown __RPC_FAR *__RPC_FAR *ppunkVal;
                IDispatch __RPC_FAR *__RPC_FAR *ppdispVal;
#ifdef NOTNEEDED
                SAFEARRAY __RPC_FAR *__RPC_FAR *pparray;
#endif
                VARIANT __RPC_FAR *pvarVal;
                PVOID byref;
                CHAR cVal;
                USHORT uiVal;
                ULONG ulVal;
                INT intVal;
                UINT uintVal;
#ifdef NOTNEEDED
                DECIMAL __RPC_FAR *pdecVal;
#endif
                CHAR __RPC_FAR *pcVal;
                USHORT __RPC_FAR *puiVal;
                ULONG __RPC_FAR *pulVal;
                INT __RPC_FAR *pintVal;
                UINT __RPC_FAR *puintVal;
                struct  __tagBRECORD
                    {
                    PVOID pvRecord;
#ifdef NOTNEEDED
                    IRecordInfo __RPC_FAR *pRecInfo;
#endif
                    }	__VARIANT_NAME_4;
                }	__VARIANT_NAME_3;
            }	__VARIANT_NAME_2;
#ifdef NOTNEEDED
        DECIMAL decVal;
#endif
        }	__VARIANT_NAME_1;
    };
typedef VARIANT __RPC_FAR *LPVARIANT;

typedef VARIANT VARIANTARG;

typedef VARIANT __RPC_FAR *LPVARIANTARG;


typedef LONG DISPID;

typedef struct  tagDISPPARAMS
    {
    /* [size_is] */ VARIANTARG __RPC_FAR *rgvarg;
    /* [size_is] */ DISPID __RPC_FAR *rgdispidNamedArgs;
    UINT cArgs;
    UINT cNamedArgs;
    }	DISPPARAMS;

EXTERN_C const IID IID_IDispatch;

MIDL_INTERFACE("00020400-0000-0000-C000-000000000046")
IDispatch : public IUnknown {};

#endif
