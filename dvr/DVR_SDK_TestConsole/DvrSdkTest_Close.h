//-----------------------------------------------------------------------------
//   THIS SOURCE CODE IS PROPRIETARY INFORMATION BELONGING TO Cidana (Shanghai).
//   ANY USE INCLUDING BUT NOT LIMITED TO COPYING OF CODE, CONCEPTS, AND/OR
//   ALGORITHMS IS PROHIBITED EXCEPT WITH EXPRESS WRITTEN PERMISSION BY THE 
//   COMPANY.
//
//   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//   KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//   IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//   PURPOSE.
//
//   Copyright (c) 2007 - 2008  Cidana (Shanghai) Co., Ltd.  All Rights Reserved.
//
//-----------------------------------------------------------------------------

#ifndef _CIAVNTEST_CLOSE_H_
#define _CIAVNTEST_CLOSE_H_

#include <cstring>
#include "CiAvnTestBase.h"

class CiAvnTest_Close: public CiAvnTestBase
{
public:
    CiAvnTest_Close(CIAVN_HANDLE hAvn)
        : CiAvnTestBase(hAvn)
    {
    }

    ~CiAvnTest_Close()
    {
    }

    const char *Command()
    {
        return "close";
    }

    const char *Usage()
    {
        const char *usage = ""
            "close\n"
            "Close the SDK and uninitialize all the media features\n"
            "Examples\n"
            "close\n";

        return usage;
    }

    int ProcessCmdLine(int argc, char *argv[])
    {
        if (argc != 1) {
            return CIAVN_RES_EFAIL;
        }

        return CIAVN_RES_SOK;
    }

    int Test();
};

#endif
