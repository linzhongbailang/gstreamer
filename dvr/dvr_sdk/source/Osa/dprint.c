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
#ifdef LINUX86
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <dprint.h>

#define BUFFER_SIZE 40960

PFN_DVR_SDK_CONSOLE g_consoleCallback = NULL;
void *g_consoleCallbackContext = NULL;
static char g_gstconsoleCallbck=0;

static char buffer[BUFFER_SIZE] = { 0 };

#ifdef WIN32
#include <Windows.h>

int
gettimeofday(struct timeval *tp, void *tzp)
{
	SYSTEMTIME wtm;
	GetLocalTime(&wtm);

	tp->tv_sec = wtm.wHour * 60 * 60 + wtm.wMinute * 60 + wtm.wSecond;
	tp->tv_usec = wtm.wMilliseconds * 1000;
	return (0);
}
#endif

void DPrint(int level, const char *format, ...)
{
	char*  p = NULL;

	va_list vl;

	va_start(vl, format);

	p = &buffer[0];

	vsnprintf(p, BUFFER_SIZE, format, vl);
	va_end(vl);

	buffer[BUFFER_SIZE - 1] = 0x00;

	if (g_consoleCallback)
		g_consoleCallback(g_consoleCallbackContext, level, buffer);
}

