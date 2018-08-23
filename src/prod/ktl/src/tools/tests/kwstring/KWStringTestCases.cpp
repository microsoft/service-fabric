    /*++

Copyright (c) Microsoft Corporation

Module Name:

    KWStringTestCases.cpp

Abstract:

    This file contains the test case table.

    To add a new unit test case:
    1. Declare your test case routine in KWStringTests.h.
    2. Add an entry to the gs_KuTestCases table in KWStringTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

    --*/
#pragma once
#include "KWStringTests.h"

//
// A table containing information of all test cases.
//

const KU_TEST_ENTRY gs_KuTestCases[] =
{
    {   L"KWStringTest",              KWStringTest,         L"BVT", L"KWString unit test"}
};
