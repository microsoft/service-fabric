// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace Data::LoggingReplicator;
using namespace TxnReplicator;
using namespace Data::Utilities;
using namespace LoggingReplicatorTests;

#define TESTCHECKPOINTMANAGER_TAG 'MHCT'

TestCheckpointManager::TestCheckpointManager()
    : KObject()
    , KShared()
    , KWeakRefType<TestCheckpointManager>()
    , ICheckpointManager()
    , nextBarrierLogRecord_(nullptr)
{
}

TestCheckpointManager::~TestCheckpointManager()
{
}

TestCheckpointManager::SPtr TestCheckpointManager::Create(__in KAllocator & allocator)
{
    TestCheckpointManager * pointer = _new(TESTCHECKPOINTMANAGER_TAG, allocator) TestCheckpointManager();
    CODING_ERROR_ASSERT(pointer != nullptr);

    return TestCheckpointManager::SPtr(pointer);
}

void TestCheckpointManager::SetLastStableLSN(
    __in FABRIC_SEQUENCE_NUMBER lastStableLSN)
{
    lastStableLSN_ = lastStableLSN;
}

void TestCheckpointManager::SetNextBarrierLogRecord(
    __in BarrierLogRecord & nextBarrierLogRecord)
{
    nextBarrierLogRecord_.Put(&nextBarrierLogRecord);
}

FABRIC_SEQUENCE_NUMBER TestCheckpointManager::get_LastStableLsn() const
{
    return lastStableLSN_;
}

NTSTATUS TestCheckpointManager::ReplicateBarrier(BarrierLogRecord::SPtr & result)
{
    result = nextBarrierLogRecord_.Get();
    return STATUS_SUCCESS;
}

Awaitable<void> TestCheckpointManager::ApplyCheckpointIfPermittedAsync(
    __in LogRecord & record,
    __in NTSTATUS flushError)
{
    UNREFERENCED_PARAMETER(record);
    UNREFERENCED_PARAMETER(flushError);

    co_await suspend_never{};
    co_return;
}

void TestCheckpointManager::ApplyLogHeadTruncationIfPermitted(
    __in LogRecord & record,
    __in NTSTATUS flushError)
{
    UNREFERENCED_PARAMETER(record);
    UNREFERENCED_PARAMETER(flushError);
}

CopyModeFlag::Enum TestCheckpointManager::GetLogRecordsToCopy(
    __in ProgressVector & targetProgressVector,
    __in TxnReplicator::Epoch const & targetLogHeadEpoch,
    __in FABRIC_SEQUENCE_NUMBER targetLogHeadLsn,
    __in FABRIC_SEQUENCE_NUMBER targetLogTailLsn,
    __in FABRIC_SEQUENCE_NUMBER lastRecoveredAtomicRedoOperationLsn,
    __in LONG64 targetReplicaId,
    __out FABRIC_SEQUENCE_NUMBER & sourceStartingLsn,
    __out FABRIC_SEQUENCE_NUMBER & targetStartingLsn,
    __out Data::Utilities::IAsyncEnumerator<LogRecord::SPtr>::SPtr & logRecordsToCopy,
    __out BeginCheckpointLogRecord::SPtr & completedBeginCheckpointRecord)
{
    UNREFERENCED_PARAMETER(targetProgressVector);
    UNREFERENCED_PARAMETER(targetLogHeadEpoch);
    UNREFERENCED_PARAMETER(targetLogHeadLsn);
    UNREFERENCED_PARAMETER(targetLogTailLsn);
    UNREFERENCED_PARAMETER(lastRecoveredAtomicRedoOperationLsn);
    UNREFERENCED_PARAMETER(targetReplicaId);
    UNREFERENCED_PARAMETER(sourceStartingLsn);
    UNREFERENCED_PARAMETER(targetStartingLsn);
    UNREFERENCED_PARAMETER(logRecordsToCopy);
    UNREFERENCED_PARAMETER(completedBeginCheckpointRecord);

    return CopyModeFlag::Full;
}

Awaitable<bool> TestCheckpointManager::AcquireBackupAndCopyConsistencyLockAsync(
    __in KStringView const locker,
    __in KDuration const & timeout,
    __in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(locker);
    UNREFERENCED_PARAMETER(timeout);
    UNREFERENCED_PARAMETER(cancellationToken);

    co_await suspend_never{};
    co_return true;
}

void TestCheckpointManager::ReleaseBackupAndCopyConsistencyLock(__in KStringView const & lockReleaser)
{
    UNREFERENCED_PARAMETER(lockReleaser);
}

Awaitable<bool> TestCheckpointManager::AcquireStateManagerApiLockAsync(
    __in KStringView const locker,
    __in KDuration const & timeout,
    __in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(locker);
    UNREFERENCED_PARAMETER(timeout);
    UNREFERENCED_PARAMETER(cancellationToken);

    co_await suspend_never{};
    co_return true;
}

void TestCheckpointManager::ReleaseStateManagerApiLock(__in KStringView const & lockReleaser)
{
    UNREFERENCED_PARAMETER(lockReleaser);
}
