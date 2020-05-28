/*===========================================================================*\
* Copyright 2003 O-Film Technologies, Inc., All Rights Reserved.
* O-Film Confidential
*
* DESCRIPTION:
*
* ABBREVIATIONS:
*   TODO: List of abbreviations used, or reference(s) to external document(s)
*
* TRACEABILITY INFO:
*   Design Document(s):
*     TODO: Update list of design document(s)
*
*   Requirements Document(s):
*     TODO: Update list of requirements document(s)
*
*   Applicable Standards (in order of precedence: highest first):
*
* DEVIATIONS FROM STANDARDS:
*   TODO: List of deviations from standards in this file, or
*   None.
*
\*===========================================================================*/

#ifndef _DVRMONITOR_MOUNT_WIN32_H_
#define _DVRMONITOR_MOUNT_WIN32_H_

#include "DVR_SDK_DEF.h"

#include "osa_tsk.h"
#include "osa_notifier.h"
#include "osa.h"

#include <list>
#include <map>

class DvrDeviceChangeMonitor;

class DvrMonitorMount_Win32
{
public:
	DvrMonitorMount_Win32(OsaNotifier *notifier);
	~DvrMonitorMount_Win32();

	void StartMonitor(std::list<DVR_DEVICE> *pDevices);
	void StopMonitor();

	void NotifyMount(DVR_DEVICE_TYPE devType, const char *mountPoint);
	void NotifyMountFailure(DVR_DEVICE_TYPE devType, const char *devPath);
	void NotifyUnMount(DVR_DEVICE_TYPE devType, const char *mountPoint);
	int EnumUSBDevices(void* GUID);
	bool IsUsbDevice(wchar_t letter);

	void SetHandle(HWND hWnd);
	HWND GetHandle(void);
	BOOL      m_running;
private:
	DISABLE_COPY(DvrMonitorMount_Win32)

	OsaNotifier *notifier;

	OSA_TskHndl m_hThread;

	HWND      m_hwnd;
};

#endif
