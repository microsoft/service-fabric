/*++

Copyright (c) Microsoft Corporation

Module Name:

    RvdLoggerTests.h

Abstract:

    Contains unit test case routine declarations.

    To add a new unit test case:
    1. Declare your test case routine in RvdLoggerTests.h.
    2. Add an entry to the gs_KuTestCases table in RvdLoggerTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
        this file or any other file.

--*/

#pragma once

#if defined(PLATFORM_UNIX)
#include <boost/test/unit_test.hpp>
#include "boost-taef.h"
#endif

#ifdef KMUNIT_FRAMEWORK
#if KTL_USER_MODE
#include <KmUser.h>
#else
#include <KmUnit.h>
#endif
#endif

#include <InternalKtlLogger.h>

//** Main entry points for unit tests
NTSTATUS
RvdLoggerAliasTests(__in int argc, __in_ecount(argc) WCHAR* args[]);

NTSTATUS
BasicDiskLoggerTest(__in int argc, __in_ecount(argc) WCHAR* args[]);

NTSTATUS
DiskLoggerStructureVerifyTests(__in int argc, __in_ecount(argc) WCHAR* args[]);

NTSTATUS
LogStreamAsyncIoTests(__in int argc, __in_ecount(argc) WCHAR* args[]);

NTSTATUS
RvdLoggerRecoveryTests(__in int argc, __in_ecount(argc) WCHAR* args[]);

NTSTATUS
RvdLoggerReservationTests(__in int argc, __in_ecount(argc) WCHAR* args[]);

//** Misc externs
NTSTATUS
BasicLogStreamTest(
    WCHAR const TestDriveLetter,
    BOOLEAN DoFinalStreamDeleteTest = TRUE,
    LogState::SPtr *const ResultingState = nullptr);

static GUID const TestLogIdGuid = {0x14575ca2, 0x1f3f, 0x42a8, {0x96, 0x71, 0xC2, 0xFc, 0xA7, 0xC4, 0x75, 0xa6}};

struct TestMetadata
{
    ULONG       DataRandomSeed;
    ULONGLONG   DataChecksum;
};

const ULONG TestIoBufferRecordArraySize = (RvdDiskLogConstants::BlockSize * 3) / sizeof(ULONG);
static_assert(
    (((TestIoBufferRecordArraySize * sizeof(ULONG)) % RvdDiskLogConstants::BlockSize) == 0),
    "TestIoBufferRecordSize mismatch");

struct TestIoBufferRecord
{
    ULONG           RecordItems[TestIoBufferRecordArraySize];
};

BOOLEAN
ValidateRandomRecord(TestMetadata& Metadata, KIoBuffer& Record);

#ifdef _WIN64
VOID
StaticTestCallback(
    PVOID EventPtr,
    KAsyncContextBase* const Parent,
    KAsyncContextBase& CompletedContext);
#endif

extern "C" NTSTATUS LastStaticTestCallbackStatus;

