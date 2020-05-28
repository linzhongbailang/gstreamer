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

#ifndef _WTYPES_H
#define _WTYPES_H

#include "rpc.h"
#include "rpcndr.h"

#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef LONG HRESULT;
#endif // !_HRESULT_DEFINED

#ifndef __IID_DEFINED__
#define __IID_DEFINED__
typedef GUID                IID;
#define IID_NULL    GUID_NULL

typedef GUID                CLSID;
#define CLSID_NULL  GUID_NULL

#if defined(__cplusplus)
typedef const GUID&         REFGUID;
typedef const IID&          REFIID;
typedef const CLSID&        REFCLSID;
#else
#define REFGUID       const GUID * const
#define REFIID        const IID * const
#define REFCLSID      const CLSID * const
#endif
#endif // __IID_DEFINED__

typedef 
enum tagCLSCTX
    {	CLSCTX_INPROC_SERVER	= 0x1,
	CLSCTX_INPROC_HANDLER	= 0x2,
	CLSCTX_LOCAL_SERVER	= 0x4,
	CLSCTX_INPROC_SERVER16	= 0x8,
	CLSCTX_REMOTE_SERVER	= 0x10,
	CLSCTX_INPROC_HANDLER16	= 0x20,
	CLSCTX_INPROC_SERVERX86	= 0x40,
	CLSCTX_INPROC_HANDLERX86	= 0x80,
	CLSCTX_ESERVER_HANDLER	= 0x100
    }	CLSCTX;

// COM server types
#define CLSCTX_INPROC           (CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER)
// With DCOM, CLSCTX_REMOTE_SERVER should be included
#if (_WIN32_WINNT >= 0x0400 ) || defined(_WIN32_DCOM) // DCOM
#define CLSCTX_ALL              (CLSCTX_INPROC_SERVER| \
                                 CLSCTX_INPROC_HANDLER| \
                                 CLSCTX_LOCAL_SERVER| \
                                 CLSCTX_REMOTE_SERVER)

#define CLSCTX_SERVER           (CLSCTX_INPROC_SERVER|CLSCTX_LOCAL_SERVER|CLSCTX_REMOTE_SERVER)
#else
#define CLSCTX_ALL              (CLSCTX_INPROC_SERVER| \
                                 CLSCTX_INPROC_HANDLER| \
                                 CLSCTX_LOCAL_SERVER )

#define CLSCTX_SERVER           (CLSCTX_INPROC_SERVER|CLSCTX_LOCAL_SERVER)
#endif


// used in utilgdi.h
typedef struct  tagPALETTEENTRY {
  BYTE peRed;
  BYTE peGreen;
  BYTE peBlue;
  BYTE peFlags;
} PALETTEENTRY;

typedef struct tagLOGPALETTE {
  WORD          palVersion;
  WORD          palNumEntries;
  PALETTEENTRY  palPalEntry[1];
} LOGPALETTE, *PLOGPALETTE, *LPLOGPALETTE;

#ifndef _LCID_DEFINED
#define _LCID_DEFINED
typedef DWORD LCID;
#endif // !_LCID_DEFINED

typedef double DOUBLE;

#endif // _WTYPES_H
