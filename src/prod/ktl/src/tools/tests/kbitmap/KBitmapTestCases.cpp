    /*++

Copyright (c) Microsoft Corporation

Module Name:

    KBitmapTestCases.cpp

Abstract:

    This file contains the test case table.

    To add a new unit test case:
    1. Declare your test case routine in KBitmapTests.h.
    2. Add an entry to the gs_KuTestCases table in KBitmapTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

    --*/
#pragma once
#include "KBitmapTests.h"

//
// A table containing information of all test cases.
//

const KU_TEST_ENTRY gs_KuTestCases[] =
{
    {   L"KBitmapTest",              KBitmapTest,         L"BVT", L"KBitmap unit test"}
};
