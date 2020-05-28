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
#include "DvrSdkTest_Player_Get_Position.h"

int DvrSdkTest_Player_Get_Position::Test()
{
	//! [Player_Get_Position Example]
	DVR_RESULT res;

	// unsigned m_uPos. The position is specified in Miliseconds
	res = Dvr_PlayBack_Get(m_hDvr, DVR_PLAYER_PROP_POSITION, &m_uPos, sizeof(m_uPos), NULL);
	//! [Player_Get_Position Example]
	if (res == DVR_RES_SOK)
		printf("%u\n", m_uPos);
	else
		printf("Get Seek Failed\n");

	return res;
}
