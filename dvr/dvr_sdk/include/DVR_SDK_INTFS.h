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
#ifndef _DVR_SDK_INTFS_H_
#define _DVR_SDK_INTFS_H_

#include "DVR_SDK_DEF.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @ingroup Common
 * Creates a handle to the Dvr Engine that can be used for distribution.
 * This function does not allocate a significant system resources. 
 * Then full allocation of resources begins with the Dvr_MediaMgr_Open() call.
 * Example:
 * @snippet DvrSdkTestExec.cpp Initialize Example
 *
 * @return 
 * DVR_RES_SOK on success. <br>
 * DVR_RES_EUNEXPECTED if the engine is already initialized. <br>
 * DVR_RES_EOUTOFMEMORY if out of memory.
 * @version 
 */
DVR_API DVR_RESULT Dvr_Sdk_Initialize(void);

/**
 * @ingroup Common
 * Destroys the Dvr engine data and deallocates the handle. 
 * If the dvr engine is open, Dvr_MediaMgr_Close() should be called before
 * due to potential thread interlocking issues. 
 * Nevertheless, if the DVR has been opened and is
 * not properly closed before calling Unitialize, Dvr_MediaMgr_Close() will be called before destruction.
 * Example:
 * @snippet DvrSdkTestExec.cpp Uninitialize Example
 *
 * @return 
 * DVR_RES_SOK on success. <br>
 * DVR_RES_EUNEXPECTED if ADvrvn handle is not initialized. <br>
 * DVR_RES_EFAIL if memory deallocation failed.
 * @version 
 */
DVR_API DVR_RESULT Dvr_Sdk_DeInitialize(void);

/**
 * @ingroup Common
 * Open dvr media manager, will initialize the media features, such as usb insertion monitoring, the notification module and file scanning module
 *
 * @snippet DvrSdkTest_Open.cpp Open Example
 *
 * @return 
 * DVR_RES_SOK on success <br>  
 * DVR_RES_EINVALIDARG if pszUrl is invalid. <br>
 * DVR_RES_EPOINTER if pszUrl is NULL. <br>
 * DVR_RES_EUNEXPECTED if Dvr handle is not initialized or already open. <br>
 * DVR_RES_EFAIL on general failure.
 * @version 
 */
DVR_API DVR_RESULT Dvr_Sdk_MediaMgr_Open(void);

/**
 * @ingroup Common
 * Close dvr media manager
 * @example
 * @snippet DvrSdkTest_Close.cpp Close Example
 * @return 
 * DVR_RES_SOK on success <br>
 * DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
 * DVR_RES_EFAIL on general failure.
 * @remark
 * 
 * @version 
 */
DVR_API DVR_RESULT Dvr_Sdk_MediaMgr_Close(void);

/**
* @ingroup Common
* Reset dvr media manager database. After card format, this function need to be called
*
* @snippet DvrSdkTest_Refresh.cpp Refresh Example
*
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EINVALIDARG if pszUrl is invalid. <br>
* DVR_RES_EPOINTER if pszUrl is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or already open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_MediaMgr_Reset(void);

/**
 * @ingroup Common
 * Register notification callback into dvr.
 * Example:
 * @snippet DvrSdkTest_Open.cpp RegisterNotify Example
 *
 * @param pCallback function pointer of notification callback.
 * @param pContext passed as an argument to the callback function.
 * @return 
 * DVR_RES_SOK on success. <br>
 * DVR_RES_EUNEXPECTED if Dvr handle is not initialized. <br>
 * DVR_RES_EFAIL general error.
 * @version 
 */
DVR_API DVR_RESULT Dvr_Sdk_RegisterNotify(_IN PFN_DVR_SDK_NOTIFY pCallback, _IN DVR_VOID *pContext);

/**
 * @ingroup Common
 * UnRegister notification callback from dvr.
 * Example:
 * @snippet DvrSdkTest_Close.cpp UnRegister Example
 *
 * @param pCallback function pointer of notification callback that was registered.
 * @param pContext owner of callback function                 
 * @return 
 * DVR_RES_SOK on success, <br>
 * DVR_RES_EUNEXPECTED if Dvr handle is not initialized. <br>
 * DVR_RES_EFAIL callback not registered or general error.
 * @version 
 */
DVR_API DVR_RESULT Dvr_Sdk_UnRegisterNotify(_IN PFN_DVR_SDK_NOTIFY pCallback, _IN DVR_VOID *pContext);

/**
* @ingroup Common
* Register status/warning/error message callback into dvr.
* Example:
* @snippet DvrSdkTestExec.cpp RegisterConsole Example
*
* @param pCallback function pointer of console callback.
* @param pContext passed as an argument to the callback function.
* @return
* DVR_RES_SOK on success. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized. <br>
* DVR_RES_EFAIL general error.
* @remark  Entry and exit into any API function are logged through RegisterConsole.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_RegisterConsole(_IN PFN_DVR_SDK_CONSOLE pCallback, _IN void *pContext);
DVR_API DVR_RESULT Dvr_Sdk_RegisterGSTConsole(_IN PFN_DVR_SDK_CONSOLE pCallback, _IN void *pContext);

/**
* @ingroup Common
* UnRegister status/warning/error message callback from dvr.
* Example:
* @snippet DvrSdkTestExec.cpp UnRegisterConsole Example
*
* @param pCallback function pointer of console callback that was registered.
* @param pContext owner of callback function
* @return
* DVR_RES_SOK on success, <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized. <br>
* DVR_RES_EFAIL callback not registered or general error.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_UnRegisterConsole(_IN PFN_DVR_SDK_CONSOLE pCallback, _IN void *pContext);

/**
* @ingroup Recorder
* Initialize recorder, allocate related resource.
* Example:
* @snippet DvrSdkTestExec.cpp UnRegisterConsole Example
*
* @return
* DVR_RES_SOK on success, <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized. <br>
* DVR_RES_EOUTOFMEMORY if out of memory. 
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Recorder_Init(void);

/**
* @ingroup Recorder
* DeInitialize recorder, free related resource.
* Example:
* @snippet DvrSdkTestExec.cpp UnRegisterConsole Example
*
* @return
* DVR_RES_SOK on success, <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized. <br>
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Recorder_DeInit(void);

/**
* @ingroup Recorder
* Reset recorder, reset related resource.
* Example:
* @snippet DvrSdkTestExec.cpp UnRegisterConsole Example
*
* @return
* DVR_RES_SOK on success, <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized. <br>
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Recorder_Reset(void);

/**
* @ingroup Recorder
* Start recording. 
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Recorder_Start(void);

/**
* @ingroup Recorder
* Stop recording.
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Recorder_Stop(void);

/**
* @ingroup Recorder
* Acquire input buffer
* @param hDvr engine handle after opened by Dvr_MediaMgr_Open().
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Recorder_AcquireInputBuf(_OUT DVR_VOID **ppvBuffer);

/**
* @ingroup Recorder
* clear recorder/photo msgqueue
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/

DVR_API DVR_RESULT Dvr_Sdk_Recorder_AsyncOpFlush(void);

/**
* @ingroup Recorder
* Add the camera data to the recorder pipeline
* @param hDvr engine handle after opened by Dvr_MediaMgr_Open().
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Recorder_AddFrame(_IN DVR_IO_FRAME *pInputFrame);
/**
* @ingroup Recorder
* Set system property of recorder. See \ref DVR_RECORDER_PROP for all the properties can be set.\n
*
* @param hDvr engine handle after opened by Dvr_MediaMgr_Open().
* @param eProp type of property to set
* @param pPropData property data, depends on type
* @param nPropSize size of property data
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EPROPIDUNSUPPORTED if eProp is not supported. <br>
* DVR_RES_EINVALIDARG if pPropData is not correct or nPropSize is not proper. <br>
* DVR_RES_EPOINTER if pPropData is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Recorder_Set(DVR_RECORDER_PROP eProp, _IN DVR_VOID *pPropData, DVR_S32 nPropSize);

/**
* @ingroup Recorder
* Get system property of recorder.
*
* @param hDvr engine handle after opened by Dvr_MediaMgr_Open().
* @param eProp type of property to get
* @param pPropData property data, depends on type
* @param nPropSize size of property data
* @param pnSizeReturned size of the returned property data.  May be NULL.
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EPROPIDUNSUPPORTED if eProp is not supported. <br>
* DVR_RES_EINVALIDARG if nPropSize is not proper. <br>
* DVR_RES_EPOINTER if pPropData is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @remark  On failure, none of the pointers to data will be set.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Recorder_Get(DVR_RECORDER_PROP eProp, _OUT DVR_VOID *pPropData, DVR_S32 nPropSize, _OUT DVR_S32 *pnSizeReturned);

/**
* @ingroup Recorder
* start loop recording
*
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EINVALIDARG if pszUrl is invalid. <br>
* DVR_RES_EPOINTER if pszUrl is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or already open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Recorder_LoopRec_Start(void);

/**
* @ingroup Recorder
* stop loop recording
*
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EINVALIDARG if pszUrl is invalid. <br>
* DVR_RES_EPOINTER if pszUrl is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or already open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Recorder_LoopRec_Stop(void);

/**
* @ingroup Recorder
* Trigger emergency event for emergency record
*
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EINVALIDARG if pszUrl is invalid. <br>
* DVR_RES_EPOINTER if pszUrl is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or already open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Recorder_EventRec_Start(void);

/**
* @ingroup Recorder
* Stop emergency event for emergency recorder
*
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EINVALIDARG if pszUrl is invalid. <br>
* DVR_RES_EPOINTER if pszUrl is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or already open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Recorder_EventRec_Stop(void);

/**
* @ingroup Recorder
* Trigger apa event for das record
*
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EINVALIDARG if pszUrl is invalid. <br>
* DVR_RES_EPOINTER if pszUrl is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or already open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Recorder_DasRec_Start();

/**
* @ingroup Recorder
* Stop apa event for das recorder
*
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EINVALIDARG if pszUrl is invalid. <br>
* DVR_RES_EPOINTER if pszUrl is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or already open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Recorder_DasRec_Stop();

/**
* @ingroup Recorder
* Trigger iacc recording
*
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EINVALIDARG if pszUrl is invalid. <br>
* DVR_RES_EPOINTER if pszUrl is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or already open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Recorder_IaccRec_Start(int type);

/**
* @ingroup Recorder
* Stop iacc recording
*
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EINVALIDARG if pszUrl is invalid. <br>
* DVR_RES_EPOINTER if pszUrl is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or already open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Recorder_IaccRec_Stop(int type);

/**
* @ingroup Recorder
* Register new frame callback for recorder
*
* @param ptfnNewVehData new vehicle data callback
* @param user data pass to callback
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Recorder_RegisterVehDataUpdateCallBack(PFN_NEW_VEHICLE_DATA ptfnNewVehData, void *pContext);

/**
* @ingroup Recorder
* Trigger photo event for taking a photo
*
* @param eType photo type
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EINVALIDARG if pszUrl is invalid. <br>
* DVR_RES_EPOINTER if pszUrl is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or already open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Recorder_TakePhoto(DVR_PHOTO_PARAM *pParam);
DVR_API DVR_RESULT Dvr_Sdk_Recorder_TakeOverlay(_IN void *canbuffer,_IN void *osdbuffer,_IN int cansize,_IN int osdsize);


/**
* @ingroup Player
* Initialize player, allocate related resource.
*
* @return
* DVR_RES_SOK on success, <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized. <br>
* DVR_RES_EOUTOFMEMORY if out of memory. 
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Player_Init(void);

/**
* @ingroup Player
* DeInitialize player, free related resource.
*
* @return
* DVR_RES_SOK on success, <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized. <br>
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Player_DeInit(void);

/**
* @ingroup Player
* Opens a file for engine playback, it should be absolute file path
*
* The pzsUrl specifies the resource to open. 
* @param hDvr engine handle after opened by Dvr_MediaMgr_Open().
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EINVALIDARG if pszUrl is invalid. <br>
* DVR_RES_EPOINTER if pszUrl is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or already open. <br>
* DVR_RES_EFAIL on general failure.
* @remark
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Player_Open(_IN const char *szFileName, _IN int type);

/**
* @ingroup Player
* Close current playing file
*
* The pzsUrl specifies the resource to open.
* @param hDvr engine handle after opened by Dvr_MediaMgr_Open().
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EINVALIDARG if pszUrl is invalid. <br>
* DVR_RES_EPOINTER if pszUrl is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or already open. <br>
* DVR_RES_EFAIL on general failure.
* @remark
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Player_Close(void);

/**
* @ingroup Player
* Start playing current file.
* @snippet DvrTest_Play.cpp Play Example
* @param hDvr engine handle after opened by Dvr_MediaMgr_Open().
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @remark PlayAtPosition may be implemented with an Open, Pause, Seek, Play.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Player_Play(void);

/**
* @ingroup Player
* Stop playing current file.
* @snippet DvrTest_Play.cpp Play Example
* @param hDvr engine handle after opened by Dvr_MediaMgr_Open().
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @remark PlayAtPosition may be implemented with an Open, Pause, Seek, Play.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Player_Stop(void);

/**
* @ingroup Player
* Pause playing current file.
* @snippet DvrTest_Play.cpp Play Example
* @param hDvr engine handle after opened by Dvr_MediaMgr_Open().
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @remark PlayAtPosition may be implemented with an Open, Pause, Seek, Play.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Player_Pause(void);

/**
* @ingroup Player
* Resume playing current file.
* @snippet DvrTest_Play.cpp Play Example
* @param hDvr engine handle after opened by Dvr_MediaMgr_Open().
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @remark PlayAtPosition may be implemented with an Open, Pause, Seek, Play.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Player_Resume(void);

/**
* @ingroup Player
* Play next file in playlist.
*
* @param hDvr engine handle after opened by Dvr_MediaMgr_Open().
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EINVALIDARG if nLevel is invalid. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Player_Next(void);

/**
* @ingroup Player
* Play previous file in playlist.
*
* @param hDvr engine handle after opened by Dvr_MediaMgr_Open().
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EINVALIDARG if nLevel is invalid. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Player_Prev(void);
DVR_API DVR_RESULT Dvr_Sdk_Player_StepF(void);
DVR_API DVR_RESULT Dvr_Sdk_Player_StepB(void);

DVR_API DVR_RESULT Dvr_Sdk_Player_Advance(int nDelta);

/**
* @ingroup Player
* Set system property of player. See \ref Dvr_PLAYER_PROP for all the properties can be set.\n
*
* @param hDvr engine handle after opened by Dvr_MediaMgr_Open().
* @param eProp type of property to set
* @param pPropData property data, depends on type
* @param nPropSize size of property data
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EPROPIDUNSUPPORTED if eProp is not supported. <br>
* DVR_RES_EINVALIDARG if pPropData is not correct or nPropSize is not proper. <br>
* DVR_RES_EPOINTER if pPropData is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Player_Set(DVR_PLAYER_PROP eProp, _IN DVR_VOID *pPropData, DVR_S32 nPropSize);

/**
* @ingroup Player
* Get system property of player.
*
* @param hDvr engine handle after opened by Dvr_MediaMgr_Open().
* @param eProp type of property to get
* @param pPropData property data, depends on type
* @param nPropSize size of property data
* @param pnSizeReturned size of the returned property data.  May be NULL.
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EPROPIDUNSUPPORTED if eProp is not supported. <br>
* DVR_RES_EINVALIDARG if nPropSize is not proper. <br>
* DVR_RES_EPOINTER if pPropData is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @remark  On failure, none of the pointers to data will be set.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Player_Get(DVR_PLAYER_PROP eProp, _OUT DVR_VOID *pPropData, DVR_S32 nPropSize, _OUT DVR_S32 *pnSizeReturned);

/**
* @ingroup Player
* Trigger photo event for print screen
*
* @param eType photo type
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EINVALIDARG if pszUrl is invalid. <br>
* DVR_RES_EPOINTER if pszUrl is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or already open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Player_PrintScreen(DVR_PHOTO_PARAM *pParam);

/**
* @ingroup Player
* Register new frame callback for player
*
* @param ptfnNewFrame new frame callback
* @param user data pass to callback
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Player_RegisterCallBack(PFN_PLAYER_NEWFRAME ptfnNewFrame, void *pContext);

/**
* @ingroup Recorder
* Register new vehicle data update callback for player
*
* @param ptfnNewVehData new vehicle data callback
* @param user data pass to callback
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Player_RegisterVehDataUpdateCallBack(PFN_NEW_VEHICLE_DATA ptfnNewVehData, void *pContext);

/**
* @ingroup Player
* Undate the new frame's image buffer
*
* @param pImageBuf the new frame's image buffer
* @param user data pass to callback
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EFAIL on general failure.
* @version
*/

DVR_API DVR_RESULT Dvr_Sdk_Player_Frame_Update(DVR_IO_FRAME *pInputFrame);

/**
* @ingroup Player
* Open photo
*
* @param szFileName photo file name
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Photo_Open(_IN const char *szFileName);

/**
* @ingroup Player
* Close
*
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Photo_Close(void);

/**
* @ingroup Player
* Open next photo in database
*
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Photo_Next(void);

DVR_API DVR_RESULT Dvr_Sdk_Photo_Advance(int nDelta);

/**
* @ingroup Player
* Open prev photo in database
*
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Photo_Prev(void);
/**
* @ingroup Common
* Enumerate devices with given type.
*
* These are full mount points of devices.
* The application will automount on Dvr_Initialize() or any device change notification. For example:
* @snippet DvrSdkTest_EnumDevices.cpp EnumDevices Example
* @param hDvr is the engine after it has been opened by Dvr_Initialize().
* @param eType type of device to enumerate
* @param pDevices retrieved devices
* @return
* DVR_RES_SOK on success, <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized. <br>
* DVR_RES_EINVALIDARG if eType is invalid. <br>
* DVR_RES_EPOINTER if pDevices is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized. <br>
* DVR_RES_EOUTOFMEMORY if there is not enough memory for the device array
* @remark Call Dvr_Free after finished using pDevice->pDrivePathArr
* @version
*
*/
DVR_API DVR_RESULT Dvr_Sdk_EnumDevices(DVR_DEVICE_TYPE eType, _OUT DVR_DEVICE_ARRAY *pDevices);

/**
* @ingroup Common
* Active the given drive 
*
* @param szDrive is a logical name for the mount point. 
* @return
* DVR_RES_SOK on success, <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized. <br>
* DVR_RES_EINVALIDARG if eType is invalid. <br>
* DVR_RES_EPOINTER if pDevices is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized. <br>
* DVR_RES_EOUTOFMEMORY if there is not enough memory for the device array
* @remark Call Dvr_Free after finished using pDevice->pDrivePathArr
* @version
*
*/
DVR_API DVR_RESULT Dvr_Sdk_SetActiveDrive(_IN DVR_DEVICE *szDrive);

/**
* @ingroup Common
* Get the actived drive 
*
* @param szDrive will be fill with actived drive
* @return
* DVR_RES_SOK on success, <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized. <br>
* DVR_RES_EINVALIDARG if eType is invalid. <br>
* DVR_RES_EPOINTER if pDevices is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized. <br>
* DVR_RES_EOUTOFMEMORY if there is not enough memory for the device array
* @remark Call Dvr_Free after finished using pDevice->pDrivePathArr
* @version
*
*/
DVR_API DVR_RESULT Dvr_Sdk_GetActiveDrive(_IN DVR_DEVICE *szDrive);

/**
* @ingroup Common
* Clear the actived drive, after db umount, please clear active drive
*
* @return
* DVR_RES_SOK on success, <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized. <br>
* DVR_RES_EINVALIDARG if eType is invalid. <br>
* DVR_RES_EPOINTER if pDevices is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized. <br>
* DVR_RES_EOUTOFMEMORY if there is not enough memory for the device array
* @remark Call Dvr_Free after finished using pDevice->pDrivePathArr
* @version
*
*/
DVR_API DVR_RESULT Dvr_Sdk_ClearActiveDrive(void);

/**
* @ingroup Common
* Free memory which allocated by SDK
* @snippet DvrSdkTest_Open.cpp Dvr_Free Example
* @param hDvr the Dvr handle returned by Dvr_Initialize().
* @param pAllocData is the memory block to be freed.
* @return
* DVR_RES_SOK on success, <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized. <br>
* DVR_RES_EINVALIDARG if eType is invalid. <br>
* DVR_RES_EPOINTER if the pAllocData is NULL
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_Free(_IN void *pAllocData);

/**
* @ingroup Database
* This creates a new named drive handle szDrive and scans the path szDrive to create a File Database.
* Media/metadata scanning is accomplished elsewhere.
* @snippet DvrSdkTest_Open.cpp Dvr_DB_Mount Example
* @param hDvr engine handle after opened by Dvr_Initialize().
* @param szDrive is a logical name for the mount point.  "a" and "b" can be used, or the szDirectory could be used - this is just a string name.
* @param szDirectory is the fully qualified mount point of the device.
* @param pFileScan is the file scan method.
* @param bBlock indicates whether the drive scan should block.
* @param nPriority is a value that either boosts (positive) the working thread priority or decreases (negative) the working thread priority. 0 keeps same.
* @param pbCreate is a pointer to a BOOL which is TRUE if the drive handle is created.
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EUNEXPECTED if engine not initialized <br>
* DVR_RES_EPOINTER if szDrive or pFileScan is NULL <br>
* DVR_RES_EFAIL on mounting failure.
*
* @remark
* If bBlock is FALSE, this is an asynchronous command and returns immediately.
* The event notification DVR_NOTIFICATION_TYPE_SCANSTATE will be used to update progress.
*
* During scan, access to commands that read or write the File Database (excepting Dvr_DB_CancelScan)
* will block the calling thread until
* the scan is finished. This is for safety reasons.
*
* This function can be called anytime after Dvr_Initialize() and even if playback is occuring,
* but performance may be impacted. Drives empty of media files will not have any directories shown.
*
* The drive handle szDrive can be removed with Dvr_DB_Unmount().
*
* [Unimplemented behavior notes:
* If caching is selected, a similar structure database may be loaded temporarily while scanning is active.
* The cached name will be returned with DVR_NOTIFICATION_TYPE_SCANSTATE.  If the cached is identical
* to the final scan, this will be notified.  Otherwise, the cached name should be removed and all
* Ids flushed when the scan is completed because the cache is not quite correct.
* If partial is selected, a partial database may be loaded temporarily while scanning is active, with the
* partial database name returned with DVR_NOTIFICATION_TYPE_SCANSTATE.  The partial database should be
* removed and all Ids flushed when scan is complete as it is no longer needed.]
*
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_DB_Mount(_IN const char *szDrive, _IN const char *szDevice, _IN int nDevId);

/**
* @ingroup Database
* Remove drive and all of its media information from memory.  The file database and media information
* may be stored on the local flash system and reused at a later time, depending on the caching state
* upon calling Dvr_DB_Mount().
*
* @snippet DvrSdkTest_Open.cpp Dvr_DB_Unmount Example
* @param hDvr engine handle after created by Dvr_Initialize().
* @param szDrive string to remove from the database
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EUNEXPECTED if engine not initialized <br>
* DVR_RES_EPOINTER if szDrive is NULL <br>
* DVR_RES_EFAIL on mounting failure
*
* @remark  Can cancel an ongoing scan through Dvr_DB_CancelScan();
*
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_DB_Unmount(_IN const char *szDrive);

DVR_API DVR_RESULT Dvr_Sdk_FileMapDB_ScanLibrary(_IN const char *szDrive);

DVR_API DVR_RESULT Dvr_Sdk_FileMapDB_SetFailedItem(_IN const char *szFileName, _IN unsigned eFolderType);

DVR_API DVR_RESULT Dvr_Sdk_FileMapDB_GetItemCountByType(_OUT DVR_U32 *puCount, _IN unsigned eFolderType);

DVR_API DVR_RESULT Dvr_Sdk_FileMapDB_GetNameByRelPos(_IN unsigned uRelPos, _IN unsigned eFolderType, _OUT DVR_FILEMAP_META_ITEM *pszItem, _IN int nSize);

DVR_API DVR_RESULT Dvr_Sdk_FileMapDB_GePosByName(_OUT unsigned *puRelPos, _IN const char *szFileName, _IN unsigned eFolderType);

DVR_API DVR_RESULT Dvr_Sdk_FileMapDB_GetNextFile(_IN const char *szFileName, _OUT DVR_FILEMAP_META_ITEM *pszItem,_IN unsigned eFolderType);

DVR_API DVR_RESULT Dvr_Sdk_FileMapDB_GetPrevFile(_IN const char *szFileName, _OUT DVR_FILEMAP_META_ITEM *pszItem, _IN unsigned eFolderType);

DVR_API DVR_RESULT Dvr_Sdk_FileMapDB_GetFileInfo(_IN const char *pszDrive, _IN char *pszFullName, _IN DVR_FOLDER_TYPE eFolderType, _OUT PDVR_DB_IDXFILE idxfile);

DVR_API DVR_RESULT Dvr_Sdk_FileMapDB_DelItem(_IN const char *szFileName, _IN unsigned uDirId);

DVR_API DVR_RESULT Dvr_Sdk_FileMapDB_AddItem(_IN DVR_FILEMAP_META_ITEM *pszItem, _IN unsigned eFolderType);

DVR_API DVR_RESULT Dvr_Sdk_FileMapDB_Clear(void);

/**
* @ingroup CommonService
* Host control manager Common service for attaching handler
*
* @param handler handler for attaching
* @param pContext the context that will be passed to handler
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EPROPIDUNSUPPORTED if eProp is not supported. <br>
* DVR_RES_EINVALIDARG if pPropData is not correct or nPropSize is not proper. <br>
* DVR_RES_EPOINTER if pPropData is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_ComSvcHcMgr_AttachHandler(_IN DVR_HCMGR_HANDLER *handler, _IN void *pContext);

/**
* @ingroup CommonService
* Host control manager Common service for detaching handler
*
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EPROPIDUNSUPPORTED if eProp is not supported. <br>
* DVR_RES_EINVALIDARG if pPropData is not correct or nPropSize is not proper. <br>
* DVR_RES_EPOINTER if pPropData is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_ComSvcHcMgr_DetachHandler(void);

/**
* @ingroup CommonService
* Host control manager Common service for reset handler
*
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EPROPIDUNSUPPORTED if eProp is not supported. <br>
* DVR_RES_EINVALIDARG if pPropData is not correct or nPropSize is not proper. <br>
* DVR_RES_EPOINTER if pPropData is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_ComSvcHcMgr_ResetHandler(void);

/**
* @ingroup CommonService
* Host control manager Common service for receiving message
*
* @param msg received message.
* @param waitOption wait option.
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EPROPIDUNSUPPORTED if eProp is not supported. <br>
* DVR_RES_EINVALIDARG if pPropData is not correct or nPropSize is not proper. <br>
* DVR_RES_EPOINTER if pPropData is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_ComSvcHcMgr_RcvMsg(_OUT APP_MESSAGE *msg, _IN DVR_U32 waitOption);

/**
* @ingroup CommonService
* Host control manager Common service for sending message
*
* @param msg sended message.
* @param param1 first parameter
* @param param2 second parameter
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EPROPIDUNSUPPORTED if eProp is not supported. <br>
* DVR_RES_EINVALIDARG if pPropData is not correct or nPropSize is not proper. <br>
* DVR_RES_EPOINTER if pPropData is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_ComSvcHcMgr_SndMsg(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2);

/**
* @ingroup CommonService
* Common service for timer register. register handler with trigger frequency id
*
* @param tid trigger frequency id
* @param handler handler for registering
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EPROPIDUNSUPPORTED if eProp is not supported. <br>
* DVR_RES_EINVALIDARG if pPropData is not correct or nPropSize is not proper. <br>
* DVR_RES_EPOINTER if pPropData is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_ComSvrTimer_Register(int tid, PFN_APPTIMER_HANDLER handler);

/**
* @ingroup CommonService
* Common service for timer unregister. unregister handler with trigger frequency id
*
* @param tid trigger frequency id
* @param handler handler unregistering 
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EPROPIDUNSUPPORTED if eProp is not supported. <br>
* DVR_RES_EINVALIDARG if pPropData is not correct or nPropSize is not proper. <br>
* DVR_RES_EPOINTER if pPropData is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_ComSvrTimer_UnRegister(int tid, PFN_APPTIMER_HANDLER handler);

DVR_API DVR_RESULT Dvr_Sdk_ComSvrTimer_Handler(int tid);

/**
* @ingroup CommonService
* Common service for timer start.
*
* @param tid trigger frequency id whitch will start
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EPROPIDUNSUPPORTED if eProp is not supported. <br>
* DVR_RES_EINVALIDARG if pPropData is not correct or nPropSize is not proper. <br>
* DVR_RES_EPOINTER if pPropData is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_ComSvrTimer_Start(int tid);

/**
* @ingroup CommonService
* Common service for timer unregister all.
*
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EPROPIDUNSUPPORTED if eProp is not supported. <br>
* DVR_RES_EINVALIDARG if pPropData is not correct or nPropSize is not proper. <br>
* DVR_RES_EPOINTER if pPropData is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_ComSvrTimer_UnRegisterAll(void);

/**
* @ingroup CommonService
* Common service for card format. the card must be unmount before formating.
*
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EPROPIDUNSUPPORTED if eProp is not supported. <br>
* DVR_RES_EINVALIDARG if pPropData is not correct or nPropSize is not proper. <br>
* DVR_RES_EPOINTER if pPropData is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_ComSvcAsyncOp_CardFormat();

/**
* @ingroup CommonService
* Common service for file async copy
*
* @param srcFn the src filename for copy from
* @param dstFn the dst filename for copy to
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EPROPIDUNSUPPORTED if eProp is not supported. <br>
* DVR_RES_EINVALIDARG if pPropData is not correct or nPropSize is not proper. <br>
* DVR_RES_EPOINTER if pPropData is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_ComSvcAsyncOp_FileCopy(char *srcFn, char *dstFn);

/**
* @ingroup CommonService
* Common service for file async move
*
* @param srcFn the src filename for move 
* @param dstFn the dst filename move to
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EPROPIDUNSUPPORTED if eProp is not supported. <br>
* DVR_RES_EINVALIDARG if pPropData is not correct or nPropSize is not proper. <br>
* DVR_RES_EPOINTER if pPropData is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_ComSvcAsyncOp_FileMove(char *srcFn, char *dstFn);

/**
* @ingroup CommonService
* Common service for file async delete
*
* @param filename the filename for delete
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EPROPIDUNSUPPORTED if eProp is not supported. <br>
* DVR_RES_EINVALIDARG if pPropData is not correct or nPropSize is not proper. <br>
* DVR_RES_EPOINTER if pPropData is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_ComSvcAsyncOp_FileDel(char *filename);


/**
* @ingroup CommonService
* Common service for file sync copy
*
* @param srcFn the src filename for copy from
* @param dstFn the dst filename for copy to
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EPROPIDUNSUPPORTED if eProp is not supported. <br>
* DVR_RES_EINVALIDARG if pPropData is not correct or nPropSize is not proper. <br>
* DVR_RES_EPOINTER if pPropData is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_ComSvcSyncOp_FileCopy(char *srcFn, char *dstFn);

/**
* @ingroup CommonService
* Common service for file sync move
*
* @param srcFn the src filename for move 
* @param dstFn the dst filename move to
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EPROPIDUNSUPPORTED if eProp is not supported. <br>
* DVR_RES_EINVALIDARG if pPropData is not correct or nPropSize is not proper. <br>
* DVR_RES_EPOINTER if pPropData is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_ComSvcSyncOp_FileMove(char *srcFn, char *dstFn);

/**
* @ingroup CommonService
* Common service for file sync delete
*
* @param filename the filename for delete
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EPROPIDUNSUPPORTED if eProp is not supported. <br>
* DVR_RES_EINVALIDARG if pPropData is not correct or nPropSize is not proper. <br>
* DVR_RES_EPOINTER if pPropData is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_ComSvcSyncOp_FileDel(char *filename);

/**
* @ingroup StorageAsyncOp
* Send message to registered handler for async handle.
*
* @param msg the id that registered handler can recognize
* @param param1 first parameter pass to handler
* @param param1 second parameter pass to handler
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EPROPIDUNSUPPORTED if eProp is not supported. <br>
* DVR_RES_EINVALIDARG if pPropData is not correct or nPropSize is not proper. <br>
* DVR_RES_EPOINTER if pPropData is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_StorageAsyncOp_SndMsg(DVR_U32 msg, DVR_U32 param1, DVR_U32 param2);

/**
* @ingroup StorageAsyncOp
* Register handler to sdk for async handle.
*
* @param handler the struct wrappers handle and msg.
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EPROPIDUNSUPPORTED if eProp is not supported. <br>
* DVR_RES_EINVALIDARG if pPropData is not correct or nPropSize is not proper. <br>
* DVR_RES_EPOINTER if pPropData is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_StorageAsyncOp_RegHandler(DVR_STORAGE_HANDLER *handler);

/**
* @ingroup StorageCard
* Set the property of storage card
*
* @param eProp type of property to set
* @param pPropData property data, depends on type
* @param nPropSize size of property data
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EPROPIDUNSUPPORTED if eProp is not supported. <br>
* DVR_RES_EINVALIDARG if pPropData is not correct or nPropSize is not proper. <br>
* DVR_RES_EPOINTER if pPropData is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_StorageCard_Set(DVR_STORAGE_PROP eProp, _IN DVR_VOID *pPropData, DVR_S32 nPropSize);

/**
* @ingroup StorageCard
* Get the property of monitor storage
*
* @param eProp property(such as threshold, enable detect, enable msg and quota)
* @param pPropData property data, depends on type
* @param nPropSize size of property data
* @param pnSizeReturned size return
* @return
* DVR_RES_OK -- success <br>
* DVR_RES_EUNEXPECTED if engine not initialized <br>
* DVR_RES_EINVALIDARG if hDvr is an invalid handle (not the one retrived through Dvr_Inintialize() or has already get uninitialized by Dvr_Uninitialize() <br>
* DVR_RES_EPOINTER    if either szDrive or puId is NULL <br>
* DVR_RES_ENOENT      if the drive which szDrive points to has not been scanned yet or if the file does not exist
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP eProp, _OUT DVR_VOID *pPropData, DVR_S32 nPropSize, _OUT DVR_S32 *pnSizeReturned);

/**
* @ingroup StorageCard
* Check the status of storage card
*
* @return
* DVR_RES_OK -- success <br>
* DVR_RES_EUNEXPECTED if engine not initialized <br>
* DVR_RES_EINVALIDARG if hDvr is an invalid handle (not the one retrived through Dvr_Inintialize() or has already get uninitialized by Dvr_Uninitialize() <br>
* DVR_RES_EPOINTER    if either szDrive or puId is NULL <br>
* DVR_RES_ENOENT      if the drive which szDrive points to has not been scanned yet or if the file does not exist
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_StorageCard_CheckStatus(void);

/**
* @ingroup MonitorStorage
* Set the property of monitor storage
*
* @param eProp type of property to set
* @param pPropData property data, depends on type
* @param nPropSize size of property data
* @return
* DVR_RES_SOK on success <br>
* DVR_RES_EPROPIDUNSUPPORTED if eProp is not supported. <br>
* DVR_RES_EINVALIDARG if pPropData is not correct or nPropSize is not proper. <br>
* DVR_RES_EPOINTER if pPropData is NULL. <br>
* DVR_RES_EUNEXPECTED if Dvr handle is not initialized or not open. <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_MonitorStorage_Set(DVR_MONITOR_STORAGE_PROP eProp, _IN DVR_VOID *pPropData, DVR_S32 nPropSize);

/**
* @ingroup MonitorStorage
* Get the property of monitor storage
*
* @param eProp property(such as threshold, enable detect, enable msg and quota)
* @param pPropData property data, depends on type
* @param nPropSize size of property data
* @param pnSizeReturned size return
* @return
* DVR_RES_OK -- success <br>
* DVR_RES_EUNEXPECTED if engine not initialized <br>
* DVR_RES_EINVALIDARG if hDvr is an invalid handle (not the one retrived through Dvr_Inintialize() or has already get uninitialized by Dvr_Uninitialize() <br>
* DVR_RES_EPOINTER    if either szDrive or puId is NULL <br>
* DVR_RES_ENOENT      if the drive which szDrive points to has not been scanned yet or if the file does not exist
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_MonitorStorage_Get(DVR_MONITOR_STORAGE_PROP eProp, _OUT DVR_VOID *pPropData, DVR_S32 nPropSize, _OUT DVR_S32 *pnSizeReturned);

/**
* @ingroup MetaData
* Create meta data with given file name
*
* @param ppMetaDataHandle meta data's handle
* @param pvFileName the file name
* @param pu32ItemNum the item num that file contains
* @return
* DVR_RES_OK -- success <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_MetaData_Create(void **ppMetaDataHandle, void* pvFileName, DVR_U32 *pu32ItemNum);

/**
* @ingroup MetaData
* Destroy meta data 
*
* @param ppMetaDataHandle meta data's handle
* @return
* DVR_RES_OK -- success <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_MetaData_Destroy(void *pvMetaDataHandle);

/**
* @ingroup MetaData
* Get the item num in the file
*
* @param ppMetaDataHandle meta data's handle
* @param pu32ItemNum item num
* @return
* DVR_RES_OK -- success <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_MetaData_GetItemNum(void *pvMetaDataHandle, _OUT DVR_U32 *pu32ItemNum);

/**
* @ingroup MetaData
* Get the item by index
*
* @param ppMetaDataHandle meta data's handle
* @param pItem obtained item
* @param u32Index item's index
* @return
* DVR_RES_OK -- success <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_MetaData_GetDataByIndex(void *pvMetaDataHandle, _OUT DVR_METADATA_ITEM* pItem, _IN DVR_U32 u32Index);

/**
* @ingroup MetaData
* Get the item by type
*
* @param ppMetaDataHandle meta data's handle
* @param pItem obtained item
* @param u32Index item's type
* @return
* DVR_RES_OK -- success <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_MetaData_GetDataByType(void *pvMetaDataHandle, _OUT DVR_METADATA_ITEM* pItem, _IN DVR_METADATA_TYPE eType);

/**
* @ingroup MetaData
* Get the media info 
*
* @param ppMetaDataHandle meta data's handle
* @param pInfo obtained media info
* @return
* DVR_RES_OK -- success <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_MetaData_GetMediaInfo(void *pvMetaDataHandle, _OUT DVR_MEDIA_INFO *pInfo);

/**
* @ingroup MetaData
* Get the track by index
*
* @param ppMetaDataHandle meta data's handle
* @param pTrack obtained track
* @param u32Index track's index
* @return
* DVR_RES_OK -- success <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_MetaData_GetTrackByIndex(void *pvMetaDataHandle, _OUT DVR_MEDIA_TRACK* pTrack, _IN DVR_U32 u32Index);

/**
* @ingroup MetaData
* Get the file's preview
*
* @param pPreviewOpt the option of thumbnail
* @param pBuf
* @param pCbSize
* @return
* DVR_RES_OK -- success <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_MetaData_GetFilePreview(DVR_PREVIEW_OPTION *pPreviewOpt, DVR_U8 *pBuf, DVR_U32 *pCbSize);

/**
* @ingroup MetaData
* Format display name
*
* @param srcName src name of the file
* @param dstName dst name of the file after format
* @param u32DurationMs the duration of the file
* @return
* DVR_RES_OK -- success <br>
* DVR_RES_EFAIL on general failure.
* @version
*/
DVR_API DVR_RESULT Dvr_Sdk_FormatDisplayName(char *srcName, char *dstName, DVR_U32 u32DurationMs);

DVR_API DVR_RESULT Dvr_Sdk_LoadThumbNail(char *filename, unsigned char *pPreviewBuf, int nSize);

DVR_API DVR_RESULT Dvr_Sdk_UpdateSystemTime(DVR_U32 year, DVR_U32 month, DVR_U32 day, DVR_U32 hour, DVR_U32 minute, DVR_U32 second);

DVR_API DVR_RESULT Dvr_Sdk_MarkSDSpeed(_IN char *szDrive, _IN char *pszpoint, _IN DVR_SDSPEED_TYPE type);
DVR_API DVR_RESULT Dvr_Sdk_GetSDSpeekType(_IN  char *szDrive, _IN char *pszpoint, _OUT DVR_U32 *type);

#ifdef __cplusplus
}
#endif

#endif
