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

#ifndef REGTOOL_H_
#define REGTOOL_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This is a wrapper for some registry functions declared in utilreg
 * to avoid declaration conflicts between windows.h and qt's own windows.h.
 */

#if 0
int setRegInt(char * name,int val);
int getRegInt(char * name,int val);
#endif
int setRegStr(char * name,char * val);
int getRegStr(char * name,char * val);

#ifdef __cplusplus
}
#endif

#endif  /* REGTOOL_H_ */
