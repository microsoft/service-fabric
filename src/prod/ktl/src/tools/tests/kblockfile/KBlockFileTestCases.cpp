    /*++

Copyright (c) Microsoft Corporation

Module Name:

    KBlockFileTestCases.cpp

Abstract:

    This file contains the test case table.

    To add a new unit test case:
    1. Declare your test case routine in KBlockFileTests.h.
    2. Add an entry to the gs_KuTestCases table in KBlockFileTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

    --*/
#pragma once
#include "KBlockFileTests.h"

//
// A table containing information of all test cases.
//

const KU_TEST_ENTRY gs_KuTestCases[] =
{
    {   L"KBlockFileTest",              KBlockFileTest,         L"BVT", L"KBlockFileTests -- requires a GPT disk"}
};
