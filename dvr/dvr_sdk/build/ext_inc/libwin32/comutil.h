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
// comutil.h - Native C++ compiler COM support - BSTR, VARIANT wrappers header

#if !defined(_INC_COMUTIL)
#define _INC_COMUTIL

#include <ole2.h>

class _com_error;

//////////////////////////////////////////////////////////////////////////////
//
// Error checking routines
//
//////////////////////////////////////////////////////////////////////////////

inline void __stdcall _com_issue_error(HRESULT)
{
return;
}

//////////////////////////////////////////////////////////////////////////////
//
// Routines for handling conversions between BSTR and char*
//
//////////////////////////////////////////////////////////////////////////////

namespace _com_util {
	// Convert char * to BSTR
	//
	inline BSTR __stdcall ConvertStringToBSTR(const char* pSrc) throw(_com_error)
	{
		return pSrc==0 ? 0 : strdup(pSrc);
	}

	// Convert BSTR to char *
	//
	inline char* __stdcall ConvertBSTRToString(BSTR pSrc) throw(_com_error)
	{
		return pSrc==0 ? 0 : strdup(pSrc);
	}

	inline void CheckError(HRESULT hr) throw(_com_error)
	{
		if (FAILED(hr)) {
			_com_issue_error(hr);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
//
// Wrapper class for BSTR
//
//////////////////////////////////////////////////////////////////////////////

class _bstr_t {
public:
	// Constructors
	//
	_bstr_t() throw();
	_bstr_t(const _bstr_t& s) throw();
	_bstr_t(const char* s) throw(_com_error);

	// Destructor
	//
	~_bstr_t() throw();

	// Assignment operators
	//
	_bstr_t& operator=(const _bstr_t& s) throw(_com_error);
	_bstr_t& operator=(const char* s) throw(_com_error);

	// Operators
	_bstr_t& operator+=(const _bstr_t& s) throw(_com_error);
	_bstr_t operator+(const _bstr_t& s) const throw(_com_error);

	// Extractors
	//
	operator const char*() const throw(_com_error);
	operator char*() const throw(_com_error);

	operator const wchar_t*() const throw();
	operator wchar_t*() const throw();

	// Low-level helper functions
	//
	BSTR copy() const throw(_com_error);
	unsigned int length() const throw();

private:
	// Referenced counted wrapper
	//
	class Data_t {
	public:
		// Constructors
		//
		Data_t(const char* s) throw(_com_error);
		Data_t(const _bstr_t& s1, const _bstr_t& s2) throw(_com_error);

		// Reference counting routines
		//
		unsigned long AddRef() throw();
		unsigned long Release() throw();

		// Extractors
		//
		operator const char*() const throw(_com_error);

		// Low-level helper functions
		//
		const char* GetString() const throw(_com_error);

		BSTR Copy() const throw(_com_error);
		unsigned int Length() const throw();

		~Data_t() throw();

	private:
		mutable char*	m_str;
		char*		m_wstr;
		unsigned long	m_RefCount;

		// Never allow default construction
		//
		Data_t() throw();

		// Never allow copy
		//
		Data_t(const Data_t& s) throw();

		void _Free() throw();
	};

private:
	// Reference counted representation
	//
	Data_t* m_Data;

private:
	// Low-level utilities
	//
	void _AddRef() throw();
	void _Free() throw();
	int _Compare(const _bstr_t& str) const throw();
};

//////////////////////////////////////////////////////////////////////////////
//
// Constructors
//
//////////////////////////////////////////////////////////////////////////////

// Default constructor
//
inline _bstr_t::_bstr_t() throw()
	: m_Data(NULL)
{
}

// Copy constructor
//
inline _bstr_t::_bstr_t(const _bstr_t& s) throw()
	: m_Data(s.m_Data)
{
	_AddRef();
}

inline _bstr_t::_bstr_t(const char* s) throw(_com_error)
	: m_Data(new Data_t(s))
{
	if (m_Data == NULL) {
		_com_issue_error(E_OUTOFMEMORY);
	}
}

// Destructor
//
inline _bstr_t::~_bstr_t() throw()
{
	_Free();
}

//////////////////////////////////////////////////////////////////////////////
//
// Extractors
//
//////////////////////////////////////////////////////////////////////////////

// Extract a const char_t*
//
inline _bstr_t::operator const char*() const throw(_com_error)
{
	return (m_Data != NULL) ? m_Data->GetString() : NULL;
}

// Extract a char_t*
//
inline _bstr_t::operator char*() const throw(_com_error)
{
	return const_cast<char*>((m_Data != NULL) ? m_Data->GetString() : NULL);
}

// Extract a const wchar_t*
//
inline _bstr_t::operator const wchar_t*() const throw()
{
	return (const wchar_t*)((m_Data != NULL) ? m_Data->GetString() : NULL);
}

// Extract a wchar_t*
//
inline _bstr_t::operator wchar_t*() const throw()
{
	return (wchar_t*)((m_Data != NULL) ? m_Data->GetString() : NULL);
}

//////////////////////////////////////////////////////////////////////////////
//
// Low-level help functions
//
//////////////////////////////////////////////////////////////////////////////

// Extract a copy of the wrapped BSTR
//
inline BSTR _bstr_t::copy() const throw(_com_error)
{
	return (m_Data != NULL) ? m_Data->Copy() : NULL;
}

// Return the length of the wrapped BSTR
//
inline unsigned int _bstr_t::length() const throw()
{
	return (m_Data != NULL) ? m_Data->Length() : 0;
}

// AddRef the BSTR
//
inline void _bstr_t::_AddRef() throw()
{
	if (m_Data != NULL) {
		m_Data->AddRef();
	}
}

// Free the BSTR
//
inline void _bstr_t::_Free() throw()
{
	if (m_Data != NULL) {
		m_Data->Release();
		m_Data = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////////
//
// Reference counted wrapper - Constructors
//
//////////////////////////////////////////////////////////////////////////////

// Construct a Data_t from a const char*
//
inline _bstr_t::Data_t::Data_t(const char* s) throw(_com_error)
	: m_str(NULL), m_RefCount(1)
{
	m_wstr = _com_util::ConvertStringToBSTR(s);

	if (m_wstr == NULL && s != NULL) {
		_com_issue_error(E_OUTOFMEMORY);
	}
}

inline _bstr_t::Data_t::Data_t(const _bstr_t& s1, const _bstr_t& s2) throw(_com_error)
	: m_str(NULL), m_RefCount(1)
{
	const unsigned int l1 = s1.length();
	const unsigned int l2 = s2.length();

	m_wstr = ::SysAllocStringByteLen(NULL, (l1 + l2) * sizeof(char));

	if (m_wstr == NULL) {
		if (l1 + l2 == 0) {
			return;
		}
		_com_issue_error(E_OUTOFMEMORY);
	}

	const char* wstr1 = static_cast<const char*>(s1);

	if (wstr1 != NULL) {
		memcpy(m_wstr, wstr1, (l1 + 1) * sizeof(char));
	}

	const char* wstr2 = static_cast<const char*>(s2);

	if (wstr2 != NULL) {
		memcpy(m_wstr + l1, wstr2, (l2 + 1) * sizeof(char));
	}
}

//////////////////////////////////////////////////////////////////////////////
//
// Reference counted wrapper - reference counting routines
//
//////////////////////////////////////////////////////////////////////////////

inline unsigned long _bstr_t::Data_t::AddRef() throw()
{
	InterlockedIncrement(reinterpret_cast<long*>(&m_RefCount));
	return m_RefCount;
}

inline unsigned long _bstr_t::Data_t::Release() throw()
{
	if (!InterlockedDecrement(reinterpret_cast<long*>(&m_RefCount))) {
		delete this;
		return 0;
	}

	return m_RefCount;
}

//////////////////////////////////////////////////////////////////////////////
//
// Reference counted wrapper - extractors
//
//////////////////////////////////////////////////////////////////////////////

// Extract a const char_t*
//
inline _bstr_t::Data_t::operator const char*() const throw(_com_error)
{
	return GetString();
}

//////////////////////////////////////////////////////////////////////////////
//
// Reference counted wrapper - helper functions
//
//////////////////////////////////////////////////////////////////////////////

inline const char* _bstr_t::Data_t::GetString() const throw(_com_error)
{
	if (m_str == NULL) {
		m_str = _com_util::ConvertBSTRToString(m_wstr);

		if (m_str == NULL && m_wstr != NULL) {
			_com_issue_error(E_OUTOFMEMORY);
		}
	}

	return m_str;
}

// Return a copy of the wrapped BSTR
//
inline BSTR _bstr_t::Data_t::Copy() const throw(_com_error)
{
	if (m_wstr != NULL) {
		BSTR bstr = ::SysAllocStringByteLen(reinterpret_cast<char*>(m_wstr),
											::SysStringByteLen(m_wstr));

		if (bstr == NULL) {
			_com_issue_error(E_OUTOFMEMORY);
		}

		return bstr;
	}

	return NULL;
}

// Return the length of the wrapper BSTR
//
inline unsigned int _bstr_t::Data_t::Length() const throw()
{
	return m_wstr ? ::SysStringLen(m_wstr) : 0;
}

// Destruct this object
//
inline _bstr_t::Data_t::~Data_t() throw()
{
	_Free();
}

// Free up this object
//
inline void _bstr_t::Data_t::_Free() throw()
{
	if (m_wstr != NULL) {
		::SysFreeString(m_wstr);
	}

	if (m_str != NULL) {
		free(m_str);
	}
}

inline _bstr_t& _bstr_t::operator=(const _bstr_t& s) throw(_com_error)
{
	_Free();
	m_Data = new Data_t(s);

	return *this;
}

inline _bstr_t& _bstr_t::operator=(const char* s) throw(_com_error)
{
	_Free();
	m_Data = new Data_t(s);

	return *this;
}

inline _bstr_t& _bstr_t::operator+=(const _bstr_t& s) throw(_com_error)
{
	Data_t* newData = new Data_t(*this, s);

	_Free();
	m_Data = newData;

	return *this;
}

inline _bstr_t _bstr_t::operator+(const _bstr_t& s) const throw(_com_error)
{
	_bstr_t b = *this;
	b += s;

	return b;
}

#endif	// _INC_COMUTIL
