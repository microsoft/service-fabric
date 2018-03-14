// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace StateManagerTests;
using namespace ktl;

TestOperationDataStream::SPtr TestOperationDataStream::Create(
    __in LONG64 prepareCheckpointLSN, 
    __in KAllocator & allocator)
{
    SPtr result = _new(TEST_TAG, allocator) TestOperationDataStream(prepareCheckpointLSN);

    ASSERT_IFNOT(
        result != nullptr,
        "TestOperationDataStream: Must not be nullptr, PrepareCheckpointLSN: {0}",
        prepareCheckpointLSN);
    ASSERT_IFNOT(
        NT_SUCCESS(result->Status()),
        "TestOperationDataStream: Create TestOperationDataStream failed, PrepareCheckpointLSN: {0}",
        prepareCheckpointLSN);

    return result;
}

ktl::Awaitable<NTSTATUS> TestOperationDataStream::GetNextAsync(
    __in CancellationToken const & cancellationToken,
    __out Data::Utilities::OperationData::CSPtr & result) noexcept
{
    KShared$ApiEntry();

    ASSERT_IFNOT(
        isDisposed_ == false,
        "TestOperationDataStream: GetNextAsync: TestOperationDataStream has been disposed. PrepareCheckpointLSN: {0}",
        prepareCheckpointLSN_);

    if (currentIndex_ == 0)
    {
        Data::Utilities::BinaryWriter writer(GetThisAllocator());
        writer.Write(prepareCheckpointLSN_);

        KArray<KBuffer::CSPtr> operationArray(GetThisAllocator());
        NTSTATUS status = operationArray.Append(KBuffer::CSPtr(writer.GetBuffer(0).RawPtr()));
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "TestOperationDataStream: GetNextAsync: Append failed. PrepareCheckpointLSN: {0}",
            prepareCheckpointLSN_);

        currentIndex_++;

        result = Data::Utilities::OperationData::Create(operationArray, GetThisAllocator());
        co_return STATUS_SUCCESS;
    }

    result = nullptr;
    co_return STATUS_SUCCESS;
}

void TestOperationDataStream::Dispose()
{
    isDisposed_ = true;
}

TestOperationDataStream::TestOperationDataStream(__in LONG64 prepareCheckpointLSN)
    : prepareCheckpointLSN_(prepareCheckpointLSN)
{
}

TestOperationDataStream::~TestOperationDataStream()
{
    ASSERT_IFNOT(
        isDisposed_ == true,
        "TestOperationDataStream: Dtor: should be disposed.");
}
