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
#include <cstddef>

#include <CI_AVN_INTFS.h>
#include "CiAvnTest_Close.h"
#include "CiAvnTest_Open.h"
#include "CiAvnTestUtils.h"

int CiAvnTest_Close::Test()
{
//! [Close Example]
    CIAVN_RESULT res;
//! [UnregisterNotify Example]
    res = CIAVN_UnregisterNotify(m_hAvn, CiAvnTest_Open::Notify, CiAvnTestUtils::NotifyContext());
    res = CIAVN_Close(m_hAvn);
//! [UnregisterNotify Example]
    return res;
//! [Close Example]
}
