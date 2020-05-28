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
// platform-independent definition of IUnknown and IClassFactory

#ifndef __iviunknwn_h__
#define __iviunknwn_h__

/* Forward Declarations */

#ifndef __IUnknown_FWD_DEFINED__
#define __IUnknown_FWD_DEFINED__
typedef interface IUnknown IUnknown;
#endif 	/* __IUnknown_FWD_DEFINED__ */


#ifndef __IClassFactory_FWD_DEFINED__
#define __IClassFactory_FWD_DEFINED__
typedef interface IClassFactory IClassFactory;
#endif 	/* __IClassFactory_FWD_DEFINED__ */


/* header files for imported files */
#include "wtypes.h"

#ifndef __IUnknown_INTERFACE_DEFINED__
#define __IUnknown_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IUnknown
 * at Thu Sep 11 10:57:03 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][uuid][object][local] */


typedef /* [unique] */ IUnknown __RPC_FAR *LPUNKNOWN;

//////////////////////////////////////////////////////////////////
// IID_IUnknown and all other system IIDs are provided in UUID.LIB
// Link that library in with your proxies, clients and servers
//////////////////////////////////////////////////////////////////

#ifdef __cplusplus
EXTERN_C const IID IID_IUnknown;

    MIDL_INTERFACE("00000000-0000-0000-C000-000000000046")
	IUnknown
    {
    public:
		static const GUID s_guid;

        BEGIN_INTERFACE
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject) = 0;

        virtual ULONG STDMETHODCALLTYPE AddRef( void) = 0;

        virtual ULONG STDMETHODCALLTYPE Release( void) = 0;

        END_INTERFACE
    };
#endif  /* __cplusplus */
#endif 	/* __IUnknown_INTERFACE_DEFINED__ */


#ifndef __IClassFactory_INTERFACE_DEFINED__
#define __IClassFactory_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IClassFactory
 * at Thu Sep 11 10:57:03 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IClassFactory __RPC_FAR *LPCLASSFACTORY;

#ifdef __cplusplus
EXTERN_C const IID IID_IClassFactory;

    MIDL_INTERFACE("00000001-0000-0000-C000-000000000046")
    IClassFactory : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE CreateInstance( 
            /* [unique][in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE LockServer( 
            /* [in] */ BOOL fLock) = 0;
        
    };

#endif  /* __cplusplus */
#endif 	/* __IClassFactory_INTERFACE_DEFINED__ */

#endif  /* __iviunknwn_h__ */ 

