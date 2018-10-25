    /*++

Copyright (c) Microsoft Corporation

Module Name:

    KThreadPoolTestCases.cpp

Abstract:

    This file contains the test case table.

    To add a new unit test case:
    1. Declare your test case routine in KThreadPoolTests.h.
    2. Add an entry to the gs_KuTestCases table in KThreadPoolTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

    --*/
#pragma once
#include "KThreadPoolTests.h"

//
// A table containing information of all test cases.
//

const KU_TEST_ENTRY gs_KuTestCases[] =
{
    {   L"KThreadPoolTest",              KThreadPoolTest,         L"BVT", L"KThreadPool unit test"}
};

#if defined(PLATFORM_UNIX)
const ULONG gs_KuTestCasesSize = ARRAYSIZE(gs_KuTestCases);
#endif

