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
// comdef.h - Native C++ compiler COM support - main definitions header

#if !defined(_INC_COMDEF)
#define _INC_COMDEF

#include <ole2.h>
#include <ocidl.h>  // called from #include <olectl.h>

#include <comutil.h>

#if 1

class _com_error {
};

#else

class _com_error {
public:
	// Constructors
	//
	_com_error(HRESULT hr,
               IErrorInfo* perrinfo = NULL,
               bool fAddRef = false) throw();
	_com_error(const _com_error& that) throw();

	// Destructor
	//
	virtual ~_com_error() throw();

	// Assignment operator
	//
	_com_error& operator=(const _com_error& that) throw();

	// Accessors
	//
	HRESULT Error() const throw();
	WORD WCode() const throw();
	IErrorInfo * ErrorInfo() const throw();

	// IErrorInfo method accessors
	//
	_bstr_t Description() const throw(_com_error);
	DWORD HelpContext() const throw();
	_bstr_t HelpFile() const throw(_com_error);
	_bstr_t Source() const throw(_com_error);
	GUID GUID() const throw();

	// FormatMessage accessors
	const TCHAR * ErrorMessage() const throw();

	// EXCEPINFO.wCode <-> HRESULT mappers
	static HRESULT WCodeToHRESULT(WORD wCode) throw();
	static WORD HRESULTToWCode(HRESULT hr) throw();

private:
	enum {
		WCODE_HRESULT_FIRST = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x200),
		WCODE_HRESULT_LAST = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF+1, 0) - 1
	};
	const HRESULT			m_hresult;
	IErrorInfo *			m_perrinfo;
	mutable TCHAR *			m_pszMsg;
};

inline _com_error::_com_error(HRESULT hr,
                              IErrorInfo* perrinfo,
                              bool fAddRef) throw()
	: m_hresult(hr), m_perrinfo(perrinfo), m_pszMsg(NULL)
{
	if (m_perrinfo != NULL && fAddRef) {
		m_perrinfo->AddRef();
	}
}

inline _com_error::_com_error(const _com_error& that) throw()
	: m_hresult(that.m_hresult), m_perrinfo(that.m_perrinfo), m_pszMsg(NULL)
{
	if (m_perrinfo != NULL) {
		m_perrinfo->AddRef();
	}
}

inline _com_error::~_com_error() throw()
{
	if (m_perrinfo != NULL) {
		m_perrinfo->Release();
	}
	if (m_pszMsg != NULL) {
		LocalFree((HLOCAL)m_pszMsg);
	}
}

inline _com_error& _com_error::operator=(const _com_error& that) throw()
{
	if (this != &that) {
		this->_com_error::~_com_error();
		this->_com_error::_com_error(that);
	}
	return *this;
}

inline HRESULT _com_error::Error() const throw()
{
	return m_hresult;
}

inline WORD _com_error::WCode() const throw()
{
	return HRESULTToWCode(m_hresult);
}

inline IErrorInfo * _com_error::ErrorInfo() const throw()
{
	if (m_perrinfo != NULL) {
		m_perrinfo->AddRef();
	}
	return m_perrinfo;
}

inline _bstr_t _com_error::Description() const throw(_com_error)
{
	BSTR bstr = NULL;
	if (m_perrinfo != NULL) {
		m_perrinfo->GetDescription(&bstr);
	}
	return _bstr_t(bstr, false);
}

inline DWORD _com_error::HelpContext() const throw()
{
	DWORD dwHelpContext = 0;
	if (m_perrinfo != NULL) {
		m_perrinfo->GetHelpContext(&dwHelpContext);
	}
	return dwHelpContext;
}

inline _bstr_t _com_error::HelpFile() const throw(_com_error)
{
	BSTR bstr = NULL;
	if (m_perrinfo != NULL) {
		m_perrinfo->GetHelpFile(&bstr);
	}
	return _bstr_t(bstr, false);
}

inline _bstr_t _com_error::Source() const throw(_com_error)
{
	BSTR bstr = NULL;
	if (m_perrinfo != NULL) {
		m_perrinfo->GetSource(&bstr);
	}
	return _bstr_t(bstr, false);
}

inline _GUID _com_error::GUID() const throw()
{
	_GUID guid;
	memcpy(&guid, &__uuidof(NULL), sizeof(_GUID));
	if (m_perrinfo != NULL) {
		m_perrinfo->GetGUID(&guid);
	}
	return guid;
}

inline const TCHAR * _com_error::ErrorMessage() const throw()
{
	if (m_pszMsg == NULL) {
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
					  NULL,
					  m_hresult,
					  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					  (LPTSTR)&m_pszMsg,
					  0,
					  NULL );
		if (m_pszMsg != NULL) {
			int nLen = lstrlen(m_pszMsg);
			if (nLen > 1 && m_pszMsg[nLen - 1] == '\n') {
				m_pszMsg[nLen - 1] = 0;
				if (m_pszMsg[nLen - 2] == '\r') {
				        m_pszMsg[nLen - 2] = 0;
				}
			}
		} else {
			m_pszMsg = (LPTSTR)LocalAlloc(0, 32 * sizeof(TCHAR));
			if (m_pszMsg != NULL) {
				WORD wCode = WCode();
				if (wCode != 0) {
					wsprintf(m_pszMsg, TEXT("IDispatch error #%d"), wCode);
				} else {
					wsprintf(m_pszMsg, TEXT("Unknown error 0x%0lX"), m_hresult);
				}
			}
		}
	}
	return m_pszMsg;
}

inline HRESULT _com_error::WCodeToHRESULT(WORD wCode) throw()
{
	return wCode >= 0xFE00 ? WCODE_HRESULT_LAST : WCODE_HRESULT_FIRST + wCode;
}

inline WORD _com_error::HRESULTToWCode(HRESULT hr) throw()
{
	return (hr >= WCODE_HRESULT_FIRST && hr <= WCODE_HRESULT_LAST)
		? WORD(hr - WCODE_HRESULT_FIRST)
		: 0;
}
#endif

#endif  /* _INC_COMDEF */

