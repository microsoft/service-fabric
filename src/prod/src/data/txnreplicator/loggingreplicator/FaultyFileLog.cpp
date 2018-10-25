// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LoggingReplicator;
using namespace TxnReplicator;
using namespace Data::Utilities;

FaultyFileLog::FaultyFileLog(__in PartitionedReplicaId const & traceId)
#ifdef PLATFORM_UNIX
    : Data::Log::FileLogicalLog()
#else
    : FileLog(traceId)
#endif
{
#ifdef PLATFORM_UNIX
    UNREFERENCED_PARAMETER(traceId);
#endif

    throwExceptionInAppendAsync_ = false;
    throwExceptionInFlushWithMarkerAsync_ = false;
    appendAsyncDelayInMs_ = 0;
    flushWithMarkerAsyncDelayInMs_ = 0;
    testExceptionStatusCode_ = STATUS_INSUFFICIENT_RESOURCES;
}

FaultyFileLog::~FaultyFileLog()
{
}

NTSTATUS FaultyFileLog::Create(
    __in PartitionedReplicaId const & traceId,
    __in KAllocator& allocator,
    __out FaultyFileLog::SPtr& fileLogicalLog)
{
    NTSTATUS status;
    FaultyFileLog::SPtr context;

    context = _new(FAULTYFILELOGMANAGER_TAG, allocator) FaultyFileLog(traceId);
    if (!context)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, nullptr, 0, 0);
        return status;
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    fileLogicalLog = Ktl::Move(context);

    return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> FaultyFileLog::AppendAsync(
    __in KBuffer const& buffer,
    __in LONG offsetIntoBuffer,
    __in ULONG count,
    __in CancellationToken const& cancellationToken
)
{
    if (appendAsyncDelayInMs_ > 0)
    {
        NTSTATUS status = co_await KTimer::StartTimerAsync(GetThisAllocator(), FAULTYFILELOGMANAGER_TAG, appendAsyncDelayInMs_, nullptr);
        THROW_ON_FAILURE(status);
    }

    if (throwExceptionInAppendAsync_)
    {
        co_return testExceptionStatusCode_;
    }
    else
    {
        NTSTATUS status = co_await __super::AppendAsync(buffer, offsetIntoBuffer, count, cancellationToken);
        co_return status;
    }
}

Awaitable<NTSTATUS> FaultyFileLog::FlushWithMarkerAsync(__in CancellationToken const& cancellationToken)
{
    if (flushWithMarkerAsyncDelayInMs_ > 0)
    {
        NTSTATUS status = co_await KTimer::StartTimerAsync(GetThisAllocator(), FAULTYFILELOGMANAGER_TAG, flushWithMarkerAsyncDelayInMs_, nullptr);
        THROW_ON_FAILURE(status);
    }

    if (throwExceptionInFlushWithMarkerAsync_)
    {
        co_return testExceptionStatusCode_;
    }
    else
    {
        NTSTATUS status = co_await __super::FlushWithMarkerAsync(cancellationToken);
        co_return status;
    }
}
