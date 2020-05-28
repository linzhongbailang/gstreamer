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

#ifndef _DVRSDKTESTUTILS_H_
#define _DVRSDKTESTUTILS_H_

class DvrSdkTestUtils
{
public:
    static const char *CurrentDrive();
    static void SetCurrentDrive(const char *szDrive);
    static bool ShowPosition();
    static void SetShowPosition(bool bShow);
    static int  LogLevel();
    static void SetLogLevel(int nLogLevel);
    static void *NotifyContext();
    static void SetNotifyContext(void *context);

private:
    static char *m_szCurrentDrive;
    static bool m_bShowPosition;
    static int  m_nLogLevel;
    static void *m_pNotifyContext;
};

#endif
