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

#ifndef _DPRINT_H_
#define _DPRINT_H_

#include <DVR_SDK_DEF.h>

#ifdef __cplusplus 
extern "C" {
#endif

#define DPRINT_VERBOSE	5
#define DPRINT_LOG		4
#define DPRINT_INFO		3
#define DPRINT_WARN		2
#define DPRINT_ERR	    1 

#ifdef _WIN32
	//#define DPrint
	void DPrint(int level, const char *format, ...);
#else
	void DPrint(int level, const char *format, ...);
#endif

	extern PFN_DVR_SDK_CONSOLE  g_consoleCallback;
	extern void              *g_consoleCallbackContext;

#ifndef __linux__
#ifndef __func__
#define __func__	__FUNCTION__
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif
