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
#ifdef _MSC_VER
#pragma warning(disable:4996)	// 4706: assignment within conditional expression
#endif

#include "DvrComSvc_SyncOp.h"

DvrComSvcSyncOp::DvrComSvcSyncOp(void *sysCtrl)
{
    m_sysCtrl = (DvrSystemControl *)sysCtrl;
}

DvrComSvcSyncOp::~DvrComSvcSyncOp()
{

}

int DvrComSvcSyncOp::DvrComSvcSyncOp_FileCopy(char *srcFn, char *dstFn)
{
    unsigned id = 0;
    char *szSrcFullName = srcFn;
	char *szDstFullName = dstFn;	
    int res = 0;

    if (m_sysCtrl == NULL)
    {
        return DVR_RES_EPOINTER;
    }

    return DVR::OSA_FileCopy(szSrcFullName, szDstFullName);
}

int DvrComSvcSyncOp::DvrComSvcSyncOp_FileMove(char *srcFn, char *dstFn)
{
    unsigned folderType;
    char *szSrcFullName = srcFn;
	char *szDstFullName = dstFn;
    int res = 0;

    if (m_sysCtrl == NULL)
    {
        return DVR_RES_EPOINTER;
    }

    res = m_sysCtrl->FileMapDB_GetTypeByName(m_sysCtrl->CurrentMountPoint(), szSrcFullName, folderType);
    if (DVR_FAILED(res))
    {
        DPrint(DPRINT_ERR, "DvrComSvcSyncOp::DvrComSvcSyncOp_FileMove(): Can not find %s\n", srcFn);
        return DVR_RES_EFAIL;
    }

    return DVR::OSA_FileMove(szSrcFullName, szDstFullName);
}

int DvrComSvcSyncOp::DvrComSvcSyncOp_FileDel(char *filename)
{
    unsigned folderType;
    char *szFullName = filename;
    int res = 0;

    if (m_sysCtrl == NULL)
    {
        return DVR_RES_EPOINTER;
    }

    res = m_sysCtrl->FileMapDB_GetTypeByName(m_sysCtrl->CurrentMountPoint(), szFullName,folderType);
    if (DVR_FAILED(res))
    {
        DPrint(DPRINT_ERR, "DvrComSvcSyncOp::DvrComSvcSyncOp_FileDel(): Can not find %s\n", szFullName);
        return DVR_RES_EFAIL;
    }

    return DVR::OSA_FileDel(szFullName);
}
