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
#ifndef _DVR_APP_INTFS_INPUT_H_
#define _DVR_APP_INTFS_INPUT_H_


#ifdef __cplusplus
extern "C"
{
#endif

//input interface


//call frome worksapce/APP
void AVMInterface_setDvrIsRecoding(bool value);    
void AVMInterface_setDvrUIMode(int layout,int viewindex);



#ifdef __cplusplus
}
#endif

#endif
