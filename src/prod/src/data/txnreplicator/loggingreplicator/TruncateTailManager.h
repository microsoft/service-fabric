// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        //
        // Used by SecondaryDrainManager to perform log tail truncation if false progress is detected
        //
        class TruncateTailManager final
            : public KObject<TruncateTailManager>
            , public KShared<TruncateTailManager>
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LR>
        {
            K_FORCE_SHARED(TruncateTailManager)

        public:

            static TruncateTailManager::SPtr Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in ReplicatedLogManager & replicatedLogManager,
                __in TransactionMap & transactionsMap,
                __in TxnReplicator::IStateProviderManager & stateManager,
                __in IBackupManager & backupManager,
                __in LONG64 tailLsn,
                __in TxnReplicator::ApplyContext::Enum falseProgressApplyContext,
                __in LogRecordLib::RecoveryPhysicalLogReader & recoveryLogsReader,
                __in LogRecordLib::InvalidLogRecords & invalidLogRecords,
                __in KAllocator & allocator);

            ktl::Awaitable<NTSTATUS> TruncateTailAsync(__out LogRecordLib::LogRecord::SPtr & newTail) noexcept;

        private:

            TruncateTailManager(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in ReplicatedLogManager & replicatedLogManager,
                __in TransactionMap & transactionsMap,
                __in TxnReplicator::IStateProviderManager & stateManager,
                __in IBackupManager & backupManager,
                __in LONG64 tailLsn,
                __in TxnReplicator::ApplyContext::Enum falseProgressApplyContext,
                __in LogRecordLib::RecoveryPhysicalLogReader & recoveryLogsReader,
                __in LogRecordLib::InvalidLogRecords & invalidLogRecords);

            ReplicatedLogManager::SPtr const replicatedLogManager_;
            TransactionMap::SPtr const transactionsMap_;
            TxnReplicator::IStateProviderManager::SPtr const stateManager_;
            IBackupManager::SPtr const backupManager_;
            LONG64 const tailLsn_;
            TxnReplicator::ApplyContext::Enum const falseProgressApplyContext_;
            LogRecordLib::RecoveryPhysicalLogReader::SPtr const recoveryLogsReader_;
            LogRecordLib::InvalidLogRecords::SPtr const invalidRecords_;
        };
    }
}
