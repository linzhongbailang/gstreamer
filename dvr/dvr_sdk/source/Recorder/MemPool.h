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
#ifndef _MEMORY_POOL_H_
#define _MEMORY_POOL_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	void* phyAddr;
	unsigned int size;
}MemBlock_T;

void* 	MemPool_Init(MemBlock_T *pBlockTbl, int nBlock);
int 	MemPool_DeInit(void *pMemPool);
void*	MemPool_Malloc(void *pMemPool, unsigned int size, unsigned int alignment);
int 	MemPool_Free(void *ptr);
void*	MemPool_Calloc(void *pMemPool, unsigned int size);
void 	MemPool_Memset(void *ptr, int value, int size);
int 	MemPool_GetInfo(void *pMemPool, void **start_address, unsigned int *pool_size, unsigned int *min_allocation, unsigned int *available);

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif

