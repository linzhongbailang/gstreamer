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

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "DvrSdkTestUtils.h"

char *DvrSdkTestUtils::m_szCurrentDrive = NULL;
bool  DvrSdkTestUtils::m_bShowPosition = false;
int   DvrSdkTestUtils::m_nLogLevel = 0;
void *DvrSdkTestUtils::m_pNotifyContext = NULL;

const char *DvrSdkTestUtils::CurrentDrive()
{
    return m_szCurrentDrive;
}

void DvrSdkTestUtils::SetCurrentDrive(const char *szDrive)
{
    if (m_szCurrentDrive) {
        free(m_szCurrentDrive);
    }

    m_szCurrentDrive = strdup(szDrive);
    printf("%s\n", m_szCurrentDrive);
}

bool DvrSdkTestUtils::ShowPosition()
{
    return m_bShowPosition;
}

void DvrSdkTestUtils::SetShowPosition(bool bShow)
{
    m_bShowPosition = bShow;
}

int DvrSdkTestUtils::LogLevel()
{
    return m_nLogLevel;
}

void DvrSdkTestUtils::SetLogLevel(int nLogLevel)
{
    m_nLogLevel = nLogLevel;
}

void *DvrSdkTestUtils::NotifyContext()
{
    return m_pNotifyContext;
}

void DvrSdkTestUtils::SetNotifyContext(void *pContext)
{
    m_pNotifyContext = pContext;
}
