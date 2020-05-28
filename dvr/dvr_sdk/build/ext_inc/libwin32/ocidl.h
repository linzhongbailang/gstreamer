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

#ifndef _OCIDL_H_
#define _OCIDL_H_

typedef struct tagCAUUID 
{ 
    ULONG     cElems; 
    GUID FAR* pElems; 
} CAUUID; 


DECLARE_INTERFACE_(ISpecifyPropertyPages, IUnknown)
{
	STDMETHOD(GetPages)(CAUUID *pPages) PURE;
};

#endif /* _OCIDL_H_ */
