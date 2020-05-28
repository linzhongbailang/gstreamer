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

#ifndef _DVRMONITOR_MOUNT_H_
#define _DVRMONITOR_MOUNT_H_

#include <pthread.h>
#ifdef __linux__
#include <sys/poll.h>
#endif

#include <list>
#include <map>
#include <string>
#include <DVR_SDK_DEF.h>

#include "osa_tsk.h"
#include "osa_notifier.h"
#include "osa.h"

#include "DvrSystemControl.h"

class DvrMonitorMount
{
public:
	DvrMonitorMount(OsaNotifier *notifier, void *sysCtrl);
	~DvrMonitorMount();

	void StartMonitor(std::list<DVR_DEVICE> *pDevices);
	void StopMonitor();

	void NotifyFsCheck(DVR_DEVICE_TYPE devType, const char *mountPoint, const char *dev, int intfClass);
	void NotifyMount(DVR_DEVICE_TYPE devType, const char *mountPoint, const char *dev, int intfClass);
	void NotifyMountFailure(DVR_DEVICE_TYPE devType, const char *mountPoint, const char *dev);
	void NotifyUnMount(DVR_DEVICE_TYPE devType, const char *mountPoint, const char *dev);
	void PurgeMountPoint();
	void EnumUSBDevice(std::list<DVR_DEVICE> *pDevices);
	int  GetUSBInterfaceClass(const char *intf);
	void GetUSBHDDevice(const char *mountPoint, char *dev);
	int SetMountStatus(bool bIsMounted)
	{
		m_bHasMounted = bIsMounted;
	}

private:
	DISABLE_COPY(DvrMonitorMount)

	void PurgeDeviceNode();
	void SetupSystemParameters();

	static void Monitor_MountTaskEntry(void *ptr);
	static void Monitor_MountTaskEntry_TFCard(void *ptr);

	OSA_TskHndl m_hThread;
	OSA_TskHndl m_hThread_TFCard;

	OsaNotifier *notifier;

	int m_hubLevel;
	int m_fd;
	int m_wd;
	struct pollfd m_pfd;

	DvrSystemControl *m_sysCtrl;
	char m_szDevice[PATH_MAX];
    char m_emmcDevice[PATH_MAX];
	bool m_bHasMounted;
    bool m_bRemove;
	bool m_bErrFormatPlugIn;
};

#endif
