
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "DVR_PLAYER_INTFS.h"
#include "PlaybackEngine.h"
#include "PhotoPlaybackEngine.h"

typedef struct _tagPLAYBACK_SDK
{
	CPlaybackEngine*  m_pEngine;
	DVR_PLAYER_CALLBACK_ARRAY  m_ptfnCallBackArray;
	GMutex  m_MutexLock;
	gboolean  m_bUnderLock;
}PlAYBACK_SDK;

#define MAX_WAIT_FOR_LOCK_TIME 10*MICROSEC_TO_SEC  // 10 sec
static PlAYBACK_SDK* g_ptSdkAboutToFree = NULL;
class CLockSDK
{
public:
	CLockSDK(PlAYBACK_SDK* pPlaybackSDK) : m_ptPlaybackSDK(pPlaybackSDK), m_bSuccess(FALSE)
	{
		g_mutex_lock(&pPlaybackSDK->m_MutexLock);
		gint64 dwStartWaitTime = g_get_real_time();
		while (m_ptPlaybackSDK->m_bUnderLock &&
			g_get_real_time() - dwStartWaitTime < MAX_WAIT_FOR_LOCK_TIME &&
			m_ptPlaybackSDK != g_ptSdkAboutToFree)
		{
			g_usleep(50);
		}

		if (m_ptPlaybackSDK == g_ptSdkAboutToFree)
		{
			m_bSuccess = FALSE;
		}
		else if (!m_ptPlaybackSDK->m_bUnderLock)
		{
			m_ptPlaybackSDK->m_bUnderLock = TRUE;
			m_bSuccess = TRUE;
		}
		g_mutex_unlock(&m_ptPlaybackSDK->m_MutexLock);
	}
	~CLockSDK()
	{
		if (m_bSuccess)
			m_ptPlaybackSDK->m_bUnderLock = FALSE;
	}

public:
	gboolean  m_bSuccess;
	PlAYBACK_SDK*  m_ptPlaybackSDK;

private:
	CLockSDK() {}
};

/**************************************************************************************************
 **************************************************************************************************
 *			Player interface		  
 * Description:
 *	This interface defines the player control and other 
 *	configuration methods of video player for the 
 *	application layer.
 **************************************************************************************************
 *************************************************************************************************/

/**************************************************************************************************
 * Function declare:
 *	Dvr_Player_Initialize
 * Description: Initialize the handle of player object.
 * Parameters: 
 *	phPlayerObj 	[in/out]:
 *		The handle of player object. Input a pointer of handle, the returned handle value 
 *		will be the initialized player object handle.
 * Return value:
 *	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
 * Notes:
 *	The initial interface must be called once ahead of other player interfaces, and the returned 
 *	player object handle should pass to all other implemented player interface.
 *************************************************************************************************/
int Dvr_Player_Initialize(void** phPlayer)
{
	if (phPlayer == NULL)
	{
		return DVR_RES_EINVALIDARG;
	}

	PlAYBACK_SDK*  pPlaybackSDK = new PlAYBACK_SDK;
	*phPlayer = (void*)pPlaybackSDK;
	if (pPlaybackSDK == NULL)
	{
		return DVR_RES_EOUTOFMEMORY;
	}

	memset(pPlaybackSDK, 0, sizeof(PlAYBACK_SDK));

	g_mutex_init(&pPlaybackSDK->m_MutexLock);
	pPlaybackSDK->m_bUnderLock = FALSE;

	pPlaybackSDK->m_pEngine = new CPlaybackEngine();
	if (pPlaybackSDK->m_pEngine == NULL)
	{
		Dvr_Player_DeInitialize(*phPlayer);
		return DVR_RES_EOUTOFMEMORY;
	}

	memset(&pPlaybackSDK->m_ptfnCallBackArray, 0, sizeof(DVR_PLAYER_CALLBACK_ARRAY));

	return DVR_RES_SOK;
}

/**************************************************************************************************
 * Function declare:
 *	Dvr_Player_DeInitialize
 * Description:
 *	Destroy the handle of player object.
 * Parameters:
 *	pPlayerObj 	[in]:
 *		Player object handle created by Dvr_Player_Initialize interface
 * Return value:
 *	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
 * Notes:
 *	The destroy interface must be called while exiting player program.
 *************************************************************************************************/
int Dvr_Player_DeInitialize(void* phPlayer)
{
	if (phPlayer == NULL)
	{
		return DVR_RES_EINVALIDARG;
	} 
	PlAYBACK_SDK*  pPlaybackSDK = (PlAYBACK_SDK*)phPlayer;

	CLockSDK* ptLock = new CLockSDK(pPlaybackSDK);
	if (!ptLock->m_bSuccess)
	{
		delete ptLock;
		return DVR_RES_EFAIL;
	}
	g_ptSdkAboutToFree = pPlaybackSDK;

	if (pPlaybackSDK->m_pEngine)
	{
		pPlaybackSDK->m_pEngine->Close(TRUE);
		delete pPlaybackSDK->m_pEngine;
		pPlaybackSDK->m_pEngine = NULL;
	}

	g_mutex_clear(&pPlaybackSDK->m_MutexLock);

	delete ptLock;
	delete pPlaybackSDK;

	g_ptSdkAboutToFree = NULL;

	return DVR_RES_SOK;
}

/**************************************************************************************************
 * Function declare:
 *	Dvr_Player_Open
 * Description:
 *	Open the clip via a specified URL.
 * Parameters:
 * 	pPlayerObj 	[in]:
 *		Player object handle created by Dvr_Player_Initialize interface.
 * 	szURL 		[in]:
 * 		The URL is the specified file including whole path. In the future, it will support
 *		standard URL format.
 * Return value:
 *	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
 *************************************************************************************************/
int Dvr_Player_Open(void* hPlayer, void* pvFileName)
{	
	PlAYBACK_SDK*  pPlaybackSDK = (PlAYBACK_SDK*)hPlayer;
	if (pPlaybackSDK == NULL)
	{
		return DVR_RES_EINVALIDARG;
	}
	Dvr_Player_Close(hPlayer);  // do this to make sure previous file is closed
	CLockSDK tLock(pPlaybackSDK);
	if (!tLock.m_bSuccess)
	{
		return DVR_RES_EFAIL;
	}
	if (pPlaybackSDK->m_pEngine == NULL)
	{
		return DVR_RES_EPOINTER;
	}

	pPlaybackSDK->m_pEngine->SetFuncList((void*)&pPlaybackSDK->m_ptfnCallBackArray);
	
    int ret = pPlaybackSDK->m_pEngine->Open((gpointer)pvFileName,
            pvFileName == NULL ? 0 : (glong)(strlen((gchar *)pvFileName)*sizeof(char)));

	return ret;
}

int Dvr_Player_Frame_Update(void* hPlayer, DVR_IO_FRAME *pInputFrame)
{
	PlAYBACK_SDK*  pPlaybackSDK = (PlAYBACK_SDK*)hPlayer;
	if (pPlaybackSDK == NULL)
	{
		return DVR_RES_EINVALIDARG;
	}
	
	if (pPlaybackSDK->m_pEngine == NULL)
	{
		return DVR_RES_EPOINTER;
	}

	int ret = pPlaybackSDK->m_pEngine->FrameUpdate(pInputFrame);

	return ret;
}

/**************************************************************************************************
 * Function declare:
 *	Dvr_Player_Close
 * Description:
 *	Close the clip resource, the state will switch to initial status.
 * Parameters:
 * 	pPlayerObj 	[in]:
 *		Player object handle created by Dvr_Player_Initialize interface.
 * Return value:
 *	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
 * Notes:
 *	Call this function to close the media stream and release related resources.
 *************************************************************************************************/
int Dvr_Player_Close(void* hPlayer)
{
	PlAYBACK_SDK*  pPlaybackSDK = (PlAYBACK_SDK*)hPlayer;

	if (pPlaybackSDK == NULL)
	{
		return DVR_RES_EINVALIDARG;
	}

	CLockSDK tLock(pPlaybackSDK);
	if (!tLock.m_bSuccess)
	{
		return DVR_RES_EFAIL;
	}

	if (pPlaybackSDK->m_pEngine)
	{
		int ret = pPlaybackSDK->m_pEngine->Close(TRUE);
		if (ret != DVR_RES_SOK)
		{
			return DVR_RES_EFAIL;
		}
	}

	return DVR_RES_SOK;
}

/**************************************************************************************************
 * Function declare:
 *	Dvr_Player_RegisterCallBack
 * Description:
 *	Register player callback function Return value is the error code.
 * Parameters:
 *	pPlayerObj	[in]:
 *		Player object handle created by Dvr_Player_Initialize interface.
 *	pPlayerCallback	[in]:
 *		The function point of playback callback.
 *	lUserData	[in]:
 *		User data which will be passed with callback function
 * Return value:
 *	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
 *************************************************************************************************/
int Dvr_Player_RegisterCallBack(void* hPlayer, DVR_PLAYER_CALLBACK_ARRAY *ptfnCallBackArray)
{
	PlAYBACK_SDK*  pPlaybackSDK = (PlAYBACK_SDK*)hPlayer;

	if (pPlaybackSDK == NULL || ptfnCallBackArray == NULL)
	{
		return DVR_RES_EINVALIDARG;
	}
	CLockSDK tLock(pPlaybackSDK);
	if (!tLock.m_bSuccess)
	{
		return DVR_RES_EFAIL;
	}

	memcpy(&pPlaybackSDK->m_ptfnCallBackArray, ptfnCallBackArray, sizeof(DVR_PLAYER_CALLBACK_ARRAY));

	return DVR_RES_SOK;
}

/**************************************************************************************************
 * Function declare:
 *	Dvr_Player_Play
 * Description:
 *	Start to playback the opened clip or paused clip.
 * Parameters:
 *	pPlayerObj	[in]:
 *		Player object handle created by Dvr_Player_Initialize interface.
 * Return value:
 *	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
 *************************************************************************************************/
int Dvr_Player_Play(void* hPlayer)
{
	PlAYBACK_SDK*  pPlaybackSDK = (PlAYBACK_SDK*)hPlayer;

	if (pPlaybackSDK == NULL)
	{
		return DVR_RES_EINVALIDARG;
	}

	CLockSDK tLock(pPlaybackSDK);
	if (!tLock.m_bSuccess)
	{
		return DVR_RES_EFAIL;
	}
	if (pPlaybackSDK->m_pEngine == NULL)
	{
		return DVR_RES_EPOINTER;
	}

	int  ret = pPlaybackSDK->m_pEngine->Run();
	if (ret != DVR_RES_SOK)
	{
		return DVR_RES_EFAIL;
	}

	return DVR_RES_SOK;
}

/**************************************************************************************************
* Function declare:
*	Dvr_Player_FastForward
* Description:
*	Start to playback the opened clip or paused clip.
* Parameters:
*	pPlayerObj	[in]:
*		Player object handle created by Dvr_Player_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Player_FastForward(void* hPlayer)
{
	PlAYBACK_SDK*  pPlaybackSDK = (PlAYBACK_SDK*)hPlayer;

	if (pPlaybackSDK == NULL)
	{
		return DVR_RES_EINVALIDARG;
	}
	CLockSDK tLock(pPlaybackSDK);
	if (!tLock.m_bSuccess)
	{
		return DVR_RES_EFAIL;
	}
	if (pPlaybackSDK->m_pEngine == NULL)
	{
		return DVR_RES_EPOINTER;
	}

	int  ret = pPlaybackSDK->m_pEngine->FastPlay(TRUE);
	if (ret != DVR_RES_SOK)
	{
		return DVR_RES_EFAIL;
	}

	return DVR_RES_SOK;
}

int Dvr_Player_FastScan(void* hPlayer, int s32Direction)
{
	PlAYBACK_SDK*  pPlaybackSDK = (PlAYBACK_SDK*)hPlayer;

	if (pPlaybackSDK == NULL)
	{
		return DVR_RES_EINVALIDARG;
	}
	CLockSDK tLock(pPlaybackSDK);
	if (!tLock.m_bSuccess)
	{
		return DVR_RES_EFAIL;
	}
	if (pPlaybackSDK->m_pEngine == NULL)
	{
		return DVR_RES_EPOINTER;
	}

	int  ret = pPlaybackSDK->m_pEngine->FastScan(s32Direction);
	if (ret != DVR_RES_SOK)
	{
		return DVR_RES_EFAIL;
	}

	return DVR_RES_SOK;
}

/**************************************************************************************************
 * Function declare:
 *	Dvr_Player_Pause
 * Description:
 *	Try to pause the playing clip.
 * Parameters:
 *	pPlayerObj	[in]:
 *		Player object handle created by Dvr_Player_Initialize interface.
 * Return value:
 *	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
 *************************************************************************************************/
int Dvr_Player_Pause(void* hPlayer)
{
	PlAYBACK_SDK*  pPlaybackSDK = (PlAYBACK_SDK*)hPlayer;

	if (pPlaybackSDK == NULL)
	{
		return DVR_RES_EINVALIDARG;
	}
	CLockSDK tLock(pPlaybackSDK);
	if (!tLock.m_bSuccess)
	{
		return DVR_RES_EFAIL;
	}
	if (pPlaybackSDK->m_pEngine == NULL)
	{
		return DVR_RES_EPOINTER;
	}

	int  ret = pPlaybackSDK->m_pEngine->Pause();
	if (ret != DVR_RES_SOK)
	{
		return DVR_RES_EFAIL;
	}

	return DVR_RES_SOK;
}

/**************************************************************************************************
 * Function declare:
 *	Dvr_Player_Resume
 * Description:
 *	Try to Resume the playing clip.
 * Parameters:
 *	pPlayerObj	[in]:
 *		Player object handle created by Dvr_Player_Initialize interface.
 * Return value:
 *	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
 *************************************************************************************************/
int Dvr_Player_Resume(void* hPlayer)
{
	PlAYBACK_SDK*  pPlaybackSDK = (PlAYBACK_SDK*)hPlayer;

	if (pPlaybackSDK == NULL)
	{
		return DVR_RES_EINVALIDARG;
	}
	CLockSDK tLock(pPlaybackSDK);
	if (!tLock.m_bSuccess)
	{
		return DVR_RES_EFAIL;
	}
	if (pPlaybackSDK->m_pEngine == NULL)
	{
		return DVR_RES_EPOINTER;
	}

	int  ret = pPlaybackSDK->m_pEngine->Run();
	if (ret != DVR_RES_SOK)
	{
		return DVR_RES_EFAIL;
	}

	return DVR_RES_SOK;
}

/**************************************************************************************************
 * Function declare:
 *	Dvr_Player_Stop
 * Description:
 *	Try to stop the playing clip.
 * Parameters:
 *	pPlayerObj	[in]:
 *		Player object handle created by Dvr_Player_Initialize interface.
 * Return value:
 *	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
 *************************************************************************************************/
int Dvr_Player_Stop(void* hPlayer)
{
	PlAYBACK_SDK*  pPlaybackSDK = (PlAYBACK_SDK*)hPlayer;

	if (pPlaybackSDK == NULL)
	{
		return DVR_RES_EINVALIDARG;
	}
	CLockSDK tLock(pPlaybackSDK);
	if (!tLock.m_bSuccess)
	{
		return DVR_RES_EFAIL;
	}
	if (pPlaybackSDK->m_pEngine == NULL)
	{
		return DVR_RES_EPOINTER;
	}

	int  ret = pPlaybackSDK->m_pEngine->Stop(TRUE);
	if (ret != DVR_RES_SOK)
	{
		return DVR_RES_EFAIL;
	}

	return DVR_RES_SOK;
}

/**************************************************************************************************
* Function declare:
*	Dvr_Player_ShowNextFrame
* Description:
*	Enter step-frame mode that the video frame will forward one by one by calling this interface.
* Parameters:
*	pPlayer		[in]:
*		Player object handle created by Dvr_Player_Initialize interface.
* Return value:
*	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
*************************************************************************************************/
int Dvr_Player_ShowNextFrame(void* hPlayer)
{
	return DVR_RES_SOK;
}

/**************************************************************************************************
 * Function declare:
 *	Dvr_Player_SetPosition
 * Description:
 *	Seek the clip to the specified position.
 * Parameters:
 *	pPlayerObj	[in]:
 *		Player object handle created by Dvr_Player_Initialize interface.
 *	dwPosition	[in]:
 *		The position in ms which wants to seek to.
 * Return value:
 *	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
 *************************************************************************************************/
int Dvr_Player_SetPosition(void* hPlayer, unsigned int u32Position)
{
	PlAYBACK_SDK*  pPlaybackSDK = (PlAYBACK_SDK*)hPlayer;

	if (pPlaybackSDK == NULL)
	{
		return DVR_RES_EINVALIDARG;
	}
	CLockSDK tLock(pPlaybackSDK);
	if (!tLock.m_bSuccess)
	{
		return DVR_RES_EFAIL;
	}
	if (pPlaybackSDK->m_pEngine == NULL)
	{
		return DVR_RES_EPOINTER;
	}

	int  ret = pPlaybackSDK->m_pEngine->SetPosition((glong)u32Position, NULL, 1);
	if (ret != DVR_RES_SOK)
	{
		return DVR_RES_EFAIL;
	}

	return DVR_RES_SOK;
}


/**************************************************************************************************
 * Function declare:
 *	Dvr_Player_GetPosition
 * Description:
 *	Get the current position of clip.
 * Parameters:
 *	pPlayerObj	[in]:
 *		Player object handle created by Dvr_Player_Initialize interface.
 *	dwPosition	[out]:
 *		The current playback position.
 * Return value:
 *	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
 *************************************************************************************************/
int Dvr_Player_GetPosition(void* hPlayer, unsigned int* pu32Position)
{
	PlAYBACK_SDK*  pPlaybackSDK = (PlAYBACK_SDK*)hPlayer;

	if (pPlaybackSDK == NULL || pu32Position == NULL)
	{
		return DVR_RES_EINVALIDARG;
	}
	CLockSDK tLock(pPlaybackSDK);
	if (!tLock.m_bSuccess)
	{
		return DVR_RES_EFAIL;
	}
	if (pPlaybackSDK->m_pEngine == NULL)
	{
		return DVR_RES_EPOINTER;
	}

	int  ret = pPlaybackSDK->m_pEngine->GetPosition((glong*)pu32Position);
	if (ret != DVR_RES_SOK)
	{
		return DVR_RES_EFAIL;
	}

	return DVR_RES_SOK;
}

/**************************************************************************************************
 * Function declare:
 *	Dvr_Player_SetConfig
 * Description:
 *	Set the specified configuration to player.
 * Parameters:
 *	pPlayerObj	[in]:
 *		Player object handle created by Dvr_Player_Initialize interface.
 *	dwCfgType	[in]:
 *		The configuration type.
 *	pValue		[in]:
 *		The value, and it is correlative to specified configuration type.
 * Return value:
 *	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
 *************************************************************************************************/
int Dvr_Player_SetConfig(void* hPlayer, DVR_PLAYER_CFG_TYPE eCfgType, void* pvCfgData, unsigned int u32CfgDataSize)
{
	PlAYBACK_SDK*  pPlaybackSDK = (PlAYBACK_SDK*)hPlayer;

	if (pPlaybackSDK == NULL || pvCfgData == NULL)
	{
		return DVR_RES_EINVALIDARG;
	}

	CLockSDK tLock(pPlaybackSDK);
	if (!tLock.m_bSuccess)
	{
		return DVR_RES_EFAIL;
	}
	if (pPlaybackSDK->m_pEngine == NULL)
	{
		return DVR_RES_EPOINTER;
	}

	return pPlaybackSDK->m_pEngine->Set(eCfgType, 0, 0, pvCfgData, u32CfgDataSize);
}

/**************************************************************************************************
 * Function declare:
 *	Dvr_Player_GetConfig
 * Description:
 *	Get the specified configuration of player.
 * Parameters:
 *	pPlayerObj	[in]:
 *		Player object handle created by Dvr_Player_Initialize interface.
 *	dwCfgType	[in]:
 *		The configuration type.
 *	pValue		[out]:
 *		The value, and it is correlative to specified configuration type.
 * Return value:
 *	DVR_RES_SOK if success. !DVR_RES_SOK if any error occurs. Return value is the error code.
 *************************************************************************************************/
int Dvr_Player_GetConfig(void* hPlayer, DVR_PLAYER_CFG_TYPE eCfgType, void* pvCfgData, unsigned int u32CfgDataSize)
{
	PlAYBACK_SDK*  pPlaybackSDK = (PlAYBACK_SDK*)hPlayer;

	if (pPlaybackSDK == NULL || pvCfgData == NULL)
	{
		return DVR_RES_EINVALIDARG;
	}

	CLockSDK tLock(pPlaybackSDK);
	if (!tLock.m_bSuccess)
	{
		return DVR_RES_EFAIL;
	}
	if (pPlaybackSDK->m_pEngine == NULL)
	{
		return DVR_RES_EPOINTER;
	}

	return pPlaybackSDK->m_pEngine->Get(eCfgType, 0, 0, pvCfgData, u32CfgDataSize, 0);
}

int Dvr_Player_PrintScreen(void * hPlayer, DVR_PHOTO_PARAM *pParam)
{
	PlAYBACK_SDK*  pPlaybackSDK = (PlAYBACK_SDK*)hPlayer;

	if (pPlaybackSDK == NULL)
	{
		return DVR_RES_EINVALIDARG;
	}

	if (pPlaybackSDK->m_pEngine == NULL)
	{
		return DVR_RES_EPOINTER;
	}

	return pPlaybackSDK->m_pEngine->PrintScreen(pParam);
}

int Dvr_Photo_Player_Open(void *hPlayer, const char *fileName)
{
    PlAYBACK_SDK*  pPlaybackSDK = (PlAYBACK_SDK*)hPlayer;
    if (pPlaybackSDK == NULL)
    {
        return DVR_RES_EINVALIDARG;
    }
    Dvr_Photo_Player_Close(hPlayer);  // do this to make sure previous file is closed
    CLockSDK tLock(pPlaybackSDK);
    if (!tLock.m_bSuccess)
    {
        return DVR_RES_EFAIL;
    }
    if (pPlaybackSDK->m_pEngine == NULL)
    {
        return DVR_RES_EPOINTER;
    }

    pPlaybackSDK->m_pEngine->SetFuncList((void*)&pPlaybackSDK->m_ptfnCallBackArray);

    int ret = pPlaybackSDK->m_pEngine->PhotoOpen(fileName);

	return ret;
}

int Dvr_Photo_Player_Close(void *hPlayer)
{
    PlAYBACK_SDK*  pPlaybackSDK = (PlAYBACK_SDK*)hPlayer;

    if (pPlaybackSDK == NULL)
    {
        return DVR_RES_EINVALIDARG;
    }

    CLockSDK tLock(pPlaybackSDK);
    if (!tLock.m_bSuccess)
    {
        return DVR_RES_EFAIL;
    }

    if (pPlaybackSDK->m_pEngine)
    {
        int ret = pPlaybackSDK->m_pEngine->PhotoClose();
        if (ret != DVR_RES_SOK)
        {
            return DVR_RES_EFAIL;
        }
    }

    return DVR_RES_SOK;
}

int Dvr_Photo_Player_Play(void *hPlayer)
{
    PlAYBACK_SDK*  pPlaybackSDK = (PlAYBACK_SDK*)hPlayer;

    if (pPlaybackSDK == NULL)
    {
        return DVR_RES_EINVALIDARG;
    }

    CLockSDK tLock(pPlaybackSDK);
    if (!tLock.m_bSuccess)
    {
        return DVR_RES_EFAIL;
    }

    if (pPlaybackSDK->m_pEngine)
    {
        int ret = pPlaybackSDK->m_pEngine->PhotoPlay();
        if (ret != DVR_RES_SOK)
        {
            return DVR_RES_EFAIL;
        }
    }

    return DVR_RES_SOK;
}

int Dvr_Photo_Player_Stop(void *hPlayer)
{
    PlAYBACK_SDK*  pPlaybackSDK = (PlAYBACK_SDK*)hPlayer;

    if (pPlaybackSDK == NULL)
    {
        return DVR_RES_EINVALIDARG;
    }

    CLockSDK tLock(pPlaybackSDK);
    if (!tLock.m_bSuccess)
    {
        return DVR_RES_EFAIL;
    }

    if (pPlaybackSDK->m_pEngine)
    {
        int ret = pPlaybackSDK->m_pEngine->PhotoStop();
        if (ret != DVR_RES_SOK)
        {
            return DVR_RES_EFAIL;
        }
    }

    return DVR_RES_SOK;
}