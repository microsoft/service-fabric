/*++

Copyright (c) Microsoft Corporation

Module Name:

    RvdLoggerTestCases.cpp

Abstract:

    This file contains the test case table.

    To add a new unit test case:
    1. Declare your test case routine in RvdLoggerTests.h.
    2. Add an entry to the gs_KuTestCases table in RvdLoggerTestCases.cpp.
    3. Implement your test case routine. The implementation can be in 
        this file or any other file.

--*/
#pragma once
#include "RvdLoggerTests.h"

//
// A table containing information of all test cases.
//

const KU_TEST_ENTRY gs_KuTestCases[] = 
{
    {   L"RvdLoggerAliasTests",             RvdLoggerAliasTests,  L"CIT,BVT", L"RvdLoggerAliasTests <DriveLetter>"},
    {   L"BasicDiskLoggerTest",             BasicDiskLoggerTest,  L"CIT,BVT", L"BasicDiskLoggerTest <DriveLetter>"},
    {   L"DiskLoggerStructureVerifyTests",  DiskLoggerStructureVerifyTests,  L"CIT,BVT", L"DiskLoggerStructureVerifyTests <DriveLetter>"},
    {   L"DiskLogStreamAsyncIoTests",       LogStreamAsyncIoTests,  L"CIT,BVT", L"LogStreamAsyncIoTests <DriveLetter>"},
    {   L"RvdLoggerRecoveryTests",          RvdLoggerRecoveryTests,  L"CIT,BVT", L"RvdLoggerRecoveryTests <DriveLetter> [OverallDurationInSecs [MaxRunDurationInSecs [MinRunDuration]]]"},
};
