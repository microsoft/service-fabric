/*++

Copyright (c) Microsoft Corporation

Module Name:

    SampleTestCases.cpp

Abstract:

    This file contains the test case table.

    To add a new unit test case:
    1. Declare your test case routine in SampleTests.h.
    2. Add an entry to the gs_KuTestCases table in SampleTestCases.cpp.
    3. Implement your test case routine. The implementation can be in 
        this file or any other file.

--*/
#pragma once
#include "SampleTests.h"

//
// A table containing information of all test cases.
//

const KU_TEST_ENTRY gs_KuTestCases[] = 
{
    {   L"SamplePass",              SamplePass,         L"BVT,TEST", L"no parameters for samplepass"},
    {   L"SampleFail",              SampleFail,         L"FVT", L"no parameters for samplefail"},
	{   L"SampleSecondFail",        SampleSecondFail,   L"BVT", L"no parameters for samplesecondfail"}
};
