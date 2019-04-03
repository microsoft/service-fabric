/*++
Copyright (c) Microsoft Corporation

Module Name: KPriorityQueueTestCases.cpp

Abstract: This file contains the test case table.
--*/

#pragma once
#include "KPriorityQueueTests.h"

//
// A table containing information of all test cases.
//
const KU_TEST_ENTRY gs_KuTestCases[] =
{
    {   L"KPriorityQueueTest",            KPriorityQueueTest,         L"BVT", L"KPriorityQueue tests."},
};

const ULONG gs_KuTestCasesSize = ARRAYSIZE(gs_KuTestCases);
