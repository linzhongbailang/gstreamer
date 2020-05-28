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
#ifdef WIN32

#include <windows.h>
#include <iostream>
#include <string>
#include <tchar.h>
#include <setupapi.h>
#include <devguid.h>
#include <Winbase.h>
#include <Dbt.h>

#include <dprint.h>
#include "osa_notifier.h"

#include "DvrMonitor_Mount_Win32.h"

#ifdef _MSC_VER
#pragma warning(disable:4996)	// 4706: assignment within conditional expression
#endif

static void Monitor_MountTaskEntry(void *ptr);

static const char * const VFAT_MOUNT_OPTION = "iocharset=utf8";
static const char * const NTFS_MOUNT_OPTION = "nls=utf8";

static LRESULT CALLBACK WinProc(HWND hwnd, UINT uMsg, WPARAM wp, LPARAM lp);

static const GUID GUID_DEVINTERFACE = { 0xA5DCBF10, 0x6530, 0x11D2, { 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED } }; // usbÉè±¸guid

static const GUID FATFS_MOUNT_GUID = { 0x169e1941, 0x4ce, 0x4690, { 0x97, 0xac, 0x77, 0x61, 0x87, 0xeb, 0x67, 0xcc } };

// Compute Device Class: this is used to get the tree control root icon
static const GUID GUID_DEVCLASS_COMPUTER =
{ 0x4D36E966, 0xE325, 0x11CE, { 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18 } };

// Copy from HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\DeviceClasses
static const GUID GUID_DEVINTERFACE_LIST[] =
{
	// GUID_DEVINTERFACE_USB_DEVICE
	{ 0xA5DCBF10, 0x6530, 0x11D2, { 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED } },

	// GUID_DEVINTERFACE_DISK
	{ 0x53f56307, 0xb6bf, 0x11d0, { 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b } },

	// GUID_DEVINTERFACE_HID, 
	{ 0x4D1E55B2, 0xF16F, 0x11CF, { 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } },

	// GUID_NDIS_LAN_CLASS
	{ 0xad498944, 0x762f, 0x11d0, { 0x8d, 0xcb, 0x00, 0xc0, 0x4f, 0xc3, 0x35, 0x8c } }

	//// GUID_DEVINTERFACE_COMPORT
	//{ 0x86e0d1e0, 0x8089, 0x11d0, { 0x9c, 0xe4, 0x08, 0x00, 0x3e, 0x30, 0x1f, 0x73 } },

	//// GUID_DEVINTERFACE_SERENUM_BUS_ENUMERATOR
	//{ 0x4D36E978, 0xE325, 0x11CE, { 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18 } },

	//// GUID_DEVINTERFACE_PARALLEL
	//{ 0x97F76EF0, 0xF883, 0x11D0, { 0xAF, 0x1F, 0x00, 0x00, 0xF8, 0x00, 0x84, 0x5C } },

	//// GUID_DEVINTERFACE_PARCLASS
	//{ 0x811FC6A5, 0xF728, 0x11D0, { 0xA5, 0x37, 0x00, 0x00, 0xF8, 0x75, 0x3E, 0xD1 } }
};

class DvrDeviceChangeMonitor
{
public:
	DvrDeviceChangeMonitor();
	~DvrDeviceChangeMonitor();

	static LRESULT CALLBACK Proc(HWND hwnd, UINT uMsg, WPARAM wp, LPARAM lp);

	static HWND m_hwnd;
};

DvrMonitorMount_Win32::DvrMonitorMount_Win32(OsaNotifier *notifier)
{
	this->notifier = notifier;

	m_hwnd = NULL;
}

DvrMonitorMount_Win32::~DvrMonitorMount_Win32()
{
	StopMonitor();
}

void DvrMonitorMount_Win32::StartMonitor(std::list<DVR_DEVICE> *pDevices)
{
	int nPos = 0;
	DWORD dwDriveList;
	DVR_DEVICE_TYPE devType = DVR_DEVICE_TYPE_SD;
	CHAR  chDisk;

	dwDriveList = GetLogicalDrives();
	while (dwDriveList) {
		if (dwDriveList & 0x1) {
			char dstpath[PATH_MAX] = { 0 };

			chDisk = 'A' + nPos;

			dstpath[0] = chDisk;
			strcat(dstpath, ":\\");

			bool isUsb = IsUsbDevice(chDisk);
			if (isUsb)
			{
				DVR_DEVICE device;
				memset(&device, 0, sizeof(device));

				device.eType = DVR_DEVICE_TYPE_USBHD;
				strcpy(device.szMountPoint, dstpath);
				pDevices->push_back(device);
			}

			unsigned driveType = GetDriveType((LPCTSTR)dstpath);
			if (driveType == DRIVE_REMOVABLE) {
				devType = DVR_DEVICE_TYPE_USBHD;
				NotifyMount(devType, dstpath);
			}
		}

		++nPos;
		if (nPos > 10)
			break;
		dwDriveList = dwDriveList >> 1;
	}

	EnumUSBDevices((LPGUID)&GUID_DEVINTERFACE);

	DVR::OSA_tskCreate(&m_hThread, Monitor_MountTaskEntry, this);
}

void DvrMonitorMount_Win32::StopMonitor()
{
	if (IsWindow(GetHandle())) {
		DestroyWindow(GetHandle());
		SetHandle(NULL);
	}
}

void DvrMonitorMount_Win32::NotifyMount(DVR_DEVICE_TYPE devType, const char *mountPoint)
{
	DVR_DEVICE_EVENT deviceEvent = DVR_DEVICE_EVENT_PLUGIN;

	DVR_DEVICE device;
	memset(&device, 0, sizeof(device));

	device.eType = devType;
	strcpy(device.szMountPoint, mountPoint);

	OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_DEVICEEVENT, (void *)&deviceEvent, (long)sizeof(deviceEvent),
		(void *)&device, (long)sizeof(device), notifier);
}

void DvrMonitorMount_Win32::NotifyMountFailure(DVR_DEVICE_TYPE devType, const char *devPath)
{
	DVR_DEVICE_EVENT deviceEvent = DVR_DEVICE_EVENT_ERROR;

	DVR_DEVICE device;
	memset(&device, 0, sizeof(device));

	device.eType = devType;
	strcpy(device.szMountPoint, devPath);

	OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_DEVICEEVENT, (void *)&deviceEvent, (long)sizeof(deviceEvent),
		(void *)&device, (long)sizeof(device), notifier);
}

void DvrMonitorMount_Win32::NotifyUnMount(DVR_DEVICE_TYPE devType, const char *mountPoint)
{
	DVR_DEVICE_EVENT deviceEvent = DVR_DEVICE_EVENT_PLUGOUT;

	DVR_DEVICE device;
	memset(&device, 0, sizeof(device));

	device.eType = devType;
	strcpy(device.szMountPoint, mountPoint);

	OsaNotifier::SendNotification(DVR_NOTIFICATION_TYPE_DEVICEEVENT, (void *)&deviceEvent, (long)sizeof(deviceEvent),
		(void *)&device, (long)sizeof(device), notifier);
}

void DvrMonitorMount_Win32::SetHandle(HWND hWnd)
{
	m_hwnd = hWnd;
}

HWND DvrMonitorMount_Win32::GetHandle(void)
{
	return m_hwnd;
}

bool DvrMonitorMount_Win32::IsUsbDevice(wchar_t letter)
{
	wchar_t volumeAccessPath[] = L"\\\\.\\X:";
	volumeAccessPath[4] = letter;

	HANDLE deviceHandle = CreateFileW(
		volumeAccessPath,
		0,                // no access to the drive
		FILE_SHARE_READ | // share mode
		FILE_SHARE_WRITE,
		NULL,             // default security attributes
		OPEN_EXISTING,    // disposition
		0,                // file attributes
		NULL);            // do not copy file attributes

	// setup query
	STORAGE_PROPERTY_QUERY query;
	memset(&query, 0, sizeof(query));
	query.PropertyId = StorageDeviceProperty;
	query.QueryType = PropertyStandardQuery;

	// issue query
	DWORD bytes;
	STORAGE_DEVICE_DESCRIPTOR devd;
	STORAGE_BUS_TYPE busType = BusTypeUnknown;

	if (DeviceIoControl(deviceHandle,
		IOCTL_STORAGE_QUERY_PROPERTY,
		&query, sizeof(query),
		&devd, sizeof(devd),
		&bytes, NULL))
	{
		busType = devd.BusType;
	}
	else
	{
		std::cout << "Failed to define bus type for: " << letter << "\n";
	}

	CloseHandle(deviceHandle);

	return BusTypeUsb == busType;
}

int DvrMonitorMount_Win32::EnumUSBDevices(void* GUID)
{
	HDEVINFO hDevInfo;
	SP_DEVINFO_DATA DeviceInfoData;
	DWORD i;

	if (GUID == NULL) {
		return -1;
	}

	// Create a HDEVINFO with all present devices.
	hDevInfo = SetupDiGetClassDevs((LPGUID)GUID,
		0, // Enumerator
		0,
		DIGCF_PRESENT | DIGCF_ALLCLASSES);

	if (hDevInfo == INVALID_HANDLE_VALUE) {
		// Insert error handling here.
		return 1;
	}

	// Enumerate through all devices in Set.
	DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	for (i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData); i++) {
		DWORD nSize = 0;
		TCHAR buf[MAX_PATH] = { 0 };
		int nVendor, nProduct;

		if (SetupDiGetDeviceInstanceId(hDevInfo, &DeviceInfoData, buf, sizeof(buf), &nSize)) {
			//             if (IsAppleDevice(buf, &nVendor, &nProduct)) {
			//                 NotifyIpodPlugin(nVendor, nProduct);
			//             }
		}
	}

	//  Cleanup
	SetupDiDestroyDeviceInfoList(hDevInfo);
	return 0;
}

static void Monitor_MountTaskEntry(void *ptr)
{
	HWND hWnd;
	DvrMonitorMount_Win32* p = (DvrMonitorMount_Win32*)ptr;
	WNDCLASS wc;

	wc.lpszClassName = TEXT("DeviceChangeMsgReceiverWnd");
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.hCursor = NULL;
	wc.hIcon = NULL;
	wc.hInstance = GetModuleHandle(NULL);
	wc.style = 0;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.lpszMenuName = 0;
	wc.lpfnWndProc = WinProc;

	RegisterClass(&wc);

	hWnd = CreateWindow(TEXT("DeviceChangeMsgReceiverWnd"),
		TEXT("DeviceChangeMsgReceiverWnd"),
		0,
		0, 0, 0, 0,
		NULL,
		NULL,
		GetModuleHandle(NULL),
		NULL);

	if (!hWnd) {
		return;
	}
	p->SetHandle(hWnd);
#pragma warning (suppress : 4311)
	SetWindowLong(hWnd, GWL_USERDATA, (LONG)p);

	HDEVNOTIFY hDevNotify;
	DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
	ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
	NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	for (int i = 0; i < sizeof(GUID_DEVINTERFACE_LIST) / sizeof(GUID); i++) {
		NotificationFilter.dbcc_classguid = GUID_DEVINTERFACE_LIST[i];
		hDevNotify = RegisterDeviceNotification(hWnd, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
		if (!hDevNotify) {
			hDevNotify = NULL;
		}
	}

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return;
}

static char FirstDriveFromMask(ULONG unitmask)
{
	char   i;

	for (i = 0; i < 26; ++i) {
		if (unitmask & 0x1)
			break;
		unitmask = unitmask >> 1;
	}

	return (i + 'A');
}

static LRESULT CALLBACK WinProc(HWND hwnd, UINT uMsg, WPARAM wp, LPARAM lp)
{
#pragma warning (suppress : 4312)
	DvrMonitorMount_Win32* p = (DvrMonitorMount_Win32*)GetWindowLong(hwnd, GWL_USERDATA);
	std::wstring szDevId;

	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_DEVICECHANGE:
		if (DBT_DEVICEARRIVAL == wp || DBT_DEVICEREMOVECOMPLETE == wp)
		{
			PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lp;
			PDEV_BROADCAST_DEVICEINTERFACE pDevInf;
			PDEV_BROADCAST_HANDLE pDevHnd;
			PDEV_BROADCAST_OEM pDevOem;
			PDEV_BROADCAST_PORT pDevPort;
			PDEV_BROADCAST_VOLUME pDevVolume;
			CHAR dstpath[PATH_MAX] = { 0 };
			TCHAR wcsPath[PATH_MAX] = { 0 };
			CHAR chDisk;
			DVR_DEVICE_TYPE devType = DVR_DEVICE_TYPE_SD;

			switch (pHdr->dbch_devicetype) {
			case DBT_DEVTYP_DEVICEINTERFACE:
				pDevInf = (PDEV_BROADCAST_DEVICEINTERFACE)pHdr;
				break;

			case DBT_DEVTYP_HANDLE:
				pDevHnd = (PDEV_BROADCAST_HANDLE)pHdr;
				break;

			case DBT_DEVTYP_OEM:
				pDevOem = (PDEV_BROADCAST_OEM)pHdr;
				break;

			case DBT_DEVTYP_PORT:
				pDevPort = (PDEV_BROADCAST_PORT)pHdr;
				break;

			case DBT_DEVTYP_VOLUME:
				pDevVolume = (PDEV_BROADCAST_VOLUME)pHdr;
				chDisk = FirstDriveFromMask(pDevVolume->dbcv_unitmask);

				dstpath[0] = chDisk;
				strcat(dstpath, ":\\");

				if (wp == DBT_DEVICEARRIVAL) {
					p->NotifyMount(devType, dstpath);
				}
				else if (wp == DBT_DEVICEREMOVECOMPLETE) {
					p->NotifyUnMount(devType, dstpath);
				}
				break;
			}
		}

		break;
	}

	return DefWindowProc(hwnd, uMsg, wp, lp);
}

#endif