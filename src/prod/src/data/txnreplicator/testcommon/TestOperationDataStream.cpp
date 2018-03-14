// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TxnReplicator::TestCommon;
using namespace ktl;

using namespace Data::Utilities;

Common::StringLiteral const TraceComponent = "TestOperationDataStream";

TestOperationDataStream::SPtr TestOperationDataStream::Create(
    __in Data::Utilities::PartitionedReplicaId const & traceId,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in FABRIC_SEQUENCE_NUMBER checkpointLSN,
    __in VersionedState const & data,
    __in KAllocator & allocator) noexcept
{
    SPtr result = _new(TEST_OPERATION_DATA_STREAM_TAG, allocator) TestOperationDataStream(traceId, stateProviderId, checkpointLSN, data);

    ASSERT_IFNOT(
        result != nullptr,
        "{0}:{1}:{2} TestOperationDataStream: result should not be nullptr.",
        traceId.TracePartitionId,
        traceId.ReplicaId,
        stateProviderId);
    ASSERT_IFNOT(
        NT_SUCCESS(result->Status()),
        "{0}:{1}:{2} TestOperationDataStream: Create TestOperationDataStream failed.",
        traceId.TracePartitionId,
        traceId.ReplicaId,
        stateProviderId);

    return result;
}

ktl::Awaitable<NTSTATUS> TestOperationDataStream::GetNextAsync(
    __in ktl::CancellationToken const & cancellationToken,
    __out Data::Utilities::OperationData::CSPtr & result) noexcept
{
    KShared$ApiEntry();

    NTSTATUS status;

    ASSERT_IFNOT(
        isDisposed_ == false,
        "{0}:{1}:{2} TestOperationDataStream: GetNextAsync: TestOperationDataStream has been disposed.",
        TracePartitionId,
        ReplicaId,
        stateProviderId_);

    if (currentIndex_ == 0)
    {
        Trace.WriteInfo(
            TraceComponent,
            "{0}:{1}:{2} GetNextAsync: CheckpointLSN: {3} Version: {4}",
            TracePartitionId,
            ReplicaId,
            stateProviderId_,
            checkpointLSN_,
            data_->Version);

        KArray<KBuffer::CSPtr> operationArray(GetThisAllocator());
        {
            BinaryWriter writer(GetThisAllocator());

            writer.Write(checkpointLSN_);
            status = operationArray.Append(KBuffer::CSPtr(writer.GetBuffer(0).RawPtr()));
            ASSERT_IFNOT(
                NT_SUCCESS(status),
                "{0}:{1}:{2} TestOperationDataStream: GetNextAsync: Append CheckpointLSN failed. CheckpointLSN: {3}",
                TracePartitionId,
                ReplicaId,
                stateProviderId_,
                checkpointLSN_);
        }

        {
            BinaryWriter writer(GetThisAllocator());

            writer.Write(data_->Version);
            status = operationArray.Append(KBuffer::CSPtr(writer.GetBuffer(0).RawPtr()));
            ASSERT_IFNOT(
                NT_SUCCESS(status),
                "{0}:{1}:{2} TestOperationDataStream: GetNextAsync: Append Version failed. Version: {3}",
                TracePartitionId,
                ReplicaId,
                stateProviderId_,
                data_->Version);
        }

        currentIndex_++;

        result = OperationData::Create(operationArray, GetThisAllocator());
        co_return STATUS_SUCCESS;
    }
    else if (currentIndex_ == 1)
    {
        Trace.WriteInfo(
            TraceComponent,
            "{0}:{1}:{2} GetNextAsync: Value: {3}",
            TracePartitionId,
            ReplicaId,
            stateProviderId_,
            ToStringLiteral(*(data_->Value)));

        KArray<KBuffer::CSPtr> operationArray(GetThisAllocator());
        {
            BinaryWriter writer(GetThisAllocator());

            writer.Write(*data_->Value, UTF16);
            status = operationArray.Append(KBuffer::CSPtr(writer.GetBuffer(0).RawPtr()));
            ASSERT_IFNOT(
                NT_SUCCESS(status),
                "{0}:{1}:{2} TestOperationDataStream: GetNextAsync: Append Value failed. Value: {3}",
                TracePartitionId,
                ReplicaId,
                stateProviderId_,
                ToStringLiteral(*(data_->Value)));
        }

        currentIndex_++;

        result = OperationData::Create(operationArray, GetThisAllocator());
        co_return STATUS_SUCCESS;
    }

    result = nullptr;
    co_return STATUS_SUCCESS;
}

void TestOperationDataStream::Dispose()
{
    isDisposed_ = true;
}

TestOperationDataStream::TestOperationDataStream(
    __in Data::Utilities::PartitionedReplicaId const & traceId,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in FABRIC_SEQUENCE_NUMBER checkpointLSN,              // prepare checkpoint LSN of the last completed checkpoint.
    __in VersionedState const & data) noexcept
    : OperationDataStream()
    , PartitionedReplicaTraceComponent(traceId)
    , stateProviderId_(stateProviderId)
    , checkpointLSN_(checkpointLSN)
    , data_(&data)
{
    ASSERT_IFNOT(
        checkpointLSN_ > FABRIC_INVALID_SEQUENCE_NUMBER, 
        "{0}:{1}:{2} TestOperationDataStream: CheckpointLSN {3} < 0", 
        TracePartitionId,
        ReplicaId,
        stateProviderId_,
        checkpointLSN_);

    ASSERT_IFNOT(
        data_->Value != nullptr,
        "{0}:{1}:{2} TestOperationDataStream: Value is null.",
        TracePartitionId,
        ReplicaId,
        stateProviderId_);

    ASSERT_IFNOT(
        data_->Value->IsNullTerminated(),
        "{0}:{1}:{2} TestOperationDataStream: Value is not null terminated. Value: {3}",
        TracePartitionId,
        ReplicaId,
        stateProviderId_,
        ToStringLiteral(*(data_->Value)));

    Trace.WriteInfo(
        TraceComponent,
        "{0}:{1}:{2} CheckpointLSN: {3} Version: {4} Value: {5}",
        TracePartitionId,
        ReplicaId,
        stateProviderId_,
        checkpointLSN_,
        data_->Version,
        ToStringLiteral(*(data_->Value)));
}

TestOperationDataStream::~TestOperationDataStream()
{
    ASSERT_IFNOT(
        isDisposed_ == true,
        "{0}:{1}:{2} TestOperationDataStream: Must already have been disposed.",
        TracePartitionId,
        ReplicaId,
        stateProviderId_);
}
