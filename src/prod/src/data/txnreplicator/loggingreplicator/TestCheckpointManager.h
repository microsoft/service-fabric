// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{
    class TestCheckpointManager final
        : public Data::LoggingReplicator::ICheckpointManager
        , public KObject<TestCheckpointManager>
        , public KShared<TestCheckpointManager>
        , public KWeakRefType<TestCheckpointManager>
    {
        K_FORCE_SHARED(TestCheckpointManager);
        K_SHARED_INTERFACE_IMP(ICheckpointManager);
        K_WEAKREF_INTERFACE_IMP(ICheckpointManager, TestCheckpointManager);
 
    public:
        static TestCheckpointManager::SPtr Create(__in KAllocator & allocator);

    public: // Test APIs
        void SetLastStableLSN(
            __in FABRIC_SEQUENCE_NUMBER lastStableLSN);

        void SetNextBarrierLogRecord(
            __in Data::LogRecordLib::BarrierLogRecord & nextBarrierLogRecord);

    public: // ICheckpointManager
        __declspec(property(get = get_LastStableLsn)) LONG64 LastStableLsn;
        FABRIC_SEQUENCE_NUMBER get_LastStableLsn() const;

        NTSTATUS ReplicateBarrier(Data::LogRecordLib::BarrierLogRecord::SPtr & result);

        ktl::Awaitable<void> ApplyCheckpointIfPermittedAsync(
            __in Data::LogRecordLib::LogRecord & record,
            __in NTSTATUS flushError) override;

        void ApplyLogHeadTruncationIfPermitted(
            __in Data::LogRecordLib::LogRecord & record,
            __in NTSTATUS flushError) override;

        virtual Data::LogRecordLib::CopyModeFlag::Enum GetLogRecordsToCopy(
            __in Data::LogRecordLib::ProgressVector & targetProgressVector,
            __in TxnReplicator::Epoch const & targetLogHeadEpoch,
            __in FABRIC_SEQUENCE_NUMBER targetLogHeadLsn,
            __in FABRIC_SEQUENCE_NUMBER targetLogTailLsn,
            __in FABRIC_SEQUENCE_NUMBER lastRecoveredAtomicRedoOperationLsn,
            __in LONG64 targetReplicaId,
            __out FABRIC_SEQUENCE_NUMBER & sourceStartingLsn,
            __out FABRIC_SEQUENCE_NUMBER & targetStartingLsn,
            __out Data::Utilities::IAsyncEnumerator<Data::LogRecordLib::LogRecord::SPtr>::SPtr & logRecordsToCopy,
            __out Data::LogRecordLib::BeginCheckpointLogRecord::SPtr & completedBeginCheckpointRecord) override;

        virtual ktl::Awaitable<bool> AcquireBackupAndCopyConsistencyLockAsync(
            __in KStringView const locker,
            __in KDuration const & timeout,
            __in ktl::CancellationToken const & cancellationToken) override;
        virtual void ReleaseBackupAndCopyConsistencyLock(__in KStringView const & lockReleaser) override;

        virtual ktl::Awaitable<bool> AcquireStateManagerApiLockAsync(
            __in KStringView const locker,
            __in KDuration const & timeout,
            __in ktl::CancellationToken const & cancellationToken) override;
        virtual void ReleaseStateManagerApiLock(__in KStringView const & lockReleaser) override;

    private:
        FABRIC_SEQUENCE_NUMBER lastStableLSN_ = FABRIC_INVALID_SEQUENCE_NUMBER;
        Data::Utilities::ThreadSafeSPtrCache<Data::LogRecordLib::BarrierLogRecord> nextBarrierLogRecord_;
    };
}

