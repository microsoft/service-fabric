// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LoggingReplicator;
using namespace TxnReplicator;
using namespace Data::Utilities;

FaultyFileLogicalLog::FaultyFileLogicalLog()
{
    throwExceptionInAppendAsync_ = false;
    throwExceptionInFlushWithMarkerAsync_ = false;
    appendAsyncDelayInMs_ = 0;
    flushWithMarkerAsyncDelayInMs_ = 0;
    testExceptionStatusCode_ = STATUS_INSUFFICIENT_RESOURCES;
}

FaultyFileLogicalLog::~FaultyFileLogicalLog()
{
}

NTSTATUS FaultyFileLogicalLog::Create(
    __out FaultyFileLogicalLog::SPtr& fileLogicalLog,
    __in KAllocator& allocator)
{
    NTSTATUS status;
    FaultyFileLogicalLog::SPtr context;

    context = _new(FAULTYFILELOGMANAGER_TAG, allocator) FaultyFileLogicalLog();
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

ktl::Awaitable<NTSTATUS> FaultyFileLogicalLog::CloseAsync(__in CancellationToken const & cancellationToken)
{
    NTSTATUS status = co_await __super::CloseAsync(cancellationToken);
    co_return status;
}

ktl::Awaitable<NTSTATUS> FaultyFileLogicalLog::AppendAsync(
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

ktl::Awaitable<NTSTATUS> FaultyFileLogicalLog::FlushWithMarkerAsync(__in CancellationToken const& cancellationToken)
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
