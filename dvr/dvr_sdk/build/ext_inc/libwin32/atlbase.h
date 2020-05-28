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

#ifndef _ATLBASE_H_
#define _ATLBASE_H_

#if defined(__stdc__) && !defined(__cplusplus)
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#include <windows.h>
#include <ole2.h>

// use IVI's COM headers for Linux

#ifndef __SYMBIAN32__
#define CBaseUnknown CBaseUnknown_atlbase_h
#include "../../Shared/Lib/LibComBase/LibComBase.h"
#undef  CBaseUnknown
#endif

class CComBSTR
{
public:
	BSTR m_str;
	CComBSTR()            				{ m_str = NULL; }
	CComBSTR(const CComBSTR& bstrSrc)	{ m_str = ::SysAllocString(bstrSrc.m_str);	}
	CComBSTR(LPCSTR pSrc) 				{ m_str = ::SysAllocString(pSrc); }
	~CComBSTR()           {
		if (m_str)	{
			::SysFreeString(m_str);
			m_str = NULL;
		}
	}
	operator BSTR() const { return m_str; }
	BSTR Copy()     const { return ::SysAllocStringByteLen(m_str, ::SysStringLen(m_str)); }
	CComBSTR& operator=(const CComBSTR& bstrSrc)
	{
		return *this = bstrSrc.m_str;
	}
	CComBSTR& operator=(LPCSTR pSrc)
	{
		::SysFreeString(m_str);
		m_str = ::SysAllocString(pSrc);
		return *this;
	}
	BSTR* operator&()
	{
		return &m_str;
	}
	unsigned int Length() const
	{
		return (m_str == NULL) ? 0 : strlen(m_str);
	}
	CComBSTR& operator+=(const CComBSTR& bstrSrc)
	{
		unsigned int len = Length() + bstrSrc.Length() + 1;
		BSTR str = (BSTR)malloc(len);
		strcpy(str, m_str);
		strcat(str, bstrSrc.m_str);
		free(m_str);
		m_str = str;
		return *this;
	}
	CComBSTR& operator+=(LPCSTR szSrc)
	{
		unsigned int len = Length() + strlen(szSrc) + 1;
		BSTR str = (BSTR)malloc(len);
		strcpy(str, m_str);
		strcat(str, szSrc);
		free(m_str);
		m_str = str;
		return *this;
	}
	void Empty()
	{
		free(m_str);
		m_str = NULL;
	}
	void Attach(BSTR src)
	{
		assert(m_str == NULL);
		m_str = src;
	}
	BSTR Detach()
	{
		BSTR s = m_str;
		m_str = NULL;
		return s;
	}
};

template <class T, const IID* piid, const GUID *plibid = 0>
class IDispatchImpl : public T {};

#endif /* _ATLBASE_H_ */
