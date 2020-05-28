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
#include <cstddef>

#include <DVR_SDK_INTFS.h>
#include <cstdio>
#include "DvrSdkTest_EnumDevices.h"

int DvrSdkTest_EnumDevices::Test()
{
//! [EnumDevices Example]
    DVR_RESULT res;
    DVR_DEVICE_ARRAY Devices;

    const char *sd     = "SD";
    const char *udisk  = "UDISK";
    const char *usbhub = "USBHUB";

    res = Dvr_EnumDevices(m_hDvr, DVR_DEVICE_TYPE_ALL, &Devices);

    if (DVR_SUCCEEDED(res)) {
        for (int i = 0; i < Devices.nDeviceNum; i ++) {
            const char *type = "UNKNOWN";
            if (Devices.pDriveArray[i]->eType == DVR_DEVICE_TYPE_SD)
                type = sd;
            else if (Devices.pDriveArray[i]->eType == DVR_DEVICE_TYPE_USBHD)
                type = udisk;						  					  
            else if (Devices.pDriveArray[i]->eType == DVR_DEVICE_TYPE_USBHUB)
                type = usbhub;						  
 

            const char *connect = "UNKOWN";
            switch (Devices.pDriveArray[i]->eConnectType) {
                case DVR_DEVICE_CONNECT_TYPE_UNKNOWN:
                    connect = "UNKNOWN";
                    break;
                case DVR_DEVICE_CONNECT_TYPE_BEFOREPOWER:
                    connect = "BEFORE";
                    break;
                case DVR_DEVICE_CONNECT_TYPE_AFTERPOWER:
                    connect = "AFTER";
                    break;
            }
            printf(("[%d] %s %s %s %s\n"), i, type, Devices.pDriveArray[i]->szDrivePath, Devices.pDriveArray[i]->szDevice, connect);

            Dvr_Free(m_hDvr, Devices.pDriveArray[i]);
        }
    }
//! [EnumDevices Example]

    return res;
}
