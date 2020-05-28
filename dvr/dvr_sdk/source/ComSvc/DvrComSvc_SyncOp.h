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
#ifndef _DVR_COMSVC_SYNC_OP_H_
#define _DVR_COMSVC_SYNC_OP_H_

#include "DVR_SDK_INTFS.h"
#include "osa.h"
#include "osa_mutex.h"
#include "osa_tsk.h"
#include "osa_msgq.h"
#include "osa_fs.h"
#include "DvrSystemControl.h"


class DvrComSvcSyncOp
{
public:
    DvrComSvcSyncOp(void *sysCtrl);
    ~DvrComSvcSyncOp(void);

    int DvrComSvcSyncOp_FileCopy(char *srcFn, char *dstFn);
    int DvrComSvcSyncOp_FileMove(char *srcFn, char *dstFn);
    int DvrComSvcSyncOp_FileDel(char *filename);

private:
    DISABLE_COPY(DvrComSvcSyncOp)
       
    DvrSystemControl *m_sysCtrl;
};

#endif /* _DVR_COMSVC_ASYNC_OP_H_ */
