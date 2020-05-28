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

#ifndef _DVR_SDCARD_SPEEDTEST_H_
#define _DVR_SDCARD_SPEEDTEST_H_

#define DATASIZE 1024*1024

class DvrSDCardSpeedTest
{
public:
	DvrSDCardSpeedTest();
	~DvrSDCardSpeedTest();
	
	bool IsLowSpeedSDCard(void);
	bool IsLowSpeedCheck(const char *path);
	
private:
	char data[DATASIZE];

private:
	void WriteTest(char *fileName);
	void ReadTest(char *fileName);
	void syncAndDropCache();
	float GetWriteSpeed(const char *path);	
};

#endif
