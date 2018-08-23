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
        // Responsible for replicating logical records and inserting into the log in LSN order
        // For physical records, replication is skipped
        //
        class ReplicatedLogManager final
            : public KObject<ReplicatedLogManager>
            , public KShared<ReplicatedLogManager>
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LR>
            , public IReplicatedLogManager
        {
            K_FORCE_SHARED(ReplicatedLogManager);
            K_SHARED_INTERFACE_IMP(IReplicatedLogManager);

        public:

            typedef KDelegate<LogRecordLib::BeginCheckpointLogRecord::SPtr(bool)> AppendCheckpointCallback;
            typedef KDelegate<bool(LogRecordLib::IndexingLogRecord &)> IsGoodLogHeadCandidateCalculator;

            static ReplicatedLogManager::SPtr Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in LogManager & logManager,
                __in RecoveredOrCopiedCheckpointState & recoveredrCopiedCheckpointState,
                __in_opt LogRecordLib::IndexingLogRecord * const currentHead,
                __in RoleContextDrainState & roleContextDrainState,
                __in IStateReplicator & stateReplicator,
                __in LogRecordLib::InvalidLogRecords & invalidLogRecords,
                __in KAllocator & allocator);

#ifdef DBG
            // This method is to ensure that the V2 replicator does not pass invalid data to either the V1 or the logger
            // If the user passes in NULL, the V2 replicator does the right conversions
            static void ValidateOperationData(__in Utilities::OperationData const & data);
#endif

            __declspec(property(get = get_CurrentLogHeadRecord)) LogRecordLib::IndexingLogRecord::SPtr CurrentLogHeadRecord;
            LogRecordLib::IndexingLogRecord::SPtr get_CurrentLogHeadRecord() const
            {
                return currentLogHeadRecord_.Get();
            }

            __declspec(property(get = get_CurrentLogTailEpoch)) TxnReplicator::Epoch CurrentLogTailEpoch;
            TxnReplicator::Epoch get_CurrentLogTailEpoch() const override
            {
                return currentLogTailEpoch_;
            }

            __declspec(property(get = get_CurrentLogTailLsn)) LONG64 CurrentLogTailLsn;
            LONG64 get_CurrentLogTailLsn() const override
            {
                return currentLogTailLsn_;
            }

            __declspec(property(get = get_LastCompletedBeginCheckpointRecord)) LogRecordLib::BeginCheckpointLogRecord::SPtr LastCompletedBeginCheckpointRecord;
            LogRecordLib::BeginCheckpointLogRecord::SPtr get_LastCompletedBeginCheckpointRecord() const
            {
                if (LastCompletedEndCheckpointRecord == nullptr)
                {
                    return nullptr;
                }

                return LastCompletedEndCheckpointRecord->LastCompletedBeginCheckpointRecord;
            }

            __declspec(property(get = get_LastCompletedEndCheckpointRecord)) LogRecordLib::EndCheckpointLogRecord::SPtr LastCompletedEndCheckpointRecord;
            LogRecordLib::EndCheckpointLogRecord::SPtr get_LastCompletedEndCheckpointRecord() const override
            {
                return lastCompletedEndCheckpointRecord_.Get();
            }

            __declspec(property(get = get_LastIndexRecord)) LogRecordLib::IndexingLogRecord::SPtr LastIndexRecord;
            LogRecordLib::IndexingLogRecord::SPtr get_LastIndexRecord() const
            {
                return lastIndexRecord_.Get();
            }

            __declspec(property(get = get_LastInformationRecord)) LogRecordLib::InformationLogRecord::SPtr LastInformationRecord;
            LogRecordLib::InformationLogRecord::SPtr get_LastInformationRecord() const override
            {
                return lastInformationRecord_.Get();
            }

            __declspec(property(get = get_LastInProgressCheckpointRecord)) LogRecordLib::BeginCheckpointLogRecord::SPtr LastInProgressCheckpointRecord;
            LogRecordLib::BeginCheckpointLogRecord::SPtr get_LastInProgressCheckpointRecord() const
            {
                return lastInProgressCheckpointRecord_.Get();
            }

            __declspec(property(get = get_LastInProgressTruncateHeadRecord)) LogRecordLib::TruncateHeadLogRecord::SPtr LastInProgressTruncateHeadRecord;
            LogRecordLib::TruncateHeadLogRecord::SPtr get_LastInProgressTruncateHeadRecord() const
            {
                return lastInProgressTruncateHeadRecord_.Get();
            }

            __declspec(property(get = get_LastLinkedPhysicalRecord)) LogRecordLib::PhysicalLogRecord::SPtr LastLinkedPhysicalRecord;
            LogRecordLib::PhysicalLogRecord::SPtr get_LastLinkedPhysicalRecord() const
            {
                return lastLinkedPhysicalRecord_.Get();
            }

            __declspec(property(get = get_InnerLogManager)) LogManager::SPtr InnerLogManager;
            LogManager::SPtr get_InnerLogManager() const
            {
                // Never re-assigned after construction. Safe to read
                return logManager_;
            }

            __declspec(property(get = get_ProgressVectorValue)) LogRecordLib::ProgressVector::SPtr ProgressVectorValue;
            LogRecordLib::ProgressVector::SPtr get_ProgressVectorValue() const override
            {
                LogRecordLib::ProgressVector::SPtr result;

                K_LOCK_BLOCK(progressVectorLock_)
                {
                    result = progressVector_;
                }

                return result;
            }

            __declspec(property(get = get_RoleContextDrainStateValue)) RoleContextDrainState::SPtr RoleContextDrainStateValue;
            RoleContextDrainState::SPtr get_RoleContextDrainStateValue() const
            {
                // Never re-assigned after construction. Safe to read
                return roleContextDrainState_;
            }

            // IStateReplicator::SPtr const stateReplicator_
            __declspec(property(get = get_StateReplicator)) IStateReplicator::SPtr const StateReplicator;
            IStateReplicator::SPtr get_StateReplicator() const
            {
                return stateReplicator_;
            }

            TxnReplicator::Epoch GetEpoch(
                __in FABRIC_SEQUENCE_NUMBER LSN) const override;

            LogRecordLib::IndexingLogRecord::CSPtr GetIndexingLogRecordForBackup() const override;

            NTSTATUS AppendBarrier(
                __in LogRecordLib::BarrierLogRecord & barrierLogRecord,
                __in bool isPrimary);

            LONG64 AppendWithoutReplication(
                __in LogRecordLib::LogicalLogRecord & record,
                __in_opt TransactionManager * const transactionManager);

            ktl::Awaitable<void> AwaitLsnOrderingTaskOnPrimaryAsync();

            ktl::Awaitable<NTSTATUS> AwaitReplication(__in LogRecordLib::LogicalLogRecord & logicalRecord);

            void CompleteCheckpoint();

            void EndCheckpoint(__in LogRecordLib::BeginCheckpointLogRecord & lastInProgressCheckpointRecord);

            ktl::Awaitable<void> FlushEndCheckpointAsync();

            ktl::Awaitable<void> FlushInformationRecordAsync(
                __in LogRecordLib::InformationEvent::Enum type,
                __in bool closeLog,
                __in KStringView const initiator);

            LogRecordLib::LogRecord::SPtr GetEarliestRecord();

            void Index();

            void Information(__in LogRecordLib::InformationEvent::Enum type) override;

            void OnCompletePendingCheckpoint();

            void OnCompletePendingLogHeadTruncation();

            void OnTruncateTailOfLastLinkedPhysicalRecord();

            NTSTATUS ReplicateAndLog(
                __in LogRecordLib::LogicalLogRecord & record,
                __out LONG64 & bufferedRecordSizeBytes,
                __in_opt TransactionManager * const transactionManager) override;

            void Reuse();

            void Reuse(
                __in LogRecordLib::ProgressVector & progressVector,
                __in_opt LogRecordLib::EndCheckpointLogRecord * const lastCompletedEndCheckpointRecord,
                __in_opt LogRecordLib::PhysicalLogRecord * const lastLinkedPhysicalRecord,
                __in_opt LogRecordLib::InformationLogRecord * const lastInformationRecord,
                __in LogRecordLib::IndexingLogRecord & currentLogHeadRecord,
                __in TxnReplicator::Epoch const & tailEpoch,
                __in LONG64 tailLsn);

            void SetCheckpointCallback(__in AppendCheckpointCallback const & appendCheckpointCallback);

            void SetTailEpoch(__in TxnReplicator::Epoch const & epoch);

            void SetTailLsn(__in LONG64 lsn);

            void InsertBeginCheckpoint(__in LogRecordLib::BeginCheckpointLogRecord & record);

            void InsertTruncateHeadCallerHoldsLock(
                __in LogRecordLib::TruncateHeadLogRecord & record,
                __in LogRecordLib::IndexingLogRecord & headRecord);

            void OnReplicationError(
                __in LogRecordLib::LogicalLogRecord const & record,
                __in NTSTATUS errorCode);

            LogRecordLib::TruncateHeadLogRecord::SPtr TruncateHead(
                __in bool isStable,
                __in LONG64 lastPeriodicTruncationTimeStampTicks,
                __in IsGoodLogHeadCandidateCalculator & isGoodLogHeadCandidateCalculator);

            void TruncateTail(__in LONG64 tailLsn);

            void UpdateEpochRecord(
                __in LogRecordLib::UpdateEpochLogRecord & record);

        private:

            ReplicatedLogManager(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in LogManager & logManager,
                __in RecoveredOrCopiedCheckpointState & recoveredrCopiedCheckpointState,
                __in_opt LogRecordLib::IndexingLogRecord * const currentHead,
                __in RoleContextDrainState & roleContextDrainState,
                __in IStateReplicator & stateReplicator,
                __in LogRecordLib::InvalidLogRecords & invalidLogRecords);

            void AddCheckpointIfNeededCallerHoldsLock(__in bool isPrimary);

            LONG64 Append(
                __in LogRecordLib::LogicalLogRecord & record,
                __in_opt TransactionManager * const transactionManager,
                __in bool isPrimary);

            void BeginCheckpointCallerHoldsLock(__in LogRecordLib::BeginCheckpointLogRecord & record);

            void EndCheckpointCallerHoldsLock(__in LogRecordLib::BeginCheckpointLogRecord & lastInProgressCheckpointRecord);

            LogRecordLib::LogRecord::SPtr GetEarliestRecordCallerHoldsLock();

            void InformationCallerHoldsLock(__in LogRecordLib::InformationEvent::Enum type);

            LONG64 InsertBufferedRecordCallerHoldsLock(
                __in LogRecordLib::LogicalLogRecord & record,
                __in bool isPrimary,
                __out LONG & pendingCount);

            LONG64 LsnOrderingInsertCallerHoldsLock(
                __in LogRecordLib::LogicalLogRecord & record,
                __in bool isPrimary,
                __out ktl::AwaitableCompletionSource<void>::SPtr & lsnOrderingTcsToSignal);

            void OnBarrierBufferedCallerHoldsLock(
                __in LogRecordLib::BarrierLogRecord & record,
                __in bool isPrimary);

            void OnOperationAcceptance();

            void OnOperationAcceptanceException(); // We are not inside lsnorderinglock_ here

            LONG OnOperationLogInitiationCallerHoldsLock();

            NTSTATUS BeginReplicate(__in LogRecordLib::LogicalLogRecord & record);

            LogManager::SPtr const logManager_;
            RoleContextDrainState::SPtr const roleContextDrainState_;
            IStateReplicator::SPtr const stateReplicator_;
            RecoveredOrCopiedCheckpointState::SPtr const recoveredOrCopiedCheckpointState_;
            LogRecordLib::InvalidLogRecords::SPtr const invalidLogRecords_;

            mutable KSpinLock lsnOrderingLock_;
            KArray<LogRecordLib::LogicalLogRecord::SPtr> lsnOrderingQueue_;
            ktl::AwaitableCompletionSource<void>::SPtr lsnOrderingTcs_;
            Common::atomic_long operationAcceptedCount_;
            AppendCheckpointCallback appendCheckpointCallback_;

            Data::Utilities::ThreadSafeSPtrCache<LogRecordLib::IndexingLogRecord> currentLogHeadRecord_;
            TxnReplicator::Epoch currentLogTailEpoch_;
            volatile LONG64 currentLogTailLsn_;
            Data::Utilities::ThreadSafeSPtrCache<LogRecordLib::EndCheckpointLogRecord> lastCompletedEndCheckpointRecord_;
            Data::Utilities::ThreadSafeSPtrCache<LogRecordLib::IndexingLogRecord> lastIndexRecord_;
            Data::Utilities::ThreadSafeSPtrCache<LogRecordLib::InformationLogRecord> lastInformationRecord_;
            Data::Utilities::ThreadSafeSPtrCache<LogRecordLib::BeginCheckpointLogRecord> lastInProgressCheckpointRecord_;
            Data::Utilities::ThreadSafeSPtrCache<LogRecordLib::TruncateHeadLogRecord> lastInProgressTruncateHeadRecord_;
            Data::Utilities::ThreadSafeSPtrCache<LogRecordLib::PhysicalLogRecord> lastLinkedPhysicalRecord_;

            // It is possible InitiateCheckpoint method is trying to read progress vector while update epoch is invoked in parallel
            // Hence need to protect it
            mutable KSpinLock progressVectorLock_;
            LogRecordLib::ProgressVector::SPtr progressVector_;
        };
    }
}
