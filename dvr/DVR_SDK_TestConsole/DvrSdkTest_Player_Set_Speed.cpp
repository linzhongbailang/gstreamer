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
#include "DvrSdkTest_Player_Set_Speed.h"

int DvrSdkTest_Player_Set_Speed::Test()
{
	//! [Player_Set_Speed Example]
	DVR_RESULT res;

	// int m_nSpeed. The base speed is 1000 (1X). Positive value means forward and negative value means rewind.
	res = Dvr_PlayBack_Set(m_hDvr, DVR_PLAYER_PROP_SPEED, &m_nSpeed, sizeof(m_nSpeed));
	//! [Player_Set_Speed Example]

	return res;
}