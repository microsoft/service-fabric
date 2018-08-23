    /*++

Copyright (c) Microsoft Corporation

Module Name:

    KAsyncQueueTestCases.cpp

Abstract:

    This file contains the test case table.

    To add a new unit test case:
    1. Declare your test case routine in KAsyncQueueTests.h.
    2. Add an entry to the gs_KuTestCases table in KAsyncQueueTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

    --*/
#pragma once
#include "KAsyncQueueTests.h"

//
// A table containing information of all test cases.
//

const KU_TEST_ENTRY gs_KuTestCases[] =
{
    {   L"KAsyncHelperTests",              KAsyncHelperTests,         L"BVT", L"Basic KAsync helper derivation unit tests"}
};

const ULONG gs_KuTestCasesSize = ARRAYSIZE(gs_KuTestCases);
