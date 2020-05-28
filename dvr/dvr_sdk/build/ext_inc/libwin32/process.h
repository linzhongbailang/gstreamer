//=============================================================================
//	THIS SOURCE CODE IS PROPRIETARY INFORMATION BELONGING TO INTERVIDEO, INC.
// 	ANY USE INCLUDING BUT NOT LIMITED TO COPYING OF CODE, CONCEPTS, AND/OR
//	ALGORITHMS IS PROHIBITED EXCEPT WITH EXPRESS WRITTEN PERMISSION BY THE 
//	COMPANY.
//
// 	THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
// 	KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// 	IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
// 	PURPOSE.
//
// 	Copyright (c) 2004 - 2005  InterVideo Corporation.  All Rights Reserved.
//
//-----------------------------------------------------------------------------

#ifndef _PROCESS_H
#define _PROCESS_H 1

#ifdef __cplusplus
extern "C" {
#endif

unsigned long _beginthread(void (*start_address)(void *), 
						   unsigned unused_stack_size, void *arglist);

unsigned long _beginthreadex(void *security,
							 unsigned unused_stack_size,
							 unsigned (*start_address)(void *),
							 void *arglist, 
							 unsigned initflag,
							 unsigned *thrdaddr); 

void _endthread(void);

void _endthreadex(unsigned retval);

#if !defined(_WINDOWS_H)
#include <wincompat.h>
int __stdcall DllMain(void *, unsigned long, void *);
#endif

#ifdef __cplusplus
}
#endif

#endif //_PROCESS_H
