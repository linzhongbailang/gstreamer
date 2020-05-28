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
// "winbase.h" - Microsoft's 32-bit Base APIs

#ifndef _WINBASE_
#define _WINBASE_

#include <fcntl.h>

#define WINADVAPI
#define WINBASEAPI

#define INVALID_HANDLE_VALUE ((HANDLE)-1)
#define INVALID_FILE_SIZE    ((DWORD)-1)
#define INVALID_SET_FILE_POINTER ((DWORD)-1)

#define FILE_BEGIN   SEEK_SET
#define FILE_CURRENT SEEK_CUR
#define FILE_END     SEEK_END

#define WAIT_FAILED    (DWORD)0xFFFFFFFF
#define WAIT_OBJECT_0  ((STATUS_WAIT_0 ) + 0 )
#define WAIT_ABANDONED ((STATUS_ABANDONED_WAIT_0 ) + 0)
#define WAIT_TIMEOUT   STATUS_TIMEOUT
#define STILL_ACTIVE   STATUS_PENDING

#define FILE_FLAG_NO_BUFFERING 0      // not used
#define FILE_FLAG_SEQUENTIAL_SCAN 0   // not used

#define CREATE_NEW			O_CREAT
#define CREATE_ALWAYS		O_CREAT | O_TRUNC
#define OPEN_EXISTING  		0
#define OPEN_ALWAYS			-1
#define TRUNCATE_EXISTING	O_TRUNC

// Used for SetThreadPriority
#define THREAD_PRIORITY_LOWEST          THREAD_BASE_PRIORITY_MIN
#define THREAD_PRIORITY_BELOW_NORMAL    (THREAD_PRIORITY_LOWEST+1)
#define THREAD_PRIORITY_NORMAL          0
#define THREAD_PRIORITY_HIGHEST         THREAD_BASE_PRIORITY_MAX
#define THREAD_PRIORITY_ABOVE_NORMAL    (THREAD_PRIORITY_HIGHEST-1)
#define THREAD_PRIORITY_ERROR_RETURN    (MAXLONG)

#define THREAD_PRIORITY_TIME_CRITICAL   THREAD_BASE_PRIORITY_LOWRT
#define THREAD_PRIORITY_IDLE            THREAD_BASE_PRIORITY_IDLE

//
//  File structures
//

typedef struct tagOVERLAPPED {
    DWORD  Internal;
    DWORD  InternalHigh;
    DWORD  Offset;
    DWORD  OffsetHigh;
    HANDLE hEvent;
} OVERLAPPED, FAR *LPOVERLAPPED, *POVERLAPPED;

typedef struct _SECURITY_ATTRIBUTES {
	DWORD nLength;
	/* [size_is] */ LPVOID lpSecurityDescriptor;
	BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

//
//  File System time stamps are represented with the following structure:
//

typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

BOOL WINAPI GetProcessTimes(HANDLE hProcess,
							LPFILETIME lpCreationTime,
							LPFILETIME lpExitTime,
							LPFILETIME lpKernelTime,
							LPFILETIME lpUserTime);

//
// System time is represented with the following structure:
//

typedef struct _SYSTEMTIME	{
	WORD wYear;
	WORD wMonth;
	WORD wDayOfWeek;
	WORD wDay;
	WORD wHour;
	WORD wMinute;
	WORD wSecond;
	WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

VOID WINAPI GetLocalTime(LPSYSTEMTIME lpSystemTime);
VOID WINAPI GetSystemTime(LPSYSTEMTIME lpSystemTime);


typedef DWORD (WINAPI *PTHREAD_START_ROUTINE)(LPVOID lpThreadParameter);
typedef PTHREAD_START_ROUTINE LPTHREAD_START_ROUTINE;

typedef struct _MEMORYSTATUS {
    DWORD dwLength;
    DWORD dwMemoryLoad;
    DWORD dwTotalPhys;
    DWORD dwAvailPhys;
    DWORD dwTotalPageFile;
    DWORD dwAvailPageFile;
    DWORD dwTotalVirtual;
    DWORD dwAvailVirtual;
} MEMORYSTATUS, *LPMEMORYSTATUS;

VOID GlobalMemoryStatus(LPMEMORYSTATUS lpBuffer);

#define CREATE_SUSPENDED            0x00000004

#define INFINITE 0xFFFFFFFF

//#define NO_ASM
/* Use assembly version of atomic operations if possible, */
/* unless NO_ASM is defined */
#if  !defined(NO_ASM)
#if   defined(__i386__) && !defined(__APPLE__)
#define USE_INTEL_ASM 1
#elif defined(__mips__)
#define USE_MIPS_ASM  1
#endif
#endif

#if USE_MIPS_ASM
extern __inline__ void LNatomic_add(int i, long * v)
{
        unsigned long temp;

        __asm__ __volatile__(
                "1:   ll      %0, %1      # atomic_add\n"
                "     addu    %0, %2                  \n"
                "     sc      %0, %1                  \n"
                "     beqz    %0, 1b                  \n"
                : "=&r" (temp), "=m" (*v)
                : "Ir" (i), "m" (*v));
}

extern __inline__ void LNatomic_sub(int i, long * v)
{
        unsigned long temp;

        __asm__ __volatile__(
                "1:   ll      %0, %1      # atomic_sub\n"
                "     subu    %0, %2                  \n"
                "     sc      %0, %1                  \n"
                "     beqz    %0, 1b                  \n"
                : "=&r" (temp), "=m" (*v)
                : "Ir" (i), "m" (*v));
}

extern __inline__ unsigned long LNxchg_u32(long * m, unsigned long val)
{
        unsigned long dummy;

        __asm__ __volatile__(
                ".set\tpush\t\t\t\t# xchg_u32\n\t"
                ".set\tnoreorder\n\t"
                ".set\tnomacro\n\t" 
                "ll\t%0, %3\n"  
                "1:\tmove\t%2, %z4\n\t"
                "sc\t%2, %1\n\t"
                "beqzl\t%2, 1b\n\t"
                " ll\t%0, %3\n\t" 
                "sync\n\t"
                ".set\tpop"
                : "=&r" (val), "=m" (*m), "=&r" (dummy)
                : "R" (*m), "Jr" (val)
                : "memory");

        return val;
}
#endif

static inline long InterlockedIncrement(long *pLong)
	{
#if   USE_INTEL_ASM
	register int __res;
	__asm__ __volatile__ ("movl    $1,%0\n\t"
						  "lock    xadd %0,(%1)\n\t"
						  "inc     %0\n\t"
						  : "=a" (__res), "=r" (pLong)
						  : "1" (pLong));
	return __res;
#elif USE_MIPS_ASM
	LNatomic_add(1, pLong);
	return *pLong;
#else
	return ++(*pLong);
#endif
	}

static inline long InterlockedDecrement(long *pLong)
	{
#if   USE_INTEL_ASM
	register int __res;
	__asm__ __volatile__ ("movl    $0xffffffff,%0\n\t"
						  "lock    xadd %0,(%1)\n\t"
						  "dec     %0\n\t"
						  : "=a" (__res), "=r" (pLong)
						  : "1" (pLong));
	return __res;
#elif USE_MIPS_ASM
	LNatomic_sub(1, pLong);
	return *pLong;
#else
	return --(*pLong);
#endif
	}

static inline long InterlockedExchange(
  LPLONG Target, // pointer to the value to exchange
  LONG Value     // new value for Target
)
	{
#if   USE_INTEL_ASM
	register int __res;
	__asm__ __volatile__ ("movl    (%2),%0\n\t"
						  "1:\n\t"
						  "lock    cmpxchgl %3,(%1)\n\t"
						  "jne 1b\n\t"
						  : "=a" (__res), "=c" (Target):
						  "1" (Target), "d" (Value));
	return __res;
#elif USE_MIPS_ASM
	return (long)LNxchg_u32(Target, (unsigned long)Value);
#else
	long oldvalue = *Target;
	*Target = Value;
	return oldvalue;
#endif
	}
	
static inline long InterlockedExchangeAdd(
	LPLONG pLong,
	LONG Value
)
	{
#if   USE_INTEL_ASM
	register int __res;
	__asm__ __volatile__ ("movl    (%2),%0\n\t"
						  "lock    xadd %0,(%1)\n\t"
						  "movl    (%1),%0\n\t"
						  : "=a" (__res), "=r" (pLong)
						  : "1" (pLong));
	return __res;
#elif USE_MIPS_ASM
	if (Value >= 0)
		LNatomic_add((unsigned long)Value, pLong);
	else
		LNatomic_sub((unsigned long)Value, pLong);
	return *pLong;
#else
	*pLong += Value;
	return *pLong;
#endif
	}

#define lstrlen    strlen
#define lstrcpy    strcpy
#define lstrcpyn   strncpy
#define lstrcat    strcat
#define lstrcmpi   stricmp

// Thread-related functions

static inline BOOL VALID_HANDLE(HANDLE h) 
	{ return h && h!=INVALID_HANDLE_VALUE; }

HANDLE CreateThread(LPSECURITY_ATTRIBUTES unusedlpThreadAttributes,
					SIZE_T unused_dwStackSize,
					LPTHREAD_START_ROUTINE lpStartAddress,
					LPVOID lpParameter, 
					DWORD dwCreationFlags,
					LPDWORD lpThreadId);
HANDLE GetCurrentThread(void);
DWORD  GetCurrentThreadId(void);  // Linux implementation returns HANDLE here
BOOL   TerminateThread(HANDLE hThread, DWORD dwExitCode);
VOID   ExitThread(DWORD dwExitCode);
BOOL   GetExitCodeThread(HANDLE hThread, LPDWORD lpExitCode);
DWORD  SuspendThread(HANDLE hThread);
DWORD  ResumeThread(HANDLE hThread);

DWORD  WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
DWORD  WaitForMultipleObjects(DWORD nCount, CONST HANDLE *lpHandles,
							  BOOL fWaitAll, DWORD dwMilliseconds);

enum { MUTEX_ALL_ACCESS, SYNCHRONIZE };
HANDLE OpenMutex(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCTSTR lpName);
HANDLE CreateMutex(LPSECURITY_ATTRIBUTES lpMutexAttributes,	BOOL bInitialOwner,
				   LPCSTR lpName);
BOOL   ReleaseMutex(HANDLE hMutex);

HANDLE CreateSemaphore(HANDLE unused, LONG lInitialCount, 
					   LONG lMaximumCount, LPCTSTR lpName);
BOOL   ReleaseSemaphore(HANDLE hSemaphore, LONG lReleaseCount,
						LPLONG lpPreviousCount);

HANDLE CreateEvent(HANDLE unused, BOOL bManualReset,
				   BOOL bInitialState, LPCTSTR lpName);
BOOL   SetEvent(HANDLE hEvent);
BOOL   ResetEvent(HANDLE hEvent);

// File-related functions

HANDLE CreateFile(LPCTSTR lpFileName, DWORD dwDesiredAccess,
				  DWORD dwShareMode, HANDLE unused1,
				  DWORD dwCreationDisposition,
				  DWORD dwFlagsAndAttributes, HANDLE unused2);
BOOL   DeleteFile(LPCTSTR lpFileName);
DWORD  GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh);
DWORD  SetFilePointer(HANDLE hFile, LONG lDistanceToMove,
					  PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod);
static inline DWORD GetLastError(void) { return errno; /* NO_ERROR == 0 */ }
BOOL   ReadFile (HANDLE hFile, LPVOID lpBuffer,
				 DWORD nNumberOfBytesToRead,
				 LPDWORD lpNumberOfBytesRead,
				 LPOVERLAPPED lpOverLapped);
BOOL   WriteFile(HANDLE hFile, LPCVOID lpBuffer,
				 DWORD nNumberOfBytesToWrite,
				 LPDWORD lpNumberOfBytesWritten,
				 HANDLE unused);
BOOL   FlushFileBuffers(HANDLE hFile);
BOOL   CreateDirectory(LPCTSTR strPathName, 
					   LPSECURITY_ATTRIBUTES lpSecurityAttribute);
BOOL   CloseHandle(HANDLE hObject);

BOOL  CreatePipe(HANDLE* hReadPipe, HANDLE* hWritePipe, LPSECURITY_ATTRIBUTES lpPipeAttributes, DWORD nSize);

BOOL   GetVolumeInformation(
  LPCTSTR lpRootPathName,           // root directory
  LPTSTR  lpVolumeNameBuffer,       // volume name buffer
  DWORD   nVolumeNameSize,          // length of name buffer
  LPDWORD lpVolumeSerialNumber,     // volume serial number
  LPDWORD lpMaximumComponentLength, // maximum file name length
  LPDWORD lpFileSystemFlags,        // file system options
  LPTSTR  lpFileSystemNameBuffer,   // file system name buffer
  DWORD   nFileSystemNameSize       // length of file system name buffer
);

// memory mapping

HANDLE CreateFileMapping(HANDLE hFile, LPSECURITY_ATTRIBUTES lpAttributes,
						 DWORD flProtect, DWORD dwMaximumSizeHigh,
						 DWORD dwMaximumSizeLow, LPCTSTR lpName); 

#define FILE_MAP_READ PROT_READ

LPVOID MapViewOfFile(HANDLE hFileMappingObject, DWORD dwDesiredAccess,
					 DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow,
					 SIZE_T dwNumberOfBytesToMap);

BOOL UnmapViewOfFile(LPVOID lpBaseAddress);

// IVI Extension
BOOL UnmapViewOfFileEx(LPVOID lpBaseAddress, SIZE_T dwNumberOfBytes);

// (not implemented)
int  GetThreadPriority(HANDLE hThread);// { return 0; /*"normal"*/ }
BOOL SetThreadPriority(HANDLE hThread, int nPriority); 
	                                                 //{return FALSE;}
static inline void DisableThreadLibraryCalls(HINSTANCE hInstance) {}

// (not implemented)
HINSTANCE LoadLibrary(LPCTSTR lpLibFileName);
BOOL      FreeLibrary(HMODULE hLibModule);
FARPROC   GetProcAddress(HMODULE hModule, LPCSTR lpProcName);

// process control

DWORD GetCurrentProcessId(void);

static inline VOID ExitProcess(UINT uExitCode) { exit(uExitCode); }

//
// Power Management APIs
//

#define AC_LINE_OFFLINE                 0x00
#define AC_LINE_ONLINE                  0x01
#define AC_LINE_BACKUP_POWER            0x02
#define AC_LINE_UNKNOWN                 0xFF

#define BATTERY_FLAG_HIGH               0x01
#define BATTERY_FLAG_LOW                0x02
#define BATTERY_FLAG_CRITICAL           0x04
#define BATTERY_FLAG_CHARGING           0x08
#define BATTERY_FLAG_NO_BATTERY         0x80
#define BATTERY_FLAG_UNKNOWN            0xFF

#define BATTERY_PERCENTAGE_UNKNOWN      0xFF

#define BATTERY_LIFE_UNKNOWN        0xFFFFFFFF

typedef struct _SYSTEM_POWER_STATUS {
    BYTE ACLineStatus;
    BYTE BatteryFlag;
    BYTE BatteryLifePercent;
    BYTE Reserved1;
    DWORD BatteryLifeTime;
    DWORD BatteryFullLifeTime;
}   SYSTEM_POWER_STATUS, *LPSYSTEM_POWER_STATUS;

BOOL WINAPI GetSystemPowerStatus(LPSYSTEM_POWER_STATUS lpSystemPowerStatus);
BOOL WINAPI SetSystemPowerState (BOOL fSuspend, BOOL fForce);

#define LMEM_FIXED          0x0000
#define LMEM_ZEROINIT       0x0040
#define LPTR                (LMEM_FIXED | LMEM_ZEROINIT)
HLOCAL WINAPI LocalAlloc(UINT uFlags, UINT uBytes);
HLOCAL WINAPI LocalFree(HLOCAL hMem);

#endif /* _WINBASE_ */
