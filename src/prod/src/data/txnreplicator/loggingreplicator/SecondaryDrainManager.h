// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        class SecondaryDrainManager final
            : public KObject<SecondaryDrainManager>
            , public KShared<SecondaryDrainManager>
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LR>
        {
            K_FORCE_SHARED(SecondaryDrainManager)

        public:

            static SecondaryDrainManager::SPtr Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in RecoveredOrCopiedCheckpointState & recoveredOrCopiedCheckpointState,
                __in RoleContextDrainState & roleContextDrainState,
                __in OperationProcessor & recordsProcessor,
                __in IStateReplicator & fabricReplicator,
                __in IBackupManager & backupManager,
                __in CheckpointManager & checkpointManager,
                __in TransactionManager & transactionManager,
                __in ReplicatedLogManager & replicatedLogManager,
                __in TxnReplicator::IStateProviderManager & stateManager,
                __in RecoveryManager & recoveryManager,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in LogRecordLib::InvalidLogRecords & invalidLogRecords,
                __in KAllocator & allocator);

            void Reuse();

            void OnSuccessfulRecovery(__in LONG64 recoveredLsn);

            ktl::Awaitable<void> WaitForCopyAndReplicationDrainToCompleteAsync();

            ktl::Awaitable<void> WaitForCopyDrainToCompleteAsync();

            void StartBuildingSecondary();

            __declspec(property(get = get_ReadConsistentAfterLsn)) LONG64 ReadConsistentAfterLsn;
            LONG64 get_ReadConsistentAfterLsn() const
            {
                return readConsistentAfterLsn_;
            }

            void Test_SetTestHookContext(TestHooks::TestHookContext const &);

        private:

            // This is used primarly in the copy drain logic where a batch of copy operations must synchronize on an atomic count before ack-ing the upper level operation
            // However, since using atomic is not possible (due to the variable going out of scope after 1 copy operation gets drained), we need a shared pointer version of it
            class SharedAtomicLong
                : public KObject<SharedAtomicLong>
                , public KShared<SharedAtomicLong>
            {
                K_FORCE_SHARED(SharedAtomicLong)

            public:
                static SharedAtomicLong::SPtr Create(
                    __in long startingCount,
                    __in KAllocator & allocator);

                long DecrementAndGet();

            private:
                SharedAtomicLong(__in long startingCount);

                Common::atomic_long long_;
            };

            SecondaryDrainManager(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in RecoveredOrCopiedCheckpointState & recoveredOrCopiedCheckpointState,
                __in RoleContextDrainState & roleContextDrainState,
                __in OperationProcessor & recordsProcessor,
                __in IStateReplicator & fabricReplicator,
                __in IBackupManager & backupManager,
                __in CheckpointManager & checkpointManager,
                __in TransactionManager & transactionManager,
                __in ReplicatedLogManager & replicatedLogManager,
                __in TxnReplicator::IStateProviderManager & stateManager,
                __in RecoveryManager & recoveryManager,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in LogRecordLib::InvalidLogRecords & invalidLogRecords);

            static void ProcessDuplicateRecord(__in LogRecordLib::LogRecord & record);

            void InitializeCopyAndReplicationDrainTcs();

            ktl::Task BuildSecondaryAsync();

            ktl::Awaitable<NTSTATUS> BuildSecondaryInternalAsync();

            bool CopiedUpdateEpoch(__in LogRecordLib::UpdateEpochLogRecord & record);

            ktl::Awaitable<NTSTATUS> CopyOrBuildReplicaAsync(__in IOperationStream & copyStream);

            ktl::Awaitable<NTSTATUS> DrainCopyStreamAsync(
                __in IOperationStream & copyStream,
                __in IOperation * operation,
                __in LogRecordLib::BeginCheckpointLogRecord * copiedCheckpointRecord,
                __in bool renamedCopyLogSuccessfully);

            ktl::Awaitable<NTSTATUS> DrainReplicationStreamAsync(__in IOperationStream & replicationStream);

            ktl::Awaitable<NTSTATUS> DrainStateStreamAsync(
                __in IOperationStream & copyStateStream,
                __out IOperation::SPtr & result);

            ktl::Awaitable<void> LogLogicalRecordOnSecondaryAsync(__in LogRecordLib::LogicalLogRecord & record);

            void StartLoggingOnSecondary(__in LogRecordLib::LogicalLogRecord & record);

            ktl::Awaitable<NTSTATUS> TruncateTailAsync(__in LONG64 tailLsn);

            ktl::Awaitable<NTSTATUS> TruncateTailIfNecessary(
                __in IOperationStream & copyStream,
                __out IOperation::SPtr & result);

            ktl::Task AwaitFlushingCopiedRecordTask(
                __in LogRecordLib::LogicalLogRecord & copiedRecord,
                __inout SharedAtomicLong & flushesOutstanding,
                __in ktl::AwaitableCompletionSource<void> & allRecordsFlushedTcs);

            ktl::Task AcknowledgeCopyOperationTask(
                __in ktl::AwaitableCompletionSource<void> & allRecordsFlushedTcs,
                __in IOperation & operation,
                __inout Common::atomic_long & acksOutstanding,
                __in ktl::AwaitableCompletionSource<void> & allOperationsAckedTcs);

            ktl::Task AwaitFlushingReplicatedRecordTask(
                __in LogRecordLib::LogicalLogRecord & replicatedRecord,
                __in IOperation & operation,
                __in LONG64 operationSize,
                __inout Common::atomic_long & acksOutstanding,
                __inout Common::atomic_long & bytesOutstanding,
                __in ktl::AwaitableCompletionSource<void> & allOperationsAckedTcs);

            ReplicatedLogManager::SPtr const replicatedLogManager_;
            CheckpointManager::SPtr const checkpointManager_;
            TransactionManager::SPtr const transactionManager_;
            IStateReplicator::SPtr const fabricReplicator_;
            IBackupManager::SPtr const backupManager_;
            TxnReplicator::IStateProviderManager::SPtr const stateManager_;
            RecoveryManager::SPtr const recoveryManager_;

            RoleContextDrainState::SPtr const roleContextDrainState_;
            OperationProcessor::SPtr const recordsProcessor_;
            LONG64 copiedUptoLsn_;

            LogRecordLib::InvalidLogRecords::SPtr const invalidLogRecords_;

            RecoveredOrCopiedCheckpointState::SPtr const recoveredOrCopiedCheckpointState_;

            // Pointer to a config object shared throughout this replicator instance
            TxnReplicator::TRInternalSettingsSPtr const transactionalReplicatorConfig_;

            ktl::AwaitableCompletionSource<void>::SPtr copyStreamDrainCompletionTcs_;
            ktl::AwaitableCompletionSource<void>::SPtr replicationStreamDrainCompletionTcs_;

            LONG64 readConsistentAfterLsn_;

            // Used for fault injection from FabricTest
            //
            TestHooks::TestHookContext testHookContext_;
        };
    }
}
