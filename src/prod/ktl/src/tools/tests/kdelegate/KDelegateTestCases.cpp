/*++

Copyright (c) Microsoft Corporation

Module Name:

    KDelegateTestCases.cpp

Abstract:

    This file contains the test case table.

    To add a new unit test case:
    1. Declare your test case routine in SampleTests.h.
    2. Add an entry to the gs_KuTestCases table in SampleTestCases.cpp.
    3. Implement your test case routine. The implementation can be in 
        this file or any other file.

--*/
#pragma once

#include "KDelegateBasicTests.h"
#include "KDelegateInheritanceTests.h"
#include "KDelegateScaleTests.h"

//
// A table containing information of all test cases.
//

const KU_TEST_ENTRY gs_KuTestCases[] = 
{
    {   L"ConstructorTests",              ConstructorTests,         L"BVT", L"no parameters for samplepass"},
    {   L"BindTests",              BindTests,         L"BVT", L"no parameters for samplepass"},
    {   L"RebindTests",              RebindTests,         L"BVT", L"no parameters for samplefail"},
    {   L"FunctionScaleTests",              FunctionScaleTests,         L"BVT", L"no parameters for samplepass"},
    {   L"FunctionObjectScaleTests",              FunctionObjectScaleTests,         L"BVT", L"no parameters for samplepass"},
    {   L"InheritanceTests",              InheritanceTests,         L"BVT", L"no parameters for samplepass"}
};
