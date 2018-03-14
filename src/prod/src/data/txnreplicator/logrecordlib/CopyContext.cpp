// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

CopyContext::CopyContext(
    __in PartitionedReplicaId const & traceId,
    __in LONG64 replicaId,
    __in ProgressVector & progressVector,
    __in IndexingLogRecord const & logHeadRecord,
    __in LONG64 logTailLsn,
    __in LONG64 latestRecoveredAtomicRedoOperationLsn,
    __in KAllocator & allocator)
    : OperationDataStream()
    , PartitionedReplicaTraceComponent(traceId)
    , isDone_(false)
{
    BinaryWriter bw(allocator);

    bw.Write(replicaId);
    progressVector.Write(bw);
    bw.Write(logHeadRecord.CurrentEpoch.DataLossVersion);
    bw.Write(logHeadRecord.CurrentEpoch.ConfigurationVersion);
    bw.Write(logHeadRecord.Lsn);
    bw.Write(logTailLsn);
    bw.Write(latestRecoveredAtomicRedoOperationLsn);

    KArray<KBuffer::CSPtr> buffers(allocator);
    THROW_ON_CONSTRUCTOR_FAILURE(buffers);

    KBuffer::SPtr buffer = bw.GetBuffer(0);
    NTSTATUS status = buffers.Append(buffer.RawPtr());
    THROW_ON_FAILURE(status);

    copyData_ = OperationData::Create(buffers, allocator);
}

CopyContext::~CopyContext()
{
}

CopyContext::SPtr CopyContext::Create(
    __in PartitionedReplicaId const & traceId,
    __in LONG64 replicaId,
    __in ProgressVector & progressVector,
    __in IndexingLogRecord const & logHeadRecord,
    __in LONG64 logTailLsn,
    __in LONG64 latestRecoveredAtomicRedoOperationLsn,
    __in KAllocator & allocator)
{
    CopyContext * pointer = _new(COPYCONTEXT_TAG, allocator) CopyContext(
        traceId, 
        replicaId, 
        progressVector, 
        logHeadRecord, 
        logTailLsn, 
        latestRecoveredAtomicRedoOperationLsn, 
        allocator);
    
    THROW_ON_ALLOCATION_FAILURE(pointer);
    return CopyContext::SPtr(pointer);
}

void CopyContext::Dispose()
{
}

Awaitable<NTSTATUS> CopyContext::GetNextAsync(
    __in CancellationToken const & cancellationToken,
    __out OperationData::CSPtr & result) noexcept
{
    result = (isDone_) ? nullptr : copyData_;
    isDone_ = true;

    co_await suspend_never{};
    co_return STATUS_SUCCESS;
}
