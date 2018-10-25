    /*++

Copyright (c) Microsoft Corporation

Module Name:

    XmlTestCases.cpp

Abstract:

    This file contains the test case table.

    To add a new unit test case:
    1. Declare your test case routine in XmlTests.h.
    2. Add an entry to the gs_KuTestCases table in XmlTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

    --*/
#pragma once
#include "XmlTests.h"

//
// A table containing information of all test cases.
//

const KU_TEST_ENTRY gs_KuTestCases[] =
{
    {   L"XmlBasicTest",              XmlBasicTest,         L"BVT", L"Basic XML Parser Tests"},
    {   L"DomBasicTest",              DomBasicTest,         L"BVT", L"Basic XML DOM Tests"}
};

const ULONG gs_KuTestCasesSize = ARRAYSIZE(gs_KuTestCases);
