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
#include "DvrSdkTest_Player_Open.h"
#include "DvrSdkTestUtils.h"

void OnNewFrame(void *pPlayer, void *pUserData)
{
	DVR_PLAYER_FRAME *pFrame = (DVR_PLAYER_FRAME *)pUserData;
	void* player = (void*)pPlayer;

	printf("image[0x%x, %d x %d], crop[x:%d, y:%d, w:%d, h:%d], can[0x%x, %d]\n",
		pFrame->pImageBuf, pFrame->nImageWidth, pFrame->nImageHeight,
		pFrame->crop.x, pFrame->crop.y, pFrame->crop.width, pFrame->crop.height,
		pFrame->pCanBuf, pFrame->nCanSize);

	Dvr_PlayBack_Frame_UnRef(player, pFrame->pImageBuf);

	return;
}

int DvrSdkTest_Player_Open::Test()
{
//! [Open Example]
    DVR_RESULT res;

	res = Dvr_PlayBack_RegisterCallBack(m_hDvr, OnNewFrame);

	res = Dvr_PlayBack_Open(m_hDvr, m_szFileName);

    return res;
//! [Open Example]
}
