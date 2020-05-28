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
// not part of standard WIN32 API - is this file still being used somewhere?

#ifndef OLEAUTO_H_
#define OLEAUTO_H_

static inline BSTR SysAllocString(const char * copyThis)
{
	return strdup(copyThis);
}

static inline VOID SysFreeString( BSTR bstr )
{
	free(bstr);
}

static inline BSTR SysAllocStringByteLen( LPCSTR psz, unsigned int len  )
{
	if (psz != NULL)
		return strdup(psz);
	else
	{
		BSTR s = (BSTR) malloc(len + 1);
		memset(s, 0, len+1);
		return s;
	}
}

static inline UINT SysStringByteLen( BSTR bstr  )
{
	return strlen((char *)bstr);
}

static inline UINT SysStringLen( BSTR bstr  )
{
	return strlen((char *)bstr);
}

#endif
