// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        interface ICheckpointManager
        {
            K_SHARED_INTERFACE(ICheckpointManager);
            K_WEAKREF_INTERFACE(ICheckpointManager);

        public:
            __declspec(property(get = get_LastStableLsn)) FABRIC_SEQUENCE_NUMBER LastStableLsn;
            virtual FABRIC_SEQUENCE_NUMBER get_LastStableLsn() const = 0;

            virtual NTSTATUS ReplicateBarrier(__out LogRecordLib::BarrierLogRecord::SPtr & result) = 0;

            virtual ktl::Awaitable<void> ApplyCheckpointIfPermittedAsync(
                __in LogRecordLib::LogRecord & record,
                __in NTSTATUS flushError) = 0;

            virtual void ApplyLogHeadTruncationIfPermitted(
                __in LogRecordLib::LogRecord & record,
                __in NTSTATUS flushError) = 0;

            virtual LogRecordLib::CopyModeFlag::Enum GetLogRecordsToCopy(
                __in LogRecordLib::ProgressVector & targetProgressVector,
                __in TxnReplicator::Epoch const & targetLogHeadEpoch,
                __in LONG64 targetLogHeadLsn,
                __in LONG64 targetLogTailLsn,
                __in LONG64 lastRecoveredAtomicRedoOperationLsn,
                __in LONG64 targetReplicaId,
                __out LONG64 & sourceStartingLsn,
                __out LONG64 & targetStartingLsn,
                __out Utilities::IAsyncEnumerator<LogRecordLib::LogRecord::SPtr>::SPtr & logRecordsToCopy,
                __out LogRecordLib::BeginCheckpointLogRecord::SPtr & completedBeginCheckpointRecord) = 0;

            virtual ktl::Awaitable<bool> AcquireBackupAndCopyConsistencyLockAsync(
                __in KStringView const locker,
                __in KDuration const & timeout,
                __in ktl::CancellationToken const & cancellationToken) = 0;
            
            virtual void ReleaseBackupAndCopyConsistencyLock(__in KStringView const & lockReleaser) = 0;

            virtual ktl::Awaitable<bool> AcquireStateManagerApiLockAsync(
                __in KStringView const locker,
                __in KDuration const & timeout,
                __in ktl::CancellationToken const & cancellationToken) = 0;

            virtual void ReleaseStateManagerApiLock(__in KStringView const & lockReleaser) = 0;
        };
    }
}
