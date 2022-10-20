// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        class CopyStream final
            : public TxnReplicator::OperationDataStream
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LR>
        {
            K_FORCE_SHARED(CopyStream)
            K_SHARED_INTERFACE_IMP(IDisposable)

        public:
            typedef KDelegate<ktl::Awaitable<NTSTATUS>(LONG64)> WaitForLogFlushUpToLsnCallback;

            static CopyStream::SPtr Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in ReplicatedLogManager & replicatedLogManager,
                __in TxnReplicator::IStateProviderManager& stateManager,
                __in ICheckpointManager & checkpointManager,
                __in WaitForLogFlushUpToLsnCallback const & waitForLogFlushUptoLsn,
                __in LONG64 replicaId,
                __in LONG64 uptoLsn,
                __in TxnReplicator::OperationDataStream & copyContext,
                __in LogRecordLib::CopyStageBuffers & copyStageBuffers,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in bool hasPersistedState,
                __in KAllocator & allocator);

            ktl::Awaitable<NTSTATUS> GetNextAsync(
                __in ktl::CancellationToken const & cancellationToken,
                __out Utilities::OperationData::CSPtr & result) noexcept override;

            void Dispose() override;

        private:
            CopyStream(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in ReplicatedLogManager & replicatedLogManager,
                __in TxnReplicator::IStateProviderManager & stateManager,
                __in ICheckpointManager & checkpointManager,
                __in WaitForLogFlushUpToLsnCallback const & waitForLogFlushUptoLsn,
                __in LONG64 replicaId,
                __in LONG64 uptoLsn,
                __in TxnReplicator::OperationDataStream & copyContext,
                __in LogRecordLib::CopyStageBuffers & copyStageBuffers,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in bool hasPersistedState,
                __in KAllocator & allocator);

            ktl::Awaitable<NTSTATUS> GetNextAsyncSafe(
                __in ktl::CancellationToken const & cancellationToken,
                Utilities::OperationData::CSPtr & result);

            //
            // returns the metadata for the copied state that contains:
            // - version of the copy metadata state
            // - progress ProgressVectorEntry of the copied checkpoint
            // - copied Checkpoints Epoch
            // - sequence number of the first operation in the log to be copied
            // - sequence number of the copied checkpoints sequence number
            // - uptoSequence number for the copy provided by the volatile replicator
            // - highest possible the state providers could have copied
            //
            Utilities::OperationData::CSPtr GetCopyMetadata(__in LogRecordLib::CopyStage::Enum copyStageToWrite);

            Utilities::OperationData::CSPtr GetCopyStateMetadata();

            // Returns the copy batch size
            // Returns 1 if the configured CopyBatchSizeInKB is greater than MaxReplicationMessageSize
            int64 GetCopyBatchSizeInBytes();

            ULONG32 const copyMetadataVersion_ = 1;
            ULONG32 const copyStateMetadataVersion_ = 1;
            // 2 * sizeof(int) + sizeof(long)
            ULONG32 const copyMetadataByteSize_ = 16;
            // sizeof(int) + 6 * sizeof(long)
            ULONG32 const copyStateMetadataByteSizeNotIncludingProgressVector_ = 52;

            ReplicatedLogManager::SPtr const replicatedLogManager_;
            ICheckpointManager::SPtr const checkpointManager_;
            TxnReplicator::IStateProviderManager::SPtr const stateManager_;
            LONG64 const replicaId_;

            LogRecordLib::CopyStageBuffers::SPtr const copyStageBuffers_;

            // Pointer to a config object shared throughout this replicator instance
            TxnReplicator::TRInternalSettingsSPtr const transactionalReplicatorConfig_;
            bool hasPersistedState_ = true;

            TxnReplicator::OperationDataStream::SPtr copyContext_;
            
            KDelegate<ktl::Awaitable<NTSTATUS>(LONG64)> waitForLogFlushUptoLsn_;
            LogRecordLib::BeginCheckpointLogRecord::SPtr beginCheckpointRecord_;
            ULONG32 copiedRecordNumber_;
            LogRecordLib::CopyStage::Enum copyStage_;
            TxnReplicator::OperationDataStream::SPtr copyStateStream_;
            
            LONG64 currentTargetLsn_;
            LONG64 latestRecoveredAtomicRedoOperationLsn_;
            Utilities::IAsyncEnumerator<LogRecordLib::LogRecord::SPtr>::SPtr logRecordsToCopy_;
            LONG64 sourceStartingLsn_;
            
            TxnReplicator::Epoch targetLogHeadEpoch_;
            LONG64 targetLogHeadLsn_;
            LogRecordLib::ProgressVector::SPtr targetProgressVector_;
            LONG64 targetReplicaId_;
            LONG64 targetStartingLsn_;
            
            LONG64 uptoLsn_;
            Utilities::BinaryWriter bw_;

#ifdef DBG
            Common::StopwatchTime previousGetNextCallReturnedTime_;
#endif

            bool isDisposed_;

            // Maximum replication message size from V1 replicator
            LONG64 const maxReplicationMessageSizeInBytes_;
        };
    }
}
