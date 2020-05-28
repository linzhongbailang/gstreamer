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

#ifndef _WINSOCK2_H_
#define _WINSOCK2_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <process.h>
#include <errno.h>
#include "windows.h"

#define MAKEWORD(a, b)              ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#define INVALID_SOCKET              (SOCKET)(~0)
#define SOCKET_ERROR                (-1)
#define _int64                      long long
#define WSAStartup(x,y)             (0)
#define WSACleanup()                (0)
#define WSACreateEvent()            (1)
#define WSAEventSelect(x,y,z)       (0)
#define WSAEnumNetworkEvents(x,y,z) (0)
#define WSAGetLastError()           (errno)
#define WSAEWOULDBLOCK              EWOULDBLOCK
#define closesocket(s)				close(s)

#ifdef INADDR_ANY
#undef INADDR_ANY
#define INADDR_ANY                  (unsigned long)(0)
#endif

typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct hostent  HOSTENT, *PHOSTENT, *LPHOSTENT;
typedef unsigned int    GROUP;
typedef long            INT_PTR, *PINT_PTR;
typedef int*            LPINT;
typedef int             WSADATA, WSAEVENT, WSANETWORKEVENTS;

#endif //_WINSOCK2_H_
