

#ifndef _OSA_H_
#define _OSA_H_

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <dprint.h>
#include <DVR_SDK_DEF.h>

#ifdef  __cplusplus
#ifndef __BEGIN_C_PROTO__
#define __BEGIN_C_PROTO__ extern "C" {
#endif
#ifndef __END_C_PROTO__
#define __END_C_PROTO__ }
#endif
#else
#ifndef __BEGIN_C_PROTO__
#define __BEGIN_C_PROTO__
#endif
#ifndef __END_C_PROTO__
#define __END_C_PROTO__
#endif
#endif

#define DISABLE_COPY(Class) \
    Class(const Class &);\
    Class &operator=(const Class &);

void OSA_Sleep(int miliseconds);
int OSA_FormatDisplayName(char *srcName, char *dstName, unsigned int duration_in_ms);
int OSA_UpdateSystemTime(int year, int month, int day, int hour, int minute, int second);
void OSA_GetSystemTime(int *year, int *month, int *day, int *hour, int *minute, int *second);

FILE *vpopen(const char* cmdstring, const char *type);
int vpclose(FILE *fp);


#endif /* _OSA_H_ */



