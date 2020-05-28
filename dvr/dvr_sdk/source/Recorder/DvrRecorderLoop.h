/*******************************************************************************
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
\*********************************************************************************/
#ifndef _DVR_RECORDER_LOOP_H_
#define _DVR_RECORDER_LOOP_H_

#include <windows.h>
#include "DVR_RECORDER_INTFS.h"
#include "DVR_SDK_INTFS.h"
#include "DvrSystemControl.h"

#define DEFAULT_BADFILE_THUMBNAIL "/opt/avm/svres/badFile.bmp"
class DvrRecorderLoop
{
public:
	DvrRecorderLoop(void *sysCtrl);
	~DvrRecorderLoop();

	DVR_RESULT	Start();
	DVR_RESULT	Stop();
	DVR_RESULT	Reset();
    DVR_RESULT	LoopRecStart();
    DVR_RESULT	LoopRecStop();
    DVR_RESULT	EventRecStart();
    DVR_RESULT	EventRecStop();
    DVR_RESULT	DasRecStart();
    DVR_RESULT	DasRecStop();
    DVR_RESULT	IaccRecStart(int type);
    DVR_RESULT	IaccRecStop(int type);
	DVR_RESULT	Set(DVR_RECORDER_PROP type, void *configData, unsigned int size);
	DVR_RESULT	Get(DVR_RECORDER_PROP type, void *configData, unsigned int size);
	DVR_RESULT	AddFrame(DVR_IO_FRAME *pInputFrame);
	DVR_RESULT	AsyncOpFlush(void);
	DVR_RESULT	AcauireInputBuf(void **ppvBuffer);
    DVR_RESULT	Photo(DVR_PHOTO_PARAM *pParam);
    DVR_RESULT  Overlay(void *canbuffer, void *osdbuffer,int cansize, int osdsize);
	float Position();

    static DVR_RESULT LoadThumbNail(char *filename, unsigned char *pPreviewBuf, int nSize);

private:
	DISABLE_COPY(DvrRecorderLoop)
 	static int   Notify(void *pContext, DVR_NOTIFICATION_TYPE enuType, void *pParam1, void *pParam2);
	static void *TimerCallback(void *arg);
	static int  RecorderMessageCallback(void *pvParam1, void *pvParam2, void *pvParam3, void *pvContext);
	static int	CheckPhotoStorage(void *pvContext, DVR_RECORDER_FILE_OP* file_op);

	static DVR_RESULT LoopEnc_Storage_SearchOldestFile(DVR_U32 param1, DVR_U32 param2, void *pContext);
	static DVR_RESULT LoopEnc_Storage_DeleteFile(DVR_U32 param1, DVR_U32 param2, void *pContext);
    static DVR_RESULT LoopEnc_Storage_Return(DVR_U32 param1, DVR_U32 param2, void *pContext);
	static DVR_RESULT DAS_Storage_DeleteFile(DVR_U32 param1, DVR_U32 param2, void *pContext);
	static DVR_RESULT MoveToEmergencuyFolder(DVR_RECORDER_FILE_OP *file_op);

	unsigned ClipPeriod();

	DVR_RECORDER_CALLBACK_ARRAY  m_ptfnCallBackArray;

	HANDLE		m_pEngine;
	DvrSystemControl *		   m_sysCtrl;

	DvrMutex    m_curFileLock;
	char *      m_curRecFileName;
	DvrTimer * m_timer;
	DVR_BOOL	m_bExit;
	DVR_BOOL	m_bMove;

	/*Loop Encode Storage Handle*/
	//DVR_DB_FILE_INFO m_fileToDelete;
	DVR_U32		m_StorageHandleFlag;
    DVR_FILEMAP_META_ITEM m_fileToDelete;
    DVR_FILEMAP_META_ITEM m_DasfileToDelete[DVR_DAS_TYPE_NUM - DVR_DAS_TYPE_APA];
};

#endif
