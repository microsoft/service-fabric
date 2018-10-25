    /*++

Copyright (c) Microsoft Corporation

Module Name:

    KStringViewTestCases.cpp

Abstract:

    This file contains the test case table.

    To add a new unit test case:
    1. Declare your test case routine in KStringViewTests.h.
    2. Add an entry to the gs_KuTestCases table in KStringViewTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

    --*/
#pragma once
#include "KStringViewTests.h"

//
// A table containing information of all test cases.
//

const KU_TEST_ENTRY gs_KuTestCases[] =
{
    {   L"KStringViewTest",              KStringViewTest,         L"BVT", L"Template for first test"}
};

const ULONG gs_KuTestCasesSize = ARRAYSIZE(gs_KuTestCases);
