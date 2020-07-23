// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace NightWatchTXRService;

PerfResult::~PerfResult()
{
}

PerfResult::PerfResult(
    __in ULONG numberOfOperations,
    __in __int64 durationSeconds,
    __in double operationsPerSecond,
    __in double operationsPerMillisecond,
    __in double throughput,
    __in double averageLatency,
    __in ULONG numberOfWriters,
    __in int64 startTicks,
    __in int64 endTicks,
    __in std::wstring codePackageDirectory,
    __in TestStatus::Enum const & status,
    __in_opt Data::Utilities::SharedException const * exception)
    : numberOfOperations_(numberOfOperations)
    , durationSeconds_(durationSeconds)
    , operationsPerSecond_(operationsPerSecond)
    , operationsPerMillisecond_(operationsPerMillisecond)
    , throughput_(throughput)
    , averageLatency_(averageLatency)
    , numberOfWriters_(numberOfWriters)
    , startTicks_(startTicks)
    , endTicks_(endTicks)
    , codePackageDirectory_(codePackageDirectory)
    , status_(status)
    , exception_(exception)
{
}

PerfResult::SPtr PerfResult::Create(
    __in ULONG numberOfOperations,
    __in __int64 durationSeconds,
    __in double operationsPerSecond,
    __in double operationsPerMillisecond,
    __in double throughput,
    __in double averageLatency,
    __in ULONG numberOfWriters,
    __in int64 startTicks,
    __in int64 endTicks,
    __in std::wstring codePackageDirectory,
    __in TestStatus::Enum const & status,
    __in_opt Data::Utilities::SharedException const * exception,
    __in KAllocator & allocator)
{
    PerfResult * pointer = _new(NIGHTWATCHTESTRUNNER_TAG, allocator)PerfResult(
        numberOfOperations,
        durationSeconds,
        operationsPerSecond,
        operationsPerMillisecond,
        throughput,
        averageLatency,
        numberOfWriters,
        startTicks,
        endTicks,
        codePackageDirectory,
        status,
        exception);

    THROW_ON_ALLOCATION_FAILURE(pointer);

    return PerfResult::SPtr(pointer);
}

PerfResult::SPtr PerfResult::Create(
    __in_opt Data::Utilities::SharedException const * exception,
    __in KAllocator & allocator)
{
    PerfResult * pointer = _new(NIGHTWATCHTESTRUNNER_TAG, allocator)PerfResult(
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        L"",
        TestStatus::Enum::Invalid,
        exception);

    THROW_ON_ALLOCATION_FAILURE(pointer);

    return PerfResult::SPtr(pointer);
}
