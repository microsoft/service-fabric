// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

using namespace ktl;
using namespace Data::LoggingReplicator;
using namespace TxnReplicator;
using namespace Data::Utilities;
using namespace LoggingReplicatorTests;

#define TESTSTATESTREAM_TAG 'tSST'

TestStateStream::TestStateStream(__in ULONG32 successfulOperationCount)
    : successfulOperationCount_(successfulOperationCount),
    currentOperationCount_(0)
{
}

TestStateStream::~TestStateStream()
{
}

TestStateStream::SPtr TestStateStream::Create(__in KAllocator & allocator, __in ULONG32 successfulOperationCount)
{
    TestStateStream * value = _new(TESTSTATESTREAM_TAG, allocator) TestStateStream(successfulOperationCount);
    THROW_ON_ALLOCATION_FAILURE(value);

    return TestStateStream::SPtr(value);
}

void TestStateStream::Dispose()
{
}

Awaitable<NTSTATUS> TestStateStream::GetNextAsync(
    __in CancellationToken const & cancellationToken,
    OperationData::CSPtr & result) noexcept
{
    currentOperationCount_++;
    if (currentOperationCount_ > successfulOperationCount_) {
        co_await suspend_never{};
        result = nullptr;
        co_return STATUS_SUCCESS;
    }
    else
    {
        BinaryWriter bw(GetThisAllocator());
        bw.Write(0);

        KArray<KBuffer::CSPtr> buffers(GetThisAllocator());
        THROW_ON_CONSTRUCTOR_FAILURE(buffers);

        KBuffer::SPtr buffer = bw.GetBuffer(0);
        NTSTATUS status = buffers.Append(buffer.RawPtr());
        THROW_ON_FAILURE(status);

        co_await suspend_never();
        result = OperationData::Create(buffers, GetThisAllocator());
        co_return STATUS_SUCCESS;
    }
}
