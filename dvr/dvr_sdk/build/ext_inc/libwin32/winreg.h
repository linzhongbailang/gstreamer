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
// Windows 32-Bit registry API

#ifndef _WINREG_
#define _WINREG_

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <sys/stat.h>

#include "windows.h"
#include "winerror.h"

DECLARE_HANDLE(HKEY);
typedef HKEY *PHKEY;

typedef char* PFILENAME;
typedef long  REGSAM ;
typedef long  SECURITY_INFORMATION ;
typedef void* PSECURITY_DESCRIPTOR ;
typedef void* PVALENT;

extern HKEY HKEY_CLASSES_ROOT;
extern HKEY HKEY_CURRENT_CONFIG;
extern HKEY HKEY_CURRENT_USER;
extern HKEY HKEY_LOCAL_MACHINE;
extern HKEY HKEY_USERS;
extern HKEY HKEY_PERFORMANCE_DATA;
extern HKEY HKEY_DYN_DATA;

enum {
REG_OPTION_NON_VOLATILE,
REG_OPTION_VOLATILE,
REG_OPTION_BACKUP_RESTORE,
};

enum {
REG_CREATED_NEW_KEY,
REG_OPENED_EXISTING_KEY
};

enum REG_VAL_TYPE { REG_BINARY, REG_DWORD, REG_DWORD_LITTLE_ENDIAN, 
					REG_DWORD_BIG_ENDIAN, REG_EXPAND_SZ, REG_LINK, 
					REG_MULTI_SZ, REG_NONE, REG_QWORD, REG_QWORD_LITTLE_ENDIAN,
					REG_RESOURCE_LIST, REG_SZ };

#define FILETIME time_t
#define PFILETIME void *

/* WIN32 Registry APIs */
LONG RegCloseKey(
	HKEY hKey
);

LONG RegCreateKey(
	HKEY hKey, 
	LPCTSTR lpSubKey, 
	PHKEY phkResult
);

LONG RegCreateKeyEx(
	HKEY hKey, 
	LPCTSTR lpSubKey, 
	DWORD Reserved,
	LPTSTR lpClass,
	DWORD dwOptions,
	REGSAM samDesired,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	PHKEY phkResult,
	LPDWORD lpdwDisposition
);

LONG RegOpenKey(
	HKEY hKey,
	LPCTSTR lpSubKey,
	PHKEY phkResult
);

LONG RegOpenKeyEx(
	HKEY hKey,
	LPCTSTR lpSubKey,
	DWORD ulOptions,
	REGSAM samDesired,
	PHKEY phkResult
);

LONG RegConnectRegistry(
	LPCTSTR lpMachineName,
	HKEY hKey,
	PHKEY phkResult
);

LONG RegDeleteKey(
	HKEY hKey,
	LPCTSTR lpSubKey
);

LONG RegDeleteValue(
	HKEY hKey,
	LPCTSTR lpValueName
);

LONG RegDisablePredefinedCache();

LONG RegEnumKey(
	HKEY hKey,
	DWORD dwIndex,
	LPTSTR lpName,
	DWORD cbName
);

LONG RegEnumKeyEx(
	HKEY hKey,
	DWORD dwIndex,
	LPTSTR lpName,
	LPDWORD lpcName,
	LPDWORD lpReserved,
	LPTSTR lpClass,
	LPDWORD lpcClass,
	PFILETIME lpftLastWriteTime
);

LONG RegEnumValue(
	HKEY hKey,
	DWORD dwIndex,
	LPTSTR lpValueName,
	LPDWORD lpcValueName,
	LPDWORD lpReserved,
	LPDWORD lpType,
	LPBYTE lpData,
	LPDWORD lpcbData
);

LONG RegFlushKey(
	HKEY hKey
);

LONG RegGetKeySecurity(
	HKEY hKey,
	SECURITY_INFORMATION SecurityInformation,
	PSECURITY_DESCRIPTOR pSecurityDescriptor,
	LPDWORD lpcbSecurityDescriptor
);

LONG RegLoadKey(
	HKEY hKey,
	LPCTSTR lpSubKey,
	LPCTSTR lpFile
);


LONG RegNotifyChangeKeyValue(
	HKEY hKey,
	BOOL bWatchSubTree,
	DWORD dwNotifyFilter,
	HANDLE hEvent,
	BOOL fAsynchronous
);

LONG RegOpenCurrentUser(
	REGSAM samDesired,
	PHKEY phkResult
);

LONG RegOpenKeyEx(
	HKEY hKey,
	LPCTSTR lpSubKey,
	DWORD ulOptions,
	REGSAM samDesired,
	PHKEY phkResult
);

LONG RegOpenUserClassesRoot(
	HANDLE hToken,
	DWORD dwOptions,
	REGSAM samDesired,
	PHKEY phkResult
);

LONG RegOverridePredefKey(
	HKEY hKey,
	HKEY hNewHKey
);

LONG RegQueryInfoKey ( 
	HKEY hKey,
	LPTSTR lpClass,
	LPDWORD lpcClass,
	LPDWORD lpReserved,
	LPDWORD lpcSubkeys,
	LPDWORD lpcMaxSubKeyLen,
	LPDWORD lpcMaxClassLen,
	LPDWORD lpcValues,
	LPDWORD lpcMaxValueNameLen,
	LPDWORD lpcMaxValueLen,
	LPDWORD lpcbSecurityDescriptor,
	PFILENAME lpfrLastWriteTime
);

LONG RegQueryMultipleValues(
	HKEY hKey,
	PVALENT val_list,
	DWORD num_vals,
	LPTSTR lpValueBuf,
	LPDWORD ldwTotSize
);

LONG RegQueryValueEx(
	HKEY hKey,
	LPCTSTR lpValueName,
	LPDWORD lpReserved,
	LPDWORD lpType,
	LPBYTE lpData,
	LPDWORD lpcbData
);

LONG RegReplaceKey(
	HKEY hKey,
	LPCTSTR lpSubKey,
	LPCTSTR lpNewFile,
	LPCTSTR lpOldFile
);

LONG RestoreKey(
	HKEY hKey,
	LPCTSTR lpFile,
	DWORD dwFlags
);

LONG RegSaveKey(
	HKEY hKey,
	LPCTSTR lpFile,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes
);

LONG RegSetKeySecurity(
	HKEY hKey,
	SECURITY_INFORMATION SecurityInformation,
	PSECURITY_DESCRIPTOR pSecurityDescriptor
);

LONG RegSetValueEx(
	HKEY hKey,
	LPCTSTR lpValueName,
	DWORD Reserved,
	DWORD dwType,
	CONST BYTE *lpData,
	DWORD cbData
);

LONG RegUnLoadKey(
	HKEY hKey,
	LPCTSTR lpSubKey
);

#ifdef __cplusplus
}
#endif

#endif
