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
#include "DvrSdkTestCmd_Select.h"
#include "DvrSdkTestUtils.h"

int DvrSdkTestCmd_Select::ExecCmd()
{
    DVR_RESULT res;
    DVR_DEVICE_ARRAY Devices;

	res = Dvr_EnumDevices(m_hDvr, DVR_DEVICE_TYPE_ALL, &Devices);

    if (DVR_SUCCEEDED(res)) {
        if (m_nIndex < Devices.nDeviceNum && m_nIndex >= 0) {
            DvrSdkTestUtils::SetCurrentDrive(Devices.pDriveArray[m_nIndex]->szDrivePath);
			Dvr_SetCurrentDrive(m_hDvr, Devices.pDriveArray[m_nIndex]->szDrivePath);
        }
        for (int i = 0; i < Devices.nDeviceNum; i++) {
			Dvr_Free(m_hDvr, Devices.pDriveArray[i]);
        }
    }
    return DVR_RES_SOK;
}
