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

#ifndef __RPCNDR_H_VERSION__
#define __RPCNDR_H_VERSION__ 1

typedef unsigned char boolean;

#define DECLSPEC_UUID(x)
#define MIDL_INTERFACE(x)   struct
#define BEGIN_INTERFACE
#define END_INTERFACE

#define __RPC_FAR
#define __RPC_USER
#define __RPC_STUB

struct IRpcStubBuffer;
struct IRpcChannelBuffer;
typedef struct RPC_MESSAGE *PRPC_MESSAGE;

#endif
